/******************************************************************************
*
*	File:		workspace.cpp
*
*	Description:	workspace panel
*	
*	Written by:	Robert Polic
*
*	Copyright 1996, Be Incorporated
*
*******************************************************************************/

#include "gr_types.h"
#include "proto.h"
#include "server.h"
#include <string.h>
#include "view.h"
#include "font.h"
#include <window.h>
#include <workspace.h>
#include "render.h"
#include "renderUtil.h"
#include "graphicDevice.h"
#include "bitmap.h"
#include "eventport.h"

#include <Debug.h>

/*-------------------------------------------------------------*/

class	TWSView : public TView {
private:
TWindow*			fWindow;
SFont               vfont;
public:
long				fActive;

					TWSView(rect*, TWindow*);
					~TWSView();
			char	click(button_event);
virtual		void	Draw(bool inUpdate);
	virtual	void	HandleEvent(BParcel *event);
			void	DrawSpace(long, bool);
			void	HiliteSpace(long, long);
			void	SelectSpace(long);
			bool	IsVisible(TWindow *window);
};

#define as_long(v) *((long *)&v)

/*-------------------------------------------------------------*/

extern 	void				switcher(long i);
extern 	TWindow *			windowlist;
extern 	"C" long			cur_switch;
extern 	long				ws_count;
extern 	rgb_color			desk_rgb[MAX_WORKSPACE];
extern 	display_mode		ws_res[MAX_WORKSPACE];
		bool				get_bounds(TWindow*, rect*, rect*, long);
		void				set_bounds(TWindow*, rect*, long);
		void				offset_bounds(TWindow*, long, long, long);
		void				get_counts(long*, long*);
		void				space_rect(TWindow*, rect*, long, long);
		float				screen_width(long);
		float				screen_height(long);

/*===============================================================*/

TWS::TWS(TWindow* window)
{
	rect	r;

	fWindow = window;

	fWindow->get_bound(&r);
	r.bottom -= r.top;
	r.right -= r.left;
	r.top = r.left = 0;
	fView = new TWSView(&r, fWindow);
	
	fWindow->add_view(fView);
}

/*---------------------------------------------------------------*/

void TWS::DeskRGB(int32 index)
{
	fView->DrawSpace(index, FALSE);
}

/*---------------------------------------------------------------*/

void TWS::SetWorkspace(long space)
{
	fView->SelectSpace(space);
}

/*---------------------------------------------------------------*/

void TWS::WindowChanged(TWindow* window)
{
	long	loop;

	for (loop = 0; loop < ws_count; loop++)
		if (window->is_ws_visible(loop))
			fView->DrawSpace(loop, FALSE);
}

/*---------------------------------------------------------------*/

void TWS::Refresh()
{
	long	loop;

	for (loop = 0; loop < ws_count; loop++)
		fView->DrawSpace(loop, TRUE);
	fView->SelectSpace(fView->fActive);
}

/*===============================================================*/

TWSView::TWSView(rect *r, TWindow *window)
		: TView(r, FOLLOW_BORDERS | FULL_DRAW, vfWillDraw)
{
	fActive = cur_switch;
	fWindow = window;
	as_long(view_color) = TRANSPARENT_RGBA;
}

TWSView::~TWSView() {
}

/*---------------------------------------------------------------*/

void TWSView::HandleEvent(BParcel *event)
{
	if (event->what == B_MOUSE_DOWN) {
		fpoint fp;
		event->FindPoint("be:view_where", &fp);
		button_event info;
		info.h = (int32)fp.h;
		info.v = (int32)fp.v;
		info.buttons = event->FindInt32("buttons");
		click(info);
	};
};

