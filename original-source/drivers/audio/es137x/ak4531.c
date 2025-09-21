#include <KernelExport.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "ak4531.h"

/*****************************************************************************
* AKM AK4531 CODEC Control Register Map
*/
#define MASTER_VOLUME_LCH_REGISTER      0x00
#define MASTER_VOLUME_RCH_REGISTER      0x01
#define VOICE_VOLUME_LCH_REGISTER       0x02
#define VOICE_VOLUME_RCH_REGISTER       0x03
#define FM_VOLUME_LCH_REGISTER          0x04
#define FM_VOLUME_RCH_REGISTER          0x05
#define CD_AUDIO_VOLUME_LCH_REGISTER    0x06
#define CD_AUDIO_VOLUME_RCH_REGISTER    0x07
#define LINE_VOLUME_LCH_REGISTER        0x08
#define LINE_VOLUME_RCH_REGISTER        0x09
#define AUX_VOLUME_LCH_REGISTER         0x0A
#define AUX_VOLUME_RCH_REGISTER         0x0B
#define MONO1_VOLUME_REGISTER           0x0C
#define MONO2_VOLUME_REGISTER           0x0D
#define MIC_VOLUME_REGISTER             0x0E
#define MONO_OUT_VOLUME_REGISTER        0x0F
#define OUTPUT_MIXER_SW1_REGISTER       0x10
#define OUTPUT_MIXER_SW2_REGISTER       0x11
#define LCH_INPUT_MIXER_SW1_REGISTER    0x12
#define RCH_INPUT_MIXER_SW1_REGISTER    0x13
#define LCH_INPUT_MIXER_SW2_REGISTER    0x14
#define RCH_INPUT_MIXER_SW2_REGISTER    0x15
#define RESET_POWER_DOWN_REGISTER       0x16
#define CLOCK_SELECT_REGISTER           0x17
#define AD_INPUT_SELECT_REGISTER        0x18
#define MIC_AMP_GAIN_REGISTER           0x19

// the bits for volume att/gain
#define BITS_MASTER_VOLUME_ATT          0x1F
#define BITS_VOICE_VOLUME_GAIN          0x1F
#define BITS_FM_VOLUME_GAIN             0x1F
#define BITS_CD_AUDIO__VOLUME_GAIN      0x1F
#define BITS_LINE_VOLUME_GAIN           0x1F
#define BITS_AUX_VOLUME_GAIN            0x1F
#define BITS_MONO1_VOLUME_GAIN          0x1F
#define BITS_MONO2_VOLUME_GAIN          0x1F
#define BITS_MIC_VOLUME_GAIN            0x1F
#define BITS_MONO_OUT_VOLUME_ATT        0x07

// mix bit position(0-7)
#define MIXBIT_MASTER_VOLUME_MUTE       7
#define MIXBIT_VOICE_VOLUME_MUTE        7
#define MIXBIT_FM_VOLUME_MUTE           7
#define MIXBIT_CD_AUDIO_VOLUME_MUTE     7
#define MIXBIT_LINE_VOLUME_MUTE         7
#define MIXBIT_AUX_VOLUME_MUTE          7
#define MIXBIT_MONO1_VOLUME_MUTE        7
#define MIXBIT_MONO2_VOLUME_MUTE        7
#define MIXBIT_MIC_VOLUME_MUTE          7
#define MIXBIT_MONO_OUT_VOLUME_MUTE     7

// mix bit position(0-7)
#define MIXBIT_OUTPUT_SW1_FML           6
#define MIXBIT_OUTPUT_SW1_FMR           5
#define MIXBIT_OUTPUT_SW1_LINEL         4
#define MIXBIT_OUTPUT_SW1_LINER         3
#define MIXBIT_OUTPUT_SW1_CDL           2
#define MIXBIT_OUTPUT_SW1_CDR           1
#define MIXBIT_OUTPUT_SW1_MIC           0
#define MIXBIT_OUTPUT_SW2_AUXL          5
#define MIXBIT_OUTPUT_SW2_AUXR          4
#define MIXBIT_OUTPUT_SW2_VOICEL        3
#define MIXBIT_OUTPUT_SW2_VOICER        2
#define MIXBIT_OUTPUT_SW2_MONO2         1
#define MIXBIT_OUTPUT_SW2_MONO1         0

