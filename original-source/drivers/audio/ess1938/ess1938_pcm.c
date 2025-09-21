
#include "ess1938.h"
#include <string.h>
#include <byteorder.h>
#include <sound.h>
#include <R3MediaDefs.h>
#include <KernelExport.h>
#include <audio_driver.h>

extern int sprintf(char *, const char *, ...);

static status_t pcm_open(const char *name, uint32 flags, void **cookie);
static status_t pcm_close(void *cookie);
static status_t pcm_free(void *cookie);
static status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);
static status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks pcm_hooks = {
    &pcm_open,
    &pcm_close,
    &pcm_free,
    &pcm_control,
    &pcm_read,
    &pcm_write,
    NULL,			/* select */
    NULL,			/* deselect */
    NULL,			/* readv */
	NULL 			/* writev */
};

static sound_setup default_sound_setup = {
	{ mic, 0xa, 0, 7, 0, 7, 0, 7, 0, 15, 0},
	{ mic, 0xa, 0, 7, 0, 7, 0, 7, 0, 15, 0},
	kHz_44_1, /* not used */
	linear_16bit_little_endian_stereo, /* not used */
	linear_16bit_little_endian_stereo, /* not used */
	0,   /* dither enable     */
	15,  /* loop attn (mic)   */	
	0,   /* loop enable (mic) */
	0,   /* output boost      */
	0,   /* highpass enable   */
	0x6, /* mono gain         */
	0x6  /* mono mute    	  */
};
	
static pcm_cfg default_pcm = {
	44100.0,	/* sample rate */
	2,			/* channels */
	0x2,		/* format */		
#if B_HOST_IS_BENDIAN
	1,			/* endian (big) */
#else
	0,			/* endian (little) */
#endif
	0,			/* header size */
	PLAYBACK_BUF_SIZE,	/* these are currently hard-coded */
	RECORD_BUF_SIZE		/* and cannot be changed without re-compile */
};

static status_t 
configure_pcm(
	pcm_dev * port,
	pcm_cfg * configx,
	bool force)
{
	pcm_cfg config = *configx;
	status_t err = B_OK, err2;
	uint32 s;	/* size of buffer */

	ddprintf("ess1938: configure_pcm()\n");

	/* check args */
	if (config.sample_rate != default_pcm.sample_rate) {
		config.sample_rate = default_pcm.sample_rate;
		err = B_ERROR;
	}
	if (config.channels != default_pcm.channels) {
		config.channels = default_pcm.channels;
		err = B_ERROR;
	}
	/* secret format of format: upper nybble = signed, unsigned, float */
	/* lower nybble = bytes per sample */
	if ( config.format != default_pcm.format ) {
		config.format = default_pcm.format;
		err = B_ERROR;
	}
	if (config.buf_header < 0) {
		config.buf_header = default_pcm.buf_header;
	}

	/* figure out buffer size that's a power of two and within size limits */
	/* default should be 256 samples for a comfy 6 ms latency */
	/* minimum should be 32 samples for a more extreme 0.75ms latency */
	/* right now it's hardcoded to the default */
	if (!config.play_buf_size ) {
		config.play_buf_size = default_pcm.play_buf_size;
	}
	else if (	config.play_buf_size !=  default_pcm.play_buf_size ) {
		config.play_buf_size = default_pcm.play_buf_size;
		err = B_ERROR;
	}
	if (!config.rec_buf_size) {
		config.rec_buf_size = default_pcm.rec_buf_size;
	}
	else if (	config.rec_buf_size != default_pcm.rec_buf_size ) {
		config.rec_buf_size = default_pcm.rec_buf_size;
		err = B_ERROR;
	}

	
	err2 =  acquire_sem_etc(port->card->mixer_sem,1,B_CAN_INTERRUPT,0);
	if (err2 ==  B_NO_ERROR) {
		port->config = config;
		err2 = release_sem(port->card->mixer_sem);
	}
	if (err2 < B_NO_ERROR) {
		err = err2;
	}

	setup_dma_2(port->card);
	setup_dma_1(port->card);

	return err;
}


