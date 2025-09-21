
#include <Debug.h>
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
			#u