#include <IDE.h>
#include <KernelExport.h>
#include <ide_device_info.h>
#include <Drivers.h>
#include <Mime.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ata_devicelist.h"
#include <ide_settings.h>
#include <block_io.h>
#include <scsi.h>
#include <driver_settings.h>

#if __INTEL__
#include <biosdriveinfo.h>
#endif

#if _BUILDING_kernel || BOOT
#include <drivers_priv.h>
#endif

#if defined(__MWERKS__)
#define UNUSED(x)
#define UNUSED_ARG(x)
#else
#define UNUSED(x) x
#define UNUSED_ARG(x) x = x;
#endif

extern unsigned char icon_disk[];
extern unsigned char icon_disk_mini[];
extern unsigned char icon_removable[];
extern unsigned char icon_removable_mini[];

const bigtime_t int_timeout = 10000000;
const bigtime_t removable_int_timeout = 15000000;

#define ATA_CMD_READ		0x20
#define ATA_CMD_WRITE		0x30
#define ATA_CMD_READ_DMA	0xc8
#define ATA_CMD_WRITE_DMA	0xca

ide_module_info *ide;

static bool default_dma_enable; 

/*------------------------------------------------------------------------------
** debug output
*/

#include <debugoutput.h>

int debug_level_error = 3;
int debug_level_info = 1;
int debug_level_flow = 0;

static status_t
read_settings_global()
{
	const char *debugprefix = DEBUGPREFIX "read_settings_global:";
	const char *drivername = "ata";
	void *settingshandle;
	const driver_settings *s;

	settingshandle = load_driver_settings(drivername);
	if(settingshandle == 0)
		return B_ERROR;
	s = get_driver_settings(settingshandle);
	if(s) {
		int i;
		for(i = 0; i < s->parameter_count; i++) {
			driver_parameter *gp = &s->parameters[i];
			if(strcmp(gp->name, "global") == 0) {
				int j;
				for(j = 0; j < gp->parameter_count; j++) {
					driver_parameter *p = &gp->parameters[j];
					if(strcmp(p->name, "debug_level_error") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_error takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_error = strtol(p->values[0], NULL, 10);
					}
					else if(strcmp(p->name, "debug_level_info") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_info takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_info = strtol(p->values[0], NULL, 10);
					}
					else if(strcmp(p->name, "debug_level_flow") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_flow takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_flow = strtol(p->values[0], NULL, 10);
					}
					else {
						DE(3) dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
					}
				}
			}
		}
	}
	unload_driver_settings(settingshandle);
	return B_NO_ERROR;
}

/*------------------------------------------------------------------------------
** wait_status predicates
*/

static
status_t drdy_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t drdy_timeout)
{
	if ((status & (ATA_BSY|ATA_DRDY)) == ATA_DRDY)
		return B_NO_ERROR;
	if ((sample_time - start_time) > drdy_timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

static
status_t drq_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t drq_timeout)
{
	if (status & ATA_ERR) {
		DE(2) dprintf("IDE ATA -- drq_p: error: status = 0x%02x\n", status);
		return B_ERROR;
	}
	if (status & ATA_DRQ)
		return B_NO_ERROR;
	if ((sample_time - start_time) > drq_timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

static
status_t drq_down_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t drq_timeout)
{
	if (status & ATA_ERR) {
		DE(2) dprintf("IDE ATA -- drq_down_p: error: status = 0x%02x\n", status);
		return B_ERROR;
	}
	if ((status & ATA_DRQ) == 0)
		return B_NO_ERROR;
	if ((sample_time - start_time) > drq_timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

/*------------------------------------------------------------------------------
** CFA feature set functions
*/

static status_t
cfa_request_extended_error_code(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- cfa_request_extended_error_code:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	tf->write.command = 0x03;	/* CFA request_extended_error_code */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s send command failed\n", debugprefix);
		goto err;
	}
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	err = B_ERROR;
	bus->read_command_block_regs(buscookie, tf, ide_mask_error);
	if(status & ATA_ERR) {
		if(tf->read.error & 0x04) {
			DE(2) dprintf("%s not supported\n", debugprefix);
		}
		else {
			DE(2) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		}
	}
	else {
		switch(tf->read.error) {
			case 0x27: /* Write protect violation */
				DE(2) dprintf("%s device is write protected\n",
				              debugprefix);
				err = B_READ_ONLY_DEVICE;
				break;
			default:
				DE(2) dprintf("%s extended error code 0x%02x\n",
				              debugprefix, tf->read.error);
		}
	}
err:
	return err;
}

/*------------------------------------------------------------------------------
** removable media feature set functions
*/

static status_t
get_media_status(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- get_media_status:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	err = acquire_bus(d);
	if (err != B_NO_ERROR) {
		return err;
	}
	err = h->status;
	if(err != B_NO_ERROR) {
		h->status = B_NO_ERROR;
		goto err;
	}

	if(d->di->removable_media.media_status_notification_supported != 1)
		goto err; /* err == B_NO_ERROR */

	if(d->priv->media_change_requested) {
		DI(1) dprintf("%s report stored media change requested\n", debugprefix);
		d->priv->media_change_requested = false;
		err = B_DEV_MEDIA_CHANGE_REQUESTED;
		goto err;
	}
	tf->write.command = 0xda;	/* GET MEDIA STATUS */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s send command failed\n", debugprefix);
		goto err;
	}
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x40) {
			DI(1) dprintf("%s media write protected\n", debugprefix);
		}
		if(tf->read.error & 0x02) {
			DI(2) dprintf("%s no media\n", debugprefix);
			err = B_DEV_NO_MEDIA;
			goto err;
		}
		if(tf->read.error & 0x08) {
			DI(1) dprintf("%s media change requested\n", debugprefix);
			err = B_DEV_MEDIA_CHANGE_REQUESTED;
			goto err;
		}
		if(tf->read.error & 0x20) {
			DI(1) dprintf("%s media changed\n", debugprefix);
			err = B_DEV_MEDIA_CHANGED;
			media_changed_in_lun(d, 0);
			h->status = B_NO_ERROR;
			goto err;
		}
		if(tf->read.error & 0x04) {
			DI(1) dprintf("%s not supported\n", debugprefix);
			err = B_NOT_ALLOWED;
			goto err;
		}
		DE(2) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		err = B_ERROR;
		goto err;
	}
err:
	release_bus(d);
	DI(2) dprintf("%s %s\n", debugprefix, strerror(err));
	return err;
}

static status_t
media_eject(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- media_eject:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	err = acquire_bus(d);
	if (err != B_NO_ERROR) {
		return err;
	}
	err = h->status;
	if(err != B_NO_ERROR)
		goto err;
	tf->write.command = 0xed;	/* MEDIA EJECT */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s media eject command failed\n", debugprefix);
		goto err;
	}
	DF(3) dprintf("%s int_wait\n", debugprefix);
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x02) {
			DE(2) dprintf("%s no media\n", debugprefix);
			err = B_DEV_NO_MEDIA;
			goto err;
		}
		if(tf->read.error & 0x04) {
			DE(2) dprintf("%s not supported\n", debugprefix);
			err = B_NOT_ALLOWED;
			goto err;
		}
		DE(2) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		err = B_ERROR;
		goto err;
	}
