/*
 * $Log:   V:/Flite/archives/FLite/src/DOCSYS.C_V  $
 * 
 *    Rev 1.3   Jan 13 2000 18:06:16   vadimk
 * TrueFFS OSAK 4.1
 *
 *    Rev 1.2   14 Apr 1999 14:12:38   marina
 * NDOC2window FAR0* was changed to NDoc2Window
 *
 *    Rev 1.1   25 Feb 1999 11:59:12   marina
 * fix include problem
 *
 *    Rev 1.0   23 Feb 1999 21:05:22   marina
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1999			*/
/*									*/
/************************************************************************/


#include "docsys.h"

/*********************************************/
/*     Miscellaneous routines                */
/*********************************************/

#ifdef USE_FUNC

void flWrite8bitReg(FLFlash vol, unsigned offset,Reg8bitType Data)
{
  (*((volatile DOCAccessType FAR0 *)NFDC21thisVars->win + offset)) = DOC_DATA(Data);
}

void flPreInitWrite8bitReg(unsigned driveNo,NDOC2window winBase,
                           unsigned offset,Reg8bitType Data)
{
  (*((volatile DOCAccessType FAR0 *)winBase + offset)) = DOC_DATA(Data);
}

Reg8bitType flRead8bitReg(FLFlash vol,unsigned offset)
{
  return( (Reg8bitType)(*((volatile DOCAccessType FAR0 *)NFDC21thisVars->win + offset)) );
}

Reg8bitType flPreInitRead8bitReg(unsigned driveNo,NDOC2window winBase,
                                 unsigned offset)
{
  return( (Reg8bitType)(*((volatile DOCAccessType FAR0 *)winBase + offset)) );
}
#endif
void docread(FLFlash vol,unsigned regOffset,void FAR1* dest,unsigned int count)
{
 #if DOC_ACCESS_TYPE == 8
 my_memcpy(dest,(void FAR0*)((NDOC2window)NFDC21thisWin+regOffset),count);
 #else
  register int  i;
  for(i=0;( i < count );i++)
    *((Reg8bitType FAR1 *)dest + i) = flRead8bitReg(&vol, regOffset + i);
#endif
}

void docwrite(FLFlash vol,void FAR1* src,unsigned int count)
{
 #if DOC_ACCESS_TYPE == 8
 my_memcpy((void FAR0*)((NDOC2window)NFDC21thisWin+NFDC21thisIO),src,count);
 #else
  register int  i;
  for(i=0;( i < count );i++)
    flWrite8bitReg(&vol, NFDC21thisIO + i, *((Reg8bitType FAR1 *)src + i));
  #endif
}

void docset(FLFlash vol,unsigned int count, unsigned char val)
{
#if DOC_ACCESS_TYPE == 8
my_memset((void FAR0*)((NDOC2window)NFDC21thisWin+NFDC21thisIO),val,count);
#else
  register int  i;
  for(i=0;( i < count );i++)
    flWrite8bitReg(&vol, NFDC21thisIO + i, val);
#endif
}

