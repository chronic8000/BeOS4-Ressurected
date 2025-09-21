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

/*----------------------------------------------------------------------------*/


void horizontal_line_copy_pattern(	port *theport,
				 					long x1,
				 					long x2,
				 					long y1,
				 					rect *cliprect,
				 					pattern *the_pattern,
				 					long phase_x,
				 					long phase_y)
{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	uchar	pat_elem;



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


	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	pat_elem = the_pattern->data[phase_y];
	pat_elem = rol(pat_elem, phase_x);
	
	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;

		*base = (*base & leftmask) | (pat_elem & ~(leftmask));
		return;
	}

	 	 
	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase = (*tmpbase & leftmask) | (pat_elem & (~leftmask));
	tmpbase++;
	x = bytecount;
	while (x > 0) {
		x--;
		*tmpbase = pat_elem;
		tmpbase++;
	}
	*tmpbase = (*tmpbase & rightmask) | (pat_elem & (~rightmask));
}

/*--------------------------------------------------------------------*/

void horizontal_line_xor_pattern(port *theport,
				 				long x1,
				 				long x2,
				 				long y1,
				 				rect *cliprect,
				 				pattern *the_pattern,
				 				long phase_x,
				 				long phase_y)
{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	uchar	pat_elem;

#ifdef COLOR

/* @@@@ */

	if (theport->base == screenport.base) {
		c_horizontal_line_xor(&c_port, x1, x2, y1, cliprect, 0xff);
		#ifdef	NOBW
		return;
		#endif
	}

/* @@@@ */

#endif


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


	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	pat_elem = the_pattern->data[phase_y];
	pat_elem = rol(pat_elem, phase_x);
	
	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;

		*base ^= (~leftmask & pat_elem);
		return;
	}

	 	 
	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase ^= (~leftmask & pat_elem);
	tmpbase++;
	x = bytecount;
	while (x > 0) {
		x--;
		*tmpbase ^= pat_elem;
		tmpbase++;
	}
	*tmpbase ^= (~rightmask & pat_elem);
}


/*--------------------------------------------------------------------*/

void horizontal_line_or_pattern( port *theport,
				 				long x1,
				 				long x2,
				 				long y1,
				 				rect *cliprect,
				 				pattern *the_pattern,
				 				long phase_x,
				 				long phase_y)

{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	uchar	pat_elem;
	


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

	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	pat_elem = the_pattern->data[phase_y];
	pat_elem = rol(pat_elem, phase_x);
	
	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;
		*base |= (~leftmask & pat_elem);
		return;
	}

	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase |= ~leftmask;
	tmpbase++;
	x = bytecount;

	while (x > 4) {
		x -= 4;
		*tmpbase |= pat_elem;
		tmpbase++;
		*tmpbase |= pat_elem;
		tmpbase++;
		*tmpbase |= pat_elem;
		tmpbase++;
		*tmpbase |= pat_elem;
		tmpbase++;
	}

	while (x > 0) {
		x--;
		*tmpbase |= pat_elem;
		tmpbase++;
	}

	*tmpbase |= (~rightmask & pat_elem);
}


/*--------------------------------------------------------------------*/

void horizontal_line_and_pattern(port *theport,
				 				long x1,
				 				long x2,
				 				long y1,
				 				rect *cliprect,
				 				pattern *the_pattern,
				 				long phase_x,
				 				long phase_y)

{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	uchar	pat_elem;


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

	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	pat_elem = the_pattern->data[phase_y];
	pat_elem = rol(pat_elem, phase_x);
	pat_elem = ~pat_elem;
	
	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;
		*base &= (leftmask | pat_elem);
		return;
	}

	 	 
	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase &= (leftmask | pat_elem);
	tmpbase++;
	x = bytecount;
	while (x > 0) {
		x--;
		*tmpbase &= pat_elem;
		tmpbase++;
	}
	*tmpbase &= (rightmask | pat_elem);
}



/*--------------------------------------------------------------------*/ 

