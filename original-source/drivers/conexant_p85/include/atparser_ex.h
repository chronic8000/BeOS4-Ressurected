/****************************************************************************************
                     Version Control Information

	$Header:   P:/d942/octopus/archives/Include/AtParser_Ex.h_v   1.5   20 Apr 2000 13:39:02   woodrw  $

*****************************************************************************************/


/****************************************************************************************

File Name:			AtParser_eh.h		

File Description:	AT Parser data structures

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


#ifndef __ATPARSER_EX_H__
#define __ATPARSER_EX_H__


#define ATDTE_MAJOR_VERSION   0
#define ATDTE_MINOR_VERSION   1

#define AT_DTE_VERSION (((ATDTE_MAJOR_VERSION) << 16) | (ATDTE_MINOR_VERSION))


typedef enum 
{
	ATDTE_CONFIG_LAST     /* Dummy */

}ATDTE_CONFIG_CODE;

typedef enum 
{
    ATDTE_MONITOR_STATUSMGR_CLIENT,
    ATDTE_MONITOR_GET_PIM,

	ATDTE_MONITOR_LAST    /* Dummy */

}ATDTE_MONITOR_CODE;

typedef enum 
{
    ATDTE_CONTROL_PORTCONFIG,   /* Set port configuration   */
    ATDTE_CONTROL_CHAR,         /* Send immediate character */
    ATDTE_CONTROL_SETUP_STATUS,  /* Set initial state for statuses (CTS, RLSD etc) */
    ATDTE_CONTROL_DTR,
    ATDTE_CONTROL_RTS,

	ATDTE_CONTROL_LAST          /* Dummy                  */

}ATDTE_CONTROL_CODE;

PVOID AtDteGetInterface(void);

typedef struct I_AT_DTE_TAG
{

	UINT32		(*GetInterfaceVersion)  (void);
	HANDLE		(*Create)               (PSYSTEM_INSTANCES_T pSystemInstances);			
	COM_STATUS	(*Destroy)              (HANDLE hInst);
	COM_STATUS	(*Init)                 (HANDLE hInst);
	COM_STATUS	(*Close)                (HANDLE hInst);
	COM_STATUS	(*Configure)            (HANDLE hInst, ATDTE_CONFIG_CODE  eCode,  PVOID pConfig);
	COM_STATUS	(*Monitor)              (HANDLE hInst, ATDTE_MONITOR_CODE eCode, PVOID pMonitor);
	COM_STATUS	(*Control)              (HANDLE hInst, ATDTE_CONTROL_CODE eCode, PVOID pControl);

    UINT32      (*TxWrite)              (HANDLE hInst, PVOID pBuf, UINT32 dwCount);
    UINT32      (*RxRead)               (HANDLE hInst, PVOID pBuf, UINT32 dwCount);

} I_AT_DTE_T, *PI_AT_DTE_T;


#endif      /* #ifndef __ATPARSER_EX_H__ */
