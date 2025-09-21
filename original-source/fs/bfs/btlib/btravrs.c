#include    <limits.h>
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
bt_traverse(BT_INDEX *b, int d, bt_chrp kbuf, int maxlen,
			int *retlen, bt_off_t *retrrv)
{
	struct	bt_cache *p;
	bt_off_t	savp, rrn;

	/* if the current page is not set, just zing up to the head */
	/* or the tail - the opposite side of the tree, whatever it is */
	if(b->cpag == BT_NULL)
		if(bt_goto(b,d == BT_EOF ? BT_BOF:BT_EOF) == BT_ERR) 
			return(BT_ERR);

top:
	
	/* save */
	savp = b->cpag;

	if((p = bt_rpage(b,b->cpag)) == NULL)
		return(BT_ERR);

	/* adjust current key # */
	if (b->dup_page == 0) {
		if(d == BT_EOF)
			b->cky++;
		else
			b->cky--;
	}

	/* have we run out of keys in the page ? */
	while((d == BT_EOF && b->cky >=  KEYCNT(p->p)) || 
		(d == BT_BOF && b->cky < 0)) {

		if(d == BT_EOF)
			b->cpag = RSIB(p->p);
		else
			b->cpag = LSIB(p->p);

		/* we are there - wherever there is */
		if(b->cpag != BT_NULL) {
			if((p = bt_rpage(b,b->cpag)) == NULL)
				return(BT_ERR);

			/* reset current key */
			if(d == BT_EOF)
				b->cky = 0;
			else
				b->cky = KEYCNT(p->p) - 1;
		} else {
			/* we have actually HIT the end */
			/* restore current page */
			b->cpag = savp;
			/* and point the key just past the end of the page */
			if(d == BT_EOF)
				b->cky = KEYCNT(p->p);
			else
				b->cky = -1;
			*retlen = 0;
			*retrrv = 0;
			return(d);
		}
	}

	if(bt_keyof(b->cky,p->p,kbuf,retlen,&rrn,maxlen) == BT_ERR)
		return(BT_ERR);

	if (IS_DUP_CHAIN(rrn)) {  /* then we have to go get the real info */
		int ret;

		if (d == BT_EOF)
			ret = bt_getnextdup(b, rrn, retrrv);
		else
			ret = bt_getprevdup(b, rrn, retrrv);
		
		if (ret == 0) {   /* then there's no more duplicates for this guy */
			goto top;
		} else if (ret == BT_ERR) {
			return BT_ERR;
		}
	} else {
		*retrrv = rrn;
	}

	/* mark all as well with tree */
	bt_clearerr(b);
	return(BT_OK);
}
