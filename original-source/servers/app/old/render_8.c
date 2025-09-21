//******************************************************************************
//
//	File:		render.c
//
//	Description:	8 bit renderer & bitters.
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
#include "render_24.h"

/*----------------------------------------------------------------------------*/
extern void tri_blit(long, long, long, long, long, long);
extern char	tri_blit_1_to_8(port *from_port, rect *from, rect *to, short mode);
extern short VGALine(short, long, long, long, long, uchar, uchar, uchar, short, short, short, short, short);
extern short VGARect(short, long, long, long, long, uchar, uchar, uchar);
/*----------------------------------------------------------------------------*/

#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))

/*----------------------------------------------------------------------------*/

#define	PUT_LONG(ptr, what)		\
		*((ulong *)ptr) = what;	\
		ptr += sizeof(ulong);

#define	PUT_SHORT(ptr, what)	\
		*((ushort *)ptr) = what;	\
		ptr += sizeof(ushort);

/*----------------------------------------------------------------------------*/

static long	sqr(long v)
{
	return(v * v);
}

/*----------------------------------------------------------------------------*/

uchar	color_mapper(rgb_color a_color, server_color_map *a_clut)
{
	rgb_color	color1;
	long		error;
	long		best_error;
	long		best_i;
	long		i;
	long		lum_1;
	long		lum_2;

	best_error = 10000000;

	lum_1 = a_color.red + a_color.green + (a_color.blue >> 1);

	for (i = 0; i <= 254; i++) {
		color1 = a_clut->color_list[i];
		lum_2 = a_color.red + a_color.green + (a_color.blue >> 1);


		error = 3 * sqr(lum_1 - lum_2);

		error += sqr(a_color.red - color1.red);
		if (error < best_error) {
			error += (sqr((a_color.blue - color1.blue) >> 1));
			
			if (error < best_error) {
				error += sqr(a_color.green - color1.green);
				if (error < best_error) {
					best_error = error;
					if (error == 0)
						return(i);
					best_i = i;
				}
			}
		}
	}	
	return(best_i);
}

/*----------------------------------------------------------------------------*/

long	find_unused(char *used, long client_color, server_color_map *the_cs)
{
	rgb_color	c;
	rgb_color	c1;
	long		index;
	long		min_error = 10000000;
	long		best = -1;
	long		error;

	c = the_cs->color_list[client_color];
	c.red = 255 - c.red;
	c.green = 255 - c.green;
	c.blue = 255 - c.blue;

	for (index = 0; index < 256; index++) {
		if ((!used[index]) && (index != client_color)) {
			c1 = the_cs->color_list[index];
			error = sqr(c1.red - c.red);
			error += sqr(c1.green - c.green);
			error += sqr(c1.blue - c.blue) >> 1;
			if (error < min_error) {
				min_error = error;
				best = index;
			}
		}
	}
	if (best < 0)
		best = client_color;
	return(best);
}

/*----------------------------------------------------------------------------*/

void	create_inverted(server_color_map *the_cs)
{
	rgb_color	c;
	long		index;
	long		red;
	long		green;
	long		blue;
	uchar		best;
	char		used[256];
	

	for (index = 0; index < 256; index++)
		used[index] = 0;
	
//!!! the following lines assumes that the first and last entries
// of the color map are black and white or the opposit.
	
	used[0] = 1;
	used[255] = 1;
	the_cs->invert_map[0] = 255;
	the_cs->invert_map[255] = 0;

	for (index = 1; index < 255; index++) {
		if (!used[index]) {
			c = the_cs->color_list[index];
			c.red = 255 - c.red;
			c.green = 255 - c.green;
			c.blue = 255 - c.blue;
			best = color_mapper(c, the_cs);
			if (used[best]) {
				best = find_unused(used, index, the_cs);
			}
			the_cs->invert_map[index] = best;
			the_cs->invert_map[best] = index;
			used[index] = 1;
			used[best] = 1;
		}
	}

	for (red   = 0; red   < 256; red += 8)
	for (green = 0; green < 256; green += 8)
	for (blue  = 0; blue  < 256; blue += 8) {

		c.red = red;
		c.green = green;
		c.blue = blue;

		best = color_mapper(c, the_cs);
		index = (((c.red & 0xf8 ) << 7) |
		 		((c.green & 0xf8) << 2) |
		 		((c.blue & 0xf8 ) >> 3));

		the_cs->inverted[index] = best;
	}
}

/*----------------------------------------------------------------------------*/

uchar	color_2_index(register server_color_map *a_cs, rgb_color c)
{
	long	index;
	

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	index = ((((as_long(c) << 7) & (0xf8 << 7))) |
		 	(((as_long(c) >> 6) & (0xf8 << 2))) |
		 	(((as_long(c) >> 19) & (0xf8 >> 3))));
#else
	index = ((((as_long(c) >> 17) & (0xf8 << 7))) |
		 	(((as_long(c) >> 14) & (0xf8 << 2))) |
		 	(((as_long(c) >> 11) & (0xf8 >> 3))));
#endif
	if ((as_long(c)) == TRANSPARENT_MAGIC) {
		return(TRANSPARENT_INDEX);
	}

	return(a_cs->inverted[index]);
}

/*----------------------------------------------------------------------------*/

uchar calc_hilite_8(server_color_map *a_cs, uchar old_color, rgb_color fore_color, rgb_color back_color)
{
	uchar	back_as_byte;
	uchar	fore_as_byte;

	fore_as_byte = color_2_index(a_cs, fore_color);
	back_as_byte = color_2_index(a_cs, back_color);
	if (fore_as_byte == old_color)
		return back_as_byte;
	else {
		if (back_as_byte == old_color)
			return fore_as_byte;
	}
	return old_color;
}

/*----------------------------------------------------------------------------*/

uchar calc_max_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.red + c2.green + c2.blue;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c1));
	else
		return(color_2_index(a_cs, c2));
}

// THIS IS A VERSION FOR SWAPED CHANNELS !!

uchar calc_max_8_x(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.blue + c2.green + c2.red;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c1));
	else
		return(color_2_index(a_cs, c2));
}

/*--------------------------------------------------------------------*/ 
// for macintosh.
// c1 is in rgba format.
// c2 is in argb format.
// -------- rgba

uchar calc_max_8_y(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.green + c2.blue + c2.alpha;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c1));
	else
		return(color_2_index(a_cs, c2));
}

/*--------------------------------------------------------------------*/ 

uchar calc_min_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.red + c2.green + c2.blue;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c2));
	else
		return(color_2_index(a_cs, c1));

}

// THIS IS A VERSION FOR SWAPED CHANNELS !!

uchar calc_min_8_x(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.blue + c2.green + c2.red;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c2));
	else
		return(color_2_index(a_cs, c1));

}

// for macintosh.
// c1 is in rgba format.
// c2 is in argb format.
// -------- rgba

