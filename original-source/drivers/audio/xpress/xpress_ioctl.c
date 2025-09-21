/* includes */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

/* for floor and ceil */
#include <math.h>

#include "xpress.h"

/* globals */
extern xpress_dev cards[MAX_XPRESS_BD];
extern pci_module_info	*pci;
extern int card_count;
extern game_mixer_control xxx_controls[X_NUM_CONTROLS];
extern game_get_mixer_level_info xxx_level_info[X_NUM_LEVELS];
extern game_get_mixer_mux_info xxx_mux_info[X_NUM_MUXES];
extern game_mux_item xxx_mux_items[X_NUM_MUX_ITEMS];
extern game_get_mixer_enable_info xxx_enable_info[X_NUM_ENABLES];


/* xpress_stream.c */
status_t lock_stream(open_xpress * );
void     unlock_stream(open_xpress * );
status_t allocate_stream(void * ,void *);
status_t close_stream(void *, void *);
status_t allocate_buffer(void *, void *, uint16 );
status_t queue_buffer(void *, void *);
status_t free_buffer(void *, void *);
status_t run_streams(void *, void *, int );

/* xpress_mix.c */
inline int16 control_to_enable_ordinal( uint16 );
inline int16 control_to_level_ordinal( uint16 );
inline int16 control_to_mux_ordinal( uint16 );
inline int16 control_mux_to_mux_item( uint16 );
status_t get_control_value(open_xpress *, game_mixer_control_value *);
status_t set_control_value(open_xpress *, game_mixer_control_value *);

/* xpress_ac97glue.c */
status_t xpress_cached_codec_write( void *, uchar, uint16, uint16 );
void	 lock_ac97(xpress_dev *, uchar);
void	 unlock_ac97(xpress_dev *, uchar);

/* the functions... */
status_t
xpress_game_open_stream(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_open_stream * pgos = (game_open_stream *) data;
	status_t err = B_OK;

	if (pgos == NULL){
		return B_BAD_VALUE;
	}
	
	/* "validate" the struct */
	if (pgos->info_size < sizeof(game_open_stream)) {
		err = B_BAD_VALUE;
	}
	pgos->info_size = sizeof(game_open_stream);

	if (err == B_OK) {
		if ((!GAME_IS_DAC(pgos->adc_dac_id)) && (!GAME_IS_ADC(pgos->adc_dac_id))) {
			err = B_BAD_VALUE;
		}
		if ((GAME_IS_DAC(pgos->adc_dac_id)) &&
			(GAME_DAC_ORDINAL(pgos->adc_dac_id) >= X_NUM_DACS)){
			err = B_BAD_VALUE;
		}
		if ((GAME_IS_ADC(pgos->adc_dac_id)) &&
			(GAME_ADC_ORDINAL(pgos->adc_dac_id)>= X_NUM_ADCS)){
			err = B_BAD_VALUE;
		}
	}

	/* allocate stream */
	if (err == B_OK) {
		/* is there a stream available? */
		lock_stream(card);
		err = allocate_stream(card, pgos);
		unlock_stream(card);
	}
	return err;
							
}

status_t
xpress_game_queue_stream_buffer(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_queue_stream_buffer * pgqsb = (game_queue_stream_buffer *) data;
	status_t err = B_OK;

	if (pgqsb == NULL){
		return B_BAD_VALUE;
	}
	
	/*	validate the struct...*/
	if (pgqsb->info_size < sizeof(game_queue_stream_buffer)) {
		pgqsb->info_size = sizeof(game_queue_stream_buffer);
		err = B_BAD_VALUE;
	}
	
	/* I want the lock here because close stream could invalidate the stream */
	lock_stream(card);
	
		/* is the buffer bound to a different stream? */
		if ( card->device->buffer_data[GAME_BUFFER_ORDINAL(pgqsb->buffer)].stream != GAME_NO_ID  && 
			 card->device->buffer_data[GAME_BUFFER_ORDINAL(pgqsb->buffer)].stream != pgqsb->stream ) {
				err = B_BAD_VALUE;
		}
		/* is the stream valid? */
		if (pgqsb->stream == GAME_NO_ID || !GAME_IS_STREAM(pgqsb->stream)) {
			ddprintf(("stream not valid: %d\n", pgqsb->stream));
			err = B_BAD_VALUE;
		}
		/* if not opened, looping, or the stream is closing, it's an error */
		if (!(card->device->stream_data[GAME_STREAM_ORDINAL(pgqsb->stream)].my_flags & X_ALLOCATED)) {
				err = B_ERROR;
		}
		if ( (card->device->stream_data[GAME_STREAM_ORDINAL(pgqsb->stream)].my_flags &
																(X_CLOSING ))) {
				err = EBUSY;
		}

//	dprintf("Q");
		/* fix the flag check here */
//		if (pgqsb->flags && (pgqsb->flags != GAME_RELEASE_SEM_WHEN_DONE)) {
//			err = B_BAD_VALUE;
//		}

		if ((pgqsb->frame_count << 2) > card->device->buffer_data[GAME_BUFFER_ORDINAL(pgqsb->buffer)].byte_size) {
			ddprintf(("xpress: attempt to queue %ld more data than was allocated %ld.\n",pgqsb->frame_count << 2,card->device->buffer_data[GAME_BUFFER_ORDINAL(pgqsb->buffer)].byte_size));
			err = B_BAD_VALUE;
		}

		if ((pgqsb->flags & GAME_BUFFER_PING_PONG) && (pgqsb->flags & GAME_BUFFER_LOOPING)) {
			if ((pgqsb->page_frame_count << 2) < X_MIN_BUFF_ALLOC ) { 
			ddprintf(("xpress: page_frame_count < minimum buffer allocation\n"));
			err = B_BAD_VALUE;
			}
			if ( pgqsb->frame_count % pgqsb->page_frame_count ) {
			/* this will change in the next? rev. if not a multiple, you may need to ignore */
			/* the next to last semaphore release and just release at the end of buffer.... */ 
			ddprintf(("xpress: page_frame_count not an even multiple of frame_count\n"));
			err = B_BAD_VALUE;
			}
		}	

		if ((pgqsb->frame_count << 2) < X_MIN_BUFF_ALLOC) {
			ddprintf(("xpress: Give me a bigger one, you bad boy.\n"));
			err = B_BAD_VALUE;
		}

		/* if everything passed, call queue_buffer */
		if (err == B_OK) {
			err = queue_buffer(card, pgqsb);
		}
	
	unlock_stream(card);
	/* set get_last_time here */
//	dprintf("xpress: queue buffer needs get_last_time here if flag set\n");
		
	return err;
}

