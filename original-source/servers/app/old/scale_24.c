//******************************************************************************
//
//	File:		scale.24.c
//
//	Description:	32 bit utils for streching bits up and down.
//	
//	Written by:	Pierre Raynaud-Richard
//
//	Copyright 1996, Be Incorporated
//
//	Change History: Original version		 oct 12 92 bgs (Benoit)
//			        Adapted for 8 bits		 mar 30 93 bgs (Benoit)
//			        Adapted for 32 bits		 feb 28 96 bgs
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

static long dest_range;
static long scale_step;

/*--------------------------------------------------------------*/

void	scale_bits_24(long *src_ptr, long *dst_ptr)
{
	long     pos,dest,step;
	long     data;
	
	pos = scale_step/2;
	dest = dest_range;
	step = scale_step;
	
	while (dest > 0) {
		*dst_ptr++ = src_ptr[pos>>15];
		pos += step;
		dest--;
	}
}

/*--------------------------------------------------------------*/
/*
now here is a blit with source and destination not overlapping.
*/

void	scaled_blit_24(	char *src_base,
		      			char *dst_base,
		      			long source_row,
		      			long dst_row,
		      			long src_x_range,
		      			long dst_x_range,
		      			long src_y_range,
		      			long dst_y_range,
					    long st,
					    long sb)
{
	long	cur_src_y;
	long	cur_dst_y;
	long	error;
	long	*tmp_source_buffer;
	long	*tmp_dest_buffer;
	long	*tmp_ptr1;
	long	*tmp_ptr2;
	long	i;	


// calcul horizontal scale parameters
	scale_step = (src_x_range<<15)/dst_x_range;
	dest_range = dst_x_range;
	
	tmp_source_buffer = (long*)gr_malloc(src_x_range*4 + 16);
	tmp_dest_buffer = (long*)gr_malloc(dst_x_range*4 + 16);

// Special case where src_y_range == dst_y_range
	if (src_y_range == dst_y_range) {
		
		src_base += source_row*st;
		
		for (cur_dst_y = 0; cur_dst_y < (dst_y_range-(sb+st)); cur_dst_y++) {
			scale_bits_24((long*)src_base,(long*)dst_base);
			
			src_base += source_row;
			dst_base += dst_row;
		}
	}

	else if (src_y_range < dst_y_range) {
/* this is the scale up in the y axis case */
		cur_src_y = 0;
		error = dst_y_range;

		cur_dst_y = 0;

		while(cur_dst_y <= (dst_y_range-sb)) {
			cur_dst_y++;
			
			error += src_y_range;
			if (error > dst_y_range) {
				tmp_ptr1 = (long*)src_base;
 
/* if we change the source line, refill the source buffer with it */

				scale_bits_24((long*)tmp_ptr1,(long*)tmp_dest_buffer);

				error -= dst_y_range;

				if (cur_src_y < src_y_range)  
					src_base += source_row;

				cur_src_y++;
			}

			if (cur_dst_y >= st) {
				tmp_ptr1 = (long*)tmp_dest_buffer;
				tmp_ptr2 = (long*)dst_base;

				i = dst_x_range;
				while(i > 4) {
					tmp_ptr2[0] = tmp_ptr1[0];
					tmp_ptr2[1] = tmp_ptr1[1];
					tmp_ptr2[2] = tmp_ptr1[2];
					tmp_ptr2[3] = tmp_ptr1[3];
					i -= 4;
					tmp_ptr2 += 4;
					tmp_ptr1 += 4;
				}
				while(i > 0) {
					*tmp_ptr2++ = *tmp_ptr1++;
					i--;
				}

				dst_base += dst_row;
			}
		}
	}
	else {

/* this is the scale down case in the y axis 	*/
/* In that case we need to OR the scan lines 	*/
/* that are responsible for a given destination	*/
/* scan line.									*/

		cur_dst_y = 0;
		cur_src_y = 0;

		error = src_y_range>>1;
		while (cur_dst_y <= (dst_y_range-sb)) {
			cur_src_y++;
			
			error += dst_y_range;
			if (error >= src_y_range) {
				if (cur_dst_y >= st) {
					scale_bits_24((long*)src_base,(long*)dst_base);				
					dst_base += dst_row; 
				}
				error -= src_y_range;
				cur_dst_y++;
			}

			if (cur_src_y < src_y_range)
				src_base += source_row;
		}
	}

	gr_free((char *)tmp_source_buffer);
	gr_free((char *)tmp_dest_buffer);
}

/*--------------------------------------------------------------*/


void	blit_scale_24(port *from_port,
					  port *to_port,
					  rect *srcrect,
					  rect *dstrect,
					  long mode,
					  long st,
					  long sb)
{
	long	src_y_range;
	long	dst_y_range;
	long	src_x_range;
	long	dst_x_range;
	port	*tmp_dest_port;
	rect	bound;
	rect	tmp;
	uchar	*tmp_src_base;

	src_y_range = srcrect->bottom - srcrect->top;
	dst_y_range = dstrect->bottom - dstrect->top;

	src_x_range = srcrect->right - srcrect->left;
	dst_x_range = dstrect->right - dstrect->left;
	
	if ((src_x_range < 0) ||
	    (dst_x_range < 0) ||
	    (src_y_range < 0) ||
	    (dst_y_range < 0)) 
		return;

	bound.top = 0;
	bound.left = 0;
	bound.bottom = dst_y_range - (sb+st)+10;
	bound.right = dst_x_range;

	tmp_dest_port = new_offscreen_24(&bound, FALSE);


	bound.bottom -= 10;

	offset_rect(srcrect, from_port->origin.h, from_port->origin.v);
	
	tmp_src_base = from_port->base;
	tmp_src_base = tmp_src_base + srcrect->left*4;
	tmp_src_base = tmp_src_base + srcrect->top*from_port->rowbyte;

	offset_rect(srcrect, -from_port->origin.h, -from_port->origin.v);

	scaled_blit_24(	(char*)tmp_src_base,
		    		(char*)tmp_dest_port->base,
		    		from_port->rowbyte,
		    		tmp_dest_port->rowbyte,
		    		src_x_range + 1,
		    		dst_x_range + 1,
		    		src_y_range + 1,
		    		dst_y_range + 1, st, (sb+1));

	tmp.top = dstrect->top + st;
	tmp.bottom = dstrect->bottom - sb;
	tmp.left = dstrect->left;
	tmp.right = dstrect->right;

	blit_ns(tmp_dest_port, to_port, &bound, &tmp, mode);
	dispose_offscreen(tmp_dest_port);
}
