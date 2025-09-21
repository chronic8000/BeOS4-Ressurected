/*	pcm.h*/
/*	Sound Blaster Pro*/
/*	Joseph Wang*/
/*	4.4.99*/

#ifndef PCM_H
#define PCM_H

#include "sound.h"
#include "audio_driver.h"

/*	These formats are for the raw interface.	*/
/*	The old interface is hardcoded at 44kHz stereo 16 bit */
/*	with software conversion. */
#define DEFAULT_SAMPLE_RATE		22050.0F
#define DEFAULT_CHANNELS		2
#define DEFAULT_FORMAT			0x11
#define DEFAULT_ENDIAN			0
#define DEFAULT_BUFFER_HEADER	0
#define DEFAULT_WRITE_BUF_SIZE	B_PAGE_SIZE
#define DEFAULT_REC_BUF_SIZE	B_PAGE_SIZE


#if 0

enum {
	MODE_NONE,
	MODE_WRITE_8BIT_AUTO
	};

//iocntrl
enum {
	SOUND_GET_PARAMS = B_DEVICE_OP_CODES_END,
	SOUND_SET_PARAMS,
	SOUND_SET_PLAYBACK_COMPLETION_SEM,
	SOUND_SET_CAPTURE_COMPLETION_SEM,
	SOUND_GET_PLAYBACK_TIMESTAMP,
	SOUND_GET_CAPTURE_TIMESTAMP,
	SOUND_DEBUG_ON,
	SOUND_DEBUG_OFF,
	SOUND_UNSAFE_WRITE,
	SOUND_UNSAFE_READ,
	SOUND_LOCK_FOR_DMA
	};
enum {
	B_AUDIO_GET_AUDIO_FORMAT = B_AUDIO_DRIVER_BASE,
	B_AUDIO_SET_AUDIO_FORMAT,
	B_AUDIO_GET_PREFERRED_SAMPLE_RATES,
	B_ROUTING_GET_VALUES,
	B_ROUTING_SET_VALUES,
	B_MIXER_GET_VALUES,
	B_MIXER_SET_VALUES,
	
	/* SB16 driver extension */
	B_MIXER_GET_SCOPE = B_AUDIO_DRIVER_BASE + 99
	};

typedef struct {
	float       sample_rate;    /* ~4000 - ~48000, maybe more */
	int32       channels;       /* 1 or 2 */
	int32       format;         /* 0x11 (uchar), 0x2 (short) or 0x24 (float) */
	int32       big_endian;    /* 0 for little endian, 1 for big endian */
	size_t      buffer_header;     /* typically 0 or 16 */
	size_t      write_buf_size;	/* size of playback buffer (latency) */
	size_t		rec_buf_size;	/* size of record buffer (latency) */
} audio_format;

typedef struct {
  int32 buffer_number;
  int32 subscriber_count;
  bigtime_t time;
  int32 reserved_1;
  int32 reserved_2;
  bigtime_t sample_clock;
} audio_buffer_header;
#endif

/*extern int32 mode, write_offset, write_size, write_state, write_latency;*/
/*extern bigtime_t write_time, write_clock;*/
/*extern sem_id write_sem;*/
/**/
// extern size_t write_limit;

status_t pcm_open(const char *name, uint32 flags, void **cookie);
status_t pcm_close(void *cookie);
status_t pcm_free(void *cookie);
status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);
status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);
status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);

/*extern status_t pcm_set_params(sound_setup *setup);*/
/*extern status_t pcm_get_params(sound_setup *setup);*/
/*extern void set_dma_mode(int32 new_mode, audio_format *format, int32 len);*/
int32 pcm_write_inth(void *data);

#endif
