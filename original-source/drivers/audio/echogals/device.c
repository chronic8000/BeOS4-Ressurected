/*************************************************************************

 device.c
 
 Entry points for each multi_audio device handled by this driver.

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>
#include "monkey.h"
#include "commpage.h"
#include "util.h"
#include "multi_audio.h"
#include "multi_control.h"

extern PMONKEYINFO pMonkeyBarrel;

/*------------------------------------------------------------------------

	device_open - handle open() calls

------------------------------------------------------------------------*/

static status_t device_open (const char *name, 
							 uint32 flags, 
							 void** cookie)
{
	PMONKEYINFO pMI;
	status_t	rval;
	PMA_CLIENT		pClient;
	
	DPF_ON;
	DPF2(("\n\n\nDGL: device_open %s\n",name));
	
	// Walk through the monkey barrel and find a matching name	
	pMI = pMonkeyBarrel;
	rval = B_ERROR;
	while (NULL != pMI)
	{
		if (0 == strcmp(pMI->szCardDeviceName,name))
		{
			rval = B_OK;
			break;
		}
		pMI = pMI->pNext;
	}

	if (B_ERROR == rval)
	{
		DPF2(("DGL:\tFailed to match name %s\n",name));
		return rval;
	}
	
	// Is the card OK?
	if (0 != (pMI->dwFlags & PMI_FLAG_BADBOARD))
	{
		DPF1(("DGL: Attempted to open a bad board %s\n",pMI->szCardDeviceName));
		return B_NOT_ALLOWED;
	}

	// Serialize the control requests
	if (B_OK != acquire_sem(pMI->ControlSem))
	{
		DPF2(("DGL:\tFailed to acquire control semaphore!\n"));
		return B_ERROR;
	}

	// Make a client for this guy
	pClient = add_ma_client(pMI,flags);
	if (NULL == pClient)
	{
		DPF2(("DGL: \tFailed to open %s\n",name));	
		rval = B_NOT_ALLOWED;
	}
	else
	{
		DPF1(("DGL: \tOpened %s\n",pMI->szCardDeviceName));
		*cookie = pClient;
		
		rval = B_OK;

		// Initialize the buffer exchange state for full access
		// clients
		if (pClient->fFullAccess)
			reset_buffer_exchange_state(pMI);
	}
	
	// Release the semaphore
	if (B_OK != release_sem(pMI->ControlSem))
	{
		DPF2(("DGL:\tSomehow release_sem for control semaphore failed\n"));
		rval = B_ERROR;
	}
	
	DPF_OFF;
	return rval;

} // end of device_open


/*------------------------------------------------------------------------

	device_read - handle read() calls

------------------------------------------------------------------------*/

static status_t device_read (void* cookie, 
							 off_t position, 
							 void *buf, 
							 size_t* num_bytes)
{
	DPF_ON;
	DPF2(("DGL: device_read\n"));

	*num_bytes = 0;				/* tell caller nothing was read */

	DPF_OFF;
	return B_IO_ERROR;

} // end of device_read


/*------------------------------------------------------------------------
	device_write - handle write() calls
------------------------------------------------------------------------*/

static status_t device_write(void* cookie, 
							 off_t position, 
							 const void* buffer, 
  							 size_t* num_bytes)
{

	DPF_ON;
	DPF2(("DGL: device_write\n"));

	*num_bytes = 0;				/* tell caller nothing was written */

	DPF_OFF;
	return B_IO_ERROR;

} // end of device_write


/*------------------------------------------------------------------------

	device_control - handle ioctl calls

------------------------------------------------------------------------*/

