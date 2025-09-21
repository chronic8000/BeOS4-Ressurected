/* ++++++++++
	serial.c
	Copyright (C) 1991-2 Be Labs, Inc.  All Rights Reserved
	This is the driver for the isa-compatible serial ports.

	Modification History (most recent first):

	24 feb 95	elr		Substantial rewrite.
	?? ??? ??	elr		new today
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <ISA.h>
#include <Drivers.h>
#include <interrupts.h>
#include "cbuf.h"
#include "serial.h"

#include <termios.h>


/* this turns on printing of port statistics */
#define OUTPUT_BUFFER_SIZE			(512)
#define INPUT_BUFFER_SIZE			(2024)
#define	HIGH_WATER					(384)			/* bytes free to stop */
#define LOW_WATER					(512)			/* bytes free to restart */

#define	not				!
#define	unless(expr)	if (not (expr))
#define	and				&&
#define	nel( a)			(sizeof( a) / sizeof( (a)[0]))

static int _set();
static int32 ser_inth(void *);

static char *serial_name[] = {
#if __INTEL__
	"old/serial1",
	"old/serial2",
#else
	"ports/serial1",
	"ports/serial2",
	"ports/serial3",
	"ports/serial4",
	"ports/com3",
	"ports/com4",
#endif	
	NULL
};

/* ------------------------------------------------------------ */
/*
 * This serial driver manages all 16550 UARTs in the system, which are
 * SERIAL 1 & 2 (Same address as COM1 & 2, but on the Super I/O, using
 *	ISA interrupts.
 * SERIAL 3 & 4 (The first two serial ports on the TI quad UART, which
 * have their own I/O addresses distinct from COM 3 & 4)
 * COM 3 & 4 (Which use the I/O adress for COM 3 & 4, and ISA interrupt
 * irq4 and irq3.  These are REAL COM3 & 4 ports that might be in an ISA
 * slot.)
 */

#define NSERPORTS	6

#define SETAW	1
#define SETAF	2

#define	OUTPUT_STOPPED		(0x01)			/* bits in the stopped map */
#define INPUT_STOPPED		(0x02)

#if	PRINT_STATISTICS
struct serial_stat {
	int				msr_ints;
	int				lsr_ints;
	int				thre_ints;
	int				rdr_ints;
	int				flow_stops;
	int				flow_starts;
	int				inflow_stops;
	int				inflow_starts;
	int				overruns;
	int				parity_errors;
	int				framing_errors;
	int				break_interrupts;
	int				recv_chars;
	int				recv_ret_chars;
	int				xmit_chars;
	int				dropped_chars;
};
#endif

struct serial_drvr_data {
	cbuf			*ser_ibuf, *ser_obuf;
	sem_id			ser_rdsem, ser_wrsem;
	int				ser_closesem;
	int				ser_eventsem;
	int				rbr, thr, iir, lcr, lsr, ier, dll, dlm, mcr, msr;
	volatile char	ser_tidle;
	volatile char	stopped;
	char			portno;
	char			intmask;
	char			clock;
	unsigned char	event;
	unsigned char	send_break;
	unsigned char	deferred_set;
	struct termios	tctl;
};

#if PRINT_STATISTICS
static struct serial_stat		stats[NSERPORTS];
#endif

static struct serial_drvr_data  serial_data[NSERPORTS];
static unsigned char			open_ports[NSERPORTS] = {0, 0, 0, 0};

/* bit field defintions for init_flags */

#define CLOCK_SPEED		0x03	/* 2 bits of clock => 4 possible clock speeds */
#define CLOCK_24_MHZ	0x00
#define CLOCK_8_MHZ		0x01

/*
 * Test to see if another thread is trying to close us
 */
#define IS_CLOSING(x) ((x)->ser_rdsem == -1)

static struct drvr_init {
	ushort				offset;
	uchar				init_flags;
	uchar				int_id;
} drvr_init [NSERPORTS] = {
	{SERIAL1, CLOCK_24_MHZ, B_INT_COM1},
	{SERIAL2, CLOCK_24_MHZ, B_INT_COM2},
#ifndef __INTEL__
	{SERIAL3, CLOCK_8_MHZ, B_INT_SERIAL3},
	{SERIAL4, CLOCK_8_MHZ, B_INT_SERIAL4},
	{COM3, CLOCK_24_MHZ, B_INT_COM1},
	{COM4, CLOCK_24_MHZ, B_INT_COM2}
#endif	
};

static char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;

/* ----------
	init_hardware - see if serial ports are present
----- */

status_t
init_hardware (void)
{
	return B_NO_ERROR;
}


/* ----------
	init_driver - load-time driver initialization
----- */
status_t
init_driver()
{
#ifdef __INTEL__
	if(debug_output_enabled()) {
		open_ports[0] = 1;
	}
#elif __POWERPC__
	if (platform() == B_MAC_PLATFORM)
		return B_ERROR;
	if(debug_output_enabled()) {
		open_ports[3] = 1;
	}
#endif
	return get_module(isa_name, (module_info **) &isa);
}


void
uninit_driver()
{
	put_module(isa_name);
}


/*
 * Return the number of characters ready for reading
 * If blocking, wait up to the timeout before returning
 */
