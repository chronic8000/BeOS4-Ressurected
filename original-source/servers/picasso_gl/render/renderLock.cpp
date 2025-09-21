
//******************************************************************************
//
//	File:			renderLock.cpp
//	Description:	The locking routines for rendering contexts, ports, and canvi
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
#include "shape.h"
#include "lines.h"
#include "fastmath.h"
#include "renderArc.h"
#include "render.h"
#include "renderLock.h"
#include "fregion.h"
#include "rect.h"
#include "region.h"

#include <support2/Locker.h>
#include <support2/Debug.h>

#ifndef DebugPrintf
#define DebugPrintf(a) _sPrintf a
#endif

#if 0
#define DebugSync(a) _sPrintf a
#else
#define DebugSync(a)
#endif

extern void grComposeTotalClip(RenderContext *context);

static BLocker accelSyncLock("accelSyncLock");

void grUnlockAccel_SlowPath(RenderContext *context, RenderSubPort *port, RenderCanvas *canvas)
{
	port->accelLocked = false;
	if (canvas->fUnlockAccelPackage) {
		canvas->fUnlockAccelPackage(
			canvas->accelVendor,
			&port->accelToken,
			&port->accelSyncToken);
		port->haveSyncToken = true;
		DebugSync(("Thread %ld stored sync token for port %p #%Ld\n",
						find_thread(NULL), port,
						port->accelSyncToken.counter));
		if (canvas->accelPackage && canvas->accelPackage->lastUser == port) {
			// We can now update the global accelerant with this sync token,
			// if it has any pending hardware operations.
			accelSyncLock.Lock();
			if (canvas->accelPackage->lastUser == port &&
					canvas->accelPackage->hardwarePending != 0 &&
					!canvas->accelPackage->haveSyncToken) {
				canvas->accelPackage->lastUser = NULL;
				canvas->accelPackage->haveSyncToken = true;
				canvas->accelPackage->lastSyncToken = port->accelSyncToken;
				DebugSync(("Thread %ld storing global sync token for accel op 0x%lx #%Ld\n",
								find_thread(NULL), canvas->accelPackage->hardwarePending,
								canvas->accelPackage->lastSyncToken.counter));
			}
			accelSyncLock.Unlock();
		}
	}
};

static inline void grLockCanvas(RenderContext *context, RenderSubPort *port, RenderCache *cache, region *touchingArg, bool force)
{
	uint32 flags = RCANVAS_FORMAT_CHANGED;
	if (!(port->touching = touchingArg)) port->touching = port->portClip;

	RenderCanvas *canvas = port->canvas =
		port->fLockRenderCanvas(port->canvasVendor,
			port->touching,&cache->canvasToken,&flags);

	if ((flags & RCANVAS_FORMAT_CHANGED) || force) {
		canvas->renderingPackage =
			renderingPackages
				[canvas->pixels.endianess]
				[canvas->pixels.pixelFormat];
		if (!(context->changedFlags & FORE_COLOR_CHANGED))
			canvas->renderingPackage->ForeColorChanged(context,port);
		if (!(context->changedFlags & BACK_COLOR_CHANGED))
			canvas->renderingPackage->BackColorChanged(context,port);
		if (!(context->changedFlags & (DRAWOP_CHANGED|STIPPLE_CHANGED)))
			canvas->renderingPackage->PickRenderers(context,port);
			grPickCanvasRenderers_OneCanvas(context,port);
	};
};

void grLockCanvi_SlowPath(RenderPort *ueberPort, uint32 primitives, region *touchingArg)
{
	RenderCache *cachePtr = &ueberPort->context->cache;
	RenderCache **cache = &cachePtr;
	RenderContext *context = ueberPort->context;
	RenderSubPort *port = ueberPort;
	bool force;

	ueberPort->canvasLocked = true;
	
	while (port) {
		force = !(*cache);
		if (force) {
			*cache = (RenderCache*)grMalloc(sizeof(RenderCache),"RenderCache",MALLOC_CANNOT_FAIL);
			(*cache)->canvasToken = 0xFFFFFFFF;
			(*cache)->hardwareMask = 0;
			(*cache)->next = NULL;
		};
		port->cache = (*cache);
		
		grLockCanvas(context,port,*cache,touchingArg,force);
		
		port = port->next;
		cache = &(*cache)->next;
	};

	if (context->changedFlags & FORE_COLOR_CHANGED)
		grForeColorChanged(context);
	if (context->changedFlags & BACK_COLOR_CHANGED)
		grBackColorChanged(context);
	if (context->changedFlags & (DRAWOP_CHANGED|STIPPLE_CHANGED))
		grPickCanvasRenderers(context);

	context->changedFlags &= ~(FORE_COLOR_CHANGED|BACK_COLOR_CHANGED|DRAWOP_CHANGED|STIPPLE_CHANGED);
};

