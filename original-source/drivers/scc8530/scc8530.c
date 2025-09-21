
/*
 * scc8530.c
 *
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 *
 * Driver for AMD SCC8530 serial chip.
 */

#include <Debug.h>
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <interrupts.h>
#include <Errors.h>

#include <gcentral.h>
#include <mapio.h>
#include <scc8530.h>
#include <dbdma.h>
#include <cbuf.h>
#include "mac_tnt.h"

#include <termios.h>

#define NSERPORTS 2
#define SCC_CHANNEL_A	0
#define SCC_CHANNEL_B	1

/*
 * These should be powers of 2
 */
#define OUTPUT_BUFFER_SIZE	1024
#define INPUT_BUFFER_SIZE	2048

#define HIGH_WATER			256			/* bytes free to stop  */
#define LOW_WATER			1024 		/* bytes free to restart */

#define	OUTPUT_STOPPED		(0x01)			/* bits in the stopped map */
#define INPUT_STOPPED		(0x02)

/*
 * Test to see if another thread is trying to close us
 */
#define IS_CLOSING(x) ((x)->rdsem == -1)


#ifndef B230400
#define B230400	(B115200 + 1)
#endif

typedef struct scc_channel_state {
	/* -- BeOS stuff -- */
	cbuf			*ibuf;
	cbuf			*obuf;
	sem_id			rdsem;
	sem_id			wrsem;
	sem_id			eventsem;
	long			ser_xmit_idle;
	sem_id			closesem;
	unsigned long	int_id;
	unsigned long	channel;
	bool			initialized;
	
	/* -- tty stuff -- */
	struct termios	tctl;
	unsigned long	stopped;
	bool			full_modem;
	unsigned long	cts;
	unsigned long	dcd;
	
	/* -- 8530 specific stuff -- */
	volatile char *	channel_cmd;
	volatile char *	channel_data;
	uchar			cur_regs[16];
} scc_channel_state;

static long channel_open[NSERPORTS] = { 0, 0 };

static scc_channel_state scc_channel_data[2];
static scc_channel_state *cs_a = &scc_channel_data[SCC_CHANNEL_A];
static scc_channel_state *cs_b = &scc_channel_data[SCC_CHANNEL_B];

static void wrcmd (scc_channel_state *cs, int reg, uchar val);
static uchar rdcmd (scc_channel_state *cs, int reg);
static void wrdata (scc_channel_state *cs, uchar val);
static uchar rddata (scc_channel_state *cs);
static void send_break (scc_channel_state *cs, int set);
static void interrupt_if_idle (scc_channel_state *cs);
static void flush_onchip_fifo (scc_channel_state *cs);

typedef struct scc_regval {
	/*
	 * See p. 7-5 of spec
	 */
	int		reg;
	uchar	val;
} scc_regval;

extern char *serial_name[];

/*
 * If you mess with these, most probably the driver will
 * not work anymore.
 */
static scc_regval scc_init_tbl[] = {
	/* program the intr vector */
	{ SCC_W2,	0x18 },			/* irrelevant -- we don't use this */
	{ SCC_W9,	SCC_W9_vector_includes_status },

	/* timing base defaults */
	{ SCC_W4, 	SCC_W4_clock_x16 | SCC_W4_stop_bits_1 },

	/* enable dtr, rts and ss */
	{ SCC_W5,	SCC_W5_dtr | SCC_W5_rts },

	/* baud rates */
	{ SCC_W14, 	SCC_W14_brg_enable },

	/* interrupt conditions */
	{ SCC_W1,	SCC_W1_rx_int_all_special | /*SCC_W1_parity_is_special |*/
   	  SCC_W1_ext_status_int_enable | SCC_W1_tx_int_enable },

};


/*
 * BRG formula is:
 *				          ClockFrequency
 *	BRGconstant = 	---------------------------  -  2
 *			        2 * BaudRate * ClockDivider
 */
/*
 * Speed selections with clock x16
 *
 * The Apple ERS says that the ClockFrequency is 3.672 MHz.
 */

static short scc_speeds[] = {
	0,							/* 0 	*/
	2293,						/* 50 	*/
	1528,						/* 75 	*/
	1041,						/* 110 	*/
	854,						/* 134.5 */
	763,						/* 150 	*/
	572,						/* 200 	*/
	381,						/* 300 	*/
	189,						/* 600 	*/
	94,						/* 1200 */
	62,						/* 1800 */
	46,						/* 2400 */
	22,						/* 4800 */
	10,						/* 9600 */
	4,						/* 19200 */
	1,						/* 38400 */
	0,						/* 57600 */
};


static volatile char	*mac_io;		/* -> mac i/o chip */

static struct drvr_init {
	volatile char *	channel_cmd;
	volatile char *	channel_data;
	uchar			reset_cmd;
	unsigned long	int_id;
} drvr_init_tbl[2];



