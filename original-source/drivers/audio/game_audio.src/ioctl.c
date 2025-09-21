/************************************************************************/
/*                                                                      */
/*                              ioctl.c 	                            */
/*                      												*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include <KernelExport.h>
#include "sound.h"
//#include "R3MediaDefs.h"
#include "mvp4_debug.h"
#include "gaudio_card.h"
#include "wrapper.h"

#define CHECK_SIZE(type, name)   type * name = (type *) data; 			\
								 if (name->info_size < sizeof(type) ||  \
								    name->info_size == 0)               \
								 {                                      \
								   err = B_ERROR;                       \
								   DB_PRINTF(("Error: %s %s\n", #type, #name)); \
								   name->info_size = sizeof(type);      \
								   return err;                          \
								 }                                      \
								 name->info_size = sizeof(type);        \
								 								 
/* These variables need to be protected against their corresponding functions 
   found in buffer.c, stream.c, queue.c and SGD.c using lock() and unlock() */
extern stream playback_stream,								/* one stream for playback */
              capture_stream;								/* another for capture */
extern audiobuffer *audiobuffers[GAME_MAX_BUFFER_COUNT];	/* allocation table for buffers */
extern SGD playback_SGD,									/* DMA table for playback */
           capture_SGD;										/* and for capture */

/* get info on specific inputs/outputs */								 
status_t get_info_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
	int i;
	CHECK_SIZE(game_get_info, g_get_info);
			
	strcpy(g_get_info->name, "mvp4g");
	strcpy(g_get_info->vendor , "via");
	g_get_info->ordinal = 0;
	g_get_info->version = 1;
	g_get_info->dac_count = 1;
	g_get_info->adc_count = 1;
	g_get_info->mixer_count = 1;
	g_get_info->buffer_count = 8;
	
	return err;
	
}

status_t get_dac_infos_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
    
    CHECK_SIZE(game_get_dac_infos, dac_info);
    // we have only 1 dac in mvp4 card
    acquire_benaphore();
    dac_info->info->dac_id 			 = gaudio->dac_id;
    dac_info->info->linked_adc_id 	 = gaudio->adc_id;
    dac_info->info->linked_mixer_id  = gaudio->mixer_id;
    dac_info->info->frame_rates    	 = gaudio->frame_rate;
	dac_info->info->cur_frame_rate 	 = gaudio->frame_rate;
	dac_info->info->cur_stream_count = gaudio->cur_stream_count;
	dac_info->info->channel_counts   = gaudio->dac_channels;   
	dac_info->info->cur_channel_count= gaudio->dac_channels;
	dac_info->info->formats          = gaudio->dac_format;  
	dac_info->info->cur_format       = gaudio->dac_format;
	release_benaphore();
		     
	// for class 1 and class 2 devices, this value will always be 1
	dac_info->info->max_stream_count = 1;
                       
    dac_info->info->stream_flags = GAME_STREAMS_HAVE_VOLUME |
								   GAME_STREAMS_HAVE_FRAME_RATE |
								   GAME_STREAMS_HAVE_FORMAT |
								   GAME_STREAMS_HAVE_CHANNEL_COUNT;
										   
	dac_info->info->min_chunk_frame_count = 256;
	// 0 for fixed chunk size 
    dac_info->info->chunk_frame_count_increment = 0,
    dac_info->info->max_chunk_frame_count = 16384;
    dac_info->info->cur_chunk_frame_count =	4096;	    	
	strcpy(dac_info->info->name, "mvp4_dac"  );
		    
	dac_info->info->cvsr_max       = 0;
    dac_info->info->designations   = B_CHANNEL_LEFT |
								     B_CHANNEL_RIGHT ;
	dac_info->info->cur_cvsr 	   = 0;
		
									
    // request :  dac_info->in_request_count 
    // but we have only one  DAC   
    dac_info->out_actual_count = 1;
    return err;
}

