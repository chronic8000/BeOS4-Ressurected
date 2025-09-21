//******************************************************************************
//
//******************************************************************************

#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <image.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _BOX_H
#include <Box.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _NODE_INFO_H
#include <NodeInfo.h>
#endif
#ifndef _ENTRY_H
#include <Entry.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _REGION_H
#include <Region.h>
#endif
#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif

#include <Roster.h>
#include <Screen.h>
#include <TextView.h>
#include <Timer.h>
#include <ShutdownUtil.h>
#include <priv_syscalls.h>
#include <interface_misc.h>

static	bool shutdownRunning = false;

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

class TBox : public BBox {
public:
					TBox(BRect);
					~TBox();

virtual	void		Draw(BRect update);
virtual	void		AttachedToWindow();
		void		SetZoom(bool state);
		bool		Zoom() const { return fDrawZoom; };
		BRect		ZoomArea();

private:
typedef BBox _inherited;

		bool		fDrawZoom;
		bool		fZoomOnly;
};

class TBitmapView : public BView {
public:
					TBitmapView(BRect r);
virtual				~TBitmapView();
virtual	void		Draw(BRect update);
virtual	void		MouseDown(BPoint wh);
		void		SetBitmap(BBitmap *b);
		void		Reset();
		void		Animate();

private:
		BBitmap		*fBitmap;
		BBitmap		*fBitmapDark;
		float		fDivider;
		bool		fSense;
		bigtime_t	fLastClick;
};

enum {
	/*
	 WARNING - do not change the order of this enumeration. It defines the
	 order of these states.
	*/
	S_IDLE				= 0,
	S_SHUTDOWN_SCRIPT,
	S_QUIT_UI_APPS,
	S_QUIT_UI_APPS_TEMP,
	S_QUIT_ROSTER_APPS,
	S_KILL_ALL_TEAMS,
	S_SHUTDOWN_FINISH_SCRIPT,
	S_DONE,
	S_CANCEL
};

class TShutdownWindow : public BWindow {
public:
					TShutdownWindow(BRect frame, bool isReboot);
					~TShutdownWindow();

virtual	void		DispatchMessage(BMessage *msg, BHandler *target);
virtual	void		MessageReceived(BMessage *msg);
virtual	bool		QuitRequested();
		bool		BlockedByModal() const { return fBlockedByModal; };
		bool		ModalInCurWS() const { return fModalInCurWS; };

		void		Start();
		void		Cancel();


private:
typedef BWindow _inherited;

		void		queue_action();
		void		next_state();

		void		do_process();
		void		quit_team(team_id team, bool hard_way = false);
		void		calc_hint();
		void		init_wait(team_info *tinfo, app_info *ainfo, bool qr);
		status_t	exec_script(const char *name);
static	int32		_wait_entry_(void *);

		TBox		*fTop;
		BMenuBar	*fMenuBar;
		BAlert		*fAlert;
		BTextView	*fText;
		TBitmapView	*fIconView;
		BButton		*fCancelButton;
		BButton		*fKillButton;
		BButton		*fRebootButton;
		BList		fAppList;
		BList		fSkipList;
		int32		fState;
		bool		fIsReboot;
		bool		fBlockedByModal;
		bool		fModalInCurWS;

		team_id		fCurTeam;
		thread_id	fWaitThread;
		thread_id	fScriptThread;
		bigtime_t	fStartTime;
		int32		fSeconds;
		timer_token	fTimerToken;
};


/*------------------------------------------------------------*/

enum {
	CMD_CANCEL_SHUTDOWN = 'cncl',
	CMD_SKIP_APP = 'skip',
	CMD_KILL_APP = 'kill',
	CMD_PROCESS = 'proc',
	CMD_REBOOT = 'rebt',
	CMD_NEXT_STATE = 'nxts',
	CMD_ACTIVATE = 'acta',
	CMD_SCRIPT_DONE = 'sdon'
};

_TRoster_	*RealRoster;

/*------------------------------------------------------------*/

