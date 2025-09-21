//   GINA.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1999  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Gina-specific routines
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

bool ginaCheckSampleRate(DWORD dwSampleRate);
status_t ginaSetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate);
status_t ginaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock);
status_t ginaGetNominalLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);
extern int32 irq_handler(void *cookie);
extern WORD pwGinallDSP[];

JACKCOUNT gina_jack_count = 
{
	GINA_ANALOG_OUT,
	GINA_DIGITAL_OUT,
	GINA_ANALOG_IN,
	GINA_DIGITAL_IN,
	GINA_OUT_JACKS,
	GINA_IN_JACKS,
	GINA_JACKS,
	GINA_ANALOG_CONNECTOR
};

// How many there are of each type of node
// This duplicates some information that's in the jack count structure
// Merge the two someday when I have nothing better to do.
int32 gina_node_type_counts[NUM_ECHO_NODE_TYPES] =
{
	0,									// dummy
	GINA_ANALOG_IN_GAIN,				// ANALOG_IN_GAIN
	GINA_ANALOG_OUT_GAIN,				// ANALOG_OUT_GAIN
	GINA_MONITOR_GAIN,					// MONITOR_GAIN
	GINA_ANALOG_OUT,					// ANALOG_OUT_CHANNEL
	GINA_DIGITAL_OUT,					// DIGITAL_OUT_CHANNEL
	GINA_IN_JACKS,						// IN_CHANNEL
	GINA_OUT_JACKS,						// OUT_BUS
	GINA_ANALOG_IN,						// ANALOG_IN_BUS
	GINA_DIGITAL_IN						// DIGITAL_IN_BUS	
};

// Array for mapping Be channel & bus numbers to internal ID numbers
int32 gina_channel_map[GINA_JACKS*2];

CARDTYPE GinaCard = 
	{
		ginaCheckSampleRate,
		ginaSetSampleRate,
		ginaSetInputClock,
		NULL,							// SetOutputClock
		NULL,							// SetNominalOutLevel
		NULL,							// SetNominalInLevel
		ginaGetNominalLevel,
		ginaGetNominalLevel,
		irq_handler,
		&gina_jack_count,				// jc
		gina_node_type_counts,			// node_type_counts
		gina_channel_map,				// channel_map		
		"gina",							// szName
		pwGinallDSP,					// pwDspCode
		0,								// dwNumMidiPorts
		B_SR_11025 |					// dwSampleRates
		B_SR_22050 |
		B_SR_44100 |
		B_SR_48000 |
		B_SR_IS_GLOBAL,
		0.0,							// flMinCvsrRate
		0.0,							// flMaxCvsrRate
		B_MULTI_LOCK_INTERNAL | 		// dwLockSources
		B_MULTI_LOCK_SPDIF,
		0								// dwTimecodeSources
		
	};


//--------------------------------------------------------------------------
// ginaCheckSampleRate
// 
// Check the sample rate for Darla and Gina
//
//--------------------------------------------------------------------------

bool ginaCheckSampleRate(DWORD dwSampleRate)
{
	switch (dwSampleRate)
	{
		case 11025 :
		case 22050 : 
		case 44100 :
		case 48000 :
			return true;
	}

	return false;

} // ginaCheckSampleRate


//--------------------------------------------------------------------------
//
// CopyGinaDarlaFilters
//
//--------------------------------------------------------------------------
void CopyGinaDarlaFilters( DWORD dwSampleRate, PMONKEYINFO pMI )
{
	PCOMMPAGE	pCommPage;
	int			*piPlayCoeff,*piRecCoeff;
	int			i,iPlayTaps,iRecordTaps;

	pCommPage = pMI->pCommPage;
	
	switch ( dwSampleRate )
	{
	 	case 11025 :
			DPF2( ( "CopyGinaDarlaFilters: 11025\n" ) );
			piPlayCoeff = PlayFilter11;
			piRecCoeff = RecordFilter11;
			iPlayTaps = PLAY_FILTER11_TAPS;
			iRecordTaps = RECORD_FILTER11_TAPS;
			break;
		case 22050 : 
			DPF2( ( "CopyGinaDarlaFilters: 22050\n" ) );
			piPlayCoeff = PlayFilter22;
			piRecCoeff = RecordFilter22;
			iPlayTaps = PLAY_FILTER22_TAPS;	
			iRecordTaps = RECORD_FILTER22_TAPS;
			break;
		case 32000:
			DPF2( ( "CopyGinaDarlaFilters: 32000\n" ) );
			piPlayCoeff = PlayFilter32;
			piRecCoeff = RecordFilter32;
			iPlayTaps = PLAY_FILTER32_TAPS;
			iRecordTaps = RECORD_FILTER32_TAPS;
			break;
		default:
			DPF2( ( "CopyGinaDarlaFilters: None avaiable\n" ) );
			return;
	}
	
	// Copy play filter				
	for ( i = 0; i<iPlayTaps; i++ )
		pCommPage->dwPlayCoeff[i] = B_HOST_TO_LENDIAN_INT32(piPlayCoeff[i]);
						
	// Copy record filter
	for (i=0; i<iRecordTaps; i++)
		pCommPage->dwRecCoeff[i] = B_HOST_TO_LENDIAN_INT32(piRecCoeff[i]);
		
}	// End of CopyGinaDarlaFilters


