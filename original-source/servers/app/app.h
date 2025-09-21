
//******************************************************************************
//
//	File:			app.h
//	Description:	Class to encapsulate per-client information
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef APP_H
#define APP_H

#include <SupportDefs.h>
#include "heap.h"
#include "lock.h"

#define AFLAG_DISPOSE_DRAG		0x00000001
#define AFLAG_CURSOR_HIDDEN		0x00000002
#define AFLAG_CURSOR_OBSCURED	0x00000004

#define ANYHOST					0
#define LOCALHOST				1

class SBitmap;
class EventPipe;
class SCursor;
class TSession;
class BParcel;

class TCPIPSocket;
class SApp {

	private:

		static	sem_id				m_shutdownSem;
		static	Benaphore			m_appListLock;
		static	SApp *				m_firstApp;
				SApp *				m_next;
				
				HeapArea *			m_RWHeap;
				HeapArea *			m_ROHeap;
				TSession *			m_session;
				int32				m_teamID;
				uint32				m_flags;
				port_id				m_eventPort;
				thread_id			m_appTask;
				SBitmap *			m_icon;
				SCursor *			m_cursor;
				
				uint32				m_host;
				EventPipe *			m_eventPipes;
				port_id				m_eventPiperPort;
				thread_id			m_eventPiper;
				TCPIPSocket *		m_eventSocket;
				CountedBenaphore	m_eventSocketLock;

	public:

							SApp(TSession *session, int32 teamID, port_id eventPort=-1, uint32 hostDesig=LOCALHOST);
							~SApp();
	
				bool		IsLocal() { return (m_host == LOCALHOST); };

				SCursor *	Cursor();
				void		SetCursor(SCursor *cursor);
				status_t	SetCursor();

				uint32		Flags() { return m_flags; };
				void		SetFlag(uint32 flag) { atomic_or((int32*)&m_flags,flag); };
				void		UnsetFlag(uint32 flag) { atomic_and((int32*)&m_flags,~flag); };
	
				status_t	NewCursor();
				status_t	DeleteCursor();
				void		ShowCursor();
				void		HideCursor();
				void		ObscureCursor();
				bool		CursorIsHidden();

				status_t 	NewLBX();
				status_t 	DeleteLBX();

				status_t	NewBitmap();
				status_t	DeleteBitmap();

				status_t	NewWindow(TSession *session = NULL);
				status_t	NewOffscreenWindow(TSession *session = NULL);
				void		GetOverlayRestrictions();

				status_t	SendEvent(BParcel *event, bigtime_t timeout=0, int32 thid=-1);

		static	void		LockAppList() { m_appListLock.Lock(); };
		static	void		UnlockAppList() { m_appListLock.Unlock(); };
		static	SApp *		LookupAppByTeamID(int32 teamID, uint32 hostDesign=ANYHOST);
		static	SApp *		FirstApp() { return m_firstApp; };
				SApp *		Next() { return m_next; };
				HeapArea *	RWHeap() { return m_RWHeap; };
				HeapArea *	ROHeap() { return m_ROHeap; };
				int32		TeamID() { return m_teamID; };
				SBitmap *	MiniIcon() { return m_icon; };

		static	bool		ShutdownClients();
		static	int32		LaunchAppTask(SApp *app);
				int32		AppTask();
				
				void		MouseUp(BParcel *msg, fpoint location, area_id droppedArea);

				#ifdef REMOTE_DISPLAY
				void		RemoveEventPipe(int32 clientTag);
				port_id		AddEventPipe(uint32 atomic, uint32 nextBunch, sem_id clientBlock, int32 clientTag);
		static	int32		EventPiperLauncher(SApp *app);
				int32		EventPiper();
				void		SetEventSocket(TCPIPSocket *socket);
				#endif
};

#endif
