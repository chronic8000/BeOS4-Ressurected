/* ++++++++++
	FILE:	font_draw_1.c
	REVS:	$Revision$
	NAME:	pierre
	DATE:	Sat Mar  1 10:03:07 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _FONT_DRAW_H
#include "font_draw.h"
#endif

/* main entry to draw black&white string into an 1 bit port */
void fc_draw_string1(port      *a_port,
					 fc_string *str,
					 rect      clipbox,
					 int       mode) {
	int          i;
	int          cur_row;
	rect         cur, src;
	uchar        *src_base, *cur_base;
	fc_char      *my_char;
	fc_point     *my_offset;

	if (str->bits_per_pixel != FC_BLACK_AND_WHITE)
		return;
	/* preload port parameters */
	cur_base = a_port->base;
	cur_row = a_port->rowbyte;
	/* mode reduction */
	if ((mode != op_and) && (mode != op_xor))
		mode = op_or;
	/* go through all the characters */
	for (i=0; i<str->char_count; i++) {
		my_char = str->chars[i];
		my_offset = str->offsets+i;
		if ((my_offset->x < -100000) ||
			(my_offset->x > +100000) ||
			(my_offset->y < -100000) ||
			(my_offset->y > +100000))
			continue;
		/* calculate character horizontal limits */
		cur.left = my_char->bbox.left + my_offset->x;
		cur.right = my_char->bbox.right + my_offset->x;
		src.left = 0;
		src.right = cur.right-cur.left;
		if (src.right < 0)
			continue;
		/* cliping with horizontal external box */
		if (clipbox.left > cur.left) {
			if (clipbox.left > cur.right)
				continue;
			src.left += clipbox.left-cur.left;
			cur.left = clipbox.left;
		}
		if (clipbox.right < cur.right) {
			if (clipbox.right < cur.left)
				continue;
			src.right += clipbox.right-cur.right;
			cur.right = clipbox.right;
		}
		/* calculate character vertical limits */
		cur.top = my_char->bbox.top + my_offset->y;
		cur.bottom = my_char->bbox.bottom + my_offset->y;
		src.top = 0;
		src.bottom = cur.bottom-cur.top;
		/* cliping with vertical external box */
		if (clipbox.top > cur.top) {
			if (clipbox.top > cur.bottom)
				continue;
			src.top += clipbox.top-cur.top;
			cur.top = clipbox.top;
		}
		if (clipbox.bottom < cur.bottom) {
			if (clipbox.bottom < cur.top)
				continue;
			src.bottom += clipbox.bottom-cur.bottom;
			cur.bottom = clipbox.bottom;
		}
		/* complete description before drawing */
		src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
		fc_bw_char1(cur_base, cur_row, cur, src_base, src, mode);
	}		
}

/* copy a black&white char into an 1 bit port */
void fc_bw_char1(uchar *dst_base, int dst_row, rect dst,
				 uchar *src_base, rect src, int mode) {
	int        i, j, cmd, hmin, hmax, imin, imax, kmin, kmax, offset;
	uchar      mask, mask2;
	uchar      *dst_ptr;

	dst_ptr = dst_base+dst_row*dst.top;
	offset = dst.left-src.left;
	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (TRUE) {
			/* decode next run */
			do {
				cmd = *src_base++;
				/* end of run list for this line */
				if (cmd == 0xff)
					goto next_line;
				hmin += cmd;
			} while (cmd == 0xfe);
			hmax = hmin;
			do {
				cmd = *src_base++;
				hmax += cmd;
			} while (cmd == 0xfe);
			/* horizontal clipping */
			if (hmin > src.right) {
				do src_base++; while (src_base[-1] != 0xff);
				goto next_line;
			}
			if (hmin >= src.left)
				imin = hmin;
			else
				imin = src.left;
			if (hmax <= src.right)
				imax = hmax;
			else
				imax = src.right+1;
			imin += offset;
			imax += offset;
			/* draw the visible part */
			if (imin < imax) {
				kmin = imin>>3;
				if ((imax & 0xfffffff8) <= imin) {
					mask = (0x100 >> (imin & 7)) - (0x100 >> (imax & 7));
					if (mode == op_or)
						dst_ptr[kmin] |= mask;
					else if (mode == op_xor)
						dst_ptr[kmin] ^= mask;
					else
						dst_ptr[kmin] &= ~mask;
				}
				else {
					kmax = imax>>3;
					mask = (0x100 >> (imin & 7)) - 1;
					mask2 = 0x100 - (0x100 >> (imax&7));
					if (mode == op_or) {
						dst_ptr[kmin] |= mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] = 0xff;
						dst_ptr[kmax] |= mask2;
					}
					else if (mode == op_xor) {
						dst_ptr[kmin] ^= mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] ^= 0xff;
						dst_ptr[kmax] ^= mask2;
					}
					else {
						dst_ptr[kmin] &= ~mask;
						for (i=kmin+1; i<kmax; i++)
							dst_ptr[i] = 0x00;
						dst_ptr[kmax] &= ~mask2;
					}
				}
			}
			hmin = hmax;
		}
	next_line:
		dst_ptr += dst_row;
	}
}



