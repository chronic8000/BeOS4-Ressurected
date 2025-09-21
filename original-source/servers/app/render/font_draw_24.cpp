/* ++++++++++
	FILE:	font_draw_24.c
	REVS:	$Revision: 1.5 $
	NAME:	pierre
	DATE:	Sat Mar  1 10:03:07 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderLock.h"
#include "font_draw.h"

typedef void (*DrawChar)(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src,
							int16 mode, uint32 *colors, uint32 *b_colors);

/* copy a black&white char into an 32 bits port, endianess independant */
void fc_bw_char32(uchar *dst_base, int dst_row, rect dst,
				  uchar *src_base, rect src, uint colors) {
	int        i, j, cmd, hmin, hmax, imin, imax;
	uint       *dst_ptr;

	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
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
			/* draw the visible part */
			for (i=imin; i<imax; i++)
				dst_ptr[i] = colors;
			hmin = hmax;
		}
	next_line:
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

/* draw a black&white char into an 32 bits port, all other cases, endianess independant */
void fc_bw_char32other(uchar *dst_base, int dst_row, rect dst,
					   uchar *src_base, rect src,
					   int mode, uint colors, uint b_colors, int energy) {
	int        i, j, cmd, hmin, hmax, imin, imax;
	uint       color, color2, b_color, b_color2, color_in, color_out;
	uint       *dst_ptr;

	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
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
			/* process the visible part */
			for (i=imin; i<imax; i++) {
				color = colors;
				b_color = b_colors;
				color_in = dst_ptr[i];
				switch (mode) {
				case OP_ADD:
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color += color2;
					b_color += b_color2;
					color2 = color & 0x800800L;
					b_color2 = b_color & 0x800800L;
					color2 -= color2>>8;
					b_color2 -= b_color2>>8;
					color |= color2;
					b_color |= b_color2;
					break;
				case OP_SUB:
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color ^= 0x7f87f8;
					b_color ^= 0x7f87f8;
					color += color2-0x8008L;
					b_color += b_color2-0x8008L;
					color2 = color & 0x800800L;
					b_color2 = b_color & 0x800800L;
					color2 -= color2>>8;
					b_color2 -= b_color2>>8;
					color &= color2;
					b_color &= b_color2;
					break;
				case OP_BLEND:
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color = (color+color2)>>1;
					b_color = (b_color+b_color2)>>1;
					break;
				case OP_INVERT:
					color_out = 0xffffffff^color_in;
					goto put_pixel;
				case OP_MIN:
					if (((color_in>>24)&0xff)+((color_in>>16)&0xff)+
						((color_in>>8)&0xff)+(color_in&0xff) <= energy)
						goto next_pixel;
					break;
				case OP_MAX:
					if (((color_in>>24)&0xff)+((color_in>>16)&0xff)+
						((color_in>>8)&0xff)+(color_in&0xff) >= energy)
						goto next_pixel;
					break;
				}
				color_out = ((color<<9  )&0xff000000) |
					        ((color<<13 )&0x00ff0000) |
							((b_color>>7)&0x0000ff00) |
							((b_color>>3)&0x000000ff);
				put_pixel:
				dst_ptr[i] = color_out;
				next_pixel:;
			}
			hmin = hmax;
		}
	next_line:
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

