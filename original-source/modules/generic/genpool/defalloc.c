/* :ts=8 bk=0
 *
 * defalloc.c:	Default allocation logic
 *
 * Leo L. Schwab					1999.08.27
 */
#include <kernel/OS.h>
#include <surface/genpool.h>

#include "metapool.h"


/*****************************************************************************
 * Prototypes.
 */
extern struct mempool *lookuppool (uint32 poolid);
extern status_t markrange (struct mempool *mp,
			   struct BMemRange *cl_mr,
			   void *userdata,
			   uint32 userdatasize);
extern status_t unmarkrange (struct mempool *mp, uint32 offset);

uint32 align_up (uint32 base_orig, uint32 carebits, uint32 statebits);
uint32 align_down (uint32 base_orig, uint32 carebits, uint32 statebits);

static int highbit (uint32 val);


/*****************************************************************************
 * Allocation logic.
 */
status_t
allocbymemspec (struct BMemSpec *ms)
{
	mempool		*mp;
	BRangeNode	*freenode, *newnode;
	status_t	retval;
	uint32		freebase, allocbase;
	uint32		carebits, statebits;
	uint32		size;
	void		*base;

	if (!(mp = lookuppool (ms->ms_PoolID)))
		return (B_POOLERR_UNKNOWN_POOLID);

	if (!(size = ms->ms_MR.mr_Size))
		return (B_BAD_VALUE);

	if (carebits = ms->ms_AddrCareBits)
		statebits = ms->ms_AddrStateBits & carebits;

	acquire_sem (mp->mp_pi.pi_PoolLock);	/*  FIXME: timeout?  */

	freenode = NULL;
	base = mp->mp_pi.pi_RangeLists;

//printf ("About to surf freelists...\n");
	if (ms->ms_AllocFlags & B_MEMSPECF_FROMTOP) {
		/*
		 * Walk MemList backwards, trying to allocate from top.
		 */
		for (newnode = (BRangeNode *)
				LASTNODE_OP (base, &mp->mp_freelist->rl_List);
		     PREVNODE_OP (base, newnode);
		     newnode = (BRangeNode *) PREVNODE_OP (base, newnode))
		{
			if (newnode->rn_MR.mr_Size < size)
				continue;

			freebase = newnode->rn_MR.mr_Offset;

			allocbase = freebase + newnode->rn_MR.mr_Size - size;
			if (carebits)
				allocbase =
				 align_down (allocbase, carebits, statebits);

			if (allocbase >= freebase) {
				freenode = newnode;
				break;
			}
		}
	} else {
		/*
		 * Traditional allocation from bottom-up.
		 */
		for (newnode = (BRangeNode *)
				FIRSTNODE_OP (base,
					      &((BRangeList *) base)->rl_List);
		     NEXTNODE_OP (base, newnode);
		     newnode = (BRangeNode *) NEXTNODE_OP (base, newnode))
		{
//printf ("Looking at node 0x%08lx (offset 0x%08lx, size %d)\n",
// newnode, newnode->rn_MR.mr_Offset, newnode->rn_MR.mr_Size);
			if (newnode->rn_MR.mr_Size < size)
				/*
				 * No amount of adjustment will change
				 * this.
				 */
				continue;
	
			freebase = newnode->rn_MR.mr_Offset;
	
			allocbase =
			 carebits ? align_up (freebase, carebits, statebits)
				  : freebase;

			if (allocbase + size <=
			     freebase + newnode->rn_MR.mr_Size)
			{
				/*  Found one  */
				freenode = newnode;
				break;
			}
		}
	}

	if (freenode) {
		BMemRange	mr;

//printf ("About to mark range 0x%08lx:%d...\n", allocbase, size);
		mr.mr_Offset	= allocbase;
		mr.mr_Size	= size;
		mr.mr_Owner	= ms->ms_MR.mr_Owner;
		if ((retval = markrange (mp, &mr, NULL, 0)) == B_OK)
			memcpy (&ms->ms_MR, &mr, sizeof (mr));
	} else
		retval = B_NO_MEMORY;

	release_sem (mp->mp_pi.pi_PoolLock);

	return (retval);
}

status_t
freebymemspec (struct BMemSpec *ms)
{
	mempool		*mp;
	status_t	retval;

	if (!(mp = lookuppool (ms->ms_PoolID)))
		return (B_POOLERR_UNKNOWN_POOLID);

	acquire_sem (mp->mp_pi.pi_PoolLock);	/*  FIXME: timeout?  */
	retval = unmarkrange (mp, ms->ms_MR.mr_Offset);
	release_sem (mp->mp_pi.pi_PoolLock);

	return (retval);
}


