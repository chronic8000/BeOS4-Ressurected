/*****************************************************************************
*                                                                            *
*                        Copyright 1994, 1995                                *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/************************** B T S T Y P E S  . H ******************************
 *                                                                           *
 * Bitstream Standard Type Definitions                                       *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) Oct. 24, 1994 Created MG                                              *
 *                                                                           *
 *     $Header: //bliss/user/archive/rcs/speedo/btstypes.h,v 33.0 97/03/17 17:44:29 shawn Release $
 *                                                                                    
 *     $Log:	btstypes.h,v $
 * Revision 33.0  97/03/17  17:44:29  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.3  96/10/24  12:02:12  shawn
 * Added typedef for boolean for Non-TDPS/IS implementations.
 * 
 * Revision 32.2  96/07/02  16:57:28  shawn
 * Replace #define of NULL with BTSNULL.
 * Remove boolean typedef.
 * 
 * Revision 32.1  96/06/05  15:35:11  shawn
 * Added support for non-CSP boolean typedef.
 * 
 * Revision 32.0  95/02/15  16:30:12  shawn
 * Release
 * 
 * Revision 31.2  95/01/12  11:15:15  roberte
 *   Udated Copyright header.
 *  .
 * 
 * Revision 31.1  95/01/04  16:28:51  roberte
 * Release
 * 
 * Revision 1.1  94/10/24  17:26:12  mark
 * Initial revision
 * 
 ****************************************************************************/

#ifndef BTS_TYPES_DEFINED
#define BTS_TYPES_DEFINED
 
#define FUNCTION

#define FALSE                 0
#define TRUE                  1

/* CSP #define for NULL */
#define BTSNULL (void *)0

/* CSP typedef for boolean */
typedef char    btsbool;

/* General typedef for boolean */
#ifndef VFONT
typedef unsigned char   boolean;
#endif

#ifndef STDEF
#ifndef SPD_BMAP

#if __STDC__ || defined(sgi) || defined(AIXV3) || defined(_IBMR2) || defined(MSDOS)
typedef signed char fix7;
#else
typedef   char     fix7;
#endif

#ifdef real
#undef real
#endif
typedef   double   real;

typedef   unsigned char ufix8;

#endif

typedef   short    fix15;

typedef   unsigned short ufix16;

typedef   long     fix31;

typedef   unsigned long ufix32;

#define		DATA_TYPES	/* define this to avoid repetition in stdef.h */
#endif

#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#endif

