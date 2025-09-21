//******************************************************************************
//
//	File:			server.cpp
//	Description:	The main server class
//	
//	Written by:		Benoit Schillings
//
//	Copyright 1992-1998, Be Incorporated
//
//******************************************************************************/

#ifndef	SERVER_H
#define	SERVER_H

#include <time.h> 
#include <math.h> 

#include <Accelerant.h>
#include "heap.h"

class TSession;
class TWindow;
class TView;
class BParcel;
class TCursor;
class TokenSpace;
struct vague_display_mode;

struct event_pipe {
	void *	routeTo;
	uint32	eventTypes;
};

#define LOCK_STATE_NONE  0
#define LOCK_STATE_READ  1
#define LOCK_STATE_WRITE 2

class WMInteraction;
class BGraphicDevice;

class TServer {

	public:

				int32			m_forceMove;
				int32			m_checkFocus;
				TWindow *		m_lastClickedWindow;	// 16/1/2001 - Mathias: Filter out wrong double clicks

				Benaphore		m_mouseFilterSync;
				Benaphore		m_mouseFilterLock;
				fpoint			m_last_curpos;
				fpoint			m_target_curpos;
				bigtime_t		m_when_pos;
				sem_id			m_sem_retrace;
				bigtime_t		m_cpu_update_fps;
				bigtime_t 		ScreenRefreshPeriod() { return m_cpu_update_fps; }
				void			update_mode_info(BGraphicDevice *device);
				float			mouse_filter(const float& t);

				long			curs_h;
				long			curs_v;
				char			button;
				bigtime_t		last_user_action;
				char			quit;
				long			fAppKitPort;
				long			fInputServerPort;
				BParcel			*the_tbrol;
				long			tbrol_dh;
				long			tbrol_dv;
				ulong			last_modifiers;
				char			target_mouse;
				char			no_zoom;
		
								TServer();
				void			run();

				void			get_mouse(long *h, long *v);
				char			test_button();
				void			user_action();
				bigtime_t		update_busy();
				int32			check_update();
				void			new_session(message *a_message);
				void			set_drag_stuff(TSession *a_session);
				void			set_drag_bm(TSession *a_session);
				void			set_drag_pixel(TSession *a_session);
				void			focus_follows_mouse(TWindow *target = (TWindow*)-1, int32 lockState=LOCK_STATE_NONE);
				bool			do_workspace_switch(uchar);
				void			force_mouse_moved();
		
				int32			InputThread();
				int32			MouseInView(BParcel *event,	TView *view, point localCoords);

				TWindow *		MouseMoved(BParcel *event, TWindow **otherTarget);
				TWindow *		MouseUp(BParcel *event);
				TWindow *		MouseDown(BParcel *event);
				TWindow *		KeyDown(BParcel *event);
				TWindow *		FocusedEvent(BParcel *event);
				
				point			GetMouse();
				
		inline	WMInteraction *	WindowManager();
		
		static	int32			MouseDownThreadLaunch(TServer *server);
				int32			MouseDownThread();

		static	int32			AppKitThreadLaunch(TServer *server);
				int32			AppKitThread();

		static	int32			CursorThreadLaunch(TServer *server);
				int32			CursorThread();

		static	int32			CursorThreadMoveLaunch(TServer *server);
				int32			CursorThreadMove();

		static	int32			WindowManagerThreadLaunch(TServer *server);

		static	int32			UpdaterThreadLaunch(TServer *server);
				int32			UpdaterThread();

		static	int32			PsychoEraserThreadLaunch(TServer *server);
				int32			PsychoEraserThread();

		static	int32			picassoLaunch(TServer *server);
				int32			picasso();

#ifdef REMOTE_DISPLAY
		static	int32			RemoteListenerLaunch(TServer *server);
				int32			RemoteListener();
#endif
		
				void			LockWindowsForRead();
				void			UnlockWindowsForRead();
				void			LockWindowsForWrite();
				void			UnlockWindowsForWrite();

				void			LockWindowsForRead(int32 pid);
				void			UnlockWindowsForRead(int32 pid);
				void			LockWindowsForWrite(int32 pid);

				void			DowngradeWindowsWriteToRead();

				void			WindowNeedsUpdate();
				void			WindowEventMaskChanged(TWindow *w, uint32 oldEventMask);
				void			WindowCloseNotify(TWindow *w);

	static		sem_id			m_cursorSem;
	static		area_id			m_cursorArea;
	static		int32 *			m_cursorAtom;

	private:

				void			RecalcEventMaskAndRemove(TWindow *w);
				
				point			m_mouseLocation;
				TWindow *		m_mouseWindow;
				TWindow *		m_eventChain;
				uint32			m_eventMask;
				sem_id			m_psychoEraserSem;
				Heap 			m_heap;
				WMInteraction * m_wm;
};

WMInteraction * TServer::WindowManager()
{
	return m_wm;
}

extern	TServer		*the_server;
extern	TokenSpace	*tokens;
extern 	TCursor		*the_cursor;

extern	status_t	SetScreenMode(int32					screen,
								  vague_display_mode	*vmode,
								  display_mode			*dmode,
								  int32					stick,
								  bool					send_msg = true);

extern	BMessage*	LockUISettings();
enum {
	// This flag is in addition to those for the
	// public update_ui_settings API.
	B_CHANGED_UI_SETTINGS = 0x10000000
};
extern	status_t	UnlockUISettings(uint32 flags=0);

#endif