char TWSView::click(button_event start)
{
	bool		first = true;
	bool		moved = false;
	bool		no_window = false;
	bool		tracking_me = false;
	int32		first_clicks;
	long		h, v;
	long		temp_h, temp_v;
	long		width, height;
	long		last_one;
	long		this_one;
	long		orig_space=0;
	long		last_space=0;
	long		orig_x=0, orig_y=0;
	long		win_h, win_v;
	long		i, j, dh, dv, oh, ov;
	ulong		old_ws=0;
	float		scale_x, scale_y;
	point		mouse1;
	point		mouse2;
	point		last_point;
	point		orig_point;
	rect		r, r1, r2;
	rect		orig_bounds;
	TWindow		*window;
	TWindow		*track_window = NULL;
	BParcel		an_event(B_WORKSPACES_CHANGED);

	get_bound(&r);
	get_counts(&h, &v);
	width = r.right / h;
	height = r.bottom / v;
	last_one = fActive;
	last_point.h = last_point.v = -1;
	
	win_h = win_v = 0;		// memory used to check alignment during drag
	
	first_clicks = 2;
	
	window = Owner();
	window->UnlockViewsForRead();

	if (!window->is_focus())
		window->bring_to_front();

	// set the front limit at the workspace window.
	set_front_limit(fWindow);

	while (the_server->test_button() != 0) {
		the_server->get_mouse(&mouse1.h, &mouse1.v);

		owner->LockRegionsForRead();
		owner->global_to_local(&mouse1);
		WindowToLocal(&mouse1);
		owner->UnlockRegionsForRead();

		mouse2 = mouse1;
		if ((mouse1.h <= r.right) && (mouse1.h > (width * h - 1)))
			mouse1.h = width * h - 1;
		if ((mouse1.v <= r.bottom) && (mouse1.v > (height * h - 1)))
			mouse1.v = height * v - 1;
		this_one = (mouse1.v / height) * h + (mouse1.h / width);

		if ((mouse1.v < r.top) || (mouse1.v > r.bottom) ||
			(mouse1.h < r.left) || (mouse1.h > r.right)) {

			// The mouse has gone outside the panel.  If tracking workspaces,
			// snap back to the original workspace.  If tracking a window,
			// return the window to its original position.

			if ((no_window) || (!track_window)) {
				HiliteSpace(last_one, fActive);
				last_one = fActive;
			}
			else if ((orig_space != last_space) ||
					 (orig_point.h != last_point.h) ||
					 (orig_point.v != last_point.v)) {

				owner->LockRegionsForRead();
				get_bounds(track_window, &r1, &r2, last_space);
				owner->UnlockRegionsForRead();

				if (orig_space != last_space)
					track_window->move_between_workspace(last_space, orig_space);
				if (last_space == fActive) {
					wait_regions();
					do_atomic_transaction();
					signal_regions();
				}
				set_bounds(track_window, &orig_bounds, orig_space);
				if (orig_space == fActive)
					track_window->bring_to_front();
				DrawSpace(orig_space, false);
				if (orig_space != last_space)
					DrawSpace(last_space, false);
				last_point = orig_point;
				last_space = orig_space;
				last_one = orig_space;
			}
			win_h = win_v = 0;	// reset the alignment
		}
		else if ((this_one >= 0) && (this_one < ws_count)) {
		
			if (this_one != last_one) {

				// The selected workspace has changed.  If tracking workspaces,
				// de-select the previous one and select the new one.  Else if
				// tracking a window and the window doesn't live in all spaces,
				// remove the window from the previous space and position it in
				// the current space.  If the window already exists in this space,
				// return the window in the previous space to its original position
				// and start tracking the window in the new workspace.

				win_h = win_v = 0;	// resst the alignment
				if ((no_window) || (!track_window)) {
					HiliteSpace(last_one, this_one);
					SelectSpace(fActive);
				}
				else {
					space_rect(fWindow, &r1, this_one, 1);
					scale_x = (float)(r1.right - r1.left) /
									screen_width(this_one);
					scale_y = (float)(r1.bottom - r1.top) /
									screen_height(this_one);
					j = (int32)((float)(mouse2.h - r1.left) / scale_x);
					i = (int32)((float)(mouse2.v - r1.top) / scale_y);
					if (!track_window->is_ws_visible(this_one)) {
						owner->LockRegionsForRead();
							get_bounds(track_window, &r1, &r2, last_space);
						owner->UnlockRegionsForRead();
						temp_h = r1.right - r1.left;
						temp_v = r1.bottom - r1.top;
						r1.top = i - orig_y;
						r1.left = j - orig_x;
						r1.bottom = r1.top + temp_v;
						r1.right = r1.left + temp_h;
//						track_window->set_ws_visible(last_space, false, false);
						track_window->move_between_workspace(last_space, this_one);
						if (last_space == fActive) {
							wait_regions();
							do_atomic_transaction();
							signal_regions();
						}
						set_bounds(track_window, &r1, this_one);
//						track_window->set_ws_visible(this_one, true);
						if (this_one == fActive)
							track_window->bring_to_front();
						DrawSpace(last_space, false);
						DrawSpace(this_one, false);
						last_point = mouse2;
					}
					else {
						set_bounds(track_window, &orig_bounds, orig_space);
						if (orig_space == fActive)
							track_window->bring_to_front();
						DrawSpace(orig_space, false);
						owner->LockRegionsForRead();
							get_bounds(track_window, &orig_bounds, &r2, this_one);
						owner->UnlockRegionsForRead();
						temp_h = orig_bounds.right - orig_bounds.left;
						temp_v = orig_bounds.bottom - orig_bounds.top;
						r1.top = i - orig_y;
						r1.left = j - orig_x;
						r1.bottom = r1.top + temp_v;
						r1.right = r1.left + temp_h;
						if (track_window != fWindow) {
							set_bounds(track_window, &r1, this_one);
							if (this_one == fActive)
								track_window->bring_to_front();
						}
						DrawSpace(this_one, false);
						orig_space = this_one;
						last_point = mouse2;
						space_rect(fWindow, &r1, this_one, 1);
						orig_point.h = r1.left +
							(int32)(((orig_bounds.left + orig_x) * scale_x));
						orig_point.v = r1.top +
							(int32)(((orig_bounds.top + orig_y) * scale_y));
					}
				}
				last_one = this_one;
				last_space = this_one;
			}

			// If we're tracking a window and the mouse has moved, time to
			// move the window.  If we don't know what we are tracking
			// (window or workspace) and the mouse has moved, it's time to
			// find out.

			if ((!no_window) &&
				(((first_clicks <= 0) && ((last_point.h != mouse2.h) || (last_point.v != mouse2.v))) ||
				 (last_point.h < mouse2.h-2) ||
				 (last_point.v < mouse2.v-2) ||
				 (last_point.h > mouse2.h+2) ||
				 (last_point.v > mouse2.v+2))) {
				first_clicks--; 
				space_rect(fWindow, &r1, this_one, 1);
				// Calculate the scaling values.
				scale_x = (float)(r1.right - r1.left) /
								screen_width(this_one);
				scale_y = (float)(r1.bottom - r1.top) /
								screen_height(this_one);
				// Scale the mouse points.
				j = (int32)((float)(mouse2.h - r1.left) / scale_x);
				i = (int32)((float)(mouse2.v - r1.top) / scale_y);

				// A little paranoia here - make sure the window we're tracking still exists
				if (track_window) {
					the_server->LockWindowsForRead();
					window = windowlist;
					if (window) {
						while((window) && (window != track_window))
							window = (TWindow *)window->nextwindow;
						if (track_window != window)
							no_window = true;
					}
					the_server->UnlockWindowsForRead();
				}

				// If we don't have a window, let's see if there is one under
				// the mouse.

				if (!track_window) {
					bool hiliteSpace = false;
					the_server->LockWindowsForRead();
					window = windowlist;
					if (window) {
						while((window) && (!track_window)) {
							if (IsVisible(window))
								if (get_bounds(window, &r1, &r2, this_one)) {
									if (((j >= r1.left) && (j <= r1.right) &&
										(i >= r1.top) && (i <= r1.bottom)) ||
										((j >= r2.left) && (j <= r2.right) &&
										(i >= r2.top) && (i <= r2.bottom))) {
										track_window = window;
										old_ws = track_window->get_ws();
										orig_space = this_one;
										last_space = this_one;
										orig_bounds = r1;
										orig_point = mouse2;
										orig_x = j - r1.left;
										orig_y = i - r1.top;
										hiliteSpace = true;
										if (track_window == fWindow)
											tracking_me = TRUE;
									}
								}
							window = (TWindow *)window->nextwindow;
						}
					}
					win_h = win_v = 0;	// reset the alignment
					the_server->UnlockWindowsForRead();
					if (hiliteSpace) HiliteSpace(this_one, fActive);
				}
				// If we've clicked on a non-dragable window don't track it (unless, wink,
				// wink, you know the special handshake. Do not drag a window screen.
				if ((track_window) && (!no_window)) {
					if (((track_window->flags & NO_MOVE) && (start.buttons != 7)) ||
						(track_window->is_screen())) {
						no_window = true;
						track_window = NULL;
					}
					// If we're tracking the workspace panel and we're on the active workspace
					// don't track it (bad things happen)
					else if ((tracking_me) && (this_one == fActive)) {
						break;
					}
					else {
						if ((last_point.h != -1) && (last_point.v != -1)) {
							i = (int32)((float)(mouse2.h - last_point.h) / scale_x);
							j = (int32)((float)(mouse2.v - last_point.v) / scale_y);
							oh = track_window->aligned_h(0);
							ov = track_window->aligned_v(0);
							dh = track_window->aligned_h(win_h + i + oh) - track_window->aligned_h(win_h + oh);
							dv = track_window->aligned_v(win_v + j + ov) - track_window->aligned_v(win_v + ov);
							win_h += i;
							win_v += j;

							moved = true;
							DrawSpace(last_space, false);
							offset_bounds(track_window, dh, dv, last_space);
						}
					}
				}
				else {
					no_window = TRUE;
					track_window = NULL;
				}
				last_point = mouse2;
			}
			if (first) {
				first = FALSE;
				orig_space = this_one;
			}
		}
		snooze(35000);
	}
	// release the front limit on the workspace window
	set_front_limit(NULL);

	if ((track_window) && (old_ws != track_window->get_ws())) {
		an_event.AddInt64("when",system_time());
		an_event.AddInt32("old",old_ws);
		an_event.AddInt32("new",track_window->get_ws());
		track_window->EventPort()->ReplaceEvent(&an_event,
			&track_window->m_queueIter_ws,
			&track_window->m_wsMsgToken);
		track_window->send_event(&an_event);
	}
	if (!moved)
		switcher(last_one);
	else
		HiliteSpace(last_one, cur_switch);

	Owner()->LockViewsForRead();
	return(TRUE);
}

