/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/privsfnt.h,v 14.0 97/03/17 17:54:24 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	privsfnt.h,v $
 * Revision 14.0  97/03/17  17:54:24  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.3  96/07/02  13:49:32  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.2  95/02/23  09:37:39  roberte
 * Oops! Forgot semi-colon after new prototype.
 * 
 * Revision 10.1  95/02/23  09:34:35  roberte
 * Restored conditional INCL_PCLETTO prototype that got munged
 * during ANSI upgrade.
 * 
 * Revision 10.0  95/02/15  16:24:44  roberte
 * Release
 * 
 * Revision 9.2  95/01/11  13:43:35  shawn
 * Changed traceFunc parameter in sfnt_ReadSFNT() to type fs_FuncType
 * 
 * Revision 9.1  95/01/04  16:35:47  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:21:19  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:31:47  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:58:34  roberte
 * Release
 * 
 * Revision 6.91  93/08/30  14:52:20  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:21:44  roberte
 * Release
 * 
 * Revision 6.3  92/11/19  16:06:31  roberte
 * Release
 * 
 * Revision 6.2  92/04/30  11:31:59  leeann
 * stripped 1 non-ASCII character
 * 
 * Revision 6.1  91/08/14  16:47:30  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:28:39  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:53:04  mark
 * add rcs control strings
 * 
*************************************************************************************/

/*
	File:		private sfnt.h

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	) 1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <4>	 7/18/90	MR		Change return types to int for computemapping and readsfnt
		 <3>	 7/14/90	MR		changed SQRT to conditional FIXEDSQRT2
		 <2>	 7/13/90	MR		Change parameters to ReadSFNT and ComputeMapping
		<1+>	 4/18/90	CL		
		 <1>	 3/21/90	EMT		First checked in with Mike Reed's blessing.

	To Do:
*/

/* PUBLIC PROTOTYPE CALLS */

/*
 * Below we have private stuff
 * This has nothing to do with the file format.
 */

GLOBAL_PROTO void *sfnt_GetTablePtr(fsg_SplineKey *key, sfnt_tableIndex n, btsbool mustHaveTable);

/*
 * Creates mapping for finding offset table
 */

GLOBAL_PROTO void sfnt_DoOffsetTableMap(fsg_SplineKey *key);

/* perfect spot size (Fixed) */
#ifndef FIXEDSQRT2
#define FIXEDSQRT2 0x00016A0A
#endif

/*
 * Returns offset and length for table n
 */

GLOBAL_PROTO void sfnt_GetOffsetAndLength(fsg_SplineKey *key, int32 *offsetT, unsigned *lengthT, sfnt_tableIndex n);

GLOBAL_PROTO int sfnt_ComputeMapping(fsg_SplineKey *key, uint16 platformID, uint16 specificID);

GLOBAL_PROTO void sfnt_ReadSFNTMetrics(fsg_SplineKey *key, uint16 glyphIndex);

#if INCL_PCLETTO
GLOBAL_PROTO int sfnt_ReadSFNT(fsg_SplineKey *key, int16 *elementCount, uint16 gIndex,
						   btsbool useHints, fs_FuncType traceFunc, btsbool isPCLetto);
#else
GLOBAL_PROTO int sfnt_ReadSFNT(fsg_SplineKey *key, int16 *elementCount, uint16 gIndex, btsbool useHints,  fs_FuncType traceFunc);
#endif
