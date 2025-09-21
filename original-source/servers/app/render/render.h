ess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode
#undef DestBits

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define DestBits												16
	#define RenderMode											DEFINE_OP_COPY
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy1ToLittle16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopy8ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyLittle15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyBig15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyLittle16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyBig16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyLittle32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleCopyBig32ToLittle16
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
					#define BlitFuncName						BlitScaleCopy1ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToBig16
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
					#define BlitFuncName						BlitScaleOver1ToLittle16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver8ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOver8ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverLittle15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverBig15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverLittle16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverBig16ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverLittle32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleOverBig32ToLittle16
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
					#define BlitFuncName						BlitScaleOver1ToBig32
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver8ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_BLEND
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
	#define SourceAlpha											SOURCE_ALPHA_PIXEL
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleAlphaLittle15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleAlphaBig15ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleAlphaLittle32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig32ToLittle16
					#include "blitting.inc"
#if ROTATE_DISPLAY
					#define RotatedBlit							1
					#define BlitFuncName						RotatedBlitScaleAlphaBig32ToLittle16
					#include "blitting.inc"
					#undef RotatedBlit
#endif
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess								BIG_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName		