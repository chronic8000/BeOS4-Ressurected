//******************************************************************************
//
//	File:		wdef_new_sb.c
//
//	Description:	brand new wdef, you will love it.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1994, Be Incorporated
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

#ifndef	MWINDOW_H
#include "mwindow.h"
#endif

#ifndef	_TEXT_H
#include "text.h"
#endif

#ifndef	_STRING_H
#include <string.h>
#endif

#include "render.h"
#include "renderUtil.h"
#include "rect.h"

#define DEBUG	1
#include <Debug.h>

/*---------------------------------------------------------------*/
void	cleanup_region(region*);
/*---------------------------------------------------------------*/

#define	bar_size	WD_TAB_HEIGHT
#define	BB			WD_FRAME_SIZE
#define	lbs			BB	/*left border size*/
#define	rbs			BB
#define	bbs			BB	/*bottom border size*/
#define	tbs			BB


#define	go_off_top	2
#define	go_off_left	4
#define	go_dx		8
#define	go_dy		8

/*---------------------------------------------------------------*/
#define ul1(pid,left,top,right,bottom,c)			\
	lineto_d_c(pid, left, top, right, bottom, c)
/*---------------------------------------------------------------*/
#define	BOXW	7

/*---------------------------------------------------------------*/

void	TWindow::w11_window_redraw(region* damage)
{
	w10_window_redraw(damage);
}

/*---------------------------------------------------------------*/


void	TWindow::w11_update_close_box(char hilited)
{
	w10_update_close_box(hilited);
}

/*---------------------------------------------------------------*/

void	TWindow::w11_update_minimized_close_box(rect *box, char hilited)
{
	w10_update_minimized_close_box(box, hilited);
}

/*---------------------------------------------------------------*/

void	TWindow::w11_update_hilite()
{
	w10_update_hilite();
}

/*---------------------------------------------------------------*/

region*	TWindow::w11_calc_full(long proc_id, rect* bound)
{
	region	*rgn;
	rect	bnd;

	rgn = newregion();

	bnd = *bound;
	bnd.bottom = bnd.top + bar_size;
	bnd.right = bnd.left + w10_calc_title_width(*bound);
	ra(rgn, 0) = bnd;
	bnd = *bound;
	bnd.top = bnd.top + bar_size + 1;
	ra(rgn, 1) = bnd;

	rgn->count = 2;
	cleanup_region(rgn);

	return(rgn);
}

/*---------------------------------------------------------------*/

region*	TWindow::w11_calc_draw(long proc_id, rect* bound)
{
	region	*rgn, *rgn2, *rgn3;
	rect	tmp_rect;

	tmp_rect=*bound;

	tmp_rect.top += bar_size + tbs + 1;
	tmp_rect.bottom -= bbs;
	tmp_rect.left += lbs;
	tmp_rect.right -= rbs;
	
	rgn=newregion();
	set_region(rgn,&tmp_rect);

	return(rgn);
/* let's keep this code for when we will move the resize box in the wdef */
	tmp_rect.top = tmp_rect.bottom-13;
	tmp_rect.left = tmp_rect.right-13;

	rgn2 = newregion();
	set_region(rgn2,&tmp_rect);

	rgn3 = newregion();
	sub_region(rgn, rgn2, rgn3);
	
	kill_region(rgn);
	kill_region(rgn2);
	
	return(rgn3);
}

/*---------------------------------------------------------------*/

long 	TWindow::w11_find_part(long h, long v)
{
	rect	tmp_rect;
	rect	tmp_rect1;

	tmp_rect = window_bound;

	tmp_rect1 = tmp_rect;

	tmp_rect1.bottom = tmp_rect1.top + CLOSE_ICON_HEIGHT + ICON_MARGIN;
	tmp_rect1.top += ICON_MARGIN;
	tmp_rect1.right = tmp_rect1.left + CLOSE_ICON_WIDTH + ICON_MARGIN;
	tmp_rect1.left += ICON_MARGIN;

	if (point_in_rect(&tmp_rect1, h, v)) {
		
		if (flags & NO_GOAWAY)
			return(TITLE_BAR);
		else
			return(CLOSE_BOX);
	}

	if (!(flags & NO_ZOOM)) {
		tmp_rect1.right = tmp_rect.left + w10_calc_title_width(window_bound);
		tmp_rect1.left = tmp_rect1.right - (ZOOM_ICON_WIDTH + ICON_MARGIN);
		tmp_rect1.right -= ICON_MARGIN;
		if (point_in_rect(&tmp_rect1, h, v))
			return(ZOOM_BOX);
	}

	/* if (!(flags & NO_MINIMIZE)) {
		if (!(flags & NO_ZOOM)) {
			tmp_rect1.right = tmp_rect1.left - ICON_MARGIN;
			tmp_rect1.left = tmp_rect1.right - MINIMIZE_ICON_WIDTH;
		}
		else {
			tmp_rect1.right = tmp_rect.left + w10_calc_title_width(window_bound);
			tmp_rect1.left = tmp_rect1.right - (MINIMIZE_ICON_WIDTHi + ICON_MARGIN);
			tmp_rect1.right -= ICON_MARGIN;
		}
		if (point_in_rect(&tmp_rect1, h, v))
			return(MINIMIZE_BOX);
	} */

	tmp_rect1 = tmp_rect;
	tmp_rect1.top = tmp_rect1.bottom - 16;
	tmp_rect1.left = tmp_rect1.right - 16;

	if (point_in_rect(&tmp_rect1, h, v)) {
		if ((flags & NO_SIZE) == 0) 
			return(RESIZE_AREA);
	}

	tmp_rect1 = tmp_rect;
	tmp_rect1.bottom = tmp_rect1.top + bar_size;
	tmp_rect1.right = tmp_rect1.left + w10_calc_title_width(window_bound);
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	// check for click in top border
	tmp_rect1 = tmp_rect;
	tmp_rect1.top = tmp_rect1.top + bar_size + 1;
	tmp_rect1.bottom = tmp_rect1.top + (tbs-1);
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	// check for click in bottom border
	tmp_rect1 = tmp_rect;
	tmp_rect1.top = tmp_rect1.bottom - (bbs-1);
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	// check for click in left border
	tmp_rect1 = tmp_rect;
	tmp_rect1.right = tmp_rect1.left + (lbs-1);
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	// check for click in right border
	tmp_rect1 = tmp_rect;
	tmp_rect1.left = tmp_rect1.right - (rbs-1);
	tmp_rect1.top += bar_size;
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	return(CONTENT_AREA);
}

