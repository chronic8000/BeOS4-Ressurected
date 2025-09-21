
#include <Debug.h>
#include <math.h>

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "accelPackage.h"
#include "renderdefs.h"
#include "renderLock.h"

#include "as_debug.h"

extern void fc_bw_char32(uchar *dst_base, int dst_row, rect dst, uchar *src_base, rect src, uint colors);
extern void fc_bw_char32other(uchar *dst_base, int dst_row, rect dst, uchar *src_base, rect src, int mode, uint colors, uint b_colors, int energy);
extern void fc_gray_char32(uchar *dst_base, int dst_row, rect dst, uchar *src_base, int src_row, rect src, uint *colors);
extern void fc_gray_char32x(uchar *dst_base, int dst_row, rect dst, uchar *src2_base, int src2_row, rect src2, uchar *src_base, int src_row, rect src, uint *colors);
extern void fc_gray_char32other(uchar *dst_base, int dst_row, rect dst, uchar *src_base, int src_row, rect src, int16 mode, uint *colors, uint *b_colors);
extern void fc_gray_char32over(uchar *dst_base, int dst_row,rect dst, uchar *src_base,int src_row, rect src, int16 mode, uint *colors, uint *b_colors);

#define DestBits 32

#define RenderMode DEFINE_OP_COPY
	#define RenderPattern 0
		#define DrawOneLineFunctionName DrawOneLineCopy32
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineCopyP32
		#define FillRectsFunctionName FillRectsCopyP32
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_OVER
	#define RenderPattern 0
		#define DrawOneLineFunctionName DrawOneLineOverErase32
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineOverEraseP32
		#define FillRectsFunctionName FillRectsOverEraseP32
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_INVERT
	#define RenderPattern 0
		#define FillRectsFunctionName FillRectsInvert32
		#define DrawOneLineFunctionName DrawOneLineInvert32
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineInvertP32
		#define FillRectsFunctionName FillRectsInvertP32
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode
#define RenderMode DEFINE_OP_BLEND
	#define AlphaFunction ALPHA_FUNCTION_OVERLAY
		#define SourceAlpha SOURCE_ALPHA_PIXEL
			#define DestEndianess LITTLE_ENDIAN
				#define RenderPattern 0
					#define FillRectsFunctionName FillRectsAlpha32Little
					#define DrawOneLineFunctionName DrawOneLineAlpha32Little
					#include "fillspan.inc"
				#undef RenderPattern
				#define RenderPattern 1
					#define FillRectsFunctionName FillRectsAlphaP32Little
					#define DrawOneLineFunctionName DrawOneLineAlphaP32Little
					#include "fillspan.inc"
				#undef RenderPattern
			#undef DestEndianess
			#define DestEndianess BIG_ENDIAN
				#define RenderPattern 0
					#define FillRectsFunctionName FillRectsAlpha32Big
					#define DrawOneLineFunctionName DrawOneLineAlpha32Big
					#include "fillspan.inc"
				#undef RenderPattern
				#define RenderPattern 1
					#define FillRectsFunctionName FillRectsAlphaP32Big
					#define DrawOneLineFunctionName DrawOneLineAlphaP32Big
					#include "fillspan.inc"
				#undef RenderPattern
			#undef DestEndianess
		#undef SourceAlpha
	#undef AlphaFunction
	#define AlphaFunction ALPHA_FUNCTION_COMPOSITE
		#define SourceAlpha SOURCE_ALPHA_PIXEL
			#define DestEndianess LITTLE_ENDIAN
				#define RenderPattern 0
					#define FillRectsFunctionName FillRectsAlphaComposite32Little
					#define DrawOneLineFunctionName DrawOneLineAlphaComposite32Little
					#include "fillspan.inc"
				#undef RenderPattern
				#define RenderPattern 1
					#define FillRectsFunctionName FillRectsAlphaCompositeP32Little
					#define DrawOneLineFunctionName DrawOneLineAlphaCompositeP32Little
					#include "fillspan.inc"
				#undef RenderPattern
			#undef DestEndianess
			#define DestEndianess BIG_ENDIAN
				#define RenderPattern 0
					#define FillRectsFunctionName FillRectsAlphaComposite32Big
					#define DrawOneLineFunctionName DrawOneLineAlphaComposite32Big
					#include "fillspan.inc"
				#undef RenderPattern
				#define RenderPattern 1
					#define FillRectsFunctionName FillRectsAlphaCompositeP32Big
					#define DrawOneLineFunctionName DrawOneLineAlphaCompositeP32Big
					#include "fillspan.inc"
				#undef RenderPattern
			#undef DestEndianess
		#undef SourceAlpha
	#undef AlphaFunction
