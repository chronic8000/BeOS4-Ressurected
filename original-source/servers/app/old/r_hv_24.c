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

/*--------------------------------------------------------------------*/
#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))
/*--------------------------------------------------------------------*/

void horizontal_line_xor_pattern_24(port	*a_port,
				    				long	x1,
				    				long	x2,
				    				long	y1,
				    				rect	*cliprect,
				    				pattern	*the_pattern,
				    				long	phase_x,
				    				long	phase_y)
{
	ulong	*base;		
	long	rowbyte;	
	ulong	pb,invertmask;	
	long	i;
	long	tmp;
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

	x2++;
/* END CLIPPING CODE HERE */

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	umask = 0x80;
	invertmask = -1L;

	while (i > 0) {
		i--;
		if (pb & umask)
			*base = invertmask^(*base);
		base++;

		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}
}

/*--------------------------------------------------------------------*/

void horizontal_line_or_pattern_24(	port	*a_port,
				    				long	x1,
				    				long	x2,
				    				long	y1,
				    				rect	*cliprect,
				    				pattern *the_pattern,
				    				long	phase_x,
				    				long	phase_y)
{
	ulong	*base;		
	long	rowbyte;	
	ulong	pb,fore_color_long;	
	long	i,j;
	long	tmp;
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

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	
	x2++;
/* END CLIPPING CODE HERE */

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	umask = 0x80;

	while (i > 7) {
		j = 4;
		while (j--) {
			if (pb & umask)
				*base = fore_color_long;
			base++;
			umask >>= 1;

			if (pb & umask)
				*base = fore_color_long;
			base++;
			umask >>= 1;
		}
		i -= 8;
		umask = 0x80;
	}

	while (i > 0) {
		i--;
		if (pb & umask)
			*base = fore_color_long;
		base++;
		umask >>= 1;
	}
}

/*--------------------------------------------------------------------*/

void horizontal_line_copy_pattern_24(port	*a_port,
				    				 long	x1,
				    				 long	x2,
				    				 long	y1,
				    				 rect	*cliprect,
				    				 pattern*the_pattern,
				    				 long	phase_x,
				    				 long	phase_y)
{
	ulong	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	tmp;
	ulong	fore_color_long,back_color_long;
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

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	x2++;

/* END CLIPPING CODE HERE */

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;

	umask = 0x80;

	while (i > 0) {
		i--;
		if (pb & umask)
			*base++ = fore_color_long;
		else
			*base++ = back_color_long;
		umask >>= 1;
		if (umask == 0)
			umask = 0x80;
	}
}

/*--------------------------------------------------------------------*/

