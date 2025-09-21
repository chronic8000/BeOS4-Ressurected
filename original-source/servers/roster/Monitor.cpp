/*****************************************************************************

	File : Monitor.cpp

	Written by: Peter Potrebic

	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <string.h>
#include <Messenger.h>
#include <Message.h>

#include "Monitor.h"

TMonitor::TMonitor(const message_map *map)
	: fListeners(10)
{
	int32	i = 0;

	while (map && map->bit_flag) {
		fMap[i].bit_flag = map->bit_flag;
		fMap[i].what = map->what;

		if (i == 31)
			break;
		i++;
		map++;
	}
	fMap[i].bit_flag = 0;
	fMap[i].what = 0;
}

/*---------------------------------------------------------------*/

TMonitor::~TMonitor()
{
}

/*---------------------------------------------------------------*/

status_t TMonitor::Add(const BMessenger &target, uint32 event_mask)
{
	status_t	err = B_OK;
	int			i;
	listener	*alistener;

	// check to make sure that given target isn't already listed
	bool	found = false;
	i = 0;
	while ((alistener = (listener *) fListeners.ItemAt(i++)) != 0) {
		if (target == alistener->mess) {
			found = true;
			break;
		}
	}
	if (found) {
		alistener->event_mask = event_mask;
	} else {
//+		PRINT(("Adding monitor: team=%d, mask=%x\n",
//+			target.Team(), event_mask));
		if (!fListeners.AddItem(new listener(target, event_mask)))
			err = B_ERROR;
	}

	return err;
}

/*---------------------------------------------------------------*/

status_t TMonitor::Remove(const BMessenger &target)
{
	status_t	err;
	listener	*alistener;
	bool		found = false;
	int			i = 0;
	
	while ((alistener = (listener *) fListeners.ItemAt(i)) != 0) {
		if (target == alistener->mess) {
			found = true;
			break;
		}
		i++;
	}
	if (found) {
//+		PRINT(("Removing monitor: team=%d\n",
//+			target.Team()));
		err = fListeners.RemoveItem(i) ? B_OK : B_ERROR;
		delete alistener;
	} else {
		err = B_BAD_VALUE;
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t TMonitor::Notify(BMessage *msg)
{
	int32		i = 0;
	listener	*alistener;
	status_t	err;

//+	PRINT(("Notify(count=%d)\n", fListeners.CountItems()));

	while ((alistener = (listener *) fListeners.ItemAt(i++)) != NULL) {
		uint32		event_mask = alistener->event_mask;
		BMessenger	mess = alistener->mess;

		if (!event_mask_hit(msg->what, event_mask)) {
//+			PRINT(("\tMiss: team=%d, what=%.4s, mask=%x\n",
//+				mess.Team(), (char *) &(msg->what), event_mask));
			continue;
		}

//+		PRINT(("\tNotify: team=%d, what=%.4s\n",
//+			mess.Team(), (char *) &(msg->what)));
		if ((err = mess.SendMessage(msg, (BHandler *) NULL, 0.0)) != B_OK) {
//+			PRINT(("SendMessage err = %x (%s)\n", err, strerror(err)));
			// error talking to one of the listeners.
			// remove it from the list
//+			PRINT(("Removing app from the list\n"));
			i--;
			fListeners.RemoveItem(i);
			delete alistener;
		}
	}
	return B_OK;
}

/*---------------------------------------------------------------*/

bool TMonitor::event_mask_hit(uint32 what, uint32 mask)
{
	int32		i = 0;
	bool		val = false;

//+	PRINT(("what=%.4s, mask=%x\n", (char*) &what, mask));

	while (fMap[i].bit_flag != 0) {
//+		PRINT(("	compare(%x, %x)\n", fMap[i].what, what));
		if (fMap[i].what == what) {
			val = (mask & fMap[i].bit_flag) != 0;
//+			PRINT(("		bit=%x, val=%d\n", fMap[i].bit_flag, val));
			break;
		}
		i++;
	}

	return val;
}


/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
