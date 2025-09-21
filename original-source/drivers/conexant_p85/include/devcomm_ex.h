/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/DevComm_ex.h_v   1.5   29 Jun 2000 11:50:02   lisitse  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			DevComm.h

File Description:	Interface controller object prototypes.

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


#include "comtypes.h"
#include "intfctrl_ex.h"

HANDLE	ConnectBlock(			BLOCK_TYPE	BlockType , 
								UINT32		BlockID );

VOID	DisconnectBlock(		HANDLE		hBlock	);

PVOID	GetInterfaceFuncTbl(	HANDLE		hBlock	);

