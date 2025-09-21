//******************************************************************************
//
//	File:		view.cpp
//
//	Description:	all the stuff needed for views.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			New Event system	may 94	bgs
//
//
//******************************************************************************/
   
#include <stdio.h>
#include <stdlib.h>
#include <OS.h>
#include "gr_types.h"
#include "proto.h"

#ifndef	AS_SUPPORT_H
#include <as_support.h>
#endif

#include "view.h"
#include "server.h"
#include "render.h"

#include <messages.h>
#include "Container.h"
#include "blit.h"
 
#undef DEBUG
#define	DEBUG	0
#include <Debug.h>
/*-------------------------------------------------------------*/

// TView constructor

region * TView::VisibleRegion()
{
	return this->renderContext->visibleRegion;
};

void TView::SetVisibleRegion(region *r)
{
	grSetVisibleRegion(this->renderContext, r);
};

void TView::SetVisibleRegion(rect *r)
{
	grSetVisibleRegion(this->renderContext, r);
};

extern pattern _black;

TView::TView(rect *bnd, char draw, long size_rules)
{
	region	*a_region;

// init fields of the view.
	bound 			= 	*bnd;
	do_draw			= 	draw;
	next_sibling	= 	0;
	first_child		= 	0;
	offset_h 		= 	0;
	offset_v		= 	0;
	request_h 		= 	0;
	request_v		= 	0;
	parent			= 	0;
	rules			= 	size_rules;
	owner			= 	0;
	user_clip 		= 	0;
	client_token	= 	-1;
	scale			= 	1.0;
	view_color.red	=	255;
	view_color.green=	255;
	view_color.blue	=	255;
	marked_for_print=	0;
	is_client_view	=   0;
	clippedToClient = true;

	valid_signature = VIEW_SIGNATURE;

	renderContext = grNewRenderContext();
	grInitRenderContext(renderContext);
	SetVisibleRegion(&bound);
	grSyncGlue(renderContext,this, getpid(),&_black);
	
	// clip insane values on boundary
	if (bnd->top < -1048576)
		bnd->top = -1048576;
	if (bnd->top > 1048576)
		bnd->top = 1048576;
	if (bnd->left < -1048576)
		bnd->left = -1048576;
	if (bnd->left > 1048576)
		bnd->left = 1048576;
	if (bnd->right < -1048576)
		bnd->right = -1048576;
	if (bnd->right > 1048576)
		bnd->right = 1048576;
	if (bnd->bottom < -1048576)
		bnd->bottom = -1048576;
	if (bnd->bottom > 1048576)
		bnd->bottom = 1048576;
		
// allocate a new token to identify the view on the client side.

	server_token = tokens->new_token(getpid(), TT_VIEW, this);
}

TView::~TView()
{
	// Placeholder to allow virtual destructor...
	/*	Do we have to get rid of associated regions here (if they
		are even still used at all)... */
	grDestroyRenderContext(renderContext);
	grFreeRenderContext(renderContext);
};

/*-------------------------------------------------------------*/

void	TView::set_flags(long flags)
{
	this->rules	= flags;
}

/*-------------------------------------------------------------*/

void	TView::close()
{
}

/*-------------------------------------------------------------*/
// remove a view from the child list of a given view.

void	TView::remove_from_list(TView *a_view)
{
	TView	*tmp;
	TView	*tmp1;


	tmp = first_child;
	if (tmp == a_view) {
		first_child = tmp->next_sibling;
		return;
	}

// scan the child list until the right view is found.
// when found, remove it from the linked list.

	while (tmp) {
		tmp1 = tmp->next_sibling;
		if (tmp1 == a_view) {
			tmp->next_sibling = tmp1->next_sibling;
			return;	
		}
		tmp = tmp1;
	}
// we should never come to that case.

	dprintf("we should not be here. Call Bill Gates at (515)372.23.12\n");
}

/*-------------------------------------------------------------*/

void	TView::do_close1()
{
	TView	*a_child;
	TView	*a_child1;

	a_child = first_child;

	while (a_child) {
		a_child1 = a_child->next_sibling;
		a_child->do_close1();
		a_child = a_child1;
	}

	close();		// user hook
	inval();

	tokens->remove_token(server_token);

	if (the_server->last_view_md == this)
		the_server->last_view_md = 0;
	
	valid_signature = 0;
		
	delete this;
}

/*-------------------------------------------------------------*/
// close view will remove the view from the view tree,
// and recurs. remove the subview from that part of the view tree.
// finally, the area used by the sub-tree will be invaled from the window,
// and the token will be remove from the token space.

void	TView::do_close()
{
	TView	*a_child;
	TView	*a_child1;

// remove the parent from the tree.

	wait_regions();
	if (parent)
		parent->remove_from_list(this);
	
	if (parent) {
		parent->recalc_visible(0);
	}
	
	
	a_child = first_child;

// call recursively the close1 for all the childs of the view.

	while (a_child) {
		a_child1 = a_child->next_sibling;
		a_child->do_close1();
		a_child = a_child1;
	}

	close();		// user hook

// inval the rectangle of the top of the tree.

	inval();

// remove the visible region if the view was a draw view.
// remove the token from the token space.

	tokens->remove_token(server_token);
	signal_regions();

	if (owner->last_view_mouse == this)
		the_server->force_move++;

	if (the_server->last_view_md == this)
		the_server->last_view_md = 0;
	
	valid_signature = 0;
		
	delete this;
}

/*-------------------------------------------------------------*/
// add_view will add the view <a_view> to the view tree.


void	TView::add_view(TView *a_view, bool last)
{
	rect	view_bound;
	rect	tmp_rect;
	TView	*tmp;
	region	*region_1;
	region	*region_2;
	long	origin0_h;
	long	origin0_v;


	a_view->scale = this->scale;
// resize the region to a bigger region since we will have to make
// some complex operation of it.

	//xprintf("add view\n");
	wait_regions();
// TEST
	//if ((request_h != offset_h) || (request_v != offset_v)) {
		//set();
		//do_set_origin(-request_h, -request_v, 1);	//was 1
	//}
// END TEST

// set back the origin to 0, 0 to make computations easier.

	if (owner) {
		owner->get_origin(&origin0_h, &origin0_v);
		owner->set_origin(0 + offset_h, 0 + offset_v);
	}

// offset the bound of the new view by the position of the bound of
// the parent view.

	if ((request_h != offset_h) || (request_v != offset_v)) {
		a_view->offset(bound.left + request_h, bound.top + request_v, 0);
	}
	else {
		a_view->offset(bound.left + offset_h, bound.top + offset_v, 0);
	}
// copy the owner of the parent view in all the views of the new view tree.

	a_view->set_owner(owner);

// setup the parent pointer.

	a_view->parent = this;

	view_bound = a_view->bound;
	
// insert the new view tree at the end of the childs list.

	if (first_child == 0) {
		first_child = a_view;
	}
	else {
		tmp = first_child;
		if (last) {
			while (tmp->next_sibling != 0)
				tmp = tmp->next_sibling;

			tmp->next_sibling = a_view;
		}
		else {
			first_child = a_view;
			a_view->next_sibling = tmp;
		}
	}


	region_1 = newregion();
	region_2 = newregion();
	
// substract the bound of the new view from the visible area.

	if (do_draw) {
		set_region(region_1, &view_bound);
		sub_region(VisibleRegion(), region_1, region_2);
		SetVisibleRegion(region_2);
		if (owner) {
			//owner->inval(&view_bound);
		}
	}

// compute the visible region of the new view.
// = bound of the new view - bound of the parent view

	
	if (last) {
		if (a_view->do_draw) {
			tmp_rect = a_view->min_parent_rect();
			set_region(region_1, &tmp_rect);
			and_region(region_1, a_view->VisibleRegion(), region_2);
			a_view->SetVisibleRegion(region_2);
			if (owner) {
				//owner->inval(&view_bound);
			}
		}
	}
	else {
		set_region(region_1, &view_bound);
		a_view->SetVisibleRegion(region_1);
		tmp = first_child;
		while ((tmp = tmp->next_sibling) != 0) {
			sub_region(tmp->VisibleRegion(), region_1, region_2);
			tmp->SetVisibleRegion(region_1);
		}
	}

// dispose those tmp regions.

	kill_region(region_1);
	kill_region(region_2);

// and set back origins as they were.

	if (owner) {
		owner->set_origin(origin0_h, origin0_v);
		a_view->inval();
		the_server->force_move++;
	}
	signal_regions();
}

