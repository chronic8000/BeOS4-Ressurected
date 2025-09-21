
#include <support2/Debug.h>
#include <math.h>

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "accelPackage.h"
#include "renderdefs.h"
#include "renderLock.h"

#if ROTATE_DISPLAY
#include "rotatedBlit.inc"
#endif

#define DestBits 													16
	#define RenderMode 												DEFINE_OP_COPY
		#define RenderPattern 										0
			#define DrawLinesFunctionName DrawLinesCopy1516
			#define DrawOneLineFunctionName DrawOneLineCopy1516
			#include "fillspan.inc"
		#undef RenderPattern
		#define RenderPattern 										1
			#define DrawOneLineFunctionName DrawOneLineCopyP1516
			#define FillRectsFunctionName FillRectsCopyP1516
			#include "fillspan.inc"
		#undef RenderPattern
	#undef RenderMode
	#define RenderMode 												DEFINE_OP_OVER
		#define RenderPattern 										0
			#define DrawOneLineFunctionName DrawOneLineOverErase1516
			#include "fillspan.inc"
		#undef RenderPattern
		#define RenderPattern 										1
			#define DrawOneLineFunctionName DrawOneLineOverEraseP1516
			#define FillRectsFunctionName FillRectsOverEraseP1516
			#include "fillspan.inc"
		#undef RenderPattern
	#undef RenderMode
	#define RenderMode 												DEFINE_OP_INVERT
		#define RenderPattern 										0
			#define DrawOneLineFunctionName DrawOneLineInvert1516
			#define FillRectsFunctionName FillRectsInvert1516
			#include "fillspan.inc"
		#undef RenderPattern
		#define RenderPattern 										1
			#define DrawOneLineFunctionName DrawOneLineInvertP1516
			#define FillRectsFunctionName FillRectsInvertP1516
			#include "fillspan.inc"
		#undef RenderPattern
	#undef RenderMode
	#define RenderMode 												DEFINE_OP_BLEND
	#define SourceAlpha												SOURCE_ALPHA_PIXEL
	#define AlphaFunction											ALPHA_FUNCTION_OVERLAY
		#define DestEndianess										LITTLE_ENDIAN
			#define RenderPattern 									0
				#define FillRectsFunctionName FillRectsAlpha16Little
				#define DrawOneLineFunctionName DrawOneLineAlpha16Little
				#include "fillspan.inc"
			#undef RenderPattern
			#define RenderPattern 									1
				#define FillRectsFunctionName FillRectsAlphaP16Little
				#define DrawOneLineFunctionName DrawOneLineAlphaP16Little
				#include "fillspan.inc"
			#undef RenderPattern
		#undef DestEndianess
		#define DestEndianess										BIG_ENDIAN
			#define RenderPattern 									0
				#define FillRectsFunctionName FillRectsAlpha16Big
				#define DrawOneLineFunctionName DrawOneLineAlpha16Big
				#include "fillspan.inc"
			#undef RenderPattern
			#define RenderPattern 									1
				#define FillRectsFunctionName FillRectsAlphaP16Big
				#define DrawOneLineFunctionName DrawOneLineAlphaP16Big
				#include "fillspan.inc"
			#undef RenderPattern
		#undef DestEndianess
	#undef SourceAlpha
	#undef AlphaFunction
	#undef RenderMode
	#define RenderMode 												DEFINE_OP_OTHER
		#define RenderPattern 										0
			#define DrawOneLineFunctionName DrawOneLineOther1516
			#define FillRectsFunctionName FillRectsOther1516
			#include "fillspan.inc"
		#undef RenderPattern
		#define RenderPattern 										1
			#define DrawOneLineFunctionName DrawOneLineOtherP1516
			#define FillRectsFunctionName FillRectsOtherP1516
			#include "fillspan.inc"
		#undef RenderPattern
	#undef RenderMode
#undef DestBits

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define DestBits 													15
	#define RenderMode 												DEFINE_OP_BLEND
	#define SourceAlpha												SOURCE_ALPHA_PIXEL
	#define AlphaFunction											ALPHA_FUNCTION_OVERLAY
		#define DestEndianess										LITTLE_ENDIAN
			#define RenderPattern 									0
				#define FillRectsFunctionName FillRectsAlpha15Little
				#define DrawOneLineFunctionName DrawOneLineAlpha15Little
				#include "fillspan.inc"
			#undef RenderPattern
			#define RenderPattern 									1
				#define FillRectsFunctionName FillRectsAlphaP15Little
				#define DrawOneLineFunctionName DrawOneLineAlphaP15Little
				#include "fillspan.inc"
			#undef RenderPattern
		#undef DestEndianess
		#define DestEndianess										BIG_ENDIAN
			#define RenderPattern 									0
				#define FillRectsFunctionName FillRectsAlpha15Big
				#define DrawOneLineFunctionName DrawOneLineAlpha15Big
				#include "fillspan.inc"
			#undef RenderPattern
			#define RenderPattern 									1
				#define FillRectsFunctionName FillRectsAlphaP15Big
				#define DrawOneLineFunctionName DrawOneLineAlphaP15Big
				#include "fillspan.inc"
			#undef RenderPattern
		#undef DestEndianess
	#undef SourceAlpha
	#undef AlphaFunction
	#undef RenderMode
#undef DestBits

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

#define BlitScaling 												0
	#define RenderMode												DEFINE_OP_COPY
		#define DestBits 											15
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy1ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy8ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig15ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									16
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle16ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig16ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy1ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy8ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle15ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									16
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle16ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig16ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
		#undef DestBits
	
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

		#define DestBits 											16
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy1ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopy1ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy8ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopy8ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopyLittle15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopyBig15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									16
/*
#if ROTATE_DISPLAY
					#define SourceEndianess							LITTLE_ENDIAN
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopySameFormat16
						#include "blitting.inc"
						#undef RotatedBlit
					#undef SourceEndianess
#endif
*/
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig16ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopyBig16ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopyLittle32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitCopyBig32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy1ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopy8ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									16
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle16ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitCopyLittle32ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitCopyBig32ToBig16
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
		#define DestBits 											15
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver1ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver8ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle15ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig15ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver1ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver8ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle15ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig15ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
		#undef DestBits
	
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

		#define DestBits 											16
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver1ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOver1ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver8ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOver8ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOverLittle15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOverBig15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOverLittle32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitOverBig32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits									1
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver1ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									8
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOver8ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitOverLittle32ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitOverBig32ToBig16
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
	#define SourceAlpha												SOURCE_ALPHA_PIXEL
	#define AlphaFunction											ALPHA_FUNCTION_OVERLAY
		#define DestBits 											15
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits 									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle15ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig15ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits 									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig32ToLittle15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits 									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle15ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig15ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits 									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig32ToBig15
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
		#undef DestBits

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

		#define DestBits 											16
			#define DestEndianess									LITTLE_ENDIAN
				#define SourceBits 									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitAlphaLittle15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig15ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitAlphaBig15ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits 									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitAlphaLittle32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig32ToLittle16
						#include "blitting.inc"
#if ROTATE_DISPLAY
						#define RotatedBlit							1
						#define BlitFuncName						RotatedBlitAlphaBig32ToLittle16
						#include "blitting.inc"
						#undef RotatedBlit
#endif
					#undef SourceEndianess
				#undef SourceBits
			#undef DestEndianess
			#define DestEndianess									BIG_ENDIAN
				#define SourceBits 									15
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig15ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
				#undef SourceBits
				#define SourceBits 									32
					#define SourceEndianess							LITTLE_ENDIAN
						#define BlitFuncName						BlitAlphaLittle32ToBig16
						#include "blitting.inc"
					#undef SourceEndianess
					#define SourceEndianess							BIG_ENDIAN
						#define BlitFuncName						BlitAlphaBig32ToBig16
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

#define BlitScaling 											1
	#define RenderMode											DEFINE_OP_COPY
		#define DestBits										15
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy1ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy1ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopy8ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleCopyLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleCopyBig32ToBig15
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
					#define BlitFuncName						BlitScaleOver1ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOver8ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToLittle15
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
					#define BlitFuncName						BlitScaleOver8ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleOverLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleOverBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_BLEND
	#define SourceAlpha											SOURCE_ALPHA_PIXEL
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef AlphaFunction
	#define AlphaFunction										ALPHA_FUNCTION_COMPOSITE
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleAlphaCompositeBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef AlphaFunction
	#undef SourceAlpha
	#define SourceAlpha											SOURCE_ALPHA_CONSTANT
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlpha8ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef AlphaFunction
	#define AlphaFunction										ALPHA_FUNCTION_COMPOSITE
		#define DestEndianess									LITTLE_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaComposite8ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
		#define DestEndianess									BIG_ENDIAN
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaComposite8ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeLittle32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleConstAlphaCompositeBig32ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef AlphaFunction
	#undef SourceAlpha

	#undef RenderMode

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

	#define RenderMode											DEFINE_OP_FUNCTION
		#define DestEndianess									LITTLE_ENDIAN
/*
			#define SourceBits									1
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction1ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
*/
			#define SourceBits									8
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunction8ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig16ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									32
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle32ToLittle15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig32ToLittle15
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
					#define BlitFuncName						BlitScaleFunction8ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									15
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
				#define SourceEndianess							BIG_ENDIAN
					#define BlitFuncName						BlitScaleFunctionBig15ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
			#define SourceBits									16
				#define SourceEndianess							LITTLE_ENDIAN
					#define BlitFuncName						BlitScaleFunctionLittle16ToBig15
					#include "blitting.inc"
				#undef SourceEndianess
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
					#define BlitFuncName						BlitScaleAlphaBig32ToBig16
					#include "blitting.inc"
				#undef SourceEndianess
			#undef SourceBits
		#undef DestEndianess
	#undef SourceAlpha
	#undef AlphaFunction
	#define AlphaFunction										ALPHA_FUNCTION_OVERLAY
	#define SourceAlpha											SOURCE_ALPHA_CONSTANT
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
	offset.x = port->rotater->RotateDV(port->origin.y);
	offset.y = port->rotater->RotateDH(port->origin.x);
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

			thisRect.left = curSrcRect->left + offset.x;
			thisRect.top = curSrcRect->top + offset.y;
			thisRect.right = curSrcRect->right + offset.x;
			thisRect.bottom = curSrcRect->bottom + offset.y;

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
		while (true) {
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
		while (true) {																	\
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
							 ((colorIn & 0x00000FF)<< 1)								\
							) >															\
							(															\
							 (((theColor>>3) & 0x7F80000)>>18) +						\
							 (((theColor>>3) & 0x003FE00)>> 9) +						\
							 (((theColor>>3) & 0x00000FF)<< 1)							\
							);															\
					   if ((gr && (mode==OP_MIN)) ||									\
					       (!gr && (mode==OP_MAX))) {									\
							colorIn = pixel;											\
							goto alreadyInNativeFormat;									\
						} else goto next_pixel;											\
				};																		\
				colorIn = BackToCanvas(colorIn);										\
				alreadyInNativeFormat:													\
				dst_ptr[i] = colorIn;													\
				next_pixel:;															\
			};																			\
			hmin = hmax;																\
		}																				\
		next_line:																		\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
	}																					\
}

