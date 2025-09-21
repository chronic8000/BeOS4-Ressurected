/*****************************************************************************
*                                                                            *
*                          Copyright 1993                                    *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ O P T  . H ******************************
 *                                                                           *
 *  Optional include file for user override of default CSP options
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_OPT.H 7.2 1997/03/18 16:04:28 SHAWN release $
 *                                                                                    
 *     $Log: CSP_OPT.H $
 *     Revision 7.2  1997/03/18 16:04:28  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:05:39  mark
 *     Release
 * Revision 6.1  96/08/27  11:58:13  mark
 * Release
 * 
 * Revision 5.1  96/03/14  14:21:15  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:06  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:48  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:42  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:45:24  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/


#if INCL_TPS
#include "useropt.h"	/* look at 4in1's user options */
#define	INCL_CACHE	0
#endif
