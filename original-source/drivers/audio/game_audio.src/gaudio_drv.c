/************************************************************************/
/*                                                                      */
/*                              gaudio_drv.c 	                      	*/
/*                                                                      */
/* 	Developed by Evgeny                     							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include <KernelExport.h>
#include <PCI.h>
#include <ISA.h>
#include <Drivers.h>
#include <Errors.h>
#include <isapnp.h>
#include <config_manager.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include "device.h"
#include "mvp4_debug.h"

#include "gaudio_card.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

static char pci_name[] = B_PCI_MODULE_NAME;
pci_module_info *pci;

#if SUPPORTS_ISA
static struct isa_module_info *isainfo;	// to read/write in ports 
#endif

static char gameport_name[] = B_GAMEPORT_MODULE_NAME;
generic_gameport_module * gameport;
static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module * mpu401;


#define MAX_CARDS 4

extern device_hooks gaudio_hooks;
extern device_hooks midi_hooks;
extern device_hooks joy_hooks;




static audio_card_t card[MAX_CARDS];

static int card_count = 0;

 

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */
const char** publish_devices()
{
	static const char* device_names[MAX_DEV+1];
	int i, count;
	DB_PRINTF(("publish_devices()\n"));
	for(i=0, count= GetDeviceCount(); i < count; i++) 
	{
		device_names[i] = GetDeviceName(i);
		DB_PRINTF(("%s\n", device_names[i]));
	}
	device_names[i] = NULL; //null-terminated array
	
//	device_names[0] = "audio/game/mvp4g/1";
//	device_names[1] = NULL;
	return   device_names;

}



/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */
device_hooks* find_device(const char* name)
{
	
	DB_PRINTF(("find_device(): here!\n"));
	return FindDeviceHooks(name);
}

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t init_hardware (void)
{ 
	status_t err = ENODEV;
	pci_info info;
#if SUPPORTS_ISA
	config_manager_for_driver_module_info *config_info; //for isa
#endif
	int n;
	
		 
	DB_PRINTF(("init_hardware(): here!\n"));

	if (get_module(pci_name, (module_info **)&pci)!=B_OK) {
		DB_PRINTF(("init_hardware(): PCI module not found! \n"));
		return ENOSYS;
	}
#if SUPPORTS_ISA
	if( get_module(	B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&config_info)!= B_OK ) 
	{
		DB_PRINTF(("---->init_hardware(): config_manager not found! \n"));
		config_info = NULL;
	}
#endif

	for(n = 0; CardDescriptors[n] != NULL; n++)
	{
		int ix = 0;
		if(CardDescriptors[n]->bus == PCI) 
		{
			while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
				if (info.vendor_id == CardDescriptors[n]->vendor_id && 	
					info.device_id ==  CardDescriptors[n]->device_id) {
					DB_PRINTF(("init_hardware(): Card found! VendorID = %x, DeviceID = %x\n", info.vendor_id, info.device_id));
					err = B_OK;
					break;
				}
				ix++;
			}
		}
#if SUPPORTS_ISA
		else if((config_info != NULL) && (CardDescriptors[n]->bus == ISA))
		{
			uint64 cookie = 0;
			struct isa_info *iinfo ;
			struct device_info dev_info;
			struct device_info* dinfo = NULL ;
			
			while(config_info->get_next_device_info(B_ISA_BUS, &cookie, &dev_info, sizeof(dev_info)) == B_OK) 
			{
				int result ;
				if (dev_info.config_status != B_OK)	continue;
	
			/*	get card configuration*/
				if( (result = config_info->get_size_of_current_configuration_for(cookie)) < 0 ) 
				{
				  DB_PRINTF(("---->get card configuration error. Size is negative ! \n"));
				  continue;
				}
	
			/*	alloc memory for dinfo, to look through devices  */
			    dinfo = (struct device_info *) malloc(dev_info.size) ; 			// result			
				if (dinfo == NULL )
				{
					put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
					DB_PRINTF(("---->error alocating memory\n"));
					return ENOMEM; 		
				}			
				
				config_info->get_device_info_for(cookie, dinfo, dev_info.size);
				iinfo = (struct isa_info *)((char *)dinfo + dinfo->bus_dependent_info_offset);
	
				if (iinfo->vendor_id == CardDescriptors[n]->vendor_id && 	
				  //  iinfo->card_id ==  CardDescriptors[n]->device_id && 
				    EQUAL_EISA_PRODUCT_ID ( (EISA_PRODUCT_ID) iinfo->logical_device_id,  
					                      (EISA_PRODUCT_ID) CardDescriptors[n]->logical_id  ) )
				{
					DB_PRINTF(("The card is found! VendorID = 0x%lx, DeviceID = 0x%lx, LogicalID = 0x%lx\n", iinfo->vendor_id, iinfo->card_id ,iinfo->logical_device_id));	
					err = B_OK;
					free(dinfo);
					break;
				}
				ix++;
				free(dinfo);
			}

		}
#endif	//	SUPPORTS_ISA
		else continue;
		if(err == B_OK) break;
	}