/*-------------------------------------------------------------*/

void	TView::set_debug()
{
	rect	clip;
	region	*tmp;
	long	h0;
	long	v0;

// set the port to the right window.

	if (get_port() != (port*)&(owner->window_port)) {
		SERIAL_PRINT(("mismatch %lx %lx\n",
				 get_port(), &(owner->window_port)));
		if (get_port()->window_obj) 
		SERIAL_PRINT(("port1=%s\n",
				 ((TWindow *)(get_port()->window_obj))->window_name));
		SERIAL_PRINT(("port2=%s\n", owner->window_name));
	}

	owner->set();

	clip = bound;
	
// if this is a draw view, then clip to the visible region
// otherwise clip to the view bound.

	if (!do_draw) {
		owner->set_user_clip(&bound);
	}
	else {
		if (user_clip) {
			tmp = newregion();
			offset_region(user_clip, bound.left+offset_h, bound.top+offset_v);
			and_region(user_clip, VisibleRegion(), tmp);
			offset_region(user_clip, -(bound.left+offset_h), -(bound.top+offset_v));
			owner->set_user_clip(tmp);
			kill_region(tmp);
		}
		else
			owner->set_user_clip(VisibleRegion());
	}
	
// set the origin to the view origin.

	owner->get_origin(&h0, &v0);
	owner->set_origin(bound.left + offset_h,
		   	  		  bound.top +  offset_v);
	if (
	   ((bound.left + offset_h) != h0) ||
	   ((bound.top + offset_v) != v0)
	   ) {
		xprintf("w = %lx, nomatch with pid = %ld, boundleft=%ld, h0 = %ld, boundtop = %ld, v0 = %ld\n",
			this, 
			getpid(),
			bound.left,
			h0,
			bound.top,
			v0);
	}
		
}

/*-------------------------------------------------------------*/
		
void	TView::update_clip()
{
	region	*tmp;
		
	if (user_clip && VisibleRegion()) {
		tmp = newregion();
/*
		xprintf("visible %ld %ld %ld %ld\n", visible->bound.top,
											visible->bound.left,
											visible->bound.bottom,
											visible->bound.right);
		xprintf("user %ld %ld %ld %ld\n", user_clip->bound.top,
											user_clip->bound.left,
											user_clip->bound.bottom,
											user_clip->bound.right);
*/
		offset_region(user_clip, bound.left+offset_h, bound.top+offset_v);
		and_region(user_clip, VisibleRegion(), tmp);
		offset_region(user_clip, -(bound.left+offset_h), -(bound.top+offset_v));
		owner->set_user_clip(tmp);
		kill_region(tmp);
	}
}

/*-------------------------------------------------------------*/

void	TView::set()
{
	rect	clip;
	region	*tmp;

// set the port to the right window.

	wait_regions();
	owner->set();

	clip = bound;
	
// if this is a draw view, then clip to the visible region
// otherwise clip to the view bound.

	if (!do_draw) {
		owner->set_user_clip(&bound);
	}
	else {
		if (user_clip) {
			tmp = newregion();
			offset_region(user_clip, bound.left+offset_h, bound.top+offset_v);
			and_region(user_clip, VisibleRegion(), tmp);
			offset_region(user_clip, -(bound.left+offset_h), -(bound.top+offset_v));
			owner->set_user_clip(tmp);
			kill_region(tmp);
		}
		else
			owner->set_user_clip(VisibleRegion());
	}
	
// set the origin to the view origin.

	owner->set_origin(bound.left + offset_h,
		   	  		  bound.top +  offset_v);
	signal_regions();
}

/*-------------------------------------------------------------*/
// reset origins will check for views that have a requested origin
// different from the current origin.
// This situation occurs if the origin of a view has been modified
// while that view was in update.
// reset_origins is called after every update.

void	TView::reset_origins()
{
	TView	*a_child;

	a_child = first_child;

	while (a_child) {
		a_child->reset_origins();
		a_child = a_child->next_sibling;
	}
	

	if (do_draw) {
		if ((request_h != offset_h) || (request_v != offset_v)) {
			set();
			do_set_origin(-request_h, -request_v, 0);	//was 1
		}
	}
}

/*-------------------------------------------------------------*/
// update will setup the check for an intersection between
// the update region and the view bound and will exit if none is found.
// Otherwise the draw method will be called for the whole view tree


void	TView::update(region *upd_region)
{
	rect	clip;
	TView	*a_child;

// check the intersection with the update region and exit if none.

	if (!rect_in_region(upd_region, &bound))
		return;

	a_child = first_child;

// recursive call of update for the whole tree.

	while (a_child) {
		a_child->update(upd_region);
		a_child = a_child->next_sibling;
	}
	

// if draw view,

	if (do_draw) {
		saved_clip = user_clip;
		user_clip = 0;

		clip = bound;

// set the clip to visible & update.

		owner->set_user_clip(VisibleRegion());

// set the origin of the window to the origin of the view.

		owner->set_origin(bound.left + offset_h,
		   	   	bound.top +  offset_v);
	
// and call the draw stuff.

		draw();

		if (user_clip)
			kill_region(user_clip);

		user_clip = saved_clip;
	}
}

/*-------------------------------------------------------------*/

void	TView::draw()
{
}

/*-------------------------------------------------------------*/

char	TView::click_up(button_event info)
{
	
	return(0);
}

/*-------------------------------------------------------------*/

char	TView::click(button_event info)
{
	
	return(0);
}

/*-------------------------------------------------------------*/

char	TView::force_process_click_up(button_event info)
{
	TView	*a_child;
	char	taken;

	info.h -= (bound.left + offset_h);
	info.v -= (bound.top + offset_v);

	taken = click_up(info);
	return(taken);
}

/*-------------------------------------------------------------*/
// process_click will search for the smallest view which encloses
// the click.

