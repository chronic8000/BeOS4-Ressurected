/*	xpress.h	*/

#if !defined(xpress_h)
#define xpress_h
#include <KernelExport.h>
#include "game_audio.h"
#include "ac97.h"

/* debug */
#if DEBUG
#define ddprintf(x) dprintf x
#define TESTING
#define KASSERT(x) { if (!x) {dprintf("Assert failed in %s on line %d \n",__FILE__,__LINE__); panic("ASSERT"); }}
#define IKASSERT(x) {if (!x) {kprintf("Assert failed in %s on line %d \n",__FILE__,__LINE__); panic("ASSERT"); }}
#else
#define ddprintf(x) (void)0
#define KASSERT(x) (void)0
#define IKASSERT(x) (void)0
#undef  TESTING
#endif


/* hardware specific defines */
#define XPRESS_VENDOR_ID	0x1078
#define XPRESS_DEVICE_ID	0x0103

/* hardware register offsets */
#define X_CODEC_COMMAND_REG		0x0c
#define X_CODEC_STATUS_REG		0x08

/* driver specific */
#define MAX_XPRESS_BD			1

#define X_MIN_BUFF_ALLOC		4096

#define X_NUM_DACS				1
#define X_NUM_DAC_STREAMS		1
#define X_NUM_ADCS				1
#define X_NUM_ADC_STREAMS		1
#define X_MAX_STREAMS			((X_NUM_DACS *	\
								  X_NUM_DAC_STREAMS) + \
								 (X_NUM_ADCS * 	\
								  X_NUM_ADC_STREAMS) )  
#define X_MAX_STREAM_BUFFERS	32
#define X_MAX_UNBOUND_BUFFERS	32
#define X_MAX_BUFFERS			((X_MAX_STREAM_BUFFERS	\
								* X_MAX_STREAMS) +		\
								  X_MAX_UNBOUND_BUFFERS )

enum {
	X_RECORD_GAIN = 0,
	X_RECORD_MUX,
	X_MIC_BOOST,
	X_MIC_MUX,
	X_NUM_ADC_CONTROLS
};	
enum {	
	X_MASTER = X_NUM_ADC_CONTROLS,
	X_HEADPHONE,
	X_MASTER_MONO,
	X_BEEP,
	X_PCM_OUT,
	X_PHONE,
	X_MIC_OUT,
	X_LINE_IN,
	X_CD,
	X_VIDEO,
	X_AUX,
	X_3D_HW_BYPASS,
	X_3D_SOUND,
	X_LOOPBACK,
	X_MONO_MUX,
	X_NUM_CONTROLS
};	

#define X_NUM_DAC_CONTROLS (X_NUM_CONTROLS - X_NUM_ADC_CONTROLS)

#define X_NUM_MIXERS 2
#define X_NUM_LEVELS 12
#define X_NUM_ENABLES 4
#define X_NUM_MUXES 3
#define X_NUM_MUX_ITEMS 12


#define X_VERSION 0x5150
#define X_MODEL_NAME "Xpress Audio"
#define X_MANUF_NAME "National Semiconductor"

#define MEM_RD32(a)	  (*((uint32 *)(a)))
#define MEM_RD16(a)	  (*((uint16 *)(a)))
#define MEM_RD8(a)	  (*((uint8  *)(a)))
#define MEM_WR32(a,v) (*((uint32 *)(a))) = ((uint32)(v))
#define MEM_WR8(a,v)  (*((uint8  *)(a))) = ((uint8) (v))

/* # of PRDs */
#define PRD_DEPTH		8
/* # of slots between add and remove */
#define PRD_FREE_SLOTS	1	

#define X_EOT		(0x8000)
#define X_EOP		(0x4000)
#define	X_JMP		(0x2000)

#define SUPER_SECRET_START_OF_BUFFER (0x2000)
#define SUPER_SECRET_END_OF_BUFFER (0x4000)
#define SUPER_SECRET_CLEAR_EOP (0x8000)

/* cached codec controls simplify setup */
#define X_MAX_CCC 64

struct _PRD_data {
	void * 				buff_phys_addr;
	uint16				size;
	uint16				flags;
};	

struct _buffer_data {
	physical_entry *  	phys_entry;
	area_id				area;
	size_t				offset;
	size_t				byte_size;
	uint32 *			virt_addr;
	int16				stream;
	int16				num_entries;
	int16				idx;
	/* ref count is number of queues */
	/* and as such must be protected */
	/* by the queue spinlock         */
	int16				ref_count;
};	

typedef struct queue_item queue_item;
struct queue_item {
	queue_item *		next;
	uint32				q_flags;
	uint32 *			virt_addr;
	void *				phys_addr;
//remove this....
	int16				phys_idx;
	int16				buffer;
	int16				in_prd;
	uint16				size;
};

#define X_ALLOCATED		0x01
#define X_CLOSING		0x02
#define X_DMA_DONE_SEM	0x04
#define X_DMA_STOPPED	0x08

struct _stream_data {
	/* queue items are touched in the isr   */
	/* they must be protected by a spinlock */
	/* see also ref_count in buffer_data    */
	queue_item *		stream_buffer_queue; //buffer_ids
	queue_item *		free_list; 
	uint32				prd_add;
	uint32				prd_remove;
	/* non queue items............ */
	uint32				frame_count;
	bigtime_t			timestamp;
	volatile struct _PRD_data *
						prd_table;			
	int16				my_flags;
	int16				state;
	sem_id				gos_stream_sem;
	int16				gos_adc_dac_id;	
	int16				bind_count;	//buffer_ids
	sem_id				all_done_sem;
	void *				cookie;
};


typedef struct xpress_dev xpress_dev;

typedef struct xpress_host xpress_host;
struct xpress_host {
	xpress_dev *	device;
	uchar			index;
};

struct xpress_dev {
	uint32					ix;
	uint32					pci_bus;
	uint32					pci_device;
	uint32					pci_function;
	uint32					pci_interrupt;
	uint32					f3bar;
	uint32					open_mode;
	sem_id					open_sem;
	sem_id					control_sem;
	sem_id					irq_sem;
	bigtime_t				timestamp;
	cpu_status				cp;
	uint32					spinlock;
	area_id					f3_area;
	uchar *					f3;
	area_id					fourf0_area;
	uchar *					fourf0;
	/* ac97 support			*/
	int32					ac97ben[1];
	sem_id					ac97sem[1];
	ac97_t					codec[1];
	xpress_host 			xhost[1];
	uint16					ccc[X_MAX_CCC];

	/* stream (resource) support */
	int32					stream_ben;
	sem_id					stream_sem;

	game_mixer_info			mixer_info[X_NUM_MIXERS];
	game_adc_info			adc_info[X_NUM_ADCS];
	game_dac_info			dac_info[X_NUM_DACS];
	struct _stream_data		stream_data[X_MAX_STREAMS];	//total
	struct _buffer_data		buffer_data[X_MAX_BUFFERS]; //total
	int16					current_buffer_count;
};

typedef struct open_xpress open_xpress;
struct open_xpress {
	xpress_dev *	device;
	uint32			mode;
};


#endif /* xpress_h */