static status_t
pcm_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	pcm_dev * port = NULL;
	char name_buf[256];
	status_t status;
	cpu_status cp;

	ddprintf("ess1938: pcm_open()\n");

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].pcm.oldname)) {
			goto gotit;
		}
	}
	ddprintf("ess1938: %s NOT found\n", name);
	return ENODEV;

gotit:
	ddprintf("ess1938: found %s at 0x%.2x\n", name, ix);
	*cookie = port = &cards[ix].pcm;
	
	acquire_sem(port->card->open_close_sem);

	if (port->card->init_cnt++ == 0) { /* init_cnt protected by open_close sem */
#if 1
		reset_mixer(port->card);
		reset_ess1938(port->card);
		acquire_sem(port->card->controller_sem);
		set_controller(port->card, ENABLE_EXTENDED_CMDS);
		release_sem(port->card->controller_sem);
#endif
		configure_pcm(port, &default_pcm, true);
		memset(port->card->dma_buf, 0x0, port->config.play_buf_size * 2);
		
		port->card->dma_buf_ptr = port->card->dma_buf;
		port->card->dma_cap_buf_ptr = port->card->dma_cap_buf;
		
		increment_interrupt_handler( port->card );

		cp = disable_interrupts();
		acquire_spinlock(&port->card->spinlock);
	
		enable_card_interrupts(port->card);

		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);
		
		/* init volume */
		pcm_control(port, SOUND_SET_PARAMS, &default_sound_setup, 0);
 
		start_dma_2(port->card);
		start_dma_1(port->card);

		status = B_OK;
	}
	else {
		port->card->init_cnt--;
		status = EBUSY;
	}

	release_sem(port->card->open_close_sem);

	return status;
}


static status_t
pcm_close(
	void * cookie)
{
	cpu_status cp;
	pcm_dev * port = (pcm_dev *)cookie;

	ddprintf("ess1938: pcm_close()\n");
	acquire_sem(port->card->open_close_sem);
	
	if ( --(port->card->init_cnt) == 0) {
		/* stop ioctls */
		atomic_add(&port->card->closing,1);
	}
	release_sem(port->card->open_close_sem);
	return B_OK;
}


static status_t
pcm_free(
	void * cookie)
{
	cpu_status cp;
	pcm_dev * port = (pcm_dev *)cookie;

	ddprintf("ess1938: pcm_free()\n");
	acquire_sem(port->card->open_close_sem);
	if ( port->card->init_cnt == 0) {
		stop_dma_1(port->card);
		stop_dma_2(port->card);
		ddprintf("ess1938: pcm_free() dma stopped\n");

		cp = disable_interrupts();
		acquire_spinlock(&port->card->spinlock);

		/* release any waiting semaphores */
		if (atomic_add(&port->card->write_waiting,-1) <= 0 ) {
			/* no write waiting */
			atomic_add(&port->card->write_waiting, 1);
		}
		else {
			/* somebody's blocked */
			ddprintf("ess1938: release write_sem in close\n");
			release_sem_etc(port->card->write_sem, 1, B_DO_NOT_RESCHEDULE);
		}			
		if (atomic_add(&port->card->read_waiting,-1) <= 0 ) {
			/* no read waiting */
			atomic_add(&port->card->read_waiting, 1);
		}
		else {
			/* somebody's blocked */
			ddprintf("ess1938: release read_sem in close\n");
			release_sem_etc(port->card->read_sem, 1, B_DO_NOT_RESCHEDULE);
		}			

		disable_card_interrupts(port->card);

		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);

		decrement_interrupt_handler(port->card);
		
		/* we are now closed */
		atomic_add(&port->card->closing,-1);
	}
	release_sem(port->card->open_close_sem);
	return B_OK;
}


