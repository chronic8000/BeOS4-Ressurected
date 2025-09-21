/* Copyright 2001, Be Incorporated. All Rights Reserved */
/* includes */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

#include "envy24.h"

/* globals */
extern envy24_dev cards[MAX_ENVY24_BD];
extern pci_module_info	*pci;
extern int card_count;

/* explicit declarations for the compiler */
status_t get_envy24_memory( envy24_dev *, uint32, uint32);
inline uint8   PCI_IO_RD (int);
inline uint16  PCI_IO_RD_16 (int);
inline uint32  PCI_IO_RD_32 (int);
inline void    PCI_IO_WR (int, uint8);
inline void    PCI_IO_WR_16 (int, uint16);
inline void    PCI_IO_WR_32(int, uint32);
int envy24_codec_set_sample_rate( envy24_dev *, bool, uint32);      
status_t envy24_validate_sample_rate(multi_format_info *);
uint32 envy24_convert(uint32);

/* the functions... */

status_t
envy24_multi_get_description(
							void * cookie,
							void * data,
							size_t len)
{
	open_envy24 * card = (open_envy24 *)cookie;
	multi_description * pmd = (multi_description *) data;
	int channel, i;

	(void)len;	// unused

	if (pmd == NULL || pmd->info_size < sizeof(multi_description)) {
		return B_BAD_VALUE;
	}	
	if(card->device->ix) {
		sprintf(pmd->friendly_name, "Envy24 %0lu", card->device->ix + 1);
	}
	else {
		sprintf(pmd->friendly_name, "Envy24");
	}
	strcpy(pmd->vendor_info,"IC Ensemble");
	pmd->info_size = sizeof(multi_description);
	pmd->interface_version = B_MINIMUM_INTERFACE_VERSION;
	pmd->interface_minimum = B_MINIMUM_INTERFACE_VERSION;
	pmd->input_channel_count  = DMAINS_PER_BD;
	pmd->output_channel_count = DMAOUTS_PER_BD;
	pmd->input_bus_channel_count  = INPUTS_PER_BD;
	pmd->output_bus_channel_count = OUTPUTS_PER_BD + MONITORS_PER_BD;
	pmd->aux_bus_channel_count = AUXS_PER_BD;	//hmmmmmmmm.
	channel = 0;
	for (i = 0; i < DMAOUTS_PER_BD; i++) {
		ddprintf(("envy24: request_channel_count is %ld, channel is %d\n",
			pmd->request_channel_count, channel));
		if (channel >= pmd->request_channel_count) {
			break;
		}
		pmd->channels[channel].channel_id =	channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_CHANNEL;
		pmd->channels[channel].designations	= 0;
		pmd->channels[channel].connectors = 0;
		channel++;											
	}
	for (i = 0; i < DMAINS_PER_BD; i++)
	{
		if (channel >= pmd->request_channel_count) {
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_INPUT_CHANNEL;
		pmd->channels[channel].designations = 0;
		pmd->channels[channel].connectors = 0;
		channel++;
	}	
	for (i = 0; i < OUTPUTS_PER_BD; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf(("envy24: request_channel_count killed output busses \n"));
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_BUS;
		pmd->channels[channel].designations = i < 8 ? B_CHANNEL_MONO_BUS : B_CHANNEL_STEREO_BUS | (i & 0x1 ? B_CHANNEL_RIGHT : B_CHANNEL_LEFT);
		pmd->channels[channel].connectors = i < 8 ? B_CHANNEL_QUARTER_INCH_MONO : B_CHANNEL_COAX_SPDIF;
		channel++;
	}	
	for (i = 0; i < MONITORS_PER_BD; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf(("envy24: request_channel_count killed monitor setup \n"));
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_BUS;
		pmd->channels[channel].designations = B_CHANNEL_STEREO_BUS | (i & 0x1 ? B_CHANNEL_RIGHT : B_CHANNEL_LEFT);
		pmd->channels[channel].connectors = B_CHANNEL_MINI_JACK_STEREO;
		channel++;
	}	
	for (i = 0; i < INPUTS_PER_BD; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf(("envy24: request_channel_count killed input busses \n"));
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_INPUT_BUS;
		pmd->channels[channel].designations = i < 8 ? B_CHANNEL_MONO_BUS : B_CHANNEL_STEREO_BUS | (i & 0x1 ? B_CHANNEL_RIGHT : B_CHANNEL_LEFT);
		pmd->channels[channel].connectors = i < 8 ? B_CHANNEL_QUARTER_INCH_MONO : B_CHANNEL_COAX_SPDIF;
		channel++;
	}	
	for (i = 0; i < AUXS_PER_BD; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf(("envy24: request_channel_count killed aux busses \n"));
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_AUX_BUS;
		pmd->channels[channel].designations = B_CHANNEL_STEREO_BUS | (i & 0x1 ? B_CHANNEL_RIGHT : B_CHANNEL_LEFT);
		pmd->channels[channel].connectors = 0;
		channel++;
	}	
	pmd->input_rates  = B_SR_48000 |
    			        B_SR_24000 |
			            B_SR_12000 |
/* Not supported by api?!			            B_SR_9600  |  */
			            B_SR_32000 |
			            B_SR_16000 |
			            B_SR_8000  | 
			            B_SR_44100 |
			            B_SR_22050 |
			            B_SR_11025 |
			            B_SR_96000 |
			            B_SR_88200 |
						B_SR_IS_GLOBAL;
	pmd->min_cvsr_rate = 0;
	pmd->max_cvsr_rate = 0;
	pmd->output_rates = pmd->input_rates | B_SR_SAME_AS_INPUT ;
	/* the api only supports LEFT justified... */
	pmd->input_formats = B_FMT_24BIT | B_FMT_IS_GLOBAL;
	pmd->output_formats = pmd->input_formats | B_FMT_SAME_AS_INPUT;
	pmd->lock_sources = B_MULTI_LOCK_INTERNAL |
						B_MULTI_LOCK_SPDIF |
						B_MULTI_LOCK_WORDCLOCK;
	pmd->timecode_sources = 0;									  

	/* I _think_ it clicks.... */
	pmd->interface_flags =	B_MULTI_INTERFACE_PLAYBACK | 
							B_MULTI_INTERFACE_RECORD |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_OUTPUTS |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_INPUTS |
							B_MULTI_INTERFACE_TIMECODE;
	/* let's do this later... */
	//FIX ME!!!!! 
	pmd->control_panel[0] = 0;

	return B_OK;
							
}

status_t
envy24_multi_get_enabled_channels(
							void * cookie,
							void * data,
							size_t len)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_channel_enable * pmce = (multi_channel_enable *) data;
	
	(void)len;	// unused

	if ( pmce != NULL && pmce->info_size >= sizeof(multi_channel_enable)) {
		pmce->lock_source     = cd->mce.lock_source;
		pmce->lock_data       = cd->mce.lock_data;
		pmce->timecode_source = cd->mce.timecode_source;
		pmce->connectors      = NULL;
		*(uint32 *)pmce->enable_bits = (uint32) (cd->mce.enable_bits);
		return B_OK;
	}
	else {
		ddprintf(("envy24: get_enabled_channels NULL ptr or bad size %p \n",pmce));
		return B_BAD_VALUE;
	}		
}

