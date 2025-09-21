/******************************************************************
    file name: shared.h
	This is the header file for Modem 

	author: Han C. Yeh 
	
	copyright @ PCtel Inc. Al rights reserved.
	history:
	03-26-94:	hcy created
	01-30-98:	hcy Moved txData/rxData/eyePat buffers. Added
					codecType and bufferType fields
	02-12-98:	gaa Added definitions for v.90
******************************************************************/ 
/* WARNING!!!!  don't allocate any memory location in this header file */
#ifndef SHARED_H
#define SHARED_H
#include "intdef.h"
#include "ind_ref.h" 

#undef		VMODEM_REAL_TIME_DEBUG
#define		AUTO_CPU_UTILIZATION

#define		VMODEM_DEBUG_STRING_SIZE	256
#define		MAX_TONE_NUM				10
#define		EYE_PATTERN_SIZE			(64*2)
#define		MAX_DATA_BUFFER_SIZE		(64*2)

/* DSP/Controller data passing format */
/*--------------------------------------------------- */
/*  modem			bytes/block		 bits/byte */
/*---------------------------------------------------  */
/* 	bell_300(fsk)		1				1/8(tx/rx) */
/*	bell_212			2				2 */
/*	v21(fsk) 			1  				1/8(tx/rx) */
/*	v22					2				2 */
/* 	v22bis				2 				4 */
/* 	v23(fsk)            1				4/8(tx/rx) */
/*	v27ter-2400			2 				2 */
/* 	v27ter-4800			4				6 */
/*	v29-7200			8				3 */
/*	v29-9600			8  				4  */
/*	v17-7200			8				3 */
/*	v17-9600			8				4 */
/*	v17-12000			8				5  */
/*	v17-14400			8				6 */
/*	v32-4800			8  				2 */
/*	v32-9600			8 				4 */
/*	v32-9600T			8 				4  */
/*	v32bis-7200			8				3 */
/* 	v32bis-9600			8				4 */
/*	v32bis-12000		8				5 */
/*	v32bis-14400		8 				6  */
/*	v34_2400			1				8  */
/*	v34_4800			2				8 */
/*	v34_7200			3				8 */
/*	v34_9600			4				8 */
/*	v34_12000			5				8 */
/*	v34_14400			6				8 */
/*	v34_16800			7				8 */
/*	v34_19200			8				8 */
/*	v34_21600			9				8 */
/*	v34_24000			10				8 */
/*	v34_26400			11				8 */
/*	v34_28800			12				8											 */

/*----------------------------------------------------- */
/*   modem type */
/*----------------------------------------------------- */
/* */
enum 
{
	bell_300,  	
	bell_212,
	v21,				
	v21fax,
	v22,
	v23,	
	v27ter,
	v29,		
	v17,
	v32,
	v32bis,
	v32ter,        			
	v34,
	v56k,
	v90,
}; 
  
/*----------------------------------------------------- */
/*   speed type */
/*----------------------------------------------------- */
/*  */
enum {
	v22_1200,  		/* 0 */
	v22bis,			/* 1 */
};
enum {
   	v23_75_1200,	/* tx/rx: 75/1200 */
	v23_1200_75,	/* tx/rx:1200/75	 */
};  
enum {
   	v27_2400,
	v27_4800,
};  
enum {
   	v29_7200,			
	v29_9600,
};
enum {
	v17_4800,		/* unused! */
   	v17_7200,
	v17_9600,
	v17_12000,  		
	v17_14400,		
}; 

