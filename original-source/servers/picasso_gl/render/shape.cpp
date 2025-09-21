
//******************************************************************************
//
//	File:		shape.cpp
//
//	Description:	High-level device independent description format for
//				describing arbitrary shapes
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "BArray.h"
#include "shape.h"
#include "edges.h"
#include <math.h>
#include <support2/Debug.h>

/* fpi == fixed point integer */
struct fpipoint {
	int32 h,v;
};


inline point f2ipoint(fpoint f)
{
	point p;
	p.h = f.h + 0.5;
	p.v = f.v + 0.5;
	return p;
};

inline fpipoint f2fpipoint(fpoint f)
{
	fpipoint p;
	p.h = (f.h * 65536.0) + 0.5;
	p.v = (f.v * 65536.0) + 0.5;
	return p;
};

inline fpoint add(fpoint p1, fpoint p2)
{
	fpoint p;
	p.h = p1.h + p2.h;
	p.v = p1.v + p2.v;
	return p;
};

frect PolyShape::Bounds()
{
	return m_bound;
};

int32 PolyShape::IterateStart()
{
	return 0;
};

bool PolyShape::Iterate(int32 *token, fpoint **vertices, int32 *vertexCount)
{
	int32 i=0,t=*token;

	if (t >= m_polyPoints.CountItems())
		return false;
	*vertices = m_polyPoints.Items() + t;
	while (m_polyPoints[t].h < (float)1e29) {
		t++; i++;
	};
	t++;
	*token = t;
	*vertexCount = i;
	return true;
};

PolyShape::PolyShape() : m_polyPoints(256)
{
	m_bound.top = 1e30;
	m_bound.bottom = -1e30;
	m_bound.left = 1e30;
	m_bound.right = -1e30;
	m_error = false;
};

PolyShape::~PolyShape()
{
};

#if 0

struct IRegionBoundary {
	int16	pixel,r;
	int8		entry;
	float	otherAxisBegin, otherAxisEnd;
	float	pixelF;
};

/* Pierre's quicksort makes another appearance */
void kiltsort(IRegionBoundary **imin, IRegionBoundary **imax)
{
	IRegionBoundary	*iref;
	IRegionBoundary	**imin2, **imax2;
	uint16			index;
	int32			offset;
	int32			ref;
	
	// init sorting
	imin2 = imin;
	imax2 = imax;
	// randomize the quicksort
	index = imax-imin;
	if (index > 2) {	
		if ((crc_alea <<= 1) < 0)
			crc_alea ^= 0x71a3;
		index = (index>>1)|(index>>2);
		index |= index>>2;
		index &= (crc_alea>>11);
		offset = -1-index;
		iref = imax2[offset];
		imax2[offset] = *imax2;
	}
	else {
		iref = *imax2;
	}

	ref = (((int32)iref->pixel)<<17)+iref->entry;
	// do iterative sorting
	while (imin2 != imax2) {
		if (*imin2 < iref) {
			*imax2-- = *imin2;
		loop_min:
			if (imin2 == imax2) break;
			if (((((int32)(*imax2)->pixel)<<17)+(*imax2)->entry) <= ref) {
				imax2--;
				goto loop_min;
			}
			*imin2 = *imax2;
		}
		imin2++;
	}

	// put the reference back
	*imin2 = iref;

	// do recursive sorting
	if (imin2-1 > imin)
		kiltsort(imin, imin2-1);
	if (imax2+1 < imax)
		kiltsort(imax2+1, imax);
}