//--------------------------------------------------------------------------
// ConvertGinaDarlaRate
//
// Converts a Gina/Darla sampling rate to the physical rate
// 
//--------------------------------------------------------------------------
DWORD ConvertGinaDarlaRate(DWORD dwRate)
{
	switch ( dwRate )
	{
		case 11025 :
	 	case 22050 : 
			DPF2( ( "ConvertGinaDarlaRate: %d to 44100\n", dwRate ) );
	 		return( 44100 );

		case 32000 : 
			DPF2( ( "ConvertGinaDarlaRate: %d to 48000\n", dwRate ) );
			return( 48000 );
	}
	
	DPF2( ( "ConvertGinaDarlaRate: %d unchanged\n", dwRate ) );
	return( dwRate );
}	//DWORD ConvertGinaDarlaRate(DWORD dwRate)


//--------------------------------------------------------------------------
// SelectGinaDarlaClock
// 
//--------------------------------------------------------------------------
BYTE SelectGinaDarlaClock( DWORD dwSampleRate, PMONKEYINFO pMI )
{
	DWORD	dwPhysNewRate;
	BYTE	bNewClock;
	
	dwPhysNewRate = ConvertGinaDarlaRate( dwSampleRate );

	if ( dwPhysNewRate == 44100 )
	{
		bNewClock = GD_CLOCK_44;
		DPF2( ( "SelectGinaDarlaClock: GD_CLOCK_44\n" ) );
	}
	else
	{
		bNewClock = GD_CLOCK_48;
		DPF2( ( "SelectGinaDarlaClock: GD_CLOCK_48\n" ) );
	}

	if ( bNewClock == pMI->bGDCurrentClockState )
	{
		DPF2( ( "SelectGinaDarlaClock: GD_CLOCK_NOCHANGE\n" ) );
		return( GD_CLOCK_NOCHANGE );
	}
	return( bNewClock );
}	// End of SelectGinaDarlaClock


//--------------------------------------------------------------------------
// SelectGinaDarlaResampler
// 
//--------------------------------------------------------------------------
BYTE SelectGinaDarlaResampler(DWORD dwSampleRate, PMONKEYINFO pMI)
{
	// Determine the resampler state
	switch ( dwSampleRate )
	{
		case 11025 : 
			DPF2( ( "SelectGinaDarlaResampler: GD_RESAMPLE_11\n" ) );
			return GD_RESAMPLE_11;

		case 22050 : 
			DPF2( ( "SelectGinaDarlaResampler: GD_RESAMPLE_22\n" ) );
			return GD_RESAMPLE_22;

		case 32000 :
			DPF2( ( "SelectGinaDarlaResampler: GD_RESAMPLE_32\n" ) );
			return  GD_RESAMPLE_32;

		case 44100 : 
		case 48000 : 
		default : 
			DPF2( ( "SelectGinaDarlaResampler: GD_RESAMPLE_SAME\n" ) );
			return  GD_RESAMPLE_SAME;		
	}		
} // End of SelectGinaDarlaResampler


//--------------------------------------------------------------------------
//
// SelectGinaDarlaSpdifStatus
// 
//--------------------------------------------------------------------------
BYTE SelectGinaDarlaSpdifStatus(DWORD dwSampleRate,PMONKEYINFO pMI)
{
	DWORD		dwPhysNewRate;
	BYTE		bNewStatus;
	
	dwPhysNewRate = ConvertGinaDarlaRate( dwSampleRate );
	
	if ( dwPhysNewRate == 44100 )
	{
		bNewStatus = GD_SPDIF_STATUS_44;
		DPF2( ( "SelectGinaDarlaSpdifStatus: GD_SPDIF_STATUS_44\n" ) );
	}
	else
	{
 		bNewStatus = GD_SPDIF_STATUS_48;
		DPF2( ( "SelectGinaDarlaSpdifStatus: GD_SPDIF_STATUS_48\n" ) );
	}
	if ( bNewStatus == pMI->bGDCurrentSpdifStatus )
	{
		bNewStatus = GD_SPDIF_STATUS_NOCHANGE;		
		DPF2( ( "SelectGinaDarlaSpdifStatus: GD_SPDIF_STATUS_NOCHANGE\n" ) );
	}

	return( bNewStatus );
}	// End of SelectGinaDarlaSpdifStatus

