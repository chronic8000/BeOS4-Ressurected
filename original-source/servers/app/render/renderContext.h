
//******************************************************************************
//
//	File:			renderContext.h
//	Description:	Structures and functions for dealing with rendering contexts
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <stdio.h>

#include <IRegion.h>

#include "basic_types.h"
#include "renderTypes.h"
#include "font.h"

struct fregion;
struct RenderFuncs;
struct RenderCanvas;
struct RenderPort;
class ExtendedClip;
class SThickener;
class SPath;
class SPicture;

typedef RenderPort *	(*LockRenderPort)(void *vendor, uint32 *token, int32 pid);
typedef void 			(*UnlockRenderPort)(void *vendor, RenderPort *port, int32 pid);

// These are the low-level hardware operations that may be available.
enum HardwareFeatureOps {
	hwRectFillOp = 	0x00000001,
	hwRectCopyOp = 	0x00000002,
	hwSpanFillOp = 	0x00000004,
	hwBlitOp = 		0x00000008,
	hwLineOp = 		0x00000010,
	hwTextOp = 		0x00000020
};

// These are the types of hardware acceleration that can be performed,
// which may be a combination of multiple core hardware operations.
enum HardwareFeatures {
	hwRectFill = 	hwRectFillOp,
	hwRectCopy = 	hwRectCopyOp,
	hwSpanFill = 	hwRectFillOp|hwSpanFillOp,	// software rendering uses rect fill
	hwBlit = 		hwBlitOp,
	hwLine = 		hwLineOp,
	hwText = 		hwTextOp
};

struct RenderState {
						RenderState();
						~RenderState();
	RenderState&		operator=(const RenderState&);

	region *			globalEffective_Clip;
	fpoint				globalEffective_Origin;
	float				globalEffective_Scale;

	rect				localPictureClipLastRasterBounds;
	fpoint				localPictureClipOffset;
	bool				localPictureClipInverted;
	SPicture *			localPictureClip;
	region *			localRegionClip;
	fpoint				localOrigin;
	float				localScale;

	fpoint				penLocation;
	short				drawOp;
	short				srcAlphaSrc;
	short				alphaFunction;
	short				capRule;
	short				joinRule;
	float				miterLimit;
	float				penSize;
	rgb_color			foreColor;
	rgb_color			backColor;
	pattern 			stipplePattern;
	SFont				fontContext;

	RenderState *		next;
};

/* Canvas-specific rendering state. */
struct RenderCache {
	RenderCache *		next;

	uint32				canvasToken;
	uint32				hardwareMask;

	DrawString			fDrawString;
	DrawOneLine			fDrawOneLine;
	FillRects			fFillRects;
	FillSpans			fFillSpans;
	ColorOpFunc			fColorOpTransFunc;
	BlitFunction *		fBlitTable;
	BlitFunction *		fScaledBlitTable;
#if ROTATE_DISPLAY
	BlitFunction *		fRotatedBlitTable;
	BlitFunction *		fScaledRotatedBlitTable;
#endif

	uint32				foreColorCanvasFormat;
	uint32				backColorCanvasFormat;
	uint32				foreSrcAlpha,backSrcAlpha;
	uint32				foreAlphaPix,backAlphaPix;
	uint32				fontColors[25];
};

struct RenderContext {
	RenderFuncs *		renderFuncs;

	region *			globalEffective_Clip;
	fpoint				globalEffective_Origin;
	float				globalEffective_Scale;

	rect				localPictureClipLastRasterBounds;
	fpoint				localPictureClipOffset;
	bool				localPictureClipInverted;
	SPicture *			localPictureClip;
	region *			localRegionClip;
	fpoint				localOrigin;
	float				localScale;

	fpoint				penLocation;
	short				drawOp;
	short				srcAlphaSrc;
	short				alphaFunction;
	short				capRule;
	short				joinRule;
	float				miterLimit;
	float				penSize;
	rgb_color			foreColor;
	rgb_color			backColor;
	SFont				fontContext;
	pattern 			stipplePattern;
#if ROTATE_DISPLAY
	pattern 			realStipplePattern;
#endif

	/* Cached localEffectives */
	region *			localEffective_Clip;
	fpoint				localEffective_Origin;

	/* Cached totalEffectives */
	region *			totalEffective_Clip;	
	fpoint				totalEffective_Origin;
	float				totalEffective_Scale;
	float				totalEffective_PenSize;

	int32				spanBatch;

	rect				pictureClipStackRasterBounds;
	RenderState *		stateStack;
	SPicture *			record;
	uint8				stippling;
	int8				solidColor;
	uint32				foreColorARGB,backColorARGB;
	bool				buriedPictureClip;
	uint32				stateFlags;
	uint32				fontFlags;
	uint32				changedFlags;

	SThickener *		lineThickener;
	
	CoordTransform		fCoordTransform;
	RenderCache			cache;

	/* Buying a port */
	int32				ownerThread;
	void *				portVendor;
	uint32				portToken;
	LockRenderPort		fLockRenderPort;
	UnlockRenderPort	fUnlockRenderPort;
	RenderPort *		renderPort;
	int32				hasLock,portLockCount;
	bool				portLocked;
	int32				lockPersistence,opCount;
	int64				wasLocked;
};

#endif