#undef RenderMode
#define RenderMode DEFINE_OP_OTHER
	#define RenderPattern 0
		#define DrawOneLineFunctionName DrawOneLineOther32
		#define FillRectsFunctionName FillRectsOther32
		#include "fillspan.inc"
	#undef RenderPattern
	#define RenderPattern 1
		#define DrawOneLineFunctionName DrawOneLineOtherP32
		#define FillRectsFunctionName FillRectsOtherP32
		#include "fillspan.inc"
	#undef RenderPattern
#undef RenderMode


#define DestBits 												32

#define BlitScaling 											0
	#define RenderMode											DEFINE_OP_COPY
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopy1ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopy8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopyLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitCopyBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopyLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitCopyBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitCopyBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopy1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopy8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopyLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitCopyBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopyLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitCopyBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitCopyLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_OVER
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOver1ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOver8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOverLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitOverBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOverLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitOverBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOver1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOver8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOverLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitOverBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitOverLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitOverBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_BLEND
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitAlphaLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitAlphaBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef DestEndianess
			#define DestEndianess								BIG_ENDIAN
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitAlphaLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitAlphaBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess

		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitAlphaLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitAlphaBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef DestEndianess
			#define DestEndianess								BIG_ENDIAN
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitAlphaLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitAlphaBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode
#undef BlitScaling

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define BlitScaling 											1
	#define RenderMode											DEFINE_OP_COPY
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy1ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_OVER
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver1ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_BLEND
		#define AlphaFunction 									ALPHA_FUNCTION_OVERLAY
		#define SourceAlpha 									SOURCE_ALPHA_PIXEL
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess								BIG_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#undef SourceAlpha
		#define SourceAlpha 									SOURCE_ALPHA_CONSTANT
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#undef SourceAlpha
		#undef AlphaFunction
		#define AlphaFunction 									ALPHA_FUNCTION_COMPOSITE
		#define SourceAlpha 									SOURCE_ALPHA_PIXEL
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess								BIG_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#undef SourceAlpha
		#define SourceAlpha 									SOURCE_ALPHA_CONSTANT
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaComposite8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess								BIG_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaComposite8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig32ToBig32
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
					#define BlitFuncName						BlitScaleFunction1ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction8ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToLittle32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToLittle32
					#include "blitting.inc"
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
					#define BlitFuncName						BlitScaleFunction8ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode
#undef BlitScaling

