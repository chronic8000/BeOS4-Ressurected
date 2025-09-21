/* ================================================================================
 *	File:		pctel.h
 *
 *	    Copyright (C) 1999, PCtel, Inc. All rights reserved.
 *
 *	Purpose:	Header file for ptserial.c
 *
 *	Author:		William Hsu <william_hsu@pctel.com>
 *
 *	Date:		04/15/99
 *
 *	Revision History:
 *
 *	Date		Who		What
 *	----		---		----
 *  04/15/99	whsu	Creation
 * ================================================================================
 */

/* 0x50 is just a magic number to make these relatively unique ('P')  */
#define PCTEL_IOCTL_COUNTRY		0x5001

/* Defined in Controller */
extern unsigned long GlobalTimer;

extern void modem_main(void);
extern void PctelInitCtrlVars(int country);

/* Defined in DSP */
extern void dspMain(void);

