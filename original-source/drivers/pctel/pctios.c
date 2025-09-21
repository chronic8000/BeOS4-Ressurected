/*********************************************************
* PCTIOS.C - A HSP MODEM IO Service                      *
* Copyright (c) 1999 By PCTEL INC                        *
* 04/04/99: tyin created                                 *
* 04/20/99: support DOS                                  *
* 04/21/20: support WIN95                                *
*********************************************************/
/*#define WIN32_VXD */	/* defined when build for Win95 */

//#define BEOS

#ifndef __GNU__
#ifdef WIN32_VXD

#define WANTVXDWRAPS
#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vwin32.h>
#include <winerror.h>
#include <vxdwraps.h>

#else /* DOS */

#include <stdio.h>
#include <conio.h>

#endif /* WIN32_VXD */

#else	/* ifndef __GNU__ */

#include <stdio.h>
#ifdef __BEOS__
#include <PCI.h>
#else
#include <asm/io.h>
#endif
#endif	/* ifndef __GNU__ */

#include "pctios.h"
/* PCT_HWTYPE is defined in pctios.h */

#define PORT_IIO			0		/* indirect IO */
#define PORT_DIO			1   	/* direct IO   */
#define PORT_DMA			2   	/* DMA IO      */
#define PTIO_DELAY			3

#ifdef WIN32_VXD
/* lock code and data segment in WIN32 VxD */
#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG
#endif

#ifndef __GNU__
#define GETASIC(port)  		i86_inpw(port)
#define PUTASIC(port,data)  i86_outpw(port,data)
#else
#ifndef __BEOS__	/* BeOS port */
#define GETASIC(port)  		inw(port)
#define PUTASIC(port,data)  outw(data, port);
#else
extern pci_module_info		*pci;  //declared in zz.c
#define GETASIC(port)		((*pci->read_io_16)(port))
#define PUTASIC(port, data)	((*pci->write_io_16)(port , data))
#endif
#endif

typedef struct PTIOS_ST
{
	USINT	portBase;
	USINT	portMode;
	USINT	portData;
	USINT	portIndex;
	USINT	asicType;
	USINT	codecType;
	USINT	asicFlags;
	USINT	asicCmdCnt;
	USSHORT	asicCmd[16];
	USSHORT	ctrlReg[8];
	LPVOID	asicIOPtr;
	USINT	asicIODelay;
  	USINT	codecReadIndex;
} PTIOS; 

static PTIOS ptio = {0};
USSHORT ASICDataTx[PCTASIC_BUFSIZE] = {0};
USSHORT ASICDataRx[PCTASIC_BUFSIZE] = {0};

enum {
    /* ASIC control register bit mask */
    ASIC_CTRL_SLEEP	= 0x8000,
    ASIC_CTRL_AFEPDN	= 0x4000,
    ASIC_CTRL_XIRQEN	= 0x2000,
    ASIC_CTRL_XIRQPOL	= 0x1000,
    ASIC_CTRL_AFERST	= 0x0800,
    ASIC_CTRL_AFEHC0	= 0x0400,
    ASIC_CTRL_LPBMD1	= 0x0200,
    ASIC_CTRL_LPBMD0	= 0x0100,
    ASIC_CTRL_NTORST	= 0x0080,
    ASIC_CTRL_ENIRQ	= 0x0040,
    ASIC_CTRL_START	= 0x0020,
    ASIC_CTRL_RESERVED1	= 0x0010,
    ASIC_CTRL_ENTX	= 0x0008,
    ASIC_CTRL_RESERVED2	= 0x0004,
    ASIC_CTRL_RESERVED3	= 0x0002,
    ASIC_CTRL_RESERVED4	= 0x0001
};

enum {
    afeCFG = ASIC_CTRL_ENIRQ | ASIC_CTRL_ENTX | ASIC_CTRL_NTORST |
	     ASIC_CTRL_AFEPDN | ASIC_CTRL_AFERST
};


/******* ASIC command string ****************************
 To build your driver, first set PCT_HWTYPE to your
 hardware configuration and then fill the following
 variables gPctelRegMap, gPctelInitcmd and
 gPctelStartcmd to its right values.
 ******* ASIC command string ****************************/
