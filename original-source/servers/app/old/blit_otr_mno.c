//******************************************************************************
//
//	File:		blit_otr_mno.c
//
//	Description:	other 1 bit blitting and scrolling cases.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			split from blit.c	bgs	03/21/93
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

#define	DEBUG 1
#include <Debug.h>

/*--------------------------------------------------------------------*/

void blit_xor_mono(port *from_port, port *to_port, rect *srcrect, rect *dstrect)
{
	ulong		a_scanline[MAXBMWIDTH];
	long		i;
	long		srcbytecount;
	long		dstbytecount;
	long		y;
	long		srcright;
	long		dstright;
	ulong		*scanlineptr;
	ulong		tmplong;
	ulong		tmplong1;
	long		shift0;
	long		shift1;
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
	uchar		skipfirstbyte=0;
	uchar		inverted=0;

	
	srcright=srcrect->right+1;
	dstright=dstrect->right+1;

	bitshift0=(srcrect->left & 7);
	bitshift1=(dstrect->left & 7);
	secondmask=(0xff >> bitshift1);
	firstmask =~secondmask;

	mask3=(0xff >> (dstright & 7));

	if (mask3==0xff)
		mask3=0x00;

	mask4=~mask3;

	mask5=mask3 | firstmask;
	mask6=~mask5;

	shift0=bitshift1-bitshift0;
	if (shift0<0) {
		shift0=8+shift0;
		skipfirstbyte=1;
	}
	shift1=32-shift0;

	if (dstrect->top > srcrect->top && (from_port == to_port)) {
		srcptr=calcbase(from_port,srcrect->left,srcrect->bottom);
		dstptr=calcbase(to_port  ,dstrect->left,dstrect->bottom);
		inverted=1;
	}
	else {
		srcptr=calcbase(from_port,srcrect->left,srcrect->top);
		dstptr=calcbase(to_port  ,dstrect->left,dstrect->top);
	}

	srcbytecount=((srcright+7) >> 3)-((srcrect->left) >> 3)/*+1*/;
	dstbytecount=((dstright+7) >> 3)-((dstrect->left) >> 3);
	
	y=srcrect->top;


/* here comes an optimization for align bitmap copy */


	while (y<=srcrect->bottom) {
		y++;
		tmpptr=srcptr;

		if (inverted) 
			srcptr-=from_port->rowbyte;
		else 
			srcptr+=from_port->rowbyte;

		tmpptr1=(uchar *)&a_scanline[0];
		
		i=srcbytecount;
		
		while (i--) {
			*tmpptr1++=*tmpptr;
			tmpptr++;
		}
		*tmpptr1 = 0x00;		/* zero padding for shifted case */

		
		if (shift0!=0) {
			tmplong=0;
			scanlineptr=&a_scanline[0];
			i=(srcbytecount+4) >> 2;
	
			while (i--) {
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
			}
		}

				 
		tmpptr=((uchar *)&a_scanline[0])+skipfirstbyte;
		tmpptr1=dstptr;
		
		if (inverted) 
			dstptr-=to_port->rowbyte;
		else 
			dstptr+=to_port->rowbyte;	

		if (dstbytecount>1) {
			i=dstbytecount-2;
			*tmpptr1=*tmpptr1 ^ (*tmpptr & secondmask);
			tmpptr++;
			tmpptr1++;
			
			while (i>0) {
				i--;
				*tmpptr1^=*tmpptr++;
				tmpptr1++;
			}
			
		    	*tmpptr1=*tmpptr1 ^ (*tmpptr & mask4);
		}
		else {	
			*tmpptr1=*tmpptr1 ^ (*tmpptr & mask6);
		}
	}
}

/*--------------------------------------------------------------------*/

