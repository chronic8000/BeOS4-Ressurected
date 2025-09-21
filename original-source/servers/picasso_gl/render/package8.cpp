
#include <support2/Debug.h>
#include <math.h>

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderLock.h"
#include "accelPackage.h"
#include "renderdefs.h"

extern void fc_bw_char8(uchar *dst_base, int dst_row, rect dst, uchar *src_base, rect src, uint colors);
extern void fc_bw_char8other(uchar *dst_base, int dst_row, rect dst, uchar *src_base, rect src, int mode, ColorMap *cs, uint colors, int energy);
extern void fc_gray_char8(uchar *dst_base, int dst_row, rect dst, uchar *src_base, int src_row, rect src, uint32 *colors);
extern void fc_gray_char8x(uchar *dst_base, int dst_row, rect dst, uchar *src2_base, int src2_row, rect src2, uchar *src_base, int src_row, rect src, uint32 *colors);
extern void fc_gray_char8other(uchar *dst_base, int dst_row, rect dst, uchar *src_base, int src_row, rect src, int16 mode, ColorMap *cs, uint32 *colors);
extern void fc_gray_char8over(uchar *dst_base, int dst_row, rect dst, uchar *src_base, int src_row, rect src, int16 mode, ColorMap *cs, uint32 *colors);

typedef void (*DrawChar)(
	uchar *dst_base, int dst_row, rect dst,
	uchar *src_base, int src_row, rect src,
	int16 mode, ColorMap *cs, uint32 *colors);

typedef void (*AlphaDrawChar)(
	uint8 *dst_base, int32 dst_row, rect dst,
	uint8 *src_base, int32 src_row, rect src,
	uint32 *colors, uint32 *alphas, uint8 *inverted, uint32 *index2rgb);

#define rol(b, n) (((uint8)(b) << (n)) | ((uint8)(b) >> (8-(n))))
#define	write_long(ptr, what) *((uint32 *)ptr) = what;
#define	as_long(v) *((uint32 *)&v)

#define DestBits 8

#define RenderMode DEFINE_OP_COPY
	#define RenderPattern 0
		#define DrawLinesFunctionName DrawLinesCopy8
		#define DrawOneLineFunctionName DrawOneLineCopy8
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineCopyP8
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_OVER
	#define RenderPattern 0
		#define DrawOneLineFunctionName DrawOneLineOverErase8
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsOverEraseP8
		#define DrawOneLineFunctionName DrawOneLineOverEraseP8
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_INVERT
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsInvert8
		#define DrawOneLineFunctionName DrawOneLineInvert8
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsInvertP8
		#define DrawOneLineFunctionName DrawOneLineInvertP8
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_BLEND
	#define SourceAlpha SOURCE_ALPHA_PIXEL
	#define AlphaFunction ALPHA_FUNCTION_OVERLAY
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsAlpha8
		#define DrawOneLineFunctionName DrawOneLineAlpha8
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsAlphaP8
		#define DrawOneLineFunctionName DrawOneLineAlphaP8
		#include "fillspan.inc"
	#undef RenderPattern
	#undef SourceAlpha
	#undef AlphaFunction
#undef RenderMode
#define RenderMode DEFINE_OP_OTHER
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsOther8
		#define DrawOneLineFunctionName DrawOneLineOther8
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define FillRectsFunctionName FillRectsOtherP8
		#define DrawOneLineFunctionName DrawOneLineOtherP8
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode


#define BlitScaling 											0
#define RenderMode												DEFINE_OP_COPY

	/*	Generate all cases which copy to 8-bit from any other format.  The format
		of 8-bit pixels is (obviously) endianess independent. */

	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitCopy1To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitCopyLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitCopyBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										16
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitCopyLittle16To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitCopyBig16To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitCopyLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitCopyBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define RenderMode												DEFINE_OP_OVER
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitOver1To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitOver8To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitOverLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitOverBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitOverLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitOverBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define RenderMode												DEFINE_OP_BLEND
	#define SourceAlpha											SOURCE_ALPHA_PIXEL
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitAlphaLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitAlphaBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitAlphaLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitAlphaBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
	#undef SourceAlpha
	#undef AlphaFunction
