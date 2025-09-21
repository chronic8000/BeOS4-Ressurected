//******************************************************************************
//
//	File:		server.cpp
//
//	Description:	all the stuff needed for views.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			new event system	bgs	may 94
//
//
//******************************************************************************/
   
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <time.h> 
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <OS.h>
#include <DataIO.h>
#include <Debug.h>
#include <Drivers.h>
#include <File.h>
#include <FindDirectory.h>
#include <String.h>
#include <game_input_channel.h>
#include <direct_window_priv.h>

#include "gr_types.h"
#include "macro.h"
#include "proto.h"
#include "server.h"
#include "window.h"
#include "off.h"
#include "messages.h"
#include "font.h"
#include "font_cache.h"
#include "newPicture.h"
#include "sam.h"
#include "debug_update.h"

#include "settings.h"
#include "graphicDevice.h"
#include "app.h"
#include "as_debug.h"
#include "rect.h"
#include "render.h"
#include "path.h"
#include "renderUtil.h"
#include "dwindow.h"
#include "heap.h"
#include "eventport.h"
#include "cursor.h"

#include "input.h"
#include "DecorState.h"
#include "nuDecor.h"
#include "termio.h"
#include "old_ui_settings.h"
#if BOOT_ANIMATION
#include <logo_anim.h>
#endif

// From Window.h (as Trey was afraid of #include'ing it here :-)
#define B_CURRENT_WORKSPACE    0
#define B_ALL_WORKSPACES       0xffffffff

area_id		TServer::m_cursorArea = -1;
int32 *		TServer::m_cursorAtom = NULL;
sem_id		TServer::m_cursorSem = -1;

Benaphore brol_sem("dragLock");
long reroute_port = 0;		// apparently not used
long export_modifiers;
ulong lastMods = 0;
static Benaphore ui_sem("uiLock");
static BMessage ui_settings;
TServer	*the_server = NULL;
TCursor	*the_cursor = NULL;
long curs_old_h = 0;
long curs_old_v = 0;
rgb_color desk_rgb[MAX_WORKSPACE];
long cur_switch = 0;

extern TWindow	*desk_window;
extern sem_id direct_screen_sem;
extern char *compiledWhen;
extern system_info	globalSystemInfo;

extern rgb_color desk_rgb[MAX_WORKSPACE];
extern	bool front_protection;
extern	region  *desktop_region_refresh;
extern	long ws_count;
extern	long last_ws;
extern	TWindow *console;
extern	TWS *workspace;
extern	TokenSpace *tokens;
extern 	TWindow *windowlist;
extern	int screen_x, screen_y;

/*---------------------------------------------------------------*/

inline point npoint(int32 h, int32 v) { point p; p.h = h; p.v = v; return p; };

/*---------------------------------------------------------------*/

HeapArea *		globalReadonlyHeap;

// HACK - change for multi-headed support
char			PreviousDevice[256];
display_mode	ws_res[32];

/*---------------------------------------------------------------*/


extern void	switcher(long i);
extern "C" long get_driver_info(long token, void *info);
extern int32 lock_direct_screen(int32 index, int32 state);
extern void InitColorMap();

/*---------------------------------------------------------------*/

#if ((AS_DEBUG != 0) || (MINI_SERVER != 0))

void StartDebugScreenManip(point *p)
{
	if (!the_server) return;
	
	if (the_cursor) the_cursor->Hide();
	the_server->get_mouse(&p->h, &p->v);
	wait_regions();
	open_atomic_transaction();
	TWindow *the_window = windowlist;
	while (the_window) {
		if (the_window->dw != 0)
			the_window->dw->Disconnect();
		the_window = the_window->nextwindow;
	}
	the_window = windowlist;
	while (the_window) {
		if (the_window->dw != 0)
			the_window->dw->Acknowledge();
		the_window = the_window->nextwindow;
	}
};

void EndDebugScreenManip(point p)
{
	int32 h = BGraphicDevice::Device(0)->Height();
	recalc_all_regions();
	if (p.v > h-1) p.v = h-1;
	if (the_cursor) the_cursor->MoveCursor(p.h, p.v);
	close_atomic_transaction();
	signal_regions();
	if (the_cursor) the_cursor->Show();
};

#endif

/*-------------------------------------------------------------*/

BMessage* LockUISettings()
{
	ui_sem.Lock();
	return &ui_settings;
}

status_t UnlockUISettings(uint32 flags)
{
	status_t result = B_OK;
	if ((flags&B_SAVE_UI_SETTINGS) != 0) {
		char path [PATH_MAX];
		BFile file;
		settings_path ("UI_settings", path, PATH_MAX);
		if ((result=file.SetTo(path, B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE)) >= B_OK) {
			result = ui_settings.Flatten(&file);
		}
	}
	ui_sem.Unlock();
	return result;
}

// -------------------------------------------------------------
// #pragma mark -

void TServer::LockWindowsForRead()
{
	GlobalRegionsLock.ReadLock();
};

void TServer::LockWindowsForRead(int32 pid)
{
	GlobalRegionsLock.ReadLock(pid);
};

void TServer::UnlockWindowsForRead()
{
	GlobalRegionsLock.ReadUnlock();
};

void TServer::UnlockWindowsForRead(int32 pid)
{
	GlobalRegionsLock.ReadUnlock(pid);
};

void TServer::LockWindowsForWrite()
{
	GlobalRegionsLock.WriteLock();
};

void TServer::LockWindowsForWrite(int32 pid)
{
	GlobalRegionsLock.WriteLock(pid);
};

void TServer::UnlockWindowsForWrite()
{
	GlobalRegionsLock.WriteUnlock();
};

void TServer::DowngradeWindowsWriteToRead()
{
	GlobalRegionsLock.DowngradeWriteToRead();
}

// -------------------------------------------------------------
// #pragma mark -

//	This is the default desktop filler. It will only get used if no desktop window
//	exists (i.e. the Tracker was not launched or was killed/crashed).

void paint_desktop()
{
	if (!the_server) return;
	the_server->LockWindowsForWrite();
	if (desk_window && desk_window->TopView() && desk_window->TopView()->first_child) {
		desk_window->TopView()->first_child->view_color = desk_rgb[cur_switch];
		desk_window->ExposeViews(desk_window->VisibleRegion(),redrawExposed);
	} else {
		region *desk_region = calc_desk();
		desk_fill(desk_region);
		kill_region(desk_region);
	};
	the_server->UnlockWindowsForWrite();
}

void desk_fill(region *the_region)
{
	if (empty_region(the_region)) return;

	if (desk_rgb[cur_switch].alpha > 0) {
		RenderContext *context = BGraphicDevice::Device(0)->Context();
		grSetForeColor(context,desk_rgb[cur_switch]);
		grSetStipplePattern(context,get_pat(BLACK));
		grSetDrawOp(context,OP_COPY);
		grFillRegion(context,the_region);
	}
}

#if BOOT_ANIMATION
#include <logo_anim.inc>

long play_animation(void *data) {
	char			path[PATH_MAX];
	char			*bits;
	rect			bound;
	int32			delay, top, left;
	Pixels			pix;
	bigtime_t		t0;
	anim_frame		*frame;
	anim_header		*header;
	RenderContext	*context;
	
	// load data file
	find_directory (B_BEOS_ETC_DIRECTORY, -1, true, path, PATH_MAX);
	strcat (path, "/animlogo");
	if ((header = init_animation(path)) == NULL) return -1;
	// configure buffer structure
	pix.bytesPerRow = header->buffer_h*2;
	pix.pixelFormat = PIX_16BIT;
	pix.endianess = LENDIAN;
	pix.isCompressed = false;
	pix.isLbxCompressed = false;
#if ROTATE_DISPLAY
	pix.pixelIsRotated = false;
#endif
	bits = (char*)malloc(pix.bytesPerRow*header->buffer_v+header->total_margin);
	if (bits == NULL)
		goto abort_animation;
	// check the context
	context = BGraphicDevice::Device(0)->Context();
	grLock(context);
	bound = BGraphicDevice::Device(0)->Port()->portClip->Bounds();
	top = bound.bottom-(header->buffer_v+header->bottom_offset);
	left = ((bound.right-bound.left) - (header->buffer_h-1))/2;
	grUnlock(context);
	// time to go to sleep
	snooze(7000000);
	// increase priority
	set_thread_priority(find_thread(NULL), B_REAL_TIME_PRIORITY);
	// do the animation
	frame = header->frames;
	t0 = system_time();
	delay = 0;
	while (delay >= 0) {
		delay = get_next_frame(bits+header->front_margin, pix.bytesPerRow, frame, header);
		snooze_until(t0+delay, B_SYSTEM_TIMEBASE);
		t0 = system_time();
		grLock(context);
		grSetDrawOp(context,OP_OVER);
		if (frame->plans & 12) {
			pix.pixelData = (uint8*)bits+header->front_margin+
									header->rect2.h*2+
									header->rect2.v*pix.bytesPerRow;
			pix.w = header->rect2.dh;
			pix.h = header->rect2.dv;
			grDrawLocPixels(context, left + header->rect2.h,
									 top + header->rect2.v, &pix);
		}
		pix.pixelData = (uint8*)bits+header->front_margin+
								frame->draw.h*2+
								frame->draw.v*pix.bytesPerRow;
		pix.w = frame->draw.dh;
		pix.h = frame->draw.dv;
		grDrawLocPixels(context, left + frame->draw.h,
								 top + frame->draw.v, &pix);
		grUnlock(context);
		frame++;
	}
//	_sPrintf("######### Total time : %d\n", (int32)(system_time()-t0));

	free(bits);
abort_animation:
	free_animation(header);
	return 0;
}
#endif

void initgraphics()
{
	long	loop;
	display_mode dmode;
	ulong	x;
	char	path [PATH_MAX];
	BGraphicDevice *bgd;
	BFile	file;
	uint32	tag;
	
	direct_screen_sem = create_sem(1, "direct_screen_sem"); // Oops :-)
	
	InitColorMap();
	grInitRenderLib(xgetpid,(ColorMap*)system_cs->color_list,DebugPrintfReal);
	BGraphicDevice::Init();

	window_init();
	
	bgd = BGraphicDevice::Device(0);
	if (bgd == NULL)
	{ // No device found, this is a FATAL error.
		const char * const s = "app_server: No device found! FATAL error, crashing now.\n";
		printf(s);
		_sPrintf(s);
		debugger("app_server: No device found!\n");
	}
		
	// get the driver's "default/safe" mode
	bgd->GetBaseMode(&dmode);
	// pre-load the preferences, in case the settings file is
	//  missing or hosed
	for (loop = 0; loop < MAX_WORKSPACE; loop++) {
		ws_res[loop] = dmode;
		desk_rgb[loop] = rgb(51, 102, 152);
	};
	ws_count = WS_COUNT;
	PreviousDevice[0] = '\0';
	ReadSettings();
	// if the device name changed
	if (strncmp(PreviousDevice, BGraphicDevice::Device(0)->DevicePath(), 255) != 0) {
		vague_display_mode vdm;
		display_mode dm;
		// we're going to hose the user's settings, just try and hose them less
		for (loop = 0; loop < MAX_WORKSPACE; loop++) {
			// remember to thank George for Vaguify() and Concretize()
			bgd->Vagueify(&ws_res[loop], &vdm);
			if (bgd->Concretize(&vdm, &dm) == B_OK)
				// use the concretized display mode if possible
				ws_res[loop] = dm;
			else
				// otherwise fall back to the safe mode
				ws_res[loop] = dmode;
			// leave the user's desktop colors alone
		}
		// set the current device for saving
		strcpy(PreviousDevice, bgd->DevicePath());
	}
#if 0
	for (loop = 0; loop < MAX_WORKSPACE; loop++) {
		DebugPrintf(("ws_res[%d] = {\n", loop));
		DebugPrintf(("  timing  %d  %d %d %d %d",
			ws_res[loop].timing.pixel_clock,
			ws_res[loop].timing.h_display,
			ws_res[loop].timing.h_sync_start,
			ws_res[loop].timing.h_sync_end,
			ws_res[loop].timing.h_total
		));
		DebugPrintf(("  %d  %d %d %d 0x%08x\n",
			ws_res[loop].timing.v_display,
			ws_res[loop].timing.v_sync_start,
			ws_res[loop].timing.v_sync_end,
			ws_res[loop].timing.v_total,
			ws_res[loop].timing.flags
		));
		DebugPrintf(("  space 0x%08x\n", ws_res[loop].space));
		DebugPrintf(("  virtual %d %d\n", ws_res[loop].virtual_width, ws_res[loop].virtual_height));
		DebugPrintf(("  offset %d %d\n", ws_res[loop].h_display_start, ws_res[loop].v_display_start));
		DebugPrintf(("  flags 0x%08x\n}\n", ws_res[loop].flags));
	}
#endif
	if (BGraphicDevice::Device(0)->SetMode(&ws_res[0]) != B_OK)
		BGraphicDevice::Device(0)->SetBaseMode();

	the_server->update_mode_info(BGraphicDevice::Device(0));
	
#if BOOT_ANIMATION
	rgb_color save_color = desk_rgb[cur_switch];
	desk_rgb[cur_switch] = rgb(0, 0, 0);
	paint_desktop();
	desk_rgb[cur_switch] = save_color;
	resume_thread(spawn_thread(play_animation, "boot animation", B_URGENT_DISPLAY_PRIORITY, 0L));
#else
	paint_desktop();
#endif

	the_cursor = BGraphicDevice::Device(0)->Cursor();

	/* launch the font system */
	SFont::Start();
	
	/* load the client-side user interface settings */
	settings_path ("UI_settings", path, PATH_MAX);
	if (file.SetTo(path, B_READ_ONLY) >= B_OK) {
		BMessage* settings = LockUISettings();
		if (file.Read(&tag, sizeof(tag)) != sizeof(tag) || tag != 'uinf') {
			// Read as new-style settings.
			file.Seek(0, SEEK_SET);
			settings->Unflatten(&file);
#if _R5_COMPATIBLE_
		} else {
			// Need to parse old style settings.
			file.Seek(0, SEEK_SET);
			read_old_ui_settings(file, settings);
#endif
		}
		UnlockUISettings();
	}
	
	/* while loading the app_server settings we didn't actually load the decor,
	   because that requires having the font system up and running; this code will
	   take whatever decor we found in the settings and make it load now */
	ReloadDecor();
	
	DebugPrintf(("app_server (compiled %s) debugging console. 'help' works.\n", compiledWhen));
}

/*-------------------------------------------------------------*/

bigtime_t TServer::update_busy()
{
// Here we try to estimate the time needed to refresh the screen.
// We go through our list of window that are waiting for an update.
// The window that had the slowest last update is our reference.
// If we have only one CPU, then this is what we are looking for.
// Each extra CPU gives us a slowest window for free, since we
// have only one thread/window (may not be true with IK2).
// If the slowest window renders at less than 12 fps, then we
// won't try this extra CPU optimisation, because we want to slow
// down the update rate, so that this slow window can keep up.

	const int nbCpu = globalSystemInfo.cpu_count;
	bigtime_t updateTimes[B_MAX_CPU_COUNT];
	memset(updateTimes, 0, sizeof(updateTimes));

	wait_regions();
	bigtime_t time = 0;
	const bigtime_t now = system_time();
	TWindow *the_window = windowlist;
	while (the_window) {
		// Windows who didn't answer to server's request for more than 5s are ignored until
		// they complete their update.
		if ((the_window->last_pending_update) && (now > (the_window->last_pending_update + 5000000LL)))
			goto ignore_dead_window;

		if (the_window->update_pending()) {		
			if (updateTimes[0] < the_window->m_lastUpdateTime) {
				if (nbCpu > 1) memmove(updateTimes+1, updateTimes, sizeof(updateTimes[0])*(nbCpu-1));
				updateTimes[0] = the_window->m_lastUpdateTime;
			}
		}		
ignore_dead_window:
		the_window = the_window->NextWindowFlat();
	}

	signal_regions();
	
	if (updateTimes[0] > (1000000/12))
	{ // The slowest window draws at less than 12 fps.
		return updateTimes[0];
	}
	return updateTimes[nbCpu-1];
}

