/* Copyright 2001, Be Incorporated. All Rights Reserved */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

#include "envy24.h"

extern ac97_module_info_v3 *ac97;

typedef struct mix_channel_controls mix_channel_controls;
struct mix_channel_controls {
	int32 channel;
	int32 control_count;
	int32 *control_id;
};	


void set_gain(envy24_dev * card, uint16 channel);
void set_mux(envy24_dev * card, uint16 ctl_idx);
void set_enable(envy24_dev * card, uint16 enable);
void initialize_cached_controls( envy24_dev * );

//To save space, we should generate the table data on the fly and only as needed...

static multi_mix_connection connections_envy24[] =
{

	/* direct connects */
	{ 34,  10, {0, 0}},
	{ 35,  11, {0, 0}},
	{ 36,  12, {0, 0}},
	{ 37,  13, {0, 0}},
	{ 38,  14, {0, 0}},
	{ 39,  15, {0, 0}},
	{ 40,  16, {0, 0}},
	{ 41,  17, {0, 0}},
	{ 42,  18, {0, 0}},
	{ 43,  19, {0, 0}},
	{ 44,  20, {0, 0}},
	{ 45,  21, {0, 0}},

	/* to nodes */
	{  0, 300, {0, 0}},

	{ 34, 300, {0, 0}},
	{ 35, 300, {0, 0}},
	{ 36, 300, {0, 0}},
	{ 37, 300, {0, 0}},
	{ 38, 300, {0, 0}},
	{ 39, 300, {0, 0}},
	{ 40, 300, {0, 0}},
	{ 41, 300, {0, 0}},
	{ 42, 300, {0, 0}},
	{ 43, 300, {0, 0}},

	{ 44, 300, {0, 0}},

	{  1, 301, {0, 0}},

	{ 34, 301, {0, 0}},
	{ 35, 301, {0, 0}},
	{ 36, 301, {0, 0}},
	{ 37, 301, {0, 0}},
	{ 38, 301, {0, 0}},
	{ 39, 301, {0, 0}},
	{ 40, 301, {0, 0}},
	{ 41, 301, {0, 0}},
	{ 42, 301, {0, 0}},
	{ 43, 301, {0, 0}},

	{ 45, 301, {0, 0}},

	{  2, 302, {0, 0}},

	{ 34, 302, {0, 0}},
	{ 35, 302, {0, 0}},
	{ 36, 302, {0, 0}},
	{ 37, 302, {0, 0}},
	{ 38, 302, {0, 0}},
	{ 39, 302, {0, 0}},
	{ 40, 302, {0, 0}},
	{ 41, 302, {0, 0}},
	{ 42, 302, {0, 0}},
	{ 43, 302, {0, 0}},

	{  3, 303, {0, 0}},

	{ 34, 303, {0, 0}},
	{ 35, 303, {0, 0}},
	{ 36, 303, {0, 0}},
	{ 37, 303, {0, 0}},
	{ 38, 303, {0, 0}},
	{ 39, 303, {0, 0}},
	{ 40, 303, {0, 0}},
	{ 41, 303, {0, 0}},
	{ 42, 303, {0, 0}},
	{ 43, 303, {0, 0}},

	{  4, 304, {0, 0}},

	{ 34, 304, {0, 0}},
	{ 35, 304, {0, 0}},
	{ 36, 304, {0, 0}},
	{ 37, 304, {0, 0}},
	{ 38, 304, {0, 0}},
	{ 39, 304, {0, 0}},
	{ 40, 304, {0, 0}},
	{ 41, 304, {0, 0}},
	{ 42, 304, {0, 0}},
	{ 43, 304, {0, 0}},

	{  5, 305, {0, 0}},

	{ 34, 305, {0, 0}},
	{ 35, 305, {0, 0}},
	{ 36, 305, {0, 0}},
	{ 37, 305, {0, 0}},
	{ 38, 305, {0, 0}},
	{ 39, 305, {0, 0}},
	{ 40, 305, {0, 0}},
	{ 41, 305, {0, 0}},
	{ 42, 305, {0, 0}},
	{ 43, 305, {0, 0}},

	{  6, 306, {0, 0}},

	{ 34, 306, {0, 0}},
	{ 35, 306, {0, 0}},
	{ 36, 306, {0, 0}},
	{ 37, 306, {0, 0}},
	{ 38, 306, {0, 0}},
	{ 39, 306, {0, 0}},
	{ 40, 306, {0, 0}},
	{ 41, 306, {0, 0}},
	{ 42, 306, {0, 0}},
	{ 43, 306, {0, 0}},

	{  7, 307, {0, 0}},

	{ 34, 307, {0, 0}},
	{ 35, 307, {0, 0}},
	{ 36, 307, {0, 0}},
	{ 37, 307, {0, 0}},
	{ 38, 307, {0, 0}},
	{ 39, 307, {0, 0}},
	{ 40, 307, {0, 0}},
	{ 41, 307, {0, 0}},
	{ 42, 307, {0, 0}},
	{ 43, 307, {0, 0}},

	{  8, 308, {0, 0}},

	{ 34, 308, {0, 0}},
	{ 35, 308, {0, 0}},
	{ 36, 308, {0, 0}},
	{ 37, 308, {0, 0}},
	{ 38, 308, {0, 0}},
	{ 39, 308, {0, 0}},
	{ 40, 308, {0, 0}},
	{ 41, 308, {0, 0}},
	{ 42, 308, {0, 0}},
	{ 43, 308, {0, 0}},

	{ 44, 308, {0, 0}},

	{  9, 309, {0, 0}},

	{ 34, 309, {0, 0}},
	{ 35, 309, {0, 0}},
	{ 36, 309, {0, 0}},
	{ 37, 309, {0, 0}},
	{ 38, 309, {0, 0}},
	{ 39, 309, {0, 0}},
	{ 40, 309, {0, 0}},
	{ 41, 309, {0, 0}},
	{ 42, 309, {0, 0}},
	{ 43, 309, {0, 0}},

	{ 45, 309, {0, 0}},

	{ 44, 400, {0, 0}},
	{ 45, 401, {0, 0}},

	/* from nodes */
	{300,  22, {0, 0}},
	{301,  23, {0, 0}},
	{302,  24, {0, 0}},
	{303,  25, {0, 0}},
	{304,  26, {0, 0}},
	{305,  27, {0, 0}},
	{306,  28, {0, 0}},
	{307,  29, {0, 0}},
	{308,  30, {0, 0}},
	{309,  31, {0, 0}},

	{400,  500, {0, 0}},
	{401,  501, {0, 0}},
	
	{500,  32, {0, 0}},
	{501,  33, {0, 0}},

	/* channel to gain */
	{  0, 100, {0, 0}}, 
	{  1, 101, {0, 0}}, 
	{  2, 102, {0, 0}}, 
	{  3, 103, {0, 0}}, 
	{  4, 104, {0, 0}}, 
	{  5, 105, {0, 0}}, 
	{  6, 106, {0, 0}}, 
	{  7, 107, {0, 0}}, 
	{  8, 108, {0, 0}}, 
	{  9, 109, {0, 0}}, 
	{ 34, 110, {0, 0}}, 
	{ 35, 111, {0, 0}}, 
	{ 36, 112, {0, 0}}, 
	{ 37, 113, {0, 0}}, 
	{ 38, 114, {0, 0}}, 
	{ 39, 115, {0, 0}}, 
	{ 40, 116, {0, 0}}, 
	{ 41, 117, {0, 0}}, 
	{ 42, 118, {0, 0}}, 
	{ 43, 119, {0, 0}}, 

	{  0, 200, {0, 0}}, 
	{  1, 201, {0, 0}}, 
	{  2, 202, {0, 0}}, 
	{  3, 203, {0, 0}}, 
	{  4, 204, {0, 0}}, 
	{  5, 205, {0, 0}}, 
	{  6, 206, {0, 0}}, 
	{  7, 207, {0, 0}}, 
	{  8, 208, {0, 0}}, 
	{  9, 209, {0, 0}}, 
	{ 34, 210, {0, 0}}, 
	{ 35, 211, {0, 0}}, 
	{ 36, 212, {0, 0}}, 
	{ 37, 213, {0, 0}}, 
	{ 38, 214, {0, 0}}, 
	{ 39, 215, {0, 0}}, 
	{ 40, 216, {0, 0}}, 
	{ 41, 217, {0, 0}}, 
	{ 42, 218, {0, 0}}, 
	{ 43, 219, {0, 0}}, 

	/* gain to auxbus (implicit mixer) */
	{100,  44, {0, 0}},	
	{101,  44, {0, 0}},	
	{102,  44, {0, 0}},	
	{103,  44, {0, 0}},	
	{104,  44, {0, 0}},	
	{105,  44, {0, 0}},	
	{106,  44, {0, 0}},	
	{107,  44, {0, 0}},	
	{108,  44, {0, 0}},	
	{109,  44, {0, 0}},	
	{110,  44, {0, 0}},	
	{111,  44, {0, 0}},	
	{112,  44, {0, 0}},	
	{113,  44, {0, 0}},	
	{114,  44, {0, 0}},	
	{115,  44, {0, 0}},	
	{116,  44, {0, 0}},	
	{117,  44, {0, 0}},	
	{118,  44, {0, 0}},	
	{119,  44, {0, 0}},	

	{200,  45, {0, 0}},	
	{201,  45, {0, 0}},	
	{202,  45, {0, 0}},	
	{203,  45, {0, 0}},	
	{204,  45, {0, 0}},	
	{205,  45, {0, 0}},	
	{206,  45, {0, 0}},	
	{207,  45, {0, 0}},	
	{208,  45, {0, 0}},	
	{209,  45, {0, 0}},	
	{210,  45, {0, 0}},	
	{211,  45, {0, 0}},	
	{212,  45, {0, 0}},	
	{213,  45, {0, 0}},	
	{214,  45, {0, 0}},	
	{215,  45, {0, 0}},	
	{216,  45, {0, 0}},	
	{217,  45, {0, 0}},	
	{218,  45, {0, 0}},	
	{219,  45, {0, 0}}	
};


