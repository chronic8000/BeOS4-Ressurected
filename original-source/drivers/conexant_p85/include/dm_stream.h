/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/dm_stream.h_v   1.4   20 Apr 2000 13:57:38   woodrw  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			dm_stream.h

File Description:	Device Manager stream enumerated types.

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


#ifndef _DMSTREAM_H
#define _DMSTREAM_H

typedef enum {
	DEVMGR_STREAM_NULL,
	DEVMGR_STREAM_HDLC,
	DEVMGR_STREAM_V14,
	DEVMGR_STREAM_SYNCTx_V14Rx,
	DEVMGR_STREAM_DIRECT,
	DEVMGR_STREAM_VOICE,
	DEVMGR_STREAM_DCP
} eStreamType;

#endif // _DMSTREAM_H
