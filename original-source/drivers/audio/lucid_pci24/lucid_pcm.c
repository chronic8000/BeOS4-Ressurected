#include "lucid_pci24.h"
#include <string.h>
#include <byteorder.h>

int sprintf(char *, const char *, ...);

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

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
	NULL,
	NULL
};

/* This configuration struct is well-defined for /dev/audio devices */
static pcm_cfg default_pcm = {
	44100.0,	/* sample rate */
	2,			/* channels */
	0x2,		/* format */		
	0,			/* little endian */
	0,			/* header size */
	FIFO_DEPTH,	/* these are currently hard-coded */
	FIFO_DEPTH	/* and cannot be changed without re-compile */
};


/* utility function to apply changes to chip configuration for audio device */
static status_t 
configure_pcm(
	pcm_dev *pcm,
	pcm_cfg *config,
	bool force)
{
	status_t err = B_OK;
	uint32 *dsp = pcm->card->dsp;
	
	ddprintf("pci24: begin configure_pcm\n");

	if (config->sample_rate <= 32000.0) {
		config->sample_rate = 32000.0;
		// set DSP's transmit sample rate to 32k 
    	ddprintf("pci24: Setting 32k sample rate\n" );
    	send_host_cmd( kHstCmdXmt32, dsp );
    	wait_on_status(dsp);
    	send_host_cmd( kHstCmdRcv32, dsp );
    	wait_on_status(dsp);
	}
	if (config->sample_rate >= 48000.0) {
		config->sample_rate = 48000.0;
		// set DSP's transmit sample rate to 44k 
    	ddprintf("pci24: Setting 48k sample rate\n" );
    	send_host_cmd( kHstCmdXmt48, dsp );
    	wait_on_status(dsp);
    	send_host_cmd( kHstCmdRcv48, dsp );
    	wait_on_status(dsp);
	}
	if (config->sample_rate > 32000.0 &&
		config->sample_rate < 48000.0 &&
		config->sample_rate != 44100.0) {
		config->sample_rate = 44100.0;
		// set DSP's transmit sample rate to 48k 
    	ddprintf("pci24: Setting 44k sample rate\n" );
		send_host_cmd( kHstCmdXmt44, dsp );
    	wait_on_status(pcm->card->dsp);
    	send_host_cmd( kHstCmdRcv44, dsp );
    	wait_on_status(pcm->card->dsp);
	}
	pcm->config.sample_rate = config->sample_rate;

    // set SPDIF output
	ddprintf("pci24: set SPDIF out\n");
	send_host_cmd( kHstCmdXmtSPDIF, dsp );
    wait_on_status(dsp);
/*

	// disable playthrough
	ddprintf("pci24: disable playthrough\n");
	send_host_cmd( kHstCmdPlaythruDisable, dsp );
	wait_on_status(dsp);

	// enable asynch i/o
	ddprintf("pci24: enable asynch io\n");
	send_host_cmd( kHstCmdAsynch, dsp );
	wait_on_status(dsp);
*/
	if (config->channels != 2) {
		config->channels = 2;
	}
	pcm->config.channels = config->channels;
	
	/* secret format of format: upper nybble = signed, unsigned, float */
	/* lower nybble = bytes per sample */
	if (config->format != 0x2) {
		config->format = 0x2;
	}
	pcm->config.format = config->format;
	
	if (config->buf_header < 0) {
		config->buf_header = 0;
	}
	pcm->config.buf_header = config->buf_header;
	pcm->config.big_endian = 0;
	pcm->config.play_buf_size = FIFO_DEPTH;
	pcm->config.rec_buf_size = FIFO_DEPTH;
	
	ddprintf("pci24: end configure_pcm\n");
	return err;
}


