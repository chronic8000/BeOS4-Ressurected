/*************************************************************************

 main.c
 
 Main entry points for Echo Darla/Gina/Layla driver for BeOS.

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include "monkey.h"
#include "init.h"

extern void mixerInit();

// question what about big/little endian reading config space?

// Tell Be what driver API version we expect
int32 api_version = B_CUR_DRIVER_API_VERSION ;

extern bigtime_t max_timeout;

/*------------------------------------------------------------------------

	init_hardware - called once the first time the driver is loaded
	
------------------------------------------------------------------------*/

status_t init_hardware (void)
{
	uint32 cards_found;

	DPF_ON;
	DPF1(("DGL: init_hardware\n"));
	
	// Detect & init all the cards, build the monkey barrel
	cards_found = build_monkey_list(true);
	
	// And destroy it right away
	free_all_monkeys();

	DPF2(("DGL: init_hardware done\n\n"));
	DPF_OFF;	

	// Return the appropriate value	
	if (0 == cards_found)
	{
		return ENODEV;
	}
	
	return B_OK;

} // end of init_hardware


/*------------------------------------------------------------------------

	init_driver - optional function - called every time the driver
	is loaded.

------------------------------------------------------------------------*/

status_t init_driver (void)
{
	uint32 cards_found;

	DPF_ON;
	DPF1(("DGL: init_driver\n"));

	// Init the mixer
	mixerInit();

	// Detect & init all the cards, build the monkey barrel
	cards_found = build_monkey_list(false);
	
	DPF_OFF;	

	// Return the appropriate value
	if (0 == cards_found)
	{
		return ENODEV;
	}
	
	return B_OK;

	
} // end of init_driver


/*------------------------------------------------------------------------
	
	uninit_driver - optional function - called every time the driver
	is unloaded

------------------------------------------------------------------------*/

#ifdef DEBUG
extern int irq_count;
#endif

void uninit_driver (void)
{

	DPF_ON;
	DPF1(("DGL: uninit_driver\n"));
	
	// Free all the monkeys
	free_all_monkeys();
	
	DPF1(("DGL: Max timeout is %d\n",max_timeout));
	DPF2(("DGL: irq_count is %d\n",irq_count));
	DPF1(("\n\n"));
	DPF_OFF;

} // end of uninit_driver



/*------------------------------------------------------------------------

	publish_devices - return a null-terminated array of devices
	supported by this driver.

------------------------------------------------------------------------*/

const char *device_names[MAX_DEVICES+1];
uint32 device_name_index = 0;

const char** publish_devices()
{
	DPF_ON;
	DPF2(("DGL: publish_devices\n"));
	DPF_OFF;

	if (NULL == device_names[0])
		return NULL;

	return device_names;
} // end of publish_devices