static long
waitevent(
		  struct serial_drvr_data *mydata
		  )
{
	long err;
	bigtime_t timeout;
	long      len;
	bigtime_t then;
	bigtime_t now;
	bigtime_t this_timeout;

	err = acquire_sem_etc(mydata->ser_closesem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_NO_ERROR) {
		return (err);
	}
	if (IS_CLOSING(mydata)) {
		len = B_INTERRUPTED;
	} else {
		timeout = ((bigtime_t)mydata->tctl.c_cc[VTIME]) * 100000;
		if (timeout == 0) {
			timeout = B_INFINITE_TIMEOUT;
		}
		this_timeout = timeout;
		while ((len = CBUF_AVAIL(mydata->ser_ibuf)) == 0) { 
			then = system_time();
			err = acquire_sem_etc(mydata->ser_rdsem, 1,
								  B_CAN_INTERRUPT|B_TIMEOUT, this_timeout);
			now = system_time();
			if (err < B_NO_ERROR) {
				if (err != B_TIMED_OUT) {
					/*
					 * If the error wasn't a timeout, then we should return
					 * the error back to the user
					 */
					len = err;
				}
				break;
			} 
			if ((len = CBUF_AVAIL(mydata->ser_ibuf)) > 0) {
				/*
				 * Allow a read to occur
				 */
				release_sem(mydata->ser_rdsem);
				break;
			}
			this_timeout -= (now - then);

			/*
			 * If the user wished to time out and we are out of time, return
			 */
			if (timeout != 0 && this_timeout <= 0) {
				break;
			}
		}
	}
	release_sem(mydata->ser_closesem);
	return (len);
}


/*
 * ser_control.
 */

static status_t
ser_control(void *_mydata, uint32 msg, void *buf, size_t len)
{
	struct serial_drvr_data *mydata = (struct serial_drvr_data *) _mydata;
	int		arg = (int) buf;

	/* dprintf("msg = %x/%x\n", msg/TCGETA); */
	switch (msg) {
	case TCGETA:
		return(ser_get(mydata, (struct termios *)buf));
	case TCSETA:
		return(ser_set(mydata, (struct termioss *)buf));
	case TCQUERYCONNECTED:
		if ((*isa->read_io_8)(mydata->msr) & 0x80)
			return 1;
		else
			return 0;
	case TCGETBITS:
		*(unsigned *)buf = (*isa->read_io_8)(mydata->msr) >> 4;
		break;

	case TCSETAF:
		/* SETA, deferred, with flush. */
		memcpy(&mydata->tctl, buf, sizeof(mydata->tctl));
		if( mydata->ser_tidle ) {
			cbuf_flush(mydata->ser_ibuf);
			_set(mydata);
		}
		else
			mydata->deferred_set = SETAF;
		return 0;
	case TCSETAW:
		/* SETA, deferred, no flush. */
		memcpy(&mydata->tctl, buf, sizeof(mydata->tctl));
		if( mydata->ser_tidle )
			_set(mydata);
		else
			mydata->deferred_set = SETAW;
		return 0;
	case TCWAITEVENT:
		len = waitevent(mydata);
		if (len >= 0 && buf)
			*(int *)buf = len;
		return (len);
	case TCSBRK:
		/* Send a break when the current obuf is mt */
		mydata->send_break = 1;
		if( mydata->ser_tidle )
			ser_thre_int(mydata);
		return 0;
	case TCFLSH:
		/* flush obuf, ibuf, or both */
		if( arg == 0 || arg == 2)
			cbuf_flush(mydata->ser_ibuf);
		if(arg == 1 || arg == 2)
			cbuf_flush(mydata->ser_obuf);
		return 0;
	case TCXONC:
		if( !(mydata->tctl.c_iflag & IXON) )
			return -1;
		if(arg == 0)
			mydata->stopped |= OUTPUT_STOPPED;
		if(arg == 1) {
			mydata->stopped &= ~OUTPUT_STOPPED;
			ser_thre_int(mydata); /* prime pump. */
		}
		return 0;
	case TCSETDTR:
		ser_setpin(mydata, MCR_DTR, *((bool *)buf));
		break;
	case TCSETRTS:
		ser_setpin(mydata, MCR_RTS, *((bool *)buf));
		break;
	case 'ichr':
		if (*(uint *)buf == 0) {
			cpu_status ps = cbuf_lock( mydata->ser_ibuf);
			*(uint *)buf = CBUF_AVAIL( mydata->ser_ibuf);
			cbuf_unlock( mydata->ser_ibuf, ps);
			return (B_OK);
		}
		return (B_ERROR);
	default:
		return(-1);
	}
	return (0);
}

static	void
interrupt_if_idle(struct serial_drvr_data *mydata)
{
	if (mydata->ser_tidle) {				/* if it is idle, prime pump */
		ser_thre_int(mydata);				/* simulate interrupt */
	}
}

