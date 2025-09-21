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
 *  Module name              : compr_format.h    1.6
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : format for an operation.
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:17 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	The instruction compressor uses format bits to determine 
 *	the format of all operations in the next instruction. 
 *	The first instruction of a code segment is a branchtarget 
 *	and has a special known format. This module determines for 
 *	each of the operation's opcodes and for all special conditions 
 *	(like whether opcode can be represented in 32 bits, whether 
 *	the guard is true, and whether the operation is part of a 
 *	branch target instruction) what format is to be used for 
 *	compression of the operation. 
 *
 *	This module is used internally in the compressor only.
 *	It exports one enum for all the possible formats, two
 *	arrays to determine the 2-bit format code for a format
 *	and an array to determine the size of the operation for
 *	a given 2-bit format code.
 *	Then there are two interface functions, initialize the module
 * 	and given and operation and whether it is part of a branch target
 *	the format for decompression.
 */

#ifndef _FORMAT_H
#define _FORMAT_H

#include "tmtypes.h"
#include "asm.h"
#include "compr_util.h"

typedef enum {
	FMT_BINARY_UNGUARDED_SHORT 		= 0,
	FMT_UNARY_PARAM7_UNGUARDED_SHORT 	= 1,
	FMT_BINARY_UNGUARDED_PARAM7_RESULTLESS_SHORT= 2,
	FMT_UNARY_SHORT 			= 3,
	FMT_BINARY_SHORT 			= 4,
	FMT_UNARY_PARAM7_SHORT 			= 5,
	FMT_BINARY_PARAM7_RESULTLESS_SHORT 	= 6,
	FMT_BINARY_UNGUARDED 			= 7,
	FMT_BINARY_RESULTLESS 			= 8,
	FMT_UNARY_PARAM7_UNGUARDED 		= 9,
	FMT_UNARY 				= 10,
	FMT_BINARY_PARAM7_RESULTLESS 		= 11,
	FMT_BINARY 				= 12,
	FMT_UNARY_PARAM7 			= 13,
	FMT_ZEROARY_PARAM32 			= 14,
	FMT_ZEROARY_PARAM32_RESULTLESS_1 	= 15,
	FMT_ZEROARY_PARAM32_RESULTLESS_2 	= 16,
	FMT_NOP 				= 17
#define	FMT_MAX_FORMATS				  18
} FMT_Format;


typedef enum {
	FMT_CODE_26 		= 0,
	FMT_CODE_34 		= 1,
	FMT_CODE_42 		= 2,
	FMT_CODE_NOP 		= 3,
} FMT_Code;

	/*
	 * for each format what is the size-encoding format 
	 */
extern FMT_Code FMT_code[FMT_MAX_FORMATS];

	/*
	 * per format code, how many bytes for this operation 
	 */
extern UInt8 	FMT_operation_nrof_bytes[4];

/*
 * Function         : initialize the module. 
 * Parameters       : -
 * Function result  : -
 * Precondition     : A machine description file is parsed.
 * Postcondition    : -      
 * Sideeffects      : -  
 */
 
extern void FMT_init( void );


/*
 * Function         : given an operation and whether it is part of
 *                    a branchtarget, determine the compression format.
 * Parameters       : is_branch_target (I) operation is part of a 
 *                                         branchtarget instruction
 *                    op               (I) the operation
 * Function result  : format for decompression.
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */
 
extern FMT_Format 
FMT_determine_format( Bool is_branch_target, ASM_Operation op );

#endif
