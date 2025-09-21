/* ++++++++++
	FILE:	midi.c
	REVS:	$Revision: 1.28 $
	NAME:	herold
	DATE:	Fri Jan 05 11:27:53 PST 1996
	Copyright (c) 1991-96 by Be Incorporated.  All Rights Reserved.

	This is a midi port driver.

	Modification History (most recent first):

	01 Oct 93	elr		new today (ex serial.c)
	02 May 96	rwh		rework to eliminate race conditions
+++++ */

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <interrupts.h>
#include "serial.h"
#include "midi_driver.h"

/* -----
	local types
----- */

#define OBUF_SIZE	512
#define IBUF_SIZE	512
#define NMIDIPORTS	2

#define BIT_TIME_USEC	(1000000/31250)
#define BYTE_TIME_USEC	((BIT_TIME_USEC)*11)

typedef struct {
  spinlock	lock;			/* spinlock for buffer access */
  long		size;			/* size of buffer */
  long		pending;		/* bytes in buffer */
  long		op;				/* index of first byte */
  char*		buf;			/* -> start of buffer */
} mbuf;

typedef struct {
	bigtime_t	r_timeout;	/* timeout for reads */
	mbuf		ibuf;		/* circular buffer for input data */
	mbuf		obuf;		/* circular buffer for output data */
	sem_id		rdsem;		/* wait semaphore for input */
	sem_id		wrsem;		/* wait semaphore for output */
	vuchar*		rbr;		/* -> receive buffer register */
	vuchar*		thr;		/* -> transmitter holding register */
	vuchar*		iir;		/* -> interrupt identification register */
	vuchar*		lcr;		/* -> line control register */
	vuchar*		lsr;		/* -> line status register */
	vuchar*		ier;		/* -> interrupt enable register */
	vuchar*		dll;		/* -> divisor latch lsbyte */
	vuchar*		dlm;		/* -> divisor latch msbyte */
	vuchar*		mcr;		/* -> modem control register */
	char		id;			/* midi port number */
    bool		overflow;	/* input buffer overflowing */
} midi_info;

typedef struct {
	ushort	offset;			/* offset in isa i/o space to port */
	uchar	interrupt_id;	/* interrupt number */
} midi_init;

/* -----
	globals for this module
----- */

static vuchar *isa_io;								/* -> isa i/o space */
static long open_ports [NMIDIPORTS] = {0, 0};		/* per-port open flag */
static midi_info midi_data [NMIDIPORTS];			/* per-port data */
static midi_init midi_init_table [NMIDIPORTS] = {	/* per-port init info */
	{MIDI1, B_INT_MIDI1},
	{MIDI2, B_INT_MIDI2}
};

/* ----------
	find_isa_bus - locate isa bus
----- */

static bool
find_isa_bus (void)
{
	area_info	a;
	area_id		id;

	id = find_area ("isa_io");
	if (id < 0)
		return FALSE;
	if (get_area_info (id, &a) < 0)
		return FALSE;

	isa_io = (vuchar *) a.address;
	return TRUE;
}


/* ----------
	init_hardware
----- */

