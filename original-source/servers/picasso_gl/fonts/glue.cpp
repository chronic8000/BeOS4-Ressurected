#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <support2/Debug.h>

using namespace B::Support2;

#include "as_support.h"
#include "basic_types.h"
#include "font_renderer.h"
#include "font_cache.h"
#include "t2k.h"
#include "path.h"
#include "settings.h"

// Turn this on to use FontFusion's algorithmic bolding.
// Turned off because the current FontFusion algorithm sucks.
// (We do our own bitmap-based bolding instead.)
#define USE_FF_BOLDING 0

typedef struct fc_renderer_context {
	uint32			length;
	uchar			file_type;
	uint8			grey_scale_quality;
	uint16			algo_faces;
	uint8			bpp;
	uint8			hinting;
	uint8			gray_level;
	uint8			default_flag;
	int32			size;
	int32			matrix[4];
	uint16			command;
	uint16			token;
	short			fontType;
	tsiMemObject	*mem;
	InputStream 	*in;
	sfntClass 		*font;
	T2K 			*scaler;
	FILE			*fp;
} fc_renderer_context;

static uint8 aa_quality();
static uint8* gamma_table();
static int32 bold_factor(fc_renderer_context *ctxt, uint8 grey_scale_quality);
static void dump_metrics(T2K *t2k);

void fc_flush_cache(int32 /*clientID*/) {
}

void fc_init_diffusion_table() {
}

static void reset(fc_renderer_context *ctxt) {
	//fprintf(stderr, "Reset fc_render_context: %p\n", ctxt);
	ctxt->length = 0;
	ctxt->file_type = FC_BAD_FILE;
	ctxt->bpp = 0;
	ctxt->hinting = 0;
	ctxt->grey_scale_quality = 0;
	ctxt->algo_faces = 0;
	ctxt->gray_level = 0;
	ctxt->default_flag = 0;
	ctxt->command = 0;
	ctxt->token = 0xffff;
	ctxt->fontType = -1;
	ctxt->mem = NULL;
	ctxt->in = NULL;
	ctxt->font = NULL;
	ctxt->scaler = NULL;
	ctxt->fp = NULL;
}

static bool cleanup_font(struct fc_renderer_context *ctxt) {
	int		errCode;

	//fprintf(stderr, "Clean up fc_render_context: %p\n", ctxt);
	if (ctxt->scaler) {
		DeleteT2K(ctxt->scaler, &errCode);
		if (errCode != 0)
			goto error;	
	}
	if (ctxt->font) {
		Delete_sfntClass(ctxt->font, &errCode);
		if (errCode != 0)
			goto error;	
	}
	return true;
	
error:
	reset(ctxt);
	return false;
}

static void clean_up(fc_renderer_context *ctxt) {
	int		errCode;

	//fprintf(stderr, "Clean up fc_render_context: %p\n", ctxt);
	
	if (!cleanup_font(ctxt))
		goto error;
		
	if (ctxt->in != NULL) {
		Delete_InputStream(ctxt->in, &errCode);
		if (errCode != 0)
			goto error;	
	}
	if (ctxt->mem != NULL)
		tsi_DeleteMemhandler(ctxt->mem);
error:
	reset(ctxt);
}

struct fc_renderer_context *fc_create_new_context() {
	fc_renderer_context		*ctxt;

	ctxt = (struct fc_renderer_context *)malloc(sizeof(fc_renderer_context));
	reset(ctxt);
	return ctxt;
};

static int pf_read_to_ram(void *id, uint8 *dest_ram, uint32 offset, int32 numBytes) {
	fc_renderer_context	*ctxt;
	
	ctxt = (fc_renderer_context*) id;
	fseek(ctxt->fp, offset, SEEK_SET);
	if (fread(dest_ram, 1, numBytes, ctxt->fp) != (size_t)numBytes)
		return -1;
	return 0;
}

static bool build_font(struct fc_renderer_context *ctxt) {
	int				errCode;
	
	/* Create an sfntClass object*/
#if USE_FF_BOLDING
	T2K_AlgStyleDescriptor style;
	if (ctxt->algo_faces&B_BOLD_FACE) {
		style.StyleFunc			= tsi_SHAPET_BOLD_GLYPH;
		style.StyleMetricsFunc	= tsi_SHAPET_BOLD_METRICS;
		if (ctxt->size >= 18)
			style.params[0] = 6L << 14;
		else if (ctxt->size >= 8)
			style.params[0] = (6L<<14) + ((18-ctxt->size) << 13);
		else
			style.params[0] = (6L<<14) + (10<<13);
	} else {
		style.StyleFunc = NULL;
	}
	ctxt->font = New_sfntClass(ctxt->mem, ctxt->fontType, ctxt->in,
								style.StyleFunc ? &style : NULL, &errCode);
#else
	ctxt->font = New_sfntClass(ctxt->mem, ctxt->fontType, ctxt->in, NULL, &errCode);
#endif
	if (errCode != 0)
		return false;

	/* Create a T2K font scaler object.  */
	ctxt->scaler = NewT2K(ctxt->mem, ctxt->font, &errCode);
	if (errCode != 0)
		return false;
	Set_PlatformID(ctxt->scaler, 3);
	Set_PlatformSpecificID(ctxt->scaler, 1);
	FF_SetRemapTable(ctxt->scaler, gamma_table());
	return true;
}

