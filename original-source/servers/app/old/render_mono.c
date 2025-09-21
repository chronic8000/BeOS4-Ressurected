//******************************************************************************
//
//	File:		render.c
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
/*--------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* Pixel primitives */

void pixel_copy(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	mask;
	long	off;

	off = (x >> 3);

	mask = 0x80 >> (x & 7);
	base = a_port->base + (y * a_port->rowbyte) + off;
	*base |= mask;
}

/*-----------------------------------------------------------------*/

void pixel_or(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	mask;
	long	off;

	off = (x >> 3);

	mask = 0x80 >> (x & 7);
	base = a_port->base + (y * a_port->rowbyte) + off;
	*base |= mask;
}

/*-----------------------------------------------------------------*/

void pixel_xor(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	mask;
	long	off;

	off = (x >> 3);

	mask = 0x80 >> (x & 7);
	base = a_port->base + (y * a_port->rowbyte) + off;
	*base ^= mask;
}

/*-----------------------------------------------------------------*/

void pixel_and(port *a_port, long x, long y)
{
	uchar	*base;
	uchar	mask;
	long	off;

	off = (x >> 3);

	mask = 0x80 >> (x & 7);
	base = a_port->base + (y * a_port->rowbyte) + off;
	*base &= ~mask;
}


