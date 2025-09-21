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


//#undef	DEBUG_HARD

/*----------------------------------------------------------------------------*/
#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))
#define	as_long(v)				\
	*((long *)&v)
/*----------------------------------------------------------------------------*/

void rect_fill_copy_24_np(	port		*a_port,
			 				rect		*r,
			 				rgb_color	color)
{
	ulong	*base;		
	long	rowbyte;	
	long	y,x,i;	
	rect	tmprect;	
	ulong	*tmpbase;	
	ulong	fore_color_long;
	long	size;

	fore_color_long = fcolor_2_long(a_port, color);

	tmprect = *r;
	tmprect.right += 1;
	
		
	size = (r->bottom-r->top)*(r->right-r->left);
#ifdef	DEBUG_HARD
	if (size > 150)
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (rect_24_jmp) {
			(rect_24_jmp)(r->left, r->top, r->right, r->bottom, fore_color_long);
			(synchro_jmp)();
			return;
		}
	}
#endif

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	while (y <= tmprect.bottom) {
		tmpbase = base;
		base += rowbyte;

		i = x;

		while (i > 0) {
			i--;
			*tmpbase++ = fore_color_long;
		}
		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_copy_24_pat0(port	*a_port,
							rect	*r,
			 				pattern	*the_pattern,
			 				long	phase_x,
			 				long	phase_y)
{
	ulong	*base,*tmpbase;		
	long	rowbyte,y,x,i;	
	rect	tmprect;	
	ulong	pb;	
	ulong	fore_color_long,back_color_long;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_24_np(a_port, r, a_port->fore_color);
		return;
	}	
	if ((as_long(the_pattern->data[0]) == 0) &&
	    (as_long(the_pattern->data[4]) == 0)) {
		rect_fill_copy_24_np(a_port, r, a_port->back_color);
		return;
	}	
	
	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);
	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + tmprect.top * rowbyte + tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;
	
	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);

		tmpbase = base;
		base += rowbyte;

		i = x;
		umask = 0x80;

		while (i > 0) {
			i--;
			if (pb & umask)
				*tmpbase++ = fore_color_long;
			else
				*tmpbase++ = back_color_long;
			umask >>= 1;
			if (umask == 0)
				umask = 0x80;
		}
		y++;
	}
}

/*----------------------------------------------------------------------------*/


void rect_fill_copy_24(	port	*a_port,
			 			rect	*r,
			 			pattern *the_pattern,
			 			long	phase_x,
			 			long	phase_y)
{
	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_24_np(a_port, r, a_port->fore_color);
		return;
	}	
	if ((as_long(the_pattern->data[0]) == 0) &&
	    (as_long(the_pattern->data[4]) == 0)) {
		rect_fill_copy_24_np(a_port, r, a_port->back_color);
		return;
	}	

	rect_fill_copy_24_pat0(a_port, r, the_pattern, phase_x, phase_y);
}


void rect_fill_other_24_opt(port	*a_port,
						 	rect	*r,
			 				pattern *the_pattern,
			 				long	phase_x,
			 				long	phase_y,
			 				short	mode)
{
	long		y;
	
	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		for (y=r->top;y<=r->bottom;y++)
			horizontal_line_other_24(a_port,r->left,r->right,y,r,mode);
	}
	else
		for (y=r->top;y<=r->bottom;y++)
			horizontal_line_other_pattern_24(a_port,r->left,r->right,y,r,the_pattern,phase_x,phase_y,mode);
}


/*--------------------------------------------------------------------*/ 

void rect_fill_other_24(port	*a_port,
			 			rect	*r,
			 			pattern	*the_pattern,
			 			long	phase_x,
			 			long	phase_y,
			 			short	mode)
{
	long		y;
	
	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		for (y=r->top;y<=r->bottom;y++)
			horizontal_line_other_24(a_port,r->left,r->right,y,r,mode);
	}
	else
		for (y=r->top;y<=r->bottom;y++)
			horizontal_line_other_pattern_24(a_port,r->left,r->right,y,r,the_pattern,phase_x,phase_y,mode);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_min_24  (port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_min);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_max_24  (port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_max);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_hilite_24  (port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_hilite);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_add_24  (port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_add);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_sub_24   (port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_sub);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_blend_24 (port *a_port,
						rect *r,
						pattern *the_pattern,
						long phase_x,
						long phase_y)
{
	rect_fill_other_24(a_port, r, the_pattern, phase_x, phase_y, op_blend);
}

/*--------------------------------------------------------------------*/ 
typedef union {
		ulong	as_long[2];
		double	as_double;
} trick1;
/*--------------------------------------------------------------------*/ 

