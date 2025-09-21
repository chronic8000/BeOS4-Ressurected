
//******************************************************************************
//
//	File:			edges.cpp
//	Description:	Low-level resolution dependent format for describing
//					rasterized polygons or collections of polygons.
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "edges.h"
#include "bezier.h"
#include "fastmath.h"
#include "pointInlines.h"

#include <support2/Debug.h>

status_t SEdges::DoEdge(fpoint p1, fpoint p2, int16 prevSign, 
						bool skipStart, bool skipEnd)
{
	edge *e;
	
	fpoint *start;
	float y1,y2,dx,dy,dxdy,x;
	int32 s1,s2,tmp,slot,yi1,yi2,ix,idxdy;

	s1 = sgn(p2.y-p1.y);

	if (s1>0) {
		start = &p1;
		if (m_edgeEnds && !skipStart) {
			y1 = floor(p1.y+0.5);
			if (prevSign>0) y1 += 1.0;
		} else {
			y1 = floor(p1.y);
			if (y1 != p1.y) y1 += 1.0;
		};
		if (m_edgeEnds && !skipEnd)
			y2 = floor(p2.y+0.5);
		else {
			y2 = floor(p2.y);
			if (y2 == p2.y) y2 -= 1.0;
		};
	} else if (s1<0) {
		start = &p2;
		if (m_edgeEnds && !skipEnd)
			y1 = floor(p2.y+0.5);
		else {
			y1 = floor(p2.y);
			if (y1 != p2.y) y1 += 1.0;
		};
		if (m_edgeEnds && !skipStart) {
			y2 = floor(p1.y+0.5);
			if (prevSign<0) y2 -= 1.0;
		} else {
			y2 = floor(p1.y);
			if (y2 == p1.y) y2 -= 1.0;
		};
	} else
		return B_OK;

	yi1 = (int32)floor(y1+0.5);
	yi2 = (int32)floor(y2+0.5);

	if (yi1 < m_bounds.top) {
		yi1 = m_bounds.top;
		y1 	= yi1;
	};

	if (yi2 > m_bounds.bottom) {
		yi2 = m_bounds.bottom;
		y2 	= yi2;
	};

	if ((yi1 > yi2)||(yi2 < m_bounds.top)||(yi1 > m_bounds.bottom))
		return B_OK;

	m_prevSign = s1;
	
	dx = p2.x-p1.x;
	dy = p2.y-p1.y;
	dxdy = dx/dy;
	x = start->x + (y1-start->y)*dxdy;

repeat:
	if ((x < m_bounds.left) || (x > m_bounds.right)) {
		if (x < m_bounds.left) {
			if ((x + (y2-y1)*dxdy) > m_bounds.left) {
				y1 = y1 + (m_bounds.left-x)/dxdy;
				tmp = yi1;
				yi1 = (int32)floor(y1);
				if (yi1 != y1) yi1++;
				y1 = yi1;
				x = start->x + (y1-start->y)*dxdy;
				if (x < m_bounds.left) x = m_bounds.left;
				s2 = yi1;
				if (yi2<yi1) s2 = yi2;
				while (tmp < s2) {
					m_windings[tmp-m_bounds.top] += s1;
					tmp++;
				};
				if (yi1 > yi2) return B_OK;
				goto repeat;
			} else {
				tmp = yi1;
				while (tmp <= yi2) {
					m_windings[tmp-m_bounds.top] += s1;
					tmp++;
				};
				return B_OK;
			};
		} else {
			if ((x + (y2-y1)*dxdy) < m_bounds.right) {
				y1 = y1 + (m_bounds.right-x)/dxdy;
				yi1 = (int32)floor(y1);
				if (yi1 != y1) yi1++;
				y1 = yi1;
				x = start->x + (y1-start->y)*dxdy;
				if (x > m_bounds.right) x = m_bounds.right;
				if (yi1 > yi2) return B_OK;
				goto repeat;
			} else {
				return B_OK;
			};
		};
	} else {
		ix = (int32)floor((x*65536.0) + 0.5);
		if (dxdy > m_width) idxdy = m_width*65536;
		else if (dxdy < -m_width) idxdy = -m_width*65536;
		else idxdy = (int32)floor((dxdy*65536.0)+0.5);
	};

	if (!(e = AllocEdge())) return B_ERROR;

	e->yTop = yi1;
	e->yBot = yi2;
	e->yDir = s1;
	e->x	= ix;
	e->dxdy	= idxdy;

	slot = e->yTop - m_bounds.top;
	e->next = m_scanlines[slot];
	m_scanlines[slot] = e;

	return B_OK;
}

