/*****************************************************************************

	File : main.cpp

	$Revision: 1.34 $

	Written by: Peter Potrebic, Steve Horowitz

	Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <main.h>

#include <OS.h>
#include <roster_private.h>
#include <AppDefsPrivate.h>
#include <Messenger.h>
#include <OS.h>
#include <image.h>
#include <Application.h>
#include <Autolock.h>
#include <token.h>
#include <FindDirectory.h>
#include <private/storage/mime_private.h>
#include <apm.h>

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>


#define CHUNK_SIZE 3

#define kTrackerSignature "application/x-vnd.Be-TRAK"

TState		*gState;
_TRoster_	*gTheRoster;

TState::TState()
{
	int fd;
	status_t err = B_OK;

	BPath p;
	find_directory(_MIME_PARENT_DIR_, &p);
	p.Append(META_MIME_ROOT);
	p.Append("__reg_data");

	fd = open(p.Path(), O_RDONLY);
	if (fd >= 0) {
		ssize_t	r = read(fd, this, sizeof(TState));
//+		PRINT(("read=%d, fVersion=%d\n", r, fVersion));
		fPrevVersion = fVersion;
		if ((r != sizeof(TState)) || (fVersion != kStateVersion)) {
			bool handled_upgrade = false;
			// code to upgrade from ver 2 to 3.
			if (fVersion == 2 && (kStateVersion >= 3)) {
				fShutdownLoc = BPoint(180, 50);
				err = B_OK;
				handled_upgrade = true;
			}
			if ((fVersion <= 3) && (kStateVersion == 4)) {
				// bumped version to 4 when changing intel (pe) executables
				// to have a different file type from PPC (pef) executables
				err = B_OK;
				handled_upgrade = true;
			}

			// save the version number on disk into the 'prev' field

//+			fPrevVersion = fVersion;
			if (!handled_upgrade) 
				err = B_ERROR;
			else
				fVersion = kStateVersion;
			
		}
	} else
		err = fd;

	if ((fd < 0) || err) {
//+		PRINT(("error dealing with __reg_data\n"));
		fPrevVersion = 0;
		fShutdownLoc = BPoint(180, 50);
		fVersion = kStateVersion;
		fLastCompleteCycle = 0;
		fLastShutdownTime = 0;
		fQueryTime = 864952100;	// ??? bug with Queries... should be 0
		fDragVisible = false;
	}

	fStartupTime = time(NULL);
	fQueryIssueTime = 0;

	/// sanity checking on the saved times
	if (fLastShutdownTime > fStartupTime)
		fLastShutdownTime = 0;
	if (fLastCompleteCycle > fStartupTime)
		fLastCompleteCycle = 0;
	if (fQueryTime > fStartupTime)
		fQueryTime = 0;

	gTheRoster->fDragVisible = fDragVisible;

//+	PRINT(("StartUp      = %d\n", fStartupTime)); 
//+	PRINT(("LastShutdown = %d\n", fLastShutdownTime)); 
//+	PRINT(("CompleteCycle    = %d\n", fLastCompleteCycle)); 
//+	PRINT(("QueryTime  = %d\n", fQueryTime)); 
//+	PRINT(("DragVisible    = %d\n", fDragVisible)); 

	if (fd >= 0)
		close(fd);
}


TState::~TState()
{
	int fd;
	BPath p;
	find_directory(_MIME_PARENT_DIR_, &p, true);
	p.Append(META_MIME_ROOT);
	p.Append("__reg_data");

	fLastShutdownTime = time(NULL);
	fDragVisible = gTheRoster->fDragVisible;
	fVersion = fPrevVersion = kStateVersion;

	fd = open(p.Path(), O_CREAT | O_WRONLY, 0666);
	if (fd >= 0) {
		write(fd, this, sizeof(TState));
		close(fd);
	}
//+	PRINT(("StartUp		= %d\n", fStartupTime)); 
//+	PRINT(("LastShutdown	= %d\n", fLastShutdownTime)); 
//+	PRINT(("CompleteCycle	= %d\n", fLastCompleteCycle)); 
//+	PRINT(("QueryTime  = %d\n", fQueryTime)); 
//+	PRINT(("DragVisible    = %d\n", fDragVisible)); 
}


uint32 to_old_sig(const char *mime_sig)
{
	uint32 old;

	if (!mime_sig)
		return 0;

	char *p = strchr(mime_sig, '/');
	if (!p)
		old = 0xffffffff;
	else
		memcpy(&old, p+1, sizeof(uint32));

	return old;
}

//void FixVolumes();

int
main()
{
//	FixVolumes();
	
	TRosterApp app;

	app.Run();
	return 0;
}

// #pragma mark -

TRosterApp::TRosterApp()
	:	BApplication(ROSTER_SIG)
{	
	fTheRoster = new _TRoster_();
	gTheRoster = fTheRoster;
	gState = new TState();

	fMainRegistrar = new TRegistrar(fTheRoster, true);
	fMainClip = new TClipHandler(MAIN_CLIPBOARD_NAME, false);

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	fMime = TMimeWorker::NewWorker("main_mime", B_NORMAL_PRIORITY);
#endif

	AddHandler(fMainRegistrar);
	AddHandler(fMainClip);

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	fFilter = new TFilter(fMainRegistrar, BMessenger(fMainClip),
		BMessenger(fMime));
#else
	fFilter = new TFilter(fMainRegistrar, BMessenger(fMainClip),
		BMessenger());
#endif

	AddFilter(fFilter);

	fTimerToken = 0;

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	fMime->Run();
#endif
}

TRosterApp::~TRosterApp()
{
	PRINT(("Should never be here in ~TRosterApp\n"));
}

void TRosterApp::Pulse()
{
	fTheRoster->ValidateApps();
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	TMimeWorker::PulseAll();
#endif
}


void TRosterApp::ReadyToRun()
{
	entry_ref	ref;
	int32		err;
	BPath		path;

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	BMessenger	mess(fMime);
	BMimeType::_set_local_dispatch_target_(&mess, &TRosterApp::SendMimeChangedNotices);

	BMessage	reply;
	mess.SendMessage(CMD_FIRST_WAKE_UP, &reply);		// wake up!
#endif
	SERIAL_PRINT(("finished with startup\n"));

	BMessenger	me(this);
	BMessage	m(B_PULSE);
	fTimerToken = gTheRoster->Timer()->Add(me, m, me, 5*1000000, -1);

	// Must manually register SELF. Previous attempts would have caused
	// either recursion/deadlock or Messenger failure as proper thread didn't
	// yet exist (see the rename_thread below).
	port_id	port = find_port(ROSTER_PORT_NAME);
	find_directory(B_BEOS_SERVERS_DIRECTORY, &path);
	path.Append("registrar");

	err = get_ref_for_path(path.Path(), &ref);
//+	PRINT(("get_ref_from_path(%s), err=%x\n", path.Path(), err));

	if (err != B_OK) 
		SERIAL_PRINT(("ReadyToRun failed\n"));


	fTheRoster->AddApplication(ROSTER_MIME_SIG, &ref,
		B_EXCLUSIVE_LAUNCH | B_BACKGROUND_APP, Team(), Thread(), port, 1);

	// this signals the world that the registrar/roster is ready
	rename_thread(Thread(), ROSTER_THREAD_NAME);
}

void 
TRosterApp::SendMimeChangedNotices(BMessage *message)
{
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	static_cast<TRosterApp *>(be_app)->fMime->MetaMimeChanged(message);
#else
	(void)message;
#endif
}


TFilter::TFilter(TRegistrar *reg, BMessenger main_clip, BMessenger mime)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fPreferred = reg;
	fMainClip = main_clip;
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	fMime = mime;
#else
	(void)mime;
#endif
}

filter_result TFilter::Filter(BMessage *msg, BHandler **target)
{
//+	SERIAL_PRINT(("ROSTER SERVER: filter msg %.4s, orig=%s\n",
//+		(char *) &(msg->what), *target ? (*target)->Name() : "null"));

	filter_result	result = B_DISPATCH_MESSAGE;
	uint32			what = msg->what;

	if (what == CMD_NEW_CLIP) {
		result = B_SKIP_MESSAGE;
		fMainClip.SendMessage(msg);
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	} else if ((what >= CMD_FIRST_MIME_CMD) && (what <= CMD_LAST_MIME_CMD)) {
		result = B_SKIP_MESSAGE;
		fMime.SendMessage(msg);
#endif
	} else if (!((what == B_READY_TO_RUN) || (what == B_PULSE))) {
		*target = fPreferred;
	}

//+	SERIAL_PRINT(("\t:new target=%s\n", *target ? (*target)->Name() : "null"));
	return result;
}

TRegistrar::TRegistrar(_TRoster_ *roster, bool is_main)
	: BHandler(is_main ? "main_reg" : "sub_reg")
{
	fRoster = roster;
	fIsMain = is_main;
}

void TRegistrar::MessageReceived(BMessage *msg)
{
	status_t err = B_OK;
	bool send_reply = true;
	BMessage reply(B_REPLY);

	const char *recentAppSig = NULL;
	int32 recentMaxCount = 10;


//+	SERIAL_PRINT(("ROSTER SERVER: received msg %.4s (%x)\n",
//+		(char *) &(msg->what), msg->what));

	Busy wait = fIsMain ? NO_BUSY_WAIT : BUSY_WAIT;

	/*
	 *****
	 ***** Be very defensive here. Don't assume that the correct data 
	 ***** is in the message. Can't have the roster_server crashing!!!
	 *****
	*/
	switch (msg->what) {
		case B_CANCEL:
			SERIAL_PRINT(("Some app just canceled out of a QuitRequested\n"));
			break;
			
		case CMD_GET_DRAG_STATE:
			err = reply.AddBool("visible", fRoster->fDragVisible);
			break;
			
		case CMD_SET_DRAG_STATE:
			{
				bool	b;
				err = msg->FindBool("visible", &b);
//+				SERIAL_PRINT(("roster: b=%d, err=%x\n", b, err));
				if (err == B_OK && (fRoster->fDragVisible != b)) {
//+					SERIAL_PRINT(("roster HERE\n"));
					fRoster->fDragVisible = b;
					BMessage bbb(_SHOW_DRAG_HANDLES_);
					bbb.AddBool("visible", b);
					Broadcast(&bbb, BMessenger());
				}
				send_reply = false;
				break;
			}
			
		case CMD_MSG_SCHEDULER: 
			err = fRoster->HandleTimerMsg(msg, &reply);
			break;

		case CMD_MONITOR_APPS:
			{
				bool start;
				uint32 emask;
				BMessenger target;
				err = msg->FindBool("start", &start);
				if (err != B_OK)
					break;
				err = msg->FindMessenger("target", &target);
				if (err != B_OK)
					break;
				err = msg->FindInt32("event_mask", (int32 *) &emask);
				err = fRoster->MonitorApps(target, start, emask);
				break;
			}
			
		case CMD_BROADCAST:
			{
				BMessage	broad;
				BMessenger	reply_mess;
				err = msg->FindMessage("message", &broad);
				if (err != B_OK)
					break;
				err = msg->FindMessenger("reply_to", &reply_mess);
				if (err != B_OK)
					break;
				Broadcast(&broad, reply_mess);
				send_reply = false;
				break;
			}
			
		case CMD_LAUNCH_INFO:
			{
				const char *mime_sig;
				uint32 flags;
				port_id port;
				entry_ref ref;
				bool running;
	
				err = msg->FindString("mime_sig", &mime_sig);
				err = err | msg->FindInt32("flags", (int32*) &flags);
				err = err | msg->FindRef("ref", &ref);
				err = err | (port_id) msg->FindInt32("port", &port);
				if (err != B_OK) 
					err = B_BAD_VALUE;
				else {
					uint32 token = fRoster->LaunchPrep(mime_sig, &ref, flags, port, &running);
					reply.AddInt32("token", token);
					reply.AddBool("running", running);
				}
				break;
			}

		case CMD_ADD_APP:
			{
				entry_ref ref;
				const char *mime_sig;
				uint32 flags;
				team_id	team;
				thread_id thread;
				port_id port;
				bool full_reg;
				
				err = msg->FindString("mime_sig", &mime_sig);
				err = err | msg->FindInt32("flags", (int32 *)&flags);
				err = err | msg->FindInt32("team", &team);
				err = err | msg->FindInt32("thread", &thread);
				err = err | msg->FindInt32("port", &port);
				err = err | msg->FindBool("full_reg", &full_reg);
				err = err | msg->FindRef("ref", &ref);
				
				if (err != B_OK) 
					err = B_BAD_VALUE;
				else {
					uint32 token = fRoster->AddApplication(mime_sig, &ref, flags, team,
						thread, port, full_reg);
					reply.AddInt32("token", token);
				}
				break;
			}

		case CMD_REMOVE_APP:
			{
				team_id	team;
				err = msg->FindInt32("team", &team);
				if (err != B_OK) 
					err = B_BAD_VALUE;
				else 
					err = fRoster->RemoveApp(team);

				break;
			}

		case CMD_TEAM_FOR:
			{
				const char	*mime_sig;
				entry_ref ref;
	
				if (msg->HasString("mime_sig")) 
					err = msg->FindString("mime_sig", &mime_sig);
				else if (msg->HasRef("ref"))
					err = msg->FindRef("ref", &ref);
				else 
					err = B_BAD_VALUE;

				if (err != B_OK) {
					err = B_BAD_VALUE;
					break;
				}
	
				team_id team;
				if (ref.name)
					team = fRoster->TeamFor(&ref, &wait);
				else
					team = fRoster->TeamFor(mime_sig, &wait);
	
				if (wait == MUST_BUSY_WAIT) {
					// tried to avoid busy waiting, but failed. So we must do
					// this asynchronously.
					AsyncHandle(msg);
					send_reply = false;
				} else 
					reply.AddInt32("result", team);
	
				break;
			}
			
		case CMD_GET_APP_INFO:
			{
				const char *mime_sig = NULL;
				team_id	team = -1;
				entry_ref ref;
				bool active_app = false;
				BMessenger window;
				
				if (msg->HasInt32("team"))
					err = msg->FindInt32("team", &team);
				else if (msg->HasString("mime_sig"))
					err = msg->FindString("mime_sig", &mime_sig);
				else if (msg->HasBool("active")) {
					active_app = true;
					err = B_OK;
				} else if (msg->HasRef("ref"))
					err = msg->FindRef("ref", &ref);
				else 
					err = B_BAD_VALUE;

	
				if (err != B_OK) {
					err = B_BAD_VALUE;
					break;
				}
	
				app_info ainfo;
				if (team != -1)
					err = fRoster->GetRunningAppInfo(team, &ainfo);
				else if (active_app)
					err = fRoster->GetActiveAppInfo(&ainfo, &window);
				else if (ref.name)
					err = fRoster->GetAppInfo(&ref, &ainfo, &wait);
				else
					err = fRoster->GetAppInfo(mime_sig, &ainfo, &wait);
	
				if (wait == MUST_BUSY_WAIT) {
					// tried to avoid busy waiting, but failed. So we must do
					// this asynchronously.
					AsyncHandle(msg);
					send_reply = false;
				} else if (err == B_OK) {
					reply.AddString("mime_sig", ainfo.signature);
					reply.AddInt32("thread", ainfo.thread);
					reply.AddInt32("team", ainfo.team);
					reply.AddInt32("port", ainfo.port);
					reply.AddInt32("flags", ainfo.flags);
					reply.AddRef("ref", &ainfo.ref);
					if (active_app && window.IsValid()) {
						reply.AddMessenger("window", window);
					}
				}
	
				break;
			}

		case CMD_GET_APP_LIST:
			{
				const char *mime_sig = NULL;
				BList list;
	
				if (msg->HasString("mime_sig")) 
					err = msg->FindString("mime_sig", &mime_sig);
	
				if (mime_sig)
					fRoster->GetAppList(mime_sig, &list);
				else
					fRoster->GetAppList(&list);
	
				int32 count = list.CountItems();
				if (count > 0) 
					reply.AddData("items", B_RAW_TYPE, list.Items(),
						count * (sizeof(void *)));

				break;
			}

		case CMD_IS_APP_PRE:
			{
				team_id	 team = -1;
				entry_ref ref;
	
				err = msg->FindInt32("team", &team);
				err = err | msg->FindRef("ref", &ref);
				if (err != B_OK) {
					err = B_BAD_VALUE;
					break;
				}
	
				app_info ainfo;

				bool result = fRoster->IsAppPreRegistered(&ref, team, &ainfo, &wait);
	
				if (wait == MUST_BUSY_WAIT) {
					// tried to avoid busy waiting, but failed. So we must do
					// this asynchronously.
					AsyncHandle(msg);
					send_reply = false;
				} else if (result) {
					reply.AddString("mime_sig", ainfo.signature);
					reply.AddInt32("thread", ainfo.thread);
					reply.AddInt32("team", ainfo.team);
					reply.AddInt32("port", ainfo.port);
					reply.AddInt32("flags", ainfo.flags);
					reply.AddRef("ref", &ainfo.ref);
				}

				break;
			}

		case CMD_COMPLETE_REG:
			{
				team_id		team;
				thread_id	thread;
				port_id		port;
	
				err = msg->FindInt32("team", &team);
				err = err | msg->FindInt32("thread", &thread);
				err = err | msg->FindInt32("port", &port);
				if (err != B_OK) {
					err = B_BAD_VALUE;
					break;
				}
				fRoster->CompleteRegistration(team, thread, port);
				break;
			}

		case CMD_REMOVE_PRE_REG:
			{
				uint32 entry_token;
				err = msg->FindInt32("token", (int32 *)&entry_token);
				if (err != B_OK)
					err = B_BAD_VALUE;
				else
					fRoster->RemovePreRegApp(entry_token);
				break;
			}

		case CMD_SET_INFO:
			{
				team_id		team = -1;
				uint32		token;
	
				if (msg->HasInt32("token")) {
					token = msg->FindInt32("token");
					fRoster->SetThreadAndTeam(token, msg->FindInt32("thread"),
						msg->FindInt32("team"));
				} else {
					err = msg->FindInt32("team", &team);
					if (err != B_OK) {
						err = B_BAD_VALUE;
						break;
					}
					if (msg->HasString("mime_sig"))
						fRoster->SetSignature(team, msg->FindString("mime_sig"));
					else if (msg->HasInt32("thread"))
						fRoster->SetThread(team, msg->FindInt32("thread"));
					else if (msg->HasInt32("flags"))
						fRoster->SetAppFlags(team, msg->FindInt32("flags"));
				}
	
				break;
			}

		case CMD_UPDATE_ACTIVE:
			{
				team_id	team;
				bool	result = B_OK;
				err = msg->FindInt32("team", &team);
				if (err != B_OK)
					err = B_BAD_VALUE;
				else {
					BMessenger window;
					msg->FindMessenger("window", &window);
					result = fRoster->UpdateActiveApp(team, window);
				}
				reply.AddBool("result", result);
				break;
			}

		case CMD_DUMP_ROSTER:
			fRoster->DumpRoster();
			break;

		case CMD_ACTIVATE_APP:
			{
				team_id	team;
				err = msg->FindInt32("team", &team);
				if (err != B_OK) 
					err = B_BAD_VALUE;
				else
					err = fRoster->ActivateApp(team);

			}
			break;

		case CMD_GET_RECENT_DOCUMENT_LIST:
			// fall through

		case CMD_GET_RECENT_FOLDER_LIST:
			msg->FindString("be:app_sig", &recentAppSig);
			// fall through
			
		case CMD_GET_RECENT_APP_LIST:
			{
				msg->FindInt32("be:count", &recentMaxCount);
	
				BMessage result;
				switch (msg->what) {
					case CMD_GET_RECENT_DOCUMENT_LIST:
						fRoster->GetRecentFiles(recentAppSig, msg, recentMaxCount, &result);
						break;
	
					case CMD_GET_RECENT_FOLDER_LIST:
						fRoster->GetRecentFolders(recentAppSig, recentMaxCount, &result);
						break;
	
					case CMD_GET_RECENT_APP_LIST:
						fRoster->GetRecentApps(recentMaxCount, &result);
						break;
				}
				reply.AddMessage("result", &result);
				break;
			}
		case CMD_DOCUMENT_OPENED:
		case CMD_FOLDER_OPENED:
			{
				const char *appSig = NULL;
				entry_ref ref;
				err = msg->FindString("be:app_sig", &appSig);
				if (err == B_OK)
					err = msg->FindRef("refs", &ref);
				
				if (err != B_OK)
					err = B_BAD_VALUE;
				else if (msg->what == CMD_DOCUMENT_OPENED)
					fRoster->DocumentOpened(&ref, appSig);
				else
					fRoster->FolderOpened(&ref, appSig);
			}
			break;
			
		case CMD_SHUTDOWN_SYSTEM:
			fRoster->ShuttingDown(false);
			break;

		case CMD_REBOOT_SYSTEM:
			fRoster->ShuttingDown(true);
			break;

#ifdef APM_SUPPORT
		// we should ask apps if this is okay at some point
		case CMD_SUSPEND_SYSTEM:
			_kapm_control_(APM_SLEEP);
			break;
#endif
			
		case B_REPLY:
			send_reply = false;
			break;

		default:
			{
//+				char buffer[100];
//+				sprintf(buffer, "should never be here, what=(%d, 0x%x)\n",
//+				msg->what, msg->what);
//+				SERIAL_PRINT((buffer));
				err = B_BAD_VALUE;
				break;
			}
	}

	if (send_reply) {
		reply.AddInt32("error", err);
		msg->SendReply(&reply);
	}
}

