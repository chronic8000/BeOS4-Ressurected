//******************************************************************************
//
//	File:		wdef_col1.c
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
#include  "text.h"
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

/*---------------------------------------------------------------*/

void	TWindow::w1_draw_go_box(rect bound, char hilited)
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
		dark == rgb(255, 0, 0);
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

void	TWindow::w1_window_redraw(region* damage)
{
	port		*saved_port;
	region		*tmp_region;
	region		*oldclip;
	rect		tmp_r;
	rect		tmp_r1;
	rect		tmp_r2;
	long		i;
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
			visible_region,
			tmp_region);

	//oldclip = (region *)getclip();

	setclip(tmp_region);

	tmp_r = window_bound;
	rect_g2l(&window_port, &tmp_r);


/* Fill the biggest part of the drag bar	*/

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left + 1,
		 tmp_r.top + bar_size - 1,
		 tmp_r.right - 1);

	
	if ((state & look_active) == 0)
		tmp_color = rgb(g_light, g_light, g_light);
	else 
		tmp_color = rgb(180, 180, 180);

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
		w1_draw_go_box(tmp_r2, 0);

	tmp_r = window_bound;
	rect_g2l(&window_port, &tmp_r);
	
	dark = rgb(g_very_dark, g_very_dark, g_very_dark);
	
	rect_frame(&tmp_r, op_copy, dark, dark);
	setup_color(getpid(), rgb_black, rgb_white);
	

	kill_region(tmp_region);
	//kill_region(oldclip);

	set_port(saved_port);
}

/*---------------------------------------------------------------*/

void	TWindow::w1_update_close_box(char hilited)
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

	setclip(visible_region);

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
	
	w1_draw_go_box(tmp_r2, hilited);

	setup_color(getpid(), rgb_black, rgb_white);

	set_port(saved_port);
}

/*---------------------------------------------------------------*/

void	TWindow::w1_update_hilite()
{
	port		*saved_port;
	region		*oldclip;
	rect		tmp_r;
	rect		tmp_r1;
	rect		tmp_r2;
	long		i;
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

	//oldclip=(region *)getclip();

	setclip(visible_region);

	tmp_r=window_bound;

	rect_g2l(&window_port,&tmp_r);

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left + 1,
		 tmp_r.top + bar_size - 1,
		 tmp_r.right - 1);

	
	if ((state & look_active) == 0)
		tmp_color = rgb(g_light, g_light, g_light);
	else 
		tmp_color = rgb(180, 180, 180);

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
		w1_draw_go_box(tmp_r2, 0);

	setup_color(getpid(), rgb_black, rgb_white);

	//kill_region(oldclip);
	set_port(saved_port);
}

/*---------------------------------------------------------------*/


region*	TWindow::w1_calc_full(long proc_id, rect* bound)
{
	region	*rgn;
	rect	bnd;
	rect	tmp;
	long	right;


	bnd = *bound;

	rgn = newregion();

	ra(rgn, 0) = bnd;

	rgn->count = 1;
	cleanup_region(rgn);

	return(rgn);
}

/*---------------------------------------------------------------*/

region*	TWindow::w1_calc_draw(long proc_id, rect* bound)
{
	region	*rgn;
	rect	tmp_rect;


	tmp_rect=*bound;

	tmp_rect.top+=bar_size;
	tmp_rect.bottom -= bbs;
	tmp_rect.left += lbs;
	tmp_rect.right -= rbs;
	
	rgn=newregion();
	set_region(rgn,&tmp_rect);
	return(rgn);
}

/*---------------------------------------------------------------*/

long 	TWindow::w1_find_part(long h, long v)
{
	rect	tmp_rect;
	rect	tmp_rect1;

	tmp_rect = window_bound;

	tmp_rect1 = tmp_rect;
	tmp_rect1.bottom = tmp_rect1.top + bar_size;
	tmp_rect1.right = tmp_rect1.left + bar_size - 1;

	if (point_in_rect(tmp_rect1, h, v)) {
		if (flags & NO_GOAWAY)
			return(TITLE_BAR);
		else
			return(CLOSE_BOX);
	}


	tmp_rect1 = tmp_rect;
	tmp_rect1.bottom = tmp_rect1.top + bar_size;
	if (point_in_rect(tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	tmp_rect.top = tmp_rect.bottom - 12;
	tmp_rect.left = tmp_rect.right - 12;

	if (point_in_rect(tmp_rect, h, v)) {
		if (flags & NO_SIZE) 
			return(CONTENT_AREA);
		else
			return(RESIZE_AREA);
	}

	return(CONTENT_AREA);
}


/*---------------------------------------------------------------*/

region*	TWindow::w1_calc_update(rect old_rect, rect new_rect)
{
	rect	tmp_rect;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;


/* build the right side rectangle and make a region with it */

	tmp_rect.left = min(old_rect.right, new_rect.right) - 1;
	tmp_rect.right = max(old_rect.right, new_rect.right);
	tmp_rect.top = new_rect.top;
	tmp_rect.bottom = new_rect.bottom;


	tmp_region1 = newregion();
	set_region(tmp_region1, &tmp_rect);

/* build the left side rectangle and make a region with it */

	tmp_rect.top = min(old_rect.bottom, new_rect.bottom) - 1;
	tmp_rect.bottom = max(old_rect.bottom, new_rect.bottom);
	tmp_rect.left = new_rect.left;
	tmp_rect.right = new_rect.right;


	tmp_region2=newregion();
	set_region(tmp_region2, &tmp_rect);


	tmp_region3 = newregion();

/* take the union of those two rectangles		 */

	or_region(tmp_region1, tmp_region2, tmp_region3);

/* intersect it with the visible region			 */

	and_region(tmp_region3,
		   visible_region,
		   tmp_region1);

/* dispose the tmp regions				*/

	kill_region(tmp_region2);
	kill_region(tmp_region3);
	return(tmp_region1);
}


/*---------------------------------------------------------------*/
