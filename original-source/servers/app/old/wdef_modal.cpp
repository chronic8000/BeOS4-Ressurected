//******************************************************************************
//
//	File:		wdef_modal.c
//
//	Description:	modal wdef
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

#include "render.h"
#include "renderUtil.h"
#include "rect.h"

/*---------------------------------------------------------------*/
void	cleanup_region(region*);
/*---------------------------------------------------------------*/

#define	bar_size	0
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

void	TWindow::w3_window_redraw(region* damage)
{
	if (empty_region(damage)) return;

	fpoint newOrigin;
	grLock(wdefContext);
		newOrigin.h = -ClientRegion()->bound.left;
		newOrigin.v = -ClientRegion()->bound.top;
		grSetOrigin(wdefContext,newOrigin);
		grClipToRegion(wdefContext,damage);
		w_draw_frame(damage);
	grUnlock(wdefContext);
}

/*---------------------------------------------------------------*/

void	TWindow::w3_update_hilite()
{
	fpoint newOrigin;
	grLock(wdefContext);
		newOrigin.h = -ClientRegion()->bound.left;
		newOrigin.v = -ClientRegion()->bound.top;
		grSetOrigin(wdefContext,newOrigin);
		grClearClip(wdefContext);
		w_draw_frame(NULL);
	grUnlock(wdefContext);
}

/*---------------------------------------------------------------*/


region*	TWindow::w3_calc_full(long proc_id, rect* bound)
{
	region	*rgn;

	rgn = newregion();
	ra(rgn, 0) = *bound;
	rgn->count = 1;
	cleanup_region(rgn);

	return(rgn);
}

/*---------------------------------------------------------------*/

region*	TWindow::w3_calc_draw(long proc_id, rect* bound)
{
	region	*rgn;
	rect	tmp_rect;


	tmp_rect=*bound;

	tmp_rect.top += tbs;
	tmp_rect.bottom -= bbs;
	tmp_rect.left += lbs;
	tmp_rect.right -= rbs;
	
	rgn=newregion();
	set_region(rgn, &tmp_rect);
	return(rgn);
}

/*---------------------------------------------------------------*/

void	TWindow::w3_update_close_box(char hilited)
{
}

/*---------------------------------------------------------------*/

long 	TWindow::w3_find_part(long h, long v)
{
	rect	tmp_rect;
	rect	tmp_rect1;

	tmp_rect = window_bound;

	if (!(flags & NO_SIZE)) {
		tmp_rect1 = tmp_rect;
		tmp_rect1.top = tmp_rect1.bottom - 21;
		tmp_rect1.left = tmp_rect1.right - 4;
	
		if (point_in_rect(&tmp_rect1, h, v))
			return(RESIZE_AREA);
	
		tmp_rect1 = tmp_rect;
		tmp_rect1.top = tmp_rect1.bottom - 4;
		tmp_rect1.left = tmp_rect1.right - 21;
	
		if (point_in_rect(&tmp_rect1, h, v))
			return(RESIZE_AREA);
	}

	tmp_rect1 = tmp_rect;
	tmp_rect1.left = tmp_rect1.right - rbs;
	tmp_rect1.top += bar_size;
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	tmp_rect1 = tmp_rect;
	tmp_rect1.bottom = tmp_rect1.top + tbs;
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}
	tmp_rect1 = tmp_rect;
	tmp_rect1.right = tmp_rect1.left + lbs;
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}
	tmp_rect1 = tmp_rect;
	tmp_rect1.top = tmp_rect1.bottom - bbs;
	if (point_in_rect(&tmp_rect1, h, v)) {
		return(TITLE_BAR);
	}

	return(CONTENT_AREA);
}


/*---------------------------------------------------------------*/

region * TWindow::w3_calc_update(rect old_rect, rect new_rect)
{
	rect	tmp_rect;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;

	tmp_rect.top = 0;
	tmp_rect.bottom = 0;
	tmp_rect.left = 0;
	tmp_rect.right = 10000;

/* build the right side rectangle and make a region with it */

	if (0 && old_rect.right == new_rect.right) {
		tmp_rect.left = min(old_rect.right, new_rect.right);
		tmp_rect.right = tmp_rect.left;
		tmp_rect.top = new_rect.bottom;
		tmp_rect.bottom = new_rect.bottom;
	}
	else {
		tmp_rect.left = min(old_rect.right, new_rect.right) - rbs;
		tmp_rect.right = max(old_rect.right, new_rect.right);
		tmp_rect.top = new_rect.top;
		tmp_rect.bottom = new_rect.bottom;
	}


	tmp_region1 = newregion();
	set_region(tmp_region1, &tmp_rect);

/* build the left side rectangle and make a region with it */

	tmp_rect.top = min(old_rect.bottom, new_rect.bottom) - bbs;
	tmp_rect.bottom = max(old_rect.bottom, new_rect.bottom);
	tmp_rect.left = new_rect.left;
	tmp_rect.right = new_rect.right;


	tmp_region2=newregion();
	set_region(tmp_region2, &tmp_rect);


	tmp_region3 = newregion();

/* take the union of those two rectangles		 */

	or_region(tmp_region1, tmp_region2, tmp_region3);
	and_region(tmp_region3, VisibleRegion(), tmp_region1);

/* dispose the tmp regions				*/

	kill_region(tmp_region2);
	kill_region(tmp_region3);
	return(tmp_region1);
}


/*---------------------------------------------------------------*/
