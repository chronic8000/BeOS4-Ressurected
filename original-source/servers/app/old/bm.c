/*---------------------------------------------------------------*/
/*

	block_move(char *from,char *to,long count)


	Will move <count> bytes from <from>, to <to> with good
	use of the bus bandw.
	Will work if source and destinations overlap.

	01/04/92	Original version	Benoit

*/
/*---------------------------------------------------------------*/
	


#include <stdio.h>
#include <stdlib.h>
/*---------------------------------------------------------------*/

typedef union {
	char	as_char[4];
	short	as_short[2];
	long	as_long;
	}	trick;

/*---------------------------------------------------------------*/

#define	ulong	unsigned long

void	dqm16(char *, char *, long);

/*---------------------------------------------------------------*/

static void	blockmove0(from, to, count)
char		*from;
char		*to;
long		count;
{
	trick	tmp;

/* case of same alignment of blocks on 16 multiple address*/

	if (((long)from & 0x0f) == ((long)to & 0x0f)) {
		while ((count > 0) && ((long)from & 0x0f)) { 
			*to++ = *from++;
			count--;
		}

		dqm16(from, to, count);

		from += (count & 0xfffffff0);
		to  +=(count & 0xfffffff0);

		count &= 0x0f;

		while (count > 0) {
			*to++ = *from++;
			count--;
		}
		return;
	}

/* case of same alignment of blocks on 4 multiple address */
	
	if (((long)from & 3) == ((long)to & 3)) {
		while ((count > 0) && ((long)from & 3)) {
			*to++ = *from++;
			count--;
		}

		while (count > 4) {
			*((long *)to)++ = *((long *)from)++;
			count -= 4;
		}

		while (count-- > 0)
			*to++ = *from++;
		return;
	}


/* case of alignment by two				  */
/* get align on a long, then fetch long and write 2 short */

	if (((long) from & 1) == ((long)to & 1)) {
		while ((count > 0) && ((long)from & 3)) {
			*to++ = *from++;
			count--;
		}

		while (count > 4) {
			tmp.as_long = *((long *)from)++;
			count -= 4;
			*((short *)to)++ = tmp.as_short[0];
			*((short *)to)++ = tmp.as_short[1];
		}
		while (count-- > 0)
			*to++ = *from++;
		return;
	}

/*
   other cases are processed by getting the source aligned,
   fetching one long at a time and writting it byte per byte
   in the destination by extracting bytes out of it.
*/

	while ((count > 0) && ((long)from & 3)) {
		*to++ = *from++;
		count--;
	}

	while (count > 4) {
		tmp.as_long = *((long *)from)++;
		count -= 4;
		*to++ = tmp.as_char[0];
		*to++ = tmp.as_char[1];
		*to++ = tmp.as_char[2];
		*to++ = tmp.as_char[3];
	}

	while (count-- > 0)
		*to++ = *from++;	
}

/*---------------------------------------------------------------*/

static	void	blockmove1(from, to, count)
char		*from;
char		*to;
long		count;
{
	trick	tmp;

	from=(char *)((ulong)from + count - 1);
	to  =(char *)((ulong)to   + count - 1);


/* case of same alignment of blocks on 4 multiple address */
	
	if (((long)from & 3) == ((long)to & 3)) {
		while ((count > 0) && ((long)from & 3) != 3) {
			*to-- = *from--;
			count--;
		}

		from -= 3;
		to   -= 3;

		while (count > 4) {
			*((long *)to)-- = *((long *)from)--;
			count -= 4;
		}

		from += 3;
		to   += 3;

		while (count-- > 0)
			*to-- = *from--;
		return;
	}


/* case of alignment by two				  */

	if (((long) from & 1) == ((long)to & 1)) {
		while ((count > 0) && ((long)from & 3) != 3) {
			*to-- = *from--;
			count--;
		}

		from -= 3;
		to   -= 3;

		while (count > 4) {
			tmp.as_long = *((long *)from)--;
			count -= 4;
			*((short *)to)++ = tmp.as_short[0];
			*((short *)to)++ = tmp.as_short[1];
			to -= 8;
		}
		from += 3;
		to   += 3;

		while (count-- > 0)
			*to-- = *from--;
		
		return;
	}

/*
   other cases are processed by getting the source aligned,
   fetching one long at a time and writting it byte per byte
   in the destination by extracting bytes out of it.
*/


	while ((count > 0) && ((long)from & 3) != 3) {
		*to-- = *from--;
		count--;
	}


	from -= 3;
	to   -= 3;

	while (count > 4) {
		tmp.as_long = *((long *)from)--;
		count -= 4;
		*to++ = tmp.as_char[0];
		*to++ = tmp.as_char[1];
		*to++ = tmp.as_char[2];
		*to++ = tmp.as_char[3];
		to -= 8;
	}

	from += 3;
	to   += 3;

	while (count-- > 0)
		*to-- = *from--;	
}


/*---------------------------------------------------------------*/

void	block_move(from, to, count)
char	*from;
char	*to;
long	count;

{
	if (((ulong)to > (ulong)from)	 &&
	   ((ulong)to < ((ulong)from + count)))

		blockmove1(from, to, count);
	else
		blockmove0(from, to, count);
}

/*---------------------------------------------------------------*/

