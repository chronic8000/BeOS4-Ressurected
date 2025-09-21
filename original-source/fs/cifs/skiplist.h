#ifndef SKIPLIST_H
#define SKIPLIST_H

#include "sl_internal.h"

extern SkipList   NewSL(int (*compare)(), void (*freeitem)(), int flags);
extern void	   FreeSL(SkipList l);
extern int	   InsertSL(SkipList l, void *key);
extern int	   DeleteSL(SkipList l, void *key);
extern void	  *SearchSL(SkipList l, void *key);
extern void	  *NextSL(SkipList l, void *key);
extern void	   DoForSL(SkipList  l, int (*function)(), void *arg);
extern void		DoForRangeSL(SkipList l, void *key, int (*compare)(), int (*func)(), void *arg);
extern int		NumInSL(SkipList l);


#endif // SKIPLIST_H
