#include	<sys/types.h>
#include	<stdio.h>
#include	"btconf.h"
#include	"btree.h"
#include	"btintern.h"

/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.

*/


int
bt_close(BT_INDEX *b)
{
	struct	bt_cache *p1;
	struct	bt_cache *p2;
	int	rv;

	rv = bt_sync(b);

	/* close fd */
#if 0
	if(bt_fileno(b) != -1)
		if(close(bt_fileno(b)) < 0)
			rv = BT_ERR;
#endif

	/* free stack */
	if(b->stack != NULL)
		(void)free((char *)b->stack);

	/* free cache */
	for(p1 = b->lru; p1 != NULL;) {

		/* K&R say not to use free()d stuff */
		p2 = p1->next;

		/* free it */
		(void)free((char *)p1->p);
		(void)free((char *)p1);
		p1 = p2;
	}

	/* free handle itself */
	(void)free((char *)b);
	return(rv);
}