status_t
xpress_game_run_streams(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_run_streams * pgrs = (game_run_streams *) data;
	status_t err = B_OK;

	if (pgrs == NULL){
		return B_BAD_VALUE;
	}
	
	/*	validate the struct...*/
	if (pgrs->info_size < sizeof(game_run_streams)) {
		err = B_BAD_VALUE;
	}
	pgrs->info_size = sizeof(game_run_streams);

	lock_stream(card);
		if (!pgrs->in_stream_count) {
			err = B_BAD_VALUE;
		}
		
		if (err == B_OK) {
			int i, j = 0;
	
			for (i = 0; i < pgrs->in_stream_count; i++) {
				err = run_streams(card, pgrs, i);
				if (err == B_OK) {
					j++;
				}
			}
			pgrs->out_actual_count = j;
			if (j) {
				err = B_OK;
			}	
		}

	unlock_stream(card);
	
	return err;
							
}

status_t
xpress_game_close_stream(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_close_stream * pgcs = (game_close_stream *) data;
	status_t err = B_OK;

	if (pgcs == NULL){
		return B_BAD_VALUE;
	}
	
	/*validate the struct...*/
	if (pgcs->info_size < sizeof(game_close_stream)) {
		err = B_BAD_VALUE;
	}

	pgcs->info_size = sizeof(game_close_stream);

	if (pgcs->stream == GAME_NO_ID || !GAME_IS_STREAM(pgcs->stream)){
		err = B_BAD_VALUE;
	}	
	/* stop all other requests for this stream by locking the stream */	
	lock_stream(card);
		{
			/* don't close twice.... */
			struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgcs->stream)]);
			if ((psd->my_flags & X_CLOSING) || !(psd->my_flags & X_ALLOCATED)) {
				err = B_ERROR;
			}
			
			/* set the closing flag */
			psd->my_flags |= X_CLOSING;			
		}	
	unlock_stream(card);		
		
		if (err == B_OK) {
			/* find and close the stream */
			err = close_stream(card, pgcs);
		}

	/* the stream is now all closed, go   */
	/* ahead and remove the closing flag  */
	/* as well as zeroing everything else */
	lock_stream(card);
		{
			struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgcs->stream)]);
			/* clear all flags */
			psd->my_flags = X_DMA_STOPPED;
			psd->state = GAME_STREAM_STOPPED;
			psd->bind_count = 0;
			psd->prd_add = 0;
			psd->prd_remove = 0;
			psd->gos_stream_sem = B_ERROR;
			psd->gos_adc_dac_id = GAME_NO_ID;
			psd->stream_buffer_queue = NULL;
			psd->free_list = NULL;
			/* leave the all_done semaphore alone..... */
			psd->cookie = NULL;
		}	
	unlock_stream(card);		

	ddprintf(("xpress: close_stream always uses BLOCK_UNTIL_COMPLETE..\n"));
	return err;
							
}