#define AlphaPreprocessFuncs(AlphaPreprocessName,AlphaDrawString,AlphaDrawChar,AlphaDrawCharAA)	\
static void AlphaDrawCharAA (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int32 i,j,gray;																				\
	uint32 colorIn,theColor;																	\
	uint16 *dst_ptr;																			\
	uint8 *src_ptr,*dst_rowBase;																\
	src_ptr = src_base+src_row*src.top;															\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);					\
	for (j=src.top; j<=src.bottom; j++) {														\
		for (i=src.left; i<=src.right; i++) {													\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;											\
			if (gray != 0) {																	\
				theColor = *dst_ptr;															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha[gray]) +									\
						colors[gray  ]) >> 8) & 0x00FF00FF) |									\
					(((((colorIn & 0x0000FF00) * alpha[gray]) +									\
						colors[gray+8]) >> 8) & 0x0000FF00) ;									\
				*dst_ptr = ComponentsToPixel(colorIn>>8,colorIn,colorIn<<8);					\
			};																					\
			dst_ptr++;																			\
		};																						\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);												\
		src_ptr += src_row;																		\
	};																							\
};																								\
																								\
static void AlphaDrawChar	 (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int			i, j, cmd, hmin, hmax, imin, imax;												\
	uint16 *	dst_ptr;																		\
	uint32		theColor,colorIn,alpha0=alpha[0],color0=colors[0],color8=colors[8];				\
	uint8 *		dst_rowBase;																	\
																								\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+(dst.left-src.left)*2);		\
	for (j=0; j<src.top; j++)																	\
		do src_base++; while (src_base[-1] != 0xff);											\
	for (; j<=src.bottom; j++) {																\
		hmin = 0;																				\
		while (true) {																			\
			do {																				\
				cmd = *src_base++;																\
				if (cmd == 0xff)																\
					goto next_line;																\
				hmin += cmd;																	\
			} while (cmd == 0xfe);																\
			hmax = hmin;																		\
			do {																				\
				cmd = *src_base++;																\
				hmax += cmd;																	\
			} while (cmd == 0xfe);																\
			if (hmin > src.right) {																\
				do src_base++; while (src_base[-1] != 0xff);									\
				goto next_line;																	\
			}																					\
			if (hmin >= src.left)																\
				imin = hmin;																	\
			else																				\
				imin = src.left;																\
			if (hmax <= src.right)																\
				imax = hmax;																	\
			else																				\
				imax = src.right+1;																\
			for (i=imin; i<imax; i++) {															\
				theColor = dst_ptr[i];															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha0) +										\
						color0) >> 8) & 0x00FF00FF) |											\
					(((((colorIn & 0x0000FF00) * alpha0) +										\
						color8) >> 8) & 0x0000FF00) ;											\
				dst_ptr[i] = ComponentsToPixel(colorIn>>8,colorIn,colorIn<<8);					\
			};																					\
			hmin = hmax;																		\
		}																						\
		next_line:																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);												\
	}																							\
}																								\
																								\
static void AlphaPreprocessName (																\
	RenderContext *	context,																	\
	RenderSubPort *	port,																		\
	fc_string *		str)																		\
{																								\
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;										\
	uint32																						\
		*colors = port->cache->fontColors,														\
		*alphas = port->cache->fontColors+16;													\
	int8 endianess = port->canvas->pixels.endianess;											\
	rgb_color fc = context->foreColor;															\
																								\
	if (fc.alpha == 0) {																		\
		port->cache->fDrawString = noop;														\
		return;																					\
	};																							\
																								\
	if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {							\
		alpha = fc.alpha;																		\
		alpha += (alpha >> 7);																	\
		da = (fc.alpha * alpha);																\
		dr = (fc.red * alpha);																	\
		dg = (fc.green * alpha);																\
		db = (fc.blue * alpha);																	\
		colors[0] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[8] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256-alpha;																	\
		port->cache->fontColors[24] = (uint32)AlphaDrawChar;									\
	} else {																					\
		da = alpha = fc.alpha;																	\
		alpha += (alpha >> 7);																	\
		colors[0] = 0L;																			\
		colors[7] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[15] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256;																		\
		alphas[7] = 256-alpha;																	\
																								\
		da = (da<<8)/7;																			\
		a0 = 0x80;																				\
																								\
		for (i=1; i<7; i++) {																	\
			a0 += da;																			\
			alpha = (a0+0x80) >> 8;																\
			alpha += (alpha >> 7);																\
			colors[i] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;					\
			colors[i+8] = (((uint32)fc.green)<<8) * alpha;										\
			alphas[i] = 256-alpha;																\
		}																						\
		port->cache->fontColors[24] = (uint32)AlphaDrawCharAA;									\
	};																							\
	port->cache->fDrawString = AlphaDrawString;													\
	AlphaDrawString(context,port,str);															\
};

#define PrecalcColorsFunc(PrecalcColorsFuncName)										\
static void PrecalcColorsFuncName (rgb_color fc, rgb_color bc, uint32 *colors)			\
{																						\
	int32 a0, r0, g0, b0, da, dr, dg, db, i;											\
																						\
	colors[0] = ComponentsToPixel((bc.red<<8),(bc.green<<8),(bc.blue<<8));				\
	colors[7] = ComponentsToPixel((fc.red<<8),(fc.green<<8),(fc.blue<<8));				\
	r0 = ((uint32)bc.red)<<8;															\
	g0 = ((uint32)bc.green)<<8;															\
	b0 = ((uint32)bc.blue)<<8;															\
	dr = ((fc.red<<8)-r0)>>3;															\
	dg = ((fc.green<<8)-g0)>>3;															\
	db = ((fc.blue<<8)-b0)>>3;															\
	r0 += 0x80;																			\
	g0 += 0x80;																			\
	b0 += 0x80;																			\
	for (i=1; i<7; i++) {																\
		r0 += dr; g0 += dg; b0 += db;													\
		colors[i] = ComponentsToPixel(r0,g0,b0);										\
	};																					\
	for (i=8; i<15; i++) colors[i] = colors[7];											\
};

#define DrawOverGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawAlphaGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawInvertGrayChar1516(FuncName)												\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,otherColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = 0xFFFF - *dst_ptr;											\
			else if (gray != 0) {														\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				otherColor = CanvasTo898(colorIn) * (8-gray);							\
				colorIn = 0xFFFF - colorIn;												\
				colorIn = (CanvasTo898(colorIn) * gray) + otherColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawBlendGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				colorIn = ((colorIn*(16-gray)) + theColor)>>1;							\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawOtherGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray,energy;																\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	bool gr;																			\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray = 8-(gray+(gray>>2));												\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				switch (mode) {															\
					case OP_SUB:														\
						theColor = (theColor|(0x8040100<<3)) - colorIn*gray;			\
						theColor &= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor &= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_ADD:														\
						theColor = theColor + colorIn*gray;								\
						theColor |= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor |= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_MIN:														\
					case OP_MAX:														\
						theColor = (colorIn*gray + theColor);							\
						gr =(															\
							 ((colorIn & (0x7F80000<<3))>>18) +							\
							 ((colorIn & (0x003FE00<<3))>> 9) +							\
							 ((colorIn & (0x00000FF<<3))<< 1)							\
							) >															\
							(															\
							 ((theColor & (0x7F80000<<3))>>18) +						\
							 ((theColor & (0x003FE00<<3))>> 9) +						\
							 ((theColor & (0x00000FF<<3))<< 1)							\
							);															\
						if ((gr && (mode==OP_MAX)) ||									\
					       (!gr && (mode==OP_MIN))) {									\
							dst_ptr++;													\
							continue;													\
						};																\
				};																		\
				*dst_ptr = BackToCanvas(theColor);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#if ROTATE_DISPLAY
#define DrawOverGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawAlphaGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawInvertGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,otherColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = 0xFFFF - *dst_ptr;											\
			else if (gray != 0) {														\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				otherColor = CanvasTo898(colorIn) * (8-gray);							\
				colorIn = 0xFFFF - colorIn;												\
				colorIn = (CanvasTo898(colorIn) * gray) + otherColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawBlendGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				colorIn = ((colorIn*(16-gray)) + theColor)>>1;							\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawOtherGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray,energy;																\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	bool gr;																			\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray = 8-(gray+(gray>>2));												\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				switch (mode) {															\
					case OP_SUB:														\
						theColor = (theColor|(0x8040100<<3)) - colorIn*gray;			\
						theColor &= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor &= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_ADD:														\
						theColor = theColor + colorIn*gray;								\
						theColor |= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor |= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_MIN:														\
					case OP_MAX:														\
						theColor = (colorIn*gray + theColor);							\
						gr =(															\
							 ((colorIn & (0x7F80000<<3))>>18) +							\
							 ((colorIn & (0x003FE00<<3))>> 9) +							\
							 ((colorIn & (0x00000FF<<3))<< 1)							\
							) >															\
							(															\
							 ((theColor & (0x7F80000<<3))>>18) +						\
							 ((theColor & (0x003FE00<<3))>> 9) +						\
							 ((theColor & (0x00000FF<<3))<< 1)							\
							);															\
						if ((gr && (mode==OP_MAX)) ||									\
					       (!gr && (mode==OP_MIN))) {									\
							dst_ptr++;													\
							continue;													\
						};																\
				};																		\
				*dst_ptr = BackToCanvas(theColor);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

// --------------------------------------------------------
// Rotated Alpha text functions
// --------------------------------------------------------

#define AlphaPreprocessRotatedFuncs(AlphaPreprocessName,AlphaDrawString,AlphaDrawChar,AlphaDrawCharAA)	\
static void AlphaDrawCharAA (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int32 i,j,gray;																				\
	uint32 colorIn,theColor;																	\
	uint16 *dst_ptr;																			\
	uint8 *src_ptr;																				\
	uint16 *dst_rowBase;																		\
	src_ptr = src_base+src_row*src.top;															\
	dst_rowBase = (uint16 *)(dst_base+dst_row*dst.top)+dst.right;								\
	for (j=src.top; j<=src.bottom; j++) {														\
		dst_ptr = dst_rowBase;																	\
		for (i=src.left; i<=src.right; i++) {													\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;											\
			if (gray != 0) {																	\
				theColor = *dst_ptr;															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha[gray]) +									\
						colors[gray  ]) >> 8) & 0x00FF00FF) |									\
					(((((colorIn & 0x0000FF00) * alpha[gray]) +									\
						colors[gray+8]) >> 8) & 0x0000FF00) ;									\
				*dst_ptr = ComponentsToPixel(colorIn>>8,colorIn,colorIn<<8);					\
			};																					\
			dst_ptr += dst_row/2;																\
		};																						\
		src_ptr += src_row;																		\
		dst_rowBase-=1;																			\
	};																							\
};																								\
																								\
