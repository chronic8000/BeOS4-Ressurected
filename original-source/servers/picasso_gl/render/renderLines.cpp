
//******************************************************************************
//
//	File:			renderLines.cpp
//	Description:	Line rendering commands
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
#include "rect.h"

#include <support2/Debug.h>

static int32 shiftForPixFormat[] = { 0, 0, 1, 1, 2 };
#define top_code 	0x01
#define bottom_code	0x02
#define left_code	0x04
#define right_code	0x08

#define SPAN_BATCH 32

void grDrawManyLines
(
	RenderContext *context,
	fpoint *ptList, int32 numPts, fpoint preOrigin,
	bool closed, a_line *theLineArray=NULL
)
{
	float fx0,fy0,fx1,fy1;
	int32 x0,y0,x1,y1,dx,dy,xexcess,yexcess,start,end;
	int32 numPixels,xStart,yStart,xInc,yInc,count,ptCount;
	bool startIn, endIn, lastLine, endClipped;
	int32 spans[SPAN_BATCH*3],spanCount;
	uint32 code1,code2,code;
	a_line *lineArray;
	fpoint *pts;
	float slope;
	fpoint origin;
	rect r;

	RenderSubPort *port = context->renderPort;

	// 12/16/2000 - mathias: (see below)
	const uint32 ctxtForeColor = *((uint32*)&context->foreColor);

	while (port) {
		frect bounds;
		DrawOneLine drawOneLine = port->cache->fDrawOneLine;
		uint8 *base = (uint8*)port->canvas->pixels.pixelData;
		int32 bpr = port->canvas->pixels.bytesPerRow;
		const rect *clip = port->RealDrawingRegion()->Rects();
		int32 clipCount = port->RealDrawingRegion()->CountRects();
		origin.x = preOrigin.x + port->origin.x;
		origin.y = preOrigin.y + port->origin.y;

		// 12/16/2000 - mathias:
		// Reset the context's color to its original value (because the current port still has this color)
		// (see below)
		*((uint32*)&context->foreColor) = ctxtForeColor;
	
		while (clipCount--) {
			bounds.top = clip->top - 0.5;
			bounds.left = clip->left - 0.5;
			bounds.bottom = clip->bottom + 0.4995;
			bounds.right = clip->right + 0.4995;
			
			pts = ptList;
			ptCount = numPts;
			lineArray = theLineArray;
			lastLine = true;
			if (!lineArray) {
				ptCount--;
				lastLine = false;
			};
			while (ptCount-- > 0) {
				if (lineArray) {
					// 12/16/2000 - mathias: Here, we actually don't know the 'forecolor' used by the current 'port'
					// because during the last loop we set the forecolor _only_ for the previous port
					// and not for all ports (reread slowly :} ).
					// Actually, I lied, we know the forecolor of that port, it's the value 'context->foreColor' had
					// when we entered this function (it has been modified since then) [ctxtForeColor]
				
					const uint32 lineColor = *((uint32*)&lineArray->col);
					const uint32 lastColor = *((uint32*)&context->foreColor);
					if (lastColor != lineColor)
					{
						rgb_color fc = context->foreColor = lineArray->col;
						context->foreColorARGB =
							(((uint32)fc.alpha)<<24)|
							(((uint32)fc.red)<<16)|
							(((uint32)fc.green)<<8)|
							(((uint32)fc.blue));
						port->canvas->renderingPackage->ForeColorChanged(context,port);
					};
#if ROTATE_DISPLAY
					if (port->canvasIsRotated) {
						fy0 = port->rotater->RotateX(lineArray->x0 + origin.x);
						fy1 = port->rotater->RotateX(lineArray->x1 + origin.x);
						fx0 = port->rotater->RotateY(lineArray->y0 + origin.y);
						fx1 = port->rotater->RotateY(lineArray->y1 + origin.y);
					} else
#endif
					{
						fx0 = lineArray->x0 + origin.x;
						fx1 = lineArray->x1 + origin.x;
						fy0 = lineArray->y0 + origin.y;
						fy1 = lineArray->y1 + origin.y;
					}
					lineArray++;
				} else {
#if ROTATE_DISPLAY
					if (port->canvasIsRotated) {
						fy0 = port->rotater->RotateX(pts[0].x + origin.x);
						fx0 = port->rotater->RotateY(pts[0].y + origin.y);
						fy1 = port->rotater->RotateX(pts[1].h + origin.x);
						fx1 = port->rotater->RotateY(pts[1].v + origin.y);
					} else
#endif
					{
						fx0 = pts[0].x + origin.x;
						fy0 = pts[0].y + origin.y;
						fx1 = pts[1].x + origin.x;
						fy1 = pts[1].y + origin.y;
					}
					if (!ptCount && !closed) lastLine = true;
					pts++;
				};
		
				backIn:
		
				// 10/30/2000 - mathias: here we write the clipping test so that
				// lines with NAN coordinates are _rejected_ from the visible region
				// This fixes #20000331-18552 : "a StrokeLine with nan crashes the system"
				code1 = code2 = bottom_code | right_code | left_code | top_code;

				if (fx0 <= bounds.right)	code1 &= ~right_code;
				if (fy0 <= bounds.bottom)	code1 &= ~bottom_code;
				if (fx0 >= bounds.left)		code1 &= ~left_code;
				if (fy0 >= bounds.top)		code1 &= ~top_code;
				if (fx1 <= bounds.right)	code2 &= ~right_code;
				if (fy1 <= bounds.bottom)	code2 &= ~bottom_code;
				if (fx1 >= bounds.left)		code2 &= ~left_code;
				if (fy1 >= bounds.top)		code2 &= ~top_code;
				if (code1 & code2) continue;
		
				// 12/17/2000 - mathias: The 10/30/2000 fix against NAN was not good enough.
				// It was working only if both line ends X or Y were NAN. For example, if only fx0
				// was NAN, the line was not rejected (leading to bad things like crashes or infinite loops)
				// Also inf values must be rejected. Here, we check if x or y is not in visible range and in this
				// case we check if this is a finite() number; if not, don't try to draw this line.

				if ((code1 & (right_code|left_code)) && (!finitef(fx0)))	continue;
				if ((code1 & (top_code|bottom_code)) && (!finitef(fy0)))	continue;
				if ((code2 & (right_code|left_code)) && (!finitef(fx1)))	continue;
				if ((code2 & (top_code|bottom_code)) && (!finitef(fy1)))	continue;
		
				/* Top level clipping */
				endClipped = false;
				if (code1 | code2) {
					count = 0;
					again:
					if (code1 & bottom_code) {
						fx0 += (bounds.bottom-fy0)*(fx1-fx0)/(fy1-fy0);
						fy0 = bounds.bottom;
					} else if (code1 & top_code) {
						fx0 += (bounds.top-fy0)*(fx1-fx0)/(fy1-fy0);
						fy0 = bounds.top;
					} else if (code1 & left_code) {
						fy0 += (bounds.left-fx0)*(fy1-fy0)/(fx1-fx0);
						fx0 = bounds.left;
					} else if (code1 & right_code) {
						fy0 += (bounds.right-fx0)*(fy1-fy0)/(fx1-fx0);
						fx0 = bounds.right;
					} else {
						slope = fx0;
						fx0 = fx1;
						fx1 = slope;
						slope = fy0;
						fy0 = fy1;
						fy1 = slope;
						code = code1;
						code1 = code2;
						code2 = code;
						endClipped = true;
						count++;
						goto again;
					};

					code1 = bottom_code | right_code | left_code | top_code;
					if (fy0 <= bounds.bottom)	code1 &= ~bottom_code;
					if (fx0 <= bounds.right)	code1 &= ~right_code;
					if (fx0 >= bounds.left)		code1 &= ~left_code;
					if (fy0 >= bounds.top)		code1 &= ~top_code;
					if (code1 & code2) continue;
					if (code1 | code2) goto again;
					if (count) {
						slope = fx0;
						fx0 = fx1;
						fx1 = slope;
						slope = fy0;
						fy0 = fy1;
						fy1 = slope;
					};
				};
		
				drawIt:
		
				x0 = (int32)(fx0 * 65536.0);
				y0 = (int32)(fy0 * 65536.0);
				x1 = (int32)(fx1 * 65536.0);
				y1 = (int32)(fy1 * 65536.0);
		
				dx=x1-x0;
				dy=y1-y0;
				
				if (!dx && !dy) {
					if (!lastLine) continue;
					drawOneLine(context,port,base,bpr,1,
						(x0+0x8000)&0xFFFF0000,
						(y0+0x8000)&0xFFFF0000,
						0,0);
					continue;
				};
					
				xexcess = x0 - ((x0 + 0x8000) & 0xFFFF0000);
				if (xexcess < 0) xexcess = -xexcess;
				yexcess = y0 - ((y0 + 0x8000) & 0xFFFF0000);
				if (yexcess < 0) yexcess = -yexcess;
				startIn = ((xexcess + yexcess) < 0x8000);
			
				xexcess = x1 - ((x1 + 0x8000) & 0xFFFF0000);
				if (xexcess < 0) xexcess = -xexcess;
				yexcess = y1 - ((y1 + 0x8000) & 0xFFFF0000);
				if (yexcess < 0) yexcess = -yexcess;
				endIn = ((xexcess + yexcess) < 0x8000);
			
			    if (abs((int)dx) >= abs((int)dy)) {
					if (dx > 0) {
						if (startIn) start = (x0+0x8000) >> 16;
						else start = (x0 >> 16) + 1;
						if (endIn) end = ((x1+0x8000) >> 16) - !(lastLine || endClipped);
						else end = (x1 >> 16);
						if (start > end) continue;
					} else {
						if (startIn) start = (x0+0x8000) >> 16;
						else start = (x0 >> 16);
						if (endIn) end = ((x1+0x8000) >> 16) + !(lastLine || endClipped);
						else end = (x1 >> 16) + 1;
						if (start < end) continue;
					};
		
					slope = ((float)(y1-y0))/(x1-x0);
					numPixels = abs((int)(end-start))+1;
					xStart = (start << 16);
					yStart = y0 + (int32)(slope * (xStart - x0)) + 0x8000;
					xStart += 0x8000;
					if (end>start)	xInc = 0x10000;
					else			xInc = -0x10000;
					yInc = (int32)(xInc * slope);
			    } else {
					if (dy > 0) {
						if (startIn) start = (y0+0x8000) >> 16;
						else start = (y0 >> 16) + 1;
						if (endIn) end = ((y1+0x8000) >> 16) - !(lastLine || endClipped);
						else end = (y1 >> 16);
						if (start > end) continue;
					} else {
						if (startIn) start = (y0+0x8000) >> 16;
						else start = (y0 >> 16);
						if (endIn) end = ((y1+0x8000) >> 16) + !(lastLine || endClipped);
						else end = (y1 >> 16) + 1;
						if (start < end) continue;
					};
		
					slope = ((float)(x1-x0))/(y1-y0);
					numPixels = abs((int)(end-start))+1;
					yStart = (start << 16);
					xStart = x0 + (int32)(slope * (yStart - y0)) + 0x8000;
					yStart += 0x8000;
					if (end>start)	yInc = 0x10000;
					else			yInc = -0x10000;
					xInc = (int32)(yInc * slope);
			    }
		
				drawOneLine(context,port,base,bpr,numPixels,xStart,yStart,xInc,yInc);
			};
		
			if (!lineArray && closed && (ptCount==-1)) {
#if ROTATE_DISPLAY
				if (port->canvasIsRotated) {
					fy0 = port->rotater->RotateX(pts[0].x + origin.x);
					fx0 = port->rotater->RotateY(pts[0].y + origin.y);
					fy1 = port->rotater->RotateX(ptList[1].x + origin.x);
					fx1 = port->rotater->RotateY(ptList[1].y + origin.y);
				} else
#endif
				{
					fx0 = pts[0].x + origin.x;
					fy0 = pts[0].y + origin.y;
					fx1 = ptList[0].x + origin.x;
					fy1 = ptList[0].y + origin.y;
				}
				lastLine = true;
				pts++;
				goto backIn;
			};
	
			clip++;
		};
		port = port->next;
	};
}

