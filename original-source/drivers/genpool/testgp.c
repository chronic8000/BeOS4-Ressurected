/* :ts=8 bk=0
 *
 * testgp.c:	Test to make sure my memory allocator is sane.
 *
 * gcc -I$BUILDHOME/src/kit/dinky/include -I$BUILDHOME/src/kit/surface/include testgp.c -ldinky
 *
 * Leo L. Schwab					1999.08.12
 */
#include <kernel/OS.h>
#include <surface/genpool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#ifndef	B_MEMSPECF_FROMTOP
#define	B_MEMSPECF_FROMTOP	1
#endif

#define	NNODES	4096
#define	NTESTS	2048

#define	TESTAREASIZE	(16<<20)


int32 checkfreelist (struct BPoolInfo *pi, int32 *nnodes);
int32 checkalloclist (struct BPoolInfo *pi, int32 *nnodes);
void freeseqandcheck (void **ptrs, int32 *sizes, int32 n);
void freerndandcheck (void **ptrs, int32 *sizes, int32 n);

void *testalloc (int32 size, uint32 carebits, uint32 statebits);
void testfree (void *ptr, int32 size);
status_t allocalignedrange (struct BMemRange *mr,
			    struct BPoolInfo *pi,
			    uint32 size,
			    uint32 carebits,
			    uint32 statebits,
			    uint32 allocflags);

uint32 align_up (uint32 base_orig, uint32 carebits, uint32 statebits);
uint32 align_down (uint32 base_orig, uint32 carebits, uint32 statebits);
static int highbit (uint32 val);

status_t initmem (uint32 size);
void shutdownmem (void);



static BPoolInfo	poolinfo;
static area_id		pool_aid = -1,
			rangelists_aid = -1;
static void		*gptrs[NTESTS];
static int32		gsizes[NTESTS];
static int		gPoolfd;

static int		gDEBUG;


