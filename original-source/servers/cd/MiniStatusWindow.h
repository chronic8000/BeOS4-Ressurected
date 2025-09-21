/*------------------------------------------------------------*/
//
//	File:		MiniStatusWindow.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#ifndef MINI_STATS_WINDOW_H
#define MINI_STATS_WINDOW_H

#include <list>

#include <Font.h>
#include <Message.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>


struct track_data
{
	char*			name;
	ePlayerStatus	status;
};


/*=================================================================*/

class TStatusRegion : public BView
{
	public:
							TStatusRegion		(BView*,
												 BRect frame,
												 int32 time=-1);
							~TStatusRegion		();
		virtual void		AttachedToWindow	();
		virtual void		Draw				(BRect);
		virtual void		MessageReceived		(BMessage*);
		virtual void		Pulse				();
		void				SetAutoShow			(bool s = false)
													{ fAutoShow = s; }
		void				SetTimeRemaining	(int32 t)
													{ fTimeRemaining = t; }

private:	
		bool				UpdateText			(track_data*,
												 const cd_daemon_player_data*,
												 const char*);
		void				UpdateWindow		();

		bool				fAutoShow;
		int32				fPlayerCount;
		int32				fTimeRemaining;
		BRect				fWindowFrame;
		BRect				fReplicantFrame;
		BRect				fParentFrame;
		BBitmap*			fIcon;
		BView*				fParent;
		std
		::list <track_data>	fTrack;
		std
		::list <track_data>
		::iterator	fIter;
};


/*=================================================================*/

class TMiniStatusWindow : public BWindow
{
	public:
							TMiniStatusWindow	(BView*,
												 int32 length = -1);
		virtual void		MessageReceived		(BMessage*);
		virtual bool		QuitRequested		();

private:
		BView*				fParent;
		TStatusRegion*		fStatsView;
};
#endif
