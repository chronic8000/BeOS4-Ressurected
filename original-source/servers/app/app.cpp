
//******************************************************************************
//
//	File:			app.cpp
//	Description:	Class to encapsulate per-client information
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "app.h"
#include "session.h"
#include "bitmap.h"
#include "bitmapcollection.h"
#include "token.h"
#include "mwindow.h"
#include "off.h"
#include "cursor.h"
#include "server.h"
#include "as_debug.h"
#include "eventport.h"
#include "TCPIPSocket.h"

#include <AppDefs.h>
#include <Debug.h>

extern TWindow *front_limit;    
extern "C" long cur_switch;
extern bool IsValidGraphicsType(uint32 type);
extern bool IsValidGraphicsType(uint32 type, bool willDraw);
extern void GraphicsTypeToInternal(uint32 type, int32 *pixelFormat, uint8 *endianess);
extern bool bitmaps_support_space(color_space space, uint32 * out_support_flags);

Benaphore	SApp::m_appListLock("appListLock");
SApp *		SApp::m_firstApp = NULL;
sem_id		SApp::m_shutdownSem = -1;

SApp::SApp(TSession *session, int32 teamID, port_id eventPort, uint32 hostDesig)
{
	team_info ti;
	char RWstr[B_OS_NAME_LENGTH+64],
		 ROstr[B_OS_NAME_LENGTH+64],
		 threadName[B_OS_NAME_LENGTH+64];

	m_session = session;
	m_teamID = teamID;
	m_flags = 0;
	m_icon = NULL;
	m_eventSocket = NULL;
	m_eventPort = eventPort;
	m_host = hostDesig;
	m_eventPipes = NULL;
	m_eventPiperPort = -1;
	m_eventPiper = -1;
	m_cursor = NULL;
	
	if (get_team_info(teamID,&ti) == B_OK) {
		char *p = ti.args,*slash=ti.args;
		while ((*p != 0) && (*p != ' ')) {
			if (*p == '/') slash = p+1;
			p++;
		};
		*p = 0;
		sprintf(RWstr,"%s:%ld:RWHeap",slash,teamID);
		sprintf(ROstr,"%s:%ld:ROHeap",slash,teamID);
		sprintf(threadName,"a:%.24s:%ld",slash,teamID);
	} else {
		strcpy(RWstr,"(unknown):RWHeap");
		strcpy(ROstr,"(unknown):ROHeap");
		strcpy(threadName,"a:(unknown)");
	};
	m_RWHeap = new HeapArea(RWstr,4096,8);
	m_RWHeap->ServerLock();
	m_ROHeap = new HeapArea(ROstr,4096,8);
	m_ROHeap->ServerLock();
	
	LockAppList();
	m_next = m_firstApp;
	m_firstApp = this;
	UnlockAppList();
	
	m_appTask = xspawn_thread(
		(thread_entry)SApp::LaunchAppTask, threadName,
		B_DISPLAY_PRIORITY, this);
};

SApp::~SApp()
{
	int32 dummy;
	if (m_eventPiperPort > 0) delete_port(m_eventPiperPort);
	if (m_eventPiper > 0) wait_for_thread(m_eventPiper,&dummy);
	LockAppList();
	SApp **p = &m_firstApp;
	while (*p && (*p != this)) p = &(*p)->m_next;
	if (!(*p)) {
		DebugPrintf(("App 0x%08x was never added to the app list!\n",this));
		return;
	};
	*p = (*p)->m_next;
	if (!m_firstApp && (m_shutdownSem!=-1)) release_sem(m_shutdownSem);
	m_next = NULL;
	UnlockAppList();
	m_RWHeap->ServerUnlock();
	m_ROHeap->ServerUnlock();
	if (m_icon) m_icon->ServerUnlock();
};

status_t SApp::SendEvent(BParcel *event, bigtime_t timeout, int32 clientToken)
{
	status_t	err;
	
#ifdef REMOTE_DISPLAY
	if (m_eventSocket) {
		char *		flattened=NULL;
		int32		size;
	
		event->Flatten(&flattened, &size);
	
		m_eventSocketLock.Lock();
		err = m_eventSocket->Send(&clientToken,4);
		err = m_eventSocket->Send(&size,4);
		err = m_eventSocket->Send(flattened,size);
		m_eventSocketLock.Unlock();
		grFree(flattened);
		return err;
	};
#endif

	/* 'pjpp' is the port message code used to designate a normal, run-of-the-mill BMessage */
	err = event->WritePort(m_eventPort, 'pjpp', B_TIMEOUT, timeout);
	return err;

};

