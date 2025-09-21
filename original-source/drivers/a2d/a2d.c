/* ++++++++++
	FILE:	a2d.c
	REVS:	$Revision: 1.35 $
	NAME:	herold
	DATE:	Wed Oct 18 15:54:56 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

	The a2d and joystick drivers.
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <ByteOrder.h>
#include <fcntl.h>
#include <Drivers.h>
#include <geek_hw.h>
#include <joystick_driver.h>
#include <stdlib.h>

#define SUPPORT_REV3	1
#define DEBUG 1

#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf 
#endif

/* -----
	private globals
----- */
	
static vshort	*geek_output_enable;
static vshort	*geek_a2d_data;
static vshort	*geek_a2d_control;
static vshort	*joy_mux;
static vuchar	*joy_button;

static long		open_device [8] = {0,0,0,0,0,0,0,0};	/* a2d 0-3 joy 4-7 */
static sem_id	a2d_hw = -1;					/* sem to access a2d h/w */
static bool		joy_mux_delay_needed = FALSE;	/* early i/o cards were slow */


/* ----------
	find_a2d - determines if a2d hardware is present, and set up globals
	to point to it.
----- */
static bool
find_a2d (void)
{
	area_info	a;
	area_id		id;
	int			version;

	version = io_card_version();
	if (version < 0)
		return FALSE;

	id = find_area ("isa_io");
	if (id < 0)
		return FALSE;
	if (get_area_info (id, &a) < 0)
		return FALSE;

	geek_output_enable = (vshort *) ((long)a.address + GEEK_OUTPUT_ENABLE);
	geek_a2d_data = (vshort *) ((long)a.address + GEEK_A2D_DATA);
	geek_a2d_control =  (vshort *) ((long) a.address + GEEK_A2D_CONTROL);
	joy_mux =  (vshort *) ((long) a.address + JOY_MUX);
	joy_button = (vuchar *) ((long) a.address + JOY_BUTTON);

	if (version < 5)
		joy_mux_delay_needed = TRUE;	/* early i/o cards were slow */

	return TRUE;
}

/* ----------
	force_deref - work around compiler optimizing away volatile references by passing them
	to this function.
----- */
static void force_deref (int x) {}
	


/* ----------
	init_hardware - determine if a2d present, initialize it.
----- */
status_t
init_hardware (void)
{
	if (!find_a2d())
		return B_ERROR;

	force_deref (*geek_a2d_data);	/* clear old interrupts w/read */
	__eieio();

#if SUPPORT_REV3
	/* see if mux bits are in separate location, or shared w/digital port 
	   output enable register. */

	*joy_mux = 0;
	__eieio();
	if ((B_LENDIAN_TO_HOST_INT16 (*joy_mux) & 0x0300) == 0) {
		__eieio();
		*joy_mux = B_HOST_TO_LENDIAN_INT16 (0x0300);
		__eieio();
		if ((B_LENDIAN_TO_HOST_INT16 (*joy_mux) & 0x0300) != 0x0300) {
			ddprintf ("init_hardware: mux shared w/output enable\n");
			joy_mux = geek_output_enable;
		}
	}
	__eieio();
#endif
	return B_NO_ERROR;
}
	
/* ----------
	init_driver - does one-time initialization of the a2d driver.
----- */
status_t
init_driver (void)
{
	if (!find_a2d())
		return B_ERROR;

	if ((a2d_hw = create_sem (1, "a2d hw access")) < 0)
		return B_ERROR;

	set_sem_owner(a2d_hw, B_SYSTEM_TEAM);

	return B_NO_ERROR;
}


/* ----------
	uninit_driver - does cleanup of a2d driver
----- */
void
uninit_driver (void)
{
	if (a2d_hw >= 0)
		delete_sem (a2d_hw);

	a2d_hw = -1;
}


/* ----------
	a2d_hw_read
----- */
static ushort
a2d_hw_read (short control_reg)
{
	ushort	data;
	vshort	*ctl = geek_a2d_control;

	acquire_sem(a2d_hw);

	/* setup, start conversion */
	*ctl = B_HOST_TO_LENDIAN_INT16 (control_reg);
	__eieio();

	/* wait till conversion done */
	while (!(B_LENDIAN_TO_HOST_INT16 (*ctl) & 0x0800)) {
		__eieio();
		spin (10);
	}
	__eieio();

	/* read 12 bit result */
	data = B_LENDIAN_TO_HOST_INT16 (*geek_a2d_data) & 0x0FFF;
	__eieio();
	
	release_sem (a2d_hw);

	return data;
}


/* ----------
	joy_close
----- */

static status_t
joy_close (short id)
{
	if ((id < 4) || (id >7))
		return B_ERROR;

	if (open_device[id] == 0)
		return B_ERROR;

	open_device[id] = 0;

	return B_NO_ERROR;
}


/* ----------
	joy_read
----- */

