/* ++++++++++
	FILE:	font_draw_8.c
	REVS:	$Revision: 1.4 $
	NAME:	pierre
	DATE:	Sat Mar  1 10:03:07 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */


#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "accelPackage.h"
#include "renderdefs.h"
#include "font_draw.h"

/* copy a black&white char into an 8 bits port */
void fc_bw_char8(uchar *dst_base, int dst_row, rect dst,
				 uchar *src_base, rect src, uint colors) {
	int        i, j, cmd, hmin, hmax, imin, imax, imed;
	uchar      *dst_ptr;

	dst_ptr = dst_base+dst_row*dst.top+(dst.left-src.left);
	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (true) {
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
			/* draw the visible part */
			if ((imax-imin) >= 7) {
				imed = (imin+3) & 0xfffffffcL;
				for (i=imin; i<imed; i++)
					dst_ptr[i] = (uchar)colors;
				imin = imax & 0xfffffffcL;
				for (; i<imin; i+=4)
					*((uint32*)(dst_ptr+i)) = colors;
			}
			for (i=imin; i<imax; i++)
				dst_ptr[i] = (uchar)colors;
			hmin = hmax;
		}
	next_line:
		dst_ptr += dst_row;
	}
}

/* draw a black&white char into an 8 bits port, all other cases */
void fc_bw_char8other(uchar *dst_base, int dst_row, rect dst,
					  uchar *src_base, rect src,
					  int mode, ColorMap *cs, uint colors, int energy) {
	int        i, j, cmd, hmin, hmax, imin, imax, index;
	uint       color, color2;
	uchar      *dst_ptr;
	uchar      *inverted;
	rgb_color  *color_list, *col;

	/* preload port parameters */
	inverted = cs->inverted;
	color_list = cs->color_list;
	dst_ptr = dst_base+dst_row*dst.top+(dst.left-src.left);
	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (true) {
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
			/* process the visible part */
			for (i=imin; i<imax; i++) {
				color = colors;
				col = color_list+dst_ptr[i];
				switch (mode) {
				case OP_ADD:
					color &= 0x3fcff3fcL;
					color2 = (col->red<<22) | (col->green<<12) | (col->blue<<2);
					color = color+color2;
					color2 = (color & 0x40100400L);
					color2 -= (color2>>8);
					color |= color2;
					break;
				case OP_SUB:
					color &= 0x3fcff3fcL;
					color2 = (col->red<<22) | (col->green<<12) | (col->blue<<2);
					color ^= 0x3fcff3fcL;
					color = color+color2-0x00401004L;
					color2 = (color & 0x40100400L);
					color2 -= (color2>>8);
					color &= color2;
					break;
				case OP_BLEND:
					color &= 0x3fcff3fcL;
					color2 = (col->red<<22) | (col->green<<12) | (col->blue<<2);
					color = (color+color2)>>1;
					break;
				case OP_MIN:
					if (col->red + col->green + col->blue <= energy)
						goto next_pixel;
					break;
				case OP_MAX:
					if (col->red + col->green + col->blue >= energy)
						goto next_pixel;
					break;
				case OP_INVERT:
					index = ((col->red<<7) | (col->green<<2) | (col->blue>>3)) ^ 0x7fff;
					goto put_pixel;
				}
				index = ((color>>15) & (0xf8 << 7)) |
					    ((color>>10) & (0xf8 << 2)) |
						((color>>5)  & (0xf8 >> 3));
				put_pixel:
				dst_ptr[i] = inverted[index];
				next_pixel:;
			}
			hmin = hmax;
		}
	next_line:
		dst_ptr += dst_row;
	}
}

/* copy a grayscale char into an 8 bits port, without char overlaping */
void fc_gray_char8(uchar *dst_base, int dst_row, rect dst,
				   uchar *src_base, int src_row, rect src, uint32 *colors) {
	int        i, j, gray2, gray;
	uchar      *src_ptr, *dst_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr = dst_base+dst_row*dst.top+dst.left-src.left;
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
			i++;
		}
		while (i<=src.right-1) {
			gray2 = src_ptr[i>>1];
			gray = gray2>>4;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
			gray = gray2&7;
			if (gray > 0)
				dst_ptr[i+1] = colors[gray];
			i+=2;
		}
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
		}
		src_ptr += src_row;
		dst_ptr += dst_row;
	}
}

