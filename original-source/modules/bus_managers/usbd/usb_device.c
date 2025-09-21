#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "usbd.h"

#if __GNUC__
#define ID "usbd: [" __FUNCTION__ "] "
#else
#define ID "usbd: "
#endif

#define _USBD_DEBUG 0

#if _USBD_DEBUG
#define ddprintf(x...)	dprintf(x)
#else
#define ddprintf(x...)
#endif

#define CHECK_DEVICE(d) if(SIG_CHECK_DEVICE(d)) return B_BAD_VALUE;


const usb_device_descriptor *get_device_descriptor(const usb_id dev_id)
{
	bm_usb_device * d;
	d = id_to_device(dev_id);

	if(SIG_CHECK_DEVICE(d)) return NULL;
	return &(d->descriptor);
}

const usb_configuration_info *get_nth_configuration(const usb_id dev_id, uint index)
{
	bm_usb_device * dev;
	dev = id_to_device(dev_id);
	
	if(SIG_CHECK_DEVICE(dev)) return NULL;
	if(index >= dev->descriptor.num_configurations) return NULL;
	return &(dev->configuration[index]);
}

const usb_configuration_info *get_configuration(const usb_id dev_id)
{
	bm_usb_device * dev;
	dev = id_to_device(dev_id);
	
	if(SIG_CHECK_DEVICE(dev)) return NULL;
	return dev->active_conf;
}

status_t get_descriptor(const usb_id dev_id, 
						uint8 type, uint8 index, uint16 lang,
						void *data, size_t len, size_t *actual_len)
{
	return send_request(dev_id, USB_REQTYPE_DEVICE_IN, 
						USB_REQUEST_GET_DESCRIPTOR, 
						((type<<8) | index), lang, len,
						data, actual_len);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#if 0
status_t set_configuration(const usb_id dev_id, 
						   const usb_configuration_info *conf)
{
	status_t status;
	bm_usb_device *d = id_to_device(dev_id);
	usb_interface_info * ifc;
	uint i,j;
		
	if (acquire_sem(d->lock) != B_OK) {
		dprintf(ID "error acquiring semaphore\n");
		return B_ERROR;
	}
	
	if (d->active_conf != conf) {
		/* tear down old config */
		ddprintf(ID "changing conf from %p to %p\n", d->active_conf, conf);
		status = tear_down_endpoints(d, false);
		if (status) {
			dprintf(ID "error in tear_down_endpoints: %s\n", strerror(status));
			goto error;
		}
	} else {
		ddprintf(ID "not changing conf\n");
	}
		
	/* Tell device to change its config */
	status = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, 
						  USB_REQUEST_SET_CONFIGURATION, 
						  conf ? conf->descr->configuration_value : 0,
						  0, 0, NULL, NULL);
	
	if(status) {
		dprintf(ID "error with send_request: %s\n", strerror(status));
		goto error;
	}
	
	/* Create the pipes if we're not unconfigured */
	if(conf){
		/* For each interface */
		for(i = 0; i < conf->interface_count; i++) {
			/* For each alternate to this interface */
			for(j = 0; j < conf->interface[i].alt_count; j++) {
				ddprintf("CONFIG: alt interface[%d]\n", j);
				ifc = &conf->interface[i].alt[j];
				
				/* Set the active interface to 0 */
				if (j == 0) {
					bm_usb_interface * interface;
					
					/* Activate the default alt interface */
					if (conf->interface[i].alt[j].handle == 0) {
						/* Create the resources for this interface */
						status = create_interface(d, ifc, &interface);
						if (status) {
							dprintf(ID "error with create_interface: %s\n", strerror(status));
							goto error;
						}
						
						status = activate_interface(interface);
						if (status) {
							dprintf(ID "error with activate_interface: %s\n", strerror(status));
							goto error;
						}
						
						conf->interface[i].alt[j].handle = interface->id;
						conf->interface[i].active = ifc;
					}
				} else {
					conf->interface[i].alt[j].handle = 0;
				}
			}
		}
	}
	
	d->active_conf = (usb_configuration_info *) conf;
		
error:
	release_sem(d->lock);
	return status;
}