/********************************************************/

void grStrokeLineArray_Fat(
	RenderContext *context,
	a_line *lineArray, int32 lineArrayCount)
{
	rgb_color oldCol;
	grGetForeColor(context,&oldCol);
	while (lineArrayCount--) {
		grSetForeColor(context,lineArray->col);
		grStrokeLine(context,*((fpoint*)&lineArray->x0),*((fpoint*)&lineArray->x1));
		lineArray++;
	};
	grSetForeColor(context,oldCol);
};

void grStrokeLineArray_Thin(
	RenderContext *context,
	a_line *lineArray, int32 lineArrayCount)
{
	rgb_color oldCol,otherCol;
	pattern oldPat;
	uint64 black = 0xFFFFFFFFFFFFFFFFLL;
	
	if (context->fCoordTransform)
		for (int32 i=0;i<lineArrayCount;i++)
			context->fCoordTransform(context,(fpoint*)&lineArray[i].x0,2);
	
	grGetForeColor(context,&oldCol);
	grGetStipplePattern(context,&oldPat);
	if (*((uint64*)&oldPat) != black)
		grSetStipplePattern(context,(pattern*)(&black));

	fpoint origin = context->totalEffective_Origin;

	RenderPort *port = grLockPort(context);
		grLockCanvi(port,&port->drawingBounds,hwLine);
			grDrawManyLines(context,NULL,lineArrayCount,origin,false,lineArray);
		grUnlockCanvi(port);
	grUnlockPort(context);
	
	if (*((uint64*)&oldPat) != black)
		grSetStipplePattern(context,&oldPat);

	grGetForeColor(context,&otherCol);
	if (*((uint32*)&otherCol) != *((uint32*)&oldCol))
		grSetForeColor(context,oldCol);
};

