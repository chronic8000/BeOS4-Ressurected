
//******************************************************************************
//
//	File:			render.h
//	Description:	The rendering API
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDER_H
#define RENDER_H

#ifndef USE_FONTS
#define USE_FONTS 1
#endif

#include <support2/SupportDefs.h>
#include <raster2/GraphicsDefs.h>

#include "region.h"
#include "as_support.h"
#include "basic_types.h"
#include "font.h"

#include "renderdefs.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderUtil.h"
#include "accelPackage.h"
#include "renderFuncs.h"

struct RenderFuncs;
struct RenderContext;
struct RenderPort;
struct RenderCanvas;
struct RenderingPackage;
struct AccelPackage;

#if 0
class TSession;
#endif

extern float defaultStringDeltas[2];
extern pattern highPattern;
extern pattern lowPattern;

/* Public API */

typedef int32 	(*ThreadIdFunc)();
typedef void 	(*DebugOutputFunc)(const char *format, ...);
void			grInitRenderLib(ThreadIdFunc moreOrLessGetPid, ColorMap *systemMap, DebugOutputFunc debugOut);
void			grSetContextPort(RenderContext *context, void *vendor, LockRenderPort lock, UnlockRenderPort unlock);
void			grSetPortCanvas(RenderSubPort *port, void *vendor, LockRenderCanvas lock, UnlockRenderCanvas unlock);
void			grSetCanvasAccel(RenderCanvas *canvas, void *vendor, LockAccelPackage lock, UnlockAccelPackage unlock);

/* Some shorthand for common "locking" */
void			grSetContextPort_NoLock(RenderContext *context, RenderPort *port);
void			grSetPortCanvas_NoLock(RenderPort *port, RenderCanvas *canvas);

RenderContext *	grNewRenderContext();
void			grInitRenderContext(RenderContext *render);
void			grDestroyRenderContext(RenderContext *render);
void			grFreeRenderContext(RenderContext *context);

RenderSubPort *	grNewRenderSubPort();
void			grInitRenderSubPort(RenderSubPort *port);
void			grDestroyRenderSubPort(RenderSubPort *port);
void			grFreeRenderSubPort(RenderSubPort *port);

RenderPort *	grNewRenderPort();
void			grInitRenderPort(RenderPort *port);
void			grDestroyRenderPort(RenderPort *port);
void			grFreeRenderPort(RenderPort *port);

RenderPort *	grLock(RenderContext *context);
void			grUnlock(RenderContext *context);
void			grSync(RenderContext *context);

void			grContextDebugOutput(RenderContext *context);
void			grPortDebugOutput(RenderPort *port);
void			grCanvasDebugOutput(RenderCanvas *canvas);

void			grTruncateString(RenderContext *context, float targetWidth, int32 mode, const uchar *srcStr, uchar *dstStr);

void			grMergeClip(RenderContext *context);
void			grClipToRegion(RenderContext *context, region *reg);
void			grClearClip(RenderContext *context);

void			grGetClip(RenderContext *context, region *clip);
void			grGetLocalClip(RenderContext *context, region *localClip);
void			grGetUpdateRegion(RenderContext *context, region *update);

void			grSetRounding(RenderContext *context, bool on);
void			grSetScale(RenderContext *render, float scale);
void			grSetOrigin(RenderContext *render, fpoint origin);
void			grSetPenLocation(RenderContext *render, fpoint penLoc);
void			grMovePenLocation(RenderContext *render, fpoint delta);
void			grSetStipplePattern(RenderContext *render, pattern *stipplePattern);
void			grSetPenSize(RenderContext *render, float penSize);
void			grSetForeColor(RenderContext *render, rgb_color foreColor);
void			grSetBackColor(	RenderContext *render, rgb_color backColor);
void			grSetDrawOp(RenderContext *render, short drawOp);
void			grSetBlendingMode(RenderContext *context, int16 srcAlphaSrc, int16 alphaFunction);
void			grSetLineMode(RenderContext *render, short capRule, short joinRule, float miterLimit);
void			grSetFontFromPacket(RenderContext *render, const void *packet, size_t size, int32 mask);
#if 0
void			grSetFontFromSession(RenderContext *render, TSession* a_session, bool read_mask=false);
#endif
void			grSetFont(RenderContext *context, const SFont &font);
void			grForceFontFlags(RenderContext *context, uint32 flags);