//--------------------------------------------------------------------------
//
// ginaSetSampleRate
// 
// Set the sample rate for Darla and Gina
//
//--------------------------------------------------------------------------

status_t ginaSetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate)
{
	PCOMMPAGE	pCommPage;
	BYTE		bGDClockState,bGDResamplerState,bGDSpdifStatus;
	DWORD		fUseSpdifInClock;

	DPF2(("DGL: ginaSetSampleRate to %ld\n",dwSampleRate));
	
	// Check to see if the caller doesn't really want to change the sample
	// rate, but just use the one already stored in the MI
	if (SAMPLE_RATE_NO_CHANGE == dwSampleRate)
		dwSampleRate = pMI->dwSampleRate;

	pCommPage = pMI->pCommPage;	

	fUseSpdifInClock = pMI->wCardType == GINA &&
					   pMI->wInputClock == CLK_CLOCKINSPDIF;

	// Select the clock
	bGDClockState = SelectGinaDarlaClock( dwSampleRate, pMI );
	if ( fUseSpdifInClock )
		bGDClockState = GD_CLOCK_SPDIFIN;

	if (pMI->bGDCurrentClockState == bGDClockState)
	{
		bGDClockState = GD_CLOCK_NOCHANGE;
	}

	// Select S/PDIF output status
	bGDSpdifStatus = SelectGinaDarlaSpdifStatus( dwSampleRate, pMI );
	if ( fUseSpdifInClock &&
		 dwSampleRate == 32000 &&
		 pMI->bGDCurrentSpdifStatus != GD_SPDIF_STATUS_32 )
	{
		bGDSpdifStatus = GD_SPDIF_STATUS_32;
	}

	// Select the resampler
	bGDResamplerState = SelectGinaDarlaResampler( dwSampleRate, pMI );

	// If S/PDIF input clock is selected for 32 kHz, don't resample
	if ( fUseSpdifInClock && 
	     bGDResamplerState == GD_RESAMPLE_32 )
	{
	    bGDResamplerState = GD_RESAMPLE_SAME;
	}

	// Copy the filter coefficients if necessary			    
	if ( bGDResamplerState != pMI->bGDCurrentResamplerState )
		CopyGinaDarlaFilters( dwSampleRate, pMI );

	// Write the audio state to the comm page
	pCommPage->bGDClockState = bGDClockState;
	pCommPage->bGDSpdifStatus = bGDSpdifStatus;
	pCommPage->bGDResamplerState = bGDResamplerState;

	// Send command to DSP
	SendVector( pMI, SET_GD_AUDIO_STATE );
	
	// Copy the new audio state to the MI
	if ( bGDClockState != GD_CLOCK_NOCHANGE )
		pMI->bGDCurrentClockState = bGDClockState;

	if ( bGDSpdifStatus != GD_SPDIF_STATUS_NOCHANGE )
		pMI->bGDCurrentSpdifStatus = bGDSpdifStatus;

	pMI->bGDCurrentResamplerState = bGDResamplerState;
	
	pMI->dwSampleRate = dwSampleRate;
	
	// Set the appropriate shift factor
	switch (dwSampleRate)
	{
		case 11025 :
			pMI->dwShiftFactor = 2;
			break;
		case 22050 :
			pMI->dwShiftFactor = 1;
			break;
		default :
			pMI->dwShiftFactor = 0;
			break;
	}		

#ifdef DEBUG

	switch ( bGDClockState )
	{
		case GD_CLOCK_NOCHANGE :
			DPF2( ("\tginaSetSampleRate: Set Gina clock to GD_CLOCK_NOCHANGE\n" ) );
			break;
		case GD_CLOCK_44 :
			DPF2( ("\tginaSetSampleRate: Set Gina clock to GD_CLOCK_44\n" ) );
			break;
		case GD_CLOCK_48 :
			DPF2( ("\tginaSetSampleRate: Set Gina clock to GD_CLOCK_48\n" ) );
			break;
		case GD_CLOCK_SPDIFIN :
			DPF2( ("\tginaSetSampleRate: Set Gina clock to GD_CLOCK_SPDIF\n" ) );
			break;
		default :
			DPF2( ("\tginaSetSampleRate: Set Gina clock to GD_CLOCK_UNDEF\n" ) );
			break;
	}
	switch ( bGDResamplerState )
	{
		case GD_RESAMPLE_11 :
			DPF2( ("\tginaSetSampleRate: Set resampler state to GD_RESAMPLE_11\n" ) );
			break;
		case GD_RESAMPLE_22 :
			DPF2( ("\tginaSetSampleRate: Set resampler state to GD_RESAMPLE_22\n" ) );
			break;
		case GD_RESAMPLE_32 :
			DPF2( ("\tginaSetSampleRate: Set resampler state to GD_RESAMPLE_32\n" ) );
			break;
		case GD_RESAMPLE_SAME :
			DPF2( ("\tginaSetSampleRate: Set resampler state to GD_RESAMPLE_SAME\n" ) );
			break;
		default :
			DPF2( ("\tginaSetSampleRate: Set resampler state to GD_RESAMPLE_UNDEF\n" ) );
			break;
	}
	switch ( bGDSpdifStatus )
	{
		case GD_SPDIF_STATUS_NOCHANGE	:
			DPF2( ("\tginaSetSampleRate: SPDIF status set to GD_SPDIF_STATUS_NOCHANGE\n" ) );
			break;
		case GD_SPDIF_STATUS_44	:
			DPF2( ("\tginaSetSampleRate: SPDIF status set to GD_SPDIF_STATUS_44\n" ) );
			break;
		case GD_SPDIF_STATUS_48	:
			DPF2( ("\tginaSetSampleRate: SPDIF status set to GD_SPDIF_STATUS_48\n" ) );
			break;
		case GD_SPDIF_STATUS_32	:
			DPF2( ("\tginaSetSampleRate: SPDIF status set to GD_SPDIF_STATUS_32\n" ) );
			break;
		default :
			DPF2( ("\tginaSetSampleRate: SPDIF status set to GD_SPDIF_STATUS_UNDEF\n" ) );
			break;
	}
	DPF2( ( "\tginaSetSampleRate: new sample rate %d\n", pMI->dwSampleRate ) );
#endif // DEBUG
	
	return B_OK;

} // ginaSetSampleRate