static void
scc_channel_init (scc_channel_state *cs, uchar reset_cmd)
{
	int	i;
	int reg;
	uchar val;

	if (!cs->initialized) {
		/*
		 * ensure reg pointer at zero (7.1.1.4) 
		 * and reset appropriate channel. 
		 */	
		rdcmd (cs, 0);
		wrcmd (cs, SCC_W9, reset_cmd);
		snooze(50);
		cs->initialized = 1;
	}
	flush_onchip_fifo (cs);
	
	for (i = 0; i < sizeof (scc_init_tbl)/sizeof(scc_init_tbl[0]); i++) {
		reg = scc_init_tbl[i].reg;
		val = scc_init_tbl[i].val;
		cs->cur_regs[reg] = val;
		wrcmd (cs, reg, val);
	}
}


static int
scc_set_params (scc_channel_state *cs)
{
	uchar val;
	int bps;
	struct termios *t = &cs->tctl;
	cpu_status ps;
	
	bps = t->c_cflag & CBAUD;
	
	if (bps == B0  ||  (bps > B230400  &&  bps != B31250)) {
		send_break (cs, TRUE);
		return 0;
	}

	ps = cbuf_lock (cs->ibuf);

#if 0
	val = drvr_init_tbl[cs->channel].reset_cmd;
	wrcmd (cs_a, SCC_W9, val);
	snooze (25);
#endif
	
	/* stop bits, normally 1 */
	val = ((t->c_cflag & CSTOPB) ? SCC_W4_stop_bits_2 : SCC_W4_stop_bits_1);

	/* parity */
	if (t->c_cflag & PARENB) {
		if ((t->c_cflag & PARODD) == 0) {
			val |= SCC_W4_parity_even;
		}
		val |= SCC_W4_parity_enable;
	}

	/* set it now, must be first after reset */
	if (bps == B31250) {
		val |= SCC_W4_clock_x32;
	} else if (bps == B115200) {
		val |= SCC_W4_clock_x32;
	} else {
		val |= SCC_W4_clock_x16;
	} 
	wrcmd (cs, SCC_W4, val);
	
	/* vector again */
	wrcmd (cs, SCC_W2, 0x18);

	/*
	 * clear break, keep rts dtr
	 */
	cs->cur_regs[5] &= (SCC_W5_dtr | SCC_W5_rts);

	/* bits per char */
	if ((t->c_cflag & CSIZE) == CS8) {
		cs->cur_regs[3] = SCC_W3_rx_bits_per_char_8;
		cs->cur_regs[5] |= SCC_W5_tx_bits_per_char_8;
	} else {
		cs->cur_regs[3] = SCC_W3_rx_bits_per_char_7;
		cs->cur_regs[5] |= SCC_W5_tx_bits_per_char_7;
	}
	wrcmd (cs, SCC_W3, cs->cur_regs[3]);
	wrcmd (cs, SCC_W5, cs->cur_regs[5]);

	wrcmd (cs, SCC_W6, 0);
	wrcmd (cs, SCC_W7, 0);
/*	wrcmd (cs_a, SCC_W9, SCC_W9_no_vector);*/
	wrcmd (cs, SCC_W9, SCC_W9_vector_includes_status);

	wrcmd (cs, SCC_W10, 0);

	/* clock config.
	 * For speeds upto and including 56 Kbps, we can use the internal baud
	 * rate generator (BRG).  For faster speeds, we use the RTxC pin which
	 * is tied to the PCLK (3.672 MHz) and use the appropriate divider.
	 * x32 for 115.2 Kbps, x16 for 230.4 Kbps. */
	if (bps == B31250) {
		/* for Midi only */
		val = SCC_W11_rx_clock_trxc;
		val |= SCC_W11_tx_clock_trxc;
		wrcmd (cs, SCC_W11, val);
		wrcmd (cs, SCC_W12, 0);
		wrcmd (cs, SCC_W13, 0);
		cs->full_modem = 0;
	} else if (bps >= B115200) {
		val = SCC_W11_rx_clock_rtxc;
		val |= SCC_W11_tx_clock_rtxc;
		wrcmd (cs, SCC_W11, val);
		wrcmd (cs, SCC_W12, 0);
		wrcmd (cs, SCC_W13, 0);
	} else {
		val = SCC_W11_rx_clock_brg;
		val |= SCC_W11_tx_clock_brg;
		wrcmd (cs, SCC_W11, val);
		wrcmd (cs, SCC_W12, (uchar) (scc_speeds[bps] & 0xff));
		wrcmd (cs, SCC_W13, (uchar) (scc_speeds[bps] >> 8));
	}

	val = cs->cur_regs[14];
	wrcmd (cs, SCC_W14, val);

	if (cs->full_modem) 
		val = SCC_W15_break_abort_int_enable | SCC_W15_cts_int_enable | SCC_W15_dcd_int_enable;
	else
		val = SCC_W15_break_abort_int_enable;
	wrcmd (cs, SCC_W15, val);

	/* reset ext status int twice */
	wrcmd (cs, 0, SCC_W0_cmd_reset_ext_status);
	wrcmd (cs, 0, SCC_W0_cmd_reset_ext_status);

    /* now the enables */
	cs->cur_regs[3] |= SCC_W3_rx_enable;
	wrcmd (cs, SCC_W3, cs->cur_regs[3]);

	cs->cur_regs[5] |= SCC_W5_tx_enable;
	wrcmd (cs, SCC_W5, cs->cur_regs[5]);

	/* master intr enable */
	wrcmd (cs, SCC_W1, cs->cur_regs[1]);
	wrcmd (cs, SCC_W9, SCC_W9_master_int_enable | SCC_W9_vector_includes_status);

	cbuf_unlock (cs->ibuf, ps);
/*	dprintf ("finished setting params\n");*/
	
	return 0;
}



