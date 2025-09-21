
//******************************************************************************
//
//	File:			renderCanvas.h
//	Description:	Structures and functions for dealing with rendering canvi
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef ACCELPACKAGE_H
#define ACCELPACKAGE_H

#include "basic_types.h"
#include <Accelerant.h>

typedef long (*Accel_SetCursorShape)(uchar *,uchar *,long,long,long,long);
typedef long (*Accel_MoveCursor)(long,long);
typedef long (*Accel_ShowCursor)(long);
typedef long (*Accel_Line)(long,long,long,long,uint32,bool,short,short,short,short);
typedef long (*Accel_Rect)(long,long,long,long,ulong);
typedef long (*Accel_Blit)(long,long,long,long,long,long);
typedef long (*Accel_Sync)(void);

struct AccelPackage {
	uint32					infoFlags;
	
	// These can be accessed only by renderLock.cpp
	uint32					hardwarePending;
	void*					lastUser;
	bool					haveSyncToken;
	sync_token				lastSyncToken;
	
	Accel_SetCursorShape	setCursorShape;
	Accel_MoveCursor		moveCursor;
	Accel_ShowCursor		showCursor;
	Accel_Line				line;
	Accel_Rect				rect;
	Accel_Blit				blit;
	Accel_Sync				sync;
	
	acquire_engine			engineAcquire;
	release_engine			engineRelease;
	wait_engine_idle		engineIdle;
	sync_to_token			engineSync;
	get_sync_token			engineGetSyncToken;

	screen_to_screen_blit	rectCopy;
	fill_rectangle			rectFill;
	invert_rectangle		rectInvert;
	fill_span				spanFill;
};

/* A popular form of acceleration */
extern AccelPackage noAccel;

#endif
