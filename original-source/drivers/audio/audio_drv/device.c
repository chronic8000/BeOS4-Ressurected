/************************************************************************/
/*                                                                      */
/*                              device.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <KernelExport.h>
#include <OS.h>
#include <Drivers.h>
#include <string.h>

#include "device.h"
#include "debug.h"

struct {
	char name[B_OS_NAME_LENGTH];
	device_hooks* hooks;
	void* cookie;
} device[MAX_DEV];

static int dev_count = 0;


status_t RegisterDevice(const char* path, const char* name, device_hooks* hooks, void* cookie)
{
	DB_PRINTF(("RegisterDevice() path = %s, name = %s\n", path, name));
	if(!(dev_count < MAX_DEV)) 
	{
		DB_PRINTF(("RegisterDevice() too many devices. return B_ERROR;\n"));
		return B_ERROR;
	}
	memset(device[dev_count].name, 0, sizeof(device[dev_count].name));
	strncpy(device[dev_count].name, path, sizeof(device[dev_count].name)-1);
	strncat(device[dev_count].name, name, sizeof(device[dev_count].name)-1 - strlen(device[dev_count].name));
	device[dev_count].hooks = hooks;
	device[dev_count].cookie = cookie;

	DB_PRINTF(("Registered %s\n", device[dev_count].name));

	dev_count++;
	return B_OK;
}

device_hooks* FindDeviceHooks(const char* name)
{
	int i;
	DB_PRINTF(("FindDeviceHooks(): %s\n", name));
	for(i = 0; i < dev_count; i++) if(strcmp(name,device[i].name) == 0) return device[i].hooks;
	return NULL;
}

void* FindDeviceCookie(const char* name)
{
	int i;
	DB_PRINTF(("FindDeviceCookie(): %s\n", name));
	for(i = 0; i < dev_count; i++) if(strcmp(name,device[i].name) == 0) return device[i].cookie;
	return NULL;
}

const char* GetDeviceName(int idx)
{
	DB_PRINTF(("GetDeviceName(%d): %s\n",idx, device[idx].name));
	return device[idx].name;
}

int GetDeviceCount()
{
	return dev_count;
}
