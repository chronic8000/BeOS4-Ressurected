
//******************************************************************************
//
//	File:			shape.h
//	Description:	A base class for interchange of line/bezier/arc shapes
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef SHAPE_H
#define SHAPE_H

#include "renderInternal.h"
#include "basic_types.h"

class SShaper
{
public:
							SShaper() {};
	virtual					~SShaper() {};

	virtual	void			OpenShapes(fpoint) {};
	virtual	void			CloseShapes(bool /*closeLast=false*/) {};
	
	virtual	void			MoveTo(fpoint, bool /*closeLast=false*/) {};
	virtual	void			AddLineSegments(fpoint *, int32 /*count*/) {};
	virtual	void			AddBezierSegments(fpoint *, int32 /*count*/) {};
};

#endif
