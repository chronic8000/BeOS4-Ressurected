
//******************************************************************************
//
//	File:			renderTypes.h
//	Description:	Types and structures used by the rendering API
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERTYPES_H
#define RENDERTYPES_H

#include <stdio.h>
#include "region.h"

#include "basic_types.h"
#include "font_defs.h"

typedef B::Raster2::BColor32 rgb_color;

struct RenderContext;
struct RenderCache;
struct RenderSubPort;
struct RenderCanvas;

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		float		x0;
		float		y0;
		float		x1;
		float		y1;
		rgb_color	col;
	} a_line;

#ifdef __cplusplus
}
#endif

struct ColorMap
{
	rgb_color	color_list[256];
	uchar		invert_map[256];
	uchar		inverted[32768];
	uint32		index2ARGB[256];
};

struct Pixels {
	int32				h,w;
	int32				bytesPerRow;
	int32				pixelFormat;
	ColorMap *			colorMap;
	uint8 * 			pixelData;
	uint8				endianess:1;
	uint8				pixelIsRotated:1;
	bool				isCompressed:1;
	bool				isLbxCompressed:1;		// implies isCompressed
	uint8				pixelReserved:6;
	uint8 *				pixelDataDMA;
};

typedef void (*DrawString)(
	RenderContext *context,
	RenderSubPort *port,
	fc_string * string);
typedef void (*DrawOneLine)(
 	RenderContext *context,
	RenderSubPort *port,
 	uint8 *base, int32 bpr,
	int32 numPixels,
	int32 x, int32 y,
	int32 xInc, int32 yInc);
typedef void (*FillRects)(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount);
typedef void (*FillSpans)(
	RenderContext *context,
	RenderSubPort *port,
	int32 *fillSpans, int32 spanCount);
typedef void (*BlitFunction) (
	RenderContext *context,
	RenderSubPort *port,
	Pixels *srcPixMap, Pixels *dstPixMap,
	uint8 *src, int32 srcBytesPerRow,
	uint8 *dst, int32 dstBytesPerRow,
	int32 countX, int32 fixedX, int32 incX,
	int32 countY, int32 errorY, int32 incY, int32 defaultPtrIncY,
	int32 bytesPerPixel, int32 srcOffs, int32 directionFlags);

typedef void (*CoordTransform)(
	RenderContext *context,
	fpoint *points, int32 numPoints);
typedef uint32 (*ColorOpFunc) (
	uint32 srcPix, uint32 dstPix, RenderContext *context, RenderSubPort *port);
typedef uint32 (*ColorOpFuncPixel) (
	uint32 srcPix, uint32 dstPix, RenderContext *context);

struct RenderingPackage {
	void		(*PickRenderers)(RenderContext *, RenderSubPort *);
	void		(*ForeColorChanged)(RenderContext *, RenderSubPort *);
	void		(*BackColorChanged)(RenderContext *, RenderSubPort *);
};

#endif
