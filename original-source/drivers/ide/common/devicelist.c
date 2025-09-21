#include "devicelist.h"

#include <stdlib.h>
#include <stdio.h>
#include <KernelExport.h>
#include <string.h>
#include <IDE.h>

#include <debugoutput.h>

char **ide_names;

static device_entry *devicelist;
static device_entry *recycle_devicelist;
static sem_id devicelist_sem;
static sem_id handlerlist_sem;
static sem_id wake_sem;
static int wake_waiting;
static bool devicelist_valid = false;

static status_t 
allocate_device(device_entry **device)
{
	if ((*device = malloc(sizeof(device_entry))) == NULL)			goto e0;
	if (((*device)->tf = malloc(sizeof(ide_task_file))) == NULL)	goto e1;
	if (((*device)->di = malloc(sizeof(ide_device_info))) == NULL)	goto e2;

#if 0
	dprintf("allocated device at 0x%08x\n", *device);
	dprintf("allocated task_file at 0x%08x\n", ai->tf);
	dprintf("allocated device_info at 0x%08x\n", ai->di);
#endif

	return B_NO_ERROR;

e2:	free((*device)->tf);
e1:	free(*device);
e0:	return B_NO_MEMORY;	

}

static void 
free_device(device_entry *device)
{
#if 0
	dprintf("IDE ATA: free_device(0x%08x)\n", device);
	if(device) {
		dprintf("IDE ATA: free_device di: 0x%08x\n", device->di);
		dprintf("IDE ATA: free_device tf: 0x%08x\n", device->tf);
	}
#endif
	free(device->di);
	free(device->tf);
	free(device);
}


static status_t
ndrq_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
		   bigtime_t timeout)
{
	if ((status & (ATA_DRQ | ATA_BSY)) == 0)
		return B_NO_ERROR;
	if ((sample_time - start_time) > timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

static status_t
identify_device(device_entry *d, bool atapi, bool samsung_master)
{
	const char debugprefix[] = DEBUGPREFIX "identify_device:";
	uint32 buscookie = d->buscookie;
	ide_bus_info *bus = d->bus;
	ide_task_file *tf = d->tf;
	status_t err;
	uint8 status;

	err = bus->acquire_bus(buscookie);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock bus %ld\n", debugprefix, d->busnum);
		goto err1;
	}

	/*
	 * Send IDENTIFY PACKET DEVICE or IDENTIFY DEVICE
	 */
	
	tf->lba.command = (atapi ? 0xa1 : 0xec); 
	err = ide->send_ata(bus, buscookie, tf, (atapi ? false : true));
	if(err != B_NO_ERROR) {
		DI(2) dprintf("%s could not send command to device %ld on bus %ld\n",
		              debugprefix, d->devicenum, d->busnum);
		goto err2;
	}
	DF(3) dprintf("%s bus %ld, device %ld sent IDENTIFY command\n"
	              "%*s (status 0x%02x)\n", 
	              debugprefix, d->busnum, d->devicenum,
	              (int)sizeof(debugprefix)-1, "",
	              bus->get_altstatus(buscookie));
	err = bus->intwait(buscookie, 100000);   /* 100 msec */
	if(err != B_NO_ERROR) {
		status = bus->get_altstatus(buscookie);
		DF(3) dprintf("%s bus %ld, device %ld no interrupt after 100ms\n"
	              "%*s (status 0x%02x)\n", 
	              debugprefix, d->busnum, d->devicenum,
	              (int)sizeof(debugprefix)-1, "",
	              status);
		if((status & ATA_BSY) == 0) {
			err = bus->intwait(buscookie, 1000);	/* allow 1 msec */
		}
		else {
			DI(1) dprintf("%s wait longer for interrupt from "
			              "device %ld on bus %ld\n",
			              debugprefix, d->devicenum, d->busnum);
			err = bus->intwait(buscookie, 20000000);   /* 20 sec */
		}
		if(err != B_NO_ERROR) {
			DI(2) dprintf("%s did not get interrupt from device %ld on bus %ld"
			              "\n%*s (status 0x%02x)\n", 
			              debugprefix, d->devicenum, d->busnum,
			              (int)sizeof(debugprefix)-1, "",
						  bus->get_altstatus(buscookie));
			goto err2;
		}
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_ERR) {
		DI(2) dprintf("%s device %ld on bus %ld is not ATA%s\n",
		              debugprefix, d->devicenum, d->busnum, atapi ? "PI" : "");
		err = B_ERROR;
		if(samsung_master)
			bus->read_pio_16(buscookie, (uint16*)d->di, 256);
		goto err2;
	}
	if(status & ATA_BSY) {
		DE(2) dprintf("%s device %ld on bus %ld was busy after interrupt\n",
		              debugprefix, d->devicenum, d->busnum);
		ide->reset(bus, buscookie);
		err = B_ERROR;
		goto err2;
	}
	if((status & ATA_DRQ) == 0) {
		DE(2) dprintf("%s device %ld on bus %ld did not assert DRQ\n",
		              debugprefix, d->devicenum, d->busnum);
		err = B_ERROR;
		goto err2;
	}

	bus->read_pio_16(buscookie, (uint16*)d->di, 256);
	
	err = ide->wait_status(bus, buscookie, ndrq_p, 100000);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s DRQ still asserted after data on device %ld on bus "
		              "%ld\n", debugprefix, d->devicenum, d->busnum);
	}
	
	DF(2) dprintf("%s bus %ld, device %ld got data\n", 
	              debugprefix, d->busnum, d->devicenum);
	
