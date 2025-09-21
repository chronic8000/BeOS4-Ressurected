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
bt_zap(BT_INDEX *b)
{
	struct	bt_cache *op;

	/* toast superblock */
	b->sblk.levs = 1;
	b->sblk.root = bt_pagesiz(b);
	b->sblk.freep = BT_NULL;
	b->sblk.high = 2 * bt_pagesiz(b);
	b->dirt++;
	if(bt_wsuper(b) == BT_ERR)
		return(BT_ERR);

	/* set up first leaf */
	if((op = bt_rpage(b,BT_NULL)) == NULL)
		return(BT_ERR);

	/* note - set cky and cpag case called by bt_load() */
	op->num = b->sblk.root; 
	KEYCNT(op->p) = 0;
	KEYLEN(op->p) = 0;
	LSIB(op->p) = RSIB(op->p) = BT_NULL;
	HIPT(op->p) = BT_NULL;

	op->flags = BT_CHE_DIRTY;

#ifdef	USE_FTRUNCATE
	if(ftruncate(bt_fileno(b),b->sblk.high) == -1)
		return(BT_ERR);
#endif

	return(bt_wpage(b,op));
}
