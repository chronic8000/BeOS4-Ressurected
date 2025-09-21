//******************************************************************************
//
//	File:		blit_copy_mono.c
//
//	Description:	1 bit blitting and scrolling for copy mode.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			broken from the main file 03/22/93	bgs
//
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

#ifndef	DISPATCH_H
#include "dispatch.h"
#endif

/*--------------------------------------------------------------------*/

void blit_copy_dv(the_port,srcrect,dstrect)

port		*the_port;
rect		*srcrect;
rect		*dstrect;
{
	long		right;
	long		left;
	long		cpt;
	long		cpt1;
	long		y;
	long		dy;
	long		scan_step;
	long		space_step;
	long		spacing;
	long		spacing3;
	long		byte_count;
	uchar		left_mask;
	uchar		right_mask;
	uchar		mask;
	char		byte0;
	char		byte1;

	char		byte0_b;
	char		byte1_b;

	uchar		*src_ptr;
	uchar		*dst_ptr;
	uchar		*src_ptr1;
	uchar		*ptr_byte;
	uchar		*ptr_byte_b;


	right = srcrect->right + 1;
	left  = srcrect->left;


	left_mask = ((uchar)0xff) >> (left & 7);
	right_mask = ~(((uchar)0xff) >> (right & 7));
	
	cpt = (srcrect->bottom - srcrect->top);
	
	if ((right >> 3) == (left >> 3)) {
		mask = right_mask & left_mask;
		if (srcrect->top > dstrect->top) {
			src_ptr = calcbase(the_port, left, srcrect->top);
			dst_ptr = calcbase(the_port, left, dstrect->top);
			scan_step = the_port->rowbyte;
		}
		else {
			src_ptr = calcbase(the_port, left, srcrect->bottom);
			dst_ptr = calcbase(the_port, left, dstrect->bottom);
			scan_step = -the_port->rowbyte;
		}
		for (y = 0; y <= cpt; y++) {
			*dst_ptr = (*dst_ptr & (~mask)) | (*src_ptr & mask);
			dst_ptr += scan_step;
			src_ptr += scan_step;
		}
		return;
	}

	if (left_mask != 0xff) {
		if (srcrect->top > dstrect->top) {
			src_ptr = calcbase(the_port, left, srcrect->top);
			dst_ptr = calcbase(the_port, left, dstrect->top);
			scan_step = the_port->rowbyte;
		}
		else {
			src_ptr = calcbase(the_port, left, srcrect->bottom);
			dst_ptr = calcbase(the_port, left, dstrect->bottom);
			scan_step = -the_port->rowbyte;
		}
		for (y = 0; y <= cpt; y++) {
			*dst_ptr = (*dst_ptr & (~left_mask)) | (*src_ptr & left_mask);
			dst_ptr += scan_step;
			src_ptr += scan_step;
		}
	}

	if (right_mask != 0x00) {
		if (srcrect->top > dstrect->top) {
			src_ptr = calcbase(the_port, right, srcrect->top);
			dst_ptr = calcbase(the_port, right, dstrect->top);
			scan_step = the_port->rowbyte;
		}
		else {
			src_ptr = calcbase(the_port, right, srcrect->bottom);
			dst_ptr = calcbase(the_port, right, dstrect->bottom);
			scan_step = -the_port->rowbyte;
		}
		for (y = 0; y <= cpt; y++) {
			*dst_ptr = (*dst_ptr & (~right_mask)) | (*src_ptr & right_mask);
			dst_ptr += scan_step;
			src_ptr += scan_step;
		}
	}


	left  = (left + 7) & (0x7ffffff8);
	right = (right & 0x7ffffff8);

	 
	spacing = dstrect->top - srcrect->top;
	if (spacing < 0)
		spacing = -spacing;
	
	if (srcrect->top > dstrect->top) {
		src_ptr = calcbase(the_port, left, srcrect->bottom);
		dst_ptr = calcbase(the_port, left, dstrect->bottom);
		scan_step = -the_port->rowbyte;
		space_step = scan_step * spacing;
	}
	else {
		src_ptr = calcbase(the_port, left, srcrect->top);
		dst_ptr = calcbase(the_port, left, dstrect->top);
		scan_step = the_port->rowbyte;
		space_step = scan_step * spacing;
	}



	byte_count = (right >> 3) - (left >> 3);

	while (byte_count > 2) {	
		byte_count -= 2;
		src_ptr1 = src_ptr;

		spacing3 = spacing * 3;
		cpt1 = cpt - spacing3;

		for (dy = 0; dy < spacing; dy++) {
			y = dy;
			ptr_byte = src_ptr1;
			src_ptr1 += scan_step;
			byte0 = *ptr_byte;
			ptr_byte_b = ptr_byte + 1;
			byte0_b = *ptr_byte_b;

			ptr_byte += space_step;
			ptr_byte_b += space_step;

			while (y < cpt1) {
				byte1 = byte0;
				byte0 = *ptr_byte;

				byte1_b = byte0_b;
				byte0_b = *ptr_byte_b;

				if (byte0 != byte1)
					*ptr_byte = byte1;

				if (byte0_b != byte1_b)
					*ptr_byte_b = byte1_b;

				ptr_byte   += space_step;
				ptr_byte_b += space_step;


				byte1   = *ptr_byte;
				byte1_b = *ptr_byte_b;


				if (byte0 != byte1)
					*ptr_byte = byte0;
				if (byte0_b != byte1_b)
					*ptr_byte_b = byte0_b;
				
				ptr_byte   += space_step;
				ptr_byte_b += space_step;

				byte0 = *ptr_byte;
				byte0_b = *ptr_byte_b;

				if (byte0 != byte1)
					*ptr_byte = byte1;

				if (byte0_b != byte1_b)
					*ptr_byte_b = byte1_b;

				ptr_byte   += space_step;
				ptr_byte_b += space_step;

				y += (spacing*3);
			}

			while (y <= cpt) {
				byte1 = byte0;
				byte0 = *ptr_byte;
				byte1_b = byte0_b;
				byte0_b = *ptr_byte_b;

				if (byte0 != byte1)
					*ptr_byte = byte1;
				if (byte0_b != byte1_b)
					*ptr_byte_b = byte1_b;
				
				ptr_byte   += space_step;
				ptr_byte_b += space_step;
				y += spacing;
			}
		}
		src_ptr += 1;
		src_ptr += 1;
	}
	
	while (byte_count-- > 0) {	
		src_ptr1 = src_ptr;

		spacing3 = spacing * 3;
		cpt1 = cpt - spacing3;

		for (dy = 0; dy < spacing; dy++) {
			y = dy;
			ptr_byte = src_ptr1;
			src_ptr1 += scan_step;
			byte0 = *ptr_byte;

			ptr_byte += space_step;

			while (y < cpt1) {
				byte1 = byte0;
				byte0 = *ptr_byte;
				if (byte0 != byte1)
					*ptr_byte = byte1;
				ptr_byte += space_step;


				byte1 = *ptr_byte;
				if (byte0 != byte1)
					*ptr_byte = byte0;
				ptr_byte += space_step;

				byte0 = *ptr_byte;
				if (byte0 != byte1)
					*ptr_byte = byte1;
				ptr_byte += space_step;

				y += (spacing*3);
			}

			while (y <= cpt) {
				byte1 = byte0;
				byte0 = *ptr_byte;

				if (byte0 != byte1) {
					*ptr_byte = byte1;
				}

				ptr_byte += space_step;
				y += spacing;
			}
		}
		src_ptr += 1;
	}
}