/********************************************************/

void grStrokeLine_Fat(
	RenderContext *context,
	fpoint p1, fpoint p2)
{
	RDebugPrintf((context,"grStrokeLine_Fat(%f,%f,%f,%f)\n",p1.h,p1.v,p2.h,p2.v));

	if (p1 == p2) {
		if (context->capRule == ROUND_CAP) {
			float halfPen = context->totalEffective_PenSize/2.0;
			fpoint fp(halfPen, halfPen);
			grFillEllipse_FatThin(context,p1,fp,true);
		} else if (context->capRule == SQUARE_CAP) {
			frect fr;
			float halfPen = context->totalEffective_PenSize/2.0;
			fr.left = p1.x - halfPen;
			fr.top = p1.y - halfPen;
			fr.right = fr.left + context->totalEffective_PenSize;
			fr.bottom = fr.top + context->totalEffective_PenSize;
			grFillRect_FatThin(context,fr);
		}
	} else {
		if (context->fCoordTransform) {
			context->fCoordTransform(context,&p1,1);
			context->fCoordTransform(context,&p2,1);
		}
		context->lineThickener->Clear();
		context->lineThickener->OpenShapes(p1);
		context->lineThickener->AddLineSegments(&p2,1);
		context->lineThickener->CloseShapes(false);
		grLockPort(context);
		grFillPath_Internal(context,context->lineThickener,true,false);
		grUnlockPort(context);
	}
}