#if (PCT_HWTYPE == PCT_HWA971)		/* pc_chips CM8738 */
	USSHORT gPctelRegMap[8] = {0,4,8,12,16,20,24,28};
	USSHORT gPctelInitcmd[] = {
    	CTRLREG3|F_VARLEN, 2, 0x00, 0xC0, /* ClockDivider */
    	CTRLREG2|F_VARLEN, 2, 0x00, 0x15, /* AFE Reset+OnHook */
    	CTRLREG1|F_VARLEN, 2, PCTASIC_START,0x00, /* START */
    	CTRLREG4,             PCTASIC_BUFSIZE,    /* BufSize */
    	CTRLREG0|F_VARLEN,48, 0x00, 0x00, /* SILAB3032 command */
    						  1, 0x0A00, 1, 0x0700, 1, 0x0803, 1, 0x0922,	
    						  1, 0x0600, 1, 0x0203, 1, 0x0502, 1, 0x0D00,
							  1, 0x1008,
		/* FLUSH Buffers */
	  						  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	  						  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	  						  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		/* START and STOP, INT ENABLE */
    	CTRLREG1|F_VARLEN, 5, PCTASIC_START,0x00, 
    	 					  PCTASIC_CFGINIT,PCTASIC_CFGINIT|PCTASIC_START,
							  PCTASIC_CFGINIT&~PCTASIC_START,
    	(USSHORT)-1
    };
#if 0
	USSHORT gPctelDeinitcmd[] = { (USSHORT)-1 }; /* to be done */
    USSHORT gPctelStartcmd[] = {
    	CTRLREG1|F_VARLEN, 5, PCTASIC_START,0x00, 
    	 					  PCTASIC_CFGINIT,PCTASIC_CFGINIT|PCTASIC_START,
							  PCTASIC_CFGINIT&~PCTASIC_START,
    	(USSHORT)-1
    };
#endif
	USSHORT gPctelDeinitcmd[] = {
    	CTRLREG0|F_VARLEN,4 , 0x00, 0x00,
    						  1, 0x0608,
    	CTRLREG2, 			  0x05, 			/* Power down AFE+OnHook */
    	CTRLREG4,             PCTASIC_BUFSIZE,	/* BufSize */
    	CTRLREG1,             0x08,			   	/* Disable interrupt; Disable Timeout Reset */
    	(USSHORT)-1
    };
	USSHORT gPctelClockDiv[16] = {
		0x00C0,0x0922, 0x00C0,0x0989, 0x00C0,0x0956,
		0x00C0,0x0934, 0x00C0,0x0923, 0x00C0,0x0969,
		0x00C0,0x0922, 0x00C0,0x0922
	};
	USSHORT gPctelCmdWord[16] = {
		0x0500,0x0502, 0x050A,0x2C00, 
		0x0690,0x0680, 0x0000,0x0000 
	};
#elif (PCT_HWTYPE == PCT_HW7891)	/* pc_chips CM8338 or PCI */
    USSHORT gPctelRegMap[8] = {0,1,2,3,4,5,6,7};
	USSHORT gPctelInitcmd[] = {
	  CTRLREG2,				0x0030,
	  CTRLREG1,				0x0000,
	  CTRLREG3,				0x2018,
	  CTRLREG5,				0x0080,
	  CTRLREG2,				0x3C01,
	  CTRLREG1|F_VARLEN, 2,	ASIC_CTRL_AFERST | ASIC_CTRL_START,
							ASIC_CTRL_AFERST,
	  CTRLREG0|F_VARLEN,48, 0x00, 0x00,
    						1, 0x0A00, 1, 0x0700, 1, 0x0801, 1, 0x0922,	
    						1, 0x0680, 1, 0x0203, 1, 0x0502, 1, 0x0D00,
	  						1, 0x1008,
	  /* FLUSH Buffers */
	  						0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	  						0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	  						0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	  						0x0, 0x0, 0x0, 0x0,
	  /* START and STOP, INT ENABLE */
	  CTRLREG1|F_VARLEN, 2,	ASIC_CTRL_AFERST | ASIC_CTRL_START,
							ASIC_CTRL_AFERST,
	  CTRLREG1|F_VARLEN, 2,	afeCFG | ASIC_CTRL_START,
							afeCFG & ~ASIC_CTRL_START,
	  (USSHORT)-1
    };
	USSHORT gPctelDeinitcmd[] = {
    	CTRLREG0|F_VARLEN,4 , 0x00, 0x00,
    						  1, 0x0608,
    	CTRLREG2,             0x0801,
    	CTRLREG3,             0xA018, 
    	CTRLREG1,             0x7C80,
    	(USSHORT)-1
    };
	USSHORT gPctelClockDiv[16] = {
		0x0080,0x0922, 0x0080,0x0989, 0x0080,0x0956,
		0x0080,0x0934, 0x0080,0x0923, 0x0080,0x0969,
		0x0080,0x0922, 0x0080,0x0922
	};
	USSHORT gPctelCmdWord[16] = {
		0x0500,0x0502, 0x050A,0x2C00, 
		0x0610,0x0680, 0x0000,0x0000 
	};
  
