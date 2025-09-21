/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : ObjPrint.h	1.6
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Printing of objectfile related items
 *
 *  Last update              : 15:58:13 - 98/03/11
 *
 *  Description              :  
 *
 *                         This module defines a number of functions
 *                         for printing various objectfile items.
 *                         The format of the functions is such that they
 *                         can be used as the second parameter to the
 *                         ModBld_traverse functions defined in ModuleBuild.h
 */

#ifndef  ObjPrint_INCLUDED
#define  ObjPrint_INCLUDED

/*---------------------------- Includes --------------------------------------*/

#include <stdio.h>
#include "TMObj.h"
#include "ModuleBuild.h"

/*---------------------------- Defines ---------------------------------------*/

#define OP_VERBOSE                0x000000001
#define OP_HEADER_ALWAYS          0x000000002
#define OP_HEADER_NEVER           0x000000004
#define OP_DATA                   0x000000008
#define OP_DISASSEMBLE            0x000000010
#define OP_SCATTER                0x000000020
#define OP_RECURSE_SECTIONS       0x000000040
#define OP_RECURSE_SECTIONUNITS   0x000000080
#define OP_RECURSE_SYMBOLS        0x000000100
#define OP_RECURSE_REFERENCES     0x000000200
#define OP_NO_SECTIONS            0x000000400
#define OP_NO_SECTIONUNITS        0x000000800
#define OP_NO_SYMBOLS             0x000001000
#define OP_NO_REFERENCES          0x000002000
#define OP_SYSTEM                 0x000004000

/*----------------------------- Types ----------------------------------------*/

typedef struct ObjPrint_Args {
        FILE                *stream;   /* output stream */
        UInt32           count;    /* counter of how many times a function was called */
        UInt32           flags;    /* a combination of the OP_ flags defined above */
        Pointer                data;     /* possible extra data */
} ObjPrint_Args;

/*--------------------------- Functions --------------------------------------*/

void ObjPrint_Set_Header_Freq(
        UInt32             header_freq
     );

void ObjPrint_Module(
        ModBld_Module          module,
        Pointer                  args
     );

void ObjPrint_Section(
        ModBld_Module          module,
        TMObj_Section          section,
        Pointer                  args
     );

void ObjPrint_SectionUnit(
        ModBld_Module          module,
        TMObj_SectionUnit      section,
        Pointer                  args      /* data is "bytes" pointer from section */
     );

void ObjPrint_Symbol(
        ModBld_Module          module,
        TMObj_Symbol           symbol,
        Pointer                  args
     );

void ObjPrint_Marker_Reference(
        ModBld_Module          module,
        TMObj_Marker_Reference marker_reference,
        Pointer                  args
     );

void ObjPrint_Symbol_Reference(
        ModBld_Module          module,
        TMObj_Symbol_Reference symbol_reference,
        Pointer                  args
     );

void ObjPrint_JTab_Reference(
        ModBld_Module          module,
        TMObj_JTab_Reference jtab_reference,
        Pointer                  args
     );

void ObjPrint_ExternalModule(
        ModBld_Module          module,
        TMObj_External_Module  extmod,
        Pointer                  args
     );

void ObjPrint_Data            (
        FILE                  *st,
        UInt8             *data,
        UInt32            length
     );

void ObjPrint_Disassemble     (
        FILE                  *st,
        UInt8             *data,
        UInt32            length
     );

#endif /* ObjPrint_INCLUDED */
     
   
