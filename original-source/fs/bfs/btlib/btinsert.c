#include	<stdio.h>
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

int be_chatty = 0;


int
bt_insert(BT_INDEX *b, bt_chrp key, int len, bt_off_t rrn, int dupflg)
{
	struct	bt_cache *kp;	/* scratch buffer for promoted keys */
	struct	bt_cache *op;	/* old page */
	int			staklev;
	bt_off_t	ipag;		/* page to insert into */
	bt_off_t	ival;		/* index value to insert */
	int			keyat;		/* key # at which to insert */
	bt_chrp		kp1;		/* rotating key buffer pointers */
	bt_chrp		kp2;
	bt_off_t	ptj;		/* junk */
	int			sr;
	bt_off_t	npag=BT_NULL;/* new page # */
	bt_off_t	nr=BT_NULL;	/* new page for the root block */

	if(len > BT_MAXK(b)) {
		bt_errno(b) = BT_KTOOBIG;
		return(BT_ERR);
	}

	if (rrn < 0) {
		return BT_ERR;
	}

	if(len <= 0) {
		bt_errno(b) = BT_ZEROKEY;
		return(BT_ERR);
	}

	if(bt_seekdown(b,key,len) == BT_ERR) {
		return(BT_ERR);
	}

	/* set current stack level */
	staklev = b->sblk.levs - 1;
	ipag = b->stack[staklev].pg;
/*printf("ipag == %d, staklev %d (stack @ 0x%x)\n", ipag, staklev, &b->stack[0]); */
	
	/* allocate a scratch buffer for the key and promoted key */
	/* since all keys will be < pagesiz/2, "split" the buffer */
	if((kp = bt_rpage(b,BT_NULL)) == NULL)
		return(BT_ERR);

	/* set promotion key ptrs */
	kp1 = kp->p;
	/* split buffers MUST be aligned! */
	kp2 = kp->p + DOALIGN(bt_pagesiz(b) / 2);

	(void)memcpy((char *)kp1,(char *)key,len);
	ival = rrn;

	/* invalidate current notion of where we are in the tree */
	b->cpag = BT_NULL;

	/* set up key location for first page insert */
	if((op = bt_rpage(b,ipag)) == NULL)
		goto bombout;

	/* locate insert point, check if duplicate */
	/* this has the side-effect of setting keyat for the first */
	/* run of insert. after the first insert, subsequent key */
	/* positions are gotten from the stack, rather than searching */
	sr = bt_srchpg(kp1,len,bt_dtype(b),b->cmpfn,&ptj,&keyat,op->p);
	if(sr == BT_ERR) {
		bt_errno(b) = BT_PAGESRCH;
		return(BT_ERR);
	}

	/* if the search returned OK, we have a duplicate key and */
	/* check to see if dupflg is set - if so, we slam a new */
	/* copy of the rrn in, and return immediately. */
	if(sr == BT_OK && HIPT(op->p) == BT_NULL) {
		/* handle dup keys */
		if(dupflg == 0) {
			bt_errno(b) = BT_DUPKEY;
			goto bombout;
		} else {
		    if (bt_wpage(b,kp) == BT_ERR)  /* free up this scratch page */
			return BT_ERR;
		    
		    return bt_dupinsert(b, op, keyat, rrn);
		}
	}


	/* this loop should be comfortably repeated until we perform a */
	/* simple insert without a split, which will clean everything up */
	/* and return correctly. breaking out indicates a fatal problem */
	while(1) {

		/* here is where we figure out if we need to split, or */
		/* if we can just perform a simple insert instead */
		if((int)KEYUSE(op->p) + len + sizeof(int) + sizeof(bt_off_t) < bt_pagesiz(b)) {
			struct	bt_cache *tp;

			if((tp = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;

			bt_inspg(kp1,len,&ival,keyat,op->p,tp->p);
			/* swap the page numbers, invalidate the old, */
			/* mark the new as dirty to force a write */
			tp->num = op->num;
			tp->flags = BT_CHE_DIRTY;
			op->num = BT_NULL;

			if(bt_wpage(b,op) == BT_ERR ||
				bt_wpage(b,tp) == BT_ERR ||
				bt_wpage(b,kp) == BT_ERR)
				return(BT_ERR);

			/* mark all as well with tree */
			bt_clearerr(b);
			return(BT_OK);
		} else {
			struct	bt_cache *lp;	/* new page to hold low keys */
			struct	bt_cache *hp;	/* new page to hold hi keys */
			bt_off_t	savlft;		/* saved left sib page # */

			/* allocate new page for low keys */
			if((npag = bt_newpage(b)) == BT_NULL)
				goto bombout;

			/* 
			   Only get this new page if ipag is the root block.
			   We do it here instead of down below so if it fails
			   we can back out gracefully.
			*/
			if (ipag == b->sblk.root) {
				if ((nr = bt_newpage(b)) == BT_NULL)
					goto bombout;
			}

			/* allocate new scratch page for low keys */
			if((lp = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;
			/* allocate new scratch page for low keys */
			if((hp = bt_rpage(b,BT_NULL)) == NULL)
				goto bombout;

			/* and do the thing */
			bt_splpg(kp1,len,&ival,keyat,bt_pagesiz(b)/2,op->p,lp->p,hp->p,kp2,&len);


			/* patch sibs */
			LSIB(hp->p) = npag;
			RSIB(hp->p) = RSIB(op->p);
			LSIB(lp->p) = LSIB(op->p);
			savlft = LSIB(op->p);
			RSIB(lp->p) = ipag;

			/* mark newly split pages as real */
			lp->num = npag;
			lp->flags = BT_CHE_DIRTY;
			hp->num = ipag;
			hp->flags = BT_CHE_DIRTY;

			op->num = BT_NULL;

			if(bt_wpage(b,op) == BT_ERR ||
				bt_wpage(b,lp) == BT_ERR ||
				bt_wpage(b,hp) == BT_ERR)
				goto bombout;

			/* patch right sib ptr of low pages left sib */
			if(savlft != BT_NULL) {
				if((op = bt_rpage(b,savlft)) == NULL)
					goto bombout;
				RSIB(op->p) = npag;
				op->flags = BT_CHE_DIRTY;
				if(bt_wpage(b,op) == BT_ERR)
					goto bombout;
			}

			/* since we are not done, the new value */
			/* ptr to insert at the next level is that */
			/* of the new page we just allocated */
			ival = npag;


			/* if current page was root, make new root */
			if(ipag == b->sblk.root) {
				/* we already created the new page up above */

				/* two scratch pages */
				if((op = bt_rpage(b,BT_NULL)) == NULL)
					goto bombout;
				if((lp = bt_rpage(b,BT_NULL)) == NULL)
					goto bombout;

				/* prime empty root page */
				LSIB(op->p) = RSIB(op->p) = BT_NULL;
				KEYCNT(op->p) = 0;
				KEYLEN(op->p) = 0;
				HIPT(op->p) = ipag;

				/* we already know where to insert */
				bt_inspg(kp2,len,&npag,0,op->p,lp->p);

				/* fix cache stuff and requeue */
				op->num = BT_NULL;
				lp->flags = BT_CHE_DIRTY;
				lp->num = nr;
				

				if(bt_wpage(b,lp) == BT_ERR ||
					bt_wpage(b,kp) == BT_ERR ||
					bt_wpage(b,op) == BT_ERR)
					goto bombout;

				/* finally, sync up root */
				b->sblk.root = nr;
				b->sblk.levs++;
				b->dirt++;
				if(bt_wsuper(b) == BT_ERR)
					goto bombout;

				/* mark all as well with tree */
				bt_clearerr(b);

				/* return - we are done */
				return(BT_OK);
			}
		}

		/* still going, pop stack level */
		staklev--;
		keyat = b->stack[staklev].ky;

		/* current insert page is read from stack */
		ipag = b->stack[staklev].pg;

		if((op = bt_rpage(b,ipag)) == NULL)
			goto bombout;

		/* flip key buffer ptrs */
		if(kp1 != kp->p) {
			kp1 = kp->p;
			kp2 = kp->p + DOALIGN(bt_pagesiz(b) / 2);
		} else {
			kp1 = kp->p + DOALIGN(bt_pagesiz(b) / 2);
			kp2 = kp->p;
		}
	}

bombout:
	/* if we reach this point, some fatal error has doubtless occurred */
	/* try to bail out, though the tree is almost certainly toast */
	if (npag != BT_NULL)
		bt_freepage(b, npag);
	if (nr != BT_NULL)
		bt_freepage(b, nr);
	if(op != NULL)
		(void)bt_wpage(b,op);
	if(kp != NULL)
		(void)bt_wpage(b,kp);
	return(BT_ERR);
}
