#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <ByteOrder.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <PCI.h>

#include "DeviceStatusProtocol.h"
#include "PCI_Functions.h"

#define TOUCH(x) ((void)x)

#define POWER_MASK                   0x6900
#define MAIL_MASK                    0x6908
#define NETWORK_MASK                 0x690e

#define MAX_SIZE 16

//Uncomment to Debug this device without actually writing to any physical devices
//#define DESKTOP_DEBUG

#define DEVICE_SHORTNAME	 			"clipper"
#define DEVICE_PUBLISHEDNAMECOUNT		3	

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	"misc/status/clipper/1",
	NULL
};

static const char*	properties[] = {
	"email",
	"connection",
	"power",
	NULL
};

static int values[] = {
	0, /*email*/
	0, /*network*/
	1 /*power*/};

status_t init_driver();
void uninit_driver();

status_t clipper_leds_open(const char *name, uint32 mode, void **cookie);
status_t clipper_leds_close(void *cookie);
status_t clipper_leds_free(void *cookie);
status_t clipper_leds_ioctl(void *cookie, uint32 op, void *data, size_t len);
status_t clipper_leds_read(void *cookie, off_t pos, void *data, size_t *numBytes);
status_t clipper_leds_write(void *cookie, off_t pos, const void *data, size_t *numBytes);

static device_hooks hooks = {
		&clipper_leds_open,
		&clipper_leds_close,
		&clipper_leds_free,
		&clipper_leds_ioctl,
		&clipper_leds_read,
		&clipper_leds_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
};

device_hooks *find_device(const char *name)
{
	if (!strcmp(name, device_name_list[0]))
		return &hooks;
	if (!strcmp(name, device_name_list[1]))
		return &hooks;
	if (!strcmp(name, device_name_list[2]))
		return &hooks;

	return NULL;
}

const char **publish_devices(){	return device_name_list; }

status_t init_hardware()
{
	uint8 gpioCtrlReg1, gpioCtrlReg3;	
	
	dprintf("clipper_leds: init_hardware\n");
   
#ifndef DESKTOP_DEBUG   
	GetPMIOBase();
	
	//activate GPO8 for "EMAIL LED"
	gpioCtrlReg1 = ReadPCIByte( 0, 7, 0, 0x74 );
	WritePCIByte( 0, 7, 0, 0x74, (gpioCtrlReg1 | 0x04) );
	gpioCtrlReg3 = ReadPCIByte( 0, 7, 0, 0x76 );
	WritePCIByte( 0, 7, 0, 0x76, (gpioCtrlReg3 & 0xFE) );
#endif
	
	//Initialize LEDS to default states
	set_led(0, values[0]);
	set_led(1, values[1]);
	set_led(2, values[2]);

	return B_OK;
}


//Private Function Prototypes
static void get_name_info(status_get_name_info * nis);
static void get_name_value(status_name_value * nis);
static status_t set_name_value(status_name_value * nis);
static status_t set_led(int index, int value);
static status_t process_write_line(char *start, char*stop);

status_t init_driver()
{
	dprintf("clipper_leds: init_driver\n");
	return B_OK;
}

void uninit_driver()
{
	dprintf("clipper_leds: uninit_driver\n");
}

status_t clipper_leds_open(const char *name, uint32 mode, void **cookie)
{
	dprintf("clipper_leds: clipper_leds_open\n");
	
	return B_OK;
}

status_t clipper_leds_close(void *cookie)
{
	dprintf("clipper_leds: clipper_leds_close\n");

	return B_OK;
}

status_t clipper_leds_free(void *cookie)
{
	dprintf("clipper_leds: clipper_leds_free\n");
	
	return B_OK;
}