static void FillRectsCopyOverErase32(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
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

	color = (drawOp == OP_COPY) ? *(&port->cache->foreColorCanvasFormat + context->solidColor)
								: ((context->drawOp==OP_OVER) 	? port->cache->foreColorCanvasFormat
																: port->cache->backColorCanvasFormat);

	while (rectCount) {
		curClipRect = clipRects;
		clips = clipCount;
		while (clips) {
			thisRect.left = curSrcRect->left + offset.h;
			thisRect.right = curSrcRect->right + offset.h;
			thisRect.top = curSrcRect->top + offset.v;
			thisRect.bottom = curSrcRect->bottom + offset.v;
			if (curClipRect->top > thisRect.top) thisRect.top = curClipRect->top;
			if (curClipRect->bottom < thisRect.bottom) thisRect.bottom = curClipRect->bottom;
			if (curClipRect->left > thisRect.left) thisRect.left = curClipRect->left;
			if (curClipRect->right < thisRect.right) thisRect.right = curClipRect->right;
			if ((thisRect.top <= thisRect.bottom) && (thisRect.left <= thisRect.right)) {
				height = (thisRect.bottom-thisRect.top+1);
				width = (thisRect.right-thisRect.left+1);
				pixelInc = rowBytes - (width<<2);
				pixel = (uint8*)(base+(thisRect.top*rowBytes)+(thisRect.left<<2));
				while (height) {
					i = width;
					while (i--) {
						*((uint32*)pixel) = color;
						pixel+=4;
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

extern BlitFunction BlitCopySameFormat;

static BlitFunction copyBlitTableLittle[] = {
	BlitCopy1ToLittle32,
	BlitCopy1ToLittle32,
	BlitCopy8ToLittle32,
	BlitCopy8ToLittle32,
	BlitCopyLittle15ToLittle32,
	BlitCopyBig15ToLittle32,
	BlitCopyLittle16ToLittle32,
	BlitCopyBig16ToLittle32,
	BlitCopySameFormat,
	BlitCopyBig32ToLittle32
};

static BlitFunction copyBlitTableBig[] = {
	BlitCopy1ToBig32,
	BlitCopy1ToBig32,
	BlitCopy8ToBig32,
	BlitCopy8ToBig32,
	BlitCopyLittle15ToBig32,
	BlitCopyBig15ToBig32,
	BlitCopyLittle16ToBig32,
	BlitCopyBig16ToBig32,
	BlitCopyLittle32ToBig32,
	BlitCopySameFormat
};

static BlitFunction overBlitTableLittle[] = {
	BlitOver1ToLittle32,
	BlitOver1ToLittle32,
	BlitOver8ToLittle32,
	BlitOver8ToLittle32,
	BlitOverLittle15ToLittle32,
	BlitOverBig15ToLittle32,
	BlitCopyLittle16ToLittle32,
	BlitCopyBig16ToLittle32,
	BlitOverLittle32ToLittle32,
	BlitOverBig32ToLittle32
};

static BlitFunction overBlitTableBig[] = {
	BlitOver1ToBig32,
	BlitOver1ToBig32,
	BlitOver8ToBig32,
	BlitOver8ToBig32,
	BlitOverLittle15ToBig32,
	BlitOverBig15ToBig32,
	BlitCopyLittle16ToBig32,
	BlitCopyBig16ToBig32,
	BlitOverLittle32ToBig32,
	BlitOverBig32ToBig32
};

static BlitFunction eraseBlitTableLittle[] = {
	BlitOver1ToLittle32,
	BlitOver1ToLittle32,
	BlitScaleFunction8ToLittle32,
	BlitScaleFunction8ToLittle32,
	BlitScaleFunctionLittle15ToLittle32,
	BlitScaleFunctionBig15ToLittle32,
	BlitScaleFunctionLittle16ToLittle32,
	BlitScaleFunctionBig16ToLittle32,
	BlitScaleFunctionLittle32ToLittle32,
	BlitScaleFunctionBig32ToLittle32
};

static BlitFunction eraseBlitTableBig[] = {
	BlitOver1ToBig32,
	BlitOver1ToBig32,
	BlitScaleFunction8ToBig32,
	BlitScaleFunction8ToBig32,
	BlitScaleFunctionLittle15ToBig32,
	BlitScaleFunctionBig15ToBig32,
	BlitScaleFunctionLittle16ToBig32,
	BlitScaleFunctionBig16ToBig32,
	BlitScaleFunctionLittle32ToBig32,
	BlitScaleFunctionBig32ToBig32
};

static BlitFunction alphaBlitTableLittle[] = {
	BlitOver1ToLittle32,
	BlitOver1ToLittle32,
	BlitOver8ToLittle32,
	BlitOver8ToLittle32,
	BlitAlphaLittle15ToLittle32,
	BlitAlphaBig15ToLittle32,
	BlitCopyLittle16ToLittle32,
	BlitCopyBig16ToLittle32,
	BlitAlphaLittle32ToLittle32,
	BlitAlphaBig32ToLittle32
};

static BlitFunction alphaBlitTableBig[] = {
	BlitOver1ToBig32,
	BlitOver1ToBig32,
	BlitOver8ToBig32,
	BlitOver8ToBig32,
	BlitAlphaLittle15ToBig32,
	BlitAlphaBig15ToBig32,
	BlitCopyLittle16ToBig32,
	BlitCopyBig16ToBig32,
	BlitAlphaLittle32ToBig32,
	BlitAlphaBig32ToBig32
};

static BlitFunction scaleCopyBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToLittle32,
	BlitScaleCopy8ToLittle32,
	BlitScaleCopyLittle15ToLittle32,
	BlitScaleCopyBig15ToLittle32,
	BlitScaleCopyLittle16ToLittle32,
	BlitScaleCopyBig16ToLittle32,
	BlitScaleCopyLittle32ToLittle32,
	BlitScaleCopyBig32ToLittle32
};

static BlitFunction scaleCopyBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToBig32,
	BlitScaleCopy8ToBig32,
	BlitScaleCopyLittle15ToBig32,
	BlitScaleCopyBig15ToBig32,
	BlitScaleCopyLittle16ToBig32,
	BlitScaleCopyBig16ToBig32,
	BlitScaleCopyLittle32ToBig32,
	BlitScaleCopyBig32ToBig32
};

static BlitFunction scaleOverBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle32,
	BlitScaleOver8ToLittle32,
	BlitScaleOverLittle15ToLittle32,
	BlitScaleOverBig15ToLittle32,
	BlitScaleCopyLittle16ToLittle32,
	BlitScaleCopyBig16ToLittle32,
	BlitScaleOverLittle32ToLittle32,
	BlitScaleOverBig32ToLittle32
};

static BlitFunction scaleOverBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig32,
	BlitScaleOver8ToBig32,
	BlitScaleOverLittle15ToBig32,
	BlitScaleOverBig15ToBig32,
	BlitScaleCopyLittle16ToBig32,
	BlitScaleCopyBig16ToBig32,
	BlitScaleOverLittle32ToBig32,
	BlitScaleOverBig32ToBig32
};

static BlitFunction scaleAlphaBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle32,
	BlitScaleOver8ToLittle32,
	BlitScaleAlphaLittle15ToLittle32,
	BlitScaleAlphaBig15ToLittle32,
	BlitScaleCopyLittle16ToLittle32,
	BlitScaleCopyBig16ToLittle32,
	BlitScaleAlphaLittle32ToLittle32,
	BlitScaleAlphaBig32ToLittle32
};

static BlitFunction scaleAlphaBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig32,
	BlitScaleOver8ToBig32,
	BlitScaleAlphaLittle15ToBig32,
	BlitScaleAlphaBig15ToBig32,
	BlitScaleCopyLittle16ToBig32,
	BlitScaleCopyBig16ToBig32,
	BlitScaleAlphaLittle32ToBig32,
	BlitScaleAlphaBig32ToBig32
};