/*---------------------------------------------------------------*/

long 	TWindow::w11_find_minimized_part(rect *box, long h, long v)
{
	rect	tmp_rect;

	tmp_rect.top = box->top + ICON_MARGIN;
	tmp_rect.bottom = tmp_rect.top + CLOSE_ICON_HEIGHT;
	tmp_rect.left = box->left + ICON_MARGIN;
	tmp_rect.right = tmp_rect.left + CLOSE_ICON_WIDTH;

	if ((point_in_rect(&tmp_rect, h, v)) && (!(flags & NO_GOAWAY)))
		return(CLOSE_BOX);
	return(TITLE_BAR);
}

/*---------------------------------------------------------------*/

region*	TWindow::w11_calc_update(rect old_rect, rect new_rect)
{
	rect	tmp_rect;
	region	*tmp_region0 = 0;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	long	tw;

	tmp_rect.top = 0;
	tmp_rect.bottom = 0;
	tmp_rect.left = 0;
	tmp_rect.right = 10000;

	tw = w10_calc_title_width(tmp_rect);
	if ((old_rect.right != new_rect.right)
	   &&((new_rect.right - new_rect.left) <= tw ||
	      (old_rect.right - old_rect.left) <= tw)) {
		tmp_region0 = newregion();	
		tmp_rect.right = new_rect.left + tw;
		/* UGLY HARDCODE VALUE DEFINING UPDATE REGION OF WINDOW TITLE */
//		tmp_rect.left = min(old_rect.right - 30, new_rect.right - 30);
		tmp_rect.left = min(old_rect.right, new_rect.right)-40;
		if (!(flags & NO_ZOOM))
			tmp_rect.left -= (ZOOM_ICON_WIDTH + ICON_MARGIN);
//+		if (!(flags & NO_MINIMIZE))
//+			tmp_rect.left -= (MINIMIZE_ICON_WIDTH + ICON_MARGIN);
		tmp_rect.top = min(old_rect.top, new_rect.top);
		tmp_rect.bottom = max(old_rect.top + bar_size, new_rect.top + bar_size);
		set_region(tmp_region0, &tmp_rect);
	}
	    
	
	old_rect.top += bar_size + 1;
	new_rect.top += bar_size + 1;

/* build the right side rectangle and make a region with it */

	if (0 && old_rect.right == new_rect.right) {
		tmp_rect.left = min(old_rect.right, new_rect.right);
		tmp_rect.right = tmp_rect.left;
		tmp_rect.top = new_rect.bottom;
		tmp_rect.bottom = new_rect.bottom;
	}
	else {
		tmp_rect.left = min(old_rect.right, new_rect.right) - (rbs-1);
		tmp_rect.right = max(old_rect.right, new_rect.right);
		tmp_rect.top = new_rect.top;
		tmp_rect.bottom = new_rect.bottom;
	}


	tmp_region1 = newregion();
	set_region(tmp_region1, &tmp_rect);

/* build the left side rectangle and make a region with it */

	tmp_rect.top = min(old_rect.bottom, new_rect.bottom) - (bbs-1);
	tmp_rect.bottom = max(old_rect.bottom, new_rect.bottom);
	tmp_rect.left = new_rect.left;
	tmp_rect.right = new_rect.right;


	tmp_region2=newregion();
	set_region(tmp_region2, &tmp_rect);


	tmp_region3 = newregion();

/* take the union of those two rectangles		 */

	or_region(tmp_region1, tmp_region2, tmp_region3);
	if (tmp_region0) {
		or_region(tmp_region0, tmp_region3, tmp_region2);
		kill_region(tmp_region0);

/* intersect it with the visible region			 */
		
		and_region(tmp_region2,
		   	   VisibleRegion(),
		   	   tmp_region1);
	}
	else {
		and_region(tmp_region3,
			   VisibleRegion(),
			   tmp_region1);
	}

/* dispose the tmp regions				*/

	kill_region(tmp_region2);
	kill_region(tmp_region3);
	return(tmp_region1);
}


/*---------------------------------------------------------------*/