void TRegistrar::AsyncHandle(BMessage *msg)
{
	BLooper *loop = Looper();

	ASSERT(loop);
	ASSERT(fIsMain);
	ASSERT(loop->CurrentMessage() == msg);

	loop = new BLooper();
	TRegistrar *sub_reg = new TRegistrar(fRoster, false);
	loop->Lock();
	loop->AddHandler(sub_reg);
	loop->Unlock();
	loop->PostMessage(msg, sub_reg);
	loop->PostMessage(B_QUIT_REQUESTED);
	loop->Run();
}

void TRegistrar::Broadcast(BMessage *msg, BMessenger reply_to)
{
	BList		list;
	int32		i = 0;
	team_id		tid;
	int32		err;

	fRoster->GetAppList(&list);
	while ((tid = (team_id) list.ItemAt(i++)) != 0) {
		if (tid == be_app->Team())
			continue;			// skip myself
		BMessenger	mess = fRoster->MakeMessenger(NULL, tid);
		// notice the NO_TIMEOUT. Don't want to block sending this msg.
		err = mess.SendMessage(msg, reply_to, 0.0);
	}
}

void CleanupRegistrar()
{
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
	TMimeWorker::TerminateWorkers();
#endif
	delete gState;
	gState = NULL;
}

struct message_map app_monitor_map[] = {
	{ B_REQUEST_LAUNCHED, B_SOME_APP_LAUNCHED },
	{ B_REQUEST_QUIT, B_SOME_APP_QUIT },
	{ B_REQUEST_APP_ACTIVATED, B_SOME_APP_ACTIVATED },
	{ B_REQUEST_WINDOW_ACTIVATED, B_SOME_WINDOW_ACTIVATED },
	{ 0, 0}
};

