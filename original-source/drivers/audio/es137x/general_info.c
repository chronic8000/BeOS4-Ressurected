#define GENERAL_INFO_MODULE //Allways have this definition in one of hw_depended modules.
							//It is used by audio_drv.h for fixing the linkeditor problems  
#include <driver_settings.h>
#include "audio_drv.h"


extern card_descrtiptor_t es1370_audio_descr;
extern card_descrtiptor_t es1371_audio_descr;
extern card_descrtiptor_t es5880_audio_descr;
extern es137x_GetDriverSettings(void* hSettings);

card_descrtiptor_t* CardDescriptors[] = {
	&es1370_audio_descr, 
	&es1371_audio_descr,
	&es5880_audio_descr,
	NULL
};


void GetDriverSettings()
{
	void* hSettings = NULL;
	hSettings = load_driver_settings("es137x");
	if(hSettings == NULL) return;
	es137x_GetDriverSettings(hSettings);
	unload_driver_settings(hSettings);
}