#undef RenderMode
#undef BlitScaling

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define BlitScaling												1
#define RenderMode												DEFINE_OP_COPY
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
/*
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleCopy1To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
*/
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleCopy8To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleCopyLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleCopyBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										16
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleCopyLittle16To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleCopyBig16To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleCopyLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleCopyBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define RenderMode												DEFINE_OP_OVER
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
/*
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleOver1To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
*/
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleOver8To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleOverLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleOverBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										16
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleOverLittle16To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleOverBig16To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleOverLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleOverBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define RenderMode												DEFINE_OP_FUNCTION
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
/*
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleFunction1To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
*/
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleFunction8To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleFunctionLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleFunctionBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										16
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleFunctionLittle16To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleFunctionBig16To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleFunctionLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleFunctionBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define RenderMode												DEFINE_OP_BLEND
	#define DestBits 											8
	#define DestEndianess										LITTLE_ENDIAN
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
	#define SourceAlpha											SOURCE_ALPHA_PIXEL
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleAlphaLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleAlphaBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleAlphaLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleAlphaBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef SourceAlpha
	#define SourceAlpha											SOURCE_ALPHA_CONSTANT
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleConstAlpha8To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaLittle15To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaBig15To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										16
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaLittle16To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaBig16To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaLittle32To8
				#include "blitting.inc"
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#define BlitFuncName							BlitScaleConstAlphaBig32To8
				#include "blitting.inc"
			#undef SourceEndianess
		#undef SourceBits
	#undef SourceAlpha
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

static void FillRectsCopy8(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	RenderCache *cache = port->cache;
	int32		rowBytes=port->canvas->pixels.bytesPerRow,rc;
	uint8		*pixel,*base=((uint8*)port->canvas->pixels.pixelData);
	int32		i,width,height,pixelInc;
	uint32		color;
	int32		clips;
	rect		thisRect;
	const rect	*curSrcRect=fillRects,*curClipRect;
	int16		drawOp = context->drawOp;
	const rect	*clipRects = port->RealDrawingRegion()->Rects();
	const int32	clipCount = port->RealDrawingRegion()->CountRects();
	point		offset = port->origin;

	grAssertLocked(context, port);
	
	color = (drawOp == OP_COPY) ? *(&cache->foreColorCanvasFormat + context->solidColor)
								: ((context->drawOp==OP_OVER) 	? cache->foreColorCanvasFormat
																: cache->backColorCanvasFormat);
	
	while (rectCount) {
		curClipRect = clipRects;
		clips = clipCount;
		while (clips) {
			thisRect.left = curSrcRect->left + offset.x;
			thisRect.right = curSrcRect->right + offset.x;
			thisRect.top = curSrcRect->top + offset.y;
			thisRect.bottom = curSrcRect->bottom + offset.y;
			if (curClipRect->top > thisRect.top) thisRect.top = curClipRect->top;
			if (curClipRect->bottom < thisRect.bottom) thisRect.bottom = curClipRect->bottom;
			if (curClipRect->left > thisRect.left) thisRect.left = curClipRect->left;
			if (curClipRect->right < thisRect.right) thisRect.right = curClipRect->right;
			if ((thisRect.top <= thisRect.bottom) && (thisRect.left <= thisRect.right)) {
				height = (thisRect.bottom-thisRect.top+1);
				width = (thisRect.right-thisRect.left+1);
				pixelInc = rowBytes - width;
				pixel = base+(thisRect.top*rowBytes)+thisRect.left;
				while (height) {
					i = width;
					while (i && (((uint32)pixel & 0x3) != 0)) {
						i--; *pixel++ = color;
					}
					while (i >= 8) {
						write_long(pixel, color); pixel += 4;
						write_long(pixel, color); pixel += 4;
						i -= 8;
					};
					if (i >= 4) {
						write_long(pixel, color); pixel += 4;
						i -= 4;
					};
					while (i > 0) {
						i--; *pixel++ = color;
					};
					height--;
					pixel += pixelInc;
				};
			};
			clips--;
			curClipRect++;
		};
		rectCount--;
		curSrcRect++;
	};
};

