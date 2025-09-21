/*************************************************************************

 laylamixer.c
 
 Mixer node tree for Layla
 
 Copyright (c) 1999 Echo Corporation 

*************************************************************************/

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "monkey.h"
#include "commpage.h"
#include "multi_audio.h"
#include "mixer.h"

float GetAnalogInputGain(PMONKEYINFO, int32);
void SetAnalogInputGain(PMONKEYINFO, int32, float);
int32 GetAnalogInputGainConnections(PMONKEYINFO, int32, multi_mix_connection *);
void GetAnalogInputGainName( PMONKEYINFO, int32, char *);
float GetAnalogOutputGain(PMONKEYINFO, int32);
void SetAnalogOutputGain(PMONKEYINFO, int32, float);
int32 GetAnalogOutputGainConnections(PMONKEYINFO, int32, multi_mix_connection *);
void GetAnalogOutputGainName( PMONKEYINFO, int32, char *);
float GetMonitorGain(PMONKEYINFO, int32);
void SetMonitorGain(PMONKEYINFO, int32, float);
int32 GetMonitorConnections(PMONKEYINFO, int32, multi_mix_connection *);
void GetMonitorName( PMONKEYINFO, int32, char *);
int32 GetAnalogOutChannelConnections(PMONKEYINFO, int32, multi_mix_connection *);
int32 GetDigitalOutChannelConnections(PMONKEYINFO, int32, multi_mix_connection *);
int32 GetAnalogInBusConnections(PMONKEYINFO, int32, multi_mix_connection *);
int32 GetDigitalInBusConnections(PMONKEYINFO, int32, multi_mix_connection *);
int32 NoConnect(PMONKEYINFO, int32, multi_mix_connection *);
void NoName( PMONKEYINFO pMI, int32 offset, char *dest);

extern JACKCOUNT darla_jack_count;
extern JACKCOUNT gina_jack_count;
extern JACKCOUNT layla_jack_count;

// Array describing each type of mixer node

NODETYPE node_types[NUM_ECHO_NODE_TYPES] = 
{
	{							// Dummy entry since ANALOG_IN_GAIN is one
		0,0,0,0,NULL,NULL,NoConnect,NoName
	},
	
	{							// ANALOG_IN_GAIN		
		B_MULTI_MIX_GAIN,	
		INPUT_GAIN_MIN,
		INPUT_GAIN_MAX,
		0.5,
		GetAnalogInputGain,
		SetAnalogInputGain,
		GetAnalogInputGainConnections,
		GetAnalogInputGainName
	},
	
	{							// ANALOG_OUT_GAIN		
		B_MULTI_MIX_GAIN,	
		OUTPUT_GAIN_MIN,
		OUTPUT_GAIN_MAX,
		1.0,
		GetAnalogOutputGain,
		SetAnalogOutputGain,
		GetAnalogOutputGainConnections,
		GetAnalogOutputGainName
	},
	
	{							// MONITOR_GAIN
		B_MULTI_MIX_GAIN,	
		OUTPUT_GAIN_MIN,
		OUTPUT_GAIN_MAX,
		1.0,
		GetMonitorGain,
		SetMonitorGain,
		GetMonitorConnections,
		GetMonitorName
	},
	
	{							// ANALOG_OUT_CHANNEL
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		GetAnalogOutChannelConnections,
		NoName
	},
	
	{							// DIGITAL_OUT_CHANNEL
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		GetDigitalOutChannelConnections,
		NoName
	},
	
	{							// IN_CHANNEL
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		NoConnect,
		NoName
	},

	{							// OUT_BUS
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		NoConnect,
		NoName
	},
	
	{							// ANALOG_IN_BUS
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		GetAnalogInBusConnections,
		NoName
	},
	
	{							// DIGITAL_IN_BUS
		0,
		0.0,
		0.0,
		0.0,
		NULL,
		NULL,
		GetDigitalInBusConnections,
		NoName
	},
	

} ;

extern int32 darla_channel_map[];
extern int32 gina_channel_map[];
extern int32 layla_channel_map[];
extern int32 darla24_channel_map[];

int32 map_gina_offset(int32 offset)
{
	if (0 == offset)
		return 1;
		
	return 0;

} // map_gina_offset

