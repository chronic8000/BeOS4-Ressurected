
#ifndef RENDER_INTERNAL
#define RENDER_INTERNAL

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <IRegion.h>
#include <stdlib.h>
#include <memory.h>
#include "basic_types.h"
#include "renderTypes.h"
#include "renderdefs.h"
#include "renderUtil.h"

class fregion;
struct RenderContext;
struct RenderCache;
struct ColorMap;
struct RenderPort;
struct RenderCanvas;
struct RenderingPackage;
struct Pixels;
class SPath;

// Set this to 1 for debugging
#ifdef DEBUGGING_CONSOLE
#	define RENDER_DEBUG 1
#else
#	define RENDER_DEBUG 0
#endif

#if RENDER_DEBUG
#	define RDebugPrintf(a) RDebugPrintfReal a
#	define RDebugDumpState RDebugDumpStateReal
extern void RDebugPrintfReal(struct RenderContext *context, const char *format, ...);
#else
#	define RDebugPrintf(a)
#	define RDebugDumpState
#endif

#define USE_FONTS 1

#define MAX_OUT 0x7ffffffd
#define MIN_32_BIT ((int32)0xF0000000)
#define MAX_32_BIT ((int32)0x0FFFFFFF)

#define sgn(a) 	(((a)<0) ? -1 : (a)>0 ? 1 : 0)

#define PENSIZE_CHANGED			0x00000001
#define LINEMODE_CHANGED		0x00000002
#define STIPPLE_CHANGED			0x00000004
#define DRAWOP_CHANGED			0x00000008
#define SCALE_CHANGED			0x00000010
#define FORE_COLOR_CHANGED		0x00000020
#define BACK_COLOR_CHANGED		0x00000040
#define ORIGIN_CHANGED			0x00000080
#define LOCATION_CHANGED		0x00000100
#define CLIP_CHANGED			0x00000200

#define RENDER_ROUND_COORDS		0x00000001
#define RENDER_DEBUG_PRINTING	0x00000004

extern  RenderingPackage *renderingPackages[2][5];

#define ARGBMagicTransparent TRANSPARENT_MAGIC_32BIT
extern void *colorOpTable[];
extern void *colorOpTransTable[];

extern void grSyncRecordingState(RenderContext *context);
extern void grForeColorChanged(RenderContext *context);
extern void grBackColorChanged(RenderContext *context);

/* The sync routine used for contexts that have real ports */
extern void grSyncEverything(RenderContext *context, RenderPort *renderPort, bool clipToClientArea);

/* The sync routine used for contexts that are directly on a canvas */
extern void grSyncDirect(RenderContext *context, RenderPort *renderPort, bool clipToClientArea);

void grPickCanvasRenderers(RenderContext *render);
void grPickCanvasRenderers_OneCanvas(RenderContext *render, RenderSubPort *port);
void grPickRenderers(RenderContext *render);

extern void grFillSpansSoftware(RenderContext *context, RenderSubPort *port, int32 *fillSpans, int32 spanCount);
extern void grFillSpansHardware(RenderContext *context, RenderSubPort *port, int32 *fillSpans, int32 spanCount);
extern void grFillRectsHardware(RenderContext *context, RenderSubPort *port, rect *fillRects, int32 rectCount);
extern void grInvertRectsHardware(RenderContext *context, RenderSubPort *port, rect *fillRects, int32 rectCount);

extern void grFillRects(RenderContext *context, RenderPort *ueberPort, rect *fillRects, int32 rectCount);
extern void grFillSpans(RenderContext *context, RenderPort *ueberPort, int32 *fillSpans, int32 spanCount);

