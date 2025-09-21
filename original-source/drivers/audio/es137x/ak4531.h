#ifndef AK4531_H
#define AK4531_H

#include "sound.h" 

typedef struct ak4531_t
{
	void* host;
	void (*write)(void* host, uint8 reg, uint8 value);
   	uint8 regs[26];
	uint8 reserved[32];
}ak4531_t;

status_t ak4531_init(void** codec,void* host, void (*codec_write)(void* host, uint8 reg, uint8 value));
void ak4531_Setup(void* the_codec);
void ak4531_uninit(void* the_codec);
status_t ak4531_SoundSetup(void* the_codec, sound_setup *ss);
#endif
