/*************************************************************************

 init.c
 
 Init routines

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <config_manager.h>
#include <PCI.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "monkey.h"
#include "56301.h"
#include "commpage.h"
#include "init.h"
#include "loaders.h"
#include "fixed.h"
#include "util.h"
#include "api.h"

status_t init_single_card(	pci_module_info *pmodule,
							pci_info *pci_card_info,
							uint32 card_count,
							uint32 card_type_count,
							bool load_dsp,
							ushort subsystem_id);

extern uint32 device_name_index;
extern const char *device_names[MAX_DEVICES+1];
extern JACKCOUNT darla_jack_count;
extern JACKCOUNT gina_jack_count;
extern JACKCOUNT layla_jack_count;
extern JACKCOUNT darla24_jack_count;
extern int32 darla_node_type_counts[];
extern int32 gina_node_type_counts[];
extern int32 layla_node_type_counts[];
extern int32 darla24_node_type_counts[];
extern int32 darla_channel_map[];
extern int32 gina_channel_map[];
extern int32 layla_channel_map[];
extern int32 darla24_channel_map[];
extern CARDTYPE DarlaCard;
extern CARDTYPE GinaCard;
extern CARDTYPE LaylaCard;
extern CARDTYPE Darla24Card;

PMONKEYINFO pMonkeyBarrel;

/*------------------------------------------------------------------------

	build_monkey_list
	
	Returns the count of the number of cards that were found
	and successfully set up.
	
------------------------------------------------------------------------*/

uint32 build_monkey_list(bool load_dsp)
{
	pci_module_info	*pmodule;
	long			pci_count;
	pci_info		pci_card_info;
	uint32			card_count,i;
	uint32			darla_count,gina_count,layla_count,*count_ptr;
	uint32			darla24_count;
	status_t		rval;
	bool			card_type_ok;
	ushort			subvendor_id,subsystem_id;
	
#ifdef __POWERPC__
	uint32			subsystem_temp;
#endif	

	DPF1(("DGL: build_monkey_list\n"));

	//	
	// Clear out the array for publish_devices
	//
	for (i=0; i<MAX_DEVICES; i++)
		device_names[i] = NULL;		
		
	// Clear the monkey barrel
	pMonkeyBarrel = NULL;

	//
	// Get the module to talk to the PCI bus manager
	//
	rval = get_module(B_PCI_MODULE_NAME,
					  (module_info **) &pmodule);
	if (rval != B_OK)
	{
		dprintf(("DGL: \tFailed on get_module\n"));
		return 0; 	// return no cards found
	}

	//
	// Scan through all the PCI cards looking for ours
	//
	card_count = 0;
	pci_count = 0;
	darla_count = 0;
	gina_count = 0;
	layla_count = 0;
	darla24_count = 0;
	device_name_index = 0;
	do
	{
		rval = pmodule->get_nth_pci_info(pci_count,&pci_card_info);

#ifdef __POWERPC__
		
		// The subvendor & subsystem fields in the pci_card_info struct
		// don't seem to be filled out correctly for PowerPC, so do it
		// directly.
		subsystem_temp = pmodule->read_pci_config(	pci_card_info.bus,
													pci_card_info.device,
													pci_card_info.function,
													PCI_subsystem_id,
													4);
		subvendor_id = subsystem_temp & 0xffff;
		subsystem_id = (subsystem_temp >> 16) & 0xffff;

#endif // __POWERPC__

#ifdef __INTEL__

		// These fields are OK with the Intel kernel		
		subvendor_id = pci_card_info.u.h0.subsystem_vendor_id;
		subsystem_id = pci_card_info.u.h0.subsystem_id ;
		
#endif // __INTEL__
		
		if (B_OK == rval)
		{
			// Is this a 56301 based Echo card?
			if (pci_card_info.vendor_id == MOTOROLA_VENDOR_ID &&
				pci_card_info.device_id == MOTOROLA_56301_ID &&
				subvendor_id == ECHO_VENDOR_ID)
			{
				card_type_ok = true;
				switch (subsystem_id)
				{
					case DARLA :
						count_ptr = &darla_count;
						DPF2(("DGL: \tFound a Darla\n"));
						break;
					case GINA :
						count_ptr = &gina_count;
						DPF2(("DGL: \tFound a Gina\n"));
						break;
					case LAYLA :
						count_ptr = &layla_count;
						DPF2(("DGL: \tFound a Layla\n"));
						break;
					case DARLA24 :
						count_ptr = &darla24_count;
						DPF2(("DGL: \tFound a Darla24\n"));
						break;
					default :
						count_ptr = NULL;
						card_type_ok = false;
						break;
				}
					
				if (card_type_ok)
				{
					if (B_OK == init_single_card(pmodule,&pci_card_info,card_count,
												 *count_ptr,load_dsp,subsystem_id))
					{
						DPF2(("DGL:\tCard init OK\n"));
						card_count++;
						*count_ptr = *count_ptr + 1;
						
						// This driver can only handle MAX_DEVICES cards
						if (MAX_DEVICES == card_count)
						{
							DPF2(("DGL: \tFound %d cards; halting bus scan\n",card_count));
							break;
						}
					}
				
				}
			}
	
			pci_count++;
		}				
	
	} while (B_OK == rval);
	
	// 
	// Put the PCI bus module away	
	//
	put_module(B_PCI_MODULE_NAME);

	// Return the number of cards found
	DPF1(("DGL: \tbuild_monkey_list found %d\n",card_count));
	return card_count;

} // end of build_monkey_list


