//******************************************************************************
//
//	File:			render.cpp
//	Description:	General rendering commands
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "accelPackage.h"
#include "edges.h"
#include "bezier.h"
#include "lines.h"
#include "fastmath.h"
#include "renderArc.h"
#include "render.h"
#include "renderLock.h"
#include "rect.h"
#include "fregion.h"
#include "shape.h"

#include <support2/Debug.h>

int32			(*xGetPid)();
ColorMap *		xSystemColorMap;
DebugOutputFunc	xDebugOut;

void grInitRenderLib(ThreadIdFunc func, ColorMap *systemMap, DebugOutputFunc debugOut)
{
	xGetPid = func;
	xSystemColorMap = systemMap;
	xDebugOut = debugOut;
};

void grSetContextPort(RenderContext *context, void *vendor,
	LockRenderPort lock, UnlockRenderPort unlock)
{
	context->portVendor = vendor;
	context->fLockRenderPort = lock;
	context->fUnlockRenderPort = unlock;
	context->renderPort = NULL;
	context->portToken = 0xFFFFFFFF;
	context->cache.canvasToken = 0xFFFFFFFF;
};


RenderPort * NoLock_LockPort(RenderPort *port)
{
	return port;
};

void NoLock_UnlockPort()
{
};

void grSetContextPort_NoLock(RenderContext *context, RenderPort *port)
{
	grSetContextPort(context,port,
		(LockRenderPort)NoLock_LockPort,
		(UnlockRenderPort)NoLock_UnlockPort);
};

void grSetPortCanvas(RenderSubPort *port, void *vendor,
	LockRenderCanvas lock, UnlockRenderCanvas unlock)
{
	port->canvasVendor = vendor;
	port->fLockRenderCanvas = lock;
	port->fUnlockRenderCanvas = unlock;
	port->canvas = NULL;
	port->accelToken = NULL;
	port->hardwarePending = 0;
	port->accelLockCount = 0;
	port->accelLocked = false;
	port->haveSyncToken = false;
	for (int32 i=0;i<12;i++) port->accelSyncToken.opaque[i] = 0;
	port->accelSyncToken.counter = 0;
	port->accelSyncToken.engine_id = 0;
};

RenderCanvas * NoLock_LockCanvas(RenderCanvas *canvas, region *bound, uint32 *token, uint32 *flags)
{
	*flags = 0;
	return canvas;
};

void NoLock_UnlockCanvas(RenderCanvas *canvas, region *bound)
{
};

void grSetPortCanvas_NoLock(RenderPort *port, RenderCanvas *canvas)
{
	grSetPortCanvas(port,canvas,
		(LockRenderCanvas)NoLock_LockCanvas,
		(UnlockRenderCanvas)NoLock_UnlockCanvas);
};

void grSetCanvasAccel(RenderCanvas *canvas, void *vendor,
	LockAccelPackage lock, UnlockAccelPackage unlock)
{
	canvas->accelVendor = vendor;
	canvas->fLockAccelPackage = lock;
	canvas->fUnlockAccelPackage = unlock;
	canvas->accelPackage = NULL;
};

extern 	RenderingPackage renderPackage1;
extern 	RenderingPackage renderPackage8;
extern 	RenderingPackage renderPackage15Little;
extern 	RenderingPackage renderPackage16Little;
extern 	RenderingPackage renderPackage15Big;
extern 	RenderingPackage renderPackage16Big;
extern 	RenderingPackage renderPackage32Little;
extern 	RenderingPackage renderPackage32Big;

RenderingPackage *renderingPackages[2][5] = {
	{
		&renderPackage1,
		&renderPackage8,
		&renderPackage15Little,
		&renderPackage16Little,
		&renderPackage32Little
	},
	{
		&renderPackage1,
		&renderPackage8,
		&renderPackage15Big,
		&renderPackage16Big,
		&renderPackage32Big
	}
};

AccelPackage noAccel = {
	0,
	
	0,
	NULL,
	false,
	{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },

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
	NULL,
	NULL
};

void nop()
{
};

void grCopyRegion(region ** src, region ** dst)
{
	if (*src) {
		if (!*dst)
			*dst = newregion();
		copy_region(*src,*dst);
	} else {
		if (*dst) kill_region(*dst);
		*dst = NULL;
	};
};

void grCopyRegion(fregion ** src, fregion ** dst)
{
	if (*src) {
		if (!*dst) *dst = new fregion(*src);
		else (*dst)->Copy(*src);
	} else {
		if (*dst) {
			delete *dst;
			*dst = NULL;
		};
	};
};

#define SPAN_BATCH 32
#define RECT_BATCH 256

static const rect fakerect(0,0,0x70000000,0x70000000);
static region fakeregion(fakerect);

struct span { int32 top,left,right; };