status_t
envy24_multi_set_enabled_channels(
							void * cookie,
							void * data,
							size_t len)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_channel_enable * pmce = (multi_channel_enable *) data;
	int32 chan;
	uint32 mask;
	status_t err = B_OK;

	(void)len;	// unused

	ddprintf(("envy24: set_enabled_channels \n"));

	if (pmce == NULL || pmce->enable_bits == NULL || pmce->info_size < sizeof(multi_channel_enable)) {
		ddprintf(("envy24: NULL pointer or info_size wrong\n"));
		return B_BAD_VALUE;
	}

	/* do the locks */
	switch (pmce->lock_source) {
		case B_MULTI_LOCK_SPDIF:
			break;
		case B_MULTI_LOCK_INTERNAL:
			break;
		case B_MULTI_LOCK_INPUT_CHANNEL:
		case B_MULTI_LOCK_WORDCLOCK:
		case B_MULTI_LOCK_LIGHTPIPE:
		default:
			ddprintf(("envy24: Unsupported synch source (0x%lx)\n", pmce->lock_source));
			err = B_BAD_VALUE;
			break;
	}

	if (err != B_OK) {
		return err;
	}


	/* do the clocks */
	switch (pmce->timecode_source) {
	
		case B_MULTI_NO_TIMECODE:
			break;
		case B_MULTI_TIMECODE_30:			/* MIDI */
		case B_MULTI_TIMECODE_30_DROP_2:	/* NTSC */
		case B_MULTI_TIMECODE_30_DROP_4:	/* Brazil */
		case B_MULTI_TIMECODE_25:			/* PAL */
		case B_MULTI_TIMECODE_24:			/* Film */
	
		default:
			ddprintf(("envy24: Unsupported time source (0x%lx)\n", pmce->timecode_source));
			err = B_BAD_VALUE;
			break;
	}

	if (err != B_OK) {
		return err;
	}

	//FIX ME!!!!! (need mask for request_channel_count and/or error return)
	//	if (changed_channels & invalid_channel_mask) {
	//		return B_BAD_VALUE;
	//	}	
	
	/* all values are now good, copy and set */
	cd->mce = * (pmce);
	cd->mce.enable_bits = (char *)(* (uint32 *)(pmce->enable_bits));

	err = envy24_codec_set_sample_rate(	cd,
										(cd->mce.lock_source == B_MULTI_LOCK_INTERNAL),
   										cd->sample_rate );
	ddprintf(("envy24_codec_set_sample_rate() returned %08lx\n", err));

	// for now, enable them all....
	{ cpu_status cp;
		cp = disable_interrupts();
		acquire_spinlock(&cd->spinlock);
			PCI_IO_WR(cd->mt_iobase + 0x30, 0);
			PCI_IO_WR(cd->mt_iobase + 0x31, 0);
		release_spinlock(&cd->spinlock);
		restore_interrupts(cp);
	}
	//enable/disable accordingly....(in the future)
	for (chan = 0, mask = 1; chan < OUTPUTS_PER_BD ; chan++, mask <<= 1) {
		uint32 enabled = (uint32)cd->mce.enable_bits;
		if (mask & enabled) {
			
		}
		else {
		
		}
	}

	ddprintf(("envy24: (set) enabled channels are 0x%lx lock source 0x%lx (internal %x)\n", (uint32) cd->mce.enable_bits, cd->mce.lock_source, B_MULTI_LOCK_INTERNAL));	

	return B_OK;
}