void fc_set_file(struct fc_renderer_context *ctxt, FILE *fpID, uint16 token) {
	int				errCode;
	
	if (ctxt->token == token) {
		/* The scaler is already initialized, but update the file */
		ctxt->fp = fpID;
		return;
	}

	/* Get rid of whatever may be left in the context */
	clean_up(ctxt);

	//fprintf(stderr, "Set file fc_render_context: %p, token=0x%x\n", ctxt, token);
	
	ctxt->file_type = file_table[token].file_type;
	
	/* Read the font into memory. */
	errCode	= fseek(fpID, 0L, SEEK_END);
	if (errCode != 0)
		goto clean_up_exit;
	ctxt->length = (unsigned long)ftell(fpID);
	errCode = ferror(fpID);
	if (errCode != 0)
		goto clean_up_exit;
		
	/* reset the offset */
	fseek(fpID, 0L, SEEK_SET);
	ctxt->fp = fpID;

	/* Create the Memhandler object. */
	ctxt->mem = tsi_NewMemhandler(&errCode);
	if (errCode != 0)
		goto reset_exit;
		
	/* Create the InputStream object, with data already in memory.  We
	   always do this for type1 fonts because FontFusion seems to require
	   that.  We always do this for stroke fonts because rendering from
	   disk is -very- slow due to their disk IO. */
	if (loadEntireFontFile || ctxt->file_type == FC_TYPE1_FONT ||
			ctxt->file_type == FC_STROKE_FONT) {
		void *data = tsi_AllocMem(ctxt->mem, ctxt->length);
		if (fread(data, 1, ctxt->length, fpID) != ctxt->length) {
			tsi_DeAllocMem(ctxt->mem, data);
			goto reset_exit;
		}

		fseek(fpID, 0L, SEEK_SET);		
		ctxt->in = New_InputStream(ctxt->mem, (unsigned char*)data, ctxt->length, &errCode);
		if (errCode != 0) {
			tsi_DeAllocMem(ctxt->mem, data);
			goto reset_exit;
		}		
	} else {
		ctxt->in = New_NonRamInputStream(ctxt->mem, (void*)ctxt, pf_read_to_ram, ctxt->length, &errCode);
		if (errCode != 0)
			goto reset_exit;
	}

	ctxt->fontType = FF_FontTypeFromStream(ctxt->in);
	if (ctxt->fontType == -1)
		goto clean_up_exit;

	if (!build_font(ctxt))
		goto reset_exit;
		
	/* succesful exit */
	ctxt->token = token;
	ctxt->bpp = 0xff;
	return;

	/* failure exit, clean_up required for FontFusion */
clean_up_exit:
	clean_up(ctxt);
	return;

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
	return;
};

int64 fc_context_weight(struct fc_renderer_context *ctxt)
{
	return (int64(ctxt->length)*(ctxt->file_type == FC_STROKE_FONT ? 500 : 20))/100;
}

static
int setup_font(struct fc_renderer_context *ctxt, fc_context *spec,
			   int gray_level, int default_flag, bool /*postscript*/)
{
	int					errCode;
	T2K_TRANS_MATRIX	trans;
	
	if (ctxt->mem == NULL)
		return 0;
	
	const fc_matrix* const matrix = spec->matrix();
	
	/* lazy modification */
	if ((ctxt->algo_faces == spec->params()->algo_faces()) &&
		(ctxt->bpp == (uint8)spec->bits_per_pixel()) &&
		(ctxt->hinting == spec->hinting()) &&
		(ctxt->gray_level == (uint8)gray_level) &&
		(ctxt->default_flag == (uint8)default_flag) &&
		(ctxt->size == spec->size()) &&
		(ctxt->matrix[0] == matrix->xxmult) &&
		(ctxt->matrix[1] == matrix->yxmult) &&
		(ctxt->matrix[2] == matrix->xymult) &&
		(ctxt->matrix[3] == matrix->yymult))
		return 1;

	ctxt->bpp = (uint8)spec->bits_per_pixel();
	ctxt->hinting = spec->hinting();
	ctxt->gray_level = gray_level;
	ctxt->default_flag = default_flag;
	
#if USE_FF_BOLDING
	if ((ctxt->algo_faces != spec->params()->algo_faces() ||
			((spec->params()->algo_faces()&B_BOLD_FACE) != 0
				&& ctxt->size != spec->size()))
			&& ctxt->in != NULL) {
		/* Nasty case: the algorithmic faces have changed, so we need
		   to regenerate the font class if we are already using one */
		ctxt->algo_faces = spec->params()->algo_faces();
		ctxt->size = spec->size();
		if (!cleanup_font(ctxt))
			goto reset_exit;
		if (!build_font(ctxt))
			goto reset_exit;	
	}
#else
	ctxt->algo_faces = spec->params()->algo_faces();
#endif

	if ((ctxt->algo_faces&B_ITALIC_FACE) != 0) {
		// Here is a shearing matrix to simulate italics.
		// The complicated value is tan(12 degrees).
		static const fc_matrix shear = {
			65536,	(int32)(0.218*65536),
			0,		65536,
			0,		0
		};
		
		// Multiply shear with requested transformation matrix.
		fc_matrix out;
		fc_multiply(&out, &shear, matrix);
		ctxt->matrix[0] = out.xxmult;
		ctxt->matrix[1] = out.yxmult;
		ctxt->matrix[2] = out.xymult;
		ctxt->matrix[3] = out.yymult;
	} else {
		ctxt->matrix[0] = matrix->xxmult;
		ctxt->matrix[1] = matrix->yxmult;
		ctxt->matrix[2] = matrix->xymult;
		ctxt->matrix[3] = matrix->yymult;
	}
	
	ctxt->size = spec->size();
	
	//fprintf(stderr, "Reset fc_render_context: %p\n", ctxt);
	//fprintf(stderr, "Setting up: token=%d, grid=%d\n", ctxt->token, grid_fit);
	
	/* Set the transformation */
	trans.t00 = ctxt->matrix[0];
	trans.t01 = ctxt->matrix[1];
	trans.t10 = ctxt->matrix[2];
	trans.t11 = ctxt->matrix[3];
//	printf("Matrix:%08x, %08x, %08x, %08x\n", matrix[0], matrix[1], matrix[2], matrix[3]);
	T2K_NewTransformation(ctxt->scaler, true, 72, 72, &trans, true, &errCode);
	if (errCode != 0)
		goto reset_exit;		
	
	/* setup grey scale quality and options */
	if (spec->bits_per_pixel() == 1)
		ctxt->grey_scale_quality = BLACK_AND_WHITE_BITMAP;		
	else
		ctxt->grey_scale_quality = aa_quality();

	/* Make stroke fonts a little darker weight */
	if (ctxt->file_type == FC_STROKE_FONT && T2K_GetNumAxes(ctxt->scaler) == 1) {
		/* The ideal stroke weight is 0.5, but at small point sizes this is
		   too light.  Adjust the weight at small sizes to attempt to keep
		   lines at least one pixel width in size. */
		int weight = 50;
		if (spec->size() < 25) {
			weight = 50 + ((25-spec->size())*4)/2;
			if (weight > 100)
				weight = 100;
		}
	    T2K_SetCoordinate(ctxt->scaler, 0, (0x10000*weight)/100);
	}

	ctxt->command = T2K_SCAN_CONVERT;

	// Note that GRID_FIT or NAT_GRID_FIT do not work with tv mode.
	if (ctxt->bpp == FC_TV_SCALE)
		ctxt->command |= (ctxt->hinting ? T2K_TV_MODE_2 : T2K_TV_MODE);
	else if (ctxt->hinting)
		ctxt->command |= T2K_GRID_FIT | T2K_NAT_GRID_FIT;

	if (default_flag == 1)
		ctxt->command |= T2K_CODE_IS_GINDEX;

	return 1;		

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
	return 0;
}