char	TView::process_click_up(button_event info)
{
	TView	*a_child;
	char	taken;

	if (!point_in_rect(&bound, info.h, info.v))
		return(0);

	a_child = first_child;
	while (a_child) {
		if (a_child->process_click_up(info))
			return(1);
		a_child = a_child->next_sibling;
	}

	info.h -= (bound.left + offset_h);
	info.v -= (bound.top + offset_v);

	taken = click_up(info);
	return(taken);
}

/*-------------------------------------------------------------*/
// process_click will search for the smallest view which encloses
// the click.

char	TView::process_click(button_event info)
{
	TView	*a_child;
	char	taken;

// if not in this part of the tree, exit.

	if (!point_in_rect(&bound, info.h, info.v))
		return(0);

	a_child = first_child;

// try the click in all the sub-tree by recursion.

	while (a_child) {
		if (a_child->process_click(info))
			return(1);
		a_child = a_child->next_sibling;
	}


// if found, set the port and pass the click to the view.

	info.h -= bound.left;
	info.v -= bound.top;

	info.h -= offset_h;
	info.v -= offset_v;

	the_server->last_view_md = this;
	taken = click(info);
	return(taken);
}

/*-------------------------------------------------------------*/
// return the bound of the view in local coordinates.

void	TView::get_bound(rect *bnd)
{
	bnd->top    = -offset_v;
	bnd->left   = -offset_h;
	bnd->right  = (bound.right - bound.left) - offset_h;
	bnd->bottom = (bound.bottom - bound.top) - offset_v;
}

/*-------------------------------------------------------------*/
// return the view location relative the the parent bound.

void	TView::get_location(rect* loc)
{
	if (parent) {
		loc->top = bound.top - parent->bound.top;
		loc->left = bound.left - parent->bound.left;
		loc->bottom = loc->top + (bound.bottom - bound.top);
		loc->right = loc->left + (bound.right - bound.left);
	}
	else
		*loc = bound;
}

/*-------------------------------------------------------------*/

char	TView::can_draw()
{
	return(do_draw);
}

/*-------------------------------------------------------------*/
// recurs. set the owner of a view tree to <the_owner>.

void	TView::set_owner(TWindow *the_owner)
{
	TView	*a_child;

	owner = the_owner;
	
	a_child = first_child;

	while (a_child) {
		a_child->set_owner(the_owner);
		a_child = a_child->next_sibling;
	}
}

/*-------------------------------------------------------------*/
// offset the regions and bound of a view tree by <dv>, <dh>

void	TView::offset(long dh, long dv, char redo_regions)
{
	TView	*a_child;

	offset_rect(&bound, dh, dv);	
	if (do_draw) grOffsetContext(((MView*)this)->renderContext, dh, dv);

	a_child = first_child;

	while (a_child) {
		a_child->offset(dh, dv, redo_regions);
		a_child = a_child->next_sibling;
	}

	if (redo_regions) {
		recalc_visible(0);
	}
}

/*-------------------------------------------------------------*/
// recalc the bound of the view from the changes in the parent
// view bound and the resizing rules.

rect	TView::recalc_bound(rect old_parent_rect, rect new_parent_rect, char move_case)
{
	long	delta_top;
	long	delta_left;
	long	delta_bottom;
	long	delta_right;
	long	old_vcenter;
	long	old_hcenter;
	long	new_vcenter;
	long	new_hcenter;
	rect	new_bound;

// calc the delta of the top/left/bottom/right of the parent bound.

	delta_top		= new_parent_rect.top - old_parent_rect.top;
	delta_left		= new_parent_rect.left - old_parent_rect.left;
	delta_bottom	= new_parent_rect.bottom - old_parent_rect.bottom;
	delta_right		= new_parent_rect.right - old_parent_rect.right;


	new_bound = bound;
	if (move_case & 1) {
		new_bound.top += delta_top;
		new_bound.bottom += delta_top;
		new_bound.left += delta_left;
		new_bound.right += delta_left;
		return(new_bound);
	}
	
	// The top-level view is special. It can be moved and resize at the same time
	// because of a fullscreen Move/Size window transition.
	if (move_case & 2) {
		delta_bottom -= delta_top;
		delta_right -= delta_left;
		delta_top = 0;
		delta_left = 0;
	}

// process the top.

	switch((rules >> 12) & 0xf) {
		case	POS_TOP		: new_bound.top += delta_top;break;
		case	POS_BOTTOM	: new_bound.top += delta_bottom;break;
		case	POS_CENTER	:
			new_vcenter	= (new_parent_rect.top + new_parent_rect.bottom) / 2;
			old_vcenter	= (old_parent_rect.top + old_parent_rect.bottom) / 2;
			new_bound.top = new_vcenter + (bound.top - old_vcenter);
			break;
	}

// process the bottom.
	
	switch((rules >> 4) & 0xf) {
		case	POS_TOP		: new_bound.bottom += delta_top;break;
		case	POS_BOTTOM	: new_bound.bottom += delta_bottom;break;
		case	POS_CENTER	:
			new_vcenter	= (new_parent_rect.top + new_parent_rect.bottom) / 2;
			old_vcenter	= (old_parent_rect.top + old_parent_rect.bottom) / 2;
			new_bound.bottom = new_vcenter + (bound.bottom - old_vcenter);
			break;
	}

// process the left.

	switch((rules >> 8) & 0xf) {
		case	POS_LEFT	: new_bound.left += delta_left;break;
		case	POS_RIGHT	: new_bound.left += delta_right;break;
		case	POS_CENTER	:
			new_hcenter	= (new_parent_rect.left + new_parent_rect.right) / 2;
			old_hcenter	= (old_parent_rect.left + old_parent_rect.right) / 2;
			new_bound.left = new_hcenter + (bound.left - old_hcenter);
			break;
	}

// process the right.

	switch(rules & 0xf) {
		case	POS_LEFT	: new_bound.right += delta_left;break;
		case	POS_RIGHT	: new_bound.right += delta_right;break;
		case	POS_CENTER	:
			new_hcenter	= (new_parent_rect.left + new_parent_rect.right) / 2;
			old_hcenter	= (old_parent_rect.left + old_parent_rect.right) / 2;
			new_bound.right = new_hcenter + (bound.right - old_hcenter);
			break;
	}
	
/*	xprintf("old parent [%d,%d,%d,%d]\n",
			old_parent_rect.left, old_parent_rect.top, old_parent_rect.right, old_parent_rect.bottom);
	xprintf("new parent [%d,%d,%d,%d]\n",
			new_parent_rect.left, new_parent_rect.top, new_parent_rect.right, new_parent_rect.bottom);
	xprintf("old bound [%d,%d,%d,%d]\n",
			bound.left, bound.top, bound.right, bound.bottom);
	xprintf("new bound [%d,%d,%d,%d]\n",
			new_bound.left, new_bound.top, new_bound.right, new_bound.bottom);*/
	return(new_bound);
}

/*-------------------------------------------------------------*/

