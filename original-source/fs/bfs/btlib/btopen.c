#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/file.h>
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


BT_INDEX *
bt_open(struct bfs_info *bfs, struct bfs_inode *bi, int flags, int mode)
{
	BT_INDEX	*ret;
	struct bt_cache *cp1;
	struct bt_cache *cp2;
	int		r;
	size_t  len = sizeof(struct bt_super);

	if((ret = (BT_INDEX *)malloc(sizeof(BT_INDEX))) == NULL)
		return(NULL);

	memset(ret, 0, sizeof(*ret));

	/* clear error */
	bt_errno(ret) = BT_NOERROR;

	/* set funcp to something inoffensive */
	bt_cmpfn(ret) = 0;

#if 0
	if((bt_fileno(ret) = open(path,(flags|O_RDWR),mode)) < 0)
		goto bombout;
#endif
	
	ret->bfs = bfs;
	ret->bi  = bi;

	r = read_data_stream(bfs_info(ret), 0, (char *)&ret->sblk,
						 &len);

	/* failure to read anything - initialize tree */
	if (r != 0) {
		char	*jnk;
		if((jnk = malloc((unsigned)BT_DFLTPSIZ)) != NULL) {
			ret->sblk.magic = BT_DFLTMAGIC;
			bt_pagesiz(ret) = BT_DFLTPSIZ;
			bt_dtype(ret) = BT_DFLTDTYPE;
			ret->sblk.levs = 1;
			ret->sblk.root = (bt_off_t)BT_DFLTPSIZ;
			ret->sblk.freep = BT_NULL;
			ret->sblk.high = (bt_off_t)(2 * BT_DFLTPSIZ);

			/* mark super block as dirty and sync */
			ret->dirt = 1;

			/* write and pretend we read a legit superblock */
			if(bt_wsuper(ret) == BT_OK)
				r = sizeof(struct bt_super);

			/* now make jnk into an empty first page */
			KEYCNT(jnk) = 0;
			KEYLEN(jnk) = 0;
			LSIB(jnk) = RSIB(jnk) = BT_NULL;
			HIPT(jnk) = BT_NULL;

			/* now, since the cache is not set up yet, we */
			/* must directly write the page. */
			len = BT_DFLTPSIZ;
			r = write_data_stream(bfs_info(ret), BT_DFLTPSIZ, jnk, &len);
			(void)free(jnk);
		}
	}

	/* yet another sanity check */
	if(r != 0 || ret->sblk.magic != BT_DFLTMAGIC)
		goto bombout;

	/* initialize locator stack */
	ret->shih = ret->sblk.levs + 2;
	ret->stack = (struct bt_stack *)malloc((unsigned)(ret->shih * sizeof(struct bt_stack)));
	if(ret->stack == NULL)
		goto bombout;

	ret->num_cached_pages = 0;
	ret->max_cached_pages = BT_MINCACHE;

#if 1
	ret->lru = ret->mru = NULL;
#else	
	/* initialize cache */
	cp2 = ret->lru = NULL;
	for(r = 0; r < BT_MINCACHE + BT_DFLTCACHE; r++) {
		cp1 = (struct bt_cache *)malloc(sizeof(struct bt_cache)); 
		if(cp1 == NULL)
			goto bombout;

		if(ret->lru == NULL)
			ret->lru = cp1;
		cp1->prev = cp2;
		cp1->num = BT_NULL;
		cp1->next = NULL;
		cp1->flags = BT_CHE_CLEAN;
		if((cp1->p = (bt_chrp)malloc((unsigned)bt_pagesiz(ret))) == NULL)
			goto bombout;
		if(cp2 != NULL)
			cp2->next = cp1;

		cp2 = cp1;
	}
	ret->mru = cp1;
#endif

	/* set cache type flag */
	ret->cflg = BT_DFLTCFLAG;

	/* no valid current key */
	ret->cpag = BT_NULL;

	/* all is well */
	return(ret);

bombout:
	(void)bt_close(ret);
	return(NULL);
}
