/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/fontmath.h,v 14.0 97/03/17 17:53:31 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	fontmath.h,v $
 * Revision 14.0  97/03/17  17:53:31  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.0  95/02/15  16:22:57  roberte
 * Release
 * 
 * Revision 9.1  95/01/04  16:32:32  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:19:39  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:28:59  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:57:11  roberte
 * Release
 * 
 * Revision 6.92  94/04/01  16:04:48  roberte
 * Got rid of some nested comments.
 * Employed new GLOBAL_PROTO macro for all function prototypes.
 * 
 * Revision 6.91  93/08/30  14:50:39  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:10:03  roberte
 * Release
 * 
 * Revision 6.4  93/03/02  16:50:49  mark
 * added prototypes for all fixed point functions
 * 
 * Revision 6.3  92/11/19  15:45:57  roberte
 * Release
 * 
 * Revision 6.1  91/08/14  16:45:14  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:26  mark
 * Release
 * 
 * Revision 4.3  91/08/07  11:50:58  mark
 * remove rcsstatus string
 * 
 * Revision 4.2  91/08/07  11:41:57  mark
 * added RCS control strings
 * 
*************************************************************************************/

/*
	File:		FontMath.h

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	(c) 1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <4>	11/27/90	MR		make pascal declaration a macro, conditionalize traps -vs-
									externs for Fix/Frac math routines. [ph]
		 <3>	 11/5/90	MR		Move [U]SHORTMUL into FSCdefs.h Rename FixMulDiv to LongMulDiv.
									[rb]
		 <2>	10/20/90	MR		Add some new math routines (stolen from skia). [rj]
		 <1>	 4/11/90	dba		first checked in

	To Do:
*/

#define HIBITSET			0x80000000
#define POSINFINITY			0x7FFFFFFF
#define NEGINFINITY			0x80000000
#define HIWORDMASK			0xffff0000
#define LOWORDMASK			0x0000ffff
#define FIXONEHALF			0x00008000
#define ONESHORTFRAC		(1 << 14)

#define FIXROUND( x )		(int16)(((x) + FIXONEHALF) >> 16)
#define ROUNDFIXED( x )		(((x) + FIXONEHALF) & HIWORDMASK)
#define DOT6TOFIX(n)		((n) << 10)
#define HIWORD(n)			((uint16)((uint32)(n) >> 16))
#define LOWORD(n)			((uint16)(n))
#define LOWSIXBITS			63

typedef short ShortFrac;			/* 2.14 */

#ifdef NOT_ON_THE_MAC
/*  PASCAL Fixed FixRatio(short numer,short denom);
 *  PASCAL Fixed FixMul(Fixed a,Fixed b);
 *  PASCAL Fixed FixDiv(Fixed x,Fixed y);
 *  PASCAL Fract FracMul(Fract x,Fract y);
 *  PASCAL Fract FracDiv(Fract x,Fract y);
 *  PASCAL Fract FracSqrt(Fract x);         */
GLOBAL_PROTO PASCAL Fixed FixRatio(short numer, short denom);
GLOBAL_PROTO PASCAL Fixed FixMul(Fixed x, Fixed y);
GLOBAL_PROTO PASCAL Fixed FixDiv(Fixed x, Fixed y);
GLOBAL_PROTO PASCAL Fract FracMul(Fract x, Fract y);
GLOBAL_PROTO PASCAL Fract FracDiv(Fract x, Fract y);
GLOBAL_PROTO PASCAL Fract FracSqrt(Fract x);
#else
#ifndef __TOOLUTILS__
GLOBAL_PROTO PASCAL Fixed FixRatio(short numer,short denom) = 0xA869;
GLOBAL_PROTO PASCAL Fixed FixMul(Fixed a,Fixed b) = 0xA868;
#endif
#ifndef __FIXMATH__
GLOBAL_PROTO PASCAL Fixed FixDiv(Fixed x,Fixed y) = 0xA84D;
GLOBAL_PROTO PASCAL Fract FracMul(Fract x,Fract y) = 0xA84A;
GLOBAL_PROTO PASCAL Fract FracDiv(Fract x,Fract y) = 0xA84B;
GLOBAL_PROTO PASCAL Fract FracSqrt(Fract x) = 0xA849;
#endif
#endif

GLOBAL_PROTO long LongMulDiv (int32 a, int32 b, int32 c );              /* (a*b)/c */
GLOBAL_PROTO long ShortFracMul (long a, ShortFrac b);
GLOBAL_PROTO ShortFrac ShortFracDot (ShortFrac a, ShortFrac b);
GLOBAL_PROTO int32 ShortMulDiv (int32 a, int32 b, int32 c);            /* (a*b)/c */
GLOBAL_PROTO F26Dot6 Mul26Dot6 (F26Dot6 a, F26Dot6 b);
GLOBAL_PROTO F26Dot6 Div26Dot6 (F26Dot6 a, F26Dot6 b);

