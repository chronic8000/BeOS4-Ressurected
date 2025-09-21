/*------------------------------------------------------------*/
//
//	File:		cd_daemon.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Dragger.h>
#include <Drivers.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Message.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <SymLink.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "DeskbarReplicant.h"
#include "CDDBSupport.h"
#include "cd_daemon.h"

// strings that need to be localized
const char* kNO_DRIVES			= "No CD-ROM drives where found (cd_daemon will quit).";

// strings that shouldn't be localized
const char* kDAEMON_PREFS		= "cd_daemon prefs";
const char* kLOOKUP				= "Lookup: ";
const char* kLOOKUP_CREATE_LOCAL= "Create local file: ";
const char* kLOOKUP_PORT		= "Port: ";
const char* kLOOKUP_SERVER		= "Server: ";

const char*	kAUTO_PLAY_FIELD	= "Autoplay: ";
const char* kREPEAT_FIELD		= "Repeat: ";
const char* kSHUFFLE_FIELD		= "Suffle: ";
const char* kVOLUME_FIELD		= "Volume: ";
const char* kRESUME_FIELD		= "Resume: ";
const char* kKEY_FIELD			= "Key: ";
const char* kPOSITION_FIELD		= "Position: ";


/*=================================================================*/

CDDaemon::CDDaemon()
	: BApplication(kCD_DAEMON_NAME)
{
	BMessage	msg;
	BMimeType	mime;
	BVolume		volume;

	FindAllPlayers("/dev/disk");
	LoadPreferences();
	SetPulseRate(500000);
	srand((uint32)(fmod(system_time(), 1000000000.0)));

	fs_create_index(volume.Device(), kCD_FILE_KEY, B_INT32_TYPE, 0);

	mime.SetType("text/x-cd_tracks");
	if (!mime.IsInstalled())
	{
		BMimeType	text;

		text.SetType("text/plain");
		if (text.IsInstalled())
		{
			char	name[B_FILE_NAME_LENGTH];
			BBitmap	*bitmap;

			text.GetPreferredApp(name);
			mime.SetPreferredApp(name);
			text.SetType(name);
			bitmap = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
			text.GetIconForType("text/plain", bitmap, B_MINI_ICON);
			mime.SetIcon(bitmap, B_MINI_ICON);
			delete bitmap;

			bitmap = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
			text.GetIconForType("text/plain", bitmap, B_LARGE_ICON);
			mime.SetIcon(bitmap, B_LARGE_ICON);
			delete bitmap;
		}
		else
			mime.SetPreferredApp("application/x-vnd.Be-STEE");
		mime.Install();
	}

	if ((mime.GetAttrInfo(&msg) != B_NO_ERROR) || (!msg.HasString("attr:public_name")))
	{
		mime.SetShortDescription("CD Tracks");
		mime.SetLongDescription("CD Tracks");

		msg.AddString("attr:public_name", "Tracks");
		msg.AddString("attr:name", kCD_FILE_TRACKS);
		msg.AddInt32("attr:type", B_STRING_TYPE);
		msg.AddBool("attr:viewable", true);
		msg.AddBool("attr:editable", false);
		msg.AddInt32("attr:width", 200);
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT);
		msg.AddBool("attr:extra", false);
		mime.SetAttrInfo(&msg);
	}
}

/*-----------------------------------------------------------------*/

CDDaemon::~CDDaemon()
{
	show_deskbar_icon(false);
	RemovePlayers();
}

/*-----------------------------------------------------------------*/

void CDDaemon::ArgvReceived(int32 , char **)
{
}

/*-----------------------------------------------------------------*/