int fc_setup_tt_font(	struct fc_renderer_context *ctxt, fc_context *spec,
						int gray_level, int default_flag)
{
	return setup_font(ctxt, spec, gray_level, default_flag, false);
}

int fc_setup_stroke_font(	struct fc_renderer_context *ctxt, fc_context *spec,
							int gray_level, int default_flag)
{
	return setup_font(ctxt, spec, gray_level, default_flag, false);
}

int fc_setup_ps_font(	struct fc_renderer_context *ctxt, fc_context *spec,
						int gray_level, int default_flag)
{
	return setup_font(ctxt, spec, gray_level, default_flag, true);
}
						 
void fc_release_font(struct fc_renderer_context */*ctxt*/)
{
//	clean_up(ctxt);
}

static size_t generate_bw_bitmap(uint8* dest, const uint8* src,
								const size_t w, const size_t h,
								const size_t rowBytes, int32 bolding);

static inline int32 round_26dot6(F26Dot6 val)
{
	double frac = ((double)val) * (1.0/64.0);
	if (frac >= 0) return (int32)(frac+.5);
	return (int32)(frac-.5);
}

static inline double get_16dot16(F16Dot16 val)
{
	return ((double)val) * (1.0/65536.0);
}

static inline int32 round_16dot16(F16Dot16 val)
{
	double frac = ((double)val) * (1.0/65535.0);
	if (frac >= 0) return (int32)(frac+.5);
	return (int32)(frac-.5);
}

