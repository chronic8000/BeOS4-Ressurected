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
#include <unistd.h>
#include <math.h>
#include <as_support.h>
#include <messages.h>
#include <InterfaceDefs.h>
#include <shared_defs.h>
#include <support/StreamIO.h>

#include "gr_types.h"
#include "proto.h"
#include "view.h"
#include "bitmap.h"
#include "server.h"
#include "render.h"
#include "parcel.h"
#include "render.h"
#include "rect.h"
#include "as_debug.h"
#include "macro.h"
#include "eventport.h"
#include "app.h"
#include "input.h"
#include "cursor.h"
#include "nuDecor.h"


#define VIEW_TRANSACTION_LIMIT		(100)
	// Maximum nb of rects of the region to update
	// for a view transaction


inline point npoint(int32 h, int32 v) { point p; p.h = h; p.v = v; return p; };
inline fpoint nfpoint(float h, float v) { fpoint p; p.h = h; p.v = v; return p; };

#include <Debug.h>

static const uint8 gClearStagesPartial[4][3] = {
	{ 0, 0, clearSolid | clearBitmaps },
	{ 0, 0, clearSolid | clearBitmaps },
	{ clearSolid, clearBitmaps, 0 },
	{ clearSolid, clearBitmaps, 0 }
};

uint8 TView::s_clearStages[16][3];

/*-------------------------------------------------------------*/

ViewDBInfo::ViewDBInfo(region *bufferRegion, int32 pixelFormat, uint8 endianess)
	: bitmap(
		bufferRegion->Bounds().right-bufferRegion->Bounds().left+1,
		bufferRegion->Bounds().bottom-bufferRegion->Bounds().top+1,
		pixelFormat,endianess,false
#if ROTATE_DISPLAY
		,B_BITMAP_IS_ROTATED)
#else
		)
#endif
{
	grInitRenderPort(&port);

#if ROTATE_DISPLAY
	if (bitmap.Canvas()->pixels.pixelIsRotated) {
		port.canvasIsRotated = true;
		port.rotater = new DisplayRotater;
		port.rotater->mirrorFValue = (float)(bitmap.Canvas()->pixels.w-1);
		port.rotater->mirrorIValue = bitmap.Canvas()->pixels.w-1;
	} else {
		port.rotater = NULL;
	}
#endif

	originInWindow.h = bufferRegion->Bounds().left;
	originInWindow.v = bufferRegion->Bounds().top;

	bitmap.SetPort(&port);
	realClip = newregion();
	next = NULL;
}

ViewDBInfo::~ViewDBInfo()
{
#if ROTATE_DISPLAY
	delete port.rotater;
#endif
	grDestroyRenderSubPort(&port);
	kill_region(realClip);
};

/*-------------------------------------------------------------*/

void TView::InitViews()
{
	for (int32 i=0;i<16;i++) {
		for (int32 stage=0;stage<3;stage++) {
			s_clearStages[i][stage] = 0;
			for (int32 j=0;j<4;j++) {
				if (i & (1<<j)) s_clearStages[i][stage] |= gClearStagesPartial[j][stage];
			}
		}
	}
}

region * TView::VisibleRegion()
{
	return m_visibleRegion;
};

void TView::OffsetVisibleRegion(int32 h, int32 v)
{
	offset_region(m_visibleRegion,h,v);
	if (owner) owner->ViewRegionsChanged(this);
};

void TView::SetVisibleRegion(region *r)
{
	copy_region(r,m_visibleRegion);
	if (owner) owner->ViewRegionsChanged(this);
};

void TView::SetVisibleRegion(rect *r)
{
	set_region(m_visibleRegion,r);
	if (owner) owner->ViewRegionsChanged(this);
};

TView::TView(rect *bnd, uint32 size_rules, uint32 flags)
{
#if AS_DEBUG
	SetName("TView");
#endif

	m_signature 			= VIEW_SIGNATURE;
	m_phantomBound			=
	bound 					= *bnd;

	owner					= NULL;
	next_sibling 			=
	first_child				=
	parent					= NULL;

	offset_h 				=
	offset_v				= 0;

	view_color.red			=
	view_color.green		=
	view_color.blue			= 255;
	view_color.alpha		= 0;

	rules					= size_rules;
	client_token			= NO_TOKEN;

	m_flags					= flags;

	m_rFlags 				= VFLAG_CLIP_TO_CLIENT;

	m_backgroundBitmap 		= NULL;
	m_visibleRegion 		= newregion();
	
	m_nextInEventChain		= NULL;
	m_nextInDoubleBufferChain = NULL;
	m_eventMask				= 0;
	m_augEventMask			= 0;
	m_augOptions			= 0;
	m_eventOptions			= 0;

	m_pointers				= 0;
	m_queueIter_mm			=
	m_queueIter_size		=
	m_queueIter_move		= 0xFFFFFFFF;
	m_mmMsgToken			= 
	m_sizeMsgToken			= 
	m_moveMsgToken			= NONZERO_NULL;
	
	m_lastVisRect.left		=
	m_lastVisRect.right		=
	m_lastVisRect.top		=
	m_lastVisRect.bottom	= -1;
	
	m_cursor				= NULL;

	m_doubleBufferOn		= 0;
	m_db					= NULL;
	m_doubleBufferRegion	= NULL;

	renderContext = grNewRenderContext();
	grInitRenderContext(renderContext);
	grSetRounding(renderContext,!(rules&SUBPIXEL_PRECISE));
	SetVisibleRegion(&bound);
	
	// clip insane values on boundary
	bnd->top = min(1048576,max(-1048576,bnd->top));
	bnd->bottom = min(1048576,max(-1048576,bnd->bottom));
	bnd->left = min(1048576,max(-1048576,bnd->left));
	bnd->right = min(1048576,max(-1048576,bnd->right));
	
	ClientLock();
	
	// allocate a new token to identify the view to the client
	server_token = tokens->new_token(xgetpid(), TT_VIEW, this);
}

TView::~TView()
{
	kill_region(m_visibleRegion);

	grDestroyRenderContext(renderContext);
	grFreeRenderContext(renderContext);
	renderContext = NULL;
	if (m_backgroundBitmap) {
		m_backgroundBitmap->bitmap->ServerUnlock();
		delete m_backgroundBitmap;
	};
};

/*-------------------------------------------------------------*/

void	TView::set_flags(long flags)
{
	if ((this->rules & SUBPIXEL_PRECISE) != ((uint32)flags & SUBPIXEL_PRECISE))
		grSetRounding(renderContext,!(flags&SUBPIXEL_PRECISE));
	this->rules	= flags;
}

/*-------------------------------------------------------------*/

void	TView::remove_from_list(TView *a_view)
{
	TView **v = &first_child;
	while (*v && (*v != a_view)) v = &(*v)->next_sibling;
	if (*v == a_view) {
		*v = (*v)->next_sibling;
		return;
	};

	DebugPrintf(("remove_from_list failed!\n"));
}

/*-------------------------------------------------------------*/

void TView::do_close(bool recursive)
{
	TView	*a_child;
	TView	*a_child1;

	UnaugmentEventMask(LOCK_STATE_WRITE);
	
	if (!recursive) {
		if (parent) {
			parent->remove_from_list(this);
			parent->RecalcVisible(NULL);
		}
	};
	
	a_child = first_child;
	while (a_child) {
		a_child1 = a_child->next_sibling;
		a_child->do_close(true);
		a_child = a_child1;
	}

	tokens->remove_token(server_token);
	m_signature = 0;

	if (owner) {
		owner->ViewCloseNotify(this);
		owner = NULL;
	}
	
	ClientUnlock();
}

/*-------------------------------------------------------------*/

void TView::add_view(TView *a_view, region *inval, bool last)
{
	rect	view_bound;
	rect	tmp_rect;
	region	*region_1;
	region	*region_2;

	a_view->offset(m_phantomBound.left + offset_h, m_phantomBound.top + offset_v, 0);
	a_view->SetOwner(Owner());
	a_view->parent = this;
	view_bound = a_view->m_phantomBound;
	
	TView **vp = &first_child;
	if (last) while (*vp) vp = &(*vp)->next_sibling;
	a_view->next_sibling = *vp;
	*vp = a_view;
	
	if (inval) {
		region_1 = newregion();
		region_2 = newregion();
		if (!(rules & NO_CLIP_TO_CHILDREN)) {
			set_region(region_1, &view_bound);
			sub_region(VisibleRegion(), region_1, region_2);
			SetVisibleRegion(region_2);
		}

		tmp_rect = a_view->min_parent_rect();
		set_region(region_1, &tmp_rect);
		and_region(region_1, a_view->VisibleRegion(), region_2);
		a_view->SetVisibleRegion(region_2);

		set_region(region_1,&view_bound);
		or_region(region_1,inval,region_2);
		copy_region(region_2,inval);
		the_server->force_mouse_moved();
		kill_region(region_1);
		kill_region(region_2);
	} else {
		a_view->m_flags |= vfIsNew;
		if (parent) parent->m_flags |= vfNeedsVisRecalc;
	};
}