enum {
    v32_4800,
	v32_9600,
	v32_9600T,
};
enum {
	v32bis_4800,
    v32bis_7200,
	v32bis_9600,		
	v32bis_12000, 	 																									
	v32bis_14400,   
};
enum{
	v32ter_16800,
	v32ter_19200,
};
enum{
	v34_2400_bps=0, 			/* v34, 2400 bps */
	v34_4800_bps=2,
	v34_7200_bps=4,
	v34_9600_bps=6,
	v34_12000_bps=8,
	v34_14400_bps=10,
	v34_16800_bps=12,
	v34_19200_bps=14,
	v34_21600_bps=16,
	v34_24000_bps=18,
	v34_26400_bps=20,
	v34_28800_bps=22,
	v34_31200_bps=24,
	v34_33600_bps=26,
};
enum{
 	v34_2400_baud, 		/* v34, 2400 baud */
	v34_2743_baud,		/* v34, 2743 baud */
	v34_2800_baud,		/* v34, 2800 baud */
	v34_3000_baud, 		/* v34, 3000 baud */
	v34_3200_baud, 		/* v34, 3200 baud */
	v34_3429_baud, 		/* v34, 3429 baud */
};
enum{
	v56_32000_bps=0,
	v56_34000_bps=2,
	v56_36000_bps=4,
	v56_38000_bps=6,
	v56_40000_bps=8,
	v56_42000_bps=10,
	v56_44000_bps=12,
	v56_46000_bps=14,
	v56_48000_bps=16,
	v56_50000_bps=18,
	v56_52000_bps=20,
	v56_54000_bps=22,
	v56_56000_bps=24,
	v56_58000_bps=26,
	v56_60000_bps=28,
};
enum{
	v90_28000_bps,	/* bps = 28000 + (4000*speed)/3 */
	v90_29333_bps,
	v90_30666_bps,
	v90_32000_bps,
	v90_33333_bps,
	v90_34666_bps,
	v90_36000_bps,
	v90_37333_bps,
	v90_38666_bps,
	v90_40000_bps,
	v90_41333_bps,
	v90_42666_bps,
	v90_44000_bps,
	v90_45333_bps,
	v90_46666_bps,
	v90_48000_bps,
	v90_49333_bps,
	v90_50666_bps,
	v90_52000_bps,
	v90_53333_bps,
	v90_54666_bps,
	v90_56000_bps,
};

/* asic buffer size */
enum{
	FIXED32,
	FLEX32,
	FLEX64,
	FLEX128
};

/* codec type */
enum{
	SGS,
	DELTA,
	SILICON
};

/*	cmdCode definition */
enum{
	IDLE,
	LOOP3,
	START_UP,				
	RETRAIN,
	RE_RETRAIN,				
	TONE_GEN_AND_DET,	/* cmdParam1(hi byte): # of tones to be generated. */
						/* cmdParam1(lo byte): # of tones to be detected. */
						/* 350-600 bandpass in default. */
	DIAL_CHAR,			/* cmdParam1 : CHAR to be dialed */
						/* cmdParam2 : 1:char sent 0: not sent */
						/* dtmfLevel[0]/[1]: low band/high band  */
						/* dtmfDuration: in 10 ms unit */
						/* dtmfInterDigitDelay in 10 ms unit */
	DTMF_DETECTION,		/* */
	CALLER_ID,			/* unused!! */
	VOICE,				/* for voice vxd */
	SPECTRUM_ANA,		/* for board spectrum analysis */
						/* cmdParam1: 150*n signal frequency( in Hz) to be sent */
						/* n = [0, 25], n=0: no signal to be sent  */
	PULSE_DIAL,			/* for pulse dialing */
};					  

/* for START_UP/LOOP3/RETRAIN/RE_RETRAIN cmds */
/* v17 case:  		cmdParam1: 1/0 short/long train */
/* v32 case:		cmdParam2: 1/0 low/high speed mode */