static void AlphaDrawChar	 (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
}																								\
																								\
static void AlphaPreprocessName (																\
	RenderContext *	context,																	\
	RenderSubPort *	port,																		\
	fc_string *		str)																		\
{																								\
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;										\
	uint32																						\
		*colors = port->cache->fontColors,														\
		*alphas = port->cache->fontColors+16;													\
	int8 endianess = port->canvas->pixels.endianess;											\
	rgb_color fc = context->foreColor;															\
																								\
	if (fc.alpha == 0) {																		\
		port->cache->fDrawString = noop;														\
		return;																					\
	};																							\
																								\
	if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {							\
		alpha = fc.alpha;																		\
		alpha += (alpha >> 7);																	\
		da = (fc.alpha * alpha);																\
		dr = (fc.red * alpha);																	\
		dg = (fc.green * alpha);																\
		db = (fc.blue * alpha);																	\
		colors[0] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[8] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256-alpha;																	\
		port->cache->fontColors[24] = (uint32)AlphaDrawChar;									\
	} else {																					\
		da = alpha = fc.alpha;																	\
		alpha += (alpha >> 7);																	\
		colors[0] = 0L;																			\
		colors[7] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[15] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256;																		\
		alphas[7] = 256-alpha;																	\
																								\
		da = (da<<8)/7;																			\
		a0 = 0x80;																				\
																								\
		for (i=1; i<7; i++) {																	\
			a0 += da;																			\
			alpha = (a0+0x80) >> 8;																\
			alpha += (alpha >> 7);																\
			colors[i] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;					\
			colors[i+8] = (((uint32)fc.green)<<8) * alpha;										\
			alphas[i] = 256-alpha;																\
		}																						\
		port->cache->fontColors[24] = (uint32)AlphaDrawCharAA;									\
	};																							\
	port->cache->fDrawString = AlphaDrawString;													\
	AlphaDrawString(context,port,str);															\
};


#endif



static void AlphaDrawString(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	AlphaDrawChar drawChar = (AlphaDrawChar)colors[24];
	
	grAssertLocked(context, port);

	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
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
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors+16);
		}		
	}
}

#if ROTATE_DISPLAY

static void AlphaDrawRotatedString(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	region *	clip = port->drawingClip;
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	AlphaDrawChar drawChar = (AlphaDrawChar)colors[24];

	grAssertLocked(context, port);

	DisplayRotater	*rotater = port->rotater;

	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
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
			rotater->RotateRect(&cur, &cur);
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors+16);
		}		
	}
}

#endif

// Native endianess 15-bit
#define SourceFormatBits		15
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x7C00)<<9) | ((canvas&0x03E0)<<5) | (canvas&0x001F)))
#define BackToCanvas(col) \
	(0x8000 | ((col>>12)&0x7C00) | ((col>>8)&0x03E0) | ((col>>3)&0x001F))
#define ComponentsToPixel(r,g,b) (0x8000 | ((r>>1)&0x7C00) | ((g>>6)&0x03E0) | ((b>>11)&0x001F))
AlphaPreprocessFuncs(AlphaNative15Preprocess,AlphaDrawString,AlphaNative15DrawChar,AlphaNative15DrawCharAA)
PrecalcColorsFunc(ColorsNative15)
DrawOverGrayChar1516(DrawOverGrayChar15Native)
DrawInvertGrayChar1516(DrawInvertGrayChar15Native)
DrawBlendGrayChar1516(DrawBlendGrayChar15Native)
DrawOtherGrayChar1516(DrawOtherGrayChar15Native)
DrawOtherBWChar1516(DrawOtherBWChar15Native)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Native endianess 16-bit
#define SourceFormatBits		16
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0xF800)<<8) | ((canvas&0x07E0)<<4) | (canvas&0x001F)))
#define BackToCanvas(col) \
	(((col>>11)&0xF800) | ((col>>7)&0x07E0) | ((col>>3)&0x001F))
#define ComponentsToPixel(r,g,b) ((r&0xF800) | ((g>>5)&0x07E0) | ((b>>11)&0x001F))
AlphaPreprocessFuncs(AlphaNative16Preprocess,AlphaDrawString,AlphaNative16DrawChar,AlphaNative16DrawCharAA)
PrecalcColorsFunc(ColorsNative16)
DrawOverGrayChar1516(DrawOverGrayChar16Native)
DrawInvertGrayChar1516(DrawInvertGrayChar16Native)
DrawBlendGrayChar1516(DrawBlendGrayChar16Native)
DrawOtherGrayChar1516(DrawOtherGrayChar16Native)
DrawOtherBWChar1516(DrawOtherBWChar16Native)
#if ROTATE_DISPLAY
AlphaPreprocessRotatedFuncs(AlphaNative16PreprocessRotated,AlphaDrawRotatedString,AlphaNative16DrawRotatedChar,AlphaNative16DrawRotatedCharAA)
DrawOverGrayRotatedChar1516(DrawOverGrayRotatedChar16)
DrawInvertGrayRotatedChar1516(DrawInvertGrayRotatedChar16)
DrawBlendGrayRotatedChar1516(DrawBlendGrayRotatedChar16)
DrawOtherGrayRotatedChar1516(DrawOtherGrayRotatedChar16)
#endif
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Opposite endianess 15-bit
#define SourceFormatBits		15
#define SourceFormatEndianess	NonHostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x007C)<<17) | ((canvas&0x0003)<<13) | ((canvas&0xE000)>>3) | ((canvas&0x1F00)>>8)))
#define BackToCanvas(col) \
	(0x0080 | ((col>>20)&0x007C) | ((col>>16)&0x0003) | (col&0xE000) | ((col<<5)&0x1F00))
#define ComponentsToPixel(r,g,b) (0x0080 | ((r>>9)&0x007C) | ((g>>14)&0x0003) | ((g<<2)&0xE000) | ((b>>3)&0x1F00))
AlphaPreprocessFuncs(AlphaOpp15Preprocess,AlphaDrawString,AlphaOpp15DrawChar,AlphaOpp15DrawCharAA)
PrecalcColorsFunc(ColorsOpp15)
DrawOverGrayChar1516(DrawOverGrayChar15Opp)
DrawInvertGrayChar1516(DrawInvertGrayChar15Opp)
DrawBlendGrayChar1516(DrawBlendGrayChar15Opp)
DrawOtherGrayChar1516(DrawOtherGrayChar15Opp)
DrawOtherBWChar1516(DrawOtherBWChar15Opp)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Opposite endianess 16-bit
#define SourceFormatBits		16
#define SourceFormatEndianess	NonHostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x00F8)<<16) | ((canvas&0x0007)<<12) | ((canvas&0xE000)>>4) | ((canvas&0x1F00)>>8)))
#define BackToCanvas(col) \
	(((col>>19)&0x00F8) | ((col>>15)&0x0007) | ((col<<1)&0xE000) | ((col<<5)&0x1F00))
#define ComponentsToPixel(r,g,b) (((r>>8)&0x00F8) | ((g>>13)&0x0007) | ((g<<3)&0xE000) | ((b>>3)&0x1F00))
AlphaPreprocessFuncs(AlphaOpp16Preprocess,AlphaDrawString,AlphaOpp16DrawChar,AlphaOpp16DrawCharAA)
PrecalcColorsFunc(ColorsOpp16)
DrawOverGrayChar1516(DrawOverGrayChar16Opp)
DrawInvertGrayChar1516(DrawInvertGrayChar16Opp)
DrawBlendGrayChar1516(DrawBlendGrayChar16Opp)
DrawOtherGrayChar1516(DrawOtherGrayChar16Opp)
DrawOtherBWChar1516(DrawOtherBWChar16Opp)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

Precalc precalcTable[2][2] =
{
	{ ColorsNative15, ColorsNative16 },
	{ ColorsOpp15, ColorsOpp16 }
};

DrawOtherBWChar drawOtherBWChar[2][2] =
{
	{ DrawOtherBWChar15Native, DrawOtherBWChar16Native },
	{ DrawOtherBWChar15Opp, DrawOtherBWChar16Opp }
};

void * drawOtherChar[11][2][2] =
{
	{	{ NULL,NULL }, { NULL,NULL }  },
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ DrawInvertGrayChar15Native, DrawInvertGrayChar16Native },
		{ DrawInvertGrayChar15Opp, DrawInvertGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawBlendGrayChar15Native, DrawBlendGrayChar16Native },
		{ DrawBlendGrayChar15Opp, DrawBlendGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ NULL, NULL },
		{ NULL, NULL }
	}
};

#if ROTATE_DISPLAY
void *drawOtherRotatedChar[11] = {
	NULL,
	DrawOverGrayRotatedChar16,
	DrawOverGrayRotatedChar16,
	DrawInvertGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawBlendGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOverGrayRotatedChar16,
	NULL
};
#endif

/* main entry to draw black&white string into an 32 bits port, both endianess */
static void DrawStringNonAAOther(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int			i, a, r, g, b, index, energy0;
	int			cur_row;
	int			mode_direct, endianess;
	rect		cur, src, clipbox;
	uint32		pixel;
	uchar *		src_base, *cur_base;
	fc_char *	my_char;
	fc_point *	my_offset;
	int			mode,k=0;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	DrawOtherBWChar drawChar = NULL;

	grAssertLocked(context, port);

#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		return;
	/* PIERRE TO BE CONTINUED */
#endif
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;

	if (mode == OP_ERASE)
		pixel = port->cache->backColorCanvasFormat;
	else
		pixel = port->cache->foreColorCanvasFormat;

	if ((mode <= OP_ERASE) || (mode == OP_SELECT))
		drawChar = DrawCopyBWChar1516;
	else
		drawChar = drawOtherBWChar[endianess!=HOST_ENDIANESS][port->canvas->pixels.pixelFormat-PIX_15BIT];

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
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
			drawChar(cur_base, cur_row, cur, src_base, src, pixel, mode);
		}
	}
}

static void DrawGrayChar1516(	uchar *dst_base, int dst_row, rect dst,
								uchar *src_base, int src_row, rect src,
								uint32 *colors) {
	int		i, j;
	uint32	gray;
	uint16	*dst_ptr;
	uchar	*src_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint16*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		if (i&1) {
			if (((uint32)dst_ptr)&0x03) {
				gray = src_ptr[i>>1]&7;
				if (gray) dst_ptr[i] = colors[gray];
				i++;
				goto aligned;
			};
			gray = src_ptr[i>>1];
			unaligned:;
			while (i<=src.right-1) {
				gray = ((gray<<8)|src_ptr[(i+1)>>1]) & 0x777;
				if (gray & 0x770) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>8]) | ShiftToWord1(colors[(gray>>4)&7]);
				i+=2;
			};
			if (i == src.right) {
				gray &= 7;
				if (gray) dst_ptr[i] = colors[gray];
			}
		} else {
			if (((uint32)dst_ptr)&0x03) {
				gray = src_ptr[i>>1];
				if (gray>>4) dst_ptr[i] = colors[gray>>4];
				i++;
				goto unaligned;
			};
			aligned:
			while (i<src.right-2) {
				gray = src_ptr[i>>1];
				if (gray) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				gray = src_ptr[(i>>1)+1];
				if (gray) *((uint32*)(dst_ptr+i+2)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				i+=4;
			}
			while (i<=src.right-1) {
				gray = src_ptr[i>>1];
				if (gray) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				i+=2;
			};
			if (i == src.right) {
				gray = src_ptr[i>>1]>>4;
				if (gray > 0) dst_ptr[i] = colors[gray];
			}
		};
		src_ptr += src_row;
		dst_ptr = (uint16*)((uchar*)dst_ptr+dst_row);
	}
}

