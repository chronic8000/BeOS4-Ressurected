#define GENERAL_INFO_MODULE //Allways have this definition in one of hw_depended modules.
							//It is used by audio_drv.h for fixing the linkge problem  
#include <driver_settings.h>
#include "audio_drv.h"
#include "ymf724.h"
#include "ymf_add.h"


card_descrtiptor_t* CardDescriptors[] = {
	&ymf724_descriptor, 
	&ymf724f_descriptor,
	&ymf734_descriptor,
	&ymf737_descriptor,
	&ymf738_descriptor,
	&ymf738_teg_descriptor,
	&ymf740_descriptor,
	&ymf740c_descriptor,
	&ymf744_descriptor,
	&ymf754_descriptor,
	NULL
};

void GetDriverSettings()
{
	void* hSettings = load_driver_settings("ymf724");
	
	ymf724_GetDriverSettings(hSettings);
	unload_driver_settings(hSettings);
}
