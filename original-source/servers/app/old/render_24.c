#include "render_24.h"

#ifndef PROTO_H
#include "proto.h"
#endif

#include <vga.h>

/*----------------------------------------------------------------------------*/

char	point_in_rect(rect *r, long h, long v);

/*----------------------------------------------------------------------------*/

#define	energy(x) ((((x)&0xFF000000L)>>24)+(((x)&0xFF0000L)>>16)+(((x)&0xFF00L)>>8))

#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8 - n)))

/*----------------------------------------------------------------------------*/

#define	write_long(ptr, what)			\
	*((long *)ptr) = what;
#define	read_long(ptr, what)			\
	what = *(long *)ptr;
#define	as_long(v)				\
	*((long *)&v)
#define	PUT_LONG(ptr, what)		\
		*((ulong *)ptr) = what;	\
		ptr += sizeof(ulong);
#define	PUT_SHORT(ptr, what)	\
		*((ushort *)ptr) = what;	\
		ptr += sizeof(ushort);
#define	color_2_long(x)			\
		(*((ulong*)(&(x))))

/*----------------------------------------------------------------------------*/

ulong	fcolor_2_long(port *a_port, rgb_color c)
{
	uchar	tmp[4];
	uchar	tmp_char;

	if (MAC_PORT(a_port)) {
		tmp[1] = c.red;
		tmp[2] = c.green;
		tmp[3] = c.blue;
		tmp[0] = 0;
		return(color_2_long(tmp));
	}

	tmp_char = c.red;
	c.red = c.blue;
	c.blue = tmp_char;
	return(color_2_long(c));
}

/*----------------------------------------------------------------------------*/

long	sqr(long v)
{
	return(v * v);
}

/*----------------------------------------------------------------------------*/

ulong	calc_hilite_24(ulong old_value, ulong fore_color, ulong back_color)
{
	if (old_value == back_color)
		return fore_color;

	if (old_value == fore_color)
		return back_color;
	else
		return old_value;
}

/*----------------------------------------------------------------------------*/


ulong calc_max_24(ulong c1, ulong c2)
{
	long	level1;
	long	level2;

	if (c1 == TRANSPARENT_MAGIC)
		return(TRANSPARENT_LONG);

	level1 = energy(c1);
	level2 = energy(c2);
		
	if (level1 > level2) 
		return c1;
	else
		return c2;
}

/*--------------------------------------------------------------------*/ 

ulong calc_min_24(ulong c1, ulong c2)
{
	long	level1;
	long	level2;

	if (c1 == TRANSPARENT_MAGIC)
		return(TRANSPARENT_LONG);

	level1 = energy(c1);
	level2 = energy(c2);
		
	if (level1 > level2) 
		return c2;
	else
		return c1;

}

/*--------------------------------------------------------------------*/ 

ulong calc_blend1_24(ulong c1, ulong c2)
{
	return ((c1 & 0xFEFEFEFE)>>1)+((c2 & 0xFEFEFEFE)>>1)+(c1 & c2 & 0x01010101L);
}

/*--------------------------------------------------------------------*/ 

ulong calc_blend_24(ulong c1, ulong c2)
{
	if (c1 == TRANSPARENT_MAGIC)
		return(TRANSPARENT_LONG);

	return ((c1 & 0xFEFEFEFE)>>1)+((c2 & 0xFEFEFEFE)>>1)+(c1 & c2 & 0x01010101L);
}

/*--------------------------------------------------------------------*/ 

ulong calc_add_24(ulong c1, ulong c2)
{
	if (c1 == TRANSPARENT_MAGIC)
		return(TRANSPARENT_LONG);	
		
// magic formula, processing the sum of each component, and setting to 0xFF if overflow
	return ((((((c1^c2)>>1)^((c1>>1)+(c2>>1)))&0x80808080L)>>7)*0xFF)|(c1+c2);
}

/*--------------------------------------------------------------------*/ 
/* will sub c2 from c1 */

ulong calc_sub_24(ulong c1, ulong c2)
{
	if (c1 == TRANSPARENT_MAGIC)
		return(TRANSPARENT_LONG);	

// idem for sub
	c2 ^= 0xFFFFFFFFL;
	return ((((((c1^c2)>>1)^((c1>>1)+(c2>>1)))&0x80808080L)>>7)*0xFF)&(c1+c2+1);
}

