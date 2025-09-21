#ifndef _ESS1938_H
#define _ESS1938_H

#include <PCI.h>
#include <audio_driver.h>
#include <KernelExport.h>	/* for physical_entry */
#include <midi_driver.h>
#include <joystick_driver.h>

#if DEBUG
#define ddprintf dprintf
#define KPRINTF  kprintf
#else
#define ddprintf (void)
#define KPRINTF  (void)
#endif

#if defined(__INTEL__)
#define EIEIO()
#elif defined(__POWERPC__)
#define EIEIO() __eieio()
#endif

#define ESS1938_VENDOR_ID 	0x125D
#define ESS1938_DEVICE_ID 	0x1969

#define DEVNAME				32
#define NUM_CARDS 			3

#define	PLAYBACK_BUF_SIZE	2048
#define	RECORD_BUF_SIZE		2048

#define MIN_MEMORY_SIZE		256


typedef struct audio_format pcm_cfg;

typedef struct {
	uchar					left;
	uchar					right;
} stereo;

typedef struct {
	struct _ess1938_dev *	card;
	void *					driver;
	void *					cookie;
	int32					count;
	char					name[64];
} midi_dev;

typedef struct {
	void *					driver;
	char					name[64];
} joy_dev;

typedef struct {
	struct _ess1938_dev * 	card;
	char					oldname[DEVNAME];
	pcm_cfg					config;     /* mixer_sem */
	stereo					aux1_mute;	/* mixer_sem */
	stereo					aux2_mute;  /* mixer_sem */
	stereo					line_mute;  /* mixer_sem */
	stereo					dac_mute;   /* mixer_sem */
	stereo					mic_mute;   /* mixer_sem */

} pcm_dev;

typedef struct _ess1938_dev {
	char			name[DEVNAME];	/* used for resources */
	int32			spinlock;		/* duhhhhh */
	sem_id			open_close_sem;	/* serialize opens, closes, etc.. */
	sem_id			pcmwrite_sem;   /* serialize pcmwrite */
	sem_id			pcmread_sem;   	/* serialize pcmread */
	sem_id			mixer_sem;		/* for access to sbbase + 4h/5h */
	sem_id			controller_sem;	/* for access to sbbase + Ah/Ch/Eh (Eh?) */
	sem_id			write_sem;
	sem_id			read_sem;
	sem_id			lastint_sem;	/* for the last interrupt (pan bug) */
	sem_id			old_play_sem;   /* for unsafe write */
	sem_id			old_cap_sem;	/* for unsafe read */
	int				iobase;			/* base address (main) */
	int 			sbbase;			/* base address (sb) */
	int				vcbase;			/* base address (vc) -- this is DDMA */
	char *			dma_buf;		/* playback */
	char * 			dma_buf_ptr;
	area_id			dma_area;
	physical_entry	dma_phys;
	char *			dma_cap_buf;	/* capture */
	char * 			dma_cap_buf_ptr;
	area_id			dma_cap_area;
	physical_entry	dma_cap_phys;
	int32			init_cnt;
	int32			inth_count;
	int32			closing;
	bigtime_t		write_time;
	uint64			write_total;
	int32			write_waiting;
	bigtime_t		read_time;
	uint64			read_total;
	int32			read_waiting;
	char			low_byte_l;
	char			high_byte_l;
	char			low_byte_r;
	pci_info		info;
	pcm_dev			pcm;
	midi_dev		midi;
	joy_dev			joy;
} ess1938_dev;

extern int32 num_cards;
extern ess1938_dev cards[NUM_CARDS];
extern generic_gameport_module * gameport;

/* Offsets */
#define MIXER_COMMAND	0x04
#define MIXER_DATA		0x05
#define AUDIO_RESET		0x06
#define CONTROL_READ	0x0A
#define CONTROL_WRITE	0x0C
#define CONTROL_STATUS	0x0C
#define RD_BUF_STATUS	0x0E
#define IMR_ISR			0x07

/* DDMA Offsets */
#define DDMA_CLEAR		0x0D
#define DDMA_MODE		0x0B
#define DDMA_COUNT_L	0x04
#define DDMA_COUNT_H	0x05
#define DDMA_MASK		0x0F

/* Commands */
#define READ_CONTROL_REG		0xC0
#define ENABLE_EXTENDED_CMDS	0xC6

/* Controller Registers */
#define RECORD_LEVEL	0xB4
#define AUDIO_1_SR		0xA1
#define AUDIO_1_FILTER  0xA2
#define AUDIO_1_CTRL_1  0xB7
#define AUDIO_1_CTRL_2	0xB8
#define AUDIO_1_XFER	0xB9
#define AUDIO_1_XFR_L	0xA4
#define AUDIO_1_XFR_H	0xA5
#define ANALOG_CTRL		0xA8
#define LEGACY_IRQ_CTRL	0xB1
#define LEGACY_DRQ_CTRL	0xB2

/* Source Select Register */
#define ADC_SOURCE	0x1C
/* Source Select Values */
#define MIC		0x0		/* default */
#define AUXA	0x2		/* CD	*/
#define MIC_X	0x4		/* ???	*/
#define MIXER	0x5		/* record mixer */
#define LINE	0x6		/* line in */

/* Mixer Registers */
#define RESET			0x00
#define MASTER_VOL_CTRL	0x64
#define AUDIO_1_PB_VOL	0x14
#define AUDIO_2_PB_VOL	0x7C
#define AUDIO_2_CAP_VOL	0x69
#define MIC_PB_VOL		0x1A
#define MIC_CAP_VOL		0x68
#define FM_DAC_PB_VOL	0x36	/* HWWT */
#define FM_DAC_CAP_VOL	0x6B	/* HWWT */
#define AUXA_PB_VOL		0x38	/* CD	*/
#define AUXA_CAP_VOL	0x6A	/* CD	*/
#define AUXB_PB_VOL		0x3A
#define AUXB_CAP_VOL	0x6C
#define LINE_PB_VOL		0x3E
#define LINE_CAP_VOL	0x6E
#define MONO_IN_PB_VOL	0x6D
#define MONO_IN_CAP_VOL	0x6F
#define MIC_MONO_MISC	0x7D
#define AUDIO_2_CTRL_1	0x78
#define AUDIO_2_CTRL_2  0x7A
#define AUDIO_2_SR		0x70
#define AUDIO_2_MODE	0x71
#define AUDIO_2_FILTER  0x72
#define AUDIO_2_XFR_L	0x74
#define AUDIO_2_XFR_H	0x76

/* Prototypes */
uchar get_direct(int);
void set_direct(int, uchar,	uchar);
uchar get_indirect( int, int, int);
void set_indirect( int, int, int, uchar, uchar );
uchar get_controller( ess1938_dev *);
void set_controller( ess1938_dev *,	uchar);
void set_controller_fast( ess1938_dev *, uchar, uchar);
void start_dma_2( ess1938_dev *);
void setup_dma_2( ess1938_dev *);
void setup_dma_1( ess1938_dev *);

extern bool midi_interrupt(ess1938_dev *);
extern void midi_interrupt_op(int32 op, void * data);

#endif /* _ESS1938_H */

