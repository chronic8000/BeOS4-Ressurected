/* Skeleton BeOS Device Driver
**
** This driver doesn't actually DO anything, but it contains
** all the hooks needed to be a "real" driver.  It's a
** skeleton that you can start putting code into.
**
*/


#include <stdlib.h>
#include <string.h>

#include <Drivers.h>
#include <KernelExport.h>

static uint8 *gBuffer;
size_t bufSize;
sem_id bufLock;

/* This hook is called when a program opens one of the entries
** exported by the driver. The name of the entry is passed in
** name, along with the flags passed to the open() call. cookie
** is a pointer to a region of memory large enough to hold a
** single pointer. This can be used to store state information
** associated with the open instance; typically the driver
** allocates a chunk of memory as large as it needs and stores
** a pointer to that memory in this area.
*/

static status_t 
driver_open(const char *name, uint32 flags, void **cookiep)
{
	return B_OK;
}

/* This hook is called when an open instance of the driver is
** closed. Note that there may still be outstanding transactions
** on this instance in other threads, so this function should
** not be used to reclaim instance-wide resources. Blocking
** drivers must unblock ongoing transactions when the close hook
** is called.
*/
static status_t 
driver_close(void *cookie)
{
	return B_OK;
}

/* This hook is called after an open instance of the driver has
** been closed and all the outstanding transactions have completed.
** This is the proper place to perform uninitialization activities.
*/
static status_t
driver_free(void *_cookie)
{
	return B_OK;
}

/* This hook handles ioctl() requests. The control hook provides
** a means of instructing the driver to perform actions that don't
** map directly to read() or write(). It is passed the cookie for
** the open instance as well as a command code op and parameters
** data and len supplied by the caller. data and len have no
** intrinsic relation; they are simply two arguments to ioctl().
** The interpretation of the command codes and the parameter
** information is defined by the driver.
*/
static status_t 
driver_control(void *cookie, uint32 op, void *data, size_t len)
{
	return EINVAL;
}

/* This hook handles read requests on an open instance of the
** device. The function reads *numBytes bytes from offset position
** to the memory buffer data. Precisely what this means is device
** specific. The driver should set *numBytes to the number of bytes
** processed and return either B_OK or an error code, as appropriate.
*/
static status_t 
driver_read(void *_cookie, off_t position, void *data, size_t *numBytes)
{
	status_t	err;
	err = acquire_sem(bufLock);
	if(err != B_NO_ERROR)
		return err;

	if (position > bufSize)
		*numBytes = 0;
	else
		if (position + *numBytes > bufSize)
			*numBytes = bufSize - position;

	memcpy(data, gBuffer, *numBytes);
	release_sem(bufLock);
	return B_OK;
}

/* This hook handles write requests on an open instance of the 
** device.  The arguments have the same meaning as in the read hook,
** except data is written instead of read.
*/
static status_t 
driver_write(void *_cookie, off_t position, const void *data, size_t *numBytes)
{
	status_t	err;
	char		*tmp;

	err = acquire_sem(bufLock);
	if(err != B_NO_ERROR)
		return err;
	
	if (position + *numBytes > bufSize) {
		tmp = (char *) realloc(gBuffer, position + *numBytes);
		if (!tmp) {
			*numBytes = 0;
			err = ENOMEM;
			goto error1;
		}
		gBuffer = tmp;
		//memset(gBuffer + cookie->size, 0, position + *numBytes - bufSize);
		bufSize = position + *numBytes;
	}
	memcpy(gBuffer + position, data, *numBytes);
	
	err = B_NO_ERROR;
	release_sem(bufLock);
error1:
	return err;
}


static device_hooks driver_hooks = {
	driver_open,
	driver_close,
	driver_free,
	driver_control,
	driver_read,
	driver_write,
	NULL,
	NULL,
	NULL,
	NULL	
};

/* This function is called when the system is booted, allowing
** the driver to detect and reset the hardware. The function
** should return B_OK if the initialization is successful or an
** error code if it is not. If the function returns an error,
** the driver will not be used.
*/
_EXPORT status_t
init_hardware(void)
{
	return B_OK;
}

/* Devfs calls this hook to discover the names, relative to
** /dev, of the driver's supported devices. The driver should
** return a NULL-terminated array of strings enumerating the
** list of installed devices supported by the driver.
*/
_EXPORT const char **
publish_devices()
{
	static const char *names[] = { "misc/opera-state", NULL };
	return names;
}

/* When an exported /dev entry is accessed, devfs calls a set
** of driver hook functions to fulfill the request. find_device()
** reports the hooks for the entry name in a device_hooks structure.
*/
_EXPORT device_hooks *
find_device(const char *name)
{
	return &driver_hooks;
}

/* Devfs loads and unloads drivers on an as-needed basis. This
** function is called when the driver is loaded into memory,
** allowing it to allocate any system resources it needs to
** function properly.
*/
_EXPORT status_t
init_driver(void)
{
	gBuffer = NULL;
	bufSize = 0;
	bufLock = create_sem(1, "opera-state bufferLock");
	if(bufLock < 0)
		return bufLock;
	return B_OK;
}

/* Conversely, this function is called when the driver is
** unloaded from memory, allowing it to clean up after itself.
**/
_EXPORT void
uninit_driver(void)
{
	delete_sem(bufLock);
}

/* This variable defines the API version to which the driver was
** written; it should always be set to B_CUR_DRIVER_API_VERSION
** (whose value, naturally, changes with the driver API).
*/
_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;

