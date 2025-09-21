/* hid.h
 *
 */

#ifndef HID_H
#define HID_H

#include <SupportDefs.h>
#include <USB.h>
#include "cbuf.h"
#include "kb_mouse_driver.h"

#define DEBUG 1

#define IS_CONTROL_ENDPOINT(x)			(((x)->attributes & 3) == 0)
#define IS_INTERRUPT_ENDPOINT(x)		(((x)->attributes & 3) == 3)

/* some USB magic values */
#define MAX_USB_DEVICES					128
#define HID_CLASS						0x03
#define HID_BOOT_SUBCLASS				0x01
#define HID_KEYBOARD_PROTOCOL			0x01
#define HID_MOUSE_PROTOCOL				0x02

/* size of the interrupt buffer */
#define INT_BUFFER_SIZE				16

/* driver stuff */
#define DRIVER_NAME 	"hid"

/* using ... in macros is a GCC extension */
#if DEBUG
# if __GNUC__
#  define zprintf(f...)		dprintf("\x1b[31m" DRIVER_NAME "\x1b[0m: " f)
#  define zzprintf(f...)	dprintf("\x1b[34m" DRIVER_NAME "\x1b[0m: " f)
# else
#  warn debugging output is broken because this is not GCC
#  define zprintf
#  define zzprintf
# endif
#else /* DEBUG */
#define zprintf
#define zzprintf
#endif

extern usb_module_info		*usb;

#endif /* HID_H */
