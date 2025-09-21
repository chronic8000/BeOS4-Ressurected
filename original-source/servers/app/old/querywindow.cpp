//******************************************************************************
//
//	File:		querywindow.c
//
//	Description:	one of the wdef.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1993, Be Incorporated
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

#ifndef	WINDOW_H
#include "window.h"
#endif

#ifndef	_TEXT_H
#include "text.h"
#endif

/*---------------------------------------------------------------*/

#define	bar_size	15
#define	lbs		1	/*left border size*/
#define	rbs		1
#define	bbs		1	/*bottom border size*/


#define	go_off_top	2
#define	go_off_left	4
#define	go_dx		8
#define	go_dy		8

#define	g_dark		160
#define	g_very_dark	80
#define	g_light		227
#define	g_light1	150

/*---------------------------------------------------------------*/

void	TWindow::w5_draw_go_box(rect bound, char hilited)
{
	rgb_color	dark;
	rgb_color	light;
	
#define	g_very_dark	80
	
	if ((state & look_active) != 0)
		dark = rgb(g_very_dark, g_very_dark, g_very_dark);
	else
		dark = rgb(g_dark, g_dark, g_dark);
	
	light= rgb(g_light, g_light, g_light);

	if (hilited) {
		dark = rgb(255, 0, 0);
		lineto(	bound.left, bound.top, bound.right - 1, bound.top, op_copy, 2, dark, dark);
		lineto(	bound.left, bound.top, bound.left, bound.bottom - 1, op_copy, 2, dark, dark);
		lineto(	bound.right - 1, bound.top + 1, bound.right - 1, bound.bottom - 1, op_copy, 1, rgb_white, rgb_white);
		lineto(	bound.right - 1, bound.bottom - 1, bound.left + 1, bound.bottom - 1, op_copy, 1, rgb_white, rgb_white);
	}

	lineto(	bound.left, bound.top, bound.right - 1, bound.top, op_copy, 1, dark, dark);
	lineto(	bound.left, bound.top, bound.left, bound.bottom - 1, op_copy, 1, dark, dark);
	lineto(	bound.right - 1, bound.top + 1, bound.right - 1, bound.bottom - 1, op_copy, 1, rgb_white, rgb_white);
	lineto(	bound.right - 1, bound.bottom - 1, bound.left + 1, bound.bottom - 1, op_copy, 1, rgb_white, rgb_white);
}

/*---------------------------------------------------------------*/

void	draw_draw_area1(rect r)
{
	r.top += bar_size + 2 + 14;
	r.bottom -= 15;
	r.left += 1;
	r.right -= 15;

	rect_fill(&r, get_pat(BLACK), op_copy, 1, rgb_white, rgb_white);
}

/*---------------------------------------------------------------*/

void	TWindow::w5_window_redraw(region* damage)
{
	port		*saved_port;
	region		*tmp_region;
	region		*oldclip;
	rect		tmp_r;
	rect		tmp_r1;
	rect		tmp_r2;
	long		i;
	long		old_clip;
	long		right;
	long		oldh;
	long		oldv;
	rgb_color	tmp_color;
	rgb_color	tmp_color2;
	rgb_color	dark;

	long	old_origin_h;
	long	old_origin_v;

	if (empty_region(damage))
		return;

	saved_port = get_port();
	set();

	old_origin_h = window_port.origin.h;
	old_origin_v = window_port.origin.v;

	window_port.origin.h = window_bound.left;
	window_port.origin.v = window_bound.top;


	tmp_region = newregion();
	
	and_region(	damage,
			VisibleRegion(),
			tmp_region);

	//oldclip = (region *)getclip();

	setclip(tmp_region);

	tmp_r = window_bound;
	rect_g2l(&window_port, &tmp_r);


	draw_draw_area1(tmp_r);

/* Fill the biggest part of the drag bar	*/

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left + 1,
		 tmp_r.top + bar_size - 1,
		 tmp_r.right - 1);

	
	if ((state & look_active) == 0)
		tmp_color = rgb(50 * 4, 50 * 4, 255);
	else 
		tmp_color = rgb(30 * 4, 30 * 4, 255);

	rect_fill(&tmp_r1, get_pat(BLACK), op_copy, 1, tmp_color, tmp_color);