/*-------------------------------------------------------------*/
		
void TView::Draw(bool isUpdate)
{
	(void)isUpdate;
}

/*-------------------------------------------------------------*/

void TView::get_bound(rect *bnd)
{
	bnd->top    = (int32)(-offset_v);
	bnd->left   = (int32)(-offset_h);
	bnd->right  = (int32)((m_phantomBound.right - m_phantomBound.left) - offset_h);
	bnd->bottom = (int32)((m_phantomBound.bottom - m_phantomBound.top) - offset_v);
}

void TView::GetFrameInWindow(rect *bnd)
{
	bnd->left   = (int32)(m_phantomBound.left);
	bnd->top    = (int32)(m_phantomBound.top);
	bnd->right  = (int32)(m_phantomBound.right);
	bnd->bottom = (int32)(m_phantomBound.bottom);
}

void TView::GetFrameInParent(rect* loc, bool screwedUp)
{
	if (parent) {
		loc->top = (int32)(m_phantomBound.top - parent->m_phantomBound.top);
		loc->left = (int32)(m_phantomBound.left - parent->m_phantomBound.left);
		loc->bottom = (int32)(loc->top + (m_phantomBound.bottom - m_phantomBound.top));
		loc->right = (int32)(loc->left + (m_phantomBound.right - m_phantomBound.left));
		if (!screwedUp) {
			loc->top = (int32)(loc->top-parent->offset_v);
			loc->bottom = (int32)(loc->bottom-parent->offset_v);
			loc->left = (int32)(loc->left-parent->offset_h);
			loc->right = (int32)(loc->right-parent->offset_h);
		};
	} else
		*loc = m_phantomBound;
}

/*-------------------------------------------------------------*/

void TView::SetOwner(TWindow *_owner)
{
	owner = _owner;

	grSetContextPort(renderContext,this,
		(LockRenderPort)TWindow::LockPort,
		(UnlockRenderPort)TWindow::UnlockPort);

	TView *a_child = first_child;
	while (a_child) {
		a_child->SetOwner(_owner);
		a_child = a_child->next_sibling;
	}
}

/*-------------------------------------------------------------*/

SCursor * TView::Cursor()
{
	return m_cursor;
}

void TView::SetCursor(SCursor *cursor)
{
	if (cursor) cursor->ServerLock();
	if (m_cursor) m_cursor->ServerUnlock();
	m_cursor = cursor;
}

/*-------------------------------------------------------------*/

void TView::offset(long dh, long dv, char redo_regions)
{
	offset_rect(&bound, dh, dv);
	offset_rect(&m_phantomBound, dh, dv);
	/*if (WillDraw())*/ OffsetVisibleRegion(dh, dv);

	TView *a_child = first_child;
	while (a_child) {
		a_child->offset(dh, dv, redo_regions);
		a_child = a_child->next_sibling;
	}

	if (redo_regions) RecalcVisible(NULL);
}

/*-------------------------------------------------------------*/

void TView::RecalcVisible(region *inval)
{
	region	*tmp_region1;
	region	*tmp_region2;
	rect	tmp;

//	if (!WillDraw()) return;

	region *newVisible = newregion();
	set_region(newVisible, &m_phantomBound);
	tmp_region1 = newregion();
	tmp_region2 = newregion();

	if (parent) {
		tmp = min_parent_rect();
		set_region(tmp_region1, &tmp);
		and_region(tmp_region1, newVisible, tmp_region2);
		copy_region(tmp_region2, newVisible);
	};

	SubAllChildren(tmp_region1, tmp_region2, newVisible);

	if (inval) {
		sub_region(newVisible, VisibleRegion(), tmp_region1);
		or_region(tmp_region1,inval,tmp_region2);
		copy_region(tmp_region2,inval);
	}

	SetVisibleRegion(newVisible);
	kill_region(newVisible);

	kill_region(tmp_region1);
	kill_region(tmp_region2);

	m_flags &= ~vfNeedsVisRecalc;
}

/*-------------------------------------------------------------*/

void	TView::send_movesize_event(rect *old, rect *newr)
{
	long	new_width;
	long	new_height;
	BParcel	a_parcel;

	a_parcel.AddInt64("when",system_time());
	a_parcel.SetTarget(client_token, false);
	if ((old->left != newr->left) || (old->top != newr->top)) {
		a_parcel.AddPoint("where",nfpoint(newr->left, newr->top));
		a_parcel.what = B_VIEW_MOVED;
		Owner()->EventPort()->ReplaceEvent(&a_parcel,&m_queueIter_move,&m_moveMsgToken);
	}

	new_width = newr->right - newr->left;
	new_height = newr->bottom - newr->top;

	if (((old->bottom - old->top) != new_height) ||
	    ((old->right - old->left) != new_width)) {
		a_parcel.AddInt32("width", new_width);
		a_parcel.AddInt32("height",new_height);
		a_parcel.what = B_VIEW_RESIZED;
		Owner()->EventPort()->ReplaceEvent(&a_parcel,&m_queueIter_size,&m_sizeMsgToken);
	}
}

/*-------------------------------------------------------------*/

void TView::Move(long dh, long dv, region *inval)
{
	rect new_bound;

	if ((dh == 0) && (dv == 0)) return;

	new_bound = m_phantomBound;
	offset_rect(&new_bound, dh, dv);
	SetBounds(new_bound);
	if (inval) {
		ViewTransaction(inval);
		parent->RecalcVisible(inval);
		the_server->force_mouse_moved();
	};
}

void TView::MoveTo(long h, long v, region *inval, bool screwedUp)
{
	if (parent) {
		h += parent->m_phantomBound.left;
		v += parent->m_phantomBound.top;
		if (!screwedUp) {
			h = (int32)(h+parent->offset_h);
			v = (int32)(v+parent->offset_v);
		};
	}
	Move(h-m_phantomBound.left,v-m_phantomBound.top,inval);
}

/*-------------------------------------------------------------*/

void TView::Resize(long dh, long dv, region *inval)
{
	rect new_bound;

	if ((dh == 0) && (dv == 0)) return;

	new_bound = m_phantomBound;
	new_bound.bottom += dv;
	new_bound.right += dh;

	SetBounds(new_bound);
	if (inval) {
		ViewTransaction(inval);
		parent->RecalcVisible(inval);
		the_server->force_mouse_moved();
	};
}

void TView::ResizeTo(long h, long v, region *inval)
{
	Resize(
		h-(m_phantomBound.right-m_phantomBound.left),
		v-(m_phantomBound.bottom-m_phantomBound.top),
		inval);
}

/*-------------------------------------------------------------*/

void TView::WindowToLocal(long *h, long *v)
{
	*h -= (int32)(m_phantomBound.left + offset_h);
	*v -= (int32)(m_phantomBound.top + offset_v);
}

void TView::WindowToLocal(float *h, float *v)
{
	*h -= (int32)(m_phantomBound.left + offset_h);
	*v -= (int32)(m_phantomBound.top + offset_v);
}

void TView::LocalToWindow(long *h, long *v)
{
	*h += (int32)(m_phantomBound.left + offset_h);
	*v += (int32)(m_phantomBound.top + offset_v);
}

void TView::LocalToWindow(float *h, float *v)
{
	*h += (int32)(m_phantomBound.left + offset_h);
	*v += (int32)(m_phantomBound.top + offset_v);
}

/*-------------------------------------------------------------*/

