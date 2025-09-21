
#include <raster2/GraphicsDefs.h>
#include <support2/Debug.h>
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
	colorOp_copy,
	colorOp_over,
	colorOp_erase,
	colorOp_invert,
	colorOp_add,
	colorOp_sub,
	colorOp_blend,
	colorOp_min,
	colorOp_max,
	colorOp_select
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
	Either8::Add,
	Either8::Sub,
	Either8::Blend,
	Either8::Min,
	Either8::Max,
	Either8::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Little15::Add,
	Little15::Sub,
	Little15::Blend,
	Little15::Min,
	Little15::Max,
	Little15::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Little16::Add,
	Little16::Sub,
	Little16::Blend,
	Little16::Min,
	Little16::Max,
	Little16::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Little32::Add,
	Little32::Sub,
	Little32::Blend,
	Little32::Min,
	Little32::Max,
	Little32::Select,

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
	Either8::Add,
	Either8::Sub,
	Either8::Blend,
	Either8::Min,
	Either8::Max,
	Either8::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Big15::Add,
	Big15::Sub,
	Big15::Blend,
	Big15::Min,
	Big15::Max,
	Big15::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Big16::Add,
	Big16::Sub,
	Big16::Blend,
	Big16::Min,
	Big16::Max,
	Big16::Select,

	NULL,
	NULL,
	NULL,
	NULL,
	Big32::Add,
	Big32::Sub,
	Big32::Blend,
	Big32::Min,
	Big32::Max,
	Big32::Select
};