TShutdownWindow::TShutdownWindow(BRect frame, bool isReboot)
	: BWindow(frame, "Shutdown Status", B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE |
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BRect	bounds = Bounds();
	BRect	r;	
	BRect	rr;	

	fState = S_IDLE;
	fSeconds = 0;
	fStartTime = 0;
	fIsReboot = isReboot;
	fCurTeam = -1;
	fScriptThread = -1;
	fWaitThread = -1;
	fBlockedByModal = false;
	fModalInCurWS = false;

	fTop = new TBox(bounds);
	AddChild(fTop);
//+	fTop->SetViewColor(B_TRANSPARENT_8_BIT, B_TRANSPARENT_8_BIT,
//+		B_TRANSPARENT_8_BIT);
//+	fTop->SetLowColor(216, 216, 216);

	r = fTop->Bounds();
	BBox *box = fTop;

	/*
	 Setup the main text field.
	*/
	rr.left = r.left + 55;
	rr.right = r.right - 8;
	rr.top = r.top + 10;
	rr.bottom = rr.top + 30;
	BRect	textr = rr;
	textr.OffsetTo(B_ORIGIN);
	fText = new BTextView(rr, "text", textr, B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW | B_PULSE_NEEDED);
	box->AddChild(fText);
	font_height th;
	fText->GetFontHeight(&th);
	fText->ResizeTo(rr.Width(), (th.ascent+th.descent+th.leading)*2 + 2);
	fText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fText->SetLowColor(fText->ViewColor());
	fText->SetWordWrap(true);
	fText->MakeEditable(false);
	fText->MakeSelectable(false);

	/*
	 Setup the icon view that displays the app's icon
	*/
	rr.left = r.left + 15;
	rr.top = r.top + 8;
	rr.right = rr.left + B_LARGE_ICON;
	rr.bottom = rr.top + B_LARGE_ICON;
	fIconView = new TBitmapView(rr);
	fIconView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fIconView->SetLowColor(fIconView->ViewColor());
	box->AddChild(fIconView);

	/*
	 The Kill Application button
	*/

	rr.left = r.left + 20;
	rr.right = rr.left + 115;
	rr.top = fText->Frame().bottom + 7;
	rr.bottom = rr.top + 20;

	fKillButton = new BButton(rr, "kill", "Kill Application",
		new BMessage(CMD_KILL_APP));
	box->AddChild(fKillButton);
	fKillButton->ResizeToPreferred();
	fKillButton->SetEnabled(false);

	rr.OffsetBy(100, 0);

	/*
	 The Cancel button
	*/
	fCancelButton = new BButton(rr, "quit", "Cancel Shutdown",
		new BMessage(CMD_CANCEL_SHUTDOWN));
	box->AddChild(fCancelButton);
	fCancelButton->ResizeToPreferred();
	fCancelButton->SetEnabled(false);

	/*
	 Determine the wider button and use that for the narrowed. Also position
	 inside window along the bottom
	*/
	BRect	r1 = fKillButton->Bounds();
	BRect	r2 = fCancelButton->Bounds();
	const float	bspacing = 9.0;	// space between buttons & margins
	float	width;
	if (r1.Width() > r2.Width()) {
		width = r1.Width();
		fCancelButton->ResizeTo(width, r1.Height());
	} else {
		width = r2.Width();
		fKillButton->ResizeTo(width, r1.Height());
	}
	float window_w = (width*2) + (2*bspacing) + 50;
	ResizeTo(window_w, fCancelButton->Frame().bottom + 9);
	bounds = Bounds();

	fKillButton->MoveTo(bounds.right - bspacing - width - bspacing - width,
		fKillButton->Frame().top);
	fCancelButton->MoveTo(bounds.right - bspacing - width,
		fCancelButton->Frame().top);

	/*
	 The "reboot" button. Only visible if software shutdown doesn't work.
	*/
	fRebootButton = new BButton(rr, "reboot", "Reboot System",
		new BMessage(CMD_REBOOT));
	fRebootButton->Hide();
	box->AddChild(fRebootButton);
	fRebootButton->ResizeToPreferred();
	fRebootButton->MoveTo(
		(bounds.Width() - fRebootButton->Bounds().Width()) / 2.0,
		fCancelButton->Frame().top);

	BMessenger	me(this);
	BMessage	m(B_PULSE);
	fTimerToken = gTheRoster->Timer()->Add(me, m, me, 100000, -1);

	SetWorkspaces(B_ALL_WORKSPACES);

	fText->SetText("Tidying things up a bit...");

	Show();

	// do an extra sync here at the start of the process.
	sync();
}