#if SUPPORTS_ISA
	if (config_info) put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
#endif
	put_module(pci_name);
	DB_PRINTF(("init_hardware: return %lx\n", err));
	return err;
}

/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */


status_t init_driver (void)
{
	status_t err = ENODEV;
	pci_info info;
	int n;
	config_manager_for_driver_module_info *config_info;//for isa cards

		

	DB_PRINTF(("init_driver(): here! \n"));

	/*	init PCI structure */
	if (get_module(pci_name, (module_info **)&pci)!=B_OK) 
	{
		DB_PRINTF(("init_driver(): PCI module not found! \n"));
		return ENOSYS;
	}
	
#if SUPPORTS_ISA
	/*	init isa structure */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&(isainfo)) != B_OK ) {
		DB_PRINTF(("init_driver(): ISA Bus module not found! \n"));
		isainfo = NULL;
	} 
#endif
	
#if SUPPORTS_ISA
	if(B_OK != get_module(	B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME,
					(module_info **)&config_info) )		
	{
		DB_PRINTF(("init_driver(): Config manager module not found! \n"));
		config_info = NULL;
	}
#endif
	if (get_module(gameport_name, (module_info **) &gameport))
	{
		DB_PRINTF(("init_driver(): Gameport module not found!\n"));
		gameport = NULL;
	}
	if (get_module(mpu401_name, (module_info **) &mpu401))
	{
		DB_PRINTF(("init_driver(): MIDI module not found!\n"));
		mpu401 = NULL;
	}
	GetDriverSettings();
	card_count = 0;
	for(n = 0; CardDescriptors[n] != NULL; n++)
	{
		int ix = 0;
		int num_by_type = 0;

		if (CardDescriptors[n]->bus == PCI) 
        {
			DB_PRINTF(("looking for pci device: %s, vendor_id = 0x%lx, device_id = 0x%lx\n",CardDescriptors[n]->name,CardDescriptors[n]->vendor_id,CardDescriptors[n]->device_id));
			while ((*pci->get_nth_pci_info)(ix, &info) == B_OK && card_count < MAX_CARDS)
			{
				DB_PRINTF(("     Found pci device: vendor_id = 0x%lx, device_id = 0x%lx\n",info.vendor_id,info.device_id));
				
				if (info.vendor_id == CardDescriptors[n]->vendor_id &&
					info.device_id ==  CardDescriptors[n]->device_id)
				{
					memset((void *)&card[card_count], 0, sizeof(sound_card_t));
	
	// Fix: to make sure different types of yamaha cards do not use the same ports for midi and joystick
					//card[card_count].hw.idx = num_by_type;	
					card[card_count].hw.idx = card_count;
					
					num_by_type++; 
					sprintf(card[card_count].hw.name,"%s/%d",
								 CardDescriptors[n]->name, num_by_type);
					DB_PRINTF(("init_driver(): initializing card %s ...\n", card[card_count].hw.name));
					card[card_count].hw.host = &card[card_count];
					card[card_count].hw.pcm_playback_interrupt = &pcm_playback_interrupt;
					card[card_count].hw.pcm_capture_interrupt = &pcm_capture_interrupt;
					card[card_count].hw.midi_interrupt = &midi_interrupt;
					card[card_count].hw.func = CardDescriptors[n]->func;
	
					card[card_count].hw.bus.pci.info = info;
					card[card_count].gaudio.card = &card[card_count].hw;
					card[card_count].midi.card = &card[card_count].hw;
					card[card_count].joy.card = &card[card_count].hw;
					if(card[card_count].hw.func->Init(&card[card_count].hw) == B_OK)
					{
	                   	RegisterDevice("audio/game/", card[card_count].hw.name, &gaudio_hooks, &card[card_count].gaudio);
		              
						err = B_OK;
						if((mpu401 != 0) && ((*card[card_count].hw.func->InitMidi)(&card[card_count].hw) == B_OK))
						{
							status_t status; 
							card[card_count].midi.driver = NULL;
							if (card[card_count].hw.func->mpu401_io_hooks == NULL)
							{
								status = (*mpu401->create_device)(card[card_count].hw.mpu401_base,
											&card[card_count].midi.driver, 0, midi_interrupt_op, 
											&card[card_count].midi);
							}
							else
							{
								status = (*mpu401->create_device_alt)(card[card_count].hw.mpu401_base, card[card_count].hw.hw,
											card[card_count].hw.func->mpu401_io_hooks->read_port_func,
											card[card_count].hw.func->mpu401_io_hooks->write_port_func,
											&card[card_count].midi.driver, 0, midi_interrupt_op, 
											&card[card_count].midi);
							}
							DB_PRINTF(("init_driver(): create mpu401_device - status = %d, B_OK = %d\n",status,B_OK));
							if(status == B_OK)
							{
								DB_PRINTF(("init_driver(): midi.driver = 0x%lx\n",(uint32)card[card_count].midi.driver));
								if(card[card_count].midi.driver)
									RegisterDevice("midi/", card[card_count].hw.name, &midi_hooks, &card[card_count].midi);
							}	
						}					
						if((gameport != 0) && (*card[card_count].hw.func->InitJoystick)(&card[card_count].hw) == B_OK)
						{
							status_t status; 
							card[card_count].joy.driver = NULL;
							if (card[card_count].hw.func->joy_io_hooks == NULL)
							{
								status = (*gameport->create_device)(card[card_count].hw.joy_base, &card[card_count].joy.driver);
							}
							else
							{
								status = (*gameport->create_device_alt)(card[card_count].hw.hw,
											card[card_count].hw.func->joy_io_hooks->read_port_func,
											card[card_count].hw.func->joy_io_hooks->write_port_func,
											&card[card_count].joy.driver);
							}
							if(status == B_OK)
							{
								if(card[card_count].joy.driver)
									RegisterDevice("joystick/", card[card_count].hw.name, &joy_hooks, &card[card_count].joy);
							}
						} 
						card_count++;
					} //if
				} //while
				ix++;
			}  //if (CardDescriptors[n]->bus == PCI) 
		}	 
	} // for
#if SUPPORTS_ISA
	if (config_info) put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
#endif
	DB_PRINTF(("init_driver(): done! (error 0x%x)\n", err));
	return err;
}

/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void uninit_driver (void)
{
	DB_PRINTF(("uninit_driver(): here! \n"));
	while (card_count>0)
	{
		card_count--;
		(*card[card_count].hw.func->Uninit)(&card[card_count].hw);
	}
	if ((gameport != 0) && card[card_count].joy.driver)(*gameport->delete_device)(card[card_count].joy.driver);
	if ((mpu401 != 0) && card[card_count].midi.driver)(*mpu401->delete_device)(card[card_count].midi.driver);

	put_module(pci_name);
#if SUPPORTS_ISA
	if (isainfo != NULL) put_module(B_ISA_MODULE_NAME);
#endif
	if (gameport != NULL) put_module(gameport_name);
	if (mpu401 != NULL) put_module(mpu401_name);
	
	DB_PRINTF(("uninit_driver(): return!\n"));
}