#if PRINT_STATISTICS
void
print_statistics(struct serial_drvr_data *mydata)
{
	int dn = mydata->portno;

	dprintf("Serial port %d stats:\n", dn + 1);
	dprintf("Sent total %d chars, received %d chars\n", stats[dn].xmit_chars,
			stats[dn].recv_chars);
	dprintf("Characters pulled from ibuf %d\n", stats[dn].recv_ret_chars);
	dprintf("Dropped chars, not put in ibuf %d\n", stats[dn].dropped_chars);
	dprintf("lsr ints: %d msr ints: %d thre ints: %d rdr ints: %d\n",
			stats[dn].lsr_ints, stats[dn].msr_ints, stats[dn].thre_ints, stats[dn].rdr_ints);
	dprintf("outflow stops: %d outflow starts: %d inflow stops:%d inflow starts %d\n",
			stats[dn].flow_stops, stats[dn].flow_starts, stats[dn].inflow_stops, stats[dn].inflow_starts);
	dprintf("Overruns %d Parity: %d Framing: %d Breaks: %d\n", stats[dn].overruns,
			stats[dn].parity_errors, stats[dn].framing_errors, stats[dn].break_interrupts);
	dprintf("Obuf contains %d bytes, ibuf contains %d bytes\n", 
			cbuf_avail(mydata->ser_obuf), cbuf_avail(mydata->ser_ibuf) );
}
#endif

/* this is the serial port write routine.  It will send each character
 * until the output buffer is full, and then wait on the semaphore to
 * keep the output buffer full.
 */

static status_t
ser_write(void *_mydata, off_t pos, const void *buf,
		  size_t *len)
{
	struct serial_drvr_data *mydata = (struct serial_drvr_data *) _mydata;

	int						sent = 0, err, avail, n;
	bool					is_full;
	uchar					*s = (uchar*) buf;
	size_t					count, bytes_left;
	cbuf					*obuf;
	cpu_status				ps;
	int						timed_out;

	count = *len;
	bytes_left = count;
	obuf = mydata->ser_obuf;

	lock_memory((void *)buf, count, 0);
	while (bytes_left > 0) {

		timed_out = 0;

		err = acquire_sem_etc(mydata->ser_wrsem, 1, 
							  B_TIMEOUT | B_CAN_INTERRUPT, 3000000);
		if (err == B_TIMED_OUT) {
			if (!mydata->ser_tidle && !(mydata->stopped & OUTPUT_STOPPED)) {
				dprintf("Serial port timeout!\n");
				dprintf("lsr = %02x, stopped = %02x, idle = %d\n", 
						(*isa->read_io_8)(mydata->lsr),
						(uchar)mydata->stopped, mydata->ser_tidle);
#if PRINT_STATISTICS
				print_statistics(mydata);
#endif
				timed_out++;
			} else {
				continue;
			}
		} else if (err < B_NO_ERROR) {
			unlock_memory((void *)buf, count, 0);
			*len = (count - bytes_left);
			return(err);
		}

		ps = cbuf_lock(obuf);				/* put stuff in outbuf */
		avail = CBUF_FREE(obuf);			/* how much room is there? */
		n = min(avail, bytes_left);			/* how much should we put? */
		CBUF_PUTN(obuf, (char*)s, n);		/* put it in all at once */
		is_full = (CBUF_FREE(obuf) == 0);	/* are we now full? */
		if (timed_out) {
			mydata->ser_tidle = 1;
		}
		cbuf_unlock(obuf, ps);				/* unlock it */
		if (!is_full)						/* If it wasn't full, */
			release_sem(mydata->ser_wrsem); /* we release for next time */
		s += n;	bytes_left -= n; sent += n;	/* buf, count, amount sent */
		interrupt_if_idle(mydata);			/* if xmitter idle, start it */
	}
	unlock_memory((void *)buf, count, 0);
	return 0;
}

/*
 * test for re-enabling input. Done when a character is removed from cbuf,
 * perhaps freeing up space above the high water mark.
 */

static void
reenable_rts(struct serial_drvr_data *mydata)
{
	cpu_status	ps;

	ps = cbuf_lock(mydata->ser_ibuf);
	if ((mydata->tctl.c_cflag & RTSFLOW) && /* do we have HW Flow? */
		(mydata->stopped & INPUT_STOPPED) && /* is input currently stopped? */
		CBUF_FREE(mydata->ser_ibuf) >= LOW_WATER) {	/* and is there now room? */ 
#if PRINT_STATISTICS
		stats[mydata->portno].inflow_starts += 1;
#endif
		mydata->stopped &= ~INPUT_STOPPED;	/* unstopping input */
		(*isa->write_io_8)(mydata->mcr, (*isa->read_io_8)(mydata->mcr) | MCR_RTS);
		/* then assert RTS */
	}
	cbuf_unlock(mydata->ser_ibuf, ps);
}

/*
 * Serial read routine.  Note that the line discipline flags can greatly
 * affect the behavior of this routine.
 */


