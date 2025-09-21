/************************************************************************/
/*                                                                      */
/*                              gaudio.c 	                            */
/*                      												*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include <KernelExport.h>
#include "sound.h"
//#include "R3MediaDefs.h"
#include "gaudio_card.h"
#include "device.h"
#include "mvp4_debug.h"
#include "queue.h"
#include "lock.h"

/* These variables need to be protected against their corresponding functions 
   found in buffer.c, stream.c, queue.c and SGD.c using lock() and unlock() */
extern stream playback_stream,								/* one stream for playback */
              capture_stream;								/* another for capture */
extern audiobuffer *audiobuffers[GAME_MAX_BUFFER_COUNT];	/* allocation table for buffers */
extern SGD playback_SGD,									/* DMA table for playback */
           capture_SGD;										/* and for capture */

void Stop_Playback_Pcm(gaudio_dev *gaudio)
{
	DB_PRINTF(("Stop_Playback_Pcm():\n"));
	/* Stop playback */
	acquire_benaphore();
	(*gaudio->card->func->Stop_Playback_Pcm)(gaudio->card);
	gaudio->Playback.is_running = false;
	release_benaphore();
}

void Stop_Capture_Pcm(gaudio_dev *gaudio)
{
	DB_PRINTF(("Stop_Capcure_Pcm():\n"));
	/* Stop capture */
	acquire_benaphore();
	(*gaudio->card->func->Stop_Capture_Pcm)(gaudio->card);
	gaudio->Capture.is_running = false; 
	release_benaphore();
}

void Start_Playback_Pcm(gaudio_dev *gaudio)
{
	DB_PRINTF(("Start_Playback_Pcm():\n"));
	/* Start playback */
	acquire_benaphore();
	(*gaudio->card->func->Start_Playback_Pcm)(gaudio->card);
	gaudio->Playback.is_running = true;     /* allow write calls to be processed */
	release_benaphore();
}

void Start_Capture_Pcm(gaudio_dev *gaudio)
{
	DB_PRINTF(("StartCapturePcm():\n"));
	/* Start capture */
	acquire_benaphore();
	(*gaudio->card->func->Start_Capture_Pcm)(gaudio->card);
	gaudio->Capture.is_running = true;  /* allow read  calls to be processed */
	release_benaphore();
}

static status_t gaudio_open (const char *name, uint32 flags, void** cookie)
{	
	gaudio_dev* gaudio;
	status_t err = B_OK;
           
	DB_PRINTF(("gaudio_open(). Name %s:\n", name));
	
	init_benaphore();

    gaudio = (gaudio_dev*)FindDeviceCookie(name);
    
    if (gaudio == NULL)
    {
	   DB_PRINTF(("Can not find gaudio structure. gaudio == NULL!!\n"));
	   return ENODEV; 
    }  

    acquire_benaphore();
    gaudio->dac_id = GAME_MAKE_DAC_ID(1);
	gaudio->adc_id = GAME_MAKE_ADC_ID(1);
	gaudio->mixer_id = GAME_MAKE_MIXER_ID(1);
	gaudio->cur_stream_count = 0;
	gaudio->frame_rate       = B_SR_44100;   /* for fixed frame rate */
	release_benaphore();
	
	*cookie = gaudio;
	if(!gaudio) return ENODEV; 
/*
	if(atomic_add(&gaudio->OpenCount,1) != 0)
	{
		atomic_add(&gaudio->OpenCount,-1);	
		return B_BUSY;
	}
*/

    /* Set default  for Mvp4 format  and SR*/
    acquire_benaphore();
	gaudio->dac_format = B_FMT_16BIT;
	gaudio->dac_channels = 2;  // stereo
	
	(*gaudio->card->func->SetPlaybackFormat)(gaudio->card,gaudio->dac_format, gaudio->dac_channels); 
	(*gaudio->card->func->SetPlaybackSR)(gaudio->card, 44100);
	
	gaudio->adc_format = B_FMT_16BIT;
	gaudio->adc_channels = 2;  // stereo
	(*gaudio->card->func->SetCaptureFormat)(gaudio->card, gaudio->adc_format, gaudio->adc_channels);
	(*gaudio->card->func->SetCaptureSR)(gaudio->card, 44100);
	
	(*gaudio->card->func->SetSampleRate)(gaudio->card, 44100);
	    
    gaudio->Playback.stopped =0;       
	gaudio->Capture.stopped  =0;
		
	release_benaphore();    
	
 	DB_PRINTF(("gaudio_open(): return B_OK\n"));
	return B_OK;
}