/*------------------------------------------------------------------------

	ginaSetInputClock
	
------------------------------------------------------------------------*/

status_t ginaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock)
{
	PCOMMPAGE pCommPage;
	
	DPF2(("DGL: ginaSetInputClock\n"));
	
	pCommPage = pMI->pCommPage;

	pMI->wInputClock = wInputClock;

	switch ( wInputClock )
	{
		case CLK_CLOCKININTERNAL : 
	
			// Reset the audio state to unknown (just in case)
			pMI->bGDCurrentClockState = GD_CLOCK_UNDEF;
			pMI->bGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;
			pMI->bGDCurrentResamplerState = GD_RESAMPLE_UNDEF;
	
			CT->SetSampleRate( pMI, SAMPLE_RATE_NO_CHANGE);
			DPF2( ( "\tSet Gina clock to INTERNAL\n" ) );
			break;
			
		case CLK_CLOCKINSPDIF : 
			pCommPage->bGDClockState = GD_CLOCK_SPDIFIN;
			pCommPage->bGDSpdifStatus = GD_SPDIF_STATUS_NOCHANGE;
			pCommPage->bGDResamplerState = GD_RESAMPLE_SAME;
	
			SendVector( pMI, SET_GD_AUDIO_STATE );
	
			pMI->bGDCurrentClockState = GD_CLOCK_SPDIFIN;
			pMI->bGDCurrentResamplerState = GD_RESAMPLE_SAME;					
			DPF2( ( "\tSet Gina clock to SPDIF\n" ) );
			break;
		default :
			dprintf("Trying to set Gina to input clock 0x%x; only"
					"supports internal and S/PDIF clock\n",
					wInputClock);
			return B_BAD_VALUE;					
			
	}	// switch ( pMI->wInputClock )
	
	return B_OK;

} // ginaSetInputClock

/*------------------------------------------------------------------------

	ginaGetNominalLevel
	
------------------------------------------------------------------------*/
status_t ginaGetNominalLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel)
{

	// Gina and Darla are always -10
	*piLevel = B_MULTI_NOMINAL_MINUS_10;
	
	return B_OK;
	
} // ginaGetNominalLevel
		