/*------------------------------------------------------------------------

	free_all_monkeys
	
------------------------------------------------------------------------*/

void free_all_monkeys(void)
{
	PMONKEYINFO pMI,pTemp;

	DPF2(("DGL: free_all_monkeys\n"));

	// 
	// Set all the cards to 48 kHz so they don't click on reloading
	// 
	pMI = pMonkeyBarrel;
	while (pMI != NULL)
	{
		if (0 == (pMI->dwFlags & PMI_FLAG_BADBOARD))
		{
			//pMI->dwSampleRate = 48000;
			//SetSampleRate(pMI);
			CT->SetSampleRate(pMI,48000);
			
			reset_card(pMI);
		}
		pMI = pMI->pNext;
	}

	//
	// Free the stuff for each monkey
	//
	pMI = pMonkeyBarrel;
	while (pMI != NULL)
	{
		// Unhook the interrupt
		remove_io_interrupt_handler( (long) pMI->bIRQ,
									 CT->InterruptHandler,
									 pMI);
		DPF2(("DGL: \tUnhooked IRQ %d\n",(long) pMI->bIRQ));
		
		// Delete the semaphores
		delete_sem(pMI->IrqSem);
		delete_sem(pMI->ControlSem);
		
		// free the area for the hardware registers
		delete_area(pMI->HardwareRegsAreaID);

		// free the comm page
		delete_area(pMI->CommPageAreaID);
		
		// free the audio buffers
		destroy_audio_buffers(pMI);

		// free the monkey info
		pTemp = pMI;
		pMI = pMI->pNext;
		free(pTemp);
	}	

	pMonkeyBarrel = NULL;

} // end of free_all_monkeys

/*------------------------------------------------------------------------

	create_monkey_info
	
	Builds and initializes a monkey info structure
	
------------------------------------------------------------------------*/