static void DrawGrayCharX1516(	uchar *dst_base, int dst_row, rect dst,
								uchar *src2_base, int src2_row, rect src2,
								uchar *src_base, int src_row, rect src,
								uint32 *colors) {
	int        i, i2, j, gray;
	uint16     *dst_ptr;
	uchar      *src_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr = (uint16*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		i2 = src2.left;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) dst_ptr[i] = colors[gray];
			i++; i2++;
		}
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			gray += (((src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7)<<4) |
					 ((src2_ptr[(i2+1)>>1]>>(4-((i2<<2)&4)))&7);
			if (gray > 0) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
			i+=2; i2+=2;
		}
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) dst_ptr[i] = colors[gray];
		}
		src_ptr += src_row;
		src2_ptr += src2_row;
		dst_ptr = (uint16*)((uchar*)dst_ptr+dst_row);
	}
}

static void DrawStringAACopy(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = port->cache->fontColors;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	
	grAssertLocked(context, port);
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	prev.top = -1048576;
	prev.left = -1048576;
	prev.right = -1048576;
	prev.bottom = -1048576;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
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

			/* test character not-overlaping */
			if ((prev.bottom < cur.top) ||
				(prev.right < cur.left) ||
				(prev.left > cur.right) ||
				(prev.top > cur.bottom)) {
				DrawGrayChar1516(	cur_base, cur_row, cur,
									src_base, src_row, src, colors);
			/* characters overlaping */
			} else {
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
					DrawGrayChar1516(	cur_base, cur_row, tmp,
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
					DrawGrayChar1516(	cur_base, cur_row, tmp,
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
					DrawGrayChar1516(	cur_base, cur_row, tmp,
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
					DrawGrayChar1516(	cur_base, cur_row, tmp,
										src_base, src_row, src_tmp, colors);
				}
				/* at last the horrible overlaping band... */
				src_tmp.top = src.top + (inter.top-cur.top);
				src_tmp.left = src.left + (inter.left-cur.left);
				src_tmp.right = src.right + (inter.right-cur.right);
				src_tmp.bottom = src.bottom + (inter.bottom-cur.bottom);
				src_prev_tmp.top = src_prev.top + (inter.top-prev.top);
				src_prev_tmp.left = src_prev.left + (inter.left-prev.left);
				src_prev_tmp.right = src_prev.right + (inter.right-prev.right);
				src_prev_tmp.bottom = src_prev.bottom + (inter.bottom-prev.bottom);
				DrawGrayCharX1516(	cur_base, cur_row, inter,
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

static void DrawStringAAOther(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	RenderCache *	cache = port->cache;
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	DrawOtherChar drawChar = (DrawOtherChar)colors[15];
	
	grAssertLocked(context, port);
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
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
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors[9], mode);
		}		
	}
}

#if ROTATE_DISPLAY
static void DrawRotatedGrayChar1516(uchar *dst_base, int dst_row, rect dst,
									uchar *src_base, int src_row, rect src,
									uint32 *colors) {
	int		i, j;
	uint32	gray;
	uint16	*dst_ptr, *dst_ptr2;
	uchar	*src_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		dst_ptr = dst_ptr2;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			if (gray) *dst_ptr = colors[gray];
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);	// add one row
			i++;
		};
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			if (gray) {
				*dst_ptr = colors[gray>>4];
				dst_ptr[dst_row/2] = colors[gray&7];
			}
			dst_ptr += dst_row;		// this add two rows (uint16*)
			i+=2;
		};
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			if (gray) *dst_ptr = colors[gray];
		}
		src_ptr += src_row;
		dst_ptr2 -= 1;
	}
}

static void DrawRotatedGrayCharX1516(	uchar *dst_base, int dst_row, rect dst,
										uchar *src2_base, int src2_row, rect src2,
										uchar *src_base, int src_row, rect src,
										uint32 *colors) {
	int        i, i2, j, gray;
	uint16     *dst_ptr, *dst_ptr2;
	uchar      *src_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		i2 = src2.left;
		dst_ptr = dst_ptr2;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) *dst_ptr = colors[gray];
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);	// add one row
			i++; i2++;
		}
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			gray += (((src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7)<<4) |
					 ((src2_ptr[(i2+1)>>1]>>(4-(((i2+1)<<2)&4)))&7);
			if (gray > 0) {
				*dst_ptr = colors[gray>>4];
				dst_ptr[dst_row/2] = colors[gray&7];
			}
			dst_ptr += dst_row;		// this add two rows (uint16*)
			i+=2; i2+=2;
		}
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) *dst_ptr = colors[gray];
		}
		src_ptr += src_row;
		src2_ptr += src2_row;
		dst_ptr2 -= 1;
	}
}

static void DrawRotatedStringAACopy(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	RenderCache *	cache = port->cache;
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	region *	clip = port->drawingClip;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode;
	DisplayRotater	*rotater = port->rotater;

	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	prev.top = -1048576;
	prev.left = -1048576;
	prev.right = -1048576;
	prev.bottom = -1048576;

	for (int32 k=0; k<clip->CountRects(); k++) {
		clipbox = clip->Rects()[k];
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;

			/* calculate drawing bounding box */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;			
			/* calculate source bounding box */
			src.left = 0;
			src.right = my_char->bbox.right - my_char->bbox.left;
			src.top = 0;
			src.bottom = my_char->bbox.bottom - my_char->bbox.top;
			src_row = (src.right+2)>>1;
			if (src.right < 0) continue;
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
			rotater->RotateRect(&cur, &cur);
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;

			/* test character not-overlaping */
			if ((prev.bottom < cur.top) ||
				(prev.right < cur.left) ||
				(prev.left > cur.right) ||
				(prev.top > cur.bottom))
			{
				DrawRotatedGrayChar1516(cur_base, cur_row, cur,
										src_base, src_row, src, colors);
			} else { /* characters overlaping */
				/* calculate intersection of the 2 rect */
				if (prev.left > cur.left)		inter.left = prev.left;
				else							inter.left = cur.left;
				if (prev.right < cur.right)		inter.right = prev.right;
				else							inter.right = cur.right;
				if (prev.top > cur.top)			inter.top = prev.top;
				else							inter.top = cur.top;
				if (prev.bottom < cur.bottom)	inter.bottom = prev.bottom;
				else							inter.bottom = cur.bottom;
				/* left not overlaping band */	
				dh = inter.left-cur.left-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.left;
					tmp.right = cur.left+dh;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.bottom-dh;
					src_tmp.left = src.left+(inter.top-cur.top);
					src_tmp.right = src.right+(inter.bottom-cur.bottom);
					src_tmp.bottom = src.bottom;

					DrawRotatedGrayChar1516(cur_base, cur_row, tmp,
											src_base, src_row, src_tmp, colors);
				}
				/* right not overlaping band */	
				dh = cur.right-inter.right-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.right-dh;
					tmp.right = cur.right;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top;
					src_tmp.left = src.left+(inter.top-cur.top);
					src_tmp.right = src.right+(inter.bottom-cur.bottom);
					src_tmp.bottom = src.top+dh;
					DrawRotatedGrayChar1516(cur_base, cur_row, tmp,
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
					src_tmp.right = src.left+dv;
					src_tmp.bottom = src.bottom;
					DrawRotatedGrayChar1516(cur_base, cur_row, tmp,
											src_base, src_row, src_tmp, colors);
				}
				/* bottom not overlaping band */	
				dv = cur.bottom-inter.bottom-1;
				if (dv >= 0) {
					tmp.top = cur.bottom-dv;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.bottom;
					src_tmp.top = src.top;
					src_tmp.left = src.right-dv;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom;
					DrawRotatedGrayChar1516(cur_base, cur_row, tmp,
											src_base, src_row, src_tmp, colors);
				}
				/* at last the horrible overlaping band... */
				src_tmp.top			= src.top			+	(cur.right		- inter.right);
				src_tmp.left		= src.left			+	(inter.top		- cur.top);
				src_tmp.right		= src.right			-	(cur.bottom		- inter.bottom);
				src_tmp.bottom		= src.bottom		-	(inter.left		- cur.left);
				src_prev_tmp.top	= src_prev.top		+	(prev.right		- inter.right);
				src_prev_tmp.left	= src_prev.left		+	(inter.top		- prev.top);
				src_prev_tmp.right	= src_prev.right	-	(prev.bottom	- inter.bottom);
				src_prev_tmp.bottom	= src_prev.bottom	-	(inter.left		- prev.left);
				DrawRotatedGrayCharX1516(	cur_base, cur_row, inter,
											src_prev_base, src_prev_row, src_prev_tmp,
											src_base, src_row, src_tmp, colors);
			}
			src_prev_row = src_row;
			src_prev_base = src_base;
			src_prev = src;
			prev = cur;
		}
	}
}

static void DrawRotatedStringAAOther(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = port->cache->fontColors;
	region *	clip = port->drawingClip;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode;
	DrawOtherChar drawChar = (DrawOtherChar)colors[15];
	DisplayRotater	*rotater = port->rotater;
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	for (int32 k=0; k<clip->CountRects(); k++) {
		clipbox = clip->Rects()[k];
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate drawing bounding box */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;			
			/* calculate source bounding box */
			src.left = 0;
			src.right = my_char->bbox.right - my_char->bbox.left;
			src.top = 0;
			src.bottom = my_char->bbox.bottom - my_char->bbox.top;
			src_row = (src.right+2)>>1;
			if (src.right < 0) continue;
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
			rotater->RotateRect(&cur, &cur);
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors[9], mode);
		}		
	}
}

#endif