static status_t
ser_read_guts(struct serial_drvr_data *mydata, off_t pos, void *buf, size_t *len)
{
	int				i = 0;
	bigtime_t		timeout;
	char			c;
	size_t			count;
	unsigned char	vmin, vtime;
	unsigned short	lflag;
	unsigned short	iflag;
	static char 	*bs_sp_bs = "\b \b'";
	int				err, n;
	cpu_status		ps;
	cbuf			*ibuf;
	bool			is_now_mt;

	count = *len;
	if (IS_CLOSING(mydata)) {
		*len = 0;
		return (B_INTERRUPTED);
	}
	
	lflag = mydata->tctl.c_lflag;
	iflag = mydata->tctl.c_iflag;
	ibuf = mydata->ser_ibuf;

	/* We have modified the behavior of the serial driver.  It now has
	 * a simpler interface which is still based on the VMIN and VTIME
	 * flags.  VMIN is a boolean representation of whether or not to operate
	 * in a blocking fashion.  VMIN of zero means to return with whatever
	 * characters are currently available.  VMIN non-zero means to block 
 	 * until the read is satisfied.  In this case, VTIME is also used as a
	 * timeout value:  VTIME of zero means block forever, while non-zero
	 * values of VTIME give the number of 1/10 seconds to timeout.  That is
	 * to say, if VTIME is non-zero, we'll block for each character (until the
	 * read is satisfied) and if after VTIME * .1 seconds no more characters
	 * are available, then we'll return.
	 */

	/* There are three variants :
	   - VMIN = 0, VTIME irrelevant.
	   This immediately returns with the number of characters
	   waiting, or number requested, whichever is less
	   */
	vmin = mydata->tctl.c_cc[VMIN];
	vtime = mydata->tctl.c_cc[VTIME];

	if ( vmin == 0 ) {
		ps = cbuf_lock(ibuf);
		n = min(CBUF_AVAIL(ibuf), count);
		if (n > 0)
			CBUF_GETN(ibuf, buf, n);
		cbuf_unlock(ibuf, ps);
		reenable_rts(mydata);
#if PRINT_STATISTICS
		stats[mydata->portno].recv_ret_chars += n;
#endif
		*len = n;
		return 0;
	}

	/* The next variant is:
	   - VMIN > 0, VTIME = 0:
	   This will only return the read has been satisfied, otherwise
	   blocking forever.
	   */
	if ( (vmin > 0) && (vtime == 0) ) {
		while (count > 0) {
			reenable_rts(mydata);
			err= acquire_sem_etc(mydata->ser_rdsem, 1, B_CAN_INTERRUPT, 0);
			if (err) {
				*len = i;
				return (err);
			}
			ps = cbuf_lock(ibuf);
			n = min(CBUF_AVAIL(ibuf), count);
			if (n)
				CBUF_GETN(ibuf, buf, n);
			is_now_mt = (CBUF_AVAIL(ibuf) == 0);
			cbuf_unlock(ibuf, ps);
			if (!is_now_mt) 
				release_sem(mydata->ser_rdsem);

			i += n; buf = (void *)((char *)buf + n); count -= n;
#if PRINT_STATISTICS
			stats[mydata->portno].recv_ret_chars += n;
#endif
		}
		*len = i;
		return 0;
	}

	/* The last variant is:
	   - VMIN > 0, VTIME > 0:
	   This version will block, but only for VTIME time before
	   giving up.  At that point, the number of characters received
	   is returned.
	   */
	if ( (vmin > 0) && (vtime > 0) ) {
		timeout = (bigtime_t)vtime * 100000;
		if (timeout == 0) {
			timeout = B_INFINITE_TIMEOUT;
		}
		reenable_rts(mydata);
		err= acquire_sem_etc(mydata->ser_rdsem, 1, B_CAN_INTERRUPT|B_TIMEOUT,
							 timeout);
		if ( err && (err != B_TIMED_OUT)) {
			*len = 0;
			return (err);
		}

		ps = cbuf_lock(ibuf);
		n = min(CBUF_AVAIL(ibuf), count);
		if (n)
			CBUF_GETN(ibuf, buf, n);
		is_now_mt = (CBUF_AVAIL(ibuf) == 0);
		cbuf_unlock(ibuf, ps);
		if (!is_now_mt)
			release_sem(mydata->ser_rdsem);
#if PRINT_STATISTICS
		stats[mydata->portno].recv_ret_chars += n;
#endif
		*len = n;
		return 0;
	}
	*len = 0;
	return (0);
}

static status_t
ser_read(void *_mydata, off_t pos, void *buf, size_t *len)
{
	struct serial_drvr_data *mydata = (struct serial_drvr_data *) _mydata;
	size_t	count;
	int res;
	int error;

	count = *len;
	error = acquire_sem_etc(mydata->ser_closesem, 1, B_CAN_INTERRUPT, 0);
	if (error < B_NO_ERROR) {
		return (error);
	}

	count = *len;
	lock_memory(buf, count, B_READ_DEVICE);
	res = ser_read_guts(mydata, pos, buf, len);
	unlock_memory(buf, count, B_READ_DEVICE);
	release_sem(mydata->ser_closesem);
	return (res);
}

/*-------------------------------------------------------------------*/

/*
 * ser_open.  Set up the serial port to the default, which is:
 *	19200 baud, 8 bits, 1 stop, no parity, no flow, no modem
 */

