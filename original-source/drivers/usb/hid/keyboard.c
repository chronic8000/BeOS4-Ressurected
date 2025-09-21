/* keyboard.c
 *
 */

#include <SupportDefs.h>
#include <Drivers.h>
#include <USB.h>
#include <malloc.h>
#include <stdio.h>
#include "cbuf.h"
#include "kb_mouse_driver.h"

#include "hid.h"
#include "keyboard.h"

status_t
keyboard_added(usb_device *device, usb_interface_info *ii, int32 iindex)
{
	int i;
	const usb_pipe *ep = NULL;
	keyboard_device *kb;
	uint8 current_led;
	size_t junk;
	
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

	kb = (keyboard_device *)malloc(sizeof(keyboard_device));
	if (kb == NULL)
		goto bail;

	kb->present = TRUE;
	kb->lock = 0;
	kb->device = device;
	kb->interface_index = iindex;
	kb->int_endpoint = ep;
	kb->packet_size = ii->endpoint[i].descr->max_packet_size;
	kb->buffer = NULL;
	kb->sem = -1;
	kb->control_alt_del_timeout = KB_DEFAULT_CONTROL_ALT_DEL_TIMEOUT;
	kb->reset_pending = FALSE;
	
	kb->last_error_time = 0;
	kb->interrupt_error_count = 0;

	memset(kb->key_states, 0, sizeof(kb->key_states));
	memset(kb->non_repeating, 0, sizeof(kb->non_repeating));
	memset(kb->timers, 0, sizeof(kb->timers));
	for (i = 0; i < 128; i++) {
		kb->timers[i].key = i;
		kb->timers[i].kb = kb;
	}
	kb->old_modifiers = 0;
	kb->repeat_delay = DEFAULT_KEY_REPEAT_DELAY;
	kb->repeat_interval = DEFAULT_KEY_REPEAT_INTERVAL;
	kb->led_state = 0;
	
	/* get the current state of the LEDs */
	if (usb->send_request(kb->device,
			USB_REQTYPE_INTERFACE_OUT|USB_REQTYPE_CLASS,
			HID_SET_REPORT, (0x02 << 8), kb->interface_index,
			sizeof(current_led), &current_led,
			sizeof(current_led), &junk) != B_OK) {
		zprintf("unable to get current state of LEDs\n");
	} else {
		/* it worked out okay...translate it */
		if (current_led & HID_CAPS_LOCK)
			kb->led_state |= KB_CAPS_LOCK;
		if (current_led & HID_NUM_LOCK)
			kb->led_state |= KB_NUM_LOCK;
		if (current_led & HID_SCROLL_LOCK)
			kb->led_state |= KB_SCROLL_LOCK;
	}
		
	/* set the device to be idle, we handle key repeat with timer interrupts */
	if (usb->send_request(
				kb->device,	USB_REQTYPE_INTERFACE_OUT|USB_REQTYPE_CLASS,
				HID_SET_IDLE, 0, kb->interface_index, 0, NULL, 0, &junk) != B_OK)
		zprintf("set_idle failed\n");

	acquire_sem(keyboard_list_sem);
	for (i = 0; i < MAX_USB_DEVICES; i++) {
		/* find a free slot and fill out the name */
		if (keyboard_list[i] == NULL) {
			keyboard_list[i] = kb;
			sprintf(kb->name, "%s/%d", KEYBOARD_PREFIX, i);
			kb->index = i;
			break;
		}
	}
	release_sem_etc(keyboard_list_sem, 1, B_DO_NOT_RESCHEDULE);

	/* if this ever happens, i will eat my hat */
	if (i == MAX_USB_DEVICES)
		goto bail2;

	return B_OK;

bail2:
	free(kb);
bail:
	return B_ERROR;
}