/*------------------------------------------------------------*/

TShutdownWindow::~TShutdownWindow()
{
}


/*------------------------------------------------------------*/

bool TShutdownWindow::QuitRequested()
{
	shutdownRunning = false;
	gState->fShutdownLoc = Frame().LeftTop();	// save wd loc

	gTheRoster->Timer()->Remove(fTimerToken);
	return true;
}

/*------------------------------------------------------------*/

void TShutdownWindow::Cancel()
{
	fState = S_CANCEL;
	queue_action();
}

/*------------------------------------------------------------*/

void TShutdownWindow::Start()
{
	PostMessage(CMD_NEXT_STATE);
}

/*------------------------------------------------------------*/

void TShutdownWindow::next_state()
{
	int32 new_state = fState + 1;
//+	SERIAL_PRINT(("proposed new_state = %d\n", new_state));
	switch (new_state) {
		case S_IDLE:
//+			ASSERT(0);
			break;
		case S_SHUTDOWN_SCRIPT:
//+			PRINT(("next_state: S_SHUTDOWN_SCRIPT\n"));
			break;
		case S_QUIT_UI_APPS:
			// Prime the list with the list of UI apps.
			fKillButton->SetLabel("Kill Application");
			fCancelButton->SetEnabled(true);
			BuildAppList(fAppList, fSkipList, false);
			SERIAL_PRINT(("--- UI apps:%d ---\n", fAppList.CountItems()));
			break;
		case S_QUIT_UI_APPS_TEMP:
			// Once in this state we remain here until the AppList is empty.
			BuildAppList(fAppList, fSkipList, false);
			SERIAL_PRINT(("--- UI apps:%d ---\n", fAppList.CountItems()));
			if (fAppList.CountItems() != 0) {
				new_state = S_QUIT_UI_APPS;
				break;
			}
			new_state++;		// empty, so go to next state
			fCancelButton->SetEnabled(false);
			// falling through to next state
		case S_QUIT_ROSTER_APPS:
			// build ordered list of all apps.
			// the shutdown process can no longer 
			// be aborted starting with this state
			BuildAppList(fAppList, fSkipList, true);
			SERIAL_PRINT(("--- all BApps:%d ---\n", fAppList.CountItems()));
			break;
		case S_KILL_ALL_TEAMS:
			gState->fShutdownLoc = Frame().LeftTop();
			CleanupRegistrar();
			// build ordered list of all teams.
			BuildAppList(fAppList, fSkipList, true, false);
			SERIAL_PRINT(("--- all teams:%d ---\n", fAppList.CountItems()));
			break;
		case S_SHUTDOWN_FINISH_SCRIPT:
			SERIAL_PRINT(("next_state: S_SHUTDOWN_FINISH_SCRIPT\n"));
			break;
		case S_DONE:
			break;
	}
	fState = new_state;
//+	SERIAL_PRINT(("new_state = %d\n", fState));
	queue_action();
}

/*------------------------------------------------------------*/

bool is_blocked_by_modal(team_id tid, bool *in_cur_ws)
{
	int32		count;
	int32		*tokens;
	bool		is_blocked = false;
	int32		cur_ws = current_workspace();
	window_info	*wi = NULL;

	*in_cur_ws = true;

	tokens = get_token_list(tid, &count);
	if (!tokens) {
		return is_blocked;
	}
	
	for (int32 i = 0; i < count; i++) {
		wi = get_window_info(tokens[i]);

		if (wi == NULL) {
			break;
		}
//+		PRINT(("checking wd=%s\n", wi->name));

		if (wi->show_hide_level >= 1) {
			break;
		}

		if ((wi->w_type >= 1) && (wi->w_type <= 3)) {
			is_blocked = true;
			*in_cur_ws = ((wi->workspaces & (1<<cur_ws)) != 0);
//+			PRINT(("std, b=%d, c=%d\n", is_blocked, *in_cur_ws));
			break;
		}
		free(wi);
		wi = NULL;
	}

	if (wi)
		free(wi);

	free(tokens);

	return is_blocked;
}

/*------------------------------------------------------------*/