/*-------------------------------------------------------------*/

TServer::TServer()
{
	sam_init();
	dup_init();
	TView::InitViews();
	
	// 16/1/2001 - Mathias: Filter out wrong double clicks
	m_lastClickedWindow = NULL;

	m_cpu_update_fps = 1000000/75; // set a correct value by default;
	m_sem_retrace = -1;

	m_mouseWindow = NULL;
	m_eventChain = NULL;
	m_eventMask = 0;
	m_target_curpos.v = m_target_curpos.h = -1;

	m_forceMove = 0;
	m_checkFocus = 0;
	last_modifiers = 0;
	globalReadonlyHeap = NULL;
	fAppKitPort = B_ERROR;
	m_psychoEraserSem = B_ERROR;
	m_cursorSem = B_ERROR;
	m_wm = NULL;

	the_tbrol = 0;
	button = 0;
	
	the_server = this;
	
	user_action();
	quit	= 0;
	target_mouse = 0;
	no_zoom = 0;
	tokens = new TokenSpace;
	globalReadonlyHeap = new HeapArea("globalReadonly",sizeof(server_color_map));
	initgraphics();
	fAppKitPort = create_port(30,"appkitPort");
	fInputServerPort = B_ERROR;

	m_psychoEraserSem = create_sem(0,"PsychoEraserSem");
	m_cursorSem = create_sem(0,"isCursorSem");
	m_wm = new WMInteraction(&m_heap);
}
	
/*-------------------------------------------------------------*/

// Returns the real mouse coords as seen by the input server
void TServer::get_mouse(long *h, long *v)
{
	*h = curs_h;
	*v = curs_v;
}

// Returns the filtered mouse coords (used by WindowManager and ScrollBars)
point TServer::GetMouse()
{
	point p;
	if ((m_target_curpos.v == -1) && (m_target_curpos.h == -1)) {
		get_mouse(&p.h, &p.v);
	} else {
		m_mouseFilterLock.Lock();
		const float e = mouse_filter(m_when_pos - system_time());
		p.h = (int32)floor(m_last_curpos.h + (m_target_curpos.h - m_last_curpos.h)*e + 0.5f);
		p.v = (int32)floor(m_last_curpos.v + (m_target_curpos.v - m_last_curpos.v)*e + 0.5f);
		m_mouseFilterLock.Unlock();
	}
	return p;
};

/*-------------------------------------------------------------*/

char	TServer::test_button()
{
	return(button);
}

/*-------------------------------------------------------------*/

void	TServer::user_action()
{
	last_user_action = system_time();
}

/*-------------------------------------------------------------*/

void do_clone_picture(TSession *a_session)
{
	SPicture	*p,*newP;
	int32		pictureToken,newToken=0,result;

	a_session->sread(4, &pictureToken);

	result = tokens->grab_atom(pictureToken,(SAtom**)&p);

	if (result >= 0) {
		newP = new SPicture();
		newP->Copy(p);
		newToken = tokens->new_token(a_session->session_team,TT_PICTURE|TT_ATOM,newP);
		newP->ClientLock();
		newP->SetToken(newToken);
		p->ServerUnlock();
	};

	a_session->swrite_l(newToken);
	a_session->sync();	
}

/*-------------------------------------------------------------*/

void do_get_picture_size(TSession *a_session)
{
	SPicture *	p;
	int32		pictureToken,result,len=0;

	a_session->sread(4, &pictureToken);

	result = tokens->grab_atom(pictureToken,(SAtom**)&p);

	if (result >= 0) {
		len = p->DataLength();
		p->ServerUnlock();
	};

	a_session->swrite_l(len);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void do_get_picture_data(TSession *a_session)
{
	SPicture *	p;
	int32		pictureToken,result,token;

	a_session->sread(4, &pictureToken);
	result = tokens->grab_atom(pictureToken,(SAtom**)&p);

	if (result >= 0) {
		a_session->swrite_l(p->PicLib().CountItems());
		for (int32 i=0;i<p->PicLib().CountItems();i++) {
			token = p->PicLib()[i]->Token();
			if (token == NO_TOKEN) {
				token = tokens->new_token(a_session->session_team,TT_PICTURE|TT_ATOM,p->PicLib()[i]);
				p->PicLib()[i]->SetToken(token);
			};
			p->PicLib()[i]->ClientLock();
			a_session->swrite_l(token);
		};
		a_session->swrite_l(p->DataLength());
		a_session->swrite(p->DataLength(),p->Data());
		p->ServerUnlock();
	} else {
		a_session->swrite_l(NO_TOKEN);
	};

	a_session->sync();
}

/*-------------------------------------------------------------*/

void	do_import_picture(TSession *a_session)
{
	SPicture *	p,*subpic;
	int32		pictureSize,token,subpics,result;

	a_session->sread(4, &subpics);
	p = new SPicture();
	for (int32 i=0;i<subpics;i++) {
		a_session->sread(4, &token);
		result = tokens->grab_atom(token,(SAtom**)&subpic);
		if (result >= 0) {
			p->PicLib().AddItem(subpic);
		} else {
			SPicture *fakePict = new SPicture();
			fakePict->ServerLock();
			p->PicLib().AddItem(fakePict);
		};
	};
	a_session->sread(4, &pictureSize);
	p->SetDataLength(pictureSize);
	a_session->sread(pictureSize, p->Data());
	
	if (p->Verify()) {
		token = tokens->new_token(a_session->session_team,TT_PICTURE|TT_ATOM,p);
		p->ClientLock();
		p->SetToken(token);
		a_session->swrite_l(token);
	} else {
		delete p;
		a_session->swrite_l(NO_TOKEN);
	};

	a_session->sync();
}

/*-------------------------------------------------------------*/

void	do_delete_picture(TSession *a_session)
{
	SPicture	*p;
	int32		pictureToken,result;

	a_session->sread(4, &pictureToken);
	result = tokens->grab_atom(pictureToken,(SAtom**)&p);
	if (result >= 0) {
		p->SetToken(NO_TOKEN);
		tokens->delete_atom(pictureToken,p);
	};
}

// -------------------------------------------------------------
// #pragma mark -

void	gr_get_screen_bitmap(TSession *a_session)
{
	frect		src_pos;
	frect		dst_pos;
	long		bitmap_token;
	long		result;
    long		mask_cursor;
    SAtom *		atom;
    SBitmap *	bitmap;

	a_session->sread(4, &bitmap_token);
	a_session->sread(4, &mask_cursor);
	a_session->sread(sizeof(src_pos), &src_pos);	
	a_session->sread(sizeof(dst_pos), &dst_pos);

#if ROTATE_DISPLAY
	/* feature not supported with rotated screen */
#else
	if ((src_pos.right-src_pos.left != dst_pos.right-dst_pos.left) ||
		(src_pos.bottom-src_pos.top != dst_pos.bottom-dst_pos.top)) {
		_sPrintf("Cannot scale screen image yet\n");
	} else {
		result = tokens->grab_atom(bitmap_token, &atom);
		if (result >= 0) {
			bitmap = dynamic_cast<SBitmap*>(atom);
			if (!bitmap) {
				TOff *off = dynamic_cast<TOff*>(atom);
				bitmap = off->Bitmap();
				bitmap->ServerLock();
				off->ServerUnlock();
			};
		
			RenderContext renderContext;
			RenderPort renderPort;
			grInitRenderContext(&renderContext);
			grInitRenderPort(&renderPort);

			rect r;
			r.top = r.left = 0;
			r.right = bitmap->Canvas()->pixels.w-1;
			r.bottom = bitmap->Canvas()->pixels.h-1;
			set_region(renderPort.drawingClip,&r);
			set_region(renderPort.portClip,&r);
			renderPort.portBounds = renderPort.drawingBounds = r;

			grSetContextPort_NoLock(&renderContext,&renderPort);
			bitmap->SetPort(&renderPort);

			the_server->LockWindowsForRead();
			if (mask_cursor) {
				get_screen();
				rect ir;
				ir.top = (int32)floor(src_pos.top);
				ir.left = (int32)floor(src_pos.left);
				ir.bottom = (int32)ceil(src_pos.bottom);
				ir.right = (int32)floor(src_pos.right);
				the_cursor->ClearCursorForDrawing(ir);
			};
			BGraphicDevice *device = BGraphicDevice::GameDevice(0);
			if (device == NULL)
				device = BGraphicDevice::Device(0);
			grDrawPixels(&renderContext,src_pos,dst_pos,&device->Canvas()->pixels);
			if (mask_cursor) {
				release_screen();
				the_cursor->AssertVisibleIfShown();
			};
			the_server->UnlockWindowsForRead();

			grDestroyRenderContext(&renderContext);
			grDestroyRenderPort(&renderPort);
			bitmap->ServerUnlock();
		} else {
			_sPrintf("Bad token! (get_screen_bitmap)\n");
		};
	};
#endif	
	
fast_out:
	a_session->swrite_l(0);	/* we need sync */
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	gr_get_sync_info(TSession *a_session)
{
	void    *buf;
	long    token;
	long    size, error=-1;
	
	a_session->sread(4, &token);

	a_session->swrite_l(0);
	a_session->swrite_l(error);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_clone_info(TSession *a_session)
{
	void    *buf;
	long    index;
	long    size, error=-1;
	
	a_session->sread(4, &index);

	a_session->swrite_l(0);
	a_session->swrite_l(error);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_lock_direct_screen(TSession *a_session)
{
	long    index;
	long	window_token;
	long	err, state;
	
	a_session->sread(4, &index);
	a_session->sread(4, &state);	
	a_session->sread(4, &window_token);
	
	err = lock_direct_screen(index, state);

	a_session->swrite_l(err);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_driver_info(TSession *a_session)
{
	long				result=-1;
	uint32				dd_token;
	direct_driver_info1	info;   
	
	a_session->sread(4, &dd_token);

	/* XXXXXXXXXXXXX Game Kit broken XXXXXXXXXXXXXXXXX */
	
//	result = get_driver_info(dd_token, (void*)&info);
	
//	a_session->swrite_l(sizeof(direct_driver_info1));
//	a_session->swrite(sizeof(direct_driver_info1),&info);
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_control_graphics_card(TSession *a_session)
{
	void    *buf;
	long    index,cmd;
	long    size,error=-1;
	
	a_session->sread(4, &index);
	a_session->sread(4, &cmd);
	BGraphicDevice *device = BGraphicDevice::GameDevice(index);
	error = B_ERROR;
	if (device) {
		switch (cmd) {
			case B_MOVE_DISPLAY: {
				uint16 x,y;
				a_session->sread(sizeof(uint16), &x);
				a_session->sread(sizeof(uint16), &y);
				// do the move
				error = device->MoveDisplay(x,y);
				a_session->swrite_l(error);
			} break;
			case B_PROPOSE_DISPLAY_MODE: {
				display_mode target, low, high;
				a_session->sread(sizeof(display_mode), &target);
				a_session->sread(sizeof(display_mode), &low);
				a_session->sread(sizeof(display_mode), &high);
				error = device->ProposeDisplayMode(&target, &low, &high);
				a_session->swrite_l(error);
				a_session->swrite(sizeof(display_mode), &target);
			} break;
			default:
				a_session->swrite_l(error);
				break;
		}
	}
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_set_color_map(TSession *a_session)
{
	uint8	*buf;
	long	index, from, to, i,j;
	long	size;
	rgb_color tmp;

	a_session->sread(4, &index);
	a_session->sread(4, &from);
	a_session->sread(4, &to);
	if ((from >= 0) && (to <= 255) && (from <= to)) {
//		xprintf("Set color map called [%d-%d]!!\n", from, to);
		size = (to+1-from)*3;
		buf = (uint8*)grMalloc(size,"cmap_buf",MALLOC_LOW);
		j = 0;
		for (i=from; i<=to; i++) {
			a_session->sread(sizeof(rgb_color), &tmp);
			if (buf) {
				buf[j++] = tmp.red;
				buf[j++] = tmp.green;
				buf[j++] = tmp.blue;
			};
		}
		if (!buf) return;
		BGraphicDevice *device = BGraphicDevice::GameDevice(index);
		if (device) {
			/* set the colors */
			device->SetIndexedColors(to+1-from, from, buf, 0);
		}
		grFree(buf);
	}
}

/*-------------------------------------------------------------*/

void gr_set_standard_space(TSession *a_session)
{
	display_mode dmode;
	long    index, error, save;
	
	a_session->sread(4, &index);
	a_session->sread(sizeof(display_mode), &dmode);
	
	BGraphicDevice *device = BGraphicDevice::GameDevice(index);
	error = B_ERROR;
	if (device) {
//		get_screen();
		DebugPrintf(("gr_set_standard_space: get_screen() completes\n"));
		error = device->SetMode(&dmode);
		DebugPrintf(("gr_set_standard_space: SetMode() completes with result %d\n", error));
//		release_screen();
		DebugPrintf(("gr_set_standard_space: release_screen() completes\n"));
	}

	a_session->swrite_l(error);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_retrace_semaphore(TSession *a_session)
{
	int32 screen;
	a_session->sread(4, &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) a_session->swrite_l(device->RetraceSemaphore());
	else a_session->swrite_l(B_ERROR);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_set_int32_setting(TSession *a_session)
{
	int32 count;
	a_session->sread(sizeof(count), &count);
	while (count > 0) {
		int32 what;
		int32 value;
		a_session->sread(sizeof(what), &what);
		a_session->sread(sizeof(value), &value);
		switch (what) {
			case AS_ANTIALIASING_SETTING:
				SFont::SetPreferredAntialiasing(value);
				break;
			case AS_ALIASING_MIN_LIMIT_SETTING:
				SFont::SetAliasingMinLimit(value);
				break;
			case AS_ALIASING_MAX_LIMIT_SETTING:
				SFont::SetAliasingMaxLimit(value);
				break;
			case AS_HINTING_MAX_LIMIT_SETTING:
				SFont::SetHintingMaxLimit(value);
				break;
			default:
				return;
		}
		count--;
	}
	WriteSettings();
}

/*-------------------------------------------------------------*/

void gr_get_int32_setting(TSession *a_session)
{
	int32 what;
	int32 value = -1;
	a_session->sread(sizeof(what), &what);
	switch (what) {
		case AS_ANTIALIASING_SETTING:
			value = SFont::PreferredAntialiasing();
			break;
		case AS_ALIASING_MIN_LIMIT_SETTING:
			value = SFont::AliasingMinLimit();
			break;
		case AS_ALIASING_MAX_LIMIT_SETTING:
			value = SFont::AliasingMaxLimit();
			break;
		case AS_HINTING_MAX_LIMIT_SETTING:
			value = SFont::HintingMaxLimit();
			break;
		default:
			return;
	}
	a_session->swrite(sizeof(value), &value);
	a_session->sync();
}

/*-------------------------------------------------------------*/

status_t SetScreenMode(int32 screen, vague_display_mode *vmode, display_mode *dmode, int32 stick, bool send_msg)
{
	status_t	result;
	TWindow		*the_window;
	long		old_width;
	long		old_height;
	long		width;
	long		height;
	long		curs_h;
	long		curs_v;
	display_mode	old_res;
	
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	
	the_cursor->Hide();
	old_width = device->Width();
	old_height = device->Height();
	the_server->get_mouse(&curs_h, &curs_v);

	wait_regions();
	open_atomic_transaction();

	the_window = windowlist;
	while (the_window) {
		if (the_window->dw != 0)
			the_window->dw->Disconnect();
		the_window = the_window->nextwindow;
	}
	the_window = windowlist;
	while (the_window) {
		if (the_window->dw != 0)
			the_window->dw->Acknowledge();
		the_window = the_window->nextwindow;
	}
	
	get_screen();
	if (vmode) result = device->SetVagueMode(vmode);
	else if (dmode) result = device->SetMode(dmode);
	else result = B_ERROR;
	release_screen();

	if (result == B_NO_ERROR) {
		old_res = ws_res[cur_switch];
		device->GetMode(&ws_res[cur_switch]);
		the_cursor->ResyncWithScreen();

		the_window = windowlist;
		while (the_window) {
			/*	Go through and inform the windows the screen they are
				on has changed res or color depth.  (The way we currently
				interface with the rendering engine makes it unneccessary to
				inform the rendering contexts.) */

			the_window->move(0, 0, 0);		// check alignment

			if (the_window->dw != 0)
				the_window->dw->NewMode();

			the_window = the_window->NextWindowFlat();
		}

		recalc_all_regions();

		kill_region(desktop_region_refresh);
		desktop_region_refresh = calc_desk();

		width = device->Width();
		height = device->Height();
		uint32 space = ws_res[cur_switch].space;
		the_window = windowlist;
		while (the_window) {
			the_window->force_full_refresh = true;
			if (the_window->is_ws_visible(cur_switch) && send_msg)
				((MWindow *)the_window)->screen_changed(width,height,space);
			the_window = the_window->NextWindowFlat();
		}
		
		if (stick)
			WriteSettings();
		result = B_NO_ERROR;

		{
			// update info about this mode
			the_server->update_mode_info(device);

			// need to send a mouse moved here to update the window underneath the cursor
			BParcel msg(FAKE_MOUSE_MOVED);
			msg.AddFloat("x",((float)curs_h / (float)old_width));
			msg.AddFloat("y",((float)curs_v / (float)old_height));
			msg.AddInt32("width",width);
			msg.AddInt32("height",height);
			preenqueue_message(&msg);
		}
	} else {
		result = B_ERROR;
	}
	
	close_atomic_transaction();
	signal_regions();
	
	the_cursor->Show();
	
	return result;
}

void gr_set_screen_res(TSession *app_session) {
	uint32	res;
	int32	stick;
	uint32	screen;
	status_t result = B_ERROR;

	app_session->sread(sizeof(int32), &screen);
	app_session->sread(sizeof(uint32), &res);
	app_session->sread(sizeof(int32), &stick);

	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) {
		display_mode dmode;
		vague_display_mode vmode;
		uint16 x,y;
		mode2parms(res,&vmode.pixelFormat,&x,&y);
		vmode.x = x;
		vmode.y = y;
		vmode.rate = 67;	// something reasonable
		result = device->Concretize(&vmode, &dmode); // let the driver pick the real rate
		if (result == B_OK)
			result = SetScreenMode(screen,0,&dmode,stick);
	}
	app_session->swrite(sizeof(status_t), &result);
	app_session->sync();
}

void gr_set_screen_rate(TSession *app_session) {
	float	rate;
	int32	stick;
	uint32	screen;
	status_t result = B_ERROR;

	app_session->sread(sizeof(int32), &screen);
	app_session->sread(sizeof(float), &rate);
	app_session->sread(sizeof(int32), &stick);

	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) {
		display_mode dmode;
		vague_display_mode vmode;
		device->GetMode(&dmode);
		device->Vagueify(&dmode, &vmode);
		vmode.rate = rate;
		device->Concretize(&vmode, &dmode);
		dmode.timing.pixel_clock = (uint32)(((double)rate * (double) dmode.timing.h_total * (double)dmode.timing.v_total) / 1000.0);
		//DebugPrintf(("Setting screen to %dx%dx%d@%.2f\n",vmode.x,vmode.y,vmode.pixelFormat,vmode.rate));
		result = SetScreenMode(screen,0,&dmode,stick, false);
	}
	app_session->swrite(sizeof(status_t), &result);
	app_session->sync();
}

/*-------------------------------------------------------------*/
void gr_set_screen_pos(TSession *app_session) {
	char	x_pos, y_pos;
	char	x_size, y_size;
	uint32	screen;
	uint32	stick;
	status_t	result = B_ERROR;

	app_session->sread(sizeof(uint32), &screen);
	app_session->sread(sizeof(char), &x_pos);
	app_session->sread(sizeof(char), &y_pos);
	app_session->sread(sizeof(char), &x_size);
	app_session->sread(sizeof(char), &y_size);
	app_session->sread(sizeof(int32), &stick);

	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) {
		display_mode dmode;
		vague_display_mode vmode;
		device->GetMode(&dmode);
		device->Vagueify(&dmode, &vmode);
		device->Concretize(&vmode, &dmode);
		DebugPrintf(("size: %d,%d  pos: %d,%d\n", x_size, y_size, x_pos, y_pos));
		if (x_size > 75) x_size = 50;
		if (x_size < 25) x_size = 50;
		if (y_size > 75) y_size = 50;
		if (y_size < 25) y_size = 50;
		if (x_pos > 75) x_pos = 50;
		if (x_pos < 25) x_pos = 50;
		if (y_pos > 75) y_pos = 50;
		if (y_pos < 25) y_pos = 50;
#if 1
		if (x_size > 50) dmode.timing.h_total -= (x_size - 50) * 8;
		else if (x_size < 50) dmode.timing.h_total += (50 - x_size) * 8;
		if (y_size > 50) dmode.timing.v_total -= (y_size - 50);
		else if (y_size < 50) dmode.timing.v_total += (50 - y_size);

		if (x_pos > 50) {
			uint16 delta = (x_pos - 50) * 8;
			dmode.timing.h_sync_start -= delta;
			dmode.timing.h_sync_end -= delta;
		} else if (x_pos < 50) {
			uint16 delta = (50 - x_pos) * 8;
			dmode.timing.h_sync_start += delta;
			dmode.timing.h_sync_end += delta;
		}
		
		if (y_pos > 50) {
			uint16 delta = (y_pos - 50);
			dmode.timing.v_sync_start -= delta;
			dmode.timing.v_sync_end -= delta;
		} else if (x_pos < 50) {
			uint16 delta = (50 - y_pos);
			dmode.timing.v_sync_start += delta;
			dmode.timing.v_sync_end += delta;
		}
#endif
		
		result = SetScreenMode(screen,0,&dmode,stick);
	}
	app_session->swrite(sizeof(status_t), &result);
	app_session->sync();
}

void gr_propose_display_mode(TSession *app_session) {
	status_t result = B_ERROR;
	uint32 screen;
	display_mode target;
	display_mode low;
	display_mode high;

	app_session->sread(sizeof(screen), &screen);
	app_session->sread(sizeof(display_mode), &target);
	app_session->sread(sizeof(display_mode), &low);
	app_session->sread(sizeof(display_mode), &high);

#if ROTATE_DISPLAY
	uint16	tmp;
	tmp = target.virtual_width;
	target.virtual_width = target.virtual_height;
	target.virtual_height = tmp;
	tmp = low.virtual_width;
	low.virtual_width = low.virtual_height;
	low.virtual_height = tmp;
	tmp = high.virtual_width;
	high.virtual_width = high.virtual_height;
	high.virtual_height = tmp;
#endif

	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) result = device->ProposeDisplayMode(&target, &low, &high);
	app_session->swrite_l(result);
	app_session->swrite(sizeof(display_mode),(void*)&target);
	app_session->sync();
}


void gr_get_display_mode_list(TSession *app_session) {
	uint32 screen;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) {
		uint32 mode_count = device->ModeCount();
		const display_mode *mode_list = device->ModeList();
		app_session->swrite_l(B_OK);
		app_session->swrite(sizeof(mode_count), &mode_count);
#if ROTATE_DISPLAY
		uint16	tmp;
		uint32	i;
		display_mode dmode;
	
		for (i=0; i<mode_count; i++) {
			dmode = mode_list[i];
			tmp = dmode.virtual_width;
			dmode.virtual_width = dmode.virtual_height;
			dmode.virtual_height = tmp;
			app_session->swrite(sizeof(display_mode), (void*)&dmode);
		}
#else
		app_session->swrite(sizeof(display_mode) * mode_count,(void*)mode_list);
#endif
	} else {
		app_session->swrite_l(B_ERROR);
	}
	app_session->sync();
}

void gr_get_display_mode(TSession *app_session) {
	uint32 screen;
	uint32 workspace;
	display_mode dmode;
	status_t result = B_ERROR;
	app_session->sread(sizeof(screen), &screen);
	app_session->sread(sizeof(workspace), &workspace);
	if ((workspace == 0xFFFFFFFF) || (workspace == cur_switch)) {
		the_server->LockWindowsForRead();
		BGraphicDevice *device = BGraphicDevice::ApparentDevice(screen, app_session->session_team);
		if (device) result = device->GetMode(&dmode);
#if ROTATE_DISPLAY
		uint16	tmp;
		tmp = dmode.virtual_width;
		dmode.virtual_width = dmode.virtual_height;
		dmode.virtual_height = tmp;
#endif
		the_server->UnlockWindowsForRead();
	} else if (workspace < ws_count) {
		result = B_OK;
		dmode = ws_res[workspace];
	}
	app_session->swrite_l(result);
	if (result == B_OK)
		app_session->swrite(sizeof(display_mode), &dmode);
	app_session->sync();
}

void gr_set_display_mode(TSession *app_session) {
	uint32 screen, stick, workspace;
	display_mode dmode;
	status_t result = B_ERROR;
	app_session->sread(sizeof(uint32), &screen);
	app_session->sread(sizeof(uint32), &workspace);
	app_session->sread(sizeof(display_mode), &dmode);
#if ROTATE_DISPLAY
	uint16	tmp;
	tmp = dmode.virtual_width;
	dmode.virtual_width = dmode.virtual_height;
	dmode.virtual_height = tmp;
#endif
	app_session->sread(sizeof(uint32), &stick);
	the_server->LockWindowsForWrite();
	BGraphicDevice *device = BGraphicDevice::ApparentDevice(screen, app_session->session_team);
	if (device) {
		if ((workspace == 0xFFFFFFFF) || (workspace == cur_switch)) {
			result = SetScreenMode(screen,0,&dmode, stick);
			device->GetMode(&dmode);
		} else {
			display_mode high = dmode;
			display_mode low = dmode;
			high.flags = ~0u;
			low.flags = 0;
			high.timing.flags = ~0u;
			low.timing.flags = 0;
			if (device->ProposeDisplayMode(&dmode, &low, &high) != B_ERROR) {
				ws_res[workspace] = dmode;
				if (stick) WriteSettings();
				result = B_OK;
			}
		}
		the_server->UnlockWindowsForWrite();
		app_session->swrite_l(result);
		if (result == B_OK) {
			app_session->swrite(sizeof(display_mode), &dmode);
		}
	} else {
		the_server->UnlockWindowsForWrite();
		app_session->swrite_l(result);
	}
	app_session->sync();
}

void gr_get_frame_buffer_config(TSession *app_session) {
	uint32 screen;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::GameDevice(screen);
#if !__INTEL__
	if (!device)
		device = BGraphicDevice::Device(screen);
#endif
	if (device) {
		RenderCanvas *rc = device->Canvas();
		frame_buffer_config fbc;
		fbc.frame_buffer = rc->pixels.pixelData;
		fbc.frame_buffer_dma = rc->pixels.pixelDataDMA;
		fbc.bytes_per_row = rc->pixels.bytesPerRow;
		app_session->swrite_l(B_OK);
		app_session->swrite(sizeof(frame_buffer_config), &fbc);
	} else {
		app_session->swrite_l(B_ERROR);
	}
	app_session->sync();
}

void gr_get_pixel_clock_limits(TSession *app_session) {
	uint32 screen;
	display_mode dmode;
	status_t result = B_ERROR;
	uint32 low, high;
	app_session->sread(sizeof(screen), &screen);
	app_session->sread(sizeof(display_mode), &dmode);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) result = device->GetPixelClockLimits(&dmode, &low, &high);
	app_session->swrite_l(result);
	if (result == B_OK) {
		app_session->swrite_l(low);
		app_session->swrite_l(high);
	}
	app_session->sync();
}

void gr_get_timing_constraints(TSession *app_session) {
	uint32 screen;
	display_timing_constraints dtc;
	status_t result = B_ERROR;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) result = device->GetTimingConstraints(&dtc);
	app_session->swrite_l(result);
	if (result == B_OK) app_session->swrite(sizeof(dtc), &dtc);
	app_session->sync();
}

void gr_get_dpms_capabilities(TSession *app_session) {
	uint32 screen, dpms_caps = 0;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) dpms_caps = device->DPMSStates();
	app_session->swrite_l(dpms_caps ? B_OK : B_ERROR);
	if (dpms_caps) app_session->swrite_l(dpms_caps);
	app_session->sync();
}

void gr_get_dpms_state(TSession *app_session) {
	uint32 screen, dpms_state = 0;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) dpms_state = device->GetDPMS();
	app_session->swrite_l(dpms_state ? B_OK : B_ERROR);
	if (dpms_state) app_session->swrite_l(dpms_state);
	app_session->sync();
}