void	TView::inval(rect *r)
{
	rect	r1;
	region	*tmp;
	region	*tmp1;
	region	*tmp2;

	if (!do_draw)
		return;

	if (owner) {
		int32 origin_v,origin_h;
		owner->get_origin(&origin_h, &origin_v);
		owner->set_origin(0, 0);

		if (VisibleRegion()->count == 1) {
			r1 = *r;
			offset_rect(&r1, bound.left + offset_h, bound.top + offset_v);	
			sect_rect(VisibleRegion()->bound, r1, &r1);
			owner->inval(&r1);
		} else {
			tmp = newregion();
			tmp1 = newregion();
			tmp2 = newregion();
			copy_region(VisibleRegion(), tmp);
			set_region(tmp1, r);
			offset_region(tmp1, bound.left + offset_h, bound.top + offset_v);
			and_region(tmp1, tmp, tmp2);
			owner->inval(tmp2);
			kill_region(tmp);
			kill_region(tmp1);
			kill_region(tmp2);
		}

		owner->set_origin(origin_h, origin_v);
	}
}

/*-------------------------------------------------------------*/
// inval the whole content of a view.

void	TView::inval()
{
	long	origin_h;
	long	origin_v;
	
	if (owner) {
		owner->get_origin(&origin_h, &origin_v);
		owner->set_origin(0, 0);
		owner->inval(&bound);
		owner->set_origin(origin_h, origin_v);
	}
}

/*-------------------------------------------------------------*/
// recalc the visible region of the view and inval the changes
// from the previous visible region if <do_update>.

void	TView::recalc_visible(char do_update)
{
	region	*tmp_region1;
	region	*tmp_region2;
	region	*copy;
	rect	tmp;
	long	origin_h;
	long	origin_v;

	// new visible region = (<bound> && <parent_bound>) - sum of all(childs bounds)

	if (!do_draw)
		return;

	owner->get_origin(&origin_h, &origin_v);
	owner->set_origin(0, 0);

	if (do_update) {
		copy = newregion();
		copy_region(VisibleRegion(), copy);
	}

	region *newVisible = newregion();
	set_region(newVisible, &bound);
	tmp_region1 = newregion();
	tmp_region2 = newregion();

	if (parent) {
		tmp = min_parent_rect();
		set_region(tmp_region1, &tmp);
		and_region(tmp_region1, newVisible, tmp_region2);
		copy_region(tmp_region2, newVisible);
	} 

	sub_all_childs(tmp_region1, tmp_region2, newVisible);
	SetVisibleRegion(newVisible);
	kill_region(newVisible);

// compute the differences with the previous visible region.
// and inval them.

	if (do_update) {
		sub_region(VisibleRegion(), copy, tmp_region1);
		if (!empty_region(tmp_region1)) {
   			owner->inval(tmp_region1);	
		}
		kill_region(copy);
	}

	kill_region(tmp_region1);
	kill_region(tmp_region2);

	owner->set_origin(origin_h, origin_v);
}

/*-------------------------------------------------------------*/

void	TView::send_movesize_event(rect *old, rect *newr)
{
	long	new_width;
	long	new_height;
	BParcel	a_parcel;

	//if (owner->check_event_port()) {
		a_parcel.AddInt64(system_time(), "when");
		a_parcel.SetTarget(client_token);
		if ((old->left != newr->left) || (old->top != newr->top)) {
			a_parcel.AddPoint(BPPoint(newr->left, newr->top), "where");
			a_parcel.SetWhat(B_VIEW_MOVED);
			owner->send_event(&a_parcel);
		}

		new_width = newr->right - newr->left;
		new_height = newr->bottom - newr->top;

		if (((old->bottom - old->top) != new_height) ||
		    ((old->right - old->left) != new_width)) {
			a_parcel.AddInt32(new_width, "width");
			a_parcel.AddInt32(new_height, "height");
			a_parcel.SetWhat(B_VIEW_RESIZED);
			owner->send_event(&a_parcel);
		}
	//}
}

/*-------------------------------------------------------------*/

void	TView::sub_all_childs(region *tmp_region1, region *tmp_region2, region *subtractFrom)
{
	TView	*a_child;

	a_child = first_child;

	while (a_child) {
		if (overlap_rect(bound, a_child->bound)) {
			set_region(tmp_region1, &(a_child->bound)); 
			sub_region(subtractFrom, tmp_region1, tmp_region2);
			copy_region(tmp_region2, subtractFrom);
		}
		a_child = a_child->next_sibling;
	}
}

#if 0
/*-------------------------------------------------------------*/
// do_bound will update the state of a hierarchy of views
// according to the changes of a parent view rectangle from
// old_pr to new_pr.
// If parent_size, the old_pr, new_pr are the rectangles of
// the parent view of TView. Else this is the rectangles of
// the view itself.

void	TView::do_bound(rect old_pr, rect new_pr, char parent_size, char move_case)
{
	rect	old_bound;
	rect	old_bound1;
	rect	new_bound;
	rect	tmp_rect;
	TView	*a_child;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	region	*tmp_region4;
	long	origin_h;
	long	origin_v;

// Save the origin and go back to a zero, zero origin system

	owner->last_view_token = -1;
	owner->get_origin(&origin_h, &origin_v);
	owner->set_origin(0, 0);

// If the rectangles are of the parent view, the calc the
// new rectangles of the son we are working on.
// To do that, call the recalc_bound method which uses the
// sizing rules to compute this rectangle.

	if (parent_size) {
		old_bound = bound;
		new_bound = recalc_bound(old_pr, new_pr, move_case);
	}
	else {
// Else we've got the rectangles already.
		old_bound = old_pr;
		new_bound = new_pr;
	}


// Only work if the rectangles have really changed.

	old_bound1 = bound;
	bound = new_bound;

	if (rules & NEED_SIZE) {
		if (!equal_rect(&old_bound, &new_bound)) {
			send_movesize_event(&old_bound, &new_bound);
		}
	}

	if (1 || (!equal_rect(&old_bound, &new_bound))) {

// Get ready to scan the list of child of the view.

		a_child = first_child;

		while (a_child) {
// Call recalc bound for all the sons of the view.
			a_child->do_bound(old_bound, new_bound, 1, move_case);
			a_child = a_child->next_sibling;
		}

// if used for drawing, we've got to recalc the region
	
		if (do_draw) {
			tmp_region1 = newregion();
			tmp_region2 = newregion();

// If the view is a full draw or if the top or left have moved
// we need to redraw the whole view.

			if ((rules & FULL_DRAW) &&
			    (vs(old_bound) != vs(new_bound) ||
			    hs(old_bound) != hs(new_bound))
			   ) {
				owner->inval(&old_bound);
			}

// We now recompute the visible region for the view.
// This region in the bound of the view minus the bound of
// all the sub_views of this view.
// We don't need to go any lower in the hierarchy since
// views are supposed to stay enclosed in their parent bound.

			set_region(visible, &new_bound);

// This is were we do the sub.

			sub_all_childs(tmp_region1, tmp_region2);

// Now could be a good time to <and> the result region with the
// bound of the parent region.

			if (parent_size) {
				//set_region(tmp_region1, &new_pr);
				tmp_rect = min_parent_rect();
				set_region(tmp_region1, &tmp_rect);
				and_region(tmp_region1, visible, tmp_region2);
			}
			else {
				if (parent) {
					tmp_rect = min_parent_rect();
					set_region(tmp_region1, &tmp_rect);
					and_region(tmp_region1, visible, tmp_region2);
				}
			}

			copy_region(tmp_region2, visible);

// If the view is a full draw or if the top or left have changed
// we need to redraw the whole view.

			if ((rules & FULL_DRAW) ||
			    (old_bound.top != new_bound.top) ||
			    ( old_bound.left != new_bound.left)) {
				if ((rules & FAST) == 0) {
					owner->inval(visible);
					set_region(tmp_region1, &old_bound);
					owner->inval(tmp_region1);
				}
			}
			else {
// Otherwise we only need to redraw the xor of the old and new visible
// regions.

				tmp_region4 = newregion();
				set_region(tmp_region1, &old_bound1);
				set_region(tmp_region2, &new_bound);
				tmp_region3 = newregion();
				sub_region(tmp_region2, tmp_region1, tmp_region4);
				sub_region(tmp_region1, tmp_region2, tmp_region3);
				or_region(tmp_region4, tmp_region3, tmp_region1);
				owner->inval(tmp_region1);	
				kill_region(tmp_region3);
				kill_region(tmp_region4);
			}		
// We don't need those regions anymore.

			kill_region(tmp_region1);
			kill_region(tmp_region2);

		}

		// If this is a "FAST" view, call the drawing method here and validate
		// the region used by the view.
		if (!equal_rect(&old_bound, &new_bound)) {
			if ((rules & FAST) != 0) {
				if (owner->InvalidRegion()) {
					debugger (NULL);
					dprintf("should not append\n");
				}
				set(); draw();
				owner->set_origin(0, 0);
				owner->valid(&new_bound);
			}
		}
	}

// Restore the origin to the previous value.
// !! It could be smart the get that out of the recursion
// and only to save the origin once.
// &&&
	owner->set_origin(origin_h, origin_v);
}
#endif

