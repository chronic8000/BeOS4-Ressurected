/* mouse.c
 *
 */

#include <SupportDefs.h>
#include <Drivers.h>
#include <USB.h>
#include <malloc.h>
#include <stdio.h>
#include "kb_mouse_driver.h"
#include "cbuf.h"

#include "hid.h"
#include "mouse.h"

/* stolen from kb_mouse.c -- this feels too fast on USB mice, though */
static void
accelerate(mouse_device *ms, mouse_pos *mp)
{
#if 0
	if (!ms->accel.enabled)
		return;
	
	if (ms->accel.accel_factor) {
		int x1, y1;
		x1 = (mp->xdelta*(256+((abs(mp->xdelta)>>2)<<ms->accel.accel_factor))) >> 8;
		if (abs(x1) > abs(mp->xdelta))
			mp->xdelta = x1;
		y1 = (mp->ydelta*(256+((abs(mp->ydelta)>>2)<<ms->accel.accel_factor))) >> 8;
		if (abs(y1) > abs(mp->ydelta))
			mp->ydelta = y1;
	}
	
	if (ms->accel.speed > 1) {
		mp->xdelta *= ms->accel.speed;
		mp->ydelta *= ms->accel.speed;
	}
#else
	return;
#endif
}

status_t
mouse_added(usb_device *device, usb_interface_info *ii, int32 iindex)
{
	int i;
	const usb_pipe *ep = NULL;
	const usb_device_descriptor *dd = NULL;
	mouse_device *ms;
		
	/* find the interrupt endpoint */
	for (i = 0; i < ii->endpoint_count; i++) {
		if (IS_INTERRUPT_ENDPOINT(ii->endpoint[i].descr)) {
			ep = ii->endpoint[i].handle;
			break;
		}
	}

	/* make sure we actually found one */
	if (ep == NULL)
		goto bail;

	ms = (mouse_device *)malloc(sizeof(mouse_device));
	if (ms == NULL)
		goto bail;
					
	/* fill out everything except for the name and the resources
	 * that will be allocated when the device is opened
	 */
	ms->present = TRUE;
	ms->lock = 0;
	ms->device = device;
	ms->int_endpoint = ep;
	ms->packet_size = ii->endpoint[i].descr->max_packet_size;
	ms->buffer = NULL;
	ms->sem = -1;
	
	ms->last_error_time = 0;
	ms->interrupt_error_count = 0;
	
	ms->accel.enabled = TRUE;
	ms->accel.accel_factor = DEFAULT_ACCEL_FACTOR;
	ms->accel.speed = DEFAULT_ACCEL_SPEED;

	ms->mapping.left = HID_BUTTON_1;
	ms->mapping.right = HID_BUTTON_2;
	ms->mapping.middle = HID_BUTTON_3;
	
	dd = usb->get_device_descriptor(device);
	switch(dd->vendor_id) {
		/* Logitech */
		case 0x046d:
			switch(dd->product_id) {
				case 0xc000: /* ?, N43 */
				case 0xc004: /* WingMan Gaming Mouse, M-BC38 */
					ms->flags &= ~HID_WHEEL;
				break;
				case 0xc001: /* Wheel Mouse, First Wheel Mouse, M-BB48 */
				case 0xc003: /* MouseMan Wheel, Scroll Wheel Mouse, M-BA47 */
				case 0xc00c: /* Wheel Mouse Optical, M-BD58 */
				case 0xc401: /* TrackMan Marble Wheel, T-BB13 */
				default:
					ms->flags |= HID_WHEEL;
				break;
			}
			break;
		default:
			ms->flags |= HID_WHEEL;
		break;
	}
	
	ms->type = MOUSE_3_BUTTON;
	
	ms->old_buttons = 0;
	ms->old_held_buttons = 0;
	ms->click_interval = DEFAULT_CLICK_INTERVAL;
	ms->last_click = 0;
	ms->click_count = 0;

	if (acquire_sem(mouse_list_sem) == B_OK) {
		for (i = 0; i < MAX_USB_DEVICES; i++) {
			/* find a free slot and fill in the name */
			if (mouse_list[i] == NULL) {
				mouse_list[i] = ms;
				sprintf(ms->name, "%s/%d", MOUSE_PREFIX, i);
				ms->index = i;
				break;
			}
		}
		release_sem_etc(mouse_list_sem, 1, B_DO_NOT_RESCHEDULE);
	}
		
	if (i == MAX_USB_DEVICES)
		goto bail2;
		
	return B_OK;

bail2:
	free(ms);
bail:
	return B_ERROR;
}

