#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include "btconf.h"
#include "btree.h"
#include "btintern.h"

/*
   This file contains routines that manage duplicate entries in
   a btree.  Duplicates are handled by hanging a chain of blocks
   off the the rrn (the user data) field of the btree node.  The
   blocks are just stored (and searched) sequentially.
*/


int
bt_getnextdup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn)
{
    int ind;
    struct bt_cache *p;
    bt_off_t   next, pg;
	
    if (b->dup_page == 0) {
	b->dup_page  = DUP_CHAIN_VAL(start_page);
	b->dup_index = -1;
    }

    if (IS_DUP_FRAG(start_page)) {
	pg  = DUP_FRAG_VAL(b->dup_page);
	ind = WHICH_DUP_FRAG(b, b->dup_page);
    } else {
	pg  = b->dup_page;
	ind = 0;
    }

    if ((p = bt_rpage(b, pg)) == NULL) {
	b->dup_page = b->dup_index = 0;
	return BT_ERR;
    }

    b->dup_index++;

    if (IS_DUP_FRAG(b->dup_page) == 0) {   /* then it's a dup chain */
	if (b->dup_index >= NUM_DUPS(p->p) && NEXT_DUP(p->p) == BT_NULL) {
	    b->dup_page  = 0;
	    b->dup_index = 0;
	
	    bt_wpage(b, p);
	    
	    return 0;           /* we're all done, go home */
	}

	if (b->dup_index >= NUM_DUPS(p->p)) {
	    next = NEXT_DUP(p->p);
	    
	    if (bt_wpage(b, p) == BT_ERR)
		return BT_ERR;

	    p = bt_rpage(b, next);
	    if (p == NULL)
		return BT_ERR;
	    
	    b->dup_page  = next;
	    b->dup_index = 0;
	}

	*retrrn = DUPS(p->p)[b->dup_index];
	bt_wpage(b, p);

	return 1;
    }

    /* if we're here the page is a dup fragment */
    if (b->dup_index >= NUM_DUPS_IN_FRAG(p->p, ind)) {
	b->dup_index = 0;
	b->dup_page  = 0;

	bt_wpage(b, p);

	return 0;
    }

    *retrrn = FRAG_DUPS(p->p, ind)[b->dup_index];
    bt_wpage(b, p);

    return 1;
}


int
bt_gotolastdup(BT_INDEX *b, bt_off_t *retrrn)
{
    int              ind;
    struct bt_cache *p;
    bt_off_t  prev = BT_NULL, pg;
	
    if (b->dup_page == 0) {
	return BT_ERR;
    }

    b->dup_page = DUP_CHAIN_VAL(b->dup_page);
    if (IS_DUP_FRAG(b->dup_page) == 0) {
	/* the we have to go to the end of the list.. ugh! */
	while (b->dup_page != BT_NULL) {
	    p = bt_rpage(b, b->dup_page);
	    if (p == NULL) {
		b->dup_page = 0;
		return BT_ERR;
	    }
		
	    prev = b->dup_page;
	    b->dup_page  = NEXT_DUP(p->p);
	    b->dup_index = NUM_DUPS(p->p) - 1;
	    
	    *retrrn = DUPS(p->p)[b->dup_index--];

	    bt_wpage(b, p);
	}

	b->dup_page = prev;
    } else {
	pg  = DUP_FRAG_VAL(b->dup_page);
	ind = WHICH_DUP_FRAG(b, b->dup_page);

	p = bt_rpage(b, pg);
	if (p == NULL)
	    return BT_ERR;

	b->dup_index = NUM_DUPS_IN_FRAG(p->p, ind) - 1;
	*retrrn = FRAG_DUPS(p->p, ind)[b->dup_index--];

	bt_wpage(b, p);
    }

    return BT_OK;
}

