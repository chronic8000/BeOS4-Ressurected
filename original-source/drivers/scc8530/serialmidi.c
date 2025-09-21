/* ++++++++++
	FILE:	serialmidi.c
	REVS:
	NAME:	marc
	DATE:
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

	This is a midi layer over the serial driver.

	Modification History (most recent first):

+++++ */

#include "OS.h"
#include "fcntl.h"
#include "midi_driver.h"

#define NMIDIPORTS 2

typedef struct {
  int32 id;
} midi_cookie;

static long open_ports [NMIDIPORTS] = {0, 0};	/* per-port open flag */
static void* serial_cookie [NMIDIPORTS];

static char* midi_names[] = {"midi/midi1", "midi/midi2", NULL};

extern device_hooks serial_device;


/* ----------
	midi_open - set up the midi port
----- */

static status_t
midi_open (const char* dname, uint32 flags, void** _cookie)
{
	midi_cookie* cookie;
	status_t err;
	int32 id;

	for (id = 0; midi_names[id]; id++)
	  if (!strcmp (dname, midi_names[id]))
		break;
	if (midi_names[id] == NULL)
	  return B_ERROR;

	cookie = (midi_cookie*) malloc (sizeof (midi_cookie));
	if ((status_t) cookie < 0)
	  return (status_t) cookie;

	if (atomic_add (&open_ports[id], 1) == 0) {
	  flags &= ~(O_RDONLY | O_WRONLY);
	  err = serial_device.open (dname, flags | O_RDWR,
								&serial_cookie[id]);
	  if (err < 0) {
		atomic_add (&open_ports[id], -1);
		free (cookie);
		return err;
	  }
	}

	cookie->id = id;
	*(midi_cookie**)_cookie = cookie;
	return B_NO_ERROR;
}


/* ----------
	midi_close - shut down midi port if last one using it
----- */

static status_t
midi_close (void* _cookie)
{
    midi_cookie* cookie = (midi_cookie*)_cookie;
	void* sookie = serial_cookie[cookie->id];
	int32 old = atomic_add (&open_ports[cookie->id], -1);

	if (old <= 0) {									/* not open? */
		atomic_add (&open_ports[cookie->id], 1);	/* should not happen */
		return B_ERROR;
	}
	if (old == 1) {								/* last client? */
	  serial_device.close (sookie);
	  serial_device.free (sookie);				/* unsafe but what else? */
	}

	return B_NO_ERROR;
}


/* -----
	midi_free
----- */

static status_t
midi_free (void* _cookie)
{
  free (_cookie);
  return 0;
}


/* -----------
	midi_control - not much here
----- */

static status_t
midi_control (void* _cookie, uint32 msg, void* buf, size_t len)
{
  midi_cookie* cookie = (midi_cookie*)_cookie;
  return serial_device.control (serial_cookie[cookie->id],
								msg, buf, len);
}


/* ----------
	midi_read - read from the midi port
----- */

static status_t
midi_read (void* _cookie, off_t pos, void* buf, size_t* numBytes)
{
  midi_cookie* cookie = (midi_cookie*)_cookie;
  return serial_device.read (serial_cookie[cookie->id],
							 pos, buf, numBytes);
}


/* ----------
	midi_write - send characters out the midi port
----- */

static status_t
midi_write (void* _cookie, off_t pos, const void* buf, size_t* numBytes)
{
  midi_cookie* cookie = (midi_cookie*)_cookie;
  return serial_device.write (serial_cookie[cookie->id],
							  pos, buf, numBytes);
}

/*
 * loadable driver stuff
 */

device_hooks midi_device = {
	midi_open,
	midi_close,
	midi_free,
	midi_control,
	midi_read,
	midi_write
};
