#include	<stdio.h>
#include    <unistd.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include    <errno.h>
#include	"btconf.h"
#include	"btree.h"
#include	"btintern.h"

extern int read_data_stream(struct bfs_info *bfs, struct bfs_inode *bi,
                            bt_off_t pos, char *buf, size_t *_len);
extern int write_data_stream(struct bfs_info *bfs, struct bfs_inode *bi,
                             bt_off_t pos, const char *buf, size_t *_len);

/*
    The define "BAD_ERR" is used so that user-level tools like
	chkbfs will not panic needlessly but the real bfs will.
*/	
#ifdef USER
#define BAD_ERR printf
#else
#define BAD_ERR bfs_die
#endif


/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.
*/


/*
	all read/write operations are in this file, as well as cache
	management stuff. the only exception to this is in btopen,
	where we write an initial page if creating a tree

	how caching is done in the btree routines: in case you care.
	------------------------------------------------------------
	the bt_rpage and bt_wpage functions do NOT fill a buffer for
	other btree functions to call - they return a pointer to a
	filled cache buffer, which can be operated on and then written
	back out. if an internal function requests page #BT_NULL, that
	tells bt_rpage to just get the least recently used page, OR
	any other scratch pages it has hanging around. preference is
	to re-use a scribbled-on page rather than having to flush a
	real page to disk and (possibly) have to read it right back.
	a flag is raised to mark scratch page buffers as in use, since
	there can be more than one page #BT_NULL and they are always
	maintained at the least-recently used end of the queue.
	writing is done just the opposite: if the page is not BT_NULL
	the page may be synced to disk (depending on cache flags)
	and will be re-inserted at the most-recently used end of the
	cache. if the page IS BT_NULL, any misc. flags get cleared
	(such as the 'this scratch page is in use' flag) and the
	buffer is re-enqueued at the least-recently used.
	the goal of all this stuff is to allow a calling function
	to grab a page in a buffer, request 2 scratch pages, split the
	first into the two scratches, mark the first (original) as
	junk (since the page is now invalidated) and to 'write' it as
	BT_NULL, while 'write'ing the 2 former scratch buffers as real
	pages. All without doing a damn thing but juggling pointers.
*/


int
bt_wsuper(BT_INDEX *b)
{
	size_t len = sizeof(struct bt_super);
	int    err;
	
	/* write a superblock to disk if and only if it is dirty */
	/* keeps files opened O_RDONLY from choking up blood */
	if(b->dirt &&
	   (err = write_data_stream(bfs_info(b), 0, (char *)&b->sblk, &len)) != 0){

		printf("*** bt_wsuper: error doing io to btree superblock (bi 0x%x)\n",
			   b->bi);
		if (err == ENOSPC)
			bfs_die("bt_wsuper: no way dude! bfs 0x%x bi 0x%x\n",b->bfs,b->bi);
		bt_errno(b) = BT_PTRINVAL;
		return(BT_ERR);
	}
	b->dirt = 0;

	return(BT_OK);
}


int
bt_sync(BT_INDEX *b)
{
	size_t            len = bt_pagesiz(b);
	struct	bt_cache *p1;

	/* flip through the cache and flush any pages marked dirty to disk */

	for(p1 = b->lru; p1 != NULL;) {
		if(p1->flags == BT_CHE_DIRTY && p1->num != BT_NULL) {
			int err;
			err = write_data_stream(bfs_info(b),p1->num,(char *)p1->p,&len);
			if(err != 0){
				printf("*** bt_sync write error pg %Ld (bi 0x%x)\n", p1->num, b->bi);
				if (err == ENOSPC)
					bfs_die("bt_sync: ENOSPC bfs 0x%x bi 0x%x\n",b->bfs,b->bi);
				bt_errno(b) = BT_PTRINVAL;
				return(BT_ERR);
			}
		}
		p1 = p1->next;
	}
	return(BT_OK);
}


static void
bt_requeue(BT_INDEX *b, struct bt_cache *p)
{
	/* re-assign the cache buffer to a new (or possibly the same) */
	/* location in the queue. buffers that have been stolen for */
	/* scrap will have a BT_NULL number, and should just go at the */
	/* least-recently used part of the cache, since they are junk. */
	/* buffers with legit values get moved to the most-recently. */
	/* this could all be inlined where appropriate, but gimme a */
	/* break! call it, uh, modular programming. this is still */
	/* plenty quick, I expect. */

	/* if the element is where it already belongs, waste no time */
	if((p->num == BT_NULL && p == b->lru) ||
		(p->num != BT_NULL && p == b->mru))
			return;

	/* unlink */
	if(p->prev != NULL)
		p->prev->next = p->next;
	if(p->next != NULL)
		p->next->prev = p->prev;
	if(p == b->lru)
		b->lru = p->next;
	if(p == b->mru)
		b->mru = p->prev;

	/* relink depending on number of page this buffer contains */
	if (b->lru == NULL) {
		b->mru = b->lru = p;
	} else if(p->num == BT_NULL) {
		b->lru->prev = p;
		p->next = b->lru;
		p->prev = NULL;
		b->lru = p;
	} else {
		b->mru->next = p;
		p->prev = b->mru;
		p->next = NULL;
		b->mru = p;
	}
}


