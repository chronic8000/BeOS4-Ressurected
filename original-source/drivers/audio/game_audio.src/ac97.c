/************************************************************************/
/*                                                                      */
/*                              ac97.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
//#define DEBUG
#include <KernelExport.h>
#include <stdlib.h>

#include "audio_drv.h"
#include "ac97.h"
#include "AC97defs.h"
#include "mvp4_debug.h"
#include "game_audio.h"

#define IS_ANUM(c) (((c>='A') && (c<='Z')) || ((c>='a') && (c<='z')) || ((c>='0') && (c<='9')))

#define AC97_VENDOR_ID(ch0,ch1,ch2) ((ch0<<24)|(ch1<<16)|(ch2<<8))

#define VENDOR_CYRRUS_LOGIC_CRYSTAL (AC97_VENDOR_ID('C','R','Y'))
#define VENDOR_SIGMATEL	(AC97_VENDOR_ID(0x83,0x84,0x76)) //Yes, it's strange because "STL" realy is (0x53,0x54,0x4c) or dec(83,84,76)
#define VENDOR_WOLFSON (AC97_VENDOR_ID('W','M','L'))
#define VENDOR_ANALOG_DEVICES (AC97_VENDOR_ID('A','D','S'))

/**************************************************************************
 * Default sound_setup parameters
**************************************************************************/
static sound_setup default_sound_setup = {
	// left channel
	{ 	
		mic,   //aux1,	// adc_source (aux1 is CD) 
		5, 	// adc_gain
		1, 		// mic_gain_enable
		30, 	// cd_mix_gain
		1, 		// cd_mix_mute
		30, 	// aux2_mix_gain
		1, 		// aux2_mix_mute
		20, 	// line_mix_gain
		1, 		// line_mix_mute
		10, 	// dac_attn
		0 		// dac_mute
	},	
	// right channel
	{ 	
		mic, //aux1,	// adc_source (aux1 is CD) 
		5, 	// adc_gain
		1, 		// mic_gain_enable
		30, 	// cd_mix_gain
		1, 		// cd_mix_mute
		30, 	// aux2_mix_gain
		1, 		// aux2_mix_mute
		20, 	// line_mix_gain
		1, 		// line_mix_mute
		10, 	// dac_attn
		0 		// dac_mute
	},	
	kHz_44_1,	// sample_rate
	linear_16bit_little_endian_stereo,
	linear_16bit_little_endian_stereo,
	0,			// dither_enable 
	0,			// mic_attn
	1,			// mic_enable
	0,  		// output_boost 
	0,			// highpass_enable 
	30,			// mono_gain
	0			// mono_mute
};

static sound_setup initial_sound_setup = {//all values here exclude sample_rate are bullshit
										  //but they are not used now
	// left channel
	{ 	
		mic,   //aux1,	// adc_source (aux1 is CD) 
		0, 	// adc_gain
		1, 		// mic_gain_enable
		30, 	// cd_mix_gain
		1, 		// cd_mix_mute
		30, 	// aux2_mix_gain
		1, 		// aux2_mix_mute
		20, 	// line_mix_gain
		1, 		// line_mix_mute
		10, 	// dac_attn
		0 		// dac_mute
	},	
	// right channel
	{ 	
		mic, //aux1,	// adc_source (aux1 is CD) 
		0, 	// adc_gain
		1, 		// mic_gain_enable
		30, 	// cd_mix_gain
		1, 		// cd_mix_mute
		30, 	// aux2_mix_gain
		1, 		// aux2_mix_mute
		20, 	// line_mix_gain
		1, 		// line_mix_mute
		10, 	// dac_attn
		0 		// dac_mute
	},	
	kHz_44_1,	// sample_rate
	linear_16bit_little_endian_stereo,
	linear_16bit_little_endian_stereo,
	0,			// dither_enable 
	0,			// mic_attn
	1,			// mic_enable
	0,  		// output_boost 
	0,			// highpass_enable 
	30,			// mono_gain
	0			// mono_mute
};


