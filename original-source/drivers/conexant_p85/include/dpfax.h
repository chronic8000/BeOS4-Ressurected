/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/dpfax.h_v   1.4   20 Apr 2000 13:57:38   woodrw  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			dpfax.h

File Description:	Device Manager Fax enumerated types

*****************************************************************************************/


/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2000                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                             Personal Computing Division                           ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
*****************************************************************************************/


#ifndef _DPFAX_H
#define _DPFAX_H

#define FAX_MODEM

typedef enum
{
    V21CH2_300_MOD  = 3,
    V27TER_2400_MOD = 24,
    V27TER_4800_MOD = 48,
    V29_7200_MOD    = 72,
    V17_7200_MOD    = 73,
    V17_7200S_MOD   = 74,
    V29_9600_MOD    = 96,
    V17_9600L_MOD   = 97,
    V17_9600S_MOD   = 98,
    V17_12000L_MOD  = 121,
    V17_12000S_MOD  = 122,
    V17_14400L_MOD  = 145,
    V17_14400S_MOD  = 146,
}   FAX_MOD;


typedef struct tagFAX_MOD_PARAMS {
	FAX_MOD	    eFaxMod;
	BOOL		bTx;
} FAX_MOD_PARAMS;

#endif // _DPFAX_H