void TView::do_set_origin(float h, float v)
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

	region  *vv_region;
	vv_region=newregion();
	if (Owner()->flags&VIEWS_OVERLAP) {
		HierarchyVisibleRegion(vv_region);
	} else {
		parent_bound = min_parent_rect();
		sect_rect(parent_bound, m_phantomBound, &vv_bound);
		set_region(vv_region,&vv_bound);
	}

	h = floor(h+0.5);
	v = floor(v+0.5);

	dh = (int32)(h + offset_h);
	dv = (int32)(v + offset_v);

	if (dh || dv) {
		offset_h = (-h);
		offset_v = (-v);

		tmp_region1 = newregion();
		tmp_region2 = newregion();
		tmp_region3 = newregion();

		if (!empty_region(owner->BadRegion())) {
			copy_region(owner->BadRegion(), tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->BadRegion(), tmp_region2);
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			copy_region(vv_region,tmp_region1);
			offset_region(tmp_region1,
			      	owner->ClientBound()->left,
			      	owner->ClientBound()->top);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			copy_region(owner->BadRegion(), tmp_region1);
			
			or_region(tmp_region1, tmp_region2, owner->BadRegion());
		}

		if (!empty_region(owner->ClearRegion())) {
			copy_region(owner->ClearRegion(), tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->ClearRegion(), tmp_region2);
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			copy_region(vv_region,tmp_region1);
			offset_region(tmp_region1,
			      	owner->ClientBound()->left,
			      	owner->ClientBound()->top);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			copy_region(owner->ClearRegion(), tmp_region1);
			
			or_region(tmp_region1, tmp_region2, owner->ClearRegion());
		}

		if (owner->InvalidRegion()) {
			copy_region(owner->InvalidRegion(), tmp_region1);
			offset_region(tmp_region1, -dh, -dv);
			copy_region(owner->InvalidRegion(), tmp_region2);
			or_region(tmp_region1, tmp_region2, tmp_region3);
			
			copy_region(vv_region,tmp_region1);
			offset_region(tmp_region1,
			      	owner->ClientBound()->left,
			      	owner->ClientBound()->top);
			and_region(tmp_region3, tmp_region1, tmp_region2);
			
			or_region(owner->InvalidRegion(), tmp_region2, tmp_region3);
			owner->SetInvalidRegion(tmp_region3);
		}

		r1 = m_phantomBound;
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

		tmp_bad = NULL;
		if (owner->InvalidRegion()) {
			tmp_bad = newregion();
			copy_region(owner->InvalidRegion(),tmp_bad);
			owner->SetInvalidRegion(NULL);
		}
		
		region *theVis = newregion();
		region *newVis = newregion();
		copy_region(VisibleRegion(),theVis);
		copy_region(vv_region,newVis);
		SetVisibleRegion(newVis);
		kill_region(newVis);
	
		offset_rect(&r1,
			(int32)(-(offset_h+m_phantomBound.left)),
			(int32)(-(offset_v+m_phantomBound.top)));
		offset_rect(&r2,
			(int32)(-(offset_h+m_phantomBound.left)),
			(int32)(-(offset_v+m_phantomBound.top)));
		point p; p.h = r1.left; p.v = r1.top;

		grLock(renderContext);
		grGetUpdateRegion(renderContext,tmp_region1);
		grCopyPixels(renderContext,r2,p,true);
		grUnlock(renderContext);

		SetVisibleRegion(theVis);
		kill_region(theVis);

		if (tmp_bad) {
			owner->SetInvalidRegion(tmp_bad);
			kill_region(tmp_bad);
		}

		offset_region(tmp_region1,
			(int32)(m_phantomBound.left+offset_h),
			(int32)(m_phantomBound.top+offset_v));
		copy_region(tmp_region1, tmp_region2);
		offset_region(tmp_region2, -dh, -dv);

		sub_region(tmp_region1, tmp_region2, tmp_region3);
		owner->Invalidate(tmp_region3, redrawScrolled);

		kill_region(tmp_region1);
		kill_region(tmp_region2);
		kill_region(tmp_region3);
	}
	
	a_son = first_child;

	while(a_son) {
		a_son->offset(-dh, -dv, 1);
		a_son = a_son->next_sibling;
	}

	if (first_child) RecalcVisible(NULL);

	kill_region(vv_region);
}

/*-------------------------------------------------------------*/

void	TView::move_bits(rect *src_rect, rect *dst_rect)
{
	int32	dh,dv;
	rect	r1,r2;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	region	*tmp_region4;
	region	*tmp_region5;

	dh = dst_rect->left - src_rect->left;
	dv = dst_rect->top - src_rect->top;
	if ((dh|dv) == 0) return;
	
	r1 = *src_rect;
	r2 = *dst_rect;

	tmp_region1 = newregion();
	tmp_region2 = newregion();
	tmp_region3 = newregion();
	tmp_region4 = newregion();
	tmp_region5 = newregion();

	grLock(renderContext);

		point p; p.h = r2.left; p.v = r2.top;
		grCopyPixels(renderContext,r1,p,true);

		grGetUpdateRegion(renderContext,tmp_region1);
		/*	tmp_region1 = the update region */
		set_region(tmp_region2, &r1);
		/*	tmp_region2 = the source rect */
		sub_region(tmp_region2,tmp_region1,tmp_region3);
		/*	tmp_region3 = the part of the source rect
			that wasn't available for blitting */
		or_region(owner->ClearRegion(),owner->BadRegion(),tmp_region4);
		offset_region(tmp_region4,
			(int32)(-(owner->ClientBound()->left + m_phantomBound.left + offset_h)),
			(int32)(-(owner->ClientBound()->top + m_phantomBound.top + offset_v)));
		/*	tmp_region4 = the part of the view that is invalid */
		and_region(tmp_region4,tmp_region2,tmp_region5);
		/*	tmp_region5 = the part of the source rect that was invalid */
		or_region(tmp_region3,tmp_region5,tmp_region2);
		/*	tmp_region2 = the part of the source rect
			that couldn't be blitted correctly */
		offset_region(tmp_region2,dh,dv);
		/*	tmp_region2 = the part of the dest rect
			that couldn't be blitted correctly */
		and_region(tmp_region2,tmp_region1,tmp_region3);
		/*	tmp_region3 = the part of the dest rect
			that couldn't be blitted correctly that is
			also part of the view's clipping region */
		offset_region(tmp_region3,
			(int32)(m_phantomBound.left + offset_h),
			(int32)(m_phantomBound.top + offset_v));
		owner->Invalidate(tmp_region3);

	grUnlock(renderContext);

	kill_region(tmp_region1);
	kill_region(tmp_region2);
	kill_region(tmp_region3);
	kill_region(tmp_region4);
	kill_region(tmp_region5);
}
	
/*-------------------------------------------------------------*/

rect	TView::min_parent_rect()
{
	rect	tmp_rect;
	TView	*tmp_view;

	set_rect(tmp_rect, -1000000000, -1000000000, 1000000000, 1000000000);

	tmp_view = parent;
	while(tmp_view) {
		sect_rect(tmp_view->m_phantomBound, tmp_rect, &tmp_rect);
		tmp_view = tmp_view->parent;
	}

	return tmp_rect;
}

/*-------------------------------------------------------------*/

void TView::BeginUpdate()
{
	m_rFlags |= VFLAG_IN_UPDATE;
};

void TView::EndUpdate()
{
	m_rFlags &= ~VFLAG_IN_UPDATE;
	m_flags &= ~(vfDrawnIn|vfDoubleBuffered);
};

/*-------------------------------------------------------------*/

void TView::PropagateBounds(
	rect old_parent_rect,
	rect new_parent_rect,
	bool isMove)
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

	delta_top		= new_parent_rect.top - old_parent_rect.top;
	delta_left		= new_parent_rect.left - old_parent_rect.left;
	delta_bottom	= new_parent_rect.bottom - old_parent_rect.bottom;
	delta_right		= new_parent_rect.right - old_parent_rect.right;

	new_bound = m_phantomBound;

	if (isMove) {
		new_bound.top += delta_top;
		new_bound.bottom += delta_top;
		new_bound.left += delta_left;
		new_bound.right += delta_left;		
	} else {
		switch((rules >> 12) & 0xf) {
			case	POS_TOP		: new_bound.top += delta_top;break;
			case	POS_BOTTOM	: new_bound.top += delta_bottom;break;
			case	POS_CENTER	:
				new_vcenter	= (new_parent_rect.top + new_parent_rect.bottom) / 2;
				old_vcenter	= (old_parent_rect.top + old_parent_rect.bottom) / 2;
				new_bound.top = new_vcenter + (m_phantomBound.top - old_vcenter);
				break;
		}
		
		switch((rules >> 4) & 0xf) {
			case	POS_TOP		: new_bound.bottom += delta_top;break;
			case	POS_BOTTOM	: new_bound.bottom += delta_bottom;break;
			case	POS_CENTER	:
				new_vcenter	= (new_parent_rect.top + new_parent_rect.bottom) / 2;
				old_vcenter	= (old_parent_rect.top + old_parent_rect.bottom) / 2;
				new_bound.bottom = new_vcenter + (m_phantomBound.bottom - old_vcenter);
				break;
		}
	
		switch((rules >> 8) & 0xf) {
			case	POS_LEFT	: new_bound.left += delta_left;break;
			case	POS_RIGHT	: new_bound.left += delta_right;break;
			case	POS_CENTER	:
				new_hcenter	= (new_parent_rect.left + new_parent_rect.right) / 2;
				old_hcenter	= (old_parent_rect.left + old_parent_rect.right) / 2;
				new_bound.left = new_hcenter + (m_phantomBound.left - old_hcenter);
				break;
		}
	
		switch(rules & 0xf) {
			case	POS_LEFT	: new_bound.right += delta_left;break;
			case	POS_RIGHT	: new_bound.right += delta_right;break;
			case	POS_CENTER	:
				new_hcenter	= (new_parent_rect.left + new_parent_rect.right) / 2;
				old_hcenter	= (old_parent_rect.left + old_parent_rect.right) / 2;
				new_bound.right = new_hcenter + (m_phantomBound.right - old_hcenter);
				break;
		}
	};

	if (!equal_rect(&new_bound,&m_phantomBound)) {
		TView *child = first_child;
		while (child) {
			child->PropagateBounds(m_phantomBound,new_bound,isMove);
			child = child->next_sibling;
		};
		
		m_phantomBound = new_bound;
		m_flags |= vfHasNewBounds;
		if (parent) parent->m_flags |= vfNeedsVisRecalc;
	};
}

