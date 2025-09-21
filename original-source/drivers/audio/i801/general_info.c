#include <driver_settings.h>
#include "audio_drv.h"


extern card_descrtiptor_t i801_audio_descr;
extern card_descrtiptor_t i801e_audio_descr;
extern bool g_enableMPU;

card_descrtiptor_t* CardDescriptors[] = {
	&i801_audio_descr, 
	&i801e_audio_descr, 
	NULL
};


void GetDriverSettings()
{
	void * h = load_driver_settings("i801");
	if (h == 0) return;
	g_enableMPU = get_driver_boolean_parameter(h, "enable_mpu", true, true);
	unload_driver_settings(h);
}