void gr_set_dpms_state(TSession *app_session) {
	uint32 screen, dpms_state;
	status_t result = B_ERROR;
	app_session->sread(sizeof(screen), &screen);
	app_session->sread(sizeof(dpms_state), &dpms_state);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (device) result = device->SetDPMS(dpms_state);
	app_session->swrite_l(result);
	app_session->sync();
}

void gr_get_accelerant_image_path(TSession *app_session) {
	uint32 screen;
	status_t result = B_ERROR;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (!device)
		app_session->swrite_l(B_ERROR);
	else {
		const char *path = device->ImagePath();
		int32	size = strlen(path);
		app_session->swrite_l(B_OK);
		app_session->swrite_l(size);
		app_session->swrite(size, (char*)path);
	}
	app_session->sync();
}

void gr_get_accelerant_clone_info(TSession *app_session) {
	uint32 screen;
	app_session->sread(sizeof(screen), &screen);
	BGraphicDevice *device = BGraphicDevice::Device(screen);
	if (!device) app_session->swrite_l(B_ERROR);
	else {
		ssize_t size = device->CloneInfoSize();
		if (size > 0) {
			const void *data = device->CloneInfo();
			if (data) {
				app_session->swrite_l(B_OK);
				app_session->swrite_l(size);
				app_session->swrite(size, (char*)data);
			} else {
				app_session->swrite_l(B_ERROR);
			}
		} else {
			app_session->swrite_l(B_ERROR);
		}
	}
	app_session->sync();
}

void gr_accelerant_perform_hack(TSession *app_session) {
	uint32 screen;
	status_t result = B_ERROR;
	ssize_t size;
	uint8 buff[32];
	void *info = (void *)buff;
	BGraphicDevice *device;
	void *output;

	app_session->sread(sizeof(screen), &screen);
	app_session->sread(sizeof(size), &size);
	if (size > sizeof(buff)) {
		info = (void *)grMalloc(size,"hack info",MALLOC_LOW);
		if (!info) {
			while (size > 0) {
				app_session->sread(sizeof(buff) < size ? sizeof(buff) : size, buff);
				size -= sizeof(buff);
			}
			goto sync_error;
		}
	}
	app_session->sread(size, info);
	the_server->LockWindowsForRead();
	device = BGraphicDevice::ApparentDevice(screen, app_session->session_team);
	if (!device) {
		the_server->UnlockWindowsForRead();
		goto sync_error;
	}
	output = 0;
	size = 0;
	result = device->PerformHack(info, &size, &output);
	the_server->UnlockWindowsForRead();
	if (result != B_OK) goto sync_error;

	app_session->swrite_l(result);
	app_session->swrite_l(size);
	if (size && output) {
		app_session->swrite(size, output);
		grFree(output);
	}
	goto sync;

sync_error:
	app_session->swrite_l(result);
sync:
	if (info && (info != buff)) grFree(info);
	app_session->sync();
}

void gr_get_display_device_path(TSession *app_session)
{
	uint32 screen;
	BGraphicDevice *device;

	app_session->sread(sizeof(screen), &screen);
	device = BGraphicDevice::Device(screen);
	app_session->swrite_l(device ? B_OK : B_ERROR);
	if (device)
	{
		const char *name = device->DevicePath();
		ssize_t size = strlen(name) + 1;
		app_session->swrite_l(size);
		app_session->swrite(size, (void *)name);
	}
	app_session->sync();
}
/*-------------------------------------------------------------*/

