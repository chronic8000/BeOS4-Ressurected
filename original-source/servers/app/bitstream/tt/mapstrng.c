/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/mapstrng.c,v 14.1 97/04/08 15:45:37 shawn Exp $                                                                       *
*                                                                                    *
*     $Log:	mapstrng.c,v $
 * Revision 14.1  97/04/08  15:45:37  shawn
 * Included in TDIS Release 6.0
 * Added many casts for DOS.
 * 
 * Revision 14.0  97/03/17  17:54:11  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.3  97/01/14  17:38:21  shawn
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 10.2  96/05/13  15:22:29  shawn
 * Inserted a required SWAPW() macro in the BinaryIteration macro.
 * 
 * Revision 10.1  95/11/17  09:50:58  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 10.0  95/02/15  16:24:16  roberte
 * Release
 * 
 * Revision 9.1  95/01/04  16:35:14  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  13:46:32  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/07/01  13:07:27  roberte
 * Made MapString2() portable to INTEL. Required also that
 * sfnt_ComputeIndex2() swap charCode before calling this function.
 * 
 * Revision 8.0  94/05/04  09:31:13  roberte
 * Release
 * 
 * Revision 7.1  94/04/13  17:31:54  roberte
 * In ComputeIndex4(), changed read of shift to calculation from
 * segCountX2 and range. Korean Windows fonts had bad shift values
 * messing up binary search. Thanks Anoosh. And special thanks to Microsoft!
 * 
 * Revision 7.0  94/04/08  11:58:15  roberte
 * Release
 * 
 * Revision 6.92  93/10/01  17:06:52  roberte
 * Added #if PROC_TRUETYPE conditional around whole file.
 * 
 * Revision 6.91  93/08/30  14:51:55  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:13:41  roberte
 * Release
 * 
 * Revision 6.6  93/03/04  11:51:54  roberte
 * Made ComputeIndex4 INTEL portable.
 * 
 * Revision 6.5  92/12/29  12:50:11  roberte
 * Now includes "spdo_prv.h" first.
 * 
 * Revision 6.4  92/11/24  13:36:56  laurar
 * include fino.h
 * 
 * Revision 6.3  92/11/19  16:05:23  roberte
 * Release
 * 
 * Revision 6.2  92/11/11  11:18:48  roberte
 * #ifdef'ed out entire file if PCLETTO is defined.
 * 
 * Revision 6.1  91/08/14  16:46:21  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:27:34  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:45:45  mark
 * added RCS control strings
 * 
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/true_type/mapstrng.c,v 14.1 97/04/08 15:45:37 shawn Exp $";
#endif

/*
	File:		MapString.c

	Contains:	Character to glyph mapping functions

	Written by:	Mike Reed

	Copyright:	(c) 1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <2>	11/16/90	MR		Fix range field of cmap-4 (thanks Eric) [rb]
		 <3>	 9/27/90	MR		Change selector in ComputeIndex4 to be log2(range) instead of
									2*log2(range), and fixed up comments
		 <2>	 8/10/90	MR		Add textLength field to MapString2
		 <1>	 7/23/90	MR		first checked in
				 7/23/90	MR		xxx put comment here xxx

	To Do:
*/


#include "spdo_prv.h"
#if PROC_TRUETYPE
#include "fino.h"
#include "fscdefs.h"
#include "mapstrng.h"


#ifndef PCLETTO
/*
 * High byte mapping through table
 *
 * Useful for the national standards for Japanese, Chinese, and Korean characters.
 *
 * Dedicated in spirit and logic to Mark Davis and the International group.
 *
 *	Algorithm: (I think)
 *		First byte indexes into KeyOffset table.  If the offset is 0, keep going, else use second byte.
 *		That offset is from beginning of data into subHeader, which has 4 words per entry.
 *			entry, extent, delta, range
 *
 *	textLength is optional.  If it is nil, it is ignored.
 */

FUNCTION long MapString2(register uint16 *map, uint8 *charCodes, int16 *glyphs, long glyphCount, long *textLength)
{
	register short count = (short)glyphCount;
	register uint8* codeP = charCodes;
	register uint16 mapMe;
	int16* origGlyphs = glyphs;

	for (--count; count >= 0; --count)
                {
		uint16 highByte = *codeP++;

		if ( SWAPW(map[highByte]) )
			mapMe = *codeP++;
		else
			mapMe = highByte;

		if (textLength && *textLength < codeP - charCodes)
		        {
			--codeP;
			if ( SWAPW(map[highByte]) )
				--codeP;
			break;
		        }

                        /* Calculate map address */

		map = ((uint16*)((int8*)map + (uint16)SWAPW(map[highByte]))) + 256;

                        /* Subtract first code */

		mapMe -= (uint16)SWAPWINC(map);

                        /* Check if within range */

		if ( mapMe < (uint16)SWAPW(*map) )
                        {
			uint16 idDelta;

			map++;
			idDelta = (uint16)SWAPWINC(map);

                                /* make word offset */
			mapMe += mapMe;

                                /* Point to glyph id array */
			map = (uint16*)((int32)map + (uint16)SWAPW(*map) + mapMe );
                                                             
                                /* Check for a non-zero value */
			if ( SWAPW(*map) )
				*glyphs++ = (uint16)SWAPW(*map) + idDelta;
			else
				*glyphs++ = 0;		/* missing */
                        }
                else
                        {
			map++;
			*glyphs++ = 0;			/* missing */					/* <4> bad mapping */
			}
                }

	if (textLength)
		*textLength = codeP - charCodes;		/* report # bytes eaten */

	return glyphs - origGlyphs;				/* return # glyphs processed */
}