static BlitFunction scaleConstAlphaBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToLittle32,
	BlitScaleConstAlpha8ToLittle32,
	BlitScaleConstAlphaLittle15ToLittle32,
	BlitScaleConstAlphaBig15ToLittle32,
	BlitScaleConstAlphaLittle16ToLittle32,
	BlitScaleConstAlphaBig16ToLittle32,
	BlitScaleConstAlphaLittle32ToLittle32,
	BlitScaleConstAlphaBig32ToLittle32
};

static BlitFunction scaleConstAlphaBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToBig32,
	BlitScaleConstAlpha8ToBig32,
	BlitScaleConstAlphaLittle15ToBig32,
	BlitScaleConstAlphaBig15ToBig32,
	BlitScaleConstAlphaLittle16ToBig32,
	BlitScaleConstAlphaBig16ToBig32,
	BlitScaleConstAlphaLittle32ToBig32,
	BlitScaleConstAlphaBig32ToBig32
};

static BlitFunction scaleConstAlphaCompositeBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleConstAlphaComposite8ToLittle32,
	BlitScaleConstAlphaComposite8ToLittle32,
	BlitScaleConstAlphaCompositeLittle15ToLittle32,
	BlitScaleConstAlphaCompositeBig15ToLittle32,
	BlitScaleConstAlphaCompositeLittle16ToLittle32,
	BlitScaleConstAlphaCompositeBig16ToLittle32,
	BlitScaleConstAlphaCompositeLittle32ToLittle32,
	BlitScaleConstAlphaCompositeBig32ToLittle32
};

static BlitFunction scaleConstAlphaCompositeBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleConstAlphaComposite8ToBig32,
	BlitScaleConstAlphaComposite8ToBig32,
	BlitScaleConstAlphaCompositeLittle15ToBig32,
	BlitScaleConstAlphaCompositeBig15ToBig32,
	BlitScaleConstAlphaCompositeLittle16ToBig32,
	BlitScaleConstAlphaCompositeBig16ToBig32,
	BlitScaleConstAlphaCompositeLittle32ToBig32,
	BlitScaleConstAlphaCompositeBig32ToBig32
};

static BlitFunction scaleAlphaCompositeBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle32,
	BlitScaleOver8ToLittle32,
	BlitScaleAlphaCompositeLittle15ToLittle32,
	BlitScaleAlphaCompositeBig15ToLittle32,
	BlitScaleCopyLittle16ToLittle32,
	BlitScaleCopyBig16ToLittle32,
	BlitScaleAlphaCompositeLittle32ToLittle32,
	BlitScaleAlphaCompositeBig32ToLittle32
};

