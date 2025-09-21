//******************************************************************************
//
//	File:		wdef_no_drop.c
//
//	Description:	one of the wdef.
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

#ifndef	WINDOW_H
#include "window.h"
#endif

#ifndef	_TEXT_H
#include  "text.h"
#endif

/*---------------------------------------------------------------*/

#define	bar_size	15

/*---------------------------------------------------------------*/


void	TWindow::w1_window_redraw(region* damage)
{
	port	*saved_port;
	region	*tmp_region;
	region	*oldclip;
	rect	tmp_r;
	rect	tmp_r1;
	rect	tmp_r2;
	long	i;
	long	old_clip;
	long	right;
	long	oldh;
	long	oldv;

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


/* Fill the content of the window		*/


	tmp_r.top += bar_size;
	tmp_r.bottom -= 1;
	tmp_r.left += 1;
	tmp_r.right -= 1;


	tmp_r = window_bound;

	rect_g2l(&window_port, &tmp_r);


/* Fill the biggest part of the drag bar	*/

	set_rect(tmp_r1,
		 tmp_r.top,
		 tmp_r.left,
		 tmp_r.top + bar_size - 2,
		 tmp_r.right);

	rect_fill(&tmp_r1, get_pat(BLACK), op_copy, 1, rgb_black, rgb_white);

	tmp_r1.left++;
	tmp_r1.right -= 1;
	tmp_r1.top+=1;
/*	tmp_r1.bottom-=1; */

/* refill the content in white, so we've got a frame	*/


	rect_fill(&tmp_r1, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);

/* Draw the title					*/

	draw_string_screen_only(window_name,
							wfont,
							op_copy,
							tmp_r1.left + 18,
							tmp_r1.top + 1 + wfont->ascent(),
							rgb_black,
							rgb_white);

	tmp_r2 = tmp_r1;
	tmp_r2.left += 4;
	tmp_r2.right = tmp_r2.left + 8;
	tmp_r2.top += 2;
	tmp_r2.bottom = tmp_r2.top + 8;

	if ((flags & NO_GOAWAY) == 0)
		rect_frame(&tmp_r2, op_copy, 1, rgb_black, rgb_white);

	if ((state & look_active) != 0) {
		rect_invert(&tmp_r1, 1, 1.0);
	}

	tmp_r1.left--;
	tmp_r1.right += 1;
	tmp_r1.top-=1;
/*	tmp_r1.bottom+=1; */




/* left border of window */
	
	lineto(tmp_r.left,
	       tmp_r.top + bar_size - 1,
	       tmp_r.left,
	       tmp_r.bottom,
	       op_copy,
	       1,
	       rgb_black,
	       rgb_white);

/* bottom line of window */

	 lineto(tmp_r.left,
	        tmp_r.bottom,
		tmp_r.right,
		tmp_r.bottom,
		op_copy,
		1,
		rgb_black,
		rgb_white);

/* right line of window */

	lineto(	tmp_r.right,
		tmp_r.bottom,
		tmp_r.right,
		tmp_r.top + bar_size - 1,
		op_copy,
		1,
		rgb_black,
		rgb_white);

/* bottom of title bar */

	lineto(	tmp_r.right,
		tmp_r.top + bar_size - 1,
		tmp_r.left,
		tmp_r.top + bar_size - 1,
		op_copy,
		1,
		rgb_black,
		rgb_white);


	kill_region(tmp_region);
	//kill_region(oldclip);


	set_port(saved_port);
}

/*---------------------------------------------------------------*/

void	TWindow::w1_update_hilite()
{
	port	*saved_port;
	region	*oldclip;
	rect	tmp_r;
	rect	tmp_r1;
	rect	tmp_r2;
	long	i;
	long	old_clip;
	long	right;
	long	oldh;
	long	oldv;

	long	old_origin_h;
	long	old_origin_v;



	saved_port=get_port();
	set();


	window_port.origin.h=window_bound.left;
	window_port.origin.v=window_bound.top;



	//oldclip=(region *)getclip();

	setclip(visible_region);

	tmp_r=window_bound;

	rect_g2l(&window_port,&tmp_r);

	set_rect(tmp_r1,
		 tmp_r.top + 1,
		 tmp_r.left, tmp_r.top + bar_size - 2,
		 tmp_r.right);

	rect_fill(&tmp_r1, get_pat(BLACK), op_copy, 1, rgb_black, rgb_white);

	tmp_r1.left++;
	tmp_r1.right -= 1;

	rect_fill(&tmp_r1, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);

	draw_string_screen_only(window_name,
							wfont,
							op_or,
							tmp_r1.left + 18,
							tmp_r1.top + 1 + wfont->ascent(),
							rgb_black,
							rgb_white);

	tmp_r2 = tmp_r1;
	tmp_r2.left += 4;
	tmp_r2.right = tmp_r2.left + 8;
	tmp_r2.top += 2;
	tmp_r2.bottom = tmp_r2.top + 8;
	if ((flags & NO_GOAWAY) == 0)
		rect_frame(&tmp_r2, op_copy, 1, rgb_black, rgb_white);

	if ((state & look_active)!=0) {
		rect_invert(&tmp_r1, 1);
	}

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


	bnd=*bound;

	rgn=newregion();

	ra(rgn,0)=bnd;

	rgn->count=1;
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
	tmp_rect.bottom-=1;
	tmp_rect.left+=1;
	tmp_rect.right-=1;
	
	rgn=newregion();
	set_region(rgn,&tmp_rect);
	return(rgn);
}

/*---------------------------------------------------------------*/

long 	TWindow::w1_find_part(long h, long v)
{
	rect	tmp_rect;
	rect	tmp_rect1;

	tmp_rect=window_bound;

	tmp_rect1=tmp_rect;
	tmp_rect1.bottom=tmp_rect1.top+bar_size;
	tmp_rect1.right=tmp_rect1.left+bar_size-1;

	if (point_in_rect(&tmp_rect1,h,v)) {
		if (flags & NO_GOAWAY)
			return(TITLE_BAR);
		else
			return(CLOSE_BOX);
	}


	tmp_rect1=tmp_rect;
	tmp_rect1.bottom=tmp_rect1.top+bar_size;
	if (point_in_rect(&tmp_rect1,h,v)) {
		return(TITLE_BAR);
	}

	tmp_rect.top=tmp_rect.bottom-12;
	tmp_rect.left=tmp_rect.right-12;

	if (point_in_rect(&tmp_rect,h,v)) {
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