//if 0
void	TView::do_bound(rect old_pr, rect new_pr, char parent_size, char move_case)
{
	wait_regions();
	region *newBad = newregion();
	int32 xoffs = owner->ClientRegion()->bound.left;
	int32 yoffs = owner->ClientRegion()->bound.top;
	if (owner->bad_region) {
		copy_region(owner->bad_region,newBad);
	};
	offset_region(newBad,-xoffs,-yoffs);
	do_bound(old_pr,new_pr,parent_size,move_case,newBad,NULL,NULL);
	offset_region(newBad,xoffs,yoffs);
	if (!owner->bad_region) {
		if (newBad->count) {
			owner->bad_region = newregion();
			and_region(newBad,owner->ClientRegion(),owner->bad_region);
		};
	} else {
		and_region(newBad,owner->ClientRegion(),owner->bad_region);
	};
	kill_region(newBad);
	signal_regions();
};
//endif

void	TView::do_bound(rect old_pr, rect new_pr, char parent_size, char move_case,
						region *needToDraw, region *needToClear,
						BArray<TView*> *localViews)
{
	rect	old_bound;
	rect	old_bound1;
	rect	new_bound;
	rect	tmp_rect;
	TView	*a_child;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	region	*tmp_region4;
	int32 originx,originy;

	if (parent_size) {
		old_bound = bound;
		new_bound = recalc_bound(old_pr, new_pr, move_case);
	} else {
		old_bound = old_pr;
		new_bound = new_pr;
	}

	old_bound1 = bound;
	bound = new_bound;

	if (rules & NEED_SIZE) {
		if (!equal_rect(&old_bound, &new_bound)) {
			send_movesize_event(&old_bound, &new_bound);
		}
	}

	if (1 || (!equal_rect(&old_bound, &new_bound))) {
		a_child = first_child;
		while (a_child) {
			a_child->do_bound(old_bound, new_bound, 1, move_case&1, needToDraw, needToClear, localViews);
			a_child = a_child->next_sibling;
		}

		region	*humRegion = newregion();
		region	*rectRegion = newregion();

		if (do_draw) {
			tmp_region1 = newregion();
			tmp_region2 = newregion();
			if ((rules & FULL_DRAW) &&
			    (vs(old_bound) != vs(new_bound) ||
			    hs(old_bound) != hs(new_bound))) {
			    if (needToDraw) {
				    set_region(rectRegion,&old_bound);
					copy_region(needToDraw,humRegion);
					or_region(humRegion,rectRegion,needToDraw);
				};
			}
			
			region *newVisible = newregion();
			set_region(newVisible, &new_bound);
			sub_all_childs(tmp_region1, tmp_region2, newVisible);
			
			if (parent_size) {
				tmp_rect = min_parent_rect();
				set_region(tmp_region1, &tmp_rect);
				and_region(tmp_region1, newVisible, tmp_region2);
			} else {
				if (parent) {
					tmp_rect = min_parent_rect();
					set_region(tmp_region1, &tmp_rect);
					and_region(tmp_region1, newVisible, tmp_region2);
				}
			}
			SetVisibleRegion(tmp_region2);
			kill_region(newVisible);

			if ((rules & FULL_DRAW) ||
			    (old_bound.top != new_bound.top) ||
			    ( old_bound.left != new_bound.left)) {
				if ((rules & FAST) == 0) {
				    if (needToDraw) {
					    set_region(rectRegion,&old_bound);
						or_region(needToDraw,VisibleRegion(),humRegion);
						or_region(humRegion,rectRegion,needToDraw);
					};
				}
			}
			else {
			    if (needToDraw) {
					tmp_region4 = newregion();
					set_region(tmp_region1, &old_bound1);
					set_region(tmp_region2, &new_bound);
					tmp_region3 = newregion();
					sub_region(tmp_region2, tmp_region1, tmp_region4);
					sub_region(tmp_region1, tmp_region2, tmp_region3);
					or_region(tmp_region4, tmp_region3, tmp_region1);
					copy_region(needToDraw,humRegion);
					or_region(humRegion,tmp_region1,needToDraw);
					kill_region(tmp_region3);
					kill_region(tmp_region4);
				};
			}		
			kill_region(tmp_region1);
			kill_region(tmp_region2);
		}
		if (!equal_rect(&old_bound, &new_bound)) {
			if ((rules & FAST) != 0) {
				if (localViews)
					localViews->AddItem(this);
				else {
					if (clippedToClient) {
						int32 xoffs = owner->ClientRegion()->bound.left;
						int32 yoffs = owner->ClientRegion()->bound.top;
						offset_region(owner->ClientRegion(),-xoffs,-yoffs);
						set_region(humRegion,&new_bound);
						and_region(owner->ClientRegion(),humRegion,rectRegion);
						offset_region(owner->ClientRegion(),xoffs,yoffs);
					} else {
						set_region(rectRegion,&new_bound);
					};
				    if (needToClear) {
	//					copy_region(needToClear,humRegion);
	//					sub_region(humRegion,rectRegion,needToClear);
					};
				    if (needToDraw) {
						copy_region(needToDraw,humRegion);
						or_region(humRegion,rectRegion,needToDraw);
	//					sub_region(humRegion,rectRegion,needToDraw);
	//					owner->valid(&new_bound);
					};
				};
			}
		}		
		kill_region(humRegion);
		kill_region(rectRegion);
	}
}

