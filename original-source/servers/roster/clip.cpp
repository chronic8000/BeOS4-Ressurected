/*****************************************************************************

	File : clip.cpp

	$Revision: 1.8 $

	Written by: Peter Potrebic

	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <string.h>
#include <Debug.h>

#include "clip.h"
#include <Looper.h>
#include <Autolock.h>
#include <Path.h>
#include <Clipboard.h>
#include <FindDirectory.h>
#include <roster_private.h>

BList	TClipHandler::sClipList(5);
BLocker	TClipHandler::sClipLock("clip_lock");
const char *kClipFile = "clipboard";

struct message_map clip_monitor_map[] = {
	{ 0x1, B_CLIPBOARD_CHANGED },
	{ 0, 0}
};

/*------------------------------------------------------------------------*/

TClipHandler::TClipHandler(const char *name, bool transient)
	:	BHandler(name),
		fClipPath(),
		fData('clip'),
		fDataOwner(),
		fCount(0),
		fTransient(transient),
		fMonitor(NULL)
{
#if 0
	if (!fTransient) {
		if (find_directory (B_USER_SYSTEM_DIRECTORY, &fClipPath) == B_OK) {
			fClipPath.Append (kClipFile);
			int clipFile = open(fClipPath.Path(), O_CREAT | O_RDONLY, 0666);
			if (clipFile >= 0) {
				long numBytes;

				read(clipFile, &numBytes, sizeof(long));
				char *buf = (char *) malloc(numBytes);
				read(clipFile, buf, numBytes);
				close(clipFile);

				fData.Unflatten(buf);
				fCount++;
				free(buf);
			}
		}
	}
#endif
	BAutolock	a(sClipLock);
	sClipList.AddItem(this);
}

/*------------------------------------------------------------------------*/

TClipHandler::~TClipHandler()
{
	BAutolock	a(sClipLock);
	sClipList.RemoveItem(this);

	if (fMonitor)
		delete fMonitor;

#if 0
	if (!fTransient && fClipPath.Path()) {
		// The ClipHandler only get deleted when the roster_server is
		// quitting. And that means that the machine is shutting down.
		// save the data to disk.
		int clipFile = open(fClipPath.Path(), O_CREAT|O_WRONLYO_TRUNC, 0666);
		if (clipFile >= 0) {
			char *flat;
			long numBytes;
			numBytes = fData.FlattenedSize();
			flat = (char*) malloc(numBytes);
			if (flat) {
				fData.Flatten(flat, numBytes);
				write(clipFile, &numBytes, sizeof(numBytes));
				write(clipFile, flat, numBytes);
				close(clipFile);
				free(flat);
			}
		}
	}
#endif
}

/*------------------------------------------------------------------------*/

void TClipHandler::MessageReceived(BMessage *msg)
{
	BMessage reply(ROSTER_REPLY);
	status_t err = B_NO_ERROR;

	switch (msg->what) {
		/*
		 *****
		 ***** Be very defensive here. Don't assume that the correct data 
		 ***** is in the message. Can't have the roster_server crashing!!!
		 *****
		*/
		case CMD_NEW_CLIP:
			{
			const char *new_name;
			err = msg->FindString("name", &new_name);
			if (err)
				break;	// panic!! ERROR!

			// we send back a messenger to the handler for this clipboard.

			if (strcmp(new_name, Name()) != 0) {
				TClipHandler	*ch;

				ch = FindClipHandler(Looper(), new_name);
				if (!ch) {
					bool	temp = msg->FindBool("transient");
//+					SERIAL_PRINT(("creating new clip (%s)\n", new_name));
					ch = new TClipHandler(new_name, temp);
					Looper()->AddHandler(ch);
				}
//+				else {
//+					SERIAL_PRINT(("connect to existing clip (%s)\n", new_name));
//+				}
				reply.AddMessenger("messenger", BMessenger(ch));
			} else {
//+				SERIAL_PRINT(("connecting to MAIN clip (%s)\n", new_name));
				reply.AddMessenger("messenger", BMessenger(this));
			}
			break;
			}
		case CMD_GET_CLIP_DATA:
			{
			// some client want the latest clipboard data. Check the count,
			// if it matches the latest then we don't need to send any data.
			uint32 client_count;
			err = msg->FindInt32("count", (long*) &client_count);
			if (err)
				break;
			if (client_count != fCount) {
				// counts are different so we have to send data
				char *flat;
				int32 numBytes;

				numBytes = fData.FlattenedSize();
				flat = (char*) malloc(numBytes);
				if (!flat) {
					err = B_NO_MEMORY;
					break;
				}
				err = fData.Flatten(flat, numBytes);
				if (err)  {
					free(flat);
					break;		// panic!
				}

				reply.AddData("data", B_RAW_TYPE, flat, numBytes);
				reply.AddMessenger("owner", fDataOwner);
				reply.AddInt32("count", fCount);
				free(flat);
			}
			break;
			}
		case CMD_SET_CLIP_DATA:
			{
			int32		size;
			const char	*raw;

			err = msg->FindData("data", B_RAW_TYPE, (const void **)&raw, &size);
			if (err)
				break;

			++fCount;

			err = fData.Unflatten(raw);
			if (err)
				break;				// panic!

			err = msg->FindMessenger("owner", &fDataOwner);
			if (err)
				break;
			if (fMonitor) {
				BMessage	m(B_CLIPBOARD_CHANGED);
				m.AddString("be:clipboard", Name());
				m.AddInt32("be:count", fCount);
				fMonitor->Notify(&m);
			}

			// tell the client the new count
			reply.AddInt32("count", fCount);
			break;
			}
		case CMD_GET_CLIP_COUNT: {
			reply.AddInt32("count", fCount);
			break;
			}
		case CMD_CLIP_MONITOR: {
			bool		start;
			uint32		emask;
			BMessenger	target;
			BMessage	reply;
			status_t	err;
			err = msg->FindBool("start", &start);
			if (!err) {
				err = msg->FindMessenger("target", &target);
				if (!err) {
					err = msg->FindInt32("event_mask", (int32 *) &emask);
					if (!fMonitor) {
						fMonitor = new TMonitor(clip_monitor_map);
					}
					if (start)
						err = fMonitor->Add(target, emask);
					else
						err = fMonitor->Remove(target);
				}
			}
			reply.AddInt32("error", err);
			msg->SendReply(&reply);
			break;
			}
		default:
			err = B_ERROR;
			break;
	}

	reply.AddInt32("error", err);
	msg->SendReply(&reply);
}

/*------------------------------------------------------------------------*/

TClipHandler *TClipHandler::FindClipHandler(BLooper *, const char *clip_name)
{
	BAutolock lock(sClipLock);

	TClipHandler *handler;
	int32 i = 0;
	while ((handler = (TClipHandler *)sClipList.ItemAt(i++)) != 0) {
		if (strcmp(handler->Name(), clip_name) == 0)
			return handler;
	}
	return NULL;
}