/* open() called from user application */
static status_t
pcm_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	pcm_dev *pcm = NULL;
	char name_buf[256];
	int32 prev_mode;

	ddprintf("pci24: pcm_open()\n");
		
	/* find the relevant device */
	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].pcm.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		return ENODEV;
	}
	
	*cookie = pcm = &cards[ix].pcm;
	
	/* if this is the first open */
	if (atomic_add(&pcm->open_count, 1) == 0) {
		ddprintf("pci24: initializing pcm\n");
		pcm->open_mode = 0;
		
		/* initialize device first time */
		pcm->card = &cards[ix];

		sprintf(name_buf, "XX:%s", pcm->name);
		name_buf[B_OS_NAME_LENGTH-1] = 0;
		/* playback */
		pcm->write_waiting = 1;
		pcm->left_to_write = 0;
		pcm->wr_fifo_size = FIFO_DEPTH;
		pcm->wr_base = (int16 *)(pcm->card->buf + RD_BUF_SIZE);
		pcm->wr_int_ptr = pcm->wr_base;
		pcm->wr_thr_ptr = pcm->wr_base;
		name_buf[0] = 'W'; name_buf[1] = 'I';
		pcm->wr_int_sem = create_sem(0, name_buf);
		if (pcm->wr_int_sem < B_OK) {
			pcm->open_count = 0;
			return pcm->wr_int_sem;
		}
		set_sem_owner(pcm->wr_int_sem, B_SYSTEM_TEAM);
		name_buf[0] = 'W'; name_buf[1] = 'E';
		pcm->wr_entry_sem = create_sem(1, name_buf);
		if (pcm->wr_entry_sem < B_OK) {
			delete_sem(pcm->wr_int_sem);
			pcm->open_count = 0;
			return pcm->wr_entry_sem;
		}
		set_sem_owner(pcm->wr_entry_sem, B_SYSTEM_TEAM);
		
		/* recording */
		pcm->read_waiting = 1;
		pcm->first_read = 1;
		pcm->left_to_read = 0;
		pcm->rd_fifo_size = FIFO_DEPTH;
		pcm->rd_time = 0;
		pcm->next_rd_time = 0;
		pcm->rd_base = (int16 *)pcm->card->buf;
		pcm->rd_int_ptr = pcm->rd_base;
		pcm->rd_thr_ptr = pcm->rd_base;
		name_buf[0] = 'R'; name_buf[1] = 'I';
		pcm->rd_int_sem = create_sem(0, name_buf);
		if (pcm->rd_int_sem < B_OK) {
			delete_sem(pcm->wr_int_sem);
			delete_sem(pcm->wr_entry_sem);
			pcm->open_count = 0;
			return pcm->rd_int_sem;
		}
		set_sem_owner(pcm->rd_int_sem, B_SYSTEM_TEAM);
		name_buf[0] = 'R'; name_buf[1] = 'E';
		pcm->rd_entry_sem = create_sem(1, name_buf);
		if (pcm->rd_entry_sem < B_OK) {
			delete_sem(pcm->wr_int_sem);
			delete_sem(pcm->wr_entry_sem);
			delete_sem(pcm->rd_int_sem);
			pcm->open_count = 0;
			return pcm->rd_entry_sem;
		}

		/* configuration */
		acquire_spinlock(&cards[ix].hardware);
		configure_pcm(pcm, &default_pcm, true);
		release_spinlock(&cards[ix].hardware);
	}
	prev_mode = pcm->open_mode;
	if ((flags & 3) == O_RDONLY) {
		ddprintf("pci24: read only\n");
		atomic_or(&pcm->open_mode, kRecord);
	}
	else if ((flags & 3) == O_WRONLY) {
		ddprintf("pci24: write only\n");
		atomic_or(&pcm->open_mode, kPlayback);
	}
	else {
		ddprintf("pci24: read and write\n");
		atomic_or(&pcm->open_mode, kPlayback|kRecord);
	}
	
	/* install interrupt handler if not already installed */
	increment_interrupt_handler(pcm->card);

	/* enable card interrupts */
	if(pcm->open_mode == kPlayback || pcm->open_mode == 3){
		ddprintf("pci24: enabling write interrupts from card\n");
		acquire_spinlock(&cards[ix].hardware);
		send_host_cmd( kHstCmdXmitEnable, pcm->card->dsp );
    	wait_on_status( pcm->card->dsp );
		release_spinlock(&cards[ix].hardware);
	}
	if(pcm->open_mode == kRecord || pcm->open_mode == 3){
		ddprintf("pci24: enabling read interrupts from card\n");
		acquire_spinlock(&cards[ix].hardware);
    	send_host_cmd( kHstCmdRecvEnable, pcm->card->dsp );
    	wait_on_status( pcm->card->dsp );
		release_spinlock(&cards[ix].hardware);
	}
	return B_OK;
}

/* close() called -- outstanding read()s and write()s may exist */
static status_t
pcm_close(
	void * cookie)
{
	pcm_dev * pcm = (pcm_dev *)cookie;
	cpu_status cp;
	int spin = 0;

	ddprintf("pci24: pcm_close()\n");

	if (atomic_add(&pcm->open_count, -1) == 1) {

		cp = disable_interrupts();
		acquire_spinlock(&pcm->card->hardware);

		/* turn off interrupts */ 	
		send_host_cmd(kHstCmdXmitDisable, pcm->card->dsp);
    	wait_on_status(pcm->card->dsp);
    	send_host_cmd(kHstCmdRecvDisable, pcm->card->dsp);
    	wait_on_status(pcm->card->dsp);

		release_spinlock(&pcm->card->hardware);
		restore_interrupts(cp);
		
		delete_sem(pcm->wr_int_sem);
		delete_sem(pcm->rd_int_sem);
		delete_sem(pcm->wr_entry_sem);
		delete_sem(pcm->rd_entry_sem);

		spin = 1;
	}


	if (spin) {
		/* wait so we know FIFO gets filled with silence */
		snooze(FIFO_DEPTH*1000/(pcm->config.sample_rate*
			(pcm->config.format&0xf)*pcm->config.channels/1000));
	}
	return B_OK;
}