/*-------------------------------------------------------------*/

bool TView::ResetDecor()
{
	return false;
}

/*-------------------------------------------------------------*/

void TView::SetBounds(rect newBound)
{
	bool 	isMove 		= false;
	int32 	dLeft   	= newBound.left - m_phantomBound.left,
			dRight  	= newBound.right - m_phantomBound.right,
			dTop    	= newBound.top - m_phantomBound.top,
			dBottom 	= newBound.bottom - m_phantomBound.bottom;

	if (!(dLeft | dRight | dTop | dBottom)) return;
	if ((dLeft == dRight) && (dTop == dBottom)) isMove = true;
	
	TView *child = first_child;
	while (child) {
		child->PropagateBounds(m_phantomBound,newBound,isMove);
		child = child->next_sibling;
	};

	m_phantomBound = newBound;
	m_flags |= vfHasNewBounds;
	if (parent) parent->m_flags |= vfNeedsVisRecalc;
};

/*-------------------------------------------------------------*/

void TView::ViewTransaction(
	region *needToDraw,
	BArray<TView*> *localViews,
	bool forceRecurse,
	rect *minParentRect,
	bool recursive)
{
	rect	visRect;
	region	*tmpRegion1 = newregion();
	region	*tmpRegion2 = newregion();
	bool	visRectChanged;

	forceRecurse = true;

	if (!minParentRect) {
		minParentRect = &visRect;
		visRect = min_parent_rect();
	};
	
	sect_rect(m_phantomBound,*minParentRect,&visRect);
	visRectChanged = !equal_rect(&visRect,&m_lastVisRect);

	if (forceRecurse || visRectChanged) {
		TView *child = first_child;	
		if (needToDraw) {
			clear_region(tmpRegion1);
			while (child) {
				child->ViewTransaction(tmpRegion1, localViews, forceRecurse, &visRect, true);
				child = child->next_sibling;
				if (tmpRegion1->CountRects() > VIEW_TRANSACTION_LIMIT) {
					set_region(tmpRegion1, &m_phantomBound);
					or_region(needToDraw,tmpRegion1,tmpRegion2);
					copy_region(tmpRegion2,needToDraw);
					goto noRegionCollection;
				};
			};
			or_region(needToDraw,tmpRegion1,tmpRegion2);
			copy_region(tmpRegion2,needToDraw);
		} else {
			noRegionCollection:
			while (child) {
				child->ViewTransaction(NULL, localViews, forceRecurse, &visRect, true);
				child = child->next_sibling;
			};
		};
	};
	
	if ((m_flags & (vfHasNewBounds|vfNeedsVisRecalc|vfIsNew)) || visRectChanged) {
		region *tmpRegion3 = newregion();
		m_lastVisRect = visRect;

		if (!(rules & FAST)) {
			if (m_flags & vfHasNewBounds) {
				if (rules & NEED_SIZE) send_movesize_event(&bound, &m_phantomBound);
				if (m_backgroundBitmap) UpdateBackgroundBitmap();
			};
			
			set_region(tmpRegion3, &m_phantomBound);
			SubAllChildren(tmpRegion1, tmpRegion2, tmpRegion3);
			set_region(tmpRegion1, minParentRect);
			and_region(tmpRegion1, tmpRegion3, tmpRegion2);

			if (needToDraw) {
				if ((m_flags & vfIsNew) ||
					(rules & FULL_DRAW) ||
				    ((bound.top  != m_phantomBound.top) ||
				     (bound.left != m_phantomBound.left))) {
					or_region(tmpRegion2,VisibleRegion(),tmpRegion3);
					copy_region(needToDraw,tmpRegion1);
					or_region(tmpRegion1,tmpRegion3,needToDraw);
				} else {
					sub_region(tmpRegion2, VisibleRegion(), tmpRegion3);
					if (recursive) {
						copy_region(needToDraw,tmpRegion1);
						or_region(tmpRegion1,tmpRegion3, needToDraw);
					} else {
						or_region(needToDraw, tmpRegion3, tmpRegion1);
						sub_region(VisibleRegion(), tmpRegion2, tmpRegion3);
						or_region(tmpRegion1,tmpRegion3, needToDraw);
					};
				}
			};

			SetVisibleRegion(tmpRegion2);
		} else {
			set_region(tmpRegion3, &m_phantomBound);
			SubAllChildren(tmpRegion1, tmpRegion2, tmpRegion3);
			set_region(tmpRegion1, minParentRect);
			and_region(tmpRegion1, tmpRegion3, tmpRegion2);
			SetVisibleRegion(tmpRegion2);

			// Should test against 'visRectChanged' too
			// oldcode: if (m_flags & (vfHasNewBounds|vfIsNew))
			if ((m_flags & (vfHasNewBounds|vfIsNew)) || visRectChanged) {
				if (localViews) localViews->AddItem(this);
				else if (needToDraw) {
					if (m_rFlags & VFLAG_CLIP_TO_CLIENT) {
						int32 xoffs = owner->ClientBound()->left;
						int32 yoffs = owner->ClientBound()->top;
						copy_region(owner->ClientRegion(),tmpRegion3);
						offset_region(tmpRegion3,-xoffs,-yoffs);
						set_region(tmpRegion1,&m_phantomBound);
						and_region(tmpRegion3,tmpRegion1,tmpRegion2);
					} else {
						set_region(tmpRegion2,&m_phantomBound);
					};
					
					copy_region(needToDraw,tmpRegion1);
					or_region(tmpRegion1,tmpRegion2,needToDraw);
				}
			}		
		};
		kill_region(tmpRegion3);
	};
	
	kill_region(tmpRegion1);
	kill_region(tmpRegion2);

	bound = m_phantomBound;
	m_flags &= ~(vfHasNewBounds|vfNeedsVisRecalc|vfIsNew);
};

/*-------------------------------------------------------------*/

void TView::TweakHierarchy(region*parentHierarchyVisible,bool overlap) {
	region* myVisible;
	region* childrenVisible = NULL;
	region* tmpRegion1;
	region* tmpRegion2;
	rect tmp_rect;

	if (!(Owner()->flags&VIEWS_OVERLAP)) {
		return;
	}

	myVisible=newregion();
	tmpRegion1=newregion();
	tmpRegion2=newregion();

	if (parentHierarchyVisible) {
		copy_region(parentHierarchyVisible,myVisible);
	} else if (parent!=NULL) {
		if (parent->HierarchyVisibleRegion(myVisible,tmpRegion1,tmpRegion2)) {
			overlap=true;
		}
	} else {
		set_rect(tmp_rect,-1000000000,-1000000000,1000000000,1000000000);
		set_region(myVisible,&tmp_rect);
	}

	if (rules&OVERLAP_CHILDREN) {
		overlap=true;
	}

	set_region(tmpRegion1,&m_phantomBound);
	and_region(tmpRegion1,myVisible,tmpRegion2);
	copy_region(tmpRegion2,myVisible);

	if (!overlap) {
		childrenVisible=newregion();
		copy_region(myVisible,childrenVisible);
	}

	for (TView*a_view=first_child;a_view!=NULL;a_view=a_view->next_sibling) {
		if (overlap) {
			a_view->TweakHierarchy(myVisible,overlap);
		} else {
			a_view->TweakHierarchy(childrenVisible,overlap);
		}
		set_region(tmpRegion1,&a_view->m_phantomBound);
		sub_region(myVisible,tmpRegion1,tmpRegion2);
		copy_region(tmpRegion2,myVisible);
	}

	if (overlap) {
		SetVisibleRegion(myVisible);
	} else {
		kill_region(childrenVisible);
	}

	kill_region(myVisible);
	kill_region(tmpRegion1);
	kill_region(tmpRegion2);
}