static status_t device_control (void* cookie, 
								uint32 op, 
								void* arg, 
								size_t len)
{
	PMA_CLIENT		pClient;
	PMONKEYINFO pMI;
	status_t	rval;

	pClient = (PMA_CLIENT) cookie;
	pMI = pClient->pMI;

	DPF_ON;
	
	rval = B_OK; // Keep the compiler from whining
	
	// Optimize for B_MULTI_BUFFER_EXCHANGE
	if (B_MULTI_BUFFER_EXCHANGE == op)
	{
		if (pClient->fFullAccess)
		{
			rval = multiBufferExchange(pMI,(multi_buffer_info *) arg);
		}
		else
		{
			rval = B_NOT_ALLOWED;
		}
	}
	else
	{
		// Serialize the control requests
		if (B_OK != acquire_sem(pMI->ControlSem))
		{
			DPF2(("DGL:\tFailed to acquire control semaphore!\n"));
			return B_ERROR;
		}
	
		DPF3(("DGL: device_control %s\n",pMI->szCardBaseName));
	
		switch (op)
		{
			case B_MULTI_GET_DESCRIPTION : 
				rval = multiGetDescription(pMI,(multi_description *) arg);
				break;
			case B_MULTI_GET_ENABLED_CHANNELS :
				rval = multiGetEnabledChannels(pMI,(multi_channel_enable *) arg);
				break;
			case B_MULTI_SET_ENABLED_CHANNELS : 
				if (pClient->fFullAccess)
					rval = multiSetEnabledChannels(pMI,(multi_channel_enable *) arg);
				else
					rval = B_NOT_ALLOWED;
					
				break;
			case B_MULTI_GET_GLOBAL_FORMAT :
				rval = multiGetGlobalFormat(pMI,(multi_format_info *) arg);
				break;
			case B_MULTI_SET_GLOBAL_FORMAT :
				if (pClient->fFullAccess)
				{
					rval = multiSetGlobalFormat(pMI,(multi_format_info *) arg);
				}
				else
				{	
					rval = B_NOT_ALLOWED;
				}
				break;
			case B_MULTI_GET_BUFFERS :
				rval = multiGetBuffers(pMI,(multi_buffer_list *) arg);
				break;
			case B_MULTI_BUFFER_FORCE_STOP : 
				if (pClient->fFullAccess)
				{
					rval = multiBufferForceStop(pMI);
				}
				else
				{
					rval = B_NOT_ALLOWED;	
				}
				break;
			case B_MULTI_GET_MIX :
				rval = multiGetMix(pMI, (multi_mix_value_info *) arg);
				break;
			case B_MULTI_SET_MIX :
				rval = multiSetMix(pMI, (multi_mix_value_info *) arg);
				break;
			case B_MULTI_LIST_MIX_CHANNELS :
				rval = multiListMixChannels(pMI,(multi_mix_channel_info *) arg);
				break;
			case B_MULTI_LIST_MIX_CONNECTIONS : 
				rval = multiListMixConnections(pMI,(multi_mix_connection_info *) arg);
				break;
			case B_MULTI_LIST_MIX_CONTROLS : 
				rval = multiListMixControls(pMI,(multi_mix_control_info *) arg);
				break;
			case B_MULTI_LIST_EXTENSIONS :
				rval = multiListExtensions(pMI,(multi_extension_list *) arg);
				break;
			case B_MULTI_GET_EXTENSION :
				rval = multiGetExtension(pMI,(multi_extension_cmd *) arg);
				break;
			case B_MULTI_SET_EXTENSION :
				rval = multiSetExtension(pMI,(multi_extension_cmd *) arg);
				break;
			default : 
				DPF2(("DGL: \tUnrecognized ioctl\n"));
				rval = B_DEV_INVALID_IOCTL;
				break;			
		}

		// Release the semaphore
		if (B_OK != release_sem(pMI->ControlSem))
		{
			DPF2(("DGL:\tSomehow release_sem for control semaphore failed\n"));
			rval = B_ERROR;
		}

	}

	DPF_OFF;
	return rval;
} // end of device_control


/*------------------------------------------------------------------------

	device_close - handle close() calls

------------------------------------------------------------------------*/

static status_t device_close (void* cookie)
{

	DPF_ON;
	DPF2(("DGL: device_close %s\n",((PMA_CLIENT) cookie)->pMI->szCardBaseName));	

	DPF_OFF;
	return B_OK;

} // end of device_close


/*------------------------------------------------------------------------
	device_free - called after the last device is closed, and after
	all i/o is complete.
------------------------------------------------------------------------*/

static status_t device_free (void* cookie)
{
	PMA_CLIENT		pClient;
	PMONKEYINFO pMI;

	DPF_ON;
	DPF1(("DGL: device_free %s\n",((PMA_CLIENT) cookie)->pMI->szCardBaseName));		

	pClient = (PMA_CLIENT) cookie;
	pMI = pClient->pMI;

	if (pClient->fFullAccess)
	{
		reset_card(pMI);
		pMI->dwEnabledChannels = 0;
	}
	
	remove_ma_client(pMI,pClient);	
	
	DPF_OFF;
	return B_OK;

} // end of device_free



/*------------------------------------------------------------------------

	find_device - return ptr to device hooks structure for a
	given device name

------------------------------------------------------------------------*/

device_hooks echo_device_hooks = {
	device_open, 			/* -> open entry point */
	device_close, 			/* -> close entry point */
	device_free,			/* -> free cookie */
	device_control, 		/* -> control entry point */
	device_read,			/* -> read entry point */
	device_write			/* -> write entry point */
};

extern device_hooks echo_midi_device_hooks;

device_hooks* find_device(const char* name)
{
	PMONKEYINFO pMI;
	
	DPF_ON;
	DPF2(("DGL: find_device %s\n",name));
	DPF_OFF;
	
	// Is this a multi_audio device name?
	pMI = pMonkeyBarrel;
	while (NULL != pMI)
	{
		if (0 == strcmp(pMI->szCardDeviceName,name))
		{
			break;
		}
		pMI = pMI->pNext;
	}

	if (NULL != pMI)
	{
		return &echo_device_hooks;
	}	
	
	// Is this a MIDI device name?
	pMI = pMonkeyBarrel;
	while (NULL != pMI)
	{
		if (0 == strcmp(pMI->szMidiDeviceName,name))
		{
			break;
		}
		pMI = pMI->pNext;
	}

	if (NULL != pMI)
	{
		return &echo_midi_device_hooks;
	}	
	
	return NULL;

} // end of find_device