status_t
init_hardware (void)
{
	return find_isa_bus() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	init_driver - locate isa bas
----- */

status_t
init_driver (void)
{
	return find_isa_bus() ? B_NO_ERROR : B_ERROR;
}


/* ----------
	mbuf_lock
----- */

static inline cpu_status
mbuf_lock (mbuf* mb)
{
  cpu_status ps = disable_interrupts();
  acquire_spinlock(&mb->lock);
  return ps;
}


/* ----------
	mbuf_unlock
----- */

static inline void
mbuf_unlock (mbuf* mb, cpu_status ps)
{
  release_spinlock(&mb->lock);
  restore_interrupts(ps);
}


/* ----------
	mbuf_init
----- */

static void
mbuf_init (mbuf* mb, long size)
{
  mb->size = size;
  mb->buf = (char*) malloc(size);
  mb->pending = 0;
  mb->op = 0;
  mb->lock = 0;
}


/* ----------
	mbuf_uninit
----- */

static void
mbuf_uninit (mbuf* mb)
{
  free (mb->buf);
}


/* ----------
	rdr_int - received data interrupt handler
----- */

static void
rdr_int (midi_info* mydata)
{
    char input_char = *mydata->rbr;
	mbuf* mb = &mydata->ibuf;
	long was_pending;
	cpu_status ps;

	//dprintf ("midi %d: rdr, r=%.8x\n", mydata->id, mydata->r_pending);

	ps = mbuf_lock(mb);
	was_pending = mb->pending;
	mb->buf[(mb->op + mb->pending++) % mb->size] = input_char;
	if (mb->pending > mb->size) {
	  mb->pending = mb->size;
	  mb->op = (mb->op + 1) % mb->size;
	}
	mbuf_unlock(mb, ps);
	
	if (was_pending == 0) {
	  release_sem_etc(mydata->rdsem, 1, B_DO_NOT_RESCHEDULE);
	  mydata->overflow = FALSE;
	}
	else if (was_pending == mb->size && !mydata->overflow) {
	  dprintf ("midi %d read interrupt: buffer overflow\n", mydata->id);
	  mydata->overflow = TRUE;
	}

	return;
}


/* ----------
	thre_int - transmitter holding register empty interrupt
----- */

static void
thre_int (midi_info* mydata)
{
	mbuf* mb = &mydata->obuf;
	long was_pending;
	cpu_status ps;

	//dprintf ("midi %d : thre, w=%.8x\n", mydata->id, mydata->w_pending);
	ps = mbuf_lock(mb);
	was_pending = mb->pending--;
	if (was_pending <= 0)
	  mb->pending = -1;						/* transmitter idle */
	else {
	  *mydata->thr = mb->buf[mb->op++];
	  if (mb->op >= mb->size)
		mb->op = 0;
	}
	mbuf_unlock(mb, ps);

	if (was_pending == mb->size)
	  release_sem_etc(mydata->wrsem, 1, B_DO_NOT_RESCHEDULE);
	return;
}


/* ----------
	lsr_int - line status interrupt
----- */

static void
lsr_int (midi_info* mydata)
{
    uchar sr = *mydata->lsr;
	dprintf ("midi %d: ", mydata->id);
	if (sr & LSR_overrun)
	  dprintf ("overrun error\n");
	else
	  dprintf ("line status interrupt %x\n", sr);
	return;
}


/* ----------
	inth - top level interrupt handler.  Determines interrupt source and
	dispatches to specific handler.
------ */

static int32
inth (void* data)
{
	midi_info* mydata = data;
	
	switch (*(mydata->iir) & 7) {
	case 0:
		dprintf ("midi: should not occur, modem ints disabled\n");
		break;
	case 1:		/* shouldn't occur, no interrupt source */
		break;
	case 2:		/* transmitter empty interrupt */
		thre_int (mydata);
		break;
	case 4:		/* received data available interrupt */
		rdr_int (mydata);
		break;
	case 6:		/* a line status interrupt */
		lsr_int (mydata);
		break;
	}
	return B_HANDLED_INTERRUPT;
}

/* ----------
	bebox_midi_open - set up the midi port
----- */

static status_t
bebox_midi_open (const char* dname, uint32 flags, void** _cookie)
{
	midi_info **cookie = (midi_info **) _cookie;
	const char** device_names = publish_devices();
	midi_info* mydata;
	vuchar* combase;
	midi_init* init;
	int id;

	if (!strncmp (dname, "/dev/", 5))
		dname += 5;

	for (id = 0; device_names[id]; id++)
	  if (!strcmp (dname, device_names[id]))
		break;
	if (device_names[id] == NULL)
	  return B_ERROR;

	mydata = midi_data + id;
	init = midi_init_table + id;
	*cookie = mydata;

	if (atomic_add (&open_ports[id], 1)) 		/* already open? */
		return B_NO_ERROR;

	mydata->id = id;
	mydata->overflow = FALSE;
	mydata->r_timeout = 1000000000000000LL;		/* one Saturnian year */
	mbuf_init (&mydata->obuf, OBUF_SIZE);
	mbuf_init (&mydata->ibuf, IBUF_SIZE);

	mydata->rdsem = create_sem (0, "midi read");
	set_sem_owner(mydata->rdsem, B_SYSTEM_TEAM);
	mydata->wrsem = create_sem (0, "midi write");
	set_sem_owner(mydata->wrsem, B_SYSTEM_TEAM);

	combase = isa_io + init->offset;

	mydata->lcr = combase + COM_LCR;
	mydata->mcr = combase + COM_MCR;
	mydata->dll = combase + COM_DLL;
	mydata->dlm = combase + COM_DLM;
	mydata->ier = combase + COM_IER;
	mydata->rbr = combase + COM_RBR;
	mydata->thr = combase + COM_THR;
	mydata->iir = combase + COM_IIR;
	mydata->lsr = combase + COM_LSR;

	*(mydata->ier) = 0;						/* disable all sources */
	*(mydata->iir) = 0;						/* reset all FIFOs */
	*(mydata->mcr) = MCR_IRQ_ENABLE;		/* and drive the IRQ pin */
	install_io_interrupt_handler (init->interrupt_id, inth, mydata, 0);
	
	*mydata->lcr = 0x83;		/* allow divisor access */
	*mydata->dlm = 0;			/* set up clock divisor for baud rate generator */
	*mydata->dll = 16;			/* 31250 baud */
	*mydata->lcr = 3;			/* remove divisor access */

	*(mydata->iir) = FCR_fifo_enable | FCR_fifo_level_1;
	*(mydata->ier) = IER_linestat_int | IER_thre_int | IER_rcv_data_int;

	/*   we always get a tranmitter empty interrupt on startup.  Wait for it,
		 then clean up and prepare for normal operation
	  while (mydata->w_pending == 0)
		snooze (1000);
	  atomic_add (&mydata->w_pending, 1);
	*/

	release_sem(mydata->wrsem);

	return B_NO_ERROR;
}


/* ----------
	bebox_midi_close - shut down midi port if last one using it
----- */

static status_t
bebox_midi_close (void* _mydata)
{
	midi_info *mydata = (midi_info *) _mydata;
	int			id;
	long		old;
	bigtime_t	end;

	id = mydata->id;

	old = atomic_add (&open_ports[id], -1);

	if (old <= 0) {							/* not open? */
		atomic_add (&open_ports[id], 1);	/* undo the damage */
		return B_ERROR;
	}

	if (old > 1)							/* not the last client? */
		return B_NO_ERROR;					/* no cleanup needed */

	/* wait for output buffer to flush, w/timeout in case it is hung */

	if (mydata->obuf.pending > 0) {
		end = system_time() + mydata->obuf.pending * BYTE_TIME_USEC;
		do {
			if (mydata->obuf.pending <= 0)
				break;
			snooze (2 * BYTE_TIME_USEC);
		} while (system_time() < end);
	}

	*(mydata->ier) = 0;						/* disable all interrupts */

	remove_io_interrupt_handler (midi_init_table[id].interrupt_id, inth,
									mydata);

	delete_sem (mydata->rdsem);
	delete_sem (mydata->wrsem);
	mbuf_uninit (&mydata->obuf);
	mbuf_uninit (&mydata->ibuf);

	return B_NO_ERROR;
}


/* -----
	bebox_midi_free
----- */

static status_t
bebox_midi_free (void* cookie)
{
  return B_NO_ERROR;
}


/* -----------
	bebox_midi_control - not much here
----- */

static status_t
bebox_midi_control (void* _mydata, uint32 msg, void* buf, size_t len)
{
    midi_info *mydata = (midi_info *) _mydata;

	switch (msg) {
	case B_MIDI_GET_READ_TIMEOUT:
	case B_MIDI_GET_READ_TIMEOUT_OLD:
		memcpy (buf, &mydata->r_timeout, sizeof (bigtime_t));
		return B_NO_ERROR;
	case B_MIDI_SET_READ_TIMEOUT:
	case B_MIDI_SET_READ_TIMEOUT_OLD:
		memcpy (&mydata->r_timeout, buf, sizeof (bigtime_t));
		return B_NO_ERROR;
	}
	return B_BAD_VALUE;
}


/* ----------
	bebox_midi_read - read from the midi port
----- */

static status_t
bebox_midi_read (void* _mydata, off_t pos, void* buf, size_t* numBytes)
{
  status_t err = B_NO_ERROR;
  midi_info *mydata = (midi_info *) _mydata;
  long count = *numBytes;
  char* read_buf = (char*) buf;
  mbuf* mb = &mydata->ibuf;
  long is_pending;
  cpu_status ps;
  long i = 0;

  while (i < count) {
	/* wait until input available */
	err = acquire_sem_etc(mydata->rdsem, 1, B_CAN_INTERRUPT | B_TIMEOUT,
						  mydata->r_timeout);
	if (err != B_NO_ERROR)
	  break;

	ps = mbuf_lock(mb);
	while (i < count && mb->pending > 0) {
	  read_buf[i++] = mb->buf[mb->op++];
	  if (mb->op >= mb->size)
		mb->op = 0;
	  --mb->pending;
	}
	is_pending = mb->pending;
	mbuf_unlock(mb, ps);

	if (is_pending > 0)
	  release_sem(mydata->rdsem);
  }

  *numBytes = i;
  return err;
}


/* ----------
	bebox_midi_write - send characters out the midi port
----- */

static status_t
bebox_midi_write (void* _mydata, off_t pos, const void* buf, size_t* numBytes)
{
  status_t err = B_NO_ERROR;
  midi_info *mydata = (midi_info *) _mydata;
  long count = *numBytes;
  char* write_buf = (char*) buf;
  mbuf* mb = &mydata->obuf;
  long is_pending;
  cpu_status ps;
  long i = 0;

  while (i < count) {
	/* wait until buffer space available */
	err = acquire_sem_etc (mydata->wrsem, 1, B_CAN_INTERRUPT, 0);
	if (err != B_NO_ERROR)
	  break;

	ps = mbuf_lock(mb);
	while (i < count && mb->pending < mb->size) {
	  if (mb->pending >= 0)
		mb->buf[(mb->op + mb->pending++) % mb->size] = write_buf[i++];
	  else {
		/* transmitter idle, start it up */
		*mydata->thr = write_buf[i++];
		mb->pending = 0;
	  }
	}
	is_pending = mb->pending;
	mbuf_unlock(mb, ps);

	if (is_pending < mb->size)
	  release_sem(mydata->wrsem);
  }

  *numBytes = i;
  return err;
}


/* -----
	driver-related structures
----- */

static char* midi_name[] = {"midi/midi1", "midi/midi2", NULL};

static device_hooks bebox_midi_device = {
		bebox_midi_open,		/* -> open entry point */
		bebox_midi_close,		/* -> close entry point */
		bebox_midi_free,		/* -> free entry point */
		bebox_midi_control,		/* -> control entry point */
		bebox_midi_read,		/* -> read entry point */
		bebox_midi_write		/* -> write entry point */
};

const char**
publish_devices()
{
	return midi_name;
}

device_hooks*
find_device(const char *name)
{
	return &bebox_midi_device;
}
