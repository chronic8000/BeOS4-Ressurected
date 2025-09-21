//   DARLA.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1999  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Darla-specific routines
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

extern bool ginaCheckSampleRate(DWORD dwSampleRate);
extern status_t ginaSetSampleRate(PMONKEYINFO pMI,uint32 dwSampleRate);
extern status_t ginaGetNominalLevel(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);
status_t darlaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock);
extern int32 irq_handler(void *cookie);
extern WORD pwDarlallDSP[];

JACKCOUNT darla_jack_count = 
{
	DARLA_ANALOG_OUT,
	DARLA_DIGITAL_OUT,
	DARLA_ANALOG_IN,
	DARLA_DIGITAL_IN,
	DARLA_OUT_JACKS,
	DARLA_IN_JACKS,
	DARLA_JACKS,
	DARLA_ANALOG_CONNECTOR
};

// How many there are of each type of node
// This duplicates some information that's in the jack count structure
// Merge the two someday when I have nothing better to do.
int32 darla_node_type_counts[NUM_ECHO_NODE_TYPES] =
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
int32 darla_channel_map[DARLA_JACKS*2];

CARDTYPE DarlaCard = 
	{
		ginaCheckSampleRate,
		ginaSetSampleRate,
		darlaSetInputClock,
		NULL,							// SetOutputClock
		NULL,							// SetNominalOutLevel
		NULL,							// SetNominalInLevel		
		ginaGetNominalLevel,
		ginaGetNominalLevel,
		irq_handler,
		&darla_jack_count,		// jc
		darla_node_type_counts,	// node_type_counts
		darla_channel_map,		// channel_map
		"darla",						// szName
		pwDarlallDSP,				// pwDspCode		
		0,								// dwNumMidiPorts
		B_SR_11025 |				// dwSampleRates
		B_SR_22050 |
		B_SR_44100 |
		B_SR_48000 |
		B_SR_IS_GLOBAL,
		0.0,							// flMinCvsrRate
		0.0,							// flMaxCvsrRate
		B_MULTI_LOCK_INTERNAL,	// dwLockSources
		0								// dwTimecodeSources
	};
	


/*------------------------------------------------------------------------

	darlaSetInputClock
	
------------------------------------------------------------------------*/

status_t darlaSetInputClock(PMONKEYINFO pMI,uint16 wInputClock)
{

	// Darla only has internal input clock
	if (CLK_CLOCKININTERNAL == wInputClock)
		return B_OK;
		
	return B_ERROR;

} // darlaSetInputClock
