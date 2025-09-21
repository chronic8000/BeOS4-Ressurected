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
 *  Module name              : compr_segment.h    1.5
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : (de)compress code segments (dtrees)
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:19 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	This module is the interface the the compressor. It can 
 *	compress and decompress a code segment (read dtree) at 
 *	a time. It is based on three other modules for, (de)compress 
 *	operations, determining format codes, and creating scatter 
 *	table entries. The module has 7 interface functions, 
 *	
 *	1   to initialize the module
 *      2/3 to create delete compressed code segments
 *      4   to compress a code segment
 *      5   to decompress a compressed code segment 
 *      6   to create a compressed code segment of a given size, consisting
 *          of nop cycles only (this is used for control flow fall thru)
 *      7   find the start of the last instruction in a compressed code 
 *          segment.
 */

#ifndef _COMPR_SEGMENT_H
#define _COMPR_SEGMENT_H

#include "tmtypes.h"
#include "asm.h"

/*
 * Function         : initialize the module
 * Parameters       : -
 * Function result  : -
 * Precondition     : -       
 * Postcondition    : -      
 * Sideeffects      : -   
 */

extern void 
CMPSEG_init(void);


/*
 * Function         : Allocate initialized memory for a CMPASM_ComprCodeSegment
 *                    Compressed instructions should not be larger 
 *                    than instructions, therefore allocate the nrof 
 *                    instruction * max length of one compressed instruction.
 *                    Add enough space to add a nop cycle or padding bits. 
 * Parameters       : nrof_inst (I) number of instructions in the code segment
 * Function result  : zero'ed piece of memory
 * Precondition     : -      
 * Postcondition    : -      
 * Sideeffects      : -   
 */

extern ASM_ComprCodeSegment
CMPSEG_alloc( Int nrof_inst );

extern void
CMPSEG_free( ASM_ComprCodeSegment cinstr );


/*
 * Function         : CMPSEG_compressed_nop, generates a nop instruction
 *                    of at least a given length. The FMT_Code bits in this
 *                    instruction represent the code for this instruction 
 *                    opposed to the next instruction (which is not there)
 * Parameters       : length      (I) resulting instruction length should at 
 *				      least be length bytes.
 * Function result  : compressed nop instruction of at least a certain length. 
 *                    FMT_Code bits in the instruction represent this nop,
 *                    rather then the format for the next instruction.
 * Precondition     : -       
 * Postcondition    : -      
 * Sideeffects      : claims memory for which the caller is responsible to
 *                    for release.   
 */

extern ASM_ComprCodeSegment
CMPSEG_compressed_nop( Int length );


/*
 * Function         : CMPSEG_start_of_last, calculates the offset of the
 *                    last instruction in a compressed instruction.
 * Parameters       : csegment  (I) compressed bitstring
 * Function result  : offset in csegment->bits start starts the last 
 *                    instruction.
 * Precondition     : -       
 * Postcondition    : -      
 * Sideeffects      : -   
 */

extern UInt32
CMPSEG_start_of_last( ASM_ComprCodeSegment csegment );

#endif