status_t
mouse_removed(usb_device *device)
{
	int i;
	status_t ret = B_ERROR;
	
	if (acquire_sem(mouse_list_sem) == B_OK) {
		for (i = 0; i < MAX_USB_DEVICES; i++) {
			if (mouse_list[i] && mouse_list[i]->device == device) {
				ret = B_OK;
				/* mark the device as gone and unblock any readers */
				mouse_list[i]->present = FALSE;
				delete_sem(mouse_list[i]->sem);
				mouse_list[i]->sem = -1;
				/* if someone is using the driver, it will be cleaned up
				 * during mouse_free.  if not, we have to do it.
				 */
				if (atomic_add(&mouse_list[i]->lock, 1) == 0)
					free_mouse(mouse_list[i]);
				break;
			}
		}
		release_sem_etc(mouse_list_sem, 1, B_DO_NOT_RESCHEDULE);
	}
		
	return ret;
}

void
free_mouse(mouse_device *ms)
{
	mouse_list[ms->index] = NULL;
	free(ms);
}

static uint32
remap_buttons(mouse_device *ms, uchar data)
{
	uint32 buttons = 0;
	
	if (data & HID_BUTTON_1)
		buttons |= ms->mapping.left;
	if (data & HID_BUTTON_2)
		buttons |= ms->mapping.right;
	if (data & HID_BUTTON_3)
		buttons |= ms->mapping.middle;
	
	return buttons;
}

static void
mouse_handler(void *cookie, uint32 status, void *_data, uint32 actual_len)
{
	mouse_device *ms = (mouse_device *)cookie;
	char *data = (char *)_data;
	mouse_pos mp;
	
	switch(status) {
	case B_OK:
		if(ms->interrupt_error_count > 0)
			ms->interrupt_error_count--;
		break;

	case B_CANCELED:
		zprintf("mouse interrupt: canceled\n");
		return;			

	/* Could be a transient error, keep trying. If the device is dead,
	timeout will be generated every 3 errors(UHCI/OHCI drivers) * 8ms(polling period) = 24 ms,	not a big deal.
	Give up after 4 rapid errors: otherwise dprint() from the real time DPC/interrupt calback will consume
	all CPU time and can prevent device removal handling so B_DEV_INVALID_PIPE will be never reached.
	 
	FIXME: detect a dead/stalled device and try to reset it/clear HALT. */
	case B_TIMED_OUT:
	case B_DEV_CRC_ERROR:
	case B_DEV_BAD_PID:
		{
		const bigtime_t	t = system_time();
		if(t - ms->last_error_time < 2*1000*1000)
			ms->interrupt_error_count++;
		else
			if(ms->interrupt_error_count > 0)
				ms->interrupt_error_count--;
		ms->last_error_time = t;
		if(ms->interrupt_error_count < 4-1) {
			zprintf("mouse int: error 0x%x, count %d\n", (unsigned)status, ms->interrupt_error_count);
			goto requeue;
		} else {
			zprintf("mouse int: error 0x%x, count %d, giving up\n", (unsigned)status, ms->interrupt_error_count);
			return;
		}
		}
		break;

	/* It's more likely that B_TIMED_OUT will be received when the hub is unplugged, but B_DEV_INVALID_PIPE is possible too. */
	case B_DEV_INVALID_PIPE:	
		zprintf("mouse int: unplugged\n");
		return;
		
	default:
		dprintf("mouse int: fatal error %08x\n", (unsigned)status);
		return;
	}
		
	
	mp.time = system_time();

	mp.buttons = remap_buttons(ms, data[0]);
	if (!ms->old_buttons && mp.buttons) {
		if (ms->last_click + ms->click_interval > mp.time && mp.buttons == ms->old_held_buttons)
			ms->click_count++;
		else
			ms->click_count = 1;
	}
	
	if (mp.buttons) {
		ms->last_click = mp.time;
		ms->old_held_buttons = mp.buttons;
	}
		
	ms->old_buttons = mp.buttons;
		
	mp.xdelta = data[1];
	mp.ydelta = -data[2];
	mp.clicks = ms->click_count;

	if ((ms->packet_size >= 4) && (ms->flags & HID_WHEEL))
		mp.wheel_delta = -data[3];
	else
		mp.wheel_delta = 0;
	
	cbuf_putn(ms->buffer, (char *)&mp, sizeof(mouse_pos));
	release_sem_etc(ms->sem, 1, B_DO_NOT_RESCHEDULE);

requeue:
	if(ms->present){
		usb->queue_interrupt(ms->int_endpoint, ms->int_buffer,
							 ms->packet_size, mouse_handler, cookie);
	}
}

