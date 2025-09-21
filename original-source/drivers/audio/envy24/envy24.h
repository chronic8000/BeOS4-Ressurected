/* envy24.h	*/
/* Copyright 2001, Be Incorporated. All Rights Reserved */

#if !defined(envy24_h)
#define envy24_h
#include <KernelExport.h>
#include "multi_audio.h"
#include "ac97_module.h"
#include "eeprom.h"

/* debug */
#if DEBUG
#define ddprintf(x) dprintf x
#define CHECK_CODE 1
#define TESTING
#else
#define ddprintf(x) (void)0
#define CHECK_CODE 0
#undef  TESTING
#endif

/* hardware specific defines */
#define ICENSEMBLE_VENDOR_ID 0x1412
#define ENVY24_DEVICE_ID 	 0x1712
#define ICE_SSVID			 0x1217
#define ENVY24_SSDID		 0x1214

#define OUTPUTS_PER_BD		 10
#define INPUTS_PER_BD		 10
#define DMAOUTS_PER_BD		 10
#define DMAINS_PER_BD		 12
#define AUXS_PER_BD			 2
#define MONITORS_PER_BD		 2

#define OUTPUT_CHNL_MASK	 0x3FF
#define INPUT_CHNL_MASK	 	 0x2FFC00

/* hardware hardware */
#define INT_MASK_REG			0x01
#define INT_STATUS_REG			0x02

#define MTPR_INT				0x10
#define MTPR_DMA_PB_ADDR_REG	0x10
#define MTPR_DMA_PB_CNT_REG		0x14
#define MTPR_DMA_PB_TC_REG		0x16
#define MTPR_DMA_REC_ADDR_REG	0x20
#define MTPR_DMA_REC_CNT_REG	0x24
#define MTPR_DMA_REC_TC_REG		0x26
#define MTPR_CTL_REG			0x18

#define MT_SAMPLE_RATE_REG      0x01
#define MT_DATA_FORMAT_REG      0x02

#define MT_ROUTE_CTL_REG_PSDOUT	0x30
#define MT_ROUTE_CTL_REG_SPDOUT	0x32
#define MT_CAP_ROUTE_REG		0x34
#define MT_VOL_CTL_REG			0x38
#define MT_VOL_IDX_REG			0x3a
#define MT_DMIX_MONITOR_ROUTE_REG 	0x3c

#define ICE_INDEX_REG           0x03
#define ICE_DATA_REG            0x04

#define ICE_IDXREG_GPIO_DATA        0x20
#define ICE_IDXREG_GPIO_WRITEMASK   0x21
#define ICE_IDXREG_GPIO_DIRECTION   0x22

#define ICE_I2C_DEVICE_ADDR_REG 	0x10
#define ICE_I2C_PORT_ADDR_REG       0x11
#define ICE_I2C_PORT_DATA_REG       0x12
#define ICE_I2C_PORT_CONTROL_REG    0x13

#define ICE_AC97_CON_INDEX		0x08
#define ICE_AC97_CON_CMD_STATUS	0x09
#define ICE_AC97_CON_DATA_PORT	0x0A

// Address of the E2Prom on the I2C bus. Use this
// value when programming the ICE_I2C_DEVICE_ADDR_REG.
#define ICE_I2C_E2PROM_ADDR         (0x50<<1)

/* ac97 stuff */
#define CONSUMER_AC97       0
#define PROFESSIONAL_AC97   1
//could be 5....
#define MAX_AC97_CODECS     1

/* driver specific */
#define ENVY24_MODE_0		0
#define MAX_ENVY24_BD		4

#define DEFAULT_MULTI_LOCK_SOURCE 		B_MULTI_LOCK_INTERNAL
#define DEFAULT_MULTI_TIMECODE_SOURCE	0
#define DEFAULT_MULTI_BUFF_SIZE			128
#define DEFAULT_SAMPLE_RATE				48000

/* driver software */
#define SUPPORTED_EVENT_MASK   (B_MULTI_EVENT_STARTED | \
								B_MULTI_EVENT_STOPPED   | \
								B_MULTI_EVENT_CHANNEL_FORMAT_CHANGED  | \
								B_MULTI_EVENT_CONTROL_CHANGED ) 
#define	MAX_EVENTS		16

typedef struct timer_plus timer_plus;
struct timer_plus {
	timer		timerx;
	void *		cookie;
};	

typedef struct envy24_channel envy24_channel;
struct envy24_channel {
	char * 	buf[2];
	uint32	current_buffer; // 0 or 1
};	

typedef struct envy24_dev envy24_dev;

typedef struct envy24_host envy24_host;
struct envy24_host {
	envy24_dev *	device;
	uchar			index;
};

struct envy24_dev {
	uint32					ix;
	uint32					pci_bus;
	uint32					pci_device;
	uint32					pci_function;
	uint32					pci_interrupt;
	uint32					ctlr_iobase;
	uint32					mt_iobase;
	uint32					open_mode;
	sem_id					open_sem;
	sem_id					control_sem;
	sem_id					irq_sem;
	uint32					spinlock;
	area_id					channel_area_pb;
	physical_entry			channel_physical_pb;
	area_id					channel_area_cap;
	physical_entry			channel_physical_cap;
	multi_channel_enable	mce;
	uint32 					channel_buff_size; // in samples
	uint32					sample_rate;	
	envy24_channel			channels[DMAINS_PER_BD + DMAOUTS_PER_BD];
	uint32					is_playing;
	bigtime_t				timestamp;
	uint64					pb_position;
	uint64					rec_position;
	uint32					preroll;
	bigtime_t				start_time;
	timer_plus				timer_stuff;
	/* EEPROM & hw stuff*/
	EEPROM					teepee;
	bool					bGpIoInitValue;
	/* codec */
	ac97_t_v3				codec[MAX_AC97_CODECS];
	sem_id 					ac97sem[MAX_AC97_CODECS];
	int32					ac97ben[MAX_AC97_CODECS];
	envy24_host 			ehost[MAX_AC97_CODECS];
	/* multi events */
	uint32					current_mask;
	uint32					queue_size;
	uint32					event_count;
	multi_get_event			event_queue[MAX_EVENTS];
	sem_id					event_sem;
	/* cached control values */
	uint16					volume[20];	
	uint32					mux[10];
	bool					enable;
	uint16					mon_volume;
	
};

typedef struct open_envy24 open_envy24;
struct open_envy24 {
	envy24_dev *	device;
	uint32			mode;
};

#endif /* envy24_h */
