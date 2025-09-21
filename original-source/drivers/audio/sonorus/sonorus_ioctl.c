/* includes */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

#include "sonorus.h"

/* debug */
#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf (void)
#endif

/* globals */
extern sonorus_dev cards[MAX_STUDIO_BD];

extern int card_count;
extern int dsp_caps;
extern int outputs_per_bd;
extern int inputs_per_bd;
extern int monitors_per_bd;
extern int packed_mode;
extern int is_playing;

/* explicit declarations for the compiler */
void	 set_sync_params_MonMix_1111( uint32, studSyncSrc_t );
void	 set_channel_param(	studParmID_t, int32, int32 );
void	 set_device_param( studParmID_t, int32 );
int32	 get_device_param( studParmID_t );
void	 set_play_enbl( bool );
void	 write_chnl_infos( uint32, chnlInfoX_t, chnlInfoY_t );
void	 write_play_buf( uint32, uint32, void*, uint32, studBufType_t, void* );
void 	 read_recd_buf( uint32, uint32, void*, uint32, studBufType_t, void* );


/* the functions... */
status_t
sonorus_multi_get_description(
							void * cookie,
							void * data,
							size_t len)
{
	open_sonorus * card = (open_sonorus *)cookie;
	multi_description * pmd = (multi_description *) data;
	int channel, i;

	if (pmd == NULL || pmd->info_size < sizeof(multi_description)) {
		return B_BAD_VALUE;
	}	
	if(card->device->ix) {
		sprintf(pmd->friendly_name, "STUDI/O %01d", card->device->ix + 1);
	}
	else {
		sprintf(pmd->friendly_name, "STUDI/O");
	}
	strcpy(pmd->vendor_info,"Sonorus");
	pmd->info_size = sizeof(multi_description);
	pmd->interface_version = B_MINIMUM_INTERFACE_VERSION;
	pmd->interface_minimum = B_MINIMUM_INTERFACE_VERSION;
	pmd->input_channel_count  = inputs_per_bd;
	pmd->output_channel_count = outputs_per_bd;
	pmd->input_bus_channel_count  = inputs_per_bd;
	pmd->output_bus_channel_count = outputs_per_bd + 2;
	pmd->aux_bus_channel_count = 0;
	channel = 0;
	for (i = 0; i < outputs_per_bd; i++) {
		if (channel >= pmd->request_channel_count) {
			break;
		}
		pmd->channels[channel].channel_id =	channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_CHANNEL;
		pmd->channels[channel].designations	= 0;
		pmd->channels[channel].connectors = 0;
		channel++;											
	}
	for (i = 0; i < inputs_per_bd; i++)
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
	for (i = 0; i < outputs_per_bd; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf("sonorus: request_channel_count killed output busses \n");
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_BUS;
		pmd->channels[channel].designations = 0;
		pmd->channels[channel].connectors = i < 8 ? B_CHANNEL_LIGHTPIPE :
													B_CHANNEL_OPTICAL_SPDIF;
		channel++;
	}	
	for (i = 0; i < monitors_per_bd; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf("sonorus: request_channel_count killed monitor setup \n");
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_OUTPUT_BUS;
		pmd->channels[channel].designations = i & 0x1 ? B_CHANNEL_RIGHT : B_CHANNEL_LEFT;
		pmd->channels[channel].connectors = B_CHANNEL_QUARTER_INCH_STEREO;
		channel++;
	}	
	for (i = 0; i < inputs_per_bd; i++)
	{
		if (channel >= pmd->request_channel_count) {
			ddprintf("sonorus: request_channel_count killed input busses \n");
			break;
		}	
		pmd->channels[channel].channel_id = channel;
		pmd->channels[channel].kind = B_MULTI_INPUT_BUS;
		pmd->channels[channel].designations = 0;
		pmd->channels[channel].connectors = i < 8 ? B_CHANNEL_LIGHTPIPE :
													B_CHANNEL_OPTICAL_SPDIF;
		channel++;
	}	
	/* how do we show that S/PDIF can support 32k? */
	pmd->input_rates  = B_SR_44100 | B_SR_48000 | B_SR_IS_GLOBAL;
	pmd->min_cvsr_rate = 0;
	pmd->max_cvsr_rate = 0;
	pmd->output_rates = pmd->input_rates | B_SR_SAME_AS_INPUT ;
	/* the api only supports LEFT justified... */
	pmd->input_formats = B_FMT_24BIT | B_FMT_IS_GLOBAL;
	pmd->output_formats = pmd->input_formats | B_FMT_SAME_AS_INPUT;
	pmd->lock_sources = B_MULTI_LOCK_INTERNAL |
						B_MULTI_LOCK_LIGHTPIPE |
						B_MULTI_LOCK_SPDIF |
						B_MULTI_LOCK_WORDCLOCK;
	pmd->timecode_sources = 0;									  

	/* I _think_ it clicks.... */
	pmd->interface_flags =	B_MULTI_INTERFACE_PLAYBACK | 
							B_MULTI_INTERFACE_RECORD |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_OUTPUTS |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_INPUTS |
							B_MULTI_INTERFACE_SOFT_PLAY_BUFFERS |
							B_MULTI_INTERFACE_SOFT_REC_BUFFERS |
							B_MULTI_INTERFACE_TIMECODE;
	/* let's do this later... */
	//FIX ME!!!!! 
	pmd->control_panel[0] = 0;

	return B_OK;
							
}

