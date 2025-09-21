/************************************************************************/
/*                                                                      */
/*                              ac97.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _AC97_H
#define _AC97_H
#include "sound.h"

typedef status_t (*ac97_write_ft)(void *host, uchar offset, uint16 value, uint16  mask );
typedef uint16(*ac97_read_ft)(void *host, uchar offset);

//Write(void *host, uchar offset, uint16 value, uint16 mask );
//Read(void *host, uchar offset);

typedef struct _ac97_t ac97_t;

struct _ac97_t{
	void *host;
	void *hw;
	ac97_write_ft Write;
	ac97_read_ft Read;	
	sound_setup soundSetup;
	sem_id soundSetupLockSem;
	int32 lock;
};

status_t ac97init(	ac97_t* ac97, 
					void* host,
					ac97_write_ft codec_write_proc,
					ac97_read_ft codec_read_proc );

void ac97uninit(ac97_t* ac97);
status_t AC97_SoundSetup(ac97_t* ac97, sound_setup *ss);
status_t  AC97_GetSoundSetup(ac97_t* ac97, sound_setup *ss);


#endif
