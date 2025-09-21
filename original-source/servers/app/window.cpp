// ******************************************************************************
//
//	File: window.cpp
//
//	Description: TWindow implementation
//	
//	Written by:	Benoit Schillings, Eric Knight,
//				George Hoffman, Dianne Hackborn, Mathias Agopian
//
//	Copyright 1993 - 2001, Be Incorporated
//
// ******************************************************************************

// ------------------------------------------------------------
// Window hierarchy
//
//	TWindow *windowlist;
//		|
//		|
//		|		NULL
//		|		^
//		|		|
//		|		o-------------------+
//		|		|					|
//		|	+---o-------+		+---o-------+
//		|	|m_parent	|		|m_parent	|
//		+-->|nextwindow	|------>|nextwindow	|------> NULL
//			|m_child	|		|m_child	|
//			+---o-------+		+---o-------+
//				|					|	^
//				|					|	|
//				V					|	+---o-------------------+
//				NULL				|		|					|
//									|	+---o-------+		+---o-------+
//									|	|m_parent	|		|m_parent	|
//									+-->|nextwindow	|------>|nextwindow	|------> NULL
//										|m_child	|		|m_child	|
//										+---o-------+		+---o-------+
//											|					|
//											V					V
//											NULL				NULL
//
//
// ------------------------------------------------------------

#include <OS.h>
#include <TLS.h>
#include <Errors.h>
#include <StreamIO.h>
#include <string.h>
#include <unistd.h>

#include "app.h"
#include "as_debug.h"
#include "as_support.h"
#include "bitmap.h"
#include "cursor.h"
#include "DecorState.h"
#include "DecorTypes.h"
#include "debug_update.h"
#include "graphicDevice.h"
#include "dwindow.h"
#include "eventport.h"
#include "input.h"
#include "messages.h"
#include "newPicture.h"
#include "nuDecor.h"
#include "proto.h"
#include "off.h"
#include "rect.h"
#include "render.h"
#include "renderUtil.h"
#include "sam.h"
#include "scroll_bar.h"
#include "server.h"
#include "window.h"

#define dprintf _sPrintf

#undef DEBUG
#define	DEBUG	0
#include <Debug.h>

// ------------------------------------------------------------
// Globals

static	int32				atomic_transaction_level = 0;

static	TWindow *			visibility_windowlist;	// fully ordered list of all windows
static	TWindow *			back_visibility_windowlist; // first is the backmost window
static	TWindow	*			old_windowlist;			// used for reordering.
		TWindow *			windowlist;				// ordered list of top windows
		TWindow	*			desk_window = NULL;
		TWindow	*			front_limit = NULL;
		TWindow	*			last_top_window = NULL;
		region	*			wmgr_tmp;

extern int32				trackThread;
extern	TCursor	*			the_cursor;
extern "C" long				cur_switch;
extern	rgb_color			desk_rgb[MAX_WORKSPACE];
extern	display_mode		ws_res[MAX_WORKSPACE];

		long				last_ws = 0;
		long				ws_count = WS_COUNT;
		bool				front_protection = false;
		region *			desktop_region_refresh = NULL;	// region to be regresh on desktop during visibility
		TWS *				workspace = NULL;
		sem_id				switch_sem;


inline fpoint nfpoint(float h, float v) { fpoint p; p.h = h; p.v = v; return p; };

            region		*calc_desk();
			void		switcher(long i);
static		region		*sum_all();

static void	_pre_visibility_tag();
static void	_visibility_simulator(bool unlock=false);
static void	_pre_ordering_tag();
static void	_ordering_simulator();
static void	_ordering_executor();
static void	_ordering_region_update(TWindow *max_window,
									TWindow *old_next_window,
									TWindow *prev_max,
									region	*redraw_desktop);
static void	_pre_activation_tag();
static bool	_activation_simulator();
static void	_activation_executor();

#if AS_DEBUG
static void dump_window_list();
#endif

//-----------------------------------------------------------

void _pre_visibility_tag()
{
	// The visibility_windowlist has no meaning inside the transaction
	// until the ordering_exector has been called
	visibility_windowlist = NULL;
	back_visibility_windowlist = NULL;

	TWindow	*a_window = windowlist;	
	while (a_window) {
		a_window->m_newClientBound.right = -1;
		a_window->m_newClientBound.left = 0;
		a_window->m_clientUpdateRegion = NULL;
		a_window->m_newDecorState = NULL;
		a_window->m_transactFlags = 0;
		a_window->m_already_blitted = false;
		if (a_window->is_visible())
			a_window->m_transactFlags = TFLAGS_IS_VISIBLE;
		a_window->force_full_refresh = false;
		a_window = a_window->NextWindowFlat();
	}	
	desktop_region_refresh = newregion();
}

//-----------------------------------------------------------

void _visibility_simulator(bool unlock)
{
	TWindow::_visibility_executor(unlock);
}

//-----------------------------------------------------------

uint32 TWindow::RegionsChanged()
{
	return (
		(m_newClientBound.left <= m_newClientBound.right) ||
		(((m_transactFlags & TFLAGS_IS_VISIBLE)!=0) != is_visible()) ||
		(m_transactFlags & TFLAGS_REGIONS_CHANGED)
	);
}

void lock_direct_regions()
{
	TWindow *a_window = windowlist;	
	while (a_window) {
		if (a_window->dw != NULL) a_window->dw->Before();
		a_window = a_window->NextWindowFlat();
	}

	a_window = windowlist;	
	while (a_window) {
		if (a_window->dw != NULL) a_window->dw->Acknowledge();
		a_window = a_window->NextWindowFlat();
	}
}

void unlock_direct_regions()
{
	TWindow *a_window = windowlist;	
	while (a_window) {
		if (a_window->dw != NULL) a_window->dw->After();
		a_window = a_window->NextWindowFlat();
	}
	a_window = windowlist;	
	while (a_window) {
		if (a_window->dw != NULL) a_window->dw->Acknowledge();
		a_window = a_window->NextWindowFlat();
	}
}

void TWindow::_visibility_executor(bool unlock)
{
	bool need_force_move = false;
	bool need_execution = false;
	int32 coverageCount = 0;
	region *tmp;

	RenderContext *ctxt = ThreadRenderContext();

	TWindow *w = visibility_windowlist;	
	while (w) {
		// mathias - 01/29/2000: This may look a little silly, but it's not:
		// first, we must call PartialCommit() and/or Layout() if needed in order to
		// use NeedAdjustLimits()/AdjustLimits(), which in turn, may require to call
		// PartialCommit() and/or Layout() again.

		if ((w->m_newDecorState) && (w->m_transactFlags & TFLAGS_TRANSACTION_OPEN)) {

			if (w->m_transactFlags & TFLAGS_NEED_COMMIT)
				w->m_newDecorState->PartialCommit();

			if (w->m_transactFlags & TFLAGS_NEED_LAYOUT) {
				// Note that we will compute this again below,
				// so don't save the resulting region here.
				if (w->m_newDecorState->Layout(NULL))
					w->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
				w->m_transactFlags |= TFLAGS_NEED_DRAW;
			}

			if (w->m_newDecorState->NeedAdjustLimits()) {
				w->m_minWidth = w->min_h;
				w->m_minHeight = w->min_v;
				w->m_maxWidth = w->max_h;
				w->m_maxHeight = w->max_v;
				w->m_newDecorState->AdjustLimits(&w->m_minWidth,&w->m_minHeight,&w->m_maxWidth,&w->m_maxHeight);
		
				// mathias - 12/10/2000: If we adjust the decor's limit, the window must be resized
				// so that it's internal state doesn't mismatch it's bounds rect. This fixes the
				// Tracker's status view with EXP's decor. See TWindow::set_min_max().
				rect r;
				w->get_bound(&r);
				w->size_to(r.right - r.left, r.bottom - r.top);
			}

			if (w->m_transactFlags & TFLAGS_NEED_COMMIT)
				w->m_newDecorState->PartialCommit();

			if (w->m_transactFlags & TFLAGS_NEED_LAYOUT) {
				if (w->m_newDecorState->Layout(&w->m_clientUpdateRegion))
					w->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
				w->m_transactFlags |= TFLAGS_NEED_DRAW;
			}
		}

		need_execution = need_execution || w->RegionsChanged() || (w->nextwindow != w->old_nextwindow);
		w = w->next_ordered_window;
	}

	//	DebugPrintf(("checked everyone: %s\n",need_execution?"need to execute":"nothing to do"));
	if (need_execution) {
		rect *r;
		int32 dh,dv;
		#define swap(a,b) { swapper = (a); (a) = (b); (b) = swapper; };
		region *beingExposed = newregion();
		region *beingCovered = newregion();
		region *coverage = newregion();
		region *coverageMask = newregion();
		region *newVisible = newregion();
		region *tmp1 = newregion();
		region *tmp2 = newregion();
		region *tmp3 = newregion();
		region *parent_clipping_region = newregion();
		region *swapper;
	
		// Special case, here we don't need to use the sorted list
		// and so we're free to use the hierachical list (windowlist, NextWindowFlat())
		// That way, we can compute on the fly the right clipping regions for the children
		TWindow *w = windowlist;
		while (w) {
			TWindow *parent = w->m_parent;

			if ((parent && (parent->RegionsChanged())) || w->RegionsChanged()) {
				coverageCount++;
				w->m_newVisibleRegion = newregion();
	
				r = &w->window_bound;
				if (w->m_newClientBound.right >= w->m_newClientBound.left) {
					if ((w->m_newClientBound.left != w->window_bound.left) ||
						(w->m_newClientBound.top != w->window_bound.top))
						w->m_transactFlags |= TFLAGS_BOUNDS_MOVED;
					if (((w->m_newClientBound.right - w->m_newClientBound.left) != (w->window_bound.right - w->window_bound.left)) ||
						((w->m_newClientBound.bottom - w->m_newClientBound.top) != (w->window_bound.bottom - w->window_bound.top)))
						w->m_transactFlags |= TFLAGS_BOUNDS_SIZED;
					r = &w->m_newClientBound;
				}

				dh = r->left - w->window_bound.left;
				dv = r->top - w->window_bound.top;

				offset_region(w->BadRegion(), dh, dv);
				offset_region(w->ClearRegion(), dh, dv);
				w->OffsetRegions(dh, dv);

				if (parent) { // Compute the parent clipping region if we're a child
					region *r = (parent->m_newVisibleRegion) ? (parent->m_newVisibleRegion) : (parent->m_fullRegion);
					and_region(parent->m_clientRegion, r, parent_clipping_region);
				}

				if ((parent && (parent->m_transactFlags & TFLAGS_REGIONS_CHANGED)) || (w->m_transactFlags & TFLAGS_REGIONS_CHANGED)) {
					region *tmp4;
					kill_region(w->m_clientRegion);
					if (w->m_newDecorState) w->m_newDecorState->GetRegions(&w->m_clientRegion,&tmp4);
					else w->Decor()->GetRegions(&w->m_clientRegion,&tmp4);
					if (w->m_clipToPicture) {
						fpoint p;
						rect rasterRect = *r;
						p.h = r->left + w->m_clipToPictureOffset.h;
						p.v = r->top + w->m_clipToPictureOffset.v;
						w->m_clipToPicture->Rasterize(ctxt,w->m_clipToPictureFlags,true,&rasterRect,p,1.0,tmp1);
						p.h = p.v = 0;
						and_region(w->m_clientRegion,tmp1,tmp2);
						swap(tmp2,w->m_clientRegion);
					}
					or_region(w->m_clientRegion,tmp4,w->m_fullRegion);
					kill_region(tmp4);					
					if (parent) { // Apply the parent clipping region to fullRegion
						copy_region(w->m_fullRegion, tmp2);
						and_region(parent_clipping_region, tmp2, w->m_fullRegion);
						w->m_transactFlags |= TFLAGS_REGIONS_CHANGED; // We will need to compute our children clipping region
					}
				}
			
				if (w->is_visible()) {
					and_region(w->m_fullRegion,ScreenPort(0)->portClip,w->m_newVisibleRegion);
					if (parent) { // Apply the parent clipping region to m_newVisibleRegion
						copy_region(w->m_newVisibleRegion, tmp2);
						and_region(parent_clipping_region, tmp2, w->m_newVisibleRegion);
						w->m_transactFlags |= TFLAGS_REGIONS_CHANGED; // We will need to compute our children clipping region
					}
				} else {
					clear_region(w->m_newVisibleRegion);
				}

				or_region(w->m_newVisibleRegion,coverageMask,tmp1);

				swap(tmp1,coverageMask);
			} else {
				w->m_newVisibleRegion = NULL;
			}
			w = w->NextWindowFlat();
		}

		w = visibility_windowlist;
		while (w) {
			if (w->m_newVisibleRegion) {
				coverageCount--;
				sub_region(w->m_newVisibleRegion,coverage,newVisible);
	
				sub_region(w->VisibleRegion(),newVisible,tmp3);	// tmp3 = region newly exposed by this window
				sub_region(tmp3,beingCovered,tmp1);				// tmp1 = part of that not covered from above
				sub_region(newVisible,w->VisibleRegion(),tmp2);	// tmp2 = region newly covered by this window
	
				if (!empty_region(tmp1)) {
					if (!empty_region(tmp2)) {
						or_region(beingExposed,tmp1,tmp3);
						sub_region(tmp3,tmp2,beingExposed);
						or_region(beingCovered,tmp2,tmp3);
						sub_region(tmp3,tmp1,beingCovered);
						swap(beingCovered,tmp3);
					} else {
						or_region(beingExposed,tmp1,tmp3);
						swap(beingExposed,tmp3);
						sub_region(beingCovered,tmp1,tmp3);
						swap(beingCovered,tmp3);
					}
				} else {
					if (!empty_region(tmp2)) {
						or_region(beingCovered,tmp2,tmp3);
						swap(beingCovered,tmp3);
						sub_region(beingExposed,tmp2,tmp3);
						swap(beingExposed,tmp3);
					}
				}
	
				swap(w->m_newVisibleRegion,newVisible);
			} else {
				if (w->is_visible()) {
					and_region(w->VisibleRegion(),beingCovered,tmp1);
					and_region(w->FullRegion(),beingExposed,tmp2);
		
					if (!empty_region(tmp1) || !empty_region(tmp2)) {
						w->m_newVisibleRegion = newregion();
						if (!empty_region(tmp1)) {
							if (!empty_region(tmp2)) {
								sub_region(w->VisibleRegion(),tmp1,tmp3);
								or_region(tmp3,tmp2,w->m_newVisibleRegion);
								sub_region(beingExposed,tmp2,tmp3);
								swap(beingExposed,tmp3);
							} else {
								sub_region(w->VisibleRegion(),tmp1,w->m_newVisibleRegion);
							}
							sub_region(beingCovered,tmp1,tmp3);
							swap(beingCovered,tmp3);
						} else {
							or_region(w->VisibleRegion(),tmp2,w->m_newVisibleRegion);
							sub_region(beingExposed,tmp2,tmp3);
							swap(beingExposed,tmp3);
						}
					}
				}
			}

			if (coverageCount && w->is_visible()) {
				and_region(coverageMask,w->FullRegion(),tmp1);
				or_region(tmp1,coverage,tmp2);
				swap(tmp2,coverage);
			}
	
			w = w->next_ordered_window;
		}
	
		lock_direct_regions();
		
		w = back_visibility_windowlist;
		while (w) {
			if (w->m_newVisibleRegion) {
				r = NULL;
				if (w->m_newClientBound.right >= w->m_newClientBound.left)
					r = &w->m_newClientBound;
				tmp = w->set_visible_region(w->m_newVisibleRegion, r, w->m_clientUpdateRegion);
				kill_region(w->m_newVisibleRegion);
				w->m_newVisibleRegion = tmp;
				need_force_move = true;
			}
			w = w->next_back_ordered_window;
		}
	
		unlock_direct_regions();
	
		if (!empty_region(desktop_region_refresh) && (!desk_window || desk_window->is_hidden())) {
			region *desk_region = calc_desk();
			or_region(desktop_region_refresh,beingExposed,tmp1);
			and_region(desk_region,tmp1,tmp2);
			desk_fill(tmp2);
			kill_region(desk_region);
		} else if (!empty_region(beingExposed)) {
			desk_fill(beingExposed);
		}
	
		kill_region(beingExposed);
		kill_region(beingCovered);
		kill_region(coverage);
		kill_region(coverageMask);
		kill_region(newVisible);
		kill_region(tmp1);
		kill_region(tmp2);
		kill_region(tmp3);
		kill_region(parent_clipping_region);
	}

//exit:

	w = visibility_windowlist;
	while (w) {
		if (w->force_full_refresh) {
			w->ExposeViews(w->ClientRegion(),redrawExposed);
			need_force_move = true;
		}

		if (w->m_transactFlags & TFLAGS_TRANSACTION_OPEN) {
			w->m_newDecorState->CommitChanges();
			w->m_newDecorState->CloseTransaction();
		}

		if (w->m_newVisibleRegion) {
			if (!empty_region(w->m_newVisibleRegion)) {
				uint8 updateCauses = redrawExposed;
				if (w->m_transactFlags & TFLAGS_BOUNDS_SIZED) updateCauses |= redrawResized;
				w->ExposeViews(w->m_newVisibleRegion,updateCauses);
			}
		}

		w->m_newClientBound = w->window_bound;
		w = w->next_ordered_window;
	}

	if (unlock) {
		atomic_transaction_level--;
		the_server->DowngradeWindowsWriteToRead();
	}

	w = visibility_windowlist;
	while (w) {
		if (w->force_full_refresh ||
			(w->m_transactFlags & TFLAGS_NEED_DRAW) ||
			(w->m_newVisibleRegion && !empty_region(w->m_newVisibleRegion))) {
			grPushState(ctxt);

			if (w->force_full_refresh) {
				ClipThreadPort(w->VisibleRegion());
				w->m_decor->Update(ctxt,w->VisibleRegion());
			} else {

				if (w->m_transactFlags & TFLAGS_NEED_DRAW) {
					ClipThreadPort(w->VisibleRegion());
					w->m_newDecorState->Draw(ctxt);
				}

				if (w->m_newVisibleRegion && !empty_region(w->m_newVisibleRegion)) {
					tmp = newregion();
					and_region(w->VisibleRegion(),w->m_newVisibleRegion,tmp);
					ClipThreadPort(tmp);
					w->m_decor->Update(ctxt,tmp);
					kill_region(tmp);
				}
			}

			ClipThreadPort(NULL);
			grPopState(ctxt);
		}

		if (w->m_newDecorState) {
			w->m_newDecorState->ServerUnlock();
			w->m_newDecorState = NULL;
		}

		if (w->m_clientUpdateRegion) {
			kill_region(w->m_clientUpdateRegion);
			w->m_clientUpdateRegion = NULL;
		}

		if (w->m_newVisibleRegion) {
			kill_region(w->m_newVisibleRegion);
			w->m_newVisibleRegion = NULL;
		}

		if (w->m_transactFlags & TFLAGS_BOUNDS_MOVED) {
			BParcel an_event(B_WINDOW_MOVED);
			an_event.AddInt64("when",system_time());
			an_event.AddPoint("where",nfpoint(w->window_bound.left,w->window_bound.top));
			w->EventPort()->ReplaceEvent(&an_event,&w->m_queueIter_move,&w->m_moveMsgToken);
		}
		
		if (w->m_transactFlags & TFLAGS_BOUNDS_SIZED) {
			BParcel an_event(B_WINDOW_RESIZED);
			an_event.AddInt64("when",system_time());
			an_event.AddInt32("width",w->window_bound.right-w->window_bound.left);
			an_event.AddInt32("height",w->window_bound.bottom-w->window_bound.top);
			w->EventPort()->ReplaceEvent(&an_event,&w->m_queueIter_size,&w->m_sizeMsgToken);
		}

		w = w->next_ordered_window;
	}

	kill_region(desktop_region_refresh);

	if (unlock) the_server->UnlockWindowsForRead();

	// simulate a mouse move if there was any change in a visible region
	// and we are not dragging a window [mathias]
	if ((need_force_move) &&
		(the_server->WindowManager()) &&
		(!the_server->WindowManager()->IsDragWindow()))
	{
		the_server->force_mouse_moved();
	}
}

//-----------------------------------------------------------

void _pre_ordering_tag()
{
	// create a linked list of the original order of the window list.
	TWindow *a_window = windowlist;
	old_windowlist = windowlist;
	while (a_window) {
		a_window->old_nextwindow = a_window->nextwindow;
		a_window->old_layer = a_window->layer;
		a_window = a_window->old_nextwindow;
	}	
}