err:
	release_bus(d);
	return err;
}

/*------------------------------------------------------------------------------
** configuration functions
*/

#if 0

static status_t
set_transfer_mode(device_handle *h, uint8 modebits)
{
	const char		*debugprefix = "IDE ATA -- set_transfer_mode:";
	device_entry	*d = h->d;
	ide_task_file	*tf = d->tf;
	ide_bus_info	*bus = d->bus;
	uint32			buscookie = d->buscookie;
	uint8			status;
	status_t		err;
	
	DI(1) dprintf("%s set transfer mode 0x%02x\n", debugprefix, modebits);
	tf->write.features = 0x03;	/* set transfer mode */
	tf->write.sector_count = modebits;
	tf->write.command = 0xef;	/* set features */
	
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s set transfer mode failed\n", debugprefix);
		goto err;
	}
	DF(3) dprintf("%s int_wait\n", debugprefix);
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x04) {
			DE(2) dprintf("%s not supported\n", debugprefix);
			err = B_NOT_ALLOWED;
			goto err;
		}
		DE(2) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		err = B_ERROR;
		goto err;
	}
err:
	return err;
}

#endif

#if 0
static status_t
check_power_mode(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- check_power_mode:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	tf->write.command = 0xe5;	/* FLUSH CACHE */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s flush cache command failed\n", debugprefix);
		goto err;
	}
	DF(3) dprintf("%s int_wait\n", debugprefix);
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		DE(4) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		err = B_ERROR;
		goto err;
	}
	d->bus->read_command_block_regs(d->buscookie, tf, ide_mask_sector_count);
	DI(1) dprintf("%s power mode: 0x%02x\n", debugprefix, tf->read.sector_count);
err:
	return err;
}
#endif

/*------------------------------------------------------------------------------
** io functions
*/

static status_t
flush_cache(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- flush_cache:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	if(d->di->command_set_feature_supported.valid != 1 ||
	   d->di->command_set_feature_supported.write_cache == 0) {
		DF(1) dprintf("%s no write cache\n", debugprefix);
		return B_NOT_ALLOWED;
	}
	DF(1) dprintf("%s flushing...\n", debugprefix);

	err = acquire_bus(d);
	if (err != B_NO_ERROR) {
		return err;
	}
	err = h->status;
	if(err != B_NO_ERROR)
		goto err;
	tf->write.command = 0xe7;	/* FLUSH CACHE */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s flush cache command failed\n", debugprefix);
		goto err;
	}
	DF(3) dprintf("%s int_wait\n", debugprefix);
	err = bus->intwait(buscookie, removable_int_timeout);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		DE(4) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
		err = B_ERROR;
		goto err;
	}
err:
	release_bus(d);
	return err;
}

/*
** setup task file for read or write
*/

static status_t
setup_read_write(ide_task_file *tf, ide_device_info *di,
	long sect, int nsect, char command)
{
	const char *debugprefix = "IDE ATA -- setup_read_write:";
	if (nsect > 256)
		return B_ERROR;

	if(tf->lba.mode == IDE_LBA_MODE) {
		DF(2) dprintf("%s LBA mode\n", debugprefix); 
		tf->lba.lba_0_to_7   = sect;
		tf->lba.lba_8_to_15  = sect >> 8;
		tf->lba.lba_16_to_23 = sect >> 16;
		tf->lba.lba_24_to_27 = sect >> 24;
		tf->lba.sector_count = nsect;
		tf->lba.command = command;
	}
	else if(tf->chs.mode == IDE_CHS_MODE) {
		uint32 cyl, sect_in_cyl, sectors_per_cylinder;

		DF(2) dprintf("%s CHS mode\n", debugprefix); 
		
		sectors_per_cylinder = di->current_heads * di->current_sectors_per_track;
		if(sectors_per_cylinder <= 0) {
			DE(1) dprintf("%s invalid geometry\n", debugprefix); 
			return B_ERROR;
		}
		cyl = sect / sectors_per_cylinder;
		sect_in_cyl = sect - (cyl * sectors_per_cylinder);

		tf->chs.cylinder_0_to_7  = cyl;
		tf->chs.cylinder_8_to_15 = cyl >> 8;
		tf->chs.head = sect_in_cyl / di->current_sectors_per_track;
		tf->chs.sector_number = (sect_in_cyl % di->current_sectors_per_track) + 1;
		tf->chs.sector_count = nsect;
		tf->chs.command = command;
	}
	else {
		DE(1) dprintf("%s Invalid mode\n", debugprefix); 
		return B_ERROR;
	}
	return B_NO_ERROR;
}

/*
** check if the read or write operation failed
*/

static status_t
check_read_error(device_handle *h, bool expect_data)
{
	const char *debugprefix = "IDE ATA -- check_read_error:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	uint8 status;
	
	spin(1);
	status = d->bus->get_altstatus(d->buscookie);

	if((status & (ATA_BSY|ATA_DRQ|ATA_ERR)) == ATA_ERR) {
		d->bus->read_command_block_regs(d->buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x02)
			return B_DEV_NO_MEDIA;
		if(tf->read.error & 0x08) {
			DI(1) dprintf("%s media change request\n", debugprefix);
			//return B_DEV_MEDIA_CHANGE_REQUESTED;
			return B_INTERRUPTED;
		}
		if(tf->read.error & 0x10) {
			DE(2) dprintf("%s drive seek error\n", debugprefix);
			return B_DEV_SEEK_ERROR;
		}
		if(tf->read.error & 0x20) {
			media_changed_in_lun(h->d, 0);
			return B_DEV_MEDIA_CHANGED;
		}
		if(tf->read.error & 0x40)
			return B_DEV_READ_ERROR;
		if(tf->read.error & 0x04) {
			DE(2) dprintf("%s drive aborted\n", debugprefix);
			return B_IO_ERROR;
		}
		if(d->di->command_set_feature_supported.cfa)
			return cfa_request_extended_error_code(h);
		DE(2) dprintf("%s read error 0x%02x\n", debugprefix, tf->read.error);
		return B_IO_ERROR;
	}
	if(expect_data) {
		if ((status & (ATA_BSY|ATA_DRQ)) != ATA_DRQ) {
			DE(2) dprintf("%s data not ready ever on %ld/%ld\n", debugprefix,
			              d->busnum, d->devicenum);
			if (status & ATA_BSY)
				ide->reset(d->bus, d->buscookie);
			return B_IO_ERROR;
		}
	}
	else {
		if(status & (ATA_BSY|ATA_DRQ)) {
			DE(2) dprintf("%s read error - status = 0x%02x\n",
			              debugprefix, status);
			ide->reset(d->bus, d->buscookie);
			return B_IO_ERROR;
		}
	}
	return B_NO_ERROR;
}