void gr_string_width(TSession *app_session) {
	int                 i, char_max;
	uint                str_count;
	uchar               buf[256];
	uchar               *d_buffer;
	float               widths[32];
	float               *t_width;
	ushort              length;
	SFont				font;

	font.ReadPacket(app_session);
	app_session->sread(4, (void*)&str_count);

	char_max = 256;
	d_buffer = buf;
	
	if (str_count > 32)
		t_width = (float*)grMalloc(sizeof(float)*str_count,"t_width",MALLOC_CANNOT_FAIL);
	else
		t_width = widths;

	for (i=0; i<str_count; i++) {
		app_session->sread(2, &length);
		if ((length+1) > char_max) {
			if (char_max > 256) grFree(d_buffer);
			d_buffer = (uchar*)grMalloc((length+1),"d_buffer",MALLOC_CANNOT_FAIL);
			char_max = length+1;
		}
		app_session->sread(length, d_buffer);
		d_buffer[length] = 0;
		t_width[i] = font.StringWidth(d_buffer);
	}

	app_session->swrite(str_count * sizeof(float), t_width);
	app_session->sync();

	if (d_buffer != buf)
		grFree(d_buffer);
	if (t_width != widths)
		grFree(t_width);
}

/*-------------------------------------------------------------*/

void gr_truncate_strings(TSession *app_session) {
	int                 i;
	uint                str_count;
	uchar               *str_buf, *cur_buf, *res_buf;
	uint16              tmp16;
	uint32              total_length, mode;
	float               width;
	SFont				font;
	
	font.ReadPacket(app_session);
	app_session->sread(4, (void*)&width);
	app_session->sread(4, (void*)&mode);
	app_session->sread(4, (void*)&str_count);
	app_session->sread(4, (void*)&total_length);
	
	str_buf = (uchar*)grMalloc(total_length+1,"str_buf",MALLOC_CANNOT_FAIL);
	str_buf[0] = 0;
	app_session->sread(total_length, str_buf+1);

	if (font.Encoding() == 0) {
		res_buf = (uchar*)grMalloc(total_length+3,"res_buf",MALLOC_CANNOT_FAIL);
		if (mode == B_TRUNCATE_SMART)
			mode = B_TRUNCATE_MIDDLE;
		cur_buf = str_buf+1;
		for (i=0; i<str_count; i++) {
			tmp16 = strlen((char*)cur_buf);
			font.TruncateString(width, mode, cur_buf, res_buf);
			cur_buf += (tmp16+1);
			tmp16 = strlen((char*)res_buf);
			app_session->swrite(2, &tmp16);
			app_session->swrite(tmp16, res_buf);
		}
		grFree((void*)res_buf);
	}
	else {
		cur_buf = str_buf+1;
		for (i=0; i<str_count; i++) {
			tmp16 = strlen((char*)cur_buf);
			app_session->swrite(2, &tmp16);
			app_session->swrite(tmp16, cur_buf);
			cur_buf += (tmp16+1);
		}
	}
	app_session->sync();

	grFree(str_buf);
}

/*-------------------------------------------------------------*/