/*---------------------------------------------------------------*/

void TWSView::Draw(bool inUpdate)
{
	long	loop;

	wait_regions();
	if (inUpdate) BeginUpdate();
	for (loop = 0; loop < ws_count; loop++) {
		DrawSpace(loop, TRUE);
	};
	SelectSpace(fActive);
	if (inUpdate) EndUpdate();
	signal_regions();
}

/*---------------------------------------------------------------*/

bool TWSView::IsVisible(TWindow *window)
{
	if (window->is_desktop() || window->is_float() || window->is_modal())
		return false;
	return true;
}

RenderPort * DirectPortForBitmap(SBitmap *bitmap);

struct WindowRect { rect r; uchar s[30]; float t; };

void TWSView::DrawSpace(long space, bool frame)
{
	uchar		title_name[128];
	long		width, height;
	long		window_count = 0;
	long		loop;
	long		title;
	float		x, y;
	rect		r, r1, r2, r3;
	rgb_color	title_c, frame_c;
	SBitmap		*off_screen;
	TWindow		*window;
	SFont       *vfont;

	wait_regions();

	grSetDrawOp(renderContext,OP_COPY);
	grSetStipplePattern(renderContext,get_pat(BLACK));

	space_rect(fWindow, &r1, space, 0);
	if (space == fActive)
		frame_c = rgb(0, 0, 0);
	else
		frame_c = rgb(255, 255, 255);
	
	if (frame) {
		grSetForeColor(renderContext,frame_c);
		grStrokeIRect(renderContext,r1);
	};

	r1.top += 1;
	r1.left += 1;
	r1.bottom -= 1;
	r1.right -= 1;

	r2.top = 0;
	r2.left = 0;
	r2.bottom = r1.bottom - r1.top;
	r2.right = r1.right - r1.left;
	off_screen = new SBitmap(
		r1.right - r1.left + 1,
		r1.bottom - r1.top + 1,
		ScreenCanvas(0)->pixels.pixelFormat,
		ScreenCanvas(0)->pixels.endianess,
		false, false
	);

	RenderPort *off_port = DirectPortForBitmap(off_screen);
	RenderContext *off_context = grNewRenderContext();
	grInitRenderContext(off_context);
	grSetContextPort_NoLock(off_context,off_port);

	grSetForeColor(off_context,desk_rgb[space]);
	grFillIRect(off_context,r2);
	x = (float)r2.right / screen_width(space);
	y = (float)r2.bottom / screen_height(space);

	BArray<WindowRect> windowRectList(20);

	the_server->LockWindowsForRead();
	window = windowlist;
	if (window) {
		while(window) {
			if (IsVisible(window) && get_bounds(window, &r, &r3, space)) {
				windowRectList.SetItems(windowRectList.CountItems()+1);
				windowRectList[window_count].r = r;
				if ((window->proc_id != 2) && (window->proc_id != 3)) {
					strncpy((char*)windowRectList[window_count].s,window->window_name,29);
					windowRectList[window_count].s[29] = 0;
				} else
					windowRectList[window_count].s[0] = 0;
				windowRectList[window_count].t = max((r.right-r.left)/5,20);
				window_count++;
			};
			window = (TWindow *)window->nextwindow;
		}
		the_server->UnlockWindowsForRead();
		
		title_c = rgb(255, 201, 30);
		frame_c = rgb(128, 128, 128);
		while((--window_count) >= 0) {
			r = windowRectList[window_count].r;
			r.bottom -= r.top;	// height
			r.top = (int32)((float)r.top * y + 0.5);
			r.bottom = r.top + (int32)((float)r.bottom * y + 0.5);
			r.right -= r.left;	// width
			r.left = (int32)((float)r.left * x + 0.5);
			r.right = r.left + (int32)((float)r.right * x + 0.5);
			grSetForeColor(off_context, rgb_white);
			grFillIRect(off_context, r);
			grSetForeColor(off_context, frame_c);
			grStrokeIRect(off_context, r);
			if (windowRectList[window_count].s[0]) {
				width = (r.right - r.left) - 6;
				if (width < 0) width = 0;
				grGetFontPointer(off_context, &vfont);
				vfont->SetSize(max_c(r2.right / 10, 8.0));
				grTruncateString(off_context, width, B_TRUNCATE_END,
					windowRectList[window_count].s, title_name);
				width -= (int)(vfont->StringWidth(title_name)+0.5);
				width = r.left + 3 + width / 2;
				height = (r.bottom - r.top) - 6;

				if (height >= vfont->Ascent()) {
					height = long((height / 2) + (vfont->Ascent() / 2));
					if (height > r.bottom)
						height = r.bottom - 3;
					height = r.top + 3 + height;
					float deltas[2] = {0.0,0.0};
					fpoint fp = {width,height};
					grSetPenLocation(off_context, fp);
					grSetForeColor(off_context, rgb_black);
					grSetBackColor(off_context, rgb_white);
					grDrawString(off_context, title_name, deltas);
					title = (int32)windowRectList[window_count].t;
					loop = (long)(20.0 * y + 0.5);
					if (loop) {
						r.right = (int32)((float)title * x + (float)r.left);
						r.top -= loop;
						r.bottom = r.top + loop - 1;
						grSetForeColor(off_context, title_c);
						grFillIRect(off_context, r);
					}
				};
			};
		}
	} else {
		the_server->UnlockWindowsForRead();
	};

	grDrawLocPixels(renderContext,r1.left,r1.top,&off_screen->Canvas()->pixels);
	grDestroyRenderContext(off_context);
	grFreeRenderContext(off_context);
	grDestroyRenderPort(off_port);
	grFreeRenderPort(off_port);
	delete off_screen;

	signal_regions();
}