//-----------------------------------------------------------

void _ordering_simulator()
{
	bool		process_completed;
	int32		i, count, simu_count;
	TWindow		*a_window, *b_window, *next_window;
	TWindow		*top, *modal;
	TWindow		*layer_top[9];

	simu_count = 0;
	do {
	// ordering simulation : pass 1 (main layer sorting)
		top = 0;
		a_window = windowlist;
		while (a_window) {
			// hidden windows are never visible, layer 0		
			if (a_window->is_hidden())
				a_window->layer = 0;
			// minimized windows (-> hidden) are not visible, layer 1
			else if (a_window->is_mini)
				a_window->layer = 1;
			// non-hidden system-wide floaters are always ws-visible, layer 7
			else if (a_window->is_system_float()) {
				a_window->layer = 7;
				if (!a_window->is_ws_visible(cur_switch))
					a_window->set_current_ws(true);
			}
			// non-hidden system-wide modals are always ws-visible, layer 6
			else if (a_window->is_system_modal()) {
				a_window->layer = 6;
				if (!a_window->is_ws_visible(cur_switch))
					a_window->set_current_ws(true);
			}
			// other non-hidden non-local windows follows standard ws visibility rules
			else if (!a_window->is_local_window() && !a_window->is_ws_visible(cur_switch))
				a_window->layer = 0;
			// non-hidden ws visible screen windows cover everything, layer 8
			else if (a_window->is_screen()) {
				a_window->layer = 8;
				a_window->fullscreen_scaling();
			}
			// non-hidden ws visible menus have a priority of 3, upgradable to 7.
			else if (a_window->is_menu())
				a_window->layer = 3;
			// non-hidden ws visible desktop window is the lowest visible, layer 2.
			else if (a_window->is_desktop())
				a_window->layer = 2;
			// other non-hidden ws visible windows are dispatched in layer 3.
			// The first non-local one is record as been the front (top).
			else {
				if (!a_window->is_local_float() && (top == 0))
					top = a_window;
				a_window->layer = 3;
			}
			// next
			a_window = a_window->NextWindowFlat();
		}
		
		// if top was assigned  and is not blocked : pass 2a
		if (top) {
			modal = top->has_modal();
			if (modal && (!modal->is_menu()))
				top = NULL;
		}

		last_top_window = top;
		if (top) {
			a_window = windowlist;
			while (a_window) {
				if (a_window->layer == 3) {
 					if (a_window->is_local_modal()) {
						if (a_window->has_subwindow(top))
							a_window->layer = 5;
					}
 					/*else if (a_window->is_menu()) {
						if (a_window->has_subwindow_menu(top))
							a_window->layer = 7;
					}*/
					else if (a_window->is_local_float()) {
						if (a_window->has_subwindow(top)) {
							a_window->set_current_ws(true);
							a_window->layer = 4;
						}
						else {
							a_window->set_current_ws(false);
							a_window->layer = 0;
						}
					}
				}
				// next
				a_window = a_window->nextwindow;
			}
		}
	// otherwise : pass 2b
		else {
			a_window = windowlist;
			while (a_window) {
				// make all local-wide floater invisible.
				if ((a_window->layer == 3) && a_window->is_local_float()) {
					a_window->set_current_ws(false);
					a_window->layer = 0;
				}
				// next
				a_window = a_window->nextwindow;
			}
		}
	// pass 2.5 : (menu promotion)
		a_window = windowlist;
		while (a_window) {
			if (a_window->is_menu() && (a_window->layer >= 3)) {
				if ((top != 0) && (a_window->has_subwindow_menu(top)))
					a_window->layer = 7;
				else {				
					b_window = windowlist;
					while (b_window) {
						if ((b_window->layer >= 3) &&
							(b_window->is_float() || b_window->is_system_modal()))
							if (a_window->has_subwindow_menu(b_window)) {
								a_window->layer = 7;
								break;
							}
						// next
						b_window = b_window->nextwindow;
					}
				}
			}
			// next
			a_window = a_window->nextwindow;
		}

	// pass 3 : (local-wide modal workspace processing)
		// check workspace settings for all local-wide modal 
		a_window = windowlist;
		while (a_window) {
			if (((a_window->layer == 3) || (a_window->layer == 5)) &&
				a_window->is_local_modal()) {
				// Set the visibility of the modal
				if ((a_window->ws_subwindows() & (1<<cur_switch)) ||
					(a_window->first_ws == cur_switch))
					a_window->set_current_ws(true);
				else {
					a_window->set_current_ws(false);
					a_window->layer = 0;
				}
			}
			a_window = a_window->nextwindow;
		}
		
	// pass 4 : (local-wide modal ordering adjustement)
		count = 40;
		do {
			process_completed = true;
			a_window = windowlist;
			while (a_window) {
				if (((a_window->layer == 3) || (a_window->layer == 5)) &&
					(a_window->is_local_modal() || a_window->is_menu())) {
					top = a_window->top_subwindows();
					if (top != 0) {
						// check if the modal is already front
						b_window = a_window;
						while (b_window) {
							if (b_window == top)
								goto already_front;
							b_window = b_window->nextwindow;
						}
						// if not, need to reoder the modal in the window list
						process_completed = false;
						a_window->remove_window();
						if (top == windowlist)
							a_window->insert_window(NULL);
						else {
							b_window = windowlist;
							while (b_window->nextwindow != top)
								b_window = b_window->nextwindow;
							a_window->insert_window(b_window);
						}
						// safety against circular loop dependencies
						count--;
						if (count == 0) {
							process_completed = true;
							break;
						}
				already_front:;
					}
				}
				a_window = a_window->nextwindow;
			}
		} while (!process_completed);
		
	// split the windowlist in sublist per layer (in inverted order)
		for (i=0; i<9; i++)
			layer_top[i] = 0;
		a_window = windowlist;
		while (a_window) {
			next_window = a_window->nextwindow;
			a_window->nextwindow = layer_top[a_window->layer];
			layer_top[a_window->layer] = a_window;
			a_window = next_window;
		}
	
	// merge the sublists (inverting order again) to get a well-sorted list
		windowlist = 0;
		for (i=0; i<9; i++) {
			a_window = layer_top[i];
			while (a_window) {
/*			xprintf("Window %s : flags %08x, look %d, feel %d, act %d -> %d\n",
					a_window->get_name(),
					a_window->flags,
					a_window->proc_id,
					a_window->behavior,
					a_window->state,
					a_window->layer);*/
				next_window = a_window->nextwindow;
				a_window->nextwindow = windowlist;
				windowlist = a_window;
				a_window = next_window;
			}
		}
	} while (!_activation_simulator() && (++simu_count < 3));
//	xprintf("##############################################\n");
			
	// execute the ordering sequence.
	_ordering_executor();

	// do the activation execution afterwards...
	_activation_executor();
}

//-----------------------------------------------------------

void _ordering_executor()
{
	bool		change_needed;
	int32		rank, max, a_rank, new_rank, old_ecart, new_ecart;
	region		*redraw_desktop, *tmp_region;
	TWindow		*a_window, *max_window=NULL, *prev_window, *prev_max=NULL, *old_next_window;

	// invert the old links and the new links
	change_needed = false;
	
	if (windowlist != old_windowlist)
		change_needed = true;
	else if (windowlist != 0)
		if (windowlist->old_layer != windowlist->layer)
			change_needed = true;
		
	a_window = old_windowlist;
	old_windowlist = windowlist;
	windowlist = a_window;
	
	while (a_window) {
		max_window = a_window->nextwindow;
		prev_window = a_window->old_nextwindow;
		if (max_window != prev_window)
			change_needed = true;
		else if (max_window != 0)
			if (max_window->old_layer != max_window->layer)
				change_needed = true;
		a_window->nextwindow = prev_window;
		a_window->old_nextwindow = max_window;
		a_window = prev_window;
	}	
	
	if (change_needed)
	{
		// do reordering as long as both lists are not synchronized
		redraw_desktop = newregion();
		
		while (true) {
			// tag window in the current order.
			rank = 0;
			a_window = windowlist;
			while (a_window) {
				a_window->rank = rank;
				a_window = a_window->nextwindow;
				rank++;
			}
			// untag then in the simulated order, and find the best move
			max = 0;
			
			rank = 0;
			a_window = old_windowlist;
			prev_window = 0;
			while (a_window) {
				// calculate its new rank after move
				if (prev_window == 0)
					new_rank = 0;
				else {
					new_rank = prev_window->rank;
					if (new_rank < a_window->rank)
						new_rank++;
				}
				// calculate the efficiency coefficient of the move
				old_ecart = abs((int)(a_window->rank - rank));
				new_ecart = abs((int)(new_rank - rank));
				a_rank = (old_ecart - new_ecart)*64 + old_ecart;
				if ((a_window->old_layer != a_window->layer) && (a_rank == 0))
					a_rank = 1;
				if (a_rank > max) {
					max = a_rank;
					max_window = a_window;
					prev_max = prev_window;
				}
				// next
				prev_window = a_window;
				a_window = a_window->old_nextwindow;
				rank++;
			}
			// if there is no more window out of order, it's finished. Just need
			// to redraw the real desktop if needed.
			if (max == 0) {
				if (!empty_region(redraw_desktop)) {
					tmp_region = newregion();
					or_region(redraw_desktop, desktop_region_refresh, tmp_region);
					copy_region(tmp_region, desktop_region_refresh);
					kill_region(tmp_region);
				}
				break;
			}
	
			// save a copy of the previous next window (before the move)
			old_next_window = max_window->nextwindow;
				
			// reorder the window in the old list.
			max_window->old_layer = max_window->layer;
			max_window->remove_window();
			max_window->insert_window(prev_max);	
		
			// update regions
			_ordering_region_update(max_window, old_next_window, prev_max, redraw_desktop);
		}	
		
		kill_region(redraw_desktop);
	}

	// Reorder the list with the children if needed
	// We construct a list where the first is the topmost window
	if ((change_needed) || (visibility_windowlist == NULL) || (back_visibility_windowlist == NULL))
	{ // for each top window, we go through its children and we invert that list
		TWindow **patch = &visibility_windowlist;
		TWindow *w = windowlist;
		while (w) {
			TWindow *previous = w;
			TWindow *c = w->m_child;
			while (c) {
				c->next_ordered_window = previous;
				previous = c;
				c->old_nextwindow = NULL; // Force an execution (TODO: we should do that only if needed)
				//c->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
				c = c->NextChild(w);
			}
			*patch = previous;
			patch = &(w->next_ordered_window);
			w = w->nextwindow;
		}
		*patch = NULL;
		
		// And now construct a back-ordered list
		// (first in the list is the backmost window)
		TWindow *previous = NULL;
		w = visibility_windowlist;
		while (w) {
			w->next_back_ordered_window = previous;
			previous = w;
			w = w->next_ordered_window;
		}
		back_visibility_windowlist = previous;
	}
}

// The window <max_window> went from just before <old_next_window> to just after
// <prev_max>. This function will update the region of all windows to take that
// change into account. It will also update the <redraw_desktop> region, in which
// we will sum the region that need to be redraw by the app_server desktop (in
// case there is no desktop window). Last thing, it will update all modified area
// on the screen (except redrawing the uncovered desktop area).
void _ordering_region_update(TWindow *max_window,
							TWindow *old_next_window,
							TWindow *prev_max,
							region	*redraw_desktop)
{
	(void)redraw_desktop;

	max_window->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
	TWindow *w = windowlist;
	TWindow *sentinel;
	
	if (prev_max) {
		while (w && (w != old_next_window) && (w != prev_max)) w = w->nextwindow;
		if (!w) return;

		if (w == old_next_window) {
			sentinel = prev_max->nextwindow;
		} else {
			sentinel = old_next_window;
			w = prev_max->nextwindow;
		}
	} else {
		sentinel = old_next_window;
		w = windowlist;
	}

	while (w && (w != sentinel)) {
		w->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
		w = w->nextwindow;
	}
}

//-----------------------------------------------------------

void _pre_activation_tag()
{
}

//-----------------------------------------------------------

bool _activation_simulator()
{
	TWindow		*a_window, *w;

	// look for a visible window_screen, and make it active
	a_window = windowlist;
	while (a_window) {
		if (a_window->is_screen() &&
			!a_window->is_hidden() &&
			a_window->is_ws_visible(cur_switch))
			goto disactivate_everything_else;
		a_window = a_window->nextwindow;
	}
	
	// look for a visible active LOCK_FOCUS window, then keep it active
	a_window = windowlist;
	while (a_window) {
		if (a_window->locks_focus() &&
			!a_window->is_hidden() &&
			a_window->is_ws_visible(cur_switch) &&
			(a_window->state & wind_active))
			goto disactivate_everything_else;
		a_window = a_window->nextwindow;
	}
	
	// look for the first activated window.
	a_window = windowlist;
	while (a_window) {
		if (a_window->is_menu())
			a_window->state |= wind_active;
		if (a_window->state & wind_active) {
			// case 0 : this window is not visible in the current workspace. A loser...
			if (a_window->is_hidden() || !a_window->is_ws_visible(cur_switch)) {
				a_window->state &= ~wind_active;
				goto find_another;
			}
			// case 1 : it's a system-wide modal : disactivate everything else.
			else if (a_window->is_system_modal()) {
	disactivate_everything_else:
				w = windowlist;
				while (w) {
					if (w != a_window)
						w->state &= ~wind_active;
					w = w->nextwindow;
				}
				a_window->state |= wind_active;
			}
			// case 2a : menu.
			else if (a_window->is_menu())
				goto simulation_complete;
			// case 2b : floater (team or system-wide).
			else if (a_window->is_float()) {
				// disactivate any other floater.
	disactivate_all_floater:
				w = a_window->nextwindow;
				while (w) {
					if (w->is_menu())
						w->state &= ~wind_active;
					w = w->nextwindow;
				}
				// look for top activated non floater-window, and desactivate
				// all others.
				w = a_window->nextwindow;
				while (w) {
					if (w->state & wind_active) {
						while (w->nextwindow) {
							w = w->nextwindow;
							w->state &= ~wind_active;
						}
						goto simulation_complete;
					}
					w = w->nextwindow;
				}
				// no top activated non floater-window. We need to pick the top
				// one behind the current floating window and activate it.
				w = a_window->nextwindow;
				while (w) {
					// first pass, we will not pick a window that doesn't like becoming
					// front and that is a subwindow of the floater
					if (((w->flags & DISLIKE_FRONT) == 0) && a_window->has_subwindow(w)) {
						// bring it front if visible.
						if (!w->is_mini && w->is_ws_visible(cur_switch) && !w->is_hidden()) {
							a_window = w;
							goto bring_to_front_local;
						}
						goto simulation_complete;
					}
					w = w->nextwindow;
				}	
				// second pass, we will pick any window this time, so let's try the first
				w = a_window->nextwindow;
				while (w) {
					// second pass, we will pick any subwindow of the floater
					if (a_window->has_subwindow(w)) {
						// bring it front if visible.
						if (!w->is_mini && w->is_ws_visible(cur_switch) && !w->is_hidden()) {
							a_window = w;
							goto bring_to_front_local;
						}
						goto simulation_complete;
					}
					w = w->nextwindow;
				}	
			}
			// case 3 : any other window : check for a modal, then disactivate
			// everything else.
			else {
				w = a_window->find_top_modal();
				if (w != 0) {
					a_window = w;
					if (w->is_menu())
						goto disactivate_all_floater;
				}
				goto disactivate_everything_else;
			}
			// activation simulation completed
			goto simulation_complete;
		}
	find_another:
		a_window = a_window->nextwindow;
	}

	// no activated window : we need to peek one.
	a_window = windowlist;
	while (a_window) {
		// first pass, we will not pick a window that doesn't like becoming
		// front or system-wide floater, or doesn't focus
		if (((a_window->flags & (DISLIKE_FRONT|DISLIKE_FOCUS)) == 0) &&
			!a_window->is_system_float() &&
			!a_window->is_mini &&
			a_window->is_ws_visible(cur_switch) &&
			!a_window->is_hidden()) {
bring_to_front_local:
			// do a bring to front
			a_window->state |= wind_active;
			a_window->remove_window();
			a_window->insert_window(a_window->get_front_limit());
			return false;
		}
		a_window = a_window->nextwindow;
	}
		
	// second pass, we will not pick a window that doesn't like becoming
	// front or system-wide floater.
	a_window = windowlist;
	while (a_window) {
		if (((a_window->flags & DISLIKE_FRONT) == 0) &&
			!a_window->is_system_float() &&
			!a_window->is_mini &&
			a_window->is_ws_visible(cur_switch) &&
			!a_window->is_hidden())
			goto bring_to_front_local;
		a_window = a_window->nextwindow;
	}
		
	// third pass, we will pick any window this time, so let's try the first
	a_window = windowlist;
	if (a_window)
	if (!a_window->is_mini &&
		a_window->is_ws_visible(cur_switch) &&
		!a_window->is_hidden())
		goto bring_to_front_local;
	
simulation_complete:
	return true;
}

//-----------------------------------------------------------

void _activation_executor()
{
	TWindow	*w = windowlist;
	while (w) {
		w->check_hilite_and_activation();
		w = w->nextwindow;
	}
}

//-----------------------------------------------------------
// #pragma mark -

void open_atomic_transaction()
{
	if (atomic_transaction_level == 0) {	
		_pre_visibility_tag();
		_pre_ordering_tag();
		_pre_activation_tag();
	}
	atomic_transaction_level++;
}

void close_atomic_transaction()
{
	if (atomic_transaction_level == 1) {
		_ordering_simulator();
		_visibility_simulator();
	
		atomic_transaction_level--;
		// check focus follows mouse.
		the_server->focus_follows_mouse();	
	}
	else
		atomic_transaction_level--;
}

void close_atomic_transaction_and_unlock()
{
	if (atomic_transaction_level == 1) {
		_ordering_simulator();
		_visibility_simulator(true);
	
		// check focus follows mouse.
		the_server->focus_follows_mouse();	
	}
	else
		atomic_transaction_level--;
}

void do_atomic_transaction()
{
	open_atomic_transaction();
	close_atomic_transaction();
}

void flush_ordering_transaction()
{
	_ordering_simulator();
	_pre_ordering_tag();
	_pre_activation_tag();
}

//-----------------------------------------------------------
// #pragma mark -

void reset_decor()
{
	wait_regions();
	open_atomic_transaction();

	TWindow *w = windowlist;
	while (w) {
		w->ResetDecor();
		w = w->NextWindowFlat();
	}

	close_atomic_transaction();		
	signal_regions();
}

void reset_activation()
{
	wait_regions();
	open_atomic_transaction();

	TWindow	*w = windowlist;
	while (w) {
		w->state &= ~wind_active;
		w = w->NextWindowFlat();
	}

	close_atomic_transaction();		
	signal_regions();
}

//-----------------------------------------------------------

void set_front_limit(TWindow *window)
{
	front_limit = window;
}

//-----------------------------------------------------------

void window_init()
{
	windowlist = NULL;
	visibility_windowlist = NULL;
	back_visibility_windowlist = NULL;
	switch_sem = create_sem(1, "switch_sem");
	wmgr_tmp = newregion();
}

//-----------------------------------------------------------

// calculate the sum of all full-regions of the visible windows.
static region *sum_all()
{
	// Here it is OK, to use the top window list
	// because children are clipped by their parents
	// (in other words, they're 100% included in them)
	region *tmp_region0 = newregion();
	region *tmp_region1 = newregion();
	TWindow *the_window = windowlist;
	while (the_window) {
		if (the_window->is_visible()) {
			or_region(tmp_region0, the_window->FullRegion(), tmp_region1);
			region *tmp = tmp_region0;
			tmp_region0 = tmp_region1;
			tmp_region1 = tmp;
		}
		the_window = the_window->nextwindow;
	}
	kill_region(tmp_region1);
	return tmp_region0;
}

//-----------------------------------------------------------

region *calc_desk()
{
	region *all_wind = sum_all();
	region *result = newregion();
	sub_region(ScreenPort(0)->portClip, all_wind, result);
	kill_region(all_wind);
	return(result);
}

//-----------------------------------------------------------

bool signature_ok(TWindow *a_window)
{
	return (a_window->valid_signature == WINDOW_SIGNATURE);
}

//-----------------------------------------------------------

bool still_a_good_window(TWindow *the_window)
{
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (a_window == the_window)
			return true;
		a_window = a_window->NextWindowFlat();
	}
	return false;
}

//-----------------------------------------------------------

