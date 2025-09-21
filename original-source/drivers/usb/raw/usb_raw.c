
// usb_raw.c
//
// Copyright 1999, Be Incorporated
//
// This driver is glue between the prototype USB Kit and the USB Bus Manager.
// The USB Bus Manager API is not finalized.  It can and WILL change.  
// This code is provided for illustration and feedback purposes only.
//

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <USB.h>

#include "usb_raw.h"

#define ddprintf dprintf

static status_t raw_open(const char *name, uint32 flags, void **cookie);
static status_t raw_free (void *cookie);
static status_t raw_close(void *cookie);
static status_t raw_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t raw_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t raw_write(void *cookie, off_t pos, const void *buf, size_t *count);

/* WARNING -- this structure is used to expose private data belonging to the USB
** Bus Manager.  DO NOT MODIFY CONTENTS.  This is a BAD HACK and will go away.
*/
typedef struct
{
	/* Used for the ID list, DO NOT MOVE */
	uint32 signature;
	usb_id id;
	void * next;
	
	void *parent;
	int refcount;
	int active; /* device is live and well */
	
	sem_id lock;
	
	char *name;
	
	void *control;
	void * bus;
	
	usb_device_descriptor descriptor;	
	usb_configuration_info *configuration;
	usb_configuration_info *active_conf;
	
	/* if non-zero this is really a usb_hub */
	uint nchildren;
	
	uint dev_address;
} usb_device0;

typedef struct rawdev
{
	struct rawdev *next;
		
	uint refcount;
	usb_device0 *dev;
	
	/* lock and notify semaphores and scratchpad buffer */
	sem_id lock;
	sem_id notify;
	area_id buffer_id;
	void *buffer;
	
	/* callback return values */
	status_t status;
	size_t actual;
	
	char name[1]; 
} rawdev;

sem_id dev_table_lock = -1;
static char **raw_names = NULL;
static int raw_count = 0;
static rawdev *raw_list = NULL;

status_t alloc_resources(rawdev *d)
{
	if((d->lock = create_sem(1,"usb rawdev lock")) < 0){
		goto ar_err1;
	}
	
	if((d->notify = create_sem(0,"usb rawdev notify")) < 0){
		goto ar_err2;
	}
	
	if((d->buffer_id = create_area("usb rawdev area", &(d->buffer),
								   B_ANY_KERNEL_ADDRESS, 4096, B_CONTIGUOUS,
								   B_READ_AREA | B_WRITE_AREA)) < 0){
		goto ar_err2;
	}
	return B_OK;

ar_err3:
	delete_sem(d->notify);
	d->notify = -1;
ar_err2:
	delete_sem(d->lock);
	d->lock = -1;
ar_err1:
	return B_ERROR;							   
}

void free_resources(rawdev *d)
{
	delete_sem(d->lock);
	delete_sem(d->notify);
	delete_area(d->buffer_id);
	d->lock = -1;
	d->notify = -1;
	d->buffer_id = -1;
	d->buffer = NULL;
}

/* convert USB stack error codes to USB raw device error codes */
status_t usberr2raw(status_t err0)
{
	switch(err0){
		case 0x00: return status_success;
		case 0x02: return status_crc_error;
		case 0x04: return status_timeout;
		case 0x08: return status_stalled;
		case 0x10: return status_aborted;
		
		default: return status_failed;
	}
}

#if 0
#define ID "raw: "
#else
#define ID "\033[32mraw:\033[0m "
#endif

static device_hooks raw_devices = {
	raw_open, 			
	raw_close, 			
	raw_free,			
	raw_control, 		
	raw_read,			
	raw_write			 
};

static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;


static status_t device_added(const usb_id id, void **cookie)
{
	status_t err;
	rawdev *rd;
	const usb_device_descriptor *dd;
	usb_device0	* d = malloc(sizeof(usb_device0));
	
	if (!d) {
		return B_NO_MEMORY;
	}
	
	d->id = id;
	err = usb->usb_ioctl(12, d, sizeof(usb_device0));
	if (err) {
		return err;
	}
	
	ddprintf(ID "device_added(%lu,...)\n",id);
	
	dd = usb->get_device_descriptor(id);
	
	if(!(rd = (rawdev*) malloc(sizeof(rawdev) + strlen(d->name) + 16))){
		return B_ERROR;
	}
	if(dd->device_class == 9){
		sprintf(rd->name,"bus/usb/%s/hub",d->name+1);
	} else {
		sprintf(rd->name,"bus/usb/%s",d->name+1);
	}
	
	rd->dev = d;
		
	acquire_sem(dev_table_lock);
	rd->next = raw_list;
	rd->refcount = 0;
	raw_list = rd;
	raw_count++;
	release_sem(dev_table_lock);
	*cookie = rd;
	
	return B_OK;
}
	