status_t
sonorus_multi_get_enabled_channels(
							void * cookie,
							void * data,
							size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_channel_enable * pmce = (multi_channel_enable *) data;
	
	if ( pmce != NULL && pmce->info_size >= sizeof(multi_channel_enable)) {
		pmce->lock_source    = cd->mce.lock_source;
		pmce->lock_data	     = cd->mce.lock_data;
		pmce->timecode_source= cd->mce.timecode_source;
		//FIX ME!!!! (endian-ness)
		*(uint32 *)pmce->enable_bits = cd->channels_enabled;
		return B_OK;
	}
	else {
		return B_BAD_VALUE;
	}		
}

status_t
sonorus_multi_set_enabled_channels(
							void * cookie,
							void * data,
							size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_channel_enable * pmce = (multi_channel_enable *) data;
	studSyncSrc_t src;
	int32 tmp,chan;
	uint32 mask;
	status_t err = B_OK;

	if (pmce == NULL || pmce->enable_bits == NULL || pmce->info_size < sizeof(multi_channel_enable)) {
		ddprintf("sonorus: NULL pointer or info_size wrong\n");
		return B_BAD_VALUE;
	}

	/* let's do the clocks...ignore timecode for now*/
	switch (pmce->lock_source) {
		case B_MULTI_LOCK_INPUT_CHANNEL:
			if (pmce->lock_data < 8) {
				src = STUD_SYNCSRC_EXTA;
			}	
			else {
				src = STUD_SYNCSRC_EXTB;
			}
			break;	
		case B_MULTI_LOCK_WORDCLOCK:
			src = STUD_SYNCSRC_EXTWC;
			break;
		case B_MULTI_LOCK_SPDIF:
			src = STUD_SYNCSRC_EXTB;
			break;
		case B_MULTI_LOCK_LIGHTPIPE:
			src = STUD_SYNCSRC_EXTA;
			break;
		case B_MULTI_LOCK_INTERNAL:
			src = STUD_SYNCSRC_INT;
			break;
		default:
			ddprintf("sonorus: Unsupported synch source (0x%lx)\n", pmce->lock_source);
			err = B_BAD_VALUE;
			src = STUD_SYNCSRC_INT;
			break;
	}

	if (err != B_OK) {
		return err;
	}
	
	//FIX ME!!!!! (need mask for request_channel_count and/or error return)
	//	if (changed_channels & invalid_channel_mask) {
	//		return B_BAD_VALUE;
	//	}	
	
	tmp = cd->channels_enabled;
	/* all values are now good, copy and set */
	//FIX ME!!!! (endian-ness)
	cd->channels_enabled = *(uint32 *) pmce->enable_bits; 
	cd->mce = *pmce;
	cd->lock_source = src;
	set_sync_params_MonMix_1111( cd->sample_rate, src);			

	//enable/disable accordingly....
	for (chan = 0, mask = 1; chan < outputs_per_bd ; chan++, mask <<= 1) {
		if ( mask & cd->channels_enabled) {
			cd->channels[chan].output_ramp = -abs(cd->channels[chan].output_ramp);
		}
		else {
			cd->channels[chan].output_ramp = abs(cd->channels[chan].output_ramp);
		}
		set_channel_param(	STUD_PARM_OUTENB,
							chan + cd->ix * outputs_per_bd ,
							0x8000000L / cd->channels[chan].output_ramp );
	}

	ddprintf("sonorus: (set) enabled channels are 0x%lx lock source 0x%lx\n", *((int32 *) (pmce->enable_bits)), pmce->lock_source);	

	return B_OK;
}