static void FillRectsCopyP8(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	RenderCache *cache = port->cache;
	int32	rowBytes=port->canvas->pixels.bytesPerRow,rc;
	uint8	*pixel,*base=((uint8*)port->canvas->pixels.pixelData),*rowBase;
	int32	i,width,height;
	uint32	patBits,pb;
	int32	phaseX=port->phase_x,phaseY=port->phase_y;
	int32	y,thisPhase,tmp;
	pattern pat = context->stipplePattern;
	uint16	indices;
	uint8	exp_pat[8];
	const rect	*clipRects = port->RealDrawingRegion()->Rects();
	const int32	clipCount = port->RealDrawingRegion()->CountRects();
	point	offset = port->origin;

	int32	clips;
	rect	thisRect;
	const rect	*curSrcRect=fillRects,*curClipRect;

	grAssertLocked(context, port);
	
	indices = cache->foreColorCanvasFormat & 0xFF;
	indices = (indices<<8) | (cache->backColorCanvasFormat & 0xFF);

	while (rectCount) {
		curClipRect = clipRects;
		clips = clipCount;
		while (clips) {
			thisRect.left = curSrcRect->left + offset.x;
			thisRect.right = curSrcRect->right + offset.x;
			thisRect.top = curSrcRect->top + offset.y;
			thisRect.bottom = curSrcRect->bottom + offset.y;
			if (curClipRect->top > thisRect.top) thisRect.top = curClipRect->top;
			if (curClipRect->bottom < thisRect.bottom) thisRect.bottom = curClipRect->bottom;
			if (curClipRect->left > thisRect.left) thisRect.left = curClipRect->left;
			if (curClipRect->right < thisRect.right) thisRect.right = curClipRect->right;
			if ((thisRect.top <= thisRect.bottom) && (thisRect.left <= thisRect.right)) {
				height = (thisRect.bottom-thisRect.top+1);
				width = (thisRect.right-thisRect.left+1);
				rowBase = base + (thisRect.top * rowBytes) + thisRect.left;
				y = phaseY+thisRect.top;
				thisPhase = (phaseX+thisRect.left) & 7;
				for (;height;height--) {
					i = width;
					pixel = rowBase;
					patBits = pat.data[y&7];
					patBits |= patBits<<8;
					patBits |= patBits<<16;
					patBits <<= (thisPhase-((uint32)pixel)) & 7;
					for (tmp=0; tmp<8; tmp++) {
						exp_pat[tmp] = (indices>>((patBits&0x80000000)>>28));
						patBits <<= 1;
					};

					while (i && ((int32)pixel & 0x03)) {
						*pixel = exp_pat[((uint32)pixel)&7];
						pixel++; i--;
					}

					if ((i >= 4) && ((int32)pixel & 0x07)) {
						*((uint32*)pixel) = *((uint32*)(exp_pat + (((uint32)pixel)&7)));
						pixel+=4; i-=4;
					}

					while (i >= 8) {
						#if (HostEndianess == BIG_ENDIAN)
						*((double*)pixel) = *((double*)exp_pat);
						#else
						*((uint32*)pixel) = *((uint32*)exp_pat);
						*((uint32*)(pixel+4)) = *((uint32*)(exp_pat+4));
						#endif
						pixel+=8; i -= 8;
					};
					
					if (i >= 4) {
						*((uint32*)pixel) = *((uint32*)(exp_pat + (((uint32)pixel)&7)));
						pixel+=4; i-=4;
					}

					while (i) {
						*pixel = exp_pat[((uint32)pixel)&7];
						pixel++; i--;
					}

					rowBase += rowBytes;
					y++;
				};
			};
			clips--;
			curClipRect++;
		};
		rectCount--;
		curSrcRect++;
	};
};