// #pragma mark -

#define kRosterSettingsDir "Roster"
#define kRosterSettings "RosterSettings"

_TRoster_::_TRoster_()
	:	fMonitor(app_monitor_map),
		fRecentItems(200, 40, 20)
{
	fCount = 0;
	fEntryToken = 1;	/* 0 is an invalid entry_token */
	fData = (_p_ainfo_ *) malloc(CHUNK_SIZE * sizeof(_p_ainfo_));
	fPhysicalCount = CHUNK_SIZE;

	fActiveApp = -1;

	fDragVisible = false;

	fTimer = new TTimer(50000);

	// save out the cached up roster settings
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
	
		// make sure there is a directory
		path.Append(kRosterSettingsDir);
		mkdir(path.Path(), 0777);
		path.Append(kRosterSettings);
		
		fRecentItems.Read(path.Path());
	}
}

_TRoster_::~_TRoster_()
{
	// delete the array itself
	free(fData);

	gTheRoster = NULL;

	status_t	err;
	thread_id	tid = fTimer->Thread();
	fTimer->Stop();
	wait_for_thread(tid, &err);
}

void 
_TRoster_::ShuttingDown(bool reboot)
{
	// save out the cached up roster settings
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	
	// make sure there is a directory
	path.Append(kRosterSettingsDir);
	mkdir(path.Path(), 0777);
	path.Append(kRosterSettings);
	
	fRecentItems.Save(path.Path());

	SpawnShutdownTask(this, reboot);
}