status_t SEdges::AddArc(	fpoint center, fpoint radius,
							float startAng, float stopAng,
							int32 connectRule)
{
	fpoint p1,p2,tmp;
	status_t r=B_OK;
	int32 s,segs,firstLine,lastLine;
	arc a[3];

	SetupArc(	center,
				radius,
				startAng,
				stopAng,
				a[0],a[1],a[2],segs,
				p1,p2,true);

	for (s=0;s<segs;s++) {
		firstLine = max(a[s].yTop,m_bounds.top);
		lastLine = min(a[s].yBot,m_bounds.bottom);
		a[s].dir = (int)connectRule;
		if (firstLine <= lastLine) m_arcList.AddItem(a[s]);
	};

	if (connectRule != B_ARC_NO_CONNECT) {
		s = sgn(center.x-p2.x);
		if (connectRule == B_ARC_CONNECT_END_BEGIN) {
			tmp = p1;
			p1 = p2;
			p2 = tmp;
			s = sgn(p2.x-center.x);
		};

		if (m_state == 2) {
			r = DoEdge(m_prevPt,p1,m_prevSign,m_skipStartOfLast,true);
			m_prevSign = 0;
		} else if (m_state == 1) {
			m_secondPt = p1;
			m_prevSign = 0;
			m_skipEndOfFirst = true;
			m_flags |= ARC_NEED_FIRST_EDGE;
			m_state = 2;
		} else {
			m_firstPt = p1;
			m_secondPt = p2;
			m_skipEndOfFirst = false;
			m_state = 2;
		};
		m_skipStartOfLast = true;
		m_prevPt = p2;
	};

	return r;
};

status_t SEdges::AddEdges(fpoint *vertices, int32 vCount)
{
	fpoint p1,p2;
	status_t r;
	int32 i=0,j;

	/* 	Now, if we need edge pixels, we have to find a place in the poly to
		start which spans at least two scan lines.  This is because the edge
		code has a problem with starting with edges of slope 0.  */

	if (!m_includeEdgePixels) goto gotOne;

	if (vCount < 3) return B_ERROR;
	for (;i<(vCount-1);i++) {
		p1 = vertices[i];
		p2 = vertices[i+1];
		if (floor(p1.y+0.5) != floor(p2.y+0.5)) goto gotOne;
	};
	p1 = vertices[vCount-1];
	p2 = vertices[0];
	if (floor(p1.y+0.5) == floor(p2.y+0.5)) return B_OK;

	gotOne:
	j=i;
	if ((r=StartEdges(vertices[i],vertices[(i+1)%vCount])) != B_OK) return r;
	i = (i+2)%vCount;
	while (i!=j) {
		if ((r=AddEdge(vertices[i])) != B_OK) return r;
		i = (i+1)%vCount;
	};
	if ((r=CloseEdges()) != B_OK) return r;
	return B_OK;
};