bool TView::HierarchyVisibleRegion(region*target,region*tmp1,region*tmp2) {
	rect tmp_rect;
	set_rect(tmp_rect,-1000000000,-1000000000,1000000000,1000000000);
	set_region(target,&tmp_rect);
	return HierarchyVisibleRegionCore(target,tmp1,tmp2);
}

bool TView::HierarchyVisibleRegion(region*target) {
	region*tmp1=newregion();
	region*tmp2=newregion();
	bool ret=HierarchyVisibleRegion(target,tmp1,tmp2);
	kill_region(tmp1);
	kill_region(tmp2);
	return ret;
}

bool TView::HierarchyVisibleRegionCore(region*target,region*tmp1,region*tmp2) {
	bool do_clip=false;
	if (parent) {
		do_clip=parent->HierarchyVisibleRegionCore(target,tmp1,tmp2);
	}
	set_region(tmp1,&m_phantomBound);
	and_region(tmp1,target,tmp2);
	copy_region(tmp2,target);
	if (do_clip) { // parent is guaranteed to be non-NULL
		for(TView*sibling=parent->first_child;sibling!=this;sibling=sibling->next_sibling) {
			set_region(tmp1,&sibling->m_phantomBound);
			sub_region(target,tmp1,tmp2);
			copy_region(tmp2,target);
		}
	}
	if (rules&OVERLAP_CHILDREN) {
		do_clip=true;
	}
	return do_clip;
}

/*-------------------------------------------------------------*/

void TView::SubFewChildren(
	region *tmp1, region *tmp2, region *subtractFrom,
	TView *a_child, rect boundArg)
{
	region *cur,*tmp3 = newregion();
	clear_region(cur=tmp2);
	while (a_child) {
		if (overlap_rect(boundArg, a_child->m_phantomBound)) {
			set_region(tmp1, &(a_child->m_phantomBound)); 
			or_region(tmp2, tmp1, cur=tmp3);
		} else {
			a_child = a_child->next_sibling;
			continue;
		};
		second:
		a_child = a_child->next_sibling;
		if (a_child) {
			if (overlap_rect(boundArg, a_child->m_phantomBound)) {
				set_region(tmp1, &(a_child->m_phantomBound)); 
				or_region(tmp3, tmp1, cur=tmp2);
			} else goto second;
			a_child = a_child->next_sibling;
		};
	};
	
	copy_region(subtractFrom,tmp1);
	sub_region(tmp1,cur,subtractFrom);
	kill_region(tmp3);
}

TView * TView::SubAllChildrenHelper(
	TView *child, int32 childCount,
	region **stack, rect *boundArg)
{
	if (childCount <= 8) {
		region *tmpR;
		clear_region(stack[0]);
		while (childCount--) {
			if (overlap_rect(*boundArg, child->m_phantomBound)) {
				set_region(stack[1], &(child->m_phantomBound));
				or_region(stack[1], stack[0], stack[2]);
				tmpR = stack[0];
				stack[0] = stack[2];
				stack[2] = tmpR;
			};
			child = child->next_sibling;
		};
	} else {
		int32 i = childCount / 2;
		child = SubAllChildrenHelper(child,i,stack+1,boundArg);
		child = SubAllChildrenHelper(child,childCount-i,stack+2,boundArg);
		or_region(stack[1],stack[2],stack[0]);
	};
	return child;
}

void TView::SubAllChildren(region *tmp1, region *tmp2, region *subtractFrom)
{
	if (!first_child || (rules & NO_CLIP_TO_CHILDREN)) return;

	int32 childCount=0;
	TView *v = first_child;
	while (v) { childCount++; v = v->next_sibling; };
	if (childCount < 10) {
		SubFewChildren(tmp1,tmp2,subtractFrom,first_child,m_phantomBound);
		return;
	};

	region *stack[48];
	int32 i = childCount,j=3;
	while (i) { j+=2; i >>= 1; };
	for (i=0;i<j;i++) stack[i] = newregion();

	i = childCount / 2;
	v = SubAllChildrenHelper(first_child,i,stack+1,&m_phantomBound);
	SubAllChildrenHelper(v,childCount-i,stack+2,&m_phantomBound);
	or_region(stack[1],stack[2],stack[0]);

	copy_region(subtractFrom,stack[1]);
	sub_region(stack[1],stack[0],subtractFrom);

	for (i=0;i<j;i++) kill_region(stack[i]);
}

/*-------------------------------------------------------------*/

TView * TView::StartDoubleBuffer(uint32 updateCauses, TView *myStartBuffering, TView **startBuffering, uint32 flags)
{
	TView *tmpView;
	
	m_nextInDoubleBufferChain = NULL;
	if (DoubleBufferRules() & updateCauses)
	{
		/* We want this view to be double buffered */
		if (	startBuffering &&
				(
					(
						((parent->rules & NO_CLIP_TO_CHILDREN) &&
						(*((uint32*)(&view_color)) == TRANSPARENT_RGBA))
					) ||
					(flags & gatherDoubleBuffering)
				)
			)
		{
			//	We have to pass this view up to the parent to start double buffering
			//	before we get down this far in the update sequence, because he might have some
			//	drawing to do before and/or after we draw.

			// mathias - 26/11/2000: We must preserve the parent's list here.
			// The original code was emptying that list when the current view had no
			// double-buffered children, which is obviously wrong. The fix simply add the
			// current view to the list in that case (I know the TView ** thing is not easy to read
			// but I've stollen this to geh, and I love it :)
			// Add our double-buffered children to our list
			m_nextInDoubleBufferChain = myStartBuffering;
			// And insert us (and all our of double-buffered children) in our parent's list
			TView **next = &m_nextInDoubleBufferChain;
			while (*next)
				next = &((*next)->m_nextInDoubleBufferChain);
			*next = *startBuffering;
			*startBuffering = this;

			m_doubleBufferRegion = newregion();
			copy_region(VisibleRegion(), m_doubleBufferRegion);

			// Don't create a buffer for this view (one of its ancestor will do it)
			myStartBuffering = NULL;
		}
		else
		{
			// mathias - 25/11/2000: Either we :
			// - fill our entire background region, or (TRANSPARENT_RGBA)
			// - the parent never draws in our drawing region, or (NO_CLIP_TO_CHILDREN)
			// - We have no double-buffered ancestor (flags & gatherDoubleBuffering)
			// - We are the top_view (startBuffering == NULL)
			// so we can wait until we get to this point in
			// the update sequence to start double buffering this view. 
			region *tmpRegion1 = newregion();
			region *tmpRegion2 = newregion();
			region *swapper;
			while (myStartBuffering)
			{
				// We have double-bufferred children. So gather their double-buffered region
				// and remove them from the doublebuffered list
				or_region(myStartBuffering->m_doubleBufferRegion,tmpRegion1,tmpRegion2);
				swapper = tmpRegion1;
				tmpRegion1 = tmpRegion2;
				tmpRegion2 = swapper;
				kill_region(myStartBuffering->m_doubleBufferRegion);
				myStartBuffering->m_doubleBufferRegion = NULL;
				tmpView = myStartBuffering->m_nextInDoubleBufferChain;
				myStartBuffering->m_nextInDoubleBufferChain = NULL;
				myStartBuffering = tmpView;
			};
			or_region(tmpRegion1,VisibleRegion(),tmpRegion2);
			m_doubleBufferRegion = tmpRegion2;
			kill_region(tmpRegion1);
			// We'll need a buffer for this view
			myStartBuffering = this;
		};
		// Our children must know that we are doublebuffered
		m_flags |= vfDoubleBuffered;
	}
	else if (flags & gatherDoubleBuffering)
	{	// This is a redular view, but one of its ancestor is double-buffered.
		// Don't insert this view in the double-buffered list
		if (myStartBuffering)
		{
			while (myStartBuffering->m_nextInDoubleBufferChain)
				myStartBuffering = myStartBuffering->m_nextInDoubleBufferChain;
			myStartBuffering->m_nextInDoubleBufferChain = *startBuffering;
			*startBuffering = myStartBuffering;
		}

		// mathias - 26/11/2000: Here, we must not set *startBuffering to NULL, because this
		// would erase the parent's list of double buffered view.

		// We don't want a buffer for this view
		myStartBuffering = NULL;
	}
	else
	{ // This is a regular view. Do nothing.
	}
	
	return myStartBuffering;
};