//	err = bus->intwait(buscookie, 100000);   /* 100 msec */
//	status = bus->get_altstatus(buscookie);
//	if(err != B_NO_ERROR) {
//		dprintf("IDE %s -- init_device: "
//		        "did not get interrupt from device %d on bus %d after data "
//				"(status %02x)\n",
//		        debugstr, devicenum, busnum, status);
//		goto err5;
//	}
//	if((status & (ATA_DRQ|ATA_ERR)) == ATA_DRQ) {
//		dprintf("IDE %s -- init_device: "
//		        "DRQ still asserted -- retry\n");
//		if(retry_count-- > 0)
//			goto retry_data;
//	}
	if(status & ATA_ERR) {
		DE(2) dprintf("%s device %ld on bus %ld returned error after "
		              "data transfer\n", debugprefix, d->devicenum, d->busnum);
		err = B_ERROR;
		goto err2;
	}
	if(atapi) {
		bus->intwait(buscookie, 10000);	/* wait for interrupt from bad device */
	}
	bus->read_command_block_regs(buscookie, tf, ide_mask_status);
	bus->release_bus(buscookie);
	prepare_ide_device_info(d->di);
	tf->lba.mode = d->di->capabilities.lba_supported ?
	               IDE_LBA_MODE : IDE_CHS_MODE;
	
	if (!is_supported_device(d->di)) {
		err = B_ERROR;
		goto err1;
	}

	DI(2) print_ide_device_info((void (*)())dprintf, d->di);
	return B_NO_ERROR;

err2:
	bus->release_bus(buscookie);
err1:
	return err;
}

static
status_t
init_device(uint32 busnum, uint32 devicenum, /*uint32 lun,*/ device_entry **d, ide_device_type type, bool samsung_master)
{
	const char debugprefix[] = DEBUGPREFIX "init_device:";
	status_t err;
	ide_task_file *tf;
	ide_device_info *di;
	device_entry *newdevice;
	uint32 buscookie;
	ide_bus_info *bus;
	bool atapi = (type == IDE_ATAPI);

	newdevice = recycle_devicelist;
	{
		device_entry *prevdevice = NULL;
		while (newdevice != NULL) {
			if((newdevice->busnum == busnum) &&
			   (newdevice->devicenum == devicenum) && newdevice->valid) {
				DI(2) dprintf("%s recycle device %ld on bus %ld\n", 
				              debugprefix, devicenum, busnum);

				err = identify_device(newdevice, atapi, false);
				if(err == B_NO_ERROR) {
					if (prevdevice == NULL) {
						recycle_devicelist = newdevice->next;
					} else {
						prevdevice->next = newdevice->next;
					}
					goto add_device;
				}
				else {
					newdevice->valid = false;
					ide->release_device(busnum, devicenum);
					goto err0;
				}
			}
			prevdevice = newdevice;
			newdevice = newdevice->next;
		}
	}

	DF(2) dprintf("%s identify device %ld on bus %ld\n",
	              debugprefix, devicenum, busnum);

	err = ide->get_nth_ide_bus(busnum, &bus, &buscookie);
	if(err != B_NO_ERROR)
		goto err0;

	err = allocate_device(&newdevice);
	if (err != B_NO_ERROR)
		goto err0;

	tf = newdevice->tf;
	di = newdevice->di;
	tf->chs.device = devicenum;

	err = bus->acquire_bus(buscookie);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock bus %ld\n", debugprefix, busnum);
		goto err3;
	}
	err = ide->handle_device(busnum, devicenum);
	bus->release_bus(buscookie);
	if(err != B_NO_ERROR) {
		DI(2) dprintf("%s device %ld on bus %ld is handled by another driver\n",
		              debugprefix, devicenum, busnum);
		goto err3;
	}

	newdevice->valid = true;
	newdevice->suspended = false;
	newdevice->busnum = busnum;
	newdevice->devicenum = devicenum;
	newdevice->bus = bus;
	newdevice->buscookie = buscookie;
	newdevice->maxlun = 0;
	newdevice->opencount = 0;
	newdevice->device_handles = NULL;
	newdevice->bad_alignment_mask = bus->get_bad_alignment_mask(buscookie);
		
	err = identify_device(newdevice, atapi, samsung_master);
	if (err != B_NO_ERROR)
		goto err4;
	
	err = configure_device(newdevice);
	if (err != B_NO_ERROR)
		goto err4;
	
