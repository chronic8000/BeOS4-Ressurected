/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : compr_scatter.h    1.6
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : make scatter table entries
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:11 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	This module creates the scatter table entries as they are 
 *	defined by the oibject modul format (TMObj.h). It is used 
 *	only internally in the compressor library. There are two 
 *	functions, one to make an entry, one to print it out (this 
 *	is only for debugging purposes). The scatter entry is 
 *	created with base the index of the current operation in 
 *	the compressed bitstream. 
 */

#ifndef _COMPR_SCATTER_H
#define _COMPR_SCATTER_H

#include "TMObj.h"
#include "tmtypes.h"

/*
 * Function         : compressor library internal function
 * Parameters       : fmt       (I) format of the operation (see compr_format.h)
 *                    opnr      (I) operation number, count from 0
 *                    op_start  (I) index in 'bytes'-array of the compressed 
 *                                  instruction where the operation starts
 *                    op_ext_start (I) index in 'bytes'-array of the compressed 
 *                                   instruction where the extension bytes 
 *                                   for this instruction start
 * Function result  : generates a scatter table entry, see TMObj.h. Base of
 *                    the scatter is the offset of the operation in the
 *                    compressed bitstream 
 * Precondition     :        
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern TMObj_Scatter
SCT_make_scatter(FMT_Format fmt, Int32 opnr, Int32 op_start, Int32 op_ext_start);


/*
 * Function         : print the value of a scatter, debug function
 * Parameters       : csegment (I) code segment where the scatter applies to
 *                    op_start (I) start of the operation on which the
 *                                 scatter applies.
 *                    sct      (I) the scatter table entry.
 * Function result  : print a 32 bit value on stdout.
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void
SCT_print(ASM_ComprCodeSegment csegment, Int32 op_start, TMObj_Scatter sct);

#endif
