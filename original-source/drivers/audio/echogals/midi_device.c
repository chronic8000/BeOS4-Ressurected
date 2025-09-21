/*************************************************************************

 midi_device.c
 
 Entry points for each MIDI device handled by this driver.

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>
#include "monkey.h"
#include "commpage.h"
#include "util.h"

extern PMONKEYINFO pMonkeyBarrel;

/*------------------------------------------------------------------------

	device_open - handle open() calls

------------------------------------------------------------------------*/

static status_t midi_open 
(
	const char *name, 
	uint32 flags, 
	void** cookie
)
{
	PMONKEYINFO 	pMI;
	PMIDI_CLIENT	pClient;
	status_t	rval;

	DPF_ON;
	DPF2(("\n\n\nDGL: midi_open %s\n",name));
	
	// Walk through the monkey barrel and find a matching name	
	pMI = pMonkeyBarrel;
	while (NULL != pMI)
	{
		if (0 == strcmp(pMI->szMidiDeviceName,name))
		{
			break;
		}
		pMI = pMI->pNext;
	}

	if (NULL == pMI)
	{
		DPF2(("DGL:\tFailed to match name %s\n",name));
		return B_ERROR;
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
	pClient = add_midi_client(pMI);
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
	}

	// Release the semaphore
	if (B_OK != release_sem(pMI->ControlSem))
	{
		DPF2(("DGL:\tSomehow release_sem for control semaphore failed\n"));
		rval = B_ERROR;
	}
	
	DPF_OFF;
	return rval;

} // end of midi_open


/*------------------------------------------------------------------------

	midi_read - handle read() calls

------------------------------------------------------------------------*/

size_t copy_midi_bytes
(
	PMIDI_CLIENT pClient,
	uint8 *buffer, 
	size_t num_bytes
)
{
	size_t bytes_copied;

	bytes_copied = 0;

	while (pClient->pBufferWrite != pClient->pBufferRead)
	{
		if (bytes_copied == num_bytes)
			break;
	
		*buffer = *(pClient->pBufferRead);
		buffer++;
		pClient->pBufferRead++;
		bytes_copied++;
		
		// Wrap around the circular buffer
		if (pClient->pBufferRead == pClient->pBufferEnd)
			pClient->pBufferRead = pClient->pBufferBase;
	}
	
	DPF2(("bytes_copied %d\n",bytes_copied));
	return bytes_copied;
}

static status_t midi_read
(	
	void* cookie, 
	off_t position, 
	void *buffer, 
	size_t* num_bytes
)
{
	PMONKEYINFO 	pMI;
	PMIDI_CLIENT	pClient;
	status_t		rval;

	DPF_ON;
	DPF2(("DGL: midi_read %p %d bytes\n",buffer,*num_bytes));
	
	if (NULL == buffer)
	{
		dprintf("DarlaGinaLayla: null pointer passed on midi_read\n");
		return B_IO_ERROR;
	}
	
	pClient = (PMIDI_CLIENT) cookie;
	pMI = pClient->pMI;	
	
	// Is there data alrady in the FIFO for this client?
	if (pClient->pBufferWrite != pClient->pBufferRead)
	{
		*num_bytes = copy_midi_bytes(pClient,(uint8 *) buffer, *num_bytes);
		DPF_OFF;
		return B_OK;
	}
	
	// Wait for the semaphore
	rval = acquire_sem_etc(	pClient->DataSem,
									1,
									B_CAN_INTERRUPT,
									B_INFINITE_TIMEOUT);

	// Copy the data
	*num_bytes = copy_midi_bytes(pClient,(uint8 *) buffer, *num_bytes);

	if (B_OK != rval)
	{
		DPF2(("DGL: Timed out waiting for semaphore\n"));	
	}

	DPF_OFF;
	return rval;

} // end of midi_read


/*------------------------------------------------------------------------
	midi_write - handle write() calls
------------------------------------------------------------------------*/

static status_t midi_write
(
	void* cookie, 
	off_t position, 
	const void* buffer, 
  	size_t* num_bytes
)
{
	size_t 			bytes_requested;
	PMONKEYINFO 	pMI;
	PMIDI_CLIENT	pClient;

	DPF_ON;
	DPF2(("DGL: midi_write\n"));

	if (NULL == buffer)
	{
		dprintf("DarlaGinaLayla: null pointer passed on midi_write\n");
		return B_IO_ERROR;
	}

	pClient = (PMIDI_CLIENT) cookie;
	pMI = pClient->pMI;	
	bytes_requested = *num_bytes;

	// Serialize the MIDI writes
	if (B_OK != acquire_sem(pMI->ControlSem))
	{
		DPF2(("DGL:\tFailed to acquire control semaphore!\n"));
		*num_bytes = 0;
		return B_ERROR;
	}

	*num_bytes = write_midi_bytes(pMI,buffer,bytes_requested);
	
	// Release the semaphore
	if (B_OK != release_sem(pMI->ControlSem))
	{
		dprintf("DGL:\tSomehow release_sem for control semaphore failed\n");
	}

	DPF_OFF;
	if (bytes_requested == *num_bytes)
		return B_OK;
	else
	{
		dprintf("DarlaGinaLayla: Wanted to write %d bytes, only wrote %d\n",
				bytes_requested,num_bytes);
		return B_IO_ERROR;
	}

} // end of midi_write


/*------------------------------------------------------------------------

	midi_control - handle ioctl calls

------------------------------------------------------------------------*/

static status_t midi_control
(
	void* cookie, 
	uint32 op, 
	void* arg, 
	size_t len
)
{
	DPF2(("DGL: ioctl %ld\n"));
	return B_OK;
} // end of midi_control


/*------------------------------------------------------------------------

	midi_close - handle close() calls

------------------------------------------------------------------------*/

static status_t midi_close (void* cookie)
{

	DPF_ON;
	DPF2(("DGL: midi_close %s\n",((PMIDI_CLIENT) cookie)->pMI->szCardBaseName));	

	DPF_OFF;
	return B_OK;

} // end of midi_close


/*------------------------------------------------------------------------
	midi_free - called after the last device is closed, and after
	all i/o is complete.
------------------------------------------------------------------------*/

static status_t midi_free (void* cookie)
{
	PMONKEYINFO 	pMI;
	PMIDI_CLIENT	pClient;

	DPF_ON;
	DPF1(("DGL: midi_free %s\n",((PMIDI_CLIENT) cookie)->pMI->szCardBaseName));		

	pClient = (PMIDI_CLIENT) cookie;
	pMI = pClient->pMI;	
	remove_midi_client(pMI,pClient);

	DPF_OFF;
	
	return B_OK;

} // end of midi_free


device_hooks echo_midi_device_hooks = {
	midi_open, 				/* -> open entry point */
	midi_close, 			/* -> close entry point */
	midi_free,				/* -> free cookie */
	midi_control, 			/* -> control entry point */
	midi_read,				/* -> read entry point */
	midi_write				/* -> write entry point */
};