void SApp::MouseUp(BParcel *msg, fpoint location, area_id droppedArea)
{
	if ((droppedArea != B_BAD_VALUE) && (Flags() & AFLAG_DISPOSE_DRAG)) {
		BParcel event(_DISPOSE_DRAG_);
		event.AddInt32("_msg_data_",droppedArea);
		event.AddInt64("when",msg->FindInt64("when"));
		if (SendEvent(&event) == B_OK)
			UnsetFlag(AFLAG_DISPOSE_DRAG);
	};
};

void SApp::ShowCursor()
{
	UnsetFlag(AFLAG_CURSOR_HIDDEN);
	the_cursor->Show();
}

void SApp::HideCursor()
{
	SetFlag(AFLAG_CURSOR_HIDDEN);
	the_cursor->Hide();
}

void SApp::ObscureCursor()
{
	SetFlag(AFLAG_CURSOR_OBSCURED);
	the_cursor->Obscure();
}

bool SApp::CursorIsHidden()
{
	return Flags() & AFLAG_CURSOR_HIDDEN;
}

SCursor * SApp::Cursor()
{
	return m_cursor;
}

status_t SApp::SetCursor()
{
	SCursor *	curs;
	int32		result;
	int32		cursorToken;

	if (m_session->sread(4,&cursorToken) < 0) return B_PROTOCOL;
	curs = SCursor::FindCursor(cursorToken);
	if (curs) {
		the_cursor->SetAppCursor(this,curs);
		curs->ServerUnlock();
	} else {
		printf("Bad cursor token on set\n");
	};

	return B_OK;
};

void SApp::SetCursor(SCursor *cursor)
{
	if (cursor) cursor->ServerLock();
	if (m_cursor) m_cursor->ServerUnlock();
	m_cursor = cursor;
}

status_t SApp::NewCursor()
{
	int32 server_token = NO_TOKEN;

	int32 cSize = 0;
	if (m_session->sread(sizeof(int32), &cSize) < 0) return B_PROTOCOL;
	if(!cSize) return B_ERROR;

	uint8 *cData = new uint8 [cSize];
	if (m_session->sread(cSize, cData) < 0) return B_PROTOCOL;
	SCursor *cursor = new SCursor(cData);
	delete [] cData;

	if (cursor != NULL) {
		server_token = tokens->new_token(m_teamID,TT_CURSOR|TT_ATOM,cursor);
		if (server_token != NO_TOKEN) {
			cursor->ClientLock();
			cursor->SetToken(server_token);
		} else {
			delete cursor;
		};
	};

	m_session->swrite_l(server_token);
	m_session->sync();
	return B_OK;
}

status_t SApp::DeleteCursor()
{
	SCursor *	p;
	int32		result;
	int32		cursorToken;

	if (m_session->sread(4,&cursorToken) < 0) return B_PROTOCOL;
	result = tokens->grab_atom(cursorToken,(SAtom**)&p);
	if (result >= 0) {
		p->SetToken(NO_TOKEN);
		tokens->delete_atom(cursorToken,p);
	} else {
		DebugPrintf(("Bad cursor token on delete\n"));
	};

	return B_OK;
}

