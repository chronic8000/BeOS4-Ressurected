#ifndef	_DEF_BT_INTERN_H

#include <stdlib.h>
#include <string.h>

/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.
*/


/*
	THIS SHOULD NOT BE INCLUDED BY USER-LEVEL PROGRAMS
*/

#define	BT_NULL	((bt_off_t)-1)
#define	BT_FREE	((bt_off_t)-2)

/* make life easier with function pointers */
typedef	int	(*FUNCP)();

/* cache flags - not needed by user code */
#define	BT_CHE_CLEAN	0
#define	BT_CHE_DIRTY	1
#define	BT_CHE_LOCKD	2

/* forward decls for internal funcs only */
extern	struct bt_cache *bt_rpage();
extern	bt_off_t		bt_newpage();
extern	void		bt_inspg();
extern	void		bt_splpg();

#ifndef	NO_BT_DEBUG
extern	void		bt_dump();
#endif

/*
minumum allowable page cache. since page sizes are variable,
the cache is also used to provide buffers for insertion and
deletion, or splitting. if there are not enough, nothing works
this value was determined from intimate knowledge of the code
*/
#define	BT_MINCACHE	6


/*
 Macros used in manipulating btree pages.

 this stuff is the machine specific part - if these macros are
 not right, you are guaranteed to know about it right away when
 this code is run. If you know exact values, you can plug them
 in directly, otherwise, we guess. getting it wrong will waste
 some space, is all. note that the bt_off_ts are clustered at the
 beginning of the page, so that there is less likely to be a
 problem with the ints following. this may cause trouble in some
 odd architectures, and if so, fix it, and let me know.

 debugging a page is hard to do, by its very nature, and it is
 equally hard to build consistency checks into the code to
 validate pages as they are read and written. there is some
 code in bt_rpage() and bt_wpage() that looks for glaringly hosed
 pages, but if something gets past them, serious problems are
 sure to follow.

	Don't even *THINK* about running this past "lint -h" !!!!!

	These macros wind up nesting ~5 layers deep and get pretty
	hairy. If your c-preprocessor is not gutsy enough, have fun
	with the cut and paste !

	Makeup of a page:
	a page is an unstructured mess stuffed into a character buffer
	with all locations being determined via pointer arithmetic.
	this structure is loosely based on Zoellic and Folk's text
	"File Structures: a conceptual toolkit". further, there is
	no distinction between internal pages and leaf pages except
	that the value in the high pointer will be BT_NULL

	fields are (in order)
	- how many -- data type (size) ---------value/description-----
	1	|	bt_off_t	|	page left sibling
	1	|	bt_off_t	|	page right sibling
	1	|	bt_off_t	|	page "high" (overflow) ptr
	1	|	short		|	count of keys in page - keycnt
	1	|	short		|	total length of keys in page
	keycnt  |       key data        |       packed key data
	keycnt	|	short		|	length index (one per key)
	keycnt	|	bt_off_t	|	child/data ptrs (one per key)

	Ideally, pages should be flagged depending on type, and if
	the page contains fixed-size objects (structs, or ints, etc)
	the length index should be left out, saving sizeof(int) 
	bytes/key, which would be susbstantial improvement. This is
	an enhancement considered for release 2.0 if ever, or is
	left as an exercise for the reader.

	--mjr();
*/

/* alignment boundary for bt_off_ts */
#define	ALIGN	sizeof(bt_off_t)
/*
#define	ALIGN	(sizeof(bt_off_t)/sizeof(char))
*/

/*
the actual alignments - the bit with the u_long may break on
some systems. Whatever, it needs to be a size that can give a valid
modulo operator on an address (Suns don't like modulo of pointers)
*/
#define	DOALIGN(s)	(((u_long)s + (ALIGN - 1)) & ~(ALIGN - 1))

/* page siblings */
#define	LSIB(p)		(*(bt_off_t *)(p))
#define	RSIB(p)		(*(bt_off_t *)(p + sizeof(bt_off_t)))
#define	HIPT(p)		(*(bt_off_t *)(p + (2 * sizeof(bt_off_t))))

