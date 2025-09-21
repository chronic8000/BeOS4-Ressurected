
//******************************************************************************
//
//	File:			renderState.cpp
//	Description:	Rendering state changes
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderLock.h"
#include "accelPackage.h"
#include "edges.h"
#include "shape.h"
#include "lines.h"
#include "fastmath.h"
#include "renderArc.h"
#include "render.h"
#include "fregion.h"
#include "rect.h"
#include "graphicDevice.h"

#include <support2/Debug.h>

/* Prototypes */
void grComputeEffectiveLocalClip(RenderContext *context);
void grComposeTotalClip(RenderContext *context);
void grMergeScale(RenderContext *context);
void grMergeOrigin(RenderContext *context);

extern RenderFuncs thinPenRenderFuncs;
extern RenderFuncs fatPenRenderFuncs;

void grRenderXform_Scale(RenderContext *context, fpoint *vertices, int32 vCount)
{
	float scale = context->totalEffective_Scale;
	while (vCount--) {
		vertices->x *= scale;
		vertices->y *= scale;
		vertices++;
	};
};

void grRenderXform_Round(RenderContext *context, fpoint *vertices, int32 vCount)
{
	while (vCount--) {
		vertices->x = floor(vertices->x + 0.5);
		vertices->y = floor(vertices->y + 0.5);
		vertices++;
	};
};

void grRenderXform_ScaleRound(RenderContext *context, fpoint *vertices, int32 vCount)
{
	float scale = context->totalEffective_Scale;
	while (vCount--) {
		vertices->x = floor(vertices->x*scale + 0.5);
		vertices->y = floor(vertices->y*scale + 0.5);
		vertices++;
	};
};

/* RenderState set-up and break-down */
RenderState::RenderState()
{
	globalEffective_Clip = NULL;
	localRegionClip = NULL;

	globalEffective_Scale = 1.0;
	localScale = 1.0;

	globalEffective_Origin.x = globalEffective_Origin.y = 0.0;
	localOrigin.x = localOrigin.y = 0.0;

	drawOp = OP_COPY;
	srcAlphaSrc = 0;
	alphaFunction = 0;
	*((uint64*)&stipplePattern.data[0]) = 0xFFFFFFFFFFFFFFFFLL;
	penSize = 1.0;
	penLocation.x = 0;
	penLocation.y = 0;
	foreColor = rgb_black;
	backColor = rgb_white;
	capRule = BUTT_CAP;
	joinRule = BEVEL_CAP;
	miterLimit = 10.0;
};

RenderState::~RenderState()
{
	if (globalEffective_Clip) kill_region(globalEffective_Clip);
	if (localRegionClip) kill_region(localRegionClip);
};

RenderState& RenderState::operator=(const RenderState &original)
{
	if (&original == this)
		return *this;

	grCopyRegion((region **)&original.globalEffective_Clip, &globalEffective_Clip);
	globalEffective_Origin = original.globalEffective_Origin;
	globalEffective_Scale = original.globalEffective_Scale;

	grCopyRegion((region **)&original.localRegionClip, &localRegionClip);
	localOrigin = original.localOrigin;
	localScale = original.localScale;
	penLocation = original.penLocation;
	drawOp = original.drawOp;
	srcAlphaSrc = original.srcAlphaSrc;
	alphaFunction = original.alphaFunction;
	capRule = original.capRule;
	joinRule = original.joinRule;
	miterLimit = original.miterLimit;
	penSize = original.penSize;
	foreColor = original.foreColor;
	backColor = original.backColor;
	stipplePattern = original.stipplePattern;
	fontContext = original.fontContext;
	return *this;
}


/* This is used as the lock routine for things that don't need to lock */
extern void noop();

void grLineDependencies(RenderContext *context)
{
	context->lineThickener->LineDependencies();
}

/*	This converts the foreground and background 32-bit RGBA values to
	pixel values for whatever canvas we're attached to. */
void grColorToCanvasFormat(
	RenderContext *context,
	RenderCanvas *canvas,
	rgb_color fc,
	int32 pixelFormat,
	int8 endianess,
	uint32 *canvasFormat)
{
	uint16 index;
	uint32 tmp;

	switch (pixelFormat) {
		case PIX_8BIT:
		{
			if (*((uint32*)&fc) == TRANSPARENT_RGBA) {
				tmp = TRANSPARENT_MAGIC_8BIT;
			} else {
				index  = ((uint16)(fc.red & 0xF8)) << 7;
				index |= ((uint16)(fc.green & 0xF8)) << 2;
				index |= ((uint16)fc.blue) >> 3;
				tmp = canvas->pixels.colorMap->inverted[index];
			};
			tmp |= tmp << 8;
			tmp |= tmp << 16;
			*canvasFormat = tmp;
			return;
		}
		case PIX_32BIT:
		{
			if (endianess == HOST_ENDIANESS)
				*canvasFormat = (fc.alpha<<24)|(fc.red<<16)|(fc.green<<8)|(fc.blue);
			else
				*canvasFormat = (fc.blue<<24)|(fc.green<<16)|(fc.red<<8)|(fc.alpha);
			return;
		}
		case PIX_15BIT:
		{
			if (*((uint32*)&fc) == TRANSPARENT_RGBA) {
				tmp = TRANSPARENT_MAGIC_15BIT;
			} else {
				tmp  = ((uint16)(fc.red & 0xF8)) << 7;
				tmp |= ((uint16)(fc.green & 0xF8)) << 2;
				tmp |= ((uint16)fc.blue) >> 3;
				tmp |= 0x8000;
			};
			if (endianess != HOST_ENDIANESS)
			  tmp = ((tmp>>8)&0xFF) | ((tmp<<8)&0xFF00);
			tmp |= tmp << 16;
			*canvasFormat = tmp;
			return;
		}
		case PIX_16BIT:
		{
			tmp  = ((uint16)(fc.red & 0xF8)) << 8;
			tmp |= ((uint16)(fc.green & 0xFC)) << 3;
			tmp |= ((uint16)fc.blue) >> 3;
			if (endianess != HOST_ENDIANESS)
				tmp = ((tmp>>8)&0xFF) | ((tmp<<8)&0xFF00);
			tmp |= tmp << 16;
			*canvasFormat = tmp;
			return;
		}
	};
};

