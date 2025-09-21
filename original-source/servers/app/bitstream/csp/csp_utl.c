/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 95                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S R _ U T L . C *******************************
 *                                                                           *
 * Character shape player utility functions.                                 *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_UTL.C 7.2 1997/03/18 16:04:33 SHAWN release $
 *                                                                                    
 *     $Log: CSP_UTL.C $
 *     Revision 7.2  1997/03/18 16:04:33  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:06:28  mark
 *     Release
 * Revision 6.1  96/08/27  11:59:04  mark
 * Release
 * 
 * Revision 5.2  96/03/21  11:39:57  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:22:03  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:58  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:30:42  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:47:30  mark
 * Release
 * 
 * Revision 1.2  95/12/12  14:54:45  john
 * Updated copyright date.
 * 
 * Revision 1.1  95/08/10  16:45:56  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/

#ifndef CSP_DEBUG
#define CSP_DEBUG   0
#endif

#if CSP_DEBUG
#include <stdio.h>
#endif

#include "csp_int.h"                    /* Public and internal header */

#if PROC_TRUEDOC || ! INCL_TPS

#if CSP_DEBUG
#include "csp_dbg.h"
#endif



FUNCTION
fix31 CspLongMult(
    fix31 x,
    fix31 y)
/*
 * Returns the product of a pair of 16.16 fixed point numbers
 */
{
btsbool negative;
ufix16  xLo;
ufix16  yLo;
ufix16  xHi;
ufix16  yHi;
ufix32  product;

if ((x == 0) || (y == 0))
	{
	return 0L;
	}
	
negative = x < 0;
if (negative)
	{
	x = -x;
	}
xLo = (ufix16)x;
xHi = ((ufix16)(x >> 16));

if (y < 0)
	{
	y = -y;
	negative = !negative;
	}
yLo = (ufix16)y;
yHi = ((ufix16)(y >> 16));

product = 
    (((ufix32)yLo * xLo) >> 16) +
    (yHi * (ufix32)xLo) +
    ((ufix32)xHi * y);
return negative? -((fix31)product): (fix31)product;
}


FUNCTION
fix15 CspReadByte(
    ufix8 *pBuff)
{
fix15 result;

result = (fix15)(*(pBuff++));

return result;
}

FUNCTION
fix15 CspReadWord(
    ufix8 *pBuff)
{
ufix16 result;

result = (ufix16)(*(pBuff++));
result = (result << 8) + (ufix16)(*(pBuff++));

return (fix15)result;
}


FUNCTION
ufix32 CspReadLong(
    ufix8 *pBuff)
{
fix31 result;

result = (ufix32)(*(pBuff++));
result = (result << 8) + (ufix32)(*(pBuff++));
result = (result << 8) + (ufix32)(*(pBuff++));

return result;
}
#endif /*PROC_TRUEDOC*/