void grStrokeLine_Thin(
	RenderContext *context,
	fpoint p1, fpoint p2)
{
	RDebugPrintf((context,"grStrokeLine_Thin(%f,%f,%f,%f)\n",p1.h,p1.v,p2.h,p2.v));

	fpoint p[2];

	if (context->fCoordTransform) {
		context->fCoordTransform(context,&p1,1);
		context->fCoordTransform(context,&p2,1);
	};

	fpoint origin = context->totalEffective_Origin;
	RenderPort *port = grLockPort(context);
		if (p1.y == p2.y) {
			grLockCanvi(port,&port->drawingBounds,hwRectFill);
			p1 += origin;
			p2 += origin;
			rect ri;
			ri.top = ri.bottom = (int32)floor(p1.y+0.5);
			if (p1.x < p2.x) {
				ri.left = (int32)floor(p1.x+0.5);
				ri.right = (int32)floor(p2.x+0.5);
			} else {
				ri.left = (int32)floor(p2.x+0.5);
				ri.right = (int32)floor(p1.x+0.5);
			};
#if ROTATE_DISPLAY
			if (port->canvasIsRotated)
				port->rotater->RotateRect(&ri, &ri);
#endif
			grFillRects(context,port,&ri,1);
		} else if (p1.x == p2.x) {
			grLockCanvi(port,&port->drawingBounds,hwRectFill);
			p1 += origin;
			p2 += origin;
			rect ri; ri.left = ri.right = (int32)floor(p1.x+0.5);
			ri.top = (int32)floor(p1.y+0.5);
			ri.bottom = (int32)floor(p2.y+0.5);
			if (ri.top > ri.bottom) {
				int32 t = ri.top;
				ri.top = ri.bottom;
				ri.bottom = t;
			};
#if ROTATE_DISPLAY
			if (port->canvasIsRotated)
				port->rotater->RotateRect(&ri, &ri);
#endif
			grFillRects(context,port,&ri,1);
		} else {
			grLockCanvi(port,&port->drawingBounds,hwLine);
			p[0] = p1;
			p[1] = p2;
			grDrawManyLines(context,p,2,origin,false);
		};
		grUnlockCanvi(port);
	grUnlockPort(context);
};