static uint32 masks[] =
{
	0x00000001,
	0x000000FF,
	0x00007FFF,
	0x0000FFFF,
	0xFFFFFFFF
};

void grColorToAlphaPreps(
	RenderContext *context,
	RenderCanvas *canvas,
	rgb_color fc,
	uint32 *preprocessedPixel,
	uint32 *alphaFactor)
{
	rgb_color c;
	int32 alpha = fc.alpha + (fc.alpha >> 7);	
	c.red = (((int32)fc.red) * alpha) >> 8;
	c.green = (((int32)fc.green) * alpha) >> 8;
	c.blue = (((int32)fc.blue) * alpha) >> 8;
	c.alpha = (((int32)fc.alpha) * alpha) >> 8;
	
	int32 pixelFormat = canvas->pixels.pixelFormat;
	int8 endianess = canvas->pixels.endianess;
	if (pixelFormat == PIX_8BIT) {
		pixelFormat = PIX_15BIT;
		endianess = HOST_ENDIANESS;
	};
	grColorToCanvasFormat(context,canvas,c,pixelFormat,endianess,preprocessedPixel);
	uint32 mask = masks[pixelFormat];
	if ((pixelFormat==PIX_15BIT) && (endianess != HOST_ENDIANESS))
		mask = ((mask>>8)&0xFF) | ((mask<<8)&0xFF00);
	*preprocessedPixel &= mask;
	*alphaFactor = alpha;
};

void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port)
{
	rgb_color fc = context->foreColor;
	RenderCanvas *canvas = port->canvas;
	grColorToCanvasFormat(context,canvas,fc,
		canvas->pixels.pixelFormat,
		canvas->pixels.endianess,
		&port->cache->foreColorCanvasFormat);
	grColorToAlphaPreps(context,canvas,fc,
		&port->cache->foreAlphaPix,
		&port->cache->foreSrcAlpha);
};

void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port)
{
	rgb_color fc = context->backColor;
	RenderCanvas *canvas = port->canvas;
	grColorToCanvasFormat(context,canvas,fc,
		canvas->pixels.pixelFormat,
		canvas->pixels.endianess,
		&port->cache->backColorCanvasFormat);
	grColorToAlphaPreps(context,canvas,fc,
		&port->cache->backAlphaPix,
		&port->cache->backSrcAlpha);
};

void grForeColorChanged(RenderContext *context)
{
	rgb_color fc = context->foreColor;
	context->foreColorARGB =
		(((uint32)fc.alpha)<<24)|
		(((uint32)fc.red)<<16)|
		(((uint32)fc.green)<<8)|
		(((uint32)fc.blue));

	RenderSubPort *port = context->renderPort;
	while (port) {
		port->canvas->renderingPackage->ForeColorChanged(context,port);
		port = port->next;
	};
};

void grBackColorChanged(RenderContext *context)
{
	rgb_color fc = context->backColor;
	context->backColorARGB =
		(((uint32)fc.alpha)<<24)|
		(((uint32)fc.red)<<16)|
		(((uint32)fc.green)<<8)|
		(((uint32)fc.blue));
	RenderSubPort *port = context->renderPort;
	while (port) {
		port->canvas->renderingPackage->BackColorChanged(context,port);
		port = port->next;
	};
};

void grPickCanvasRenderers_OneCanvas(RenderContext *context, RenderSubPort *port)
{
	int32 op = context->drawOp;
	int32 stippling = context->stippling;
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		int32		i;
		uint32		tmp, word, nuword0, nuword1;
		
		nuword0 = nuword1 = 0L;
		word = ((uint32*)&context->realStipplePattern)[0];
		for (i=3; i>=0; i--) {
			tmp = ((word>>i) & 0x11111111L) * 0x08040201L;
			nuword0 |= ((tmp >> 28) & 15)<<(24-i*8);
			nuword1 |= ((tmp >> 24) & 15)<<(24-i*8);
		}
		word = ((uint32*)&context->realStipplePattern)[1];
		for (i=3; i>=0; i--) {
			tmp = ((word>>i) & 0x11111111L) * 0x08040201L;
			nuword0 |= ((tmp >> 28) & 15)<<(28-i*8);
			nuword1 |= ((tmp >> 24) & 15)<<(28-i*8);
		}
		((uint32*)&context->stipplePattern)[0] = nuword0;
		((uint32*)&context->stipplePattern)[1] = nuword1;
	}
	else
		context->stipplePattern = context->realStipplePattern;