void TShutdownWindow::DispatchMessage(BMessage *msg, BHandler *target)
{
//+	if (msg->what == B_MOUSE_DOWN) {
//+		SERIAL_PRINT(("MD: target=%s\n", class_name(target)));
//+	}

	if (msg->what == B_PULSE) {
//+		PRINT(("pulse\n"));
		if (fCurTeam >= 0) {
			team_info	tinfo;
			if (get_team_info(fCurTeam, &tinfo) != B_OK) {
				// team is gone, so it must have quit
				queue_action();
			} else {
				bigtime_t	now = system_time();
				int32		sec;
				sec = (int32) ((now - fStartTime) / 1000000);
				if (sec > 1) {
					fIconView->Animate();

					bool is_blocked;
					bool cur_ws;
					is_blocked = is_blocked_by_modal(fCurTeam, &cur_ws);

#define MSG		" The app might be blocked on a modal panel."
					if (is_blocked != fBlockedByModal) {
						if (is_blocked) {
							if (!cur_ws) 
								fTop->SetZoom(true);
							int32	len = fText->TextLength();
							fText->Insert(len, MSG, strlen(MSG));
						} else {
							fTop->SetZoom(false);
							int32	len = fText->TextLength();
							int32	m = strlen(MSG);
							fText->Delete(len-m-1, len-1);
						}
						fBlockedByModal = is_blocked;
						fModalInCurWS = cur_ws;
					}

					if (!fKillButton->IsEnabled()) {
						fKillButton->SetEnabled(true);
					}
					if ((sec > 3) && (fState == S_KILL_ALL_TEAMS)) {
						thread_info	ti;
						int32		c = 0;
						/*
						 Some signal handling code in the Terminal changed
						 causing team launched inthe background to ignore
						 SIGHUP. So this code here starts sending _stronger_
						 signals trying to get the thing to quit.
						*/
						if (get_next_thread_info(fCurTeam, &c, &ti) == B_OK) {
							if (sec < 6) {
//+								SERIAL_PRINT(("Send team=%d SIGTERM signal\n",
//+									fCurTeam));
								kill(ti.thread, SIGTERM);
							} else {
//+								SERIAL_PRINT(("Send team=%d SIGKILL signal\n",
//+									fCurTeam));
								kill(ti.thread, SIGKILL);
							}
						}
					}
				}
			}
		} else if (fWaitThread >= 0) {
			bigtime_t	now = system_time();
			int32		sec;
			sec = (int32) ((now - fStartTime) / 1000000);
			if (sec > 1) {
				if (strcmp("", fText->Text()) == 0) {
					if (fState == S_SHUTDOWN_SCRIPT)
						fText->SetText("Waiting on some early processing...");
					else
						fText->SetText("Waiting on some final processing...");
					app_info	ainfo;
					if (RealRoster->GetRunningAppInfo(be_app->Team(),&ainfo)
						== B_OK) {
						BRect	rect(0, 0, B_LARGE_ICON-1, B_LARGE_ICON-1);
						BBitmap *icon = new BBitmap(rect, B_COLOR_8_BIT);
						if (icon && (BNodeInfo::GetTrackerIcon((const entry_ref *)&(ainfo.ref),
							icon) == B_OK)) {
								fIconView->SetBitmap(icon);
						}
					}
				}
				if ((sec > 5) && !fKillButton->IsEnabled()) {
					fKillButton->SetLabel("Kill Processing");
					fKillButton->SetEnabled(true);
				}

				fIconView->Animate();
			}
		}
	}
	BWindow::DispatchMessage(msg, target);
}