void grFillSpansSoftware(
	RenderContext *context,
	RenderSubPort *port,
	int32 *fillSpans, int32 spanCount)
{
	grAssertLocked(context, port);

	RenderCache *cache = port->cache;
	region *clipRegion = port->RealDrawingRegion();
	const rect *clipRects = clipRegion->Rects();
	const int32 clipCount = clipRegion->CountRects();
	port->SetRealDrawingRegion(&fakeregion);

	rect fillRects[RECT_BATCH];
	rect *curRect = fillRects;
	int32 rectCount = 0;
	int32 right,index,y;

	const point save_offset = port->origin;

#if ROTATE_DISPLAY
	point offset;
	offset.x = port->rotater->RotateDV(port->origin.y);
	offset.y = port->rotater->RotateDH(port->origin.x);
#else
	const point offset = port->origin;
#endif

	port->origin.x = 0;
	port->origin.y = 0;
	
	span *p2 = (span*)fillSpans;
	span *end2 = p2+spanCount;

	// Find the *first* rect in the clipping region that contains
	// the first visible span of the spans array...
	index = -1;
	const int32 spanTop = p2->top + offset.y;
	const int32 spanBottom = (end2-1)->top + offset.y;
	if ((clipRegion->Bounds().bottom >= spanTop) && (clipRegion->Bounds().top <= spanBottom))
	{ // The span is intersecting the region
		for (int32 i=0 ; i<clipCount ; i++)
		{ // find the first rect that intersects the spans
			const int32 clipTop = clipRects[i].top;
			const int32 clipBottom = clipRects[i].bottom;
			if ((clipBottom >= spanTop) && (clipTop <= spanBottom))
			{ // this is the _first_ rect that intersects the spans
				index = i;
				break;
			}
		}
	}

	if (index != -1) {
		const rect *rowBegin1 = clipRects + index;
		const rect *end1 = clipRects+clipCount;
		const rect *rowEnd1 = rowBegin1;
		const rect *p1;

		while ((rowEnd1 < end1) && (rowBegin1->top == rowEnd1->top)) rowEnd1++;
		
		while (1) {
			restartRow:
			if (rowBegin1->top > (p2->top+offset.y)) {
				if ((++p2) >= end2) goto doneSpans;
			} else if (rowBegin1->bottom < (p2->top+offset.y)) {
				if ((rowBegin1=rowEnd1) >= end1) goto doneSpans;
				while ((rowEnd1 < end1) && (rowBegin1->top == rowEnd1->top)) rowEnd1++;
			} else {
				p1 = rowBegin1;
				y = (p2->top+offset.y);
				while (1) {
					if (p1->right < (p2->left+offset.x)) {
						if ((++p1) >= rowEnd1) goto doneRow;
					} else if ((p2->right+offset.x) < p1->left) {
						if ((++p2) >= end2) goto doneSpans;
						if ((p2->top+offset.y) != y) goto restartRow;
					} else {
						curRect->top = curRect->bottom = p2->top + offset.y;
						curRect->left = p1->left;
						if ((p2->left+offset.x) > curRect->left) curRect->left = p2->left + offset.x;
						right = p1->right;
						if ((p2->right+offset.x) < right) right = p2->right + offset.x;
						curRect->right = right;
						curRect++;
						if (++rectCount == RECT_BATCH) {
							cache->fFillRects(context,port,fillRects,rectCount);
							curRect = fillRects;
							rectCount = 0;
						};
						if (p1->right == right)	{
							if ((++p1) >= rowEnd1) goto doneRow;
						};
						if ((p2->right+offset.x) == right)	{
							if ((++p2) >= end2) goto doneSpans;
							if ((p2->top+offset.y) != y) goto restartRow;
						};
					};
				};
				
				continue;

				doneRow:
				do { if ((++p2) >= end2) goto doneSpans; } while ((p2->top+offset.y) == y);
			};
		};
	}
	doneSpans:
	if (rectCount)
		cache->fFillRects(context,port,fillRects,rectCount);
	port->SetRealDrawingRegion(clipRegion);
	port->origin = save_offset;
};