int fc_make_char(struct fc_renderer_context *ctxt, void *char_desc, void *set, FC_GET_MEM get_mem) {
	int			errCode;
	int32		i, j, len;
	uint8		*ptr, *bits;
	fc_edge     *edge;
	fc_rect     *bound;
	fc_escape   *escape;
	
	if (ctxt->mem == NULL)
		return 0;
	
	/* render the glyph using font fusion */	
	T2K_RenderGlyph(ctxt->scaler, *(ushort*)char_desc, 0, 0, ctxt->grey_scale_quality,
					ctxt->command, &errCode);
	if (errCode != 0) {
		_sPrintf("fc_make_char failed, error code %d\n", errCode);
		goto reset_exit;
	}
	
	{
		const int32 width = ctxt->scaler->width;
		const int32 height = ctxt->scaler->height;
		const int32 delta = bold_factor(ctxt, ctxt->grey_scale_quality);
		
		const int32 row = ctxt->scaler->rowBytes;
			
		/* create the character header in font cache format */
			/* allocate memory for char header (except edges).
			   Reserved space for spacing information. */
			ptr = (uint8*)(get_mem)(set, sizeof(fc_edge)+sizeof(fc_rect)+
								   sizeof(fc_escape)+sizeof(fc_spacing));
			/* calculate edges in left relative coordinates */
			edge = (fc_edge*)ptr;
			edge->left = 1234567.0;
			edge->right = 1234568.0;
			/* record character bounding box */
			bound = (fc_rect*)(ptr+sizeof(fc_edge));
			bound->top = -round_26dot6(ctxt->scaler->fTop26Dot6);
			bound->left = round_26dot6(ctxt->scaler->fLeft26Dot6);
			bound->right = bound->left + width+delta-1;
			bound->bottom = bound->top + height-1;
			/* record escapement infos */
			escape = (fc_escape*)(ptr+sizeof(fc_edge)+sizeof(fc_rect));
	
			escape->x_escape = float(get_16dot16(ctxt->scaler->xLinearAdvanceWidth16Dot16));
			escape->y_escape = -float(get_16dot16(ctxt->scaler->yLinearAdvanceWidth16Dot16));
			escape->x_bm_escape = round_16dot16(ctxt->scaler->xAdvanceWidth16Dot16);
			escape->y_bm_escape = -round_16dot16(ctxt->scaler->yAdvanceWidth16Dot16);
			
			if (delta > 0) {
				const float escape_sum = fabs(escape->x_escape) + fabs(escape->y_escape);
				const int32 escape_bm_sum = abs(escape->x_bm_escape) + abs(escape->y_bm_escape);
				if (escape_sum > 0) {
					escape->x_escape += (delta*escape->x_escape)/escape_sum;
					escape->y_escape += (delta*escape->y_escape)/escape_sum;
					escape->x_bm_escape += (delta*escape->x_bm_escape)/escape_bm_sum;
					escape->y_bm_escape += (delta*escape->y_bm_escape)/escape_bm_sum;
				}
			}
			
			bits = (uint8*)ctxt->scaler->baseAddr;
			
			#if 0
			printf("Char %c@%ld: edge=(%.2f,%.2f), tl=(0x%lx.%lx,0x%lx.%lx), b=(%d,%d)-(%d,%d), esc=(%.2f,%.2f)\n",
					(char)*(ushort*)char_desc, ctxt->size, edge->left, edge->right,
					ctxt->scaler->fLeft26Dot6>>6, ctxt->scaler->fLeft26Dot6&((1<<6)-1),
					ctxt->scaler->fTop26Dot6>>6, ctxt->scaler->fTop26Dot6&((1<<6)-1),
					bound->left, bound->top, bound->right, bound->bottom,
					escape->x_escape, escape->y_escape);
			printf("Advance width=(0x%lx.%lx,0x%lx.%lx), linear=(0x%lx.%lx,0x%lx.%lx), abs=(0x%lx.%lx,0x%lx.%lx)\n",
					ctxt->scaler->xAdvanceWidth16Dot16>>16,
					ctxt->scaler->xAdvanceWidth16Dot16&((1<<16)-1),
					ctxt->scaler->yAdvanceWidth16Dot16>>16,
					ctxt->scaler->yAdvanceWidth16Dot16&((1<<16)-1),
					ctxt->scaler->xLinearAdvanceWidth16Dot16>>16,
					ctxt->scaler->xLinearAdvanceWidth16Dot16&((1<<16)-1),
					ctxt->scaler->yLinearAdvanceWidth16Dot16>>16,
					ctxt->scaler->yLinearAdvanceWidth16Dot16&((1<<16)-1),
					(ctxt->scaler->xLinearAdvanceWidth16Dot16/ctxt->size)>>16,
					(ctxt->scaler->xLinearAdvanceWidth16Dot16/ctxt->size)&((1<<16)-1),
					(ctxt->scaler->yLinearAdvanceWidth16Dot16/ctxt->size)>>16,
					(ctxt->scaler->yLinearAdvanceWidth16Dot16/ctxt->size)&((1<<16)-1));
			#endif
			
		/* extract the bitmap itself, and convert it to font cache format */
		if (ctxt->grey_scale_quality == BLACK_AND_WHITE_BITMAP) {
			/* calculate the total length */
			len = (int32)generate_bw_bitmap(NULL, bits, width, height, row, delta);
			
			/* create a black and white compressed bitmap */
			ptr = (uint8*)(get_mem)(set, len);
			generate_bw_bitmap(ptr, bits, width, height, row, delta);
		}
		else {
			/* create a grayscale bitmap */
			uint8 data8;
			ptr = (uint8*)(get_mem)(set, height*(width+delta+1)/2);
			if (delta <= 0) {
				for (j=0; j<height; j++) {
					data8 = 0;
					for (i=0; i<width; i++) {
						data8 = (bits[i]>>4) | (data8<<4);
						//printf("Font bit value: %d (0x%x), data=0x%02x\n",
						//		bits[i], bits[i], data8);
						if (i&1)
							*ptr++ = data8;
					}
					if (i&1)
						*ptr++ = data8<<4;
					bits += row;
				}
			} else {
				int32 sum;
				for (j=0; j<height; j++) {
					data8 = 0;
					sum = 0;
					for (i=0; i<(width+delta); i++) {
						if (i < width)
							sum += bits[i];
						if (sum > 127)
							data8 = 0x7 | (data8<<4);
						else
							data8 = (sum>>4) | (data8<<4);
						if (i >= delta)
							sum -= bits[i-delta];
						//printf("Font bit value: %d (0x%x), data=0x%02x\n",
						//		bits[i], bits[i], data8);
						if (i&1)
							*ptr++ = data8;
					}
					if (i&1)
						*ptr++ = data8<<4;
					bits += row;
				}
			}
		}
	}
	
	/* we are done with the FontFusion buffers */
	T2K_PurgeMemory(ctxt->scaler, 1, &errCode );
	if (errCode != 0)
		goto reset_exit;

	return 1;

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
	return 0;
}

// An attempt at unifying this code, which is currently broken.
static size_t generate_bw_bitmap(uint8* dest, const uint8* src,
								 const size_t w, const size_t h,
								 const size_t rowBytes, const int32 bolding)
{
	const int32 fw = w+bolding;
	
	size_t x, y;
	size_t count;
	int32 sum;
	
	uint8* pos = dest;
	
	/* create a black and white compressed bitmap */
	for (y=0; y<h; y++) {
		x=0;
		while (x < (size_t)fw) {
		
			/* count white pixels */
			count = 0;
			while (x < w && (src[x>>3] & (128>>(x&7))) == 0) {
				count++;
				x++;
			}
			
			/* a line that ends early is assumed to be filed with empty pixels */
			if (x >= w)
				break;
			
			/* store packed count of white pixels */
			if (dest) {
				while (count >= 254) {
					*pos++ = 254;
					count -= 254;
				}
				*pos++ = count;
			} else {
				pos += (count/254) + 1;
			}
			
			/* count black pixels, spreading by the bold factor */
			count = 0;
			sum = 0;
			do {
				if (x < w && (src[x>>3] & (128>>(x&7))) != 0)
					sum++;
				if (sum <= 0)
					break;
				if (x >= (size_t)bolding && (src[(x-bolding)>>3] & (128>>((x-bolding)&7))) != 0)
					sum--;
				count++;
				x++;
			} while (x < (size_t)fw);
			
			/* store packed count of black pixels */
			if (count > 0 || x < (size_t)fw) {
				if (dest) {
					while (count >= 254) {
						*pos++ = 254;
						count -= 254;
					}
					*pos++ = count;
				} else {
					pos += (count/254) + 1;
				}
			}
		}
		
		/* store line terminator, skip to next line */
		if (dest)
			*pos++ = 255;
		else
			pos++;
		src += rowBytes;
	}
	
	return (size_t)(pos-dest);
}