/* copy a grayscale char into an 32 bits port, without char overlaping, both endianess */
static void CopyCharAA32(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src, uint32 *colors) {
	int        i, j, gray2, gray;
	uint       *dst_ptr;
	uchar      *src_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
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
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

/* copy a grayscale char into an 32 bits port, with character overlaping, both endianess */
static void CopyCharAA32X(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src2_base, int32 src2_row, rect src2,
							uint8 *src_base, int32 src_row, rect src, uint32 *colors) {
	int        i, i2, j, gray, gray2;
	uint       *dst_ptr;
	uchar      *src_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
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
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

/* draw a grayscale char into an 32 bits port, all other cases, both endianess */
void OtherCharAA32(	uint8 *dst_base, int32 dst_row, rect dst,
					 uint8 *src_base, int32 src_row, rect src,
					 int16 mode, uint32 *colors, uint32 *b_colors)
{
	int        i, j, gray, blend;
	uint       color, b_color, color2, b_color2, color_in, color_out;
	uint       *dst_ptr;
	uchar      *src_ptr;

	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;
			if (gray != 0) {
				color = colors[gray];
				b_color = b_colors[gray];
				color_in = dst_ptr[i];
				switch (mode) {
				case OP_MIN:
					if (((color_in>>24)&0xff)+((color_in>>16)&0xff)+
						((color_in>>8)&0xff)+(color_in&0xff) <= colors[9])
						goto next_pixel;
					goto or_case;
				case OP_MAX:
					if (((color_in>>24)&0xff)+((color_in>>16)&0xff)+
						((color_in>>8)&0xff)+(color_in&0xff) >= colors[9])
						goto next_pixel;
				case OP_OVER:
				case OP_SELECT:
				case OP_ERASE:
				or_case:
					gray += (gray+1)>>3;
					color2 = ((color_in>>12)&0xff000) | ((color_in>>16)&0xff);
					b_color2 = ((color_in<<4)&0xff000) | (color_in&0xff);
					blend = 8-gray;
					color += color2*blend;
					b_color += b_color2*blend;
					break;
				case OP_ADD:
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color += color2;
					b_color += b_color2;
					color2 = color & 0x800800L;
					b_color2 = b_color & 0x800800L;
					color2 -= color2>>8;
					b_color2 -= b_color2>>8;
					color |= color2;
					b_color |= b_color2;
					break;
				case OP_SUB:
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color ^= 0x7f87f8;
					b_color ^= 0x7f87f8;
					color += color2-0x8008L;
					b_color += b_color2-0x8008L;
					color2 = color & 0x800800L;
					b_color2 = b_color & 0x800800L;
					color2 -= color2>>8;
					b_color2 -= b_color2>>8;
					color &= color2;
					b_color &= b_color2;
					break;
				case OP_BLEND:
					gray += (gray+1)>>3;
					color2 = ((color_in>>12)&0xff000) | ((color_in>>16)&0xff);
					b_color2 = ((color_in<<4)&0xff000) | (color_in&0xff);
					blend = 8-gray;
					color += color2*blend;
					b_color += b_color2*blend;
					color = (color>>1)+(color2<<2);
					b_color = (b_color>>1)+(b_color2<<2);
					break;
				case OP_INVERT:
					gray += (gray+1)>>3;
					color2 = ((color_in>>9)&0x7f8000) | ((color_in>>13)&0x7f8);
					b_color2 = ((color_in<<7)&0x7f8000) | ((color_in<<3)&0x7f8);
					color = (0x3fc3fc - color2)>>2;
					b_color = (0x3fc3fc - b_color2)>>2;
					color = color2 + color*gray;
					b_color = b_color2 + b_color*gray;
					break;
				}
				color_out = ((color<<9  )&0xff000000) |
					        ((color<<13 )&0x00ff0000) |
							((b_color>>7)&0x0000ff00) |
							((b_color>>3)&0x000000ff);
				dst_ptr[i] = color_out;
			}
		next_pixel:;
		}
		src_ptr += src_row;
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

void OverCharAA32(	uint8 *dst_base, int32 dst_row, rect dst,
					uint8 *src_base, int32 src_row, rect src,
					int16 mode, uint32 *colors, uint32 *b_colors)
{
	int        i, j, gray;
	uint       color, b_color, color2, b_color2, color_in, color_out;
	uint       *dst_ptr;
	uchar      *src_ptr;

	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;

			if (gray == 7) {
				dst_ptr[i] = colors[8];
				continue;
			};

			if (gray == 0) continue;
			
			color_in = dst_ptr[i];
			color2 = ((color_in>>12)&0xff000) | ((color_in>>16)&0xff);
			b_color2 = ((color_in<<4)&0xff000) | (color_in&0xff);
			color = colors[gray];
			b_color = b_colors[gray];
			gray = 8-(gray+((gray+1)>>3));
			color += color2*gray;
			b_color += b_color2*gray;
			color_out = ((color<<9  )&0xff000000) |
				        ((color<<13 )&0x00ff0000) |
						((b_color>>7)&0x0000ff00) |
						((b_color>>3)&0x000000ff);
			dst_ptr[i] = color_out;
		}
		src_ptr += src_row;
		dst_ptr = (uint*)((uchar*)dst_ptr+dst_row);
	}
}

