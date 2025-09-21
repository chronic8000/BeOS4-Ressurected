// mixer.h

// Defines for each card supported

#define DARLA_ANALOG_OUT				8
#define DARLA_DIGITAL_OUT			0
#define DARLA_ANALOG_IN				2
#define DARLA_DIGITAL_IN				0
#define DARLA_OUT_JACKS				(DARLA_ANALOG_OUT + DARLA_DIGITAL_OUT)
#define DARLA_IN_JACKS				(DARLA_ANALOG_IN + DARLA_DIGITAL_IN)
#define DARLA_JACKS					(DARLA_OUT_JACKS + DARLA_IN_JACKS)
#define DARLA_ANALOG_IN_GAIN		0
#define DARLA_ANALOG_OUT_GAIN		DARLA_ANALOG_OUT
#define DARLA_MONITOR_GAIN			(DARLA_OUT_JACKS * DARLA_IN_JACKS)
#define DARLA_ANALOG_CONNECTOR		B_CHANNEL_RCA

// This is the only difference for the jackcount for Darla24
#define DARLA24_ANALOG_CONNECTOR	B_CHANNEL_TRS


#define GINA_ANALOG_OUT				8	
#define GINA_DIGITAL_OUT				2
#define GINA_ANALOG_IN				2
#define GINA_DIGITAL_IN				2
#define GINA_OUT_JACKS				(GINA_ANALOG_OUT + GINA_DIGITAL_OUT)
#define GINA_IN_JACKS					(GINA_ANALOG_IN + GINA_DIGITAL_IN)
#define GINA_JACKS						(GINA_OUT_JACKS + GINA_IN_JACKS)
#define GINA_ANALOG_IN_GAIN			GINA_ANALOG_IN
#define GINA_ANALOG_OUT_GAIN		GINA_ANALOG_OUT
#define GINA_MONITOR_GAIN			(GINA_OUT_JACKS * GINA_IN_JACKS)
#define GINA_ANALOG_CONNECTOR		B_CHANNEL_QUARTER_INCH_MONO


#define LAYLA_ANALOG_OUT				10
#define LAYLA_DIGITAL_OUT			2
#define LAYLA_ANALOG_IN				8
#define LAYLA_DIGITAL_IN				2
#define LAYLA_OUT_JACKS				(LAYLA_ANALOG_OUT + LAYLA_DIGITAL_OUT)
#define LAYLA_IN_JACKS				(LAYLA_ANALOG_IN + LAYLA_DIGITAL_IN)
#define LAYLA_JACKS					(LAYLA_OUT_JACKS + LAYLA_IN_JACKS)
#define LAYLA_ANALOG_IN_GAIN		LAYLA_ANALOG_IN
#define LAYLA_ANALOG_OUT_GAIN		LAYLA_ANALOG_OUT
#define LAYLA_MONITOR_GAIN			(LAYLA_OUT_JACKS * LAYLA_IN_JACKS)
#define LAYLA_ANALOG_CONNECTOR		B_CHANNEL_TRS

// Defines for control limits

#define INPUT_GAIN_MIN				-100.0
#define INPUT_GAIN_MAX				25.5
#define INPUT_GAIN_ADJUST			100.0
#define OUTPUT_GAIN_MIN				-128.0
#define OUTPUT_GAIN_MAX				6.0

// Echo node types

enum {
	ANALOG_IN_GAIN = 1,
	ANALOG_OUT_GAIN,
	MONITOR_GAIN,
	ANALOG_OUT_CHANNEL,
	DIGITAL_OUT_CHANNEL,
	IN_CHANNEL,
	OUT_BUS,
	ANALOG_IN_BUS,
	DIGITAL_IN_BUS,
	LAST_NODE_TYPE
	};
 
#define NUM_ECHO_NODE_TYPES	LAST_NODE_TYPE
#define NODE_TYPE_SHIFT		16

#define Make_ID(v1,v2) 		(v1 << NODE_TYPE_SHIFT) | v2
#define Get_ID_Type(v1) 		(v1 >> NODE_TYPE_SHIFT)
#define Get_ID_Offset(v1)	(v1 & ((1 << NODE_TYPE_SHIFT) -1))

#define Make_Analog_Out_Channel_ID(v1) 		v1
#define Make_Digital_Out_Channel_ID(v1) 	(v1 + JC->NumAnalogOut)
#define Make_Analog_In_Channel_ID(v1) 		(v1 + JC->NumOut)
#define Make_Digital_In_Channel_ID(v1) 		(v1 + JC->NumOut + JC->NumAnalogIn)
#define Make_Out_Bus_ID(v1) 					(v1 + JC->NumJacks)
#define Make_Analog_Out_Bus_ID(v1)			Make_Out_Bus_ID(v1)
#define Make_Digital_Out_Bus_ID(v1) 			(v1 + JC->NumJacks + JC->NumAnalogOut)
#define Make_Analog_In_Bus_ID(v1)				(v1 + JC->NumJacks + JC->NumOut)
#define Make_Digital_In_Bus_ID(v1)			(v1 + JC->NumJacks + JC->NumOut + JC->NumAnalogIn)

// Function ptr typedefs

typedef float (*GETVALUE)(PMONKEYINFO, int32);
typedef void (*SETVALUE)(PMONKEYINFO, int32, float);
typedef int32 (*GETCONN)(PMONKEYINFO, int32, multi_mix_connection *);
typedef void (*GETNAME)(PMONKEYINFO, int32, char *);

// Data structure for a single mixer tree node

typedef struct tagNodeType
{
	int32		be_control_flags;	// Be-defined flags for this node
	float		min_gain;
	float		max_gain;
	float		granularity;
	GETVALUE	GetValue;
	SETVALUE	SetValue;
	GETCONN	GetConnections;
	GETNAME	GetName;	
} NODETYPE;

#define Monitor_Index(out,in) ((JC->NumIn * out) + in)