static status_t
check_write_error(device_handle *h, bool may_have_more_data)
{
	const char *debugprefix = "IDE ATA -- check_write_error:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	uint8 status;

	spin(1);
	status = d->bus->get_altstatus(d->buscookie);

	if((status & (ATA_BSY|ATA_DRQ|ATA_ERR)) == ATA_ERR) {
		d->bus->read_command_block_regs(d->buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x02)
			return B_DEV_NO_MEDIA;
		if(tf->read.error & 0x08) {
			DI(1) dprintf("%s media change request\n", debugprefix);
			//return B_DEV_MEDIA_CHANGE_REQUESTED;
			return B_INTERRUPTED;
		}
		if(tf->read.error & 0x10) {
			DE(2) dprintf("%s drive seek error\n", debugprefix);
			return B_DEV_SEEK_ERROR;
		}
		if(tf->read.error & 0x20) {
			media_changed_in_lun(h->d, 0);
			return B_DEV_MEDIA_CHANGED;
		}
		if(tf->read.error & 0x40)
			return B_READ_ONLY_DEVICE;
		if(tf->read.error & 0x04) {
			DE(2) dprintf("%s drive aborted\n", debugprefix);
			return B_IO_ERROR;
		}
		if(d->di->command_set_feature_supported.cfa)
			return cfa_request_extended_error_code(h);
		DE(2) dprintf("%s write error 0x%02x\n", debugprefix, tf->read.error);
		return B_ERROR;
	}
	if(may_have_more_data) {
		if ((status & (ATA_BSY|ATA_DRQ)) != ATA_DRQ) {
			DE(2) dprintf("%s data not ready ever on %ld/%ld\n",
			              debugprefix, d->busnum, d->devicenum);
			if (status & ATA_BSY)
				ide->reset(d->bus, d->buscookie);
			return B_IO_ERROR;
		}
	}
	else
	{
		if(status & (ATA_BSY|ATA_DRQ)) {
			DE(2) dprintf("%s write error - status = 0x%02x\n",
			              debugprefix, status);
			ide->reset(d->bus, d->buscookie);
			return B_IO_ERROR;
		}
	}
	return B_NO_ERROR;
}

/*
** dma_transfer does the actual read/write when using dma
*/

static status_t
dma_transfer(device_handle *h, long sect, int nsect,
             const iovec *vec, size_t veccount, uint32 startbyte, bool write)
{
	const char *debugprefix = "IDE ATA -- dma_transfer:";
	const char *dirstring = write ? "write" : "read";
	device_entry	*d = h->d;
	ide_task_file	*tf = d->tf;
	ide_bus_info	*bus = d->bus;
	uint32 			buscookie = d->buscookie;
	status_t		err, tmperr;
	int				retry_count;
	const int		max_retry_count = 2;

	uint32	remaining_sectors = nsect;
	uint32	start_sector = sect;
	while(remaining_sectors > 0) {
		size_t	transfersize = remaining_sectors * 512;
		uint32	transfer_sectors;

		retry_count = 0;
retry_dma_transfer:
		err = bus->prepare_dma(buscookie, vec, veccount, startbyte,
		                       512, &transfersize, write);
		if(err != B_NO_ERROR) {
			if(err != B_NOT_ALLOWED)
				DE(3) dprintf("%s prepare_dma failed\n", debugprefix);
			return err;
		}
		transfer_sectors = transfersize / 512;
		DF(2) dprintf("%s DMA transfer, %ld of %ld sectors starting at %ld\n",
		              debugprefix, transfer_sectors,
		              remaining_sectors, start_sector);
		err = setup_read_write(tf, d->di, start_sector, transfer_sectors,
		                       write? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s couldn't setup dma %s.\n",
			              debugprefix, dirstring);
			goto dma_err;
		}
		DF(2) dprintf("%s send_ata\n", debugprefix);
		err = ide->send_ata(bus, buscookie, tf, true);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s send dma %s command failed\n",
			              debugprefix, dirstring);
			goto dma_err;
		}
		err = bus->start_dma(buscookie);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s could not start dma %s\n",
			              debugprefix, dirstring);
			goto dma_err;
		}
		DF(2) dprintf("%s int_wait\n", debugprefix);
		err = bus->intwait(buscookie, int_timeout);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s dma intwait failed (status 0x%02x)\n", 
			              debugprefix, bus->get_altstatus(buscookie));
			goto dma_err;
		}
dma_err:
		tmperr = bus->finish_dma(buscookie);
		if(err != B_NO_ERROR) {
		    if(retry_count < max_retry_count) {
				retry_count++;
				DE(2) dprintf("%s retry dma %s\n", debugprefix, dirstring);
				ide->wait_status(bus, buscookie, drdy_p, 5000000);
				goto retry_dma_transfer;
			}
			ide->reset(bus, buscookie);
			return err;
		}
		if(write)
			err = check_write_error(h, false);
		else
			err = check_read_error(h, false);
		if(tmperr == B_INTERRUPTED) {
			DE(2) dprintf("%s did not complete dma transfer\n",
			              debugprefix);
			if(err == B_NO_ERROR)
				err = B_IO_ERROR;
		}
		else if(tmperr != B_NO_ERROR) {
			DE(2) dprintf("%s finish_dma failed\n", debugprefix);
			if(err == B_NO_ERROR)
				err = B_IO_ERROR;
		}
		if(err == B_INTERRUPTED) {
			DE(2) dprintf("%s interrupted, retry dma %s\n",
			              debugprefix, dirstring);
			goto retry_dma_transfer;
		}
		if((err == B_DEV_READ_ERROR || err == B_IO_ERROR) &&
		    retry_count++ < max_retry_count) {
			DE(2) dprintf("%s retry dma %s\n", debugprefix, dirstring);
			goto retry_dma_transfer;
		}
		if(err != B_NO_ERROR) {
			return err;
		}

		remaining_sectors -= transfer_sectors;
		start_sector += transfer_sectors;
		startbyte += transfer_sectors*512;
	}
	
	return B_NO_ERROR;
}

/*
** pio_vec does the pio transfer to an iovec
*/