#endif

	cache->fDrawString = (DrawString)noop;

	if (!canvas) {
		noops:
		cache->fColorOpTransFunc = (ColorOpFunc)noop;
		cache->fDrawOneLine = (DrawOneLine)noop;
		cache->fFillRects = (FillRects)noop;
		return;
	};

	cache->hardwareMask = 0;
	canvas->renderingPackage->PickRenderers(context,port);
	if (canvas->accelPackage->rectCopy) cache->hardwareMask |= hwRectCopyOp;

	if (!stippling) {
		switch (op) {
			case OP_COPY:
			hardwareFill:
				if (canvas->accelPackage->rectFill) {
					cache->fFillRects = grFillRectsHardware;
					cache->hardwareMask |= hwRectFillOp;
				};
				if (canvas->accelPackage->spanFill) {
					cache->fFillSpans = grFillSpansHardware;
					cache->hardwareMask |= hwSpanFillOp;
				};
				break;
			case OP_INVERT:
				if (canvas->accelPackage->rectInvert &&
					(canvas->pixels.pixelFormat != PIX_8BIT)) {
					cache->fFillRects = grInvertRectsHardware;
					cache->hardwareMask |= hwRectFillOp;
				};
				break;
			case OP_OVER:
				if ((((uint32*)&context->stipplePattern)[0] == 0xFFFFFFFF) &&
					(((uint32*)&context->stipplePattern)[1] == 0xFFFFFFFF))
					goto hardwareFill;
				goto noops;
			case OP_ERASE:
				if ((((uint32*)&context->stipplePattern)[0] == 0x00000000) &&
					(((uint32*)&context->stipplePattern)[1] == 0x00000000))
					goto hardwareFill;
				goto noops;
		};
	};
};

void grPickCanvasRenderers(RenderContext *context)
{
	RenderSubPort *port = context->renderPort;
	while (port) {
		grPickCanvasRenderers_OneCanvas(context,port);
		port = port->next;
	};
};

void grPickRenderFuncs(RenderContext *context)
{
	if (context->totalEffective_Scale != 1.0) {
		if (context->stateFlags & RENDER_ROUND_COORDS)
			context->fCoordTransform = grRenderXform_ScaleRound;
		else
			context->fCoordTransform = grRenderXform_Scale;
	} else {
		if (context->stateFlags & RENDER_ROUND_COORDS)
			context->fCoordTransform = grRenderXform_Round;
		else
			context->fCoordTransform = NULL;
	}
	if (context->totalEffective_PenSize > 1.0) {
		context->renderFuncs = &fatPenRenderFuncs;
	} else {
		context->renderFuncs = &thinPenRenderFuncs;
	}
}

/******************************************************/
/******************* Clipping recalc ******************/


void grClipDependencies(RenderContext *context)
{
	if (context->portLocked) {
		RenderSubPort *port = context->renderPort;
		RenderPort *ueberPort = context->renderPort;
		rect tmpRect;
		int32 count = 0;
		while (port) {	
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
			if (count) {
				tmpRect = port->drawingClip->Bounds();
				offset_rect(&tmpRect, -port->origin.x, -port->origin.y);
				union_rect(&tmpRect, &ueberPort->drawingBounds, &ueberPort->drawingBounds);
			} else {
				ueberPort->drawingBounds = port->drawingClip->Bounds();
				offset_rect(&ueberPort->drawingBounds, -port->origin.x, -port->origin.y);
			}

			port = port->next;
			count++;
		}
	} else {
		context->changedFlags |= CLIP_CHANGED;
	}
}


void grComputeEffectiveLocalClip(RenderContext *context)
{
	if (context->localRegionClip) {
		if (!context->localEffective_Clip)
			context->localEffective_Clip = newregion();
		else
			clear_region(context->localEffective_Clip);
		rect r;
		float scale = context->totalEffective_Scale;
		region *reg = context->localEffective_Clip;
		const int32 count = context->localRegionClip->CountRects();
		const rect *data = context->localRegionClip->Rects();
		float h = context->totalEffective_Origin.x;
		float v = context->totalEffective_Origin.y;

		if (scale != 1.0) {
			for (int32 i=0;i<count;i++,data++) {
				r.top = (int32)floor(data->top*scale + v);
				r.left = (int32)floor(data->left*scale + h);
				r.bottom = (int32)floor((data->bottom+1)*scale + v) - 1;
				r.right = (int32)floor((data->right+1)*scale + h) - 1;
				add_rect(reg,r);
			};
		} else if ((h != floor(h)) || (v != floor(v))) {
			for (int32 i=0;i<count;i++,data++) {
				r.top = (int32)floor(data->top + v);
				r.left = (int32)floor(data->left + h);
				r.bottom = (int32)floor((data->bottom+1) + v) - 1;
				r.right = (int32)floor((data->right+1) + h) - 1;
				add_rect(reg,r);
			};
		} else {
			copy_region(context->localRegionClip,reg);
			h = floor(h); v = floor(v);
			if ((h != 0) || (v != 0)) offset_region(reg,(int32)h,(int32)v);
		};
	} else {
		if (context->localEffective_Clip)
			kill_region(context->localEffective_Clip);
		context->localEffective_Clip = NULL;
	};

	grComposeTotalClip(context);
};