/*-------------------------------------------------------------*/
// will move a view in the parent space and will recalc the
// position of all the sub-views if needed.

void	TView::move_view(long dh, long dv)
{
	rect	old_bound;
	rect	new_bound;

// do we really need to work here ?

	if ((dh == 0) && (dv == 0))
		return;

	wait_regions();

	old_bound = bound;
	new_bound = bound;
	
// change the rect position.

	offset_rect(&new_bound, dh, dv);	

// recalc the new position of the sub-views.

	do_bound(old_bound, new_bound, 0, 1);

// if the view is not the top of the tree, the visible region
// of the parent view needs to be changed.

	if (parent) parent->recalc_visible(1);

	the_server->force_move++;
	signal_regions();
}

/*-------------------------------------------------------------*/
// quite the same as move_view

void	TView::moveto_view(long h, long v)
{
	rect	old_bound;
	rect	new_bound;
	long	p_top;
	long	p_left;

	wait_regions();
	old_bound = bound;
	
	if (parent == 0) {
		p_top = 0;
		p_left = 0;
	}
	else {
		p_top = parent->bound.top;
		p_left = parent->bound.left; 
	}

	new_bound.top = p_top + v;
	new_bound.left = p_left + h;
	new_bound.bottom = new_bound.top + (old_bound.bottom - old_bound.top);
	new_bound.right = new_bound.left + (old_bound.right - old_bound.left);

	do_bound(old_bound, new_bound, 0, 1);

	if (parent) parent->recalc_visible(1);

	the_server->force_move++;
	signal_regions();
}

/*-------------------------------------------------------------*/
// will size a view in the parent space and will recalc the
// position of all the sub-views if needed.

void	TView::size_view(long dh, long dv)
{
	rect	old_bound;
	rect	new_bound;

	if ((dh == 0) && (dv == 0))
		return;

	wait_regions();

	old_bound = bound;
	new_bound = bound;
	
	new_bound.bottom += dv;
	new_bound.right += dh;

	do_bound(old_bound, new_bound, 0, 0);

// if the view is not the top of the tree, the visible region
// of the parent view needs to be changed.
	
	if (parent) parent->recalc_visible(1);

	the_server->force_move++;
	signal_regions();
}

/*-------------------------------------------------------------*/

void	TView::sizeto_view(long h, long v)
{
	rect	old_bound;
	rect	new_bound;

	wait_regions();

	old_bound = bound;
	new_bound = bound;
	
	new_bound.bottom = old_bound.top + v;
	new_bound.right = old_bound.left + h;

	do_bound(old_bound, new_bound, 0, 0);

	if (parent) parent->recalc_visible(1);

	the_server->force_move++;
	signal_regions();
}

/*-------------------------------------------------------------*/

TView	*TView::get_sibling()
{
	return(next_sibling);
}

/*-------------------------------------------------------------*/

TView	*TView::get_parent()
{
	return(parent);
}

/*-------------------------------------------------------------*/

TView	*TView::get_child(long index)
{
	TView	*child;

	child = first_child;

	while(index > 0) {
		if (child == 0)
			return(child);
		child = child->next_sibling;
		index--;
	}
	return(child);
}

/*-------------------------------------------------------------*/
// returns -1 if view being passed in is nil or if it doesn't exist
// as a subview of this view

long	TView::get_index(TView* view)
{
	long index = 0;

	if (view == 0)
		return(-1);

	TView* child = first_child;
	while (child) {
		if (child == view)
			return(index);

		child = child->next_sibling;
		index++;
	}

	return(-1);
}

/*-------------------------------------------------------------*/

long	TView::count_child()
{
	long	count;
	TView	*child;

	count = 0;
	
	child = first_child;

	while(child) {
		count++;
		child = child->next_sibling;
	}
	return(count);
}

/*-------------------------------------------------------------*/
// return the bound of the view in screen coordinates.

void	TView::get_global_bound(rect *r)
{
	rect	tmp;
	long	dh;
	long	dv;

	wait_regions();
	tmp = bound;
	set();
	offset_rect(&tmp, +offset_h, +offset_v);	
	
	dh = owner->ClientRegion()->bound.left;
	dv = owner->ClientRegion()->bound.top;

	offset_rect(&tmp, dh, dv);	
	signal_regions();
	*r = tmp;
}

/*-------------------------------------------------------------*/
// convert from screen coordinates to view coordinates.

void	TView::global_to_local(long *h, long *v)
{
	long	dh;
	long	dv;
	
	wait_regions();
	set();
	dh = owner->ClientRegion()->bound.left;
	dv = owner->ClientRegion()->bound.top;
	dh += bound.left + offset_h;
	dv += bound.top + offset_v;
	*h -= dh;
	*v -= dv;

	signal_regions();
}

/*-------------------------------------------------------------*/
// convert from screen coordinates to view coordinates.

void	TView::global_to_local(float *h, float *v)
{
	long	dh;
	long	dv;
	
	wait_regions();
	set();
	dh = owner->ClientRegion()->bound.left;
	dv = owner->ClientRegion()->bound.top;
	dh += bound.left + offset_h;
	dv += bound.top + offset_v;
	*h -= dh;
	*v -= dv;

	signal_regions();
}

/*-------------------------------------------------------------*/
// convert from view coordinates to screen coordinates

void	TView::local_to_global(long *h, long *v)
{
	long	dh;
	long	dv;
	
	wait_regions();
	dh = owner->ClientRegion()->bound.left;
	dv = owner->ClientRegion()->bound.top;
	dh += bound.left;
	dv += bound.top;
	dh += offset_h;
	dv += offset_v;
	*h += dh;
	*v += dv;

	signal_regions();
}

/*-------------------------------------------------------------*/
// convert from view coordinates to screen coordinates

void	TView::local_to_global(float *h, float *v)
{
	long	dh;
	long	dv;
	
	wait_regions();
	dh = owner->ClientRegion()->bound.left;
	dv = owner->ClientRegion()->bound.top;
	dh += bound.left;
	dv += bound.top;
	dh += offset_h;
	dv += offset_v;
	*h += dh;
	*v += dv;

	signal_regions();
}

/*-------------------------------------------------------------*/
// convert from view coordinates to screen coordinates.

void	TView::local_to_global(point *pt)
{
	local_to_global(&(pt->h), &(pt->v));
}

/*-------------------------------------------------------------*/
// convert from view coordinates to screen coordinates.

void	TView::local_to_global(rect *r)
{
	local_to_global(&(r->left), &(r->top));
	local_to_global(&(r->right), &(r->bottom));
}

/*-------------------------------------------------------------*/
// convert from view coordinates to screen coordinates.

void	TView::local_to_global(frect *r)
{
	local_to_global(&(r->left), &(r->top));
	local_to_global(&(r->right), &(r->bottom));
}

/*-------------------------------------------------------------*/
// convert from screen coordinates to view coordinates.

void	TView::global_to_local(point *pt)
{
	global_to_local(&(pt->h), &(pt->v));
}

/*-------------------------------------------------------------*/
// convert from screen coordinates to view coordinates.

void	TView::global_to_local(rect *r)
{
	global_to_local(&(r->left), &(r->top));
	global_to_local(&(r->right), &(r->bottom));
}

