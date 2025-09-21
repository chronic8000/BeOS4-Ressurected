//******************************************************************************
//
//	File:		r_vector_24.c
//
//	Description:	24 bit renderer for vector.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			new oct 30 1992 bgs
//
//******************************************************************************/
  
#include <stdio.h>
#include <vga.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef VGA_H
#include "vga1.h"
#endif

#include "render_24.h"

void line_other_24(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	ulong		*base;
	long		dx,dy,error,cpt;
	long		rowbyte;
	ulong		fore_color,old_color;
	short		sy;

	fore_color = fcolor_2_long(a_port, a_port->fore_color);
	rowbyte = a_port->rowbyte>>2;
	
	dx = x2 - x1;
	dy = y2 - y1;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;

		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1)) {
				old_color = *base;
				switch(mode) {
					case op_add :
					*base = calc_add_24(old_color, fore_color);
					break;

					case op_sub :
					*base = calc_sub_24(old_color, fore_color);
					break;

					case op_blend :
					*base = calc_blend_24(old_color, fore_color);
					break;	

					case op_min :
					*base = calc_min_24(old_color, fore_color);
					break;	

					case op_max :
					*base = calc_max_24(old_color, fore_color);
					break;	
					case op_hilite :
					break;
				}	
			}
			else {
				if (x1 > clip.right)
					return;
			}

			base++;
			x1++;

			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1)) {
				old_color = *base;
				switch(mode) {
					case op_add :
					*base = calc_add_24(old_color, fore_color);
					break;

					case op_sub :
					*base = calc_sub_24(old_color, fore_color);
					break;

					case op_blend :
					*base = calc_blend_24(old_color, fore_color);
					break;	

					case op_min :
					*base = calc_min_24(old_color, fore_color);
					break;	

					case op_max :
					*base = calc_max_24(old_color, fore_color);
					break;	

					case op_hilite :
					break;
				}	
			}

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				base++;
				x1++;

				if (x1 > clip.right)
					return;
				error -= dy;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void line_add_24  (	port *a_port,
		  			long x1,
		  			long y1,
		  			long x2,
		  			long y2,
		  			rect clip)
{
	line_other_24(a_port, x1, y1, x2, y2, clip, op_add);
}

/*--------------------------------------------------------------------*/ 

void line_sub_24  (	port *a_port,
		  			long x1,
		  			long y1,
		  			long x2,
		  			long y2,
		  			rect clip)
{
	line_other_24(a_port, x1, y1, x2, y2, clip, op_sub);
}

/*--------------------------------------------------------------------*/ 

void line_min_24  (	port *a_port,
		  			long x1,
		  			long y1,
		  			long x2,
		  			long y2,
		  			rect clip)
{
	line_other_24(a_port, x1, y1, x2, y2, clip, op_min);
}

/*--------------------------------------------------------------------*/ 

void line_max_24  (	port *a_port,
		  			long x1,
		  			long y1,
		  			long x2,
		  			long y2,
		  			rect clip)
{
	line_other_24(a_port, x1, y1, x2, y2, clip, op_max);
}

/*--------------------------------------------------------------------*/ 

void line_blend_24(	  port *a_port,
					  long x1,
					  long y1,
					  long x2,
					  long y2,
					  rect clip)
{
	line_other_24(a_port, x1, y1, x2, y2, clip, op_blend);
}

/*--------------------------------------------------------------------*/ 

void line_xor_24(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	ulong		*base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	ulong		invertmask;
	short		sy;

	rowbyte = a_port->rowbyte>>2;
	invertmask = -1L;
	
	dx = x2 - x1;
	dy = y2 - y1;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (point_in_rect(&clip, x1, y1) && point_in_rect(&clip, x2, y2)) {
		if (dx > dy) {
			error = dx >> 1;

			cpt = x2 - x1;
			while (cpt >= 0) {
				cpt--;
				*base = invertmask+*base;
				base++;

				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		} 
		else {
			error = dy >> 1;
		
			cpt = dy;
			while (cpt >= 0) {
				cpt--;
				*base = invertmask+*base;

				base += rowbyte;
				error += dx;

				if (error >= dy) {
					base++;
					error -= dy;
				}
			}
		}
		return;
	}
	
	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base = invertmask+*base;
			else {
				if (x1 > clip.right)
					return;
			}

			base++;
			x1++;

			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base = invertmask+*base;

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				base++;
				x1++;

				if (x1 > clip.right)
					return;
				error -= dy;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void line_gen_24(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	ulong		*base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	short		sy;
	ulong		color_long;
	long		lineType;

	rowbyte = a_port->rowbyte>>2;
	
	if (mode == op_copy || mode == op_or)
		color_long = fcolor_2_long(a_port, a_port->fore_color);
	else
	if (mode == op_and) 
		color_long = fcolor_2_long(a_port, a_port->back_color);

	dx = x2 - x1;
	dy = y2 - y1;

	if ((abs(dx)+abs(dy)) < 40)
		goto skip_hw;

	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (mode == op_copy || mode == op_or || mode == op_and) {
			lineType = COPY_24;
			if (line_24_jmp) {
				(line_24_jmp)(x1, x2, y1, y2, color_long,
							  1, clip.left, clip.top, clip.right, clip.bottom);
				(synchro_jmp)();
				return;
			}
		}
	}
skip_hw:
	
	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (point_in_rect(&clip, x1, y1) && point_in_rect(&clip, x2, y2)) {
		if (dx > dy) {
			error = dx >> 1;

			cpt = x2 - x1;
			while (cpt >= 0) {
				cpt--;
				*base++ = color_long;

				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		} 
		else {
			error = dy >> 1;
		
			cpt = dy;
			while (cpt >= 0) {
				cpt--;
				*base = color_long;

				base += rowbyte;
				error += dx;

				if (error >= dy) {
					base++;
					error -= dy;
				}
			}
		}
		return;
	}
	
	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base = color_long;
			else {
				if (x1 > clip.right)
					return;
			}

			base++;
			x1++;

			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base = color_long;

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				base++;
				x1++;

				if (x1 > clip.right)
					return;
				error -= dy;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void line_or_24(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_24(a_port, x1, y1, x2, y2, clip, op_or);
}

/*--------------------------------------------------------------------*/ 

void line_copy_24(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_24(a_port, x1, y1, x2, y2, clip, op_copy);
}

/*--------------------------------------------------------------------*/ 

void line_and_24(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_24(a_port, x1, y1, x2, y2, clip, op_and);
}

/*--------------------------------------------------------------------*/ 

void line_24(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	switch(mode) {
		case	op_copy :
			line_copy_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_or :
			line_or_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_and :
			line_and_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_xor :
			line_xor_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_add :
			line_add_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_sub :
			line_sub_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_blend :
			line_blend_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_min :
			line_min_24(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_max :
			line_max_24(a_port, x1, y1, x2, y2, clip);
			return;
		case op_hilite :
			return;
	}
}