void horizontal_line_and_pattern_24(port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y)
{
	ulong	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i;
	long	tmp;
	ulong	back_color_long;
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

	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	x2++;

/* END CLIPPING CODE HERE */

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;

	umask = 0x80;

	while (i > 0) {
		i--;
		if (pb & umask)
			*base = back_color_long;
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

void horizontal_line_and_24(port *a_port,
			   				long x1,
			   				long x2,
			   				long y1,
			   				rect *cliprect)
{
	horizontal_line_and_pattern_24 (a_port, x1, x2, y1, cliprect, &data_black, 0, 0);
}
			   
/*--------------------------------------------------------------------*/

void vertical_line_copy_pattern_24( port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)

{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;
	ulong	fore_color_long,back_color_long;

	rowbyte = a_port->rowbyte>>2;

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

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	back_color_long = fcolor_2_long(a_port, a_port->back_color);
	
	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	phase_x &= 7;
	
	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base = fore_color_long;
		else
			*base = back_color_long;

		base += rowbyte;
		y1++;
	}
}


/*--------------------------------------------------------------------*/ 

void vertical_line_xor_24(	port *a_port,
			        		long x1,
			        		long y1,
			        		long y2,
			        		rect *cliprect)
{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	ulong	invertmask;

	rowbyte = a_port->rowbyte>>2;

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

	invertmask = -1L;
	
	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	while (y1 <= y2) {
		*base = invertmask^(*base);
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_and_24 ( port *a_port,
			        		long x1,
			        		long y1,
			        		long y2,
			        		rect *cliprect)

{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	ulong	back_color_long;

	rowbyte = a_port->rowbyte>>2;

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

	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	while (y1 <= y2) {
		*base = back_color_long;
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_copy_24 (port *a_port,
			        		long x1,
			        		long y1,
			        		long y2,
			        		rect *cliprect)

{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	ulong	fore_color_long;

	rowbyte = a_port->rowbyte>>2;

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

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	
#ifdef	USE_HARDWARE
	if ((y2-y1) > 40)		//has to be measured to be optimal !!!
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (line_24_jmp) {
			(line_24_jmp)(x1,x1,y1,y2,fore_color_long,0,0,0,0,0);
			(synchro_jmp)();
			return;
		}
	}

#endif

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	while ((y2 - y1) > 3) {
		*base = fore_color_long;
		base += rowbyte;
		*base = fore_color_long;
		base += rowbyte;
		*base = fore_color_long;
		base += rowbyte;
		y1 += 3;
	}

	while (y1 <= y2) {
		*base = fore_color_long;
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_copy_24(port *a_port,
				    		long x1,
				    		long x2,
				    		long y1,
				    		rect *cliprect)
{
	ulong	*base;		
	long	rowbyte;	
	long	i;
	long	tmp;
	ulong	fore_color_long;


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
	
	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);

	i = x2-x1;

#ifdef	USE_HARDWARE
	if (i > 50)			//has to be measured to be optimal !!!
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (line_24_jmp) {
			(line_24_jmp)(x1,x2,y1,y1,fore_color_long,0,0,0,0,0);
			(synchro_jmp)();
			return;
		}
	}
#endif

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	while (i >= 4) {
		i-=4;
		base[0] = fore_color_long;
		base[1] = fore_color_long;
		base[2] = fore_color_long;
		base[3] = fore_color_long;
		base += 4;
	}

	while (i >= 0) {
		i--;
		*base++ = fore_color_long;
	}
}

/*--------------------------------------------------------------------*/ 
typedef union {
		ulong	as_long[2];
		double	as_double;
} trick1;
/*--------------------------------------------------------------------*/ 


void horizontal_line_hilite_24(  port *a_port,
				 				long x1,
				 				long x2,
				 				long y1,
				 				rect *cliprect)
{
	ulong		*base;	
	long		rowbyte;	
	long		i;
	long		tmp;
	ulong		tmp_color;
	ulong		back_color;
	ulong		tmp_b,tmp_b0;
	ulong		out;
	trick1		reader;
	trick1		writer;

	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	if (cliprect) {
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
	}	

	x2++;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;
	tmp_color =  fcolor_2_long(a_port, a_port->fore_color);
	back_color = fcolor_2_long(a_port, a_port->back_color);

	if (i > 0) {
		if (((long)base & 0x07) != 0) {
			tmp_b0 = *base;
			out = calc_hilite_24(tmp_b0, tmp_color, back_color);
			*base++ = out;
			i--;
		}
	}
		
	tmp_b0 = *base;				// be certain to get a value different from the next one
	tmp_b0 -= 1;

	while(i >= 2) {
		reader.as_double = *((double *)base);
		tmp_b = reader.as_long[0];
		if (tmp_b != tmp_b0) {
			tmp_b0 = tmp_b;
			out = calc_hilite_24(tmp_b0, tmp_color, back_color);
		}
		writer.as_long[0] = out;

		tmp_b = reader.as_long[1];
		if (tmp_b != tmp_b0) {
			tmp_b0 = tmp_b;
			out = calc_hilite_24(tmp_b0, tmp_color, back_color);
		}
		writer.as_long[1] = out;
		*((double *)base) = writer.as_double;
		base += 2;
		i -= 2;
	}

	if (i > 0) {
		tmp_b = *base;
		if (tmp_b != tmp_b0) {
			tmp_b0 = tmp_b;
			out = calc_hilite_24(tmp_b0, tmp_color, back_color);
		}
		*base = out;
	}
}


/*--------------------------------------------------------------------*/ 

void horizontal_line_other_24(  port *a_port,
				 				long x1,
				 				long x2,
				 				long y1,
				 				rect *cliprect,
				 				short mode)
{
	ulong		*base;	
	long		rowbyte;	
	long		i;
	long		tmp;
	ulong		tmp_color;
	ulong		back_color;
	ulong		tmp_b,tmp_b0;
	ulong		out;

	if (mode == op_hilite) {
		horizontal_line_hilite_24(a_port,
				 				  x1,
				 				  x2,
				 				  y1,
				 				  cliprect);
		return;
	}

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

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;
	tmp_color =  fcolor_2_long(a_port, a_port->fore_color);
	back_color = fcolor_2_long(a_port, a_port->back_color);

	switch(mode) {
		case op_add:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_add_24(tmp_b0, tmp_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_add_24(tmp_b0, tmp_color);
					}
					*base++ = out;
				}
			}
			return;
		case op_sub:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_sub_24(tmp_b0, tmp_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_sub_24(tmp_b0, tmp_color);
					}
					*base++ = out;
				}
			}
			return;
		case op_min:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_min_24(tmp_b0, tmp_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_min_24(tmp_b0, tmp_color);
					}
					*base++ = out;
				}
			}
			return;
		case op_max:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_max_24(tmp_b0, tmp_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_max_24(tmp_b0, tmp_color);
					}
					*base++ = out;
				}
			}
			return;

		case op_hilite:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_hilite_24(tmp_b0, tmp_color, back_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_hilite_24(tmp_b0, tmp_color, back_color);
					}
					*base++ = out;
				}
			}
			return;

		case op_blend:
			if (i > 0) {
				i--;
				tmp_b0 = *base;
				out = calc_blend_24(tmp_b0, tmp_color);
				*base++ = out;
				while (i > 0) {
					i--;
					tmp_b = *base;
					if (tmp_b != tmp_b0) {
						tmp_b0 = tmp_b;
						out = calc_blend_24(tmp_b0, tmp_color);
					}
					*base++ = out;
				}
			}
			return;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_add_24	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_24(a_port, x1, x2, y1, cliprect, op_add);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_sub_24(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_24(a_port, x1, x2, y1, cliprect, op_sub);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_min_24	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_24(a_port, x1, x2, y1, cliprect, op_min);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_max_24(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	horizontal_line_other_24(a_port, x1, x2, y1, cliprect, op_max);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_blend_24(port *a_port,
				 			  long x1,
				 			  long x2,
				 			  long y1,
				 			  rect *cliprect)
{
	horizontal_line_other_24(a_port, x1, x2, y1, cliprect, op_blend);
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_xor_24	(port *a_port,
				 			long x1,
				 			long x2,
				 			long y1,
				 			rect *cliprect)
{
	ulong	*base;	
	ulong	invertmask;	
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

	invertmask = -1L;
	
	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	i = x2 - x1;

	while (i > 0) {
		i--;
		*base = invertmask-*base;
		base++;
	}
}


void horizontal_line_other_pattern_24(  port *a_port,
				    					long x1,
				    					long x2,
				    					long y1,
				    					rect *cliprect,
				    					pattern *the_pattern,
				    					long phase_x,
				    					long phase_y, 
				    					short mode)
{
	ulong		*base;		
	long		rowbyte;	
	ulong		pb;	
	ulong		fore_color,back_color,tmp_color,old_color;
	ulong		exp_pat[8];
	long		i,tmp;
	uchar		umask;
	
	if (x2 < x1) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

/* CLIPPING CODE HERE	*/
	
	if (cliprect != 0) {
		if (y1 < cliprect->top)
			return;
		if (y1 > cliprect->bottom)
			return;
	
		if (x1 < cliprect->left)
			x1 = cliprect->left;
		
		if (x2 > cliprect->right)
			x2 = cliprect->right;
		}
			
	if (x1 > x2) 
		return;		/*no intersection*/

	fore_color = fcolor_2_long(a_port, a_port->fore_color);
	back_color = fcolor_2_long(a_port, a_port->back_color);
	
	x2++;

/* END CLIPPING CODE HERE */

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	
	pb = the_pattern->data[(y1 + phase_y) & 7];
	pb = rol(pb, phase_x);

	i = x2 - x1;

	umask = 0x80;

	for (tmp = 0; tmp < 8; tmp++) {
		exp_pat[tmp] = (pb & umask) ? fore_color : back_color;
		umask >>= 1;
	}

	tmp = 0;

	while (i > 0) {
		i--;
		tmp_color = exp_pat[tmp & 7];
		old_color = *base;
		switch(mode) {
			case op_add :
			*base = calc_add_24(old_color, tmp_color);
			break;

			case op_sub :
			*base = calc_sub_24(old_color, tmp_color);
			break;

			case op_blend :
			*base = calc_blend_24(old_color, tmp_color);
			break;	

			case op_min :
			*base = calc_min_24(old_color, tmp_color);
			break;	

			case op_max :
			*base = calc_max_24(old_color, tmp_color);
			break;	
			
			case op_hilite :
			*base = calc_hilite_24(old_color, fore_color, back_color);
			break;
		}	
		tmp++;
		base++;
	}
}

/*--------------------------------------------------------------------*/ 

void horizontal_line_add_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_add);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_sub_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_sub);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_min_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_min);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_max_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_max);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_blend_pattern_24(	port *a_port,
					    long x1,
					    long x2,
					    long y1,
					    rect *cliprect,
					    pattern *the_pattern,
					    long phase_x,
					    long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_blend);
} 

