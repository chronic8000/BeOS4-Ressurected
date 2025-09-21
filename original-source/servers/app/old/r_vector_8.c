//******************************************************************************
//
//	File:		r_vector_8.c
//
//	Description:	8 bit renderer for vector.
//	Include	   :	rendering for rectangle.
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

#include "render_8.h"

//******************************************************************************/

void line_other_8(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	rgb_color	fore_color;
	rgb_color	old_color;
	server_color_map	*a_cs;
	short		sy;

	fore_color = a_port->fore_color;
	rowbyte = a_port->rowbyte;
	
	dx = x2 - x1;
	dy = y2 - y1;

	base = (a_port->base) + (y1 * rowbyte) + x1;

	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	a_cs = a_port->port_cs;

	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		

		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1)) {
				old_color = a_cs->color_list[*base];
				switch(mode) {
					case op_add :
					*base = calc_add_8(a_cs, old_color, fore_color);
					break;

					case op_sub :
					*base = calc_sub_8(a_cs, old_color, fore_color);
					break;

					case op_blend :
					*base = calc_blend_8(a_cs, old_color, fore_color);
					break;	

					case op_min :
					*base = calc_min_8(a_cs, old_color, fore_color);
					break;	

					case op_max :
					*base = calc_max_8(a_cs, old_color, fore_color);
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
				old_color = a_cs->color_list[*base];
				switch(mode) {
					case op_add :
					*base = calc_add_8(a_cs, old_color, fore_color);
					break;

					case op_sub :
					*base = calc_sub_8(a_cs, old_color, fore_color);
					break;

					case op_blend :
					*base = calc_blend_8(a_cs, old_color, fore_color);
					break;	

					case op_min :
					*base = calc_min_8(a_cs, old_color, fore_color);
					break;	

					case op_max :
					*base = calc_max_8(a_cs, old_color, fore_color);
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

void line_add_8  (port *a_port,
		  long x1,
		  long y1,
		  long x2,
		  long y2,
		  rect clip)
{
	line_other_8(a_port, x1, y1, x2, y2, clip, op_add);
}

/*--------------------------------------------------------------------*/ 

void line_sub_8  (port *a_port,
		  long x1,
		  long y1,
		  long x2,
		  long y2,
		  rect clip)
{
	line_other_8(a_port, x1, y1, x2, y2, clip, op_sub);
}

/*--------------------------------------------------------------------*/ 

void line_min_8  (port *a_port,
		  long x1,
		  long y1,
		  long x2,
		  long y2,
		  rect clip)
{
	line_other_8(a_port, x1, y1, x2, y2, clip, op_min);
}

/*--------------------------------------------------------------------*/ 

void line_max_8  (port *a_port,
		  long x1,
		  long y1,
		  long x2,
		  long y2,
		  rect clip)
{
	line_other_8(a_port, x1, y1, x2, y2, clip, op_max);
}

/*--------------------------------------------------------------------*/ 

void line_blend_8(port *a_port,
		  long x1,
		  long y1,
		  long x2,
		  long y2,
		  rect clip)
{
	line_other_8(a_port, x1, y1, x2, y2, clip, op_blend);
}

/*--------------------------------------------------------------------*/ 


void line_xor_8(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	uchar		*invert_map;
	short		sy;

	rowbyte = a_port->rowbyte;
	invert_map = a_port->port_cs->invert_map;
	
	dx = x2 - x1;
	dy = y2 - y1;

	base = (a_port->base) + (y1 * rowbyte) + x1;

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
				*base = invert_map[*base];
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
				*base = invert_map[*base];

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
				*base = invert_map[*base];
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
				*base = invert_map[*base];

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

void line_gen_8(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	short		sy;
	short 		lineType;
	uchar		color_byte;

	rowbyte = a_port->rowbyte;
	
	if (mode == op_copy || mode == op_or)
		color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	else
	if (mode == op_and) 
		color_byte = color_2_index(a_port->port_cs, a_port->back_color);

	dx = x2 - x1;
	dy = y2 - y1;
	if ((abs(dx)+abs(dy)) < 40)
		goto skip_hw;
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (mode == op_copy || mode == op_and || mode == op_or) {
			lineType = COPY_8;
			if (line_8_jmp) {
				(line_8_jmp)(x1, x2, y1, y2, color_byte,
							 1, clip.left, clip.top, clip.right, clip.bottom);
				(synchro_jmp)();
				return;
		}
		else
			goto skip_hw;
		}
	}
skip_hw:

	//dx = x2 - x1;
	//dy = y2 - y1;

	base = (a_port->base) + (y1 * rowbyte) + x1;

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
				*base++ = color_byte;

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
				*base = color_byte;

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
				*base = color_byte;
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
				*base = color_byte;

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

void line_or_8(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_8(a_port, x1, y1, x2, y2, clip, op_or);
}

/*--------------------------------------------------------------------*/ 

void line_copy_8(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_8(a_port, x1, y1, x2, y2, clip, op_copy);
}

/*--------------------------------------------------------------------*/ 

void line_and_8(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	line_gen_8(a_port, x1, y1, x2, y2, clip, op_and);
}

/*--------------------------------------------------------------------*/ 
		
void line_8(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	switch(mode) {
		case	op_copy :
			line_copy_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_or :
			line_or_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_and :
			line_and_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_xor :
			line_xor_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_add :
			line_add_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_sub :
			line_sub_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_blend :
			line_blend_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_min :
			line_min_8(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_max :
			line_max_8(a_port, x1, y1, x2, y2, clip);
			return;

		case	op_hilite :
			line_max_8(a_port, x1, y1, x2, y2, clip);
			return;
	}
}