static void DrawStringNonAA(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	int         i, index, energy0 = cache->fontColors[3];
	int         cur_row;
	int         mode_direct=cache->fontColors[2];
	rect        cur, src;
	uint        colors=cache->fontColors[0], b_colors=cache->fontColors[1];
	uchar       *src_base, *cur_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rect		clipbox;
	int16		mode = context->drawOp;
	int32		clipIndex=0;

	grAssertLocked(context, port);
	
	/* preload port parameters */
	const int32 clipCount = port->RealDrawingRegion()->CountRects();
	const rect* clipRects = port->RealDrawingRegion()->Rects();
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;

	if ((clipIndex=find_span_between(port->RealDrawingRegion(),str->bbox.top,str->bbox.bottom))==-1) return;
	while ((clipIndex>0) && (clipRects[clipIndex].top > str->bbox.top)) clipIndex--;
	while ((clipIndex>0) && (clipRects[clipIndex].top == clipRects[clipIndex-1].top)) clipIndex--;

	for (;clipIndex<clipCount;clipIndex++) {
		clipbox = clipRects[clipIndex];
		if (clipbox.top > str->bbox.bottom) return;
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
			if (src.right < 0) continue;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			if (mode_direct)
				fc_bw_char32(cur_base, cur_row, cur, src_base, src, colors);
			else
				fc_bw_char32other(cur_base, cur_row, cur, src_base, src,
								  mode, colors, b_colors, energy0);
		}
	}
}

static void DrawStringNonAAPreprocess(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	int32		a, r, g, b, index;
	rgb_color	fc;
	int32		mode,endianess;
	uint32 *	colors = port->cache->fontColors;
	RenderCache *cache = port->cache;
	
	grAssertLocked(context, port);
	
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;

	switch (mode) {
		case OP_ERASE:
			fc = context->backColor;
			goto record_mono_color;
		case OP_COPY:
		case OP_OVER:
		case OP_INVERT:
		case OP_SELECT:
			fc = context->foreColor;
			record_mono_color:
			a = fc.alpha; r = fc.red; g = fc.green; b = fc.blue;
			/* process the 1 color */
			if (endianess != HOST_ENDIANESS)
				colors[0] = (b<<24) | (g<<16) | (r<<8) | a;
			else
				colors[0] = (a<<24) | (r<<16) | (g<<8) | b;
			colors[2] = 1;
			break;
		default:
			fc = context->foreColor;
			a = fc.alpha; r = fc.red; g = fc.green; b = fc.blue;
			colors[3] = a+r+g+b;
			if (endianess != HOST_ENDIANESS) {	
				colors[0] = (b<<15) | (g<<3);
				colors[1] = (r<<15) | (a<<3);
			} else {
				colors[0] = (a<<15) | (r<<3);
				colors[1] = (g<<15) | (b<<3);
			}
			colors[2] = 0;
			break;
	}
	
	cache->fDrawString = DrawStringNonAA;
	DrawStringNonAA(context,port,str);
};

