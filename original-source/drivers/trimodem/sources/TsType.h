/*========================================================================
   File:    TsType.h
   Purpose: This file contains the data types used throughout TS code.

   Author:  Alain Clermont, 99-10-07
   Modifications:
			Bryan Pong, 2000-07

                    Copyright (C) 1991 by Trisignal communications Inc.
                                     All Rights Reserved
  =========================================================================*/
#ifndef TSTYPE_H
#define TSTYPE_H

#define CONST   const


/* Data types which may be machine dependent for portability
*/
typedef char                CHAR;            /* 8-bit integer */
typedef char                INT8;            /* 8-bit integer */
typedef char                BOOLEAN;         /* TRUE/FALSE sentinel */
typedef unsigned char       BYTE;            /* unsigned for 8-bit usage */
typedef unsigned char       UINT8;           /* 8-bit unsigned integer */
typedef unsigned char       BOOL;
typedef unsigned char       logical;
typedef short int           INT16;           /* 16-bit integer */
typedef unsigned short      WORD;
typedef unsigned short int  UINT16;          /* 16-bit unsigned integer */
typedef long                INT32; 
typedef unsigned long       UINT32; 
typedef unsigned long       DWORD;
typedef long unsigned int   size_t;

#endif //define TSTYPE_H