status_t get_adc_infos_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	
    CHECK_SIZE(game_get_adc_infos, adc_info );

   // we have only 1 adc in mvp4 card
   // if( adc_info->info->adc_id != gaudio->adc_id)
   //  adc_info->info->adc_id = GAME_NO_ID; // illegal id
    acquire_benaphore();
		    
    adc_info->info->adc_id 			 = gaudio->adc_id;
    adc_info->info->linked_dac_id 	 = gaudio->dac_id;
    adc_info->info->linked_mixer_id  = gaudio->mixer_id;
    adc_info->info->cur_stream_count = gaudio->cur_stream_count;
    adc_info->info->frame_rates      = gaudio->frame_rate;
    
    adc_info->info->channel_counts   = gaudio->adc_channels;   
	adc_info->info->cur_channel_count= gaudio->adc_channels;
	adc_info->info->formats          = gaudio->adc_format;  
	adc_info->info->cur_format       = gaudio->adc_format;
		    
    release_benaphore();
		    
    // for class 1 and class 2 devices, this value will always be 1
    adc_info->info->max_stream_count = 1;
		    
		    
            
    adc_info->info->stream_flags = GAME_STREAMS_HAVE_VOLUME |
								   GAME_STREAMS_HAVE_FRAME_RATE |
								   GAME_STREAMS_HAVE_FORMAT |
								   GAME_STREAMS_HAVE_CHANNEL_COUNT;
										   
    adc_info->info->min_chunk_frame_count = 256;
    //0 for fixed chunk size 
    adc_info->info->chunk_frame_count_increment = 0;
    adc_info->info->max_chunk_frame_count = 16384;
    adc_info->info->cur_chunk_frame_count =	4096;	    	
    strcpy(adc_info->info->name,"mvp4_adc");
		    
    adc_info->info->cvsr_max      = 0;
    adc_info->info->designations  = B_CHANNEL_LEFT |
                                    B_CHANNEL_RIGHT ;
	adc_info->info->cur_cvsr = 0;
			    		    
	// request :  adc_info->in_request_count 
	// but we have only one  ADC   
	adc_info->out_actual_count = 1;
	return err;
}

status_t set_codec_formats_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
    CHECK_SIZE(game_set_codec_formats, g_set_codec_formats);
    /*  we are using default for Mvp4 format */ 
    if ( GAME_IS_DAC(g_set_codec_formats->formats->codec) ) 
	{
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_CHANNELS)
	    {
	        acquire_benaphore(); 
	     	(*gaudio->card->func->SetPlaybackFormat)(gaudio->card, gaudio->dac_format, 
	     	                                         g_set_codec_formats->formats->channels); 
	     	                                    
	        gaudio->dac_channels = g_set_codec_formats->formats->channels;
	        release_benaphore();
	        
	        g_set_codec_formats->formats-> out_status = B_OK;
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_FORMAT)
	    {
	        acquire_benaphore();  
	    	(*gaudio->card->func->SetPlaybackFormat)(gaudio->card, g_set_codec_formats->formats->format, 
	     	                                         gaudio->dac_channels);
	     	      
	        gaudio->dac_format = g_set_codec_formats->formats->format;
	        release_benaphore();                                         
	        
	        g_set_codec_formats->formats-> out_status = B_OK;	
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_FRAME_RATE)
	    {
	       /*
	        acquire_benaphore();
	    	(*gaudio->card->func->SetPlaybackSR)(gaudio->card,  g_set_codec_formats->formats->frame_rate);
	    	      
	        gaudio->dac_frame_rate = g_set_codec_formats->formats->frame_rate;
	        release_benaphore();
	        
	        g_set_codec_formats->formats-> out_status = B_OK;
	      */	
	    	acquire_benaphore();
	    	(*gaudio->card->func->SetSampleRate)(gaudio->card, 44100);
	    	release_benaphore();
			    	
	    	g_set_codec_formats->formats-> out_status = B_ERROR;  //requested value could not be changhed 
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT)
	    {
	    	g_set_codec_formats->formats-> out_status = B_ERROR;  //requested value could not be changhed 
	    }
		
	}
    else if ( GAME_IS_ADC(g_set_codec_formats->formats->codec) )
	{
		if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_CHANNELS)
	    {
	        acquire_benaphore(); 
	     	(*gaudio->card->func->SetCaptureFormat)(gaudio->card, gaudio->adc_format, 
	     	                                         g_set_codec_formats->formats->channels); 
	     	                                    
	        gaudio->adc_channels = g_set_codec_formats->formats->channels;
	        release_benaphore();
	        
	        g_set_codec_formats->formats-> out_status = B_OK;
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_FORMAT)
	    {
	        acquire_benaphore();  
	    	(*gaudio->card->func->SetCaptureFormat)(gaudio->card, g_set_codec_formats->formats->format, 
	     	                                         gaudio->adc_channels);
	     	      
	        gaudio->adc_format = g_set_codec_formats->formats->format;
	        release_benaphore();                                         
	        
	        g_set_codec_formats->formats-> out_status = B_OK;	
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_FRAME_RATE)
	    {
	       /*
	        acquire_benaphore();
	    	(*gaudio->card->func->SetPlaybackSR)(gaudio->card,  g_set_codec_formats->formats->frame_rate);
	    	      
	        gaudio->dac_frame_rate = g_set_codec_formats->formats->frame_rate;
	        release_benaphore();
	        
	        g_set_codec_formats->formats-> out_status = B_OK;
	      */	
	    	acquire_benaphore();
	    	(*gaudio->card->func->SetSampleRate)(gaudio->card, 44100);
	    	release_benaphore();
			    	
	    	g_set_codec_formats->formats-> out_status = B_ERROR;  //requested value could not be changhed 
	    }
	    
	    if(g_set_codec_formats->formats->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT)
	    {
	    	g_set_codec_formats->formats-> out_status = B_ERROR;  //requested value could not be changhed 
	    }
	}
    else
        g_set_codec_formats->formats->codec = GAME_NO_ID;
	
	return err; 
}

