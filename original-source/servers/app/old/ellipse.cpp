//******************************************************************************
//
//	File:			ellipse.cpp
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

#ifndef	RENDER_H
#include "render.h"
#endif

#include <Debug.h>

/*-----------------------------------------------------------------*/

typedef	struct {
		long	x1;
		long	x2;
		} x_range;

#define POWERPC 1

/*-----------------------------------------------------------------*/

float b_sqrt(float x) {
	unsigned long	val;
	float			y,z,t;
	float	        flottant, tampon;
	
	flottant = x;
	val = *((unsigned long*)&flottant);
	val >>= 1;
	val += 0x1FC00000L;
	*((unsigned long*)&tampon) = val;
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= (float)4.0;
	x = z*z;
	t = t*t;
	y = z*y;
	t = (float)2.0*x-t;
	return t/y;
}

void StrokeEllipse(RenderContext *context, fpoint center, fpoint radius)
{
	float x,y,xa1,xa2,xb1,xb2;
	float c = radius.h/b_sqrt(radius.v);
	
	y = floor(center.v + radius.v + 0.5) - 0.5;
	if (y < center.v) {
		/* Degenerate ellipse; just a point at the center */
		return;
	};
	x = b_sqrt(y) * c;
	xa1 = floor(center.h - x + 1.0);
	xb2 = floor(center.h + x);
	/* Draw xa1-xb2,y */
	
}


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

void DrawEllipse(RenderContext *context, rect *r, short flag)
{
		region	*clip;
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
		rect	tmpRect,innerRect,r1;
		long	slop_x;
		long	slop_y;
		long	cnt;
		FillRects renderRects;
		long	pen_size = context->scaledPenSize+0.5;

		if ((r->left > r->right) || (r->top > r->bottom))
			return;

		tmpRect = *r;	
		renderRects = context->fFillRects;
		RenderCanvas *canvas = context->port->canvas;

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

		phase_x = context->phase_x;
		clip = context->drawRegion;
	
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

		grLockCanvas(canvas,&tmpRect);

		phase_y = context->phase_y;
	
		for (y = 0; y < pen_size; y++) {
			screen_y = y + tmpRect.top;

			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
		
			if (x2 >= x1) {
				r1.left = x1;
				r1.right = x2;
				r1.top = r1.bottom = screen_y;
				renderRects(context,canvas,&r1,1,clip);
			};
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
	
				if (x2 >= x1) {
					if (flag) {
						rect rs[2];
						rs[0].top = rs[0].bottom = 
						rs[1].top = rs[1].bottom = screen_y;
						rs[0].left = x1;
						rs[0].right = inner_x1;
						rs[1].left = inner_x2;
						rs[1].right = x2;
						renderRects(context,canvas,rs,2,clip);
					} else {
						r1.left = x1;
						r1.right = x2;
						r1.top = r1.bottom = screen_y;
						renderRects(context,canvas,&r1,1,clip);
					};
				}
			}
		}

		for (; y <= (b * 2) + slop_y; y++) {

			screen_y = y + tmpRect.top;
			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
	
			if (x2 >= x1) {
				r1.left = x1;
				r1.right = x2;
				r1.top = r1.bottom = screen_y;
				renderRects(context,canvas,&r1,1,clip);
			};
		}

		grUnlockCanvas(canvas);

		gr_free((char *)vector_list);
		if (flag) gr_free((char*)inner_vector_list);
}
