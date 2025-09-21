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
 *  +-------------------------------------------------------------------+ *
 *  Module name              : asm.h    1.7
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : assembler library data structures
 *
 *  Author                   : Rogier Wester
 *
 *  Last update              : 08:37:06 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : 
 *
 *	This module provides the data structures for TriMedia assembly 
 *	code. It defines the data structures and creation/deletion routine, 
 *	as well as generic print routines. 
 *
 * 	THIS MODULE ASSUMES THAT A MACHINE DESCRIPTION FILE IS PARSED
 *	
 *	This module provides three data structures:
 *
 *	ASM_Operation: lowest level part of the interface, each operation 
 *         has a obvious fileds like opcode, src1 .. dest register etc. 
 *         Special fields are:
 *         'offset' and 'scatter': 'offset' which is filled in when 
 *             an operation is compressed. This is the position for which 
 *             the scatter table entry is build.
 *         'info': this is a generic hook for users of the library. 
 *             All application specific information can be dumped in a
 *             struct named struct operinfo and can be attached to the 
 *             operation.
 *
 *	ASM_Instruction: group of operations. Instructions are linked 
 *         via a singly linked list ('next' field). Never fill in the 
 *         next field yourselve but use calls to do so. There is a 
 *         special field 'info' which is a hook for users of the 
 *         library. The user can fill in any information in that struct.
 *
 *	ASM_CodeSegment: list of instructions. Intructions can be 
 *         added to a code segment via library calls. Code segments 
 *         can be linked after each other via library calls.
 *
 *	Interfaces on the data structures are 
 *	1. Inialize the module (ASSUMES A MACHINE DESCRIPTION FILE IS PARSED)
 *	2. create/delete operation/instruction/code segment
 *	3. add instructions to a code segment
 * 	4. link two code segments
 *      5. print operation/instruction/code segment
 */

#ifndef _ASM_H
#define _ASM_H


#include "tmtypes.h"
#include "machdesc.h"
#include "TMObj.h"

typedef struct ASM_Operation	*ASM_Operation;
typedef struct ASM_Instruction	*ASM_Instruction;
typedef struct ASM_CodeSegment	*ASM_CodeSegment;
typedef struct ASM_ComprCodeSegment	 *ASM_ComprCodeSegment;

/* 
 * The assembler library allows for a memory manager
 * to be installed by the caller
 */
typedef Char *(*ASM_MallocFunc)	(Int size);
typedef void  (*ASM_FreeFunc)	(Char *);

extern ASM_MallocFunc	ASM_MALLOC;
extern ASM_FreeFunc	ASM_FREE;


/* 
 * generic nop operation.
 */
extern ASM_Operation	ASM_NOP_operation;


#define OPS_PER_INSTR	5	/* Machine.issueslots, asserted to be 5 */


	/*
	 * struct aexpr_rec open to be defined by the user of this library
	 */
struct ASM_Operation {
	struct operinfo	*info;		/* data hook for any user */
	MD_Opcode 	opcode;
	long		param;		/* modifier, iff opcode is parametric */
	MD_Register  	guard;		/* guard register */
	MD_Register  	arg1;		/* src1 register */       
	MD_Register  	arg2;		/* src2 register */        
	MD_Register  	dest; 		/* destination register */
	long		offset;		/* offset that starts this operation
					 * in the compressed instruction stream.
					 * Only valid after compression,
					 * -1 for nop
					 */
	TMObj_Scatter	scatter;	/* scatter table entry filled in by the
					 * compressor. All numbers are from the
					 * start of the operation (that is 
					 * start + offset)
					 */
};

struct ASM_Instruction {
	struct instrinfo *info;		/* data hook for any user */
	ASM_Instruction	next;
	ASM_Operation 	operations[OPS_PER_INSTR]; 
};

struct ASM_CodeSegment {
	ASM_Instruction	instructions_first;	/* first ASM_Instruction */
	ASM_Instruction	instructions_last;	/* last ASM_Instruction */
	ASM_CodeSegment	next;
};