/*
 * This is called from the receive interrupt handler.  As characters are
 * coming in, we must decide if we'd like to throttle the sender, and if
 * so whether we're allowed to given the state of both hardware or software
 * flow control having been enabled. This is called with the ibuf spinlock
 * already obtained.
 */

static void
evaluate_stopping_input (scc_channel_state *cs)
{
	uchar val;

	if (CBUF_FREE(cs->ibuf) >= HIGH_WATER)			/* no crisis! */
		return;

	if (cs->stopped & INPUT_STOPPED)				/* crisis, knew it! */
		return;

	if (cs->tctl.c_cflag & RTSFLOW) {				/* is HW flow on? */
		cs->stopped |= INPUT_STOPPED;				/* note it */
		val = cs->cur_regs[SCC_W5] & ~SCC_W5_rts;
		cs->cur_regs[SCC_W5] = val;
		wrcmd (cs, SCC_W5, val);					/* Drop RTS - we're behind. */
	}

	if (cs->tctl.c_cflag & IXON) {					/* This is SW Flow */
			/* Send an XOFF! */
	}
}

/*
 * test for re-enabling input. Done when a character is removed from cbuf,
 * perhaps freeing up space above the high water mark.
 */

static void
reenable_rts (scc_channel_state *cs)
{
	cpu_status	ps;
	uchar		val;

	ps = cbuf_lock (cs->ibuf);
	if ((cs->tctl.c_cflag & RTSFLOW)  && 		/* do we have HW Flow? */
		(cs->stopped & INPUT_STOPPED)  && 		/* is input currently stopped? */
		CBUF_FREE (cs->ibuf) >= LOW_WATER) {	/* and is there now room? */ 
		cs->stopped &= ~INPUT_STOPPED;			/* unstopping input */
		val = cs->cur_regs[SCC_W5] | SCC_W5_rts;
		wrcmd (cs, SCC_W5, val);	 			/* then assert RTS */
	}
	cbuf_unlock (cs->ibuf, ps);
}


static void
set_rts (scc_channel_state *cs, bool state)
{
	uchar val = cs->cur_regs[5];
	if (state)
		val |= SCC_W5_rts;
	else
		val &= ~SCC_W5_rts;
	cs->cur_regs[5] = val;
	wrcmd (cs, SCC_W5, val);
}


static void
set_dtr (scc_channel_state *cs, bool state)
{
	uchar val = cs->cur_regs[5];
	if (state)
		val |= SCC_W5_dtr;
	else
		val &= ~SCC_W5_dtr;
	cs->cur_regs[5] = val;
	wrcmd (cs, SCC_W5, val);
}


static void
rx_intr (scc_channel_state *cs)
{
	uchar 		rr1, rr0;
	uchar 		c;
	cpu_status	ps;
	cbuf		*ibuf;
	uchar		val;
	bool		was_mt;
	uchar		tempbuf[32];
	ulong		count = 0;
	uchar		*s = &tempbuf[0];
	int			n;


	ibuf = cs->ibuf;
	ps = cbuf_lock (ibuf);
	evaluate_stopping_input (cs);

	was_mt = (CBUF_AVAIL (ibuf) == 0);
	
	while (1) {
		rr1 = rdcmd (cs, 1);
		c = rddata (cs);

		/*
		 * First check for recv errors and clear them
		 */
		if (rr1 & (SCC_R1_crc_framing_error | 
				   SCC_R1_rx_overrun_error |
				   SCC_R1_parity_error)) {
			wrcmd (cs, SCC_W0, SCC_W0_cmd_error_reset);
		}

/*		dprintf ("read 0x%x (%c)\n", (int) c, c);*/
		*s++ = c;				/* into temp buffer */
		count++;
		if (count > sizeof(tempbuf))
			break;

		rr0 = rdcmd (cs, 0);
		if (!(rr0 & SCC_R0_rx_char_available))
			break;
	}

	n = min (CBUF_FREE (ibuf), count);
	CBUF_PUTN (ibuf, (char *)tempbuf, n);
	cbuf_unlock (ibuf, ps);
	if (was_mt) {
 		release_sem_etc (cs->rdsem, 1, B_DO_NOT_RESCHEDULE);
	}
}	
	

