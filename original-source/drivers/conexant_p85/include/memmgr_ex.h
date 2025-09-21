/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/MemMgr_ex.h_v   1.5   29 Jun 2000 11:50:04   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			MemMgrE.h	

File Description:	Memory Manager enumerated types, data structures, and prototypes.

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


#ifndef __MEM_MGR_E_H__
#define __MEM_MGR_E_H__

#include "comtypes.h"
#include "statcode.h"
#include "intfctrl_ex.h"

#define	MEM_MGR_MAJOR_VERSION	0
#define	MEM_MGR_MINOR_VERSION	2

#define UNLIMITED_QUOTA			((UINT32)-1)

#define MEM_MGR_VERSION (((MEM_MGR_MAJOR_VERSION) << 16) | (MEM_MGR_MINOR_VERSION))

typedef enum 
{
	MEMMGR_CONFIG_MAX_ALLOCATION_SIZE,			/*  UINT32* 						*/
	MEMMGR_CONFIG_LAST /* Dummy */
}MEMMGR_CONFIG_CODE;

typedef enum 
{
	MEMMGR_MONITOR_CLIENT_LEAK_DETECTION,	/* PMEM_MGR_MONITOR_DATA : In:: PMEM_MONITOR_ALLOC_DATA Out:: BOOL */
	MEMMGR_MONITOR_CLIENT_STATUS,			/* PMEM_MGR_MONITOR_DATA : IN:: HANDLE (hClient) Out:: PMEM_MONITOR_CLIENT_DATA */
	MEMMGR_MONITOR_LAST /* Dummy */
}MEMMGR_MONITOR_CODE;

typedef enum 
{
	MEMMGR_CONTROL_PRINT_ALLOC_LIST,		/* PMEM_MONITOR_PRINT_DATA_T	*/
	MEMMGR_CONTROL_PRINT_FREE_LIST,			/* PMEM_MONITOR_PRINT_DATA_T	*/
	MEMMGR_CONTROL_LAST /* Dummy */
}MEMMGR_CONTROL_CODE;

typedef struct MEM_CLIENT_INFO_TAG{
	UINT32	AllocQuota;
	BOOL	bPaged;
}MEM_CLIENT_INFO_T, *PMEM_CLIENT_INFO_T;

typedef struct MEM_MGR_MONITOR_DATA_TAG {
	PVOID	pInputData;
	PVOID	pOutputData;
} MEM_MGR_MONITOR_DATA_T, *PMEM_MGR_MONITOR_DATA_T;

typedef struct MEM_MONITOR_PRINT_DATA_TAG{
	HANDLE	hClient;
#ifdef MEM_DEBUG
	FILE*	File;
#endif
	PCHAR	PrintHeader;
} MEM_MONITOR_PRINT_DATA_T, *PMEM_MONITOR_PRINT_DATA_T;

typedef struct MEM_MONITOR_CLIENT_DATA_TAG {
	UINT32	TotalAllocs;
	UINT32	NumOfAllocs;
	BOOL	bLeakDet;
} MEM_MONITOR_CLIENT_DATA_T, *PMEM_MONITOR_CLIENT_DATA_T;

typedef struct MEM_MONITOR_ALLOC_DATA_TAG {
	HANDLE	hClient;
	PVOID	pMem;
} MEM_MONITOR_ALLOC_DATA_T, *PMEM_MONITOR_ALLOC_DATA_T;

	/********************************************************************************
	
	Function Name: MemMgrGetInterface.

	Function Description:The function returns a pointer to the memory manager interface 
						 structure that contains pointer to all the exported memory manager
						 functions.
	Arguments: None.
	
	Return Value: A pointer to the memory manager interface functions structure.

	********************************************************************************/
	GLOBAL	PVOID	MemMgrGetInterface(void);


