/* includes */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

#include "sonorus.h"

/* debug */
#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf (void)
#endif

/* globals */
extern int outputs_per_bd;

/* explicit declarations for the compiler */
void	 set_channel_param(	studParmID_t, int32, int32 );
void	 set_device_param( studParmID_t, int32 );
int32	 get_device_param( studParmID_t );



static multi_mix_connection connections_1111[] =
{
	{ 32, 100, {0, 0}},
	{ 32, 200, {0, 0}},
	{100,  10, {0, 0}},
	{  0, 200, {0, 0}},
	{200, 300, {0, 0}},
	{300, 400, {0, 0}},
	{400,  20, {0, 0}},
	{400, 500, {0, 0}},
	{400, 600, {0, 0}},
	{500,  30, {0, 0}},
	{600,  31, {0, 0}},

	{ 33, 101, {0, 0}},
	{ 33, 201, {0, 0}},
	{101,  11, {0, 0}},
	{  1, 201, {0, 0}},
	{201, 301, {0, 0}},
	{301, 401, {0, 0}},
	{401,  21, {0, 0}},
	{401, 501, {0, 0}},
	{401, 601, {0, 0}},
	{501,  30, {0, 0}},
	{601,  31, {0, 0}},

	{ 34, 102, {0, 0}},
	{ 34, 202, {0, 0}},
	{102,  12, {0, 0}},
	{  2, 202, {0, 0}},
	{202, 302, {0, 0}},
	{302, 402, {0, 0}},
	{402,  22, {0, 0}},
	{402, 502, {0, 0}},
	{402, 602, {0, 0}},
	{502,  30, {0, 0}},
	{602,  31, {0, 0}},

	{ 35, 103, {0, 0}},
	{ 35, 203, {0, 0}},
	{103,  13, {0, 0}},
	{  3, 203, {0, 0}},
	{203, 303, {0, 0}},
	{303, 403, {0, 0}},
	{403,  23, {0, 0}},
	{403, 503, {0, 0}},
	{403, 603, {0, 0}},
	{503,  30, {0, 0}},
	{603,  31, {0, 0}},

	{ 36, 104, {0, 0}},
	{ 36, 204, {0, 0}},
	{104,  14, {0, 0}},
	{  4, 204, {0, 0}},
	{204, 304, {0, 0}},
	{304, 404, {0, 0}},
	{404,  24, {0, 0}},
	{404, 504, {0, 0}},
	{404, 604, {0, 0}},
	{504,  30, {0, 0}},
	{604,  31, {0, 0}},

	{ 37, 105, {0, 0}},
	{ 37, 205, {0, 0}},
	{105,  15, {0, 0}},
	{  5, 205, {0, 0}},
	{205, 305, {0, 0}},
	{305, 405, {0, 0}},
	{405,  25, {0, 0}},
	{405, 505, {0, 0}},
	{405, 605, {0, 0}},
	{505,  30, {0, 0}},
	{605,  31, {0, 0}},
	
	{ 38, 106, {0, 0}},
	{ 38, 206, {0, 0}},
	{106,  16, {0, 0}},
	{  6, 206, {0, 0}},
	{206, 306, {0, 0}},
	{306, 406, {0, 0}},
	{406,  26, {0, 0}},
	{406, 506, {0, 0}},
	{406, 606, {0, 0}},
	{506,  30, {0, 0}},
	{606,  31, {0, 0}},

	{ 39, 107, {0, 0}},
	{ 39, 207, {0, 0}},
	{107,  17, {0, 0}},
	{  7, 207, {0, 0}},
	{207, 307, {0, 0}},
	{307, 407, {0, 0}},
	{407,  27, {0, 0}},
	{407, 507, {0, 0}},
	{407, 607, {0, 0}},
	{507,  30, {0, 0}},
	{607,  31, {0, 0}},
	
	{ 40, 108, {0, 0}},
	{ 40, 208, {0, 0}},
	{108,  18, {0, 0}},
	{  8, 208, {0, 0}},
	{208, 308, {0, 0}},
	{308, 408, {0, 0}},
	{408,  28, {0, 0}},
	{408, 508, {0, 0}},
	{408, 608, {0, 0}},
	{508,  30, {0, 0}},
	{608,  31, {0, 0}},

	{ 41, 109, {0, 0}},
	{ 41, 209, {0, 0}},
	{109,  19, {0, 0}},
	{  9, 209, {0, 0}},
	{209, 309, {0, 0}},
	{309, 409, {0, 0}},
	{409,  29, {0, 0}},
	{409, 509, {0, 0}},
	{409, 609, {0, 0}},
	{509,  30, {0, 0}},
	{609,  31, {0, 0}}
};

