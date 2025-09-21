//******************************************************************************
//
//	File:		scale_bits.c
//
//	Description:	1 bit utils for streching bits up and down.
//	Include	   :	scale bits up down + special cases for 0.5, 0.25, 2, 4
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History: Original version		 oct 12 92 bgs
//			bug fixed with shared memory	 feb 06 93 bgs
//
//******************************************************************************/

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif


//--------------------------------------------------------------------

void blit_scale_8(port *, port *, rect *, rect *, long, long, long);

void blit_scale_24(port *, port *, rect *, rect *, long, long, long);

//--------------------------------------------------------------------

/*
 scale_down4 is a special case called to scale down a row of
 bits 4 times smaller the number of bits.
 src_ptr is the source bit buffer.
 dst_ptr is the destination bit buffer.
 src is the number of bits in the source.
 dst is the number of bits in the destination.
*/


void	scale_down4(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	src_bit;

	unsigned long	dst_word;
	unsigned long	src_word;
	unsigned long	src_mask;
	unsigned long	dst_mask;


/* Scale down case	*/


	dst_word = 0;

	src_mask = 0x00;
	dst_mask = 0x80;

	src_bit = 0;


	while((src - src_bit) > 8) {
		src_bit += 8;

		src_mask = 0xf0;
		src_word = *src_ptr++;

		if (src_word) {
			if (src_word & 0xf0)
				dst_word |= dst_mask;
			dst_mask >>= 1;
			if (src_word & 0x0f)
				dst_word |= dst_mask;
			dst_mask >>= 1;
		}
		else
			dst_mask >>= 2;


		if (dst_mask == 0) {
			*dst_ptr++ = dst_word;
			dst_word = 0;
			dst_mask = 0x80;
		}
	}




	for (; src_bit < src; src_bit += 4) {
		if (!(src_mask >>= 4)) {
			src_mask = 0xf0;
			src_word = *src_ptr++;
		}

	
		if (src_word & src_mask)
			dst_word |= dst_mask;

		dst_mask >>= 1;

		if (dst_mask == 0) {
			*dst_ptr++ = dst_word;
			dst_word = 0;
			dst_mask = 0x80;
		}
	}

	if (dst_mask != 0x80)
		*dst_ptr = dst_word;
}


/*--------------------------------------------------------------*/


/*
 scale_down2 is a special case called to scale down a row of
 bits to half the number of bits.
 src_ptr is the source bit buffer.
 dst_ptr is the destination bit buffer.
 src is the number of bits in the source.
 dst is the number of bits in the destination.
*/


void	scale_down2(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	src_bit;

	unsigned long	dst_word;
	unsigned long	src_word;
	unsigned long	src_mask;
	unsigned long	dst_mask;


/* Scale down case	*/


	dst_word = 0;

	src_mask = 0x00;
	dst_mask = 0x80;

	src_bit = 0;


	while((src - src_bit) > 8) {
		src_bit += 8;

		src_mask = 0xc0;
		src_word = *src_ptr++;

		if (src_word) {
			if (src_word & 0xc0)
				dst_word |= dst_mask;
			dst_mask >>= 1;
			if (src_word & 0x30)
				dst_word |= dst_mask;
			dst_mask >>= 1;
			if (src_word & 0x0c)
				dst_word |= dst_mask;
			dst_mask >>= 1;
			if (src_word & 0x03)
				dst_word |= dst_mask;
			dst_mask >>= 1;
		}
		else
			dst_mask >>= 4;


		if (dst_mask == 0) {
			*dst_ptr++ = dst_word;
			dst_word = 0;
			dst_mask = 0x80;
		}
	}


	for (; src_bit < src; src_bit += 2) {
		if (!(src_mask >>= 2)) {
			src_mask = 0xc0;
			src_word = *src_ptr++;
		}

	
		if (src_word & src_mask)
			dst_word |= dst_mask;

		dst_mask >>= 1;

		if (dst_mask == 0) {
			*dst_ptr++ = dst_word;
			dst_word = 0;
			dst_mask = 0x80;
		}
	}

	if (dst_mask != 0x80)
		*dst_ptr = dst_word;
}

