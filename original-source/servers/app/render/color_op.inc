
/*

#define SourceFormatBits		DestBits
#define SourceFormatEndianess	DestEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define DstToARGB(a,b) ConvertPixelFormat(a,b);
#else
	#define DstToARGB(a,b) b=a;
#endif

#define SourceFormatBits		32
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			DestBits
#define DestFormatEndianess		DestEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
#else
	#define ARGBToDst(a,b) b=a;
#endif

*/

#if (DestBits == 8)
	#define CMapDeclarations																\
		uint32 *index2ARGB = (uint32*)port->canvas->pixels.colorMap->index2ARGB;			\
		uint8 *RGB2Index = (uint8*)port->canvas->pixels.colorMap->inverted;
#else
	#define CMapDeclarations
#endif

/* To simplify things, we use a class here for namespace purposes only */

class ColorOpTrans {
	public:
	
static uint32 Select(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	uint32 bp,fp,bb,ff;
	rgb_color bc = context->backColor;
	rgb_color fc = context->foreColor;
	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif
	bp = (((uint32)bc.alpha)<<24)|(((uint32)bc.red)<<16)|(((uint32)bc.blue)<<8)|bc.green;
	fp = (((uint32)fc.alpha)<<24)|(((uint32)fc.red)<<16)|(((uint32)fc.blue)<<8)|fc.green;
	ARGBToDst(bp,bb);
	ARGBToDst(fp,ff);
	if (bb == dst) return ff;
	if (ff == dst) return bb;
	return dst;
};

#undef ARGBToDst
#undef DstToARGB

static uint32 Blend(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	#define SourceFormatBits		DestBits
	#define SourceFormatEndianess	DestEndianess
	#define DestFormatBits			32
	#define DestFormatEndianess		HostEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define DstToARGB(a,b) ConvertPixelFormat(a,b);
	#else
		#define DstToARGB(a,b) b=a;
	#endif

	uint32 dstARGB;
	DstToARGB(dst,dstARGB);
	dstARGB = ((src & 0xFEFEFEFE)>>1)+((dstARGB & 0xFEFEFEFE)>>1)+(src & dstARGB & 0x01010101);

	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif
	ARGBToDst(dstARGB,dst);
	return dst;
};

#undef ARGBToDst
#undef DstToARGB

static uint32 Add(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	#define SourceFormatBits		DestBits
	#define SourceFormatEndianess	DestEndianess
	#define DestFormatBits			32
	#define DestFormatEndianess		HostEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define DstToARGB(a,b) ConvertPixelFormat(a,b);
	#else
		#define DstToARGB(a,b) b=a;
	#endif

	uint32 dstARGB;
	DstToARGB(dst,dstARGB);
	dstARGB = ((((((src^dstARGB)>>1)^((src>>1)+(dstARGB>>1)))&0x80808080L)>>7)*0xFF)|(src+dstARGB);

	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif
	ARGBToDst(dstARGB,dst);
	return dst;
};

#undef ARGBToDst
#undef DstToARGB

static uint32 Sub(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	#define SourceFormatBits		DestBits
	#define SourceFormatEndianess	DestEndianess
	#define DestFormatBits			32
	#define DestFormatEndianess		HostEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define DstToARGB(a,b) ConvertPixelFormat(a,b);
	#else
		#define DstToARGB(a,b) b=a;
	#endif

	uint32 dstARGB;
	DstToARGB(dst,dstARGB);
	src ^= 0xFFFFFFFFL;
	dstARGB = ((((((src^dstARGB)>>1)^((src>>1)+(dstARGB>>1)))&0x80808080L)>>7)*0xFF)&(src+dstARGB+1);

	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif
	ARGBToDst(dstARGB,dst);
	return dst;
};

#undef ARGBToDst
#undef DstToARGB

static uint32 Min(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	#define SourceFormatBits		DestBits
	#define SourceFormatEndianess	DestEndianess
	#define DestFormatBits			32
	#define DestFormatEndianess		HostEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define DstToARGB(a,b) ConvertPixelFormat(a,b);
	#else
		#define DstToARGB(a,b) b=a;
	#endif

	uint32 dstARGB;
	DstToARGB(dst,dstARGB);

	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif

	if (energy(src) < energy(dstARGB)) {
		ARGBToDst(src,dst);
	};
	return dst;
};

#undef ARGBToDst
#undef DstToARGB

static uint32 Max(uint32 src, uint32 dst, RenderContext *context, RenderSubPort *port)
{
	CMapDeclarations;
	#define SourceFormatBits		DestBits
	#define SourceFormatEndianess	DestEndianess
	#define DestFormatBits			32
	#define DestFormatEndianess		HostEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define DstToARGB(a,b) ConvertPixelFormat(a,b);
	#else
		#define DstToARGB(a,b) b=a;
	#endif

	uint32 dstARGB;
	DstToARGB(dst,dstARGB);

	#define SourceFormatBits		32
	#define SourceFormatEndianess	HostEndianess
	#define DestFormatBits			DestBits
	#define DestFormatEndianess		DestEndianess
	#include "pixelConvert.inc"
	#ifdef ConvertPixelFormat
		#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
	#else
		#define ARGBToDst(a,b) b=a;
	#endif

	if (energy(src) > energy(dstARGB)) {
		ARGBToDst(src,dst);
	};
	return dst;
};

#undef ARGBToDst
#undef DstToARGB
};

#undef ColorOpTrans
#undef DestBits
#undef CMapDeclarations