add_device:
	newdevice->next = devicelist;
	devicelist = newdevice;
//	DF(2) dprintf("IDE %s -- init_devicelist: added %s cookie 0x%08x\n",
//	              debugstr, newdevice->name, newdevice);
	if (d)
		*d = newdevice;
	return B_NO_ERROR;

err4:
	ide->release_device(busnum, devicenum);
err3:
	free_device(newdevice);
err0:
	return err;
}


status_t
init_devicelist(ide_device_type devtype)
{
	status_t err;

	devicelist = NULL;
	devicelist_valid = true;
	recycle_devicelist = NULL;
	ide_names = NULL;
	wake_waiting = 0;

	err = devicelist_sem = create_sem(1, (devtype == IDE_ATAPI ? 
										  "atapi_devicelist_sem" :
										  "ata_devicelist_sem"));
	if(err < B_NO_ERROR)
		goto err1;
	err = handlerlist_sem = create_sem(1, (devtype == IDE_ATAPI ? 
										  "atapi_handlerlist_sem" :
										  "ata_handlerlist_sem"));
	if(err < B_NO_ERROR)
		goto err2;
	err = wake_sem = create_sem(0, (devtype == IDE_ATAPI ?
	                                "atapi_wake_sem" : "ata_wake_sem"));
	if(err < B_NO_ERROR)
		goto err3;
	return B_NO_ERROR;
err3:
	delete_sem(handlerlist_sem);
err2:
	delete_sem(devicelist_sem);
err1:
	return err;
}

void
uninit_devicelist(void)
{
	const char debugprefix[] = DEBUGPREFIX "uninit_devicelist:";
	devicelist_valid = false;
	while (recycle_devicelist) {
		device_entry *d = recycle_devicelist;
		recycle_devicelist = d->next;
		if(d->device_handles != NULL) {
			DE(0) dprintf("%s device still open\n", debugprefix);
		}
		release_device(d);
		ide->release_device(d->busnum, d->devicenum);
		free_device(d);
	}
	while (devicelist) {
		device_entry *d = devicelist;
		devicelist = d->next;
		if(d->device_handles != NULL) {
			DE(0) dprintf("%s device still open\n", debugprefix);
		}
		release_device(d);
		ide->release_device(d->busnum, d->devicenum);
		free_device(d);
	}
	if (ide_names) {
		char **tmp = ide_names;
		while(*tmp) {
			free(*tmp);
			tmp++;
		}
		free(ide_names);
	}
	delete_sem(wake_sem);
	delete_sem(handlerlist_sem);
	delete_sem(devicelist_sem);
}

