//******************************************************************************
//
//	File:		cursor.h
//
//	Description:	cursor object.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	04/24/93	bgs	new today
//
//******************************************************************************


#ifndef	CURSOR_H
#define	CURSOR_H

//------------------------------------------------------------------------------

#include "gr_types.h"
#include "lock.h"
#include "BArray.h"
#include "graphicDevice.h"
#include "atom.h"
#include "display_rotation.h"

class TSession;
class TView;
class BGraphicDevice;
struct RenderContext;
struct RenderPort;
class SBitmap;

//------------------------------------------------------------------------------

class SCursor : public SAtom {
private:

	static	SCursor *	g_systemDefault;
			uint8		m_hotX,m_hotY;
			uint8 		m_sizeX,m_sizeY;
			uint8		m_depth;
			uint8		*m_colorBits;
			uint8		*m_maskBits;
			int32		m_token;

public:

						SCursor(void *cursorData, int32 refs=0);
						~SCursor();
	
			void		SetToken(int32 token);

			int32		Token() { return m_token; };
			int32		HotX() { return m_hotX; };
			int32		HotY() { return m_hotY; };
			int32		SizeX() { return m_sizeX; };
			int32		SizeY() { return m_sizeY; };
			int32		Depth() { return m_depth; }
			uint8 *		ColorBits() { return m_colorBits; };
			uint8 *		MaskBits() { return m_maskBits; };

	static	SCursor *	FindCursor(int32 cursorToken);
	static	SCursor *	SystemDefault();
	static	void		SetSystemDefault(SCursor *cursor);
};

class TCursor {
public:
	bool			useHardware;
	
	BGraphicDevice *screenDevice;
	RenderContext *	theScreen;
	RenderContext *	spareContext;
	RenderContext *	bitmapSaveContext;
	RenderContext *	bitmapComposeContext;
	RenderPort *	cursorUnderPort;
	RenderPort *	bitmapSavePort;
	RenderPort *	bitmapComposePort;
	SBitmap *		cursorImage;
	SBitmap *		cursorUnder;
	SBitmap *		bitmapSave;
	SBitmap *		bitmapCompose;
	SBitmap *		trackBitmap;
	SCursor *		m_cursor;

	long			curs_width;
	long			curs_height;
	long			curs_depth;

	long			curs_current_h;
	long			curs_current_v;

	bool			visible;

	long			track_rect_vsize;
	long			track_rect_hsize;
	long			track_rect_top;
	long			track_rect_left;
	
	long			track_dh;
	long			track_dv;
	long			track_flags;
	long			track_min_h, track_min_v, track_max_h, track_max_v;

	char			hot_h;
	char			hot_v;
	uint8			track_mode;

	rect			bitmap_rect;
	long			show_level;
	rect			last_exclusion;
	bool			obscured,track_hidden;
	bool			needed;
	bool			pressed;
	int16			composeOp;
	
	bool			useCompose;
	int32			composeX;
	int32			composeY;
#if ROTATE_DISPLAY
	DisplayRotater	bitmapComposeRotater;
	DisplayRotater	bitmapSaveRotater;
#endif

//------------------------------------------------------------

			TCursor(BGraphicDevice *screen);
			~TCursor();

	void	TrackBitmap(SBitmap *bitmap, int16 drawOp, long offset_h, long offset_v);
	void	TrackRect(	char mode, long top, long left, long bottom, long right, long flags = 0,
						long min_h = -100000, long min_v = -100000,
						long max_h = 100000, long max_v = 100000);
	void	StopTracking();

	void	ResyncWithScreen();

	int32	MoveCursor(long new_h, long new_v);
	void	DoSetCursor(SCursor *cursor);

	void	MakeViewCursorCurrent(TView *view);
	void	MakeSystemCursorCurrent();

	void	SetViewCursor(TView *view, SCursor *cursor);
	void	SetAppCursor(SApp *app, SCursor *cursor);
	void	SetSystemCursor(SCursor *cursor);

	void	ClearCursorForDrawing(rect test_rect);
	void	RegionWillChange(region *willChange);
	void	RegionDoneChanging(region *doneChanging, sync_token *syncToken);

	void	Show();
	void	Hide();

	// Indicate whether displaying a cursor is needed.  E.g., when using
	// a touch screen, it is not.  If this is false, then the cursor will
	// never be shown.
	void	SetCursorState(bool isPressed, bool isNeeded=true);
	
	void	AssertVisibleIfShown();
	bool	IsHidden();
	bool	IsVisible();
	void	Obscure();

private:

	region *			excludeRegion;
	region *			tmpRegion;
	sync_token *		engineSlots;
	int32				engineSlotCount;
    void *				viewID;
    void *				appID;

	// This is the cursor that was really requested by the application,
	// i.e. either the view, app, or system cursor.
	SCursor *			m_reqCursor;
	
	void				assert_visibility();
	void				set_shown(bool state);
	bool				should_show() const;
	void				do_show_or_hide();
	bool				use_replacement_cursor() const;
	
	void				select_cursor_image(bool useReplacement);
	void				force_cursor(SCursor *cursor);
	
    void				curs_save(long h, long v);
	void 				curs_restore(long h, long v, long future_h, long future_v);
	void				curs_draw(long h, long v);
	void				curs_draw_bitmap(long h, long v);

	rect				calc_bitmap_dst(long ch, long cv);
	void				bitmap_restore(long h, long v, long future_h, long future_v);
	void				remove_track_bitmap();

	void				calc_bounds(long ch, long cv, rect *r);
	void				do_show();
	void				do_hide();
	void				sub_frames(rect old_r, rect new_r, rect *vert, rect *horiz);

	void				CalcSoftwareBounds(long ch, long cv, rect *r);
	bool				ShouldBeVisible();
};

extern SCursor cursorFinger;

//------------------------------------------------------------------------------

#endif





