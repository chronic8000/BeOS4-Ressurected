/*************************************************************************

 multi_control.c
 
 ioctl handlers for the multi device
 
 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <stdio.h>
#include <string.h>
#include "monkey.h"
#include "commpage.h"
#include "multi_audio.h"
#include "multi_control.h"
#include "api.h"
#include "util.h"
#include "mixer.h"
#include "init.h"

/*------------------------------------------------------------------------

	multiGetDescription
	
	Fills out a multi_description structure 
	
------------------------------------------------------------------------*/

status_t multiGetDescription(PMONKEYINFO pMI,multi_description *pMD)
{
	int32 	channel;
	int32 	output_kind;
	int32 	input_kind;
	uint32	analog_connector = 0, j;
	uint32	digital_connector;
	int32	i;
	
	DPF2(("DGL: multiGetDescription\n"));

	if (NULL == pMD)
	{
		dprintf("DarlaGinaLayla: null ptr passed for B_MULTI_GET_DESCRIPTION\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_description) != pMD->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for B_MULTI_GET_DESCRIPTION\n");
		return B_BAD_VALUE;
	}	

	if (NULL != pMD->friendly_name)
	{
		strcpy(pMD->friendly_name,pMI->szFriendlyName);
	}
	else
	{	
		dprintf("DarlaGinaLayla: null ptr passed for friendly name in"
				" B_MULTI_GET_DESCRIPTION\n");
	}
	
	if (NULL != pMD->vendor_info)
	{
		strcpy(pMD->vendor_info,"Echo");
	}
	else
	{	
		dprintf("DarlaGinaLayla: null ptr passed for vendor info in"
				" B_MULTI_GET_DESCRIPTION\n");
	}
	
	pMD->interface_version = B_CURRENT_INTERFACE_VERSION;
	pMD->interface_minimum = B_MINIMUM_INTERFACE_VERSION;
	
	pMD->input_channel_count = JC->NumIn;
	pMD->output_channel_count = JC->NumOut;
	pMD->input_bus_channel_count = JC->NumIn;
	pMD->output_bus_channel_count = JC->NumOut;
	
	channel = 0;

	output_kind = B_MULTI_OUTPUT_CHANNEL;
	input_kind = B_MULTI_INPUT_CHANNEL;
	analog_connector = 0;
	digital_connector = 0;
	if (NULL != pMD->channels)
	{
		for (j = 0; j < 2; j++)
		{
			for (i = 0; i < JC->NumAnalogOut; i++)
			{
				if (channel >= pMD->request_channel_count)
					break;
					
				pMD->channels[channel].channel_id = channel;
				pMD->channels[channel].kind = output_kind;
				pMD->channels[channel].designations = 0;
				pMD->channels[channel].connectors = analog_connector;
				
				channel++;
			}	
		
			for (i = 0; i < JC->NumDigitalOut; i++)
			{
				if (channel >= pMD->request_channel_count)
					break;
					
				pMD->channels[channel].channel_id = channel;
				pMD->channels[channel].kind = output_kind;
				pMD->channels[channel].designations = 0;
				pMD->channels[channel].connectors = digital_connector;
				
				channel++;
			}	
			
			for (i = 0; i < JC->NumAnalogIn; i++)
			{
				if (channel >= pMD->request_channel_count)
					break;
					
				pMD->channels[channel].channel_id = channel;
				pMD->channels[channel].kind = input_kind;
				pMD->channels[channel].designations = 0;
				pMD->channels[channel].connectors = analog_connector;
				
				channel++;
			}	
			
			for (i = 0; i < JC->NumDigitalIn; i++)
			{
				if (channel >= pMD->request_channel_count)
					break;

				pMD->channels[channel].channel_id = channel;
				pMD->channels[channel].kind = input_kind;
				pMD->channels[channel].designations = 0;
				pMD->channels[channel].connectors = digital_connector;
				
				channel++;
			}	
			
			output_kind = B_MULTI_OUTPUT_BUS;
			input_kind = B_MULTI_INPUT_BUS;
			analog_connector = JC->AnalogConnector;
			digital_connector = B_CHANNEL_COAX_SPDIF;	
		} // for j loop
	} // if channels ptr is not null

	pMD->input_rates = CT->dwSampleRates;
	pMD->min_cvsr_rate = CT->flMinCvsrRate;
	pMD->max_cvsr_rate = CT->flMaxCvsrRate;

	pMD->output_rates = pMD->input_rates | B_SR_SAME_AS_INPUT;
	
	pMD->input_formats = B_FMT_24BIT | B_FMT_IS_GLOBAL;
	pMD->output_formats = pMD->input_formats | B_FMT_SAME_AS_INPUT;
	
	pMD->lock_sources = CT->dwLockSources;
	pMD->timecode_sources = CT->dwTimecodeSources;

	pMD->interface_flags = 	B_MULTI_INTERFACE_PLAYBACK | 
						  	B_MULTI_INTERFACE_RECORD |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_OUTPUTS |
							B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_INPUTS;

	if (CT->dwNumMidiPorts != 0)
	{
		pMD->interface_flags |= B_MULTI_INTERFACE_TIMECODE;
	}
	
	// No control panel yet...
	if (NULL != pMD->vendor_info)
	{
		pMD->control_panel[0] = 0;
	}
	else
	{	
		dprintf("DarlaGinaLayla: null ptr passed for control_panel in"
				" B_MULTI_GET_DESCRIPTION\n");
	}
								  
	return B_OK;

} // end of multiGetDescription