/*-------------------------------------------------------------*/
// convert from screen coordinates to view coordinates.

void	TView::global_to_local(frect *r)
{
	global_to_local(&(r->left), &(r->top));
	global_to_local(&(r->right), &(r->bottom));
}

/*-------------------------------------------------------------*/
// will look for a child view enclosing the h, v position.
// h, v are expressed in the view coordinate system.

TView	*TView::find_view(long h, long v)
{
	TView	*child;
	TView	*result;

	if (!point_in_rect(&bound, h, v))
		return(0);
	
	child = first_child;
	while(child) {
		result = child->find_view(h, v);
		
		if (result != 0)
			return(result);

		child = child->next_sibling;
	}

	return(this);
}

/*-------------------------------------------------------------*/
// change the coordinate system of the view so that the
// origin of the view is at h,v.
// The content of the view will be scrolled so that the
// previous content matches the new coordinate system.
// All exposed part generates an update.
// !! I don't know yet how this will interact with a view
// busy in the middle of an update.
// All sub_views are also offseted by the change in the
// coordinate system of the parent view.


void	TView::do_set_origin(float h, float v)
{
	do_set_origin(h, v, 0);
}

/*-------------------------------------------------------------*/
// change the origin of the view to h, v and force screen feedback
// if force is true.
// If force is false, enqueue the origin change and do it at the next
// "good" time (when no update are pending).
// Thank to the queue system, multiple set_origin can be catenate
// in one graphic operation.
/* old version

void	TView::do_set_origin(float h, float v, char force)
{
	long	dh;
	long	dv;
	rect	r1;
	rect	r2;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	region	*tmp_bad;
	TView	*a_son;

	wait_regions();


	set();

	if ((!empty_region(owner->bad_region)) ||
	    (owner->saved_bad)) {

		if (!force) {
			request_h = -h;
			request_v = -v;
			signal_regions();
			return;
		}
	}


// This is by how much we will change the origin of this view.
// Could be smart to check for 0, 0. You never know what
// people will do to waste time.

	dh = h + offset_h;
	dv = v + offset_v;
	
	if (dh || dv) {

// change the origin of the view.

		offset_h = (-h);
		offset_v = (-v);

		request_h = (-h);
		request_v = (-v);

// working with a zero origin for the scrolling will make my
// life easier.

		owner->set_origin(0, 0);

// setup source and destination rects.

		tmp_region1 = newregion();
		tmp_region2 = newregion();
		tmp_region3 = newregion();


		if (!empty_region(owner->bad_region)) {
			copy_region(owner->bad_region, tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->bad_region, tmp_region2);
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			copy_region(visible, tmp_region1);
			offset_region(tmp_region1,
			      	owner->window_port.origin.h,
			      	owner->window_port.origin.v);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			copy_region(owner->bad_region, tmp_region1);
			
			or_region(tmp_region1, tmp_region2, owner->bad_region);
			if (owner->bad_region->count > 40) 
				simplify_region(owner->bad_region);
		}

		if (owner->saved_bad) {
			copy_region(owner->saved_bad, tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->saved_bad, tmp_region2);
			

			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			copy_region(visible, tmp_region1);
			offset_region(tmp_region1,
			      	owner->window_port.origin.h,
			      	owner->window_port.origin.v);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			copy_region(owner->saved_bad, tmp_region1);
			
			or_region(tmp_region1, tmp_region2, owner->saved_bad);
			if (owner->bad_region->count > 40) 
				simplify_region(owner->saved_bad);
		}

		copy_region(owner->window_port.draw_region,
			    tmp_region1);

		offset_region(tmp_region1,
			      -owner->window_port.origin.h,
			      -owner->window_port.origin.v);


		r1 = bound;
		r2 = r1;
		offset_rect(&r2, dh, dv);


		if (dv > 0) {
			r2.bottom -= dv;
			r1.bottom -= dv;
		}

		if (dv < 0) {
			r2.top -= dv;
			r1.top -= dv;
		}


// blit them.

		tmp_bad = 0;

		if (owner->saved_bad) {
			tmp_bad = owner->saved_bad;
			owner->saved_bad = 0;
			owner->recalc_clip();
		}

		blit_ns((port *)&(owner->window_port),
	     	     (port *)&(owner->window_port),
	     	     &r2,
	     	     &r1,
	     	     op_copy);

		if (tmp_bad) {
			owner->saved_bad = tmp_bad;
			owner->recalc_clip();
		}
		
		copy_region(tmp_region1, tmp_region2);

		offset_region(tmp_region2,
			      -dh,
			      -dv);

		sub_region(tmp_region1, tmp_region2, tmp_region3);
		owner->inval(tmp_region3);

		kill_region(tmp_region1);
		kill_region(tmp_region2);
		kill_region(tmp_region3);
	}


// I should probably see if the view that was set before
// is not the one for which is just change the coordinate
// system.
// In that case, i should not use the set_origin.



// set back the context as it was before my stuff.

	owner->set_origin(bound.left + offset_h,
	   	  	  bound.top +  offset_v);
	owner->recalc_clip();

// Now we need to offset all the sub views bounds.

	
	a_son = first_child;
	while(a_son) {
		a_son->offset(-dh, -dv, 0);
		a_son = a_son->next_sibling;
	}

	signal_regions();
}
*/
// new version which will also move the sub-views

