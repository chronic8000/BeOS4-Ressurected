#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <R3MediaDefs.h>
#include <sound.h>
#include <byteorder.h>
#include <audio_driver.h>
#include <midi_driver.h>
#include <cs4231.h>
#include <unistd.h>

#define CHIP_NAME "opti931"

#define ddprintf
//#define ddprintf dprintf
#define X(N) dprintf("*%d*\n",(N));

#define NUM_CARDS 2
#define DEVNAME 32

#define BUFTIME_SIZE sizeof(audio_buffer_header)
#define MAX_DMA_BUF_SIZE (2*4096)
#define MIN_DMA_BUF_SIZE (2*1024)

#include <config_manager.h>
#include <ISA.h>
#include <isapnp.h>

extern isa_module_info *isa;

#define READ_IO_8(A) ((*isa->read_io_8)(A))
#define WRITE_IO_8(A,D) ((*isa->write_io_8)(A,D))
#define START_ISA_DMA(C,A,S,M,E) ((*isa->start_isa_dma)(C,A,S,M,E))
#define LOCK_ISA_DMA_CHANNEL(C) ((*isa->lock_isa_dma_channel)(C))
#define UNLOCK_ISA_DMA_CHANNEL(C) ((*isa->unlock_isa_dma_channel)(C))

typedef struct midi_dev {
	struct sb_card* card;
	char name[DEVNAME];
	int32 IRQ;
	int32 midi_inth_count;
	void* driver;
	void* cookie;
	int32 count;
} midi_dev;

typedef audio_format pcm_cfg;

typedef enum {DMA_STOPPED, DMA_PAUSED, DMA_RUNNING} dma_mode;

typedef struct {
  struct pcm_dev* port;
  bool		capture_p;

  dma_mode	mode;

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
  spinlock	mode_lock;		/* locks both dmas mode */

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
  int32		mc_base;
  spinlock	mc_lock;
  int32		wss_base;
  spinlock	wss_lock;
  spinlock	mpu_lock;
  uint32	card_id_0;
  uint32	card_id_1;
  uchar		card_csn;

  int32		IRQ;
  int32		DMA_C;
  int32		DMA_P;

  area_id	dma_area;
  char*		dma_addr;
  void*		dma_phys_addr;

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
extern bool pcm_interrupt_16(pcm_dma*);

extern cpu_status lock_spinner(spinlock* spinner);
extern void unlock_spinner(spinlock* spinner, cpu_status ps);
extern status_t find_low_memory(sb_card* sb);
extern void make_device_names(sb_card* sb);
extern status_t complete_mode_change (sb_card* sb);

extern uchar iread (sb_card* sb, int offset);
extern void ichange (sb_card* sb, int offset, uchar mask, uchar merge);
extern uchar mc_read (sb_card* sb, int offset);
extern void mc_change (sb_card* sb, int offset, uchar mask, uchar merge);
extern int32 midi_int_handler(void* data);
extern void midi_interrupt_op(int32 op, void* data);
