#include <string.h>
#include <Debug.h>

#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef	WINDOW_H
#include "window.h"
#endif
#ifndef	SERVER_H
#include "server.h"
#endif
#ifndef	_MESSAGES_H
#include "messages.h"
#endif
#ifndef	OFF_H
#include "off.h"
#endif
#ifndef	_STDLIB_H
#include  <stdlib.h>
#endif
#ifndef	_OS_H
#include <OS.h>
#endif
#ifndef	_SAM_H
#include "sam.h"
#endif

#include "app.h"
#include "bitmap.h"
#include "heap.h"
#include "session.h"
#include "as_debug.h"

/*------------------------------------------------------------*/
extern	"C" void	send(long, long, char *);
/*------------------------------------------------------------*/

bool IsValidGraphicsType(uint32 type);
void GraphicsTypeToInternal(uint32 type, int32 *pixelFormat, uint8 *endianess);

TOff::TOff(
	SApp *application,
	char *name,
	int32 x, int32 y,
	uint32 pixelFormat,
	ulong wflags,
	ulong bitmapFlags,
	int32 rowBytes,
	Heap *serverHeap,
	TSession *session)
	: MWindow(application)
{
	rect	bound;
	rect	view_bound;
	rect	tmp_rect;

	m_application = application;
	valid_signature = WINDOW_SIGNATURE;

	bitmap = new SBitmap(x,y,pixelFormat,true,bitmapFlags,rowBytes,serverHeap,application);
	bitmap->ServerLock();
	bound.left = bound.top = 0;
	bound.right = x-1;
	bound.bottom = y-1;
	renderCanvas = bitmap->Canvas();

	window_name = (char *)grMalloc(strlen(name)+1,"off name",MALLOC_CANNOT_FAIL);
	strcpy(window_name, name);

	m_preViewPortClip = newregion();
	m_fullRegion = newregion();
	m_clientRegion = newregion();
	m_visibleRegion = newregion();
	m_clearRegion = newregion();
	m_invalidRegion = NULL;
	m_bad_region = newregion();

	set_region(m_bad_region,&bound);
	SetFullRegion(m_bad_region);
	SetClientRegion(m_bad_region);
	SetVisibleRegion(m_bad_region);
	clear_region(m_bad_region);

	m_updateCauses[0] = 
	m_updateCauses[1] = 
	m_updateCauses[2] = 0;
	proc_id = -1;
	flags = wflags;
	window_bound = bound;
	last_view_token = -1;
	last_view = NULL;
	m_setView = NULL;
	m_viewRFlags = 0xFFFF;
	a_session    = 0;
	subwindow_list = NULL;
	ws_ext_bounds = NULL;
	dw = NULL;

	view_bound.top = 0;
	view_bound.left = 0;
	view_bound.right = bound.right - bound.left;
	view_bound.bottom = bound.bottom - bound.top;

	top_view = new TView(&view_bound, FOLLOW_BORDERS, 0);
	top_view->SetOwner(this);

	InitRenderStuff();
	bitmap->SetPort(renderPort);
#if ROTATE_DISPLAY
	if (bitmapFlags & B_BITMAP_IS_ROTATED) {
		renderPort->canvasIsRotated = true;
		renderPort->rotater = &rotater;
		rotater.mirrorFValue = (float)(y-1);
		rotater.mirrorIValue = y-1;	
	}
	else {
		renderPort->canvasIsRotated = false;
	}
#endif


	m_eventPort = NULL;
	if (session) {
		fport_id = session->receive_port;
		a_session = session;
		a_session->SetAtom(this);
		a_session->SetClient(this);
	} else {
		fport_id = create_port(BUF_MSG_SIZE,"p:(offscreen)");
	};
	client_task_id = 0;

	char name_buffer[128];
	sprintf(name_buffer, "o:%.24s:%ld", window_name, application->TeamID());
	task_id = xspawn_thread(start_task, name_buffer, B_DISPLAY_PRIORITY, this, false);
}

/*------------------------------------------------------------*/

void TOff::get_pointer()
{
	bool	is_area;
	uint32	data;

	if ((is_area = (bitmap->AreaID() != 0))==true)
		data = (uint32)bitmap->AreaID();
	else
		data = bitmap->ServerHeapOffset();
	a_session->swrite(sizeof(bool), &is_area);
	a_session->swrite_l(data);
	a_session->swrite_l(bitmap->Canvas()->pixels.bytesPerRow);
	a_session->sync();
}

/*------------------------------------------------------------*/

void TOff::moveto_w(long h, long v)
{
}

void TOff::move(long delta_h, long delta_v, char forced)
{
}

void TOff::size(long dh, long dv)
{
}

void TOff::size_to(long new_h, long new_v, char forced)
{
}

char TOff::is_front()
{
	return(1);
}

/*-----------------------------------------------------------*/

void TOff::do_close()
{
	port_id	pid = fport_id;
	tokens->remove_token(server_token);
	ServerUnlock();
	delete_port(pid);
}

TOff::~TOff()
{
	bitmap->ServerUnlock();
};

/*-----------------------------------------------------------*/

void TOff::send_to_back(bool)
{
}

/*-----------------------------------------------------------*/

void TOff::bring_to_front(bool, bool)
{
}

/*------------------------------------------------------------*/

#if AS_DEBUG
void TOff::DumpDebugInfo(DebugInfo *d)
{
	DebugPrintf(("TOff(0x%08x)++++++++++++++++++++\n",this));
	DebugPushIndent(2);
	MWindow::DumpDebugInfo(d);
	DebugPopIndent(2);
	DebugPrintf(("  bitmap -->\n"));
	DebugPushIndent(4);
	bitmap->DumpDebugInfo(d);
	DebugPopIndent(4);
	DebugPrintf(("TOff(0x%08x)--------------------\n",this));
};
#endif