void grLockCanvi_Sync(RenderContext *context, RenderSubPort *port, AccelPackage *accel)
{
	if (port->canvas->fLockAccelPackage && accel->engineSync) {
		bool retrieved = false;		// Did we sync completely up-to-date?
		
		// First sync with the local port.
		if (!port->haveSyncToken) {
			const bool locked = port->accelLocked;
			if (!locked) grLockAccel(context,port,port->canvas);
			DebugSync(("Thread %ld retrieving sync token for local port %p.\n",
						find_thread(NULL), port));
			accel->engineGetSyncToken(port->accelToken,&port->accelSyncToken);
			if (!locked) grUnlockAccel(context,port,port->canvas);
			retrieved = true;
		}
		DebugSync(("Thread %ld syncing local port %p, counter=%Ld, id=%ld.\n",
					find_thread(NULL), port, port->accelSyncToken.counter,
					port->accelSyncToken.engine_id));
		port->haveSyncToken = false;
		accel->engineSync(&port->accelSyncToken);
		
		// Now sync with any outstanding global operations.  This
		// is a little tricky because others may want to sync as well,
		// so we don't clear that outstanding op until after the
		// sync is done.
		if (!retrieved && accel->hardwarePending) {
			retrieved = false;
			
			// First pull values out of accelerant package.
			accelSyncLock.Lock();
			const bool pending = (accel->hardwarePending != 0);
			void* user = accel->lastUser;
			const bool haveSync = accel->haveSyncToken;
			sync_token syncToken;
			if (haveSync) syncToken = accel->lastSyncToken;
			accelSyncLock.Unlock();
			
			// Now wait for graphics to complete, if something is
			// still pending.  Note that we must be unlocked here
			// because we can't acquire the accelerant lock while
			// holding accelSyncLock, nor do we want to block waiting
			// for the engine while holding that lock.
			if (pending) {
				DebugSync(("Thread %ld need to sync port %p with hardware op 0x%lx\n",
							find_thread(NULL), port, accel->hardwarePending));
				if (!haveSync) {
					const bool locked = port->accelLocked;
					if (!locked) grLockAccel(context,port,port->canvas);
					accel->engineGetSyncToken(port->accelToken,&syncToken);
					if (!locked) grUnlockAccel(context,port,port->canvas);
					//DebugSync(("Retrieved sync token #%Ld\n", syncToken.counter));
					// Stuff this back in to the accelerate, so that we
					// can identify if the state has changed.
					accelSyncLock.Lock();
					if (accel->lastSyncToken.counter < syncToken.counter) {
						accel->lastSyncToken = syncToken;
						accel->haveSyncToken = true;
					}
					accelSyncLock.Unlock();
				//} else {
				//	DebugSync(("(Using sync token previously installed #%Ld)\n", syncToken.counter));
				}
				accel->engineSync(&syncToken);
			
				// Now clear accelerarant's pending operations, only if
				// they haven't changed since we got them.
				accelSyncLock.Lock();
				if (accel->hardwarePending != 0 && accel->haveSyncToken &&
						accel->lastUser == user &&
						accel->lastSyncToken.counter <= syncToken.counter) {
					//DebugSync(("Clearing out last global operation.\n"));
					accel->hardwarePending = 0;
					accel->lastUser = NULL;
					accel->haveSyncToken = false;	
				}
				accelSyncLock.Unlock();
			}
		}
	}
	port->hardwarePending = 0;
}

void grLockCanvi_Register(RenderContext *context, RenderSubPort *port, AccelPackage *accel, uint32 primitives)
{
	accelSyncLock.Lock();
	//DebugSync(("Thread %ld registering global hardware operation 0x%lx for port %p\n",
	//				find_thread(NULL), primitives, port));
	accel->hardwarePending |= primitives;
	accel->lastUser = port;
	accel->haveSyncToken = false;
	accelSyncLock.Unlock();
}

void grUnlockCanvi_SlowPath(RenderPort *ueberPort)
{
	ueberPort->canvasLocked = false;
	RenderSubPort *port = ueberPort;
	while (port) {
		port->fUnlockRenderCanvas(port->canvasVendor,port->touching,
			port->hardwarePending ? (&port->accelSyncToken) : NULL);
		port = port->next;
	}
}

void grSetLockingPersistence(RenderContext *context, int32 usecs)
{
	RDebugPrintf((context,"grSetLockingPersistence(%d) (was %d)\n",usecs,context->lockPersistence));
	context->lockPersistence = usecs;
}

