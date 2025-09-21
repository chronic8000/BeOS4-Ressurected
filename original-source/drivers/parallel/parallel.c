/* ++++++++++
	FILE:	parallel.c
	REVS:	$Revision$
	NAME:	herold
	DATE:	Sept 14, 1998
	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

	Written in 1998 by Christian Bauer.
	Adapted for Be's internal build by Robert Herold, Sept 1998.
	
	Code revisited 1999-2000 by Mathias Agopian.

	NOTES:

	- Supports all parallel ports
	- IEEE1284 compliant
	- Handle up to 4 parallel ports. Regular address are 0x378, 0x278 and 0x3BC.
	- One DMA channel is used for all ports
	- Use of Device Manager
	
	List of changes:
	
	Author		Date		Subject
	-------------------------------
	M.AG.		02/20/1999	Modification of some timeouts (see IEEE-1284)
	M.AG.		02/20/1999	release_sem() now use B_DO_NOT_RESCHEDULE (bug introduced in the last release)
	M.AG.		02/20/1999	dev_write_fifo() doesn't need to memcpy() user data anymore (area cloning)
	M.AG.		02/20/1999	Changed IRQMODE_FIFO_WRITE so that we don't need to wait for the fifo to be empty
	M.AG.		02/20/1999	dev_write_fifo() rewriten to avoid disturbing other drivers (as bt848)
	M.AG.		02/23/1999	The device detection is now done in init_hardware() instead of init_driver()
	M.AG.		02/23/1999	Fixed a bug in the detection of the FIFO size
	M.AG.		02/26/1999	DMA works!
	M.AG.		02/27/1999	Changed default timeout to 1 sec (was 0.1 sec)
	M.AG.		02/28/1999	DMA transferts are not limited to 65KB anymore.
	M.AG.		02/28/1999	Support for multiclient (can be opened several times, if acces flags are != ).
	M.AG.		02/28/1999	EINVAL is returned if attempt to access the device in incorrect access mode
	M.AG.		03/28/1999	Fixed a crashing bug when sending buffers of 65536 bytes (this is what 'cat' does)
	M.AG.		04/24/1999	Fixed a major bug in FIFO mode (the FIFOFS flag is now used to stop filling the FIFO when it's full)
	M.AG.		04/24/1999	Fixed a major bug in FIFO mode (we MUST wait the FIFO to become empty at the end of each fifo'ed Xfer)
	M.AG.		05/20/1999	Little changes to suppress some warning
	M.AG.		05/21/1999	Fixed a major bug in dev_write_xxx (we MUST wait until the device is not BUSY)
	M.AG.		06/22/2000	Fixed the IRQ write routine. Sometimes IRQ were lost.
	M.AG.		06/22/2000	Don't use the IRQ routine anymore (even if it works now), it's too hard for the kernel (50000 irq/s)
	M.AG.		06/19/2000  Fixed a potential bug when printing in DMA mode, if the last 16 bytes of the transfer were not sent, no error was reported
	M.AG.		06/19/2000	Fixed the annoying bug that didn't allowed to restart failed DMA transferts. The nb of bytes actually sent is now reported (instead of 0)
	M.AG.		06/19/2000	Fixed a bug when the FIFO would not get empty soon enough (because the printer is busy). The failure timeout is now 10s instead of 2s
	M.AG.		06/19/2000  Fixed a potential deadlock if for some reason the FIFO does not get FULL as we are filling it - weird case.

+++++ */


#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>

#include <OS.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>

#include <parallel_driver.h>
#include "parallel.h"
#include "detect.h"
#include "dongle.h"
#include "ieee1284.h"



int32 api_version = B_CUR_DRIVER_API_VERSION;


/****************************************************************/
/* Types														*/
/****************************************************************/

/****************************************************************/
/* Defines														*/
/****************************************************************/

/****************************************************************/
/* External Prototypes											*/
/****************************************************************/

extern status_t configure			(par_port_info *d);

/****************************************************************/
/* Static Prototypes											*/
/****************************************************************/

static int32	dev_irq_handler		(void *data);

static status_t dev_open			(const char *name, uint32 flags, void **cookie);
static status_t dev_close			(void *cookie);
static status_t dev_control			(void *cookie, uint32 op, void *data, size_t len);
static status_t dev_read			(void *cookie, off_t position, void *data, size_t *len);
static status_t dev_write			(void *cookie, off_t position, const void *data, size_t *len);

static status_t dev_write_pio			(par_port_info *d, const void *data, size_t *len);
static status_t dev_write_irq			(par_port_info *d, const void *data, size_t *len);
static status_t dev_write_irq_blk		(par_port_info *d, const void *data, size_t *len);
static status_t dev_write_fifo			(par_port_info *d, const void *data, size_t *len, bool is_ecp);
static status_t dev_write_fifo_blk		(par_port_info *d, const void *data, size_t *len, bool is_ecp);
static status_t dev_write_isa_dma		(par_port_info *d, const void *data, size_t *len, bool is_ecp);
static status_t dev_write_isa_dma_blk	(par_port_info *d, const void *data, size_t *len, bool is_ecp);

static status_t terminate_ecp		(par_port_info *d);
static status_t dev_wait			(par_port_info *d);
static status_t wait_transfert_end	(par_port_info *d);
static status_t wait_fifo_empty		(par_port_info *d);
static void 	send_init			(int io_base);
static status_t	HardwareInit		(void);
static device_datas_t *FindInfoArea	(bool create);


#ifdef __INTEL__
extern void get_dma_count_and_status(int channel, int *pcount, int *pstatus);
#endif

/****************************************************************/
/* Global Variables												*/
/****************************************************************/

device_datas_t *device_data;

static const char *dma_area_name = "be:area:parallel:DMA";
isa_module_info	*isa;
static const char *isa_name = B_ISA_MODULE_NAME;
static const char *device_area_name = "be:area:parallel:DeviceInfo";


/* Get device hook functions */
static device_hooks parallel_device_hooks =
{
	&dev_open,
	&dev_close,
	&dev_free,
	&dev_control,
	&dev_read,
	&dev_write
};

static device_hooks dongle_device_hooks =
{
	&dev_dgl_open,
	&dev_dgl_close,
	&dev_dgl_free,
	&dev_dgl_control,
	&dev_dgl_read,
	&dev_dgl_write
};

/* Devices names */
static char *par_port_name[NUM_PORTS] =
{
	kDeviceBaseName "1",
	kDeviceBaseName "2",
	kDeviceBaseName "3",
	kDeviceBaseName "4"
};

static char *par_port_dongle_name[NUM_PORTS] =
{
	kDongleBaseName "1",
	kDongleBaseName "2",
	kDongleBaseName "3",
	kDongleBaseName "4"
};


/****************************************************************/
/* Implementation												*/
/****************************************************************/

const char **publish_devices(void)
{
	DD(bug("parallel: publish_devices\n"));
	return name_list;
}

device_hooks *find_device(const char *name)
{
	if (strncmp(name, kDeviceBaseName, strlen(kDeviceBaseName)) == 0)
	{ /* parallel regular device */
		return &parallel_device_hooks;
	}
	else if (strncmp(name, kDongleBaseName, strlen(kDongleBaseName)) == 0)
	{ /* parallel dongle device */
		return &dongle_device_hooks;
	}
	return NULL;
}


device_datas_t *FindInfoArea(bool create)
{
	if (create == true)
	{ /* If the area exists, destroy it */
		void *addr;
		area_id area;
		const uint32 size = (((sizeof(device_datas_t) % B_PAGE_SIZE) + 1)*B_PAGE_SIZE);
		DDD(bug("parallel: FindInfoArea() -> CREATE\n"));
		while ((area = find_area(device_area_name)) > 0)
			delete_area(area);
		area = create_area(	device_area_name,
							&addr,
							B_ANY_KERNEL_ADDRESS,
							size,
							B_NO_LOCK,
							B_READ_AREA|B_WRITE_AREA);
		if (area > 0)
		{
			DDD(bug("parallel: Area created @ 0x%x\n", addr));
			return (device_datas_t *)addr;
		}
	}
	else
	{ /* Find the area */
		area_id area;
		DDD(bug("parallel: FindInfoArea() -> FIND\n"));
		area = find_area(device_area_name);
		if (area > 0)
		{
			area_info info;
			if (get_area_info(area, &info) == B_OK)
			{
				DDD(bug("parallel: Area found @ 0x%x\n", info.address));
				return info.address;
			}
		}
	}
	return (device_datas_t *)0;
}

