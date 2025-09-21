/*	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved. */

/*
 *  Run debug server in Unix mode (no GUI)
 *  Signals are sent to the offending process rather than popping up
 *  a debug window.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <OS.h>
#include <debugger.h>
#include "nogui.h"

#if DEBUG
#define ddprintf printf
#else
#define ddprintf (void)
#endif

#if __ELF__
#define IMAGE_DELETED B_ELF_IMAGE_DELETED
#define IMAGE_CREATED B_ELF_IMAGE_CREATED
#elif __POWERPC__
#define IMAGE_DELETED B_PEF_IMAGE_DELETED
#define IMAGE_CREATED B_PEF_IMAGE_CREATED
#endif



#ifndef SIGTRAP
#	define SIGTRAP 22  /* XXX */
#endif

void
send_sig(db_thread_stopped_msg *msg)
{
	int sig;
	to_nub_msg	outmsg;

	switch (msg->why) {

	case B_DEBUGGER_CALL:
		sig = SIGABRT;
		break;

	case B_BREAKPOINT_HIT:
		sig = SIGTRAP;
		break;

	case B_MACHINE_CHECK_EXCEPTION:
		sig = SIGFPE;
		break;

	case B_ALIGNMENT_EXCEPTION:
		sig = SIGBUS;
		break;

#if __INTEL__
	case B_SEGMENT_VIOLATION:
		sig = SIGSEGV;
		break;

	case B_SNGLSTP:
		sig = SIGTRAP;
		break;

	case B_NMI:
		sig = SIGABRT;
		break;

	case B_DIVIDE_ERROR:
		sig = SIGFPE;
		break;

	case B_OVERFLOW_EXCEPTION:
		sig = SIGFPE;
		break;

	case B_BOUNDS_CHECK_EXCEPTION:
		sig = SIGFPE;
		break;

	case B_INVALID_OPCODE_EXCEPTION:
		sig = SIGILL;
		break;

	case B_SEGMENT_NOT_PRESENT:
		sig = SIGSEGV;
		break;

	case B_STACK_FAULT:
		sig = SIGSEGV;
		break;

	case B_GENERAL_PROTECTION_FAULT:
		sig = SIGSEGV;
		break;

	case B_FLOATING_POINT_EXCEPTION:
		sig = SIGFPE;
		break;

#elif __POWERPC__

	case B_DATA_ACCESS_EXCEPTION:
		sig = SIGSEGV;
		break;

	case B_INSTRUCTION_ACCESS_EXCEPTION:
		sig = SIGILL;
		break;

	case B_PROGRAM_EXCEPTION:
		sig = SIGILL;
		break;
#endif
	default:
		sig = SIGFPE;
		break;
	}
	outmsg.nub_run_thread.thread = msg->thread;
	outmsg.nub_run_thread.cpu = msg->cpu;

	ddprintf("sending signal\n");
	kill(msg->thread, sig);
	ddprintf("resuming thread\n");
	write_port(msg->nub_port, B_RUN_THREAD, &outmsg, sizeof(outmsg));
}


void
nogui_debug_server(void)
{
	port_id				in_port;
	to_debugger_msg		msg;
	long				what;

	in_port = create_port(5, "exception catcher");
	if (in_port < B_NO_ERROR) {
		ddprintf ("create port failed\n");
		return;
	}
	install_default_debugger(in_port);
	while (read_port(in_port, &what, &msg, sizeof (msg)) >= B_NO_ERROR) {
		switch (what) {
		case B_THREAD_STOPPED:			/* thread stopped: here is its state */
			ddprintf("thread stopped\n");
			send_sig(&msg.thread_stopped);
			break;

		case B_TEAM_CREATED:			/* team was created */
			ddprintf("team created\n");
			break;

		case B_TEAM_DELETED:			/* team was deleted */
			ddprintf("team deleted\n");
			break;

		case IMAGE_CREATED:		/* pef image was created */
			ddprintf("image created\n");
			break;

		case IMAGE_DELETED:		/* pef image was deleted */
			ddprintf("image deleted\n");
			break;

		case B_THREAD_CREATED:			/* thread was created */
			ddprintf("thread created\n");
			break;

		case B_THREAD_DELETED:			/* thread was deleted */
			ddprintf("thread deleted\n");
			break;

		default:
			ddprintf("default\n");
			break;
		}
	}
}

/*
 * Determine if we should use a GUI debugger or not.
 * The command-line overrides the environment variable
 */
int
dont_use_gui(int argc, char **argv)
{
	char *str;
	
	if (argc > 1 && strcmp(argv[1], "-nogui") == 0) {
		return (1);
	}
	str = getenv("BEDEBUG");
	if (str != NULL && strcmp(str, "nogui") == 0) {
		return (1);
	}
	return (0);
}


