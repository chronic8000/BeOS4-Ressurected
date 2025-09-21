/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
* $Header: //bliss/user/archive/rcs/fit/finfotes.c,v 6.1 97/04/09 11:57:30 shawn Exp $
*
* $Log:	finfotes.c,v $
 * Revision 6.1  97/04/09  11:57:30  shawn
 * Includes in the TDIS 6.0 release.
 * Changed btstypes.h to speedo.h and rearranged pclhdrs.h and finfotbl.h #includes
 *    to avoid a DOS compiler warning on the redefinition of O_BINARY.
 * 
 * Revision 6.0  97/03/17  17:36:17  shawn
 * TDIS Version 6.00
 * 
 * Revision 4.3  97/03/13  11:15:41  shawn
 * Moved #ifndef INTEL below #include "speedo.h" where INTEL is defined.
 * 
 * Revision 4.2  96/12/05  11:08:15  shawn
 * Added support for the LJ5/LJ5M/LJ6L printers.
 * 
 * Revision 4.1  96/04/30  19:04:11  roberte
 * Improvement of printf display showing font index information.
 * 
 * Revision 4.0  95/02/15  16:49:23  shawn
 * Release
 * 
 * Revision 3.4  95/02/13  10:32:41  roberte
 * Upgraded to ANSI.
 * 
 * Revision 3.3  95/01/10  08:58:03  roberte
 * Updated copyright header.
 * 
 * Revision 3.2  95/01/04  16:30:49  roberte
 * Release
 * 
 * Revision 3.1  95/01/04  11:45:11  roberte
 * Release
 * 
 * Revision 2.2  94/11/14  13:35:45  roberte
 * No more stdef.h, use btstypes.h for data types.
 * 
 * Revision 2.1  94/07/20  08:54:54  roberte
 * Reversed the order of the show-all loop, now starts with first font,
 * and skips support fonts. Shows complete search path for each primary font.
 * Makes a more sensible display, especially when INCL_MFT is on.
 * 
 * Revision 2.0  94/05/04  09:25:37  roberte
 * Release
 * 
 * Revision 1.22  94/04/08  11:27:55  roberte
 * Release
 * 
 * Revision 1.1  93/04/06  09:55:30  roberte
 * Initial revision
 * 
 * Revision 1.14  93/03/16  12:59:41  roberte
 * Added error (usage) report if not enough arguments passed.
 * 
 * Revision 1.13  93/03/15  12:40:38  roberte
 * Release
 * 
 * Revision 1.4  93/02/12  16:27:35  roberte
 * Added #define of O_BINARY and use of it in the "open" call
 * to support PC applications using Microsoft or Borland C.
 * 
 * Revision 1.3  92/10/01  19:59:45  laurar
 * add include files for INTEL
 * 
 * Revision 1.2  92/09/26  14:14:04  roberte
 * Added copyright header and RCS markers. Added
 * comments, so that character dropout cascade code
 * will be clearer.
 * 
*****************************************************************************/
/***********************************************************************************************
    FILE:        FINFOTEST.C
    PROJECT:    4-in-1
    PURPOSE:    Tests binary files (Font Information Tables) for various
                font configurations for the 4-in-1 project.
    METHOD:     Accepts binary file names on the command line.  First file
				name should be a core FIT table binary file.  Adds subsequent
				tables on top.  After loading, test the missing character
				cascade, printing out either font name (if FONT_NAME is defined)
				or Bitstream font ID's to stdout.
    AUTHOR:        RJE @ Bitstream Inc.
    CREATED:    08-14-92
***********************************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef INTEL
#include <stddef.h>
#endif

#include "speedo.h"
#include "pclhdrs.h"
#include "finfotbl.h"
      
#ifndef INTEL
#define O_BINARY 0
#endif

/*
	Globals:
*/

FontInfoType gFontInfoTable[N_LOGICAL_FONTS];
char name_buf[17];

/****************************************************************************
*	main()
*	
****************************************************************************/
FUNCTION int main(int argc, char *argv[])
{
int i, nFonts, curIdx;
size_t sizeRead;
pclHdrType *font_head;
unsigned long nextCode;
int argIdx = 1;
FILE *fp;
	nFonts = 0;
	if (argc == 1)
		{/* must have some files on the command line! */
		fprintf(stderr, "Usage: %s file1 <file2> ... <fileN>\n	file1..fileN are names of FIT binary files made with the fitutil program.\n", argv[0]);
		exit(1);
		}
	while (argIdx < argc)
		{/* open each FIT binary file on the command line: */
		fp = fopen(argv[argIdx], "rb");
		if (fp)
			{
			sizeRead = fread(&gFontInfoTable[nFonts], 1, (size_t)sizeof(gFontInfoTable), fp);
			fclose(fp);
			nFonts += sizeRead / sizeof(FontInfoType);
			printf("%s +", argv[argIdx]);
			}
		else
			fprintf(stderr, "Unable to open %s for read.", argv[argIdx]);
		++argIdx;
		}
	printf("\n\n");

#ifdef OLDWAY
	for (i = nFonts - 1 ; i >= 0; i--)
		{/* for each font in the table, bottom to top: */
#else
	for (i = 0 ; i < nFonts; i++)
		{/* for each font in the table, top to bottom: */
#endif
		curIdx = i;
/*-----
		if ((ufix8)gFontInfoTable[curIdx].pdlType == (ufix8)pdlSupport)
			continue;
-----*/
#ifdef FONT_NAME
		printf("%3d %s->", i, gFontInfoTable[curIdx].stringName);
#else
		printf("%3d %4d->", i, gFontInfoTable[curIdx].pclHdrInfo.font_number);
#endif
		/* pretend failed getting a char from font [curIdx] */
		nextCode = gFontInfoTable[curIdx].nextSearchEncode;
		while (!(nextCode & NEXT_NONE))
			{/* while not at end of encoded tree search: */
			if (nextCode & NEXT_ABSTARG)
				{/* next target is absolute -> core RFS */
				curIdx = (nextCode ^= NEXT_ABSTARG);
				}
			else if (nextCode & NEXT_RELTARG)
				{/* next target relative to current position */
				curIdx += (char)(nextCode ^= NEXT_RELTARG);
				}
#ifdef FONT_NAME
			/* print out the font alias name: */
			printf("%s->", gFontInfoTable[curIdx].stringName);
#else
			/* print out the font Bitstream ID: */
			/*
			printf("%4d->", gFontInfoTable[curIdx].pclHdrInfo.font_number);
			*/
			printf("%4d[%2d]->", gFontInfoTable[curIdx].pclHdrInfo.font_number, curIdx);
#endif
			/* again, pretend failed getting a char from font [curIdx]: */
			nextCode = gFontInfoTable[curIdx].nextSearchEncode;
			}/* eo while */
		printf(" FINISHED\n");	 /* end of the line */
		}
	printf("\f"); /* nice if output directed to printer */
	exit(0);
}

/* eof finfotest.c */