/*
 *  Hardware initialization upon system startup
 */


status_t init_hardware(void)
{
	status_t err = B_OK;
	platform_type pl = platform();

	D(set_dprintf_enabled(true));
	DD(bug("parallel: init_hardware\n"));


	if (get_module(isa_name, (module_info **) &isa))
		return ENOSYS;

	if (pl == B_MAC_PLATFORM)
	{ /* This driver doesn't work on a Mac */
		err = ENODEV;
	}
	else if (pl == B_BEBOX_PLATFORM)
	{ /* On a BeBox, configure parallel port for ISA mode (I/O 0x378, IRQ 7) */
		write_io_8(MIO_INDEX, PCFG1);
		write_io_8(MIO_DATA, (read_io_8(MIO_DATA) & 0x10) | 0x09);
		write_io_8(MIO_INDEX, PCFG2);
		write_io_8(MIO_DATA, read_io_8(MIO_DATA) & 0xd0);
	}

	/* Build the devices list */
	if (err == B_OK)
	{
		err = B_ERROR;
		device_data = FindInfoArea(true);	/* Create a new info area */
		if (device_data)
			err = HardwareInit();
	}

	if (isa)
	{
		put_module(isa_name);
		isa = NULL;
	}

	return err;
}

/* Send INIT signal, unselect device */
static void send_init(int io_base)
{
	write_io_8(io_base + PCON, C_SELIN);
	snooze(INIT_DELAY);
	write_io_8(io_base + PCON, C_SELIN | C_INIT);
}

/*
 *  Driver loaded
 */

status_t init_driver(void)
{
	status_t err = B_OK;
	int i;
	
	DD(bug("parallel: init_driver\n"));
	
	/* Get ISA Module */
	if (get_module(isa_name, (module_info **) &isa))
		return ENOSYS;

	device_data = FindInfoArea(false);	/* Find the info area */
	if (!device_data)
	{ /* The info area doesn't exist (should not happen) */
		err = B_ERROR;
		device_data = FindInfoArea(true);	/* Create the info area */
		if (device_data)
			err = HardwareInit();
	}
	
	/* Create the thread safe semaphore */
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		par_port_info *d = pinfo + i;
		if (d->exists)
		{
			d->hw_lock = create_sem(1, "parallel hw_lock");
			set_sem_owner(d->hw_lock, B_SYSTEM_TEAM);
		}
	}

	/* Initialize DMA */
	if (err == B_NO_ERROR)
	{
		physical_entry pe;
		int32 next_boundary;
		area_id area;
		int32 offset = 0;

		/* Init global variables */
		dma.dma_available = false;
		dma.dma_in_use = 1;
		dma.dma_table_area = -1;
		dma.dma_table = NULL;
	
		/* If the area exists, destroy it */
		while ((area = find_area(dma_area_name)) > 0)
			delete_area(area);

		/* Physical address must not cross a 64KB page boundary */ 
		dma.dma_table_area = create_area(	dma_area_name,
											(void **)&dma.dma_table,
											B_ANY_KERNEL_ADDRESS,
											DMA_TRANSFERT_SIZE * 2,
											B_LOMEM,
											B_READ_AREA | B_WRITE_AREA);
	
		if (dma.dma_table_area > 0)
		{
			get_memory_map(dma.dma_table, DMA_TRANSFERT_SIZE, &pe, 1);
			next_boundary = ((int32)pe.address + B_MAX_ISA_DMA_COUNT) & ~(B_MAX_ISA_DMA_COUNT-1);

			if (((int32)pe.address + DMA_TRANSFERT_SIZE) > next_boundary)
				offset = next_boundary - (uint32)pe.address;

			dma.dma_table_phys	= (isa_dma_entry *)((uint32)pe.address + offset);
			dma.dma_table		= (isa_dma_entry *)((uint32)dma.dma_table + offset);

			D(bug("parallel: DMA Address [initial:%08lx, offset:%08lx] -> %08lx\n", pe.address, offset, dma.dma_table_phys));

			/* Area created, grab DMA channel */
			if (lock_isa_dma_channel(dma.dma_channel) == B_NO_ERROR)
			{
				int i;
				dma.dma_available = true;
				dma.dma_in_use = 0;
				for (i=0; i<NUM_PORTS; i++)
					if (pinfo[i].type & PAR_TYPE_ECP)
						pinfo[i].type |= PAR_TYPE_DMA;
			}
		}
	}

	
	/* If error, close the ISA Module */
	if (err != B_OK)
	{
		if (isa)
		{
			put_module(isa_name);
			isa = NULL;
		}
	}

	return err;
}

static status_t HardwareInit(void)
{
	int	i;
	int	io_base;
	int	dev_name_index;
	par_port_info *d;
	status_t err = B_OK;

	/* Build a list of availlable ports
	  (detected by BIOS and Devices manager)  */

	configure(pinfo);

	/* Check detected device as well as regular devices */
	dma.dma_channel = -1; /* FIXME */
	dev_name_index = 0;
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		D(bug("parallel:\n"));
		D(bug("parallel: testing port #%d...\n", i));
		d = pinfo + i;
		io_base = d->io_base;

		/* No port detected for this entry in the device list */
		if (io_base == 0)
			continue;

		/* FIXME */
		if (dma.dma_channel == -1)
			dma.dma_channel = d->device_dma.dma_channel;
		/* FIXME */

		/* Deselect device */
		write_io_8(io_base + PCON, C_INIT);		

		/* test for ECP */
		if (PARA_check_ecp_port(io_base) == true)
		{ /* This is an ECP port */
			d->type |= (PAR_TYPE_ECP | PAR_TYPE_REVERSE_CHANNEL);
			if (PARA_check_isa_port(d) == true)
			{ /* We only support 8 bits ISA implementation */
				if (PARA_test_fifo(d) == true)
				{ /* The FIFO is functionning correctly */
					d->type |= (PAR_TYPE_ECP | PAR_TYPE_REVERSE_CHANNEL);
					write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR); /* switch to PS/2 ECP mode */
					goto ecp_ok;
				}
			}
		}

		if (d->type & PAR_TYPE_ECP)
		{
			E(bug("parallel: User reported this port as an ECP!\n"));
			E(bug("parallel: This chipset is not supported or not functionning.\n"));
		}
		d->type &= ~(PAR_TYPE_ECP | PAR_TYPE_REVERSE_CHANNEL);

ecp_ok:
		if (PARA_check_data_port(io_base) == false)
		{ /* All ports _MUST_ pass this test! */
			if (d->exists)
			{ E(bug("parallel: port at %.4x is not functionning\n", io_base));
			}
			else
			{ D(bug("parallel: port at %.4x doesn't exist\n", io_base));
			}
			d->exists = false;
			continue;
		}

		D(bug("parallel: parallel port at 0x%x detected and operationnal\n", io_base));
		d->exists = true;
		name_list[dev_name_index*2] = par_port_name[i];
		name_list[dev_name_index*2+1] = par_port_dongle_name[i];
		dev_name_index++;

		if (PARA_check_ps2_port(io_base) == true)
		{ /* This is a reverse channel port */ 
			d->type |= PAR_TYPE_REVERSE_CHANNEL;
		}


		/* Find the IRQ line for this port */
		if (d->irq_num == M_IRQ_NONE)
			d->irq_num = PARA_irq_line(d);
		else if (d->irq_num >= 0)
		{
			int irq = PARA_irq_line(d);
			if (irq >= 0)
			{
				if (d->irq_num != irq)
				{
					E(bug("parallel: BIOS IRQ (%d) and Device Manager IRQ (%d) differ!\n", irq, d->irq_num));
				}
			}
		}
		D(bug("parallel: using IRQ # : %d\n", d->irq_num));
	}

	/* Close the device list */
	name_list[dev_name_index*2] = NULL;

	/* No parallel ports present? Then return */
	if (dev_name_index == 0)
		err = ENODEV;

	return err;
}


