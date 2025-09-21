
/************************************************************************/
/*                                                                      */
/*                              gaudio.h	                           	*/
/*                                                                      */
/* 	Developed by Evgeny											 		*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _GAUDIO_H
#define _GAUDIO_H

typedef struct _game_buffer_data{
		void*  phys_addr;
		uint32 size;
}game_buffer_data;

game_buffer_data  buffer_data[GAME_MAX_BUFFER_COUNT];


typedef struct _pcm_playback_data_t {
	sem_id LockSem;
	sem_id CompletionSem;
	volatile bool is_running;
	int32  stopped; 
}pcm_playback_data_t;

typedef struct _pcm_capture_data_t {
	sem_id LockSem;
	sem_id CompletionSem;	
	volatile bool is_running;
	int32  stopped; 
}pcm_capture_data_t;

typedef struct gaudio_dev_t{
	sound_card_t * card;
	char		name[B_OS_NAME_LENGTH];
	int32 OpenCount;

	volatile bool is_running;
	sem_id StopSem;
	
	int32 StopPlaybackFlag;
	int32 StopCaptureFlag;

	pcm_playback_data_t Playback;
	pcm_capture_data_t Capture;
	
	int16  dac_id;   		// ID of DAC
	int16  adc_id;			// ID of ADC
	int16  mixer_id; 		// mixer ID
	int16  cur_stream_count;// number of opened streams
	uint32 frame_rate;		// bit mask union of the frame rate (format defined in <adio_base.h>)
	uint32 dac_format;
	uint32 adc_format;
	uint16 dac_channels;
	uint16 adc_channels;
	
} gaudio_dev;

long init_benaphore();
void acquire_benaphore();
void release_benaphore();
void delete_benaphore();

extern bool pcm_playback_interrupt(sound_card_t *);
extern bool pcm_capture_interrupt(sound_card_t *);

void Stop_Playback_Pcm(gaudio_dev *gaudio);
void Stop_Capture_Pcm(gaudio_dev *gaudio);
void Start_Playback_Pcm(gaudio_dev *gaudio);
void Start_Capture_Pcm(gaudio_dev *gaudio);

/* ioctl's functions */

status_t  get_info_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_dac_infos_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_adc_infos_ioctl(gaudio_dev *gaudio, void *data);
status_t  set_codec_formats_ioctl(gaudio_dev *gaudio, void *data);
status_t  open_stream_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_stream_timing_ioctl(gaudio_dev *gaudio, void *data);
status_t  queue_stream_buffer_ioctl(gaudio_dev *gaudio, void *data);
status_t  run_streams_ioctl(gaudio_dev *gaudio, void *data);
status_t  close_stream_ioctl(gaudio_dev *gaudio, void *data);
status_t  open_stream_buffer_ioctl(gaudio_dev *gaudio, void *data);
status_t  close_stream_buffer_ioctl(gaudio_dev *gaudio, void *data);

status_t  get_mixer_infos_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_mixer_controls_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_mixer_level_info_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_mixer_mux_info_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_mixer_enable_info_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_mixer_control_values_ioctl(gaudio_dev *gaudio, void *data);
status_t  set_mixer_control_values_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_stream_description_ioctl(gaudio_dev *gaudio, void *data);
status_t  get_set_controls_ioctl(gaudio_dev *gaudio, void *data);
status_t  set_stream_controls_ioctl(gaudio_dev *gaudio, void *data);

#endif