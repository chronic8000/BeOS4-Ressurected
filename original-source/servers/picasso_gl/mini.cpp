
#include <stdio.h>
#include <stdlib.h>

#include <support2/ByteOrder.h>		// must be included first
#include <support2/Looper.h>
#include <support2/Locker.h>
#include <support2/StdIO.h>
#include <support2/Team.h>
#include <raster2/RasterRect.h>

#include <DirectGLWindow.h>
#include <MessageFilter.h>
#include <SupportDefs.h>
#include <View.h>

#include "enums.h"
#include "input.h"
#include "mini.h"
#include "server.h"

#include <render2/Point.h>
#include <render2/Rect.h>
#include <raster2/RasterRect.h>
#include <support2/StdIO.h>

#include "parcel.h"
#include "old/Message.h"

using B::Support2::BValue;
using B::Support2::bout;
using B::Support2::endl;
using B::Raster2::BRasterRect;

// ---------------------------------------------------

void BParcel::SetFromReallyOldMessage(const void *ptr)
{
	const ::BMessage& msg = *((::BMessage*)ptr);
	B::Old::BMessage m;
	char *buffer = (char *)malloc(msg.FlattenedSize());
	msg.Flatten(buffer, msg.FlattenedSize());
	m.Unflatten(buffer);
	free((void *)buffer);
	SetData(m.AsValue()["b:data"]);
	SetWhat(m.what);
	SetWhen(m.FindInt64("when"));
}

// ---------------------------------------------------


inline BRasterRect crect2rect(const clipping_rect r)
{
	BRasterRect rr;
	rr.top = r.top;
	rr.left = r.left;
	// convert to right-side-exclusive coordinates.
	rr.right = r.right+1;
	rr.bottom = r.bottom+1;
	return rr;
};


class AppServerView : public BView, public BMessageFilter
{
	public:

	B::Support2::atom_ref<B::Support2::BTeam>	team;
	bool initted;
	
	AppServerView(BRect r)
		:	BView(r,"top",B_FOLLOW_ALL,B_WILL_DRAW|B_NAVIGABLE),
			BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE), team(B::Support2::BLooper::Team()), initted(false) {};
	void AttachedToWindow() {
		Window()->AddCommonFilter(this);
	}
	void DetachedFromWindow() {
		Window()->RemoveCommonFilter(this);
	}
	void MouseDown(BPoint) {
		SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
		MessageReceived(Window()->CurrentMessage());
	};
	void MouseMoved(BPoint, uint32, const BMessage *) {
		MessageReceived(Window()->CurrentMessage());
	};
	void MouseUp(BPoint) {
		MessageReceived(Window()->CurrentMessage());
	};
	void KeyDown(const char *, int32) {
		MessageReceived(Window()->CurrentMessage());
	};
	void KeyUp(const char *, int32) {
		MessageReceived(Window()->CurrentMessage());
	};
	filter_result Filter(BMessage *message, BHandler **) {
		if (message->what == B_KEY_DOWN || message->what == B_KEY_UP) {
			MessageReceived(message);
			return B_SKIP_MESSAGE;
		}
		return B_DISPATCH_MESSAGE;
	}
	void MessageReceived(BMessage *msg) {

		if (!initted) {
			B::Support2::BLooper::InitOther(team.promote());
			initted = true;
		}

		if (gInputServerLink == NULL) {
			// It ispossible to receive event too early
			// (before the input_server link is actually operationnal)
			BView::MessageReceived(msg);
			return;
		}
		
		switch (msg->what) {
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_MOVED:
			case B_KEY_UP:
			case B_KEY_DOWN:
			case B_MODIFIERS_CHANGED: {
#if ROTATE_DISPLAY
				// MATHIAS: shouldn't we use the rotater here?
				BPoint bpt = msg->FindPoint("where");
				BPoint pp;
				pp.x = bpt.y;
				pp.y = Bounds().right-bpt.x;
				msg->ReplacePoint("where", pp);
#endif
				// convert the ::BMessage to BParcel
				BParcel *parcel = new BParcel;
				parcel->SetFromReallyOldMessage(msg);

				// Get the corresponding BValue, modify and write it
				BValue v = parcel->Data();
				v.RemoveItem("screen_where");
				v.RemoveItem("be:view_where");
				v.RemoveItem("be:transit");
				parcel->SetData(v);

				// Send the message
				gInputServerLink->enqueue_message(parcel);
			} break;
			default: {
				BView::MessageReceived(msg);
			} break;
		}
	}
};