status_t clipper_leds_ioctl(void *cookie, uint32 op, void *data, size_t len)
{
	TOUCH(len);

	switch(op)
	{
		case STATUS_GET_INFO:
			{
				status_get_info* ssp;
				if(*((size_t *)data) != sizeof(status_get_info)) goto badstructsize;
				ssp = (status_get_info *) data; 
				memset((void *)ssp->dev_name, 0, 16);
				strncpy(ssp->dev_name, DEVICE_SHORTNAME,15);
				ssp->dev_version = 0;  //TODO: Insert correct version here.
				ssp->name_count = DEVICE_PUBLISHEDNAMECOUNT;	
				return B_NO_ERROR; 
			}
			break;
		case STATUS_GET_NAME_INFO:
			{
				status_get_name_info * ssp;
				if(*((size_t *)data) != sizeof(status_get_name_info)) goto badstructsize;
				ssp = (status_get_name_info *) data; 
				if((ssp->name_index < 0) || (ssp->name_index >= DEVICE_PUBLISHEDNAMECOUNT))
					goto badnameindex;
				get_name_info(ssp);	
				return B_NO_ERROR; 
			}
			break;
		case STATUS_GET_NAME_VALUE:
			{
				status_name_value * ssp;
				if(*((size_t *)data) != sizeof(status_name_value)) goto badstructsize;
				ssp = (status_name_value *) data; 
				if((ssp->name_index < 0) || (ssp->name_index >= DEVICE_PUBLISHEDNAMECOUNT))
					goto badnameindex;
				get_name_value(ssp);
				return B_NO_ERROR;
			}
			break;
		case STATUS_SET_NAME_VALUE:
			{
				status_name_value * ssp;
				if(*((size_t *)data) != sizeof(status_name_value)) goto badstructsize;
				ssp = (status_name_value *) data; 
				if((ssp->name_index < 0) || (ssp->name_index >= DEVICE_PUBLISHEDNAMECOUNT))
					goto badnameindex;
				return set_name_value(ssp);		
			}
			break;
	}
		
	return ENOSYS;
badstructsize:
	dprintf("CLIPPER_LED: clipper_leds_ioctl -> Bad Structure Size...");
	return ENOSYS;
badnameindex:
	dprintf("CLIPPER_LED: clipper_leds_ioctl -> Bad Name Index...");
	return ENOSYS;
}


status_t clipper_leds_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	char rtnstr[1024];
	size_t rtnlen;
	char tmp[50];
	
	memset(rtnstr, 0, 1024);
	memset(tmp, 0, 50);
	
	dprintf("clipper_leds: clipper_leds_read\n");
			
	sprintf(tmp,"%s=%d\n", properties[0], values[0]);
	strcat(rtnstr, tmp);
	sprintf(tmp,"%s=%d\n", properties[1], values[1]);
	strcat(rtnstr, tmp);
	sprintf(tmp,"%s=%d\n", properties[2], values[2]);
	strcat(rtnstr, tmp);
	
	rtnlen = strlen(rtnstr);
	if(*numBytes > (rtnlen + 1))
	{
		if(pos <= *numBytes)
		{
			*numBytes = rtnlen;
			memset(data, 0, *numBytes);
			strncpy((char *)data, &rtnstr[pos], rtnlen);
			*numBytes = rtnlen + 1;
			return B_NO_ERROR;
		}
		else return 0;
	}
	
	return B_ERROR;
}

status_t clipper_leds_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	char *str;
	char *start;
	char *stop;
	unsigned long curpos = 0;
	unsigned long lastpos = 0;
	size_t strlen = *numBytes + 1;
	status_t status;
	
	str = malloc(strlen);
	if(str == NULL) return B_ERROR;
	
	memset(str, 0, strlen);
	memcpy(str, data, *numBytes);
	
	start = str;
	
	//Parse Line
	do
	{
		stop = strchr(start, (int)'\n');
		
		//Process Line
		status = process_write_line(start, stop);	
	
		if(status != B_NO_ERROR) return status;
		if(stop != NULL) start = &stop[1];
	}while((stop != NULL) && (((unsigned long)start - (unsigned long)data) < *numBytes) &&
		   (status == B_NO_ERROR));
	
	free(str);
	
	return status;
err:
	dprintf("clipper_leds: write failed\n");
	*numBytes = 0;
	return B_ERROR;
}


