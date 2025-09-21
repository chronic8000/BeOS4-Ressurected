
//******************************************************************************
//
//	File:			newArc.cpp
//	Description:	Ellipse, arc and roundrect drawing
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
#include "edges.h"
#include "lines.h"
#include "fastmath.h"
#include "renderArc.h"
#include "render.h"
#include "rect.h"

#include <support2/Debug.h>

#define min(a,b) (((a)>(b))?(b):(a))
#define max(a,b) (((a)<(b))?(b):(a))
#define ARC_FUDGE_FACTOR 0.4995
#define SPAN_BATCH 64

void SetupArc(
	fpoint center, fpoint radius, 
	float startAng, float arcLen,
	arc &a, arc &b, arc &c, int32 &arcSections,
	fpoint &start, fpoint &stop,
	bool forFill,
	fpoint *last, fpoint */*next*/)
{
	float 	f1,sn,cn,c1,c3,stopAng;
	int32 	top,bottom,
			aTop,aBot,bTop,bBot;

	c1 = radius.x*radius.x;
	c3 = c1/(radius.y*radius.y);
	a.cx = b.cx = c.cx = center.x;
	a.cy = b.cy = c.cy = center.y;

	if (forFill) {
		f1 = a.cy - radius.y;
		top = (int32)floor(f1);
		if (top != f1) top++;
		f1 = a.cy + radius.y;
		bottom = (int32)floor(f1);
		if (bottom == f1) bottom--;
		a.yTop = top;
		a.yBot = bottom;
	} else {
		a.yTop = top = (int32)floor(a.cy - radius.y + 0.5);
		a.yBot = bottom = (int32)floor(a.cy + radius.y + 0.5);
	};
	stop.x = start.x = center.x + radius.x;
	stop.y = start.y = center.y;

	if (fabs(arcLen) >= M_PI*2.0) {
		arcSections = 1;
		a.yTop = top;
		a.yBot = bottom;
		a.sides = ARC_LEFT | ARC_RIGHT;
	} else {
		if (arcLen > 0) {
			startAng = fmod(startAng,M_PI*2.0);
			stopAng = fmod(startAng+arcLen,M_PI*2.0);
		} else {
			stopAng = fmod(startAng,M_PI*2.0);
			startAng = fmod(startAng+arcLen,M_PI*2.0);
		};
		if (startAng < 0) startAng += M_PI*2.0;
		if (stopAng < 0) stopAng += M_PI*2.0;

		b_get_cos_sin(startAng,&cn,&sn);
		start.y = a.cy - radius.y * sn;
		start.x = a.cx + radius.x * cn;
		b_get_cos_sin(stopAng,&cn,&sn);
		stop.y = a.cy - radius.y * sn;
		stop.x = a.cx + radius.x * cn;

		if (sgn(start.x-a.cx) != sgn(stop.x-a.cx)) {
			/* The arc can be drawn in no more than 2 pieces.  Figure out what they are. */
			a.sides = ARC_LEFT|ARC_RIGHT;
			if (start.x < stop.x) {
				/* The arc goes through 3*PI/2 */
				a.yBot = bottom;
				aTop = (int32)floor(start.y);
				bTop = (int32)floor(stop.y);
				if (aTop != start.y) aTop++;
				if (bTop != stop.y) bTop++;
				
				if (start.y < stop.y) {
					b.yTop = aTop;
					b.yBot = bTop-1;
					a.yTop = bTop;
					b.sides = ARC_LEFT;
				} else {
					b.yTop = bTop;
					b.yBot = aTop-1;
					a.yTop = aTop;
					b.sides = ARC_RIGHT;
				};
				arcSections = 1 + (b.yBot >= b.yTop);
			} else {
				/* The arc goes through PI/2 */
				a.yTop = top;
				aBot = (int32)floor(start.y);
				bBot = (int32)floor(stop.y);
				if (aBot == start.y) aBot--;
				if (bBot == stop.y) bBot--;

				if (start.y < stop.y) {
					a.yBot = aBot;
					b.yTop = aBot+1;
					b.yBot = bBot;
					b.sides = ARC_LEFT;
				} else {
					a.yBot = bBot;
					b.yTop = bBot+1;
					b.yBot = aBot;
					b.sides = ARC_RIGHT;
				};
				arcSections = 1 + (b.yBot >= b.yTop);
			};
		} else if (fabs(arcLen) <= M_PI) {
			/* The arc can be drawn in one piece.  */
			if (start.y < stop.y) {
				a.yTop = (int32)floor(start.y);
				a.yBot = (int32)floor(stop.y);
				if (a.yTop != start.y) a.yTop++;
				if (a.yBot == stop.y) a.yBot--;
			} else {
				a.yTop = (int32)floor(stop.y);
				a.yBot = (int32)floor(start.y);
				if (a.yTop != stop.y) a.yTop++;
				if (a.yBot == start.y) a.yBot--;
			};
			a.sides = (start.y < stop.y)?ARC_LEFT:ARC_RIGHT;
			arcSections = 1;
		} else {
			a.yTop = top;
			c.yBot = bottom;
			a.sides = c.sides = ARC_LEFT|ARC_RIGHT;
			b.sides = (start.y < stop.y)?ARC_LEFT:ARC_RIGHT;
			if (start.y < stop.y) {
				c.yTop = (int32)floor(stop.y);
				a.yBot = (int32)floor(start.y);
				if (c.yTop != stop.y) c.yTop++;
				if (a.yBot == start.y) a.yBot--;
			} else {
				c.yTop = (int32)floor(start.y);
				a.yBot = (int32)floor(stop.y);
				if (c.yTop != start.y) c.yTop++;
				if (a.yBot == stop.y) a.yBot--;
			};
			b.yTop = a.yBot+1;
			b.yBot = c.yTop-1;
			arcSections = 3;
		};
	};

	a.c1 = b.c1 = c.c1 = c1;
	a.c3 = b.c3 = c.c3 = c3;
};