status_t _TRoster_::HandleTimerMsg(const BMessage *msg, BMessage *reply)
{
	status_t	err;
	int32		code;

	err = msg->FindInt32("code", &code);
	if (err != B_OK)
		return err;

	switch (code) {
		case CODE_TASK_ADD: {
			BMessenger	target;
			BMessenger	reply_to;
			bigtime_t	interval;
			int32		count;

			err = msg->FindMessenger("target", &target);
			if (err != B_OK)
				break;

			err = msg->FindMessenger("reply_to", &reply_to);
			if (err != B_OK)
				break;

			err = msg->FindInt64("interval", &interval);
			if (err != B_OK)
				break;

			err = msg->FindInt32("count", &count);
			if (err != B_OK)
				break;

			BMessage	*m = new BMessage();
			err = msg->FindMessage("message", m);
			if (err != B_OK) {
				delete m;
				break;
			}

			timer_token	token;
			token = fTimer->Add(target, m, reply_to, interval, count);
			if (token >= B_OK)
				reply->AddInt32("token", token);
			else
				err = token;
			break;
		}
		case CODE_TASK_REMOVE: {
			timer_token	token = -1;
			err = msg->FindInt32("token", (int32 *) &token);
			if (err == B_OK) 
				fTimer->Remove(token);
	
			break;
		}
		case CODE_TASK_ADJUST: {
			timer_token	token = -1;
			err = msg->FindInt32("token", (int32 *) &token);
			if (err == B_OK) {
				bigtime_t	interval;
				int32		count;
				bool		reset_i;
				bool		reset_c;
				reset_i = (msg->FindInt64("interval", &interval) == B_OK);
				reset_c = (msg->FindInt32("count", &count) == B_OK);

				err = fTimer->Adjust(token, reset_i, interval, reset_c, count);
			}
			break;
		}
		case CODE_TASK_INFO: {
			timer_token	token = -1;
			err = msg->FindInt32("token", (int32 *) &token);
			if (err == B_OK) {
				bigtime_t	interval;
				int32		count;
				err = fTimer->GetInfo(token, &interval, &count);
				if (err == B_OK) {
					reply->AddInt64("interval", interval);
					reply->AddInt32("count", count);
				}
			}
			break;
		}
		default: 
			err = B_BAD_VALUE;
			break;
	}
	return err;
}

