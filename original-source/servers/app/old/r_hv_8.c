//******************************************************************************
//
//	File:		render.c
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
#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))
/*--------------------------------------------------------------------*/

void horizontal_line_xor_pattern_8(	port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	tmp;
	uchar	*invert_map;
	uchar	umask;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/

	invert_map = a_port->port_cs->invert_map;
	
	x2++;

/* END CLIPPING CODE HERE */

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	umask = 0x80;

	if (i <= 7) {
		while (i > 0) {
			i--;
			if (pb & umask)
				*base = invert_map[*base];
			base++;

			umask >>= 1;
		}
	}
	else
	while (i > 0) {
		i--;
		if (pb & umask)
			*base = invert_map[*base];
		base++;

		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}
}

/*--------------------------------------------------------------------*/

void horizontal_line_or_pattern_8(	port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	j;
	long	tmp;
	uchar	fore_color_byte;
	uchar	umask;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	
	
	x2++;

/* END CLIPPING CODE HERE */

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;


	umask = 0x80;

	while (i > 7) {
		j = 4;
		while (j--) {
			if (pb & umask)
				*base = fore_color_byte;
			base++;
			umask >>= 1;

			if (pb & umask)
				*base = fore_color_byte;
			base++;
			umask >>= 1;
		}
		i -= 8;
		umask = 0x80;
	}

	while (i > 0) {
		i--;
		if (pb & umask)
			*base = fore_color_byte;
		base++;
		umask >>= 1;
		/*
		if (umask == 0)
			umask = 0x80;
		*/
	}
}


/*--------------------------------------------------------------------*/

void horizontal_line_copy_pattern_8(port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	tmp;
	uchar	fore_color_byte;
	uchar	back_color_byte;
	uchar	exp_pat[8];
	uchar	umask;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);

	x2++;

/* END CLIPPING CODE HERE */

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;

	umask = 0x80;

	while ((i > 0) && (((long)base & 0x3) != 0)) {
		i--;
		if (pb & umask)
			*base++ = fore_color_byte;
		else
			*base++ = back_color_byte;
		umask >>= 1;
	}

	for (tmp = 0; tmp < 8; tmp++) {
		exp_pat[tmp] = (pb & umask) ? fore_color_byte : back_color_byte;
		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}

	while (i >= 8) {
		write_long(base, as_long(exp_pat[0]));
		base += 4;
		write_long(base, as_long(exp_pat[4]));
		base += 4;
		i -= 8;
	}

	if (i >= 4) {
		write_long(base, as_long(exp_pat[0]));
		base += 4;
		i -= 4;
		tmp = as_long(exp_pat[0]);
		as_long(exp_pat[0]) = as_long(exp_pat[4]);
		as_long(exp_pat[4]) = tmp;
	}

	tmp = 0;

	while (i > 0) {
		i--;
		*base++ = exp_pat[tmp];
		tmp++;
	}
}

/*--------------------------------------------------------------------*/

void horizontal_line_and_pattern_8 (port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	tmp;
	uchar	fore_color_byte;
	uchar	back_color_byte;
	uchar	umask;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);
	
	x2++;

/* END CLIPPING CODE HERE */

	phase_x += x1;
	phase_x &= 7;

	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;


	umask = 0x80;

	while (i > 0) {
		i--;
		if (pb & umask)
			*base = back_color_byte;
		base++;

		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}
}

/*--------------------------------------------------------------------*/
static pattern	data_black={0xff, 0xff, 0xff, 0xff,
								0xff, 0xff, 0xff, 0xff};
/*--------------------------------------------------------------------*/

void horizontal_line_and_8( port *a_port,
			   				long x1,
			   				long x2,
			   				long y1,
			   				rect *cliprect)
{
	horizontal_line_and_pattern_8 (a_port, x1, x2, y1, cliprect, &data_black, 0, 0);
}
			   
/*--------------------------------------------------------------------*/