status_t SApp::NewBitmap()
{
	int32	server_token = NO_TOKEN;
	uint8	data_type = 0;
	uint32	data = NONZERO_NULL;
	int32	colorSpace;
	uint8	endianess;
	int32	bytesPerRow=0;

	struct {
		rect	bound;
		uint32	type;
		uint32	flags;
		int32	rowBytes;
	} bitmap_data;

	if (m_session->sread(sizeof(bitmap_data), &bitmap_data) < 0) return B_PROTOCOL;

	if (bitmap_data.type == B_GRAY8) bitmap_data.type = B_CMAP8;
	
	SBitmap *bitmap = NULL;
	int32 	y=bitmap_data.bound.bottom-bitmap_data.bound.top+1,
			x=bitmap_data.bound.right-bitmap_data.bound.left+1;
	if ((x<=0) || (y<=0) || (((float)x*(float)y)>2e9) ||
		(!IsValidGraphicsType(bitmap_data.type,false))) {
		DebugPrintf(("new_bitmap: Bad value(s)!\n"));
	} else {
		bitmap = new SBitmap(x,y,bitmap_data.type,true,bitmap_data.flags,bitmap_data.rowBytes,m_RWHeap,this);
		if (bitmap->IsValid()) {
			bytesPerRow = bitmap->Canvas()->pixels.bytesPerRow;
			server_token = tokens->new_token(m_teamID,TT_BITMAP|TT_ATOM,bitmap);
			bitmap->ClientLock();
			bitmap->SetToken(server_token);
			if (m_host == LOCALHOST) {
				if (bitmap->IsOverlay()) {
					data = (uint32)bitmap->OverlayFor()->SharedData()->ID();
					data_type = 1;
				} else if (bitmap->AreaID() > 0) {
					data = (uint32)bitmap->AreaID();
					data_type = 1;
				} else {
					data = bitmap->ServerHeapOffset();
					data_type = 0;
				};
			} else {
				data = NONZERO_NULL;
			};
		} else {
			delete bitmap;
		};
	}
	
	m_session->swrite_l(server_token);
	m_session->swrite(1, &data_type);
	m_session->swrite_l(data);
	m_session->swrite_l(bytesPerRow);
	m_session->sync();
	
	return B_OK;
}

status_t SApp::DeleteBitmap()
{
	SBitmap *	p;
	int32		result;
	int32		bitmapToken;

	if (m_session->sread(4,&bitmapToken) < 0) return B_PROTOCOL;
	result = tokens->grab_atom(bitmapToken,(SAtom**)&p);
	if (result >= 0) {
		p->SetToken(NO_TOKEN);
		tokens->delete_atom(bitmapToken,p);
	} else {
		_sPrintf("Bad bitmap token on delete\n");
	};

	return B_OK;
}

// --------------------------------------------------

status_t SApp::NewLBX()
{
	int32 lbx_size;

	// Read the parameters (lbx size)
	if (m_session->sread(sizeof(int32), &lbx_size) < 0)
		return B_PROTOCOL;

	// Create the LBX container object
	int32 server_token = 0xFFFFFFFF;
	area_id area = 0xFFFFFFFF;
	SBitmapCollection *lbx = new SBitmapCollection(lbx_size);
	if (lbx->IsValid())
	{ // Then create a token for it
		server_token = tokens->new_token(TeamID(), TT_LBX | TT_ATOM, lbx);
		lbx->ClientLock();
		area = lbx->AreaID();
	}

	// Reply to the application (token, area_id)
	m_session->swrite(4, &server_token);
	m_session->swrite(4, &area);
	m_session->sync();
	return B_OK;
}

status_t SApp::DeleteLBX()
{
	SBitmapCollection *lbx;
	int32 lbx_token;
	
	// Get the token from the session
	if (m_session->sread(sizeof(int32), &lbx_token) < 0)
		return B_PROTOCOL;

	// Get the SBitmapCollection object from the token and uncount it
	int32 result = tokens->grab_atom(lbx_token, (SAtom**)&lbx);
	if (result >= 0)
		tokens->delete_atom(lbx_token, lbx);
	return B_OK;
}

// --------------------------------------------------

status_t SApp::NewWindow(TSession *a_session)
{
	MWindow		*the_window;
	int32		namelen;
	char 		windowName[256];
	struct {
		rect		r1;
		uint32		design;
		uint32		behavior;
		uint32		flags;
		int32		client_token;
		port_id		send_port;
		port_id		inter_port;
		uint32		ws;
		int32		nameLen;
	} window_data;

	if (!a_session) a_session = m_session;
	if (a_session->sread(sizeof(window_data), &window_data) < 0) return B_PROTOCOL;
	if ((window_data.nameLen <= 0) || (window_data.nameLen > 65536)) return B_PROTOCOL;
	namelen = (window_data.nameLen<256)?window_data.nameLen:256;
	if (a_session->sread(namelen, windowName) < 0) return B_PROTOCOL;
	window_data.nameLen -= namelen;
	if (window_data.nameLen && (a_session->drain(window_data.nameLen) < 0)) return B_PROTOCOL;
	windowName[255] = 0;

	the_window = new MWindow(
		this, windowName, &window_data.r1, window_data.design, window_data.behavior,
		window_data.flags, window_data.ws, m_teamID,
		window_data.inter_port, window_data.client_token,
		(a_session!=m_session)?a_session:NULL);

	the_window->server_token = tokens->new_token(m_teamID, TT_WINDOW|TT_ATOM, the_window);
	if (a_session == m_session) the_window->send_port = window_data.send_port;
	
	if ((front_limit != 0) && (front_limit->team_id == m_teamID)) set_front_limit(NULL);

	if (the_window->is_team_modal() && (the_window->ws_subwindows() == 0))
		the_window->first_ws = cur_switch;
	
	a_session->swrite(sizeof(rect),the_window->ClientBound());
	a_session->swrite_l(the_window->server_token);
	a_session->swrite_l(the_window->fport_id);
	a_session->swrite_l(the_window->top_view->server_token);
	a_session->swrite_l(the_window->EventPort()->ClientBlock());
	a_session->swrite_l(the_window->EventPort()->FirstEvent());
	a_session->swrite_l(the_window->EventPort()->AtomicVar());
	a_session->sync();
	
	resume_thread(the_window->task_id);

	return B_OK;
};