status_t
envy24_multi_set_global_format(
							void * cookie,
							void * data,
							size_t len)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_format_info * pmfi = (multi_format_info *) data;
	status_t err = B_OK;

	(void)len;	// unused

	ddprintf(("envy24: set_global_format sr = %ld \n", pmfi->output.rate));

	if (pmfi == NULL || pmfi->info_size < sizeof(multi_format_info) ){
		ddprintf(("envy24: set_global_format null ptr, wrong struct size\n"));
		return B_BAD_VALUE;
	}	
	
	/* check supported sample rates here.... */
	err = envy24_validate_sample_rate(pmfi);
 
	/* we are ignoring timecode for now */
	if (err == B_OK) {
		cd->sample_rate = envy24_convert(pmfi->output.rate);
		
		err = envy24_codec_set_sample_rate(	cd,
											(cd->mce.lock_source == B_MULTI_LOCK_INTERNAL),
    										cd->sample_rate );

		if ( cd->current_mask & B_MULTI_EVENT_CHANNEL_FORMAT_CHANGED) {
			if ( cd->event_count >= MAX_EVENTS){
				ddprintf(("envy24: out of event space\n"));
			}
			else {
				cd->event_queue[ cd->event_count].event = B_MULTI_EVENT_CHANNEL_FORMAT_CHANGED;
				cd->event_queue[ cd->event_count].info_size = sizeof(multi_get_event);
				cd->event_queue[ cd->event_count].count = 0;
				cd->event_queue[ cd->event_count].timestamp = system_time();
				/* bump event_count */
				cd->event_count++;
				/* release sem */
				if (cd->event_sem >= B_OK) {
					release_sem(cd->event_sem);
				}		
			}	
		}
	}	

	return err;
}

status_t
envy24_multi_get_global_format(
							void * cookie,
							void * data,
							size_t len)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_format_info * pmfi = (multi_format_info *) data;
	status_t err = B_OK;
	
	(void)len;	// unused

	ddprintf(("envy24: get_global_format FIX ME\n"));
	// set the codecs here.....	
	if ( pmfi != NULL && pmfi->info_size >= sizeof(multi_format_info)) {
		//FIX ME!!!! (latency not right)
		pmfi->output_latency= 1000; // FIX THIS!!!!
		pmfi->input_latency = 1000; // FIX THIS!!!!
		pmfi->timecode_kind = cd->mce.timecode_source;
		pmfi->output.rate= envy24_convert(cd->sample_rate);
		pmfi->input.rate = pmfi->output.rate;
		pmfi->output.cvsr= 0;
		pmfi->input.cvsr = 0;
		pmfi->output.format= B_FMT_24BIT;
		pmfi->input.format = pmfi->output.format;
	}
	else {
		err = B_BAD_VALUE;
	}	
	return err;
}