#else
#error PCT_HWTYPE not defined!!!
#endif


/******* ASIC API ****************************/

#ifndef __GNU__
USSHORT VXDINLINE STDCALL i86_inpw(USINT reg) 
{
	USSHORT data;

#ifdef WIN32_VXD
	_asm mov	edx, reg
#else /* WIN16_DOS */
	_asm mov	dx, reg
#endif /* WIN32_VXD */
	_asm in		ax, dx
	_asm mov	data, ax

	return data;
}
#endif

#ifndef __GNU__
USSHORT VXDINLINE STDCALL i86_outpw(USINT reg, USSHORT data)
{
#ifdef WIN32_VXD
	_asm mov	edx, reg
#else /* WIN16_DOS */
	_asm mov	dx, reg
#endif /* WIN32_VXD */
	_asm mov	ax, data
	_asm out	dx, ax

	return data;
}
#endif


/* init PC-TEL asic, codec, register maps and IO port address */
void STDCALL PtASICInit(USINT asic, USINT codec, LPSHORT regMap,
	USINT base, USINT indexOffset, USINT dataOffset, USINT codecReadIndex)
{
	USINT i = 0;

	ptio.asicType = asic;  
	ptio.codecType = codec; 
	if (regMap)
	{	for (i=0; i<8; i++)
			ptio.ctrlReg[i] = *regMap++;
	}
	else /* use default register mapping */
	{	for (i=0; i<8; i++)
			ptio.ctrlReg[i] = i;
	}
	ptio.portBase = base;
	ptio.portMode = ((indexOffset==dataOffset)?PORT_DIO:PORT_IIO);		
	ptio.portIndex = base+indexOffset;
	ptio.portData = base+dataOffset;
	ptio.codecReadIndex = codecReadIndex;
	/* init variables to zero */
	ptio.asicFlags = 0;  
	ptio.asicCmdCnt = 0; 
	ptio.asicIODelay = 0;
}

/* get data from PC-TEL ASIC */
void STDCALL PtASICReadReg(USINT reg, LPSHORT data, USINT count)
{
	if (ptio.portMode == PORT_IIO)
	{ 	/* indirect IO */
		PUTASIC(ptio.portIndex, ptio.ctrlReg[reg]);
		reg = ptio.portData;
	}
	else /* direct IO */   
	{	reg = ptio.portBase + ptio.ctrlReg[reg];
	}
	
	/* recv data from ASIC */
	while (count > 0) {
		*data++ = GETASIC(reg);
		count--;
	}
}

/* put data into PC-TEL ASIC */
void STDCALL PtASICWriteReg(USINT reg, LPSHORT data, USINT count)
{
	if (ptio.portMode == PORT_IIO)
	{ 	/* indirect IO */
		PUTASIC(ptio.portIndex, ptio.ctrlReg[reg]);
		reg = ptio.portData;
	}
	else /* direct IO */   
	{ 	reg = ptio.portBase + ptio.ctrlReg[reg];
	}
	
	/* send data to ASIC */
	if (data)
	{   while (count > 0) {
			PUTASIC(reg, *data++);
			count--;
		}
	}
	else
	{   /* send zero to ASIC */
		while (count > 0) {
			PUTASIC(reg, 0);
			count--;
		}
	}
}

/* send/recv data to PC-TEL ASIC */
void STDCALL PtASICRdWrReg(USINT reg, LPSHORT idata, 
	LPSHORT odata, USINT count, USINT mode)
{
	LPSHORT pIn, pOut;
	USINT i;

	/* remove last-significant-bit from data */
	if (mode == 0)
	{	pOut = odata;	
		for (i=0; i<count; i++)
			*pOut++ &= 0xFFFE;
	}
	
	/* insert ASIC command words if any */
	pOut = odata;
	pIn  = ptio.asicCmd;
	i = ptio.asicCmdCnt;
	while (i > 0)
	{	/* specify command mode and insert command */
		*pOut++ |= 0x01;
		*pOut++ = *pIn++;
		i--;
	}
	ptio.asicCmdCnt = 0; /* clear after inserted */
	
	/* setup ASIC register */
	if (ptio.portMode == PORT_IIO)
	{ 	/* indirect IO */
		PUTASIC(ptio.portIndex, ptio.ctrlReg[reg]);
		reg = ptio.portData;
	}
	else /* direct IO */   
	{ 	reg = ptio.portBase + ptio.ctrlReg[reg];
	}
	
	/* send/recv data to ASIC */
	pOut = odata;
	pIn  = idata;
	while (count > 0) {
		PUTASIC(reg, *pOut++);
		*pIn++ = GETASIC(reg);
		count--;
	}

	/* read line current */
	if (ptio.asicFlags & PTFLAG_RLC)
	{	ptio.asicIODelay--;
		if (ptio.asicIODelay == 0)
		{	/* clear PTFLAG_RLC after reading line current */
			ptio.asicFlags &= ~PTFLAG_RLC;
			*(LPSHORT)ptio.asicIOPtr = (USSHORT)(0x80 | (idata[ptio.codecReadIndex]&0x0F));
		}
	}
	else if (ptio.asicFlags & PTFLAG_READ)
	{	ptio.asicIODelay--;
		if (ptio.asicIODelay == 0)
		  {	/* clear PTFLAG_READ after reading status */
			ptio.asicFlags &= ~PTFLAG_READ;
			*(LPLONG)ptio.asicIOPtr = (0x80000000 | idata[ptio.codecReadIndex]);
		}
	}

}

