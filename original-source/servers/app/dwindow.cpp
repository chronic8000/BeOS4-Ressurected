
#include <string.h>
#include <stdlib.h>
#include <PCI.h>
#include <debugger.h>
extern "C" {
#include <signal.h>
}

#include "window.h"
#include "dwindow.h"
#include "priv_syscalls.h"
#include "cursor.h"
#include "render.h"
#include "heap.h"
#include "renderdefs.h"
#include "graphicDevice.h"
#include "token.h"
#include "eventport.h"

extern	TCursor			*the_cursor; 
extern	TWindow			*windowlist;
extern	"C" short		screen_flags;
extern	TokenSpace		*tokens;

DWindow::DWindow(dw_init_info *info, TWindow *window)
{
	int				i;
	area_info		ainfo;

	// save a back pointer to the owner window.
	owner = window;
	
	// save the pointer to the resources used to communicate with the direct deamon
	clipping_area = info->clipping_area;
	disable_sem = info->disable_sem;
	disable_sem_ack = info->disable_sem_ack;
	
	// initialise the buffer description pointer (shared area).
	get_area_info(clipping_area, &ainfo);
	buf_info = (direct_buffer_info*)ainfo.address;
	
	// init internal states
	bits0 = (char*)0;
	pci0 = bits0;
	change_state = 0;
	buffer_state = 0;
	was_screen = false;
	NewDriver();
	NewMode();
	
	// initialise the buffer descriptor
	buf_info->buffer_state = (buf_state)(B_DIRECT_STOP | fullscreen_mask());
	buf_info->driver_state = (driv_state)0;
	buf_info->bits = (void*)bits0;
	buf_info->pci_bits = (void*)pci0;
	buf_info->layout = B_BUFFER_NONINTERLEAVED;
	buf_info->orientation = B_BUFFER_TOP_TO_BOTTOM;
	buf_info->_dd_type_ = 1;
	buf_info->_dd_token_ = 1;
	buf_info->clip_list_count = 1;
	buf_info->window_bounds.left = owner->window_bound.left;
	buf_info->window_bounds.right = owner->window_bound.right;
	buf_info->window_bounds.top = owner->window_bound.top;
	buf_info->window_bounds.bottom = owner->window_bound.bottom;
	buf_info->clip_bounds.left = 0;
	buf_info->clip_bounds.right = -1;
	buf_info->clip_bounds.top = 0;
	buf_info->clip_bounds.bottom = -1;
	buf_info->clip_list[0].left = 0;
	buf_info->clip_list[0].right = -1;
	buf_info->clip_list[0].top = 0;
	buf_info->clip_list[0].bottom = -1;
	
	// allocate the regions used for internal processing
	bounds = newregion();
	visible = newregion();
	old_visible = newregion();
	tempregion = newregion();
}

DWindow::~DWindow()
{
	// free the region used for internal processing
	kill_region(bounds);
	kill_region(visible);
	kill_region(old_visible);
	kill_region(tempregion);
	
	// free the resources used for communication with the client
	delete_sem(disable_sem);
	delete_sem(disable_sem_ack);
	Area::DeleteArea(clipping_area);
}

