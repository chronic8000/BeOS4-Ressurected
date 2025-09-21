CE_ALPHA_CONSTANT
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlpha8ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaLittle15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaBig15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaLittle16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaBig16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaLittle32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleConstAlphaBig32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef SourceAlpha
	#undef AlphaFunction
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_FUNCTION
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction1ToLittle16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction8ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunction8ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionLittle15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionBig15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionLittle16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionBig16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionLittle32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleFunctionBig32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction8ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode
#undef DestBits
#undef BlitScaling

typedef void (*Precalc)(rgb_color fc, rgb_color bc, uint32 *colors);
typedef void (*AlphaDrawChar)(
	uchar *dst_base, int dst_row, rect dst,
	uchar *src_base, int src_row, rect src,
	uint32 *colors, uint32 *alpha);
typedef void (*DrawOtherChar)(
	uchar *dst_base, int dst_row, rect dst,
	uchar *src_base, int src_row, rect src,
	uint32 *colors, uint32 pixel, int mode);
typedef void (*DrawOtherBWChar)(
	uchar *dst_base, int dst_row, rect dst,
	uchar *src_base, rect src, uint32 pixel, int mode);

static void noop(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
};

static void FillRectsCopyOverErase1516(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	grAssertLocked(context, port);

	RenderCache *cache = port->cache;
	region *	clipRegion = port->RealDrawingRegion();
	int32		rowBytes=port->canvas->pixels.bytesPerRow,rc;
	uint8		*pixel,*base=((uint8*)port->canvas->pixels.pixelData);
	int32		i,width,height,pixelInc;
	uint32		color;
	int32		clips;
	rect		thisRect;
	const rect	*curSrcRect=fillRects,*curClipRect;
	int16		drawOp = context->drawOp;

#if ROTATE_DISPLAY
	point offset;
	offset.h = port->rotater->RotateDV(port->origin.v);
	offset.v = port->rotater->RotateDH(port->origin.h);
#else
	const point offset = port->origin;
#endif
	
	color = (drawOp == OP_COPY) ? *(&cache->foreColorCanvasFormat + context->solidColor)
								: ((context->drawOp==OP_OVER) 	? cache->foreColorCanvasFormat
																: cache->backColorCanvasFormat);

	while (rectCount) {
		curClipRect = clipRegion->Rects();
		clips = clipRegion->CountRects();
		while (clips) {
			thisRect = *curSrcRect;

			thisRect.left = curSrcRect->left + offset.h;
			thisRect.top = curSrcRect->top + offset.v;
			thisRect.right = curSrcRect->right + offset.h;
			thisRect.bottom = curSrcRect->bottom + offset.v;

			if (curClipRect->top > thisRect.top) thisRect.top = curClipRect->top;
			if (curClipRect->bottom < thisRect.bottom) thisRect.bottom = curClipRect->bottom;
			if (curClipRect->left > thisRect.left) thisRect.left = curClipRect->left;
			if (curClipRect->right < thisRect.right) thisRect.right = curClipRect->right;
			if ((thisRect.top <= thisRect.bottom) && (thisRect.left <= thisRect.right)) {
				height = (thisRect.bottom-thisRect.top+1);
				width = (thisRect.right-thisRect.left+1);
				pixelInc = rowBytes - (width<<1);
				pixel = (uint8*)(base+(thisRect.top*rowBytes)+(thisRect.left<<1));
				while (height) {
					i = width;
					while ((i > 0) && (((uint32)pixel) & 0x03)) {
						*((uint16*)pixel) = color;
						pixel += 2; i--;
					};
					while (i >= 2) {
						*((uint32*)pixel) = color;
						pixel+=4; i-=2;
					};
					if (i > 0) {
						*((uint16*)pixel) = color;
						pixel += 2;
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

static void DrawCopyBWChar1516 (	uchar *dst_base, int dst_row, rect dst,
									uchar *src_base, rect src, uint32 pixel, int mode)
{
	int			i, j, cmd, hmin, hmax, imin, imax;
	uint16 *	dst_ptr;
	uint8 *		dst_rowBase;

	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+(dst.left-src.left)*2);
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (TRUE) {
			do {
				cmd = *src_base++;
				if (cmd == 0xff)
					goto next_line;
				hmin += cmd;
			} while (cmd == 0xfe);
			hmax = hmin;
			do {
				cmd = *src_base++;
				hmax += cmd;
			} while (cmd == 0xfe);
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
			for (i=imin; i<imax; i++) dst_ptr[i] = pixel;
			hmin = hmax;
		}
		next_line:
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);
	}
}

#define DrawOtherBWChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, rect src, uint32 pixel, int mode)				\
{																						\
	int			i, j, cmd, hmin, hmax, imin, imax;										\
	uint16 *	dst_ptr;																\
	uint8 *		dst_rowBase;															\
	uint32		theColor,colorIn;														\
	bool		gr;																		\
																						\
	theColor = CanvasTo898(pixel)<<3;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+(dst.left-src.left)*2);\
	for (j=0; j<src.top; j++)															\
		do src_base++; while (src_base[-1] != 0xff);									\
	for (; j<=src.bottom; j++) {														\
		hmin = 0;																		\
		while (TRUE) {																	\
			do {																		\
				cmd = *src_base++;														\
				if (cmd == 0xff)														\
					goto next_line;														\
				hmin += cmd;															\
			} while (cmd == 0xfe);														\
			hmax = hmin;																\
			do {																		\
				cmd = *src_base++;														\
				hmax += cmd;															\
			} while (cmd == 0xfe);														\
			if (hmin > src.right) {														\
				do src_base++; while (src_base[-1] != 0xff);							\
				goto next_line;															\
			}																			\
			if (hmin >= src.left)														\
				imin = hmin;															\
			else																		\
				imin = src.left;														\
			if (hmax <= src.right)														\
				imax = hmax;															\
			else																		\
				imax = src.right+1;														\
			for (i=imin; i<imax; i++) {													\
				colorIn = dst_ptr[i];													\
				switch (mode) {															\
					case OP_SUB:														\
						colorIn = (theColor|(0x8040100<<3)) - (CanvasTo898(colorIn)<<3);\
						colorIn &= ((0x8000100<<3) - ((colorIn&(0x8000100<<3))>>8));	\
						colorIn &= ((0x0040000<<3) - ((colorIn&(0x0040000<<3))>>9));	\
						break;															\
					case OP_ADD:														\
						colorIn = theColor + (CanvasTo898(colorIn)<<3);					\
						colorIn |= ((0x8000100<<3) - ((colorIn&(0x8000100<<3))>>8));	\
						colorIn |= ((0x0040000<<3) - ((colorIn&(0x0040000<<3))>>9));	\
						break;															\
					case OP_BLEND:														\
						colorIn = (theColor+(CanvasTo898(colorIn)<<3))>>1;				\
						break;															\
					case OP_INVERT:														\
						colorIn = 0xFFFF - colorIn;										\
						goto alreadyInNativeFormat;										\
					case OP_MIN:														\
					case OP_MAX:														\
						colorIn = CanvasTo898(colorIn);									\
						gr =(															\
							 ((colorIn & 0x7F80000)>>18) +								\
							 ((colorIn & 0x003FE00)>> 9) +								\
							 (