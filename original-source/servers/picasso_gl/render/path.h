
//******************************************************************************
//
//	File:			path.h
//	Description:	Defines a Postscript-style path
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef PATH_H
#define PATH_H

#include "shape.h"
#include "BArray.h"
#include "renderContext.h"

enum {
	PATH_LINES = 1,
	PATH_BEZIER = 2,
	PATH_END = 3
};

class SPath : public SShaper {

	protected:

				BArray<fpoint>	m_points;
				BArray<uint32>	m_ops;
				uint32			m_buildingOp;
				RenderContext *	m_context;
	
	public:
		
								SPath(RenderContext *context);
								SPath(RenderContext *context, int32 ops, int32 pts);
								~SPath();
		
		virtual	void			OpenShapes(fpoint pt);
		virtual	void			CloseShapes(bool closeLast);
		
		virtual	void			MoveTo(fpoint pt, bool closeLast=false);
		virtual	void			AddLineSegments(fpoint *pt, int32 count);
		virtual	void			AddBezierSegments(fpoint *pt, int32 count);
		
				frect			Bounds();
				
		inline	BArray<uint32> *Ops();
		inline	BArray<fpoint> *Points();
		
				void			Clear();
				status_t		Transfer(SShaper *shape);
				
				void			Copy(SPath *path, fpoint translate, float scale);
};

inline BArray<uint32> * SPath::Ops()
{
	return &m_ops;
};

inline BArray<fpoint> * SPath::Points()
{
	return &m_points;
};

#endif
