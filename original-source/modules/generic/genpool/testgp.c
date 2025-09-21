/* :ts=8 bk=0
 *
 * testgp.c:	Test to make sure my memory allocator is sane.
 *
 * gcc -Iinclude -I$BUILDHOME/src/kit/dinky/include -I$BUILDHOME/src/kit/surface/include testgp.c genpool.c ranges.c metapool.c defalloc.c -ldinky
 *
 * Leo L. Schwab					1999.08.12
 */
#include <kernel/OS.h>
#include <drivers/genpool_module.h>
#include <stdio.h>
#include <stdlib.h>

#include "metapool.h"


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



extern genpool_module	*modules[];

static genpool_module	*gpm;
static BPoolInfo	*gpoolptr;
static area_id		pool_aid = -1;
static void		*gptrs[NTESTS];
static int32		gsizes[NTESTS];

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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
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
		checkfreelist (gpoolptr, &n));
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
		checkfreelist (gpoolptr, &nentries));
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
	ms.ms_PoolID		= gpoolptr->pi_PoolID;
	ms.ms_AddrCareBits	= carebits;
	ms.ms_AddrStateBits	= statebits;
	ms.ms_AllocFlags	= 0;
	if ((retval = (gpm->gpm_AllocByMemSpec) (&ms)) < 0) {
//printf ("AllocByMemSpec() failed with %d (0x%08lx)\n",
// retval, retval);
		return (NULL);
	} else
		return ((void *) ((uint32) gpoolptr->pi_Pool +
				  ms.ms_MR.mr_Offset));
}

void
testfree (void *ptr, int32 size)
{
	BMemSpec	ms;
	status_t	retval;

	memset (&ms, 0, sizeof (ms));
	ms.ms_MR.mr_Offset	= (uint32) ptr - (uint32) gpoolptr->pi_Pool;
	ms.ms_PoolID		= gpoolptr->pi_PoolID;
	if ((retval = (gpm->gpm_FreeByMemSpec) (&ms)) < 0)
		printf ("FreeByMemSpec() failed: %d (0x%08lx)\n",
			retval, retval);
}




status_t
initmem (uint32 size)
{
	register int	i;
	status_t	retval;
	void		*poolmem;

	gpm = modules[0];
printf ("Initializing genpool module...\n");
	if ((retval = (gpm->gpm_ModuleInfo.std_ops) (B_MODULE_INIT)) < 0)
		return (retval);

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
	
	if (!(gpoolptr = (gpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err1;	/*  Look down  */
	}

	gpoolptr->pi_Pool_AID	= pool_aid;
	gpoolptr->pi_Pool	= poolmem;
	gpoolptr->pi_Size	= size;
	strcpy (gpoolptr->pi_Name, "test pool");
printf ("Creating genpool for area 0x%08lx...\n", pool_aid);
	if ((retval = (gpm->gpm_CreatePool) (gpoolptr, 2048, 0)) < 0) {
err2:		(gpm->gpm_FreePoolInfo) (gpoolptr);
err1:		delete_area (pool_aid);
err0:		(gpm->gpm_ModuleInfo.std_ops) (B_MODULE_UNINIT);
	} else {
		retval = B_OK;

		printf ("Genpool %d successfully created:\n",
			gpoolptr->pi_PoolID);
		printf ("        pi_Pool: %p (area 0x%08lx)\n",
			gpoolptr->pi_Pool, gpoolptr->pi_Pool_AID);
		printf ("  pi_RangeLists: %p (area 0x%08lx)\n",
			gpoolptr->pi_RangeLists, gpoolptr->pi_RangeLists_AID);
		printf ("  pi_RangeLocks: %p (area 0x%08lx)\n",
			gpoolptr->pi_RangeLocks, gpoolptr->pi_RangeLocks_AID);
		printf ("    pi_PoolLock: 0x%08lx\n", gpoolptr->pi_PoolLock);
		printf ("        pi_Size: %d (%dM)\n",
			gpoolptr->pi_Size, gpoolptr->pi_Size >> 20);
		printf ("       pi_Flags: 0x%08lx\n", gpoolptr->pi_Flags);
		printf ("        pi_Name: %s\n", gpoolptr->pi_Name);
	}

	return (retval);
}

void
shutdownmem (void)
{
	(gpm->gpm_DeletePool) (gpoolptr->pi_PoolID);
	(gpm->gpm_FreePoolInfo) (gpoolptr);  gpoolptr = NULL;
	delete_area (pool_aid);
	(gpm->gpm_ModuleInfo.std_ops) (B_MODULE_UNINIT);
}
