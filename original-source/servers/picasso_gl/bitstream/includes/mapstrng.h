/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/mapstrng.h,v 14.0 97/03/17 17:54:14 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	mapstrng.h,v $
 * Revision 14.0  97/03/17  17:54:14  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.0  95/02/15  16:24:25  roberte
 * Release
 * 
 * Revision 9.1  95/01/04  16:35:22  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:23:18  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:31:25  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:58:20  roberte
 * Release
 * 
 * Revision 6.92  94/04/01  16:06:16  roberte
 * Employed new PROTOTYPE macro for all function prototypes.
 * 
 * Revision 6.91  93/08/30  14:52:01  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:13:48  roberte
 * Release
 * 
 * Revision 6.4  93/01/22  15:25:12  roberte
 * Changed all prototypes to use new PROTO macro.
 * 
 * Revision 6.3  92/10/15  11:52:10  roberte
 * Changed all ifdef PROTOS_AVAIL statements to if PROTOS_AVAIL.
 * 
 * Revision 6.2  92/04/30  11:17:33  leeann
 * take out binary byte
 * 
 * Revision 6.1  91/08/14  16:46:27  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:27:39  mark
 * Release
 * 
 * Revision 4.3  91/08/07  11:52:38  mark
 * remove rcsstatus string
 * 
 * Revision 4.2  91/08/07  11:46:16  mark
 * added RCS control strings
 * 
*************************************************************************************/

/*
	File:		MapString.h

	Contains:	Char2glyph calls

	Written by:	Mike Reed

	Copyright:	) 1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <2>	 8/10/90	MR		Add textLength to MapString2
		 <1>	 7/23/90	MR		first checked in
				 7/23/90	MR		xxx put comment here xxx

	To Do:
*/

GLOBAL_PROTO long MapString0(uint8 *map, uint8 *charCodes, int16 *glyphs, long glyphCount);
GLOBAL_PROTO long MapString2(uint16 *map, uint8 *charCodes, int16 *glyphs, long glyphCount, long *textLength);
GLOBAL_PROTO long MapString4_8(uint16 *map, uint8 *charCodes, int16 *glyphs, long glyphCount);
GLOBAL_PROTO long MapString4_16(uint16 *map, uint16 *charCodes, int16 *glyphs, long glyphCount);
GLOBAL_PROTO long MapString6_8(uint16 *map, uint8 *charCodes, int16 *glyphs, long glyphCount);
GLOBAL_PROTO long MapString6_16(uint16 *map, uint16 *charCodes, int16 *glyphs, long glyphCount);