/* count of keys in page, stored in first short integer value */
#define	KEYCNT(p)	(*(short *)(p + (3 * sizeof(bt_off_t))))

/* length of keys in page, stored in second short integer value */
#define	KEYLEN(p)	(*(short *)(p + sizeof(short) + (3 * sizeof(bt_off_t))))

/* address of key string (pointer to char) */
#define	KEYADR(p)	(p + (2 * sizeof(short)) + (3 * sizeof(bt_off_t)))

/* address of first (short) key index value */
#define	KEYIND(p)	DOALIGN((KEYADR(p) + KEYLEN(p)))

/* address of first (bt_off_t) child pointer value */
#define	KEYCLD(p)	(KEYIND(p) + (KEYCNT(p) * sizeof(short)))

/* # of bytes used by page pointers, sibs, etc */
#define PTRUSE		((2 * sizeof(short)) + (4 * sizeof(bt_off_t)))

/* # of bytes used out of a page */
#define KEYUSE(p)	((KEYCNT(p) * (sizeof(bt_off_t) + sizeof(short))) + PTRUSE + KEYLEN(p))


/*
   The macros below here are all for use with duplicates.  Duplicate keys
   have their (unique) rrn values stored in a separate page.  The basic
   idea is that the if there is a duplicate key, we set its rrn in the
   leaf node to point to a duplicate page where the rrn's are stored.
   The rrn's are kept in sorted order to insure that we can always return
   to the same exact position in the tree if needed.

   However, if each duplicate key get its own separate page, that would be
   *very* costly in terms of size of the index and the time to do insertions
   and deletions. This is because when dealing with file names when something
   is a duplicate there tend to only be a few of them (in general less than 8).  
   So we introduce a notion of a duplicate "fragments" which are small groups
   of rrns stored in a single page.  The idea is that if we are about to insert
   a duplicate, we look to see if anyone else in our leaf page already is a
   duplicate and points to a duplicate fragment page.  If so, we use their page
   instead of allocating one for ourselves.

   Should the number of duplicates for a particular key grow to be more than can
   fit in a duplicate page then we spill it out to its own page (and clear our
   rrn's out of the duplicate fragment page.

   This is a bit complicated but the win is significant when you have lots of
   keys with only a few duplicates.
*/


/* a flag that can be or'ed into an bt_off_t to make it a dup chain pointer */
#define CHAIN_FLAG      (((bt_off_t)1) << (sizeof(bt_off_t)*8 - 1))

/* bit to say if the dup chain pointer page contains dup fragments */
#define DUP_FRAG_FLAG    (((bt_off_t)1) << (sizeof(bt_off_t)*8 - 2))

/* or's in a special flag to indicate that a rrn is dup chain pointer */
#define MAKE_DUP_CHAIN(x)  ((x) | CHAIN_FLAG)

/* or's flag to indicate that a rrn is dup frag pointer */
#define MAKE_DUP_FRAG(x)  ((x) | CHAIN_FLAG | DUP_FRAG_FLAG)

/* checks if a rrn is a dup chain pointer */
#define IS_DUP_CHAIN(x)  ((x) & CHAIN_FLAG)

/* is this dup chain pointer a fragment block */
#define IS_DUP_FRAG(x)   ((x) & DUP_FRAG_FLAG)


/* XXXdbg

   NOTE: this macro assumes that the page size of a tree is
         an even power of two.  beware if it is not!
*/	 
/* mask to get the index of a fragment in a dup page pointer */
#define WHICH_DUP_FRAG(b, x)  ((x) & (bt_pagesiz(b) - 1))

/* fixed maximum size of a dup fragment (empirically derived value) */
#define DUP_FRAG_SIZE   64

/* maximum number of entries in a dup fragment */
#define MAX_DUP_FRAGS  ((DUP_FRAG_SIZE - sizeof(bt_off_t)) / sizeof(bt_off_t))

/* maximum number of fragments per btree page */
#define MAX_FRAGS_PER_PAGE(b)  (bt_pagesiz(b) / DUP_FRAG_SIZE)