/*--------------------------------------------------------------------*/ 

void horizontal_line_hilite_pattern_24(	port *a_port,
					    				long x1,
					    				long x2,
					    				long y1,
					    				rect *cliprect,
					    				pattern *the_pattern,
					    				long phase_x,
					    				long phase_y)
{
	horizontal_line_other_pattern_24(a_port, x1, x2, y1, cliprect, the_pattern, phase_x, phase_y, op_hilite);
} 


/*--------------------------------------------------------------------*/ 

void vertical_line_other_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y,
			        short mode)
{
	ulong		*base;		
	long		rowbyte;	
	ulong		fore_color,back_color,tmp_color,old_color;
	ulong		exp_pat[8];
	long		tmp;
	uchar		umask;

	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	
/* CLIPPING CODE HERE	*/
	if (y1 < cliprect->top)
		y1 = cliprect->top;
	if (y2 > cliprect->bottom)
		y2 = cliprect->bottom;
	if (y1>y2)
		return;
/* END CLIPPING CODE HERE */

	fore_color = fcolor_2_long(a_port, a_port->fore_color);
	back_color = fcolor_2_long(a_port, a_port->back_color);

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	phase_x += x1;
	phase_x &= 7;
	umask = 0x80>>phase_x;

	phase_y += y1;

	for (tmp = 0; tmp < 8; tmp++) {
		exp_pat[tmp] = (the_pattern->data[phase_y & 7] & umask) ? fore_color : back_color;
		phase_y++;
	}

	tmp = 0;

	while (y1<=y2) {
		tmp_color = exp_pat[tmp & 7];
		old_color = *base;
		switch(mode) {
			case op_add :
			*base = calc_add_24(old_color, tmp_color);
			break;

			case op_sub :
			*base = calc_sub_24(old_color, tmp_color);
			break;

			case op_blend :
			*base = calc_blend_24(old_color, tmp_color);
			break;	

			case op_min :
			*base = calc_min_24(old_color, tmp_color);
			break;	

			case op_max :
			*base = calc_max_24(old_color, tmp_color);
			break;	
				
			case op_hilite :
			*base = calc_hilite_24(old_color, fore_color, back_color);
			break;
		}	
		tmp++;
		y1++;
		base+=rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_add_pattern_24(	port *a_port,
							        long x1,
							        long y1,
							        long y2,
							        rect *cliprect,
							        pattern *the_pattern,
							        long phase_x,
							        long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_add);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_sub_pattern_24(	port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_sub);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_min_pattern_24(	port *a_port,
			    				    long x1,
			    				    long y1,
			    				    long y2,
			    				    rect *cliprect,
			    				    pattern *the_pattern,
			    				    long phase_x,
			    				    long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_min);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_max_pattern_24(	port *a_port,
							        long x1,
							        long y1,
							        long y2,
							        rect *cliprect,
							        pattern *the_pattern,
							        long phase_x,
							        long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_max);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_blend_pattern_24(port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_blend);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_hilite_pattern_24(port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)
{
	vertical_line_other_pattern_24(a_port,x1,y1,y2,cliprect,the_pattern,phase_x,phase_y,op_hilite);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_other_24      (	port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
									short mode)

{
	ulong		*base;
	long		rowbyte;
	long		tmp;
	ulong		fore_color;
	ulong		back_color;
	ulong		old_color;

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

	rowbyte = a_port->rowbyte>>2;
	fore_color = fcolor_2_long(a_port, a_port->fore_color);
	back_color = fcolor_2_long(a_port, a_port->back_color);

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	while (y1 <= y2) {
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
			*base = calc_hilite_24(old_color, fore_color, back_color);
			break;
		}	
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_add_24      (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_add);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_sub_24      (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_sub);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_min_24      (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_min);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_hilite_24    (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_hilite);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_max_24      (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_max);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_blend_24    (port *a_port,
			        			long x1,
			        			long y1,
			        			long y2,
			        			rect *cliprect)
{
	vertical_line_other_24(a_port, x1, y1, y2, cliprect, op_blend);
}

/*--------------------------------------------------------------------*/ 

void vertical_line_xor_pattern_24(	port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)

{
	ulong	*base;
	ulong	invertmask;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	pb;

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

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - (x1 & 7)));
	invertmask = -1L;

	phase_x &= 7;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base = invertmask-*base;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_or_pattern_24(	port *a_port,
							        long x1,
							        long y1,
							        long y2,
							        rect *cliprect,
							        pattern *the_pattern,
							        long phase_x,
							        long phase_y)

{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	pb;
	ulong	fore_color_long;

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

	rowbyte = a_port->rowbyte>>2;

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	
	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - ((phase_x+x1) & 7)));

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];

		if (pb & mask)
			*base = fore_color_long;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_and_pattern_24(	port *a_port,
			        				long x1,
			        				long y1,
			        				long y2,
			        				rect *cliprect,
			        				pattern *the_pattern,
			        				long phase_x,
			        				long phase_y)

{
	ulong	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	pb;
	ulong	back_color_long;

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

	rowbyte = a_port->rowbyte>>2;

	back_color_long = fcolor_2_long(a_port, a_port->back_color);
	
	base = (ulong*)a_port->base + (y1 * rowbyte) + x1;

	mask = (1 << (7 - ((x1+phase_x) & 7)));

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];

		if (!(pb & mask))
			*base = back_color_long;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void horiz_line_24(port *theport, long x1, long x2, long y2, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or:
				horizontal_line_copy_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_max :
				horizontal_line_max_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_hilite :
				horizontal_line_hilite_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_add :
				horizontal_line_add_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_sub :
				horizontal_line_sub_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_min :
				horizontal_line_min_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_blend :
				horizontal_line_blend_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_xor :
				horizontal_line_xor_24(theport, x1, x2, y2, cliprect);
				return;
		case	op_and :
				horizontal_line_and_24(theport, x1, x2, y2, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void vert_line_24(port *theport, long x1, long y1, long y2, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or:
				vertical_line_copy_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_max :
				vertical_line_max_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_hilite :
				vertical_line_hilite_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_add :
				vertical_line_add_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_sub :
				vertical_line_sub_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_min :
				vertical_line_min_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_blend :
				vertical_line_blend_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_xor :
				vertical_line_xor_24(theport, x1, y1, y2, cliprect);
				return;
		case	op_and :
				vertical_line_and_24(theport, x1, y1, y2, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void vert_line_pattern_24(port *theport,
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
				vertical_line_copy_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
                return;
		case	op_or:
				vertical_line_or_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_max :
				vertical_line_max_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_hilite :
				vertical_line_hilite_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_add :
				vertical_line_add_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_and :
				vertical_line_and_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
				vertical_line_sub_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_min :
				vertical_line_min_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
				vertical_line_blend_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_xor :
				vertical_line_xor_pattern_24(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void horiz_line_pattern_24(port *theport,
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
				horizontal_line_copy_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
                return;
	    case	op_or:
				horizontal_line_or_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_max :
				horizontal_line_max_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_hilite :
				horizontal_line_hilite_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_add :
				horizontal_line_add_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_and :
				horizontal_line_and_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
				horizontal_line_sub_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_min :
				horizontal_line_min_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
				horizontal_line_blend_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_xor :
				horizontal_line_xor_pattern_24(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*--------------------------------------------------------------------*/

void run_list_24_copy(port	     *a_port,
				 	  raster_line *list,
				 	  long        list_cnt,
				 	  rect	     *cliprect,
					  char		 single_pixels)
{
	long    x1,x2,y1;
	ulong	*base;		
	long	rowbyte;	
	long	i,cnt;
	long	tmp;
	ulong	fore_color_long;


	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);

	rowbyte = a_port->rowbyte>>2;

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

		base = (ulong*)a_port->base + (y1 * rowbyte) + x1;
		
		for (i=x2-x1;i>=0;i--)
			*base++ = fore_color_long;
	}
}

/*--------------------------------------------------------------------*/

void run_list_24_xor (port	     *a_port,
				 	  raster_line *list,
				 	  long        list_cnt,
				 	  rect	     *cliprect,
					  char		 single_pixels)
{
	long    x1,x2,y1;
	ulong	*base;		
	long	rowbyte;	
	long	i,cnt;
	long	tmp;
	long    invertmask;
	long	top,left,bottom,right;

 
	rowbyte = a_port->rowbyte>>2;

	invertmask = -1L;

	top = cliprect->top;
	left = cliprect->left;
	bottom = cliprect->bottom;
	right = cliprect->right;
	
	if (single_pixels) {
		

		for (cnt=0;cnt<list_cnt;cnt++) {
			x1 = list->x0;
			y1 = list->y;

			list++;
			
			if (y1 < top)
			    continue;
			if (y1 > bottom)
				continue;

			if (x1 < left)
				continue;	

			if (x1 > right)
				continue;	
				
			base = (ulong*)a_port->base + (y1 * rowbyte) + x1;
			
			*base++ = invertmask^(*base);
		}
	}
	else
	for (cnt=0;cnt<list_cnt;cnt++) {
		x1 = list->x0;
		x2 = list->x1;
		y1 = list->y;

		list++;
		
		if (y1 < top)
		    continue;
		if (y1 > bottom)
			continue;

		if (x2 < x1) {
			tmp = x2;
			x2 = x1;
			x1 = tmp;
		}

		if (x1 < left)
			x1 = left;
	
		if (x2 > right)
			x2 = right;
			
		if (x1 > x2) 
			continue;		/*no intersection*/

		base = (ulong*)a_port->base + (y1 * rowbyte) + x1;
		
		for (i=x2-x1;i>=0;i--)
			*base++ = invertmask^(*base);
	}
}

/*--------------------------------------------------------------------*/

void run_list_24(port	     *a_port,
				 raster_line *list,
				 long        list_cnt,
				 rect	     *cliprect,
				 short       mode,
				 char		 single_pixels)
{
	long    x1,x2,y1;
	ulong	*base;		
	long	rowbyte;	
	long	i,cnt;
	long	tmp;
	ulong	fore_color_long,back_color_long;

	switch(mode) {
		case	op_or:
		case	op_copy :
				run_list_24_copy(a_port, list, list_cnt, cliprect, single_pixels);
				return;
		case	op_xor :
				run_list_24_xor(a_port, list, list_cnt, cliprect, single_pixels);
				return;
	}

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	rowbyte = a_port->rowbyte>>2;

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

		base = (ulong*)a_port->base + (y1 * rowbyte) + x1;
		
		switch (mode) {
		case	op_max :
			for (i=x2-x1;i>=0;i--)
				*base++ = calc_max_24(*base,fore_color_long);
			break;
		case	op_hilite :
			break;
		case	op_add :
			for (i=x2-x1;i>=0;i--)
				*base++ = calc_add_24(*base,fore_color_long);
			break;
		case	op_and :
			for (i=x2-x1;i>=0;i--)
				*base++ = back_color_long;
			break;
		case	op_sub :
			for (i=x2-x1;i>=0;i--)
				*base++ = calc_sub_24(*base,fore_color_long);
			break;
		case	op_min :
			for (i=x2-x1;i>=0;i--)
				*base++ = calc_min_24(*base,fore_color_long);
			break;
		case	op_blend :
			for (i=x2-x1;i>=0;i--)
				*base++ = calc_blend_24(*base,fore_color_long);
			break;
		}
	}
}

/*--------------------------------------------------------------------*/

void run_list_pattern_24(port	     *a_port,
						 raster_line *list,
						 long        list_cnt,
						 rect	     *cliprect,
						 pattern     *the_pattern,
						 long	     phase_x,
						 long	     phase_y,
						 short       mode)
{
	long    x1,x2,y1,x_phase;
	ulong	*base;		
	long	rowbyte;	
	ulong	pb;	
	long	i,cnt;
	long	tmp;
	ulong	fore_color_long,back_color_long;
	uchar	umask;
	long    invertmask;

	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	rowbyte = a_port->rowbyte>>2;

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

		base = (ulong*)a_port->base + (y1 * rowbyte) + x1;
		
		switch (mode) {
		case	op_copy:
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = fore_color_long;
				else
					*base++ = back_color_long;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_or:
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = fore_color_long;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_max :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = calc_max_24(*base,fore_color_long);
				else
					*base++ = calc_max_24(*base,back_color_long);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_hilite :
			break;
		case	op_add :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = calc_add_24(*base,fore_color_long);
				else
					*base++ = calc_add_24(*base,back_color_long);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_and :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = back_color_long;
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_sub :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = calc_sub_24(*base,fore_color_long);
				else
					*base++ = calc_sub_24(*base,back_color_long);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_min :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = calc_min_24(*base,fore_color_long);
				else
					*base++ = calc_min_24(*base,back_color_long);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_blend :
			for (i=x2-x1;i>=0;i--) {
				if (pb & umask)
					*base++ = calc_blend_24(*base,fore_color_long);
				else
					*base++ = calc_blend_24(*base,back_color_long);
				umask >>= 1;
				if (umask == 0) umask = 0x80;
			}
			break;
		case	op_xor :
			invertmask = -1L;
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




