#define GENERAL_INFO_MODULE

#include <driver_settings.h>
#include "audio_drv.h"


extern card_descrtiptor_t cs4280_audio_descr;

card_descrtiptor_t* CardDescriptors[] = {
	&cs4280_audio_descr, 
	NULL
};


void GetDriverSettings()
{
}