void DWindow::Disconnect()
{
	if (owner->HasClosed())
		return;

	// don't do anything if direct access is already stopped.
	if ((buf_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP) {
		need_acknowledge = false;
		return;
	}
	
	// send a stop request and require an acknowledge
	buf_info->buffer_state = (buf_state)(B_DIRECT_STOP | fullscreen_mask());
	need_acknowledge = true;
	release_sem(disable_sem);
}

void DWindow::NewMode()
{
	int32		pixf, end;

	if (owner->HasClosed())
		return;

	buf_info->bytes_per_row = owner->renderPort->canvas->pixels.bytesPerRow;
	pixf = (color_space)owner->renderPort->canvas->pixels.pixelFormat;
	end = (color_space)owner->renderPort->canvas->pixels.endianess;
	switch (pixf) {
	case PIX_8BIT :
		buf_info->pixel_format = (color_space)(B_CMAP8 | ((end==LENDIAN)?0:0x1000));
		buf_info->bits_per_pixel = 8;
		break;
	case PIX_15BIT :
		buf_info->pixel_format = (color_space)(B_RGB15 | ((end==LENDIAN)?0:0x1000));
		buf_info->bits_per_pixel = 16;
		break;
	case PIX_16BIT :
		buf_info->pixel_format = (color_space)(B_RGB16 | ((end==LENDIAN)?0:0x1000));
		buf_info->bits_per_pixel = 16;
		break;
	case PIX_32BIT :
		buf_info->pixel_format = (color_space)(B_RGB32 | ((end==LENDIAN)?0:0x1000));
		buf_info->bits_per_pixel = 32;
		break;
	}
	buf_info->bits = owner->renderPort->canvas->pixels.pixelData;
	if (pci0 == 0)
		buf_info->pci_bits = (void*)0;
	else
		buf_info->pci_bits = (void*)(pci0 + ((char*)buf_info->bits - bits0));

	// REVISIT: mathias 12/10/2000: don't reset the screen,
	// because this function is called at each region change
	#if !defined(MINI_SERVER)
	change_state |= B_MODE_CHANGED;
	buffer_state |= B_BUFFER_RESETED;
	#endif

}

void DWindow::NewDriver()
{
	int			i, j;
	uint32		hbase, hsize;
	pci_info	h;
	status_t	err;

	if (owner->HasClosed())
		return;

	// initialise the differential mapping between virtual and physical for that device
	bits0 = (char*)owner->renderPort->canvas->pixels.pixelData;
	pci0 = (char*)owner->renderPort->canvas->pixels.pixelDataDMA;
	
	// check for software cursor and parallel buffer access support.
	software_cursor = !BGraphicDevice::Device(0)->HasHardwareCursor();

	display_mode mode;
	BGraphicDevice::Device(0)->GetMode(&mode);
	if (mode.flags & B_PARALLEL_ACCESS)
		multiple_access = true;
	else
		multiple_access = false;
	
	// set the flag for client information
	change_state |= B_DRIVER_CHANGED;
}

void DWindow::Before()
{

	rect		new_bound, old_bound;
	region		*clip;

	if (owner->HasClosed())
		return;

	// REVISIT: mathias 12/10/2000: reinitialize the resolution/screen size
	#if defined(MINI_SERVER)
	NewMode();
	#endif

	// first, check if the visible region changed. If not, nothing to do...
	clip = owner->m_newVisibleRegion;
	if (clip == NULL) {
		// if no clipping change, then check if the buffer is stop
		if ((buf_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP) {
			clip = owner->VisibleRegion();
			if (!empty_region(clip))
				goto force_continue;
		}
		need_after = false;
		need_acknowledge = false;
		return;
	}
force_continue:

	// set the buffer change flags.
	GetContentBound(&new_bound, &old_bound);
	buffer_state |= B_CLIPPING_MODIFIED;
	if ((new_bound.left != old_bound.left) || (new_bound.top != old_bound.top))
		buffer_state |= B_BUFFER_MOVED;
	if ((new_bound.right-new_bound.left != old_bound.right-old_bound.left) ||
		(new_bound.bottom-new_bound.top != old_bound.bottom-old_bound.top))
		buffer_state |= B_BUFFER_RESIZED;

	// if the buffer is already stopped, nothing more to be done in before
	if ((buf_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP) {
		if (!empty_region(clip))
			need_after = true;
		else
			need_after = false;
		need_acknowledge = false;
		return;
	}
	
	// If the new visible region is empty, it's time to send a stop.
	if (empty_region(clip)) {
		need_after = false;
		need_acknowledge = true;
		buf_info->buffer_state = (buf_state)(B_DIRECT_STOP | fullscreen_mask());
		buffer_state |= B_BUFFER_RESETED;
		release_sem(disable_sem);
		return;
	}
	
	// If the window move, then no choice, we have to stop it.
	if ((new_bound.left != old_bound.left) || (new_bound.top != old_bound.top)) {
		need_after = true;
		need_acknowledge = true;
		buf_info->buffer_state = (buf_state)(B_DIRECT_STOP | fullscreen_mask());
		release_sem(disable_sem);
		return;
	}
	
	// set the region
	// mathias - 12/10/2000: The VisibleRegion() is too much. It must be and'ed by m_clientRegion
	// so that the window's Decor is excluded from it. See LockPort() (My opinion is that we should
	// use the same clipping region than LockPort() does).
	// This fixes a bug in Chart / GLTeaPot (all BDirectWindow applications) where the decor will be erased
	// if it has some parts inside the Window's bounds
	set_region(bounds, &new_bound);
	and_region(clip, bounds, tempregion);
	and_region(owner->m_clientRegion, tempregion, visible);	
	set_region(bounds, &old_bound);
	and_region(owner->VisibleRegion(), bounds, tempregion);
	and_region(owner->m_clientRegion, tempregion, old_visible);

	// check if there is a real need to do a modify during the before transaction
	sub_region(old_visible, visible, bounds);
	if (empty_region(bounds)) {
		// the new visible region include the old one. No need for modify
		need_after = true;
		need_acknowledge = false;
		return;
	}
	
	// settings the change flag
	buf_info->driver_state = (driv_state)change_state;
	change_state = 0;

	// set the delta-region
	and_region(visible, old_visible, bounds);
	SetRegion(bounds, &new_bound);
	
	// check if the after modify is necessary
	sub_region(visible, old_visible, bounds);
	if (empty_region(bounds))
		need_after = false;
	else
		need_after = true;
	need_acknowledge = true;
	buf_info->buffer_state = (buf_state)(B_DIRECT_MODIFY | buffer_state | fullscreen_mask());
	buffer_state = 0;
	release_sem(disable_sem);
	return;
}

void DWindow::After()
{
	rect		new_bound;
	long		dt, dl, db, dr;

	if (owner->HasClosed())
		return;

	// REVISIT: mathias 12/10/2000: reinitialize the resolution/screen size
	#if defined(MINI_SERVER)
	NewMode();
	#endif

	// nothing more if we don't need an after transaction
	if (!need_after) {
		need_acknowledge = false;
		return;
	}
	buffer_state |= B_CLIPPING_MODIFIED;
	
	// set the region
	new_bound.left = owner->window_bound.left;
	new_bound.top = owner->window_bound.top;
	new_bound.right = owner->window_bound.right;
	new_bound.bottom = owner->window_bound.bottom;

	// set the region
	// mathias - 12/10/2000: The VisibleRegion() is too much. It must be and'ed by m_clientRegion
	// so that the window's Decor is excluded from it. See LockPort() (My opinion is that we should
	// use the same clipping region than LockPort() does).
	// This fixes a bug in Chart / GLTeaPot (all BDirectWindow applications) where the decor will be erased
	// if it has some parts inside the Window's bounds
	set_region(bounds, &new_bound);
	and_region(owner->VisibleRegion(), bounds, tempregion);
	and_region(owner->m_clientRegion, tempregion, visible);
	SetRegion(visible, &new_bound);
	
	// settings the change flag
	buf_info->driver_state = (driv_state)change_state;
	change_state = 0;
	
	// do the start transaction
	need_acknowledge = true;
	if ((buf_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP) {
		was_screen = owner->is_screen();
		// in full-screen mode, need to check if it needs to hide the cursor
		if (was_screen) {
			if (software_cursor)
				the_cursor->Hide();
		}
		// in window mode, need to check if direct connection is allowed
		else if (!multiple_access || software_cursor) {
			need_acknowledge = false;
			return;
		}			
		buf_info->buffer_state = (buf_state)(B_DIRECT_START | buffer_state | fullscreen_mask());
	}
	else
		buf_info->buffer_state = (buf_state)(B_DIRECT_MODIFY | buffer_state | fullscreen_mask());
	buffer_state = 0;
	release_sem(disable_sem);
	return;
}

void DWindow::GetContentBound(rect *new_bound, rect *old_bound)
{
	rect		*new_b;
	long		dt, dl, db, dr;

	if (owner->m_newClientBound.right >= owner->m_newClientBound.left)
		new_b = &owner->m_newClientBound;
	else
		new_b = &owner->window_bound;
	new_bound->left = new_b->left;
	new_bound->top = new_b->top;
	new_bound->right = new_b->right;
	new_bound->bottom = new_b->bottom;
	old_bound->left = owner->window_bound.left;
	old_bound->top = owner->window_bound.top;
	old_bound->right = owner->window_bound.right;
	old_bound->bottom = owner->window_bound.bottom;
}

#define MAX_CLIP_LIST_COUNT ((B_PAGE_SIZE-sizeof(direct_buffer_info))/sizeof(rect))

void DWindow::SetRegion(region *clip, rect *bound)
{
	if (clip->CountRects() > MAX_CLIP_LIST_COUNT)
		buf_info->clip_list_count = MAX_CLIP_LIST_COUNT;
	else
		buf_info->clip_list_count = clip->CountRects();
	memcpy(&buf_info->window_bounds, bound, sizeof(rect));
	memcpy(&buf_info->clip_bounds, &clip->Bounds(), sizeof(rect));
	memcpy(buf_info->clip_list, clip->Rects(), sizeof(rect)*buf_info->clip_list_count);
/*	
	// print the clipping region for debug
	xprintf("window bounds: [%d,%d,%d,%d]\n",
			bound->left, bound->top, bound->right, bound->bottom);
	xprintf("clip bounds: [%d,%d,%d,%d]\n",
			clip->bound.left, clip->bound.top, clip->bound.right, clip->bound.bottom);
	for (i=0; i<clip->count; i++)
	xprintf("clip[%d] bounds: [%d,%d,%d,%d]\n", i,
			clip->data[i].left, clip->data[i].top, clip->data[i].right, clip->data[i].bottom);	
*/
}

void DWindow::Acknowledge()
{
	int32		fault_count, trash_count;
	TWindow		*a_window;
	system_info	sys_info;

	if (owner->HasClosed()) return;
	if (!need_acknowledge) return;

	// wait for the acknowledge, checking for trashing in case of failure	
	do {
		get_system_info(&sys_info);
		fault_count = sys_info.page_faults;
		if (acquire_sem_etc(disable_sem_ack, 1, B_TIMEOUT, 3000000) == B_OK)
			goto happy_ending;		
		get_system_info(&sys_info);
		trash_count = sys_info.page_faults-fault_count;
	} while (trash_count >= 300);

	// unblock using brut force...
	send_into_debugger(owner, disable_sem_ack);

happy_ending:
	// auto hide/show cursor just after a STOP in full-screen mode
	if ((buf_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP)
		if (was_screen && software_cursor)
			the_cursor->Show();
}

//------------------------------------------------------------------------------------

extern	long			cur_switch;
extern	long			ws_count;
// Direct_screen_sem protects the graphic device ownership. It can be taken only once by
// a thread that wants to switch workspace or toggle the ownership between the app_server
// and the gamekit.
extern  sem_id			direct_screen_sem;
// Switch_sem id used to insure that only one thread at a time try to disconnect a
// WindowScreen client currently owning the screen.
extern	sem_id			switch_sem;
// The Region_sem is used to protect the configuration of the graphic device.


// Region_sem need to be locked before calling that function.
TWindow *current_window_screen() {
	TWindow		*w;

	// Return the WindowScreen potentialy blocking the screen, or NULL.	
	w = windowlist;
	while (w) {
		if ((!w->is_hidden()) &&
			(!w->is_mini) &&
			(w->is_screen()) &&
			(w->is_ws_visible(cur_switch)) &&
			(w->dw == NULL))
			return w;
		w = (TWindow*)w->nextwindow;
	}
	return NULL;
}


// Region_sem need to be locked before calling that function.
TWindow *current_direct_window() {
	TWindow		*w;

	// Return the DirectWindow currently blocking the screen, or NULL.	
	w = windowlist;
	while (w) {
		if ((!w->is_hidden()) &&
			(!w->is_mini) &&
			(w->is_screen()) &&
			(w->is_ws_visible(cur_switch)) &&
			(w->dw != NULL))
			return w;
		w = (TWindow*)w->nextwindow;
	}
	return NULL;
}


// Do an atomic transaction that force refresh of all windows.
void force_global_refresh() {
	TWindow		*a_window;

	wait_regions();
	open_atomic_transaction();
	a_window = (TWindow *)windowlist;
	while (a_window) {
		a_window->force_full_refresh = true;
		a_window = (TWindow *)a_window->nextwindow;
	}
	close_atomic_transaction();
	signal_regions();
}


int32 lock_direct_screen(int32 index, int32 state) {
	// The region sem protect the graphic device configuration
	wait_regions();
	if (state == true) {
		// Direct_screen_sem protects the graphic device ownership
		acquire_sem(direct_screen_sem);
		// Clone the device descriptor, for temporary use by the WindowScreen.
		if (BGraphicDevice::GameDevice(index) != NULL)
			delete BGraphicDevice::GameDevice(index);
		BGraphicDevice::SetGameDevice(index, new BGraphicDevice(BGraphicDevice::Device(index)));
		BGraphicDevice::Device(index)->DisconnectForGame(BGraphicDevice::GameDevice(index));
	}
	else if (state == false) {
		// Set back the regular device state and release the clone, if any.
		if (BGraphicDevice::GameDevice(index) != NULL) {
			BGraphicDevice::Device(index)->ResetFromGame(BGraphicDevice::GameDevice(index));
			delete BGraphicDevice::GameDevice(index);
			BGraphicDevice::SetGameDevice(index, NULL);
			// Force a global refresh of the screen, and show the cursor.
			force_global_refresh();
			the_cursor->Show();
			// Release exclusive right of graphic device cloning 
			release_sem(direct_screen_sem);
		}
	}
	signal_regions();
	return B_OK;
}


void acquire_direct_screen_and_region_sem(bool force_kill) {
	bool			activate;
	int32			count;
	BParcel			window_event(B_WORKSPACE_ACTIVATED);
	TWindow			*a_window;
	system_info		sys_info;
	
	// If the screen if not locked by a window screen, then it's a no brainer
	wait_regions();
	a_window = current_window_screen();
	if ((a_window == NULL) && force_kill)
		a_window = current_direct_window();
	if (a_window == NULL)
		acquire_sem(direct_screen_sem);
	else {
		// Make sure the window won't be deleted before the process is completed
		a_window->ServerLock();
		// To avoid some nasty deadlock (doesn't avoid all of them sadly. The ones left will
		// be unblock by the time-out that will kill the unlucky client).
		signal_regions();
		// To guarantee that only one thread at a time is trying to unlock the windowscreen.
		acquire_sem(switch_sem);

		if (!force_kill) {
			// Send a request for release of the screen to the WindowScreen client.
			activate = false;
			window_event.AddInt64("when",system_time());
			window_event.AddBool("active",activate);
			a_window->EventPort()->SendEvent(&window_event);

			// Grab the direct_screen semaphore (for synchronisation with WindowScreen client).
			get_system_info(&sys_info);
			do {
				count = sys_info.page_faults;
				if (acquire_sem_etc(direct_screen_sem, 1, B_TIMEOUT, 3000000) == B_OK)
					goto acquire_succeeded;
				get_system_info(&sys_info);
			} while ((sys_info.page_faults - count) >= 300);
		}

		// As we failed acquiring it, we need to suspend and clean-up the current holder 	
		send_into_debugger(a_window, direct_screen_sem);
		// Simulate the WindowScreen unlock
		lock_direct_screen(0, false);
		// Now, we're ready to acquire the direct_screen_sem.
		acquire_sem(direct_screen_sem);
		
acquire_succeeded:		
		// Reverse to initial locking conditions...
		release_sem(switch_sem);
		wait_regions();
		a_window->ServerUnlock();
	}
}


void release_direct_screen_and_region_sem() {
	release_sem(direct_screen_sem);
	signal_regions();
}


void reset_direct_screen_sem() {
	acquire_direct_screen_and_region_sem(true);
	release_direct_screen_and_region_sem();
}


void get_unblocked_screen(team_id bad_team, int32 *screen_index, int32 *ws_index) {	
	int32			new_switch;
	uint32			ws_mask;
	TWindow			*w, *target;

	wait_regions();

	/* Main clean-up case : the bad team owns the screen */
	target = current_window_screen();
	if (!target)
		target = current_direct_window();
	if (target && (target->team_id == bad_team))
		reset_direct_screen_sem();
	else
		target = NULL;
	
	/* look for a workspace that is not blocked by a fullscreen window */
	ws_mask = 0;
	w = windowlist;
	while (w) {
		if (!w->is_hidden() && !w->is_mini && w->is_screen() && (w != target))
			ws_mask |= w->get_ws();
		w = (TWindow*)w->nextwindow;
	}
	/* by default we will switch to the current workspace, or a free one */
	if ((ws_mask & (1<<cur_switch)) == 0)
		new_switch = cur_switch;
	else {
		new_switch = 0;
		while ((ws_mask & 1) == 1) {
			new_switch++;
			ws_mask >>= 1;
		}
	}
	/* we didn't find an usable workspace... */
	if (new_switch >= ws_count) {
//		kill_team(bad_team);
		new_switch = cur_switch;
	}

exit:		
	*screen_index = 0;
	*ws_index = new_switch;
	
	signal_regions();
}

long window_closer(void *data)
{
	TWindow		*w;
	
	birth();
	wait_regions();
	signal_regions();
	w = (TWindow*)data;
	w->do_close();
	w->ServerUnlock();
	suicide(0); // never returns
	return 0;
}


void send_into_debugger(TWindow *window, sem_id failed_acquisition) {
	char			name[B_OS_NAME_LENGTH];
	int32			i, cookie;
	team_info		tm_info;
	thread_id		client_task_id, last_dd_id, debug_task_id;
	thread_info		t_info;
	
	// block recursion
	if (window->flags & SENT_TO_DEBUG)
		return;
	window->flags |= SENT_TO_DEBUG;
	
	// If there is no other choise, just kill the team...
	if (window->flags & REQUIRE_KILL) {
		kill_team(window->team_id);
		for (i=0; i<38; i++) {
			snooze(100000);
			if (get_team_info(window->team_id, &tm_info) == B_BAD_TEAM_ID)
				break;
		}
		snooze(200000);
		return;
	}
	
	/* look for the client side window thread */
	client_task_id = window->client_task_id;
	/* look for the direct deamon thread, if needed */
	if (window->dw == NULL) {
		cookie = 0;
		last_dd_id = 0;
		while (get_next_thread_info(window->team_id, &cookie, &t_info) == B_OK)
			if (strcmp("direct deamon", t_info.name) == 0) {
				if ((last_dd_id == 0) ||
					(last_dd_id > client_task_id) ||
					((t_info.thread < client_task_id) && (t_info.thread > last_dd_id)))
					last_dd_id = t_info.thread;
			}
		if (last_dd_id != 0)
			client_task_id = last_dd_id;
	}
	/* look for a debugger thread */
	sprintf(name, "team %ld debugtask", window->team_id);
	cookie = 0;
	debug_task_id = 0;
	while (get_next_thread_info(window->team_id, &cookie, &t_info) == B_OK)
		if (strcmp(name, t_info.name) == 0) {
			debug_task_id = t_info.thread;
			break;
		}
	/* action !! */
	cookie = 0;
	while (get_next_thread_info(window->team_id, &cookie, &t_info) == B_OK)
		if ((t_info.thread != client_task_id) && (t_info.thread != debug_task_id))
			suspend_thread(t_info.thread);
	if (client_task_id != 0) {
		window->ServerLock();
		xspawn_thread(window_closer, "closer", B_DISPLAY_PRIORITY, (void*)window);
		if (debug_task_id == 0) {
			debug_thread(client_task_id);
			suspend_thread(client_task_id);
			resume_thread(client_task_id);
		}
		else
			suspend_thread(client_task_id);
//		send_signal(client_task_id, SIGDBUG);
	}
	// Hide the window, to make sure that nobody else is going to start another
	// clean-up before the end of the normal clean-up (done asynchronously by closer).
	window->show_hide = 1;
}


