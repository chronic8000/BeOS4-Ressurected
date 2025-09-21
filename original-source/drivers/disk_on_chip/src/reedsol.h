
/*
 * $Log:   V:/reedsol.h_v  $
   
      Rev 1.7   06 Oct 1997 13:19:02   ANDRY
   ERROR -> EDC_ERROR

      Rev 1.6   10 Sep 1997 16:16:46   danig
   Got rid of generic names

      Rev 1.5   04 Sep 1997 10:03:38   DIMITRY
   ECC/EDC in hardware

      Rev 1.4   07 Jul 1997 15:24:02   amirban
   Ver 2.0

      Rev 1.3   27 May 1997 11:10:48   danig
   Changed far to FAR1

      Rev 1.2   25 May 1997 16:42:08   amirban
   Up-to-date

      Rev 1.1   18 May 1997 17:57:28   danig
   Changed NO_ERROR to NO_EDC_ERROR

      Rev 1.0   08 Apr 1997 18:36:24   danig
   Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/


#ifndef FLEDC_H
#define FLEDC_H

#include "flbase.h"

typedef enum { NO_EDC_ERROR, CORRECTABLE_ERROR, UNCORRECTABLE_ERROR, EDC_ERROR } EDCstatus;

EDCstatus flCheckAndFixEDC(char FAR1 *block, char *syndrom, FLBoolean byteSwap);

#endif