void vertical_line_or_pattern(port *theport,
						      long x1,
						      long y1,
						      long y2,
						      rect *cliprect,
						      pattern *the_pattern,
						      long phase_x,
						      long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base |= mask;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_copy_pattern(port *theport,
						      	long x1,
						      	long y1,
						      	long y2,
						      	rect *cliprect,
						      	pattern *the_pattern,
						      	long phase_x,
						      	long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base |= mask;
		else
			*base &= ~mask;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 

void vertical_line_xor_pattern(port *theport,
			      				long x1,
			      				long y1,
			      				long y2,
			      				rect *cliprect,
			      				pattern *the_pattern,
			      				long phase_x,
			      				long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;
	uchar	pb;

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb &= mask;

		if (pb)
			*base ^= mask;

		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 


void vertical_line_and_pattern(port *theport,
			      				long x1,
			      				long y1,
			      				long y2,
			      				rect *cliprect,
			      				pattern *the_pattern,
			      				long phase_x,
			      				long phase_y)

{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	pb;
	uchar	nmask;

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		pb = the_pattern->data[(phase_y + y1) & 7];
		pb = rol(pb, phase_x);
		pb |= mask;

		if (pb != 0xff)
			*base &= nmask;
		base += rowbyte;
		y1++;
	}
}


/*------------------------------------------------------------------*/


void horizontal_line_xor(port *theport, long x1, long x2, long y1, rect *cliprect)
{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;


#ifdef COLOR	

/* @@@@ */

	if (theport->base == screenport.base) {
		c_horizontal_line_xor(&c_port, x1, x2, y1, cliprect, 0xff);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */

#endif


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


	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask= (0xff >> lastbyte_theport);

	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;
		*base ^= ~leftmask;
		return;
	}

	 	 
	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase ^= ~leftmask;
	x = bytecount;
	while (x > 0) {
		x--;
		*tmpbase ^= 0xff;
		tmpbase++;
	}
	*tmpbase ^= ~rightmask;
}


/*--------------------------------------------------------------------*/

void horizontal_line_or(port *theport, long x1, long x2, long y1, rect *cliprect)
{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	
#ifdef COLOR	

/* @@@@ */

	if (theport->base == screenport.base) {
		c_horizontal_line_copy(&c_port, x1, x2, y1, cliprect, 1);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */

#endif


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

	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;
		*base |= ~leftmask;
		return;
	}

	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase |= ~leftmask;
	tmpbase++;
	x = bytecount;

	while (x > 4) {
		x -= 4;
		*tmpbase = 0xff;
		tmpbase++;
		*tmpbase = 0xff;
		tmpbase++;
		*tmpbase = 0xff;
		tmpbase++;
		*tmpbase = 0xff;
		tmpbase++;
	}

	while (x > 0) {
		x--;
		*tmpbase = 0xff;
		tmpbase++;
	}

	*tmpbase |= ~rightmask;
}


/*--------------------------------------------------------------------*/

void horizontal_line_and(port *theport, long x1, long x2, long y1, rect *cliprect)
{
	uchar	*base;
	uchar	*tmpbase;
	long	rowbyte;
	long	x;
	long	tmp;
	long	bitcount;
	long	bytecount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	


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

	bitcount = x2 - x1;

	rowbyte = theport->rowbyte;

	base = calcbase(theport, x1, y1);

	firstbyte_theport = 8 - (x1 & 7);
	lastbyte_theport = (x2 & 7);

	leftmask = (0xff << firstbyte_theport);
	rightmask = (0xff >> lastbyte_theport);

	if ((x1 >> 3) == (x2 >> 3)) {
		leftmask = leftmask | rightmask;
		*base &= leftmask;
		return;
	}

	  
	bitcount -= firstbyte_theport;
	bitcount -= lastbyte_theport;

	bytecount = bitcount >> 3;
	tmpbase = base;
	*tmpbase &= leftmask;
	tmpbase++;
	x = bytecount;
	while (x > 0) {
		x--;
		*tmpbase = 0x00;
		tmpbase++;
	}
	*tmpbase &= rightmask;
}

/*--------------------------------------------------------------------*/ 

void vertical_line_or(port *theport, long x1, long y1, long y2, rect *cliprect)
{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;

#ifdef COLOR	

/* @@@@ */

	if (theport->base == screenport.base) {
		c_vertical_line_copy(&c_port, x1, y1, y2, cliprect, 1, 0);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */
	
#endif


	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));

	while ((y2 - y1) > 4) {
		*base |= mask;
		base += rowbyte;
		y1 += 4;
		*base |= mask;
		base += rowbyte;
		*base |= mask;
		base += rowbyte;
		*base |= mask;
		base += rowbyte;
	}

	while (y1 <= y2) {
		*base |= mask;
		base += rowbyte;
		y1++;
	}
}


/*--------------------------------------------------------------------*/ 

void vertical_line_xor(port *theport, long x1, long y1, long y2, rect *cliprect)
{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;

#ifdef COLOR	

/* @@@@ */

	if (theport->base == screenport.base) {
		c_vertical_line_xor(&c_port, x1, y1, y2, cliprect, 20, 0xff);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */

#endif	

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		*base ^= mask;
		base += rowbyte;
		y1++;
	}
}

/*--------------------------------------------------------------------*/ 


void vertical_line_and(port *theport, long x1, long y1, long y2, rect *cliprect)
{
	uchar	*base;
	long	rowbyte;
	long	tmp;
	uchar	mask;
	uchar	nmask;

	rowbyte = theport->rowbyte;

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

	base = calcbase(theport, x1, y1);

	mask = (1 << (7 - (x1 & 7)));
	nmask = ~mask;

	while (y1 <= y2) {
		*base &= nmask;
		base += rowbyte;
		y1++;
	}
}

/*------------------------------------------------------------------*/

void vert_line_pattern_1(port *theport,
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
			vertical_line_copy_pattern(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
		case	op_or  :
		case	op_max :
		case	op_add :
			vertical_line_or_pattern(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
		case	op_min :
		case	op_and :
			vertical_line_and_pattern(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
		case	op_xor :
			vertical_line_xor_pattern(theport, x1, y1, y2, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*------------------------------------------------------------------*/

void vert_line_1(port *theport, long x1, long y1, long y2, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or  :
		case	op_max :
		case	op_add :
			vertical_line_or(theport, x1, y1, y2, cliprect);
				return;
		case	op_sub :
		case	op_min :
		case	op_and :
			vertical_line_and(theport, x1, y1, y2, cliprect);
				return;
		case	op_blend :
		case	op_xor :
			vertical_line_xor(theport, x1, y1, y2, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void horiz_line_pattern_1(port *theport,
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
			horizontal_line_copy_pattern(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
		case	op_or  :
		case	op_max :
		case	op_add :
			horizontal_line_or_pattern(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_sub :
		case	op_min :
		case	op_and :
			horizontal_line_and_pattern(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
		case	op_blend :
		case	op_xor :
			horizontal_line_xor_pattern(theport, x1, x2, y1, cliprect, p, phase_x, phase_y);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void horiz_line_1(port *theport, long x1, long x2, long y1, rect *cliprect, short mode)
{
	switch(mode) {
		case	op_copy:
		case	op_or  :
		case	op_max :
		case	op_add :
			horizontal_line_or(theport, x1, x2, y1, cliprect);
				return;
		case	op_sub :
		case	op_min :
		case	op_and :
			horizontal_line_and(theport, x1, x2, y1, cliprect);
				return;
		case	op_blend :
		case	op_xor :
			horizontal_line_xor(theport, x1, x2, y1, cliprect);
				return;
	}
}

/*--------------------------------------------------------------------*/ 

void run_list_1(port        *theport,
				raster_line *list,
				long        list_cnt,
				rect        *cliprect,
				short       mode)
{
	uchar	*base;
	uchar	*tmpbase;
	long	x,cnt,x1,x2,y1;
	long	tmp;
	long	bitcount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;

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

		x2++;
		bitcount = x2 - x1;

		base = calcbase(theport, x1, y1);

		firstbyte_theport = 8 - (x1 & 7);
		lastbyte_theport = (x2 & 7);

		leftmask = (0xff << firstbyte_theport);
		rightmask = (0xff >> lastbyte_theport);

		if ((x1 >> 3) == (x2 >> 3)) {
			leftmask = leftmask | rightmask;

			switch (mode) {
			case op_copy :
				*base = (*base & leftmask) | ~(leftmask);
				break;
			case op_or :
			case op_add :
			case op_sub :
			case op_blend :
			case op_min :
			case op_max :
				*base |= ~leftmask;
				break;
			case op_and :
				*base &= leftmask;
				break;
			case op_xor :
				*base ^= ~leftmask;
				break;
			}
		}
		else {
			bitcount -= firstbyte_theport;
			bitcount -= lastbyte_theport;
			
			x = bitcount >> 3;
			tmpbase = base;
			
			switch (mode) {
			case op_copy :
				*tmpbase++ = (*tmpbase & leftmask) | ~(leftmask);
				for (;x>0;x--)
					*tmpbase++ = 0xff;
				*tmpbase = (*tmpbase & rightmask) | ~(rightmask);
				break;
			case op_or :
			case op_add :
			case op_sub :
			case op_blend :
			case op_min :
			case op_max :
				*tmpbase++ |= ~leftmask;
				for (;x>0;x--)
					*tmpbase++ = 0xff;
				*tmpbase |= ~rightmask;
				break;
			case op_and :
				*tmpbase++ &= leftmask;
				for (;x>0;x--)
					*tmpbase++ &= 0x00;
				*tmpbase &= rightmask;
				break;
			case op_xor :
				*tmpbase++ ^= ~leftmask;
				for (;x>0;x--)
					*tmpbase++ ^= 0xff;
				*tmpbase ^= ~rightmask;
				break;
			}
		}
	}
}

/*--------------------------------------------------------------------*/ 

void run_list_pattern_1(port        *theport,
						raster_line *list,
						long        list_cnt,
						rect        *cliprect,
						pattern     *the_pattern,
						long        phase_x,
						long        phase_y,
						short       mode)
{
	uchar	*base;
	uchar	*tmpbase;
	long	x,cnt,x1,x2,y1;
	long	tmp;
	long	bitcount;
	long	firstbyte_theport;
	long	lastbyte_theport;
	uchar	leftmask;
	uchar	rightmask;
	uchar	pat_elem;

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

		x2++;
		bitcount = x2 - x1;

		base = calcbase(theport, x1, y1);

		firstbyte_theport = 8 - (x1 & 7);
		lastbyte_theport = (x2 & 7);

		leftmask = (0xff << firstbyte_theport);
		rightmask = (0xff >> lastbyte_theport);

		pat_elem = the_pattern->data[phase_y];
		pat_elem = rol(pat_elem, phase_x);
	
		if ((x1 >> 3) == (x2 >> 3)) {
			leftmask = leftmask | rightmask;

			switch (mode) {
			case op_copy :
				*base = (*base & leftmask) | (pat_elem & ~(leftmask));
				break;
			case op_or :
			case op_add :
			case op_sub :
			case op_blend :
			case op_min :
			case op_max :
				*base |= (~leftmask & pat_elem);
				break;
			case op_and :
				*base &= (leftmask | pat_elem);
				break;
			case op_xor :
				*base ^= (~leftmask & pat_elem);
				break;
			}
		}
		else {
			bitcount -= firstbyte_theport;
			bitcount -= lastbyte_theport;
			
			x = bitcount >> 3;
			tmpbase = base;
			
			switch (mode) {
			case op_copy :
				*tmpbase++ = (*tmpbase & leftmask) | (pat_elem & ~(leftmask));
				for (;x>0;x--)
					*tmpbase++ = pat_elem;
				*tmpbase = (*tmpbase & rightmask) | (pat_elem & ~(rightmask));
				break;
			case op_or :
			case op_add :
			case op_sub :
			case op_blend :
			case op_min :
			case op_max :
				*tmpbase++ |= (~leftmask & pat_elem);
				for (;x>0;x--)
					*tmpbase++ |= pat_elem;
				*tmpbase |= (~rightmask & pat_elem);
				break;
			case op_and :
				*tmpbase++ &= (leftmask | pat_elem);
				for (;x>0;x--)
					*tmpbase++ &= pat_elem;
				*tmpbase &= (rightmask | pat_elem);
				break;
			case op_xor :
				*tmpbase++ ^= (~leftmask & pat_elem);
				for (;x>0;x--)
					*tmpbase++ ^= pat_elem;
				*tmpbase ^= (~rightmask & pat_elem);
				break;
			}
		}
	}
}