int
bt_getprevdup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn)
{
    int              ind;
    struct bt_cache *p;
    bt_off_t  prev = BT_NULL, pg;
	
    if (b->dup_page == 0) {
	b->dup_page  = DUP_CHAIN_VAL(start_page);

	if (IS_DUP_FRAG(b->dup_page) == 0) {
	    /* the we have to go to the end of the list.. ugh! */
	    while (b->dup_page != BT_NULL) {
		p = bt_rpage(b, b->dup_page);
		if (p == NULL) {
		    b->dup_page = 0;
		    return BT_ERR;
		}
		
		prev = b->dup_page;
		b->dup_page  = NEXT_DUP(p->p);
		b->dup_index = NUM_DUPS(p->p);

		bt_wpage(b, p);
	    }

	    b->dup_page = prev;
	} else {
	    pg  = DUP_FRAG_VAL(b->dup_page);
	    ind = WHICH_DUP_FRAG(b, b->dup_page);

	    p = bt_rpage(b, pg);
	    if (p == NULL)
		return BT_ERR;

	    b->dup_index = NUM_DUPS_IN_FRAG(p->p, ind);
	    bt_wpage(b, p);
	}
    }

    if (IS_DUP_FRAG(b->dup_page)) {
	pg  = DUP_FRAG_VAL(b->dup_page);
	ind = WHICH_DUP_FRAG(b, b->dup_page);
    } else {
	pg  = b->dup_page;
	ind = 0;
    }
	  
    if ((p = bt_rpage(b, pg)) == NULL) {
	b->dup_page = b->dup_index = 0;
	return BT_ERR;
    }

    b->dup_index--;

    if (IS_DUP_FRAG(b->dup_page) == 0) {
	if (b->dup_index < 0 && PREV_DUP(p->p) == BT_NULL) {
	    b->dup_page  = 0;
	    b->dup_index = 0;
	
	    bt_wpage(b, p);
	    
	    return 0;           /* we're all done, go home */
	} else if (b->dup_index < 0) {
	    prev = PREV_DUP(p->p);
	
	    if (bt_wpage(b, p) == BT_ERR)
		return BT_ERR;
	
	    p = bt_rpage(b, prev);
	    if (p == NULL)
		return BT_ERR;
	    
	    b->dup_page  = prev;
	    b->dup_index = NUM_DUPS(p->p) - 1;
	} else if (b->dup_index > NUM_DUPS(p->p)) {
printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
	    b->dup_index = NUM_DUPS(p->p) - 2;
	}
		
	*retrrn = DUPS(p->p)[b->dup_index];
	bt_wpage(b, p);

	return 1;
    }

    /* if we get here then we're just a dup frag page */
    if (b->dup_index < 0) {
	b->dup_page  = 0;
	b->dup_index = 0;
	
	bt_wpage(b, p);
	    
	return 0;           /* we're all done, go home */
    } else if (b->dup_index >= NUM_DUPS_IN_FRAG(p->p, ind)) {
printf("YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n");
	b->dup_index = NUM_DUPS_IN_FRAG(p->p, ind) - 2;
    }

    *retrrn = FRAG_DUPS(p->p, ind)[b->dup_index];
    bt_wpage(b, p);

    return 1;
}