void grFillSpansHardware(
	RenderContext *context,
	RenderSubPort *port,
	int32 *fillSpans, int32 spanCount)
{
	RenderCache *	cache = port->cache;
	RenderCanvas *	canvas = port->canvas;
	int32			clips,y,hardwareSpanCount,right,index;
	uint32			color;
	int16			drawOp = context->drawOp;
	uint16			hardwareSpans[SPAN_BATCH*3],*thisHardwareSpan;
	region *		clipRegion = port->RealDrawingRegion();

#if ROTATE_DISPLAY
	point offset;
	offset.x = port->rotater->RotateDV(port->origin.y);
	offset.y = port->rotater->RotateDH(port->origin.x);
#else
	const point offset = port->origin;
#endif

	grAssertLocked(context, port);
	
	/*	Spans are sorted in increasing Y.  Clipping rects are sorted in increasing Y
		of the top of each rectangle.  Let's take advanatage of this when clipping. */

	fill_span hardwareSpan = canvas->accelPackage->spanFill;
	engine_token *token = grLockAccel(context,port,canvas);

#if AS_DEBUG
	if (hardwareSpan == NULL) {
		_sPrintf("*** grFillSpansHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("about to call the NULL ptr! Call Mathias immediately!");
		return;
	}
	if ((port->hardwarePending&hwSpanFillOp) == 0) {
		_sPrintf("*** grFillSpansHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("hwSpanFill not set in hardwarePending!");
		return;
	}
#endif
	
	color = (drawOp == OP_COPY) ? *(&cache->foreColorCanvasFormat + context->solidColor)
								: ((context->drawOp==OP_OVER) 	? cache->foreColorCanvasFormat
																: cache->backColorCanvasFormat);

	hardwareSpanCount = 0;
	thisHardwareSpan = hardwareSpans;

	span *p2 = (span*)fillSpans;
	span *end2 = p2+spanCount;

	// Find the *first* rect in the clipping region that contains
	// the first visible span of the spans array...
	index = -1;
	const int32 spanTop = p2->top + offset.y;
	const int32 spanBottom = (end2-1)->top + offset.y;
	const int32 clipCount = clipRegion->CountRects();
	const rect* clipRects = clipRegion->Rects();
	if ((clipRegion->Bounds().bottom >= spanTop) && (clipRegion->Bounds().top <= spanBottom))
	{ // The span is intersecting the region
		for (int32 i=0 ; i<clipCount ; i++)
		{ // find the first rect that intersects the spans
			const int32 clipTop = clipRects[i].top;
			const int32 clipBottom = clipRects[i].bottom;
			if ((clipBottom >= spanTop) && (clipTop <= spanBottom))
			{ // this is the _first_ rect that intersects the spans
				index = i;
				break;
			}
		}
	}

	if (index != -1) {
		const rect *rowBegin1 = clipRects + index;
		const rect *end1 = clipRects+clipCount;
		const rect *rowEnd1 = rowBegin1;
		const rect *p1;

		while ((rowEnd1 < end1) && (rowBegin1->top == rowEnd1->top)) rowEnd1++;
		
		while (1) {
			restartRow:
			if (rowBegin1->top > (p2->top+offset.y)) {
				if ((++p2) >= end2) goto doneSpans;
			} else if (rowBegin1->bottom < (p2->top+offset.y)) {
				if ((rowBegin1=rowEnd1) >= end1) goto doneSpans;
				while ((rowEnd1 < end1) && (rowBegin1->top == rowEnd1->top)) rowEnd1++;
			} else {
				p1 = rowBegin1;
				y = (p2->top+offset.y);
				while (1) {
					if (p1->right < (p2->left+offset.x)) {
						if ((++p1) >= rowEnd1) goto doneRow;
					} else if ((p2->right+offset.x) < p1->left) {
						if ((++p2) >= end2) goto doneSpans;
						if ((p2->top+offset.y) != y) goto restartRow;
					} else {
						thisHardwareSpan[0] = (p2->top+offset.y);
						thisHardwareSpan[1] = p1->left;
						if ((p2->left+offset.x) > thisHardwareSpan[1]) thisHardwareSpan[1] = (p2->left+offset.x);
						right = p1->right;
						if ((p2->right+offset.x) < right) right = (p2->right+offset.x);
						thisHardwareSpan[2] = right;
						thisHardwareSpan += 3;
						hardwareSpanCount++;
						if (hardwareSpanCount == SPAN_BATCH) {
							hardwareSpan(token,color,hardwareSpans,hardwareSpanCount);
							thisHardwareSpan = hardwareSpans;
							hardwareSpanCount = 0;
						};
						if (p1->right == right)	{
							if ((++p1) >= rowEnd1) goto doneRow;
						};
						if ((p2->right+offset.x) == right)	{
							if ((++p2) >= end2) goto doneSpans;
							if ((p2->top+offset.y) != y) goto restartRow;
						};
					};
				};
				
				continue;

				doneRow:
				do { if ((++p2) >= end2) goto doneSpans; } while ((p2->top+offset.y) == y);
			};
		};
	}

	doneSpans:

	if (hardwareSpanCount) hardwareSpan(token,color,hardwareSpans,hardwareSpanCount);
	grUnlockAccel(context,port,canvas);
};

void grFillRectsHardware(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	RenderCache *		cache = port->cache;
	RenderCanvas *		canvas = port->canvas;
	int32				clips,i,left,right,top,bottom;
	uint32				color;
	const rect			*curSrcRect=fillRects,*curClipRect;
	int16				drawOp = context->drawOp;
	fill_rect_params	rects[RECT_BATCH],*thisRect;
	const rect*			clipRects = port->RealDrawingRegion()->Rects();
	const int32			clipCount = port->RealDrawingRegion()->CountRects();

#if ROTATE_DISPLAY
	point offset;
	offset.x = port->rotater->RotateDV(port->origin.y);
	offset.y = port->rotater->RotateDH(port->origin.x);
#else
	const point offset = port->origin;
#endif

	grAssertLocked(context, port);
	
	fill_rectangle hardwareRect = canvas->accelPackage->rectFill;
	engine_token *token = grLockAccel(context,port,canvas);

#if AS_DEBUG
	if (hardwareRect == NULL) {
		_sPrintf("*** grFillRectsHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("about to call the NULL ptr! Call Mathias immediately!");
		return;
	}
	if ((port->hardwarePending&hwRectFillOp) == 0) {
		_sPrintf("*** grFillRectsHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("hwRectFill not set in hardwarePending!");
		return;
	}
#endif
	
	color = (drawOp == OP_COPY) ? *(&cache->foreColorCanvasFormat + context->solidColor)
								: ((context->drawOp==OP_OVER) 	? cache->foreColorCanvasFormat
																: cache->backColorCanvasFormat);

	i = 0;
	thisRect = rects;
	while (rectCount) {
		curClipRect = clipRects;
		clips = clipCount;
		while (clips) {
			top = curSrcRect->top + offset.y;
			if (curClipRect->top > top) top = curClipRect->top;
			bottom = curSrcRect->bottom + offset.y;
			if (curClipRect->bottom < bottom) bottom = curClipRect->bottom;
			left = curSrcRect->left + offset.x;
			if (curClipRect->left > left) left = curClipRect->left;
			right = curSrcRect->right + offset.x;
			if (curClipRect->right < right) right = curClipRect->right;
			clips--;
			curClipRect++;
			if ((left <= right) && (top <= bottom)) {
				thisRect->left = left;
				thisRect->right = right;
				thisRect->top = top;
				thisRect->bottom = bottom;
				thisRect++;
				if (++i == RECT_BATCH) {
					hardwareRect(token,color,rects,i);
					thisRect = rects; i = 0;
				};
			};
		};
		rectCount--;
		curSrcRect++;
	};

	if (i) hardwareRect(token,color,rects,i);
	grUnlockAccel(context,port,canvas);
};

void grInvertRectsHardware(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	RenderCache *		cache = port->cache;
	RenderCanvas *		canvas = port->canvas;
	int32				clips,i,left,right,top,bottom;
	uint32				color;
	const rect			*curSrcRect=fillRects,*curClipRect;
	int16				drawOp = context->drawOp;
	fill_rect_params	rects[RECT_BATCH],*thisRect;
	const rect*			clipRects = port->RealDrawingRegion()->Rects();
	const int32			clipCount = port->RealDrawingRegion()->CountRects();

#if ROTATE_DISPLAY
	point offset;
	offset.x = port->rotater->RotateDV(port->origin.y);
	offset.y = port->rotater->RotateDH(port->origin.x);
#else
	const point offset = port->origin;
#endif

	grAssertLocked(context, port);
	
	invert_rectangle hardwareRect = canvas->accelPackage->rectInvert;
	engine_token *token = grLockAccel(context,port,canvas);

#if AS_DEBUG
	if (hardwareRect == NULL) {
		_sPrintf("*** grInvertRectsHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("about to call the NULL ptr! Call Mathias immediately!");
		return;
	}
	if ((port->hardwarePending&hwRectFillOp) == 0) {
		_sPrintf("*** grInvertRectsHardware: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, canvas, canvas->accelPackage);
		debugger("hwRectFill not set in hardwarePending!");
		return;
	}
#endif
	
	i = 0;
	thisRect = rects;
	while (rectCount) {
		curClipRect = clipRects;
		clips = clipCount;
		while (clips) {
			top = curSrcRect->top + offset.y;
			if (curClipRect->top > top) top = curClipRect->top;
			bottom = curSrcRect->bottom + offset.y;
			if (curClipRect->bottom < bottom) bottom = curClipRect->bottom;
			left = curSrcRect->left + offset.x;
			if (curClipRect->left > left) left = curClipRect->left;
			right = curSrcRect->right + offset.x;
			if (curClipRect->right < right) right = curClipRect->right;
			clips--;
			curClipRect++;
			if ((left <= right) && (top <= bottom)) {
				thisRect->left = left;
				thisRect->right = right;
				thisRect->top = top;
				thisRect->bottom = bottom;
				thisRect++;
				if (++i == RECT_BATCH) {
					hardwareRect(token,rects,i);
					thisRect = rects; i = 0;
				};
			};
		};
		rectCount--;
		curSrcRect++;
	};

	if (i) hardwareRect(token,rects,i);
	grUnlockAccel(context,port,canvas);
};

void grFillRects(
	RenderContext *context,
	RenderPort *ueberPort,
	rect *fillRects, int32 rectCount)
{
	RenderSubPort *port = ueberPort;
	while (port) {
		port->cache->fFillRects(context,port,fillRects,rectCount);
		port = port->next;
	}
}

void grFillSpans(
	RenderContext *context,
	RenderPort *ueberPort,
	int32 *fillSpans, int32 spanCount)
{
	RenderSubPort *port = ueberPort;
	while (port) {
		port->cache->fFillSpans(context,port,fillSpans,spanCount);
		port = port->next;
	};
}

void grFillPath_Internal(
	RenderContext *context, SPath *shape, bool translate, bool includeEdgePixels)
{
	RenderPort *port = context->renderPort;

	fpoint origin;
	if (translate)	origin = context->totalEffective_Origin;
	else			origin = fpoint(0,0);

	rect b1 = port->drawingBounds;
	frect fb2 = shape->Bounds();
	rect b2;
	b2.left = (int32)(fb2.left + origin.x + 0.49995);
	b2.top = (int32)(fb2.top + origin.y + 0.49995);
	b2.right = (int32)(origin.x + fb2.right);
	b2.bottom = (int32)(origin.y + fb2.bottom);

	if (b1.left < b2.left) b1.left = b2.left;
	if (b1.top < b2.top) b1.top = b2.top;
	if (b1.right > b2.right) b1.right = b2.right;
	if (b1.bottom > b2.bottom) b1.bottom = b2.bottom;

	if ((b1.left > b1.right) || (b1.top > b1.bottom)) return;

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotateRect(&b1, &b1);
		const int32 count = shape->Points()->CountItems();
		for (int32 i=0 ; i<count ; i++) {
			fpoint& point = (*shape->Points())[i];
			port->rotater->RotatePoint(&point, &point);
		}
	}
#endif


	SEdges edges(shape->Points()->CountItems(),&b1,includeEdgePixels,origin);
	
	if (shape->Transfer(&edges) == B_OK) {
		grLockCanvi(port,&b1,hwSpanFill);
			edges.ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
		grUnlockCanvi(port);
	}
}

void grFillPolygon_Internal(
	RenderContext *context, fpoint *vertices, int32 vCount, bool translate, bool includeEdgePixels)
{	
	RenderPort *port = grLockPort(context);

	fpoint origin;
	if (translate)	origin = context->totalEffective_Origin;
	else 			origin = fpoint(0,0);

	frect fb2;
	fb2.right = fb2.left = vertices[0].x;
	fb2.bottom = fb2.top = -vertices[0].y;
	for (int32 i=0;i<vCount;i++) {
		vertices[i].x += origin.x;
		vertices[i].y += origin.y;
		if (vertices[i].x < fb2.left) fb2.left = vertices[i].x;
		if (vertices[i].x > fb2.right) fb2.right = vertices[i].x;
		if (vertices[i].y < fb2.top) fb2.top = vertices[i].y;
		if (vertices[i].y > fb2.bottom) fb2.bottom = vertices[i].y;
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			port->rotater->RotatePoint(vertices+i, vertices+i);
#endif
	}

	rect b1 = port->drawingBounds;
	rect b2;
	b2.left = (int32)(fb2.left + 0.49995);
	b2.top = (int32)(fb2.top + 0.49995);
	b2.right = (int32)(fb2.right);
	b2.bottom = (int32)(fb2.bottom);
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotateRect(&b1, &b1);
		port->rotater->RotateRect(&b2, &b2);
	}
#endif

	if (b1.left < b2.left) b1.left = b2.left;
	if (b1.top < b2.top) b1.top = b2.top;
	if (b1.right > b2.right) b1.right = b2.right;
	if (b1.bottom > b2.bottom) b1.bottom = b2.bottom;

	if ((b1.left > b1.right) || (b1.top > b1.bottom)) {
		grUnlockPort(context);
		return;
	}

	SEdges edges(vCount,&b1,includeEdgePixels);
	if (edges.AddEdges(vertices,vCount) == B_OK) {
		grLockCanvi(port,NULL,hwSpanFill);
			edges.ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
		grUnlockCanvi(port);
	}

	grUnlockPort(context);
}

/********************* Public API ***********************/

void grStrokeRect_Fat(
	RenderContext *context,
	frect r)
{
	RDebugPrintf((context,"grStrokeRect_Fat(%f,%f,%f,%f)\n",r.left,r.top,r.right,r.bottom));

	/*	This may look like a hack, but it isn't, really.
		This routine will only get selected by the pick
		function if we are doing only scaling for our
		linear transform.  Therefore, this is perfectly
		safe and will have the desired results. */
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	rect outer,inner;
	float f;
	
	if (r.top > r.bottom) {
		f = r.bottom;
		r.bottom = r.top;
		r.top = f;
	};
	if (r.left > r.right) {
		f = r.left;
		r.left = r.right;
		r.right = f;
	};

	RenderPort *port = grLockPort(context);
	
	r.left += context->totalEffective_Origin.x;
	r.right += context->totalEffective_Origin.x;
	r.top += context->totalEffective_Origin.y;
	r.bottom += context->totalEffective_Origin.y;
	
	float halfPen = context->totalEffective_PenSize*0.5;
	outer.left = (int32)(r.left-halfPen); 	outer.left++;
	inner.right = (int32)(r.right-halfPen);	inner.right++;
	inner.left = (int32)(r.left-halfPen+context->totalEffective_PenSize);
	outer.right = (int32)(r.right-halfPen+context->totalEffective_PenSize);
	outer.top = (int32)(r.top-halfPen); 	outer.top++;
	inner.bottom = (int32)(r.bottom-halfPen);	inner.bottom++;
	inner.top = (int32)(r.top-halfPen+context->totalEffective_PenSize);
	outer.bottom = (int32)(r.bottom-halfPen+context->totalEffective_PenSize);

	int32 rectCount=0;
	rect rl[4];
	
	if ((inner.left >= inner.right-1) || (inner.top >= inner.bottom-1)) {
		rl[0] = outer;
		rectCount = 1;
	} else {
		rl[0].left = outer.left;
		rl[0].right = outer.right;
		rl[0].top = outer.top;
		rl[0].bottom = inner.top;
		rl[3].left = outer.left;
		rl[3].right = outer.right;
		rl[3].top = inner.bottom;
		rl[3].bottom = outer.bottom;
		rl[1].left = outer.left;
		rl[1].right = inner.left;
		rl[1].top = inner.top+1;
		rl[1].bottom = inner.bottom-1;
		rl[2].left = inner.right;
		rl[2].right = outer.right;
		rl[2].top = inner.top+1;
		rl[2].bottom = inner.bottom-1;
		rectCount = 4;
#if ROTATE_DISPLAY
		if (port->canvasIsRotated) {
			port->rotater->RotateRect(rl+1, rl+1);
			port->rotater->RotateRect(rl+2, rl+2);
			port->rotater->RotateRect(rl+3, rl+3);
		}
#endif
	};

#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(rl, rl);
#endif

	grLockCanvi(port,&outer,hwRectFill);
		grFillRects(context,port,rl,rectCount);
	grUnlockCanvi(port);

	grUnlockPort(context);
};

void grStrokeRect_Thin(
	RenderContext *context,
	frect r)
{
	RDebugPrintf((context,"grStrokeRect_Thin(%f,%f,%f,%f)\n",r.left,r.top,r.right,r.bottom));

	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	rect ri[4];
	int32 x1,x2,y1,y2,tmp;
	fpoint origin = context->totalEffective_Origin;
	x1 = (int32)floor(r.left + origin.x + 0.5);
	x2 = (int32)floor(r.right + origin.x + 0.5);
	y1 = (int32)floor(r.top + origin.y + 0.5);
	y2 = (int32)floor(r.bottom + origin.y + 0.5);
	if (x2 < x1) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	};
	if (y2 < y1) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	};

	RenderPort *port = grLockPort(context);
		grLockCanvi(port,&port->drawingBounds,hwRectFill);
			ri[0].left =
			ri[1].left = x1;
			ri[0].right =
			ri[1].right = x2;
			ri[0].top = ri[0].bottom = y1;
			ri[1].top = ri[1].bottom = y2;
			ri[3].top =
			ri[2].top = y1+1;
			ri[3].bottom =
			ri[2].bottom = y2-1;
			tmp = 2;
#if ROTATE_DISPLAY
			if (port->canvasIsRotated) {
				port->rotater->RotateRect(ri, ri);
				port->rotater->RotateRect(ri+1, ri+1);
			}
#endif
			if (ri[3].top <= ri[3].bottom) {
				ri[3].left = ri[3].right = x1;
				ri[2].left = ri[2].right = x2;
				tmp = 4;
#if ROTATE_DISPLAY
				if (port->canvasIsRotated) {
					port->rotater->RotateRect(ri+2, ri+2);
					port->rotater->RotateRect(ri+3, ri+3);
				}
#endif
			};
			grFillRects(context,port,ri,tmp);
		grUnlockCanvi(port);
	grUnlockPort(context);
};

/********************************************************/

void grStrokeBezier_Fat(
	RenderContext *context,
	fpoint *controlPoints)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,controlPoints,4);
	
	context->lineThickener->Clear();
	grLockPort(context);
	context->lineThickener->UpdateClipBounds();
	context->lineThickener->OpenShapes(controlPoints[0]);
	context->lineThickener->AddBezierSegments(controlPoints+1,1);
	context->lineThickener->CloseShapes(false);
	grFillPath_Internal(context,context->lineThickener,true,false);
	grUnlockPort(context);
};