static status_t
ser_open(const char *name, uint32 flags, void **_cookie)
{
	struct serial_drvr_data **cookie = (struct serial_drvr_data **) _cookie;

	char					dn;
	struct serial_drvr_data	*mydata;
	struct drvr_init		*dinit;
	int						combase;
	char					sbuf[32];

	for (dn = 0; dn < NSERPORTS; dn++)
	 	if (!strcmp(name, serial_name[dn]))
			break;
	if (dn >= NSERPORTS)
		return B_ENTRY_NOT_FOUND;

	mydata = serial_data + dn;
	*cookie = mydata;

	if (open_ports[dn])
		return B_PERMISSION_DENIED;

	mydata->ser_obuf = cbuf_init(OUTPUT_BUFFER_SIZE);
	mydata->ser_ibuf = cbuf_init(INPUT_BUFFER_SIZE);

	/*
 	 * Set up the default characteristics.  Mapping CR->NL, 19200 bps,
	 * 8 data bits, 1 stop, even parity.  Canonical input processing.
 	 */
	mydata->tctl.c_iflag = 0;
	mydata->tctl.c_oflag = 0;
	mydata->tctl.c_cflag = B19200 | CS8 | CTSFLOW | RTSFLOW;
	mydata->tctl.c_lflag = 0;
	mydata->tctl.c_cc[VMIN] = 1;
	mydata->tctl.c_cc[VTIME] = 0;
	sprintf(sbuf, "serial%d read block", dn+1);
	mydata->ser_rdsem = create_sem(0, sbuf);
	set_sem_owner(mydata->ser_rdsem, B_SYSTEM_TEAM);
	sprintf(sbuf, "serial%d write block", dn+1);
	mydata->ser_wrsem = create_sem(1, sbuf);
	set_sem_owner(mydata->ser_wrsem, B_SYSTEM_TEAM);

	sprintf(sbuf, "Ser%d event", dn+1);
	mydata->ser_eventsem = create_sem(0, sbuf);
	mydata->portno = dn;
	mydata->ser_tidle = 1;
	mydata->stopped = 0;
	mydata->deferred_set = 0;
	mydata->send_break = 0;
	sprintf(sbuf, "Ser%d close", dn+1);
	mydata->ser_closesem = create_sem(1, sbuf);

	dinit = drvr_init + dn;
	combase = dinit->offset;

	mydata->lcr = combase + COM_LCR;
	mydata->mcr = combase + COM_MCR;
	mydata->dll = combase + COM_DLL;
	mydata->dlm = combase + COM_DLM;
	mydata->ier = combase + COM_IER;
	mydata->rbr = combase + COM_RBR;
	mydata->thr = combase + COM_THR;
	mydata->iir = combase + COM_IIR;
	mydata->lsr = combase + COM_LSR;
	mydata->msr = combase + COM_MSR;

	(*isa->write_io_8)(mydata->ier, 0);					/* disable all sources */
	(*isa->write_io_8)(mydata->iir, 0);					/* reset all FIFOs */
	(*isa->write_io_8)(mydata->mcr, MCR_IRQ_ENABLE);	/* and drive the IRQ pin */
	install_io_interrupt_handler (dinit->int_id, ser_inth, mydata, 0);

	mydata->clock = (dinit->init_flags & CLOCK_SPEED);

	/* now set the hardware to match... */
	_set(mydata);
	(*isa->write_io_8)(mydata->iir, FCR_fifo_enable | FCR_fifo_level_1);
	(*isa->write_io_8)(mydata->ier,
			IER_modemstat_int | IER_linestat_int | IER_thre_int |
			IER_rcv_data_int);

#if PRINT_STATISTICS
	memset(&stats[dn], 0, sizeof(stats[dn]));
#endif
	open_ports[dn] = 1;
	return(0);
}

/*
 * Serial close.  We close the driver, if it is open.
 */

static status_t
ser_close(void *_mydata)
{
	struct serial_drvr_data *mydata = (struct serial_drvr_data *) _mydata;
	int						dn;
	struct drvr_init		*dinit;
	int counter = 0;

	dn = mydata->portno;

	if (!open_ports[dn])					/* if not open, don't close it */
		return B_NO_ERROR;

	while (!cbuf_mt(mydata->ser_obuf)) {
		if (counter++ > 5000)
			break;
		if (mydata->stopped & OUTPUT_STOPPED)
			break;
		snooze(5000);
	}
	counter = 0;
	while (((*isa->read_io_8)(mydata->lsr) & LSR_temt) == 0) {
		if (counter++ > 1000)
			break;
		snooze(2000);						/* wait for shift reg to empty */
	}

	dinit = drvr_init + dn;					/* point at init structure */

	remove_io_interrupt_handler (dinit->int_id, ser_inth, mydata);

	(*isa->write_io_8)(mydata->ier, 0);					/* disable all interrupts */
	(*isa->write_io_8)(mydata->mcr, 0);					/* disable RTS/DTR and IRQout */
	(*isa->write_io_8)(mydata->iir, 0);					/* reset all FIFOs */

	open_ports[dn] = 0;
	delete_sem (mydata->ser_rdsem);
	delete_sem (mydata->ser_wrsem);
	delete_sem (mydata->ser_eventsem);

	mydata->ser_rdsem = -1;
	acquire_sem(mydata->ser_closesem);
	delete_sem(mydata->ser_closesem);

#if PRINT_STATISTICS
	print_statistics(mydata);
#endif
	cbuf_delete (mydata->ser_obuf);
	cbuf_delete (mydata->ser_ibuf);
	return B_NO_ERROR;
}

