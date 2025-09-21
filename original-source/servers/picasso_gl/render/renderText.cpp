
//******************************************************************************
//
//	File:			renderText.cpp
//	Description:	Text rendering commands
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
#include "fastmath.h"
#include "render.h"
#include "rect.h"
#include "as_debug.h"
#include <support2/Debug.h>

#define SHOW_STRINGS 0

/* A utility routine */
void grTruncateString(RenderContext *context,
	float targetWidth, int32 mode, const uchar *srcStr, uchar *dstStr)
{
#if USE_FONTS
	if (!context->fontContext.IsValid()) SFont::GetStandardFont(B_PLAIN_FONT, &context->fontContext);
	context->fontContext.TruncateString(targetWidth, mode, srcStr, dstStr);
#endif
};

float defaultStringDeltas[2] = {0.0, 0.0};

#if AS_DEBUG
static const char *drawOpName[] =
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

static const char *srcAlphaSrcName[] =
{
	"B_PIXEL_ALPHA",
	"B_CONSTANT_ALPHA"
};

static const char *alphaFuncName[] =
{
	"B_ALPHA_OVERLAY",
	"B_ALPHA_COMPOSITE"
};

void grDumpState(RenderContext *context)
{
	DebugPrintf(("-- Context dump ----------------------------------------------------\n"));
	DebugPrintf(("  Drawing op:           %s\n",drawOpName[context->drawOp]));
	DebugPrintf(("  Source alpha source:  %s\n",srcAlphaSrcName[context->srcAlphaSrc]));
	DebugPrintf(("  Alpha function:       %s\n",alphaFuncName[context->alphaFunction]));
	DebugPrintf(("  Fore color:           {%d,%d,%d,%d}\n",	context->foreColor.red,context->foreColor.green,
															context->foreColor.blue,context->foreColor.alpha));
	DebugPrintf(("  Back color:           {%d,%d,%d,%d}\n",	context->backColor.red,context->backColor.green,
															context->backColor.blue,context->backColor.alpha));
	DebugPrintf(("  Stipple pattern:      %016Lx\n",*((uint64*)&context->stipplePattern)));
	DebugPrintf(("--------------------------------------------------------------------\n"));
};
#endif