#define maxLinearX2 16
#define BinaryIteration \
		newP = (uint16 *) ((int8 *)tableP + (range >>= 1)); \
		if ( charCode > (uint16)SWAPW(*newP) ) tableP = newP;

/*
 * Segment mapping to delta values, Yack.. !
 *
 * In memory of Peter Edberg. Initial code taken from code example supplied by Peter.
 */

FUNCTION LOCAL_PROTO uint16 ComputeIndex4(uint16 *tableP, uint16 charCode)
{
	register uint16 idDelta;
	register uint16 offset, segCountX2, index;

                /* Initialize to the missing glyph */

	index = 0;

                /* Get the Segment Count */

	segCountX2 = (uint16)SWAPWINC(tableP);

	if ( segCountX2 < maxLinearX2 )
                {
		tableP += 3; /* skip binary search parameters */
                }
        else
                {
		/* start with unrolled binary search */

		register uint16 *newP;
		register int16  range; 		/* size of current search range */
		register uint16 selector; 	/* where to jump into unrolled binary search */
		register uint16 shift; 		/* for shifting of range */

		range	 = (uint16)SWAPWINC(tableP); 	/* == 2*(2**floor(log2(segCount))) == 2*largest power of two <= segCount */
		selector = (uint16)SWAPWINC(tableP);	/* == log2(range/2) */

		    /* calculate shift from segCountX2 and range: */
		shift = segCountX2 - range;

		    /* skip reading possibly bad shift from file: */
		tableP++;

   		    /* tableP points at endCount[] */
                    /* If the character is in this range */

		if ( charCode >= (uint16)SWAPW(*((uint16 *)((int8 *)tableP + range))))
			tableP = (uint16 *)((int8 *)tableP + shift);

		switch ( selector )
                        {
                        case 15: BinaryIteration;
                        case 14: BinaryIteration;
                        case 13: BinaryIteration;
                        case 12: BinaryIteration;
                        case 11: BinaryIteration;
                        case 10: BinaryIteration;
                        case  9: BinaryIteration;
                        case  8: BinaryIteration;
                        case  7: BinaryIteration;
                        case  6: BinaryIteration;
                        case  5: BinaryIteration;
                        case  4: BinaryIteration;
                        case  3:
                        case  2:  /* drop through */
                        case  1:
                        case  0:
			break;
                        }
                }

                /* Now do linear search */
	while ( charCode > (uint16)SWAPW(*tableP))
		tableP++;

	        /* One more */
        tableP++;
                
                /* End of search, now do mapping */
                /* point at startCount[] */

	tableP = (uint16 *)((int8 *)tableP + segCountX2);

                /* Check if within range */

	if ( charCode >= (uint16)SWAPW(*tableP) )
                {

		offset = charCode - (uint16)SWAPW(*tableP);

                        /* point to idDelta[] */
		tableP = (uint16 *)((int8 *)tableP + segCountX2);

		idDelta = (uint16)SWAPW(*tableP);

                        /* point to idRangeOffset[] */
		tableP = (uint16 *)((int8 *)tableP + segCountX2);

                        /* If range offset entry is not zero */

		if ( SWAPW(*tableP) )
                        {
                                /* Make word offset */
			offset += offset;

                                /* Point to glyphIndexArray[] */
			tableP 	= (uint16 *)((int8 *)tableP + (uint16)SWAPW(*tableP) + offset );
                                    
                                /* Check the value */
			if (index = (uint16)SWAPW(*tableP))
				index += idDelta;
                        }
                else
                                /* Add idDelta to glyph index */
			index	= charCode + idDelta;

                }

	return index;
}



FUNCTION long MapString4_16(uint16 *map, uint16 *charCodes, int16 *glyphs, long glyphCount)
{
	register short count = (short)glyphCount;

	for (--count; count >= 0; --count)
		*glyphs++ = ComputeIndex4( map, *charCodes++ );

	return glyphCount << 1;
}


/*
 * Trimmed Table Mapping - 16 bit character codes
 */

FUNCTION long MapString6_16(register uint16 *map, uint16 *charCodes, register int16 *glyphs, long glyphCount)
{
	register short count = glyphCount - 1;
	uint16 firstCode, entryCount;
	int16* origGlyphs = glyphs;

        firstCode = (uint16)SWAPWINC(map);
        entryCount = (uint16)SWAPWINC(map);

	for (; count >= 0; --count)
	{
		uint16 charCode = *charCodes++ - firstCode;
		if ( charCode < entryCount )
			*glyphs++ = (int16)SWAPW(map[charCode]);
		else
			*glyphs++ = 0; /* missing char */
	}

	return glyphs - origGlyphs;
}
#endif /* PCLETTO */
#endif /* PROC_TRUETYPE */