int
bt_find_dup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn)
{
    int i, last_num_dups = 0, ret = BT_OK;
    struct	bt_cache   *p;
    bt_off_t   last, *dups;
    
    if (*retrrn < 0)
	bt_die("find_dup called with rrn %d < 0\n", *retrrn);

    if (IS_DUP_FRAG(start_page)) {   /* deal with this here */
	int i, ind, num;
	
	/*
	   this is actually correct (because we don't want to mask
	   off the FRAG bit since future getnext's or getprev's need it)
	*/
	b->dup_page = DUP_CHAIN_VAL(start_page);

	p = bt_rpage(b, DUP_FRAG_VAL(b->dup_page));
	if (p == NULL) {
	    b->dup_page = b->dup_index = 0;
	    return BT_ERR;
	}

	ind = WHICH_DUP_FRAG(b, start_page);
	num = NUM_DUPS_IN_FRAG(p->p, ind);
	for(i=0; i < num; i++) {
	    if (FRAG_DUPS(p->p, ind)[i] >= *retrrn)
		break;
	}

	if (i >= num) {   /* then we did not find it */
	    b->dup_index = b->dup_page = 0;
	    bt_wpage(b, p);	
	    return BT_NF;
	}

	if (FRAG_DUPS(p->p, ind)[i] > *retrrn) {
	    *retrrn = FRAG_DUPS(p->p, ind)[i];  /* store what we did find */
	    b->dup_index = i - 1;

	    ret = BT_NF;
	} else if (FRAG_DUPS(p->p, ind)[i] == *retrrn) {
	    b->dup_index = i;

	    ret = BT_OK;
	} else {
	    bt_die("you hoser! you're not > or ==  !\n");
	}

	bt_wpage(b, p);	
	return ret;
    }

    /* from here on out it's just standard dup chain stuff */

    last = b->dup_page = DUP_CHAIN_VAL(start_page);
    b->dup_index = 0;
    
    while (b->dup_page != BT_NULL) {
	if ((p = bt_rpage(b, b->dup_page)) == NULL) {
	    b->dup_page = b->dup_index = 0;
	    return BT_ERR;
	}

	dups = DUPS(p->p);
	for(i=0; i < NUM_DUPS(p->p); i++) {
	    if (dups[i] >= *retrrn)
		break;
	}

	if (i >= NUM_DUPS(p->p)) {
	    last = b->dup_page;
	    last_num_dups = NUM_DUPS(p->p) - 1;
	    
	    b->dup_page = NEXT_DUP(p->p);
	    bt_wpage(b, p);
	    continue;
	}
	    

	if (dups[i] == *retrrn) {
	    b->dup_index = i;
	    bt_wpage(b, p);
	    break;
	}

	if (dups[i] > *retrrn) {
	    *retrrn = dups[i];      /* store what we actually found */
	    if (i == 0) {
		b->dup_page  = last;
		b->dup_index = last_num_dups;
	    } else {
		b->dup_index = i - 1;
	    }

	    bt_wpage(b, p);
	    ret = BT_NF;
	    break;
	}

	bt_die("your hardware is busted!\n");
    }

    if (b->dup_page == BT_NULL) {
	b->dup_page = 0;
	ret = BT_NF;
    }
    
    return ret;
}



/*
   Here are the routines for inserting into a dup chain (as
   opposed to a dup fragment, which is below
*/   

static int
append_dup_chain(BT_INDEX *b, struct bt_cache *p, bt_off_t new_rrn)
{
    bt_off_t npg;
    struct bt_cache *new_page;
    
    if ((npg = bt_newpage(b)) == BT_NULL) {
	return BT_ERR;
    }

    if ((new_page = bt_rpage(b, BT_NULL)) == NULL) {
	bt_freepage(b, npg);
	bt_wpage(b, p);
	return BT_ERR;
    }

    b->num_dup_pages++;

    new_page->num   = npg;
    new_page->flags = BT_CHE_DIRTY;

    PREV_DUP(new_page->p) = p->num;
    NEXT_DUP(new_page->p) = BT_NULL;
    NUM_DUPS(new_page->p) = 1;

    DUPS(new_page->p)[0] = new_rrn;
	    
    NEXT_DUP(p->p) = new_page->num;
    p->flags = BT_CHE_DIRTY;

    if (bt_wpage(b, new_page) == BT_ERR)
	return BT_ERR;

    if (bt_wpage(b, p) == BT_ERR)
	return BT_ERR;
    
    return BT_OK;
}



static int
split_dup_page(BT_INDEX *b, struct bt_cache *split,
	       int index, bt_off_t new_rrn)
{
    int      copy_n;
    bt_off_t npg, next_pg;
    struct bt_cache *new_page, *next;

    if ((npg = bt_newpage(b)) == BT_NULL)
	return BT_ERR;

    if ((new_page = bt_rpage(b, BT_NULL)) == NULL)
	return BT_ERR;

  
    b->num_dup_pages++;

    new_page->num   = npg;
    new_page->flags = BT_CHE_DIRTY;

    PREV_DUP(new_page->p) = split->num;
    NEXT_DUP(new_page->p) = NEXT_DUP(split->p);

    next_pg = NEXT_DUP(split->p);
    NEXT_DUP(split->p) = new_page->num;

    copy_n = NUM_DUPS(split->p) - index;
    memcpy(DUPS(new_page->p), &DUPS(split->p)[index], copy_n * sizeof(bt_off_t));
    
    DUPS(split->p)[index] = new_rrn;
    NUM_DUPS(split->p) = index + 1;

    NUM_DUPS(new_page->p) = copy_n;


    if (bt_wpage(b, new_page) == BT_ERR)
	return BT_ERR;
	    
    split->flags = BT_CHE_DIRTY;
    if (bt_wpage(b, split) == BT_ERR)
	return BT_ERR;

    next = bt_rpage(b, next_pg);
    if (next == NULL) { 
	return BT_ERR;    /* XXXdbg - the tree is fucked at this point */
    }

    PREV_DUP(next->p) = npg;
    next->flags = BT_CHE_DIRTY;
    bt_wpage(b, next);

    return BT_OK;
}


