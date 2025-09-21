/* ++++++++++
	skel.c

	This is a skeletal device driver for a fictional PCI device, a 'skel'.  
	The driver supports multiple clients, i.e. it can be open more than once.

	Like many real-world devices, this fictional one has a multiprocessor-
	unfriendly memory mapped interface.  It requires two accesses to the
	hardware to get anything done.  The driver therefore must protect
	all hardware accesses with a spinlock.

	Our fictional device has onboard read and write fifos.  Every
	time a new data item is available in the fifo, a read interrupt
	is generated.  Every time a space becomes available in the
	write fifo, a write interrupt is generated.  Semaphores are used
	to regulate client access to the fifos.
+++++ */

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include "skel_driver.h"


/* -----
	offsets in PCI i/o space to various h/w registers
----- */

#define	SKEL_COMMAND	0x00	/* offset to command register */
#define	SKEL_DATA		0x01	/* offset to data register */


/* -----
	commands (defined by the skel h/w.  See the databook "Skeletal
	Device Family", National Mototel Instruments (aka NMI).
---- */

#define SKEL_CMD_READ			0x00	/* read from fifo */
#define SKEL_CMD_WRITE			0x01	/* write into fifo */
#define SKEL_CMD_GET_INT_STATUS	0x02	/* see what caused interrupt */
#define SKEL_CMD_CLEAR_INT		0x03	/* clear the interrupt */
#define SKEL_CMD_SLOW			0x04	/* run slowly */
#define SKEL_CMD_FAST			0x04	/* run fastly */

/* -----
	flags for interrupt status register
----- */

#define SKEL_INT_READ 			0x01	/* interrupt is for a read */
#define SKEL_INT_WRITE 			0x02	/* interrupt is for a write */

/* -----	
	misc tidbits
----- */

#define SKEL_WRITE_FIFO_DEPTH	32

/* -----
	private types, definitions
----- */

#if DEBUG
#define ddprintf(x)	dprintf x
#else
#define ddprintf(x) (void)0
#endif

#define NUM_OPENS  2		/* the max # of concurrent opens for this driver */

/* -----
	each open instance of the driver has a private data area.  We just put
	some silly statistics there.  If this driver were to support several
	distinct identical devices (for instance, the serial ports), some
	context for each individual device could be kept there as well.
----- */

typedef struct {
	long num_bytes_written;
	long num_bytes_read;
} skel_data;


/* -----
	globals for this driver
----- */

static long				open_lock = 0;			/* spinlock for open/close */
static long				hw_lock = 0;			/* spinlock for hw access */
static long				read_sem = -1;			/* data avail to read sem */
static long				write_sem = -1;			/* space avail to write sem */
static long				open_count = NUM_OPENS;	/* # opens we can do */
static volatile bool	initialized = FALSE;	/* driver set up */
static bool				found_device = FALSE;	/* flag: found our h/w */
static pci_info			pci;					/* pci info */
static vchar			*skel;					/* -> skel h/w */


/* ----------
	init_driver

	Called when driver is loaded.  One-time initializations go here.
----- */
status_t
init_driver (void)
{
	int			i;

	ddprintf (("skeleton driver: %s %s\n", __DATE__, __TIME__));
	
	for (i = 0; ; i++) {
		if (get_nth_pci_info (i, &pci) != B_NO_ERROR) {
			ddprintf (("No skeleton hardware found!\n"));
			return B_ERROR;
		}
		if ((pci.vendor_id == 0x1234) && (pci.device_id == 0x5678)) {
			break;
		}
	}
	found_device = TRUE;

	skel = (vchar *) pci.u.h0.base_registers[0];

	/* h/w initializations could go here */

	return B_NO_ERROR;
}


/* ----------
	enter_crit
----- */
static cpu_status
enter_crit (spinlock *lock)
{
	cpu_status	status;

	status = disable_interrupts ();
	acquire_spinlock (lock);

	return status;
}


/* ----------
	exit_crit
----- */
static void
exit_crit (spinlock *lock, cpu_status status)
{
	release_spinlock (lock);
	restore_interrupts (status);
}


/* ----------
	skel_inth - the interrupt handler.

	NOTE that being rescheduled in the middle of an interrupt
	handler is usually not a good idea, especially when the hardware is
	not in a consistent state or a spinlock is held.  To avoid being
	rescheduled, we use a special version of release_sem.
----- */

static bool
skel_inth (void *data)
{
	cpu_status	status;
	char		val;
	
	/* NOTE that we do not use enter_crit(), as interrupts are already
	   disabled by virtue of being in an interrupt handler.  No harm
	   would come of using enter_crit(), it just would be a tad slower */

	acquire_spinlock (&hw_lock);			/* get exclusive access to h/w */

	*(skel + SKEL_COMMAND) = SKEL_CMD_GET_INT_STATUS;
	val = *(skel + SKEL_DATA);

	if (val & SKEL_INT_READ) {						/* a read interrupt? */
		*(skel + SKEL_COMMAND) = SKEL_CMD_CLEAR_INT;/* clear interrupt */
		*(skel + SKEL_DATA) = SKEL_INT_READ;
		release_spinlock (&hw_lock);
		release_sem_etc(read_sem, 1, B_DO_NOT_RESCHEDULE);
		return TRUE;
	} else if (val & SKEL_INT_WRITE) {				/* a write interrupt? */
		*(skel + SKEL_COMMAND) = SKEL_CMD_CLEAR_INT;/* clear interrupt */
		*(skel + SKEL_DATA) = SKEL_INT_WRITE;
		release_spinlock (&hw_lock);
		release_sem_etc(write_sem, 1, B_DO_NOT_RESCHEDULE);
		return TRUE;
	}

	release_spinlock (&hw_lock);
	return FALSE;
}