status_t
envy24_multi_get_buffers(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_buffer_list * pmbl = (multi_buffer_list *) data;
	int32 chan;
	uint32 buf, page_bytes;
	cpu_status cp;
	status_t err = B_OK;
	
	(void)len;	// unused

	ddprintf(("envy24: get_buffers\n"));
	if (pmbl == NULL || pmbl->info_size < sizeof(multi_buffer_list)){
		return B_BAD_VALUE;
	}	
	
	/* We need to always return the following: */
	pmbl->flags = 0;
	
	pmbl->return_playback_buffers = 2; /* double buffers */	
	pmbl->return_playback_channels = DMAOUTS_PER_BD;
	pmbl->return_playback_buffer_size = cd->channel_buff_size; /* samples */

	pmbl->return_record_buffers = 2; /* double buffers */
	pmbl->return_record_channels = DMAINS_PER_BD;		
	pmbl->return_record_buffer_size = cd->channel_buff_size; /* samples */
	
	/* Buffer query? */
	if ((pmbl->playback_buffers == NULL) &&
		(pmbl->record_buffers == NULL))	{
		ddprintf(("envy24: buffer query\n"));
		return err;
	}
	else {
		/* it's for real, see if we can accomodate the requests..*/
		if (pmbl->request_playback_buffer_size != pmbl->request_record_buffer_size ||
			pmbl->request_playback_buffers     != 2                                ||
			pmbl->request_playback_channels	   != DMAOUTS_PER_BD                   ||
			pmbl->request_record_buffers	   != 2                                ||	
			pmbl->request_record_channels      != DMAINS_PER_BD                     ) {
				err = B_BAD_VALUE;
		}
		else {
			switch (pmbl->request_playback_buffer_size) {
			case   64:
			case  128:
			case  256:
			case  512:
			case 1024:
			case 2048:
				pmbl->return_playback_buffer_size = pmbl->request_playback_buffer_size;			
				pmbl->return_record_buffer_size   = pmbl->request_playback_buffer_size;
				cd->channel_buff_size             = pmbl->request_playback_buffer_size;
			case    0:			/* Keep it at cd->channel_buff_size */
			default:			/* Return the defaults if we get wrong values */
				err = B_OK;
				break;
			}
		}			
	}

	if (err != B_OK) {
		dprintf("envy24: get_buffers has bad value\n");
		return err;
	}	
	if (cd->is_playing ) {
		dprintf("envy24: get_buffers called while playing\n");
		err = B_BUSY;
		return err;
	}	

	/* here goes..............................................*/
	/* allocate the buffers in get_buffers and return the     */
	/* pointers. set_buffers is not allowed with this card    */
	/* try to allocate around the 16 MB address, (we must     */
	/* have an address < 256 MB) if it fails, allocate with   */
	/* the LOMEM flag.                                        */
	/* note that we currently free and allocate each time     */
	/* we are called. A preferred method would be to check    */
	/* the size and free then allocate, if, and only if, the  */
	/* the new buff size is bigger than the previous buff     */
	/* size.                                                  */

	/* playback */
	if (cd->channels[0].buf[0] != NULL) {
		/* free it up */
		delete_area(cd->channel_area_pb);
		cd->channel_area_pb    = B_ERROR;
		cd->channels[0].buf[0] = NULL;
	}	
	/* 32 bit samples */
	page_bytes = ((DMAOUTS_PER_BD *	cd->channel_buff_size *	2 *
					sizeof(int32) /	B_PAGE_SIZE)+1) * B_PAGE_SIZE;
	ddprintf(("envy24: playback page bytes is 0x%lx\n",page_bytes));
	err = get_envy24_memory(cd, page_bytes, 1);
	if (err != B_OK) {
			return err;
	}

	/* capture */		
	if (cd->channels[DMAOUTS_PER_BD].buf[0] != NULL) {
		/* free it up */
		delete_area(cd->channel_area_cap);
		cd->channel_area_cap = B_ERROR;
		cd->channels[DMAOUTS_PER_BD].buf[0] = NULL;
	}	
	/* 32 bit samples */
	page_bytes = ((DMAINS_PER_BD * cd->channel_buff_size * 2 *
					sizeof(int32) /	B_PAGE_SIZE)+1) * B_PAGE_SIZE;
	ddprintf(("envy24: capture page bytes is 0x%lx\n",page_bytes));
	err = get_envy24_memory(cd, page_bytes, 0);
	if (err != B_OK) {
		/* free it up */
		delete_area(cd->channel_area_pb);
		cd->channel_area_pb   = B_ERROR;
		cd->channels[0].buf[0] = NULL;
		return err;
	}

	/* configure the buffer arrays, then fill   */	
	/* the rest of the multi buffer list struct */
	/* playback */
	for (buf = 0; buf < 2; buf++) {
		for (chan = 0; chan < DMAOUTS_PER_BD; chan++) {	
			cd->channels[chan].buf[buf] =
						cd->channels[0].buf[0] +
						(chan * sizeof(int32)) +
						(buf * cd->channel_buff_size * sizeof(int32) * DMAOUTS_PER_BD);

			pmbl->playback_buffers[buf][chan].base = 
					(char *) cd->channels[chan].buf[buf];
			pmbl->playback_buffers[buf][chan].stride = DMAOUTS_PER_BD * sizeof(int32);
#if 0
			ddprintf(("envy24: chan %02ld  buf %02ld  ptr %p\n",
									chan,
									buf,
									cd->channels[chan].buf[buf]));	
#endif
		}
	}
		
	/* capture */
	for (buf = 0; buf < 2; buf++) {
		for (chan = 0; chan < DMAINS_PER_BD; chan++) {	
			cd->channels[DMAOUTS_PER_BD + chan].buf[buf] =
						cd->channels[DMAOUTS_PER_BD].buf[0] +
						(chan * sizeof(int32)) +
						(buf * cd->channel_buff_size * sizeof(int32) * DMAINS_PER_BD);

			pmbl->record_buffers[buf][chan].base = 
					(char *) cd->channels[DMAOUTS_PER_BD + chan].buf[buf];
			pmbl->record_buffers[buf][chan].stride = DMAINS_PER_BD * sizeof(int32);
#if 0
			ddprintf(("envy24: chan %02ld  buf %02ld  ptr %p\n",
									DMAOUTS_PER_BD + chan,
									buf,
									cd->channels[DMAOUTS_PER_BD + chan].buf[buf]));	
#endif
		}
	}

	ddprintf(("envy24: chan %02d  buf %02d  ptr %p\n",
									0,
									0,
									cd->channels[0].buf[0]));			
	ddprintf(("envy24: chan %02d  buf %02d  ptr %p\n",
									DMAOUTS_PER_BD,
									0,
									cd->channels[DMAOUTS_PER_BD].buf[0]));	

	/* set the chip */
	/* acquire sem */
	cp = disable_interrupts();
	acquire_spinlock(&cd->spinlock);
		PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_PB_ADDR_REG,  (uint32)(cd->channel_physical_pb.address) );
		PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_CNT_REG,   (DMAOUTS_PER_BD * (cd->channel_buff_size << 1)) - 1 );
		PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_TC_REG,    (DMAOUTS_PER_BD *  cd->channel_buff_size) - 1 );
		PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_REC_ADDR_REG, (uint32)(cd->channel_physical_cap.address) );
		PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_CNT_REG,  (DMAINS_PER_BD  * (cd->channel_buff_size << 1)) - 1 );
		PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_TC_REG,   (DMAINS_PER_BD  *  cd->channel_buff_size)  - 1 );
	release_spinlock(&cd->spinlock);
	restore_interrupts(cp);

	return err;
}