/*
 *  Driver unloaded
 */

void uninit_driver(void)
{
	int i;
	DD(bug("parallel: uninit_driver\n"));

	/* Delete the thread safe semaphore */
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		par_port_info *d = pinfo + i;
		if (d->exists)
			delete_sem(d->hw_lock);
	}

	/* Free DMA channel */
	if (dma.dma_available)
		unlock_isa_dma_channel(dma.dma_channel);

	/* Free DMA table area */
	if (dma.dma_table_area > 0)
		delete_area(dma.dma_table_area);

	if (isa)
		put_module(isa_name);
}




static int lookup_device_name(const char *name) 
{
	if (strncmp(name, kDeviceBaseName, strlen(kDeviceBaseName)))
		return -1;
	return name[strlen(name) - 1] - '1';
}

/********************************/
/*  Open device					*/
/********************************/

static status_t dev_open(const char *name, uint32 flags, void **cookie)
{
	int port_num;
	par_port_info *d;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_open(%s, %lu, %08lx)\n", name, flags, cookie));

	/* Check device name, select right par_port_info */
	port_num = lookup_device_name(name);
	if (port_num < 0)
		return EINVAL;
	d = pinfo + port_num;

	return do_dev_open(d, flags, cookie);
}

status_t do_dev_open(par_port_info *d, uint32 flags, void **cookie)
{
	user_info *info;
	status_t err;
	int current;
	int open_mask;
	if ((err = acquire_sem(d->hw_lock)) != B_OK)
		return err;

	/* Open the device with the requested access */
	switch (flags & O_ACCMODE)
	{
		case O_RDONLY:	// We can always open the driver in O_RDONLY
			d->open_mode |= (open_mask = M_RDONLY_MASK);
			break;
			
		case O_WRONLY: // We cannot open the driver for writting if it is already opened for writting
			if ((d->open_mode & M_WRONLY_MASK) == 0) {
				d->open_mode |= (open_mask = M_WRONLY_MASK);
				break;
			}
			err = EBUSY;
			goto error_already_opened;
	
		case O_RDWR: // We cannot open the driver for writting if it is already opened for writting
			if ((d->open_mode & M_WRONLY_MASK) == 0) {
				d->open_mode |= (open_mask = M_RDWR_MASK);
				break;
			}
			err = EBUSY;
			goto error_already_opened;

		default:
			err = EINVAL;
			goto error_already_opened;
	}
	
	/* Init the device the 1st time it's opened */
	if (d->open == 0)
		err = init_open_device(d);
	
	if (err != B_OK)
		goto error;
	
	info = (user_info *)malloc(sizeof(user_info));
	if (info)
	{
		info->d = d;
		info->open_mode = open_mask;
		*cookie = (void *)info;
	}
	else
	{
		/* Not enough memory : Abort */
		err = ENOMEM;
		goto error;
	}

	d->open++;
	release_sem(d->hw_lock);
	return B_OK;

error:
error_already_opened:
	release_sem(d->hw_lock);
	*cookie = NULL;
	return err;
}



status_t init_open_device(par_port_info	*d)
{
	status_t err = B_OK;

	/* Allocate FIFO buffer */
	d->buffer = malloc(TMP_BUFFER_SIZE);
	if (d->buffer == NULL)
		return ENOMEM;

	/* Initialize par_port_info variables */
	d->mode = PAR_MODE_CENTRONICS;
	d->irq_mode = IRQMODE_INACTIVE;
	d->ieee1284_mode = IEEE1284_COMPATIBILITY;
	d->ecp_read_mode = false;
	d->timeout = 1000000;	/* 1s timeout by default */
	d->failure = 10000000;	/* 10s for the failure timeout */
	d->type &= ~PAR_TYPE_IRQ;
	d->error = false;

	/* Create semaphores */
	d->irq_sem = create_sem(0, "parallel irq_sem");
	set_sem_owner(d->irq_sem, B_SYSTEM_TEAM);


	/* Go to compatibility mode */
	terminate_ieee1284(d);

	/* Set port to PS/2 mode, clear FIFO, disable interrupts */
	write_io_8(d->io_base + PCON, C_INIT);
	if (d->type & PAR_TYPE_ECP)
		write_io_8(d->io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);

	/* Install interrupt handler */
	if (d->irq_num > 0)
	{
		D(bug("parallel: install_io_interrupt_handler : IRQ #%d, handler: %08lx\n", d->irq_num, (void *)&dev_irq_handler));
		err = install_io_interrupt_handler(d->irq_num, dev_irq_handler, (void *)d, 0);
		if (err != B_OK)
			goto error;
		d->type |= PAR_TYPE_IRQ;
	}

	/* Select device and return OK */
	write_io_8(d->io_base + PCON, C_SELIN | C_INIT);
	return B_OK;

error:
	/* Free allocated resources */
	delete_sem(d->irq_sem);
	free(d->buffer);
	return err;
}


/*
 *  Close device
 */

static status_t dev_close(void *cookie)
{
	par_port_info *d = ((user_info *)cookie)->d;
	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_close\n"));
	if (d == NULL)
		return EINVAL;
	return B_OK;
}


/*
 *  Close device, I/O completed
 */

status_t dev_free(void *cookie)
{
	status_t err;
	par_port_info *d = ((user_info *)cookie)->d;
	int32 open_mode = ((user_info *)cookie)->open_mode;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_free\n"));
	if (d == NULL)
		return EINVAL;

	if ((err = acquire_sem(d->hw_lock)) != B_OK)
		return err;

	/* this works because the driver can be opened for write only once */
	if (open_mode & M_WRONLY_MASK)
		d->open_mode &= ~M_WRONLY_MASK;
	d->open--;

	if (d->open == 0)
	{ /* This is the last opened driver */

		/* Terminate IEEE 1284 mode */
		terminate_ieee1284(d);
	
		/* the driver is not opened anymore */
		d->open_mode = 0;

		/* Set port to ISA compatible mode, clear FIFO, disable interrupts, unselect device */
		if (d->type & PAR_TYPE_ECP)
			write_io_8(d->io_base + ECR, EC_ISA_COMPATIBLE | EC_ERRINTR | EC_SERVICEINTR);
		write_io_8(d->io_base + PCON, C_INIT);
	
		/* Remove interrupt handler */
		if (d->type & PAR_TYPE_IRQ)
		{
			D(bug("parallel: remove_io_interrupt_handler : IRQ #%d, handler: %08lx\n", d->irq_num, (void *)&dev_irq_handler));
			remove_io_interrupt_handler(d->irq_num, dev_irq_handler, (void *)d);
		}
	
		/* Delete semaphores */
		delete_sem(d->irq_sem);

		/* Delete FIFO buffer */
		free(d->buffer);
	}

	release_sem(d->hw_lock);

	/* Free the user cookie */
	free(cookie);

	return B_OK;
}


/*
 *  Wait that the device become READY and operationnal
 */
 
static status_t dev_wait(par_port_info *d)
{
	register int i;
	bigtime_t timeout_time = system_time() + d->failure;
	bigtime_t nap_length = 1000;
	const int io_stat = d->io_base + PSTAT;
	int stat;
	
	/* STAT register OK */
	const int mask	= S_BUSY | S_SEL | S_FAULT | S_PERR;
	const int val	= S_BUSY | S_SEL | S_FAULT;
	/* STAT register ERROR */
	const int emask	= S_SEL | S_FAULT | S_PERR;
	const int eval	= S_SEL | S_FAULT;

	/* ~20us busy-waiting */
	for (i=0 ; i<10 ; i++)
	{
		stat = read_io_8(io_stat);
		if ((stat & mask) == val)
			return B_OK;
		if ((stat & emask) != eval)
			return EIO;
		spin(2);
	}

	/* This seems to be a slow device */	
	while(system_time() < timeout_time)
	{
		stat = read_io_8(io_stat);
		if ((stat & mask) == val)
			return B_OK;
		if ((stat & emask) != eval)
			return EIO;
		snooze(nap_length);
		if(nap_length < 500000)
			nap_length *= 2;
	}

	return EIO;
}