void Shape::ConvertAndClipEdgesRecurse(
	fpoint p1, fpoint p2,
	IRegionBoundary **hp, IRegionBoundary **vp,
	int32 xDir, int32 yDir,
	int32 rNum, Edges &edges)
{
	IRegionBoundary *h, IRegionBoundary *v;
	
	if (rNum>=0) {
		newRect:
		while ((h=(hp+(xDir==1))) && (sgn(p2.h-h->pixelF)==xDir))
		{
			hp += xDir;
			if (h->r == rNum) {
				/*	We've hit an exit boundary.  Intersect
					the line with the rectangle edge to see if we hit
					it within it's bounds. */
					slope = (p2.v-p1.v)/(p2.h-p1.h);
					y = FindIntersectionWithX(p1,slope,h->pixelF);
					if ((y>=h->otherAxisBegin) && (y<=h->otherAxisEnd)) {
						/*	We hit this edge.  Check to see if we're
							continuing on into another drawable rectangle.
							If so, we want to keep the edge whole. */
						hp += (xDir==1); // hp is now the considered edge
						while ((*hp) && (h->pixel == (*hp)->pixel)) {
							/* Check to see if we're entering this rectangle */
							if ((*hp)->entry && (y>=(*hp)->otherAxisBegin) && (y<=(*hp)->otherAxisEnd)) {
								/*	Yep, we're entering another rectangle, so we don't need
									to split this edge.  Update the state to reflect that
									we're in a new rectangle and continue. */
								rNum = (*hp)->r;
								goto newRect;
							};
							/*	Not entering that one, try the next */
							hp+=xDir;
						};
						hp -= (xDir==1); // hp is now the edge to the left
						/*	We didn't enter any neighboring rectangle directly, so
							we have to split this edge.  Add the partial edge to the Edges
							object. */
						pTmp.h = h->pixelF;
						pTmp.v = y;
						edges.AddEdge(p1,pTmp);
						
						/*	Now recurse.  Before we recurse, we have to . */
						while ((h=(hp+(xDir==1))) && (sgn(pTmp.h-h->pixelF)==xDir)) hp+=xDir;
						while ((v=(vp+(yDir==1))) && (sgn(pTmp.v-v->pixelF)==yDir)) vp+=yDir;
						ConvertAndClipEdgesRecurse(pTmp,p2,hb,vb);
					};
			};
		};
	};	
};


void Shape::ConvertAndClipEdges(region *clippingRegion, Edges &edges)
{
	int32 i,hNum,vNum,rNum,xDir,yDir;
	fpoint p1,p2,pTmp;
	float x,y,slope;
	int32 rectCount = clippingRegion;
	int32 rc4=rectCount*4,rc2=rectCount*2;
	IRegionBoundary *boundaries = 
		(IRegionBoundary*)malloc(sizeof(IRegionBoundary)*rc4 + 4);
	IRegionBoundary **boundsPtrs =
		(IRegionBoundary**)malloc(sizeof(IRegionBoundary*)*rc4 + 4);
	boundsPtrs[0] = &boundaries[rc4];
	boundsPtrs[rc2] = &boundaries[rc4+1];
	boundsPtrs[rc2+1] = &boundaries[rc4+2];
	boundsPtrs[rc4+3] = &boundaries[rc4+3];
	IRegionBoundary **hBounds = boundsPtrs+1;
	IRegionBoundary **vBounds = hBounds + (rc2) + 1;
	
	Bitmap hBits(rectCount);
	Bitmap vBits(rectCount);

	IRegionBoundary *alloc=boundaries,**hp=hBounds,**vp=vBounds,*h,*v;
	rect *r=clippingRegion->data;
	for (i=0;i<rectCount;i++) {
		*hp = h = alloc++;
		h->pixelF = (float)(h->pixel = r->left)+0.5;
		h->r = i;
		h->entry = 1;
		h->otherAxisBegin = ((float)r->top) - 0.5;
		h->otherAxisEnd = ((float)r->bottom) + 0.5;

		*hp = h = alloc++;
		h->pixelF = (float)(h->pixel = r->right+1)-0.5;
		h->r = i;
		h->entry = 0;
		h->otherAxisBegin = ((float)r->top) - 0.5;
		h->otherAxisEnd = ((float)r->bottom) + 0.5;

		*vp = v = alloc++;
		v->pixelF = (float)(v->pixel = r->top)-0.5;
		h->r = i;
		h->entry = 1;
		v->otherAxisBegin = ((float)r->left) - 0.5;
		v->otherAxisEnd = ((float)r->right) + 0.5;

		*vp = v = alloc++;
		v->pixelF = (float)(v->pixel = r->bottom+1)-0.5;
		h->r = i;
		h->entry = 0;
		v->otherAxisBegin = ((float)r->left) - 0.5;
		v->otherAxisEnd = ((float)r->right) + 0.5;
		r++;
	};

	kiltsort(hBounds,hBounds+rc2-1);
	kiltsort(vBounds,vBounds+rc2-1);

	fpoint *points = m_polyPoints.Items();
	int32 pointCount = m_polyPoints.CountItems();

	i = 0;
	while (i<pointCount) {
		start = i;

		vp = hBounds;
		hp = vBounds;
		rNum = -1;

		p1 = points[i];
		while ((*hp) && (p1.h > (*hp)->pixelF)) hp++; hp--;
		if ((*hp)->entry) hNum = (*hp)->r; else hNum = -65536;
		while ((*vp) && (p1.v > (*vp)->pixelF)) vp++; vp--;
		if ((*vp)->entry) vNum = (*vp)->r; else vNum = -65536;
		if ((hNum+vNum) > 0)
			rNum = hNum;
				
		while (points[++i].h < ((float)1e29)) {
			p2 = points[i];
			xDir = sgn(p2.h-p1.h);
			yDir = sgn(p2.v-p1.v);
			
			
			
			newRect:
			
			if (rNum>=0) {
				while ((h=(hp+(xDir==1))) && (sgn(p2.h-h->pixelF)==xDir))
				{
					hp += xDir;
					if (h->r == rNum) {
						/*	We've hit an exit boundary.  Intersect
							the line with the rectangle edge to see if we hit
							it within it's bounds. */
							slope = (p2.v-p1.v)/(p2.h-p1.h);
							y = FindIntersectionWithX(p1,slope,h->pixelF);
							if ((y>=h->otherAxisBegin) && (y<=h->otherAxisEnd)) {
								/*	We hit this edge first.  Check to see if we're
									continuing on into another drawable rectangle.
									If so, we want to keep the edge whole. */
								hp+=xDir;
								while ((*hp) && (h->pixel == (*hp)->pixel)) {
									/* Check to see if we're entering this rectangle */
									if ((*hp)->entry && (y>=(*hp)->otherAxisBegin) && (y<=(*hp)->otherAxisEnd)) {
										/*	Yep, we're entering another rectangle, so we don't need
											to split this edge.  Update the state to reflect that
											we're in a new rectangle (including catching up with
											the Y component) and continue. */
										rNum = hNum = vNum = (*hp)->r;
										vp += (yDir==1); while ((*vp)->r != rNum) vp+=yDir;
										goto newRect;
									}; 
									/*	Not entering that one, try the next */
								};
								/*	We didn't enter any neighboring rectangle directly, so
									we have to split this edge.  Add the partial edge to the Edges
									object and update the state to reflect that we're now outside
									any clipping rectangle.  Then jump to the clipped case. */
								pTmp.h = h->pixelF;
								pTmp.v = y;
								edges.AddEdge(p1,pTmp);
								rNum = -1;
								vp += (yDir==1); while ((*vp)->r != rNum) vp+=yDir;
								goto outsideClip;
							};
					};
				};
			};
			
			outsideClip:
			
			/*	We are not currently inside any clipping region.  Step until
				we are or we reach the end of the line. */
			while ((h=(hp+(xDir==1))) && (sgn(p2.h-h->pixelF)==xDir)) {
				hp+=xDir;
				if (h->entry == (xDir==1)) {
				};
			};
			while ((v=(vp+(yDir==1))) && (sgn(p2.v-v->pixelF)==yDir)) vp+=yDir;
			
		};
	};
};

