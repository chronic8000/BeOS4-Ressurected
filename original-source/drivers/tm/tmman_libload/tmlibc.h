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
 *  Module name:	tmlibc.h	1.6
 *  Last update:	11:34:30 - 98/01/09
 *  Description:	This module contains prototypes for some functions
 *			from the TCS standard C library libc.a
 *			which are not declared by the usual "standard" headers.
 *  Revision:		970717	Initial version.
 */
 
#if	!defined(__TMLIBC_H__)
#define	__TMLIBC_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

/* ---------------------------- Includes --------------------------- */

#include <stddef.h>
#include <tmlib/tmtypes.h>

/* ---------------------------- Functions -------------------------- */

/*
 * Function:	_dcball(): data cache copyback
 * Parameters:	none
 * Result:	none
 * Side effect:	Copyback of entire data cache.
 * Note:	This routine is noninterruptible!
 */
extern	void	_dcball(void);

/*
 * Function:	_dclr(): data cache clear
 * Parameters:	none
 * Result:	none
 * Side effect:	Clears entire data cache, discarding its contents.
 * Note:	This routine is noninterruptible!
 */
extern	void	_dclr(void);

/*
 * Function:	_dlock(): data cache lock
 * Parameters:	address	(I)	base address of block to clear
 *		size	(I)	size of block to clear
 * Result:	none
 * Side effect:	Locks size bytes starting at address in the data cache.
 * Note:	Implements software workaround for hardware
 *		data cache locking bug.
 */
extern	void	_dlock(UInt32 address, UInt32 size);

/*
 * Function:	_cache_copyback(): copyback a range of memory (to SDRAM)
 * Parameters:	start_address	(I)	base address of block to copyback
 *		size	        (I)	size of block to copyback
 * Result:	none
 * Side effect:	copyback size bytes starting at start_address in the data cache.
 */
void		_cache_copyback(void *start_address, int size);

/*
 * Function:	_cache_invalidate(): invalidate a range of memory
 * Parameters:	start_address	(I)	base address of block to invalidate
 *		size	        (I)	size of block to invalidate
 * Result:	none
 * Side effect:	invalidate size bytes starting at start_address in the data cache.
 */
void		_cache_invalidate(void *start_address, int size);

/*
 * Function:	_cache_malloc(): allocate D-cache aligned memory block
 * Parameters:	size	(I)	request size of block to be allocated
 *		set	(I)	desired cache set of the result (see Note)
 * Result:	pointer to the allocated memory block
 * Note:        If set is not ANYSET (-1), set % NSETS (32) gives the desired
 *              cache set of the result.  The requested size is rounded up to
 *              NBLOCK (64) multiple.  The result can be freed with _cache_free,
 *              but not with the standard free.
 */
void		*_cache_malloc(size_t size, int set);

/*
 * Function:	_cache_free(): frees D-cache aligned memory block
 * Parameters:	ptr	(I)	pointer to block to be freed
 * Result:	none
 * Note:        the memory block must have been allocated with _cache_malloc,
 *              and must not be blocks allocated with the standard malloc.
 */
void		_cache_free(void *ptr);

/*
 * Function:	_add_free(): add non-malloc'ed memory block to memory free list
 * Parameters:	ptr	(I)	pointer to block to be freed
 *		size	(I)	size of block to be freed
 * Result:	none
 * Note:	the memory block must ->not<- have been allocated with malloc.
 */
void		_add_free(void *ptr, size_t size);


/*
 * Function:	_long_udiv(): In place 64/32 bit unsigned integer division
 *                               n := ( n[1] * 2**32 +  n[0] ) / d
 * Parameters:	n	(IO)	64 bit unsigned divident
 *		d	(I)	32 bit unsigned divisor
 * Result:	none (result placed back into n)
 * Note:	The only possible error is d==0 (division by zero).
 *              However, similar to normal integer division in C 
 *              there is no possibility of detecting this other than
 *              checking d before or after a call to this function.
 *              In case of division by zero, this function completes
 *              with n undefined.
 *
 *              This function completes in a constant time of about
 *              84 instruction cycles, or 490 cycles with cache effects
 *              taken into account. The corresponding numbers for 32 bit 
 *              integer division in the TriMedia SDK are 65 / 250.
 *
 *              Note that 32 * 32 -> 64 bit integer multiplication
 *              can be performed by:
 *
 *                     #include <custom_defs.h>
 *
 *                     UInt a,b,result[2];
 *
 *                     result[0]= a * b;
 *                     result[1]= UMULM(a,b);
 *                     
 */
void _long_udiv( UInt n[2], UInt d);



#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* !defined(__TMLIBC_H__) */

/* end of tmlibc.h */
