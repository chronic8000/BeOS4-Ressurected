
//******************************************************************************
//
//	File:			renderCanvas.h
//	Description:	Structures and functions for dealing with rendering canvi
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERCANVAS_H
#define RENDERCANVAS_H

#include <Accelerant.h>
#include "basic_types.h"
#include "renderTypes.h"

struct AccelPackage;
typedef void *			(*LockAccelPackage)(void *vendor, engine_token **token, sync_token *sync);
typedef void 			(*UnlockAccelPackage)(void *vendor, engine_token **token, sync_token *sync);

/*	The pixel plane onto which we are rendering.  This will have a set
	of rendering functions and a set of acceleration functions associated
	with it.  All elements are only valid while the structure is locked.
	If acceleration functions are to be used, they must be locked separately. */
struct RenderCanvas {
	/* Vendor's responsibility ------------------*/
	Pixels	 				pixels;
	RenderingPackage *		renderingPackage;
	rgb_color				overlayHack;

	/* Buying an acceleration package -----------*/
	void *					accelVendor;
	LockAccelPackage		fLockAccelPackage;
	UnlockAccelPackage		fUnlockAccelPackage;
	AccelPackage *			accelPackage;
	RenderPort *			port;
};

/* RenderCanvas infoFlags */
enum {
	RCANVAS_SIZE_CHANGED	= 0x00000001,
	RCANVAS_FORMAT_CHANGED	= 0x00000002
};

void 	grInitRenderCanvas(RenderCanvas *canvas);
void	grDestroyRenderCanvas(RenderCanvas *canvas);

#endif
