#include <driver_settings.h>
#include "audio_drv.h"
#include "ymf724.h"
#include "yamaha.h"

extern sound_card_functions_t ymf724_functions;

card_descrtiptor_t ymf724f_descriptor = {
	"ymf724f",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF724F_DEVICE_ID
};

card_descrtiptor_t ymf734_descriptor = {
	"ymf734",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF734_DEVICE_ID
};

card_descrtiptor_t ymf737_descriptor = {
	"ymf737",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF737_DEVICE_ID
};

card_descrtiptor_t ymf738_descriptor = {
	"ymf738",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF738_DEVICE_ID
};

card_descrtiptor_t ymf738_teg_descriptor = {
	"ymf738_teg",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF738_TEG_DEVICE_ID
};

card_descrtiptor_t ymf740_descriptor = {
	"ymf740",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF740_DEVICE_ID
};

card_descrtiptor_t ymf740c_descriptor = {
	"ymf740c",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF740C_DEVICE_ID
};

card_descrtiptor_t ymf744_descriptor = {
	"ymf744",
	PCI, 
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID, 
	YMF744_DEVICE_ID
};

card_descrtiptor_t ymf754_descriptor = {
	"ymf754",
	PCI,
	&ymf724_functions,		//use the same code as ymf724
	YAMAHA_VENDOR_ID,
	YMF754_DEVICE_ID
};