/*---------------------------------------------------------------*/

void TWSView::HiliteSpace(long old_space, long new_space)
{
	long		space;
	long		loop;
	rect		r;
	rgb_color	c;

	wait_regions();

	grSetDrawOp(renderContext,OP_COPY);
	grSetStipplePattern(renderContext,get_pat(BLACK));
	for (loop = 0; loop < 2; loop++) {
		if (!loop) {
			space = old_space;
			if (space == fActive)
				c = rgb(0, 0, 0);
			else
				c = rgb(255, 255, 255);
		}
		else {
			space = new_space;
			if (space == fActive)
				c = rgb(0, 0, 0);
			else
				c = rgb(96, 96, 96);
		}
		space_rect(fWindow, &r, space, 0);
		grSetForeColor(renderContext,c);
		grStrokeIRect(renderContext,r);
	}

	signal_regions();
}

/*---------------------------------------------------------------*/

void TWSView::SelectSpace(long space)
{
	long		loop;
	rect		r;
	rgb_color	c;

	wait_regions();

	grSetDrawOp(renderContext,OP_COPY);
	grSetStipplePattern(renderContext,get_pat(BLACK));
	for (loop = (space == fActive); loop < 2; loop++) {
		space_rect(fWindow, &r, fActive, 0);
		if (loop)
			c = rgb(0, 0, 0);
		else {
			c = rgb(255, 255, 255);
			fActive = space;
		}
		grSetForeColor(renderContext,c);
		grStrokeIRect(renderContext,r);
	}

	signal_regions();
}