void grInscribeStrokeArc_FatThin(
	RenderContext *context,
	frect r, float arcBegin, float arcLen)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	fpoint center,radius;
	center.x = (r.left + (r.right-r.left)/2.0);
	center.y = (r.top + (r.bottom-r.top)/2.0);
	radius.x = (r.right-center.x);
	radius.y = (r.bottom-center.y);
	if (context->totalEffective_PenSize > 1.0)
		grStrokeArc_Fat(context,center,radius,arcBegin,arcLen,false);
	else
		grStrokeArc_Thin(context,center,radius,arcBegin,arcLen,false);
};

void grInscribeStrokeEllipse_FatThin(
	RenderContext *context, frect r)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	fpoint center,radius;
	center.x = (r.left + (r.right-r.left)/2.0);
	center.y = (r.top + (r.bottom-r.top)/2.0);
	radius.x = (r.right-center.x);
	radius.y = (r.bottom-center.y);
	if (context->totalEffective_PenSize > 1.0)
		grStrokeEllipse_Fat(context,center,radius,false);
	else
		grStrokeEllipse_Thin(context,center,radius,false);
};

void grInscribeFillArc_FatThin(
	RenderContext *context,
	frect r, float arcBegin, float arcLen)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	fpoint center,radius;
	center.x = (r.left + (r.right-r.left)/2.0);
	center.y = (r.top + (r.bottom-r.top)/2.0);
	radius.x = (r.right-center.x);
	radius.y = (r.bottom-center.y);
	grFillArc_FatThin(context,center,radius,arcBegin,arcLen,false);
};

void grInscribeFillEllipse_FatThin(
	RenderContext *context, frect r)
{
	if (context->fCoordTransform)
		context->fCoordTransform(context,(fpoint*)&r,2);

	fpoint center,radius;
	center.x = (r.left + (r.right-r.left)/2.0);
	center.y = (r.top + (r.bottom-r.top)/2.0);
	radius.x = (r.right-center.x);
	radius.y = (r.bottom-center.y);
	grFillEllipse_FatThin(context,center,radius,false);
};

void grStrokeArc_Fat(
	RenderContext *context,
	fpoint center, fpoint radius,
	float arcBegin, float arcLen,
	bool xform)
{
	arcBegin = arcBegin*M_PI/180.0;
	arcLen = arcLen*M_PI/180.0;

	if (arcLen == 0) return;
	if ((arcLen <= -M_PI*2) || (arcLen >= M_PI*2)) {
		grStrokeEllipse_Fat(context,center,radius,xform);
		return;
	};

	RenderPort *port = grLockPort(context);
	
	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	};
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
		arcBegin = port->rotater->RotateAlpha(arcBegin);
	}
#endif

	float ba,ea,pm,pa,sn,cn,len;
	fpoint rad1,rad2,rad3,start,stop,vect,pt;
	rect r;
	pm = context->totalEffective_PenSize/2.0;
	pa = context->totalEffective_PenSize-pm;
	rad1.x = radius.x - pm;
	rad1.y = radius.y - pm;
	rad2.x = radius.x + pa;
	rad2.y = radius.y + pa;
	
	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r.top = (int32)floor(center.y-rad2.y);
	r.bottom = (int32)floor(center.y+rad2.y+1.0);
	r.left = (int32)floor(center.x-rad2.x);
	r.right = (int32)floor(center.x+rad2.x+1.0);
	sect_rect(bound,r,&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	};

	if (arcLen > 0) {
		ba = arcBegin;
		ea = arcBegin+arcLen;
	} else {
		ba = arcBegin+arcLen;
		ea = arcBegin;
	};
	
	short capping = context->capRule;
	if (capping != BUTT_CAP) {
		b_get_cos_sin(ba,&cn,&sn);
		start.y = center.y - radius.y * sn;
		start.x = center.x + radius.x * cn;
		b_get_cos_sin(ea,&cn,&sn);
		stop.y = center.y - radius.y * sn;
		stop.x = center.x + radius.x * cn;
		rad3.x = rad3.y = pm;
	};

	SEdges *e = new SEdges(20,&bound,false);
	e->AddArc(center,rad2,arcBegin,arcLen,B_ARC_CONNECT_BEGIN_END);
	if (capping == ROUND_CAP) e->AddArc(stop,rad3,ea,M_PI,B_ARC_CONNECT_BEGIN_END);
	else if (capping == SQUARE_CAP) {
		vect.x = stop.x - center.x;
		vect.y = stop.y - center.y;
		len = b_sqrt(vect.x*vect.x + vect.y*vect.y);
		vect.x /= len;
		vect.y /= len;
		pt.x = stop.x + (vect.x + vect.y)*pm;
		pt.y = stop.y + (vect.y - vect.x)*pm;
		e->AddEdge(pt);
		pt.x = stop.x - (vect.x - vect.y)*pm;
		pt.y = stop.y - (vect.y + vect.x)*pm;
		e->AddEdge(pt);
	};
	e->AddArc(center,rad1,arcBegin,arcLen,B_ARC_CONNECT_END_BEGIN);
	if (capping == ROUND_CAP) e->AddArc(start,rad3,ba+M_PI,M_PI,B_ARC_CONNECT_BEGIN_END);
	else if (capping == SQUARE_CAP) {
		vect.x = start.x - center.x;
		vect.y = start.y - center.y;
		len = b_sqrt(vect.x*vect.x + vect.y*vect.y);
		vect.x /= len;
		vect.y /= len;
		pt.x = start.x - (vect.x + vect.y)*pm;
		pt.y = start.y - (vect.y - vect.x)*pm;
		e->AddEdge(pt);
		pt.x = start.x + (vect.x - vect.y)*pm;
		pt.y = start.y + (vect.y + vect.x)*pm;
		e->AddEdge(pt);
	};
	e->CloseEdges();
	
	grLockCanvi(port,&bound,hwSpanFill);
	e->ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
	grUnlockCanvi(port);
	grUnlockPort(context);

	delete e;
};