/* put formated data into PC-TEL ASIC */
void STDCALL PtASICWriteRegEx(LPSHORT data)
{
	USSHORT count;
	USINT reg;
	
	reg = *data++;
	while (reg != (USSHORT)-1) 
	{   
		count = (USSHORT)((reg&F_VARLEN) ? (*data++) : 1);
		if (ptio.portMode == PORT_IIO)
		{ 	/* indirect IO */
			PUTASIC(ptio.portIndex, ptio.ctrlReg[(reg&0xFF)]);
			reg = ptio.portData;
		}
		else /* direct IO */   
		{ 	reg = ptio.portBase + ptio.ctrlReg[(reg&0xFF)];
		}
	
		while (count > 0) 
		{
			PUTASIC(reg, *data++);
			count--;
		}
		reg = *data++;
	}
}


/******* DSP API ****************************/

/* called by DSP to restart ASIC */
void STDCALL PtASICRestart(void)
{
	USSHORT tdata;

    tdata = 0x00C8;					/* NOTIMEOUT+IRQEN+TXEN */
	if (ptio.asicType == PCT789)
		tdata |= 0x4800;
#if 0
    tdata |= 0x0020; 				/* add START bit */ /* whsu */
	PtASICWriteReg(CTRLREG1, &tdata, 1);
    tdata &= ~0x0020; 				/* remove START bit */
    PtASICWriteReg(CTRLREG1, &tdata, 1);
#else
	PtASICWriteReg(CTRLREG1, &tdata, 1);
    tdata |= 0x0020; 				/* add START bit */
    PtASICWriteReg(CTRLREG1, &tdata, 1);
#endif
}

/* called by DSP to read External Input Register of ASIC */
void STDCALL PtASICRdInport(LPSHORT data)
{
	USSHORT tdata;     
	
	PtASICReadReg(CTRLREG2, &tdata, 1); /* read RING status */
	*data = tdata;
}

/* called by DSP to write External Output Register of ASIC */
void STDCALL PtASICWrOutport(LPSHORT data)
{
	USSHORT asicCmdCID=0;
	USSHORT tdata;

	tdata = *data;
	if ((ptio.codecType == C_SI3032) ||
		(ptio.codecType == C_SI3034))
	{	
		if (ptio.asicType == PCT789)
		  {
			/* add the magic number */
			/* Make sure CODEC PowerDown on PCT789T-A (bit 4) is off */
			tdata = 0x3C00 | (*data & (OUTR_OFHR+OUTR_CIDR+OUTR_MUTE));
			if (*data & OUTR_OFHR)
			  tdata |= 0x0020;			/* off-hook is bit 5 on PCT789T-A */
			if (*data & OUTR_HANDSET)
			  tdata |= 0x0004;			/* handset relay is bit 2 on PCT789T-A */
		  }

#if 0	/* Valid for PCT288 only */
		if (!(tdata & OUTR_AFERN))
		  tdata ^= OUTR_AFERN; 	/* ignore AFE reset (SILAB3032) */
#endif

		if (ptio.asicType == PCTAC97) 
		  if (*data & OUTR_MUTE)
			tdata |= 0x8000;

		tdata ^= OUTR_OFHR; 		/* set On-Hook/Off-Hook */
    }
	PtASICWriteReg(CTRLREG2, &tdata, 1);
	
	/* set CODEC CALLED-ID here */
	if (tdata & OUTR_CIDR) 
	{	/* turn called-id on */
		if (!(ptio.asicFlags & PTFLAG_CID))
		{	ptio.asicFlags |= PTFLAG_CID;
			asicCmdCID = gPctelCmdWord[2];
		}
	}
	else 
	{	/* turn called-id off */
		if (ptio.asicFlags & PTFLAG_CID)
		{	ptio.asicFlags &= ~PTFLAG_CID;
			asicCmdCID = gPctelCmdWord[1];
		}
	}
	if (asicCmdCID)
	{	ptio.asicCmd[ptio.asicCmdCnt] = asicCmdCID;
		ptio.asicCmdCnt++;
	}
}