/* open stream on dac/adc if free */
status_t open_stream_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	stream *open_stream;
	
    CHECK_SIZE(game_open_stream, g_open_stream);				
	DB_PRINTF(("open_stream_ioctl\n"));
	
			
	/* assume any DAC id simply refers to our single available DAC */
	if ( GAME_IS_DAC(g_open_stream->adc_dac_id) ) 
	{
		lock();
		
		if ( !(open_stream = (stream *) make_stream(dac)) ) {
			unlock();
			DB_PRINTF(("open_stream_ioctl: Failure! unable to make stream\n"));
			g_open_stream->out_stream_id = GAME_NO_ID;
			return B_BUSY;
		}	
		unlock();	
		
		DB_PRINTF(("open_stream_ioctl: Open Stream for DAC\n"));
        
		/* other fields for open_stream filled in make_stream */
		open_stream->stream_sem = g_open_stream->stream_sem;
			   			    
		acquire_benaphore();
		g_open_stream->frame_rate 	 = gaudio->frame_rate;
		g_open_stream->format 		 = gaudio->dac_format;
		g_open_stream->channel_count = gaudio->dac_channels;	// for stereo
		release_benaphore();	
							
		g_open_stream->cvsr_rate 	 = 0; 	//no hw support 
		g_open_stream->designations  = B_CHANNEL_LEFT | B_CHANNEL_RIGHT; 
		g_open_stream->out_stream_id = open_stream->streamID;  // created stream ID
		return B_OK;
	} 
	/* assume any ADC id simply refers to our single available ADC */
	else if ( GAME_IS_ADC(g_open_stream->adc_dac_id) )
	{	
		lock();
		
		if ( !(open_stream = (stream *) make_stream(adc)) ) {
			unlock();
			DB_PRINTF(("open_stream_ioctl: Failure\n"));
			g_open_stream->out_stream_id = GAME_NO_ID;
			return B_BUSY;
		}	
		unlock();
			
		DB_PRINTF(("open_stream_ioctl: Open Stream for DAC\n"));
	
		/* other fields for open_stream filled in make_stream */
		open_stream->stream_sem = g_open_stream->stream_sem;
			   			    
		acquire_benaphore();
		g_open_stream->frame_rate 	 = gaudio->frame_rate;
		g_open_stream->format 		 = gaudio->adc_format;
		g_open_stream->channel_count = gaudio->adc_channels;
		release_benaphore();						
		g_open_stream->cvsr_rate 	 = 0; 	/* no hw support */ 
		g_open_stream->designations  = B_CHANNEL_LEFT | B_CHANNEL_RIGHT; 
		g_open_stream->out_stream_id = open_stream->streamID;  
		return B_OK;
	}
	/* invalid adc_dac_id */
	g_open_stream->out_stream_id = GAME_NO_ID;
	return B_ERROR;
}

status_t  get_stream_timing_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	
    CHECK_SIZE(game_get_stream_timing, g_get_stream_timing);
			
   	if ( !GAME_IS_STREAM(g_get_stream_timing->stream) )
   		g_get_stream_timing->stream = GAME_NO_ID;
   	else
   	if (g_get_stream_timing->stream == playback_stream.streamID)
	{
		/* stopped, paused, running */
	   
	   g_get_stream_timing->state  = playback_stream.state;
	   //g_get_stream_timing->timing = 0;
	}
	else
	if(g_get_stream_timing->stream == capture_stream.streamID)
	{
		/*	stopped, paused, running	*/
   		g_get_stream_timing->state  = capture_stream.state;
		//g_get_stream_timing->timing = 0;
	}
	else
		g_get_stream_timing->stream = GAME_NO_ID;
	
    return err;
}		   