status_t
envy24_multi_set_buffers(
				void * cookie,
				void * data,
				size_t len)
{
	(void)cookie;	// unused
	(void)data;		// unused
	(void)len;		// unused

	/* the envy24 doesn't support soft buffers so fail the call */
	/* let me rephase that, I'm too lazy to map the virtual     */
	/* addresses to a physical address...and then do all the    */
	/* requisite error checking (make sure it's less than 256 M.*/
	return B_NOT_ALLOWED;
}

status_t
envy24_multi_force_stop( void * cookie)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	cpu_status cp;	
	ddprintf(("envy24: force stop \n"));

	cp = disable_interrupts();
	acquire_spinlock(&cd->spinlock);
		/* shut down output channels */
		/* shut down input channels */
		PCI_IO_WR(cd->mt_iobase + MTPR_CTL_REG, 0);
		/* turn off the interrupt, an int could come if */
		/* it came after disable_int, before shut down  */
		PCI_IO_WR(cd->mt_iobase, 0x43);
		
		cd->is_playing = false;
	release_spinlock(&cd->spinlock);
	restore_interrupts(cp);

	/* prepare device: set initial buffer index to 0 */
	cd->channels[0].current_buffer = 0;
	cd->preroll = 0;

	if (cd->timer_stuff.cookie) {
		cancel_timer( (timer *) &(cd->timer_stuff));
		cd->timer_stuff.cookie = 0;
	}	

	/* reset the irq_semaphore */
	{
		char name[32];
		delete_sem(cd->irq_sem);
		sprintf(name, "envy24 %ld irq_sem", cd->ix);
		cd->irq_sem = create_sem(1,name);
		set_sem_owner(cd->irq_sem, B_SYSTEM_TEAM);
	}

	/* an event has transpired, put in the queue */
	if ( cd->current_mask & B_MULTI_EVENT_STOPPED ) {
		if ( cd->event_count >= MAX_EVENTS){
			ddprintf(("envy24: out of event space\n"));
		}
		else {
			cd->event_queue[ cd->event_count].event = B_MULTI_EVENT_STOPPED;
			cd->event_queue[ cd->event_count].info_size = sizeof(multi_get_event);
			cd->event_queue[ cd->event_count].count = 0;
			cd->event_queue[ cd->event_count].timestamp = system_time();
			/* bump event_count */
			cd->event_count++;
			/* release sem */
			if (cd->event_sem >= B_OK) {
				release_sem(cd->event_sem);
			}		
		}	
	}	
	
	return B_OK;
}


