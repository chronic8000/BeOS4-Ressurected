/*	pcm.c*/
/*	Sound Blaster Pro*/
/*	Joseph Wang*/
/*	11.4.99*/

#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>
#include <stdlib.h>
#include <unistd.h>
#include <R3MediaDefs.h>

#include "sbpro.h"
/*#include "driver.h"*/
#include "pcm.h"
#include "driver.h"

//static int32 mode, write_offset, write_size, write_state, write_latency;
static bigtime_t write_time, write_clock;
static sem_id write_sem;
sem_id int_sem;
static bool dma_running = false;
static bool setup_pending = false;
static sound_setup sb_sound_setup;

extern void start_dma(void);
extern void stop_dma(void);

/*static status_t pcm_open(const char *name, uint32 flags, void **cookie);*/
/*static status_t pcm_close(void *cookie);*/
/*static status_t pcm_free(void *cookie);*/
/*static status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);*/
/*static status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);*/
/*static status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);*/

static status_t pcm_set_params(sound_setup *setup);
static status_t pcm_get_params(sound_setup *setup);
//static void set_dma_mode(int32 new_mode, audio_format *format, int32 len);
/*static int32 pcm_write_inth(void *data);*/

status_t pcm_open(const char *name, uint32 flags, void **cookie) {
	audio_format *format=(audio_format *)malloc(sizeof(audio_format));
	dprintf("pcm_open(%s)\n", name);
	format->sample_rate=DEFAULT_SAMPLE_RATE;
	format->channels=DEFAULT_CHANNELS;
	format->format=DEFAULT_FORMAT;
	format->big_endian=DEFAULT_ENDIAN;
	format->buf_header=DEFAULT_BUFFER_HEADER;
	format->play_buf_size=DEFAULT_WRITE_BUF_SIZE;
	format->rec_buf_size=DEFAULT_REC_BUF_SIZE;
	*cookie=(void *)format;	//	hplus	bad assignment of "cookie"
//	mode=MODE_NONE;
	return B_OK;
	}

status_t pcm_close(void *cookie) {
	dprintf("sbpro: pcm_close()\n");
	return B_OK;
	}

status_t pcm_free(void *cookie) {
	dprintf("sbpro: pcm_free()\n");
	free(cookie);	//	hplus	leak!
	return B_OK;
	}

status_t pcm_control(void *cookie, uint32 op, void *data, size_t len) {
	status_t err;
	audio_buffer_header *header;
	static float rates[]={22050.0, 0, 0, 0};
	if (op != SOUND_UNSAFE_WRITE) {
		dprintf("pcm_control(%ld)\n", op);
	}
	switch(op) {
	case B_AUDIO_SET_AUDIO_FORMAT:
		//	just tell him what we do
		memcpy(data, cookie, min(sizeof(audio_format), len));
		return B_OK;
	case B_AUDIO_GET_AUDIO_FORMAT:
		memcpy(data, cookie, sizeof(audio_format));
		return B_OK;
	case B_AUDIO_GET_PREFERRED_SAMPLE_RATES:
		memcpy(data, rates, sizeof(rates));
		return B_OK;
	case SOUND_GET_PARAMS:
		return pcm_get_params((sound_setup *)data);
	case SOUND_SET_PARAMS:
		return pcm_set_params((sound_setup *)data);
/*	case SOUND_SET_CAPTURE_COMPLETION_SEM:*/
	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		write_sem=*((sem_id *)data);
		return B_OK;
	case SOUND_UNSAFE_WRITE: 
		header=(audio_buffer_header *)data;
		len=header[0].reserved_1-sizeof(header[0]);
		err=pcm_write(NULL, 0, (void *)&header[1], &len);	//	cookie
		acquire_sl();
		header[0].sample_clock=(bigtime_t)(write_clock * ((double)1000000.0 / (22050.0 * 2)));	//	bytes per frame
		header[0].time=write_time;		//	hplus bad time calculation
		release_sl();
		return release_sem(write_sem);
#if 0
	  case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
		if (data) *(int32*) data = 4096;
		else {
			dprintf("data is NULL\n");
		}
		return B_OK;
	  case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
		if (data) {
			if ((*(int32*)data == 4096) || (*(int32 *)data == 8192) || (*(int32*)data == 16384)) {
				//	each halfbuffer 4416 is 4096 means each halfbuffer 228 is 1024 means total is 2048
				write_buffer.size = (*(int32*)data)/2;	//	downsampling, times two buffers
			}
			else {
				return B_BAD_VALUE;
			}
		}
		else {
			dprintf("data is NULL\n");
			return B_BAD_VALUE;
		}
		return B_OK;
#endif
	}
	return EINVAL;
}

