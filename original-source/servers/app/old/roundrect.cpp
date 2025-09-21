//******************************************************************************
//
//	File:			roundrect.cpp
//
//	Description:	1 bit round rectangle bitters.
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

#ifndef	RENDER_H
#include "render.h"
#endif

/*-----------------------------------------------------------------*/

typedef	struct {
		long	x1;
		long	x2;
		} x_range;

/*-----------------------------------------------------------------*/

extern void	ellipse_calc (x_range *, long xc, long yc, long a, long b);

/*-----------------------------------------------------------------*/

void DrawRoundRect(RenderContext *context, rect *r, long xRadius, long yRadius, bool fillFlag)
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
		rect	tmpRect, innerRect, rs[2];
		long	slop_x;
		long	slop_y;
		long	stretch_x;
		long	stretch_y;
		rect	cursor_rect;
		long	pen_size=context->scaledPenSize+0.5;
		FillRects renderRects;
		
		if ((r->left > r->right) || (r->top > r->bottom))
			return;

		renderRects = context->fFillRects;
		RenderCanvas *canvas = context->port->canvas;

		tmpRect.left = r->left;
		if (2*xRadius >= pen_size+1)
			tmpRect.right = tmpRect.left + (xRadius * 2);
		else
			tmpRect.right = tmpRect.left + pen_size+1;
		tmpRect.top = r->top;
		if (2*yRadius >= pen_size+1)
			tmpRect.bottom = tmpRect.top + (yRadius * 2);
		else
			tmpRect.bottom = tmpRect.top + pen_size+1;

		if ((tmpRect.right - tmpRect.left) > (r->right - r->left))
			tmpRect.right = tmpRect.left + (r->right - r->left);
		if ((tmpRect.bottom - tmpRect.top) > (r->bottom - r->top))
			tmpRect.bottom = tmpRect.top + (r->bottom - r->top);

		stretch_x = (r->right - r->left) - (tmpRect.right - tmpRect.left);
		stretch_y = (r->bottom - r->top) - (tmpRect.bottom - tmpRect.top);

		tmpRect.right += (pen_size - 1);
		tmpRect.bottom += (pen_size - 1);

		innerRect.left = tmpRect.left + pen_size;
		innerRect.top = tmpRect.top + pen_size;
		innerRect.right = tmpRect.right - pen_size;
		innerRect.bottom = tmpRect.bottom - pen_size;

		if (innerRect.left > innerRect.right) {
			innerRect.left = 0;
			innerRect.right = 0;
			fillFlag = FALSE;
		}
		if (innerRect.top > innerRect.bottom) {
			innerRect.top = 0;
			innerRect.bottom = 0;
			fillFlag = FALSE;
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
		slop_x += stretch_x;
		slop_y = (tmpRect.bottom - tmpRect.top) & 1;
		slop_y += stretch_y;

		phase_x = context->phase_x;
		clip = context->drawRegion;
	
		vector_list = (x_range *)gr_malloc((((b + slop_y) * 2) + 4) * sizeof(x_range));

		for (y = 0; y <= ((b + slop_y) * 2); y++) {
			vector_list[y].x1 = center_x; /* 100000000; */
			vector_list[y].x2 = center_x; /* -1000000000; */
		}
		ellipse_calc(vector_list, center_x, b, a, b);

		if (fillFlag) {
			inner_vector_list = (x_range *)gr_malloc((((inner_b + slop_y) * 2) + 4) * sizeof(x_range));
			for (y = 0; y <= ((inner_b + slop_y) * 2); y++) {
				inner_vector_list[y].x1 = inner_center_x; /* 100000000; */
				inner_vector_list[y].x2 = inner_center_x; /* -100000000; */
			}
			ellipse_calc(inner_vector_list, inner_center_x, inner_b, inner_a, inner_b);
		}

		if (slop_y) {
			for (y = (b * 2) ; y >= b; y--)
				vector_list[y + slop_y] = vector_list[y];
			for (y = b + 1; y < b + slop_y; y++)
				vector_list[y] = vector_list[b];
			if (fillFlag) {	
				for (y = (inner_b * 2); y >= inner_b; y--)
					inner_vector_list[y + slop_y] = inner_vector_list[y];
				for (y = inner_b + 1; y < inner_b + slop_y; y++)
					inner_vector_list[y] = inner_vector_list[inner_b];
			}
		}

		set_rect(cursor_rect,tmpRect.top, tmpRect.left, tmpRect.bottom + stretch_y, tmpRect.right + stretch_x);
		grLockCanvas(canvas,&cursor_rect);
		phase_y = context->phase_y;
	
		for (y = 0; y < pen_size; y++) {
			screen_y = y + tmpRect.top;

			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
		
			if (x2 >= x1) {
				rs[0].top = rs[0].bottom = screen_y;
				rs[0].left = x1;
				rs[0].right = x2;
				renderRects(context,canvas,rs,1,clip);
			};
		}

		if (pen_size < (b * 2) + slop_y) {
			for (; y <= ((inner_b * 2) + slop_y + pen_size); y++) {

				screen_y = y + tmpRect.top;
				x1 = vector_list[y].x1;
				x2 = vector_list[y].x2 + slop_x;

				if (fillFlag) {
					inner_x1 = inner_vector_list[y - pen_size].x1 - 1;
					inner_x2 = inner_vector_list[y - pen_size].x2 + 1 + slop_x;
				}
	
				if (x2 >= x1) {
					if (fillFlag) {
						rs[0].top = rs[0].bottom = 
						rs[1].top = rs[1].bottom = screen_y;
						rs[0].left = x1;
						rs[0].right = inner_x1;
						rs[1].left = inner_x2;
						rs[1].right = x2;
						renderRects(context,canvas,rs,2,clip);
					}
					else {
						rs[0].top = rs[0].bottom = screen_y;
						rs[0].left = x1;
						rs[0].right = x2;
						renderRects(context,canvas,rs,1,clip);
					}
				}
			}
		}

		for (; y <= (b * 2) + slop_y; y++) {

			screen_y = y + tmpRect.top;
			x1 = vector_list[y].x1;
			x2 = vector_list[y].x2 + slop_x;
		
			if (x2 >= x1) {
				rs[0].top = rs[0].bottom = screen_y;
				rs[0].left = x1;
				rs[0].right = x2;
				renderRects(context,canvas,rs,1,clip);
			};
		}

		grUnlockCanvas(canvas);
	
		gr_free((char *)vector_list);
		if (fillFlag)
			gr_free((char*)inner_vector_list);
}
