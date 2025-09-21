#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <ByteOrder.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <ISA.h>

#include "DeviceStatusProtocol.h"

#define TOUCH(x) ((void)x)

#define CONFIG_INDEX                 0x2e
#define CONFIG_DATA                  0x2F

#define LOGICAL_DEVICE_NUMBER_INDEX  0x07
#define BASE_ADDR_MSB_INDEX          0x60
#define BASE_ADDR_LSB_INDEX          0x61

#define GPIO_DEVICE_NUMBER           0x07

#define GPIO_DATA_OUT_2_ADDR		 (gpio_base+4)
#define MAIL_MASK                    0x01
#define POWER_MASK                   0x02
#define NETWORK_MASK                 0x04

#define MAX_SIZE 16

//Uncomment to Debug this device without actually writing to any physical devices
//#define DESKTOP_DEBUG

#define DEVICE_SHORTNAME	 			"ibm730"
#define DEVICE_PUBLISHEDNAMECOUNT		3	

isa_module_info   *isa;
uint16             gpio_base;

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	"misc/status/ibm730",
	NULL
};

static const char*	properties[] = {
	"email",
	"connection",
	"power",
	NULL
};

static int prop_lengths[3] = {
	5,
	10,
	5
};

static int values[] = {
	0, /*email*/
	0, /*network*/
	1 /*power*/};

status_t init_driver();
void uninit_driver();

status_t ibm730_leds_open(const char *name, uint32 mode, void **cookie);
status_t ibm730_leds_close(void *cookie);
status_t ibm730_leds_free(void *cookie);
status_t ibm730_leds_ioctl(void *cookie, uint32 op, void *data, size_t len);
status_t ibm730_leds_read(void *cookie, off_t pos, void *data, size_t *numBytes);
status_t ibm730_leds_write(void *cookie, off_t pos, const void *data, size_t *numBytes);

static device_hooks hooks = {
		&ibm730_leds_open,
		&ibm730_leds_close,
		&ibm730_leds_free,
		&ibm730_leds_ioctl,
		&ibm730_leds_read,
		&ibm730_leds_write,
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
	status_t err;
	cpu_status ps;
	
	dprintf("ibm730_leds: init_driver\n");
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR)
		goto err1;

	ps = disable_interrupts(); /* not safe on mp systems */
	isa->write_io_8(CONFIG_INDEX, LOGICAL_DEVICE_NUMBER_INDEX);
	isa->write_io_8(CONFIG_DATA, GPIO_DEVICE_NUMBER);
	isa->write_io_8(CONFIG_INDEX, BASE_ADDR_MSB_INDEX);
	gpio_base = isa->read_io_8(CONFIG_DATA);
	isa->write_io_8(CONFIG_INDEX, BASE_ADDR_LSB_INDEX);
	gpio_base = gpio_base << 8 | isa->read_io_8(CONFIG_DATA);
	restore_interrupts(ps);

	
	//Initialize LEDS to default states
	set_led(0, values[0]);
	set_led(1, values[1]);
	set_led(2, values[2]);

	return B_OK;
err1:
	dprintf("ibm730_leds: init_hardware failed\n");
	return err;
}


//Private Function Prototypes
static void get_name_info(status_get_name_info * nis);
static void get_name_value(status_name_value * nis);
static status_t set_name_value(status_name_value * nis);
static status_t set_led(int index, int value);
static status_t process_write_line(char *start, char*stop);

status_t init_driver()
{
	status_t err;
	cpu_status ps;
	
	dprintf("ibm730_leds: init_driver\n");
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR)
		goto err1;

	ps = disable_interrupts(); /* not safe on mp systems */
	isa->write_io_8(CONFIG_INDEX, LOGICAL_DEVICE_NUMBER_INDEX);
	isa->write_io_8(CONFIG_DATA, GPIO_DEVICE_NUMBER);
	isa->write_io_8(CONFIG_INDEX, BASE_ADDR_MSB_INDEX);
	gpio_base = isa->read_io_8(CONFIG_DATA);
	isa->write_io_8(CONFIG_INDEX, BASE_ADDR_LSB_INDEX);
	gpio_base = gpio_base << 8 | isa->read_io_8(CONFIG_DATA);
	restore_interrupts(ps);

	return B_NO_ERROR;

err1:
	dprintf("ibm730_leds: init_driver failed\n");
	return err;
}

void uninit_driver()
{
	dprintf("ibm730_leds: uninit_driver\n");
}

status_t ibm730_leds_open(const char *name, uint32 mode, void **cookie)
{
	dprintf("ibm730_leds: ibm730_leds_open\n");
	
	return B_OK;
}

status_t ibm730_leds_close(void *cookie)
{
	dprintf("ibm730_leds: ibm730_leds_close\n");

	return B_OK;
}

status_t ibm730_leds_free(void *cookie)
{
	dprintf("ibm730_leds: ibm730_leds_free\n");
	
	return B_OK;
}

status_t ibm730_leds_ioctl(void *cookie, uint32 op, void *data, size_t len)
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
	dprintf("CLIPPER_LED: ibm730_leds_ioctl -> Bad Structure Size...");
	return ENOSYS;
badnameindex:
	dprintf("CLIPPER_LED: ibm730_leds_ioctl -> Bad Name Index...");
	return ENOSYS;
}


status_t ibm730_leds_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	char rtnstr[1024];
	size_t rtnlen;
	char tmp[50];
	
	memset(rtnstr, 0, 1024);
	memset(tmp, 0, 50);
	
	dprintf("ibm730_leds: ibm730_leds_read\n");
			
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

status_t ibm730_leds_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
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
	dprintf("ibm730_leds: write failed\n");
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
	
	cpu_status   ps;
	uint8        mask;
	
	if((value < 0) || (value > 1)) rtn = EINVAL;
		
	switch(index)
	{
		case 0: //email property
			 
			mask = MAIL_MASK;
			break;
		case 1: //network property
			mask = NETWORK_MASK;
			break;
		case 2: //power property
			mask = POWER_MASK;
			break;
	}
	
	ps = disable_interrupts(); /* not safe on mp systems */
	if(value == 0)
		isa->write_io_8(GPIO_DATA_OUT_2_ADDR,
		                isa->read_io_8(GPIO_DATA_OUT_2_ADDR) | mask);
	else
		isa->write_io_8(GPIO_DATA_OUT_2_ADDR,
		                isa->read_io_8(GPIO_DATA_OUT_2_ADDR) & ~mask);
	restore_interrupts(ps);
	
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
		if(curpos <  prop_lengths[0])
		{	
			if(*curchar != (properties[0])[curpos]) led_email = false; 
		}
		else led_email = false; 
		
		if(curpos < prop_lengths[1])
		{	
			if(*curchar != (properties[1])[curpos]) led_network = false; 
		}
		else led_network = false; 
		
		if(curpos < prop_lengths[2])
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
	
	if(led_email && (curpos == prop_lengths[0])) return set_led(0, value);
			
	if(led_network && (curpos == prop_lengths[1])) return set_led(1, value);
	
	if(led_power && (curpos == prop_lengths[2])) return set_led(2, value);
		
	return ENOENT;
}