/*---------------------------------------------------------------*/
/* Return a given window bound for a given space.  If the given
/* space is the active space just return the bounds.  Otherwise
/* look up the bounds in the bounds array.  If the window doesn't
/* exist in the given space or is not visible, return an error.
/*---------------------------------------------------------------*/

bool get_bounds(TWindow* window, rect *r, rect *tab, long space)
{	
	if (window->is_ws_visible(space) && !window->is_hidden()) {
		if ((space != cur_switch) && (window->ws_ext_bounds != 0L))
			*r = window->ws_ext_bounds[space];
		else
			window->get_bound(r);
		*tab = *r;
		if ((window->proc_id != 2) && (window->proc_id != 3)) {
			tab->bottom = tab->top;
			tab->top -= 25;
			tab->right = tab->left + max((tab->right-tab->left)/5,20);
		}
		return true;
	}
	return false;
}


/*---------------------------------------------------------------*/
/* Set the new bounds of a window in a given workspace
/*---------------------------------------------------------------*/

void set_bounds(TWindow *track_window, rect *bound, long space)
{
	if ((track_window->ws_ext_bounds != 0) && (space != cur_switch))
		track_window->ws_ext_bounds[space] = *bound;
	else
		track_window->moveto_w(bound->left, bound->top);
}

/*---------------------------------------------------------------*/
/* Set the new bounds of a window in a given workspace
/*---------------------------------------------------------------*/