/*--------------------------------------------------------------------*/ 

void pixel_xor_24(port *a_port, long x, long y)
{
	ulong	*base;
	
	base = (ulong*)(a_port->base + (y * a_port->rowbyte)) + x;
	*base = -1L-*base;
}

/*--------------------------------------------------------------------*/ 

void pixel_and_24(port *a_port, long x, long y)
{
	((ulong*)(a_port->base + (y * a_port->rowbyte)))[x] = fcolor_2_long(a_port, a_port->back_color);
}

/*--------------------------------------------------------------------*/ 

void pixel_copy_24(port *a_port, long x, long y)
{
	((ulong*)(a_port->base + (y * a_port->rowbyte)))[x] = fcolor_2_long(a_port, a_port->fore_color);
}

/*--------------------------------------------------------------------*/ 

void	pixel_add_24(port *a_port, long x, long y)
{
	pixel_other_24(a_port, x, y, op_add);
}

/*--------------------------------------------------------------------*/ 

void	pixel_sub_24(port *a_port, long x, long y)
{
	pixel_other_24(a_port, x, y, op_sub);
}

/*--------------------------------------------------------------------*/ 

void	pixel_min_24(port *a_port, long x, long y)
{
	pixel_other_24(a_port, x, y, op_min);
}

/*--------------------------------------------------------------------*/ 

void	pixel_max_24(port *a_port, long x, long y)
{
	pixel_other_24(a_port, x, y, op_max);
}

/*--------------------------------------------------------------------*/ 

void	pixel_blend_24(port *a_port, long x, long y)
{
	pixel_other_24(a_port, x, y, op_blend);
}

/*--------------------------------------------------------------------*/ 

void	pixel_other_24(port *a_port, long x, long y, short mode)
{
	ulong		*base;
	ulong		old_color;
	
	base = (ulong*)(a_port->base + (y * a_port->rowbyte)) + x;
	old_color = *base;

	switch(mode) {
		case op_add :
			*base = calc_add_24(old_color, fcolor_2_long(a_port, a_port->fore_color));
			return;
		case op_sub :
			*base = calc_sub_24(old_color, fcolor_2_long(a_port, a_port->fore_color));
			return;
		case op_blend :
			*base = calc_blend_24(old_color, fcolor_2_long(a_port, a_port->fore_color));
			return;	
		case op_min :
			*base = calc_min_24(old_color, fcolor_2_long(a_port, a_port->fore_color));
			return;	
		case op_max :
			*base = calc_max_24(old_color, fcolor_2_long(a_port, a_port->fore_color));
			return;	
		case op_hilite :
			*base = calc_hilite_24(old_color, as_long(a_port->fore_color), as_long(a_port->back_color));
			return;
	}	
}

/*--------------------------------------------------------------------*/ 


void copy_other_8to24(	port *from_port,
						port *dst_port,
						rect *from,
						rect *to,
						short mode)
{
	long		y,dx;
	ulong		*dst_base;
	uchar		*src_base,*tmp_base;
	long		src_rowbyte,dst_rowbyte;
	ulong		old_color,src_color;
	server_color_map	*a_cs;
	uchar		value;

	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	a_cs = from_port->port_cs;

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = (ulong*)dst_port->base + (y * dst_rowbyte) + to->left;
		dx = from->left;
		tmp_base = (src_base + dx);

		dx = to->right - to->left;
		
		while (dx >= 0) {
			dx--;

			value = *tmp_base++;
			if (value == TRANSPARENT_INDEX) {
				dst_base++;
			}
			else {
				src_color = a_cs->xcolor_list[value];
				old_color = *dst_base;
				switch(mode) {
					case op_add :
						*dst_base++ = calc_add_24(old_color, src_color);
						break;

					case op_sub :
						*dst_base++ = calc_sub_24(old_color, src_color);
						break;

					case op_blend :
						*dst_base++ = calc_blend_24(old_color, src_color);
						break;

					case op_min :
						*dst_base++ = calc_min_24(old_color, src_color);
						break;

					case op_max :
						*dst_base++ = calc_max_24(old_color, src_color);
						break;
					
					case op_hilite :
						*dst_base = calc_hilite_24(old_color, src_color, as_long(dst_port->back_color));
						break;
				}	
			}
		}
		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_8to24copy_s  (port *from_port,
							   	 port *dst_port,
							   	 rect *from,
							   	 rect *to)
{
	long		y,dx,dx1;
	ulong		*dst_base,*dst_base1,*dst_base2;
	uchar		*src_base,*tmp_base;
	long		src_rowbyte,dst_rowbyte;
	ulong		a_byte;
	ulong		a_byte0;
	ulong		color_24;
	server_color_map	*a_cs;
	
	src_rowbyte = from_port->rowbyte-1;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
	
	a_cs = from_port->port_cs;

	dx1 = to->right - to->left + 1;

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		
		dst_base = dst_base2;
		dst_base1 = dst_base + dx1;

		tmp_base = src_base + dx;

		a_byte0 = 257;
		while (dst_base1 != dst_base) {
			a_byte = *tmp_base++;
			if (a_byte != a_byte0) {
				color_24 = a_cs->xcolor_list[a_byte];
				a_byte0 = a_byte;
			}
			*dst_base++ = color_24;
		}

		dst_base2 += dst_rowbyte;
		src_base += src_rowbyte;
	}
}


