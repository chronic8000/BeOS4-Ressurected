//   DARLA24.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1999  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Darla24-specific routines
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
#include "mixer.h"

// Constants used by Darla24
#define D24_96000		0x0
#define D24_48000		0x1
#define D24_44100		0x2
#define D24_32000		0x3
#define D24_22050		0x4
#define D24_16000		0x5
#define D24_11025		0x6
#define D24_8000		0x7
#define D24_88200		0x8
#define D24_ESYNC		0x9


bool darla24CheckSampleRate(DWORD dwSampleRate);
status_t darla24SetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate);
status_t darla24SetInputClock(PMONKEYINFO pMI,uint16 wInputClock);
status_t darla24SetNominalOutLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel);
status_t darla24GetNominalOutLevel(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel);
status_t darla24SetNominalInLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel);
status_t darla24GetNominalInLevel(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel);
extern int32 irq_handler(void *cookie);
extern WORD pwDrla24llDSP[];

JACKCOUNT darla24_jack_count = 
{
	DARLA_ANALOG_OUT,
	DARLA_DIGITAL_OUT,
	DARLA_ANALOG_IN,
	DARLA_DIGITAL_IN,
	DARLA_OUT_JACKS,
	DARLA_IN_JACKS,
	DARLA_JACKS,
	DARLA24_ANALOG_CONNECTOR
};

// How many there are of each type of node
// This duplicates some information that's in the jack count structure
// Merge the two someday when I have nothing better to do.
int32 darla24_node_type_counts[NUM_ECHO_NODE_TYPES] =
{
	0,									// dummy
	DARLA_ANALOG_IN_GAIN,				// ANALOG_IN_GAIN
	DARLA_ANALOG_OUT_GAIN,				// ANALOG_OUT_GAIN
	DARLA_MONITOR_GAIN,					// MONITOR_GAIN
	DARLA_ANALOG_OUT,					// ANALOG_OUT_CHANNEL
	DARLA_DIGITAL_OUT,					// DIGITAL_OUT_CHANNEL
	DARLA_IN_JACKS,						// IN_CHANNEL
	DARLA_OUT_JACKS,					// OUT_BUS
	DARLA_ANALOG_IN,					// ANALOG_IN_BUS
	DARLA_DIGITAL_IN					// DIGITAL_IN_BUS	
};

// Array for mapping Be channel & bus numbers to internal ID numbers
int32 darla24_channel_map[DARLA_JACKS*2];

CARDTYPE Darla24Card = 
	{
		darla24CheckSampleRate,
		darla24SetSampleRate,
		darla24SetInputClock,
		NULL,									// SetOutputClock
		darla24SetNominalOutLevel,		// SetNominalOutLevel
		darla24SetNominalInLevel,		// SetNominalInLevel		
		darla24GetNominalOutLevel,
		darla24GetNominalInLevel,
		irq_handler,
		&darla24_jack_count,				// jc
		darla24_node_type_counts,		// node_type_counts
		darla24_channel_map,				// channel_map
		"darla24",								// szName
		pwDrla24llDSP,						// pwDspCode		
		0,										// dwNumMidiPorts
		B_SR_8000  |						// dwSampleRates
		B_SR_11025 |
		B_SR_16000 |
		B_SR_22050 |
		B_SR_32000 |
		B_SR_44100 |
		B_SR_48000 |
		B_SR_88200 |
		B_SR_96000 |
		B_SR_IS_GLOBAL,
		0.0,							// flMinCvsrRate
		0.0,							// flMaxCvsrRate
		B_MULTI_LOCK_INTERNAL |	// dwLockSources
		B_MULTI_LOCK_SUPERCLOCK,
		0								// dwTimecodeSources
	};
	

//--------------------------------------------------------------------------
// darla24CheckSampleRate
// 
// Check the sample rate for Darla24
//
//--------------------------------------------------------------------------

bool darla24CheckSampleRate(DWORD dwSampleRate)
{
	switch (dwSampleRate)
	{
		case 8000 :
		case 11025 :
		case 16000 :
		case 22050 : 
		case 32000 :
		case 44100 :
		case 48000 :
		case 88200 :
		case 96000 :
			return true;
	}

	return false;

} // darla24CheckSampleRate