static struct multi_mix_control controls_1111[] = {
	{ 100, B_MULTI_MIX_ENABLE,   0, {}, "Record Enable" },
	{ 101, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 102, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 103, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 104, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 105, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 106, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 107, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 108, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },
	{ 109, B_MULTI_MIX_ENABLE, 100, {}, "Record Enable" },

	{ 200, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 201, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 202, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 203, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 204, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 205, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 206, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 207, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 208, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },
	{ 209, B_MULTI_MIX_MUX,      0, {}, "Input Monitor" },

	{ 300, B_MULTI_MIX_ENABLE,   0, {}, "Output Enable" },
	{ 301, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 302, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 303, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 304, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 305, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 306, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 307, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 308, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },
	{ 309, B_MULTI_MIX_ENABLE, 300, {}, "Output Enable" },

	{ 400, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 1 Gain (Out)" },
	{ 401, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 2 Gain (Out)" },
	{ 402, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 3 Gain (Out)" },
	{ 403, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 4 Gain (Out)" },
	{ 404, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 5 Gain (Out)" },
	{ 405, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 6 Gain (Out)" },
	{ 406, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 7 Gain (Out)" },
	{ 407, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 8 Gain (Out)" },
	{ 408, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 9 Gain (Out)" },
	{ 409, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Channel 10 Gain (Out)"},

	{ 500, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 1)" },
	{ 501, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 2)" },
	{ 502, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 3)" },
	{ 503, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 4)" },
	{ 504, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 5)" },
	{ 505, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 6)" },
	{ 506, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 7)" },
	{ 507, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 8)" },
	{ 508, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 9)" },
	{ 509, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Left Monitor Gain (ch. 10)"},

	{ 600, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 1)" },
	{ 601, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 2)" },
	{ 602, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 3)" },
	{ 603, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 4)" },
	{ 604, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 5)" },
	{ 605, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 6)" },
	{ 606, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 7)" },
	{ 607, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 8)" },
	{ 608, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 9)" },
	{ 609, B_MULTI_MIX_GAIN,     0, {{-96.0, 12.0, 1.0}}, "Right Monitor Gain (ch. 10)"}
	
};

static int32 ch_0_ctls[] = { -1 };
static int32 ch_1_ctls[] = { -1 };
static int32 ch_2_ctls[] = { -1 };
static int32 ch_3_ctls[] = { -1 };
static int32 ch_4_ctls[] = { -1 };
static int32 ch_5_ctls[] = { -1 };
static int32 ch_6_ctls[] = { -1 };
static int32 ch_7_ctls[] = { -1 };
static int32 ch_8_ctls[] = { -1 };
static int32 ch_9_ctls[] = { -1 };
static int32 ch_10_ctls[] ={ 100, -1};
static int32 ch_11_ctls[] ={ 101, -1};
static int32 ch_12_ctls[] ={ 102, -1};
static int32 ch_13_ctls[] ={ 103, -1};
static int32 ch_14_ctls[] ={ 104, -1};
static int32 ch_15_ctls[] ={ 105, -1};
static int32 ch_16_ctls[] ={ 106, -1};
static int32 ch_17_ctls[] ={ 107, -1};
static int32 ch_18_ctls[] ={ 108, -1};
static int32 ch_19_ctls[] ={ 109, -1};
static int32 ch_20_ctls[] ={ 200, 300, 400, -1};
static int32 ch_21_ctls[] ={ 201, 301, 401, -1};
static int32 ch_22_ctls[] ={ 202, 302, 402, -1};
static int32 ch_23_ctls[] ={ 203, 303, 403, -1};
static int32 ch_24_ctls[] ={ 204, 304, 404, -1};
static int32 ch_25_ctls[] ={ 205, 305, 405, -1};
static int32 ch_26_ctls[] ={ 206, 306, 406, -1};
static int32 ch_27_ctls[] ={ 207, 307, 407, -1};
static int32 ch_28_ctls[] ={ 208, 308, 408, -1};
static int32 ch_29_ctls[] ={ 209, 309, 409, -1};
static int32 ch_30_ctls[] ={ 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, -1};
static int32 ch_31_ctls[] ={ 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, -1};
static int32 ch_32_ctls[] ={ -1 };
static int32 ch_33_ctls[] ={ -1 };
static int32 ch_34_ctls[] ={ -1 };
static int32 ch_35_ctls[] ={ -1 };
static int32 ch_36_ctls[] ={ -1 };
static int32 ch_37_ctls[] ={ -1 };
static int32 ch_38_ctls[] ={ -1 };
static int32 ch_39_ctls[] ={ -1 };
static int32 ch_40_ctls[] ={ -1 };
static int32 ch_41_ctls[] ={ -1 };

#define CH(x) { x, (sizeof(ch_## x ##_ctls)-sizeof(int32))/sizeof(int32), ch_## x ##_ctls }

mix_channel_controls channel_controls_1111[] =
{
	CH( 0),
	CH( 1),
	CH( 2),
	CH( 3),
	CH( 4),
	CH( 5),
	CH( 6),
	CH( 7),
	CH( 8),
	CH( 9),
	CH(10),
	CH(11),
	CH(12),
	CH(13),
	CH(14),
	CH(15),
	CH(16),
	CH(17),
	CH(18),
	CH(19),
	CH(20),
	CH(21),
	CH(22),
	CH(23),
	CH(24),
	CH(25),
	CH(26),
	CH(27),
	CH(28),
	CH(29),
	CH(30),
	CH(31),
	CH(32),
	CH(33),
	CH(34),
	CH(35),
	CH(36),
	CH(37),
	CH(38),
	CH(39),
	CH(40),
	CH(41)
};	

//FIXME: look at the return values v. return type!!!!
static status_t list_channel_controls_1111(int32 channel, int32 max, int32 * controls)
{
	int ix, cnt = sizeof(channel_controls_1111)/sizeof(channel_controls_1111[0]);

	memset(controls, 0, sizeof(int32)*max);
	for (ix = 0; ix < cnt; ix++) {
		if (channel_controls_1111[ix].channel == channel) {
			if (channel_controls_1111[ix].control_count < max)
				max = channel_controls_1111[ix].control_count;
			memcpy(controls, channel_controls_1111[ix].control_id, max*sizeof(int32));
			return channel_controls_1111[ix].control_count;
		}
	}
	return B_MISMATCHED_VALUES;
}


status_t
sonorus_multi_list_mix_channels(
				void * cookie,
				void * data,
				size_t len)
{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mix_channel_info * pmmci = (multi_mix_channel_info *) data;

	int ix,max;
	status_t err = B_OK;

	if (pmmci->info_size < sizeof(multi_mix_channel_info)) {
		return B_BAD_VALUE;
	}	
	pmmci->info_size = sizeof(multi_mix_channel_info);

	pmmci->actual_count = 0;
	for (ix = 0; ix < pmmci->channel_count; ix++) {
		if (pmmci->controls) {
			max = list_channel_controls_1111(	pmmci->channels[ix],
												pmmci->max_count,
												pmmci->controls[ix]);
		}
		else {
			/* NULL is valid if we are looking for the number of controls */
			/* but set max_count to zero to prevent exceptions            */								
			max = list_channel_controls_1111(	pmmci->channels[ix],
												0,
												(int32 *) NULL );
		}
		if (max > pmmci->actual_count) {
			pmmci->actual_count = max;
		}	
		if (max < 0) {
			err = max;
		}	
	}
	return err;
}


static status_t get_control_1111(multi_mix_control * control)
{
	uint32 ix;

	for (ix=0; ix<sizeof(controls_1111)/sizeof(controls_1111[0]); ix++) {
		if (controls_1111[ix].id == control->id) {
			control->flags  = controls_1111[ix].flags;
			control->master = controls_1111[ix].master;
			if (control->flags & B_MULTI_MIX_GAIN) {
				control->u.gain.min_gain    = controls_1111[ix].u.gain.min_gain;
				control->u.gain.max_gain    = controls_1111[ix].u.gain.max_gain;
				control->u.gain.granularity = controls_1111[ix].u.gain.granularity;
			}
			strcpy(control->name, controls_1111[ix].name);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

status_t
sonorus_multi_list_mix_controls(
				void * cookie,
				void * data,
				size_t len)
{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mix_control_info * pmmci = (multi_mix_control_info *) data;
	int32 ix;
	status_t err = B_OK;
	
	if (pmmci->info_size < sizeof(multi_mix_control_info)) {
		return B_BAD_VALUE;
	}
	pmmci->info_size = sizeof(multi_mix_control_info);

	for (ix=0; ix<pmmci->control_count; ix++) {
		err = get_control_1111(&pmmci->controls[ix]);
		if (err < B_OK) {
			return err;
		}	
	}
	return err;
}

status_t
sonorus_multi_list_mix_connections(
				void * cookie,
				void * data,
				size_t len)
{
//	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mix_connection_info * pmmci = (multi_mix_connection_info *) data;
	int max;

	if (pmmci->info_size < sizeof(multi_mix_connection_info))
		return B_BAD_VALUE;
		
	pmmci->info_size = sizeof(multi_mix_connection_info);

	max = pmmci->max_count;
	pmmci->actual_count = sizeof(connections_1111)/sizeof(connections_1111[0]);
	if (max > pmmci->actual_count)
		max = pmmci->actual_count;
	memcpy(pmmci->connections, connections_1111, max*sizeof(connections_1111[0]));
	return B_OK;
}


static status_t
get_control_master(
				int32 ctl_id )
{
	int32 ix, cnt = sizeof(controls_1111)/sizeof(controls_1111[0]);
again:
	for (ix = 0; ix < cnt; ix++) {
		if (controls_1111[ix].id == ctl_id) {
			if ( controls_1111[ix].master ) {
				if (ctl_id == controls_1111[ix].master) {
					return ix;	//	special case to be safe
				}
				/* oh, oh. goto */
				ctl_id = controls_1111[ix].master;
				goto again;				
			}		
			else {
				return ix;
			}	 	
		}
	}
	return B_MISMATCHED_VALUES;
	
}				

static status_t
get_info(
		sonorus_dev * cd,
		multi_mix_value * pmmv )
{		
	int32 idx, channel, ramp, samples;
	int32 beg, end;
	uint32 tmp;
	bigtime_t t;
	cpu_status cp;
	
	idx = get_control_master(pmmv->id);
	if (idx < 0 ) {
		return B_MISMATCHED_VALUES;
	}

	switch (controls_1111[idx].id) {
	case 100:
		pmmv->u.enable = cd->record_enable;
		pmmv->ramp = 0;
		break;
	case 200:
	case 201:
	case 202:
	case 203:
	case 204:
	case 205:
	case 206:
	case 207:
	case 208:
	case 209:
		channel = controls_1111[idx].id - 200;
		if ( cd->channels[channel].monitor_ramp < 0 ) {
			pmmv->u.mux = 1;
			ramp = -cd->channels[channel].monitor_ramp;
		}
		else {
			pmmv->u.mux = 2;
			ramp = cd->channels[channel].monitor_ramp;
		}
		/* calculate ramp..... */
		t = system_time() - cd->channels[channel].monitor_ts;
		samples = ramp - ((t/1000000) * cd->sample_rate) ;
		if (samples > 0) {   
			pmmv->ramp = (samples / cd->sample_rate)*1000000;
		}
		else {
			pmmv->ramp = 0;
		}		
		break;
	
	case 300:
		channel = controls_1111[idx].id - 300;
		if ( cd->channels[channel].output_ramp < 0 ) {
			pmmv->u.enable = true;
			ramp = -cd->channels[channel].output_ramp;
		}
		else {
			pmmv->u.enable = false;
			ramp = cd->channels[channel].output_ramp;	
		}
		/* calculate ramp..... */
		t = system_time() - cd->channels[channel].output_ts;
		samples = ramp - ((t/1000000) * cd->sample_rate) ;
		if (samples > 0) {   
			pmmv->ramp = (samples / cd->sample_rate)*1000000;
		}
		else {
			pmmv->ramp = 0;
		}		
		break;
	
	case 400:
	case 401:
	case 402:
	case 403:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
	case 409:
		channel = pmmv->id - 400;
		tmp = cd->channels[0].current_buffer & 1;
		beg = cd->chix[tmp].begGain[channel] ;
		end = cd->chiy[tmp].endGain[channel];
		pmmv->u.gain = (float) ((float)end / (float)STUD_UNITY_GAIN);
		if (beg == end) {
			pmmv->ramp = 0;
		}
		else {
			/* TODO: race condition: could switch buffers on int */
			/* prior to the disable call */
			cp = disable_interrupts();
			samples = get_device_param(STUD_PARM_POSL);
			samples = (get_device_param(STUD_PARM_POSH)<<24)|samples;
			restore_interrupts(cp);
			samples %= cd->channel_buff_size;
			samples = cd->channel_buff_size - samples;			
			pmmv->ramp = (samples / cd->sample_rate)*1000000;
		}	
		break;
	
	case 500:
	case 501:
	case 502:
	case 503:
	case 504:
	case 505:
	case 506:
	case 507:
	case 508:
	case 509:
		channel = pmmv->id - 500;
		tmp = cd->channels[0].current_buffer & 1;
		beg = cd->chix[tmp].begPanL[channel];
		end = cd->chiy[tmp].endPanL[channel];
		pmmv->u.gain = (float) ((float) end/ (float)STUD_UNITY_GAIN);
		if (beg == end) {
			pmmv->ramp = 0;
		}
		else {
			/* TODO: race condition: could switch buffers on int */
			/* prior to the disable call */
			cp = disable_interrupts();
			samples = get_device_param(STUD_PARM_POSL);
			samples = (get_device_param(STUD_PARM_POSH)<<24)|samples;
			restore_interrupts(cp);
			samples %= cd->channel_buff_size;
			samples = cd->channel_buff_size - samples;			
			pmmv->ramp = (samples / cd->sample_rate)*1000000;
		}	
		break;
	
	case 600:
	case 601:
	case 602:
	case 603:
	case 604:
	case 605:
	case 606:
	case 607:
	case 608:
	case 609:
		channel = pmmv->id - 600;
		tmp = cd->channels[0].current_buffer & 1;
		beg = cd->chix[tmp].begPanR[channel];
		end = cd->chiy[tmp].endPanR[channel];
		pmmv->u.gain = (float) ((float)end/(float)STUD_UNITY_GAIN);
		if (beg == end) {
			pmmv->ramp = 0;
		}
		else {
			/* TODO: race condition: could switch buffers on int */
			/* prior to the disable call */
			cp = disable_interrupts();
			samples = get_device_param(STUD_PARM_POSL);
			samples = (get_device_param(STUD_PARM_POSH)<<24)|samples;
			restore_interrupts(cp);
			samples %= cd->channel_buff_size;
			samples = cd->channel_buff_size - samples;			
			pmmv->ramp = (samples / cd->sample_rate)*1000000;
		}	
	break;
	default:
		return B_MISMATCHED_VALUES;
	}
	return B_OK;	
}	

status_t
sonorus_multi_get_mix(
				void * cookie,
				void * data,
				size_t len)
{
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mix_value_info * pmmvi = (multi_mix_value_info *) data;
	int ix;
	status_t err = B_OK;

	if (pmmvi->info_size < sizeof(multi_mix_value_info))
		return B_BAD_VALUE;
		
	pmmvi->info_size = sizeof(multi_mix_value_info);
	for ( ix = 0; ix < pmmvi->item_count; ix++ ) {
		err = get_info(cd, &(pmmvi->values[ix]));
		if (err < B_OK) {
			return err;
		}	
	}
	
	return err;
}

static status_t
set_info(
		sonorus_dev * cd,
		multi_mix_value * pmmv )
{		
	int32 idx, channel;
	
	idx = get_control_master(pmmv->id);
	if (idx < 0 ) {
		return B_MISMATCHED_VALUES;
	}
//FIXME: need to handle at_frame correctly
	switch (controls_1111[idx].id) {
	case 100:
		cd->record_enable = pmmv->u.enable;
		set_device_param( STUD_PARM_RECENB, cd->record_enable);					
		break;
	case 200:
	case 201:
	case 202:
	case 203:
	case 204:
	case 205:
	case 206:
	case 207:
	case 208:
	case 209:
		channel = controls_1111[idx].id - 200;
		
		if (pmmv->ramp * cd->sample_rate > cd->channel_buff_size ) {
			cd->channels[channel].monitor_ramp = cd->channel_buff_size;
		}
		else if ( pmmv->ramp * cd->sample_rate <= 0 ) {
			cd->channels[channel].monitor_ramp = MIN_RAMP_SIZE;
		}
		else {
			cd->channels[channel].monitor_ramp = pmmv->ramp;
		}	 
		if (pmmv->u.mux == 1) {
			cd->channels[channel].monitor_ramp = -cd->channels[channel].monitor_ramp;
		}
		set_channel_param( 	STUD_PARM_INPMON,
							channel + cd->ix * outputs_per_bd,
							0x8000000L / cd->channels[channel].monitor_ramp );
		cd->channels[channel].monitor_ts = system_time();
		
		break;
	
	case 300:
		channel = controls_1111[idx].id - 300;

		if (pmmv->ramp * cd->sample_rate > cd->channel_buff_size ) {
			cd->channels[channel].output_ramp = cd->channel_buff_size;
		}
		else if ( pmmv->ramp * cd->sample_rate <= 0 ) {
			cd->channels[channel].output_ramp = MIN_RAMP_SIZE;
		}
		else {
			cd->channels[channel].output_ramp = pmmv->ramp;
		}	 
		if (pmmv->u.enable) {
			cd->channels[channel].output_ramp = -cd->channels[channel].output_ramp;
		}	
		set_channel_param( 	STUD_PARM_OUTENB,
							channel + cd->ix * outputs_per_bd,
							0x8000000L / cd->channels[channel].output_ramp );
		cd->channels[channel].output_ts = system_time();

		break;
	
	case 400:
	case 401:
	case 402:
	case 403:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
	case 409:
		channel = controls_1111[idx].id - 400;
		if (pmmv->ramp == 0) {
			cd->chix[0].begGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
		else {
//FIXME: Need to handle ramping correctly!		
			cd->chix[0].begGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endGain[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
		break;
	
	case 500:
	case 501:
	case 502:
	case 503:
	case 504:
	case 505:
	case 506:
	case 507:
	case 508:
	case 509:
		channel = controls_1111[idx].id - 500;
		if (pmmv->ramp == 0) {
			cd->chix[0].begPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
		else {
//FIXME: Need to handle ramping correctly!		
			cd->chix[0].begPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endPanL[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
		break;
	
	case 600:
	case 601:
	case 602:
	case 603:
	case 604:
	case 605:
	case 606:
	case 607:
	case 608:
	case 609:
		channel = controls_1111[idx].id - 600;
		if (pmmv->ramp == 0) {
			cd->chix[0].begPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
		else {
//FIXME: Need to handle ramping correctly!		
			cd->chix[0].begPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[0].endPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chix[1].begPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
			cd->chiy[1].endPanR[ channel ] = pmmv->u.gain * (float) STUD_UNITY_GAIN;
		}
	break;
	default:
		return B_MISMATCHED_VALUES;
	}
	return B_OK;	
}	

status_t
sonorus_multi_set_mix(
				void * cookie,
				void * data,
				size_t len)
{
	status_t err = B_OK;
	int32 ix;
	sonorus_dev * cd = ((open_sonorus *)cookie)->device;
	multi_mix_value_info * pmmvi = (multi_mix_value_info *) data;
	
	if (pmmvi->info_size < sizeof(multi_mix_value_info))
		return B_BAD_VALUE;
		
	pmmvi->info_size = sizeof(multi_mix_value_info);
	for ( ix = 0; ix < pmmvi->item_count; ix++ ) {
		err = set_info(cd, &(pmmvi->values[ix]));
		if (err < B_OK) {
			return err;
		}
		else { /* is this ...an event? */
			if ( cd->current_mask & B_MULTI_EVENT_CONTROL_CHANGED) {
				if (ix >= 100 && (ix / 100) + cd->event_count >= MAX_EVENTS){
					ddprintf("sonorus: out of event space\n");
				}
				else {
					cd->event_queue[(ix / 100) + cd->event_count].u.controls[ix % 100] = pmmvi->values[ix].id;
					cd->event_queue[(ix / 100) + cd->event_count].count = (ix % 100) + 1;
					cd->event_queue[(ix / 100) + cd->event_count].event = B_MULTI_EVENT_CONTROL_CHANGED;
					cd->event_queue[(ix / 100) + cd->event_count].info_size = sizeof(multi_get_event);
					cd->event_queue[(ix / 100) + cd->event_count].timestamp = system_time();
				}	
			}	
		}	
	}
	if ( cd->current_mask & B_MULTI_EVENT_CONTROL_CHANGED) {
		/* bump event_count */
		cd->event_count = (ix / 100) + 1 + cd->event_count;
		if (cd->event_count > MAX_EVENTS) {
			cd->event_count = MAX_EVENTS;
		}
		/* release sem */
		if (cd->event_sem >= B_OK) {
			release_sem(cd->event_sem);
		}		
	}	
	return err;
}