static status_t device_removed(void *cookie)
{
	rawdev *dev,*last;

	acquire_sem(dev_table_lock);
	for(last = NULL, dev = raw_list; dev; dev = dev->next){
		if(dev == ((rawdev*) cookie)){
			if(last){
				last->next = dev->next;
			} else {
				raw_list = dev->next;
			}
			break;
		}
		last = dev;
	}
	raw_count--;
	ddprintf(ID "device_removed(%lu)\n", dev->dev->id);
	free(dev->dev);
	dev->dev = NULL;
	if(!dev->refcount){
		free(dev);
	}	
	release_sem(dev_table_lock);
		
	return B_OK;
}

static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};

_EXPORT
status_t init_hardware(void)
{
	ddprintf(ID "init_hardware()\n");
	return B_OK;
}                   

_EXPORT
status_t
init_driver(void)
{
	int i;
	
	ddprintf(ID " init_driver()\n");

#if 0		
	if(load_driver_symbols("usb_raw") == B_OK) {
		ddprintf(ID "loaded symbols\n");
	} else {
		dprintf(ID "no symbols for you!\n");
	}
#endif
	
	if(get_module(usb_name,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",usb_name);
		return B_ERROR;
	} 
	
	dev_table_lock = create_sem(1,"usb_raw:dev_table");
	ddprintf(ID "USBD loaded okay\n");
	usb->register_driver("usb_raw",NULL,0, NULL);
	usb->install_notify("usb_raw",&notify_hooks);
	return B_OK;
err:
	put_module(usb_name);
	return B_ERROR;
}

_EXPORT
void
uninit_driver(void)
{
	if(raw_names){
		int i;
		for(i=1;raw_names[i];i++) free(raw_names[i]);
		free(raw_names);
	}
	
	ddprintf(ID "uninit_driver()\n"); 
	usb->uninstall_notify("usb_raw");
	delete_sem(dev_table_lock);
	
	if(raw_list){
		dprintf(ID "EEK. devices still exist at teardown\n");
	}
	
	put_module(usb_name);
}

_EXPORT
const char**
publish_devices()
{
	int i,j;
	rawdev *dev;
	ddprintf(ID "publish_devices()\n"); 
	
	if(raw_names){
		for(i=1;raw_names[i];i++) free(raw_names[i]);
		free(raw_names);
	}
	
	acquire_sem(dev_table_lock);	
	raw_names =(char**) malloc(sizeof(char*) * (2 + raw_count));
	raw_names[0] = "bus/usb/unload";
	for(j = 1, dev = raw_list; dev; dev = dev->next){
		if(dev->dev) {
			raw_names[j] = strdup(dev->name);
			ddprintf(ID "publishing '%s'\n",raw_names[j]);
			j++;
		}
	}
	raw_names[j] = NULL;
	release_sem(dev_table_lock);
	return (const char **) raw_names;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
	return &raw_devices;
}

static status_t
raw_open(const char *dname, uint32 flags, void **cookie)
{	
	status_t r = ENODEV;
	rawdev *dev;
	
	ddprintf(ID "opening \"%s\"\n",dname);
	if(!strcmp(dname,"bus/usb/unload")) {
		usb->usb_ioctl(42,NULL,0);
		put_module("bus_managers/usb/v2");
		return ENODEV;
	}
	
	acquire_sem(dev_table_lock);
	for(dev = raw_list; dev; dev = dev->next){
		if(dev->dev && !strcmp(dname,dev->name)){
			if(dev->refcount){
				/* disallow multiple open of a USB device */
				r = ENODEV;
			} else {
				r = alloc_resources(dev);
				if(r == B_OK) {
					dev->refcount++;
					*cookie = dev;
				}
			}
			break;
		}		
	}
	release_sem(dev_table_lock);
	return r;
}

 
static status_t
raw_free (void *cookie)
{
	rawdev *d = (rawdev *)cookie;
	
	acquire_sem(dev_table_lock);
	d->refcount--;
	free_resources(d);
	if(!d->refcount && !d->dev){
		free(d);
	}
	release_sem(dev_table_lock);
	return B_OK;
}


static status_t
raw_close(void *cookie)
{
	return B_OK;
}



static status_t
raw_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	rawdev *d = (rawdev *)cookie;
	*count = 0;
	return B_OK;
}