// Return the window at coordinate (h,v). Or NULL for the desktop
// Only look at the top windows (the parents). Only used for ffm basicaly.
TWindow *find_window(long h, long v)
{
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (point_in_region(a_window->VisibleRegion(), h, v))
			return a_window;
		a_window = a_window->nextwindow;
	}
	return NULL;
}

TWindow *find_window_hierachy(long h, long v)
{
	if (visibility_windowlist == NULL)
	{ // Use the non ordered list (should not happen), but we want to be safe.
		TWindow	*a_window = windowlist;
		while (a_window) {
			if (point_in_region(a_window->VisibleRegion(), h, v))
				return a_window;
			a_window = a_window->NextWindowFlat();
		}
	} else { // Use the ordered list (should always be the case)
		TWindow	*a_window = visibility_windowlist;
		while (a_window) {
			if (point_in_region(a_window->VisibleRegion(), h, v))
				return a_window;
			a_window = a_window->next_ordered_window;
		}
	}
	return NULL;
}

//-----------------------------------------------------------

// return the first active window of the window list, or NULL.
TWindow	*find_target_for_keys()
{
	// We don't send key events to children
	// (TODO: we could change that eventually)
	TWindow	*a_window = windowlist;
	while(a_window) {
		if (a_window->state & wind_active)
			return a_window;
		a_window = a_window->nextwindow;
	}
	return NULL;
}

//-----------------------------------------------------------

void priv_set_workspace_count(long count)
{
	if (count >= ws_count) {
		ws_count = count;
		wait_regions();
		if (workspace)
			workspace->Refresh();
		signal_regions();
	}
	else {
		ws_count = count;
		wait_regions();
		if (workspace)
			workspace->Refresh();
		signal_regions();
		if (ws_count - 1 < cur_switch)
			switcher(ws_count - 1);
	}
}

//-----------------------------------------------------------

void activate_team(long team_id, int32 *h, int32 *v)
{
	int32		i, count;
	uint32		ws_team;
	TWindow		*a_window, *front;
	TWindow		**list_window;

	wait_regions();

	*h = -10000;
	*v = -10000;

	// count how many items are in the windowlist.
	a_window = windowlist;
	count = 0;
	while (a_window != 0) {
		count++;
		a_window = a_window->nextwindow;
	}
	// create a temporary array to store the list of windows
	// to be send front.
	list_window = (TWindow **)grMalloc(count*sizeof(TWindow *), "list_window", MALLOC_CANNOT_FAIL);
	
	// go pick the good windows
	count = 0;
	ws_team = 0;
	a_window = windowlist;
	while (a_window != 0) {
		if ((a_window->team_id == team_id) &&
			!a_window->is_local_float() &&
			!a_window->is_desktop() &&
			!a_window->is_hidden() &&
			!a_window->is_mini) {
			ws_team |= a_window->get_ws();
			list_window[count++] = a_window;
		}
		a_window = a_window->nextwindow;
	}
	
	// reorder all windows in one ordering transaction.
	open_atomic_transaction();
	
	for (i=count-1; i>=0; i--) {
		a_window = list_window[i];
		front = a_window->get_front_limit();
		if (front == NULL)
			a_window->make_active();
		a_window->remove_window();
		a_window->insert_window(front);		
		a_window->ws_changed();
	}
	
	close_atomic_transaction();

	// try to warp the mouse if we're in focus follows mouse
	if (the_server->target_mouse & 2)
		for (i=0; i<count; i++) {
			a_window = list_window[i];
			if (a_window->warp_mouse(h, v))
				break;
		}
	
	// switch workspace if necessary
	if ((ws_team & (1<<cur_switch)) == 0)
		for (i = 0; i < ws_count; i++)
			if (ws_team & (1<<i)) {
				switcher(i);
				break;
			}
			
	// check focus follows mouse.
//	the_server->focus_follows_mouse();	
				
	grFree(list_window);
	signal_regions();
}

// ---------------------------------------------------------- 
// #pragma mark -

TWindow::TWindow(SApp *application)
	: m_viewLock("viewLock")
{
	m_application = application;
	minimal_setup();
}

TWindow::TWindow(
	SApp *application,
	char *name, rect *bound,
	uint32 design, uint32 inBehavior,
	ulong wflags, ulong ws,	int32 tid,
	port_id eventPort, int32 clientToken,
	TSession *session)
	: m_viewLock("viewLock")
{
	rect	view_bound;
	long	i;	

	m_application = application;

	// commun setup with the default constructor
	minimal_setup();

	// set the signature as being valid.
	valid_signature = WINDOW_SIGNATURE;
	
	// set the attribute flags, wdef_id and behavior per default
	flags = 0;
	team_id = tid;
	proc_id = -1;

	behavior = NORMAL;
	last_pending_update = 0;

	m_fullRegion = newregion();
	m_clearRegion = newregion();
	m_clientRegion = newregion();
	m_visibleRegion = newregion();
	m_preViewPortClip = newregion();
	m_invalidRegion = NULL;
	m_bad_region = newregion();

	// initialize various flags
	m_updateCauses[0] = 0;
	m_updateCauses[1] = 0;
	m_updateCauses[2] = 0;
	state = 0;			// window not active
	show_hide = 1;		// initial state is hidden
	layer = 0;			// hidden means layer 0
	inter_port = -1;
	m_setView = NULL;
	is_mini = false;
	was_mini = false;
	m_viewRFlags = 0xFFFF;
	last_view_token = -1;
	last_view = NULL;
	top_view = 0; 		// activate could be called before top_view has anythings inside 	
	dw = NULL;
	m_transactFlags = 0;

	// initialize the min and max sizes to their extremal values.
	m_minWidth	= min_h = 10;
	m_minHeight	= min_v = 10;
	m_maxWidth	= max_h = 9999;
	m_maxHeight	= max_v = 9999;

	// get a copy of the window name
	window_name = (char *)grMalloc(strlen(name)+1,"window_name",MALLOC_CANNOT_FAIL);
	strcpy(window_name, name);

	// set the first workspace trick for team-wide modal	
	first_ws = -1;

	// clip insane values on boundary
	if (bound->top < -8192)			bound->top = -8192;
	else if (bound->top > 8192)		bound->top = 8192;
	if (bound->left < -8192)		bound->left = -8192;
	else if (bound->left > 8192)	bound->left = 8192;
	if (bound->right < -8192)		bound->right = -8192;
	else if (bound->right > 8192)	bound->right = 8192;
	if (bound->bottom < -8192)		bound->bottom = -8192;
	else if (bound->bottom > 8192)	bound->bottom = 8192;

	// initialise the sub-window list management
	subwindow_list = 0;
	subwindow_count  = 0;

	// protect atomicity of regions and window_port creation
	LockRegionsForWrite();
	
	m_eventPort = new SEventPort(
		m_application->RWHeap(),
		m_application->ROHeap(),
		eventPort,
		clientToken);
	#ifdef REMOTE_DISPLAY
	port_id newport = application->AddEventPipe(
		EventPort()->AtomicVar(),
		EventPort()->FirstEvent(),
		EventPort()->ClientBlock(),
		eventPort);
	if (newport != -1) {
		m_eventPort->SetRealPort(newport);
		m_eventPort->SetTag(EventPort()->ClientBlock());
	};
	#endif

	// init workspace bounds and activation.
	window_bound = *bound;
	if (ws == 0)
		ws = 1<<cur_switch;
	if (is_multi_ws(ws)) {
		ws_ext_bounds = (rect*)grMalloc(sizeof(rect)*MAX_WORKSPACE,"ws_ext_bounds",MALLOC_CANNOT_FAIL);
		for (i = 0; i < MAX_WORKSPACE; i++) {
			ws_ext_bounds[i].left = 0;
			ws_ext_bounds[i].top = 0;
			ws_ext_bounds[i].right = -1;
			ws_ext_bounds[i].bottom = -1;
		}
	} else
		ws_ext_bounds = 0;

	set_ws(ws);

	// insert in the window list
	nextwindow = NULL;
	next_ordered_window = NULL;
	m_parent = NULL;
	m_child = NULL;
	insert_window((TWindow *)-1); // insert in the back of the list
	
	// create the top_view.
	view_bound = *ClientBound();
	offset_rect(&view_bound,-view_bound.left,-view_bound.top);
	top_view = new TView(&view_bound, FOLLOW_BORDERS, 0);
	top_view->SetOwner(this);

	InitRenderStuff();

	// spawn the server-side thread
	char name_buffer[32];
	if (session) {
		fport_id = session->receive_port;
		a_session = session;
		a_session->SetAtom(this);
		a_session->SetClient(this);
	} else {
		sprintf(name_buffer, "p:%.24s", window_name);
		fport_id = create_port(BUF_MSG_SIZE,name_buffer);
	}
	sprintf(name_buffer, "w:%.24s:%ld", window_name, application->TeamID());
	task_id = xspawn_thread(start_task, name_buffer, B_DISPLAY_PRIORITY, this, false);

	client_task_id = 0;

	// identify and enable the workspace window.
	if ((wflags & WS_WINDOW) && (!workspace)) {
		workspace = new TWS(this);
		flags |= WS_WINDOW;
	}

	// set the window regions
	m_newClientBound = window_bound;
	
	// set the standard flags and attributes
	set_flags(design, inBehavior, wflags);

	// end of atomic section
	UnlockRegionsForWrite();

	// menu reset the front limit
	if (is_menu())
		set_front_limit(NULL);

	// record the desk window.
	if (is_desktop())
		desk_window = this;
		
	// check window alignment
	move(0, 0, 0);
	
	inter_port = eventPort;
	client_token = clientToken;	
}

TWindow::~TWindow()
{
	if (a_session) a_session->sclose();

	if (task_id >= B_OK) {
		if (task_id != find_thread(NULL)) {
			#if AS_DEBUG
				if (task_id == trackThread) {
					DebugPrintf(("track(%d): thread %d is killing me!\n",trackThread,getpid()));
					CallStack blah;
					blah.Update();
					blah.Print();
					DebugPrintf(("\n"));
				};
			#endif
	
			wait_for_thread(task_id,&task_id);
			task_id = -1;
		} else {
			#if AS_DEBUG
				if (task_id == trackThread) {
					DebugPrintf(("track(%d): I'm dying!\n",trackThread,getpid()));
					CallStack blah;
					blah.Update();
					blah.Print();
					DebugPrintf(("\n"));
				};
			#endif
		};
	}
	
	dup_release_windowR(this);

	if (renderPort) {
		grDestroyRenderPort(renderPort);
		grFreeRenderPort(renderPort);
		renderPort = NULL;
	}

	if (top_view) {
		top_view->do_close();
		top_view = NULL;
	}
	
	kill_region(m_preViewPortClip);
	kill_region(m_fullRegion);
	kill_region(m_clientRegion);
	kill_region(m_visibleRegion);
	kill_region(m_clearRegion);
	if (m_invalidRegion) kill_region(m_invalidRegion);
	kill_region(BadRegion());	

	delete dw;
	grFree(window_name);
	grFree(ws_ext_bounds);
	grFree(subwindow_list);
	delete m_eventPort;
	if (m_decor) m_decor->ServerUnlock();
	delete a_session;
	if (m_clipToPicture) m_clipToPicture->ServerUnlock();
}

//-----------------------------------------------------------
// #pragma mark -

// Warp the mouse inside the window (to be used only in focus follows mouse mode).
// Return true if it was possible, else if the window was hidden.
bool TWindow::warp_mouse(int32 *h, int32 *v)
{
	bool result;

	// check for a visible area large enough to be a target
	wait_regions();
	
	region *rgn = VisibleRegion();
	// first, look for the largest rect not too thin.
	const int32 count = rgn->CountRects();
	const rect* rects = rgn->Rects();
	int32 imax = -1;
	int32 max = 0;
	for (int i=0; i<count; i++) {
		const rect& frame = rects[i];
		point p;
		the_server->get_mouse(&p.h, &p.h);
		if ((p.h >= frame.left) &&
			(p.h <= frame.right) &&
			(p.v >= frame.top) &&
			(p.v <= frame.bottom)) {
			*h = p.h;
			*v = p.v;
			result = true;
			goto exit;
		}
		const int32 dh = frame.right-frame.left-4;
		const int32 dv = frame.bottom-frame.top-4;
		const int32 size = dh*dv - 25;
		if (size > max) {
			max = size;
			imax = i;
		}
	}
	// if we fail, look for the largest rect
	if (imax == -1)
		for (int i=0; i<count; i++) {
			const rect& frame = rects[i];
			const int32 size = (frame.right-frame.left-1)*(frame.bottom-frame.top-1);
			if (size >= max) {
				max = size;
				imax = i;
			}
		}

	// if we didn't completly fail, move the mouse.
	if (imax != -1) {
		const rect& frame = rects[imax];
		*h = (frame.left + frame.right)/2;
		*v = (frame.bottom + frame.top)/2;
		result = true;
	}
	else
		result = false;
	
exit:
	signal_regions();
	return result;
}

//-----------------------------------------------------------

// return the modal currently "blocking" a given team, or NULL if none.
TWindow	*TWindow::has_modal()
{
	TWindow	*a_window;

	// nothing can block a system-wide floater or modal
	if (is_system_float() || is_system_modal())
		goto end_null;

	// anything else can be blocked by a system modal
	a_window = windowlist;
	while (a_window != 0) {
		if (a_window->is_system_modal() && a_window->is_visible())
			goto end;
		a_window = a_window->nextwindow;
	}
	
	// other windows could be blocked by a local-wide modal
	a_window = windowlist;
	while (a_window != 0) {
 		if ((a_window->is_local_modal() || a_window->is_menu()) &&
			a_window->has_subwindow(this) &&
			a_window->is_visible())
			goto end;
		a_window = a_window->nextwindow;
	}
	
	// in any other case, it's not blocked.
end_null:
	a_window = NULL;
end:
	return a_window;
}

//-----------------------------------------------------------

// return the top modal of the hierarchy of modal window blocking the target.
// if there's multiple top modal, then return one at random.
// if a loop is detected or there is no modal, return NULL.
TWindow	*TWindow::find_top_modal()
{
	// set all tmp variable to 0
	TWindow *a_window = windowlist;
	while (a_window) {
		a_window->w_tmp = 0;
		a_window = a_window->nextwindow;
	}
	
	// set the window as origin, tmp = 1
	w_tmp = 1;
	a_window = this;
	
	// propagate dependencies from there, at random (first found).
	TWindow *b_window;
	while (true) {
		b_window = a_window->has_modal();
		if (b_window == 0)
			break;
		// test if there is a loop in modal dependencies...
		if (b_window->w_tmp == 1)
			return 0L;
		b_window->w_tmp = 1;
		a_window = b_window;
	}	

	// we found a top modal
	return (a_window == this) ? (NULL) : (a_window);
}

//-----------------------------------------------------------

void TWindow::minimal_setup()
{
	dup_create_windowR(this);
	ServerLock(); // one for the window list
	ServerLock(); // 
	ServerLock(); // two for the window thread itself
	m_mouseView = NULL;
	m_eventChain = NULL;
	m_eventMask = 0;
	m_dampFocusedEvents = 0;
	m_decor = NULL;
	m_clipToPicture = NULL;
	a_session = NULL;
	m_statusFlags = 0;
	m_updateBuffer = NULL;
	m_clientUpdateRegion = NULL;
	m_newVisibleRegion = NULL;
	m_newDecorState = NULL;
	m_dbViews = NULL;
	m_ueberPort = NULL;
	
	m_decorMMToken =
	m_sizeMsgToken =
	m_moveMsgToken =
	m_activateMsgToken =
	m_wsMsgToken = NONZERO_NULL;

	m_queueIter_decorMM =
	m_queueIter_move =
	m_queueIter_size =
	m_queueIter_activate =
	m_queueIter_ws = 0xFFFFFFFF;
	
	m_lockFocusCount =
	m_viewTransactionCount =
	m_updateDisabledCount = 0;

	m_lastUpdateTime = 0;
	
	renderPort = NULL;
}

void TWindow::InitRenderStuff()
{
	renderPort = grNewRenderPort();
	grInitRenderPort(renderPort);
#if ROTATE_DISPLAY
	renderPort->canvasIsRotated = &(BGraphicDevice::Device(0)->Port()->canvasIsRotated);
	renderPort->rotater = BGraphicDevice::Device(0)->Port()->rotater;
#endif

	renderCanvas = BGraphicDevice::Device(0)->Canvas();

	//	We're using the view as our magic ptr value, but
	//	the wdef doesn't have a view.  We'll use the top_view,
	//	because we know that it will never draw.
	
	if (BGraphicDevice::Device(0)->IsLameCard()) {
		grSetPortCanvas(renderPort,BGraphicDevice::Device(0),
			(LockRenderCanvas)LockScreenCanvasLameAssCard,
			(UnlockRenderCanvas)UnlockScreenCanvasLameAssCard);
	} else {
		grSetPortCanvas(renderPort,BGraphicDevice::Device(0),
			(LockRenderCanvas)LockScreenCanvas,
			(UnlockRenderCanvas)UnlockScreenCanvas);
	}
	
	renderPort->canvas = renderCanvas;	
}

void TWindow::set_min_max(long minh, long maxh, long minv, long maxv)
{
	// Make sure the parameters are coherent. If they're not,
	// (Need)AdjustLimits() would get confused ("ImageViewer with small images" bug).
	if (maxh < minh)	maxh = minh;
	if (maxv < minv)	maxv = minv;
	m_minWidth	= min_h = minh;
	m_minHeight	= min_v = minv;
	m_maxWidth	= max_h = maxh;
	m_maxHeight	= max_v = maxv;
	Decor()->AdjustLimits(&m_minWidth,&m_minHeight,&m_maxWidth,&m_maxHeight);
	rect r;
	get_bound(&r);
	size_to(r.right - r.left, r.bottom - r.top);
}

// ------------------------------------------------------------

int32 TWindow::remove_subwindow(TWindow *a_window)
{
	LockRegionsForWrite();
	for (int i = 0; i < subwindow_count; i++) {
		if (subwindow_list[i] == a_window) {
			for (int j = (i+1); j < subwindow_count; j++)
				subwindow_list[j-1] = subwindow_list[j];
			subwindow_count--;
			// update ordering and activation
			do_atomic_transaction();
			UnlockRegionsForWrite();
			return B_NO_ERROR;
		}
	}
	UnlockRegionsForWrite();
	return B_ERROR;
}

// ------------------------------------------------------------

bool TWindow::has_subwindow(TWindow *a_window)
{
	if (a_window == this)	return false;
	if (is_team_modal())	return has_subwindow_team_modal(a_window);
 	if (is_menu())	 		return has_subwindow_menu(a_window);
 	if (is_team_float())	return has_subwindow_team_float(a_window);
	if (is_list_window())	return has_subwindow_list_window(a_window);
	return false;
}

// ------------------------------------------------------------

int32 TWindow::add_subwindow(TWindow *a_window)
{
	if (a_window->is_menu())
		return B_ERROR;

	for (int i=0 ; i<subwindow_count ; i++)
		if (subwindow_list[i] == a_window)
			return B_ERROR;

	int new_cnt = subwindow_count + 1;
	TWindow **new_p = (TWindow **)grRealloc(subwindow_list, new_cnt*sizeof(TWindow *), "subwindow_list", MALLOC_LOW);
	if (new_p == 0) return B_ERROR;
	subwindow_list = new_p;
	subwindow_list[subwindow_count] = a_window;
	subwindow_count++;
	// update ordering and activation
	LockRegionsForWrite();
	do_atomic_transaction();
	UnlockRegionsForWrite();
	return B_NO_ERROR;
}

// ------------------------------------------------------------

// for local-wide window, return the ws mask of ws in which one
// of the subwindows in visible.
uint32 TWindow::ws_subwindows()
{
	TWindow	*a_window;
	uint32 ws = 0;
	// Team-wide window. Check all windows of the same team.
	if (is_team_window()) {
		a_window = windowlist;
		while (a_window) {
			if ((a_window->team_id == team_id) &&
				!a_window->is_local_window() &&
				!a_window->is_mini &&
				!a_window->is_hidden())
				ws |= a_window->get_ws();
			a_window = a_window->nextwindow;
		}
	}
	// List-wide window. Check all window in the list.
	if (is_list_window() || is_team_modal()) {
		for (int i=0 ; i<subwindow_count ; i++) {
			a_window = subwindow_list[i];
			if (!a_window->is_local_window() &&
				!a_window->is_mini &&
				!a_window->is_hidden())
				ws |= a_window->get_ws();
		}
	}
	return ws;
}

// ------------------------------------------------------------

