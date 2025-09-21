/*------------------------------------------------------------*/
//
//	File:		DeskbarReplicant.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#ifndef DESKBAR_REPLICANT_H
#define DESKBAR_REPLICANT_H

#include <list>
#include <Button.h>
#include <FilePanel.h>
#include <MenuItem.h>

#include "cd.h"


class DirPanel;
class TMiniStatusWindow;
class _EXPORT DeskbarReplicant;


struct track_status
{
	ePlayerStatus	status;
	int32			track;
};


/*=================================================================*/

class DirFilter : public BRefFilter
{
	public:
		virtual bool		Filter				(const entry_ref*,
												 BNode*,
												 struct stat*,
												 const char*);
};


/*=================================================================*/

class DeskbarReplicant : public BView
{
	public:
							DeskbarReplicant	();
							DeskbarReplicant	(BMessage*);
		virtual				~DeskbarReplicant	();
		virtual status_t	Archive				(BMessage*,
												 bool deep = true) const;
		virtual void		AttachedToWindow	();
		virtual void		DetachedFromWindow	();
		virtual void		Draw				(BRect);
		virtual void		MessageReceived		(BMessage*);
		virtual void		MouseDown			(BPoint);
		virtual void		MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		virtual void		Pulse				();
		static DeskbarReplicant*	Instantiate	(BMessage*);

	private:
		void				BuildCODECMenu		(BPopUpMenu*);
		BPopUpMenu*			BuildMenu			(BMessage*,
												 cd_daemon_preferences_data*,
												 uint32);
		void				CloseStatusWindow	();
		DirPanel*			CreateDiscPanel		(BMessenger*);
		BFilePanel*			CreateTrackPanel	(BMessenger*);
		bool				IsData				(const scsi_toc*,
												 int32);
		void				ShowContextMenu		(BPoint,
												 uint32);
		void				ShowStatusWindow	(int32);

		bool				fHovering;
		int32				fPlayerCount;
		float				fDiscPanelResize;
		float				fTrackPanelResize;
		BBitmap*			fBitmap;
		BFilePanel*			fSaveTrackPanel;
		BMessage			fRipMessage;
		BPoint				fHoverPoint;
		BPopUpMenu*			fDiscCodecMenu;
		BPopUpMenu*			fTrackCodecMenu;
		DirFilter			fDirFilter;
		DirPanel*			fSaveDiscPanel;
		TMiniStatusWindow*	fMiniStatusWindow;
		time_t				fHoverTime;
		std::
		list<track_status>	fTrack;
		std::
		list<track_status>::
		iterator			fIter;
};

void 		show_deskbar_icon					(bool);
status_t	About								(void*);


/*=================================================================*/

class PlayMenuItem : public BMenuItem
{
	public:
							PlayMenuItem		(const char*,
												 BMessage*,
												 int32,
												 int32);
		virtual void		Draw				();

	private:
		float				fAscent;
		float				fOffset;
		int32				fItem;
		int32				fItems;
		BFont				fFont;
		BView*				fView;
};


/*=================================================================*/

class ScanMenuItem : public BMenuItem
{
	public:
							ScanMenuItem		(const char*,
												 BMessage*);
							~ScanMenuItem		();
		virtual void		Draw				();

	private:
		void				Scan				(bool);

		bool				fScanning;
		BMessage			fScanMsg;
		BMessage			fStopScanMsg;
};

/*=================================================================*/

class SelectButton : public BButton
{

	public:
							SelectButton		(BFilePanel*,
												 BRect,
												 const char*
												 name,
												 const char*label,
												 BMessage*,
												 uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
												 uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		status_t			Invoke				(BMessage*);
		void				SetLabel			(const char*);

	private:
		BFilePanel*			fPanel;
};


/*=================================================================*/

class DirPanel : public BFilePanel
{
	public:
							DirPanel			(BMessenger* target = NULL,
												 entry_ref* start_directory = NULL,
												 BMessage* message = NULL,
												 BRefFilter* rf = NULL);
		void				SelectionChanged	();
		void				SetMessage			(BMessage*);
		void				SetPanelDirectory	(entry_ref*);
		void				SetTarget			(BMessenger);

	private:
		void 				SetDir				(entry_ref*,
												 BMessage*);
		BButton*			fSelectButton;
};
#endif