static status_t
joy_read (int id, void *buf)
{
	short		mux;
	joystick	j;
	uchar		buttons;
	static struct {
		char	button_1_mask;
		char	button_2_mask;
		char	mux_x;
		char	mux_y;
		short	a2d_select;
	} info[4] = {
		{ 0x01, 0x08, 0x00, 0x01, 0x002C },	/* joystick 1  (port a, primary) */
		{ 0x02, 0x04, 0x02, 0x03, 0x002C },	/* joystick 2  (port a, secondary) */
		{ 0x10, 0x80, 0x00, 0x01, 0x006C },	/* joystick 3  (port b, primary) */
		{ 0x20, 0x40, 0x02, 0x03, 0x006C }	/* joystick 4  (port b, secondary) */
	};

	if ((id < 4) || (id > 7))
		return B_ERROR;
	id -= 4;

	/* set joystick mux for x coordinate, read it */

	mux = B_LENDIAN_TO_HOST_INT16 (*joy_mux);
	__eieio();
	mux = (mux & 0xFCFF) + (info[id].mux_x << 8);
	*joy_mux = B_HOST_TO_LENDIAN_INT16 (mux);
	__eieio();
	if (joy_mux_delay_needed)		/* early i/o cards were slow */
		spin (100);
	j.horizontal = a2d_hw_read (info[id].a2d_select);

	/* set joystick mux for y coordinate, read it */

	mux = B_LENDIAN_TO_HOST_INT16 (*joy_mux);
	__eieio();
	mux = (mux & 0xFCFF) + (info[id].mux_y << 8);
	*joy_mux = B_HOST_TO_LENDIAN_INT16 (mux);
	__eieio();
	if (joy_mux_delay_needed)		/* early i/o cards were slow */
		spin (100);
	j.vertical = a2d_hw_read (info[id].a2d_select);

	buttons = *joy_button;
	j.timestamp = system_time();

	j.button1 = (buttons & info[id].button_1_mask) ? TRUE : FALSE;
	j.button2 = (buttons & info[id].button_2_mask) ? TRUE : FALSE;

	memcpy (buf, &j, sizeof (j));

	return 1;
}


/* ----------
	a2d_close
----- */

static status_t
a2d_close (short id)
{

	if ((id < 0) || (id > 3))
		return B_ERROR;

	if (open_device[id] == 0)
		return B_ERROR;

	open_device[id] = 0;

	return B_NO_ERROR;
}


/* ----------
	a2d_read
----- */

static status_t
a2d_read (int id, void *buf)
{
	short		val;
	static short control_reg[4] = {
		0x000C,		/* a/d 0, single ended, unipolar */
		0x004C,		/* a/d 1, single ended, unipolar */
		0x001C,		/* a/d 2, single ended, unipolar */
		0x005C		/* a/d 3, single ended, unipolar */
	};

	if ((id >3) || (id <0))
		return B_ERROR;

	val = a2d_hw_read (control_reg[id]);
	*(char *)buf = val >> 8;
	*((char *)buf + 1) = val;

	return 2;
}

/* -----
	generic function used for driver-kernel interface
----- */

static status_t
device_open (const char *name, uint32 flags, void** cookie)
{
	int	id;
	
	ddprintf("trying to open %s.\n",name);

	if (flags != O_RDONLY) {
		dprintf("%s is read only. Thanks, come again...\n", name);
		return B_ERROR;
	}

	id = name [strlen(name)-1] - '1';
	if (id < 0 || id > 3)
		return B_ERROR;

	/* distinguish open of a2d from joystick */
	id = (name[strlen(name) - 3] == 'd') ? id : id + 4;
	ddprintf("driver_open: id is %x.\n", id);

	if (atomic_or (&open_device[id], 1))
		return B_ERROR;						/*already opened ? */
		
	*cookie = (void*) id;		/* ! not really giving a cookie adress, just an id */
	return B_NO_ERROR;
}

/* -----
	device close
----- */

static status_t device_close(void* cookie)
{
	short 	id;

	id = (int) cookie;
	if (id < 4)
		return a2d_close(id);
	else
		return joy_close(id);
}


/* -----
	device read
----- */

static status_t
device_read (void* cookie, off_t position, void* buffer, size_t* num_bytes)
{
	short	id;

	id = (int) cookie;
	if (id < 4)
		return a2d_read(id, buffer);
	else
		return joy_read(id, buffer);
}


/* -----
	device write: read only!
----- */
static status_t 
device_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


/* -----
	device control
----- */
static status_t
device_control (void* cookie, uint32 op, void* arg, size_t len)
{
	short id;

	id = (int) cookie;

	if ( (id > (-1)) && (id < 4)) {	/* hidden calls only for a2d */

		switch (op) {
		case 0xfeed0006:			/* hidden call: read channel 6 */
			return a2d_hw_read (0x003C);

		case 0xfeed0007:			/* hidden call: read channel 7 */
			return a2d_hw_read (0x007C);

		default:
			;
		}
	}
	return B_ERROR;
}

/* -----
	device_free - free my cookie!
----- */

static status_t 
device_free (void* cookie)
{
	/*no cookie was created, thanks come again */ 
	return B_NO_ERROR;
}


/* -----
	driver-related structures
----- */

device_hooks	a2d_devices[] = {
	device_open, 				/* -> open entry point */
	device_close, 				/* -> close entry point */
	device_free,				/* -> free cookies allocated */
	device_control, 			/* -> control entry point */
	device_read,				/* -> read entry point */
	device_write				/* -> write entry point */
};

static const char* a2d_device_names[] = {
	"beboxhw/geekport/a2d/a2d_1",
	"beboxhw/geekport/a2d/a2d_2",
	"beboxhw/geekport/a2d/a2d_3",
	"beboxhw/geekport/a2d/a2d_4",
	"joystick/joystick_1",
	"joystick/joystick_2",
	"joystick/joystick_3",
	"joystick/joystick_4",
	NULL
};

const char**
publish_devices()
{
	return a2d_device_names;
}

device_hooks* find_device(const char *name)
{
	return	a2d_devices;
}


