/****************************************************************************************
 *                     Version Control Information										*
 *                                                                                      *
 * $Header:   P:/d878/octopus/archives/OsDDK/OsInterface.c_v   1.3   13 Mar 2000 11:15:32   rayskymy  $
 * 
 * Interface Manager OS specific implementation for monolitic driver

*****************************************************************************************/
#include "ostypedefs.h"
#include "comtypes.h"
#include "devcomm_ex.h"


#include "memmgr_ex.h"
#include "rtmgr_ex.h"
#include "statmgr_ex.h"
//#include "DbgMgr.h"
#include "configmgr_ex.h"
#include "nvmmgr_ex.h"
#include "atparser_ex.h"
#include "sessionmgr.h"
#include "comctrl_ex.h"
#include "devmgr.h"

typedef struct tagBLOCKS_INFO
{
    BLOCK_TYPE  eBlockType;
    UINT32      dwBlockID;
    PVOID       (*pfnGetInterface)(void);

} BLOCK_INFO, *PBLOCK_INFO;

BLOCK_INFO  gBlockInfo[] =
{
    { BLOCK_COM_CTRL,    0, ComCtrlGetInterface      },
    { BLOCK_MEM_MGR,     0, MemMgrGetInterface       },
    { BLOCK_RT_MGR,      0, RTMgrGetInterface        },
//    { BLOCK_DEBUG_MGR,   0, DbgMgrGetInterface  },
    { BLOCK_STATUS_MGR,  0, StatMgrGetInterface      },
    { BLOCK_CONFIG_MGR,  0, ConfigMgrGetInterface    },
#ifndef NO_SESSIONS
	{ BLOCK_DEVICE_MGR,	 0, DevMgrGetInterface		 },
    { BLOCK_SESSION_MGR, 0, SessionMgrGetInterface   },
#endif    
    { BLOCK_NVM_MGR,     0, NVMMgrGetInterface       },
    { BLOCK_AT_DTE,      0, AtDteGetInterface        }

};
#define BLOCK_NUM    ( sizeof(gBlockInfo)/sizeof(BLOCK_INFO) )


HANDLE  ConnectBlock(BLOCK_TYPE BlockType, UINT32 BlockID)
{
    int i;
    for ( i = 0; i < BLOCK_NUM; i++ )
    {
        if( (gBlockInfo[i].eBlockType  ==  BlockType ) &&
            (gBlockInfo[i].dwBlockID   ==  BlockID)         )
        {
            return (HANDLE)&gBlockInfo[i];
        }
    }
    return NULL;
}

VOID DisconnectBlock(HANDLE hBlock)
{
}

PVOID GetInterfaceFuncTbl(HANDLE hBlock)
{
    PBLOCK_INFO pBlock = (PBLOCK_INFO)hBlock;

    if( pBlock == NULL || pBlock->pfnGetInterface == NULL )
        return NULL;

    return pBlock->pfnGetInterface();
}