struct ASM_ComprCodeSegment {
	Bool		fall_thru;	/* keep with next CMPINSTR_ComprInstr */
	UInt32		length;		/* length of compressed bitstring */
	/* no padding, but make sure bytes starts 4 byte aligned */
	UInt8		*bytes;		/* the actual bitstring, allocate  
					 * enough for padding or a nop cycle
					 */
};



/*
 * Function         : create an operation, zero the memory
 * Parameters       : -
 * Function result  : initialized memory
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern ASM_Operation	ASM_create_operation(void);
extern void		ASM_delete_operation(ASM_Operation);

/*
 * Function         : create an nop operation
 * Parameters       : -
 * Function result  : -
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern ASM_Operation	ASM_create_nop(void);

/*
 * Function         : create an instruction, zero the memory
 * Parameters       : -
 * Function result  : initialized memory
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern ASM_Instruction	ASM_create_instruction(void);
extern void		ASM_delete_instruction(ASM_Instruction);

/*
 * Function         : create a code segment, zero the memory
 * Parameters       : -
 * Function result  : initialized memory
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern ASM_CodeSegment	ASM_create_code_segment(void);
extern void		ASM_delete_code_segment(ASM_CodeSegment);

/*
 * Function         : add an instruction at the end of a code segment.
 * Parameters       : code_segment (IO) code segment at which the instruction
 *                                      has to be attached.
 *                    instruction  (I)  instruction to attach
 * Function result  : -
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void
ASM_code_segment_add_instruction(ASM_CodeSegment, ASM_Instruction);

/*
 * Function         : add a code segment at the end of a list of code segments.
 * Parameters       : first (I0) code segment at which the code segment
 *                               has to be added.
 *                    last  (I)  code segment to add to the list
 * Function result  :
 * Precondition     : 
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void
ASM_code_segment_add_code_segment(ASM_CodeSegment, ASM_CodeSegment);


/*
 * Function         : print operation/instruction/code segments to
 *                    a file.
 * Parameters       :  
 * Function result  :
 * Precondition     : 
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void ASM_print_operation(ASM_Operation oper);
extern void ASM_print_instruction(ASM_Instruction instr);
extern void ASM_print_code_segment(ASM_CodeSegment segm);
extern void ASM_change_outfile(FILE *outfile);


/*
 * Function         : Initialize the library. Specify memory allocation 
 *                    and freeing routines. 
 * Parameters       : outfile   (I) default file to output to
 *                    mem_alloc (I) memory allocation routine.
 *                    mem_free  (I) memory free routine.
 * Function result  :
 * Precondition     : machine description file is parsed.
 * Postcondition    : -      
 * Sideeffects      : -  
 */

extern void ASM_init(FILE *, ASM_MallocFunc, ASM_FreeFunc);



/*
 *
 *	EXPORTED BY compr_segment
 *
 */



/*
 * Function         : CMPSEG_compress, compresses a code segment.
 * Parameters       : segment   (I) the code segment to be compressed
 *	              fall_thru (I) whether code segment is 'keep with next'
 * Function result  : compressed instruction stream
 * Precondition     : -       
 * Postcondition    : -      
 * Sideeffects      : The scatter entries in the operations get filled in 
 *                    with a non-NULL pointer that poits to a scatter table
 *                    entry.
 *                    claims memory for which the caller is responsible to
 *                    for release.
 */

ASM_ComprCodeSegment
ASM_compress( ASM_CodeSegment segment, Bool fall_thru );



/*
 * Function         : decompress_instructions, decompresses an unshuffled
 *                    instruction stream, does not detect branch targets.
 * Parameters       : lenght (I) The length in bytes of the bitstream
 *                               to be decompressed
 *                    bytes  (I) The actual bytes.
 * Function result  : decompressed instruction stream, possible NULL on 
 *                    unsuccessful decompression.
 * Precondition     : -       
 * Postcondition    : -
 * Sideeffects      : -   
 */

ASM_CodeSegment
ASM_decompress( UInt32 length, UInt8 *bytes );


#endif