/* main entry to draw black&white string into an 8 bits port */
static void DrawStringNonAA(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int			i, r, g, b, index;
	int			cur_row, energy0;
	int			mode_direct;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	rc=clip->CountRects();
	int32		ri;
	rect		cur, src,clipbox;
	uint		colors;
	uchar *		inverted;
	uchar *		src_base, *cur_base;
	fc_char *	my_char;
	fc_point *	my_offset;
	int16		mode = context->drawOp;
	rgb_color	foreColor,backColor;

	grAssertLocked(context, port);
	
	/* preload port parameters */
	inverted = canvas->pixels.colorMap->inverted;
	cur_base = (uchar*)canvas->pixels.pixelData;
	cur_row = canvas->pixels.bytesPerRow;
	foreColor = context->foreColor;
	backColor = context->backColor;

	colors = cache->fontColors[0];
	mode_direct = cache->fontColors[1];
	energy0 = cache->fontColors[2];

	if ((ri=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((ri>0) && (clipRects[ri].top > str->bbox.top)) ri--;
	while ((ri>0) && (clipRects[ri].top == clipRects[ri-1].top)) ri--;

	/* go through all the characters */
	for (;ri<rc;ri++) {
		clipbox = clipRects[ri];
		if (clipbox.top > str->bbox.bottom) return;
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
			if (mode_direct)
				fc_bw_char8(cur_base, cur_row, cur, src_base, src, colors);
			else
				fc_bw_char8other(cur_base, cur_row, cur, src_base, src,
								 mode, canvas->pixels.colorMap, colors, energy0);
		}
	}
};

static void DrawStringNonAAPreprocess(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	int			i, r, g, b, index;
	int			cur_row, energy0;
	int			mode_direct;
	rect		cur, src,clipbox;
	uint		colors;
	uchar *		inverted;
	uchar *		src_base, *cur_base;
	fc_char *	my_char;
	fc_point *	my_offset;
	int16		mode = context->drawOp;
	rgb_color	foreColor,backColor;

	grAssertLocked(context, port);
	
	/* preload port parameters */
	inverted = port->canvas->pixels.colorMap->inverted;
	cur_base = (uchar*)port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	foreColor = context->foreColor;
	backColor = context->backColor;

	/* copy mode color init */
	switch (mode) {
	case OP_ERASE:
		index = ((backColor.red		<< 7) & (0xf8 << 7))	|
				((backColor.green	<< 2) & (0xf8 << 2))	|
				((backColor.blue	>> 3) & (0xf8 >> 3))	;
		goto record_mono_color;
	case OP_COPY:
	case OP_OVER:
	case OP_INVERT:
	case OP_SELECT:
		index = ((foreColor.red		<< 7) & (0xf8 << 7))	|
				((foreColor.green	<< 2) & (0xf8 << 2))	|
				((foreColor.blue	>> 3) & (0xf8 >> 3))	;
	record_mono_color:
		colors = inverted[index];
		colors |= (colors<<8);
		colors |= (colors<<16);
		mode_direct = true;
		break;
	default:
		r = foreColor.red;
		g = foreColor.green;
		b = foreColor.blue;
		energy0 = r+g+b;
		colors = ((r<<22)&0x3f800000) | ((g<<12)&0x0fe000) | (b<<2);
		mode_direct = true;
		break;
	}

	cache->fontColors[0] = colors;
	cache->fontColors[1] = mode_direct;
	cache->fontColors[2] = energy0;

	cache->fDrawString = DrawStringNonAA;
	DrawStringNonAA(context,port,str);
};

static void DrawStringOther(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int			i, r0, g0, b0, dr, dg, db, dh, dv, index, energy0;
	int			src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	rc=clip->CountRects();
	int32		r;
	rect		prev, src_prev, cur, src, inter, clipbox;
	rect		tmp, src_tmp, src_prev_tmp;
	uchar *		src_base, *cur_base, *src_prev_base;
	uint32 *	colors = cache->fontColors;
	uchar *		inverted;
	fc_char *	my_char;
	fc_point *	my_offset;
	int16		mode = context->drawOp;
	rgb_color	foreColor,backColor;
	DrawChar	drawChar;

	grAssertLocked(context, port);
	
	inverted = canvas->pixels.colorMap->inverted;
	cur_base = (uchar*)canvas->pixels.pixelData;
	cur_row = canvas->pixels.bytesPerRow;
	foreColor = context->foreColor;
	backColor = context->backColor;
	if (mode == OP_OVER)
		drawChar = fc_gray_char8over;
	else
		drawChar = fc_gray_char8other;

	if ((r=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((r>0) && (clipRects[r].top > str->bbox.top)) r--;
	while ((r>0) && (clipRects[r].top == clipRects[r-1].top)) r--;

	/* go through all the characters */
	for (;r<rc;r++) {
		clipbox = clipRects[r];
		if (clipbox.top > str->bbox.bottom) return;
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(cur_base, cur_row, cur,
			   src_base, src_row, src,
			   mode, canvas->pixels.colorMap, colors);
		}
	}
}

static void DrawStringCopy(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int			i, dh, dv, index;
	int			src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	rc=clip->CountRects();
	int32		r;
	rect		prev, src_prev, cur, src, inter, clipbox;
	rect		tmp, src_tmp, src_prev_tmp;
	uint32 *	colors = cache->fontColors;
	uint8 *		inverted;
	uchar *		src_base, *cur_base, *src_prev_base;
	fc_char *	my_char;
	fc_point *	my_offset;

	grAssertLocked(context, port);
	
	/* preload canvas parameters */
	inverted = canvas->pixels.colorMap->inverted;
	cur_base = (uchar*)canvas->pixels.pixelData;
	cur_row = canvas->pixels.bytesPerRow;

	/* init empty previous character box */
	prev.top = -1048576;
	prev.left = -1048576;
	prev.right = -1048576;
	prev.bottom = -1048576;

	if ((r=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((r>0) && (clipRects[r].top > str->bbox.top)) r--;
	while ((r>0) && (clipRects[r].top == clipRects[r-1].top)) r--;

	for (;r<rc;r++) {
		clipbox = clipRects[r];
		if (clipbox.top > str->bbox.bottom) return;
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		
			/* test character not-overlaping */
			if ((prev.bottom < cur.top) ||
				(prev.right < cur.left) ||
				(prev.left > cur.right) ||
				(prev.top > cur.bottom))
				fc_gray_char8(cur_base, cur_row, cur,
							  src_base, src_row, src, colors);
			/* characters overlaping */
			else {
				/* calculate intersection of the 2 rect */
				if (prev.left > cur.left)
					inter.left = prev.left;
				else
					inter.left = cur.left;
				if (prev.right < cur.right)
					inter.right = prev.right;
				else
					inter.right = cur.right;
				if (prev.top > cur.top)
					inter.top = prev.top;
				else
					inter.top = cur.top;
				if (prev.bottom < cur.bottom)
					inter.bottom = prev.bottom;
				else
					inter.bottom = cur.bottom;
				/* left not overlaping band */	
				dh = inter.left-cur.left-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.left;
					tmp.right = cur.left+dh;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.left;
					src_tmp.right = src.left+dh;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					fc_gray_char8(cur_base, cur_row, tmp,
								  src_base, src_row, src_tmp, colors);
				}
				/* right not overlaping band */	
				dh = cur.right-inter.right-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.right-dh;
					tmp.right = cur.right;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.right-dh;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					fc_gray_char8(cur_base, cur_row, tmp,
								  src_base, src_row, src_tmp, colors);
				}
				/* top not overlaping band */	
				dv = inter.top-cur.top-1;
				if (dv >= 0) {
					tmp.top = cur.top;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.top+dv;
					src_tmp.top = src.top;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.top+dv;
					fc_gray_char8(cur_base, cur_row, tmp,
								  src_base, src_row, src_tmp, colors);
				}
				/* bottom not overlaping band */	
				dv = cur.bottom-inter.bottom-1;
				if (dv >= 0) {
					tmp.top = cur.bottom-dv;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.bottom;
					src_tmp.top = src.bottom-dv;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom;
					fc_gray_char8(cur_base, cur_row, tmp,
								  src_base, src_row, src_tmp, colors);
				}
				/* at least the horrible overlaping band... */
				src_tmp.top = src.top + (inter.top-cur.top);
				src_tmp.left = src.left + (inter.left-cur.left);
				src_tmp.right = src.right + (inter.right-cur.right);
				src_tmp.bottom = src.bottom + (inter.bottom-cur.bottom);
				src_prev_tmp.top = src_prev.top + (inter.top-prev.top);
				src_prev_tmp.left = src_prev.left + (inter.left-prev.left);
				src_prev_tmp.right = src_prev.right + (inter.right-prev.right);
				src_prev_tmp.bottom = src_prev.bottom + (inter.bottom-prev.bottom);
				fc_gray_char8x(cur_base, cur_row, inter,
							   src_prev_base, src_prev_row, src_prev_tmp,
							   src_base, src_row, src_tmp, colors);
			}
			src_prev_row = src_row;
			src_prev_base = src_base;
			src_prev.top = src.top;
			src_prev.left = src.left;
			src_prev.right = src.right;
			src_prev.bottom = src.bottom;
			prev.top = cur.top;
			prev.left = cur.left;
			prev.right = cur.right;
			prev.bottom = cur.bottom;
		}
	}
};

static void DrawStringCopyPreprocess(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int			i, r0, g0, b0, dr, dg, db, index;
	uint32 *	colors = cache->fontColors;
	uchar *		inverted;
	rgb_color	foreColor,backColor;

	grAssertLocked(context, port);
	
	inverted = canvas->pixels.colorMap->inverted;
	foreColor = context->foreColor;
	backColor = context->backColor;

	index =	((foreColor.red		<< 7) & (0xf8 << 7)) |
			((foreColor.green	<< 2) & (0xf8 << 2)) |
			((foreColor.blue	>> 3) & (0xf8 >> 3));
	colors[7] = inverted[index];
	r0 = backColor.red 		<< 8;
	g0 = backColor.green 	<< 8;
	b0 = backColor.blue 	<< 8;
	dr = ((foreColor.red	<< 8) - r0) / 7;
	dg = ((foreColor.green	<< 8) - g0) / 7;
	db = ((foreColor.blue	<< 8) - b0) / 7;
	r0 += 0x80; g0 += 0x80; b0 += 0x80;
	for (i=1; i<7; i++) {
		r0 += dr; g0 += dg; b0 += db;
		index = 	((r0>> 1) & (0xf8 << 7)) |
					((g0>> 6) & (0xf8 << 2)) |
					((b0>>11) & (0xf8 >> 3));
		colors[i] = inverted[index];
	}
	for (i=8; i<15; i++) colors[i] = colors[7];

	cache->fDrawString = DrawStringCopy;
	DrawStringCopy(context,port,str);
};

static void DrawStringOtherPreprocess(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	int			i, r0, g0, b0, dr, dg, db, index;
	uint32 *	colors = cache->fontColors;
	int16		mode = context->drawOp;
	uchar *		inverted;
	rgb_color	foreColor,backColor;

	grAssertLocked(context, port);
	
	inverted = port->canvas->pixels.colorMap->inverted;
	foreColor = context->foreColor;
	backColor = context->backColor;

	if (mode == OP_ERASE) {
		dr = backColor.red;
		dg = backColor.green;
		db = backColor.blue;
	} else {
		dr = foreColor.red;
		dg = foreColor.green;
		db = foreColor.blue;
	}
	colors[0] = 0L;
	colors[7] = ((dr<<22)&0x3fc00000) | ((dg<<12)&0x0fe000) | (db<<2);
	colors[9] = dr+dg+db;
	dr <<= 5;
	dg <<= 5;
	db <<= 5;
	r0 = 0x80;
	g0 = 0x80;
	b0 = 0x80;
	for (i=1; i<7; i++) {
		r0 += dr;
		g0 += dg;
		b0 += db;
		colors[i] = ((r0<<14)&0x3fc00000) | ((g0<<4)&0x0fe000) | (b0>>6);
	}
	
	r0 = colors[7];
	r0 = ((r0>>15) & (0xf8 << 7)) |
		 ((r0>>10) & (0xf8 << 2)) |
		 ((r0>>5)  & (0xf8 >> 3));
	colors[8] = inverted[r0];

	cache->fDrawString = DrawStringOther;
	DrawStringOther(context,port,str);
};

static void AlphaChar32(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src,
							uint32 *colors, uint32 *alphas, uint8 *RGB2Index, uint32 *index2ARGB)
{
	int32	i, j, cmd, hmin, hmax, imin, imax;
	uint32	color,alpha,pixIn;
	uint8 *dst_ptr;

	color = colors[0];
	alpha = alphas[0];
	dst_ptr = (uint8*)(dst_base+dst_row*dst.top)+(dst.left-src.left);

	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (true) {
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
			/* draw the visible part */
			for (i=imin; i<imax; i++) {
				pixIn = index2ARGB[dst_ptr[i]];
				pixIn = (
								((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
								((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
							 ) + color;
				dst_ptr[i] = RGB2Index[
								((pixIn & 0x00F80000) >> 9) |
								((pixIn & 0x0000F800) >> 6) |
								((pixIn & 0x000000F8) >> 3) ];
			};
			hmin = hmax;
		}
	next_line:
		dst_ptr = ((uint8*)dst_ptr+dst_row);
	}
}

static void AlphaCharAA32(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src,
							uint32 *colors, uint32 *alphas, uint8 *RGB2Index, uint32 *index2ARGB)
{
	int32		i, j;
	uint32		pixIn,alpha,gray;
	uint8 *		dst_ptr,*src_ptr;

	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint8*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;
			alpha = alphas[gray];
			if (alpha == 0) {
				dst_ptr[i] = colors[7];
				continue;
			};
			
			if (alpha == 256) continue;

			pixIn = index2ARGB[dst_ptr[i]];
			pixIn = (
							((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
							((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
						 ) + colors[gray];
			dst_ptr[i] = RGB2Index[
							((pixIn & 0x00F80000) >> 9) |
							((pixIn & 0x0000F800) >> 6) |
							((pixIn & 0x000000F8) >> 3) ];

		}
		src_ptr += src_row;
		dst_ptr = ((uint8*)dst_ptr+dst_row);
	}
}

static void DrawStringAlpha(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	int         	src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        	prev, src_prev, cur, src, inter;
	rect        	tmp, src_tmp, src_prev_tmp;
	uchar       	*src_base, *cur_base, *src_prev_base;
	fc_char     	*my_char;
	fc_point    	*my_offset;
	AlphaDrawChar	alphaDrawChar;
	rect			clipbox;
	uint8 *			inverted;
	uint32 *		index2ARGB;
	int32 			clipIndex=0;
	
	grAssertLocked(context, port);
	
	/* preload context parameters */
	uint32 *colors = cache->fontColors,*alphas = cache->fontColors+8;
	region *clip = port->RealDrawingRegion();
	const rect* clipRects = clip->Rects();
	const int32 clipCount = clip->CountRects();
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	inverted = port->canvas->pixels.colorMap->inverted;
	index2ARGB = port->canvas->pixels.colorMap->index2ARGB;
	alphaDrawChar = (AlphaDrawChar)cache->fontColors[16];
		
	if ((clipIndex=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((clipIndex>0) && (clipRects[clipIndex].top > str->bbox.top)) clipIndex--;
	while ((clipIndex>0) && (clipRects[clipIndex].top == clipRects[clipIndex-1].top)) clipIndex--;

	for (;clipIndex<clipCount;clipIndex++) {
		clipbox = clipRects[clipIndex];
		if (clipbox.top > str->bbox.bottom) return;
		/* go through all the characters */
		for (int32 i=0; i<str->char_count; i++) {
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		
			alphaDrawChar(	cur_base, cur_row, cur,
							src_base, src_row, src,
							colors, alphas, inverted, index2ARGB);
		}
	}
}

static void noop(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
};

static void DrawStringAlphaPreprocess(
	RenderContext *	context,
	RenderSubPort *port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;
	uint32
		*colors = cache->fontColors,
		*alphas = cache->fontColors+8;
	int8 endianess = port->canvas->pixels.endianess;
	rgb_color fc = context->foreColor;

	grAssertLocked(context, port);
	
	if (fc.alpha == 0) {
		cache->fDrawString = noop;
		return;
	};

	if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {
		if (fc.alpha == 255) {
			DrawStringNonAAPreprocess(context,port,str);
			return;
		};

		alpha = fc.alpha;
		alpha += (alpha >> 7);
		da = (fc.alpha * alpha);
		dr = (fc.red * alpha);
		dg = (fc.green * alpha);
		db = (fc.blue * alpha);
		colors[0] = (((da<<16) | dg) & 0xFF00FF00) |
					(((dr<<8) | (db>>8)) & 0x00FF00FF) ;
		alphas[0] = 256-alpha;
		cache->fontColors[16] = (uint32)AlphaChar32;
	} else {
		da = alpha = fc.alpha;
		alpha += (alpha >> 7);
		pixIn = context->foreColorARGB;
		colors[0] = 0L;
		colors[7] = (
					((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
					((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
				 );
		alphas[0] = 256;
		alphas[7] = 256-alpha;
		
		da = (da<<8)/7;
		a0 = 0x80;
		
		for (i=1; i<7; i++) {
			a0 += da;
			alpha = (a0+0x80) >> 8;
			alpha += (alpha >> 7);
			colors[i] = (
						((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
						((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
					 );
			alphas[i] = 256 - alpha;
		}
		cache->fontColors[16] = (uint32)AlphaCharAA32;
	};
	cache->fDrawString = DrawStringAlpha;
	DrawStringAlpha(context,port,str);
};

extern BlitFunction BlitCopySameFormat;

static BlitFunction copyBlitTable[] = {
	BlitCopy1To8,
	BlitCopy1To8,
	BlitCopySameFormat,
	BlitCopySameFormat,
	BlitCopyLittle15To8,
	BlitCopyBig15To8,
	BlitCopyLittle16To8,
	BlitCopyBig16To8,
	BlitCopyLittle32To8,
	BlitCopyBig32To8
};

static BlitFunction overBlitTable[] = {
	BlitOver1To8,
	BlitOver1To8,
	BlitOver8To8,
	BlitOver8To8,
	BlitOverLittle15To8,
	BlitOverBig15To8,
	BlitCopyLittle16To8,
	BlitCopyBig16To8,
	BlitOverLittle32To8,
	BlitOverBig32To8
};

static BlitFunction eraseBlitTable[] = {
	BlitOver1To8,
	BlitOver1To8,
	BlitScaleFunction8To8,
	BlitScaleFunction8To8,
	BlitScaleFunctionLittle15To8,
	BlitScaleFunctionBig15To8,
	BlitScaleFunctionLittle16To8,
	BlitScaleFunctionBig16To8,
	BlitScaleFunctionLittle32To8,
	BlitScaleFunctionBig32To8
};

static BlitFunction alphaBlitTable[] = {
	BlitOver1To8,
	BlitOver1To8,
	BlitOver8To8,
	BlitOver8To8,
	BlitAlphaLittle15To8,
	BlitAlphaBig15To8,
	BlitCopyLittle16To8,
	BlitCopyBig16To8,
	BlitAlphaLittle32To8,
	BlitAlphaBig32To8
};

static BlitFunction scaleCopyBlitTable[] = {
	NULL,
	NULL,
	BlitScaleCopy8To8,
	BlitScaleCopy8To8,
	BlitScaleCopyLittle15To8,
	BlitScaleCopyBig15To8,
	BlitScaleCopyLittle16To8,
	BlitScaleCopyBig16To8,
	BlitScaleCopyLittle32To8,
	BlitScaleCopyBig32To8
};

static BlitFunction scaleOverBlitTable[] = {
	NULL,
	NULL,
	BlitScaleOver8To8,
	BlitScaleOver8To8,
	BlitScaleOverLittle15To8,
	BlitScaleOverBig15To8,
	BlitScaleCopyLittle16To8,
	BlitScaleCopyBig16To8,
	BlitScaleOverLittle32To8,
	BlitScaleOverBig32To8
};

static BlitFunction scaleAlphaBlitTable[] = {
	NULL,
	NULL,
	BlitScaleOver8To8,
	BlitScaleOver8To8,
	BlitScaleAlphaLittle15To8,
	BlitScaleAlphaBig15To8,
	BlitScaleCopyLittle16To8,
	BlitScaleCopyBig16To8,
	BlitScaleAlphaLittle32To8,
	BlitScaleAlphaBig32To8
};

static BlitFunction scaleConstAlphaBlitTable[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8To8,
	BlitScaleConstAlpha8To8,
	BlitScaleConstAlphaLittle15To8,
	BlitScaleConstAlphaBig15To8,
	BlitScaleConstAlphaLittle16To8,
	BlitScaleConstAlphaBig16To8,
	BlitScaleConstAlphaLittle32To8,
	BlitScaleConstAlphaBig32To8
};

static BlitFunction scaleFunctionBlitTable[] = {
	NULL,
	NULL,
	BlitScaleFunction8To8,
	BlitScaleFunction8To8,
	BlitScaleFunctionLittle15To8,
	BlitScaleFunctionBig15To8,
	BlitScaleFunctionLittle16To8,
	BlitScaleFunctionBig16To8,
	BlitScaleFunctionLittle32To8,
	BlitScaleFunctionBig32To8
};

static DrawOneLine fDrawOneLine8[NUM_OPS][2] = 
{
	{ &DrawOneLineCopy8, &DrawOneLineCopyP8 },
	{ &DrawOneLineOverErase8, &DrawOneLineOverEraseP8 },
	{ &DrawOneLineOverErase8, &DrawOneLineOverEraseP8 },
	{ &DrawOneLineInvert8, &DrawOneLineInvertP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineOther8, &DrawOneLineOtherP8 },
	{ &DrawOneLineAlpha8, &DrawOneLineAlphaP8 }
};

static FillRects fFillRects8[NUM_OPS][2] = 
{
	{ &FillRectsCopy8, &FillRectsCopyP8 },
	{ &FillRectsCopy8, &FillRectsOverEraseP8 },
	{ &FillRectsCopy8, &FillRectsOverEraseP8 },
	{ &FillRectsInvert8, &FillRectsInvertP8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsOther8, &FillRectsOther8 },
	{ &FillRectsAlpha8, &FillRectsAlphaP8 }
};

static BlitFunction *fBlitTables[NUM_OPS] =
{
	copyBlitTable,
	overBlitTable,
	eraseBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	NULL
};

static BlitFunction *fScaledBlitTables[NUM_OPS] =
{
	scaleCopyBlitTable,
	scaleOverBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	scaleFunctionBlitTable,
	NULL
};

static DrawString DrawStringTable8[2][NUM_OPS] =
{
	{
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringAlphaPreprocess
	},
	{
		DrawStringCopyPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringAlphaPreprocess
	}
};

static void ColorChanged(
	RenderContext *context,
	RenderCache *cache,
	RenderSubPort *port,
	rgb_color fc,
	uint32 *canvasFormat,
	uint32 *srcAlpha,
	uint32 *alphaPix)
{
	uint32 pixIn,alpha,tmp,index;

	if ((*((uint32*)&fc) == TRANSPARENT_RGBA) ||
		(*((uint32*)&fc) == *((uint32*)&port->canvas->overlayHack))) {
		tmp = TRANSPARENT_MAGIC_8BIT;
	} else {
		index  = ((uint16)(fc.red & 0xF8)) << 7;
		index |= ((uint16)(fc.green & 0xF8)) << 2;
		index |= ((uint16)fc.blue) >> 3;
		tmp = port->canvas->pixels.colorMap->inverted[index];
	};
	tmp |= tmp << 8;
	tmp |= tmp << 16;
	*canvasFormat = tmp;

	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable8
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};

	alpha = fc.alpha;
	alpha += alpha >> 7;
	*srcAlpha = alpha;
	fc.red = (fc.red * alpha) >> 8;
	fc.green = (fc.green * alpha) >> 8;
	fc.blue = (fc.blue * alpha) >> 8;
	index  = ((uint16)(fc.red & 0xF8)) << 7;
	index |= ((uint16)(fc.green & 0xF8)) << 2;
	index |= ((uint16)fc.blue) >> 3;
	*alphaPix = index;
};

static void ForeColorChanged8(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged(
		context,cache,port,context->foreColor,
		&cache->foreColorCanvasFormat,
		&cache->foreSrcAlpha,
		&cache->foreAlphaPix);
};

static void BackColorChanged8(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged(
		context,cache,port,context->backColor,
		&cache->backColorCanvasFormat,
		&cache->backSrcAlpha,
		&cache->backAlphaPix);
};

void PickRenderers8(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects8[op][stippling];
	cache->fDrawOneLine = fDrawOneLine8[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
#if ROTATE_DISPLAY
	cache->fRotatedBlitTable = NULL;
	cache->fScaledRotatedBlitTable = NULL;
#endif
	cache->fBlitTable = fBlitTables[op];
	cache->fScaledBlitTable = fScaledBlitTables[op];
	if (!cache->fBlitTable) {
		if (context->srcAlphaSrc == SOURCE_ALPHA_PIXEL) {
			cache->fBlitTable = alphaBlitTable;
			cache->fScaledBlitTable = scaleAlphaBlitTable;
		} else {
			cache->fBlitTable =
			cache->fScaledBlitTable = scaleConstAlphaBlitTable;
		};
	};
	
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[PIX_8BIT*10+op];

	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable8
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};
};

extern void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port);
extern void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port);

RenderingPackage renderPackage8 = {
	PickRenderers8,
	ForeColorChanged8,
	BackColorChanged8
};
