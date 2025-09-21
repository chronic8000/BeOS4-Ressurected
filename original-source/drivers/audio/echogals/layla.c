//   LAYLA.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1999  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Layla-specific routines
//---------------------------------------------------------------------------

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include "multi_audio.h"
#include "monkey.h"
#include "56301.h"
#include "commpage.h"
#include "util.h"
#include "api.h"
#include "filter.h"
#include "mixer.h"

bool laylaCheckSampleRate(DWORD dwSampleRate);
status_t laylaSetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate);
status_t laylaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock);
status_t laylaSetOutputClock(PMONKEYINFO pMI,uint16 wOutputClock);
status_t laylaSetNominalOutLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel);
status_t laylaGetNominalOutLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);
status_t laylaGetNominalInLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);
extern int32 layla_irq_handler(void *cookie);
extern WORD pwLaylallDSP[];

JACKCOUNT layla_jack_count = 
{
	LAYLA_ANALOG_OUT,
	LAYLA_DIGITAL_OUT,
	LAYLA_ANALOG_IN,
	LAYLA_DIGITAL_IN,
	LAYLA_OUT_JACKS,
	LAYLA_IN_JACKS,
	LAYLA_JACKS,
	LAYLA_ANALOG_CONNECTOR
};

// How many there are of each type of node
// This duplicates some information that's in the jack count structure
// Merge the two someday when I have nothing better to do.
int32 layla_node_type_counts[NUM_ECHO_NODE_TYPES] =
{
	0,									// dummy
	LAYLA_ANALOG_IN_GAIN,				// ANALOG_IN_GAIN
	LAYLA_ANALOG_OUT_GAIN,				// ANALOG_OUT_GAIN
	LAYLA_MONITOR_GAIN,					// MONITOR_GAIN
	LAYLA_ANALOG_OUT,					// ANALOG_OUT_CHANNEL
	LAYLA_DIGITAL_OUT,					// DIGITAL_OUT_CHANNEL
	LAYLA_IN_JACKS,						// IN_CHANNEL
	LAYLA_OUT_JACKS,					// OUT_BUS
	LAYLA_ANALOG_IN,					// ANALOG_IN_BUS
	LAYLA_DIGITAL_IN					// DIGITAL_IN_BUS	
};

// Array for mapping Be channel & bus numbers to internal ID numbers
int32 layla_channel_map[LAYLA_JACKS*2];

	
CARDTYPE LaylaCard = 
	{
		laylaCheckSampleRate,
		laylaSetSampleRate,
		laylaSetInputClock,
		laylaSetOutputClock,
		laylaSetNominalOutLevel,
		NULL,
		laylaGetNominalOutLevel,
		laylaGetNominalInLevel,
		layla_irq_handler,
		&layla_jack_count,		// jc
		layla_node_type_counts,	// node_type_counts
		layla_channel_map,		// channel_map		
		"layla",						// szName
		pwLaylallDSP,				// pwDspCode		
		1,								// dwNumMidiPorts
		B_SR_8000  |				// dsSampleRates
		B_SR_11025 |
		B_SR_12000 |
		B_SR_16000 |
		B_SR_22050 |
		B_SR_24000 |
		B_SR_32000 |
		B_SR_44100 |
		B_SR_48000 |
		B_SR_IS_GLOBAL |
		B_SR_CVSR,
		8000.0,						// flMinCvsrRate
		50000.0,						// flMaxCvsrRate
		B_MULTI_LOCK_INTERNAL |	// dwLockSources
		B_MULTI_LOCK_WORDCLOCK |
		B_MULTI_LOCK_SUPERCLOCK |
		B_MULTI_LOCK_SPDIF,
		B_MULTI_TIMECODE_MTC		// dwTimecodeSources
	};



//--------------------------------------------------------------------------
// laylaCheckSampleRate
// 
// Check the sample rate for Layla
//
//--------------------------------------------------------------------------

bool laylaCheckSampleRate(DWORD dwSampleRate)
{

	if ((dwSampleRate >= 8000) && (dwSampleRate <= 50000))
		return true;

	return false;

} // laylaCheckSampleRate


