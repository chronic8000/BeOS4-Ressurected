#include	<stdio.h>
#include    <stdarg.h>
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


#ifdef	YES_BT_DEBUG

void
bt_die(const char *fmt, ...)
{
  va_list     ap;
  static char buff[512];

  printf("\n");
  va_start(ap, fmt);
  vsprintf(buff, fmt, ap);
  va_end(ap);

  printf("%s\n", buff);
  
  printf("spinning forever.\n");
  while(1)
	  ;
}


void
bt_dump(BT_INDEX *b, struct bt_cache *p)
{
	int	x;
	int	rl;
	bt_off_t	rv;
#if 0							/* If there is a problem here in
								 * INTEL turn this #ifdef on.
								 */
	char 	buf[512];
#else	
	char	buf[BT_DFLTPSIZ];
#endif	

	if(KEYUSE(p->p) > bt_pagesiz(b))
		(void)printf("WARNING overfull page!!!\n");


	(void)printf("Dump:page=%Ld, cnt=%d, len=%d, ",
		p->num,KEYCNT(p->p),KEYLEN(p->p));

	(void)printf("lsib=%Ld, rsib=%Ld, high=%Ld, used=%d\n",
		LSIB(p->p),RSIB(p->p),HIPT(p->p),KEYUSE(p->p));

	if(BT_MAXK(b) > sizeof(buf)) {
		(void)printf("page size too big to dump keys (sorry)\n");
		return;
	}

	/* print keys/len/chld index */
	for(x = 0; x < KEYCNT(p->p); x++) {
		if(bt_keyof(x,p->p,(bt_chrp)buf,&rl,&rv,(int)sizeof(buf)) == BT_ERR) {
			(void)printf("cannot access key #%d\n",x);
			return;
		}

		switch(bt_dtype(b)) {
			case	BT_STRING:
				buf[rl] = '\0';
				(void)printf("key=\"%s\",len=%d,rrv=%Ld",buf,rl,rv);
				break;

			case	BT_INT:
				(void)printf("key=%d,len=%d,rrv=%Ld",*(int *)buf,rl,rv);
				break;

			case	BT_UINT:
				(void)printf("key=%u,len=%d,rrv=%Ld",*(unsigned int *)buf,rl,rv);
				break;

			case	BT_LONG_LONG:
				(void)printf("key=%Ld,len=%d,rrv=%Ld",*(long long *)buf,rl,rv);
				break;

			case	BT_ULONG_LONG:
				(void)printf("key=%Lu,len=%d,rrv=%Ld",
							 *(unsigned long long *)buf,rl,rv);
				break;

			case	BT_FLOAT:
				(void)printf("key=%f,len=%d,rrv=%Ld",*(float *)buf,rl,rv);
				break;

			case	BT_DOUBLE:
				(void)printf("key=%f,len=%d,rrv=%Ld",*(double *)buf,rl,rv);
				break;

			case	BT_USRDEF:
				(void)printf("key=<usrdef>,len=%d,rrv=%Ld",rl,rv);
				break;
		}

		if(x % 4 == 3)
			(void)printf("\n");
		else
			if(x != KEYCNT(p->p) - 1)
				(void)printf(", ");
	}
	(void)printf("\n");
}


void
bt_dup_dump(BT_INDEX *b, struct bt_cache *p)
{
	int	x;

	(void)printf("Dump:page=%Ld, cnt=%d, max=%d, ",
		p->num, (int)NUM_DUPS(p->p), (int)MAX_DUPS(b));

	(void)printf("prev=%Ld, next=%Ld\n", PREV_DUP(p->p), NEXT_DUP(p->p));

	/* print keys/len/chld index */
	for(x = 0; x < NUM_DUPS(p->p); x++) {
		printf("%Ld\n", DUPS(p->p)[x]);
	}
}
#endif