void rect_fill_xor_24_np (port *a_port,
			 			  rect *r)
{
	ulong		*base,*tmpbase;		
	long		rowbyte;	
	long		y,x,i,size;	
	rect		tmprect;	
	ulong		invertmask;
	trick1		reader, writer;

	size = (r->bottom-r->top)*(r->right-r->left);
#ifdef	DEBUG_HARD
	if (size > 80)
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (invert_rect_24_jmp) {
			(invert_rect_24_jmp)(r->left, r->top, r->right, r->bottom);
			(synchro_jmp)();
			return;
		}
	}
#endif

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	invertmask = -1L;

	if (x == 0)
		return;

	while (y <= tmprect.bottom) {

		tmpbase = base;
		base += rowbyte;

		i = x;

		if ((long)tmpbase & 0x07) {
			*tmpbase = invertmask - *tmpbase;
			tmpbase++;
			i--;
		}		

		while(i > 4) {
			i -= 4;
			reader.as_double = *((double *)tmpbase);
			writer.as_long[0] = invertmask - reader.as_long[0];
			writer.as_long[1] = invertmask - reader.as_long[1];
			*((double *)tmpbase) = writer.as_double;
			tmpbase += 2;
			reader.as_double = *((double *)tmpbase);
			writer.as_long[0] = invertmask - reader.as_long[0];
			writer.as_long[1] = invertmask - reader.as_long[1];
			*((double *)tmpbase) = writer.as_double;
			tmpbase += 2;
		}

		while (i > 0) {
			i--;
			*tmpbase++ = invertmask - *tmpbase;
		}
		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_xor_24     ( port *a_port,
			 				rect *r,
			 				pattern *the_pattern,
			 				long phase_x,
			 				long phase_y)
{
	ulong	*base,*tmpbase;		
	long	rowbyte;	
	long	y,x,i;	
	rect	tmprect;	
	ulong	pb,invertmask;	
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_xor_24_np(a_port, r);
		return;
	}	

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte>>2;

	invertmask = -1L;
	base = (ulong*)a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;

	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);

		tmpbase = base;
		base += rowbyte;

		if (pb) {
			i = x;

			umask = 0x80;

			while (i > 0) {
				i--;
				if (pb & umask)
					*tmpbase = invertmask-*tmpbase;
				tmpbase++;
				umask >>= 1;
				if (umask == 0)
					umask = 0x80; 
			}
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_or_24     (	port *a_port,
			 				rect *r,
			 				pattern *the_pattern,
			 				long phase_x,
			 				long phase_y)
{
	rect	tmprect;	
	ulong	*base,*tmpbase;	
	long	rowbyte;	
	long	y,x,i;	
	ulong	pb;	
	ulong	fore_color_long;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_24_np(a_port, r, a_port->fore_color);
		return;
	}	
	
	fore_color_long = fcolor_2_long(a_port, a_port->fore_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;
	
	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);

		tmpbase = base;
		base += rowbyte;

		if (pb) {
			i = x;

			umask = 0x80;

			while (i > 0) {
				i--;
				if (pb & umask)
					*tmpbase = fore_color_long;
				tmpbase++;	
				umask >>= 1;
				if (umask == 0)
					umask = 0x80; 
			}
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_and_24    (	port *a_port,
			 				rect *r,
			 				pattern *the_pattern,
			 				long phase_x,
			 				long phase_y)
{
	ulong	*base,*tmpbase;		
	long	rowbyte;	
	long	y,x,i;	
	rect	tmprect;	
	ulong	pb;	
	ulong	back_color_long;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_24_np(a_port, r, a_port->back_color);
		return;
	}	
	
	back_color_long = fcolor_2_long(a_port, a_port->back_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte>>2;

	base = (ulong*)a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;
	
	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);
		pb = ~pb;

		tmpbase = base;
		base += rowbyte;
		if (pb) {

			i = x;

			umask = 0x80;

			while (i > 0) {
				i--;
				if (pb & umask)
					*tmpbase = back_color_long;
				tmpbase++;
				umask >>= 1;
				if (umask == 0)
					umask = 0x80; 
			}
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_24(port	*a_port, rect *r, pattern *the_pattern, long phase_x, long phase_y, short mode)
{
	switch(mode) {
		case	op_copy:
				rect_fill_copy_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_or:
				rect_fill_or_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_max :
				rect_fill_max_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_hilite :
				rect_fill_hilite_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_add :
				rect_fill_add_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_sub :
				rect_fill_sub_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_min :
				rect_fill_min_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_blend :
				rect_fill_blend_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_xor :
				rect_fill_xor_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_and :
				rect_fill_and_24(a_port, r, the_pattern, phase_x, phase_y);
				return;
	}
}

 
