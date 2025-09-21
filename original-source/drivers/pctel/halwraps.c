/*********************************************************
* HALWRAPS.C - HAL layer function wrapper                *
* Copyright (c) 1999 By PCTEL INC                        *
*********************************************************/

#include <stdlib.h>

#include "pctios.h"
#include "halwraps.h"


/* global variables */

unsigned short lineCurrent;
unsigned long SiRegValueR[5];
unsigned long SiRegValueW[5];
int SiNumReg;
unsigned short ASICstatus;
unsigned short ASICerrCnt;
short afeTxPtr[128];
short afeRxPtr[128];

/* private variables */
typedef struct HALWRAPS_ST
{
  unsigned long DltTickCount;
  unsigned long TotalTickCount;
  unsigned short Fifosize;
} HALWRAPS;

static HALWRAPS HalInfo = {0};

/* lifted from v34.h */
enum{
	FS_7200,
	FS_8000,		/* FS_8229, */
	FS_8400,
	FS_9000,
	FS_9600,
	FS_10287,
	FS_12000,
	FS_16000,
	FS_END
};

static int FreqTable[FS_END] =
{ 7200, 8000, 8400, 9000, 9600, 10287, 12000, 16000 };



#ifdef AUDIO_ROUTING

#define InputBufLen   128
#define OutputBufLen  150
#define INIT          0
#define GO            1

extern int IntrWrite(const char *buffer, int size);
extern char intr_buffer[75];

static short InputBuffer[128];
static short OutputBuffer[150];
static int OutCnt;
static int InputCnt;
static int Localfs;
static int Localsample;
static int AudioOn = 0; 
static float Localrate;

int LocalInterpolator(short *InBuf, short InBufLen, short inCnt, 
                      short *OutBuf, float R, int Func)  
{ 
	static  short	*InBufPtr; 
	short	*InBufEnd; 
	static float	PreX1; 
	static	float	ItrpX=0.0f; 
	float	X, frac; 
	short	OutCnt; 
 
	switch (Func) 
	{ 
	case INIT : 
		InBufPtr=InBuf;	// 1st entry 
		ItrpX=0.0f; 
		PreX1=0; 
		return	0; 
	 
	case GO : 
		InBufEnd=InBuf+InBufLen; 
		OutCnt=0; 
		while (inCnt>0) 
		{ 
			frac=ItrpX; 
			ItrpX += R; 
			while (ItrpX >= 1.0f) 
			{ 
				ItrpX -= 1.0f; 
				inCnt -- ; 
				if (InBufPtr++ == InBufEnd) 
					InBufPtr = InBuf; 
			} 
			X=(float)*InBufPtr; 
			*OutBuf++ = (1.0f-frac)*PreX1+frac*X;  
			OutCnt++;    
			PreX1=X; 
			 
		} 
		return	OutCnt; 
	default : 
		return	0; 
	} 
} 
#endif	/* AUDIO_ROUTING */



/* function definitions */
void HAL_Init(int iobase)
{
  HalInfo.DltTickCount = 213;
  HalInfo.TotalTickCount = 0;
  HalInfo.Fifosize = 24;

#if (PCT_HWTYPE == PCT_HWA971)		/* pc_chips CM8738 */
  /* Init ASIC IO port address */
  PtASICInit(PCTAC97, C_SI3032, gPctelRegMap,
			 iobase, PCTAC97CM_INDXP, PCTAC97CM_DATAP,
			 AC97_CODEC_READ_INDEX);
#elif (PCT_HWTYPE == PCT_HW7891)	/* pc_chips CM8338 or PCI */
  /* Init ASIC IO port address */
  PtASICInit(PCT789, C_SI3032, gPctelRegMap,
			 iobase, PCT789_INDXP, PCT789_DATAP,
			 PCT789_CODEC_READ_INDEX);
#else
#error PCT_HWTYPE not defined!!!
#endif

  /* Init ASIC and CODEC */
  PtASICWriteRegEx(gPctelInitcmd);
}