static status_t
pio_vec(device_handle *h, const iovec *vec, size_t veccount,
        uint32 startbyte, bool write)
{
	device_entry	*d = h->d;
	ide_bus_info	*bus = d->bus;
	uint32 			buscookie = d->buscookie;

	uint32	bytesleft = 512;
	uint8	*buf;
	uint32	transfersize;
	uint8	tmpbuf[2];
	bool	remainder = false;
	
	while(bytesleft > 0) {
		while(veccount > 0 && vec->iov_len <= startbyte) {
			startbyte -= vec->iov_len;
			vec++;
			veccount--;
		}
		if(veccount == 0) {
			return B_BAD_VALUE;
		}
		buf = (uint8*)vec->iov_base+startbyte;
		transfersize = min(vec->iov_len-startbyte, bytesleft);
		if(write) {
			if(remainder) {
				remainder = false;
				tmpbuf[1] = buf[0];
				transfersize = 1;
				bus->write_pio_16(buscookie, (uint16*)tmpbuf, 1);
			}
			else {
				bus->write_pio_16(buscookie, (uint16*)buf, transfersize/2);
				if(transfersize&1) {
					tmpbuf[0] = buf[transfersize-1];
					remainder = true;
				}
			}
		}
		else {
			if(remainder) {
				remainder = false;
				buf[0] = tmpbuf[1];
				transfersize = 1;
			}
			else {
				bus->read_pio_16(buscookie, (uint16*)buf, transfersize/2);
				if(transfersize&1) {
					bus->read_pio_16(buscookie, (uint16*)tmpbuf, 1);
					buf[transfersize-1]=tmpbuf[0];
					remainder = true;
				}
			}
		}
		startbyte += transfersize;
		bytesleft -= transfersize;
	}
	return B_NO_ERROR;
}

/*
** pio_transfer does the actual read/write when not using dma
*/

static status_t
pio_transfer(device_handle *h, long sect, int nsect,
             const iovec *vec, size_t veccount, uint32 startbyte, bool write)
{
	const char *debugprefix = "IDE ATA -- pio_transfer:";
	const char *dirstring = write ? "write" : "read";
	device_entry	*d = h->d;
	ide_task_file	*tf = d->tf;
	ide_bus_info	*bus = d->bus;
	uint32 			buscookie = d->buscookie;
	status_t		err;
	int				retry_count;
	const int		max_retry_count = 2;

	retry_count = 0;

retry_transfer:
	err = setup_read_write (tf, d->di, sect, nsect,
	                        write ? ATA_CMD_WRITE : ATA_CMD_READ);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s couldn't setup %s.\n", debugprefix, dirstring);
		return err;
	}

	err = ide->send_ata(bus, buscookie, tf, true);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s send command failed\n", debugprefix);
		return err;
	}

	while (nsect > 0) {
		while(veccount > 0 && vec->iov_len <= startbyte) {
			startbyte -= vec->iov_len;
			vec++;
			veccount--;
		}
		if(veccount == 0) {
			return B_BAD_VALUE;
		}

		if(write) {
			err = ide->wait_status(bus, buscookie, drq_p, int_timeout);
			if (err != B_NO_ERROR){
				DE(2) dprintf("%s device %ld on bus %ld not ready!\n",
				              debugprefix, d->devicenum, d->busnum);
				return err;
			}
	
			err = pio_vec(h, vec, veccount, startbyte, write);
			if(err != B_NO_ERROR)
				return err;
		}

		err = bus->intwait(buscookie, int_timeout);
		if (err != B_NO_ERROR) {
			if(retry_count++ < max_retry_count) {
				DE(2) dprintf("%s intwait failed, retry %s\n",
				              debugprefix, dirstring);
				goto retry_transfer;
			}
			else {
				DE(2) dprintf("%s intwait failed\n", debugprefix);
				return err;
			}
		}

		if(write)
			err = check_write_error(h, nsect > 1);
		else
			err = check_read_error(h, true);
		if(err == B_INTERRUPTED) {
			DE(2) dprintf("%s interrupted, retry %s\n", debugprefix, dirstring);
			goto retry_transfer;
		}
		if((err == B_DEV_READ_ERROR || err == B_IO_ERROR) &&
		    retry_count++ < max_retry_count) {
			DE(2) dprintf("%s retry %s\n", debugprefix, dirstring);
			goto retry_transfer;
		}
		if(err != B_NO_ERROR)
			return err;

		if(!write) {
			err = pio_vec(h, vec, veccount, startbyte, write);
			if(err != B_NO_ERROR)
				return err;
		}

		startbyte += 512;
		sect++;
		nsect--;
	}
	if(!write) {
		err = ide->wait_status(bus, buscookie, drq_down_p, 1000000);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s %ld/%ld is sending more than one sector/interrupt"
			              "\n                       status: 0x%02x\n",
			              debugprefix, d->busnum, d->devicenum,
			              bus->get_altstatus(buscookie));
		}
	}
	if(bus->intwait(buscookie, 1) == B_NO_ERROR) {
		DE(2) dprintf("%s cleared interrupt on exit\n", debugprefix);
	}
	return err;
}

/*
** transfer sectors using dma or pio
*/

static status_t
transfer_sectors(device_handle *h, long sect, int nsect, const iovec *vec,
                 size_t veccount, uint32 startbyte, bool write)
{
	const char *debugprefix = "IDE ATA -- transfersectors:";
	device_entry	*d = h->d;
	status_t		err;
	status_t		dmaerr;

	if(d->di->capabilities.dma_supported && d->priv->dma_enabled) {
		err = dma_transfer(h, sect, nsect, vec, veccount, startbyte, write);
		if(err == B_DEV_MEDIA_CHANGED || err == B_DEV_NO_MEDIA)
			return err;
		if(err != B_NO_ERROR) {
#if 0
			check_power_mode(h);
#endif
			DE(2) dprintf("%s dma read failed, try pio\n", debugprefix);
			dmaerr = err;
			err = pio_transfer(h, sect, nsect, vec, veccount, startbyte, write);
			if(err == B_NO_ERROR) {
				if(dmaerr != B_NOT_ALLOWED) {
					DE(2) dprintf("%s DMA disabled\n", debugprefix);
					d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_FAILURE;
				}
				else {
					d->priv->dma_status = B_IDE_DMA_STATUS_CONTROLLER_DISABLED;
				}
				d->priv->dma_enabled = false;
			}
		}
	}
	else {
		err = pio_transfer(h, sect, nsect, vec, veccount, startbyte, write);
	}
	return err;
}

/*------------------------------------------------------------------------------
** Device handling
*/

bool
is_supported_device(ide_device_info *di)
{
	if(di->command_set_feature_supported.valid != 1) {
		di->command_set_feature_supported.cfa = 0;
	}
	if (di->general_configuration.protocol_type >= 2) {
		if(*((uint16*)di) == 0x848a) {
			DI(1) dprintf("IDE ATA -- is_supported_device: "
			              "found solid state memory device\n");
			*((uint16*)di) = 0; // clear removable flag
			
			di->command_set_feature_supported.cfa = 1;
			return true;
		}
		if(*((uint16*)di) == 0x844a) {
			DI(1) dprintf("IDE ATA -- is_supported_device: "
			              "found buggy solid state memory device\n");
			*((uint16*)di) = 0; // clear removable flag
			
			di->command_set_feature_supported.cfa = 1;
			return true;
		}
		DE(4) dprintf("IDE ATA -- is_supported_device: "
		              "device found is not ATA, (iw0: 0x%04x)\n",
		              *((uint16*)di));
		return false;
	}
	return true;
}

