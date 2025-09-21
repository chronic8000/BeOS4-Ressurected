
//******************************************************************************
//
//	File:			rect.h
//	Description:	Interface to the rect code
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RECT_H
#define RECT_H

#include "basic_types.h"

void		offset_rect(rect *, long, long);
void		offset_frect(frect *, float, float);
char		equal_rect(const rect *, const rect *);
char		point_in_rect(const rect *, long, long);
void		inset_rect(rect *, long, long);
void		sect_rect(rect, rect, rect *);
char 		overlap_rect(rect rect1, rect rect2);
void		union_rect(const rect *rect1, const rect *rect2, rect *result);
void		sort_rect(rect *);
char		enclose(const rect *r1, const rect *r2);

#endif