status_t _TRoster_::MonitorApps(const BMessenger &target, bool start,
	uint32 event_mask)
{
	if (start)
		return fMonitor.Add(target, event_mask);
	else
		return fMonitor.Remove(target);
}

status_t _TRoster_::ActivateApp(team_id team)
{
	return be_roster->ActivateApp(team);
}

void _TRoster_::ValidateApps()
{
	BAutolock auto_lock(fLock);

//+	SERIAL_PRINT(("VALIDATE ROSTER\n"));

	_p_ainfo_	*entry = fData;
	int32		count = fCount;		// fCount will change as we delete items

	for (int32 index = 0; index < count; index++) {

		// skip apps that doesn't have a thread yet (isn't fully registered)
		if (!entry->full_reg && (entry->thread == -1))
			continue;

		thread_info	tinfo;
		thread_id	main_thread = entry->thread;
		int32		err = get_thread_info(main_thread, &tinfo);

//+		if (main_thread <= 0) {
//+			SERIAL_PRINT(("VALIDATE: bad thread=%d\n", main_thread));
//+		}

		if (err != B_NO_ERROR) {
//+			if (entry->team <= 0) {
//+				SERIAL_PRINT(("VALIDATE: bad team=%d\n", entry->team));
//+			}
			RemoveApp(entry->team);
		} else {
			// go to the next entry;
			entry++;
		}
	}
}


bool _TRoster_::ValidateIndex(int32 index)
{
	ASSERT((fLock.IsLocked()));

	if ((index < 0) || (index >= fCount))
		return (false);

	_p_ainfo_ *entry = fData + index;

	if ((!entry->full_reg) && (entry->thread == -1))
		return (true);

	thread_info tinfo;
	if (get_thread_info(entry->thread, &tinfo) == B_NO_ERROR)
		return (true);
	else
		RemoveApp(entry->team);

	return (false);
}

int32 _TRoster_::LaunchPrep(const char *mime_sig, const entry_ref *ref,
			uint32 app_flags, port_id app_port, bool *is_running)
{
	int32	entry_token = -1;
	bool	running;

	BAutolock auto_lock(fLock);

	// determine run status based on "type" of application
	// B_MULTIPLE_LAUNCH	- always launch/register another instance of app
	// B_EXCLUSIVE_LAUNCH	- ensure only one instance of an app with given
	//				   	  mime_sig is running at one time
	// B_SINGLE_LAUNCH	- ensure only one instance of an app with given db_ref
	//					  is running at one time
	if ((app_flags & B_LAUNCH_MASK) == B_MULTIPLE_LAUNCH)
		running = false;
	else {
		// check to see if app was pre-registered already
		if ((app_flags & B_LAUNCH_MASK) == B_EXCLUSIVE_LAUNCH)
			running = ValidateIndex(IndexOfApp(mime_sig));
		else
			running = ValidateIndex(IndexOfRef(ref));
	}

	if (!running) {
		// pre-register application, to be completed by BApp constructor
//+		SERIAL_PRINT(("pre-registering, port=%d\n", app_port));
		entry_token = AddApplication(mime_sig, ref, app_flags, -1, -1,
			app_port, false);
	}
//+	SERIAL_PRINT(("Launch (REMOTE), running=%d, token=%d\n",
//+		running, entry_token));

	*is_running = running;
	return entry_token;
}

void _TRoster_::SetThread(team_id team, thread_id thread)
{
	int32	index;

	BAutolock auto_lock(fLock);

	index = IndexOfTeam(team);
	ASSERT(index != -1);

	fData[index].thread = thread;
}

void _TRoster_::SetThreadAndTeam(uint32 entry_token, thread_id thread, team_id team)
{
	int32	index;

	ASSERT(entry_token != 0);

	BAutolock auto_lock(fLock);

	index = IndexOfToken(entry_token);
	ASSERT(index != -1);

	fData[index].thread = thread;
	fData[index].team = team;
}

bool _TRoster_::IsAppPreRegistered(const entry_ref *ref, team_id team,
	app_info *info, Busy *wait) const
{
	bool		rval = false;
	
	_p_ainfo_	*entry = fData;
	int32		index;
	bool		still_waiting = true;
	bool		potential_candidate;

	while (still_waiting) {
		{
		BAutolock auto_lock((BLocker&) fLock);

		// assume the worst, that there isn't a pre_reg instance of app
		potential_candidate = false;

		for (entry = fData, index = 0; index < fCount; index++, entry++) {
			if ((*entry->ref == *ref) && (!entry->full_reg)) {

					// found a pre_reg instance of (ref)
					potential_candidate = true;

					// if the teams match then we are done
					if (entry->team == team) {
						still_waiting = false;
						rval = true;
						GetRunningAppInfo(team, info);
						break;
					}
					// team id's don't match so continue search of roster data
			}
		}
		}

		if (!potential_candidate)
			break;
		
		if ((*wait == NO_BUSY_WAIT) && still_waiting) {
			// we've got a potential candidate, but the team_id hasn't
			// been filled in yet, AND we can't busy wait here.
			*wait = MUST_BUSY_WAIT;
			rval = false;
			break;
		}

		if (still_waiting)
			snooze(100000);
	}

	return rval;
}