/* called by DSP to set the desired ASIC buffer size */
void STDCALL PtASICSetBufferSize(USINT size)
{
	USSHORT tdata;

	/* set ASIC buffer size */
#if (PCT_HWTYPE == PCT_HWA971)		/* pc_chips CM8738 */
	tdata = (USSHORT)(size&0xFF);
	PtASICWriteReg(CTRLREG4, &tdata, 1); 
#elif (PCT_HWTYPE == PCT_HW7891)	/* pc_chips CM8338 or PCI */
	tdata = (USSHORT)(size&0xFF) | 0x2000;	/* whsu */
	PtASICWriteReg(CTRLREG3, &tdata, 1); 
#else
#error PCT_HWTYPE not defined!!!
#endif
	/* flush buffer and read/clear ASIC status */
	PtASICWriteReg(CTRLREG0, NULL, size);
	PtASICReadReg(CTRLREG1, &tdata, 1);
	/* restart ASIC again */
	PtASICRestart();
}

/* called by DSP to set the desired sample frequency */
void STDCALL PtASICSetSampleFreq(USINT fs)
{
	USSHORT tdata;     
	
	/* set ASIC frequency */
	fs = ((fs&0x07)<<1);
	tdata = gPctelClockDiv[fs];
#if (PCT_HWTYPE == PCT_HWA971)		/* pc_chips CM8738 */
	PtASICWriteReg(CTRLREG3, &tdata, 1); /* set clock divider */
#elif (PCT_HWTYPE == PCT_HW7891)	/* pc_chips CM8338 or PCI */
	PtASICWriteReg(CTRLREG5, &tdata, 1); /* set clock divider */	 /* whsu */
#else
#error PCT_HWTYPE not defined!!!
#endif
	/* set CODEC frequency, too */
	ptio.asicCmd[ptio.asicCmdCnt] = gPctelClockDiv[fs+1];
	ptio.asicCmdCnt++;
}

/* called by DSP to insert the special command words */
void STDCALL PtASICRdWrEscape(USINT flags, LPVOID inPtr,
							  LPVOID outPtr, int length)
{
	LPSHORT pCmd;
	USINT cmdCnt;
	USINT i, mask = 0x0001;     
		
	cmdCnt = ptio.asicCmdCnt;
	pCmd = &ptio.asicCmd[cmdCnt];
	/* process data */
	for (i=0; i<16; i++)
	  {	/* add command words to the internal queue */
		if (flags & mask)
		{   
		  switch (1 << i)
			{
			case PTESC_CMD_RLC:		/* read line current */
			  if (!(ptio.asicFlags & PTFLAG_RLC) &&
				  !(ptio.asicFlags & PTFLAG_READ))
				{
				  ptio.asicFlags |= PTFLAG_RLC;
				  ptio.asicIOPtr = inPtr;
				  ptio.asicIODelay = PTIO_DELAY;
				  *pCmd++ = gPctelCmdWord[i];
				  cmdCnt++;			
				}	
			  break;

			case PTESC_CMD_WRITE:
			  while (length-- > 0)
				{
				  *pCmd++ = *(LPLONG)outPtr++;
				  cmdCnt++;
				}
			  break;

			case PTESC_CMD_READ:
			  if (!(ptio.asicFlags & PTFLAG_RLC) &&
				  !(ptio.asicFlags & PTFLAG_READ))
				{
				  ptio.asicFlags |= PTFLAG_READ;
				  ptio.asicIOPtr = inPtr;
				  ptio.asicIODelay = PTIO_DELAY;
				  *pCmd++ = (USSHORT)((*(LPLONG)outPtr++ & 0xFF00) | 0x2000);
				  cmdCnt++;			
				}	
			  break;

			default:	/* write command word */
			  *pCmd++ = gPctelCmdWord[i];
			  cmdCnt++;
			  break;
			}
		}
		mask <<= 1;
	}
	/* update counter of ASIC command words */
	ptio.asicCmdCnt = cmdCnt;
}