/********************************************************/

void grStrokeILine(
	RenderContext *context,
	int32 x1, int32 y1,
	int32 x2, int32 y2)
{
	point origin;
	origin.x = (int32)floor(context->totalEffective_Origin.x+0.5);
	origin.y = (int32)floor(context->totalEffective_Origin.y+0.5);
	x1 += origin.x;
	y1 += origin.y;
	x2 += origin.x;
	y2 += origin.y;

	RenderPort *port = grLockPort(context);
		if (y1 == y2) {
			grLockCanvi(port,&port->drawingBounds,hwRectFill);
			rect ri;
			ri.top = ri.bottom = y1;
			if (x1 < x2) {
				ri.left = x1;
				ri.right = x2;
			} else {
				ri.left = x2;
				ri.right = x1;
			};
#if ROTATE_DISPLAY
			if (port->canvasIsRotated)
				port->rotater->RotateRect(&ri, &ri);
#endif
			grFillRects(context,port,&ri,1);
		} else if (x1 == x2) {
			grLockCanvi(port,&port->drawingBounds,hwRectFill);
			rect r;
			r.left = r.right = x1;
			if (y1 < y2) {
				r.top = y1;
				r.bottom = y2;
			} else {
				r.top = y2;
				r.bottom = y1;
			};
#if ROTATE_DISPLAY
			if (port->canvasIsRotated)
				port->rotater->RotateRect(&r, &r);
#endif
			grFillRects(context,port,&r,1);
		} else {
			grLockCanvi(port,&port->drawingBounds,hwLine);
			fpoint p[2];
			fpoint fakeOrigin(0,0);
			p[0] = fpoint(x1, y1);
			p[1] = fpoint(x2, y2);
			grDrawManyLines(context,p,2,fakeOrigin,false);
		};
		grUnlockCanvi(port);
	grUnlockPort(context);
};
