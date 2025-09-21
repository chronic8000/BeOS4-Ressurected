/*************************************************************************

 util.c
 
 Utility routines to talk to the hardware and other things

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "multi_audio.h"
#include "monkey.h"
#include "56301.h"
#include "commpage.h"
#include "util.h"
#include "api.h"

/*------------------------------------------------------------------------

	SendVector
	
------------------------------------------------------------------------*/

#ifdef DEBUG
bigtime_t	max_timeout = 0;
#endif

status_t SendVector( PMONKEYINFO pMI, DWORD dwCommand)
{
	bigtime_t	timeout;
	DWORD		*pMonkeyBase,dwTemp;
	cpu_status	cs;

#ifdef DEBUG
	char *	pszCmd;
	bigtime_t	start_time;	

	switch ( dwCommand )
	{
		case ACK_INT :
			pszCmd = "ACK_INT";
			break;
		case START_TRANSFER :
			pszCmd = "START_TRANSFER";
			break;
		case CONFIRM_XFER_STATE :
			pszCmd = "CONFIRM_XFER_STATE";
			break;
		case METERS_ON :
			pszCmd = "METERS_ON";
			break;
		case METERS_OFF :
			pszCmd = "METERS_OFF";
			break;
		case UPDATE_OUTGAIN :
			pszCmd = "UPDATE_OUTGAIN";
			break;
		case UPDATE_INGAIN :
			pszCmd = "UPDATE_INGAIN";
			break;
		case ADDBUFF_WAVEOUT0 :
			pszCmd = "ADDBUFF_WAVEOUT0";
			break;
		case UNBLOCK_MIDI :
			pszCmd = "UNBLOCK_MIDI";
			break;
		case UPDATE_CLOCKS :
			pszCmd = "UPDATE_CLOCKS";
			break;
		case SET_LAYLA_SAMPLE_RATE :
			pszCmd = "SET_SAMPLE_RATE";
			break;
		case MIDI_IN_START :
			pszCmd = "MIDI_IN_START";
			break;
		case MIDI_WRITE :
			pszCmd = "MIDI_WRITE";
			break;
		case STOP_TRANSFER :
			pszCmd = "STOP_TRANSFER";
			break;
		case RESET_PEAK_METERS :
			pszCmd = "RESET_PEAK_METERS";
			break;
		case UNBLOCK_IRQ :
			pszCmd = "UNBLOCK_IRQ";
			break;
		case UPDATE_FLAGS :
			pszCmd = "UPDATE_FLAGS";
			break;
		case 0x80ff :
			pszCmd = "Software reset";
			break;
		default :
			pszCmd = "?????";
			break;
	}

	DPF2(("DGL: SendVector pMI %s 0x%x\n"
	     "     dwCommand %s (0x%x)\n",
		pMI->szCardBaseName,
		(DWORD) pMI,
		pszCmd,
		dwCommand));

#endif
	
#ifdef DEBUG
	if (pMI == NULL)
	{
		DPF2(("DGL: SendVector Invalid pMI 0x%x\n",(DWORD) pMI));
		return B_ERROR;
	}
#endif		
	
	pMonkeyBase = pMI->pMonkeyBase;

	cs = disable_interrupts();
	acquire_spinlock(&pMI->dwHardwareSpinlock);

	timeout = system_time() + VECTOR_TIMEOUT;	// system_time returns microseconds

#ifdef DEBUG
	start_time = timeout - VECTOR_TIMEOUT;
#endif	
	
	while (system_time() < timeout)
	{
		dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_VECTOR_REG]);
		if (0 == (dwTemp & VECTOR_READY)) // HI32 HCVR HC
		{
			pMonkeyBase[CHI32_VECTOR_REG] = B_HOST_TO_LENDIAN_INT32(dwCommand);
			__eieio();
			
#ifdef DEBUG
			max_timeout = system_time() - start_time;
#endif			
			release_spinlock(&pMI->dwHardwareSpinlock);
			restore_interrupts(cs);
			return B_OK;
		}
		spin(1);	// Be friendly and let someone else use the bus
	}

	release_spinlock(&pMI->dwHardwareSpinlock);
	restore_interrupts(cs);

#ifdef DEBUG
	dprintf("\tPunked out on SendVector!  Vector register is 0x%x\n",
				B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_VECTOR_REG]));
	max_timeout = system_time() - start_time;				
#endif

	DEBUGBREAK;
	return B_ERROR;
}	// SendVector

/*------------------------------------------------------------------------

	VMONK_Read_DSP
	
------------------------------------------------------------------------*/