void _TRoster_::CompleteRegistration(team_id team, thread_id thread,
	 port_id port)
{
	_p_ainfo_	*entry;
	int32		index;

//+	SERIAL_PRINT(("CompleteRegistration: team=%d, port=%d\n", team, port));

	BAutolock auto_lock(fLock);

	index = IndexOfTeam(team);
	ASSERT(index != -1);
	ASSERT(!fData[index].full_reg);
	fData[index].full_reg = true;
	fData[index].thread = thread;
	fData[index].port = port;
	entry = &fData[index];

	// if we successfully completed registration then tell browser
	Notify(B_SOME_APP_LAUNCHED, entry->signature, entry->team, entry->thread,
			entry->flags, entry->ref);

	return;
}

void _TRoster_::DumpRoster() const
{
//	_p_ainfo_	*entry = fData;
//	int32		index;

	BAutolock auto_lock((BLocker&) fLock);

//+	PRINT(("-----------------------\nDumpRoster:\n"));
//+	for (index = 0; index < fCount; index++) {
//+		PRINT(("f=%d, name=%s, sig=%s, tid=%d, team=%d, port=%d, flags=%x, dir=%d\n",
//+			entry->full_reg, entry->ref.name,
//+			entry->signature,
//+			entry->thread, entry->team, entry->port,
//+			entry->flags, entry->ref->directory));
//+		entry++;
//+	}
//+	PRINT(("-----------------------\n"));
}

void _TRoster_::GetAppList(BList* team_list) const
{
	_p_ainfo_	*entry = fData;
	int32		index;

	BAutolock auto_lock((BLocker&) fLock);

	for (index = 0; index < fCount; index++) {
		// only add fully registered apps
		if (entry->full_reg)
			team_list->AddItem((void*)entry->team);
		entry++;
	}
//+	DumpRoster();
}

void _TRoster_::GetAppList(const char *mime_sig, BList* team_list) const
{
	_p_ainfo_	*entry = fData;
	int32		index;

	BAutolock auto_lock((BLocker&) fLock);

	for (index = 0; index < fCount; index++) {
		if (entry->full_reg && (strcasecmp(entry->signature, mime_sig) == 0))
			team_list->AddItem((void*)entry->team);
		entry++;
	}
}

status_t _TRoster_::GetIndexInfo(int32 index, app_info *info) const
{
	// this is a private method
	status_t err = B_OK;

	if (!fLock.IsLocked())
		return -1;
	
	if (index != -1) {
		strcpy(info->signature, fData[index].signature);
		info->thread = fData[index].thread;
		info->team = fData[index].team;
		info->port = fData[index].port;
		info->flags = fData[index].flags;
		info->ref = *fData[index].ref;
	} else {
		err = B_BAD_INDEX;
		info->signature[0] = '\0';
		info->thread = -1;
		info->team = -1;
		info->port = -1;
		info->flags = 0;
	}

	return err;
}

status_t _TRoster_::GetAppInfo(const entry_ref *ref, app_info *info,
	Busy *wait) const
{
	status_t err = B_OK;
	team_id team;

//+	SERIAL_PRINT(("GetAppInfo(ref)\n"));
	team = TeamFor(ref, wait);
	if (team >= 0) {
		err = GetRunningAppInfo(team, info);
	} else {
		// ??? GetAppInfo only works on running applications
		err = B_ERROR;
		info->signature[0] = '\0';
		info->thread = -1;
		info->team = -1;
		info->port = -1;
		info->flags = 0;
	}

	return err;
}

status_t _TRoster_::GetAppInfo(const char *mime_sig, app_info *info,
	Busy *wait) const
{
	status_t err = B_OK;
	team_id team;

//+	SERIAL_PRINT(("GetAppInfo(mime_sig)\n"));
	team = TeamFor(mime_sig, wait);
	if (team >= 0) {
		err = GetRunningAppInfo(team, info);
	} else {
		// ??? GetAppInfo only works on running applications
		err = B_ERROR;
		info->signature[0] = '\0';
		info->thread = -1;
		info->team = -1;
		info->port = -1;
		info->flags = 0;
	}

	return err;
}

status_t _TRoster_::GetRunningAppInfo(team_id team, app_info *info) const
{
	int32	index;
	status_t err = B_OK;

	BAutolock auto_lock((BLocker&) fLock);
	
	index = IndexOfTeam(team);
	if (index == -1) {
		err = B_BAD_TEAM_ID;
	} else {
		err = GetIndexInfo(index, info);
		ASSERT(err == B_NO_ERROR);
	}
//+	SERIAL_PRINT(("GetRunningAppInfo(team=%d), err=%x, index=%d\n",
//+		team, err, index));

	return err;
}

status_t _TRoster_::GetActiveAppInfo(app_info *info, BMessenger* window) const
{
	status_t err;

//+	SERIAL_PRINT(("GetActiveAppInfo\n"));

	BAutolock auto_lock((BLocker&) fLock);

	if (window) *window = fActiveWindow;
	
	if (fActiveApp != -1)
		err =  GetRunningAppInfo(fActiveApp, info);
	else
		// in practice this shouldn't happen - there's always an active app.
		err = B_ERROR;

	return err;
}

bool _TRoster_::UpdateActiveApp(team_id team, const BMessenger& window)
{
	/*
	 This is called whenever a window is 'activated'. Look at
	 the BWindow code 'activation' code. The passed in thread_id
	 is the 'application' thread of the window being activated
	 (it is NOT the window thread). The roster uses that info to
	 determine if a new application was 'activated'
	*/

	BAutolock auto_lock(fLock);

	app_info	ainfo;
	status_t	ainfoResult = B_ERROR;
	bool		changed = false;
	
	if (team != fActiveApp) {
		changed = true;
		
		/*
		 The currently active app is getting deactivated so send it a message.
		 Can't construct a messenger in the normal fashion. That would cause
		 a deadlock (creating a messenger usually involves sending a message
		 to the registrar... that's me... deadlock.
		*/
		BMessenger	mess = MakeMessenger(NULL, fActiveApp);
		if (mess.IsValid()) {
			BMessage msg(B_APP_ACTIVATED);
			msg.AddBool("active", 0);
			mess.SendMessage(&msg, (BHandler *) NULL, 500000.0); // 1/2 second wait
		}
	
		fActiveApp = team;
		
		mess = MakeMessenger(NULL, fActiveApp);
		if (mess.IsValid()) {
			BMessage msg(B_APP_ACTIVATED);
			msg.AddBool("active", 1);
			mess.SendMessage(&msg, (BHandler *) NULL, 500000.0); // 1/2 second wait
		}
		
		ainfoResult = GetRunningAppInfo(team, &ainfo);
		if (ainfoResult == B_NO_ERROR) {
			Notify(APP_SWITCH, ainfo.signature, ainfo.team, ainfo.thread,
				ainfo.flags, &ainfo.ref);
		}
	}
	
	if (changed || (window.IsValid() && fActiveWindow != window)) {
		changed = true;
		fActiveWindow = window;
		
		if (ainfoResult < B_OK) {
			ainfoResult = GetRunningAppInfo(team, &ainfo);
		}
		if (ainfoResult == B_NO_ERROR) {
			Notify(B_SOME_WINDOW_ACTIVATED, ainfo.signature, ainfo.team, ainfo.thread,
				ainfo.flags, &ainfo.ref, &fActiveWindow);
		}
	}
	
	return changed;
}