void get_name_info(status_get_name_info * nis)
{
	switch(nis->name_index)
	{
		case 0: //email property
			nis->flags = (statusIsWritable | statusIsReadable);
			nis->value_type = typeIsNumber;
			nis->min_value = 0;
			nis->max_value = 1;
			memset(nis->name, 0, 16);
			strncpy(nis->name, properties[0], 15);	
			break;
		case 1: //network property
			nis->flags = (statusIsWritable | statusIsReadable);
			nis->value_type = typeIsNumber;
			nis->min_value = 0;
			nis->max_value = 1;
			memset(nis->name, 0, 16);
			strncpy(nis->name, properties[1], 15);	
			break;
		case 2: //power property
			nis->flags = (statusIsWritable | statusIsReadable);
			nis->value_type = typeIsNumber;
			nis->min_value = 0;
			nis->max_value = 1;
			memset(nis->name, 0, 16);
			strncpy(nis->name, properties[2], 15);	
			break;
	}
}

void get_name_value(status_name_value * nis)
{
	switch(nis->name_index)
	{
		case 0: //email property
			nis->value_type = typeIsNumber;	//    out: actual type
			nis->io_value = values[0];		//numbers: the number
			break;
		case 1: //network property
			nis->value_type = typeIsNumber;	//    out: actual type
			nis->io_value = values[1];		//numbers: the number
			break;
		case 2: //power property
			nis->value_type = typeIsNumber;	//    out: actual type
			nis->io_value = values[2];		//numbers: the number
			break;
	}
}

status_t set_name_value(status_name_value* nis)
{
	switch(nis->name_index)
	{
		case 0: //email property
			nis->value_type = typeIsNumber;	//    out: actual type
			values[0] = nis->io_value;		//numbers: the number
			return set_led(0, values[0]);
		case 1: //network property
			nis->value_type = typeIsNumber;	//    out: actual type
			values[1] = nis->io_value;		//numbers: the number
			return set_led(1, values[1]);
		case 2: //power property
			nis->value_type = typeIsNumber;	//    out: actual type
			values[2] = nis->io_value;		//numbers: the number			
			return set_led(2, values[2]);
	}
	return B_ERROR;
}

status_t set_led(int index, int value)
{
	status_t rtn = B_NO_ERROR;
	
	switch(index)
	{
		case 0: //email property
			if((value < 0) || (value > 1)) rtn = EINVAL; 
#ifndef DESKTOP_DEBUG
			if(rtn == B_NO_ERROR) SetGPO( (uint16)(MAIL_MASK & 0x00FF ), (value == 1)?0:1 );
#endif
			break;
		case 1: //network property
			if((value < 0) || (value > 1)) rtn = EINVAL;
#ifndef DESKTOP_DEBUG
			if(rtn == B_NO_ERROR) SetGPO( (uint16)(NETWORK_MASK & 0x00FF ), (value == 1)?0:1 );
#endif
			break;
		case 2: //power property
			if((value < 0) || (value > 1)) rtn = EINVAL;
#ifndef DESKTOP_DEBUG
			if(rtn == B_NO_ERROR) SetGPO( (uint16)(POWER_MASK & 0x00FF ), value );		
#endif
			break;
	}
	
	return rtn;
}

status_t process_write_line(char *start, char*stop)
{
	bool led_email = true;
	bool led_network = true;
	bool led_power = true;
	int value = 0;
	int curpos = 0;
	
	char *curchar = start;
			
	while((*curchar != '=') && (curchar <= stop))
	{
		//check for email
		if(curpos < 5)
		{	
			if(*curchar != (properties[0])[curpos]) led_email = false; 
		}
		else led_email = false; 
		
		if(curpos < 7)
		{	
			if(*curchar != (properties[1])[curpos]) led_network = false; 
		}
		else led_network = false; 
		
		if(curpos < 5)
		{	
			if(*curchar != (properties[2])[curpos]) led_power = false; 
		}
		else led_power = false; 
		
		curchar++;
		curpos++;
	}
	
	if((!led_email) && (!led_network) && (!led_power)) return ENOENT;  
	
	curchar++;
	value = atoi(curchar);
	
	if(led_email && (curpos == 5)) return set_led(0, value);
			
	if(led_network && (curpos == 7)) return set_led(1, value);
	
	if(led_power && (curpos == 5)) return set_led(2, value);
		
	return ENOENT;
}