static void
tx_intr (scc_channel_state *cs)
{
	cbuf		*obuf;
	cpu_status	ps;
	uchar		ch;
	bool		was_full;
	int			i;
	uchar		rr0;
	
	/* XXX Maybe do baudrate changes first? */

	obuf = cs->obuf;
	if (obuf == NULL) {   /* this should never happen but just in case... */
		return;
	}

	ps = cbuf_lock (obuf);
	if (CBUF_MT (obuf)) {		/* no more chars to send */
		wrcmd (cs, 0, SCC_W0_cmd_reset_tx_int);
		atomic_or (&cs->ser_xmit_idle, 1);

		cbuf_unlock (obuf, ps);
		return;
	}

	if (cs->stopped & OUTPUT_STOPPED) {
		wrcmd (cs, 0, SCC_W0_cmd_reset_tx_int);
		atomic_or (&cs->ser_xmit_idle, 1);

		cbuf_unlock (obuf, ps);
		return;
	}

	was_full = (CBUF_FREE (obuf) == 0);
	i = CBUF_AVAIL (obuf);
	if (i) {
		CBUF_GETN (obuf, (char *)&ch, 1);
		atomic_and (&cs->ser_xmit_idle, 0);
	}
	cbuf_unlock (obuf, ps);

	if (was_full)
		release_sem_etc (cs->wrsem, 1, B_DO_NOT_RESCHEDULE);

	if (i) {
/*		dprintf ("writing %c (0x%x)\n", ch, ch);*/
		wrdata (cs, ch);
	}
}

	
static void
status_intr (scc_channel_state *cs)
{
	uchar 		rr0;
	ushort		cflag = cs->tctl.c_cflag;
	cpu_status	ps;
	cbuf		*obuf = cs->obuf;
	
/*	dprintf ("got status intr\n");*/

	rr0 = rdcmd (cs, 0);

	cs->dcd = (rr0 & SCC_R0_dcd);
	cs->cts = !(rr0 & SCC_R0_cts);
	wrcmd (cs, 0, SCC_W0_cmd_reset_ext_status);

	if (cflag & CTSFLOW) {
		ps = cbuf_lock (obuf);
		if (cs->cts) {
			/* restart output */
			cs->stopped &= ~OUTPUT_STOPPED;
			cbuf_unlock (obuf, ps);

			interrupt_if_idle (cs);
		} else {
			/* cts is clear; stop output */
			cs->stopped |= OUTPUT_STOPPED;
			cbuf_unlock (obuf, ps);
		}
	}

	return;
}


static void
interrupt_if_idle (scc_channel_state *cs)
{
	if (atomic_or (&cs->ser_xmit_idle, 0)) {	/* if it is idle, prime pump */
		tx_intr (cs);						/* simulate interrupt */
	}
}

/*
 * this is the serial port write routine.  It will send each character
 * until the output buffer is full, and then wait on the semaphore to
 * keep the output buffer full.
 */
static status_t
serial_write(void *cookie, off_t pos, const void *buf, size_t *len)
{
	int						sent = 0, err, avail, n;
	bool					is_full;
	uchar					*s = (uchar *)buf;
	size_t 					count = *len;
	ulong					bytes_left = count;
	scc_channel_state 		*cs = (scc_channel_state *)cookie;
	cbuf					*obuf;
	cpu_status				ps;

	obuf = cs->obuf;

	lock_memory ((char *) buf, count, 0);
	while (bytes_left > 0) {
		err = acquire_sem_etc (cs->wrsem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_NO_ERROR) {
			unlock_memory ((char *) buf, count, 0);
			*len = count - bytes_left;
			return err;
		}

		ps = cbuf_lock (obuf);				/* put stuff in outbuf */
		avail = CBUF_FREE (obuf);			/* how much room is there? */
		n = min (avail, bytes_left);		/* how much should we put? */
		CBUF_PUTN (obuf, (char *)s, n);		/* put it in all at once */
		is_full = (CBUF_FREE(obuf) == 0);	/* are we now full? */
		cbuf_unlock (obuf, ps);				/* unlock it */
		if (!is_full)						/* If it wasn't full, */
			release_sem (cs->wrsem); 		/* we release for next time */
		s += n;	bytes_left -= n; sent += n;	/* buf, count, amount sent */
		interrupt_if_idle (cs);				/* if xmitter idle, start it */
	}
	unlock_memory ((char *) buf, count, 0);
	return sent;								/* return sent characters */
}