typedef struct ac97hw_t{
	union{
		struct{
			uint16 ID1;
			uint16 ID2;
		}reg;
		struct {
			char S; //second
			char F; //first
			uchar Rev;
			char T;//third 
		}ch;
	}VendorID;
	uint16 ExtendedAudioStatus;
	union{
		struct
		{
			bool OldSampleRateControlRegs; //on AD1819, AD1819A and probably others
		}AD;//Analog devices
	}VendSpecific;
}ac97hw_t;


void ac97uninit(ac97_t* ac97)
{
	delete_sem(ac97->soundSetupLockSem);
	if(ac97->hw) free(ac97->hw);
} 


status_t ac97init(	ac97_t* ac97, 
					void* host,
					ac97_write_ft codec_write_proc,
					ac97_read_ft codec_read_proc )
{
	bigtime_t timeout;
	ac97hw_t* ac97hw;
	
	DB_PRINTF(("ac97init()\n"));
	ac97->host = host;
	ac97->Write = codec_write_proc;
	ac97->Read = codec_read_proc;
	ac97->hw = NULL;
	ac97->hw = malloc(sizeof(ac97hw_t));
	if(ac97->hw == NULL) return ENOMEM;
	memset(ac97->hw,0,sizeof(ac97hw_t));
	ac97hw = (ac97hw_t*)ac97->hw;
	ac97->lock = 0;
	

	/* Semaphor for codec read/write */
	ac97->soundSetupLockSem = create_sem(1, "soundSetupLockSem");
	if (ac97->soundSetupLockSem < 0)	{
		return ac97->soundSetupLockSem; 
	}

	/* Soft reset */
	(*ac97->Write)(ac97->host, AC97_RESET, 0xffff, 0xffff); 	
	timeout = system_time()+1000000LL;
	while (((*ac97->Read)(ac97->host, AC97_POWERDOWN) & 0xf) != 0xf) {
		if (system_time() > timeout) {
			dprintf("ac97: powerup after reset timed out\n");
			return B_TIMED_OUT;
		}
		snooze(1000);
	}
	snooze(10000);

	/* Mute all mixer input channels (just to make sure nothing is going through) */
	{
		int i;
		uint8 ac97_regs[] = {
								AC97_PC_BEEP, 
								AC97_PHONE, 
								AC97_MIC, 
								AC97_LINE, 
								AC97_CD, 
								AC97_VIDEO, 
								AC97_AUX, 
								AC97_PCM, 
								AC97_REC_GAIN, 
								AC97_REC_GAIN_MIC,
								0xff	// terminate the list
							},
			ac97_ext_regs[] = {	// AC97 version 2.0 extension regs
								AC97_2_C_LFE, 
								AC97_2_L_R_SURR,
								AC97_2_LINE1_DAC_ADC, 
								AC97_2_LINE2_DAC_ADC, 
								AC97_2_HANDSET_DAC_ADC,
								0xff	// terminate the list
							};
		
		for (i = 0; ac97_regs[i] != 0xff; i++)
			(*ac97->Write)(ac97->host, ac97_regs[i], 
							(uint16)(0x1<<16), 
							(uint16)(0x1<<16)); // set mute bit on
							
		for (i = 0; ac97_ext_regs[i] != 0xff; i++)
			(*ac97->Write)(ac97->host, ac97_ext_regs[i], 
							(uint16)((0x1<<16)|(0x1<<8)), 
							(uint16)((0x1<<16)|(0x1<<8))); // set mute bits on 
	
	}	

	// read VendorID regs 
	ac97hw->VendorID.reg.ID1 = (*ac97->Read)(ac97->host, AC97_VENDOR_ID1);
	ac97hw->VendorID.reg.ID2 = (*ac97->Read)(ac97->host, AC97_VENDOR_ID2);

    dprintf("AC97 VendorID1 = 0x%x, VendorID2 = 0x%x\n",
					(*ac97->Read)(ac97->host, AC97_VENDOR_ID1), (*ac97->Read)(ac97->host, AC97_VENDOR_ID2));
	if(IS_ANUM(ac97hw->VendorID.ch.F) && IS_ANUM(ac97hw->VendorID.ch.S) && IS_ANUM(ac97hw->VendorID.ch.T))
		dprintf("AC97 Vendor ID: %c%c%c%d\n",ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,
					ac97hw->VendorID.ch.T, (uint16)ac97hw->VendorID.ch.Rev);

	{ //enable Variable Rate if supported
		uint16 supported = ac97->Read(ac97->host, 0x28);
		dprintf("ExtendedAudioID = 0x%x\n",supported);
		if(supported & 0x1 )//if Variable Rate Pcm Audio supported
		{
			ac97->Write(ac97->host, 0x2a, 0x1, 0x1);//enable VRA
			if(supported & (0x1<<3))//if Variable Rate Mic Input supported
				ac97->Write(ac97->host, 0x2a, 0x1<<3, 0x1<<3);//enable VRM
			ac97hw->ExtendedAudioStatus = ac97->Read(ac97->host, 0x2a);
			dprintf("ac97hw->ExtendedAudioStatus = 0x%x\n",ac97hw->ExtendedAudioStatus);
		}
	}


	switch( AC97_VENDOR_ID(ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,ac97hw->VendorID.ch.T) )
	{
	case VENDOR_CYRRUS_LOGIC_CRYSTAL:
		{
			switch(ac97hw->VendorID.ch.Rev)
		 	{
		 	case 0x03:
				dprintf("ac97 is Cyrrus Logic cs4297\n");
				break;
		 	case 0x33:
				dprintf("ac97 is Cyrrus Logic cs4299\n");
				break;
			default:
				dprintf("ac97 vendor is Cyrrus Logic.\n");
				break;
		 	}
		}
		break;
	case VENDOR_SIGMATEL:
		{
			switch(ac97hw->VendorID.ch.Rev)
		 	{
			case 0x04:											
				dprintf("ac97 is Sigmatel 9701/03\n");
				break;
			case 0x05:											
				dprintf("ac97 is SigmaTel 9704\n");
				// enable the extra attenuation
				(*ac97->Write) (ac97->host, 0x5A, 0xABAB, 0xffff );
				(*ac97->Write) (ac97->host, 0x5C, 0x0008, 0xffff );
				break;
			case 0x08:								
				dprintf("ac97 is SigmaTel 9708\n");
				{
					uint16 wRegVal, wRegVal2;		
					(*ac97->Write)(ac97->host, AC97_CTRL_STAT, 0x8000, 0x8000 );
					// identify revision to make configuration adjustments
					wRegVal = (*ac97->Read)(ac97->host, 0x72 ); 
					wRegVal2 = (*ac97->Read)(ac97->host, 0x6c ); 
					if (wRegVal == 0x0000 && wRegVal2 == 0x0000)	// LA2
					{
						dprintf("This is a Sigmatel STAC9708 LA2 codec\n");
			
						// CIC filter
						(*ac97->Write) (ac97->host, 0x76, 0xABBA, 0xff);
						(*ac97->Write) (ac97->host, 0x78, 0x1000, 0xff);
			
						// increase the analog currents
						(*ac97->Write) (ac97->host, 0x70, 0xABBA, 0xff );
						(*ac97->Write) (ac97->host, 0x72, 0x0007, 0xff );
					}
					else if (wRegVal == 0x8000 && wRegVal2 == 0x0000)	// LA4
					{
						dprintf("This is a Sigmatel STAC9708 LA4 codec\n");
						
							// rear channel inversion fix
							wRegVal = (*ac97->Read)(ac97->host, 0x6E ) ; 
							wRegVal |= 0x0008;
							(*ac97->Write) (ac97->host, 0x6E, wRegVal, 0xffff );
					}
					else
						dprintf("This is a Sigmatel STAC9708 unknown revision codec. Probably needs configuration adjustments\n");
				}
				break;
				
			case 0x44:
				dprintf("ac97 is SigmaTel 9744\n");		
				break;
			default:
				dprintf("ac97 vendor is Sigmatel.\n");
				break;
		 	}
		}
		break;
	case VENDOR_WOLFSON:
		dprintf("ac97 vendor is Wolfson.\n");
		break;
	case VENDOR_ANALOG_DEVICES:
		{
			switch(ac97hw->VendorID.ch.Rev)
		 	{
		 	case 0x00:
		 	case 0x03://I do not have specs for this release but it supports SampleRateControl as AD1819 
				{
				dprintf("ac97 is Analog Devices AD1819/AD1819A/...\n");
//				dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74));
				dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74));
				(*ac97->Write) (ac97->host, 0x74, (0x1<<11), (0x1<<11));
				dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74));
				ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE;
				ac97hw->ExtendedAudioStatus = 0x1;
				}
				break;
		/* 	case 0x03:
		 	//I do not have specs for this release but it supports SampleRateControl as AD1819 
				dprintf("ac97 vendor is Analog Devices.\n");
				ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE;
				ac97hw->ExtendedAudioStatus = 0x1;
		    		break;*/
		 	case 0x40:
				dprintf("ac97 is Analog Devices AD1881\n");
				break;
			default:
				//I do not have the specs for this AD ac97 codec revision
				dprintf("ac97 vendor is Analog Devices.\n");
				dprintf("ac97hw->ExtendedAudioStatus = 0x%x\n",ac97hw->ExtendedAudioStatus);
				if((ac97hw->ExtendedAudioStatus & 0x1) == 0)
				{//but it does not support Variable Rate by the standard way
					dprintf("Initial DAC rate is %d, ADC rate is %d\n",ac97->Read(ac97->host, 0x7a),ac97->Read(ac97->host, 0x78));

					if( (ac97->Read(ac97->host, 0x7a) == 0xBB80) &&
							(ac97->Read(ac97->host, 0x78) == 0xBB80) )
					{//And, yes, support using 0x7a and 0x78 regs 
						dprintf("ac97 Analog Devices old variable rate control.\n");
						ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE;
						ac97hw->ExtendedAudioStatus = 0x1;
					}
				}
				break;
		 	}
		}
		break;
	default:
		break;
	}

	/* Set default values (they will be re-set by MediaServer later, anyway) */
	ac97->soundSetup = initial_sound_setup;
	{
		sound_setup tmp = default_sound_setup;
		AC97_SoundSetup(ac97, &tmp);
	}

	return B_OK;

}