void gr_get_escapements(TSession *app_session) {
	int                 i, mode;
	uchar               *d_buffer;
	const uchar         *str;
	float               *d_info, *d_info2;
	float               esc_x, esc_y;
	float               dnsp, dsp;
	uint8				command;
	ushort              count;
	ushort              length;
	SFont				font;
	fc_escape_table     e_table;

	font.ReadPacket(app_session);
	app_session->sread(4, (void*)&dnsp);
	app_session->sread(4, (void*)&dsp);
	app_session->sread(1, (void*)&command);
	app_session->sread(2, (void*)&length);

	// allocate oversized buffers is needed for output
	if (command == FC_ESCAPE_AS_FLOAT)
		count = length;
	else
		count = 2*length;		
	d_info = (float*)grMalloc(sizeof(float)*count,"d_info",MALLOC_CANNOT_FAIL);	
	if (command == FC_ESCAPE_AS_BPOINT_PAIR)
		d_info2 = (float*)grMalloc(sizeof(float)*count,"d_info2",MALLOC_CANNOT_FAIL);
	else
		d_info2 = NULL;

	// read the string to processed
	d_buffer = (uchar*)grMalloc(length+1,"d_buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(length, d_buffer);
	d_buffer[length] = 0;
	
	// linear escapment only.
	if (command == FC_ESCAPE_AS_FLOAT) {
		if (font.RotationRaw() == 0.0) {
			esc_x = 1.0;
			esc_y = 0.0;
		}
		else {
			esc_x = cos(font.RotationRaw());
			esc_y = -sin(font.RotationRaw());
		}
		count = 0;
		str = d_buffer;
		while (str != 0L) {
			str = font.GetEscapement(&e_table, str, dnsp, dsp);
			for (i=0; i<e_table.count; i++)
				d_info[i+count] = e_table.escapes[i].x_escape*esc_x +
								  e_table.escapes[i].y_escape*esc_y;
			count += e_table.count;
		}
	}
	// bidirectional escapement.
	else {
		count = 0;
		str = d_buffer;
		while (str != 0L) {
			str = font.GetEscapement(&e_table, str, dnsp, dsp);
			for (i=0; i<e_table.count; i++) {
				d_info[2*i+count] = e_table.escapes[i].x_escape;
				d_info[2*i+count+1] = e_table.escapes[i].y_escape;
			}
			if (d_info2 != NULL) for (i=0; i<e_table.count; i++) {
				d_info2[2*i+count] = e_table.offsets[i].h;
				d_info2[2*i+count+1] = e_table.offsets[i].v;
			}
			count += 2*e_table.count;
		}
	}
	
	app_session->swrite(2, &count);
	if (count > 0) {
		app_session->swrite(count * sizeof(float), d_info);
		if (d_info2 != NULL)
			app_session->swrite(count * sizeof(float), d_info2);
	}
	app_session->sync();

	grFree(d_buffer);
	grFree(d_info);
	if (d_info2 != NULL)
		grFree(d_info2);
}

/*-------------------------------------------------------------*/

void gr_get_font_glyphs(TSession *app_session) {
	int32               i, mode, opCount, ptCount;
	uchar               buf[16];
	uchar               *d_buffer;
	const uchar         *str;
	ushort              count;
	ushort              length;
	SFont				font;
	fc_glyph_table		g_table;
	
	font.ReadPacket(app_session);
	app_session->sread(2, (void*)&length);

	if (length > 15)
		d_buffer = (uchar*)grMalloc(length+1,"d_buffer",MALLOC_CANNOT_FAIL);
	else
		d_buffer = buf;	
	app_session->sread(length, d_buffer);
	d_buffer[length] = 0;

	str = d_buffer;
	while (str != 0L) {
		str = font.GetGlyphs(&g_table, str);
		for (i=0; i<g_table.count; i++) {
			if (g_table.glyphs[i] != NULL) {
				opCount = g_table.glyphs[i]->Ops()->CountItems();
				ptCount = g_table.glyphs[i]->Points()->CountItems();
				if (opCount && ptCount) {
					for (int32 k=0;k<ptCount;k++) {
						(*g_table.glyphs[i]->Points())[k].v = -((*g_table.glyphs[i]->Points())[k].v - 0.5);
						(*g_table.glyphs[i]->Points())[k].h -= 0.5;
					};
					app_session->swrite(4, &opCount);
					app_session->swrite(4, &ptCount);
					
					if ((opCount != 0) && (ptCount != 0)) {
						app_session->swrite(sizeof(uint32)*opCount,
											g_table.glyphs[i]->Ops()->Items());
						app_session->swrite(sizeof(fpoint)*ptCount,
											g_table.glyphs[i]->Points()->Items());
					}
				} else {
					opCount = 0;
					ptCount = 0;
					app_session->swrite(4, &opCount);
					app_session->swrite(4, &ptCount);
				};
				delete g_table.glyphs[i];
			}
			else {
				opCount = 0;
				ptCount = 0;
				app_session->swrite(4, &opCount);
				app_session->swrite(4, &ptCount);
			}
		}
	}

	app_session->sync();

	if (d_buffer != buf)
		grFree(d_buffer);
}

/*-------------------------------------------------------------*/

void gr_get_edges(TSession *app_session) {
	int                 i;
	uchar               buf[16];
	uchar               *d_buffer;
	const uchar         *str;
	ushort              count;
	ushort              length;
	server_edge_info	info[16];
	server_edge_info	*d_info;
	SFont				font;
	fc_edge_table       e_table;

	font.ReadPacket(app_session);
	font.SetShearRaw(0.0);
	font.SetRotationRaw(0.0);
	app_session->sread(2, (void*)&length);

	if (length > 15) {
		d_buffer = (uchar*)grMalloc(length+1,"d_buffer",MALLOC_CANNOT_FAIL);
		d_info = (server_edge_info*)grMalloc(sizeof(server_edge_info)*(length+1),"d_info",MALLOC_CANNOT_FAIL);
	}
	else {
		d_buffer = buf;
		d_info = info;
	}
	
	app_session->sread(length, d_buffer);
	d_buffer[length] = 0;

	count = 0;
	str = d_buffer;
	while (str != 0L) {
		str = font.GetEdge(&e_table, str);
		for (i=0; i<e_table.count; i++) {
			d_info[i+count].left = e_table.edges[i].left;
			d_info[i+count].right = e_table.edges[i].right;
		}
		count += e_table.count;
	}

	app_session->swrite(2, &count);
	if (count > 0)
		app_session->swrite(count * sizeof(server_edge_info), d_info);
	app_session->sync();

	if (d_buffer != buf) {
		grFree(d_buffer);
		grFree(d_info);
	}
}

/*-------------------------------------------------------------*/

void gr_get_height(TSession *app_session) {
	font_height			height;
	SFont				font;
	
	font.ReadPacket(app_session);
	font.GetHeight(&height);
	app_session->swrite(sizeof(height), &height);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_font_flags_info(TSession *app_session) {	
	uint16		buffer[2];
	uint16		faces;
	uint32		flags;
	
	app_session->sread(4, (void*)buffer);
	flags = fc_get_style_file_flags_and_faces(buffer[0], buffer[1], &faces);
	app_session->swrite(4, (void*)&flags);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_font_bbox(TSession *app_session) {	
	float		rect[4];
	uint16		buffer[2];
	uint32		blocks[4];
	
	app_session->sread(4, (void*)buffer);
	fc_get_style_bbox_and_blocks(buffer[0], buffer[1], rect, blocks);
	app_session->swrite(4*sizeof(float), (void*)rect);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_font_blocks(TSession *app_session) {	
	float		rect[4];
	uint16		buffer[2];
	uint32		blocks[4];
	uint64		buf64;
	
	app_session->sread(4, (void*)buffer);
	fc_get_style_bbox_and_blocks(buffer[0], buffer[1], rect, blocks);
	buf64 = (uint64)blocks[0] + (((uint64)blocks[1])<<32);
	app_session->swrite(sizeof(uint64), (void*)&buf64);
	buf64 = (uint64)blocks[2] + (((uint64)blocks[3])<<32);
	app_session->swrite(sizeof(uint64), (void*)&buf64);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_glyphs_bbox(TSession *app_session) {	
	int                 i;
	char				command[2];
	uchar               buf[16];
	uchar               *d_buffer;
	const uchar         *str;
	float               info[16*4];
	float               *d_info;
	float               delta_nsp, delta_sp;
	double				offset_h, offset_v;
	ushort              count;
	ushort              length;
	SFont				font;
	fc_bbox_table		b_table;

	font.ReadPacket(app_session);
	app_session->sread(2, (void*)&command);
	app_session->sread(4, (void*)&delta_nsp);
	app_session->sread(4, (void*)&delta_sp);
	app_session->sread(2, (void*)&length);

	if (length > 15) {
		d_buffer = (uchar*)grMalloc(length+1,"d_buffer",MALLOC_CANNOT_FAIL);
		d_info = (float*)grMalloc(4*sizeof(float)*(length+1),"d_info",MALLOC_CANNOT_FAIL);
	}
	else {
		d_buffer = buf;
		d_info = info;
	}

	app_session->sread(length, d_buffer);
	d_buffer[length] = 0;

	offset_h = offset_v = 0.5;

	count = 0;
	str = d_buffer;
	while (str != 0L) {
		str = font.GetBBox(&b_table, str, delta_nsp, delta_sp,
							command[0], command[1], &offset_h, &offset_v);
		for (i=0; i<b_table.count; i++) {
			d_info[(i+count)*4] = b_table.bbox[i].left;
			d_info[(i+count)*4+1] = b_table.bbox[i].top;
			d_info[(i+count)*4+2] = b_table.bbox[i].right;
			d_info[(i+count)*4+3] = b_table.bbox[i].bottom;
		}
		count += b_table.count;
	}

	app_session->swrite(2, &count);
	if (count > 0)
		app_session->swrite(count * 4*sizeof(float), d_info);
	app_session->sync();

	if (d_buffer != buf) {
		grFree(d_buffer);
		grFree(d_info);
	}
}

/*-------------------------------------------------------------*/

void gr_get_strings_bbox(TSession *app_session) {	
	uchar               buf[128];
	uchar				*d_buffer;
	int32               i, count;
	float               delta_nsp, delta_sp;
	fc_bbox				bbox;
	uint16				length, max_length;
	uint32				mode;
	SFont				font;

	font.ReadPacket(app_session);
	app_session->sread(4, (void*)&mode);
	app_session->sread(4, (void*)&count);

	max_length = 128;
	d_buffer = buf;	
	for (i=0; i<count; i++) {
		app_session->sread(4, (void*)&delta_nsp);
		app_session->sread(4, (void*)&delta_sp);
		app_session->sread(2, (void*)&length);

		if (length > (max_length-1)) {
			if (d_buffer != buf)
				grFree(d_buffer);
			max_length = length+1;
			d_buffer = (uchar*)grMalloc(max_length,"d_buffer",MALLOC_CANNOT_FAIL);
		}
	
		app_session->sread(length, d_buffer);
		d_buffer[length] = 0;
		font.GetStringBBox(d_buffer, delta_nsp, delta_sp, mode, &bbox);
		app_session->swrite(sizeof(fc_bbox), &bbox);
	}

	app_session->sync();

	if (d_buffer != buf)
		grFree(d_buffer);
}

/*-------------------------------------------------------------*/

void gr_get_has_glyphs(TSession *app_session) {	
	bool				*has;
	uchar                *d_buffer;
	ushort              length;
	SFont				font;

	font.ReadPacket(app_session);
	app_session->sread(2, (void*)&length);

	d_buffer = (uchar*)grMalloc(length+1,"d_buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(length, d_buffer);
	d_buffer[length] = 0;

	has = (bool*)grMalloc(length+1,"has",MALLOC_CANNOT_FAIL);
	length = font.HasGlyphs(d_buffer, has);
	
	app_session->swrite(2, &length);
	if (length > 0)
		app_session->swrite(length, has);
	app_session->sync();

	grFree(d_buffer);
	grFree(has);
}

/*-------------------------------------------------------------*/

void gr_get_font_ids(TSession *app_session) {
	uint         flags;
	ushort       buffer[3];
	uint32		 extended_faces;
	font_style   s_name;
	font_family  f_name;
	
	app_session->sread(2, buffer+0);
	app_session->sread(sizeof(font_family), f_name);
	app_session->sread(sizeof(font_style), s_name);
	app_session->sread(4, &extended_faces);

	if (f_name[0] != 0)
		buffer[0] = fc_get_family_id(f_name);	
	if (extended_faces != 0xffffffff)
		buffer[1] = fc_get_closest_face_style(buffer[0], extended_faces);
	else if (s_name[0] != 0)
		buffer[1] = fc_get_style_id(buffer[0], s_name);
	else
		buffer[1] = fc_get_default_style_id(buffer[0]);
	flags = fc_get_style_file_flags_and_faces(buffer[0], buffer[1], buffer+2);
	
	if (extended_faces != 0xffffffff) {
		// we can algorithmically generate bold and italic faces, so always
		// allow those to be set.
		bool regular = false;
		if (buffer[2] == B_REGULAR_FACE) {
			regular = true;
			buffer[2] = 0;
		}
		buffer[2] |= (extended_faces&(B_BOLD_FACE|B_ITALIC_FACE));
		if (buffer[2] == 0 && regular)
			buffer[2] = B_REGULAR_FACE;
	}
	
	app_session->swrite(6, buffer);
	app_session->swrite(4, &flags);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_font_names(TSession *app_session) {
	int          i;
	char         *name;
	ushort       buffer[2];
	font_style   s_name;
	font_family  f_name;
	
	app_session->sread(4, buffer);

	name = fc_get_family_name(buffer[0]);
	if (name == 0L)
		f_name[0] = 0;
	else
		for (i=0; i<sizeof(font_family); i++) {
			f_name[i] = name[i];
			if (name[i] == 0) break;
		}

	name = fc_get_style_name(buffer[0], buffer[1]);
	if (name == 0L)
		s_name[0] = 0;
	else
		for (i=0; i<sizeof(font_family); i++) {
			s_name[i] = name[i];
			if (name[i] == 0) break;
		}
	
	app_session->swrite(sizeof(font_family), (void*)&f_name[0]);
	app_session->swrite(sizeof(font_style), (void*)&s_name[0]);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_family_list(TSession *app_session) {
	int          i, j, ref_id;
	int          count;
	char         *name;
	uchar        flag;
	font_family  f_name;

	count = fc_get_family_count(&ref_id);
	app_session->swrite_l(count);
	for (i=0; i<count; i++) {
		name = fc_next_family(&ref_id, &flag);
		for (j=0; j<sizeof(font_family); j++) {
			f_name[j] = name[j];
			if (name[j] == 0) break;
		}
		app_session->swrite(sizeof(font_family), (void*)&f_name[0]);
		app_session->swrite(1, (void*)&flag);
	}
	app_session->swrite(sizeof(bigtime_t), (void*)&b_font_change_time);
	app_session->sync();
	fc_release_family_count();
}

/*-------------------------------------------------------------*/

void gr_get_style_list(TSession *app_session) {
	int          i, j, f_id, ref_id;
	int          count;
	char         *name;
	uchar        flag;
	uint16		 face;
	font_style   s_name;
	font_family  f_name;
	
	app_session->sread(sizeof(font_family), f_name);
	f_id = fc_get_family_id(f_name);
	if (f_id < 0)
		app_session->swrite_l(0);
	else {
		count = fc_get_style_count(f_id, &ref_id);
		app_session->swrite_l(count);
		for (i=0; i<count; i++) {
			name = fc_next_style(f_id, &ref_id, &flag, &face);
			for (j=0; j<sizeof(font_style); j++) {
				s_name[j] = name[j];
				if (name[j] == 0) break;
			}
			app_session->swrite(sizeof(font_style), (void*)&s_name[0]);
			app_session->swrite(1, (void*)&flag);
			app_session->swrite(2, (void*)&face);
		}
	}
	app_session->sync();
	fc_release_style_count();
}

/*-------------------------------------------------------------*/

void gr_get_tuned_font_list(TSession *app_session) {
	int                 i, ref, count;
	uint16				buffer[2];
	fc_tuned_font_info	info;
	
	app_session->sread(4, (void*)buffer);
	count = fc_get_tuned_count(buffer[0], buffer[1]);
	app_session->swrite_l(count);
	ref = 0;
	for (i=0; i<count; i++) {
		ref = fc_next_tuned(buffer[0], buffer[1], ref, &info);
		app_session->swrite(sizeof(info), &info);
	}
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_font_change_time(TSession *app_session) {
	fc_lock_font_list();
	fc_set_default_font_file_list();
	fc_scan_all_dir(FALSE);
	fc_save_font_file_list();
	fc_unlock_font_list();

	app_session->swrite(sizeof(bigtime_t), (void*)&b_font_change_time);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_standard_fonts(TSession *app_session) {
	SFont		font;
	float        size;
	int32        index;
	font_style   s_name;
	font_family  f_name;
	
	app_session->sread(4, &index);

	if ((index >= -100) && (index < 0)) {
		index += 100;
		SFont::GetSystemFont((font_which)index, &font);
	}
	else {
		SFont::GetStandardFont((font_which)index, &font);
	}
	
	font.WritePacket(app_session);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_set_standard_fonts(TSession *app_session) {
	int32        index;
	SFont        font;
	
	app_session->sread(4, &index);
	if (font.ReadPacket(app_session) == B_OK) {
		SFont::SetStandardFont((font_which)index, font);
	}
}

/*-------------------------------------------------------------*/

void gr_get_font_cache_settings(TSession *app_session) {
	int32                  version, id;
	fc_font_cache_settings set;
	
	app_session->sread(4, &version);
	app_session->sread(4, &id);

	if ((version == 0) &&
		(id & (B_SCREEN_FONT_CACHE|B_PRINTING_FONT_CACHE)) &&
		(id & (B_DEFAULT_CACHE_SETTING|B_APP_CACHE_SETTING))) {
		fc_lock_font_list();
		if (id & B_APP_CACHE_SETTING)
			fc_get_app_settings((id&B_SCREEN_FONT_CACHE)?FC_GRAY_SCALE:FC_BLACK_AND_WHITE,
								getpid(), &set);
		else if (id & B_SCREEN_FONT_CACHE)
			fc_gray_system_status.get_settings(&set);
		else
			fc_bw_system_status.get_settings(&set);
		app_session->swrite_l(sizeof(fc_font_cache_settings));
		app_session->swrite(sizeof(fc_font_cache_settings), &set);
		fc_unlock_font_list();
	}
	else app_session->swrite_l(0);
	
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_set_font_cache_settings(TSession *app_session) {
	void                   *buffer;
	int32                  version, id, size, err;
	fc_font_cache_settings *set0;
	
	app_session->sread(4, &version);
	app_session->sread(4, &id);
	app_session->sread(4, &size);
	buffer = (void*)grMalloc(size,"buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(size, buffer);

	err = B_ERROR;
	
	if ((id & (B_SCREEN_FONT_CACHE|B_PRINTING_FONT_CACHE)) &&
		(id & (B_DEFAULT_CACHE_SETTING|B_APP_CACHE_SETTING))) {
		bool save = false;
		fc_lock_font_list();
		set0 = (fc_font_cache_settings*)buffer;
		if (id & B_APP_CACHE_SETTING)
			fc_set_app_settings((id&B_SCREEN_FONT_CACHE)?FC_GRAY_SCALE:FC_BLACK_AND_WHITE,
								getpid(), set0);
		else if (id & B_SCREEN_FONT_CACHE) {
			fc_gray_system_status.set_settings(set0, FC_GRAY_SCALE);
			save = true;
		}
		else {
			fc_bw_system_status.set_settings(set0, FC_BLACK_AND_WHITE);
			save = true;
		}
		fc_check_cache_settings();
		fc_unlock_font_list();
		if (save) {
			SFont::SavePrefs();
			// This is a gross way to get the window decor to
			// use the currently set fonts.
			if (SFont::FontsChanged()) {
				ReloadDecor();
				reset_decor();
			}
		}
	}
	
	grFree(buffer);
	app_session->swrite_l(err);
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_remove_font_dir(TSession *app_session) {
	char      *buffer;
	int32     size;
	
	app_session->sread(4, &size);
	buffer = (char*)grMalloc(size,"buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(size, buffer);
	
	fc_lock_font_list();
	fc_remove_dir(buffer);
	fc_unlock_font_list();

	grFree(buffer);
	app_session->swrite_l(B_NO_ERROR);  /* to be fixed */
}

/*-------------------------------------------------------------*/

void gr_change_font_dir_status(TSession *app_session) {
	char      *buffer;
	int32     size, status;
	
	app_session->sread(4, &size);
	app_session->sread(4, &status);
	buffer = (char*)grMalloc(size,"buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(size, buffer);
	
	fc_lock_font_list();
	fc_set_dir(buffer, status);
	fc_unlock_font_list();

	grFree(buffer);
	app_session->swrite_l(B_NO_ERROR); /* to be fixed */
}

/*-------------------------------------------------------------*/

void gr_change_font_file_status(TSession *app_session) {
	char      *buffer;
	int32     size, dir_id, status;
	
	app_session->sread(4, &size);
	app_session->sread(4, &dir_id);
	app_session->sread(4, &status);
	buffer = (char*)grMalloc(size,"buffer",MALLOC_CANNOT_FAIL);
	app_session->sread(size, buffer);
	
	fc_lock_font_list();
	fc_set_file(dir_id, buffer, (status)?FC_ENABLE_FILE:FC_DISABLE_FILE);
	fc_unlock_font_list();

	grFree(buffer);
	app_session->swrite_l(B_NO_ERROR); /* to be fixed */
}

/*-------------------------------------------------------------*/

void gr_get_font_dir_list(TSession *app_session) {
	int          i, ref_id, dir_id;
	int          count, length;
	char         *name;
	uchar        flag, type;
	
	fc_lock_font_list();
	count = fc_get_dir_count(&ref_id);
	app_session->swrite_l(count);
	for (i=0; i<count; i++) {
		name = fc_next_dir(&ref_id, &flag, &dir_id, &type);
		length = strlen(name)+1;
		app_session->swrite_l(dir_id);
		app_session->swrite_l(length);
		app_session->swrite(length, (void*)name);
		app_session->swrite(1, (void*)&flag);
		app_session->swrite(1, (void*)&type);
	}
	fc_unlock_font_list();
	app_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_font_file_list(TSession *app_session) {
	int            i, j, ref_id;
	int            count, length;
	char           *name;
	font_style     s_name;
	font_family    f_name;
	fc_file_entry  *fe;
	
	fc_lock_font_list();
	count = fc_get_file_count(&ref_id);
	app_session->swrite_l(count);
	for (j=0; j<count; j++) {
		fe = fc_										\
		next_line:																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);												\
	}																							\
}																								\
																								\
static void AlphaPreprocessName (																\
	RenderContext *	context,																	\
	RenderSubPort *	port,																		\
	fc_string *		str)																		\
{																								\
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;										\
	uint32																						\
		*colors = port->cache->fontColors,														\
		*alphas = port->cache->fontColors+16;													\
	int8 endianess = port->canvas->pixels.endianess;											\
	rgb_color fc = context->foreColor;															\
																								\
	if (fc.alpha == 0) {																		\
		port->cache->fDrawString = noop;														\
		return;																					\
	};																							\
																								\
	if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {							\
		alpha = fc.alpha;																		\
		alpha += (alpha >> 7);																	\
		da = (fc.alpha * alpha);																\
		dr = (fc.red * alpha);																	\
		dg = (fc.green * alpha);																\
		db = (fc.blue * alpha);																	\
		colors[0] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[8] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256-alpha;																	\
		port->cache->fontColors[24] = (uint32)AlphaDrawChar;									\
	} else {																					\
		da = alpha = fc.alpha;																	\
		alpha += (alpha >> 7);																	\
		colors[0] = 0L;																			\
		colors[7] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[15] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256;																		\
		alphas[7] = 256-alpha;																	\
																								\
		da = (da<<8)/7;																			\
		a0 = 0x80;																				\
																								\
		for (i=1; i<7; i++) {																	\
			a0 += da;																			\
			alpha = (a0+0x80) >> 8;																\
			alpha += (alpha >> 7);																\
			colors[i] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;					\
			colors[i+8] = (((uint32)fc.green)<<8) * alpha;										\
			alphas[i] = 256-alpha;																\
		}																						\
		port->cache->fontColors[24] = (uint32)AlphaDrawCharAA;									\
	};																							\
	port->cache->fDrawString = AlphaDrawString;													\
	AlphaDrawString(context,port,str);															\
};

#define PrecalcColorsFunc(PrecalcColorsFuncName)										\
static void PrecalcColorsFuncName (rgb_color fc, rgb_color bc, uint32 *colors)			\
{																						\
	int32 a0, r0, g0, b0, da, dr, dg, db, i;											\
																						\
	colors[0] = ComponentsToPixel((bc.red<<8),(bc.green<<8),(bc.blue<<8));				\
	colors[7] = ComponentsToPixel((fc.red<<8),(fc.green<<8),(fc.blue<<8));				\
	r0 = ((uint32)bc.red)<<8;															\
	g0 = ((uint32)bc.green)<<8;															\
	b0 = ((uint32)bc.blue)<<8;															\
	dr = ((fc.red<<8)-r0)>>3;															\
	dg = ((fc.green<<8)-g0)>>3;															\
	db = ((fc.blue<<8)-b0)>>3;															\
	r0 += 0x80;																			\
	g0 += 0x80;																			\
	b0 += 0x80;																			\
	for (i=1; i<7; i++) {																\
		r0 += dr; g0 += dg; b0 += db;													\
		colors[i] = ComponentsToPixel(r0,g0,b0);										\
	};																					\
	for (i=8; i<15; i++) colors[i] = colors[7];											\
};

#define DrawOverGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawAlphaGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawInvertGrayChar1516(FuncName)												\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,otherColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = 0xFFFF - *dst_ptr;											\
			else if (gray != 0) {														\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				otherColor = CanvasTo898(colorIn) * (8-gray);							\
				colorIn = 0xFFFF - colorIn;												\
				colorIn = (CanvasTo898(colorIn) * gray) + otherColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawBlendGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				colorIn = ((colorIn*(16-gray)) + theColor)>>1;							\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#define DrawOtherGrayChar1516(FuncName)													\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray,energy;																\
	uint32 colorIn,theColor;															\
	uint16 *dst_ptr;																	\
	uint8 *src_ptr,*dst_rowBase;														\
	bool gr;																			\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+dst.left*2);			\
	for (j=src.top; j<=src.bottom; j++) {												\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray = 8-(gray+(gray>>2));												\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				switch (mode) {															\
					case OP_SUB:														\
						theColor = (theColor|(0x8040100<<3)) - colorIn*gray;			\
						theColor &= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor &= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_ADD:														\
						theColor = theColor + colorIn*gray;								\
						theColor |= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor |= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_MIN:														\
					case OP_MAX:														\
						theColor = (colorIn*gray + theColor);							\
						gr =(															\
							 ((colorIn & (0x7F80000<<3))>>18) +							\
							 ((colorIn & (0x003FE00<<3))>> 9) +							\
							 ((colorIn & (0x00000FF<<3))<< 1)							\
							) >															\
							(															\
							 ((theColor & (0x7F80000<<3))>>18) +						\
							 ((theColor & (0x003FE00<<3))>> 9) +						\
							 ((theColor & (0x00000FF<<3))<< 1)							\
							);															\
						if ((gr && (mode==OP_MAX)) ||									\
					       (!gr && (mode==OP_MIN))) {									\
							dst_ptr++;													\
							continue;													\
						};																\
				};																		\
				*dst_ptr = BackToCanvas(theColor);										\
			};																			\
			dst_ptr++;																	\
		};																				\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
		src_ptr += src_row;																\
	};																					\
};

#if ROTATE_DISPLAY
#define DrawOverGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawAlphaGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = pixel;														\
			else if (gray != 0) {														\
 				theColor = colors[gray];												\
 				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = (CanvasTo898(colorIn) * (8-gray)) + theColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawInvertGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,otherColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray == 7)																\
				*dst_ptr = 0xFFFF - *dst_ptr;											\
			else if (gray != 0) {														\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				otherColor = CanvasTo898(colorIn) * (8-gray);							\
				colorIn = 0xFFFF - colorIn;												\
				colorIn = (CanvasTo898(colorIn) * gray) + otherColor;					\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawBlendGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray;																		\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray += (gray+1)>>3;													\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				colorIn = ((colorIn*(16-gray)) + theColor)>>1;							\
				*dst_ptr = BackToCanvas(colorIn);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

#define DrawOtherGrayRotatedChar1516(FuncName)											\
static void FuncName (	uchar *dst_base, int dst_row, rect dst,							\
						uchar *src_base, int src_row, rect src,							\
						uint32 *colors, uint32 pixel, int mode)							\
{																						\
	int32 i,j,gray,energy;																\
	uint32 colorIn,theColor;															\
	uint16	*dst_ptr, *dst_ptr2;														\
	uint8 *src_ptr;																		\
	bool gr;																			\
	src_ptr = src_base+src_row*src.top;													\
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;							\
	for (j=src.top; j<=src.bottom; j++) {												\
		dst_ptr = dst_ptr2;																\
		for (i=src.left; i<=src.right; i++) {											\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;									\
			if (gray != 0) {															\
 				theColor = colors[gray];												\
				gray = 8-(gray+(gray>>2));												\
				colorIn = *dst_ptr;														\
				colorIn = CanvasTo898(colorIn);											\
				switch (mode) {															\
					case OP_SUB:														\
						theColor = (theColor|(0x8040100<<3)) - colorIn*gray;			\
						theColor &= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor &= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_ADD:														\
						theColor = theColor + colorIn*gray;								\
						theColor |= ((0x8000100<<3) - ((theColor&(0x8000100<<3))>>8));	\
						theColor |= ((0x0040000<<3) - ((theColor&(0x0040000<<3))>>9));	\
						break;															\
					case OP_MIN:														\
					case OP_MAX:														\
						theColor = (colorIn*gray + theColor);							\
						gr =(															\
							 ((colorIn & (0x7F80000<<3))>>18) +							\
							 ((colorIn & (0x003FE00<<3))>> 9) +							\
							 ((colorIn & (0x00000FF<<3))<< 1)							\
							) >															\
							(															\
							 ((theColor & (0x7F80000<<3))>>18) +						\
							 ((theColor & (0x003FE00<<3))>> 9) +						\
							 ((theColor & (0x00000FF<<3))<< 1)							\
							);															\
						if ((gr && (mode==OP_MAX)) ||									\
					       (!gr && (mode==OP_MIN))) {									\
							dst_ptr++;													\
							continue;													\
						};																\
				};																		\
				*dst_ptr = BackToCanvas(theColor);										\
			};																			\
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);								\
		};																				\
		dst_ptr2 -= 1;																	\
		src_ptr += src_row;																\
	};																					\
};

// --------------------------------------------------------
// Rotated Alpha text functions
// --------------------------------------------------------

#define AlphaPreprocessRotatedFuncs(AlphaPreprocessName,AlphaDrawString,AlphaDrawChar,AlphaDrawCharAA)	\
static void AlphaDrawCharAA (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int32 i,j,gray;																				\
	uint32 colorIn,theColor;																	\
	uint16 *dst_ptr;																			\
	uint8 *src_ptr;																				\
	uint16 *dst_rowBase;																		\
	src_ptr = src_base+src_row*src.top;															\
	dst_rowBase = (uint16 *)(dst_base+dst_row*dst.top)+dst.right;								\
	for (j=src.top; j<=src.bottom; j++) {														\
		dst_ptr = dst_rowBase;																	\
		for (i=src.left; i<=src.right; i++) {													\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;											\
			if (gray != 0) {																	\
				theColor = *dst_ptr;															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha[gray]) +									\
						colors[gray  ]) >> 8) & 0x00FF00FF) |									\
					(((((colorIn & 0x0000FF00) * alpha[gray]) +									\
						colors[gray+8]) >> 8) & 0x0000FF00) ;									\
				*dst_ptr = ComponentsToPixel(colorIn>>8,colorIn,colorIn<<8);					\
			};																					\
			dst_ptr += dst_row/2;																\
		};																						\
		src_ptr += src_row;																		\
		dst_rowBase-=1;																			\
	};																							\
};																								\
																								\
static void AlphaDrawChar	 (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
}																								\
																								\
static void AlphaPreprocessName (																\
	RenderContext *	context,																	\
	RenderSubPort *	port,																		\
	fc_string *		str)																		\
{																								\
	uint32 i, a0, r0, g0, b0, da, dr, dg, db, alpha, pixIn;										\
	uint32																						\
		*colors = port->cache->fontColors,														\
		*alphas = port->cache->fontColors+16;													\
	int8 endianess = port->canvas->pixels.endianess;											\
	rgb_color fc = context->foreColor;															\
																								\
	if (fc.alpha == 0) {																		\
		port->cache->fDrawString = noop;														\
		return;																					\
	};																							\
																								\
	if (context->fontContext.BitsPerPixel() == FC_BLACK_AND_WHITE) {							\
		alpha = fc.alpha;																		\
		alpha += (alpha >> 7);																	\
		da = (fc.alpha * alpha);																\
		dr = (fc.red * alpha);																	\
		dg = (fc.green * alpha);																\
		db = (fc.blue * alpha);																	\
		colors[0] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[8] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256-alpha;																	\
		port->cache->fontColors[24] = (uint32)AlphaDrawChar;									\
	} else {																					\
		da = alpha = fc.alpha;																	\
		alpha += (alpha >> 7);																	\
		colors[0] = 0L;																			\
		colors[7] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;						\
		colors[15] = (((uint32)fc.green)<<8) * alpha;											\
		alphas[0] = 256;																		\
		alphas[7] = 256-alpha;																	\
																								\
		da = (da<<8)/7;																			\
		a0 = 0x80;																				\
																								\
		for (i=1; i<7; i++) {																	\
			a0 += da;																			\
			alpha = (a0+0x80) >> 8;																\
			alpha += (alpha >> 7);																\
			colors[i] = ((((uint32)fc.red)<<16) | (((uint32)fc.blue))) * alpha;					\
			colors[i+8] = (((uint32)fc.green)<<8) * alpha;										\
			alphas[i] = 256-alpha;																\
		}																						\
		port->cache->fontColors[24] = (uint32)AlphaDrawCharAA;									\
	};																							\
	port->cache->fDrawString = AlphaDrawString;													\
	AlphaDrawString(context,port,str);															\
};


#endif



static void AlphaDrawString(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	AlphaDrawChar drawChar = (AlphaDrawChar)colors[24];
	
	grAssertLocked(context, port);

	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
		if (clipbox.top > str->bbox.bottom) return;
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}
			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;
			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors+16);
		}		
	}
}

#if ROTATE_DISPLAY

static void AlphaDrawRotatedString(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	RenderCache *cache = port->cache;
	region *	clip = port->drawingClip;
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	AlphaDrawChar drawChar = (AlphaDrawChar)colors[24];

	grAssertLocked(context, port);

	DisplayRotater	*rotater = port->rotater;

	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
		if (clipbox.top > str->bbox.bottom) return;
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}
			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;
			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			rotater->RotateRect(&cur, &cur);
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors+16);
		}		
	}
}

#endif

// Native endianess 15-bit
#define SourceFormatBits		15
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x7C00)<<9) | ((canvas&0x03E0)<<5) | (canvas&0x001F)))
#define BackToCanvas(col) \
	(0x8000 | ((col>>12)&0x7C00) | ((col>>8)&0x03E0) | ((col>>3)&0x001F))