// mix bit position(0-7)
#define MIXBIT_INPUT_SW1_FML            6
#define MIXBIT_INPUT_SW1_FMR            5
#define MIXBIT_INPUT_SW1_LINEL          4
#define MIXBIT_INPUT_SW1_LINER          3
#define MIXBIT_INPUT_SW1_CDL            2
#define MIXBIT_INPUT_SW1_CDR            1
#define MIXBIT_INPUT_SW1_MIC            0
#define MIXBIT_INPUT_SW2_TMIC           7
#define MIXBIT_INPUT_SW2_TMONO1         6
#define MIXBIT_INPUT_SW2_TMONO2         5
#define MIXBIT_INPUT_SW2_AUXL           4
#define MIXBIT_INPUT_SW2_AUXR           3
#define MIXBIT_INPUT_SW2_VOICE          2
#define MIXBIT_INPUT_SW2_MONO2          1
#define MIXBIT_INPUT_SW2_MONO1          0

// bit to power down & reset
#define B_RESET_POWER_DOWN_PD           0x02
#define B_RESET_POWER_DOWN_RST          0x01

// bit to select the clocks
#define B_CLOCK_SELECT_CSEL2            0x02
#define B_CLOCK_SELECT_CSEL1            0x01

// mix bit position(0-7)
#define MIXBIT_AD_INPUT_SELECT_ADSEL    0

// mix bit position(0-7)
#define MIXBIT_MIC_AMP_GAIN_MGAIN       0

// Write/Reset/Power Down Timing in uS
// Need to double check specs to get exact required timing... this is just
// an arbitray value for now...
#define CODEC_RESET_DELAY       20
#define CODEC_WRITE_DELAY       20


static
uint8 ak4531_regs_defaults[] =
{//    0,      1,      2,      3,      4,      5,      6,      7,
    0x80,   0x80,   0x06,   0x06,   0x06,   0x06,   0x06,   0x06,
    0x06,   0x06,   0x06,   0x06,   0x06,   0x06,   0x06,   0x80,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x03,   0x03,
    0x00,   0x00
};

static void ak4531_reset(ak4531_t* codec);
static uint8 codec_read(ak4531_t* codec, uint8 reg);
static void codec_write(ak4531_t* codec, uint8 reg, uint8 value);
static uint8 codec_read_bits(ak4531_t* codec, uint8 reg, uint bits, uint8 shift);
static void codec_write_bits(ak4531_t* codec, uint8 reg, uint8 bits,
					   uint8 shift, uint8 value);


status_t ak4531_init(void** codec, void* host, void (*codec_write)(void* host, uint8 reg, uint8 value))
{
    
	ak4531_t* c = NULL;
	*codec = NULL;
	*codec = (ak4531_t*)malloc(sizeof(ak4531_t));
	if(*codec==NULL)
	{
		dprintf("ak4531: memory allocation err\n");
		return ENOMEM;
	}	
	
	c = *codec;
	c->host = host; 
	c->write = codec_write;
				
	//dprintf("Init: c = 0x%lx, c->host = 0x%lx, c->write = 0x%lx \n",
	//	 (uint32)c, (uint32)c->host, (uint32)c->write) ;
		 
	ak4531_reset(c);
	return B_OK; 
}

void ak4531_uninit(void* the_codec)
{
	ak4531_t* codec = (ak4531_t*)the_codec;
	if(codec!=NULL) free(the_codec);
}