static void DrawStringAAPreprocess(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp;
	uint32 *	colors = port->cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode;
	
	grAssertLocked(context, port);
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if (mode == OP_COPY) {
		precalcTable
			[endianess!=HOST_ENDIANESS]
			[port->canvas->pixels.pixelFormat-PIX_15BIT](fc,bc,colors);
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			port->cache->fDrawString = DrawRotatedStringAACopy;
		else
#endif
			port->cache->fDrawString = DrawStringAACopy;
	} else {
		uint32 pixel = port->cache->foreColorCanvasFormat;
		int32 r0, g0, b0, dr, dg, db;

		if (mode == OP_ERASE) {
			fc = bc;
			pixel = port->cache->backColorCanvasFormat;
		};

		dr = fc.red; dg = fc.green; db = fc.blue;
		colors[0] = 0;
		colors[7] = (dr<<19) | (dg<<10) | ((dg&0x80)<<2) | db;
		colors[9] = pixel;
		dr <<= 5; dg <<= 5; db <<= 5;
		r0 = 0x80; g0 = 0x80; b0 = 0x80;
		
		/* min & max mode : store the good color and its energy */
		for (i=1; i<7; i++) {
			r0 += dr; g0 += dg; b0 += db;
			colors[i] = ((r0&0xFF00)<<11) | ((g0&0xFF80)<<2) | (b0>>8);
		}
		
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
		/* This assume native endianess and 16bpp, as non of the other rotated
		   handles anything else in anycase */
			port->cache->fontColors[15] = (int32)drawOtherRotatedChar[mode];
		else
#endif
		port->cache->fontColors[15] =
			(int32)drawOtherChar
			[mode][endianess!=HOST_ENDIANESS]
			[port->canvas->pixels.pixelFormat-PIX_15BIT];

#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			port->cache->fDrawString = DrawRotatedStringAAOther;
		else
#endif
		port->cache->fDrawString = DrawStringAAOther;
	}
	port->cache->fDrawString(context,port,str);
}

extern BlitFunction BlitCopySameFormat;

static BlitFunction copyBlitTable15Little[] = {
	BlitCopy1ToLittle15,
	BlitCopy1ToLittle15,
	BlitCopy8ToLittle15,
	BlitCopy8ToLittle15,
	BlitCopySameFormat,
	BlitCopyBig15ToLittle15,
	BlitCopyLittle16ToLittle15,
	BlitCopyBig16ToLittle15,
	BlitCopyLittle32ToLittle15,
	BlitCopyBig32ToLittle15
};

static BlitFunction copyBlitTable15Big[] = {
	BlitCopy1ToBig15,
	BlitCopy1ToBig15,
	BlitCopy8ToBig15,
	BlitCopy8ToBig15,
	BlitCopyLittle15ToBig15,
	BlitCopySameFormat,
	BlitCopyLittle16ToBig15,
	BlitCopyBig16ToBig15,
	BlitCopyLittle32ToBig15,
	BlitCopyBig32ToBig15
};

static BlitFunction overBlitTable15Little[] = {
	BlitOver1ToLittle15,
	BlitOver1ToLittle15,
	BlitOver8ToLittle15,
	BlitOver8ToLittle15,
	BlitOverLittle15ToLittle15,
	BlitOverBig15ToLittle15,
	BlitCopyLittle16ToLittle15,
	BlitCopyBig16ToLittle15,
	BlitOverLittle32ToLittle15,
	BlitOverBig32ToLittle15
};

static BlitFunction overBlitTable15Big[] = {
	BlitOver1ToBig15,
	BlitOver1ToBig15,
	BlitOver8ToBig15,
	BlitOver8ToBig15,
	BlitOverLittle15ToBig15,
	BlitOverBig15ToBig15,
	BlitCopyLittle16ToBig15,
	BlitCopyBig16ToBig15,
	BlitOverLittle32ToBig15,
	BlitOverBig32ToBig15
};

static BlitFunction eraseBlitTable15Little[] = {
	BlitOver1ToLittle15,
	BlitOver1ToLittle15,
	BlitScaleFunction8ToLittle15,
	BlitScaleFunction8ToLittle15,
	BlitScaleFunctionLittle15ToLittle15,
	BlitScaleFunctionBig15ToLittle15,
	BlitScaleFunctionLittle16ToLittle15,
	BlitScaleFunctionBig16ToLittle15,
	BlitScaleFunctionLittle32ToLittle15,
	BlitScaleFunctionBig32ToLittle15
};

static BlitFunction eraseBlitTable15Big[] = {
	BlitOver1ToBig15,
	BlitOver1ToBig15,
	BlitScaleFunction8ToBig15,
	BlitScaleFunction8ToBig15,
	BlitScaleFunctionLittle15ToBig15,
	BlitScaleFunctionBig15ToBig15,
	BlitScaleFunctionLittle16ToBig15,
	BlitScaleFunctionBig16ToBig15,
	BlitScaleFunctionLittle32ToBig15,
	BlitScaleFunctionBig32ToBig15
};

static BlitFunction alphaBlitTable15Little[] = {
	BlitOver1ToLittle15,
	BlitOver1ToLittle15,
	BlitOver8ToLittle15,
	BlitOver8ToLittle15,
	BlitAlphaLittle15ToLittle15,
	BlitAlphaBig15ToLittle15,
	BlitCopyLittle16ToLittle15,
	BlitCopyBig16ToLittle15,
	BlitAlphaLittle32ToLittle15,
	BlitAlphaBig32ToLittle15
};

static BlitFunction alphaBlitTable15Big[] = {
	BlitOver1ToBig15,
	BlitOver1ToBig15,
	BlitOver8ToBig15,
	BlitOver8ToBig15,
	BlitAlphaLittle15ToBig15,
	BlitAlphaBig15ToBig15,
	BlitCopyLittle16ToBig15,
	BlitCopyBig16ToBig15,
	BlitAlphaLittle32ToBig15,
	BlitAlphaBig32ToBig15
};

static BlitFunction scaleCopyBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToLittle15,
	BlitScaleCopy8ToLittle15,
	BlitScaleCopyLittle15ToLittle15,
	BlitScaleCopyBig15ToLittle15,
	BlitScaleCopyLittle16ToLittle15,
	BlitScaleCopyBig16ToLittle15,
	BlitScaleCopyLittle32ToLittle15,
	BlitScaleCopyBig32ToLittle15
};

static BlitFunction scaleCopyBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToBig15,
	BlitScaleCopy8ToBig15,
	BlitScaleCopyLittle15ToBig15,
	BlitScaleCopyBig15ToBig15,
	BlitScaleCopyLittle16ToBig15,
	BlitScaleCopyBig16ToBig15,
	BlitScaleCopyLittle32ToBig15,
	BlitScaleCopyBig32ToBig15
};

static BlitFunction scaleOverBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle15,
	BlitScaleOver8ToLittle15,
	BlitScaleOverLittle15ToLittle15,
	BlitScaleOverBig15ToLittle15,
	BlitScaleCopyLittle16ToLittle15,
	BlitScaleCopyBig16ToLittle15,
	BlitScaleOverLittle32ToLittle15,
	BlitScaleOverBig32ToLittle15
};

static BlitFunction scaleOverBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig15,
	BlitScaleOver8ToBig15,
	BlitScaleOverLittle15ToBig15,
	BlitScaleOverBig15ToBig15,
	BlitScaleCopyLittle16ToBig15,
	BlitScaleCopyBig16ToBig15,
	BlitScaleOverLittle32ToBig15,
	BlitScaleOverBig32ToBig15
};

static BlitFunction scaleAlphaBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle15,
	BlitScaleOver8ToLittle15,
	BlitScaleAlphaLittle15ToLittle15,
	BlitScaleAlphaBig15ToLittle15,
	BlitScaleCopyLittle16ToLittle15,
	BlitScaleCopyBig16ToLittle15,
	BlitScaleAlphaLittle32ToLittle15,
	BlitScaleAlphaBig32ToLittle15
};

static BlitFunction scaleAlphaBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig15,
	BlitScaleOver8ToBig15,
	BlitScaleAlphaLittle15ToBig15,
	BlitScaleAlphaBig15ToBig15,
	BlitScaleCopyLittle16ToBig15,
	BlitScaleCopyBig16ToBig15,
	BlitScaleAlphaLittle32ToBig15,
	BlitScaleAlphaBig32ToBig15
};

static BlitFunction scaleFunctionBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToLittle15,
	BlitScaleFunction8ToLittle15,
	BlitScaleFunctionLittle15ToLittle15,
	BlitScaleFunctionBig15ToLittle15,
	BlitScaleFunctionLittle16ToLittle15,
	BlitScaleFunctionBig16ToLittle15,
	BlitScaleFunctionLittle32ToLittle15,
	BlitScaleFunctionBig32ToLittle15
};

static BlitFunction scaleFunctionBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToBig15,
	BlitScaleFunction8ToBig15,
	BlitScaleFunctionLittle15ToBig15,
	BlitScaleFunctionBig15ToBig15,
	BlitScaleFunctionLittle16ToBig15,
	BlitScaleFunctionBig16ToBig15,
	BlitScaleFunctionLittle32ToBig15,
	BlitScaleFunctionBig32ToBig15
};

static BlitFunction copyBlitTable16Little[] = {
	BlitCopy1ToLittle16,
	BlitCopy1ToLittle16,
	BlitCopy8ToLittle16,
	BlitCopy8ToLittle16,
	BlitCopyLittle15ToLittle16,
	BlitCopyBig15ToLittle16,
	BlitCopySameFormat,
	BlitCopyBig16ToLittle16,
	BlitCopyLittle32ToLittle16,
	BlitCopyBig32ToLittle16
};

static BlitFunction copyBlitTable16Big[] = {
	BlitCopy1ToBig16,
	BlitCopy1ToBig16,
	BlitCopy8ToBig16,
	BlitCopy8ToBig16,
	BlitCopyLittle15ToBig16,
	BlitCopyBig15ToBig16,
	BlitCopyLittle16ToBig16,
	BlitCopySameFormat,
	BlitCopyLittle32ToBig16,
	BlitCopyBig32ToBig16
};

static BlitFunction overBlitTable16Little[] = {
	BlitOver1ToLittle16,
	BlitOver1ToLittle16,
	BlitOver8ToLittle16,
	BlitOver8ToLittle16,
	BlitOverLittle15ToLittle16,
	BlitOverBig15ToLittle16,
	BlitCopySameFormat,
	BlitCopyBig16ToLittle16,
	BlitOverLittle32ToLittle16,
	BlitOverBig32ToLittle16
};

static BlitFunction overBlitTable16Big[] = {
	BlitOver1ToBig16,
	BlitOver1ToBig16,
	BlitOver8ToBig16,
	BlitOver8ToBig16,
	BlitOverLittle15ToBig16,
	BlitOverBig15ToBig16,
	BlitCopyLittle16ToBig16,
	BlitCopySameFormat,
	BlitOverLittle32ToBig16,
	BlitOverBig32ToBig16
};

static BlitFunction eraseBlitTable16Little[] = {
	BlitOver1ToLittle16,
	BlitOver1ToLittle16,
	BlitScaleFunction8ToLittle16,
	BlitScaleFunction8ToLittle16,
	BlitScaleFunctionLittle15ToLittle16,
	BlitScaleFunctionBig15ToLittle16,
	BlitScaleFunctionLittle16ToLittle16,
	BlitScaleFunctionBig16ToLittle16,
	BlitScaleFunctionLittle32ToLittle16,
	BlitScaleFunctionBig32ToLittle16
};