float GetAnalogInputGain(PMONKEYINFO pMI, int32 offset)
{
	PCOMMPAGE	pCommPage;
	float		rval;
	
	pCommPage = pMI->pCommPage;
	// Analog input gains in the comm page range from 0x64 (-50 dB) to 
	// 0xfb (+25.5) dB; units are .5 dB.
	// 
	// To convert from the byte value to a float, first divide by
	// two to change units to dB instead of .5 dB.  Next, subtract the 
	// gain adjust factor so that 0 dB is at 0.0 instead of at 
	// 0xc8.
	
	if (GINA == pMI->wCardType)
	{
		// Swap analog input gains for Gina hardware only
		offset = map_gina_offset(offset);
	}

	rval = (float) pCommPage->LineLevel[offset + JC->NumOut];
	
	rval /= 2.0;				// convert to units of decibels
	rval -= INPUT_GAIN_ADJUST;	// adjust where 0 dB is
	
	return rval;
}

void SetAnalogInputGain(PMONKEYINFO pMI, int32 offset, float gain)
{
	PCOMMPAGE	pCommPage;
	
	pCommPage = pMI->pCommPage;
	
	if (gain < INPUT_GAIN_MIN)
		gain = INPUT_GAIN_MIN;
		
	if (gain > INPUT_GAIN_MAX)
		gain = INPUT_GAIN_MAX;

	gain += INPUT_GAIN_ADJUST;
	gain *= 2.0;

	if (GINA == pMI->wCardType)
	{
		// Swap analog input gains for Gina hardware only
		offset = map_gina_offset(offset);
	}

	pCommPage->LineLevel[offset + JC->NumOut] = (int8) gain;
	DPF2(("DGL: JC->NumOut is 0x%x\n",JC->NumOut));
	DPF2(("DGL: SetAnalogInputGain wrote LineLevel[%d] with 0x%x\n",offset+JC->NumOut,(uint8) (int8) gain));
}

int32 GetAnalogInputGainConnections(	PMONKEYINFO pMI, 
										int32 offset, 
										multi_mix_connection *pmmc)
{
	int32 i;
	int32 from;
	int32 monitor_index;

	// Analog input gains connect to the corresponding analog input
	// channel and one monitor gain for each output bus
	from = Make_ID(ANALOG_IN_GAIN,offset);
	pmmc->from = from;
	pmmc->to = Make_Analog_In_Channel_ID(offset);
	pmmc++;

	for (i = 0; i < JC->NumOut; i++)
	{
		pmmc->from = from;
		
		monitor_index = Monitor_Index(i,offset);
		pmmc->to = Make_ID(MONITOR_GAIN, monitor_index);
		
		pmmc++;
	}	

	return (1 + JC->NumOut);
}

void GetAnalogInputGainName( PMONKEYINFO pMI, int32 offset, char *dest)
{
	sprintf(dest,"Input Gain %ld",offset);
}


float GetAnalogOutputGain(PMONKEYINFO pMI, int32 offset)
{
	PCOMMPAGE	pCommPage;
	
	pCommPage = pMI->pCommPage;

	// Analog output gains in the comm page range from 0x06 (+6 dB) to 
	// 0x80 (-128) dB; units are dB.
	return (float) (int8) pCommPage->LineLevel[offset];
}


void SetAnalogOutputGain(PMONKEYINFO pMI, int32 offset, float gain)
{
	PCOMMPAGE	pCommPage;
	
	pCommPage = pMI->pCommPage;
	
	if (gain < OUTPUT_GAIN_MIN)
		gain = OUTPUT_GAIN_MIN;
		
	if (gain > OUTPUT_GAIN_MAX)
		gain = OUTPUT_GAIN_MAX;

	pCommPage->LineLevel[offset] = (int8) gain;

	DPF2(("DGL: SetAnalogOutputGain wrote LineLevel[%d] with 0x%x\n",offset,(uint8) (int8) gain ));
}

int32 GetAnalogOutputGainConnections(	PMONKEYINFO pMI, 
										int32 offset, 
										multi_mix_connection *pmmc)
{
	// Analog output gains connect directly to the corresponding
	// analog output bus
	//
	// Analog output bus number comes after all the output and
	// input channels
	pmmc->from = Make_ID(ANALOG_OUT_GAIN,offset);
	pmmc->to = Make_Analog_Out_Bus_ID(offset);
	
	return 1;
}