void grStrokeBezier_Thin(
	RenderContext *context,
	fpoint *controlPoints)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,controlPoints,4);
	
	float ps;
	frect r;
	rect ri;
	BArray<fpoint> curve;
	RenderPort *port = grLockPort(context);
		ri = port->drawingBounds;
		r.left = -1;
		r.right = ri.right - ri.left + 1;
		r.top = -1;
		r.bottom = ri.bottom - ri.top + 1;
		curve.AddItem(controlPoints[0]);
		TesselateClippedCubicBezier(controlPoints,curve,r,0.4);
		grLockCanvi(port,&port->drawingBounds,hwLine);
			grDrawManyLines(context,curve.Items(),curve.CountItems(),
				context->totalEffective_Origin,false,NULL);
		grUnlockCanvi(port);
	grUnlockPort(context);
};

/********************************************************/

void grStrokePolygon_Fat(
	RenderContext *context,
	fpoint *vertices, int32 numVertices,
	bool closed)
{	
	#if RENDER_DEBUG
		RDebugPrintf((context,"grStrokePolygon_Fat() --> "));
		for (int32 i=0;i<numVertices;i++) {
			RDebugPrintf((context,"(%f,%f), ",vertices[i].x,vertices[i].y));
		};
		RDebugPrintf((context,"\n"));
	#endif

	if (context->fCoordTransform)
		context->fCoordTransform(context,vertices,numVertices);

	context->lineThickener->Clear();
	context->lineThickener->OpenShapes(vertices[0]);
	context->lineThickener->AddLineSegments(vertices+1,numVertices-1);
	context->lineThickener->CloseShapes(closed);
	grLockPort(context);
	grFillPath_Internal(context,context->lineThickener,true,false);
	grUnlockPort(context);
};