/* All outstanding I/O has completed after close() */
static status_t
pcm_free(
	void * cookie)
{
	pcm_dev * pcm = (pcm_dev *)cookie;

	ddprintf("pci24: pcm_free()\n");

	if (pcm->open_count == 0) {

		/* the last free will actually stop everything  */
		acquire_spinlock(&pcm->card->hardware);
	
		decrement_interrupt_handler(pcm->card);
	
		release_spinlock(&pcm->card->hardware);
	}

	return B_OK;
}


/* user ioctl() -- note that the config struct is well-defined! */
static status_t
pcm_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t unused)
{
	pcm_dev * pcm = (pcm_dev *)cookie;
	status_t err = B_BAD_VALUE;
	static float rates[4] = { 48000.0, 44100.0, 32000.0, 44100.0 };

	ddprintf("pci24: pcm_control()\n");
	switch (iop) {
	case B_AUDIO_GET_AUDIO_FORMAT:
		ddprintf("pci24: B_AUDIO_GET_AUDIO_FORMAT\n");
		memcpy(data, &pcm->config, sizeof(pcm->config));
		return B_OK;
	case B_AUDIO_GET_PREFERRED_SAMPLE_RATES:
		ddprintf("pci24: B_AUDIO_GET_PREFERRED_SAMPLE_RATES\n");
		memcpy(data, rates, sizeof(rates));
		return B_OK;
	case B_AUDIO_SET_AUDIO_FORMAT:
		ddprintf("pci24: B_AUDIO_SET_AUDIO_FORMAT\n");
		memcpy(&pcm->config, data, sizeof(pcm->config));
		err = B_OK;
		break;
	default:
		ddprintf("pci24: unknown code %d\n", iop);
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK) {
		cpu_status cp = disable_interrupts();
		acquire_spinlock(&pcm->card->hardware);
		err = configure_pcm(pcm, &pcm->config, false);
		release_spinlock(&pcm->card->hardware);
		restore_interrupts(cp);
	}

	return err;
}


static status_t
pcm_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	pcm_dev * pcm = (pcm_dev *)cookie;
	int32 hdrsize = pcm->config.buf_header;
	void * hdrptr = data;
	audio_buf_header hdr;
	status_t err;
	uint32 index = 0;
	uint32 half_rd_buf = RD_BUF_SIZE/2;
	uint32 end = 0;
	uint32 left = pcm->left_to_read;
	uint32 fifo = pcm->rd_fifo_size;
	uint32 requested = (*nread - hdrsize)>>1;
	uint32 togo = requested;
	int16 *output;

	//ddprintf("pci24: pcm_read()\n"); /* we're here */
	
	/* if this is the first read,
		set the read ptr to the interrupt ptr */
	if(atomic_and(&pcm->first_read, 0) == 1)
	{
		pcm->rd_thr_ptr = pcm->rd_int_ptr;
		ddprintf("pci24: first read\n");
	}

	*nread = 0;
	data = ((char *)data)+hdrsize;
	output = (int16 *)data;
	err = acquire_sem_etc(pcm->rd_entry_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}
		
	hdr.capture_time = pcm->rd_time;

	while(togo > 0)
	{
		if(left>0 && left<=togo)
		{
			ddprintf("pci24: left branch 1\n");
			end = left;
			left = 0;
			pcm->left_to_read = 0;
		}
		else if(left>0 && left>togo)
		{
			ddprintf("pci24: left branch 2\n");
			end = togo;
			atomic_and(&pcm->read_waiting, 0);
			pcm->left_to_read = left - togo;
		}
		else if(togo<fifo)
		{
			ddprintf("pci24: togo branch\n");
			end = togo;
			atomic_and(&pcm->read_waiting, 0);
			err = acquire_sem_etc(pcm->rd_int_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK)
			{
				release_sem(pcm->rd_entry_sem);
				return err;
			}
			pcm->left_to_read = fifo - togo;
		}
		else
		{
			end = fifo;
			atomic_and(&pcm->read_waiting, 0);
			err = acquire_sem_etc(pcm->rd_int_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK)
			{
				release_sem(pcm->rd_entry_sem);
				return err;
			}
		}
		togo -= end;
		
		//ddprintf("pci24: sem latency = %d\n", system_time()-pcm->next_rd_time);
		while(end > 0)
		{
			if(pcm->rd_thr_ptr + end < pcm->rd_base + half_rd_buf)
			{
				memcpy(output, pcm->rd_thr_ptr, end<<1);
				output += end;
				pcm->rd_thr_ptr += end;
				end = 0;
			}
			else
			{
				index = half_rd_buf - (pcm->rd_thr_ptr - pcm->rd_base);
				memcpy(output, pcm->rd_thr_ptr, index<<1);
				pcm->rd_thr_ptr = pcm->rd_base;
				output += index;
				end -= index;
			}
		}
		
#if 0
		index += end;
				
		/*copy the data from the area into the buffer*/
		for(index; index<end; index++)
		{
			*(output + index) = *pcm->rd_thr_ptr;
			pcm->rd_thr_ptr++;
			if(pcm->rd_thr_ptr - pcm->rd_base >= RD_BUF_SIZE/2)
			{
				pcm->rd_thr_ptr = pcm->rd_base;
				//ddprintf("pci24: cycling thr ptr\n");
			}
		}
#endif
	}
	
	*nread = 2*(requested - togo);
	
	/*	provide header if requested	*/
	if (hdrsize > 0) {
		ddprintf("header %d\n", hdrsize);
		*nread += hdrsize;
		hdr.capture_size = *nread;
		hdr.sample_rate = pcm->config.sample_rate;
		if (hdrsize > sizeof(hdr)) {
			hdrsize = sizeof(hdr);
		}
		memcpy(hdrptr, &hdr, hdrsize);
	}

	//ddprintf("pci24: %d samples, %d bytes read\n", requested-togo, *nread);

	release_sem(pcm->rd_entry_sem);
	return B_OK;
}


