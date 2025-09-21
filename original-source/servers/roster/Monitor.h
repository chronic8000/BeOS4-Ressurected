/*****************************************************************************

	File : Monitor.h

	Written by: Peter Potrebic

	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef MONITOR_H
#define MONITOR_H

#include <Messenger.h>
#include <Message.h>
#include <List.h>

/* --------------------------------------------------------------- */

struct message_map {
	uint32	bit_flag;
	uint32	what;
};

/* --------------------------------------------------------------- */

struct listener {
	listener(BMessenger m, uint32 a) : mess(m), event_mask(a) {}
	BMessenger	mess;
	uint32		event_mask;
};

/* --------------------------------------------------------------- */

class TMonitor {
public:
					TMonitor(const message_map *map);
virtual				~TMonitor();

		status_t	Add(const BMessenger &target, uint32 mask);
		status_t	Remove(const BMessenger &target);
		status_t	Notify(BMessage *msg);

private:
		bool		event_mask_hit(uint32 what, uint32 mask);

		BList		fListeners;
		message_map	fMap[32];
};

/* --------------------------------------------------------------- */

#endif