void GetAnalogOutputGainName( PMONKEYINFO pMI, int32 offset, char *dest)
{
	sprintf(dest,"Output Gain %ld",offset);
}


float GetMonitorGain(PMONKEYINFO pMI, int32 offset)
{
	PCOMMPAGE	pCommPage;
	
	pCommPage = pMI->pCommPage;

	// Monitor gains in the comm page range from 0x06 (+6 dB) to 
	// 0x80 (-128) dB; units are dB.
	return (float) (int8) pCommPage->Monitors[offset];

}

void SetMonitorGain(PMONKEYINFO pMI, int32 offset, float gain)
{
	PCOMMPAGE	pCommPage;
	
	pCommPage = pMI->pCommPage;
	
	if (gain < OUTPUT_GAIN_MIN)
		gain = OUTPUT_GAIN_MIN;
		
	if (gain > OUTPUT_GAIN_MAX)
		gain = OUTPUT_GAIN_MAX;

	pCommPage->Monitors[offset] = (int8) gain;

	DPF2(("DGL: Wrote Monitors[%d] with 0x%x\n",offset,(uint8) (int8) gain));
}


int32 GetMonitorConnections(	PMONKEYINFO pMI, 
								int32 offset, 
								multi_mix_connection *pmmc)
{
	pmmc->from = Make_ID(MONITOR_GAIN,offset);

	// Monitor gains connect to the corresponding
	// analog output bus
	//
	// Analog output bus number comes after all the output and
	// input channels

	offset /= JC->NumIn; // Get output bus number

	pmmc->to = Make_Out_Bus_ID(offset);
	
	return 1;

}


void GetMonitorName( PMONKEYINFO pMI, int32 offset, char *dest)
{
	int32	input;
	int32	output;
	char	in_str[16],out_str[16];
	
	output = offset / JC->NumIn;
	input = offset % JC->NumIn;
	
	if (output < JC->NumAnalogOut)
	{
		sprintf(out_str,"Out %ld",output + 1);
	}
	else
	{
		output -= JC->NumAnalogOut;
		if (output & 1)
		{
			strcpy(out_str,"S/PDIF Out R");
		}
		else
		{
			strcpy(out_str,"S/PDIF Out L");
		}
			
	}
	
	if (input < JC->NumAnalogIn)
	{
		sprintf(in_str,"In %ld",input + 1);
	}
	else
	{
		input -= JC->NumAnalogIn;
		if (input & 1)
		{
			strcpy(in_str,"S/PDIF In R");
		}
		else
		{
			strcpy(in_str,"S/PDIF In L");
		}
	
	}

	sprintf(dest,"Mon %s->%s",in_str,out_str);
}


int32 GetAnalogOutChannelConnections(	PMONKEYINFO pMI, 
										int32 offset, 
										multi_mix_connection *pmmc)
{
	pmmc->from = Make_Analog_Out_Channel_ID(offset);

	// Analog output channels connect to the corresponding
	// analog output gain control
	pmmc->to = Make_ID(ANALOG_OUT_GAIN,offset);

	return 1;
}

int32 GetDigitalOutChannelConnections(	PMONKEYINFO pMI, 
										int32 offset, 
										multi_mix_connection *pmmc)
{
	pmmc->from = Make_Digital_Out_Channel_ID(offset);

	// Analog output channels connect to the corresponding
	// output bus
	pmmc->to = Make_Digital_Out_Bus_ID(offset);

	return 1;

}


int32 GetAnalogInBusConnections(	PMONKEYINFO pMI, 
								int32 offset, 
								multi_mix_connection *pmmc)
{
	pmmc->from = Make_Analog_In_Bus_ID(offset);

	// Analog input busses connect to analog input gains
	pmmc->to = Make_ID(ANALOG_IN_GAIN,offset);

	return 1;
}

int32 GetDigitalInBusConnections(	PMONKEYINFO pMI, 
									int32 offset, 
									multi_mix_connection *pmmc)
{
	int32 from;
	int32 i;
	int32 monitor_index;
	
	from = Make_Digital_In_Bus_ID(offset);
	pmmc->from = from;

	// Digital input busses connect to digital input channels
	// and one monitor gain for each output 
	pmmc->to = Make_Digital_In_Channel_ID(offset);
	pmmc++;

	offset += JC->NumAnalogIn;
	for (i = 0; i < JC->NumOut; i++)
	{
		pmmc->from = from;
		
		monitor_index = Monitor_Index(i,offset);
		pmmc->to = Make_ID(MONITOR_GAIN, monitor_index);
		
		pmmc++;
	}	

	return (1 + JC->NumOut);
}


