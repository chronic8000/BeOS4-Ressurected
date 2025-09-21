/****************************************************************************************
*                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/ConfigMgr_Ex.h_v   1.6   29 Jun 2000 11:50:02   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			ConfigMgr_ex.h	

File Description:	Configuration Manager Data Strucutres	

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


#ifndef __CONFIG_MGR_EX_H__
#define __CONFIG_MGR_EX_H__

#include "configtypes.h"


#define	CONFIG_MGR_MAJOR_VERSION	0
#define	CONFIG_MGR_MINOR_VERSION	1

typedef enum 
{
	CONFIGMGR_CONFIG_ESTIMATED_CONFIG_NUM,
	CONFIG_NVM_ID,						/*  UINT32 *						*/
	CONFIGMGR_CONFIR_LAST /* dummy */
}CONFIGMGR_CONFIG_CODE;

typedef enum 
{
    CONFIGMGR_CONTROL_LOAD_PROFILE_DEFAULT,     // Load Factory-default profile
    CONFIGMGR_CONTROL_LOAD_PROFILE_0,           // Load Stored Profile 0
    CONFIGMGR_CONTROL_LOAD_PROFILE_1,           // Load Stored Profile 1

    CONFIGMGR_CONTROL_STORE_PROFILE_0,          // Save current settings into Profile 0
    CONFIGMGR_CONTROL_STORE_PROFILE_1,          // Save current settings into Profile 1

    CONFIGMGR_CONTROL_CHANGE_COUNTRY,           // Change current country
	CONFIGMGR_CONTROL_LAST /* dummy */
}CONFIGMGR_CONTROL_CODE;

typedef enum 
{
    CONFIGMGR_MONITOR_CURRENT_PROFILE,          // Get current profile (PROFILE_DATA)
    CONFIGMGR_MONITOR_PROFILE_0,                // Get stored profile 0 (PROFILE_DATA)
    CONFIGMGR_MONITOR_PROFILE_1,                // Get stored profile 1 (PROFILE_DATA)

	CONFIGMGR_MONITOR_LAST /* dummy */
}CONFIGMGR_MONITOR_CODE;

#define CONFIG_MGR_VERSION (((CONFIG_MGR_MAJOR_VERSION) << 16) | (CONFIG_MGR_MINOR_VERSION))


	/********************************************************************************
	
	Function Name: ConfigMgrGetInterface.

	Function Description:	The function returns a pointer to the configuration manager
							interface structure that contains pointer to all the exported
							configuration manager functions.
	Arguments: None.
	
	Return Value: A pointer to the configuration manager interface functions structure.

	********************************************************************************/
	GLOBAL	PVOID	ConfigMgrGetInterface(void);

typedef struct I_CONFIG_MGR_TAG{

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
	UINT32		(*GetInterfaceVersion)();


	/********************************************************************************

	Function Name: Create.

	Function Description: The function creates a block instance.
	
	Arguments: MemMgrInterface - Access (function table & instance handle ) 
	                             to memory manager.

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
							 IN CONFIGMGR_CONFIG_CODE	ConfigCode,
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
						   IN	CONFIGMGR_MONITOR_CODE	MonitorCode,
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
						   IN	CONFIGMGR_CONTROL_CODE	ControlCode,
						   IN	PVOID					pControl);


/********************************************************************************

Function Name: QueryConfiguration.

Function Description:	Returnes in pConfigData the data value of the 
						queried code.

Arguments: hBlock		- block handle of configuration manager instance.
		   ControlCode	- enumerated code identifying the control query.
		   pConfigData	- pointer to a structure to be filled with the queried data.
		   DataSize		- size of data to be copied to pConfigData

Return Value:	COM_STATUS_SUCCESS if pConfigData was updated successfully, else
				error code.

********************************************************************************/
	COM_STATUS	(*QueryConfiguration)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PVOID				pConfigData,
						IN	int					DataSize);

/********************************************************************************

Function Name: DirectQueryConfiguration.

Function Description:	Returnes in ppConfigData a pointer to configuration manager 
						internal database of the queried code data.

Arguments: hBlock		- block handle of configuration manager instance.
		   ControlCode	- enumerated code identifying the control query.
		   ppConfigData	- address of pointer filled with the queried data pointer.

Return Value:	COM_STATUS_SUCCESS if ppConfigData was updated successfully, else
				error code.

********************************************************************************/
	COM_STATUS	(*DirectQueryConfiguration)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						OUT	PPVOID				ppConfigData);

/********************************************************************************

Function Name: UpdateConfiguration.

Function Description:	Updates the configuration data for the ControlCode. This
						function may allocate memory.

Arguments: hBlock		- block handle of configuration manager instance.
		   ControlCode	- enumerated code identifying the control .
		   pConfigData	- pointer to structure containing the configuration data.
		   DataSize		- size of configuration data

Return Value:	COM_STATUS_SUCCESS if configuration manager was updated successfully, 
				else error code.

********************************************************************************/
	COM_STATUS	(*UpdateConfiguration)(
						IN	HANDLE				hBlock,
						IN	CFGMGR_CODE		    ControlCode,
						IN	PVOID				pConfigData,
						IN	int					DataSize);


/********************************************************************************

Function Name: RestoreAllConfigurations.

Function Description:	Updates the configuration data for the ControlCode. This
						function may allocate memory.

Arguments: hBlock		- block handle of configuration manager instance.

Return Value:	COM_STATUS_SUCCESS if all configurations supported by NVM manager
				were updated from NVM to configuration manager database.

********************************************************************************/
	COM_STATUS	(*RestoreAllConfigurations)(
						IN	HANDLE				hBlock);


/********************************************************************************

Function Name: SaveAllConfigurations.

Function Description:	Updates the configuration data for the ControlCode. This
						function may allocate memory.

Arguments: hBlock		- block handle of configuration manager instance.

Return Value:	COM_STATUS_SUCCESS if all configurations supported by NVM manager
				were saved from configuration manager database to NVM.

********************************************************************************/
	COM_STATUS	(*SaveAllConfigurations)(
						IN	HANDLE				hBlock);

/********************************************************************************

Function Name: RestoreConfiguration.

Function Description:	Updates the configuration data for the ControlCode. This
						function may allocate memory.

Arguments:	hBlock		- block handle of configuration manager instance.
			ControlCode - Code of configuration to be restored from NVM

Return Value:	COM_STATUS_SUCCESS if configuration was restored from NVM to 
				configuration manager database.
				

********************************************************************************/
	COM_STATUS	(*RestoreConfiguration)(
						IN	HANDLE				hBlock, 
						IN	CFGMGR_CODE		    ControlCode);

/********************************************************************************

Function Name: SaveConfiguration.

Function Description:	Saves configuration manager ControlCode data to NVM.

Arguments:	hBlock		- block handle of configuration manager instance.
			ControlCode - Code of configuration to be restored from NVM

Return Value:	COM_STATUS_SUCCESS if configuration was saved to NVM.
				

********************************************************************************/
	COM_STATUS	(*SaveConfiguration)(
						IN	HANDLE				hBlock, 
						IN	CFGMGR_CODE		    ControlCode);

} I_CONFIG_MGR_T, *PI_CONFIG_MGR_T;

#endif /* __CONFIG_MGR_EX_H__ */
