#ifndef _ATAPI_CMDS_H
#define _ATAPI_CMDS_H

#include "devicelist.h"
#include <scsi.h>

typedef struct {
	uint32 last_lba;				   /* last logical block address */
	uint32 blocksize;				   /* in bytes */
} atapi_capacity_data;

typedef struct {
	LBITFIELD8_3(
		current_slot		: 5,
		changer_state		: 2,
		fault				: 1
	);
	LBITFIELD8_2(
		reserved_1_0to4		: 5,
		cd_mechanism_state	: 3
	);
	uint8	current_lba_16_to_23;
	uint8	current_lba_8_to_15;
	uint8	current_lba_0_to_7;
	uint8	number_of_slots;
	uint16	slot_table_size;	// big endian
} atapi_mechanism_status_header;

typedef struct {
	LBITFIELD8_3(
		change				: 1,
		reserved_0_1to6		: 6,
		disc_present		: 1
	);
	uint8 reserved_1;
	uint8 reserved_2;
	uint8 reserved_3;
} atapi_slot_info;

#ifdef MILLISEC
#undef MILLISEC
#endif
#define MILLISEC	1000 


/*
 * Exported
 */

extern status_t test_unit_ready(device_handle *h);
extern status_t read_capacity(device_handle *h, atapi_capacity_data *data);
extern status_t play_audio_msf(device_handle *h, scsi_play_position *position);
extern status_t request_sense(device_handle *h, uint8 *prevcmd, uint8 *sense_data, size_t sense_data_size);
extern status_t readsector(device_handle *h, uint32 sect, uint32 nsect, const iovec *vec, size_t count, uint32 startbyte);
extern status_t writesector(device_handle *h, uint32 sect, uint32 nsect, const iovec *vec, size_t count, uint32 startbyte);
extern status_t read_toc(device_handle *h, scsi_toc *tocp);
extern status_t atapi_inquiry(device_handle *h, scsi_inquiry *inq_data_p);
extern status_t changer_unload_cd(device_entry *a);
extern status_t changer_load_cd(device_entry *a);
extern status_t mechanism_status(device_handle *h, void *buffer,
                                 size_t buffersize);
extern status_t mode_sense(device_handle *h, uint8 page_control,
                           uint8 page_code, uint8 *buf, size_t buflen);
extern status_t mode_select(device_handle *h, bool save_page,
                            uint8 *buf, size_t buflen);
extern status_t stop_play_scan_cd(device_handle *h);
extern status_t pause_resume(device_handle *h, bool resume);
extern status_t load_unload_cd(device_handle *h, uint8 slot, bool load);
extern status_t start_stop_unit(device_handle *h, bool start, bool eject);
extern status_t get_current_position(device_handle *h, scsi_position *pos);
extern status_t scan_audio(device_handle *h, scsi_scan *scan_info);
extern status_t read_cd(device_handle *h, scsi_read_cd *read_info);
extern status_t prevent_allow(device_handle *h, bool prevent);


/*
 * Imported
 */

extern status_t send_atapi_command(device_handle *h, uint8 *cmd_data,
                                   const iovec *vec, size_t veccount,
                                   uint32 startbyte, size_t *numBytes,
                                   bool write, bool expect_short_read);

extern status_t reconfigure_device(device_handle *h);

#endif /* _ATAPI_CMDS_H */