static status_t
pcm_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	pcm_dev * pcm = (pcm_dev *)cookie;
	status_t err = B_ERROR;
	uint32 index = 0;
	uint32 end = 0;
	uint32 half_wr_buf = WR_BUF_SIZE/2;
	uint32 left = pcm->left_to_write;	/* in 16bit samples! */
	uint32 fifo = pcm->wr_fifo_size;	/* in 16bit samples! */
	uint32 requested = (*nwritten)/2;
	uint32 togo = requested;
	int32 sample = 0;
	int16 * input = (int16 *)data;

#if 0
	ddprintf("pci24: pcm_write()\n"); /* we're here */
	ddprintf("pci24: wr_fifo_size = %d\n", fifo);
	ddprintf("pci24: requested = %d\n", requested);	
#endif

	*nwritten = 0;
	
	err = acquire_sem_etc(pcm->wr_entry_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		ddprintf("pci24: error acquiring semaphore\n");
		return err;
	}
		
	while(togo > 0)
	{
		//ddprintf("pci24: %d samples to go\n", togo);
		
		if(left>0 && left<=togo)
		{
			//ddprintf("pci24: left branch\n");
			end = left;
			left = 0;
			pcm->left_to_write = 0;
		}
		else if(left>0 && left>togo)
		{
			end = togo;
			atomic_and(&pcm->write_waiting, 0);
			pcm->left_to_write = left - togo;
		}
		else if(togo<fifo)
		{
			//ddprintf("pci24: togo branch\n");
			end = togo;
			atomic_and(&pcm->write_waiting, 0);
			err = acquire_sem_etc(pcm->wr_int_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK)
			{
				ddprintf("pci24: couldn't acquire wr_int_sem #1\n");
				release_sem(pcm->wr_entry_sem);
				return err;
			}
			pcm->left_to_write = fifo - togo;
		}
		else
		{
			//ddprintf("pci24: else branch\n");
			end = fifo;
			atomic_and(&pcm->write_waiting, 0);
			err = acquire_sem_etc(pcm->wr_int_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK)
			{
				ddprintf("pci24: couldn't acquire wr_int_sem #2\n");
				release_sem(pcm->wr_entry_sem);
				return err;
			}
		}
		
		togo -= end;
		end += index;
		
		while(end > 0)
		{
			if(pcm->wr_thr_ptr + end < pcm->wr_base + half_wr_buf)
			{
				memcpy(pcm->wr_thr_ptr, input, end<<1);
				input += end;
				pcm->wr_thr_ptr += end;
				end = 0;
			}
			else
			{
				index = half_wr_buf - (pcm->wr_thr_ptr - pcm->wr_base);
				memcpy(pcm->wr_thr_ptr, input, index<<1);
				pcm->wr_thr_ptr = pcm->wr_base;
				input += index;
				end -= index;
			}
		}
		
	}
	*nwritten = 2*(requested-togo);

	//ddprintf("pci24: %d samples, %d bytes written\n", requested-togo, *nwritten);
	
	release_sem(pcm->wr_entry_sem);
	return B_OK;
}