PMONKEYINFO create_monkey_info(	pci_info *pci_card_info, 
								ushort subsystem_id,
								uint32 card_count)
{
	char 		temp_str[MAX_DEVICE_NAME+1],*name;
	PMONKEYINFO pMI,pTemp;
	
	DPF2(("DGL: create_monkey_info\n"));

	// Note that calling malloc from a kernel driver 
	// guarantees locked memory
	DPF2(("DGL\tAllocating %d bytes for monkey info\n",sizeof(MONKEYINFO)));	
	pMI = (PMONKEYINFO) malloc(sizeof(MONKEYINFO));
	if (NULL == pMI)
	{
		dprintf(("DGL: \tFailed to allocate monkey info"));
		return NULL;
	}
	DPF2(("DGL:\tpMI is 0x%x\n",(uint32) pMI));

	//
	// Initialize the structure
	//	
	memset(pMI,0,sizeof(MONKEYINFO));

	// PCI fields	
	pMI->wCardType = subsystem_id & 0xfff0;
	pMI->wCardRev = subsystem_id & 0xf;
	pMI->dwMonkeyPhysAddr = pci_card_info->u.h0.base_registers[0];
	pMI->pNext = NULL;
	
	// Set up the CARDTYPE pointer
	switch (pMI->wCardType)
	{
		case DARLA :
			pMI->ct = &DarlaCard;
			break;
		case GINA :
			pMI->ct = &GinaCard;
			break;
		case LAYLA : 
			pMI->ct = &LaylaCard;
			break;			
		case DARLA24 :
		 	pMI->ct = &Darla24Card;
		 	break;
	}

	// Name the card
	name = CT->szName;
	sprintf(pMI->szCardBaseName,"%s/%lu",name,card_count + 1);
	sprintf(pMI->szCardDeviceName,"audio/multi/%s",pMI->szCardBaseName);
	sprintf(pMI->szFriendlyName,"%s-%lu",name,card_count+1);
	pMI->szFriendlyName[0] = toupper(pMI->szFriendlyName[0]);
	DPF2(("Friendly name is %s\n",pMI->szFriendlyName));

	// Set up the control semaphore
	sprintf(temp_str,"%s open sem",pMI->szCardBaseName);
	pMI->ControlSem = create_sem(1,temp_str);
	set_sem_owner(pMI->ControlSem, B_SYSTEM_TEAM);
	
	// Init more stuff
	pMI->wInputClock = CLK_CLOCKININTERNAL;
	pMI->wOutputClock =	CLK_CLOCKOUTWORD;
	pMI->dwSampleRate = 48000;
	pMI->bGDCurrentClockState = GD_CLOCK_UNDEF;
	pMI->bGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;
	pMI->bGDCurrentResamplerState = GD_RESAMPLE_UNDEF;
	pMI->bASICLoaded = FALSE;
	pMI->dwSingleBufferSizeSamples = DEFAULT_SINGLE_BUFFER_SIZE_SAMPLES;
	
	// Point to the jack count for this card
	JC = CT->jc;
	pMI->node_type_counts = CT->node_type_counts;
	pMI->channel_map = CT->channel_map;
							
	// 
	// Add the structure to the linked list
	//
	if (NULL == pMonkeyBarrel)
	{
		pMonkeyBarrel = pMI;
	}
	else
	{
		pTemp = pMonkeyBarrel;
		while (pTemp->pNext != NULL)
			pTemp = pTemp->pNext;
		pTemp->pNext = pMI;	
	}
	
	return pMI;

} // end of create_monkey_info

/*------------------------------------------------------------------------

	create_comm_page
	
	Builds and initializes a monkey info structure
	
------------------------------------------------------------------------*/

PCOMMPAGE create_comm_page(PMONKEYINFO pMI)
{
	area_id 		temp_area_id;
	char 			temp_str[MAX_DEVICE_NAME+1];
	PCOMMPAGE 		pCommPage;
	uint32			i;
	physical_entry	cp_phys_entry;

	DPF2(("DGL: create_comm_page\n"));
	
	//
	// Create the area
	// 
	sprintf(temp_str,"%s commpage",pMI->szCardBaseName);
	DPF2(("DGL\tAllocating %d bytes for comm page\n",CP_SIZE_IN_PAGES));
	temp_area_id = create_area(	temp_str,
								(void **) &pCommPage,
								B_ANY_KERNEL_ADDRESS,
								CP_SIZE_IN_PAGES,
								B_FULL_LOCK,
								B_READ_AREA | B_WRITE_AREA);
	if (temp_area_id < 0)
	{
		dprintf("DGL: \tFailed to allocate comm page: error code 0x%x\n",temp_area_id);
		return NULL;
	}
	DPF2(("DGL:\tpCommPage is 0x%x\n",(uint32) pCommPage));
	
	pMI->pCommPage = pCommPage;
	pMI->CommPageAreaID = temp_area_id;
	
	//
	// Map the comm page to a physical address
	//
	get_memory_map(pCommPage,sizeof(COMMPAGE),&cp_phys_entry,1);
	pMI->dwCommPagePhys = (DWORD) cp_phys_entry.address;
	DPF2(("DGL: \tComm page physical address is 0x%x\n",cp_phys_entry.address));
	
#ifdef DEBUG

	if (0 == pMI->dwCommPagePhys)
	{
		dprintf(("DGL: \tComm page physical address is 0!\n"));
		return NULL;
	}

#endif

	//	
	// Initialize the comm page
	//
	memset(pCommPage,0,sizeof(COMMPAGE));
	
	for (i = 0; i < MONITOR_ARRAY_SIZE; i++)
	{
		pCommPage->Monitors[i] = 0x80;	// 0x80 means mute (-128 dB)
		pCommPage->MonitorCopy[i] = 0x80;
	}
	pCommPage->dwHandshake = B_HOST_TO_LENDIAN_INT32(0xffffffff);
	
#if B_HOST_IS_LENDIAN
	pCommPage->dwAudioFormat[ 0 ] = B_HOST_TO_LENDIAN_INT32(AUDIOFORM_32LE) ;	
#else
	pCommPage->dwAudioFormat[ 0 ] = B_HOST_TO_LENDIAN_INT32(AUDIOFORM_32BE) ;	
#endif

	pCommPage->wInputClock =
					B_HOST_TO_LENDIAN_INT16( CLK_CLOCKININTERNAL );
	pCommPage->wOutputClock = 
					B_HOST_TO_LENDIAN_INT16( CLK_CLOCKOUTWORD );
	
	return pCommPage;
} // end of create_comm_page

