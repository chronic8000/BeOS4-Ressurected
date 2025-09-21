#include <KernelExport.h>
#include <Drivers.h>
#include "generic_wrapper.h"


static char *ec9xx_name[] = { "net/ec9xx/0", NULL };

static device_hooks ec9xx_device =  {
	generic_open, 		/* -> open entry point */
	generic_close,		/* -> close entry point */
	generic_free,			/* -> free entry point */
	generic_control, 		/* -> control entry point */
	generic_read,			/* -> read entry point */
	generic_write,			/* -> write entry point */
	NULL,					/* -> select entry point */
	NULL					/* -> deselect entry point */
};


const char **
publish_devices()
{
	return ec9xx_name;
}

device_hooks *
find_device(const char *name)
{
	return &ec9xx_device;
}

int32		api_version = B_CUR_DRIVER_API_VERSION;