void ak4531_Setup(void* the_codec)
{
	ak4531_t* codec = (ak4531_t*)the_codec;
	DB_PRINTF(("In ak4531_Setup\n"));
	dprintf("Setup: c = 0x%lx,c->host = 0x%lx, c->write = 0x%lx \n",
		 (uint32)codec,(uint32)codec->host,(uint32)codec->write) ;
	
    // Selects the clocks for CODEC-ADC(PLL) & CODEC-DAC(PLL).
    codec_write(codec,CLOCK_SELECT_REGISTER,0);
     
    // unmute master 
    codec_write(codec, MASTER_VOLUME_LCH_REGISTER,0x00);
    codec_write(codec, MASTER_VOLUME_RCH_REGISTER,0x00);
}

status_t ak4531_SoundSetup(void* the_codec, sound_setup *ss)
{
	ak4531_t* codec = (ak4531_t*)the_codec;
	DB_PRINTF(("In ak4531_SoundSetup\n"));
	
	
	// adc source
	codec_write_bits(codec, LCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_LINEL,(ss->left.adc_source == line)?1:0);
	codec_write_bits(codec, RCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_LINER,(ss->right.adc_source == line)?1:0);
	codec_write_bits(codec, LCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_CDL,(ss->left.adc_source == aux1)?1:0);
	codec_write_bits(codec, RCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_CDR,(ss->right.adc_source == aux1)?1:0);
	codec_write_bits(codec, LCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_MIC,(ss->left.adc_source == mic)?1:0);
	codec_write_bits(codec, RCH_INPUT_MIXER_SW1_REGISTER ,1,MIXBIT_INPUT_SW1_MIC,(ss->right.adc_source == mic)?1:0);
	codec_write_bits(codec, LCH_INPUT_MIXER_SW2_REGISTER ,1,MIXBIT_INPUT_SW2_VOICE,(ss->left.adc_source == loopback)?1:0);
	codec_write_bits(codec, RCH_INPUT_MIXER_SW2_REGISTER ,1,MIXBIT_INPUT_SW2_VOICE,(ss->right.adc_source == loopback)?1:0);

	//adc gain - no control
	
	// adc input from mixer
    //codec_write_bits(codec, AD_INPUT_SELECT_REGISTER,1,MIXBIT_AD_INPUT_SELECT_ADSEL,0);
	
	// mic gain enable
	// no separate control for the left and right set both enable if one is enable
	ss->left.mic_gain_enable = ss->left.mic_gain_enable || ss->right.mic_gain_enable;
	ss->right.mic_gain_enable = ss->left.mic_gain_enable;

	codec_write_bits(codec, MIC_AMP_GAIN_REGISTER,1,MIXBIT_MIC_AMP_GAIN_MGAIN,ss->right.mic_gain_enable ? 1 : 0);
	
	// cd volume
	codec_write(codec, CD_AUDIO_VOLUME_LCH_REGISTER,(uint8)(ss->left.aux1_mix_gain));
    codec_write(codec, CD_AUDIO_VOLUME_RCH_REGISTER,(uint8)(ss->right.aux1_mix_gain));
	
 	// cd mute    
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1, MIXBIT_OUTPUT_SW1_CDL,ss->left.aux1_mix_mute ? 0 : 1);
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1, MIXBIT_OUTPUT_SW1_CDR,ss->right.aux1_mix_mute ? 0 : 1);

	// aux volume
    codec_write(codec, AUX_VOLUME_LCH_REGISTER,(uint8)((ss->left.aux2_mix_gain)));
    codec_write(codec, AUX_VOLUME_RCH_REGISTER,(uint8)((ss->right.aux2_mix_gain)));

 	// aux mute    
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1, MIXBIT_OUTPUT_SW2_AUXL,ss->left.aux2_mix_mute ? 0 : 1);
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1, MIXBIT_OUTPUT_SW2_AUXR,ss->right.aux2_mix_mute ? 0 : 1);

	// line volume
    codec_write(codec, LINE_VOLUME_LCH_REGISTER,(uint8)((ss->left.line_mix_gain)));
    codec_write(codec, LINE_VOLUME_RCH_REGISTER,(uint8)((ss->right.line_mix_gain)));

 	// line mute    
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1,MIXBIT_OUTPUT_SW1_LINEL,ss->left.line_mix_mute ? 0 : 1);
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1,MIXBIT_OUTPUT_SW1_LINER,ss->right.line_mix_mute ? 0 : 1);

	// dac attn
    codec_write(codec, VOICE_VOLUME_LCH_REGISTER,(uint8)((ss->left.dac_attn>>1)));
    codec_write(codec, VOICE_VOLUME_RCH_REGISTER,(uint8)((ss->right.dac_attn>>1)));

 	// dac mute    
	codec_write_bits(codec, OUTPUT_MIXER_SW2_REGISTER,1,MIXBIT_OUTPUT_SW2_VOICEL,ss->left.dac_mute ? 0 : 1);
	codec_write_bits(codec, OUTPUT_MIXER_SW2_REGISTER,1,MIXBIT_OUTPUT_SW2_VOICER,ss->right.dac_mute ? 0 : 1);


	// mic attn
    codec_write(codec, MIC_VOLUME_REGISTER,(uint8)(ss->loop_attn>>2));
	// mic enable
	codec_write_bits(codec, OUTPUT_MIXER_SW1_REGISTER,1,MIXBIT_OUTPUT_SW1_MIC,ss->loop_enable ? 1 : 0);