#if __INTEL__
static uint32 get_checksum(device_handle *h, uint32 sector, uint32 num)
{
	device_entry *d = h->d;
	uint32	drive_sum;
	status_t err;
	uint32 *buffer;
	iovec	tmpvec;
		
	err = acquire_bus(d);
	if (err != B_NO_ERROR)
		goto err;
	buffer = malloc(num * 0x200);
	if (!buffer) {
		release_bus(d);
		goto err;
	}
	tmpvec.iov_base = buffer;
	tmpvec.iov_len = num * 0x200;
	err = transfer_sectors(h, sector, num, &tmpvec, 1, 0, false); /* read */
	drive_sum = boot_calculate_hash(buffer, num * 0x200);
	free(buffer);
	release_bus(d);
	if (err != B_NO_ERROR)
		goto err;
	return drive_sum;
err:
	dprintf("IDE ATA -- get_sector_1_checksum: %s reading first block\n",
	        strerror(err));
	return 0;
}

static void
get_bios_drive_info(device_handle *h)
{
	const char *debugprefix = "IDE ATA -- get_bios_driveinfo:";
	device_entry *d = h->d;
	char my_id[17];
	struct bios_drive_info *bd;
	uint32 i;
	bool available_geometry = false;

	d->priv->bios_drive_info = NULL;
	if(bios_drive_info == NULL)
		return;

	sprintf(my_id, "ata/%lu/%lu", d->busnum, d->devicenum);

	for(bd = bios_drive_info; bd->bios_id; bd++) {
		if(strcmp(bd->beos_id, my_id) == 0) {
			DI(1) dprintf("%s %s match bios drive 0x%0x\n",
			              debugprefix, my_id, bd->bios_id);
			d->priv->bios_drive_info = bd;
			return;
		}
		if(bd->beos_id[0] == '\0')
			available_geometry = true;
	}
	if(!available_geometry) {
		DI(1) dprintf("%s all bios drives match other devices\n",
		              debugprefix);
		return;
	}

	for(bd = bios_drive_info; bd->bios_id; bd++) {
		struct bios_drive_info *checkbd;

		if(bd->beos_id[0] != '\0')
			continue;

		for (i=0;i<bd->num_hashes;i++) {
			if (get_checksum(h, (uint32)(bd->hashes[i].offset / 0x200),
					bd->hashes[i].size / 0x200) != bd->hashes[i].hash)
				break;
		}

		if (i != bd->num_hashes) continue;

		for(checkbd = bios_drive_info; checkbd->bios_id; checkbd++) {
			if (bd == checkbd) continue;
			if (bd->num_hashes != checkbd->num_hashes) continue;

			for (i=0;i<bd->num_hashes;i++) {
				if ((bd->hashes[i].offset != checkbd->hashes[i].offset) ||
					(bd->hashes[i].size != checkbd->hashes[i].size) ||
					(bd->hashes[i].hash != checkbd->hashes[i].hash))
					break;
			}
			if (i != bd->num_hashes) continue;

			if (bd->c == checkbd->c &&
				bd->h == checkbd->h &&
				bd->s == checkbd->s)
				continue;

			DE(1) dprintf("%s first block checksum not unique\n",
			              debugprefix);
			return;
		}

		DI(1) dprintf("%s checksum of %s matched bios drive 0x%0x\n",
		              debugprefix, my_id, bd->bios_id);
		strcpy(bd->beos_id, my_id);
		d->priv->bios_drive_info = bd;
		return;
	}
}

#endif

status_t
use_settings_device(device_entry *d, driver_parameter *p)
{
	const char *debugprefix = "IDE ATA -- use_settings_device:";
	if(strcmp(p->name, "nodma") == 0) {
		d->priv->dma_enabled = false;
		d->priv->dma_status = B_IDE_DMA_STATUS_USER_DISABLED_CONFIG;
	}
	else if(strcmp(p->name, "forcedma") == 0) {
		d->priv->dma_enabled = true;
		d->priv->dma_status = B_IDE_DMA_STATUS_ENABLED;
	}
	else if(strcmp(p->name, "max_transfer_size") == 0) {
		if(p->value_count != 1) {
			DE(2) dprintf("%s max_transfer_size takes one argument\n",
			              debugprefix);
		}
		else {
			d->priv->max_transfer_size = strtol(p->values[0], NULL, 10);
			if(d->priv->max_transfer_size % 512) {
				DE(2) dprintf("%s max_transfer_size, %d, must be a multiple "
				              "of 512\n",
				              debugprefix, d->priv->max_transfer_size);
				d->priv->max_transfer_size /= 512;
				d->priv->max_transfer_size *= 512;
			}
			if(d->priv->max_transfer_size < 512 ||
			   d->priv->max_transfer_size > 256*512) {
				DE(2) dprintf("%s max_transfer_size, %d, must be between 512 "
				              "and 131072\n",
				              debugprefix, d->priv->max_transfer_size);
				d->priv->max_transfer_size = 0;
			}
		}
	}
	else {
		dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
	}
	return B_NO_ERROR;
}

