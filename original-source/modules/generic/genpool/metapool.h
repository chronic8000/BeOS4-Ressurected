/* :ts=8 bk=0
 *
 * metapool.h:	Definitions for internal memory management.
 *
 * Leo L. Schwab					1999.01.04
 */
#ifndef _METAPOOL_H
#define	_METAPOOL_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Each region of free space is represented by a FreeNode.
 */
typedef struct FreeNode {
	struct FreeNode	*fn_Next;
	uint32		fn_Size;
} FreeNode;


typedef struct mempool {
	BPoolInfo	mp_pi;
	BRangeList	*mp_freelist;
	BRangeList	*mp_alloclist;
	BRangeNode	*mp_ripcord;		/*  if allocation fails */
	FreeNode	*mp_firstfreenode;	/*  ...in pi_RangeLists	*/
	uint32		mp_rangelistsize;	/*  Area size in bytes	*/
	uint32		mp_ripcordpulled;	/*  ripcord in use	*/
	uint32		mp_firstfreelock;	/*  ...in pi_RangeLocks	*/
	uint32		mp_lastusedlock;	/*  ...in pi_RangeLocks	*/
	uint32		mp_nrangelocks;
	uint32		mp_opencount;
} mempool;


extern struct BRangeNode *allocrangenode (struct mempool *mp, uint32 usersize);
extern void freerangenode (struct mempool *mp, struct BRangeNode *rn);
extern void initrangenodepool (struct mempool *mp,
			       uint32 size,
			       uint32 initialreserved);


#ifdef __cplusplus
}
#endif

#endif	/*  _METAPOOL_H  */
