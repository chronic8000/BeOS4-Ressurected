/* :ts=8 bk=0
 *
 * genpool.c:	"Generic" pool manager.
 *
 * Leo L. Schwab					1999.08.12
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/module.h>
#include <dinky/bena4.h>
#include <dinky/listnode_op.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <surface/genpool.h>
#include <drivers/genpool_module.h>

#include "metapool.h"
#include "ranges.h"


/****************************************************************************
 * #defines
 */
#define	MAXPOOLS		128	/*  Must be power of 2  */


/*****************************************************************************
 * Bookkeeppiinngg structures.
 */


/****************************************************************************
 * Prototypes.
 */
extern status_t allocbymemspec (struct BMemSpec *ms);
extern status_t freebymemspec (struct BMemSpec *ms);


/****************************************************************************
 * Globals.
 */
static Bena4		gLock;
static mempool		*gPools[MAXPOOLS];
static int32		gMaxPoolID = -1;
static uint32		gNPools;
static uint32		gOwnerID;	/*  Source of unique owner IDs  */


/****************************************************************************
 * PoolInfo management.
 */
static struct BPoolInfo *
allocpoolinfo (void)
{
	BPoolInfo	*pi;

	if (pi = malloc (sizeof (*pi)))
		memset (pi, 0, sizeof (*pi));
	return (pi);
}

static void
freepoolinfo (struct BPoolInfo *pi)
{
	free (pi);
}


/****************************************************************************
 * Memory pool management.
 */
struct mempool *
lookuppool (uint32 poolid)
{
	mempool	*mp;
	uint32	idx;
	
	idx = poolid & (MAXPOOLS - 1);
	if ((mp = gPools[idx])  &&  mp->mp_pi.pi_PoolID == poolid)
		return (mp);
	else
		return (NULL);
}


/*
 * Fields requring initialization by the client:
 *
 * pi_Pool_AID:	Area ID of the memory region being managed.
 * pi_Pool:	Systemwide kernel address of memory region (optional).
 * pi_Size:	Size of pool, in bytes.
 * pi_Flags:	More settings.
 * pi_Name:	Name of this pool.
 */
