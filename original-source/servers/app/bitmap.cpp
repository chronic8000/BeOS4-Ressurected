
//******************************************************************************
//
//	File:			bitmap.cpp
//
//	Description:	An SAtom based class to handle bitmaps
//	
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <math.h>
#include "proto.h"
#include "bitmap.h"
#include "token.h"
#include "bitmapcollection.h"
#include "DrawEngine.h"

#include <Debug.h>

extern TokenSpace *tokens;
extern bool IsValidGraphicsType(uint32 type);
extern void GraphicsTypeToInternal(uint32 type, int32 *pixelFormat, uint8 *endianess);
extern void GraphicsTypeToExternal(int32 pixelFormat, uint8 endianess, uint32 *type);

bool
bitmaps_support_space(
	color_space space,
	uint32 * out_support_flags)
{
	static color_space supported_spaces[] = {
	    B_RGB32,
	    B_RGBA32,
	    B_RGB16,
	    B_RGB15,
	    B_RGBA15,
	    B_CMAP8,
	    B_GRAY1,
	    B_RGB32_BIG,
	    B_RGBA32_BIG,
	    B_RGB16_BIG,
	    B_RGB15_BIG,
	    B_RGBA15_BIG,
	};

	for (color_space * sptr = supported_spaces; 
		sptr<&supported_spaces[sizeof(supported_spaces)/sizeof(supported_spaces[0])];
		sptr++)
		if (*sptr == space) {
			*out_support_flags = 0x3;
			return true;
		}

	*out_support_flags = 0;
	return false;
}