uchar calc_min_8_y(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	long	level1;
	long	level2;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	level1 = c1.red + c1.green + c1.blue;
	level2 = c2.green + c2.blue + c2.alpha;
		
	if (level1 > level2) 
		return(color_2_index(a_cs, c2));
	else
		return(color_2_index(a_cs, c1));

}
/*--------------------------------------------------------------------*/ 

uchar calc_blend1_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;

	col.red   = (c1.red + c2.red) >> 1;
	col.green = (c1.green + c2.green) >> 1;
	col.blue  = (c1.blue + c2.blue) >> 1;

	return(color_2_index(a_cs, col));
}

/*--------------------------------------------------------------------*/ 

uchar calc_blend_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	col.red   = (c1.red + c2.red) >> 1;
	col.green = (c1.green + c2.green) >> 1;
	col.blue  = (c1.blue + c2.blue) >> 1;

	return(color_2_index(a_cs, col));
}

// THIS IS A VERSION FOR SWAPED CHANNELS !!

uchar calc_blend_8_x(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	col.red   = (c1.red + c2.blue) >> 1;
	col.green = (c1.green + c2.green) >> 1;
	col.blue  = (c1.blue + c2.red) >> 1;

	return(color_2_index(a_cs, col));
}

// for macintosh.
// c1 is in rgba format.
// c2 is in argb format.
// -------- rgba
// red of c1 is green of c2
// green of c1 is blue of c2
// blue of c1 is alpha of c2

uchar calc_blend_8_y(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);

	col.red   = (c1.red + c2.green) >> 1;
	col.green = (c1.green + c2.blue) >> 1;
	col.blue  = (c1.blue + c2.alpha) >> 1;

	return(color_2_index(a_cs, col));
}

/*--------------------------------------------------------------------*/ 

uchar calc_add_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
		
	tmp   = (c1.red + c2.red);
	if (tmp > 255)
		tmp = 255;
	col.red = tmp;

	tmp   = (c1.green + c2.green);
	if (tmp > 255)
		tmp = 255;
	col.green = tmp;

	tmp   = (c1.blue + c2.blue);
	if (tmp > 255)
		tmp = 255;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}

// THIS IS A VERSION FOR SWAPED CHANNELS !!

uchar calc_add_8_x(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
		
	tmp   = (c1.red + c2.blue);
	if (tmp > 255)
		tmp = 255;
	col.red = tmp;

	tmp   = (c1.green + c2.green);
	if (tmp > 255)
		tmp = 255;
	col.green = tmp;

	tmp   = (c1.blue + c2.red);
	if (tmp > 255)
		tmp = 255;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}

/*--------------------------------------------------------------------*/ 
// red of c1 is green of c2
// green of c1 is blue of c2
// blue of c1 is alpha of c2

uchar calc_add_8_y(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
		
	tmp   = (c1.red + c2.green);
	if (tmp > 255)
		tmp = 255;
	col.red = tmp;

	tmp   = (c1.green + c2.blue);
	if (tmp > 255)
		tmp = 255;
	col.green = tmp;

	tmp   = (c1.blue + c2.alpha);
	if (tmp > 255)
		tmp = 255;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}

/*--------------------------------------------------------------------*/ 
/* will sub c2 from c1 */

uchar calc_sub_8(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
	tmp   = (c1.red - c2.red);
	if (tmp < 0)
		tmp = 0;
	col.red = tmp;

	tmp   = (c1.green - c2.green);
	if (tmp < 0)
		tmp = 0;
	col.green = tmp;

	tmp   = (c1.blue - c2.blue);
	if (tmp < 0)
		tmp = 0;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}


// THIS IS A VERSION FOR SWAPED CHANNELS !!

uchar calc_sub_8_x(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
	tmp   = (c1.red - c2.blue);
	if (tmp < 0)
		tmp = 0;
	col.red = tmp;

	tmp   = (c1.green - c2.green);
	if (tmp < 0)
		tmp = 0;
	col.green = tmp;

	tmp   = (c1.blue - c2.red);
	if (tmp < 0)
		tmp = 0;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}

// red of c1 is green of c2
// green of c1 is blue of c2
// blue of c1 is alpha of c2

uchar calc_sub_8_y(server_color_map *a_cs, rgb_color c1, rgb_color c2)
{
	rgb_color	col;
	long		tmp;

	if ((as_long(c1)) == TRANSPARENT_MAGIC)
		return(TRANSPARENT_INDEX);
	
	tmp   = (c1.red - c2.green);
	if (tmp < 0)
		tmp = 0;
	col.red = tmp;

	tmp   = (c1.green - c2.blue);
	if (tmp < 0)
		tmp = 0;
	col.green = tmp;

	tmp   = (c1.blue - c2.alpha);
	if (tmp < 0)
		tmp = 0;
	col.blue = tmp;

	return(color_2_index(a_cs, col));
}

/*--------------------------------------------------------------------*/ 

void pixel_xor_8(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	*invert_map;
	
	base = a_port->base + (y * a_port->rowbyte) + x;
	invert_map = a_port->port_cs->invert_map;
	*base = invert_map[*base];
}

/*--------------------------------------------------------------------*/ 

void pixel_and_8(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	back_color_byte;
	
	back_color_byte = color_2_index(a_port->port_cs, a_port->back_color);
	
	base = a_port->base + (y * a_port->rowbyte) + x;
	*base = back_color_byte;
}

/*--------------------------------------------------------------------*/ 

void pixel_copy_8(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	fore_color_byte;
	
	fore_color_byte = color_2_index(a_port->port_cs, a_port->fore_color);
	
	base = a_port->base + (y * a_port->rowbyte) + x;
	*base = fore_color_byte;
}

/*--------------------------------------------------------------------*/ 

void	pixel_add_8(port *a_port, long x, long y)
{
	pixel_other_8(a_port, x, y, op_add);
}

/*--------------------------------------------------------------------*/ 

void	pixel_sub_8(port *a_port, long x, long y)
{
	pixel_other_8(a_port, x, y, op_sub);
}

/*--------------------------------------------------------------------*/ 

void	pixel_min_8(port *a_port, long x, long y)
{
	pixel_other_8(a_port, x, y, op_min);
}

/*--------------------------------------------------------------------*/ 

void	pixel_max_8(port *a_port, long x, long y)
{
	pixel_other_8(a_port, x, y, op_max);
}

/*--------------------------------------------------------------------*/ 

void	pixel_blend_8(port *a_port, long x, long y)
{
	pixel_other_8(a_port, x, y, op_blend);
}

/*--------------------------------------------------------------------*/ 

