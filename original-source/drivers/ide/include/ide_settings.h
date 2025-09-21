#ifndef IDE_SETTINGS_H
#define IDE_SETTINGS_H

#include <driver_settings.h>

/*
 * Exported 
 */

extern status_t read_settings_device(device_entry *d, const char *drivername);

/*
 * Imported 
 */

extern status_t use_settings_device(device_entry *d, driver_parameter *s);

extern bool ide_dma_disabled(void);

#endif /* IDE_SETTINGS_H */