static BlitFunction scaleAlphaCompositeBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig32,
	BlitScaleOver8ToBig32,
	BlitScaleConstAlphaCompositeLittle15ToBig32,
	BlitScaleConstAlphaCompositeBig15ToBig32,
	BlitScaleCopyLittle16ToBig32,
	BlitScaleCopyBig16ToBig32,
	BlitScaleAlphaCompositeLittle32ToBig32,
	BlitScaleAlphaCompositeBig32ToBig32
};

static BlitFunction scaleFunctionBlitTableLittle[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToLittle32,
	BlitScaleFunction8ToLittle32,
	BlitScaleFunctionLittle15ToLittle32,
	BlitScaleFunctionBig15ToLittle32,
	BlitScaleFunctionLittle16ToLittle32,
	BlitScaleFunctionBig16ToLittle32,
	BlitScaleFunctionLittle32ToLittle32,
	BlitScaleFunctionBig32ToLittle32
};

static BlitFunction scaleFunctionBlitTableBig[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToBig32,
	BlitScaleFunction8ToBig32,
	BlitScaleFunctionLittle15ToBig32,
	BlitScaleFunctionBig15ToBig32,
	BlitScaleFunctionLittle16ToBig32,
	BlitScaleFunctionBig16ToBig32,
	BlitScaleFunctionLittle32ToBig32,
	BlitScaleFunctionBig32ToBig32
};

static BlitFunction *fBlitTablesLittle[NUM_OPS] =
{
	copyBlitTableLittle,
	overBlitTableLittle,
	eraseBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	NULL
};

static BlitFunction *fScaledBlitTablesLittle[NUM_OPS] =
{
	scaleCopyBlitTableLittle,
	scaleOverBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	scaleFunctionBlitTableLittle,
	NULL
};

static BlitFunction *fBlitTablesBig[NUM_OPS] =
{
	copyBlitTableBig,
	overBlitTableBig,
	eraseBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	NULL
};

static BlitFunction *fScaledBlitTablesBig[NUM_OPS] =
{
	scaleCopyBlitTableBig,
	scaleOverBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	scaleFunctionBlitTableBig,
	NULL
};

static BlitFunction *AlphaLittle[2][2] =
{
	{
		alphaBlitTableLittle,
		scaleAlphaCompositeBlitTableLittle
	},
	{
		scaleConstAlphaBlitTableLittle,
		scaleConstAlphaCompositeBlitTableLittle
	}
};

static BlitFunction *AlphaBig[2][2] =
{
	{
		alphaBlitTableBig,
		scaleAlphaCompositeBlitTableBig
	},
	{
		scaleConstAlphaBlitTableBig,
		scaleConstAlphaCompositeBlitTableBig
	}
};

static BlitFunction *ScaledAlphaLittle[2][2] =
{
	{
		scaleAlphaBlitTableLittle,
		scaleAlphaCompositeBlitTableLittle
	},
	{
		scaleConstAlphaBlitTableLittle,
		scaleConstAlphaCompositeBlitTableLittle
	}
};

static BlitFunction *ScaledAlphaBig[2][2] =
{
	{
		scaleAlphaBlitTableBig,
		scaleAlphaCompositeBlitTableBig
	},
	{
		scaleConstAlphaBlitTableBig,
		scaleConstAlphaCompositeBlitTableBig
	}
};

extern DrawString DrawStringTable32[2][NUM_OPS];

static DrawOneLine fDrawOneLine32[NUM_OPS][2] = 
{
	{ &DrawOneLineCopy32, &DrawOneLineCopyP32 },
	{ &DrawOneLineOverErase32, &DrawOneLineOverEraseP32 },
	{ &DrawOneLineOverErase32, &DrawOneLineOverEraseP32 },
	{ &DrawOneLineInvert32, &DrawOneLineInvertP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ &DrawOneLineOther32, &DrawOneLineOtherP32 },
	{ NULL,NULL }
};

static FillRects fFillRects32[NUM_OPS][2] = 
{
	{ &FillRectsCopyOverErase32, &FillRectsCopyP32 },
	{ &FillRectsCopyOverErase32, &FillRectsOverEraseP32 },
	{ &FillRectsCopyOverErase32, &FillRectsOverEraseP32 },
	{ &FillRectsInvert32, &FillRectsInvertP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ &FillRectsOther32, &FillRectsOtherP32 },
	{ NULL,NULL }
};