/*--------------------------------------------------------------*/

void	scale_up2(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	dst_bit;

	unsigned char	dst_word;
	unsigned char	src_word;
	unsigned long	src_mask;
	unsigned long	dst_mask;


/* Scale up case	*/



	dst_word = 0;

	src_mask = 0x00;
	dst_mask = 0xc0 << 2;

	for (dst_bit = 0; dst_bit < dst; dst_bit += 2) {
			
		dst_mask >>= 2;
		src_mask >>= 1;


		if (src_mask == 0) {
			src_word = *src_ptr++;
			src_mask = 0x80;
		}

		if (dst_mask == 0) {
			dst_mask = 0xc0;
			*dst_ptr++ = dst_word;
			dst_word = 0;
		}

	
		if (src_word & src_mask)
			dst_word |= dst_mask;
	}


	*dst_ptr = dst_word;
}

/*--------------------------------------------------------------*/

void	scale_up_bits(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	dst_bit;
	long	error;
	long	debug;

	unsigned char	dst_word;
	unsigned char	src_word;
	unsigned long	src_mask;
	unsigned long	dst_mask;


/* Scale up case	*/

/*
	if ((src + src) == dst) {
		scale_up2(src_ptr, dst_ptr, src, dst);
		return;
	}
*/
	
/*
	debug = 0;
*/
	error = dst;

	dst_word = 0;

	src_mask = 0x00;
	dst_mask = 0x80 << 1;

	debug = 0;

	for (dst_bit = 0; dst_bit < dst; dst_bit++) {
			
		error += src;

		if (error > dst) {
			error -= dst;
			src_mask >>= 1;
			/*
			debug++;
			*/
			if (src_mask == 0) {
				src_word = *src_ptr++;
				src_mask = 0x80;
			}
		}
		
		dst_mask >>= 1;

		if (dst_mask == 0) {
			dst_mask = 0x80;
			*dst_ptr++ = dst_word;
			dst_word = 0;
		}

	
		if (src_word & src_mask)
			dst_word |= dst_mask;
	}
	/*
	if (debug > src)
		printf("too many %ld %ld\n", debug, src);
	*/

	*dst_ptr = dst_word;
}

/*--------------------------------------------------------------*/

void	scale_bits(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	src_bit;
	long	error;

	unsigned char	dst_word;
	unsigned char	src_word;
	unsigned long	src_mask;
	unsigned long	dst_mask;


//??? Optimize for when src == dst
/* Scale down case	*/

	if (src > dst) {
		/*
		if (src == (dst + dst)) {
			scale_down2(src_ptr, dst_ptr, src, dst);
			return;
		}

		if (src == (dst << 2)) {
			scale_down4(src_ptr, dst_ptr, src, dst);
			return;
		}
		*/

		error = 0;

		dst_word = 0;

		src_mask = 0x00;
		dst_mask = 0x80;

		for (src_bit = 0; src_bit < src; src_bit++) {
			
			if (!(src_mask >>= 1)) {
				src_mask = 0x80;
				src_word = *src_ptr++;
			}

	
			if (src_word & src_mask)
				dst_word |= dst_mask;

			error += dst;

			if (error > src) {
				error -= src;
				dst_mask >>= 1;
				if (dst_mask == 0) {
					*dst_ptr++ = dst_word;
					dst_word = 0;
					dst_mask = 0x80;
				}
			}
		}
		if (dst_mask != 0x80)
			*dst_ptr = dst_word;
	}
	else
		scale_up_bits(src_ptr, dst_ptr, src, dst);
}

/*--------------------------------------------------------------*/
/*
now here is a blit with source and destination not overlapping.
*/