#endif

int32 PolyShape::PointOverEstimate()
{
	return m_polyPoints.CountItems();
};

bool PolyShape::ConvertToEdges(Edges &edges, fpoint translate, bool edgePixels)
{
	int32 start,i,j;
	fpoint p1,p2;

	if (m_error) return false;

	fpoint *points = m_polyPoints.Items();
	int32 pointCount = m_polyPoints.CountItems();

	/* 	Now, if we need edge pixels, we have to find a place in the poly to
		start which spans at least two scan lines.  This is because the edge
		code has a problem with starting with edges of slope 0. 
		
		NOTE: This assumes there is only ONE SHAPE in the shape buffer.  This
		is true in every case in which this routine would get called with
		edgePixels == true. */

	if (edgePixels) {
		i = 0;
		while (i<pointCount-1) {
			p1 = add(points[i],translate);
			p2 = add(points[(i+1)%(pointCount-1)],translate);
			if (floor(p1.v+0.5) != floor(p2.v+0.5)) break;
			i++;
		};
		if (i == (pointCount-1)) return false;
		edges.StartEdges(p1,p2);
		start = i;
		j=2;
		while (((start+j)%(pointCount-1)) != i) {
			p2 = add(points[(start+j)%(pointCount-1)],translate);
			edges.AddEdge(p2);
			j++;
		};
		edges.CloseEdges();
		return true;
	};

	i = 0;
	while (i<pointCount) {
		start = i;
		p1 = points[start];
		p2 = points[start+1];
		p1 = add(p1,translate);
		p2 = add(p2,translate);
		edges.StartEdges(p1,p2);
		j=2;
		while (start==i) {
			p2 = points[start+j];
			if (p2.h > ((float)1e29)) {
				p2 = points[start];
				i = start+j+1;
			} else {
				p2 = add(p2,translate);
				edges.AddEdge(p2);
			};
			j++;
		};
		edges.CloseEdges();
	};
	return true;
};
