#include <KernelExport.h>
#include <Drivers.h>
#include "generic_wrapper.h"


static char *eepro100_name[] = { "net/eepro100/0", NULL };

static device_hooks eepro100_device =  {
	generic_open, 		/* -> open entry point */
	generic_close,		/* -> close entry point */
	generic_free,			/* -> free entry point */
	generic_control, 		/* -> control entry point */
	generic_read,			/* -> read entry point */
	generic_write			/* -> write entry point */
};


const char **
publish_devices()
{  
	return eepro100_name;
}

device_hooks *
find_device(const char *name)
{
	return &eepro100_device;
}