status_t set_alt_interface(const usb_id dev_id, const usb_interface_info *ifc)						   
{
	uint i,j,k;
	usb_configuration_info *cfg;
	status_t err;
	bm_usb_device * dev;
	
	dev = id_to_device(dev_id);
	if (!dev || !ifc) {
		return B_BAD_VALUE;
	}
	
	for(i = 0; i < dev->descriptor.num_configurations; i++) {
		cfg = dev->configuration + i;
		for(j = 0; j < cfg->interface_count; j++) {
			for(k = 0; k < cfg->interface[j].alt_count; k++) {
				if((cfg->interface[j].alt + k) == ifc) {
					goto got_one;
				}
			}
		}
	}
	
	/* The interface info was not found */
	return B_ENTRY_NOT_FOUND;
	
got_one:
	/* Keep track of which interface is active */
	if (cfg->interface[j].active != ifc) {
		usb_id	ifc_id = cfg->interface[j].active->handle;
		bm_usb_interface * interface = id_to_interface(ifc_id);
		
		if(!ifc) {
			dprintf(ID "error in id_to_interface\n");
			return B_ERROR;
		}
		
		err = destroy_interface(interface);
		if (err) {
			dprintf(ID "error in destroy_interface: %s\n", strerror(err));
			return err;
		}
		
		err = create_interface(dev, ifc, &interface);
		if (err) {
			dprintf(ID "error in create_interface: %s\n", strerror(err));
			return err;
		}
		cfg->interface[j].active = (usb_interface_info *) ifc;
		cfg->interface[j].active->handle = interface->id;
	}
		
	/* Tell the device to change */
	ddprintf(ID "Changing to configuration[%d], alternate setting 0x%02x\n",i,cfg->interface[j].active->descr->alternate_setting);
	err = send_request(dev_id, USB_REQTYPE_INTERFACE_OUT, USB_REQUEST_SET_INTERFACE,
					ifc->descr->alternate_setting, k, 0, NULL, NULL);
	if (err != B_OK) {
		dprintf(ID "problem with send_request\n");
		return err;
	}
	
	return B_OK;
}
#else


status_t set_configuration(const usb_id dev_id, 
						   const usb_configuration_info *conf)
{
	bm_usb_device *d = id_to_device(dev_id);
	status_t err;
		
	if (acquire_sem(d->lock) != B_OK) {
		dprintf(ID "error acquiring semaphore\n");
		return B_ERROR;
	}

	if(d->active_conf != conf)
	{
		usb_interface_list* ifc_list = NULL;

		/* create the new configuration: create all interfaces */
		if(conf)
		{
			/* For each interface */
			for(ifc_list = conf->interface; ifc_list < conf->interface + conf->interface_count; ifc_list++)
			{
				if(ifc_list->active == NULL)	/* set_alt_interface() was not called before the first set_configuration() */
				{
					/* ASSERT(ifc_list->alt[0]->descr->alternate_setting == 0); FIXME uncomment ASSERT */
					ifc_list->active = &ifc_list->alt[0];
				}
				/* create active interface and its pipes/endpoints */
				err = create_interface(d, ifc_list->active, NULL);
				if(err != B_OK)
				{
					dprintf(ID "error with create_interface(): %s\n", strerror(err));
					goto err0;
				}
			}

			err = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, USB_REQUEST_SET_CONFIGURATION, conf->descr->configuration_value ,  0, 0, NULL, NULL);
			if(err != B_OK)
			{
				dprintf(ID "error with SetConfiguration(%u): %s\n", conf ? conf->descr->configuration_value : 0, strerror(err));
				goto err0;
			}

			/* For each interface */
			for(ifc_list = conf->interface; ifc_list < conf->interface + conf->interface_count; ifc_list++)
			{
				const usb_interface_descriptor* const	ifc_descr = ifc_list->active->descr;
				if(ifc_descr->alternate_setting != 0)
				{
					err = send_request(dev_id, USB_REQTYPE_INTERFACE_OUT, USB_REQUEST_SET_INTERFACE,
						ifc_descr->alternate_setting, ifc_descr->interface_number, 0, NULL, NULL);
					if(err != B_OK)
					{
						dprintf(ID "error with SetInterface(ifc %u, alt %u): %s\n", ifc_descr->interface_number, ifc_descr->alternate_setting, strerror(err));
						goto err0;
						/* BUGBUG it's a fatal error, there is no recovery at this point, so we need to destroy everything
						and unconfigure the device */
					}
				}
			}
		}
		else
		{
			err = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, USB_REQUEST_SET_CONFIGURATION, 0,  0, 0, NULL, NULL);
			if(err != B_OK)
			{
				dprintf(ID "error with SetConfiguration(0): %s\n", strerror(err));
				goto err0;
			}
		}
		
		if(d->active_conf)
		{
			/* tear down old config */
			err = tear_down_endpoints(d, false);
			if(err != B_OK)	/* FIXME change to ASSERT(): should not return an error! */ 
				dprintf(ID "error in tear_down_endpoints: %s\n", strerror(err));
		}
		
		d->active_conf = (usb_configuration_info*)conf;
		release_sem(d->lock);
		return B_OK;
	
	err0:	/* destroy all new interfaces */
		if(conf != NULL)
		{
			usb_interface_list* new_ifc_list;
			/* For each interface */
			for(new_ifc_list = conf->interface; new_ifc_list < ifc_list; new_ifc_list++)
			{
				status_t err_2;
				err_2 = destroy_interface(id_to_interface(new_ifc_list->active->handle));
				if(err_2 != B_OK)
					dprintf(ID "error with destroy_interface(): %s\n", strerror(err_2));
				new_ifc_list->active = NULL;	
			}
		}
		release_sem(d->lock);
		return err;
	}
	else	/* the new config is the same as the old config */
	{
		if(conf)
		{
			usb_interface_list* ifc_list;
			/* Tell the device to set the same config/reset data toggle in all endpoints. */
			err = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, USB_REQUEST_SET_CONFIGURATION, 
				conf->descr->configuration_value,  0, 0, NULL, NULL);
			if(err != B_OK)
			{
				dprintf(ID "error with SetConfiguration(): %s\n", strerror(err));
				return err;
			}
			/* For each interface */
			for(ifc_list = conf->interface; ifc_list < conf->interface + conf->interface_count; ifc_list++)
				reset_data_toggle_for_interface(id_to_interface(ifc_list->active->handle));
		}
		release_sem(d->lock);
		return B_OK;
	}
}