static status_t
createpool (struct BPoolInfo *pi, uint32 maxallocs, uint32 userdatasize)
{
	mempool		*mp;
	BRangeList	*rl;
	BRangeNode	*rn;
	area_id		lists_aid, locks_aid;
	status_t	retval;
	uint32		idx;
	uint32		locksiz;
	char		areaname[B_OS_NAME_LENGTH + 16], *nameend;

	if (gNPools >= MAXPOOLS  ||  !(mp = malloc (sizeof (mempool))))
		return (B_NO_MEMORY);
	
	memset (mp, 0, sizeof (*mp));
	lists_aid = locks_aid = -1;

	strncpy (areaname, pi->pi_Name, B_OS_NAME_LENGTH);
	areaname[B_OS_NAME_LENGTH - 1] = '\0';
	nameend = areaname + strlen (areaname);
	
	/*
	 * Create storage for RangeLists and Locks.
	 * Space for the RangeLists must include:
	 *	o 'maxallocs' BRangeNodes with userdata,
	 *	o At least one BRangeNode to hold the free list,
	 *	o Two BRangeList structures (one alloc, one free).
	 */
	mp->mp_rangelistsize =
	 (((maxallocs + 1) * ((userdatasize + sizeof (BRangeNode) + 7) & ~7)) +
	  ((sizeof (BRangeList) * 2 + 7) & ~7) +
	  B_PAGE_SIZE - 1) &
	 ~(B_PAGE_SIZE - 1);
	strcpy (nameend, " range lists");
	if ((retval = create_area (areaname,
				   (void **) &mp->mp_pi.pi_RangeLists,
				   B_ANY_KERNEL_ADDRESS,
				   mp->mp_rangelistsize,
				   B_FULL_LOCK,	/*  B_NO_LOCK?  */
				   B_READ_AREA)) < 0)
		goto err0;	/*  Look down  */
	mp->mp_pi.pi_RangeLists_AID = lists_aid = retval;

	locksiz = (maxallocs * sizeof (uint32) + B_PAGE_SIZE - 1) &
		  ~(B_PAGE_SIZE - 1);
	strcpy (nameend, " range locks");
	if ((retval = create_area (areaname,
				   (void **) &mp->mp_pi.pi_RangeLocks,
				   B_ANY_KERNEL_ADDRESS,
				   locksiz,
				   B_FULL_LOCK,	/*  B_NO_LOCK?  */
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err1;	/*  Look down  */
	mp->mp_pi.pi_RangeLocks_AID = locks_aid = retval;
	memset ((void *) mp->mp_pi.pi_RangeLocks, ~0, locksiz);
	
	strcpy (nameend, " pool");
	if ((retval = create_sem (1, areaname)) < 0)
		goto err2;	/*  Look down  */

	/*  Initialize mempool structure.  */
	mp->mp_pi.pi_PoolLock	= retval;
	mp->mp_pi.pi_Pool_AID	= pi->pi_Pool_AID;
	mp->mp_pi.pi_Pool	= pi->pi_Pool;		/*  May be NULL  */
	mp->mp_pi.pi_Size	= pi->pi_Size;
	mp->mp_pi.pi_Flags	= pi->pi_Flags;
	*nameend = '\0';
	strcpy (mp->mp_pi.pi_Name, areaname);

	mp->mp_firstfreelock	= 0;
	mp->mp_nrangelocks	= locksiz / sizeof (uint32);
	mp->mp_opencount	= 1;

	/*  Initialize metapool to manage the RangeList pool.  */
	initrangenodepool (mp, mp->mp_rangelistsize, sizeof (BRangeList) * 2);

	/*  Initialize RangeLists  */
	/*  First list is free list.  This is a bit involved.  */
	rl = mp->mp_pi.pi_RangeLists;
	InitList_OP (rl, &rl->rl_List);
	rl->rl_Total = mp->mp_pi.pi_Size;
	mp->mp_freelist = rl;

	rn = allocrangenode (mp, 0);
	memset (rn, 0, sizeof (*rn));
	rn->rn_MR.mr_Size = mp->mp_pi.pi_Size;
	AddHead_OP (rl, &rn->rn_Node, &rl->rl_List);

	/*  Next is the allocated list.  This is simpler.  */
	rl++;
	InitList_OP (mp->mp_pi.pi_RangeLists, &rl->rl_List);
	rl->rl_Total = 0;
	mp->mp_alloclist = rl;

	/*
	 * Add it to the global list of pools.
	 */
	BLockBena4 (&gLock);
	/*  Find a free slot  */
	while (gPools[(idx = ++gMaxPoolID) & (MAXPOOLS - 1)])
		;
	mp->mp_pi.pi_PoolID = idx;
	gPools[idx & (MAXPOOLS - 1)] = mp;
	gNPools++;
	BUnlockBena4 (&gLock);
	memcpy (pi, &mp->mp_pi, sizeof (*pi));
//	pi->pi_RangeLists = NULL;
	retval = B_OK;

	if (0) {
err2:		delete_area (locks_aid);
err1:		delete_area (lists_aid);
err0:		free (mp);
	}

	return (retval);
}

static status_t
clonepool (struct BPoolInfo *pi)
{
	mempool	*mp;
	
	if (mp = lookuppool (pi->pi_PoolID)) {
		memcpy (pi, &mp->mp_pi, sizeof (*pi));
		mp->mp_opencount++;
		return (B_OK);
	} else
		return (B_POOLERR_UNKNOWN_POOLID);
}


static status_t
deletepool (struct mempool *mp)
{
	acquire_sem (mp->mp_pi.pi_PoolLock);

	gPools[mp->mp_pi.pi_PoolID & (MAXPOOLS - 1)] = NULL;
	gNPools--;
	
	/*
	 * FIXME: Sweep through RangeLocks and see if any
	 * locks are held, indicating that the hardware is
	 * still dancing on the resource.
	 */
	delete_sem (mp->mp_pi.pi_PoolLock);
	delete_area (mp->mp_pi.pi_RangeLocks_AID);
	delete_area (mp->mp_pi.pi_RangeLists_AID);
	free (mp);
	return (B_OK);
}

static status_t
deletepool_id (uint32 poolid)
{
	mempool		*mp;
	status_t	retval = B_POOLERR_UNKNOWN_POOLID;

	BLockBena4 (&gLock);
	
	if (mp = lookuppool (poolid)) {
		/*  Found the pool s/he wants to get rid of.  */
		if (!--mp->mp_opencount)
			retval = deletepool (mp);
		else
			retval = B_OK;
	}

	BUnlockBena4 (&gLock);
	return (retval);
}


static uint32
findpool (const char *name)
{
	mempool	*mp;
	uint32	retval = 0;
	int	idx;

	BLockBena4 (&gLock);

	for (idx = 0;  idx < MAXPOOLS;  idx++) {
		if ((mp = gPools[idx])  &&
		     !strcasecmp (mp->mp_pi.pi_Name, name))
		{
			retval = mp->mp_pi.pi_PoolID;
			break;
		}
	}

	BUnlockBena4 (&gLock);
	return (retval);
}

static uint32
getuniqueownerid (void)
{
	return (++gOwnerID);
}


/****************************************************************************
 * Pool lock management.
 */
static status_t
lockpool (uint32 poolid)
{
	mempool		*mp;
	status_t	retval = B_POOLERR_UNKNOWN_POOLID;

	BLockBena4 (&gLock);

	if (mp = lookuppool (poolid)) {
		/*  FIXME: Timeout parameter?  */
		acquire_sem (mp->mp_pi.pi_PoolLock);
		retval = B_OK;
	}

	BUnlockBena4 (&gLock);
	return (retval);
}

static void
unlockpool (uint32 poolid)
{
	mempool		*mp;

	BLockBena4 (&gLock);

	if (mp = lookuppool (poolid))
		release_sem (mp->mp_pi.pi_PoolLock);

	BUnlockBena4 (&gLock);
}


/****************************************************************************
 * Range marker.
 * THE POOL MUST BE LOCKED BEFORE CALLING THESE ROUTINES!
 */
/*  Returns ~0 if it can't find anything.  */
static uint32
findfreerangelock (struct mempool *mp)
{
	register uint32	i, n;
	int		retrying = FALSE;

retry:
	for (i = mp->mp_firstfreelock;  i < mp->mp_nrangelocks;  i++) {
		if (mp->mp_pi.pi_RangeLocks[i] == ~0)
			return (i);
	}

	/*
	 * All locks in use.  Enlarge the lock area and try again.
	 */
	if (retrying)
		/*  Oh, wait, we already did that...  */
		return (~0);

	/*  Increase area by one page.  */
	n = mp->mp_nrangelocks * sizeof (uint32) + B_PAGE_SIZE;
//printf ("Sizing RangeLocks up from %d to %d...\n",
// mp->mp_nrangelocks * sizeof (uint32), n);
	if (resize_area (mp->mp_pi.pi_RangeLocks_AID, n) < 0)
		return (~0);

	/*  Initialize new locks to unused.  */
//printf ("...Success; retrying findfreerangelock().\n");
	n /= sizeof (uint32);
	for (i = mp->mp_nrangelocks;  i < n; )
		mp->mp_pi.pi_RangeLocks[i++] = ~0;
	mp->mp_nrangelocks = n;
	retrying = TRUE;
	goto retry;	/*  Look up  */
}

void
reserverangelock (struct mempool *mp, uint32 idx)
{
	/*  Mark as in use, unlocked.  */
	atomic_and (mp->mp_pi.pi_RangeLocks + idx, 0);

	if (idx > mp->mp_firstfreelock)
		mp->mp_firstfreelock = idx + 1;
	if (idx > mp->mp_lastusedlock)
		mp->mp_lastusedlock = idx;
}


void
disposerangelock (struct mempool *mp, uint32 idx)
{
	/*  Mark as free.  */
	atomic_or (mp->mp_pi.pi_RangeLocks + idx, ~0);

	if (idx < mp->mp_firstfreelock)
		mp->mp_firstfreelock = idx;

	if (idx == mp->mp_lastusedlock) {
#if 0
		/*  Nothing to do with Dennis Fong  */
		size_t	thresh = 2 * B_PAGE_SIZE / sizeof (uint32);
#endif

		while (idx > 0) {
			idx--;
			if (mp->mp_pi.pi_RangeLocks[idx] >= 0)
				break;
		}
		mp->mp_lastusedlock = idx;

#if 0
		if (mp->mp_nrangelocks - idx > thresh) {
			/*
			 * Shrink area, leaving at least one page for
			 * expansion.
			 */
			thresh += idx;
			thresh *= sizeof (uint32);
			thresh--;
			thresh &= ~(B_PAGE_SIZE - 1);
//printf ("Sizing RangeLocks down from %d to %d...\n",
// mp->mp_nrangelocks * sizeof (uint32), thresh);
			if (resize_area
			     (mp->mp_pi.pi_RangeLocks_AID, thresh) == B_OK)
			{
				mp->mp_nrangelocks = thresh / sizeof (uint32);
//printf ("...Success! mp_nrangelocks == %d\n", mp->mp_nrangelocks);
			}
		}
#endif
	}
}


status_t
markrange (
struct mempool		*mp,
struct BMemRange	*cl_mr,
const void		*userdata,
uint32			userdatasize
)
{
	BRangeNode	*rn;
	status_t	retval;
	uint32		i;

	if ((i = findfreerangelock (mp)) == ~0)
		return (B_NO_MEMORY);

	/*  Found a lock.  Create a RangeNode  */
	if (rn = allocrangenode (mp, userdatasize)) {
		rn->rn_UserSize		= userdatasize;
		rn->rn_MR.mr_Offset	= cl_mr->mr_Offset;
		rn->rn_MR.mr_Size	= cl_mr->mr_Size;
		rn->rn_MR.mr_Owner	= cl_mr->mr_Owner;
		rn->rn_MR.mr_LockIdx	= i;
	
		if ((retval = procurerange (&rn->rn_MR, mp)) == B_OK) {
			if ((retval = logrange (rn, mp)) == B_OK) {
				/*  Mark the lock as reserved  */
				reserverangelock (mp, i);
	
				/*  Copy userdata to RangeNode.  */
				if (userdata  &&  userdatasize)
					memcpy (rn->rn_MR.mr_UserData,
						userdata,
						userdatasize);
				/*  Update client's MemRange.  */
				cl_mr->mr_LockIdx = i;
	
				return (B_OK);
			}
			returnrange (&rn->rn_MR, mp);
		}
		freerangenode (mp, rn);
	} else
		retval = B_NO_MEMORY;
	return (retval);
}

static status_t
markrange_id (
uint32			poolid,
struct BMemRange	*cl_mr,
const void		*userdata,
uint32			userdatasize
)
{
	mempool	*mp;

	if (!(mp = lookuppool (poolid)))
		return (B_POOLERR_UNKNOWN_POOLID);

	return (markrange (mp, cl_mr, userdata, userdatasize));
}

status_t
unmarkrange (struct mempool *mp, uint32 offset)
{
	BRangeNode	*rn;
	status_t	retval;

	if (rn = unlogrange (offset, mp)) {
		uint32	idx;

		idx = rn->rn_MR.mr_LockIdx;
		if (atomic_add (mp->mp_pi.pi_RangeLocks + idx, 1) > 0) {
			/*  In use; put it back.  */
			/*  FIXME: This could be more efficient.  */
			atomic_add (mp->mp_pi.pi_RangeLocks + idx, -1);
			logrange (rn, mp);
			return (B_POOLERR_MEMRANGE_LOCKED);
		}

		/*
		 * returnrange() may cause mp_ripcord to get pulled, so
		 * be sure to freerangenode(rn) as soon as possible so it
		 * can get recovered.
		 */
		retval = returnrange (&rn->rn_MR, mp);
//if (retval < 0)
// printf ("=== returnrange() returned %d (0x%08lx)\n", retval, retval);
		/*  Free rangelock  */
		disposerangelock (mp, idx);

		/*  Delete RangeNode  */
		freerangenode (mp, rn);
		return (retval);
	} else {
//printf ("=== Bad Offset supplied to unlogrange(): 0x%08lx\n", offset);
		return (B_POOLERR_UNKNOWN_OFFSET);
	}
}


static status_t
unmarkrange_id (uint32 poolid, uint32 offset)
{
	mempool	*mp;

	if (!(mp = lookuppool (poolid)))
		return (B_POOLERR_UNKNOWN_POOLID);

	return (unmarkrange (mp, offset));
}

static status_t
unmarkrangesownedby (uint32 poolid, uint32 owner)
{
	BRangeNode	*rn, *next;
	mempool		*mp;
	status_t	retval = B_OK;
	void		*base;

	if (!(mp = lookuppool (poolid)))
		return (B_POOLERR_UNKNOWN_POOLID);

	base = mp->mp_pi.pi_RangeLists;
	for (rn = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_alloclist->rl_List);
	     next = (BRangeNode *) NEXTNODE_OP (base, rn);
	     rn = next)
	{
		if (rn->rn_MR.mr_Owner == owner) {
			uint32	idx;

			idx = rn->rn_MR.mr_LockIdx;
			if (atomic_add (mp->mp_pi.pi_RangeLocks + idx, 1) > 0)
			{
				/*  In use; put it back.  */
				atomic_add (mp->mp_pi.pi_RangeLocks + idx, -1);
				retval = B_POOLERR_MEMRANGE_LOCKED;
				continue;
			}
			RemNode_OP (base, (Node_OP *) rn);
			mp->mp_alloclist->rl_Total -= rn->rn_MR.mr_Size;

			returnrange (&rn->rn_MR, mp);

			/*  Free rangelock  */
			disposerangelock (mp, idx);

			/*  Delete RangeNode  */
			freerangenode (mp, rn);
		}
	}
	return (retval);
}



/*****************************************************************************
 * Module setup/teardown.
 */
static status_t
init (void)
{
	status_t retval;

	if ((retval = BInitOwnedBena4 (&gLock,
				       "GenPool module list lock",
				       B_SYSTEM_TEAM)) == B_OK)
	{
		gMaxPoolID = 0;		/*  0 == always invalid ID  */
	}

//kprintf ("||| init: returning %d\n", retval);
	return (retval);
}

static status_t
uninit (void)
{
	/*
	 * Gee, I hope no one's still using these Pools, since all record
	 * of them is about to go away.
	 */
	if (gMaxPoolID < 0)
		/*  Never initialized; we're outa there.  */
		return (B_OK);

	BLockBena4 (&gLock);


	/*  Tear down mempools.  */
      {	mempool		*mp;
	int		idx;

	for (idx = 0;  idx < MAXPOOLS;  idx++)
		if (mp = gPools[idx])
			deletepool (mp);
      }

	BDisposeBena4 (&gLock);
	gMaxPoolID = -1;
	
	return (B_OK);
}

static status_t
std_ops (int32 op, ...)
{
	switch (op) {
	case B_MODULE_INIT:
		return (init ());
	case B_MODULE_UNINIT:
		return (uninit ());
	default:
		return (B_ERROR);
	}
}


static genpool_module	gpmod = {
	{
		B_GENPOOL_MODULE_NAME,
		B_KEEP_LOADED,
		std_ops
	},
	allocpoolinfo,			/*  gpm_AllocPoolInfo		*/
	freepoolinfo,			/*  gpm_FreePoolInfo		*/
	createpool,			/*  gpm_CreatePool		*/
	clonepool,			/*  gpm_ClonePool		*/
	deletepool_id,			/*  gpm_DeletePool		*/
	findpool,			/*  gpm_FindPool		*/
	NULL,				/*  gpm_GetNextPoolID		*/
	getuniqueownerid,		/*  gpm_GetUniqueOwnerID	*/

	lockpool,			/*  gpm_LockPool		*/
	unlockpool,			/*  gpm_UnlockPool		*/

	markrange_id,			/*  gpm_MarkRange		*/
	unmarkrange_id,			/*  gpm_UnmarkRange		*/
	unmarkrangesownedby,		/*  gpm_UnmarkRangesOwnedBy	*/

	allocbymemspec,			/*  gpm_AllocByMemSpec		*/
	freebymemspec			/*  gpm_FreeByMemSpec		*/
};

/*
 * Old version 1 module structure, which doesn't have the gpm_GetUniqueOwnerID
 * vector.  We need this to remain binary-compatible.  This can be deleted
 * once we've shipped this new version.
 */
#ifdef V1_COMPAT

typedef struct genpool_module_v1 {
	module_info	gpm_ModuleInfo;
	struct BPoolInfo *
			(*gpm_AllocPoolInfo) (void);
	void		(*gpm_FreePoolInfo) (struct BPoolInfo *pi);
	status_t	(*gpm_CreatePool) (struct BPoolInfo *pi,
					   uint32 maxallocs,
					   uint32 userdatasize);
	status_t	(*gpm_ClonePool) (struct BPoolInfo *pi);
	status_t	(*gpm_DeletePool) (uint32 poolid);
	uint32		(*gpm_FindPool) (const char *name);
/**/	uint32		(*gpm_GetNextPoolID) (uint32 *cookie);

	status_t	(*gpm_LockPool) (uint32 poolid);
	void		(*gpm_UnlockPool) (uint32 poolid);

	status_t	(*gpm_MarkRange) (uint32 poolid,
					  struct BMemRange *cl_mr,
					  const void *userdata,
					  uint32 userdatasize);
	status_t	(*gpm_UnmarkRange) (uint32 poolid, uint32 offset);
	status_t	(*gpm_UnmarkRangesOwnedBy) (uint32 poolid,
						    uint32 owner);

	/*  Default allocation logic  */
	status_t	(*gpm_AllocByMemSpec) (struct BMemSpec *ms);
	status_t	(*gpm_FreeByMemSpec) (struct BMemSpec *ms);
} genpool_module_v1;


static genpool_module_v1	gpmod_v1 = {
	{
		"generic/genpool/v1",
		B_KEEP_LOADED,
		std_ops
	},
	allocpoolinfo,			/*  gpm_AllocPoolInfo		*/
	freepoolinfo,			/*  gpm_FreePoolInfo		*/
	createpool,			/*  gpm_CreatePool		*/
	clonepool,			/*  gpm_ClonePool		*/
	deletepool_id,			/*  gpm_DeletePool		*/
	findpool,			/*  gpm_FindPool		*/
	NULL,				/*  gpm_GetNextPoolID		*/

	lockpool,			/*  gpm_LockPool		*/
	unlockpool,			/*  gpm_UnlockPool		*/

	markrange_id,			/*  gpm_MarkRange		*/
	unmarkrange_id,			/*  gpm_UnmarkRange		*/
	unmarkrangesownedby,		/*  gpm_UnmarkRangesOwnedBy	*/

	allocbymemspec,			/*  gpm_AllocByMemSpec		*/
	freebymemspec			/*  gpm_FreeByMemSpec		*/
};

#endif	/*  V1_COMPAT  */


_EXPORT module_info	*modules[] = {
	(module_info *) &gpmod,
#ifdef V1_COMPAT
	(module_info *) &gpmod_v1,
#endif
	NULL
};
