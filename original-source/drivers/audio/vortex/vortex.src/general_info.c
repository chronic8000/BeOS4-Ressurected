#define GENERAL_INFO_MODULE //Allways have this definition in one of hw_depended modules.
							//It is used by audio_drv.h for fixing the linkge problem  
#include <driver_settings.h>
#include "audio_drv.h"
#include "vortex.h"


card_descrtiptor_t* CardDescriptors[] = {
	&au8820_descriptor, 
	&au8830_descriptor,
	NULL
};

void GetDriverSettings()
{
}