/* enqueue given buffer into specified stream */
status_t queue_stream_buffer_ioctl (gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	stream *target_stream = NULL;
	SGD *target_SGD = NULL;
	int16 buffer_ordinal;
	
	CHECK_SIZE(game_queue_stream_buffer, g_queue_stream_buffer);
    DB_PRINTF(("queue_stream_buffer_ioctl\n"));
    
    if (err != B_OK)
    	return B_ERROR;
    
    lock();
    
    /* verify valid bufferID */
    if ( !GAME_IS_BUFFER(g_queue_stream_buffer->buffer) || 
         !audiobuffers[buffer_ordinal = GAME_BUFFER_ORDINAL(g_queue_stream_buffer->buffer)] )
    {
    	unlock();
    	g_queue_stream_buffer->buffer = GAME_NO_ID;
    	DB_PRINTF(("queue_stream_buffer_ioctl: invalid bufferID\n"));
    	return B_ERROR;
    }
    
    /* verify buffer can be added to stream (matching streamID's or wildcard buffer) */
    if ( g_queue_stream_buffer->stream != audiobuffers[buffer_ordinal]->streamID &&
         audiobuffers[buffer_ordinal]->streamID != GAME_NO_ID )
    {
    	unlock();
    	g_queue_stream_buffer->stream = GAME_NO_ID;
    	DB_PRINTF(("queue_stream_buffer_ioctl: buffer does not belong to stream\n"));
    	return B_ERROR;
    }
            
 	target_stream = get_stream(g_queue_stream_buffer->stream);
 	target_SGD = get_SGD(g_queue_stream_buffer->stream);
		
	if (!target_stream)	{
    	unlock();
    	g_queue_stream_buffer->stream = GAME_NO_ID;
    	DB_PRINTF(("queue_stream_buffer_ioctl: invalid streamID\n"));
     	return B_ERROR;		
	}
	
	add_buffer_to_queue(g_queue_stream_buffer->stream, 
		                g_queue_stream_buffer->buffer, 
		                g_queue_stream_buffer->flags);

	if (target_stream->state == GAME_STREAM_PAUSED ||
	    target_stream->state == GAME_STREAM_STOPPED) 
	{
		unlock();
		return B_OK;
	}                    
		                    
	/* NOTE: stream state must be running at this point */
		
	if (!target_SGD->state.SGD_active) 
	{
		fill_table_from_queue(g_queue_stream_buffer->stream);
		target_SGD->state.SGD_active = true;

		if (target_SGD == &playback_SGD)
		{
			unlock();
			DB_PRINTF(("Starting SGD for playback\n"));
			Start_Playback_Pcm(gaudio);
		}
		else
		{
			unlock();
			DB_PRINTF(("Starting SGD for capture\n"));
			Start_Capture_Pcm(gaudio);
		}
		return B_OK;
	}
		
	if (target_SGD->state.SGD_stopped)
	{
		fill_table_from_queue(g_queue_stream_buffer->stream);
		target_SGD->state.SGD_stopped = 0;
				
	    if (target_SGD == &playback_SGD)
	    {
	    	unlock();
	        DB_PRINTF(("RE-START Playback !!!!!!!!!\n"));
	    	(*gaudio->card->func->Re_StartPlayback)(gaudio->card); 
	    }
		else 
		{
			unlock();
		    DB_PRINTF(("RE-START Capture!!!!!!!!!\n"));
			(*gaudio->card->func->Re_StartCapture)(gaudio->card); 
		}
		return B_OK;
	}
	
	unlock();
	return B_OK;
} 

