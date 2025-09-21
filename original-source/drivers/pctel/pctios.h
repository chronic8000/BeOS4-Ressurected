/*********************************************************
* PCTIOS.H - A HSP MODEM IO Service Header               *
* Copyright (c) 1999 By PCTEL INC                        *
*********************************************************/
#ifndef PCTIOS_H
#define PCTIOS_H

#ifndef __GNU__
#ifdef WIN32_VXD
#define FAR
#define STDCALL _cdecl /*_stdcall*/     
#else /* WIN16 */
#define FAR far
#define STDCALL
#define VXDINLINE _inline
#endif

#else	/* ifndef __GNU__ */
#define FAR
#define STDCALL
#define VXDINLINE inline
#endif

/* definition of basic types */
typedef unsigned int USINT;
typedef unsigned short USSHORT;
typedef unsigned short FAR * LPSHORT;
typedef unsigned long FAR * LPLONG;
typedef unsigned long USLONG;
typedef void FAR * LPVOID;

/* definition of hardware configuration */
#define PCT_HW1010		0x10 /* PCT101 + ST7546 */
#define PCT_HW2880		0x20 /* PCT288 + ST7546 */
#define PCT_HW3880		0x30 /* PCT388 + ST7546 */
#define PCT_HWA970		0x50 /* PTAC97 + ST7546 */
#define PCT_HWA971		0x51 /* PTAC97 + SI3032 (pc-chips) */
#define PCT_HW7890		0x70 /* PCT789 + ST7546 */
#define PCT_HW7891		0x71 /* PCT789 + SI3032 */	
#define PCT_HW8880		0x80 /* reserved */
#define PCT_HW9990		0x90 /* reserved */

#define F_VARLEN		0x1000  /*  indicate a variable length frame */

#define PCT388_INDXP 	0x02 /* PC-TEL 388 ASIC index port */
#define PCT388_DATAP 	0x00 /* PC-TEL 388 ASIC data port */
#define PCT789_INDXP 	0x04 /* PC-TEL 789 ASIC index port */
#define PCT789_DATAP 	0x00 /* PC-TEL 789 ASIC data port */
#define PCTAC97CM_INDXP	0x00 /* PC-CHIPS/CMEDIA ASIC index port */
#define PCTAC97CM_DATAP	0x00 /* PC-CHIPS/CMEDIA ASIC data port */

/* PCTEL ASIC Defintions */
#define PCTASIC_BUFSIZE 0x0018
#define PCTASIC_IRQEN   0x0040
#define PCTASIC_START   0x0020
#define PCTASIC_SKIP    0x0010
#define PCTASIC_TXEN    0x0008
#define PCTASIC_ISEL2   0x0004
#define PCTASIC_ISEL1   0x0002
#define PCTASIC_ISEL0   0x0001
#define PCTASIC_CFGINIT (PCTASIC_IRQEN+PCTASIC_TXEN+0x80)

#define PTESC_CMD_OHD		0x1		/* Offhook disable */
#define PTESC_CMD_OHE		0x2		/* Offhook enable */
#define PTESC_CMD_CID		0x4		/* Caller ID enable */
#define PTESC_CMD_RLC		0x8		/* Read LineCurrent */
#define PTESC_CMD_PDL_RST	0x10	/* Power Down Line Side (low power) */
#define PTESC_CMD_PDL_INIT	0x20	/* Power Down Line Side (normal operation) */
#define PTESC_CMD_WRITE		0x40	/* Write SI Command Register */
#define PTESC_CMD_READ		0x80	/* Read SI Status Register */

/* PCTEL ASIC flags for PtASICRdWrEscape() */
#define PTFLAG_CID			0x01	/* caller ID */
#define PTFLAG_RLC			0x02   	/* read line current */
#define PTFLAG_READ			0x04	/* read status register */
#define PTFLAG_END			0x08   	

/* PCTEL CODEC Definitions */
#define AC97_CODEC_READ_INDEX		0x01
#define PCT789_CODEC_READ_INDEX		0x02

#define	OUTR_OFHR			0x01
#define	OUTR_CIDR			0x02
#define	OUTR_AFERN			0x04
#define	OUTR_MUTE			0x08 
#define	OUTR_POWER_DOWN		0x10
#define	OUTR_HANDSET		0x20
#define	OUTR_CODEC_MODE     0x40
#define	OUTR_RESERVED		0x80