SEdges::SEdges(int32 countGuess, rect *bounds, bool includeEdgePixels, fpoint translation)
	: m_edgeLists(64), m_arcList(16), m_bezierBuffer(64)
{
	m_translation = translation;
	m_state = 0;
	m_flags = 0;
	m_includeEdgePixels = m_edgeEnds = includeEdgePixels;
	m_edgesPerList = countGuess;
	m_edgeLists.AddItem((edge*)grMalloc(sizeof(edge)*countGuess,"InitialEdgeMalloc",MALLOC_CANNOT_FAIL));
	m_addList = 0;
	m_addIndex = 0;
	if (bounds) {
		m_bounds = *bounds;
		m_bounds.left--;
		m_bounds.top--;
		m_bounds.right++;
		m_bounds.bottom++;
		m_width = m_bounds.right - m_bounds.left + 1;
		m_boundLeft = m_bounds.left;
		m_boundRight = m_bounds.right;
	} else {
		m_bounds.top = 0;
		m_bounds.bottom = -1;
		m_bounds.left = 0;
		m_bounds.right = -1;
		m_boundLeft = m_bounds.left;
		m_boundRight = m_bounds.right;
	};
	if ((m_bounds.top > m_bounds.bottom) ||
		(m_bounds.left > m_bounds.right)) {
		m_boundsSet = false;
		m_scanlines = NULL;
		m_arcScanlines = NULL;
		m_windings = NULL;
		m_scanLineCount = 0;
	} else {
		m_scanLineCount = m_bounds.bottom-m_bounds.top+1;
		m_boundsSet = true;
		m_scanlines = (edge**)grMalloc(sizeof(edge*) * m_scanLineCount,"Edge scanlines",MALLOC_CANNOT_FAIL);
		m_arcScanlines = (arc**)grMalloc(sizeof(arc*) * m_scanLineCount,"Arc scanlines",MALLOC_CANNOT_FAIL);
		m_windings = (int16*)grMalloc(sizeof(int16) * m_scanLineCount,"Edge windings",MALLOC_CANNOT_FAIL);
		for (int i=0;i<m_scanLineCount;i++) {
			m_scanlines[i] = NULL;
			m_arcScanlines[i] = NULL;
			m_windings[i] = 0;
		};
	};
};

SEdges::~SEdges()
{
	for (int i=0;i<m_edgeLists.CountItems();i++) {
		if (m_edgeLists[i] != NULL)
			grFree(m_edgeLists[i]);
	};
	if (m_scanlines) grFree(m_scanlines);
	if (m_arcScanlines) grFree(m_arcScanlines);
	if (m_windings) grFree(m_windings);
};

status_t SEdges::SetBounds(rect *bounds)
{
	if (m_boundsSet)
		return B_ERROR;

	if (m_scanLineCount == 0) {
		m_bounds = *bounds;
		m_bounds.left--;
		m_bounds.top--;
		m_bounds.right++;
		m_bounds.bottom++;
		m_boundLeft = m_bounds.left;
		m_boundRight = m_bounds.right;
		m_width = m_bounds.right - m_bounds.left + 1;
		m_scanLineCount = m_bounds.bottom-m_bounds.top+1;
		m_scanlines = (edge**)grMalloc(sizeof(edge) * m_scanLineCount,"Edge scanlines",MALLOC_CANNOT_FAIL);
		m_windings = (int16*)grMalloc(sizeof(int16) * m_scanLineCount,"Edge windings",MALLOC_CANNOT_FAIL);
		for (int i=0;i<m_scanLineCount;i++) {
			m_scanlines[i] = NULL;
			m_windings[i] = 0;
		};
		return B_OK;
	};

	return B_OK;
};

/*
static edge* UpdateActiveEdges(edge* active, edge* edgeTable[], long curY, long y_min, long y_max)
{
	edge *e, **ep;
	for (ep = &active, e = *ep; e != NULL; e = *ep) {
		if (e->yBot < curY) {
			*ep = e->next;
		} else
			ep = &e->next;
	};
	if (curY <= y_max) {
		*ep = edgeTable[curY - y_min];
		edgeTable[curY - y_min] = NULL;
		return active;
	} else
		return NULL;
}
*/

struct crossing {
	int8 dir;
	int32 x;
};

struct cliprect {
	rect *	r;
	cliprect *next;
};

#define P
//define P _sPrintf

#define SPAN_BATCH 32