/*------------------------------------------------------------------------

	multiBufferExchange
	
	Handles the real work of audio transfers
	
	This function does not have to be thread safe
	
------------------------------------------------------------------------*/

status_t multiBufferExchange(PMONKEYINFO pMI, multi_buffer_info *pMBI)
{
	PCOMMPAGE	pCommPage;
	status_t	rval;
	DWORD			dwDelta;

	if (NULL == pMBI)
	{
		dprintf("DarlaGinaLayla: null ptr passed for B_MULTI_BUFFER_EXCHANGE\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_buffer_info) != pMBI->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for B_MULTI_BUFFER_EXCHANGE\n");
		return B_BAD_VALUE;
	}	

	// If not started, start
	if (0 == pMI->dwActiveChannels)
	{
		if (0 == pMI->dwEnabledChannels)
		{
			DPF2(("DGL: multiBufferExchange called with dwEnabledChannels set to zero\n"));
			return B_ERROR;
		}
	
		pCommPage = pMI->pCommPage;
		pCommPage->dwStart = B_HOST_TO_LENDIAN_INT32(pMI->dwEnabledChannels);
		StartDSP(pMI);		
		pMI->dwActiveChannels = pMI->dwEnabledChannels;
		DPF2(("DGL: multiBufferExchange started %s\n",pMI->szCardBaseName));
	}
	
	// Wait for the interrupt
	rval = acquire_sem_etc(pMI->IrqSem, 1, B_TIMEOUT, 100 * 1000);
	if (B_OK != rval)
	{
		DPF1(("DGL:\tFailed to acquire IRQ semaphore!\n",rval));
#ifdef DEBUG
		switch (rval)
		{
			case B_BAD_SEM_ID : 
				DPF2(("\t\tB_BAD_SEM_ID\n"));
				break;
			case B_INTERRUPTED :
				DPF2(("\t\tB_INTERRUPTED\n"));
				break;
			case B_BAD_VALUE :
				DPF2(("\t\tB_BAD_VALUE\n"));
				break;
			case B_WOULD_BLOCK :
				DPF2(("\t\tB_WOULD_BLOCK\n"));
				break;
			case B_TIMED_OUT :
				DPF2(("\t\tB_TIMED_OUT\n"));
				break;
			default :
				DPF2(("\t\tunknown 0x%x\n",rval));
				break;
		}
#endif
		return rval;
	}
	
	//
	// Figure out qwSampleTimestamp.  Since the sample counter from the
	// DSP is 32 bits, handle it like to this to handle counter wraparound
	//
	// Need to shift down by dwShiftFactor to handle 11 and 22 kHz on
	// Gina and Darla.
	//
	dwDelta = pMI->dwSampleTimestamp - pMI->dwLastSampleTimestamp;
	dwDelta >>= pMI->dwShiftFactor;
	pMI->qwSampleTimestamp += dwDelta;
	pMI->dwLastSampleTimestamp = pMI->dwSampleTimestamp;
	
	// Fill out the data structure and return
	pMBI->flags = pMI->dwMbxFlags;
	pMBI->played_real_time = pMI->qwSystemTimestamp;
	pMBI->played_frames_count = pMI->qwSampleTimestamp;
	pMBI->playback_buffer_cycle = (pMI->dwActualBufferCount - 1) & 1;
	pMBI->recorded_real_time = pMI->qwSystemTimestamp;
	pMBI->recorded_frames_count = pMI->qwSampleTimestamp;
	pMBI->record_buffer_cycle = (pMI->dwActualBufferCount - 1) & 1;
	
	return B_OK;

} // end of multiBufferExchange

/*------------------------------------------------------------------------

	multiGetEnabledChannels
	
------------------------------------------------------------------------*/

status_t multiGetEnabledChannels(PMONKEYINFO pMI,multi_channel_enable *pMCE)
{
	DPF2(("multiGetEnabledChannels\n"));

	if (NULL == pMCE)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_GET_ENABLED_CHANNELS\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_channel_enable) != pMCE->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_GET_ENABLED_CHANNELS\n");
		dprintf("Wanted 0x%x, got 0x%x\n",sizeof(multi_description),pMCE->info_size);
		return B_BAD_VALUE;
	}	

	if (NULL != pMCE->enable_bits)
	{
		*((DWORD *) pMCE->enable_bits) = pMI->dwEnabledChannels;
		DPF2(("\t*enable_bits is 0x%x\n",*((DWORD *) pMCE->enable_bits)));
	}

	pMCE->lock_source = B_MULTI_LOCK_INTERNAL;
	pMCE->lock_data = 0;

	return B_OK;
	
} // end of multiGetEnabledChannels