void	TView::do_set_origin(float h, float v, char force)
{
	long	dh;
	long	dv;
	rect	r1;
	rect	r2;
	rect	parent_bound;
	rect	vv_bound;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	region	*tmp_bad;
	TView	*a_son;

	force = 1;
	wait_regions();
	if ((!empty_region(owner->bad_region)) ||
	    (owner->InvalidRegion())) {
		if (!force) {
			request_h = -h;
			request_v = -v;
			signal_regions();
			return;
		}
	}

	set();
	parent_bound = min_parent_rect();
	sect_rect(parent_bound, bound, &vv_bound);
	owner->set_user_clip(&vv_bound);

// This is by how much we will change the origin of this view.
// Could be smart to check for 0, 0. You never know what
// people will do to waste time.

	dh = h + offset_h;
	dv = v + offset_v;
	
	if (dh || dv) {

// change the origin of the view.

		offset_h = (-h);
		offset_v = (-v);

		request_h = (-h);
		request_v = (-v);

// working with a zero origin for the scrolling will make my
// life easier.

		owner->set_origin(0, 0);

// setup source and destination rects.

		tmp_region1 = newregion();
		tmp_region2 = newregion();
		tmp_region3 = newregion();

		if (!empty_region(owner->bad_region)) {
			//printf("bad1\n");
			copy_region(owner->bad_region, tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->bad_region, tmp_region2);
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			set_region(tmp_region1, &vv_bound);
			
			offset_region(tmp_region1,
			      	owner->window_port.origin.h,
			      	owner->window_port.origin.v);

			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			copy_region(owner->bad_region, tmp_region1);
			
			or_region(tmp_region1, tmp_region2, owner->bad_region);
			if (owner->bad_region->count > 40) 
				simplify_region(owner->bad_region);
		}

		if (owner->InvalidRegion()) {
			//printf("bad2\n");
			copy_region(owner->InvalidRegion(), tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->InvalidRegion(), tmp_region2);
			
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			set_region(tmp_region1, &vv_bound);

			offset_region(tmp_region1,
			      	owner->window_port.origin.h,
			      	owner->window_port.origin.v);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			or_region(owner->InvalidRegion(), tmp_region2, tmp_region3);
			if (tmp_region3->count > 40); simplify_region(tmp_region3);
			owner->SetInvalidRegion(tmp_region3);
		}

		copy_region(owner->window_port.draw_region,
			    tmp_region1);

		offset_region(tmp_region1,
			      -owner->window_port.origin.h,
			      -owner->window_port.origin.v);

		r1 = bound;
		r2 = r1;
		offset_rect(&r2, dh, dv);

		if (dv > 0) {
			r2.bottom -= dv;
			r1.bottom -= dv;
		}

		if (dv < 0) {
			r2.top -= dv;
			r1.top -= dv;
		}


// blit them.

		tmp_bad = NULL;
		if (owner->InvalidRegion()) {
			tmp_bad = newregion();
			copy_region(owner->InvalidRegion(),tmp_bad);
			owner->SetInvalidRegion(NULL);
		}
		
		region *theVis = newregion();
		region *newVis = newregion();
		copy_region(VisibleRegion(),theVis);
		set_region(newVis,&vv_bound);
		SetVisibleRegion(newVis);
		kill_region(newVis);
	
		offset_rect(&r1,-(offset_h+bound.left),-(offset_v+bound.top));
		offset_rect(&r2,-(offset_h+bound.left),-(offset_v+bound.top));
		point p; p.h = r1.left; p.v = r1.top;
		/* This was physically painful */
		grDoCopyPixels(((MView*)this)->renderContext,r2,p,true);

		SetVisibleRegion(theVis);
		kill_region(theVis);

		if (tmp_bad) {
			owner->SetInvalidRegion(tmp_bad);
			kill_region(tmp_bad);
		}

		copy_region(tmp_region1, tmp_region2);

		offset_region(tmp_region2,
			      -dh,
			      -dv);

		sub_region(tmp_region1, tmp_region2, tmp_region3);
		owner->inval(tmp_region3);

		kill_region(tmp_region1);
		kill_region(tmp_region2);
		kill_region(tmp_region3);
	}


// I should probably see if the view that was set before
// is not the one for which is just change the coordinate
// system.
// In that case, i should not use the set_origin.

// set back the context as it was before my stuff.

	owner->set_origin(bound.left + offset_h,
	   	  	  bound.top +  offset_v);
	owner->recalc_clip();
	owner->set_origin(0, 0);

// Now we need to offset all the sub views bounds.
	
	a_son = first_child;

	while(a_son) {
		a_son->offset(-dh, -dv, 1);
		a_son = a_son->next_sibling;
	}

	if (first_child) recalc_visible(0);

/* set is called to be certain that the view has been recliped and
   that the origin is back at the right position */

	TView::set();
	signal_regions();
}

/*-------------------------------------------------------------*/

void	TView::set_origin(float h, float v)
{
	message	a_message;
	
	a_message.what = SET_ORIGIN;
	a_message.parm1 = (long)this;
	
	a_message.parm2 = h;
	a_message.parm3 = v;
	owner->send_message(&a_message);
}

/*-------------------------------------------------------------*/
// !! the inval case for move_bits will not handle to scale_bits
// case. !!!

void	TView::move_bits(rect *src_rect, rect *dst_rect)
{
	long	dh;
	long	dv;
	rect	r1;
	rect	r2;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;

// &&& This is a very dirty trick but until the server is ok
// this is still the best way.

	//while(owner->update_busy()) snooze(2000);

// We don't want the window or anythings to move while we
// are playing with the regions.

//	wait_regions();
	grLockPort(renderContext);

// This is by how much we will change the origin of this view.
// Could be smart to check for 0, 0. You never know what
// people will do to waste time.

	dh = dst_rect->left - src_rect->left;
	dv = dst_rect->top - src_rect->top;
	
// setup source and destination rects.

	tmp_region1 = newregion();
	tmp_region2 = newregion();
	tmp_region3 = newregion();

// If there is a bad region, then we've got to hack it to keep
// a correct update.

	r1 = *src_rect;
	r2 = *dst_rect;

// blit them.

	point p; p.h = r2.left; p.v = r2.top;
	grDoCopyPixels(((MView*)this)->renderContext,r1,p,true);

	copy_region(owner->window_port.draw_region,
		    tmp_region1); 

	offset_region(tmp_region1,
		      -(owner->ClientRegion()->bound.left + bound.left + offset_h),
		      -(owner->ClientRegion()->bound.top + bound.top +  offset_v));
	
	set_region(tmp_region2, &r1);

	and_region(tmp_region1,
		   tmp_region2,
		   tmp_region3);

	sub_region(tmp_region2,
		   tmp_region3,
		   tmp_region1);

	offset_region(tmp_region1, dh, dv);
	
	owner->inval(tmp_region1);

	kill_region(tmp_region1);
	kill_region(tmp_region2);
	kill_region(tmp_region3);

	grUnlockPort(renderContext);
//	signal_regions();
}
	
/*-------------------------------------------------------------*/

rect	TView::min_parent_rect()
{
	rect	tmp_rect;
	TView	*tmp_view;

	set_rect(tmp_rect, -1000000000, -1000000000, 1000000000, 1000000000);
	tmp_view = parent;
	
	while(tmp_view) {
		sect_rect(tmp_view->bound, tmp_rect, &tmp_rect);
		tmp_view = tmp_view->parent;
	}

	return(tmp_rect);
}

/*-------------------------------------------------------------*/

bool	TView::need_hilite()
{
	return(0);
}

/*-------------------------------------------------------------*/

void	TView::setup_recording0(rect pr_bound, rect ref_rect, TPicture *p)
{
	TView	*a_child;
	
	a_child = first_child;
	
	if (p) {
		fake_bound = bound; 
		marked_for_print = 1;
	}
	else
		marked_for_print = 0;

//	pic_offset_h = bound.left - ref_rect.left + offset_h;
//	pic_offset_v = bound.top - ref_rect.top + offset_v;

//	cp = p;
	while (a_child) {		
		a_child->setup_recording0(pr_bound, ref_rect, p);
		a_child = a_child->next_sibling;
	}
}

/*-------------------------------------------------------------*/

void	TView::setup_recording_recurs(rect pr_bound, TPicture *p)
{
	rect	r;
	TView	*a_child;

	wait_regions();
//	pic_offset_h = 0;
//	pic_offset_v = 0;

	if (p) {
		fake_bound = pr_bound; 
		marked_for_print = 1;
		//pic_offset_v = -pr_bound.top - offset_v;
		//pic_offset_h = -pr_bound.left - offset_h;
//		pic_offset_v = -pr_bound.top;
//		pic_offset_h = -pr_bound.left;
	}
	else
		marked_for_print = 0;
//	cp = p;
	r = bound;

	a_child = first_child;

	while (a_child) {		
		a_child->setup_recording0(pr_bound, r, p);
		a_child = a_child->next_sibling;
	}
	signal_regions();
}

/*-------------------------------------------------------------*/
