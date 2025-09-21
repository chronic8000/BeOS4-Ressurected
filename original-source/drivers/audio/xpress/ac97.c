/************************************************************************/
/*                                                                      */
/*                              ac97.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/*																		*/
/*																		*/
/************************************************************************/

#include <KernelExport.h>

#include "audio_drv.h"
#include "ac97.h"
#include "AC97defs.h"
#include "debug.h"

/*****************************************************************************
 * Default sound_setup parameters
 */
static sound_setup default_sound_setup = {
	// left channel
	{ 	
		aux1,	// adc_source (aux1 is CD) 
		0, 	// adc_gain
		0, 		// mic_gain_enable
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
		aux1,	// adc_source (aux1 is CD) 
		0, 	// adc_gain
		0, 		// mic_gain_enable
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
	linear_16bit_little_endian_stereo,			// playback_format (ignored, always 16bit-linear)
	linear_16bit_little_endian_stereo,			// capture_format (ignored, always 16bit-linear)
	0,			// dither_enable 
	10,			// mic_attn
	0,			// mic_enable
	0,  		// output_boost (ignored, always on)
	0,			// highpass_enable (ignored, always on)
	30,			// mono_gain
	0			// mono_mute
};

typedef struct _ac97_cap_t{
	uint32	EchoMic;		//
	uint32	ModemRate;		//
	uint32	Tone;			//
	uint32	SimStereo;		//
	uint32	HpOut;			//
	uint32	Loud;			//
	uint32	DAC_18Bit;		//
	uint32	DAC_20Bit;		//
	uint32	ADC_18Bit;		//
	uint32	ADC_20Bit;		//
	uint32	cap3DCtrl;			//
	
	uint32	BitHpOutVol;		//
	uint32	BitTone;			//
	uint32	Bit3DCtrl;			//

	uint32	VendorID;			// 
	uint32	RevisionID;		// 
	uint32	QuadCodecType;				// 
	
	}ac97_cap_t;

/* explicits.... */
status_t AC97_SoundSetup(ac97_t * , sound_setup * );




void ac97uninit(ac97_t* ac97)
{
	delete_sem(ac97->soundSetupLockSem);
} 



////////////////////////////////////////////////////////////////



void DetectCodecCaps(ac97_t* ac97, ac97_cap_t* cap)
{
	uint16	wRegVal;	
	uint32	dwVendorID;

	
	dbprintf(("DetectCodecCaps\n"));
	
	// read Reset reg to decode the capabilities
	wRegVal = (*ac97->Read)(ac97->host, AC97_RESET ) ;
	
	dbprintf(("DetectCodecCaps ---> read Reset Register: 0x%x\n", wRegVal));

	cap->EchoMic	= wRegVal & 1;		
	cap->ModemRate	= wRegVal & (1 << 1);		
//	cap->Tone		= ((wRegVal & (1 << 2)) == 0) ? ASP_NO_TONE_CTRL : ASP_HW_TONE_CTRL;			
	cap->SimStereo 	= wRegVal & (1 << 3);		
	cap->HpOut		= wRegVal & (1 << 4);			
	cap->Loud		= wRegVal & (1 << 5);			
	cap->DAC_18Bit	= wRegVal & (1 << 6);		
	cap->DAC_20Bit	= wRegVal & (1 << 7);		
	cap->ADC_18Bit	= wRegVal & (1 << 8);		
	cap->ADC_20Bit	= wRegVal & (1 << 9);		
	cap->cap3DCtrl	= (wRegVal >> 10) & 0x1f;		

	// read 3D Control reg to see if the selected enhancement 
	// is of either fixed or variable center and depth
	if (cap->cap3DCtrl != 0)
		{
			wRegVal = (*ac97->Read)( ac97->host, AC97_3D_CONTROL );
			cap->cap3DCtrl = (wRegVal == 0) ? AC97_CODEC_3D_VARIABLE : AC97_CODEC_3D_FIXED;
		}


	// read volume bits from codec...
	dbprintf(("DetectCodecCaps ---> read volume bits...\n"));
	if ( (*ac97->Write) (ac97->host, AC97_HEADPHONE, 0xA020, 0xffff ) != B_ERROR && 			//4
		 (wRegVal = (*ac97->Read)(ac97->host, AC97_HEADPHONE ))  == 0xA020 ) 
	{
		cap->BitHpOutVol = 6;
	    dbprintf(("DetectCodecCaps ---> Headphone vol bits = %ld\n", cap->BitHpOutVol));
	}
	
	if ((*ac97->Write) (ac97->host, AC97_MASTER_TONE, 0x0101, 0xffff ) != B_ERROR )	// 8
	{ 					     
		wRegVal = (*ac97->Read)(ac97->host,AC97_MASTER_TONE ) ;
		if ( wRegVal == 0x0101 )
		{
				cap->BitTone = 4;
				dbprintf(("DetectCodecCaps --->  Tone Conttrol vol bits = %ld\n", cap->BitTone));
		}
	}


	// read VendorID regs 
	wRegVal = (*ac97->Read)(ac97->host, AC97_VENDOR_ID1) ;
	dwVendorID = wRegVal << 16;
	wRegVal = (*ac97->Read)(ac97->host, AC97_VENDOR_ID2) ;  
	dwVendorID |= wRegVal ;	

	// modify caps based on Vendor ID
	//  Note: we can't detect all caps data from auto detection so 
	//  we have to check Vendor ID to make sure that we have the correct settings.

	switch (dwVendorID)
	{
	case 0x43525903:
		cap->VendorID = AU_AC97CODEC_CRYSTAL_4297;
		cap->HpOut = 1;
		dprintf("ac97: DetectCodecCaps: Crystal cs4297\n");
		break;
	case 0x43525933:
		cap->VendorID = AU_AC97CODEC_CRYSTAL_4299;
		cap->HpOut = 1;
		dprintf("ac97: DetectCodecCaps: Crystal cs4299\n");
		break;

	case 0x83847604:											// SigmaTel 9701/03
		cap->VendorID = AU_AC97CODEC_SIGMATEL_9701;
		dprintf("ac97: DetectCodecCaps: Sigmatel 9701\n");
		break;
		
	case 0x83847605:											// SigmaTel 9704
		cap->VendorID = AU_AC97CODEC_SIGMATEL_9704;
		cap->HpOut = 1;
		cap->cap3DCtrl = AC97_CODEC_3D_VAR_DEPTH_ONLY;
		cap->Bit3DCtrl = 2;
		break;

													// SigmaTel 9708 ; it's in fact for Diamond Monster 
	case 0x83847608:								// these variables maybe are used somewhere to play PCM 
		cap->VendorID = AU_AC97CODEC_SIGMATEL_9708;
		cap->HpOut = 1;	
		cap->cap3DCtrl = AC97_CODEC_3D_VAR_DEPTH_ONLY;
		cap->Bit3DCtrl = 2;
		dprintf("ac97: DetectCodecCaps ---> SigmaTel 9708\n");		
		cap->QuadCodecType = AC97_QUAD_CODEC_ASYMMETRIC;
		break;
		
	case 0x83847644:
		cap->VendorID = AU_AC97CODEC_SIGMATEL_9744;
		cap->HpOut = 0;	
		cap->cap3DCtrl = AC97_CODEC_3D_VAR_DEPTH_ONLY;
		cap->Bit3DCtrl = 2;
		dprintf("ac97: DetectCodecCaps ---> SigmaTel 9744\n");		
		cap->QuadCodecType = AC97_QUAD_CODEC_NONE;
		break;
		
	default:   
		if ((dwVendorID & 0xFFFFFFF0) == 0x574D4C00)
		{
			// Wolfson WM9702 WM9703 
			cap->VendorID = AU_AC97CODEC_WOLFSON_9704;
			cap->HpOut = 1;	
			cap->cap3DCtrl = AC97_CODEC_3D_VAR_DEPTH_ONLY;
			cap->Bit3DCtrl = 4;
			cap->QuadCodecType = AC97_QUAD_CODEC_SYMMETRIC;
		}
		else
			cap->VendorID = AU_AC97CODEC_UNKNOWN;
		break;
	}
	DB_PRINTF(("DetectCodecCaps ---> Codec Vendor ID = %ld (0x%lx)\n", cap->VendorID, dwVendorID));
	DB_PRINTF(("DetectCodecCaps ---> 3D Control vol bits: %ld\n", cap->Bit3DCtrl));
}

					    

	
void InitVendorFeatures(ac97_t* ac97, ac97_cap_t* cap)   // init special features from the different vendors
{						
	uint16	wRegVal, wRegVal2;


	DB_PRINTF(("InitVendorFeatures() ----> codec vendor is %ld\n", cap->VendorID));

	switch (cap->VendorID)  // must be kept somewhere or transmited by caller
	{
	case AU_AC97CODEC_SIGMATEL_9704:
		// enable the extra attenuation
		(*ac97->Write) (ac97->host, 0x5A, 0xABAB, 0xffff );
		(*ac97->Write) (ac97->host, 0x5C, 0x0008, 0xffff );
		DB_PRINTF(("InitVendorFeatures() ----> turn on the Sigmatel STAC9704 extra attenuation."));
		break;

	case AU_AC97CODEC_SIGMATEL_9708:
		// enable EAPD of Powerdown Control/Status Register
		wRegVal = (*ac97->Read)(ac97->host, AC97_CTRL_STAT );
		wRegVal |= 0x8000;
		
		if ((*ac97->Write) (ac97->host, AC97_CTRL_STAT, wRegVal, 0xffff ) == B_ERROR ) {
				DB_PRINTF(("InitVendorFeatures() ----> Fail to write AC97_CTRL_STAT = 0x%x\n", wRegVal));
		}
		// identify revision to make configuration adjustments
		wRegVal = (*ac97->Read)(ac97->host, 0x72 ); 
		wRegVal2 = (*ac97->Read)(ac97->host, 0x6c ); 
		if (wRegVal == 0x0000 && wRegVal2 == 0x0000)	// LA2
		{
			dprintf("ac97: InitVendorFeatures() ----> This is a Sigmatel STAC9708 LA2 codec\n");

			cap->RevisionID = AU_AC97CODEC_REV_ST9708LA2;

			// CIC filter
			(*ac97->Write) (ac97->host, 0x76, 0xABBA, 0xff);
			(*ac97->Write) (ac97->host, 0x78, 0x1000, 0xff);

			// increase the analog currents
			(*ac97->Write) (ac97->host, 0x70, 0xABBA, 0xff );
			(*ac97->Write) (ac97->host, 0x72, 0x0007, 0xff );
		}
		else if (wRegVal == 0x8000 && wRegVal2 == 0x0000)	// LA4
		{
			dprintf("ac97: InitVendorFeatures() ----> This is a Sigmatel STAC9708 LA4 codec\n");
			
			cap->RevisionID = AU_AC97CODEC_REV_ST9708LA4;

				// rear channel inversion fix
				wRegVal = (*ac97->Read)(ac97->host, 0x6E ) ; 
				wRegVal |= 0x0008;
				if ((*ac97->Write) (ac97->host, 0x6E, wRegVal, 0xffff ) == B_ERROR) {							
						DB_PRINTF(("InitVendorFeatures() ----> Failed to write to 6Eh register."));
				}		
		}
		else  {
			DB_PRINTF(("InitVendorFeatures() ---->: We are not supporting this Sigmatel STAC9708 codec"));
		}
		// mute PCM2 Out
		(*ac97->Write) (ac97->host, 0x04, 0x8808, 0xffff );
/*
		// save the codec channels of secondary output for core topology
		m_dw2ndOutputLeftChannel = 4;
		m_dw2ndOutputRightChannel = 5;
			// enable these two channels
		dwValue = mapped_read_32( &(ac97->hw)->regs,OFFSET(ser.ChannelEnable));
		DB_PRINTF(("InitVendorFeatures() ----> enable the codec channels (%ld, %ld) for secondary output (%lx)", 
			m_dw2ndOutputLeftChannel, m_dw2ndOutputRightChannel, dwValue));
		dwValue = dwValue | 0x3000;
		mapped_write_32( &(ac97->hw)->regs ,OFFSET(ser.ChannelEnable), dwValue , 0xffffffff);
*/
		break;

	default:
		if (cap->VendorID == 0) {
			dprintf("ac97: This Codec is not tested, hope it works!\n");
		}
		break;
	}
}

////////////////////////////////////////////////////////////////




status_t ac97init(	ac97_t* ac97, 
					void* host,
					ac97_write_ft codec_write_proc,
					ac97_read_ft codec_read_proc,
					bool init_hw )
{
	bigtime_t timeout;
	uint16 tmp;

	DB_PRINTF(("ac97init() %d\n",init_hw));
	ac97->host = host;
	ac97->Write = codec_write_proc;
	ac97->Read = codec_read_proc;
	ac97->lock = 0;


	/* Semaphor for codec read/write */
	ac97->soundSetupLockSem = create_sem(1, "soundSetupLockSem");
	if (ac97->soundSetupLockSem < 0)	{
		return ac97->soundSetupLockSem; 
	}

	if (init_hw) {
		/* Soft reset */
		(*ac97->Write)(ac97->host, AC97_RESET, 0xffff, 0xffff); 	
		timeout = system_time()+1000000LL;
		tmp = AC97_BAD_VALUE;
		while (((tmp & 0xf) != 0xf) || (tmp == AC97_BAD_VALUE)) {
			tmp = (*ac97->Read)(ac97->host, AC97_POWERDOWN);
			if (system_time() > timeout) {
				dprintf("ac97: powerup after reset timed out\n");
				return B_TIMED_OUT;
			}
			snooze(1000);
		}
		dprintf("ac97: powerup is 0x%x\n",tmp);
	
		/* Test codec interface */
	/*	(*ac97->Write)(ac97->host, AC97_MASTER, 0xffff, 0xffff); 
		if((*ac97->Read)(ac97->host, AC97_MASTER) & ((0x1<<14) | (0x3<<6)) )
			return B_ERROR;
	*/
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
								(uint16)(0x1<<15), 
								(uint16)(0x1<<15)); // set mute bit on
								
			for (i = 0; ac97_ext_regs[i] != 0xff; i++)
				(*ac97->Write)(ac97->host, ac97_ext_regs[i], 
								(uint16)((0x1<<15)|(0x1<<7)), 
								(uint16)((0x1<<15)|(0x1<<7))); // set mute bits on 
		
		}	
	
	
	
	
		/* Set default values (they will be re-set by MediaServer later, anyway) */
		ac97->soundSetup = default_sound_setup;
		{
			sound_setup tmp = default_sound_setup;
			AC97_SoundSetup(ac97, &tmp);
		}
	
		DB_PRINTF(("AC97_POWERDOWN: 0x%x\n", (*ac97->Read)(ac97->host, AC97_POWERDOWN)));
		{
			ac97_cap_t caps;
			DetectCodecCaps(ac97, &caps);
			InitVendorFeatures(ac97,&caps);
		}
	}

	return B_OK;

}

status_t AC97_SoundSetup(ac97_t* ac97, sound_setup *ss)
{
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
	
	// Done. Send back new values
	*ss = ac97->soundSetup;
	ss->sample_rate = old_sr;
	ss->playback_format = old_sf_play;
	ss->capture_format = old_sf_cap;

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

