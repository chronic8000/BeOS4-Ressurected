/* ++++++++++
	FILE:	entry.c
	REVS:	$Revision: 1.17 $
	NAME:	herold
	DATE:	Wed May 10 16:59:50 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
++++ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <debugger.h>
#include "thread.h"
#include "cmd.h"
#include "io.h"
#include "cwindow.h"
#include "simple.h"
#include "launcher.h"

#define PRINT(x)

extern void cmd_exit (rec_thread *thr, io_rec *ior);

/* ----------
	debug_team is the main loop for debugging a team of threads.
----- */
long
debug_team (void *arg)
{
	long         ret;
	rec_team	*tmr;
	rec_thread	thread_rec;
	io_rec		*ior;
	thread_info tinfo;
	int			useDebugger = 0;
	bool		loadsymbols = true;
	char		*bedebug = getenv("BEDEBUG");
	char		*bedebugsyms = getenv("BEDEBUGSYMBOLS");

	if (bedebug != NULL) {
		if ( (strcasecmp(bedebug, "false") != 0) && 
			 (strcasecmp(bedebug, "0") != 0) )
			useDebugger = 1;
	}
	if (bedebugsyms != NULL) {
		if ( (strcasecmp(bedebugsyms, "false") == 0) || 
			 (strcasecmp(bedebugsyms, "0") == 0) )
			loadsymbols = false;
	}

	/*	if this is the app_server, or if the app_server is not yet initialized,
		do not even attempt to put up a dialog */
	ret = get_thread_info(find_thread("picasso"), &tinfo);
	if (ret || (tinfo.team == (team_id) arg))
	{
		useDebugger = 1;
	}

	tmr = find_team ((team_id) arg);

	if (!(ior = new_tokenizer(tmr->team)))
		return B_ERROR;	/* *** WHAT SHOULD WE DO HERE *** */

	for (;;) {
		acquire_sem (tmr->thread_waiting_sem);	/* wait for a stopped thread */

		if (tmr->flags & TEAM_DELETABLE)		/* no more team? */
			break;

		for (;;) {
			check_all_threads (tmr);

			if (find_debuggable_thread (tmr, &thread_rec) != B_NO_ERROR)
				break;

			if (useDebugger == 0) {
				int simpleresult = simple_debug_message(&thread_rec, ior);
				if (simpleresult == 0) {
					ret = -1;
					break;
				}
				loadsymbols = simpleresult == 1;
				useDebugger = 1;
			}

			if(loadsymbols && !tmr->sym) {
				/*bigtime_t t1;*/
				dbprintf (ior, "loading symbols\n");
				fflush(ior->outfp);
				/*t1 = system_time();*/
				load_all_symbols (tmr);
				/*dbprintf (ior, "done %2.2f seconds\n", (double)(system_time()-t1)/1000000.0);*/
			}
			else if(!loadsymbols && !tmr->sym) {
				load_no_symbols (tmr);
			}

			ret = operate_on_thread (&thread_rec, ior);
			if (ret < 0)
				break;
			PRINT(("after operate_on_thread\n"));
			commit_thread_state (tmr, &thread_rec);
		}

		if (ret < 0)
			break;
	}

	if ((ret == -2) || (ret == -3)) {
		port_id new_db_port = tmr->new_db_port;
		team_id id = tmr->team;
		team_id debugger_id;
		delete_tokenizer(ior);
		delete_team(tmr);
		debugger_id = (ret == -2) ? launch_debugger(new_db_port, id) : launch_debug_proxy(new_db_port, id);
	} else {
		cmd_exit(&thread_rec, ior);
		delete_tokenizer(ior);
		delete_team(tmr);
	}

	return 0;
}

/* ----------
	thread_stopped handles the thread_stopped message from the kernel
----- */

void
thread_stopped (db_thread_stopped_msg *msg)
{
	rec_team	*tmr;
	rec_thread	*thr;

	/* lookup team record, or create new one */
	if (!(tmr = find_team (msg->team)))
		if (!(tmr = new_team (msg->team, msg->nub_port, debug_team)))
			return;

	/* lookup thread, save thread's cpu state */

	thr = find_or_new_thread (tmr, msg->thread);
	thr->why = msg->why;
	thr->cpu = msg->cpu;
	thr->flags |= (THREAD_DEBUGABLE + THREAD_JUST_STOPPED);
	
	release_sem (tmr->thread_waiting_sem);
}


void
team_created (db_team_created_msg *msg)
{
	/* in this debugger, we lazily register new teams when they require
	   attention from the debugger */
}


void
team_deleted (db_team_deleted_msg *msg)
{
	rec_team	*tmr;

	/* lookup team, mark as deleted */

	if ((tmr = find_team (msg->team))) {
		tmr->flags |= TEAM_DELETABLE;
		
		release_sem (tmr->thread_waiting_sem);
	}
}


void
pef_image_created (db_pef_image_created_msg *msg)
{
}


void
pef_image_deleted (db_pef_image_deleted_msg *msg)
{
}


void
thread_created (db_thread_created_msg *msg)
{
	/* in this debugger, we lazily register new threads when they require
	   attention from the debugger */
}


void
thread_deleted (db_thread_deleted_msg *msg)
{
	rec_team	*tmr;

	/* lookup team record */

	if (!(tmr = find_team (msg->team)))
		return;
	
	mark_thread_deletable (tmr, msg->thread);
	release_sem (tmr->thread_waiting_sem);
}