/*------------------------------------------------------------------------

	create_audio_buffers
	
------------------------------------------------------------------------*/

status_t create_audio_buffers(PMONKEYINFO pMI)
{
	DWORD			dwAudioPhys;
	area_id			temp_area_id;
	uint8			*pbOutAudioBuffers;
	uint8			*pbInAudioBuffers;	
	uint8 			*pbDucks;
	PPLE			pDuck;
	DWORD			dwAreaSize;
	char 			temp_str[MAX_DEVICE_NAME+1];	
	PCOMMPAGE		pCommPage;
	int32			i;
	int32			channel;
	DWORD			dwSingleBufferSizeSamples;
	DWORD			dwSingleBufferSizeBytes;
	DWORD			dwDoubleBufferSizeBytes;
	
	DPF2(("DGL: create_audio_buffers\n"));
	
	dwSingleBufferSizeSamples = pMI->dwSingleBufferSizeSamples;
	dwSingleBufferSizeBytes = dwSingleBufferSizeSamples * BYTES_PER_SAMPLE;
	dwDoubleBufferSizeBytes = dwSingleBufferSizeBytes << 1;

	// create an area for the output audio buffers
#ifdef DEBUG
	if (dwDoubleBufferSizeBytes > B_PAGE_SIZE)
	{
		dprintf("Audio buffers too big: %d\n",dwDoubleBufferSizeBytes);	
	}
#endif	

	sprintf(temp_str,"%s out buffers",pMI->szCardBaseName);
	dwAreaSize = PAGE_SIZE_ROUND_UP(dwDoubleBufferSizeBytes * JC->NumOut);
	DPF2(("DGL:\tAllocating %d bytes for audio output buffers\n",dwAreaSize));
	temp_area_id = create_area(	temp_str,
									(void **) &pbOutAudioBuffers,
									B_ANY_KERNEL_ADDRESS,
									dwAreaSize,
									B_FULL_LOCK,
									B_READ_AREA | B_WRITE_AREA);
	if (temp_area_id < 0)
	{
		dprintf("DGL: \tFailed to allocate output audio buffers: error code 0x%x\n",temp_area_id);
		return B_ERROR;
	}
	memset(pbOutAudioBuffers,0,dwAreaSize);

	DPF2(("DGL:\tpbOutAudioBuffers at 0x%x\n",(uint32) pbOutAudioBuffers));
	
	channel = 0;
	for (i = 0; i < JC->NumOut; i++)
	{
		pMI->pAudio[channel][0] = (int32 *) (pbOutAudioBuffers + i*dwDoubleBufferSizeBytes);
		pMI->pAudio[channel][1] = pMI->pAudio[channel][0] + dwSingleBufferSizeSamples;
		DPF3((	"DGL:\tAudio out buffer %d 0x%x,0x%x\n",
				channel,
				pMI->pAudio[channel][0],
				pMI->pAudio[channel][1]));
		channel++;
	}
	pMI->OutAudioBufferAreaID = temp_area_id;
	
	// create an area for the input audio buffers
	sprintf(temp_str,"%s in buffers",pMI->szCardBaseName);
	dwAreaSize = PAGE_SIZE_ROUND_UP(dwDoubleBufferSizeBytes * JC->NumIn);
	DPF2(("DGL:\tAllocating %d bytes for audio input buffers\n",dwAreaSize));
	temp_area_id = create_area(	temp_str,
									(void **) &pbInAudioBuffers,
									B_ANY_KERNEL_ADDRESS,
									dwAreaSize,
									B_FULL_LOCK,
									B_READ_AREA | B_WRITE_AREA);
	if (temp_area_id < 0)
	{
		dprintf("DGL: \tFailed to allocate input audio buffers: error code 0x%x\n",temp_area_id);
		return B_ERROR;
	}
	memset(pbInAudioBuffers,0,dwAreaSize);

	DPF2(("DGL:\tpbInAudioBuffers at 0x%x\n",(uint32) pbInAudioBuffers));
	for (i = 0; i < JC->NumIn; i++)
	{
		pMI->pAudio[channel][0] = (int32 *) (pbInAudioBuffers + i*dwDoubleBufferSizeBytes);
		pMI->pAudio[channel][1] = pMI->pAudio[channel][0] + dwSingleBufferSizeSamples;
		DPF3((	"DGL:\tAudio in buffer %d 0x%x,0x%x\n",
				i,
				pMI->pAudio[channel][0],
				pMI->pAudio[channel][1]));
		channel++;
	}
	pMI->InAudioBufferAreaID = temp_area_id;


	// Create an area for the ducks.  A duck is ten doublewords == 40 bytes, so round up
	// to 64 bytes per duck. I know this is wonky, but I'll make it a real data structure
	// later...
	sprintf(temp_str,"%s ducks",pMI->szCardBaseName);
	dwAreaSize = PAGE_SIZE_ROUND_UP(JC->NumJacks * DUCK_SIZE_ALLOC);	
	DPF2(("DGL:\tAllocating %d bytes for ducks\n",dwAreaSize));	
	temp_area_id = create_area(	temp_str,
								(void **) &pbDucks,
								B_ANY_KERNEL_ADDRESS,
								dwAreaSize,
								B_FULL_LOCK,
								B_READ_AREA | B_WRITE_AREA);
	if (temp_area_id < 0)
	{
		dprintf("DGL: \tFailed to allocate ducks: error code 0x%x\n",temp_area_id);
		return B_ERROR;
	}
	memset(pbDucks,0,dwAreaSize);

	DPF2(("DGL:\tpDuck at 0x%x\n",(uint32) pbDucks));
	
	// store the logical pointers for the ducks
	for (i = 0; i < JC->NumJacks; i++)
	{
		pMI->pDuck[i] = (PPLE) (pbDucks + DUCK_SIZE_ALLOC*i);
		DPF3(("DGL:\tDuck %2d 0x%x\n",i,pMI->pDuck[i]));		
	}
	pMI->DuckAreaID = temp_area_id;
	
	// map the ducks to physical memory and fill out the comm page
	pCommPage = pMI->pCommPage;	
	for (i=0; i < JC->NumJacks; i++)
	{
		pCommPage->dwDuckListPhys[i] = 
			B_HOST_TO_LENDIAN_INT32(get_physical_address(pMI->pDuck[i],DUCK_SIZE_ALLOC));
		DPF3(("DGL:\tDuck physical %2d 0x%x\n",i,
				B_LENDIAN_TO_HOST_INT32(pCommPage->dwDuckListPhys[i])));		
	}
	
	// Fill out the ducks
	for (i = 0; i<JC->NumJacks; i++)
	{
		pDuck = pMI->pDuck[i];
		dwAudioPhys = get_physical_address(pMI->pAudio[i][0],dwDoubleBufferSizeBytes);
		
		pDuck[0].dwPhysicalAddress = B_HOST_TO_LENDIAN_INT32(dwAudioPhys);
		pDuck[0].dwLength = B_HOST_TO_LENDIAN_INT32(dwSingleBufferSizeBytes);	
		pDuck[1].dwPhysicalAddress = 0;
		pDuck[1].dwLength = 0;
		pDuck[2].dwPhysicalAddress = 
			B_HOST_TO_LENDIAN_INT32(dwAudioPhys + dwSingleBufferSizeBytes);
		pDuck[2].dwLength = B_HOST_TO_LENDIAN_INT32(dwSingleBufferSizeBytes);	
		pDuck[3].dwPhysicalAddress = 0;
		pDuck[3].dwLength = 0;
		pDuck[4].dwPhysicalAddress = 
			B_HOST_TO_LENDIAN_INT32(pCommPage->dwDuckListPhys[i]);
		pDuck[4].dwLength = 0;
	}
	
#ifdef DEBUG
	for (i = 0; i<JC->NumJacks; i++)
	{
		DPF3(("DGL:\tDuck %2d:\t",i));

		pDuck = pMI->pDuck[i];
		DPF3(("0x%08x 0x%08x ",
			B_LENDIAN_TO_HOST_INT32(pDuck[0].dwPhysicalAddress),
			B_LENDIAN_TO_HOST_INT32(pDuck[0].dwLength)));
		DPF3((" 0x%08x 0x%08x\n",
			B_LENDIAN_TO_HOST_INT32(pDuck[1].dwPhysicalAddress),
			B_LENDIAN_TO_HOST_INT32(pDuck[1].dwLength)));
		DPF3(("\t\t\t0x%08x 0x%08x ",
			B_LENDIAN_TO_HOST_INT32(pDuck[2].dwPhysicalAddress),
			B_LENDIAN_TO_HOST_INT32(pDuck[2].dwLength)));
		DPF3((" 0x%08x 0x%08x\n",
			B_LENDIAN_TO_HOST_INT32(pDuck[3].dwPhysicalAddress),
			B_LENDIAN_TO_HOST_INT32(pDuck[3].dwLength)));
		DPF3(("\t\t\t0x%08x 0x%08x\n",
			B_LENDIAN_TO_HOST_INT32(pDuck[4].dwPhysicalAddress),
			B_LENDIAN_TO_HOST_INT32(pDuck[4].dwLength)));		
	}
	
	
#endif
	return B_OK;
	
} // end of create_audio_buffers