static status_t
serial_intr (void *data)
{
	uchar rr3, rr3a;

	rr3a = 0;

	/* Note: only channel A has an RR3 */
	while ((rr3 = rdcmd (cs_a, 3))) {
		/* Handle receive interrupts first. */
		if (rr3 & SCC_R3_rx_int_a)
			rx_intr (cs_a);
		if (rr3 & SCC_R3_rx_int_b)
			rx_intr (cs_b);

		/* Handle status interrupts (i.e. flow control). */
		if ((rr3 & SCC_R3_ext_stat_int_a) && channel_open[0]) {
			status_intr (cs_a);
		}

		if ((rr3 & SCC_R3_ext_stat_int_b) && channel_open[1]) {
			status_intr (cs_b);
		}

		/* Handle transmit done interrupts. */
		if ((rr3 & SCC_R3_tx_int_a) && channel_open[0]) {
			tx_intr (cs_a);
		}
		if ((rr3 & SCC_R3_tx_int_b) && channel_open[1]) {
			tx_intr (cs_b);
		}

		rr3a |= rr3;
	}

	/* Clear interrupt. */
	if (rr3a & (SCC_R3_tx_int_a | SCC_R3_rx_int_a | SCC_R3_ext_stat_int_a)) {
		wrcmd (cs_a, 0, SCC_W0_cmd_reset_highest_ius);
	}
	if (rr3a & (SCC_R3_tx_int_b | SCC_R3_rx_int_b | SCC_R3_ext_stat_int_b)) {
		wrcmd (cs_b, 0, SCC_W0_cmd_reset_highest_ius);
	}
	return (TRUE);
}

static status_t
serial_open(const char *name, uint32 flags, void **cookiep)
{
	const char *dname = name;
	char	channel;
	scc_channel_state *cs;
	scc_channel_state **csp = (scc_channel_state **)cookiep;
	char 	sbuf[32];
	struct drvr_init *drvr_init;
	bool midi_mode_p;

	for (channel = 0; serial_name[channel]; channel++)
	 	if (!strcmp(name, serial_name[channel]))
			break;
	if (serial_name[channel] == NULL)
		return B_ERROR;

	midi_mode_p = (channel >= NSERPORTS);
	channel %= NSERPORTS;

	/* if debugging output is on, don't allow opens of the modem port */
	if (debug_output_enabled() && channel == 0) {
		return B_ERROR;
	}

	if (atomic_add(&channel_open[channel], 1) != 0) {
		atomic_add(&channel_open[channel], -1);
		return B_ERROR;
	}

	cs = &scc_channel_data[channel];
	*csp = (void *)cs;

	memset (cs, 0, sizeof (scc_channel_state));

	cs->ibuf = cbuf_init (INPUT_BUFFER_SIZE);
	cs->obuf = cbuf_init (OUTPUT_BUFFER_SIZE);

	sprintf (sbuf, "%s read sem", &dname[5]);
	cs->rdsem = create_sem (0, sbuf);
	set_sem_owner (cs->rdsem, B_SYSTEM_TEAM);
	sprintf (sbuf, "%s write sem", &dname[5]);
	cs->wrsem = create_sem (1, sbuf);
	set_sem_owner (cs->wrsem, B_SYSTEM_TEAM);
	
	sprintf (sbuf, "%s event sem", &dname[5]);
	cs->eventsem = create_sem (0, sbuf);
	atomic_or (&cs->ser_xmit_idle, 1);
	/* XXX other stuff reqd? */
	sprintf (sbuf, "%s close sem", &dname[5]);
	cs->closesem = create_sem (1, sbuf);

	drvr_init = &drvr_init_tbl[channel];

	cs->int_id = drvr_init->int_id;
	cs->channel = channel;
	
	/*
	 * Setup the default characteristics. Mapping CR->NL, 19200 bps,
	 * 8 data bits, 1 stop, No parity. Canonical input processing.
	 */
	cs->tctl.c_iflag = 0;
	cs->tctl.c_oflag = 0;
	cs->tctl.c_cflag = (midi_mode_p ? B31250 : B19200) | CS8 | CTSFLOW | RTSFLOW;
	cs->tctl.c_lflag = 0;
	cs->tctl.c_cc[VMIN] = 1;
	cs->tctl.c_cc[VTIME] = 0;

	cs->dcd = 1;
	cs->cts = SCC_R0_cts;

	cs->stopped = 0;
	cs->full_modem = !midi_mode_p;
	
	cs_a->channel_cmd = drvr_init_tbl[SCC_CHANNEL_A].channel_cmd;
	cs_b->channel_cmd = drvr_init_tbl[SCC_CHANNEL_B].channel_cmd;
	cs_a->channel_data = drvr_init_tbl[SCC_CHANNEL_A].channel_data;
	cs_b->channel_data = drvr_init_tbl[SCC_CHANNEL_B].channel_data;
	
	scc_channel_init (cs, drvr_init_tbl[cs->channel].reset_cmd);
	scc_set_params (cs);

/*	dprintf ("enabling interrupts\n");*/

	install_io_interrupt_handler (drvr_init->int_id, serial_intr, NULL, 0);

/*	dprintf ("done enabling interrupts\n");*/

/*	dprintf ("leaving open routine\n");*/
	
	return B_NO_ERROR;
}

