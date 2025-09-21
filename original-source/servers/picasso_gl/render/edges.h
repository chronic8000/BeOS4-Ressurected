
//******************************************************************************
//
//	File:			edges.h
//	Description:	Low-level resolution dependent format for describing
//					rasterized polygons or collections of polygons.
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef EDGES_H
#define EDGES_H

#include <stdlib.h>
#include <math.h>
#include "renderInternal.h"
#include "BArray.h"
#include "renderArc.h"
#include "shape.h"

struct edge {
	edge *next;
	int8 yDir;
	int16 yTop, yBot;
	int32 x, dxdy;
};

enum fill_rule {
	B_EVEN_ODD_RULE=0,
	B_NON_ZERO_WINDING_RULE
};                

#define ARC_NEED_FIRST_EDGE 	0x01
#define EDGES_STARTED			0x02

struct RenderContext;

class SEdges : public SShaper
{

	public:

								SEdges(	int32 countGuess=1024, rect *bounds=NULL,
										bool m_includeEdgePixels=false,
										fpoint translation=fpoint(0,0));
								~SEdges();

		virtual	void			OpenShapes(fpoint p);
		virtual	void			CloseShapes(bool closeLast=false);
		
		virtual	void			MoveTo(fpoint pt, bool closeLast=false);
		virtual	void			AddLineSegments(fpoint *p, int32 count);
		virtual	void			AddBezierSegments(fpoint *p, int32 count);

				status_t		AddEdges(fpoint *vertices, int32 vCount);
		inline	status_t		AddEdge(fpoint p);
				status_t		AddArc(	fpoint center, fpoint radius,
										float startAng, float stopAng,
										int32 connectRule);
		inline	status_t		StartEdges(fpoint p1);
		inline	status_t		StartEdges(fpoint p1, fpoint p2);
		inline	status_t		CloseEdges();
		inline	int32			CountEdges();
		inline	void			SetArcHack(bool arcHack) {
									m_edgeEnds = m_includeEdgePixels & !arcHack;
								};
		
				status_t		ScanConvertAndRender(
									fill_rule fillingRule, RenderContext *context);
				status_t		SetBounds(rect *bounds);
				status_t		IncludeScanLines(int32 minY, int32 maxY);

				void			DrawWindingRuns(
									RenderContext *context,
									int32 spanBatch,
									rect clipBound);

	private:

				status_t		DoEdge(				fpoint p1,
													fpoint p2,
													int16 s,
													bool skipStart=false,
													bool skipEnd=false);

		inline	edge *			AllocEdge();
		
				fpoint			m_translation;
				bool			m_includeEdgePixels;
				int32			m_state;
				BArray<edge*>	m_edgeLists;
				BArray<arc>		m_arcList;
				edge **			m_scanlines;
				arc **			m_arcScanlines;
				int16 *			m_windings;
		
				int32			m_addList;
				int32			m_addIndex;
		
				int32			m_edgesPerList;
				rect			m_bounds;
				int32			m_width;
				bool			m_boundsSet;
				int32			m_scanLineCount;
		
				float			m_boundLeft;
				float			m_boundRight;
				
				int16			m_prevSign;
				uint16			m_flags;
				bool			m_edgeEnds, m_skipEndOfFirst, m_skipStartOfLast;
				fpoint			m_firstPt, m_secondPt, m_prevPt;

				BArray<fpoint>	m_bezierBuffer;
};

inline int32 SEdges::CountEdges()
{
	return (m_addList*m_edgesPerList) + m_addIndex;
};

inline edge *SEdges::AllocEdge()
{
	if (m_addIndex == m_edgesPerList) {
		m_edgeLists.AddItem((edge*)grMalloc(sizeof(edge)*m_edgesPerList,"AllocEdge",MALLOC_CANNOT_FAIL));
		m_addList++;
		m_addIndex = 0;
	};
	return &m_edgeLists[m_addList][m_addIndex++];
};

inline status_t SEdges::StartEdges(fpoint p1)
{
	m_firstPt = m_prevPt = p1;
	m_state = 1;
	m_skipEndOfFirst = m_skipStartOfLast = false;
	return B_OK;
}

inline status_t SEdges::StartEdges(fpoint p1, fpoint p2)
{
	m_prevSign = sgn(p2.y-p1.y);
	m_secondPt = m_prevPt = p2;
	m_firstPt = p1;
	m_state = 2;
	m_skipEndOfFirst = m_skipStartOfLast = false;
	m_flags |= ARC_NEED_FIRST_EDGE;
	return B_OK;
}

inline status_t SEdges::CloseEdges()
{
	status_t r;
	
	r = DoEdge(m_prevPt,m_firstPt,m_prevSign,m_skipStartOfLast,false);

	m_state = 0;
	if (r != B_OK) return r;
	if (m_flags & ARC_NEED_FIRST_EDGE) {
		r = DoEdge(m_firstPt,m_secondPt,m_prevSign,false,m_skipEndOfFirst);
	};

	m_flags = 0;
	return r;
}

inline status_t SEdges::AddEdge(fpoint p)
{
	if (m_state == 1) {
		m_secondPt = m_prevPt = p;
		m_flags |= ARC_NEED_FIRST_EDGE;
		m_prevSign = sgn(m_secondPt.y-m_firstPt.y);
		m_state = 2;
		return B_OK;
	};

	status_t r = DoEdge(m_prevPt,p,m_prevSign,m_skipStartOfLast,false);
	m_skipStartOfLast = false;
	m_prevPt = p;
	return r;
};

#endif