/*
If setting the new interface produces an error (for example there is no bandwith for the new interface)
then the old interface and its endpoits/pipes have to remain valid!
*/
status_t set_alt_interface(const usb_id dev_id, usb_interface_info* ifc_info)						   
{
	uint conf_num, ifc_num, alt_num;
	usb_configuration_info	*cfg;
	usb_interface_list		*ifc_list;
	bm_usb_device 			*dev;
	status_t err;
	
	dev = id_to_device(dev_id);
	if (!dev || !ifc_info)
		return B_BAD_VALUE;
	
	for(cfg = dev->configuration; cfg < dev->configuration + dev->descriptor.num_configurations; cfg++)
	{
		for(ifc_list = cfg->interface; ifc_list < cfg->interface + cfg->interface_count; ifc_list++)
		{
			const usb_interface_info*	alt_ifc_info;
			for(alt_ifc_info = ifc_list->alt; alt_ifc_info < ifc_list->alt + ifc_list->alt_count; alt_ifc_info++)
			{
				if(alt_ifc_info == ifc_info)
					goto got_one;
			}
		}
	}
	
	/* The interface info was not found */
	return B_ENTRY_NOT_FOUND;
	
got_one:
	/*
	if the device is configured
		create the new interface,
		if successfull
			send SetInterface to the device.
			destroy the old active interface
			remember the new alt setting: set active pointer
	
	else (the device is not configured)
		remember the new alt setting: set active pointer
	*/
	if(dev->active_conf != NULL)
	{
		usb_interface_info*	const	old_active_ifc_info = ifc_list->active;
		bm_usb_interface* const old_active_bm_ifc = id_to_interface(old_active_ifc_info->handle);

		if(ifc_info != old_active_ifc_info)
		{
			bm_usb_interface*	new_bm_ifc;
			
			err = create_interface(dev, ifc_info, &new_bm_ifc);
			if(err)
			{
				dprintf(ID "error in create_interface: %s\n", strerror(err));
				goto err0;
			}
			
			/* Tell the device to change */
			dprintf(ID "Changing ifc %u from alt %u to %u\n",
				ifc_info->descr->interface_number, old_active_ifc_info->descr->alternate_setting, ifc_info->descr->alternate_setting);
			err = send_request(dev_id, USB_REQTYPE_INTERFACE_OUT, USB_REQUEST_SET_INTERFACE,
						ifc_info->descr->alternate_setting, ifc_info->descr->interface_number, 0, NULL, NULL);
			if(err != B_OK)
			{
				dprintf(ID "error in SetInterface: %s\n", strerror(err));
				goto err1;
			}
			
			/* destroy the old active interface */
			err = destroy_interface(old_active_bm_ifc);
			if(err != B_OK)
			{
				dprintf(ID "error in destroy_interface(): %s\n", strerror(err));
				/* It is not a fatal error, keep going.  */
			}
			/* remember the new alt setting: set active pointer */
			ifc_list->active = (usb_interface_info*)ifc_info; 

			return B_OK;

		err1:
			destroy_interface(new_bm_ifc);
		err0:	
			return err;
		}
		else	/* just send SetInterface to the device, it resets data toogle in all interface endpoints */
		{
			
			err = send_request(dev_id, USB_REQTYPE_INTERFACE_OUT, USB_REQUEST_SET_INTERFACE,
				ifc_info->descr->alternate_setting, ifc_info->descr->interface_number, 0, NULL, NULL);
			if(err != B_OK)
			{
				dprintf(ID "error in SetInterface: %s\n", strerror(err));
				return err;
			}
			/* reset HCD data toggle for endpoints */
			reset_data_toggle_for_interface(old_active_bm_ifc);
		}
	}
	else
	{
		/* remember the new alt setting: set active pointer */
		ifc_list->active = (usb_interface_info*)ifc_info; 
	}
	return B_OK;
}