status_t
envy24_multi_buffer_exchange(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_buffer_info * pmbi = (multi_buffer_info *) data;
	cpu_status cp;

	(void)len;	// unused

	/* no parameter checking for now, increase speed */	

	if (B_OK != acquire_sem_etc(cd->irq_sem, 1, B_CAN_INTERRUPT, 0)) {
		ddprintf(("envy24: no irq semaphore in MBE\n"));
		return B_ERROR;
	}

	/* If not started, start (unless we are waiting on start_at) */
	if (!cd->is_playing && !cd->start_time) {
		ddprintf(("envy24: Start Playing\n"));

		cp = disable_interrupts();
		acquire_spinlock(&cd->spinlock);

			PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_PB_ADDR_REG,  (uint32)(cd->channel_physical_pb.address) );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_CNT_REG,   (DMAOUTS_PER_BD * (cd->channel_buff_size << 1)) - 1 );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_TC_REG,    (DMAOUTS_PER_BD *  cd->channel_buff_size) - 1 );
			PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_REC_ADDR_REG, (uint32)(cd->channel_physical_cap.address) );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_CNT_REG,  (DMAINS_PER_BD  * (cd->channel_buff_size << 1)) - 1 );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_TC_REG,   (DMAINS_PER_BD  *  cd->channel_buff_size)  - 1 );

			PCI_IO_WR(cd->mt_iobase + MTPR_CTL_REG, 0x5);
			cd->is_playing = true;
			cd->channels[0].current_buffer = 0;
		release_spinlock(&cd->spinlock);
		restore_interrupts(cp);
#if 1
		/* an event has transpired, put in the queue */
		if ( cd->current_mask & B_MULTI_EVENT_STARTED ) {
			if ( cd->event_count >= MAX_EVENTS){
				ddprintf(("envy24: out of event space\n"));
			}
			else {
				cd->event_queue[ cd->event_count].event = B_MULTI_EVENT_STARTED;
				cd->event_queue[ cd->event_count].info_size = sizeof(multi_get_event);
				cd->event_queue[ cd->event_count].count = 0;
				cd->event_queue[ cd->event_count].timestamp = system_time();
				/* bump event_count */
				cd->event_count++;
				/* release sem */
				if (cd->event_sem >= B_OK) {
					release_sem(cd->event_sem);
				}		
			}	
		}
#endif	
	}
	if (cd->preroll < 1 ) { 
	 	pmbi->record_buffer_cycle = -1;
		cd->preroll++;	
		pmbi->playback_buffer_cycle = 	cd->channels[0].current_buffer++ & 1;
	}
	else {
		pmbi->record_buffer_cycle = cd->channels[0].current_buffer & 1;
		pmbi->playback_buffer_cycle = 	cd->channels[0].current_buffer++ & 1;
	}

//dprintf("%d\n",pmbi->playback_buffer_cycle);

	/* pmbi->flags */
	pmbi->played_real_time		= cd->timestamp;
	pmbi->played_frames_count	= cd->pb_position;
	pmbi->recorded_real_time	= cd->timestamp;
	pmbi->recorded_frames_count	= cd->rec_position;

	return B_OK;
}

static multi_mode_info mode_info[] = {
{	ENVY24_MODE_0,
	0,
	"Envy24 Reference",
	INPUTS_PER_BD,
	OUTPUTS_PER_BD,
	96000.0,
	96000.0,
	B_FMT_24BIT | B_FMT_IS_GLOBAL,
	B_FMT_24BIT | B_FMT_IS_GLOBAL | B_FMT_SAME_AS_INPUT,
	{}
}
};	
status_t
envy24_multi_list_modes(
				void * cookie,
				void * data,
				size_t len)

{
//	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_mode_list * pmml = (multi_mode_list *) data;
	int32 max;
	
	(void)cookie;	// unused
	(void)len;		// unused

	if (pmml == NULL || pmml->info_size < sizeof(multi_mode_list)){
		return B_BAD_VALUE;
	}	
	pmml->info_size = sizeof(multi_mode_list);
	max = pmml->in_request_count;
	pmml->out_actual_count = sizeof(mode_info)/sizeof(mode_info[0]);
	if (max > pmml->out_actual_count) { 
		max = pmml->out_actual_count;
	}	
	memcpy(pmml->io_modes, mode_info, max*sizeof(mode_info[0]));
	return B_OK;
}

status_t
envy24_multi_get_mode(
				void * cookie,
				void * data,
				size_t len)

{
//	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	status_t err = B_OK;

	(void)cookie;	// unused
	(void)len;		// unused

	if (data != NULL) {
		(*(int32 *) data) = ENVY24_MODE_0;
	}
	else {
		err = B_BAD_VALUE;
	}		  
	return err;
}

status_t
envy24_multi_set_mode(
				void * cookie,
				void * data,
				size_t len)

{
//	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	
	(void)cookie;	// unused
	(void)len;		// unused

	if ( data == NULL || *(int32 *) data != ENVY24_MODE_0) {
		return B_BAD_VALUE;
	}
	return B_OK;
}

status_t
envy24_multi_list_extensions(
				void * cookie,
				void * data,
				size_t len)

{
//	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_extension_list * pmel = (multi_extension_list *) data;
	
	(void)cookie;	// unused
	(void)len;		// unused

	if (pmel == NULL || pmel->info_size < sizeof(multi_extension_list)){
		return B_BAD_VALUE;
	}	
	pmel->info_size = sizeof(multi_extension_list);
	pmel->actual_count = 0;
	return B_OK;
}

