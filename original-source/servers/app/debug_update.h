//******************************************************************************
//
//	File:		debug_update.h
//
//	Description:	Gather debug information about the update mechanism.
//	
//	Written by:	Pierre Raynaud-Richard
//
//	Copyright 1998, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#ifndef _DEBUG_UPDATE_H
#define _DEBUG_UPDATE_H

#include <SupportDefs.h>
#include <window.h>

/* select DEBUG_UPDATE mode */
#define DUP_OFF		0
#define DUP_ON		1

#define DUP_MODE	DUP_OFF

#define	DUP_LEVEL_COUNT			11

#define DUP_SUBMIT_UPDATE		0
#define DUP_NOTIFY_DONE_FLAG	1
#define DUP_DELAY_UPDATE		2
#define DUP_PORT_COUNT			3
#define DUP_TRIGGER_UPDATE		4
#define	DUP_REQUEST_UPDATE		5
#define	DUP_START_UPDATE		6
#define DUP_FULL_COUNT			7
#define	DUP_UPDATE_READY		8
#define	DUP_UPDATE_DONE			9
#define	DUP_END_UPDATE			10

#define	DUP_DELAY	10000000

/* sam mode macro dispatcher */
#if (DUP_MODE == DUP_OFF)
    #define   dup_init()          			{;}
    #define   dup_end()           			{;}
    #define   dup_create_window(a)  		{;}
    #define   dup_release_window(a) 		{;}
    #define   dup_tag_window(a,b)   		{;}
    #define   dup_first_tag_window(a,b)		{;}
    #define   dup_count_tag_window(a,b,c)	{;}
    #define   dup_counter_window(a,b)		{;}
#elif (DUP_MODE == DUP_ON)
    #define   dup_init()          			dup_initR()
    #define   dup_end()           			dup_endR()
    #define   dup_create_window(a)  		dup_create_windowR(a)
    #define   dup_release_window(a) 		dup_release_windowR(a)
    #define   dup_tag_window(a,b)   		dup_tag_windowR(a,b)
    #define   dup_first_tag_window(a,b)		dup_first_tag_windowR(a,b)
    #define   dup_count_tag_window(a,b,c)	dup_count_tag_windowR(a,b,c)
    #define   dup_counter_window(a,b)		dup_counter_windowR(a,b)
#endif

	void dup_initR();
	void dup_endR();
	void dup_create_windowR(TWindow *w);
	void dup_release_windowR(TWindow *w);
	void dup_tag_windowR(TWindow *w, int32 level);
	void dup_first_tag_windowR(TWindow *w, int32 status);
	void dup_count_tag_windowR(TWindow *w, int32 level, int32 status);
	void dup_counter_windowR(TWindow *w, int32 level);

#endif