/* Draw the title					*/


	if ((state & look_active) == 0)
		tmp_color2 = rgb(100, 100, 100);
	else 
		tmp_color2 = rgb(0, 0, 0);
	
	draw_string_screen_only(window_name,
							wfont,
							op_copy,
							tmp_r1.left + 25,
							tmp_r1.top + 1 + wfont->ascent(),
							tmp_color2,
							tmp_color);

	tmp_r2 = tmp_r1;
	tmp_r2.left += go_off_left;
	tmp_r2.right = tmp_r2.left + go_dx;
	tmp_r2.top += go_off_top;
	tmp_r2.bottom = tmp_r2.top + go_dy;

	if ((flags & NO_GOAWAY) == 0)
		w5_draw_go_box(tmp_r2, 0);

	tmp_r = window_bound;
	rect_g2l(&window_port, &tmp_r);
	
	dark = rgb(g_very_dark, g_very_dark, g_very_dark);
	
	rect_frame(&tmp_r, op_copy, dark, dark);
	setup_color(getpid(), rgb_black, rgb_white);
	

	kill_region(tmp_region);

	set_port(saved_port);
}

/*---------------------------------------------------------------*/


void	TWindow::w5_update_close_box(char hilited)
{
	port		*saved_port;
	rect		tmp_r;
	rect		tmp_r1;
	rect		tmp_r2;
	long		right;
	long		oldh;
	long		oldv;
	rgb_color	tmp_color;

	saved_port = get_port();
	set();

	window_port.origin.h = window_bound.left;
	window_port.origin.v = window_bound.top;

	setclip(VisibleRegion());

	tmp_r = window_bound;

	rect_g2l(&window_port, &tmp_r);

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left + 1,
		 tmp_r.top + bar_size - 1,
		 tmp_r.right - 1);

	
	if ((state & look_active) == 0)
		tmp_color = rgb(g_light, g_light, g_light);
	else 
		tmp_color = rgb(180, 180, 180);

	
	tmp_r2 = tmp_r1;
	tmp_r2.left += go_off_left;
	tmp_r2.right = tmp_r2.left + go_dx;
	tmp_r2.top += go_off_top;
	tmp_r2.bottom = tmp_r2.top + go_dy;
	
	rect_fill(&tmp_r2, get_pat(BLACK), op_copy, 1, tmp_color, tmp_color);
	
	w5_draw_go_box(tmp_r2, hilited);

	setup_color(getpid(), rgb_black, rgb_white);

	//set_port(saved_port);
}

/*---------------------------------------------------------------*/

void	TWindow::w5_update_hilite()
{
	port		*saved_port;
	region		*oldclip;
	rect		tmp_r;
	rect		tmp_r1;
	rect		tmp_r2;
	long		i;
	long		old_clip;
	long		right;
	long		oldh;
	long		oldv;
	rgb_color	tmp_color;
	rgb_color	tmp_color2;
	rgb_color	dark;

	long		old_origin_h;
	long		old_origin_v;



	saved_port = get_port();
	set();

	window_port.origin.h=window_bound.left;
	window_port.origin.v=window_bound.top;


	setclip(VisibleRegion());

	tmp_r=window_bound;

	rect_g2l(&window_port,&tmp_r);

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left + 1,
		 tmp_r.top + bar_size - 1,
		 tmp_r.right - 1);

	
	if ((state & look_active) == 0)
		tmp_color = rgb(50 * 4, 50 * 4, 255);
	else 
		tmp_color = rgb(30 * 4, 30 * 4, 255);

	rect_fill(&tmp_r1, get_pat(BLACK), op_copy, 1, tmp_color, tmp_color);
	

	if ((state & look_active) == 0)
		tmp_color2 = rgb(100, 100, 100);
	else 
		tmp_color2 = rgb(0, 0, 0);
	
	draw_string_screen_only(window_name,
							wfont,
							op_copy,
							tmp_r1.left + 25,
							tmp_r1.top + 1 + wfont->ascent(),
							tmp_color2,
							tmp_color);

	tmp_r2 = tmp_r1;
	tmp_r2.left += go_off_left;
	tmp_r2.right = tmp_r2.left + go_dx;
	tmp_r2.top += go_off_top;
	tmp_r2.bottom = tmp_r2.top + go_dy;
	
	if ((flags & NO_GOAWAY) == 0)
		w5_draw_go_box(tmp_r2, 0);

	setup_color(getpid(), rgb_black, rgb_white);

	set_port(saved_port);
}

/*---------------------------------------------------------------*/


region*	TWindow::w5_calc_full(long proc_id, rect* bound)
{
	return(w1_calc_full(proc_id, bound));
}

/*---------------------------------------------------------------*/

region*	TWindow::w5_calc_draw(long proc_id, rect* bound)
{
	return(w1_calc_draw(proc_id, bound));
}

/*---------------------------------------------------------------*/

long 	TWindow::w5_find_part(long h, long v)
{
	return(w1_find_part(h, v));
}


/*---------------------------------------------------------------*/

region*	TWindow::w5_calc_update(rect old_rect, rect new_rect)
{
	return(w1_calc_update(old_rect, new_rect));
}