static void DrawStringCopy(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, dh, dv;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp;
	uint32 *	colors = cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rect		clipbox;
	int32 		clipIndex=0;
	
	grAssertLocked(context, port);
	
	/* preload context parameters */
	const int32 clipCount = port->RealDrawingRegion()->CountRects();
	const rect* clipRects = port->RealDrawingRegion()->Rects();
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;

	/* init empty previous character box */
	prev.top = -1048576;
	prev.left = -1048576;
	prev.right = -1048576;
	prev.bottom = -1048576;

	if ((clipIndex=find_span_between(port->RealDrawingRegion(),str->bbox.top,str->bbox.bottom))==-1) return;
	while ((clipIndex>0) && (clipRects[clipIndex].top > str->bbox.top)) clipIndex--;
	while ((clipIndex>0) && (clipRects[clipIndex].top == clipRects[clipIndex-1].top)) clipIndex--;

	for (;clipIndex<clipCount;clipIndex++) {
		clipbox = clipRects[clipIndex];
		if (clipbox.top > str->bbox.bottom) return;
		/* go through all the characters */
		for (int32 i=0; i<str->char_count; i++) {
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
	
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
	
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		
	
			/* test character not-overlaping */
			if ((prev.bottom < cur.top) ||
				(prev.right < cur.left) ||
				(prev.left > cur.right) ||
				(prev.top > cur.bottom)) {
				CopyCharAA32(	cur_base, cur_row, cur,
								src_base, src_row, src, colors);
			/* characters overlaping */
			} else {
				/* calculate intersection of the 2 rect */
				if (prev.left > cur.left)
					inter.left = prev.left;
				else
					inter.left = cur.left;
				if (prev.right < cur.right)
					inter.right = prev.right;
				else
					inter.right = cur.right;
				if (prev.top > cur.top)
					inter.top = prev.top;
				else
					inter.top = cur.top;
				if (prev.bottom < cur.bottom)
					inter.bottom = prev.bottom;
				else
					inter.bottom = cur.bottom;
				/* left not overlaping band */	
				dh = inter.left-cur.left-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.left;
					tmp.right = cur.left+dh;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.left;
					src_tmp.right = src.left+dh;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					CopyCharAA32(	cur_base, cur_row, tmp,
								 	src_base, src_row, src_tmp, colors);
				}
				/* right not overlaping band */	
				dh = cur.right-inter.right-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.right-dh;
					tmp.right = cur.right;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.right-dh;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					CopyCharAA32(	cur_base, cur_row, tmp,
									src_base, src_row, src_tmp, colors);
				}
				/* top not overlaping band */	
				dv = inter.top-cur.top-1;
				if (dv >= 0) {
					tmp.top = cur.top;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.top+dv;
					src_tmp.top = src.top;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.top+dv;
					CopyCharAA32(	cur_base, cur_row, tmp,
									src_base, src_row, src_tmp, colors);
				}
				/* bottom not overlaping band */	
				dv = cur.bottom-inter.bottom-1;
				if (dv >= 0) {
					tmp.top = cur.bottom-dv;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.bottom;
					src_tmp.top = src.bottom-dv;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom;
					CopyCharAA32(	cur_base, cur_row, tmp,
									src_base, src_row, src_tmp, colors);
				}
				/* at last the horrible overlaping band... */
				src_tmp.top = src.top + (inter.top-cur.top);
				src_tmp.left = src.left + (inter.left-cur.left);
				src_tmp.right = src.right + (inter.right-cur.right);
				src_tmp.bottom = src.bottom + (inter.bottom-cur.bottom);
				src_prev_tmp.top = src_prev.top + (inter.top-prev.top);
				src_prev_tmp.left = src_prev.left + (inter.left-prev.left);
				src_prev_tmp.right = src_prev.right + (inter.right-prev.right);
				src_prev_tmp.bottom = src_prev.bottom + (inter.bottom-prev.bottom);
				CopyCharAA32X(	cur_base, cur_row, inter,
								src_prev_base, src_prev_row, src_prev_tmp,
								src_base, src_row, src_tmp, colors);
			}
			src_prev_row = src_row;
			src_prev_base = src_base;
			src_prev.top = src.top;
			src_prev.left = src.left;
			src_prev.right = src.right;
			src_prev.bottom = src.bottom;
			prev.top = cur.top;
			prev.left = cur.left;
			prev.right = cur.right;
			prev.bottom = cur.bottom;
		}
	};
};

