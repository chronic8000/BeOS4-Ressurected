/* :ts=8 bk=0
 *
 * ranges.c:	Routines to allocate and free MemRanges.
 *
 * Leo L. Schwab					1999.08.10
 */
#include <surface/genpool.h>
#include <stdio.h>

#include "ranges.h"


/*****************************************************************************
 * These routines manipulate the freelist, the list that enumerates all
 * unallocated ranges.
 */
/*
 * Procures a range of bytes from the freelist.
 */
status_t
procurerange (struct BMemRange *mr, struct mempool *mp)
{
	BRangeNode	*freenode, *newnode;
	status_t	retval;
	uint32		freebase, allocbase, allocend;
	uint32		size;
	void 		*base;

	if (!(size = mr->mr_Size))
		return (B_BAD_VALUE);

	base = mp->mp_pi.pi_RangeLists;
	freenode = NULL;
	allocend = (allocbase = mr->mr_Offset) + mr->mr_Size;

	/*
	 * Search for a free range surrounding the client's request.
	 */
	for (newnode = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_freelist->rl_List);
	     NEXTNODE_OP (base, newnode);
	     newnode = (BRangeNode *) NEXTNODE_OP (base, newnode))
	{
		if (newnode->rn_MR.mr_Offset >= allocend  ||
		    newnode->rn_MR.mr_Offset + newnode->rn_MR.mr_Size <=
		     allocbase)
			/*  Definitely not intersecting the client's range  */
			continue;

		if (newnode->rn_MR.mr_Offset > allocbase  ||
		    newnode->rn_MR.mr_Offset + newnode->rn_MR.mr_Size <
		     allocend)
			/*  Client range overlaps already-reserved range  */
			return (B_POOLERR_MEMRANGE_UNAVAILABLE);

		/*  Found an encompassing range.  */
		freenode = newnode;
		freebase = freenode->rn_MR.mr_Offset;
		break;
	}

	if (freenode) {
		if (allocbase == freebase) {
			if (size == freenode->rn_MR.mr_Size) {
				/*  Exactly consumed block, bye-bye  */
				RemNode_OP (base, (Node_OP *) freenode);
				freerangenode (mp, freenode);
			} else {
				/*  Shrink free block by allocation  */
				freenode->rn_MR.mr_Offset += size;
				freenode->rn_MR.mr_Size -= size;
			}
		} else {
			/*
			 * allocbase is greater than freebase; we may
			 * need to create a new RangeNode.
			 */
			if (allocbase + size ==
			    freebase + freenode->rn_MR.mr_Size)
			{
				/* Exact fit into end; shorten block */
				freenode->rn_MR.mr_Size -= size;
			} else {
				/*
				 * The icky case.  Create a new
				 * free block; describe free areas
				 * before and after allocated region.
				 */
				/* Create new block after allocation */
				if (!(newnode = allocrangenode (mp, 0)))
					goto nomem;	/*  Look down  */
				newnode->rn_MR.mr_Offset =
				 allocbase + size;
				newnode->rn_MR.mr_Size =
				 freebase + freenode->rn_MR.mr_Size -
				 allocbase - size;
				InsertNodeAfter_OP (base,
						    (Node_OP *) newnode,
						    (Node_OP *) freenode);

				/*  Shorten existing block  */
				freenode->rn_MR.mr_Size = allocbase - freebase;
			}
		}
		mp->mp_freelist->rl_Total -= size;
		retval = B_OK;
	} else
nomem:		retval = B_NO_MEMORY;

	return (retval);
}

/*
 * Return a range to the free list.
 */