static status_t
pcm_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	pcm_dev * port = (pcm_dev *)cookie;
	static float rates[1] =  { 44100.0 };
	pcm_cfg config;
	status_t err = B_BAD_VALUE;
	bool configure = false;

	if ( port->card->closing) {
		snooze(1000);
		return B_ERROR;
	}	

	switch (iop) {
	case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
		err = B_ERROR;
		break;
	case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
		err = B_ERROR;
		break;
	case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
		err =  acquire_sem_etc(port->card->mixer_sem,1,B_CAN_INTERRUPT,0);
		if (err ==  B_NO_ERROR) {
			*((size_t *)(data)) = port->config.rec_buf_size;
			err = release_sem(port->card->mixer_sem);
		}
		break;
	case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
		err =  acquire_sem_etc(port->card->mixer_sem,1,B_CAN_INTERRUPT,0);
		if (err ==  B_NO_ERROR) {
			*((size_t *)(data)) = port->config.play_buf_size;
			err = release_sem(port->card->mixer_sem);
		}
		break;

	case B_AUDIO_GET_AUDIO_FORMAT:
		err =  acquire_sem_etc(port->card->mixer_sem,1,B_CAN_INTERRUPT,0);
		if (err ==  B_NO_ERROR) {
			memcpy(data, &port->config, sizeof(port->config));
			err = release_sem(port->card->mixer_sem);
		}
		break;
	case B_AUDIO_GET_PREFERRED_SAMPLE_RATES:
		memcpy(data, rates, sizeof(rates));
		err = B_OK;
		break;
	case B_AUDIO_SET_AUDIO_FORMAT:
		memcpy(&config, data, sizeof(config));
		configure = true;
		break;
	case SOUND_GET_PARAMS:
		{ 
		sound_setup * sound = (sound_setup *)data;
		uchar u;
		
		cpu_status cp = disable_interrupts();
		acquire_spinlock(&(port->card->spinlock));

		sound->sample_rate = kHz_44_1;
		if (!port->config.big_endian == !B_HOST_IS_BENDIAN) {
			sound->playback_format = linear_16bit_big_endian_stereo;
			sound->capture_format = linear_16bit_big_endian_stereo;
		}
		else {
			sound->playback_format = linear_16bit_little_endian_stereo;
			sound->capture_format = linear_16bit_little_endian_stereo;
		}
		sound->dither_enable = false;
		sound->loop_attn = 0;
		sound->loop_enable = 0;
		sound->output_boost = 0;
		sound->highpass_enable = 0;
		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MONO_IN_PB_VOL);
		sound->mono_gain = u & 0xf;
		/* No pan--synch left with right			*/
		sound->mono_mute = !(u & 0xf);
		/* ADC Source */
		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							ADC_SOURCE);
		switch (u&7) {
		case LINE:
			sound->left.adc_source = line;
			sound->right.adc_source = line;
			break;
		case AUXA:
			sound->left.adc_source = aux1;
			sound->right.adc_source = aux1;
			break;
		case MIC:
		case MIC_X:
			sound->left.adc_source = mic;
			sound->right.adc_source = mic;
			break;
		default:
			ddprintf("ess1938: error (not supported)\n");
			/* default to mic */
			sound->left.adc_source = mic;
			sound->right.adc_source = mic;
			break;
		}

		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MIC_MONO_MISC);
		
		sound->left.mic_gain_enable = u & 0x8;
		sound->right.mic_gain_enable = sound->left.mic_gain_enable;

		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MIC_PB_VOL);
		port->mic_mute.right = port->mic_mute.left;
		sound->loop_enable = !port->mic_mute.left;
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		if (port->mic_mute.left) {
			sound->loop_attn  = port->mic_mute.left;
		}
		else {
			sound->loop_attn = 63 - ((u&0xf) << 2  | u&0x3);;
		}

		
		
		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUXA_PB_VOL);
		sound->left.aux1_mix_mute = port->aux1_mute.left;		/* virtual mutes */
		sound->right.aux1_mix_mute = port->aux1_mute.right;		/* virtual mutes */
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		if (port->aux1_mute.left) {
			sound->left.aux1_mix_gain = port->aux1_mute.left;
		}
		else {
			sound->left.aux1_mix_gain =  31 - ((u&0xf0) >> 3 | ((u&0x10)>>4));
		}
		if (port->aux1_mute.right) {
			sound->right.aux1_mix_gain = port->aux1_mute.right;
		}
		else {
			sound->right.aux1_mix_gain = 31 - ((u&0xf) << 1 | u&0x1);
		}

		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUXB_PB_VOL);
		sound->left.aux2_mix_mute = port->aux2_mute.left;		/* virtual mutes */
		sound->right.aux2_mix_mute = port->aux2_mute.right;		/* virtual mutes */
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		if (port->aux2_mute.left) {
			sound->left.aux2_mix_gain = port->aux2_mute.left;
		}
		else {
			sound->left.aux2_mix_gain =  31 - ((u&0xf0) >> 3 | ((u&0x10)>>4));
		}
		if (port->aux2_mute.right) {
			sound->right.aux2_mix_gain = port->aux2_mute.right;
		}
		else {
			sound->right.aux2_mix_gain = 31 - ((u&0xf) << 1 | u&0x1);
		}

		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							LINE_PB_VOL);
		sound->left.line_mix_mute = port->line_mute.left;		/* virtual mutes */
		sound->right.line_mix_mute = port->line_mute.right;		/* virtual mutes */
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		if (port->line_mute.left) {
			sound->left.line_mix_gain = port->line_mute.left;
		}
		else {
			sound->left.line_mix_gain =  31 - ((u&0xf0) >> 3 | ((u&0x10)>>4));
		}
		if (port->line_mute.right) {
			sound->right.line_mix_gain = port->line_mute.right;
		}
		else {
			sound->right.line_mix_gain = 31 - ((u&0xf) << 1 | u&0x1);
		}

		u = get_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUDIO_2_PB_VOL);
		sound->left.dac_mute = port->dac_mute.left;		/* virtual mutes */
		sound->right.dac_mute = port->dac_mute.right;	/* virtual mutes */
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		if (port->dac_mute.left) {
			sound->left.dac_attn = port->dac_mute.left;
		}
		else {
			sound->left.dac_attn =  63 - ((u&0xf0) >> 2 | ((u&0x30)>>4)); /* volume creep bug! */
		}
		if (port->dac_mute.right) {
			sound->right.dac_attn = port->dac_mute.right;
		}
		else {
			sound->right.dac_attn = 63 - ((u&0xf) << 2  | u&0x3); /* volume creep bug! */
		}

		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);

		acquire_sem(port->card->controller_sem);
		set_controller(port->card, READ_CONTROL_REG);
		set_controller(port->card, RECORD_LEVEL);
		u=get_controller(port->card);
		sound->left.adc_gain = (u&0xf0)>>4;
		sound->right.adc_gain = (u&0xf);
		release_sem(port->card->controller_sem);
		
        }
		err = B_OK;
		break;
	case SOUND_SET_PARAMS:
		{ 
		sound_setup * sound = (sound_setup *)data;
		uchar u;
		
		cpu_status cp = disable_interrupts();
		acquire_spinlock(&(port->card->spinlock));
		
		if (sound->mono_mute) {
			u = 0x00;
		}
		else {
			u =	((sound->mono_gain & 0xf ) << 4) | (sound->mono_gain & 0xf);
		}
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MONO_IN_PB_VOL,
							u,
							0xff );
		switch (sound->right.adc_source) {
			case line:
				u = LINE;
				break;
			case mic:
			case loopback:	
				u = MIC;
				break;
			case aux1:
				u = AUXA;
				break;
			default:
				u = MIXER;
				break;
		}

		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							ADC_SOURCE,
							u,
							0x07);
		if (sound->right.mic_gain_enable) {					
			u = 0x08;
		}	
		else {
			u = 0x00;
		}	
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MIC_MONO_MISC,
							u,
							0x08);

		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		port->mic_mute.left = !sound->loop_enable ? sound->loop_attn : 0; 
		port->mic_mute.right = port->mic_mute.left;
		if (!sound->loop_enable ) {
			u = 0x00;
		}
		else {
			u = ((63-sound->loop_attn) & 0x3c ) << 2;
			u = u | (((63-sound->loop_attn) & 0x3c ) >> 2);
		}
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							MIC_PB_VOL,
							u,
							0xff);
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		port->aux1_mute.left = sound->left.aux1_mix_mute ? sound->left.aux1_mix_gain : 0; 
		port->aux1_mute.right = sound->right.aux1_mix_mute ? sound->right.aux1_mix_gain : 0; 
		if (sound->left.aux1_mix_mute ) {
			u = 0x00;
		}
		else {
			u =	((31 - sound->left.aux1_mix_gain) & 0x1e ) << 3;
		}
		if (! sound->right.aux1_mix_mute ) {
			u =  u | ((31 - sound->right.aux1_mix_gain) & 0x1f ) >> 1;
		}
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUXA_PB_VOL,
							u,
							0xff);
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		port->aux2_mute.left = sound->left.aux2_mix_mute ? sound->left.aux2_mix_gain : 0; 
		port->aux2_mute.right = sound->right.aux2_mix_mute ? sound->right.aux2_mix_gain : 0; 
		if (sound->left.aux2_mix_mute ) {
			u = 0x00;
		}
		else {
			u =	((31 - sound->left.aux2_mix_gain) & 0x1e ) << 3;
		}
		if (! sound->right.aux2_mix_mute ) {
			u =  u | ((31 - sound->right.aux2_mix_gain) & 0x1f ) >> 1;
		}
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUXB_PB_VOL,
							u,
							0xff);
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		port->line_mute.left = sound->left.line_mix_mute ? sound->left.line_mix_gain : 0; 
		port->line_mute.right = sound->right.line_mix_mute ? sound->right.line_mix_gain : 0; 
		if (sound->left.line_mix_mute ) {
			u = 0x00;
		}
		else {
			u =	((31 - sound->left.line_mix_gain) & 0x1e ) << 3;
		}
		if (! sound->right.line_mix_mute ) {
			u =  u | ((31 - sound->right.line_mix_gain) & 0x1f ) >> 1;
		}
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							LINE_PB_VOL,
							u,
							0xff);
		/* save the gain if we are going to mute the channel, retrieve it when asked */
		/* this is necessary only because we don't have a hardware mute bit */ 	
		port->dac_mute.left = sound->left.dac_mute ? sound->left.dac_attn : 0; 
		port->dac_mute.right = sound->right.dac_mute ? sound->right.dac_attn : 0; 
		if (sound->left.dac_mute ) {
			u = 0x00;
		}
		else {
			u = ((63-sound->left.dac_attn) & 0x3c ) << 2;
		}
		if (! sound->right.dac_mute ) {
			u = u | (((63-sound->right.dac_attn) & 0x3c ) >> 2);
		}

		set_indirect(	port->card->sbbase + MIXER_COMMAND,
							port->card->sbbase + MIXER_DATA,
							AUDIO_2_PB_VOL,
							u,
							0xff);
		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);

		acquire_sem(port->card->controller_sem);
		u = (sound->left.adc_gain & 0xf)| ((sound->right.adc_gain & 0xf) << 4 );
		set_controller(port->card, RECORD_LEVEL); /* NO MASK! */
		set_controller(port->card, u);
		release_sem(port->card->controller_sem);
		
        }
		err = B_OK;
		break;
	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		port->card->old_play_sem = *(sem_id *)data;
		err = B_OK;
		break;
	case SOUND_SET_CAPTURE_COMPLETION_SEM:
		port->card->old_cap_sem = *(sem_id *)data;
		err = B_OK;
		break;
	case SOUND_UNSAFE_WRITE: {
 		uchar u;
		audio_buffer_header * buf = (audio_buffer_header *)data;
		size_t n = buf->reserved_1-sizeof(*buf);

		pcm_write(cookie, 0, buf+1, &n);
		buf->time = port->card->write_time;
		buf->sample_clock = port->card->write_total/4 * 10000 / 441;
		err = release_sem(port->card->old_play_sem);
		
		} break;
	case SOUND_UNSAFE_READ: {

		audio_buffer_header * buf = (audio_buffer_header *)data;
		size_t n = buf->reserved_1-sizeof(*buf);
		pcm_read(cookie, 0, buf+1, &n);
		buf->time = port->card->read_time;
		buf->sample_clock = port->card->read_total/4 * 10000 / 441; 
		err = release_sem(port->card->old_cap_sem);
		} break;
	case SOUND_LOCK_FOR_DMA:
		err = B_OK;
		break;

	default:
		ddprintf("ess1938: unknown code %d\n", iop);
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK && configure == TRUE)
	{
		err = configure_pcm(port, &config, FALSE);
	}
	return err;
}

