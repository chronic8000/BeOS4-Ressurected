/*
 * $Log:   V:/Flite/archives/OSAK/src/docbdk.h_V  $
 * 
 *    Rev 1.1   Apr 06 2000 20:11:48   vadimk
 * Names are changed in the BDK struct
 *
 *    Rev 1.0   Jan 13 2000 17:48:44   vadimk
 * Initial revision.
 *
 */
/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-99              */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef _DOC_BDK_H_
#define _DOC_BDK_H_
#ifdef BDK_ACCESS


typedef struct {
unsigned char oldSign[4];
unsigned char newSign[4];
unsigned char signOffset;
unsigned long startingBlock;
unsigned long length;
unsigned char flags;
unsigned char FAR2 *bdkBuffer;
} BDKStruct;

extern FLStatus bdkReadInit(IOreq FAR2 *ioreq);
extern FLStatus bdkReadBlock(IOreq FAR2 *ioreq);
extern FLStatus bdkWriteInit(IOreq FAR2 *ioreq);
extern FLStatus bdkWriteBlock(IOreq FAR2 *ioreq);
extern FLStatus bdkErase(IOreq FAR2 *ioreq);
extern FLStatus bdkCreate(IOreq FAR2 *ioreq);

#endif
#endif