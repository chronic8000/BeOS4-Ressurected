//---------------------------------------------------------------------------
//   LLI.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1997  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Low-latency VxD interface
//---------------------------------------------------------------------------

#include "fixed.h"
#include "configmg.h"
#include "verinfo.h"
#include "api.h"
#include "lli_defs.h"
#include "lli.h"
#include "init.h"
#include "mtc.h"
#include "Loaders.h"
#include	"MixerMsgs.h"

#pragma VxD_PAGEABLE_CODE_SEG

#pragma VxD_PAGEABLE_DATA_SEG

#pragma VxD_PAGEABLE_CODE_SEG

//#pragma warning (push)							// save warning
#pragma warning (disable:4035)				// turn off no return code warning

CONFIGRET VXDINLINE
CONFIGMG_Yield( ULONG ulMicroseconds, ULONG ulFlags )
{
	Touch_Register(eax)
	Touch_Register(ecx)
	Touch_Register(edx)
	_asm push ulFlags
	_asm push ulMicroseconds
	VxDCall(_CONFIGMG_Yield);
	_asm add  esp, 2*4
}

//#pragma warning (pop)							// restore no return code warning

#include "LLI_IOC.c"

//------------------------------------------------------------------------------
//
// FindTheMonkey
//
//------------------------------------------------------------------------------
PMONKEYINFO FindTheMonkey( DWORD dwCardID )
{
	PMONKEYINFO		pMI;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext )
	{
		if ( pMI->DevNode == dwCardID )
			return( pMI );
	}
	DPF( ("FindTheMonkey: Failed card ID 0x%x\n",dwCardID) );
	return( NULL );
}		// PMONKEYINFO FindTheMonkey( DWORD dwCardID )


//
//	Just another way to use the previous function
//
DWORD FindTheMonkeyInIoParams( PDIOCPARAMETERS	pDP,
										 PMONKEYINFO *		ppMI )
{
	DWORD	dwCardID;

	if ( pDP->cbInBuffer < sizeof(DWORD) )
	{
		DPF( ("FindTheMonkeyInIoParams: Invalid buffer size, act %ul exp %ul\n",
				pDP->cbInBuffer,
				sizeof(DWORD)) );
		return( LOWLATRET_BADBUFFERSIZE );
	}

	dwCardID = *( (PDWORD) pDP->lpvInBuffer );
	if ( NULL == ( *ppMI = FindTheMonkey( dwCardID ) ) )
		return( LOWLATRET_BADCARDID );

	if ( (*ppMI)->wAudioAPIMode != AUDIO_APIMODE_LL0 )
	{
		DPF( ("FindTheMonkeyInIoParams: Card 0x%x not open!\n",(*ppMI)->DevNode) );
		return( LOWLATRET_NOTOPEN );
	}
	return( LOWLATRET_OK );
}		// DWORD FindTheMonkeyInIoParams( ... )


// Let mixer know we changed the gains while it wasn't looking
void __cdecl NotifyMixerAtAppyTime( DWORD dwRefData )
{
	DWORD	dwMsg[ 6 ];

	_asm pushad					; Save all 32 bit registers

	memset( dwMsg, 0, sizeof( dwMsg ) );
//	dwMsg[ 0 ] = BSM_INSTALLABLEDRIVERS;
	dwMsg[ 0 ] = BSM_ALLCOMPONENTS;
/*
#define BSF_QUERY               0x00000001
#define BSF_IGNORECURRENTTASK   0x00000002
#define BSF_FLUSHDISK           0x00000004
#define BSF_NOHANG              0x00000008
#define BSF_POSTMESSAGE         0x00000010
#define BSF_FORCEIFHUNG         0x00000020
#define BSF_NOTIMEOUTIFNOTHUNG  0x00000040
*/
	if ( (DWORD) -1 == _SHELL_BroadcastSystemMessage( BSF_POSTMESSAGE  |
																		BSF_FORCEIFHUNG |
																		BSF_NOTIMEOUTIFNOTHUNG |
																		BSF_IGNORECURRENTTASK,
//																	  dwMsg,
																	  NULL,
																	  MXDM_GAINCHANGE,
																	  dwRefData,
																	  0 ) )
	{
		DPF( ("NotifyMixerAtAppyTime: Unable to broadcast system message!\n") );
	}
	else
	{
		DPF( ("NotifyMixerAtAppyTime: Broadcast system message!\n") );
	}
	_asm	popad
}	// void __cdecl NotifyMixerAtAppyTime( DWORD dwRefData )


// Let mixer know we changed the gains while it wasn't looking
void NotifyMixer( DWORD dwRefData )
{
/*
	Under construction!
	if ( 0 == _SHELL_CallAtAppyTime( NotifyMixerAtAppyTime,
											   dwRefData,
											   0, 0 ) )
//											   CAAFL_RING0 ) )
	{
		DPF( ("NotifyMixer: Unable to schedule appy time event!\n") );
		return;
	}
	DPF( ("NotifyMixer: Schedule appy time event!\n") );
*/
}	// void NotifyMixer( DWORD dwRefData )


//------------------------------------------------------------------------------
// Device
//------------------------------------------------------------------------------
DWORD _stdcall VECHOGAL_DeviceIOControl( DWORD  			dwService,
                                         DWORD  			dwDDB,
                                         DWORD  			hDevice,
                                         PDIOCPARAMETERS	pDIOCParams )
{
	DWORD dwRetVal = 0;

	DPF( ("VECHOGAL_DeviceIOControl 0x%x\n", dwService) );

	// Handle the DIOC_GETVERSION message (Return 0)
	pDIOCParams->lpcbBytesReturned = 0;
	if ( dwService == 0 ) 
		return( 0L );

	// Is this a valid service call?
	if ( (sizeof(DIOCHandlerLookup)/sizeof(DIOCHANDLER)) > dwService )
	{
		// Call the service function
		dwRetVal = DIOCHandlerLookup[dwService](hDevice, pDIOCParams);
	}
	else
	{
		dwRetVal = LOWLATRET_NOTSUPPORTED;
	}
	DPF( ( "VECHOGAL_DeviceIOControl Ret 0x%x\n", dwRetVal) );
	return( dwRetVal );
}		// DWORD _stdcall VECHOGAL_DeviceIOControl( ... )


//------------------------------------------------------------------------------
// VECHOGAL_Get_Version
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetVersion( PLOWLATVERSION pVer )
{
	pVer->cVersion = APPVERSION;
	pVer->cRevision = APPREVISION;
	pVer->cRelease = APPRELEASE;
	pVer->cReserved = 0; 						// Please set to Zero
	DPF( ("VECHOGAL_GetVersion: Ver %u Rev %u Rel %u\n",
			pVer->cVersion,
			pVer->cRevision,
			pVer->cRelease) );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetVersion


//------------------------------------------------------------------------------
//
// VECHOGAL_GetNumCards
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetNumCards( PDWORD pdwNumCards )
{
	PMONKEYINFO pMI;

	// Count the number of cards in the barrel	
	*pdwNumCards = 0;
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
	   (*pdwNumCards)++;
	}
	DPF( ("VECHOGAL_GetNumCards: %u\n", *pdwNumCards) );
   return( LOWLATRET_OK );
}		// end of VECHOGAL_GetNumCards


//------------------------------------------------------------------------------
//
// GetClockCaps
//
//------------------------------------------------------------------------------
void GetClockCaps( PMONKEYINFO	pMI,
						 PDWORD			pdwInClockTypes,
						 PDWORD			pdwOutClockTypes )
{
	*pdwInClockTypes = LOWLATCAPS_HAS_INTERNAL_CLOCK;
	*pdwOutClockTypes = 0;
	switch ( pMI->wCardType )
	{
		case GRACE :
		case DARLA :
		case DARLA24 :
			break;

		case GINA24 :
		case GINA :	
			*pdwInClockTypes |= LOWLATCAPS_HAS_SPDIF_CLOCK;
			break;

		case LAYLA :
			*pdwInClockTypes |= LOWLATCAPS_HAS_SPDIF_CLOCK |
									  LOWLATCAPS_HAS_WORD_CLOCK  |
									  LOWLATCAPS_HAS_SUPER_CLOCK |
									  LOWLATCAPS_HAS_MTC_CLOCK;
			*pdwOutClockTypes |=
				LOWLATCAPS_HAS_WORD_CLOCK | LOWLATCAPS_HAS_SUPER_CLOCK;
			break;
	}
}	// GetClockCaps


//------------------------------------------------------------------------------
//
// GetCaps
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetCaps( PLOWLATCAPS	pllCaps,
								DWORD			dwBfrSz,
								PDWORD		pdwSize )
{
	PMONKEYINFO pMI;
	DWORD			dwReturn;
	int			i;

	dwReturn = LOWLATRET_OK;
	*pdwSize = 0;
	for (	pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )	
	{
		memset( pllCaps, 0, sizeof(LOWLATCAPS) );
		strcpy( pllCaps->szName, pMI->szCardInstallName );
		pllCaps->dwCardType = pMI->wCardType;
		pllCaps->dwCardID = pMI->DevNode;
		pllCaps->dwNumAudioOut = 2 * pMI->wNumWaveOut;
		pllCaps->dwNumAudioIn = 2 * pMI->wNumWaveIn;
		pllCaps->dwNumMidiOut = pMI->wNumMidiOut;
		pllCaps->dwNumMidiIn = pMI->wNumMidiIn;
		switch ( pMI->wCardType )
		{
			case DARLA24 :
			case GINA24 : // fixme
			case GRACE :
			case DARLA :
				for ( i = 0; i < (int) pllCaps->dwNumAudioOut; i++ )
					pllCaps->dwAudioOutCaps[ i ] = LOWLATCAPS_HASGAINCTRL;
				break;

			case GINA :
			case LAYLA :
				for ( i = 0; i < (int) pllCaps->dwNumAudioOut - 2; i++ )
					pllCaps->dwAudioOutCaps[ i ] = LOWLATCAPS_HASGAINCTRL;
				for ( ; i < (int) pllCaps->dwNumAudioOut; i++ )
					pllCaps->dwAudioOutCaps[ i ] = LOWLATCAPS_DIGITAL;
				for ( i = 0; i < (int) pllCaps->dwNumAudioIn - 2; i++ )
					pllCaps->dwAudioInCaps[ i ] = LOWLATCAPS_HASGAINCTRL;
				for ( ; i < (int) pllCaps->dwNumAudioIn; i++ )
					pllCaps->dwAudioInCaps[ i ] = LOWLATCAPS_DIGITAL;
				break;
		}
	
		GetClockCaps( pMI, &pllCaps->dwInClockTypes, &pllCaps->dwOutClockTypes );
		DPF( ("GetCaps: Card %d Type = 0x%x, ID 0x%x, # out %d, "
			   "# in %d # midi out %d #midi in %d\n",
				*pdwSize / sizeof(LOWLATCAPS),
				pllCaps->dwCardType,
				pllCaps->dwCardID,
				pllCaps->dwNumAudioOut,
				pllCaps->dwNumAudioIn,
				pllCaps->dwNumMidiOut,
				pllCaps->dwNumMidiIn) );

		pllCaps++;
		*pdwSize += sizeof(LOWLATCAPS);
		if ( dwBfrSz < *pdwSize )
		{
			DPF( ("GetCaps: Invalid out buffer size, act %ul exp %ul\n",
					dwBfrSz,
					*pdwSize) );
			dwReturn = LOWLATRET_BADBUFFERSIZE;
			break;
		}
	}
	return( dwReturn );
}		// VECHOGAL_GetCaps