static void
swap_copy(
	short * dest,
	const short * src,
	int c)
{
	while (c > 1) {
		unsigned short sh = *(src++);
		*(dest++) = ((sh << 8) | (sh >> 8));
		c -= 2;
	}
}

static void
swap_copy_bytes(
	short * dest,
	const uchar * src,
	int c)
{
	while (c > 1) {
		uchar uc = *(src++);
		unsigned short sh = (unsigned short) (*(src++));
		*(dest++) = (short)((sh << 8) | uc);
		c -= 2;
	}
}

static status_t
pcm_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	pcm_dev * port 	= (pcm_dev *)cookie;
	uint32 cnt		= *nread;
	char * bug_fix  = (char * ) data;

	if (acquire_sem(port->card->pcmread_sem) >= B_OK)
	{
		while (cnt >= port->config.rec_buf_size)
		{
			atomic_add(&port->card->read_waiting,1);
			acquire_sem_etc(port->card->read_sem, 1, B_CAN_INTERRUPT, 0);
	
#if B_HOST_IS_LENDIAN
			bug_fix[0] = port->card->low_byte_l;
			bug_fix[1] = port->card->high_byte_l;
			bug_fix[2] = port->card->low_byte_r;
			memcpy(bug_fix+ 3, port->card->dma_cap_buf_ptr, port->config.rec_buf_size - 3);
#else
			//UNTESTED (capture not working on PPC.....)
			bug_fix[0] = port->card->high_byte_l;
			bug_fix[1] = port->card->low_byte_l;
			bug_fix[2] = port->card->dma_cap_buf_ptr[0];
			bug_fix[3] = port->card->low_byte_r;

			swap_copy_bytes(	(short *)(bug_fix+4),				
						(const uchar *)(port->card->dma_cap_buf_ptr + 1),
						port->config.rec_buf_size - 4);
#endif
			port->card->low_byte_r = port->card->dma_cap_buf_ptr[port->config.rec_buf_size-1];
			port->card->high_byte_l = port->card->dma_cap_buf_ptr[port->config.rec_buf_size-2];
			port->card->low_byte_l = port->card->dma_cap_buf_ptr[port->config.rec_buf_size-3];
			cnt -= port->config.rec_buf_size;
			bug_fix += port->config.rec_buf_size;
		}

		release_sem(port->card->pcmread_sem);
	}
	*nread -= cnt;
	return B_OK;
	
}


static status_t
pcm_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	pcm_dev * port 	= (pcm_dev *)cookie;
	int16* samples	= (int16 *) data;
	uint32 cnt		= *nwritten;
	if (acquire_sem_etc(port->card->pcmwrite_sem, 1, B_CAN_INTERRUPT, 0) >= B_OK)
	{
		while (cnt >= port->config.play_buf_size)
		{
			atomic_add(&port->card->write_waiting,1);
			acquire_sem_etc(port->card->write_sem, 1, B_CAN_INTERRUPT, 0);
#if BYTE_ORDER == LITTLE_ENDIAN		
			memcpy(port->card->dma_buf_ptr, samples, port->config.play_buf_size);
#else
			swap_copy(	(short *)(port->card->dma_buf_ptr),
						(const short *)samples,
						port->config.play_buf_size);
#endif
			cnt -= port->config.play_buf_size;
			samples += (port->config.play_buf_size/sizeof(int16));
		}
		release_sem(port->card->pcmwrite_sem);
	}
	*nwritten -= cnt;
	return B_OK;
	
}