static status_t
serial_read_guts (scc_channel_state *cs, off_t pos, uchar *buf, size_t *len)
{
	cbuf				*ibuf;
	unsigned short		lflag, iflag;
	uchar				vmin, vtime;
	cpu_status			ps;
	long				n;
	status_t			err;
	long				i = 0;
	bool				is_now_mt;
	bigtime_t			timeout;
	size_t				count = *len;

	if (IS_CLOSING (cs)) {
		*len = 0;
		return B_INTERRUPTED;
	}

	lflag = cs->tctl.c_lflag;
	iflag = cs->tctl.c_iflag;
	ibuf = cs->ibuf;

	vmin = cs->tctl.c_cc[VMIN];
	vtime = cs->tctl.c_cc[VTIME];

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
	if (vmin == 0) {
		ps = cbuf_lock (ibuf);
		n = min (CBUF_AVAIL (ibuf), count);
		if (n > 0) 
			CBUF_GETN (ibuf, (char *)buf, n);
		cbuf_unlock (ibuf, ps);
		reenable_rts (cs);

		*len = n;
		return B_OK;
	}

	/* The next variant is:
		- VMIN > 0, VTIME = 0:
			This will only return the read has been satisfied, otherwise
			blocking forever.
	*/
	if (vmin > 0  &&  vtime == 0) {
		while (count > 0) {
			reenable_rts (cs);
			err = acquire_sem_etc (cs->rdsem, 1, B_CAN_INTERRUPT, 0);
			if (err) {
				*len = i;
				return err;
			}
			ps = cbuf_lock (ibuf);
			n = min (CBUF_AVAIL (ibuf), count);
			if (n)
				CBUF_GETN (ibuf, (char *)buf, n);
			is_now_mt = (CBUF_AVAIL (ibuf) == 0);
			cbuf_unlock (ibuf, ps);
			if (!is_now_mt) 
				release_sem (cs->rdsem);

			i += n;
			buf += n; 
			count -= n;
		}
		
		*len = i;
		return B_OK;
	}

	/* The last variant is:
		- VMIN > 0, VTIME > 0:
			This version will block, but only for VTIME time before
			giving up.  At that point, the number of characters received
			is returned.
	*/
	if (vmin > 0  &&  vtime > 0) {
		timeout = (bigtime_t) vtime * 100000;
		reenable_rts (cs);
		err = acquire_sem_etc (cs->rdsem, 1, B_CAN_INTERRUPT | B_TIMEOUT,
							   timeout);
		if (err && (err != B_TIMED_OUT)) {
			*len = 0;
			return err;
		}

		ps = cbuf_lock (ibuf);
		n = min (CBUF_AVAIL (ibuf), count);
		if (n)
			CBUF_GETN (ibuf, (char *)buf, n);
		is_now_mt = (CBUF_AVAIL (ibuf) == 0);
		cbuf_unlock (ibuf, ps);
		if (!is_now_mt)
			release_sem (cs->rdsem);
		*len = n;
		return B_OK;
	}

	/* should never get here */
	return B_ERROR;
}


static status_t
serial_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	status_t err;
	status_t ret;
	size_t count = *len;
	scc_channel_state *cs = (scc_channel_state *)cookie;

	err = acquire_sem_etc (cs->closesem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_NO_ERROR)
		return err;

	lock_memory ((char *) buf, count, B_READ_DEVICE);
	ret = serial_read_guts(cs, pos, (uchar *)buf, len);
	unlock_memory ((char *) buf, count, B_READ_DEVICE);

	release_sem (cs->closesem);

	return ret;
}


static void
send_break (scc_channel_state *cs, int set)
{
	if (set) {
		cs->cur_regs[SCC_W5] |= SCC_W5_send_break;
	} else {
		cs->cur_regs[SCC_W5] &= ~SCC_W5_send_break;
	}
	wrcmd (cs, SCC_W5, cs->cur_regs[SCC_W5]);
}


static void
flush_onchip_fifo (scc_channel_state *cs)
{
	uchar rr0, rr1, c;

	while (1) {
		rr0 = rdcmd (cs, 0);
		if ((rr0 & SCC_R0_rx_char_available) == 0)
			break;

		/*
		 * First read the status, because reading the data
		 * destroys the status of this char.
		 */
		rr1 = rdcmd (cs, 1);
		c = rddata (cs);

		if (rr1 & (SCC_R1_parity_error |
				   SCC_R1_rx_overrun_error |
				   SCC_R1_crc_framing_error)) {
			wrcmd (cs, 0, SCC_W0_cmd_error_reset);
		}
	}
}

