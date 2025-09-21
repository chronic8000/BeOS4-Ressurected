/*========================================================================
   File:	DevelopEnvironDef.h
   Purpose:	Description of each compiler behavior for various aspect
            such as bit field ordering.  This mechanism relies on a
            per-compiler unique define (or build environment).

            This file is included in common.h .

   Author:	Alain Clermont , 1999-10-12
   Modifications:	
			Bryan Pong, 2000-07

             		Copyright (C) 1999 by Trisignal communications Inc.
                            	     All Rights Reserved
=========================================================================*/
#ifndef DEVELOPENVIRONDEF_H
#define DEVELOPENVIRONDEF_H

/***************************************************************************
    BIT ORDERING    BIT ORDERING    BIT ORDERING    BIT ORDERING   
 ***************************************************************************/
#define BIT_FIELD_ORDERING    BIT_FIELD_LOW_TO_HIGH

/***************************************************************************
    SHARED LIBRARY IMPORT/EXPORT     SHARED LIBRARY IMPORT/EXPORT    SHAR
 ***************************************************************************/
/* These are optional macros used to import/export functions and data
   into/from a shared library. */
#define SHARED_LIB_EXPORT
#define SHARED_LIB_IMPORT
#define SHARED_LIB_IGNORE_DLL_IF_WARNING
#define SHARED_LIB_CONSIDER_DLL_IF_WARNING


/***************************************************************************
                            MISC. KEY WORDS
 ***************************************************************************/
#if defined VXD_PASSIVE || defined __SHC__ || defined __CODEWARRIOR__
#define interrupt 
#define packed
#endif

#endif  //DEVELOPENVIRONDEF_H