status_t
xpress_game_open_stream_buffer(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_open_stream_buffer * pgosb = (game_open_stream_buffer *) data;
	xpress_dev * pdev  = card->device;
	struct _stream_data * psd = NULL; 
	int16 idx = 0;
	status_t err = B_OK;

	if (pgosb == NULL){
		ddprintf(("xpress: gosb null ptr\n"));
		return B_BAD_VALUE;
	}
	
	/* validate the struct... */
	if (pgosb->info_size < sizeof(game_open_stream_buffer)) {
		ddprintf(("xpress: header size mismatch\n"));
		err = B_BAD_VALUE;
	}
	pgosb->info_size = sizeof(game_open_stream_buffer);

	if (err == B_OK) {
		if (pgosb->byte_size < X_MIN_BUFF_ALLOC) {
			ddprintf(("xpress: allocation too small\n"));
//			err = B_BAD_VALUE;
		}
	}
	
	if (err == B_OK) {
		/* if there is a stream id, is it valid (opened)?   */
		/* anytime you touch stream data (incl. dac and buf */
		/* you need to lock the resources....               */
		lock_stream(card);
		{
			/* Are there more buffers available? */
			if (pdev->current_buffer_count >= X_MAX_BUFFERS) {
				ddprintf(("xpress: no more buffers\n"));
				err = ENOMEM;
			}
	
			if ((GAME_IS_STREAM(pgosb->stream)) && (pgosb->stream != GAME_NO_ID) && (err == B_OK)) {
				psd = &(pdev->stream_data[GAME_STREAM_ORDINAL(pgosb->stream)]);
				/* not opened or closing is an error */
				if (!(psd->my_flags & X_ALLOCATED) || (psd->my_flags & X_CLOSING) ) {
					ddprintf(("xpress: closing/not opened\n"));
					err = B_ERROR;
				}
				else {
					if (psd->bind_count == X_MAX_STREAM_BUFFERS) {
						/* no more buffers */
						ddprintf(("xpress: no more stream buffers\n"));
						err = ENOMEM;
					}
				} 
			}
			
			if (err == B_OK) {
				err = allocate_buffer(card, pgosb, idx);
			}
		}
		unlock_stream(card);
	}

	if (err != B_OK) {
		/* could this overflow if info_size is too small? */
		pgosb->area   = B_ERROR;
		pgosb->buffer = GAME_NO_ID;
	}
	return err;
							
}

status_t
xpress_game_close_stream_buffer(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_close_stream_buffer * pgcsb = (game_close_stream_buffer *) data;
	status_t err = B_OK;

	if (pgcsb == NULL){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgcsb->info_size < sizeof(game_close_stream_buffer)) {
		ddprintf(("xpress: close_stream_buffer header mismatch\n"));
		err = B_BAD_VALUE;
	}
	pgcsb->info_size = sizeof(game_close_stream_buffer);
	
	if (err == B_OK) {
		lock_stream(card);
			/* more validation */
			/* check the buffer id */
			if ((pgcsb->buffer == GAME_NO_ID || !GAME_IS_BUFFER(pgcsb->buffer)) ) {
				ddprintf(("xpress: close_stream_buffer buffer has no id or is not a buffer\n"));
				err = B_BAD_VALUE;
			}
			/* validate the stream... */
			else 
			{
				struct _buffer_data * pbd = &(card->device->buffer_data[GAME_BUFFER_ORDINAL(pgcsb->buffer)]);
				
				if (pbd->stream != GAME_NO_ID) {
					if ( !GAME_IS_STREAM(pbd->stream)) {
						ddprintf(("xpress: close_stream_buffer stream has no id or is not a stream\n"));
						err = B_BAD_VALUE;
					}
					else {
						struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pbd->stream)]);
						if ( (psd->my_flags & X_CLOSING) || !(psd->my_flags & X_ALLOCATED)) {
							ddprintf(("xpress: close_stream_buffer closing or stream is not allocated\n"));
							err = B_ERROR;
						}	
					}
				}
			}

			if (err == B_OK) {
				err = free_buffer(card, pgcsb);
			}
		unlock_stream(card);
	}
	return err;
							
}

status_t
xpress_game_get_info(
							void * cookie,
							void * data,
							size_t len)
{
	//open_xpress * card = (open_xpress *)cookie;
	game_get_info * pggi = (game_get_info *) data;
	status_t err = B_OK;

	if (pggi == NULL){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pggi->info_size < sizeof(game_get_info)) {
		err = B_BAD_VALUE;
	}
	pggi->info_size = sizeof(game_get_info);
	
	if (err == B_OK) {
		strcpy(pggi->name, X_MODEL_NAME);
		strcpy(pggi->vendor, X_MANUF_NAME);
		pggi->ordinal = 0; /* this should not be hardcoded */
		pggi->version = X_VERSION;
		pggi->dac_count = X_NUM_DACS;
		pggi->adc_count = X_NUM_ADCS;
		pggi->mixer_count=X_NUM_MIXERS;
		pggi->buffer_count=X_MAX_STREAM_BUFFERS;
	}
	return err;
							
}

