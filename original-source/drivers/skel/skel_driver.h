/*
   	This file is where you can define public interfaces
	to your driver (device control opcodes, structs, etc).
*/

#ifndef _SKEL_H
#define _SKEL_H

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

/* -----
	ioctl opcodes
----- */

enum {
	SKEL_SLOW = B_DEVICE_OP_CODES_END + 1,
	SKEL_FAST,
	SKEL_REPORT_STATISTICS
};

/* -----
	structure returned from statistics call
----- */

typedef struct {
	long	num_reads;
	long	num_writes;
	long	current_clients;
} skel_stats;


#endif  /* _SKEL_H */