#endif


status_t set_feature(const usb_id id, uint16 selector)
{
	int8			request_type, index;
	status_t		status;
	size_t			act;
	bm_usb_device		* dev;
	bm_usb_pipe		* pipe;
	usb_id			dev_id;
	
	/* The request type needs to be set based on the expected
	** recipient, see USB 1.1 spec Table 9-6 (pg 188)
	*/
	pipe = id_to_pipe(id);
	if (pipe) {
		if (!pipe->ifc->dev) {
			return B_DEV_INVALID_PIPE;
		}
		dev_id = pipe->ifc->dev->id;
		request_type = USB_REQTYPE_ENDPOINT_OUT;
		index = pipe->addr_flags & PIPE_ADDR_MASK;
	} else {
		dev = id_to_device(id);
		if (dev) {
			request_type = USB_REQTYPE_DEVICE_OUT;
			index = 0;
			dev_id = id;
		} else {
			return B_BAD_VALUE;
		}
	}
	
	status = send_request(dev_id, request_type, USB_REQUEST_SET_FEATURE, selector,
						  index, 0, NULL, &act);
	
	return status;
}

status_t clear_feature(const usb_id id, uint16 selector)
{
	int8			request_type, index;
	status_t		status;
	size_t			act;
	bm_usb_device		* dev;
	bm_usb_pipe		* pipe;
	usb_id			dev_id;
	
	/* The request type needs to be set based on the expected
	** recipient, see USB 1.1 spec Table 9-6 (pg 188)
	*/
	pipe = id_to_pipe(id);
	if (pipe) {
		if (!pipe->ifc->dev) {
			return B_DEV_INVALID_PIPE;
		}
		dev = pipe->ifc->dev;
		dev_id = pipe->ifc->dev->id;
		request_type = USB_REQTYPE_ENDPOINT_OUT;
		index = pipe->addr_flags & PIPE_ADDR_MASK;
	} else {
		dev = id_to_device(id);
		if (dev) {
			request_type = USB_REQTYPE_DEVICE_OUT;
			index = 0;
			dev_id = id;
		} else {
			return B_BAD_VALUE;
		}
	}
	
	status = send_request(dev_id, request_type, USB_REQUEST_CLEAR_FEATURE, selector,
						  index, 0, NULL, &act);
	
	if (status) {
		return status;
	}
	
	/* Reset the pipe's data toggle if we have cleared a halt */
	if (selector == USB_FEATURE_ENDPOINT_HALT && pipe) {
		status = dev->bus->ops->reset_data_toggle(pipe->ept);
	}
	
	return status;
}

status_t get_status(const usb_id id, uint16 *status_in)
{
	int8			request_type, index;
	status_t		status;
	size_t			act;
	bm_usb_device		* dev;
	bm_usb_pipe		* pipe;
	usb_id			dev_id;
	
	/* The request type needs to be set based on the expected
	** recipient, see USB 1.1 spec Table 9-6 (pg 188)
	*/
	pipe = id_to_pipe(id);
	if (pipe) {
		if (!pipe->ifc->dev) {
			return B_DEV_INVALID_PIPE;
		}
		dev_id = pipe->ifc->dev->id;
		request_type = USB_REQTYPE_ENDPOINT_OUT;
		index = pipe->addr_flags & PIPE_ADDR_MASK;
	} else {
		dev = id_to_device(id);
		if (dev) {
			request_type = USB_REQTYPE_DEVICE_OUT;
			index = 0;
			dev_id = id;
		} else {
			return B_BAD_VALUE;
		}
	}
	
	status = send_request(dev_id, request_type, USB_REQUEST_GET_STATUS, 0,
						  index, 2, status_in, &act);
	
	return status;
}

status_t set_address(const usb_id dev_id)
{
	status_t status;
	usb_endpoint_descriptor ctrl_ep0;
	bm_usb_pipe *pipe;
	int new_addr;
	bm_usb_device * d;
	bm_usb_interface * ifc;
	
	d = id_to_device(dev_id);
	if (!d) {
		return B_BAD_VALUE;
	}
	
	new_addr = get_address(d->bus);
	if (new_addr < 0) {
		dprintf(ID "get_address failed!\n");
		return B_ERROR;
	}
	
	dprintf(ID "setting address %d for device %p\n", new_addr, d);
	
	ctrl_ep0.length = 8;
	ctrl_ep0.descriptor_type = USB_DESCRIPTOR_ENDPOINT;
	ctrl_ep0.endpoint_address = 0;
	ctrl_ep0.attributes = 0;
	ctrl_ep0.max_packet_size = d->descriptor.max_packet_size_0;
	ctrl_ep0.interval = 0;
	
	status = create_interface(d, NULL, &ifc);
	if (status) {
		goto error;
	}
	
	status = create_pipe(d->bus, &pipe, &ctrl_ep0, new_addr, 
						 d->control->addr_flags & PIPE_LOWSPEED ? 1 : 0, ifc);
	if(status) {
		put_address(d->bus, new_addr);
		goto error;
	}
	
	status = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, USB_REQUEST_SET_ADDRESS, 
						  new_addr, 0, 0, NULL, NULL);
	if(status){
		goto error2;
	}
	
	d->control = pipe;
	d->dev_address = new_addr;
	
	return B_OK;