status_t VMONK_Read_DSP( PMONKEYINFO pMI, DWORD *pdwData)
{
	DWORD 		dwStatus;
	bigtime_t	timeout;
	PDWORD		pMonkeyBase;

	pMonkeyBase = pMI->pMonkeyBase;
	
	timeout = system_time() + 100000;
	while (timeout > system_time()) 
	{
		dwStatus = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
		if ((dwStatus & HOST_READ_FULL) != 0)
		{
			*pdwData = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_DATA_REG]);
			return MMSYSERR_NOERROR;
		}
	}	
	
	DEBUGBREAK;
	return FAIL;
}	// DWORD	VMONK_Read_DSP( PMONKEYINFO pMI, DWORD *pdwData)


/*------------------------------------------------------------------------

	VMONK_Write_DSP
	
------------------------------------------------------------------------*/

status_t VMONK_Write_DSP( PMONKEYINFO pMI, DWORD dwData )
{
	DWORD 		dwStatus;
	bigtime_t	timeout;
	PDWORD		pMonkeyBase;

	pMonkeyBase = pMI->pMonkeyBase;
	
	timeout = system_time() + 100000;
	while (timeout > system_time()) 
	{
		dwStatus = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
		if ((dwStatus & HOST_WRITE_EMPTY) != 0)
		{
			pMonkeyBase[CHI32_DATA_REG] = B_HOST_TO_LENDIAN_INT32(dwData);
			__eieio();
			return MMSYSERR_NOERROR;
		}

	}	

	DPF1(("Failed on write DSP: status is 0x%x\n",pMonkeyBase[CHI32_STATUS_REG]));	
	return FAIL;
}	// VMONK_Write_DSP

/*------------------------------------------------------------------------

	reset_card
	
------------------------------------------------------------------------*/

void reset_card(PMONKEYINFO pMI)
{
	PCOMMPAGE pCommPage;

	// Stop any audio transfers
	pCommPage = pMI->pCommPage;
	pCommPage->dwStop = B_HOST_TO_LENDIAN_INT32(0xffffffff);
	pCommPage->dwReset = B_HOST_TO_LENDIAN_INT32(0xffffffff);	
	StopDSP(pMI);
	
	// one tenth second snooze; this is to allow any final interrupts
	// to happen.  This will be fixed by changing the DSP code.
	snooze(100 * 1000);
	
	pMI->dwActiveChannels = 0;
}

/*------------------------------------------------------------------------

	reset_buffer_exchange_state
	
------------------------------------------------------------------------*/

void reset_buffer_exchange_state(PMONKEYINFO pMI)
{
	char temp_str[MAX_DEVICE_NAME+1];

	// Stop any audio transfers from the card
	reset_card(pMI);

	// Reset the IRQ semaphore to a count of 1
	sprintf(temp_str,"%s irq sem",pMI->szCardBaseName);
	delete_sem(pMI->IrqSem);	// in case it already exists
	pMI->IrqSem = create_sem(1,temp_str);	
	set_sem_owner(pMI->IrqSem,B_SYSTEM_TEAM);

	// Reset the buffer counts
	pMI->dwActualBufferCount = 0;
	
	// Reset the buffer timestamp
	pMI->dwSampleTimestamp = 0;
	pMI->dwLastSampleTimestamp = 0;
	pMI->qwSystemTimestamp = 0;
	pMI->qwSampleTimestamp = 0;
	
	// Zero the audio buffers
	memset(pMI->pAudio[0][0],0,	pMI->dwSingleBufferSizeSamples *
								BYTES_PER_SAMPLE * 2 * 
								JC->NumOut);

} // end of reset_buffer_exchange_state

/*------------------------------------------------------------------------

	get_physical_address
	
------------------------------------------------------------------------*/

DWORD get_physical_address(void *pLogical,DWORD dwSize)
{
	physical_entry	temp_phys_entry;

	get_memory_map(pLogical,dwSize,&temp_phys_entry,1);
	
	return (DWORD) temp_phys_entry.address;
	
} // end of get_physical_address	
	
	
/*------------------------------------------------------------------------

	setup_input_gain
	
	This function will go away
	
------------------------------------------------------------------------*/

void setup_input_gain(PMONKEYINFO pMI)	
{
	PCOMMPAGE pCommPage;
	int index;
	
	DPF2(("DGL: Setting input gain to 0 dB\n"));
	
	pCommPage = pMI->pCommPage;

	// Set up inputs for 0 dB gain
	for (	index = JC->NumOut; 
			index < (JC->NumAnalogIn + JC->NumOut);
			index++)
	{
			pCommPage->LineLevel[index] = 0xc8;
	}
	SendVector(pMI,UPDATE_INGAIN);

}  // end of setup_input_gain

