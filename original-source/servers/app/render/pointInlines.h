
#ifndef POINT_INLINES_H
#define POINT_INLINES_H

#include "basic_types.h"

inline fpoint add(fpoint p0, fpoint p1)
{
	fpoint p;
	p.h = p0.h + p1.h;
	p.v = p0.v + p1.v;
	return p;
};

inline fpoint sub(fpoint p0, fpoint p1)
{
	fpoint p;
	p.h = p0.h - p1.h;
	p.v = p0.v - p1.v;
	return p;
};

inline point f2ipoint(fpoint f)
{
	point p;
	p.h = (int32)(f.h + 0.5);
	p.v = (int32)(f.v + 0.5);
	return p;
};

inline fpoint MidPoint(fpoint p0, fpoint p1)
{
	fpoint p;
	p.h = (p0.h + p1.h)*0.5;
	p.v = (p0.v + p1.v)*0.5;
	return p;
};

inline int FindIntersection(fpoint p0, fpoint d0, fpoint p1, fpoint d1, fpoint &p)
{
	float t1,cross;
	
	cross = d1.h*d0.v - d1.v*d0.h;
		
	t1 = ((p1.v-p0.v)*d0.h - (p1.h-p0.h)*d0.v) / cross;
	if (fabs(t1) > 1e10)
		return -1;
	p.h = p1.h + d1.h*t1;
	p.v = p1.v + d1.v*t1;
	return 0;
};

inline int FindSegmentIntersection(fpoint s1, fpoint s2, fpoint t1, fpoint t2, fpoint &p)
{
	fpoint sd,td;
	float t;
	
	sd.h = s2.h - s1.h;
	sd.v = s2.v - s1.v;
	td.h = t2.h - t1.h;
	td.v = t2.v - t1.v;
	
	if (FindIntersection(s1,sd,t2,td,p) != 0)
		return 1;

	if (sd.v > sd.h)
		t = ((p.v - s1.v)/sd.v);
	else
		t = ((p.h - s1.h)/sd.h);
	if (t < 0)
		return -1;

	if (td.v > td.h)
		t = ((p.v - t1.v)/td.v);
	else
		t = ((p.h - t1.h)/td.h);
	if (t > 1)
		return -1;
		
	return 0;	
};

#endif