/* change state of stream to specified state */
status_t run_streams_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	int index;
	stream *target_stream;
	SGD *target_SGD;
	enum game_stream_state present_state,
	                       next_state;
	
	CHECK_SIZE(game_run_streams, g_run_streams);							 								 				
	DB_PRINTF(("run_streams_ioctl\n"));
		
	g_run_streams->out_actual_count = 0;	
	
	/* for each stream state change request ("in_stream_count" requests) */
	for (index = 0; index < g_run_streams->in_stream_count; index++) 
	{
		//g_run_streams->streams[index].out_timing = 0;	/* needs to be implemented */

		/* bad streamID */
		if ( !GAME_IS_STREAM(g_run_streams->streams[index].stream) ) {
			DB_PRINTF(("run_streams_ioctl: invalid stream id passed\n"));
			continue;
		}
		
		lock();
		
		/* find the stream for requested state change */
		target_stream = NULL;
		if (g_run_streams->streams[index].stream == playback_stream.streamID) {
			target_stream = &playback_stream;
			target_SGD = &playback_SGD;
		}
		if (g_run_streams->streams[index].stream == capture_stream.streamID) {
			target_stream = &capture_stream;
			target_SGD = &capture_SGD;
		}
		
		/* invalid stream or pausing a stopped stream */
		if (!target_stream) {
			unlock();
			DB_PRINTF(("run_streams_ioctl: bad stream id passed\n"));
			continue;
		}
		
		present_state = target_stream->state;
		next_state = g_run_streams->streams[index].state;
		
		/* stream is closing and state change requested ?!?! */
		/* policy: terminate and dump */
		if (target_stream->pending_close && present_state != next_state)
		{
			int scan;
			
			if (target_SGD == &playback_SGD)
				Stop_Playback_Pcm(gaudio);
			else
				Stop_Capture_Pcm(gaudio);
				
			target_SGD->size = 0;
			target_SGD->readpoint = 0;
			target_SGD->state.SGD_active = 0;
			target_SGD->state.SGD_stopped = 0;
			target_SGD->state.SGD_paused = 0;

			/* dump nodes in SGD */
			for (scan = 0; scan < NSLOTS; scan++) {
				free_buffer_node(target_SGD->nodes_in_SGD[scan]);	/* will trigger stream closure */
				target_SGD->nodes_in_SGD[scan] = NULL;
			}
			target_stream->state = GAME_STREAM_STOPPED;
			
			unlock();
			continue;
		}
		
		switch (present_state) 
		{
			case GAME_STREAM_STOPPED:
				switch (next_state) 
				{
					case GAME_STREAM_STOPPED:
						unlock();
						continue;	/* no state change */
					
					case GAME_STREAM_PAUSED:
						unlock();
						continue;	/* illegal request */
					
					case GAME_STREAM_RUNNING:
					{
						unlock();
						DB_PRINTF(("Requesting stop to running state change\n"));
						lock();
						fill_table_from_queue(g_run_streams->streams[index].stream);
						
						/* if SGD is empty, will start on first queueing of buffer to avoid
						   playing empty buffers */
						if (target_SGD->size > 0) {
							unlock();
							if (target_SGD == &playback_SGD)
								Start_Playback_Pcm(gaudio);
							else
								Start_Capture_Pcm(gaudio);
							lock();
							target_SGD->state.SGD_active = true;
							target_SGD->state.SGD_stopped = 0;
							target_SGD->state.SGD_paused = 0;
						}
						target_stream->state = GAME_STREAM_RUNNING;
						unlock();
						g_run_streams->out_actual_count++;
						continue;
					}
				}
			
			case GAME_STREAM_PAUSED:
				switch (next_state) 
				{
					case GAME_STREAM_STOPPED:
					{
						int scan;
						audiobuffer_node *temp;
			
						/* terminate SGD */
						unlock();
						if (target_SGD == &playback_SGD)
							Stop_Playback_Pcm(gaudio);
						else
							Stop_Capture_Pcm(gaudio);
						lock();
						
						target_SGD->size = 0;
						target_SGD->readpoint = 0;
						target_SGD->state.SGD_active = 0;
						target_SGD->state.SGD_stopped = 0;
						target_SGD->state.SGD_paused = 0;
			
						/* free nodes in the SGD */
						for (scan = 0; scan < NSLOTS; scan++) {
							free_buffer_node(target_SGD->nodes_in_SGD[scan]);
							target_SGD->nodes_in_SGD[scan] = NULL;
						}
			
						/* free nodes in stream queue */
						while (target_stream->queue.head) 
						{
							temp = target_stream->queue.head;
							
							/* release stream semaphore if necessary */
							if (temp->flags & GAME_RELEASE_SEM_WHEN_DONE)
							{	
								unlock();
								DB_PRINTF(("free_buffer_node: released stream semaphore\n"));
								lock();
								release_sem_etc(temp->in_stream->stream_sem, 1, B_DO_NOT_RESCHEDULE);	
							}

							free_buffer_node(temp);
							target_stream->queue.head = target_stream->queue.head->next;
						}
						target_stream->queue.head = NULL;
						target_stream->queue.tail = NULL;			
						target_stream->state = GAME_STREAM_STOPPED;
						unlock();
						g_run_streams->out_actual_count++;
						continue;
					}
					
					case GAME_STREAM_PAUSED:
						unlock();
						continue;	/* no state change */
					
					case GAME_STREAM_RUNNING:
					{
						if (!target_SGD->state.SGD_stopped)
						{
							unlock();
							if (target_SGD == &playback_SGD)
								(*gaudio->card->func->Unpause_Playback)(gaudio->card);
							else
								(*gaudio->card->func->Unpause_Capture)(gaudio->card);
							lock();
							
							target_SGD->state.SGD_paused = false;
						}
						target_stream->state = GAME_STREAM_RUNNING;
						unlock();
						g_run_streams->out_actual_count++;
						continue;
					}
				}
			
			case GAME_STREAM_RUNNING:
				switch (next_state) 
				{
					case GAME_STREAM_STOPPED:
					{
						int scan;
						audiobuffer_node *temp;
			
						/* terminate SGD */
						unlock();
						if (target_SGD == &playback_SGD)
							Stop_Playback_Pcm(gaudio);
						else
							Stop_Capture_Pcm(gaudio);
						lock();
						
						target_SGD->size = 0;
						target_SGD->readpoint = 0;
						target_SGD->state.SGD_active = 0;
						target_SGD->state.SGD_stopped = 0;
						target_SGD->state.SGD_paused = 0;
			
						/* free nodes in the SGD */
						for (scan = 0; scan < NSLOTS; scan++) {
							free_buffer_node(target_SGD->nodes_in_SGD[scan]);
							target_SGD->nodes_in_SGD[scan] = NULL;
						}
			
						/* free nodes in stream queue */
						while (target_stream->queue.head) 
						{
							temp = target_stream->queue.head;
						
							/* release stream semaphore if necessary */
							if (temp->flags & GAME_RELEASE_SEM_WHEN_DONE)
							{	
								unlock();
								DB_PRINTF(("free_buffer_node: released stream semaphore\n"));
								lock();
								release_sem_etc(temp->in_stream->stream_sem, 1, B_DO_NOT_RESCHEDULE);	
							}
						
							free_buffer_node(temp);
							target_stream->queue.head = target_stream->queue.head->next;
						}
						target_stream->queue.head = NULL;
						target_stream->queue.tail = NULL;			
						target_stream->state = GAME_STREAM_STOPPED;
						unlock();
						g_run_streams->out_actual_count++;
						continue;
					}
					
					case GAME_STREAM_PAUSED:
					{
						if (!target_SGD->state.SGD_stopped)
						{
							unlock();
							if (target_SGD == &playback_SGD)
								(*gaudio->card->func->Pause_Playback)(gaudio->card);
							else
							    (*gaudio->card->func->Pause_Capture)(gaudio->card);
							lock();
							
							target_SGD->state.SGD_paused = true;
						}
						target_stream->state = GAME_STREAM_PAUSED;
						unlock();
						g_run_streams->out_actual_count++;
						continue;
					}
				
					case GAME_STREAM_RUNNING:
						unlock();
						continue;	/* no state change */
				}				
		}
	}
}

