//******************************************************************************
//
//	File:		Point.h
//
//	Description:	BPoint class header.
//	
//	Written by:	Steve Horowitz
//
//	Revision history
//	1/20/94 tdl	Added BPoint::ScriptGetPackLen, ::ScriptPack, ::ScriptUnpack
//	3/05/94 benoit	Kept only the minimum set to be compatible with new Bparcel
//
//	Copyright 1993, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef	_POINT_H
#define	_POINT_H

#ifndef _APP_DEFS_H
#include <AppDefs.h>
#endif

class BPoint {

public:
	long x;
	long y;

	// constructors
	BPoint();
	BPoint(long X, long Y);
	BPoint(const BPoint& pt);
	
	// assignment
	void		Set(long X, long Y);
};


inline BPoint::BPoint()
{
}

inline BPoint::BPoint(long X, long Y)
{
	x = X;
	y = Y;
}

inline BPoint::BPoint(const BPoint& pt)
{
	x = pt.x;
	y = pt.y;
}

inline void BPoint::Set(long X, long Y)
{
	x = X;
	y = Y;
}

#endif
