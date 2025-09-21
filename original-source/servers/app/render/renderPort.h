
//******************************************************************************
//
//	File:			renderPort.h
//	Description:	Structures and functions for dealing with rendering ports
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERPORT_H
#define RENDERPORT_H

#include <Accelerant.h>
#include "basic_types.h"
#include "display_rotation.h"

/*	This is the region on the canvas into which we are rendering.  A RenderPort is simply
	a region of pixels which can be locked such that this region does not change, plus an
	integral origin which everything rendered into it is translated by.  A view can be seen
	as a merger of a RenderContext and a RenderPort.  Note that the RenderPort's region should
	be considered volatile unless it is locked.  Note also that the region is in a coordinate
	system such that (0,0) is the upper left corner of the canvas. */

struct RenderCanvas;
typedef RenderCanvas *	(*LockRenderCanvas)(void *vendor, region *willChange, uint32 *token, uint32 *flags);
typedef void 			(*UnlockRenderCanvas)(void *vendor, region *doneChanging, sync_token *token);

struct RenderSubPort {
	/* The vendor is responsible for filling these in -----*/
	RenderSubPort *			next;
	point					origin;
	region *				portClip;
	uint32					infoFlags;

	/* Context cache --------------------------------------*/
	RenderCache *			cache;
	region *				drawingClip;
	int16					phase_x,phase_y;

	/* Buying a canvas ------------------------------------*/
	void *					canvasVendor;
	engine_token *			accelToken;
	region *				touching;
	uint32					hardwarePending;
	int32					accelLockCount;
	bool					accelLocked;
	bool					haveSyncToken;
	sync_token				accelSyncToken;
	LockRenderCanvas		fLockRenderCanvas;
	UnlockRenderCanvas		fUnlockRenderCanvas;
	RenderCanvas *			canvas;

/* Extension to handle software display rotation by 90 degres. This contains the cached
   version of the rotated drawingClip */
#if ROTATE_DISPLAY
	region *				rotatedDrawingClip;
	DisplayRotater *		rotater;
	bool					canvasIsRotated;
	inline void				SwapDrawingRegions();
#endif		
	inline region *			RealDrawingRegion();
	inline void				SetRealDrawingRegion(region *reg);
};


#if ROTATE_DISPLAY

inline region *RenderSubPort::RealDrawingRegion() {
	return (canvasIsRotated) ? rotatedDrawingClip : drawingClip;
}
inline void RenderSubPort::SetRealDrawingRegion(region *reg) {
	((canvasIsRotated) ? (rotatedDrawingClip) : (drawingClip)) = reg;
}
inline void RenderSubPort::SwapDrawingRegions() {
	region * const tmp = rotatedDrawingClip;
	rotatedDrawingClip = drawingClip;
	drawingClip = tmp;
}

#else

inline region *RenderSubPort::RealDrawingRegion() { return drawingClip; }
inline void RenderSubPort::SetRealDrawingRegion(region *reg) {	drawingClip = reg; }

#endif


struct RenderPort : public RenderSubPort {
	RenderContext *			context;
	int32					canvasLockCount;
	bool					canvasLocked;
	rect					portBounds;
	rect					drawingBounds;
	uint32					ueberInfoFlags;
};

/* RenderPort infoFlags */
enum {
	RPORT_RECALC_REGION		= 0x00000001,
	RPORT_RECALC_ORIGIN		= 0x00000002
};

void	grInitRenderPort(RenderPort *port);
void	grDestroyRenderPort(RenderPort *port);

#endif
