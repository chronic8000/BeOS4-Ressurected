
//******************************************************************************
//
//	File:			lines.h
//	Description:	Utility routines to generate polygonal thick lines
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef	LINES_H
#define	LINES_H

#include "renderInternal.h"
#include "path.h"

typedef void (*LineCapFunc)(
	SThickener *thickener,
	fpoint p, fpoint delta);

typedef void (*LineJoinFunc)(
	SThickener *thickener,
	fpoint corner, fpoint delta1, fpoint delta2);

/*	SThinLineRenderer will actually lock the port and canvas and
	render the lines you give it as you give them to it. */
class SThinLineRenderer : public SShaper {

	private:

				BArray<fpoint>	m_curve;
				frect			m_clipBounds;
				RenderContext *	m_context;
				
	public:

								SThinLineRenderer(RenderContext *context);
								~SThinLineRenderer();

		virtual	void			OpenShapes(fpoint pt);
		virtual	void			CloseShapes(bool closeLast=false);

		virtual	void			MoveTo(fpoint pt, bool closeLast=false);
		virtual	void			AddLineSegments(fpoint *pt, int32 count);
		virtual	void			AddBezierSegments(fpoint *pt, int32 count);
};

/*	SThickener does no rendering, it just expands the path it's given
	into a thick line path. */
class SThickener : public SPath {

	private:

				BArray<fpoint>	m_curve;
				frect			m_clipBounds;
				RenderContext *	m_context;

				LineCapFunc		m_capFunc;
				LineCapFunc		m_internalCapFunc;
				LineJoinFunc	m_joinFunc;

				float			m_theta;
				int32			m_numThetaSteps;
				int32			m_thetaStepsHighWater;
				float *			m_cosThetaSteps;
				float *			m_sinThetaSteps;
				float			m_cosMiterLimit;
				float			m_halfPen;
				float			m_penSize2_;

				float			m_lastDelta;

				void			JoinedLines(fpoint *curve, int32 curveLen, bool closed);
				void			CappedLines(fpoint *curve, int32 curveLen, bool closed);

	public:
		
								SThickener(RenderContext *context);
								~SThickener();

		static	void 			GenerateRoundCap(SThickener *shape, fpoint p, fpoint delta);
		static	void 			GenerateSquareCap(SThickener *shape, fpoint p, fpoint delta);
		static	void 			GenerateButtCap(SThickener *shape, fpoint p, fpoint delta);
		static	void 			GenerateRoundCorner(SThickener *shape, fpoint corner, fpoint delta1, fpoint delta2);
		static	void 			GenerateMiterCorner(SThickener *shape, fpoint corner, fpoint delta1, fpoint delta2);
		static	void 			GenerateBevelCorner(SThickener *shape, fpoint corner, fpoint delta1, fpoint delta2);
		
				void			LineDependencies();
				void			UpdateClipBounds();
				void			IgnoreClipBounds();
				void			ThickenBuffer(bool close);
		
		virtual	void			OpenShapes(fpoint pt);
		virtual	void			CloseShapes(bool closeLast=false);

		virtual	void			MoveTo(fpoint pt, bool closeLast=false);
		virtual	void			AddLineSegments(fpoint *pt, int32 count);
		virtual	void			AddBezierSegments(fpoint *pt, int32 count);
};

#endif