int
main (int ac, char **av)
{
	register int	i;
	int32		size, total;
	uint32		care, state;
	status_t	retval;
	
	if ((retval = initmem (TESTAREASIZE)) < 0)
		printf ("Can't initialize memory manager: %d (0x%08lx)\n",
			retval, retval);
	printf ("Allocated %d for workspace.\n", TESTAREASIZE);

	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, complex mask (8 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		care = random () % 0xFF;
		state = random () & care;
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (&poolinfo, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (&poolinfo, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	shutdownmem ();
	return (0);
}


int32
checkfreelist (struct BPoolInfo *pi, int32 *nnodes)
{
	BRangeNode	*rn;
	int32		total = 0;
	void		*base;

	if (nnodes)	*nnodes = 0;
	base = pi->pi_RangeLists;
	for (rn = (BRangeNode *) FIRSTNODE_OP (base, &((BRangeList *) base)->rl_List);
	     NEXTNODE_OP (base, rn);
	     rn = (BRangeNode *) NEXTNODE_OP (base, rn))
	{
#if 0
if (gDEBUG) {
 printf ("RangeNode #%d @ 0x%08lx:\n", *nnodes, rn);
 printf ("      rn_n_Next: 0x%08lx\n", rn->rn_Node.n_Next);
 printf ("      rn_n_Prev: 0x%08lx\n", rn->rn_Node.n_Prev);
 printf ("    rn_UserSize: %d\n", rn->rn_UserSize);
 printf ("   rn_mr_Offset: 0x%08lx\n", rn->rn_MR.mr_Offset);
 printf ("     rn_mr_Size: %d\n", rn->rn_MR.mr_Size);
 printf ("    rn_mr_Owner: %d\n", rn->rn_MR.mr_Owner);
 printf ("  rn_mr_LockIdx: %d\n", rn->rn_MR.mr_LockIdx);
}
#endif
		total += rn->rn_MR.mr_Size;
		if (nnodes)	(*nnodes)++;
	}

	return (total);
}

int32
checkalloclist (struct BPoolInfo *pi, int32 *nnodes)
{
	BRangeNode	*rn;
	int32		total = 0;
	void		*base;

	if (nnodes)	*nnodes = 0;
	base = pi->pi_RangeLists;
	for (rn = (BRangeNode *) FIRSTNODE_OP (base, &((BRangeList *) base + 1)->rl_List);
	     NEXTNODE_OP (base, rn);
	     rn = (BRangeNode *) NEXTNODE_OP (base, rn))
	{
		total += rn->rn_MR.mr_Size;
		if (nnodes)	(*nnodes)++;
	}

	return (total);
}


void
freeseqandcheck (void **ptrs, int32 *sizes, int32 n)
{
	while (--n >= 0) {
		if (*ptrs)
			testfree (*ptrs, *sizes);
		*ptrs++ = NULL;
		*sizes++ = 0;
	}
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (&poolinfo, &n));
	printf ("Nodes remaining on free list: %d\n", n);
}

void
freerndandcheck (void **ptrs, int32 *sizes, int32 nentries)
{
	register int	i, n;

	n = nentries;
	while (--n >= 0) {
		i = random () % nentries;
		if (!sizes[i]) {
			if (i & 1) {
				do {
					if (i <= 0)		i = nentries;
				} while (!sizes[--i]);
			} else {
				do {
					if (i >= nentries - 1)	i = -1;
				} while (!sizes[++i]);
			}
		}
		if (sizes[i] > 0)
			testfree (ptrs[i], sizes[i]);
		ptrs[i] = NULL;
		sizes[i] = 0;
	}
gDEBUG = TRUE;
	printf ("Freed all allocations; reported free space: %d\n",
		checkfreelist (&poolinfo, &nentries));
	printf ("Nodes remaining on free list: %d\n", nentries);
gDEBUG = FALSE;
}


/*****************************************************************************
 * Allocation logic.
 */
void *
testalloc (int32 size, uint32 carebits, uint32 statebits)
{
	BMemSpec	ms;
	status_t	retval;

//printf ("Attempting allocation of %d bytes (0x%08lx : 0x%08lx)\n",
// size, carebits, statebits);
	memset (&ms, 0, sizeof (ms));
	ms.ms_MR.mr_Size	= size;
	ms.ms_MR.mr_Owner	= 0;
	ms.ms_PoolID		= poolinfo.pi_PoolID;
	ms.ms_AddrCareBits	= carebits;
	ms.ms_AddrStateBits	= statebits;
	ms.ms_AllocFlags	= 0;
	if ((retval = ioctl (gPoolfd,
			     B_IOCTL_ALLOCBYMEMSPEC,
			     &ms,
			     sizeof (ms))) < 0)
	{
//printf ("allocalignedrange() failed with %d (0x%08lx)\n",
// retval, retval);
		return (NULL);
	} else
		return ((void *) ((uint32) poolinfo.pi_Pool +
				  ms.ms_MR.mr_Offset));
}

void
testfree (void *ptr, int32 size)
{
	BMemSpec	ms;
	status_t	retval;

	memset (&ms, 0, sizeof (ms));
	ms.ms_MR.mr_Offset	= (uint32) ptr - (uint32) poolinfo.pi_Pool;
	ms.ms_PoolID		= poolinfo.pi_PoolID;
	if (ioctl (gPoolfd, B_IOCTL_FREEBYMEMSPEC, &ms, sizeof (ms)) < 0)
		printf ("B_IOCTL_FREEBYMEMSPEC failed: %d (0x%08lx)\n",
			errno, errno);
}


#if 0
status_t
allocalignedrange (
struct BMemRange	*mr,
struct BPoolInfo	*pi,
uint32			size,
uint32			carebits,
uint32			statebits,
uint32			allocflags
)
{
	BRangeNode	*freenode, *newnode;
	status_t	retval;
	uint32		freebase, allocbase;
	void		*base;

	if (!size)
		return (B_BAD_VALUE);

	if (ioctl (gPoolfd, B_IOCTL_LOCKPOOL, pi->pi_PoolID, 0) < 0)
		return (errno);

	if (carebits)
		statebits &= carebits;

	freenode = NULL;
	base = pi->pi_RangeLists;

//printf ("About to surf freelists...\n");
	if (allocflags & B_MEMSPECF_FROMTOP) {
		/*
		 * Walk MemList backwards, trying to allocate from top.
		 */
		for (newnode = (BRangeNode *) LASTNODE_OP (base, &((BRangeList *) base)->rl_List);
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
		for (newnode = (BRangeNode *) FIRSTNODE_OP (base, &((BRangeList *) base)->rl_List);
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
		BIoctl_MarkRange	imr;

//printf ("About to mark range 0x%08lx:%d...\n", allocbase, size);
		mr->mr_Offset	= allocbase;
		mr->mr_Size	= size;
		mr->mr_Owner	= 0;
		
		imr.imr_PoolID		= pi->pi_PoolID;
		imr.imr_MemRange	= mr;
		imr.imr_UserData	= NULL;
		imr.imr_UserDataSize	= 0;
		if ((retval = ioctl (gPoolfd,
				     B_IOCTL_MARKRANGE,
				     &imr,
				     sizeof (imr))) < 0)
			retval = errno;
	} else
		retval = B_NO_MEMORY;

	ioctl (gPoolfd, B_IOCTL_UNLOCKPOOL, pi->pi_PoolID, 0);

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
#if defined(__INTEL__)
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




#endif





status_t
initmem (uint32 size)
{
	register int	i;
	status_t	retval;
	void		*poolmem;

	if ((retval = open ("/dev/misc/genpool", O_RDWR)) < 0)
		return (retval);
	gPoolfd = retval;

	size += B_PAGE_SIZE - 1;
	size &= ~(B_PAGE_SIZE - 1);
printf ("Creating pool area...\n");
	if ((retval = create_area ("test pool area",
				   &poolmem,
				   B_ANY_ADDRESS,
				   size,
				   B_NO_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err0;	/*  Look down  */
	pool_aid = retval;

	poolinfo.pi_Pool_AID	= pool_aid;
	poolinfo.pi_Pool	= poolmem;
	poolinfo.pi_Size	= size;
	strcpy (poolinfo.pi_Name, "test pool");
printf ("Creating genpool for area 0x%08lx...\n", pool_aid);
	if (ioctl (gPoolfd,
		   B_IOCTL_CREATEPOOL,
		   &poolinfo,
		   sizeof (poolinfo)) < 0)
	{
		retval = errno;	/*  POSIX blows diseased goats  */
		goto err1;	/*  Look down  */
	}

printf ("Cloning rangelist area...\n");
	if ((retval = clone_area ("test rangelists clone",
				  &poolinfo.pi_RangeLists,
				  B_ANY_ADDRESS,
				  B_READ_AREA,	/*  NO WRITE!  */
				  poolinfo.pi_RangeLists_AID)) < 0)
	{
		ioctl (gPoolfd, B_IOCTL_DELETEPOOL, poolinfo.pi_PoolID, 0);
err1:		delete_area (pool_aid);
err0:		close (gPoolfd);
	} else {
		rangelists_aid = retval;
		retval = B_OK;

		printf ("Genpool %d successfully created:\n",
			poolinfo.pi_PoolID);
		printf ("        pi_Pool: %p (area 0x%08lx)\n",
			poolinfo.pi_Pool, poolinfo.pi_Pool_AID);
		printf ("  pi_RangeLists: %p (area 0x%08lx)\n",
			poolinfo.pi_RangeLists, poolinfo.pi_RangeLists_AID);
		printf ("  pi_RangeLocks: %p (area 0x%08lx)\n",
			poolinfo.pi_RangeLocks, poolinfo.pi_RangeLocks_AID);
		printf ("    pi_PoolLock: 0x%08lx\n", poolinfo.pi_PoolLock);
		printf ("        pi_Size: %d (%dM)\n",
			poolinfo.pi_Size, poolinfo.pi_Size >> 20);
		printf ("       pi_Flags: 0x%08lx\n", poolinfo.pi_Flags);
		printf ("        pi_Name: %s\n", poolinfo.pi_Name);
	}

	return (retval);
}

void
shutdownmem (void)
{
	delete_area (rangelists_aid);
	ioctl (gPoolfd, B_IOCTL_DELETEPOOL, poolinfo.pi_PoolID, 0);
	delete_area (pool_aid);
	close (gPoolfd);
}