typedef struct I_MEM_MGR_TAG{

	/************ Common Interface functions. ************/
	
	/********************************************************************************
	
	Function Name: GetInterfaceVersion.

	Function Description:The function returns the interface compilation version.
						 The function should be called before any other function 
						 in the function table in order to verify that the calling 
						 code and the block code are compatible.
	
	Arguments: None.
	
	Return Value: The interface compilation version.

	********************************************************************************/
	UINT32		(*GetInterfaceVersion)(void);

	/********************************************************************************

	Function Name: Create.

	Function Description: The function creates a block instance.
	
	Arguments: MemMgrInterface - not in use for memory manager - should be NULL 
	
	Return Value: A handle to the created instance. (this handle will passed as an 
				  argument to all other functions to identify the block instance).
				  (in case of error it returns NULL).

	********************************************************************************/
	HANDLE		(*Create)(IN	PSYSTEM_INSTANCES_T	pSystemInstances);			

	/********************************************************************************

	Function Name: Destroy.

	Function Description: The function destroy the block instance identify by the hBlock
						  handle. After the destroying this handle becomes invalid. 
	
	Arguments: hblock - a handle to the block to destroy. (this handle was returned by 
						the create function when we created this block instance).
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully 
				  destroyed).

	********************************************************************************/
	COM_STATUS	(*Destroy)(IN HANDLE	hBlock);

	/********************************************************************************

	Function Name: Init.

	Function Description: The function initialize the block identified by the hBlock 
						  parameter.
	
	Arguments: a handle to the block to initialize.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  initialized).

	********************************************************************************/
	COM_STATUS	(*Init)(IN	HANDLE	hBlock);

	/********************************************************************************

	Function Name: Close.

	Function Description: The function closes the block identified by the hBlock handle.
						  After the instance was closed, its handle can be destroyed by 
						  the destroy function.
	
	Arguments: a handle to the block to close.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Close)(IN HANDLE	hBlock);		

	/********************************************************************************

	Function Name: Configure.

	Function Description: The function is used to update the block instance
						  configurations. Each ConfigCode associates with a specific
						  structure containing the configuration value/data.
						  The function can be called only after create and before init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ConfigCode - enumerated code identifying the configuration type.
			   pConfig - pointer to a structure containing the configuration data.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Configure)(IN HANDLE					hBlock,
							 IN MEMMGR_CONFIG_CODE		ConfigCode,
							 IN PVOID					pConfig);

	/********************************************************************************

	Function Name: Monitor.

	Function Description: The function is used to monitor a block instance.
						  Each MonitorCode is associates with a specific structure
						  containing the Monitoring information.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   MonitorCode - enumerated code identifying the monitor type.
			   pMonitor - pointer to a structure that will be filled with the monitored data.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  monitored).

	********************************************************************************/
	COM_STATUS	(*Monitor)(IN	HANDLE				hBlock,
						   IN	MEMMGR_MONITOR_CODE	MonitorCode,
						   OUT	PVOID				pMonitor);

	/********************************************************************************

	Function Name: Control.

	Function Description: The function is used to change the value of the block 
						  instance parameters. Each ControlCode is associates with 
						  a specific structure containing the control data/value.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ControlCode - enumerated code identifying the control type.
			   pControl - pointer to a structure containing the control data.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  controlled).

	********************************************************************************/

	COM_STATUS	(*Control)(	IN	HANDLE				hBlock,
							IN	MEMMGR_CONTROL_CODE	ControlCode,
							IN	PVOID				pControl);



	/************ Specific Interface functions. ************/
	
	/********************************************************************************

	Function Name: RegisterClient

	Function Description: The function registers a client to a specific memory manager.

	Arguments: hBlock - block handle (identify of the block).
			   pClientConfig - a pointer to a structure containing the client registration
			   parameters.
						   	
	Return Value: a handle to the registered client. (the handle is used to the UnregisterClient,
				  Free and Alloc functions).

	********************************************************************************/
	HANDLE	(*RegisterClient)(IN	HANDLE				hBlock,
							  IN	PMEM_CLIENT_INFO_T	pClientConfig);

	/********************************************************************************

	Function Name: UnregisterClient

	Function Description: The function unregisters a client from a specific memory manager.
						  the function frees all the client allocated memory.

	Arguments: hBlock - block handle (identify of the block).
			   hClient - the client handle (returned by RegisterClient function).
			   	
	Return Value: COM_STATUS ( COM_STATUS_SUCCESS if the client was successfully unregistered).

	********************************************************************************/
	COM_STATUS	(*UnregisterClient)(IN	HANDLE	hBlock,
									IN	HANDLE	hClient);

	/********************************************************************************

	Function Name: Alloc

	Function Description: The function allocates 'Size' bytes from the system memory.
						  The function should be called after Init and before Close.

	Arguments: hBlock - block handle (identify of the block).
			   hClient - the client handle (returned by RegisterClient function).
			   Name - is the identify name of the allocated memory.
			   Size - is the number of bytes the function should allocate.
			   	
	Return Value: a pointer to the allocated memory space or NULL if the allocation failed.

	********************************************************************************/
	PVOID	(*Alloc)(IN	HANDLE	hBlock,
					 IN	HANDLE	hClient,
					 IN	PCHAR	Name,
					 IN	UINT32	ReqSize);


	/********************************************************************************

	Function Name: Free

	Function Description: The function frees the memory pMem is pointing to.

	Arguments: hBlock - block handle (identify of the block).
			   hClient - the client handle (returned by RegisterClient function).
			   pMem - a pointer to the allocated memory (returned by Alloc function).
			   	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the memory was successfully freed).

	********************************************************************************/
	COM_STATUS	(*Free)(IN	HANDLE	hBlock,
						IN	HANDLE	hClient,
						IN	PVOID	pMem);

} I_MEM_MGR_T, *PI_MEM_MGR_T;

#endif /* __MEM_MGR_E_H__ */