void fc_make_safety_char(struct fc_renderer_context */*ctxt*/, float size, int bpp, void *set, FC_GET_MEM get_mem) {
	int         i, j;
	int         hmin, hmax, dv, dh;
	char        *ptr;
	fc_edge     *edge;
	fc_rect     *bound;
	fc_escape   *escape;
	
	if (size > 250.0) size = 250.0;
	if (size < 5.0) size = 5.0;
	hmin = (int)(size*0.08+0.5);
	hmax = (int)(size*0.50+0.5);
	dv = (int)(size*0.68+0.5);
	/* allocate memory for char header (except edges).
	   Reserved space for spacing information. */
	ptr = (char*)(get_mem)(set, sizeof(fc_edge)+sizeof(fc_rect)+
						   sizeof(fc_escape)+sizeof(fc_spacing));
	/* calculate edges in left relative coordinates */
	edge = (fc_edge*)ptr;
	edge->left = size*0.08;
	edge->right = size*0.50;
	/* record character bounding box */
	bound = (fc_rect*)(ptr+sizeof(fc_edge));
	bound->left = hmin;
	bound->bottom = 0;
	bound->right = hmax-1;
	bound->top = 1-dv;
	/* record escapement infos */
	escape = (fc_escape*)(ptr+sizeof(fc_edge)+sizeof(fc_rect));
	escape->x_escape = size*0.58;
	escape->y_escape = 0.0;
	/* create a black and white box */
	if (bpp == FC_BLACK_AND_WHITE) {
		ptr = (char*)(get_mem)(set, 5*dv-4);
		for (i=0; i<dv; i++) {
			if ((i==0) || (i==(dv-1))) {
				ptr[0] = 0;
				ptr[1] = hmax-hmin+1;
				ptr[2] = 0xff;
				ptr += 3;
			}
			else {
				ptr[0] = 0;
				ptr[1] = 1;
				ptr[2] = hmax-hmin-1;
				ptr[3] = 1;
				ptr[4] = 0xff;
				ptr += 5;
			}
		}
	}
	/* create a grayscale box */
	else {		
		dh = (hmax-hmin+1)>>1;
		hmax = (hmax-hmin)&1;
		ptr = (char*)(get_mem)(set, dv*dh);
		for (i=0; i<dv; i++) {
			if ((i==0) || (i==(dv-1))) {
				for (j=0; j<dh; j++)
					ptr[j] = 0x77;
			}
			else {
				ptr[0] = 0x70;
				for (j=1; j<dh-1; j++)
					ptr[j] = 0x00;
				if (hmax)
					ptr[dh-1] = 0x70;
				else
					ptr[dh-1] = 0x07;
			}
			ptr += dh;
		}		
	}
}

int fc_make_outline(struct fc_renderer_context *ctxt, void *char_desc, void **glyph_path) {
	int			errCode;
	SPath		*gpath;
	GlyphClass	*glyph;

	if (ctxt->mem == NULL)
		goto exit;
		
	/* render the glyph using font fusion */
	T2K_RenderGlyph(ctxt->scaler, *(ushort*)char_desc, 0, 0, ctxt->grey_scale_quality,
					(ctxt->command|T2K_RETURN_OUTLINES)&~T2K_SCAN_CONVERT, &errCode);
	if (errCode != 0)
		goto reset_exit;
	
	glyph = ctxt->scaler->glyph;
	
/*
	printf("%d contours\n", glyph->contourCount);
	for (iContour=0; iContour<glyph->contourCount; iContour++) {
		printf("Contour %d, %d points\n", iContour, glyph->ep[iContour]-glyph->sp[iContour]+1);
		for (iPoint=glyph->sp[iContour]; iPoint<=glyph->ep[iContour]; iPoint++)
			printf("Point (%f, %f), %d\n",
				   (float)glyph->x[iPoint]/64.0,
				   (float)glyph->y[iPoint]/64.0, (int32)glyph->onCurve[iPoint]);
	}
*/

	/* create the SPath object */
	{
		gpath = new SPath(NULL);
		gpath->Clear();

		/* generate path from contours.  this is based on Make2ndDegreeEdgeList
		   in t2ksc.c.  yes, it's ugly and gross and shows its frankensteinian
		   origins way too much, but hey it works. */
		
	    int startPoint, lastPoint, ctr;
	    int ptA, ptB, ptC=0;
		long Ax, Bx, Cx, Ay, By, Cy;
		fpoint pt[3];
		register long *x = glyph->x;
		register long *y = glyph->y;
		register uchar *onCurve = glyph->onCurve;
		enum shape_state {
			kShapeStart,
			kContourStart,
			kContourContinue
		};
		shape_state state = kShapeStart;
		
		//fprintf(stderr, "Building shape: glyph=%p, #c=%d, #p=%d, x=%p, y=%p\n",
		//		glyph, glyph->contourCount, glyph->pointCount, x, y);
		
		ctr = 0;
SC_startContour:
		while ( ctr < glyph->contourCount ) {
			//fprintf(stderr, "Now at contour #%d\n", ctr);
			ptA = startPoint = glyph->sp[ctr];
			lastPoint = glyph->ep[ctr];
			if ( onCurve[ ptA ] ) {
				Ax  = x[ ptA ];	Ay  = y[ ptA ];
				ptB = -1; Bx = By = 0;
			} else {
				Bx  = x[ ptA ]; By  = y[ ptA ];
				ptB = lastPoint;
	
				Ax  = x[ ptB ]; Ay = y[ ptB ];
				if ( !onCurve[ ptB ] ) {
					Ax = (Ax + Bx + 1) >> 1;
					Ay = (Ay + By + 1) >> 1;
				}
				ptB = ptA; ptA = lastPoint; /* SWAP A, B */
			}
			
			if (state != kShapeStart)
				state = kContourStart;
			
			for (;;) {
/* SC_AOnBOff: */
				while ( ptB >= 0 ) {	
					//fprintf(stderr, "Bezier ptA=%d, ptB=%d, ptC=%d\n", ptA, ptB, ptC);
					ptC = (ptB + 1); /* ptC = NEXTPT( ptB, ctr ); */
					if ( ptC > lastPoint ) ptC = startPoint;
					Cx = x[ ptC ];	Cy = y[ ptC ];
					if (state != kContourContinue) {
						pt[0].x = (float)Ax*(1/64.0);
						pt[0].y = (float)Ay*(1/64.0);
						if (state == kShapeStart)
							gpath->OpenShapes(pt[0]);
						else
							gpath->MoveTo(pt[0], true);
						//fprintf(stderr, "Starting contour: (%.2f,%.2f)\n",
						//		pt[0].h, pt[0].v);
						state = kContourContinue;
					}
					pt[0].x = (float)Ax*(1/64.0);
					pt[0].y = (float)Ay*(1/64.0);
					pt[1].x = (float)Bx*(1/64.0);
					pt[1].y = (float)By*(1/64.0);
					if ( ! onCurve[ ptC ]) {
						register long tmpX = ((Bx + Cx + 1) >> 1);
						register long tmpY = ((By + Cy + 1) >> 1);
						pt[2].x = (float)tmpX*(1/64.0);
						pt[2].y = (float)tmpY*(1/64.0);
						//fprintf(stderr, "Adding bezier: (%.2f,%.2f)-(%.2f,%.2f)\n",
						//		pt[1].h, pt[1].v, pt[2].h, pt[2].v);
						gpath->AddBezierSegments(pt, 1);
						if ( ptC == startPoint ) {ctr++; goto SC_startContour;}	/********************** continue SC_startContour */
						Ax = tmpX; Ay = tmpY;
						Bx = Cx; By = Cy; ptA = ptB; ptB = ptC;
						continue; /********************** continue SC_AOnBOff */
					}
					pt[2].x = (float)Cx*(1/64.0);
					pt[2].y = (float)Cy*(1/64.0);
					//fprintf(stderr, "Adding bezier: (%.2f,%.2f)-(%.2f,%.2f)\n",
					//		pt[1].h, pt[1].v, pt[2].h, pt[2].v);
					gpath->AddBezierSegments(pt, 1);
					if ( ptC == startPoint ) {ctr++; goto SC_startContour;}	/********************** continue SC_startContour */
					ptA = ptC; Ax = Cx; Ay = Cy;
					break; /***********************/
				}
/* SC_AOn:	*/		
				for (;;) {
					//fprintf(stderr, "Line ptA=%d, ptB=%d\n", ptA, ptB);
					ptB = (ptA + 1); /* ptB = NEXTPT( ptA, ctr ); */
					if ( ptB > lastPoint ) ptB = startPoint;
					Bx = x[ ptB ];	By = y[ ptB ];
					if ( onCurve[ ptB ] ) {
						if (state != kContourContinue) {
							pt[0].x = (float)Ax*(1/64.0);
							pt[0].y = (float)Ay*(1/64.0);
							if (state == kShapeStart)
								gpath->OpenShapes(pt[0]);
							else
								gpath->MoveTo(pt[0], true);
							//fprintf(stderr, "Starting contour: (%.2f,%.2f)\n",
							//		pt[0].h, pt[0].v);
							state = kContourContinue;
						}
						pt[0].x = (float)Bx*(1/64.0);
						pt[0].y = (float)By*(1/64.0);
						//fprintf(stderr, "Adding line: (%.2f,%.2f)\n",
						//		pt[0].h, pt[0].v);
						gpath->AddLineSegments(pt, 1);
						if ( ptB == startPoint ) {ctr++; goto SC_startContour;}		/********************** continue SC_startContour */
						ptA = ptB; Ax = Bx; Ay = By;
						continue;  /********************** continue SC_AOn */
					}
					if ( ptB == startPoint ) {ctr++; goto SC_startContour;}			/**********************continue SC_startContour */
					break; /***********************/
				}
			}
		}
		
		gpath->CloseShapes(true);
	}
	
	*(SPath**)glyph_path = gpath;
	return 1;

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
exit:
	*(SPath**)glyph_path = NULL;
	return 0;
}