/*------------------------------------------------------------------------

	multiSetEnabledChannels
	
------------------------------------------------------------------------*/

status_t multiSetEnabledChannels(PMONKEYINFO pMI,multi_channel_enable *pMCE)
{
	DWORD 	dwEnableBits,dwOutMask,dwInMask,dwFlagTemp;
	WORD	wNewInputClock;
#ifdef DEBUG
	DWORD	dwTemp;
#endif
	
	if (NULL == pMCE)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_SET_ENABLED_CHANNELS\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_channel_enable) != pMCE->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_SET_ENABLED_CHANNELS\n");
		return B_BAD_VALUE;
	}	

	
	if (NULL != pMCE->enable_bits)
	{
		dwEnableBits = *((DWORD *) pMCE->enable_bits);
	}
	else
		dwEnableBits = 0;


#ifdef DEBUG
	// Construct a bitmask for all the channels and make sure that
	// the client isn't trying to enabled non-existent channels	
	dwTemp = 1 << JC->NumJacks;
	dwTemp -= 1;
	dwTemp = ~dwTemp;
	
	if ((dwTemp & dwEnableBits) != 0)
	{
		DPF1(("DGL: multiSetEnabledChannels got a bad channel mask\n"));
		DPF1(("DGL:\tGot mask 0x%x, test mask 0x%x\n",dwEnableBits,dwTemp));
		return B_BAD_VALUE;
	}	
#endif

	// If the enable mask has changed, reset the audio hardware first
	if (pMI->dwEnabledChannels != dwEnableBits)
	{
		reset_buffer_exchange_state(pMI);
	}

	// Set the new mask
	pMI->dwEnabledChannels = dwEnableBits;

	DPF2(("DGL: multiSetEnabledChannels set enabled channels to 0x%x\n",
		  pMI->dwEnabledChannels));	
		  
	// Set up the flags value for multiBufferExchange
	dwOutMask = (1 << JC->NumOut) - 1;
	dwInMask = 	((1 << JC->NumIn) - 1) << JC->NumOut;
	dwFlagTemp = 0;
	if (0 != (dwOutMask & dwEnableBits))
		dwFlagTemp |= B_MULTI_BUFFER_PLAYBACK;
	if (0 != (dwInMask & dwEnableBits))
		dwFlagTemp |= B_MULTI_BUFFER_RECORD;
		
	pMI->dwMbxFlags = dwFlagTemp;

	// Map the multi_audio lock source codes to Echo input clock codes
	wNewInputClock = pMI->wInputClock;
	switch (pMCE->lock_source)
	{
		case B_MULTI_LOCK_INTERNAL :
			wNewInputClock = CLK_CLOCKININTERNAL;
			break;
		case B_MULTI_LOCK_SPDIF :
			wNewInputClock = CLK_CLOCKINSPDIF;
			break;
		case B_MULTI_LOCK_WORDCLOCK :
			wNewInputClock = CLK_CLOCKINWORD;
			break;
		case B_MULTI_LOCK_SUPERCLOCK :
			wNewInputClock = CLK_CLOCKINSUPER;
			break;
		case B_MULTI_LOCK_MTC :
			wNewInputClock = CLK_CLOCKINMIDI;
			break;
		default :
			dprintf("DarlaGinaLayla: Lock source 0x%x not recognized\n",
					pMCE->lock_source);
			return B_BAD_VALUE;
	}
	
	// If the input clock has changed, send it to the DSP
	if (wNewInputClock != pMI->wInputClock)
	{
		CT->SetInputClock(pMI,wNewInputClock);
	}

	return B_OK;
	
} // end of multiSetEnabledChannels

/*------------------------------------------------------------------------

	multiGetGlobalFormat
	
------------------------------------------------------------------------*/