void grGiveUpLocks(RenderContext *context)
{
#if __INTEL__
	if (context->wasLocked && (context->hasLock==find_thread(NULL))) {
#else
	if (context->wasLocked && (context->hasLock==xGetPid())) {
#endif
		if (context->portLockCount == 0) {
			// the lock is no longer held, release it now.
			grUnlockPort_SlowPath(context);
		} else {
			// the lock is still held, release it as soon as last
			// reference is removed.
			context->wasLocked = 0;
		}
	}
}

void grSync(RenderContext *context)
{
	RenderSubPort *port = grLockPort(context);
	uint32 hardwarePending = port->hardwarePending;
	port = port->next;
	while (port) {
		hardwarePending |= port->hardwarePending;
		port = port->next;
	}
	if (hardwarePending) {
		grLockCanvi(context->renderPort,&context->renderPort->portBounds,0x80000000);
		grUnlockCanvi(context->renderPort);
	}
	grUnlockPort(context);
}

RenderPort * grLockPort_SlowPath(RenderContext *context, int32 pid)
{	
	RenderPort *ueberPort = context->fLockRenderPort(context->portVendor,&context->portToken,pid);
	context->renderPort = ueberPort;

	if (!ueberPort->ueberInfoFlags &
		!(context->changedFlags & (CLIP_CHANGED|ORIGIN_CHANGED)))
	{
leaving:
		context->portLockCount = 1;
		context->portLocked = true;
		context->hasLock = pid;
		if (context->lockPersistence)
			context->wasLocked = system_time();
		ueberPort->context = context;
		return ueberPort;
	}

	RenderSubPort *port = ueberPort;
	int32 count = 0;
	while (port) {
		if (port->infoFlags || (context->changedFlags & (CLIP_CHANGED|ORIGIN_CHANGED))) {
			if (context->totalEffective_Clip) {
				offset_region(context->totalEffective_Clip, port->origin.x, port->origin.y);
				and_region(port->portClip, context->totalEffective_Clip, port->drawingClip);
				offset_region(context->totalEffective_Clip, -port->origin.x, -port->origin.y);
			} else {
				copy_region(port->portClip, port->drawingClip);
			}
#if ROTATE_DISPLAY
			if (port->canvasIsRotated)
				port->rotater->RotateRegion(port->drawingClip, port->rotatedDrawingClip);
#endif
			port->phase_x = ((int32)(-port->origin.x)) & 0x7;
			port->phase_y = ((int32)(-port->origin.y)) & 0x7;
		}

		if (count) {
			rect tmpRect = port->drawingClip->Bounds();
			offset_rect(&tmpRect, -port->origin.x, -port->origin.y);
			union_rect(&tmpRect, &ueberPort->drawingBounds, &ueberPort->drawingBounds);
		} else {
			ueberPort->drawingBounds = port->drawingClip->Bounds();
			offset_rect(&ueberPort->drawingBounds, -port->origin.x, -port->origin.y);
		}

		port = port->next;
		count++;
	}
	context->changedFlags &= ~(CLIP_CHANGED|ORIGIN_CHANGED);
	goto leaving;
}

void grUnlockPort_SlowPath(RenderContext *context)
{
//	RDebugPrintf((context,"Persistent lock being given up: %d > %d (opCount = %d)\n",
//		(int32)(now - context->wasLocked), context->lockPersistence, context->opCount));
	context->wasLocked = 0;
	context->portLocked = false;
	context->portLockCount = 0;
	RenderPort *ueberPort = context->renderPort;
	RenderSubPort *port = ueberPort;

	while (port) {
		#if AS_DEBUG
		if (port->accelLockCount) {
			DebugPrintf(("grUnlockPort -- port %p accelLock still held at %ld\n", port, port->accelLockCount));
			debugger("grUnlockPort with accelLock still held!");
		}
		#endif
		if (port->accelLocked) {
			port->accelLockCount = 1;
			grUnlockAccel(context,port,port->canvas);
		};
		port = port->next;
	};

	#if AS_DEBUG
	if (ueberPort->canvasLockCount) {
		DebugPrintf(("grUnlockPort -- context %p canvasLock still held at %ld\n", context, ueberPort->canvasLockCount));
		debugger("grUnlockPort with canvasLock still held!");
	}
	#endif
	if (ueberPort->canvasLocked) {
		ueberPort->canvasLockCount = 1;
		grUnlockCanvi(ueberPort);
	};
	
	context->hasLock = -1;
	context->fUnlockRenderPort(context->portVendor,context->renderPort,context->ownerThread);
};

RenderPort *grLock(RenderContext *context)
{
	return grLockPort(context);
};

void grUnlock(RenderContext *context)
{
	grUnlockPort(context);
};