status_t
configure_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- configure_device:";
	ide_task_file *tf = d->tf;
	uint8	status;
	uint8	media_status_features=0;
	device_handle h;
	
	h.next = NULL;
	h.name = "temp handle";
	h.d = d;
	h.lun = -1; // global commands only
	h.priv = NULL;
	h.status = B_NO_ERROR;
	
	d->priv = malloc(sizeof(driver_device_entry));
	if(d->priv == NULL) {
		return B_NO_MEMORY;
	}
	d->priv->max_transfer_size = 0;	/* use max possible */
	d->priv->dma_enabled = default_dma_enable;
	if(d->di->capabilities.dma_supported)
		d->priv->dma_status = B_IDE_DMA_STATUS_ENABLED;
	else
		d->priv->dma_status = B_IDE_DMA_STATUS_UNSUPPORTED;
	if(d->priv->dma_enabled && d->di->capabilities.dma_supported) {
		uint32	drive_dma_mode = 0;
		uint32	controller_dma_mode = ~0;
		if(get_selected_dma_mode(d->di, &drive_dma_mode) != B_NO_ERROR) {
			DE(2) dprintf("%s selected dma mode bad,\n"
			              "%28s disable dma for this device\n",
			              debugprefix, "");
			d->priv->dma_enabled = false;
			d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_CONFIG;
		}
		else {
			DI(1) dprintf("%s drive dma mode: 0x%02lx\n",
			              debugprefix, drive_dma_mode);
	
			if(d->bus->get_dma_mode(d->buscookie, d->devicenum, &controller_dma_mode) == B_OK) {
				DI(1) dprintf("%s controller dma mode: 0x%02lx\n",
				              debugprefix, controller_dma_mode);
				if((drive_dma_mode & 0xf0) != (controller_dma_mode & 0xf0) ||
				   (drive_dma_mode & 0x0f) < (controller_dma_mode & 0x0f)) {
					DE(2) dprintf("%s drive and controller dma mode mismatch,\n"
					              "%28s disable dma for this device\n",
					              debugprefix, "");
					d->priv->dma_enabled = false;
					d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_CONFIG;
				}
			}
		}
	}
	read_settings_device(d, "ata");
	if(default_dma_enable == false) {
		d->priv->dma_enabled = false;
		d->priv->dma_status = B_IDE_DMA_STATUS_USER_DISABLED_SAFE_MODE;
	}

	d->priv->media_change_requested = false;
	d->priv->locked = false;

	if(d->di->removable_media.media_status_notification_supported == 1) {
		status_t err;
		tf->write.command = 0xef;	/* SET FEATURES */
		tf->write.features = 0x95;	/* Enable Media Status Notification */
		err = acquire_bus(d);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock bus\n", debugprefix);
		}
		else {
			err = ide->send_ata(d->bus, d->buscookie, d->tf, true);
			if (err == B_NO_ERROR)  {
				err = d->bus->intwait(d->buscookie, 1000000);
			}
			if(err == B_NO_ERROR) {
				status = d->bus->get_altstatus(d->buscookie);
				if((status & (ATA_BSY|ATA_DRQ|ATA_ERR)) == ATA_ERR) {
					d->bus->read_command_block_regs(d->buscookie, tf,
					                                ide_mask_error);
					if(tf->read.error & 0x04)
						err = B_IO_ERROR;
					else {
						DE(2) dprintf("%s error 0x%02x\n",
						              debugprefix, tf->read.error);
						err = B_ERROR;
					}
				}
				if(status & (ATA_BSY|ATA_DRQ)) {
					DE(2) dprintf("%s error - status = 0x%02x\n",
					              debugprefix, status);
					err = B_IO_ERROR;
				}
			}
			if(err == B_NO_ERROR) {
				d->bus->read_command_block_regs(d->buscookie, tf,
				                                ide_mask_cylinder);
				media_status_features = tf->read.cylinder_high;
			}
			release_bus(d);
		}
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s couldn't enable media status notification\n",
			              debugprefix);
		}
		else {
			status_t err;
			int max_loop = 16;
			DI(2) dprintf("%s enabled media status notification\n"
			              "                               "
			              "pej %d, lock %d, pena %d\n",
			              debugprefix,
			              (media_status_features&4)==4,
			              (media_status_features&2)==2,
			              (media_status_features&1)==1);
			do {
				err = get_media_status(&h);
				if(err == B_NO_ERROR)
					break;
				if(err == B_DEV_NO_MEDIA)
					break;
			} while(--max_loop > 0);
			if(max_loop == 0)
				DE(2) dprintf("%s get media status "
				              "did not settle on a valid status\n"
				              "                             got %s\n",
				              debugprefix,
				              strerror(err));
		}
	}

#if __INTEL__
	get_bios_drive_info(&h);
#endif

	return B_NO_ERROR;
}

void
release_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- release_device:";
	ide_task_file *tf = d->tf;
	uint8	status;
	uint8	media_status_features=0;
	if(d->di->removable_media.media_status_notification_supported == 1) {
		status_t err;
		tf->write.command = 0xef;	/* SET FEATURES */
		tf->write.features = 0x31;	/* Disable Media Status Notification */
		err = acquire_bus(d);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock bus\n", debugprefix);
		}
		else {
			err = ide->send_ata(d->bus, d->buscookie, d->tf, true);
			if (err == B_NO_ERROR)  {
				err = d->bus->intwait(d->buscookie, 1000000);
			}
			if(err == B_NO_ERROR) {
				status = d->bus->get_altstatus(d->buscookie);
				if((status & (ATA_BSY|ATA_DRQ|ATA_ERR)) == ATA_ERR) {
					d->bus->read_command_block_regs(d->buscookie, tf, ide_mask_error);
					if(tf->read.error & 0x04)
						err = B_IO_ERROR;
					else {
						DE(2) dprintf("%s error 0x%02x\n", debugprefix,
						              tf->read.error);
						err = B_ERROR;
					}
				}
				if(status & (ATA_BSY|ATA_DRQ)) {
					DE(2) dprintf("%s error - status = 0x%02x\n",
					              debugprefix, status);
					err = B_IO_ERROR;
				}
			}
			if(err == B_NO_ERROR) {
				d->bus->read_command_block_regs(d->buscookie, tf, ide_mask_cylinder);
				media_status_features = tf->read.cylinder_high;
			}
			release_bus(d);
		}
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s couldn't disable media status notification\n",
			              debugprefix);
		}
		else {
			DI(2) dprintf("%s disabled media status notification\n"
			              "                               "
			              "pej %d, lock %d, pena %d\n",
			              debugprefix,
			              (media_status_features&4)==4,
			              (media_status_features&2)==2,
			              (media_status_features&1)==1);
		}
	}
	free(d->priv);
}

void
wake_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- wake_device:";
	if(d->di->command_set_feature_supported.valid != 1 ||
	   d->di->command_set_feature_supported.power_management == 0) {
		DF(1) dprintf("%s no power_management\n", debugprefix);
		return;
	}
	DF(1) dprintf("%s power_management supported\n", debugprefix);
}

void
suspend_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- suspend_device:";
	if(d->di->command_set_feature_supported.valid != 1 ||
	   d->di->command_set_feature_supported.power_management == 0) {
		DF(1) dprintf("%s no power_management\n", debugprefix);
		return;
	}
	DF(1) dprintf("%s power_management supported\n", debugprefix);
}

/*------------------------------------------------------------------------------
** block io functions
*/

uint32 get_capacity(void *cookie)
{
	device_handle *h = cookie;
	
	ide_device_info *di = h->d->di;
	if (di->capabilities.lba_supported && di->user_addressable_sectors != 0) {
		return di->user_addressable_sectors;
	}
	else {
		return di->current_sectors_per_track *
		       di->current_cylinders * di->current_heads;
	}
}

uint32 get_blocksize(void * UNUSED(cookie))
{
	UNUSED_ARG(cookie)
	return 512;
}

uint32 get_bad_alignment_mask(void *cookie)
{
	device_handle *h = cookie;
	device_entry *d = h->d;

	if(d->di->capabilities.dma_supported && d->priv->dma_enabled)
		return d->bad_alignment_mask;
	else
		return 0;
}

uint32 get_max_transfer_size(void *cookie)
{
	device_handle *h = cookie;
	if(h->d->priv->max_transfer_size >= 512 &&
	   h->d->priv->max_transfer_size < 256*512)
	   	return h->d->priv->max_transfer_size;
	return 256*512;	/* we can only transfer 256 sectors at a time */
}

