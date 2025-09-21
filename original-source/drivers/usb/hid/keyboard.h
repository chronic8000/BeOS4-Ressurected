/* keyboard.h
 *
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <KernelExport.h>

#define KB_SHIFT						0x00000001
#define KB_COMMAND						0x00000002
#define KB_CONTROL						0x00000004
#define KB_CAPS_LOCK					0x00000008
#define KB_SCROLL_LOCK					0x00000010
#define KB_NUM_LOCK						0x00000020
#define KB_OPTION						0x00000040
#define KB_MENU							0x00000080
#define KB_LEFT_SHIFT					0x00000100
#define KB_RIGHT_SHIFT					0x00000200
#define KB_LEFT_COMMAND					0x00000400
#define KB_RIGHT_COMMAND				0x00000800
#define KB_LEFT_CONTROL					0x00001000
#define KB_RIGHT_CONTROL				0x00002000
#define KB_LEFT_OPTION					0x00004000
#define KB_RIGHT_OPTION					0x00008000

/* HID modifier bits */
#define HID_LEFT_CTRL					(1)
#define HID_LEFT_SHIFT					(1 << 1)
#define HID_LEFT_ALT					(1 << 2)
#define HID_LEFT_GUI					(1 << 3)
#define HID_RIGHT_CTRL					(1 << 4)
#define HID_RIGHT_SHIFT					(1 << 5)
#define HID_RIGHT_ALT					(1 << 6)
#define HID_RIGHT_GUI					(1 << 7)

/* YET ANOTHER redundancy in kb_mouse...sigh */
#define KB_IOCTL_SHIFT_KEY				0x00000001
#define KB_IOCTL_COMMAND_KEY			0x00000002
#define KB_IOCTL_CONTROL_KEY			0x00000004
#define KB_IOCTL_CAPS_LOCK				0x00000008
#define KB_IOCTL_SCROLL_LOCK			0x00000010
#define KB_IOCTL_NUM_LOCK				0x00000020
#define KB_IOCTL_OPTION_KEY				0x00000040
#define KB_IOCTL_MENU_KEY				0x00000080

/* HID LED bits */
#define HID_NUM_LOCK					(1)
#define HID_CAPS_LOCK					(1 << 1)
#define HID_SCROLL_LOCK					(1 << 2)

/* HID-specific requests */
#define HID_GET_REPORT					0x01
#define HID_GET_IDLE					0x02
#define HID_GET_PROTOCOL				0x03
#define HID_SET_REPORT					0x09
#define HID_SET_IDLE					0x0a
#define HID_SET_PROTOCOL				0x0b

#define KEYBOARD_PREFIX	"input/keyboard/usb"
#define DEFAULT_KEY_REPEAT_DELAY			250000
#define DEFAULT_KEY_REPEAT_INTERVAL			250000


/* macros for dealing with key_states - do not attempt to read */
#define IS_KEY_SET(ks, k)			((ks)[(k)>>3]&(1<<((7-(k))&7)))
#define SET_KEY(ks, k)				((ks)[(k)>>3]|=(1<<((7-(k))&7)))
#define UNSET_KEY(ks, k)			((ks)[(k)>>3]&=~(1<<((7-(k))&7)))
#define TOGGLE_KEY(ks, k)			((ks)[(k)>>3]^=(1<<((7-(k))&7)))

typedef struct _keyboard_device keyboard_device;
struct _keyboard_device {
	char			name[B_PATH_NAME_LENGTH];		/* input/keyboard/usb/n */
	int32			index;							/* 'n' in the above */

	bool			present;						/* is this device present? */
	int32			lock;							/* is this device in use? */
	usb_device		*device;						/* USB device handle */
	int32			interface_index;				/* USB interface index */
	usb_pipe		*int_endpoint;					/* USB interrupt endpoint */
	char			int_buffer[INT_BUFFER_SIZE];	/* buffer for interrupt data */
	size_t			packet_size;					/* actual interrupt data size */
	cbuf			*buffer;						/* keyboard input buffer */
	sem_id			sem;							/* data availability semaphore */
	
	/* USB error handling */
	bigtime_t		last_error_time;
	int	 			interrupt_error_count;
	
	uchar			key_states[32];
	uchar			non_repeating[32];
	uint32			old_modifiers;

	struct repeat_timer {
		timer				timer;
		keyboard_device		*kb;
		uchar				key;
	} timers[128];

	bigtime_t		repeat_delay;					/* delay before auto-repeat */
	bigtime_t		repeat_interval;				/* delay between key repeat */
	uint32			led_state;						/* state of the LED locks */
	timer			reset_timer;
	bigtime_t		control_alt_del_timeout;
	bool			reset_pending;
};

extern keyboard_device		*keyboard_list[MAX_USB_DEVICES];
extern sem_id				keyboard_list_sem;

extern status_t keyboard_added(usb_device *device, usb_interface_info *ii, int32 iindex);
extern status_t keyboard_removed(usb_device *device);
extern void free_keyboard(keyboard_device *device);
extern device_hooks			keyboard_device_hooks;

#endif /* KEYBOARD_H */
