//******************************************************************************
//
//	File:		cursor.cpp
//
//	Description:	1 bit cursor routines + screen semaphore.
//	
//	Written by:	Benoit Schillings
//	Desc
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <OS.h>

#include "gr_types.h"
#include "cursor.h"
#include "server.h"
#include "render.h"
#include "as_token.h"
#include "renderUtil.h"
#include "bitmap.h"
#include "rect.h"
#include "graphicDevice.h"

#include <support2/Debug.h>

/*----------------------------------------------------------------*/

#define	BITMAP_MODE			0xff
#define	MOVE_RECT_MODE		0x01
#define	SIZE_RECT_MODE		0x02
#define	PIXEL_MODE			0x03
#define	NO_TRACKING			0x00

/*---------------------------------------------------------------*/

#define	C_SIZE	16

/*---------------------------------------------------------------*/

SCursor *SCursor::g_systemDefault = NULL;;
#define standardCursorCount 7
SCursor * standardCursors[standardCursorCount+1] = {
	NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

SCursor::SCursor(void *cursorData, int32 refs)
{
	uint8 *p = ((uint8*)cursorData)+2;
	m_hotX = p[0];
	m_hotY = p[1];
	memcpy(m_colorBits,p+2,32);
	memcpy(m_maskBits,p+34,32);
	while (refs--) ServerLock();
}

SCursor::~SCursor()
{
}

void SCursor::SetToken(int32 token)
{
	m_token = token;
}

SCursor * SCursor::SystemDefault()
{
	return g_systemDefault;
};

void SCursor::SetSystemDefault(SCursor *cursor)
{
	g_systemDefault = cursor;
}

SCursor * SCursor::FindCursor(int32 cursorToken)
{
	SCursor *c = NULL;
	int32 result;

	if (cursorToken < 65536) {
		if ((cursorToken >= 0) && (cursorToken <= standardCursorCount)) {
			c = standardCursors[cursorToken];
			if (c) c->ServerLock();
		};
	} else {
		result = tokens->grab_atom(cursorToken,(SAtom**)&c);
		if (result < 0) c = NULL;
	};
	
	return c;
};

/*---------------------------------------------------------------*/

RenderPort * DirectPortForBitmap(SBitmap *bitmap)
{
	RenderPort *p = grNewRenderPort();
	grInitRenderPort(p);

	rect r;
	r.top = r.left = 0;
#if ROTATE_DISPLAY
	if (bitmap->Canvas()->pixels.pixelIsRotated) {
		r.right = bitmap->Canvas()->pixels.h-1;
		r.bottom = bitmap->Canvas()->pixels.w-1;
	} else
#endif	
	{
		r.right = bitmap->Canvas()->pixels.w-1;
		r.bottom = bitmap->Canvas()->pixels.h-1;
	}
	p->portBounds = p->drawingBounds = r;
	set_region(p->portClip,&r);
	set_region(p->drawingClip,&r);
	bitmap->SetPort(p);
	return p;
};

TCursor::TCursor(BGraphicDevice *screen)
{
	if (SCursor::g_systemDefault == NULL) {
		// Initialize the cursors the first time
		standardCursors[1] = new SCursor(finger_cursor,1);
		standardCursors[2] = new SCursor(ibeam_cursor,1);
		standardCursors[3] = new SCursor(arrow_cursor,1);
		standardCursors[4] = new SCursor(mover_cursor,1);
		standardCursors[5] = new SCursor(cross_cursor,1);
		standardCursors[6] = new SCursor(watch_cursor,1);
		standardCursors[7] = new SCursor(hand_cursor,1);
		SCursor::g_systemDefault = standardCursors[1];
	}

	curs_current_h = 0;
	curs_current_v = 0;

	track_rect_vsize = 100;
	track_rect_hsize = 100;
	track_dh	 = -20;
	track_dv	 = -20;
	set_rect(last_exclusion, 0, 0, 32000, 32000);

	show_level = 0;
	visible = true;
	track_rect_top  = 100;
	track_rect_left = 100;

	track_mode = NO_TRACKING;
	track_flags = 0;
	theScreen = NULL;//screen->MouseContext();
	screenDevice = screen;

	useHardware = true;//screenDevice->HasHardwareCursor();
	engineSlotCount = screenDevice->EngineCount();
	engineSlots = (sync_token*)grMalloc(sizeof(sync_token)*engineSlotCount,"sync token slots",MALLOC_CANNOT_FAIL);
	
	composeOp = OP_COPY;
	useCompose = false;
	cursorImage = NULL;
	cursorUnder = NULL;
	cursorUnderPort = NULL;
	spareContext = NULL;
	m_cursor = NULL;
	obscured = false;
	track_hidden = false;
	excludeRegion = newregion();
	tmpRegion = newregion();
	ResyncWithScreen();

	get_screen();
	curs_save(curs_current_h, curs_current_v);
	release_screen();

//	DoSetCursor(SCursor::SystemDefault());
	
	DoSetCursor(new SCursor(finger_cursor,1));
	
	Obscure();
}

void TCursor::ResyncWithScreen()
{
	if (!cursorImage)
		cursorImage = new SBitmap(C_SIZE,C_SIZE*2,B_GRAY1,false,0,2);
	if (cursorUnder)
		delete cursorUnder;
	cursorUnder = new SBitmap(
		C_SIZE,C_SIZE,
		screenDevice->Canvas()->pixels.pixelFormat,
		screenDevice->Canvas()->pixels.endianess);
	if (!cursorUnderPort) {
		cursorUnderPort = DirectPortForBitmap(cursorUnder);
	} else {
		cursorUnder->SetPort(cursorUnderPort);
	}

	//	This will use the upper left of the screen for the cursor "offscreen".
	//	Useful for debugging...

//	RenderCanvas *c = screenDevice->Canvas();
//	cursorUnder->Canvas()->pixels.pixelData = c->pixels.pixelData;
//	cursorUnder->Canvas()->pixels.bytesPerRow = c->pixels.bytesPerRow;

	if (!spareContext) {
		spareContext = grNewRenderContext();
		grInitRenderContext(spareContext);
		grSetContextPort_NoLock(spareContext,cursorUnderPort);
	}

	/* Get the port primed */
	grLock(spareContext);
	grUnlock(spareContext);
	
	for (int32 i=0;i<engineSlotCount;i++) {
		engineSlots[i].engine_id = 0;
		engineSlots[i].counter = 0;
	}
}

/*---------------------------------------------------------------*/

TCursor::~TCursor()
{
}

/*---------------------------------------------------------------*/

void	TCursor::sub_frames(rect old_r, rect new_r, rect *vert, rect *horiz)
{
	rect	vertical;
	rect	horizontal;

	vertical.left = 0;
	vertical.right = -1;
	horizontal.top = 0;
	horizontal.bottom = -1;

	if (old_r.left < new_r.left) {
		vertical.left = old_r.left;
		vertical.right = new_r.left - 1;
	}
	if (old_r.right > new_r.right) {
		vertical.left = new_r.right + 1;
		vertical.right = old_r.right;
	}

	vertical.top = old_r.top;
	vertical.bottom = old_r.bottom;

	if (old_r.top < new_r.top) {
		horizontal.top = old_r.top;
		horizontal.bottom = new_r.top - 1;
	}

	if (old_r.bottom > new_r.bottom) {
		horizontal.top = new_r.bottom + 1;
		horizontal.bottom = old_r.bottom;
	}
	horizontal.left = old_r.left;
	horizontal.right = old_r.right;

	sect_rect(horizontal, old_r, horiz);
	sect_rect(vertical, old_r, vert); 
}

/*---------------------------------------------------------------*/

rect TCursor::calc_bitmap_dst(long ch, long cv)
{
	rect tmp = bitmap_rect;
	offset_rect(&tmp, (ch-track_dh), (cv-track_dv)); 
	return tmp;
}

//static uint8 *realBitmapBits = NULL;

void TCursor::remove_track_bitmap()
{
	get_screen();
		show_level = -1; do_hide();
		grDestroyRenderContext(bitmapSaveContext);
		grFreeRenderContext(bitmapSaveContext);
		grDestroyRenderPort(bitmapSavePort);
		grFreeRenderPort(bitmapSavePort);
		trackBitmap->ServerUnlock();

		if (useCompose) {
			grDestroyRenderContext(bitmapComposeContext);
			grFreeRenderContext(bitmapComposeContext);
			grDestroyRenderPort(bitmapComposePort);
			grFreeRenderPort(bitmapComposePort);
			delete bitmapCompose;
		};

/*
		if (realBitmapBits) bitmapSave->Canvas()->pixels.pixelData = realBitmapBits;
		realBitmapBits = NULL;
*/
		delete bitmapSave;
		trackBitmap = NULL;
		bitmapSave = NULL;
		bitmapCompose = NULL;
		track_mode = 0;
		track_hidden = false;
		useCompose = false;
		show_level = 0; do_show();
	release_screen();
}

/*---------------------------------------------------------------*/

void TCursor::calc_bounds(long ch, long cv, rect *r)
{
	long tmp;
	if (track_flags & ONLY_HSIZE)	cv = track_rect_top + track_rect_vsize - track_dv;
	if (track_flags & ONLY_VSIZE)	ch = track_rect_left + track_rect_hsize - track_dh;
	switch(track_mode) {
		case MOVE_RECT_MODE:
			r->top  = cv - track_dv;
			r->left = ch - track_dh;
			r->bottom = r->top + track_rect_vsize;
			r->right = r->left + track_rect_hsize;
			return;
		case SIZE_RECT_MODE:
			r->top  = track_rect_top;
			r->left = track_rect_left;
			r->bottom = cv + track_dv;
			r->right = ch + track_dh;

			if (r->bottom-r->top < track_min_v)
				r->bottom = r->top+track_min_v;
			if (r->bottom-r->top > track_max_v)
				r->bottom = r->top+track_max_v;
				
			if (r->right-r->left < track_min_h)
				r->right = r->left+track_min_h;
			if (r->right-r->left > track_max_h)
				r->right = r->left+track_max_h;
				
			if (r->bottom < r->top) {
				tmp = r->bottom;
				r->bottom = r->top;
				r->top = tmp;
			}
			if (r->right < r->left) {
				tmp = r->right;
				r->right = r->left;
				r->left = tmp;
			}
			return;
		case BITMAP_MODE:
			*r = calc_bitmap_dst(ch, cv);
			r->bottom += 5;
			r->top -= 5;
			r->left -= 5;
			r->right += 5;
			return;
	}
}

/*--------------------------------------------------------------*/

void TCursor::bitmap_restore(long h, long v, long future_h, long future_v)
{
	rect	old_dst;
	rect	new_dst;
	rect	sect;
	rect	parcel_1;
	rect	tmp;
	rect	parcel_2;
	long	dh, dv;

	dh = future_h - h;
	dv = future_v - v;

	old_dst = calc_bitmap_dst(h, v);
	new_dst = calc_bitmap_dst(future_h, future_v);

	sub_frames(old_dst, new_dst, &parcel_1, &parcel_2);
	grSetDrawOp(theScreen, OP_COPY);
	
	if (!empty_rect(parcel_1)) {
		tmp = parcel_1;
		offset_rect(&tmp, -(h - track_dh), -(v - track_dv));
		grDrawIPixels(theScreen, tmp, parcel_1, &bitmapSave->Canvas()->pixels);
	}

	if (!empty_rect(parcel_2)) {
		if (!equal_rect(&parcel_1, &parcel_2)) {
			tmp = parcel_2;
			offset_rect(&tmp, -(h - track_dh), -(v - track_dv)); 
			grDrawIPixels(theScreen, tmp, parcel_2, &bitmapSave->Canvas()->pixels);
		}
	}

	sect_rect(old_dst, new_dst, &sect);
	offset_rect(&sect, -(future_h - track_dh), -(future_v - track_dv));
	tmp = sect;

	offset_rect(&tmp, dh, dv);

	point p(sect.left, sect.top);
	grCopyPixels(bitmapSaveContext, tmp, p, true);

	if (future_h > -1000) {
		sub_frames(new_dst, old_dst, &parcel_1, &parcel_2);

		if (!empty_rect(parcel_1)) {
			tmp = parcel_1;
			offset_rect(&tmp, -(future_h - track_dh), -(future_v - track_dv));
			grDrawIPixels(bitmapSaveContext, parcel_1, tmp, &screenDevice->Canvas()->pixels);
		}
		if ((!empty_rect(parcel_2)) && (!equal_rect(&parcel_1, &parcel_2))) {
			tmp = parcel_2;
			offset_rect(&tmp, -(future_h - track_dh), -(future_v - track_dv)); 
			grDrawIPixels(bitmapSaveContext, parcel_2, tmp, &screenDevice->Canvas()->pixels);
		}
	}
}

/*--------------------------------------------------------------*/

void  TCursor::curs_restore(long h, long v, long future_h, long future_v)
{
	rect src_rect;
	rect dst_rect;

	switch(track_mode) {
		case BITMAP_MODE:
			bitmap_restore(h, v, future_h, future_v);
			return;
		case SIZE_RECT_MODE:
		case MOVE_RECT_MODE:
			calc_bounds(h, v, &src_rect);
			if (!empty_rect(src_rect) || (track_mode == SIZE_RECT_MODE)) {
				grSetStipplePattern(theScreen, get_pat(GRAY));
				grSetDrawOp(theScreen, OP_INVERT);
				grStrokeIRect(theScreen, src_rect);
			}
			break;
	}

	if (!useHardware) {
		set_rect(dst_rect, v, h, v+C_SIZE-1, h+C_SIZE-1);
		set_rect(src_rect, 0, 0, C_SIZE-1, C_SIZE-1);
		grSetDrawOp(theScreen, OP_COPY);
		grDrawIPixels(theScreen, src_rect, dst_rect, &cursorUnder->Canvas()->pixels);
	}
}


/*--------------------------------------------------------------*/

void TCursor::curs_draw(long h, long v)
{
	static rect	total_cursor;
	rect src_rect;
	rect dst_rect, dst2_rect;

	if (track_mode == BITMAP_MODE) {
		dst2_rect = calc_bitmap_dst(h, v);

		if (useCompose) {
			grSetDrawOp(bitmapComposeContext, OP_COPY);
			grDrawIPixels(bitmapComposeContext, bitmap_rect, bitmap_rect, &bitmapSave->Canvas()->pixels);
			grSetDrawOp(bitmapComposeContext, composeOp);
			trackBitmap->LockPixels();
			grDrawLocPixels(bitmapComposeContext, composeX, composeY, &trackBitmap->Canvas()->pixels);
			trackBitmap->UnlockPixels();

			set_rect(dst_rect, track_dv, track_dh, track_dv+C_SIZE-1, track_dh+C_SIZE-1);
			set_rect(src_rect, C_SIZE, 0, 2*C_SIZE-1, C_SIZE-1);
			
			grSetForeColor(bitmapComposeContext, rgb(0,0,0));
			grSetBackColor(bitmapComposeContext, rgb(255,255,255));

			if (!useHardware && !track_hidden) {
				grSetDrawOp(bitmapComposeContext, OP_ERASE);
				grDrawIPixels(bitmapComposeContext,src_rect,dst_rect,&cursorImage->Canvas()->pixels);	
				src_rect.top -= C_SIZE;
				src_rect.bottom -= C_SIZE;
				grSetDrawOp(bitmapComposeContext, OP_OVER);
				grDrawIPixels(bitmapComposeContext,src_rect,dst_rect,&cursorImage->Canvas()->pixels);
			};
	
			grLock(theScreen);
			grSetDrawOp(theScreen, OP_COPY);
			grDrawIPixels(theScreen, bitmap_rect, dst2_rect, &bitmapCompose->Canvas()->pixels);
			grUnlock(theScreen);
		} else {
			grLock(theScreen);
			grSetDrawOp(theScreen, OP_COPY);
			trackBitmap->LockPixels();
			grDrawIPixels(theScreen, bitmap_rect, dst2_rect, &trackBitmap->Canvas()->pixels);
			trackBitmap->UnlockPixels();
			grUnlock(theScreen);
		}
		total_cursor = dst2_rect;
	} else {
		if (!useHardware && !track_hidden) {
			set_rect(dst_rect, v, h, v+C_SIZE-1, h+C_SIZE-1);
			set_rect(src_rect, C_SIZE, 0, 2*C_SIZE-1, C_SIZE-1);
			
			grSetForeColor(theScreen, rgb(0,0,0));
			grSetBackColor(theScreen, rgb(255,255,255));

			grSetDrawOp(theScreen, OP_ERASE);
			grDrawIPixels(theScreen,src_rect,dst_rect,&cursorImage->Canvas()->pixels);	
			src_rect.top -= C_SIZE;
			src_rect.bottom -= C_SIZE;
			grSetDrawOp(theScreen, OP_OVER);
			grDrawIPixels(theScreen,src_rect,dst_rect,&cursorImage->Canvas()->pixels);
	
			total_cursor = dst_rect;
		}
		switch(track_mode) {
			case SIZE_RECT_MODE:
			case MOVE_RECT_MODE:
				calc_bounds(h, v, &src_rect);
				if (!empty_rect(src_rect) || (track_mode == SIZE_RECT_MODE)) {
					grSetStipplePattern(theScreen, get_pat(GRAY));
					grSetDrawOp(theScreen, OP_INVERT);
					grStrokeIRect(theScreen, src_rect);
					if (useHardware) total_cursor = src_rect;
					else union_rect(&total_cursor,&src_rect,&total_cursor);
				}
				break;
		}
	}
}

/*--------------------------------------------------------------*/

void TCursor::curs_save(long h, long v)
{
	rect src_rect;
	if (track_mode != BITMAP_MODE) {
		if (!useHardware) {
			rect dst_rect;
			set_rect(src_rect, v, h, v+C_SIZE-1, h+C_SIZE-1);
			set_rect(dst_rect, 0, 0, C_SIZE-1, C_SIZE-1);
			grDrawIPixels(spareContext, src_rect, dst_rect, &screenDevice->Canvas()->pixels);
		}
	} else {
		src_rect = calc_bitmap_dst(h, v);
		grDrawIPixels(bitmapSaveContext, src_rect, bitmap_rect, &screenDevice->Canvas()->pixels);
	}
}

/*--------------------------------------------------------------*/

void TCursor::do_show()
{
	if (useHardware && !track_hidden) {
		screenDevice->ShowCursor(true);
		screenDevice->SetCursorShape(
			16,16,hot_h,hot_v,
			cursorImage->Canvas()->pixels.pixelData+32,
			cursorImage->Canvas()->pixels.pixelData);
	}
	if (visible) return;
	curs_save(curs_current_h, curs_current_v);
	curs_draw(curs_current_h, curs_current_v);
	visible = true;
	obscured = false;
}

/*--------------------------------------------------------------*/

void TCursor::do_hide()
{
	if (useHardware) {
		screenDevice->ShowCursor(false);
	}
	if (!visible) return;
	curs_restore(curs_current_h, curs_current_v, 10000, 10000);
	visible = false;
}

/*-------------------------------------------------------------*/

void TCursor::ClearCursorForDrawing(rect test_rect)
{
	rect	intersect; 	
	rect	cursor_rect;
	rect	r_rect;

	if (!visible || (useHardware && !track_mode)) return;
	
	if (track_mode == NO_TRACKING) {
		set_rect(cursor_rect,
				 (curs_current_v - 1),
				 (curs_current_h - 1),
				 (curs_current_v + C_SIZE + 1),
				 (curs_current_h + C_SIZE + 1));	
		sect_rect(test_rect, cursor_rect, &intersect);			
	} else {
		calc_bounds(curs_current_h, curs_current_v, &r_rect);
		set_rect(cursor_rect,
				 (curs_current_v - 1),
				 (curs_current_h - 1),
				 (curs_current_v + C_SIZE + 1),
				 (curs_current_h + C_SIZE + 1));	
		union_rect(&r_rect, &cursor_rect, &r_rect);
		sect_rect(r_rect, test_rect, &intersect);
	}

	if (empty_rect(intersect)) return;
	visible = false;
	curs_restore(curs_current_h, curs_current_v, -1000, -1000);
}

/*-------------------------------------------------------------*/

bool	TCursor::IsHidden()
{
	return show_level < 0;
}

/*-------------------------------------------------------------*/

void	TCursor::Show()
{
	get_screen();
	show_level = 0; do_show();
	release_screen();
}

/*-------------------------------------------------------------*/

void	TCursor::Hide()
{
	get_screen();
	show_level = -1; do_hide();
	release_screen();
}

/*---------------------------------------------------------------*/

void	TCursor::Obscure()
{
	get_screen();
	if (!obscured) {
		obscured = true;
		show_level = -1; do_hide();
	};
	release_screen();
}

/*---------------------------------------------------------------*/

void	TCursor::StopTracking()
{
	if (track_mode == BITMAP_MODE) {
		remove_track_bitmap();
	} else {
		TrackRect(0, 0, 0, 0, 0);
	};
	track_flags = 0;
}

/*---------------------------------------------------------------*/

void	TCursor::TrackRect(	char mode, long top, long left, long bottom, long right,
							long flags, long min_h, long min_v, long max_h, long max_v)
{	
	get_screen();

	screenDevice->Canvas()->accelPackage->engineIdle();

	track_hidden = mode?IsHidden():false;

	show_level = -1; do_hide();

	track_mode = mode;
	track_flags = flags;
	track_min_h = min_h;
	track_min_v = min_v;
	track_max_h = max_h;
	track_max_v = max_v;

	switch(mode) {
		case MOVE_RECT_MODE :
			track_rect_vsize = bottom - top;
			track_rect_hsize = right - left;
			track_dh	 = curs_current_h - left - hot_h;
			track_dv	 = curs_current_v - top - hot_v;
			break;
		case SIZE_RECT_MODE :
			track_rect_vsize = bottom - top;
			track_rect_hsize = right - left;
			track_rect_top  = top;
			track_rect_left = left;
			track_dh  = right - curs_current_h;
			track_dv  = bottom - curs_current_v;
			break;
	}

	show_level = 0; do_show();
	release_screen();
}

/*-------------------------------------------------------------*/

void TCursor::TrackBitmap(SBitmap *bitmap, int16 drawOp, long offset_h, long offset_v)
{
	get_screen();
	const bool isHidden = IsHidden();

	screenDevice->Canvas()->accelPackage->engineIdle();

	if (track_mode == BITMAP_MODE)
		remove_track_bitmap();
	trackBitmap = bitmap;
	composeOp = drawOp;

	useCompose = ((!useHardware && !isHidden) || (composeOp != OP_COPY));
	composeX = composeY = 0;
	offset_h -= hot_h;
	offset_v -= hot_v;

	int32 sizex = bitmap->Canvas()->pixels.w;
	int32 sizey = bitmap->Canvas()->pixels.h;

#if ROTATE_DISPLAY
		const uint32 flags = B_BITMAP_IS_ROTATED;
#else
		const uint32 flags = 0;
#endif

	if (useCompose) {
		if (!useHardware) {
			if (offset_h < 0) {
				sizex -= offset_h;
				composeX -= offset_h;
				offset_h = 0;
			} else if (offset_h > (sizex-16)) {
				sizex += offset_h - (sizex-16);
				offset_h = sizex-16;
			}
			if (offset_v < 0) {
				sizey -= offset_v;
				composeY -= offset_v;
				offset_v = 0;
			} else if (offset_v > (sizey-16)) {
				sizey += offset_v - (sizey-16);
				offset_v = sizey-16;
			}
		}
		bitmapCompose = new SBitmap(sizex, sizey,
									screenDevice->Canvas()->pixels.pixelFormat,
									screenDevice->Canvas()->pixels.endianess,
									false, flags);
		bitmapComposePort = DirectPortForBitmap(bitmapCompose);
		bitmapComposeContext = grNewRenderContext();
		grInitRenderContext(bitmapComposeContext);
#if ROTATE_DISPLAY
		bitmapComposeRotater.mirrorIValue = sizey-1;
		bitmapComposeRotater.mirrorFValue = sizey-1;
		bitmapComposePort->rotater = &bitmapComposeRotater;
		bitmapComposePort->canvasIsRotated = true;	// make it a rotated port
#endif
		grSetContextPort_NoLock(bitmapComposeContext, bitmapComposePort);
		grLock(bitmapComposeContext);
		grUnlock(bitmapComposeContext);
	}



	bitmapSave = new SBitmap(	sizex,sizey,
								screenDevice->Canvas()->pixels.pixelFormat,
								screenDevice->Canvas()->pixels.endianess,
								false, flags);
	bitmapSavePort = DirectPortForBitmap(bitmapSave);
	bitmapSaveContext = grNewRenderContext();
	grInitRenderContext(bitmapSaveContext);
#if ROTATE_DISPLAY
	bitmapSaveRotater.mirrorIValue = sizey-1;
	bitmapSaveRotater.mirrorFValue = sizey-1;
	bitmapSavePort->rotater = &bitmapSaveRotater;
	bitmapSavePort->canvasIsRotated = true;	/* make it a rotated port */
#endif
	grSetContextPort_NoLock(bitmapSaveContext,bitmapSavePort);
	grLock(bitmapSaveContext);
	grUnlock(bitmapSaveContext);

	bitmap_rect.top = bitmap_rect.left = 0;
	bitmap_rect.right = sizex-1;
	bitmap_rect.bottom = sizey-1;

	//	This will use the upper left of the screen for the bitmap "offscreen".
	//	Useful for debugging...
//	RenderCanvas *c = screenDevice->Canvas();
//	realBitmapBits = bitmapSave->Canvas()->pixels.pixelData;
//	bitmapSave->Canvas()->pixels.pixelData = c->pixels.pixelData;
//	bitmapSave->Canvas()->pixels.bytesPerRow = c->pixels.bytesPerRow;

	grSetStipplePattern(theScreen, get_pat(BLACK));
	show_level = -1;
	do_hide();
	track_dh = offset_h;
	track_dv = offset_v;
	track_mode = BITMAP_MODE;
	track_hidden = isHidden;
	show_level = 0;
	do_show();

	release_screen();
}

/*-------------------------------------------------------------*/

void TCursor::DoSetCursor(SCursor *cursor)
{
	int32	i, old_hot_h, old_hot_v;
	uchar	*tmp;
	bool    newCursor, was_visible;
	uint8 *	cursorPtr = cursorImage->Canvas()->pixels.pixelData;
	
	cursor->ServerLock();
	if (m_cursor) m_cursor->ServerUnlock();
	m_cursor = cursor;
	old_hot_h = hot_h;
	old_hot_v = hot_v;
	
	hot_v = cursor->HotX();
	hot_h = cursor->HotY();

	tmp = cursorPtr+32;
	for (i = 0; i < 32; i++) *tmp++ ^= 0xff;
	newCursor = (memcmp(cursorPtr, cursor->ColorBits(), 64) != 0);
	if (newCursor) memcpy(cursorPtr, cursor->ColorBits(), 64);
	tmp = cursorPtr+32;
	for (i = 0; i < 32; i++) *tmp++ ^= 0xff;

	if (newCursor) {
		was_visible = visible;
		if (was_visible) {
			show_level = -1;
			do_hide();
		}

		curs_current_h += old_hot_h-hot_h;
		curs_current_v += old_hot_v-hot_v;
		track_dh += old_hot_h-hot_h;
		track_dv += old_hot_v-hot_v;
		if (useHardware) {
			screenDevice->SetCursorShape(16,16,hot_h,hot_v,cursorPtr+32,cursorPtr);
			screenDevice->MoveCursor(curs_current_h+hot_h, curs_current_v+hot_v);
		}
		if (was_visible) {
			show_level = 0;
			do_show();
		}
	}
}

void TCursor::MakeSystemCursorCurrent()
{
	get_screen();
	
		SCursor *cursor = SCursor::SystemDefault();
		if (m_cursor != cursor) DoSetCursor(cursor);
		
	release_screen();
};

void TCursor::SetSystemCursor(SCursor *cursor)
{
	get_screen();
		
		SCursor::SetSystemDefault(cursor);
		if (m_cursor != cursor)
			DoSetCursor(cursor);

	release_screen();
};

/*--------------------------------------------------------------*/

int32 TCursor::MoveCursor(long new_h, long new_v)
{
	bool cursor_moved = false;
	bool gotRegions=false,needSave;
	int32 oldX,oldY,prio;

	if (!useHardware || (track_mode != NO_TRACKING)) {
		gotRegions = true;
		the_server->LockWindowsForRead();
	};

	get_screen();

	if (useHardware) {
		screenDevice->MoveCursor(new_h, new_v);
	}

	prio = 99;
	if (track_mode == BITMAP_MODE) prio = 30;
	else if (track_mode != NO_TRACKING) prio = 50;
		
	new_h -= hot_h;
	new_v -= hot_v;
	cursor_moved = (!((new_h == curs_current_h) && (new_v == curs_current_v)));

	if (cursor_moved) {
		if (obscured) {
			obscured = false;
			show_level = 0; do_show();
		};

		needSave = true;
		oldX = curs_current_h;
		oldY = curs_current_v;
		curs_current_h = new_h;
		curs_current_v = new_v;

		if (ShouldBeVisible()) {
			if (visible) {
				curs_restore(oldX, oldY, new_h, new_v);
				needSave = (track_mode != BITMAP_MODE);
			} else {
				for (int32 i=0;i<engineSlotCount;i++) {
					if (engineSlots[i].engine_id) {
						screenDevice->Canvas()->accelPackage->engineSync(engineSlots+i);
						engineSlots[i].engine_id = 0;
					}
				}
			}
			if (needSave) curs_save(new_h, new_v);
			curs_draw(curs_current_h, curs_current_v);
			visible = true;
		} else {
			if (visible) curs_restore(oldX, oldY, -10000, -10000);
			visible = false;
		}
	}

	release_screen();

	if (gotRegions) the_server->UnlockWindowsForRead();
	return prio;
}

/*--------------------------------------------------------------*/

void TCursor::RegionWillChange(region *willChange)
{
	/* If the cursor is totally in hardware, we don't have to do anything */
	if (useHardware && (track_mode == NO_TRACKING)) return;

	get_screen();

	copy_region(excludeRegion,tmpRegion);
	or_region(tmpRegion,willChange,excludeRegion);

	if (visible && !ShouldBeVisible()) {
		curs_restore(curs_current_h, curs_current_v, -1000, -1000);
		visible = false;
	};

	release_screen();
};

int32 dcount = 0;
void TCursor::RegionDoneChanging(region *doneChange, sync_token *syncToken)
{
	if (useHardware && (track_mode == NO_TRACKING)) return;

	get_screen();
	
	sub_region(excludeRegion,doneChange,tmpRegion);
	copy_region(tmpRegion,excludeRegion);
	if (syncToken && (engineSlots[syncToken->engine_id-1].counter < syncToken->counter)) {
		engineSlots[syncToken->engine_id-1] = *syncToken;
	};

	release_screen();
};

/*--------------------------------------------------------------*/

void TCursor::CalcSoftwareBounds(long ch, long cv, rect *r)
{
	long tmp;

	if (track_flags & ONLY_HSIZE)
		cv = track_rect_top + track_rect_vsize - track_dv;
	if (track_flags & ONLY_VSIZE)
		ch = track_rect_left + track_rect_hsize - track_dh;
	switch(track_mode) {
		case MOVE_RECT_MODE:
			r->top  = cv - track_dv;
			r->left = ch - track_dh;
			r->bottom = r->top + track_rect_vsize;
			r->right = r->left + track_rect_hsize;
			break;
		case SIZE_RECT_MODE:
			r->top  = track_rect_top;
			r->left = track_rect_left;
			r->bottom = cv + track_dv;
			r->right = ch + track_dh;

			if (r->bottom-r->top < track_min_v)
				r->bottom = r->top+track_min_v;
			if (r->bottom-r->top > track_max_v)
				r->bottom = r->top+track_max_v;
				
			if (r->right-r->left < track_min_h)
				r->right = r->left+track_min_h;
			if (r->right-r->left > track_max_h)
				r->right = r->left+track_max_h;
				
			if (r->bottom < r->top) {
				tmp = r->bottom;
				r->bottom = r->top;
				r->top = tmp;
			}
			if (r->right < r->left) {
				tmp = r->right;
				r->right = r->left;
				r->left = tmp;
			}
			break;
		case BITMAP_MODE:
			*r = calc_bitmap_dst(ch, cv);
			r->bottom += 5;
			r->top -= 5;
			r->left -= 5;
			r->right += 5;
			break;
		default:
			if (!useHardware) {
				set_rect((*r), cv, ch, cv+C_SIZE-1, ch+C_SIZE-1);
			} else {
				set_rect((*r), 1, 1, 0, 0);
			};
			return;
	}
	
	if (!useHardware) {
		if (r->top > cv) r->top = cv;
		if (r->bottom < (cv+C_SIZE-1)) r->bottom = (cv+C_SIZE-1);
		if (r->left > ch) r->left = ch;
		if (r->right < (ch+C_SIZE-1)) r->right = (ch+C_SIZE-1);
	};
}

/*--------------------------------------------------------------*/

void TCursor::assert_visibility()
{
	if (!visible && ShouldBeVisible()) {
		for (int32 i=0;i<engineSlotCount;i++) {
			if (engineSlots[i].engine_id) {
				screenDevice->Canvas()->accelPackage->engineSync(engineSlots+i);
				engineSlots[i].engine_id = 0;
			}
		}
		do_show();
	}
}

void TCursor::AssertVisibleIfShown()
{
	get_screen();
	assert_visibility();
	release_screen();
}

bool TCursor::IsVisible()
{
	return visible;
}

bool TCursor::ShouldBeVisible()
{
	if (!IsHidden()) {
		rect bounds;
		CalcSoftwareBounds(curs_current_h,curs_current_v,&bounds);
		return !rect_in_region(excludeRegion,&bounds);
	}
	return false;
}


