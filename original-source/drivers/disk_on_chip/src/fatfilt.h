/*
 * $Log:   V:/Flite/archives/Flite/src/fatfilt.h_V  $
 * 
 *    Rev 1.1   Feb 03 2000 17:02:56   danig
 * Everything under: #ifdef ABS_READ_WRITE
 * 
 *    Rev 1.0   Jul 07 1999 21:11:50   marinak
 * Initial revision
 */

#ifndef FAT_FILT
#define FAT_FILT

#include "flreq.h"

#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*                            f f C a l l                               */
/*                                                                      */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.   */
/*                                                                      */
/* Parameters:                                                          */
/*      function	: Block device driver function code 	        */
/*                                 (listed below)                       */
/*      ioreq		: IOreq structure				*/
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

typedef unsigned ffFunctionNo;

extern FLStatus ffCall(ffFunctionNo functionNo, IOreq FAR2 *ioreq);

#define ffCheckBeforeWrite(ioreq) ffCall(0,ioreq)
#define MAX_FATFILT_HEAP 2

#endif  /* ABS_READ_WRITE */
#endif