static BlitFunction eraseBlitTable16Big[] = {
	BlitOver1ToBig16,
	BlitOver1ToBig16,
	BlitScaleFunction8ToBig16,
	BlitScaleFunction8ToBig16,
	BlitScaleFunctionLittle15ToBig16,
	BlitScaleFunctionBig15ToBig16,
	BlitScaleFunctionLittle16ToBig16,
	BlitScaleFunctionBig16ToBig16,
	BlitScaleFunctionLittle32ToBig16,
	BlitScaleFunctionBig32ToBig16
};

static BlitFunction alphaBlitTable16Little[] = {
	BlitOver1ToLittle16,
	BlitOver1ToLittle16,
	BlitOver8ToLittle16,
	BlitOver8ToLittle16,
	BlitAlphaLittle15ToLittle16,
	BlitAlphaBig15ToLittle16,
	BlitCopySameFormat,
	BlitCopyBig16ToLittle16,
	BlitAlphaLittle32ToLittle16,
	BlitAlphaBig32ToLittle16
};

static BlitFunction alphaBlitTable16Big[] = {
	BlitOver1ToBig16,
	BlitOver1ToBig16,
	BlitOver8ToBig16,
	BlitOver8ToBig16,
	BlitAlphaLittle15ToBig16,
	BlitAlphaBig15ToBig16,
	BlitCopyLittle16ToBig16,
	BlitCopySameFormat,
	BlitAlphaLittle32ToBig16,
	BlitAlphaBig32ToBig16
};

static BlitFunction scaleCopyBlitTable16Little[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToLittle16,
	BlitScaleCopy8ToLittle16,
	BlitScaleCopyLittle15ToLittle16,
	BlitScaleCopyBig15ToLittle16,
	BlitScaleCopyLittle16ToLittle16,
	BlitScaleCopyBig16ToLittle16,
	BlitScaleCopyLittle32ToLittle16,
	BlitScaleCopyBig32ToLittle16
};

static BlitFunction scaleCopyBlitTable16Big[] = {
	NULL,
	NULL,
	BlitScaleCopy8ToBig16,
	BlitScaleCopy8ToBig16,
	BlitScaleCopyLittle15ToBig16,
	BlitScaleCopyBig15ToBig16,
	BlitScaleCopyLittle16ToBig16,
	BlitScaleCopyBig16ToBig16,
	BlitScaleCopyLittle32ToBig16,
	BlitScaleCopyBig32ToBig16
};

static BlitFunction scaleOverBlitTable16Little[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle16,
	BlitScaleOver8ToLittle16,
	BlitScaleOverLittle15ToLittle16,
	BlitScaleOverBig15ToLittle16,
	BlitScaleCopyLittle16ToLittle16,
	BlitScaleCopyBig16ToLittle16,
	BlitScaleOverLittle32ToLittle16,
	BlitScaleOverBig32ToLittle16
};

static BlitFunction scaleOverBlitTable16Big[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig16,
	BlitScaleOver8ToBig16,
	BlitScaleOverLittle15ToBig16,
	BlitScaleOverBig15ToBig16,
	BlitScaleCopyLittle16ToBig16,
	BlitScaleCopyBig16ToBig16,
	BlitScaleOverLittle32ToBig16,
	BlitScaleOverBig32ToBig16
};

static BlitFunction scaleAlphaBlitTable16Little[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle16,
	BlitScaleOver8ToLittle16,
	BlitScaleAlphaLittle15ToLittle16,
	BlitScaleAlphaBig15ToLittle16,
	BlitScaleCopyLittle16ToLittle16,
	BlitScaleCopyBig16ToLittle16,
	BlitScaleAlphaLittle32ToLittle16,
	BlitScaleAlphaBig32ToLittle16
};

static BlitFunction scaleAlphaBlitTable16Big[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig16,
	BlitScaleOver8ToBig16,
	BlitScaleAlphaLittle15ToBig16,
	BlitScaleAlphaBig15ToBig16,
	BlitScaleCopyLittle16ToBig16,
	BlitScaleCopyBig16ToBig16,
	BlitScaleAlphaLittle32ToBig16,
	BlitScaleAlphaBig32ToBig16
};

static BlitFunction scaleFunctionBlitTable16Little[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToLittle16,
	BlitScaleFunction8ToLittle16,
	BlitScaleFunctionLittle15ToLittle16,
	BlitScaleFunctionBig15ToLittle16,
	BlitScaleFunctionLittle16ToLittle16,
	BlitScaleFunctionBig16ToLittle16,
	BlitScaleFunctionLittle32ToLittle16,
	BlitScaleFunctionBig32ToLittle16
};

static BlitFunction scaleFunctionBlitTable16Big[] = {
	NULL,
	NULL,
	BlitScaleFunction8ToBig16,
	BlitScaleFunction8ToBig16,
	BlitScaleFunctionLittle15ToBig16,
	BlitScaleFunctionBig15ToBig16,
	BlitScaleFunctionLittle16ToBig16,
	BlitScaleFunctionBig16ToBig16,
	BlitScaleFunctionLittle32ToBig16,
	BlitScaleFunctionBig32ToBig16
};

#if ROTATE_DISPLAY
static BlitFunction copyRotatedBlitTable16Little[] = {
	RotatedBlitCopy1ToLittle16,
	RotatedBlitCopy1ToLittle16,
	RotatedBlitCopy8ToLittle16,
	RotatedBlitCopy8ToLittle16,
	RotatedBlitCopyLittle15ToLittle16,
	RotatedBlitCopyBig15ToLittle16,
	RotatedBlitCopySameFormat16,
	RotatedBlitCopyBig16ToLittle16,
	RotatedBlitCopyLittle32ToLittle16,
	RotatedBlitCopyBig32ToLittle16
};

static BlitFunction overRotatedBlitTable16Little[] = {
	RotatedBlitOver1ToLittle16,
	RotatedBlitOver1ToLittle16,
	RotatedBlitOver8ToLittle16,
	RotatedBlitOver8ToLittle16,
	RotatedBlitOverLittle15ToLittle16,
	RotatedBlitOverBig15ToLittle16,
	RotatedBlitCopySameFormat16,
	RotatedBlitCopyBig16ToLittle16,
	RotatedBlitOverLittle32ToLittle16,
	RotatedBlitOverBig32ToLittle16
};

static BlitFunction eraseRotatedBlitTable16Little[] = {
	RotatedBlitOver1ToLittle16,
	RotatedBlitOver1ToLittle16,
	RotatedBlitScaleFunction8ToLittle16,
	RotatedBlitScaleFunction8ToLittle16,
	RotatedBlitScaleFunctionLittle15ToLittle16,
	RotatedBlitScaleFunctionBig15ToLittle16,
	RotatedBlitScaleFunctionLittle16ToLittle16,
	RotatedBlitScaleFunctionBig16ToLittle16,
	RotatedBlitScaleFunctionLittle32ToLittle16,
	RotatedBlitScaleFunctionBig32ToLittle16
};

static BlitFunction alphaRotatedBlitTable16Little[] = {
	RotatedBlitOver1ToLittle16,
	RotatedBlitOver1ToLittle16,
	RotatedBlitOver8ToLittle16,
	RotatedBlitOver8ToLittle16,
	RotatedBlitAlphaLittle15ToLittle16,
	RotatedBlitAlphaBig15ToLittle16,
	RotatedBlitCopySameFormat16,
	RotatedBlitCopyBig16ToLittle16,
	RotatedBlitAlphaLittle32ToLittle16,
	RotatedBlitAlphaBig32ToLittle16
};

static BlitFunction scaleCopyRotatedBlitTable16Little[] = {
	NULL,
	NULL,
	RotatedBlitScaleCopy8ToLittle16,
	RotatedBlitScaleCopy8ToLittle16,
	RotatedBlitScaleCopyLittle15ToLittle16,
	RotatedBlitScaleCopyBig15ToLittle16,
	RotatedBlitScaleCopyLittle16ToLittle16,
	RotatedBlitScaleCopyBig16ToLittle16,
	RotatedBlitScaleCopyLittle32ToLittle16,
	RotatedBlitScaleCopyBig32ToLittle16
};

static BlitFunction scaleOverRotatedBlitTable16Little[] = {
	NULL,
	NULL,
	RotatedBlitScaleOver8ToLittle16,
	RotatedBlitScaleOver8ToLittle16,
	RotatedBlitScaleOverLittle15ToLittle16,
	RotatedBlitScaleOverBig15ToLittle16,
	RotatedBlitScaleCopyLittle16ToLittle16,
	RotatedBlitScaleCopyBig16ToLittle16,
	RotatedBlitScaleOverLittle32ToLittle16,
	RotatedBlitScaleOverBig32ToLittle16
};

static BlitFunction scaleAlphaRotatedBlitTable16Little[] = {
	NULL,
	NULL,
	RotatedBlitScaleOver8ToLittle16,
	RotatedBlitScaleOver8ToLittle16,
	RotatedBlitScaleAlphaLittle15ToLittle16,
	RotatedBlitScaleAlphaBig15ToLittle16,
	RotatedBlitScaleCopyLittle16ToLittle16,
	RotatedBlitScaleCopyBig16ToLittle16,
	RotatedBlitScaleAlphaLittle32ToLittle16,
	RotatedBlitScaleAlphaBig32ToLittle16
};

static BlitFunction scaleFunctionRotatedBlitTable16Little[] = {
	NULL,
	NULL,
	RotatedBlitScaleFunction8ToLittle16,
	RotatedBlitScaleFunction8ToLittle16,
	RotatedBlitScaleFunctionLittle15ToLittle16,
	RotatedBlitScaleFunctionBig15ToLittle16,
	RotatedBlitScaleFunctionLittle16ToLittle16,
	RotatedBlitScaleFunctionBig16ToLittle16,
	RotatedBlitScaleFunctionLittle32ToLittle16,
	RotatedBlitScaleFunctionBig32ToLittle16
};

static BlitFunction scaleConstAlphaRotatedBlitTable16Little[] = {
	NULL,
	NULL,
	RotatedBlitScaleConstAlpha8ToLittle16,
	RotatedBlitScaleConstAlpha8ToLittle16,
	RotatedBlitScaleConstAlphaLittle15ToLittle16,
	RotatedBlitScaleConstAlphaBig15ToLittle16,
	RotatedBlitScaleConstAlphaLittle16ToLittle16,
	RotatedBlitScaleConstAlphaBig16ToLittle16,
	RotatedBlitScaleConstAlphaLittle32ToLittle16,
	RotatedBlitScaleConstAlphaBig32ToLittle16
};
#endif

static DrawOneLine fDrawOneLine1516[NUM_OPS][2] = 
{
	{ &DrawOneLineCopy1516, &DrawOneLineCopyP1516 },
	{ &DrawOneLineOverErase1516, &DrawOneLineOverEraseP1516 },
	{ &DrawOneLineOverErase1516, &DrawOneLineOverEraseP1516 },
	{ &DrawOneLineInvert1516, &DrawOneLineInvertP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ &DrawOneLineOther1516, &DrawOneLineOtherP1516 },
	{ NULL, NULL }
};