#define ComponentsToPixel(r,g,b) (0x8000 | ((r>>1)&0x7C00) | ((g>>6)&0x03E0) | ((b>>11)&0x001F))
AlphaPreprocessFuncs(AlphaNative15Preprocess,AlphaDrawString,AlphaNative15DrawChar,AlphaNative15DrawCharAA)
PrecalcColorsFunc(ColorsNative15)
DrawOverGrayChar1516(DrawOverGrayChar15Native)
DrawInvertGrayChar1516(DrawInvertGrayChar15Native)
DrawBlendGrayChar1516(DrawBlendGrayChar15Native)
DrawOtherGrayChar1516(DrawOtherGrayChar15Native)
DrawOtherBWChar1516(DrawOtherBWChar15Native)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Native endianess 16-bit
#define SourceFormatBits		16
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0xF800)<<8) | ((canvas&0x07E0)<<4) | (canvas&0x001F)))
#define BackToCanvas(col) \
	(((col>>11)&0xF800) | ((col>>7)&0x07E0) | ((col>>3)&0x001F))
#define ComponentsToPixel(r,g,b) ((r&0xF800) | ((g>>5)&0x07E0) | ((b>>11)&0x001F))
AlphaPreprocessFuncs(AlphaNative16Preprocess,AlphaDrawString,AlphaNative16DrawChar,AlphaNative16DrawCharAA)
PrecalcColorsFunc(ColorsNative16)
DrawOverGrayChar1516(DrawOverGrayChar16Native)
DrawInvertGrayChar1516(DrawInvertGrayChar16Native)
DrawBlendGrayChar1516(DrawBlendGrayChar16Native)
DrawOtherGrayChar1516(DrawOtherGrayChar16Native)
DrawOtherBWChar1516(DrawOtherBWChar16Native)
#if ROTATE_DISPLAY
AlphaPreprocessRotatedFuncs(AlphaNative16PreprocessRotated,AlphaDrawRotatedString,AlphaNative16DrawRotatedChar,AlphaNative16DrawRotatedCharAA)
DrawOverGrayRotatedChar1516(DrawOverGrayRotatedChar16)
DrawInvertGrayRotatedChar1516(DrawInvertGrayRotatedChar16)
DrawBlendGrayRotatedChar1516(DrawBlendGrayRotatedChar16)
DrawOtherGrayRotatedChar1516(DrawOtherGrayRotatedChar16)
#endif
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Opposite endianess 15-bit
#define SourceFormatBits		15
#define SourceFormatEndianess	NonHostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x007C)<<17) | ((canvas&0x0003)<<13) | ((canvas&0xE000)>>3) | ((canvas&0x1F00)>>8)))
#define BackToCanvas(col) \
	(0x0080 | ((col>>20)&0x007C) | ((col>>16)&0x0003) | (col&0xE000) | ((col<<5)&0x1F00))
#define ComponentsToPixel(r,g,b) (0x0080 | ((r>>9)&0x007C) | ((g>>14)&0x0003) | ((g<<2)&0xE000) | ((b>>3)&0x1F00))
AlphaPreprocessFuncs(AlphaOpp15Preprocess,AlphaDrawString,AlphaOpp15DrawChar,AlphaOpp15DrawCharAA)
PrecalcColorsFunc(ColorsOpp15)
DrawOverGrayChar1516(DrawOverGrayChar15Opp)
DrawInvertGrayChar1516(DrawInvertGrayChar15Opp)
DrawBlendGrayChar1516(DrawBlendGrayChar15Opp)
DrawOtherGrayChar1516(DrawOtherGrayChar15Opp)
DrawOtherBWChar1516(DrawOtherBWChar15Opp)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

// Opposite endianess 16-bit
#define SourceFormatBits		16
#define SourceFormatEndianess	NonHostEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#define CanvasToARGB(a,b) 		ConvertPixelFormat(a,b);
#define CanvasTo898(canvas) \
	((((canvas&0x00F8)<<16) | ((canvas&0x0007)<<12) | ((canvas&0xE000)>>4) | ((canvas&0x1F00)>>8)))
#define BackToCanvas(col) \
	(((col>>19)&0x00F8) | ((col>>15)&0x0007) | ((col<<1)&0xE000) | ((col<<5)&0x1F00))
#define ComponentsToPixel(r,g,b) (((r>>8)&0x00F8) | ((g>>13)&0x0007) | ((g<<3)&0xE000) | ((b>>3)&0x1F00))
AlphaPreprocessFuncs(AlphaOpp16Preprocess,AlphaDrawString,AlphaOpp16DrawChar,AlphaOpp16DrawCharAA)
PrecalcColorsFunc(ColorsOpp16)
DrawOverGrayChar1516(DrawOverGrayChar16Opp)
DrawInvertGrayChar1516(DrawInvertGrayChar16Opp)
DrawBlendGrayChar1516(DrawBlendGrayChar16Opp)
DrawOtherGrayChar1516(DrawOtherGrayChar16Opp)
DrawOtherBWChar1516(DrawOtherBWChar16Opp)
#undef CanvasTo898
#undef BackToCanvas
#undef ComponentsToPixel
#undef CanvasToARGB

Precalc precalcTable[2][2] =
{
	{ ColorsNative15, ColorsNative16 },
	{ ColorsOpp15, ColorsOpp16 }
};

DrawOtherBWChar drawOtherBWChar[2][2] =
{
	{ DrawOtherBWChar15Native, DrawOtherBWChar16Native },
	{ DrawOtherBWChar15Opp, DrawOtherBWChar16Opp }
};

void * drawOtherChar[11][2][2] =
{
	{	{ NULL,NULL }, { NULL,NULL }  },
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ DrawInvertGrayChar15Native, DrawInvertGrayChar16Native },
		{ DrawInvertGrayChar15Opp, DrawInvertGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawBlendGrayChar15Native, DrawBlendGrayChar16Native },
		{ DrawBlendGrayChar15Opp, DrawBlendGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOtherGrayChar15Native, DrawOtherGrayChar16Native },
		{ DrawOtherGrayChar15Opp, DrawOtherGrayChar16Opp }
	},
	{
		{ DrawOverGrayChar15Native, DrawOverGrayChar16Native },
		{ DrawOverGrayChar15Opp, DrawOverGrayChar16Opp }
	},
	{
		{ NULL, NULL },
		{ NULL, NULL }
	}
};

#if ROTATE_DISPLAY
void *drawOtherRotatedChar[11] = {
	NULL,
	DrawOverGrayRotatedChar16,
	DrawOverGrayRotatedChar16,
	DrawInvertGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawBlendGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOtherGrayRotatedChar16,
	DrawOverGrayRotatedChar16,
	NULL
};
#endif

/* main entry to draw black&white string into an 32 bits port, both endianess */
static void DrawStringNonAAOther(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int			i, a, r, g, b, index, energy0;
	int			cur_row;
	int			mode_direct, endianess;
	rect		cur, src, clipbox;
	uint32		pixel;
	uchar *		src_base, *cur_base;
	fc_char *	my_char;
	fc_point *	my_offset;
	int			mode,k=0;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	DrawOtherBWChar drawChar = NULL;

	grAssertLocked(context, port);

#if ROTATE_DISPLAY
	if (port->canvasIsRotated)
		return;
	/* PIERRE TO BE CONTINUED */
#endif
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;

	if (mode == OP_ERASE)
		pixel = port->cache->backColorCanvasFormat;
	else
		pixel = port->cache->foreColorCanvasFormat;

	if ((mode <= OP_ERASE) || (mode == OP_SELECT))
		drawChar = DrawCopyBWChar1516;
	else
		drawChar = drawOtherBWChar[endianess!=HOST_ENDIANESS][port->canvas->pixels.pixelFormat-PIX_15BIT];

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
		if (clipbox.top > str->bbox.bottom) return;
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0)
				continue;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right)
					continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left)
					continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}
			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;
			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom)
					continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top)
					continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(cur_base, cur_row, cur, src_base, src, pixel, mode);
		}
	}
}

static void DrawGrayChar1516(	uchar *dst_base, int dst_row, rect dst,
								uchar *src_base, int src_row, rect src,
								uint32 *colors) {
	int		i, j;
	uint32	gray;
	uint16	*dst_ptr;
	uchar	*src_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr = (uint16*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		if (i&1) {
			if (((uint32)dst_ptr)&0x03) {
				gray = src_ptr[i>>1]&7;
				if (gray) dst_ptr[i] = colors[gray];
				i++;
				goto aligned;
			};
			gray = src_ptr[i>>1];
			unaligned:;
			while (i<=src.right-1) {
				gray = ((gray<<8)|src_ptr[(i+1)>>1]) & 0x777;
				if (gray & 0x770) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>8]) | ShiftToWord1(colors[(gray>>4)&7]);
				i+=2;
			};
			if (i == src.right) {
				gray &= 7;
				if (gray) dst_ptr[i] = colors[gray];
			}
		} else {
			if (((uint32)dst_ptr)&0x03) {
				gray = src_ptr[i>>1];
				if (gray>>4) dst_ptr[i] = colors[gray>>4];
				i++;
				goto unaligned;
			};
			aligned:
			while (i<src.right-2) {
				gray = src_ptr[i>>1];
				if (gray) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				gray = src_ptr[(i>>1)+1];
				if (gray) *((uint32*)(dst_ptr+i+2)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				i+=4;
			}
			while (i<=src.right-1) {
				gray = src_ptr[i>>1];
				if (gray) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
				i+=2;
			};
			if (i == src.right) {
				gray = src_ptr[i>>1]>>4;
				if (gray > 0) dst_ptr[i] = colors[gray];
			}
		};
		src_ptr += src_row;
		dst_ptr = (uint16*)((uchar*)dst_ptr+dst_row);
	}
}