/*
 *  Wait 35ms for ECP acknowledge
 */

#define wait_for_ecp_ack	wait_for_ieee1284_status


/*
 *  Negotiate ECP mode
 */

static status_t negotiate_ecp(par_port_info *d, uint8 ext_byte)
{
	status_t err = negotiate_ieee1284(d, ext_byte);
	
	if (err != B_OK)
		return err;

	/* We are now in ECP forward mode */
	d->ecp_read_mode = false;
	return B_OK;
}

/*
 *  Terminate ECP mode
 */

static status_t terminate_ecp(par_port_info *d)
{
	/* Switch to PS/2 mode for termination, clear FIFO and interrupts */
	write_io_8(d->io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);

	/* Terminate IEEE 1284 */
	return terminate_ieee1284(d);
}


/*
 *  Negotiate ECP reverse transfer
 */

static status_t negotiate_ecp_read(par_port_info *d)
{
	int io_base = d->io_base;

	/* The chip must be in 001 (PS2) mode prior to change the direction */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);	/* to PS/2 */
	write_io_8(io_base + PCON, C_DIR | C_INIT | C_AUTOFD);				/* reverse mode */
	write_io_8(io_base + ECR, EC_ECP | EC_ERRINTR | EC_SERVICEINTR);	/* return to ECP mode */
	spin(5);

	/* Request reverse transfer */
	write_io_8(io_base + PCON, C_DIR);

	/* Wait 35ms for AckReverse */
	if (!wait_for_ecp_ack(d, S_PERR, 0)) {
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
		write_io_8(io_base + PCON, C_INIT);
		return EIO;
	}

	/* OK, we are now in reverse mode */
	d->ecp_read_mode = true;
	return B_OK;
}


/*
 *  Negotiate ECP forward transfer
 */

static status_t negotiate_ecp_write(par_port_info *d)
{
	int io_base = d->io_base;

	/* Negate INIT */
	write_io_8(io_base + PCON, C_INIT | C_DIR);

	/* Wait 35ms for AckReverse */
	if (!wait_for_ecp_ack(d, S_PERR, S_PERR)) {
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
		write_io_8(io_base + PCON, C_INIT);
		return EIO;
	}

	/* OK, set port to forward mode */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + PCON, C_INIT);
	d->ecp_read_mode = false;
	return B_OK;
}


/*
 *  Control device
 */

static status_t dev_control(void *cookie, uint32 op, void *data, size_t len)
{
	par_port_info	*d = ((user_info *)cookie)->d;
	status_t		err;
	parallel_mode	mode = *(parallel_mode *)data;
	int ext_byte;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_control(%08lx, %08lx, %08lx, %ld)\n", cookie, op, data, len));

	switch (op)
	{
		case PAR_STATUS:
			/* Return PSTAT register (no need to acquire hw_lock) */
			*(uint8 *)data = read_io_8(d->io_base + PSTAT) & 0xF8;
			return B_OK;
	
		case PAR_RESET:
			/* Send INIT signal */
			err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
			if (!err) {
				if (d->mode == PAR_MODE_ECP) {
					/* Terminate ECP mode */
					terminate_ecp(d);
					d->mode = PAR_MODE_CENTRONICS;
				}
				send_init(d->io_base);
				release_sem(d->hw_lock);
				return B_OK;
			} else
				return err;

		case PAR_GET_PORT_TYPE:
			/* Get port type */
			*(int32 *)data = d->type;
			return B_OK;

		case PAR_SET_MODE: {	/* Set operating mode */
			mode = *(parallel_mode *)data;
			switch (mode) {

				/* Centronics nibble mode can always be used */
				case PAR_MODE_CENTRONICS:
					if (d->mode == PAR_MODE_ECP) {
						/* Terminate ECP mode */
						err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
						if (!err) {
							terminate_ecp(d);
							release_sem(d->hw_lock);
						} else
							return err;
					}
					d->mode = mode;
					return B_OK;

				/* Centronics byte mode requires a real reverse channel */
				case PAR_MODE_CENTRONICS_BYTE:
					if (d->type & PAR_TYPE_REVERSE_CHANNEL) {
						if (d->mode == PAR_MODE_ECP) {
							/* Terminate ECP mode */
							err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
							if (!err) {
								terminate_ecp(d);
								release_sem(d->hw_lock);
							} else
								return err;
						}
						d->mode = mode;
						return B_OK;
					}
					return EINVAL;

				/* ECP mode requires the interrupt */
				case PAR_MODE_ECP:
				case PAR_MODE_ECP_DEVICE_ID:
					if ((d->type & (PAR_TYPE_ECP | PAR_TYPE_IRQ)) == (PAR_TYPE_ECP | PAR_TYPE_IRQ)) {
						err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
						if (!err) {
							err = negotiate_ecp(d, mode == PAR_MODE_ECP_DEVICE_ID ? 0x14 : 0x10);
							release_sem(d->hw_lock);
							if (!err) {
								d->mode = PAR_MODE_ECP;
								return B_OK;
							} else
								return err;
						} else
							return err;
					}
					return EINVAL;

				default:
					return EINVAL;
			}
		}

		case PAR_REQUEST_DEVICE_ID:
			ext_byte = *(int *)data;
			if (ext_byte & 0x4)
			{
				err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
				if (err == B_NO_ERROR)
				{
					err = negotiate_ieee1284(d, ext_byte);
					release_sem(d->hw_lock);
				}
				return err;
			}
			return EINVAL;

		case PAR_GET_MODE:		/* Get current operating mode */
			*(int32 *)data = d->mode;
			return B_OK;

		case PAR_SET_READ_TIMEOUT:	/* Set ECP read timeout */
			d->timeout = *(bigtime_t *)data;
			return B_OK;

		case PAR_GET_READ_TIMEOUT:	/* Get ECP read timeout */
			*(bigtime_t *)data = d->timeout;
			return B_OK;

		case PAR_ABORT:				/* Abort the current read/write operation */
			delete_sem(d->irq_sem);	/* Delete semaphores */
			return B_OK;

		default:
			return EINVAL;
	}
}


/*-----------------------------------------------------------------*/
#pragma mark -

/*
 *  Read from device
 */

/* Method 1: Nibble mode transfer via programmed I/O and polling */
static status_t dev_read_nibble_pio(par_port_info *d, void *data, size_t *len)
{
	const int io_base = d->io_base;
	uint8 *p = (uint8 *)data;
	const size_t length = *len;
	size_t actual = 0;
	status_t err = B_OK;
	uint8 hinib;
	uint8 lonib;

	DDD(bug("parallel: read_nibble_pio, %d bytes\n", *len));

	/* Here, we are in Reverse Channel Nibble Mode */
	while (actual < length)
	{
		/* Is there any data availlable ? */
		if (read_io_8(io_base + PSTAT) & S_FAULT)
		{
			/* No data availlable */
			/* Go to reverse mode "idle state" */
			write_io_8(io_base + PCON, C_INIT | C_AUTOFD);
			break;
		}

		/* Negate HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_AUTOFD);

		/* Wait for PtrClk (ACK) low */
		if (!wait_ack_low(d))
		{
			/* timeout -> No more data ? */
			break;
		}

		/* Get low nibble */
		lonib = read_io_8(io_base + PSTAT);

		/* Assert HostBusy */
		write_io_8(io_base + PCON, C_INIT);

		/* Wait for PtrClk (ACK) high */
		if (!wait_ack_high(d))
		{
			/* timeout -> No more data ? */
			break;
		}

		/* Get the 2nd Nibble */

		/* Negate HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_AUTOFD);

		/* Wait for PtrClk (ACK) low */
		if (!wait_ack_low(d))
		{
			/* timeout -> No more data ? */
			break;
		}

		/* Get high nibble */
		hinib = read_io_8(io_base + PSTAT);

		/* Assert HostBusy */
		write_io_8(io_base + PCON, C_INIT);

		/* Wait for PtrClk (ACK) high */
		if (!wait_ack_high(d))
		{
			/* timeout -> No more data ? */
			break;
		}

		/* Construct data byte */
		hinib = (hinib >> 3) & 0x17;
		if (!(hinib & 0x10)) hinib |= 0x08;
		hinib &= 0xF;
		lonib = (lonib >> 3) & 0x17;
		if (!(lonib & 0x10)) lonib |= 0x08;
		lonib &= 0xF;
		*p++ = (hinib << 4) | lonib;

		actual++;
	}

	/* Return number of received bytes */
	*len = actual;
	DDD(bug("parallel: %d bytes read\n", actual));
	return err;
}