static status_t
raw_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	rawdev *d = (rawdev *)cookie;
//	*count = 0;
	return B_OK;
}

void cb_control(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	rawdev *d = (rawdev *) cookie;
	d->actual = actual_len;
	d->status = status;
	release_sem(d->notify);
}
				
#include <Errors.h>
static status_t
raw_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	status_t ret = B_DEV_INVALID_IOCTL;
	status_t status;
	rawdev *d = (rawdev *)cookie;
	usb_raw_command *cmd = (usb_raw_command *) arg1;
	
	if(!d->dev) return ENODEV;
	
	ret = B_OK;	
	switch(msg){
	case check_version:
		status = USB_RAW_DRIVER_VERSION;
		break;
		
	case get_device_descriptor: {
		const usb_device_descriptor *dd = usb->get_device_descriptor(d->dev->id);
		memcpy(cmd->device.descr, dd, sizeof(usb_device_descriptor));
		status = status_success;
		break;
	}	
	
	case get_configuration_descriptor:{
		const usb_configuration_info *ci = 
		usb->get_nth_configuration(d->dev->id,cmd->config.config_num);
		if(ci) {
			memcpy(cmd->config.descr,ci->descr,sizeof(usb_configuration_descriptor));
			status = status_success;
		} else {
			status = status_invalid_configuration;
		}
		break;
	}
	
	case get_interface_descriptor:{
		const usb_configuration_info *ci = 
		usb->get_nth_configuration(d->dev->id,cmd->interface.config_num);
		if(!ci){
			status = status_invalid_configuration;
			break;
		}
		if(cmd->interface.interface_num < ci->interface_count) {
			memcpy(cmd->interface.descr,ci->interface[cmd->interface.interface_num].active->descr,
				   sizeof(usb_interface_descriptor));
			status = status_success;
		} else {
			status = status_invalid_interface;
		}
		break;
	}
	
	case get_endpoint_descriptor:{
		const usb_configuration_info *ci = 
		usb->get_nth_configuration(d->dev->id,cmd->endpoint.config_num);
		if(!ci){
			status = status_invalid_configuration;
			break;
		}
		if(cmd->endpoint.interface_num < ci->interface_count) {
			const usb_interface_info *ii = ci->interface[cmd->endpoint.interface_num].active;
			if(cmd->endpoint.endpoint_num < ii->endpoint_count){
				memcpy(cmd->endpoint.descr, ii->endpoint[cmd->endpoint.endpoint_num].descr,
					   sizeof(usb_endpoint_descriptor));
				status = status_success;
			} else {
				status = status_invalid_endpoint;
			}
		} else {
			status = status_invalid_interface;
		}
		break;
	}
	
	case get_descriptor:{
		size_t act;
		char *x;
		if(x = (char *) malloc(256)){
			dprintf("here goes nothing\n");
			
			if(status = usb->send_request(d->dev->id, 0x81, 0x06,
										  (cmd->descr.type << 8 )| cmd->descr.index,
										  cmd->descr.lang, cmd->descr.length,
										  x, &act)){
#if 0									 
			if(usb->get_descriptor(d->dev->id, cmd->descr.type, cmd->descr.index, 
								   cmd->descr.lang, x, cmd->descr.length, &act)){
#endif
				dprintf("suckage: %02x\n",status);
									   
				status = status_aborted;
			} else {
				dprintf("success! %d (%d)\n",act,cmd->descr.length);
				cmd->descr.length = act;
				status = status_success;
				memcpy(cmd->descr.data,x,act);
			}
			free(x);
		}			   
		break;
	}
	
	case get_string_descriptor:{
		size_t act;
		int sz;
		char *x;
		if(x = (char *) malloc(256)){
			if(usb->get_descriptor(d->dev->id, USB_DESCRIPTOR_STRING, cmd->string.string_num, 0, x, 2, &act)){
				status = status_aborted;
			} else {
				sz = *((unsigned char *)x);
				if(usb->get_descriptor(d->dev->id, USB_DESCRIPTOR_STRING, cmd->string.string_num, 0, x, sz, &act)){
					status = status_aborted;
					cmd->string.length = 0;
				} else {
					if(act > cmd->string.length) act = cmd->string.length;
					cmd->string.length = act;
					status = status_success;
					memcpy(cmd->string.descr,x,act);
				}
			}
			free(x);
		}			   
		break;
	}
	
	case get_other_descriptor:{
		const usb_configuration_info *ci = 
		usb->get_nth_configuration(d->dev->id,cmd->other.config_num);
		if(!ci){
			status = status_invalid_configuration;
			break;
		}
		if(cmd->other.interface_num < ci->interface_count) {
			const usb_interface_info *ii = ci->interface[cmd->other.interface_num].active;
			if(cmd->other.other_num < ii->generic_count){
				usb_descriptor *descr = ii->generic[cmd->other.other_num];
				if(descr->generic.length > cmd->other.length){
					status = status_not_enough_space;
				} else {
					memcpy(cmd->other.descr, descr, descr->generic.length);
					status = status_success;
				}
			} else {
				status = status_invalid_endpoint;
			}
		} else {
			status = status_invalid_interface;
		}
		break;
	}
	
	case set_configuration:{
		const usb_configuration_info *ci = usb->get_nth_configuration(d->dev->id,cmd->config.config_num);
		if(ci){
			if(usb->set_configuration(d->dev->id, ci)){
				status = status_invalid_configuration;
			} else {
				status = status_success;
			}
		} else {
			status = status_invalid_configuration;
		}
		break;
	}
	
	case do_control:{
		if(cmd->control.length > 4096) {
			status = status_failed;
			break;
		}
		acquire_sem(d->lock);
		memcpy(d->buffer,cmd->control.data,cmd->control.length);
		if(status = usb->queue_request(d->dev->id, cmd->control.request_type, 
									   cmd->control.request, cmd->control.value, 
									   cmd->control.index, cmd->control.length,
									   d->buffer, cb_control, d)){
			status = usberr2raw(d->status);
		} else {
			acquire_sem(d->notify);
			status = usberr2raw(d->status);
			memcpy(cmd->control.data,d->buffer,cmd->control.length);
			cmd->control.length = d->actual;
		}
		release_sem(d->lock);
		break;
	}
	
	case do_interrupt:
	case do_bulk:
	{
		const usb_configuration_info *cfg;
		const usb_interface_info *ifc;
		const usb_endpoint_info *ept;
		
		if(!(cfg = usb->get_configuration(d->dev->id))){
			status = status_invalid_configuration;
			break;
		}
		if(cmd->interrupt.interface >= cfg->interface_count){
			status = status_invalid_interface;
			break;
		}
		ifc = cfg->interface[cmd->interrupt.interface].active;
		if(cmd->interrupt.endpoint >= ifc->endpoint_count){
			status = status_invalid_endpoint;
			break;
		}
		ept = &(ifc->endpoint[cmd->interrupt.endpoint]);
		if(!ept->handle){
			status = status_invalid_endpoint;
			break;
		}
		
		if(cmd->interrupt.length > 4096) {
			status = status_failed;
			break;
		}
		
		acquire_sem(d->lock);
		memcpy(d->buffer,cmd->interrupt.data,cmd->interrupt.length);
		if(msg == do_interrupt){
			status = usb->queue_interrupt(ept->handle, d->buffer, cmd->interrupt.length,
										  cb_control, d);
		} else {
			status = usb->queue_bulk(ept->handle, d->buffer, cmd->interrupt.length,
									 cb_control, d);
		}
										 
		if(status){
			status = usberr2raw(status);
		} else {
			if(acquire_sem_etc(d->notify,1,B_CAN_INTERRUPT,0)){
				/* timeout or abort */
				usb->cancel_queued_transfers(ept->handle);
				acquire_sem(d->notify);
			}
			memcpy(cmd->interrupt.data,d->buffer,cmd->interrupt.length);
			status = usberr2raw(d->status);
			cmd->interrupt.length = d->actual;
		}
		release_sem(d->lock);
		break;
	}
	
	default:
		ret = B_DEV_INVALID_IOCTL;
	}
	
	if(ret == B_OK) cmd->generic.status = status;
	return ret;
}



