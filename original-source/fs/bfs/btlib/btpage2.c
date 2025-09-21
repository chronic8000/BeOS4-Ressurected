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
bt_delpg(int at, bt_chrp in, bt_chrp out)
{
	int	iky;			/* key iterator */
	REGISTER bt_chrp	icp;	/* old key pointer */
	REGISTER bt_chrp	ocp;	/* new key pointer */
	REGISTER short	*iin;		/* old index/length pointer */
	REGISTER short	*iout;		/* new index/length pointer */
	REGISTER bt_off_t	*lin;		/* old child ptr */
	REGISTER bt_off_t	*lout;		/* new child ptr */
	REGISTER int	t;		/* iterator */
	int	dropd = 0;		/* flag AND count of dropped bytes */
	int	shif = 0;		/* length index shift after drop */

	/* delete a key from a page. very similar to insert in page */
	/* except we do it in 2 steps, like a split because we are not */
	/* sure how long the key being dropped is until we get there. */
	/* since we don't know how long it is, we can't set up the */
	/* index and child ptrs, and have to do that in a second pass */

	/* fix key count in new page */
	KEYCNT(out) = KEYCNT(in) - 1;

	/* fix left and right sibs */
	LSIB(out) = LSIB(in);
	RSIB(out) = RSIB(in);

	/* fix high pointer (may change later) */
	HIPT(out) = HIPT(in);

	/* init various pointers */
	ocp = KEYADR(out);
	icp = KEYADR(in);
	iin = (short *)KEYIND(in);

	/* start copy/drop of keys */
	for(iky = 0; iky < KEYCNT(in); iky++) {
		if(at == iky) {
			if(iky == 0)
				for(t = 0; t < *iin; t++)
					icp++;
			else
				for(t = 0; t < (*iin - *(iin - 1)); t++)
					icp++;
			dropd = t;
		} else {
			if(iky == 0)
				for(t = 0; t < *iin; t++)
					*ocp++ = *icp++;
			else
				for(t = 0; t < (*iin - *(iin - 1)); t++)
					*ocp++ = *icp++;
		}
		iin++;
	}

	/* fix key count length new page */
	if(at == KEYCNT(in) && HIPT(in) != BT_NULL) {
		/* if we are dropping last key from an index page, */
		/* drop the last key (effectively) by decrementing */
		/* the length. kind of a kludgey way to do it... */
		KEYLEN(out) = KEYLEN(in) - t;
	} else {
		/* key length of the new page is length of old minus */
		/* the length of the key that we already dropped */
		KEYLEN(out) = KEYLEN(in) - dropd;
	}

	/* reset pointers in prep for ptr/index copy/drop */
	iin = (short *)KEYIND(in);
	iout = (short *)KEYIND(out);
	lin = (bt_off_t *)KEYCLD(in);
	lout = (bt_off_t *)KEYCLD(out);

	/* start copy/drop of ptrs */
	for(iky = 0; iky < KEYCNT(in); iky++) {
		if(at == iky) {
			shif = dropd;
		} else {
			if(iky == KEYCNT(in) - 1 && HIPT(in) != BT_NULL && at == KEYCNT(in)) {
				HIPT(out) = *lin;
			} else {
				*iout++ = *iin - shif;
				*lout++ = *lin;
			}
		}
		iin++;
		lin++;
	}

	return 0;
}

int
bt_keyof(int n, bt_chrp p, bt_chrp dbuf, int *dlen, bt_off_t *dptr, int max)
{

	/* clip the numbered key out of the page and return it in */
	/* kbuf. the key length and rrv are also returned as well */
	short	*ip;
	bt_chrp	 cp;

	if(KEYCNT(p) == 0) {
		*dlen = 0;
		*dptr = BT_NULL;
		return(BT_OK);
	}

	cp = KEYADR(p);
	ip = (short *)KEYIND(p);

	CHECK_PTR(cp, p, cleanup);
	CHECK_PTR(ip, p, cleanup);

	if(n == 0) {
		*dlen = *ip;
	} else {
		CHECK_PTR(ip + n, p, cleanup);
		*dlen = *(ip + n) - *(ip + (n - 1));
		cp += *(ip + (n - 1));
	}

	if(*dlen > max)
		return(BT_ERR);

	CHECK_PTR(KEYCLD(p) + n, p, cleanup);
	*dptr = *((bt_off_t *)KEYCLD(p) + n);
	(void)memcpy((char *)dbuf,(char *)cp,*dlen);

	return(BT_OK);

cleanup:
	return BT_ERR;
}