/* close stream with specified options */
status_t close_stream_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	stream *target_stream = NULL;
	SGD *target_SGD = NULL;
	sem_id close_sem;

	CHECK_SIZE(game_close_stream, g_close_stream);
	DB_PRINTF(("close_stream_ioctl\n"));
	lock();	

	target_stream = get_stream(g_close_stream->stream);
		
	/* invalid stream */
	if (!target_stream) 
	{
		unlock();
		DB_PRINTF(("close_stream_ioctl: Failure! invalid streamID\n"));
	    g_close_stream->stream = GAME_NO_ID;		
		return B_ERROR;
	}
	
	/* force flush, proper close to be supported later */
	g_close_stream->flags |= GAME_CLOSE_FLUSH_DATA;
		
	/* call to close_stream, verify proper parameters */
	if (close_stream(gaudio, target_stream, g_close_stream->flags) < 0) 
	{
		unlock();
	    g_close_stream->stream = GAME_NO_ID;		
		DB_PRINTF(("close_stream_ioctl: Failure\n"));
		return B_ERROR;
	}

	unlock();
	return B_OK;
}

/* create a buffer, including allocation of contiguous memory */
status_t open_stream_buffer_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	audiobuffer *new_buffer;
	stream *target_stream = NULL;
	
    CHECK_SIZE(game_open_stream_buffer, open_stream_buffer);	
    DB_PRINTF(("open_stream_buffer_ioctl\n"));	

	if (err != B_OK)
		return err;
	
	lock();

	target_stream = get_stream(open_stream_buffer->stream);
	
	/* stream invalid */
	if ( !target_stream && open_stream_buffer->stream != GAME_NO_ID ) {
		unlock();
		//open_stream_buffer->out_status = B_ERROR;
		open_stream_buffer->stream = GAME_NO_ID;
		DB_PRINTF(("open_stream_buffer_ioctl: Failure! invalid streamID\n"));	
		return B_BAD_VALUE;
	}

	/* verify successful memory allocation */
	if ( !(new_buffer = (audiobuffer *) make_buffer(target_stream, open_stream_buffer->byte_size)) ) {
		//open_stream_buffer->out_status = B_NO_MEMORY;
		unlock();
		open_stream_buffer->buffer = GAME_NO_ID;
		DB_PRINTF(("open_stream_buffer_ioctol: Failure!\n"));
		return B_NO_MEMORY;
	}
	
	unlock();
	
	/* buffer created successfully */
    DB_PRINTF(("open_stream_buffer_ioctl: buffer created:\n"));
    DB_PRINTF(("address: %lx\n", new_buffer->phys_addr));
    DB_PRINTF(("areaID: %d\n", new_buffer->areaID));
    DB_PRINTF(("bufferID: %lx\n", new_buffer->bufferID));
    DB_PRINTF(("size: return %ld\n", new_buffer->size));

	open_stream_buffer->stream = new_buffer->streamID;
	//open_stream_buffer->out_status = B_OK;
    open_stream_buffer->area = new_buffer->areaID;
	open_stream_buffer->offset = 0; 
	open_stream_buffer->buffer = new_buffer->bufferID;
	
	return B_OK;
}