uint32
align_up (uint32 base_orig, uint32 carebits, uint32 statebits)
{
	if (carebits  &&  (base_orig & carebits) != statebits) {
//printf ("+++ Computing: care 0x%02x, state 0x%02x, base_orig 0x%08lx\n",
// carebits, statebits, base_orig);
//fflush (stdout);
		/*
		 * This is the core of the aligned memory allocator.
		 * I spent several days working out these few lines
		 * of code.  PLEASE DO NOT FSCK WITH THEM UNLESS YOU
		 * UNDERSTAND HOW IT WORKS *AND* WHAT IT'S SUPPOSED TO
		 * BE DOING.
		 ****
		 * The object of the game here is to find the smallest
		 * value that is larger than 'base_orig' whose bits are
		 * in the state required by 'carebits' and 'statebits'.
		 ****
		 * FIXME: 64 bit architectures?
		 */
#if 0
/*  First pass; darned close...  */
		carefield = (1 << (highbit (carebits)) + 1) - 1;
		base = (base_orig & ~carefield) | statebits;
		while ((diff = base_orig - base) > 0) {
			/*  This may not be need to be iterative...  */
			diff &= carebits;
			diff += carebits;
			diff &= ~carebits;
			base += diff;
		}
#endif
		register uint32	base = base_orig;
		register uint32	diff;

		if ((base & carebits) < statebits) {
			/*
			 * A modified OR will do it.
			 */
			diff = (statebits ^ base) & carebits;

			base &= -(1 << highbit (diff));
		} else {
			/*
			 * Find bits in base which are 1 but need to be 0.
			 */
			diff = base & carebits & ~statebits;

			/*
			 * Lower order bits aren't important.
			 * Find MSB of difference.
			 */
			diff = 1 << highbit (diff);

			/*
			 * Now watch closely...
			 * ORing in 'carebits' forces carry
			 * bits though them when we...
			 */
			base = ((base_orig | carebits)
			/*  ...add the difference.  */
						       + diff)
			/*  Force bits below diff to zero.  */
							       & (-diff);
		}
		/*  Mask 'carebits' back out  */
		base_orig = base & ~carebits
		/*  and OR in required state.  */
					     | statebits;
//printf ("    new base = 0x%08lx\n", base_orig);
	}
	return (base_orig);
}

uint32
align_down (uint32 base_orig, uint32 carebits, uint32 statebits)
{
	if (carebits  &&  (base_orig & carebits) != statebits) {
		/*
		 * In this case, the object is to find the largest value
		 * that is smaller than 'base_orig' having bits in the state
		 * required by 'carebits' and 'statebits'.
		 */
		register uint32	base = base_orig;
		register uint32	diff;

		if ((base & carebits) > statebits) {
			/*
			 * A modified OR will do it.
			 */
			diff = (statebits ^ base) & carebits;

			base |= (2 << highbit (diff)) - 1;
		} else {
			/*
			 * Find bits in base which are 0 but need to be 1.
			 */
			diff = ~base & carebits & statebits;

			/*
			 * Lower order bits aren't important.
			 * Find MSB of difference.
			 */
			diff = 1 << highbit (diff);

			/*
			 * Now watch closely...
			 * ANDing out 'carebits' forces carry
			 * bits though them when we...
			 */
			base = ((base_orig & ~carebits)
			/*  ...subtract the difference.  */
							- diff)
			/*  Force bits below diff to one.  */
								| (diff - 1);
		}
		/*  Mask 'carebits' back out  */
		base_orig = base & ~carebits
		/*  and OR in required state.  */
					     | statebits;
	}
	return (base_orig);
}


/*
 * Returns bit number of most significant bit set in 'val'.
 */
#if __INTEL__
static int
highbit (uint32 val)
{
	int retval;
	__asm__ ("BSR %%edx, %%eax": "=a" (retval) : "d" (val));
	return (retval);
}

#elif __POWERPC__

// returns -1 when val == 0 on PPC, but don't rely on it for other CPUs
static asm int
highbit (uint32 val)
{
	cntlzw	r5, r3
	neg	r4, r5
	addi	r3, r4, 31
	blr
}

#else

// slow C version
static int
highbit (uint32 val)
{
	uint32	t;
	int	retval = 0;

	if (t = val & 0xffff0000)	val = t, retval += 16;
	if (t = val & 0xff00ff00)	val = t, retval += 8;
	if (t = val & 0xf0f0f0f0)	val = t, retval += 4;
	if (t = val & 0xcccccccc)	val = t, retval += 2;
	if (val & 0xaaaaaaaa)		retval += 1;

	return (retval);
}
#endif