void grComposeTotalClip(RenderContext *context)
{
	if (context->globalEffective_Clip || context->localEffective_Clip) {
		if (!context->totalEffective_Clip)
			context->totalEffective_Clip = newregion();
		if (context->localEffective_Clip && context->globalEffective_Clip) {
			and_region(
				context->localEffective_Clip,
				context->globalEffective_Clip,
				context->totalEffective_Clip);
		} else if (context->localEffective_Clip) {
			copy_region(context->localEffective_Clip,context->totalEffective_Clip);
		} else {
			copy_region(context->globalEffective_Clip,context->totalEffective_Clip);
		}
		grClipDependencies(context);
	} else if (context->totalEffective_Clip) {
		kill_region(context->totalEffective_Clip);
		context->totalEffective_Clip = NULL;
		grClipDependencies(context);
	}
}

void grClipToRegion(RenderContext *context, region *reg)
{
	if (reg == NULL) {
		grClearClip(context);
		return;
	}

	RDebugPrintf((context,"grClipToRegion()\n"));

	if (!context->localRegionClip)
		context->localRegionClip = newregion();
	copy_region(reg, context->localRegionClip);

	if (context->totalEffective_Scale == 1.0) {
		if (!context->localEffective_Clip)
			context->localEffective_Clip = newregion();
		copy_region(reg, context->localEffective_Clip);
		if ((context->totalEffective_Origin.x != 0) ||
			(context->totalEffective_Origin.y != 0)) {
			offset_region(context->localEffective_Clip,
				(int32)floor(context->totalEffective_Origin.x+0.5),
				(int32)floor(context->totalEffective_Origin.y+0.5));
		}
		grComposeTotalClip(context);
		return;
	}
	grComputeEffectiveLocalClip(context);
}

void grClearClip(RenderContext *context)
{
	RDebugPrintf((context,"grClearClip()\n"));

	if (!context->localRegionClip) return;

	if (context->localRegionClip) {
		kill_region(context->localRegionClip);
		context->localRegionClip = NULL;
	}

	grComputeEffectiveLocalClip(context);
}

/******************************************************/
/******************* Stack pushing ********************/

/* Pushes the scale stack down */
void grMergeScale(RenderContext *context)
{
	context->totalEffective_Scale =
	context->globalEffective_Scale = 
		context->localScale * context->globalEffective_Scale;
	context->localScale = 1.0;
};

/* Pushes the clipping stack down */
void grMergeClip(RenderContext *context)
{
	if (!context->localEffective_Clip) return;
	if (!context->globalEffective_Clip) {
		context->globalEffective_Clip = context->localEffective_Clip;
	} else {
		kill_region(context->globalEffective_Clip);
		kill_region(context->localEffective_Clip);
		context->globalEffective_Clip = context->totalEffective_Clip;
		context->totalEffective_Clip = newregion();
		copy_region(context->globalEffective_Clip,context->totalEffective_Clip);
	}
	context->localEffective_Clip = NULL;
	if (context->localRegionClip) kill_region(context->localRegionClip);
	context->localRegionClip = NULL;
}

/* Pushes the origin stack down */
void grMergeOrigin(RenderContext *context)
{
	context->globalEffective_Origin.x = context->totalEffective_Origin.x;
	context->globalEffective_Origin.y = context->totalEffective_Origin.y;
	context->localOrigin.x = 
	context->localOrigin.y = 0.0;
	context->localEffective_Origin.x = 
	context->localEffective_Origin.y = 0.0;
};

/**********************************************************/
/* Public API *********************************************/
/**********************************************************/

void grHoldPicks(RenderContext *context)
{
	/* XXXXXXXXX */
	return;
/*
	context->holdPicks++;
	context->changedFlags = 0;
*/
};

void grAllowPicks(RenderContext *context)
{
	/* XXXXXXXXX */
	return;
/*
	if (!context->holdPicks) return;
	
	context->holdPicks--;
	if (!context->holdPicks) {
		if (context->changedFlags & SCALE_CHANGED)
			context->totalEffective_Scale = context->localScale * context->globalEffective_Scale;
		if (context->changedFlags & (PENSIZE_CHANGED|SCALE_CHANGED))
			context->scaledPenSize = context->totalEffective_Scale*context->penSize;
		if (context->changedFlags & (PENSIZE_CHANGED|ORIGIN_CHANGED))
			grOriginDependencies(context);
		if (context->changedFlags & (LINEMODE_CHANGED|PENSIZE_CHANGED|SCALE_CHANGED))
			grLineDependencies(context);
		if (context->changedFlags & (DRAWOP_CHANGED|PENSIZE_CHANGED|STIPPLE_CHANGED|SCALE_CHANGED))
			grPickRenderers(context);
		if (context->changedFlags & (CLIP_CHANGED|SCALE_CHANGED)) {
			grComputeEffectiveLocalClip(context);
			grComposeTotalClip(context);
		};
		context->changedFlags = 0;
	};
*/
};

/**********************************************************/
/**************** State queries ***************************/

void grGetPixelFormat(RenderContext *context, int32 *pixelFormat)
{
	RenderPort *ueberPort = grLockPort(context);
	grLockCanvi(ueberPort,NULL,0);
	*pixelFormat = PIX_INVALIDFORMAT;
	RenderSubPort *port = ueberPort;
	while (port) {
		if (*pixelFormat < port->canvas->pixels.pixelFormat)
			*pixelFormat = port->canvas->pixels.pixelFormat;
		port = port->next;
	};
	grUnlockCanvi(ueberPort);
	grUnlockPort(context);
};

