/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/NVMMgr_ex.h_v   1.7   29 Jun 2000 11:50:06   lisitse  $
* 
****************************************************************************************/


/****************************************************************************************

File Name:			NVMMgr_ex.h	

File Description:	NVM Manager data structures and prototypes.

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



#ifndef __NVM_MGR_EX_H__
#define __NVM_MGR_EX_H__

#include "intfctrl_ex.h"
#include "configtypes.h"
#define	NVM_MGR_MAJOR_VERSION	0
#define	NVM_MGR_MINOR_VERSION	1

typedef enum 
{
//	NVMMGR_CONFIG_SET_DEV_INFO /* OS dependend value passed by the Port Driver */
	NVMMGR_CONFIG_LAST /* dummy */
}NVMMGR_CONFIG_CODE;

typedef enum 
{
	NVMMGR_CONTROL_LAST /* dummy */
}NVMMGR_CONTROL_CODE;

typedef enum 
{
	NVMMGR_MONITOR_LAST /* dummy */
}NVMMGR_MONITOR_CODE;


typedef COM_STATUS (*PCB_QUERY)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PVOID				pConfigData,
						IN	int					DataSize);

typedef COM_STATUS (*PCB_DIRECT_QUERY)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PPVOID				ppConfigData);

typedef COM_STATUS	(*PCB_UPDATE)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PVOID				pConfigData,
						IN	int					DataSize);

#define NVM_MGR_VERSION ((NVM_MGR_MAJOR_VERSION) << 16) | (NVM_MGR_MINOR_VERSION)


/********************************************************************************

Function Name: NVMMgrGetInterface.

Function Description:	The function returns a pointer to the NVM manager
						interface structure that contains pointer to all the exported
						NVM manager functions.
Arguments: None.

Return Value: A pointer to the NVM manager interface functions structure.

********************************************************************************/
GLOBAL	PVOID	NVMMgrGetInterface(void);



/************ Common Interface functions. ************/
typedef struct I_NVM_MGR_TAG{


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

Arguments: pSystemInstances - Access to Interface controller interface, and
								All system modules

Return Value: An handle to the created instance. (this handle will passed as an 
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
						 IN NVMMGR_CONFIG_CODE		ConfigCode,
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
COM_STATUS	(*Monitor)(IN	HANDLE					hBlock,
					   IN	NVMMGR_MONITOR_CODE		MonitorCode,
					   OUT	PVOID					pMonitor);

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
COM_STATUS	(*Control)(IN	HANDLE					hBlock,
					   IN	NVMMGR_CONTROL_CODE		ControlCode,
					   IN	PVOID					pControl);


/********************************************************************************

Function Name: WriteConfigToNVM.

Function Description:	The NVM manager updates NVM with the configuration data.

Arguments: hBlock		- block handle of NVM manager instance.
		   ControlCode	- enumerated code identifying the control query.
		   pConfigData	- pointer to a structure to be saved to NVM.

Return Value:	Returnes COM_STAUTS_SUCCESS if the configure data was successfully 
				written to NVM.If the NVM does not support the code , returns 
				COM_STATUS_CODE_NOT_SUPPORTED.
********************************************************************************/
	COM_STATUS	(*WriteConfigToNVM)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PVOID				pConfigData);

/********************************************************************************

Function Name: ReadConfigFromNVM.

Function Description:	The NVM manager reads the configuration data from the NVM
						and stores it into pConfigData.

Arguments: hBlock		- block handle of configuration manager instance.
		   ControlCode	- enumerated code identifying the control query.
		   pConfigData	- pointer to a structure to be filled with the queried data.

Return Value:	Returns COM_STATUS_SUCCESS if the configure data was successfule
				read fom NVM into pConfigData
				error code.

********************************************************************************/
	COM_STATUS	(*ReadConfigFromNVM)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PVOID				pConfigData);

/********************************************************************************

Function Name: SaveAllConfig.

Function Description:	Saves all the configuration codes supported by NVM manager
						to NVM. The function is synchronic and does not return untill
						all configurations were queried by the pCbQuery and save to NVM.

Arguments: hBlock		- block handle of NVM manager instance.
		   pCbQuery		- Callback function for querying configuration data.

Return Value:	Returnes COM_STAUTS_SUCCESS if all configurations data were 
				successfully written to NVM.
********************************************************************************/
	COM_STATUS	(*SaveAllConfig)(
						IN	HANDLE				hBlock,
						IN	HANDLE				hCBBlock,
						PCB_DIRECT_QUERY		pCbQuery);

/********************************************************************************

Function Name: RestoreAllConfig.

Function Description:	Restores all the configuration codes supported by NVM manager
						from NVM. The function is synchronic and does not return untill
						all configurations were updated by the pCbUpdate.

Arguments: hBlock		- block handle of NVM manager instance.
		   pCbUpdate	- Callback function for updating configuration data.

Return Value:	Returnes COM_STAUTS_SUCCESS if all configurations data were 
				successfully restored from NVM.
********************************************************************************/
	COM_STATUS	(*RestoreAllConfig)(
						IN	HANDLE				hBlock,
						IN	HANDLE				hCBBlock,
						PCB_UPDATE				pCbQuery);

/********************************************************************************

Function Name: ChangeCountry

Function Description:	Restores all the configuration codes supported by NVM manager
						from NVM. The function is synchronic and does not return untill
						all configurations were updated by the pCbUpdate.

Arguments: hBlock		- block handle of NVM manager instance.
           wCountry     - new country T35 code
		   pCbDirectQuery	- Callback function for direct querying configuration data.

Return Value:	Returnes COM_STAUTS_SUCCESS if all configurations data were 
				successfully restored from NVM.
********************************************************************************/
	COM_STATUS	(*ChangeCountry)(
						IN	HANDLE				hBlock,
                        IN  UINT16              wCountry,
						IN	HANDLE				hCBBlock,
						PCB_DIRECT_QUERY		pCbDirectQuery);



} I_NVM_MGR_T, *PI_NVM_MGR_T;

#endif /* __NVM_MGR_EX_H__ */
