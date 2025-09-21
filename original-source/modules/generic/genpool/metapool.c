/* :ts=8 bk=0
 *
 * metapool.c:	Routines to manage the free space in the RangeLists
 *		themselves.
 *
 * Leo L. Schwab					1999.08.10
 */
#include <surface/genpool.h>
#include <stdio.h>

#include "metapool.h"


/*
 * Since only we have write privileges to the area containing the RangeLists,
 * we can pull this old, old trick: The freelists are maintained in the free
 * memory areas themselves.  This means that the nodes themselves move, which
 * makes the game slightly more interesting.
 */
struct BRangeNode *
allocrangenode (struct mempool *mp, uint32 usersize)
{
	FreeNode	*prevnode, *thisnode;
	int		retrying = false;

	if (mp->mp_ripcordpulled)
		return (NULL);

	/*  Add size of RangeNode, and round up to nearest multiple of 8.  */
	usersize = (usersize + sizeof (BRangeNode) + 7) & ~7;

	/*
	 * Find a free area of sufficient size.
	 */
retry:	for (prevnode = NULL, thisnode = mp->mp_firstfreenode;
	     thisnode;
	     prevnode = thisnode, thisnode = thisnode->fn_Next)
	{
		if (thisnode->fn_Size >= usersize)
			break;
	}

	if (thisnode) {
		if (thisnode->fn_Size == usersize) {
			/*  Exactly consumed block, bye-bye  */
			if (prevnode)
				prevnode->fn_Next = thisnode->fn_Next;
			else
				mp->mp_firstfreenode = thisnode->fn_Next;
		} else {
			/*  Relocate node, shrinking by size of request  */
			FreeNode	*movednode;

			movednode = (FreeNode *) ((uint32) thisnode +
						  usersize);
			movednode->fn_Next = thisnode->fn_Next;
			movednode->fn_Size = thisnode->fn_Size - usersize;
			if (movednode->fn_Size < sizeof (FreeNode)) {
				kprintf (">>> Deep bandini in node allocator; \
fn_Size = %d\n", movednode->fn_Size);
				return (NULL);
			}
			if (prevnode)
				prevnode->fn_Next = movednode;
			else
				mp->mp_firstfreenode = movednode;
		}
		return ((BRangeNode *) thisnode);
	} else {
		status_t	retval;
		uint32		newsize;

		/*  Resize this area to make room and try again.  */
//printf ("Sizing RangeLists up.\n");
		if (retrying)
			/*  Oops, we already tried that...  */
			return (NULL);
		
		newsize = mp->mp_rangelistsize + B_PAGE_SIZE;
//printf ("...from %d to %d...\n", mp->mp_rangelistsize, newsize);
		if ((retval = resize_area (mp->mp_pi.pi_RangeLists_AID, newsize)) < 0)
		{
//printf ("resize_area() failed; retval = %d (0x%08lx)\n", retval, retval);
			return (NULL);
		}

		if (prevnode)
			prevnode->fn_Size += B_PAGE_SIZE;
		else {
			/*  Woah, we're full to the rafters!  */
			mp->mp_firstfreenode =
			 (FreeNode *) ((uint32) mp->mp_pi.pi_RangeLists +
				       mp->mp_rangelistsize);
			mp->mp_firstfreenode->fn_Next = NULL;
			mp->mp_firstfreenode->fn_Size = B_PAGE_SIZE;
		}
		mp->mp_rangelistsize = newsize;
		retrying = true;
//printf ("Success; retrying allocrangenode()\n");
		goto retry;	/*  Look up  */
	}
}