/*------------------------------------------------------------------------

	add_ma_client
	
	Add a client for the multi_audio driver
	
------------------------------------------------------------------------*/

PMA_CLIENT add_ma_client(PMONKEYINFO pMI,uint32 flags)
{
	MA_CLIENT *pClient;
	bool	fFullAccess;

	fFullAccess = true;	
	if (B_MULTI_CONTROL == flags)
		fFullAccess = false;

	// If this is a full access client, make sure no other client has
	// full access
	if (fFullAccess)
	{
		pClient = pMI->ma_clients;
		while (NULL != pClient)
		{
			if (pClient->fFullAccess)
				return NULL;

			pClient = pClient->pNext;		
		}
	}

	pClient = (MA_CLIENT *) malloc(sizeof(MA_CLIENT));
	if (NULL == pClient)
	{
		dprintf("DarlaGinaLayla: \tmalloc fail trying to add client\n");
		return NULL;
	}
	
	DPF2(("DGL: \tAdded client 0x%x (full access 0x%x) to %s\n",
			(DWORD) pClient,fFullAccess,pMI->szCardBaseName));
		
	pClient->pMI = pMI;
	pClient->fFullAccess = fFullAccess;
	pClient->pNext = pMI->ma_clients;
	
	pMI->ma_clients = pClient;
	
	return pClient;
} // add_ma_client


/*------------------------------------------------------------------------

	remove_ma_client
	
------------------------------------------------------------------------*/

void remove_ma_client(PMONKEYINFO pMI,PMA_CLIENT pClient)
{
	PMA_CLIENT pTemp;
	
	// Special case- removing head of list
	if (pMI->ma_clients == pClient)
	{
		pMI->ma_clients = pClient->pNext;
		free(pClient);

		DPF2(("DGL: \tRemoved client 0x%x from %s\n",
			(DWORD) pClient,pMI->szCardBaseName));

		return;
	}
	
	// Not removing head of list
	pTemp = pMI->ma_clients;
	while (NULL != pTemp)
	{
		if (pClient == pTemp->pNext)
			break;
			
		pTemp = pTemp->pNext;
	}
	
	if (NULL != pTemp)
	{
		pTemp->pNext = pClient->pNext;
		free(pClient);		

		DPF2(("DGL: \tRemoved client 0x%x from %s\n",
			(DWORD) pClient,pMI->szCardBaseName));
	}
	
} // remove_ma_client

/*------------------------------------------------------------------------

	add_midi_client
	
	Add a client for the MIDI driver
	
------------------------------------------------------------------------*/

PMIDI_CLIENT add_midi_client(PMONKEYINFO pMI)
{
	MIDI_CLIENT *pClient,*pTemp;
	char		temp_str[33];
	cpu_status	cs;	
	
	if (MAX_DEVICES == pMI->dwNumMidiClients)
		return NULL;
	
	// 
	// Create the client
	//
	pClient = (MIDI_CLIENT *) malloc(sizeof(MIDI_CLIENT));
	if (NULL == pClient)
	{
		dprintf("DarlaGinaLayla: \tmalloc fail trying to add MIDI client\n");
		return NULL;
	}
	
	DPF2(("DGL: \tAdded MIDI client 0x%x to %s\n",
			(DWORD) pClient,pMI->szCardBaseName));

	// Initialize the semaphore for this client
	sprintf(temp_str,"%s MIDI sem",pMI->szCardBaseName);
	pClient->DataSem = create_sem(0,temp_str);
	set_sem_owner(pClient->DataSem, B_SYSTEM_TEAM);
	
	// Create the MIDI input buffer for this client
	pClient->pBufferBase = (uint8 *) malloc(MIDI_INPUT_BUFFER_SIZE);
	if (NULL == pClient->pBufferBase)
	{
		remove_midi_client(pMI,pClient);
		return NULL;
	}
	
	pClient->pBufferWrite = pClient->pBufferBase;
	pClient->pBufferRead = pClient->pBufferBase;	
	pClient->pBufferEnd = pClient->pBufferBase + MIDI_INPUT_BUFFER_SIZE;
	
	DPF2(("DGL: \tMIDI buffer %p, semaphore 0x%x, buffer end %p\n",
			pClient->pBufferBase,pClient->DataSem,pClient->pBufferEnd));

	//
	// Add the client to the list
	//
	pTemp = pMI->midi_clients;
		
	cs = disable_interrupts();
	acquire_spinlock(&pMI->dwHardwareSpinlock);

	pClient->pMI = pMI;
	pClient->pNext = pMI->midi_clients;
	pMI->midi_clients = pClient;
	
	release_spinlock(&pMI->dwHardwareSpinlock);
	restore_interrupts(cs);
	
	pMI->dwNumMidiClients++;
			
	//
	// Turn on MIDI input?
	//
	if (NULL == pTemp)
	{
		PCOMMPAGE pCommPage;
		
		pCommPage = pMI->pCommPage;
		pCommPage->dwFlags |= CPFLAG_MIDI_INPUT;
		SendVector(pMI,UPDATE_FLAGS);
	}
	
	return pClient;
} // add_midi_client