/*----------------------------------------------------- */
/*   cmdParam3 bit */
/*-----------------------------------------------------  */
#define	AUTO_MODE					0x0001
#define	GUARD_TONE_550				0x0002
#define	GUARD_TONE_1800				0x0004
#define	AUTO_RETRAIN				0x0008
#define	AUTO_FALLBACK				0x0010
#define	FAST_HANG_UP				0x0020 
#define CID_DET						0x0040
#define	VID_DET						0x0080
#define VID_SND_AM_ANSWER			0x0100
#define CNG_ENABLE					0x0200
#define EUROPE_CPF_ENABLE			0x0400
#define V34_RATE_INITIATE			0x0800
#define	V34_CI_ENABLE				0x1000
#define	V34_HIGH_CONNECT_RATE		0x2000		/* for WHQL test only		 */
#define	V8BIS_ENABLE				0x4000
#define	A_LAW_ENABLE				0x8000
	
/*----------------------------------------------------- */
/*   cmdParam4 bits */
/*-----------------------------------------------------  */
#define V90_DPCM_MODE				0x0001
#define SILICON_DC_RESET            0x0002
#define AUSTRALIA_CASE              0x0004
#define SILICON_LC_RESET            0x0008
#define SILICON_LC_INIT             0x0010
#define SILICON_REG_READ            0x0020
#define SILICON_REG_WRITE           0x0040
/*----------------------------------------------------- */
/*   DSP Status Bit */
/*----------------------------------------------------- */
#define	CTS_BIT						0x0001
#define	CD_BIT						0x0002
#define	RETRAIN_DETECTED			0x0004
#define	TRAIN_FAILURE				0x0008		
#define CLEAR_DOWN_BIT				0x0010
#define DIAL_CHAR_DONE				0x0020
#define	DTMF_DETECTED_BIT			0x0040
#define	V27_4800_DATA_ON			0x0080 
#define V32_INPROCESS               0x0100 
#define DSR_BIT						0x0200
#define V34_INPROCESS				0x0400
#define	SPECTRUM_ANA_DONE			0x0800
#define V34_RATE_RESPONSE			0x1000	
#define V34_DSP_RATE_INITIATE		0x2000
/*----------------------------------------------------- */
/*   DSP Status Mask Bit */
/*-----------------------------------------------------  */
#define		CTS_CLEAR				0xfffe
#define		CD_CLEAR				0xfffd 
#define		DSR_CLEAR				0xfdff
#define		V27_4800_DATA_OFF		0xff7f
/*----------------------------------------------------- */
/*   Tone Status Bit */
/*----------------------------------------------------- */
#define TONE0_DET					0x0001
#define TONE1_DET					0x0002
#define TONE2_DET					0x0004
#define TONE3_DET					0x0008
#define TONE4_DET					0x0010
#define TONE5_DET					0x0020
#define TONE6_DET					0x0040
#define TONE7_DET					0x0080
#define TONE8_DET					0x0100
#define TONE9_DET					0x0200
													/* v8 org mode det */
#define REGULAR_ANSWER_DETECTED		0x0400			
#define	AM_ANSWER_DETECTED			0x0800          				
#define	VID_TONE_1650_DETECTED		0x1000
#define	VID_TONE_2225_DETECTED		0x2000
													/* v8 ans mode det */
#define	VID_TONE_1100_DETECTED		0x1000
#define	VID_TONE_1300_DETECTED		0x2000
#define	VID_TONE_1800_DETECTED		0x4000

#define CPT_DET						0x8000
 
enum{
	MINUS_0_DBM,
	MINUS_1_DBM,
	MINUS_2_DBM,
	MINUS_3_DBM,
	MINUS_4_DBM,
	MINUS_5_DBM,
	MINUS_6_DBM,
	MINUS_7_DBM,
	MINUS_8_DBM,
	MINUS_9_DBM,
	MINUS_10_DBM,
	MINUS_11_DBM,
	MINUS_12_DBM,
	MINUS_13_DBM,
	MINUS_14_DBM,
	MINUS_15_DBM,
	MINUS_FF_DBM=0xff,
};