/*
 * Return the number of characters ready for reading
 * If blocking, wait up to the timeout before returning
 */
static status_t
waitevent (scc_channel_state *cs)
{
	status_t err;
	bigtime_t timeout;
	long len;
	bigtime_t then;
	bigtime_t now;
	bigtime_t this_timeout;

	err = acquire_sem_etc (cs->closesem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_NO_ERROR) {
		return err;
	}

	if (IS_CLOSING (cs)) {
		len = B_INTERRUPTED;
	} else {
		timeout = ((bigtime_t) cs->tctl.c_cc[VTIME]) * 100000;
		if (timeout == 0) {
			timeout = B_INFINITE_TIMEOUT;
		}
		this_timeout = timeout;
		while ((len = CBUF_AVAIL (cs->ibuf)) == 0) { 
			then = system_time();
			err = acquire_sem_etc (cs->rdsem, 1,
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
				release_sem (cs->rdsem);
				break;
			} 
			if ((len = CBUF_AVAIL (cs->ibuf)) > 0) {
				/*
				 * Allow a read to occur
				 */
				release_sem (cs->rdsem);
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
	release_sem (cs->closesem);
	return len;
}


static long
scc_getbits (scc_channel_state *cs)
{
	unsigned long val = 0;

	if (cs->dcd) {
		val |= TCGB_DCD;
	}
	if (cs->cts) {
		val |= TCGB_CTS;
	}
/*	dprintf ("scc_getbits returning %d\n", val);*/
	return val;
}

static status_t
serial_control(void *cookie, uint32 msg, void *buf, size_t buflen)
{
	scc_channel_state *cs = (scc_channel_state *)cookie;
	int arg = (int) buf;
	int len;

/*	dprintf ("serial ioctl 0x%x\n", msg);*/

	switch (msg) {
	case TCGETA:
/*		dprintf ("ioctl get params\n");*/
		memcpy (buf, &cs->tctl, sizeof (cs->tctl));
		return 0;

	case TCSETA:
/*		dprintf ("ioctl set params\n");*/
		memcpy (&cs->tctl, buf, sizeof (cs->tctl));
		return scc_set_params (cs);

	case TCSETAF:
		/* deferred set with flush */
/*		dprintf ("ioctl deferred set with flush\n");*/
		memcpy (&cs->tctl, buf, sizeof (cs->tctl));
		if (atomic_or (&cs->ser_xmit_idle, 0)) { 
			flush_onchip_fifo (cs);
			cbuf_flush (cs->ibuf);
			scc_set_params (cs);
		}
		return 0;
		
	case TCSETAW:
		/* deferred set, no flush */
/*		dprintf ("ioctl deferred set no flush\n");*/
		memcpy (&cs->tctl, buf, sizeof (cs->tctl));
		if (atomic_or (&cs->ser_xmit_idle, 0)) 
			scc_set_params (cs);
		return 0;
		
	case TCSBRK:
		/* Send a break when the current obuf is mt */
		if (atomic_or (&cs->ser_xmit_idle, 0))
			serial_intr (cs);
		return 0;
		
	case TCFLSH:
		/* flush obuf, ibuf, or both */
		if (arg == 0  ||  arg == 2)
			cbuf_flush (cs->ibuf);
		if (arg == 1  ||  arg == 2)
			cbuf_flush (cs->obuf);
		return 0;

	case TCWAITEVENT:
/*		dprintf ("ioctl waitevent\n");*/
		len = waitevent(cs);
		if (len >= 0 && buf) {
			*(int *)buf = len;
		}
		return (len);

	case TCSETRTS:
/*		dprintf ("%s rts\n", (*(bool *)buf ? "setting" : "clearing"));*/
		set_rts (cs, *(bool *) buf);
		break;
		
	case TCSETDTR:
/*		dprintf ("%s dtr\n", (*(bool *)buf ? "setting" : "clearing"));*/
		set_dtr (cs, *(bool *) buf);
		break;

	case TCGETBITS:
/*		dprintf ("ioctl getbits\n");*/
		*(unsigned *)buf =  scc_getbits (cs);
		break;

	case 'ichr':
		if (*(uint *)buf == 0) {
			cpu_status ps = cbuf_lock( cs->ibuf);
			*(uint *)buf = CBUF_AVAIL( cs->ibuf);
			cbuf_unlock( cs->ibuf, ps);
			return (B_OK);
		}
		return (B_ERROR);

	case TCXONC:
	default:
		return -1;
	}

	return 0;
}


static status_t
serial_close(void *cookie)
{
	/* the real close is done down in serial_free() */
	return 0;
}


/* ----------
	wrcmd writes a value to an scc control register, with the appropriate delays
----- */

static void wrcmd (scc_channel_state *cs, int reg, uchar val)
{
	vchar	*scc_cmd = cs->channel_cmd;

	*scc_cmd = reg;
	__eieio();
	*scc_cmd = val;
	__eieio();
}


	
/* ----------
	rdcmd reads a value from an scc control register, with the appropriate delays
----- */

static uchar rdcmd (scc_channel_state *cs, int reg)
{
	vchar	*scc_cmd = cs->channel_cmd;
	uchar	val;

	*scc_cmd = reg;
	__eieio();
	val = *scc_cmd;
	__eieio();

	return val;
}

/* ----------
	wrdata writes a data value to an scc data register, with the appropriate delays
----- */

static void wrdata (scc_channel_state *cs, uchar val)
{
	*cs->channel_data = val;
	__eieio();
}


	
/* ----------
	rddata reads a data value from an scc data register, with the appropriate delays
----- */

static uchar rddata (scc_channel_state *cs)
{
	uchar val; 

	val = *cs->channel_data;
 	__eieio();
 	return val;
}

static status_t
serial_free(void *cookie)
{
	int counter = 0;
	scc_channel_state *cs = (scc_channel_state *)cookie;

/*	dprintf ("closing serial port\n");*/

	if (!channel_open[cs->channel]) 
		return B_NO_ERROR;

	while (!cbuf_mt (cs->obuf)) {
		if (counter++ > 5000)
			break;
		if (cs->stopped & OUTPUT_STOPPED)
			break;
		snooze (5000);
	}

	wrcmd( cs, SCC_W1, 0);
	wrcmd( cs, SCC_W5, 0);

	remove_io_interrupt_handler (cs->int_id, serial_intr, NULL);

	delete_sem (cs->rdsem);
	delete_sem (cs->wrsem);
	delete_sem (cs->eventsem);

	cs->rdsem = -1;
	acquire_sem (cs->closesem);
	delete_sem (cs->closesem);

	cbuf_delete (cs->obuf);
	cbuf_delete (cs->ibuf);

	atomic_add(&channel_open[cs->channel], -1);  /* free it up again */

	return (B_OK);
}

/*
 * loadable driver stuff
 */

extern device_hooks midi_device;

static char *serial_name[] = {
  "ports/modem", "ports/printer",
  "midi/midi1", "midi/midi2",
  NULL
};

device_hooks serial_device = {
	serial_open,
	serial_close,
	serial_free,
	serial_control,
	serial_read,
	serial_write
};

const char **
publish_devices(void)
{
	return (serial_name);
}

device_hooks *
find_device(const char *name)
{
	return (strncmp(name, "midi", 4) ? &serial_device : &midi_device);
}

status_t
init_driver (void)
{
#if 0
	area_info	a;
	area_id		id;

	id = find_area ("mac_io");
	if (id < 0)
		return B_ERROR;
	
	if (get_area_info (id, &a) < 0)
		return B_ERROR;

	mac_io = a.address;

	return B_NO_ERROR;
#else
	char	*pa[3];					/* phys addrs controller, dma regs */
	char	*va[3];					/* virt addrs controller, dma regs */
	int		intrs[3];				/* interrupt ids */

	dprintf ("scc8530: init_driver");

	/* get address, interrupt id for channel a */
	if (get_mac_device_info ("ch-a", 3, &pa[0], &va[0], 3, &intrs[0]) < 0) {
		dprintf ("No scc8530.  Sorry, rico\n");
		return B_ERROR;
	}

	drvr_init_tbl[SCC_CHANNEL_A].channel_cmd = va[0];
	drvr_init_tbl[SCC_CHANNEL_A].channel_data = va[0] + 0x10;
	drvr_init_tbl[SCC_CHANNEL_A].reset_cmd = SCC_W9_reset_chan_a;
	drvr_init_tbl[SCC_CHANNEL_A].int_id = intrs[0];

	dprintf ("SCC ch-a: regs phys %.8x regs virt %.8x int %d\n",
		pa[0], va[0], intrs[0]);

	/* get address, interrupt id for channel b */
	if (get_mac_device_info ("ch-b", 3, &pa[0], &va[0], 3, &intrs[0]) < 0) {
		dprintf ("No scc8530 ch-b.  Sorry, rico\n");
		return B_ERROR;
	}

	drvr_init_tbl[SCC_CHANNEL_B].channel_cmd = va[0];
	drvr_init_tbl[SCC_CHANNEL_B].channel_data = va[0] + 0x10;
	drvr_init_tbl[SCC_CHANNEL_B].reset_cmd = SCC_W9_reset_chan_b;
	drvr_init_tbl[SCC_CHANNEL_B].int_id = intrs[0];

	dprintf ("SCC ch-b: regs phys %.8x regs virt %.8x int %d\n",
		pa[0], va[0], intrs[0]);

	return B_NO_ERROR;
#endif
}