extern void grCopyRegion(region **src, region **dst);
extern void grCopyRegion(fregion **src, fregion **dst);
extern void grFillPath_Internal(RenderContext *context, SPath *shape, bool translate, bool includeEdgePixels);
extern void grFillPolygon_internal(RenderContext *context, fpoint *vertices, int32 vCount, bool translate, bool includeEdgePixels);
extern void grStrokeRect_Record(RenderContext *context, frect r);
extern void grStrokeRect_Fat(RenderContext *context, frect r);
extern void grStrokeRect_Thin(RenderContext *context, frect r);
extern void grStrokeBezier_Record(RenderContext *context, fpoint *controlPoints);
extern void grStrokeBezier_Fat(RenderContext *context, fpoint *controlPoints);
extern void grStrokeBezier_Thin(RenderContext *context, fpoint *controlPoints);
extern void grStrokePolygon_Record(RenderContext *context, fpoint *vertices, int32 numVertices, bool closed);
extern void grStrokePolygon_Fat(RenderContext *context, fpoint *vertices, int32 numVertices, bool closed);
extern void grStrokePolygon_Thin(RenderContext *context, fpoint *vertices, int32 numVertices, bool closed);
extern void grFillRegion_Record(RenderContext *context, region *reg);
extern void grFillRegion_FatThin(RenderContext *context, region *reg);
extern void grFillRect_Record(RenderContext *context, frect fr);
extern void grFillRect_FatThin(RenderContext *context, frect fr);
extern void grFillBezier_Record(RenderContext *context, fpoint *controlPoints);
extern void grFillBezier_FatThin(RenderContext *context, fpoint *controlPoints);
extern void grFillPolygon_Record(RenderContext *context, fpoint *vertices, int32 numVertices);
extern void grFillPolygon_FatThin(RenderContext *context, fpoint *vertices, int32 numVertices);
extern void grInscribeStrokeArc_Record(RenderContext *context, frect r, float arcBegin, float arcLen);
extern void grInscribeStrokeEllipse_Record(RenderContext *context, frect r);
extern void grInscribeStrokeArc_FatThin(RenderContext *context, frect r, float arcBegin, float arcLen);
extern void grInscribeStrokeEllipse_FatThin(RenderContext *context, frect r);
extern void grInscribeFillArc_Record(RenderContext *context, frect r, float arcBegin, float arcLen);
extern void grInscribeFillEllipse_Record(RenderContext *context, frect r);
extern void grInscribeFillArc_FatThin(RenderContext *context, frect r, float arcBegin, float arcLen);
extern void grInscribeFillEllipse_FatThin(RenderContext *context, frect r);
extern void grStrokeArc_Record(RenderContext *context, fpoint center, fpoint radius, float arcBegin, float arcLen, bool xform);
extern void grStrokeArc_Fat(RenderContext *context, fpoint center, fpoint radius, float arcBegin, float arcLen, bool xform);
extern void grStrokeArc_Thin(RenderContext *context, fpoint center, fpoint radius, float arcBegin, float arcLen, bool xform);
extern void grStrokeEllipse_Record(RenderContext *context, fpoint center, fpoint radius, bool xform);
extern void grStrokeEllipse_Fat(RenderContext *context, fpoint center, fpoint radius, bool xform);
extern void grStrokeEllipse_Thin(RenderContext *context, fpoint center, fpoint radius, bool xform);
extern void grFillArc_Record(RenderContext *context, fpoint center, fpoint radius, float arcBegin, float arcLen, bool xform);
extern void grFillArc_FatThin(RenderContext *context, fpoint center, fpoint radius, float arcBegin, float arcLen, bool xform);
extern void grFillEllipse_Record(RenderContext *context, fpoint center, fpoint radius, bool xform);
extern void grFillEllipse_FatThin(RenderContext *context, fpoint center, fpoint radius, bool xform);
extern void grStrokeRoundRect_Record(RenderContext *context, frect frame, fpoint radius);
extern void grStrokeRoundRect_Thin(RenderContext *context, frect _frame, fpoint radius);
extern void grStrokeRoundRect_Fat(RenderContext *context, frect frame, fpoint radius);
extern void grFillRoundRect_Record(RenderContext *context, frect frame, fpoint radius);
extern void grFillRoundRect_FatThin(RenderContext *context, frect frame, fpoint radius);
extern void grDrawManyLines(RenderContext *context, fpoint *pts, int32 ptCount, fpoint origin, bool closed, a_line *lineArray);
extern void grRenderThickLines(RenderContext *context, fpoint *vertices, int32 vCount, bool closed, bool translate=true);
extern void grRenderThinLines(RenderContext *context, fpoint *vertices, int32 vCount, bool closed, bool translate=true);
extern void grStrokeLineArray_RecordFat(RenderContext *context, a_line *lineArray, int32 lineArrayCount);
extern void grStrokeLineArray_Thin(RenderContext *context, a_line *lineArray, int32 lineArrayCount);
extern void grStrokeLine_Record(RenderContext *context, fpoint p1, fpoint p2);
extern void grStrokeLine_Fat(RenderContext *context, fpoint p1, fpoint p2);
extern void grStrokeLine_Thin(RenderContext *context, fpoint p1, fpoint p2);
extern void grStrokeILine(RenderContext *context, int32 x1, int32 y1, int32 x2, int32 y2);
extern void grDrawPixelsRecordPart(RenderContext *context, rect pixelsNeeded, int32 bpp, frect srcRect, frect dstRect, Pixels *src);
extern void grDrawPixels_Record(RenderContext *context, frect srcRect, frect dstRect, Pixels *src);
extern void grDrawPixels_FatThin(RenderContext *context, frect srcRect, frect dstRect, Pixels *src);
extern void grTruncateString(RenderContext *context, float targetWidth, int32 mode, const char *srcStr, char *dstStr);
extern void grDrawString_Record(RenderContext *context, const uchar *string, float *deltas);
extern void grDrawString_FatThin(RenderContext *context, const uchar *string, float *deltas);

extern void grMatchContextToPort(RenderContext *context);
extern void grMatchContextToCanvas(RenderContext *context);

extern int32		(*xGetPid)();
extern ColorMap *	xSystemColorMap;

#define min(a,b) (((a)>(b))?(b):(a))
#define max(a,b) (((a)<(b))?(b):(a))

#endif
