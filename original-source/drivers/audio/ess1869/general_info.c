#define GENERAL_INFO_MODULE //Allways have this definition in one of hw_depended modules.
							//It is used by audio_drv.h for fixing the linkge problem  
#include <driver_settings.h>
#include "audio_drv.h"

#define 		ESS_VENDOR_ID			0x3007316
#define 		ESS_DEVICE_ID			0xffffffff

#define 		ESS_LOGICAL_ID_68		0x68187316
#define 		ESS_LOGICAL_ID_69		0x69187316
#define 		ESS_LOGICAL_ID_78		0x78187316
#define 		ESS_LOGICAL_ID_79		0x79187316


extern card_descrtiptor_t ess1868_descriptor;
extern card_descrtiptor_t ess1869_descriptor;
extern card_descrtiptor_t ess1878_descriptor;
extern card_descrtiptor_t ess1879_descriptor;

card_descrtiptor_t* CardDescriptors[] = {
	&ess1868_descriptor, 
	&ess1869_descriptor, 
	&ess1878_descriptor, 
	&ess1879_descriptor, 
	NULL
};



void GetDriverSettings()
{
}