/*------------------------------------------------------------------------

	remove_midi_client
	
------------------------------------------------------------------------*/

void remove_midi_client(PMONKEYINFO pMI,PMIDI_CLIENT pClient)
{
	PMIDI_CLIENT 	pTemp;
	status_t		rval;
	cpu_status		cs;	
	
	cs = disable_interrupts();
	acquire_spinlock(&pMI->dwHardwareSpinlock);

	// Special case- removing head of list
	if (pMI->midi_clients == pClient)
	{
		pMI->midi_clients = pClient->pNext;

		DPF2(("DGL: \tRemoved MIDI client 0x%x from %s\n",
			(DWORD) pClient,pMI->szCardBaseName));
	}
	else
	{
	
		// Not removing head of list
		pTemp = pMI->midi_clients;
		while (NULL != pTemp)
		{
			if (pClient == pTemp->pNext)
				break;
				
			pTemp = pTemp->pNext;
		}
		
		if (NULL != pTemp)
		{
			pTemp->pNext = pClient->pNext;
	
			DPF2(("DGL: \tRemoved MIDI client 0x%x from %s\n",
				(DWORD) pClient,pMI->szCardBaseName));
		}
	}
	
	release_spinlock(&pMI->dwHardwareSpinlock);
	restore_interrupts(cs);

	// Free the MIDI input buffer
	free(pClient->pBufferBase);
	
	// Delete the semaphore
	rval = delete_sem(pClient->DataSem);
	if (B_OK != rval)
	{
		dprintf("DGL: Failed to delete semaphore 0x%x: error code 0x%x\n",
				pClient->DataSem,rval);
	}
	
	// Free the client struct
	free(pClient);
	
	pMI->dwNumMidiClients--;
		
	//
	// Turn off MIDI input?
	//
	if (NULL == pMI->midi_clients)
	{
		PCOMMPAGE pCommPage;
		
		pCommPage = pMI->pCommPage;
		pCommPage->dwFlags &= ~CPFLAG_MIDI_INPUT;
		SendVector(pMI,UPDATE_FLAGS);
	}

	
} // remove_midi_client


/*------------------------------------------------------------------------

	write_midi_bytes
	
------------------------------------------------------------------------*/

#define HANDSHAKE_TIMEOUT	500

size_t write_midi_bytes(PMONKEYINFO pMI, uint8 const *buffer,size_t bytes_requested)
{
	PCOMMPAGE	pCommPage;
	DWORD		dwLengthTemp, dwShift, dwPacked, i;
	size_t		bytes_written;
	int32		index;
	
	DPF2(("\n\nwrite_midi_bytes ptr %p bytes %ld\n",buffer,bytes_requested));

	pCommPage = (PCOMMPAGE) pMI->pCommPage;

	bytes_written = 0;
	index = 0;
	while (bytes_requested > 0 )
	{
		if ( bytes_requested > 3 )
			dwLengthTemp = 3;
		else
			dwLengthTemp = bytes_requested;
		
		dwShift = 16;
		dwPacked = 0;
		for ( i = 0; i < dwLengthTemp; i++ )
		{
			dwPacked |= (DWORD) buffer[index] << dwShift;
			index++;
			dwShift -= 8;
		}

		dwPacked |= dwLengthTemp << 24;		
		bytes_requested -= dwLengthTemp;

		//
		// Wait up to two milliseconds for the handshake from the DSP 
		//
		for (i=0; i < HANDSHAKE_TIMEOUT; i++)
		{
			// Look for the handshake value
			if ( 0 != pCommPage->dwHandshake )
				break;

			// Give the DSP time to access the comm page
				snooze(2);
		}

		if (0 == pCommPage->dwHandshake)
		{
			dprintf("DarlaGinaLayla: Time-out waiting for DSP to consume MIDI"
					"data\n"); 
			return bytes_written;
		}

		//
		// So send the packed MIDI bytes already
		//
		pCommPage->dwHandshake = 0;	// Clear the handshake
		pCommPage->dwMIDIOutData = B_HOST_TO_LENDIAN_INT32(dwPacked);
		SendVector( pMI, MIDI_WRITE );
		
		bytes_written += dwLengthTemp;

	} // dwLength 

	return bytes_written;
} // write_midi_bytes


