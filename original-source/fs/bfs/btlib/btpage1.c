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
bt_srchpg(bt_chrp k, int kl, int dtyp, FUNCP cmpf, bt_off_t *ptr, int *num,
 		  bt_chrp s)
{
	int	lo = 0, m, hi;		/* low, mid, high ptrs for binsrch */
	short	*ip;			/* offset/beginning of key index */
	bt_off_t	*cl;			/* offset/beginning of child index */
	bt_chrp	cp;			/* offset/beginning of keys */
	REGISTER bt_off_t	cmpval;		/* returned value of comparison */
	REGISTER bt_chrp	kp1;	/* tmp key pointer */
	REGISTER bt_chrp	kp2;
	REGISTER int	l1;		/* key lengths */
	REGISTER int	l2;

	/* this value is decremented by one to keep from */
	/* looking past the top of the key list */
	hi = KEYCNT(s) - 1;
	if (hi > 1024) {
		printf("bt_srchpg: bad index! hi %d\n", hi);
		return BT_NF;
	}

	/* save effort if this is an empty page */
	/* also saves from binary search where hi < lo! */
	if(KEYCNT(s) == 0) {
		*num = 0;
		*ptr = HIPT(s);
		return(BT_NF);
	}

	/* beginning of key index */
	ip = (short *)KEYIND(s);
	CHECK_PTR(ip, s, cleanup);

	/* beginning of key string */
	cp = KEYADR(s);
	CHECK_PTR(cp, s, cleanup);

	/* beginning of the child pointers */
	cl = (bt_off_t *)KEYCLD(s);
	CHECK_PTR(cl, s, cleanup);

	/* hold onto your cerebellums, folks !! */
	while(lo <= hi) {

		/* if the first key is the one we want, its length */
		/* is not calculated using the subtracted offsets, */
		/* but is rather calculated by using the offset */
		/* to the beginning of key #2, which starts where */
		/* the first key ends, by definition */

		/* actual comparison is done over len bytes from */
		/* the offset that is extracted out of the index */

		/* set up pointers for the compare */
		kp1 = k;
		l1 = kl;
		if((m = (lo + hi) >> 1) == 0) {
			kp2 = cp;
			l2 = *ip;
		} else {
			CHECK_PTR((ip + m - 1), s, cleanup);
			kp2 = cp + *(ip + m - 1);
			CHECK_PTR(kp2, s, cleanup);
			l2 = (*(ip + m) - *(ip + (m - 1)));
		}

		/* do the compare based on data type */
		if(dtyp == BT_STRING) {
				while(l1 != 0 && l2 != 0) {
					if(*kp1 != *kp2) {
						cmpval = *kp1 - *kp2;
						goto endcmp;
					}
					kp1++;
					kp2++;
					l1--;
					l2--;
				}

				if (l1 == 0 && l2 == 1 && *kp2 == '\0')
					cmpval = 0;
				else if(l1 != l2)
					cmpval = l1 - l2;
				else
					cmpval = 0;
		} else if(dtyp == BT_INT) {
				cmpval = *(int *)kp1 - *(int *)kp2;
		} else if(dtyp == BT_UINT) {
				cmpval = *(unsigned int *)kp1 - *(int *)kp2;
		} else if(dtyp == BT_LONG_LONG) {
				cmpval = *(long long *)kp1 - *(long long *)kp2;
		} else if(dtyp == BT_ULONG_LONG) {
				cmpval = *(unsigned long long *)kp1 -
					     *(unsigned long long *)kp2;
		} else if(dtyp == BT_FLOAT) {
				cmpval = 0;
				 if(*(float *)kp1 > *(float *)kp2)
					cmpval = 1;
				else
					if(*(float *)kp1 < *(float *)kp2)
						cmpval = -1;
		} else if(dtyp == BT_DOUBLE) {
				cmpval = 0;
				 if(*(double *)kp1 > *(double *)kp2)
					cmpval = 1;
				else
					if(*(double *)kp1 < *(double *)kp2)
						cmpval = -1;
		} else if(dtyp == BT_USRDEF) {
			if(cmpf == 0)
				return(BT_ERR);
			cmpval = (*cmpf)(kp1,l1,kp2,l2);
		} else
			return(BT_ERR);
endcmp:
		if(cmpval < 0) {
			hi = m - 1;
			*num = m;
		} else {
			if(cmpval > 0) {
				lo = m + 1;
				*num = m + 1;
			} else {
				/* return current key # in num */
				*num = m;
				CHECK_PTR(cl + m, s, cleanup);
				*ptr = *(cl + m);
				return(BT_OK);
			}
		}
	}

	if(*num == KEYCNT(s)) {
		*ptr = HIPT(s);
	} else {
		CHECK_PTR((cl + *num), s, cleanup);
		*ptr = *(cl + *num);
	}

cleanup:
	return(BT_NF);
}


void
bt_inspg(bt_chrp key, int len, bt_off_t *ptr, int at, bt_chrp in, bt_chrp out)
{
	REGISTER short	*iin;		/* index/length ptr to old page */
	REGISTER short	*iout;		/* index/length ptr to new page */
	REGISTER bt_off_t	*lin;		/* child ptr to old page */
	REGISTER bt_off_t	*lout;		/* child ptr to new page */
	REGISTER bt_chrp	kin;		/* key ptr to old page */
	REGISTER bt_chrp	kout;		/* key ptr to new page */
	REGISTER int	t;		/* iterator */
	int	iky;			/* key number in old page */
	int	oky;			/* key number in new page */
	int	copied = 0;		/* used as flag AND shift of lengths */

	/* fix key count in new page */
	KEYCNT(out) = KEYCNT(in) + 1;

	/* fix key length in new page */
	KEYLEN(out) = KEYLEN(in) + len;

	/* left and right sibs */
	LSIB(out) = LSIB(in);
	RSIB(out) = RSIB(in);

	/* and high pointer */
	HIPT(out) = HIPT(in);

	/* set the key start pointers, index and child pointers */
	kin  = KEYADR(in);
	kout = KEYADR(out);

	iin  = (short *)KEYIND(in);
	iout = (short *)KEYIND(out);

	lin  = (bt_off_t *)KEYCLD(in);
	lout = (bt_off_t *)KEYCLD(out);

	for(oky = iky = 0; oky < KEYCNT(out); oky++) {

		CHECK_PTR(iin,  in,  cleanup);
		CHECK_PTR(iout, out, cleanup);
		CHECK_PTR(lin,  in,  cleanup);
		CHECK_PTR(lout, out, cleanup);

		/* insert the key at this point in the page */
		/* copied is used later to calculate lenght offsets */
		if(at == iky && copied == 0) {

			/* copy the key into the key space */
			for(t = 0; t < len; t++)
				*kout++ = *key++;

			/* fix up index lengths if key is first out */
			if(oky == 0)
				*iout = len;
			else
				*iout = *(iout - 1) + len;

			iout++;

			/* offset any more index lengths by this much */
			copied = len;

			/* copy ptrs */
			*lout++ = *ptr;

		} else {
			if(iky == 0)
				for(t = 0; t < *iin; t++)
					*kout++ = *kin++;
			else
				for(t = 0; t < (*iin - *(iin - 1)); t++)
					*kout++ = *kin++;
			iky++;
			*iout++ = *iin + copied;
			iin++;
			*lout++ = *lin++;
		}
	}
	lout--;
	return;


cleanup:    /* XXXdbg -- only come here from CHECK_PTR() !!! */
	bfs_die("bt_inspg: btree pages are hosed! (in 0x%x out 0x%x)\n", in, out);
}
