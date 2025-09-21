#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <ISA.h>
#include <kernel/OS.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include <parallel_driver.h>
#include "parallel.h"
#include "ieee1284.h"


/* ===============================*/
/* Wait 35ms for a given status   */
/* ===============================*/

bool wait_for_ieee1284_status(par_port_info *d, const uint8 mask, const uint8 val)
{
	const int io_stat = d->io_base + PSTAT;
	int	i;

	/* 5ms busy-waiting */
	for (i=0 ; i<100 ; i++)
	{
		if ((read_io_8(io_stat) & mask) == val)
			return true;
		spin(50);
	}

	/* This seems to be a slow device, wait 30 ms */	
	for (i=0 ; i<3 ; i++)
	{
		snooze(10000);
		if ((read_io_8(io_stat) & mask) == val)
			return true;
	}

	return false;
}

/* ====================*/
/* Reset the interface */
/* ====================*/
 
status_t reset_ieee1284(par_port_info *d)
{
	int	io_base = d->io_base;
	write_io_8(io_base + PCON, C_SELIN | C_INIT);
	spin(5000);
	terminate_ieee1284(d);
	return B_OK;
}


/* ==============================
 *  Negotiate IEEE-P1284  mode
 *
 * Input  : Extensibility Request
 * Output : B_OK or EIO
 * ============================== */
 
status_t negotiate_ieee1284(par_port_info *d, int ext_byte)
{
	int		io_base = d->io_base;
	uint8	stat;

	D(bug("parallel: negotiate_ieee1284: %02X -> %02X\n", d->ieee1284_mode, ext_byte));

	/* Check that we are not already in the requested mode */
	// In this test, don't mask ext_byte with 0x04
	// because we won't be able to get the evice ID twice in a row
	// if we do that [will return B_OK without doing anything]
	if (d->ieee1284_mode == ext_byte)
		return B_OK;
	
	/* If the requested mode is different, go to compatibility mode */
	if (d->ieee1284_mode != IEEE1284_COMPATIBILITY)
		terminate_ieee1284(d);

	/* Compatibility mode: no negotiation. */
	if (ext_byte == IEEE1284_COMPATIBILITY)
		return B_OK;

	/* Initiate IEEE 1284 negotiation */
	write_io_8(io_base + PDATA, ext_byte);
	spin(1);
	write_io_8(io_base + PCON, C_INIT | C_AUTOFD);

	/* Wait 35ms for acknowledge */
	if (!wait_for_ieee1284_status(d,	S_PERR | S_SEL | S_FAULT | S_ACK,
										S_PERR | S_SEL | S_FAULT))
	{
		/* Not IEEE-1284 compliant */
		DD(bug("parallel: Not IEEE-1284 compliant\n"));
		reset_ieee1284(d);
		return EIO;
	}

	/* Send strobe */
	write_io_8(io_base + PCON, C_INIT | C_AUTOFD | C_STROBE);
	spin(1);

	/* Tell the device we recognize this is a IEEE 1284 device */
	write_io_8(io_base + PCON, C_INIT);

	/* Wait for ack high -> the sequence is over */
	if (!wait_ack_high(d))
	{
		/* Unusual error */
		terminate_ieee1284(d);
		DD(bug("parallel: Wait for ack high Unusual error\n"));
		return EIO;
	}

	/*
	** Requested mode supported?
	** WARNING: For Nibble Mode, S_SEL is set to LOW to indicate
	** that the mode is availlable (see IEEE-1284)
	*/

	stat = read_io_8(io_base + PSTAT);
	if (ext_byte == 0x00)	/* Nibble mode? -> reverse S_SEL bit */
		stat ^= S_SEL;

	if ((stat & S_SEL) == 0)
	{
		/* Requested mode not supported */
		terminate_ieee1284(d);
		DD(bug("parallel: Requested mode not supported\n"));
		return EIO;
	}

	d->ieee1284_mode = (ext_byte & ~0x04);
	D(bug("parallel: negotiate_ieee1284: mode %02X set!\n", d->ieee1284_mode));

	/* Special case for ECP */
	if (ext_byte & 0x30)
	{
		/* We requested ECP mode */

		/* Event 30: Set nAutoFd low */
		write_io_8(io_base + PCON, C_INIT | C_AUTOFD);
	
		/* Event 31: PError goes high. */
		if (!wait_for_ieee1284_status(d, S_PERR, S_PERR))
		{
			/* Not IEEE-1284 compliant */
			terminate_ieee1284(d);
			return EIO;
		}
	}

	return B_OK;
}

/* ===================================================
** This ensure at the end of this call that the driver 
** is in compatibilty (forward) mode.
** =================================================== */

status_t terminate_ieee1284(par_port_info *d)
{
	int	io_base = d->io_base;
	DD(bug("parallel: Terminate IEEE 1284\n"));

	d->ieee1284_mode = IEEE1284_COMPATIBILITY;
	
	/* nSelectIn low, nAutoFd high */
	write_io_8(io_base + PCON, C_SELIN | C_INIT);

	/* Wait 35ms for acknowledge : nAck low */
	if (!wait_for_ieee1284_status(d, S_ACK, 0))
	{	/* Not IEEE-1284 compliant */
		return EIO;
	}

	/* nAutoFd low */
	write_io_8(io_base + PCON, C_AUTOFD | C_SELIN | C_INIT);

	/* Wait 35ms for acknowledge : nAck high */
	if (!wait_for_ieee1284_status(d, S_ACK, S_ACK))
	{	/* Not IEEE-1284 compliant */
		return EIO;
	}

	/* nAutoFd high */
	write_io_8(io_base + PCON, C_SELIN | C_INIT);
	return B_OK;
}