/*------------------------------------------------------------*/
void TShutdownWindow::MessageReceived(BMessage *msg)
{

	switch (msg->what) {
		case CMD_NEXT_STATE: {
			next_state();
			break;
		}
		
		case CMD_SCRIPT_DONE: {
			int32	result;
			msg->FindInt32("result", &result);
			SERIAL_PRINT(("Script done (%x)!!!\n", result));

			fStartTime = 0;
			fWaitThread = -1;
			fScriptThread = -1;

			if (result == B_OK) {
				next_state();
			} else
				PostMessage(CMD_CANCEL_SHUTDOWN);

			break;
		}
		
		case CMD_PROCESS: {
			do_process();
			break;
		}
		
		case B_CANCEL: {
			if (fState < S_QUIT_ROSTER_APPS) {
				SERIAL_PRINT(("received CANCEL from %d, cancelling shutdown...\n", fCurTeam));
				Cancel();
			}
			else {
				// can't abort shutdown anymore, add cur app to
				// the skip list, then move on to the next app
				SERIAL_PRINT(("received CANCEL from %d, continuing with shutdown!\n", fCurTeam));
				if (fCurTeam >= 0)
					fSkipList.AddItem((void *)fCurTeam);
				queue_action();
			}
			break;
		}

		case CMD_KILL_APP: {
			if (fCurTeam >= 0) {
				kill_team(fCurTeam);
				fSkipList.AddItem((void*)fCurTeam);
				queue_action();
			} else if (fWaitThread >= 0) {
				kill_thread(fWaitThread);
				kill_thread(fScriptThread);
				fWaitThread = -1;
				fScriptThread = -1;
				next_state();
			}

			break;
		}

		case CMD_SKIP_APP: {
			ASSERT(fCurTeam >= 0);
			fSkipList.AddItem((void*)fCurTeam);
			queue_action();
			break;
		}

		case CMD_CANCEL_SHUTDOWN: {
			if (fState <= S_QUIT_UI_APPS)
				PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case CMD_REBOOT: {
			for(;;)
				_kshutdown_(true);
			break;
		}

		case CMD_ACTIVATE: {
			if (fState <= S_QUIT_UI_APPS) {
//+				SERIAL_PRINT(("Activating %d\n", fCurTeam));
				RealRoster->ActivateApp(fCurTeam);
			}
			break;
		}

		default:
			_inherited::MessageReceived(msg);
			break;
	}
}

/*------------------------------------------------------------*/

void TShutdownWindow::do_process()
{
	switch (fState) {
		case S_IDLE: {
//+			ASSERT(0);
			break;
		}
		case S_SHUTDOWN_SCRIPT: {
			SERIAL_PRINT(("do_process: S_SHUTDOWN_SCRIPT\n"));

			status_t	err = exec_script("ShutdownScript");

			if (err) {
				SERIAL_TRACE();
				next_state();
			}
			// else we just wait for the script to finish

			break;
		}
		case S_QUIT_UI_APPS: {
			/*
			 In this state we build the list of UI apps (non-important)
			 And try to quit each one. If the list is empty we're done
			 with this state and we move on to the next.
			*/
			if (fAppList.CountItems() == 0) {
				next_state();
				break;
			}
			team_id team = (team_id) fAppList.RemoveItem((int32)0);
//+			SERIAL_PRINT(("quit_ui_app(%d)\n", team));
			quit_team(team);
			break;
		}
		case S_QUIT_ROSTER_APPS: {
			if (fAppList.CountItems() == 0) {
				next_state();
				break;
			}
			team_id team = (team_id) fAppList.RemoveItem((int32)0);
//+			SERIAL_PRINT(("quit_reg_app(%d)\n", team));
			quit_team(team);
			break;
		}
		case S_KILL_ALL_TEAMS: {
			if (fAppList.CountItems() == 0) {
				next_state();
				break;
			}
			team_id team = (team_id) fAppList.RemoveItem((int32)0);
			SERIAL_PRINT(("kill_all_app(%d)\n", team));
			quit_team(team, true);
			break;
		}
		case S_SHUTDOWN_FINISH_SCRIPT: {

			status_t	err = exec_script("ShutdownFinishScript");
			SERIAL_PRINT(("do_process: S_SHUTDOWN_FINISH_SCRIPT err=%x\n",err));

			if (err) {
				SERIAL_TRACE();
				next_state();
			}
			// else we just wait for the script to finish

			break;
		}
		case S_DONE: {
			sync();

			SetTitle("Shutdown Status");
			fText->SetText("");
			fCancelButton->Hide();
			fKillButton->Hide();

			UnmountAllVolumes();
			sync();

			snooze(500000);
			_kshutdown_(fIsReboot);
			fRebootButton->Show();
			SetDefaultButton(fRebootButton);
			fText->SetText("It's now safe to turn off the computer.");
			SetTitle("System is Shut Down");
			/*
			 Hmm... it didn't work!.. probably because the hardware
			 does not support soft power off. The destrcutor of this
			 window will bring up an alert.
			*/
//+			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case S_CANCEL: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
	}
}


/*------------------------------------------------------------*/

void TShutdownWindow::queue_action()
{
	Lock();
	if (fTop->Zoom())
		fTop->SetZoom(false);
	fBlockedByModal = fModalInCurWS = false;
	fText->SetText("");
	fIconView->Reset();
	if (fKillButton->IsEnabled())
		fKillButton->SetEnabled(false);
	fCurTeam = -1;
	fStartTime = 0;
	fSeconds = 0;
	Unlock();
	PostMessage(CMD_PROCESS);
}

/*------------------------------------------------------------*/

void TShutdownWindow::quit_team(team_id team, bool hard_way)
{
	team_info	tinfo;
	app_info	ainfo;
	bool		have_ainfo = false;
	status_t	err;
	bool		skipping;

	skipping = (fSkipList.IndexOf((void*) team) >= 0);
	if (skipping)
		goto died;

	if (!hard_way) {
		err = RealRoster->GetRunningAppInfo(team, &ainfo);
		have_ainfo = (err == B_OK);
	}

	err = get_team_info(team, &tinfo);
	if (err) {
		goto died;
	}

	SERIAL_PRINT(("quit_team(%d) (%s) h=%d, ainfo=%d\n",
		tinfo.team, tinfo.args, hard_way, have_ainfo));

	if (have_ainfo) {
		BMessenger	mess = RealRoster->MakeMessenger(NULL, team);
		BMessage	msg(B_QUIT_REQUESTED);
		msg.AddBool("_shutdown_", true);

		err = mess.SendMessage(&msg, this, QUIT_TIMEOUT);
		if (err == B_TIMED_OUT) {
			TRACE();
			// ??? what if port is full for SendMessage blocks???
		} else if (err) {
			// The app probably died, so just move on to the next app
			TRACE();
			goto died;
		} else {
			init_wait(&tinfo, &ainfo, true);
		}
	} else {
		/*
		 Apparently this isn't a UI app, so send it a signal
		*/
		thread_info	tt;
		int32		cookie = 0;
		if (get_next_thread_info(team, &cookie, &tt) != B_NO_ERROR)
			goto died;

		kill(tt.thread, SIGTERM);
		init_wait(&tinfo, NULL, false);
	}

	return;

died:
	queue_action();
	return;
}

/*------------------------------------------------------------*/

void TShutdownWindow::init_wait(team_info *tinfo, app_info *ainfo, bool)
{
	char name[B_FILE_NAME_LENGTH];
	Basename(tinfo->args, name);

	char *buf = (char *) malloc(strlen(name) + 100);
	if (buf) {
		sprintf(buf, "Asking \"%s\" to quit.", name);
		fText->SetText(buf);
		free(buf);
	} else
		fText->SetText(name);


	if (ainfo) {
		BRect	rect(0, 0, B_LARGE_ICON-1, B_LARGE_ICON-1);
		BBitmap *icon = new BBitmap(rect, B_COLOR_8_BIT);
		if (BNodeInfo::GetTrackerIcon((const entry_ref *)&(ainfo->ref), icon) == B_OK)
			fIconView->SetBitmap(icon);
	}

	fCurTeam = tinfo->team;
	fStartTime = system_time();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

TBitmapView::TBitmapView(BRect rect)
	: BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBitmap = fBitmapDark = NULL;
	SetDrawingMode(B_OP_OVER);
	fDivider = B_LARGE_ICON;
	fSense = false;
	fLastClick = 0;
	SetLowColor(B_TRANSPARENT_8_BIT, B_TRANSPARENT_8_BIT,
		B_TRANSPARENT_8_BIT);
	SetViewColor(B_TRANSPARENT_8_BIT, B_TRANSPARENT_8_BIT,
		B_TRANSPARENT_8_BIT);
}

/*------------------------------------------------------------*/

TBitmapView::~TBitmapView()
{
	delete fBitmap;
	delete fBitmapDark;
}

/*------------------------------------------------------------*/

void TBitmapView::MouseDown(BPoint)
{
	if (!fBitmap || !fBitmapDark)
		return;
	
	int32 clicks;
	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	if (clicks == 2) 
		Window()->PostMessage(CMD_ACTIVATE);
}

/*------------------------------------------------------------*/

void TBitmapView::Draw(BRect)
{
	if (!fBitmap || !fBitmapDark)
		return;
	
	BBitmap *b1;
	BBitmap *b2;

	if (fSense) {
		b1 = fBitmapDark;
		b2 = fBitmap;
	} else {
		b1 = fBitmap;
		b2 = fBitmapDark;
	}

	float div = fDivider;
	if (div < 0.0)
		div = 0.0;

	if (div == B_LARGE_ICON) {
//+		SERIAL_PRINT(("all light %.1f\n", div));
		DrawBitmap(b1, B_ORIGIN);
	} else if (div == 0.0) {
//+		SERIAL_PRINT(("all dark %.1f (%.1f)\n", div, fDivider));
		DrawBitmap(b2, B_ORIGIN);
	} else {
//+		SERIAL_PRINT(("split %.1f\n", div));
		BRect	r(0, 0, B_LARGE_ICON, div);
		DrawBitmap(b1, r, r);

		BRect r2(0, div+1.0, B_LARGE_ICON, B_LARGE_ICON);
		DrawBitmap(b2, r2, r2);
	}
	Sync();
}

/*------------------------------------------------------------*/

void TBitmapView::Reset()
{
	SetBitmap(NULL);
	fDivider = B_LARGE_ICON;
	fSense = false;
}

/*------------------------------------------------------------*/

void TBitmapView::Animate()
{
	if (!fBitmap || !fBitmapDark)
		return;

	fDivider -= 4.0;
	if (fDivider <= -4.0) {
		fDivider = B_LARGE_ICON;
		fSense = !fSense;
	}
	
	Draw(Bounds());
}

/*------------------------------------------------------------*/

void TBitmapView::SetBitmap(BBitmap *b)
{
	if (!b && !fBitmap)
		return;

	if (fBitmap) 
		delete fBitmap;
	if (fBitmapDark) {
		delete fBitmapDark;
		fBitmapDark = NULL;
	}
	fBitmap = b;

	if (fBitmap) {
		BRect	rect(0, 0, B_LARGE_ICON-1, B_LARGE_ICON-1);
		fBitmapDark = new BBitmap(rect, B_COLOR_8_BIT);
		MakeHilitedIcon(fBitmap, fBitmapDark);
	}

	Invalidate();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void SpawnShutdownTask(_TRoster_ *roster, bool isReboot)
{
	if (shutdownRunning)
		return;

	RealRoster = roster;
	MakeHiliteTable();

	BPoint	loc = gState->fShutdownLoc;

	// Let's make sure that the window will be visible
	BScreen	screen;
	BRect	sf = screen.Frame();

	if (!sf.Contains(loc+BPoint(260,90))) {
		// just force the window to default loc
		loc = BPoint(180, 50);
	}

	TShutdownWindow *w = new TShutdownWindow(BRect(loc, loc + BPoint(265, 93)),
		isReboot);
	w->Start();
	if (w->Thread() >= 0)
		shutdownRunning = true;
}

void
UnmountAllVolumes(void)
{
	long		cookie;
	fs_info		info;
	BPath		mount;

	cookie = 0;
//+	SERIAL_PRINT(("UnmountAllVolumes\n"));
	while (!_kstatfs_(-1, &cookie, -1, NULL, &info)) {
		BDirectory		dir;
		BEntry			entry;
		node_ref		node;

		node.device = info.dev;
		node.node = info.root;
		dir.SetTo(&node);
		dir.GetEntry(&entry);
		entry.GetPath(&mount);

//+		SERIAL_PRINT(("Roster-Unmount %s\n", mount.Path()));
		(void) unmount(mount.Path());
	}
}

/*------------------------------------------------------------*/

int32 TShutdownWindow::_wait_entry_(void *arg)
{
	status_t	result = B_OK;
	TShutdownWindow *w = (TShutdownWindow *) arg;
	wait_for_thread(w->fScriptThread, &result);

	if (w->fState == S_SHUTDOWN_FINISH_SCRIPT)
		result = B_OK;
	else if (w->fState == S_SHUTDOWN_SCRIPT && result != 0) {
		BAlert	*alert = new BAlert("",
			w->fIsReboot ?
			"The Shutdown script returned an error. Continue with Restart?" :
			"The Shutdown script returned an error. Continue with Shut Down?",
			"Cancel", "Continue", NULL, B_WIDTH_AS_USUAL,
			B_OFFSET_SPACING, B_STOP_ALERT);
		int32 i = alert->Go();
		if (i == 1) {
			result = B_OK;
		}
	}

	BMessage	msg(CMD_SCRIPT_DONE);
	msg.AddInt32("result", result);
	w->PostMessage(&msg);
	return 0;
}


/*------------------------------------------------------------*/

status_t TShutdownWindow::exec_script(const char *name)
{
	BPath		path;
	status_t	result = B_ERROR;

	if (find_directory(B_BEOS_BOOT_DIRECTORY, &path) == B_OK) {
		path.Append(name);
		
		BEntry	entry(path.Path());

		if (entry.Exists()) {
			const char *argv[3];
			argv[0] = "/bin/sh";
			argv[1] = path.Path();
			argv[2] = NULL;

			fScriptThread = load_image(2, argv, (const char **)environ);
			if (fScriptThread < 0)
				goto done;
			
			fWaitThread = spawn_thread(_wait_entry_, "wait",
				B_NORMAL_PRIORITY, this);
			if (fWaitThread < 0) {
				fScriptThread = -1;
				goto done;
			}
			result = resume_thread(fWaitThread);
			fStartTime = system_time();
		}
	}

done:

	return result;
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

TBox::TBox(BRect f)
	: BBox(f, "top", B_FOLLOW_ALL, B_WILL_DRAW | B_DRAW_ON_CHILDREN,
		B_PLAIN_BORDER), fDrawZoom(false), fZoomOnly(false)
{
}

/*------------------------------------------------------------*/

TBox::~TBox()
{
}

void TBox::SetZoom(bool state)
{
	fDrawZoom = state;
	fZoomOnly = true;
	Draw(ZoomArea());
	fZoomOnly = false;
};

/*------------------------------------------------------------*/

void TBox::AttachedToWindow()
{
	BBox::AttachedToWindow();

//+	SetLowColor(216,216,216);
//+	SetViewColor(B_TRANSPARENT_8_BIT, B_TRANSPARENT_8_BIT,
//+		B_TRANSPARENT_8_BIT);
}

/*------------------------------------------------------------*/

BRect TBox::ZoomArea()
{
	BRect	b = Bounds();
	b.right = b.left + 14;
	b.top += 20;
	b.bottom = b.top + 15;

	return b;
}

/*------------------------------------------------------------*/

void TBox::Draw(BRect update)
{
	const rgb_color kBlack = { 0, 0, 0, 255 };
	const rgb_color bg = LowColor();
	
	BRect	bounds = Bounds();
	BRect	b;
	
	SetHighColor(bg.blend_copy(kBlack, 40));
	if (fZoomOnly) {
		b = update;
		FillRect(b);
	} else {
		_inherited::Draw(update);
		
		SetHighColor(bg.blend_copy(kBlack, 40));
		b = bounds;
		b.right = b.left + 30;
		FillRect(b);

		SetHighColor(bg);

		StrokeLine(b.LeftTop() - BPoint(1,1), b.RightTop() - BPoint(0,1));

		b.top -= 1;
		b.left = b.right + 1;
		b.right = bounds.right;
		FillRect(b);
	}

	if (fDrawZoom) {
		BPoint	p = bounds.LeftTop() + BPoint(3, 20);
		SetHighColor(bg.blend_copy(kBlack, 160));
		StrokeLine(p + BPoint(2,2), p + BPoint(3,2));
		StrokeLine(p + BPoint(5,2), p + BPoint(8,2));

		StrokeLine(p + BPoint(0,5), p + BPoint(1,5));
		StrokeLine(p + BPoint(3,5), p + BPoint(8,5));

		StrokeLine(p + BPoint(1,8), p + BPoint(2,8));
		StrokeLine(p + BPoint(4,8), p + BPoint(8,8));
	}
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