void grStrokePolygon_Thin(
	RenderContext *context,
	fpoint *vertices, int32 numVertices,
	bool closed)
{	
	#if RENDER_DEBUG
		RDebugPrintf((context,"grStrokePolygon_Thin() --> "));
		for (int32 i=0;i<numVertices;i++) {
			RDebugPrintf((context,"(%f,%f), ",vertices[i].x,vertices[i].y));
		};
		RDebugPrintf((context,"\n"));
	#endif

	if (context->fCoordTransform)
		context->fCoordTransform(context,vertices,numVertices);

	RenderPort *port = grLockPort(context);
		grLockCanvi(port,&port->drawingBounds,hwLine);
			grDrawManyLines(context,vertices,numVertices,
				context->totalEffective_Origin,closed,NULL);
		grUnlockCanvi(port);
	grUnlockPort(context);
};

/********************************************************/

void grFillRegion_FatThin(
	RenderContext *context,
	region *reg)
{
	RenderPort *port = grLockPort(context);
		grLockCanvi(port,&port->drawingBounds,hwRectFill);
			fpoint origin = context->totalEffective_Origin;
			const int32 N = reg->CountRects();
			const rect* clipRects = reg->Rects();
			if (context->fCoordTransform) {
				float f[4];
				rect r;
				for (int32 i=0;i<N;i++) {
					r = clipRects[i];
					f[0] = r.top-0.5;
					f[1] = r.left-0.5;
					f[2] = (r.bottom+1)-0.5;
					f[3] = (r.right+1)-0.5;
					context->fCoordTransform(context,(fpoint*)f,2);
					r.top = (int32)floor(f[0] + 1.0 + origin.y);
					r.left = (int32)floor(f[1] + 1.0 + origin.x);
					r.bottom = (int32)floor(f[2] + origin.y);
					r.right = (int32)floor(f[3] + origin.x);
#if ROTATE_DISPLAY
					if (port->canvasIsRotated)
						port->rotater->RotateRect(&r, &r);
#endif
					grFillRects(context,port,&r,1);
				};
			} else {
				int32 ox = (int32)floor(origin.x + 0.5);
				int32 oy = (int32)floor(origin.y + 0.5);
				if (ox || oy) {
					offset_region(reg,ox,oy);
					clipRects = reg->Rects();
				}				

#if ROTATE_DISPLAY
				IRegion r;
				reg->Rotate(*(port->rotater), &r);
				grFillRects(context,port,(rect*)r.Rects(),r.CountRects());
#else
				grFillRects(context,port,(rect*)clipRects,N);
#endif

				if (ox || oy) offset_region(reg,-ox,-oy);
			};
		grUnlockCanvi(port);
	grUnlockPort(context);
};