inline int32 xfloor(float f)
{
	int32 i = (int32)floor(f);
	if (i == f) i--;
	return i;
};

inline int32 xceil(float f)
{
	int32 i = (int32)floor(f);
	if (i != f) i++;
	return i;
};
	
void grStrokeArc_Thin(
	RenderContext *context,
	fpoint center, fpoint radius,
	float arcBegin, float arcLen,
	bool xform)
{
	int32 spans[(SPAN_BATCH+4)*3],spanCount;

	arcBegin = arcBegin*M_PI/180.0;
	arcLen = arcLen*M_PI/180.0;

	if (arcLen == 0) return;
	if ((arcLen <= -M_PI*2) || (arcLen >= M_PI*2)) {
		grStrokeEllipse_Thin(context,center,radius,xform);
		return;
	};

	RDebugPrintf((context,"grStrokeArc_Thin({%f,%f},{%f,%f},%f,%f)\n",
		center.x,center.y,radius.x,radius.y,arcBegin,arcLen));

	RenderPort *port = grLockPort(context);

	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	};
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
		arcBegin = port->rotater->RotateAlpha(arcBegin);
	}
#endif

	float x,y,x1,x2,startAng,stopAng,cn,sn;
	uint32 edgeFlags;
	int32 ix1,ix2,actualY,i,lx1,lx2,np,nr,leftStart,leftStop,rightStart,rightStop;
	fpoint start,stop,pt[2],*p;
	rect r[4];
	arc a;
	SetupArc(center,radius,0,M_PI*2.0,a,a,a,i,start,stop,true);

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r[0].top = (int32)floor(center.y-radius.y);
	r[0].bottom = (int32)floor(center.y+radius.y+1.0);
	r[0].left = (int32)floor(center.x-radius.x);
	r[0].right = (int32)floor(center.x+radius.x+1.0);
	sect_rect(bound,r[0],&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	}

	if (arcLen > 0) {
		startAng = fmod(arcBegin,M_PI*2.0);
		stopAng = fmod(arcBegin+arcLen,M_PI*2.0);
	} else {
		stopAng = fmod(arcBegin,M_PI*2.0);
		startAng = fmod(arcBegin+arcLen,M_PI*2.0);
	}

	if (startAng < 0) startAng += M_PI*2.0;
	if (stopAng < 0) stopAng += M_PI*2.0;
	
	b_get_cos_sin(startAng,&cn,&sn);
	pt[0].y = a.cy - radius.y * sn;
	pt[0].x = a.cx + radius.x * cn;
	b_get_cos_sin(stopAng,&cn,&sn);
	pt[1].y = a.cy - radius.y * sn;
	pt[1].x = a.cx + radius.x * cn;
	
	grLockCanvi(port,&bound,hwSpanFill);

	actualY = a.yTop;
	y = actualY - a.cy;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	lx1 = xceil(x1);
	lx2 = xfloor(x2);

	spanCount = 0;
	if (pt[0].y >= actualY) {
		if (pt[1].y >= actualY) {
			cn = (M_PI/2.0) - startAng;
			if (cn < 0) cn += M_PI*2;
			if (fabs(arcLen) > cn) {
				edgeFlags = ARC_LEFT|ARC_RIGHT;
				spans[0] = actualY;
				spans[1] = lx1;
				spans[2] = lx2;
				spanCount = 3;
			} else {
				edgeFlags = 0;
			};
			p = pt;
			np = 2;
		} else {
			/* The arc ends in this first segment */
			edgeFlags = ARC_RIGHT;
			spans[0] = actualY;
			spans[2] = lx2;
			spans[1] = xceil(pt[1].x);
			if (spans[1] < lx1) spans[1] = lx1;
			spanCount = 3;
			p = pt;
			np = 1;
		};
	} else {
		if (pt[1].y >= actualY) {
			/* The arc starts in this first segment */
			edgeFlags = ARC_LEFT;
			spans[0] = actualY;
			spans[1] = lx1;
			spans[2] = xfloor(pt[0].x);
			if (spans[2] > lx2) spans[2] = lx2;
			spanCount = 3;
			p = pt+1;
			np = 1;
		} else {
			if (pt[0].x >= pt[1].x) {
				/* The arc starts and ends in this first segment */
				edgeFlags = 0;
				spans[0] = actualY;
				spans[1] = xceil(pt[1].x);
				spans[2] = xfloor(pt[0].x);
				if (spans[2] > lx2) spans[2] = lx2;
				if (spans[1] < lx1) spans[1] = lx1;
				spanCount = 3;
			} else {
				/* The arc starts to both sides */
				edgeFlags = ARC_LEFT|ARC_RIGHT;
				spans[0] = spans[3] = actualY;
				spans[1] = lx1;
				spans[2] = xfloor(pt[0].x);
				spans[5] = lx2;
				spans[4] = xceil(pt[1].x);
				if (spans[2] > lx2) spans[2] = lx2;
				if (spans[4] < lx1) spans[4] = lx1;
				spanCount = 6;
			};
			p = pt;
			np = 0;
		};
	};

	if (pt[1].y < pt[0].y) {
		cn = pt[1].x;
		pt[1].x = pt[0].x;
		pt[0].x = cn;
		cn = pt[1].y;
		pt[1].y = pt[0].y;
		pt[0].y = cn;
		if (np == 1) p += (p==pt)?1:-1;
	};

	y += 1.0;
	actualY++;

	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	while (actualY < a.cy) {
		ix1 = xceil(x1);
		ix2 = xfloor(x2);

		leftStart = leftStop = ix1;
		if ((lx1-1) > ix1) leftStop = lx1-1;
		nr = 0;

		if (edgeFlags & ARC_LEFT) {
			spans[spanCount] = actualY;
			spans[spanCount+1] = leftStart;
			spans[spanCount+2] = leftStop;
			spanCount+=3;
		};

		for (i=0;i<np;i++) {
			if ((pt[i].x < a.cx) && (pt[i].y < actualY)) {
				/* We have to toggle the left edge state. */
				if (edgeFlags & ARC_LEFT) {
					/* We have to truncate the previous rect */
					spans[spanCount-2] = xceil(pt[i].x);
					edgeFlags &= ~ARC_LEFT;
				} else {
					/* We have to create a new rect */
					spans[spanCount] = actualY;
					spans[spanCount+1] = leftStart;
					spans[spanCount+2] = xfloor(pt[i].x);
					if (spans[spanCount+2] > leftStop) spans[spanCount+2] = leftStop;
					spanCount+=3;
					edgeFlags |= ARC_LEFT;
				};
				if (i == 0) { p++; i--; };
				np--;
			};
		};

		rightStart = rightStop = ix2;
		if ((lx2+1) < ix2) rightStart = lx2+1;

		if (edgeFlags & ARC_RIGHT) {
			spans[spanCount] = actualY;
			spans[spanCount+1] = rightStart;
			spans[spanCount+2] = rightStop;
			spanCount+=3;
		};

		for (i=0;i<np;i++) {
			if ((pt[i].x > a.cx) && (pt[i].y < actualY)) {
				/* We have to toggle the left edge state. */
				if (edgeFlags & ARC_RIGHT) {
					/* We have to truncate the previous rect */
					spans[spanCount-1] = xfloor(pt[i].x);
					edgeFlags &= ~ARC_RIGHT;
				} else {
					/* We have to create a new rect */
					spans[spanCount] = actualY;
					spans[spanCount+2] = rightStop;
					spans[spanCount+1] = xceil(pt[i].x);
					if (spans[spanCount+1] < rightStart) spans[spanCount+1] = rightStart;
					spanCount+=3;
					edgeFlags |= ARC_RIGHT;
				};
				if (i == 0) { p++; i--; };
				np--;
			};
		};

		lx1 = ix1;
		lx2 = ix2;

		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};

		y += 1.0;
		actualY++;

		x = b_sqrt(a.c1 - a.c3*y*y);
		x1 = a.cx - x;
		x2 = a.cx + x;
	};

	lx1 = (int32)floor(x1);
	lx2 = (int32)floor(x2);
	if (lx1 != x1) lx1++;
	if (lx2 == x2) lx2--;

	y += 1.0;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	while (actualY < a.yBot) {
		ix1 = xceil(x1);
		ix2 = xfloor(x2);

		leftStart = leftStop = lx1;
		if ((ix1-1) > lx1) leftStop = ix1-1;
		nr = 0;

		if (edgeFlags & ARC_LEFT) {
			spans[spanCount] = actualY;
			spans[spanCount+1] = leftStart;
			spans[spanCount+2] = leftStop;
			spanCount+=3;
		};

		for (i=0;i<np;i++) {
			if ((pt[i].x < a.cx) && (pt[i].y < (actualY+1))) {
				/* We have to toggle the left edge state. */
				if (edgeFlags & ARC_LEFT) {
					/* We have to truncate the previous rect */
					spans[spanCount-1] = xfloor(pt[i].x);
					edgeFlags &= ~ARC_LEFT;
				} else {
					/* We have to create a new rect */
					spans[spanCount] = actualY;
					spans[spanCount+2] = leftStop;
					spans[spanCount+1] = xceil(pt[i].x);
					if (spans[spanCount+1] < leftStart) spans[spanCount+1] = leftStart;
					spanCount+=3;
					edgeFlags |= ARC_LEFT;
				};
				if (i == 0) { p++; i--; };
				np--;
			};
		};

		rightStart = rightStop = lx2;
		if ((ix2+1) < lx2) rightStart = ix2+1;

		if (edgeFlags & ARC_RIGHT) {
			spans[spanCount] = actualY;
			spans[spanCount+1] = rightStart;
			spans[spanCount+2] = rightStop;
			spanCount+=3;
		};

		for (i=0;i<np;i++) {
			if ((pt[i].x > a.cx) && (pt[i].y < (actualY+1))) {
				/* We have to toggle the left edge state. */
				if (edgeFlags & ARC_RIGHT) {
					/* We have to truncate the previous rect */
					spans[spanCount-2] = xceil(pt[i].x);
					edgeFlags &= ~ARC_RIGHT;
				} else {
					/* We have to create a new rect */
					spans[spanCount] = actualY;
					spans[spanCount+1] = rightStart;
					spans[spanCount+2] = xfloor(pt[i].x);
					if (spans[spanCount+2] > rightStop) spans[spanCount+2] = rightStop;
					spanCount+=3;
					edgeFlags |= ARC_RIGHT;
				};
				if (i == 0) { p++; i--; };
				np--;
			};
		};

		lx1 = ix1;
		lx2 = ix2;

		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		y += 1.0;
		actualY++;

		x = b_sqrt(a.c1 - a.c3*y*y);
		x1 = a.cx - x;
		x2 = a.cx + x;
	};

	actualY = a.yBot;
	y = actualY - a.cy;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	lx1 = xceil(x1);
	lx2 = xfloor(x2);

	if (edgeFlags == (ARC_RIGHT|ARC_LEFT))	{
		if (np) {
			spans[spanCount] = spans[spanCount+3] = actualY;
			spans[spanCount+1] = lx1;
			spans[spanCount+5] = lx2;
			sn = pt[0].x;
			cn = pt[1].x;
			if (sn < cn) {
				x = sn;
				sn = cn;
				cn = x;
			};
			spans[spanCount+2] = xfloor(cn);
			spans[spanCount+4] = xceil(sn);
			spanCount+=6;
		} else {
			spans[spanCount] = actualY;
			spans[spanCount+1] = lx1;
			spans[spanCount+2] = lx2;
			spanCount+=3;
		};
	} else if (edgeFlags == ARC_RIGHT)			{
		spans[spanCount] = actualY;
		spans[spanCount+2] = lx2;
		spans[spanCount+1] = xceil(p[0].x);
		spanCount+=3;
	} else if (edgeFlags == ARC_LEFT)			{
		spans[spanCount] = actualY;
		spans[spanCount+1] = lx1;
		spans[spanCount+2] = xfloor(p[1].x);
		spanCount+=3;
	} else										{
		if (np) {
			spans[spanCount] = actualY;
			sn = pt[0].x;
			cn = pt[1].x;
			if (sn < cn) {
				x = sn;
				sn = cn;
				cn = x;
			};
			spans[spanCount+1] = xceil(cn);
			spans[spanCount+2] = xfloor(sn);
			spanCount+=3;
		};
	};

	if (spanCount) grFillSpans(context,port,spans,spanCount/3);
	
	grUnlockCanvi(port);
	grUnlockPort(context);
};