int32 _TRoster_::IndexOfThread(thread_id thread) const
{
	if (!fLock.IsLocked() || (thread < 0))
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		if (entry->thread == thread)
			return index;
		entry++;
	}

	return(-1);
}

int32 _TRoster_::IndexOfTeam(team_id team) const
{
	if (!fLock.IsLocked() || (team < 0))
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		if (entry->team == team)
			return index;
		entry++;
	}

	return(-1);
}

int32 _TRoster_::IndexOfToken(uint32 a_token) const
{
	if (!fLock.IsLocked())
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		if (entry->entry_token == a_token)
			return index;
		entry++;
	}

	return(-1);
}

int32 _TRoster_::IndexOfApp(const char *mime_sig) const
{
	if (!fLock.IsLocked() || !mime_sig)
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		if (strcasecmp(entry->signature, mime_sig) == 0)
			return index;
		entry++;
	}

	return(-1);
}

int32 _TRoster_::IndexOfRef(const entry_ref *ref) const
{
	entry_ref	*aref;

	if (!fLock.IsLocked() || (ref->name[0] == '\0'))
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		aref = entry->ref;
		if ((aref->device == ref->device) &&
			(aref->directory == ref->directory) &&
			!strcmp(aref->name, ref->name))
			return index;
		entry++;
	}

	return(-1);
}

int32 _TRoster_::IndexOfPort(port_id port) const
{
	if (!fLock.IsLocked() || (port == -1))
		return -1;
	
	_p_ainfo_	*entry = fData;
	int32		index;

	for (index = 0; index < fCount; index++) {
		if (entry->port == port)
			return index;
		entry++;
	}

	return(-1);
}

team_id _TRoster_::TeamFor(const char *mime_sig, Busy *wait) const
{
	team_id		team = -1;
	bool		still_waiting = true;
	int32		index;

	while (still_waiting) {
		{
		BAutolock auto_lock((BLocker&) fLock);

		if ((index = IndexOfApp(mime_sig)) != -1) {
			if (fData[index].full_reg) {
				team = fData[index].team;
				still_waiting = false;
			} else if (*wait == NO_BUSY_WAIT) {
				// we've got a potential candidate, but app isn't fully
				// registered yet, AND we can't busy wait here.
				*wait = MUST_BUSY_WAIT;
				break;
			}

		} else 
			still_waiting = false;

		}

		if (still_waiting)
			snooze(50000);
	}

	return team;
}

team_id _TRoster_::TeamFor(const entry_ref *ref, Busy *wait) const
{
	team_id	team = -1;
	bool		still_waiting = true;
	int32		index;

	while (still_waiting) {
		{
		BAutolock auto_lock((BLocker&) fLock);

		if ((index = IndexOfRef(ref)) != -1) {
			if (fData[index].full_reg) {
				team = fData[index].team;
				still_waiting = false;
			} else if (*wait == NO_BUSY_WAIT) {
				// we've got a potential candidate, but the team_id hasn't
				// been filled in yet, AND we can't busy wait here.
				*wait = MUST_BUSY_WAIT;
				break;
			}

		} else {
			still_waiting = false;
		}
	
		}

		if (still_waiting)
			snooze(100000);
	}

	return(team);
}

#if 0

bool _TRoster_::IsRunning(const char *mime_sig, Busy *wait) const
{
	return(TeamFor(mime_sig, wait) != -1);
}

bool _TRoster_::IsRunning(const entry_ref *ref, Busy *wait) const
{
	return(TeamFor(ref, wait) != -1);
}
#endif

void _TRoster_::SetAppFlags(team_id team, uint32 flags)
{
	_p_ainfo_	*entry;
	int32		index;

	BAutolock auto_lock(fLock);

	if ((index = IndexOfTeam(team)) != -1) {
		entry = &fData[index];
//		SPRINT(("SetAppFlags - 0x%x (sig=%s)\n",
//			flags, entry->signature));
		entry->flags = flags;
	}
}

void _TRoster_::SetSignature(team_id team, const char *mime_sig)
{
	_p_ainfo_	*entry;
	int32		index;

	BAutolock auto_lock(fLock);
//+	SERIAL_PRINT(("SetSignature (new=%s)\n", mime_sig));

	if ((index = IndexOfTeam(team)) != -1) {
		entry = &fData[index];

		// don't set if we already have a valid sig
//+		SERIAL_PRINT(("	OldSignature (%s) (%x)\n", entry->signature,
//+			*(entry->signature)));
		if (entry->signature[0] == '\0') {
			strcpy(entry->signature, mime_sig);

			// let browser know if we have a change in registration
			Notify(B_SOME_APP_LAUNCHED, entry->signature, entry->team, entry->thread,
				entry->flags, entry->ref);
		}
	}
}

void _TRoster_::RemoveIndex(int32 index)
{
	ASSERT((fLock.IsLocked()));
	
	ASSERT((index >= 0 && index < fCount));

	fCount--;

	delete fData[index].ref;

	memcpy(	fData + index,							// dest
			fData + index + 1,						// src
			(fCount - index) * sizeof(_p_ainfo_));	// count
}

