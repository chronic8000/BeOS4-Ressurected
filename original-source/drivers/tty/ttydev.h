/* ++++++++++
	FILE:	ttydev.h
	REVS:	$Revision$
	NAME:	rico
	DATE:	Fri Jan 05 11:27:53 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _TTYDEV_H
#define _TTYDEV_H

#ifndef _OS_H
#include		<OS.h>
#endif

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

/* ---
	ioctl ids
--- */

enum {
	TTYGETINFO = B_DEVICE_OP_CODES_END + 1
};

#define	TTYGETA			1
#define	TTYSETA			2
#define	TTYSETAW		3
#define	TTYSETAF		4
#define	TTYREAD			5
#define	TTYWRITE		6
#define	TTYBLOCK		7
#define	TTYNONBLOCK		8
#define TTYRABORT       9
#define TTYGETWINSIZE  10
#define TTYSETWINSIZE  11

#define	MAX_PACKET_SIZE	8192

typedef struct tty_info {
	port_id		qport;
	port_id		rport;
	port_id		wport;
	port_id		cport;
} tty_info;

#endif