/* returns the value of a dup frag pointer */
#define DUP_FRAG_VAL(x)  ((x) & ~(CHAIN_FLAG|DUP_FRAG_FLAG|(bt_pagesiz(b)-1)))

/* returns the value of a dup chain pointer */
#define DUP_CHAIN_VAL(x)  ((x) & ~(CHAIN_FLAG))

/*
   a dup frag consists of the number of duplicates followed by
   the rrn values as an array.  the first word (a bt_off_t) is
   the number of duplicates in the fragment, then the bt_off_t's
   of the rrns.
*/   
#define NUM_DUPS_IN_FRAG(p, which) (*(bt_off_t *)(p + (DUP_FRAG_SIZE * which)))
#define FRAG_DUPS(p, which) ((bt_off_t *)(p + (DUP_FRAG_SIZE * which) + sizeof(bt_off_t)))


/*
   duplicate block chains have prev, next and num entries as the
   first three items.  those three items are followed by the rrn
   values for the duplicate entries.  the rrns are kept in sorted
   order (very important).
   
*/   
#define PREV_DUP(p)       (*(bt_off_t *)p)
#define NEXT_DUP(p)       (*(bt_off_t *)(p + 1*sizeof(bt_off_t)))
#define NUM_DUPS(p)       (*(bt_off_t *)(p + 2*sizeof(bt_off_t)))
#define DUPS(p)           ((bt_off_t *)(p + 3*sizeof(bt_off_t)))
#define DUP_USE(p)        (NUM_DUPS(p) * sizeof(bt_off_t) + 3*sizeof(bt_off_t))
#define MAX_DUPS(bt)      ((bt_pagesiz(bt) / sizeof(bt_off_t)) - 3)

/*
  This macro checks a child/keyval pointer to make sure that
  it's in range for the in-memory btree page.
  
  XXXdbg --  NOTE: this macro assumes that btree pages are 
                   always 1024 bytes in size!!!
*/  
#define CHECK_PTR(ptr, page, cleanup_label)                                   \
    do {                                                                      \
		if ((char *)(ptr) >= ((char *)(page) + 1024)) {                       \
			goto cleanup_label;                                               \
		}                                                                     \
	} while (0)

//		this belongs a couple of lines further up :)
//			printf("%s:%d: index pointer out of range! (%s 0x%x, max 0x%x)\n", \
//				   __FILE__, __LINE__, ##ptr, (ptr), ((page)+1024));          \

/* internal prototypes */
int       bt_wsuper(BT_INDEX *b);
int       bt_freepage(BT_INDEX *b, bt_off_t pag);
bt_off_t     bt_newpage(BT_INDEX *b);
int       bt_srchpg(bt_chrp k, int kl, int dtyp, FUNCP cmpf, bt_off_t *ptr, int *num,
		    bt_chrp s);
void      bt_inspg(bt_chrp key, int len, bt_off_t *ptr, int at, bt_chrp in,
		   bt_chrp out);
int       bt_delpg(int at, bt_chrp in, bt_chrp out);
int       bt_keyof(int n, bt_chrp p, bt_chrp dbuf, int *dlen, bt_off_t *dptr,
		   int max);
void       bt_splpg(bt_chrp key, int len, bt_off_t *ptr, int at, int siz,
		    bt_chrp old, bt_chrp lo, bt_chrp hi, bt_chrp new, int *nlen);
struct bt_cache *bt_rpage(BT_INDEX *b, bt_off_t num);
int              bt_wpage(BT_INDEX *b, struct bt_cache *p);
int       bt_seekdown(BT_INDEX *b, bt_chrp key, int len);
int       bt_getnextdup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn);
int       bt_getprevdup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn);
int       bt_find_dup(BT_INDEX *b, bt_off_t start_page, bt_off_t *retrrn);
int       bt_dupinsert(BT_INDEX *b, struct bt_cache *cp, int keyat,
					   bt_off_t new_rrn);
int       bt_dupdelete(BT_INDEX *b, struct bt_cache *cp, int keyat,
					   bt_off_t key_rrn, bt_off_t *rrn_to_del);
void      bt_die(const char *fmt, ...);


#define	_DEF_BT_INTERN_H
#endif
