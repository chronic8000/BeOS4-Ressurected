//******************************************************************************
//
//	Description:	1 bit renderer & bitters.
//	Include	   :	rendering for rectangle.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			created as a whole aug 18 1992 bgs
//
//******************************************************************************/
  
#include <stdio.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif
 
#ifndef	PROTO_H
#include "proto.h"
#endif

/*----------------------------------------------------------------------------*/

#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8-n)))

#define	rol(b, n) (((uchar)b << n) | ((uchar)b >> (8-n)))

/*----------------------------------------------------------------------------*/
/* RECTANGLE.	*/

void rect_fill_and_mono(port *a_port,
						rect *r,
						pattern *the_pattern,
						long phase_x,
						long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	bitcount;	
	long	bytecount;	
	long	firstbyte_bits;	
	long	lastbyte_bits;	
	uchar	leftmask;	
	uchar	rightmask;		
	long	x;	
	uchar	*tmpbase;	
	ulong	patternbyte;	
		

	tmprect = *r;
	tmprect.right += 1;		/*include last pixel*/

	bitcount = tmprect.right - tmprect.left;
	rowbyte = a_port->rowbyte;

	base = calcbase(a_port, tmprect.left, tmprect.top);	

	firstbyte_bits = 8 - (tmprect.left & 7);
	lastbyte_bits = (tmprect.right & 7);

	leftmask = (0xff << firstbyte_bits);
	rightmask = (0xff >> lastbyte_bits);


	if ((tmprect.left >> 3) == (tmprect.right >> 3)) {
		leftmask = leftmask | rightmask;
		y = tmprect.top;
		while (y <= tmprect.bottom) {
			patternbyte = the_pattern->data[(y + phase_y) & 7];
			patternbyte = rol(patternbyte, phase_x);
			y++;
			*base &= ~(patternbyte & ~(leftmask));
			base += rowbyte;
		} 
		return;
	}

	 	 
	bitcount -= firstbyte_bits;
	bitcount -= lastbyte_bits;

	bytecount = bitcount >> 3;
	y = tmprect.top;
	while (y <= tmprect.bottom) {
		patternbyte = the_pattern->data[(y + phase_y) & 7];
		patternbyte = rol(patternbyte, phase_x);
		y++;
		tmpbase = base;
		base += rowbyte;
		*tmpbase &= ~(patternbyte & (~leftmask));
		tmpbase++;
		x = bytecount;

		while (x > 4) {
			*tmpbase &= ~patternbyte;
			tmpbase++;
			*tmpbase &= ~patternbyte;
			tmpbase++;
			x -= 2;
		}

		while (x > 0) {
			x--;
			*tmpbase &= ~patternbyte;
			tmpbase++;
		}
		*tmpbase &= ~(patternbyte & (~rightmask));
	}
}

/*--------------------------------------------------------------------*/

void rect_fill_xor_mono(port *a_port,
						rect *r,
						pattern *the_pattern,
						long phase_x,
						long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	bitcount;	
	long	bytecount;	
	long	firstbyte_bits;	
	long	lastbyte_bits;	
	uchar	leftmask;	
	uchar	rightmask;		
	long	x;	
	uchar	*tmpbase;	
	ulong	patternbyte;	
		

	tmprect = *r;
	tmprect.right += 1;		/*include last pixel*/

	bitcount = tmprect.right - tmprect.left;
	rowbyte = a_port->rowbyte;

	base = calcbase(a_port, tmprect.left, tmprect.top);	

	firstbyte_bits = 8 - (tmprect.left & 7);
	lastbyte_bits = (tmprect.right & 7);

	leftmask = (0xff << firstbyte_bits);
	rightmask = (0xff >> lastbyte_bits);


	if ((tmprect.left >> 3) == (tmprect.right >> 3)) {
		leftmask = leftmask | rightmask;
		y = tmprect.top;
		while (y <= tmprect.bottom) {
			patternbyte = the_pattern->data[(y + phase_y) & 7];
			patternbyte = rol(patternbyte, phase_x);
			y++;
			*base = *base ^ (patternbyte & ~(leftmask));
			base += rowbyte;
		} 
		return;
	}

	 	 
	bitcount -= firstbyte_bits;
	bitcount -= lastbyte_bits;

	bytecount = bitcount >> 3;
	y = tmprect.top;
	while (y <= tmprect.bottom) {
		patternbyte = the_pattern->data[(y + phase_y) & 7];
		patternbyte = rol(patternbyte, phase_x);
		y++;
		tmpbase = base;
		base += rowbyte;
		*tmpbase = *tmpbase ^ (patternbyte & (~leftmask));
		tmpbase++;
		x = bytecount;

		while (x > 4) {
			*tmpbase ^= patternbyte;
			tmpbase++;
			*tmpbase ^= patternbyte;
			tmpbase++;
			*tmpbase ^= patternbyte;
			tmpbase++;
			*tmpbase ^= patternbyte;
			tmpbase++;
			x -= 4;
		}

		while (x > 0) {
			x--;
			*tmpbase ^= patternbyte;
			tmpbase++;
		}
		*tmpbase ^= (patternbyte & (~rightmask));
	}
}

/*--------------------------------------------------------------------*/