void fc_process_edges(struct fc_renderer_context *ctxt, void *char_desc, fc_edge *edge) {
	int32		delta;
	int			errCode;

	if (ctxt->mem == NULL)
		return;
		
	/* render the glyph using font fusion */	
	T2K_RenderGlyph(ctxt->scaler, *(ushort*)char_desc, 0, 0, BLACK_AND_WHITE_BITMAP,
					ctxt->command, &errCode);
	if (errCode != 0)
		goto reset_exit;

	delta = bold_factor(ctxt, BLACK_AND_WHITE_BITMAP);
	
	if (ctxt->scaler->baseAddr != NULL) {
		edge->left = (float)(ctxt->scaler->fLeft26Dot6>>6)*(1.0/250.0);
		edge->right = (float)((ctxt->scaler->fLeft26Dot6>>6) + ctxt->scaler->width)*(1.0/250.0) + delta;
	}
	else {
		edge->left = 0.0;
		edge->right = 0.6;
	}
	return;

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
	return;
}

void fc_process_bbox(struct fc_renderer_context *ctxt, void *char_desc, fc_bbox *bbox) {
	int32		delta;
	int			errCode;

	if (ctxt->mem == NULL)
		return;
		
	/* render the glyph using font fusion */	
	T2K_RenderGlyph(ctxt->scaler, *(ushort*)char_desc, 0, 0, BLACK_AND_WHITE_BITMAP,
					ctxt->command, &errCode);
	if (errCode != 0)
		goto reset_exit;

	delta = bold_factor(ctxt, BLACK_AND_WHITE_BITMAP);
	
	if (ctxt->scaler->baseAddr != NULL) {
		bbox->top = -(float)(ctxt->scaler->fTop26Dot6>>6)*(1.0/250.0);
		bbox->left = (float)(ctxt->scaler->fLeft26Dot6>>6)*(1.0/250.0);
		bbox->right = (float)((ctxt->scaler->fLeft26Dot6>>6) + ctxt->scaler->width)*(1.0/250.0) + delta;
		bbox->bottom = (float)(-(ctxt->scaler->fTop26Dot6>>6) + ctxt->scaler->height)*(1.0/250.0);
//		printf("[%f, %f, %f, %f]\n", bbox->left, bbox->top, bbox->right, bbox->bottom);
	}
	else {
		bbox->top = -1.0;
		bbox->left = 0.0;
		bbox->right = 0.6;
		bbox->bottom = 0.1;	
	}
	return;

	/* failure exit, clean-up required for non FontFusion */
reset_exit:
	reset(ctxt);
	return;
}

