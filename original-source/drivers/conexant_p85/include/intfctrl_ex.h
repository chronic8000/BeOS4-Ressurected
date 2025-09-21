/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/IntfCtrl_ex.h_v   1.4   29 Jun 2000 11:50:04   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			IntfCtrl_ex.h

File Description:	Interface Controller object data structures.

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


#ifndef _INTFCTRL_H_
#define _INTFCTRL_H_

#include "comtypes.h"

typedef enum
{
    BLOCK_COM_CTRL,
	BLOCK_MEM_MGR, 
	BLOCK_RT_MGR, 
	BLOCK_DEBUG_MGR, 
	BLOCK_STATUS_MGR, 
	BLOCK_CONFIG_MGR,
	BLOCK_DEVICE_MGR,
	BLOCK_SESSION_MGR,
	BLOCK_NVM_MGR,
    BLOCK_AT_DTE,
	BLOCK_LAST
}BLOCK_TYPE;


typedef PVOID (*FUNC_CREATE_INTERFACE)(	BLOCK_TYPE	BlockType	,
										UINT32		BlockID	);
typedef COM_STATUS (*FUNC_DESTROY_INTERFACE)(PVOID			Interface);

typedef struct INTF_CTRL_FUNC_TABLE_TAG
{
	FUNC_CREATE_INTERFACE	CreateInterface;
	FUNC_DESTROY_INTERFACE	DestroyInterface;
}INTF_CTRL_FUNC_TABLE_T, *PINTF_CTRL_FUNC_TABLE_T;

typedef struct INSTANCE_ACCESS_INFO_TAG
{
	PVOID		InterfaceFunctionTable;
	HANDLE		hInstance;
}INSTANCE_ACCESS_INFO_T, *PINSTANCE_ACCESS_INFO_T;

typedef struct SYSTEM_INSTANCES_TAG
{
    HANDLE                  hTest;
	INTF_CTRL_FUNC_TABLE_T	*pIntrfceCntrlFuncTable;
	INSTANCE_ACCESS_INFO_T	MemMgrInstanceInfo;
	INSTANCE_ACCESS_INFO_T	RTMgrInstanceInfo;
	INSTANCE_ACCESS_INFO_T	DbgMgrInstanceInfo;
	INSTANCE_ACCESS_INFO_T	StatusMgrInstanceInfo;
	INSTANCE_ACCESS_INFO_T	ConfigMgrInstanceInfo;
	INSTANCE_ACCESS_INFO_T	DevMgrInstanceInfo;
    INSTANCE_ACCESS_INFO_T  SessionMgrInstanceInfo;
    INSTANCE_ACCESS_INFO_T  AtDteInstanceInfo;
    INSTANCE_ACCESS_INFO_T  ComCtrlInstanceInfo;

}SYSTEM_INSTANCES_T, * PSYSTEM_INSTANCES_T;

PVOID GetInterfaceControllerFuncTable(	void );

#endif