static status_t
ser_free(void *mydata)
{
	return 0;
}

static int
_set(struct serial_drvr_data *mydata)
{
	cpu_status		ps;
	struct termios	*tp;
	static ushort	divisors[][2] = { 
		/* 24 Mhz	8 Mhz */
		{ 2304,		10000 },	/* 0 baud */
		{ 2304,		10000 },	/* 50 baud */
		{ 1536,		6667 },		/* 75 baud */
		{ 1047,		4545 },		/* 110 baud */
		{ 857,		3717 },		/* 134 baud */
		{ 768,		3333 },		/* 150 baud */
		{ 512,		2500 },		/* 200 baud */
		{ 384,		1667 },		/* 300 baud */
		{ 192,		833 },		/* 600 baud */
		{ 96,		417 },		/* 1200 baud */
		{ 64,		277 },		/* 1800 baud */
		{ 48,		208 },		/* 2400 baud */
		{ 24,		104 },		/* 4800 baud */
		{ 12,		52 },		/* 9600 baud */
		{ 6,		26 },		/* 19200 baud */
		{ 3,		13 }, 		/* 38400 baud */
		{ 2,		9 },		/* 57600 baud */
		{ 1, 		4 },		/* 115200 baud (7.89% error with 8 MHz) */
		{ 0, 		2 },		/* 230400 baud (7.89% error with 8 MHz) */
		{ 4,		16 }		/* 31250 baud (8% error with 24 MHz) */
	};
	uchar	mcr, lcr;
	ushort	the_divisor;

	tp = &mydata->tctl;

	unless (((tp->c_cflag&CBAUD) < nel( divisors))
	and (the_divisor = divisors[tp->c_cflag&CBAUD][mydata->clock]))
		return (-1);

	/* setup the line control register as appropriate */
	if( tp->c_cflag & CS8 )
		lcr = LCR_8bit;
	else
		lcr = LCR_7bit;

	if( tp->c_cflag & CSTOPB )
		lcr |= LCR_2stop;

	if( tp->c_cflag & PARENB )
		lcr |= LCR_parity_enable;

	if( !(tp->c_cflag & PARODD) )
		lcr |= LCR_even_parity;
	

	ps = cbuf_lock(mydata->ser_ibuf);

	(*isa->write_io_8)(mydata->lcr, LCR_divisor_access);	/* gain access to divisor regs (see manual) */
	(*isa->write_io_8)(mydata->dll, the_divisor & 0x00ff);
	(*isa->write_io_8)(mydata->dlm, the_divisor >> 8);
	(*isa->write_io_8)(mydata->lcr, lcr);	/* configure lcr, turning off divisor access */
	
	mcr = MCR_RTS | MCR_DTR;

	if (tp->c_cflag & (CTSFLOW | RTSFLOW)) {			/* HW Flow ON */
		mydata->stopped = 0;							/* default OK */
		if (CBUF_FREE(mydata->ser_ibuf) >= HIGH_WATER) 	/* check input buffer */
			mcr |= MCR_RTS;								/* OK, assert RTS */
		else											/* input full so... */
			mydata->stopped |= INPUT_STOPPED;			/* mark us stopped */
		if (((*isa->read_io_8)(mydata->msr) & MSR_CTS) == 0) {	/* are they stopping us? */
			mydata->stopped |= OUTPUT_STOPPED;			/* yes, mark the state */
		}
	} else {											/* HW Flow OFF */
		mydata->stopped = 0;							/* unstop */
		/*ser_thre_int(mydata);							/* and let her rip */
	}
	(*isa->write_io_8)(mydata->mcr, (*isa->read_io_8)(mydata->mcr) | mcr);	/* flip on RTS, DTR */
	cbuf_unlock(mydata->ser_ibuf, ps);
	return(0);
}

/*
 * ser_setpin.  Assert or de-assert the requested pin (DTR or RTS)
 */

static
ser_setpin(struct serial_drvr_data *mydata, char pin, bool assert)
{
	if (assert)
		(*isa->write_io_8)(mydata->mcr, (*isa->read_io_8)(mydata->mcr) | pin);	/* assert requested pin */
	else
		(*isa->write_io_8)(mydata->mcr, (*isa->read_io_8)(mydata->mcr) & ~pin);	/* de-assert requested pin */
	return (0);
}


/*
 * ser_set.  For the drvr, take the control structure tp and copy it to the
 * local copy we keep.  Then call _set to actually make hardware look like
 * what we want.
 */

static
ser_set(struct serial_drvr_data *mydata, struct termios *tp)
{
	memcpy(&mydata->tctl, tp, sizeof (mydata->tctl));

	return _set(mydata);
}

/*
 * Ser_get.  Take an externally formatted serial line record buffer and
 * fill it in with the current settings of this driver.
 */

static
ser_get(struct serial_drvr_data *mydata, struct termios *tp)
{
	memcpy(tp, &mydata->tctl, sizeof (struct termios));
	return(0);
}

/*
 * ser_inth.  This is an interrupt handler for a UART.  dn is the port #
 * we think the interrupt is for.
 */

