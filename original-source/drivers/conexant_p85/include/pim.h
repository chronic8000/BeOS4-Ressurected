/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/pim.h_v   1.5   20 Apr 2000 14:19:30   woodrw  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			pim.h

File Description:	Plug-in module enumerated types, data structures, and prototype.

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


#if !defined(__PIM_H__)
#define __PIM_H__


typedef enum {
   /* Tx status return from controller */
   HOSTTOHAL_NOT_EOF,
   HOSTTOHAL_IS_EOF,
   HOSTTOHAL_TX_ABORT,
   HOSTTOHAL_BUFFER_EMPTY,
   /* Tx status to controller */
   HALTOHOST_TX_UNDERRUN,
   HALTOHOST_REQUEST_DATA,        /* normal case */
   /* Rx status return from DevMgr */
   HALTOHOST_NOT_EOF,
   HALTOHOST_GOOD_EOF,
   HALTOHOST_BAD_CRC_EOF,
   HALTOHOST_ABORT_EOF,
   HALTOHOST_RX_OVERRUN,
   /* Rx status to DevMgr */
   HOSTTOHAL_BUFFER_FULL,
   HOSTTOHAL_RX_OK,               /* normal case */
   HOSTTOHAL_RX_ABORT            /* Host abort RX */
   } STREAM_STATUS;

typedef BOOL (*PIM_CALLBACK_FUNC)(HANDLE Handle, PVOID pBuffer, UINT32* puiSize, STREAM_STATUS* peStatus);

/* PIM interface description structure */
typedef struct tagHPIMDESC {
	HANDLE              hReadObject;
	HANDLE              hWriteObject;
	PIM_CALLBACK_FUNC   fpRead;
	PIM_CALLBACK_FUNC   fpWrite;
} PIM_INTERFACE_STRUCT, *PPIM_INTERFACE_STRUCT;

#endif 