static void DrawStringCopyPreprocess(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int32 i, a0, r0, g0, b0, da, dr, dg, db;
	uint32 *colors = cache->fontColors;
	int8 endianess = canvas->pixels.endianess;
	rgb_color fc,bc;

	grAssertLocked(context, port);
	
	fc = context->foreColor;
	bc = context->backColor;
	if (endianess == HOST_ENDIANESS) {
		colors[7] = (((uint)fc.alpha)<<24) | (((uint)fc.red  )<<16) |
					(((uint)fc.green)<<8) | ((uint)fc.blue );
	} else {
		colors[7] = (((uint)fc.blue)<<24) | (((uint)fc.green)<<16) |
					(((uint)fc.red)<<8) | ((uint)fc.alpha);
	};
	a0 = ((int32)bc.alpha)<<8;
	r0 = ((int32)bc.red)<<8;
	g0 = ((int32)bc.green)<<8;
	b0 = ((int32)bc.blue)<<8;
	da = ((((int32)fc.alpha)<<8)-a0)/7;
	dr = ((((int32)fc.red)<<8)-r0)/7;
	dg = ((((int32)fc.green)<<8)-g0)/7;
	db = ((((int32)fc.blue)<<8)-b0)/7;
	a0 += 0x80; r0 += 0x80; g0 += 0x80; b0 += 0x80;
	if (endianess == HOST_ENDIANESS) {
		for (i=1; i<7; i++) {
			a0 += da; r0 += dr; g0 += dg; b0 += db;
			colors[i] = ((a0<<16)&0xff000000)|((r0<<8)&0xff0000)|(g0&0xff00)|(b0>>8);
		}
	} else {
		for (i=1; i<7; i++) {
			a0 += da; r0 += dr; g0 += dg; b0 += db;
			colors[i] = ((b0<<16)&0xff000000)|((g0<<8)&0xff0000)|(r0&0xff00)|(a0>>8);
		}
	};
	for (i=8; i<15; i++) colors[i] = colors[7];

	cache->fDrawString = DrawStringCopy;
	DrawStringCopy(context,port,str);
};

static void DrawStringNonCopy(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	int16 		mode=context->drawOp;
	DrawChar	drawChar;
	rect		clipbox;
	int32 		clipIndex=0;
	
	grAssertLocked(context, port);
	
	/* preload context parameters */
	uint32 *colors = cache->fontColors,*b_colors = cache->fontColors+16;
	const int32 clipCount = port->RealDrawingRegion()->CountRects();
	const rect* clipRects = port->RealDrawingRegion()->Rects();
	cur_base = canvas->pixels.pixelData;
	cur_row = canvas->pixels.bytesPerRow;
	if ((mode == OP_OVER) || (mode == OP_ALPHA))
		drawChar = OverCharAA32;
	else
		drawChar = OtherCharAA32;
		
	if ((clipIndex=find_span_between(port->RealDrawingRegion(),str->bbox.top,str->bbox.bottom))==-1) return;
	while ((clipIndex>0) && (clipRects[clipIndex].top > str->bbox.top)) clipIndex--;
	while ((clipIndex>0) && (clipRects[clipIndex].top == clipRects[clipIndex-1].top)) clipIndex--;

	for (;clipIndex<clipCount;clipIndex++) {
		clipbox = clipRects[clipIndex];
		if (clipbox.top > str->bbox.bottom) return;
		/* go through all the characters */
		for (int32 i=0; i<str->char_count; i++) {
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						mode, colors, b_colors);
		}		
	}
}

static void DrawStringOtherPreprocess(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int32 i, a0, r0, g0, b0, da, dr, dg, db, dh, dv;
	uint32 *colors = cache->fontColors,*b_colors = cache->fontColors+16;
	int8 endianess = canvas->pixels.endianess;
	rgb_color fc,bc;
	fc = context->foreColor;
	bc = context->backColor;

	grAssertLocked(context, port);
	
	if (context->drawOp == OP_ERASE) {
		da = bc.alpha; dr = bc.red; dg = bc.green; db = bc.blue;
	} else {
		da = fc.alpha; dr = fc.red; dg = fc.green; db = fc.blue;
	}
	colors[0] = 0L;
	b_colors[0] = 0L;
	colors[9] = da+db+dg+dr;
	if (endianess == HOST_ENDIANESS) {
		colors[7] = (da<<15) | (dr<<3);
		b_colors[7] = (dg<<15) | (db<<3);
	} else {
		colors[7] = (db<<15) | (dg<<3);
		b_colors[7] = (dr<<15) | (da<<3);
	};
	da <<= 5; dr <<= 5; dg <<= 5; db <<= 5;
	a0 = 0x80; r0 = 0x80; g0 = 0x80; b0 = 0x80;
	
	if (endianess == HOST_ENDIANESS) {
		for (i=1; i<7; i++) {
			a0 += da; r0 += dr; g0 += dg; b0 += db;
			colors[i] = ((a0<<7)&0x7ff000) | (r0>>5);
			b_colors[i] = ((g0<<7)&0x7ff000) | (b0>>5);			
		}
	} else {
		for (i=1; i<7; i++) {
			a0 += da; r0 += dr; g0 += dg; b0 += db;
			colors[i] = ((b0<<7)&0x7ff000) | (g0>>5);
			b_colors[i] = ((r0<<7)&0x7ff000) | (a0>>5);
		}
	};

	colors[8] =
		((colors[7]<<9  )&0xff000000) |
        ((colors[7]<<13 )&0x00ff0000) |
		((b_colors[7]>>7)&0x0000ff00) |
		((b_colors[7]>>3)&0x000000ff);

	cache->fDrawString = DrawStringNonCopy;
	DrawStringNonCopy(context,port,str);
};

