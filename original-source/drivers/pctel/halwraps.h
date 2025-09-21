/* ================================================================================
 *	File:		halwraps.h
 *
 *	    Copyright (C) 1999, PCtel, Inc. All rights reserved.
 *
 *	Purpose:	Header file for HAL layer wrapper
 *
 *	Author:		William Hsu <william_hsu@pctel.com>
 *
 *	Date:		06/03/99
 *
 *	Revision History:
 *
 *	Date		Who		What
 *	----		---		----
 *  06/03/99	whsu	Creation
 * ================================================================================
 */
#ifndef HALWRAPS_H
#define HALWRAPS_H

#define ASIC7		0x60
#define SI3032		0x02

/* #define AUDIO_ROUTING */

/* Exported functions to PTSERIAL */
void HAL_Init(int iobase);
void HAL_Deinit(int iobase);
int HAL_DoInterrupt(void);
int HAL_GetTimer(void);

/* Exported functions to DSP */
void SetSampleFreq(int fs_index, int notused);
void SetAsicBufSize(int bufSize);
void RdASIC(void);
void WrASIC(void);
void RestartPt(void);
void WriteOutport(int cmd);
int ReadInport(void);
void CheckLineCurrent(void);
void SiPatch(void);
void WriteOutSiReg(int cmd);
void WriteSiReg(void);
void ReadSiReg(void);

/* Exported variables to DSP */
extern unsigned short lineCurrent;
extern unsigned long SiRegValueR[5];
extern unsigned long SiRegValueW[5];
extern int SiNumReg;
extern unsigned short ASICstatus;
extern unsigned short ASICerrCnt;
extern short afeTxPtr[128];
extern short afeRxPtr[128];

#endif /* HALWRAPS_H */

