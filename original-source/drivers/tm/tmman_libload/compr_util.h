#ifndef _COMPR_UTIL_H
#define _COMPR_UTIL_H

#include "asm.h"

#define SHORT_OP_LENGTH		26
#define MEDIUM_OP_LENGTH	34
#define LONG_OP_LENGTH		42
#define MAX_IL			28	/* 2 + 3*3 + 1 + 2*3 + 5*2 */


#define OPS_1_GRP	3	/* nrof ops in the first operation group */
#define OPS_2_GRP	2	/* nrof ops in the ssecond operation group */

#define BPB		8	/* bits per byte */
	/*
	 * nrof bits per format, not propagated everywhere
	 */
#define FMT_BITS	2

	/*
	 * nrof bytes used for formatting the next instruction
	 */
#define FMT_BYTES	(((OPS_PER_INSTR * FMT_BITS) / BPB) + (((OPS_PER_INSTR * FMT_BITS) % BPB) != 0))

	/*
	 * Make a mask for the format byte for the last
	 * format byte (currently the second byte)
	 */
#define LAST_FMT_INV_MASK	(((signed char)0x80) >> (BPB - (OPS_PER_INSTR % 4) * FMT_BITS - 1))
#define LAST_FMT_MASK		(~LAST_FMT_INV_MASK)


	/* 
	 * assume nrof format bytes == 2
	 */
#define store_format_code(start, code0, code1)			\
{								\
	*((UInt8 *)start+0) = code0;				\
	*((UInt8 *)start+1) = (code1 & LAST_FMT_MASK) | (*((UInt8 *)start+1) & ~LAST_FMT_MASK); \
}


	/*
	 * The start of operation bits for a certain
	 * operation number. Can not be used to determine
	 * the start of the extension bytes.
	 * start counting at 0.
	 * We skip the 2 format bytes, and the 2bit byte iff idx >= OPS_1_GRP
	 */
	 
#define start_index_op(idx)	(2 + idx * OPS_1_GRP + (idx >= OPS_1_GRP))

#define start_index_ext(nrof_used_ops)	(2 + nrof_used_ops * OPS_1_GRP + (nrof_used_ops > OPS_1_GRP))

#endif