/********************************************************/

void grFillRect_FatThin(
	RenderContext *context,
	frect fr)
{
	RDebugPrintf((context,"grFillRect_FatThin(%f,%f,%f,%f)\n",fr.left,fr.top,fr.right,fr.bottom));

	rect r;
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&fr,2);

	fpoint origin = context->totalEffective_Origin;
	r.left = (int32)floor(fr.left + origin.x + 0.5);
	r.top = (int32)floor(fr.top + origin.y + 0.5);
	r.right = (int32)floor(fr.right + origin.x + 0.5);
	r.bottom = (int32)floor(fr.bottom + origin.y + 0.5);

	RenderPort *port = grLockPort(context);
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			port->rotater->RotateRect(&r, &r);
#endif
		grLockCanvi(port,NULL,hwRectFill);
			grFillRects(context,port,&r,1);
		grUnlockCanvi(port);
	grUnlockPort(context);
};

/********************************************************/

void grFillBezier_FatThin(
	RenderContext *context,
	fpoint *controlPoints)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,controlPoints,4);
	
	RenderPort *port = grLockPort(context);
		fpoint origin = context->totalEffective_Origin;
		for (int32 i=0;i<4;i++) {
			controlPoints[i] += origin;
		}

		rect ri = port->drawingBounds;
		frect r;
		r.left = ri.left;
		r.right = ri.right;
		r.top = ri.top;
		r.bottom = ri.bottom;

		BArray<fpoint> curve;
		TesselateClippedCubicBezier(controlPoints,curve,r,0.4);
		grFillPolygon_Internal(context,curve.Items(),curve.CountItems(),false,true);
	grUnlockPort(context);
};