/* country */
#define CTRY_USA			1
#define CTRY_FRANCE			2
#define	CTRY_GERMANY		3
#define CTRY_ITALY			4
#define	CTRY_SWEDEN			5
#define	CTRY_UK				6
#define CTRY_JAPAN			7
#define	CTRY_AUSTRALIA		8
#define	CTRY_SPAIN			9
#define CTRY_TAIWAN			10
#define CTRY_SINGAPORE		11
#define CTRY_KOREA			12
#define	CTRY_SWITZERLAND	13
#define CTRY_NORWAY			14
#define CTRY_NETHERLANDS	15
#define	CTRY_BELGIUM		16
#define	CTRY_CANADA			17
#define	CTRY_IRELAND		18
#define	CTRY_PORTUGAL		19
#define	CTRY_POLAND			20
#define	CTRY_HUNGARY		21
#define CTRY_FINLAND		22
#define	CTRY_DENMARK		23
#define	CTRY_AUSTRIA		24
#define CTRY_SAFRICA		25
#define CTRY_TBR21			26
#define CTRY_CHINA			27
#define CTRY_MALAYSIA		28
#define CTRY_LUXUMBURG		29
#define CTRY_GREECE			30
#define CTRY_ICELAND		31
#define CTRY_NEWZEALAND		32
#define CTRY_BRAZIL			33


