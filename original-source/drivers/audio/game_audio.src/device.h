/************************************************************************/
/*                                                                      */
/*                              device.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _DEVICE_H
#define _DEVICE_H
#include <Drivers.h>

#define MAX_DEV 100


status_t RegisterDevice(const char* path, const char* name, device_hooks* hooks, void* cookie);
device_hooks* FindDeviceHooks(const char* name);
void* FindDeviceCookie(const char* name);
const char* GetDeviceName(int idx);
int GetDeviceCount();

#endif