static int32
ser_inth (void *data)
{
	struct serial_drvr_data *mydata = data;

	while (1) {
		switch ((*isa->read_io_8)(mydata->iir) & IIR_bits) {
		case IIR_modemstat_int:
			ser_msr_int(mydata);
			break;
		case IIR_thre_int:							/* xmitter holding empty interrupt */
			ser_thre_int(mydata);
			break;
		case IIR_rcv_data_int:						/* received data available */
		case IIR_timeout_int:
			ser_rdr_int(mydata);
			break;
		case IIR_linestat_int:						/* line status interrupt */
			ser_lsr_int(mydata);
			break;
		default:
		case IIR_no_interrupt:
			return B_HANDLED_INTERRUPT;
		}
	}
	return B_HANDLED_INTERRUPT;
}

#define XON 	0x11
#define XOFF 	0x13

/*
 * This is called from the receive interrupt handler.  As characters are
 * coming in, we must decide if we'd like to throttle the sender, and if
 * so whether we're allowed to given the state of both hardware or software
 * flow control having been enabled. This is called with the ibuf spinlock
 * already obtained.
 */

static void
evaluate_stopping_input(struct serial_drvr_data *mydata)
{

	if (CBUF_FREE(mydata->ser_ibuf) >= HIGH_WATER)		/* no crisis! */
		return;

	if (mydata->stopped & INPUT_STOPPED)				/* crisis, knew it! */
		return;

	if (mydata->tctl.c_cflag & RTSFLOW) {				/* is HW flow on? */
		mydata->stopped |= INPUT_STOPPED;				/* note it */
		(*isa->write_io_8)(mydata->mcr, 			       
			   (*isa->read_io_8)(mydata->mcr) & ~MCR_RTS);		/* Drop RTS - we're behind. */ 
	}

	if (mydata->tctl.c_cflag & IXON) {					/* This is SW Flow */
		/* Send an XOFF! */
	}
#if PRINT_STATISTICS
	stats[mydata->portno].inflow_stops += 1;
#endif
}

/* receive data ready interrupt 
 */
static
ser_rdr_int(struct serial_drvr_data *mydata)
{
	unsigned char	lsr, c, tb[32];
	unsigned char	*s = tb;
	int				n, scount = 0;
	cpu_status		ps;
	bool			was_mt;
	cbuf			*ibuf = mydata->ser_ibuf;

#if PRINT_STATISTICS
	stats[mydata->portno].rdr_ints += 1;
#endif
	ps = cbuf_lock(ibuf);							/* gain control of ibuf */
	evaluate_stopping_input(mydata);				/* do we do flow control? */
	was_mt = (CBUF_AVAIL(ibuf) == 0);
	while ((*isa->read_io_8)(mydata->lsr) & LSR_data_ready) {	/* now for each char */
		*s++ = (*isa->read_io_8)(mydata->rbr);					/* into temp buffer */
		scount += 1;								/* count them */
		if (scount > sizeof(tb))					/* a little paranoia */
			break;
	}
#if 0
	if(mydata->tctl.c_iflag & IXON) {			/* do we have SW flow? */
		if ((c == XOFF)) {						/* yes, was it XOFF? */
			mydata->stopped |= OUTPUT_STOPPED;	/* yes, stop output */
			/* return;*/ continue;							
		}
		/* wasn't XOFF. Are we stopped, && is any flow OK, or is it XON? */
		if ((mydata->stopped & OUTPUT_STOPPED) &&
			((mydata->tctl.c_iflag & IXANY) || (c == XON))) {
			mydata->stopped &= ~OUTPUT_STOPPED;	/* restart output */
			ser_thre_int(mydata);
			return 0;
		}
	}											/* end of do we have SW */
#endif
	n = min(CBUF_FREE(ibuf), scount);
	CBUF_PUTN(ibuf, (char*)tb, n);
	cbuf_unlock(ibuf, ps);
#if PRINT_STATISTICS
	if (n < scount)
		stats[mydata->portno].dropped_chars += (scount - n);
	stats[mydata->portno].recv_chars += scount;
#endif
	if (was_mt)
		release_sem_etc(mydata->ser_rdsem, 1, B_DO_NOT_RESCHEDULE);
	return 0;
}

static
ser_lsr_int(struct serial_drvr_data *mydata)
{
	unsigned char lsr, dummy;
	long		scount;

#if PRINT_STATISTICS
	stats[mydata->portno].lsr_ints += 1;
#endif
	lsr = (*isa->read_io_8)(mydata->lsr);
	if (lsr & LSR_data_ready)				/* remove errant data byte */
		dummy = (*isa->read_io_8)(mydata->rbr);
	get_sem_count(mydata->ser_eventsem, &scount);
	if((lsr & LSR_break) && (scount < 0) && !(mydata->send_break) ) {
		mydata->event = EV_BREAK;
		release_sem(mydata->ser_eventsem);
	}
#if PRINT_STATISTICS
	if (lsr & LSR_overrun)
		stats[mydata->portno].overruns += 1;
	if (lsr & LSR_parity)
		stats[mydata->portno].parity_errors += 1;
	if (lsr & LSR_framing)
		stats[mydata->portno].framing_errors += 1;
	if (lsr & LSR_break)
		stats[mydata->portno].break_interrupts += 1;
#endif
	return((int)dummy);
}