static FillRects fFillRects1516[NUM_OPS][2] = 
{
	{ &FillRectsCopyOverErase1516, &FillRectsCopyP1516 },
	{ &FillRectsCopyOverErase1516, &FillRectsOverEraseP1516 },
	{ &FillRectsCopyOverErase1516, &FillRectsOverEraseP1516 },
	{ &FillRectsInvert1516, &FillRectsInvertP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ &FillRectsOther1516, &FillRectsOtherP1516 },
	{ NULL, NULL }
};

static BlitFunction *fBlitTables15Little[NUM_OPS] =
{
	copyBlitTable15Little,
	overBlitTable15Little,
	eraseBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	NULL
};

static BlitFunction *fScaledBlitTables15Little[NUM_OPS] =
{
	scaleCopyBlitTable15Little,
	scaleOverBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	scaleFunctionBlitTable15Little,
	NULL
};

static BlitFunction scaleConstAlphaBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToLittle15,
	BlitScaleConstAlpha8ToLittle15,
	BlitScaleConstAlphaLittle15ToLittle15,
	BlitScaleConstAlphaBig15ToLittle15,
	BlitScaleConstAlphaLittle16ToLittle15,
	BlitScaleConstAlphaBig16ToLittle15,
	BlitScaleConstAlphaLittle32ToLittle15,
	BlitScaleConstAlphaBig32ToLittle15
};

static BlitFunction scaleConstAlphaBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToBig15,
	BlitScaleConstAlpha8ToBig15,
	BlitScaleConstAlphaLittle15ToBig15,
	BlitScaleConstAlphaBig15ToBig15,
	BlitScaleConstAlphaLittle16ToBig15,
	BlitScaleConstAlphaBig16ToBig15,
	BlitScaleConstAlphaLittle32ToBig15,
	BlitScaleConstAlphaBig32ToBig15
};

static BlitFunction scaleAlphaCompositeBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleOver8ToLittle15,
	BlitScaleOver8ToLittle15,
	BlitScaleAlphaCompositeLittle15ToLittle15,
	BlitScaleAlphaCompositeBig15ToLittle15,
	BlitScaleCopyLittle16ToLittle15,
	BlitScaleCopyBig16ToLittle15,
	BlitScaleAlphaCompositeLittle32ToLittle15,
	BlitScaleAlphaCompositeBig32ToLittle15
};

static BlitFunction scaleAlphaCompositeBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleOver8ToBig15,
	BlitScaleOver8ToBig15,
	BlitScaleAlphaCompositeLittle15ToBig15,
	BlitScaleAlphaCompositeBig15ToBig15,
	BlitScaleCopyLittle16ToBig15,
	BlitScaleCopyBig16ToBig15,
	BlitScaleAlphaCompositeLittle32ToBig15,
	BlitScaleAlphaCompositeBig32ToBig15
};

static BlitFunction scaleConstAlphaCompositeBlitTable15Little[] = {
	NULL,
	NULL,
	BlitScaleConstAlphaComposite8ToLittle15,
	BlitScaleConstAlphaComposite8ToLittle15,
	BlitScaleConstAlphaCompositeLittle15ToLittle15,
	BlitScaleConstAlphaCompositeBig15ToLittle15,
	BlitScaleConstAlphaCompositeLittle16ToLittle15,
	BlitScaleConstAlphaCompositeBig16ToLittle15,
	BlitScaleConstAlphaCompositeLittle32ToLittle15,
	BlitScaleConstAlphaCompositeBig32ToLittle15
};

static BlitFunction scaleConstAlphaCompositeBlitTable15Big[] = {
	NULL,
	NULL,
	BlitScaleConstAlphaComposite8ToBig15,
	BlitScaleConstAlphaComposite8ToBig15,
	BlitScaleConstAlphaCompositeLittle15ToBig15,
	BlitScaleConstAlphaCompositeBig15ToBig15,
	BlitScaleConstAlphaCompositeLittle16ToBig15,
	BlitScaleConstAlphaCompositeBig16ToBig15,
	BlitScaleConstAlphaCompositeLittle32ToBig15,
	BlitScaleConstAlphaCompositeBig32ToBig15
};

static BlitFunction *alphaLittle15[2][2] =
{
	{
		alphaBlitTable15Little,
		scaleAlphaCompositeBlitTable15Little
	},
	{
		scaleConstAlphaBlitTable15Little,
		scaleConstAlphaCompositeBlitTable15Little
	}
};

static BlitFunction *scaleAlphaLittle15[2][2] =
{
	{
		scaleAlphaBlitTable15Little,
		scaleAlphaCompositeBlitTable15Little
	},
	{
		scaleConstAlphaBlitTable15Little,
		scaleConstAlphaCompositeBlitTable15Little
	}
};

static BlitFunction *alphaBig15[2][2] =
{
	{
		alphaBlitTable15Big,
		scaleAlphaCompositeBlitTable15Big
	},
	{
		scaleConstAlphaBlitTable15Big,
		scaleConstAlphaCompositeBlitTable15Big
	}
};

static BlitFunction *scaleAlphaBig15[2][2] =
{
	{
		scaleAlphaBlitTable15Big,
		scaleAlphaCompositeBlitTable15Big
	},
	{
		scaleConstAlphaBlitTable15Big,
		scaleConstAlphaCompositeBlitTable15Big
	}
};

static DrawString DrawStringTable1516[2] = {
	DrawStringNonAAOther,
	DrawStringAAPreprocess
};

#if B_HOST_IS_LENDIAN
	#define AlphaLittle15Preprocess AlphaNative15Preprocess
	#define AlphaBig15Preprocess AlphaOpp15Preprocess
	#define AlphaLittle16Preprocess AlphaNative16Preprocess
	#define AlphaBig16Preprocess AlphaOpp16Preprocess
	#define AlphaLittle16PreprocessRotated AlphaNative16PreprocessRotated
#else
	#define AlphaBig15Preprocess AlphaNative15Preprocess
	#define AlphaLittle15Preprocess AlphaOpp15Preprocess
	#define AlphaBig16Preprocess AlphaNative16Preprocess
	#define AlphaLittle16Preprocess AlphaOpp16Preprocess
	#define AlphaBig16PreprocessRotated AlphaNative16PreprocessRotated
#endif

