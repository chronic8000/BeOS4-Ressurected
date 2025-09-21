#include "audio_base.h"
#include "game_audio.h"
#include "audio_drv.h"
#include "gaudio.h"
#include "midi.h"
#include "joy.h"

typedef struct{
	sound_card_t hw;
	gaudio_dev	gaudio;
	midi_dev	midi;
	joy_dev		joy;
//	mux_dev		mux;
//	mixer_dev	mixer;

} audio_card_t;