void HAL_Deinit(int iobase)
{
  PtASICWriteRegEx(gPctelDeinitcmd);
}

int HAL_DoInterrupt(void)
{
  int intMask = 0x04;

  PtASICReadReg(CTRLREG3, &ASICerrCnt, 1);
  PtASICReadReg(CTRLREG1, &ASICstatus, 1);

  HalInfo.TotalTickCount += HalInfo.DltTickCount;

  //kprintf("Status- %x\n", (ASICstatus & intMask));
  return(ASICstatus & intMask);
}

int HAL_GetTimer(void)
{
  return (HalInfo.TotalTickCount >> 6);
}

void SetSampleFreq(int fs_index, int notused)
{
  PtASICSetSampleFreq(fs_index);

  /* each step is 1/64 ms */
  HalInfo.DltTickCount = (HalInfo.Fifosize * 64000)/(FreqTable[fs_index]);

#ifdef AUDIO_ROUTING  
  Localfs = FreqTable[fs_index];              /* robertju */
#endif
}

void SetAsicBufSize(int bufSize)
{
#ifdef AUDIO_ROUTING  
  Localsample =  InputCnt = bufSize;          /* robertju */
#endif

  PtASICSetBufferSize(bufSize);
  HalInfo.Fifosize = bufSize;
}

void RdASIC(void)
{}

void WrASIC(void)
{
#ifdef AUDIO_ROUTING
  int i;
  static int count = 0;

  for (i=0; i<Localsample; i++)
	{
	  InputBuffer[i] = afeRxPtr[i];    
	}

  Localrate = Localfs/8000.0;
  
  LocalInterpolator(InputBuffer, InputBufLen, InputCnt, 
                    OutputBuffer, Localrate, INIT);                 
  OutCnt = LocalInterpolator(InputBuffer, InputBufLen, 
                             InputCnt, OutputBuffer, Localrate, GO);     
  
  for (i=0; i<OutCnt; i++)
	{
      intr_buffer[count] = OutputBuffer[i]>>10;  
      count++;  

      if (count >= 75)
        {    
		  if (AudioOn)
          	IntrWrite(intr_buffer, 75);
          count = 0;  
        } 
	}  
#endif

  PtASICRdWrReg(CTRLREG0, &afeRxPtr[0], &afeTxPtr[0],
				HalInfo.Fifosize, 0);
}

void RestartPt(void)
{
  PtASICRestart();
}

void WriteOutport(int cmd)
{
  unsigned short short_cmd = (unsigned short)cmd;

#ifdef AUDIO_ROUTING
  AudioOn = (cmd & OUTR_MUTE) ? 1 : 0;
#endif

  PtASICWrOutport(&short_cmd);
}

int ReadInport(void)
{
  unsigned short retval;

  PtASICRdInport(&retval);
  return (int)retval;
};

void CheckLineCurrent(void)
{
  lineCurrent = 0;
  PtASICRdWrEscape(PTESC_CMD_RLC, &lineCurrent, 0, 1);
}

void SiPatch(void)
{
  /* toggle offhook signal for SiLab patch */
  PtASICRdWrEscape(PTESC_CMD_OHD | PTESC_CMD_OHE, 0, 0, 1);
}

void WriteOutSiReg(int cmd)
{
  if (cmd & 0x01)
	PtASICRdWrEscape(PTESC_CMD_PDL_INIT, 0, 0, 1);
  else
	PtASICRdWrEscape(PTESC_CMD_PDL_RST, 0, 0, 1);
}

void WriteSiReg(void)
{
  PtASICRdWrEscape(PTESC_CMD_WRITE, 0, SiRegValueW, SiNumReg);
}

void ReadSiReg(void)
{
  SiRegValueR[0] = 0;
  PtASICRdWrEscape(PTESC_CMD_READ, SiRegValueR, SiRegValueW, SiNumReg);
};