status_t
read_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
            const iovec *vec, size_t count, uint32 startbyte)
{
	device_handle *h = cookie;
	device_entry *d = h->d;
	status_t err;

	err = acquire_bus(d);
	if (err != B_NO_ERROR)
		return err;
	err = h->status;
	if(err == B_NO_ERROR)
		err = transfer_sectors(h, start_block, num_blocks, vec, count,
		                       startbyte, false); /* read */
	release_bus(d);
	return err;
}

status_t
write_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
             const iovec *vec, size_t count, uint32 startbyte)
{
	device_handle *h = cookie;
	device_entry *d = h->d;
	status_t err;

	err = acquire_bus(d);
	if (err != B_NO_ERROR)
		return err;
	err = h->status;
	if(err == B_NO_ERROR)
		err = transfer_sectors(h, start_block, num_blocks, vec, count,
		                       startbyte, true); /* write */
	release_bus(d);
	return err;
}

/*------------------------------------------------------------------------------
** Device entry points
*/

static status_t
ata_open(const char *name, uint32 UNUSED(flags), device_handle **h)
{
	status_t err;
	UNUSED_ARG(flags)
	if ((err = open_device(name, h, IDE_ATA)) != B_NO_ERROR) {
		DE(1) dprintf("IDE ATA -- open: could not open %s\n", name);
		return err;
	}
	get_media_status(*h);
	return err;
}

static status_t
ata_close(device_handle * UNUSED(h))
{
	UNUSED_ARG(h)
	return B_NO_ERROR;
}

static status_t
ata_free(device_handle *h)
{
	return close_device(h);
}

static status_t
ata_control(device_handle *h, uint32 msg, void *arg1, size_t UNUSED(len))
{
	const char *debugprefix = "IDE ATA -- control:";
	device_entry *d = h->d;
	ide_device_info *di = d->di;
	UNUSED_ARG(len)

//	DF(2) dprintf("IDE ATA -- control: msg = 0x%08x\n", msg);
	switch (msg) {
		case B_GET_MEDIA_STATUS:
			DF(1) dprintf("%s B_GET_MEDIA_STATUS\n", debugprefix);
			(*(status_t*)arg1) = get_media_status(h);
			return B_NO_ERROR;

		case B_GET_GEOMETRY: {
			device_geometry *dinfo = (device_geometry *) arg1;

			DF(1) dprintf("%s B_GET_GEOMETRY\n", debugprefix);
			
			if (di->capabilities.lba_supported) {
				dinfo->sectors_per_track = 1;
				dinfo->cylinder_count    = di->user_addressable_sectors;
				dinfo->head_count        = 1;
			} else {
				dinfo->sectors_per_track = di->current_sectors_per_track;
				dinfo->cylinder_count    = di->current_cylinders;
				dinfo->head_count        = di->current_heads;
			}
			
			dinfo->bytes_per_sector = 512;
			dinfo->removable = di->general_configuration.removable_media;
			dinfo->read_only = FALSE;	/* XXX should reflect the media */
			dinfo->device_type = B_DISK;
			dinfo->write_once = FALSE;
			return B_NO_ERROR;
		}	
#if __INTEL__
		case B_GET_BIOS_GEOMETRY: {
			device_geometry *dinfo = (device_geometry *) arg1;
	
			if(d->priv->bios_drive_info == NULL)
				return B_ERROR;
			
			dinfo->sectors_per_track = d->priv->bios_drive_info->s;
			dinfo->cylinder_count    = d->priv->bios_drive_info->c;
			dinfo->head_count        = d->priv->bios_drive_info->h;
			dinfo->bytes_per_sector = 512;
			dinfo->removable = di->general_configuration.removable_media;
			dinfo->read_only = FALSE;
			dinfo->device_type = B_DISK;
			dinfo->write_once = FALSE;
			return B_NO_ERROR;
		}
		
		case B_GET_BIOS_DRIVE_ID: {
			uint8	*bios_id_p = (uint8*) arg1;
			if(d->priv->bios_drive_info == NULL)
				return B_ERROR;
			*bios_id_p = d->priv->bios_drive_info->bios_id;
			return B_NO_ERROR;
		}
#endif
#if 0
		case B_FORMAT_DEVICE:
			if (!*((char *) arg1))
				return B_NO_ERROR;
			
			block = (uchar*)malloc(d->block_size);
			memset(block, 0, d->block_size);
			sectors = d->sectors_per_cylinder * d->cylinders;
			for (loop = 0; loop < sectors; loop++) {
				sz = d->block_size;
				err = ide_write(d, loop * d->block_size, block, &sz);
				if (err || (sz != d->block_size)) {
					free(block);
					return B_ERROR;
				}
			}
			free(block);
			return B_NO_ERROR;
#endif
		case B_GET_DEVICE_SIZE:
			if (di->capabilities.lba_supported)
				*(size_t*)arg1 = di->user_addressable_sectors * 512;
			else
				*(size_t*)arg1 = di->current_capacity_in_sectors * 512;
			DF(1) dprintf("%s B_GET_DEVICE_SIZE <- %ld\n",
			              debugprefix, *(size_t*)arg1);
			return B_NO_ERROR;
			
		case B_GET_ICON:
			DF(1) dprintf("%s B_GET_ICON\n", debugprefix);
			switch (((device_icon *)arg1)->icon_size) {
			case B_LARGE_ICON:
				if (di->general_configuration.removable_media)
					memcpy(((device_icon *)arg1)->icon_data, &icon_removable,
						   B_LARGE_ICON * B_LARGE_ICON);
				else
					memcpy(((device_icon *)arg1)->icon_data, &icon_disk,
													B_LARGE_ICON * B_LARGE_ICON);
				break;
			case B_MINI_ICON:
				if (di->general_configuration.removable_media)
					memcpy(((device_icon *)arg1)->icon_data, &icon_removable_mini,
						   B_MINI_ICON * B_MINI_ICON);
				else
					memcpy(((device_icon *)arg1)->icon_data, &icon_disk_mini,
						   B_MINI_ICON * B_MINI_ICON);
				break;
			default:
				return B_BAD_TYPE;
			}
			return B_NO_ERROR;

		case B_EJECT_DEVICE:
		case B_SCSI_EJECT:
			DF(1) dprintf("%s B_EJECT_DEVICE\n", debugprefix);
			if(d->priv->locked)
				return B_NOT_ALLOWED;
			else
				return media_eject(h);

		case B_SCSI_PREVENT_ALLOW:
			DF(1) dprintf("%s B_SCSI_PREVENT_ALLOW %d\n",
			              debugprefix, *(bool *)arg1);
			d->priv->locked = *(bool *)arg1;
			return B_NO_ERROR;
		
		case B_FLUSH_DRIVE_CACHE:
			return flush_cache(h);

		case B_GET_IDE_DEVICE_INFO:
			DF(1) dprintf("%s B_GET_IDE_DEVICE_INFO\n", debugprefix);
			memcpy(arg1, h->d->di, sizeof(ide_device_info));
			return B_NO_ERROR;

		case B_GET_IDE_DEVICE_STATUS: {
			ide_device_status *ds = arg1;
			uint32 drive_dma_mode = ~0;
			DF(1) dprintf("%s B_GET_IDE_DEVICE_STATUS\n", debugprefix);
			if(ds->version != 1)
				return B_BAD_VALUE;
			ds->dma_status = d->priv->dma_status;
			ds->pio_mode = di->pio_cycle_timing.pio_data_transfer_mode_number;
			get_selected_dma_mode(di, &drive_dma_mode);
			ds->dma_mode = drive_dma_mode;
			return B_NO_ERROR;
		}
			
	}
	DE(2) dprintf("%s unknown ioctl 0x%08lx\n", debugprefix, msg);
	return B_ERROR;
}

