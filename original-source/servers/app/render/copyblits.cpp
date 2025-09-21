
#include <GraphicsDefs.h>
#include <Debug.h>
#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"

#define BlitScaling 											0
#define RenderMode												DEFINE_OP_COPY
	//	Generate one case for copying same format without
	//	worrying about write gathering (i.e. for optimal PCI
	//	transactions on PPC)
	#define DestBits 											8
	#define SourceBits											8
	#define DestEndianess										LITTLE_ENDIAN
	#define SourceEndianess										LITTLE_ENDIAN

#if __INTEL__
	#define WorryAboutWriteGathering							0
	#define BlitFuncName										BlitCopySameFormatNoWriteGather
	#include "blitting.inc"
	#undef WorryAboutWriteGathering
#else
	//	Generate one case for copying same format,
	//	worrying about write gathering
	#define WorryAboutWriteGathering							1
	#define BlitFuncName										BlitCopySameFormatWriteGather
	#include "blitting.inc"
	#undef WorryAboutWriteGathering
#endif

	#undef DestBits
	#undef SourceBits
	#undef DestEndianess
	#undef SourceEndianess
#undef RenderMode
#undef BlitScaling

#if __INTEL__
BlitFunction BlitCopySameFormat = BlitCopySameFormatNoWriteGather;
#else
BlitFunction BlitCopySameFormat = BlitCopySameFormatWriteGather;
#endif