void grStrokeEllipse_Fat(
	RenderContext *context,
	fpoint center, fpoint radius,
	bool xform)
{
	RenderPort *port = grLockPort(context);
	int32 spans[(SPAN_BATCH+4)*3],spanCount;

	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	}
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	float x,x1,x2,ey,iy,y;
	int32 xa1,xa2,xb1,xb2,actualY,i;
	fpoint rad1,rad2,start,stop;
	arc external,internal;
	rect r[2];
	float halfPenWidth = context->totalEffective_PenSize/2.0;
	
	rad1.x = radius.x + halfPenWidth;
	rad1.y = radius.y + halfPenWidth;
	rad2.x = radius.x - halfPenWidth;
	rad2.y = radius.y - halfPenWidth;

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r[0].top = (int32)floor(center.y-rad1.y);
	r[0].bottom = (int32)floor(center.y+rad1.y+1.0);
	r[0].left = (int32)floor(center.x-rad1.x);
	r[0].right = (int32)floor(center.x+rad1.x+1.0);
	sect_rect(bound,r[0],&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	};

	SetupArc(center,rad1,0,M_PI*2.0,external,external,external,i,start,stop,true);
	SetupArc(center,rad2,0,M_PI*2.0,internal,internal,internal,i,start,stop,true);

	grLockCanvi(port,&bound,hwSpanFill);

	spanCount = 0;
	actualY = external.yTop;
	ey = actualY - external.cy;
	while (actualY < internal.yTop) {
		y = external.c1 - external.c3*ey*ey;
		x = b_sqrt(y);
		x1 = external.cx - x;
		x2 = external.cx + x;
		xa1 = (int32)floor(x1);
		xb2 = (int32)floor(x2);
		if (xa1 != x1) xa1++;
		if (xb2 == x2) xb2--;
		spans[spanCount] = actualY;
		spans[spanCount+1] = xa1;
		spans[spanCount+2] = xb2;
		spanCount+=3;
		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		ey += 1.0;
		actualY++;
	};

	iy = actualY - internal.cy;
	while (actualY <= internal.yBot) {
		y = external.c1 - external.c3*ey*ey;
		x = b_sqrt(y);
		x1 = external.cx - x;
		x2 = external.cx + x;
		xa1 = (int32)floor(x1);
		xb2 = (int32)floor(x2);
		if (xa1 != x1) xa1++;
		if (xb2 == x2) xb2--;

		y = internal.c1 - internal.c3*iy*iy;
		x = b_sqrt(y);
		if (x > 0) {
			x1 = internal.cx - x;
			x2 = internal.cx + x;
			xa2 = (int32)floor(x1);
			xb1 = (int32)floor(x2);
			if (xb1 != x2) xb1++;
			if (xa2 == x1) xa2--;
			spans[spanCount] = spans[spanCount+3] = actualY;
			spans[spanCount+1] = xa1;
			spans[spanCount+2] = xa2;
			spans[spanCount+4] = xb1;
			spans[spanCount+5] = xb2;
			spanCount+=6;
		} else {
			spans[spanCount] = actualY;
			spans[spanCount+1] = xa1;
			spans[spanCount+2] = xb2;
			spanCount+=3;
		};
		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		ey += 1.0; iy += 1.0;
		actualY++;
	};

	while (actualY <= external.yBot) {
		y = external.c1 - external.c3*ey*ey;
		x = b_sqrt(y);
		x1 = external.cx - x;
		x2 = external.cx + x;
		xa1 = (int32)floor(x1);
		xb2 = (int32)floor(x2);
		if (xa1 != x1) xa1++;
		if (xb2 == x2) xb2--;
		spans[spanCount] = actualY;
		spans[spanCount+1] = xa1;
		spans[spanCount+2] = xb2;
		spanCount+=3;
		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		ey += 1.0; actualY++;
	};

	if (spanCount) grFillSpans(context,port,spans,spanCount/3);

	grUnlockCanvi(port);
	grUnlockPort(context);
};