status_t multiGetGlobalFormat(PMONKEYINFO pMI,multi_format_info * pMFI)
{
	uint	rate;
	
	DPF2(("DGL: multiGetGlobalFormat\n"));

	if (NULL == pMFI)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_GET_GLOBAL_FORMAT\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_format_info) != pMFI->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_GET_GLOBAL_FORMAT\n");
		return B_BAD_VALUE;
	}	

	// 32 samples for the DSP FIFO, plus 25 samples for the DACs
	pMFI->output_latency = ((32 + 25) * 1000000) / pMI->dwSampleRate;
	DPF2(("\toutput_latency is %d\n",pMFI->output_latency));
	
	// 32 samples for the DSP FIFO, plus 32 samples for the ADCs
	pMFI->input_latency = ((32 + 32) * 1000000) / pMI->dwSampleRate;
	DPF2(("\tinput_latency is %d\n",pMFI->input_latency));	

	pMFI->timecode_kind = 0;
	
	switch (pMI->dwSampleRate)
	{
		case 8000 : 
			rate = B_SR_8000;
			break;			
		case 11025 : 
			rate = B_SR_11025;
			break;			
		case 12000 : 
			rate = B_SR_12000;
			break;
		case 16000 : 
			rate = B_SR_16000;
			break;
		case 22050 : 
			rate = B_SR_22050 ;
			break;
		case 24000 : 
			rate = B_SR_24000 ;
			break;
		case 32000 : 
			rate = B_SR_32000;
			break;
		case 44100 : 
			rate = B_SR_44100;
			break;
		case 48000 : 
			rate = B_SR_48000;
			break;
		case 88200 :
			rate = B_SR_88200;
			break;
		case 96000 :
			rate = B_SR_96000;
			break;
		default :
			rate = 0;
			break;
	}
	pMFI->output.rate = rate;
	pMFI->input.rate = pMFI->output.rate;
	DPF2(("\toutput.rate is 0x%x\n",pMFI->output.rate));
	
	pMFI->output.cvsr = pMI->dwSampleRate;
	pMFI->input.cvsr = pMFI->output.cvsr;
	DPF2(("\tsetting output.cvsr to %d\n",pMI->dwSampleRate));
	
	pMFI->output.format = B_FMT_24BIT;
	pMFI->input.format = pMFI->output.format;
		
	return B_OK;

} // end of multiGetGlobalFormat

/*------------------------------------------------------------------------

	multiSetGlobalFormat
	
------------------------------------------------------------------------*/

status_t multiSetGlobalFormat(PMONKEYINFO pMI,multi_format_info * pMFI)
{
	DWORD	dwRate;
	bool	rate_ok;
	
	DPF2(("DGL: multiSetGlobalFormat\n"));
	
	if (NULL == pMFI)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_SET_GLOBAL_FORMAT\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_format_info) != pMFI->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_SET_GLOBAL_FORMAT\n");
		return B_BAD_VALUE;
	}	

#ifdef DEBUG
	if ((0 == pMFI->output.cvsr) &&
	    (0 == pMFI->output.rate))
	{
		DPF1(("DGL: multiSetGlobalFormat got output.cvsr and output.rate both zero\n"));
		return B_BAD_VALUE;
	}
	if ((0 == pMFI->input.cvsr) &&
	    (0 == pMFI->input.rate))
	{
		DPF1(("DGL: multiSetGlobalFormat got input.cvsr and input.rate both zero\n"));
		return B_BAD_VALUE;
	}

	if (B_FMT_24BIT != pMFI->output.format)
	{
		DPF1(("DGL: multiSetGlobalFormat got bad format value for output: 0x%x\n",pMFI->output.format));
		return B_BAD_VALUE;
	}
	if (B_FMT_24BIT != pMFI->input.format)
	{
		DPF1(("DGL: multiSetGlobalFormat got bad format value for input: 0x%x\n",pMFI->input.format));
		return B_BAD_VALUE;
	}

	if (pMFI->output.cvsr != pMFI->input.cvsr)
	{
		DPF1(("DGL: multiSetGlobalFormat cvsr does not match for input & output\n"));
		return B_BAD_VALUE;
	}
	if (pMFI->output.rate != pMFI->input.rate)
	{
		DPF1(("DGL: multiSetGlobalFormat rate does not match for input & output\n"));
		return B_BAD_VALUE;
	}

#endif

	// 
	// Ignore everything but the sample rate for now; later, add
	// timecode sync?  Fixme?
	// What if both the rate and the cvsr fields are set?
	//

	// If the card supports cvsr and the cvsr is non-zero, use that. 
	// Otherwise use the rate setting.
	if ( (0 != (CT->dwSampleRates & B_SR_CVSR)) &&
		  (0.0 != pMFI->output.cvsr) )
	{
		// Round the float value
		dwRate = pMFI->output.cvsr + 0.5;
		
		DPF2(("DGL: \tUsing CVSR setting: %u\n",dwRate));
	}
	else
	{
		DPF2(("DGL: \tUsing output.rate of 0x%x\n",pMFI->output.rate));
		
		// Find what rate matches the bit setting
		switch (pMFI->output.rate)
		{
			case B_SR_8000 :
				dwRate = 8000 ;
				break;
			case B_SR_11025 :
				dwRate = 11025;
				break;
			case B_SR_12000 :
				dwRate = 12000;
				break;
			case B_SR_16000 :
				dwRate = 16000;
				break;
			case B_SR_22050 :
				dwRate = 22050;
				break;
			case B_SR_24000 :
				dwRate = 24000 ;
				break;
			case B_SR_32000 :
				dwRate = 32000;
				break;
			case B_SR_44100 :
				dwRate = 44100;
				break;
			case B_SR_48000 :
				dwRate = 48000;
				break;
			case B_SR_88200 :
				dwRate = 88200;
				break;
			case B_SR_96000 :
				dwRate = 96000;
				break;
			default :
				dwRate = 0;
				break;		
		}
	
	}
	DPF2(("DGL: \tdwRate is %d\n",dwRate));
	
	// Validate the sample rate setting
	rate_ok = CT->CheckSampleRate(dwRate);
	if (false == rate_ok)
	{
		DPF1(("DGL: multiSetGlobalFormat got bad rate for %s: rate is 0x%x, cvsr is %f\n",
			pMI->szCardBaseName,pMFI->output.rate,pMFI->output.cvsr));
		return B_BAD_VALUE;
	}

	// Set the actual rate	
	CT->SetSampleRate(pMI,dwRate);
	
	return B_OK;

} // end of multiSetGlobalFormat