static struct multi_mix_control controls_envy24[] = {
	{ 100, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 1)" },
	{ 101, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 2)" },
	{ 102, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 3)" },
	{ 103, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 4)" },
	{ 104, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 5)" },
	{ 105, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 6)" },
	{ 106, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 7)" },
	{ 107, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 8)" },
	{ 108, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch. 9)" },
	{ 109, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain Out (ch.10)" },
	{ 110, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 1)" },
	{ 111, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 2)" },
	{ 112, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 3)" },
	{ 113, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 4)" },
	{ 114, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 5)" },
	{ 115, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 6)" },
	{ 116, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 7)" },
	{ 117, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 8)" },
	{ 118, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch. 9)" },
	{ 119, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix L Gain In  (ch.10)" },

	{ 200, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 1)" },
	{ 201, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 2)" },
	{ 202, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 3)" },
	{ 203, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 4)" },
	{ 204, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 5)" },
	{ 205, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 6)" },
	{ 206, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 7)" },
	{ 207, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 8)" },
	{ 208, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch. 9)" },
	{ 209, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain Out (ch.10)" },
	{ 210, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 1)" },
	{ 211, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 2)" },
	{ 212, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 3)" },
	{ 213, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 4)" },
	{ 214, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 5)" },
	{ 215, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 6)" },
	{ 216, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 7)" },
	{ 217, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 8)" },
	{ 218, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch. 9)" },
	{ 219, B_MULTI_MIX_GAIN,   0, {{-144.0,  0.0, 1.5}}, "DMix R Gain In  (ch.10)" },

	{ 300, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 301, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 302, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 303, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 304, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 305, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 306, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 307, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 308, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	{ 309, B_MULTI_MIX_MUX,      0, {}, "Output Select" },
	
	{ 400, B_MULTI_MIX_ENABLE, 400, {}, "Monitor Enable" },
	{ 401, B_MULTI_MIX_ENABLE, 400, {}, "Monitor Enable" },

	{ 500, B_MULTI_MIX_GAIN,   0, {{-94.5,  0.0, 1.5}}, "Monitor L Gain" },
	{ 501, B_MULTI_MIX_GAIN,   0, {{-94.5,  0.0, 1.5}}, "Monitor R Gain" },
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
static int32 ch_10_ctls[] ={ -1 };
static int32 ch_11_ctls[] ={ -1 };
static int32 ch_12_ctls[] ={ -1 };
static int32 ch_13_ctls[] ={ -1 };
static int32 ch_14_ctls[] ={ -1 };
static int32 ch_15_ctls[] ={ -1 };
static int32 ch_16_ctls[] ={ -1 };
static int32 ch_17_ctls[] ={ -1 };
static int32 ch_18_ctls[] ={ -1 };
static int32 ch_19_ctls[] ={ -1 };
static int32 ch_20_ctls[] ={ -1 };
static int32 ch_21_ctls[] ={ -1 };
static int32 ch_22_ctls[] ={ 300, -1 };
static int32 ch_23_ctls[] ={ 301, -1 };
static int32 ch_24_ctls[] ={ 302, -1 };
static int32 ch_25_ctls[] ={ 303, -1 };
static int32 ch_26_ctls[] ={ 304, -1 };
static int32 ch_27_ctls[] ={ 305, -1 };
static int32 ch_28_ctls[] ={ 306, -1 };
static int32 ch_29_ctls[] ={ 307, -1 };
static int32 ch_30_ctls[] ={ 308, -1 };
static int32 ch_31_ctls[] ={ 309, -1 };
static int32 ch_32_ctls[] ={ 400, 500, -1 };
static int32 ch_33_ctls[] ={ 401, 501, -1 };
static int32 ch_34_ctls[] ={ -1 };
static int32 ch_35_ctls[] ={ -1 };
static int32 ch_36_ctls[] ={ -1 };
static int32 ch_37_ctls[] ={ -1 };
static int32 ch_38_ctls[] ={ -1 };
static int32 ch_39_ctls[] ={ -1 };
static int32 ch_40_ctls[] ={ -1 };
static int32 ch_41_ctls[] ={ -1 };
static int32 ch_42_ctls[] ={ -1 };
static int32 ch_43_ctls[] ={ -1 };
static int32 ch_44_ctls[] ={ 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, \
							 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, -1 };
static int32 ch_45_ctls[] ={ 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, \
							 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, -1 };

#define CH(x) { x, (sizeof(ch_## x ##_ctls)-sizeof(int32))/sizeof(int32), ch_## x ##_ctls }

mix_channel_controls channel_controls_envy24[] =
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
	CH(41),
	CH(42),
	CH(43),
	CH(44),
	CH(45)
};	

//How big can we make the object file?
//(A simple algorithm should replace this...)
#define MAX_GAIN_INDEX	97
// 0 - 96  == 0 - -144, 
#define MUTE_INDEX		0xFFFF
static float gain_table[98] = {
	1.0000000000,
	0.8413951416,
	0.7079457844,
	0.5956621435,
	0.5011872336,
	0.4216965034,
	0.3548133892,
	0.2985382619,
	0.2511886432,
	0.2113489040,
	0.1778279410,
	0.1496235656,
	0.1258925412,
	0.1059253725,
	0.0891250938,
	0.0749894209,
	0.0630957344,
	0.0530884444,
	0.0446683592,
	0.0375837404,
	0.0316227766,
	0.0266072506,
	0.0223872114,
	0.0188364909,
	0.0158489319,
	0.0133352143,
	0.0112201845,
	0.0094406088,
	0.0079432823,
	0.0066834392,
	0.0056234133,
	0.0047315126,
	0.0039810717,
	0.0033496544,
	0.0028183829,
	0.0023713737,
	0.0019952623,
	0.0016788040,
	0.0014125375,
	0.0011885022,
	0.0010000000,
	0.0008413951,
	0.0007079458,
	0.0005956621,
	0.0005011872,
	0.0004216965,
	0.0003548134,
	0.0002985383,
	0.0002511886,
	0.0002113489,
	0.0001778279,
	0.0001496236,
	0.0001258925,
	0.0001059254,
	0.0000891251,
	0.0000749894,
	0.0000630957,
	0.0000530884,
	0.0000446684,
	0.0000375837,
	0.0000316228,
	0.0000266073,
	0.0000223872,
	0.0000188365,
	0.0000158489,
	0.0000133352,
	0.0000112202,
	0.0000094406,
	0.0000079433,
	0.0000066834,
	0.0000056234,
	0.0000047315,
	0.0000039811,
	0.0000033497,
	0.0000028184,
	0.0000023714,
	0.0000019953,
	0.0000016788,
	0.0000014125,
	0.0000011885,
	0.0000010000,
	0.0000008414,
	0.0000007079,
	0.0000005957,
	0.0000005012,
	0.0000004217,
	0.0000003548,
	0.0000002985,
	0.0000002512,
	0.0000002113,
	0.0000001778,
	0.0000001496,
	0.0000001259,
	0.0000001059,
	0.0000000891,
	0.0000000750,
	0.0000000631,
	0.0
};

static float idx_to_float(uint16 idx)
{
	if (idx >= MAX_GAIN_INDEX) {
		return 0.0;
	}	
	else return gain_table[idx];
}

static uint16 float_to_idx(float gain)
{
	uint16 i = 0;
	if (gain >= 1.0) {
		return 0;
	}	
	else if (gain <= 0.0) {
		 return MUTE_INDEX;
	}	
	else while (gain_table[i] > gain) {
			i++;
	}
	return i;
}

//FIXME: look at the return values v. return type!!!!
static status_t list_channel_controls_envy24(int32 channel, int32 max, int32 * controls)
{
	int ix, cnt = sizeof(channel_controls_envy24)/sizeof(channel_controls_envy24[0]);

	memset(controls, 0, sizeof(int32)*max);
	for (ix = 0; ix < cnt; ix++) {
		if (channel_controls_envy24[ix].channel == channel) {
			if (channel_controls_envy24[ix].control_count < max)
				max = channel_controls_envy24[ix].control_count;
			memcpy(controls, channel_controls_envy24[ix].control_id, max*sizeof(int32));
			return channel_controls_envy24[ix].control_count;
		}
	}
	return B_MISMATCHED_VALUES;
}


status_t
envy24_multi_list_mix_channels(
				void * cookie,
				void * data,
				size_t len)
{
	multi_mix_channel_info * pmmci = (multi_mix_channel_info *) data;

	int ix,max;
	status_t err = B_OK;

	(void)cookie;	// unused
	(void)len;		// unused

	if (pmmci->info_size < sizeof(multi_mix_channel_info)) {
		return B_BAD_VALUE;
	}	
	pmmci->info_size = sizeof(multi_mix_channel_info);

	pmmci->actual_count = 0;
	for (ix = 0; ix < pmmci->channel_count; ix++) {
		if (pmmci->controls) {
			max = list_channel_controls_envy24(	pmmci->channels[ix],
												pmmci->max_count,
												pmmci->controls[ix]);
		}
		else {
			/* NULL is valid if we are looking for the number of controls */
			/* but set max_count to zero to prevent exceptions            */								
			max = list_channel_controls_envy24(	pmmci->channels[ix],
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


static status_t get_control_envy24(multi_mix_control * control)
{
	uint32 ix;

	for (ix=0; ix<sizeof(controls_envy24)/sizeof(controls_envy24[0]); ix++) {
		if (controls_envy24[ix].id == control->id) {
			control->flags  = controls_envy24[ix].flags;
			control->master = controls_envy24[ix].master;
			if (control->flags & B_MULTI_MIX_GAIN) {
				control->u.gain.min_gain    = controls_envy24[ix].u.gain.min_gain;
				control->u.gain.max_gain    = controls_envy24[ix].u.gain.max_gain;
				control->u.gain.granularity = controls_envy24[ix].u.gain.granularity;
			}
			strcpy(control->name, controls_envy24[ix].name);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

status_t
envy24_multi_list_mix_controls(
				void * cookie,
				void * data,
				size_t len)
{
	multi_mix_control_info * pmmci = (multi_mix_control_info *) data;
	int32 ix;
	status_t err = B_OK;
	
	(void)cookie;	// unused
	(void)len;		// unused

	if (pmmci->info_size < sizeof(multi_mix_control_info)) {
		return B_BAD_VALUE;
	}
	pmmci->info_size = sizeof(multi_mix_control_info);

	for (ix=0; ix<pmmci->control_count; ix++) {
		err = get_control_envy24(&pmmci->controls[ix]);
		if (err < B_OK) {
			return err;
		}	
	}
	return err;
}

status_t
envy24_multi_list_mix_connections(
				void * cookie,
				void * data,
				size_t len)
{
	multi_mix_connection_info * pmmci = (multi_mix_connection_info *) data;
	int max;

	(void)cookie;	// unused
	(void)len;		// unused

	if (pmmci->info_size < sizeof(multi_mix_connection_info))
		return B_BAD_VALUE;
		
	pmmci->info_size = sizeof(multi_mix_connection_info);

	max = pmmci->max_count;
	pmmci->actual_count = sizeof(connections_envy24)/sizeof(connections_envy24[0]);
	if (max > pmmci->actual_count)
		max = pmmci->actual_count;
	memcpy(pmmci->connections, connections_envy24, max*sizeof(connections_envy24[0]));
	return B_OK;
}


static status_t
get_control_master(
				int32 ctl_id )
{
	int32 ix, cnt = sizeof(controls_envy24)/sizeof(controls_envy24[0]);
again:
	for (ix = 0; ix < cnt; ix++) {
		if (controls_envy24[ix].id == ctl_id) {
			if ( controls_envy24[ix].master ) {
				if (ctl_id == controls_envy24[ix].master) {
					return ix;	//	master master is itself....got it?
				}
				/* oh, oh. goto */
				ctl_id = controls_envy24[ix].master;
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
		envy24_dev * cd,
		multi_mix_value * pmmv )
{		
	int32 channel;
	
	switch ( pmmv->id ) {
	case 100:
	case 101:
	case 102:
	case 103:
	case 104:
	case 105:
	case 106:
	case 107:
	case 108:
	case 109:
	case 110:
	case 111:
	case 112:
	case 113:
	case 114:
	case 115:
	case 116:
	case 117:
	case 118:
	case 119:
		channel = pmmv->id - 100;
		pmmv->u.gain = idx_to_float( cd->volume[channel] & 0xFF ) ;
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
	case 210:
	case 211:
	case 212:
	case 213:
	case 214:
	case 215:
	case 216:
	case 217:
	case 218:
	case 219:
		channel = pmmv->id - 200;
		pmmv->u.gain = idx_to_float( (cd->volume[channel] >> 8) & 0xFF );
		break;

	case 300:
	case 301:
	case 302:
	case 303:
	case 304:
	case 305:
	case 306:
	case 307:
	case 308:
	case 309:
		channel = pmmv->id - 300;
		pmmv->u.mux = cd->mux[channel];
		break;
	
	case 400:
	case 401:
		//ganged.....
		//simple case doesn't need get_control_master()
		pmmv->u.enable = cd->enable;
		pmmv->ramp = 0;
		break;
	case 500:
		pmmv->u.gain = idx_to_float( (cd->mon_volume >> 8) & 0xFF );
		break;
	case 501:
		pmmv->u.gain = idx_to_float( cd->mon_volume & 0xFF ) ;
		break;	
	default:
		return B_MISMATCHED_VALUES;
	}
	return B_OK;	
}	

status_t
envy24_multi_get_mix(
				void * cookie,
				void * data,
				size_t len)
{
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_mix_value_info * pmmvi = (multi_mix_value_info *) data;
	int ix;
	status_t err = B_OK;

	(void)len;	// unused

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


static status_t validate_mux( uint16 ctl_idx, uint32 mux)
{
	uint32 mask = 0x800;
	status_t err= B_OK;
	
	ddprintf(("validate_mux(%d, %ld)\n", ctl_idx, mux));
	while (mask && !(mask & mux)) mask = mask >> 1; 

	switch (ctl_idx) {
		case 0:		//supports 0, 34-41, 42-43, 44
		case 1:		//supports 1, 34-41, 42-43, 45
		case 8:		//supports 8, 34-41, 42-43, 44
		case 9:		//supports 9, 34-41, 42-43, 45
			if ( mux & 0xfffff000 || !mux ||  mask != mux ) {
				err = B_BAD_VALUE;
			}
			break;
		case 2:		//supports 2, 34-41, 42-43
		case 3:		//supports 3, 34-41, 42-43
		case 4:		//supports 4, 34-41, 42-43
		case 5:		//supports 5, 34-41, 42-43
		case 6:		//supports 6, 34-41, 42-43
		case 7:		//supports 7, 34-41, 42-43
			if ( mux & 0xfffff800 || !mux ||  mask != mux ) {
				err = B_BAD_VALUE;
			}
			break;	
		default:
			err = B_BAD_VALUE;
			break;
	}
	return err;
}


static status_t
set_info(
		envy24_dev * cd,
		multi_mix_value * pmmv )
{		
	int32 idx, channel;
	status_t err = B_MISMATCHED_VALUES;
	
	switch ( pmmv->id ) {
	case 100:
	case 101:
	case 102:
	case 103:
	case 104:
	case 105:
	case 106:
	case 107:
	case 108:
	case 109:
	case 110:
	case 111:
	case 112:
	case 113:
	case 114:
	case 115:
	case 116:
	case 117:
	case 118:
	case 119:
		channel = pmmv->id - 100;
		if (pmmv->u.gain > 1.0 || pmmv->u.gain < 0.0 ) {
			err = B_MISMATCHED_VALUES;
		}
		else {
			cd->volume[channel] &= 0xff00;
			cd->volume[channel] |=  float_to_idx(pmmv->u.gain);
			set_gain(cd, channel);
			err = B_OK;
		}
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
	case 210:
	case 211:
	case 212:
	case 213:
	case 214:
	case 215:
	case 216:
	case 217:
	case 218:
	case 219:
		channel = pmmv->id - 200;
		if (pmmv->u.gain > 1.0 || pmmv->u.gain < 0.0 ) {
			err = B_MISMATCHED_VALUES;
		}
		else {	
			cd->volume[channel] &= 0x00ff;
			cd->volume[channel] |=  float_to_idx(pmmv->u.gain) << 8;
			set_gain(cd, channel);
			err = B_OK;
		}
		break;

	case 300:
	case 301:
	case 302:
	case 303:
	case 304:
	case 305:
	case 306:
	case 307:
	case 308:
	case 309:
		idx = pmmv->id - 300;
		err = validate_mux(idx, pmmv->u.mux);
		if ( err == B_OK ) {
			cd->mux[idx] = pmmv->u.mux;
			set_mux(cd, idx);
		}	
		break;
		
	case 400:
	case 401:
		cd->enable = pmmv->u.enable;
		set_enable(cd, pmmv->u.enable);
		err = B_OK;
	break;
	case 500:
		if (pmmv->u.gain > 1.0 || pmmv->u.gain < 0.0 ) {
			err = B_MISMATCHED_VALUES;
		}
		else {	
			cd->mon_volume &= 0x00ff;
			cd->mon_volume |=  float_to_idx(pmmv->u.gain) << 8;

			if (ac97 != NULL ) { //there is an AC97 here
				(*ac97->AC97_cached_codec_write)(&cd->codec[0], AC97_PCM,
													cd->mon_volume, 0xffff);
			}
			err = B_OK;
		}
		break;
	case 501:
		if (pmmv->u.gain > 1.0 || pmmv->u.gain < 0.0 ) {
			err = B_MISMATCHED_VALUES;
		}
		else {
			cd->mon_volume &= 0xff00;
			cd->mon_volume |=  float_to_idx(pmmv->u.gain);
			
			if (ac97 != NULL ) { //there is an AC97 here
				(*ac97->AC97_cached_codec_write)(&cd->codec[0], AC97_PCM,
													cd->mon_volume, 0xffff);
			}
			
			err = B_OK;
		}
		break;	

	default:
		err = B_MISMATCHED_VALUES;
	}
	return err;	
}	

status_t
envy24_multi_set_mix(
				void * cookie,
				void * data,
				size_t len)
{
	status_t err = B_OK;
	int32 ix;
	envy24_dev * cd = ((open_envy24 *)cookie)->device;
	multi_mix_value_info * pmmvi = (multi_mix_value_info *) data;
	
	(void)len;	// unused

	if (pmmvi->info_size < sizeof(multi_mix_value_info)) {
		return B_BAD_VALUE;
	}
		
	pmmvi->info_size = sizeof(multi_mix_value_info);
	for ( ix = 0; ix < pmmvi->item_count; ix++ ) {
		err = set_info(cd, &(pmmvi->values[ix]));
		if (err < B_OK) {
			ddprintf(("set_info() failed\n"));
			return err;
		}
		else { /* is this ...an event? */
			if ( cd->current_mask & B_MULTI_EVENT_CONTROL_CHANGED) {
				if (ix >= 100 && (ix / 100) + cd->event_count >= MAX_EVENTS){
					ddprintf(("envy24: out of event space\n"));
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

void initialize_cached_controls( envy24_dev * card)
{
	
	multi_mix_value mmv;
	uint32 i, j;

	// mute everything first, then
	// set it to desired values...
	
	for (i = 0; i < 20; i ++) {
		mmv.id = 100 + i;
		mmv.u.gain = 0.0;
		card->volume[i] = 0xFFFF;	//mutes both left and right channels!
									//only needed first time through
		set_info(card, &mmv);
		mmv.id = 200 + i;
		mmv.u.gain = 0.0;
		set_info(card, &mmv);
	}

	for (i = 0; i < 20; i ++) {
		mmv.id = 100 + i;
		mmv.u.gain =  i & 1 ? 0.0 : 1.0;
		set_info(card, &mmv);
		mmv.id = 200 + i;
		mmv.u.gain =  i & 1 ? 1.0 : 0.0;
		set_info(card, &mmv);
	}

	card->mon_volume = 0x0000;  // 0 dB attenuation for both channels
								// only needed first time through
	mmv.id = 500;
	mmv.u.gain = 1.0;
	set_info(card, &mmv);							
	mmv.id = 501;
	mmv.u.gain = 1.0;
	set_info(card, &mmv);							


	mmv.id = 400;
	mmv.u.enable = 1;
	card->enable = mmv.u.enable;
	set_info(card, &mmv);

	for (i = 0; i < 10; i ++) {
		for (j = 8; j ; j = j >> 1) {
			mmv.id = 300 + i;
			mmv.u.mux = j;			//2 not valid on all channels....
			card->mux[i] = mmv.u.mux;
			set_info(card, &mmv);
		}
	}
}
