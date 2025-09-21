/*****************************************************************************
*                                                                            *
*                       Copyright 1993 - 95                                  *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ D B G . H *******************************
 *                                                                           *
 * Character shape player debug printing functions header file               *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_DBG.H 7.2 1997/03/18 16:04:22 SHAWN release $
 *                                                                                    
 *     $Log: CSP_DBG.H $
 *     Revision 7.2  1997/03/18 16:04:22  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:04:52  mark
 *     Release
 * Revision 6.1  96/08/27  11:57:25  mark
 * Release
 * 
 * Revision 5.1  96/03/14  14:20:26  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:20  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:59  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:54  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:44:44  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/


#define SHOW(X) printf(#X " = %ld\n", (long)(X))

GLOBAL_PROTO 
void ShowPfrHeaderData(
    pfrHeaderData_t *pPfrHeaderData);

GLOBAL_PROTO 
void ShowLogFontDir(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO 
void ShowPfrTable(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO 
void ShowBmapCharTable(
    cspGlobals_t *pCspGlobals);

#if CSP_MAX_DYNAMIC_FONTS > 0
GLOBAL_PROTO 
void ShowPhysFontTable(
    cspGlobals_t *pCspGlobals);
#endif

GLOBAL_PROTO 
void ShowFontIDtable(
    cspGlobals_t *pCspGlobals,
    fix31 fontIDtableSize);