void TView::Gather(	region **regionToClear,
					BArray<ViewInfo> *viewList,
					uint32 updateCauses,
					TView **startBuffering,
					uint32 flags)
{
	int32 listIndex=0;
	if (rect_in_region(*regionToClear, &m_phantomBound)) {

		if (WillDraw()) {
			listIndex = viewList->CountItems();
			viewList->AddItem(ViewInfo(this,NULL));
		};

		TView *child = first_child;
		TView *myStartBuffering = NULL;
		uint32 childFlags = flags;
		if (DoubleBufferRules() & updateCauses) childFlags |= gatherDoubleBuffering;
		while (child) {
			child->Gather(regionToClear,viewList,updateCauses,&myStartBuffering,childFlags);
			child = child->next_sibling;
		};

		if (WillDraw()) {
			(*viewList)[listIndex].startBuffering =
				StartDoubleBuffer(updateCauses,myStartBuffering,startBuffering,flags);
		};
	};
};

/*-------------------------------------------------------------*/

TView *	TView::FindView(point location)
{
	TView	*v,*r;
	
	if (!point_in_rect(&m_phantomBound, location.h, location.v)) return NULL;

	v = first_child;
	while (v) {
		r = v->FindView(location);
		if (r) return r;
		v = v->next_sibling;
	}

	return this;
};

/*-------------------------------------------------------------*/

void TView::SetEventMask(uint32 eventMask, uint32 options)
{
	uint32 oldEventMask = EventMask();
	options &= aoNoPointerHistory;
	m_eventOptions = options;
	m_eventMask = eventMask;
	if ((EventMask() != oldEventMask) && owner)
		owner->ViewEventMaskChanged(this,oldEventMask);
};

void TView::UnaugmentEventMask(int32 lockState)
{
	uint32 oldEventMask = EventMask();
	m_augEventMask = 0;
	if (owner) {
		if (m_augOptions & aoLockWindowFocus)
			owner->unlock_focus(lockState);
		if (m_augOptions & aoUsurpViewFocus)
			owner->UndampFocusedEvents();
		if (EventMask() != oldEventMask)
			owner->ViewEventMaskChanged(this,oldEventMask);
	};
	m_augOptions = 0;
}

void TView::AugmentEventMask(uint32 eventMask, uint32 options)
{
	if (m_augEventMask || m_augOptions) return;
	if (!eventMask && !options) return;
	
	uint32 oldEventMask = EventMask();
	if (eventMask && !m_augEventMask && (options & aoUsurpViewFocus) && owner)
		owner->DampFocusedEvents();
	if (options & aoLockWindowFocus) owner->lock_focus();
	m_augEventMask = eventMask;
	m_augOptions = options;
	if ((EventMask() != oldEventMask) && owner)
		owner->ViewEventMaskChanged(this,oldEventMask);
};

/*-------------------------------------------------------------*/

void TView::HandleEvent(BParcel *)
{
}

void TView::HandleDecorEvent(Event *)
{
}

void TView::DispatchEvent(BParcel *event)
{
#if AS_DEBUG
	if (event->what == B_MOUSE_DOWN) {
		uint32 buttons = event->FindInt32("buttons");
		uint32 modifiers = event->FindInt32("modifiers");
		if (modifiers & SCROLL_LOCK) { // The secret debugging click signal
			bool earlyExit = true;
			if (buttons == 4) {
				if (modifiers & CONTROL_KEY)
					DumpDebugInfo(NULL);
				else if (modifiers & SHIFT_KEY)
					SetDoubleBufferRules(0);
				else
					SetDoubleBufferRules(redrawAll);
			} else if (buttons == 2) {		// Turning OFF debug printing
				if (modifiers & CONTROL_KEY)
					SetTrace(tfInfo,false);
				else if (modifiers & SHIFT_KEY)
					grSetDebugPrinting(renderContext,false);
				else if (modifiers & COMMAND_KEY)
					grSetLockingPersistence(renderContext,0);
				else earlyExit = false;
			} else {				// Turning ON debug printing
				if (modifiers & CONTROL_KEY)
					SetTrace(tfInfo,true);
				else if (modifiers & SHIFT_KEY)
					grSetDebugPrinting(renderContext,true);
				else if (modifiers & COMMAND_KEY)
					grSetLockingPersistence(renderContext,2000);
				else earlyExit = false;
			};
			if (earlyExit) return;
		};
	};
#endif

	if ((event->Routing() & rtThin) || (event->EventMask() & (m_eventMask|m_augEventMask))) {
		if (owner && (IsClientView() || (this == owner->top_view))) {
			event->SetTarget(client_token, false);
			if (((m_augOptions|m_eventOptions) & aoNoPointerHistory) &&
				(event->what == B_MOUSE_MOVED))
				owner->EventPort()->ReplaceEvent(event,&m_queueIter_mm,&m_mmMsgToken);
			else
				owner->EventPort()->SendEvent(event);
		} else {
			event->SetTarget(server_token, false);
			if (((m_augOptions|m_eventOptions) & aoNoPointerHistory) &&
				(event->what == B_MOUSE_MOVED))
				the_server->WindowManager()->EventPort()->ReplaceEvent(event,&m_queueIter_mm,&m_mmMsgToken,owner);
			else
				the_server->WindowManager()->EventPort()->SendEvent(event,server_token,false,owner);
		};
		uint32 transit = event->FindInt32("be:transit");
		if ((transit == B_ENTERED) || (transit == B_EXITED)) {
			m_queueIter_mm = 0xFFFFFFFF;
			m_mmMsgToken = NONZERO_NULL;
		};
	};
	
	if ((event->what == B_MOUSE_UP) && m_augEventMask)
		UnaugmentEventMask(LOCK_STATE_READ);
};

/*-------------------------------------------------------------*/

uint32 TView::Clear(RenderContext *theContext, uint32 clearFlags)
{
	uint32 myFlags = 0;
	if (m_backgroundBitmap && !(m_backgroundBitmap->options & AS_OVERLAY_BITMAP))
		myFlags |= clearBitmaps;
	if (*((uint32*)(&view_color)) != TRANSPARENT_RGBA)
		myFlags |= clearSolid;

	clearFlags &= myFlags;

	if (myFlags & clearBitmaps)
	{
		int32 tmpI;
		frect fdstRect;
		SBitmap *bitmap = m_backgroundBitmap->bitmap;
		frect srcRect = m_backgroundBitmap->srcRect;
		rect dstRect = m_backgroundBitmap->finalDst;
		if ((dstRect.left > dstRect.right) ||
			(dstRect.top > dstRect.bottom))
			goto color;

		if (clearFlags & clearBitmaps)
		{
			dstRect.left = (int32)(dstRect.left + offset_h);
			dstRect.right = (int32)(dstRect.right + offset_h);
			dstRect.top = (int32)(dstRect.top + offset_v);
			dstRect.bottom = (int32)(dstRect.bottom + offset_v);
	
			if (m_backgroundBitmap->options & B_TILE_BITMAP_X) {
				int32 i = dstRect.right - dstRect.left + 1;
				dstRect.left = dstRect.left % i;
				if (dstRect.left > 0) dstRect.left -= i;
				dstRect.right = dstRect.left + i - 1;
			};
	
			if (m_backgroundBitmap->options & B_TILE_BITMAP_Y) {
				int32 i = dstRect.bottom - dstRect.top + 1;
				dstRect.top = dstRect.top % i;
				if (dstRect.top > 0) dstRect.top -= i;
				dstRect.bottom = dstRect.top + i - 1;
			};
	
			dstRect.left += m_phantomBound.left;
			dstRect.right += m_phantomBound.left;
			dstRect.top += m_phantomBound.top;
			dstRect.bottom += m_phantomBound.top;
	
			region *newClip = newregion();
			region *oldClip = newregion();
			grGetLocalClip(theContext,oldClip);
			if (empty_region(oldClip)) { // This means there's no local clipping region
				kill_region(oldClip);
				oldClip = NULL;
				copy_region(VisibleRegion(),newClip);
			} else {
				and_region(VisibleRegion(),oldClip,newClip);
			}
			grClipToRegion(theContext,newClip);
			kill_region(newClip);
	
			bitmap->LockPixels();
	
			if (m_backgroundBitmap->options & B_TILE_BITMAP) {
				rect dstRectX,dstRectY;
				dstRectY = dstRect;
				int32 bottom = dstRectY.bottom;
				if (m_backgroundBitmap->options & B_TILE_BITMAP_Y) bottom = m_phantomBound.bottom;
				int32 right = dstRectY.right;
				if (m_backgroundBitmap->options & B_TILE_BITMAP_X) right = m_phantomBound.right;
				while (dstRectY.top <= bottom) {
					dstRectX = dstRectY;
					while (dstRectX.left <= right) {
						fdstRect.left = dstRectX.left;
						fdstRect.right = dstRectX.right;
						fdstRect.top = dstRectX.top;
						fdstRect.bottom = dstRectX.bottom;
						grDrawPixels(theContext,srcRect,fdstRect,&bitmap->Canvas()->pixels);
						tmpI = dstRectX.right;
						dstRectX.right = tmpI + tmpI - dstRectX.left + 1;
						dstRectX.left = tmpI + 1;
					};
					tmpI = dstRectY.bottom;
					dstRectY.bottom = tmpI + tmpI - dstRectX.top + 1;
					dstRectY.top = tmpI + 1;
				};
				dstRect.bottom = bottom;
				dstRect.right = right;
			} else {
				fdstRect.left = dstRect.left;
				fdstRect.right = dstRect.right;
				fdstRect.top = dstRect.top;
				fdstRect.bottom = dstRect.bottom;
				grDrawPixels(theContext,srcRect,fdstRect,&bitmap->Canvas()->pixels);
			};
	
			if ((myFlags & clearSolid) && IsClientView())
			{
				region *bitmapRegion = newregion();
				region *result = newregion();
				set_region(bitmapRegion,&dstRect);
				sub_region(VisibleRegion(),bitmapRegion,result);
				grSetForeColor(theContext, view_color);
				grFillRegion(theContext, result);
				kill_region(bitmapRegion);
				kill_region(result);
			};
	
			bitmap->UnlockPixels();
	
			grClipToRegion(theContext, oldClip);	// oldClip=NULL, removes the region.
			if (oldClip)
				kill_region(oldClip);
		}
	}
	else if (myFlags & clearSolid)
	{
		color:
		if (clearFlags & clearSolid) {
			if (!(rules & FAST)) {
				grSetForeColor(theContext, view_color);
				grFillRegion(theContext, VisibleRegion());
			}
		}
	}
	
	return myFlags;
}