/*------------------------------------------------------------------------

	multiGetBuffers
	
------------------------------------------------------------------------*/

#define MIN_SINGLE_BUFFER_SIZE_SAMPLES	64
#define MAX_SINGLE_BUFFER_SIZE_SAMPLES	512

status_t multiGetBuffers(PMONKEYINFO pMI,multi_buffer_list * pMBL)
{
	int		buffer;
	int 	channel;
	uint32	i;
	uint32	request_buffer_size;

	DPF2(("DGL: multiGetBuffers\n"));
	
	if (NULL == pMBL)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_GET_BUFFERS\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_buffer_list) != pMBL->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_GET_BUFFERS\n");
		return B_BAD_VALUE;
	}	
	
	//
	// Check the requested buffer size and create new buffers
	// if need be
	//
	
	// Take the max of the playback and record requested sizes
	request_buffer_size = pMBL->request_playback_buffer_size;
	if (request_buffer_size < pMBL->request_record_buffer_size)
		request_buffer_size = pMBL->request_record_buffer_size;
		
	// Map the requested size to a supported size
	if (0 == request_buffer_size)
	{
		request_buffer_size = DEFAULT_SINGLE_BUFFER_SIZE_SAMPLES;
	}
	else
	{
		DPF2(("\trequest_buffer_size %d\n",request_buffer_size));
	
		// Round the requested size up to the next valid size
		i = MIN_SINGLE_BUFFER_SIZE_SAMPLES;
		while (i <= MAX_SINGLE_BUFFER_SIZE_SAMPLES)
		{
			if (request_buffer_size <= i)
			{
				request_buffer_size = i;
				break;
			} 
			
			// Only allow powers of two for buffer sizes
			i <<= 1;
		}

		if (request_buffer_size > MAX_SINGLE_BUFFER_SIZE_SAMPLES)
			request_buffer_size = MAX_SINGLE_BUFFER_SIZE_SAMPLES;
	}

	// Create new buffers if necessary
	if ((request_buffer_size != pMI->dwSingleBufferSizeSamples) &&
		(0 == pMI->dwActiveChannels))
	{
		DPF2(("\tNew single buffer size %d\n",request_buffer_size));
		pMI->dwSingleBufferSizeSamples = request_buffer_size;
		destroy_audio_buffers(pMI);
		create_audio_buffers(pMI);
	}	

	// Always return info
	pMBL->flags = 0;
	
	// playback buffers
	pMBL->return_playback_buffers = 2;	// Always double-buffering
	pMBL->return_playback_channels = JC->NumOut;
	pMBL->return_playback_buffer_size = pMI->dwSingleBufferSizeSamples;

	// record buffers
	pMBL->return_record_buffers = 2; // Always double-buffering
	pMBL->return_record_channels = JC->NumIn;		
	pMBL->return_record_buffer_size = pMI->dwSingleBufferSizeSamples;
	
	// Was this just a buffer size query?
	if ((NULL == pMBL->playback_buffers) &&
		(NULL == pMBL->record_buffers))
	{
		DPF2(("DGL: multiGetBuffers query only\n"));
		return B_OK;
	}

	// Test for bad parameters
	if (pMBL->request_playback_buffers < 2 ||
	 	 pMBL->request_playback_channels < (int32) JC->NumOut ||
	 	 pMBL->request_record_buffers < 2 ||
	 	 pMBL->request_record_channels < (int32) JC->NumIn ||
	 	 NULL == pMBL->playback_buffers ||
	 	 NULL == pMBL->record_buffers)
	{
		DPF1(("DGL: multiGetBuffers bad thing happened\n"));
		DPF1(("DGL: \trequest_playback_buffers %d\n",pMBL->request_playback_buffers));
		DPF1(("DGL: \trequest_playback_channels %d\n",pMBL->request_playback_channels));		
		DPF1(("DGL: \trequest_record_buffers %d\n",pMBL->request_record_buffers));
		DPF1(("DGL: \trequest_record_channels %d\n",pMBL->request_record_channels));		
		DPF1(("DGL: \tplayback_buffers 0x%x\n",pMBL->playback_buffers));
		DPF1(("DGL: \trecord_buffers 0x%x\n",pMBL->record_buffers));
		
		return B_BAD_VALUE;
	}

	// Fill out the playback buffer addresses
	for (buffer = 0; buffer < 2; buffer++)
	{
		for (channel = 0; channel < JC->NumOut; channel++)
		{	
			pMBL->playback_buffers[buffer][channel].base = 
				(char *) pMI->pAudio[channel][buffer];
			pMBL->playback_buffers[buffer][channel].stride = sizeof(int32);
		}
	}
	
	// Fill out the record buffer addresses
	for (buffer = 0; buffer < 2; buffer++)
	{
		for (channel = 0; channel < JC->NumIn; channel++)
		{	
			pMBL->record_buffers[buffer][channel].base = 
				(char *) pMI->pAudio[channel + JC->NumOut][buffer];
			pMBL->record_buffers[buffer][channel].stride = sizeof(int32);
		}
	}

	return B_OK;

} // end of multiGetBuffers