void vertical_line_copy_pattern_8(	port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;
	uchar	fore_color_byte;
	uchar	back_color_byte;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);
	
	base = a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	phase_x &= 7;
	
	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base = fore_color_byte;
		else
			*base = back_color_byte;

		base += rowbyte;
		y1++;
	}
}


/*--------------------------------------------------------------------*/ 

void vertical_line_xor_8(port *a_port,
			        	long x1,
			        	long y1,
			        	long y2,
			        	rect *cliprect)
{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	*invert_map;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	invert_map = a_port->port_cs->invert_map;
	
	base = a_port->base + (y1 * rowbyte) + x1;

	while (y1 <= y2) {
		*base = invert_map[*base];
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_and_8(port *a_port,
			        	 long x1,
			        	 long y1,
			        	 long y2,
			        	 rect *cliprect)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	back_color_byte;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);

	base = a_port->base + (y1 * rowbyte) + x1;

	while (y1 <= y2) {
		*base = back_color_byte;
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_copy_8  (port *a_port,
			        		long x1,
			        		long y1,
			        		long y2,
			        		rect *cliprect)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	fore_color_byte;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	
#ifdef	USE_HARDWARE
	if ((y2-y1) > 60)		//has to be measured to be optimal !!!
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (line_8_jmp) {
			(line_8_jmp)(x1,x1,y1,y2,fore_color_byte,0,0,0,0,0);
			(synchro_jmp)();
			return;
		}
	}

#endif

	base = a_port->base + (y1 * rowbyte) + x1;

	while ((y2 - y1) > 3) {
		*base = fore_color_byte;
		base += rowbyte;
		*base = fore_color_byte;
		base += rowbyte;
		*base = fore_color_byte;
		base += rowbyte;
		y1 += 3;
	}

	while (y1 <= y2) {
		*base = fore_color_byte;
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 
/*--------------------------------------------------------------------*/ 

void horizontal_line_copy_8(port *a_port,
				    		long x1,
				    		long x2,
				    		long y1,
				    		rect *cliprect)
{
	uchar	*base;		
	long	rowbyte;	
	long	long_fore;
	long	i;
	long	tmp;
	uchar	fore_color_byte;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;

	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


/* END CLIPPING CODE HERE */
	
	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);

	i = x2 - x1;

#ifdef	USE_HARDWARE
	
	if (i > 80)			//has to be measured to be optimal !!!
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (line_8_jmp) {
			(line_8_jmp)(x1,x2,y1,y1,fore_color_byte,0,0,0,0,0);
			(synchro_jmp)();
			return;
		}
	}
#endif

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;


	if (i > 7) {
		long_fore = (fore_color_byte << 24) |
			    (fore_color_byte << 16) |
			    (fore_color_byte << 8)  |
			    fore_color_byte;	

		while ((i >= 0) && (((long)base & 0x3) != 0)) {
			i--;
			*base++ = fore_color_byte;
		}

		while (i >= 8) {
			write_long(base, long_fore);
			base += 4;
			write_long(base, long_fore);
			base += 4;
			i -= 8;
		}

		if (i >= 4) {
			write_long(base, long_fore);
			base += 4;
			i -= 4;
		}
	}

	while (i >= 0) {
		i--;
		*base++ = fore_color_byte;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_other_8(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect,
				 			short mode)
{
	uchar		*base;	
	long		rowbyte;	
	long		i;
	long		tmp;
	rgb_color	tmp_color;
	rgb_color	back_color;
	rgb_color	old_color;
	server_color_map	*a_cs;
	ushort		tmp_b;
	ushort		tmp_b0;
	uchar		out;

	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/
	
	x2++;

	a_cs = a_port->port_cs;
	
	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;
	tmp_color = a_port->fore_color;
	back_color = a_port->back_color;

	tmp_b0 = 257;

	switch(mode) {
		case op_add:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_add_8(a_cs, old_color, tmp_color);
				}
				*base++ = out;
			}
			return;
		case op_sub:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_sub_8(a_cs, old_color, tmp_color);
				}
				*base++ = out;
			}
			return;
		case op_min:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_min_8(a_cs, old_color, tmp_color);
				}
				*base++ = out;
			}
			return;
		case op_max:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_max_8(a_cs, old_color, tmp_color);
				}
				*base++ = out;
			}
			return;
		case op_hilite:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_hilite_8(a_cs, tmp_b, tmp_color, back_color);
				}
				*base++ = out;
			}
			return;
		case op_blend:
			while (i > 0) {
				i--;
				tmp_b = *base;
				if (tmp_b != tmp_b0) {
					tmp_b0 = tmp_b;
					old_color = a_cs->color_list[tmp_b];
					out = calc_blend_8(a_cs, old_color, tmp_color);
				}
				*base++ = out;
			}
			return;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_add_8	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_add);
}
/*--------------------------------------------------------------------*/ 