/*--------------------------------------------------------------------*/

typedef union {
		char	as_char[MAXBMWIDTH*4];
		ulong	as_long[MAXBMWIDTH];
} trick;

/*--------------------------------------------------------------------*/

void blit_copy_mono(port *from_port, port *to_port, rect *srcrect, rect *dstrect)
{
	trick		a_scanline;
	short		i;
	short		srcbytecount;
	short		dstbytecount;
	long		y;
	long		srcright;
	long		dstright;
	ulong		*scanlineptr;
	ulong		tmplong;
	ulong		tmplong1;
	short		shift0;
	short		shift1;
	char		bitshift0;
	char		bitshift1;
	uchar		firstmask;
	uchar		secondmask;
	uchar		mask3;
	uchar		mask4;
	uchar		mask5;
	uchar		mask6;
	uchar		*srcptr;
	uchar		*dstptr;
	uchar		*tmpptr;
	uchar		*tmpptr1;
	uchar		*saved_tmpptr1;
	uchar		skipfirstbyte=0;
	uchar		inverted=0;


	if (from_port == to_port) {
		if (srcrect->left == dstrect->left) {
			blit_copy_dv(from_port, srcrect, dstrect);
			return;
		}
	}
		



	srcright = srcrect->right + 1;
	dstright = dstrect->right + 1;

	bitshift0 = (srcrect->left & 7);
	bitshift1 = (dstrect->left & 7);
	secondmask = (0xff >> bitshift1);
	firstmask = ~secondmask;

	mask3 = (0xff >> (dstright & 7));

	if (mask3 == 0xff)
		mask3 = 0x00;

	mask4 = ~mask3;

	mask5 = mask3 | firstmask;
	mask6 = ~mask5;

	shift0 = bitshift1 - bitshift0;
	if (shift0 < 0) {
		shift0 = 8 + shift0;
		skipfirstbyte = 1;
	}
	shift1 = 32 - shift0;

	if (dstrect->top > srcrect->top) {
		srcptr = calcbase(from_port, srcrect->left, srcrect->bottom);
		dstptr = calcbase(to_port  ,dstrect->left, dstrect->bottom);
		inverted = 1;
	}
	else {
		srcptr = calcbase(from_port, srcrect->left, srcrect->top);
		dstptr = calcbase(to_port  , dstrect->left, dstrect->top);
	}

	srcbytecount = ((srcright + 7) >> 3) - ((srcrect->left) >> 3) + 1;
	dstbytecount = ((dstright + 7) >> 3) - ((dstrect->left) >> 3);
	
	y = srcrect->top;


/* here comes an optimization for align bitmap copy */


	if ((shift0 == 0) && ((from_port != to_port) || (srcright == dstright))) {

		while (y <= srcrect->bottom) {
			y++;
			tmpptr = srcptr;

			if (inverted) 
				srcptr -= from_port->rowbyte;
			else 
				srcptr += from_port->rowbyte;

			tmpptr1=dstptr;
		
			if (inverted) 
				dstptr -= to_port->rowbyte;
			else 
				dstptr += to_port->rowbyte;	

			if (((dstright - 1) >> 3) != (dstrect->left >> 3)) {

				i = dstbytecount - 2;
				*tmpptr1 = (*tmpptr1 & firstmask) | (*tmpptr & secondmask);
				
				tmpptr += 1;
				tmpptr1 += 1;
		
				while (i > 0) {
					i--;
					*tmpptr1 = *tmpptr;
					tmpptr += 1;
					tmpptr1 += 1;
				}

			    	*tmpptr1 = (*tmpptr1 & mask3) | (*tmpptr & mask4);
			}
			else {
				*tmpptr1 = (*tmpptr1 & mask5) | (*tmpptr & mask6);	
			}
		}
		return;
	}



	while (y <= srcrect->bottom) {
		
		y++;
		tmpptr = srcptr;

		if (inverted) 
			srcptr -= from_port->rowbyte;
		else 
			srcptr += from_port->rowbyte;

		tmpptr1 = (uchar *)&a_scanline.as_char[0];
		
		i = srcbytecount;
		
		while (i >= 2) {
			*(short *)tmpptr1 = (*tmpptr) << 8 | (*(tmpptr += 1));
			tmpptr1 += 2;
			tmpptr += 1;
			i -= 2;
		}

		while (i > 0) {
			i--;
			*tmpptr1++ = *tmpptr;
			tmpptr += 1;
		}

		
		if (shift0 != 0) {
			tmplong = 0;
			scanlineptr = &a_scanline.as_long[0];
			i = (srcbytecount + 3) >> 2;
	
			while (i >= 2) {
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
				i -= 2;
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
			}

			while (i > 0) {
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
				i--;
			}
		}

		tmpptr = ((uchar *)&a_scanline.as_char[0]) + skipfirstbyte;
		tmpptr1 = dstptr;
		
		if (inverted) 
			dstptr -= to_port->rowbyte;
		else 
			dstptr += to_port->rowbyte;	

		if (dstbytecount > 1) {
			i = dstbytecount - 2;
			*tmpptr1 = (*tmpptr1 & firstmask) | (*tmpptr & secondmask);
			
			tmpptr1 += 1;

			tmpptr += (i + 1);
			saved_tmpptr1 = tmpptr1;

			if (skipfirstbyte) {
				#include "megablit2.x"
			}
			else {
				#include "megablit1.x"
			}

			tmpptr1 = saved_tmpptr1 + i;
			
		   	*tmpptr1 = (*tmpptr1 & mask3) | (*tmpptr & mask4);
		}
		else {	
			*tmpptr1 = (*tmpptr1 & mask5) | (*tmpptr & mask6);
		}
	}
}



/*---------------------------------------------------------------------*/

