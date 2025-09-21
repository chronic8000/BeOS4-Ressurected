/*
 * This file contains the header definitions for using
 * dmalloc and dfree.
 *
 *   Dominic Giampaolo
 *   (nick@cs.wpi.edu)
 */

#ifndef _DMALLOC_H
#define _DMALLOC_H

void *dmalloc(unsigned int size);
void *dcalloc(unsigned int nelem, unsigned int size);
void *drealloc(void *ptr, unsigned int size);
void  dfree(void *mem);
void  dcfree(void *mem);
char *dstrdup(char *str);
void  check_mem(void);

/*
 * This next part insures that the dmalloc routines only take place
 * when needed.  If you don't want them in your code, you would just
 * #define NO_DMALLOC before #including this file and you'd be all set.
 *
 */
#if  !defined(_DMALLOC_C) && !defined(NO_DMALLOC)

/*
 * NOTE: We define these simple translations instead of defining
 *       function macros so that we'll pick up all references to
 *       malloc() and free(), even when they are being passed as
 *       function pointers.
 */

#define calloc   dcalloc
#define realloc  drealloc
#define malloc   dmalloc
#define cfree    dcfree
#define free     dfree
#define strdup   dstrdup

#endif  /* !defined(_DMALLOC_C) || !defined(NO_DMALLOC) */


#endif /* _DMALLOC_H */