void SEdges::DrawWindingRuns(RenderContext *context, int32 spanBatch, rect clipBound)
{
	edge			**e1,*e2,*active;
	arc				**a1,*a2,*activeArcs;
	int16			minY=m_bounds.top,maxY=m_bounds.bottom,curDir;
	int32 			numCoords, begin, end, i, j, k, hum;
	int32			beginX, endX, x, curY, ec=CountEdges(),ac=m_arcList.CountItems(),x1,x2;
	float			fx,fy;
	int32			rightX=m_bounds.right<<16;
	int32			leftX=m_bounds.left<<16;
	int32			beginAdd,endAdd;
	crossing		stackCrossings[4];
	crossing 		*crossings=stackCrossings;
	int32			*spans,spanIndex=0,stackSpans[SPAN_BATCH*3];

	if (!m_scanLineCount) return;

	spanBatch *= 3;
	if (spanBatch <= (SPAN_BATCH*3)) spans = stackSpans;
	else spans = (int32*)grMalloc(spanBatch*sizeof(int32),"Span buffer",MALLOC_CANNOT_FAIL);

	RenderPort *port = context->renderPort;

	if (m_includeEdgePixels) {
		beginAdd = 32768;
		endAdd = 32768;
	} else {
		beginAdd = 65536;
		endAdd = 0;
	};

	if (clipBound.bottom < maxY) maxY = clipBound.bottom;
	if (minY > maxY) return;

	for (i=0;i<ac;i++) {
		j = max(m_arcList[i].yTop,minY);
		k = min(m_arcList[i].yBot,maxY);
		if (j <= k) {
			a2 = &m_arcList[i];
			a2->next = m_arcScanlines[j-minY];
			m_arcScanlines[j-minY] = a2;
		};
	};

	for (	curY=minY;
			(curY <= maxY) &&
			(m_scanlines[curY-minY] == NULL) && 
			(m_arcScanlines[curY-minY] == NULL) &&
			(m_windings[curY-minY]==0);
			curY++) {
	};
	if (curY > maxY) return;

	ac<<=1;
	if ((ec+ac) > 4) crossings = (crossing*)grMalloc(sizeof(crossing)*(ec+ac),"Edge drawing",MALLOC_CANNOT_FAIL);

 	active = NULL;
 	activeArcs = NULL;
	a1 = &activeArcs;
	while (curY < clipBound.top) {
		e1 = &active;
		while (*e1) {
			(*e1)->x += (*e1)->dxdy;
			e1 = &(*e1)->next;
		};

		e2 = m_scanlines[curY-minY];
		while (e2) {
			if (e2->yBot >= clipBound.top) {
				e2->x += e2->dxdy;
				(*e1) = e2;
				e1 = &e2->next;
			};
			e2 = e2->next;
		};
		*e1 = NULL;

		a2 = m_arcScanlines[curY-minY];
		while (a2) {
			if (a2->yBot >= clipBound.top) {
				(*a1) = a2;
				a1 = &a2->next;
			};
			a2 = a2->next;
		};
		*a1 = NULL;
		
		curY++;
	};

	while (curY <= maxY) {
		/* Handle edges */
		e1 = &active; numCoords=0;
		repeatIt:
		while (*e1) {
			i=numCoords-1;
			x=(*e1)->x;
			while ((i>=0) && (x < crossings[i].x)) {
				crossings[i+1] = crossings[i]; i--;
			};
			crossings[++i].x = x;
			crossings[i].dir = (*e1)->yDir;
			if ((*e1)->yBot == curY) {
				*e1 = (*e1)->next;
			} else {
				j = (*e1)->x;
				if ((j <= rightX) && (j >= leftX))
					(*e1)->x = j + (*e1)->dxdy;
				e1 = &(*e1)->next;
			};
			numCoords++;
		};
		if (m_scanlines[curY-minY]) {
			*e1 = m_scanlines[curY-minY];
			m_scanlines[curY-minY] = NULL;
			goto repeatIt;
		};

		/* Handle arcs */
		a1 = &activeArcs;
		int arcs=0;
		repeatIt2:
		while (*a1) {
			i=numCoords-1;
			fy = curY - (*a1)->cy;
			fx = (*a1)->c1 - (*a1)->c3*fy*fy;
			if (fx > 0) fx = b_sqrt(fx); else fx = 0;

			if ((*a1)->sides & ARC_RIGHT) {
				hum = 1 + ((*a1)->sides & ARC_LEFT);
				x2 = (int32)(((*a1)->cx + fx)*65536.0);
				while ((i>=0) && (x2 < crossings[i].x)) {
					crossings[i+hum] = crossings[i]; i--;
				};
				crossings[i+hum].x = x2;
				crossings[i+hum].dir = (*a1)->dir;
				numCoords++;
			};
			if ((*a1)->sides & ARC_LEFT) {
				x1 = (int32)(((*a1)->cx - fx)*65536.0);
				while ((i>=0) && (x1 < crossings[i].x)) {
					crossings[i+1] = crossings[i]; i--;
				};
				crossings[++i].x = x1;
				crossings[i].dir = -(*a1)->dir;
				numCoords++;
			};
			if ((*a1)->yBot == curY) {
				*a1 = (*a1)->next;
			} else {
				a1 = &(*a1)->next;
			};
			arcs++;
		};
		if (m_arcScanlines[curY-minY]) {
			*a1 = m_arcScanlines[curY-minY];
			m_arcScanlines[curY-minY] = NULL;
			goto repeatIt2;
		};

		if (numCoords) {
			begin = 0;
			curDir = m_windings[curY-minY];
			if (curDir) {
				end = begin;
				beginX = leftX;
				goto jumpOnIn;
			};
			do {
				curDir = crossings[begin].dir;
				beginX = crossings[begin].x + beginAdd;
				end = begin+1;
				jumpOnIn:
				while (1) {
					if (end == numCoords) {
						endX = rightX;
						break;
					}
					curDir += crossings[end].dir;
					if (curDir == 0) {
						endX = crossings[end].x + endAdd;
						break;
					};
					end++;
				}
				
				if ((beginX < endX) || ((beginX < endX+2) && m_includeEdgePixels)) {
					spans[spanIndex] = curY;
					spans[spanIndex+1] = (beginX>>16);
					spans[spanIndex+2] = (endX>>16);
					spanIndex+=3;
					if (spanIndex == spanBatch) {
						grFillSpans(context, port, spans, spanIndex/3);
						spanIndex = 0;
					};
				};
				begin = end+1;
			} while (begin < numCoords);
		} else if (m_windings[curY-minY] != 0) {
			spans[spanIndex] = curY;
			spans[spanIndex+1] = m_bounds.left;
			spans[spanIndex+2] = m_bounds.right;
			spanIndex+=3;
			if (spanIndex == spanBatch) {
				grFillSpans(context, port, spans, spanIndex/3);
				spanIndex = 0;
			};
		};
		curY++;
	};

	if (spanIndex) grFillSpans(context, port, spans, spanIndex/3);
	if (crossings != stackCrossings) grFree(crossings);
	if (spans != stackSpans) grFree(spans);
}