/*------------------------------------------------------------------------

	multiBufferForceStop
	
------------------------------------------------------------------------*/

status_t multiBufferForceStop(PMONKEYINFO pMI)
{

	reset_buffer_exchange_state(pMI);
	
	return B_OK;

} // end of multiBufferForceStop


/*------------------------------------------------------------------------

	multiListExtensions
	
------------------------------------------------------------------------*/

status_t multiListExtensions(PMONKEYINFO pMI,multi_extension_list * pMEL)
{
	uint	ext;

	DPF2(("DGL: multiListExtensions\n"));
	
	if (NULL == pMEL)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_LIST_EXTENSIONS\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_extension_list) != pMEL->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_LIST_EXTENSIONS\n");
		return B_BAD_VALUE;
	}	

	if (NULL == pMEL->extensions)
	{
		dprintf("DarlaGinaLayla: null ptr (multi_extension_list->extensions) passed for"
				" B_MULTI_LIST_EXTENSIONS\n");
		return B_BAD_VALUE;
	}
	
	ext = 0;
	
	//
	// Add S/PDIF format extension if this card has an S/PDIF output
	//
	if (JC->NumDigitalOut != 0)
	{
		if (ext >= pMEL->max_count)
		{
			pMEL->actual_count = ext;
			return B_OK;
		}

		pMEL->extensions[ext].code = B_MULTI_EX_DIGITAL_FORMAT;
		pMEL->extensions[ext].flags = 0;
		strcpy(pMEL->extensions[ext].name,"S/PDIF out format");
		
		ext++;		
	}
	
	if (NULL != CT->SetOutputClock)
	{
		//
		// Add the clock generation extension
		// 
		if (ext >= pMEL->max_count)
		{
			pMEL->actual_count = ext;
			return B_OK;
		}

		pMEL->extensions[ext].code = B_MULTI_EX_CLOCK_GENERATION;
		pMEL->extensions[ext].flags = 0;
		strcpy(pMEL->extensions[ext].name,"Output clock");
		
		ext++;		
	}
		
	if (NULL != CT->SetNominalOutLevel)
	{
		//
		// Add the output nominal level select
		// 
		if (ext >= pMEL->max_count)
		{
			pMEL->actual_count = ext;
			return B_OK;
		}

		pMEL->extensions[ext].code = B_MULTI_EX_OUTPUT_NOMINAL;
		pMEL->extensions[ext].flags = 0;
		strcpy(pMEL->extensions[ext].name,"Nominal out levels");
		
		ext++;		
	}

	if (NULL != CT->SetNominalInLevel)
	{
		//
		// Add the input nominal level select
		// 
		if (ext >= pMEL->max_count)
		{
			pMEL->actual_count = ext;
			return B_OK;
		}

		pMEL->extensions[ext].code = B_MULTI_EX_INPUT_NOMINAL;
		pMEL->extensions[ext].flags = 0;
		strcpy(pMEL->extensions[ext].name,"Nominal input levels");
		
		ext++;		
	}

	pMEL->actual_count = ext;
	return B_OK;

} // end of multiListExtensions


/*------------------------------------------------------------------------

	multiGetExtension
	
------------------------------------------------------------------------*/

/* 
*/