status_t
envy24_multi_get_extension(
				void * cookie,
				void * data,
				size_t len)

{
	(void)cookie;	// unused
	(void)data;		// unused
	(void)len;		// unused

	return B_NOT_ALLOWED;
}

status_t
envy24_multi_set_extension(
				void * cookie,
				void * data,
				size_t len)

{
	(void)cookie;	// unused
	(void)data;		// unused
	(void)len;		// unused

	return B_NOT_ALLOWED;
}

status_t
envy24_multi_get_event_info(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_get_event_info * pgei = (multi_get_event_info *) data;
	
	(void)len;		// unused

	if (pgei == NULL || pgei->info_size < sizeof(multi_get_event_info)){
		return B_BAD_VALUE;
	}	
	pgei->info_size = sizeof(multi_get_event_info);
	pgei->supported_mask= SUPPORTED_EVENT_MASK;
	pgei->current_mask	= cd->current_mask;
	pgei->queue_size	= cd->queue_size;
	pgei->event_count	= cd->event_count;
	return B_OK;
}

status_t
envy24_multi_set_event_info(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_set_event_info * psei = (multi_set_event_info *) data;
	
	(void)len;		// unused

	if (psei == NULL ||
		psei->info_size < sizeof(multi_set_event_info) ){
		return B_BAD_VALUE;
	}

	cd->current_mask= SUPPORTED_EVENT_MASK & psei->in_mask;

	/* semaphore may be "null" without an error */
	cd->event_sem	= psei->semaphore;	
	
	if (psei->queue_size >= MAX_EVENTS ) {
		cd->queue_size = MAX_EVENTS;
	}
	else {
		cd->queue_size = psei->queue_size;
		if (cd->event_count > cd->queue_size ) {
			uint32 i,j;
			j = cd->event_count + 1 - cd->queue_size ;
			/* sleazy hack, very inefficient--we need to use linked lists.... */
			for (i=0; i< cd->queue_size; i++,j++) {
				cd->event_queue[i] = cd->event_queue[j];
			}
			cd->event_count = cd->queue_size;
		}	
	}	

	return B_OK;
}

status_t
envy24_multi_get_event(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_get_event * pge = (multi_get_event *) data;
	uint32 i;
	/* sleazy hack, very inefficient--we need to use linked lists.... */
	
	(void)len;		// unused

	if (pge == NULL || pge->info_size < sizeof(multi_get_event)){
		return B_BAD_VALUE;
	}
	
	if (cd->event_count) {
		*pge = cd->event_queue[0];
		cd->event_count--;
		for (i=0; i< cd->event_count; i++) {
			cd->event_queue[i] = cd->event_queue[i+1];
		}
	}	
	else {
		pge->info_size= sizeof(multi_get_event);
		pge->event    = B_MULTI_EVENT_NONE;
		pge->timestamp= system_time();
		pge->count    = 0;
	}
	return B_OK;
}


status_t
timer_set_start(
				timer * cookie
				)
{
	timer_plus * tp = (timer_plus *)cookie;
	envy24_dev * cd= (envy24_dev *) (tp->cookie);
	status_t err;
	cpu_status cp;

	/* need a semaphore.... */

	/* if timer expired in the middle of force_stop */
	/* we can still kill the start................. */
	if (cd->start_time) {

		cp = disable_interrupts();
		acquire_spinlock(&(cd->spinlock));
		
			PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_PB_ADDR_REG,  (uint32)(cd->channel_physical_pb.address) );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_CNT_REG,   (DMAOUTS_PER_BD * (cd->channel_buff_size << 1)) - 1 );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_PB_TC_REG,    (DMAOUTS_PER_BD *  cd->channel_buff_size) - 1 );
			PCI_IO_WR_32(cd->mt_iobase + MTPR_DMA_REC_ADDR_REG, (uint32)(cd->channel_physical_cap.address) );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_CNT_REG,  (DMAINS_PER_BD  * (cd->channel_buff_size << 1)) - 1 );
			PCI_IO_WR_16(cd->mt_iobase + MTPR_DMA_REC_TC_REG,   (DMAINS_PER_BD  *  cd->channel_buff_size)  - 1 );
		
			PCI_IO_WR(cd->mt_iobase + MTPR_CTL_REG, 0x5);
			cd->is_playing = true;
			cd->channels[0].current_buffer = 0;

		release_spinlock(&(cd->spinlock));
	    restore_interrupts(cp);

		cd->start_time = (bigtime_t) 0;
		
		/* an event has transpired, put in the queue */
		if ( cd->current_mask & B_MULTI_EVENT_STARTED ) {
			if ( cd->event_count >= MAX_EVENTS){
				ddprintf(("envy24: out of event space\n"));
			}
			else {
				cd->event_queue[ cd->event_count].event = B_MULTI_EVENT_STARTED;
				cd->event_queue[ cd->event_count].info_size = sizeof(multi_get_event);
				cd->event_queue[ cd->event_count].count = 0;
				cd->event_queue[ cd->event_count].timestamp = system_time();
				/* bump event_count */
				cd->event_count++;
				/* release sem */
				if (cd->event_sem >= B_OK) {
					release_sem(cd->event_sem);
				}		
			}	
		}	
		
	}	
	err = B_OK;
	return err;	
}				