status_t SEdges::ScanConvertAndRender(fill_rule /*fillingRule*/, RenderContext *context)
{
	rect clip_bounds = context->renderPort->drawingBounds;
#if ROTATE_DISPLAY
	if (context->renderPort->canvasIsRotated)
		context->renderPort->rotater->RotateRect(&clip_bounds, &clip_bounds);
#endif		
	DrawWindingRuns(context,context->spanBatch,clip_bounds);
	return B_OK;
};

void SEdges::OpenShapes(fpoint p)
{
	StartEdges(add(p,m_translation));
};

void SEdges::CloseShapes(bool /*closeLast*/)
{
	CloseEdges();
};

void SEdges::MoveTo(fpoint p, bool /*closeLast*/)
{
	CloseEdges();
	StartEdges(add(p,m_translation));
};

void SEdges::AddLineSegments(fpoint *p, int32 count)
{
	while (count--) AddEdge(add(*p++,m_translation));
};

void SEdges::AddBezierSegments(fpoint *p, int32 count)
{
	frect fbounds;
	fbounds.left = m_bounds.left-0.5;
	fbounds.top = m_bounds.top-0.5;
	fbounds.right = m_bounds.right+0.5;
	fbounds.bottom = m_bounds.bottom+0.5;

	while (count--) {
		fpoint pts[4];
		pts[0] = m_prevPt;
		pts[1] = add(p[0],m_translation);
		pts[2] = add(p[1],m_translation);
		pts[3] = add(p[2],m_translation);
	
		m_bezierBuffer.SetItems(0);
		TesselateClippedCubicBezier(pts,m_bezierBuffer,fbounds,0.4);
		for (int32 i=0;i<m_bezierBuffer.CountItems();i++)
			AddEdge(m_bezierBuffer[i]);
		p+=3;
	};
};