status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len) {
	return B_OK;
	}

status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len) {
	int togo = *len;	//	in bytes
	int wrtn = 0;		//	in bytes
	bool start = false;
	int ix;				//	in frames
	status_t err;
	static bool printed = false;

	if (!printed) {
		dprintf("pcm_write(size = %d)\n", (int)*len);
		dprintf("write_buffer.size = %ld\n", write_buffer.size);
		printed = true;
	}

	while (togo > 0) {
		uchar * dest;
		const int16 * idata = (const int16 *)((char *)data+wrtn);
		if (atomic_add(&write_buffer.pending, 1) == 0) {	//	enqueue ourselves
			start = true;
			write_buffer.halfcount = 0;
		}
		if ((err = acquire_sem_etc(write_buffer.int_sem, 1, B_TIMEOUT | B_CAN_INTERRUPT, 500000LL)) < B_OK) {
			dprintf("sbpro: acquire_sem(int_sem) timeout\n");
			acquire_sl();
			stop_dma();
			release_sl();
			release_sem_etc(write_buffer.int_sem, 2, B_DO_NOT_RESCHEDULE);
			break;
		}
		dest = write_buffer.data + (write_buffer.halfcount & 1)*(write_buffer.size/2) +
			write_buffer.written;
		if (cookie == NULL) {	//	called from ioctl()
			/*	downsample from 16/44 to 8/22	*/
			if (togo/8 < (write_buffer.size/2-write_buffer.written)/2) {
				ix = togo/8;
				memset(dest+ix*2, 0x80, (write_buffer.size/2-write_buffer.written)-(ix*2));
			}
			else {
				ix = (write_buffer.size/2-write_buffer.written)/2;
			}
			wrtn += ix*8;
	//		write_buffer.written += ix*2;
			while (ix-- > 0) {
				int samp_r, samp_l = (idata[ix*4]+idata[ix*4+2])>>1;
				if (samp_l < -32767) dest[ix*2] = 0;
				else if (samp_l > 32767) dest[ix*2] = 0xff;
				else dest[ix*2] = (samp_l>>8)^0x80;
				samp_r = (idata[ix*4+1]+idata[ix*4+3])>>1;
				if (samp_r < -32767) dest[ix*2] = 0;
				else if (samp_r > 32767) dest[ix*2] = 0xff;
				else dest[ix*2+1] = (samp_r>>8)^0x80;
			}
			togo -= (write_buffer.size/2-write_buffer.written)/2*8;	//	frames to bytes
		}
		else {		//	raw mode
			//	ix in bytes
			if (togo < write_buffer.size/2) {
				ix = togo;
				memset(dest+ix, 0x80, write_buffer.size/2-ix);
			}
			else {
				ix = write_buffer.size/2;
			}
			memcpy(dest, idata, ix);
			togo -= ix;
			wrtn += ix;
		}
//		if (write_buffer.written >= write_buffer.size/2) {
			atomic_add(&write_buffer.halfcount, 1);	//	write next half
			write_buffer.written = 0;
//		}
//		else {
//			atomic_add(&write_buffer.pending, -1);
//			release_sem_etc(write_buffer.int_sem, 1, B_DO_NOT_RESCHEDULE);
//			start = false;
//		}
		if (start) {
			start_dma();
		}
	}
	*len = wrtn;
	return wrtn < 1 ? err : B_OK;
}


/*support functions*/


