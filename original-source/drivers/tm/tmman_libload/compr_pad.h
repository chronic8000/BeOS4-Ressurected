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
 *  Module name              : compr_pad.h    1.5
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Add padding bytes
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:13 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	When compressed, code segments are combined to make an 
 *	executable image the branch target restiction of the
 *	TM1 has to be obeyed. That is branch targets will not 
 *	overlap or end at a cache line boundary. Whenever this 
 *	would happen padding bits have to be inserted. This 
 *	module provides the interface to that.
 *
 *	There is one interface routine that takes two compressed
 *	code segments and the offset of the first code segment 
 *	in a cache line and then adds padding bits when necessary.
 *	Control flow fall thru is supported by inserting a nop cycle
 *	whenever appropriate.
 */

#ifndef _COMPR_PAD_H_
#define _COMPR_PAD_H_

#include "asm.h"

/*
 * Function         : PAD_pad adds padding bits to bitstr1. 
 *                    For this, it needs the offset in a cacheline where 
 *                    bitstr1 starts and then together with the length of 
 *                    the first bitstring the padding can be determined. 
 *                    When fall_thru for bitstr1 is true (that is bitstr2 
 *                    is supposed to be the next physical code segment), 
 *                    the format code of the branchtarget (bitstr2) is 
 *                    put in the last instruction of the previous bitstring 
 *                    (bitstr1). When the branchtarget alignment does 
 *                    not allow a direct copy of bitstr2 after bitstr1, a 
 *                    bitstring with a nop cycle is added to bitstr1.  
 *                    The size if this bitstring will be at least the gap 
 *                    to fill (from the end of the last instruction of 
 *                    bitstr to the end of the cache line). 
 *                    The added nop will be considered to be a part of 
 *                    the first bitstring, therefore, the appropriate
 *                    format bits are set in the last instruction of 
 *                    bitstr1 and in the added nopcycle the branchtarget 
 *                    for bitstr2 is encoded.
 * Parameters       : bitstr1      (I) first bitstring
 *                    cache_offset (I) bitstr1 starts at cache_offset 
 *                                     from a cache boundary 
 *                    bitstr2      (I) bitstring to append
 * Function result  : void result, however:
 *                    bitstr1 got padded. A nop cycle could be added when
 *                    fall_thru is true, zero bytes can be added to bitstr1,
 *                    or the function does nothing when no padding is 
 *                    necessary. When padding is added this can be seen 
 *                    in the length field of bitstr1.
 * Precondition     : Assumes that there is enough space in bitstr1 to
 *                    add either padding bytes or a nop cycle. 
 *                    At most MAX_IL bytes are padded.       
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void
PAD_pad( ASM_ComprCodeSegment, UInt32, ASM_ComprCodeSegment );

#endif