void blit_and_mono(port *from_port, port *to_port, rect *srcrect, rect *dstrect)
{
	ulong		a_scanline[MAXBMWIDTH];
	long		i;
	long		srcbytecount;
	long		dstbytecount;
	long		y;
	long		srcright;
	long		dstright;
	ulong		*scanlineptr;
	ulong		tmplong;
	ulong		tmplong1;
	long		shift0;
	long		shift1;
	char		bitshift0;
	char		bitshift1;
	uchar		firstmask;
	uchar		secondmask;
	uchar		mask3;
	uchar		mask4;
	uchar		mask5;
	uchar		mask6;
	uchar		tmp_byte;
	uchar		*srcptr;
	uchar		*dstptr;
	uchar		*tmpptr;
	uchar		*tmpptr1;
	uchar		skipfirstbyte=0;
	uchar		inverted=0;




	srcright=srcrect->right+1;
	dstright=dstrect->right+1;

	bitshift0=(srcrect->left & 7);
	bitshift1=(dstrect->left & 7);

	secondmask=(0xff >> bitshift1);
	firstmask =~secondmask;

	mask3=(0xff >> (dstright & 7));
	if (mask3==0xff)
		mask3=0x00;
	mask4=~mask3;

	mask5=mask3 | firstmask;
	mask6=~mask5;

	shift0=bitshift1-bitshift0;
	if (shift0<0) {
		shift0=8+shift0;
		skipfirstbyte=1;
	}
	shift1=32-shift0;

	if (dstrect->top > srcrect->top && (from_port == to_port)) {
		srcptr=calcbase(from_port,srcrect->left,srcrect->bottom);
		dstptr=calcbase(to_port  ,dstrect->left,dstrect->bottom);
		inverted=1;
	}
	else {
		srcptr=calcbase(from_port,srcrect->left,srcrect->top);
		dstptr=calcbase(to_port  ,dstrect->left,dstrect->top);
	}

	srcbytecount=((srcright+7) >> 3)-((srcrect->left   ) >> 3)/*+1*/;
	dstbytecount=((dstright+7) >> 3)-((dstrect->left   ) >> 3);
	
	y=srcrect->top;
	

	mask4=~mask4;
	mask6=~mask6;
	secondmask=~secondmask;




	while (y<=srcrect->bottom) {
		y++;
		tmpptr=srcptr;

		if (inverted) 
			srcptr -= from_port->rowbyte;
		else 
			srcptr += from_port->rowbyte;

		tmpptr1 = (uchar *)&a_scanline[0];

		i = srcbytecount;
		while (i--) {
			*tmpptr1++ = *tmpptr;
			tmpptr++;
		}
		*tmpptr1 = 0x00;		/* zero padding for shifted case */

		if (shift0!=0) {
			tmplong=0;
			scanlineptr = &a_scanline[0];
			i = (srcbytecount + 4) >> 2;

			while (i--) {
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
			}
		}
		 
		tmpptr=((uchar *)&a_scanline[0])+skipfirstbyte;
		tmpptr1=dstptr;
		
		if (inverted) 
			dstptr -= to_port->rowbyte;
		else 
			dstptr += to_port->rowbyte;	

		if (dstbytecount > 1) {
			i = dstbytecount - 2;
			
			tmp_byte = *tmpptr1;
			
			if (tmp_byte)
				*tmpptr1 = tmp_byte & (*tmpptr | secondmask);
			
			tmpptr++;
			tmpptr1++;
		
			while (i > 0) {
				*tmpptr1 &= *tmpptr++;
				tmpptr1++;
				i--;
			}

			tmp_byte = *tmpptr1;

			if (tmp_byte)
		    		*tmpptr1 = tmp_byte & (*tmpptr | mask4);
		}
		else {
			tmp_byte = *tmpptr1;
			if (tmp_byte)
				*tmpptr1 = tmp_byte & (*tmpptr | mask6);	
		}
	}
}

/*--------------------------------------------------------------------*/

void blit_or_mono(port *from_port, port *to_port, rect *srcrect, rect *dstrect)
{
	ulong		a_scanline[MAXBMWIDTH];
	long		i;
	long		srcbytecount;
	long		dstbytecount;
	long		y;
	long		srcright;
	long		dstright;
	ulong		*scanlineptr;
	ulong		tmplong;
	ulong		tmplong1;
	long		shift0;
	long		shift1;
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
	uchar		skipfirstbyte = 0;
	uchar		inverted = 0;

	

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

	if (dstrect->top > srcrect->top && (from_port == to_port)) {
		srcptr = calcbase(from_port, srcrect->left, srcrect->bottom);
		dstptr = calcbase(to_port  , dstrect->left, dstrect->bottom);
		inverted = 1;
	}
	else {
		srcptr = calcbase(from_port, srcrect->left, srcrect->top);
		dstptr = calcbase(to_port  , dstrect->left, dstrect->top);
	}

	srcbytecount = ((srcright + 7) >> 3)-((srcrect->left) >> 3)/*+1*/;
	dstbytecount = ((dstright + 7) >> 3)-((dstrect->left) >> 3);
	
	y = srcrect->top;


/* here comes an optimization for align bitmap copy */


	while (y <= srcrect->bottom) {
		y++;
		tmpptr = srcptr;

		if (inverted) 
			srcptr -= from_port->rowbyte;
		else 
			srcptr += from_port->rowbyte;

		tmpptr1 = (uchar *)&a_scanline[0];
		
		i=srcbytecount;
		
		while (i--) {
			*tmpptr1++ = *tmpptr;
			tmpptr++;
		}
		*tmpptr1 = 0x00;		/* zero padding for shifted case */

		
		if (shift0 != 0) {
			tmplong = 0;
			scanlineptr = &a_scanline[0];
			i = (srcbytecount + 4) >> 2;
	
			while (i--) {
				tmplong1 = tmplong;
				tmplong = *scanlineptr;
		 		*scanlineptr++ = (tmplong1 << shift1) | (tmplong >> shift0); 
			}
		}

				 
		tmpptr = ((uchar *)&a_scanline[0]) + skipfirstbyte;
		tmpptr1 = dstptr;
		
		if (inverted) 
			dstptr -= to_port->rowbyte;
		else 
			dstptr += to_port->rowbyte;	

		if (dstbytecount > 1) {
			i = dstbytecount - 2;
			*tmpptr1 = *tmpptr1 | (*tmpptr & secondmask);
			tmpptr++;
			tmpptr1++;
			
			while (i > 0) {
				i--;
				*tmpptr1 |= *tmpptr++;
				tmpptr1++;
			}
			
		    	*tmpptr1=*tmpptr1 | (*tmpptr & mask4);
		}
		else {	
			*tmpptr1=*tmpptr1 | (*tmpptr & mask6);
		}
	}
}

/*--------------------------------------------------------------------*/