static
ser_msr_int(struct serial_drvr_data *mydata)
{
	cpu_status		ps;
	bool			was_idle;
	cbuf			*obuf = mydata->ser_obuf;
	long		scount;
	unsigned char	msr = (*isa->read_io_8)(mydata->msr);
	unsigned short	cflag = mydata->tctl.c_cflag;
	
#if PRINT_STATISTICS
	stats[mydata->portno].msr_ints += 1;
#endif
	
	if ( (msr & MSR_DCTS) && (cflag & CTSFLOW) ){	/* Delta CTS, CTS changed */
		ps = cbuf_lock(obuf);
		if (msr & MSR_CTS) {						/* delta CTS, where is CTS? */
#if PRINT_STATISTICS
			stats[mydata->portno].flow_starts += 1;	/* yes, restart output */
#endif
			mydata->stopped &= ~OUTPUT_STOPPED;		/* restart output */
			was_idle = mydata->ser_tidle;
			cbuf_unlock(obuf, ps);
			/*if (was_idle)
			  ser_thre_int(mydata);*/
			interrupt_if_idle(mydata);
			return 0;
		} else {
			/*  CTS is clear.  must stop output. */
#if PRINT_STATISTICS
			stats[mydata->portno].flow_stops += 1;
#endif
			mydata->stopped |= OUTPUT_STOPPED;
			cbuf_unlock(obuf, ps);
			return 0;
		}
	}
#if 0
	get_sem_count(mydata->ser_eventsem, &scount);
	if((msr & MSR_TERI) && (scount < 0)) {
		mydata->event = EV_RING;
		release_sem(mydata->ser_eventsem);
	}
	get_sem_count(mydata->ser_eventsem, &scount);
	if((msr & MSR_DDCD) && (scount < 0)) {
		mydata->event = (msr & MSR_DCD) ? EV_CARRIER : EV_CARRIERLOST;
		release_sem(mydata->ser_eventsem);
	}
#endif
	return 0;
}

static
ser_thre_int(struct serial_drvr_data *mydata)
{
	bool 				was_full;
	int					i;
	unsigned char		c[16], *s;
	cbuf				*obuf;
	cpu_status			ps;

	obuf = mydata->ser_obuf;

	ps = cbuf_lock(obuf);
	if (((*isa->read_io_8)(mydata->lsr) & LSR_thre) == 0) {
		cbuf_unlock(obuf, ps);
		return 0;
	}

#if PRINT_STATISTICS
	stats[mydata->portno].thre_ints += 1;
#endif

	if (CBUF_MT(obuf)) {    				 /* no more characters to send */
		mydata->ser_tidle = 1;
		cbuf_unlock(obuf, ps);
		return 0;
	}
#if 0
	if(mydata->deferred_set) {
		if(mydata->deferred_set == SETAF)
			cbuf_flush(mydata->ser_ibuf);
		mydata->deferred_set = 0;
		_set(mydata);
	}
	/*
	 * Send break.  To do this, load the thr with all zeros and assert the
	 *  break bit in lcr.  When this character is done, we'll get another
	 *  thre interrupt.  Clear the break bit in lcr when this happens and
	 *  continue merrily along our way.
	 */
	if(mydata->send_break == 1) {
		(*isa->write_io_8)(mydata->lcr, (*isa->read_io_8)(mydata->lcr) | LCR_break_enable);
		(*isa->write_io_8)(mydata->thr, 0);
		mydata->send_break = 2;
	}
#endif
	
	if (mydata->stopped & OUTPUT_STOPPED) {		/* is output stopped? */
		mydata->ser_tidle = 1;
		cbuf_unlock(obuf, ps);
		return 0;									/* yes, just return */
	}

	/* otherwise, get a character, send, and signal if anybody interested */
	/* continue to do this until the FIFO fills, i.e. until thre goes away */
	
	i = min(16, CBUF_AVAIL(obuf));
	was_full = (CBUF_FREE(obuf) == 0);
	CBUF_GETN(obuf, (char *)c, i);
	mydata->ser_tidle = 0;

	s = c;
#if PRINT_STATISTICS
	stats[mydata->portno].xmit_chars += i;
#endif

	while (i-- > 0) {
		(*isa->write_io_8)(mydata->thr, *s);
		s++;
	}

	cbuf_unlock(obuf, ps);
	if (was_full)
		release_sem_etc(mydata->ser_wrsem, 1, B_DO_NOT_RESCHEDULE);
	return 0;
}

/* -----
	driver-related structures
----- */

static device_hooks serial_device =  {
	ser_open, 			/* -> open entry point */
	ser_close, 			/* -> close entry point */
	ser_free,			/* -> free entry point */
	ser_control, 		/* -> control entry point */
	ser_read,			/* -> read entry point */
	ser_write,			/* -> write entry point */
	NULL,				/* -> select entry point */
	NULL				/* -> deselect entry point */
};


const char **
publish_devices()
{
	return serial_name;
}

device_hooks *
find_device(const char *name)
{
	return &serial_device;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
