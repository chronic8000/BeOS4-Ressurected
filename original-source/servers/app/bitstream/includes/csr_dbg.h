/*****************************************************************************
*                                                                            *
*                          Copyright 1993 - 95                               *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S R _ D B G . H *******************************
 *                                                                           *
 *  Function prototypes for debugging functions.
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *     $Header: R:/src/CSR/rcs/CSR_DBG.H 6.3 1997/03/18 15:59:39 SHAWN release $
 *                                                                                    
 *     $Log: CSR_DBG.H $
 *     Revision 6.3  1997/03/18 15:59:39  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 6.2  1996/11/15 17:15:57  john
 *     Added missing prototype.
 * Revision 6.1  96/10/10  14:08:17  mark
 * Release
 * 
 * Revision 5.1  96/08/27  11:51:13  mark
 * Release
 * 
 * Revision 4.1  96/03/14  14:15:00  mark
 * Release
 * 
 * Revision 3.1  96/03/05  13:51:01  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:50:09  mark
 * Release
 * 
 * Revision 1.2  95/12/12  14:39:46  john
 * Updated copyright date.
 * 
 * Revision 1.1  95/08/10  18:21:23  john
 * Initial revision
 * 
 * Revision 10.1  95/07/31  10:24:40  mark
 * Release
 * 
 * Revision 9.1  95/04/26  15:48:31  mark
 * Release
 * 
 * Revision 8.1  95/03/13  15:48:32  mark
 * Release
 * 
 * Revision 7.4  95/02/24  11:32:50  john
 * Changes to accomodate structure name change from bmapSpecs_t
 *     to csrBmapSpecs_t.
 * 
 * Revision 7.3  95/02/07  17:06:12  john
 * New definitions in support of bitmap recording.
 * 
 * Revision 7.2  95/01/05  17:52:41  john
 * Updated SHOW and SHOWR macros for ansi compatibility.
 * 
 * Revision 7.1  94/08/26  16:17:02  mark
 * Release
 * 
 * Revision 6.1  94/08/25  14:09:27  mark
 * Release
 * 
 * Revision 4.5  94/08/08  09:50:27  john
 * Proto for ShowFontInfo added.
 * 
 * Revision 4.4  94/07/14  16:22:01  john
 * Eliminated proto for the function ShowKeyPoint.
 * 
 * Revision 4.3  94/07/13  09:38:32  john
 * Function protos added for:
 *     CheckSegment()
 *     CheckCubicSegment()
 *     CheckCurveDepth()
 * 
 * Revision 4.2  94/07/11  13:53:59  john
 * pCsrGlobals argument added to ShowPhysicalFont().
 * 
 * Revision 4.1  94/07/05  09:55:31  mark
 * Release
 * 
 * Revision 3.1  94/06/20  09:00:30  mark
 * Release
 * 
 * Revision 2.1  94/04/25  14:43:47  mark
 * Release
 * 
 * Revision 1.4  94/04/15  14:58:07  john
 * Added prototype for ShowGlyphTree().
 * 
 * Revision 1.3  94/04/07  10:42:55  john
 * Modifications in connection with new data structures for
 *     logical and physical font records.
 * 
 * Revision 1.2  94/01/26  13:46:41  mark
 * add header with copyright and RCS tags.
 * 
 * 
 *                                                                           *
 ****************************************************************************/

#define SHOW(X) printf(#X " = %ld\n", (long)(X))
#define SHOWR(X) printf(#X " = %f\n", (float)(X))

void ShowFontInfo(
    csrFontInfo_t *pFontInfo);

void ShowPhysicalFont(
    csrGlobals_t *pCsrGlobals,
    physicalFont_t *pPhysicalFont);

void ShowLogicalFont(
    logicalFont_t *pLogicalFont);

void ShowCharTree(
    character_t *pCharacter);

void ShowSegmentPoint(
    segPoint_t *pSegment);

void ShowEdgeLists(
    contour_t *pContour);

void ShowContourTree(
    contour_t *pContour);

void ShowContourGroup(
    contour_t *pContourGroup);

void ShowContour(
    contour_t *pContour);

void ShowContourPoints(
    contour_t *pContour);

void ShowGlyphTree(
    glyph_t *pGlyphTree);

void ShowGlyph(
    glyph_t *pGlyph);

void ShowGlyphElements(
    charElement_t CharElement[], 
    int nContourGroups);

void ShowBmapInfo(
    physicalFont_t *pPhysicalFont);

void ShowBmapImage(
    csrGlobals_t *pCsrGlobals,
    csrBmapSpecs_t *pBmapSpecs,
    ufix8 *pBmapImage);

void ShowBmapCharTree(
    bmapChar_t *pBmapChar);

void ShowPfrMap(
    pfrMap_t *pPfrMap);

void CheckSegment(
    segPoint_t *pSegStart,
    segPoint_t *pSegEnd);

void CheckCubicSegment(
    fix15 x0,
    fix15 y0,
    fix15 x1,
    fix15 y1,
    fix15 x2,
    fix15 y2,
    fix15 x3,
    fix15 y3);

void CheckCurveDepth(
	fix15 depth,
    fix15  xx1,
    fix15  yy1,
    fix15  xx2,
    fix15  yy2,
    fix15  xx3,
    fix15  yy3);

void ForceBufferOverflow(void);