/*****************************************************************
** DSP <---> Controller shared RAM (declared in Controller part)
******************************************************************/
typedef struct{ 
	USint16			cmdCode; 		/* command from controller(C->D) */
	USint16			cmdParam1;		/* command parameters(C->D)  */
	USint16			cmdParam2;		/* 0:high speed, 1: low speed(<=2400) (C->D)  */
	USint16			cmdParam3;		/* b0: 0/1: auto mode disable/enable */
									/* b2/b1: for guard tone */
									/* 	00: no guard tone */
									/* 	01: 550 Hz */
									/* 	10: 1800 Hz */
									/* b3: 0/1: auto retrain disable/enable  */
									/* b4: 0/1: auto fallback disable/enable */
									/* b5: 0/1: fast hang up disable/enable */
									/* b6: 0/1: caller ID det off/on */
									/* b7: enable vid detection */
									/* b8: send am-answer tone */
									/* b9: CNG enable */
									/* b10: select Europe call progress filter */
									/* b11: enable v34 rate initatiation */
									/* b12: enable CI generation */
									/* b13: 2400 bps higher v34 connection rate for WHQL test */
									/* b14: v8bis enable */
									/* b15: alaw enable  */
	USint16			asicOutport;	/* bits assignment */
									/* b0: offHook */
									/* b1: CID_Relay */
									/* b2: AFE Reset Bar */
									/* b3: Mute Bar */
									/* b4: Power Down Mode(ST7546) */
									/* b5: Handset */
									/* b6: Codec Mode select(ST7546) */
									/* b7: Reserved  */
	USint16			asicInport;		/* b0: ring */
									/* b1: loop-current(or handset detected) */
									/* b2: loop-voltage */
	USint16			asicStatus;
	Boolean			orgMode;		/* 1: org, 0: ans(C->D) */
	int				modem;			/* modem type(C->D) */
	USint32			speed;			/* speed in each modem type(C->D) */
									/* v34 case: */
									/* byte 0: tx data rate */
									/* byte 1: tx baud rate */
	int				finalModem;		/* final negotiated modem(D->C) */
	USint32			finalSpeed;		/* final negotiated speed(D->C)  */
									/* v34 case: */
									/* byte 0: tx data rate */
									/* byte 1: tx baud rate */
									/* byte 2: rx data rate */
									/* byte 3: rx baud rate */
	USint16			dspStatus; 		/* data status to controller(D->C) */
	USint16			roundTripDelay; /* in unit of 10 ms(D->C) */
	int				version;		/* version number(C->D) */
	int				txLevel;		/* tx level, in -dBm unit(C->D) */
	int				rxLevel;		/* rx level, in -dBm unit(D->C) */
	int				dtmfTxLevel[2];	/* DTMF tx level, in -dBm unit(C->D) */
	int				dtmfDuration;	/* in 10 ms unit(C->D) */
	int				dtmfInterDigitDelay; /* in 10 ms unit(C->D) */
	char			detectedDTMF;	/* detected DTMF(for DTMF detection)(D->C) */
	int				toneGenFreq[MAX_TONE_NUM];	/* in Hz unit(C->D) */
	Boolean			tonePhaseReverse; /* for answer tone only */
	int				toneGenLevel[MAX_TONE_NUM]; /* in -dBm unit(C->D) */
	int				toneDetFreq[MAX_TONE_NUM];	/* 	in Hz unit(C->D)  */
	int				toneDetThreshold[MAX_TONE_NUM]; /* in -dBm unit */
									 /* the last one is for call progress det thrreshold */
	USint16			toneDetStatus;	/* tone detection status(D->C) */

	USint16			localRetrain;	/* locally initiated retrain count(D->C) */
	USint16			localReneg;		/* locally initiated reneg count(D->C) */
	USint16			remoteRetrain;	/* remotely initiated retrain count(D->C) */
	USint16			remoteReneg;	/* remotely initiated reneg count(D->C) */
	USint16			trainUpCount;	/* train up count (v34/v90 only)(D->C) */
	USint16			trainDownCount;	/* train down count (v34/v90 only)(D->C) */
	int16			actTxLevel;		/* actual tx level, in -dBm unit(D->C) */
	USint32			pcmPreRbs;		/* v90 RBS before the pad (D->C) */
	USint32			pcmPad;			/* v90 Pad (D->C) */
	USint32			pcmPostRbs;		/* v90 RBS after the pad (D->C) */
	unsigned char	filler0[100];	/* filler for backward compatibility */
	unsigned char	checkCurrent;   /* Silicon Lab  */
	unsigned char	lineCurrent;    /* Silicon Lab */
	float			sigP;			/* signal power */
	float			noiseP;			/* noise power */
	
	USint32			cmdParam4;		/* command parameters (C->D) */
	USint32			cmdParam5;		/* command parameters (C->D) */
	USint32			maxRxPower;		/* max V90 rx level, 0-31, Follow sever if > 31. (C->D) */
   	USint32			filler1[29];	/* for backward compatibility //### */
	USint32         CountryCode;    /* country code setting */
	USint32         SiRegValue;     /* for Silicon DAA international version //### */
	USint32			rxFrameCnt;		/* Count of good HDLC frames (C->D) */
	USint32			rxErrFrameCnt;	/* Count of bad frames (C->D) */
   	USint32			errFreeSec;		/* time since last error (in microsecs) (D->C) */
   	USint32			avgErrFreeSec;	/* average time between errors (in microsecs) (D->C) */
	USint32			SiNumReg;		/* for Silicon DAA international version */
	USint32			SiRegValue1[4];	/* for Silicon DAA international version */
   	USint32			filler2[85];	/* for backward compatibility */
	int				fastestModem;	/* (D->C) fastest allowed modem type */
	int				cpuUtilization;	/* (C->D)max cpu utilization */
									/* 0: 100 % */
									/* 1-9: 10% - 90 % (default 7) */
	USint32			loopPeriod;		/*  modem interrupt period in micro second unit */
	USint16			underRunCnt;
	USint16			overRunCnt;
	unsigned char	vmodemDebugStr[VMODEM_DEBUG_STRING_SIZE];
	int				bufferType;
	int				codecType;
	int				filler3;
	unsigned char	txDataBuf[MAX_DATA_BUFFER_SIZE];/* TX Data Buffer(C->D) */
	unsigned char	rxDataBuf[MAX_DATA_BUFFER_SIZE];/* pascal string,RX Data Buffer(D->C) */
  fPoint			eyePat[EYE_PATTERN_SIZE]; /* for eye-pattern display */
}modemShared,*modemSharedPtr,**modemSharedHand; 

#endif