/***********************************************************************************/
/* ignore incoming values of mono_gain and mono_mute for now*/
/* there is no GUI support for them in  Audio Settings panel of Media Preferences */
/*  and audio_server sets strange value*/
	ss->mono_mute = 0;
	ss->mono_gain = 63;  // default 
/***********************************************************************************/	
	{
		uint att = (~(ss->mono_gain/2) & 0x1f);

		// mono_gain
		// mono_mute
	    codec_write(codec, MONO_OUT_VOLUME_REGISTER,(uint8)(att>>2) | 
	    				   (ss->mono_mute ? (1<<MIXBIT_MONO_OUT_VOLUME_MUTE) : 0));
		//set master Out volume based on mono volume 
	    codec_write(codec, MASTER_VOLUME_LCH_REGISTER,(uint8)att | 
	    				   (ss->mono_mute ? (1<<MIXBIT_MONO_OUT_VOLUME_MUTE) : 0));
	    codec_write(codec, MASTER_VOLUME_RCH_REGISTER,(uint8)att | 
	    				   (ss->mono_mute ? (1<<MIXBIT_MONO_OUT_VOLUME_MUTE) : 0));
	
	}
	return B_OK;
}


static void ak4531_reset(ak4531_t* codec)
{
	int i;
	
 	codec->write(codec->host,RESET_POWER_DOWN_REGISTER,B_RESET_POWER_DOWN_PD);
 	codec->write(codec->host,RESET_POWER_DOWN_REGISTER,
 			B_RESET_POWER_DOWN_PD | B_RESET_POWER_DOWN_RST);

    // Initialize the registers map
    for (i=0;i<sizeof(codec->regs);i++)
    {
        codec->regs[i] = ak4531_regs_defaults[i];
    }
}

static uint8 codec_read(ak4531_t* codec, uint8 reg)
{
    return codec->regs[reg];
}

static void codec_write(ak4531_t* codec, uint8 reg, uint8 value)
{

 	codec->write(codec->host,reg,value);
    // store it
    codec->regs[reg] = value;
}

static uint8 codec_read_bits(ak4531_t* codec, uint8 reg, uint bits, uint8 shift)
{
    uint8 data = codec_read(codec, reg);

    return(data >> shift) & ~(((uint8)0xff) << bits);
}

static void codec_write_bits(ak4531_t* codec, uint8 reg, uint8 bits,
					   uint8 shift, uint8 value)
{
    uint8 mask = (~(((uint8)0xff) << bits)) << shift;
    uint8 data = codec_read(codec, reg);
    codec_write(codec, reg, (data & ~mask) | ( (value << shift) & mask));
}
