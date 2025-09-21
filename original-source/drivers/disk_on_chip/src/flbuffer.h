
/*
 * $Log:   V:/Flite/archives/FLite/src/FLBUFFER.H_V  $
   
      Rev 1.6   Jan 13 2000 18:17:40   vadimk
   TrueFFS OSAK 4.1
   
      Rev 1.5   10 Sep 1997 16:15:52   danig
   Got rid of generic names
   
      Rev 1.4   28 Aug 1997 16:47:30   danig
   include flbase.h
   
      Rev 1.3   07 Jul 1997 15:23:28   amirban
   Ver 2.0
   
      Rev 1.2   18 Aug 1996 13:48:02   amirban
   Comments
   
      Rev 1.1   01 Jul 1996 15:41:58   amirban
   Doesn't define buffer
   
      Rev 1.0   20 Mar 1996 13:33:20   amirban
   Initial revision.
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

#ifndef FLBUFFER_H
#define FLBUFFER_H

#include "flbase.h"

typedef struct {
  unsigned char data[SECTOR_SIZE];	/* sector buffer */
  SectorNo	sectorNo;		/* current sector in buffer */
  void		*owner;			/* owner of buffer */
  FLBoolean	dirty;			/* sector in buffer was changed */
  FLBoolean	checkPoint;		/* sector in buffer must be flushed */
} FLBuffer;

#endif