/* close buffer if it is not enqueued */
status_t close_stream_buffer_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	int16 buffer_ordinal;
	
	CHECK_SIZE(game_close_stream_buffer, close_stream_buffer);
	DB_PRINTF(("close_stream_buffer_ioctl\n"));

	if (err != B_OK)
		return err;
				
	lock();
	
	/* if invalid ID or the corresponding buffer isn't allocated */
	if ( !GAME_IS_BUFFER(close_stream_buffer->buffer) || 
	     !audiobuffers[buffer_ordinal = GAME_BUFFER_ORDINAL(close_stream_buffer->buffer)] ) 
	{
		unlock();
		DB_PRINTF(("close_stream_buffer_ioctl: Failure! invalid bufferID\n"));
		close_stream_buffer->buffer = GAME_NO_ID;
		return B_ERROR;
	}	
	/* if buffer is still enqueued */ 
	else if (audiobuffers[buffer_ordinal]->enqueue_count) 
	{
		unlock();
		DB_PRINTF(("close_stream_buffer_ioctl: Failure! buffer still enqueued\n"));
		return B_ERROR;
	}
	
	/* due to type-checking above, close buffer should not error */
	if (close_buffer(close_stream_buffer->buffer) < 0) {
		unlock();
		DB_PRINTF(("close_stream_buffer_ioctl: Failure! unexpected close_buffer failure\n"));
		return B_ERROR;
	}
	
	unlock();
	return B_OK;
}

/*  mixer's  ioctls */

status_t  get_mixer_infos_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	CHECK_SIZE(game_get_mixer_infos, g_get_mixer_infos);
	DB_PRINTF(("GAME_GET_MIXER_INFOS\n"));
	//g_get_mixer_infos->info->mixer_id = gaudio->mixer_id;
	g_get_mixer_infos->info->linked_codec_id = -1;
	g_get_mixer_infos->out_actual_count = 1;
	strcpy(g_get_mixer_infos->info->name, "mvp4_mix");
	g_get_mixer_infos->info->control_count = 5; 
	return err;
}	

status_t get_mixer_controls_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
    int n;
    game_mixer_control *mix_control;
	CHECK_SIZE(game_get_mixer_controls, g_get_mixer_controls);
		
	for(n=0, mix_control = g_get_mixer_controls->control;
	    n < g_get_mixer_controls->in_request_count; 
	    n++)
	{ 
	    DB_PRINTF(("GAME_GET_MIXER_CONTROLS  %d\n", n));
    	mix_control[n].control_id  =  GAME_MAKE_CONTROL_ID(n);
   		mix_control[n].mixer_id  =	gaudio->mixer_id;
   	    if(n==0) mix_control[n].type = GAME_MIXER_CONTROL_IS_ENABLE;
   		else     mix_control[n].type = GAME_MIXER_CONTROL_IS_LEVEL; 
   		//else if(n==2) mix_control[n].type = GAME_MIXER_CONTROL_IS_MUX; 
   		//mix_control[n].type = GAME_MIXER_CONTROL_IS_LEVEL; 
		mix_control[n].flags = 0;  //GAME_MIXER_CONTROL_AUXILIARY;
    	mix_control[n].parent_id = GAME_NO_ID;
    }
		    
       g_get_mixer_controls->out_actual_count = n;  
	   return err;
}

status_t get_mixer_level_info_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
    int id;
   	CHECK_SIZE(game_get_mixer_level_info, g_get_mixer_level_info);
   	DB_PRINTF(("GAME_GET_MIXER_LEVEL_INFO\n"));
   	id = g_get_mixer_level_info -> control_id; 
   //if(id == 0) g_get_mixer_level_info -> control_id = GAME_NO_ID;
   	g_get_mixer_level_info -> mixer_id = gaudio->mixer_id;
   	
   	g_get_mixer_level_info -> flags = GAME_LEVEL_HAS_MUTE |
									  GAME_LEVEL_VALUE_IN_DB |
									  GAME_LEVEL_HAS_DISP_VALUES; /*|
									  GAME_LEVEL_ZERO_MEANS_NEGATIVE_INFINITY;*/
											
   	g_get_mixer_level_info -> min_value = 0;
   	g_get_mixer_level_info -> max_value = 31;
   	g_get_mixer_level_info -> normal_value = 10;
   	g_get_mixer_level_info -> min_value_disp = 0;
   	g_get_mixer_level_info -> max_value_disp = 30;
	    	
   	//strcpy(g_get_mixer_level_info -> disp_format, "format");
    
    if( id <2 )
    {
    	strcpy(g_get_mixer_level_info ->label , "master");	
   		g_get_mixer_level_info ->type = GAME_LEVEL_AC97_MASTER;
   		strcpy(g_get_mixer_level_info -> disp_format, "");  
   	}
   	else if (id == 2)
    {
    	strcpy(g_get_mixer_level_info ->label , "mic");	
   		g_get_mixer_level_info ->type = GAME_LEVEL_AC97_MIC;  
   	}
   	else if (id == 3)
   	{
    	strcpy(g_get_mixer_level_info ->label , "CD");	
   		g_get_mixer_level_info ->type = GAME_LEVEL_AC97_CD;  
   	}
   	else if (id == 4)
   	{
    	strcpy(g_get_mixer_level_info ->label , "Line IN");	
   		g_get_mixer_level_info ->type = GAME_LEVEL_AC97_LINE_IN;  
   	}
   	g_get_mixer_level_info->value_count = 2; 
   	
    return err;  	    	
}