/* Method 3: Byte mode transfer via programmed I/O and polling */
static status_t dev_read_byte_pio(par_port_info *d, void *data, size_t *len)
{
	const int io_base = d->io_base;
	uint8 *p = (uint8 *)data;
	const size_t length = *len;
	size_t	actual = 0;
	status_t err = B_OK;

	DDD(bug("parallel: read_byte_pio, %d bytes\n", *len));

	while (actual < length)
	{
		/* Is there any data availlable ? */
		if (read_io_8(io_base + PSTAT) & S_FAULT)
		{
			/* No data availlable */
			/* Go to reverse mode "idle state" */
			write_io_8(io_base + PCON, C_INIT | C_DIR | C_AUTOFD);
			break;
		}

		/* Negate HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_AUTOFD);

		/* Wait for PtrClk (ACK) low */
		if (!wait_ack_low(d))
		{
			err = EIO;
			break;
		}

		/* Get byte */
		*p++ = read_io_8(io_base + PDATA);

		/* Assert HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_DIR);

		/* Wait for PtrClk (ACK) high */
		if (!wait_ack_high(d))
		{
			err = EIO;
			break;
		}

		/* Send HostClk */
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_STROBE);
		spin(50);
		write_io_8(io_base + PCON, C_INIT | C_DIR);

		actual++;
	}

	/* Return number of received bytes */
	*len = actual;
	DDD(bug("parallel:  %d bytes read\n", actual));
	return err;
}

/* Method 4: Byte mode transfer via programmed I/O and interrupt */
static status_t dev_read_byte_irq(par_port_info *d, void *data, size_t *len)
{
	const int io_base = d->io_base;
	uint8 *p = (uint8 *)data;
	const size_t length = *len;
	size_t	actual = 0;
	status_t err = B_OK;

	DDD(bug("parallel: read_byte_irq, %d bytes\n", *len));

	/* Set interrupt variables */
	d->error = false;
	d->irq_mode = IRQMODE_INACTIVE;

	while (actual < length)
	{
		/* Is there any data availlable ? */
		if (read_io_8(io_base + PSTAT) & S_FAULT)
		{
			/* No data availlable */
			/* Go to reverse mode "idle state" */
			write_io_8(io_base + PCON, C_INIT | C_DIR | C_AUTOFD);
			break;
		}

		/* Negate HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_IRQEN | C_AUTOFD);

		/* Wait for PtrClk (ACK) high->low */
		err = acquire_sem_etc(d->irq_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, d->timeout);
		if (err)
			break;

		/* Get byte */
		*p++ = read_io_8(io_base + PDATA);

		/* Assert HostBusy */
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_IRQEN);

		/* Wait for PtrClk (ACK) high */
		if (!wait_ack_high(d))
		{
			err = EIO;
			break;
		}

		/* Send HostClk */
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_IRQEN | C_STROBE);
		spin(50);
		write_io_8(io_base + PCON, C_INIT | C_DIR | C_IRQEN);

		actual++;
	}

	/* Reset port to forward direction, disable interrupt */
	d->irq_mode = IRQMODE_INACTIVE;

	/* Return number of received bytes */
	*len = actual;
	DDD(bug("parallel:  %d bytes read\n", actual));
	return err;
}

/* Method 5: ECP transfer via programmed I/O and interrupt */
static status_t dev_read_ecp(par_port_info *d, const void *data, size_t *len)
{
	int io_base;
	uint8 *p;
	size_t length;
	size_t actual;
	status_t err;
	size_t transfer_size;
	size_t transferred;

	DDD(bug("parallel: read_ecp, %d bytes\n", *len));

	io_base = d->io_base;

	/* Get transfer parameters */
	p = (uint8 *)data;
	length = *len;
	actual = 0;
	err = B_OK;

	/* If we are in write mode, negotiate reverse transfer */
	if (!d->ecp_read_mode) {
		err = negotiate_ecp_read(d);
		if (err) {
			*len = 0;
			return err;
		}
	}

	/* If there's data in the FIFO, get it now */
	while (length && !(read_io_8(io_base + ECR) & EC_FIFOES)) {
		*p++ = read_io_8(io_base + SDFIFO);
		length--;
		actual++;
	}

	/* Transfer remaining data with interrupt */
	while (length)
	{
		/* Transfer chunks of TMP_BUFFER_SIZE */
		transfer_size = length > TMP_BUFFER_SIZE ? TMP_BUFFER_SIZE : length;

		/* Set interrupt variables */
		d->error = false;
		d->data = (uint8 *)d->buffer;
		d->length = transfer_size;
		d->actual = 0;
		d->irq_mode = IRQMODE_FIFO_READ;

		/* Enable FIFO service interrupt */
		write_io_8(io_base + ECR, EC_ECP | EC_ERRINTR);

		/* Wait for transfer to be finished */
		err = acquire_sem_etc(d->irq_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, d->timeout);
		transferred = d->actual;
		memcpy(p, d->buffer, transferred);
		actual += transferred;
		length -= transferred;
		p += transferred;

		/* Error? Then exit */
		if (err || d->error)
			break;
	}

	/* Disable interrupt */
	d->irq_mode = IRQMODE_INACTIVE;
	write_io_8(io_base + ECR, EC_ECP | EC_ERRINTR | EC_SERVICEINTR);

	/* If we timed out and there's data left in the FIFO, get it now */
	if (err == B_TIMED_OUT) {
		DDD(bug("parallel: time out, %d bytes read\n", actual));
		while (length && !(read_io_8(io_base + ECR) & EC_FIFOES)) {
			*p++ = read_io_8(io_base + SDFIFO);
			length--;
			actual++;
		}
		err = B_OK;
	}

	/* Return number of transmitted bytes */
	*len = actual;
	DDD(bug("parallel:  %d bytes read\n", actual));
	if (!err && d->error)
		err = EIO;
	return err;
}

static status_t dev_read(void *cookie, off_t position, void *data, size_t *len)
{
	par_port_info *d = ((user_info *)cookie)->d;
	int32 open_mode = ((user_info *)cookie)->open_mode;
	status_t err;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_read(%08lx, %lu, %08lx, %lu)\n", cookie, (uint32)position, data, (uint32)(*len)));

	if ((open_mode & M_RDONLY_MASK) == 0)
		return EINVAL;

	/* Nothing to transfer? Then return */
	if (*len == 0)
		return B_OK;

	/* Acquire access to parallel port */
	err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
	if (err)
	{
		*len = 0;
		return err;
	}

	/* Select reception method */
	switch (d->mode)
	{
		case PAR_MODE_CENTRONICS:
			err = negotiate_ieee1284(d, IEEE1284_NIBBLE);
			if (err == B_NO_ERROR)
			{
				/* Nibble mode, polled */
				err = dev_read_nibble_pio(d, data, len);
			}
			else
			{
				*len = 0;
				err = EIO;
			}
			break;

		case PAR_MODE_CENTRONICS_BYTE:
			err = negotiate_ieee1284(d, IEEE1284_BYTE);
			if (err == B_NO_ERROR)
			{
				if (d->type & PAR_TYPE_IRQ)
				{
					/* Byte mode, with interrupt */
					err = dev_read_byte_irq(d, data, len);
				}
				else
				{
					/* Byte mode, polled */
					err = dev_read_byte_pio(d, data, len);
				}
			}
			else
			{
				*len = 0;
				err = EIO;
			}
			break;

		case PAR_MODE_ECP:
			/* ECP mode */
			err = dev_read_ecp(d, data, len);
			break;
	}

	/* Free parallel port */
	release_sem(d->hw_lock);
	return err;
}