void offset_bounds(TWindow *track_window, long dh, long dv, long space)
{
	if ((track_window->ws_ext_bounds != 0) && (space != cur_switch)) {
		track_window->ws_ext_bounds[space].top += dv;
		track_window->ws_ext_bounds[space].left += dh;
		track_window->ws_ext_bounds[space].right += dh;
		track_window->ws_ext_bounds[space].bottom += dv;
	}
	else
		track_window->move(dh, dv, 0);
}

/*---------------------------------------------------------------*/
/* Return an h and v pair for the defined number of workspaces.
/*---------------------------------------------------------------*/

void get_counts(long *h, long *v)
{
	switch (ws_count) {
		case 1:
			*h = 1; *v = 1; break;
		case 2:
			*h = 2; *v = 1; break;
		case 3:
			*h = 3; *v = 1; break;
		case 4:
			*h = 2; *v = 2; break;
		case 5:
			*h = 5; *v = 1; break;
		case 6:
			*h = 3; *v = 2; break;
		case 7:
			*h = 7; *v = 1; break;
		case 8:
			*h = 4; *v = 2; break;
		case 9:
			*h = 3; *v = 3; break;
		case 10:
			*h = 5; *v = 2; break;
		case 11:
			*h = 11; *v = 1; break;
		case 12:
			*h = 4; *v = 3; break;
		case 13:
			*h = 13; *v = 1; break;
		case 14:
			*h = 7; *v = 2; break;
		case 15:
			*h = 5; *v = 3; break;
		case 16:
			*h = 4; *v = 4; break;
		case 17:
			*h = 17; *v = 1; break;
		case 18:
			*h = 6; *v = 3; break;
		case 19:
			*h = 19; *v = 1; break;
		case 20:
			*h = 5; *v = 4; break;
		case 21:
			*h = 7; *v = 3; break;
		case 22:
			*h = 11; *v = 2; break;
		case 23:
			*h = 23; *v = 1; break;
		case 24:
			*h = 6; *v = 4; break;
		case 25:
			*h = 5; *v = 5; break;
		case 26:
			*h = 13; *v = 2; break;
		case 27:
			*h = 9; *v = 3; break;
		case 28:
			*h = 7; *v = 4; break;
		case 29:
			*h = 29; *v = 1; break;
		case 30:
			*h = 6; *v = 5; break;
		case 31:
			*h = 31; *v = 1; break;
		case 32:
			*h = 8; *v = 4; break;
	}
}