void	scaled_blit(char *src_base,
		    char *dst_base,
		    long source_row,
		    long dst_row,
		    long src_x_range,
		    long dst_x_range,
		    long src_y_range,
		    long dst_y_range)
{
	long	cur_src_y;
	long	cur_dst_y;
	long	error;
	char	*tmp_source_buffer;
	char	*tmp_dest_buffer;
	char	*tmp_ptr1;
	char	*tmp_ptr2;
	long	src_count;
	long	dst_count;
	long	i;	

	src_count = (src_x_range + 7) / 8;
	dst_count = (dst_x_range + 7) / 8;
	tmp_source_buffer = (char *)gr_malloc(src_count + 4);
	tmp_dest_buffer = (char *)gr_malloc(dst_count + 4);

	if (src_y_range == dst_y_range) {
// Special case where src_y_range == dst_y_range
		for (cur_dst_y = 0; cur_dst_y < dst_y_range; cur_dst_y++) {
			scale_bits(src_base,
				   dst_base,
				   src_x_range,
				   dst_x_range);
			
			src_base += source_row;
			dst_base += dst_row;
		}
	}
	else if (src_y_range < dst_y_range) {
/* this is the scale up in the y axis case */
		cur_src_y = 0;
		error = dst_y_range;

		cur_dst_y = 0;

		while(cur_dst_y < dst_y_range) {
			cur_dst_y++;
			
			error += src_y_range;
			if (error > dst_y_range) {
				tmp_ptr1 = src_base;
				tmp_ptr2 = tmp_source_buffer;
 
/* if we change the source line, refill the source buffer with it */

				for (i = 0; i < src_count; i++) {
					*tmp_ptr2++ = *tmp_ptr1;
					tmp_ptr1 += 1;	
				}
				
				tmp_ptr1 = tmp_dest_buffer;

				scale_bits(tmp_source_buffer,
					   tmp_dest_buffer,
					   src_x_range,
					   dst_x_range);

				error -= dst_y_range;

				if (cur_src_y < src_y_range)  
					src_base += source_row;

				cur_src_y++;
			}

			tmp_ptr1 = tmp_dest_buffer;
			tmp_ptr2 = dst_base;

			for (i = 0; i < dst_count; i++) {
				*tmp_ptr2 = *tmp_ptr1++;
				tmp_ptr2 += 1;
			}

			dst_base += dst_row;
		}
	}
	else {

/* this is the scale down case in the y axis 	*/
/* In that case we need to OR the scan lines 	*/
/* that are responsible for a given destination	*/
/* scan line.					*/

		cur_dst_y = 0;
		cur_src_y = 0;

		tmp_ptr1 = tmp_source_buffer;
		for (i = 0; i < src_count; i++) 
			*tmp_ptr1++ = 0x00;

			
		error = 0;
		while (cur_dst_y < dst_y_range) {
			cur_src_y++;
			
			tmp_ptr1 = tmp_source_buffer;
			tmp_ptr2 = src_base;
			
			if (cur_src_y < src_y_range)
				src_base += source_row;

/* or the byte of the different source scan lines that */
/* will be scaled down to one destination scan line    */

			for (i = 0; i < src_count; i++) { 
				*tmp_ptr1++ |= *tmp_ptr2;
				tmp_ptr2 += 1;
			}
			
			error += dst_y_range;
			if (error > src_y_range) {
				error -= src_y_range;

/* Destination scan line is changing so we can 		*/
/* scale and blit the line.		       		*/

				scale_bits(tmp_source_buffer,
					   tmp_dest_buffer,
					   src_x_range,
					   dst_x_range);
				
				tmp_ptr1 = tmp_source_buffer;
				for (i = 0; i < src_count; i++) 
					*tmp_ptr1++ = 0x00;
				
				tmp_ptr1 = tmp_dest_buffer;
				tmp_ptr2 = dst_base;

				for (i = 0; i < dst_count; i++) {
					*tmp_ptr2 = *tmp_ptr1++;
					tmp_ptr2 += 1;
				}

				dst_base += dst_row; 
				cur_dst_y++;
			}
		}
	}

	gr_free((char *)tmp_source_buffer);
	gr_free((char *)tmp_dest_buffer);
}

