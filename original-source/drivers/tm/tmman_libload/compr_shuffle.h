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
 *  Module name              : compr_shuffle.h    1.4
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : shuffle
 *
 *  Author                   : 
 *
 *  Last update              : 08:37:21 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	After an object is fully linked and an memory image is made, 
 *	the text segment has to be 'shuffled' in order for the target 
 *	to understand the code. Shuffling is done on groups of 32 bytes 
 *	which are 32-byte aligned. For each 32 byte part a 32 byte output
 *	is produced.
 *	
 *	Shuffling can be done in place but other overlap than fully 
 *	overlapping input and output is not supported.
 */
 
#ifndef _SHUFFLE_H_
#define _SHUFFLE_H_

/*
 * Function         : SHFL_shuffle shuffles a bitsting of a specified 
 *                    number of  bytes.
 *                    A pointer to the unshuffled bitstream is passed as
 *                    well as pointer to where the output should go to.
 *                    Shuffling can be done in place, that is when the
 *                    two pointers have equal value. Other than equal
 *                    pointers (full overlap) overlap is NOT supported.
 *                    Since shuffling and unshuffling work on 32 byte units
 *                    of data, the length must be a multiple of 32.
 * Parameters       : unshuffled (I) pointer to unshuffled data
 *                    shuffled   (O) pointer to shuffled data 
 *                    length     (I) number of bytes to shuffle,
 *                                   supposed to be a multiple 32 
 * Function result  : when compiled with NDEBUG defined, the result
 *                    will always be 0. When compiled with NDEBUG un- 
 *                    defined, the length passed to the function is checked
 *                    to be a multiple of 32 bytes, and the overlap
 *                    constraint is checked. In case of an error -1 is 
 *                    returned.
 * Precondition     : 'shuffled' is a pointer to an array of at least
 *                    'long' bytes. when 'shuffled' == 'unshuffled'
 *                    the shuffling is done inplace.
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern int 
SHFL_shuffle(unsigned char *unshuffled, unsigned char *shuffled, unsigned long length);

#endif
