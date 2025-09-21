/************************************************************************/
/*                                                                      */
/*                              pcm.h	                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _PCM_H
#define _PCM_H

typedef struct _pcm_playback_data_t {
	sem_id LockSem;
	sem_id CompletionSem;
	int32 Lock;
	sem_id SyncSem;
	int32 SyncFlag;
	vint	HalfNo;
	int HalfBufsTwoClear;
	uchar *DmaBufAddr;
	size_t DmaBufHalfSize;
}pcm_playback_data_t;

typedef struct _pcm_capture_data_t {
	sem_id LockSem;
	sem_id CompletionSem;
	int32 Lock;
	sem_id SyncSem;
	int32 SyncFlag;
	vint	HalfNo;
	volatile size_t Offset;
	uchar *DmaBufAddr;
	size_t DmaBufHalfSize;

}pcm_capture_data_t;



typedef struct pcm_dev_t{
	sound_card_t * card;
	char		name[B_OS_NAME_LENGTH];
	int32 OpenCount;
	volatile bool is_runing;
	sem_id StopSem;
	int32 StopFlag;
	pcm_playback_data_t Playback;
	pcm_capture_data_t Capture;
}pcm_dev;

extern bool pcm_playback_interrupt(sound_card_t *, int half_no);
extern bool pcm_capture_interrupt(sound_card_t *, int half_no);

#endif