status_t
xpress_game_get_dac_infos(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_dac_infos * pggdi = (game_get_dac_infos *) data;
	status_t err = B_OK;

	if (pggdi == NULL){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pggdi->info_size < sizeof(game_get_dac_infos)) {
		err = B_BAD_VALUE;
	}
	pggdi->info_size = sizeof(game_get_dac_infos);
	pggdi->out_actual_count = 0;
	if (err == B_OK) {
		int16 i;
//dprintf("xpress: dac info request count %d\n",pggdi->in_request_count);				
		for(i = 0; i < pggdi->in_request_count; i++) {
			int16 di = pggdi->info[i].dac_id;
			memset(&(pggdi->info[i]), 0, sizeof(struct game_dac_info));
			if (GAME_IS_DAC(di) && (GAME_DAC_ORDINAL(di) < X_NUM_DACS)) {
				pggdi->info[i] = card->device->dac_info[GAME_DAC_ORDINAL(di)];
				pggdi->out_actual_count++;
			}
			else {
				pggdi->info[i].dac_id = GAME_NO_ID;
			}
		}
	}
	return err;
							
}


status_t
xpress_game_get_adc_infos(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_adc_infos * pggai = (game_get_adc_infos *) data;
	status_t err = B_OK;

	if (pggai == NULL){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pggai->info_size < sizeof(game_get_adc_infos)) {
		err = B_BAD_VALUE;
	}
	pggai->info_size = sizeof(game_get_adc_infos);
	pggai->out_actual_count = 0;
	if (err == B_OK) {
		int16 i;
		for(i = 0; i < pggai->in_request_count; i++) {
			int16 di = pggai->info[i].adc_id;
				memset(&(pggai->info[i]), 0, sizeof(struct game_adc_info));
			if (GAME_IS_ADC(di) && (GAME_ADC_ORDINAL(di) < X_NUM_ADCS)) {
				pggai->info[i] = card->device->adc_info[GAME_ADC_ORDINAL(di)];
				pggai->out_actual_count++;
			}
			else {
				pggai->info[i].adc_id = GAME_NO_ID;
			}
		}
	}
	return err;
							
}