status_t SApp::NewOffscreenWindow(TSession *a_session)
{
	TOff		*the_window=NULL;
	char		windowName[256];
	int32		h,w,namelen;
	int32		rtoken = NO_TOKEN;
	uint32		supported;
	port_id		rport = B_BAD_PORT_ID;
	
	struct {
		rect		r1;
		int32		type;
		uint32		flags;
		uint32		bitmapFlags;
		int32		rowBytes;
		int32		client_token;
		port_id		send_port;
		port_id		inter_port;
		int32		nameLen;
	} off_data;

	if (!a_session) a_session = m_session;

	if (a_session->sread(sizeof(off_data), &off_data) < 0) return B_PROTOCOL;
	if ((off_data.nameLen < 0) || (off_data.nameLen > 65536)) return B_PROTOCOL;
	namelen = (off_data.nameLen<256)?off_data.nameLen:256;
	if (a_session->sread(namelen, windowName) < 0) return B_PROTOCOL;
	off_data.nameLen -= namelen;
	if (off_data.nameLen && (a_session->drain(off_data.nameLen) < 0)) return B_PROTOCOL;
	windowName[255] = 0;
	
	if (off_data.type == B_GRAY8) off_data.type = B_CMAP8;
	if (IsValidGraphicsType(off_data.type)) {
		h = off_data.r1.bottom - off_data.r1.top + 1;
		w = off_data.r1.right - off_data.r1.left + 1;
		bitmaps_support_space((color_space)off_data.type,&supported);
		if (supported & B_BITMAPS_SUPPORT_ATTACHED_VIEWS) {

			the_window = new TOff(this, windowName, w, h, off_data.type, off_data.flags,
				off_data.bitmapFlags, off_data.rowBytes, m_RWHeap, (a_session!=m_session)?a_session:NULL);

			if (the_window->task_id < B_OK) { // something terrible has just happened. We ran out of threads!
				goto error_no_thread;
			} else {
				if (the_window->Bitmap()->IsValid()) {
					the_window->server_token = tokens->new_token(m_teamID, TT_WINDOW|TT_OFFSCREEN|TT_ATOM, the_window);
					the_window->team_id = m_teamID;
					the_window->client_token = off_data.client_token;
					if (a_session == m_session) {
						the_window->send_port = off_data.send_port;
						the_window->inter_port = off_data.inter_port;
					}
					rtoken = the_window->server_token;
					rport = the_window->fport_id;
				} else {
					// Here it is safe to close the window, because
					// there are 2 references on it around (for the thread)
					the_window->do_close();
				}
			}
		}
	}

	a_session->swrite_l(rtoken);
	a_session->swrite_l(rport);
	a_session->sync();
	if (the_window) resume_thread(the_window->task_id);
	return B_OK;
	
error_no_thread:
	// The atom was taken twice for the window-thread itself.
	// However this thread is not running, so unlock the window.
	the_window->ServerUnlock();
	the_window->ServerUnlock();
	// We must write to the session before destroying the window
	// because the window dtor _will_ delete the session!
	a_session->swrite_l(rtoken);
	a_session->swrite_l(rport);
	a_session->sync();

	// And finaly close the window, which will close the session.
	the_window->do_close();
	return B_OK;
}