int sniff_read_file(void *id, uint8 *dest_ram, uint32 offset, int32 numBytes) {
	fseek((FILE*) id, offset, SEEK_SET);
	if (fread(dest_ram, 1, numBytes, (FILE*) id) != (size_t)numBytes)
		return -1;
	return 0;
}

static char* extract_font_name(T2K* scaler, int which,
								int which2, const char* ifAllElseFails)
{
	char *utf8Name = NULL;
	int err;
	T2K_SetNameString(scaler, 0x0409, which, &err);
	if (!scaler->nameString16 || !scaler->nameString8)
		T2K_SetNameString(scaler, 0x0409, which2, &err);
	if (scaler->nameString16) {
		size_t length;
		for (length=0; scaler->nameString16[length]; length++) {
			scaler->nameString16[length] = B_BENDIAN_TO_HOST_INT16(scaler->nameString16[length]);
		}
		utf8Name = fc_convert_font_name_to_utf8((const char*)scaler->nameString16,
												length*2, B_FONT_FAMILY_LENGTH,
												0xffff, 0);
	} else {
		if (scaler->nameString8)
			ifAllElseFails = (const char*)scaler->nameString8+1;
		if (ifAllElseFails) {
			utf8Name = (char*)grMalloc(strlen(ifAllElseFails),"glue:utf8Name",MALLOC_MEDIUM);
			strcpy(utf8Name, ifAllElseFails);
		}
	}
	return utf8Name;
}

bool get_stroke_info(fc_file_entry *fe, FILE *fp)
{
	bool result = false;
	tsiMemObject *mem = 0;
	InputStream *in = 0;
	T2K *scaler = 0;
	int errCode = 0;
	T2K_TRANS_MATRIX trans;
	sfntClass *font = 0;
	long length;
	char *utf8Name;
	uint32 *bitmap;

	mem	= tsi_NewMemhandler(&errCode);
	if (mem == 0)
		goto error1;
		
	fseek(fp, 0L, SEEK_END);
	length = ftell(fp);
	in = New_NonRamInputStream(mem, fp, sniff_read_file, length, &errCode);
	if (errCode != 0)
		goto error2;

	font = New_sfntClass(mem, FONT_TYPE_TT_OR_T2K, in, NULL, &errCode);
	if (errCode != 0)
		goto error3;
	
	scaler = NewT2K(font->mem, font, &errCode);
	if (errCode != 0)
		goto error4;

	Set_PlatformID(scaler, 3);
	Set_PlatformSpecificID(scaler, 1); 

	/* Set the transformation */
	trans.t00 = ONE16Dot16;
	trans.t01 = 0;
	trans.t10 = 0;
	trans.t11 = ONE16Dot16;
	T2K_NewTransformation(scaler, false, 72, 72, &trans, true, &errCode);
	if (errCode != 0)
		goto error5;
		
	if (scaler->horizontalFontMetricsAreValid) {
		fe->ascent = float(scaler->yAscender)/(1<<16);
		fe->descent = -float(scaler->yDescender)/(1<<16);
		fe->leading = float(scaler->yLineGap)/(1<<16);
		//xprintf("Found height: asc=%.2f, desc=%.2f, lead=%.2f\n",
		//		fe->ascent, fe->descent, fe->leading);
	} else {
		fe->ascent = 0.8;
		fe->descent = 0.2;
		fe->leading = 0.1;
		//xprintf("Default height: asc=%.2f, desc=%.2f, lead=%.2f\n",
		//		fe->ascent, fe->descent, fe->leading);
	}
	
	utf8Name = extract_font_name(scaler, 1, 4, NULL);
	if (!utf8Name)
		goto error5;
	fe->family_id = fc_register_family(utf8Name, "FFS");
	grFree(utf8Name);
		
	utf8Name = extract_font_name(scaler, 2, 2, "Regular");
	if (!utf8Name)
		goto error5;
	
	// Quick and dirty check for some standard faces.
	if (strcasecmp(utf8Name, "Regular"))
		fe->faces = B_REGULAR_FACE;
	if (strcasecmp(utf8Name, "Roman"))
		fe->faces = B_REGULAR_FACE;
	else if (strcasecmp(utf8Name, "Bold"))
		fe->faces = B_BOLD_FACE;
	else if (strcasecmp(utf8Name, "Italic"))
		fe->faces = B_ITALIC_FACE;
	else
		fe->faces = 0;
		
	fe->style_id = fc_set_style(fe->family_id, utf8Name, fe - file_table,
		fe->flags, fe->faces, fe->gasp, true);
	
	grFree(utf8Name);
	
	// A hideous way to build up the has_glyph map.
	bitmap = (uint32*)grMalloc(sizeof(fc_block_bitmap)*256,"fc:temp block_bitmap",MALLOC_MEDIUM);
	if (bitmap) {
		memset(bitmap, 0, sizeof(fc_block_bitmap)*256);
		for (int32 i=0; i<0x10000; i+=32) {
			uint32 bits = 0;
			for (int32 j=0; j<32; j++) {
				int err;
				if (T2K_GetGlyphIndex(scaler, i+j, &err) != 0 && err == 0)
					bits |= 1<<j;
			}
			bitmap[i/32] = bits;
		}
		fc_set_char_map(fe, bitmap);
		grFree(bitmap);
	}
	
	result = true;

error5:
	T2K_PurgeMemory(scaler, 1, &errCode);
	DeleteT2K(scaler, &errCode);
error4:
	FF_Delete_sfntClass(font, &errCode);
error3:
	Delete_InputStream(in, &errCode);
error2:
	tsi_DeleteMemhandler(mem);
error1:
	return result;
}