status_t
xpress_game_set_codec_formats(
							void * cookie,
							void * data,
							size_t length)
{
	open_xpress * card = (open_xpress *)cookie;
	game_set_codec_formats * pgscf = (game_set_codec_formats *)data;
	status_t err = B_OK;
	
	/* validate */
	if (pgscf == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgscf->info_size < sizeof(game_set_codec_formats)) {
		err = B_BAD_VALUE;
	}
	
	pgscf->info_size = sizeof(game_set_codec_formats);
	pgscf->out_actual_count = 0;
	if (err == B_OK) {
		int16 i;
		uint16 tmp;

		for(i = 0; i < pgscf->in_request_count; i++) {
			int16 ci = pgscf->formats[i].codec;
			game_codec_format * pgcf = &(pgscf->formats[i]);

			/* is this a dac or an adc? */
			if (GAME_IS_DAC(ci) && (GAME_DAC_ORDINAL(ci) < X_NUM_DACS)) {
				/* There is only 1 dac in this version, so dac is valid */
				game_dac_info * pdac = &(card->device->dac_info[GAME_DAC_ORDINAL(ci)]);
				KASSERT((GAME_DAC_ORDINAL(ci) == 0));

				if (pgcf->flags & GAME_CODEC_SET_CHANNELS) {
					/* destructive doesn't matter because we only support 0x2 (stereo) */
					if (pgcf->channels != 0x2 ) {
						pgcf->out_status = B_BAD_VALUE;
						continue;
					}
					else {
						pdac->cur_channel_count = pgcf->channels;
					}
				}				
				if (pgcf->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT) {
					/* destructive shouldn't matter... */
					if (pgcf->chunk_frame_count < pdac->min_chunk_frame_count ||
						pgcf->chunk_frame_count > pdac->max_chunk_frame_count ||
						pgcf->chunk_frame_count & 0x3 ) {
						pgcf->out_status = B_BAD_VALUE;
						continue;
					}
					else {
						pdac->cur_chunk_frame_count = pgcf->chunk_frame_count;
					}
				}				
				if (pgcf->flags & GAME_CODEC_SET_FORMAT) {
					/* destructive doesn't matter because we only support 16 bit */
					if (pgcf->format != B_FMT_16BIT ) {
						pgcf->out_status = B_BAD_VALUE;
						continue;
					}
					else {
						pdac->cur_format = pgcf->format;
					}
				}				
				if (pgcf->flags & GAME_CODEC_SET_FRAME_RATE) {
					/* fully functional feature */
					if ( (pgcf->frame_rate & B_SR_CVSR) ) {
						/* use the cvsr field */
						/* validate */
						if ( (pgcf->cvsr < 8000.0) || (pgcf->cvsr > 48000.0) ||
							 (!(pgcf->flags & GAME_CODEC_CLOSEST_OK) && (ceil(pgcf->cvsr) - floor(pgcf->cvsr) != 0.0)) ) {
							pgcf->out_status = B_BAD_VALUE;
							continue;
						}
						/* now set sample rate */
						xpress_cached_codec_write(&(card->device->xhost[0]), 0x2c, (uint16)pgcf->cvsr, 0xffff );	/* dac */
						ddprintf(("setting the dac sample rate to 0x%x\n",(uint16)pgcf->cvsr));

						/* update the sample rate */
						/* worst case: a second opener changes the sample rate  */
						/* while we are reading. Our read gets messed up and we */
						/* return a bad value. It's not an issue, but to be sure*/
						/* we lock.                                             */
						lock_ac97(card->device, card->device->xhost[0].index);
							pdac->cur_cvsr = (float) card->device->ccc[0x2c]; /* dac */
							tmp = card->device->ccc[0x2a]; /* cvsr */
						unlock_ac97(card->device, card->device->xhost[0].index);
						if (!(tmp & 0x1)) {
							xpress_cached_codec_write(&(card->device->xhost[0]), 0x2a, 0x1, 0xffff );	/* turn cvsr on */
						}
						KASSERT((pdac->cur_cvsr == pgcf->cvsr));
					}
					else {
						/* it's an error */
						pgcf->out_status = B_BAD_VALUE;
						continue;
					}
				}				
				pgcf->out_status = B_OK;
				pgscf->out_actual_count++;
			}
			else {
				if (GAME_IS_ADC(ci) && (GAME_ADC_ORDINAL(ci) < X_NUM_ADCS)) {
					/* There is only 1 adc in this version, so adc is valid */
					game_adc_info * padc = &(card->device->adc_info[GAME_ADC_ORDINAL(ci)]);
					KASSERT((GAME_ADC_ORDINAL(ci) == 0));
	
					if (pgcf->flags & GAME_CODEC_SET_CHANNELS) {
						/* destructive doesn't matter because we only support 0x2 (stereo) */
						if (pgcf->channels != 0x2 ) {
							pgcf->out_status = B_BAD_VALUE;
							continue;
						}
						else {
							padc->cur_channel_count = pgcf->channels;
						}
					}				
					if (pgcf->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT) {
						/* destructive shouldn't matter... */
						if (pgcf->chunk_frame_count < padc->min_chunk_frame_count ||
							pgcf->chunk_frame_count > padc->max_chunk_frame_count ||
							pgcf->chunk_frame_count & 0x3 ) {
							pgcf->out_status = B_BAD_VALUE;
							continue;
						}
						else {
							padc->cur_chunk_frame_count = pgcf->chunk_frame_count;
						}
					}				
					if (pgcf->flags & GAME_CODEC_SET_FORMAT) {
						/* destructive doesn't matter because we only support 16 bit */
						if (pgcf->format != B_FMT_16BIT ) {
							pgcf->out_status = B_BAD_VALUE;
							continue;
						}
						else {
							padc->cur_format = pgcf->format;
						}
					}				
					if (pgcf->flags & GAME_CODEC_SET_FRAME_RATE) {
						/* fully functional feature */
						if ( (pgcf->frame_rate & B_SR_CVSR) ) {
							/* use the cvsr field */
							/* validate */
							if ( (pgcf->cvsr < 8000.0) || (pgcf->cvsr > 48000.0) ||
								 (!(pgcf->flags & GAME_CODEC_CLOSEST_OK) && (ceil(pgcf->cvsr) - floor(pgcf->cvsr) != 0.0)) ) {
								pgcf->out_status = B_BAD_VALUE;
								continue;
							}
							/* now set sample rate */
							xpress_cached_codec_write(&(card->device->xhost[0]), 0x32, (uint16)pgcf->cvsr, 0xffff );	/* adc */
							ddprintf(("setting the adc sample rate to 0x%x\n",(uint16)pgcf->cvsr));
	
							/* update the sample rate */
							/* worst case: a second opener changes the sample rate  */
							/* while we are reading. Our read gets messed up and we */
							/* return a bad value. It's not an issue, but to be sure*/
							/* we lock.                                             */
							lock_ac97(card->device, card->device->xhost[0].index);
								padc->cur_cvsr = (float) card->device->ccc[0x32]; /* adc */
								tmp = card->device->ccc[0x2a]; /* cvsr */
							unlock_ac97(card->device, card->device->xhost[0].index);
							if (!(tmp & 0x1)) {
								xpress_cached_codec_write(&(card->device->xhost[0]), 0x2a, 0x1, 0xffff );	/* turn cvsr on */
							}
							KASSERT((padc->cur_cvsr == pgcf->cvsr));
						}
						else {
							/* it's an error */
							pgcf->out_status = B_BAD_VALUE;
							continue;
						}
					}				
					pgcf->out_status = B_OK;
					pgscf->out_actual_count++;
				}
				else {
					pgscf->formats[i].codec = GAME_NO_ID;
				}
			}	
		}
		
	}
	return err;
}

