//******************************************************************************
//
//	File:		fregion.h
//
//	Description:	Floating-point (scalable) regions
//	
//	Written by:	George Hoffman
//
//	Copyright 1993, Be Incorporated
//
//******************************************************************************/

#ifndef FREGION_H
#define FREGION_H

#include <IRegion.h>

#include "basic_types.h"
#include "BArray.h"

class fregion;
class fregion {

	frect			m_bounds;
	BArray<frect> 	m_rects;
	int32			m_signature;

public:
	
					fregion();
					fregion(frect *rects, int32 rectCount);
					fregion(region *r);
					fregion(fregion *r);
					~fregion();

	frect &			Bounds();
	void			Copy(region *reg);
	void			Copy(fregion *reg);
	void			Copy(frect *fr, int32 rectCount);
	int32			RectCount();
	frect &			RectAt(int32 index);
	region *		IRegion(float scale=1.0);
};

#endif
