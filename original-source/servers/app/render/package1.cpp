
#include "renderInternal.h"
#include "renderContext.h"
#include "renderLock.h"
#include "renderPort.h"
#include "renderCanvas.h"

#if USE_FONTS
#include "font_draw.h"
#endif

#include <math.h>
#include <Debug.h>

#define RenderMode DEFINE_OP_COPY
	#define RenderPattern 0
		#define DrawOneLineFunctionName DrawOneLineCopy1
		#define FillRectsFunctionName FillRectsCopy1
		#include "macro_1bit.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineCopyP1
		#define FillRectsFunctionName FillRectsCopyP1
		#include "macro_1bit.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_OVER
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsOver1
		#define DrawOneLineFunctionName DrawOneLineOver1
		#include "macro_1bit.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsOverP1
		#define DrawOneLineFunctionName DrawOneLineOverP1
		#include "macro_1bit.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_INVERT
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsInvert1
		#define DrawOneLineFunctionName DrawOneLineInvert1
		#include "macro_1bit.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsInvertP1
		#define DrawOneLineFunctionName DrawOneLineInvertP1
		#include "macro_1bit.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_ERASE
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsErase1
		#define DrawOneLineFunctionName DrawOneLineErase1
		#include "macro_1bit.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsEraseP1
		#define DrawOneLineFunctionName DrawOneLineEraseP1
		#include "macro_1bit.inc"
	#undef RenderPattern
#undef RenderMode

#if USE_FONTS
void fc_bw_char1(uchar *dst_base, int dst_row, rect dst,
				 uchar *src_base, rect src, int mode) {
	int        i, j, cmd, hmin, hmax, imin, imax, kmin, kmax, offset;
	uchar      mask, mask2;
	uchar      *dst_ptr;

	dst_ptr = dst_base+dst_row*dst.top;
	offset = dst.left-src.left;
	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (TRUE) {
			/* decode next run */
			do {
				cmd = *src_base++;
				/* end of run list for this line */
				if (cmd == 0xff)
					goto next_line;
				hmin += cmd;
			} while (cmd == 0xfe);
			hmax = hmin;
			do {
				cmd = *src_base++;
				hmax += cmd;
			} while (cmd == 0xfe);
			/* horizontal clipping */
			if (hmin > src.right) {
				do src_base++; while (src_base[-1] != 0xff);
				goto next_line;
			}
			if (hmin >= src.left)
				imin = hmin;
			else
				imin = src.left;
			if (hmax <= src.right)
				imax = hmax;
			else
				imax = src.right+1;
			imin += offset;
			imax += offset;
			/* draw the visible part */
			if (imin < imax) {
				kmin = imin>>3;
				if ((imax & 0xfffffff8) <= imin) {
					mask = (0x100 >> (imin & 7)) - (0x100 >> (imax & 7));
					if (mode == OP_OVER)
						dst_ptr[kmin] |= mask;
					else if (mode == OP_INVERT)
						dst_ptr[kmin] ^= mask;
					else
						dst_ptr[kmin] &= ~mask;
				}
				else {
					kmax = imax>>3;
					mask = (0x100 >> (imin & 7)) - 1;
					mask2 = 0x100 - (0x100 >> (imax&7));
					if (mode == OP_OVER) {
						dst_ptr[kmin] |= mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] = 0xff;
						dst_ptr[kmax] |= mask2;
					}
					else if (mode == OP_INVERT) {
						dst_ptr[kmin] ^= mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] ^= 0xff;
						dst_ptr[kmax] ^= mask2;
					}
					else {
						dst_ptr[kmin] &= ~mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] = 0x00;
						dst_ptr[kmax] &= ~mask2;
					}
				}
			}
			hmin = hmax;
		}
	next_line:
		dst_ptr += dst_row;
	}
}