status_t multiGetExtension(PMONKEYINFO pMI,multi_extension_cmd * pMEC)
{
	multi_ex_clock_generation 	*pMECG;
	multi_ex_digital_format 	*pMEDF;
	multi_ex_nominal_level		*pMENL;
	PCOMMPAGE					pCommPage;
	
	if (NULL == pMEC)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_GET_EXTENSION\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_extension_cmd) != pMEC->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_GET_EXTENSION\n");
		return B_BAD_VALUE;
	}	
	
	if (NULL == pMEC->out_data)
	{
		dprintf("DarlaGinaLayla: null ptr (multi_extension_cmd->out_data) passed for"
				" B_MULTI_GET_EXTENSION\n");
		return B_BAD_VALUE;
	}

	DPF2(("DGL: multiGetExtension\n"));

	pCommPage = pMI->pCommPage;	
	switch (pMEC->code)
	{

		//
		// Get the output clock
		//
		case B_MULTI_EX_CLOCK_GENERATION :
			if (NULL == CT->SetOutputClock)
			{
				dprintf("DGL: Someone tried to get B_MULTI_EX_CLOCK_GENERATION for bad card type\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->out_size < sizeof(multi_ex_clock_generation))
			{
				dprintf("DGL: out_size not big enough to fill out multi_ex_clock_generation"
						"in B_MULTI_GET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMECG = (multi_ex_clock_generation *) pMEC->out_data;
			pMECG->channel = -1;
						
			if (CLK_CLOCKOUTSUPER == pCommPage->wOutputClock)
				pMECG->clock = B_MULTI_LOCK_SUPERCLOCK;
			else
				pMECG->clock = B_MULTI_LOCK_WORDCLOCK;
				
			break;

		//
		// Get the consumer/pro switch
		//
		case B_MULTI_EX_DIGITAL_FORMAT :
			if (0 == JC->NumDigitalOut)
			{
				dprintf("DGL: Someone tried to get B_MULTI_EX_DIGITAL_FORMAT for a card without S/PDIF\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->out_size < sizeof(multi_ex_digital_format))
			{
				dprintf("DGL: out_size not big enough to fill out multi_ex_digital_format"
						"in B_MULTI_GET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMEDF = (multi_ex_digital_format *) pMEC->out_data;
			pMEDF->channel = -1;
						
			if (0 != (pCommPage->dwFlags & CPFLAG_SPDIFPRO))
				pMEDF->format = B_CHANNEL_COAX_EBU;
			else
				pMEDF->format = B_CHANNEL_COAX_SPDIF;
				
			break;
			
		//
		// Nominal output levels
		// 
		case B_MULTI_EX_OUTPUT_NOMINAL :
			if (pMEC->in_size < sizeof(multi_ex_nominal_level))
			{
				dprintf("DGL: in_size not big enough for multi_ex_nominal_level"
						"in B_MULTI_GET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMENL = (multi_ex_nominal_level *) pMEC->in_data;
			CT->GetNominalOutLevel(pMI,pMENL->channel,&(pMENL->level));
			break;

		//
		// Nominal input levels
		// 
		case B_MULTI_EX_INPUT_NOMINAL :
			if (pMEC->in_size < sizeof(multi_ex_nominal_level))
			{
				dprintf("DGL: in_size not big enough for multi_ex_nominal_level"
						"in B_MULTI_GET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMENL = (multi_ex_nominal_level *) pMEC->in_data;
			CT->GetNominalInLevel(pMI,pMENL->channel,&(pMENL->level));
			break;
			
		default:
			DPF1(("DGL: Unknown extension 0x%x requested in B_MULTI_GET_EXTENSION\n"));
			break;
	}

	return B_OK;

} // end of multiGetExtension


/*------------------------------------------------------------------------

	multiSetExtension
	
------------------------------------------------------------------------*/

status_t multiSetExtension(PMONKEYINFO pMI,multi_extension_cmd * pMEC)
{
	multi_ex_clock_generation 	*pMECG;
	multi_ex_digital_format 	*pMEDF;
	multi_ex_nominal_level		*pMENL;
	PCOMMPAGE 					pCommPage;
	uint16						wOutputClock;

	if (NULL == pMEC)
	{
		dprintf("DarlaGinaLayla: null ptr passed for"
				" B_MULTI_GET_EXTENSION\n");
		return B_BAD_VALUE;
	}
	
	if (sizeof(multi_extension_cmd) != pMEC->info_size)
	{
		dprintf("DarlaGinaLayla: structure size mismatch for"
				" B_MULTI_GET_EXTENSION\n");
		return B_BAD_VALUE;
	}	

	if (NULL == pMEC->in_data)
	{
		dprintf("DarlaGinaLayla: null ptr (multi_extension_cmd->in_data) passed for"
				" B_MULTI_SET_EXTENSION\n");
		return B_BAD_VALUE;
	}

	DPF2(("DGL: multiSetExtension\n"));
	pCommPage = pMI->pCommPage;	
	switch (pMEC->code)
	{
		//
		// Output clock
		// 
		
		case B_MULTI_EX_CLOCK_GENERATION :
			if (NULL == CT->SetOutputClock)
			{
				dprintf("DGL: Trying to set B_MULTI_EX_CLOCK_GENERATION for a bad card type\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->in_size < sizeof(multi_ex_clock_generation))
			{
				dprintf("DGL: in_size not big enough for multi_ex_clock_generation"
						"in B_MULTI_SET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMECG = (multi_ex_clock_generation *) pMEC->in_data;

			// Map from Be defines to Echo defines
			wOutputClock = 0;
			switch (pMECG->clock)
			{
				case B_MULTI_LOCK_SUPERCLOCK :
					wOutputClock = CLK_CLOCKOUTSUPER;
					break;
				case B_MULTI_LOCK_WORDCLOCK :
					wOutputClock = CLK_CLOCKOUTWORD;
					break;
				default : 
					dprintf("DGL: Bad value for clock in B_MULTI_EX_CLOCK_GENERATION\n");
					return B_BAD_VALUE;
			}				

			CT->SetOutputClock(pMI,wOutputClock);
				
			break;


		//
		// S/PDIF out consumer/professional
		// 

		case B_MULTI_EX_DIGITAL_FORMAT :
			if (0 == JC->NumDigitalOut)
			{
				dprintf("DGL: Trying to set B_MULTI_EX_DIGITAL_FORMAT for a card without S/PDIF\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->in_size < sizeof(multi_ex_digital_format))
			{
				dprintf("DGL: in_size not big enough for multi_ex_digital_format"
						"in B_MULTI_SET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMEDF = (multi_ex_digital_format *) pMEC->in_data;
			if (pMEDF->format == B_CHANNEL_COAX_EBU)
				pCommPage->dwFlags |= CPFLAG_SPDIFPRO;
			else
				pCommPage->dwFlags &= ~CPFLAG_SPDIFPRO;
				
			SendVector( pMI, UPDATE_FLAGS );				
				
			break;
			
		//
		// Nominal output levels
		// 
		case B_MULTI_EX_OUTPUT_NOMINAL :
			if (NULL == CT->SetNominalOutLevel)
			{
				dprintf("DGL: Trying to set B_MULTI_EX_OUTPUT_NOMINAL for an invalid card\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->in_size < sizeof(multi_ex_nominal_level))
			{
				dprintf("DGL: in_size not big enough for multi_ex_nominal_level"
						"in B_MULTI_SET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMENL = (multi_ex_nominal_level *) pMEC->in_data;
			
			CT->SetNominalOutLevel(pMI,pMENL->channel,pMENL->level);
			break;

		//
		// Nominal input levels
		// 
		case B_MULTI_EX_INPUT_NOMINAL :
			if (NULL == CT->SetNominalInLevel)
			{
				dprintf("DGL: Trying to set B_MULTI_EX_INPUT_NOMINAL for an invalid card\n");
				return B_BAD_VALUE;
			}
			
			if (pMEC->in_size < sizeof(multi_ex_nominal_level))
			{
				dprintf("DGL: in_size not big enough for multi_ex_nominal_level"
						"in B_MULTI_SET_EXTENSION\n");
				return B_BAD_VALUE;
			}
			
			pMENL = (multi_ex_nominal_level *) pMEC->in_data;
			
			pMENL->channel += pMI->jc->NumOut;
			//set_nominal_level(pMI,pMENL);				
			CT->SetNominalInLevel(pMI,pMENL->channel,pMENL->level);
			break;

		default:
			DPF1(("DGL: Trying to set unknown extension 0x%x\n"));
			break;
	}

	return B_OK;

} // end of multiSetExtension


/*------------------------------------------------------------------------

	echoGetAvailableSync
	
	fixme - i'd like to lose this, but how else do you detect the
	available clocks?
	
------------------------------------------------------------------------*/

status_t echoGetAvailableSync(PMONKEYINFO pMI, DWORD *pAvailable)
{
	PCOMMPAGE pCommPage;
	
	*pAvailable = B_MULTI_LOCK_INTERNAL ;
	
	pCommPage = pMI->pCommPage;
	switch (pMI->wCardType)
	{
		case DARLA :
			break;
			
		case GINA : 
			
			if ( pCommPage->dwStatusClocks & CLKFLAG_CLOCKSPDIFAVAIL )
			{
				*pAvailable |= B_MULTI_LOCK_SPDIF;
			}
			
			break;
			
		case LAYLA :
		
			if ( pCommPage->dwStatusClocks & CLKFLAG_CLOCKSPDIFAVAIL )
			{
				*pAvailable |= B_MULTI_LOCK_SPDIF;
			}
			
			if ( pCommPage->dwStatusClocks & CLKFLAG_CLOCKWORDAVAIL )
			{
			
				if ( pCommPage->dwStatusClocks & CLKFLAG_CLOCKSUPERAVAIL )		
				{
					*pAvailable |= B_MULTI_LOCK_SUPERCLOCK;
				}
				else
				{
					*pAvailable |= B_MULTI_LOCK_WORDCLOCK;
				}
			}
			
			break;
			
		case DARLA24 :
			break;	// fixme I need to add this
	}
	


	return B_OK;

} // multiSetMix