status_t
returnrange (struct BMemRange *mr, struct mempool *mp)
{
	register BRangeNode	*curr, *prev;
	uint32			size, offset;
	void			*base;

	/*
	 * Search for the correct nodes in which to place the newly freed
	 * region.
	 ****
	 * FIXME: There should be a debugging version that checks if the area
	 * to be freed overlaps existing free regions, indicating client
	 * screwup.
	 */
	offset = mr->mr_Offset;		/*  zero is a valid offset  */
	if (!(size = mr->mr_Size))
		return (B_BAD_VALUE);

//printf ("--- Freeing %d @ 0x%08lx\n", size, offset);
	base = mp->mp_pi.pi_RangeLists;
	for (prev = NULL, curr = (BRangeNode *)
			   FIRSTNODE_OP (base, &mp->mp_freelist->rl_List);
	     NEXTNODE_OP (base, curr);
	     prev = curr, curr = (BRangeNode *) NEXTNODE_OP (base, curr))
	{
		if (curr->rn_MR.mr_Offset > offset)
			break;
	}
	if (NEXTNODE_OP (base, curr)  &&
	    offset + size == curr->rn_MR.mr_Offset)
	{
//printf ("--- Prepend\n");
		/*  Exactly prepends to this block.  */
		curr->rn_MR.mr_Offset = offset;
		curr->rn_MR.mr_Size += size;
		if (prev  &&
		    prev->rn_MR.mr_Offset + prev->rn_MR.mr_Size == offset)
		{
			/*  Coalesce current and previous blocks.  */
//printf ("---- Coalesce\n");
			prev->rn_MR.mr_Size += curr->rn_MR.mr_Size;
			RemNode_OP (base, (Node_OP *) curr);
			freerangenode (mp, curr);
		}
	} else if (prev  &&
		   prev->rn_MR.mr_Offset + prev->rn_MR.mr_Size == offset)
	{
//printf ("--- Append\n");
		/*  Exactly appends to previous block.  */
		prev->rn_MR.mr_Size += size;
	} else {
		/*  Doesn't mate with anything.  Create new node.  */
		BRangeNode	*newnode;
//printf ("--- Orphan\n");

		if (!(newnode = allocrangenode (mp, 0))) {
			/*  $#|+!!  Pull the ripcord.  */
//printf ("!|! Pulling ripcord.\n");
			newnode = mp->mp_ripcord;
			mp->mp_ripcordpulled = true;
		}
		newnode->rn_MR.mr_Offset = offset;
		newnode->rn_MR.mr_Size = size;
		if (prev)
			InsertNodeAfter_OP (base,
					    (Node_OP *) newnode,
					    (Node_OP *) prev);
		else
			InsertNodeBefore_OP (base,
					     (Node_OP *) newnode,
					     (Node_OP *) curr);
	}
	mp->mp_freelist->rl_Total += size;
	return (B_OK);
}


/*****************************************************************************
 * These routines manipulate the alloclist, the list that records allocated
 * ranges.
 */
/*
 * Record an obtained range in the alloclist.
 */
status_t
logrange (struct BRangeNode *rn, struct mempool *mp)
{
	BRangeNode	*where;
	void		*base;

	/*
	 * Insert in order of mr_Offset.
	 */
	base = mp->mp_pi.pi_RangeLists;
	for (where = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_alloclist->rl_List);
	     NEXTNODE_OP (base, where);
	     where = (BRangeNode *) NEXTNODE_OP (base, where))
	{
		if (where->rn_MR.mr_Offset > rn->rn_MR.mr_Offset)
			break;
	}
	InsertNodeBefore_OP (base, (Node_OP *) rn, (Node_OP *) where);
	mp->mp_alloclist->rl_Total += rn->rn_MR.mr_Size;
	return (B_OK);
}

/*
 * Delist an obtained range from the alloclist.
 */
struct BRangeNode *
unlogrange (uint32 offset, struct mempool *mp)
{
	BRangeNode	*rn;
	void		*base;

	base = mp->mp_pi.pi_RangeLists;
	for (rn = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_alloclist->rl_List);
	     NEXTNODE_OP (base, rn);
	     rn = (BRangeNode *) NEXTNODE_OP (base, rn))
	{
		if (rn->rn_MR.mr_Offset == offset) {
			RemNode_OP (base, (Node_OP *) rn);
			mp->mp_alloclist->rl_Total -= rn->rn_MR.mr_Size;
			return (rn);
		}
	}
	return (NULL);
}
