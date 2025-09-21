
#include <GraphicsDefs.h>
#include <Debug.h>
#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"

#define	energy(x) ((((x)&0xFF0000)>>16)+(((x)&0xFF00)>>8)+(((x)&0xFF)))

/* 	These work under the assumption that both the source and destination
	are in 32-bit, host endianess */

uint32 colorOp_copy(uint32 src, uint32 /*dst*/)
{
	/*	This is actually incorrect for transparent pixels,
		but then I don't think it will ever be used. */
	return src;
};

uint32 colorOp_over(uint32 src, uint32  /*dst*/)
{
	return src;
};

uint32 colorOp_invert(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	return (0xFFFFFFFF - dst);
};

uint32 colorOp_erase(uint32 src, uint32 /*dst*/, RenderContext *context)
{
	if (src == ARGBMagicTransparent) return src;
	rgb_color c = context->backColor;
	return (((uint32)c.alpha)<<24)|(((uint32)c.red)<<16)|(((uint32)c.blue)<<8)|c.green;
};

uint32 colorOp_select(uint32 src, uint32 dst, RenderContext *context)
{
	if (src == ARGBMagicTransparent) return src;
	rgb_color bc = context->backColor;
	rgb_color fc = context->foreColor;
	uint32 bp = (((uint32)bc.alpha)<<24)|(((uint32)bc.red)<<16)|(((uint32)bc.blue)<<8)|bc.green;
	uint32 fp = (((uint32)fc.alpha)<<24)|(((uint32)fc.red)<<16)|(((uint32)fc.blue)<<8)|fc.green;
	if (bp == dst) return fp;
	if (fp == dst) return bp;
	return ARGBMagicTransparent;
};

uint32 colorOp_blend(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	return ((src & 0xFEFEFEFE)>>1)+((dst & 0xFEFEFEFE)>>1)+(src & dst & 0x01010101);
};

uint32 colorOp_add(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	return ((((((src^dst)>>1)^((src>>1)+(dst>>1)))&0x80808080L)>>7)*0xFF)|(src+dst);
};

uint32 colorOp_sub(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	src ^= 0xFFFFFFFFL;
	return ((((((src^dst)>>1)^((src>>1)+(dst>>1)))&0x80808080L)>>7)*0xFF)&(src+dst+1);
};

uint32 colorOp_min(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	if (energy(src) < energy(dst))
		return src;
	else
		return dst;
};

uint32 colorOp_max(uint32 src, uint32 dst)
{
	if (src == ARGBMagicTransparent) return src;
	if (energy(src) > energy(dst))
		return src;
	else
		return dst;
};

#define DestEndianess											LITTLE_ENDIAN
	#define DestBits											8
		#define ColorOpTrans									Either8
		#include "color_op.inc"
	#define DestBits											15
		#define ColorOpTrans									Little15
		#include "color_op.inc"
	#define DestBits											16
		#define ColorOpTrans									Little16
		#include "color_op.inc"
	#define DestBits											32
		#define ColorOpTrans									Little32
		#include "color_op.inc"
#undef DestEndianess
#define DestEndianess											BIG_ENDIAN
	#define DestBits											15
		#define ColorOpTrans									Big15
		#include "color_op.inc"
	#define DestBits											16
		#define ColorOpTrans									Big16
		#include "color_op.inc"
	#define DestBits											32
		#define ColorOpTrans									Big32
		#include "color_op.inc"
#undef DestEndianess

void * colorOpTable[] = {
	(void *)colorOp_copy,
	(void *)colorOp_over,
	(void *)colorOp_erase,
	(void *)colorOp_invert,
	(void *)colorOp_add,
	(void *)colorOp_sub,
	(void *)colorOp_blend,
	(void *)colorOp_min,
	(void *)colorOp_max,
	(void *)colorOp_select
};

/* [endianess][bits][operation] */
void * colorOpTransTable[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Either8::Add,
	(void *)Either8::Sub,
	(void *)Either8::Blend,
	(void *)Either8::Min,
	(void *)Either8::Max,
	(void *)Either8::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Little15::Add,
	(void *)Little15::Sub,
	(void *)Little15::Blend,
	(void *)Little15::Min,
	(void *)Little15::Max,
	(void *)Little15::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Little16::Add,
	(void *)Little16::Sub,
	(void *)Little16::Blend,
	(void *)Little16::Min,
	(void *)Little16::Max,
	(void *)Little16::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Little32::Add,
	(void *)Little32::Sub,
	(void *)Little32::Blend,
	(void *)Little32::Min,
	(void *)Little32::Max,
	(void *)Little32::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Either8::Add,
	(void *)Either8::Sub,
	(void *)Either8::Blend,
	(void *)Either8::Min,
	(void *)Either8::Max,
	(void *)Either8::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Big15::Add,
	(void *)Big15::Sub,
	(void *)Big15::Blend,
	(void *)Big15::Min,
	(void *)Big15::Max,
	(void *)Big15::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Big16::Add,
	(void *)Big16::Sub,
	(void *)Big16::Blend,
	(void *)Big16::Min,
	(void *)Big16::Max,
	(void *)Big16::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	(void *)Big32::Add,
	(void *)Big32::Sub,
	(void *)Big32::Blend,
	(void *)Big32::Min,
	(void *)Big32::Max,
	(void *)Big32::Select
};
