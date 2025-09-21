/* ++++++++++
	FILE:	sam.h
	REVS:	$Revision$
	NAME:	pierre
	DATE:	Thu Jun 12 09:58:25 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _SAM_H
#define _SAM_H

#include <SupportDefs.h>

/* select SAM mode */
#define SAM_OFF   0
#define SAM_TEST  1
#define SAM_SET   2

#define SAM_MODE  SAM_SET

/* sam mode macro dispatcher */
#if (SAM_MODE == SAM_OFF)
    #define   sam_thread_in(x)    {;}
    #define   sam_thread_status() {;}
    #define   sam_thread_out()    {;}
    #define   sam_init()          {;}
    #define   sam_end()           {;}
#elif (SAM_MODE == SAM_TEST)
    #define   sam_thread_in(x)    sam_thread_inR(x)
    #define   sam_thread_status() sam_thread_statusR()
    #define   sam_thread_out()    sam_thread_outR()
    #define   sam_init()          sam_initR()
    #define   sam_end()           sam_endR()
#elif (SAM_MODE == SAM_SET)
    #define   sam_thread_in(x)    sam_thread_setR(x)
    #define   sam_thread_status() {;}
    #define   sam_thread_out()    {;}
    #define   sam_init()          {;}
    #define   sam_end()           {;}
#endif

#ifdef __cplusplus
extern "C" {
#endif

	void sam_thread_setR(int);
	void sam_thread_inR(int);
	void sam_thread_statusR(void);
	void sam_thread_outR(void);
	void sam_initR(void);
	void sam_endR(void);

#ifdef __cplusplus
}
#endif

#endif