error2:
	destroy_pipe(pipe);
error:
	put_address(d->bus, new_addr);
	return status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void get_device(bm_usb_device *dev)
{
	if(acquire_sem(dev->lock) == B_OK){
		dev->refcount++;
		dprintf(ID "device %s refcount++, now %d\n",dev->name,dev->refcount);
		release_sem(dev->lock);
	} else {
		dprintf(ID "device %p is dead?\n",dev);
	}
}

void put_device(bm_usb_device *dev)
{
	if(acquire_sem(dev->lock) == B_OK){
		dev->refcount--;
		dprintf(ID "device %s refcount--, now %d\n",dev->name,dev->refcount);
		if(dev->refcount <= 0){
			/* refcount hit zero - no more owners */
			if(dev->active) {
				/* it's still phsyically on the bus, just reset to unconfigured */
				dprintf(ID "unconfiguring\n");
				release_sem(dev->lock);
				/* XXX - race condition */
				set_configuration(dev->id, NULL);
			} else {
				/* it's dead. bury it */
				release_sem(dev->lock);
				delete_device(dev);
			}
		} else {
			release_sem(dev->lock);
		}
	}
}

void deactivate_device(bm_usb_device * d)
{
	bool	notify = false;
	
	dprintf(ID "putting %p, which is ID %lu\n", d, d->id);
	
	if (acquire_sem(d->lock) != B_OK) {
		return;
	}
	
	if(d->active){
		notify = true;
		dprintf(ID "deleting %s\n",d->name);
		if(d->nchildren){
			/* this device is a hub */
			uint i;
			for(i=0;i<d->nchildren;i++){
				if(((usb_hub*)d)->children[i]) {
					deactivate_device(((usb_hub*)d)->children[i]);
				}
			}
			d->refcount--; /* we hold a refcount on hubs, release it */
		}
	}
	
	release_sem(d->lock);
	if(notify) {
		do_notify(d, USB_DEVICE_REMOVED);
	}
	MCHECK(); // BUGBUG
	return;
}

/*	delete_device
**	Make sure to call device_removed hooks for any drivers that have an
**	interest in this device
*/
void delete_device(bm_usb_device *d)
{
	int i;
	
	if (!d) {
		return;
	}
	
	if (acquire_sem(d->lock) != B_OK) {
		return;
	}
	
	if(d->refcount <= 0 && !d->active){
		dprintf(ID "deleted device %p: %s\n", d, d->name);
		put_address(d->bus, d->dev_address);
		
		/* XXX - usbd hangs if the root hub is removed */
		if(d->parent) {
			tear_down_endpoints(d, true);
			put_usb_id(d);
		}
		
		delete_sem(d->lock);
		for(i=0;i<d->descriptor.num_configurations;i++){
			free((d->configuration + i)->interface);
		}
		free(d->name);
		/* XXX we leak memory here. not good */
		free(d);
	} else {
		release_sem(d->lock);
		dprintf(ID "cannot delete device %s yet, it is %sactive and refcount is %d\n",d->name, (d->active ? "" : "not "), d->refcount);
	}
	MCHECK(); // BUGBUG
}

