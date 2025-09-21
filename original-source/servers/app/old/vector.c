//******************************************************************************
//
//	File:		vector.c
//
//	Description:	1 bit vector drawing, + horizontal and vertical.
//			special casing.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef	POLYS_H
#include "polys.h"
#endif

#include "math.h"

#if 0

/*------------------------------------------------------------------*/
extern	rect	total_cursor;
/*------------------------------------------------------------------*/

#define left_code	0x01
#define	right_code	0x02
#define	bottom_code	0x04
#define	top_code	0x08


/*------------------------------------------------------------------*/
typedef void (*HLINEP)(port *, long, long, long, rect*, short mode);
typedef void (*VLINEP)(port *, long, long, long, rect*, short mode);
typedef void (*LINEP)(port *, long, long, long, long, rect, short);
typedef void (*LINEPM)(port *, long, long, long, long, rect, int mode);
typedef void (*RUNLISTP)(port*,raster_line*,long,rect*,pattern*,long,long,short);
typedef void (*RUNLIST)(port*,raster_line*,long,rect*,short,char);

void line_clipper(port *a_port, long xx1, long yy1, long xx2, long yy2, rect cliprect, short mode)
{
	long		tmp;
	uchar		code1;
	uchar		code2;
	rect        crect;
	LINEP		line_jmp;

	line_jmp = (LINEP)a_port->gr_procs[VECTOR_RTN];

	if (xx1 > xx2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	code1 = 0;
	if (yy1 > cliprect.bottom)
		code1 |= bottom_code;
	else
	if (yy1 < cliprect.top)
		code1 |= top_code;

	if (xx1 < cliprect.left)
		code1 |= left_code;
	else
	if (xx1 > cliprect.right)
		code1 |= right_code;

	code2 = 0;
	if (yy2 > cliprect.bottom)
		code2 |= bottom_code;
	else
	if (yy2 < cliprect.top)
		code2 |= top_code;

	if (xx2 < cliprect.left)
		code2 |= left_code;
	else
	if (xx2 > cliprect.right)
		code2 |= right_code;

	if ((code1 & code2) == 0) {
/* The previous formula (coming from an unknown book...    */
/*	if (((code1 | code2) == 0) || ((code1 ^ code2) != 0))  */
		if ((code1 | code2) != 0) {
			/* filter to detect and truncate line clearly to long, if potential truncation */
			tmp = yy1-yy2;
			if (tmp < 0)
				tmp = xx2-xx1-tmp;
			else
				tmp += xx2-xx1;
			/* do the truncation */	
			if (tmp > 8192) {
				/* used the max of the real cliping rect and a standard gabarit to limit
				   errors for small drawing areas. The errors will be independant of the
				   cliping region for drawing areas included in [-2048, -2048, 4096, 4096] */
				crect = cliprect;
				if (crect.left > -2048) crect.left = -2048;
				if (crect.right < 4096) crect.right = 4096;
				if (crect.top > -2048) crect.top = -2048;
				if (crect.bottom < 4096) crect.bottom = 4096;
				/* truncate on the left side */
				if (xx1 < crect.left) {
					tmp = crect.left-xx1;
					yy1 += (long)((double)(yy2-yy1)*(double)tmp/(double)(xx2-xx1));
					xx1 = crect.left;
				}
				/* truncate on the right side */
				if (xx2 > crect.right) {
					tmp = xx2-crect.right;
					yy2 += (long)((double)(yy1-yy2)*(double)tmp/(double)(xx2-xx1));
					xx2 = crect.right;
				}
				/* truncate on the top and bottom side */
				if (yy1 > yy2) {
					/* truncate on the top side */
					if (yy2 < crect.top) {
						tmp = crect.top-yy2;
						xx2 += (long)((double)(xx1-xx2)*(double)tmp/(double)(yy1-yy2));
						yy2 = crect.top;
					}
					/* truncate on the bottom side */
					if (yy1 > crect.bottom) {
						tmp = yy1-crect.bottom;
						xx1 += (long)((double)(xx2-xx1)*(double)tmp/(double)(yy1-yy2));
						yy1 = crect.bottom;
					}					
				}
				else {
					/* truncate on the top side */
					if (yy1 < crect.top) {
						tmp = crect.top-yy1;
						xx1 += (long)((double)(xx2-xx1)*(double)tmp/(double)(yy2-yy1));
						yy1 = crect.top;
					}
					/* truncate on the bottom side */
					if (yy2 > crect.bottom) {
						tmp = yy2-crect.bottom;
						xx2 += (long)((double)(xx1-xx2)*(double)tmp/(double)(yy2-yy1));
						yy2 = crect.bottom;
					}
				}
			}
		}
		(line_jmp)(a_port,xx1,yy1,xx2,yy2,cliprect,mode);
	}
}

/*--------------------------------------------------------------------*/

void _lineto_d_c(port *a_port,
	     		 short x1,
	     		 short y1,
	     		 short x2,
	     		 short y2)
{
	region	*theclip;
	rect	cursor_rect;
	long	rectcount;
	long	i;	
	long	tmp;
	LINEPM	*jmp;
	short	mode = op_copy;


	x1 += a_port->origin.h;
	x2 += a_port->origin.h;
	y1 += a_port->origin.v;
	y2 += a_port->origin.v;


	if (y1 < y2) {
		cursor_rect.top = y1;
		cursor_rect.bottom = y2;
	}
	else {
		cursor_rect.top = y2;
		cursor_rect.bottom = y1;
	}

	if (x1 < x2) {
		cursor_rect.left = x1;
		cursor_rect.right = x2;
	}
	else {
		cursor_rect.left = x2;
		cursor_rect.right = x1;
	}

	if ((a_port->port_type & OFFSCREEN_PORT)  == 0) {
		get_screen();

		if (cursor_rect.top > total_cursor.bottom)
			goto skip;
		if (cursor_rect.bottom < total_cursor.top)
			goto skip;
		cursor_hide(cursor_rect);
	}
skip:;

	theclip = a_port->draw_region;
	rectcount = theclip->count;

	if (y1 == y2) {
		jmp = (LINEPM*)a_port->gr_procs[HORIZ_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((HLINEP)jmp)
			(a_port, x1, x2, y1, &(theclip->data[i]), mode);
			if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
				release_screen();
			}
		return;
	}

	if (x1 == x2) {
		jmp = (LINEPM*)a_port->gr_procs[VERT_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((VLINEP)jmp)
			(a_port, x1, y1, y2, &(theclip->data[i]), mode);
		if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
			release_screen();
		}
		return;
	}

	for (i = 0; i < rectcount; i++) {
		 line_clipper(a_port, x1, y1, x2, y2,
		 	      	  theclip->data[i],
			      	  mode);
	}
	if ((a_port->port_type & OFFSCREEN_PORT)  == 0) {
		release_screen();
	}
}

/*--------------------------------------------------------------------*/

void _lineto(port *a_port,
	    	long x1,
	    	long y1,
	    	long x2,
	    	long y2,
	    	short mode,
	    	long pen_size)
{
	region	*theclip;
	rect	cursor_rect;
	long	rectcount;
	long	i;	
	long	tmp;
	PVRF	*jmp;

	if (pen_size > 1) {
		_lineto_thick(a_port, x1, y1, x2, y2, mode, pen_size);
		return;
	}
	
	x1 += a_port->origin.h;
	x2 += a_port->origin.h;
	y1 += a_port->origin.v;
	y2 += a_port->origin.v;


	if (y1 < y2) {
		cursor_rect.top = y1;
		cursor_rect.bottom = y2;
	}
	else {
		cursor_rect.top = y2;
		cursor_rect.bottom = y1;
	}

	if ((a_port->port_type & OFFSCREEN_PORT)  == 0) {
		get_screen();
	}

	if (cursor_rect.top > total_cursor.bottom)
		goto skip;
	if (cursor_rect.bottom < total_cursor.top)
		goto skip;

	if (x1 < x2) {
		cursor_rect.left = x1;
		cursor_rect.right = x2;
	}
	else {
		cursor_rect.left = x2;
		cursor_rect.right = x1;
	}
	if (cursor_rect.left > total_cursor.right)
		goto skip;

	if ((a_port->port_type & OFFSCREEN_PORT)  == 0) {
		cursor_hide(cursor_rect);
	}

skip:;

	theclip = a_port->draw_region;
	rectcount = theclip->count;

	if (y1 == y2) {
		jmp = a_port->gr_procs[HORIZ_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((HLINEP)jmp)
			(a_port, x1, x2, y1, &(theclip->data[i]), mode);
			if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
				release_screen();
			}
		return;
	}

	if (x1 == x2) {
		jmp = a_port->gr_procs[VERT_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((VLINEP)jmp)
			(a_port, x1, y1, y2, &(theclip->data[i]), mode);
		if ((a_port->port_type & OFFSCREEN_PORT) == 0) {
			release_screen();
		}
		return;
	}

	for (i = 0; i < rectcount; i++) {
		 line_clipper(a_port, x1, y1, x2, y2,
		 	      theclip->data[i],
			      mode);
	}
	if ((a_port->port_type & OFFSCREEN_PORT)  == 0) {
		release_screen();
	}
}

/*--------------------------------------------------------------------*/

void _lineto1(port *a_port,
	      long x1,
	      long y1,
	      long x2,
	      long y2,
	      short mode,
	      long pen_size)
{
	region	*theclip;
	rect	cursor_rect;
	long	rectcount;
	long	i;	
	long	tmp;
	PVRF	*jmp;


	if (pen_size > 1) {
		_lineto_thick(a_port, x1, y1, x2, y2, mode, pen_size);
		return;
	}

	x1 += a_port->origin.h;
	x2 += a_port->origin.h;
	y1 += a_port->origin.v;
	y2 += a_port->origin.v;


	if (y1 < y2) {
		cursor_rect.top = y1;
		cursor_rect.bottom = y2;
	}
	else {
		cursor_rect.top = y2;
		cursor_rect.bottom = y1;
	}

	if (x1 < x2) {
		cursor_rect.left = x1;
		cursor_rect.right = x2;
	}
	else {
		cursor_rect.left = x2;
		cursor_rect.right = x1;
	}

	if (cursor_rect.left > total_cursor.right)
		goto skip;

	if (cursor_rect.top > total_cursor.bottom)
		goto skip;
	
	if ((a_port->port_type & OFFSCREEN_PORT)  == 0)
		cursor_hide(cursor_rect);
	
skip:;

	theclip = a_port->draw_region;
	rectcount = theclip->count;

	if (y1 == y2) {
		jmp = a_port->gr_procs[HORIZ_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((HLINEP)jmp)
			(a_port, x1, x2, y1, &(theclip->data[i]), mode);
		return;
	}

	if (x1 == x2) {
		jmp = a_port->gr_procs[VERT_VECT_RTN];
		for (i = 0; i < rectcount; i++)
			((VLINEP)jmp)
			(a_port, x1, y1, y2, &(theclip->data[i]), mode);
		return;
	}

	for (i = 0; i < rectcount; i++) {
		 line_clipper(a_port, x1, y1, x2, y2,
		 	      theclip->data[i],
			      mode);
	}
}

/*--------------------------------------------------------------------*/

void _lineto_thick(	port *a_port,
	     		long xx1,
	     		long yy1,
	     		long xx2,
	     		long yy2,
	     		short mode,
	     		long pen_size)
{
	_lineto_pat(a_port, xx1, yy1, xx2, yy2, mode, pen_size, get_pat(BLACK));
}

/*--------------------------------------------------------------------*/
/*
void _lineto_pat(	port *a_port,
	     		long xx1,
	     		long yy1,
	     		long xx2,
	     		long yy2,
	     		short mode,
	     		long pen_size,
			pattern *a_pat)
{
	point	list[7];
	rect	bound;
	long	i;
	long	tmp;


	if (yy1 > yy2) {
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
	}

	pen_size--;
	
	if (xx1 == xx2) {
		bound.top = yy1 - (pen_size>>1);
		bound.bottom = yy2 + ((pen_size+1)>>1);
		bound.left = xx1 - (pen_size>>1);
		bound.right = xx1 + ((pen_size+1)>>1);
		_rect_fill(a_port, &bound, 1, a_pat, mode);
		return;
	} 

	if (yy1 == yy2) {
		bound.top = yy1 - (pen_size>>1);
		bound.bottom = yy1 + ((pen_size+1)>>1);
		bound.left = min(xx1,xx2) - (pen_size>>1);
		bound.right = max(xx1,xx2) + ((pen_size+1)>>1);
		_rect_fill(a_port, &bound, 1, a_pat, mode);
		return;
	}

	bound.top = 10000000;
	bound.bottom = -10000000;
	bound.left = 10000000;
	bound.right = -10000000;

	if (xx2 >= xx1) {
		list[0].h = xx1; list[0].v = yy1;
		list[1].h = xx1 + pen_size; list[1].v = yy1;
		list[2].h = xx2 + pen_size; list[2].v = yy2;
		list[3].h = xx2 + pen_size; list[3].v = yy2 + pen_size;
		list[4].h = xx2; list[4].v = yy2 + pen_size;
		list[5].h = xx1; list[5].v = yy1 + pen_size;
		list[6].h = xx1; list[6].v = yy1;
	}
	else {
		list[0].h = xx1; list[0].v = yy1;
		list[1].h = xx1 + pen_size; list[1].v = yy1;
		list[2].h = xx1 + pen_size; list[2].v = yy1 + pen_size;
		list[3].h = xx2 + pen_size; list[3].v = yy2 + pen_size;
		list[4].h = xx2; list[4].v = yy2 + pen_size;
		list[5].h = xx2; list[5].v = yy2;
		list[6].h = xx1; list[6].v = yy1;
	}

	for (i = 0; i < 6; i++) {
		bound.top	= min(bound.top, list[i].v);
		bound.bottom	= max(bound.bottom, list[i].v);
		bound.left 	= min(bound.left, list[i].h);
		bound.right	= max(bound.right, list[i].h);
	}

	_poly_fill(a_port, &bound, 6, list, a_pat, mode);
}
*/

#define	SQRT2	1.414213
#pragma global_optimizer off

//---------------------------------------------------------------

long	to_int_x(float v)
{
	if (v < 0)
		return(v - 0.5);
	return(v+0.5);
}

//---------------------------------------------------------------

void _lineto_pat(	port	*a_port,
	     			long	xx1,
	     			long	yy1,
	     			long	xx2,
	     			long	yy2,
	     			short	mode,
	     			long	pen_size,
					pattern *a_pat)
{
	point	list[5];
	rect	bound;
	long	i;
	long	tmp;
	float	l;
	float	dh, dv;
	long	idh, idv;
	float	px1, px2;
	float	py1, py2;
	float	ipx1, ipx2;
	float	ipy1, ipy2;
	
	//xprintf("0p0\n");
	if (yy1 > yy2) {
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
	}

	pen_size--;

/*
	if (xx1 == xx2) {
		dh = pen_size/2;
		bound.top = yy1;
		bound.bottom = yy2;
		bound.left = xx1 - dh;
		bound.right = xx1 + pen_size - dh;
		_rect_fill(a_port, &bound, 1, a_pat, mode);
		return;
	} 

	if (yy1 == yy2) {
		bound.top = yy1 - (pen_size>>1);
		bound.bottom = yy1 + ((pen_size+1)>>1);
		bound.left = min(xx1,xx2) - (pen_size>>1);
		bound.right = max(xx1,xx2) + ((pen_size+1)>>1);
		_rect_fill(a_port, &bound, 1, a_pat, mode);
		return;
	}
*/
	
/* simple line drau using set_pixel for pen_size <= 1 */
	if (pen_size <= 0) {
		long           dx,dy,sx,error,cpt,rectcnt;
		long           phase_x,phase_y;
		region	       *theclip;
		raster_line    *draw,*buf;
		RUNLISTP       *jmp;
		
		xx1 += a_port->origin.h;
		xx2 += a_port->origin.h;
		yy1 += a_port->origin.v;
		yy2 += a_port->origin.v;
		
		dx = xx2 - xx1;
		dy = yy2 - yy1;

		if (dx < 0) {
			sx = -1;
			dx = -dx;
		}
		else sx = 1;
	
		buf = (raster_line*)gr_malloc(sizeof(raster_line)*(dy+2));
		draw = buf;
		get_screen();
		if (dx > dy) {
			error = dx >> 1;

			draw->x0 = xx1;

			for (cpt=dx;cpt>=0;cpt--) {
				xx1 += sx;
				error += dy;
				if (error >= dx) {
					draw->x1 = xx1-sx;
					draw->y = yy1;
					draw++;
					draw->x0 = xx1;
					error -= dx;
					yy1++;
				}
			}
			draw->x1 = xx1-sx;
			draw->y = yy1;
		} 
		else {
			error = dy >> 1;
		
			for (cpt=dy;cpt>=0;cpt--) {
				draw->x0 = xx1;
				draw->x1 = xx1;
				draw->y = yy1;
				draw++;
				error += dx;
				yy1++;
				
				if (error >= dy) {
					xx1 += sx;					
					error -= dy;
				}
			}
		}

		theclip = a_port->draw_region;
		rectcnt = theclip->count;
		
		if ((*(long*)&a_pat->data[0] == 0xffffffffL) &&
			(*(long*)&a_pat->data[4] == 0xffffffffL)) {
			jmp = a_port->gr_procs[RUN_LIST];
			for (i=0;i<rectcnt;i++)
				((RUNLIST)jmp)(a_port,buf,dy+1,theclip->data+i,mode,0);
		}
		else {
			jmp = a_port->gr_procs[RUN_LIST_PAT];
			phase_x = 7 - (a_port->origin.h & 7);
			phase_y = (a_port->origin.v) & 7; 
			for (i=0;i<rectcnt;i++)
				((RUNLISTP)jmp)(a_port,buf,dy+1,theclip->data+i,
								a_pat,phase_x,phase_y,mode);
		}
		free(buf);
		release_screen();
		return;
	}

/* other cases using a fill_poly */
	bound.top 	 = 10000000;
	bound.bottom = -10000000;
	bound.left 	 = 10000000;
	bound.right  = -10000000;

	dh = xx2-xx1;
	dv = yy2-yy1;

	l = sqrt(dh*dh+dv*dv);
	if (l == 0.0) l = 1;
	l = l / pen_size;
	px1 = (dv) / l;
	py1 = (dh) / l;


	dh = px1/2;
	dv = py1/2;
	idh = to_int_x(dh);
	idv = to_int_x(dv);
	ipx1 = to_int_x(px1);
	ipy1 = to_int_x(py1);

	list[0].h = xx1 - idh	  ; list[0].v = yy1 + (ipy1-idv);
	list[1].h = xx1 + (ipx1-idh); list[1].v = yy1 - (idv);
	list[2].h = xx2 + (ipx1-idh); list[2].v = yy2 - (idv);
	list[3].h = xx2 - idh	  ; list[3].v = yy2 + (ipy1 - idv);

	for (i = 0; i < 4; i++) {
		bound.top		= min(bound.top, list[i].v);
		bound.bottom	= max(bound.bottom, list[i].v);
		bound.left 		= min(bound.left, list[i].h);
		bound.right		= max(bound.right, list[i].h);
	}

	_poly_fill(a_port, &bound, 4, list, a_pat, mode);
}

#pragma global_optimizer on

//---------------------------------------------------------------

double			t0,t1;

void	do_pixel_array(port *a_port,
					   long count,
					   short_point *pts,
					   long dx,
					   long dy,
					   rect includer,
					   short mode)
{
	raster_line    *draw;
	RUNLISTP       *jmp;
	long			i;
	raster_line    *p;
	long			h,v;

	t0 = t1;
	t1 = system_time();
	draw = (raster_line*)gr_malloc(sizeof(raster_line)*count);

	p = draw;
	for (i = 0; i < count; i++) {
		h = pts->h + dx;
		v = pts->v + dy;
		
		if (h >= includer.left  &&
			h < includer.right  &&
			v >= includer.top   &&
			v <  includer.bottom) {
			p->x1 = p->x0 = h; 
			p->y = v;
			p++;
		}

		pts++;
	}


	jmp = a_port->gr_procs[RUN_LIST];

	((RUNLIST)jmp)(a_port,draw,count,&(a_port->bbox), mode,1);
	free((char *)draw);
} 

//---------------------------------------------------------------

void	do_pixel_array_delta(port *a_port,
							 long count,
							 short_point *pts,
							 long dx1,
							 long dy1,
							 long dx2,
							 long dy2,
							 raster_line *temp_array,
							 short mode)
{
	raster_line    *draw;
	RUNLISTP       *jmp;
	long			i;
	raster_line    *p;

	t0 = t1;
	t1 = system_time();
	draw = temp_array;

	p = draw;
	for (i = 0; i < count; i++) {
		p->x1 = p->x0 = pts->h + dx1; 
		p->y = pts->v + dy1;
		p++;
		p->x1 = p->x0 = pts->h + dx2; 
		p->y = pts->v + dy2;
		p++;
		pts++;
	}


	jmp = a_port->gr_procs[RUN_LIST];

	((RUNLIST)jmp)(a_port,draw,count*2,&(a_port->bbox), mode,1);
} 

#endif