/*-----------------------------------------------------------------*/
#pragma mark -

/* ===================================================
** wait_transfert_end()
** Wait the transfert finished. Returns the status.
** =================================================== */

status_t wait_transfert_end(par_port_info *d)
{
	/* STAT register OK */
	const int mask	= S_BUSY | S_SEL | S_FAULT | S_PERR;
	const int val	= S_BUSY | S_SEL | S_FAULT;
	/* STAT register ERROR */
	const int emask	= S_SEL | S_FAULT | S_PERR;
	const int eval	= S_SEL | S_FAULT;

	status_t err;
	const int io_base = d->io_base;
	do
	{
		err = acquire_sem_etc(d->irq_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, d->timeout);
		if (err == B_TIMED_OUT)
		{
			/* Timeout! Try to figure the reason of the timeout */
		
			int stat = read_io_8(io_base + PSTAT);

			DD(bug("wait_transfert_end: status = %02X [mask = %02X, eval = %02X]\n", stat, emask, eval));

			/* Is there an error? */
			if (d->error = ((stat & emask) != eval))
				return EIO;

			/* Or did we loose the IRQ? - ie: printer not busy */
			if ((stat & mask) == val)
			{ // we missed the IRQ! We return an error, and the irq routine will retry
				return EIO;
			}
			
			/* Nothing special, the transfert is slow, that's it! */
		}
	} while (err == B_TIMED_OUT);

	if ((err == B_OK) && (d->error == true))
		return EIO;

	return err;
}


/* ===================================================
** wait_fifo_empty()
** Wait the transfert finished. Returns the status.
** =================================================== */

status_t wait_fifo_empty(par_port_info *d)
{
	register int i;
	bigtime_t timeout_time = system_time() + d->failure;
	bigtime_t nap_length = 1000;	
	const int io_stat = d->io_base + PSTAT;
	const int io_ecr = d->io_base + ECR;

	/* ~20us busy-waiting */
	for (i=0 ; i<10 ; i++)
	{
		if (read_io_8(io_ecr) & EC_FIFOES)
			return B_OK;
		if ((read_io_8(io_stat) & (S_SEL | S_FAULT | S_PERR)) != (S_SEL | S_FAULT))
			return EIO;
		spin(2);
	}

	/* This seems to be a slow device */	
	while(system_time() < timeout_time)
	{
		if (read_io_8(io_ecr) & EC_FIFOES)
			return B_OK;
		if ((read_io_8(io_stat) & (S_SEL | S_FAULT | S_PERR)) != (S_SEL | S_FAULT))
			return EIO;
		snooze(nap_length);
		if (nap_length < 500000)
			nap_length *= 2;
	}

	return EIO;
}


/*-----------------------------------------------------------------*/
#pragma mark -

/*
 *  Write to device
 */

/* Method 1: Centronics transfer via programmed I/O and polling */
static status_t dev_write_pio(par_port_info *d, const void *data, size_t *len)
{
	const int io_base = d->io_base;
	const uint8 *p = (uint8 *)data;
	const size_t length = *len;
	status_t err = B_OK;
	size_t actual = 0;

	DDD(bug("parallel: dev_write_pio, %ld bytes\n", length));

	while (actual < length)
	{
		err = dev_wait(d);
		if (err != B_OK)
			break;

		/* Transmit byte with strobe */
		write_io_8(io_base + PDATA, *p++);
		spin(1);	/* "setup" min 0.75us, see "Parallel port complete", page 225 */
		write_io_8(io_base + PCON, C_SELIN | C_INIT | C_STROBE);
		spin(1);	/* "nStrobe" min 0.75us, see "Parallel port complete", page 225 */
		write_io_8(io_base + PCON, C_SELIN | C_INIT);
		spin(2);	/* see "Parallel port complete", page 227 */
		actual++;
	}

	/* Return number of transmitted bytes */
	*len = actual;
	DDD(bug("parallel: dev_write_pio, %ld bytes written\n", actual));
	return err;
}


/* Method 2: Centronics transfer via programmed I/O and interrupt */
static status_t dev_write_irq(par_port_info *d, const void *data, size_t *len)
{
	status_t err = B_OK;
	size_t length = *len;
	*len = 0;

	DD(bug("parallel: dev_write_irq, %ld bytes\n", length));

	while ((length) && (err == B_NO_ERROR))
	{
		const size_t TRANSFER_SIZE = TMP_BUFFER_SIZE;
		size_t sent = ((length < TRANSFER_SIZE) ? (length) : (TRANSFER_SIZE));
		memcpy(d->buffer, (((const char *)data) + (*len)), sent);
		err = dev_write_irq_blk(d, d->buffer, &sent);
		length -= sent;
		*len += sent;
	}

	DD(bug("parallel: dev_write_irq, %ld bytes written\n", *len));

	return err;
}

static status_t dev_write_irq_blk(par_port_info *d, const void *data, size_t *len)
{
	const int io_base = d->io_base;
	const size_t length = *len;
	size_t actual = 0;
	status_t err = B_OK;

	DDD(bug("parallel: dev_write_irq_blk, %ld bytes\n", length));

	/* no byte sent, yet */
	*len = actual;

	/* Check if device is ready */
	err = dev_wait(d);
	if (err != B_OK)
		return err;
	
	/* Set interrupt variables */
	d->data = (uint8 *)data;
	d->length = length;
	d->actual = actual;
	d->irq_mode = IRQMODE_WRITE_ACK;

	while (true)
	{
		d->error = false;
	
		/* Send the 1st byte */
		write_io_8(io_base + PDATA, (d->data)[d->actual]);
		spin(1);	/* "setup" min 0.75us, see "Parallel port complete", page 225 */
		write_io_8(io_base + PCON, C_SELIN | C_INIT | C_IRQEN | C_STROBE);
		spin(1);	/* "nStrobe" min 0.75us, see "Parallel port complete", page 225 */
		write_io_8(io_base + PCON, C_SELIN | C_INIT | C_IRQEN);
	
		/* Wait the end of the tranfert */
		if ((err = wait_transfert_end(d)) == B_OK)
			break;

		D(bug("parallel: ERROR during transfert!\n"));

		/* There was an error, or the device is very slow - give it a chance */
		if ((err = dev_wait(d)) != B_OK)
			break;

		D(bug("parallel: Retrying...\n"));
	}
	
	/* Disable interrupt */
	write_io_8(io_base + PCON, C_SELIN | C_INIT);
	d->irq_mode = IRQMODE_INACTIVE;

	/* Nb of transferred bytes */
	actual = d->actual;

	/* Return number of transmitted bytes */
	*len = actual;
	DDD(bug("parallel: dev_write_irq_blk, %ld bytes written\n", actual));
	return err;
}


/* Method 3: Centronics or ECP transfer via programmed I/O, FIFO and interrupt */
static status_t dev_write_fifo(par_port_info *d, const void *data, size_t *len, bool is_ecp)
{
	status_t err = B_OK;
	size_t length = *len;
	*len = 0;

	DD(bug("parallel: dev_write_fifo, %ld bytes\n", length));

	while ((length) && (err == B_NO_ERROR))
	{
		size_t sent = ((length < TMP_BUFFER_SIZE) ? (length) : (TMP_BUFFER_SIZE));
		memcpy(d->buffer, (((const char *)data) + (*len)), sent);
		err = dev_write_fifo_blk(d, d->buffer, &sent, is_ecp);
		length -= sent;
		*len += sent;
	}

	DD(bug("parallel: dev_write_fifo, %ld bytes written\n", (*len)));

	return err;
}