// For local-wide window, return the top window of its subwindows.
// Return NULL if no subwindow or no visible subwindows.
TWindow	*TWindow::top_subwindows()
{
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (has_subwindow(a_window) && !a_window->is_mini && !a_window->is_hidden()) 
			return a_window;
		a_window = a_window->nextwindow;
	}
	return NULL;
}

// ------------------------------------------------------------

int32 TWindow::kind_2_proc_id(int32 kind)
{
	switch (kind) {
	case 2 :       // B_MENU_WINDOW
	case 3 :       // B_MODAL_WINDOW
	case 7 :	   // FLOATING WINDOW
	case 11 :      // B_DOCUMENT_WINDOW
	case 20:       // B_BORDERED_WINDOW
	case 25:       // B_HIROSHI_WINDOW
		return kind;
	case 1 :       // B_TITLED_WINDOW
	default:       
		return 10;
	case 19 :      // B_NO_BORDER_WINDOW
	case 4 :       // B_DESKTOP_WINDOW
		return 4;
	}
}


int32 TWindow::proc_id_2_kind(int32 id)
{
	switch (id) {
	case 2 :       // B_MENU_WINDOW
	case 3 :       // B_MODAL_WINDOW
	case 7 :	   // FLOATING WINDOW
	case 11 :      // B_DOCUMENT_WINDOW
	case 20:       // B_BORDERED_WINDOW
	case 25:       // B_HIROSHI_WINDOW
		return id;
	case 4 :       // B_DESKTOP_WINDOW -> B_NO_BORDER_WINDOW
		return 19;
	case 10 :       // B_TITLED_WINDOW
	default:       
		return 1;
	}
}

// ------------------------------------------------------------

void TWindow::moveto_w(long h, long v)
{
	LockRegionsForWrite();	
	h = aligned_h(h);
	v = aligned_v(v);
	const long old_h = NewClientBound().left;
	const long old_v = NewClientBound().top;
	const long dh = h - old_h;
	const long dv = v - old_v;
	if ((dh == 0) && (dv == 0)) {
		UnlockRegionsForWrite();
		return;
	}
	move_window(dh, dv);
	ws_changed();	
	UnlockRegionsForWrite();
}

// ------------------------------------------------------------

void TWindow::move(long delta_h, long delta_v, char /*force*/)
{
	LockRegionsForWrite();

	const int32 old_h = ClientBound()->left;
	const int32 old_v = ClientBound()->top;
	
	delta_h = aligned_h(old_h + delta_h) - old_h;
	delta_v = aligned_v(old_v + delta_v) - old_v;

	if ((delta_h == 0) && (delta_v == 0)) {
		UnlockRegionsForWrite();
		return;
	}
		
	move_window(delta_h, delta_v);
	ws_changed();
	UnlockRegionsForWrite();
}

// ------------------------------------------------------------

void TWindow::fullscreen_scaling()
{
	rect full_bound;
	full_bound.left = 0;
	full_bound.top = 0;
	full_bound.right = ws_res[cur_switch].virtual_width - 1;
	full_bound.bottom = ws_res[cur_switch].virtual_height - 1;
	if ((m_newClientBound.left == 0) && (m_newClientBound.right == -1) &&
		(full_bound.top == window_bound.top) &&
		(full_bound.left == window_bound.left) &&
		(full_bound.right == window_bound.right) &&
		(full_bound.bottom == window_bound.bottom))
		return;
	if ((full_bound.top != m_newClientBound.top) ||
		(full_bound.left != m_newClientBound.left) ||
		(full_bound.right != m_newClientBound.right) ||
		(full_bound.bottom != m_newClientBound.bottom))
		resize_window(full_bound);
}

// ------------------------------------------------------------

int32 TWindow::set_fullscreen(uint32 *enable)
{
	int32 result;
	TWindow	*a_window;

	LockRegionsForWrite();
	
	if (*enable) {
		// if already fullscreen, return an error
		if (is_screen()) {
			*enable = true;
			result = B_ERROR;
			goto exit;
		}
		// check that this window would not interfere with another
		// already existing fullscreen window in any workspace.
		a_window = windowlist;	
		while (a_window) {
			if (a_window->is_screen())
				if ((a_window->get_ws() & get_ws()) != 0) {
					xprintf("Failed checking on: %s\n", a_window->get_name());
					*enable = false;
					result = B_ERROR;
					goto exit;
				}
			a_window = a_window->nextwindow;
		}	
		// save the previous behavior and size/position
		not_fullscreen_behavior = behavior;
		not_fullscreen_bound = window_bound;
		// switch to window_screen
		behavior = WINDOW_SCREEN;
		open_atomic_transaction();
		bring_to_front();
		close_atomic_transaction();
		result = B_NO_ERROR;
	}
	else {
		if (!is_screen()) {
			*enable = false;
			result = B_ERROR;
			goto exit;
		}
		// set the previous behavior and size/position back
		behavior = not_fullscreen_behavior;
		open_atomic_transaction();
		resize_window(not_fullscreen_bound);
		close_atomic_transaction();
		result = B_NO_ERROR;
	}
exit:	
	UnlockRegionsForWrite();
	return result;
}

// ------------------------------------------------------------

void TWindow::size(long dh, long dv)
{
	if ((dh == 0) && (dv == 0))
		return;
	LockRegionsForWrite();
	const long old_h = ClientBound()->right - ClientBound()->left + dh;
	const long old_v = ClientBound()->bottom - ClientBound()->top + dv;
	size_to(old_h, old_v, 1);
	UnlockRegionsForWrite();
}

// ------------------------------------------------------------

void TWindow::size_to(long new_h, long new_v, char /*forced*/)
{
	LockRegionsForWrite();

	long old_h = NewClientBound().right - NewClientBound().left + 1;
	long old_v = NewClientBound().bottom - NewClientBound().top + 1;

	new_h = aligned_width(new_h) + 1;
	new_v = aligned_height(new_v) + 1;
	if (new_h > m_maxWidth+1)		new_h = m_maxWidth+1;
	else if (new_h < m_minWidth+1)	new_h = m_minWidth+1;
	if (new_v > m_maxHeight+1)		new_v = m_maxHeight+1;
	else if (new_v < m_minHeight+1)	new_v = m_minHeight+1;

	if ((old_h == new_h) && (old_v == new_v)) {
		UnlockRegionsForWrite();
		return;
	}

	open_atomic_transaction();
	rect new_bound;
	new_bound.left = NewClientBound().left;
	new_bound.top = NewClientBound().top;
	new_bound.right = new_bound.left+new_h-1;
	new_bound.bottom = new_bound.top+new_v-1;
	resize_window(new_bound);
	close_atomic_transaction();
	
	ws_changed();	
	UnlockRegionsForWrite();
}

// ------------------------------------------------------------

void TWindow::draw()
{
}

// ------------------------------------------------------------

void TWindow::update()
{
}

// ------------------------------------------------------------

void TWindow::update_scroll_bars0(TView* a_view)
{
	TView *a_child = a_view->first_child;	
	while (a_child) {
		update_scroll_bars0(a_child);
		a_child = a_child->next_sibling;
	}
	if (a_view->NeedsHilite())
		a_view->Draw(false);
}

void TWindow::update_scroll_bars(TView *a_view)
{
	LockRegionsForWrite();

	if (a_view == NULL)
		a_view = top_view;

	if (top_view == NULL) {
		UnlockRegionsForWrite();
		return;
	}

	update_scroll_bars0(a_view);
	UnlockRegionsForWrite();
}

// ------------------------------------------------------------

bool TWindow::can_zoom()
{
	return ((flags & NO_ZOOM) ? false : true);
}

//-----------------------------------------------------------

void recalc_all_regions()
{
	open_atomic_transaction();

	TWindow *w = windowlist;
	while (w && w->is_visible()) {
		w->m_transactFlags |= TFLAGS_REGIONS_CHANGED;
		w = w->NextWindowFlat();
	}

	close_atomic_transaction();
}

//-----------------------------------------------------------

int32 TWindow::ViewCloseNotify(TView *v)
{
	if (last_view == v) {
		last_view = NULL;
		last_view_token = -1;
	}
	if (m_mouseView == v) m_mouseView = NULL;
	if (v->EventMask()) RecalcEventMaskAndRemove(v);
	return 0; // XXX What was I going to return here? --geh
}

//-----------------------------------------------------------

void TWindow::RecalcEventMaskAndRemove(TView *toRemove)
{
	uint32 totalEventMask = 0;
	TView **p = &m_eventChain;
	while (*p) {
		if (*p == toRemove)
			*p = (*p)->m_nextInEventChain;
		else {
			totalEventMask |= (*p)->EventMask();
			p = &(*p)->m_nextInEventChain;
		}
	}
	
	if (totalEventMask != m_eventMask) {
		uint32 oldEventMask = m_eventMask;
		m_eventMask = totalEventMask;
		the_server->WindowEventMaskChanged(this,oldEventMask);
	}
}

int32 TWindow::ViewEventMaskChanged(TView *v, uint32 oldEventMask)
{
	if (!(oldEventMask & ~v->EventMask())) {
		if (!oldEventMask) {
			v->m_nextInEventChain = m_eventChain;
			m_eventChain = v;
		}
		if ((m_eventMask|v->EventMask()) != m_eventMask) {
			uint32 oldEventMask = m_eventMask;
			m_eventMask |= v->EventMask();
			the_server->WindowEventMaskChanged(this,oldEventMask);
		}
	} else {
		RecalcEventMaskAndRemove(v->EventMask()?NULL:v);
	}
	return 0; // XXX What was I going to return here? --geh
}

//-----------------------------------------------------------

void TWindow::do_close()
{
	TWindow	*w, *d_window;
	void	*foo;
	short	w_type;
	port_id	pid;
	
	#if AS_DEBUG
		if (task_id == trackThread) {
			if (getpid() == task_id) {
				DebugPrintf(("track(%d): I'm closing myself\n",trackThread,getpid()));
			} else {
				DebugPrintf(("track(%d): thread %d is closing me!\n",trackThread,getpid()));
			};
			CallStack blah;
			blah.Update();
			blah.Print();
			DebugPrintf(("\n"));
		};
	#endif

	if (!LockRegionsForWrite()) {
		d_window = current_window_screen();
		if (d_window == NULL)
			d_window = current_direct_window();
	
		// make sure it's hidden !
		if (show_hide < -2) show_hide = -2;
		while (show_hide <= 0) do_hide();
		
		if (is_workspace() && workspace) {
			delete workspace;
			workspace = NULL;
		}
	
		int32 result = tokens->get_token(server_token, &w_type, &foo);
		if ((result < 0) || !(w_type & TT_WINDOW)) {
			DebugPrintf(("Token has already been removed! %d\n",server_token));
		}
		tokens->remove_token(server_token);
	
		clear_region(m_preViewPortClip);
		clear_region(m_fullRegion);
		clear_region(m_clientRegion);
		clear_region(m_visibleRegion);
		if (m_invalidRegion) clear_region(m_invalidRegion);
	
		w = windowlist;
		while (w) {
			if (w->has_subwindow_list_window(this))
				w->remove_subwindow(this);
			w = w->nextwindow;
		}

		// Remove the window from the windowlist
		
			while (m_child)	{	// If this window has a child, then remove it first
				m_child->remove_self(); // This will also move the child at the top level
			} // m_child will be patched by remove_self()
			
			if (m_parent) // This window is itslef a child of someone
				remove_self();
			remove_window();

		the_server->WindowCloseNotify(this);

		#ifdef REMOTE_DISPLAY
		m_application->RemoveEventPipe(inter_port);
		#endif
		valid_signature = 0;
		pid = fport_id;

		UnlockRegionsForWrite();
		
		if (d_window == this)
			lock_direct_screen(0, false);
			
		ServerUnlock();
		delete_port(pid);
	} else {
		UnlockRegionsForWrite();
//		ServerUnlock();
	}
}

//-----------------------------------------------------------

void TWindow::bring_to_front(bool switch_workspace, bool recursive)
{
	TWindow	*front, *a_window, *b_window;

	// set the good workspace if enabled and necessary
	if (LockRegionsForWrite()) {
		UnlockRegionsForWrite();
		return;
	}
	
	// This window is not a top window. It can be reordered.
	// TODO: Actualy it could be reordered with its siblings
	if (m_parent) {
		do_atomic_transaction();
		UnlockRegionsForWrite();
		return;
	}

	open_atomic_transaction();
	
	if (switch_workspace && !is_ws_visible(cur_switch)) {
	 	if (!(flags & NO_WS_SWITCH)) {
			for (int i=0; i<ws_count; i++)
				if (is_ws_visible(i)) {
					switcher(i);
					goto happy_switch;
				}
			set_ws(1<<cur_switch);
			if (workspace)
				workspace->Refresh();
		} else if (flags & MOVE_TO_WS) {
			if (is_multi_ws(get_ws()))
				set_ws_visible(cur_switch, true);
			else
				set_ws(1<<cur_switch);
			if (workspace)
				workspace->Refresh();
		}
	}
	
happy_switch:
	// unminimize if necessary
	if (is_mini) {
		do_wind_maximize(switch_workspace);
	}

	// if it's a local floater, bring to front a window attached to it.
	if (is_local_float()) {
		// if we can find a subwindow that is not a local floater, then the problem
		// is closed, as there's not risk of infinite loop.
		a_window = windowlist;
		while (a_window) {
			if (has_subwindow(a_window) &&
				a_window->is_ws_visible(cur_switch) &&
				!a_window->is_local_float()) {
				a_window->bring_to_front(switch_workspace);
				goto recursive_bring_completed;
			}
			a_window = a_window->nextwindow;
		}
		
		// if we can't find anything else than a local floater, go into safe recursive
		// analysis.
		a_window = windowlist;
		while (a_window) {
			if (has_subwindow(a_window) &&
				a_window->is_ws_visible(cur_switch)) {
				// anti-loop init if first step of the recursion
				if (recursive == false) {
					b_window = windowlist;
					while (b_window) {
						b_window->w_tmp = 0;
						b_window = b_window->nextwindow;
					}
				}
				if (w_tmp == 0)
					w_tmp = 1;
				else
					goto recursive_bring_completed;
				a_window->bring_to_front(switch_workspace, true);
				break;
			}
			a_window = a_window->nextwindow;
		}
recursive_bring_completed:;
	}

	// if it's the desktop window, bring to front all local windows
	// attached to it.
	if (is_desktop()) {
		a_window = find_top_modal();
		if (a_window != NULL)
			a_window->bring_to_front();
	}

	// send the window as front as needed...
	front = get_front_limit();
	bring_just_behind(front);
	if (front == NULL)
		make_active();

	close_atomic_transaction();

	ws_changed();

	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

// select the front level for the newly promoted window.
TWindow *TWindow::get_front_limit()
{
	// for modal or menus, the front limit doesn't apply.
	if (is_modal() || is_menu())
		return NULL;

	// check that the front-limit is still in the list.
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (a_window == front_limit)
			return front_limit;
		a_window = a_window->nextwindow;
	}
	// in the other case, no front limit.
	return NULL;
}

//-----------------------------------------------------------

int32 TWindow::test_active(TWindow **active)
{
	TWindow	*top_sub;

	*active = this;
	// return true if already focus
	if (is_focus())
		return window_active;

	// 10/27/2000 - mathias: if the window is blocked by a modal, it cannot be active
	if (has_modal())
		return window_never_active;

	// check if the window doesn't accept focus
	if (flags & DISLIKE_FOCUS) {
		if (!is_float())
			return window_never_active;
		top_sub = top_subwindows();
		if (top_sub == 0)
			return window_never_active;
		if (top_sub->flags & DISLIKE_FOCUS)
			return window_never_active;
		*active = top_sub;
		if (top_sub->is_focus())
			return window_trans_active;
		return window_not_trans_active;
	}
	return window_not_active;
}

//-----------------------------------------------------------

void TWindow::make_active(bool status)
{
	TWindow	*w;
	TWindow *active;	

	// turn active
	if (status) {
		// do nothing if already focus or refuse focus
		switch (test_active(&active)) {
			case window_never_active:
				// If blocked by a modal, make this modal active
				if ((w = find_top_modal()) && (w->is_menu() == false) && (w != this))
					w->make_active(true);
				return;
			case window_active:
			case window_trans_active:
			case window_not_active:
				break;
			case window_not_trans_active:
				top_subwindows()->make_active(true);
				return;
		}

		// check type before canceling the activation status	
		LockRegionsForWrite();

		if (is_float()) {
			w = windowlist;
			while (w) {
				if ((w->is_float() || w->is_menu()) && !w->locks_focus())
					w->state &= ~wind_active;
				w = w->nextwindow;
			}		
		} else if (is_menu()) {
			w = windowlist;
			while (w) {
				if (w->is_menu() && !w->locks_focus())
					w->state &= ~wind_active;
				w = w->nextwindow;
			}		
		} else {
			w = windowlist;
			while (w) {
				if (!w->locks_focus())
					w->state &= ~wind_active;
				w = w->nextwindow;
			}		
		}
		UnlockRegionsForWrite();
		// set the window active flag
		active->state |= wind_active;
		return;
	}
	// turn inactive
	the_server->focus_follows_mouse();	
	if (!locks_focus())
		state &= ~wind_active;
}

//-----------------------------------------------------------

