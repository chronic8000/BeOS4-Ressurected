#ifndef _DEVICELIST_H
#define _DEVICELIST_H

#ifdef STATIC_DRIVER_SYMBOL_MAP
#include STATIC_DRIVER_SYMBOL_MAP
#endif

#include "checkmalloc.h"
#include <IDE.h>
#include <ide_device_info.h>

typedef struct device_entry {
	struct device_entry			*next;
	bool						valid;
	bool						suspended;
//	char 						*basename;
	uint32						busnum;
	uint32						devicenum;
	ide_bus_info 				*bus;
	uint32 						buscookie;
	int							opencount;
	ide_device_info 			*di;
	ide_task_file 				*tf;
	struct device_handle		*device_handles;
	uint32 						bad_alignment_mask;
	uint32						maxlun;
	struct driver_device_entry	*priv;
} device_entry;

typedef struct device_handle {
	struct device_handle		*next;
	char						*name;
	device_entry				*d;
	uint32						lun;
	status_t					status;	// B_OK or B_DEV_MEDIA_CHANGED
	bool						not_ready_on_open;
	struct driver_device_handle	*priv;
} device_handle;

typedef enum {
	IDE_ATA = 0,
	IDE_ATAPI = 1
} ide_device_type;

/*
 * Exported 
 */

extern status_t init_devicelist(ide_device_type devtype);
extern void uninit_devicelist(void);
extern status_t create_devicelist(ide_device_type devtype);
extern status_t open_device(const char *name, device_handle **hp, ide_device_type devtype);
extern status_t close_device(device_handle *h);
extern status_t media_changed_in_lun(device_entry *d, uint32 lun);
extern status_t wake_devices();
extern status_t suspend_devices();

extern status_t acquire_bus(device_entry *d);
extern status_t release_bus(device_entry *d);

extern char **ide_names;

/*
 * Imported 
 */
extern bool is_supported_device(ide_device_info *di);
extern status_t configure_device(device_entry *d);
extern void release_device(device_entry *d);
extern void wake_device(device_entry *d);
extern void suspend_device(device_entry *d);

extern ide_module_info *ide;

#endif /* _DEVICELIST_H */