//--------------------------------------------------------------------------
//	Kill some time
//	Allow other threads to run.  Use a loop instead of pausing for
//	desired time as Time_Slice_Sleep can wake up early.
//--------------------------------------------------------------------------
void	Sleep( DWORD dwMs )
{
	DWORD 		dwTimeout;

	dwTimeout = Get_System_Time() + dwMs;
	while ( dwTimeout >= Get_System_Time() ) 
		Time_Slice_Sleep( 1 );
}	// void Sleep( dwMs )


//------------------------------------------------------------------------------
//
// Stop all monkeys
//
//------------------------------------------------------------------------------
static void StopMonkeys()
{
	PMONKEYINFO		pMI;
	PCOMMPAGE		pCommPage;
	LOWLATFORMAT	Format;
	DWORD				dwDelay;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext)
	{
		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
		if ( pCommPage->dwStop != 0 || pCommPage->dwReset != 0 )
			break;
	}

	if (pMI == NULL)
		return;
	
	StopDSP( NULL );
	//
	//	Compute how long to wait for current buffer to complete.
	//	Add 10m.s. as safety margin.
	//	Assume double buffer (2 interrupts per buffer), to be absolutely
	//	correct, we need to save the pOpenAudio->dwAudioBufferNotifyStep
	//	so we can use it instead of dwAudioBufferSize.
	//
	dwDelay = 10;
	if ( pCommPage->dwSampleRate != 0 &&
		  pMI->LowLat.Audio.dwAudioBufferSize != 0 )
	{
		VECHOGAL_GetFormat( &Format );
		dwDelay += pMI->LowLat.Audio.dwAudioBufferSize / 2 *
															// bytes per interrupt
						8 / Format.dwBitsPerSample *
															// Convert to samples per interrupt
						1000 / Format.dwSampleRate;
															// Convert samples to ms.
	}
	Sleep( dwDelay );
	DPF( ("StopMonkeys: wait %u ms.\n", dwDelay) );
}		// StopMonkeys


//------------------------------------------------------------------------------
//
// Reset all Monkeys
//
//------------------------------------------------------------------------------
static void ResetMonkeys()
{
	PMONKEYINFO	pMI;
	PCOMMPAGE	pCommPage;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext)
	{
		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
		pCommPage->dwStop = pMI->LowLat.Audio.dwIoMask;
		pCommPage->dwStart = 0;
		pCommPage->dwReset = pMI->LowLat.Audio.dwIoMask;
	}
	StopMonkeys();
	DPF( ("ResetMonkey: 0x%x\n", pMI->DevNode) );
}		// ResetMonkeys


//------------------------------------------------------------------------------
//
// VECHOGAL_Reset
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_Reset( void )
{
	PMONKEYINFO		pMI;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext)
	{
		if ( pMI->wAudioAPIMode != AUDIO_APIMODE_LL0 )
		{
			DPF( ("VECHOGAL_Reset: Card 0x%x not open!\n",pMI->DevNode) );
			return( LOWLATRET_NOTOPEN );
		}
	}
	ResetMonkeys( pMI );
	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext)
	{
		//
		// Interrupts Off
		//
		_asm pushf		
		_asm cli
		//
		//	Alert ring 3 user that a reset has occurred so he can
		//	reset his buffer ptrs.
		//	It is here rather than in ResetMonkey because the only
		//	time we need to tell the user is after a start command.
		//
		pMI->LowLat.Audio.wResetFlag = TRUE;
		//
		//	Always treat the reset operation as a unique event.  It is
		//	possible for the reset flag to be set before a buffer
		//	complete interrupt occurs.  We need to be sure we wakeup
		//	the ring3 app which should be waiting in WaitForAudio.
		//
		if ( pMI->LowLat.Audio.AudioSem )
			SignalSemaphore( pMI->LowLat.Audio.AudioSem );
		//	
		// Interrupts back On
		//
		_asm popf
	}
	DPF( ("VECHOGAL_Reset: 0x%x OK\n",pMI->DevNode) );
	return( LOWLATRET_OK );
}		// VECHOGAL_Reset 