status_t
xpress_game_get_mixer_infos(
							void * cookie,
							void * data,
							size_t length)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_infos * pgmi = (game_get_mixer_infos * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (pgmi == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmi->info_size < sizeof(game_get_mixer_infos)) {
		err = B_BAD_VALUE;
	}

	pgmi->info_size = sizeof(game_get_mixer_infos);
	pgmi->out_actual_count = 0;

	if (err == B_OK) {
		if (!pgmi->in_request_count) {
			pgmi->out_actual_count = X_NUM_MIXERS;
		}
		else {
			int16 i;
			for (i = 0; i <  pgmi->in_request_count; i++) {

				int16 mi = pgmi->info[i].mixer_id;
				memset(&(pgmi->info[i]), 0, sizeof(struct game_mixer_info));
				if (GAME_IS_MIXER(mi) && (GAME_MIXER_ORDINAL(mi) < X_NUM_MIXERS)) {
					pgmi->info[i] = card->device->mixer_info[GAME_MIXER_ORDINAL(mi)];
					pgmi->out_actual_count++;
				}
				else {
					pgmi->info[i].mixer_id = GAME_NO_ID;
				}
			}	
		}
	}
	return err;
}


status_t
xpress_game_get_mixer_controls(
								void * cookie,
								void * data,
								size_t length)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_controls * pgmc = (game_get_mixer_controls * ) (data);
	uint16 ordinal;
	uint16 max_ctl;
	status_t err = B_OK;
	
	/* validate */
	if (pgmc == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmc->info_size < sizeof(game_get_mixer_controls)) {
		err = B_BAD_VALUE;
	}

	pgmc->info_size = sizeof(game_get_mixer_controls);
	pgmc->out_actual_count = 0;

	if (!GAME_IS_MIXER(pgmc->mixer_id)) {
		err = B_BAD_VALUE;
	}
	else {
		ordinal = GAME_MIXER_ORDINAL(pgmc->mixer_id);
		max_ctl = ordinal ? X_NUM_DAC_CONTROLS : X_NUM_ADC_CONTROLS;
		if ((pgmc->in_request_count + pgmc->from_ordinal) > max_ctl) {
			err = B_BAD_VALUE;
		}
	}

	if (err == B_OK) {
		if (!pgmc->in_request_count) {
			pgmc->out_actual_count = max_ctl;
		}
		else {
			int16 i;
			game_mixer_control * pcontrol = ordinal  ? 	&(xxx_controls[X_NUM_ADC_CONTROLS + pgmc->from_ordinal]) :
														&(xxx_controls[pgmc->from_ordinal]);
			for (i = 0; i < pgmc->in_request_count; i++) {
				memset(&(pgmc->control[i]), 0, sizeof(game_mixer_control));
				pgmc->control[i] = pcontrol[i];
				pgmc->out_actual_count++;
			}	
		}
	}
	return err;
}