/* ----------
	skel_open - called on each open().
----- */

static status_t
skel_open (device_info *dev, uint32 flags)
{
	int			old;
	skel_data	*sd;
	cpu_status	status;
	status_t		retval = B_ERROR;

	if (!found_device)					/* was device found? */
		return B_ERROR;

	status = enter_crit (&open_lock);
	
	/* ensure we don't open the driver too many times */

	if (open_count < 1) {
		exit_crit (&open_lock, status);
		return B_ERROR;
	}

	/* the first open sets up the global resources needed by the driver */

	if (open_count == NUM_OPENS) {		/* first one? */

		read_sem = create_sem (0, "skel read");
		if (read_sem < 0)
			goto err_exit;

		write_sem = create_sem (SKEL_WRITE_FIFO_DEPTH, "skel write");
		if (write_sem < 0)
			goto err_exit;

		/* change sem owner to system, so they are not destroyed when
		   this particular client goes away */

		set_sem_owner (read_sem, B_SYSTEM_TEAM);
		set_sem_owner (write_sem, B_SYSTEM_TEAM);

		install_io_interrupt_handler (pci.u.h0.interrupt_line, skel_inth, NULL, 0);
	}
	
	open_count -= 1;
	exit_crit (&open_lock, status);
	sd = dev->private_data = calloc (1, sizeof(*sd));
	return B_NO_ERROR;

err_exit:
	if (read_sem >= 0)
		delete_sem (read_sem);
	if (write_sem >= 0);
		delete_sem (read_sem);
	exit_crit (&open_lock, status);
	return B_ERROR;
}


/* ----------
	skel_close
----- */

static status_t
skel_close (device_info *dev)
{
	cpu_status	status;

	status = enter_crit (&open_lock);
	
	open_count += 1;
	
	if (open_count >= NUM_OPENS) {	/* last one out turns off the lights */
		remove_io_interrupt_handler (pci.u.h0.interrupt_line, skel_inth);
		delete_sem (write_sem);
		delete_sem (read_sem);
		open_count = NUM_OPENS;		/* just in case... */
	}

	exit_crit (&open_lock, status);
	free (dev->private_data);		/* free up resources we're using */

	return B_NO_ERROR;
}


/* ----------
	skel_read - read some stuff from our device.  We ignore the 'pos'
	argument, as the skel is not a positionable device.
----- */

static status_t
skel_read (device_info *dev, void *buf, ulong count, ulong pos)
{
	long		i;
	skel_data	*sd;
	cpu_status	status;

	sd = (skel_data *)dev->private_data;

	for (i = 0; i < count; i++) {
		acquire_sem(read_sem);				/* wait for something to read */
		status = enter_crit (&hw_lock);		/* get exclusive hw access */

		*(skel + SKEL_COMMAND) = SKEL_CMD_READ;
		((char *)buf)[i] = *(skel + SKEL_DATA);

		exit_crit (&hw_lock, status);
	}

	sd->num_bytes_read += count;
	return count;
}


/* ----------
	skel_write - write some stuff to our device.  We ignore the 'pos'
	argument, as the skel is not a positionable device.
----- */

static status_t
skel_write (device_info *dev, void *buf, ulong count, ulong pos)
{
	long		i;
	skel_data	*sd;
	cpu_status	status;

	sd = (skel_data *)dev->private_data;

	for( i = 0; i < count; i++) {
		acquire_sem(write_sem);				/* wait for space to write */
		status = enter_crit (&hw_lock);		/* get exclusive hw access */

		*(skel + SKEL_COMMAND) = SKEL_CMD_WRITE;
		*(skel + SKEL_DATA) = ((char *)buf)[i];

		exit_crit (&hw_lock, status);
	}

	sd->num_bytes_written += count;
	return count;
}


/* ----------
	skel_control
----- */

static status_t
skel_control (device_info *dev, uint32 msg, void *buf)
{
	cpu_status	status;
	skel_data	*sd;
	skel_stats	*stats;

	sd = (skel_data *)dev->private_data;
	
	switch (msg) {

	case SKEL_SLOW:					/* make h/w go slow */
		status = enter_crit (&hw_lock);
		*(skel + SKEL_COMMAND) = SKEL_CMD_SLOW;
		exit_crit (&hw_lock, status);
		break;

	case SKEL_FAST:					/* make h/w go fast */
		status = enter_crit (&hw_lock);
		*(skel + SKEL_COMMAND) = SKEL_CMD_FAST;
		exit_crit (&hw_lock, status);
		break;

	case SKEL_REPORT_STATISTICS:	/* how are we doing so far? */
		stats = (skel_stats *) buf;
		stats->num_reads = sd->num_bytes_read;
		stats->num_writes = sd->num_bytes_written;
		stats->current_clients = NUM_OPENS - open_count;
		return B_NO_ERROR;
				
	default:
		return B_ERROR;
	}

	return B_NO_ERROR;
}


/* -----
	driver-related structures
----- */

device_hooks	devices[] = {
	{
		"/dev/skel",	/* name */
		skel_open, 		/* -> open entry point */
		skel_close, 		/* -> close entry point */
		skel_control, 	/* -> control entry point */
		skel_read,		/* -> read entry point */
		skel_write		/* -> write entry point */
	},
	0
};