int
add_dup_chain(BT_INDEX *b, bt_off_t save_rrn, bt_off_t new_rrn)
{
    int              i;
    bt_off_t        *dups;
    struct bt_cache *np;
    
    if ((np = bt_rpage(b, DUP_CHAIN_VAL(save_rrn))) == NULL) {
	return BT_ERR;
    }
	
    while(1) {
	bt_off_t next;
	
	dups = DUPS(np->p);

	for(i=0; i < NUM_DUPS(np->p); i++) {
	    if (dups[i] == new_rrn) {
		bt_wpage(b, np);
		bt_errno(b) = BT_DUPRRN;
		return BT_ERR;
	    } else if (dups[i] > new_rrn) {
		break;
	    }
	}
	    
	/* append a new dup page (don't split since this is at the end) */
	if (i >= MAX_DUPS(b) && NEXT_DUP(np->p) == BT_NULL) {
	    return append_dup_chain(b, np, new_rrn);
	}

	/* page is full, have to get a new page to store it */
	if (NUM_DUPS(np->p) == MAX_DUPS(b) && i < NUM_DUPS(np->p) &&
	    dups[i] > new_rrn) {
	    return split_dup_page(b, np, i, new_rrn);
	}

	/* now some simple insertion cases */
	if (i < NUM_DUPS(np->p) && dups[i] > new_rrn) {
	    memmove(&dups[i+1], &dups[i], (NUM_DUPS(np->p)-i)*sizeof(bt_off_t));
	    goto cleanout;
	}

	/* adding to the end of a dup page because there is room */
	if (i >= NUM_DUPS(np->p) && NEXT_DUP(np->p) == BT_NULL &&
	    NUM_DUPS(np->p) < MAX_DUPS(b)) {
	    
	    goto cleanout;
	}


	if (NEXT_DUP(np->p) == BT_NULL)
	    bt_die("np %d has no next pointer but we haven't added rrn %d yet!\n",
		   np->num, new_rrn);

	/* if we get here, we have to keep reading through the dup pages */
	next = NEXT_DUP(np->p);
	bt_wpage(b, np);

	if ((np = bt_rpage(b, next)) == NULL) {
	    return BT_ERR;
	}
    }

    bt_die("dup insert is fucked!\n");
    return(BT_OK);

 cleanout:
    dups[i] = new_rrn;
    NUM_DUPS(np->p) += 1;
    np->flags = BT_CHE_DIRTY;

    if (bt_wpage(b, np) == BT_ERR)
	return BT_ERR;

    /* mark all as well with tree */
    bt_clearerr(b);
    return BT_OK;
}