int32 NoConnect( PMONKEYINFO pMI, int32 offset, multi_mix_connection *pmmc)
{
	return 0;
}

void NoName( PMONKEYINFO pMI, int32 offset, char *dest)
{
	*dest = 0;
}

/*------------------------------------------------------------------------

	mixerInit
	
------------------------------------------------------------------------*/

void init_channel_map(int32 *map, JACKCOUNT *jc)
{
	int32	i,channel;
	
	channel = 0;
	
	// Analog output channels
	for (i = 0; i < jc->NumAnalogOut; i++)
	{
		map[channel++] = Make_ID(ANALOG_OUT_CHANNEL,i);
	}
	
	// Digital output channels
	for (i = 0; i < jc->NumDigitalOut; i++)
	{
		map[channel++] = Make_ID(DIGITAL_OUT_CHANNEL,i);
	}
	
	// Input channels
	for (i = 0; i < jc->NumIn; i++)
	{
		map[channel++] = Make_ID(IN_CHANNEL,i);
	}
	
	// Output busses
	for (i = 0; i < jc->NumOut; i++)
	{
		map[channel++] = Make_ID(OUT_BUS,i);
	}
	
	// Analog input busses
	for (i = 0; i < jc->NumAnalogIn; i++)
	{
		map[channel++] = Make_ID(ANALOG_IN_BUS,i);
	}
	
	// Digital input busses
	for (i = 0; i < jc->NumAnalogOut; i++)
	{
		map[channel++] = Make_ID(DIGITAL_IN_BUS,i);
	}

} // init_channel_map


void mixerInit()
{

	//
	// Create the channel maps for each card type
	//
	init_channel_map(darla_channel_map,&darla_jack_count);
	init_channel_map(gina_channel_map,&gina_jack_count);	
	init_channel_map(layla_channel_map,&layla_jack_count);
	
	DPF2(("DGL: mixerInit done\n"));	

} // mixerInit


/*------------------------------------------------------------------------

	multiListMixConnections
	
------------------------------------------------------------------------*/

