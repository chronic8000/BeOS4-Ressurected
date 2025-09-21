//******************************************************************************
//
//	File:			ellipse.c
//
//	Description:	1 bit rendering of ellipses and circles.
//	
//	Written by:		Benoit Schillings
//
//	Copyright 1992, Be Incorporated
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

/*-----------------------------------------------------------------*/

typedef	struct {
		long	x1;
		long	x2;
		} x_range;

#define POWERPC 1

/*-----------------------------------------------------------------*/

void	ellipse_calc (x_range *vector_list, long xc, long yc, long a, long b)
{
	long	x = 0;
	long	y;
	float	b2;
	float	a2;
	float	d1;
	float	d2;
	float	x1;
	float	y1;

	y = b;
	b2 = b * b;
	a2 = a * a;
	d1 = b2 - a2 * b + a2 / 4;
	if (a > b) {
		vector_list[b - y].x1 = xc - a;
		vector_list[b - y].x2 = xc + a;
		vector_list[b + y].x1 = xc - a;
		vector_list[b + y].x2 = xc + a;
	}
	else {
		vector_list[b - y].x1 = xc - x;
		vector_list[b - y].x2 = xc + x;
		vector_list[b + y].x1 = xc - x;
		vector_list[b + y].x2 = xc + x;
	}

	y1 = y;
	while (a2 * (y1 - 0.5) > b2 * (x + 1)) {
		if (d1 < 0) {
			d1 += b2 * (2 * x + 3);
			x += 1;
		}
		else {
			d1 += b2 * (2 * x + 3) + a2 * (-2 * y + 2);
			x += 1;
			y -= 1;
		}
		vector_list[b - y].x1 = xc - x;
		vector_list[b - y].x2 = xc + x;
		vector_list[b + y].x1 = xc - x;
		vector_list[b + y].x2 = xc + x;
		y1 = y;
	}

	x1 = x;
	d2 = b2 * (x1 + 0.5) * (x1 + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
	while (y > 0) {
		if (d2 < 0) {
			d2 += b2 * (2 * x + 2) + a2 * (-2 * y + 3);
			x += 1;
			y -= 1;
		}
		else {
			d2 += a2 * (-2 * y + 3);
			y -= 1;
		}
		vector_list[b - y].x1 = xc - x;
		vector_list[b - y].x2 = xc + x;
		vector_list[b + y].x1 = xc - x;
		vector_list[b + y].x2 = xc + x;
		x1 = x;
	}
}

/*-----------------------------------------------------------------*/
typedef void (*HF)(port *, long, long, long, rect*, pattern *, long, long, short);

void _ellipse(port *a_port, pattern *p, rect *r, short mode, long pen_size, short flag)
{
		region	*clip;
		long	count;
		long	i;
		x_range	*vector_list;
		x_range	*inner_vector_list;
		long	y;
		long	screen_y;
		long	x1;
		long	x2;
		rect	clip_rect;
		long 	phase_x;
		long 	phase_y; 
		HF		horiz_pat_jmp;
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
		long	cnt;

		if ((r->left > r->right) || (r->top > r->bottom))
			return;

		tmpRect = *r;	

		tmpRect.top -= pen_size/2;
		tmpRect.left -= pen_size/2;
		tmpRect.bottom -= pen_size/2;
		tmpRect.right -= pen_size/2;
		
		tmpRect.left += a_port->origin.h;
		tmpRect.right += a_port->origin.h + (pen_size - 1);
		tmpRect.top += a_port->origin.v;
		tmpRect.bottom += a_port->origin.v + (pen_size - 1);

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

		horiz_pat_jmp = (HF)a_port->gr_procs[HORIZ_VECT_RTN_PAT];
		phase_x = 7 - (a_port->origin.h & 7);

		clip = a_port->draw_region;
		count = clip->count;
	
		cnt = (b + slop_y) * 2 + 4;
		vector_list = (x_range *)gr_malloc((cnt+4) * sizeof(x_range));
		for (y = 0; y <= (cnt); y++) {
			vector_list[y].x1 = 10000000;	/*center_x;*/ /* 100000000; */
			vector_list[y].x2 = -10000000;  /*center_x;*/ /* -1000000000; */
		}
		ellipse_calc(vector_list, center_x, b, a, b);

		if (flag) {
			cnt = (inner_b + slop_y) * 2 + 4;
			inner_vector_list = (x_range *)gr_malloc((cnt+4) * sizeof(x_range));
			for (y = 0; y <= (cnt); y++) {
				inner_vector_list[y].x1 = 10000000;  /*inner_center_x;*/ /* 100000000; */
				inner_vector_list[y].x2 = -10000000; /*inner_center_x;*/ /* -100000000; */
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

		get_screen();
		cursor_hide(tmpRect);
	
		phase_y = (/*screen_y + */a_port->origin.v) & 7;
	
		for (y = 0; y < pen_size; y++) {
			screen_y = y + tmpRect.top;

			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
		
			if (x2 >= x1)
				for (i = 0; i < count; i++) {
					clip_rect = ra(clip, i);
					(horiz_pat_jmp)
					(a_port, x1, x2, screen_y, &clip_rect, p, phase_x, phase_y, mode);
				}
		}

		if (pen_size < (b * 2) + slop_y) {
			for (; y <= ((inner_b * 2) + slop_y + pen_size); y++) {

				screen_y = y + tmpRect.top;
				x1 = vector_list[y].x1;
				x2 = vector_list[y].x2 + slop_x;

				if (flag) {
					inner_x1 = inner_vector_list[y - pen_size].x1 - 1;
					inner_x2 = inner_vector_list[y - pen_size].x2 + 1 + slop_x;
				}
	
				if (x2 >= x1)
					for (i = 0; i < count; i++) {
						clip_rect = ra(clip, i);
						if (flag) {
							(horiz_pat_jmp)(a_port, x1, inner_x1, screen_y, &clip_rect, p, phase_x, phase_y, mode);
							(horiz_pat_jmp)(a_port, inner_x2, x2, screen_y, &clip_rect, p, phase_x, phase_y, mode);
						}
						else
							(horiz_pat_jmp)(a_port, x1, x2, screen_y, &clip_rect, p, phase_x, phase_y, mode);
					}
			}
		}

		for (; y <= (b * 2) + slop_y; y++) {

			screen_y = y + tmpRect.top;
			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
	
			if (x2 >= x1)
				for (i = 0; i < count; i++) {
					clip_rect = ra(clip, i);
					(horiz_pat_jmp)(a_port, x1, x2, screen_y, &clip_rect, p, phase_x, phase_y, mode);
				}
		}

		release_screen();
	
		gr_free((char *)vector_list);
		if (flag)
			gr_free((char*)inner_vector_list);
}