void CDDaemon::MessageReceived(BMessage *msg)
{
	const char*	player;
	BMessage	reply(msg->what);
	device_data	device;

	switch(msg->what)
	{
		case B_NODE_MONITOR:
			{
				int32	opcode;

				if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR)
				{
					switch (opcode)
					{
						case B_ATTR_CHANGED:
							{
								node_ref	node;

								msg->FindInt32("device", &node.device);
								msg->FindInt64("node", &node.node);
								for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
								{
									device = *fIter;
									if (device.node == node)
									{
										GetTrackList(&device);
										*fIter = device;
										break;
									}
								}
							}
							break;
					}
				}
			}
			break;

		case eCDDaemonQuit:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case eCDDaemonSubscribe:
			{
				const char*	target;

				if (msg->FindString(kCD_DAEMON_TARGET, &target) == B_NO_ERROR)
				{
					uint32	event = eCDDaemonAnyChange;

					msg->FindInt32(kCD_DAEMON_NOTIFY_EVENT, (int32)&event);
					AddSubscriber(target, event);
				}
			}
			break;

		case eCDDaemonUnsubscribe:
			{
				const char*	target;

				if (msg->FindString(kCD_DAEMON_TARGET, &target) == B_NO_ERROR)
					RemoveSubscriber(target);
			}
			break;

		case eCDDaemonPlayerCount:
			reply.AddInt32(kCD_DAEMON_PLAYER_COUNT, fDevices.size());
			msg->SendReply(&reply);
			break;

		case eCDDaemonPlayerData:
			{
				for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
				{
					device = *fIter;
					reply.AddData(kCD_DAEMON_PLAYER_DATA, B_RAW_TYPE, &device.player, sizeof(cd_daemon_player_data));
					if (device.track_list)
						reply.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, device.track_list);
					else
						reply.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, "");
				}
				msg->SendReply(&reply);
			}
			break;

		case eCDDaemonPlayerPlay:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				int32	end_time;
				int32	start_time;
				int32	track;

				if (device.player.status == ePlayerStatusStopped)
					SetShuffleBits(&device);
				if (msg->FindInt32(kCD_DAEMON_PLAYER_TRACK, &track) == B_NO_ERROR)
				{
					Play(&device, track);
					device.stopped = false;
				}
				else if ((msg->FindInt32(kCD_DAEMON_PLAYER_START_TIME, &start_time) == B_NO_ERROR) &&
						 (msg->FindInt32(kCD_DAEMON_PLAYER_END_TIME, &end_time) == B_NO_ERROR))
				{
					PlayPosition(&device, start_time, end_time);
					device.stopped = true;
				}
				else 
				{
					if (device.player.status == ePlayerStatusPaused)
						Pause(&device);
					else if ((device.player.status != ePlayerStatusPlaying) || (device.player.shuffle))
					{
						Play(&device);
						device.stopped = false;
					}
				}
				*fIter = device;
			}
			break;

		case eCDDaemonPlayerPause:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
				Pause(&device);
			break;

		case eCDDaemonPlayerStop:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				Stop(&device);
				device.stopped = true;
				SetShuffleBits(&device);
				*fIter = device;
			}
			break;

		case eCDDaemonPlayerVolume:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				int32	volume;
				
				if (msg->FindInt32(kCD_DAEMON_PLAYER_VOLUME, &volume) == B_NO_ERROR)
				{
					SetVolume(&device, volume);
					SaveDevicePrefs(&device);
					*fIter = device;
				}
			}
			break;

		case eCDDaemonPlayerMuteAll:
			if (msg->FindBool(kCD_DAEMON_PLAYER_MUTE, &fPreferences.muted) == B_NO_ERROR)
				Mute();
			break;

		case eCDDaemonPlayerEject:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				Eject(&device);
				device.stopped = true;
				SetShuffleBits(&device);
				*fIter = device;
			}
			break;

		case eCDDaemonPlayerScanFWD:
		case eCDDaemonPlayerScanBCK:
		case eCDDaemonPlayerStopScan:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
				Scan(&device, msg->what);
			break;

		case eCDDaemonPlayerAutoPlay:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				device.player.auto_play = !device.player.auto_play;
				*fIter = device;
				SaveDevicePrefs(&device, eCDDaemonAutoPlayChanged);
			}
			break;

		case eCDDaemonPlayerRepeatMode:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				device.player.repeat = !device.player.repeat;
				if ((device.player.status != ePlayerStatusPlaying) && (device.player.status != ePlayerStatusPaused))
					device.stopped = true;
				*fIter = device;
				SaveDevicePrefs(&device, eCDDaemonRepeatChanged);
			}
			break;

		case eCDDaemonPlayerShuffleMode:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				device.player.shuffle = !device.player.shuffle;
				if ((device.player.status != ePlayerStatusPlaying) && (device.player.status != ePlayerStatusPaused))
					device.stopped = true;
				*fIter = device;
				SaveDevicePrefs(&device, eCDDaemonShuffleChanged);
			}
			break;

		case eCDDaemonPlayerLookupNow:
			{
				bool	lookup = fPreferences.lookup;
				fPreferences.lookup = true;

				for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
				{
					device = *fIter;
					if ((device.player.key) && (!device.track_list))
					{
						if (device.query)
						{
							delete device.query;
							device.query = NULL;
						}
						GetTrackList(&device);
						*fIter = device;
					}
				}
				fPreferences.lookup = lookup;
			}
			break;

		case eCDDaemonPreferenceData:
			reply.AddData(kCD_DAEMON_PREFERENCE_DATA, B_RAW_TYPE, &fPreferences, sizeof(cd_daemon_preferences_data));
			msg->SendReply(&reply);
			break;

		case eCDDaemonSetPreferenceData:
			{
				cd_daemon_preferences_data*	data;
				ssize_t						size;

				if (msg->FindData(kCD_DAEMON_PREFERENCE_DATA, B_RAW_TYPE, (const void**)&data, &size) == B_NO_ERROR)
					fPreferences = *data;
				if (fPreferences.lookup)
				{
					for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
					{
						device = *fIter;
						if ((device.player.key) && (!device.track_list))
						{
							GetTrackList(&device);
							*fIter = device;
						}
					}
				}
				SavePreferences();
			}
			break;

		case eCDDaemonPlayerSetTrackList:
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				uint32	key;

				if (msg->FindInt32(kCD_FILE_KEY, (int32*)&key) == B_NO_ERROR)
				{
					const char*	tracks;

					if (msg->FindString(kCD_DAEMON_PLAYER_TRACK_LIST, &tracks) == B_NO_ERROR)
					{
						SaveTrackList(&device, tracks);
						*fIter = device;
					}
				}
			}
			break;

		case 'CDDB':
			msg->FindString(kCD_DAEMON_PLAYER_NAME, &player);
			if (FindPlayer(player, &device) == B_NO_ERROR)
			{
				uint32	key;

				if ((msg->FindInt32(kCD_FILE_KEY, (int32*)&key) == B_NO_ERROR) && (key == device.player.key))
				{
					const char*	tracks;

					if (msg->FindString(kCD_DAEMON_PLAYER_TRACK_LIST, &tracks) == B_NO_ERROR)
					{
						if (fPreferences.create_local_file)
							SaveTrackList(&device, tracks);
						*fIter = device;
					}
				}
			}
			break;

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::Pulse()
{
	int			fd;
	device_data	device;
	scsi_volume	volume;

	for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
	{
		device = *fIter;
		uint32					event = eCDDaemonNoChange;
		uint32					last_key = device.player.key;
		cd_daemon_player_data	old_data = device.player;

		device.player.key = 0;
		device.player.status = ePlayerStatusUnknown;
		if ((fd = open(device.player.name, 0)) > 0)
		{
			status_t		media_status;
			scsi_position	pos;

			if (ioctl(fd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status)) >= 0)
			{
				switch (media_status)
				{
					case B_DEV_NO_MEDIA:
						device.player.status = ePlayerStatusEmpty;
						break;

					case B_DEV_NOT_READY:
						device.player.status = ePlayerStatusUnknown;
						break;

					case B_DEV_DOOR_OPEN:
						device.player.status = ePlayerStatusDoorOpen;
						break;

					default:
						if (ioctl(fd, B_SCSI_GET_POSITION, &pos) == B_NO_ERROR)
						{
							if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
					  			((pos.position[1] == 0x12) && (!pos.position[6])))
					  		{
					  			device.player.status = ePlayerStatusStopped;
					  			if (((device.player.repeat) && (!device.stopped)) ||
					  				((device.player.shuffle) && (!device.stopped) &&
					  				((device.shuffle_bits[0]) || (device.shuffle_bits[1]) ||
					  				 (device.shuffle_bits[2]) || (device.shuffle_bits[3]))))
					  			{
						  			Play(&device);
						  			device.player.status = ePlayerStatusPlaying;
						  			device.stopped = false;
					  			}
							}
					  		else if (pos.position[1] == 0x11)
								device.player.status = ePlayerStatusPlaying;
							else
								device.player.status = ePlayerStatusPaused;
			
							if (device.player.status == ePlayerStatusStopped)
							{
								device.player.current_track = 0;
								device.player.current_track_position = 0;
								device.player.current_disc_position = 0;
							}
							else
							{
								device.player.current_track = pos.position[6];
								device.player.current_track_position = (pos.position[13] * 60) + pos.position[14];
								device.player.current_disc_position = (pos.position[9] * 60) + pos.position[10];
							}
			
							if (ioctl(fd, B_SCSI_GET_TOC, &device.player.TOC) == B_NO_ERROR)
							{
								char	byte;
			
								device.player.first_track = device.player.TOC.toc_data[2];
								device.player.last_track = device.player.TOC.toc_data[3];
								device.player.total_tracks = device.player.last_track - device.player.first_track + 1;
			
								byte = device.player.TOC.toc_data[4 + (device.player.total_tracks * 8) + 5];
								device.player.total_time = byte * 60;
								device.player.key = 0;
								device.player.key = (byte / 10) << 20;
								device.player.key |= (byte % 10) << 16;
								byte = device.player.TOC.toc_data[4 + (device.player.total_tracks * 8) + 6];
								device.player.total_time += byte;
								device.player.key |= (byte / 10) << 12;
								device.player.key |= (byte % 10) << 8;
								byte = device.player.TOC.toc_data[4 + (device.player.total_tracks * 8) + 7];
								device.player.key |= (byte / 10) << 4;
								device.player.key |= byte % 10;
			
								if (device.player.key != last_key)
								{
									if (device.track_list)
										watch_node(&device.node, B_STOP_WATCHING, this);
									GetTrackList(&device);
									SetShuffleBits(&device);
									if ((device.player.auto_play) && (device.player.status == ePlayerStatusStopped))
									{
										if ((device.player.resume) && (device.last_key == device.player.key) && (device.last_position))
										{
											int32	end;
			
											if (device.player.shuffle)
											{
												int32 track = TrackFromPosition(&device.player.TOC, device.last_position * 75);
			
												SetShuffleBits(&device);
												SetShuffleBit(&device, track, false);
												end = TrackEndingTime(&device.player.TOC, track);
											}
											else
												end = TrackEndingTime(&device.player.TOC, device.player.last_track);
											PlayPosition(&device, device.last_position * 75, end);
										}
										else
											Play(&device);
										device.stopped = false;
									}
								}
							}
							break;
					}
				}
			}
			else
			{
				free(device.track_list);
				device.track_list = NULL;
				free(device.query);
				device.query = NULL;
				device.player.key = 0;
			}

			if (!fPreferences.muted)
			{
				if (ioctl(fd, B_SCSI_GET_VOLUME, &volume) == B_NO_ERROR)
					device.player.volume = volume.port0_volume;
				else
					device.player.volume = 0;
			}

			close(fd);
		}
		device.last_key = 0;
		device.last_position = 0;
		*fIter = device;
		if (old_data.status != device.player.status)
			event |= eCDDaemonStatusChanged;
		if (old_data.key != device.player.key)
			event |= eCDDaemonDiscChanged;
		if (old_data.current_track != device.player.current_track)
			event |= eCDDaemonTrackChanged;
		if (old_data.current_disc_position != device.player.current_disc_position)
			event |= eCDDaemonPositionChanged;
		if (old_data.volume != device.player.volume)
			event |= eCDDaemonVolumeChanged;
		if (event)
			NotifySubscribers(&device, event);
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::ReadyToRun()
{
	if (fDevices.size())
		show_deskbar_icon(true);
	else
	{
		show_deskbar_icon(false);
		(new BAlert("", kNO_DRIVES, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::AddSubscriber(const char* target, uint32 event)
{
	BMessage		msg(eCDDaemonSubscribe);
	BMessenger*		messenger;
	device_data		device;
	subscriber_data	subscriber;

	for (std::list<subscriber_data>::iterator iter = fSubscribers.begin();
	     iter != fSubscribers.end();
	     iter++
	    )
	{
		if (strcmp((*iter).target, target) == 0)
			return;
	}
	subscriber.target = (char*)malloc(strlen(target) + 1);
	strcpy(subscriber.target, target);
	subscriber.change_events = event;
	fSubscribers.insert(fSubscribers.end(), subscriber);

	msg.AddInt32(kCD_DAEMON_PLAYER_COUNT, fDevices.size());
	for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
	{
		device = *fIter;
		msg.AddData(kCD_DAEMON_PLAYER_DATA, B_RAW_TYPE, &device.player, sizeof(cd_daemon_player_data));
		if (device.track_list)
			msg.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, device.track_list);
		else
			msg.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, NULL);
	}

	messenger = new BMessenger(target);
	if (messenger->IsValid())
		messenger->SendMessage(&msg);
	delete messenger;
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::CreateCDFile(BFile* file, const char* tracks)
{
	char*		new_name;
	char		name[B_FILE_NAME_LENGTH];
	char		appended_name[B_FILE_NAME_LENGTH];
	int32		index = 0;
	status_t	result;
	BDirectory	dir;
	BEntry		entry;
	BNodeInfo	node;
	BPath		path;

	if ((result = find_directory(B_USER_DIRECTORY, &path)) != B_NO_ERROR)
		return result;
	dir.SetTo(path.Path());
	if ((result = dir.InitCheck()) != B_NO_ERROR)
		return result;
	if ((result = dir.FindEntry("cd", &entry)) != B_NO_ERROR)
	{
		if ((result = dir.CreateDirectory("cd", &dir)) != B_NO_ERROR)
			return result;
	}
	else {
		dir.SetTo(&entry);
		if ((result = dir.InitCheck()) != B_NO_ERROR)
			return result;
	}

	while ((tracks[index]) && (tracks[index] != '\n'))
	{
		if ((tracks[index] == '/') || (tracks[index] == ':'))
			name[index] = '-';
		else
			name[index] = tracks[index];
		index++;
	}
	name[index] = 0;

	index = 0;
	new_name = name;
	while (1)
	{
		if (dir.CreateFile(new_name, file, true) == B_NO_ERROR)
		{
			node.SetTo(file);
			node.SetType("text/x-cd_tracks");
			break;
		}
		else
		{
			sprintf(appended_name, "%s:%d", name, (int)index);
			index++;
			new_name = appended_name;
		}
	}
	return B_NO_ERROR;
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::Eject(device_data* device)
{
	int32		fd;
	status_t	result;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		status_t	media_status = B_NO_ERROR;
		ioctl(fd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
		result = ioctl(fd, media_status == B_DEV_DOOR_OPEN ? B_LOAD_MEDIA : B_EJECT_DEVICE);
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

void CDDaemon::FindAllPlayers(const char* directory)
{
	const char			*name;
	int					fd;
	BPath				path;
	BDirectory			dir;
	BEntry				entry;
	device_geometry		geometry;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR)
	{
		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0)
		{
			entry.GetPath(&path);
			name = path.Path();
			if (entry.IsDirectory())
				FindAllPlayers(name);
			else if ((strstr(name, "/raw")) && ((fd = open(name, O_RDWR)) >= 0))
			{
				if (ioctl(fd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR)
				{
					if (geometry.device_type == B_CD)
					{
						device_data	device;

						strcpy(device.player.name, name);
						device.stopped = true;
						device.player.status = ePlayerStatusUnknown;
						device.track_list = NULL;
						device.query = NULL;
						LoadDevicePrefs(&device);
						fDevices.insert(fDevices.end(), device);
					}
				}
				close(fd);
			}
		}
	}
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::FindFile(char* name, BFile* file, uint32 mode)
{
	char		file_name[B_FILE_NAME_LENGTH];
	int32		index = 0;
	BDirectory	dir;
	BEntry		entry;
	BNodeInfo	info;
	BPath		path;
	status_t	result = B_ERROR;

	/* clean up the name */
	while (name[index])
	{
		if (name[index] == '/')
			file_name[index] = '_';
		else
			file_name[index] = name[index];
		index++;
		if (index == 255)
			break;
	}
	file_name[index] = 0;

	if ((result = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true)) != B_NO_ERROR)
		return result;
	dir.SetTo(path.Path());
	if ((result = dir.FindEntry("cd_daemon", &entry)) != B_NO_ERROR)
	{
		if ((result = dir.CreateDirectory("cd_daemon", &dir)) != B_NO_ERROR)
			return result;
	}
	else
	{
		dir.SetTo(&entry);
		if ((result = dir.InitCheck()) != B_NO_ERROR)
			return result;
	}
	if (dir.FindEntry(file_name, &entry) == B_NO_ERROR)
	{
		if (file->SetTo(&entry, mode) == B_NO_ERROR)
			return B_NO_ERROR;
		entry.Remove();
	}
	if ((result = dir.CreateFile(file_name, file, TRUE)) != B_NO_ERROR)
		return result;

	info.SetTo(file);
	info.SetType("text/plain");
	return result;
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::FindPlayer(const char* player, device_data* device)
{
	for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
	{
		*device = *fIter;
		if (strcmp(player, device->player.name) == 0)
			return B_NO_ERROR;
	}
	return B_ERROR;
}

/*-----------------------------------------------------------------*/

void CDDaemon::GetTrackList(device_data* device)
{
	BFile*		file;
	attr_info	info;

	free(device->track_list);
	device->track_list = NULL;

	if ((file = QueryForFile(device->player.key)))
	{
		if (file->GetAttrInfo(kCD_FILE_TRACKS, &info) == B_NO_ERROR)
		{
			device->track_list = (char *)malloc(info.size);
			file->ReadAttr(kCD_FILE_TRACKS, B_STRING_TYPE, 0, device->track_list, info.size);
			file->GetNodeRef(&device->node);
			watch_node(&device->node, B_WATCH_ATTR, this);
		}
		delete file;
	}

	if ((!device->track_list) && (fPreferences.lookup) && (HasAudio(device)))
	{
		if (device->query)
			device->query->SetToSite(fPreferences.server, fPreferences.port);
		else
			device->query = new CDDBQuery(fPreferences.server, fPreferences.port);
		BMessage* msg = new BMessage('CDDB');
		msg->AddString(kCD_DAEMON_PLAYER_NAME, device->player.name);
		msg->AddInt32(kCD_FILE_KEY, device->player.key);
		device->query->GetTitles(&device->player.TOC, msg, this);
	}
}

/*-----------------------------------------------------------------*/

bool CDDaemon::HasAudio(device_data* device)
{
	for (int32 i = device->player.TOC.toc_data[2]; i <= device->player.TOC.toc_data[3]; i++)
	{
		if (!IsData(&device->player.TOC, i))
			return true;
	}
	return false;
}

/*-----------------------------------------------------------------*/

bool CDDaemon::IsData(const scsi_toc* TOC, int32 track)
{
	return TOC->toc_data[((track - TOC->toc_data[2]) * 8) + 4 + 1] & 4;
}

/*-----------------------------------------------------------------*/

void CDDaemon::LoadDevicePrefs(device_data* device)
{
	BFile	file;

	device->player.auto_play = false;
	device->player.shuffle = false;
	device->player.repeat = false;
	device->player.resume = true;
	device->last_key = 0;
	device->last_position = 0;

	if (FindFile(device->player.name, &file, B_READ_ONLY) == B_NO_ERROR)
	{
		char*	prefs;
		off_t	size;

		file.GetSize(&size);
		prefs = (char *)malloc(size + 1);
		if (prefs)
		{
			char*	offset;

			file.Read(prefs, size);
			prefs[size] = 0;

			if ((offset = strstr(prefs, kAUTO_PLAY_FIELD)))
				device->player.auto_play = offset[strlen(kAUTO_PLAY_FIELD)] != '0';

			if ((offset = strstr(prefs, kREPEAT_FIELD)))
				device->player.repeat = offset[strlen(kREPEAT_FIELD)] != '0';

			if ((offset = strstr(prefs, kSHUFFLE_FIELD)))
				device->player.shuffle = offset[strlen(kSHUFFLE_FIELD)] != '0';

			if ((offset = strstr(prefs, kRESUME_FIELD)))
				device->player.resume = offset[strlen(kRESUME_FIELD)] != '0';

			if (device->player.resume)
			{
				if ((offset = strstr(prefs, kKEY_FIELD)))
					device->last_key = strtol(&offset[strlen(kKEY_FIELD)], NULL, 0);

				if ((offset = strstr(prefs, kPOSITION_FIELD)))
					device->last_position = strtol(&offset[strlen(kPOSITION_FIELD)], NULL, 0);
			}

			if ((offset = strstr(prefs, kVOLUME_FIELD)))
			{
				int32 volume = strtol(&offset[strlen(kVOLUME_FIELD)], NULL, 0);
				SetVolume(device, volume);
			}
			free(prefs);
		}
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::LoadPreferences()
{
	BFile	file;

	strcpy(fPreferences.server, "freedb.freedb.org");
	fPreferences.port = 888;
	fPreferences.lookup = false;
	fPreferences.create_local_file = true;
	fPreferences.muted = false;

	if (FindFile((char *)kDAEMON_PREFS, &file, B_READ_ONLY) == B_NO_ERROR)
	{
		char*	prefs;
		off_t	size;

		file.GetSize(&size);
		prefs = (char *)malloc(size + 1);
		if (prefs)
		{
			char*	offset;

			file.Read(prefs, size);
			prefs[size] = 0;

			if ((offset = strstr(prefs, kLOOKUP_SERVER)))
			{
				int32 len = 0;

				offset += strlen(kLOOKUP_SERVER);
				while ((offset[len] != '\n') && (offset[len]))
					len++;
				strncpy(fPreferences.server, offset, len);
				fPreferences.server[len] = 0;
			}

			if ((offset = strstr(prefs, kLOOKUP)))
				fPreferences.lookup = offset[strlen(kLOOKUP)] != '0';

			if ((offset = strstr(prefs, kLOOKUP_CREATE_LOCAL)))
				fPreferences.create_local_file = offset[strlen(kLOOKUP_CREATE_LOCAL)] != '0';

			if ((offset = strstr(prefs, kLOOKUP_PORT)))
			{
				int32 len = 0;

				offset += strlen(kLOOKUP_PORT);
				while ((offset[len] != '\n') && (offset[len]))
					len++;
				offset[len] = 0;
				fPreferences.port = strtol(offset, NULL, 0);
			}
			free(prefs);
		}
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::Mute()
{
	device_data	device;

	for (fIter = fDevices.begin(); fIter != fDevices.end(); ++fIter)
	{
		device = *fIter;
		SetVolume(&device, (fPreferences.muted) ? 0 : device.player.volume, true);
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::NotifySubscribers(device_data* device, uint32 event)
{
	BMessage		msg(eCDDaemonPlayerDataChanged);
	subscriber_data	subscriber;

	if (device)
	{
		msg.AddData(kCD_DAEMON_PLAYER_DATA, B_RAW_TYPE, &device->player, sizeof(cd_daemon_player_data));
		if (device->track_list)
			msg.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, device->track_list);
		else
			msg.AddString(kCD_DAEMON_PLAYER_TRACK_LIST, "");
	}
	else
		msg.AddData(kCD_DAEMON_PREFERENCE_DATA, B_RAW_TYPE, &fPreferences, sizeof(cd_daemon_preferences_data));
	msg.AddInt32(kCD_DAEMON_NOTIFY_EVENT, event);

	for (std::list<subscriber_data>::iterator iter = fSubscribers.begin();
	     iter != fSubscribers.end();
	     iter++
	    )
	{
		subscriber = *iter;
		if (subscriber.change_events & event)
		{
			BMessenger*		messenger;

			messenger = new BMessenger(subscriber.target);
			if (messenger->IsValid())
				messenger->SendMessage(&msg);
			delete messenger;
		}
	}
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::Pause(device_data* device)
{
	int32		fd;
	status_t	result;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		result = ioctl(fd, (device->player.status == ePlayerStatusPaused) ? B_SCSI_RESUME_AUDIO : B_SCSI_PAUSE_AUDIO);
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::Play(device_data* device, int32 start)
{
	int32		fd;
	status_t	result;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		scsi_play_track		track;

		if (start)
			track.start_track = start;
		else if (device->player.shuffle)
		{
			uint32	n;

			if ((!device->shuffle_bits[0]) && (!device->shuffle_bits[1]) &&
				(!device->shuffle_bits[2]) && (!device->shuffle_bits[3]))
			{
				SetShuffleBits(device);
				if ((!device->shuffle_bits[0]) && (!device->shuffle_bits[1]) &&
					(!device->shuffle_bits[2]) && (!device->shuffle_bits[3]))
				{
					device->stopped = true;
					return B_ERROR;
				}
			}
			do {
				n = rand();
				if (n <= RAND_MAX - ((RAND_MAX + 1) % (uint32)device->player.total_tracks + 1))
					n = n % device->player.total_tracks + 1;
			} while (((int)n < device->player.first_track) || ((int)n > device->player.last_track) || (!ShuffleBit(device, n)));
			SetShuffleBit(device, n, false);
			track.start_track = n;
		}
		else
		{
			track.start_track = 100;
			for (int32 i = device->player.TOC.toc_data[2]; i <= device->player.TOC.toc_data[3]; i++)
			{
				if (!IsData(&device->player.TOC, i))
				{
					track.start_track = i;
					break;
				}
			}
			if (track.start_track == 100)
				return B_ERROR;
		}
		track.start_index = 1;
		track.end_track = (device->player.shuffle) ? track.start_track : 99;
		track.end_index = 1;
		result = ioctl(fd, B_SCSI_PLAY_TRACK, &track);
		close(fd);
	}
	else
		result = fd;
	return result;
}


/*-----------------------------------------------------------------*/

status_t CDDaemon::PlayPosition(device_data* device, int32 start_time, int32 end_time)
{
	int32		fd;
	status_t	result;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		int32				time;
		scsi_play_position	audio;

		time = start_time;
		audio.start_m = time / (60 * 75);
		time %= (60 * 75);
		audio.start_s = time / 75;
		audio.start_f = time % 75;
	
		time = end_time;
		audio.end_m = time / (60 * 75);
		time %= (60 * 75);
		audio.end_s = time / 75;
		audio.end_f = time % 75;
	
		if ((audio.start_m == audio.end_m) &&
			(audio.start_s == audio.end_s) &&
			(audio.start_f == audio.end_f))
			result = ioctl(fd, B_SCSI_STOP_AUDIO);
		else
			result = ioctl(fd, B_SCSI_PLAY_POSITION, &audio);
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

BFile* CDDaemon::QueryForFile(uint32 key)
{
	char			predicate[256];
	BEntry			entry;
	BFile*			file = NULL;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	sprintf(predicate, "%s=%ld", kCD_FILE_KEY, key);
	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(predicate);
	query.Fetch();

	if (query.GetNextEntry(&entry) == B_NO_ERROR)
	{
		file = new BFile(&entry, O_RDWR);
		if (file->GetAttrInfo(kCD_FILE_TRACKS, &info) != B_NO_ERROR)
		{
			delete file;
			file = NULL;
		}
	}
	return file;
}

/*-----------------------------------------------------------------*/

void CDDaemon::RemovePlayers()
{
	device_data	device;

	fIter = fDevices.begin();
	while (fDevices.size())
	{
		device = *fDevices.begin();
		SaveDevicePrefs(&device, eCDDaemonNoChange, device.player.resume);
		free(device.track_list);
		delete(device.query);
		fDevices.erase(fDevices.begin());
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::RemoveSubscriber(const char* target)
{
	for (std::list<subscriber_data>::iterator iter = fSubscribers.begin();
	     iter != fSubscribers.end();
	     iter++
	    )
	{
		if (strcmp((*iter).target, target) == 0)
		{
			free((*iter).target);
			fSubscribers.erase(iter);
			return;
		}
	}
}

/*-----------------------------------------------------------------*/

void CDDaemon::SaveDevicePrefs(device_data* device, uint32 event, bool save_position)
{
	char	str[32];
	BFile	file;

	if (FindFile(device->player.name, &file, B_READ_WRITE | B_ERASE_FILE) == B_NO_ERROR)
	{
		file.Write(kAUTO_PLAY_FIELD, strlen(kAUTO_PLAY_FIELD));
		sprintf(str, "%d\n", device->player.auto_play);
		file.Write(str, strlen(str));

		file.Write(kREPEAT_FIELD, strlen(kREPEAT_FIELD));
		sprintf(str, "%d\n", device->player.repeat);
		file.Write(str, strlen(str));

		file.Write(kSHUFFLE_FIELD, strlen(kSHUFFLE_FIELD));
		sprintf(str, "%d\n", device->player.shuffle);
		file.Write(str, strlen(str));

		file.Write(kVOLUME_FIELD, strlen(kVOLUME_FIELD));
		sprintf(str, "%d\n", (int)device->player.volume);
		file.Write(str, strlen(str));

		file.Write(kRESUME_FIELD, strlen(kRESUME_FIELD));
		sprintf(str, "%d\n", (int)device->player.resume);
		file.Write(str, strlen(str));

		if ((save_position) && (device->player.status == ePlayerStatusPlaying))
		{
			file.Write(kKEY_FIELD, strlen(kKEY_FIELD));
			sprintf(str, "%d\n", (int)device->player.key);
			file.Write(str, strlen(str));

			file.Write(kPOSITION_FIELD, strlen(kPOSITION_FIELD));
			sprintf(str, "%d\n", (int)device->player.current_disc_position);
			file.Write(str, strlen(str));
		}
	}

	if (event)
		NotifySubscribers(device, event);
}

/*-----------------------------------------------------------------*/

void CDDaemon::SavePreferences()
{
	char	str[32];
	BFile	file;

	if (FindFile((char *)kDAEMON_PREFS, &file, B_READ_WRITE | B_ERASE_FILE) == B_NO_ERROR)
	{
		file.Write(kLOOKUP_SERVER, strlen(kLOOKUP_SERVER));
		file.Write(fPreferences.server, strlen(fPreferences.server));
		file.Write("\n", 1);

		file.Write(kLOOKUP_PORT, strlen(kLOOKUP_PORT));
		sprintf(str, "%d\n", (int)fPreferences.port);
		file.Write(str, strlen(str));

		file.Write(kLOOKUP, strlen(kLOOKUP));
		sprintf(str, "%d\n", fPreferences.lookup);
		file.Write(str, strlen(str));

		file.Write(kLOOKUP_CREATE_LOCAL, strlen(kLOOKUP_CREATE_LOCAL));
		sprintf(str, "%d\n", fPreferences.create_local_file);
		file.Write(str, strlen(str));
	}
	NotifySubscribers(NULL, eCDDaemonPreferencesChanged);
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::Scan(device_data* device, int32 function, int32 speed)
{
	int32		fd;
	status_t	result;
	scsi_scan	scan;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		switch (function)
		{
			case eCDDaemonPlayerScanFWD:
				scan.direction = 1;
				break;
			case eCDDaemonPlayerScanBCK:
				scan.direction = -1;
				break;
			case eCDDaemonPlayerStopScan:
				scan.direction = 0;
				break;
		}
		scan.speed = speed;
		result = ioctl(fd, B_SCSI_SCAN, &scan);
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

void CDDaemon::SaveTrackList(device_data* device, const char* tracks, bool watch)
{
	char		str[16];
	const char*	offset = tracks;
	int32		index = 0;
	int32		length = 0;
	int32		time;
	BFile*		file;

	if (device->track_list)
		free(device->track_list);
	device->track_list = (char*)malloc(strlen(tracks) + 1);
	strncpy(device->track_list, tracks, strlen(tracks) + 1);

	if (!(file = QueryForFile(device->player.key)))
	{
		file = new BFile;
		if (CreateCDFile(file, tracks) != B_NO_ERROR)
		{
			delete file;
			return;
		}
	}
	file->WriteAttr(kCD_FILE_KEY, B_INT32_TYPE, 0, &device->player.key, sizeof(int32));
	file->WriteAttr(kCD_FILE_TRACKS, B_STRING_TYPE, 0, tracks, strlen(tracks) + 1);

	while (offset[length])
	{
		while (offset[length] != '\n')
			length++;
		file->Write(offset, length);
		file->Write("\t", 1);
		if (offset == tracks)
			time = device->player.total_time;
		else
		{
			int32	start;

			start = (device->player.TOC.toc_data[4 + ((index - 1) * 8) + 5] * 60) +
					(device->player.TOC.toc_data[4 + ((index - 1) * 8) + 6]);
			time = ((device->player.TOC.toc_data[4 + (index * 8) + 5] * 60) +
				   (device->player.TOC.toc_data[4 + (index * 8) + 6])) - start;
		}
		sprintf(str, "%d:%.2d\n", (int)time / 60, (int)time % 60);
		file->Write(str, strlen(str));
		offset += length + 1;
		length = 0;
		index++;
	}

	if (watch)
	{
		file->GetNodeRef(&device->node);
		watch_node(&device->node, B_WATCH_ATTR, this);
	}
	delete file;

	NotifySubscribers(device, eCDDaemonTrackListChanged);
}

/*-----------------------------------------------------------------*/

void CDDaemon::SetShuffleBit(device_data* device, int32 track, bool set)
{
	if (set)
		device->shuffle_bits[track / 32] |= 1 << (track % 32);
	else
		device->shuffle_bits[track / 32] &= 0xffffffff ^ (1 << (track % 32));
}

/*-----------------------------------------------------------------*/

void CDDaemon::SetShuffleBits(device_data* device)
{
	device->shuffle_bits[0] = device->shuffle_bits[1] =
	device->shuffle_bits[2] = device->shuffle_bits[3] = 0;

	for (int32 i = device->player.TOC.toc_data[2]; i <= device->player.TOC.toc_data[3]; i++)
	{
		if (!IsData(&device->player.TOC, i))
			SetShuffleBit(device, i, true);
	}
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::SetVolume(device_data* device, int32 vol, bool setting_mute)
{
	int32		fd;
	status_t	result = B_NO_ERROR;
	scsi_volume	volume;

	if ((vol < 0) || (vol > 255))
		return B_ERROR;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		volume.port0_volume = vol;
		volume.port1_volume = vol;
		volume.flags = B_SCSI_PORT0_VOLUME | B_SCSI_PORT1_VOLUME;
		if (((!setting_mute) && (!fPreferences.muted)) || (setting_mute))
			result = ioctl(fd, B_SCSI_SET_VOLUME, &volume);
		if (!setting_mute)
			device->player.volume = vol;
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

bool CDDaemon::ShuffleBit(device_data* device, int32 track)
{
	return (device->shuffle_bits[track / 32] & 1 << (track % 32));
}

/*-----------------------------------------------------------------*/

status_t CDDaemon::Stop(device_data* device)
{
	int32		fd;
	status_t	result;

	if ((fd = open(device->player.name, 0)) > 0)
	{
		result = ioctl(fd, B_SCSI_STOP_AUDIO);
		close(fd);
	}
	else
		result = fd;
	return result;
}

/*-----------------------------------------------------------------*/

int32 CDDaemon::TrackEndingTime(scsi_toc* toc, int32 track)
{
	return (toc->toc_data[4 + (track * 8) + 5] * 60 * 75) +
		   (toc->toc_data[4 + (track * 8) + 6] * 75) +
		   (toc->toc_data[4 + (track * 8) + 6]);
}

/*-----------------------------------------------------------------*/

int32 CDDaemon::TrackFromPosition(scsi_toc* toc, int32 time)
{
	int32	track = toc->toc_data[2];

	while ((track <= toc->toc_data[3]) && (time > TrackEndingTime(toc, track)))
		track++;
	return track;
}


/*=================================================================*/

int main()
{
	CDDaemon*	daemon = new CDDaemon;

	daemon->Run();
	delete daemon;
}

