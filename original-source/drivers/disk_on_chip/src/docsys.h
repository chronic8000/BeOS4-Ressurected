/*
 * $Log:   V:/Flite/archives/FLite/src/DOCSYS.H_V  $
   
      Rev 1.6   Jan 24 2000 14:34:00   vadimk
   LSB changed to FL_LSB

      Rev 1.5   Jan 13 2000 18:07:14   vadimk
   TrueFFS OSAK 4.1

      Rev 1.3   14 Apr 1999 14:13:18   marina
   NDOC2window FAR0* was changed to NDoc2Window

      Rev 1.2   08 Apr 1999 11:53:32   marina
   change include nfdc2148.h to include diskonc.h

      Rev 1.1   25 Feb 1999 11:58:50   marina
   fix include problem

      Rev 1.0   23 Feb 1999 21:05:40   marina
   Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1999			*/
/*									*/
/************************************************************************/

#ifndef DOCSYS_H
#define DOCSYS_H

/*#define USE_FUNC*/

#define DOC_ACCESS_TYPE        8

#if DOC_ACCESS_TYPE == 8
typedef unsigned char  DOCAccessType;
#define DOC_DATA   DUB
#endif
#if DOC_ACCESS_TYPE == 16
typedef unsigned short DOCAccessType;
#define DOC_DATA   DBL
#endif
#if DOC_ACCESS_TYPE == 32
typedef unsigned long  DOCAccessType;
#define DOC_DATA   DDBL
#endif
/*-----------------------------------------------*/
#define DOC_WIN    (0x400L * DOC_ACCESS_TYPE)

#define FL_LSB(x)     (0x00ff & (x))
#define DUB(x)     ( (unsigned char)FL_LSB(x) )
#define DBL(x)     ( (unsigned char)FL_LSB(x) * 0x101u )
#define DDBL(x)    ( DBL(FL_LSB(x)) * 0x10001l )

#include "diskonc.h"

extern void docread(FLFlash vol,unsigned regOffset,void FAR1* dest,unsigned int count);
extern void docwrite(FLFlash vol,void FAR1* src,unsigned int count);
extern void docset(FLFlash vol,unsigned int count, unsigned char val);

#ifdef USE_FUNC
extern void flWrite8bitReg(FLFlash vol, unsigned offset,Reg8bitType Data);
Reg8bitType flRead8bitReg(FLFlash vol,unsigned offset);
extern void flPreInitWrite8bitReg(unsigned driveNo,NDOC2window winBase,
                           unsigned offset,Reg8bitType Data);
extern Reg8bitType flPreInitRead8bitReg(unsigned driveNo,NDOC2window winBase,
                                 unsigned offset);


#else

#define flWrite8bitReg(vol,offset,Data) (*((volatile DOCAccessType FAR0 *)NFDC21thisVars->win + offset)) = DOC_DATA(Data)
#define flRead8bitReg(vol,offset) (Reg8bitType)(*((volatile DOCAccessType FAR0 *)NFDC21thisVars->win + offset))
#define flPreInitWrite8bitReg(driveNo,winBase,offset,Data) (*((volatile DOCAccessType FAR0 *)winBase + offset)) = DOC_DATA(Data)
#define flPreInitRead8bitReg(driveNo,winBase,offset) (Reg8bitType)(*((volatile DOCAccessType FAR0 *)winBase + offset))

#endif /* USE_FUNC_FOR_ACCESS */
#endif