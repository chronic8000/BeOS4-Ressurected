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

void line_and_1(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	short		sy;
	uchar		curmask;

	rowbyte = a_port->rowbyte;


	dx = x2 - x1;
	dy = y2 - y1;

	base = calcbase(a_port, x1, y1);

	curmask = 1 << 7 - (x1 & 0x07);
	
	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base &= ~curmask;

			curmask = curmask >> 1;
			x1++;

			if (curmask == 0) {
					curmask = 0x80;
					base++;
			}
			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base &= ~curmask;

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				curmask = curmask >> 1;
				x1++;
				if (curmask == 0) {
					curmask = 0x80;
					base++;
				}
				error -= dy;
			}
		}
	}
}

/*------------------------------------------------------------------*/

void line_or_1(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	short		sy;
	uchar		curmask;

#ifdef COLOR	

/* @@@@ */

	if (a_port->base == screenport.base) {
		c_line_copy(&c_port, x1, y1, x2, y2, 80, clip);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */		

#endif

	rowbyte = a_port->rowbyte;


	dx = x2 - x1;
	dy = y2 - y1;

	base = calcbase(a_port, x1, y1);

	curmask = 1 << 7 - (x1 & 0x07);
	
	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (point_in_rect(&clip, x1, y1) && point_in_rect(&clip, x2, y2)) {
		if (dx > dy) {
			error = dx >> 1;

			cpt = x2 - x1;
			while (cpt >= 0) {
				cpt--;
				*base |= curmask;

				curmask = curmask >> 1;

				if (curmask == 0) {
						curmask = 0x80;
						base++;
				}
				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		} 
		else {
			error = dy >> 1;
		
			cpt = dy;
			while (cpt >= 0) {
				cpt--;
				*base |= curmask;

				base += rowbyte;
				error += dx;

				if (error >= dy) {
					curmask = curmask >> 1;

					if (curmask == 0) {
						curmask = 0x80;
						base++;
					}
					error -= dy;
				}
			}
		}
		return;
	}
	
	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base |= curmask;
			else {
				if (x1 > clip.right)
					return;
			}

			curmask = curmask >> 1;
			x1++;

			if (curmask == 0) {
					curmask = 0x80;
					base++;
			}
			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base |= curmask;

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				curmask = curmask >> 1;
				x1++;

				if (x1 > clip.right)
					return;

				if (curmask == 0) {
					curmask = 0x80;
					base++;
				}
				error -= dy;
			}
		}
	}
}


/*------------------------------------------------------------------*/

void line_xor_1(port *a_port, long x1, long y1, long x2, long y2, rect clip)
{
	uchar	*	base;
	long		dx;
	long		dy;
	long		rowbyte;
	long		error;
	long		cpt;
	short		sy;
	uchar		curmask;

#ifdef COLOR	

/* @@@@ */

	if (a_port->base == screenport.base) {
		c_line_xor(&c_port, x1, y1, x2, y2, 0xff, clip);
		#ifdef	NOBW
		return;
		#endif
	}
/* @@@@ */		

#endif	

	rowbyte = a_port->rowbyte;


	dx = x2 - x1;
	dy = y2 - y1;

	base = calcbase(a_port, x1, y1);

	curmask = 1 << 7 - (x1 & 0x07);
	
	sy = 1;

	if (dy < 0) {
		sy = -1;
		dy = -dy;
	 	rowbyte = -rowbyte;
	}

	if (point_in_rect(&clip, x1, y1) && point_in_rect(&clip, x2, y2)) {
		if (dx > dy) {
			error = dx >> 1;

			cpt = x2 - x1;
			while (cpt >= 0) {
				cpt--;
				*base ^= curmask;

				curmask = curmask >> 1;

				if (curmask == 0) {
						curmask = 0x80;
						base++;
				}
				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		} 
		else {
			error = dy >> 1;
		
			cpt = dy;
			while (cpt >= 0) {
				cpt--;
				*base ^= curmask;

				base += rowbyte;
				error += dx;

				if (error >= dy) {
					curmask = curmask >> 1;

					if (curmask == 0) {
						curmask = 0x80;
						base++;
					}
					error -= dy;
				}
			}
		}
		return;
	}
	
	if (dx > dy) {
		error = dx >> 1;

		cpt = x2 - x1;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base ^= curmask;
			else {
				if (x1 > clip.right)
					return;
			}

			curmask = curmask >> 1;
			x1++;

			if (curmask == 0) {
					curmask = 0x80;
					base++;
			}
			error += dy;
			if (error >= dx) {
				base += rowbyte;
				error -= dx;
				y1 += sy;
			}
		}
	} 
	else {
		error = dy >> 1;
		
		cpt = dy;
		while (cpt >= 0) {
			cpt--;
			if (point_in_rect(&clip, x1, y1))
				*base ^= curmask;

			base += rowbyte;
			error += dx;
			y1 += sy;

			if (error >= dy) {
				curmask = curmask >> 1;
				x1++;

				if (x1 > clip.right)
					return;

				if (curmask == 0) {
					curmask = 0x80;
					base++;
				}
				error -= dy;
			}
		}
	}
}

/*------------------------------------------------------------------*/

void line_1(port *a_port, long x1, long y1, long x2, long y2, rect clip, short mode)
{
	switch(mode) {
		case	op_copy :
			line_or_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_or :
			line_or_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_and :
			line_and_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_xor :
			line_xor_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_add :
			line_or_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_sub :
			line_or_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_blend :
			line_xor_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_min :
			line_and_1(a_port, x1, y1, x2, y2, clip);
			return;
		case	op_max :
			line_or_1(a_port, x1, y1, x2, y2, clip);
			return;
	}
}