static struct {
	color_space space;
	size_t	chunk;
	size_t	pixels;
} s_pixel_info[] = {
	{ B_RGB32, 4, 1 },	/* B[7:0]  G[7:0]  R[7:0]  -[7:0]					*/
	{ B_RGBA32, 4, 1 },	/* B[7:0]  G[7:0]  R[7:0]  A[7:0]					*/
	{ B_RGB24, 3, 1 },	/* B[7:0]  G[7:0]  R[7:0]							*/
	{ B_RGB16, 2, 1 },	/* G[2:0],B[4:0]  R[4:0],G[5:3]						*/
	{ B_RGB15, 2, 1 },	/* G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]			*/
	{ B_RGBA15, 2, 1 },	/* G[2:0],B[4:0]  	   A[0],R[4:0],G[4:3]			*/
	{ B_CMAP8, 1, 1 },	/* D[7:0]  											*/
	{ B_GRAY8, 1, 1 },	/* Y[7:0]											*/
	{ B_GRAY1, 1, 8 },	/* Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]	*/

	/* big endian version, when the encoding is not endianess independent */
	{ B_RGB32_BIG, 4, 1 },	/* -[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	{ B_RGBA32_BIG, 4, 1 },	/* A[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	{ B_RGB24_BIG, 3, 1 },	/* R[7:0]  G[7:0]  B[7:0]							*/
	{ B_RGB16_BIG, 2, 1 },	/* R[4:0],G[5:3]  G[2:0],B[4:0]						*/
	{ B_RGB15_BIG, 2, 1 },	/* -[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
	{ B_RGBA15_BIG, 2, 1 },	/* A[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/

	/* little-endian declarations, for completness */
	{ B_RGB32_LITTLE, 4, 1 },
	{ B_RGBA32_LITTLE, 4, 1 },
	{ B_RGB24_LITTLE, 3, 1 },
	{ B_RGB16_LITTLE, 2, 1 },
	{ B_RGB15_LITTLE, 2, 1 },
	{ B_RGBA15_LITTLE, 2, 1 },

	/* non linear color space -- note that these are here for exchange purposes;	*/
	/* a BBitmap or BView may not necessarily support all these color spaces.	*/

	/* Loss/Saturation points are Y 16-235 (absoulte); Cb/Cr 16-240 (center 128) */

	{ B_YCbCr422, 4, 2 },	/* Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]  Y2[7:0]...	*/
								/* Cb2[7:0]  Y3[7:0]  Cr2[7:0]						*/
	{ B_YCbCr411, 12, 8 },	/* Cb0[7:0]  Y0[7:0]  Cr0[7:0]  Y1[7:0]  Cb4[7:0]...*/
								/* Y2[7:0]  Cr4[7:0]  Y3[7:0]  Y4[7:0]  Y5[7:0]...	*/
								/* Y6[7:0]  Y7[7:0]	 								*/
	{ B_YCbCr444, 3, 1 },	/* Y0[7:0]  Cb0[7:0]  Cr0[7:0]		*/
	{ B_YCbCr420, 3, 2 },	/* Non-interlaced only, Cb0  Y0  Y1  Cb2 Y2  Y3  on even scan lines ... */
								/* Cr0  Y0  Y1  Cr2 Y2  Y3  on odd scan lines */

	/* Extrema points are Y 0 - 207 (absolute) U -91 - 91 (offset 128) V -127 - 127 (offset 128) */
	/* note that YUV byte order is different from YCbCr */
	/* USE YCbCr, not YUV, when that's what you mean (which is most video!) */
	{ B_YUV422, 4, 2 }, /* U0[7:0]  Y0[7:0]   V0[7:0]  Y1[7:0] ... */
								/* U2[7:0]  Y2[7:0]   V2[7:0]  Y3[7:0]  */
	{ B_YUV411, 12, 8 }, /* U0[7:0]  Y0[7:0]  Y1[7:0]  V0[7:0]  Y2[7:0]  Y3[7:0]  */
								/* U4[7:0]  Y4[7:0]  Y5[7:0]  V4[7:0]  Y6[7:0]  Y7[7:0]  */
	{ B_YUV444, 3, 1 },	/* U0[7:0]  Y0[7:0]  V0[7:0]  U1[7:0]  Y1[7:0]  V1[7:0] */
	{ B_YUV420, 6, 4 },	/* Non-interlaced only, U0  Y0  Y1  U2 Y2  Y3  on even scan lines ... */
								/* V0  Y0  Y1  V2 Y2  Y3  on odd scan lines */
	{ B_YUV9, 5, 4 },	/* planar?	410?								*/
	{ B_YUV12, 6, 4 },	/* planar?	420?								*/

	{ B_UVL24, 3, 1 },	/* U0[7:0] V0[7:0] L0[7:0] ... */
	{ B_UVL32, 4, 1 },	/* U0[7:0] V0[7:0] L0[7:0] X0[7:0]... */
	{ B_UVLA32, 4, 1 },	/* U0[7:0] V0[7:0] L0[7:0] A0[7:0]... */

	{ B_LAB24, 3, 1 },	/* L0[7:0] a0[7:0] b0[7:0] ...  (a is not alpha!) */
	{ B_LAB32, 4, 1 },	/* L0[7:0] a0[7:0] b0[7:0] X0[7:0] ... (b is not alpha!) */
	{ B_LABA32, 4, 1 },	/* L0[7:0] a0[7:0] b0[7:0] A0[7:0] ... (A is alpha) */

	/* red is at hue = 0 */

	{ B_HSI24, 3, 1 },	/* H[7:0]  S[7:0]  I[7:0]							*/
	{ B_HSI32, 4, 1 },	/* H[7:0]  S[7:0]  I[7:0]  X[7:0]					*/
	{ B_HSIA32, 4, 1 },	/* H[7:0]  S[7:0]  I[7:0]  A[7:0]					*/

	{ B_HSV24, 3, 1 },	/* H[7:0]  S[7:0]  V[7:0]							*/
	{ B_HSV32, 4, 1 },	/* H[7:0]  S[7:0]  V[7:0]  X[7:0]					*/
	{ B_HSVA32, 4, 1 },	/* H[7:0]  S[7:0]  V[7:0]  A[7:0]					*/

	{ B_HLS24, 3, 1 },	/* H[7:0]  L[7:0]  S[7:0]							*/
	{ B_HLS32, 4, 1 },	/* H[7:0]  L[7:0]  S[7:0]  X[7:0]					*/
	{ B_HLSA32, 4, 1 },	/* H[7:0]  L[7:0]  S[7:0]  A[7:0]					*/

	{ B_CMY24, 3, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  			No gray removal done		*/
	{ B_CMY32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  X[7:0]	No gray removal done		*/
	{ B_CMYA32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  A[7:0]	No gray removal done		*/
	{ B_CMYK32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  K[7:0]					*/
	{ B_NO_COLOR_SPACE, 0, 0 }
};

status_t
get_pixel_size_for(
	color_space space,
	size_t * pixel_chunk,
	size_t * row_alignment,
	size_t * pixels_per_chunk)
{
	for (int ix=0; s_pixel_info[ix].space; ix++) {
		if (s_pixel_info[ix].space == space) {
			if (pixel_chunk) *pixel_chunk = s_pixel_info[ix].chunk;
			if (row_alignment) {
				int align = 4;
				while (align < s_pixel_info[ix].chunk) {
					align <<= 1;
				}
				*row_alignment = align;
			}
			if (pixels_per_chunk) *pixels_per_chunk = s_pixel_info[ix].pixels;
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

RenderCanvas * LockOverlayBitmap(SBitmap *b, region *bound, uint32 *token, uint32 *flags)
{
	b->overlay->LockBits();
	b->canvas.pixels.pixelData = (uint8*)b->overlay->Bits();
	*flags = 0;
	*token = 0;
	return &b->canvas;
};

void UnlockOverlayBitmap(SBitmap *b, region *bound, sync_token *token)
{
	b->overlay->UnlockBits();
};

RenderCanvas * LockBitmap(SBitmap *b, region *bound, uint32 *token, uint32 *flags)
{
	b->serverHeap->Lock();
	b->canvas.pixels.pixelData = (uint8*)b->serverHeap->Ptr(b->serverHeapOffset);
	int32 screenType = 	b->canvas.pixels.pixelFormat +
						(((int32)b->canvas.pixels.endianess)<<16);
	if (*token == screenType) {
		*flags = 0;
		return &b->canvas;
	};

	*flags = RCANVAS_FORMAT_CHANGED;
	*token = screenType;
	return &b->canvas;
};

void UnlockBitmap(SBitmap *b, region *bound, sync_token *token)
{
	b->serverHeap->Unlock();
};

RenderCanvas * LockBitmapFake(SBitmap *b, region *bound, uint32 *token, uint32 *flags)
{
	RenderCanvas *canvas = b->Canvas();
	int32 screenType =      canvas->pixels.pixelFormat +
							(((int32)canvas->pixels.endianess)<<16);
	if (*token == screenType) {
		*flags = 0;
		return canvas;
	};
	
	*flags = RCANVAS_FORMAT_CHANGED;
	*token = screenType;
	return canvas;
};

void UnlockBitmapFake(SBitmap *b, region *bound)
{
};

SBitmap::SBitmap(
	int32 x, int32 y,
	int32 colorSpace,
	uint8 endianess,
	bool clientBitmap,
	uint32 flags,
	int32 bytesPerRow,
	Heap *theServerHeap,
	SApp *team)
{
	InitData(x,y,0,colorSpace,endianess,clientBitmap,flags,bytesPerRow,theServerHeap,team);
};

SBitmap::SBitmap(
	int32 x, int32 y,
	uint32 pixelFormat,
	bool clientBitmap,
	uint32 flags,
	int32 bytesPerRow,
	Heap *theServerHeap,
	SApp *team)
{
	InitData(x,y,pixelFormat,-1,0,clientBitmap,flags,bytesPerRow,theServerHeap,team);
};

extern RenderingPackage *renderingPackages[2][5];

void SBitmap::InitData(
	int32 x, int32 y,
	uint32 pixelFormat,
	int32 colorSpace,
	uint8 endianess,
	bool clientBitmap,
	uint32 flags,
	int32 bytesPerRow,
	Heap *theServerHeap,
	SApp *team)
{
	int32 bitmapSize, pixelBits,tmpBPR;
	uint32 clearPixel;
	size_t row_alignment,chunk,pix_per_chunk;
	server_token = NO_TOKEN;

	if (colorSpace == -1) GraphicsTypeToInternal(pixelFormat,&colorSpace,&endianess);
	if (!pixelFormat) GraphicsTypeToExternal(colorSpace,endianess,&pixelFormat);

	canvas.pixels.pixelData = NULL;
	canvas.pixels.colorMap = (ColorMap*)system_cs->color_list;
	canvas.pixels.pixelFormat = colorSpace;
	canvas.pixels.endianess = endianess;
#if ROTATE_DISPLAY
	if (flags & B_BITMAP_IS_ROTATED) {
		canvas.pixels.pixelIsRotated = true;
		tmpBPR = y;
		y = x;
		x = tmpBPR;
	} else {
		canvas.pixels.pixelIsRotated = false;
	}
#endif
	canvas.pixels.w = x;
	canvas.pixels.h = y;
	canvas.pixels.isCompressed = false;
	canvas.pixels.isLbxCompressed = false;
	*((uint32*)&canvas.overlayHack) = TRANSPARENT_RGBA;
	canvas.fLockAccelPackage = NULL;
	canvas.fUnlockAccelPackage = NULL;
	canvas.accelPackage = &noAccel;
	properties = 0;
	areaID = 0;
	serverHeap = NULL;
	serverHeapOffset = NONZERO_NULL;
	overlay = NULL;

	if (flags & BITMAP_IS_LBX_COMPRESSED)
	{
		DebugPrintf(("creating LBX compressed bitmap %08x\n", this));
		fLbxPixelData.draw_engine = NULL;
		fLbxPixelData.index = -1;
		fLbxPixelData.lbx_token = 0;
		canvas.pixels.pixelData = (uint8 *)&fLbxPixelData;
		canvas.pixels.isCompressed = 1;
		canvas.pixels.isLbxCompressed = 1;
		canvas.pixels.bytesPerRow = 0;
		serverHeapOffset = 0;
		return;
	}

	get_pixel_size_for((color_space)pixelFormat,&chunk,&row_alignment,&pix_per_chunk);

	switch (colorSpace) {
		case PIX_1BIT : clearPixel = 0;				break;
		case PIX_8BIT : clearPixel = 0x3F3F3F3F;	break;
		case PIX_15BIT: 
		case PIX_16BIT:
		case PIX_32BIT:
		default:		clearPixel = 0xFFFFFFFF;	break;
	};

	tmpBPR = ((x+pix_per_chunk-1)/pix_per_chunk) * chunk;
	canvas.pixels.bytesPerRow = ((tmpBPR+row_alignment-1) / row_alignment) * row_alignment;

	if (bytesPerRow != -1) {
		if (bytesPerRow < tmpBPR) bytesPerRow = tmpBPR;
		canvas.pixels.bytesPerRow = bytesPerRow;
	};

	bitmapSize = y*canvas.pixels.bytesPerRow;
	
	if ((float)y * (float)canvas.pixels.bytesPerRow > 2e9) {
		properties |= BITMAP_IS_INVALID;
	} else if (clientBitmap) {
		properties |= BITMAP_IS_SHARED;
		if (flags & BITMAP_WILL_OVERLAY) {
			status_t err = BGraphicDevice::Device(0)->CreateOverlay(team,(color_space)pixelFormat,x,y,
				(flags & BITMAP_RESERVE_OVERLAY_CHANNEL)?OVERLAY_RESERVE_CHANNEL:0,
				&overlay);
			DebugPrintf(("SBitmap: err=%d,  pixelFormat=%08x\n",err,pixelFormat));
			if (err != B_OK) properties |= BITMAP_IS_INVALID;
			else {
				properties |= BITMAP_IS_OVERLAY;
				canvas.pixels.bytesPerRow = overlay->BytesPerRow();
				canvas.pixels.pixelData = (uint8*)overlay->Bits();
			};
		} else if (flags & BITMAP_IS_AREA) {
			areaID = Area::CreateArea("bitmap", (void **)&canvas.pixels.pixelData,
						B_ANY_ADDRESS, (bitmapSize+4095) & 0xFFFFF000,
						((flags & BITMAP_IS_CONTIGUOUS)?B_CONTIGUOUS:
						((flags & BITMAP_IS_LOCKED)?B_FULL_LOCK:B_NO_LOCK)),
						B_READ_AREA|B_WRITE_AREA);
			if (areaID <= 0)
				properties |= BITMAP_IS_INVALID;
		} else {
			properties |= BITMAP_IS_INVALID;
			if (theServerHeap) {
				serverHeap = theServerHeap;
				serverHeapOffset = serverHeap->Alloc(bitmapSize);
				serverHeap->ServerLock();
				canvas.pixels.pixelData = NULL;
				if (serverHeapOffset != NONZERO_NULL)
					properties &= ~BITMAP_IS_INVALID;
				else {
					DebugPrintf(("SBitmap::SBitmap(): Server heap alloc failed!\n"));
				};
			};
		};
	} else {
		canvas.pixels.pixelData = (uint8*)grMalloc(bitmapSize,"local offscreen",MALLOC_CANNOT_FAIL);
		if (!canvas.pixels.pixelData) properties |= BITMAP_IS_INVALID;
	};
	
	if (IsValid() && (flags & BITMAP_CLEAR_TO_WHITE)) {
		LockPixels();
			uint32 *p = (uint32*)canvas.pixels.pixelData;
			int32 count = canvas.pixels.bytesPerRow * canvas.pixels.h;
			while (count>=4) { *p++ = clearPixel; count-=4; };
			uint8 *bp = (uint8*)p;
			while (count>0) { *bp++ = clearPixel; count--; };
		UnlockPixels();
	};

	canvas.renderingPackage = renderingPackages[endianess][colorSpace];
};

void SBitmap::SetPort(RenderSubPort *port)
{
	if (overlay)
		grSetPortCanvas(port,this,(LockRenderCanvas)LockOverlayBitmap,
			(UnlockRenderCanvas)UnlockOverlayBitmap);
	else if (serverHeap)
		grSetPortCanvas(port,this,(LockRenderCanvas)LockBitmap,
			(UnlockRenderCanvas)UnlockBitmap);
	else
		grSetPortCanvas(port,this,(LockRenderCanvas)LockBitmapFake,
			(UnlockRenderCanvas)UnlockBitmapFake);
};

void SBitmap::PreloadPixels(rect *r)
{
	volatile uchar	*from;
	volatile uchar	*to;
	volatile uchar	bid;
	Pixels *		pixels = &canvas.pixels;
	int32			top,bottom;

	LockPixels();

	if (r) {
		top = r->top;
		bottom = r->bottom;
		if (top < 0) top = 0;
		if (bottom >= pixels->h) bottom = pixels->h-1;
	} else {
		top = 0;
		bottom = pixels->h-1;
	};
	
	from = pixels->pixelData + (pixels->bytesPerRow * top);
	to 	 = pixels->pixelData + (pixels->bytesPerRow * bottom);
	
	while(from < to) {
		bid = *from;
		from += 4096;
	}

	UnlockPixels();
}

// -------------------------------------------------------
// #pragma mark -

int32 SBitmap::SelectLbxBitmap(int32 lbx_token, int32 index)
{
	// Make sure this bitmap is not already linked to a MBPC
	if (fLbxPixelData.draw_engine != NULL)
		return B_BAD_VALUE;
		
	// Just grab an Atom to the corresponding SBitmapCollection object
	SBitmapCollection *lbx;
	int32 result = tokens->grab_atom(lbx_token, (SAtom **)&lbx);
	if (result != B_OK)
		return result;
		
	// Our pixeldata are just the lbx object and the bitmap index
	fLbxPixelData.draw_engine = new LBX_DrawEngine(lbx->Container(), index);
	fLbxPixelData.index = index;
	fLbxPixelData.lbx_token = lbx_token;

	lbx->ClientLock();
	lbx->ServerUnlock();
	return B_OK;
}

uint32 SBitmap::ServerHeapOffset()
{
	return serverHeapOffset;
};

area_id SBitmap::AreaID()
{
	return areaID;
};

bool SBitmap::IsValid()
{
	return !(properties & BITMAP_IS_INVALID);
};

bool SBitmap::IsShared()
{
	return properties & BITMAP_IS_SHARED;
};

bool SBitmap::IsOverlay()
{
	return properties & BITMAP_IS_OVERLAY;
};

Overlay * SBitmap::OverlayFor()
{
	return overlay;
};

RenderCanvas * SBitmap::Canvas()
{
	return &canvas;
};

SBitmap::~SBitmap()
{
	if (fLbxPixelData.draw_engine)
	{ // unlink from the lbx object
		delete fLbxPixelData.draw_engine;

		SBitmapCollection *lbx;
		int32 result = tokens->grab_atom(fLbxPixelData.lbx_token, (SAtom **)&lbx);
		if (result >= 0) {
			tokens->delete_atom(fLbxPixelData.lbx_token, lbx);
		}
		canvas.pixels.isCompressed = 0;
		canvas.pixels.pixelData = NULL;
	}

	if (server_token) tokens->remove_token(server_token);
	if (overlay) delete overlay;
	else if (areaID != 0) {
		Area::DeleteArea(areaID);
	} else if (serverHeap) {
		serverHeap->Free(serverHeapOffset);
		serverHeap->ServerUnlock();
	} else if (canvas.pixels.pixelData) {
		grFree(canvas.pixels.pixelData);
	};
};

int32 SBitmap::Token()
{
	return server_token;
};

void SBitmap::SetToken(int32 token)
{
	server_token = token;
};

/*-------------------------------------------------------------*/

#if AS_DEBUG
void SBitmap::DumpDebugInfo(DebugInfo *d)
{
	DebugPrintf(("SBitmap(0x%08x)++++++++++++++++++++\n",this));
	DebugPushIndent(2);
	SAtom::DumpDebugInfo(d);
	DebugPopIndent(2);
	DebugPrintf(("  serverHeap       = 0x%08x\n",serverHeap));
	DebugPrintf(("  serverHeapOffset = 0x%08x\n",serverHeapOffset));
	DebugPrintf(("  properties       = 0x%08x\n",properties));
	if (d->debugFlags & DFLAG_BITMAP_TO_CANVAS) {
		DebugPrintf(("  canvas -->\n"));
		DebugPushIndent(4);
		grCanvasDebugOutput(&canvas);
		DebugPopIndent(4);
	};
	DebugPrintf(("SBitmap(0x%08x)--------------------\n",this));
};
#endif
