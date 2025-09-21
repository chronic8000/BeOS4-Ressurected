#define GENERAL_INFO_MODULE //Allways have this definition in one of hw_depended modules.
							//It is used by audio_drv.h for fixing the linkge problem  
#include <driver_settings.h>
#include "audio_drv.h"


extern card_descrtiptor_t mvp4_audio_descr;

card_descrtiptor_t* CardDescriptors[] = {
	&mvp4_audio_descr, 
	NULL
};


void GetDriverSettings()
{
}