status_t AC97_SoundSetup(ac97_t* ac97, sound_setup *ss)
{
	ac97hw_t* ac97hw = (ac97hw_t*)ac97->hw;

	enum sample_rate old_sr = ss->sample_rate;
	enum sample_format old_sf_play = ss->playback_format;
	enum sample_format old_sf_cap = ss->capture_format;

	status_t err = acquire_sem_etc(ac97->soundSetupLockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if(err != B_OK) return err;


	// Set master Out volume attenuation 
	// for now, hardcode to max., since there are no controls in the control panel 
	ac97->Write(ac97->host, AC97_MASTER,  
			0x0000 |	//((ss->mono_gain/2) << 8) | (ss->mono_gain/2) | 
			((ss->mono_mute) ? 0x8000 : 0), 
			0xffff
		);
		
	// Set headphones Out volume attenuation
	ac97->Write(ac97->host,AC97_HEADPHONE, 
			0x0000 |	//((ss->mono_gain/2) << 8) | (ss->mono_gain/2) |
			((ss->mono_mute) ? 0x8000 : 0), 
			0xffff
		);
		
	// Set master mono Out volume attenuation
	ac97->Write(ac97->host,AC97_MASTER_MONO, 
			0x0000 |	// (ss->mono_gain/2) |
			((ss->mono_mute) ? 0x8000 : 0), 
			0xffff
		);
	ac97->soundSetup.mono_mute = ss->mono_mute;	
	ac97->soundSetup.mono_gain = ss->mono_gain;	

/*
	// Set Mic Volume
	// no separate controls for the left and right mute, so set both enable if one is enable	ac97->soundSetup.left.dac_mute = ss->left.dac_mute || ss->right.dac_mute; 
	ac97->soundSetup.left.mic_gain_enable = ss->left.mic_gain_enable || ss->right.mic_gain_enable;
	ac97->soundSetup.right.mic_gain_enable = ac97->soundSetup.left.mic_gain_enable;
	ac97->Write(ac97->host, AC97_MIC,  
			(ss->loop_attn / 2)
			 | ((ss->loop_enable) ? 0 : 0x8000)
			 | ((ac97->soundSetup.left.mic_gain_enable) ? 0x40 : 0 ), // +20dB enable bit
			0xffff
		);
	ac97->soundSetup.loop_attn = ss->loop_attn;
	ac97->soundSetup.loop_enable = ss->loop_enable;
*/
	// Set Line-In Volume 
	ac97->soundSetup.left.line_mix_mute = ss->left.line_mix_mute || ss->right.line_mix_mute;
	ac97->soundSetup.right.line_mix_mute = ac97->soundSetup.left.line_mix_mute;
	ac97->Write(ac97->host, AC97_LINE, 
			((ss->left.line_mix_gain) << 8) | (ss->right.line_mix_gain) |
			((ac97->soundSetup.left.line_mix_mute) ? 0x8000 : 0), 
			0xffff
		);
	ac97->soundSetup.left.line_mix_gain= ss->left.line_mix_gain;
	ac97->soundSetup.right.line_mix_gain= ss->right.line_mix_gain;

	// CD Volume (aux1 controls CD) 
	ac97->soundSetup.left.aux1_mix_mute = ss->left.aux1_mix_mute || ss->right.aux1_mix_mute;
	ac97->soundSetup.right.aux1_mix_mute = ac97->soundSetup.left.aux1_mix_mute;
	ac97->Write(ac97->host, AC97_CD, 
			(ss->left.aux1_mix_gain << 8) | (ss->right.aux1_mix_gain) |
			((ac97->soundSetup.left.aux1_mix_mute) ? 0x8000 : 0),
			0xffff
		);
	ac97->soundSetup.left.aux1_mix_gain = ss->left.aux1_mix_gain;
	ac97->soundSetup.right.aux1_mix_gain = ss->right.aux1_mix_gain;
	
	// Set AUX Volume
	ac97->soundSetup.left.aux2_mix_mute = ss->left.aux2_mix_mute || ss->right.aux2_mix_mute;
	ac97->soundSetup.right.aux2_mix_mute = ac97->soundSetup.left.aux2_mix_mute;
	ac97->Write(ac97->host, AC97_PHONE, 
			(ss->left.aux2_mix_gain << 8)
			| (ss->right.aux2_mix_gain)
			| ((ac97->soundSetup.left.aux2_mix_mute) ? 0x8000 : 0),
			0xffff
		);
	ac97->soundSetup.left.aux2_mix_gain = ss->left.aux2_mix_gain;
	ac97->soundSetup.right.aux2_mix_gain = ss->right.aux2_mix_gain;

	// Set PCM Out volume	
	ac97->soundSetup.left.dac_mute = ss->left.dac_mute || ss->right.dac_mute;
	ac97->soundSetup.right.dac_mute = ac97->soundSetup.left.dac_mute; 
	ac97->Write(ac97->host, AC97_PCM, 
			((ss->left.dac_attn/2) << 8) | (ss->right.dac_attn/2) |
			((ac97->soundSetup.left.dac_mute) ? 0x8000 : 0),
			0xffff
		);
	ac97->soundSetup.left.dac_attn = ss->left.dac_attn;
	ac97->soundSetup.right.dac_attn = ss->right.dac_attn;
/*	
	{// Set Record Select (input source)
		static unsigned char rec_src[4] = {
			4, // line
			1, // aux1 is CD
			0, // mic
			5  // loopback is stereo mix
		};
		ac97->Write(ac97->host, AC97_REC_SEL, 
			(rec_src[ss->left.adc_source] << 8)
			| rec_src[ss->right.adc_source],
			0xffff
			);
	}
	ac97->soundSetup.left.adc_source = ss->left.adc_source;		
	ac97->soundSetup.right.adc_source = ss->right.adc_source;		
		
	//	Set ADC (Record Gain) Volume
	ac97->Write(ac97->host, AC97_REC_GAIN,  
						((ss->left.adc_gain) << 8) | (ss->right.adc_gain),
						0xffff
					);
	//	Set Record Gain Mic Volume 
	ac97->Write(ac97->host, AC97_REC_GAIN_MIC, 
						(ss->left.adc_gain + ss->right.adc_gain) / 2 ,
						0xffff
					);
	ac97->soundSetup.left.adc_gain = ss->left.adc_gain;	
	ac97->soundSetup.right.adc_gain = ss->right.adc_gain;	
*/	
	ss->sample_rate = kHz_44_1;
	
	if(ss->sample_rate!= kHz_48_0) if(ss->sample_rate!=ac97->soundSetup.sample_rate)
	{
		dprintf("ss->sample_rate = %d, ac97->soundSetup.sample_rate = %d\n",ss->sample_rate,ac97->soundSetup.sample_rate);
		if(ac97hw->ExtendedAudioStatus & 0x1)
		{//Support variable rate
			uchar DACRateReg = 0x2c, ADCRateReg = 0x32;
			uint16 rate;
			switch( AC97_VENDOR_ID(ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,ac97hw->VendorID.ch.T) )
			{
			case VENDOR_ANALOG_DEVICES:
				if(ac97hw->VendSpecific.AD.OldSampleRateControlRegs)
				{
					DACRateReg = 0x7a;
					ADCRateReg = 0x78;
				}
				break;
			default:
				break;
			}
			switch(ss->sample_rate)
			{
			case kHz_48_0:
				rate = 48000;
				break;
			case kHz_44_1:
			default:
				rate = 44100;
				break;
			}	
			ac97->Write(ac97->host, DACRateReg, rate,0xffff);
			dprintf("Current rate is %d\n",ac97->Read(ac97->host, DACRateReg));
			ac97->Write(ac97->host, ADCRateReg, rate,0xffff);
			if(ac97hw->ExtendedAudioStatus & (0x1<<3))
				ac97->Write(ac97->host, 0x34, rate,0xffff);
			ac97->soundSetup.sample_rate = ss->sample_rate;
		}
	}
	release_sem(ac97->soundSetupLockSem);
	return B_OK;
}

status_t  AC97_GetSoundSetup(ac97_t* ac97, sound_setup *ss)
{
	status_t err;
	err = acquire_sem_etc(ac97->soundSetupLockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if(err != B_OK) return err;
	*ss = ac97->soundSetup;
	release_sem(ac97->soundSetupLockSem);
	return B_OK;
}


status_t  AC97_SetSampleRate(ac97_t* ac97, uint16 rate)
{
	ac97hw_t* ac97hw = (ac97hw_t*)ac97->hw;
	if(ac97hw->ExtendedAudioStatus & 0x1)
	{//Support variable rate
			uchar DACRateReg = 0x2c, ADCRateReg = 0x32;
			
			switch( AC97_VENDOR_ID(ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,ac97hw->VendorID.ch.T) )
			{
				case VENDOR_ANALOG_DEVICES:
					if(ac97hw->VendSpecific.AD.OldSampleRateControlRegs)
					{
						DACRateReg = 0x7a;
						ADCRateReg = 0x78;
					}
					break;
				default:
					break;
			}
			switch(rate)
			{
				case kHz_48_0:
					rate = 48000;
					break;
				case kHz_44_1:
				default:
					rate = 44100;
				break;
			}	
			
			ac97->Write(ac97->host, DACRateReg, rate,0xffff);
			dprintf("Current rate is %d\n",ac97->Read(ac97->host, DACRateReg));
			ac97->Write(ac97->host, ADCRateReg, rate,0xffff);
			if(ac97hw->ExtendedAudioStatus & (0x1<<3))
				ac97->Write(ac97->host, 0x34, rate,0xffff);
	}
	

}

status_t  AC97_SetSound(ac97_t* ac97, int16 types, int16 value)
{
	ac97hw_t* ac97hw = (ac97hw_t*)ac97->hw;
	
	status_t err = acquire_sem_etc(ac97->soundSetupLockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if(err != B_OK) return err;

    switch (types) 
	{
		
		case GAME_LEVEL_AC97_MASTER:
			// Set master Out volume attenuation 
			// for now, hardcode to max., since there are no controls in the control panel 
			ac97->Write(ac97->host, AC97_MASTER,  
						(value<<8) | value |	//((ss->mono_gain/2) << 8) | (ss->mono_gain/2) | 
						0,          //stereo ???
						0xffff
						);
			break;			
		case GAME_LEVEL_AC97_PHONE:	
			// Set headphones Out volume attenuation
			ac97->Write(ac97->host,AC97_HEADPHONE, 
						0x0000 |	//((ss->mono_gain/2) << 8) | (ss->mono_gain/2) |
						0,          // stereo 
						0xffff
						);
			break;
		case GAME_LEVEL_AC97_MONO:
			// Set master mono Out volume attenuation
			ac97->Write(ac97->host,AC97_MASTER_MONO, 
						0x0000 |	// (ss->mono_gain/2) |
						0,          //  stereo 
						0xffff
						);
			break;
	
		case GAME_LEVEL_AC97_PCM:
			// Set PCM Out volume	
			ac97->Write(ac97->host, AC97_PCM, 
						(value/2 <<8) | value/2 |
						0,  // stereo
						0xffff
						);
		    break;
		
		case GAME_LEVEL_AC97_LINE_IN:
			ac97->Write(ac97->host, AC97_PHONE, 0x8808,	0xffff);
		    ac97->Write(ac97->host, AC97_CD, 0x8808,	0xffff);
		    ac97->Write(ac97->host, AC97_VIDEO, 0x8808,	0xffff);
			ac97->Write(ac97->host, AC97_MIC, 0x8808,	0xffff);

			ac97->Write(ac97->host, AC97_REC_SEL, 
				        0X0404,   // line in select
		    		    0xffff
						);

			ac97->Write(ac97->host, AC97_LINE, 
						0x0808,
						0xffff
						);
			//	Set ADC (Record Gain) Volume
			ac97->Write(ac97->host, AC97_REC_GAIN,
	                    (value <<8 )| value,  
	                 	0xffff
						);
		    break;	

		case GAME_LEVEL_AC97_CD:
		    ac97->Write(ac97->host, AC97_PHONE, 0x8808,	0xffff);
		    ac97->Write(ac97->host, AC97_LINE, 0x8808,	0xffff);
		    ac97->Write(ac97->host, AC97_VIDEO, 0x8808,	0xffff);
			ac97->Write(ac97->host, AC97_MIC, 0x8808,	0xffff);

			ac97->Write(ac97->host, AC97_REC_SEL, 
				        0X0101,   // cd select
		    		    0xffff
						);

			ac97->Write(ac97->host, AC97_CD, 
						0x0808,
						0xffff
						);
			//	Set ADC (Record Gain) Volume
			ac97->Write(ac97->host, AC97_REC_GAIN,
	                    (value <<8 )| value,  
	                 	0xffff
						);
		    break;
		    
		case GAME_LEVEL_AC97_MIC:	
           ac97->Write(ac97->host, AC97_CD, 0x8808,	0xffff);
           ac97->Write(ac97->host, AC97_PHONE, 0x8808,	0xffff);
		   ac97->Write(ac97->host, AC97_LINE, 0x8808,	0xffff);
		   ac97->Write(ac97->host, AC97_VIDEO, 0x8808,	0xffff);
            // Mic Volume	
			ac97->Write(ac97->host, AC97_MIC,  
						 0x0008,   //(ss->loop_attn / 2)
						 //((ss->loop_enable) ? 0 : 0x8000)
						 //((ac97->soundSetup.left.mic_gain_enable) ? 0x40 : 0 ), // +20dB enable bit
						 0xffff
			             );
							
			ac97->Write(ac97->host, AC97_REC_SEL, 
				        0,   // mic select
		    		    0xffff
						);
				
			//	Set ADC (Record Gain) Volume
			ac97->Write(ac97->host, AC97_REC_GAIN,
	                    (value <<8 )| value,  
	                 	0xffff
						);
			//	Set Record Gain Mic Volume 
			ac97->Write(ac97->host, AC97_REC_GAIN_MIC, 
	                    0, //value,
						0xffff
						);	
			break;			
	}
	
	release_sem(ac97->soundSetupLockSem);
	return B_OK;
}