//--------------------------------------------------------------------------
// laylaSetSampleRate
// 
// Set the sample rate for Layla
//
// Layla is simple; just send it the sampling rate (assuming that the clock
// mode is correct).
//
//--------------------------------------------------------------------------

status_t laylaSetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate)
{
	PCOMMPAGE pCommPage;

	DPF2(("DGL: laylaSetSampleRate\n"));
	
	// Check to see if the caller doesn't really want to change the sample
	// rate, but just use the one already stored in the MI
	if (SAMPLE_RATE_NO_CHANGE == dwSampleRate)
		dwSampleRate = pMI->dwSampleRate;
	
#if 0   	
			if ( pMI->MTC.bFlags & MTC_FLAG_SYNC_ON )
			{
				pMI->MTC.dwBaseSampleRate = dwSampleRate;
				DPF2( ( "SetSampleRate: Layla clock already set to "
						 "MTC_FLAG_SYNCH_ON\n" ) );
			}

			if ( pMI->MTC.bFlags & MTC_FLAG_SYNCHED )
			{
				DPF2( ( "SetSampleRate: Layla clock already set to "
						 "MTC_FLAG_SYNCHED\n" ) );
				return;
			}
#endif			
			
	// Only set the clock for internal mode
	if ( pMI->wInputClock != CLK_CLOCKININTERNAL )
	{
		DPF2( ( "\tlaylaSetSampleRate: Layla clock already set to "
				 "CLK_CLOCKININTERNAL\n" ) );
		return B_BAD_VALUE;
	}
	
	// Sanity check - skip if the new rate is 0
	// fixme use checksamplerate
	if ( (dwSampleRate < 8000) || (dwSampleRate > 50000) )
	{
		DPF2( ( "\tlaylaSetSampleRate: Layla sample rate out of range, no change made\n" ) );
		DEBUGBREAK;
		return B_BAD_VALUE;
	}

	pCommPage = pMI->pCommPage;
	pCommPage->dwSampleRate = B_HOST_TO_LENDIAN_INT32(dwSampleRate);
	SendVector( pMI,SET_LAYLA_SAMPLE_RATE );

	pMI->dwSampleRate = dwSampleRate;
	
	DPF2( ( "\tlaylaSetSampleRate: Layla sample rate changed to %d\n",
			 dwSampleRate ) );

	return B_OK;

} // laylaSetSampleRate

/*------------------------------------------------------------------------

	laylaSetInputClock
	
------------------------------------------------------------------------*/

status_t laylaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock)
{
	PCOMMPAGE 	pCommPage;
	bool			fSetRate;
	
	DPF2(("DGL: laylaSetInputClock\n"));
	
	pMI->wInputClock = wInputClock;
	
	pCommPage = pMI->pCommPage;
	fSetRate = false;
	switch ( wInputClock )
	{
		case CLK_CLOCKININTERNAL : 
		{
			DPF2( ( "\tSet Layla clock to INTERNAL\n" ) );
	
			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ((pMI->dwSampleRate < 8000) ||
			    (pMI->dwSampleRate > 50000))
			{
				pMI->dwSampleRate = 48000;
			}
			
			if (pMI->wInputClock != CLK_CLOCKININTERNAL)
				fSetRate = true;
				
			pCommPage->wInputClock = 
				B_HOST_TO_LENDIAN_INT16( CLK_CLOCKININTERNAL );
			
			break;
		} // CLK_CLOCKININTERNAL
		case CLK_CLOCKINSPDIF:
		{
			DPF2( ( "\tSet Layla clock to SPDIF\n" ) );
	
			pCommPage->wInputClock = 
				B_HOST_TO_LENDIAN_INT16( CLK_CLOCKINSPDIF );
			break;
		} // CLK_CLOCKINSPDIF
		case CLK_CLOCKINWORD: 
		{
			DPF2( ( "\tSet Layla clock to WORD\n" ) );
			
			pCommPage->wInputClock = 
				B_HOST_TO_LENDIAN_INT16( CLK_CLOCKINWORD );
	
			break;
		} // CLK_CLOCKINWORD
		case CLK_CLOCKINSUPER: 
		{
			DPF2( ( "\tSet Layla clock to SUPER WORD\n" ) );
			
			pCommPage->wInputClock = 
				B_HOST_TO_LENDIAN_INT16( CLK_CLOCKINSUPER );
	
			break;						
		} // CLK_CLOCKINSUPER
		case CLK_CLOCKINMIDI:
		{
			DPF2( ( "\tMTC clock not yet supported\n" ) );
			
			pCommPage->wInputClock = 
				B_HOST_TO_LENDIAN_INT16( CLK_CLOCKININTERNAL );
	
		}	// CLK_CLOCKINMIDI
		
		default :
			dprintf("Input clock 0x%x not supported for Layla\n");
			return B_BAD_VALUE;
		
	}		// switch (pMI->wInputClock)
	
	SendVector( pMI, UPDATE_CLOCKS );
	
	// Set Layla sample rate to something sane if word or superword is
	// being turned off
	if ( fSetRate )
	{
		CT->SetSampleRate(pMI,SAMPLE_RATE_NO_CHANGE);
	}

	return B_OK;
	
} // laylaSetInputClock


