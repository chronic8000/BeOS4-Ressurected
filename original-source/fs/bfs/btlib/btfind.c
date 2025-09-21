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
bt_find(BT_INDEX *b, bt_chrp key, int len, bt_off_t *real_rrn)
{
	struct	bt_cache *op;	/* old page */
	int	sr;
	bt_off_t rrn;

	if(len > BT_MAXK(b)) {
		bt_errno(b) = BT_KTOOBIG;
		return(BT_ERR);
	}

	if(len <= 0) {
		bt_errno(b) = BT_ZEROKEY;
		return(BT_ERR);
	}

	b->dup_page = b->dup_index = 0;

	if(bt_seekdown(b,key,len) == BT_ERR)
		return(BT_ERR);

	if((op = bt_rpage(b,b->stack[b->sblk.levs - 1].pg)) == NULL)
		return(BT_ERR);

	b->cpag = op->num;

	/* mark all as well with tree */
	bt_clearerr(b);

	sr = bt_srchpg(key,len,bt_dtype(b),b->cmpfn,&rrn,&b->cky,op->p);
	if(sr == BT_OK) {
		if (IS_DUP_CHAIN(rrn)) {
			if (*real_rrn != BT_ANY_RRN) {
				sr = bt_find_dup(b, rrn, real_rrn);
			} else {
				sr = bt_getnextdup(b, rrn, real_rrn);
				if (sr == 1)
					sr = BT_OK;
				else if (sr == 0)
					sr = BT_NF;
			}
		} else {
			*real_rrn = rrn;
		}

		return(sr);
	}
	if(sr == BT_ERR) {
		bt_errno(b) = BT_PAGESRCH;
		return(BT_ERR);
	}
	b->cky--;
	return(BT_NF);
}