void grGetForeColor(RenderContext *context, rgb_color *foreColor)
{
	*foreColor = context->foreColor;
};

void grGetBackColor(RenderContext *context, rgb_color *backColor)
{
	*backColor = context->backColor;
};

void grGetLineMode(RenderContext *context, short *capRule, short *joinRule, float *miterLimit)
{
	*capRule = context->capRule;
	*joinRule = context->joinRule;
	*miterLimit = context->miterLimit;
};

void grGetDrawOp(RenderContext *context, short *drawOp)
{
	*drawOp = context->drawOp;
};

void grGetBlendingMode(RenderContext *context, int16 *srcAlphaSrc, int16 *alphaFunction)
{
	*srcAlphaSrc = context->srcAlphaSrc;
	*alphaFunction = context->alphaFunction;
};

void grGetPenSize(RenderContext *context, float *penSize)
{
	*penSize = context->penSize;
};

void grGetStipplePattern(RenderContext *context, pattern *stipplePattern)
{
#if ROTATE_DISPLAY
	*stipplePattern = context->realStipplePattern;
#else
	*stipplePattern = context->stipplePattern;
#endif
};

void grGetTotalScale(RenderContext *context, float *scale)
{
	*scale = (context->localScale * context->globalEffective_Scale);
};

void grGetOrigin(RenderContext *context, fpoint *origin)
{
	*origin = context->localOrigin;
};

void grGetPenLocation(RenderContext *context, fpoint *penLoc)
{
	*penLoc = context->penLocation;
};

void grGetClip(RenderContext *context, region *clip)
{
	if (context->changedFlags |= CLIP_CHANGED) {
		grComputeEffectiveLocalClip(context);
	};
	if (context->totalEffective_Clip)
		copy_region(context->totalEffective_Clip,clip);
	else 
		clear_region(clip);
};

void grGetLocalClip(RenderContext *context, region *clip)
{
	if (context->changedFlags |= CLIP_CHANGED) {
		grComputeEffectiveLocalClip(context);
	};
	if (context->localEffective_Clip) {
		copy_region(context->localEffective_Clip,clip);
		offset_region(clip,
			(int32)floor(-context->totalEffective_Origin.x),
			(int32)floor(-context->totalEffective_Origin.y));
	} else 
		clear_region(clip);
};

void grGetUpdateRegion(RenderContext *context, region *update)
{
	region *tmp = newregion();
	region *r1 = tmp;
	region *r2 = update;
	region *swap;

	clear_region(update);

	RenderSubPort *port = grLockPort(context);
	
	while (port) {
		offset_region(port->drawingClip,-port->origin.x,-port->origin.y);
		or_region(r1,port->drawingClip,r2);
		offset_region(port->drawingClip,port->origin.x,port->origin.y);
		swap = r2;
		r2 = r1;
		r1 = swap;

		port = port->next;
	};
	
	grUnlockPort(context);

	if (r1 != update) copy_region(r1,update);
	
	kill_region(tmp);
};

void grGetScale(RenderContext *context, float *scale)
{
	*scale = context->localScale;
};

/**********************************************************/
/**************** State changes ***************************/

#if USE_FONTS
void grGetFontPointer(RenderContext *context, SFont** into)
{
	if (!context->fontContext.IsValid()) SFont::GetStandardFont(B_PLAIN_FONT, &context->fontContext);
	*into = &context->fontContext;
};
#endif

void grGetFont(RenderContext *context, SFont* into)
{
	if (context->fontContext.IsValid())
		*into = context->fontContext;
	else
		SFont::GetStandardFont(B_PLAIN_FONT, into);
};

void grSetFontFromPacket(RenderContext *context, const void *packet, size_t size, int32 mask)
{
	int32 oldBits=-1;
	if (!context->fontContext.IsValid())
		SFont::GetStandardFont(B_PLAIN_FONT, &context->fontContext);
	else
		oldBits = context->fontContext.BitsPerPixel();
	context->fontContext.SetTo(packet, size, mask);
	context->fontContext.ForceFlags(context->fontFlags);
	if (oldBits != context->fontContext.BitsPerPixel()) {
		if (context->portLocked && context->renderPort->canvasLocked)
			grPickCanvasRenderers(context);
		else
			context->changedFlags |= DRAWOP_CHANGED;
	};
};

#if 0
// TODO: read a font from the session (called from mwindow.h)
void grSetFontFromSession(RenderContext *context, TSession* a_session, bool read_mask)
{
	int32 oldBits=-1;
	if (!context->fontContext.IsValid())
		SFont::GetStandardFont(B_PLAIN_FONT, &context->fontContext);
	else
		oldBits = context->fontContext.BitsPerPixel();
	context->fontContext.ReadPacket(a_session, read_mask);
	context->fontContext.ForceFlags(context->fontFlags);
	if (oldBits != context->fontContext.BitsPerPixel()) {
		if (context->portLocked && context->renderPort->canvasLocked)
			grPickCanvasRenderers(context);
		else
			context->changedFlags |= DRAWOP_CHANGED;
	};
};
#endif