void
bt_splpg(bt_chrp key, int len, bt_off_t *ptr, int at, int siz, bt_chrp old,
 		 bt_chrp lo, bt_chrp hi, bt_chrp new, int *nlen)
{
	int	oky;			/* key iterator */
	int	iky;			/* key iterator */
	int	byt = 0;		/* key byte cnt for low page */
	REGISTER bt_chrp	icp;	/* old key pointer */
	REGISTER bt_chrp	ocp;	/* new key pointer */
	REGISTER short *iin;		/* old index/length pointer */
	REGISTER short *iout;		/* new index/length pointer */
	REGISTER bt_off_t	*lin;		/* old child ptr */
	REGISTER bt_off_t	*lout;		/* new child ptr */
	REGISTER int	t;		/* iterator */
	char	copied = 0;		/* flag */
	int	shif = 0;		/* length index shift after insert */
	int	dropbyt = 0;		/* bytes dropped in index split */
	int	splitat;		/* key # where we did the split */
	
	/* this is somewhat different from a simple page insert, since */
	/* we don't yet know HOW long each key block will be, and as a */
	/* result we can't copy the pointers until the keys have been */
	/* done. first we copy the keys, fix up the length and count */
	/* values, which allows us to correctly get the addresses of */
	/* the various pointer offsets, and then we copy the keys/ptrs */
	/* there are, as you would expect, tons of special cases and */
	/* so on, in this function. The MAIN special cases are: the */
	/* last key in the low page is the one that needs to be sent */
	/* up to the calling function for insertion in the parent page */
	/* after the split. This is done with a little kludgery stuck */
	/* in the logic where we switch pages during the key copy. The */
	/* second special case is when we are splitting an index page, */
	/* the low child ptr has to become "meaningful" so what needs */
	/* to happen is that the lowest key in the high page needs to */
	/* get dropped onto the floor, rather than copied. *THEN* the */
	/* pointers need to be adjusted during the pointer copy. Ick ! */

	/* set pointers to keys prior to copy. low key first. */
	icp = KEYADR(old);
	ocp = KEYADR(lo);


	/* set the child ptr pointer here - it is used to tell */
	/* if this is an index page when we do the key copy */
	lin = (bt_off_t *)KEYCLD(old);

	/* warning! used as a temp var to tell if we have switched pages. */
	KEYLEN(lo) = 0;

	/* set pointer to old index - output indices are unknown still */
	iin = (short *)KEYIND(old);

	/* copy one more key than was in the old page */
	for(iky = oky = 0; oky < KEYCNT(old) + 1; oky++) {

		/* if we are at insertion point, insert key here */
		if(at == iky && copied == 0) {
			for(t = 0; t < len; t++)
				*ocp++ = *key++;
			byt += len;
			copied++;
		} else {
			/* the usual nonsense with key #0 length */
			if(iky == 0) {
				for(t = 0; t < *iin; t++)
					*ocp++ = *icp++;
				byt += *iin;
			} else {
				for(t = 0; t < *iin - *(iin - 1); t++)
					*ocp++ = *icp++;
				byt += *iin - *(iin - 1);
			}
			iky++;
			iin++;
		}

		/* when the first page is full, start on the second */
		if(KEYLEN(lo) == 0 && (PTRUSE + byt + (oky * (sizeof(short) + sizeof(bt_off_t)))) > siz) {

			/* set length and count for low page */
			KEYLEN(lo) = byt;
			KEYCNT(lo) = oky + 1;

			/* remember the # of the key where we split */
			splitat = oky;

			/* set high pointer for low page */
			/* if this is an interior page, drop the */
			/* last key and turn the last ptr into */
			/* the high pointer (done below) */
			if(HIPT(old) != BT_NULL) {
				KEYCNT(lo) = oky;

				/* keep track of how many bytes we just */
				/* lost so we can fix key lengths later */
				dropbyt = t;

				/* adjust length of low key */
				KEYLEN(lo) = byt - dropbyt;
			}

			/* return the dropped or otherwise high key */
			/* in new, and set nlen, if applicable */
 			if(new != (bt_chrp)0) {
				bt_chrp	tmpp;

 				*nlen = t;
 				tmpp = new + (t - 1);
 				/* do the copy */
 				while(t--)
 					*tmpp-- = *(--ocp);
			}

			/* now set the out pointer to point to next page */
			ocp = KEYADR(hi);

		}
	}

	/* set key counts - if we split an index, we dropped one */
	if(HIPT(old) == BT_NULL) {
		KEYCNT(hi) = oky - KEYCNT(lo);
	} else {
		KEYCNT(hi) = (oky - KEYCNT(lo)) - 1;
	}

	/* set key lengths - if we split an index, adjust high value */
	KEYLEN(hi) = byt - KEYLEN(lo) - dropbyt;

	/* set the locations of the ptrs */
	iout = (short *)KEYIND(lo);
	iin  = (short *)KEYIND(old);
	lout = (bt_off_t *)KEYCLD(lo);

	/* copy ptrs and insert new ptr at specified location */
	copied = 0;

	for(iky = oky = 0; oky < KEYCNT(old) + 1; oky++) {

		if(at == iky && copied == 0) {
			copied++;
			if(oky == splitat && HIPT(old) != BT_NULL) {
				HIPT(lo) = *ptr;

				/* since we did this, dropped bytes */
				/* dont count. ignore them */
				dropbyt = 0; 
			} else {
				*lout = *ptr;

				/* set up length/index ptrs */
				/* do not do this if the ptr was */
				/* inserted at a dropped location */
				if(iky == 0 || oky == splitat + 1)
					*iout = len;
				else 
					*iout = *(iout - 1) + len;

				/* length values now shift due to insert */
				shif += len;
			}
		} else {
			if(oky == splitat && HIPT(old) != BT_NULL) {
				/* normal ptr insert at boundary */
				HIPT(lo) = *lin++;
				iin++;
			} else {
				*lout = *lin++;

				*iout = *iin + shif;
				iin++;
			}

			iky++;
		}
		iout++;
		lout++;

		if(oky == splitat) {

			lout = (bt_off_t *)KEYCLD(hi);
			iout = (short *)KEYIND(hi);

			/* index/length values now shift due to split */
			shif -= (KEYLEN(lo) + dropbyt);

			if(HIPT(old) == BT_NULL)
				HIPT(lo) = BT_NULL;
		}

	}

	/* the high pointer of the high page will be that of the */
	/* original page. the low pages high pointer should have */
	/* already been fixed up properly somewhere in code above */
	HIPT(hi) = HIPT(old);
}