static void
pcm_apply_params(sound_setup * sound)
{
	int value=0;
	if(!sound->left.aux1_mix_mute)
			value+=224-((sound->left.aux1_mix_gain<<3)&0xe0);
	if(!sound->right.aux1_mix_mute)
			value+=14-((sound->right.aux1_mix_gain>>1)&0x0e);
	write_reg(SBPRO_CD, value);
/*			aux2 maps to MIDI, 5->4*/
	value=0;
	if(!sound->left.aux2_mix_mute)
			value+=224-((sound->left.aux2_mix_gain<<3)&0xe0);
	if(!sound->right.aux2_mix_mute)
			value+=14-((sound->right.aux2_mix_gain>>1)&0x0e);
	write_reg(SBPRO_MIDI, value);
/*			set line levels, 5->4*/
	value=0;
	if(!sound->left.line_mix_mute)
			value+=224-((sound->left.line_mix_gain<<3)&0xe0);
	if(!sound->right.line_mix_mute)
			value+=14-((sound->right.line_mix_gain>>1)&0x0e);
	write_reg(SBPRO_LINE, value);
/*			dac maps to voice, 6->4*/
	value=0;
	if(!sound->left.dac_mute)
			value+=224-((sound->left.dac_attn<<2)&0xe0);
	if(!sound->right.dac_mute)
			value+=14-((sound->right.dac_attn>>2)&0x0e);
	write_reg(SBPRO_VOICE, value);
/*			only support 22.1kHz*/
/*			only support 8 bit stereo playback*/
/*			only support 8 bit stereo capture*/
/*			no control for dither_enable*/
/*			no control for loop_attn*/
/*			no control for loop back*/
/*			no control for output boosk*/
/*			no control for high pass filter*/
/*			no control for speaker gain*/
/*			un/mute speaker*/
/*			if(sound->mono_mute) hw_codec_write_byte(SB16_SPEAKER_DISABLE);*/
/*			else hw_codec_write_byte(SB16_SPEAKER_ENABLE);*/
	return;
}

void start_dma()
{
	physical_entry table[2];
	int ix;

	ix = get_memory_map(write_buffer.data, write_buffer.size, table, 2);
		dprintf("start_dma(0x%x, %d) (error %d)\n", (int)table[0].address, (int)table[0].size, ix);
	if (ix != 0) {
		return;
	}
	acquire_sl();	//	this should be done without preemption
	if (setup_pending) {
		pcm_apply_params(&sb_sound_setup);
		setup_pending = false;
	}
	dma_running = true;
	isa_info->start_isa_dma(dma8, table[0].address, table[0].size, 0x58, 0x34);
	if (write_time != 0) {
		bigtime_t st = system_time();
		write_clock += st-write_time;
		write_time = st;
	}
	else {
		write_clock = 0;
		write_time = system_time();
	}
	write_data(SBPRO_SPEAKER_ENABLE);
	write_data(SBPRO_TIME_CONSTANT);
	write_data(0xE9);
	write_reg(0xe, 0x22);
	write_data(SBPRO_TRANSFER_SIZE);
	write_data((write_buffer.size/2-1)&0xff);
	write_data((write_buffer.size/2-1)>>8);
	write_data(SBPRO_PLAYBACK_8_BIT);
	release_sl();
}

void stop_dma()
{
//static bool printed = false;
//	if (!printed) {
		dprintf("stop_dma()\n");
//		printed = true;
//	}
	dma_running = false;
	reset_hw();
	if (setup_pending) {
		pcm_apply_params(&sb_sound_setup);
		setup_pending = false;
	}
}

int32 pcm_write_inth(void * data)
{
//static bool printed = false;
	bigtime_t s_time = system_time();
	int32 that;

//	if (!printed) {
//		dprintf("pcm_write_inth()\n");
//		printed = true;
//	}
	acquire_sl();
	write_time = s_time;
	write_clock += write_buffer.size/2;	//	how many bytes transferred
//	ack interrupt
	(void)read_io(SBPRO_READ_BUFFER_STATUS);
//	check for DMA pause
	that = atomic_add(&write_buffer.pending, -1);
	if (that < 0) {
		//	turn off DMA
		stop_dma();
		memset(write_buffer.data, 0x80, write_buffer.size);
	}
	else if (that > 0) {
		release_sem_etc(write_buffer.int_sem, 1, B_DO_NOT_RESCHEDULE);
	}
	release_sl();
	return B_HANDLED_INTERRUPT;
}