status_t
create_devicelist(ide_device_type devtype)
{
	const char debugprefix[] = DEBUGPREFIX "create_devicelist:";
	uint32 busnum;
	status_t err;
	int devicecount = 0;

	err = acquire_sem_etc(devicelist_sem, 1, B_CAN_INTERRUPT, 0);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s could not access devicelist, %s\n",
		              debugprefix, strerror(err));
		return err;
	}

	devicelist_valid = false;
	while (devicelist) {
		device_entry *d = devicelist;
		devicelist = d->next;
		if (d->opencount > 0) {
			d->next = recycle_devicelist;
			recycle_devicelist = d;
		}
		else {
			if(d->device_handles != NULL) {
				DE(0) dprintf("%s closed device still has device handles\n",
				              debugprefix);
			}
			release_device(d);
			ide->release_device(d->busnum, d->devicenum);
			free_device(d);
		}
	}
	if (ide_names) {
		char **tmp = ide_names;
		while(*tmp) {
			free(*tmp);
			tmp++;
		}
		free(ide_names);
		ide_names = NULL;
	}
	devicelist = NULL;

	for (busnum = 0; busnum < ide->get_ide_bus_count(); busnum++) {
		int devicenum;
		bool samsung_master = false;
		
		for(devicenum = 0; devicenum < 2; devicenum++) {
			device_entry *device;
			err = init_device(busnum, devicenum, &device, devtype, samsung_master);
			if(err == B_NO_MEMORY) {
				DE(1) dprintf("%s out of memory error\n", debugprefix);
				goto err0;
			}
			if(err == B_NO_ERROR) {
				devicecount+= device->maxlun+1;
				if(strncmp(device->di->model_number, "SAMSUNG", strlen("SAMSUNG")) == 0) {
					samsung_master = true;
				}
			}
		}
	}
	{
		int i;
		device_entry *device = devicelist;

		ide_names = malloc(sizeof(char*) * (devicecount+1));
		if (ide_names == NULL) {
			err = B_NO_MEMORY;
			goto err0;
		}
		i=0;
		while(device != NULL) {
			uint32 lun;
			for(lun=0; lun <= device->maxlun; lun++) {
				char devicename[48]; // strlen("disk/ide/atapi/4294967295/master/4294967295/raw")+1
				if (i >= devicecount) {
					DE(0) dprintf("%s corrupted devicelist, "
					              "namearray is too small\n", debugprefix);
					err = B_ERROR;
					goto err0;
				}
				sprintf(devicename, "disk/ide/%s/%lu/%s/%lu/raw",
						devtype == IDE_ATAPI ? "atapi" : "ata", 
						device->busnum,
						device->devicenum == 0 ? "master" : "slave", lun);
				ide_names[i] = malloc(strlen(devicename)+1);
				if(ide_names[i] == NULL) {
					err = B_NO_MEMORY;
					goto err0;
				}
				strcpy(ide_names[i], devicename);
				i++;
			}
			device = device->next;
		}
		if (i != devicecount) {
			DE(0) dprintf("%s corrupted devicelist, namearray is too big\n",
			              debugprefix);
			err = B_ERROR;
			goto err0;
		}
		ide_names[i] = NULL;
	}
	devicelist_valid = true;
	release_sem(devicelist_sem);
	return B_NO_ERROR;
err0:
	while(devicelist) {
		device_entry *tmp = devicelist;
		devicelist = tmp->next;
		release_device(tmp);
		ide->release_device(tmp->busnum, tmp->devicenum);
		free_device(tmp);
	}
	devicelist_valid = true;
	release_sem(devicelist_sem);
	return err;	
}

static int
scan_dev_params(const char *name, const char *format, 
				uint32 *busnum, char *devicestring, uint32 *lun) 
{
	const char *n = name;
	char buf[20];
	int i = 0;

	if (strncmp(n, format, strlen(format)) != 0)
		return 0;
	
	n += strlen(format); 
	while (*n != '/') {
		buf[i++] = *n;
		n++;
	}
	buf[i] = 0;
	*busnum = atoi(buf);
	n++;

	i = 0;
	while (*n != '/') {
		devicestring[i++] = *n;
		n++;
	}
	devicestring[i] = 0;
	n++;
	
	i = 0;
	while (*n != '/') {
		buf[i++] = *n;
		n++;
	}
	buf[i] = 0;

	*lun = atoi(buf);
	return 3;
}