void grDrawString_FatThin(
	RenderContext *context,
	const uchar *string,
	float *deltas)
{
	RDebugPrintf((context,"grDrawString_FatThin(\"%s\")\n",string));

#if AS_DEBUG
	if (strcmp((char*)string,"DEBUG:STATE_DUMP") == 0) {
		grDumpState(context);
		return;
	} else if (strcmp((char*)string,"DEBUG:PRINT_ON") == 0) {
		grSetDebugPrinting(context,true);
		return;
	} else if (strcmp((char*)string,"DEBUG:PRINT_OFF") == 0) {
		grSetDebugPrinting(context,false);
		return;
	};
#endif
	
	bool needPick = false;
	if (!context->fontContext.IsValid()) {
		SFont::GetStandardFont(B_PLAIN_FONT, &context->fontContext);
		needPick = true;
	};
	SFont& font = context->fontContext;
	float origSize = 0;
	float dnsp,dsp,scale=context->totalEffective_Scale;
	point origin;
	fc_string draw;
	int32 i;
	double xx, yy;
	int32 dxx, dyy;
	region *theclip;
	rect cursor_rect,tmpR;
	point diff,thisOrigin,lastOrigin;
	
	if (scale != 1.0) {
		// We need to worry about a couple situations here:
		// * A change in size may change the bpp used by the font,
		//   requiring that we update the renderers.
		// * If the context has anti-aliasing turned off, we can't
		//   allow a change in size to turn it back on.
		const int oldBits = font.BitsPerPixel();
		origSize = font.Size();
		font.SetSize(origSize*scale);
		dnsp = deltas[0]*scale;
		dsp = deltas[1]*scale;
		font.SetOrigin(	context->penLocation.x*scale,
						context->penLocation.y*scale);
		if (oldBits != font.BitsPerPixel())
			grPickCanvasRenderers(context);
	} else {
		dnsp = deltas[0];
		dsp = deltas[1];
		font.SetOrigin(	context->penLocation.x,
						context->penLocation.y);
	};
	
	xx = yy = 0.5;
	dxx = (int32)floor(font.OriginX()+0.5);
	dyy = (int32)floor(font.OriginY()+0.5);
	
	#if SHOW_STRINGS
	bool infoShown = false;
	const uchar *orig_string = string;
	if ((context->stateFlags&RENDER_NO_AA_FONTS) != 0 && font.BitsPerPixel() != 1) {
		DebugPrintf(("Inconsistent font bpp in context %p!\n", context));
		debugger("Kill me!");
	}
	#endif
	
	while (string != NULL) {
		RenderPort *ueberPort = grLockPort(context);
		grLockCanvi(ueberPort,NULL,hwText);

		if (needPick) grPickCanvasRenderers(context);
		string = font.LockString(&draw, string, dnsp, dsp, &xx, &yy);

		origin.x = (int32)(context->totalEffective_Origin.x + dxx);
		origin.y = (int32)(context->totalEffective_Origin.y + dyy);

		lastOrigin = point(0,0);
		RenderSubPort *port = ueberPort;
		while (port) {
			theclip = port->drawingClip;
	
			thisOrigin.x = origin.x + port->origin.x;
			thisOrigin.y = origin.y + port->origin.y;
			diff.x = thisOrigin.x - lastOrigin.x;
			diff.y = thisOrigin.y - lastOrigin.y;
	
			tmpR.left = draw.bbox.left + diff.x;
			tmpR.right = draw.bbox.right + diff.x;
			tmpR.top = draw.bbox.top + diff.y;
			tmpR.bottom = draw.bbox.bottom + diff.y;
		
			cursor_rect.right  = min(tmpR.right,  theclip->Bounds().right);
			cursor_rect.bottom = min(tmpR.bottom, theclip->Bounds().bottom);
			cursor_rect.top    = max(tmpR.top,    theclip->Bounds().top);
			cursor_rect.left   = max(tmpR.left,   theclip->Bounds().left);
			
			if ((theclip->Bounds().bottom >= tmpR.top) &&
				(theclip->Bounds().top <= tmpR.bottom) &&
				(theclip->Bounds().left <= tmpR.right) &&
				(theclip->Bounds().right >= tmpR.left)) {
				
				draw.bbox = tmpR;
				for (i=0; i<draw.char_count; i++) {
					draw.offsets[i].x += diff.x;
					draw.offsets[i].y += diff.y;
				}
				
				#if SHOW_STRINGS
				if (!infoShown) {
					printf("Drawing string %s: bpp=%d, mode=%d, non-aa=%d\n",
							(const char*)orig_string, font.BitsPerPixel(), context->drawOp,
							(context->stateFlags & RENDER_NO_AA_FONTS) ? true : false);
					infoShown = true;
				}
				printf("Drawing into port %p, func is %p\n",
						port, port->cache->fDrawString);
				#endif
				
				lastOrigin = thisOrigin;
				port->cache->fDrawString(context,port,&draw);
			}
			
			port = port->next;
		};
		
		font.UnlockString();

		grUnlockCanvi(ueberPort);
		grUnlockPort(context);
	}
	
	font.SetOrigin(font.OriginX()+(xx-0.5), font.OriginY()+(yy-0.5));
	
	if (scale != 1.0) {
		const int oldBits = font.BitsPerPixel();
		font.SetSize(origSize);
		context->penLocation.x = font.OriginX() / scale;
		context->penLocation.y = font.OriginY() / scale;
		if (oldBits != font.BitsPerPixel())
			grPickCanvasRenderers(context);
	} else {
		context->penLocation.x = font.OriginX();
		context->penLocation.y = font.OriginY();
	};

};
