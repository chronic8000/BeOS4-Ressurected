#ifndef SETUP_H
#define SETUP_H

#include "driver.h"
#include "sis620defs.h"

status_t map_device	(devinfo *di);
status_t sis630_map_device (devinfo *di);
void unmap_device	(devinfo *di);
status_t sis_init	(devinfo *di);

// VARIABLES GLOBALES de driver.o

extern pci_module_info *pci_bus;
extern driverdata *sis_dd;

#endif
