/* mouse.h
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "hid.h"

#define MOUSE_PREFIX				"input/mouse/usb"
#define DEFAULT_ACCEL_FACTOR		7
#define DEFAULT_ACCEL_SPEED			1
#define DEFAULT_CLICK_INTERVAL		500000

#define HID_BUTTON_1					0x01
#define HID_BUTTON_2					0x02
#define HID_BUTTON_3					0x04

#define HID_WHEEL						0x01

typedef struct _mouse_device mouse_device;
struct _mouse_device {
	char			name[B_PATH_NAME_LENGTH];		/* input/mouse/usb/n */
	int32			index;							/* 'n' in the above */
	
	bool			present;						/* is this device present? */
	int32			lock;							/* is this device in use? */
	usb_device		*device;						/* USB device handle */
	usb_pipe		*int_endpoint;					/* USB interrupt endpoint */
	char			int_buffer[INT_BUFFER_SIZE];	/* buffer for interrupt data */
	size_t			packet_size;					/* actual interrupt data size */
	cbuf			*buffer;						/* mouse input buffer */
	sem_id			sem;							/* data availability semaphore */
	mouse_accel		accel;							/* mouse acceleration info */
	map_mouse		mapping;						/* button map */
	mouse_type		type;							/* number of buttons */
	int				flags;
	
	/* USB error handling */
	bigtime_t		last_error_time;
	int	 			interrupt_error_count;
	
	uint32			old_buttons;					/* button state at last int */
	uint32			old_held_buttons;				/* button state last time a button was held */
	bigtime_t		click_interval;					/* double-click time */
	bigtime_t		last_click;						/* time of last click */
	int32			click_count;					/* number of clicks */
};

extern mouse_device			*mouse_list[MAX_USB_DEVICES];
extern sem_id				mouse_list_sem;

extern status_t mouse_added(usb_device *device, usb_interface_info *ii, int32 iindex);
extern status_t mouse_removed(usb_device *device);
extern void free_mouse(mouse_device *device);

extern device_hooks			mouse_device_hooks;

#endif /* MOUSE_H */