/* PtASICInit() definitions */
enum AsicTypes
{ 
	PCT1010 = 0,			/* PCTEL ASIC I   (ISA) */
	PCT288	= 2,			/* PCTEL ASIC II  (ISA) */
	PCT388	= 3,			/* PCTEL ASIC III (ISA) */
	PCT789	= 7,			/* PCTEL ASIC 7   (PCI) */
	PCTAC97 = 77,			/* PCTEL AC97     (PCI) */
	PCT899	= 8,            /* PCTEL reserved       */
	PCT999	= 9,            /* PCTEL reserved       */
};

enum CodecTypes
{ 
	C_ST7546= 0,            /* CODEC: ST7546        */
	C_SI3032= 1,			/* CODEC: SI_LAB 3032   */
	C_SI3034= 2,			/* CODEC: SI_LAB 3034   */
	C_ST7550= 3,			/* CODEC: ST7550        */
};

enum AsicCtrlRegs
{ 
	CTRLREG0 = 0,
	CTRLREG1 = 1,
	CTRLREG2 = 2,
	CTRLREG3 = 3,
	CTRLREG4 = 4,
	CTRLREG5 = 5,
	CTRLREG6 = 6,
	CTRLREG7 = 7,
};

/************* ASIC IO exports **************************/

extern USSHORT gPctelRegMap[];
extern USSHORT gPctelInitcmd[];
extern USSHORT gPctelDeinitcmd[];
extern USSHORT gPctelStartcmd[];
extern USSHORT gPctelClockDiv[];
extern USSHORT gPctelCmdWord[];

/************* ASIC IO API **************************/

/* init PC-TEL asic, codec, register maps & IO port address, 
   must call before any ASIC IO access.
   Parameters:
   asic   - choose one of AsicTypes (defined above)  
   codec  - choose one of CodecTypes (defined above)  
   regMap - pointer to an array of register mapping
   base   - specify the IO port address
   indexOffset - offset of PCTEL ASIC index port
   dataOffset  - offset of PCTEL ASIC data port */
void STDCALL PtASICInit(USINT asic, USINT codec, LPSHORT regMap,
	USINT base, USINT indexOffset, USINT dataOffset, USINT codecReadIndex);

/* get data from PC-TEL ASIC */
void STDCALL PtASICReadReg(USINT reg, LPSHORT data, USINT count);
/* put data into PC-TEL ASIC; if data==NULL, put zeros into ASIC */
void STDCALL PtASICWriteReg(USINT reg, LPSHORT data, USINT count);
/* send/recv data to PC-TEL ASIC; if mode==0, data mode; otherwise command mode */
void STDCALL PtASICRdWrReg(USINT reg, LPSHORT idata, LPSHORT odata, USINT count, USINT mode);

/* send formated data into PC-TEL ASIC 
   the data stream is in the format of {reg[len]data}...{-1}
   reg <= 255 - one 16-bit data follows.
   reg == F_VARLEN+(0..255) 
              - variable length data follows, 
   		        the first 16-bit data is the length of data.
   reg == -1  - end of data, no more data follows.
*/
void STDCALL PtASICWriteRegEx(LPSHORT data);


/************* DSP API **************************/

/* called by DSP to restart ASIC */
void STDCALL PtASICRestart(void);

/* called by DSP to read External Output Register of ASIC.
   The read value is saved at variable *data */
void STDCALL PtASICRdInport(LPSHORT data);

/* called by DSP to write External Output Register of ASIC. 
   The value of *data will be write to ASIC */
void STDCALL PtASICWrOutport(LPSHORT data);

/* called by DSP to set the desired ASIC buffer size */
void STDCALL PtASICSetBufferSize(USINT size);

/* called by DSP to set the desired sample frequency */
void STDCALL PtASICSetSampleFreq(USINT fs);

/* called by DSP to insert the special command words */
void STDCALL PtASICRdWrEscape(USINT flags, LPVOID inPtr,
							  LPVOID outPtr, int length);

#if 1	/* CM8338 */
#define PCT_HWTYPE PCT_HW7891
#else	/* CM8738 */
#define PCT_HWTYPE PCT_HWA971
#endif


#endif /* PCTIOS_H */