void			grGetPixelFormat(RenderContext *render, int32 *pixelFormat);
void			grGetTotalScale(RenderContext *render, float *scale);
void			grGetOrigin(RenderContext *render, fpoint *origin);
void			grGetPenLocation(RenderContext *render, fpoint *penLoc);
void			grGetStipplePattern(RenderContext *render, pattern *stipplePattern);
void			grGetPenSize(RenderContext *render, float *penSize);
void			grGetForeColor(RenderContext *render, rgb_color *foreColor);
void			grGetBackColor(RenderContext *render, rgb_color *backColor);
void			grGetDrawOp(RenderContext *render, short *drawOp);
void			grGetBlendingMode(RenderContext *context, int16 *srcAlphaSrc, int16 *alphaFunction);
void			grGetLineMode(RenderContext *render, short *capRule, short *joinRule, float *miterLimit);
void			grGetFont(RenderContext *context, SFont *font);
void			grGetFontPointer(RenderContext *context, SFont **font);

/*	Integer version of stroke line is only for
	local (server-side) views.  It will never be called
	as the result of a client request */
void			grStrokeILine(RenderContext *context, int32 x1, int32 y1, int32 x2, int32 y2);
void			grStrokeLineArray(RenderContext *context, a_line *lineArray, int32 lineArrayCount);
void			grStrokeLine(RenderContext *render, fpoint p1, fpoint p2);
void			grStrokeRect(RenderContext *render, frect rectangle);
void			grStrokeIRect(RenderContext *context, rect r);
void			grStrokeRoundRect(RenderContext *render, frect rectangle, fpoint radius);
void 			grStrokeBezier(RenderContext *render, fpoint *controlPoints);
void			grInscribeStrokeArc(RenderContext *context, frect r, float arcBegin, float arcLen);
void			grStrokeArc(RenderContext *render, fpoint center, fpoint radius, float startTheta, float stopTheta);
void			grInscribeStrokeEllipse(RenderContext *context, frect r);
void			grStrokeEllipse(RenderContext *render, fpoint center, fpoint radius);
void 			grStrokePolygon(RenderContext *render, fpoint *vertices, int32 vertexCount, bool closed);
void 			grStrokePath(RenderContext *render, SPath *path);

void			grFillRegion(RenderContext *context, region *r);
void			grFillIRect(RenderContext *context, rect r);
void			grFillRect(RenderContext *render, frect rectangle);
void			grFillRoundRect(RenderContext *render, frect rectangle, fpoint radius);
void 			grFillBezier(RenderContext *render, fpoint *controlPoints);
void			grInscribeFillArc(RenderContext *context, frect r, float arcBegin, float arcLen);
void			grFillArc(RenderContext *render, fpoint center, fpoint radius, float startTheta, float stopTheta);
void			grInscribeFillEllipse(RenderContext *context, frect r);
void			grFillEllipse(RenderContext *render, fpoint center, fpoint radius);
void 			grFillPolygon(RenderContext *render, fpoint *vertices, int32 vertexCount);
void 			grFillPath(RenderContext *render, SPath *path);

void			grDrawString(RenderContext *render, const uchar *string, float *deltas=defaultStringDeltas);
void			grDrawPixels(RenderContext *render, frect srcRect, frect dstRect, Pixels *src);
void			grDrawIPixels(RenderContext *context, rect srcRect, rect dstRect, Pixels *src);
void			grDrawLocPixels(RenderContext *context, int32 x, int32 y, Pixels *src);

void			grCopyPixels(RenderContext *context, rect f_rect, point to,	bool clipSource=false);

void			grSetLockingPersistence(RenderContext *context, int32 usecs);
void			grGiveUpLocks(RenderContext *context);

void			grSetDebugPrinting(RenderContext *context, bool on);
void			grSetOwnerThread(RenderContext *context, int32 owner);

void 			grSaveState(RenderContext *context, RenderState *state);
void 			grRestoreState(RenderContext *context, RenderState *state);

void			grPushState(RenderContext *context);
void			grPopState(RenderContext *context);

/***********************************************************/
/* Inlines for the stuff that just calls function pointers */

inline	void grStrokeLineArray(
	RenderContext *context,
	a_line *lineArray, int32 lineArrayCount)
{
	context->renderFuncs->fStrokeLineArray(context,lineArray,lineArrayCount);
}

inline void grStrokeLine(
	RenderContext *context,
	fpoint p1, fpoint p2)
{
	context->renderFuncs->fStrokeLine(context,p1,p2);
}

inline void grStrokeRect(
	RenderContext *context,
	frect rectangle)
{
	context->renderFuncs->fStrokeRect(context,rectangle);
};

inline void grStrokeIRect(
	RenderContext *context,
	rect r)
{
	frect fr;
	fr.top = r.top;
	fr.left = r.left;
	fr.bottom = r.bottom;
	fr.right = r.right;
	context->renderFuncs->fStrokeRect(context,fr);
};

inline void grStrokeRoundRect(
	RenderContext *context,
	frect rectangle, fpoint radius)
{
	context->renderFuncs->fStrokeRoundRect(context,rectangle,radius);
};

