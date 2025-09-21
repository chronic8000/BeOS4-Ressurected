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
 *  Module name              : compr_operation.h    1.6
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : (de)compress operations
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:15 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	The module implements a part of the compression as written 
 *	down in the document "TM-1 compressed instruction format" 
 *	by Eino Jacobs of date 95/6/23. This module specifically 
 *	implements table 5. The module has three interface functions, 
 *	one to initialize the module, one to compress an operation, 
 *	and one to decompress an operation. 
 *	
 *	Rather then making the implementation very generic and all 
 *	purpose it has hardcoded knowledge about offset etc. This 
 *	was done to keep things simple and understandable (the old 
 *	compressor was generic, not easy to understand and slow, 
 *	my intention was to overcome these problems at the cost of 
 *	genericity).
 *	
 *	It happens to be that Table 5 as it is written down in 
 *	the document is not very suited for implementation (it has 
 *	the confusing assumption that bytes have little endian bits, 
 *	that is the left most bit in a byte is the least significant 
 *	bit). So, next to that document you need the TABLES.TXT file 
 *	to understand this implementation. This module's initialization 
 *	contains most of the functional description of the table, then 
 *	the implementation of compress/decompress actually uses the bits 
 *	and sizes to do its work
 */

#ifndef _COMP_OPERATION_H
#define _COMP_OPERATION_H

#include "tmtypes.h"
#include "asm.h"
#include "compr_format.h"

typedef struct CMOP_ComprOperation{
	UInt8		nrof_extension_bytes;	/* 0, 1, 2 */
	UInt8		twobit;		/* two bit part */
	UInt8		bytes[5];	/* avoid exporting compr_util.h 
					 * (LONG_OP_LENGTH - 2) / BPB 
					 */
} *CMOP_ComprOperation;


/*
 * Function         : initialize tables needed for fast compression
 * Parameters       : -
 * Function result  : -
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */
 
extern void CMPOP_init( void );


/*
 * Function         : compress an operation.
 * Parameters       : fmt (I) format of the operation, see compr_format.h
 *                    op  (I) operation to compress.
 *                    cop (I) zero'ed compressed operation.
 * Function result  : compressed operation.
 * Precondition     : cop point to a zero'ed compressed operation.
 * Postcondition    : -      
 * Sideeffects      : -  
 */
 
extern void
CMPOP_compress( FMT_Format fmt, ASM_Operation op, CMOP_ComprOperation cop );


/*
 * Function         : decompress an operation.
 * Parameters       : fmt    (I) format code of the operation (26/34/42 bytes)
 *                    twobit (I) two bit piece of the operation.
 *                    op_start (I)  index in the 'bytes'-array of a compressed
 *                                  code segment where the operation bytes
 *                                  start.
 *                    ext_start (I) index in the 'bytes'-array of a compressed
 *                                  code segment where the extension bytes
 *                                  start.
 * Function result  : decompressed operation.
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */
 
extern ASM_Operation
CMPOP_decompress( FMT_Code fmt, UInt8 twobit, UInt8 *op_start, UInt8 *ext_start);

#endif