status_t multiListMixConnections(PMONKEYINFO pMI,multi_mix_connection_info *pmmci)
{
	int32 conn_index,i,j,k,num_temp_conn,total_conn;
	multi_mix_connection temp_connections[32];
	
	DPF2(("multiListMixConnections"));
	
	if (NULL == pmmci)
	{
		dprintf("DarlaGinaLayla: null parameter for B_MULTI_LIST_MIX_CONNECTIONS\n");
		return B_BAD_VALUE;
	}

	// Check the structure size
	if (pmmci->info_size < sizeof(multi_mix_connection_info))
	{
		dprintf("Structure size mismatch on B_MULTI_LIST_MIX_CONNECTIONS for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
		
	// 	Go thorough each node type and get the connections
	conn_index = 0;
	total_conn = 0;
	for (i = 1; i < NUM_ECHO_NODE_TYPES; i++)	// node types start at 1
	{
		if (NULL == node_types[i].GetConnections)
			continue;
			
		for (j = 0; j < pMI->node_type_counts[i]; j++)
		{
			num_temp_conn = node_types[i].GetConnections(pMI,j,temp_connections);
			for (k = 0; k < num_temp_conn; k++)
			{
			 	// Copy the connections if max_count has not been exceeded
			 	if (conn_index < pmmci->max_count)
			 	{
			 		memcpy(	(pmmci->connections) + conn_index,
			 				temp_connections + k, 
			 				sizeof(multi_mix_connection));
			 		conn_index++;
			 	}	
			
			}	// for k
			
			total_conn += num_temp_conn;
		} // for j
	
	} // for i 
	
	pmmci->actual_count = total_conn;
	
	return B_OK;

} // multiListMixConnections 


/*------------------------------------------------------------------------

	multiListMixChannels
	
------------------------------------------------------------------------*/

status_t multiListMixChannels(PMONKEYINFO pMI,multi_mix_channel_info *pmmci)
{
	int32 channel_idx,j,id,type,offset,channel;
	int32 max_actual_count,curr_control;
	multi_mix_connection temp_connections[32];
	int32 num_temp_conn,conn_type;

	DPF2(("multiListMixChannels"));

	if (NULL == pmmci)
	{
		dprintf("DarlaGinaLayla: null parameter for B_MULTI_LIST_MIX_CHANNELS\n");
		return B_BAD_VALUE;
	}

	// Check the structure size
	if (pmmci->info_size < sizeof(multi_mix_connection_info))
	{
		dprintf("Structure size mismatch on B_MULTI_LIST_MIX_CONNECTIONS for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Check the channels parameter of pmmci
	if ((NULL == pmmci->channels) &&
		(0 != pmmci->channel_count))
	{
		dprintf("DarlaGinaLayla: channels memboer of multi_mix_channel info for"
				"B_MULTI_LIST_MIX_CONNECTIONS is null\n");
		return B_BAD_VALUE;
	}

	// Check the controls parameter of pmmci
	if ((NULL == pmmci->controls) &&
		(0 != pmmci->max_count))
	{ 
		dprintf("DarlaGinaLayla: controls memboer of multi_mix_channel info for"
				"B_MULTI_LIST_MIX_CONNECTIONS is null\n");
		return B_BAD_VALUE;
	}

	// Loop over all requested busses and channels
	max_actual_count = 0;
	for (channel_idx = 0; channel_idx < pmmci->channel_count; channel_idx++)
	{
		channel = pmmci->channels[channel_idx];
	
		id = pMI->channel_map[channel];
		type = Get_ID_Type(id);
		offset = Get_ID_Offset(id);

		// List the controls that this node type is attached to 
		num_temp_conn = node_types[type].GetConnections(pMI,offset,temp_connections);
		
		// Go through each connection and put it in the client list if it is a control
		curr_control = 0;
		for (j = 0; j < num_temp_conn; j++)
		{	
			conn_type = Get_ID_Type(temp_connections[j].to);
			if (0 != node_types[conn_type].be_control_flags)			
			{
				if (curr_control < pmmci->max_count)
				{
					pmmci->controls[channel_idx][curr_control] = 
						temp_connections[j].to;
				}	
				
				curr_control++;
			}
			
		} // j for all returned connections	

		if (curr_control > max_actual_count)
		{
			max_actual_count = curr_control;
		}

		while (curr_control < pmmci->max_count)
		{
			pmmci->controls[channel_idx][curr_control++] = 0;				
		}
	
	}	// channel_idx for all requested channels
	
	pmmci->actual_count = max_actual_count;
	
	return B_OK;
	
} // multiLisMixChannels

/*------------------------------------------------------------------------

	multiGetMix
	
------------------------------------------------------------------------*/

status_t multiGetMix(PMONKEYINFO pMI, multi_mix_value_info *pmmvi)
{
	int32 	type,offset;
	int32	values_idx;

	// Validate pmmvi
	if (NULL == pmmvi)
	{
		dprintf("Null pointer to multi_mix_value_info passed for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Check the structure size
	if (pmmvi->info_size < sizeof(multi_mix_value_info))
	{
		dprintf("Structure size mismatch on B_MULTI_GET_MIX for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Validate the values pointer
	if ((NULL == pmmvi->values) &&
		(0 != pmmvi->item_count))
	{
		dprintf("values member of multi_mix_value_info is NULL for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}

	//
	// Fill out the struct
	//
	for (values_idx = 0 ; values_idx < pmmvi->item_count; values_idx++)
	{
		type = Get_ID_Type(pmmvi->values[values_idx].id);
		offset = Get_ID_Offset(pmmvi->values[values_idx].id);
	
		// Validate the requested control ID
		if ( (type >= NUM_ECHO_NODE_TYPES) ||
			 (offset >= pMI->node_type_counts[type]))
		{
			dprintf("Bad control ID 0x%x requested on B_MULTI_GET_MIX for DarlaGinaLayla\n",
					pmmvi->values[values_idx].id);
			return B_BAD_VALUE;
		}
		
		pmmvi->values[values_idx].u.gain = node_types[type].GetValue(pMI,offset);
	
	}
	
	return B_OK;
	
} // multiGetMix


/*------------------------------------------------------------------------

	multiListMixControls
	
------------------------------------------------------------------------*/

status_t multiListMixControls(PMONKEYINFO pMI, multi_mix_control_info *pmmci)
{
	int32 control_idx;
	int32 type,offset;
	
	// Validate pmmci
	if (NULL == pmmci)
	{
		dprintf("Null pointer to multi_mix_control_info passed for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Check the structure size
	if (pmmci->info_size < sizeof(multi_mix_control_info))
	{
		dprintf("Structure size mismatch on B_MULTI_LIST_MIX_CONTROLS for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Validate the controls pointer
	if ((NULL == pmmci->controls) &&
		(0 != pmmci->control_count))
	{
		dprintf("controls member of multi_mix_control_info is NULL for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}

	//
	// Fill out the struct
	//	
	for (control_idx = 0 ; control_idx < pmmci->control_count; control_idx++)
	{
		type = Get_ID_Type(pmmci->controls[control_idx].id);
		offset = Get_ID_Offset(pmmci->controls[control_idx].id);
	
		// Validate the requested control ID
		// fixme make this a subroutine
		if ( (type >= NUM_ECHO_NODE_TYPES) ||
			 (offset >= pMI->node_type_counts[type]))
		{
			dprintf("Bad control ID 0x%x requested on B_MULTI_LIST_MIX_CONTROLS for DarlaGinaLayla\n",
					pmmci->controls[control_idx].id);
			return B_BAD_VALUE;
		}
	
		pmmci->controls[control_idx].flags = node_types[type].be_control_flags;
		pmmci->controls[control_idx].master = 0;
		pmmci->controls[control_idx].u.gain.min_gain = node_types[type].min_gain;
		pmmci->controls[control_idx].u.gain.max_gain = node_types[type].max_gain;
		pmmci->controls[control_idx].u.gain.granularity = node_types[type].granularity;
		
		node_types[type].GetName(pMI, offset, pmmci->controls[control_idx].name);
	}	
	
	return B_OK;

} // multiListMixControls

/*------------------------------------------------------------------------

	multiSetMix
	
------------------------------------------------------------------------*/

status_t multiSetMix(PMONKEYINFO pMI, multi_mix_value_info *pmmvi)
{
	bool	output_gains_changed,input_gains_changed;
	int32	value_idx,type,offset;

	output_gains_changed = false;
	input_gains_changed	= false;

	// Validate pmmvi
	if (NULL == pmmvi)
	{
		dprintf("Null argument for MULTI_SET_MIX passed for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Check the structure size
	if (pmmvi->info_size < sizeof(multi_mix_value_info))
	{
		dprintf("Structure size mismatch on B_MULTI_SET_MIX for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	// Validate values pointer
	if ((NULL == pmmvi->values) &&
		(0 != pmmvi->item_count))
	{
		dprintf("values pointer is NULL for MULTI_SET_MIX for DarlaGinaLayla\n");
		return B_BAD_VALUE;
	}
	
	//
	// Walk through each control change
	//
	for (value_idx = 0; value_idx < pmmvi->item_count; value_idx++)
	{
		type = Get_ID_Type(pmmvi->values[value_idx].id);
		offset = Get_ID_Offset(pmmvi->values[value_idx].id);
	
		// Validate the requested control ID
		// fixme make this a subroutine
		if ( (type >= NUM_ECHO_NODE_TYPES) ||
			 (offset >= pMI->node_type_counts[type]))
		{
			dprintf("Bad control ID 0x%x on B_SET_MIX for DarlaGinaLayla\n",
					pmmvi->values[value_idx].id);
			return B_BAD_VALUE;
		}
		
		node_types[type].SetValue(pMI,offset,pmmvi->values[value_idx].u.gain);
		
		if (ANALOG_IN_GAIN == type)
			input_gains_changed = true;
			
		if ((ANALOG_OUT_GAIN == type) ||
			(MONITOR_GAIN == type))
			output_gains_changed = true;
	}	
	
	//
	// Tell the DSP to make the necessary changes
	//
	if (input_gains_changed)
		SendVector(pMI,UPDATE_INGAIN);
		
	if (output_gains_changed)
		SendVector(pMI,UPDATE_OUTGAIN);

	return B_OK;

} // multiSetMix
