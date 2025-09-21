/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.

*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
    

*/
/*
 * InsideLine(tm) Frame Format
 * Copyright(c) 1999 Broadcom Corporation.
 *
 * $Id: iline.h,v 9.14 2000/03/08 04:59:04 abagchi Exp $
 */

#ifndef _ILINE_H_
#define	_ILINE_H_

#include "typedefs.h"

/*
 * On the wire frame format:
 * FrameControl  EtherHeader EthernetPayload EthernetFCS PayloadCRC16
 * (4bytes)      (14bytes)   (46-1500bytes)   (4bytes)    (2bytes)
 */

struct framecontrol {
	uint8	type;		/* frame type */
	uint8	ctl;		/* si and pri */
	uint8	txdesc;		/* transmit encoding descriptor */
	uint8	chk;		/* header check crc8 */
};

#define	CTL_SI_MASK		0xf
#define	CTL_PRI_MASK		0x70
#define	CTL_PRI_SHIFT		4

#define	ILINE_HDR_LEN		4
#define	ILINE_CRC16_LEN		2
#define ILINE_HDRMINPE_LEN	(ILINE_HDR_LEN + 14)
#define ILINE_MIN_LEN		(ILINE_HDR_LEN + 64 + ILINE_CRC16_LEN)
#define	ILINE_MAX_LEN		(ILINE_HDR_LEN + 1526 + ILINE_CRC16_LEN)
#define	ILINE_PREAMBLE_LEN	16
#define	ILINE_MIN_BPB		2
#define	ILINE_MAX_BPB		8
#define	ILINE_NPRI		8
#define	ILINE_MIN_PRI		0
#define	ILINE_MAX_PRI		7

#define MAX_BANDS		2
#define MAX_PES			14
#define PE_NULL			0
#define PE_BAND1_2BPB		1
#define PE_BAND1_3BPB		2
#define PE_BAND1_4BPB		3
#define PE_BAND1_5BPB		4
#define PE_BAND1_6BPB		5
#define PE_BAND1_7BPB		6
#define PE_BAND1_8BPB		7
#define PE_HPNA_1_0		8
#define PE_BAND2_2BPB		9
#define PE_BAND2_3BPB		10
#define PE_BAND2_4BPB		11
#define PE_BAND2_5BPB		12
#define PE_BAND2_6BPB		13
#define PE_BAND2_7BPB		14
#define PE_BAND2_8BPB		15

/* hpna 1.0 header is just 4byte pcom field */
#define	TUT_HDR_LEN		4
#define TUT_MIN_LEN		(TUT_HDR_LEN + 64)
#define	PCOM_V1			0	/* hpna 1.x station */
#define	PCOM_V2			1	/* hpna 2.x station */
#define	PCOM_V2D		2	/* hpna 2.x station that has seen hpna 1.x */

#endif	/* _ILINE_H_ */