typedef void (*AlphaDrawChar)(	uint8 *dst_base, int32 dst_row, rect dst,
								uint8 *src_base, int32 src_row, rect src,
								uint32 *colors, uint32 *alphas);

#define AlphaCompositeAAFunc(FuncName)											\
	static void FuncName (	uint8 *dst_base, int32 dst_row, rect dst,			\
							uint8 *src_base, int32 src_row, rect src,			\
							uint32 *colors, uint32 *alphas)						\
	{																			\
		int32		i, j;														\
		uint32		pixIn,alpha,gray,*dst_ptr,									\
					srcFactor,dstFactor,tmpVar1,								\
					srcPixel=colors[0],dstPixel,dstAlpha;						\
		uint8		*src_ptr;													\
																				\
		/* init read and read/write pointers */									\
		src_ptr = src_base+src_row*src.top;										\
		dst_ptr = (uint32*)(dst_base+dst_row*dst.top)+(dst.left-src.left);		\
		for (j=src.top; j<=src.bottom; j++) {									\
			for (i=src.left; i<=src.right; i++) {								\
				gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;						\
				alpha = alphas[gray];											\
																				\
				if (alpha == 256) {												\
					dst_ptr[i] = srcPixel;										\
					continue;													\
				};																\
																				\
				if (alpha == 0) continue;										\
																				\
				dstPixel = dst_ptr[i];											\
				AlphaBlend(srcPixel,dstPixel,alpha);							\
				dst_ptr[i] = dstPixel;											\
			}																	\
			src_ptr += src_row;													\
			dst_ptr = (uint32*)((uint8*)dst_ptr+dst_row);						\
		}																		\
	}

#define AlphaCompositeFunc(FuncName)											\
	static void FuncName (	uint8 *dst_base, int32 dst_row, rect dst,			\
							uint8 *src_base, int32 src_row, rect src,			\
							uint32 *colors, uint32 *alphas)						\
	{																			\
		int32	i, j, cmd, hmin, hmax, imin, imax;								\
		uint32	*dst_ptr,color,alpha,pixIn,srcFactor,							\
				dstFactor,tmpVar1,dstPixel,dstAlpha;							\
																				\
		color = colors[0];														\
		alpha = alphas[0];														\
		dst_ptr = (uint32*)(dst_base+dst_row*dst.top)+(dst.left-src.left);		\
																				\
		/* pass through the top lines that are clipped */						\
		for (j=0; j<src.top; j++)												\
			do src_base++; while (src_base[-1] != 0xff);						\
		/* for each visible lines, draw the visible runs */						\
		for (; j<=src.bottom; j++) {											\
			hmin = 0;															\
			while (TRUE) {														\
				/* decode next run */											\
				do {															\
					cmd = *src_base++;											\
					/* end of run list for this line */							\
					if (cmd == 0xff)											\
						goto next_line;											\
					hmin += cmd;												\
				} while (cmd == 0xfe);											\
				hmax = hmin;													\
				do {															\
					cmd = *src_base++;											\
					hmax += cmd;												\
				} while (cmd == 0xfe);											\
				/* horizontal clipping */										\
				if (hmin > src.right) {											\
					do src_base++; while (src_base[-1] != 0xff);				\
					goto next_line;												\
				}																\
				if (hmin >= src.left)											\
					imin = hmin;												\
				else															\
					imin = src.left;											\
				if (hmax <= src.right)											\
					imax = hmax;												\
				else															\
					imax = src.right+1;											\
				/* draw the visible part */										\
				for (i=imin; i<imax; i++) {										\
					pixIn = dst_ptr[i];											\
					AlphaBlend(color,dstPixel,alpha);							\
					dst_ptr[i] = dstPixel;										\
				};																\
				hmin = hmax;													\
			}																	\
		next_line:																\
			dst_ptr = (uint32*)((uint8*)dst_ptr+dst_row);						\
		}																		\
	}