/*--------------------------------------------------------------------*/ 

typedef union {
		ulong	as_long[2];
		double	as_double;
} trick1;

/*--------------------------------------------------------------------*/ 

void	copy_block_8to24copy(port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to)
{
	long		y,dx,dx1;
	ulong		*dst_base,*dst_base1,*dst_base2;
	uchar		*src_base,*tmp_base;
	long		src_rowbyte,dst_rowbyte;
	ulong		a_byte;
	ulong		a_byte0;
	ulong		color_24;
	trick1		t;
	server_color_map	*a_cs;
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte) + from->left;
	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
	
	a_cs = from_port->port_cs;
	dx1 = to->right - to->left + 1;

	for (y = to->top; y <= to->bottom; y++) {
		dx = dx1;
		dst_base = dst_base2;
		tmp_base = src_base;

		if (dx > 1) {
			if (((long)dst_base & 0x07) != 0) {
				a_byte = *tmp_base++;
				color_24 = a_cs->xcolor_list[a_byte];
				*dst_base++ = color_24;
				dx--;
			}
		}
		a_byte0 = 257;
		while (dx > 1) {
			a_byte = *tmp_base++;
			if (a_byte != a_byte0) {
				t.as_long[0] = a_cs->xcolor_list[a_byte];
				a_byte0 = a_byte;
			}
			else
				t.as_long[0] = t.as_long[1];

			a_byte = *tmp_base++;
			if (a_byte != a_byte0) {
				t.as_long[1] = a_cs->xcolor_list[a_byte];
				a_byte0 = a_byte;
			}
			else
				t.as_long[1] = t.as_long[0];

			*((double*)dst_base) = t.as_double;
			dst_base += 2;
			dx -= 2;
		}

		while (dx > 0) {
			a_byte = *tmp_base++;
			color_24 = a_cs->xcolor_list[a_byte];
			*dst_base++ = color_24;
			dx--;
		}

		dst_base2 += dst_rowbyte;
		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	copy_block_8to24or  (port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to)
{
	long		y,dx,dx1;
	ulong		*dst_base,*dst_base1,*dst_base2;
	uchar		*src_base,*tmp_base;
	long		src_rowbyte,dst_rowbyte;
	server_color_map	*a_cs;
	uchar		a_byte;	

	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
	
	a_cs = from_port->port_cs;
	dx1 = to->right - to->left + 1;

	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		
		dst_base = dst_base2;
		dst_base1 = dst_base + dx1;

		tmp_base = src_base + dx;

		while (dst_base1 != dst_base) {
			a_byte = *tmp_base++;
			if (a_byte != TRANSPARENT_INDEX)
				*dst_base = a_cs->xcolor_list[a_byte];
			dst_base++;
		}

		dst_base2 += dst_rowbyte;
		src_base += src_rowbyte;
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_to_screen_24(port *src_port,
			 			  port *dst_port,
			 			  rect *src_rect,
			 			  rect *dst_rect)
{
	copy_block_8to24copy(src_port,dst_port,src_rect,dst_rect);
}


/*--------------------------------------------------------------------*/ 

void	blit_copy_s_eq_d_24(port *src_port,
			 			 	port *dst_port,
			 			 	rect *source_rect,
			 			 	rect *dest_rect)
{
	copy_block_8to24copy(src_port,dst_port,source_rect,dest_rect);
}

/*--------------------------------------------------------------------*/ 

port	tmp_port;

/*--------------------------------------------------------------------*/ 

void	copy_block_1to241or(port *from_port,
							port *dst_port,
							rect *from,
							rect *to)
{
	long	y;
	ulong	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	ulong	*dst_base1;
	ulong	*dst_base2;
	long	src_rowbyte;
	long	dst_rowbyte;
	long	dx;
	long	dx1;
	uchar	a_bw_byte;
	uchar	mask;
	ulong	fore_color_long;
	ulong	back_color_long;

	fore_color_long = fcolor_2_long(dst_port, dst_port->fore_color);
	back_color_long = fcolor_2_long(dst_port, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
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
				*dst_base = fore_color_long;
			
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

void	copy_block_1to241or_s  (port *from_port,
							    port *dst_port,
							    rect *from,
							    rect *to)
{
	ulong	*dst_base;
	uchar	*src_base;
	ulong	*dst_base2;
	short	y;
	short	src_rowbyte;
	short	dst_rowbyte;
	short	dx;
	short	dx1;
	ushort	a_bw_word;
	ushort	mask;
	ushort	mask1;
	ulong	fore_color_long;

	fore_color_long = fcolor_2_long(dst_port, dst_port->fore_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	src_rowbyte--;	

	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
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
				*dst_base = fore_color_long;
			
			dst_base++;
			a_bw_word <<=1;
		}
skip:;
		src_base += src_rowbyte;		//note that rowbyte was post dec once at the start of the routine		//note that rowbyte was post dec once at the start of the routine
	}
}


/*--------------------------------------------------------------------*/ 

void	copy_block_1to241copy(port *from_port,
							  port *dst_port,
							  rect *from,
							  rect *to)
{
	ulong	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	ulong	*dst_base1;
	ulong	*dst_base2;
	short	y;
	short	src_rowbyte;
	short	dst_rowbyte;
	short	dx;
	short	dx1;
	uchar	a_bw_byte;
	uchar	mask;
	uchar	mask0;
	ulong	fore_color_long;
	ulong	back_color_long;

	fore_color_long = fcolor_2_long(dst_port, dst_port->fore_color);
	back_color_long = fcolor_2_long(dst_port, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	dst_base2 = (ulong*)dst_port->base + (dst_rowbyte * to->top) + to->left;
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
				*dst_base++ = fore_color_long;
			else	
				*dst_base++ = back_color_long;

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

void	copy_block_1to241(	port *from_port,
							port *dst_port,
							rect *from,
							rect *to,
							short mode)
{
	long	y;
	ulong	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	ulong	*dst_base1;
	long	src_rowbyte;
	long	dst_rowbyte;
	long	dx;
	uchar	a_bw_byte;
	uchar	mask;
	ulong	fore_color_long;
	ulong	back_color_long;
	ulong	invertmask;

	fore_color_long = fcolor_2_long(dst_port, dst_port->fore_color);
	back_color_long = fcolor_2_long(dst_port, dst_port->back_color);
	
	src_rowbyte = from_port->rowbyte;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_base = from_port->base + (from->top * src_rowbyte);
	
	for (y = to->top; y <= to->bottom; y++) {
		dx = from->left;
		dst_base = (ulong*)dst_port->base + (y * dst_rowbyte) + to->left;
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
				*dst_base++ = fore_color_long;
			else	
				*dst_base++ = back_color_long;
			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		if (mode == op_or)
		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base = fore_color_long;
			
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
				*dst_base = back_color_long;

			dst_base++;
			mask >>= 1;
			if (mask == 0) {
				mask = 0x80;
				a_bw_byte = *tmp_base++;
			}
		}

		invertmask = -1L;

		if (mode == op_xor)
		while (dst_base1 != dst_base) {

			if (a_bw_byte & mask)
				*dst_base = invertmask-*dst_base;
			
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

void	blit_xor_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong	*src;
	ulong	*dst;
	ulong	*src_cpy;
	ulong	*dst_cpy;
	long	src_rowbyte,dst_rowbyte;
	long	x;
	long	dy;
	long	dx;
	ulong	white_value;
	ulong	tmp_value;
	ulong	invertmask;

	invertmask = -1L;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_LONG;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

		while(x > 1) {
			tmp_value = *src++;
			x -= 2;

			if (tmp_value != white_value)
				*dst = invertmask-*dst;
			dst++;

			tmp_value = *src++;
			if (tmp_value != white_value)
				*dst = invertmask-*dst;

			dst++;
		}

		while(x > 0) {
			tmp_value = *src++;
			x--;
			if (tmp_value != white_value)
				*dst = invertmask-*dst;
			dst++;
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_and_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong	*src;
	ulong	*dst;
	ulong	*src_cpy;
	ulong	*dst_cpy;
	long	src_rowbyte,dst_rowbyte;
	long	x;
	long	dy;
	long	dx;
	ulong	white_value;
	ulong	fore_value;
	ulong	tmp_value;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_LONG;
	fore_value = fcolor_2_long(dst_port, dst_port->fore_color);

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
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

#if __POWERPC__

__asm void	*move_reverse() {
	        cmpwi       r5, 0
			ble         cr0, out
do_move:
			lwz			r7, 0 (r4)
			addi		r4, r4, 4
			addic.		r5, r5, -4
			stwbrx		r7, r0, r3
			addi		r3, r3, 4
			bne			cr0, do_move
out:
			blr
}

/*--------------------------------------------------------------------*/ 

__asm void *ap_memcpy_c2nc() {
	cmpli	cr2, 0, r5, 7
	andi.	r7, r3, 7
	rlwinm	r7, r5, 0, 29, 29
	cmpli	cr4, 0, r7, 0
	rlwinm	r8, r5, 0, 30, 30
	cmpli	cr5, 0, r8, 0
	rlwinm	r9, r5, 0, 31, 31
	cmpli	cr6, 0, r9, 0
	ble		cr2, queue
	beq		cr0, al4
	subfic	r6, r3, 8
	rlwinm	r9, r6, 0, 31, 31
	cmpli	cr6, 0, r9, 0
	rlwinm	r8, r6, 0, 30, 30
	cmpli	cr5, 0, r8, 0
	rlwinm	r7, r6, 0, 29, 29
	cmpli	cr4, 0, r7, 0
	beq		cr6, al1
	lbz		r9, +0(r4)
	subi	r5, r5, 1
	addi	r4, r4, 1
	addi	r3, r3, 1
	stb		r9, -1(r3)
al1:
	beq		cr5, al2
	lhz		r9, +0(r4)
	subi	r5, r5, 2
	addi	r4, r4, 2
	addi	r3, r3, 2
	sth		r9, -2(r3)
al2:
	beq		cr4, al4
	lwz		r9, +0(r4)
	subi	r5, r5, 4
	addi	r4, r4, 4
	addi	r3, r3, 4
	stw		r9, -4(r3)
al4:
	sub		r7,	r3, r4
	andi.	r7, r7, 7
	rlwinm	r7, r5, 27, 5, 31
	cmpli	cr7, 0, r7, 0
	rlwinm	r6, r5, 29, 30, 31
	cmpli	cr3, 0, r6, 0
	rlwinm	r8, r5, 0, 29, 29
	cmpli	cr4, 0, r8, 0
	rlwinm	r8, r5, 0, 30, 30
	cmpli	cr5, 0, r8, 0
	rlwinm	r8, r5, 0, 31, 31
	cmpli	cr6, 0, r8, 0
	beq		cr0, align0
	beq		cr7, loop1
loop8:
	li		r10, 31
	dcbt	r10, r4
	subi	r7, r7, 1
	cmpli	cr7, 0, r7, 0
	li		r10, 4
loop7:
	lwz		r8,	+0(r4)
	lwz		r9,	+4(r4)
	subic.	r10, r10, 1
	stw	 	r8, -8(SP)
	stw		r9, -4(SP)
	lfd		f0, -8(SP)
	addi	r4, r4, 8
	addi	r3, r3, 8
	stfd	f0, -8(r3)
	bne		cr0, loop7
	bne		cr7, loop8
loop1:
	beq		cr3, queue
loop:
	lwz		r8,	+0(r4)
	lwz		r9,	+4(r4)
	subic.	r6, r6, 1
	stw	 	r8, -8(SP)
	stw		r9, -4(SP)
	lfd		f0, -8(SP)
	addi	r4, r4, 8
	addi	r3, r3, 8
	stfd	f0, -8(r3)
	bne		cr0, loop
	b		queue
align0:
	beq		cr7, align1
align8:
	li		r10, 31
	dcbt	r10, r4
	subi	r7, r7, 1
	cmpli	cr7, 0, r7, 0
	li		r10, 4
align7:
	subic.	r10, r10, 1
	lfd		f0, +0(r4)
	addi	r3, r3, 8
	addi	r4, r4, 8
	stfd	f0, -8(r3)
	bne		cr0, align7
	bne		cr7, align8
align1:
	beq		cr3, queue
align2:
	subic.	r6, r6, 1
	lfd		f0, +0(r4)
	addi	r3, r3, 8
	addi	r4, r4, 8
	stfd	f0, -8(r3)
	bne		cr0, align2
queue:
	beq		cr4, no4
	lwz		r9, +0(r4)
	addi	r4, r4, 4
	addi	r3, r3, 4
	stw		r9, -4(r3)
no4:
	beq		cr5, no2
	lhz		r9, +0(r4)
	addi	r3, r3, 2
	addi	r4, r4, 2
	sth		r9, -2(r3)
no2:
	beq		cr6, no1
	lbz		r9, +0(r4)
	addi	r3, r3, 1
	stb		r9, -1(r3)
no1:
	blr
}

#endif /* __POWERPC__ */

/*--------------------------------------------------------------------*/ 

void	blit_copy_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong	*src;
	ulong	*dst;
	ulong	*src_cpy;
	ulong	*dst_cpy;
	long	src_rowbyte,dst_rowbyte;
	long	x;
	long	dy;
	long	dx;
	ulong	tmp_value;
	char	is_same_port;	
	char	*buffer;

	is_same_port = 0;
	if (src_port->base == dst_port->base) {
		if ((src_port->base == screen_pointer)) {
			if (blit_jmp) {
				(blit_jmp)(src_rect->left, src_rect->top, dst_rect->left, dst_rect->top, src_rect->right - src_rect->left, src_rect->bottom - src_rect->top);
				(synchro_jmp)();
				return;
			}
		}
		is_same_port = 1;
		buffer = (char *)malloc(src_port->rowbyte);
	}

	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	if ((ulong)src_cpy < (ulong)dst_cpy) {
		src_cpy += src_rowbyte * (dy-1);
		dst_cpy += dst_rowbyte * (dy-1);
		src_rowbyte = -src_rowbyte;
		dst_rowbyte = -dst_rowbyte;
	}
	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;

		x = dx;

		if (is_same_port) {
			memmove(dst, src, dx*4);
		}
		else {
#if __POWERPC__
			if (MAC_PORT(dst_port) || MAC_PORT(src_port)) {
				move_reverse(dst, src, dx*4);
			}
			else {
				ap_memcpy_c2nc(dst, src, dx*4);
			}
#elif __INTEL__
			{
			  uint32    i;
			  uint32    *from, *to;

			  from = (uint32*)src;
			  to = (uint32*)dst;
			  for (i=0; i<dx; i++)
				*to++ = *from++;
			}
//			memcpy(dst, src, dx*4);
#endif /* __POWERPC__ */
		}
	}
	if (is_same_port)
		free((char *)buffer);
}


/*--------------------------------------------------------------------*/ 

void	blit_or_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong	*src;
	ulong	*dst;
	ulong	*src_cpy;
	ulong	*dst_cpy;
	long	src_rowbyte,dst_rowbyte;
	long	x;
	long	dy;
	long	dx;
	ulong	white_value;
	ulong	tmp_value;
		
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	white_value = TRANSPARENT_LONG;

	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				tmp_value = *src++;
				x--;
				if (tmp_value != white_value)
					__stwbrx(tmp_value, dst, 0);
				dst++;
			}
		}
		else 
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				tmp_value = *src++;
				x--;
				if (tmp_value != white_value)
					*dst = tmp_value;
				dst++;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_blend_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong		*src;
	ulong		*dst;
	ulong		*src_cpy;
	ulong		*dst_cpy;
	long		src_rowbyte,dst_rowbyte;
	ulong		value1;
	ulong		value2;
	ulong		old_value1 = TRANSPARENT_LONG;
	ulong		old_value2 = TRANSPARENT_LONG;
	long		x;
	long		dy;
	long		dx;
	ulong		result_long;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_blend_24(value1,value2);
				}
				__stwbrx(result_long, dst, 0);
				dst++;
			}
		}
		else
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_blend_24(value1,value2);
				}
				*dst++ = result_long;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_add_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong		*src;
	ulong		*dst;
	ulong		*src_cpy;
	ulong		*dst_cpy;
	long		src_rowbyte,dst_rowbyte;
	ulong		value1;
	ulong		value2;
	ulong		old_value1 = TRANSPARENT_LONG;
	ulong		old_value2 = TRANSPARENT_LONG;
	long		x;
	long		dy;
	long		dx;
	ulong		result_long;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_add_24(value1,value2);
				}
				__stwbrx(result_long, dst, 0);
				dst++;
			}
		}
		else
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_add_24(value1,value2);
				}
				*dst++ = result_long;
			}
		}
	}
}
/*--------------------------------------------------------------------*/ 

