#define R3_COMPATIBILITY 0

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <R3MediaDefs.h>
#include <sound.h>
#include <byteorder.h>
#include <audio_driver.h>
#include <midi_driver.h>
#include <config_manager.h>
#include <cs4231.h>
#include <unistd.h>

#define CHIP_NAME "awe64"

#define ddprintf
//#define ddprintf dprintf
#define X(N) dprintf("*%d*\n",(N));

//#undef ANALYZE
//#include <analyser.h>

#define NUM_CARDS 2
#define DEVNAME 32

#include <ISA.h>
#include <isapnp.h>
extern isa_module_info *isa;

#define READ_IO_8(A) ((*isa->read_io_8)(A))
#define WRITE_IO_8(A,D) ((*isa->write_io_8)(A,D))
#define START_ISA_DMA(C,A,S,M,E) ((*isa->start_isa_dma)(C,A,S,M,E))
#define LOCK_ISA_DMA_CHANNEL(C) ((*isa->lock_isa_dma_channel)(C))
#define UNLOCK_ISA_DMA_CHANNEL(C) ((*isa->unlock_isa_dma_channel)(C))
#define GET_NTH_ISA(N,DEV,FD,COOKIE) (get_nth_isa(COOKIE, DEV))

#define SB_Mix_Address		4
#define SB_Mix_Data			5
#define SB_DSP_Reset		6
#define SB_DSP_Read			10
#define SB_DSP_Write		12
#define SB_DSP_Write_Status	12
#define SB_DSP_Read_Status	14
#define SB_DSP_DMA_8_Ack	14
#define SB_DSP_DMA_16_Ack	15

#define SB_DSP_SetOutputRate	0x41
#define SB_DSP_SetInputRate		0x42
#define SB_DSP_Playback_16		0xb6
#define SB_DSP_Capture_16		0xbe
#define SB_DSP_Playback_8		0xc6
#define SB_DSP_Capture_8		0xce
#define SB_DSP_Pause_8			0xd0
#define SB_DSP_Continue_8		0xd4
#define SB_DSP_Pause_16			0xd5
#define SB_DSP_Continue_16		0xd6
#define SB_DSP_Exit_16			0xd9
#define SB_DSP_Exit_8			0xda
#define SB_DSP_GetVersion		0xe1

#define DMA_INT_8_BIT		1
#define DMA_INT_16_BIT		2
#define DMA_INT_MIDI_BIT	4

#define BUFTIME_SIZE		sizeof(audio_buffer_header)
#define MAX_DMA_BUF_SIZE	(2*4096)
#define MIN_DMA_BUF_SIZE	(2*1024)

typedef struct _midi_dev {
	struct sb_card *card;
	void *		driver;
	void *		cookie;
	int32		count;
	uint32      base;
	char		name[64];
} midi_dev;

typedef audio_format pcm_cfg;

typedef enum {DMA_STOPPED, DMA_PAUSED, DMA_RUNNING} dma_mode;

typedef struct {
  struct pcm_dev* port;
  bool		capture_p;
  bool		wide_p;
  int32		channel;
  int32		preferred_buf_size;

  spinlock	mode_lock;
  dma_mode	mode;

  int32		start;
  int32		pause;
  int32		cont;
  int32		exit;

  char*		buf_addr;
  int32		buf_size;
  void*		buf_phys_addr;
  uint32	index;
  sem_id	sem;

  bigtime_t	last_int;
  int64		inth_count;
  int64		rw_count;
} pcm_dma;

typedef struct pcm_dev {
  struct sb_card* card;
  char		name[DEVNAME];
  int32		open_count;

  pcm_dma	c;				/* capture dma */
  pcm_dma	p;				/* playback dma */

  sem_id	read_lock;		/* could be a benaphore */
  sem_id	write_lock;		/* could be a benaphore */
  pcm_cfg	config;
} pcm_dev;

typedef struct {
  struct sb_card* card;
  char		name[DEVNAME];
  int32		open_count;
} mux_dev;

typedef struct {
  struct sb_card* card;
  char		name[DEVNAME];
  int32		open_count;
} mixer_dev;

typedef struct {
  struct sb_card* card;
  char		name[DEVNAME];
  int32		open_count;
  pcm_dev*	pcm;

  sound_setup setup;
  sem_id	p_completion_sem;	/* sem to release when buffer complete */
  sem_id	c_completion_sem;	/* sem to release when buffer complete */
} old_dev;

typedef struct sb_card {
  char		name[DEVNAME];	/* used for resources */
  int32		inth_count;
  spinlock	dsp_lock;
  spinlock	mix_lock;
  int32		base;
  int32		version;

  int32		IRQ;
  int32		DMA_C;
  int32		DMA_P;

  area_id	dma_area;
  char*		dma_addr;
  void*		dma_phys_addr;
  uint32	sample_rate;

  pcm_dev	pcm;
  mux_dev	mux;
  mixer_dev	mix;
  midi_dev	midi;
  old_dev	old;

} sb_card;


extern int32 num_cards;
extern sb_card cards[NUM_CARDS];

extern void increment_interrupt_handler(sb_card*);
extern void decrement_interrupt_handler(sb_card*);
extern void pcm_interrupt_16(sb_card*);

extern cpu_status lock_spinner(spinlock* spinner);
extern void unlock_spinner(spinlock* spinner, cpu_status ps);
extern uchar dsp_read_byte(sb_card* sb);
extern void dsp_write_byte(sb_card* sb, uchar byte);
extern int32 dsp_read_n(sb_card* sb, uchar command, uchar n);
extern void dsp_write_n(sb_card* sb, uchar command, uchar n, int32 data);
extern uchar mixer_reg(sb_card* sb, uchar reg);
extern void mixer_change(sb_card* sb, uchar reg, uchar mask, uchar merge);
extern status_t find_low_memory(sb_card* sb);
extern void make_device_names(sb_card* sb);

extern bool midi_interrupt(sb_card* dev);