int
create_dup_chain(BT_INDEX *b, struct bt_cache *cp,
		 int keyat, bt_off_t new_rrn)
{
    int i, ind, num;
    struct bt_cache *new_page, *op;
    bt_off_t         npg, *rrns, save_rrn;
	    
    rrns = (bt_off_t *)KEYCLD(cp->p);
    save_rrn = rrns[keyat];

    if (IS_DUP_FRAG(save_rrn) == 0) {
	bt_die("create_dup_chain called but start_rrn 0x%x not a dup frag\n",
	       save_rrn);
    }

    op = bt_rpage(b, DUP_FRAG_VAL(save_rrn));
    if (op == NULL) {
	bt_wpage(b, cp);
	return BT_ERR;
    }
	
    ind = WHICH_DUP_FRAG(b, save_rrn);

    /* first check if this is a duplicate rrn */
    for(i=0; i < NUM_DUPS_IN_FRAG(op->p, ind); i++) {
	if (FRAG_DUPS(op->p, ind)[i] == new_rrn) {
	    bt_wpage(b, op);
	    bt_wpage(b, cp);
	    bt_errno(b) = BT_DUPRRN;
	    return BT_ERR;
	}
    }

    for(i=0; i < MAX_FRAGS_PER_PAGE(b); i++) {
	if (i == ind)
	    continue;

	if (NUM_DUPS_IN_FRAG(op->p, i) != 0)
	    break;
    }
	
    if (i >= MAX_FRAGS_PER_PAGE(b)) {
	new_page = op;           /* cool! we can just re-use this page */
    } else {
	if ((npg = bt_newpage(b)) == BT_NULL) {
	    bt_wpage(b, op);
	    bt_wpage(b, cp);
	    return BT_ERR;
	}

	if ((new_page = bt_rpage(b, BT_NULL)) == NULL) {
	    bt_freepage(b, npg);
	    bt_wpage(b, op);
	    bt_wpage(b, cp);
	    return BT_ERR;
	}

	b->num_dup_pages++;

	new_page->num = npg;
    }
    
    if (new_page == op) {
	/* need to move the current stuff out of the way
	   so we can setup the header */
	if (ind < 2) {

	    memcpy(FRAG_DUPS(op->p, 2), FRAG_DUPS(op->p, ind),
		   NUM_DUPS_IN_FRAG(op->p, ind) * sizeof(bt_off_t));
	    NUM_DUPS_IN_FRAG(op->p, 2) = NUM_DUPS_IN_FRAG(op->p, ind);
	    ind = 2;
	}
    }
	
    num = NUM_DUPS_IN_FRAG(op->p, ind);
    PREV_DUP(new_page->p) = BT_NULL;
    NEXT_DUP(new_page->p) = BT_NULL;
    NUM_DUPS(new_page->p) = num + 1;

    new_page->flags = BT_CHE_DIRTY;

    for(i=0; i < num; i++) {
	if (FRAG_DUPS(op->p, ind)[i] > new_rrn)
	    break;

	DUPS(new_page->p)[i] = FRAG_DUPS(op->p, ind)[i];
    }

    DUPS(new_page->p)[i] = new_rrn;

    for(; i < num; i++) {
	DUPS(new_page->p)[i+1] = FRAG_DUPS(op->p, ind)[i];
    }
    

    rrns[keyat] = MAKE_DUP_CHAIN(new_page->num);
    cp->flags = BT_CHE_DIRTY;
    
    if (op != new_page)
	bt_wpage(b, op);

    if(bt_wpage(b,cp) == BT_ERR || bt_wpage(b,new_page) == BT_ERR)
	return(BT_ERR);
    
    return BT_OK;
}



/*
   here are the routines for inserting into a dup fragment.
*/   

int
create_dup_frag(BT_INDEX *b, struct bt_cache *cp,
		int keyat, bt_off_t new_rrn)
{
    struct bt_cache *new_page;
    bt_off_t         npg, *rrns, save_rrn;
	    
    rrns = (bt_off_t *)KEYCLD(cp->p);
    save_rrn = rrns[keyat];

    if ((npg = bt_newpage(b)) == BT_NULL) {
	bt_wpage(b, cp);
	return BT_ERR;
    }

    if ((new_page = bt_rpage(b, BT_NULL)) == NULL) {
	bt_freepage(b, npg);
	bt_wpage(b, cp);
	return BT_ERR;
    }

    b->num_dup_pages++;

    new_page->num = npg;

    memset(new_page->p, 0, bt_pagesiz(b));

    if (save_rrn < new_rrn) {
	FRAG_DUPS(new_page->p, 0)[0] = save_rrn;
	FRAG_DUPS(new_page->p, 0)[1] = new_rrn;
    } else {
	FRAG_DUPS(new_page->p, 0)[0] = new_rrn;
	FRAG_DUPS(new_page->p, 0)[1] = save_rrn;
    }

    NUM_DUPS_IN_FRAG(new_page->p, 0) = 2;
    
    rrns[keyat]     = MAKE_DUP_FRAG(npg);
    cp->flags       = BT_CHE_DIRTY;
    new_page->flags = BT_CHE_DIRTY;
    
    if(bt_wpage(b,cp) == BT_ERR || bt_wpage(b,new_page) == BT_ERR)
	return(BT_ERR);
    
    return BT_OK;
}