static status_t dev_write_fifo_blk(par_port_info *d, const void *data, size_t *len, bool is_ecp)
{
	int io_base = d->io_base;
	size_t length = *len;
	size_t actual = 0;
	status_t err = B_OK;

	DDD(bug("parallel: dev_write_fifo_blk %s, %ld bytes\n", is_ecp ? "ecp" : "centronics", length));

	/* no byte sent, yet */
	*len = actual;

	/* Check if device is ready */
	if (!is_ecp)
	{
		err = dev_wait(d);
		if (err != B_OK)
			return err;
	}

	/* If we are in ECP read mode, negotiate forward transfer */
	if (is_ecp && d->ecp_read_mode)
	{
		err = negotiate_ecp_write(d);
		if (err)
			return err;
	}

	/* Set interrupt variables */
	d->error = false;
	d->data = (uint8 *)data;
	d->length = length;
	d->actual = actual;
	d->irq_mode = IRQMODE_FIFO_WRITE;

	/* Switch to FIFO mode, Enable service interrupt & Error interrupt */
	write_io_8(io_base + ECR, (is_ecp ? EC_ECP : EC_ISA_FIFO));

	/* Wait for transfer to be finished */	
	wait_transfert_end(d);

	/* Disable Interrupts */
	write_io_8(io_base + ECR, (is_ecp ? EC_ECP : EC_ISA_FIFO) | EC_ERRINTR | EC_SERVICEINTR);
	d->irq_mode = IRQMODE_INACTIVE;

	/* Nb bytes that were filled into the FIFO */
	actual = d->actual;

	/* Wait for the FIFO to become empty */
	err = wait_fifo_empty(d);
	if (err != B_NO_ERROR)
	{ /* The printer doesn't respond */
		int remaining_bytes_in_fifo = d->fifosize;

		/* Prevent data from being transfered */
		write_io_8(io_base + PCON, C_SELIN | C_INIT | C_STROBE);
		
		/* Push data to the FIFO until it becomes full */
		while (!(read_io_8(io_base + ECR) & EC_FIFOFS))
		{
			write_io_8(io_base + PDATA, 0);
			remaining_bytes_in_fifo--;
		}
		/* Reset the FIFO */
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
		write_io_8(io_base + PCON, C_SELIN | C_INIT);		

		DDD(bug("parallel:  ***** %d bytes remaining in FIFO\n", remaining_bytes_in_fifo));
		actual -= remaining_bytes_in_fifo;
	}
	else
	{
		/* Disable interrupts and FIFO/ECP mode */
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
	}

	/* Return number of transmitted bytes */
	*len = actual;
	DDD(bug("parallel: dev_write_fifo_blk, %ld bytes written\n", actual));
	return err;
}




/* Method 4: Centronics or ECP transfer via DMA and interrupt */
static status_t dev_write_isa_dma(par_port_info *d, const void *data, size_t *len, bool is_ecp)
{
	status_t err = B_OK;
	size_t length = *len;
	*len = 0;

	DD(bug("parallel: dev_write_isa_dma %s, %ld bytes\n", is_ecp ? "ecp" : "centronics", length));

	while ((length) && (err == B_NO_ERROR))
	{
		size_t sent = ((length < DMA_TRANSFERT_SIZE) ? (length) : (DMA_TRANSFERT_SIZE));
		err = dev_write_isa_dma_blk(d, (((const char *)data) + (*len)), &sent, is_ecp);
		length -= sent;
		*len += sent;
	}

	DD(bug("parallel: dev_write_isa_dma, %ld bytes written\n", (*len)));

	return err;
}

static status_t dev_write_isa_dma_blk(par_port_info *d, const void *data, size_t *len, bool is_ecp)
{
	int io_base = d->io_base;
	size_t length = *len;
	size_t	actual = 0;
	status_t err = B_OK;

	DDD(bug("parallel: dev_write_isa_dma_blk %s, %ld bytes\n", is_ecp ? "ecp" : "centronics", length));

	/* no byte sent, yet */
	*len = actual;

	/* Check if device is ready */
	if (!is_ecp)
	{
		err = dev_wait(d);
		if (err != B_OK)
			return err;
	}

	/* If we are in ECP read mode, negotiate forward transfer */
	if (is_ecp && d->ecp_read_mode)
	{
		err = negotiate_ecp_write(d);
		if (err)
		{
			*len = 0;
			return err;
		}
	}

	/* copy the buffer in DMA'able memory */ 
	memcpy(dma.dma_table, data, length);

	/* Set interrupt variables */
	d->error = false;
	d->irq_mode = IRQMODE_DMA;

	/* Start DMA */
	err = start_isa_dma(	d->device_dma.dma_channel,
							dma.dma_table_phys,
							length,
							0x08,
							0x00
						);
	if (err)
	{
		D(bug("parallel: start_isa_dma returned %d\n", err));
		*len = 0;
		d->irq_mode = IRQMODE_INACTIVE;
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
		return err;
	}

	/* Switch to FIFO mode, enable DMA and service interrupt */
	write_io_8(io_base + ECR, (is_ecp ? EC_ECP : EC_ISA_FIFO) | EC_DMAEN | EC_ERRINTR | EC_SERVICEINTR);
	write_io_8(io_base + ECR, (is_ecp ? EC_ECP : EC_ISA_FIFO) | EC_DMAEN);

	/* Wait for transfer to be finished */	
	err = wait_transfert_end(d);

	/* Disable interrupts and DMA */
	write_io_8(io_base + ECR, (is_ecp ? EC_ECP : EC_ISA_FIFO) | EC_ERRINTR | EC_SERVICEINTR);
	d->irq_mode = IRQMODE_INACTIVE;

	if (err != B_OK)
	{
		int pcount;
		int pstatus;
		#ifdef __INTEL__
		get_dma_count_and_status(d->device_dma.dma_channel, &pcount, &pstatus);
		#else
		pcount = 0xFFFF;	// in PPC we don't know were the DMA is.
		#endif
		actual = length - ((pcount+1)&0xFFFF);
		DDD(bug("parallel:  ***** %d bytes remaining in DMA\n", pcount));
	}
	else
	{
		actual = length;
	}

	/* Wait for the FIFO to become empty */
	err = wait_fifo_empty(d);
	if (err != B_NO_ERROR)
	{ /* The printer doesn't respond */
		int remaining_bytes_in_fifo = d->fifosize;

		/* Prevent data from being transfered */
		write_io_8(io_base + PCON, C_SELIN | C_INIT | C_STROBE);
		
		/* Push data to the FIFO until it becomes full */
		while (!(read_io_8(io_base + ECR) & EC_FIFOFS))
		{
			write_io_8(io_base + PDATA, 0);
			remaining_bytes_in_fifo--;
			if (remaining_bytes_in_fifo < 0)
			{ /* -GASP!!- How is it possible ?? - Hardware problem, maybe */
				DDD(bug("parallel:  ***** The FIFO stay empty as we are filling it!\n"));
				remaining_bytes_in_fifo = 0; /* Give it a chance to retry, by assuming the fifo, was actually empty */
				break;
			}
		}
		/* Reset the FIFO */
		write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);
		write_io_8(io_base + PCON, C_SELIN | C_INIT);		

		DDD(bug("parallel:  ***** %d bytes remaining in FIFO\n", remaining_bytes_in_fifo));
		actual -= remaining_bytes_in_fifo;
	}

	/* Return to PS2 mode  */
	write_io_8(io_base + ECR, EC_PS2 | EC_ERRINTR | EC_SERVICEINTR);

	/* Return number of transmitted bytes */
	*len = actual;
	DDD(bug("parallel: dev_write_isa_dma_blk, %ld bytes written\n", actual));
	return err;
}