void grStrokeEllipse_Thin(
	RenderContext *context,
	fpoint center, fpoint radius,
	bool xform)
{
	RDebugPrintf((context,"grStrokeEllipse_Thin({%f,%f},{%f,%f})\n",
		center.x,center.y,radius.x,radius.y));

	RenderPort *port = grLockPort(context);
	int32 spans[(SPAN_BATCH+4)*3],spanCount;
	int32 lasty,lastLeft,lastRight;

	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	}
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	float x,y,x1,x2;
	int32 ix1,ix2,actualY,i,lx1,lx2;
	fpoint start,stop;
	rect r[2];
	arc a;
	SetupArc(center,radius,0,M_PI*2.0,a,a,a,i,start,stop,true);

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r[0].top = (int32)floor(center.y-radius.y);
	r[0].bottom = (int32)floor(center.y+radius.y+1.0);
	r[0].left = (int32)floor(center.x-radius.x);
	r[0].right = (int32)floor(center.x+radius.x+1.0);
	sect_rect(bound,r[0],&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	};
	
	grLockCanvi(port,&bound,hwSpanFill);

	spanCount = 0;
	actualY = a.yBot;
	y = actualY - a.cy;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	lx1 = (int32)floor(x1);
	lx2 = (int32)floor(x2);
	if (lx1 != x1) lx1++;
	if (lx2 == x2) lx2--;
	lasty = actualY;
	lastLeft = lx1;
	lastRight = lx2;
	
	actualY = a.yTop;
	y = actualY - a.cy;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	lx1 = (int32)floor(x1);
	lx2 = (int32)floor(x2);
	if (lx1 != x1) lx1++;
	if (lx2 == x2) lx2--;
	spans[spanCount] = actualY;
	spans[spanCount+1] = lx1;
	spans[spanCount+2] = lx2;
	spanCount+=3;
	y += 1.0;
	actualY++;

	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	while (actualY < a.cy) {
		ix1 = (int32)floor(x1);
		ix2 = (int32)floor(x2);
		if (ix1 != x1) ix1++;
		if (ix2 == x2) ix2--;
		spans[spanCount] = spans[spanCount+3] = actualY;
		spans[spanCount+1] = spans[spanCount+2] = ix1;
		if ((lx1-1) > ix1) spans[spanCount+2] = lx1-1;
		spans[spanCount+4] = spans[spanCount+5] = ix2;
		if ((lx2+1) < ix2) spans[spanCount+4] = lx2+1;
		spanCount += 6;

		lx1 = ix1;
		lx2 = ix2;

		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		y += 1.0;
		actualY++;

		x = b_sqrt(a.c1 - a.c3*y*y);
		x1 = a.cx - x;
		x2 = a.cx + x;
	};

	lx1 = (int32)floor(x1);
	lx2 = (int32)floor(x2);
	if (lx1 != x1) lx1++;
	if (lx2 == x2) lx2--;

	y += 1.0;
	x = b_sqrt(a.c1 - a.c3*y*y);
	x1 = a.cx - x;
	x2 = a.cx + x;
	while (actualY < a.yBot) {
		ix1 = (int32)floor(x1);
		ix2 = (int32)floor(x2);
		if (ix1 != x1) ix1++;
		if (ix2 == x2) ix2--;
		spans[spanCount] = spans[spanCount+3] = actualY;
		spans[spanCount+1] = spans[spanCount+2] = lx1;
		if ((ix1-1) > lx1) spans[spanCount+2] = ix1-1;
		spans[spanCount+4] = spans[spanCount+5] = lx2;
		if ((ix2+1) < lx2) spans[spanCount+4] = ix2+1;
		spanCount += 6;

		lx1 = ix1;
		lx2 = ix2;

		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		y += 1.0;
		actualY++;

		x = b_sqrt(a.c1 - a.c3*y*y);
		x1 = a.cx - x;
		x2 = a.cx + x;
	};

	spans[spanCount] = lasty;
	spans[spanCount+1] = lastLeft;
	spans[spanCount+2] = lastRight;
	spanCount+=3;
	grFillSpans(context,port,spans,spanCount/3);

	grUnlockCanvi(port);
	grUnlockPort(context);
};

