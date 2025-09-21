/*****************************************************************************

	File : main.h

	Written by: Peter Potrebic, Steve Horowitz

	Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <time.h>

#include <Application.h>
#include <MessageFilter.h>
#include <Mime.h>

#include "clip.h"
#include "mime_priv.h"
#include "Monitor.h"
#include "Timer.h"
#include "RecentFiles.h"

class _TRoster_;
class TFilter;
class TRegistrar;

struct _p_ainfo_ {
	thread_id	thread;
	team_id		team;
	port_id		port;
	uint32		flags;
	entry_ref	*ref;
	bool		full_reg;
	uint32		entry_token;
	char		signature[B_MIME_TYPE_LENGTH];
};

enum Busy {
	BUSY_WAIT,
	NO_BUSY_WAIT,
	MUST_BUSY_WAIT
};

/* --------------------------------------------------------------- */

class TRosterApp : public BApplication {
public:
						TRosterApp();
virtual					~TRosterApp();

virtual	void			ReadyToRun();
virtual	void			Pulse();
		
static	void			SendMimeChangedNotices(BMessage *);
						// used by the BMimeType class to cause mime changed notices
						// to be sent
private:
		_TRoster_		*fTheRoster;
		TFilter			*fFilter;
		TRegistrar		*fMainRegistrar;
		TClipHandler	*fMainClip;
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
		TMimeWorker		*fMime;
#endif
		timer_token		fTimerToken;
};

/* --------------------------------------------------------------- */

class TRegistrar : public BHandler {
public:
					TRegistrar(_TRoster_ *roster, bool is_main = FALSE);

virtual	void		MessageReceived(BMessage *msg);

private:
		void		AsyncHandle(BMessage *msg);
		void		Broadcast(BMessage *msg, BMessenger reply_to);

		_TRoster_	*fRoster;
		bool		fIsMain;
};

/* --------------------------------------------------------------- */

class TFilter : public BMessageFilter {
public:
						TFilter(TRegistrar *reg,
								BMessenger main_clip,
								BMessenger mime);
virtual	filter_result	Filter(BMessage *msg, BHandler **target);

private:
		TRegistrar	*fPreferred;
		BMessenger	fMainClip;
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
		BMessenger	fMime;
#endif
};

/* --------------------------------------------------------------- */

class _TRoster_ {
public:
					_TRoster_();
virtual				~_TRoster_();

		void		ShuttingDown(bool reboot);
		team_id		TeamFor(const char *mime_sig, Busy *wait) const;
		team_id		TeamFor(const entry_ref *ref, Busy *wait) const;
		void		GetAppList(const char *mime_sig, BList *team_ids) const;
		void		GetAppList(BList *team_id_list) const;
		status_t	GetAppInfo(const char *sig, app_info *info, Busy *w) const;
		status_t	GetAppInfo(const entry_ref *r, app_info *i, Busy *w) const;
		status_t	GetRunningAppInfo(team_id team, app_info *info) const;
		status_t	GetActiveAppInfo(app_info *info, BMessenger *window=NULL) const;
		int32		RemoveApp(team_id team);
		void		ValidateApps();
		bool		ValidateIndex(int32 index);
		status_t	ActivateApp(team_id team);

		void		GetRecentFiles(const char *fromApp, const BMessage *typeList,
						int32 maxCount, BMessage *result) const;
		void		GetRecentFolders(const char *fromApp, int32 maxCount,
						BMessage *result) const;
		void		GetRecentApps(int32 maxCount, BMessage *result) const;

		void		DocumentOpened(const entry_ref *doc, const char *appSig);
		void		FolderOpened(const entry_ref *doc, const char *appSig);

		BMessenger	MakeMessenger(const char *mime_sig, team_id tid);

		status_t	MonitorApps(const BMessenger &target,
								bool start,
								uint32 event_mask);
		
		status_t	HandleTimerMsg(const BMessage *msg, BMessage *reply);

		TTimer		*Timer() const
						{ return fTimer; };

private:
friend class TRegistrar;
friend class TRosterApp;
friend class TState;

		uint32		AddApplication(const char *mime,
									const entry_ref *ref,
									uint32 flags,
									team_id team,
									thread_id thread,
									port_id port,
									bool full_reg);
		bool		IsAppPreRegistered(const entry_ref *ref,
										team_id team,
										app_info *info,
										Busy *wait) const;
		void		CompleteRegistration(team_id team,
										thread_id thread,
										port_id port);
		void		RemovePreRegApp(uint32 entry_token);
		void		SetSignature(team_id team, const char *mime);
		void		SetThread(team_id team, thread_id tid);
		void		SetAppFlags(team_id team, uint32 flags);
		void		SetThreadAndTeam(uint32 entry_token,
									 thread_id tid,
									 team_id team);
		void		DumpRoster() const;
		bool		UpdateActiveApp(team_id team, const BMessenger& window);

		int32		IndexOfApp(const char *mime_sig) const;
		int32		IndexOfThread(thread_id thread) const;
		int32		IndexOfTeam(team_id thread) const;
		int32		IndexOfRef(const entry_ref *ref) const;
		int32		IndexOfPort(port_id port) const;
		int32		IndexOfToken(uint32 a_token) const;
		status_t	GetIndexInfo(int32 index, app_info *info) const;
		void		RemoveIndex(int32 index);

		int32		LaunchPrep(	const char *mime_sig,
								const entry_ref *ref,
								uint32 app_flags,
								port_id port,
								bool *is_running);
		void		Notify(	type_code what,
							const char *mime_sig,
							team_id team,
							thread_id thread,
							uint32 flags,
							const entry_ref *ref,
							const BMessenger* window=NULL);
		status_t	RemoveMonitor(const BMessenger &target);

		BLocker		fLock;
		int32		fCount;
		_p_ainfo_	*fData;
		int32		fPhysicalCount;
		uint32		fEntryToken;
		team_id		fActiveApp;
		BMessenger	fActiveWindow;
		TMonitor	fMonitor;
		TTimer		*fTimer;
		bool		fDragVisible;
		RecentItemsDict fRecentItems;
};

extern _TRoster_ *gTheRoster;

void	SpawnShutdownTask(_TRoster_ *roster, bool isReboot);
void	CleanupRegistrar();

/* ----------------------------------------------------------------- */


const int32	kStateVersion = 4;

struct TState {
	// no virtuals in this object! It is read/written from/to a file
	// and it is safer to add new fields to end of struct
		int32		fVersion;
		time_t		fLastShutdownTime;
		time_t		fStartupTime;
		time_t		fLastCompleteCycle;
		time_t		fQueryTime;
		time_t		fQueryIssueTime;
		bool		fDragVisible;
		BPoint		fShutdownLoc;
		int32		fPrevVersion;	// if differs from fVersion means
									// that we've booted into new version

					TState();
					~TState();
};

extern TState	*gState;

#endif
