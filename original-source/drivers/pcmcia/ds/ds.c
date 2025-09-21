/*======================================================================

    PC Card Driver Services
    
    ds.c 1.2 1998/11/06 02:58:03
    
    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

static cs_client_module_info *cs;
static ds_module_info *ds;
#define CardServices		cs->_CardServices

static status_t ds_open(const char *name, uint32 flags, void **cookie)
{
	client_handle_t *handle;
	int i;
	
    /* Use the last digit in the name as the socket number */
    i = *(name+strlen(name)-1) - '0';
	
	handle = (client_handle_t*) malloc(sizeof(client_handle_t));
	
	if(ds->get_handle(i,handle)){
		free(handle);
		return ENODEV;
	} else {
		*cookie = handle;
		return B_OK;
	}
}

static status_t 
ds_close(void *cookie)
{
    return B_OK;
}

static status_t 
ds_free(void *cookie)
{
	free(cookie);
    return B_OK;
}

static status_t 
ds_read(void *cookie, off_t pos, void *data, size_t *len)
{
    return EIO;
}

static status_t 
ds_write(void *cookie, off_t pos, const void *data, size_t *len)
{
	return EIO;
}

/*====================================================================*/

static status_t ds_ioctl(void *cookie, uint32 cmd, void *arg, size_t len)
{
    client_handle_t handle = *((client_handle_t*)cookie);
    int ret, err, size;
    ds_ioctl_arg_t buf;
    
    err = ret = 0;
    size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
    if (size > sizeof(ds_ioctl_arg_t)) return EINVAL;
    if (cmd & IOC_IN) copy_from_user((char *)&buf, (char *)arg, size);

//    dprintf("ds_ioctl(%x,%d,...)\n",cookie,cmd);
    
    switch (cmd) {
    case DS_ADJUST_RESOURCE_INFO:
	ret = CardServices(AdjustResourceInfo, handle, &buf.adjust);
	break;
    case DS_GET_CARD_SERVICES_INFO:
	ret = CardServices(GetCardServicesInfo, &buf.servinfo);
	break;
    case DS_GET_CONFIGURATION_INFO:
	ret = CardServices(GetConfigurationInfo, handle, &buf.config);
	break;
    case DS_GET_FIRST_TUPLE:
	ret = CardServices(GetFirstTuple, handle, &buf.tuple);
	break;
    case DS_GET_NEXT_TUPLE:
	ret = CardServices(GetNextTuple, handle, &buf.tuple);
	break;
    case DS_GET_TUPLE_DATA:
	buf.tuple.TupleData = buf.tuple_parse.data;
	buf.tuple.TupleDataMax = sizeof(buf.tuple_parse.data);
	ret = CardServices(GetTupleData, handle, &buf.tuple);
	break;
    case DS_PARSE_TUPLE:
	buf.tuple.TupleData = buf.tuple_parse.data;
	ret = CardServices(ParseTuple, handle, &buf.tuple,
			   &buf.tuple_parse.parse);
	break;
    case DS_RESET_CARD:
	ret = CardServices(ResetCard, handle, NULL);
	break;
    case DS_GET_STATUS:
	ret = CardServices(GetStatus, handle, &buf.status);
	break;
    case DS_VALIDATE_CIS:
	ret = CardServices(ValidateCIS, handle, &buf.cisinfo);
	break;
    case DS_SUSPEND_CARD:
	ret = CardServices(SuspendCard, handle, NULL);
	break;
    case DS_RESUME_CARD:
	ret = CardServices(ResumeCard, handle, NULL);
	break;
    case DS_EJECT_CARD:
	ret = CardServices(EjectCard, handle, NULL);
	break;
    case DS_INSERT_CARD:
	ret = CardServices(InsertCard, handle, NULL);
	break;
    case DS_ACCESS_CONFIGURATION_REGISTER:
	ret = CardServices(AccessConfigurationRegister, handle,
			   &buf.conf_reg);
	break;
    case DS_GET_FIRST_REGION:
        ret = CardServices(GetFirstRegion, handle, &buf.region);
	break;
    case DS_GET_NEXT_REGION:
	ret = CardServices(GetNextRegion, handle, &buf.region);
	break;
    case DS_REPLACE_CIS:
	ret = CardServices(ReplaceCIS, handle, &buf.cisdump);
	break;
#if 0
    case DS_BIND_REQUEST:
	err = bind_request(i, &buf.bind_info);
	break;
    case DS_GET_DEVICE_INFO:
	err = get_device_info(i, &buf.bind_info);
	break;
    case DS_GET_NEXT_DEVICE:
	err = get_next_device(i, &buf.bind_info);
	break;
    case DS_UNBIND_REQUEST:
	err = unbind_request(i, &buf.bind_info);
	break;
    case DS_BIND_MTD:
	err = bind_mtd(i, &buf.mtd_info);
	break;
#endif
    default:
	err = EINVAL;
    }
    
    if ((err == 0) && (ret != CS_SUCCESS)) {
//	dprintf("ds_ioctl: ret = %d\n", ret);
	switch (ret) {
	case CS_BAD_SOCKET: case CS_NO_CARD:
	    err = ENODEV; break;
	case CS_BAD_ARGS: case CS_BAD_ATTRIBUTE: case CS_BAD_IRQ:
	case CS_BAD_TUPLE:
	    err = EINVAL; break;
	case CS_IN_USE:
	    err = EBUSY; break;
	case CS_OUT_OF_RESOURCE:
	    err = ENOSPC; break;
	case CS_NO_MORE_ITEMS:
	    err = ENOSPC; break;
	case CS_UNSUPPORTED_FUNCTION:
	    err = ENOSYS; break;
	default:
	    err = EIO; break;
	}
    }
    
    if (cmd & IOC_OUT) copy_to_user((char *)arg, (char *)&buf, size);
     
    return err;
} /* ds_ioctl */

/*====================================================================*/

static device_hooks ds_device_hooks = {
    &ds_open,
    &ds_close,
    &ds_free,
    &ds_ioctl,
    &ds_read,
    &ds_write,
};

/*====================================================================*/

static char **names = NULL;

status_t init_hardware(void)
{
    return B_OK;
}

static int sockets;

static char *basename = "bus/pcmcia/sock/%d";

status_t init_driver(void)
{
	client_handle_t handle;
	int i;
	
	if(get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs)){
		return B_ERROR;
	}
	if(get_module(DS_MODULE_NAME, (struct module_info **)&ds)){
		put_module(CS_CLIENT_MODULE_NAME);
		return B_ERROR;
	}

	sockets = 0;
	while(ds->get_handle(sockets,&handle) == B_OK) sockets++;

	names = (char **) malloc(sizeof(char*) * (sockets+1));
	for(i=0;i<sockets;i++){
		names[i] = (char *) malloc(strlen(basename) + 8);
		sprintf(names[i],basename,i);
	}
	names[i] = 0;	
    
    return B_OK;
}

void uninit_driver(void)
{
	int i;
	if (names != NULL) {
		for (i = 0; i < sockets; i++)
			free(names[i]);
		free(names);
	}
	put_module(DS_MODULE_NAME);
	put_module(CS_CLIENT_MODULE_NAME);
}

const char **publish_devices(void)
{
	return (const char**) names;
}

device_hooks *find_device(const char *name)
{
    return &ds_device_hooks;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;