status_t xpress_game_get_mixer_level_info( 
										void * cookie,
										void * data,
										size_t length)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_level_info * pgmli = (game_get_mixer_level_info * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (pgmli == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmli->info_size < sizeof(game_get_mixer_level_info)) {
		err = B_BAD_VALUE;
	}

	pgmli->info_size = sizeof(game_get_mixer_level_info);

	if (!GAME_IS_CONTROL(pgmli->control_id)) {
		pgmli->control_id = GAME_NO_ID;
		err = B_BAD_VALUE;
	}

	if (err == B_OK) {
		int16 i = control_to_level_ordinal(pgmli->control_id );
		if (i >= 0 && i < X_NUM_LEVELS) {
			*pgmli = xxx_level_info[i];
		}
		else {
			pgmli->control_id = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
	}

	return err;
}

status_t
xpress_game_get_mixer_mux_info(
								void * cookie,
								void * data,
								size_t length)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_mux_info * pgmmi = (game_get_mixer_mux_info * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (pgmmi == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmmi->info_size < sizeof(game_get_mixer_mux_info)) {
		err = B_BAD_VALUE;
	}

	pgmmi->info_size = sizeof(game_get_mixer_mux_info);

	if (!GAME_IS_CONTROL(pgmmi->control_id)) {
		pgmmi->control_id = GAME_NO_ID;
		err = B_BAD_VALUE;
	}

	if (err == B_OK) {
		int16 i = control_to_mux_ordinal(pgmmi->control_id );
		int16 j = control_mux_to_mux_item(pgmmi->control_id);
		if (i >= 0 && i < X_NUM_MUXES ) {
			/* save important ones */
			uint16 in_req = pgmmi->in_request_count;
			game_mux_item * pgmi = pgmmi->items;
			
			if (!in_req) {
				pgmmi->out_actual_count = xxx_mux_info[i].out_actual_count;
			}
			else {
				*pgmmi = xxx_mux_info[i];
				/* add in trashed and other items */
				pgmmi->in_request_count = in_req;
				pgmmi->out_actual_count = min(in_req, pgmmi->out_actual_count);
				pgmmi->items = pgmi;
				memcpy(pgmmi->items, &xxx_mux_items[j], pgmmi->out_actual_count * sizeof(game_mux_item));
			}		
		}
		else {
			pgmmi->control_id = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
	}
	return err;
}

status_t
xpress_game_get_mixer_enable_info(
									void * cookie,
									void * data,
									size_t length)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_enable_info * pgmei = (game_get_mixer_enable_info * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (pgmei == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmei->info_size < sizeof(game_get_mixer_enable_info)) {
		err = B_BAD_VALUE;
	}

	pgmei->info_size = sizeof(game_get_mixer_enable_info);

	if (!GAME_IS_CONTROL(pgmei->control_id)) {
		pgmei->control_id = GAME_NO_ID;
		err = B_BAD_VALUE;
	}

	if (err == B_OK) {
		int16 i = control_to_enable_ordinal(pgmei->control_id );
		if (i >= 0 && i < X_NUM_ENABLES) {
			*pgmei = xxx_enable_info[i];
		}
		else {
			pgmei->control_id = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
	}

	return err;
}

status_t
xpress_game_get_mixer_control_values(
									void * cookie,
									void * data,
									size_t length)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_mixer_control_values * pgmcv = (game_get_mixer_control_values * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (pgmcv == NULL){
		return B_BAD_VALUE;
	}
	
	if (pgmcv->info_size < sizeof(game_get_mixer_control_values)) {
		err = B_BAD_VALUE;
	}

	pgmcv->info_size = sizeof(game_get_mixer_control_values);
	pgmcv->out_actual_count = 0;

	if (err == B_OK) {
		if (!pgmcv->in_request_count) {
			pgmcv->out_actual_count = X_NUM_CONTROLS;
		}
		else {
			int16 i;
			for (i = 0; i < pgmcv->in_request_count; i++) {
				if (get_control_value(card, &(pgmcv->values[i])) == B_OK) {
					pgmcv->out_actual_count++;
				}
				else {
					err = B_BAD_VALUE;
				}
			}	
		}
	}
	return err;
}

status_t
xpress_game_set_mixer_control_values(
									void * cookie,
									void * data,
									size_t length)
{
	open_xpress * card = (open_xpress *)cookie;
	game_set_mixer_control_values * psmcv = (game_set_mixer_control_values * ) (data);
	status_t err = B_OK;
	
	/* validate */
	if (psmcv == NULL){
		return B_BAD_VALUE;
	}
	
	if (psmcv->info_size < sizeof(game_set_mixer_control_values)) {
		err = B_BAD_VALUE;
	}

	psmcv->info_size = sizeof(game_set_mixer_control_values);
	psmcv->out_actual_count = 0;

	if (err == B_OK) {
		if (!psmcv->in_request_count) {
			psmcv->out_actual_count = X_NUM_CONTROLS;
		}
		else {
			int16 i;
			for (i = 0; i < psmcv->in_request_count; i++) {
				if (set_control_value(card, &(psmcv->values[i])) == B_OK) {
					psmcv->out_actual_count++;
				}
				else {
					err = B_BAD_VALUE;
				}
			}	
		}
	}
	return err;
}

status_t
xpress_game_get_stream_timing(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_stream_timing * pgst = (game_get_stream_timing *) data;
	uint32 frame_cvsr = 0;
	struct _stream_data * psd = NULL;
	status_t err = B_OK;

	if ((pgst == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgst->info_size < sizeof(game_get_stream_timing) ) {
		err = B_BAD_VALUE;
	}
	pgst->info_size = sizeof(game_get_stream_timing);
	
	if (err == B_OK) {
		/* is the stream valid? */
		if (pgst->stream == GAME_NO_ID || !GAME_IS_STREAM(pgst->stream)) {
			pgst->stream = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
		else {
			psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgst->stream)]);
		}
	}
	
	if (err == B_OK ) {	
		if (!(psd->my_flags & X_ALLOCATED)) {
			pgst->stream = GAME_NO_ID;
			err = B_ERROR;
		}
	}
	
	if (err == B_OK) {
		/* some more validation */
		if( GAME_IS_DAC(psd->gos_adc_dac_id) ) {
			frame_cvsr = (uint32) card->device->dac_info[GAME_DAC_ORDINAL(psd->gos_adc_dac_id)].cur_cvsr;
		}
		else if ( GAME_IS_ADC(psd->gos_adc_dac_id) ) {
			frame_cvsr = (uint32) card->device->adc_info[GAME_ADC_ORDINAL(psd->gos_adc_dac_id)].cur_cvsr;
		}
		else {
			err = B_ERROR;
		}	
	}
	
	if (err == B_OK) {
		/* frame_count and timestamp are changed in the int handler, */
		/* I should use lock_queue......maybe later  */
		pgst->state = psd->state;
		pgst->timing.cur_frame_rate = frame_cvsr;
		pgst->timing.frames_played  = psd->frame_count;
		pgst->timing.at_time 		= psd->timestamp;
	}
	return err;
							
}

status_t
xpress_game_get_device_state(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_device_state * pgds = (game_get_device_state *) data;
	status_t err = B_OK;

	if ((pgds == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgds->info_size < sizeof(game_get_device_state) ) {
		err = B_BAD_VALUE;
	}
	pgds->info_size = sizeof(game_get_device_state);

	if (err == B_OK) {
		pgds->out_version = 0x534f;
		pgds->out_actual_size = 0;
	}

	return err;
}

status_t
xpress_game_set_device_state(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_set_device_state * psds = (game_set_device_state *) data;
	status_t err = B_OK;

	if ((psds == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (psds->info_size < sizeof(game_set_device_state) ) {
		err = B_BAD_VALUE;
	}
	psds->info_size = sizeof(game_set_device_state);

	/* do nothing for now... */

	return err;
}


status_t
xpress_game_get_interface_info(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_interface_info * pgii = (game_get_interface_info *) data;
	status_t err = B_OK;

	if ((pgii == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgii->info_size < sizeof(game_get_interface_info) ) {
		err = B_BAD_VALUE;
	}
	pgii->info_size = sizeof(game_get_interface_info);

	if (err == B_OK) {
		pgii->actual_size = 0;
		pgii->out_name[0] = 0x0;
		pgii->first_opcode = 0;
		pgii->last_opcode = 0;
	}

	return err;
}


status_t
xpress_game_get_interfaces(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_interfaces * pgi = (game_get_interfaces *) data;
	status_t err = B_OK;

	if ((pgi == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgi->info_size < sizeof(game_get_interfaces) ) {
		err = B_BAD_VALUE;
	}

	if (err == B_OK) {
		pgi->info_size = sizeof(game_get_interfaces);
		pgi->actual_count = 0;
	}
	
	return err;
}

#if 0
status_t
xpress_game_get_stream_description(
							void * cookie,
							void * data,
							size_t len)
{
	open_xpress * card = (open_xpress *)cookie;
	game_get_stream_description * pgsd = (game_get_stream_description *) data;
	status_t err = B_OK;

	if ((pgsd == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgsd->info_size < sizeof(game_get_stream_description) ) {
		err = B_BAD_VALUE;
	}

	/* risky if info_size is 0 */
	pgsd->info_size = sizeof(game_get_stream_description);

	if (err == B_OK) {
		
		/* is the stream valid? */
		if (pgsd->stream == GAME_NO_ID || !GAME_IS_STREAM(pgsd->stream)) {
			pgsd->stream = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
		else
		if (!(card->device->stream_data[GAME_STREAM_ORDINAL(pgsd->stream)].my_flags & X_ALLOCATED)) {
			pgsd->stream = GAME_NO_ID;
			err = B_ERROR;
		}
		else {	
			pgsd->codec_id = card->device->stream_data[GAME_STREAM_ORDINAL(pgsd->stream)].gos_adc_dac_id;
			pgsd->caps = 0;
			pgsd->volume_mapping = 0;
			pgsd->min_volume = 0;
			pgsd->max_volume = 0;
			pgsd->normal_point = 0;
			pgsd->min_volume_db = 0.0;
			pgsd->max_volume_db = 0.0;
		}
	}
	
	return err;
}
#endif

status_t
xpress_game_get_stream_controls(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_get_stream_controls * pgsc = (game_get_stream_controls *) data;
	status_t err = B_OK;

	if ((pgsc == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pgsc->info_size < sizeof(game_get_stream_controls) ) {
		err = B_BAD_VALUE;
	}


	/* risky if info_size is 0 */
	pgsc->info_size = sizeof(game_get_stream_controls);

	if (err == B_OK) {
		pgsc->out_actual_count = 0;
	}
	
	return err;
}

status_t
xpress_game_set_stream_controls(
							void * cookie,
							void * data,
							size_t len)
{
//	open_xpress * card = (open_xpress *)cookie;
	game_set_stream_controls * pssc = (game_set_stream_controls *) data;
	status_t err = B_OK;

	if ((pssc == NULL) ){
		return B_BAD_VALUE;
	}
	
	/* a little bit of validation */
	if (pssc->info_size < sizeof(game_set_stream_controls) ) {
		err = B_BAD_VALUE;
	}


	/* risky if info_size is 0 */
	pssc->info_size = sizeof(game_set_stream_controls);

	if (err == B_OK) {
		pssc->out_actual_count = 0;
		pssc->ramp_time = 0;
	}
	
	return err;
}
