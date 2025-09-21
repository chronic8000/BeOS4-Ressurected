#ifndef DAC960_H
#define DAC960_H

#include <OS.h>
#include <KernelExport.h>

#define PCI_DAC960_VENDOR_ID 0x1069
#define PCI_DAC960_DEVICE_ID 0x10

#define IN_MAILBOX_FULL 0x01
#define IN_NEW_COMMAND  0x01
#define IN_STATUS_ACK   0x02
#define IN_SOFT_RESET   0x08

#define OUT_STATUS_AVAIL 0x01

#define INT_ENABLE 0xFB
#define INT_DISABLE 0xFF

typedef struct
{
	uint16 blocks;
	uint16 reserved[3];
	physical_entry sglist[33];
} SGLIST;

typedef struct 
{
	uchar system_drives;
	uchar _reserved0[3];
	
	uint32 size[32];
	
	uint16 _reserved1;
	uchar status_flags;
	uchar _reserved2;
	
	uchar firmware_minor_version;
	uchar firmware_major_version;
	uchar rebuild_flag;
	uchar max_commands;
	
	uchar offline_sd_count;
	uchar _reserved3;
	uint16 eventlogseqnum;
	
	uchar critical_sd_count;
	uchar _reserved4[3];
	
	uchar dead_count;
	uchar _reserved5;
	uchar rebuild_count;
	uchar misc_flags;
	
	struct {
		uchar target;
		uchar channel;
	} dead[20];
} enquiry_data;

typedef struct 
{
	uchar hardware_id[4];
	uchar firmware_id[4];
	uchar _reserved0[4];
	uchar configured_channels;
	uchar actual_channels;
	uchar max_targets;
	uchar max_tags;
	uchar max_sys_drives;
	uchar max_arms;
	uchar max_spans;
	uchar _reserved1;
	uchar _reserved2[4];
	uint32 dram_size;
	uchar cache_size[5];
	uchar flash_size[3];
	uint32 nvram_size;
	uint16 memory_type;
	uint16 clock_speed;
	uchar _reserved3[2];
	uint16 hardware_speed;
	uchar _reserved4[12];
	uint16 max_commands;
	uint16 max_sg_entries;
	uint16 max_dp;
	uint16 max_iod;
	uint16 max_comb;
	uchar latency;
	uchar _reserved5;
	uchar scsi_time_out;
	uchar _reserved6;
	uint16 min_freelines;
	uchar _reserved7[8];
	uchar rate_const;
	uchar _reserved8[3];
	uchar _reserved9[8];
	uint16 physical_block_size;
	uint16 logical_block_size;
	uint16 maxblk;
	uint16 blocking_factor;
	uint16 cache_line_size;
	uchar scsi_cap;
	uchar _reserved10[5];
	uint16 firmware_build_number;
	uchar fault_mgmt_type;
	uchar _reserved11;
	uint32 firmware_features;
	uchar _reserved12[8];
} enquiry2_data;

typedef struct
{
	uchar opcode;        /* 0xb3 / 0xb4 = read / write */
	uchar id;
	uchar sgcount;       /* 33 max */
	uchar _reserved0;
	uint32 block;
	uint32 phys_ptr;
	uchar system_drive;
} rw_ext_sg_cmd;

typedef struct 
{
	uchar opcode;
	uchar id;
	uchar drive;
	uchar _reserved0[5];
	uint32 phys_ptr;
	uchar _reserved1[1];
} cmd_type3;

typedef struct 
{
	uchar opcode;
	uchar id;
	uchar channel;
	uchar target;
	uchar _reserved0[4];
	uint32 phys_ptr;
	uchar _reserved1[1];
} cmd_dev_state;

typedef struct 
{
	uchar present;
	uchar flags;
	uchar status;
	uchar _reserved;
	uchar period;
	uchar offset;
	uint32 size;
} device_state;

typedef struct 
{
	uint32 size;
	uchar state;
	uchar level;
	uchar _reserved[2];
} sys_drv_info;

typedef struct 
{
	uchar state;
	uchar status;
	uchar modifier1;
	uchar modifier2;
	uchar raid_level;
	uchar valid_arms;
	uchar valid_spans;
	uchar _reserved;
	struct {
		uint32 start;
		uint32 blocks;
		struct {
			uchar target:4;
			uchar channel:4;
		} arm[8];
	} span[4];
} sys_drv;

typedef struct 
{
	uint32 blocks;
	sys_drv sys_drv;             /* system drive info */
	device_state state[4][8];  /* status for each arm of each span */
} drive_info;

typedef struct
{
	uint32 channel;
	uint32 target;
} ioctl_rebuild;


typedef struct _DACCMD
{
	struct _DACCMD *next;
	int id;
	
	sem_id done;
	SGLIST *sglist;
	uint32 sglist_phys;
	uint32 status;
	
	union {
		uchar byte[32];
		uint32 word[4];
		cmd_type3 type3;
		rw_ext_sg_cmd io;
		cmd_dev_state getstate;
	} u;
} DACCMD;

typedef struct 
{
	int num;
	area_id shared_area, workspace_area;
	int irq;
	char name[64];
	int namelen;
	
	sem_id mailbox_lock;
	
	volatile uchar *inbound_doorbell;
	volatile uchar *outbound_doorbell;
	volatile uint32 *interrupt_mask;
	volatile uint32 *mailbox;
	volatile uchar *status_sequence;
	volatile uint32 *status;

	uint32 system_drives[32];
	
	int32 free_count;
	sem_id free_sem;
	spinlock free_lock;
	
	DACCMD *free_cmd;
	DACCMD cmds[15];
} DAC960;

#endif