int32 TView::ClearGather(	region **regionToClear,
							RenderContext *theContext,
							region **tmpRegion,
							BArray<ViewInfo> *viewList,
							uint32 updateCauses,
							int32 stage,
							TView **startBuffering,
							uint32 gatherFlags)
{
	if (stage > clearStageWindow)
		return stage;

	int32 nextStage = clearStageWindow+1;
	int32 listIndex=0;
	uint32 myFlags;
	uint8 flags = s_clearStages[updateCauses][stage];
	if ((stage == clearStageWindow) && DrawnIn()) flags = clearAll;

	// We can't erase a double-buffered view here, because its bitmap doesn't exists yet.
	if (DoubleBufferRules() & updateCauses)
		flags = 0;
	
	if (rect_in_region(*regionToClear, &m_phantomBound)) {
		myFlags = Clear(theContext, flags);

		nextStage = stage+1;
		while ((nextStage < clearStageWindow) && !(myFlags & s_clearStages[updateCauses][nextStage])) {
			nextStage++;
		}
	
		if (nextStage > clearStageWindow) nextStage = -1;

		if (tmpRegion) {
			region *vis = VisibleRegion();
			if (rules & NO_CLIP_TO_CHILDREN) {
				//	Our visible region includes our children, but one or more
				//	of them might want a background, so we need to subtract them
				//	from what we are subtracting from the regionToClear.
				vis = newregion();
				copy_region(VisibleRegion(),vis);
				region *tmpRegion2 = newregion();
				rules &= ~NO_CLIP_TO_CHILDREN;
				SubAllChildren(*tmpRegion,tmpRegion2,vis);
				rules |= NO_CLIP_TO_CHILDREN;
				kill_region(tmpRegion2);
			}
			sub_region(*regionToClear, vis, *tmpRegion);
			region *old = *regionToClear;
			*regionToClear = *tmpRegion;
			*tmpRegion = old;
			if (rules & NO_CLIP_TO_CHILDREN)
				kill_region(vis);
		}

		if (viewList) {
			listIndex = viewList->CountItems();
			viewList->AddItem(ViewInfo(this,NULL));
		}

		int32 childNextStage;
		TView *child = first_child;
		TView *myStartBuffering = NULL;
		uint32 childFlags = gatherFlags;
		if (DoubleBufferRules() & updateCauses) childFlags |= gatherDoubleBuffering;
		while (child) {
			childNextStage = child->ClearGather(regionToClear,theContext,tmpRegion,
				viewList,updateCauses,stage,&myStartBuffering,childFlags);
			if (childNextStage < nextStage)
				nextStage = childNextStage;
			child = child->next_sibling;
		}

		if (viewList) {
			(*viewList)[listIndex].startBuffering =
				StartDoubleBuffer(updateCauses,myStartBuffering,startBuffering,gatherFlags);
		}
	}	
	return nextStage;
}

/*-------------------------------------------------------------*/

void TView::GatherBackgrounds(region *regionToClear, void *_bitmapList, uint32 updateCauses, int32 stage)
{
	BArray<SBitmap*> *bitmapList = (BArray<SBitmap*> *)_bitmapList;
//	if (!WillDraw()) goto skip;
	if (rect_in_region(regionToClear, &m_phantomBound)) {
		uint8 flags = s_clearStages[updateCauses][stage];
		if ((stage == clearStageWindow) && DrawnIn()) flags = clearAll;

		if ((flags & clearBitmaps) && m_backgroundBitmap && !(m_backgroundBitmap->options & AS_OVERLAY_BITMAP)) {
			m_backgroundBitmap->bitmap->ServerLock();
			bitmapList->AddItem(m_backgroundBitmap->bitmap);
		};

		TView *child = first_child;	
		while (child) {
			child->GatherBackgrounds(regionToClear,bitmapList,updateCauses,stage);
			child = child->next_sibling;
		};
	};
};

void TView::ClearViewBitmap()
{
	if (!m_backgroundBitmap) return;
	if (m_backgroundBitmap->options & AS_OVERLAY_BITMAP)
		m_backgroundBitmap->bitmap->OverlayFor()->Unconfigure();
	m_backgroundBitmap->bitmap->ServerUnlock();
	delete m_backgroundBitmap;
	m_backgroundBitmap = NULL;
};

status_t TView::SetViewBitmap(	SBitmap *bitmap,
								frect srcRect, frect dstRect,
								uint32 followFlags, uint32 options)
{
	if (options & AS_OVERLAY_BITMAP) {
		if (!bitmap->IsOverlay()) return B_ERROR;
		options &= ~B_TILE_BITMAP;
	};

	SBitmap *toRelease = NULL;
	if (m_backgroundBitmap) {
		/*	We already have a background bitmap, so
			we need to get rid of it first. */
		
		if (m_backgroundBitmap->bitmap != bitmap) {
			if (m_backgroundBitmap->options & AS_OVERLAY_BITMAP) {
				bitmap->OverlayFor()->StealChannelFrom(m_backgroundBitmap->bitmap->OverlayFor(),options);
			};
		};
		toRelease = m_backgroundBitmap->bitmap;
	} else
		m_backgroundBitmap = new ViewBitmap;
	
	bitmap->ServerLock();
	m_backgroundBitmap->bitmap = bitmap;
	m_backgroundBitmap->srcRect = srcRect;
	m_backgroundBitmap->dstRect = dstRect;
	m_backgroundBitmap->originalViewBounds = m_phantomBound;
	m_backgroundBitmap->followFlags = followFlags;
	m_backgroundBitmap->options = options;
	UpdateBackgroundBitmap();
	
	if (toRelease) toRelease->ServerUnlock();

	return m_backgroundBitmap->error;
};

void TView::UpdateOverlays()
{
	if (m_backgroundBitmap && (m_backgroundBitmap->options & AS_OVERLAY_BITMAP))
		UpdateBackgroundBitmap();

	TView *child = first_child;	
	while (child) {
		child->UpdateOverlays();
		child = child->next_sibling;
	};
};