status_t  get_mixer_mux_info_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
   	CHECK_SIZE(game_get_mixer_mux_info, g_get_mixer_mux_info);
   	DB_PRINTF(("GAME_GET_MIXER_MUX_INFO\n"));
   	return err;
}

status_t  get_mixer_enable_info_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
   	CHECK_SIZE(game_get_mixer_enable_info, g_get_mixer_enable_info);
   	DB_PRINTF(("GAME_GET_MIXER_ENABLE_INFO\n"));
   	g_get_mixer_enable_info-> mixer_id = gaudio->mixer_id;
   	strcpy(g_get_mixer_enable_info->label , "Mvp4_mixer");
   	g_get_mixer_enable_info-> normal = GAME_ENABLE_ON;
   	strcpy(g_get_mixer_enable_info->enabled_label , "mixer_enable");
   	strcpy(g_get_mixer_enable_info->disabled_label , "mixer_disable");
	return err;
}

status_t  get_mixer_control_values_ioctl(gaudio_dev *gaudio, void *data)
{
    status_t err = B_OK;
    
    //game_mixer_control_value  *mix_control_value;
    //int n = 0;
	CHECK_SIZE(game_get_mixer_control_values, g_get_mixer_control_values);
	DB_PRINTF(("GAME_GET_MIXER_CONTROL_VALUES \n"));
	//mix_control_value = g_get_mixer_control_values->values;
	/*for(n=0;
	    n < g_get_mixer_control_values->in_request_count; 
	    n++)
	{*/
	
	 //  g_get_mixer_control_values[0].values->type = GAME_MIXER_CONTROL_IS_LEVEL;
	 //  g_get_mixer_control_values[0].values->u.level.values[0] = 0;
	 //  g_get_mixer_control_values[0].values->u.level.flags = 0;
	 //  g_get_mixer_control_values[0].values->u.mux.mask = 0;
	 //  g_get_mixer_control_values[0].values->u.enable.enabled = 1;
	  
	   
	/*
	   	mix_control_value[n].type = GAME_MIXER_CONTROL_IS_LEVEL;
       	
		mix_control_value[n].u.level.values[0] = 10;
		mix_control_value[n].u.level.values[1] = 10;
		mix_control_value[n].u.level.values[2] = 10;
		mix_control_value[n].u.level.values[3] = 10;
		mix_control_value[n].u.level.values[4] = 10;
		mix_control_value[n].u.level.values[5] = 10; 
		mix_control_value[n].u.level.flags = 0;
		mix_control_value[n].u.mux.mask = 0;
		mix_control_value[n].u.enable.enabled = 1; */
	// }
	
	g_get_mixer_control_values->out_actual_count = 2;
	
	return err;
}
		
status_t  set_mixer_control_values_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	int16  value;
	int16 type;
	game_mixer_control_value  *mix_control_value;

	CHECK_SIZE(game_set_mixer_control_values, g_set_mixer_control_values);
	
	DB_PRINTF(("GAME_SET_MIXER_CONTROL_VALUES\n"));
	
	mix_control_value = g_set_mixer_control_values->values;
    // if(	g_set_mixer_control_values->values->control !=	
    value = mix_control_value[0].u.level.values[0];  
    DB_PRINTF(("val.0 = %d, val.1 = %d, val.2 = %d, val.3 = %d, val.4 = %d, val.5 = %d\n",
                mix_control_value[0].u.level.values[0],
                mix_control_value[1].u.level.values[1],
                mix_control_value[2].u.level.values[2],
                mix_control_value[3].u.level.values[3],
                mix_control_value[4].u.level.values[4],
                mix_control_value[5].u.level.values[5]));
                
    if(g_set_mixer_control_values->values->control_id == 2)
    	type = GAME_LEVEL_AC97_MIC;
    else	
    if(g_set_mixer_control_values->values->control_id == 3)
    	type =  GAME_LEVEL_AC97_CD;	
	else
	if(g_set_mixer_control_values->values->control_id == 4)
    	type =  GAME_LEVEL_AC97_LINE_IN;	
    else
        type = GAME_LEVEL_AC97_MASTER;	 
                   
                
    err = (*gaudio->card->func->SetSound)(gaudio->card, type, value);
    return err;
}

status_t get_stream_description_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	CHECK_SIZE(game_get_stream_description, g_get_stream_description);
	DB_PRINTF(("GAME_GET_STREAM_DESCRIPTOR\n"));
	return err;
}


status_t  get_stream_controls_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	CHECK_SIZE(game_get_stream_controls, g_get_stream_controls);
	DB_PRINTF(("SET_STREAM_CONTROL\n"));
	return err;
}

status_t  set_stream_controls_ioctl(gaudio_dev *gaudio, void *data)
{
	status_t err = B_OK;
	CHECK_SIZE(game_set_stream_controls, g_set_stream_controls);
	DB_PRINTF((" SET_STREAM_CONTROL\n"));
	return err;
}

		