void grFillArc_FatThin(
	RenderContext *context,
	fpoint center, fpoint radius,
	float arcBegin, float arcLen,
	bool xform)
{	
	arcBegin = arcBegin*M_PI/180.0;
	arcLen = arcLen*M_PI/180.0;
	
	if (arcLen == 0) return;
	if ((arcLen <= -M_PI*2) || (arcLen >= M_PI*2)) {
		grFillEllipse_FatThin(context,center,radius,xform);
		return;
	};

	RenderPort *port = grLockPort(context);

	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	}
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
		arcBegin = port->rotater->RotateAlpha(arcBegin);
	}
#endif

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	SEdges *e = new SEdges(5,&bound,false);
	e->StartEdges(center);
	e->AddArc(center,radius,arcBegin,arcLen,B_ARC_CONNECT_BEGIN_END);
	e->CloseEdges();

	grLockCanvi(port,&bound,hwSpanFill);
	e->ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
	grUnlockCanvi(port);
	grUnlockPort(context);

	delete e;
};

void grFillEllipse_FatThin(
	RenderContext *context,
	fpoint center, fpoint radius,
	bool xform)
{
	float x,y,x1,x2;
	int32 xa1,xb2,actualY,i;
	fpoint start,stop;
	rect r;
	arc a;

	RenderPort *port = grLockPort(context);
	int32 spans[(SPAN_BATCH+4)*3],spanCount;

	if (xform && context->fCoordTransform) {
		context->fCoordTransform(context,&center,1);
		context->fCoordTransform(context,&radius,1);
	}
	center.x += context->totalEffective_Origin.x;
	center.y += context->totalEffective_Origin.y;
	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotatePoint(&center, &center);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	SetupArc(center,radius,0,M_PI*2.0,a,a,a,i,start,stop,true);

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r.top = (int32)floor(center.y - radius.y);
	r.left = (int32)floor(center.x - radius.x);
	r.bottom = (int32)floor(center.y + radius.y + 1.0);
	r.right = (int32)floor(center.x + radius.x + 1.0);
	sect_rect(bound,r,&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	};
	
	grLockCanvi(port,&bound,hwSpanFill);

	spanCount = 0;
	actualY = a.yTop;
	y = actualY - a.cy;
	while (actualY <= a.yBot) {
		x = b_sqrt(a.c1 - a.c3*y*y);
		x1 = a.cx - x;
		x2 = a.cx + x;
		xa1 = (int32)floor(x1);
		xb2 = (int32)floor(x2);
		if (xa1 != x1) xa1++;
		if (xb2 == x2) xb2--;
		spans[spanCount] = actualY;
		spans[spanCount+1] = xa1;
		spans[spanCount+2] = xb2;
		spanCount+=3;
		if (spanCount >= (SPAN_BATCH*3)) {
			grFillSpans(context,port,spans,spanCount/3);
			spanCount = 0;
		};
		y += 1.0; actualY++;
	};

	if (spanCount) grFillSpans(context,port,spans,spanCount/3);

	grUnlockCanvi(port);
	grUnlockPort(context);
};

