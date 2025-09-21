/* ++++++++++
	FILE:	digital.c
	REVS:	$Revision: 1.14 $
	NAME:	herold
	DATE:	Tue Jan 23 13:09:56 PST 1996
	Copyright (c) 1996-97 by Be Incorporated.  All Rights Reserved.

	Driver for the digital i/o portion of the geek port.
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <ByteOrder.h>
#include <Drivers.h>
#include <geek_hw.h>
#include <digital_driver.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf
#endif

/* -----
	private definitions and types
----- */

#define PORT_A_MASK	0x01
#define PORT_B_MASK	0x02


/* -----
	private globals
----- */

static int		open_count[2] = {0,0};	/* # outstanding opens to each port */
static spinlock	port_locks[2] = {0,0};	/* locks for port access */
static vshort 	*output_enable;			/* -> output enable reg */
static vchar	*port_addr[2];			/* -> ports */


/* ----------
	find_digport - determines if digital port hardware is present, and
	set up globals to point to it.
----- */
bool
find_digport (void)
{
	area_info	a;
	area_id		id;

	if (io_card_version() < 0)
		return FALSE;

	id = find_area ("isa_io");
	if (id < 0)
		return FALSE;
	if (get_area_info (id, &a) < 0)
		return FALSE;

	output_enable = (vshort *) ((char *)a.address + GEEK_OUTPUT_ENABLE);
	port_addr[0] = (vchar *)a.address + GEEK_DIGITAL_PORT_A;
	port_addr[1] = (vchar *)a.address + GEEK_DIGITAL_PORT_B;
	
	return TRUE;
}
	

/* ----------
	lock_port - gains exclusive access to hw, driver resources
----- */
static cpu_status
lock_port (int id)
{
	cpu_status	ps;
	
	ps = disable_interrupts();
	acquire_spinlock (port_locks + id);

	return ps;
}


/* ----------
	unlock_port - relinquish exclusive access to hw, driver resources
----- */
static void
unlock_port (int id, cpu_status ps)
{
	release_spinlock (port_locks + id);
	restore_interrupts(ps);
}


/* ----------
	set_digital_port_direction

	assumes port is locked, or does not have to be.
----- */
static void
set_digital_port_direction (int id, int direction)
{
	short		oe;
	static char	masks[2] = {PORT_A_MASK, PORT_B_MASK};

	oe = B_LENDIAN_TO_HOST_INT16 (*output_enable);
	__eieio();
 	if (direction == DIGITAL_PORT_INPUT)
		*output_enable = B_HOST_TO_LENDIAN_INT16 (oe & ~masks[id]);
	else
		*output_enable = B_HOST_TO_LENDIAN_INT16 (oe  | masks[id]);
	__eieio();
}
 

/* ----------
	init_hardware - looks for geekport, sets digital ports to inputs
----- */
status_t
init_hardware (void)
{
	if (!find_digport())
		return B_ERROR;

	set_digital_port_direction (0, DIGITAL_PORT_INPUT);
	set_digital_port_direction (1, DIGITAL_PORT_INPUT);

	return B_NO_ERROR;
}


/* ----------
	init_driver - set up driver globals
----- */
status_t
init_driver (void)
{
	return find_digport() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	digital_port_open - set up digital ports
----- */
static status_t 
digital_port_open (const char *name, uint32 flags, void **cookie)
{
	int			id;
	cpu_status	ps;

	/* distinguish open of port a from port b */

	ddprintf("opening '%s'\n", name);
	id = (name [strlen(name)-1] == 'a') ? 0 : 1;

	ps = lock_port (id);
	open_count [id] += 1;				/* count opens on this port */
	unlock_port (id, ps);

	*cookie = (void *)id;				/* remember which port this is */
	return B_NO_ERROR;
}



/* ----------
	digital_port_close
----- */
static status_t
digital_port_close (void *cookie)
{
	cpu_status	ps;
	int			id;

	id = (int) cookie;
	if (id < 0 || id > 1)				/* sanity check */
		return B_ERROR;

	ps = lock_port (id);

	open_count[id] -= 1;
	if (open_count[id] == 0)
		set_digital_port_direction (id, DIGITAL_PORT_INPUT);
	
	unlock_port (id, ps);
	
	return B_NO_ERROR;
}


/* ----------
	digital_port_read
----- */

static status_t
digital_port_read (void *cookie, off_t pos, void *buf, size_t *nbytes)
{
	int			id;

	id = (int) cookie;
	if (id < 0 || id > 1)				/* sanity check */
		return B_ERROR;

	*(char *)buf = *port_addr[id];

	return B_NO_ERROR;
}


/* ----------
	digital_port_write
----- */

static status_t
digital_port_write (void *cookie, off_t pos, const void *buf, size_t *nbytes) 
{
	int			id;

	id = (int) cookie;
	if (id < 0 || id > 1)				/* sanity check */
		return B_ERROR;

	*port_addr[id] = *(char *)buf;

	return B_NO_ERROR;
}


/* ----------
	digital_port_control
----- */

static status_t
digital_port_control (void *cookie, uint32 op, void *arg, size_t len)
{
	int 		id;
	int			dir;
	cpu_status	ps;

	id = (int) cookie;
	if (id < 0 || id > 1)				/* sanity check */
		return B_ERROR;

	switch (op) {
	case SET_DIGITAL_PORT_DIRECTION:

		dir = (int)arg;
		if (dir != DIGITAL_PORT_INPUT && dir != DIGITAL_PORT_OUTPUT)
			return B_ERROR;
		
		ps = lock_port (id);
		set_digital_port_direction (id, dir);
		unlock_port (id, ps);

		return B_NO_ERROR;
	}
	return B_ERROR;
}


/* -----------
    digital_port_free
----------- */

static status_t
digital_port_free (void *cookie)
{
	return B_NO_ERROR;
}


/* -----
	public globals and functions
----- */

static const char *digital_name[] = {
  "beboxhw/geekport/digital/digital_port_a",
  "beboxhw/geekport/digital/digital_port_b",
  NULL
};


device_hooks	digital_port_devices = 	{
    digital_port_open, 		/* -> open entry point */
    digital_port_close, 	/* -> close entry point */
    digital_port_free,      /* -> free cookies allocated */
    digital_port_control, 	/* -> control entry point */
    digital_port_read,		/* -> read entry point */
    digital_port_write		/* -> write entry point */
};

const char **
publish_devices()
{
	return digital_name;
}

device_hooks *
find_device(const char *name)
{
	return &digital_port_devices;
}