static void PickRenderers32Little(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects32[op][stippling];
	cache->fDrawOneLine = fDrawOneLine32[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
#if ROTATE_DISPLAY
	cache->fRotatedBlitTable = NULL;
	cache->fScaledRotatedBlitTable = NULL;
#endif
	cache->fBlitTable = fBlitTablesLittle[op];
	cache->fScaledBlitTable = fScaledBlitTablesLittle[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[PIX_32BIT*10+op];

	if (!cache->fBlitTable) {
		cache->fBlitTable = AlphaLittle[context->srcAlphaSrc][context->alphaFunction];
		cache->fScaledBlitTable = ScaledAlphaLittle[context->srcAlphaSrc][context->alphaFunction];
	};

	if (!cache->fFillRects) {
		if (context->alphaFunction == ALPHA_FUNCTION_COMPOSITE) {
			cache->fFillRects = FillRectsAlphaComposite32Little;
			cache->fDrawOneLine = DrawOneLineAlphaComposite32Little;
		} else {
			cache->fFillRects = FillRectsAlpha32Little;
			cache->fDrawOneLine = DrawOneLineAlpha32Little;
		};
	};

	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][op];
	};
};

static void PickRenderers32Big(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects32[op][stippling];
	cache->fDrawOneLine = fDrawOneLine32[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
#if ROTATE_DISPLAY
	cache->fRotatedBlitTable = NULL;
	cache->fScaledRotatedBlitTable = NULL;
#endif
	cache->fBlitTable = fBlitTablesBig[op];
	cache->fScaledBlitTable = fScaledBlitTablesBig[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[(5*10)+PIX_32BIT*10+op];

	if (!cache->fBlitTable) {
		cache->fBlitTable = AlphaBig[context->srcAlphaSrc][context->alphaFunction];
		cache->fScaledBlitTable = ScaledAlphaBig[context->srcAlphaSrc][context->alphaFunction];
	};

	if (!cache->fFillRects) {
		if (context->alphaFunction == ALPHA_FUNCTION_COMPOSITE) {
			cache->fFillRects = FillRectsAlphaComposite32Big;
			cache->fDrawOneLine = DrawOneLineAlphaComposite32Big;
		} else {
			cache->fFillRects = FillRectsAlpha32Big;
			cache->fDrawOneLine = DrawOneLineAlpha32Big;
		};
	};

	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][op];
	};
};

static void ForeColorChanged32Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	uint32 pixIn,alpha;
	cache->foreColorCanvasFormat = pixIn = context->foreColorARGB;
	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};

	alpha = context->foreColor.alpha;
	alpha += alpha >> 7;
	cache->foreSrcAlpha = alpha;
	cache->foreAlphaPix =
		(
			((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
			((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
		);
};

static void BackColorChanged32Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	uint32 pixIn,alpha;
	cache->backColorCanvasFormat = pixIn = context->backColorARGB;
	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};
	alpha = context->backColor.alpha;
	alpha += alpha >> 7;
	cache->backSrcAlpha = alpha;
	cache->backAlphaPix =
		(
			((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
			((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
		);
};

static void ForeColorChanged32NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	uint32 pixIn,alpha;
	rgb_color fc = context->foreColor;
	cache->foreColorCanvasFormat = pixIn = (fc.blue<<24)|(fc.green<<16)|(fc.red<<8)|(fc.alpha);
	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};

	alpha = context->foreColor.alpha;
	alpha += alpha >> 7;
	cache->foreSrcAlpha = alpha;
	cache->foreAlphaPix =
		(
			((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
			((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
		);
};

static void BackColorChanged32NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	uint32 pixIn,alpha;
	rgb_color fc = context->backColor;
	cache->backColorCanvasFormat = pixIn = (fc.blue<<24)|(fc.green<<16)|(fc.red<<8)|(fc.alpha);
	if (context->fontContext.IsValid()) {
		cache->fDrawString = DrawStringTable32
			[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE][context->drawOp];
	};

	alpha = context->backColor.alpha;
	alpha += alpha >> 7;
	cache->backSrcAlpha = alpha;
	cache->backAlphaPix =
		(
			((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
			((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
		);
};

extern void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port);
extern void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port);

RenderingPackage renderPackage32Little = {
	PickRenderers32Little,
#if B_HOST_IS_LENDIAN
	ForeColorChanged32Host,
	BackColorChanged32Host
#else
	ForeColorChanged32NonHost,
	BackColorChanged32NonHost
#endif
};

RenderingPackage renderPackage32Big = {
	PickRenderers32Big,
#if B_HOST_IS_LENDIAN
	ForeColorChanged32NonHost,
	BackColorChanged32NonHost
#else
	ForeColorChanged32Host,
	BackColorChanged32Host
#endif
};