void grStrokeRoundRect_Thin(
	RenderContext *context,
	frect _frame, fpoint radius)
{
	frect frame = _frame;
	RenderPort *port = grLockPort(context);

	if (context->fCoordTransform) {
		context->fCoordTransform(context,(fpoint*)&frame,2);
		context->fCoordTransform(context,&radius,1);
	}

	if ((frame.left > frame.right) || (frame.top > frame.bottom)) {
		grUnlockPort(context);
		return;
	}

	if ((radius.x <= 1.0) || (radius.y <= 1.0)) {
		grStrokeRect(context,_frame);
		grUnlockPort(context);
		return;
	}

	if (radius.x > (frame.right-frame.left)*0.5)
		radius.x = (frame.right-frame.left)*0.5;
	if (radius.y > (frame.bottom-frame.top)*0.5)
		radius.y = (frame.bottom-frame.top)*0.5;
	frame.left += context->totalEffective_Origin.x;
	frame.right += context->totalEffective_Origin.x;
	frame.top += context->totalEffective_Origin.y;
	frame.bottom += context->totalEffective_Origin.y;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotateRect(&frame, &frame);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	float x,y;
	int32 ix1,ix2,actualY,i,lx1,lx2;
	fpoint start,stop,center;
	rect r[2];
	frect innerFrame;
	arc a,b;

	innerFrame.left = frame.left + radius.x;
	innerFrame.right = frame.right - radius.x;
	innerFrame.top = frame.top + radius.y;
	innerFrame.bottom = frame.bottom - radius.y;

	radius.x += ARC_FUDGE_FACTOR;
	radius.y += ARC_FUDGE_FACTOR;

	center.x = innerFrame.left;
	center.y = innerFrame.top;
	SetupArc(center,radius,0,M_PI*2.0,a,a,a,i,start,stop,true);
	center.x = innerFrame.left;
	center.y = innerFrame.bottom;
	SetupArc(center,radius,0,M_PI*2.0,b,b,b,i,start,stop,true);

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r[0].top = (int32)floor(frame.top);
	r[0].bottom = (int32)floor(frame.bottom+1.0);
	r[0].left = (int32)floor(frame.left);
	r[0].right = (int32)floor(frame.right+1.0);
	sect_rect(bound,r[0],&bound);
	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	};
	
	grLockCanvi(port,&bound,hwRectFill);

	actualY = b.yBot;
	y = actualY - b.cy;
	x = b_sqrt(b.c1 - b.c3*y*y);
	r[0].top = r[0].bottom = actualY;
	r[0].left = xceil(innerFrame.left - x);
	r[0].right = xfloor(innerFrame.right + x);
	grFillRects(context,port,r,1);
	
	actualY = a.yTop;
	y = actualY - a.cy;
	x = b_sqrt(a.c1 - a.c3*y*y);
	r[0].top = r[0].bottom = actualY;
	r[0].left = lx1 = xceil(innerFrame.left - x);
	r[0].right = lx2 = xfloor(innerFrame.right + x);
	grFillRects(context,port,r,1);
	y += 1.0;
	actualY++;

	x = b_sqrt(a.c1 - a.c3*y*y);
	while (actualY < a.cy) {
		ix1 = xceil(innerFrame.left - x);
		ix2 = xfloor(innerFrame.right + x);
		r[0].top = r[0].bottom =
		r[1].top = r[1].bottom = actualY;

		r[0].left = r[0].right = ix1;
		if ((lx1-1) > ix1) r[0].right = lx1-1;
		r[1].left = r[1].right = ix2;
		if ((lx2+1) < ix2) r[1].left = lx2+1;

		lx1 = ix1;
		lx2 = ix2;

		grFillRects(context,port,r,2);
		y += 1.0;
		actualY++;

		x = b_sqrt(a.c1 - a.c3*y*y);
	};

	r[1].top = r[0].top = actualY;
	r[0].right = r[0].left = (int32)floor(frame.left+0.5);
	r[1].right = r[1].left = (int32)floor(frame.right+0.5);
	r[1].bottom = r[0].bottom = (actualY = xceil(b.cy))-1;
	grFillRects(context,port,r,2);

	y = actualY - b.cy;
	x = b_sqrt(b.c1 - b.c3*y*y);
	lx1 = xceil(innerFrame.left - x);
	lx2 = xfloor(innerFrame.right + x);
	y += 1.0;
	x = b_sqrt(b.c1 - b.c3*y*y);
	while (actualY < b.yBot) {
		ix1 = xceil(innerFrame.left - x);
		ix2 = xfloor(innerFrame.right + x);
		r[0].top = r[0].bottom =
		r[1].top = r[1].bottom = actualY;

		r[0].left = r[0].right = lx1;
		if ((ix1-1) > lx1) r[0].right = ix1-1;
		r[1].left = r[1].right = lx2;
		if ((ix2+1) < lx2) r[1].left = ix2+1;

		lx1 = ix1;
		lx2 = ix2;

		grFillRects(context,port,r,2);
		y += 1.0;
		actualY++;

		x = b_sqrt(b.c1 - b.c3*y*y);
	};

	grUnlockCanvi(port);
	grUnlockPort(context);
};