void	blit_sub_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong		*src;
	ulong		*dst;
	ulong		*src_cpy;
	ulong		*dst_cpy;
	long		src_rowbyte,dst_rowbyte;
	ulong		value1;
	ulong		value2;
	ulong		old_value1 = TRANSPARENT_LONG;
	ulong		old_value2 = TRANSPARENT_LONG;
	long		x;
	long		dy;
	long		dx;
	ulong		result_long;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_sub_24(value2,value1);
				}
				__stwbrx(result_long, dst, 0);
				dst++;
			}
		}
		else
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_sub_24(value2,value1);
				}
				*dst++ = result_long;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_min_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong		*src;
	ulong		*dst;
	ulong		*src_cpy;
	ulong		*dst_cpy;
	long		src_rowbyte,dst_rowbyte;
	ulong		value1;
	ulong		value2;
	ulong		old_value1 = TRANSPARENT_LONG;
	ulong		old_value2 = TRANSPARENT_LONG;
	long		x;
	long		dy;
	long		dx;
	ulong		result_long;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_min_24(value1,value2);
				}
				__stwbrx(result_long, dst, 0);
				dst++;
			}
		}
		else
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_min_24(value1,value2);
				}
				*dst++ = result_long;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void	blit_max_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect)
{
	ulong		*src;
	ulong		*dst;
	ulong		*src_cpy;
	ulong		*dst_cpy;
	long		src_rowbyte,dst_rowbyte;
	ulong		value1;
	ulong		value2;
	ulong		old_value1 = TRANSPARENT_LONG;
	ulong		old_value2 = TRANSPARENT_LONG;
	long		x;
	long		dy;
	long		dx;
	ulong		result_long;
	
	src_rowbyte = src_port->rowbyte>>2;
	dst_rowbyte = dst_port->rowbyte>>2;
	
	src_cpy = (ulong*)src_port->base + (src_rect->top * src_rowbyte) + src_rect->left;
	dst_cpy = (ulong*)dst_port->base + (dst_rect->top * dst_rowbyte) + dst_rect->left;
	
	dy = 1 + src_rect->bottom - src_rect->top;
	dx = 1 + src_rect->right  - src_rect->left;

	while(dy > 0) {
		dy--;
		src = src_cpy;
		dst = dst_cpy;
		
		src_cpy += src_rowbyte;
		dst_cpy += dst_rowbyte;
		x = dx;

#if __POWERPC__
		if (MAC_PORT(dst_port)) {
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_max_24(value1,value2);
				}
				__stwbrx(result_long, dst, 0);
				dst++;
			}
		}
		else
#endif /* __POWERPC__ */
		{
			while(x > 0) {
				x--;
				value1 = *src++;
				value2 = *dst;

				if ((value1 != old_value1) || (value2 != old_value2)) {
					old_value1 = value1;
					old_value2 = value2;
					if (value1 == TRANSPARENT_LONG)
						result_long = value2;
					else
						result_long = calc_max_24(value1,value2);
				}
				*dst++ = result_long;
			}
		}
	}
}

