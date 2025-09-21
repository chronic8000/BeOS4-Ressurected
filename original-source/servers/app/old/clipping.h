
//******************************************************************************
//
//	File:		clipping.h
//
//	Description:	Various routines useful for clipping
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef CLIPPING_H
#define CLIPPING_H

#ifndef GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif 

#ifndef AS_SUPPORT_H
#include "as_support.h"
#endif 

#ifndef	RENDER_H
#include "render.h"
#endif

#define B_CLIPPED_IN	0x0000
#define B_CLIPPED_OUT	0xFFFF

inline float FindIntersectionWithX(fpoint p0, float slope, float x)
{
	return p0.v + (x-p0.h)*slope;
};

inline float FindIntersectionWithY(fpoint p0, float slope, float y)
{
	return p0.h + (y-p0.v)/slope;
};

uint16 ClipLineToLeftX(fpoint *p1, fpoint *p2, float x)
{
	fpoint d;
	d.h = p2->h-p2->h;
	d.v = p2->v-p2->v;
	float y = FindIntersectionWithX(*p1,d,x);
	if (sgn(p1->y-y) == sgn(p2->y-y)) {
		if ((x-p2.x)>0) {
			p2->x = x;
			p2->y = y;
		} else {
			p1->x = x;
			p1->y = y;			
		};
	};
};

uint16 ClipLineToRightX(fpoint *p1, fpoint *p2, float x)
{
	fpoint d;
	d.h = p2->h-p2->h;
	d.v = p2->v-p2->v;
	float y = FindIntersectionWithX(*p1,d,x);
	if (sgn(p1->y-y) == sgn(p2->y-y)) {
		if ((x-p1.x)>0) {
			p2->x = x;
			p2->y = y;
		} else {
			p1->x = x;
			p1->y = y;			
		};
	};	
};

uint16 ClipLineToTopY(fpoint *p1, fpoint *p2, float y);
{
	fpoint d;
	d.h = p2->h-p2->h;
	d.v = p2->v-p2->v;
	float x = FindIntersectionWithY(*p1,d,y);
	if (sgn(p1->x-x) == sgn(p2->x-x)) {
		if ((y-p2.y)>0) {
			p2->x = x;
			p2->y = y;
		} else {
			p1->x = x;
			p1->y = y;			
		};
	};	
};

uint16 ClipLineToBottomY(fpoint *p1, fpoint *p2, float y);
{
	fpoint d;
	d.h = p2->h-p2->h;
	d.v = p2->v-p2->v;
	float x = FindIntersectionWithY(*p1,d,y);
	if (sgn(p1->x-x) == sgn(p2->x-x)) {
		if ((y-p1.y)>0) {
			p2->x = x;
			p2->y = y;
		} else {
			p1->x = x;
			p1->y = y;			
		};
	};	
};

uint16 ClipLineToRect(
	fpoint *p1, fpoint *p2, frect *clipRect);
uint16 ClipLineToRegion(
	fpoint p1, fpoint p2, region *clipRegion,
	fpoint *newP1s, fpoint newP2s);

#endif
