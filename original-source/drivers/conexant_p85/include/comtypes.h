/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/ComTypes.h_v   1.5   28 Jun 2000 13:42:16   lisitse  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			ComTypes.h	

File Description:	COM Status Types

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

#ifndef __COM_TYPES_H__
#define __COM_TYPES_H__

typedef enum {
	COM_STATUS_SUCCESS, 
	COM_STATUS_FAIL, 
	COM_STATUS_CODE_NOT_SUPPORTED, 
	COM_STATUS_NULL_POINTER,
	COM_STATUS_INST_NOT_CONFIGURED,
	COM_STATUS_INST_NOT_INIT,
	COM_STATUS_INST_NOT_CLOSED,
	COM_STATUS_INVALID_HANDLE,
	COM_STATUS_INVALID_PARAM,
	COM_STATUS_INVALID_MODE,
    COM_STATUS_VALUE_NOT_FOUND,
	COM_STATUS_ALLOC_FAILED,
	COM_STATUS_CREATE_FAILED,
	COM_STATUS_OPEN_FAILED,
	COM_STATUS_BLACKLISTED,
	COM_STATUS_DELAYED,
	COM_STATUS_UNKNOWN,
}COM_STATUS;							



typedef VOID (*PCBFUNC)(PVOID);
typedef PVOID (*PALLOC_FUNC)(	int /* Alloc Size */, 
								PVOID /* pRefData */);	
typedef void (*PFREE_FUNC)(		PVOID /* Block Pointer */,
								PVOID /* pRefData */);		

/*////////////////////////////////////////////////////////////////*/

// Data in AtCmd.c (AtPlusMS) relies on the enum order
// If make any changes in this enum - modify that data as well

typedef enum {
	MODULATION_TYPE_BELL103,
	MODULATION_TYPE_BELL212,		
	MODULATION_TYPE_V21,
	MODULATION_TYPE_V22,		
	MODULATION_TYPE_V22BIS,		
	MODULATION_TYPE_V23,		
	MODULATION_TYPE_V32,		
	MODULATION_TYPE_V32BIS,		
	MODULATION_TYPE_V34,		
	MODULATION_TYPE_K56,		
	MODULATION_TYPE_V90,		
	MODULATION_TYPE_VFC,		

	MODULATION_TYPE_K56A,
    MODULATION_TYPE_V34_CLEAR,
    MODULATION_TYPE_V33,
    MODULATION_TYPE_V32BIS_CLEAR,
	MODULATION_TYPE_V29,		
	MODULATION_TYPE_V27,
    MODULATION_TYPE_V21C2, 
    MODULATION_TYPE_V17,
}MODULATION_TYPE;


typedef enum {
	ANSWER_MODE,
	CALLER_MODE
} CALL_MODE;



#endif /* __COM_TYPES_H__ */
