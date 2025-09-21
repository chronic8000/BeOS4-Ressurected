/*------------------------------------------------------------*/
//
//	File:		cd_daemon.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#ifndef CD_DAEMON_H
#define CD_DAEMON_H

#include <list>

#include <Application.h>
#include <BeBuild.h>

#include "cd.h"


struct device_data
{
	void			PrintToStream()
					{
						player.PrintToStream();
						(track_list) ?
							printf("%s\n", track_list)
								:
							printf("No track data\n");
						printf("stopped:       %d\n", stopped);
						printf("shuffle:       %.8x%.8x%.8x%.8x\n", (int)shuffle_bits[0],
																	(int)shuffle_bits[1],
																	(int)shuffle_bits[2],
																	(int)shuffle_bits[3]);
						printf("last_key:      %d\n", (int)last_key);
						printf("last position: %d:%d\n", (int)last_position / 60, (int)last_position % 60);
					};
	cd_daemon_player_data	player;
	bool					stopped;
	uint32					shuffle_bits[4];
	uint32					last_key;
	int32					last_position;
	node_ref				node;
	char*					track_list;
	CDDBQuery*				query;
};

struct subscriber_data
{
	char*					target;
	int32					change_events;
};


/*=================================================================*/

class CDDaemon : public BApplication
{
	public:
							CDDaemon			();
							~CDDaemon			();
		virtual void		ArgvReceived		(int32,
												 char**);
		virtual	void		MessageReceived		(BMessage*);	
		virtual	void		Pulse				();
		virtual void		ReadyToRun			();

	private:
		void				AddSubscriber		(const char*,
												 uint32);
		status_t			CreateCDFile		(BFile*,
												 const char*);
		status_t			Eject				(device_data*);
		status_t			FindFile			(char*,
												 BFile*,
												 uint32);
		status_t			FindPlayer			(const char*,
												 device_data*);
		void				FindAllPlayers		(const char*);
		void				GetTrackList		(device_data*);
		bool				HasAudio			(device_data*);
		bool				IsData				(const scsi_toc*,
												 int32);
		void				LoadDevicePrefs		(device_data*);
		void				LoadPreferences		();
		void				Mute				();
		void				NotifySubscribers	(device_data*,
												 uint32);
		status_t			Pause				(device_data*);
		status_t			Play				(device_data*,
												 int32 start = 0);
		status_t			PlayPosition		(device_data*,
												 int32,
												 int32);
		BFile*				QueryForFile		(uint32);
		void				RemovePlayers		();
		void				RemoveSubscriber	(const char*);
		void				SaveDevicePrefs		(device_data*,
												 uint32 event = eCDDaemonNoChange,
												 bool save_position = false);
		void				SavePreferences		();
		void				SaveTrackList		(device_data*,
												 const char*,
												 bool watch = true);
		status_t			Scan				(device_data*,
												 int32,
												 int32 speed = 0);
		void				SetShuffleBit		(device_data*,
												 int32,
												 bool);
		void				SetShuffleBits		(device_data*);
		status_t			SetVolume			(device_data*,
												 int32,
												 bool setting_mute = false);
		bool				ShuffleBit			(device_data*,
												 int32);
		status_t			Stop				(device_data*);
		int32				TrackEndingTime		(scsi_toc*,
												 int32);
		int32				TrackFromPosition	(scsi_toc*,
												 int32);

		cd_daemon_preferences_data				fPreferences;
		std::list <device_data>					fDevices;
		std::list <device_data>::iterator		fIter;
		std::list <subscriber_data>				fSubscribers;
};
#endif