void grStrokeRoundRect_Fat(
	RenderContext *context,
	frect frame, fpoint radius)
{
	RenderPort *port = grLockPort(context);

	if (context->fCoordTransform) {
		context->fCoordTransform(context,(fpoint*)&frame,2);
		context->fCoordTransform(context,&radius,1);
	}
	
	frame.left += context->totalEffective_Origin.x;
	frame.right += context->totalEffective_Origin.x;
	frame.top += context->totalEffective_Origin.y;
	frame.bottom += context->totalEffective_Origin.y;

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotateRect(&frame, &frame);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	float penSize = context->totalEffective_PenSize;
	if (penSize < 1.01) penSize = 1.01;

	{
		fpoint r,p;
		rect rt;
		
		rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			port->rotater->RotateRect(&bound, &bound);
#endif

		rt.top = (int32)floor(frame.top-1.0);
		rt.bottom = (int32)floor(frame.bottom+2.0);
		rt.left = (int32)floor(frame.left-1.0);
		rt.right = (int32)floor(frame.right+2.0);
		sect_rect(bound,rt,&bound);
		if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
			grUnlockPort(context);
			return;
		};
		
		SEdges *e = new SEdges(20,&rt,false);

		r.x = radius.x + ARC_FUDGE_FACTOR;
		r.y = radius.y + ARC_FUDGE_FACTOR;
		p.x = frame.left+radius.x; p.y = frame.top+radius.y;
		e->AddArc(p,r,M_PI/2.0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
		p.y = frame.bottom-radius.y;
		e->AddArc(p,r,M_PI,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
		p.x = frame.right-radius.x;
		e->AddArc(p,r,M_PI*3.0/2.0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
		p.y = frame.top+radius.y;
		e->AddArc(p,r,0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
		e->CloseEdges();
		
		r.x = radius.x - penSize + ARC_FUDGE_FACTOR;
		r.y = radius.y - penSize + ARC_FUDGE_FACTOR;
		if ((r.x > 0) || (r.y > 0)) {
			p.x = frame.left+radius.x; p.y = frame.top+radius.y;
			e->AddArc(p,r,M_PI/2.0,M_PI/2.0,B_ARC_CONNECT_END_BEGIN);
			p.x = frame.right-radius.x;
			e->AddArc(p,r,0,M_PI/2.0,B_ARC_CONNECT_END_BEGIN);
			p.y = frame.bottom-radius.y;
			e->AddArc(p,r,M_PI*3.0/2.0,M_PI/2.0,B_ARC_CONNECT_END_BEGIN);
			p.x = frame.left+radius.x;
			e->AddArc(p,r,M_PI,M_PI/2.0,B_ARC_CONNECT_END_BEGIN);
		} else {
			p.x = frame.left+penSize-ARC_FUDGE_FACTOR;
			p.y = frame.top+penSize-ARC_FUDGE_FACTOR;
			e->StartEdges(p);
			p.x = frame.right-penSize+ARC_FUDGE_FACTOR;
			e->AddEdge(p);
			p.y = frame.bottom-penSize+ARC_FUDGE_FACTOR;
			e->AddEdge(p);
			p.x = frame.left+penSize-ARC_FUDGE_FACTOR;
			e->AddEdge(p);
		};
		e->CloseEdges();
		
		grLockCanvi(port,&rt,hwSpanFill);
		e->ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
		grUnlockCanvi(port);
		
		grUnlockPort(context);
		delete e;
	};
};

void grFillRoundRect_FatThin(
	RenderContext *context,
	frect frame, fpoint radius)
{
	rect r;
	fpoint p,rad;

	RenderPort *port = grLockPort(context);

	frect _frame = frame;
	if (context->fCoordTransform) {
		context->fCoordTransform(context,(fpoint*)&frame,2);
		context->fCoordTransform(context,&radius,1);
	}
	
	if ((frame.left > frame.right) || (frame.top > frame.bottom)) {
		grUnlockPort(context);
		return;
	}

	if ((radius.x <= 1.0) || (radius.y <= 1.0)) {
		grFillRect(context,_frame);
		grUnlockPort(context);
		return;
	}

	if (radius.x > (frame.right-frame.left)*0.5)
		radius.x = (frame.right-frame.left)*0.5;
	if (radius.y > (frame.bottom-frame.top)*0.5)
		radius.y = (frame.bottom-frame.top)*0.5;

	frame.left += context->totalEffective_Origin.x;
	frame.right += context->totalEffective_Origin.x;
	frame.top += context->totalEffective_Origin.y;
	frame.bottom += context->totalEffective_Origin.y;

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		port->rotater->RotateRect(&frame, &frame);
		port->rotater->RotateRadius(&radius, &radius);
	}
#endif

	rect bound = port->drawingBounds;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		port->rotater->RotateRect(&bound, &bound);
#endif

	r.top = (int32)floor(frame.top);
	r.bottom = (int32)floor(frame.bottom+1.0);
	r.left = (int32)floor(frame.left);
	r.right = (int32)floor(frame.right+1.0);
	sect_rect(bound,r,&bound);

	if ((bound.top > bound.bottom) || (bound.left > bound.right)) {
		grUnlockPort(context);
		return;
	}
	
	SEdges *e = new SEdges(5,&bound,false);

	rad.x = radius.x + ARC_FUDGE_FACTOR;
	rad.y = radius.y + ARC_FUDGE_FACTOR;
	p.x = frame.left+radius.x; p.y = frame.top+radius.y;
	e->AddArc(p,rad,M_PI/2.0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
	p.x = frame.left+radius.x; p.y = frame.bottom-radius.y;
	e->AddArc(p,rad,M_PI,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
	p.x = frame.right-radius.x; p.y = frame.bottom-radius.y;
	e->AddArc(p,rad,M_PI*3.0/2.0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);
	p.x = frame.right-radius.x; p.y = frame.top+radius.y;
	e->AddArc(p,rad,0,M_PI/2.0,B_ARC_CONNECT_BEGIN_END);

	e->CloseEdges();

	grLockCanvi(port,&bound,hwSpanFill);
	e->ScanConvertAndRender(B_NON_ZERO_WINDING_RULE,context);
	grUnlockCanvi(port);

	grUnlockPort(context);
	
	delete e;
};
