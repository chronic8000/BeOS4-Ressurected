#ifndef _ATAPI_DEVICELIST_H
#define _ATAPI_DEVICELIST_H

#include <scsi.h>

typedef struct lun_info {
	uint32 				blocksize;
	uint32				capacity;
	scsi_toc			toc;
	bool				capacity_valid;
	bool				write_protect;
} lun_info;

typedef struct driver_device_entry {
	lun_info				*lia;
	uint32					current_lun; // 0xffffffff = unknown
	struct {
		uchar	end_m;
		uchar	end_s;
		uchar	end_f;
	} audio;
	struct atapi_mechanism_status *ms;
	bool Support_Disc_Present;
	bool read_write_12_unsupported;
	bool locked;
	bool media_change_requested;
	bool dma_enabled;
	uint8	dma_status;
} driver_device_entry;


typedef struct driver_device_handle {
	lun_info	*li;
	bool command_in_progress; // used by request sense to prevent infinite
	                          // recursion
} driver_device_handle;

#include <devicelist.h>

#endif