/********************************************************/

void grFillPolygon_FatThin(
	RenderContext *context,
	fpoint *vertices, int32 numVertices)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,vertices,numVertices);
	
	grFillPolygon_Internal(context,vertices,numVertices,true,true);
};

/********************************************************/

void grStrokePath_Thin(RenderContext *context, SPath *path)
{
	float penx = context->penLocation.x;
	float peny = context->penLocation.y;
	int32 count = path->Points()->CountItems();
	for (int32 i=0;i<count;i++) {
		(*path->Points())[i].x += penx;
		(*path->Points())[i].y += peny;
	};

	if (context->fCoordTransform)
		context->fCoordTransform(
			context,
			path->Points()->Items(),
			path->Points()->CountItems());
		
	SThinLineRenderer approx(context);
	path->Transfer(&approx);
};

void grStrokePath_Fat(RenderContext *context, SPath *path)
{
	float penx = context->penLocation.x;
	float peny = context->penLocation.y;
	int32 count = path->Points()->CountItems();
	for (int32 i=0;i<count;i++) {
		(*path->Points())[i].x += penx;
		(*path->Points())[i].y += peny;
	};

	if (context->fCoordTransform)
		context->fCoordTransform(
			context,
			path->Points()->Items(),
			path->Points()->CountItems());

	context->lineThickener->Clear();
	grLockPort(context);
	context->lineThickener->UpdateClipBounds();
	path->Transfer(context->lineThickener);
	grFillPath_Internal(context,context->lineThickener,true,false);
	grUnlockPort(context);
};