//--------------------------------------------------------------------------
//
// darla24SetSampleRate
// 
// Set the sample rate for Darla
//
//--------------------------------------------------------------------------

status_t darla24SetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate)
{
	PCOMMPAGE	pCommPage;
	BYTE		bClock;

	pCommPage = (PCOMMPAGE) pMI->pCommPage;	

	// Check to see if the caller doesn't really want to change the sample
	// rate, but just use the one already stored in the MI
	if (SAMPLE_RATE_NO_CHANGE == dwSampleRate)
		dwSampleRate = pMI->dwSampleRate;

	bClock = D24_48000;		// just to make the compiler happy
	switch (dwSampleRate)
	{
		case 96000 :
			bClock = D24_96000;
			break;
		case 88200 :
			bClock = D24_88200;
			break;
		case 48000 : 
			bClock = D24_48000;
			break;
		case 44100 :
			bClock = D24_44100 ;
			break;		
		case 32000 :
			bClock = D24_32000 ;
			break;		
		case 22050 :
			bClock = D24_22050 ;
			break;		
		case 16000 : 
			bClock = D24_16000 ;
			break;
		case 11025 :
			bClock = D24_11025 ;
			break;		
		case 8000 :
		 	bClock = D24_8000 ;
			break;
	}

	//
	// Override the sample rate if this card is set to Echo sync.  
	// pCommPage->wInputClock is just being used as a parameter here;
	// the DSP ignores it.
	//	
	if (CLK_CLOCKINSUPER == pMI->wInputClock)
		bClock = D24_ESYNC;

	pCommPage->bGDClockState = bClock;
	pCommPage->bGDSpdifStatus = 0;
	pCommPage->bGDResamplerState = 0;

	// Send command to DSP
	SendVector( pMI, SET_GD_AUDIO_STATE );
	
	pMI->dwSampleRate = dwSampleRate;

	DPF2( ( "\tdarla24SetSampleRate: Darla24 sample rate changed to %d\n",
			 dwSampleRate ) );

	return B_OK;

} // darla24SetSampleRate


/*------------------------------------------------------------------------

	darla24SetInputClock
	
------------------------------------------------------------------------*/

status_t darla24SetInputClock(PMONKEYINFO pMI,uint16 wInputClock)
{
	status_t rval;	

	switch (wInputClock)
	{
		case CLK_CLOCKININTERNAL :
		case CLK_CLOCKINSUPER :
			pMI->wInputClock = wInputClock;
			rval = darla24SetSampleRate(pMI,SAMPLE_RATE_NO_CHANGE);
			break;
			
		default :
			rval = B_BAD_VALUE;
			break;			
	}

	return rval;

} // darlaSetInputClock


/*------------------------------------------------------------------------

	darla24SetNominalOutLevel
	
------------------------------------------------------------------------*/

status_t darla24SetNominalOutLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel)
{
	return set_nominal_level(pMI,iChannel,iLevel,false);
} // darla24SetNominalOutLevel


/*------------------------------------------------------------------------

	darla24GetNominalOutLevel
	
------------------------------------------------------------------------*/

status_t darla24GetNominalOutLevel(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel)
{
	get_nominal_level(pMI,iChannel,piLevel,false);

	return B_OK;
	
} // darla24GetNominalOutLevel


/*------------------------------------------------------------------------

	darla24SetNominalInLevel
	
------------------------------------------------------------------------*/

status_t darla24SetNominalInLevel(PMONKEYINFO pMI,int32 iChannel,int32 iLevel)
{
	return set_nominal_level(pMI,iChannel,iLevel,true);
} // darla24SetNominalInLevel


/*------------------------------------------------------------------------

	darla24GetNominalInLevel
	
------------------------------------------------------------------------*/

status_t darla24GetNominalInLevel(PMONKEYINFO pMI, int32 iChannel, int32 *piLevel)
{
	get_nominal_level(pMI,iChannel,piLevel,true);

	return B_OK;
	
} // darla24GetNominalInLevel
