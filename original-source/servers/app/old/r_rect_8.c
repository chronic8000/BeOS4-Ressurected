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

rgb_color	last_blend_fcolor;
rgb_color	last_blend_bcolor;

long		last_blend_mode = -1;
uchar		blend_array1[256];
uchar		blend_array2[256];
/*----------------------------------------------------------------------------*/
#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))
/*----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/ 

#ifdef __INTEL__
#define INTEL_BCC_BAD_ARGS_FIX 1
#endif
#if INTEL_BCC_BAD_ARGS_FIX

static void FOO(a, b, c, d, e)
	int a, b, c, d, e;
{

}

#endif

void rect_fill_copy_8_np(port *a_port,
			 			 rect *r,
			 			 rgb_color color)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	x;	
	uchar	*tmpbase;	
	long	fore_long;
	long	i;
	uchar	fore_color_byte;
	long	size;	

	fore_color_byte = color_2_index(a_port->port_cs, color);

	size = (r->bottom-r->top)*(r->right-r->left);
	if (size > 400)
	if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
		if (rect_8_jmp) {
#if INTEL_BCC_BAD_ARGS_FIX
			FOO(r->left, r->top, r->right, r->bottom, fore_color_byte);
#endif
			(rect_8_jmp)(r->left, r->top, r->right, r->bottom, fore_color_byte);
			(synchro_jmp);
			return;
		}
	}

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	fore_long = (fore_color_byte << 24) |
		    (fore_color_byte << 16) |
		    (fore_color_byte << 8)  |
		    fore_color_byte;	
	
	while (y <= tmprect.bottom) {
		tmpbase = base;
		base += rowbyte;

		i = x;

		while ((i > 0) && (((long)tmpbase & 0x3) != 0)) {
			i--;
			*tmpbase++ = fore_color_byte;
		}

		while (i >= 8) {
			write_long(tmpbase, fore_long);
			tmpbase += 4;
			write_long(tmpbase, fore_long);
			tmpbase += 4;
			i -= 8;
		}

		if (i >= 4) {
			write_long(tmpbase, fore_long);
			tmpbase += 4;
			i -= 4;
		}


		while (i > 0) {
			i--;
			*tmpbase++ = fore_color_byte;
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_copy_8_pat0( port *a_port,
			 				rect *r,
			 				pattern *the_pattern,
			 				long phase_x,
			 				long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	x;	
	uchar	*tmpbase;	
	ulong	pb;	
	long	i;
	long	tmp;
	uchar	fore_color_byte;
	uchar	back_color_byte;
	uchar	exp_pat[8];
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_8_np(a_port, r, a_port->fore_color);
		return;
	}	
	if ((as_long(the_pattern->data[0]) == 0) &&
	    (as_long(the_pattern->data[4]) == 0)) {
		rect_fill_copy_8_np(a_port, r, a_port->back_color);
		return;
	}	
	
	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

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

		while ((i > 0) && (((long)tmpbase & 0x3) != 0)) {
			i--;
			if (pb & umask)
				*tmpbase++ = fore_color_byte;
			else
				*tmpbase++ = back_color_byte;
			umask >>= 1;
		}

		for (tmp = 0; tmp < 8; tmp++) {
			if (umask == 0)
				umask = 0x80;
			if (pb & umask) {
				exp_pat[tmp] = fore_color_byte;
			}
			else {
				exp_pat[tmp] = back_color_byte;
			}

			umask >>= 1;
		}

		while (i >= 8) {
			write_long(tmpbase, as_long(exp_pat[0]));
			tmpbase += 4;
			write_long(tmpbase, as_long(exp_pat[4]));
			tmpbase += 4;
			i -= 8;
		}

		if (i >= 4) {
			write_long(tmpbase, as_long(exp_pat[0]));
			tmpbase += 4;
			i -= 4;
			tmp = as_long(exp_pat[0]);
			as_long(exp_pat[0]) = as_long(exp_pat[4]);
			as_long(exp_pat[4]) = tmp;
		}

		tmp = 0;

		while (i > 0) {
			i--;
			*tmpbase++ = exp_pat[tmp];
			tmp++;
		}

		y++;
	}
}

/*----------------------------------------------------------------------------*/

