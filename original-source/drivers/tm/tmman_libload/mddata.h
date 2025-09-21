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
 *  Module name              : mddata.h    1.4
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Machine description file data
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:27 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	For each opcode supply the basic format type (see compr_format.h). 
 *	The basic format does not short opcodes, branch targets, 
 *	and unguardedness of operations into account.
 *
 *	This module is only used internally in the compressor library
 *	and assumes that the machine description file is parsed.
 */

#ifndef _COMPR_MDD_H
#define _COMPR_MDD_H

#include "tmtypes.h"

typedef enum {
	BASIC_BINARY			= 0,
	BASIC_BINARY_RESULTLESS		= 1,
	BASIC_BINARY_PARAM7_RESULTLESS	= 2,
	BASIC_UNARY			= 3,
	BASIC_UNARY_PARAM7		= 4,
	BASIC_UNARY_RESULTLESS		= 5,
	BASIC_UNARY_PARAM7_RESULTLESS	= 6,
	BASIC_ZEROARY			= 7,
	BASIC_ZEROARY_PARAM32		= 8,
	BASIC_ZEROARY_PARAM32_RESULTLESS_1= 9,
	BASIC_ZEROARY_PARAM32_RESULTLESS_2= 10,
	BASIC_ZEROARY_RESULTLESS	= 11
#define MAX_BASIC_FORMAT	  	  12
} MDD_BasicFormat;

#define MAX_OPCODES		256
#define MAX_SHORT_OPCODE	32


	/*
	 * Per opcode the basic opcode type, as in MAX_BASIC_FORMAT
	 * the S and L bits in table 5 (compression document)
	 */
extern MDD_BasicFormat	MDD_type[MAX_OPCODES];
extern UInt8		MDD_S[MAX_OPCODES];
extern UInt8		MDD_L[MAX_OPCODES];

/*
 * Function         : Initialize the library.  
 * Parameters       : -
 * Function result  : -
 * Precondition     : machine description file is parsed.
 * Postcondition    : -      
 * Sideeffects      : -  
 */
extern void MDD_init(void);

#endif