status_t
keyboard_removed(usb_device *device)
{
	int i;
	
	acquire_sem(keyboard_list_sem);
	for (i = 0; i < MAX_USB_DEVICES; i++) {
		/* this will break if we have a USB device that
		 * exports more than one keyboard interface
		 */
		if (keyboard_list[i] && keyboard_list[i]->device == device) {
			/* the device is gone */
			keyboard_list[i]->present = FALSE;
			/* stop any pending resets */
			cancel_timer (&keyboard_list[i]->reset_timer);
			/* unblock any readers */
			delete_sem(keyboard_list[i]->sem);
			keyboard_list[i]->sem = -1;
			if (atomic_add(&keyboard_list[i]->lock, 1) == 0)
				free_keyboard(keyboard_list[i]);
			break;
		}
	}
	release_sem_etc(keyboard_list_sem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}

void
free_keyboard(keyboard_device *kb)
{
	keyboard_list[kb->index] = NULL;
	free(kb);
}

/* index into this table with a USB keycode, come back out with a
 * kb_mouse keycode.  fun for the whole family.
 */
static uchar key_remapping_table[] = {
/*000*/	0x00, 0x00, 0x00, 0x00, 0x3c, 0x50, 0x4e, 0x3e, 0x29, 0x3f,
/*010*/	0x40, 0x41,	0x2e, 0x42, 0x43, 0x44, 0x52, 0x51, 0x2f, 0x30,
/*020*/	0x27, 0x2a, 0x3d, 0x2b,	0x2d, 0x4f, 0x28, 0x4d, 0x2c, 0x4c,
/*030*/	0x12, 0x13, 0x14, 0x15, 0x16, 0x17,	0x18, 0x19, 0x1a, 0x1b,
/*040*/	0x47, 0x01, 0x1e, 0x26, 0x5e, 0x1c, 0x1d, 0x31,	0x32, 0x33,
/*050*/	0x00, 0x45, 0x46, 0x11, 0x53, 0x54, 0x55, 0x3b, 0x02, 0x03,
/*060*/	0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
/*070*/	0x0e, 0x0f,	0x10, 0x1f, 0x20, 0x21, 0x34, 0x35, 0x36, 0x63,
/*080*/	0x61, 0x62, 0x57, 0x22,	0x23, 0x24, 0x25, 0x3a, 0x5b, 0x58,
/*090*/	0x59, 0x5a, 0x48, 0x49, 0x4a, 0x37, 0x38, 0x39, 0x64, 0x65,
/*100*/	0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*110*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*120*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*130*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*140*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*150*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*160*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*170*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*180*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*190*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*200*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*210*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*220*/	0x00, 0x00, 0x00, 0x00, 0x5c, 0x4b, 0x5d, 0x66, 0x60, 0x56,
/*230*/	0x5f, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*240*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*250*/	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void
fire_event(keyboard_device *kb, bool down, uchar key)
{
	raw_key_info io;
	
	io.is_keydown = down;
	io.be_keycode = key;
	io.timestamp = system_time();
	cbuf_putn(kb->buffer, (char *)&io, sizeof(io));
	release_sem_etc(kb->sem, 1, B_DO_NOT_RESCHEDULE);
}


static int32
repeat_handler(timer *t)
{
	struct repeat_timer *r = (struct repeat_timer *)t;

	fire_event(r->kb, TRUE, r->key);
	return B_HANDLED_INTERRUPT;
}

static int32
delay_handler(timer *t)
{
	struct repeat_timer *r = (struct repeat_timer *)t;

	fire_event(r->kb, TRUE, r->key);
	add_timer(t, repeat_handler, r->kb->repeat_interval, B_PERIODIC_TIMER);

	return B_HANDLED_INTERRUPT;
}

extern void do_control_alt_del();

/* ----------
	reset_timer_handler
----- */
static status_t
reset_timer_handler(timer *t)
{
	do_control_alt_del();			/* never returns... */
	return B_HANDLED_INTERRUPT; 	/* but just in case it does... */
}



static void
update_led_state(keyboard_device *kb)
{
	uint8 led_data;
	size_t len;
	
	led_data = 0;
	
	if (kb->led_state & KB_CAPS_LOCK)
		led_data |= HID_CAPS_LOCK;
	if (kb->led_state & KB_NUM_LOCK)
		led_data |= HID_NUM_LOCK;
	if (kb->led_state & KB_SCROLL_LOCK)
		led_data |= HID_SCROLL_LOCK;

	if (usb->send_request(kb->device,
			USB_REQTYPE_INTERFACE_OUT|USB_REQTYPE_CLASS,
			HID_SET_REPORT, (0x02 << 8), kb->interface_index,
			sizeof(led_data), &led_data, sizeof(led_data), &len) != B_OK)
		zprintf("error setting LED state\n");
}

static bool
handle_special_cases(keyboard_device *kb, uchar *current, uchar mods)
{
	if ( (mods & (HID_LEFT_ALT | HID_RIGHT_ALT)) && IS_KEY_SET(current, 0x0e)) {
		if ( IS_KEY_SET(current, 62) ) {
			kernel_debugger("You rang?");
		}
		return TRUE;
	}
	
	/* death grip */
	if ( (mods & (HID_LEFT_CTRL /*| HID_RIGHT_CTRL*/)) &&
	     (mods & (HID_LEFT_ALT /*| HID_RIGHT_ALT*/)) &&
		 (IS_KEY_SET(current, 0x34) || IS_KEY_SET(current, 0x65))) {
		if (!kb->reset_pending) {
			zprintf("usb keyboard: reset requested\n");
			add_timer(&kb->reset_timer, reset_timer_handler,
				kb->control_alt_del_timeout, B_ONE_SHOT_RELATIVE_TIMER);
			kb->reset_pending = TRUE;
		}
		return FALSE;
	
	/* end of death grip */
	} else if (kb->reset_pending) {
		zprintf("usb keyboard: reset aborted\n");
		cancel_timer(&kb->reset_timer);
		kb->reset_pending = FALSE;
	}
	
	return FALSE;
}

static void
simulate_modifier_keys (char *current, uchar mods)
{
	int				i;
	uchar			key;
	static uchar	bit_to_key[8] = {
		0x5c, 0x4b, 0x5d, 0x66, 0x60, 0x56, 0x5f, 0x67
	};
		
	for (i = 0; i < 7; i++) {
		if (mods & (1 << i)) {
			key = bit_to_key[i];
			SET_KEY(current, key);
		}
	}
}

static void
keyboard_handler(void *cookie, uint32 status, void *_data, uint32 actual_len)
{
	keyboard_device *kb = (keyboard_device *)cookie;
	uchar *data = (uchar *)_data;
	uchar current_state[32];
	uint32 mods;
	int i;
	uchar key;
	
	
	switch(status) {
	case B_OK:
		if(kb->interrupt_error_count > 0)
			kb->interrupt_error_count--;
		break;

	case B_CANCELED:
		zprintf("keyboard interrupt: canceled\n");
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
		if(t - kb->last_error_time < 2*1000*1000)
			kb->interrupt_error_count++;
		else
			if(kb->interrupt_error_count > 0)
				kb->interrupt_error_count--;
		kb->last_error_time = t;
		if(kb->interrupt_error_count < 4-1) {
			zprintf("keyboard int: error 0x%x, count %d\n", (unsigned)status, kb->interrupt_error_count);
			goto requeue;
		} else {
			zprintf("keyboard int: error 0x%x, count %d, giving up\n", (unsigned)status, kb->interrupt_error_count);
			return;
		}
		}
		break;

	/* It's more likely that B_TIMED_OUT will be received when the hub is unplugged, but B_DEV_INVALID_PIPE is possible too. */
	case B_DEV_INVALID_PIPE:	
		zprintf("keyboard int: unplugged\n");
		return;
		
	default:
		dprintf("keyboard int: fatal error %08x\n", (unsigned)status);
		return;
	}

	
#if 0
zprintf("KB: data=%02x%02x%02x%02x%02x%02x%02x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);
#endif

	mods = data[0];

	memset(current_state, 0, sizeof(current_state));

	/* make a key_state clone containing the current keyboard state */
	for (i = 2; i < actual_len; i++) {
		/* 0-3 = error codes */
		if (data[i] < 4)
			continue;
			
		key = key_remapping_table[data[i]];
		if (key == 0) {
			zprintf("UNKNOWN KEY: usb=0x%x\n", data[i]);
			continue;
		}

		SET_KEY(current_state, key);
	}

	/* alt-sysrq, control-alt-del, lock keys */
	if (handle_special_cases(kb, current_state, mods))
		goto requeue;
	
	/* simulate modifier keys */
	simulate_modifier_keys (current_state, mods);

/* this should never happen now that we set the keyboard to be idle */
#if 0
	if (!memcmp(kb->key_states, current_state, sizeof(current_state)))
		goto requeue;	
#endif

	for (key = 1; key < 128; key++) {
		/* the key has gone down */
		if (IS_KEY_SET(current_state, key) && !IS_KEY_SET(kb->key_states, key)) {
			fire_event(kb, TRUE, key);
			if (!IS_KEY_SET(kb->non_repeating, key))
				add_timer((timer *)&kb->timers[key], delay_handler, kb->repeat_delay,
							B_ONE_SHOT_RELATIVE_TIMER);
			continue;
		}
	
		/* the key has come up */
		if (IS_KEY_SET(kb->key_states, key) && !IS_KEY_SET(current_state, key)) {
			fire_event(kb, FALSE, key);
			if (!IS_KEY_SET(kb->non_repeating, key))
				cancel_timer((timer *)&kb->timers[key]);
			continue;
		}
	}
	memcpy (kb->key_states, current_state, sizeof (kb->key_states));

requeue:
	if(kb->present){
		usb->queue_interrupt(kb->int_endpoint, kb->int_buffer,
							 kb->packet_size, keyboard_handler, cookie);
	}
}

static status_t
keyboard_open(const char *name, uint32 flags, void **cookie)
{
	int i;
	keyboard_device *kb = NULL;

	acquire_sem(keyboard_list_sem);
	for (i = 0; i < MAX_USB_DEVICES; i++) {
		if (keyboard_list[i] && !strcmp(name, keyboard_list[i]->name)) {
			kb = keyboard_list[i];
			break;
		}
	}
	release_sem_etc(keyboard_list_sem, 1, B_DO_NOT_RESCHEDULE);

	if (kb == NULL) {
		zprintf("keyboard_open called with name=%s!!!\n", name);
		return B_ERROR;
	}
	
	if (atomic_add(&kb->lock, 1) > 0)
		goto bail;
		
	kb->buffer = cbuf_init(sizeof(raw_key_info) * 32);
	if (kb->buffer == NULL)
		goto bail;

	kb->sem = create_sem(0, "usb keyboard");
	if (kb->sem < B_OK)
		goto bail2;
		
	if (usb->queue_interrupt(kb->int_endpoint, kb->int_buffer,
				kb->packet_size, keyboard_handler, kb) != B_OK) {
		zprintf("unable to queue first keyboard interrupt!\n");
		goto bail3;
	}
	
	*cookie = kb;
	return B_OK;

bail3:
	delete_sem(kb->sem);
bail2:
	cbuf_delete(kb->buffer);	
bail:
	atomic_add(&kb->lock, -1);
	return B_ERROR;
}

static status_t
keyboard_close(void *cookie)
{
	keyboard_device *kb = (keyboard_device *)cookie;
	uchar key;
	
	/* cancel pending key-repeat timers */
	for (key = 0; key < 128; key++) {
		cancel_timer((timer *)&(kb->timers[key]));
	}
	
	/* make sure we receive no more interrupts */
	if(kb->present) usb->cancel_queued_transfers(kb->int_endpoint);
	
	return B_OK;
}

static status_t
keyboard_free(void *cookie)
{
	keyboard_device *kb = (keyboard_device *)cookie;

	/* clean up */
	delete_sem(kb->sem);
	cbuf_delete(kb->buffer);
	atomic_add(&kb->lock, -1);

	free_keyboard(kb);
	
	return B_OK;
}


/* if for some reason this is being used on a mac, someone will need
 * to implement KB_GETSTATES because the mac bootloader uses it
 */
static status_t
keyboard_control(void *cookie, uint32 op, void *buf, size_t len)
{
	status_t ret;
	keyboard_device *kb = (keyboard_device *)cookie;

	if (!kb->present)
		return ENODEV;
		
	switch(op) {
	case KB_READ: {
			raw_key_info locked_io;

			ret = acquire_sem_etc(kb->sem, 1, B_CAN_INTERRUPT, 0);
			if (ret != B_OK)
				return ret;

			cbuf_getn(kb->buffer, (char *)&locked_io, sizeof(raw_key_info));

			*(raw_key_info *)buf = locked_io;
			return B_OK;
	}
			
	case KB_SET_LEDS: {
			led_info *led = (led_info *) buf;

			kb->led_state = 0;
			if (led->caps_lock)
				kb->led_state |= KB_CAPS_LOCK;
			if (led->num_lock)
				kb->led_state |= KB_NUM_LOCK;
			if (led->scroll_lock)
				kb->led_state |= KB_SCROLL_LOCK;

			update_led_state(kb);
			return B_OK;
		}

	case KB_SET_KEY_REPEATING: {
		uint32 key = *(uint32 *)buf;

		if (key > (sizeof (kb->non_repeating) * 8))
			return EINVAL;

		UNSET_KEY (kb->non_repeating, key);
		return B_OK;
		}

	case KB_SET_KEY_NONREPEATING: {
		uint32 key = *(uint32 *)buf;

		if (key > (sizeof (kb->non_repeating) * 8))
			return EINVAL;

		SET_KEY (kb->non_repeating, key);
		return B_OK;
		}

	case KB_SET_KEY_REPEAT_RATE:
			kb->repeat_interval = (bigtime_t)(10000000 / *(int32 *)buf);
			return B_OK;
		
	case KB_GET_KEY_REPEAT_RATE:
			*(int32 *)buf = (int32)(kb->repeat_interval / 10000000);
			return B_OK;
	
	case KB_SET_KEY_REPEAT_DELAY:
			kb->repeat_delay = *(bigtime_t *)buf;
			return B_OK;

	case KB_GET_KEY_REPEAT_DELAY:
			*(bigtime_t *)buf = kb->repeat_delay;
			return B_OK;

	case KB_SET_CONTROL_ALT_DEL_TIMEOUT:
		kb->control_alt_del_timeout = *(bigtime_t *)buf;
		return B_NO_ERROR;

	case KB_DELAY_CONTROL_ALT_DEL:
		kb->control_alt_del_timeout = *(bigtime_t *)buf;
		if (kb->reset_pending)
		{
			cancel_timer(&kb->reset_timer);
			add_timer(&kb->reset_timer, reset_timer_handler,
				kb->control_alt_del_timeout, B_ONE_SHOT_RELATIVE_TIMER);
		}
		return B_NO_ERROR;

	case KB_CANCEL_CONTROL_ALT_DEL:
		if (kb->reset_pending) {
			zprintf("usb keyboard: reset cancelled\n");
			cancel_timer(&kb->reset_timer);
			kb->reset_pending = FALSE;
		}
		return B_NO_ERROR;
		
	default:
		zprintf("usb keyboard: unknown ioctl 0x%x\n", op);	
		return B_DEV_INVALID_IOCTL;
	}

	return B_OK;
}

static status_t
keyboard_read(void *cookie, off_t pos, void *data, size_t *len)
{
	return B_ERROR;
}

static status_t
keyboard_write(void *cookie, off_t pos, const void *data, size_t *len)
{
	return B_ERROR;
}

device_hooks keyboard_device_hooks = {
	keyboard_open,
	keyboard_close,
	keyboard_free,
	keyboard_control,
	keyboard_read,
	keyboard_write,
	NULL,	/* keyboard_select */
	NULL,	/* keyboard_deselect */
	NULL,	/* keyboard_readv */
	NULL	/* keyboard_writev */
};
