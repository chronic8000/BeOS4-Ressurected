/************************************************************************/
/*                                                                      */
/*                              AC97defs.h                             	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef __AC97_CODEC_H
#define __AC97_CODEC_H

/*
 *  Audio Codec '97 registers
 *  For more details see AC'97 Component Specification, revision 2.1,
 *  by Intel Corporation (http://developer.intel.com).
 */


#define AC97_RESET			0x00	/* Reset */
#define AC97_MASTER			0x02	/* Master Volume */
#define AC97_HEADPHONE		0x04	/* Headphone Volume (optional) */
#define AC97_MASTER_MONO	0x06	/* Master Volume Mono (optional) */
#define AC97_MASTER_TONE	0x08	/* Master Tone (Bass & Treble) (optional) */
#define AC97_PC_BEEP		0x0a	/* PC Beep Volume (optinal) */
#define AC97_PHONE			0x0c	/* Phone Volume (optional) */
#define AC97_MIC			0x0e	/* MIC Volume */
#define AC97_LINE			0x10	/* Line In Volume */
#define AC97_CD				0x12	/* CD Volume */
#define AC97_VIDEO			0x14	/* Video Volume (optional) */
#define AC97_AUX			0x16	/* AUX Volume (optional) */
#define AC97_PCM			0x18	/* PCM Volume */
#define AC97_REC_SEL		0x1a	/* Record Select */
#define AC97_REC_GAIN		0x1c	/* Record Gain */
#define AC97_REC_GAIN_MIC	0x1e	/* Record Gain MIC (optional) */
#define AC97_GENERAL_PURPOSE	0x20	/* General Purpose (optional) */
#define AC97_3D_CONTROL		0x22	/* 3D Control (optional) */
#define AC97_RESERVED		0x24	/* Reserved */
#define AC97_POWERDOWN		0x26	/* Powerdown control / status */
#define AC97_CTRL_STAT	0x26

/* range 0x28-0x3a - AUDIO */
#define AC97_2_C_LFE		0x36	/* 6-channel volume control */
#define AC97_2_L_R_SURR		0x38	
/* range 0x3c-0x58 - MODEM */
#define AC97_2_LINE1_DAC_ADC	0x46	/* modem dac/adc level conrol */
#define AC97_2_LINE2_DAC_ADC	0x48	
#define AC97_2_HANDSET_DAC_ADC	0x48	
/* range 0x5a-0x7b - Vendor Specific */
#define AC97_VENDOR_ID1		0x7c	/* Vendor ID1 */
#define AC97_VENDOR_ID2		0x7e	/* Vendor ID2 (revision) */


/////////////////////////////////////////////////////////////////////////////////////
/* added by Gabriel */


/* about some producers */
#define AU_AC97CODEC_UNKNOWN			0x00
#define AU_AC97CODEC_SIGMATEL_9701		0x01
#define AU_AC97CODEC_SIGMATEL_9704		0x02
#define AU_AC97CODEC_SIGMATEL_9708		0x03
#define AU_AC97CODEC_SIGMATEL_9744		0x04
#define AU_AC97CODEC_WOLFSON_9704		0x05
#define AU_AC97CODEC_CRYSTAL_4297		0x06
#define AU_AC97CODEC_CRYSTAL_4299		0x07

// AC'97 Quad codec type
#define AC97_QUAD_CODEC_NONE			(420)							// non-quad codec
#define AC97_QUAD_CODEC_ASYMMETRIC		(AC97_QUAD_CODEC_NONE + 1)	// asymmetric quad codec
#define AC97_QUAD_CODEC_SYMMETRIC		(AC97_QUAD_CODEC_NONE + 2)	// symmetric quad codec


// AC'97 CODEC 3D Effect Type
#define AC97_CODEC_3D_NONE              (410)     
#define AC97_CODEC_3D_FIXED             (AC97_CODEC_3D_NONE + 1)
#define AC97_CODEC_3D_VARIABLE          (AC97_CODEC_3D_NONE + 2)
#define AC97_CODEC_3D_VAR_CENTER_ONLY	(AC97_CODEC_3D_NONE + 3)
#define AC97_CODEC_3D_VAR_DEPTH_ONLY	(AC97_CODEC_3D_NONE + 4)


/////////////////////////////////////////////////////////////////////////////
// AC'97 codec revisions

#define AU_AC97CODEC_REV_UNKNOWN			(300)
#define AU_AC97CODEC_REV_ST9708LA2			(AU_AC97CODEC_REV_UNKNOWN + 1)
#define AU_AC97CODEC_REV_ST9708LA4			(AU_AC97CODEC_REV_UNKNOWN + 2)



#endif				/* __AC97_CODEC_H */