struct bt_cache *
bt_rpage(BT_INDEX *b, bt_off_t num)
{
	size_t           len = bt_pagesiz(b);
	struct bt_cache	*p;

	/* sanity check - only BT_NULL is allowed to be a page less */
	/* than 0 and no pages are allowed to be read past EOF. */
	if(num == 0L || num >= b->sblk.high || (num < 0L && num != BT_NULL) || (num != BT_NULL && (num % bt_pagesiz(b)) != 0)) {
		BAD_ERR("*** bt_rpage num is bad %Ld (bi 0x%x)\n", num, b->bi);
		bt_errno(b) = BT_PTRINVAL;
		return(NULL);
	}
	
	/* scan the cache for the desired page. if it is not there, */
	/* load the desired stuff into the lru. if the requested page */
	/* is BT_NULL we could probably cheat by just using the lru, */
	/* but keeping the code smaller and cleaner is probably nicer */
	for(p = b->mru; p != NULL; p = p->prev) {

		/* if the page number matches and the buffer is not */
		/* marked as an allocated scratch buffer, return it */
		if(num == p->num && p->flags != BT_CHE_LOCKD) {
			/* if it is a scratch buffer, lock it */
			if(p->num == BT_NULL)
				p->flags = BT_CHE_LOCKD;

			/* recache the page (moves it to mru) */
			bt_requeue(b,p);
			return(p);
		}
	}
	
	/* if we haven't allocated our fill of temp pages, just do it */
	if (p == NULL && b->num_cached_pages < b->max_cached_pages) {
		p = (struct bt_cache *)malloc(sizeof(struct bt_cache)); 
		if(p == NULL)
			return NULL;

		if((p->p = (bt_chrp)malloc((unsigned)bt_pagesiz(b))) == NULL) {
			free(p);
			return NULL;
		}

		p->next = NULL;
		p->prev = NULL;
		
		b->num_cached_pages++;
	} else {

		/* obviously, we didnt find it, so flush the lru and read */
		/* one complication is to make sure we dont clobber allocated */
		/* scratch page buffers. we have to seek backwards until we */
		/* get a page that is not locked */
		for(p = b->lru; p != NULL; p = p->next)
			if(p->flags != BT_CHE_LOCKD)
			break;
		
		/* sanity check */
		if(p == NULL) {
			bt_errno(b) = BT_NOBUFFERS;
			return(NULL);
		}

		/* flush if the page is dirty and not a scratch buffer */
		if(p->num != BT_NULL && p->flags == BT_CHE_DIRTY) {
			int err;

			err = write_data_stream(bfs_info(b), p->num,(char *)p->p,&len);
			if(err != 0) {
				printf("*** bt_rpage (wr) bad pg %Ld (bi 0x%x)\n",p->num,b->bi);
				if (err == ENOSPC)
					bfs_die("bt_rpage: flushing dirty: bfs 0x%x bi 0x%x\n",
							b->bfs, b->bi);
				bt_errno(b) = BT_PTRINVAL;
				return(NULL);
			}
		}
	}

	/* read new data if not a scratch buffer */
	if(num != BT_NULL) {

		if(read_data_stream(bfs_info(b), num, (char *)p->p,	&len) != 0) {
			printf("*** bt_rpage (rd) bad pg %Ld (bi 0x%x)\n", num, b->bi);
			bt_errno(b) = BT_PTRINVAL;
			return(NULL);
		}
		p->flags = BT_CHE_CLEAN;
		p->num = num;

	} else {
		p->flags = BT_CHE_LOCKD;
		p->num = BT_NULL;
	}

	bt_requeue(b,p);
	return(p);
}



int
bt_wpage(BT_INDEX *b, struct bt_cache *p)
{
	size_t len = bt_pagesiz(b);
int err;
	
	if(p->num != BT_NULL) {

		if(b->cflg != BT_RWCACHE && p->flags == BT_CHE_DIRTY) {

			err = write_data_stream(bfs_info(b), p->num, (char *)p->p,&len);
			if(err != 0){
				printf("*** bt_wpage bad pg %Ld (bi 0x%x)\n", p->num, b->bi);
				if (err == ENOSPC)
					bfs_die("bt_wpage: ENOSPC! bfs 0x%x bi 0x%x\n", b->bfs,
							b->bi);
				bt_errno(b) = BT_PTRINVAL;
				return(BT_ERR);
			}
			p->flags = BT_CHE_CLEAN;
		}
	} else {
		/* unlock scratch buffer */
		p->flags = BT_CHE_CLEAN;
	}

	bt_requeue(b,p);
	return(BT_OK);
}



