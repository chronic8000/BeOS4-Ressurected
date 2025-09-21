/*************************************************************************

 fixed.c
 
 Interrupt-time stuff

 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include "monkey.h"
#include "commpage.h"
#include "56301.h"
#include "util.h"
#include "fixed.h"

/*------------------------------------------------------------------------

	SendVectorIsr
	
------------------------------------------------------------------------*/

status_t SendVectorIsr( PMONKEYINFO pMI, DWORD dwCommand)
{
	bigtime_t	timeout;
	DWORD		*pMonkeyBase;
	DWORD		dwTemp;
	cpu_status	cs;

	pMonkeyBase = pMI->pMonkeyBase;

	cs = disable_interrupts();
	acquire_spinlock(&pMI->dwHardwareSpinlock);

	timeout = system_time() + VECTOR_TIMEOUT;	// system_time returns microseconds

	while (system_time() < timeout)
	{
		dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_VECTOR_REG]);
		if (0 == (dwTemp & VECTOR_READY)) // HI32 HCVR HC
		{
			pMonkeyBase[CHI32_VECTOR_REG] = B_HOST_TO_LENDIAN_INT32(dwCommand);
			__eieio();
			release_spinlock(&pMI->dwHardwareSpinlock);
			restore_interrupts(cs);
			return B_OK;
		}

		spin(1);	// Be friendly and let someone else use the bus
	}

	release_spinlock(&pMI->dwHardwareSpinlock);
	restore_interrupts(cs);

	return B_ERROR;
}	// SendVectorIsr


/*------------------------------------------------------------------------

	irq_handler
	
------------------------------------------------------------------------*/

#ifdef DEBUG
int irq_count = 0;
#endif

int32 irq_handler(void *cookie)
{
	PMONKEYINFO 	pMI;
	DWORD 			*pMonkeyBase;
	PCOMMPAGE 		pCommPage;
	int32			IrqSemCount;
	DWORD			dwTemp;

#ifdef DEBUG	
	irq_count++;
#endif

	// Do the timestamp stuff ASAP
	pMI = (PMONKEYINFO) cookie;
	pCommPage = pMI->pCommPage;
	pMI->qwSystemTimestamp = system_time();
	pMI->dwSampleTimestamp = pCommPage->dwPosition;
	
	// Does this interrupt belong to me?
	pMonkeyBase = pMI->pMonkeyBase;
	dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
	if (0 == (dwTemp & CHI32_HINT))
		return B_UNHANDLED_INTERRUPT;

	//
	// Handle the interrupt
	//
	SendVectorIsr(pMI,ACK_INT);

	dwTemp = *((DWORD *)pCommPage->wBuffersConsumed);
	pMI->dwActualBufferCount = B_LENDIAN_TO_HOST_INT32(dwTemp);
	
	SendVectorIsr(pMI,UNBLOCK_IRQ);

	get_sem_count(pMI->IrqSem,&IrqSemCount);
	if (IrqSemCount < 1)
	{
		// Someone is waiting for the semaphore
		release_sem_etc(pMI->IrqSem, 1, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
} // irq_handler


/*------------------------------------------------------------------------

	layla_irq_handler
	
------------------------------------------------------------------------*/

int32 layla_irq_handler(void *cookie)
{
	PMONKEYINFO 	pMI;
	DWORD 			*pMonkeyBase;
	PCOMMPAGE 		pCommPage;
	int32			IrqSemCount;
	DWORD			dwTemp;
	DWORD			dwIrqType;
	int32			rval;
	uint16			*pwMidi,wMidiCount;
	MIDI_CLIENT		*pClient;
	cpu_status		cs;
	sem_id			midi_client_sems[MAX_DEVICES];
	int				sem_idx;

#ifdef DEBUG	
	irq_count++;
#endif

	// Do the timestamp stuff ASAP
	pMI = (PMONKEYINFO) cookie;
	pCommPage = pMI->pCommPage;
	pMI->qwSystemTimestamp = system_time();
	pMI->dwSampleTimestamp = pCommPage->dwPosition;
	
	// Does this interrupt belong to me?
	pMonkeyBase = pMI->pMonkeyBase;
	dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
	if (0 == (dwTemp & CHI32_HINT))
		return B_UNHANDLED_INTERRUPT;

	//
	// Acknowledge the interrupt
	//
	SendVectorIsr(pMI,ACK_INT);
	
	dwIrqType = B_LENDIAN_TO_HOST_INT32(pCommPage->dwHi32Extended);
	rval = B_HANDLED_INTERRUPT;

	//
	// Handle audio interrupts
	//
	if (dwIrqType & HI32_EXT_AUDIOIRQ)
	{
		dwTemp = *((DWORD *)pCommPage->wBuffersConsumed);
		pMI->dwActualBufferCount = B_LENDIAN_TO_HOST_INT32(dwTemp);
		
		SendVectorIsr(pMI,UNBLOCK_IRQ);
	
		get_sem_count(pMI->IrqSem,&IrqSemCount);
		if (IrqSemCount < 1)
		{
			// Someone is waiting for the semaphore
			release_sem_etc(pMI->IrqSem, 1, B_DO_NOT_RESCHEDULE);
			rval = B_INVOKE_SCHEDULER;
		}
	}

	//
	// Handle MIDI input interrupts
	//
	if (dwIrqType & HI32_EXT_MIDIIRQ)
	{
		// Go through all the clients and write the MIDI data to
		// their buffer.  Need to protect this with a spinlock
		// in case a client is being added or removed in the
		// foreground
		
		// Get the spinlock
		cs = disable_interrupts();
		acquire_spinlock(&pMI->dwHardwareSpinlock);
		
		pClient = pMI->midi_clients;
		sem_idx = 0;
		while (NULL != pClient)
		{
			pwMidi = (uint16 *) pCommPage->dwPlayCoeff;
			wMidiCount = *pwMidi;
			pwMidi++;

			while (wMidiCount-- > 0)
			{
				*(pClient->pBufferWrite) = (uint8) *pwMidi;
				pwMidi++;
				pClient->pBufferWrite++;

				// Wrap around the circular buffer
				if (pClient->pBufferWrite == pClient->pBufferEnd)
					pClient->pBufferWrite = pClient->pBufferBase;
			}
			
			midi_client_sems[sem_idx++] = pClient->DataSem;
			
			pClient = pClient->pNext;
		}
		
		// Release the spinlock
		release_spinlock(&pMI->dwHardwareSpinlock);
		restore_interrupts(cs);

		// Release the semaphores for each client
		// if the client is waiting
		sem_idx--;
		while (sem_idx >= 0)
		{
			get_sem_count(midi_client_sems[sem_idx],&IrqSemCount);		
			if (IrqSemCount < 0)
			{
				release_sem_etc(midi_client_sems[sem_idx],
								1, 
								B_DO_NOT_RESCHEDULE);
				rval = B_INVOKE_SCHEDULER;
			}
			
			sem_idx--;
		}
		
		SendVectorIsr(pMI,UNBLOCK_MIDI);
	}
	
	return rval;
} // layla_irq_handler

