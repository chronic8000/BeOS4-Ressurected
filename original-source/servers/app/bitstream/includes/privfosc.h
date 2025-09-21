/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/privfosc.h,v 14.0 97/03/17 17:54:21 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	privfosc.h,v $
 * Revision 14.0  97/03/17  17:54:21  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.0  95/02/15  16:24:39  roberte
 * Release
 * 
 * Revision 9.1  95/01/04  16:35:39  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  13:22:04  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/07/28  11:24:08  roberte
 * Commented out prototype for non-existant function fs_SetMem().
 * Corrected prototype for fs_SetUpKey().
 * 
 * Revision 8.0  94/05/04  09:31:40  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:58:30  roberte
 * Release
 * 
 * Revision 6.91  93/08/30  14:52:14  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:21:38  roberte
 * Release
 * 
 * Revision 6.3  92/11/19  16:06:25  roberte
 * Release
 * 
 * Revision 6.2  92/04/30  11:26:45  leeann
 * stripped 2 non-ASCII characters
 * 
 * Revision 6.1  91/08/14  16:47:25  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:28:34  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:47:59  mark
 * lock fnt.h
 * added RCS control strings
 * 
*************************************************************************************/

/*
	File:		privateFontScaler.h

	Contains:	Nothing Important

	Written by:	Charlton E. Lui

	Copyright:	) 1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <3>	 7/18/90	MR		change parameters of SetupKey to be input*, unsigned and int
		 <2>	  5/3/90	RB		nothing new
		 <1>	 4/16/90	HJR		first checked in
		 <1>	 4/10/90	CL		first checked in

	To Do:
*/

/* ****************************************************
**
** CKL	02/20/1990	Added ANSI-C prototypes
**
** ****************************************************
*/
 
/*
 *
 *  ) Apple Computer Inc. 1988, 1989, 1990.
 *
 *	The file defines private sfnt stuff
 *
 * History:
 * 
 */




/* extern pascal int32 fs_SetMem(register fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr); */
/* extern fsg_SplineKey* fs_SetUpKey(fs_GlyphInputType* inptr, unsigned stateBits, int* error); */
/* extern int32 fs_SetMem(); */

GLOBAL_PROTO fsg_SplineKey *fs_SetUpKey(fs_GlyphInputType *inptr, unsigned stateBits, int *error);