/*------------------------------------------------------------------------

	laylaSetOutputClock
	
------------------------------------------------------------------------*/

status_t laylaSetOutputClock(PMONKEYINFO pMI,uint16 wOutputClock)
{
	PCOMMPAGE pCommPage;
	
	pCommPage = pMI->pCommPage;
	
	pCommPage->wOutputClock = B_HOST_TO_LENDIAN_INT16(wOutputClock);
	
	SendVector( pMI, UPDATE_CLOCKS );

	return B_OK;

} // laylaSetOutputClock


/*------------------------------------------------------------------------

	laylaSetNominalOutLevel
	
------------------------------------------------------------------------*/

status_t set_nominal_level(PMONKEYINFO pMI, int32 iChannel, int32 iLevel,bool fInput)
{
	PCOMMPAGE	pCommPage;
	DWORD			dwMask;

	pCommPage = pMI->pCommPage;	
	
	// If channel is -1, set all output nominal levels
	if (-1 == iChannel)
	{
		if (fInput)
		{
			dwMask = (1 << JC->NumAnalogIn) - 1;	
			dwMask <<= JC->NumOut;
		}
		else
		{
			dwMask = (1 << JC->NumAnalogOut) - 1;
		}
	}
	else
	{
		dwMask = 1 << iChannel;
		if (fInput)
		{
			dwMask <<= JC->NumOut;
		}
	}
	
	if (B_MULTI_NOMINAL_MINUS_10 == iLevel)
	{
		// Clear the bit(s) for -10
		pCommPage->dwNominalLevels &= ~dwMask;
	}
	else
	{
		// Set the bit(s) for +4
		pCommPage->dwNominalLevels |= dwMask;
	}
	
	// This vector command sets both input and output nominal levels
	SendVector(pMI,UPDATE_OUTGAIN);				

	return B_OK;
			
} // set_nominal_level


status_t laylaSetNominalOutLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel)
{
	return set_nominal_level(pMI,iChannel,iLevel,false);
} // laylaSetNominalOutLevel


/*------------------------------------------------------------------------

	laylaGetNominalOutLevel
	
------------------------------------------------------------------------*/

void get_nominal_level(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel,bool fInput)
{
	PCOMMPAGE	pCommPage;
	DWORD			dwMask;

	pCommPage = pMI->pCommPage;	
	
	dwMask = 1 << iChannel;
	if (fInput)
		dwMask <<= JC->NumOut;

	// The bit is set for -10, clear for +4
	if ( dwMask & pCommPage->dwNominalLevels )
		*piLevel = B_MULTI_NOMINAL_MINUS_10;
	else
		*piLevel = B_MULTI_NOMINAL_PLUS_4;
		
} // get_nominal_level

status_t laylaGetNominalOutLevel(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel)
{
	get_nominal_level(pMI,iChannel,piLevel,false);

	return B_OK;
	
} // laylaGetNominalOutLevel


/*------------------------------------------------------------------------

	laylaGetNominalInLevel
	
------------------------------------------------------------------------*/

status_t laylaGetNominalInLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel)
{

	// For now, always return -10 for Layla inputs; fixme change this when
	// I have nothing better to do
	*piLevel = B_MULTI_NOMINAL_MINUS_10;
	
	return B_OK;
	
} // laylaGetNominalInLevel