inline void grStrokeBezier(
	RenderContext *context,
	fpoint *controlPoints)
{
	context->renderFuncs->fStrokeBezier(context,controlPoints);
};

inline void grStrokeArc(
	RenderContext *context,
	fpoint center, fpoint radius,
	float startTheta, float stopTheta)
{
	context->renderFuncs->fStrokeArc(context,center,radius,startTheta,stopTheta,true);
};

inline void grStrokeEllipse(
	RenderContext *context,
	fpoint center, fpoint radius)
{
	context->renderFuncs->fStrokeEllipse(context,center,radius,true);
};

inline void grInscribeStrokeArc(
	RenderContext *context,
	frect r, float startTheta, float stopTheta)
{
	context->renderFuncs->fInscribeStrokeArc(context,r,startTheta,stopTheta);
};

inline void grInscribeStrokeEllipse(
	RenderContext *context,
	frect r)
{
	context->renderFuncs->fInscribeStrokeEllipse(context,r);
};

inline void grStrokePolygon(
	RenderContext *context,
	fpoint *vertices, int32 vertexCount,
	bool closed)
{
	context->renderFuncs->fStrokePolygon(context,vertices,vertexCount,closed);
};

inline void grStrokePath(
	RenderContext *context, SPath *path)
{
	context->renderFuncs->fStrokePath(context,path);
};

inline void grFillRegion(
	RenderContext *context, region *r)
{
	context->renderFuncs->fFillRegion(context,r);
};

inline void grFillIRect(
	RenderContext *context,
	rect r)
{
	frect fr;
	fr.top = r.top;
	fr.left = r.left;
	fr.bottom = r.bottom;
	fr.right = r.right;
	context->renderFuncs->fFillRect(context,fr);
};

inline void grFillRect(
	RenderContext *context,
	frect rectangle)
{
	context->renderFuncs->fFillRect(context,rectangle);
};

inline void grFillRoundRect(
	RenderContext *context,
	frect rectangle, fpoint radius)
{
	context->renderFuncs->fFillRoundRect(context,rectangle,radius);
};

inline void grFillBezier(
	RenderContext *context,
	fpoint *controlPoints)
{
	context->renderFuncs->fFillBezier(context,controlPoints);
};

inline void grFillArc(
	RenderContext *context,
	fpoint center, fpoint radius,
	float startTheta, float stopTheta)
{
	context->renderFuncs->fFillArc(context,center,radius,startTheta,stopTheta,true);
};

inline void grFillEllipse(
	RenderContext *context,
	fpoint center, fpoint radius)
{
	context->renderFuncs->fFillEllipse(context,center,radius,true);
};

inline void grInscribeFillArc(
	RenderContext *context,
	frect r, float startTheta, float stopTheta)
{
	context->renderFuncs->fInscribeFillArc(context,r,startTheta,stopTheta);
};

inline void grInscribeFillEllipse(
	RenderContext *context,
	frect r)
{
	context->renderFuncs->fInscribeFillEllipse(context,r);
};

inline void grFillPolygon(
	RenderContext *context,
	fpoint *vertices, int32 vertexCount)
{
	context->renderFuncs->fFillPolygon(context,vertices,vertexCount);
};

inline void grFillPath(
	RenderContext *context, SPath *path)
{
	context->renderFuncs->fFillPath(context,path);
};

inline void grDrawString(
	RenderContext *context,
	const uchar *string, float *deltas)
{
	context->renderFuncs->fDrawString(context,string,deltas);
};

inline void grDrawPixels(
	RenderContext *context,
	frect srcRect, frect dstRect,
	Pixels *src)
{
	context->renderFuncs->fDrawPixels(context,srcRect,dstRect,src);
};

inline void grDrawLocPixels(
	RenderContext *context,
	int32 x, int32 y,
	Pixels *src)
{
	frect srcR,dstR;
	srcR.left = srcR.top = 0;
	srcR.right = src->w-1;
	srcR.bottom = src->h-1;
	dstR.left = x;
	dstR.top = y;
	dstR.right = (x+srcR.right);
	dstR.bottom = (y+srcR.bottom);
	context->renderFuncs->fDrawPixels(context,srcR,dstR,src);
};

inline void grDrawIPixels(
	RenderContext *context,
	rect srcRect, rect dstRect,
	Pixels *src)
{
	frect s,d;
	s.top = srcRect.top;
	s.bottom = srcRect.bottom;
	s.left = srcRect.left;
	s.right = srcRect.right;
	d.top = dstRect.top;
	d.bottom = dstRect.bottom;
	d.left = dstRect.left;
	d.right = dstRect.right;
	context->renderFuncs->fDrawPixels(context,s,d,src);
};

#endif