static status_t
parse_name(const char *name, uint32 *busnum, uint32 *devicenum, 
		   uint32 *lun, ide_device_type devtype)
{
	const char debugprefix[] = DEBUGPREFIX "parse_name:";
	char devicestring[7]; // strlen("master")+1
	char *format = (devtype == IDE_ATAPI) ? 
					"disk/ide/atapi/" :
					"disk/ide/ata/";
	
	DF(2) dprintf("%s name = %s\n", debugprefix, name);

	if(scan_dev_params(name, format, busnum, devicestring, lun) != 3) {
		goto err;
	}
	if(strcmp("master", devicestring) == 0)
		*devicenum = 0;
	else if(strcmp("slave", devicestring) == 0)
		*devicenum = 1;
	else {
		goto err;
	}
	return B_NO_ERROR;

  err:
	DE(1) dprintf("%s %s is not a valid device name\n", debugprefix, name);
	return B_ERROR;
}

status_t 
open_device(const char *name, device_handle **hp, ide_device_type devtype)
{
	const char debugprefix[] = DEBUGPREFIX "open_device:";
	uint32 busnum, devicenum, lun;
	status_t err;
	device_entry *device;
	device_handle *h;

	err = B_NO_MEMORY;	
	h = *hp = malloc(sizeof(device_handle));
	if(h == NULL)
		goto memerr0;
	h->name = malloc(strlen(name)+1);
	if(h->name == NULL)
		goto memerr1;
	strcpy(h->name, name);
	
	err = acquire_sem_etc(devicelist_sem, 1, B_CAN_INTERRUPT, 0);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock devicelist, %s\n",
		              debugprefix, strerror(err));
		goto err0;
	}

	err = parse_name(name, &busnum, &devicenum, &lun, devtype);
	if (err != B_NO_ERROR) {
		goto err1;
	}
	h->next = NULL;
	h->lun = lun;
	h->status = B_NO_ERROR;
	h->not_ready_on_open = false;
	
	device = devicelist;
	while(device != NULL) {
		if((device->busnum == busnum) &&
		   (device->devicenum == devicenum)) {
			h->d = device;
			goto found;
		}
		device = device->next;
	}
	err = init_device(busnum, devicenum, &(h->d), devtype, false);

	if(err != B_NO_ERROR)
		goto err1;
		
found:
	err = acquire_sem(handlerlist_sem);
	if(err != B_NO_ERROR)
		goto err1;
	h->next = h->d->device_handles;
	h->d->device_handles = h;
	h->d->opencount++;
	release_sem(handlerlist_sem);
	release_sem(devicelist_sem);
	DF(2) dprintf("%s opencount = %d\n", debugprefix, h->d->opencount);
	return B_NO_ERROR;

err1:
	release_sem(devicelist_sem);
err0:
	free(h->name);
memerr1:
	free(h);
memerr0:
	return err;
}

status_t
close_device(device_handle *h)
{
	const char debugprefix[] = DEBUGPREFIX "open_device:";
	status_t err;
	device_handle *prev, *tmp;
	device_entry *d = h->d;
	
	err = acquire_sem(devicelist_sem);
	if(err != B_NO_ERROR)
		return err;

	err = acquire_sem(handlerlist_sem);
	if(err != B_NO_ERROR)
		goto internalerr;
	d->opencount--;
	
	prev = NULL;
	tmp = h->d->device_handles;
	while(tmp && (tmp != h)) {
		prev = tmp;
		tmp = tmp->next;
	}
	if(tmp == NULL) {
		DE(0) dprintf("%s corrupt device_handle list\n", debugprefix);
		release_sem(handlerlist_sem);
		goto internalerr;
	}
	if(prev) {
		prev->next = h->next;
	}
	else {
		d->device_handles = h->next;
	}
	release_sem(handlerlist_sem);
	free(h->name);
	free(h);
	
	if(!d->valid && d->opencount == 0) {
		device_entry *pd = recycle_devicelist;

		DI(1) dprintf("%s invalid device now unused\n", debugprefix);

		if(d->device_handles != NULL) {
			DE(0) dprintf("%s closed device still has device handles\n",
			              debugprefix);
			goto internalerr;
		}

		if(pd == d) {
			recycle_devicelist = d->next;
		}
		else {
			while(pd != NULL && pd->next != d) {
				pd = pd->next;
			}
			if(pd == NULL) {
				DE(0) dprintf("%s invalid device not in recycle list\n", debugprefix);
				goto internalerr;
			}
			pd->next = d->next;
		}
		release_device(d);
		free_device(d);
	}
internalerr:
	release_sem(devicelist_sem);
	
	return B_NO_ERROR;
}

