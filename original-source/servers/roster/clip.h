/*****************************************************************************

	File : main.cpp

	$Revision: 1.3 $

	Written by: Peter Potrebic

	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef CLIP_H
#define CLIP_H

#include <List.h>
#include <Locker.h>
#include <Handler.h>
#include <Path.h>
#include <Monitor.h>

class TClipHandler;

class TClipHandler : public BHandler {
public:
					TClipHandler(const char *name, bool transient);
virtual				~TClipHandler();

virtual void		MessageReceived(BMessage *msg);

static	TClipHandler	*FindClipHandler(BLooper *loop, const char *clip_name);

private:

static	BList		sClipList;
static	BLocker		sClipLock;

		BPath		fClipPath;
		BMessage	fData;		
		BMessenger	fDataOwner;
		uint32		fCount;
		bool		fTransient;
		TMonitor	*fMonitor;
};	

#endif