int
add_dup_frag(BT_INDEX *b, struct bt_cache *cp, int keyat, bt_off_t new_rrn)
{
    int              i, ind;
    bt_off_t        *rrns, save_rrn;
    struct bt_cache *np;
    
    rrns = (bt_off_t *)KEYCLD(cp->p);
    
    save_rrn = rrns[keyat];

    np = bt_rpage(b, DUP_FRAG_VAL(rrns[keyat]));
    if (np == NULL) {
	bt_wpage(b, cp);
	return BT_ERR;
    }

    ind = WHICH_DUP_FRAG(b, rrns[keyat]);

    if (NUM_DUPS_IN_FRAG(np->p, ind) + 1 > MAX_DUP_FRAGS) {
	/*
	   ugh! that means we have clean this guy out and
	   create a "real" dup chain.
	*/
	return create_dup_chain(b, cp, keyat, new_rrn);
    }

    bt_wpage(b, cp);

    for(i=0; i < NUM_DUPS_IN_FRAG(np->p, ind); i++) {
	if (FRAG_DUPS(np->p, ind)[i] >= new_rrn)
	    break;
    }

    if (i < NUM_DUPS_IN_FRAG(np->p, ind) &&
	FRAG_DUPS(np->p, ind)[i] == new_rrn) {
	bt_wpage(b, np);
	bt_errno(b) = BT_DUPRRN;

	return BT_ERR;
    }

    if (i < NUM_DUPS_IN_FRAG(np->p, ind)) {
	memmove(&FRAG_DUPS(np->p, ind)[i+1], &FRAG_DUPS(np->p, ind)[i],
		(NUM_DUPS_IN_FRAG(np->p, ind) - i) * sizeof(bt_off_t));

    }

    FRAG_DUPS(np->p, ind)[i] = new_rrn;
    NUM_DUPS_IN_FRAG(np->p, ind) += 1;
    
    np->flags = BT_CHE_DIRTY;
    
    if(bt_wpage(b,np) == BT_ERR)
	return(BT_ERR);

    return BT_OK;
}


int
create_new_dup(BT_INDEX *b, struct bt_cache *cp,
	       int keyat, bt_off_t new_rrn)
{
    int i, j;
    struct bt_cache *np;
    bt_off_t        *rrns, save_rrn;
	    
    rrns = (bt_off_t *)KEYCLD(cp->p);
    save_rrn = rrns[keyat];

    if (new_rrn == save_rrn) {
	bt_wpage(b, cp);
	bt_errno(b) = BT_DUPRRN;
	return BT_ERR;
    }

    /*
       this is potentially pretty darn expensive if there are
       tons of duplicates (because we wind up reading lots of
       fragment pages that are full).  it would be nice if we
       could know if a fragment was full or not without having
       to read it.
    */
    for(i=0; i < KEYCNT(cp->p); i++) {
	/* here we play a little game to insure that we don't lose
	   our "cp" pointer.  what can happen is that because it's
	   not locked, it will fall off the end of the lru list if
	   we read lots of pages here.  This insures that we the
	   "cp" page stays in the cache.  ugly.
	*/   
	bt_wpage(b, cp);
	cp = bt_rpage(b, cp->num);

	if ( ! (IS_DUP_CHAIN(rrns[i]) && IS_DUP_FRAG(rrns[i]))) {
	    continue;
	}

	np = bt_rpage(b, DUP_FRAG_VAL(rrns[i]));
	if (np == NULL) {
	    bt_wpage(b, cp);
	    return BT_ERR;
	}

	for(j=0; j < MAX_FRAGS_PER_PAGE(b); j++)
	    if (NUM_DUPS_IN_FRAG(np->p, j) == 0)
		break;

	if (j < MAX_FRAGS_PER_PAGE(b))
	    break;

	bt_wpage(b, np);
    }

    if (i >= KEYCNT(cp->p)) {
	/* then be the first one on the block to have a frag page! */
	return create_dup_frag(b, cp, keyat, new_rrn);
    }

    /*
       if we get here then we just use the free fragment
       found at index "j" and return.
    */   
    
    if (save_rrn < new_rrn) {
	FRAG_DUPS(np->p, j)[0] = save_rrn;
	FRAG_DUPS(np->p, j)[1] = new_rrn;
    } else {
	FRAG_DUPS(np->p, j)[0] = new_rrn;
	FRAG_DUPS(np->p, j)[1] = save_rrn;
    }

    NUM_DUPS_IN_FRAG(np->p, j) = 2;
    
    if (np->num & j) {
	bt_die("dup fragment offset %d and page %d collided!\n", np->num, j);
    }

    rrns[keyat] = MAKE_DUP_FRAG((np->num | j));
    cp->flags = BT_CHE_DIRTY;
    np->flags = BT_CHE_DIRTY;
    
    if(bt_wpage(b,cp) == BT_ERR || bt_wpage(b,np) == BT_ERR)
	return(BT_ERR);
    
    return BT_OK;
}