void grSetFont(RenderContext *context, const SFont& font)
{
	int32 oldBits=-1;
	if (context->fontContext.IsValid()) {
		oldBits = context->fontContext.BitsPerPixel();
	};
	context->fontContext = font;
	context->fontContext.ForceFlags(context->fontFlags);
	context->fontContext.Refresh();
	if (oldBits != context->fontContext.BitsPerPixel()) {
		if (context->portLocked && context->renderPort->canvasLocked)
			grPickCanvasRenderers(context);
		else
			context->changedFlags |= DRAWOP_CHANGED;
	};
};

void grSetForeColor(RenderContext *context, rgb_color fc)
{
	RDebugPrintf((context,"grSetForeColor(%d,%d,%d,%d)\n",
		fc.red,fc.green,fc.blue,fc.alpha));

	context->foreColor = fc;
	if (context->portLocked && context->renderPort->canvasLocked)
		grForeColorChanged(context);
	else
		context->changedFlags |= FORE_COLOR_CHANGED;
};

void grSetBackColor(RenderContext *context, rgb_color bc)
{
	RDebugPrintf((context,"grSetBackColor(%d,%d,%d,%d)\n",
		bc.red,bc.green,bc.blue,bc.alpha));

 	context->backColor = bc;
	if (context->portLocked && context->renderPort->canvasLocked)
		grBackColorChanged(context);
	else
		context->changedFlags |= BACK_COLOR_CHANGED;
};

void grSetLineMode(RenderContext *context, short capRule, short joinRule, float miterLimit)
{
	context->capRule = capRule;
	context->joinRule = joinRule;
	context->miterLimit = miterLimit;
	grLineDependencies(context);
};

void grSetPenSize(RenderContext *context, float penSize)
{
	if (penSize < 0.0) penSize = 0.0;
	context->penSize = penSize;
	context->changedFlags |= PENSIZE_CHANGED;
	context->totalEffective_PenSize = context->totalEffective_Scale*penSize;
	grLineDependencies(context);
	grPickRenderFuncs(context);
};

const char *drawOpName[] =
{
	"B_OP_COPY",
	"B_OP_OVER",
	"B_OP_ERASE",
	"B_OP_INVERT",
	"B_OP_ADD",
	"B_OP_SUB",
	"B_OP_BLEND",
	"B_OP_MIN",
	"B_OP_MAX",
	"B_OP_SELECT",
	"B_OP_ALPHA"
};

const char *srcAlphaSrcName[] =
{
	"B_PIXEL_ALPHA",
	"B_CONSTANT_ALPHA"
};

const char *alphaFuncName[] =
{
	"B_ALPHA_OVERLAY",
	"B_ALPHA_COMPOSITE"
};

void grSetDrawOp(RenderContext *context, short drawOp)
{
	if ((drawOp < B_OP_COPY) || (drawOp > B_OP_ALPHA)) {
		RDebugPrintf((context,"grSetDrawOp() with bad value! (%u)\n", (int)drawOp));
		return;
	}
	RDebugPrintf((context,"grSetDrawOp(%s)\n",drawOpName[drawOp]));
	context->drawOp = drawOp;
	if (context->portLocked && context->renderPort->canvasLocked)
		grPickCanvasRenderers(context);
	else
		context->changedFlags |= DRAWOP_CHANGED;
};

void grSetBlendingMode(RenderContext *context, int16 srcAlphaSrc, int16 alphaFunction)
{
	RDebugPrintf((context,"grSetBlendingMode(%s,%s)\n",
		srcAlphaSrcName[srcAlphaSrc],alphaFuncName[alphaFunction]));
	context->srcAlphaSrc = srcAlphaSrc;
	context->alphaFunction = alphaFunction;
	if (context->portLocked && context->renderPort->canvasLocked)
		grPickCanvasRenderers(context);
	else
		context->changedFlags |= DRAWOP_CHANGED;
};

void grSetStipplePattern(RenderContext *context, pattern *stipplePattern)
{
	RDebugPrintf((context,"grSetStipplePattern(%016Lx)\n",*(int64*)stipplePattern));
	bool needPick;
	if ((!stipplePattern) || (*(uint64*)stipplePattern == 0xFFFFFFFFFFFFFFFFULL)) {
		needPick = context->stippling || (context->drawOp != OP_COPY);
		context->stippling = 0;
		context->solidColor = 0;
	} else if (*(uint64*)stipplePattern == 0x0000000000000000ULL) {
		needPick = context->stippling || (context->drawOp != OP_COPY);
		context->stippling = 0;
		context->solidColor = 1;
	} else {
		needPick = !context->stippling;
		context->stippling = 1;
	};
#if ROTATE_DISPLAY
	context->realStipplePattern = *stipplePattern;
#else
	context->stipplePattern = *stipplePattern;
#endif
	if (needPick) {
		if (context->portLocked && context->renderPort->canvasLocked)
			grPickCanvasRenderers(context);
		else
			context->changedFlags |= STIPPLE_CHANGED;
	};
};

void grSetPenLocation(RenderContext *context, fpoint penLoc)
{
	context->penLocation = penLoc;
	context->changedFlags |= LOCATION_CHANGED;
};

void grMovePenLocation(RenderContext *context, fpoint delta)
{
	context->penLocation += delta;
	context->changedFlags |= LOCATION_CHANGED;
}

