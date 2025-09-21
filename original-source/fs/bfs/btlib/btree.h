#ifndef	_DEF_BTREE_H

/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.
*/


/*
there are some potential problems with just using char * instead of
unsigned char *, because of sign extension and so on. the following
typedef indicates the best type to use as a pointer to an arbitrary
chunk of data for the system. since a lot of pointer math is done on
these, it should be either a signed or unsigned char, or odd results
may occur.
typedef	char *		bt_chrp;
*/
typedef	unsigned char *	bt_chrp;
#ifndef REAL_LONG_LONG
#define REAL_LONG_LONG 1
#endif
#ifdef REAL_LONG_LONG
typedef long long bt_off_t;
#else
typedef long bt_off_t;
#endif

/* return codes */
#define	BT_OK	0
#define	BT_ERR	-1
#define	BT_NF	1
#define	BT_EOF	2
#define	BT_BOF	3

/* arguments to bt_optopen */
#define	BT_PATH		1
#define	BT_PSIZE	2
#define	BT_CACHE	3
#define	BT_CFLAG	4
#define	BT_OMODE	5
#define	BT_OPERM	6
#define	BT_MAGIC	7
#define	BT_DTYPE	8
#define	BT_LABEL	9
#define	BT_INODE	10
#define	BT_BFS_INFO	11

/* cache modes, acceptable argument to BT_CFLAG */
#define	BT_NOCACHE	0
#define	BT_ROCACHE	1
#define	BT_RWCACHE	2

/* recognized data types, acceptable argument to BT_DTYPE */
#define	BT_STRING	  0
#define	BT_INT		  1
#define	BT_UINT		  2
#define	BT_LONG_LONG  3
#define	BT_ULONG_LONG 4
#define	BT_FLOAT	  5
#define	BT_DOUBLE	  6
#define	BT_USRDEF	  7

/* cache handle */
struct	bt_cache {
	bt_off_t	num;
	struct	bt_cache *next;
	struct	bt_cache *prev;
	bt_chrp	p;
	char	flags;
};

/* super block */
struct	bt_super {
	long	magic;
	int	psiz;
	int	levs;
	int	dtyp;
	bt_off_t	root;
	bt_off_t	freep;
	bt_off_t	high;
};

struct	bt_stack {
	bt_off_t	pg;
	int	ky;
};



struct bfs_info;
struct bfs_inode;
extern int read_data_stream(struct bfs_info *bfs, struct bfs_inode *bi,
                            bt_off_t pos, char *buf, size_t *_len);
extern int write_data_stream(struct bfs_info *bfs, struct bfs_inode *bi,
                             bt_off_t pos, const char *buf, size_t *_len);
extern int bfs_die(const char *fmt, ...);

/* b+tree index handle */
typedef struct bt_index {
	struct bfs_info  *bfs;
	struct bfs_inode *bi;
	int	bt_errno;
	int	dirt;
	int	cflg;
	int	cky;
	bt_off_t	cpag;
	bt_off_t   dup_page;
	bt_off_t   dup_index;
	bt_off_t   num_dup_pages;
	int	(*cmpfn)();
	int num_cached_pages;
	int max_cached_pages;
	struct	bt_super sblk;
	struct	bt_cache *lru;
	struct	bt_cache *mru;
	struct	bt_stack *stack;
	int	shih;
} BT_INDEX;


/* pseudo functions */
#define bfs_info(b)     b->bfs, b->bi
#define	bt_fileno(b)	(b->fd)
#define	bt_pagesiz(b)	(b->sblk.psiz)
#define	bt_dtype(b)	(b->sblk.dtyp)
#define	bt_cmpfn(b)	(b->cmpfn)
#define	bt_errno(b)	(b->bt_errno)
#define	bt_clearerr(b)	(b->bt_errno = BT_NOERROR)

/* biggest size of a key - depends on the page size of the tree */
#define	BT_MAXK(b)	(((b->sblk.psiz-(4*sizeof(int))-(5*sizeof(bt_off_t)))/2) - sizeof(bt_off_t))

/* size of page 0 label - depends on the page size and sblk size */
#define	BT_LABSIZ(b)	(b->sblk.psiz - (sizeof(struct bt_super) + 1))

/* btree error codes and messages */
extern	char	*bt_errs[];

/* error constants */
#define	BT_NOERROR	0
#define	BT_KTOOBIG	1
#define	BT_ZEROKEY	2
#define	BT_DUPKEY	3
#define	BT_PTRINVAL	4
#define	BT_NOBUFFERS	5
#define	BT_LTOOBIG	6
#define	BT_BTOOSMALL	7
#define	BT_BADPAGE	8
#define	BT_PAGESRCH	9
#define	BT_BADUSERARG	10
#define	BT_DUPRRN	11
#define BT_NOSPC        12


#define BT_ANY_RRN  ((bt_off_t)-1)

BT_INDEX *bt_open(struct bfs_info *bfs, struct bfs_inode *bi, int flags, int mode);
BT_INDEX *bt_optopen(int type, ...);
int       bt_close(BT_INDEX *b);

int       bt_insert(BT_INDEX *b, bt_chrp key, int len, bt_off_t rrn, int dupflg);
int       bt_find(BT_INDEX *b, bt_chrp key, int len, bt_off_t *rrn);
int       bt_delete(BT_INDEX *b, bt_chrp key, int len, bt_off_t *rrn_to_del);
int       bt_traverse(BT_INDEX *b, int d, bt_chrp kbuf, int maxlen,
					  int *retlen, bt_off_t *retrrv);
int       bt_goto(BT_INDEX *b, int d);
int       bt_gotolastdup(BT_INDEX *b, bt_off_t *retrrn);
void      bt_dump(BT_INDEX *b, struct bt_cache *p);
void      bt_dup_dump(BT_INDEX *b, struct bt_cache *p);
void      bt_perror(BT_INDEX *b, char *s);

int       bt_sync(BT_INDEX *b);
int       bt_zap(BT_INDEX *b);

int       bt_wlabel(BT_INDEX *b, bt_chrp buf, int siz);
int       bt_rlabel(BT_INDEX *b, bt_chrp buf, int siz);
int       bt_load(BT_INDEX *b, bt_chrp key, int len, bt_off_t rrn, int flag);


#ifndef USER
#ifndef printf
#define printf dprintf
#include <KernelExport.h>
#endif
#endif /* USER */


#define	_DEF_BTREE_H
#endif