/*---------------------------------------------------------------*/
/* Return the workspace rect within the workspace preference
/* panel window for a given workspace.
/*---------------------------------------------------------------*/

void space_rect(TWindow* window, rect* r, long space, long inset)
{
	long	i, j, h, v;
	long	width, height;
	rect	bound;

	window->get_bound(&bound);
	get_counts(&h, &v);
	i = space / h;
	j = space % h;
	width = (bound.right - bound.left) / h;
	height = (bound.bottom - bound.top) / v;
	r->top = i * height + inset;
	r->left = j * width + inset;

	// workspaces on the right or bottom edge actually go all the way to
	// the edge.
	if (i != (v - 1))
		r->bottom = (i + 1) * height - inset;
	else
		r->bottom = (bound.bottom - bound.top) - inset;
	if (j != (h - 1))
		r->right = (j + 1) * width - inset;
	else
		r->right = (bound.right - bound.left) - inset;
}

/*---------------------------------------------------------------*/
/* Return the width of the screen for the workspaces resolution.
/*---------------------------------------------------------------*/

float screen_width(long space)
{
	return ws_res[space].virtual_width;
}

/*---------------------------------------------------------------*/
/* Return the height of the screen for the workspaces resolution.
/*---------------------------------------------------------------*/

float screen_height(long space)
{
	return ws_res[space].virtual_height;
}