static void DrawGrayCharX1516(	uchar *dst_base, int dst_row, rect dst,
								uchar *src2_base, int src2_row, rect src2,
								uchar *src_base, int src_row, rect src,
								uint32 *colors) {
	int        i, i2, j, gray;
	uint16     *dst_ptr;
	uchar      *src_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr = (uint16*)(dst_base+dst_row*dst.top)+(dst.left-src.left);
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		i2 = src2.left;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) dst_ptr[i] = colors[gray];
			i++; i2++;
		}
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			gray += (((src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7)<<4) |
					 ((src2_ptr[(i2+1)>>1]>>(4-((i2<<2)&4)))&7);
			if (gray > 0) *((uint32*)(dst_ptr+i)) = ShiftToWord0(colors[gray>>4]) | ShiftToWord1(colors[gray&7]);
			i+=2; i2+=2;
		}
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) dst_ptr[i] = colors[gray];
		}
		src_ptr += src_row;
		src2_ptr += src2_row;
		dst_ptr = (uint16*)((uchar*)dst_ptr+dst_row);
	}
}

static void DrawStringAACopy(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = port->cache->fontColors;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	
	grAssertLocked(context, port);
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	prev.top = -1048576;
	prev.left = -1048576;
	prev.right = -1048576;
	prev.bottom = -1048576;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
		if (clipbox.top > str->bbox.bottom) return;
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;

			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;

			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}

			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;

			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}

			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;		

			/* test character not-overlaping */
			if ((prev.bottom < cur.top) ||
				(prev.right < cur.left) ||
				(prev.left > cur.right) ||
				(prev.top > cur.bottom)) {
				DrawGrayChar1516(	cur_base, cur_row, cur,
									src_base, src_row, src, colors);
			/* characters overlaping */
			} else {
				/* calculate intersection of the 2 rect */
				if (prev.left > cur.left)
					inter.left = prev.left;
				else
					inter.left = cur.left;
				if (prev.right < cur.right)
					inter.right = prev.right;
				else
					inter.right = cur.right;
				if (prev.top > cur.top)
					inter.top = prev.top;
				else
					inter.top = cur.top;
				if (prev.bottom < cur.bottom)
					inter.bottom = prev.bottom;
				else
					inter.bottom = cur.bottom;
				/* left not overlaping band */	
				dh = inter.left-cur.left-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.left;
					tmp.right = cur.left+dh;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.left;
					src_tmp.right = src.left+dh;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					DrawGrayChar1516(	cur_base, cur_row, tmp,
										src_base, src_row, src_tmp, colors);
				}
				/* right not overlaping band */	
				dh = cur.right-inter.right-1;
				if (dh >= 0) {
					tmp.top = inter.top;
					tmp.left = cur.right-dh;
					tmp.right = cur.right;
					tmp.bottom = inter.bottom;
					src_tmp.top = src.top+(inter.top-cur.top);
					src_tmp.left = src.right-dh;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom+(inter.bottom-cur.bottom);
					DrawGrayChar1516(	cur_base, cur_row, tmp,
										src_base, src_row, src_tmp, colors);
				}
				/* top not overlaping band */	
				dv = inter.top-cur.top-1;
				if (dv >= 0) {
					tmp.top = cur.top;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.top+dv;
					src_tmp.top = src.top;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.top+dv;
					DrawGrayChar1516(	cur_base, cur_row, tmp,
										src_base, src_row, src_tmp, colors);
				}
				/* bottom not overlaping band */	
				dv = cur.bottom-inter.bottom-1;
				if (dv >= 0) {
					tmp.top = cur.bottom-dv;
					tmp.left = cur.left;
					tmp.right = cur.right;
					tmp.bottom = cur.bottom;
					src_tmp.top = src.bottom-dv;
					src_tmp.left = src.left;
					src_tmp.right = src.right;
					src_tmp.bottom = src.bottom;
					DrawGrayChar1516(	cur_base, cur_row, tmp,
										src_base, src_row, src_tmp, colors);
				}
				/* at last the horrible overlaping band... */
				src_tmp.top = src.top + (inter.top-cur.top);
				src_tmp.left = src.left + (inter.left-cur.left);
				src_tmp.right = src.right + (inter.right-cur.right);
				src_tmp.bottom = src.bottom + (inter.bottom-cur.bottom);
				src_prev_tmp.top = src_prev.top + (inter.top-prev.top);
				src_prev_tmp.left = src_prev.left + (inter.left-prev.left);
				src_prev_tmp.right = src_prev.right + (inter.right-prev.right);
				src_prev_tmp.bottom = src_prev.bottom + (inter.bottom-prev.bottom);
				DrawGrayCharX1516(	cur_base, cur_row, inter,
									src_prev_base, src_prev_row, src_prev_tmp,
									src_base, src_row, src_tmp, colors);
			}
			src_prev_row = src_row;
			src_prev_base = src_base;
			src_prev.top = src.top;
			src_prev.left = src.left;
			src_prev.right = src.right;
			src_prev.bottom = src.bottom;
			prev.top = cur.top;
			prev.left = cur.left;
			prev.right = cur.right;
			prev.bottom = cur.bottom;
		}
	}
};

static void DrawStringAAOther(
	RenderContext *	context,
	RenderSubPort *	port,
	fc_string *		str)
{
	RenderCache *	cache = port->cache;
	int         i, dh, dv, endianess;
	int         src_row, cur_row, src_prev_row, prev_dh, prev_dv, energy0;
	rect        prev, src_prev, cur, src, inter;
	rect        tmp, src_tmp, src_prev_tmp,clipbox;
	uint32 *	colors = cache->fontColors;
	region *	clip = port->RealDrawingRegion();
	const rect	*clipRects = clip->Rects();
	const int32	clipCount = clip->CountRects();
	uchar       *src_base, *cur_base, *src_prev_base;
	fc_char     *my_char;
	fc_point    *my_offset;
	rgb_color   fc,bc;
	int32		mode,k=0;
	DrawOtherChar drawChar = (DrawOtherChar)colors[15];
	
	grAssertLocked(context, port);
	
	cur_base = port->canvas->pixels.pixelData;
	cur_row = port->canvas->pixels.bytesPerRow;
	mode = context->drawOp;
	endianess = port->canvas->pixels.endianess;
	fc = context->foreColor;
	bc = context->backColor;

	if ((k=find_span_between(clip,str->bbox.top,str->bbox.bottom))==-1) return;
	while ((k>0) && (clipRects[k].top > str->bbox.top)) k--;
	while ((k>0) && (clipRects[k].top == clipRects[k-1].top)) k--;

	for (; k<clipCount; k++) {
		clipbox = clipRects[k];
		if (clipbox.top > str->bbox.bottom) return;
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0) continue;
			src_row = (src.right+2)>>1;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right) continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left) continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}
			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;
			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom) continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top) continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			drawChar(	cur_base, cur_row, cur,
						src_base, src_row, src,
						colors, colors[9], mode);
		}		
	}
}

#if ROTATE_DISPLAY
static void DrawRotatedGrayChar1516(uchar *dst_base, int dst_row, rect dst,
									uchar *src_base, int src_row, rect src,
									uint32 *colors) {
	int		i, j;
	uint32	gray;
	uint16	*dst_ptr, *dst_ptr2;
	uchar	*src_ptr;

	src_ptr = src_base+src_row*src.top;
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		dst_ptr = dst_ptr2;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			if (gray) *dst_ptr = colors[gray];
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);	// add one row
			i++;
		};
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			if (gray) {
				*dst_ptr = colors[gray>>4];
				dst_ptr[dst_row/2] = colors[gray&7];
			}
			dst_ptr += dst_row;		// this add two rows (uint16*)
			i+=2;
		};
		if (i == src.right) {
			gray = src_ptr[i>>1]>>4;
			if (gray) *dst_ptr = colors[gray];
		}
		src_ptr += src_row;
		dst_ptr2 -= 1;
	}
}