/* copy a grayscale char into an 8 bits port, with character overlaping */
void fc_gray_char8x(uchar *dst_base, int dst_row, rect dst,
					uchar *src2_base, int src2_row, rect src2,
					uchar *src_base, int src_row, rect src, uint32 *colors) {
	int        i, i2, j, gray, gray2;
	uchar      *src_ptr, *dst_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr = dst_base+dst_row*dst.top+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		i2 = src2.left;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
			i++;
			i2++;
		}
		while (i<=src.right-1) {
			gray2 = src_ptr[i>>1];
			gray = gray2>>4;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
			i2++;
			gray = gray2&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0)
				dst_ptr[i+1] = colors[gray];
			i+=2;
			i2++;
		}
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0)
				dst_ptr[i] = colors[gray];
		}
		src_ptr += src_row;
		src2_ptr += src2_row;
		dst_ptr += dst_row;
	}
}

/* draw a grayscale char into an 8 bits port, all other cases */
void fc_gray_char8other(uchar *dst_base, int dst_row, rect dst,
						uchar *src_base, int src_row, rect src,
						int16 mode, ColorMap *cs, uint32 *colors) {
	int        i, j, index, gray, energy;
	uchar      *src_ptr, *dst_ptr;
	uchar      *inverted;
	ulong      color, color2;
	rgb_color  *color_list, *col;
	energy = colors[9];

	/* preload port parameters */
	inverted = cs->inverted;
	color_list = cs->color_list;
	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = dst_base+dst_row*dst.top+dst.left-src.left;
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;
			if (gray != 0) {
				color = colors[gray];
				col = color_list+dst_ptr[i];
				switch (mode) {
				case OP_MAX:
					if (col->red+col->green+col->blue >= energy)
						goto next_pixel;
					goto or_case;
				case OP_MIN:
					if (col->red+col->green+col->blue <= energy)
						goto next_pixel;
				case OP_OVER:
				case OP_SELECT:
				case OP_ERASE:
				or_case:
					gray += (gray+1)>>3;
					color2 = ((col->red<<19)  & (0x7f<<20)) |
						     ((col->green<<9) & (0x7f<<10)) |
							  (col->blue>>1);
					color += color2*(8-gray);
					break;
				case OP_ADD:
					color &= 0x3fcff3fcL;
					color2 = (col->red<<22) | (col->green<<12) | (col->blue<<2);
					color = color+color2;
					color2 = (color & 0x40100400L);
					color2 -= (color2>>8);
					color |= color2;
					break;
				case OP_SUB:
					color &= 0x3fcff3fcL;
					color2 = (col->red<<22) | (col->green<<12) | (col->blue<<2);
					color ^= 0x3fcff3fcL;
					color = color+color2-0x00401004L;
					color2 = (color & 0x40100400L);
					color2 -= (color2>>8);
					color &= color2;
					break;
				case OP_BLEND:
					gray += (gray+1)>>3;
					color2 = ((col->red<<19)  & (0x7f<<20)) |
						     ((col->green<<9) & (0x7f<<10)) |
							  (col->blue>>1);
					color += color2*(8-gray);
					color &= 0x3fcff3fcL;
					color = (color>>1)+(color2<<2);
					break;
				case OP_INVERT:
					gray += (gray+1)>>3;
					color2 = ((col->red<<19)  & (0x7f<<20)) |
						     ((col->green<<9) & (0x7f<<10)) |
							  (col->blue>>1);
					color2 <<= 3;
					color = (0x1fc7f1fc-color2)>>2;
					color = color2 + color*gray;
					break;
				}
				index = ((color>>15) & (0xf8 << 7)) |
					    ((color>>10) & (0xf8 << 2)) |
						((color>>5)  & (0xf8 >> 3));
				dst_ptr[i] = inverted[index];
			}
		next_pixel:;
		}
		src_ptr += src_row;
		dst_ptr += dst_row;
	}
}

void fc_gray_char8over(uchar *dst_base, int dst_row, rect dst,
						uchar *src_base, int src_row, rect src,
						int16 /*mode*/, ColorMap *cs, uint32 *colors) {
	int        i, j, index, gray;
	uchar      *src_ptr, *dst_ptr;
	uchar      *inverted;
	ulong      color, color2;
	rgb_color  *color_list, *col;

	/* preload port parameters */
	inverted = cs->inverted;
	color_list = cs->color_list;
	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = dst_base+dst_row*dst.top+dst.left-src.left;
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;
			if (gray == 7) {
				dst_ptr[i] = colors[8];
				continue;
			};

			if (gray == 0) continue;
			
			col = color_list+dst_ptr[i];
			color2 = ((col->red<<19)  & (0x7f<<20)) |
				     ((col->green<<9) & (0x7f<<10)) |
					  (col->blue>>1);
			color = colors[gray] + color2*(8-(gray+((gray+1)>>3)));
			index = ((color>>15) & (0xf8 << 7)) |
				    ((color>>10) & (0xf8 << 2)) |
					((color>>5)  & (0xf8 >> 3));
			dst_ptr[i] = inverted[index];
		}
		src_ptr += src_row;
		dst_ptr += dst_row;
	}
}