void grSetOrigin(RenderContext *context, fpoint origin)
{
	if (context->localOrigin == origin)
		return;
	context->localOrigin = origin;
	context->localEffective_Origin.x = context->totalEffective_Scale * context->localOrigin.x;
	context->localEffective_Origin.y = context->totalEffective_Scale * context->localOrigin.y;
	context->totalEffective_Origin.x = context->globalEffective_Origin.x + context->localEffective_Origin.x;
	context->totalEffective_Origin.y = context->globalEffective_Origin.y + context->localEffective_Origin.y;
	grComputeEffectiveLocalClip(context);
	context->changedFlags |= ORIGIN_CHANGED;
};

void grSetScale(RenderContext *context, float scale)
{
	if (context->localScale == scale) return;

	context->localScale = scale;
	context->totalEffective_Scale = scale * context->globalEffective_Scale;
	context->totalEffective_PenSize = context->penSize * context->totalEffective_Scale;
	grComputeEffectiveLocalClip(context);
	grLineDependencies(context);
	grPickRenderFuncs(context);
};

void grForceFontFlags(RenderContext *context, uint32 flags)
{
	if (flags != context->fontFlags) {
		context->fontFlags = flags;
		// If the flags have changed, make sure the current font
		// is set up correctly by re-applying it.
		grSetFont(context, context->fontContext);
	}
};

/**********************************************************/

RenderSubPort * grNewRenderSubPort()
{
	return (RenderPort*)grMalloc(sizeof(RenderSubPort),"Render port",MALLOC_CANNOT_FAIL);
};

void grFreeRenderSubPort(RenderSubPort *p)
{
	grFree(p);
};

void grInitRenderSubPort(RenderSubPort *port)
{
	port->portClip = newregion();
	port->drawingClip = newregion();
	port->origin = point(0,0);
	port->next = NULL;
#if ROTATE_DISPLAY
	port->canvasIsRotated = false;
	port->rotatedDrawingClip = newregion();
	port->rotater = &(BGraphicDevice::Device(0)->m_rotater);
#endif
};

void grDestroyRenderSubPort(RenderSubPort *port)
{
	kill_region(port->portClip);
	kill_region(port->drawingClip);
#if ROTATE_DISPLAY
	kill_region(port->rotatedDrawingClip);
#endif
};

/**********************************************************/

RenderPort * grNewRenderPort()
{
	return (RenderPort*)grMalloc(sizeof(RenderPort),"Render port",MALLOC_CANNOT_FAIL);
};

void grFreeRenderPort(RenderPort *p)
{
	grFree(p);
};

void grInitRenderPort(RenderPort *port)
{
	grInitRenderSubPort(port);
	port->canvasLockCount = 0;
	port->canvasLocked = false;
};

void grDestroyRenderPort(RenderPort *port)
{
	grDestroyRenderSubPort(port);
};

/**********************************************************/

RenderCanvas * grNewRenderCanvas()
{
	return (RenderCanvas*)grMalloc(sizeof(RenderCanvas),"Render canvas",MALLOC_CANNOT_FAIL);
};

void grFreeRenderCanvas(RenderCanvas *c)
{
	grFree(c);
};

/**********************************************************/

RenderContext * grNewRenderContext()
{
	return (RenderContext*)grMalloc(sizeof(RenderContext),"Render context",MALLOC_CANNOT_FAIL);
};

void grFreeRenderContext(RenderContext *render)
{
	grFree(render);
};

/**********************************************************/
/**************** Context initialization ******************/

void grInitRenderCache(RenderCache *cache)
{
	cache->foreAlphaPix = 0xFF000000;
	cache->foreSrcAlpha = 256;
	cache->backAlphaPix = 0xFF000000;
	cache->backSrcAlpha = 256;
	cache->canvasToken = 0xFFFFFFFF;
	cache->hardwareMask = 0;
	cache->next = NULL;
};

void grInitRenderContext(RenderContext *render)
{
	render->changedFlags = 0xFFFFFFFF;
	render->drawOp = OP_COPY;
	render->srcAlphaSrc = 0;
	render->alphaFunction = 0;
	render->localOrigin.x = render->localOrigin.y = 0.0;
	render->globalEffective_Origin.x = render->globalEffective_Origin.y = 0.0;
	render->localEffective_Origin.x = render->localEffective_Origin.y = 0.0;
	render->totalEffective_Origin.x = render->totalEffective_Origin.y = 0.0;
	*((uint64*)&render->stipplePattern.data[0]) = 0xFFFFFFFFFFFFFFFFLL;
#if ROTATE_DISPLAY
	*((uint64*)&render->realStipplePattern.data[0]) = 0xFFFFFFFFFFFFFFFFLL;
#endif
	render->penSize = 1.0;
	render->penLocation.x = 0;
	render->penLocation.y = 0;
	render->foreColorARGB = 0xFF000000;
	render->backColorARGB = 0xFFFFFFFF;
	render->foreColor = rgb_black;
	render->backColor = rgb_white;
	render->capRule = BUTT_CAP;
	render->joinRule = BEVEL_CAP;
	render->miterLimit = 10.0;
	render->stateFlags = 0;
	render->fontFlags = 0;
	render->stateStack = NULL;

#if USE_FONTS
	new(&render->fontContext) SFont();
#endif
	
	render->stippling = 0;
	render->localScale = 1.0;
	render->totalEffective_Scale = 1.0;
	render->globalEffective_Scale = 1.0;
	render->totalEffective_PenSize = 1.0;

	render->solidColor = 0;

	render->globalEffective_Clip = NULL;
	render->localRegionClip = NULL;
	render->totalEffective_Clip = NULL;
	render->localEffective_Clip = NULL;
	
	render->spanBatch = 32;

	render->hasLock = -1;
	render->portLockCount = 0;
	render->portLocked = false;
	render->lockPersistence = 0;
	render->wasLocked = 0;
	render->ownerThread = -1;

	render->lineThickener = new SThickener(render);
	grLineDependencies(render);
	render->fCoordTransform = NULL;
	render->renderFuncs = &thinPenRenderFuncs;
	grInitRenderCache(&render->cache);
};