/*------------------------------------------------------------------------

	destroy_audio_buffers
	
------------------------------------------------------------------------*/

void destroy_audio_buffers(PMONKEYINFO pMI)
{

	delete_area(pMI->OutAudioBufferAreaID);
	delete_area(pMI->InAudioBufferAreaID);		
	delete_area(pMI->DuckAreaID);

} // destroy_audio_buffers
	
	
/*------------------------------------------------------------------------

	init_midi
	
	Initialize the MIDI driver for this card
	
------------------------------------------------------------------------*/

void init_midi(PMONKEYINFO pMI)
{
	sprintf(pMI->szMidiDeviceName,"midi/%s",pMI->szCardBaseName);

	// Add the card name to the list of published devices
	device_names[device_name_index] = pMI->szMidiDeviceName;
	device_name_index++;

} // init_midi


/*------------------------------------------------------------------------

	init_single_card
	
	Adds a single card to the monkey barrel
	
------------------------------------------------------------------------*/

status_t init_single_card
(	
	pci_module_info *pmodule,
	pci_info *pci_card_info,
	uint32 card_count,
	uint32 card_type_count,
	bool load_dsp,
	ushort subsystem_id
)
{
	char 				temp_str[MAX_DEVICE_NAME+1];
	PMONKEYINFO 	pMI;
	area_id 			temp_area_id;
	DWORD 			*pMonkeyBase;
	uint32 			pci_config_temp;
	long				rval;
	
	DPF2(("DGL: init_single_card\n"));

	// 
	// Test the physical hardware base address
	//
	if (0 == pci_card_info->u.h0.base_registers[0])
	{
		dprintf(("DGL: init_single_card found zero base hardware address\n"));
		return B_ERROR;
	}

	//
	// Allocate a monkey info structure
	//
	pMI = create_monkey_info(pci_card_info,subsystem_id,card_type_count);
	if (NULL == pMI)
	{
		return B_ERROR;
	}

	// Allocate a comm page
	if (NULL == create_comm_page(pMI))
	{
		DPF2(("DGL: \tFailed to create the comm page\n"));	
		return B_ERROR;
	}
	
	// Create the audio buffers and the ducks
	if (B_OK != create_audio_buffers(pMI))
	{
		DPF2(("DGL: \tFailed to create audio buffers and ducks\n"));
		return B_ERROR;
	}
	
	//
	// Set up the latency timer to 66 PCI cycles
	//
	pci_config_temp = pmodule->read_pci_config(	
								pci_card_info->bus,
								pci_card_info->device,
								pci_card_info->function,
								PCI_latency & 0xfc,
								4);
	pci_config_temp &= 0xffff00ff;	// Mask off latency timer byte
	pci_config_temp |= 0x00004200;	// 0x42 is 66 PCI cycles == 2 microseconds

	pmodule->write_pci_config(	pci_card_info->bus,
								pci_card_info->device,
								pci_card_info->function,
								PCI_latency & 0xfc,
								4,
								pci_config_temp);

	// 
	// Make sure the bus master is enabled and that memory space accesses are enabled
	//
	pci_config_temp = pmodule->read_pci_config(	
								pci_card_info->bus,
								pci_card_info->device,
								pci_card_info->function,
								PCI_command & 0xfc,
								4);

	pci_config_temp	|= PCI_command_memory | PCI_command_master;	
	
	pmodule->write_pci_config(	pci_card_info->bus,
								pci_card_info->device,
								pci_card_info->function,
								PCI_command & 0xfc,
								4,
								pci_config_temp);
	
	//
	// Map the physical hardware base
	//
	pMonkeyBase = NULL;
	sprintf(temp_str,"%s hardware",pMI->szCardBaseName);
	temp_area_id = map_physical_memory( temp_str,
										(void *) pci_card_info->u.h0.base_registers[0],
										B_PAGE_SIZE,
										B_ANY_KERNEL_ADDRESS,
										B_READ_AREA | B_WRITE_AREA,
										(void **) &pMonkeyBase);
	if (temp_area_id < 0)
	{
		dprintf("DGL: \tFailed to map physical memory for %s\n",pMI->szCardBaseName);
		return B_ERROR;
	}
	pMI->HardwareRegsAreaID = temp_area_id;
	pMI->pMonkeyBase = (DWORD *) pMonkeyBase;
	DPF2(("DGL: \tMapped hardware to 0x%x\n",(DWORD) pMonkeyBase));
	
	//
	// Hook the hardware interrupt
	//
	pMI->bIRQ = (BYTE) pci_card_info->u.h0.interrupt_line;
	rval = install_io_interrupt_handler( 	
				pci_card_info->u.h0.interrupt_line,
				CT->InterruptHandler, 
				pMI, 
				0);
	if (B_OK != rval)
	{
		dprintf("Failed to hook IRQ %d for %s\n",(int) pMI->bIRQ,pMI->szCardDeviceName);
		dprintf("Error code 0x%x\n",rval);
		return rval;
	}
	DPF2(("DGL:\tHooked IRQ %d for %s\n",(int) pMI->bIRQ,pMI->szCardDeviceName));
	

	//
	// Load the DSP
	//
	if (load_dsp)
	{
		if (B_OK != LoadFirmware(pMI))
		{
			DPF2(("DGL:\tFailed to load DSP\n"));	
			return B_ERROR;
		}
	}
	else
	{
		if (B_OK != SetCommPage(pMI))
		{
			DPF2(("DGL:\tFailed to set comm page\n"));	
			return B_ERROR;
		}
	}
	
	//
	// Set input gain to 0 dB
	//
	setup_input_gain(pMI);
	
	//
	// Add the card name to the list of published devices
	//
	device_names[device_name_index] = pMI->szCardDeviceName;
	device_name_index++;
	
	//
	// If this card has MIDI, set up the MIDI driver
	//
	if (CT->dwNumMidiPorts != 0)
		init_midi(pMI);
	
	return B_OK;
	
} // end of init_single_card