/* dev_write() */
static status_t dev_write(void *cookie, off_t position, const void *data, size_t *len)
{
	par_port_info *d = ((user_info *)cookie)->d;
	int32 open_mode = ((user_info *)cookie)->open_mode;
	size_t length;
	status_t err;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_write(%08lx, %lu, %08lx, %lu)\n", cookie, (uint32)position, data, (uint32)(*len)));

	if ((open_mode & M_WRONLY_MASK) == 0)
		return EINVAL;

	/* Nothing to transfer? Then return */
	length = *len;
	if (length == 0)
		return B_OK;

	/* Acquire access to parallel port */
	err = acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);
	if (err)
	{
		*len = 0;
		return err;
	}

	/* Select transmission method */
	if (d->mode == PAR_MODE_ECP)
	{
		if (atomic_or(&dma.dma_in_use, 1) == 0)
		{ /* Get DMA channel */
			err = dev_write_isa_dma(d, data, len, true);
			atomic_and(&dma.dma_in_use, ~1);
		}
		else
		{ /* Otherwise use interrupt and FIFO */
			err = dev_write_fifo(d, data, len, true);
		}
	}
	else
	{
		err = negotiate_ieee1284(d, IEEE1284_COMPATIBILITY);
		if (err == B_NO_ERROR)
		{ /* Centronics transfer */

#if 0
//d->type &= ~PAR_TYPE_ECP;	/* -> IRQ transfert */
//d->type &= ~PAR_TYPE_IRQ;	/* -> PIO transfert */
//dma.dma_in_use = 1;			/* -> FIFO tranfert */
#endif

			if (!(d->type & PAR_TYPE_IRQ))
			{ /* No interrupt available, use programmed I/O */
				err = dev_write_pio(d, data, len);
			}
			else
			{
				if (d->type & PAR_TYPE_ECP)
				{
					if (atomic_or(&dma.dma_in_use, 1) == 0)
					{ /* Get DMA channel */
						err = dev_write_isa_dma(d, data, len, false);
						atomic_and(&dma.dma_in_use, ~1);
					}
					else
					{ /* Otherwise use interrupt and FIFO */
						err = dev_write_fifo(d, data, len, false);
					}
				}
				else
				{ /* No ECP port, use programmed I/O and interrupt */
					/* **** WARNING **** Don't use the IRQ mode anymore, it's too heavy for the kernel */
					/* err = dev_write_irq(d, data, len); */
					err = dev_write_pio(d, data, len);
				}
			}
		}
	}

	/* Free parallel port */
	release_sem(d->hw_lock);
	return err;
}

/*-----------------------------------------------------------------*/
#pragma mark -

/*
 *  Interrupt handler
 */

static int32 dev_irq_handler(void *data)
{
	par_port_info *d = (par_port_info *)data;
	const int32 io_base = d->io_base;
	uint8 ecr;
	uint8 stat;

	/* Read status registers */
	ecr = (d->type & PAR_TYPE_ECP) ? read_io_8(io_base + ECR) : 0;
	stat = read_io_8(io_base + PSTAT);

	/* Act according to IRQ handler mode */
	switch (d->irq_mode)
	{
		case IRQMODE_INACTIVE:
			return B_UNHANDLED_INTERRUPT;

		case IRQMODE_WRITE_ACK:
			/* The device has accepted the last byte and is ready for another one */
			d->actual++;
			if (d->actual < d->length)
			{
				/* Must wait BUSY going low. max = 10+2.5 us, see page 225 */				
				bigtime_t timeout_time = system_time() + 13;
				do
				{
					const int mask	= S_BUSY | S_SEL | S_FAULT | S_PERR;	/* STAT register OK */
					const int val	= S_BUSY | S_SEL | S_FAULT;
					const int emask	= S_SEL | S_FAULT | S_PERR;				/* STAT register ERROR */
					const int eval	= S_SEL | S_FAULT;
					if ((stat & mask) == val)
					{ /* Transmit byte with strobe */
						write_io_8(io_base + PDATA, (d->data)[d->actual]);
						spin(1);	/* "setup" min 0.75us, see "Parallel port complete", page 225 */
						write_io_8(io_base + PCON, C_SELIN | C_INIT | C_IRQEN | C_STROBE);
						spin(1);	/* "nStrobe" min 0.75us, see "Parallel port complete", page 225 */
						write_io_8(io_base + PCON, C_SELIN | C_INIT | C_IRQEN);
						return B_HANDLED_INTERRUPT;
					}
					else if ((stat & emask) != eval)
					{
						d->error = true;
						return B_HANDLED_INTERRUPT;
					}
					stat = read_io_8(io_base + PSTAT);
				} while (system_time() < timeout_time);

				/* Here, there's an error. We received the nACK, but the printer is still busy! */
				/* This device is maybe very slow. And we can't wait any longer in the IRQ service routine. */
				/* Return an error (the xfert will be retried) */
				d->error = true;
				release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT;
			}

			d->irq_mode = IRQMODE_INACTIVE;
			write_io_8(io_base + PCON, C_SELIN | C_INIT);		/* disable interrupts */
			release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_HANDLED_INTERRUPT;

		case IRQMODE_FIFO_WRITE:
			/* FIFO write mode: Write data to FIFO, signal main thread when transfer is complete */
			if ((stat & (S_PERR | S_SEL | S_FAULT)) != (S_SEL | S_FAULT))
			{
				/* There is an IO error */
				d->error = true;
				write_io_8(io_base + ECR, ecr | EC_ERRINTR | EC_SERVICEINTR);	/* Clear interrupt */
				release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT; 
			} 

			if (ecr & EC_SERVICEINTR)
			{
				const int nb = min_c(d->fifosize, (d->length - d->actual));
				if  (nb > 0)
				{
					/* Send remaining data  */					
					int i;
					const int io_sdfifo = io_base + SDFIFO;
					const int io_ecr = io_base + ECR;

					if (read_io_8(io_ecr) & EC_FIFOES)
					{
						/* If the FIFO is empty, we can fill it without testing the FIFOFS flag */
						for (i=0 ; i<nb ; i++)
							write_io_8(io_sdfifo, *(d->data)++);
					}
					else
					{
						/* FIFO is not empty, we must check the FIFOFS flag */
						for (i=0 ; i<nb ; i++)
						{
							if (read_io_8(io_ecr) & EC_FIFOFS)
								break;
							write_io_8(io_sdfifo, *(d->data)++);
						}
					}

					d->actual += i;
					write_io_8(io_base + ECR, ecr & ~EC_SERVICEINTR);	/* Re-enable interrupt */
					return B_HANDLED_INTERRUPT;
				}
				/* Transfert finished: signal the main thread */
				write_io_8(io_base + ECR, ecr | EC_ERRINTR);			/* Clear interrupts */
				release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT;
			}
			return B_UNHANDLED_INTERRUPT;

		case IRQMODE_FIFO_READ:
			/* FIFO read mode (ECP only): Read data from FIFO, signal main thread when transfer is complete */
			if (ecr & EC_SERVICEINTR)
			{
 				int nb = min_c(d->fifosize, d->length);
 				while (nb && !(read_io_8(io_base + ECR) & EC_FIFOES))
				{
					*(d->data)++ = read_io_8(io_base + SDFIFO);
					d->length--;
					d->actual++;
					nb--;
				}

				if (!d->length)
				{
					write_io_8(io_base + ECR, ecr | EC_ERRINTR | EC_SERVICEINTR);	/* Clear interrupt */
					d->irq_mode = IRQMODE_INACTIVE;
					release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				}
				else
				{
					write_io_8(io_base + ECR, ecr & ~EC_SERVICEINTR);	/* Re-enable interrupt */
				}
				return B_HANDLED_INTERRUPT;
			}
			return B_UNHANDLED_INTERRUPT;

		case IRQMODE_DMA:
			if ((stat & (S_PERR | S_SEL | S_FAULT)) != (S_SEL | S_FAULT))
			{
				/* There is an IO error */
				d->error = true;
				write_io_8(io_base + ECR, ecr | EC_ERRINTR | EC_SERVICEINTR);	/* Clear interrupt */
				release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT; 
			} 

			if (ecr & EC_SERVICEINTR)
			{
				d->irq_mode = IRQMODE_INACTIVE;
				release_sem_etc(d->irq_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT;
			}
			return B_UNHANDLED_INTERRUPT;
	}

	return B_UNHANDLED_INTERRUPT;
}