void	pixel_other_8(port *a_port, long x, long y, short mode)
{
	uchar		*base;
	rgb_color	old_color;
	
	base = a_port->base + (y * a_port->rowbyte) + x;
	old_color = a_port->port_cs->color_list[*base];

	switch(mode) {
		case op_add :
			*base = calc_add_8(a_port->port_cs, old_color, a_port->fore_color);
			return;
		case op_sub :
			*base = calc_sub_8(a_port->port_cs, old_color, a_port->fore_color);
			return;
		case op_blend :
			*base = calc_blend_8(a_port->port_cs, old_color, a_port->fore_color);
			return;	
		case op_min :
			*base = calc_min_8(a_port->port_cs, old_color, a_port->fore_color);
			return;	
		case op_max :
			*base = calc_max_8(a_port->port_cs, old_color, a_port->fore_color);
			return;	
		case op_hilite :
			*base = calc_hilite_8(a_port->port_cs, *base, a_port->fore_color, a_port->back_color);
			return;
	}	
}

/*--------------------------------------------------------------------*/ 


void	copy_other_1to8(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode)
{
	long		y;
	uchar		*dst_base;
	uchar		*src_base;
	uchar		*tmp_base;
	long		src_rowbyte;
	long		dst_rowbyte;
	long		dx;
	rgb_color	fore_color;
	rgb_color	back_color;
	rgb_color	color;
	rgb_color	old_color;
	server_color_map	*a_cs;
	uchar		a_bw_byte;
	uchar		mask;
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	fore_color = dst_port->fore_color;
	back_color = dst_port->back_color;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	a_cs = dst_port->port_cs;

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = (dst_port->base) + (y * dst_rowbyte) + to->left;
		dx = from->left;
		tmp_base = (src_base + (dx >> 3));
		a_bw_byte = *tmp_base++;

		dx = dx & 7;
		
		mask = 0x80 >> dx;
		
		dx = to->right - to->left;
		
		while (dx >= 0) {
			dx--;

			if (a_bw_byte & mask)
				color = fore_color;
			else	
				color = back_color;
			
			old_color = a_cs->color_list[*dst_base];

			switch(mode) {
				case op_add :
					*dst_base++ = calc_add_8(a_cs, old_color, color);
					break;

				case op_sub :
					*dst_base++ = calc_sub_8(a_cs, old_color, color);
					break;

				case op_blend :
					*dst_base++ = calc_blend_8(a_cs, old_color, color);
					break;

				case op_min :
					*dst_base++ = calc_min_8(a_cs, old_color, color);
					break;

				case op_max :
					*dst_base++ = calc_max_8(a_cs, old_color, color);
					break;

				case op_hilite :
					*dst_base++ = calc_hilite_8(a_cs, *dst_base, fore_color, back_color);
					break;
			}	
			mask >>= 1;

			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}
		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_1to81or(port *from_port,
			port *dst_port,
			rect *from,
			rect *to)
{
	long	y;
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	uchar	*dst_base1;
	uchar	*dst_base2;
	long	src_rowbyte;
	long	dst_rowbyte;
	long	dx;
	long	dx1;
	long	x1;
	uchar	a_bw_byte;
	uchar	mask;
	uchar	fore_color_byte;
	uchar	back_color_byte;

	fore_color_byte = color_2_index(dst_port->port_cs, dst_port->fore_color);
	back_color_byte = color_2_index(dst_port->port_cs, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	dst_base2 = (dst_port->base) + (dst_rowbyte * to->top) + to->left;
	dx1 = to->right - to->left;

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = dst_base2;
		dst_base2 += dst_rowbyte;

		tmp_base = (src_base + (dx >> 3));
		a_bw_byte = *tmp_base++;

		mask = 0x80 >> (dx & 7);
		
		dst_base1 = dst_base + dx1 + 1;

		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base = fore_color_byte;
			
			dst_base++;

			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_1to81or_s  (port *from_port,
							   port *dst_port,
							   rect *from,
							   rect *to)
{
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*dst_base2;
	short	y;
	short	src_rowbyte;
	short	dst_rowbyte;
	short	dx;
	short	dx1;
	ushort	a_bw_word;
	ushort	mask;
	ushort	mask1;
	uchar	fore_color_byte;

	fore_color_byte = color_2_index(dst_port->port_cs, dst_port->fore_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	src_rowbyte--;	

	dst_base2 = (dst_port->base) + (dst_rowbyte * to->top) + to->left;
	dx1 = to->right - to->left;
	dx1++;	

	dx = from->left;
	src_base += dx >> 3;	
	dx &= 0x07;
	mask1 = (0xffff >> (dx1)) ^ 0xffff;

	y = to->bottom-to->top+1;
	mask = 0x8000;
	
	while(y--) {
		dst_base = dst_base2;

		a_bw_word = *src_base++ << 8;
		dst_base2 += dst_rowbyte;
		a_bw_word |= *src_base;
		a_bw_word <<= dx;

		a_bw_word &= mask1;
		
		while (a_bw_word) {
			if (a_bw_word & mask)
				*dst_base = fore_color_byte;
			
			dst_base++;
			a_bw_word <<=1;
		}
skip:;
		src_base += src_rowbyte;		//note that rowbyte was post dec once at the start of the routine		//note that rowbyte was post dec once at the start of the routine
	}
}


/*--------------------------------------------------------------------*/ 

void	copy_block_1to81copy(port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to)
{
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	uchar	*dst_base1;
	uchar	*dst_base2;
	short	y;
	short	src_rowbyte;
	short	dst_rowbyte;
	short	dx;
	short	dx1;
	uchar	a_bw_byte;
	uchar	mask;
	uchar	mask0;
	uchar	fore_color_byte;
	uchar	back_color_byte;

	fore_color_byte = color_2_index(dst_port->port_cs, dst_port->fore_color);
	back_color_byte = color_2_index(dst_port->port_cs, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	dst_base2 = (dst_port->base) + (dst_rowbyte * to->top) + to->left;
	dx1 = to->right - to->left;

	dx = from->left;
	mask0 = 0x80 >> (dx & 0x07);
	src_base += dx >> 3;	

	for (y = to->top; y <= to->bottom; y++) {
		dst_base = dst_base2;
		dst_base2 += dst_rowbyte;

		tmp_base = src_base;
		a_bw_byte = *tmp_base++;

		mask = mask0;
		
		dst_base1 = dst_base + dx1 + 1;

		while (dst_base1 != dst_base) {
			if (a_bw_byte & mask)
				*dst_base++ = fore_color_byte;
			else	
				*dst_base++ = back_color_byte;

			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_1to81(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode)
{
	long	y;
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	uchar	*dst_base1;
	long	src_rowbyte;
	long	dst_rowbyte;
	long	dx;
	uchar	*invert_map;
	uchar	a_bw_byte;
	uchar	mask;
	uchar	fore_color_byte;
	uchar	back_color_byte;

	fore_color_byte = color_2_index(dst_port->port_cs, dst_port->fore_color);
	back_color_byte = color_2_index(dst_port->port_cs, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = (dst_port->base) + (y * dst_rowbyte) + to->left;
		dx = from->left;
		tmp_base = (src_base + (dx >> 3));
		a_bw_byte = *tmp_base++;

		dx = dx & 7;
		
		mask = 0x80 >> dx;
		
		dx = to->right - to->left;
		
		dst_base1 = dst_base + dx + 1;

		if (mode == op_copy)
		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base++ = fore_color_byte;
			else	
				*dst_base++ = back_color_byte;
			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		if (mode == op_or)
		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base = fore_color_byte;
			
			dst_base++;

			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		if (mode == op_and)
		while (dst_base1 != dst_base) {

			if ((a_bw_byte & mask) == 0)
				*dst_base = back_color_byte;

			dst_base++;
			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		invert_map = dst_port->port_cs->invert_map;

		if (mode == op_xor)
		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base = invert_map[*dst_base];
			
			dst_base++;

			mask >>= 1;

			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}


		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_1to8(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode)
{
	long	y;
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	long	src_rowbyte;
	long	dst_rowbyte;
	long	dx;
	uchar	*invert_map;
	ulong	tmp_color_0;
	ulong	tmp_color_1;
	uchar	fore_color_byte;
	uchar	back_color_byte;
	uchar	a_bw_byte;
	uchar	mask;

	fore_color_byte = color_2_index(dst_port->port_cs, dst_port->fore_color);
	back_color_byte = color_2_index(dst_port->port_cs, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	tmp_color_0 	= (back_color_byte << 24) |
			  (back_color_byte << 16) |
			  (back_color_byte << 8)  |
			  (back_color_byte);

	tmp_color_1	= (fore_color_byte << 24) |
			  (fore_color_byte << 16) |
			  (fore_color_byte << 8)  |
			  (fore_color_byte);

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = (dst_port->base) + (y * dst_rowbyte) + to->left;
		dx = from->left;
		tmp_base = (src_base + (dx >> 3));
		a_bw_byte = *tmp_base++;

		dx = dx & 7;
		
		mask = 0x80 >> dx;
		
		dx = to->right - to->left;
		
		if (mode == op_copy)
		while (dx >= 0) {
			dx--;

			if (a_bw_byte & mask)
				*dst_base++ = fore_color_byte;
			else	
				*dst_base++ = back_color_byte;

			mask >>= 1;

			if (mask == 0) {
loop_copy:
				mask = 0x80;
				a_bw_byte = *tmp_base++;
				if (a_bw_byte == 0x00) {
					if (dx > 8) {
						dx -= 8;
						switch((long)dst_base & 0x03) {
						case 0x00:
							PUT_LONG(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							break;
						case 0x01:
							*dst_base++ = tmp_color_0;
							PUT_SHORT(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							*dst_base++ = tmp_color_0;
							break;
						case 0x02:
							PUT_SHORT(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							PUT_SHORT(dst_base, tmp_color_0);
							break;
						case 0x03:
							*dst_base++ = tmp_color_0;
							PUT_LONG(dst_base, tmp_color_0);
							PUT_SHORT(dst_base, tmp_color_0);
							*dst_base++ = tmp_color_0;
							break;
						}
						goto loop_copy;
					}
				}
				else
				if (a_bw_byte == 0xff) {
					if (dx > 8) {
						dx -= 8;
						switch((long)dst_base & 0x03) {
						case 0x00:
							PUT_LONG(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							break;
						case 0x01:
							*dst_base++ = tmp_color_1;
							PUT_SHORT(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							*dst_base++ = tmp_color_1;
							break;
						case 0x02:
							PUT_SHORT(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							PUT_SHORT(dst_base, tmp_color_1);
							break;
						case 0x03:
							*dst_base++ = tmp_color_1;
							PUT_LONG(dst_base, tmp_color_1);
							PUT_SHORT(dst_base, tmp_color_1);
							*dst_base++ = tmp_color_1;
							break;
						}
						goto loop_copy;
					}
				}
			}
		}

		if (mode == op_or)
		while (dx >= 0) {
			dx--;

			if (a_bw_byte & mask)
				*dst_base = fore_color_byte;
			
			dst_base++;

			mask >>= 1;

			if (mask == 0) {
loop_or:
				mask = 0x80;
				a_bw_byte = *tmp_base++;
				if (a_bw_byte == 0x00) {
					if (dx > 8) {
						dx -= 8;
						dst_base += 8;
						goto loop_or;
					}
				}
				else
				if (a_bw_byte == 0xff) {
					if (dx > 8) {
						dx -= 8;
						switch((long)dst_base & 0x03) {
						case 0x00:
							PUT_LONG(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							break;
						case 0x01:
							*dst_base++ = tmp_color_1;
							PUT_SHORT(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							*dst_base++ = tmp_color_1;
							break;
						case 0x02:
							PUT_SHORT(dst_base, tmp_color_1);
							PUT_LONG(dst_base, tmp_color_1);
							PUT_SHORT(dst_base, tmp_color_1);
							break;
						case 0x03:
							*dst_base++ = tmp_color_1;
							PUT_LONG(dst_base, tmp_color_1);
							PUT_SHORT(dst_base, tmp_color_1);
							*dst_base++ = tmp_color_1;
							break;
						}
						goto loop_or;
					}
				}
				
			}
		}

		if (mode == op_and)
		while (dx >= 0) {
			dx--;

			if ((a_bw_byte & mask) == 0)
				*dst_base = back_color_byte;

			dst_base++;

			mask >>= 1;

			if (mask == 0) {
loop_and:
				mask = 0x80;
				a_bw_byte = *tmp_base++;
				if (a_bw_byte == 0xff) {
					if (dx > 8) {
						dx -= 8;
						dst_base += 8;
						goto loop_and;
					}
				}
				else
				if (a_bw_byte == 0x00) {
					if (dx > 8) {
						dx -= 8;
						switch((long)dst_base & 0x03) {
						case 0x00:
							PUT_LONG(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							break;
						case 0x01:
							*dst_base++ = tmp_color_0;
							PUT_SHORT(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							*dst_base++ = tmp_color_0;
							break;
						case 0x02:
							PUT_SHORT(dst_base, tmp_color_0);
							PUT_LONG(dst_base, tmp_color_0);
							PUT_SHORT(dst_base, tmp_color_0);
							break;
						case 0x03:
							*dst_base++ = tmp_color_0;
							PUT_LONG(dst_base, tmp_color_0);
							PUT_SHORT(dst_base, tmp_color_0);
							*dst_base++ = tmp_color_0;
							break;
						}
						goto loop_and;
					}
				}
				
			}
		}

		invert_map = dst_port->port_cs->invert_map;

		if (mode == op_xor)
		while (dx >= 0) {
			dx--;

			if (a_bw_byte & mask)
				*dst_base = invert_map[*dst_base];
			
			dst_base++;

			mask >>= 1;

			if (mask == 0) {
loop_xor:
				mask = 0x80;
				a_bw_byte = *tmp_base++;
				if (a_bw_byte == 0x00) {
					if (dx > 8) {
						dx -= 8;
						dst_base += 8;
						goto loop_xor;
					}
				}
				else
				if (a_bw_byte == 0xff) {
					if (dx > 8) {
						dx -= 8;
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						*dst_base++ = invert_map[*dst_base];
						goto loop_xor;
					}
				}
			}
		}


		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 
/*
typedef union {
		uchar	as_char[4];
		ushort	as_short[2];
		ulong	as_long;
} trick;
*/
/*--------------------------------------------------------------------*/ 

typedef union {
		ulong	as_long[2];
		double	as_double;
} trick1;

/*--------------------------------------------------------------------*/ 


void	blit_to_screen_8   (port *src_port,
			 			 	port *dst_port,
			 			 	rect *src_rect,
			 			 	rect *dst_rect)
{
	uchar	*s_base;
	uchar	*d_base;
	uchar	*tmp_s_base;
	uchar	*tmp_d_base;
	long	dx;
	long	cdx;
	long	dy;
	trick1	t1;
	trick1	t2;

	s_base = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	d_base = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		tmp_s_base = s_base;
		tmp_d_base = d_base;
		
		s_base += src_port->rowbyte;
		d_base += dst_port->rowbyte;
		
		cdx = dx;
		
		if ((cdx > 0) && ((long)tmp_d_base & 0x01)) {
			cdx--;
			*tmp_d_base++ = *tmp_s_base++;
		}
		
		while ((cdx > 2) && ((long)tmp_d_base & 0x07)) {
			cdx-=2;
			*((short*)tmp_d_base) = *((short*)tmp_s_base);
			tmp_d_base += 2;
			tmp_s_base += 2;
		}

		while(cdx >= 16) {
			t1.as_long[0] = *((long *)tmp_s_base);
			tmp_s_base += 4;
			t1.as_long[1] = *((long *)tmp_s_base);
			tmp_s_base += 4;
			t2.as_long[0] = *((long *)tmp_s_base);
			tmp_s_base += 4;
			t2.as_long[1] = *((long *)tmp_s_base);
			tmp_s_base += 4;
			*((double *)tmp_d_base) = t1.as_double;
			tmp_d_base += 8;
			*((double *)tmp_d_base) = t2.as_double;
			tmp_d_base += 8;
			cdx -= 16;
		}

		while (cdx >= 4) {
			cdx-=4;
			*((long*)tmp_d_base) = *((long*)tmp_s_base);
			tmp_d_base += 4;
			tmp_s_base += 4;
		}

		if (cdx >= 2) {
			cdx-=2;
			*((short*)tmp_d_base) = *((short*)tmp_s_base);
			tmp_d_base += 2;
			tmp_s_base += 2;
		}
		if (cdx)
			*tmp_d_base = *tmp_s_base;
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_to_slow_8(port *src_port,
			 			 port *dst_port,
			 			 rect *src_rect,
			 			 rect *dst_rect)
{
	uchar	*s_base;
	uchar	*d_base;
	uchar	*tmp_s_base;
	uchar	*tmp_d_base;
	ushort	s1, s2;
	ulong	tmp;
	ulong	tmp1;
	long	dx;
	long	cdx;
	long	dy;
	long	shift;
	//trick	acc;

	//if (debug_modif & LEFT_SHIFT_KEY) {
		//blit_to_screen_fast(src_port, dst_port, src_rect, dst_rect);
		//return;
	//}
	s_base = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	d_base = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		tmp_s_base = s_base;
		tmp_d_base = d_base;
		
		s_base += src_port->rowbyte;
		d_base += dst_port->rowbyte;
		
		cdx = dx;
		
		while ((cdx > 0) && ((long)tmp_d_base & 0x03)) {
			cdx--;
			*tmp_d_base++ = *tmp_s_base++;
		}


		if (cdx > 4)
		if ((long)tmp_s_base & 0x01) {
			shift = (long)tmp_s_base & 0x03;
			if (shift == 1) {
				tmp_s_base--;

				tmp  = *(ulong *)tmp_s_base;
				tmp_s_base += 4;
				while(cdx > 4) {
					tmp1 = *((ulong *)tmp_s_base);
					tmp_s_base += 4;
					*((ulong  *)tmp_d_base) = (tmp << 8) | (tmp1 >> 24);
					tmp_d_base += 4;
					tmp = tmp1;
					cdx -= 4;
				}
				tmp_s_base -= 3;
			}
			else {
				tmp_s_base -= 3;
				tmp  = *((ulong *)tmp_s_base);
				tmp_s_base += 4;
				while(cdx > 4) {
					tmp1 = *((ulong *)tmp_s_base);
					tmp_s_base += 4;
					*((ulong  *)tmp_d_base) = (tmp << 24) | (tmp1 >> 8);
					tmp_d_base += 4;
					tmp = tmp1;
					cdx -= 4;
				}
				tmp_s_base -= 1;
			}
		}
		else {
			if (((long)tmp_s_base & 0x03) == 0) {
				while(cdx > 8) {
					*(ulong *)tmp_d_base = *((ulong *)tmp_s_base);
					tmp_s_base += 4;
					tmp_d_base += 4;
					*(ulong *)tmp_d_base = *((ulong *)tmp_s_base);
					tmp_s_base += 4;
					tmp_d_base += 4;
					cdx -= 8;
				}
				while(cdx > 4) {
					*(ulong *)tmp_d_base = *((ulong *)tmp_s_base);
					tmp_s_base += 4;
					tmp_d_base += 4;
					cdx -= 4;
				}
			}
			else {
				while(cdx > 4) {
					s1 = *((ushort *)tmp_s_base);
					tmp_s_base += 2;
					s2 = *((ushort *)tmp_s_base);
					tmp_s_base += 2;
					cdx -= 4;
					*(ulong *)tmp_d_base = (s1 << 16) | s2;
					tmp_d_base += 4;
				}
			}

			while(cdx > 2) {
				cdx -= 2;
				*(ushort *)tmp_d_base = *((ushort *)tmp_s_base);
				tmp_s_base += 2;
				tmp_d_base += 2;
			}
		}

		while(cdx) {
			cdx--;
			*tmp_d_base++ = *tmp_s_base++;
		}
	}
}


/*--------------------------------------------------------------------*/ 

void	blit_copy_s_eq_d_8(port *src_port,
		 			 	   port *dst_port,
		 			 	   rect *source_rect,
		 			 	   rect *dest_rect)
{
	double	tmp_double;
	ulong	*tmp_buffer;
	ulong	*tmp_buffer_cur;
	ulong	*saved_copy;
	uchar	*s_base;
	uchar	*tmp_base;
	long	wrk_left;
	long	wrk_right;
	long	wrk_top;
	long	wrk_bottom;
	ulong	acc1;
	ulong	acc2;
	trick1	a_trick;
	long	x;
	long	dx;
	long	x_cpt;
	short	y;
	short	dy;
	short	shift0;
	short	shift1;
	short	dshift;
	short	s1;
	short	s2;

	if ((src_port->base == screen_pointer) &&
	    (dst_port->base == screen_pointer)) {
		if (blit_jmp) {
			(blit_jmp)(source_rect->left, source_rect->top, dest_rect->left, dest_rect->top, source_rect->right - source_rect->left, source_rect->bottom - source_rect->top);
			(synchro_jmp)();
			return;
		}
	}


	if ((src_port->base != dst_port->base)/* && (dst_port->base == screen_pointer)*/) {
		blit_to_screen_8(src_port,
			       	     dst_port,
			       	     source_rect,
			       	     dest_rect);
		return;
	}


	wrk_left  = source_rect->left;
	wrk_right = source_rect->right + 1;
	wrk_left &= ~3;
	wrk_right = (wrk_right + 3) & ~3;

	wrk_top = source_rect->top;
	wrk_bottom = source_rect->bottom;
	dy = wrk_bottom - wrk_top + 1;
	dx = (wrk_right - wrk_left) >> 2;
	dx++;

	tmp_buffer = (ulong *)gr_malloc((4 + wrk_right - wrk_left) * dy);
	saved_copy = tmp_buffer;

	s_base = src_port->base + (wrk_top * src_port->rowbyte) + wrk_left;
	
	shift0 = source_rect->left & 0x03;
	shift1 = dest_rect->left & 0x03;

	dshift = shift0 - shift1;

	tmp_buffer_cur = saved_copy;


	if (dshift == 0) 
	for (y = wrk_top; y <= wrk_bottom; y++) {
		tmp_base = s_base;
		tmp_buffer = tmp_buffer_cur;

		x = dx;

		if ((x != 0) && ( ((long)tmp_base & 0x07) != 0)) {
			read_long(tmp_base, acc1);
			tmp_base += 4;
			*tmp_buffer++ = acc1;
			x--;
		}

		while (x > 2) {
			read_double(tmp_base, tmp_double);
			tmp_base += 8;
			write_double(tmp_buffer, tmp_double);
			x -= 2;
			tmp_buffer += 2;
		}

		if (x > 0) {
			x--;
			read_long(tmp_base, acc1);
			tmp_base += 4;
			*tmp_buffer++ = acc1;
		}

		s_base += src_port->rowbyte;
		tmp_buffer_cur += dx;
	}	
	else
	if (dshift > 0) {
		s1 = dshift * 8;
		//s2 = dshift * 24;
		s2 = 32 - s1;

		for (y = wrk_top; y <= wrk_bottom; y++) {
			tmp_base = s_base;
			tmp_buffer = tmp_buffer_cur;
			read_long(tmp_base, acc2);		//acc2 = 1234
			tmp_base += 4;
			for (x = 1; x < dx; x++) {
				read_long(tmp_base, acc1);	//acc1 = 5678
				tmp_base += 4;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				acc2 >>= s1;				//acc2 = 234x
				acc2 |= (acc1 << s2);		//acc2 = 2345
#else
				acc2 <<= s1;				//acc2 = 234x
				acc2 |= (acc1 >> s2);		//acc2 = 2345
#endif
				*tmp_buffer++ = acc2;
				acc2 = acc1;				//acc2 = 5678
			}
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
			*tmp_buffer = acc2 >> s1;
#else
			*tmp_buffer = acc2 << s1;
#endif			
			s_base += src_port->rowbyte;
			tmp_buffer_cur += dx;
		}	
	}
	else
	if (dshift < 0) {
		s1 = dshift * -8;
		//s2 = dshift * -24;
		s2 = 32 - s1;

		s_base += (4 * (dx - 1));
		tmp_buffer_cur += (dx - 1);

		for (y = wrk_top; y <= wrk_bottom; y++) {
			tmp_base = s_base;
			tmp_buffer = tmp_buffer_cur;
			read_long(tmp_base, acc2);
			tmp_base -= 4;
			
			x = 1;
			for (x = 1; x < dx; x++) {
				read_long(tmp_base, acc1);
				tmp_base -= 4;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				acc2 <<= s1;
				acc2 |= (acc1 >> s2);
#else
				acc2 >>= s1;
				acc2 |= (acc1 << s2);
#endif
				*tmp_buffer-- = acc2;
				acc2 = acc1;
			}

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
			acc2 <<= s1;
#else
			acc2 >>= s1;
#endif
			*tmp_buffer = acc2;

			s_base += src_port->rowbyte;
			tmp_buffer_cur += dx;
		}	
	}

	wrk_left  = dest_rect->left;
	wrk_right = dest_rect->right + 1;

	wrk_top = dest_rect->top;
	wrk_bottom = dest_rect->bottom;
	
	s_base = dst_port->base + (wrk_top * dst_port->rowbyte) + wrk_left;
	
	tmp_buffer_cur = (ulong *)(((uchar *)saved_copy) + shift1);

	for (y = wrk_top; y <= wrk_bottom; y++) {
		tmp_base = s_base;
		tmp_buffer = tmp_buffer_cur;
		
		x_cpt = wrk_right - wrk_left;
		
		while ((x_cpt > 0) && ((long)tmp_base & 0x07)) {
			*tmp_base++ = *((uchar *)tmp_buffer);
			tmp_buffer = (ulong *) ( (long)tmp_buffer + 1);
			x_cpt--;
		}

		if (((long)tmp_buffer & 0x03) == 0)
			while (x_cpt >= 8) {
				read_double(tmp_buffer, tmp_double);
				tmp_buffer += 2;
				write_double(tmp_base, tmp_double);
				tmp_base += 8;
				x_cpt -= 8;
			}
		else
			while (x_cpt >= 8) {
				a_trick.as_long[0] = *tmp_buffer++;
				a_trick.as_long[1] = *tmp_buffer++;
				write_double(tmp_base, a_trick.as_double);
				tmp_base += 8;
				x_cpt -= 8;
			}
		
		while (x_cpt >= 4) {
			acc1 = *tmp_buffer++;
			write_long(tmp_base, acc1);
			tmp_base += 4;
			x_cpt -= 4;
		}

		while (x_cpt > 0) {
			*tmp_base++ = *((uchar *)tmp_buffer);
			tmp_buffer = (ulong *) ((long)tmp_buffer + 1);
			x_cpt--;
		}

		s_base += dst_port->rowbyte;
		tmp_buffer_cur += dx;
	}	


	gr_free(saved_copy);
	return;
}

/*--------------------------------------------------------------------*/ 

static port	tmp_port;

/*--------------------------------------------------------------------*/ 

void	blit_xor_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar	*src;
	uchar	*dst;
	uchar	*src_cpy;
	uchar	*dst_cpy;
	long	x;
	long	dy;
	long	dx;
	uchar	white_value;
	uchar	fore_value;
	uchar	tmp_value;
	uchar	*invert_map;

	invert_map = dst_port->port_cs->invert_map;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_INDEX;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 1) {
			tmp_value = *src++;
			x -= 2;

			if (tmp_value != white_value)
				*dst = invert_map[*dst];
			dst++;

			tmp_value = *src++;
			if (tmp_value != white_value)
				*dst = invert_map[*dst];

			dst++;
		}

		while(x > 0) {
			tmp_value = *src++;
			x--;
			if (tmp_value != white_value)
				*dst = invert_map[*dst];
			dst++;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_and_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar	*src;
	uchar	*dst;
	uchar	*src_cpy;
	uchar	*dst_cpy;
	long	x;
	long	dy;
	long	dx;
	uchar	white_value;
	uchar	fore_value;
	uchar	tmp_value;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_INDEX;
	fore_value = color_2_index(dst_port->port_cs, dst_port->fore_color);

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 1) {
			tmp_value = *src++;
			x -= 2;

			if (tmp_value == white_value)
				*dst = fore_value;
			dst++;

			tmp_value = *src++;
			if (tmp_value == white_value)
				*dst = fore_value;

			dst++;
		}

		while(x > 0) {
			tmp_value = *src++;
			x--;
			if (tmp_value == white_value)
				*dst = fore_value;
			dst++;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_or_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar	*src;
	uchar	*dst;
	uchar	*src_cpy;
	uchar	*dst_cpy;
	long	x;
	long	dy;
	long	dx;
	trick	acc;
	uchar	white_value;
	uchar	tmp_value;
	
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_INDEX;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		if (x > 3) {
			while((long)src & 0x03) {
				tmp_value = *src++;
				x--;
				if (tmp_value != white_value)
					*dst = tmp_value;
				dst++;
			}
		}

		if (((long)dst & 0x01) == 0) {
			while(x > 3) {
				acc.as_long = *((ulong *)src);
				src += 4;
				x -= 4;

				if ((acc.as_char[0] != white_value) &&
				   (acc.as_char[1] != white_value)) {
					PUT_SHORT(dst, acc.as_short[0]);
				}
				else {
					if (acc.as_char[0] != white_value)
						*dst = acc.as_char[0];
					dst++;
					if (acc.as_char[1] != white_value)
						*dst = acc.as_char[1];
					dst++;
				}
				if ((acc.as_char[2] != white_value) &&
				   (acc.as_char[3] != white_value)) {
					PUT_SHORT(dst, acc.as_short[1]);
				}
				else {
					if (acc.as_char[2] != white_value)
						*dst = acc.as_char[2];
					dst++;
					if (acc.as_char[3] != white_value)
						*dst = acc.as_char[3];
					dst++;
				}
			}
		}
		else
			while(x > 3) {
				acc.as_long = *((ulong *)src);
				src += 4;
				x -= 4;

				if (acc.as_char[0] != white_value)
					*dst = acc.as_char[0];
				dst++;
				if (acc.as_char[1] != white_value)
					*dst = acc.as_char[1];
				dst++;
				if (acc.as_char[2] != white_value)
					*dst = acc.as_char[2];
				dst++;
				if (acc.as_char[3] != white_value)
					*dst = acc.as_char[3];
				dst++;
			}

		while(x > 0) {
			tmp_value = *src++;
			x--;
			if (tmp_value != white_value)
				*dst = tmp_value;
			dst++;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_blend_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar		*src;
	uchar		*dst;
	uchar		*src_cpy;
	uchar		*dst_cpy;
	long		value1;
	long		value2;
	long		old_value1 = 257;
	long		old_value2 = 257;
	long		x;
	long		dy;
	long		dx;
	server_color_map	*a_cs;
	uchar		result_byte;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = src_port->port_cs;
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;


		while(x > 0) {
			x--;
			value1 = *src++;
			value2 = *dst;

			if ((value1 != old_value1) || (value2 != old_value2)) {
				old_value1 = value1;
				old_value2 = value2;
				if (value1 == TRANSPARENT_INDEX) {
					result_byte = value2;
				}
				else
				result_byte = calc_blend_8(
					    				a_cs,
					    				a_cs->color_list[value1],
					    				a_cs->color_list[value2]);
			}
			*dst++ = result_byte;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_add_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar		*src;
	uchar		*dst;
	uchar		*src_cpy;
	uchar		*dst_cpy;
	long		value1;
	long		value2;
	long		old_value1 = 257;
	long		old_value2 = 257;
	long		x;
	long		dy;
	long		dx;
	server_color_map	*a_cs;
	uchar		result_byte;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = src_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 0) {
			x--;
			value1 = *src++;
			value2 = *dst;

			if ((value1 != old_value1) || (value2 != old_value2)) {
				old_value1 = value1;
				old_value2 = value2;
				if (value1 == TRANSPARENT_INDEX) {
					result_byte = value2;
				}
				else
				result_byte = calc_add_8(
					    a_cs,
					    a_cs->color_list[value1],
					    a_cs->color_list[value2]);
			}
			*dst++ = result_byte;
		}
	}
}
/*--------------------------------------------------------------------*/ 

void	blit_sub_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar		*src;
	uchar		*dst;
	uchar		*src_cpy;
	uchar		*dst_cpy;
	long		value1;
	long		value2;
	long		old_value1 = 257;
	long		old_value2 = 257;
	long		x;
	long		dy;
	long		dx;
	server_color_map	*a_cs;
	uchar		result_byte;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = src_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 0) {
			x--;
			value1 = *src++;
			value2 = *dst;
			
			if ((value1 != old_value1) || (value2 != old_value2)) {
				old_value1 = value1;
				old_value2 = value2;
				if (value1 == TRANSPARENT_INDEX) {
					result_byte = value2;
				}
				else
				result_byte = calc_sub_8(
					    				a_cs,
					    				a_cs->color_list[value2],
					    				a_cs->color_list[value1]);
			}
			*dst++ = result_byte;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_min_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar		*src;
	uchar		*dst;
	uchar		*src_cpy;
	uchar		*dst_cpy;
	long		value1;
	long		value2;
	long		old_value1 = 257;
	long		old_value2 = 257;
	long		x;
	long		dy;
	long		dx;
	server_color_map	*a_cs;
	uchar		result_byte;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = src_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 0) {
			x--;
			value1 = *src++;
			value2 = *dst;

			if ((value1 != old_value1) || (value2 != old_value2)) {
				old_value1 = value1;
				old_value2 = value2;
				if (value1 == TRANSPARENT_INDEX) {
					result_byte = value2;
				}
				else
					result_byte = calc_min_8(
						    				a_cs,
						    				a_cs->color_list[value1],
						    				a_cs->color_list[value2]);
			}
			*dst++ = result_byte;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_max_8(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	uchar		*src;
	uchar		*dst;
	uchar		*src_cpy;
	uchar		*dst_cpy;
	long		value1;
	long		value2;
	long		old_value1 = 257;
	long		old_value2 = 257;
	long		x;
	long		dy;
	long		dx;
	server_color_map	*a_cs;
	uchar		result_byte;
	
	src_cpy = src_port->base + (src_rect->top * src_port->rowbyte) + src_rect->left;
	dst_cpy = dst_port->base + (dst_rect->top * dst_port->rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = src_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_port->rowbyte;
		dst_cpy += dst_port->rowbyte;
		x = dx;

		while(x > 0) {
			x--;
			value1 = *src++;
			value2 = *dst;

			if ((value1 != old_value1) || (value2 != old_value2)) {
				old_value1 = value1;
				old_value2 = value2;
				if (value1 == TRANSPARENT_INDEX) {
					result_byte = value2;
				}
				else
				result_byte = calc_max_8(
					    a_cs,
					    a_cs->color_list[value1],
					    a_cs->color_list[value2]);
			}
			*dst++ = result_byte;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8copy(port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	ulong		*src;
	uchar		*dst;
	ulong		*src_cpy;
	uchar		*dst_cpy;
	ulong		value1;
	long		x;
	long		index;

	src_cpy = (ulong*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	if (0 && is_mac) {
		while(dy > 0) {
			dy--;
			src = src_cpy;
			dst = dst_cpy;
			
			src_cpy += src_rowbyte;
			dst_cpy += dst_rowbyte;
			x = dx;

			while(x > 0) {
				x--;
				value1 = *src++;
				value1 <<= 8;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				index = ((((value1 >> 11) & (0xf8 >> 3))) |
					 	(((value1 >> 14) & (0xf8 << 2))) |
					 	(((value1 >> 17) & (0xf8 << 7))));
#else
				value1 <<= 8;
				index = ((((value1 >> 27) & (0xf8 >> 3))) |
					 	(((value1 >> 14) & (0xf8 << 2))) |
					 	(((value1 >> 1) & (0xf8 << 7))));
#endif				

				result_byte = a_cs->inverted[index];
				*dst++ = result_byte;
			}
		}
	}
	else {
		while(dy > 0) {
			dy--;
			src = src_cpy;
			dst = dst_cpy;
			
			src_cpy += src_rowbyte;
			dst_cpy += dst_rowbyte;
			x = dx;

			while(x > 0) {
				x--;
				value1 = *src++;
				/*
				index = ((((value1 >> 17) & (0xf8 << 7))) |
					 	(((value1 >> 14) & (0xf8 << 2))) |
					 	(((value1 >> 11) & (0xf8 >> 3))));
				*/
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				value1 <<= 8;
				index = ((((value1 >> 11) & (0xf8 >> 3))) |
					 	(((value1 >> 14) & (0xf8 << 2))) |
					 	(((value1 >> 17) & (0xf8 << 7))));
#else
				index = ((((value1 >> 27) & (0xf8 >> 3))) |
					 	(((value1 >> 14) & (0xf8 << 2))) |
					 	(((value1 >> 1) & (0xf8 << 7))));
#endif

				result_byte = a_cs->inverted[index];
				*dst++ = result_byte;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8or  (port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	ulong		*src;
	uchar		*dst;
	ulong		*src_cpy;
	uchar		*dst_cpy;
	ulong		value1;
	long		x;
	long		index;

	src_cpy = (ulong*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	if (0 && is_mac) {
		while(dy > 0) {
			dy--;
			src = src_cpy;
			dst = dst_cpy;
			
			src_cpy += src_rowbyte;
			dst_cpy += dst_rowbyte;
			x = dx;
			while(x > 0) {
				x--;
				value1 = *src++;
				if (value1 != TRANSPARENT_LONG) {
					value1 <<= 8;
					index = ((((value1 >> 27) & (0xf8 >> 3))) |
						 	(((value1 >> 14) & (0xf8 << 2))) |
						 	(((value1 >> 1) & (0xf8 << 7))));
				

					result_byte = a_cs->inverted[index];
					*dst = result_byte;
				}
				dst++;
				
			}
		}
	}
	else {
		while(dy > 0) {
			dy--;
			src = src_cpy;
			dst = dst_cpy;
			
			src_cpy += src_rowbyte;
			dst_cpy += dst_rowbyte;
			x = dx;
			while(x > 0) {
				x--;
				value1 = *src++;
				if (value1 != TRANSPARENT_LONG) {
					index = ((((value1 >> 27) & (0xf8 >> 3))) |
						 	(((value1 >> 14) & (0xf8 << 2))) |
						 	(((value1 >> 1) & (0xf8 << 7))));
				

					result_byte = a_cs->inverted[index];
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8blend(port *from_port,
					 port *to_port,
					 rect *src_rect,
					 rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	rgb_color	*src;
	uchar		*dst;
	rgb_color	*src_cpy;
	uchar		*dst_cpy;
	rgb_color	value1;
	uchar		src_byte;
	long		x;
	long		index;

	src_cpy = (rgb_color*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;
		
		if (is_mac) {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_blend_8_y(a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
		else {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_blend_8_x(a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8min (port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	rgb_color	*src;
	uchar		*dst;
	rgb_color	*src_cpy;
	uchar		*dst_cpy;
	rgb_color	value1;
	uchar		src_byte;
	long		x;
	long		index;

	src_cpy = (rgb_color*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

		if (is_mac) {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_min_8_y  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
		else {	
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_min_8_x  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8max (port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	rgb_color	*src;
	uchar		*dst;
	rgb_color	*src_cpy;
	uchar		*dst_cpy;
	rgb_color	value1;
	uchar		src_byte;
	long		x;
	long		index;

	src_cpy = (rgb_color*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

		if (is_mac) {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_max_8_y  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
		else {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_max_8_x  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8add (port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte >> 2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	rgb_color	*src;
	uchar		*dst;
	rgb_color	*src_cpy;
	uchar		*dst_cpy;
	rgb_color	value1;
	uchar		src_byte;
	long		x;
	long		index;

	src_cpy = (rgb_color*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

		if (is_mac) {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_add_8_y  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
		else {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_add_8_x  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void blit_24to8sub (port *from_port,
					port *to_port,
					rect *src_rect,
					rect *dst_rect)
{
	long		src_rowbyte = from_port->rowbyte>>2;
	long		dst_rowbyte = to_port->rowbyte;
	long		dx, dy;	
	server_color_map	*a_cs;
	uchar		result_byte;
	rgb_color	*src;
	uchar		*dst;
	rgb_color	*src_cpy;
	uchar		*dst_cpy;
	rgb_color	value1;
	uchar		src_byte;
	long		x;
	long		index;

	src_cpy = (rgb_color*)from_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = to_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	a_cs = to_port->port_cs;
	
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

		if (is_mac) {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_sub_8_y  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;	
			}
		}
		else {
			while(x > 0) {
				x--;
				value1 = *src++;
				if (as_ulong(value1) != TRANSPARENT_LONG) {
					src_byte = *dst;

					result_byte = calc_sub_8_x  (a_cs,
											     a_cs->color_list[src_byte],
											     value1);
				
					*dst = result_byte;
				}
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 