static void PickRenderers15Little(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects1516[op][stippling];
	cache->fDrawOneLine = fDrawOneLine1516[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
	cache->fBlitTable = fBlitTables15Little[op];
	cache->fScaledBlitTable = fScaledBlitTables15Little[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[PIX_15BIT*10+op];

	if (!cache->fBlitTable) {
		cache->fBlitTable = alphaLittle15[context->srcAlphaSrc][context->alphaFunction];
		cache->fScaledBlitTable = scaleAlphaLittle15[context->srcAlphaSrc][context->alphaFunction];
	};

	if (!cache->fFillRects) {
		cache->fFillRects = FillRectsAlpha15Little;
		cache->fDrawOneLine = DrawOneLineAlpha15Little;
	};

	if (context->fontContext.IsValid()) {
		if (op == OP_ALPHA)
			cache->fDrawString = AlphaLittle15Preprocess;
		else
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
	};
};

static BlitFunction *fBlitTables15Big[NUM_OPS] =
{
	copyBlitTable15Big,
	overBlitTable15Big,
	eraseBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	NULL
};

static BlitFunction *fScaledBlitTables15Big[NUM_OPS] =
{
	scaleCopyBlitTable15Big,
	scaleOverBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	scaleFunctionBlitTable15Big,
	NULL
};

static void PickRenderers15Big(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects1516[op][stippling];
	cache->fDrawOneLine = fDrawOneLine1516[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
	cache->fBlitTable = fBlitTables15Big[op];
	cache->fScaledBlitTable = fScaledBlitTables15Big[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[(5*10)+PIX_15BIT*10+op];

	if (!cache->fBlitTable) {
		cache->fBlitTable = alphaBig15[context->srcAlphaSrc][context->alphaFunction];
		cache->fScaledBlitTable = scaleAlphaBig15[context->srcAlphaSrc][context->alphaFunction];
	};

	if (!cache->fFillRects) {
		cache->fFillRects = FillRectsAlpha15Big;
		cache->fDrawOneLine = DrawOneLineAlpha15Big;
	};

	if (context->fontContext.IsValid()) {
		if (op == OP_ALPHA)
			cache->fDrawString = AlphaBig15Preprocess;
		else
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
	};
};

static BlitFunction *fBlitTables16Little[NUM_OPS] =
{
	copyBlitTable16Little,
	overBlitTable16Little,
	eraseBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	NULL
};

static BlitFunction *fScaledBlitTables16Little[NUM_OPS] =
{
	scaleCopyBlitTable16Little,
	scaleOverBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	scaleFunctionBlitTable16Little,
	NULL
};

#if ROTATE_DISPLAY
static BlitFunction *fRotatedBlitTables16Little[NUM_OPS] =
{
	copyRotatedBlitTable16Little,
	overRotatedBlitTable16Little,
	eraseRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	NULL
};

static BlitFunction *fScaledRotatedBlitTables16Little[NUM_OPS] =
{
	scaleCopyRotatedBlitTable16Little,
	scaleOverRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	scaleFunctionRotatedBlitTable16Little,
	NULL
};
#endif

static BlitFunction scaleConstAlphaBlitTable16Little[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToLittle16,
	BlitScaleConstAlpha8ToLittle16,
	BlitScaleConstAlphaLittle15ToLittle16,
	BlitScaleConstAlphaBig15ToLittle16,
	BlitScaleConstAlphaLittle16ToLittle16,
	BlitScaleConstAlphaBig16ToLittle16,
	BlitScaleConstAlphaLittle32ToLittle16,
	BlitScaleConstAlphaBig32ToLittle16
};

static void PickRenderers16Little(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects1516[op][stippling];
	cache->fDrawOneLine = fDrawOneLine1516[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
#if ROTATE_DISPLAY
	cache->fRotatedBlitTable = fRotatedBlitTables16Little[op];
	cache->fScaledRotatedBlitTable = fScaledRotatedBlitTables16Little[op];
#endif
	cache->fBlitTable = fBlitTables16Little[op];
	cache->fScaledBlitTable = fScaledBlitTables16Little[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[PIX_16BIT*10+op];

#if ROTATE_DISPLAY
	if (!cache->fRotatedBlitTable) {
		if (context->srcAlphaSrc == SOURCE_ALPHA_PIXEL) {
			cache->fRotatedBlitTable = alphaRotatedBlitTable16Little;
			cache->fScaledRotatedBlitTable = scaleAlphaRotatedBlitTable16Little;
		} else {
			cache->fRotatedBlitTable =
			cache->fScaledRotatedBlitTable = scaleConstAlphaRotatedBlitTable16Little;
		};
	};
#endif
	if (!cache->fBlitTable) {
		if (context->srcAlphaSrc == SOURCE_ALPHA_PIXEL) {
			cache->fBlitTable = alphaBlitTable16Little;
			cache->fScaledBlitTable = scaleAlphaBlitTable16Little;
		} else {
			cache->fBlitTable =
			cache->fScaledBlitTable = scaleConstAlphaBlitTable16Little;
		};
	};

	if (!cache->fFillRects) {
		cache->fFillRects = FillRectsAlpha16Little;
		cache->fDrawOneLine = DrawOneLineAlpha16Little;
	};

	if (context->fontContext.IsValid()) {
		if (op == OP_ALPHA) {
#if ROTATE_DISPLAY
			if (port->canvasIsRotated) {
				cache->fDrawString = AlphaLittle16PreprocessRotated;
			} else
#endif
			cache->fDrawString = AlphaLittle16Preprocess;
		} else {
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
		}
	}
}

static BlitFunction *fBlitTables16Big[NUM_OPS] =
{
	copyBlitTable16Big,
	overBlitTable16Big,
	eraseBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	NULL
};

static BlitFunction *fScaledBlitTables16Big[NUM_OPS] =
{
	scaleCopyBlitTable16Big,
	scaleOverBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	scaleFunctionBlitTable16Big,
	NULL
};

static BlitFunction scaleConstAlphaBlitTable16Big[] = {
	NULL,
	NULL,
	BlitScaleConstAlpha8ToBig16,
	BlitScaleConstAlpha8ToBig16,
	BlitScaleConstAlphaLittle15ToBig16,
	BlitScaleConstAlphaBig15ToBig16,
	BlitScaleConstAlphaLittle16ToBig16,
	BlitScaleConstAlphaBig16ToBig16,
	BlitScaleConstAlphaLittle32ToBig16,
	BlitScaleConstAlphaBig32ToBig16
};

static void PickRenderers16Big(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	cache->fFillRects = fFillRects1516[op][stippling];
	cache->fDrawOneLine = fDrawOneLine1516[op][stippling];
	cache->fFillSpans = grFillSpansSoftware;
	cache->fBlitTable = fBlitTables16Big[op];
	cache->fScaledBlitTable = fScaledBlitTables16Big[op];
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[(5*10)+PIX_16BIT*10+op];

	if (!cache->fBlitTable) {
		if (context->srcAlphaSrc == SOURCE_ALPHA_PIXEL) {
			cache->fBlitTable = alphaBlitTable16Big;
			cache->fScaledBlitTable = scaleAlphaBlitTable16Big;
		} else {
			cache->fBlitTable =
			cache->fScaledBlitTable = scaleConstAlphaBlitTable16Big;
		};
	};

	if (!cache->fFillRects) {
		cache->fFillRects = FillRectsAlpha16Big;
		cache->fDrawOneLine = DrawOneLineAlpha16Big;
	};

	if (context->fontContext.IsValid()) {
		if (op == OP_ALPHA)
			cache->fDrawString = AlphaBig16Preprocess;
		else
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
	};
};

static void ColorChanged15(
	RenderContext *context,
	RenderCache *cache,
	rgb_color fc,
	uint32 *canvasFormat,
	uint32 *srcAlpha,
	uint32 *alphaPix,
	DrawString alphaPreprocess)
{
	uint32 pix = ((uint16)(fc.red & 0xF8)) << 7;
	pix |= ((uint16)(fc.green & 0xF8)) << 2;
	pix |= ((uint16)fc.blue) >> 3;
	pix |= ((uint16)(fc.alpha & 0x80)) << 8;
	pix |= pix << 16;
	*canvasFormat = pix;
	
	int32 alpha = fc.alpha;
	alpha += alpha >> 7;
	*srcAlpha = alpha;
	fc.red = (fc.red * alpha) >> 8;
	fc.green = (fc.green * alpha) >> 8;
	fc.blue = (fc.blue * alpha) >> 8;
	pix  = ((uint16)(fc.red & 0xF8)) << 7;
	pix |= ((uint16)(fc.green & 0xF8)) << 2;
	pix |= ((uint16)fc.blue) >> 3;
	pix |= ((uint16)(fc.alpha & 0x80)) << 8;
	*alphaPix = pix;

	if (context->fontContext.IsValid()) {
		if (context->drawOp == OP_ALPHA)
			cache->fDrawString = alphaPreprocess;
		else
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
	};
};

static void ColorChanged16(
	RenderContext *context,
	RenderCache *cache,
	rgb_color fc,
	uint32 *canvasFormat,
	uint32 *srcAlpha,
	uint32 *alphaPix,
	DrawString alphaPreprocess)
{
	uint32 pix = ((uint16)(fc.red & 0xF8)) << 8;
	pix |= ((uint16)(fc.green & 0xFC)) << 3;
	pix |= ((uint16)fc.blue) >> 3;
	pix |= pix << 16;
	*canvasFormat = pix;
	
	int32 alpha = fc.alpha;
	alpha += alpha >> 7;
	*srcAlpha = alpha;
	fc.red = (fc.red * alpha) >> 8;
	fc.green = (fc.green * alpha) >> 8;
	fc.blue = (fc.blue * alpha) >> 8;
	pix  = ((uint16)(fc.red & 0xF8)) << 8;
	pix |= ((uint16)(fc.green & 0xFC)) << 3;
	pix |= ((uint16)fc.blue) >> 3;
	*alphaPix = pix;

	if (context->fontContext.IsValid()) {
		if (context->drawOp == OP_ALPHA)
			cache->fDrawString = alphaPreprocess;
		else
			cache->fDrawString = DrawStringTable1516
				[context->fontContext.BitsPerPixel() != FC_BLACK_AND_WHITE];
	};
};

static void ForeColorChanged15Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged15(
		context,cache,context->foreColor,
		&cache->foreColorCanvasFormat,
		&cache->foreSrcAlpha,
		&cache->foreAlphaPix,
		AlphaNative15Preprocess);
};

static void BackColorChanged15Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged15(
		context,cache,context->backColor,
		&cache->backColorCanvasFormat,
		&cache->backSrcAlpha,
		&cache->backAlphaPix,
		AlphaNative15Preprocess);
};

static void ForeColorChanged15NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged15(
		context,cache,context->foreColor,
		&cache->foreColorCanvasFormat,
		&cache->foreSrcAlpha,
		&cache->foreAlphaPix,
		AlphaOpp15Preprocess);
	cache->foreColorCanvasFormat =
		((cache->foreColorCanvasFormat >> 8) & 0x00FF00FF) |
		((cache->foreColorCanvasFormat << 8) & 0xFF00FF00) ;
	cache->foreAlphaPix =
		((cache->foreAlphaPix >> 8) & 0x00FF) |
		((cache->foreAlphaPix << 8) & 0xFF00) ;
};

static void BackColorChanged15NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged15(
		context,cache,context->backColor,
		&cache->backColorCanvasFormat,
		&cache->backSrcAlpha,
		&cache->backAlphaPix,
		AlphaOpp15Preprocess);
	cache->backColorCanvasFormat =
		((cache->backColorCanvasFormat >> 8) & 0x00FF00FF) |
		((cache->backColorCanvasFormat << 8) & 0xFF00FF00) ;
	cache->backAlphaPix =
		((cache->backAlphaPix >> 8) & 0x00FF) |
		((cache->backAlphaPix << 8) & 0xFF00) ;
};

static void ForeColorChanged16Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		ColorChanged16(
			context,cache,context->foreColor,
			&cache->foreColorCanvasFormat,
			&cache->foreSrcAlpha,
			&cache->foreAlphaPix,
			AlphaNative16PreprocessRotated);
	} else
#endif
	ColorChanged16(
		context,cache,context->foreColor,
		&cache->foreColorCanvasFormat,
		&cache->foreSrcAlpha,
		&cache->foreAlphaPix,
		AlphaNative16Preprocess);
};

static void BackColorChanged16Host(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		ColorChanged16(
			context,cache,context->backColor,
			&cache->backColorCanvasFormat,
			&cache->backSrcAlpha,
			&cache->backAlphaPix,
			AlphaNative16PreprocessRotated);
	} else
#endif
	ColorChanged16(
		context,cache,context->backColor,
		&cache->backColorCanvasFormat,
		&cache->backSrcAlpha,
		&cache->backAlphaPix,
		AlphaNative16Preprocess);
};

static void ForeColorChanged16NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged16(
		context,cache,context->foreColor,
		&cache->foreColorCanvasFormat,
		&cache->foreSrcAlpha,
		&cache->foreAlphaPix,
		AlphaOpp16Preprocess);
	cache->foreColorCanvasFormat =
		((cache->foreColorCanvasFormat >> 8) & 0x00FF00FF) |
		((cache->foreColorCanvasFormat << 8) & 0xFF00FF00) ;
	cache->foreAlphaPix =
		((cache->foreAlphaPix >> 8) & 0x00FF) |
		((cache->foreAlphaPix << 8) & 0xFF00) ;
};

static void BackColorChanged16NonHost(RenderContext *context, RenderSubPort *port)
{
	grAssertLocked(context);
	
	RenderCache *cache = port->cache;
	ColorChanged16(
		context,cache,context->backColor,
		&cache->backColorCanvasFormat,
		&cache->backSrcAlpha,
		&cache->backAlphaPix,
		AlphaOpp16Preprocess);
	cache->backColorCanvasFormat =
		((cache->backColorCanvasFormat >> 8) & 0x00FF00FF) |
		((cache->backColorCanvasFormat << 8) & 0xFF00FF00) ;
	cache->backAlphaPix =
		((cache->backAlphaPix >> 8) & 0x00FF) |
		((cache->backAlphaPix << 8) & 0xFF00) ;
};

extern void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port);
extern void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port);

RenderingPackage renderPackage15Little = {
	PickRenderers15Little,
#if B_HOST_IS_LENDIAN
	ForeColorChanged15Host,
	BackColorChanged15Host
#else
	ForeColorChanged15NonHost,
	BackColorChanged15NonHost
#endif
};

RenderingPackage renderPackage15Big = {
	PickRenderers15Big,
#if B_HOST_IS_LENDIAN
	ForeColorChanged15NonHost,
	BackColorChanged15NonHost
#else
	ForeColorChanged15Host,
	BackColorChanged15Host
#endif
};

RenderingPackage renderPackage16Little = {
	PickRenderers16Little,
#if B_HOST_IS_LENDIAN
	ForeColorChanged16Host,
	BackColorChanged16Host
#else
	ForeColorChanged16NonHost,
	BackColorChanged16NonHost
#endif
};

RenderingPackage renderPackage16Big = {
	PickRenderers16Big,
#if B_HOST_IS_LENDIAN
	ForeColorChanged16NonHost,
	BackColorChanged16NonHost
#else
	ForeColorChanged16Host,
	BackColorChanged16Host
#endif
};
