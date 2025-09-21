#ifndef _ATA_DEVICELIST_H
#define _ATA_DEVICELIST_H

typedef struct driver_device_entry {
	bool locked;
	bool media_change_requested;
#if __INTEL__
	struct bios_drive_info *bios_drive_info;
#endif
	bool dma_enabled;
	uint8	dma_status;
	uint32	max_transfer_size;
} driver_device_entry;

/*
typedef struct driver_device_handle {
	//status_t status;	// B_OK or B_DEV_MEDIA_CHANGED
	//bool not_ready_on_open;
} driver_device_handle;
*/

#include <devicelist.h>

#endif
