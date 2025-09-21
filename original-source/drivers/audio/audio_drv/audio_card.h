#include "audio_drv.h"
#include "pcm.h"
#include "midi.h"
#include "joy.h"

typedef struct{
	sound_card_t hw;
	pcm_dev		pcm;
	midi_dev	midi;
	joy_dev		joy;
//	mux_dev		mux;
//	mixer_dev	mixer;

} audio_card_t;