void horizontal_line_sub_8	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_sub);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_min_8	(port *a_port,
				 long x1,
				 long x2,
				 long y1,
				 rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_min);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_max_8	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_max);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_hilite_8	(port *a_port,
				 				 long x1,
				 				 long x2,
				 				 long y1,
				 				 rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_max);
}


/*--------------------------------------------------------------------*/ 

void horizontal_line_blend_8	(port *a_port,
				 long x1,
				 long x2,
				 long y1,
				 rect *cliprect)
{
	horizontal_line_other_8(a_port, x1, x2, y1, cliprect, op_blend);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_xor_8	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	uchar	*base;	
	uchar	*invert_map;	
	long	rowbyte;	
	long	i;
	long	tmp;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}


/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


/* END CLIPPING CODE HERE */
	
	x2++;

	invert_map = a_port->port_cs->invert_map;
	
	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;

	while (i > 0) {
		i--;
		*base = invert_map[*base];
		base++;
	}
}


void vertical_line_xor_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)

{
	uchar	*base;
	uchar	*invert_map;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	pb;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	
	invert_map = a_port->port_cs->invert_map;

	base = a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));

	phase_x &= 7;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base = invert_map[*base];

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_or_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;
	uchar	fore_color_byte;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	
	base = a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	phase_x &= 7;
	
	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base = fore_color_byte;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_and_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;
	uchar	back_color_byte;

	rowbyte = a_port->rowbyte;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);
	
	base = a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	phase_x &= 7;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);

		pb &= mask;

		if (pb)
			*base = back_color_byte;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_other_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y, 
				    short mode)
{
	uchar		*base;		
	long		rowbyte;	
	ulong		pb;	
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	tmp_color;
	rgb_color	old_color;
	rgb_color	exp_pat[8];
	long		i;
	long		tmp;
	server_color_map	*a_cs;
	uchar		umask;


	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (y1 < cliprect->top)
		return;
	if (y1 > cliprect->bottom)
		return;

	if (x1 < cliprect->left)
		x1 = cliprect->left;
	
	if (x2 > cliprect->right)
		x2 = cliprect->right;
			
	if (x1 > x2) 
		return;		/*no intersection*/


	fore_color = a_port->fore_color;
	back_color = a_port->back_color;
	
	
	x2++;

/* END CLIPPING CODE HERE */

	rowbyte = a_port->rowbyte;

	base = a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	umask = 0x80;

	for (tmp = 0; tmp < 8; tmp++) {
		exp_pat[tmp] = (pb & umask) ? fore_color : back_color;
		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}

	tmp = 0;

	a_cs = a_port->port_cs;

	while (i > 0) {
		i--;
		tmp_color = exp_pat[tmp & 7];
		old_color = a_cs->color_list[*base];
		switch(mode) {
			case op_add :
			*base = calc_add_8(a_cs, old_color, tmp_color);
			break;

			case op_sub :
			*base = calc_sub_8(a_cs, old_color, tmp_color);
			break;

			case op_blend :
			*base = calc_blend_8(a_cs, old_color, tmp_color);
			break;	

			case op_min :
			*base = calc_min_8(a_cs, old_color, tmp_color);
			break;	

			case op_max :
			*base = calc_max_8(a_cs, old_color, tmp_color);
			break;	
	
			case op_hilite :
			break;
		}	
		tmp++;
		base++;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_add_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_8(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_add);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_sub_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_8(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_sub);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_min_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_8(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_min);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_max_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_8(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_max);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_blend_pattern_8(	port *a_port,
					    long x1,
					    long x2,
					    long y1,
					    rect *cliprect,
					    pattern *the_pattern,
					    long phase_x,
					    long phase_y)
{
	horizontal_line_other_pattern_8(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_blend);
} 


void vertical_line_add_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)
{
	rect	r;

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2) 
		return;		/*no intersection*/

	r.left = x1;
	r.right = x1;
	if (y1 < y2) {
		r.top = y1;
		r.bottom = y2;
	}
	else {
		r.top = y2;
		r.bottom = y1;
	}
	
	rect_fill_other_8(a_port, &r, the_pattern, phase_x, phase_y, op_add);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_sub_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)
{
	rect	r;

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2) 
		return;		/*no intersection*/

	r.left = x1;
	r.right = x1;
	if (y1 < y2) {
		r.top = y1;
		r.bottom = y2;
	}
	else {
		r.top = y2;
		r.bottom = y1;
	}
	
	rect_fill_other_8(a_port, &r, the_pattern, phase_x, phase_y, op_sub);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_min_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)
{
	rect	r;

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2) 
		return;		/*no intersection*/

	r.left = x1;
	r.right = x1;
	if (y1 < y2) {
		r.top = y1;
		r.bottom = y2;
	}
	else {
		r.top = y2;
		r.bottom = y1;
	}
	
	rect_fill_other_8(a_port, &r, the_pattern, phase_x, phase_y, op_min);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_max_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)
{
	rect	r;

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2) 
		return;		/*no intersection*/

	r.left = x1;
	r.right = x1;
	if (y1 < y2) {
		r.top = y1;
		r.bottom = y2;
	}
	else {
		r.top = y2;
		r.bottom = y1;
	}
	
	rect_fill_other_8(a_port, &r, the_pattern, phase_x, phase_y, op_max);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_blend_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y)
{
	rect	r;

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2) 
		return;		/*no intersection*/

	r.left = x1;
	r.right = x1;
	if (y1 < y2) {
		r.top = y1;
		r.bottom = y2;
	}
	else {
		r.top = y2;
		r.bottom = y1;
	}
	
	rect_fill_other_8(a_port, &r, the_pattern, phase_x, phase_y, op_blend);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_other_8      (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
				short mode)