#define RenderMode											DEFINE_OP_OVER
#define AlphaFunction										ALPHA_FUNCTION_COMPOSITE
#define SourceFormatBits									32
#define DestFormatBits										32
#define SourceFormatEndianess								LITTLE_ENDIAN
#define DestFormatEndianess									LITTLE_ENDIAN
	#include "pixelConvert.inc"
	AlphaCompositeAAFunc(AlphaCompositeCharAA32Little);
	AlphaCompositeFunc(AlphaCompositeChar32Little);
#define SourceFormatBits									32
#define DestFormatBits										32
#define SourceFormatEndianess								BIG_ENDIAN
#define DestFormatEndianess									BIG_ENDIAN
	#include "pixelConvert.inc"
	AlphaCompositeAAFunc(AlphaCompositeCharAA32Big);
	AlphaCompositeFunc(AlphaCompositeChar32Big);
#undef SourceFormatEndianess
#undef DestFormatEndianess
#undef AlphaFunction
#undef RenderMode
#undef SourceBits
#undef DestBits

static void AlphaCharAA32(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src,
							uint32 *colors, uint32 *alphas)
{
	int32		i, j;
	uint32		pixIn,alpha,gray,*dst_ptr;
	uint8		*src_ptr;

	/* init read and read/write pointers */
	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint32*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		for (i=src.left; i<=src.right; i++) {
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;
			alpha = alphas[gray];
			if (alpha == 0) {
				dst_ptr[i] = colors[7];
				continue;
			};
			
			if (alpha == 256) continue;

			pixIn = dst_ptr[i];
			dst_ptr[i] = (
							((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
							((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
						 ) + colors[gray];
		}
		src_ptr += src_row;
		dst_ptr = (uint32*)((uint8*)dst_ptr+dst_row);
	}
}

static void AlphaChar32(	uint8 *dst_base, int32 dst_row, rect dst,
							uint8 *src_base, int32 src_row, rect src,
							uint32 *colors, uint32 *alphas)
{
	int32	i, j, cmd, hmin, hmax, imin, imax;
	uint32	*dst_ptr,color,alpha,pixIn;

	color = colors[0];
	alpha = alphas[0];
	dst_ptr = (uint32*)(dst_base+dst_row*dst.top)+(dst.left-src.left);

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
			/* draw the visible part */
			for (i=imin; i<imax; i++) {
				pixIn = dst_ptr[i];
				dst_ptr[i] = (
								((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
								((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
							 ) + color;
			};
			hmin = hmax;
		}
	next_line:
		dst_ptr = (uint32*)((uint8*)dst_ptr+dst_row);
	}
}

static void DrawStringAlpha(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	int         	src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        	prev, src_prev, cur, src, inter;
	rect        	tmp, src_tmp, src_prev_tmp;
	uchar       	*src_base, *cur_base, *src_prev_base;
	fc_char     	*my_char;
	fc_point    	*my_offset;
	AlphaDrawChar	alphaDrawChar;
	rect			clipbox;
	int32			clipIndex=0;
	
	grAssertLocked(context, port);
	
	/* preload context parameters */
	uint32 *colors = cache->fontColors,*alphas = cache->fontColors+8;
	const int32 clipCount = port->RealDrawingRegion()->CountRects();
	const rect* clipRects = port->RealDrawingRegion()->Rects();
	cur_base = canvas->pixels.pixelData;
	cur_row = canvas->pixels.bytesPerRow;
	alphaDrawChar = (AlphaDrawChar)cache->fontColors[16];
		
	if ((clipIndex=find_span_between(port->RealDrawingRegion(),str->bbox.top,str->bbox.bottom))==-1) return;
	while ((clipIndex>0) && (clipRects[clipIndex].top > str->bbox.top)) clipIndex--;
	while ((clipIndex>0) && (clipRects[clipIndex].top == clipRects[clipIndex-1].top)) clipIndex--;

	for (;clipIndex<clipCount;clipIndex++) {
		clipbox = clipRects[clipIndex];
		if (clipbox.top > str->bbox.bottom) return;
		/* go through all the characters */
		for (int32 i=0; i<str->char_count; i++) {
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
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
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
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		
			alphaDrawChar(	cur_base, cur_row, cur,
							src_base, src_row, src,
							colors, alphas);
		}
	}
}

static void noop(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
};

static void DrawStringAlphaPreprocess(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCanvas *canvas = port->canvas;
	RenderCache *cache = port->cache;
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;
	uint32
		*colors = cache->fontColors,
		*alphas = cache->fontColors+8;
	int8 endianess = canvas->pixels.endianess;
	rgb_color fc = context->foreColor;

	grAssertLocked(context, port);
	
	if (fc.alpha == 0) {
		cache->fDrawString = noop;
		return;
	};

	if (context->alphaFunction == ALPHA_FUNCTION_COMPOSITE) {
		colors[0] = cache->foreColorCanvasFormat;
		da = alpha = fc.alpha;
		alpha += (alpha >> 7);
		alphas[0] = 0;
		alphas[7] = alpha;
		da = (da<<8)/7;
		a0 = 0x80;
		for (i=1; i<7; i++) {
			a0 += da;
			alpha = (a0+0x80) >> 8;
			alpha += (alpha >> 7);
			alphas[i] = alpha;
		}
		if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {
			if (endianess == LENDIAN)
				cache->fontColors[16] = (uint32)AlphaCompositeChar32Little;
			else
				cache->fontColors[16] = (uint32)AlphaCompositeChar32Big;
		} else {
			if (endianess == LENDIAN)
				cache->fontColors[16] = (uint32)AlphaCompositeCharAA32Little;
			else
				cache->fontColors[16] = (uint32)AlphaCompositeCharAA32Big;
		};
	} else {
		if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {
			if (fc.alpha == 255) {
				DrawStringNonAAPreprocess(context,port,str);
				return;
			};
	
			alpha = fc.alpha;
			alpha += (alpha >> 7);
			da = (fc.alpha * alpha);
			dr = (fc.red * alpha);
			dg = (fc.green * alpha);
			db = (fc.blue * alpha);
			if (endianess == HOST_ENDIANESS) {
				colors[0] = (((da<<16) | dg) & 0xFF00FF00) |
							(((dr<<8) | (db>>8)) & 0x00FF00FF) ;
			} else {
				colors[0] = (((db<<16) | dr) & 0xFF00FF00) |
							(((dg<<8) | (da>>8)) & 0x00FF00FF) ;
			};
			alphas[0] = 256-alpha;
			cache->fontColors[16] = (uint32)AlphaChar32;
		} else {
			da = alpha = fc.alpha;
			alpha += (alpha >> 7);
			pixIn = cache->foreColorCanvasFormat;
			colors[0] = 0L;
			colors[7] = (
						((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
						((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
					 );
			alphas[0] = 256;
			alphas[7] = 256-alpha;
			
			da = (da<<8)/7;
			a0 = 0x80;
			
			for (i=1; i<7; i++) {
				a0 += da;
				alpha = (a0+0x80) >> 8;
				alpha += (alpha >> 7);
				colors[i] = (
							((((pixIn>>8) & 0x00FF00FF) * alpha) & 0xFF00FF00) |
							((((pixIn & 0x00FF00FF) * alpha) & 0xFF00FF00) >> 8)
						 );
				alphas[i] = 256 - alpha;
			}
			cache->fontColors[16] = (uint32)AlphaCharAA32;
		};
	};
	cache->fDrawString = DrawStringAlpha;
	DrawStringAlpha(context,port,str);
};

DrawString DrawStringTable32[2][NUM_OPS] =
{
	{
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringNonAAPreprocess,
		DrawStringAlphaPreprocess
	},
	{
		DrawStringCopyPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringOtherPreprocess,
		DrawStringAlphaPreprocess
	}
};
