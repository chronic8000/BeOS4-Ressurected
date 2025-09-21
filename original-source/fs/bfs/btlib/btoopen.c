#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<sys/stat.h>
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

/*	this way of handling opening and configuration is probably	*/
/*	unnecessarily large and awkward, but I think it gives a pretty	*/
/*	high degree of #ifdefability to make it portable across OS 	*/
/*	types, as well as giving a lot of flexibility for those who	*/
/*	never trust default options. 					*/


BT_INDEX *
bt_optopen(int type, ...)
{
	va_list		ap;
	BT_INDEX	*ret;
	struct bt_cache *cp1;
	struct bt_cache *cp2;
	int		dtyp = BT_DFLTDTYPE;
	int		csiz = BT_DFLTCACHE;
	int		cflg = BT_DFLTCFLAG;
	int		psiz = BT_DFLTPSIZ;
	int		operm = S_IREAD|S_IWRITE;
	int		omode = O_RDWR;
	bt_off_t		magic = BT_DFLTMAGIC;
	bt_chrp		labp = NULL;
	int		labl, force_create = 0;
	int		r;
	char		*path = NULL;
	struct bfs_inode   *bi = NULL;
	struct bfs_info    *bfs = NULL;
	size_t len = sizeof(struct bt_super);

	if((ret = (BT_INDEX *)malloc(sizeof(BT_INDEX))) == NULL)
		return(NULL);

	memset(ret, 0, sizeof(*ret));

	/* clear error */
	bt_errno(ret) = BT_NOERROR;

	/* zero this just in case */
	bt_cmpfn(ret) = 0;

	va_start(ap, type);
	r = type;
/*	while((r = va_arg(ap,int)) != 0) { */
	while(r != 0) {
		switch(r) {
			case	BT_INODE:
				bi = va_arg(ap,struct bfs_inode *);
				break;

			case	BT_BFS_INFO:
				bfs = va_arg(ap,struct bfs_info *);
				break;

			case	BT_PATH:
				path = va_arg(ap,char *);
				break;

			case	BT_PSIZE:
				psiz = va_arg(ap,int);
				if(psiz < sizeof(struct bt_super))
					psiz = sizeof(struct bt_super);
				break;

			case	BT_CACHE:
				csiz = va_arg(ap,int);
				if(csiz < 0)
					csiz = 0;
				break;

			case	BT_CFLAG:
				cflg = va_arg(ap,int);
				break;

			case	BT_OMODE:
				force_create = va_arg(ap,int);
				break;

			case	BT_OPERM:
				operm = va_arg(ap,int);
				break;

			case	BT_MAGIC:
				magic = va_arg(ap,bt_off_t);
				break;

			case	BT_LABEL:
				labp = va_arg(ap,bt_chrp);
				labl = va_arg(ap,int);
				break;

			case	BT_DTYPE:
				dtyp = va_arg(ap,int);
				/* next arg BETTER be a valid funcp ! */
				if(dtyp == BT_USRDEF)
					bt_cmpfn(ret) = va_arg(ap,FUNCP);
				break;

			default:
				goto bombout;
		}

		r = va_arg(ap, int);
	}

/*	if(path == NULL || (bt_fileno(ret) = open(path,omode,operm)) < 0) */
	if(bi == NULL || bfs == NULL)
		goto bombout;

	ret->bfs = bfs;
	ret->bi  = bi;

	r = read_data_stream(bfs_info(ret), 0, (char *)&ret->sblk, &len);

	/* failure to read anything - initialize tree */
	if(r != 0 || force_create) {
		char	*jnk;
		if((jnk = malloc((unsigned)psiz)) != NULL) {
			ret->sblk.magic = magic;
			bt_pagesiz(ret) = psiz;
			bt_dtype(ret) = dtyp;
			ret->sblk.levs = 1;
			ret->sblk.root = (bt_off_t)psiz;
			ret->sblk.freep = BT_NULL;
			ret->sblk.high = (bt_off_t)(2 * psiz);

			/* mark super block as dirty and sync */
			ret->dirt = 1;

			/* write and pretend we read a legit superblock */
			if(bt_wsuper(ret) != BT_OK) {
				free(jnk);
				goto bombout;
			}
			
			r = sizeof(struct bt_super);

			/* now make jnk into an empty first page */
			KEYCNT(jnk) = 0;
			KEYLEN(jnk) = 0;
			LSIB(jnk) = RSIB(jnk) = BT_NULL;
			HIPT(jnk) = BT_NULL;

			/* now, since the cache is not set up yet, we */
			/* must directly write the page. */
			len = psiz;
			r = write_data_stream(bfs_info(ret), psiz, jnk, &len);
			(void)free(jnk);
		}
	}

	/* yet another sanity check */
	if(r != 0 || ret->sblk.magic != magic)
		goto bombout;

	/* label it if we are supposed to */
	if(labp && bt_wlabel(ret,labp,labl) == BT_ERR)
		goto bombout;

	/* initialize locator stack */
	ret->shih = ret->sblk.levs + 2;
	ret->stack = (struct bt_stack *)malloc((unsigned)(ret->shih * sizeof(struct bt_stack)));
	if(ret->stack == NULL)
		goto bombout;

	/* initialize cache */
	cp2 = ret->lru = NULL;
	csiz += BT_MINCACHE;

	/* just in case some bonehead asks for tons of unused cache */
	if(cflg == BT_NOCACHE)
		csiz = BT_MINCACHE;

	ret->num_cached_pages = 0;
	ret->max_cached_pages = csiz;

#if 1
	ret->lru = ret->mru = NULL;
#else	
	while(csiz--) {
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
	ret->cflg = cflg;

	/* no valid current key */
	ret->cpag = BT_NULL;

	/* all is well */
	return(ret);

bombout:
	(void)bt_close(ret);
	return(NULL);
}