{
	uchar		*base;
	long		rowbyte;
	long		tmp;
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	old_color;
	server_color_map	*a_cs;

	rowbyte = a_port->rowbyte;
	fore_color = a_port->fore_color;
	back_color = a_port->back_color;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

/* CLIPPING CODE HERE	*/

	if (x1 < cliprect->left)
		return;
	if (x1 > cliprect->right)
		return;

	if (y1 < cliprect->top)
		y1 = cliprect->top;
	
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
			
	if (y1 > y2)
		return;		/*no intersection*/

/* END CLIPPING CODE HERE */

	base = a_port->base + (y1 * rowbyte) + x1;

	a_cs = a_port->port_cs;

	while (y1 <= y2) {
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
			*base = calc_hilite_8(a_cs, *base, fore_color, back_color);
			break;
		}	
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_add_8      (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_add);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_hilite_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_hilite);
}


/*--------------------------------------------------------------------*/ 

void vertical_line_sub_8      (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_sub);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_min_8      (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_min);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_max_8      (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_max);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_blend_8     (port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect)
{
	vertical_line_other_8(a_port, x1, y1, y2, cliprect, op_blend);
}

/*--------------------------------------------------------------------*/ 

void horiz_line_8(port *theport, long x1, long x2, long y2, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or:
				horizontal_line_copy_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_and:
				horizontal_line_and_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_max :
				horizontal_line_max_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_hilite :
				horizontal_line_hilite_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_add :
				horizontal_line_add_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_sub :
				horizontal_line_sub_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_min :
				horizontal_line_min_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_blend :
				horizontal_line_blend_8(theport, x1, x2, y2, cliprect);
				return;
		case	op_xor :
				horizontal_line_xor_8(theport, x1, x2, y2, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void vert_line_8(port *theport, long x1, long y1, long y2, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or:
				vertical_line_copy_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_max :
				vertical_line_max_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_hilite :
				vertical_line_hilite_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_add :
				vertical_line_add_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_and :
				vertical_line_and_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_sub :
				vertical_line_sub_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_min :
				vertical_line_min_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_blend :
				vertical_line_blend_8(theport, x1, y1, y2, cliprect);
				return;
		case	op_xor :
				vertical_line_xor_8(theport, x1, y1, y2, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void vert_line_pattern_8(port *theport,
						  long x1,
						  long y1,
						  long y2,
						  rect *cliprect,
						  pattern *p,
						  long phase_x,
						  long phase_y,
						  short mode)
{
	switch(mode) {
		case	op_copy:
				vertical_line_copy_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
		case	op_or:
				vertical_line_or_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_max :
				vertical_line_max_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_hilite :
				//vertical_line_max_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_add :
				vertical_line_add_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_and :
				vertical_line_and_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
				vertical_line_sub_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_min :
				vertical_line_min_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
				vertical_line_blend_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_xor :
				vertical_line_xor_pattern_8(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void horiz_line_pattern_8(port *theport,
						  long x1,
						  long x2,
						  long y1,
						  rect *cliprect,
						  pattern *p,
						  long phase_x,
						  long phase_y,
						  short mode)
{
	switch(mode) {
		case	op_copy:
				horizontal_line_copy_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
		case	op_or:
				horizontal_line_or_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_max :
				horizontal_line_max_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_hilite :
				return;
		case	op_add :
				horizontal_line_add_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_and :
				horizontal_line_and_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
				horizontal_line_sub_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_min :
				horizontal_line_min_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
				horizontal_line_blend_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_xor :
				horizontal_line_xor_pattern_8(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*--------------------------------------------------------------------*/

void run_list_8_xor(port	    *a_port,
					raster_line *list,
					long        list_cnt,
					rect	    *cliprect,
					char		single_pixels)
{
	long    	x1,x2,y1;
	uchar		*base;		
	long		rowbyte;	
	long		i,cnt;
	long		tmp;
	char    	invertmask;
	rgb_color	fore_color;
	server_color_map	*a_cs;
	long		top,left,bottom,right;


	a_cs = a_port->port_cs;

	rowbyte = a_port->rowbyte;

	invertmask = -1;

	top = cliprect->top;
	left = cliprect->left;
	bottom = cliprect->bottom;
	right = cliprect->right;

	if (single_pixels) {
		base = (uchar *)a_port->base;
		for (cnt=0;cnt<list_cnt;cnt++) {
			x1 = list->x0;
			y1 = list->y;

			list++;
			
			if ((y1 < top)    ||
			    (y1 > bottom) ||
			    (x1 < left)	  ||
				(x1 > right))
			    continue;

			*(base + (y1 * rowbyte) + x1) ^= invertmask;
		}
	}
	else
	for (cnt=0;cnt<list_cnt;cnt++) {
		x1 = list->x0;
		x2 = list->x1;
		y1 = list->y;

		list++;
		
		if (y1 < cliprect->top)
		    continue;
		if (y1 > cliprect->bottom)
			continue;

		if (x2 < x1) {
			tmp = x2;
			x2 = x1;
			x1 = tmp;
		}

		if (x1 < cliprect->left)
			x1 = cliprect->left;
	
		if (x2 > cliprect->right)
			x2 = cliprect->right;
			
		if (x1 > x2) 
			continue;		/*no intersection*/

		base = (uchar*)a_port->base + (y1 * rowbyte) + x1;
		
		for (i=x2-x1;i>=0;i--)
			*base = invertmask^(*base);
	}
}

/*--------------------------------------------------------------------*/

void run_list_8_copy(port	    *a_port,
					raster_line *list,
					long        list_cnt,
					rect	    *cliprect,
					char		single_pixels)
{
	long    	x1,x2,y1;
	uchar		*base;		
	long		rowbyte;	
	long		i,cnt;
	long		tmp;
	ulong		fore_color_byte;
	rgb_color	fore_color;
	server_color_map	*a_cs;


	a_cs = a_port->port_cs;
	fore_color = a_port->fore_color;
   	fore_color_byte = color_2_index(a_cs, fore_color);	

	rowbyte = a_port->rowbyte;

	for (cnt=0;cnt<list_cnt;cnt++) {
		x1 = list->x0;
		x2 = list->x1;
		y1 = list->y;

		list++;
		
		if (y1 < cliprect->top)
		    continue;
		if (y1 > cliprect->bottom)
			continue;

		if (x2 < x1) {
			tmp = x2;
			x2 = x1;
			x1 = tmp;
		}

		if (x1 < cliprect->left)
			x1 = cliprect->left;
	
		if (x2 > cliprect->right)
			x2 = cliprect->right;
			
		if (x1 > x2) 
			continue;		/*no intersection*/

		base = (uchar*)a_port->base + (y1 * rowbyte) + x1;
		
		for (i=x2-x1;i>=0;i--)
			*base++ = fore_color_byte;
	}
}

/*--------------------------------------------------------------------*/

void run_list_8(port	    *a_port,
				raster_line *list,
				long        list_cnt,
				rect	    *cliprect,
				short       mode,
				char		single_pixels)
{
	long    x1,x2,y1;
	uchar	*base;		
	long	rowbyte;	
	long	i,cnt;
	long	tmp;
	ulong	fore_color_byte,back_color_byte;
	char    invertmask;
	rgb_color old_color,fore_color,back_color;
	server_color_map	*a_cs;


	switch(mode) {
		case op_copy:
		case op_or:
			run_list_8_copy(a_port, list, list_cnt, cliprect, single_pixels);
			return;
		case op_xor:
			run_list_8_xor(a_port, list, list_cnt, cliprect, single_pixels);
			return;
	}

	a_cs = a_port->port_cs;
	fore_color = a_port->fore_color;
	back_color = a_port->back_color;
   	fore_color_byte = color_2_index(a_cs, fore_color);	
	back_color_byte = color_2_index(a_cs, back_color);

	rowbyte = a_port->rowbyte;

	for (cnt=0;cnt<list_cnt;cnt++) {
		x1 = list->x0;
		x2 = list->x1;
		y1 = list->y;

		list++;
		
		if (y1 < cliprect->top)
		    continue;
		if (y1 > cliprect->bottom)
			continue;

		if (x2 < x1) {
			tmp = x2;
			x2 = x1;
			x1 = tmp;
		}

		if (x1 < cliprect->left)
			x1 = cliprect->left;
	
		if (x2 > cliprect->right)
			x2 = cliprect->right;
			
		if (x1 > x2) 
			continue;		/*no intersection*/

		base = (uchar*)a_port->base + (y1 * rowbyte) + x1;
		
		switch (mode) {
		case	op_copy:
			for (i=x2-x1;i>=0;i--)
				*base++ = fore_color_byte;
			break;
		case	op_or:
			for (i=x2-x1;i>=0;i--)
				*base++ = fore_color_byte;
			break;
		case	op_max :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				*base++ = calc_max_8(a_cs,old_color,fore_color);
			}
			break;
		case	op_hilite :
			for (i=x2-x1;i>=0;i--) {
				*base++ = calc_hilite_8(a_cs,*base,fore_color,back_color);
			}
			break;
		case	op_add :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				*base++ = calc_add_8(a_cs,old_color,fore_color);
			}
			break;
		case	op_and :
			for (i=x2-x1;i>=0;i--)
				*base++ = back_color_byte;
			break;
		case	op_sub :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				*base++ = calc_sub_8(a_cs,old_color,fore_color);
			}
			break;
		case	op_min :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				*base++ = calc_min_8(a_cs,old_color,fore_color);
			}
			break;
		case	op_blend :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				*base++ = calc_blend_8(a_cs,old_color,fore_color);
			}
			break;
		}
	}
}

/*--------------------------------------------------------------------*/

void run_list_pattern_8(port	    *a_port,
						raster_line *list,
						long        list_cnt,
						rect	    *cliprect,
						pattern     *the_pattern,
						long	    phase_x,
						long	    phase_y,
						short       mode)
{
	long    x1,x2,y1,x_phase;
	uchar	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i,cnt;
	long	tmp;
	ulong	fore_color_byte,back_color_byte;
	uchar	umask;
	char    invertmask;
	rgb_color old_color,fore_color;
	rgb_color back_color;
	server_color_map	*a_cs;

	a_cs = a_port->port_cs;
	fore_color = a_port->fore_color;
	back_color = a_port->back_color;
   	fore_color_byte = color_2_index(a_cs, fore_color);	
	back_color_byte = color_2_index(a_cs, a_port->back_color);

	rowbyte = a_port->rowbyte;

	for (cnt=0;cnt<list_cnt;cnt++) {
		x1 = list->x0;
		x2 = list->x1;
		y1 = list->y;

		list++;
		
		if (y1 < cliprect->top)
		    continue;
		if (y1 > cliprect->bottom)
			continue;

		if (x2 < x1) {
			tmp = x2;
			x2 = x1;
			x1 = tmp;
		}

		if (x1 < cliprect->left)
			x1 = cliprect->left;
	
		if (x2 > cliprect->right)
			x2 = cliprect->right;
			
		if (x1 > x2) 
			continue;		/*no intersection*/

		x_phase = (phase_x+x1)&7;
		
		pb = the_pattern->data[(y1 + phase_y) & 7];
		pb = rol(pb, x_phase);

		umask = 0x80;

		base = (uchar*)a_port->base + (y1 * rowbyte) + x1;
		
		switch (mode) {
		case	op_copy:
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = fore_color_byte;
				else
					*base++ = back_color_byte;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_or:
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = fore_color_byte;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_max :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				if (pb & umask)
					*base++ = calc_max_8(a_cs,old_color,fore_color);
				else
					*base++ = calc_max_8(a_cs,old_color,fore_color);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_hilite :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base = calc_hilite_8(a_cs,*base,fore_color,back_color);
				base++;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_add :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				if (pb & umask)
					*base++ = calc_add_8(a_cs,old_color,fore_color);
				else
					*base++ = calc_add_8(a_cs,old_color,fore_color);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_and :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = back_color_byte;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_sub :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				if (pb & umask)
					*base++ = calc_sub_8(a_cs,old_color,fore_color);
				else
					*base++ = calc_sub_8(a_cs,old_color,fore_color);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_min :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				if (pb & umask)
					*base++ = calc_min_8(a_cs,old_color,fore_color);
				else
					*base++ = calc_min_8(a_cs,old_color,fore_color);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_blend :
			for (i=x2-x1;i>=0;i--) {
				old_color = a_cs->color_list[*base];
				if (pb & umask)
					*base++ = calc_blend_8(a_cs,old_color,fore_color);
				else
					*base++ = calc_blend_8(a_cs,old_color,fore_color);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_xor :
			invertmask = -1;
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base = invertmask^(*base);
				base++;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		}
	}
}





















