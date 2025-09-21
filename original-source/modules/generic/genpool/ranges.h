/* :ts=8 bk=0
 *
 * ranges.h:	MemRange prototypes.
 *
 * Leo L. Schwab					1999.08.10
 */
#ifndef	_RANGES_H
#define	_RANGES_H

#ifndef	_METAPOOL_H
#include "metapool.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


extern status_t procurerange (struct BMemRange *mr, struct mempool *mp);
extern status_t returnrange (struct BMemRange *mr, struct mempool *mp);
extern status_t logrange (struct BRangeNode *rn, struct mempool *mp);
extern struct BRangeNode *unlogrange (uint32 offset, struct mempool *mp);


#ifdef __cplusplus
}
#endif

#endif	/*  _RANGES_H  */