static void DrawRotatedGrayCharX1516(	uchar *dst_base, int dst_row, rect dst,
										uchar *src2_base, int src2_row, rect src2,
										uchar *src_base, int src_row, rect src,
										uint32 *colors) {
	int        i, i2, j, gray;
	uint16     *dst_ptr, *dst_ptr2;
	uchar      *src_ptr, *src2_ptr;

	src_ptr = src_base+src_row*src.top;
	src2_ptr = src2_base+src2_row*src2.top;
	dst_ptr2 = (uint16*)(dst_base+dst_row*dst.top)+dst.right;
	for (j=src.top; j<=src.bottom; j++) {
		i = src.left;
		i2 = src2.left;
		dst_ptr = dst_ptr2;
		if (i&1) {
			gray = src_ptr[i>>1]&7;
			gray += (src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7;
			if (gray > 0) *dst_ptr = colors[gray];
			dst_ptr = (uint16*)(((char*)dst_ptr)+dst_row);	// add one row
			i++; i2++;
		}
		while (i<=src.right-1) {
			gray = src_ptr[i>>1];
			gray += (((src2_ptr[i2>>1]>>(4-((i2<<2)&4)))&7)<<4) |
					 ((src2_ptr[(i2+1)>>1]>>(4-(((i2+1)<<2)&4)))&7);
			if (gray > 0) {
				*dst_ptr = colors[gray>>4];
				dst_ptr[dst_row/2] = colors[gray&7];
			}
			dst_ptr +kòdó¡òóÅòõó¸òfôìò¬ôEóùôôXõÂôëõ»õ¼ö…öy÷2÷øŞ÷~ø·ø	ùuù¡ùİù{úëùôúúNûIúmû³ú¬ûÙúÜûèú?ü³úOüÂúGüûüÃûüMüIüãü¦ühıÑüÎı0ı_şXıíşÃı•ÿşîÿş Ôş êş7 éş Ûş ¡ş" 'şU áı… wıŸ 2ıª ¶ü° \üİ íûêûÖ ,üº JüŸ 2ü½ üØ RüÇ oüS ¼üÂÿüaÿTü8ÿ)üÿPü¸şpüBşsüşüïıÏûşšûıı¡ûèıŠûÜı]ûGşCû‰şoûºş¥ûÇşÈûñş¬û,ÿßû0ÿöûDÿ-üxÿNü­ÿ–üßÿÑüºÿıâÿ	ı ıüQ òü3 Bı: `ıa fı¤ ”ış ÌıB%şIrşD¬ş%Îşÿº fÿø ”ÿêÿ,W 1¡ mù ”AÔj,—nÂ¥¼;ŒíÕ3u;°£í$cşıéÆÔÙ¢Şnğ÷9€!	¶—	‘	EÃ	ıÕ	Ö	
²8
 
ã	“J
ŞÄ
dÆcB	­¹	á

-Q
qu
yË
i3qß}—fJG‰
vú	¨m	ç	cCÙ|YÌ
äÏú˜ÜBâêÙÌã¹½ã8$¨
½Û	¡ÊpÃ£“°’eE«ëíQÔÓ©uj*G“ V·ÿÔş şüÿKı…ÿü%ÿÒûÉş ûYş{úÈıúı«ù˜üQù+üÅøàûPøNûû÷ıú†÷ùú÷ûö'ûüõ/ûõLûHõ”û(õèûÍô@ü|ôfüOôüôĞü#ô=ıJô ı^ô5ş’ôŸşªôÛşôôHÿ'õ•ÿIõ	 UõC 4õ™ (õ´ 2õç õõ"õ2Nõ.ŠõCŞõHDöD÷0™÷N?øašøpÒø'ùÚ gù ×ùQ óù. 
ú éù İùÓÿôù¹ÿ+úŸÿZúÿ¸ú‚ÿûšÿwûÎÿİûæÿIüöÿ¤üïÿ.ı zı4 Òı[ Üıg şO #ş	 €şÖÿÿÀÿUÿçÿÊÿåÿ? İÿÖÿ™ <P Í— òøEa)_-b)[·’}‡cbSV>DDåJ±f¬jÁkÅâzú“©!»áÃ¦]Boİ R¼   è _ È  Ÿ »ÿµ |ÿŞ $ÿœ ÿ3 ÀşÑÿAş‡ÿ¿ıIÿqıêşıPşÔüªıVü5ıüÄü®ûŸü‹û­üaûšü{ûiüjû¤üOûÜü˜ûııûBıü}ıCüÊıdüÕıüíı¡üåı›ü¾ı»ü ı‚ü´ı?üŒı6ü—ıLüqıwüı™üêı¸üSşşüØşCı‘ÿ“ı@ ñı± †şõ ·şTáşYÒş:ìş½şì ®şØ ‹şÏ 4şû Ïı>}ıy†ı–»ıÛÈıG›ı¥Zı(#ıı÷ıı;×üFZüEòû'ûÚ€û’[û/ûİÀúoGúBÓùùòAùĞùáãøœ’øˆ9ø€ä÷™ï÷@ø(øî  ø î÷oì÷Å(ø øûÊø¢ø'yø?løR…øP®øXÃøsÄø†áø¥ÿø¦7ù“˜ù–ú—¨úÑ1ûäÍûÔ_ü˜İü”dı„õıƒrş,²şåÔş«ÿ¤Hÿ »ÿ¡+ ¢Ÿ ºÛ G¦×D‡ë4e£Ö	XCäS×n%©Z¨nÏ~‚J©c{{z÷Scg§æÚ;ß}ª“|XHò:®&‘ŒÉs¬Gˆ¸Ä"íR	‹ ½@á…¯Õ—Zz3ğh“´1ê²		 	í	Ò
	éUÍ˜‚Üó_¤Ü§†qùKEz<¥ı‡öğ !W eÙÿUÿÎÚşõUşåı!ıüÖüO¸ûêZû¸èúhwú .ú«õùS¼ù¬ùùjùíMù¿)ùùWĞø7³øö ø¼ aø )øh ø] á÷_ ¹÷a œ÷– ª÷Ò ˜÷,÷ª¤÷øÃ÷_øº`ø;løœCø¼gø˜¿øSùZ5ùb2ùQ(ùiù¿±ù’Üù¬üùÁ.úÎYúášú¨û]Kû—ûíÕû1üZüÓ ü~ {ü, Yüÿÿ5ü®ÿøûZÿÅûæşhûşû8ş·úşúúı“úøı|úÓı‹ú®ı†úcı¥úCıîúıOûşügûÓü¶û±ü	ü]ü‹ü9üªüüÇüøûàü­ûı…ûıAûıBû×üû—üôúoü«úWüUú4ü+úüúüúõû9úüeúü¶ú+üÎúü÷ú$üûúùûûèûû±ûûwûLûFû~û$û‰ûŞúû™úû\ú~û\úeû,ú–ûúÃûúÔûûùüú>üîùhüäùcüæùLüåù!üïùüûÙù üÑù%ü•ùMü€ùHü\ù:üJù1ü?ùLü_ùuü†ùªüÏùÒüöùı:ú6ıjúAı·úlıŞúŠıKû¹ı£ûïıüGşLü”ş°üÍşâüñşı:ÿ_ıEÿÂıƒÿ0ş´ÿ†ş×ÿæşêÿIÿèÿ²ÿ   c ( Ñ  Xôÿ¦ğÿÌğÿø÷ÿ1ÿÿH l  ´öÿí $ a ‰íÿ•Ãÿ¢‹ÿa_ÿ…6ÿŠ8ÿ±&ÿ˜ÿ Øş—˜ş”nşiQşIşÙığıMıEıÑêüküÜ 3ü_ ÜûÆÿ›û2ÿû}şû÷ıµûbı˜ûı‡û•ü\ûgüZûSüpû-üû0ü‘ûü‰ûüyû üGû/ü5ûPüû7üûüûüûü7ûSü[ûü‚ûÜüÆû>ışû”ıKüÕı”üKşåüäşDırÿ¬ı·ÿ	ş +şk bşœ ³ş­ ÿ¯ ]ÿ §ÿE ×ÿK  S g b ® y · @ø ”F½§+4³pa“áêŠõ0ÂUV§ûü‰Pş‘“åõ4N	¥	õ	Y=
c‘
‘Ş
œÔ!Ö9İRÒiç‡ÿz[–c­Šš™”¾Ììığ™?}_h~3§Ş©,g†*ºÅ}M;uİj¯ş±¡°Fœö8ªœe%=_¦
ı õ	&M	Rjî‹_¾·%ÌtÔÍá7Õ}ÛÇäùß3îB ¸Xÿ¼{ş‘×ı®2ı¥ü¬ìû‡xûU4ûû çúº ½ú| ‹ú  >úâÿÈù‚ÿjùRÿùÿøÿ-øÿÈ÷"ÿÄ÷+ÿ­÷Fÿ¶÷aÿ÷hÿÊ÷wÿó÷¢ÿø Gø0 ^øn ƒøœ †øò —ø.‰øƒwøì•øb¯øŞÓøbÒø§+ù÷HùÀùHú1úûâ‹ûÑãû¤lüoÎü;ı¦„ıèı®4ş2¢şö àş® DÿŸ „ÿ[ Ğÿ- àÿüÿéÿàÿÀÿõÿÄÿÍÿ×ÿ¿ÿ»ÿ·ÿ~ÿ¿ÿwÿ­ÿ9ÿÆÿçşíÿ¼ş®ÿÔşkÿ¦ş\ÿnşFÿ]şÿZş×ş´ş şâşmşõş>ş/ÿçı…ÿÚı…ÿşŒÿşšÿ şPÿ!şÿ6şÅş=ş\şvşş”ş¥ı‰şpı™ş6ı~şıwşÎüQş¹üşƒüíıü²ı+ü’ıühıŞû8ı’û_ıxûNı-û|ıáú¨ıœúêı?ú%ş	ú8şİùQşÖùDşÑùEşØùAşçù:şú/ş(úş-úşúşÒùñıšùåı>ùßıØø·ı¯ønı\ømıø?ıİ÷ı½÷¹ü‰÷jü÷ü³÷Æûá÷ªû+ø‹ûHø…ûµøaûùpûvù£û¤ùÚûåùüúOü[ú˜ülú¯üÀúÚüêúíüQûı†ûëüùûêüIüÅü©üŸüıaü~ıRüµıIüèıIüJşLü“şuüÚş¸üøşÔüÿı8ÿ$ıfÿLı˜ÿ]ı«ÿnıÇÿhıÂÿCıÕÿığÿªü4 sü üÀ ±ûà ]û û(êúb¶ú‚¥ú—ú,úáùx‘ùKMùíøØ ~ø„ @ø Ö÷Îÿ‹÷}ÿG÷ÿ÷•şüö1şëöŞıÊö›ıÄögıÏö6ı&÷ıO÷·üU÷}ü(÷8üõöüßöôû½öÊû¬ö‘û¬ö ûˆöœû”öŒû¹önû	÷eû<÷‚û˜÷‹ûö÷¶û:øáû´ø6üù‚ü¦ùı÷ù|ı?úËı{úş¤úLşû}şû½şüÿlü<ÿÚüRÿHıkÿ²ıˆÿLş¼ÿéş ‰ÿd  Â  y`èÌzB-ÇşA¢×F]·áMN·³*õš$Ale·	³q	Á¶	Çá	Ø
òH
ék
 ¡
>Ì
oÛ
}ò
­ö
åJ“+à7	f	â
£	µ
Ş	Ÿ
Ş	

ª
&
•
V
`
a
2
Y
*
6
7

t
É	™
	¬
a	Ç
	Û+Çw°‘¤˜{Ir#IÇ
@R
V
P„	`éS†45%Ä[dkÈZQ3¤!£ú';£fŒ–˜ŸU)¸ İ2 «åÿbsÿ<ÿ ´şÑIş}Òı%{ıãıŠ³ü}üÎ9üˆôûûœ]û;û*ûB2ûã	ûûQûûò +ûÚ Pûâ fû‡û9©ûŒ³ûå«û2°ûj û}­ûˆ½û¦ÆûÇ»û²ûîûLü÷tüê¤ü±Òü…ÿüOı94ıópı·Âıqş#^şÄ £ş„ ÿ2 jÿÓÿ«ÿ’ÿ£ÿ_ÿ¤ÿ)ÿ¡ÿìşµÿ¯şÂÿişÄÿ3şŠÿ
ş:ÿØıâşı¦şGışıUşÃüIşsü!ş/ü!şóû$ş¼ûVş¦û[ş·û¢ş×ûÇşæûÿøû7ÿü{ÿ%ü§ÿJü¯ÿ\ü®ÿ|ü®ÿxü…ÿ‹ülÿ_üEÿ-üÿçûÅşÏûqş¤û<ş§ûşıû¸ı²ûwıÅû%ıîûôüüËü2üÚüUü³ü~üü½üoüùüXü1ıVücıWü ı3üÚıüşüş÷ûDşıû[şøûşü‡ş
ü{şüûPşü9şşû)ş×ûşÆûöı¨ûÛı“û§ıXûWı"ûı
ûØüòú—üìú‚üñúgüàúˆüìú¥üìú¸üôúøüûı$û!ı.ûıFûìüeûÚü ûÂü³ûÍüĞû¥üÍû“ü
ümüiü4üÕüüıüUıáûtıÑû«ı½ûõıûMş‰û¸ş‡û
ÿ–û:ÿ¬ûTÿ¿ûbÿÓû…ÿâû·ÿüæÿ2ü gü*  ü8 Îü< òüB ı] ım İü_ Åüg ›üX güX üE Çû3 §ûR TûD Oû! >û& 7ûB 0û€ ,ûO <ûùÿQû„ÿ[û8ÿAûÚş3ûyşFûş-ûŒı,ûëüûEüû©ûÿúûğú¹úÄú6úÍú
úÃúÉùÔú®ù
ûˆù6ûù\û®ùNûİùûúıú&úûRúCû€úUû˜úeûšú{ûÖúšû5ûêûwûrüÜûßüfü$ıöüaı‚ıÈıÛı]şşìşpş+ÿ»şpÿDÿÿ¸ÿĞÿ2  q D o „ l ° s ç ¦ (î tDá¿|/Ç#øW vºğÔM-Ä ïÖ¼«I”|„ç3ˆˆ‘­¶ÇÅşÇ=	²	ÍÙ	êğ	ü	
å	ş	C	é)AßDqO6Ë"kãºÅjŠ2M÷İ³Üy×5ÒàßÅ=°p¢–Û™üvkóã ÔÄ sµ ù ‹G ( ´áÿĞÿ‘äÿıõÿ³& O@ Y ö ‡ ¨ ÷ € fJ ®, âÿEÿ¶5ÿà ş:VşVèıw ıqcıbıSı&êüÿ¹ü×¦üü`šüÀüüÿüĞRı¾|ıËŠı´¼ı…¯ı{¶ı€¯ıéı§Úıëõı4ş_şy'ş1ş·PşÌ[şœş-Åş[Ùş‹·ş¡·ş¿€şÀ:şİìı·Âı¦¥ırÌıJïı2ş%/şñDşäişÂµş¦Ùş«ÿˆÿtíşKµş!sşÚñı±’ı^ı
rıÒ jı³ ?ı )ı= ìüîÿ}ü²ÿ8üÿŞû_ÿŸûsÿ]ûwÿûrÿÕúNÿoúbÿúpÿÇùÿºù™ÿ½ù†ÿİù\ÿñùUÿú\ÿ1ú[ÿdú_ÿ¥ú‹ÿÇúªÿİúÇÿâúæÿîúşÿèú7 ËúF îú$ 
ûıÿCûòÿLûñÿ}û ¾û üüÿ?ü Kü Bü@ aü© qüü Àüaı«SıÿlıfısMı¢LıÉJıÖiıàıåÔıï%şë~şŞĞş©0ÿÿ`ÆÿpùÿR E: ,- . ®íÿPÿà yÿœ dÿU hÿúÿcÿÿnÿ[ÿ®ÿ1ÿøÿÿ úşZ İşŸ ©şë ş/Eşy2ş¦ş×ş²½ı­Xıjımºü‚üğküT=ü«üôüûü	öûÿøûüüÁ!üwü½û˜£û,Yûå 	û¶ Íúb múl úM ÑùP ‰ù7 EùD ùT Õø ¨øŸ ¶ø ¶ø‚ ¹øS ™ø3 EøöÿøŸÿå÷9ÿ÷·şY÷(ş÷œı	÷0ıÌöÀü¯öMüöäû`ö¦ûöûö«úövúéõ<úòõõùâõ«ùÍõ„ùÅõNùÊõùîõĞøö¤øEöiø‚ö;ø«öUøìö©ø÷Õø‰÷+ùÑ÷ƒùøújøÊúªø¥ûÒøgüù
ı4ùÄıSù_ş„ù»ş÷ùÿkú—ÿÛú nû üH–üıµıâMşqôş~ÿhüÿéc PÏ Ã[-ÊwW‘Ï¿(|8Õ§#d	S	
°¨
A2Ñ>j#¹P25Kg;^Ukw]ÃŠ‹‘µÑøL+BFHqC‹òÆ»Ä¿é½\Ù›
ù
ı¼	ìH	ÔJƒkV_$]
YÕ\˜M-#î¯…íMí·ñ{úçÔ½¤¦bŒò]ƒ5ÿ¨Ö…®d\{OmOf$Oîxt‚×Ø)öl 6´ÿhÿÍş~¦ş”Yş¨üı‹šıY\ı8aı"dıßIıøüa•üô,ü£¨ûU}û;ûÆPûx˜ûüêKü¨Aüb1ü-ü¾7üåûp£û(\û¢ 7ûX ìúd kú˜ !ú ú‘ zú© ôú „û¡ ü~ vü— ¸ü€ æüs ìüx ı£ /ıÈ ıÿ Êüø µüÚ wü¿ ü¾ ×û¨ Šûw —ûI ÀûéÿRü­ÿ·üiÿÔüGÿ»üòş×ü¥şÇüeşµü8ş¬üùıœü“ı„ü4ıEü
ıßûıüûğü\úºüÛùpüÁù]ü©ùVüœùDü^ùü)ùãûù¾ûù³ûşøüùVüDù„ü~ùƒü<ù‘üòøzü«økücø_üü÷Aüé÷üøÇû§ø ûIùˆû×ù{ûú’ûFú‚û„ú€ûœú†û¤ú©ûÉú×ûÖúü©úTücú‚üÔù ümùÂü4ùğüNùı³ù'ıEúAı¤úwıû‚ırû”ı³ûÅıüû;ş;ü‹ş™üäşÕüÿÕüBÿ×üIÿ‘üfÿCüeÿıûIÿ1üÿ‰üñş ışşwıÿ½ıÿÌıÿçıÎşîı³şïı­şåı™şÍı™ş”ı·şhıªş.ıŠş(ıCşâüşÂüÑıÑüıìücıı=ıQıíü¤ıŸü«ı~ü¿ıjü şdüQş‡ü¬ş¦üÿ£üZÿ¤üEÿÆüûşÁüÇş°ü§ş£ü şü}ş•ü¡ş“ü¾ş»üÿáü^ÿüü¬ÿıÖÿ'ı( TıG wıu §ıÁ Èı° Òıe «ıÎÿ“ı]ÿ|ıïş€ı”şhı=şkı?şSı4ş"ıQşı{şèüòşı+ÿ:ıiÿımÿ~ıbÿjıhÿMı8ÿGıäş)ı~şıõıêü:ıçü“üâüüÉüü¢ü,ü¬üüàüüÓüüı$ü!ı)ü]ı?üiı@ü£ı(ü´ı­û¼ıTûÔı'ûæı0û,ş2û@şRû\şpûhşµûş-üşËüºşuı×şQşÿÿ3ÿˆÿ•ÿşÿÊÿ“ áÿİÿ_ ¤ÿÿ¨% —; uP ‚G ‰] ‡V ÀU U ƒˆ ¦ ÏÌ hÎ Áç ZÔ €› µq ÑK *= u ¶ æÿûÿ n/ ®H "	g `	„ ¬	 È	´ ä	Å Ş	Ù ±	ï \	ö ×pÿ üğ sõ ã ¿á ’÷ fI ,I‰¯õà·&a/õX£WĞH2:nnÃ€ŞT¤G}úáËe¾¢ÄÑ¶Ñ¸Ä‚“` pØšöX?ŠšŞ¬BâŒáÑ		N	|Õ­nù×.]g
…ä±Ìüæ0Y îšgŒ ¡iR++Şõ•…>?®ş<ÄÀ“TwöU¨}®&_3ëæ–h7­èoN`Ì GH HÈÿ0	ÿ)„ş1
şLıw=ı_ìüYƒü`üG¸û=ûç áúÑ …ú¯ Fú¡ .úr  úh äù °ùŞÿtù«ÿxùOÿ›ù.ÿ¥ù÷ş¢ù§ş‰ùRşZùş0ù»ı3ù]ıù$ıÇø÷üøÚüø¶üµ÷²üM÷ÉüãöÿüZö'ıÊõ_ı‹õkıõsıÀõ‡ıZöšıçö†ıŠ÷ı$øıËø£ı„ù‹ıúÉı·úİı5ûğıdû şƒû1ş‘û_ş¯û•şÓûÅş¾û%ÿÈûKÿàûlÿ*ü|ÿküÌÿğüÿÿrı% îıK +ş… mşÎ ¥şÿm?ÿÂŒÿ£ÿfäÿÆ ş7 &@ f 	“ ¹ %Ÿ)#Pñ(ôÕğ¥İ”C1&ÂRí Î ã / è yÿ¤ ïş´ oş­ ş… ©ıi Tı âüËÿ ü9ÿ2ü¿şõû=ş¨û¶ı~ûUı9ûıû½üŸúKüNú÷ûãù°û•ù`û?ù+ûóøû¤øíúHøàúøúê÷Dú¿÷Íù÷ZùC÷æø÷…øÑö&ø²öø—öï÷yöí÷=öÛ÷!öÆ÷ö‹÷<ö{÷GöD÷vö4÷ ö
÷ öÍö°öÇöËö}ö÷eöI÷IöQ÷}ö\÷Íö	÷÷ıöT÷Õö•÷×öÀ÷§öó÷ˆöï÷]ö=ø@öø`öø|öøöøàöø÷ó÷÷Ö÷[÷İ÷n÷ø—÷Šø´÷êøÿ÷yùøçùPø\ú|ø¤ú¢øâúÏøHû"ù»ûlùüÎùüúıJú°ı’úNşÈú@ÿèú&  ûXiû/™ûâûäüwœüóıüL£ıˆşÑ»şú5ÿpºÿM 0£ x	Ùj!¦™(	dŠ	Âä	.
+W
T3
wñ	¹š	õn	{	:©	oØ	¼ı	ï
6<
4_
R»
OÊ
]Á
ª¾
Ë®
üu

'Ò	X	ïíÕŒª$£»reTBÈoT¹<Úì(KTt©¦½¸Éíˆ E  ô+ û| ç÷ »`¥ñ›|=k«`9`aárü„ñĞƒ°q¸uÂ©Íßş(4b™›óğ÷MŞu›ˆdÂ,î÷)	ÅF	/	N(	*	É?	Ç5	ÈI	Y	—d	œU	˜Z	Z	‡*	P	â¹•m,õÑœˆN ×ñ$¤G"¦«à3êÑïM¢[·Èˆ+D” 	 Ü ¦ÿÄ 7ÿÍ ÅşÏ bşæ şû ¿ıšı ıı«ıì zı³ BıR õüÎÿfübÿ×ûÿKû–ş¾úHş5úÙı½ùLı>ùÎüçøyüºø9ü°øüøîû“øÚûtø½ûZø û/øzûî÷wû}÷Sû÷û»öûiöëú3ö¸úõõ¦úâõ†úÜõ‘úëõ¤úö½úxöêúòöôúo÷ûõ÷)ûøaûüø’ûlùÄûùáûúùü&ú+üRú=ü{úuüòúüZûÓüâûıüqı•ü¿ı?ış×ı{ş‘şÁş!ÿ*ÿÈÿsÿE ¶ÿ¹ ïÿ& =| C¨ K© HÉ iø Š9°Ş+Pª¬	isÑ®/ôV€Qş»ºg~$Îî|·C{)>ş©Émo>8åzÏ ş – K s ®ÿ0 ÿîÿqşÀÿÅı‚ÿFı4ÿÕüæş¨ü”ş˜üGş¦ü×ı³ü‰ı·üBı®üèü¤ü