int
bt_freepage(BT_INDEX *b, bt_off_t pag)
{
	/* simple free page management is done with a linked */
	/* list linked to the left sibling of each page */
	struct	bt_cache *p;

	if(pag == 0L || pag >= b->sblk.high || (pag < 0L && pag != BT_NULL) ||
	   (pag != BT_NULL && (pag % bt_pagesiz(b)) != 0)) {
		bfs_die("*** bt_freepage page number is bad %Ld\n", pag);
	}

	if((p = bt_rpage(b,pag)) == NULL)
		return(BT_ERR);
	

	{
		bt_off_t tmp = b->sblk.freep;
		
		if(tmp == 0L || tmp >= b->sblk.high || (tmp < 0L && tmp != BT_NULL) ||
		   (tmp != BT_NULL && (tmp % bt_pagesiz(b)) != 0)) {
			bfs_die("*** bt_freepage freep is bad %Ld\n", tmp);
		}
	}

	LSIB(p->p) = b->sblk.freep;

	/* note - this value is SET but never checked. Why ? because the */
	/* in-order insert can't be bothered to reset all the free pointers */
	/* in the free list. this could be useful, however, if someone ever */
	/* writes a tree-patcher program, or something like that. */
	HIPT(p->p) = BT_FREE;

	p->flags = BT_CHE_DIRTY;
	if(bt_wpage(b,p) == BT_ERR)
		return(BT_ERR);

	b->sblk.freep = pag;
	b->dirt = 1;
	return(bt_wsuper(b));
}


bt_off_t
bt_newpage(BT_INDEX *b)
{
	bt_off_t	ret = BT_NULL, pag;
	struct	bt_cache *p;

	if(b->sblk.freep == BT_NULL) {
		size_t len;
		int    err;
		
		ret = b->sblk.high;
		b->sblk.high += bt_pagesiz(b);
		len = bt_pagesiz(b);

		/*
		   here we actually write something to force the growth of the B+Tree
		   to the size we expect.  this is necessary because if we are out
		   of disk space we need to know about it now, not later when we
		   try to write the page for real.  note that even though we're
		   writing one of our cached pages, we don't care about the contents
		   of it -- we're just trying to force the growth of the file. It
		   would be nice to grab an empty page, zero it out and write that
		   but that would increase the minimum number of cached pages we need
		   and that is undesirable.
		*/
		err = write_data_stream(bfs_info(b), ret, (char *)b->mru->p, &len);
		if (err != 0) {
			printf("could not grow btree @ 0x%x to %Ld (bi 0x%x)\n", b,
				   b->sblk.high, b->bi);
			b->sblk.high -= bt_pagesiz(b);
			bt_errno(b) = BT_NOSPC;
			ret = BT_NULL;
			if (err == ENOSPC)
				bfs_die("bt_newpage: ENOSPC! bfs 0x%x bi 0x%x\n",b->bfs,b->bi);
		}
	} else {
		pag = b->sblk.freep;

		if(pag == 0L || pag >= b->sblk.high || (pag < 0L && pag != BT_NULL) ||
		   (pag != BT_NULL && (pag % bt_pagesiz(b)) != 0)) {
			bfs_die("*** bt_newpage 1: sblk.freep is bad %Ld (b @ 0x%x)\n",
					pag, b);
		}

		if((p = bt_rpage(b,b->sblk.freep)) != NULL) {
			ret = b->sblk.freep;

			pag = LSIB(p->p);
			
			if(pag == 0L || pag >= b->sblk.high || (pag < 0L && pag != BT_NULL) ||
			   (pag != BT_NULL && (pag % bt_pagesiz(b)) != 0)) {
				bfs_die("*** bt_newpage 2: sblk.freep is bad %Ld (p @ 0x%x "
						"b @ 0x%x, lsib %Ld)\n", pag, p, b, LSIB(p->p));
			}

			b->sblk.freep = LSIB(p->p);

		} else {
			return(BT_NULL);
		}
	}
	b->dirt = 1;

	if(ret == 0L || ret >= b->sblk.high || (ret < 0L && ret != BT_NULL) ||
	   (ret != BT_NULL && (ret % bt_pagesiz(b)) != 0)) {
		bfs_die("*** bt_newpage 3: return page number is bad %Ld (b @ 0x%x)\n",
				ret, b);
	}


	if(bt_wsuper(b) == BT_ERR)
		return(BT_ERR);
	return(ret);
}
