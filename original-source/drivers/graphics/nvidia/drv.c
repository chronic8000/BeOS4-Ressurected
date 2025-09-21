/* :ts=8 bk=0
 *
 * drv.c:	Driver-wide hooks.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <dinky/bena4.h>
#include <malloc.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/debug.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * Globals.
 */
extern device_hooks	graphics_device_hooks;

_EXPORT int32		api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info		*gpci_bus;
genpool_module		*ggpm;
driverdata		*gdd;
BPoolInfo		*gfbpi;


/****************************************************************************
 * Detect hardware.
 */
_EXPORT status_t
init_hardware (void)
{
	pci_info	pcii;
	int32		idx;
	status_t	retval = B_ERROR;

	if ((retval = get_module (B_PCI_MODULE_NAME,
				  (module_info **) &gpci_bus)) != B_OK)
		/*  Well, really, what's the point?  */
		return (retval);

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0;
	     (*gpci_bus->get_nth_pci_info)(idx, &pcii) == B_NO_ERROR;
	     idx++)
	{
		if (findsupportedcard (&pcii)) {
			/*  My, that looks interesting...  */
			retval = B_OK;
			break;
		}
	}

	put_module (B_PCI_MODULE_NAME);  gpci_bus = NULL;
	return (retval);
}



/****************************************************************************
 * Surf the PCI bus for devices we can use.
 */
_EXPORT status_t
init_driver (void)
{
	status_t retval;

	/*
	 * Get a handle for the PCI bus
	 */
	if ((retval = get_module (B_PCI_MODULE_NAME,
				  (module_info **) &gpci_bus)) != B_OK)
		return (retval);

	/*
	 * Open the genpool module.
	 */
	if ((retval = get_module (B_GENPOOL_MODULE_NAME,
				  (module_info **) &ggpm)) != B_OK)
		goto err0;	/*  Look down  */

	/*  Create driver's private data area.  */
	if (!(gdd = (driverdata *) calloc (1, sizeof (driverdata)))) {
		retval = B_NO_MEMORY;
		goto err1;	/*  Look down  */
	}

	if ((retval = BInitOwnedBena4 (&gdd->dd_DriverLock,
				       "NVidia kernel driver",
				       B_SYSTEM_TEAM)) < 0)
	{
		free (gdd);
err1:		put_module (B_GENPOOL_MODULE_NAME);
err0:		put_module (B_PCI_MODULE_NAME);
		return (retval);
	}
	/*  Find all of our supported devices  */
	probe_devices ();

#if (DEBUG)
	init_debug ();
#endif

	return (B_OK);
}

_EXPORT void
uninit_driver (void)
{
dprintf (("+++ uninit_driver\n"));

	if (gdd) {
#if (DEBUG)
		uninit_debug ();
#endif
		BDisposeBena4 (&gdd->dd_DriverLock);
		free (gdd);
		gdd = NULL;
	}
	if (ggpm)	put_module (B_GENPOOL_MODULE_NAME), ggpm = NULL;
	if (gpci_bus)	put_module (B_PCI_MODULE_NAME), gpci_bus = NULL;
}


_EXPORT const char **
publish_devices (void)
{
	/*  Return the list of supported devices  */
	return ((const char **) gdd->dd_DevNames);
}


_EXPORT device_hooks *
find_device (const char *name)
{
	int index;

	for (index = 0;  gdd->dd_DevNames[index];  index++) {
		if (strcmp (name, gdd->dd_DevNames[index]) == 0)
			return (&graphics_device_hooks);
	}
	return (NULL);

}