status_t
sonorus_multi_set_global_format(
							void * cookie,
							void * data,
							size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_format_info * pmfi = (multi_format_info *) data;
	status_t err = B_OK;
	uint32 sr;

	if (pmfi == NULL || pmfi->info_size < sizeof(multi_format_info) ){
		ddprintf("sonorus: set_global_format null ptr, wrong struct size\n");
		return B_BAD_VALUE;
	}	

	if (pmfi->output.rate != pmfi->input.rate) {
		ddprintf("sonorus: set_global_format out != in \n");
		err = B_BAD_VALUE;
	} 

	if (pmfi->output.rate != B_SR_44100 &&
		pmfi->output.rate != B_SR_48000 ) {
		ddprintf("sonorus: set_global_format bad sr 0x%lx\n",pmfi->output.rate);
		err = B_BAD_VALUE;	
	}	
	
	/* we are ignoring timecode for now */
	if (err == B_OK) {
		if ( pmfi->output.rate == B_SR_44100 ) {
			sr = cd->sample_rate = 44100;
		}
		else {
			sr = cd->sample_rate = 48000;
		}
		set_sync_params_MonMix_1111( sr, cd->lock_source);
		
		if ( cd->current_mask & B_MULTI_EVENT_CHANNEL_FORMAT_CHANGED) {
			if ( cd->event_count >= MAX_EVENTS){
				ddprintf("sonorus: out of event space\n");
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
sonorus_multi_get_global_format(
							void * cookie,
							void * data,
							size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_format_info * pmfi = (multi_format_info *) data;
	status_t err = B_OK;
	if ( pmfi != NULL && pmfi->info_size >= sizeof(multi_format_info)) {
		//FIX ME!!!! (latency not right)
		pmfi->output_latency= 1000; // FIX THIS!!!!
		pmfi->input_latency = 1000; // FIX THIS!!!!
		pmfi->timecode_kind = B_MULTI_NO_TIMECODE;
		pmfi->output.rate= cd->sample_rate == 48000 ? B_SR_48000 : B_SR_44100;
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
sonorus_multi_get_buffers(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_buffer_list * pmbl = (multi_buffer_list *) data;
	int32 chan;
	uint32 buf, page_bytes;
	char * tmp;
	status_t err = B_OK;
	
	ddprintf("sonorus: get_buffers\n");
	if (pmbl == NULL || pmbl->info_size < sizeof(multi_buffer_list)){
		return B_BAD_VALUE;
	}	
	
//FIX ME! need to check request_channel_count: we will still allocate 
//FIX ME! memory for ALL the channels
//FIX ME! we just won't return all the buffer pointers........ 

	/* We need to always return the following: */
	pmbl->flags = 0;
	
	pmbl->return_playback_buffers = 2; /* double buffers */	
	pmbl->return_playback_channels = outputs_per_bd;
	pmbl->return_playback_buffer_size = cd->channel_buff_size; /* samples */

	pmbl->return_record_buffers = 2; /* double buffers */
	pmbl->return_record_channels = inputs_per_bd;		
	pmbl->return_record_buffer_size = cd->channel_buff_size; /* samples */
	
	/* Buffer query? */
	if ((pmbl->playback_buffers == NULL) &&
		(pmbl->record_buffers == NULL))	{
		ddprintf("sonorus: buffer query\n");
		return err;
	}
	else {
		/* it's for real, see if we can accomodate the requests..*/
		if (pmbl->request_playback_buffer_size != pmbl->request_record_buffer_size ||
			pmbl->request_playback_buffers     != 2                                ||
			pmbl->request_playback_channels	   != outputs_per_bd                   ||
			pmbl->request_record_buffers	   != 2                                ||	
			pmbl->request_record_channels      != inputs_per_bd                     ) {
				err = B_BAD_VALUE;
		}
		else {
			switch (pmbl->request_playback_buffer_size) {
			case   64:
			case  128:
			case  192:
			case  256:
			case  384:
			case  512:
			case  768:
			case 1024:
			case 1536:
			case 1820:         /* half of h/w buffer */
				pmbl->return_playback_buffer_size = pmbl->request_playback_buffer_size;			
				pmbl->return_record_buffer_size   = pmbl->request_playback_buffer_size;
				cd->channel_buff_size             = pmbl->request_playback_buffer_size;
			case    0:			/* Keep it at cd->channel_buff_size */
			default:			/* Return the defaults if we wrong values */
				err = B_OK;
				break;
			}
		}			
	}

	if (err != B_OK) {
		dprintf("sonorus: get_buffers has bad value\n");
		return err;
	}	

	/* here goes..............................................*/
	/* If set_buffers hasn't been called with buffers, then   */
	/* allocate the buffers in get_buffers and return the     */
	/* pointers. If set_buffers is called _after_ we have     */
	/* allocated buffers, then free those buffers and use     */
	/* the ones in set_buffers.                               */	

	if (cd->channels[0].buf[0] == NULL) {
		/* let's allocate some memory (assume 24 bit samples)       */
		/* 16 bit samples would halve our memory requirements,      */
		/* but then we couldn't change without restarting driver    */
		page_bytes = (((outputs_per_bd + inputs_per_bd) *
						CHNL_BUF_SIZE * sizeof(int32) / B_PAGE_SIZE)+1) *
						B_PAGE_SIZE;
		ddprintf("sonorus: page bytes is 0x%lx\n",page_bytes);				
		cd->channel_area = create_area(
										"sonorus",
										(void **)&tmp,
										B_ANY_KERNEL_ADDRESS,
										page_bytes, 
										B_FULL_LOCK,
										B_READ_AREA | B_WRITE_AREA);

		if (cd->channel_area >= B_OK) {
			cd->channels[0].buf[0] = tmp;
			for (chan = 0; chan < (outputs_per_bd + inputs_per_bd); chan++) {
				for (buf = 0; buf < 2; buf++) {
					cd->channels[chan].buf[buf] =
								cd->channels[0].buf[0] +
								((chan * 2) + buf) *
								(CHNL_BUF_SIZE >> 1) * sizeof(int32);
//					ddprintf("sonorus: chan %02d  buf %02d  ptr 0x%x\n",
//									chan,
//									buf,
//									cd->channels[chan].buf[buf]);			
				}
			}

					ddprintf("sonorus: chan %02d  buf %02d  ptr 0x%p\n",
									0,
									0,
									cd->channels[0].buf[0]);			

		}
		else {
			kprintf("sonorus: Unable to allocate buffer memory\n");
			err = B_ERROR;
		}	
	}
		
	/* if malloc didn't fail, or if the buffers were set in set_buffer */
	/* fill out the rest of the multi buffer list struct */
	if (cd->channels[0].buf[0] != NULL) {
		/* playback */
		for (buf = 0; buf < 2; buf++) {
			for (chan = 0; chan < outputs_per_bd; chan++) {	
				pmbl->playback_buffers[buf][chan].base = 
						(char *) cd->channels[chan].buf[buf];
				pmbl->playback_buffers[buf][chan].stride = sizeof(int32);
			}
		}
		
		/* capture */
		for (buf = 0; buf < 2; buf++) {
			for (chan = 0; chan < inputs_per_bd; chan++) {	
				pmbl->record_buffers[buf][chan].base = 
						(char *) cd->channels[chan+outputs_per_bd].buf[buf];
				pmbl->record_buffers[buf][chan].stride = sizeof(int32);
			}
		}
		err = B_OK;
	}
	return err;
}

status_t
sonorus_multi_set_buffers(
				void * cookie,
				void * data,
				size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_buffer_list * pmbl = (multi_buffer_list *) data;
	uint32 buf;
	int32 chan;
	status_t err = B_BAD_VALUE;
//FIX ME! (need to check request_channel_count)

	/* here goes..............................................*/
	/* If set_buffers hasn't been called with buffers, then   */
	/* allocate the buffers in get_buffers and return the     */
	/* pointers. If set_buffers is called _after_ we have     */
	/* allocated buffers, then free those buffers and use     */
	/* the ones in set_buffers.                               */	

	if (pmbl == NULL || pmbl->info_size < sizeof(multi_buffer_list)) {
		return err;
	}	

	if (cd->channels[0].buf[0] == NULL &&
		pmbl->record_buffers[0][0].base   == NULL &&
		pmbl->playback_buffers[0][0].base == NULL ) {
		return err;
	}	

	if (pmbl->request_playback_buffer_size != pmbl->request_record_buffer_size ||
		pmbl->request_playback_buffers     != 2                                ||
		pmbl->request_playback_channels	   != outputs_per_bd                   ||
		pmbl->request_record_buffers	   != 2                                ||	
		pmbl->request_record_channels      != inputs_per_bd                     ) {
			err = B_BAD_VALUE;
	}
	else {
		switch (pmbl->request_playback_buffer_size) {
		case   64:
		case  128:
		case  192:
		case  256:
		case  384:
		case  512:
		case  768:
		case 1024:
		case 1536:
		case 1820:         /* half of h/w buffer */
			pmbl->return_playback_buffer_size = pmbl->request_playback_buffer_size;			
			pmbl->return_record_buffer_size   = pmbl->request_playback_buffer_size;
			cd->channel_buff_size             = pmbl->request_playback_buffer_size;
			err = B_OK;
			break;
		case    0:			/* Keep it at cd->channel_buff_size */
			pmbl->return_record_buffer_size   = cd->channel_buff_size;
			pmbl->return_playback_buffer_size = pmbl->return_record_buffer_size;			
			err = B_OK;
			break;
		default:
			err = B_BAD_VALUE;
			break;	
		}
	}			

	if (cd->channel_area >= B_OK) {
		delete_area(cd->channel_area);
		cd->channel_area = B_ERROR;
	}			

	/* playback */
	for (buf = 0; buf < 2; buf++) {
		for (chan = 0; chan < outputs_per_bd; chan++) {	
			cd->channels[chan].buf[buf] = 
					pmbl->playback_buffers[buf][chan].base;
		}
	}

	/* capture */
	for (buf = 0; buf < 2; buf++) {
		for (chan = 0; chan < inputs_per_bd; chan++) {	
			cd->channels[outputs_per_bd + chan].buf[buf] =
				pmbl->record_buffers[buf][chan].base;
		}
	}
	return err;

}

status_t
sonorus_multi_force_stop( void * cookie)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	int32 channel;
	
//FIXME: should use get/set control instead of calling
//FIXME: set_channel or set_device directly...
	
	ddprintf("sonorus: force stop (sonorus_ioctl.c)\n");

	/* shut down output channels */
	for (channel = 0; channel < outputs_per_bd; channel++) {
		set_channel_param(	STUD_PARM_OUTENB,
							channel + cd->ix * outputs_per_bd ,
							0x8000000L / abs(cd->channels[channel].output_ramp) );
		set_channel_param(	STUD_PARM_INPMON,
							channel + cd->ix * outputs_per_bd ,
							0x8000000L / abs(cd->channels[channel].monitor_ramp) );
	}	 	
	/* shut down input channels */
	set_device_param( STUD_PARM_RECENB , false );

	/* let the buffer play a little (or out).... */
	snooze(5000);  
	set_play_enbl( false );
	
	/* prepare device: set initial sample counter to zero */
	set_device_param( STUD_PARM_POSL , 0 );
	set_device_param( STUD_PARM_POSH , 0 );

	/* prepare device: set initial DSP buffer index to start of channel buffers */
	set_device_param( STUD_PARM_INITOFST , 0 );

	for (channel = 0; channel < inputs_per_bd + outputs_per_bd; channel++) {
		cd->channels[channel].index = 0;
	}	 	

	cd->channels[outputs_per_bd].current_buffer = 0;
	cd->channels[0].current_buffer = 0;

	cd->start_time = 0;
	cd->preroll = 0;
	cancel_timer( (timer *) &(cd->timer_stuff));

	/* reset the irq_semaphore */
	{
		char name[32];
		delete_sem(cd->irq_sem);
		sprintf(name, "sonorus %d irq_sem", cd->ix);
		cd->irq_sem = create_sem(1,name);
		set_sem_owner(cd->irq_sem, B_SYSTEM_TEAM);
	}
	
	/* an event has transpired, put in the queue */
	if ( cd->current_mask & B_MULTI_EVENT_STOPPED ) {
		if ( cd->event_count >= MAX_EVENTS){
			ddprintf("sonorus: out of event space\n");
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


/*********************************************************************************
 *
 *		Also note that you will maintain the current record offset separate
 *		from your current play offset.  Since the DSP uses the same block
 *		of memory for recording as for playing, your interrupt handler will
 *		need to read the record samples BEFORE writing the play samples IF
 *      you use the same index for play and record.
 *
 **********************************************************************************/

status_t
sonorus_multi_buffer_exchange(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_buffer_info * pmbi = (multi_buffer_info *) data;
	uint32 mask, cntr, tmp, pos;
	int32 chan;
	cpu_status cp;	

	/* no parameter checking for now, increase speed */	

	cntr = cd->channels[0].current_buffer;
	for (chan = 0, mask = 1; chan < outputs_per_bd ; chan++, mask <<= 1) {
#if 1
		if ( mask & cd->channels_enabled) {
			write_play_buf( chan,
							cd->channels[chan].index, 
							cd->channels[chan].buf[cntr & 1], 
							cd->channel_buff_size,
							STUD_BUF_INT32 ,
							NULL );
		}
#endif
		/* slower, but keeps everything in sync */
		cd->channels[chan].index += cd->channel_buff_size;
		if (cd->channels[chan].index >= CHNL_BUF_SIZE) {
			cd->channels[chan].index %= CHNL_BUF_SIZE;
		}	
	}
	cd->chiy[cntr&1].nSamps = cd->channel_buff_size;
	write_chnl_infos( cntr&1 , cd->chix[cntr&1], cd->chiy[cntr&1]);

	if (B_OK != acquire_sem_etc(cd->irq_sem, 1, B_CAN_INTERRUPT, 0)) {
		ddprintf("sonorus: no irq semaphore in MBE\n");
		return B_ERROR;
	}

/* In a perfect world, this would be in the interrupt handler.   */
/* However, this would require more spinlocks to protect memory  */
/* In the absence of those spinlocks, the machine will hang. So, */
/* we fake it and reset the position and timestamp to match the  */
/* the buffer interrupt time.                                    */

	cp = disable_interrupts();
	cd->timestamp = system_time();
	/* upper 8 bits of current position */
	pos = get_device_param(STUD_PARM_POSH);
	/* lower 24 bits of current position */
	tmp = get_device_param(STUD_PARM_POSL);
	/* upper 8 bits of current position */
	if (pos != (uint32)get_device_param(STUD_PARM_POSH)) {
		/* we wrapped around, try it again */
		cd->timestamp = system_time();
		pos = get_device_param(STUD_PARM_POSH);
		tmp = get_device_param(STUD_PARM_POSL);
	}	
	restore_interrupts(cp);
	pos = (pos<<24)|tmp;
	/* we now have a 32 bit position--we need 64 bits */
	/* did the board counter wrap? */
	if ( (cd->position & 0x00000000ffffffff) > pos) {
		cd->position &=  0xffffffff00000000;
		cd->position += (0x0000000100000000 + pos);
	}
	else {
		cd->position &= 0xffffffff00000000;
		cd->position += pos;
	}
	/* adjust position returned to match buffer boundary */
	/* as the multinode expects the timestamp at the interrupt */
	/* we should check to see if we missed interrupts....(rev. 2 )*/
	cd->timestamp = cd->timestamp - (((uint64)(cd->position % cd->channel_buff_size)*1000000LL)/(uint64)cd->sample_rate);
	cd->position = cd->position - (cd->position % cd->channel_buff_size);



	cntr = cd->channels[outputs_per_bd].current_buffer;
	for (chan = 0, mask = 1 << outputs_per_bd; chan < inputs_per_bd ; chan++, mask <<= 1) {
#if 1
		if ( mask & cd->channels_enabled ) {
			read_recd_buf( chan,
							cd->channels[outputs_per_bd + chan].index, 
							cd->channels[outputs_per_bd + chan].buf[cntr&1], 
							cd->channel_buff_size,
							STUD_BUF_INT32 ,
							NULL );

		}
#endif
		/* slower, but keeps everything in sync */
		cd->channels[outputs_per_bd + chan].index += cd->channel_buff_size;
 		if (cd->channels[outputs_per_bd + chan].index >= CHNL_BUF_SIZE) {
			cd->channels[outputs_per_bd + chan].index %= CHNL_BUF_SIZE;
		}	
	}
	/* If not started, start (unless we are waiting on start_at) */
	if (!is_playing && cd->preroll == 0) {
//		ddprintf("sonorus: Start Playing\n");
		cd->channels[outputs_per_bd].current_buffer++;
		cd->channels[0].current_buffer++;

		for (chan = 0; chan < outputs_per_bd; chan++) {
			/* enable input monitor */
			set_channel_param( 	STUD_PARM_INPMON,
								chan + cd->ix * outputs_per_bd ,
								0x8000000L / cd->channels[chan].monitor_ramp );
			cd->channels[chan].monitor_ts = system_time();
			/* enable output enable */
			set_channel_param( 	STUD_PARM_OUTENB,
								chan + cd->ix * outputs_per_bd ,
								0x8000000L / cd->channels[chan].output_ramp );
			cd->channels[chan].output_ts = system_time();
		}
		/* enable record enable */
		set_device_param( STUD_PARM_RECENB, cd->record_enable);					
		if (!cd->start_time) {
			set_play_enbl( true );
	
			/* an event has transpired, put in the queue */
			if ( cd->current_mask & B_MULTI_EVENT_STARTED ) {
				if ( cd->event_count >= MAX_EVENTS){
					ddprintf("sonorus: out of event space\n");
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
	}
	
	cntr = cd->channels[outputs_per_bd].current_buffer;

	if (cd->preroll < 1 ) { /*was 2 */
	 	pmbi->record_buffer_cycle = -1;
		if (cd->preroll == 0) { /* was 1 */
			/* record index must be a buffer behind playback */
			/* if you preload the playback and want low latency  */
			for (chan = 0; chan < inputs_per_bd ; chan++) {
				cd->channels[outputs_per_bd + chan].index =
					(cd->channels[0].index - (1*cd->channel_buff_size)) % CHNL_BUF_SIZE; /*was 2*/
			}
		}
		cd->preroll++;	
	}
	else {
		pmbi->record_buffer_cycle = cntr & 1;
	}
	pmbi->playback_buffer_cycle = cntr & 1;


	/* pmbi->flags */
	pmbi->played_real_time		= cd->timestamp;
	pmbi->played_frames_count	= cd->position;
	pmbi->recorded_real_time	= cd->timestamp;
	pmbi->recorded_frames_count	= cd->position;

	return B_OK;
}

static multi_mode_info mode_info[] = {
{	SONORUS_MODE_1111,
	0,
	"Sonorus 1111 (ADAT & SPDIF)",
	10,
	10,
	48000.0,
	48000.0,
	B_FMT_24BIT | B_FMT_IS_GLOBAL,
	B_FMT_24BIT | B_FMT_IS_GLOBAL | B_FMT_SAME_AS_INPUT,
	{}
}
};	
status_t
sonorus_multi_list_modes(
				void * cookie,
				void * data,
				size_t len)

{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mode_list * pmml = (multi_mode_list *) data;
	int32 max;
	
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
sonorus_multi_get_mode(
				void * cookie,
				void * data,
				size_t len)

{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	status_t err = B_OK;
	if (data != NULL) {
		(*(int32 *) data) = SONORUS_MODE_1111;
	}
	else {
		err = B_BAD_VALUE;
	}		  
	return err;
}

status_t
sonorus_multi_set_mode(
				void * cookie,
				void * data,
				size_t len)

{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	
	if ( data == NULL || *(int32 *) data != SONORUS_MODE_1111) {
		return B_BAD_VALUE;
	}
	return B_OK;
}

status_t
sonorus_multi_list_extensions(
				void * cookie,
				void * data,
				size_t len)

{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_extension_list * pmel = (multi_extension_list *) data;
	
	if (pmel == NULL || pmel->info_size < sizeof(multi_extension_list)){
		return B_BAD_VALUE;
	}	
	pmel->info_size = sizeof(multi_extension_list);
	pmel->actual_count = 0;
	return B_OK;
}

status_t
sonorus_multi_get_extension(
				void * cookie,
				void * data,
				size_t len)

{
	return B_NOT_ALLOWED;
}

status_t
sonorus_multi_set_extension(
				void * cookie,
				void * data,
				size_t len)

{
	return B_NOT_ALLOWED;
}

status_t
sonorus_multi_get_event_info(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_get_event_info * pgei = (multi_get_event_info *) data;
	
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
sonorus_multi_set_event_info(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_set_event_info * psei = (multi_set_event_info *) data;
	
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
sonorus_multi_get_event(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_get_event * pge = (multi_get_event *) data;
	uint32 i;
	/* sleazy hack, very inefficient--we need to use linked lists.... */
	
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
	sonorus_dev * cd= (sonorus_dev *) (tp->cookie);
	status_t err;

	/* need a semaphore.... */

	/* if timer expired in the middle of force_stop */
	/* we can still kill the start................. */
	if (cd->start_time) {
		set_play_enbl(true);
		cd->start_time = (bigtime_t) 0;
		
		/* an event has transpired, put in the queue */
		if ( cd->current_mask & B_MULTI_EVENT_STARTED ) {
			if ( cd->event_count >= MAX_EVENTS){
				ddprintf("sonorus: out of event space\n");
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
sonorus_multi_set_start_time(
				void * cookie,
				void * data,
				size_t len)

{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	bigtime_t start_time;	

	if ( data == NULL ) {
		return B_BAD_VALUE;
	}
	start_time = * (bigtime_t *) data;
	if (system_time() < start_time && !is_playing) {
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