/**********************************************************/
/**************** Context destruction *********************/

void grDestroyRenderContext(RenderContext *render)
{
#if USE_FONTS
	render->fontContext.~SFont();
#endif
	if (render->globalEffective_Clip) kill_region(render->globalEffective_Clip);
	if (render->localEffective_Clip) kill_region(render->localEffective_Clip);
	if (render->totalEffective_Clip) kill_region(render->totalEffective_Clip);
	if (render->localRegionClip) kill_region(render->localRegionClip);
	if (render->lineThickener) delete render->lineThickener;

	while (render->stateStack) {
		_sPrintf("stateStack non-null!\n");
		RenderState *state = render->stateStack;
		render->stateStack = state->next;
		delete state;
	};
};

/**********************************************************/
/**************** Flags ***********************************/

void grSetOwnerThread(RenderContext *context, int32 owner)
{
	context->ownerThread = owner;
};

void grSetRounding(RenderContext *context, bool on)
{
	context->stateFlags = (context->stateFlags & ~RENDER_ROUND_COORDS);
	if (on) context->stateFlags |= RENDER_ROUND_COORDS;
	grPickRenderFuncs(context);
};

/**********************************************************/
/**************** State persistance ***********************/

void grSaveState(RenderContext *context, RenderState *state)
{
	state->localOrigin = context->localOrigin;
	state->globalEffective_Origin = context->globalEffective_Origin;
	state->localScale = context->localScale;
	state->globalEffective_Scale = context->globalEffective_Scale;
	state->drawOp = context->drawOp;
	state->srcAlphaSrc = context->srcAlphaSrc;
	state->alphaFunction = context->alphaFunction;
	state->capRule = context->capRule;
	state->joinRule = context->joinRule;
	state->miterLimit = context->miterLimit;
	state->penSize = context->penSize;
	state->penLocation = context->penLocation;
	state->foreColor = context->foreColor;
	state->backColor = context->backColor;
#if ROTATE_DISPLAY
	state->stipplePattern = context->realStipplePattern;
#else
	state->stipplePattern = context->stipplePattern;
#endif	

	grCopyRegion(&context->globalEffective_Clip,&state->globalEffective_Clip);
	grCopyRegion(&context->localRegionClip,&state->localRegionClip);

#if USE_FONTS
	state->fontContext = context->fontContext;
#endif
};

void grRestoreState(RenderContext *context, RenderState *state)
{
	context->globalEffective_Origin = state->globalEffective_Origin;
	grSetOrigin(context,state->localOrigin);
	context->globalEffective_Scale = state->globalEffective_Scale;
	grSetScale(context,state->localScale);

	grSetDrawOp(context,state->drawOp);
	grSetBlendingMode(context,state->srcAlphaSrc,state->alphaFunction);
	grSetLineMode(context,state->capRule,state->joinRule,state->miterLimit);
	grSetPenSize(context,state->penSize);
	grSetPenLocation(context,state->penLocation);
	grSetForeColor(context,state->foreColor);
	grSetBackColor(context,state->backColor);
	grSetStipplePattern(context,&state->stipplePattern);
	
	region *oldRegionClip = context->localRegionClip;

	grCopyRegion(&state->globalEffective_Clip,&context->globalEffective_Clip);
	grCopyRegion(&state->localRegionClip,&context->localRegionClip);

	if (oldRegionClip || context->localRegionClip)
		grComputeEffectiveLocalClip(context);

	grSetFont(context,state->fontContext);
};

/**********************************************************/
/**************** State stack *****************************/

void grPushState(RenderContext *context)
{
	RDebugPrintf((context,"grPushState\n"));

#if USE_FONTS
	// Make sure font is synchronized with global settings.
	if (context->fontContext.IsValid() && context->fontContext.Refresh()) {
		if (context->portLocked && context->renderPort->canvasLockCount)
			grPickCanvasRenderers(context);
		else
			context->changedFlags |= DRAWOP_CHANGED;
	}
#endif

	RenderState *state = new RenderState;
	grSaveState(context,state);
	grMergeClip(context);
	grMergeScale(context);
	grMergeOrigin(context);
	state->next = context->stateStack;
	context->stateStack = state;
}

void grPopState(RenderContext *context)
{
	RenderState *state = context->stateStack;
	if (state == NULL) {
		_sPrintf("can't pop -- stack is empty!\n");
	} else {
		context->stateStack = state->next;
		grRestoreState(context,state);
		delete state;
	}
	RDebugPrintf((context,"grPopState\n"));
}
