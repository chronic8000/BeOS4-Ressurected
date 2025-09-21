
/*
 * $Log:   V:/Flite/archives/FLite/src/FATLITE.H_V  $
   
      Rev 1.24   Mar 12 2000 20:03:40   veredg
   Remove include for fsapi.h
   The file system API shall be in blockdev.h from now on.
   
      Rev 1.23   Feb 03 2000 17:06:58   vadimk
   added include for
   flioctl.h,docbdk.h and fatfilt.h

      Rev 1.22   Jan 13 2000 18:11:52   vadimk
   TrueFFS OSAK 4.1

      Rev 1.21   22 Dec 1998 15:07:14   marina
   Block Device Driver and File System were separated

      Rev 1.20   09 Nov 1998 20:03:28   marina
   user call FL_FLUSH_BUFFER was added

      Rev 1.19   25 Oct 1998  3:22:48   Dimitry
   Add flSectorsInVolume function
   Add passing irFlags to physicalRead and physicalWrite to
     manage EDC and EXTRA area access

      Rev 1.18   16 Aug 1998 20:30:42   amirban
   Added ZIP_FORMAT option

      Rev 1.17   01 Mar 1998 13:00:20   amirban
   Add flAbsAddress

      Rev 1.16   26 Nov 1997 18:21:18   danig
   Call updateSocketParams from flCall

      Rev 1.15   11 Nov 1997 17:29:36   danig
   flUpdateSocketParams

      Rev 1.14   05 Nov 1997 17:10:58   danig
   flParsePath documentation

      Rev 1.13   11 Sep 1997 15:24:04   danig
   FlashType -> unsigned short

      Rev 1.12   10 Aug 1997 18:02:26   danig
   Comments

      Rev 1.11   04 Aug 1997 13:18:28   danig
   Low level API

      Rev 1.10   07 Jul 1997 15:23:24   amirban
   Ver 2.0

      Rev 1.9   21 Oct 1996 18:10:10   amirban
   Defragment I/F change

      Rev 1.8   17 Oct 1996 16:19:24   danig
   Audio features

      Rev 1.7   20 Aug 1996 13:22:44   amirban
   fsGetBPB

      Rev 1.6   14 Jul 1996 16:47:44   amirban
   Format Params

      Rev 1.5   04 Jul 1996 18:19:24   amirban
   New fsInit and modified fsGetDiskInfo

      Rev 1.4   09 Jun 1996 18:16:56   amirban
   Added fsExit

      Rev 1.3   03 Jun 1996 16:20:48   amirban
   Separated fsParsePath

      Rev 1.2   19 May 1996 19:31:16   amirban
   Got rid of aliases in IOreq

      Rev 1.1   12 May 1996 20:05:38   amirban
   Changes to findFile

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


#ifndef FATLITE_H
#define FATLITE_H

#include "blockdev.h"
#include "dosformt.h"
#include "fatfilt.h"
#include "flioctl.h"
#include "docbdk.h"


#endif