void fc_draw_string1(	RenderContext *	context,
						RenderSubPort *	port,
						fc_string *		str,
						rect			clipbox)
{
	int          i,mode;
	int          cur_row;
	rect         cur, src;
	uchar        *src_base, *cur_base;
	fc_char      *my_char;
	fc_point     *my_offset;

	if (str->bits_per_pixel != FC_BLACK_AND_WHITE) return;
	
	/* preload port parameters */
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;

	/* mode reduction */
	if ((mode != OP_ERASE) && (mode != OP_INVERT)) mode = OP_OVER;

	/* go through all the characters */
	for (i=0; i<str->char_count; i++) {
		my_char = str->chars[i];
		my_offset = str->offsets+i;
		if ((my_offset->x < -100000) ||
			(my_offset->x > +100000) ||
			(my_offset->y < -100000) ||
			(my_offset->y > +100000))
			continue;
		/* calculate character horizontal limits */
		cur.left = my_char->bbox.left + my_offset->x;
		cur.right = my_char->bbox.right + my_offset->x;
		src.left = 0;
		src.right = cur.right-cur.left;
		if (src.right < 0)
			continue;
		/* cliping with horizontal external box */
		if (clipbox.left > cur.left) {
			if (clipbox.left > cur.right)
				continue;
			src.left += clipbox.left-cur.left;
			cur.left = clipbox.left;
		}
		if (clipbox.right < cur.right) {
			if (clipbox.right < cur.left)
				continue;
			src.right += clipbox.right-cur.right;
			cur.right = clipbox.right;
		}
		/* calculate character vertical limits */
		cur.top = my_char->bbox.top + my_offset->y;
		cur.bottom = my_char->bbox.bottom + my_offset->y;
		src.top = 0;
		src.bottom = cur.bottom-cur.top;
		/* cliping with vertical external box */
		if (clipbox.top > cur.top) {
			if (clipbox.top > cur.bottom)
				continue;
			src.top += clipbox.top-cur.top;
			cur.top = clipbox.top;
		}
		if (clipbox.bottom < cur.bottom) {
			if (clipbox.bottom < cur.top)
				continue;
			src.bottom += clipbox.bottom-cur.bottom;
			cur.bottom = clipbox.bottom;
		}
		/* complete description before drawing */
		src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
		fc_bw_char1(cur_base, cur_row, cur, src_base, src, mode);
	}		
}

void DrawString1(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	const int32 N = port->RealDrawingRegion()->CountRects();
	const rect* r = port->RealDrawingRegion()->Rects();
	for (int32 i=0;i<N;i++)
		fc_draw_string1(context, port, str, r[i]);
}
#endif


extern BlitFunction BlitCopySameFormat;

static BlitFunction copyBlitTable[] = {
	BlitCopySameFormat,
	BlitCopySameFormat,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static DrawOneLine fDrawOneLine1[NUM_OPS][2] =
{
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 },
	{ &DrawOneLineOver1, &DrawOneLineOverP1 },
	{ &DrawOneLineErase1, &DrawOneLineEraseP1 },
	{ &DrawOneLineInvert1, &DrawOneLineInvertP1 },
	{ &DrawOneLineInvert1, &DrawOneLineInvertP1 },
	{ &DrawOneLineInvert1, &DrawOneLineInvertP1 },
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 },
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 },
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 },
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 },
	{ &DrawOneLineCopy1, &DrawOneLineCopyP1 }
};

static FillRects fFillRects1[NUM_OPS][2] = 
{
	{ &FillRectsCopy1, &FillRectsCopyP1 },
	{ &FillRectsOver1, &FillRectsOverP1 },
	{ &FillRectsErase1, &FillRectsEraseP1 },
	{ &FillRectsInvert1, &FillRectsInvertP1 },
	{ &FillRectsInvert1, &FillRectsInvertP1 },
	{ &FillRectsInvert1, &FillRectsInvertP1 },
	{ &FillRectsCopy1, &FillRectsCopyP1 },
	{ &FillRectsCopy1, &FillRectsCopyP1 },
	{ &FillRectsCopy1, &FillRectsCopyP1 },
	{ &FillRectsCopy1, &FillRectsCopyP1 },
	{ &FillRectsCopy1, &FillRectsCopyP1 }
};

static void PickRenderers1(RenderContext *context, RenderSubPort *port)
{
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	RenderCache *cache = port->cache;
	cache->fFillRects = fFillRects1[op][stippling];
	cache->fDrawOneLine = fDrawOneLine1[op][stippling];
	cache->fDrawString = DrawString1;
	cache->fFillSpans = grFillSpansSoftware;
	cache->fBlitTable = copyBlitTable;
	cache->fScaledBlitTable = NULL;
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[op];
};

extern void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port);
extern void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port);

RenderingPackage renderPackage1 = {
	PickRenderers1,
	grForeColorChanged_Fallback,
	grBackColorChanged_Fallback
};