void SApp::GetOverlayRestrictions()
{
	SAtom *atom;
	SBitmap *bitmap;
	int32 token,result,err=B_BAD_VALUE;
	overlay_constraints c;
	overlay_restrictions r;
	memset(&r,0,sizeof(overlay_restrictions));

	m_session->sread(sizeof(int32),&token);

	result = tokens->grab_atom(token, &atom);
	if (result >= 0) {
		bitmap = dynamic_cast<SBitmap*>(atom);
		if (!bitmap) {
			TOff *off = dynamic_cast<TOff*>(atom);
			bitmap = off->Bitmap();
			bitmap->ServerLock();
			off->ServerUnlock();
		};

		if (bitmap->IsOverlay()) {
			bitmap->OverlayFor()->GetConstraints(&c);
			memcpy(&r.source,&c.view,sizeof(overlay_limits));
			memcpy(&r.destination,&c.window,sizeof(overlay_limits));
			r.min_width_scale = c.h_scale.min;
			r.max_width_scale = c.h_scale.max;
			r.min_height_scale = c.v_scale.min;
			r.max_height_scale = c.v_scale.max;
			err = B_OK;
		};

		bitmap->ServerUnlock();
	};

	m_session->swrite(sizeof(int32),&err);
	m_session->swrite(sizeof(overlay_restrictions),&r);
	m_session->sync();
};

int32 SApp::LaunchAppTask(SApp *app)
{
	return app->AppTask();
};

SApp * SApp::LookupAppByTeamID(int32 teamID, uint32 host)
{
	LockAppList();
	SApp *a = m_firstApp;
	while (a) {
		if ((a->m_teamID == teamID) && ((host == ANYHOST) || (host == a->m_host))) break;
		a = a->m_next;
	};
	UnlockAppList();
	return a;
};

bool SApp::ShutdownClients()
{
	BParcel msg(B_QUIT_REQUESTED);
	LockAppList();
	if (!m_firstApp) {
		UnlockAppList();
		return true;
	};
	SApp *a = m_firstApp;
	while (a) {
		a->SendEvent(&msg,0);
		a = a->m_next;
	};
	if (m_shutdownSem != -1) m_shutdownSem = create_sem(0,"shutdownSem");
	UnlockAppList();
	return acquire_sem_etc(m_shutdownSem,1,B_TIMEOUT,3000000) == B_OK;
};

#ifdef REMOTE_DISPLAY

int32 SApp::EventPiper()
{
	int32 tag,size;
	uint8 buffer[1024];
	EventPipe *p;
	while (read_port(m_eventPiperPort,&tag,buffer,1024) >= 0) {
		m_eventSocketLock.Lock();
		for (p=m_eventPipes;p;p=p->next) {
			if (p->m_clientBlock == tag) {
				p->ProcessPending();
				int32 count = p->Messages();
				for (;count;count--) {
					p->LockAddresses();
					char *msg = p->GenerateMessage(&size);
					m_eventSocket->Send(&p->clientTag,4);
					m_eventSocket->Send(&size,4);
					m_eventSocket->Send(msg,size);
					p->UnlockAddresses();
				};
			};
		};
		m_eventSocketLock.Unlock();
	};
	return 0;
}

int32 SApp::EventPiperLauncher(SApp *app)
{
	return app->EventPiper();
}

void SApp::RemoveEventPipe(int32 clientTag)
{
	if (m_eventSocket) {
		m_eventSocketLock.Lock();
		EventPipe **p=&m_eventPipes;
		while (*p) {
			if ((*p)->clientTag == clientTag) {
				EventPipe *old = *p;
				*p = (*p)->next;
				delete old;
			} else {
				p = &(*p)->next;
			};
		};
		m_eventSocketLock.Unlock();
	};
}

port_id SApp::AddEventPipe(uint32 atomic, uint32 nextBunch, sem_id clientBlock, int32 clientTag)
{
	if (m_eventSocket) {
		m_eventSocketLock.Lock();
		EventPipe *p = new EventPipe(atomic,nextBunch,m_RWHeap,m_ROHeap,clientBlock);
		p->clientTag = clientTag;
		p->next = m_eventPipes;
		m_eventPipes = p;
		if (m_eventPiperPort == -1) {
			/*	This was the first event pipe added, so we need to create the
				event piper thread now. */
			m_eventPiperPort = create_port(100,"piperport");
			m_eventPiper = xspawn_thread((thread_entry)EventPiperLauncher,"event_piper",B_URGENT_DISPLAY_PRIORITY,this);
		};
		m_eventSocketLock.Unlock();
		return m_eventPiperPort;
	};
	return -1;
}

void SApp::SetEventSocket(TCPIPSocket *socket)
{
	m_eventSocket = socket;
};

#endif
