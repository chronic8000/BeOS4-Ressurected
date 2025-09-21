//******************************************************************************
//
//	File:		scale_bits_8.c
//
//	Description:	8 bit utils for streching bits up and down.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History: Original version		 oct 12 92 bgs
//			Adapted for 8 bits		 mar 30 93 bgs
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


/*--------------------------------------------------------------*/

void	scale_up_bits_8(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long		    dst_bit;
	long		    error;
	unsigned char	cur_v;

/* Scale up case	*/
	dst_bit = dst;
	error = dst+(src>>1);

	while (dst_bit--) {
		if (error >= dst) {
			error -= dst;
			cur_v = *src_ptr++;
		}
		error += src;

		*dst_ptr++ = cur_v;
	}
}

/*--------------------------------------------------------------*/


void	scale_bits_8(char *src_ptr, char *dst_ptr, long src, long dst)
{
	long	src_bit;
	long	error;


	if (src > dst) {
		error = src+(dst>>1);
		src_bit = src;

		while(src_bit--) {
			if (error >= src) {
				error -= src;
				*dst_ptr++ = *src_ptr;
			}
			error += dst;
			src_ptr++;
		}
	}
	else
		scale_up_bits_8(src_ptr, dst_ptr, src, dst);
}

/*--------------------------------------------------------------*/
/*
now here is a blit with source and destination not overlapping.
*/

void	scaled_blit_8(	char *src_base,
		      			char *dst_base,
		      			long source_row,
		      			long dst_row,
		      			long src_x_range,
		      			long dst_x_range,
		      			long src_y_range,
		      			long dst_y_range, long st, long sb)
{
	long	cur_src_y;
	long	cur_dst_y;
	long	error;
	char	*tmp_dest_buffer;
	char	*tmp_ptr1;
	char	*tmp_ptr2;
	long	src_count;
	long	dst_count;
	long	i;	

/* Special case where src_y_range == dst_y_range */
	if (src_y_range == dst_y_range) {
		src_base += source_row*st;

		for (cur_dst_y = st; cur_dst_y <= (dst_y_range-sb); cur_dst_y++) {
			scale_bits_8(src_base,
				     	 dst_base,
				     	 src_x_range,
				     	 dst_x_range);
			
			src_base += source_row;
			dst_base += dst_row;
		}
		return;
	}

	src_count = src_x_range;
	dst_count = dst_x_range;

	if (src_y_range < dst_y_range) {
		tmp_dest_buffer = (char *)gr_malloc(dst_count + 4);

/* this is the scale up in the y axis case */
		cur_src_y = 0;
		cur_dst_y = 0;
		error = dst_y_range+(src_y_range>>1);

		while(cur_dst_y <= (dst_y_range-sb)) {
			if (error >= dst_y_range) {
/* if we change the source line, refill the source buffer with it */
				error -= dst_y_range;

				scale_bits_8(src_base,
					     	 tmp_dest_buffer,
					     	 src_x_range,
					     	 dst_x_range);

				cur_src_y++;
				if (cur_src_y < src_y_range)  
					src_base += source_row;
			}
			error += src_y_range;

			if (cur_dst_y >= st) {
				tmp_ptr1 = tmp_dest_buffer;
				tmp_ptr2 = dst_base;

				i = dst_count;
				while(i > 8) {
					*((ulong *)tmp_ptr2) = *((ulong *)tmp_ptr1);
					tmp_ptr2 += 4;
					tmp_ptr1 += 4;
					i -= 8;
					*((ulong *)tmp_ptr2) = *((ulong *)tmp_ptr1);
					tmp_ptr2 += 4;
					tmp_ptr1 += 4;
				}
				while(i > 0) {
					*tmp_ptr2++ = *tmp_ptr1++;
					i--;
				}

				dst_base += dst_row;
			}

			cur_dst_y++;
		}

		gr_free((char *)tmp_dest_buffer);
	}
	else {
/* this is the scale down case in the y axis 	*/
/* In that case we need to OR the scan lines 	*/
/* that are responsible for a given destination	*/
/* scan line.									*/

		cur_dst_y = 0;
		cur_src_y = 0;
		error = src_y_range+(dst_y_range>>1);

		while (cur_dst_y <= (dst_y_range-sb)) {
			if (error >= src_y_range) {
				error -= src_y_range;

/* Destination scan line is changing so we can 		*/
/* scale and blit the line.		       		*/

				if (cur_dst_y >= st) {
					scale_bits_8(src_base,
						     	 dst_base,
						     	 src_x_range,
						     	 dst_x_range);
				
					dst_base += dst_row; 
				}
				cur_dst_y++;
			}
			error += dst_y_range;

			cur_src_y++;
			if (cur_src_y < src_y_range)
				src_base += source_row;
		}
	}
}

/*--------------------------------------------------------------*/


void	blit_scale_8(port *from_port, port *to_port, rect *srcrect, rect *dstrect, long mode, long st, long sb)
{
	long	src_y_range;
	long	dst_y_range;

	long	src_x_range;
	long	dst_x_range;

	port	*tmp_source_port;
	port	*tmp_dest_port;
	rect	bound;
	rect	tmp;
	char	source_copy = 0;

	
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


	offset_rect(srcrect, from_port->origin.h, from_port->origin.v);
	
	if ((srcrect->top != 0) || (srcrect->left != 0)) {
		tmp_source_port = new_offscreen_8(&bound, FALSE);
		blit_copy(from_port, tmp_source_port, srcrect, &bound);
		source_copy = 1;
	}
	else {
		tmp_source_port = from_port;
	}
	
	offset_rect(srcrect, -from_port->origin.h, -from_port->origin.v);

	bound.top = 0;
	bound.left = 0;
	bound.bottom = dst_y_range - (sb+st);
	bound.right = dst_x_range;

/*
	_kdprintf_("tmp dest=%ld %ld\n", bound.bottom);
*/

	tmp_dest_port = new_offscreen_8(&bound, FALSE);

	scaled_blit_8(	(char*)tmp_source_port->base,
		    		(char*)tmp_dest_port->base,
		    		tmp_source_port->rowbyte,
		    		tmp_dest_port->rowbyte,
		    		src_x_range + 1,
		    		dst_x_range + 1,
		    		src_y_range + 1,
		    		dst_y_range + 1, st, (sb+1));
	bound.bottom;

	if (source_copy)	
		dispose_offscreen(tmp_source_port);

//	bound.right -= 1;
	tmp.top = dstrect->top + st;
	tmp.bottom = dstrect->bottom - sb;
	tmp.left = dstrect->left;
	tmp.right = dstrect->right;
//	tmp.right = dstrect->right - 1;

	blit_ns(tmp_dest_port, to_port, &bound, &tmp, mode);
	dispose_offscreen(tmp_dest_port);
}