extern void inval(BSurface *surface, B::Raster2::BRasterRegion &region);
extern void lock_context(BSurface *surface);
extern void unlock_context(BSurface *surface);

class AppServerDirectGLWindow : public BDirectGLWindow
{
	public:
	
	sem_id				sem;
	bool				hasConnected;
	bool				initted;
	B::Render2::BRect			window_bound;
	B::Raster2::BRasterRegion	theRegion;
	B::Support2::BLocker		lock;
	uint32				token;
	BSurface *			surface;
	B::Support2::atom_ref<B::Support2::BTeam>	team;

	AppServerDirectGLWindow(B::Render2::BRect r, const char *name)
		: BDirectGLWindow(BRect(r.left, r.top, r.right-1, r.bottom-1), name,B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,0)
	{
		token = 2;
		surface = NULL;
		hasConnected = false;
		initted = false;
		team = B::Support2::BLooper::Team();
		window_bound.top = window_bound.left = 0;
		window_bound.right = window_bound.bottom = -1;
		r.OffsetTo(0,0);
		BView *v = new AppServerView(Bounds());
		v->SetViewColor(B_TRANSPARENT_COLOR);
		AddChild(v);
		v->MakeFocus(true);
		sem = create_sem(0,"ddsem");
	};
	~AppServerDirectGLWindow() {
	};
	bool QuitRequested() {
		return true; //SApp::ShutdownClients();
	};
	
	void DirectConnected(direct_buffer_info *info) {		
		if (!initted) {
			B::Support2::BLooper::InitOther(team.promote());
			initted = true;
		}

		B::Raster2::BRasterRegion oldRegion;
		bool wasConnected = hasConnected;

		switch(info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_STOP:
			if (wasConnected) lock_context(surface);
			break;
		case B_DIRECT_MODIFY:
			if (wasConnected) lock_context(surface);
		case B_DIRECT_START:
			if (wasConnected) {
				oldRegion = theRegion;
			}

			theRegion.MakeEmpty();
			for (uint32 i=0 ; i<info->clip_list_count ; i++) {
				theRegion.OrRect(crect2rect(info->clip_list[i]));
			}
			theRegion.OffsetBy(-info->window_bounds.left, -info->window_bounds.top);
			window_bound = crect2rect(info->window_bounds);

			if (!hasConnected) {
//				release_sem(sem);
				hasConnected = true;
			}
			if (wasConnected) {
				token++;
				B::Raster2::BRasterRegion updateRegion = theRegion - oldRegion;
				unlock_context(surface);
				if ((surface != NULL) && !updateRegion.IsEmpty()) {				
					inval(surface, updateRegion);
				}
			}
			break;
		}

		BDirectGLWindow::DirectConnected( info );
	}
};

AppServerWindow::AppServerWindow(B::Render2::BRect r, const char *name) {
	m_as_window = new AppServerDirectGLWindow(r, name);
}
AppServerWindow::~AppServerWindow() {
	delete m_as_window;
}

lock_status_t	AppServerWindow::Lock() 		{ return m_as_window->lock.Lock(); }
void			AppServerWindow::Unlock() 		{ m_as_window->lock.Unlock(); }
void			AppServerWindow::oglLock() 		{ m_as_window->MakeCurrent(); }
void			AppServerWindow::oglUnlock() 	{ m_as_window->ReleaseCurrent(); }
bool			AppServerWindow::oglIsLocked()	{ return m_as_window->IsCurrent(); }


uint32						AppServerWindow::Token()		{ return m_as_window->token; }
void						AppServerWindow::Show() 		{ m_as_window->Show(); }
void						AppServerWindow::Close() 		{ Lock(); m_as_window->Close(); }
B::Render2::BRect&			AppServerWindow::WindowBounds()	{ return m_as_window->window_bound; }
B::Raster2::BRasterRegion&	AppServerWindow::Region()		{ return m_as_window->theRegion; }
sem_id&						AppServerWindow::Sem()			{ return m_as_window->sem; }
void 						AppServerWindow::SetSurface(BSurface *surface) { m_as_window->surface = surface; }

status_t AppServerWindow::InitializeGL( uint32 id, uint32 color, uint32 depth, uint32 stencil, uint32 accum )
{
	return m_as_window->InitializeGL( id, color, depth, stencil, accum );
}

status_t AppServerWindow::ShutdownGL()
{
	m_as_window->ShutdownGL();
	return B_OK;
}


#ifdef MINI_SERVER
#	include <Application.h>
#endif

void create_bapp()
{
#ifdef MINI_SERVER
	new BApplication("application/x-vnd.Be-app_server-picasso");
#endif
}