static device_hooks ata_device = {
	(long (*)(const char *, unsigned long, void **))&ata_open,
	(long (*)(void *))&ata_close,
	(long (*)(void *))&ata_free,
	(long (*)(void *, unsigned long, void *, unsigned long))&ata_control,
	&driver_read,
	&driver_write,
	NULL, /* select */
	NULL, /* deselect */
	&driver_readv,
	&driver_writev
};

/*------------------------------------------------------------------------------
** Driver entry points
*/

static int debug_ata(int argc, char **argv);

#if _BUILDING_kernel || BOOT
#define ATA_EXPORT static
#else
#define ATA_EXPORT _EXPORT
#endif

ATA_EXPORT const char **
publish_devices()
{
	void *settingshandle;
	status_t err;

	settingshandle = load_driver_settings("ata");
	err = create_devicelist(IDE_ATA);
	if(settingshandle != NULL)
		unload_driver_settings(settingshandle);
	if(err == B_NO_ERROR) {
		return (const char **)ide_names;
	}
	else {
		static const char *no_ide_names[] = {
			NULL
		};
		DE(1) dprintf("IDE ATA -- publish_devices: failed\n");
		return no_ide_names;
	}
}

ATA_EXPORT device_hooks *
find_device(const char * UNUSED(name))
{
	UNUSED_ARG(name)
	return &ata_device;
}

ATA_EXPORT status_t
init_driver()
{
	const char *debugprefix = "IDE ATA -- init:";
	status_t err;
	
#if LOAD_ONLY_WITH_DEBUG_ENABLED
	{
		bool db = set_dprintf_enabled(true);
		set_dprintf_enabled(db);
		if(!db)
			return B_ERROR;
	}
#endif

	DF(1) dprintf("%s\n", debugprefix);
	err = init_scratch_buffer();
	if(err < B_NO_ERROR)
		goto err0;
	err = init_devicelist(IDE_ATA);
	if(err < B_NO_ERROR)
		goto err2;

	err = get_module(IDE_MODULE_NAME, (module_info **)&ide);
	if (err < B_NO_ERROR) {
		DE(1) dprintf("%s could not get ide bus manager\n", debugprefix);
		goto err3;
	}

	default_dma_enable = !ide_dma_disabled();
#if 0
	{
		void *settingshandle = load_driver_settings("safemode");
		dprintf("safemode no_ide_dma: %s\n",
			get_driver_parameter(settingshandle, "no_ide_dma", "unset", "set"));
		unload_driver_settings(settingshandle);
	}
#endif

	read_settings_global();

	add_debugger_command("atadb", debug_ata, "atadb <args> - ata info");

	DF(1) dprintf("%s done\n", debugprefix);

	return B_NO_ERROR;

err3:
	uninit_devicelist();
err2:
	uninit_scratch_buffer();
err0:
	DE(1) dprintf("%s FAILED with error %s\n", debugprefix, strerror(err));
	return err;
}

ATA_EXPORT void
uninit_driver()
{
	DF(1) dprintf("IDE ATA -- uninit\n");
	remove_debugger_command("atadb", debug_ata);
	uninit_devicelist();
	uninit_scratch_buffer();
	put_module(IDE_MODULE_NAME);
#ifdef CHECKMALLOC
	my_memory_used();
#endif
}

ATA_EXPORT void
wake_driver()
{
	wake_module(IDE_MODULE_NAME);
}

ATA_EXPORT void
suspend_driver()
{
	suspend_module(IDE_MODULE_NAME);
}

#if _BUILDING_kernel || BOOT
fixed_driver_info ata_driver_info = {
	"ata",
	B_CUR_DRIVER_API_VERSION,
	NULL,
	&publish_devices,
	&find_device,
	&init_driver,
	&uninit_driver
};
#else
ATA_EXPORT int32
api_version = B_CUR_DRIVER_API_VERSION;
#endif

/*------------------------------------------------------------------------------
** debugger
*/

static int
debug_ata(int argc, char **argv)
{
	if (argc == 1) {
		kprintf("atadb <command> <args>\n"
		        "atadb help - list ata debug commands\n");
		return 0;
	}
	if(strcmp(argv[1], "help") == 0) {
		kprintf("atadb debug commands:\n");
		kprintf("      debug [eif] <level> - set debug level\n");
		kprintf("      dump  - print device information\n");
		kprintf("      dma <bus> <device> - enable/disable dma\n");
	}
	else if(strcmp(argv[1], "debug") == 0) {
		if(argc != 4) {
			kprintf("    debug e <level> - set error output level\n");
			kprintf("    debug i <level> - set information output level\n");
			kprintf("    debug f <level> - set flow output level\n");
		}
		else {
			switch(argv[2][0]) {
				case 'e':	debug_level_error =	atoi(argv[3]);	break;
				case 'i':	debug_level_info =	atoi(argv[3]);	break;
				case 'f':	debug_level_flow =	atoi(argv[3]);	break;
				default:
					kprintf("    debug <class> <level>, class must be "
					        "e, i or f\n");
			}
		}
	}
	else if(strcmp(argv[1], "dump") == 0) {
		extern void kdump_devicelist();
		kdump_devicelist();
	}
	else if(strcmp(argv[1], "dma") == 0) {
		extern device_entry *kget_device_ptr(uint32 busnum, uint32 devicenum);
		device_entry *dev;
		if(argc < 4)
			kprintf("    dma <bus> <device> <state>\n");
		else {
			dev = kget_device_ptr(atoi(argv[2]), atoi(argv[3]));
			if(dev != NULL) {
				if(dev->priv == NULL)
					kprintf("device not valid\n");
				else {
					if(argc < 5) {
						kprintf("dma %s for device %s on bus %s\n",
						        dev->priv->dma_enabled ? "enabled" : "disabled",
						        argv[3], argv[2]);
					}
					else {
						dev->priv->dma_enabled = argv[4][0] == 'e';
					}
				}
			}
		}
	}
	else {
		kprintf("unknown command %s\n", argv[1]);
	}
	return 0;
}