status_t
mouse_open(const char *name, uint32 flags, void **cookie)
{
	int i;
	mouse_device *ms = NULL;

	acquire_sem(mouse_list_sem);
	for (i = 0; i < MAX_USB_DEVICES; i++) {
		if (mouse_list[i] && !strcmp(name, mouse_list[i]->name)) {
			ms = mouse_list[i];
			break;
		}
	}
	release_sem_etc(mouse_list_sem, 1, B_DO_NOT_RESCHEDULE);
	
	if (ms == NULL) {
		zprintf("mouse_open called with name=%s!!!\n", name);
		return B_ERROR;
	}

	if (atomic_add(&ms->lock, 1) > 0)
		goto bail;

	ms->buffer = cbuf_init(sizeof(mouse_pos) * 32);
	if (ms->buffer == NULL)
		goto bail;

	ms->sem = create_sem(0, "usb mouse");
	if (ms->sem < B_OK)
		goto bail2;

	/* queue the first mouse interrupt */
	if (usb->queue_interrupt(ms->int_endpoint, ms->int_buffer,
				ms->packet_size, mouse_handler, ms) != B_OK) {
		zprintf("unable to queue first mouse interrupt!\n");
		goto bail3;
	}
	
	*cookie = ms;
	return B_OK;

bail3:
	delete_sem(ms->sem);
bail2:
	cbuf_delete(ms->buffer);
bail:
	atomic_add(&ms->lock, -1);
	return B_ERROR;
}

status_t
mouse_close(void *cookie)
{
	mouse_device *ms = (mouse_device *)cookie;

	/* make sure we don't receive any more interrupts */
	if(ms->present) {
		usb->cancel_queued_transfers(ms->int_endpoint);
	}
	delete_sem(ms->sem);

	return B_OK;
}

status_t
mouse_free(void *cookie)
{
	mouse_device *ms = (mouse_device *)cookie;
	
	/* clean up */
	cbuf_delete(ms->buffer);
	atomic_add(&ms->lock, -1);

	free_mouse(ms);
	
	return B_OK;
}

status_t
mouse_control(void *cookie, uint32 op, void *buf, size_t len)
{
	status_t ret;
	mouse_device *ms = (mouse_device *)cookie;

	if (!ms->present)
		return ENODEV;
		
	switch(op) {
	case MS_NUM_EVENTS: {
			int32 count;
			get_sem_count(ms->sem, &count);
			return count;
		}
		
	case MS_READ: {
			mouse_pos locked_mp;
			
			ret = acquire_sem_etc(ms->sem, 1, B_CAN_INTERRUPT, 0);
			if (ret < B_OK)
				return ret;
			
			if (cbuf_getn(ms->buffer, (char *)&locked_mp, 
			    sizeof(mouse_pos)) != sizeof(mouse_pos))
					return B_ERROR;
			
//			accelerate(ms, &locked_mp);
			*(mouse_pos *)buf = locked_mp;

			return B_OK;
		}
		
	case MS_GETA: {
			mouse_accel *a = (mouse_accel *)buf;
			*a = ms->accel;
			return B_OK;
		}
			
	case MS_SETA: {
			mouse_accel *a = (mouse_accel *)buf;
			ms->accel = *a;
			return B_OK;
		}
				
	case MS_GETCLICK: {
			bigtime_t *interval = (bigtime_t *)buf;
			*interval = ms->click_interval;
			return B_OK;
		}

	case MS_SETCLICK: {
			bigtime_t *interval = (bigtime_t *)buf;
			ms->click_interval = *interval;
			return B_OK;
		}
		
	case MS_GETTYPE: {
			mouse_type *type = (mouse_type *)buf;
			*type = ms->type;
			return B_OK;
		}
		
	case MS_SETTYPE: {
			mouse_type type = *(mouse_type *)buf;
			if ((type < MOUSE_1_BUTTON) || (type > MOUSE_3_BUTTON))
				return EINVAL;
			ms->type = type;
			return B_OK;
		}
		
	case MS_GETMAP: {
			map_mouse *map = (map_mouse *)buf;
			*map = ms->mapping;
			return B_OK;
		}
		
	case MS_SETMAP: {
			map_mouse *map = (map_mouse *)buf;
			ms->mapping = *map;
			return B_OK;
		}
	}
	
	return B_DEV_INVALID_IOCTL;
}

static status_t
mouse_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	return B_ERROR;
}

static status_t
mouse_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	return B_ERROR;
}

device_hooks mouse_device_hooks = {
	mouse_open,
	mouse_close,
	mouse_free,
	mouse_control,
	mouse_read,
	mouse_write,
	NULL,	/* mouse_select */
	NULL,	/* mouse_deselect */
	NULL,	/* mouse_readv */
	NULL	/* mouse_writev */
};
