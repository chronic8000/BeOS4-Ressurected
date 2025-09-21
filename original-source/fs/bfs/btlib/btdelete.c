#include        <limits.h>
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

	$Log: btdelete.c,v $
	Revision 1.2  1997/01/08 20:24:04  dbg
	some initial prep for the switch to long longs...
	(and some better error checking in the btree code)

	Revision 1.1  1996/11/05 00:53:13  cyril
	YAHOOOOO!!!!

 * Revision 1.2  90/05/03  10:38:50  mjr
 * *** empty log message ***
 * 
*/


int
bt_delete(BT_INDEX *b, bt_chrp key, int len, bt_off_t *rrn_to_del)
{
	struct	bt_cache	*op;
	int	staklev;
	bt_off_t	dpag;		/* page to delete from */
	int	keyat;		/* key # to delete */
	bt_off_t	key_rrn;	
	int	sr, ret;

	/* 90% of this code is straight from bt_insert(). the only */
	/* difference is that we need to ensure an exact match at */
	/* leaf level only, otherwise we just track our way up the */
	/* tree, deleting keys. page concatenation and redistribution */
	/* are not implemented, though if they are to be, they should */
	/* fit fairly easily in this framework. good luck, deletes suck */

	/* the usual consistency checks */
	if(len > BT_MAXK(b)) {
		bt_errno(b) = BT_KTOOBIG;
		return(BT_ERR);
	}

	if(len <= 0) {
		bt_errno(b) = BT_ZEROKEY;
		return(BT_ERR);
	}

	if(bt_seekdown(b,key,len) == BT_ERR)
		return(BT_ERR);

	/* set current stack level */
	staklev = b->sblk.levs - 1;
	dpag = b->stack[staklev].pg;

	if((op = bt_rpage(b,dpag)) == NULL)
		return(BT_ERR);

	/* locate first del point, check if not there */
	/* there must not be anything to delete. error. */
	sr = bt_srchpg(key,len,bt_dtype(b),b->cmpfn,&key_rrn,&keyat,op->p);
	if(sr == BT_NF)
		return(BT_NF);
	if(sr == BT_ERR) {
		bt_errno(b) = BT_PAGESRCH;
		return(BT_ERR);
	}

	if (IS_DUP_CHAIN(key_rrn)) {
	    ret = bt_dupdelete(b, op, keyat, key_rrn, rrn_to_del);
	    if (ret != BT_EOF)
		return ret;
	} else if (rrn_to_del) {
	    *rrn_to_del = key_rrn;
	}

	/* this loop should be comfortably repeated until we perform a */
	/* simple delete without emptying a page. if a page concatentation */
	/* function is added, we would look until we no longer concatenate */
	while(1) {

		/* oddball special case. if the page is root and */
		/* there is no key left to delete, the tree is */
		/* (by definition) empty, having only a high pointer */
		/* left. so, the high pointer is dropped, and */
		/* turned to BT_NULL, instantly converting root */
		/* back to a leaf page. we then return. done */
		if(dpag == b->sblk.root && KEYCNT(op->p) == 0) {
			HIPT(op->p) = BT_NULL;
			KEYLEN(op->p) = 0;
			op->flags = BT_CHE_DIRTY;
			if(bt_wpage(b,op) == BT_ERR)
				return(BT_ERR);
			b->dirt++;
			b->sblk.levs = 1;
			if(bt_wsuper(b) == BT_ERR)
				return(BT_ERR);
			return(BT_OK);
		}

		/* if the page has more than one key OR is an index with */
		/* one key, just drop one key from the page, do not free it */
		/* also, if the page is the root page, do not do free it */
		if(KEYCNT(op->p) > 1 || dpag == b->sblk.root ||
			(KEYCNT(op->p) == 1 && HIPT(op->p) != BT_NULL)) {
			struct	bt_cache *tp;	/* temp copy page */

			if((tp = bt_rpage(b,BT_NULL)) == NULL)
				return(BT_ERR);
			bt_delpg(keyat,op->p,tp->p);

			if(HIPT(op->p) == BT_NULL) {
				b->cpag = op->num;
				b->cky = keyat - 1;
			}

			/* swap the page numbers, invalidate the old, */
			/* mark the new as dirty to force a write */
			tp->num = op->num;
			tp->flags = BT_CHE_DIRTY;
			op->num = BT_NULL;

			if(bt_wpage(b,op) == BT_ERR || bt_wpage(b,tp) == BT_ERR)
				return(BT_ERR);

			/* mark all as well with tree */
			bt_clearerr(b);
			return(BT_OK);
		} else {
			bt_off_t	savlft;
			bt_off_t	savrt;

			savlft = LSIB(op->p);
			savrt = RSIB(op->p);

			if(HIPT(op->p) == BT_NULL) {
				b->cpag = savlft;
				b->cky = -1;
			}

			if(savlft != BT_NULL) {
				if((op = bt_rpage(b,savlft)) == NULL)
					return(BT_ERR);

				if(HIPT(op->p) == BT_NULL)
					b->cky = KEYCNT(op->p) - 1;

				RSIB(op->p) = savrt;
				op->flags = BT_CHE_DIRTY;
				if(bt_wpage(b,op) == BT_ERR)
					return(BT_ERR);
			}

			if(savrt != BT_NULL) {
				if((op = bt_rpage(b,savrt)) == NULL)
					return(BT_ERR);
				LSIB(op->p) = savlft;
				op->flags = BT_CHE_DIRTY;
				if(bt_wpage(b,op) == BT_ERR)
					return(BT_ERR);
			}

			/* sibs are fixed, now free the page */
/*printf("0: freepag(%d)\n", dpag); */
			if(bt_freepage(b,dpag) == BT_ERR)
				return(BT_ERR);
		}

		/* still going, pop stack level */
		staklev--;

		/* set key location for next delete */
		keyat = b->stack[staklev].ky;
		dpag = b->stack[staklev].pg;

		/* allocate a scratch page and read the old */
		if((op = bt_rpage(b,dpag)) == NULL)
			return(BT_ERR);

	}
}