uint32 _TRoster_::AddApplication(const char *mime_sig, const entry_ref *ref,
	uint32 flags, team_id team, thread_id thread, port_id port, bool full_reg)
{
	uint32		entry_token;
	_p_ainfo_	*entry;
	bool		exitAdd;

//+	PRINT(("Server Dumping (AppApplication)\n"));
//+	DumpRoster();
//+	SERIAL_PRINT(("ref=%x", ref));
//+	SERIAL_PRINT(("REMOTE AddApplication: sig=%s, team=%d, port=%d, full=%d,\n\tdir=%d, vol=%d, flags=0x%x\n",
//+		mime_sig, team, port, full_reg,
//+		ref->directory, ref->device, flags));

	BAutolock auto_lock(fLock);

	// duplicate launch logic here because app might have been launched with
	// exec instead of Launch (see launch logic in LaunchAppPrivate)
	if ((flags & B_LAUNCH_MASK) == B_MULTIPLE_LAUNCH)
		exitAdd = false;
	else
		if ((flags & B_LAUNCH_MASK) == B_EXCLUSIVE_LAUNCH)
			exitAdd = ValidateIndex(IndexOfApp(mime_sig));
		else
			exitAdd = ValidateIndex(IndexOfRef(ref));

	if (exitAdd) {
		return(0);
	}

	if (fCount == fPhysicalCount) {
		fPhysicalCount += CHUNK_SIZE;

		// copy old info array into new, larger array
		fData = (_p_ainfo_*) realloc(fData,
			fPhysicalCount * sizeof(_p_ainfo_));
	}

	entry = &fData[fCount++];

	strcpy(entry->signature, mime_sig ? mime_sig : "");
	entry->thread = -1;
	entry->team = team;
	entry->thread = thread;
	entry->port = port;
	entry->ref = new entry_ref(*ref);
	entry->flags = flags;
	entry->full_reg = full_reg;

	// set the token for this entry (make sure it's not 0)
	entry_token = fEntryToken++;
	if (fEntryToken == 0)
		fEntryToken++;

	entry->entry_token = entry_token;

	ASSERT(!full_reg || (team != -1));

	// only notify if doing a full register (not a pre_register)

	if (full_reg)
		Notify(B_SOME_APP_LAUNCHED, mime_sig, team, thread, flags, ref);
	
//+	PRINT(("Server Dumping (end of AppApplication)\n"));
//+	DumpRoster();
	return(entry_token);
}

void _TRoster_::Notify(type_code what, const char *mime_sig,
	team_id team, thread_id thread, uint32 flags, const entry_ref *ref,
	const BMessenger* window)
{
	ASSERT(fLock.IsLocked());

	PRINT(("Notify: sig=%s, what=%.4s\n",
		mime_sig ? mime_sig : "???", (char *) &what));

	if ((strcasecmp(mime_sig, TASK_BAR_MIME_SIG) == 0) && 
		((what == B_SOME_APP_LAUNCHED) || (what == B_SOME_APP_QUIT))) {
		if (what == B_SOME_APP_LAUNCHED) {
			BMessenger mess(MakeMessenger(TASK_BAR_MIME_SIG, 0));
			fMonitor.Add(mess, B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
		}
	}

	BMessage	msg(what);
	
	if (mime_sig)
		msg.AddString("be:signature", mime_sig);
	if (team >= 0)
		msg.AddInt32("be:team", team);
	if (thread >= 0)
		msg.AddInt32("be:thread", thread);

	msg.AddInt32("be:flags", flags);
	if (ref)
		msg.AddRef("be:ref", ref);
	if (window)
		msg.AddMessenger("be:window", *window);
	
	fMonitor.Notify(&msg);
	
	if (what == B_SOME_APP_LAUNCHED
		&& (flags & (B_BACKGROUND_APP | B_ARGV_ONLY)) == 0
		&& strcmp(mime_sig, TASK_BAR_MIME_SIG) != 0
		&& strcmp(mime_sig, kTrackerSignature) != 0)
		// register app in the recent apps list
		fRecentItems.AccessApp(mime_sig);
}


int32 _TRoster_::RemoveApp(team_id team)
{
	int32		index;
	app_info	info;
	status_t	err;

	BAutolock auto_lock(fLock);
	
//+	PRINT(("Server Dumping (RemoveApp)\n"));
//+	DumpRoster();

	err = GetRunningAppInfo(team, &info);
	if (err != B_NO_ERROR) {
		return err;
	}

//+	strcpy(mime_sig, info.signature);

	index = IndexOfTeam(team);
	ASSERT(index >= 0);

//+	SERIAL_PRINT(("RemoveApp(team id=%d), index=%d \n", team, index));
	RemoveIndex(index);

	Notify(B_SOME_APP_QUIT, info.signature, team, info.thread,
		info.flags, &info.ref);

//+	PRINT(("Server Dumping (end of RemoveApp)\n"));
//+	DumpRoster();
	
	return B_OK;
}

void _TRoster_::RemovePreRegApp(uint32 entry_token)
{
	_p_ainfo_ *entry = fData;
	bool found = false;
	int32 index;

//+	PRINT(("RemovePreRegApp\n"));
	BAutolock auto_lock(fLock);

	for (index = 0; index < fCount; index++) {
		if (entry->entry_token == entry_token) {
			ASSERT(entry->full_reg == false);
			found = true;
			break;
		}
		entry++;
	}

	if (found)
		RemoveIndex(index);
}

BMessenger _TRoster_::MakeMessenger(const char *mime_sig, team_id tid)
{
	if (mime_sig) {
		Busy wait = NO_BUSY_WAIT;
		tid = TeamFor(mime_sig, &wait);
	}
	
	app_info ainfo;
	if (GetRunningAppInfo(tid, &ainfo) == B_OK) 
		return BMessenger(ainfo.team, ainfo.port, NO_TOKEN, false);

	return BMessenger();	
}

void 
_TRoster_::GetRecentFiles(const char *fromApp, const BMessage *typeList,
	int32 maxCount, BMessage *result) const
{
	type_code dummy;
	int32 count = 0;
	typeList->GetInfo("be:type", &dummy, &count);
	
	// if the user specified a mimeType or supertype
	// build a list of BMimeType objects, based on the vector of mime
	// type strings
	BObjectList<BMimeType> mimeTypeList(5, true);
	for (int32 index = 0; index < count; index++) {
		const char *type;
		if (typeList->FindString("be:type", index, &type) != B_OK)
			break;

		mimeTypeList.AddItem(new BMimeType(type));
	}

	fRecentItems.CollectRecentFiles(&mimeTypeList, fromApp, maxCount, result);
}

void 
_TRoster_::GetRecentFolders(const char *fromApp, int32 maxCount,
	BMessage *result) const
{
	fRecentItems.CollectRecentFolders(fromApp, maxCount, result);
}

void 
_TRoster_::GetRecentApps(int32 maxCount, BMessage *result) const
{
	fRecentItems.CollectRecentApps(maxCount, result);
}

void 
_TRoster_::DocumentOpened(const entry_ref *doc, const char *appSig)
{
	fRecentItems.AccessFile(doc, appSig);
}

void 
_TRoster_::FolderOpened(const entry_ref *doc, const char *appSig)
{
	fRecentItems.AccessFolder(doc, appSig);
}

