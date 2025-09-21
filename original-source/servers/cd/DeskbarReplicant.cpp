/*------------------------------------------------------------*/
// 0x370fc0
//	File:		DeskbarReplicant.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <MediaDefs.h>
#include <MediaTrack.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "DeskbarReplicant.h"
#include "MiniStatusWindow.h"


#define SHOW_RIP_OPTION		1
#define SHOW_SCAN_MENU		1

// strings that need to be localized
const char* kDESKBAR_CONTEXT_MENU	= "CD";
const char* kDESKBAR_VOLUME_MENU	= "Volume";
const char* kDESKBAR_MUTE			= "Mute";
const char* kDESKBAR_MUTE_ALL		= "Mute All Players";
const char* kDESKBAR_PLAYER			= "Player";
const char* kDESKBAR_PLAY			= "Play";
const char* kDESKBAR_RIP			= "Rip";
const char* kDESKBAR_TRACK			= "Track";
const char* kDESKBAR_PAUSE			= "Pause";
const char* kDESKBAR_STOP			= "Stop";
const char* kDESKBAR_EJECT			= "Open Tray";
const char* kDESKBAR_CLOSE			= "Close Tray";
const char* kDESKBAR_AUTO_PLAY		= "Auto Play";
const char* kDESKBAR_REPEAT			= "Repeat";
const char* kDESKBAR_SHUFFLE		= "Shuffle";
const char* kDESKBAR_OPEN_PLAYER	= "Open Player";
const char* kDESKBAR_LOOKUP			= "Internet Lookup";
const char* kDESKBAR_ALWAYS_LOOKUP	= "Auto Lookup";
const char* kDESKBAR_LOOKUP_NOW		= "Lookup Now";
const char* kDESKBAR_REMOVE			= "Remove";
const char* kDESKBAR_ABOUT			= "About"B_UTF8_ELLIPSIS;
const char* kDESKBAR_ABOUT_TEXT		= "by Robert Polic";
const char* kDESKBAR_ABOUT_CONFIRM	= "Big Deal";
const char* kFILE_FORMAT			= "File Format";
const char* kSAVE_DIR				= "Save Entire Disc to Directory"B_UTF8_ELLIPSIS;
const char* kSAVE_TRACK				= "Save Track as"B_UTF8_ELLIPSIS;
const char* kDIR_SELECT				= "Select";

#ifdef SHOW_SCAN_MENU
const char* kDESKBAR_SCAN			= "Scan";
const char* kDESKBAR_FORWARD		= "Forward";
const char* kDESKBAR_BACKWARD		= "Backward";
#endif

// strings that shouldn't be localized
const char*	kDESKBAR_ITEM_NAME		= "cd_daemon";


const unsigned char kAUDIO_CD_ICON[] = {
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x0e, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x0e, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0x00, 0x2f, 0xeb, 0x30, 0x00, 0xff, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff,
0x00, 0x30, 0x2a, 0x3f, 0x2d, 0x2f, 0x00, 0x00, 0x00, 0x19, 0x1a, 0x1d, 0x00, 0x00, 0x0f, 0xff,
0x00, 0x30, 0x2a, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0xe0, 0x19, 0x00, 0x0f, 0x0f,
0x00, 0x2f, 0x2d, 0x00, 0xeb, 0x2f, 0x2f, 0x00, 0x00, 0x15, 0x00, 0x19, 0x1a, 0x19, 0x00, 0x0f,
0xff, 0x00, 0x00, 0x30, 0x2a, 0x3f, 0x2d, 0x2f, 0x00, 0x00, 0x00, 0x1d, 0x1e, 0x1e, 0x00, 0x0f,
0xff, 0xff, 0x00, 0x30, 0x2a, 0x2b, 0x2c, 0x30, 0x00, 0x00, 0x19, 0x19, 0x19, 0x19, 0x00, 0x0f,
0xff, 0xff, 0x00, 0x30, 0x2c, 0x2d, 0x2c, 0x30, 0x00, 0x0a, 0x20, 0x1a, 0x19, 0x00, 0x0f, 0x0f,
0xff, 0xff, 0xff, 0x00, 0x30, 0x2f, 0x2f, 0x00, 0x0e, 0x1a, 0x34, 0x20, 0x00, 0x00, 0x0e, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x1d, 0x8b, 0x1e, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0xff, 0xff, 0xff};