status_t pcm_set_params(sound_setup *sound) {
/*			no controls for channel.adc_gain*/
/*			set channel.mic levels, 1bit->2bit*/
/*			if(sound->left.mic_gain_enable*/
/*					||sound->right.mic_gain_enable) {*/
/*				write_reg(SBPRO_MIC, 4);*/
/*				}*/
/*			else write_reg(SBPRO_MIC, 0);*/
/*			aux1 maps to CD-ROM, 5bit->4bit*/
	memcpy(&sb_sound_setup, &sound, sizeof(*sound));
	if (dma_running) {
		setup_pending = true;
		return B_OK;
	}
	acquire_sl();
	pcm_apply_params(sound);
	release_sl();
	return B_OK;
}


status_t pcm_get_params(sound_setup *sound) {
	int value, value2;

	if (dma_running) {
		memcpy(sound, &sb_sound_setup, sizeof(*sound));
		return B_OK;
	}

	acquire_sl();
/*	acquire_sl();*/
/*	release_sl();*/
/*return B_OK;*/
/*			no controls for channel.adc_gain*/
	sound->left.adc_gain=sound->right.adc_gain = 15;
/*			set channel.mic levels, 1bit->2bit*/
	sound->left.mic_gain_enable=0;
	sound->right.mic_gain_enable=0;
/*			if(read_reg(SBPRO_MIC)&6) {*/
/*				sound->left.mic_gain_enable=1;*/
/*				sound->right.mic_gain_enable=1;*/
/*				}*/
/*			else {*/
/*				sound->left.mic_gain_enable=0;*/
/*				sound->right.mic_gain_enable=0;*/
/*				}*/
/*			CD-ROM maps to aux1, 4bit->5bit, no mute control*/
	value=value2=read_reg(SBPRO_CD);
	value=30-((value>>3)&30);
	value2=30-((value2<<1)&30);
	sound->left.aux1_mix_gain=value;
	sound->right.aux1_mix_gain=value2;
	sound->left.aux1_mix_mute=sound->right.aux1_mix_mute=0;
/*			MIDI maps to aux2, 4->5*/
	value=value2=read_reg(SBPRO_MIDI);
	value=30-((value>>3)&30);
	value2=30-((value2<<1)&30);
	sound->left.aux2_mix_gain=value;
	sound->right.aux2_mix_gain=value2;
	sound->left.aux2_mix_mute=sound->right.aux2_mix_mute=0;
/*			set line levels, 4->5*/
	value=value2=read_reg(SBPRO_LINE);
	value=30-((value>>3)&30);
	value2=30-((value2<<1)&30);
	sound->left.line_mix_gain=value;
	sound->right.line_mix_gain=value2;
	sound->left.line_mix_mute=sound->right.line_mix_mute=0;
/*			dac maps to voice, 6->4*/
	value=value2=read_reg(SBPRO_LINE);
	value=56-((value>>2)&56);
	value2=56-((value2<<2)&56);
	sound->left.dac_attn=value;
	sound->right.dac_attn=value2;
	sound->left.dac_mute=sound->right.dac_mute=0;

/*			only support 22.05kHz*/
	sound->sample_rate = kHz_44_1;
/*			only support 8 bit stereo playback*/
	sound->playback_format = linear_16bit_big_endian_stereo;	//	big-endian == host-endian
/*			only support 8 bit stereo capture*/
	sound->capture_format = linear_8bit_unsigned_stereo;
/*			no control for dither_enable*/
	sound->dither_enable = 0;
/*			no control for loop_attn*/
	sound->loop_attn = 64;
/*			no control for loop back*/
	sound->loop_enable = 0;
/*			no control for output boost*/
	sound->output_boost = 0;
/*			no control for high pass filter*/
	sound->highpass_enable = 0;
/*			no control for speaker gain*/
	sound->mono_gain = 64;
/*			un/mute speaker*/
	sound->mono_mute = 0;
/*			if(sound->mono_mute) hw_codec_write_byte(SB16_SPEAKER_DISABLE);*/
/*			else hw_codec_write_byte(SB16_SPEAKER_ENABLE);*/
	release_sl();
	return B_OK;
}
