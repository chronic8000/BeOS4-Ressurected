#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
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
bt_load(BT_INDEX *b, bt_chrp key, int len, bt_off_t rrn, int flag)
{
	struct	bt_cache *op = NULL;
	struct	bt_cache *np = NULL;
	bt_off_t	irrn;
	bt_off_t	savold = BT_NULL;
	int	slev;

	/* first insert is always at leaf lev. */
	/* note that we use the stack backwards here */
	slev = 0;

	irrn = rrn;

	/* if flag is BOF, zap the tree and ready for load */
	if(flag == BT_BOF) {
		return(bt_zap(b));
	} 

	/* if flag is BT_EOF close up and finish the tree */
	if(flag == BT_EOF) {
		return(BT_OK);
	}

	/* if the flag is wrong, die. */
	if(flag != BT_OK) {
		bt_errno(b) = BT_BADUSERARG;
		return(BT_ERR);
	}

	/* safety valves */
	if(len > BT_MAXK(b)) {
		bt_errno(b) = BT_KTOOBIG;
		return(BT_ERR);
	}

	if(len <= 0) {
		bt_errno(b) = BT_ZEROKEY;
		return(BT_ERR);
	}

	/* set the bottom of the stack */
	b->stack[b->sblk.levs - 1].pg = b->sblk.root;


	/* this is similar to insert, but simpler */
	while(1) {

		if((op = bt_rpage(b,b->stack[slev].pg)) == NULL)
			goto bombout;

		/* if key will fit in page, perform simple insert */
		if((int)KEYUSE(op->p) + len + sizeof(int) + sizeof(bt_off_t) < bt_pagesiz(b)) {
			if((np = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;

			bt_inspg(key,len,&irrn,0,op->p,np->p);

			/* adjust caching and write */
			np->num = op->num;
			np->flags = BT_CHE_DIRTY;
			op->num = BT_NULL;

			if(bt_wpage(b,op) == BT_ERR || bt_wpage(b,np) == BT_ERR)
					return(BT_ERR);

			/* e finito */
			bt_clearerr(b);
			return(BT_OK);
		} else {
			bt_off_t	npag;
			int	indexsplit = 0;

			if((npag = bt_newpage(b)) == BT_NULL)
				goto bombout;

			if(HIPT(op->p) != BT_NULL)
				indexsplit++;

			/* fix up left sib in old page and start new */
			LSIB(op->p) = npag;		
			op->flags = BT_CHE_DIRTY;

			if(bt_wpage(b,op) == BT_ERR)
				goto bombout;

			if((np = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;

			KEYCNT(np->p) = KEYLEN(np->p) = 0;
			RSIB(np->p) = b->stack[slev].pg;
			LSIB(np->p) = HIPT(np->p) = BT_NULL;

			if(!indexsplit) {
				if((op = bt_rpage(b,BT_NULL)) == NULL)
					goto bombout;

				bt_inspg(key,len,&irrn,0,np->p,op->p);
				np->num = BT_NULL;
				op->num = npag;
				op->flags = BT_CHE_DIRTY;
				if(bt_wpage(b,np) == BT_ERR ||
					bt_wpage(b,op) == BT_ERR)
						goto bombout;
			} else {
				HIPT(np->p) = irrn;
				np->num = npag;
				np->flags = BT_CHE_DIRTY;
				if(bt_wpage(b,np) == BT_ERR)
					goto bombout;
			}

			/* new rrn is new page */
			irrn = npag;

			/* new current page is the new page */
			/* at this point the old one is history */
			savold = b->stack[slev].pg;
			b->stack[slev].pg = npag;
		}

		/* pop down a level (really up) */
		slev++;
		/* dynamically deal w/ stack overruns */
		if(b->sblk.levs == b->shih - 2) {
			b->shih += 3;
			b->stack = (struct bt_stack *)realloc((char *)b->stack,(unsigned)(b->shih * sizeof(struct bt_stack)));
			if(b->stack == NULL)
				return(BT_ERR);
		}

		/* do we need a new root page ? */
		if(slev == b->sblk.levs) {
			if((b->sblk.root = bt_newpage(b)) == BT_NULL)
				goto bombout;

			if((op = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;

			/* set it up as blank except the high pointer */
			/* on the next iteration, the key will be inserted */
			LSIB(op->p) = RSIB(op->p) = BT_NULL;
			KEYLEN(op->p) = KEYCNT(op->p) = 0;
			HIPT(op->p) = savold;

			op->num = b->sblk.root;
			op->flags = BT_CHE_DIRTY;

			b->dirt++;
			b->sblk.levs++;

			b->stack[b->sblk.levs - 1].pg = b->sblk.root;
			if(bt_wpage(b,op) == BT_ERR || bt_wsuper(b) == BT_ERR)
				goto bombout;
		}
	}

	/* argh ! */
bombout:
	if(op != NULL)
		(void)bt_wpage(b,op);
	if(np != NULL)
		(void)bt_wpage(b,np);
	return(BT_ERR);
}