void TWindow::bring_just_behind(TWindow *the_window)
{
	LockRegionsForWrite();
	open_atomic_transaction();
	
	// do the requested inversion.
	remove_window();
	insert_window(the_window);
	
	close_atomic_transaction();
	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

void TWindow::send_to_back(bool make_inactive)
{
	(void)make_inactive;

	LockRegionsForWrite();
	open_atomic_transaction();
	
	// hide the window temporarily. Then reorder and update the activation
	// then make the window viisble again. That way it really goes as
	// deep as possible without interference with special activation
	// rules like DeskBar or Workspace.
	show_hide++;
	flush_ordering_transaction();
	show_hide--;
	
	close_atomic_transaction();	
	ws_changed();
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

void TWindow::add_view(TView *a_view, bool last)
{
	a_view->SetOwner(this);

	region *newInval = newregion();
	LockRegionsForWrite();
		top_view->add_view(a_view,newInval,last);
		top_view->TweakHierarchy();
		Invalidate(newInval);
	UnlockRegionsForWrite();
	kill_region(newInval);
}

//-----------------------------------------------------------

void TWindow::valid(region *a_region)
{
	if (!is_visible()) return;
	LockRegionsForWrite();
	valid_region(a_region);
	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

void TWindow::valid(rect *a_rect)
{
	if (!is_visible())
		return;
	LockRegionsForWrite();
		region *a_region = newregion();
		set_region(a_region, a_rect);
		valid_region(a_region);
		kill_region(a_region);
	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

void TWindow::handle_client_message(long message)
{
	(void)message;
}
			
//-----------------------------------------------------------

void TWindow::handle_message(message *m)
{
	long	receive_port;
	long	send_port;
	long	session_id;

	switch(m->what) {
		case CLOSE:
			wait_regions();
			if (still_a_good_window(this)) do_close();
			signal_regions();
			break;

		case INIT_SESSION:
			receive_port = 	fport_id;
			send_port = m->parm2;
			session_id = m->parm1;	
			a_session = new TSession(send_port, receive_port, session_id);
			a_session->SetAtom(this);
			a_session->SetClient(this);
			break;
	}
}

//-----------------------------------------------------------

long start_task(void *arg)
{
	static_cast<TWindow *>(arg)->task();
	return B_OK;
}

//-----------------------------------------------------------

void cleanup_windows(long pid)
{
	the_server->LockWindowsForWrite();

	TWindow	*a_window = windowlist;
	while (a_window) {
		if (a_window->team_id == pid) {
			a_window->do_close();
			a_window = windowlist;
			continue;
		}
		a_window = a_window->nextwindow;
	}
	tokens->cleanup_dead(pid);

	the_server->UnlockWindowsForWrite();
}

//-----------------------------------------------------------

void TWindow::Suicide()
{
	ServerUnlock();
	ServerUnlock();
	suicide(0);
}

void TWindow::task()
{
	message	m;
	int32	result,client_message;
	
	birth();
	sam_thread_in(12);

	if (a_session) goto haveSession;
	while(1) {
		result = receive_message(&m);
		if (result == -2) Suicide();

		if (result >= 0) {
			handle_message(&m);
		} else {
			if (a_session != 0) {
				haveSession:
				do {
					result = a_session->sread2(4, &client_message);
					if (result == -2) Suicide();
					if (result == 0) handle_client_message(client_message);
				} while (!a_session->has_other() && (result == 0));
			}
		}
		sam_thread_status();
	}

	suicide(0);
}

//-----------------------------------------------------------

long TWindow::receive_message(message *a_message)
{
	long result;
	long msgCode;

	if (a_session != 0) {
		result = a_session->get_other(a_message);
		return(result);
	} else {
		SessionWillBlock();
		if (ServerUnlock() == 2) {
			ServerUnlock();
			suicide(0);
		}
		result = read_port(fport_id, &msgCode, (char *)a_message, 64);
		if (ServerLock() == 1) {
			ServerUnlock();
			ServerUnlock();
			suicide(0);
		}
		if (result < 0) { DebugPrintf(("preceive error 100\n")); };
		return(0);
	}
}

//-----------------------------------------------------------

long TWindow::inter_port_count()
{
	return port_count(inter_port);
}

long TWindow::main_port_count()
{
	return port_count(fport_id);
}

void TWindow::send_message(message *a_message)
{
	write_port_etc(fport_id, 0, a_message, 64, B_TIMEOUT, PORT_TIMEOUT);
}

void TWindow::global_to_local(long *h, long *v)
{
	*h -= ClientBound()->left;
	*v -= ClientBound()->top;
}

void TWindow::global_to_local(rect *r)
{
	global_to_local(&(r->left), &(r->top));
	global_to_local(&(r->right), &(r->bottom));
}

void TWindow::global_to_local(point *pt)
{
	global_to_local(&(pt->h), &(pt->v));
}

//-----------------------------------------------------------

void TWindow::get_bound(rect *r)
{
	*r = *ClientBound();
}

//-----------------------------------------------------------

void TWindow::ws_changed()
{
	LockRegionsForWrite();
	if (workspace)
		workspace->WindowChanged(this);
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

void TWindow::do_hide()
{
	if (front_limit == this)
		set_front_limit(NULL);

	LockRegionsForWrite();
	show_hide++;
	if (is_mini) {
		if (show_hide == INIT_HIDE) {
			was_mini = true;
			is_mini = false;
		}
	} else if (show_hide == 1) {
		ws_changed();
		do_atomic_transaction();
	}
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

void TWindow::do_show()
{
	LockRegionsForWrite();
	show_hide--;
	if (was_mini) {
		if (show_hide == 1) {
			is_mini = true;
			was_mini = false;
		}
	} else  if ((show_hide == 0) && (!is_mini)) {
		bring_to_front();
		ws_changed();
	}
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

void TWindow::do_wind_maximize(bool switch_workspace)
{
	LockRegionsForWrite();
	
	if (was_mini) {	
		show_hide--;
		was_mini = false;
	}

	if (is_mini) {
		show_hide--;
		is_mini = false;
	}
	
	ws_changed();

	if (switch_workspace)
		bring_to_front();
		
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

void TWindow::do_wind_minimize()
{
	if (is_modal() || is_float())
		return;
		
	LockRegionsForWrite();
	if (!is_mini && !was_mini) {
		if (show_hide <= 0) {
			is_mini = true;
			show_hide++;
			ws_changed();
			do_atomic_transaction();
		}
		else if (!was_mini) {
			show_hide++;
			was_mini = true;
		}
	}
	UnlockRegionsForWrite();
}

//------------------------------------------------------------

long TWindow::send_event(BParcel *an_event)
{
	bool preferred;
	if (an_event->Target(&preferred) == NO_TOKEN)
		an_event->SetTarget(client_token, false);
	return an_event->WritePort(inter_port, 1, B_TIMEOUT, PORT_TIMEOUT);
}

//------------------------------------------------------------

void TWindow::set_name(char *name)
{
	LockRegionsForWrite();

	char *name_buffer = (char *)grMalloc(strlen(name)+34, "window_name", MALLOC_LOW);
	if (name_buffer) {
		sprintf(name_buffer, "w:%s", name);
		name_buffer[32] = 0;
		rename_thread(task_id, name_buffer);

		grFree(window_name);
		strcpy(name_buffer, name);
		window_name = name_buffer;
		open_atomic_transaction();
		OpenDecorTransaction();
		m_newDecorState->SetString("@title",name);
		close_atomic_transaction();
	
		ws_changed();
	}

	UnlockRegionsForWrite();
}

//------------------------------------------------------------ 

void TWindow::get_decor_state(BMessage* into)
{
	if (m_decor) {
		m_decor->ArchiveVariables(into, PUBLIC_VISIBILITY);
		#if AS_DEBUG
		printf("Retrieved decor state: "); into->PrintToStream();
		#endif
	}
}

//------------------------------------------------------------

void TWindow::set_decor_state(const BMessage* from)
{
	LockRegionsForWrite();
	open_atomic_transaction();
	OpenDecorTransaction();
	m_newDecorState->UnarchiveVariables(from, PUBLIC_VISIBILITY);
	close_atomic_transaction();	
	ws_changed();
	UnlockRegionsForWrite();
	
	#if AS_DEBUG
	printf("Updated decor state: "); from->PrintToStream();
	#endif
}

//---------------------------------------------------------------
// insert_window will insert a window in the window list	 	 
// the insertion point in after the window "after"				 
// if after=0, the the window is added in front of all other 	 
// windows.														 
//---------------------------------------------------------------

void TWindow::insert_window(TWindow* after)
{
	// 	visibility_windowlist not valid anymore
	visibility_windowlist = NULL;
	back_visibility_windowlist = NULL;

	TWindow *saved;

	// special case : empty window list
	if (windowlist == NULL) {
		windowlist = this;
		nextwindow = NULL;
		m_parent = NULL;
	}
	// -1 : will insert it at the last position
	else if (after == (TWindow *)-1) {
		saved = windowlist;
		while(saved->nextwindow)
			saved = saved->nextwindow;
		saved->nextwindow = this;
		nextwindow = NULL;
		m_parent = NULL;
	}
	// a window pointer : insert just after it.
	else if (after) {
		saved = after->nextwindow;
		after->nextwindow = this;
		nextwindow = saved;
		m_parent = NULL;
	}
	// NULL : insert at the head of the window list
	else {
		saved = windowlist;
		nextwindow = saved;
		windowlist = this;
		m_parent = NULL;
	}
}

void TWindow::remove_window()
{
	#if AS_DEBUG
	if (m_parent) { // This is not a top window
		debugger("remove_window() called on a child window!");
		return;
	}
	#endif

	// 	visibility_windowlist not valid anymore
	visibility_windowlist = NULL;
	back_visibility_windowlist = NULL;

	TWindow **current = &windowlist;
	while (*current) {
		if (*current == this) {
			*current = nextwindow;
			m_parent = NULL;
			break;
		}
		current = &((*current)->nextwindow);
	}
}

//---------------------------------------------------------------

status_t TWindow::insert_child(TWindow *w)
{
	#if AS_DEBUG
	if (w->m_parent) {
		debugger("insert_child() this window already have a parent!");
		return B_BAD_VALUE;
	}
	#endif
	
	LockRegionsForWrite();

	// Hide the window and remove it from the windowlist
	w->do_hide();
	w->remove_window();

	// Then it is safe to insert it in its parent list
	TWindow *saved = m_child;
	m_child = w;
	w->nextwindow = saved;
	w->m_parent = this;

	// Finaly move the window at its right position and show it again
	open_atomic_transaction();
		const long h = window_bound.left + w->window_bound.left;
		const long v = window_bound.top + w->window_bound.top;
		w->moveto_w(h, v);
		w->do_show();
	close_atomic_transaction();

	UnlockRegionsForWrite();
	return B_OK;
}

status_t TWindow::remove_self()
{
	// Detach this window from its parent
	// and move it to the root level
	LockRegionsForWrite();

	TWindow *w = windowlist;
	while (w) {
		TWindow *c = w->m_child;
		while (c) {		
			if (c == this) {
				do_hide();
				TWindow **prev = &(m_parent->m_child);	// head of this level
				while (*prev != this)	// we know we will find the window here
					prev = &((*prev)->nextwindow);
				*prev = nextwindow;
				m_parent = NULL;
				nextwindow = NULL;
				// reinsert the child just before its ancestor
				insert_window(w);
				w->bring_just_behind(this);
				do_show();
				UnlockRegionsForWrite();
				return B_OK;
			}
			c = c->NextChild(w);
		}
		w = w->nextwindow;
	}

	UnlockRegionsForWrite();
	return B_BAD_VALUE;
}

//-----------------------------------------------------------

region *TWindow::set_visible_region(region *vis_region, rect *new_rect, region *clientUpdateRegion)
{
	if (flags & NO_DRAWING) {
		clear_region(vis_region);
	}
	
	RenderContext *ctxt = ThreadRenderContext();
	region *update_region = newregion();

	if (new_rect == NULL) {
		sub_region(vis_region, VisibleRegion(), update_region);
		SetVisibleRegion(vis_region);
	} else {
		const long dh = new_rect->left - window_bound.left;
		const long dv = new_rect->top - window_bound.top;
		const long dwidth = (new_rect->right - new_rect->left) - (window_bound.right - window_bound.left);
		const long dheight= (new_rect->bottom - new_rect->top) - (window_bound.bottom - window_bound.top);
		const bool moved = (dh || dv);
		const bool resized = (dwidth || dheight);

		if (moved) {
			renderPort->origin.h += dh;
			renderPort->origin.v += dv;
			*update_region = *vis_region;
			if (true) { // TODO: don't do the blitting optim' if h/w doesn't support it.
				// this the special case for translation
				IRegion tmpReg = *VisibleRegion();
				tmpReg.OffsetBy(dh, dv);

				// blit as much as possible to reduce the update region.
				IRegion blit_region = tmpReg & *vis_region;
	
				// The goal here is to remove from the clipping region the VisibleRegion() of
				// all other windows that will also move. But we do that in the reverse order (from back to top),
				// so that artifacts are less visible.
				TWindow *w = next_back_ordered_window;
				while (w) {
					const rect& b = w->NewClientBound();
					const long wdh = b.left - w->window_bound.left;
					const long wdv = b.top - w->window_bound.top;
					const long wdwidth = (b.right - b.left) - (w->window_bound.right - w->window_bound.left);
					const long wdheight= (b.bottom - b.top) - (w->window_bound.bottom - w->window_bound.top);
					const bool wmoved = (wdh || wdv);
					const bool wresized = (wdwidth || wdheight);
					if ((wdh==dh) && (wdv==dv) && (!resized)) { // The window was moved (not resized) by the same offset
						blit_region |= *(w->m_newVisibleRegion);
						w->m_already_blitted = true;
					} else if (wmoved || wresized) { // The window was moved+resized
						blit_region -= *w->VisibleRegion();
					}
					w = w->next_back_ordered_window;
				}
	
				if (m_already_blitted == false) { // The window was moved already. Don't do it again.
					rect oldOutBound = VisibleRegion()->Bounds();
					the_cursor->AssertVisibleIfShown();
					grClipToRegion(ctxt, &blit_region);
					point p;
					p.h = oldOutBound.left + dh;
					p.v = oldOutBound.top + dv;
					grCopyPixels(ctxt,oldOutBound,p);
					grClearClip(ctxt);
					grSync(ctxt);
				}

				// The real region to update is the new visible region minus what we've just blitted
				*update_region -= blit_region;
			}

			window_bound.left += dh;
			window_bound.right += dh;
			window_bound.top += dv;
			window_bound.bottom += dv;
			top_view->UpdateOverlays();
		}

		if (resized) {
			window_bound = *new_rect;

			// view resizing preparation
			BArray<TView*> localViews(10);
			region *viewInvals = newregion();
			rect topViewRect = window_bound;
			offset_rect(&topViewRect,-topViewRect.left,-topViewRect.top);
			top_view->SetBounds(topViewRect);
			if (!m_viewTransactionCount) {
				top_view->ViewTransaction(viewInvals,&localViews);
				offset_region(viewInvals,ClientBound()->left,ClientBound()->top);
			}
			top_view->TweakHierarchy();

			// set the visible region
			SetVisibleRegion(vis_region);
			region *tmpR2 = newregion();
			if (clientUpdateRegion)
				or_region(update_region,clientUpdateRegion,tmpR2);
			else
				copy_region(update_region,tmpR2);
			or_region(tmpR2,viewInvals,update_region);
			kill_region(tmpR2);
			
			// view resizing, launch updates and draws.
			region *tmpSavedBad = NULL;
			if (InvalidRegion()) {
				tmpSavedBad = newregion();
				copy_region(InvalidRegion(),tmpSavedBad);
			}
			SetInvalidRegion(NULL);

			const uint32 nb = localViews.CountItems();
			for (uint32 v=0 ; v<nb ; v++)
				localViews[v]->Draw();

			if (tmpSavedBad) {
				SetInvalidRegion(tmpSavedBad);
				kill_region(tmpSavedBad);
			}
			m_statusFlags |= WFLAG_INVALIDATED;
			kill_region(viewInvals);
		} else {
			SetVisibleRegion(vis_region);
			window_bound = *new_rect;
		}
	}
	
	if (empty_region(vis_region)) { // XXX
		clear_region(renderPort->portClip);
		renderPort->portBounds = renderPort->portClip->Bounds();
		clear_region(renderPort->drawingClip);
		renderPort->drawingBounds = renderPort->drawingClip->Bounds();
#if ROTATE_DISPLAY
		if (renderPort->canvasIsRotated)
			clear_region(renderPort->rotatedDrawingClip);
#endif
		if (BadRegion())		clear_region(BadRegion());
		if (ClearRegion())		clear_region(ClearRegion());
		if (InvalidRegion())	clear_region(InvalidRegion());
	} else { // XXX
		and_region(ClientRegion(), vis_region, renderPort->portClip);
		renderPort->portBounds = renderPort->portClip->Bounds();
		copy_region(renderPort->portClip, renderPort->drawingClip);
		renderPort->drawingBounds = renderPort->drawingClip->Bounds();
#if ROTATE_DISPLAY
		if (renderPort->canvasIsRotated)
			renderPort->rotater->RotateRegion(renderPort->drawingClip, renderPort->rotatedDrawingClip);
#endif
		if (BadRegion()) {
			and_region(BadRegion(), VisibleRegion(), vis_region);
			copy_region(vis_region, BadRegion());
		}
		if (ClearRegion()) {
			and_region(ClearRegion(), VisibleRegion(), vis_region);
			SetClearRegion(vis_region);
		}
		if (InvalidRegion()) {
			and_region(InvalidRegion(), VisibleRegion(), vis_region);
			SetInvalidRegion(vis_region);
		}
	}	

	if (force_full_refresh)
		copy_region(VisibleRegion(), update_region);

	return update_region;
}

//-----------------------------------------------------------

team_id active_team()
{ // need to be called with the region sem locked (or meaningless)
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (a_window->is_active())
			return a_window->team_id;
		a_window = a_window->nextwindow;
	}
	return -1;
}

//-----------------------------------------------------------

void TWindow::ResetDecor(TView* v)
{
	while (v) {
		if (v->ResetDecor())
			InvalInView(v);
		if (v->first_child) ResetDecor(v->first_child);
		v = v->next_sibling;
	}
}

void TWindow::ResetDecor()
{
	set_flags(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,true);
	if (top_view) ResetDecor(top_view);
}

//-----------------------------------------------------------

void TWindow::OpenDecorTransaction()
{
	if (!m_newDecorState) {
		m_newDecorState = new DecorState(m_decor);
		m_newDecorState->ServerLock();
		m_newDecorState->OpenTransaction();
		m_transactFlags |= TFLAGS_TRANSACTION_OPEN | TFLAGS_NEED_LAYOUT | TFLAGS_NEED_DRAW | TFLAGS_NEED_COMMIT;
	}
}

//-----------------------------------------------------------

void TWindow::InsertDecorTransaction(DecorState *state, uint32 tflags)
{
	if (m_newDecorState) debugger("InsertDecorTransaction: m_newDecorState is not NULL!");
	m_newDecorState = state;
	m_newDecorState->ServerLock();
	m_clientUpdateRegion = NULL;
	m_transactFlags |= tflags;
}

void TWindow::SetClientUpdate(region *clientUpdate)
{
	if (m_clientUpdateRegion) debugger("SetClientUpdate: m_clientUpdateRegion is not NULL!");
	m_clientUpdateRegion = clientUpdate;
	m_transactFlags &= ~TFLAGS_NEED_LAYOUT;
}

//-----------------------------------------------------------

void TWindow::SetActiveLook(bool active)
{
	OpenDecorTransaction();
	m_newDecorState->SetFocus(active?1:0);
}

rect TWindow::NewClientBound()
{
	return (m_newClientBound.right >= m_newClientBound.left) ? (m_newClientBound) : (window_bound);
}

void TWindow::SetNewClientBound(rect new_rect)
{
	if ((ClientBound()->left == new_rect.left) &&
		(ClientBound()->right == new_rect.right) &&
		(ClientBound()->top == new_rect.top) &&
		(ClientBound()->bottom == new_rect.bottom)) {
		m_newClientBound.left = 0;
		m_newClientBound.right = -1;
		return;
	};
	m_newClientBound = new_rect;
	OpenDecorTransaction();
	m_newDecorState->SetBounds(new_rect);
}

// ------------------------------------------------------------
// This routine will change the window bound                  
// All updated are handled for regular windows.		    	  
// ------------------------------------------------------------

void TWindow::resize_window(rect b)
{
	LockRegionsForWrite();
		open_atomic_transaction();
			SetNewClientBound(b);
		close_atomic_transaction();
	UnlockRegionsForWrite();
}

void TWindow::recalc_full()
{
	LockRegionsForWrite();
		open_atomic_transaction();
			m_transactFlags |= TFLAGS_REGIONS_CHANGED;
		close_atomic_transaction();
	UnlockRegionsForWrite();
}

// --------------------------------------------------------------------
// This routine will move the top left corner of a window by dh,d v    
// pixels.														      
// --------------------------------------------------------------------

void TWindow::move_window(long dh, long dv)
{
	LockRegionsForWrite();
		open_atomic_transaction();
			TWindow *w = this;
			while (w) {
				rect r = w->NewClientBound();
				r.left += dh;
				r.right += dh;
				r.top += dv;
				r.bottom += dv;
				w->SetNewClientBound(r);
				w = w->NextChild(this);
			}
		close_atomic_transaction();
	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

void TWindow::valid_region(region *the_region)
{
	if (empty_region(the_region)) return;

	LockRegionsForWrite();
	region	*tmp_region = wmgr_tmp;
	
	offset_region(the_region,
		ClientBound()->left,
		ClientBound()->top);

	copy_region(BadRegion(), tmp_region);
	sub_region(tmp_region, the_region, BadRegion());
	
	offset_region(the_region,
		-ClientBound()->left,
		-ClientBound()->top);

	UnlockRegionsForWrite();
}

//-----------------------------------------------------------

bool TWindow::update_pending()
{
	return (!empty_region(BadRegion()) || InvalidRegion());
}

//-----------------------------------------------------------

void TWindow::ClearToBadTransaction(region *clearRegion, uint16 updateCauses)
{
	dup_tag_windowR(this, DUP_SUBMIT_UPDATE);
	if (last_pending_update == 0LL) last_pending_update = system_time();
	region *tmp_region = wmgr_tmp;
	or_region(clearRegion,BadRegion(),tmp_region);
	copy_region(tmp_region,BadRegion());
	m_updateCauses[1] |= updateCauses;
};

void TWindow::BadToInvalidTransaction()
{
	region *tmpRegion1 = newregion();
	region *tmpRegion2 = newregion();

	and_region(ClientRegion(),BadRegion(),tmpRegion2);
	and_region(tmpRegion2,VisibleRegion(),tmpRegion1);
	SetInvalidRegion(tmpRegion1);
	clear_region(BadRegion());

	kill_region(tmpRegion1);
	kill_region(tmpRegion2);

	m_updateCauses[2] |= m_updateCauses[1];
	m_updateCauses[1] = 0;
	m_needsClear = (m_statusFlags & (WFLAG_INVALIDATED|WFLAG_DRAWN_IN)) != 0;
	m_statusFlags &= ~WFLAG_INVALIDATED;
};

void TWindow::Invalidate(region *exposed, uint16 updateCauses)
{
	if (!is_visible() || empty_region(exposed)) return;

	offset_region(exposed,
		ClientBound()->left,
		ClientBound()->top);

	and_region(exposed,ClientRegion(),wmgr_tmp);
	ExposeViews(wmgr_tmp,updateCauses);

	offset_region(exposed,
		-ClientBound()->left,
		-ClientBound()->top);

	m_statusFlags |= WFLAG_INVALIDATED;
};

void TWindow::Invalidate(rect *a_rect, uint16 updateCauses)
{
	region	*a_region;
	a_region = newregion();
	set_region(a_region, a_rect);
	Invalidate(a_region,updateCauses);
	kill_region(a_region);
}

uint32 TWindow::ExposeViews(region *exposed, uint16 causes)
{
	if (empty_region(exposed)) return 0;

	int32 nextStage = ClearViews(exposed,causes,clearStageImmediate);

	region *tmpRegion1 = newregion();

	if (m_updateCauses[0] || (nextStage == clearStageLater)) {
		/* There is more clearing to do! */
		uint16 oldCauses = m_updateCauses[0];
		m_updateCauses[0] |= causes;
		or_region(ClearRegion(),exposed,tmpRegion1);
		and_region(tmpRegion1,ClientRegion(),ClearRegion());
		if (!oldCauses) the_server->WindowNeedsUpdate();
	} else {
		/*	Ideally, we want to pass this on directly to the updater. */
		m_updateCauses[1] |= causes;
		dup_tag_windowR(this, DUP_SUBMIT_UPDATE);
		if (last_pending_update == 0LL) last_pending_update = system_time();
		or_region(BadRegion(),exposed,tmpRegion1);
		and_region(tmpRegion1,ClientRegion(),BadRegion());
	}

	kill_region(tmpRegion1);
	return nextStage;
}

uint32 TWindow::ClearViews(region *damage, uint16 causes, int32 stage)
{
	if (empty_region(damage)) return 0;

	int32 result;
	fpoint newOrigin;
	region *tmpRegion1 = newregion();
	RenderContext *ctxt = ThreadRenderContext();
	newOrigin.h = ClientBound()->left;
	newOrigin.v = ClientBound()->top;

	ClipThreadPort(VisibleRegion());
	grLock(ctxt);

		/* Clear the views */
		grSetOrigin(ctxt,newOrigin);
		and_region(damage, ClientRegion(), tmpRegion1);
		offset_region(tmpRegion1,-ClientBound()->left,-ClientBound()->top);
		grClipToRegion(ctxt,tmpRegion1);
		result = top_view->ClearGather(&tmpRegion1, ctxt, NULL, NULL, causes, stage);

	grUnlock(ctxt);
	newOrigin.h = newOrigin.v = 0;
	grClearClip(ctxt);
	grSetOrigin(ctxt,newOrigin);
	ClipThreadPort(NULL);

	kill_region(tmpRegion1);
	return result;
}

//-----------------------------------------------------------

bool TWindow::need_update()
{
	if (!empty_region(BadRegion()) &&
	    !(m_statusFlags & (WFLAG_SENT_UPDATE|WFLAG_IN_UPDATE)) &&
	    !m_updateDisabledCount) {
		return true;
	}
	return false;
}

//-----------------------------------------------------------

uchar TWindow::begin_update()
{
	LockRegionsForWrite();

		if (empty_region(BadRegion())) {
			UnlockRegionsForWrite();
			atomic_and((int32*)&m_statusFlags,~WFLAG_SENT_UPDATE);
			return 0;
		}
		BadToInvalidTransaction();
		m_lastUpdateTimeNowUs = system_time();
	UnlockRegionsForWrite();


	atomic_and((int32*)&m_statusFlags,~WFLAG_SENT_UPDATE);
	return 1;
}

//-----------------------------------------------------------

void TWindow::end_update()
{
	LockRegionsForWrite();
		SetInvalidRegion(NULL);
		m_statusFlags &= ~WFLAG_DRAWN_IN;
		m_updateCauses[2] = 0;
		m_lastUpdateTime = system_time() - m_lastUpdateTimeNowUs;
	UnlockRegionsForWrite();
}

void TWindow::check_hilite_and_activation()
{
	bool		old_look, new_look, old_active, new_active;
	BParcel		an_event(B_WINDOW_ACTIVATED);

	// old (as currently known by the client) and
	// new (as currently effective) activation state.
	old_active = (state & known_active) != 0;
	new_active = (state & wind_active) != 0;
	
	// old (as currently drawn) and
	// new (as defined from current state) activation look.
	old_look = (state & look_active) != 0;
	new_look = new_active || (is_float() && is_visible() && (flags & DISLIKE_FOCUS));
		
	// set the new states as current ones.	
	state &= ~(look_active | known_active);
	if (new_look)
		state |= look_active;
	if (new_active)
		state |= known_active;
		
	// update graphic appearance
	if (new_look != old_look)
	{
		SetActiveLook(new_look);
		
		// 10/18/2000 - mathias: the hilite of the window just changed
		// we need to redraw the scrollbars.
		update_scroll_bars();
	}

	// update client status
	if (new_active != old_active) {
		an_event.AddInt64("when",system_time());
		an_event.AddBool("active",new_active);
		EventPort()->ReplaceEvent(&an_event,&m_queueIter_activate,&m_activateMsgToken);
	}
}

//-----------------------------------------------------------

// Should be used only for floating and modal windows.
// Make the current workspace active or not. Keep the
// window in single workspace mode (always).
void TWindow::set_current_ws(bool enable)
{
	if (enable)		set_ws(1<<cur_switch);
	else			set_ws(0);
}

void TWindow::set_ws_visible(int32 index, bool status)
{
	const uint32 old = ws_active;
	if (status)		ws_active |= (1<<index);
	else			ws_active &= (0xffffffff-(1<<index));
	if (old == ws_active)
		return;

	// send an event to keep the client up to date
	BParcel an_event(B_WORKSPACES_CHANGED);
	an_event.AddInt64("when",system_time());
	an_event.AddInt32("old",old);
	an_event.AddInt32("new",ws_active);
	EventPort()->ReplaceEvent(&an_event,&m_queueIter_ws,&m_wsMsgToken);
}

void TWindow::move_between_workspace(int32 from, int32 to)
{
	if (from == to)
		return;
	const uint32 old = ws_active;
	ws_active = (ws_active &= (0xffffffff-(1<<from))) | (1<<to);
	// send an event to keep the client up to date
	BParcel	an_event(B_WORKSPACES_CHANGED);
	an_event.AddInt64("when",system_time());
	an_event.AddInt32("old", old);
	an_event.AddInt32("new", ws_active);
	EventPort()->ReplaceEvent(&an_event,&m_queueIter_ws,&m_wsMsgToken);
}

void TWindow::set_ws(uint32 ws)
{
	if (ws == ws_active)
		return;
	const uint32 old = ws_active;
	ws_active = ws;
	// send an event to keep the client up to date
	BParcel an_event(B_WORKSPACES_CHANGED);
	an_event.AddInt64("when",system_time());
	an_event.AddInt32("old",old);
	an_event.AddInt32("new",ws_active);
	EventPort()->ReplaceEvent(&an_event,&m_queueIter_ws,&m_wsMsgToken);
}

// ------------------------------------------------------------

void TWindow::set_workspace(uint32 ws)
{
	int32		i;
	uint32		old;
	BParcel		an_event(B_WORKSPACES_CHANGED);

	// refuse to change the workspace setting of a screen window.
	if (is_screen())
		return;

	// Set current workspace option
	if (!ws)
		ws = (1 << cur_switch);

	// check there is a real change
	old = get_ws();
	if (ws == old)
		return;
	
	// initialise multi-workspace mode if necessary
	if (is_multi_ws(ws) && (ws_ext_bounds == 0)) {
		ws_ext_bounds = (rect*)grMalloc(sizeof(rect)*MAX_WORKSPACE,"ws_ext_bounds",MALLOC_CANNOT_FAIL);
		for (i=0; i<MAX_WORKSPACE; i++)
			ws_ext_bounds[i] = window_bound;
	}
	
	// set the new state
	set_ws(ws);
	
	// update order
	LockRegionsForWrite();
	do_atomic_transaction();
	if (workspace) workspace->Refresh();
	UnlockRegionsForWrite();

	// send an event to keep the client up to date
	an_event.AddInt64("when",system_time());
	an_event.AddInt32("old",old);
	an_event.AddInt32("new",ws);
	EventPort()->ReplaceEvent(&an_event,&m_queueIter_ws,&m_wsMsgToken);
}

// ------------------------------------------------------------

void store_states(long i)
{
	TWindow	*a_window = windowlist;
	while (a_window) {
		if (a_window->ws_ext_bounds != 0)
			a_window->get_bound(a_window->ws_ext_bounds+i);
		a_window = a_window->NextWindowFlat();
	}
}

// ------------------------------------------------------------

void restore_state(long i)
{
	rect bnd;
	TWindow	*a_window = windowlist;
	while(a_window) {
		if (a_window->ws_ext_bounds != 0) {
			a_window->get_bound(&bnd);
			if ((a_window->ws_ext_bounds[i].right == -1) &&
				(a_window->ws_ext_bounds[i].left == 0) &&
				(a_window->ws_ext_bounds[i].top == 0) &&
				(a_window->ws_ext_bounds[i].bottom == -1)) {
				a_window->get_bound(a_window->ws_ext_bounds+i);
				a_window->ws_changed();
			} else if ((bnd.left != a_window->ws_ext_bounds[i].left) ||
					 (bnd.top != a_window->ws_ext_bounds[i].top)) {
				a_window->moveto_w(	a_window->ws_ext_bounds[i].left,
									a_window->ws_ext_bounds[i].top);
				a_window->force_full_refresh = true;
			}
		}
		a_window = a_window->NextWindowFlat();
	}
}

// ------------------------------------------------------------

void switcher(long i)
{
	bool			activate;
	TWindow			*a_window;
	BParcel			deact_event(B_WORKSPACE_ACTIVATED);
	BParcel			act_event(B_WORKSPACE_ACTIVATED);
	
	// check if the workspace switch request is valid
	if ((i != cur_switch) && (i >= 0) && (i < ws_count)) {
		// switching is an exclusive full screen operation
		acquire_direct_screen_and_region_sem();

		open_atomic_transaction();

		// move windows around, hide them, show them...
		last_ws = cur_switch;
		store_states(last_ws);
		cur_switch = i;
		restore_state(cur_switch);
		
		// reset activation to get a new front window from the window order...
		reset_activation();

		// do an ordering/activation simulation (but not a visibility execution
		flush_ordering_transaction();

		// change the resolution if needed.
		if (
			(ws_res[last_ws].timing.pixel_clock != ws_res[cur_switch].timing.pixel_clock) ||
			(ws_res[last_ws].timing.h_display != ws_res[cur_switch].timing.h_display) ||
			(ws_res[last_ws].timing.h_sync_start != ws_res[cur_switch].timing.h_sync_start) ||
			(ws_res[last_ws].timing.h_sync_end != ws_res[cur_switch].timing.h_sync_end) ||
			(ws_res[last_ws].timing.h_total != ws_res[cur_switch].timing.h_total) ||
			(ws_res[last_ws].timing.v_display != ws_res[cur_switch].timing.v_display) ||
			(ws_res[last_ws].timing.v_sync_start != ws_res[cur_switch].timing.v_sync_start) ||
			(ws_res[last_ws].timing.v_sync_end != ws_res[cur_switch].timing.v_sync_end) ||
			(ws_res[last_ws].timing.v_total != ws_res[cur_switch].timing.v_total) ||
			(ws_res[last_ws].timing.flags != ws_res[cur_switch].timing.flags) ||
			(ws_res[last_ws].virtual_width != ws_res[cur_switch].virtual_width) ||
			(ws_res[last_ws].virtual_height != ws_res[cur_switch].virtual_height) ||
			(ws_res[last_ws].space != ws_res[cur_switch].space) ||
			(ws_res[last_ws].flags != ws_res[cur_switch].flags)
			)
			// if set mode fails
			if (SetScreenMode(0, 0, &ws_res[cur_switch], false) != B_OK)
				// set to the current mode, which we know is valid
				SetScreenMode(0, 0, &ws_res[last_ws], false);
		
		// force desktop refresh.
		kill_region(desktop_region_refresh);
		desktop_region_refresh = calc_desk();

		// if there is a desktop window, and if its color changed, then invalidate it all.
		if ((desk_window) &&
			((desk_rgb[last_ws].red != desk_rgb[cur_switch].red) ||
			 (desk_rgb[last_ws].green != desk_rgb[cur_switch].green) ||
			 (desk_rgb[last_ws].blue != desk_rgb[cur_switch].blue))) {
			desk_window->force_full_refresh = true;
			/* the desk_window could contain no view */
			if (desk_window->top_view->first_child != NULL)
				desk_window->top_view->first_child->view_color = desk_rgb[cur_switch];
		}

		// close the atomic visible region transaction
		close_atomic_transaction();

		// change the current workspace enabled in the workspace app.
		if (workspace) workspace->SetWorkspace(cur_switch);

		// send the workspace activate/desactivate events.
		activate = FALSE;
		deact_event.AddInt64("when",system_time());
		deact_event.AddInt32("workspace",last_ws);
		deact_event.AddBool("active",activate);

		activate = TRUE;
		act_event.AddInt64("when",system_time());
		act_event.AddInt32("workspace",cur_switch);
		act_event.AddBool("active",activate);

		a_window = windowlist;
		while (a_window) {
			if (a_window->is_ws_visible(last_ws))		a_window->EventPort()->SendEvent(&deact_event);
			if (a_window->is_ws_visible(cur_switch))	a_window->EventPort()->SendEvent(&act_event);
			a_window = a_window->NextWindowFlat();
		}

		// switching out of exclusive full screen operation
		release_direct_screen_and_region_sem();
	}
}

// ------------------------------------------------------------

void TWindow::set_flags(uint32 inDesign, uint32 inBehavior, uint32 inFlags, bool forceDecor)
{	
	uint32		tmp_mask, id;
	TWindow		*a_window;

	LockRegionsForWrite();
	
	open_atomic_transaction();
	
	// set design if needed
	if (inDesign != 0xffffffff) {
		id = kind_2_proc_id(inDesign);
		if (id != (uint32)proc_id) {
			proc_id = id;
			forceDecor = true;
		}
	}
	if (forceDecor) {
		if (m_decor) m_decor->ServerUnlock();
		m_decor = GenerateDecor(proc_id);
		m_decor->ServerLock();

		OpenDecorTransaction();
		m_newDecorState->SetBounds(window_bound);
		// 10/29/2000 - mathias: Must send the button state to the decor
		m_newDecorState->SetInteger("@closeButton?",(flags&NO_GOAWAY)?0:1);
		m_newDecorState->SetInteger("@maximizeButton?",(flags&NO_ZOOM)?0:1);
		m_newDecorState->SetInteger("@minimizeButton?",(flags&NO_MINIMIZE)?0:1);
		m_newDecorState->SetInteger("@moveButton?",(flags&NO_MOVE)?0:1);
		m_newDecorState->SetInteger("@resizable?",(flags&NO_SIZE)?0:1);
		m_newDecorState->SetString("@title",get_name());
		m_newDecorState->SetFocus((state&look_active)?1:0);
		m_newDecorState->BitMask()->setAll();
	}

	// set flags if needed
	if (inFlags != 0xffffffff) {
		// process directly flags that can be modified without side-effect
		tmp_mask = FIRST_CLICK | DISLIKE_FRONT | MOVE_TO_WS | BLOCK_WS_SWITCH |
				   DISLIKE_FOCUS | NO_LIVE_SIZE | NO_WS_SWITCH | NO_DRAWING | LOCK_FOCUS | VIEWS_OVERLAP;
		flags = (flags&(~tmp_mask)) | (inFlags&tmp_mask);
		// process movable, zoomable, closable and minimizable
		tmp_mask = NO_ZOOM | NO_GOAWAY | NO_MINIMIZE | NO_MOVE;
		if ((flags & tmp_mask) != (inFlags & tmp_mask)) {
			flags = (flags&(~tmp_mask)) | (inFlags&tmp_mask);
			OpenDecorTransaction();
			m_newDecorState->SetInteger("@closeButton?",(flags&NO_GOAWAY)?0:1);
			m_newDecorState->SetInteger("@maximizeButton?",(flags&NO_ZOOM)?0:1);
			m_newDecorState->SetInteger("@minimizeButton?",(flags&NO_MINIMIZE)?0:1);
			m_newDecorState->SetInteger("@moveButton?",(flags&NO_MOVE)?0:1);
		}
		// process the sticky REQUIRE_KILL flag, only for direct window.
		if ((inFlags & REQUIRE_KILL) && (dw != NULL))
			flags |= REQUIRE_KILL;

		// process resizable (NEED TO BE PROCESS LAST)
		// setting any bit means setting everything, or with inherited setting
		tmp_mask = NO_SIZE | ONLY_VSIZE | ONLY_HSIZE;
		inFlags = (flags&(~tmp_mask)) | (inFlags&tmp_mask);
		// setting both half means setting all.
		if ((inFlags & NO_SIZE) || ((inFlags & ONLY_VSIZE) && (inFlags & ONLY_HSIZE)))
			inFlags = NO_SIZE;
		// do the setting.
		if (((flags^inFlags) & tmp_mask) != 0) {
			// check if it's a change from or to NO_SIZE
			if (((flags^inFlags) & NO_SIZE) != 0) {
				// Set the resize variable to the _new_ value, if changed.
				OpenDecorTransaction();
				m_newDecorState->SetInteger("@resizable?",(inFlags&NO_SIZE)?0:1);
			}
			flags = (flags&(~tmp_mask)) | (inFlags&tmp_mask);
		}
	}
	// set behavior if needed (restrict to allowed values only).
	if ((inBehavior == NORMAL) ||
		(inBehavior == MODAL_TEAM) ||
		(inBehavior == MODAL_LIST) ||
		(inBehavior == MODAL_SYSTEM) ||
		(inBehavior == FLOATING_TEAM) ||
		(inBehavior == FLOATING_LIST) ||
		(inBehavior == FLOATING_SYSTEM) ||
		(inBehavior == DESKTOP_WINDOW) ||
		(inBehavior == MENU_WINDOW)) {
		// handle collision between multiple team-wide modals for compatibility
		if (inBehavior == MODAL_TEAM) {
			a_window = windowlist;
			while (a_window) {
				if (a_window->is_team_modal() && (a_window->team_id == team_id))
					add_subwindow(a_window);
				a_window = a_window->nextwindow;
			}
		}
		// do the affectation
		if (is_screen()) {
			if (inBehavior != not_fullscreen_behavior)	
				not_fullscreen_behavior = inBehavior;
		}
		else {
			if (inBehavior != behavior) {		
				behavior = inBehavior;
			}
		}
	}
	
	// if reordering/activation check needed, do it
	close_atomic_transaction();
	UnlockRegionsForWrite();
}

//-----------------------------------------------------------
// window alignment management

void align32::set_parameters(int32 offset, int32 step, int32 mode)
{
	if (step <= 0)
		step = 1;
	offset -= step*(offset/step);
	if (offset < 0)
		offset += step;		
	_offset = offset;
	_step = step;
	_mode = mode;
}

void align32::get_parameters(int32 *offset, int32 *step, int32 *mode)
{
	*offset = _offset;
	*step = _step;
	*mode = _mode;
}

void align32::set(int32 real, TWindow *)
{
	int32		delta, offset, step, factor, base;

	switch (_mode) {
	default:
	case WINDOW_PIXEL_ALIGNMENT :
		offset = _offset;
		step = _step;
		break;
	case WINDOW_BYTE_ALIGNMENT :
		delta = grPixelFormat2BitsPerPixel(ScreenCanvas(0)->pixels.pixelFormat)>>3;
		if (delta < 1)
			delta = 1;
		for (factor=1; factor<=8; factor++) {
			offset = (_offset*factor)/delta;
			step = (_step*factor)/delta;
			if ((step*delta == _step*factor) && (offset*delta == _offset*factor))
				break;
		}
		break;
	}
	delta = (real-offset)+step/2;
	if (delta >= 0)
		base = delta/step;
	else
		base = -((step-1-delta)/step);
	_aligned = offset+base*step;
}

aligner::aligner() {
	pa = 0L;
}

aligner::~aligner() {
	if (pa != 0L)
		grFree(pa);
}

status_t aligner::set(	int32	mode,		int32	h,				int32	h_offset,
						int32	width,		int32	width_offset,	int32	v,
						int32	v_offset,	int32	height,			int32	height_offset,
						TWindow	*window)
{
	int32			dt, dl, db, dr, dh, dv;
	priv_aligner	*old_pa;

	if (((mode != WINDOW_BYTE_ALIGNMENT) && (mode != WINDOW_PIXEL_ALIGNMENT)) 	||
		(h < 0)			|| (h_offset < 0) 		|| (h_offset > h)				||
		(width < 0)		|| (width_offset < 0) 	|| (width_offset > width)		||
		(v < 0)			|| (v_offset < 0) 		|| (v_offset > v)				||
		(height < 0)	|| (height_offset < 0)	|| (height_offset > height))
		return B_ERROR;
		
	window->LockRegionsForWrite();
	
	old_pa = pa;
	if (pa == 0)
		pa = (priv_aligner*)grMalloc(sizeof(priv_aligner),"aligner stuff",MALLOC_CANNOT_FAIL);
		
	dt = dl = db = dr = 0;
	pa->h.set_parameters(h_offset, h, mode);
	pa->width.set_parameters(width_offset, width, mode);
	pa->v.set_parameters(v_offset, v, WINDOW_PIXEL_ALIGNMENT);
	pa->height.set_parameters(height_offset, height, WINDOW_PIXEL_ALIGNMENT);
	
	if (old_pa == 0) {
		dh = window->FullRegion()->Bounds().right - window->FullRegion()->Bounds().left;
		dv = window->FullRegion()->Bounds().bottom - window->FullRegion()->Bounds().top;
		h = moveto_h(window->ClientBound()->left, window);
		v = moveto_v(window->ClientBound()->top, window);
		dh = moveto_width(dh - dr - dl, window);
		dv = moveto_height(dv - db - dt, window);
	}
	else {
		h = moveto_h(pa->h.aligned(), window);
		v = moveto_v(pa->v.aligned(), window);
		dh = moveto_width(pa->width.aligned()-1, window);
		dv = moveto_height(pa->height.aligned()-1, window);
	}
		
	window->moveto_w(h, v);
	window->size_to(dh, dv);
	
	window->UnlockRegionsForWrite();
	
	return B_NO_ERROR;
}

status_t aligner::get(	int32	*mode,		int32	*h,				int32	*h_offset,
						int32	*width,		int32	*width_offset,	int32	*v,
						int32	*v_offset,	int32	*height,		int32	*height_offset)
{
	int32		fake_mode;

	if (pa == 0) {
		*mode = WINDOW_BYTE_ALIGNMENT;
		*h = 1;
		*h_offset = 0;
		*width = 1;
		*width_offset = 0;
		*v = 1;
		*v_offset = 0;
		*height = 1;
		*height_offset = 0;
		return B_NO_ERROR;
	}
	else {
		pa->h.get_parameters(h_offset, h, mode);
		pa->width.get_parameters(width_offset, width, &fake_mode);
		pa->v.get_parameters(v_offset, v, &fake_mode);
		pa->height.get_parameters(height_offset, height, &fake_mode);
		return B_NO_ERROR;
	}
}

int32 aligner::moveto_h(int32 new_h, TWindow *window)
{
	if (pa == NULL)
		return new_h;
	pa->h.set(new_h, window);
	return pa->h.aligned();
}

int32 aligner::moveto_v(int32 new_v, TWindow *window)
{
	if (pa == NULL)
		return new_v;
	pa->v.set(new_v, window);
	return pa->v.aligned();
}

int32 aligner::moveto_width(int32 new_width, TWindow *window)
{
	if (pa == NULL)
		return new_width;
	pa->width.set(new_width+1, window);
	return pa->width.aligned()-1;
}

int32 aligner::moveto_height(int32 new_height, TWindow *window)
{
	if (pa == NULL)
		return new_height;
	pa->height.set(new_height+1, window);
	return pa->height.aligned()-1;
}

// ------------------------------------------------------------
// #pragma mark -

void TWindow::SessionWillBlock()
{
	if (last_view) grGiveUpLocks(last_view->renderContext);
}

//	Locking call implementations... views and windows all use
//	the same locks, for now.

int32 TWindow::LockRegionsForRead()
{
	int32 pid = xgetpid();
	if ((pid == task_id) && last_view) grGiveUpLocks(last_view->renderContext);
	the_server->LockWindowsForRead(pid);
	return HasClosed();
}

int32 TWindow::UnlockRegionsForRead()
{
	the_server->UnlockWindowsForRead();
	return 0;
}

int32 TWindow::LockRegionsForWrite()
{
	int32 pid = xgetpid();
	if ((pid == task_id) && last_view) grGiveUpLocks(last_view->renderContext);
	the_server->LockWindowsForWrite(pid);
	return HasClosed();
}

int32 TWindow::UnlockRegionsForWrite()
{
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
	int32 pid = xgetpid();
	if ((pid == task_id) && last_view) grGiveUpLocks(last_view->renderContext);
	the_server->UnlockWindowsForWrite();
	return 0;
}

int32 TWindow::LockViewsForRead()
{
	return LockRegionsForRead();
}

int32 TWindow::UnlockViewsForRead()
{
	return UnlockRegionsForRead();
}

int32 TWindow::LockViewsForWrite()
{
	return LockRegionsForWrite();
}

int32 TWindow::UnlockViewsForWrite()
{
	return UnlockRegionsForWrite();
}

int32 TWindow::ViewRegionsChanged(TView *v)
{
	if (v == m_setView) m_setView = NULL;
	grGiveUpLocks(v->renderContext);
	return 0;
}

// ------------------------------------------------------------

RenderPort * TWindow::LockPort(TView *v, uint32 *token, int32 pid)
{
	(void)token;

	TWindow *w = v->owner;

	if (!dynamic_cast<TOff*>(w)) {
		GlobalRegionsLock.ReadLock(pid);
//		v->m_rFlags |= VFLAG_REGIONS_LOCKED;
	}

	if (v == w->m_setView) {
		w->m_ueberPort->ueberInfoFlags = w->m_ueberPort->infoFlags = 0;
		return w->m_ueberPort;
	}
	
	RenderPort *ueberPort = w->Port();
	w->m_setView = v;

	if (w->m_viewRFlags != v->m_rFlags) {
		w->m_viewRFlags = v->m_rFlags;
		if (w->m_viewRFlags & VFLAG_CLIP_TO_CLIENT) {
			if ((w->m_viewRFlags & VFLAG_IN_UPDATE) && w->m_invalidRegion) {
				and_region(w->m_clientRegion, w->VisibleRegion(), ueberPort->drawingClip);
				and_region(ueberPort->drawingClip, w->m_invalidRegion, w->m_preViewPortClip);
			} else {
				and_region(w->m_clientRegion, w->VisibleRegion(), w->m_preViewPortClip);
			}
		} else {
			if ((w->m_viewRFlags & VFLAG_IN_UPDATE) && w->m_invalidRegion) {
				and_region(w->VisibleRegion(), w->m_invalidRegion, w->m_preViewPortClip);
			} else {
				copy_region(w->VisibleRegion(), w->m_preViewPortClip);
			}
		}
		offset_region(w->m_preViewPortClip, -w->ClientBound()->left, -w->ClientBound()->top);
	}

	ueberPort->origin.h = (int32)(w->ClientBound()->left + v->bound.left + v->offset_h);
	ueberPort->origin.v = (int32)(w->ClientBound()->top + v->bound.top + v->offset_v);
	ueberPort->phase_x = 7 - (ueberPort->origin.h & 7);
	ueberPort->phase_y = 7 - (ueberPort->origin.v & 7);

	if (w->m_dbViews &&
		((w->m_viewRFlags & (VFLAG_IN_UPDATE|VFLAG_CLIP_TO_CLIENT)) == (VFLAG_IN_UPDATE|VFLAG_CLIP_TO_CLIENT))) {
		TView *dbView = w->m_dbViews;
		region *bufferedRegion = newregion();
		region *tmpRegion = newregion();
		region *tmpRegion2 = newregion();
		region *bitmapClipRegion = newregion();

		// Clip the window's current clipping with the curent view's visible region
		and_region(v->VisibleRegion(),w->m_preViewPortClip,ueberPort->drawingClip);

		// Compute the area covered by each doublebuffered view and gather them
		while (dbView) {
			and_region(ueberPort->drawingClip, dbView->m_doubleBufferRegion, tmpRegion2);

			// 10/09/2000 - mathias: we need to clip to the bitmap's size
			// if we don't do that, we may draw outside the bitmap and corrupt memory
			rect bitmapClip;
			int32 w, h;
		#if ROTATE_DISPLAY
			if (dbView->m_db->bitmap.Canvas()->pixels.pixelIsRotated) {
				w = dbView->m_db->bitmap.Canvas()->pixels.h;
				h = dbView->m_db->bitmap.Canvas()->pixels.w;
			} else 
		#endif
			{
				w = dbView->m_db->bitmap.Canvas()->pixels.w;
				h = dbView->m_db->bitmap.Canvas()->pixels.h;
			}
			set_rect(	bitmapClip,
						dbView->m_db->originInWindow.v,
						dbView->m_db->originInWindow.h,
						dbView->m_db->originInWindow.v + h-1,
						dbView->m_db->originInWindow.h + w-1);
			set_region(bitmapClipRegion, &bitmapClip);

			and_region(bitmapClipRegion, tmpRegion2, dbView->m_db->port.portClip);
			or_region(dbView->m_db->port.portClip, bufferedRegion, tmpRegion);

			region *swapper = tmpRegion;
			tmpRegion = bufferedRegion;
			bufferedRegion = swapper;
			dbView = dbView->m_db->next;
		};

		// ueberPort->portClip = drawingClip less the areas covered by doublebuffered view
		sub_region(ueberPort->drawingClip, bufferedRegion, ueberPort->portClip);
		offset_region(ueberPort->portClip,w->ClientBound()->left,w->ClientBound()->top);

		w->m_ueberPort = ueberPort;
		RenderSubPort *port,**portPtr = &ueberPort->next;
		if (empty_region(ueberPort->portClip)) portPtr = (RenderSubPort**)&w->m_ueberPort;
		else copy_region(ueberPort->drawingClip,bufferedRegion);
		
		dbView = w->m_dbViews;
		while (dbView) {
			*portPtr = port = &dbView->m_db->port;
			port->origin.h = (int32)(v->bound.left + v->offset_h - dbView->m_db->originInWindow.h);
			port->origin.v = (int32)(v->bound.top + v->offset_v - dbView->m_db->originInWindow.v);
			port->phase_x = 7 - (port->origin.h & 7);
			port->phase_y = 7 - (port->origin.v & 7);
			offset_region(port->portClip,-dbView->m_db->originInWindow.h,-dbView->m_db->originInWindow.v);
			port->infoFlags = RPORT_RECALC_REGION | RPORT_RECALC_ORIGIN;
			if (!empty_region(port->portClip)) portPtr = &port->next;
			dbView = dbView->m_db->next;
		}
		*portPtr = NULL;

		if (w->m_ueberPort == NULL) w->m_ueberPort = ueberPort;
		w->m_ueberPort->portBounds = bufferedRegion->Bounds();
		offset_rect(&w->m_ueberPort->portBounds,-((int32)(v->bound.left + v->offset_h)),-((int32)(v->bound.top + v->offset_v)));

		kill_region(bitmapClipRegion);
		kill_region(tmpRegion2);
		kill_region(tmpRegion);
		kill_region(bufferedRegion);
	} else {
		and_region(v->VisibleRegion(), w->m_preViewPortClip, ueberPort->portClip);
		offset_region(ueberPort->portClip, w->ClientBound()->left, w->ClientBound()->top);
		ueberPort->portBounds = ueberPort->portClip->Bounds();
		offset_rect(&ueberPort->portBounds, -ueberPort->origin.h, -ueberPort->origin.v);
		ueberPort->next = NULL;
		w->m_ueberPort = ueberPort;
	}

#if ROTATE_DISPLAY
	if (ueberPort->canvasIsRotated)
		ueberPort->rotater->RotateRegion(ueberPort->drawingClip, ueberPort->rotatedDrawingClip);
#endif
	
	w->m_ueberPort->ueberInfoFlags = w->m_ueberPort->infoFlags = RPORT_RECALC_REGION | RPORT_RECALC_ORIGIN;
	return w->m_ueberPort;
}

void TWindow::UnlockPort(TView *v, RenderPort *, int32 pid)
{
//	if (v->m_rFlags & VFLAG_REGIONS_LOCKED) {
//		v->m_rFlags &= ~VFLAG_REGIONS_LOCKED;
	if (!dynamic_cast<TOff*>(v->owner)) {
		if (pid < 0)	GlobalRegionsLock.ReadUnlock();
		else			GlobalRegionsLock.ReadUnlock(pid);
	}
//	}
};

// ------------------------------------------------------------
// #pragma mark -

void TWindow::OffsetRegions(int32 h, int32 v)
{
	//	We don't do the visible region here, since it may change
	//	any time we move the window.  You'll have to recalc it
	//	yourself.
	offset_region(m_clientRegion,h,v);
	offset_region(m_fullRegion,h,v);
	if (m_invalidRegion) offset_region(m_invalidRegion,h,v);
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
}

void TWindow::SetClientRegion(region *r)
{
	copy_region(r,m_clientRegion);
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
}

void TWindow::SetFullRegion(region *r)
{
	copy_region(r,m_fullRegion);
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
}

void TWindow::SetVisibleRegion(region *r)
{
	copy_region(r,VisibleRegion());
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
}

void TWindow::SetClearRegion(region *r)
{
	copy_region(r,m_clearRegion);
}

void TWindow::SetInvalidRegion(region *r)
{
	if (r) {
		if (!m_invalidRegion) m_invalidRegion = newregion();
		copy_region(r,m_invalidRegion);
	} else {
		if (m_invalidRegion) {
			kill_region(m_invalidRegion);
			m_invalidRegion = NULL;
		}
	}
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
}

//-----------------------------------------------------------

void TWindow::LocalizeEvent(BParcel *msg, TView *target, TView *underMouse, int32 *transit_out)
{
	fpoint location = msg->Location();
	if (msg->what == B_MOUSE_MOVED) {
		int32 pointer = msg->FindInt32("pointer");
		int32 transit = msg->Transit();
		const bool insideWindow = (transit == B_ENTERED) || (transit == B_INSIDE);
		if (insideWindow && (target == underMouse)) {
			transit = (target->ContainsPointer(pointer)) ? B_INSIDE : B_ENTERED;
			target->SetPointer(pointer,true);
		} else {
			transit = (target->ContainsPointer(pointer)) ? B_EXITED : B_OUTSIDE;
			target->SetPointer(pointer,false);
		}
		if (transit_out) *transit_out = transit;
		if (msg->ReplaceInt32("be:transit",transit) != B_OK)
			msg->AddInt32("be:transit",transit);
		msg->SetTransit(transit);
	}

	if (msg->HasLocation()) {
		location.h -= (target->bound.left + target->offset_h);
		location.v -= (target->bound.top + target->offset_v);
		if (msg->ReplacePoint("be:view_where",location) != B_OK)
			msg->AddPoint("be:view_where",location);
		if ((msg->what == B_MOUSE_DOWN) ||
			(msg->what == B_MOUSE_UP))
			msg->ReplacePoint("where",location);
	}
}

//-----------------------------------------------------------

void TWindow::HandleEvent(BParcel *event)
{
	int32 transit,vtransit;
	fpoint location,screenLocation;
	TView *target = NULL;
	TView *otherTarget = NULL;
	BParcel *beingDragged;
	bool inVisibleRegion=false,inClientRegion=false;
	bool eventAlreadySent=false; // do not send the event twice when it is broadcasted

	if ((beingDragged = event->Dragging()) != NULL &&
		(beingDragged->BodyArea() != NULL)) {
		event->AddInt32("_msg_data_",beingDragged->BodyArea()->ID());
		event->AddInt32("_msg_what_",beingDragged->what);
		if (beingDragged->HasWhen()) {
			event->AddInt64("_msg_when_",beingDragged->When());
		}
		Application()->SetFlag(AFLAG_DISPOSE_DRAG);
	}

	/* The event is in global space */
	if (event->HasLocation()) {
		screenLocation = event->Location();
		location.h = screenLocation.h - ClientBound()->left;
		location.v = screenLocation.v - ClientBound()->top;
		event->SetLocation(location);
		if (event->ReplacePoint("where",location) != B_OK)
			event->AddPoint("where",location);
		inVisibleRegion = point_in_region(VisibleRegion(), (int32)screenLocation.h, (int32)screenLocation.v);
		inClientRegion = point_in_region(ClientRegion(), (int32)screenLocation.h, (int32)screenLocation.v);
	}

	/*	First we take care of window-level handling and routing
		of the event.  We only want to do this if the message was
		routed here, not if it was broadcast. */
	if ((event->Routing() & rtThin) &&
		(event->what == B_MOUSE_DOWN) &&
		inVisibleRegion && !inClientRegion) { // clicked on the border/tab
		/* This is the only case the window itself cares about */
		// 16/1/2001 - Mathias: Filter out wrong double clicks
		int32 clicks, buttons;
		event->FindInt32("clicks", &clicks);
		event->FindInt32("buttons", &buttons);
		if (	clicks <= 1
				|| (the_server->m_lastClickedWindow != this)
				|| ((buttons&m_lastButtons) != m_lastButtons)
				|| (m_firstClickWindowPos.h != ClientBound()->left)
				|| (m_firstClickWindowPos.v != ClientBound()->top)) {
			// Restart double click count.  Do this if:
			// (1) This is the first click.
			// (2) The last click was not in this window.
			// (3) A button that was down during the last click
			//     is no longer held.
			// (4) The window moved (same behaviour than R5)
			the_server->m_lastClickedWindow = this;
			m_lastButtons = buttons;
			m_clickCount = 1;
			m_firstClickWindowPos.h = ClientBound()->left;
			m_firstClickWindowPos.v = ClientBound()->top;
		} else {
			m_clickCount++;
		}
	
		event->ReplaceInt32("clicks", m_clickCount);
		the_server->WindowManager()->EventPort()->SendEvent(event,client_token,false,this);
	} else if (event->Routing() & rtThin) {
		if (event->what == B_MOUSE_DOWN) { // this click cannot be part of border/tab double-click
			the_server->m_lastClickedWindow = NULL;
		}

		/* Do any reordering we need */
		bool dispatch = true;
		if (event->what == B_MOUSE_DOWN) {
			bool is_blocked = has_modal();
			dispatch = false;
	
			UnlockRegionsForRead();

			if (!is_focus()) {
				if (front_protection)
					set_front_limit(NULL);
			
				if (!(flags & FIRST_CLICK)) {
					bring_to_front();
					if (!(flags & DISLIKE_FOCUS))
						goto getOut;
				}
			}
		
			if (!is_blocked)	dispatch = true;
			else				bring_to_front();
	
			getOut:
			LockRegionsForRead();
		}

		if (dispatch) {
			if (event->HasLocation()) event->SetLocation(location);
			/* The event is in window space */
			switch (event->what) {
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
				case B_MOUSE_MOVED:
					transit = event->Transit();
					target =	(((transit==B_ENTERED) || (transit==B_INSIDE)) &&
									inClientRegion &&
									inVisibleRegion &&
									(target != top_view))
								? top_view->FindView(location) : NULL;

					if (target) target->ServerLock();

					if (m_mouseView && (m_mouseView->is_still_valid()))
						m_mouseView->ServerLock();
					else
						m_mouseView = NULL;

					if ((target) && (m_mouseView) && (target == m_mouseView)) // (B_INSIDE)
					{ // the mouse event is still in the same view						
						LocalizeEvent(event,target,target);
						fpoint view_coord;
						event->FindPoint("be:view_where", &view_coord);
						if (event->what == B_MOUSE_MOVED) {
							if ((view_coord.h == m_mouseViewCoords.h) &&
								(view_coord.v == m_mouseViewCoords.v))
							{ // don't send multiple B_MOUSE_MOVED
								goto do_nothing;
							}
						}
						target->DispatchEvent(event);
					do_nothing:;
					}
					else if ((target) && (m_mouseView) && (target != m_mouseView)) // (B_INSIDE)
					{ // mouse event changed of view
						otherTarget = m_mouseView;
						LocalizeEvent(event,m_mouseView,target);
						m_mouseView->DispatchEvent(event);
						LocalizeEvent(event,target,target);
						target->DispatchEvent(event);
						the_cursor->MakeViewCursorCurrent(target);
					}
					else if ((target == NULL) && (m_mouseView))	// (B_INSIDE, B_EXITED, ?B_ENTERED)
					{ // event in window space, but was in a view
						otherTarget = m_mouseView;
						LocalizeEvent(event,m_mouseView,target);
						m_mouseView->DispatchEvent(event);
						
						const uint32 dtransit = (transit == B_INSIDE) ? B_ENTERED : transit;
						if (event->ReplaceInt32("be:transit", dtransit) != B_OK)
							event->AddInt32("be:transit", dtransit);
						event->SetTransit(dtransit);
						event->SetTarget(client_token, false);
						if (event->what == B_MOUSE_MOVED)
							the_server->WindowManager()->EventPort()->ReplaceEvent(event,&m_queueIter_decorMM,&m_decorMMToken,this);
						else
							the_server->WindowManager()->EventPort()->SendEvent(event,client_token,false,this);
						eventAlreadySent = true; // remember that we sent this event already
						the_cursor->MakeSystemCursorCurrent();
						m_decorMMToken = NONZERO_NULL;
						m_queueIter_decorMM = 0xFFFFFFFF;
					}
					else if ((target) && (m_mouseView == NULL)) // (B_ENTERED, B_INSIDE)
					{ // event in view space, but was in window space
						const uint32 dtransit = (transit == B_INSIDE) ? B_EXITED : transit;
						if (event->ReplaceInt32("be:transit", dtransit) != B_OK)
							event->AddInt32("be:transit", dtransit);
						event->SetTransit(dtransit);
						event->SetTarget(client_token, false);
						the_server->WindowManager()->EventPort()->ReplaceEvent(event,&m_queueIter_decorMM,&m_decorMMToken,this);
						eventAlreadySent = true; // remember that we sent this event already
						m_decorMMToken = NONZERO_NULL;
						m_queueIter_decorMM = 0xFFFFFFFF;

						event->SetTransit(B_ENTERED);
						LocalizeEvent(event,target,target,&vtransit);
						target->DispatchEvent(event);
						the_cursor->MakeViewCursorCurrent(target);
					}
					else if ((target == NULL) && (m_mouseView == NULL)) // (B_EXITED, B_ENTERED, B_INSIDE)
					{ // event in window space, but was not in a view space
						if (event->ReplaceInt32("be:transit", transit) != B_OK)
							event->AddInt32("be:transit", transit);
						event->SetTarget(client_token, false);
						if (event->what == B_MOUSE_MOVED)
							the_server->WindowManager()->EventPort()->ReplaceEvent(event,&m_queueIter_decorMM,&m_decorMMToken,this);
						else
							the_server->WindowManager()->EventPort()->SendEvent(event,client_token,false,this);
						eventAlreadySent = true; // remember that we sent this event already
						if (transit == B_ENTERED) {
							the_cursor->MakeSystemCursorCurrent();
							m_decorMMToken = NONZERO_NULL;
							m_queueIter_decorMM = 0xFFFFFFFF;
						}
					}

					if (event->what == B_MOUSE_MOVED)
						event->FindPoint("be:view_where", &m_mouseViewCoords);

					// save the view under the mouse
					if (target) target->ServerUnlock();
					if (m_mouseView) m_mouseView->ServerUnlock();
					m_mouseView ‹}hstibè¾ÿÿ‰GdÇEä    ÇEè    ÇEì    EìPEèPEäP‹G`P‹G\Pèú¸ÿÿ‹Eä1ÒRPß,$ƒÄÙ_h‹Eè1ÒRPß,$ƒÄÙ_lƒÄƒ}ìuPEìPj j ‹G`P‹G\Pèœ¼ÿÿƒÄƒ}ìuİƒl´ÿÿØhÙ]üÙEüëÙîÙ_hƒ}ìuİƒl´ÿÿØlÙ]üÙEüëÙîÙ_l‹G`P‹G\Pè„µÿÿ1ÒRPß,$ƒÄÙèŞéØGhÙ]üÙEüÙ_p‹G`P‹G\PèMµÿÿ1ÒRPß,$ƒÄÙèŞéØGlÙ]üÙEüÙ_tOhƒÄÙGh² ØYßà€äEşÌ€ü@sÙAØYßà€äEşÌ€ü@s²„Òt}Wh‰UàÙBØghÙèŞÁÙ}ø‹Møµ‰MğÙmğß}ğ‹Eğ‹UôÙmø‰Æ‹G`P‹G\PèØ´ÿÿƒÄ9Æu=‹EàÙ@Ø`ÙèŞÁÙ}ø‹Møµ‰MğÙmğß}ğ‹Eğ‹UôÙmø‰Æ‹G`P‹G\Pè‹´ÿÿƒÄ9ÆtYÇGh    ÇGl    ‹G`P‹G\Pèy´ÿÿ1ÒRPß,$ƒÄÙèŞéÙ]üÙEüÙ_p‹G`P‹G\PèE´ÿÿ1ÒRPß,$ƒÄÙèŞéÙ]üÙEüÙ_tƒÄ‹G`P‹G\PèN³ÿÿ‰Gx‹G`P‹G\Pè¾·ÿÿˆEÜ‹G`P‹G\Pè·ÿÿƒÄŠUÜöÂt+ÇG|   öÂtÇG|   ‹G`P‹G\Pèã³ÿÿÁà‰GxƒÄëÇG|   <uÇG|   ‹G`P‹G\Pè©³ÿÿ¯Gx‰‡€   ÙGhƒìÙ$è1µÿÿÙ_hÙGlƒìÙ$è µÿÿÙ_lÙGpƒìÙ$èµÿÿÙ_pÙGtƒìÙ$èş´ÿÿÙ_t‹GxPèB»ÿÿ‰Gx‹G|Pè6»ÿÿ‰G|ƒÄ ‹‡€   Pè$»ÿÿ‰‡€   ƒÄ‹W‹Jj j j ¿   ĞP‹”   ÿĞ‰Ñ‰ÂƒÄ‰Ğ	Èu Gd‹W‹Jj P¿AĞP‹AÿĞƒÄƒø }¸  €ë/‹‡€   Pè¼ºÿÿƒÀ 1ÒƒÄ‹O‹qRP¿†    ÈP‹†¤   ÿĞeĞ[^_‰ì]Ã´&    ‹$ÃU‰åSèóÿÿÿÃ—ô  ‹U‹ƒ°  ‰BöEtRèÍ¹ÿÿ‹]ü‰ì]Ã‰öU‰å‹M‹U…Òt‹‰‹‰ì]Ã‹$ÃU‰åWVSèñÿÿÿÃIô  ‹u‹}‹ƒì  ‰FjFPè-¶ÿÿWVè‚ÿÿÿeô[^_‰ì]Ã‹$ÃU‰åSèóÿÿÿÃô  ‹M‹U‹ƒü  ‰ARQèOÿÿÿ‹]ü‰ì]Ã‹$ÃU‰åì   WVSèëÿÿÿÃÓó  ½ ğÿÿv ‹E‹P‹Jh   W¿AĞP‹AÿĞ‰ÆƒÄ…ö~,‹E‹P‹JVW¿AĞP‹AÿĞƒÄ9ğt¾¸ H €ë´&    Fƒøv¸ÿÿÿÿë1À¥ôïÿÿ[^_‰ì]Ã‹$ÃU‰åVSèòÿÿÿÃFó  ‹u…öt‹Pè¥¹ÿÿVèŸ¹ÿÿeø[^‰ì]Ã‰ö‹$ÃU‰åƒìWVSèîÿÿÿÃó  ‹uÇEü    1ÒÇEø    ƒ} tR9uüs5‹Eƒ8 t-‹Mü‹E‹<ˆ° ü¹ÿÿÿÿò®‰È÷ĞÂÿEü9uüs‹Mü‹Eƒ<ˆ uÓ…ÒtRè°ÿÿ‰EøƒÄ…Àu1Àës‹Uü•   Pèq°ÿÿ‰EôƒÄ…Àu‹MøQèî¸ÿÿ1ÀëL‰ö1ö;uüs3‹Uø‹Eô‰°‹M‹±PRè‹®ÿÿ‹}ø° ü¹ÿÿÿÿò®‰È÷ĞEøƒÄF;uürÍ‹Uü‹MôÇ‘    ‹Eôeè[^_‰ì]ÃU‰åWV‹M‹u1ÿ1Òƒ9 t9òru‹<‘‹D‘‰‘Bƒ<‘ ué…ÿt‹Eÿ‰ø^_‰ì]ÃU‰åWV‹M‹}1ö1Òƒ9 t…öu9<‘u‰ş…öt‹D‘‰‘Bƒ<‘ uã…öt‹Eÿ‰ğ^_‰ì]Ã´&    ¼'    ‹$ÃU‰åì<  WVSèëÿÿÿÃñ  Ç…Èûÿÿ    ‹EÆ  ‹»  …Ìûÿÿ‰…Äûÿÿv h4  ‹…ÄûÿÿP…ÈûÿÿPj èîµÿÿ‰ÆƒÄ…öu#‹Eğ9Ç|ÖEø9Ç}Ïh   …ğûÿÿP‹EPè’²ÿÿ‰ğ¥¸ûÿÿ[^_‰ì]Ãv ‹$ÃU‰åƒìWVSèîÿÿÿÃêğ  1ÿEôP‹EP‹EPèU´ÿÿ‰ÆƒÄ…öu\‹EøPè´ÿÿ‰ÇƒÄ…ÿu¾   €…öuA‹EøPWj j hGGSM‹EP‹EPèf³ÿÿ™ƒÄ;Eøu;Uüt¾  €…öuW‹EPèµ¯ÿÿ‰ÆƒÄWèºµÿÿ‰ğeè[^_‰ì]Ã‰ö‹$ÃU‰åƒìWVSèîÿÿÿÃFğ  1ö1ÿ‹U…ÒuUüEøPR‹EPhGGSM‹EPè°ÿÿƒÄ„Àu¾` €…öuN‹EøPèF³ÿÿ‰ÇƒÄ…ÿu¾   €…öu3‹EøPj j W‹EPhGGSM‹EPèi±ÿÿ‰ÆƒÄ…öuW‹EPè¯ÿÿ‰ÆƒÄWèµÿÿ‰ğeì[^_‰ì]Ãv ‹$ÃU‰åƒìWVSèîÿÿÿÃ–ï  ‹EPè{¬ÿÿ‰EüPèÂ²ÿÿ‰ÇƒÄ…ÿ”À%ÿ   ‰ÆÁæu?‹EüPW‹EPè®­ÿÿ‰ÆƒÄ…öu(‹EüPWj j hGGSM‹EP‹EPèú²ÿÿƒÄ;Eüt¾  €Wèw´ÿÿ‰ğeğ[^_‰ì]Ãv ‹$ÃU‰åƒìWVSèîÿÿÿÃï  1öÇEü    ‹EPèŞ«ÿÿ‰Ç‹EPhGGSM‹EPè
