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

/*---------------------------------------------------------------*/
void	cleanup_region(region*);
/*---------------------------------------------------------------*/

extern void scan_erase_offset(region **rr, TView* a_view, 
	int32 x, int32 y, RenderContext *wdefContext);

void	TWindow::w2_window_redraw(region* damage)
{
	if (empty_region(damage)) return;

	fpoint newOrigin;
	region *tmpRegion1 = newregion();
	region *tmpRegion2 = newregion();
	newOrigin.h = newOrigin.v = 0;

	grLock(wdefContext);
#if 0
		/* Clear the views */
		grSetOrigin(wdefContext,newOrigin);
		and_region(damage, ClientRegion(), tmpRegion1);
		offset_region(tmpRegion1,-ClientRegion()->bound.left,-ClientRegion()->bound.top);
		grClipToIRegion(wdefContext,tmpRegion1);
		top_view->ClearGather(&tmpRegion1, wdefContext, NULL);
#endif
		/* Draw the wdef */
		newOrigin.h = -ClientRegion()->bound.left;
		newOrigin.v = -ClientRegion()->bound.top;
		grSetOrigin(wdefContext,newOrigin);
		grClipToRegion(wdefContext,damage);
		grSetForeColor(wdefContext, rgb(100, 100, 100));
		rect r = window_bound;
		r.left++; r.top++; r.bottom--; r.right--;
		grStrokeIRect(wdefContext, r);

	grUnlock(wdefContext);

	kill_region(tmpRegion1);
	kill_region(tmpRegion2);
}

/*---------------------------------------------------------------*/

void	TWindow::w2_update_hilite()
{
}

/*---------------------------------------------------------------*/

void	TWindow::w2_update_close_box(char hilited)
{
}

/*---------------------------------------------------------------*/


region*	TWindow::w2_calc_full(long proc_id, rect* bound)
{
	region	*rgn;
	rect	bnd;

	bnd=*bound;
	bnd.top += 1;
	bnd.bottom -= 1;
	bnd.left += 1;
	bnd.right -= 1;

	rgn=newregion();

	ra(rgn,0)=bnd;

	rgn->count=1;
	cleanup_region(rgn);

	return(rgn);
}

/*---------------------------------------------------------------*/

region*	TWindow::w2_calc_draw(long proc_id, rect* bound)
{
	region	*rgn;
	rect	tmp_rect;


	tmp_rect=*bound;

	tmp_rect.top+=2;
	tmp_rect.bottom-=2;
	tmp_rect.left+=2;
	tmp_rect.right-=2;
	
	rgn=newregion();
	set_region(rgn,&tmp_rect);
	return(rgn);
}

/*---------------------------------------------------------------*/

long	TWindow::w2_find_part(long h, long v)
{
	return(CONTENT_AREA);
}

/*---------------------------------------------------------------*/

region*	TWindow::w2_calc_update(rect old_rect, rect new_rect)
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
		   VisibleRegion(),
		   tmp_region1);

/* dispose the tmp regions				*/

	kill_region(tmp_region2);
	kill_region(tmp_region3);
	return(tmp_region1);
}


/*---------------------------------------------------------------*/

