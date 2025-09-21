/* ++++++++++
	FILE:	d2a.c
	REVS:	$Revision: 1.13 $
	NAME:	herold
	DATE:	Thu Oct 19 15:53:34 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <geek_hw.h>
#include <stdlib.h>

/* -----
	private globals
----- */

static short const d2a_addr[4] = {
	GEEK_D2A_0,
	GEEK_D2A_1,
	GEEK_D2A_2,
	GEEK_D2A_3
};

static vchar	*isa_io;						/* -> memory mapped isa i/o */
static char		last_value [4] = {0,0,0,0};		/* last value written */


/* ----------
	find_d2a - make sure io card is there
----- */
bool
find_d2a (void)
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

	isa_io = (vchar *) a.address;
	
	return TRUE;
}
	
/* ----------
	init_hardware - make sure geekport is there
----- */
status_t
init_hardware (void)
{
	return find_d2a() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	init_driver - set up driver globals
----- */
status_t
init_driver (void)
{
	return find_d2a() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	d2a_open - set up digital ports
----- */

static status_t
d2a_open(const char* name, uint32 flags, void** cookie)
{
	cpu_status	ps;
	int			id;

	/* distinguish open of channel 0..3 */

	id = name [strlen(name)-1] - '0';
	if (id < 0 || id > 3)
		return B_ERROR;
	
	*cookie = (void*) id; 		/* remember which channel we are */

	last_value[id] = *(isa_io + d2a_addr[id]);
	__eieio();

	return B_NO_ERROR;
}


/* ----------
	d2a_close
----- */

static status_t
d2a_close (void* cookie)
{
	cpu_status	ps;
	int			id;

	id = (int) cookie;
	if (id < 0 || id > 3)
		return B_ERROR;

	return B_NO_ERROR;
}


/* ----------
	d2a_read
----- */

static status_t
d2a_read (void* cookie, off_t position, void* buffer, size_t* num_bytes)
{
	int	 	id;
	cpu_status	ps;

	id = (int) cookie;
	if (id < 0 || id > 3)
		return B_ERROR;

	*(char *)buffer = last_value [id];

	return B_NO_ERROR;
}


/* ----------
	d2a_write
----- */

static status_t
d2a_write (void *cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	int	id;
	cpu_status	ps;

	id = (int) cookie;
	if (id < 0 || id > 3)
		return B_ERROR;

	last_value [id] = *(isa_io + d2a_addr[id]) = *(char *)buffer;
	__eieio();

	return B_NO_ERROR;
}


/* ----------
	d2a_control
----- */

static status_t
d2a_control (void* cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}

/* -----
	d2a free
----- */

static status_t 
d2a_free (void* cookie)
{
	/* no cookie to free, nothing was created */
	return B_NO_ERROR;
}

/* -----
	driver-related structures
----- */

device_hooks	d2a_devices[] =
{
	d2a_open, 		/* -> open entry point */
	d2a_close, 		/* -> close entry point */
	d2a_free,		/* -> free cookies allocated*/
	d2a_control, 	/* -> control entry point */
	d2a_read,		/* -> read entry point */
	d2a_write		/* -> write entry point */
};

static const char* d2a_device_names[] = 
{
	"beboxhw/geekport/d2a/d2a_0",
	"beboxhw/geekport/d2a/d2a_1",
	"beboxhw/geekport/d2a/d2a_2",
	"beboxhw/geekport/d2a/d2a_3",
	NULL
};

const char**
publish_devices()
{
	return	d2a_device_names;
}

device_hooks*
find_device(const char* name)
{
	return d2a_devices;
}