void
freerangenode (struct mempool *mp, struct BRangeNode *rn)
{
	FreeNode	*prevnode, *thisnode, *newnode;
	uint32		size;

	if (!rn)
		return;
	size = (rn->rn_UserSize + sizeof (BRangeNode) + 7) & ~7;
	if (mp->mp_ripcordpulled) {
		/*  Recover the ripcord.  */
		uint32	diff;

//printf ("|!| Recovering ripcord.\n");
		/*
		 * Copy ripcord's data over into this newly available node.
		 * (If I wanted to be pedantic, I could skip copying the
		 * leading Node_OP structure...)
		 */
		memcpy (rn, mp->mp_ripcord, sizeof (BRangeNode));
		/*  Position copy to replace ripcord.  */
		InsertNodeBefore_OP (mp->mp_pi.pi_RangeLists,
				     (Node_OP *) rn,
				     (Node_OP *) mp->mp_ripcord);
		/*  Remove ripcord from list.  */
		RemNode_OP (mp->mp_pi.pi_RangeLists,
			    (Node_OP *) mp->mp_ripcord);
		/*  Ripcord available again.  */
		mp->mp_ripcordpulled = false;

		/*  Fudge size and address of thing we're freeing.  */
		diff = (sizeof (BRangeNode) + 7) & ~7;
		if (!(size -= diff))
			/*  Exact exchange; we're outa here.  */
			return;
		rn = (BRangeNode *) ((uint32) rn + diff);
//printf ("|!| Proceeding with freerangenode()...\n");
	}

	/*  Find out between which FreeNodes this new area fits  */
	for (prevnode = NULL, thisnode = mp->mp_firstfreenode;
	     thisnode;
	     prevnode = thisnode, thisnode = thisnode->fn_Next)
	{
		if ((uint32) rn < (uint32) thisnode)
			break;
	}

	/*
	 * I walked through all six cases by hand.  This should work...
	 */
	newnode = (FreeNode *) rn;
	newnode->fn_Next = thisnode;
	newnode->fn_Size = size;

	if (prevnode) {
		if ((uint32) prevnode + prevnode->fn_Size == (uint32) newnode)
		{
			prevnode->fn_Size += newnode->fn_Size;
			newnode = prevnode;
		} else
			prevnode->fn_Next = newnode;
	} else
		mp->mp_firstfreenode = newnode;

	if (thisnode) {
		if ((uint32) newnode + newnode->fn_Size == (uint32) thisnode)
		{
			newnode->fn_Next = thisnode->fn_Next;
			newnode->fn_Size += thisnode->fn_Size;
		}
	}

#if 0
/*
 * Well, I did this totally wrong (last entry in list is not necessarily
 * at the highest address).  I'll worry about this later...
 */
	/*  See if it's worth trying to size the area down.  */
	if (prevnode  &&  !thisnode) {
		size_t	thresh = 4 * B_PAGE_SIZE;

		if (prevnode->fn_Size > thresh) {
			thresh += mp->mp_rangelistsize - prevnode->fn_Size - 1;
			thresh &= ~(B_PAGE_SIZE - 1);
printf ("Sizing RangeLists down from %d to %d...", mp->mp_rangelistsize, thresh);
			
			if (resize_area
			     (mp->mp_pi.pi_RangeLists_AID, thresh) == B_OK)
			{
printf ("Success!");
				prevnode->fn_Size -= mp->mp_rangelistsize -
						     thresh;
				mp->mp_rangelistsize = thresh;
			}
printf ("\n");
		}
	}
#endif
}


void
initrangenodepool (struct mempool *mp, uint32 size, uint32 initialreserved)
{
	FreeNode *basenode;

	initialreserved = (initialreserved + 7) & ~7;
	basenode = (FreeNode *)
		   ((uint32) mp->mp_pi.pi_RangeLists + initialreserved);

	basenode->fn_Next = NULL;
	basenode->fn_Size = size - initialreserved;
	mp->mp_firstfreenode = basenode;
	
	/*
	 * Create a "ripcord"; an emergency backup RangeNode for returnrange()
	 * in case it needs a free RangeNode and can't get one normally.
	 */
	mp->mp_ripcord = allocrangenode (mp, 0);
	mp->mp_ripcordpulled = false;
	memset (mp->mp_ripcord, 0, sizeof (BRangeNode));
}