static status_t gaudio_close (void *cookie)
{
	gaudio_dev *gaudio = (gaudio_dev *) cookie;
	
	delete_benaphore();
/*	
	if((atomic_add(&gaudio->OpenCount,-1) - 1) != 0) return B_ERROR;
	return B_OK;
*/	
}

/* -----
	gaudio_free - called after the  close returned and 
	all i/o is complete.
----- */
static status_t gaudio_free (void* cookie)
{
    int16  ix;
    gaudio_dev *gaudio = (gaudio_dev *) cookie;
	DB_PRINTF(("gaudio_free()\n"));
	(*gaudio->card->func->Stop_Playback_Pcm)(gaudio->card);
	(*gaudio->card->func->Stop_Capture_Pcm)(gaudio->card);
	return B_OK;
}


/* ----------
	gaudio_read - handle read() calls
----- */

static status_t gaudio_read (void* cookie, off_t position, void* buf, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

/* ----------
	gaudio_write - handle write() calls
----- */

static status_t gaudio_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_OK; 
}


/* ----------
	pcm_control - handle ioctl calls
----- */
static status_t gaudio_control(void *cookie, uint32 op, void *data,size_t len)
{
   	gaudio_dev *gaudio = (gaudio_dev *)cookie;
	status_t err = B_OK;
	cpu_status cp;
	
	DB_PRINTF(("gaudio_control(): op = %ld\n",op));
	if(cookie == NULL)
	{
		DB_PRINTF(("gaudio_control(): cookie == NULL, return B_BAD_VALUE;\n"));
		return B_BAD_VALUE;
	}
	
	DB_PRINTF(("gaudio_control(): op = %ld\n",op));
	switch (op) 
	{
		// GAME_GET_INFO = B_GAME_DRIVER_BASE,
		case GAME_GET_INFO:
		    err = get_info_ioctl(gaudio, data);
			break;
		case GAME_GET_DAC_INFOS:
		    err = get_dac_infos_ioctl(gaudio, data);
		    break;
		case GAME_GET_ADC_INFOS:
			err = get_adc_infos_ioctl(gaudio, data);
		    break;
		case GAME_SET_CODEC_FORMATS:
		    err = set_codec_formats_ioctl(gaudio, data);
			break;
		/* stream management */
		case GAME_OPEN_STREAM:
		    err = open_stream_ioctl(gaudio, data);	
			break;
		case GAME_GET_STREAM_TIMING:
		    err = get_stream_timing_ioctl(gaudio, data);
			break;
		case GAME_GET_STREAM_DESCRIPTION:
			break;
		
		//GAME_GET_STREAM_CONTROLS,
		//GAME_SET_STREAM_CONTROLS,
	
		case GAME_QUEUE_STREAM_BUFFER:
			err = queue_stream_buffer_ioctl(gaudio, data);
			break;
		case GAME_RUN_STREAMS:
		    err = run_streams_ioctl(gaudio, data);							 								 
			break;
		case GAME_CLOSE_STREAM:
		    err = close_stream_ioctl(gaudio, data);
			break;
		case GAME_OPEN_STREAM_BUFFER:
		    err = open_stream_buffer_ioctl(gaudio, data);	
		  	break;
		case GAME_CLOSE_STREAM_BUFFER:
			err = close_stream_buffer_ioctl(gaudio, data);
			break;
			case  GAME_GET_MIXER_INFOS:
		    get_mixer_infos_ioctl(gaudio, data);
			break;
		case GAME_GET_MIXER_CONTROLS:
		    get_mixer_controls_ioctl(gaudio, data);
			break;
	    case GAME_GET_MIXER_LEVEL_INFO:
	        get_mixer_level_info_ioctl(gaudio, data);
	    	break;
		case GAME_GET_MIXER_MUX_INFO:
	        get_mixer_mux_info_ioctl(gaudio, data);
	    	break;
		case GAME_GET_MIXER_ENABLE_INFO:
	       	get_mixer_enable_info_ioctl(gaudio, data);
	    	break;
			
	    /* mixer control values */
	    
	    case GAME_GET_MIXER_CONTROL_VALUES:
	        get_mixer_control_values_ioctl(gaudio, data);
	    	break;
		case GAME_SET_MIXER_CONTROL_VALUES:
		    set_mixer_control_values_ioctl(gaudio, data);
			break;
		default:
			{
				DB_PRINTF(("gaudio_control(): Unknown operation!!!\n"));
				err = B_OK;
				break;
			}
	/* save/load state */
	//GAME_GET_DEVICE_STATE,
	//GAME_SET_DEVICE_STATE,
	/* extensions (interfaces) */
	//GAME_GET_INTERFACE_INFO,
	//GAME_GET_INTERFACES
		
	}
	
    if(err != B_OK)
    {
    	DB_PRINTF(("game_control(): return %ld\n",err));	
    }
	else 
		DB_PRINTF(("game_control(): return B_OK\n")); 
		
	return err;
}