const unsigned char	kDATA_CD_ICON[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0x00, 0x00, 0x1d, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1d, 0x00, 0x00, 0x0f, 0xff, 0xff,
0xff, 0x00, 0x1d, 0x1e, 0x1a, 0x1a, 0x18, 0x18, 0x18, 0x19, 0x19, 0x1e, 0x1e, 0x00, 0x0f, 0xff,
0x00, 0x60, 0x41, 0x1a, 0x3f, 0x14, 0x1d, 0x00, 0x1d, 0x14, 0x3f, 0x19, 0x41, 0x60, 0x00, 0x0f,
0x00, 0x60, 0x41, 0x44, 0xd8, 0x1e, 0x00, 0x3f, 0x00, 0x1e, 0xd8, 0x44, 0x41, 0x60, 0x00, 0x0f,
0x00, 0x60, 0x41, 0x44, 0x18, 0x14, 0x1e, 0x00, 0x1e, 0x14, 0x1d, 0x44, 0x41, 0x60, 0x00, 0x0f,
0x00, 0x1d, 0x1a, 0x19, 0x19, 0x3f, 0x18, 0x18, 0x18, 0x3f, 0x1d, 0x1a, 0x19, 0x1d, 0x00, 0x0f,
0xff, 0x00, 0x1d, 0x1e, 0x3f, 0x19, 0x19, 0x18, 0x18, 0x18, 0x3f, 0x1d, 0x1d, 0x00, 0x0f, 0x0f,
0xff, 0xff, 0x00, 0x00, 0x1d, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1d, 0x00, 0x00, 0x0f, 0x0f, 0xff,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint32	kOPEN_PLAYER = 'Plyr';


/*-----------------------------------------------------------------*/

extern "C" _EXPORT BView* instantiate_deskbar_item();
BView* instantiate_deskbar_item()
{
	return new DeskbarReplicant();
}

/*-----------------------------------------------------------------*/

void show_deskbar_icon(bool showIcon)
{
	BDeskbar	db;
	bool		exists = db.HasItem(kDESKBAR_ITEM_NAME);

	if ((showIcon) && (!exists))
	{
		int32		id;
		BRoster		roster;
		entry_ref	ref;

		roster.FindApp(kCD_DAEMON_NAME, &ref);
		db.AddItem(&ref, &id);
	}
	else if ((!showIcon) && (exists))
		db.RemoveItem(kDESKBAR_ITEM_NAME);
}


/*=================================================================*/

DeskbarReplicant::DeskbarReplicant()
	: BView(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
			kDESKBAR_ITEM_NAME, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	  fPlayerCount(0),
	  fBitmap(NULL),
	  fSaveTrackPanel(NULL),
	  fSaveDiscPanel(NULL),
	  fMiniStatusWindow(NULL)
{
	AddChild(new BDragger(BRect(-10, -10, -10, -10), this));
}

/*-----------------------------------------------------------------*/

DeskbarReplicant::DeskbarReplicant(BMessage *msg)
	: BView(msg),
	  fPlayerCount(0),
	  fBitmap(NULL),
	  fSaveTrackPanel(NULL),
	  fSaveDiscPanel(NULL),
	  fMiniStatusWindow(NULL)
{
}

/*-----------------------------------------------------------------*/

DeskbarReplicant::~DeskbarReplicant(void)
{
	delete fBitmap;
	if (fSaveTrackPanel)
	{
		fSaveTrackPanel->Window()->Lock();
		fSaveTrackPanel->Window()->ResizeBy(0, -fTrackPanelResize);
		fSaveTrackPanel->Window()->Unlock();
		delete fSaveTrackPanel;
		fSaveTrackPanel = NULL;
	}
	if (fSaveDiscPanel)
	{
		fSaveDiscPanel->Window()->Lock();
		fSaveDiscPanel->Window()->ResizeBy(0, -fDiscPanelResize);
		fSaveDiscPanel->Window()->Unlock();
		delete fSaveDiscPanel;
		fSaveDiscPanel = NULL;
	}
}

/*-----------------------------------------------------------------*/

status_t DeskbarReplicant::Archive(BMessage *msg, bool deep) const
{
	BView::Archive(msg, deep);
	msg->AddString("add_on", kCD_DAEMON_NAME);
	return B_NO_ERROR;
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::AttachedToWindow()
{
	BMessenger*	messenger;

	SetViewColor(Parent()->ViewColor());
	SetDrawingMode(B_OP_OVER);

	fBitmap = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
	fBitmap->SetBits((char *)kAUDIO_CD_ICON, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);

	fHoverPoint.x = fHoverPoint.y = -1;
	fHoverTime = 0;
	fHovering = false;

	messenger = new BMessenger(kCD_DAEMON_NAME);
	if (messenger->IsValid())
		messenger->SendMessage(eCDDaemonPlayerCount, this);
	else
		be_roster->Launch(kCD_DAEMON_NAME);
	delete messenger;
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::DetachedFromWindow()
{
	BMessenger* messenger = new BMessenger(kCD_DAEMON_NAME);
	if (messenger->IsValid())
		messenger->SendMessage(eCDDaemonQuit);
	delete messenger;
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::Draw(BRect )
{
	DrawBitmap(fBitmap);
}

/*-----------------------------------------------------------------*/

DeskbarReplicant* DeskbarReplicant::Instantiate(BMessage *msg)
{
	return new DeskbarReplicant(msg);
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case 'CODC':
			{
				media_file_format*	fileFormat;
				media_codec_info*	codecInfo;
				int32				size;

				msg->FindData("be:media_file_format", B_RAW_TYPE, (const void**)&fileFormat, &size);
				if (fRipMessage.HasData("be:media_file_format", B_RAW_TYPE))
					fRipMessage.ReplaceData("be:media_file_format", B_RAW_TYPE, fileFormat, size);
				else
					fRipMessage.AddData("be:media_file_format", B_RAW_TYPE, fileFormat, size);

				msg->FindData("be:media_codec_info", B_RAW_TYPE, (const void**)&codecInfo, &size);
				if (fRipMessage.HasData("be:media_codec_info", B_RAW_TYPE))
					fRipMessage.ReplaceData("be:media_codec_info", B_RAW_TYPE, fileFormat, size);
				else
					fRipMessage.AddData("be:media_codec_info", B_RAW_TYPE, fileFormat, size);

				if (fSaveTrackPanel)
					fSaveTrackPanel->SetMessage(&fRipMessage);
				if (fSaveDiscPanel)
					fSaveDiscPanel->SetMessage(&fRipMessage);
			}
			break;

		case eCDDaemonPlayerCount:
			{
				int32			loop;
				track_status	track;

				msg->FindInt32(kCD_DAEMON_PLAYER_COUNT, &fPlayerCount);
				track.status = ePlayerStatusEmpty;
				track.track = 0;
				for (loop = 0; loop < fPlayerCount; loop++)
					fTrack.insert(fTrack.end(), track);
			}
			break;

		case eCDDaemonPlayerData:
			{
				int32							dirty = 0;
				int32							index = 0;
				const cd_daemon_player_data*	player;
				track_status					track;
				ssize_t							size;

				fIter = fTrack.begin();
				while (msg->FindData(kCD_DAEMON_PLAYER_DATA, B_RAW_TYPE, index++, (const void**)&player, &size) == B_NO_ERROR)
				{
					if (player->status != ePlayerStatusUnknown)
					{
						track = *fIter;
						if ((((track.status == ePlayerStatusEmpty) || (track.status == ePlayerStatusDoorOpen)) &&
							((player->status != ePlayerStatusEmpty) && (player->status != ePlayerStatusDoorOpen)) &&
							 (msg->HasString(kCD_DAEMON_PLAYER_TRACK_LIST))) ||
							((player->status == ePlayerStatusPlaying) && (track.track != player->current_track)))
							dirty++;
						track.status = player->status;
						track.track = player->current_track;
						*fIter = track;
					}
					fIter++;
				}
				if (dirty)
					ShowStatusWindow(4);
			}
			break;

		default:
			BView::MessageReceived(msg);
	}
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::MouseDown(BPoint	where)
{
	CloseStatusWindow();

	BWindow*	window = Window();
	if (window == NULL)
		return;
	BMessage*	currentMsg = window->CurrentMessage();
	if (currentMsg == NULL)
		return;

	if (currentMsg->what == B_MOUSE_DOWN)
	{
		uint32	buttons = 0;
		uint32	clickCount = 0;
		uint32	modifiers = 0;

		currentMsg->FindInt32("buttons", (int32 *)&buttons);
		currentMsg->FindInt32("modifiers", (int32 *)&modifiers);

		if ((buttons & B_SECONDARY_MOUSE_BUTTON) || (modifiers & B_CONTROL_KEY))
		{
			// secondary button was clicked or control key was down, show menu and return
			ShowContextMenu(where, modifiers);
			return;
		}

		currentMsg->FindInt32("clicks", (int32 *)&clickCount);
		if (clickCount > 1)
			// double (or more) click
			be_roster->Launch("audio/x-cdda");
		else
		{
			// first click
			BPoint		saveWhere = where;
			bigtime_t	dblSpeed = 0;
			bigtime_t	start = system_time();

			get_click_speed(&dblSpeed);

			do
			{
				if ((fabs(where.x - saveWhere.x) > 4) || 
					(fabs(where.y - saveWhere.y) > 4))
					break;
				if ((system_time() - start) > (2 * dblSpeed))
				{
					ShowContextMenu(where, modifiers);
					break;
				}
				snooze(10000);
				GetMouse(&where, &buttons);
			} while (buttons);
		}
	}
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::MouseMoved(BPoint where, uint32 code, const BMessage* msg)
{
	switch (code)
	{
		case B_ENTERED_VIEW:
			fHoverPoint = where;
			fHoverTime = time(NULL);
			fHovering = true;
			break;

		case B_INSIDE_VIEW:
			if (fHovering)
			{
				if (where != fHoverPoint)
				{
					fHoverTime = time(NULL);
					fHoverPoint = where;
				}
			}
			break;

		case B_EXITED_VIEW:
			CloseStatusWindow();
			fHovering = false;
			break;

		default:
			BView::MouseMoved(where, code, msg);
			break;
	}
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::Pulse()
{
	uint32	buttons;
	BPoint	where;

	GetMouse(&where, &buttons);
	if ((fHovering) && (fHoverPoint == where) && ((fHoverTime < time(NULL))))
		ShowStatusWindow(4);
	if (!Bounds().Contains(where))
		fMiniStatusWindow = NULL;

	if (!fPlayerCount)
	{
		BMessenger* messenger = new BMessenger(kCD_DAEMON_NAME);
		if (messenger->IsValid())
			messenger->SendMessage(eCDDaemonPlayerCount, this);
		else
			be_roster->Launch(kCD_DAEMON_NAME);
		delete messenger;
	}
	else
	{
		BMessenger* messenger = new BMessenger(kCD_DAEMON_NAME);
		if (messenger->IsValid())
			messenger->SendMessage(eCDDaemonPlayerData, this);
	}

	BView::Pulse();
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::BuildCODECMenu(BPopUpMenu* menu)
{
	bool				firstItem = true;
	int32				fileFormatCookie = 0;
	media_file_format	fileFormat;

	while(get_next_file_format(&fileFormatCookie, &fileFormat) == B_OK)
	{
		int32				encoderCookie;
		media_format		inFormat;
		media_format		outFormat;
		media_codec_info	codecInfo;
		
		// If this file format allow video, exclude it from the list.
		if (fileFormat.capabilities & (media_file_format::B_KNOWS_RAW_VIDEO |
				media_file_format::B_KNOWS_ENCODED_VIDEO))
			continue;

		// Add audio encoders to the list
		inFormat.type = B_MEDIA_RAW_AUDIO;
		inFormat.u.raw_audio = media_raw_audio_format::wildcard;
		//	only do encoders that can take what we've got
		inFormat.u.raw_audio.frame_rate = 44100.0;
		inFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		inFormat.u.raw_audio.channel_count = 2;
		encoderCookie = 0;
		while (get_next_encoder(&encoderCookie, &fileFormat, &inFormat, &outFormat, &codecInfo) == B_OK)
		{
			BString	name;

			name << fileFormat.short_name << "/" << codecInfo.pretty_name;
			BMessage *message = new BMessage('CODC');
			message->AddData("be:media_file_format", B_RAW_TYPE, (const void*)
				&fileFormat, sizeof(fileFormat));
			message->AddData("be:media_codec_info", B_RAW_TYPE, (const void*)
				&codecInfo, sizeof(codecInfo));
			BMenuItem *item = new BMenuItem(name.String(), message);
			item->SetTarget(this);
			menu->AddItem(item);
			
			if (firstItem)
			{
				firstItem = false;
				item->SetMarked(true);
			}
		}
	}
}

/*-----------------------------------------------------------------*/

BPopUpMenu* DeskbarReplicant::BuildMenu(BMessage* msg, cd_daemon_preferences_data* preferences, uint32 modifiers)
{
	bool							have_a_disk = false;
	const char*						tracks;
	int32							index = 0;
	const cd_daemon_player_data*	player;
	BMenuItem*						item;
	BMessage*						m;
	BPopUpMenu*						menu = new BPopUpMenu(kDESKBAR_CONTEXT_MENU, false, false);
	ssize_t							size;

	menu->SetFont(be_plain_font);

	m = new BMessage(eCDDaemonPlayerMuteAll);
	m->AddBool(kCD_DAEMON_PLAYER_MUTE, !preferences->muted);
	menu->AddItem(item = new BMenuItem((fPlayerCount > 1) ? kDESKBAR_MUTE_ALL : kDESKBAR_MUTE, m));
	item->SetMarked(preferences->muted);
	menu->AddSeparatorItem();

	while (msg->FindData(kCD_DAEMON_PLAYER_DATA, B_RAW_TYPE, index, (const void**)&player, &size) == B_NO_ERROR)
	{
		bool		no_disc = ((player->status == ePlayerStatusUnknown) || (player->status == ePlayerStatusEmpty) || (player->status == ePlayerStatusDoorOpen));
		int32		loop;
		BMenu*		device = menu;
		BMenu*		volume = new BMenu(kDESKBAR_VOLUME_MENU);

		if (fPlayerCount > 1)
		{
			char	str[32];

			sprintf(str, "%s %d", kDESKBAR_PLAYER, (int)index + 1);
			device = new BMenu(str);
			menu->AddItem(device);
		}

		tracks = NULL;
		msg->FindString(kCD_DAEMON_PLAYER_TRACK_LIST, index, &tracks);
		if (strlen(tracks) == 0)
			tracks = NULL;

		if (no_disc)
		{
			device->AddItem(item = new BMenuItem(kDESKBAR_PLAY, new BMessage('NULL')));
			item->SetEnabled(false);
		}
		else if (tracks)
		{
			const char*	list = tracks;
			int32		offset = 0;
#ifdef SHOW_RIP_OPTION
			BMenu*		play = new BMenu((modifiers & B_OPTION_KEY) ? kDESKBAR_RIP : kDESKBAR_PLAY);
#else
			BMenu*		play = new BMenu(kDESKBAR_PLAY);
#endif

			for (loop = 0; loop <= player->last_track; loop++)
			{
				char*		str;

				while (list[offset] != '\n')
					offset++;

				str = (char *)malloc(offset + 1 + 10);
				strncpy(str, list, offset);
				str[offset] = 0;
#ifdef SHOW_RIP_OPTION
				m = new BMessage((modifiers & B_OPTION_KEY) ? 'RIP!' : eCDDaemonPlayerPlay);
#else
				m = new BMessage(eCDDaemonPlayerPlay);
#endif
				m->AddInt32(kCD_DAEMON_PLAYER_TRACK, loop);
				m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
				m->AddString("track_name", str);
				if (player->last_track < 10)
					strcat(str, "  ");
				else if (player->last_track < 100)
					strcat(str, "    ");
				else
					strcat(str, "      ");
				if (loop)
					play->AddItem(item = new PlayMenuItem(str, m, loop, player->last_track));
				else
					play->AddItem(item = new BMenuItem(str, m));

				if ((!loop) || (IsData(&player->TOC, loop)))
				{
#ifdef SHOW_RIP_OPTION
					if (!(modifiers & B_OPTION_KEY))
#endif
						item->SetEnabled(false);
					if (!loop)
						play->AddSeparatorItem();
				}
				else if (player->current_track == loop)
					item->SetMarked(true);
				free(str);
				list += offset + 1;
				offset = 0;
			}
			m = new BMessage(eCDDaemonPlayerPlay);
			m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
#ifdef SHOW_RIP_OPTION
			if (modifiers & B_OPTION_KEY)
				device->AddItem(new BMenuItem(play));
			else
#endif
			device->AddItem(new BMenuItem(play, m));
			have_a_disk = true;
		}
		else
		{
#ifdef SHOW_RIP_OPTION
			BMenu*		play = new BMenu((modifiers & B_OPTION_KEY) ? kDESKBAR_RIP : kDESKBAR_PLAY);
#else
			BMenu*		play = new BMenu(kDESKBAR_PLAY);
#endif

			for (loop = player->first_track; loop <= player->last_track; loop++)
			{
				char		str[32];

#ifdef SHOW_RIP_OPTION
				m = new BMessage((modifiers & B_OPTION_KEY) ? 'RIP!' : eCDDaemonPlayerPlay);
#else
				m = new BMessage(eCDDaemonPlayerPlay);
#endif
				m->AddInt32(kCD_DAEMON_PLAYER_TRACK, loop);
				m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
				sprintf(str, "%s %d", kDESKBAR_TRACK, (int)loop);
				m->AddString("track_name", str);
				play->AddItem(item = new BMenuItem(str, m));
				if (IsData(&player->TOC, loop))
				{
					item->SetEnabled(false);
					sprintf(str, "%s %d (DATA)", kDESKBAR_TRACK, (int)loop);
					item->SetLabel(str);
				}
				else if (player->current_track == loop)
					item->SetMarked(true);
			}
			m = new BMessage(eCDDaemonPlayerPlay);
			m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
#ifdef SHOW_RIP_OPTION
			if (modifiers & B_OPTION_KEY)
				device->AddItem(new BMenuItem(play));
			else
#endif
				device->AddItem(new BMenuItem(play, m));
			have_a_disk = true;
		}

		m = new BMessage(eCDDaemonPlayerPause);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(item = new BMenuItem(kDESKBAR_PAUSE, m));
		if ((no_disc) || (player->status == ePlayerStatusStopped))
			item->SetEnabled(false);
		else if (player->status == ePlayerStatusPaused)
			item->SetMarked(true);

#ifdef SHOW_SCAN_MENU
		{
			BMenu*	scan = new BMenu(kDESKBAR_SCAN);
			if (no_disc)
				scan->SetEnabled(false);

			m = new BMessage(eCDDaemonPlayerScanFWD);
			m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
			scan->AddItem(item = new ScanMenuItem(kDESKBAR_FORWARD, m));

			m = new BMessage(eCDDaemonPlayerScanBCK);
			m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
			scan->AddItem(item = new ScanMenuItem(kDESKBAR_BACKWARD, m));
			device->AddItem(scan);
		}
#endif

		m = new BMessage(eCDDaemonPlayerStop);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(item = new BMenuItem(kDESKBAR_STOP, m));
		if ((no_disc) || (player->status == ePlayerStatusStopped))
			item->SetEnabled(false);

		m = new BMessage(eCDDaemonPlayerEject);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(new BMenuItem((player->status == ePlayerStatusDoorOpen) ? kDESKBAR_CLOSE : kDESKBAR_EJECT, m));

		bool	marked = false;
		for (loop = 0; loop <= 9; loop++)
		{
			char	str[10];

			m = new BMessage(eCDDaemonPlayerVolume);
			m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
			m->AddInt32(kCD_DAEMON_PLAYER_VOLUME, (int32)(255 * (float)loop / 9.0));
			sprintf(str, "%d", (int)loop + 1);
			volume->AddItem(item = new BMenuItem(str, m));
			if ((!marked) && (player->volume < (int32)(255 * (float)(loop + 1) / 9.0)))
			{
				item->SetMarked(true);
				marked = true;
			}
		}
		device->AddItem(volume);

		device->AddSeparatorItem();
		m = new BMessage(eCDDaemonPlayerAutoPlay);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(item = new BMenuItem(kDESKBAR_AUTO_PLAY, m));
		item->SetMarked(player->auto_play);

		m = new BMessage(eCDDaemonPlayerRepeatMode);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(item = new BMenuItem(kDESKBAR_REPEAT, m));
		item->SetMarked(player->repeat);

		m = new BMessage(eCDDaemonPlayerShuffleMode);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(item = new BMenuItem(kDESKBAR_SHUFFLE, m));
		item->SetMarked(player->shuffle);

		device->AddSeparatorItem();

		m = new BMessage(kOPEN_PLAYER);
		m->AddString(kCD_DAEMON_PLAYER_NAME, player->name);
		device->AddItem(new BMenuItem(kDESKBAR_OPEN_PLAYER, m));

		index++;
	}
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(kDESKBAR_ABOUT, new BMessage('RMP ')));

	BMenu*	lookup = new BMenu(kDESKBAR_LOOKUP);
	lookup->AddItem(item = new BMenuItem(kDESKBAR_ALWAYS_LOOKUP, new BMessage(eCDDaemonSetPreferenceData)));
	if (preferences->lookup)
		item->SetMarked(true);
	lookup->AddItem(item = new BMenuItem(kDESKBAR_LOOKUP_NOW, new BMessage(eCDDaemonPlayerLookupNow)));
	item->SetEnabled(have_a_disk);
	menu->AddItem(lookup);
	menu->AddItem(new BMenuItem(kDESKBAR_REMOVE, new BMessage(eCDDaemonQuit)));
	return menu;
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::CloseStatusWindow()
{
	if (fMiniStatusWindow)
	{
		fMiniStatusWindow->PostMessage('clos');
		fMiniStatusWindow = NULL;
	}
}

/*-----------------------------------------------------------------*/

DirPanel* DeskbarReplicant::CreateDiscPanel(BMessenger* messenger)
{
	BMenuField*	field;
	BRect		r;
	BWindow*	save_wind;
	BView*		cancel_button;
	BView*		count_view;
	BView*		h_scroll_view;
	BView*		pose_view;
	BView*		save_button;
	BView*		save_view;
	BView*		select_button;
	BView*		v_scroll_view;
	DirPanel*	panel;

	fDiscCodecMenu = new BPopUpMenu("Format");
	BuildCODECMenu(fDiscCodecMenu);
	fDiscCodecMenu->SetRadioMode(true);

	panel = new DirPanel(messenger, NULL, NULL, &fDirFilter);
	save_wind = panel->Window();
	save_wind->Lock();
	save_view = save_wind->ChildAt(0);

	cancel_button = save_view->FindView("cancel button");
	save_button = save_view->FindView("default button");
	select_button = save_view->FindView("select_button");
	count_view = save_view->FindView("CountVw");
	pose_view = save_view->FindView("PoseView");
	h_scroll_view = save_view->FindView("HScrollBar");
	v_scroll_view = save_view->FindView("VScrollBar");

	r = cancel_button->Frame();
	r.left = count_view->Frame().left;
	r.right -= 10;
	save_view->RemoveChild(select_button);
	save_view->RemoveChild(cancel_button);
	save_view->RemoveChild(save_button);
	save_view->AddChild(field = new BMenuField(r, "format", kFILE_FORMAT, fDiscCodecMenu));
	field->SetDivider(0);
	save_view->AddChild(save_button);
	save_view->AddChild(cancel_button);
	save_view->AddChild(select_button);

	r = field->Frame();
	fDiscPanelResize = select_button->Frame().top - count_view->Frame().bottom + r.Height();
	save_wind->ResizeBy(0, fDiscPanelResize);

	pose_view->ResizeBy(0, -fDiscPanelResize);
	v_scroll_view->ResizeBy(0, -fDiscPanelResize);
	h_scroll_view->MoveBy(0, -fDiscPanelResize);
	count_view->MoveBy(0, -fDiscPanelResize);
	field->SetResizingMode(B_FOLLOW_BOTTOM);

	save_wind->SetTitle(kSAVE_DIR);
	save_wind->Unlock();

	return panel;
}

/*-----------------------------------------------------------------*/

BFilePanel* DeskbarReplicant::CreateTrackPanel(BMessenger* messenger)
{
	float		margin;
	BFilePanel*	panel;
	BMenuField*	field;
	BRect		r;
	BWindow*	save_wind;
	BView*		cancel_button;
	BView*		count_view;
	BView*		h_scroll_view;
	BView*		pose_view;
	BView*		save_button;
	BView*		save_view;
	BView*		text_view;
	BView*		v_scroll_view;

	fTrackCodecMenu = new BPopUpMenu("Format");
	BuildCODECMenu(fTrackCodecMenu);
	fTrackCodecMenu->SetRadioMode(true);
	fTrackCodecMenu->SetLabelFromMarked(true);

	panel = new BFilePanel(B_SAVE_PANEL, messenger);
	save_wind = panel->Window();
	save_wind->Lock();
	save_view = save_wind->ChildAt(0);

	cancel_button = save_view->FindView("cancel button");
	save_button = save_view->FindView("default button");
	count_view = save_view->FindView("CountVw");
	pose_view = save_view->FindView("PoseView");
	text_view = save_view->FindView("text view");
	h_scroll_view = save_view->FindView("HScrollBar");
	v_scroll_view = save_view->FindView("VScrollBar");

	r = text_view->Frame();
	margin = save_view->Bounds().bottom - r.bottom;
	r.top = r.bottom + (r.top - count_view->Frame().bottom);
	r.bottom = r.top + fTrackCodecMenu->Bounds().Height();
	r.right = cancel_button->Frame().left - margin;
	save_view->RemoveChild(cancel_button);
	save_view->RemoveChild(save_button);
	save_view->AddChild(field = new BMenuField(r, "format", kFILE_FORMAT, fTrackCodecMenu));
	field->SetDivider(0);
	save_view->AddChild(save_button);
	save_view->AddChild(cancel_button);

	r = field->Frame();
	fTrackPanelResize = r.bottom + margin - save_view->Frame().bottom;
	save_wind->ResizeBy(0, fTrackPanelResize);

	pose_view->ResizeBy(0, -fTrackPanelResize);
	v_scroll_view->ResizeBy(0, -fTrackPanelResize);
	h_scroll_view->MoveBy(0, -fTrackPanelResize);
	count_view->MoveBy(0, -fTrackPanelResize);
	text_view->MoveBy(0, -fTrackPanelResize);
	field->SetResizingMode(B_FOLLOW_BOTTOM);

	save_wind->SetTitle(kSAVE_TRACK);
	save_wind->Unlock();

	return panel;
}

/*-----------------------------------------------------------------*/

bool DeskbarReplicant::IsData(const scsi_toc* TOC, int32 track)
{
	return TOC->toc_data[((track - TOC->toc_data[2]) * 8) + 4 + 1] & 4;
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::ShowContextMenu(BPoint where, uint32 modifiers)
{
	if (fPlayerCount)
	{
		BMessenger	*messenger;

		messenger = new BMessenger(kCD_DAEMON_NAME);
		if (messenger->IsValid())
		{
			BMessage	msg;
			if (messenger->SendMessage(eCDDaemonPlayerData, &msg) == B_NO_ERROR)
			{
				BMessage					msg_preferences;
				cd_daemon_preferences_data*	preferences = NULL;
				ssize_t						size;

				if (messenger->SendMessage(eCDDaemonPreferenceData, &msg_preferences) == B_NO_ERROR)
					msg_preferences.FindData(kCD_DAEMON_PREFERENCE_DATA, B_RAW_TYPE, (const void**)&preferences, &size);
				BPopUpMenu*	menu = BuildMenu(&msg, preferences, modifiers);

				where = ConvertToScreen(where);
				BMenuItem *selected = menu->Go(where, true, true,
					BRect(where - BPoint(5, 5), where + BPoint(5, 5)));

				if ((selected != NULL) && (selected->Message() != NULL))
				{
					switch (selected->Message()->what)
					{
						case kOPEN_PLAYER:
							be_roster->Launch("audio/x-cdda");
							break;

						case 'RIP!':
							{
								int32		track;
								BMenuItem*	item = NULL;

								selected->Message()->FindInt32("track", &track);
								if (track)
								{
									const char*	name;

									if (!fSaveTrackPanel)
										fSaveTrackPanel = CreateTrackPanel(messenger);
									item = fTrackCodecMenu->FindMarked();
									Window()->PostMessage(item->Message(), this);

									selected->Message()->FindString("track_name", &name);
									fSaveTrackPanel->SetSaveText(name);
									fRipMessage = *(selected->Message());
									fRipMessage.what = eCDDaemonPlayerRip;
									fSaveTrackPanel->SetMessage(&fRipMessage);
									fSaveTrackPanel->Window()->Show();
								}
								else
								{
									if (!fSaveDiscPanel)
										fSaveDiscPanel = CreateDiscPanel(messenger);
									item = fDiscCodecMenu->FindMarked();
									Window()->PostMessage(item->Message(), this);
									fRipMessage = *(selected->Message());
									fRipMessage.what = eCDDaemonPlayerRip;
									fSaveDiscPanel->SetMessage(&fRipMessage);
									fSaveDiscPanel->Window()->Show();
								}
							}
							break;

						case eCDDaemonSetPreferenceData:
							if (!preferences)
								break;
							else
							{
								cd_daemon_preferences_data	data;

								data = *preferences;
								data.lookup = !data.lookup;
								selected->Message()->AddData(kCD_DAEMON_PREFERENCE_DATA, B_RAW_TYPE, &data, sizeof(cd_daemon_preferences_data));
							}
							// fall through
						case eCDDaemonQuit:
						case eCDDaemonPlayerPlay:
						case eCDDaemonPlayerRip:
						case eCDDaemonPlayerPause:
						case eCDDaemonPlayerStop:
						case eCDDaemonPlayerVolume:
						case eCDDaemonPlayerMuteAll:
						case eCDDaemonPlayerEject:
						case eCDDaemonPlayerAutoPlay:
						case eCDDaemonPlayerRepeatMode:
						case eCDDaemonPlayerShuffleMode:
						case eCDDaemonPlayerLookupNow:
							messenger->SendMessage(selected->Message());
							break;

						case 'RMP ':
							resume_thread(spawn_thread(About, "about", B_NORMAL_PRIORITY, NULL));
							break;
					}
				}
				delete menu;
			}
		}
		delete messenger;
	}
}

/*-----------------------------------------------------------------*/

void DeskbarReplicant::ShowStatusWindow(int32 length)
{
	if (fMiniStatusWindow)
		fMiniStatusWindow->Activate();
	else
		fMiniStatusWindow = new TMiniStatusWindow(this, length);
}

/*-----------------------------------------------------------------*/

status_t About(void* )
{
	BString	about(kDESKBAR_ITEM_NAME);
	about << "\n" << kDESKBAR_ABOUT_TEXT;

	(new BAlert("", about.String(), kDESKBAR_ABOUT_CONFIRM))->Go();
	return B_NO_ERROR;
}


/*=================================================================*/
/*
 *	Silly menuitem object just to align track numbers
*/

PlayMenuItem::PlayMenuItem(const char* label, BMessage* msg, int32 item, int32 items)
	: BMenuItem(label, msg),
	  fOffset(0),
	  fItem(item),
	  fItems(items)
{
}

/*-----------------------------------------------------------------*/

void PlayMenuItem::Draw()
{
	char		item[16];

	if (!fOffset)
	{
		font_height	finfo;

		fView = Menu();
		fView->GetFontHeight(&finfo);
		fAscent = finfo.ascent;
		if (fItems < 9)
			fOffset = fView->StringWidth("0. ");
		else if (fItems < 99)
			fOffset = fView->StringWidth("00. ");
		else
			fOffset = fView->StringWidth("000. ");
	}

	if (IsSelected())
	{
		fView->SetHighColor(152, 152, 152, 255);
		fView->FillRect(Frame());
	}

	if (!IsEnabled())
	{
		if (IsSelected())
			fView->SetHighColor(96, 96, 96, 255);
		else
			fView->SetHighColor(128, 128, 128, 255);
	}
	else
		fView->SetHighColor(0, 0, 0, 255);

	sprintf(item, "%d. ", (int)fItem);
	fView->MovePenTo(ContentLocation().x + fOffset - fView->StringWidth(item),
					 ContentLocation().y + fAscent);
	fView->SetDrawingMode(B_OP_COPY);
	fView->DrawString(item);

	fView->MovePenTo(ContentLocation().x + fOffset, ContentLocation().y);
	DrawContent();

	if (IsMarked())
	{
		BPoint startp(6, Frame().bottom - ((Frame().Height() - 10) / 2.0));
		rgb_color mark_color = {96, 96, 96, 255};

		BPoint	p;

		rgb_color c = {184, 184, 184, 255};
		fView->BeginLineArray(6);

		// draw the shadow
		p = startp;
		fView->AddLine(p, p + BPoint(4, -9), c);
		p.x += 1;
		fView->AddLine(p, p + BPoint(4, -9), c);

		// draw the right side of the check
		p = startp;
		p.y -= 1;
		fView->AddLine(p, p + BPoint(4, -9), mark_color);
		p.x -= 1;
		fView->AddLine(p, p + BPoint(4, -9), mark_color);

		// draw the left side of the check
		p.y += 1;
		fView->AddLine(p, p + BPoint(-2, -6), mark_color);
		p.y -= 1;

		p.x -= 1;
		fView->AddLine(p, p + BPoint(-2, -5), mark_color);

		fView->EndLineArray();
	}
}


/*=================================================================*/
/*
 * Menuitem object to scan a cd while mouse is over the item
*/

ScanMenuItem::ScanMenuItem(const char* label, BMessage* msg)
	: BMenuItem(label, msg),
	fScanning(false),
	fScanMsg(*msg),
	fStopScanMsg(*msg)
{
	fStopScanMsg.what = eCDDaemonPlayerStopScan;
}

/*-----------------------------------------------------------------*/

ScanMenuItem::~ScanMenuItem()
{
	if (fScanning)
		Scan(false);
}

/*-----------------------------------------------------------------*/

void ScanMenuItem::Draw()
{
	Scan(IsSelected());
	BMenuItem::Draw();
}

/*-----------------------------------------------------------------*/

void ScanMenuItem::Scan(bool scan)
{
	BMessage*	msg = NULL;

	if ((scan) && (!fScanning))
	{
		fScanning = true;
		msg = &fScanMsg;
	}
	else if ((!scan) && (fScanning))
	{
		fScanning = false;
		msg = &fStopScanMsg;
	}

	if (msg)
	{
		BMessenger*	messenger;
		messenger = new BMessenger(kCD_DAEMON_NAME);
		if (messenger->IsValid())
			messenger->SendMessage(msg);
		delete messenger;
	}
}


/*=================================================================*/

SelectButton::SelectButton(BFilePanel* panel, BRect frame, const char* name,
						   const char* label, BMessage* msg, uint32 resizing,
						   uint32 flags)
	: BButton(frame, name, label, msg, resizing, flags),
	  fPanel(panel)
{
}

/*-----------------------------------------------------------------*/

status_t SelectButton::Invoke(BMessage* msg)
{
	if (fPanel->HidesWhenDone())
		fPanel->Hide();
	return BButton::Invoke(msg);
}

/*-----------------------------------------------------------------*/

void SelectButton::SetLabel(const char* text)
{
	BRect	rect = Bounds();
	char	buffer[B_FILE_NAME_LENGTH + 15];
	char	trunc[B_FILE_NAME_LENGTH + 15];

	sprintf(buffer, "%s '%s'", kDIR_SELECT, text);

	if (StringWidth(buffer) > rect.Width() - 10)
	{
		const char*	src[1];
		char*		results[1];
		float		fixed = StringWidth(" ''") + StringWidth(kDIR_SELECT);
		BFont		font;

		GetFont(&font);

		src[0] = text;
		results[0] = trunc;
	    font.GetTruncatedStrings(src, 1, B_TRUNCATE_END, rect.Width() - fixed - 10, results);
		text = trunc;

		sprintf(buffer, "%s '%s'", kDIR_SELECT, trunc);
	}
	BButton::SetLabel(buffer);
}


/*=================================================================*/

DirPanel::DirPanel(BMessenger* target, entry_ref* dir, BMessage* msg, BRefFilter* rf)
	: BFilePanel(B_OPEN_PANEL, target, dir, B_DIRECTORY_NODE, false, msg, rf, false, true)
{
	BWindow* w = Window();
	if (w->Lock())
	{
		SetButtonLabel(B_DEFAULT_BUTTON, kDIR_SELECT);
		BView* panelback = w->ChildAt(0);

		// create new 'select this' button
		BRect clone = panelback->FindView("cancel button")->Frame();
		clone.left -= 180;
		clone.right -= 80;
		fSelectButton = new SelectButton(this, clone, "select_button", "", 0, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		panelback->AddChild(fSelectButton);
		fSelectButton->SetTarget(target ? *target : be_app_messenger);

		// set button text
		entry_ref	ref;
		if (dir)
			ref = *dir;
		else
			GetPanelDirectory(&ref);
		SetDir(&ref, msg);

		w->Unlock();
	}
}

/*-----------------------------------------------------------------*/

void DirPanel::SetPanelDirectory(entry_ref* dir)
{
	BFilePanel::SetPanelDirectory(dir);
	BWindow* w = Window();
	if (w->Lock())
	{
		SetDir(dir, fSelectButton->Message());
		w->Unlock();
	}
}

/*-----------------------------------------------------------------*/

void DirPanel::SetMessage(BMessage* msg)
{
	BFilePanel::SetMessage(msg);
	fSelectButton->SetMessage(new BMessage(*msg));
}

/*-----------------------------------------------------------------*/

void DirPanel::SetTarget(BMessenger bellhop)
{
	BFilePanel::SetTarget(bellhop);
	fSelectButton->SetTarget(bellhop);
}

/*-----------------------------------------------------------------*/

void DirPanel::SelectionChanged()
{
	entry_ref r;
	GetPanelDirectory(&r);
	SetDir(&r, fSelectButton->Message());
	BFilePanel::SelectionChanged();
}

/*-----------------------------------------------------------------*/

void DirPanel::SetDir(entry_ref* r, BMessage* msg)
{
	fSelectButton->SetLabel(r->name);
	BMessage* templ = msg ? (new BMessage(*msg)) : (new BMessage());
	templ->RemoveName("refs");
	templ->AddRef("refs", r);
	fSelectButton->SetMessage(templ);
}


/*=================================================================*/

bool DirFilter::Filter(const entry_ref*, BNode* node, struct stat*, const char*)
{
	return node->IsDirectory();
}