status_t
envy24_multi_set_start_time(
				void * cookie,
				void * data,
				size_t len)

{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	bigtime_t start_time;	

	(void)len;		// unused

	if ( data == NULL ) {
		return B_BAD_VALUE;
	}
	start_time = * (bigtime_t *) data;
	if (system_time() < start_time && !cd->is_playing) {
		/* start is in the future, actually do something... */
		/* stop MBE until start time */
		cd->start_time = start_time;
		cd->preroll = 0;
		/* setup the timer */
		cd->timer_stuff.cookie = (void *) cd;
		add_timer((timer *)&(cd->timer_stuff), timer_set_start, start_time, B_ONE_SHOT_ABSOLUTE_TIMER);		
		return B_OK;
	}	
	return B_ERROR;
}




static char * aname[3] = { 
		"envy24 cap area",
		"envy24 pb area",
		NULL
};


status_t
get_envy24_memory(
				envy24_dev *cd ,
				uint32 page_bytes,
				uint32 playback )
{
	status_t err = B_OK;
	char * tmp_buf;
	area_id tmp_area;
	physical_entry tmp_phys;
	
	if (playback) {
		playback = 1;
	}	

	tmp_area = create_area(
						aname[playback],
						(void **)&tmp_buf,
						B_ANY_KERNEL_ADDRESS,
						page_bytes, 
						B_CONTIGUOUS,
						B_READ_AREA | B_WRITE_AREA);

	if (tmp_area >=B_OK) {
		/* Get a physical address from a virtual address */
		if (get_memory_map(tmp_buf, B_PAGE_SIZE , &tmp_phys, 1) < 0) {
			/* this is supposed to be the error handler, but get_memory_map */
			/* (in the current version) ONLY returns 0 ....                  */
			delete_area(tmp_area);
			err = B_ERROR; 
			dprintf("envy24: error mapping memory\n");
		}
		else {
			/* is the area under the 256 M mark? */
			if ((*pci->ram_address)(tmp_phys.address) >= (void *)0x10000000) {
				/* try it with lomem this time..... */
				delete_area(tmp_area);
				tmp_area = create_area(
									aname[playback],
									(void **)&tmp_buf,
									B_ANY_KERNEL_ADDRESS,
									page_bytes, 
									B_LOMEM,
									B_READ_AREA | B_WRITE_AREA);
				if (tmp_area >=B_OK) {
					/* It succeeded, so our address < 16 M < 256M    */
					/* Get a physical address from a virtual address */
					if (get_memory_map(tmp_buf, B_PAGE_SIZE , &tmp_phys, 1) < 0) {
						/* this is supposed to be the error handler, but get_memory_map */
						/* (in the current version ONLY returns 0 ....                  */
						delete_area(tmp_area);
						err = B_ERROR; 
						dprintf("envy24: error mapping memory\n");
					}
					else {
						err = B_OK;
						ddprintf(("envy24: lomem physical allocation at %p\n", tmp_phys.address));
					}	
				}
				else {
					/* create area failed */
					err = B_ERROR;
					dprintf("envy24: create area error\n");
				}
			}
			else {
				/* the address is valid */
				err = B_OK;
				ddprintf(("envy24: himem physical allocation at %p\n", tmp_phys.address));
			}								
		}
	}
	else {
		/* create area failed */
		err = B_ERROR;
		dprintf("envy24: create area error\n");
	}
	
	if (err == B_OK) {
		if (playback) {
			cd->channel_area_pb     = tmp_area;
			cd->channel_physical_pb = tmp_phys;
			cd->channels[0].buf[0]  = tmp_buf;
			ddprintf(("envy24: return physical allocation at %p\n", cd->channel_physical_pb.address));
			ddprintf(("envy24: (*pci->ram_address)           %p\n", (*pci->ram_address)(cd->channel_physical_pb.address)));
		}
		else {
			cd->channel_area_cap     = tmp_area;
			cd->channel_physical_cap = tmp_phys;
			cd->channels[DMAOUTS_PER_BD].buf[0] = tmp_buf;
			/* debug */
#if DEBUG
			memset(tmp_buf, 0x55, page_bytes);
#endif			
			ddprintf(("envy24: return physical allocation at %p\n", cd->channel_physical_cap.address));
			ddprintf(("envy24: (*pci->ram_address)           %p\n", (*pci->ram_address)(cd->channel_physical_cap.address)));
		}
	}
	
	return err;	
}