/* -----
	function pointers for the device hooks entry points
----- */
device_hooks gaudio_hooks = {
	gaudio_open, 		/* -> open entry point */
	gaudio_close, 		/* -> close entry point */
	gaudio_free,			/* -> free entry point */
	gaudio_control, 		/* -> control entry point */
	gaudio_read,			/* -> read entry point */
	gaudio_write,		/* -> write entry point */
	NULL, /*device_select_hook		select;*/		/* start select */
	NULL, /*device_deselect_hook	deselect;*/		/* stop select */
	NULL, /*device_readv_hook		readv;*/		/* scatter-gather read from the device */
	NULL  /*device_writev_hook		writev;*/		/* scatter-gather write to the device */
};


/* 
	PCM playback interrupt handler 
	*/
bool pcm_playback_interrupt(sound_card_t *card)
{  
    bool stop_status;
    audiobuffer_node cleared_entry_info; 
	gaudio_dev* gaudio = &((audio_card_t*)card->host)->gaudio;
   
   lock();

	/* 
	   clear_current_entry will call free_buffer_node which
	   will perform cleanup tasks including:
	     freeing the stream semaphore if necessary
		 closing pending streams if able
	*/
	stop_status = clear_current_entry(dac, &cleared_entry_info);
	
	if (increment_readpoint(dac) != B_OK)
	{
		unlock();
		Stop_Playback_Pcm(gaudio);
		lock();
		playback_SGD.state.SGD_active = 1;
		playback_SGD.state.SGD_stopped = 0;
		playback_SGD.state.SGD_paused = 0;
		
		if (playback_stream.queue.head)
		{
			unlock();
			DB_PRINTF(("playback stopped but could requeue and restart\n"));
			lock();
		}
		
		unlock(); 
	    return true;		
	}
	
	fill_table_from_queue(playback_stream.streamID);
    
	if (stop_status)
	{   
	    unlock(); 
		DB_PRINTF((" Playback STOP DETECTED !!!!!!!!!! \n"));
		lock(); 
		playback_SGD.state.SGD_stopped = true;
    	//fill_table_from_queue(playback_stream.streamID);
    }
	unlock();
  	return true;
}


/* 
	PCM capture interrupt handler 
	*/
bool pcm_capture_interrupt(sound_card_t *card)
{
    bool stop_status;
    audiobuffer_node cleared_entry_info;
    gaudio_dev* gaudio = &((audio_card_t*)card->host)->gaudio;   

	lock();
	
	/* 
	   clear_current_entry will call free_buffer_node which
	   will perform cleanup tasks including:
	     freeing the stream semaphore if necessary
		 closing pending streams if able
	*/
    stop_status = clear_current_entry(adc, &cleared_entry_info);
    if (increment_readpoint(adc) != B_OK)
	{
		unlock();
		Stop_Capture_Pcm(gaudio);
		lock();
		capture_SGD.state.SGD_active = 1;
		capture_SGD.state.SGD_stopped = 0;
		capture_SGD.state.SGD_paused = 0;
		unlock(); 
	    return true;	
	}
    
    fill_table_from_queue(capture_stream.streamID);
          
    if (stop_status == 1)
    {
		unlock(); 
		DB_PRINTF(("Capture STOP DETECTED !!!!!!!!!! \n"));
		lock(); 
		capture_SGD.state.SGD_stopped = true;
    }
    unlock();
   	return true;
}




