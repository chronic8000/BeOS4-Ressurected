
//******************************************************************************
//
//	File:			renderLock.h
//	Description:	The _internal_ interface for locking ports and canvi
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDER_LOCK_H
#define RENDER_LOCK_H

#include "renderInternal.h"
#include "accelPackage.h"
#include "renderContext.h"
#include "renderCanvas.h"
#include "renderPort.h"

#define grLockPort 		grLockPort_FastPath
#define grUnlockPort 	grUnlockPort_FastPath
#define grLockCanvi 	grLockCanvi_FastPath
#define grUnlockCanvi 	grUnlockCanvi_FastPath
#define grLockAccel 	grLockAccel_FastPath
#define grUnlockAccel 	grUnlockAccel_FastPath

// Assert that the render context is currently locked by the
// calling thread.
inline void grAssertLocked(RenderContext *context)
{
#if AS_DEBUG
#if __INTEL__
	thread_id pid = find_thread(NULL);
#else
	thread_id pid = xGetPid();
#endif
	if (context->hasLock != pid) {
		DebugPrintf(("*** grAssertLocked: Context %p held by %ld, I am %ld\n", context, context->hasLock, pid));
		debugger("grAssertLocked Context failed!");
	}
#endif
}

// Assert that the render context and given port canvas is locked
// by the calling thread.
inline void grAssertLocked(RenderContext *context, RenderSubPort *port)
{
#if AS_DEBUG
	grAssertLocked(context);
	RenderPort *ueberPort = context->renderPort;
	if (ueberPort->canvasLockCount <= 0) {
		DebugPrintf(("*** grAssertLocked: Canvas Port %p lock count at %ld\n", ueberPort, ueberPort->canvasLockCount));
		debugger("grAssertLocked Canvas failed!");
	}
#endif
}

void			grUnlockAccel_SlowPath(RenderContext *context, RenderSubPort *port, RenderCanvas *canvas);
void			grLockCanvi_SlowPath(RenderPort *port, uint32 primitives, region *touching=NULL);
void			grLockCanvi_Sync(RenderContext *context, RenderSubPort *port, AccelPackage *accel);
void			grLockCanvi_Register(RenderContext *context, RenderSubPort *port, AccelPackage *accel, uint32 primitives);
void			grUnlockCanvi_SlowPath(RenderPort *ueberPort);
RenderPort *	grLockPort_SlowPath(RenderContext *context, int32 pid);
void			grUnlockPort_SlowPath(RenderContext *context);

inline engine_token * grLockAccel_FastPath(RenderContext *context, RenderSubPort *port, RenderCanvas *canvas)
{
	++port->accelLockCount;
	if (port->accelLocked) return port->accelToken;
	port->accelLocked = true;
	return (canvas->fLockAccelPackage)	?
		(engine_token*)canvas->fLockAccelPackage(
			canvas->accelVendor,
			&port->accelToken,
			NULL /*&port->accelSyncToken*/) : NULL;
};

inline void grUnlockAccel_FastPath(RenderContext *context, RenderSubPort *port, RenderCanvas *canvas)
{
	--port->accelLockCount;
	if (context->wasLocked) return;
	if (port->accelLockCount == 0) grUnlockAccel_SlowPath(context,port,canvas);
};

inline void grLockCanvi_FastPath(RenderPort *ueberPort, rect *bound, uint32 primitives)
{
	++ueberPort->canvasLockCount;
	if (!ueberPort->canvasLocked) grLockCanvi_SlowPath(ueberPort,primitives,NULL);
	
	RenderContext * const context = ueberPort->context;
	RenderCache *cache = &context->cache;
	RenderSubPort *port = ueberPort;
	while (port) {
		AccelPackage* const accel = port->canvas->accelPackage;
		const uint32 hardware = primitives&cache->hardwareMask;
		if (accel) {
			// If the primitive we are going to draw has a software-based
			// component, we will need to check to sync with any hardware
			// acceleration.
			if (primitives != hardware) {
#if AS_DEBUG
				if (port->accelLockCount)
					debugger("grLockCanvi called while accelLock held");
#endif
				// If this accelerant has any hardware operations running,
				// wait for them to complete.
				if (port->hardwarePending || accel->hardwarePending)
					grLockCanvi_Sync(context, port, accel);
			}
			// For now, register all hardware operations as needing a
			// global sync.
			if (hardware)
				grLockCanvi_Register(context, port, accel, hardware);
		}
		port->hardwarePending |= hardware;
		port = port->next;
		cache = cache->next;
	};
};

inline void grUnlockCanvi_FastPath(RenderPort *ueberPort)
{
	--ueberPort->canvasLockCount;
	if (ueberPort->context->wasLocked) return;
	if (ueberPort->canvasLockCount == 0) grUnlockCanvi_SlowPath(ueberPort);
};

inline RenderPort *	grLockPort_FastPath(RenderContext *context)
{
	int32 pid = context->ownerThread;
#if __INTEL__
	if (pid < 0) pid = find_thread(NULL);
#else
	if (pid < 0) pid = xGetPid();
#endif
#if AS_DEBUG
	if (pid != find_thread(NULL)) debugger("Locking by someone other than owner!");
#endif
	if (context->hasLock == pid) {
		++context->portLockCount;
		return context->renderPort;
	};
	return grLockPort_SlowPath(context,pid);
};

inline void grUnlockPort_FastPath(RenderContext *context)
{
#if AS_DEBUG
	if (context->portLockCount <= 0) debugger("Context unlocked too many times!");
#endif
	// mathias - 11/17/2000: unlock the port only when portLocked is zero
	if ((--context->portLockCount) == 0)
	{
		int64 now = system_time();
		if ((now - context->wasLocked) < context->lockPersistence)
		{
			// All locks are gone, but don't release it quite yet...
			return;
		}
		grUnlockPort_SlowPath(context);
	}
};

#endif