//============================================================================
// VECHOGAL_OpenAudio
//============================================================================
DWORD VECHOGAL_OpenAudio( PLOWLATOPENAUDIO		pOpenAudio,
								  PLOWLATAUDIOBUFFARRAY	pBufferArray,
								  PDWORD						pdwSize )
{
	PMONKEYINFO 	pMI;
	WORD				wCard;
	DWORD				dwPages;
	WORD				wNumCards = 0L;
	PCOMMPAGE		pCommPage;
	WORD				wOutput;
	WORD				wInput;
	DWORD				dwOffset;
	DWORD				dwLinear;		// Ring 0 linear addr
	PVOID				pClient;			// Ring 3 client addr
	DWORD				dwReturn;
	BOOL				bHasBuffers;
	LOWLATFORMAT	Format;

	*pdwSize = 0;
	// Check max and min buffer size
	if ( (pOpenAudio->dwAudioBufferSize < 64) ||
		  (pOpenAudio->dwAudioBufferSize > (512*1024)) )
	{
		DPF( ("VECHOGAL_OpenAudio: Invalid buffer size %ul, must be > 64 and "
				"< 512K\n",
				pOpenAudio->dwAudioBufferSize) );
		VECHOGAL_CloseAudio();
		return( LOWLATRET_BADBUFFERSIZE );
	}

	if ( pOpenAudio->dwAudioBufferNotifyStep == 0 )
	{
		DPF( ("VECHOGAL_OpenAudio: Invalid buffer notify step %ul\n",
				pOpenAudio->dwAudioBufferNotifyStep) );
		VECHOGAL_CloseAudio();
		return( LOWLATRET_INVALIDPARAM );
	}
	//
	//	Make sure caller didn't space out and forget to allocate buffers
	//
	if ( 0 == pOpenAudio->wDataSelector )
	{
		VECHOGAL_CloseAudio();
		DPF( ("VECHOGAL_OpenAudio: Caller neglected to allocate space for "
				"new audio data\n") );
		return( LOWLATRET_NOMEM );
	}
	//
	// Check that the buffer and each sub-buffer is cacheline aligned
	//
	if ( ( pOpenAudio->dwAudioBufferSize % ECHO_BUFFER_ALIGN ) != 0 )
	{
		DPF( ("VECHOGAL_OpenAudio: Invalid buffer size %ul, size must be "
				"multiple of %d\n",
				pOpenAudio->dwAudioBufferSize,
				ECHO_BUFFER_ALIGN) );
		VECHOGAL_CloseAudio();
		return( LOWLATRET_BADBUFFERSIZE );
	}
	if ( ( pOpenAudio->dwAudioBufferNotifyStep % ECHO_BUFFER_ALIGN ) != 0 )
	{
		DPF( ("VECHOGAL_OpenAudio: Invalid buffer notify step %ul, must be "
				"multiple of %d\n",
				pOpenAudio->dwAudioBufferNotifyStep,
				ECHO_BUFFER_ALIGN) );
		VECHOGAL_CloseAudio();
		return( LOWLATRET_INVALIDPARAM );
	}

	// Make sure that all cards are available
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		if ( pMI->wAudioAcquireCount != 0 )
		{
			DPF( ("VECHOGAL_OpenAudio: Not all cards are available!\n") );
			VECHOGAL_CloseAudio();
			return( LOWLATRET_BUSY );
		}
	}

   // mattg fixme only acquire the cards if everything is successful;
   // move this until later
	//	rnh I solved this problem by calling close on each failure.

	// Acquire all the cards (even if not used)
	bHasBuffers = FALSE;
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		DPF( ("VECHOGAL_OpenAudio: Acquired card %d name %s ID 0x%x in "
				"monkey barrel\n",
				wNumCards,
				pMI->szCardInstallName,
				pMI->DevNode) );
		//
		//	Make sure caller didn't space out and forget to allocate buffers
		//	for at least one of the cards
		//
		if ( NULL != pOpenAudio->LLCard[ wNumCards ].pBuffers )
		{
			bHasBuffers = TRUE;
		}
		wNumCards++;
		memset( &pMI->LowLat.Audio, 0, sizeof( LLAUDIOMI ) );
		pMI->wAudioAcquireCount++;
		pMI->wAudioAPIMode = AUDIO_APIMODE_LL0;
	}
	if ( !bHasBuffers )
	{
		VECHOGAL_CloseAudio();
		DPF( ("VECHOGAL_OpenAudio: Caller neglected to allocate space "
				"for new audio data\n") );
		return( LOWLATRET_NOMEM );
	}

	if ( pOpenAudio->wNumCards != wNumCards )
	{
		DPF( ("VECHOGAL_OpenAudio: Invalid # cards in LOWLATOPENAUDIO"
				"data structure %d expected %d\n",
				pOpenAudio->wNumCards,
				wNumCards) );
		VECHOGAL_CloseAudio();
		return( LOWLATRET_INVALIDPARAM );
	}
	//
	//	Must stop the Monkeys before doing anything!
	//
	ResetMonkeys();

	// Okay, let's go thru the cards
	for ( wCard = 0; wCard < wNumCards; wCard++ )
	{
		// Want more cards?
		if ( pOpenAudio->LLCard[ wCard ].dwCardID == NULL )
			break; 

		// Find the card
		for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
		{
			// Is this it?
			if ( pMI->DevNode == pOpenAudio->LLCard[ wCard ].dwCardID )
				break;	// Yeppers
		}

		// Any good?
		if ( pMI == NULL )
		{
			DPF( ("VECHOGAL_OpenAudio: Unable to find card %d ID 0x%x in "
					"monkey barrel\n",
					wCard,
					pOpenAudio->LLCard[ wCard ].dwCardID ) );
			VECHOGAL_CloseAudio();
			return( LOWLATRET_BADCARDID );	// No
		}
		//
		//	No allocated buffers for this card
		//
		if ( NULL == pOpenAudio->LLCard[ wCard ].pBuffers )
		{
			DPF( ("VECHOGAL_OpenAudio: Caller neglected to allocate space "
					"for new audio data\n") );
			continue;
		}
		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;

		pBufferArray[ wCard ].dwCardID = pMI->DevNode;
		pMI->LowLat.Audio.wResetFlag = FALSE;
		if ( !LoadFirmware( pMI ) )			// Load low latency firmware
		{
			VECHOGAL_CloseAudio();
			return( LOWLATRET_CANTOPEN );
		}

		for ( wOutput = 0; wOutput < (pMI->wNumWaveOut*2); wOutput++ )
		{
			pBufferArray[ wCard ].dwCardID = pMI->DevNode;
			if ( ( pOpenAudio->LLCard[ wCard ].dwAudioOutStates[ wOutput ]
													& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				pMI->LowLat.Audio.wAudioBuffersPerCard++;
				pMI->LowLat.Audio.dwIoMask |= ( 1L << wOutput );
			}
		}
		for ( wInput = 0; wInput < (pMI->wNumWaveIn*2); wInput++ )
		{
			pBufferArray[ wCard ].dwCardID = pMI->DevNode;
			if ( ( pOpenAudio->LLCard[ wCard ].dwAudioInStates[ wInput ]
													& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				pMI->LowLat.Audio.wAudioBuffersPerCard++;
				pMI->LowLat.Audio.dwIoMask |= ( 1L << (wInput + (pMI->wNumWaveOut*2)) );
			}
		}
		DPF( ("VECHOGAL_OpenAudio: dwIoMask (0x%x)\n", pMI->LowLat.Audio.dwIoMask) );
		//
		//	Convert client address to linear address
		//
		pMI->LowLat.Audio.dwDataHandle = 0;
		pMI->LowLat.Audio.dwPageCount = 0;
		pClient = pOpenAudio->LLCard[ wCard ].pBuffers;
		//
		//	Tried 2 methods to map user buffer into ring0.
		//	1)	Use Map_Flat -
		//			Returns a value that always maps to physical addr 0 in the
		//			page table.  As of now, don't understand why.
		//	2) Use SelectorMapFlat -
		//			This seems to work.  However, there are a few things to watch
		//			out for.  If the caller allocated the memory with GlobalAlloc,
		//			then the offset returned is only usable when in the calling
		//			apps address space.  When another process is running, the page
		//			table has been swapped so the pointer is no good.
		//			The caller must allocate the memory using a memoy mapped file,
		//			then the offset is usable regardless of the process currently
		//			running.  This is important because the interrupt handler
		//			stores a time stamp and a buffer position count.  It the
		//			pointer is not vaild, the system will page fault.
		//
		dwLinear = _SelectorMapFlat( Get_Cur_VM_Handle(),
											  pOpenAudio->wDataSelector,
											  0 );
		if ( dwLinear == 0xffffffff )
		{
			VECHOGAL_CloseAudio();
			DPF(("VECHOGAL_OpenAudio: Unable to map new audio data into "
				  "linear addr space\n"));
			return( LOWLATRET_NOMEM );
		}
		dwLinear += (DWORD) pClient;
		DPF( ("VECHOGAL_OpenAudio: Mapped new audio data (0x%x:%x) into "
				"linear addr space 0x%x\n",
			   pOpenAudio->wDataSelector,
				(DWORD) pClient,
				dwLinear) );
		//
		// Initialize Allocation stuff
		// Compute how many pages we need for all of the buffers.  Since we know
		//	that each buffer size is is a multiple of a quad PCI cache boundary
		//	(128 bytes) we can carve up the pages into individual buffers and
		//	know they will always be properly aligned.
		//
		pMI->LowLat.Audio.dwAudioBufferSize = pOpenAudio->dwAudioBufferSize;
		pMI->LowLat.Audio.dwAudioBufferBytesPerInterrupt =
			pOpenAudio->dwAudioBufferNotifyStep;
		//
		//	Compute how many pages we need for all of our buffers and the
		//	time stamp.  Round up to account for partial pages.
		//
		dwPages = ( pMI->LowLat.Audio.dwAudioBufferSize *
						pMI->LowLat.Audio.wAudioBuffersPerCard +
						(4096-1) ) /
						4096;
		++dwPages;						// Padding for internal use
		//
		//	Increment page count if buffer is not page aligned.
		//
		if ( 0 != ( dwLinear & (4096-1) ) )
		{
			DPF( ("VECHOGAL_OpenAudio: Audio data linear addr not page aligned, "
					"adding one more page\n") );
			++dwPages;
		}
		//
		//	Lock it so it cannot be swapped
		//
		if ( 0 == _LinPageLock( dwLinear >> 12, dwPages, 0 ) )
		{
			VECHOGAL_CloseAudio();
			DPF( ("VECHOGAL_OpenAudio: Unable to lock down %d pages of new "
					"audio data\n",
				   dwPages) );
			return( LOWLATRET_NOTSHAREABLE );
		}
		DPF( ("VECHOGAL_OpenAudio: Locked down page %d len %d of new "
				"audio data\n",
				dwLinear >> 12,
				dwPages) );
		//
		//	Save the pointer and page count
		//
		pMI->LowLat.Audio.dwDataHandle = dwLinear;
		pMI->LowLat.Audio.dwPageCount = dwPages;
		//
		//	Compute offset to get to nearest quad cache boundary
		//
		dwOffset = ECHO_BUFFER_ALIGN - ( dwLinear & (ECHO_BUFFER_ALIGN-1) );
		if ( dwOffset == ECHO_BUFFER_ALIGN )
			dwOffset = 0;

		// Make pointers for each output (sub) buffer by indexing into
		//	the large buffer provided by the caller.
		for ( wOutput = 0; wOutput < (pMI->wNumWaveOut*2); wOutput++ )
		{
			if ( ECHO_MAXAUDIOCHANNELS <= wOutput )
			{
				DPF( ("VECHOGAL_OpenAudio: Card %d output buffer %d has exceeded "
						"ECHO_MAXAUDIOCHANNELS (%d)\n",
					   wCard,
						wOutput,
						ECHO_MAXAUDIOCHANNELS) );
				break;
			}
			pBufferArray[ wCard ].AudioOutBuffers[ wOutput ].dwHandle = 0;
			if ( ( pOpenAudio->LLCard[ wCard ].dwAudioOutStates[ wOutput ]
													& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				//
				//	Compute individual buffer addresses to be returned to the client
				//
				pBufferArray[ wCard ].AudioOutBuffers[ wOutput ].dwHandle =
					(DWORD) pClient + dwOffset;
				DPF( ("VECHOGAL_OpenAudio: Card %d output buffer %d = 0x%x\n",
					   wCard,
						wOutput,
						(DWORD) pClient + dwOffset ) );
				//
				//	Build a rubber ducky for this audio output channel
				//
				dwReturn = LLPagitize( pMI,
											  pOpenAudio,
											  dwLinear + dwOffset,
											  wOutput );
				if ( dwReturn != 0L )
				{
					VECHOGAL_CloseAudio();
					return( dwReturn );
				}
				dwOffset += pOpenAudio->dwAudioBufferSize;
			}
		} // wOutput
		//
		// Make pointers for each input (sub) buffer by indexing into
		//	the large buffer provided by the caller.
		//
		for ( wInput = 0; wInput < (pMI->wNumWaveIn*2); wInput++ )
		{
			if ( ECHO_MAXAUDIOCHANNELS <= wInput )
			{
				DPF( ("VECHOGAL_OpenAudio: Card %d input buffer %d has exceeded "
						"ECHO_MAXAUDIOCHANNELS (%d)\n",
					   wCard,
						wInput,
						ECHO_MAXAUDIOCHANNELS) );
				break;
			}
			pBufferArray[ wCard ].AudioInBuffers[ wInput ].dwHandle = 0L;
			if ( ( pOpenAudio->LLCard[ wCard ].dwAudioInStates[ wInput ]
													& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				//
				//	Compute individual buffer addresses to be returned to the client
				//
				pBufferArray[ wCard ].AudioInBuffers[ wInput ].dwHandle =
					(DWORD) pClient + dwOffset;
				DPF( ("VECHOGAL_OpenAudio: Card %d input buffer %d = 0x%x\n",
					    wCard,
						 wInput,
						 (DWORD) pClient + dwOffset ) );
				//
				//	Build a rubber ducky for this audio input channel
				//
				dwReturn =
					LLPagitize( pMI,
									pOpenAudio,
									dwLinear + dwOffset,
									(WORD) (wInput + (pMI->wNumWaveOut*2)) );
				if ( dwReturn != 0L )
				{
					VECHOGAL_CloseAudio();
					return( dwReturn );
				}
				dwOffset += pOpenAudio->dwAudioBufferSize;
			}	// LLSTATE_ENABLE
		}		// wInput
		//
		//	Return selector for position info.  We map only the position info
		//	as read-only so the user doesn't party all over the comm page!
		//
		pBufferArray[ wCard ].pPosition.dwHandle =
			BuildAndGDTateSelector( (DWORD) &pCommPage->dwPosition,
											sizeof(DWORD)+1,
											1 );
		if ( pOpenAudio->wNoNotify )
		{
			// 
			// Set up the audio interrupt dispatch vector
			//
			pMI->AudioIRQVector = &AudioIRQHandlerNull;
			pMI->LowLat.Audio.AudioSem = NULL;
		}
		else
		{
			//
			// Create semaphores to wait on for audio buffer
			//	complete interrupts
			//
			if ( pMI->LowLat.Audio.wAudioBuffersPerCard > 0 )
				pMI->LowLat.Audio.AudioSem = Create_Semaphore( 0 );
	
			// 
			// Set up the audio interrupt dispatch vector
			//
			pMI->AudioIRQVector = &AudioIRQHandlerLowLatency;
		}
	}			// for ( wCard...
	//
	//	Make sure we start with a default format
	//
	Format.dwSampleRate = 44100;
	Format.dwBitsPerSample = 16;
	Format.byDataAreBigEndian = 0;	// 1 = Apple, 0 = Intel PC
	Format.byDataAreFloats = FALSE;
	VECHOGAL_SetFormat( &Format );
	*pdwSize = sizeof( LOWLATAUDIOBUFFARRAY) * wNumCards;
	DPF( ("VECHOGAL_OpenAudio: %d monkey(s) open!\n", wNumCards) );
	return( LOWLATRET_OK );
}		// VECHOGAL_OpenAudio


//============================================================================
//
//	LLPagitize
//
//============================================================================
DWORD LLPagitize( PMONKEYINFO			pMI,
						PLOWLATOPENAUDIO	pOpenAudio,
						DWORD					dwAudioBuffer,
						WORD					wDevice )
{
	HVM			SysVM;
	DWORD			dwLin, dwDuckPhysNew, dwPreSpace;
	DWORD 		dwPhys, dwByteCount, dwBytesPerPage, dwResult, dwPage;
	PDUCK			pDuck, pDuckNew;
	PCOMMPAGE	pCommPage;
	int			iPLE;
	DWORD			dwTotalBytes, dwBytesPerInterrupt;
	DWORD			dwBytesSinceInterrupt, dwPageEntry, dwDiff;

	dwBytesSinceInterrupt = 0L;
	dwBytesPerInterrupt = pOpenAudio->dwAudioBufferNotifyStep;
	dwTotalBytes = pOpenAudio->dwAudioBufferSize;
	//	
	// Make sure that the steps are on quad cachelines, we don't exceed one
	//	duck in size and that they are at least allowing double buffering.
	//
	if ( ( ( dwBytesPerInterrupt % ECHO_BUFFER_ALIGN ) != 0 ) || 
		  (   dwBytesPerInterrupt > (4096*255) ) ||
		  ( ( dwTotalBytes / dwBytesPerInterrupt ) < 2 ) )
	{
		DPF( ("LLPagitize: Invalid buffer notify step %ul\n",
				pOpenAudio->dwAudioBufferNotifyStep) );
		return( LOWLATRET_BADNOTIFYSIZE );
	}
	
	// Pagitize it
	SysVM = Get_Sys_VM_Handle();

	// Allocate space for a Rubber Ducky
	pDuckNew = pDuck =
		(PDUCK) _PageAllocate( 1,
									  PG_VM,
									  SysVM,
									  1,
									  0,
									  0x0FFF,
									  (void *) &dwDuckPhysNew,
									  PAGEUSEALIGN|PAGECONTIG|PAGEFIXED );
	pMI->LowLat.Audio.dwDucks[ wDevice ] = (DWORD) pDuckNew;
	if ( pDuckNew == 0 )
	{
		DPF( ("LLPagitize: Unable to allocate space for a new Duck\n") );
		return( LOWLATRET_NOMEM );
	}
   iPLE = 0;
	//
	//	Convert linear addr into a page and offset which can be used to get
	//	the physical addr from the page table.
	//
	dwLin = dwAudioBuffer;
	dwPage = dwLin >> 12;
	dwPreSpace = dwLin & 0x00000fff;
	dwByteCount = 0;							// Total bytes written so far
	
	do
	{
		//
		//	If duck is full, then we quit.  Probably should make a new duck here.
		//
		if ( iPLE == 510 )
		{
			DPF( ("LLPagitize: Rubber ducky overflow!\n") );
			break;
		}
		//
		//	Convert linear audio buffer address into a physical address
		//
		dwPageEntry = 0;
		dwResult = _CopyPageTable( dwPage, 1, &dwPageEntry, 0 );
		if ( dwResult == 0 )
		{
			DPF( ("LLPagitize: Unable to access page table for page %ul\n",
					dwPage) );
			return( MMSYSERR_NOMEM );
		}
		dwPhys = (dwPageEntry & 0xFFFFF000) + dwPreSpace;

		// See if it fits on the page
		if ( (dwTotalBytes - dwByteCount) > 4096 ) 
			dwBytesPerPage = 4096 - dwPreSpace;
		else
			dwBytesPerPage = dwTotalBytes - dwByteCount;
		if ( ( dwBytesPerPage + dwPreSpace ) > 4096 )
		{
			dwBytesPerPage = 4096 - dwPreSpace;
		}
		//
		// See if interrupt is to occur in the middle of this PLE
		//	and make the necessary adjustments.
		//
		dwBytesSinceInterrupt += dwBytesPerPage;
		if ( dwBytesSinceInterrupt > dwBytesPerInterrupt )
		{
			dwDiff = dwBytesSinceInterrupt - dwBytesPerInterrupt;
			dwBytesPerPage -= dwDiff;
			dwBytesSinceInterrupt -= dwDiff;
		}
		//
		//	Fill PLE in duck
		//
		pDuck[ iPLE ].dwPhysAddr = dwPhys;
		pDuck[ iPLE ].dwBytes = dwBytesPerPage;
		DPF( ("LLPagitize: PLE %d Phys 0x%x Bytes %d\n",
				iPLE, dwPhys, dwBytesPerPage) );
		//
		//	On to next PLE and page
		//
		iPLE++;
		dwByteCount += dwBytesPerPage;
		if ( ( dwPreSpace + dwBytesPerPage ) >= 4096 )
		{
			dwPage++;
			dwPreSpace = 0;					// Only first entry can be non-zero
		}
		else
		{
			//
			//	Handle case where bytes per interrupt is under one page
			//	or interrupt splits a page.
			//
			dwPreSpace += dwBytesPerPage;
		}

		// See if we need to add an interrupt
		if ( dwBytesSinceInterrupt >= dwBytesPerInterrupt )
		{
			// Flag the interrupt
			// Insert zeros so DSP knows to generate an interrupt
			pDuck[ iPLE ].dwPhysAddr = 0;
			pDuck[ iPLE ].dwBytes = 0;
			DPF( ("LLPagitize: PLE %d Phys 0x0 Bytes 0\n", iPLE) );
			iPLE++;
			//
			//	This should be zero!
			//
			dwBytesSinceInterrupt -= dwBytesPerInterrupt;
		}
	} while ( dwByteCount < dwTotalBytes );

	// Interrupts Off
	_asm pushf		
	_asm cli

	pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
	if ( NULL != pMI->pDuckList[ wDevice ] )
	{
		// Interrupts On
		_asm popf
		DPF( ("LLPagitize: pDuckList[ %d ] not empty!\n", wDevice) );
		DEBUGBREAK;
		return( LOWLATRET_BUSY );
	}
	pMI->pDuckList[ wDevice ] = pDuckNew;
	pCommPage->dwDuckListPhys[ wDevice ] = dwDuckPhysNew;

	// Add the double zero PLE but only if the last entry contained data
	if ( dwBytesSinceInterrupt != 0 )
	{
		pDuck[ iPLE ].dwPhysAddr = 0;
		pDuck[ iPLE ].dwBytes = 0L;
		iPLE++;
	}
	//
	// Wrap around to create the rubber ducky effect.
	//
	DPF( ("LLPagitize: PLE %d Phys 0x%x Bytes %d\n", iPLE,dwDuckPhysNew,0) );
	pDuck[ iPLE ].dwPhysAddr = dwDuckPhysNew;
	pDuck[ iPLE ].dwBytes = 0L;
	
	// Interrupts On
	_asm popf

	return( LOWLATRET_OK );
}		// LLPagitize


//------------------------------------------------------------------------------
//
// VECHOGAL_CloseAudio
//
//	Call this after CloseMidi or problems may result
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_CloseAudio()
{
	PMONKEYINFO		pMI;
	DWORD				dwReturn;
	int				iCt = 0;
	PCOMMPAGE		pCommPage;

	dwReturn = LOWLATRET_OK;
	
	//
	//	Must stop the Monkeys before doing anything!
	//
	ResetMonkeys();
	for (pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext)
	{
		if ( (pMI->wAudioAPIMode == AUDIO_APIMODE_LL3) ||
			  (pMI->wAudioAPIMode == AUDIO_APIMODE_LL0) )
		{
			//
			// This can happen when we close during an open failure.  So we
			//	don't really have an error here, just need to keep looking
			//	through the monkey barrel for any cards that had buffer
			//	pages allocated before the error occurred and clean them up.
			//
			if ( pMI->wAudioAcquireCount == 0 )
				continue;

			pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
			pMI->wAudioAcquireCount--;
			if ( pMI->wAudioAcquireCount == 0 )
			{
				WORD		wOutput;
				WORD		wInput;

				iCt++;
				pMI->wAudioAPIMode = AUDIO_APIMODE_NONE;
				// 
				// Reset the audio interrupt dispatch vector
				//	before doing unblock.  This insures a waiting
				//	interrupt will then be unblocked in the IRQ handler.
				//
				pMI->AudioIRQVector = &AudioIRQHandlerNull;
				UnblockAudioInterrupt( pMI, FALSE );

				if ( pMI->LowLat.Audio.AudioSem )
				{
					VMM_SEMAPHORE	AudioSem = pMI->LowLat.Audio.AudioSem;
					pMI->LowLat.Audio.AudioSem = NULL;
					SignalSemaphore( AudioSem );
					Sleep( 100 );		// Give waiting thread a change to exit
					Destroy_Semaphore( AudioSem );
				}
				//
				//	Shoot the ducks
				//
				for ( wOutput = 0; wOutput < (pMI->wNumWaveOut*2); wOutput++ )
				{
					if ( pMI->LowLat.Audio.dwDucks[ wOutput ] != 0 )
					{
						if ( 0 == _PageFree( (PVOID) pMI->LowLat.Audio.dwDucks[ wOutput ],
													0 ) )
						{
							DPF( ("CloseAudio: Unable to free duck %d addr 0x%x\n",
									wOutput,
									pMI->LowLat.Audio.dwDucks[ wOutput ] ) );
						}
						pMI->LowLat.Audio.dwDucks[ wOutput ] = 0;
						pMI->LowLat.Audio.wAudioBuffersPerCard--;
						pMI->pDuckList[ wOutput ] = NULL;
						pCommPage->dwDuckListPhys[ wOutput ]  = 0;
						pMI->pDuckListLast[ wOutput ]   = 0;
						pMI->dwDuckLastPLE[ wOutput ]   = 0;
						pMI->pDuckListGooser[ wOutput ] = 0;
						pMI->pDuckListFudd[ wOutput ]   = 0;
						}
				}
				for ( wInput = 0; wInput < (pMI->wNumWaveIn*2); wInput++ )
				{
					WORD	wDev = wInput + (pMI->wNumWaveOut*2);
					if ( pMI->LowLat.Audio.dwDucks[ wDev ] != 0 )
					{
						if ( 0 == _PageFree( (PVOID) pMI->LowLat.Audio.dwDucks[ wDev ],
													0 ) )
						{
							DPF( ("CloseAudio: Unable to free duck %d addr 0x%x\n",
									wDev,
									pMI->LowLat.Audio.dwDucks[ wDev ] ) );
						}
						pMI->LowLat.Audio.dwDucks[ wDev ] = 0;
						pMI->LowLat.Audio.wAudioBuffersPerCard--;
						pMI->pDuckList[ wDev ] = NULL;
						pCommPage->dwDuckListPhys[ wDev ]  = 0;
						pMI->pDuckListLast[ wDev ]   = 0;
						pMI->dwDuckLastPLE[ wDev ]   = 0;
						pMI->pDuckListGooser[ wDev ] = 0;
						pMI->pDuckListFudd[ wDev ]   = 0;
					}
				}
				//
				//	Unlock memory so it can be freed
				//
				if ( 0 != pMI->LowLat.Audio.dwDataHandle )
				{
					DWORD	dwPage = pMI->LowLat.Audio.dwDataHandle >> 12;
					pMI->LowLat.Audio.dwDataHandle = 0L;
					if ( 0 == _LinPageUnlock( dwPage,
													  pMI->LowLat.Audio.dwPageCount,
													  0 ) )
					{
						DPF(("CloseAudio: Unable to unlock %d pages of audio data\n",
							  pMI->LowLat.Audio.dwPageCount ));
						dwReturn = LOWLATRET_NOMEM;
					}
					DPF( ("CloseAudio: Unlocked page %d len %d of audio data\n",
							dwPage,
							pMI->LowLat.Audio.dwPageCount) );
				}
				//
				// Clear out the structure
				//
				memset( &pMI->LowLat.Audio, 0, sizeof( LLAUDIOMI ) );

				// Reset the audio state to unknown (just in case)
				pMI->bGDCurrentClockState = GD_CLOCK_UNDEF;
				pMI->bGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;
				pMI->bGDCurrentResamplerState = GD_RESAMPLE_UNDEF;
			}	// AcquireCount
		}
		else
		{
			dwReturn = LOWLATRET_NOTOPEN;
		}
	}
	DPF( ("CloseAudio: %u monkey(s) closed!\n", iCt) );
	return( dwReturn );
}		// VECHOGAL_CloseAudio


//------------------------------------------------------------------------------
//
// VECHOGAL_GetFormat
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetFormat( PLOWLATFORMAT	pFormat )
{
	PMONKEYINFO		pMI;
	// mattg fixme warning - change this so it comes from the first open card

	memset( pFormat, 0, sizeof(LOWLATFORMAT) );
	if ( ( pMI = pMonkeyBarrel ) != NULL )
	{
		PCOMMPAGE	pCommPage;

		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
		pFormat->byDataAreFloats  = FALSE;
		pFormat->byDataAreBigEndian = FALSE;
		switch ( pCommPage->dwAudioFormat[ 0 ] )
		{
			case AUDIOFORM_8 :
				pFormat->dwBitsPerSample = 8;
				break;
			case AUDIOFORM_16BE :
				pFormat->byDataAreBigEndian = TRUE;
			case AUDIOFORM_16LE :
				pFormat->dwBitsPerSample = 16;
				break;
			case AUDIOFORM_24BE :
				pFormat->byDataAreBigEndian = TRUE;
			case AUDIOFORM_24LE :
				pFormat->dwBitsPerSample = 24;
				break;
			case AUDIOFORM_32BE :
				pFormat->byDataAreBigEndian = TRUE;
			case AUDIOFORM_32LE :
				pFormat->dwBitsPerSample = 32;
				break;
			case AUDIOFORM_32FBE :
				pFormat->byDataAreBigEndian = TRUE;
			case AUDIOFORM_32FLE :
				pFormat->dwBitsPerSample = 32;
				break;
			default :
				DPF( ("VECHOGAL_GetFormat: No valid format specified\n") );
				return( LOWLATRET_BADFORMAT );
		}
		pFormat->dwSampleRate = pCommPage->dwSampleRate;
	}
	DPF( ("VECHOGAL_GetFormat: returned OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetFormat


//------------------------------------------------------------------------------
//
// VECHOGAL_SetFormat
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_SetFormat( PLOWLATFORMAT pFormat )
{
	PMONKEYINFO		pMI;

	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		PCOMMPAGE	pCommPage;
		BOOL			bBadFmt = FALSE;

		if ( pMI->wAudioAPIMode != AUDIO_APIMODE_LL0 )
		{
			DPF( ("VECHOGAL_SetFormat: Card 0x%x not open!\n",pMI->DevNode) );
			return( LOWLATRET_NOTOPEN );
		}

		switch (pMI->wCardType)
		{
			case GRACE :
				if (pFormat->dwSampleRate != 8000)
					bBadFmt = TRUE;
				break;

			case DARLA24 :
			case GINA24 :
				if (96000 == pFormat->dwSampleRate)
					break;
				if (88200 == pFormat->dwSampleRate)
					break;
				if (16000 == pFormat->dwSampleRate)
					break;
				if (8000 == pFormat->dwSampleRate)
					break;

				// Fall through into Darla/Gina

			case DARLA :
			case GINA :
				if ( pFormat->dwSampleRate != 11025 &&
					  pFormat->dwSampleRate != 22050 &&
					  pFormat->dwSampleRate != 32000 &&
					  pFormat->dwSampleRate != 44100 &&
					  pFormat->dwSampleRate != 48000 )
					bBadFmt = TRUE;
				break;

			case LAYLA:
				if ( pFormat->dwSampleRate <  8000 ||
					  pFormat->dwSampleRate > 50000 )
					bBadFmt = TRUE;
				break;

		}

/* 		if ( bBadFmt )                                             */
/* 		{                                                          */
/* 			DPF( ("VECHOGAL_SetFormat: Invalid sample rate, %ul\n", */
/* 					pFormat->dwSampleRate) );                         */
/* 			return( LOWLATRET_BADFORMAT );                          */
/* 		}                                                          */
	
		pMI->LowLat.Audio.dwAudioBufferSizeInSamples =
			pMI->LowLat.Audio.dwAudioBufferBytesPerInterrupt;
		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
		if ( pFormat->byDataAreFloats )
		{
			switch ( pFormat->dwBitsPerSample )
			{
				case 32 :
					pCommPage->dwAudioFormat[ 0 ] = AUDIOFORM_32FLE;
					pMI->LowLat.Audio.dwAudioBufferSizeInSamples /= 4;
					break;
				default :
					DPF( ("VECHOGAL_SetFormat: No valid format specified\n") );
					return( LOWLATRET_BADFORMAT );
			}
		}
		else
		{
			switch ( pFormat->dwBitsPerSample )
			{
				case 8 :
					pCommPage->dwAudioFormat[ 0 ] = AUDIOFORM_8;
					break;
				case 16 :
					pCommPage->dwAudioFormat[ 0 ] = AUDIOFORM_16LE;
					pMI->LowLat.Audio.dwAudioBufferSizeInSamples /= 2;
					break;
				case 20 :
				case 24 :
					pCommPage->dwAudioFormat[ 0 ] = AUDIOFORM_24LE;
					pMI->LowLat.Audio.dwAudioBufferSizeInSamples /= 3;
					break;
				case 32 :
					pCommPage->dwAudioFormat[ 0 ] = AUDIOFORM_32LE;
					pMI->LowLat.Audio.dwAudioBufferSizeInSamples /= 4;
					break;
				default :
					DPF( ("VECHOGAL_SetFormat: No valid format specified\n") );
					return( LOWLATRET_BADFORMAT );
			}
		}
		//
		//	Because we defined big endian formats to always follow little endian,
		//	we can get away with incrementing by one to convert one type
		//	to the other.
		//
		if ( pFormat->byDataAreBigEndian && pFormat->dwBitsPerSample != 8 )
			pCommPage->dwAudioFormat[ 0 ]++;
		pCommPage->dwSampleRate = pFormat->dwSampleRate;

		//
		//	Tell DSP the new sample rate (only for cards that can handle it)
		//
		if (0 == bBadFmt)
      {
			SetSampleRate( pMI );
      }
	}
	DPF( ("VECHOGAL_SetFormat: returned OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_SetFormat


//------------------------------------------------------------------------------
//
// VECHOGAL_Start
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_Start()
{
	PMONKEYINFO		pMI;
	PCOMMPAGE		pCommPage;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext )
	{
		if ( pMI->wAudioAPIMode != AUDIO_APIMODE_LL0 )
			return( LOWLATRET_NOTOPEN );

		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
		pCommPage->dwStop = 0;
		pCommPage->dwStart = pMI->LowLat.Audio.dwIoMask;
		pCommPage->dwReset = 0;
		pCommPage->dwPosition = 0;
	}
	StartDSP( NULL );					// Start all cards at once
	DPF( ("VECHOGAL_Start: returned OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_Start


//------------------------------------------------------------------------------
//
// VECHOGAL_Stop
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_Stop()
{
	PMONKEYINFO		pMI;
	PCOMMPAGE		pCommPage;

	for (	pMI = pMonkeyBarrel;	
			pMI != NULL;
			pMI = pMI->pNext )
	{
		if ( pMI->wAudioAPIMode != AUDIO_APIMODE_LL0 )
		{
			DPF( ("VECHOGAL_Stop: Card not open!\n") );
			return( LOWLATRET_NOTOPEN );
		}
		pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
		pCommPage->dwStop = pMI->LowLat.Audio.dwIoMask;
		pCommPage->dwStart = 0;
		pCommPage->dwReset = 0;
	}
	StopMonkeys();
	DPF( ("VECHOGAL_Stop: returned OK\n") );
	return( LOWLATRET_OK );
} 		// VECHOGAL_Stop


//------------------------------------------------------------------------------
//
// VECHOGAL_GetOutLevels
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetOutLevels( DWORD						dwCardID,
									  PLOWLATOUTGAINARRAY	pOutGains,
									  PDWORD						pdwSize )
{
	PMONKEYINFO	pMI;
	DWORD 		dwSize, dwOutputLevel, dwMask;
	PCOMMPAGE	pCommPage;
	int			i;
	
	pMI = FindTheMonkey( dwCardID );
	if ( pMI == NULL )
		return( LOWLATRET_BADCARDID );

	dwSize = sizeof( LOWLATOUTGAINARRAY ) +
				( ( 2 * pMI->wNumWaveOut - 1 ) * sizeof( int ) );
	if ( *pdwSize < dwSize )
	{
		DPF( ("VECHOGAL_GetOutLevels: Invalid buffer size, act %ul exp %ul\n",
				*pdwSize,
				dwSize) );
		return( LOWLATRET_BADBUFFERSIZE );
	}
	memset( pOutGains, 0, dwSize );
	pOutGains->dwCardID = pMI->DevNode;
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	dwOutputLevel = pCommPage->dwOutputLevel;
	dwMask = 1;
	for ( i = 0; i < pMI->wNumWaveOut; i++ )
	{
		int	iCh = i * 2;
		pOutGains->byTenFourLevel[ iCh ]   =
				( dwOutputLevel & dwMask ) ? (BYTE) TRUE : (BYTE) FALSE;
		dwMask <<= 1;
		pOutGains->byTenFourLevel[ iCh+1 ] =
				( dwOutputLevel & dwMask ) ? (BYTE) TRUE : (BYTE) FALSE;
		dwMask <<= 1;
		pOutGains->iGains[ iCh ]	=
			100 * (int) (char) pCommPage->LineLevel[ i ].bLineLeft;
		pOutGains->iGains[ iCh+1 ] =
			100 * (int) (char) pCommPage->LineLevel[ i ].bLineRight;
	}
	DPF( ("VECHOGAL_GetOutLevels: OK\n") );
	*pdwSize = dwSize;
	return( LOWLATRET_OK );
}		// VECHOGAL_GetOutLevels


//------------------------------------------------------------------------------
//
// VECHOGAL_SetOutLevels
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_SetOutLevels( PLOWLATOUTGAINARRAY pOutGains )
{
	PMONKEYINFO		pMI;
	DWORD 			dwOutputLevel, dwMask;
	PCOMMPAGE		pCommPage;
	int				i;
	
	pMI = FindTheMonkey( pOutGains->dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
	dwOutputLevel = 0;
	dwMask = 1;
	for ( i = 0; i < pMI->wNumWaveOut; i++ )
	{
		int	iCh = i * 2;
		if ( pOutGains->byTenFourLevel[ iCh ] )
			dwOutputLevel |= dwMask;
		dwMask <<= 1;
		if ( pOutGains->byTenFourLevel[ iCh+1 ] )
			dwOutputLevel |= dwMask;
		dwMask <<= 1;
		pCommPage->LineLevel[ i ].bLineLeft = pOutGains->iGains[ iCh ]    / 100;
		pCommPage->LineLevel[ i ].bLineRight = pOutGains->iGains[ iCh+1 ] / 100;
	}
	pCommPage->dwOutputLevel = dwOutputLevel;
	UpdateOutVolume( pMI );
	NotifyMixer( MXFL_OUTPUTLEVELS );	// Let mixer know we changed the gains
													// while it wasn't looking
	DPF( ("VECHOGAL_SetOutLevels: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_SetOutLevels


//------------------------------------------------------------------------------
//
// VECHOGAL_GetInGains
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetInGains( DWORD						dwCardID,
									PLOWLATINGAINARRAY	pInGains,
									PDWORD					pdwSize )
{
	PMONKEYINFO	pMI;
	PCOMMPAGE	pCommPage;
	int			i;
	DWORD			dwSize;
	
	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	dwSize = sizeof( LOWLATINGAINARRAY ) +
				( ( 2 * pMI->wNumWaveIn - 1 ) * sizeof( int ) );
	if ( *pdwSize < dwSize )
	{
		DPF( ("VECHOGAL_GetInGains: Invalid buffer size, act %ul exp %ul\n",
				*pdwSize,
				dwSize) );
		return( LOWLATRET_BADBUFFERSIZE );
	}
	memset( pInGains, 0, dwSize );
	pInGains->dwCardID = pMI->DevNode;
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	for ( i = 0; i < pMI->wNumWaveIn; i++ )
	{
		int	iCh = i * 2, j = i + pMI->wNumWaveOut;
		pInGains->iGains[ iCh ]	  =
			50 * (int) pCommPage->LineLevel[ j ].bLineLeft  - 10000;
		pInGains->iGains[ iCh+1 ] =
			50 * (int) pCommPage->LineLevel[ j ].bLineRight - 10000;
	}
	*pdwSize = dwSize;
	DPF( ("VECHOGAL_GetInGains: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetInGains


//------------------------------------------------------------------------------
//
// VECHOGAL_SetInGains
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_SetInGains( PLOWLATINGAINARRAY	pInGains )
{
	PMONKEYINFO				pMI;
	PCOMMPAGE				pCommPage;
	int						i;
	
	pMI = FindTheMonkey( pInGains->dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	for ( i = 0; i < pMI->wNumWaveOut; i++ )
	{
		int	iCh = i * 2, j = i + pMI->wNumWaveOut;
		pCommPage->LineLevel[ j ].bLineLeft  =
			( pInGains->iGains[ iCh ]   + 10000 ) / 50;
		pCommPage->LineLevel[ j ].bLineRight =
			( pInGains->iGains[ iCh+1 ] + 10000 ) / 50;
	}
	UpdateInGain( pMI );
	NotifyMixer( MXFL_INPUTGAINS );	// Let mixer know we changed the gains
												// while it wasn't looking
	DPF( ("VECHOGAL_SetInGains: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_SetInGains


//------------------------------------------------------------------------------
//
// VECHOGAL_GetMonitors
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetMonitors( DWORD				dwCardID,
									 PLOWLATMONITORS	pMonitors )
{
	PMONKEYINFO	pMI;
	PCOMMPAGE	pCommPage;
	int			i, j, nInputs, nOutputs, nMonIndex;
	
	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	memset( pMonitors, 0, sizeof( LOWLATMONITORS ) );
	//
	// Copy monitors	
	//
	nInputs  = (int) pMI->wNumWaveIn  * 2;
	nOutputs = (int) pMI->wNumWaveOut * 2;
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	pMonitors->dwCardID = dwCardID;

	for ( i = 0; i < nOutputs; i++ )
	{
		nMonIndex = i * nInputs;
		for ( j = 0; j < nInputs; j++ )
		{
			pMonitors->iMonitors[ j ][ i ] =
				100 * (int) (char) pCommPage->Monitors[ nMonIndex++ ];
		}
	}
	DPF( ("VECHOGAL_GetMonitors: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetMonitors


//------------------------------------------------------------------------------
//
// VECHOGAL_SetMonitors
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_SetMonitors( PLOWLATMONITORS	pMonitors )
{
	PMONKEYINFO			pMI;
	PCOMMPAGE			pCommPage;
	int					i, j, nInputs, nOutputs, nMonIndex;
	
	pMI = FindTheMonkey( pMonitors->dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );
	//
	// Copy monitors	
	//
	nInputs  = (int) pMI->wNumWaveIn  * 2;
	nOutputs = (int) pMI->wNumWaveOut * 2;
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
		
	for ( i = 0; i < nOutputs; i++ )
	{
		nMonIndex = i * nInputs;
		for ( j = 0; j < nInputs; j++ )
		{
			pCommPage->MonitorCopy[ nMonIndex++ ] =
				pMonitors->iMonitors[ j ][ i ] / 100;
		}
	}
	UpdateOutVolume( pMI );
	NotifyMixer( MXFL_MONITORS );	// Let mixer know we changed the gains
											// while it wasn't looking
	DPF( ("VECHOGAL_SetMonitors: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_SetMonitors


//------------------------------------------------------------------------------
//
// VECHOGAL_GetClocks
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetClocks( PLOWLATCLOCKSELECT	pClockSelect )
{
	PMONKEYINFO	pMI;
	DWORD 		dwCardID;
	PCOMMPAGE	pCommPage;
	
	dwCardID = pClockSelect->dwCardID;

	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	memset( pClockSelect, 0, sizeof( LOWLATCLOCKSELECT ) );
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	pClockSelect->dwCardID = dwCardID;

	GetClockCaps( pMI,
					  &pClockSelect->dwInClock,
					  &pClockSelect->dwOutClock );

	pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_INTERNAL;
	if ( pMI->MTC.bFlags & MTC_FLAG_SYNC_ON )
		pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_MTC;
   
	switch ( pCommPage->wInputClock )
	{
		case CLK_CLOCKININTERNAL : 
			break;

		case CLK_CLOCKINSPDIF :
			pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_SPDIF;
			break;

		case CLK_CLOCKINSUPER : 
			pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_SUPER;
			//
			//	OK to drop through here as super clock automatically
			//	means word clock as well.
			//
		case CLK_CLOCKINWORD : 
			pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_WORD;
			break;						

		case CLK_CLOCKINMIDI :
			pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_MTC;
			break;
			
		case CLK_CLOCKINDARLA24 :
			pClockSelect->dwInClock  |= LOWLATCLOCKAVAIL_ECHO;
			break;
	}
	if ( LAYLA == pMI->wCardType )
		pClockSelect->dwOutClock |= ( CLK_CLOCKOUTSUPER	== pCommPage->wOutputClock )
													? LOWLATCLOCKAVAIL_SUPER
													: LOWLATCLOCKAVAIL_WORD;
	DPF( ("VECHOGAL_GetClocks: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetClocks


//------------------------------------------------------------------------------
//
// VECHOGAL_SetClocks
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_SetClocks( PLOWLATCLOCKSELECT	pClockSelect )
{
	PMONKEYINFO				pMI;
	DWORD 					dwSize, dwCardID;
	PCOMMPAGE				pCommPage;
	
	dwSize = sizeof( LOWLATCLOCKSELECT );
	dwCardID = pClockSelect->dwCardID;

	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );
	if ( pMI->wAudioAPIMode != AUDIO_APIMODE_LL0 )
	{
		DPF( ("VECHOGAL_SetClocks: Card 0x%x not open!\n",pMI->DevNode) );
		return( LOWLATRET_NOTOPEN );
	}
	
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	//
	//	First sort out the input clocks.  Put the
	//	desired change in the comm page
	//
	if ( pClockSelect->dwInClock & LOWLATCLOCKAVAIL_SPDIF )
		pCommPage->wInputClock = CLK_CLOCKINSPDIF;
	else if ( pClockSelect->dwInClock & LOWLATCLOCKAVAIL_SUPER )
		pCommPage->wInputClock = CLK_CLOCKINSUPER;
	else if ( pClockSelect->dwInClock & LOWLATCLOCKAVAIL_WORD )
		pCommPage->wInputClock = CLK_CLOCKINWORD;
	else if ( pClockSelect->dwInClock & LOWLATCLOCKAVAIL_MTC )
		pCommPage->wInputClock = CLK_CLOCKINMIDI;
	else if ( pClockSelect->dwInClock & LOWLATCLOCKAVAIL_ECHO )
		pCommPage->wInputClock = CLK_CLOCKINDARLA24;
	else
		pCommPage->wInputClock = CLK_CLOCKININTERNAL;

	//
	//	Now do the output clocks
	//
	pCommPage->wOutputClock = 0;
	if ( LAYLA == pMI->wCardType )
	{
		if ( pClockSelect->dwOutClock & LOWLATCLOCKAVAIL_SUPER )
			pCommPage->wOutputClock = CLK_CLOCKOUTSUPER;
		else
			if ( pClockSelect->dwOutClock & LOWLATCLOCKAVAIL_WORD )
				pCommPage->wOutputClock = CLK_CLOCKOUTWORD;
	}
	//
	//	Do sanity check, set all necessary flags and update the DSP
	//
	UpdateClocks( pMI );
	NotifyMixer( MXFL_CLOCKS );	// Let mixer know we changed the gains
											// while it wasn't looking
	DPF( ("VECHOGAL_SetClocks: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_SetClocks


//------------------------------------------------------------------------------
//
// VECHOGAL_WaitForAudio
//
//	Called by app to wait for the audio buffer complete event
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_WaitForAudio( DWORD				dwCardId,
									  PLOWLATBFRSTAMP	pBfrStamp )
{
	PMONKEYINFO	pMI;

	memset( pBfrStamp, 0, sizeof(LOWLATBFRSTAMP) );
	
//	DPF( ("VECHOGAL_WaitForAudio: Wait on card id 0x%x\n", dwCardId ) );
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		if ( pMI->DevNode == dwCardId )
		{
			//
			//	Wait for semaphore if it exists.  It gets set when an Audio
			//	bufer complete interrupt occurs or the user issues a
			//	Reset I/O control call.
			//
			if ( NULL != pMI->LowLat.Audio.AudioSem )
				Wait_Semaphore( pMI->LowLat.Audio.AudioSem, BLOCK_SVC_INTS );
			//
			// Interrupts Off
			//
			_asm pushf		
			_asm cli
			if ( NULL == pMI->LowLat.Audio.AudioSem )
			{
				DPF( ("VECHOGAL_WaitForAudio: low latency mode closed while "
						"blocked for audio!\n") );
				//
				//	Make sure we don't leave the DSP hung waiting for an unblock
				//	or it will never recover w/o reboooting the PC.
				//
				if ( pMI->LowLat.Audio.BfrStamp.dwTime )
				{
					memset( &pMI->LowLat.Audio.BfrStamp, 0, sizeof( LOWLATBFRSTAMP ) );
					UnblockAudioInterrupt( pMI, TRUE );
				}
				//	
				// Interrupts back On
				//
				_asm popf
				return( LOWLATRET_NOTOPEN );
			}
			if ( pMI->LowLat.Audio.wResetFlag )
			{
				pMI->LowLat.Audio.BfrStamp.wResetFlag = TRUE;
				pMI->LowLat.Audio.wResetFlag = FALSE;
			}
			
			*pBfrStamp = pMI->LowLat.Audio.BfrStamp;
			//
			//	Make sure we don't leave the DSP hung waiting for an unblock
			//	or it will never recover w/o reboooting the PC.  Only do
			//	unblock command if Audio interrupt has occurred.  If this
			//	is only a Reset operation, then skip it.
			//
//			if ( pMI->LowLat.Audio.BfrStamp.dwTime )
			//
			//	WARNING: Ugly hack #5299!
			//	Spin until all unblocks done or we hang when the PC
			//	gets too far behind!
			//

#ifdef DEBUG
			if (pMI->dwAudioUnblockLevel > 1)
				DEBUGBREAK;
#endif

			while ( pMI->dwAudioUnblockLevel != 0 )
				UnblockAudioInterrupt( pMI, TRUE );
			memset( &pMI->LowLat.Audio.BfrStamp, 0, sizeof( LOWLATBFRSTAMP ) );
			//	
			// Interrupts back On
			//
			_asm popf
			DPF( ("VECHOGAL_WaitForAudio: %s returned OK\n",
					pMI->szCardInstallName) );
			return( LOWLATRET_OK );
		}
	}
	DPF( ("VECHOGAL_WaitForAudio: Unable to find card\n") );
	return( LOWLATRET_NOTOPEN );
}		// VECHOGAL_WaitForAudio


//------------------------------------------------------------------------------
//
// VECHOGAL_GetMeters
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_GetMeters( DWORD 				dwCardID,
								  PLOWLATMETERS	pMeters )
{
	PMONKEYINFO		pMI;
	WORD				wNum;
	PCOMMPAGE		pCommPage;
	
	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );

	memset( pMeters, 0, sizeof( LOWLATMETERS ) );
	pCommPage =(PCOMMPAGE) pMI->dwCommPageHandle;
	pMeters->dwCardID = dwCardID;

	for ( wNum = 0; wNum < pMI->wNumWaveOut; wNum++ )
	{
		pMeters->iOutVUMeters[ wNum*2     ] =
			(int) (char) pCommPage->VULevel[ wNum ].bMeterLeft * 100;
		pMeters->iOutVUMeters[ wNum*2 + 1 ] =
			(int) (char) pCommPage->VULevel[ wNum ].bMeterRight * 100;
		pMeters->iOutPeakMeters[ wNum*2     ] =
			(int) (char) pCommPage->PeakMeter[ wNum ].bMeterLeft * 100;
		pMeters->iOutPeakMeters[ wNum*2 + 1 ] =
			(int) (char) pCommPage->PeakMeter[ wNum ].bMeterRight * 100;
	}

	for ( wNum = 0; wNum < pMI->wNumWaveIn; wNum++ )
	{
		pMeters->iInVUMeters[ wNum*2     ] = 100 *
			(int) (char) pCommPage->VULevel[ pMI->wNumWaveOut + wNum ].bMeterLeft;
		pMeters->iInVUMeters[ wNum*2 + 1 ] = 100 *
			(int) (char) pCommPage->VULevel[ pMI->wNumWaveOut + wNum ].bMeterRight;
		pMeters->iInPeakMeters[ wNum*2     ] = 100 *
			(int) (char) pCommPage->PeakMeter[ pMI->wNumWaveOut + wNum ].bMeterLeft;
		pMeters->iInPeakMeters[ wNum*2 + 1 ] = 100 *
			(int) (char) pCommPage->PeakMeter[ pMI->wNumWaveOut + wNum ].bMeterRight;
	}
	DPF( ("VECHOGAL_GetMeters: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_GetMeters


//============================================================================
//
// VECHOGAL_OpenMidi
//`
//============================================================================
DWORD VECHOGAL_OpenMidi( PLOWLATOPENMIDI	pOpenMidi )
{
	PMONKEYINFO 		pMI;
	WORD					wCard;
	DWORD					dwPages;
	WORD					wNumCards = 0L;
	PCOMMPAGE			pCommPage;
	WORD					wInput;
	DWORD					dwOffset = 0;
	DWORD					dwLinear;		// Ring 0 linear addr
	PVOID					pClient;			// Ring 3 client addr

	//
	//	Make sure caller didn't space out and forget to allocate buffers
	//
	if ( 0 == pOpenMidi->wDataSelector )
	{
		VECHOGAL_CloseMidi();
		DPF( ("VECHOGAL_OpenMidi: Caller neglected to allocate space for "
				"new MIDI data\n") );
		return( LOWLATRET_NOMEM );
	}

	// Make sure that all cards are available
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		if ( pMI->LowLat.Midi.MidiSem != NULL )
		{
			DPF( ("VECHOGAL_OpenMidi: Not all cards are available!\n") );
			VECHOGAL_CloseMidi();
			return( LOWLATRET_BUSY );
		}
	}
   
	// Acquire all the cards (even if not used)
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		DPF( ("VECHOGAL_OpenMidi: Acquired card %d ID 0x%x in monkey barrel\n",
				wNumCards,
				pMI->DevNode) );
		wNumCards++;
		memset( &pMI->LowLat.Midi, 0, sizeof( LLMIDIMI ) );
	}

	// Okay, let's go thru the cards
	for ( wCard = 0; wCard < wNumCards; wCard++ )
	{
		// Want more cards?
		if ( pOpenMidi->LLMidiCard[ wCard ].dwCardID == NULL )
			continue; 

		// Find the card
		for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
		{
			// Is this it?
			if ( pMI->DevNode == pOpenMidi->LLMidiCard[ wCard ].dwCardID )
				break;	// Yeppers
		}

		// Any good?
		if ( pMI == NULL )
		{
			DPF( ("VECHOGAL_OpenMidi: Unable to find card %d ID 0x%x in "
					"monkey barrel\n",
					wCard,
					pOpenMidi->LLMidiCard[ wCard ].dwCardID ) );
			VECHOGAL_CloseMidi();
			return( LOWLATRET_BADCARDID );	// No
		}
		//
		//	Make sure caller didn't space out and forget to allocate buffers
		//
		if ( NULL == pOpenMidi->LLMidiCard[ wCard ].pBuffers )
		{
			VECHOGAL_CloseMidi();
			DPF( ("VECHOGAL_OpenMidi: Caller neglected to allocate space for "
				   "new audio data\n") );
			return( LOWLATRET_NOMEM );
		}
		pClient = pOpenMidi->LLMidiCard[ wCard ].pBuffers;
		pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;

		for ( wInput = 0; wInput < pMI->wNumMidiIn; wInput++ )
		{
			if ( (pOpenMidi->LLMidiCard[ wCard ].dwMidiInStates[ wInput ]
													& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				pMI->LowLat.Midi.wMidiBuffersPerCard++;
			}
		}
		//
		//	Convert client address to linear address
		//
		pMI->LowLat.Midi.dwMidiDataHandle = 0;
		pMI->LowLat.Midi.dwMidiPageCount = 0;
		dwLinear = _SelectorMapFlat( Get_Cur_VM_Handle(),
											  pOpenMidi->wDataSelector,
											  0 );
		if ( dwLinear == 0xffffffff )
		{
			VECHOGAL_CloseMidi();
			DPF( ("VECHOGAL_OpenMidi: Unable to map new audio data into linear "
					"addr space\n") );
			return( LOWLATRET_NOMEM );
		}
		dwLinear += (DWORD) pClient;
		DPF( ("VECHOGAL_OpenMidi: Mapped new audio data (0x%x:0) into linear "
				"addr space 0x%x\n",
			   pOpenMidi->wDataSelector,
				dwLinear) );
		//
		// Initialize Allocation stuff
		// Compute how many pages we need for all of the buffers.  Since we know
		//	that each buffer size is is a multiple of a quad PCI cache boundary
		//	(128 bytes) we can carve up the pages into individual buffers and
		//	know they will always be properly aligned.
		//
		pMI->LowLat.Midi.dwMidiBufferSize = pOpenMidi->dwMidiBufferSize;
		//
		//	Compute how many pages we need for all of our buffers and the
		//	time stamp.  Round up to account for partial pages.
		//
		dwPages = ( pMI->LowLat.Midi.dwMidiBufferSize *
						pMI->LowLat.Midi.wMidiBuffersPerCard +
						(4096-1) ) /
						4096;
		//
		//	Lock it so it cannot be swapped
		//
		if ( 0 == _LinPageLock( dwLinear >> 12, dwPages, 0 ) )
		{
			VECHOGAL_CloseMidi();
			DPF( ("VECHOGAL_OpenMidi: Unable to lock down %d pages of new "
					"audio data\n",
				   dwPages) );
			return( LOWLATRET_NOTSHAREABLE );
		}
		DPF( ("VECHOGAL_OpenMidi: Locked down page %d len %d of new audio data\n",
				 dwLinear >> 12,
				 dwPages) );
		//
		//	Save the pointer and page count
		//
		pMI->LowLat.Midi.dwMidiDataHandle = dwLinear;
		pMI->LowLat.Midi.dwMidiPageCount = dwPages;
		//
		// Make pointers for each midi input buffer by indexing into
		//	the large buffer provided by the caller.
		//
		for ( wInput = 0; wInput < pMI->wNumMidiIn; wInput++ )
		{
			if ( ECHO_MAXMIDIJACKS <= wInput )
			{
				DPF( ("VECHOGAL_OpenMidi: Card %d MIDI input buffer %d has "
						"exceeded ECHO_MAXMIDIJACKS (%d)\n",
					   wCard,
						wInput,
						ECHO_MAXMIDIJACKS) );
				break;
			}
			pMI->LowLat.Midi.dwMidiInFlags[ wInput ] =
				(WORD) pOpenMidi->LLMidiCard[ wCard ].dwMidiInStates[ wInput ];
			if ( (pMI->LowLat.Midi.dwMidiInFlags[ wInput ] 
							& LLSTATE_ENABLE ) == LLSTATE_ENABLE )
			{
				DPF( ("VECHOGAL_OpenMidi: Card %d MIDI input buffer %d = 0x%x\n",
					    wCard,
						 wInput,
						 dwLinear + dwOffset) );
				//
				//	Save ring0 addr of MIDI input buffer
				//
				pMI->LowLat.Midi.lpbyMidiData[ wInput ] =
					(LPBYTE) ( dwLinear + dwOffset );
				pMI->LowLat.Midi.dwMidiInWriteOffset[ wInput ] = 0;
				pMI->LowLat.Midi.dwMidiInReadOffset[ wInput ] = 0;
				dwOffset += pOpenMidi->dwMidiBufferSize;
			}	// LLSTATE_ENABLE
		}		// wInput
		//
		// Create semaphore to wait on for MIDI buffer
		//	complete interrupts
		//
		if ( pMI->LowLat.Midi.wMidiBuffersPerCard > 0 )
		{
			SendVector( pMI, MIDI_IN_START );		// Reset MTC on DSP
			pMI->LowLat.Midi.MidiSem = Create_Semaphore( 0 );
		}

		//
		// Set up the interrupt dispatch vector
		//
		SetMidiDispatchVector( pMI, &MidiIRQHandlerLowLatency );

		//
		// Enable MIDI input
		//
		EnableMidiInput(pMI);

	}			// for ( wCard...
	DPF( ("VECHOGAL_OpenMidi: %d monkey(s) open!\n", wNumCards) );
	return( LOWLATRET_OK );
}		// VECHOGAL_OpenMidi


//------------------------------------------------------------------------------
//
// VECHOGAL_CloseMidi
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_CloseMidi()
{
	PMONKEYINFO		pMI;
	DWORD				dwReturn;
	int				iCt = 0;

	dwReturn = LOWLATRET_OK;
	
	for ( pMI = pMonkeyBarrel; pMI != NULL; pMI = pMI->pNext )
	{
		VMM_SEMAPHORE	MidiSem;
		WORD				wInput;
		//
		// This can happen when we close during an open failure.  So we
		//	don't really have an error here, just need to keep looking
		//	through the monkey barrel for any cards that had buffer
		//	pages allocated before the error occurred and clean them up.
		//
		if ( NULL == pMI->LowLat.Midi.MidiSem )
			continue;

		MidiSem = pMI->LowLat.Midi.MidiSem;
		pMI->LowLat.Midi.MidiSem = NULL;
		SignalSemaphore( MidiSem );
		Sleep( 100 );		// Give waiting thread a chance to exit
		Destroy_Semaphore( MidiSem );
		//
		//	Disable MIDI input
		//
		for ( wInput = 0; wInput < pMI->wNumMidiIn; wInput++ )
			pMI->LowLat.Midi.dwMidiInFlags[ wInput ] = 0;
		pMI->LowLat.Midi.wMidiBuffersPerCard = 0;
		//
		//	Unlock memory so it can be freed
		//
		if ( 0 != pMI->LowLat.Midi.dwMidiDataHandle )
		{
			DWORD	dwPage = pMI->LowLat.Midi.dwMidiDataHandle >> 12;
			pMI->LowLat.Midi.dwMidiDataHandle = 0L;
			if ( 0 == _LinPageUnlock( dwPage,
											  pMI->LowLat.Midi.dwMidiPageCount,
											  0 ) )
			{
				DPF( ("CloseMidi: Unable to unlock %d pages of audio data\n",
					    pMI->LowLat.Midi.dwMidiPageCount ) );
				dwReturn = LOWLATRET_NOMEM;
			}
			DPF( ("CloseMidi: Unlocked page %d len %d of audio data\n",
					dwPage,
					pMI->LowLat.Midi.dwMidiPageCount) );
		}
		memset( &pMI->LowLat.Midi, 0, sizeof( LLMIDIMI ) );

		//
		// Reset the MIDI interrupt dispatch vector
		//
	
		ClearMidiDispatchVector( pMI, &MidiIRQHandlerLowLatency) ;

		//
		// Disable MIDI input
		//
		DisableMidiInput(pMI);

	}
	DPF( ("CloseMidi: %u monkey(s) closed!\n", iCt) );
	return( dwReturn );
}		// CloseMidi


//------------------------------------------------------------------------------
//
// VECHOGAL_ReadMidi
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_ReadMidi( DWORD dwCardID, PLOWLATREADMIDI	pReadMidi )
{
	PMONKEYINFO			pMI;
	DWORD					dwMidiDiff;
	BOOL					bDone = FALSE;
	
	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );
	memset( pReadMidi, 0, sizeof( LOWLATREADMIDI ) );
	pReadMidi->dwCardID = pMI->DevNode;
	//
	//	Only wait if MIDI enabled.  This version
	//	only supports 1 midi jack.
	//
	if ( NULL == pMI->LowLat.Midi.MidiSem ||
		  NULL == pMI->LowLat.Midi.lpbyMidiData[ 0 ] )
		return( LOWLATRET_NOMIDI );

	while ( !bDone )
	{
		if ( NULL != pMI->LowLat.Midi.MidiSem )
			Wait_Semaphore( pMI->LowLat.Midi.MidiSem, BLOCK_SVC_INTS );
		//
		// Interrupts Off
		//
		_asm pushf		
		_asm cli
		dwMidiDiff = pMI->LowLat.Midi.dwMidiInWriteOffset[ 0 ] -
						 pMI->LowLat.Midi.dwMidiInReadOffset[ 0 ];
		if ( 0 != dwMidiDiff )
		{
			if ( dwMidiDiff > pMI->LowLat.Midi.dwMidiBufferSize )
				dwMidiDiff += pMI->LowLat.Midi.dwMidiBufferSize;
														// Handle the wrap around

			// Return the read offset and count to the caller
			pReadMidi->dwStartIndex[ 0 ] = pMI->LowLat.Midi.dwMidiInReadOffset[ 0 ];
			pReadMidi->dwByteCount[ 0 ] = dwMidiDiff;

			//	Update circular midi input buffer offset
			pMI->LowLat.Midi.dwMidiInReadOffset[ 0 ] =
				pMI->LowLat.Midi.dwMidiInWriteOffset[ 0 ];
			//	
			// We be done!
			//
			bDone = TRUE;
		}
		//
		// Interrupts back On
		//
		_asm popf
		if ( NULL == pMI->LowLat.Midi.MidiSem )
		{
			DPF( ("VECHOGAL_ReadMidi: low latency mode closed while blocked "
					"for MIDI!\n") );
			return( LOWLATRET_NOTOPEN );
		}
		//
		//	Make sure we don't leave the DSP hung waiting for an unblock
		//	or it will never recover w/o rebooting the PC.
		//	
		UnblockMidiInterrupt( pMI, FALSE );
	}

	DPF( ("VECHOGAL_ReadMidi: OK\n") );
	return( LOWLATRET_OK );
}		// VECHOGAL_ReadMidi


//------------------------------------------------------------------------------
//
// VECHOGAL_WriteMidi
//
//------------------------------------------------------------------------------
DWORD VECHOGAL_WriteMidi( PLOWLATWRITEMIDI	pWriteMidi )
{
	DWORD					dwCardID, dwLength;
	PCOMMPAGE			pCommPage;
	PMONKEYINFO			pMI;
	PDWORD				lpMonkeyBase;
	PBYTE 				pData;

	pData = (PBYTE) &pWriteMidi->byData;
	dwCardID = pWriteMidi->dwCardID;
	dwLength = pWriteMidi->dwByteCount;

	pMI = FindTheMonkey( dwCardID );
	if ( NULL == pMI )
		return( LOWLATRET_BADCARDID );
	pCommPage = (PCOMMPAGE) pMI->dwCommPageHandle;
	lpMonkeyBase = (PDWORD) pMI->dwMonkeyBase;

	return( WriteMidiBytes( pMI, pData, dwLength ) );
}		// VECHOGAL_WriteMidi


//------------------------------------------------------------------------------
//
// WriteMidiBytes
//
//------------------------------------------------------------------------------

#define HANDSHAKE_TIMEOUT	500

DWORD WriteMidiBytes( PMONKEYINFO	pMI,
							 PBYTE 			pData,
							 DWORD			dwLength )
{
	PCOMMPAGE	pCommPage;
	PDWORD		lpMonkeyBase;
	DWORD			dwLengthTemp, dwShift, dwPacked, i;

	pCommPage    = (PCOMMPAGE) pMI->dwCommPageHandle;
	lpMonkeyBase = (PDWORD) pMI->dwMonkeyBase;

	while ( dwLength > 0 )
	{
		if ( dwLength > 3 )
			dwLengthTemp = 3;
		else
			dwLengthTemp = dwLength;
		
		dwShift = 16;
		dwPacked = 0;
		for ( i = 0; i < dwLengthTemp; i++ )
		{
			dwPacked |= (DWORD) *pData++ << dwShift;
			dwShift -= 8;
		}

		dwPacked |= dwLengthTemp << 24;		
		dwLength -= dwLengthTemp;

		//
		// Wait up to two milliseconds for the handshake from the DSP 
		//
		for (i=0; i < HANDSHAKE_TIMEOUT; i++)
		{
			// Look for the handshake value
			if ( 0xffffffff == pCommPage->dwHandshake )
				break;

			// Give the DSP time to access the comm page
				CONFIGMG_Yield( 2, CM_YIELD_NO_RESUME_EXEC );
		}

		if (HANDSHAKE_TIMEOUT == i)
		{
			DEBUGBREAK;
			DPF( ("WriteMidiBytes: Time-out waiting for DSP to consume "
					"data\n") );
			return( LOWLATRET_BUSY );
		}

		//
		// So send the packed MIDI bytes already
		//
		pCommPage->dwHandshake = 0;	// Clear the handshake
		pCommPage->dwMIDIOutData = dwPacked;
		SendVector( pMI, MIDI_WRITE );

	} // dwLength 

	return( LOWLATRET_OK );
}		// WriteMidiBytes

// **** End of LLI.c ****