int
bt_dupinsert(BT_INDEX *b, struct bt_cache *cp, int keyat, bt_off_t new_rrn)
{
    bt_off_t        *rrns, save_rrn;
    
    rrns = (bt_off_t *)KEYCLD(cp->p);
    
    save_rrn = rrns[keyat];
    
    if (IS_DUP_CHAIN(save_rrn) == 0) { /* have to create a dup chain */
	return create_new_dup(b, cp, keyat, new_rrn);
    }
    
    
    if (IS_DUP_FRAG(save_rrn)) {
	return add_dup_frag(b, cp, keyat, new_rrn);
    } else {
	bt_wpage(b, cp);   /* don't need it any more */
	return add_dup_chain(b, save_rrn, new_rrn);
    }

    bt_die("dupinsert got a little to wild!\n");
    
    return BT_ERR;
}


int
delete_frag(BT_INDEX *b, struct bt_cache *cp, int keyat,
	    bt_off_t key_rrn, bt_off_t *rrn_to_del)
{
    struct   bt_cache *p;
    bt_off_t start, *rrns, tmp;
    int      i, ind;
    
    start = DUP_FRAG_VAL(key_rrn);
    p = bt_rpage(b, start);
    if (p == NULL)
	return BT_ERR;

    ind = WHICH_DUP_FRAG(b, key_rrn);
    for(i=0; i < NUM_DUPS_IN_FRAG(p->p, ind); i++) {
	if (FRAG_DUPS(p->p, ind)[i] >= *rrn_to_del || *rrn_to_del == BT_NULL) {
	    break;
	}
    }

    if (i >= NUM_DUPS_IN_FRAG(p->p, ind)) {
	bt_wpage(b, p);
	bt_wpage(b, cp);

	return BT_NF;
    }

    if (*rrn_to_del == BT_NULL)
	*rrn_to_del = FRAG_DUPS(p->p, ind)[i];

    if (FRAG_DUPS(p->p, ind)[i] > *rrn_to_del) {
	bt_wpage(b, p);
	bt_wpage(b, cp);

	return BT_NF;
    }
    

    /* copy over the dead guy */
    memcpy(&FRAG_DUPS(p->p, ind)[i],
	   &FRAG_DUPS(p->p, ind)[i + 1],
	   (NUM_DUPS_IN_FRAG(p->p, ind) - (i + 1)) * sizeof(bt_off_t));

    NUM_DUPS_IN_FRAG(p->p, ind) -= 1;
    p->flags = BT_CHE_DIRTY;

    if (NUM_DUPS_IN_FRAG(p->p, ind) == 1) {
	/* we're the last guy in this fragment so move the rrn back into cp */
	NUM_DUPS_IN_FRAG(p->p, ind) -= 1;

	rrns = (bt_off_t *)KEYCLD(cp->p);
	rrns[keyat] = FRAG_DUPS(p->p, ind)[0];
	cp->flags = BT_CHE_DIRTY;
    }
    
    for(i=0; i < MAX_FRAGS_PER_PAGE(b); i++) {
	if (NUM_DUPS_IN_FRAG(p->p, i) != 0)
	    break;
    }
    
    tmp = p->num;
    bt_wpage(b, p);
    bt_wpage(b, cp);

    if (i >= MAX_FRAGS_PER_PAGE(b)) {
	bt_freepage(b, tmp);      /* this page is toast!  free it up */
    }
    
    return BT_OK;
}



