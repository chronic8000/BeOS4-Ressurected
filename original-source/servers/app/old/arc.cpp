//******************************************************************************
//
//	File:			arc.cpp
//
//	Description:	1 bit rendering of arcs and wedges.
//	
//	Written by:		Robert Polic	
//
//	Copyright 1994, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#include <stdio.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef _MATH_H
#include <math.h>
#endif

#ifndef	RENDER_H
#include "render.h"
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

typedef	struct {
		long	x1;
		long	x2;
		} x_range;

/*-----------------------------------------------------------------*/
#define _PI 3.141592653589793238462643383279502884197169399
/*-----------------------------------------------------------------*/
void	ellipse_calc (x_range *vector_list, long xc, long yc, long a, long b);
/*-----------------------------------------------------------------*/

void	vector_calc (x_range *vector_list, long top, long bottom, long x1, long y1, long x2, long y2)
{
		long	x_temp, y_temp;
		long	d, x, y, ax, ay, sx, sy, dx, dy;

		dx = x2 - x1;	ax = abs(dx) << 1;
		if (dx < 0)
			sx = -1;
		else
			sx = 1;
		dy = y2 - y1;	ay = abs(dy) << 1;
		if (dy < 0)
			sy = -1;
		else
			sy = 1;

		x = x1;
		y = y1;

		if (ax > ay) {
			d = ay - (ax >> 1);
			for ( ; ; ) {
				if ((y >= top) && (y <= bottom)) {
					if (vector_list[y - top].x2 == 100000000) {
						vector_list[y - top].x1 = x;
						vector_list[y - top].x2 = x;
					}
					else vector_list[y - top].x2 = x;
				}
				if (x == x2) return;
				if (d >= 0) {
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		}
		else {
			d = ax - (ay >> 1);
			for ( ; ; ) {
				if (( y >= top) && (y <= bottom)) {
					if (vector_list[y - top].x2 == 100000000) {
						vector_list[y - top].x1 = x;
						vector_list[y - top].x2 = x;
					}
					else vector_list[y - top].x2 = x;
				}
				if (y == y2) return;
				if (d >= 0) {
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
}

/*-----------------------------------------------------------------*/

void DrawArc(RenderContext *context, rect *r, short flag, float fstart_angle, float farc_angle)
{
		region	*clip;
		long	i;
		x_range	*vector_list;
		x_range	*inner_vector_list;
		x_range	*start_vector;
		x_range	*end_vector;
		long	y;
		long	screen_y;
		long	x1;
		long	x2;
		rect	clip_rect;
		long 	phase_x;
		long 	phase_y; 
		long	center_x;
		long	center_y;
		long	a;
		long	b;
		long	inner_center_x;
		long	inner_center_y;
		long	inner_x1;
		long	inner_x2;
		long	inner_a;
		long	inner_b;
		rect	tmpRect;
		rect	innerRect;
		long	slop_x;
		long	slop_y;
		long	excess;
		double	width;
		double	height;
		double	theAngle;
		long	radius;
		long	x0, y0;
		bool	clockwise = FALSE;
		long	return_to;
		long	come_back;
		long	temp;
		long	pen_size = context->scaledPenSize + 0.5;
		long	start_angle = fstart_angle + 0.5;
		long	arc_angle = farc_angle + 0.5;
		FillRects renderRects;

		if ((r->left > r->right) || (r->top > r->bottom))
			return;

		renderRects = context->fFillRects;
		RenderCanvas *canvas = context->port->canvas;

		tmpRect = *r;	
		tmpRect.top -= pen_size/2;
		tmpRect.left -= pen_size/2;
		tmpRect.bottom -= pen_size/2;
		tmpRect.right -= pen_size/2;
		tmpRect.right += (pen_size - 1);
		tmpRect.bottom += (pen_size - 1);

		innerRect.left = tmpRect.left + pen_size;
		innerRect.top = tmpRect.top + pen_size;
		innerRect.right = tmpRect.right - pen_size;
		innerRect.bottom = tmpRect.bottom - pen_size;

		if (innerRect.left > innerRect.right) {
			innerRect.left = 0;
			innerRect.right = 0;
			flag = FALSE;
		}
		if (innerRect.top > innerRect.bottom) {
			innerRect.top = 0;
			innerRect.bottom = 0;
			flag = FALSE;
		}

		center_x = (tmpRect.left + tmpRect.right) >> 1;
		center_y = (tmpRect.bottom + tmpRect.top) >> 1;
		a = (tmpRect.right - tmpRect.left) >> 1;
		b = (tmpRect.bottom - tmpRect.top) >> 1;

		inner_center_x = (innerRect.left + innerRect.right) >> 1;
		inner_center_y = (innerRect.bottom + innerRect.top) >> 1;
		inner_a = (innerRect.right - innerRect.left) >> 1;
		inner_b = (innerRect.bottom - innerRect.top) >> 1;

		slop_x = (tmpRect.right - tmpRect.left) & 1;
		slop_y = (tmpRect.bottom - tmpRect.top) & 1;

		if (arc_angle == 0)
			return;
		if (arc_angle > 360)
			arc_angle = 360;
		if (arc_angle < -360)
			arc_angle = -360;
		if (arc_angle < 0)
			clockwise = TRUE;
		while (start_angle < 0)
			start_angle += 360;
		if (excess = start_angle / 360)
			start_angle -= excess * 360;
		arc_angle += start_angle;

		while (arc_angle < 0)
			arc_angle += 360;
		if (excess = arc_angle / 360)
			arc_angle -= excess * 360;
		if (clockwise) {
			temp = start_angle;
			start_angle = arc_angle;
			arc_angle = temp;
		}

		width = (tmpRect.right - tmpRect.left) / 2.0;
		height = (tmpRect.bottom - tmpRect.top) / 2.0;
		radius = sqrt(width * width + height * height);

		theAngle = ((360.0 - start_angle) / 180.0) * _PI;
		x0 = radius * cos(theAngle);
		if (x0 < 0.0)
			x0 -= 0.5;
		else
			x0 += 0.5;
		y0 = radius * sin(theAngle);
		if (y0 < 0.0)
			y0 -= 0.5;
		else
			y0 += 0.5;

		start_vector = (x_range *)gr_malloc((((b + slop_y) * 2) + 4) * sizeof(x_range));
		for (y = 0; y <= ((b + slop_y) * 2); y++) {
			start_vector[y].x1 = -100000000;
			start_vector[y].x2 = 100000000;
		}
		vector_calc(start_vector, tmpRect.top, tmpRect.bottom, center_x, center_y, center_x + x0, center_y + y0);
		for (y = b - 1; y >= 0; y--)
			if (start_vector[y].x1 == -100000000) {
				start_vector[y].x1 = start_vector[y + 1].x1;
				start_vector[y].x2 = start_vector[y + 1].x2;
			}
		for (y = b + 1; y <= (b * 2) + slop_y; y++)
			if (start_vector[y].x1 == -100000000) {
				start_vector[y].x1 = start_vector[y - 1].x1;
				start_vector[y].x2 = start_vector[y - 1].x2; 
			}

		theAngle = ((360.0 - arc_angle) / 180.0) * _PI;
		x0 = radius * cos(theAngle);
		if (x0 < 0.0)
			x0 -= 0.5;
		else
			x0 += 0.5;
		y0 = radius * sin(theAngle);
		if (y0 < 0.0)
			y0 -= 0.5;
		else
			y0 += 0.5;
		end_vector = (x_range *)gr_malloc((((b + slop_y) * 2) + 4) * sizeof(x_range));
		for (y = 0; y <= ((b + slop_y) * 2); y++) {
			end_vector[y].x1 = -100000000;
			end_vector[y].x2 = 100000000;
		}
		vector_calc(end_vector, tmpRect.top, tmpRect.bottom, center_x, center_y, center_x + x0, center_y + y0);
		for (y = b - 1; y >= 0; y--)
			if (end_vector[y].x1 == -100000000) {
				end_vector[y].x1 = end_vector[y + 1].x1;
				end_vector[y].x2 = end_vector[y + 1].x2;
			}
		for (y = b + 1; y <= (b * 2) + slop_y; y++)
			if (end_vector[y].x1 == -100000000) {
				end_vector[y].x1 = end_vector[y - 1].x1;
				end_vector[y].x2 = end_vector[y - 1].x2; 
			}

		phase_x = context->phase_x;
		clip = context->drawRegion;
	
		vector_list = (x_range *)gr_malloc((((b + slop_y) * 2) + 4) * sizeof(x_range));

		for (y = 0; y <= ((b + slop_y) * 2); y++) {
			vector_list[y].x1 = center_x; /* 100000000; */
			vector_list[y].x2 = center_x; /* -1000000000; */
		}
		ellipse_calc(vector_list, center_x, b, a, b);

		if (flag) {
			inner_vector_list = (x_range *)gr_malloc((((inner_b + slop_y) * 2) + 4) * sizeof(x_range));
			for (y = 0; y <= ((inner_b + slop_y) * 2); y++) {
				inner_vector_list[y].x1 = inner_center_x; /* 100000000; */
				inner_vector_list[y].x2 = inner_center_x; /* -100000000; */
			}
			ellipse_calc(inner_vector_list, inner_center_x, inner_b, inner_a, inner_b);
		}

		if (slop_y) {
			for (y = (b * 2) + slop_y; y > b; y--)
				vector_list[y] = vector_list[y - 1];
			if (flag)
				for (y = (inner_b * 2) + slop_y; y > inner_b; y--)
					inner_vector_list[y] = inner_vector_list[y - 1];
		}

		grLockCanvas(canvas,&tmpRect);	
		phase_y = context->phase_y;
	
		for (y = 0; y < pen_size; y++) {
			screen_y = y + tmpRect.top;

			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
			return_to = 1;
			goto bounds_check;
top_section:;
		}

		if (pen_size < (b * 2) + slop_y) {
			for (; y <= ((inner_b * 2) + slop_y + pen_size); y++) {

				screen_y = y + tmpRect.top;
				x1 = vector_list[y].x1;
				x2 = vector_list[y].x2 + slop_x;

				if (x2 >= x1)
					if (flag) {
						x2 = inner_vector_list[y - pen_size].x1 - 1;
						return_to = 2;
						goto bounds_check;
middle_section_1:;
						x1 = inner_vector_list[y - pen_size].x2 + 1 + slop_x;
						x2 = vector_list[y].x2 + slop_x;
						return_to = 3;
						goto bounds_check;
middle_section_2:;
					}
					else {
						return_to = 4;
						goto bounds_check;
middle_section_3:;
					}
			}
		}

		for (; y <= (b * 2) + slop_y; y++) {

			screen_y = y + tmpRect.top;
			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
		
			if (x2 >= x1) {
				return_to = 5;
				goto bounds_check;
bottom_section:;
			}
		}

		grUnlockCanvas(canvas);
	
		gr_free((char *)start_vector);
		gr_free((char *)end_vector);
		gr_free((char *)vector_list);
		if (flag)
			gr_free((char*)inner_vector_list);
		goto done;

bounds_check:;

	if (x2 >= x1) {
		come_back = 0;
		if (start_angle < 180) {	/* Start in quad 1 or 2, arc anywhere */
			if (arc_angle > start_angle) {
				if ((arc_angle < 180) && (y < b)) {
					if (x1 < end_vector[y].x2)
						x1 = end_vector[y].x2;
					if (x2 > start_vector[y].x2)
						x2 = start_vector[y].x2;
					goto render_it;
				}
				else
				if ((arc_angle >= 180) && (y < b)) {
					if (x2 > start_vector[y].x2)
						x2 = start_vector[y].x2;
					goto render_it;
				}
				else
				if ((arc_angle >= 180) && (y >= b) && (x1 <= end_vector[y].x2)) {
					if (x2 > end_vector[y].x2)
						x2 = end_vector[y].x2;
					goto render_it;
				}
			}
			else {
				if (y >= b)
					goto render_it;
				else
					if (x2 <= start_vector[y].x2)
						goto render_it;
					else {
						temp = x2;
						x2 = start_vector[y].x2;
						come_back = 1;
						goto render_it;
section_1:;
						come_back = 0;
						x2 = temp;
						if (x1 >= end_vector[y].x2)
							goto render_it;
						x1 = end_vector[y].x2;
						goto render_it;
					}
			}
		}
		else {		/* Start in quad 2 or 3, arc anywhere */
			if ((arc_angle > start_angle) && (y < b))
				goto go_back;
			else
			if ((arc_angle > start_angle) && (y >= b)) {
				if (x1 < start_vector[y].x2)
					x1 = start_vector[y].x2;
				if (x2 > end_vector[y].x2)
					x2 = end_vector[y].x2;
				goto render_it;
			}
			else
			if ((arc_angle < 180) && (y < b)) {
				if (x1 < end_vector[y].x2)
					x1 = end_vector[y].x2;
				goto render_it;
			}
			else
			if ((arc_angle < 180) && (y >= b)) {
				if (x1 < start_vector[y].x2)
					x1 = start_vector[y].x2;
				goto render_it;
			}
			else
			if ((arc_angle >= 180) && (y < b))
				goto render_it;
			else {
				temp = x2;
				if (x2 > end_vector[y].x2)
					x2 = end_vector[y].x2;
				come_back = 2;
				goto render_it;
section_2:;
				come_back = 0;
				x2 = temp;
				if (x1 >= start_vector[y].x2)
					goto render_it;
				x1 = start_vector[y].x2;
				goto render_it;
			}
		}
	}
	goto go_back;

render_it:;
	if (x2 >= x1) {
		rect r;
		r.top = r.bottom = screen_y;
		r.left = x1; r.right = x2;
		renderRects(context,canvas,&r,1,clip);
	};

	switch (come_back) {
		case 0:
			break;
		case 1:
			goto section_1;
		case 2:
			goto section_2;
	}

go_back:;
	switch (return_to) {
		case 1:
			goto top_section;
		case 2:
			goto middle_section_1;
		case 3:
			goto middle_section_2;
		case 4:
			goto middle_section_3;
		case 5:
			goto bottom_section;
	}

done:;
}