void rect_fill_or_mono( port *a_port,
		       			rect *r,
		       			pattern *the_pattern,
		       			long phase_x,
		       			long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	bitcount;	
	long	bytecount;	
	long	firstbyte_bits;	
	long	lastbyte_bits;	
	uchar	leftmask;	
	uchar	rightmask;		
	long	x;	
	uchar	*tmpbase;	
	ulong	patternbyte;	
		

	tmprect = *r;
	tmprect.right += 1;		/*include last pixel*/

	bitcount = tmprect.right - tmprect.left;
	rowbyte = a_port->rowbyte;

	base = calcbase(a_port, tmprect.left, tmprect.top);	

	firstbyte_bits = 8 - (tmprect.left & 7);
	lastbyte_bits = (tmprect.right & 7);

	leftmask = (0xff << firstbyte_bits);
	rightmask = (0xff >> lastbyte_bits);


	if ((tmprect.left >> 3) == (tmprect.right >> 3)) {
		leftmask = leftmask | rightmask;
		y = tmprect.top;
		while (y <= tmprect.bottom) {
			patternbyte = the_pattern->data[(y + phase_y) & 7];
			patternbyte = rol(patternbyte, phase_x);
			y++;
			*base = *base | (patternbyte & ~(leftmask));
			base += rowbyte;
		} 
		return;
	}

	 	 
	bitcount -= firstbyte_bits;
	bitcount -= lastbyte_bits;

	bytecount = bitcount >> 3;
	y = tmprect.top;
	while (y <= tmprect.bottom) {
		patternbyte = the_pattern->data[(y + phase_y) & 7];
		patternbyte = rol(patternbyte, phase_x);
		y++;
		tmpbase = base;
		base += rowbyte;
		*tmpbase = *tmpbase | (patternbyte & (~leftmask));
		tmpbase++;
		x = bytecount;

		while (x > 4) {
			*tmpbase |= patternbyte;
			tmpbase++;
			*tmpbase |= patternbyte;
			tmpbase++;
			*tmpbase |= patternbyte;
			tmpbase++;
			*tmpbase |= patternbyte;
			tmpbase++;
			x -= 4;
		}

		while (x > 0) {
			x--;
			*tmpbase |= patternbyte;
			tmpbase++;
		}
		*tmpbase |= (patternbyte & (~rightmask));
	}
}

/*--------------------------------------------------------------------*/

void rect_fill_copy_mono(port *a_port,
			 			rect *r,
			 			pattern *the_pattern,
			 			long phase_x,
			 			long phase_y)
{
	uchar	*base;		
	long	rowbyte;	
	long	y;	
	rect	tmprect;	
	long	bitcount;	
	long	bytecount;	
	long	firstbyte_bits;	
	long	lastbyte_bits;	
	uchar	leftmask;	
	uchar	rightmask;		
	long	x;	
	uchar	*tmpbase;	
	ulong	patternbyte;	
		

	tmprect = *r;
	tmprect.right += 1;		/*include last pixel*/

	bitcount = tmprect.right - tmprect.left;
	rowbyte = a_port->rowbyte;

	base = calcbase(a_port, tmprect.left, tmprect.top);	

	firstbyte_bits = 8 - (tmprect.left & 7);
	lastbyte_bits = (tmprect.right & 7);

	leftmask = (0xff << firstbyte_bits);
	rightmask = (0xff >> lastbyte_bits);


	if ((tmprect.left >> 3) == (tmprect.right >> 3)) {
		leftmask = leftmask | rightmask;
		y = tmprect.top;
		while (y <= tmprect.bottom) {
			patternbyte = the_pattern->data[(y + phase_y) & 7];
			patternbyte = rol(patternbyte, phase_x);
			y++;
			*base = (*base & leftmask) | (patternbyte & ~(leftmask));
			base += rowbyte;
		} 
		return;
	}

	 	 
	bitcount -= firstbyte_bits;
	bitcount -= lastbyte_bits;

	bytecount = bitcount >> 3;
	y = tmprect.top;
	while (y <= tmprect.bottom) {
		patternbyte = the_pattern->data[(y + phase_y) & 7];
		patternbyte = rol(patternbyte, phase_x);
		y++;
		tmpbase = base;
		base += rowbyte;
		*tmpbase = (*tmpbase & leftmask) | (patternbyte & (~leftmask));
		tmpbase++;
		x = bytecount;

		while (x > 4) {
			*tmpbase = patternbyte;
			tmpbase++;
			*tmpbase = patternbyte;
			tmpbase++;
			*tmpbase = patternbyte;
			tmpbase++;
			*tmpbase = patternbyte;
			tmpbase++;
			x -= 4;
		}

		while (x > 0) {
			x--;
			*tmpbase = patternbyte;
			tmpbase++;
		}
		*tmpbase = (*tmpbase & rightmask) | (patternbyte & (~rightmask));
	}
}

/*--------------------------------------------------------------------*/

void rect_fill_1(port *a_port, rect *r, pattern *the_pattern, long phase_x, long phase_y, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or  :
		case	op_max :
		case	op_add :
			rect_fill_copy_mono(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_sub :
		case	op_min :
			rect_fill_and_mono(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_blend :
		case	op_xor :
			rect_fill_xor_mono(a_port, r, the_pattern, phase_x, phase_y);
				return;
		case	op_and :
			rect_fill_and_mono(a_port, r, the_pattern, phase_x, phase_y);
				return;
	}
}