int
bt_dupdelete(BT_INDEX *b, struct bt_cache *cp, int keyat, bt_off_t key_rrn,
			 bt_off_t *rrn_to_del)
{
    struct   bt_cache *p;
    bt_off_t next, start, *dups, *rrns, tmp;
    int      i, ret = BT_OK;
    
    start = next = DUP_CHAIN_VAL(key_rrn);

    if (IS_DUP_FRAG(start)) {
	return delete_frag(b, cp, keyat, key_rrn, rrn_to_del);
    }

    while(next != BT_NULL) {
	/* see the comment in create_new_dup() about this ditty */
	bt_wpage(b, cp);
	cp = bt_rpage(b, cp->num);

	p = bt_rpage(b, next);
	if (p == NULL) {
	    bt_wpage(b, cp);
	    return BT_ERR;
	}

	dups = DUPS(p->p);
	for(i=0; i < NUM_DUPS(p->p); i++) {
	    if (dups[i] >= *rrn_to_del || *rrn_to_del == BT_NULL)
		break;
	}

	if (i >= NUM_DUPS(p->p)) {  /* didn't find it so keep going */
	    next = NEXT_DUP(p->p);
	    bt_wpage(b, p);
	    continue;
	}

	if (*rrn_to_del == BT_NULL)
	    *rrn_to_del = dups[i];

	if (dups[i] > *rrn_to_del) {
	    bt_wpage(b, cp);
	    bt_wpage(b, p);
	    return BT_NF;
	}

	/*
	   this is the easy case: we're deleting one entry somewhere
	   in the middle of a page of duplicates.  we just copy
	   over the dead guy, mark the page as dirty and move on.
	*/
	if (NUM_DUPS(p->p) > 1) {
	    memmove(&dups[i], &dups[i+1],
		    (NUM_DUPS(p->p) - i - 1) * sizeof(bt_off_t));
	    NUM_DUPS(p->p) -= 1;

	    p->flags = BT_CHE_DIRTY;
	    
	    b->cpag = cp->num;
	    b->cky  = keyat;
	    b->dup_page = p->num;
	    if (i > 0) {
		b->dup_index = i - 1;
	    } else {
		b->dup_index = 0;
	    }

	    bt_wpage(b, p);
	    bt_wpage(b, cp);

	    return BT_OK;
	}

	/*
	   at this point the only entry left in this page is the one 
	   we're deleting so we're going to have to free this page.  
	*/  
	if (NEXT_DUP(p->p) != BT_NULL) {
	    struct bt_cache *tp;

	    tp = bt_rpage(b, NEXT_DUP(p->p));
	    if (tp == NULL)
		return BT_ERR;

	    PREV_DUP(tp->p) = PREV_DUP(p->p);
	    tp->flags = BT_CHE_DIRTY;
	    next = tp->num;
		
	    bt_wpage(b, tp);
	}

	if (PREV_DUP(p->p) != BT_NULL) {
	    struct bt_cache *tp;
	    
	    tp = bt_rpage(b, PREV_DUP(p->p));
	    if (tp == NULL)
		return BT_ERR;

	    NEXT_DUP(tp->p) = NEXT_DUP(p->p);
	    tp->flags = BT_CHE_DIRTY;
	    next = tp->num;
	    
	    bt_wpage(b, tp);
	}


	if (start == p->num) {
	    rrns = (bt_off_t *)KEYCLD(cp->p);

	    if (NEXT_DUP(p->p) == BT_NULL && NUM_DUPS(p->p) == 1) {
		next = 0;
		ret  = BT_EOF;
	    } else {
		rrns[keyat] = MAKE_DUP_CHAIN(NEXT_DUP(p->p));
		next        = NEXT_DUP(p->p);
		cp->flags   = BT_CHE_DIRTY;
		bt_wpage(b, cp);
	    }

	}

	b->cpag = cp->num;
	b->cky  = keyat;
	    
	b->dup_page  = next;
	b->dup_index = 0;
	    
	tmp = p->num;
	bt_wpage(b, p);
	bt_freepage(b, tmp);
	return ret;
	
    }   /* end of while(next != BT_NULL) */
    
    bt_wpage(b, cp);
    return BT_NF;  /* if we get here didn't find it */
}