/* only safe to call while device lock is held or device is otherwise unavailable */
status_t tear_down_endpoints(bm_usb_device *d, bool destroy_control_pipe)
{
	uint i;
	status_t err;
	
	CHECK_DEVICE(d);
	
	/* tear down old config */
	if(d->active_conf){
		/* For each interface */
		for(i = 0; i < d->active_conf->interface_count; i++) {
			/* Tear down the active alternate */
			usb_interface_info * ii = d->active_conf->interface[i].active;
			bm_usb_interface * ifc;
			
			if (ii) {
				/* Take down the interface, which also destroys the endpoints */
				ifc = id_to_interface(ii->handle);
				if (ifc) {
					err = destroy_interface(ifc);
					if (err) {
						dprintf(ID "error in destroy_interface: %s\n", strerror(err));
						return err;
					}
				}
				ii->handle = 0;
			}
		}
	}
	
	if (destroy_control_pipe) {
		bm_usb_interface * ifc = d->control->ifc; /* HACK FIXME put default control pipe into the interface? */ 
		destroy_pipe(d->control);
		destroy_interface(ifc);
	}
		
	return B_OK;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Step to the next descriptor in a linear buffer of descriptors.
** Return the descr pointer and allocate the bufsiz (space remaining)
** returns null if the new descr is invalid (bigger than buffer)
*/
static usb_descriptor *d_next(usb_descriptor *d, size_t *bufsiz)
{
	uchar *x = (uchar *) d;
	x += d->generic.length;
	*bufsiz -= d->generic.length;
	if(*bufsiz < 2) return NULL;
	d = (usb_descriptor *) x;
	if(d->generic.length > *bufsiz) return NULL;
	return d;
}

/*

Okay, the following function is pretty scary and does a couple of weird things 
to  avoid having to do a ton of mallocs to allocate the rather complex 
usb_configuration_info structure

First an array of configuration_infos are allocated based on the config
count in the device descriptor, then for each configuration:

1. it mallocs a chunk to contain the configuration_info struct, an array of
   interface_list structs, and the entire raw chunk of descriptors once it
   knows the total size and interface count (both are in the initial config
   descr which is read)

2. the entire block of descriptors is read

3. it scans all the descriptors and tabulates:
     - the number of interface descriptors (and alt counts per interface)
     - the number of endpoint descriptors
     - the number of other descriptors

4. then a single malloc is done to allocate storage for all those objects

5. Another scan of the descriptors is used to assemble the actual data
   structures

To release the memory in use by a usb_configuration_info, first you free
ci->interface[0].alt which will release the second malloc'd chunk then
free ci itself to release the first chunk.

|                        |
+------------------------+
| usb_configuration_info |
|                descr --|----.
|            interface --|--. |
+------------------------+  | |
|                        |  | |
                            | |
                            | |
+------------------------+<-` | 
| usb_interface_list     |    |
| alt_count = 2    alt --|----|------>+---------------------+            ---
+------------------------+    |       | usb_interface_info  |             ^  i
| usb_interface_list     |    |    .--|-- descr             |             |  c
| alt_count = 1    alt --|----|--. |  | ecount=1 endpoint --|--.             o
+------------------------+<---`  | |  |           generic=0 |  |             u
| raw descriptor data    |       | |  +---------------------+  |             n
|                        |<--------`  | usb_interface_info  |  |             t
|                        |<-----------|-- descr             |  |           
|                        |<----. |    | ecount=2 endpoint --|--|-. 
|                        |     | |    | gcount=3  generic --|--|-|---.
|                        |<--. | `--->+---------------------+  | |   |
|                        |   | |      | usb_interface_info  |  | |   |
|                        |   | `------|-- descr             |  | |   |     
|                        |   |        | ecount=1 endpoint --|--|-|-. |    |
|                        |<--|--.     | gcount=2  generic --|--|-|-|-|-.  v
|                        |   |  |     +---------------------+<-' | | | | ---
|                        |   |  |     | usb_endpoint_info   |    | | | |  ^  e
+------------------------+   `--|-----|-- descr             |    | | | |  |  c
                                |     +---------------------+<---' | | |     o
                                |     | usb_endpoint_enfo   |      | | |     u
                                |  ...|-- descr             |      | | |     n
                                |     +---------------------+      | | |     t
                                |     | usb_endpoint_enfo   |      | | |
                                |  ...|-- descr             |      | | |
                                |     +---------------------+<-----' | |
                                |     | usb_endpoint_enfo   |        | |  |
                                |  ...|-- descr             |        | |  v
                                |     +---------------------+<-------' | ---
                                `-----|- usb_generic_descr* |          |  ^  g
                                      |  usb_generic_descr* |          |  |  c
                                      |  usb_generic_descr* |          |     o
                                      |  usb_generic_descr* |<---------'  |  u
                                      |  usb_generic_descr* |             v  n
                                      +---------------------+            --- t
*/            

int load_configs(bm_usb_device *d)
{
	uint realsize, i, j, k;
	uchar *raw;
	usb_descriptor *ds;
	usb_configuration_info *ci;
	usb_interface_info *ii,*li;
	usb_endpoint_info *ei;
	usb_descriptor **generic;
	int e_count,i_count,g_count;
	size_t act;
	
	uint count = d->descriptor.num_configurations;
	d->configuration = (usb_configuration_info *) malloc(sizeof(usb_configuration_info)*count +
														 sizeof(usb_configuration_descriptor));
	
	for(i=0;i<count;i++){
		dprintf(ID "For loop, i = %d, count = %d\n", i, count);
		
		/* use the scratch buffer for the initial read of just the config descr */
		raw = ((uchar *) d->configuration) + (sizeof(usb_configuration_info)*count);
		ci = d->configuration + i;
		
		ci->descr = NULL;
		ci->interface_count = 0;
		
		if(get_descriptor(d->id, USB_DESCRIPTOR_CONFIGURATION, i, 0, raw, 
						  sizeof(usb_configuration_descriptor), &act)){
			dprintf(ID "failed to config descriptor %d\n",i);
			goto fail;
		}
		if(act != sizeof(usb_configuration_descriptor)) {
			dprintf(ID "error, act = %ld, sizeof(...) = %ld\n", act, sizeof(usb_configuration_descriptor));
			continue;
		}
		realsize = ((usb_configuration_descriptor *) raw)->total_length;
		ddprintf("got config descr - actual length = %d\n",realsize);
		
		ci->interface_count = ((usb_configuration_descriptor *) raw)->number_interfaces;
		
		raw = malloc((sizeof(usb_interface_list) * ci->interface_count) + realsize);
					
		ci->interface = (usb_interface_list *) raw;
		raw = (void*)  ( ((uchar*)raw) + (sizeof(usb_interface_list) * ci->interface_count) );
		ci->descr = (usb_configuration_descriptor*) raw;
					
		if(get_descriptor(d->id, USB_DESCRIPTOR_CONFIGURATION, i, 0, raw, realsize, &act)){
			dprintf(ID "failed to config descriptor %d\n",i);
			free(ci->interface);
			goto fail;
		}
		if(realsize != act){
			dprintf(ID "bad buffer?!\n");
			goto fail;
		}
		
		dprintf(ID "got real config descr. prepare to parse\n");
//		dump_buf(raw,realsize);		
		
		/* zero all interface[n].alt_count */
		memset(ci->interface, 0, sizeof(usb_interface_list)*ci->interface_count);
		
		/* count the interfaces, endpoints, and the alt interfaces per interface */
		j = 0; /* active interface number */
		k = 0; /* interface ID number */
		i_count = 0;
		e_count = 0;
		g_count = 0;
		ds = (usb_descriptor *) raw;
		while(ds && (ds = d_next(ds, &act))){			
			if(ds->generic.descriptor_type != USB_DESCRIPTOR_INTERFACE) {
				if(ds->generic.descriptor_type == USB_DESCRIPTOR_ENDPOINT){
					e_count++;
				} else {
					g_count++;
				}
				continue; /* skip things that aren't ifc descriptors */
			} else {
				i_count++;
			}
			
			if(ci->interface[j].alt_count) {
				if(k == ds->interface.interface_number){
					/* found an additional alt interface */
					ci->interface[j].alt_count++;
				} else {
					j++;
					if(j == ci->interface_count) {
						dprintf(ID "too many interfaces?!\n");
						break;
					} else {
						goto new_ifc;
					}
				}
			} else {
new_ifc:
				/* found the first one */
				k = ds->interface.interface_number;
				ci->interface[j].alt_count = 1;					
			}
		}
					
		dprintf(ID "%d interfaces, %d endpoints, %d generic\n",i_count,e_count,g_count);
		
		act = realsize;
		j = 0; /* interface number */
		ds = (usb_descriptor *) raw;
		li = NULL; /* last interface */
		ii = (usb_interface_info *) malloc(sizeof(usb_interface_info)*i_count +
										   sizeof(usb_endpoint_info)*e_count +
										   sizeof(usb_descriptor*)*g_count);
		
		ei = (usb_endpoint_info *) ( ((uchar*)ii) + (sizeof(usb_interface_info)*i_count) );
		generic = (usb_descriptor **) ( ((uchar*)ei) + (sizeof(usb_endpoint_info)*e_count) );

		ddprintf(ID "mem %p %p %p\n",ii,ei,generic);
				
		while(ds && (ds = d_next(ds, &act))){
			ddprintf("-> %d\n",ds->generic.descriptor_type);
			if(ds->generic.descriptor_type != USB_DESCRIPTOR_INTERFACE) {
				ddprintf("Next <%d>\n",ds->generic.descriptor_type);
				/* add it to the generic pool */
				*generic = ds;
				generic++;
			}
			
		next_iface:			
			if(li && (li->descr->interface_number == ds->interface.interface_number)){
				/* found an alternate interface */
				
			} else {
				li = ii;
				ci->interface[j].alt = ii;
				ci->interface[j].active = ii;
				j++;
			}
			ddprintf("New Ifc @ %p\n",ii);
			ii->descr = &(ds->interface);
			ii->handle = 0;
			ii->endpoint_count = ii->descr->num_endpoints;
			ii->endpoint = ei;
			ii->generic_count = 0;
			ii->generic = generic;
			ei += ii->descr->num_endpoints;
			
			k = 0;
			while((ds = d_next(ds, &act))){
				if(ds->generic.descriptor_type == USB_DESCRIPTOR_INTERFACE) {
					ddprintf("Next Ifc\n");
					if(ii->generic_count == 0) ii->generic = NULL;
					ii++;
					goto next_iface;
				}
				if(ds->generic.descriptor_type == USB_DESCRIPTOR_ENDPOINT){
					ddprintf("Next Ept\n");
					if(k == ii->endpoint_count) continue;
					ii->endpoint[k].descr = &(ds->endpoint);
					ii->endpoint[k].handle = 0;
					k++;
				} else {
					ddprintf("Next <%d>\n",ds->generic.descriptor_type);
					/* add it to the generic pool */
					*generic = ds;
					generic++;
					ii->generic_count++;
				}
			}
			if(ii->generic_count == 0) ii->generic = NULL;
			ii++;
		}
	}
	
	return B_OK;
	
fail:
	/* destroy any configs that were completely setup */
	for(j=0;j<i;j++) free((d->configuration + j)->interface);
	/* destroy the array */
	free(d->configuration);
	return B_ERROR;
}

bm_usb_device *new_device(usb_bus *bus, bm_usb_device *parent, int lowspeed, int portnum)
{
	size_t act;
	status_t err;
		
	bm_usb_device *d;
	d = (bm_usb_device *) malloc(sizeof(usb_hub)); //XXX damn :-)
	SIG_SET_DEVICE(d);
		
	d->nchildren = 0;
	d->active = 0;
	d->active_conf = NULL;
	d->parent = parent;
	d->refcount = 0;
	d->lock = create_sem(0,"usb lock sem");
	if (d->lock < B_OK) {
		dprintf(ID "error creating semaphore\n");
		return NULL;
	}
	d->bus = bus;
	
	if(lowspeed){
		d->control = bus->lowspeed;
	} else {
		d->control = bus->fullspeed;
	}

	ddprintf(ID "about to add dev %p to hash table\n", d);
	err = get_usb_id(d);
	if (err) {
		dprintf(ID "error with get_usb_id: %s\n", strerror(err));
		goto new_device_err0;
	}
	
	/* we only read the first 8 bytes here because maxpacketsize 
	** cannot be less than 8.  However it could be more than that
	** which would be bad because the default descriptors are set
	** to 8.  set_address() will set the packet size correctly
	** and then transfers longer than 8 bytes will work.
	*/
	err = get_descriptor(d->id, USB_DESCRIPTOR_DEVICE, 0, 0,
					   &(d->descriptor), 8, &act);
	if (err) {
		dprintf(ID "failed to get descriptor for new device (1), %s\n", strerror(err));
		goto new_device_err1;
	}
	if (act != 8) {
		dprintf(ID "wrong act_len = %ld, not 8\n", act);
		goto new_device_err1;
	}
	
	
	if(set_address(d->id)){
		dprintf(ID "failed to assign address to new device\n");
		goto new_device_err1;
	} else {
		ddprintf(ID "assigned address to new device\n");
	}
	snooze(2500);	/* >= 2 ms recovery time after Set Address, USB1.1 9.2.6.3 */
			
	if(get_descriptor(d->id, USB_DESCRIPTOR_DEVICE, 0, 0,
					   &(d->descriptor), sizeof(usb_device_descriptor), &act)){
		dprintf(ID "failed to get descriptor for new device (2)\n");	
		goto new_device_err;
	}
	if (act != sizeof(usb_device_descriptor)) {
		dprintf(ID "wrong act_len = %ld, not %ld\n", act, sizeof(usb_device_descriptor));
		goto new_device_err;
	}

	
#if _USBD_DEBUG
	{
		int i;
		uchar *x = (uchar *) &(d->descriptor);
		for(i=0;i<sizeof(usb_device_descriptor);i++) dprintf("%02x ",x[i]);
		dprintf("\n");
	}
#endif

	if(load_configs(d)){
		dprintf(ID "error parsing descriptors for new device\n");
		goto new_device_err;
	}

	if(parent){
		d->name = malloc(strlen(parent->name)+4);
		sprintf(d->name,"%s/%d",parent->name,portnum-1);
	} else {
		char name[32];
		sprintf(name,"/%d",bus->number);
		d->name = strdup(name);
	}
	d->active = 1;
	
	dump_configs(d); 
	
	dprintf(ID "new device (%s)\n",d->name);
	
	release_sem(d->lock);
	do_notify(d, USB_DEVICE_ADDED);
	return d;
	
new_device_err:
	tear_down_endpoints(d, true); /* we did set an address */
	
new_device_err1:
	put_usb_id(d);
	
new_device_err0:
	delete_sem(d->lock);
	SIG_CLR_DEVICE(d);
	free(d);
	return NULL;
}