°ÿÿƒÄ„Àt‹EPhGGSM‹EPèÁ´ÿÿ‰ÆƒÄ…öutWèò±ÿÿ‰EüƒÄ…Àu¾   €…öu[W‹EüP‹EPèá¬ÿÿ‰ÆƒÄ…öuD‹EPW‹EüP‹EPhGGSM‹EPè]²ÿÿ‰ÆƒÄ…öu Wj j ‹EüP‹EPhGGSM‹EPèù®ÿÿ‰ÆƒÄ‹EüPè‹³ÿÿ‰ğeğ[^_‰ì]Ã´&    U‰åVSè    [Ãî  ‰ŞÆğÿÿÿƒ»ğÿÿÿÿt‹ÿĞƒÆüƒ>ÿuô[^‰ì]ÃU‰åSè    [Ãçí  [‰ì]Ã´&    ‹$ÃU‰åWVSèñÿÿÿÃÅí  ‹u‹}‹U‹EPRVè§
  ƒÄƒ¾¬   tWVè   ƒÄƒ¾¬   uíeô[^_‰ì]Ã‰ö‹$ÃU‰åVSèòÿÿÿÃrí  ‹E‹M‹´  ƒúwo‰Ş+´“¼ÿÿÿæ¶    ¿    ,í  #í  í  ôì  í  í  QPèQ   ëAQPè   ë8¶    Pè>
  ë*QPèu  ë!QPè  ë¶    Pè&  ë
Ç€¬      eø[^‰ì]Ã‹$ÃU‰åƒìWVSèîÿÿÿÃÆì  ‹E¶¸  ¸   ‰Æ)ş‹U9²¬  s‹²¬  V‹UD: P‹EPèÆ  ‰ò‹E   VW‹UƒÂ ‰UüRèò©ÿÿƒÄ…ÀtFƒÿw.FüPW‹EüPèØ©ÿÿƒÄ…Àt‰ØµÿÿP‹URè¨ÿÿë/¶    ‰Ø<µÿÿP‹EPèw¨ÿÿë‹U€º  v
Ç‚´     eğ[^_‰ì]Ã‹$ÃU‰åƒìWVSèîÿÿÿÃîë  ‹EfƒxL |Xƒ¸¬  wPè¿  éÑ  ¶    jEüP‹URèñ  EüPè «ÿÿ‹u‰†œ  Vè°ÿÿj‰ğü   PVèa¯ÿÿ€NM€ƒÄ ‹Eü   ‹»p  ¹   ‰Æü¨ ó¦u6‹U‹‚œ  ƒÀ;‚¬  ‡®  ‹u‹†œ  P‹EPVè"¨ÿÿé9  t& ‹Eü   ‹»\  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡\  ‹u‹†œ  P‹EPVèà¬ÿÿéç  v ‹Eü   ‹»ø  ¹   ‰Æü¨ ó¦ut‹Eö@Ht'ƒ¸œ   „¹  ö@Ht‰ØcµÿÿP‹URèï¦ÿÿƒÄ‹u‹†œ  ‰†ì   €NHÇ†´     ‹EPVè  ‹†¼   ‰Fd‹†Ì   ‰F`éa  ¶    ‹Eü   ‹»ì  ¹   ‰Æü¨ ó¦uE‹U‹‚œ  ƒÀ;‚¬  ‡|  ‹u‹†œ  P‹EPVèªÿÿÇ†´     ‹URVèÀ  éó  ‹Eü   ‹»0  ¹   ‰Æü¨ ó¦u3‹U‹‚œ  ƒÀ;‚¬  ‡  ‹u‹†œ  P‹EPVèOªÿÿé¦  ‰ö‹Eü   ‹»H  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡Ì  ‹u‹†œ  P‹EPVè`­ÿÿéW  v ‹Eü   ‹»¤  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡|  ‹u‹†œ  P‹EPVè°­ÿÿé  v ‹Eü   ‹»È  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡,  ‹u‹†œ  P‹EPVè ¥ÿÿé·  v ‹Eü   ‹»Ì  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡Ü  ‹u‹†œ  P‹EPVèĞ©ÿÿég  v ‹Eü   ‹»@  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡Œ  ‹u‹†œ  P‹EPVè€ªÿÿé  v ‹Eü   ‹»ü  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡<  ‹u‹†œ  P‹EPVèà§ÿÿéÇ  v ‹Eü   ‹»ä  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡ì   ‹u‹†œ  P‹EPVè@§ÿÿéw  v ‹Eü   ‹»  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  ‡œ   ‹u‹†œ  P‹EPVè€©ÿÿé'  v ‹Eü   ‹»¼  ¹   ‰Æü¨ ó¦u4‹U‹‚œ  ƒÀ;‚¬  wP‹u‹†œ  P‹EPVèªÿÿéÛ   ´&    ‹Eü   ‹»ˆ  ¹   ‰Æü¨ ó¦u:‹U‹‚œ  ƒÀ;‚¬  vRè  é    t& ‹u‹†œ  P‹EPVèT§ÿÿë~‹Eü   ‹»Ø  ¹   ‰Æü¨ ó¦u‹U‹‚œ  P‹uVRè>  ëL‹Eü   ‹»h  ¹   ‰Æü¨ ó¦u‹U‹‚œ  P‹uVRè¨  ë¶    ‹U‹‚œ  P‹uVRè8  ‹E€`Meğ[^_‰ì]Ãv U‰å‹E‹UÇ€´     ‰   ‰ì]Ãv ‹$ÃU‰åWVSèñÿÿÿÃ½å  ‹uƒ¾    „«   ƒ¾¤   tA‹†   ;†¤  s‰Çë‹¾¤  W‹†Œ  PVè©ÿÿ)¾   )¾¬  )¾¤  ¾Œ  ƒÄƒ¾    tXƒ¾°   tF‹†   ;†°  s	‰Çët& ‹¾°  W‹†”  PVèÄ¨ÿÿ)¾   )¾¬  )¾°  ¾”  ƒÄƒ¾    u*ƒ¾¬  wVèİ   ë´&    j Vèp©ÿÿÇ†´     eô[^_‰ì]Ã‹$ÃU‰åƒìWVSèîÿÿÿÃ¶ä  ‹}‹E‰Eüƒ¿¤   t=‹u;·¤  r‹·¤  ‹‡Œ  VP‹EüPè@¢ÿÿ)uuü)·¬  )·¤  ·Œ  ƒÄƒ} t=ƒ¿°   t4‹u;·°  r‹·°  ‹‡”  VP‹EüPèô¡ÿÿ)·¬  )·°  ·”  eğ[^_‰ì]Ã‹$ÃU‰åƒìWVSèîÿÿÿÃä  ‹}ƒ¿¤   t8‹‡Œ  ;‡  t*‹—¤  ‰Uü1ö‰Á‹—  ;uüs¶    ŠˆFAB;uürô‹‡°  ‡¤  ;‡¨  v>   ‰Eø‹·  PWèã£ÿÿ‰Â‰—  ‹‡¤  PVRè=¡ÿÿVWèæ¦ÿÿ‹Eø‰‡¨  ƒÄƒ¿°   t6‹—¤  ‹”  ‹‡°  PQ—  Rè¡ÿÿ‹‡°  ‡¤  Ç‡°      ‹‡  ‰‡Œ  Ç‡¬      eì[^_‰ì]Ãv U‰å‹E‹M‹U‰ˆ˜  ‰°  ¤  ‰¬  ‰ˆ”  ‰ì]Ã‰ö‹$ÃU‰åƒìWVSèîÿÿÿÃÂâ  ‹EfƒxL Œ±   ƒ¸¬  wPèşÿÿé¥  ¶    jEüP‹URèÁıÿÿEüPèp¢ÿÿ‹}‰‡œ  Wèá¦ÿÿj‰şÆü   VWè0¦ÿÿ€OM€‹“ø  ¹   ‰×ü1Àó¦tÀƒÄ …Àt-‹EÇ€´     ö@L …-  ‰ØyµÿÿP‹URèeÿÿé  ‹}‹‡œ  ‰‡ì   ‹Eƒ¸ì    „Ğ   ƒ¸¤   tS‹€ì   ‰Æ‹U;‚¤  r‹²¤  V‹}‹‡Œ  PWè^¥ÿÿV‹‡Œ  PWèÈ   )·ì   )·¬  )·¤  ·Œ  ƒÄ‹Eƒ¸ì    thƒ¸°   tS‹€ì   ‰Æ‹U;‚°  r‹²°  V‹}‹‡”  PWèö¤ÿÿV‹‡”  PWè`   )·ì   )·¬  )·°  ·”  ƒÄ‹Eƒ¸ì    u(‹Uƒº¬  wRèûüÿÿët& j ‹}Wè¥ÿÿ€gMeğ[^_‰ì]Ãv ‹$ÃU‰åWVSèñÿÿÿÃÙà  ‹u‹}öFL t…ÿt‰Ø½µÿÿPVè	ÿÿƒÄ‹E‰FT‰~Xë9v …Àt‰ØêµÿÿPVèæœÿÿƒÄƒ~d u`Vèk   ‹†¼   ‰Fd‹†Ì   ‰F`ƒÄjFTPè×ÿÿƒÄƒøu-ƒ~X t‰ØÔµÿÿPVè›œÿÿƒÄƒ~d uVè    €NH€NL ëƒøûu…eô[^_‰ì]Ãv ‹$ÃU‰åWVSèñÿÿÿÃà  ‹uŠ†  ˆ†è   ‹†À   ‰†à   Š†
  ˆ†ê   Š†  ˆ†é   Š†	  ˆ†ë   ¶†ë   ¯†à   ƒÀÁè‰†ä   ‹–Ì   ¶P‹†È   @PBR†à   PVè±ÿÿ‹†¸   @P‹†Ì   P‹†È   PVè5¢ÿÿƒÄ$ƒ~P t	Vè& ÿÿƒÄ€¾   „æ  öFP„Ü  €¾  w#‹FPP¶†  P‹†Ì   @P†à   Pè³¤ÿÿƒÄ¶†  ƒø‡´  ‰Ú+”ƒ!ÿÿÿâ¶    ¼'    ÔŞ  ”Ş  $Ş  ´İ  Dİ  ÔÜ  œÜ  t& 1ÿ€¾   …m  ‹†Ì   @PVè²	  Vèh  ƒÄGƒÿK  €¾   tÖé=  