/*--------------------------------------------------------------*/


void	blit_scale(port *from_port, port *to_port, rect *srcrect, rect *dstrect, long mode)
{
	long	src_y_range;
	long	dst_y_range;

	long	src_x_range;
	long	dst_x_range;

	port	*tmp_source_port;
	port	*tmp_dest_port;
	rect	bound;
	rect	tmp;	
	long	skip_top;
	long	skip_bottom;

	tmp = *dstrect;
	offset_rect(&tmp, to_port->origin.h, to_port->origin.v);
	sect_rect(tmp, to_port->draw_region->bound, &tmp);
	offset_rect(&tmp, -to_port->origin.h, -to_port->origin.v);

	skip_top = tmp.top - dstrect->top;
	skip_bottom = dstrect->bottom - tmp.bottom;

/*
	_kdprintf_("dstrect=%ld %ld\n", dstrect->top, dstrect->bottom);
	_kdprintf_("st=%ld, sb=%ld\n", skip_top, skip_bottom);
*/	

	if (skip_top < 0)
		skip_top = 0;
	if (skip_bottom < 0)
		skip_bottom = 0;
	if (skip_top)
		skip_top -= 1;
	if (skip_bottom)
		skip_bottom -= 1;

/*	xprintf("Scale blit call from->bpp = %d\n",from_port->bit_per_pixel); */

	/* safety : refuse to do the scale blit if the source rect is out of bound */
	if ((srcrect->bottom > from_port->bbox.bottom) ||
		(srcrect->left < from_port->bbox.left) ||
		(srcrect->right > from_port->bbox.right) ||
		(srcrect->top < from_port->bbox.top))
		return;
	
	switch (from_port->bit_per_pixel) {
	case 8 :
		blit_scale_8(from_port, to_port, srcrect, dstrect, mode, skip_top, skip_bottom);
		return;
	case 32 :
		blit_scale_24(from_port, to_port, srcrect, dstrect, mode, skip_top, skip_bottom);
		return;
	}
		
	src_y_range = srcrect->bottom - srcrect->top;
	dst_y_range = dstrect->bottom - dstrect->top;

	src_x_range = srcrect->right - srcrect->left;
	dst_x_range = dstrect->right - dstrect->left;

	if ((src_x_range < 0) ||
	    (dst_x_range < 0) ||
	    (src_y_range < 0) ||
	    (dst_y_range < 0)) 
		return;


/* Now, we want to create a first temporary bitmap that will
   contain the source area aligned to a byte boundary.
   Note a a good optimization would be to only do that when
   the source bitmap is the screen and that we still have the
   hugly problem of byte stepping.
   When byte stepping will be gone, we will also be able
   to use short or longs to do the transfers.
*/

	bound.top = 0;
	bound.left = 0;
	bound.bottom = src_y_range;
	bound.right = src_x_range;

	tmp_source_port = new_offscreen(&bound, FALSE);

	blit_copy(from_port, tmp_source_port, srcrect, &bound);


	bound.top = 0;
	bound.left = 0;
	bound.bottom = dst_y_range;
	bound.right = dst_x_range;


	tmp_dest_port = new_offscreen(&bound, FALSE);

	scaled_blit((char*)tmp_source_port->base,
		    	(char*)tmp_dest_port->base,
		    	tmp_source_port->rowbyte,
		    	tmp_dest_port->rowbyte,
		    	src_x_range + 1,
		    	dst_x_range + 1,
		    	src_y_range + 1,
		    	dst_y_range + 1);
	
	dispose_offscreen(tmp_source_port);
	
/*
	tmp.top = dstrect->top + skip_top;
	tmp.bottom = dstrect->bottom - skip_bottom;
	tmp.left = dstrect->left;
	tmp.right = dstrect->right;
*/

	blit_ns(tmp_dest_port, to_port, &bound, dstrect, mode);
	dispose_offscreen(tmp_dest_port);
}