void TView::UpdateBackgroundBitmap()
{
	if (!m_backgroundBitmap) return;
	
	float newc,oldc;
	int32 delta_top,delta_left,delta_bottom,delta_right;
	uint32 frules = m_backgroundBitmap->followFlags;
	rect oldBounds = m_backgroundBitmap->originalViewBounds;
	frect dstRect = m_backgroundBitmap->dstRect;
	rect idstRect;

	delta_top		= m_phantomBound.top - oldBounds.top;
	delta_left		= m_phantomBound.left - oldBounds.left;
	delta_bottom	= m_phantomBound.bottom - oldBounds.bottom;
	delta_right		= m_phantomBound.right - oldBounds.right;
	
	if (m_backgroundBitmap->options & B_TILE_BITMAP_Y) {
		float yratio = m_phantomBound.bottom - m_phantomBound.top + 1;
		yratio /= (oldBounds.bottom - oldBounds.top + 1);

		switch((frules >> 12) & 0xf) {
			case POS_TOP:	switch((frules >> 4) & 0xf) {
								case POS_TOP:
									dstRect.top += delta_top;
									dstRect.bottom += delta_top;
									break;
								case POS_BOTTOM:
								case POS_CENTER:
									dstRect.top *= yratio;
									dstRect.bottom *= yratio;
									break;
							};
							break;
			case POS_BOTTOM:switch((frules >> 4) & 0xf) {
								case POS_TOP:
								case POS_CENTER:
									dstRect.top *= yratio;
									dstRect.bottom *= yratio;
									break;
								case POS_BOTTOM:
									dstRect.top += delta_bottom;
									dstRect.bottom += delta_bottom;
									break;
							};
							break;
			case POS_CENTER:switch((frules >> 4) & 0xf) {
								case POS_TOP:
								case POS_BOTTOM:
									dstRect.top *= yratio;
									dstRect.bottom *= yratio;
									break;
								case POS_CENTER:
									newc = ((m_phantomBound.top + m_phantomBound.bottom)/2.0) - m_phantomBound.top;
									oldc = ((oldBounds.top + oldBounds.bottom)/2.0) - oldBounds.top;
									dstRect.top += newc - oldc;
									dstRect.bottom += newc - oldc;
									break;
							};
							break;
		};
		idstRect.top = (int32)floor(dstRect.top+0.5);
		idstRect.bottom = (int32)floor(dstRect.bottom+0.5);
		if (idstRect.top > idstRect.bottom)
			idstRect.top = idstRect.bottom = (idstRect.top + idstRect.bottom)/2;
	} else {
		switch((frules >> 12) & 0xf) {
			case POS_TOP	: break;
			case POS_BOTTOM	: dstRect.top += delta_bottom - delta_top; break;
			case POS_CENTER	:
				newc = ((m_phantomBound.top + m_phantomBound.bottom)/2.0) - m_phantomBound.top;
				oldc = ((oldBounds.top + oldBounds.bottom)/2.0) - oldBounds.top;
				dstRect.top += newc - oldc;
				break;
		};
		
		switch((frules >> 4) & 0xf) {
			case POS_TOP	: break;
			case POS_BOTTOM	: dstRect.bottom += delta_bottom - delta_top;	break;
			case POS_CENTER	:
				newc = ((m_phantomBound.top + m_phantomBound.bottom)/2.0) - m_phantomBound.top;
				oldc = ((oldBounds.top + oldBounds.bottom)/2.0) - oldBounds.top;
				dstRect.bottom += newc - oldc;
				break;
		};

		idstRect.top = (int32)floor(dstRect.top+0.5);
		idstRect.bottom = (int32)floor(dstRect.bottom+0.5);
	};
		
	if (m_backgroundBitmap->options & B_TILE_BITMAP_X) {
		float xratio = m_phantomBound.right - m_phantomBound.left + 1;
		xratio /= (oldBounds.right - oldBounds.left + 1);

		switch((frules >> 8) & 0xf) {
			case POS_LEFT:	switch(frules & 0xf) {
								case POS_LEFT:
									dstRect.left += delta_left;
									dstRect.right += delta_left;
									break;
								case POS_RIGHT:
								case POS_CENTER:
									dstRect.left *= xratio;
									dstRect.right *= xratio;
									break;
							};
							break;
			case POS_RIGHT:	switch(frules & 0xf) {
								case POS_LEFT:
								case POS_CENTER:
									dstRect.left *= xratio;
									dstRect.right *= xratio;
									break;
								case POS_RIGHT:
									dstRect.left += delta_right;
									dstRect.right += delta_right;
									break;
							};
							break;
			case POS_CENTER:switch(frules & 0xf) {
								case POS_LEFT:
								case POS_RIGHT:
									dstRect.left *= xratio;
									dstRect.right *= xratio;
									break;
								case POS_CENTER:
									newc = ((m_phantomBound.left + m_phantomBound.right)/2.0) - m_phantomBound.left;
									oldc = ((oldBounds.left + oldBounds.right)/2.0) - oldBounds.left;
									dstRect.left += newc - oldc;
									dstRect.right += newc - oldc;
									break;
							};
							break;
		};
		
		idstRect.left = (int32)floor(dstRect.left+0.5);
		idstRect.right = (int32)floor(dstRect.right+0.5);
		if (idstRect.left > idstRect.right)
			idstRect.left = idstRect.right = (idstRect.left + idstRect.right)/2;
	} else {
		switch((frules >> 8) & 0xf) {
			case POS_LEFT	: break;
			case POS_RIGHT	: dstRect.left += delta_right - delta_left;		break;
			case POS_CENTER	:
				newc = ((m_phantomBound.left + m_phantomBound.right)/2.0) - m_phantomBound.left;
				oldc = ((oldBounds.left + oldBounds.right)/2.0) - oldBounds.left;
				dstRect.left += newc - oldc;
				break;
		};
		
		switch(frules & 0xf) {
			case POS_LEFT	: break;
			case POS_RIGHT	: dstRect.right += delta_right - delta_left;	break;
			case POS_CENTER	:
				newc = ((m_phantomBound.left + m_phantomBound.right)/2.0) - m_phantomBound.left;
				oldc = ((oldBounds.left + oldBounds.right)/2.0) - oldBounds.left;
				dstRect.right += newc - oldc;
				break;
		};
		
		idstRect.left = (int32)floor(dstRect.left+0.5);
		idstRect.right = (int32)floor(dstRect.right+0.5);
	};
	
	m_backgroundBitmap->finalDst = idstRect;
	m_backgroundBitmap->error = B_OK;
	if (m_backgroundBitmap->options & AS_OVERLAY_BITMAP) {
		rect isrcRect;
		isrcRect.left = (int32)m_backgroundBitmap->srcRect.left;
		isrcRect.right = (int32)m_backgroundBitmap->srcRect.right;
		isrcRect.top = (int32)m_backgroundBitmap->srcRect.top;
		isrcRect.bottom = (int32)m_backgroundBitmap->srcRect.bottom;
		LocalToWindow(&idstRect);
		idstRect.left += owner->ClientBound()->left;
		idstRect.top += owner->ClientBound()->top;
		idstRect.right += owner->ClientBound()->left;
		idstRect.bottom += owner->ClientBound()->top;
		m_backgroundBitmap->error = m_backgroundBitmap->bitmap->OverlayFor()->Configure(isrcRect,idstRect,m_backgroundBitmap->options);
	};
};

#if AS_DEBUG

extern DebugInfo debugSettings;

void TView::DumpDebugInfo(DebugInfo *d)
{
	if (!d) d = &debugSettings;
	DebugPrintf(("TView(0x%08x)++++++++++++++++++++\n",this));
	DebugPrintf(("  bound           = (%d,%d,%d,%d)\n",bound.left,bound.top,bound.right,bound.bottom));
	DebugPrintf(("  m_phantomBound  = (%d,%d,%d,%d)\n",m_phantomBound.left,m_phantomBound.top,m_phantomBound.right,m_phantomBound.bottom));
	DebugPrintf(("  server_token    = %d\n",server_token));
	DebugPrintf(("  client_token    = %d\n",client_token));
	DebugPrintf(("  rules           = %08x\n",rules));
	DebugPrintf(("  m_flags         = %08x\n",(int32)m_flags));

	if (d->debugFlags & DFLAG_VIEW_VERBOSE) {
		DebugPrintf(("  (offset)        = (%f,%f)\n",offset_h,offset_v));
		DebugPrintf(("  (family)        = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",owner,parent,next_sibling,first_child));
		DebugPrintf(("  m_signature     = '%.4s'\n",&m_signature));
		DebugPrintf(("  m_rFlags        = 0x04x\n",(uint32)m_rFlags));
		DebugPrintf(("  view_color      = %08x\n",*((uint32*)&view_color)));
		ShowRegion((VisibleRegion(),"visible"));
	};

	if (renderContext && (d->debugFlags & DFLAG_VIEW_TO_CONTEXT)) {
		DebugPrintf(("  renderContext --> {\n"));
		DebugPushIndent(4);
		grContextDebugOutput(renderContext);
		DebugPopIndent(4);
		DebugPrintf(("  }\n"));
	} else {
		DebugPrintf(("  renderContext   = 0x%08x\n",renderContext));
	};

	if (d->debugFlags & DFLAG_VIEW_TRAVERSE) {
		if (first_child) {
			DebugPrintf(("  children --> {\n"));
			TView *child = first_child;
			DebugPushIndent(4);
			while (child) {
				child->DumpDebugInfo(d);
				child = child->next_sibling;
			};
			DebugPopIndent(4);
			DebugPrintf(("  }\n"));
		} else DebugPrintf(("  No children\n"));
	};
	DebugPrintf(("TView(0x%08x)--------------------\n",this));
};
#endif