void rect_fill_copy_8(  port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_8_np(a_port, r, a_port->fore_color);
		return;
	}	
	if ((as_long(the_pattern->data[0]) == 0) &&
	    (as_long(the_pattern->data[4]) == 0)) {
		rect_fill_copy_8_np(a_port, r, a_port->back_color);
		return;
	}	

	rect_fill_copy_8_pat0(a_port, r, the_pattern, phase_x, phase_y);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_hilite_8_opt(port *a_port,
			 				rect *r)
{
	uchar		*base;		
	long		rowbyte;	
	long		y;	
	rect		tmprect;	
	long		x;	
	uchar		*tmpbase;	
	ulong		pb;	
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	a_color;
	long		i;
	long		tmp;
	uchar		*tmp_color;
	server_color_map	*a_cs;
	uchar		umask;
	uchar		old_color;
	ulong		tmp_long;
	ulong		tmp_out;

	fore_color = a_port->fore_color;
	back_color = a_port->back_color;

		
	last_blend_fcolor = fore_color;
	last_blend_bcolor = back_color;
	last_blend_mode = op_hilite;

	a_cs = a_port->port_cs;

	for (i = 0; i < 256; i++) {
		a_color = a_cs->color_list[i];
		blend_array1[i] = calc_hilite_8(a_cs, i, fore_color, back_color);
	}	

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	while (y <= tmprect.bottom) {
		tmpbase = base;
		base += rowbyte;

		i = x;

		tmp = 0;

		while (i>0 && ((long)tmpbase & 0x03)) {
			i--;
			old_color = *tmpbase;
			*tmpbase++ = blend_array1[old_color];
		}

		while (i >= 4) {
			i-=4;
			tmp_long = *((ulong *)tmpbase);
			
			tmp_out =   (blend_array1[tmp_long & 0xff])
					  | (blend_array1[(tmp_long>>8) & 0xff] << 8)
					  | (blend_array1[(tmp_long>>16) & 0xff] << 16)
					  | (blend_array1[(tmp_long>>24) & 0xff] << 24);


			*((ulong*)tmpbase) = tmp_out;
			tmpbase += 4;
		}

		while (i > 0) {
			i--;
			old_color = *tmpbase;
			*tmpbase++ = blend_array1[old_color];
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_other_8_opt(port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y,
			 short mode)
{
	uchar		*base;		
	long		rowbyte;	
	long		y;	
	rect		tmprect;	
	long		x;	
	uchar		*tmpbase;	
	ulong		pb;	
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	a_color;
	uchar		*exp_pat[8];
	long		i;
	long		tmp;
	uchar		*tmp_color;
	server_color_map	*a_cs;
	uchar		umask;
	uchar		old_color;

	fore_color = a_port->fore_color;
	back_color = a_port->back_color;

		
	if ((last_blend_mode != mode) ||
	    (as_long(last_blend_fcolor) != as_long(fore_color)) ||
	    (as_long(last_blend_bcolor) != as_long(back_color))) {
	last_blend_fcolor = fore_color;
	last_blend_bcolor = back_color;
	last_blend_mode = mode;

	a_cs = a_port->port_cs;

	switch(mode) {
		case op_add :
		for (i = 0; i < 256; i++) {
			a_color = a_cs->color_list[i];
			blend_array1[i] = calc_add_8(a_cs, a_color, fore_color);
			blend_array2[i] = calc_add_8(a_cs, a_color, back_color);
		}
		break;

		case op_sub :
		for (i = 0; i < 256; i++) {
			a_color = a_cs->color_list[i];
			blend_array1[i] = calc_sub_8(a_cs, a_color, fore_color);
			blend_array2[i] = calc_sub_8(a_cs, a_color, back_color);
		}
		break;

		case op_blend :
		for (i = 0; i < 256; i++) {
			a_color = a_cs->color_list[i];
			blend_array1[i] = calc_blend_8(a_cs, a_color, fore_color);
			blend_array2[i] = calc_blend_8(a_cs, a_color, back_color);
		}	
		break;	

		case op_min :
		for (i = 0; i < 256; i++) {
			a_color = a_cs->color_list[i];
			blend_array1[i] = calc_min_8(a_cs, a_color, fore_color);
			blend_array2[i] = calc_min_8(a_cs, a_color, back_color);
		}	
		break;	

		case op_max :
		for (i = 0; i < 256; i++) {
			a_color = a_cs->color_list[i];
			blend_array1[i] = calc_max_8(a_cs, a_color, fore_color);
			blend_array2[i] = calc_max_8(a_cs, a_color, back_color);
		}	
		break;	
		
		case op_hilite :
		break;
	}
	}

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;
	
	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);

		tmpbase = base;
		base += rowbyte;

		i = x;

		umask = 0x80;

		for (tmp = 0; tmp < 8; tmp++) {
			exp_pat[tmp] = (pb & umask) ? blend_array1 : blend_array2;
			umask >>= 1;
			if (umask == 0)
				umask = 0x80;
		}

		tmp = 0;

		while (i > 0) {
			i--;
			tmp_color = exp_pat[tmp & 7];
			old_color = *tmpbase;
			*tmpbase++ = *(tmp_color + old_color);
			tmp++;
		}

		y++;
	}
}


/*--------------------------------------------------------------------*/ 

void rect_fill_other_8(port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y,
			 short mode)
{
	uchar		*base;		
	long		rowbyte;	
	long		y;	
	rect		tmprect;	
	long		x;	
	uchar		*tmpbase;	
	ulong		pb;	
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	exp_pat[8];
	long		i;
	long		tmp;
	rgb_color	tmp_color;
	rgb_color	old_color;
	server_color_map	*a_cs;
	uchar		umask;

	if (((r->right - r->left) * (r->bottom - r->top)) > 3000) {
		rect_fill_other_8_opt(a_port, r, the_pattern,phase_x, phase_y, mode);
		return;
	}

	fore_color = a_port->fore_color;
	back_color = a_port->back_color;

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;
	
	while (y <= tmprect.bottom) {
		pb = the_pattern->data[(y + phase_y) & 7];
		pb = rol(pb, phase_x);

		tmpbase = base;
		base += rowbyte;

		i = x;

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
			old_color = a_cs->color_list[*tmpbase];
			switch(mode) {
				case op_add :
				*tmpbase = calc_add_8(a_cs, old_color, tmp_color);
				break;

				case op_sub :
				*tmpbase = calc_sub_8(a_cs, old_color, tmp_color);
				break;

				case op_blend :
				*tmpbase = calc_blend_8(a_cs, old_color, tmp_color);
				break;	

				case op_min :
				*tmpbase = calc_min_8(a_cs, old_color, tmp_color);
				break;	

				case op_max :
				*tmpbase = calc_max_8(a_cs, old_color, tmp_color);
				break;	
				
				case op_hilite :
				*tmpbase = calc_hilite_8(a_cs, *tmpbase, fore_color, back_color);
				break;
			}
			tmpbase++;	
			tmp++;
		}

		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_min_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect_fill_other_8(a_port, r, the_pattern, phase_x, phase_y, op_min);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_max_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect_fill_other_8(a_port, r, the_pattern, phase_x, phase_y, op_max);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_hilite_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect_fill_hilite_8_opt(a_port, r);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_add_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect_fill_other_8(a_port, r, the_pattern, phase_x, phase_y, op_add);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_sub_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect_fill_other_8(a_port, r, the_pattern, phase_x, phase_y, op_sub);
}

/*--------------------------------------------------------------------*/ 

void rect_fill_blend_8 (port *a_port,
			rect *r,
			pattern *the_pattern,
			long phase_x,
			long phase_y)
{
	rect_fill_other_8(a_port, r, the_pattern, phase_x, phase_y, op_blend);
}


void rect_fill_xor_8_np (port *a_port,
			 rect *r)
{
	uchar		*base;		
	long		rowbyte;	
	long		y;	
	rect		tmprect;	
	long		x;	
	uchar		*tmpbase;	
	uchar		*invert_map;
	long		i;
	ulong		reader;
	ulong		reader0;
	ulong		writer;
	ulong		all_black;
	ulong		all_white;
	rgb_color	tmp_color;
	uchar		tmp_byte;

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	invert_map = a_port->port_cs->invert_map;

	while (y <= tmprect.bottom) {

		tmpbase = base;
		base += rowbyte;

		i = x;
		
		if (i < 8) {
			while(i > 0) {
				i--;
				*tmpbase++ = invert_map[*tmpbase];
			}	
			goto skip;	
		}

		while(i > 0 && ((long)tmpbase & 0x03)) {
				i--;
				*tmpbase++ = invert_map[*tmpbase];
		}
		
		reader = *((long *)tmpbase);
		reader0= reader^0xff;		

		while (i > 4) {
			i -= 4;
			if (reader0 != reader) {
				writer  = (invert_map[reader >> 24] << 24);
				writer |= (invert_map[(reader >> 16) & 0xff] << 16);
				writer |= (invert_map[(reader >> 8) & 0xff] << 8);
				writer |= invert_map[reader & 0xff];
			}

			*((long *)tmpbase) = writer;
			tmpbase += 4;
			reader0 = reader;
			reader = *((long*)tmpbase);
		}

		while (i > 0) {
			i--;
			*tmpbase++ = invert_map[*tmpbase];
		}
skip:
		y++;
	}
}

/*--------------------------------------------------------------------*/ 

void rect_fill_xor_8     (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	x;	
	uchar	*tmpbase;	
	uchar	*invert_map;
	ulong	pb;	
	long	i;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_xor_8_np(a_port, r);
		return;
	}	

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

	y = tmprect.top;
	x = tmprect.right - tmprect.left;

	phase_x += tmprect.left;
	phase_x &= 7;

	invert_map = a_port->port_cs->invert_map;
	
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
					*tmpbase = invert_map[*tmpbase];
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

void rect_fill_or_8     (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	rect	tmprect;	
	uchar	*base;		
	uchar	*tmpbase;	
	long	rowbyte;	
	long	y;	
	long	x;	
	long	i;
	long	j;
	ulong	pb;	
	char	fore_color_byte;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_8_np(a_port, r, a_port->fore_color);
		return;
	}	
	
	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

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
			while (i > 7) {
				j = 4;
				while (j--) {
					if (pb & umask)
						*tmpbase = fore_color_byte;
					tmpbase++;
					umask >>= 1;

					if (pb & umask)
						*tmpbase = fore_color_byte;
					tmpbase++;
					umask >>= 1;
				}
				umask = 0x80;
				i -= 8;
			}

			while (i > 0) {
				i--;
				if (pb & umask)
					*tmpbase = fore_color_byte;
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

void rect_fill_and_8    (port *a_port,
			 rect *r,
			 pattern *the_pattern,
			 long phase_x,
			 long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	x;	
	uchar	*tmpbase;	
	ulong	pb;	
	long	i;
	long	j;
	uchar	back_color_byte;
	uchar	umask;

	if ((as_long(the_pattern->data[0]) == 0xffffffff) &&
	    (as_long(the_pattern->data[4]) == 0xffffffff)) {
		rect_fill_copy_8_np(a_port, r, a_port->back_color);
		return;
	}	
	
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);

	tmprect = *r;
	tmprect.right += 1;

	rowbyte = a_port->rowbyte;

	base = a_port->base + (tmprect.top * rowbyte) + tmprect.left;

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

			while (i > 7) {
				j = 4;
				while (j--) {
					if (pb & umask)
						*tmpbase = back_color_byte;
					tmpbase++;
					umask >>= 1;

					if (pb & umask)
						*tmpbase = back_color_byte;
					tmpbase++;
					umask >>= 1;
				}
				umask = 0x80;
				i -= 8;
			}
			
			while (i > 0) {
				i--;
				if (pb & umask)
					*tmpbase = back_color_byte;
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

void rect_fill_8(port *a_port, rect *r, pattern *the_pattern, long phase_x, long phase_y, short mode)
{
	switch(mode) {
		case	op_copy:
				rect_fill_copy_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_or:
				rect_fill_or_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_max :
				rect_fill_max_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_hilite :
				rect_fill_hilite_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_add :
				rect_fill_add_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_sub :
				rect_fill_sub_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_min :
				rect_fill_min_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_blend :
				rect_fill_blend_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_xor :
				rect_fill_xor_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_and :
				rect_fill_and_8(a_port, r, the_pattern, phase_x, phase_y);
				return;
	}
}