/********************************************************/

void grFillPath_FatThin(RenderContext *context, SPath *path)
{
	float penx = context->penLocation.x;
	float peny = context->penLocation.y;
	int32 count = path->Points()->CountItems();
	for (int32 i=0;i<count;i++) {
		(*path->Points())[i].x += penx;
		(*path->Points())[i].y += peny;
	};

	if (context->fCoordTransform)
		context->fCoordTransform(
			context,
			path->Points()->Items(),
			path->Points()->CountItems());

	grLockPort(context);
	grFillPath_Internal(context,path,true,false);
	grUnlockPort(context);
};

/********************************************************/
/* Generally useful functions and constants */

void grContextDebugOutput(RenderContext *context)
{
};

void grPortDebugOutput(RenderPort *port)
{
};

void grCanvasDebugOutput(RenderCanvas *canvas)
{
};

#if RENDER_DEBUG

void RDebugPrintfReal(RenderContext *context, const char *format, ...)
{
	if ((!context) || !(context->stateFlags & RENDER_DEBUG_PRINTING)) return;

	char str[2048],*p=str;

	va_list args;
	va_start(args, format);
	RenderState *theState = context->stateStack;
	while (theState) {
		*p++ = ' ';
		*p++ = ' ';
		*p++ = ' ';
		*p++ = ' ';
		theState = theState->next;
	};
	vsprintf(p,format,args);
	if (xDebugOut) xDebugOut(str);
	else {
		_sPrintf("xDebugOut not set! : %s\n",str);
	};
	va_end (args);
};

void RDebugDumpStateReal(RenderContext *context)
{
};

#endif

void grSetDebugPrinting(RenderContext *context, bool on)
{
	if (on) {
		context->stateFlags |= RENDER_DEBUG_PRINTING;
		RDebugPrintf((context,"Turning on debug printing\n"));
	} else {
		RDebugPrintf((context,"Turning off debug printing\n"));
		context->stateFlags &= ~RENDER_DEBUG_PRINTING;
	};

};

int32 grPixelFormat2BitsPerPixel(int32 format)
{
	static const int32 format_2_bpp[5] = { 1,8,16,16,32 };
	if ((format < 0) || (format >= NUM_PIXEL_FORMATS))
		return -1;
	return format_2_bpp[format];
};

const rgb_color rgb_white(255, 255, 255, 255);
const rgb_color rgb_black(0, 0, 0, 255);
const rgb_color rgb_dk_gray(78, 78, 78, 255);
const rgb_color rgb_gray(158, 158, 158, 255);

pattern usefulPatterns[] = {
	{{	// Black
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff
	}},
	{{	// Gray
		0xaa, 0x55, 0xaa, 0x55,
		0xaa, 0x55, 0xaa, 0x55
	}}
};

RenderFuncs thinPenRenderFuncs = {
	grStrokeLineArray_Thin,
	grStrokeLine_Thin,
	grFillRegion_FatThin,

	grStrokeRect_Thin,
	grFillRect_FatThin,

	grStrokeRoundRect_Thin,
	grFillRoundRect_FatThin,

	grStrokeBezier_Thin,
	grFillBezier_FatThin,

	grStrokePolygon_Thin,
	grFillPolygon_FatThin,

	grStrokePath_Thin,
	grFillPath_FatThin,

	grStrokeArc_Thin,
	grFillArc_FatThin,

	grInscribeStrokeArc_FatThin,
	grInscribeFillArc_FatThin,

	grStrokeEllipse_Thin,
	grFillEllipse_FatThin,

	grInscribeStrokeEllipse_FatThin,
	grInscribeFillEllipse_FatThin,

	grDrawString_FatThin,
	grDrawPixels_FatThin
};

RenderFuncs fatPenRenderFuncs = {
	grStrokeLineArray_Fat,
	grStrokeLine_Fat,
	grFillRegion_FatThin,

	grStrokeRect_Fat,
	grFillRect_FatThin,

	grStrokeRoundRect_Fat,
	grFillRoundRect_FatThin,

	grStrokeBezier_Fat,
	grFillBezier_FatThin,

	grStrokePolygon_Fat,
	grFillPolygon_FatThin,

	grStrokePath_Fat,
	grFillPath_FatThin,

	grStrokeArc_Fat,
	grFillArc_FatThin,

	grInscribeStrokeArc_FatThin,
	grInscribeFillArc_FatThin,

	grStrokeEllipse_Fat,
	grFillEllipse_FatThin,

	grInscribeStrokeEllipse_FatThin,
	grInscribeFillEllipse_FatThin,

	grDrawString_FatThin,
	grDrawPixels_FatThin
};