static uint8* gamma_table()
{
	static uint8* table = NULL;
	static bool initialized = false;
	static uint8 tableData[127];
	
	if (initialized)
		return table;
	
	float gamma = 1.0;
	char *env = getenv("FONT_GAMMA");
	if (env) sscanf(env, "%f", &gamma);
	
	if (gamma != 1.0) {
		for (int32 i = 0; i < 127; i++) {
			tableData[i] = (uint8)(powf(i/127.0F, 1.0F/gamma) * 128.0F + 0.5F);
			//printf("Gamma table #%ld = %d\n", i, tableData[i]);
		}
		table = tableData;
	} else {
		table = NULL;
	}
	
	initialized = true;
	return table;
}

static uint8 aa_quality()
{
	static uint8 quality = GREY_SCALE_BITMAP_HIGH_QUALITY;
	static bool initialized = false;
	
	if (initialized)
		return quality;
	
	char *env = getenv("FONT_QUALITY");
	if (env) {
		if (strcasecmp(env, "low") == 0) quality = GREY_SCALE_BITMAP_LOW_QUALITY;
		else if (strcasecmp(env, "medium") == 0) quality = GREY_SCALE_BITMAP_MEDIUM_QUALITY;
		else if (strcasecmp(env, "high") == 0) quality = GREY_SCALE_BITMAP_HIGH_QUALITY;
		else if (strcasecmp(env, "higher") == 0) quality = GREY_SCALE_BITMAP_HIGHER_QUALITY;
		else if (strcasecmp(env, "best") == 0) quality = GREY_SCALE_BITMAP_EXTREME_QUALITY;
	}
	return quality;
}

static int32 bold_factor(fc_renderer_context *ctxt, uint8 grey_scale_quality)
{
#if USE_FF_BOLDING
	return 0;

#else
	if ((ctxt->algo_faces&B_BOLD_FACE) == 0)
		return 0;
		
	bool isSpace;
	
	if (ctxt->scaler) {
		isSpace = true;
		const int32 width = ctxt->scaler->width;
		const int32 height = ctxt->scaler->height;
		const int32 row = ctxt->scaler->rowBytes;
		uint8*	bits = (uint8*)ctxt->scaler->baseAddr;
		
		int32 i, j;

		if (grey_scale_quality == BLACK_AND_WHITE_BITMAP) {
			const int32 cnt = ((width+7)/8);
			for (j=0; j<height && isSpace; j++) {
				for (i=0; i<cnt; i++) {
					if (bits[i]) {
						isSpace = false;
						break;
					}
				}
				bits += row;
			}
		} else {
			for (j=0; j<height && isSpace; j++) {
				for (i=0; i<width; i++) {
					if (bits[i]) {
						isSpace = false;
						break;
					}
				}
				bits += row;
			}
		}
		
	} else {
		isSpace = false;
	}
	
	return (isSpace ? 0 : ((int32)((ctxt->size+5)/15)));
#endif
}

static void dump_metrics(T2K *t2k)
{
	int err;
	T2K_SetNameString(t2k, 0x0409, 4, &err);
	char convertedName[255];
	for (int i = 0; i < 255 && t2k->nameString16[i] != 0; i++)
		convertedName[i] = t2k->nameString16[i] & 0xff;	

	_sPrintf("Dumping font metrics for font \"%s\" ----------------------------------\n", convertedName);

	if (t2k->horizontalFontMetricsAreValid) {
		_sPrintf("xAscender = %04x.%04x yAscender = %04x.%04x\n",
			t2k->xAscender >> 16, t2k->xAscender & 0xffff,
			t2k->yAscender >> 16, t2k->yAscender & 0xffff);
		_sPrintf("xDescender = %04x.%04x yDescender = %04x.%04x\n",
			t2k->xDescender >> 16, t2k->xDescender & 0xffff,
			t2k->yDescender >> 16, t2k->yDescender & 0xffff);
		_sPrintf("xLineGap = %04x.%04x yLineGap = %04x.%04x\n",
			t2k->xLineGap >> 16, t2k->xLineGap & 0xffff,
			t2k->yLineGap >> 16, t2k->yLineGap & 0xffff);
		_sPrintf("xMaxLinearAdvanceWidth = %04x.%04x yMaxLinearAdvanceWidth = %04x.%04x\n",
			t2k->xMaxLinearAdvanceWidth >> 16, t2k->xMaxLinearAdvanceWidth & 0xffff,
			t2k->yMaxLinearAdvanceWidth >> 16, t2k->yMaxLinearAdvanceWidth & 0xffff);
	} else
		_sPrintf("Horizontal font metrics are not valid\n");

	if (t2k->verticalFontMetricsAreValid) {
		_sPrintf("vert_xAscender = %04x.%04x vert_yAscender = %04x.%04x\n",
			t2k->vert_xAscender >> 16, t2k->vert_xAscender & 0xffff,
			t2k->vert_yAscender >> 16, t2k->vert_yAscender & 0xffff);
		_sPrintf("vert_xDescender = %04x.%04x vert_yDescender = %04x.%04x\n",
			t2k->vert_xDescender >> 16, t2k->vert_xDescender & 0xffff,
			t2k->vert_yDescender >> 16, t2k->vert_yDescender & 0xffff);
		_sPrintf("vert_xLineGap = %04x.%04x vert_yLineGap = %04x.%04x\n",
			t2k->vert_xLineGap >> 16, t2k->vert_xLineGap & 0xffff,
			t2k->vert_yLineGap >> 16, t2k->vert_yLineGap & 0xffff);
		_sPrintf("vert_xMaxLinearAdvanceWidth = %04x.%04x vert_yMaxLinearAdvanceWidth = %04x.%04x\n",
			t2k->vert_xMaxLinearAdvanceWidth >> 16, t2k->vert_xMaxLinearAdvanceWidth & 0xffff,
			t2k->vert_yMaxLinearAdvanceWidth >> 16, t2k->vert_yMaxLinearAdvanceWidth & 0xffff);
	} else
		_sPrintf("Vertical font metrics are not valid\n");

	_sPrintf("-----------------------------------------------------------------------\n");
}
