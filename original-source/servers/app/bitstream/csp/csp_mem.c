/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 94                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/************************** C S P _ M E M . C ********************************
 *                                                                           *
 * Character shape player memory manager.                                    *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_MEM.C 7.2 1997/03/18 16:04:28 SHAWN release $
 *                                                                                    
 *     $Log: CSP_MEM.C $
 *     Revision 7.2  1997/03/18 16:04:28  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:05:30  mark
 *     Release
 * Revision 6.1  96/08/27  11:58:04  mark
 * Release
 * 
 * Revision 5.3  96/07/02  14:13:37  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.2  96/03/21  11:42:17  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:21:04  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:57  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:37  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:33  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:45:16  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/

#ifndef CSP_DEBUG
#define CSP_DEBUG   0
#endif

#if CSP_DEBUG
#include <stdio.h>
#endif

#include "csp_int.h"                    /* Public and internal header */

#if PROC_TRUEDOC || ! INCL_TPS

#if CSP_DEBUG
#include "csp_dbg.h"
#endif


/***** Local function prototypes *****/
#if CSP_DEBUG >= 2
LOCAL_PROTO
void ShowMemoryStatus(
    cspGlobals_t *pCspGlobals,
    char *str);
#endif
/***** End of local function prototypes *****/

/***** Pointer alignment macros *****/
#if STRUCTALIGN == 1        /* Single byte alignment? */
#define ALIGNUP(ptr)
#define ALIGNDN(ptr)
#else
#if STRUCTALIGN == 2        /* 2-byte alignment? */
#define ALIGNUP(ptr) ptr = (ufix8 *)(((long)(ptr) + 1) & ~1)
#define ALIGNDN(ptr) ptr = (ufix8 *)((long)(ptr) & ~1)
#else                       /* Quad byte alignment? */
#define ALIGNUP(ptr) ptr = (ufix8 *)(((long)(ptr) + 3) & ~3)
#define ALIGNDN(ptr) ptr = (ufix8 *)((long)(ptr) & ~3)
#endif
#endif
/***** End of pointer alignment macros *****/


FUNCTION
btsbool CspInitAlloc(
    cspGlobals_t **pCspContext,
    void *pBuffer,
    long buffSize)
/*
 *  Initializes the memory allocation mechanism 
 *  Returns TRUE if successful, FALSE if buffer pointer is 
 *  BTSNULL or buffer size after alignment is negative or zero.
 *
 */
{
ufix8 *pAllocLimitLo;
ufix8 *pAllocLimitHi;
cspGlobals_t *pCspGlobals;

if (pBuffer == BTSNULL)
    {
    return FALSE;
    }

pAllocLimitLo = (ufix8 *)pBuffer;
pAllocLimitHi = pAllocLimitLo + buffSize;
ALIGNUP(pAllocLimitLo);
pCspGlobals = (cspGlobals_t *)pAllocLimitLo;
pAllocLimitLo += sizeof(cspGlobals_t);
ALIGNUP(pAllocLimitLo);
pCspGlobals->pAllocLimitLo = pAllocLimitLo;
pCspGlobals->pAllocNextLo = pAllocLimitLo;
ALIGNDN(pAllocLimitHi);
pCspGlobals->pAllocLimitHi = pAllocLimitHi;
pCspGlobals->pAllocNextHi = pAllocLimitHi;
if (pCspGlobals->pAllocNextLo >= pCspGlobals->pAllocNextHi)
    {
    return FALSE;
    }

*pCspContext = pCspGlobals;

#if CSP_DEBUG >= 2
ShowMemoryStatus(pCspGlobals,"InitAlloc");
#endif

return TRUE;
}


FUNCTION
void *CspAllocFixedLo(
    cspGlobals_t *pCspGlobals,
    fix31 nBytes)
/*
 *  Allocate fixed block of nBytes bytes at the low address end of
 *  the memory buffer.
 *  Returns pointer to first byte of block; BTSNULL if memory overflow
 */

{
ufix8 *retValue;

ALIGNUP(pCspGlobals->pAllocNextLo);
retValue = pCspGlobals->pAllocNextLo;
pCspGlobals->pAllocNextLo += nBytes;
if (pCspGlobals->pAllocNextLo > pCspGlobals->pAllocNextHi)
    {
    pCspGlobals->pAllocNextLo -= nBytes;
#if CSP_DEBUG
    printf("AllocFixedLo: Memory overflow\n");
#endif
    return BTSNULL;
    }

#if CSP_DEBUG >= 2
ShowMemoryStatus(pCspGlobals,"AllocFixedLo");
#endif

return (void *)retValue;
}


FUNCTION
fix31 CspMemoryLeft(cspGlobals_t *pCspGlobals)
/*
 *  Returns the number of bytes left in the main buffer.
 *  Note that due to alignment of each allocation, there
 *  is no guarantee that allocs totalling the returned
 *  value will be successful. It is safest to allow for
 *  worst-case alignment in which 3 bytes are lost for
 *  each alloc.
 */
{
return (ufix32)(pCspGlobals->pAllocNextHi - pCspGlobals->pAllocNextLo);
}


#if CSP_DEBUG >= 2

FUNCTION
static void ShowMemoryStatus(
    cspGlobals_t *pCspGlobals,
    char *str)
{
printf("=>%d low memory used, %d high memory used, %d bytes free after call to %s\n",
    (int)(pCspGlobals->pAllocNextLo - pCspGlobals->pAllocLimitLo),
    (int)(pCspGlobals->pAllocLimitHi - pCspGlobals->pAllocNextHi),
    (int)(pCspGlobals->pAllocNextHi - pCspGlobals->pAllocNextLo),
    str);
}
#endif
#endif /*PROC_TRUEDOC*/