status_t acquire_bus(device_entry *d)
{
	status_t err;

retry:
	err = d->bus->acquire_bus(d->buscookie);
	if(err != B_NO_ERROR)
		goto err1;
	if(!d->valid) {
		err = B_ENTRY_NOT_FOUND;
		goto err2;
	}
	if(d->suspended) {
		wake_waiting++;
		d->bus->release_bus(d->buscookie);
		acquire_sem(wake_sem);
		goto retry;
	}
	return B_NO_ERROR;
err2:
	d->bus->release_bus(d->buscookie);
err1:
	return err;
}

status_t release_bus(device_entry *d)
{
	return d->bus->release_bus(d->buscookie);
}

status_t media_changed_in_lun(device_entry *d, uint32 lun)
{
	const char debugprefix[] = DEBUGPREFIX "media_changed_in_lun:";
	status_t err;
	device_handle *h;

	DI(2) dprintf("%s disc in slot %ld has changed\n", debugprefix, lun);
	
	if(d->device_handles == NULL) {	// prevent deadlock in init
		DI(2) dprintf("%s disc changed in init -- ignore\n", debugprefix);
		return B_NO_ERROR;
	}

	err = acquire_sem(handlerlist_sem);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock devicelist, %s\n",
		              debugprefix, strerror(err));
		goto err;
	}
	h = d->device_handles;
	while(h) {
		if(h->lun == lun) {
			if(h->not_ready_on_open)
				h->not_ready_on_open = false;
			else
				h->status = B_DEV_MEDIA_CHANGED;
		}
		h = h->next;
	}
	release_sem(handlerlist_sem);
err:
	return err;
}

status_t
wake_devices()
{
	const char debugprefix[] = DEBUGPREFIX "wake_devices:";
	status_t       err;
	device_entry  *device;

	err = acquire_sem(devicelist_sem);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock devicelist, %s\n",
		              debugprefix, strerror(err));
		goto err0;
	}
	do {
		device = devicelist;
		while(device != NULL) {
			if(device->suspended) {
				break;
			}
			device = device->next;
		}
		if(device != NULL) {
			release_sem(devicelist_sem);
			wake_device(device);
			err = acquire_sem(devicelist_sem);
			if(err != B_NO_ERROR) {
				DE(2) dprintf("%s could not lock devicelist, %s\n",
				              debugprefix, strerror(err));
				goto err0;
			}
			device->suspended = false;
		}
	} while(device != NULL);
	release_sem(devicelist_sem);
	return B_OK;
err0:
	return err;
}

status_t
suspend_devices()
{
	const char debugprefix[] = DEBUGPREFIX "suspend_devices:";
	status_t       err;
	device_entry  *device;

	do {
		err = acquire_sem(devicelist_sem);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock devicelist, %s\n",
			              debugprefix, strerror(err));
			goto err0;
		}
		device = devicelist;
		while(device != NULL) {
			if(!device->suspended) {
				device->suspended = true;
				break;
			}
			device = device->next;
		}
		release_sem(devicelist_sem);
		if(device != NULL) {
			suspend_device(device);
		}
	} while(device != NULL);
	return B_OK;
err0:
	return err;
}

/* called from kernel debugger only */

void kdump_devicelist();
device_entry *kget_device_ptr(uint32 busnum, uint32 devicenum);

void
kdump_devicelist()
{
	device_entry *device = devicelist;
	if(!devicelist_valid) {
		kprintf("devicelist is not in a valid state\n");
		return;
	}
	while(device != NULL) {
		kprintf("Device %d on bus %d:\n",
		        (int)device->devicenum, (int)device->busnum);
		kprintf("opencount: %d\n", device->opencount);
		print_ide_device_info((void (*)())kprintf, device->di);
		device = device->next;
	}
}

device_entry *
kget_device_ptr(uint32 busnum, uint32 devicenum)
{
	device_entry *device = devicelist;
	if(!devicelist_valid) {
		kprintf("devicelist is not in a valid state\n");
		return NULL;
	}
	while(device != NULL) {
		if(busnum == device->busnum && devicenum == device->devicenum)
			return device;
		device = device->next;
	}
	kprintf("no deviceentry for device %d on bus %d\n",
	        (int)devicenum, (int)busnum);
	return NULL;
}
