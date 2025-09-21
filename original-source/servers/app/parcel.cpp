//*****************************************************************************
//
//	File:		parcel.cpp
//	
//	Written by:	Peter Potrebic & Benoit Schillings
//  Destroyed by: George Hoffman
//	Swept up by: Dianne Hackborn
//
//	Copyright 1994-97, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <Debug.h>

#include <Autolock.h>
#include <ByteOrder.h>
#include <Locker.h>
#include <MessageBody.h>
#include <StreamIO.h>
#include <String.h>

#include "parcel.h"
#include "token.h"
#include "heap.h"
#include "rect.h"
#include "as_support.h"

inline point npoint(int32 h, int32 v) { point p; p.h = h; p.v = v; return p; };
inline fpoint nfpoint(float h, float v) { fpoint p; p.h = h; p.v = v; return p; };


/* ----------------------------------------------------------------- */

int32 BParcel::Target(bool* preferred) const
{
	const message_target& t = target_struct();
	if (preferred) {
		*preferred = (t.flags & MTF_PREFERRED_TARGET) ? true : false;
	}
	return t.target;
};

void BParcel::SetTarget(int32 target, bool preferred)
{
	message_target& t = target_struct();
	t.target = target;
	t.flags = (t.flags&~MTF_PREFERRED_TARGET) | (preferred ? MTF_PREFERRED_TARGET:0);
	set_flatten_with_target(target != NO_TOKEN ? true : false);
};

/* ----------------------------------------------------------------- */

static void* parcel_malloc(size_t size)
{
	return(grMalloc(size,"messagedata",MALLOC_CANNOT_FAIL));
}

static void* parcel_realloc(void* ptr, size_t size)
{
	return(grRealloc(ptr, size, "messagedata",MALLOC_CANNOT_FAIL));
}

static void parcel_free(void* ptr)
{
	grFree(ptr);
}

//----------------------------------------------------------------------------

void BParcel::init_data()
{
	if (message_malloc != parcel_malloc) {
		message_free = parcel_free;
		message_realloc = parcel_realloc;
		message_malloc = parcel_malloc;
	}
	
	next = NULL;
	m_eventMask = 0;
	m_transit = -1;
	m_location.h = 1e11;
	m_screenLocation.h = 0;
	m_screenLocation.v = 0;
	m_routing = 0;
	m_dragging = NULL;
	m_area = NULL;
}

//----------------------------------------------------------------------------
	
BParcel::BParcel()
{
	init_data();
}

//----------------------------------------------------------------------------

BParcel::BParcel(uint32 pwhat)
	: BMessage(pwhat)
{
	init_data();
}

//----------------------------------------------------------------------------

BParcel &BParcel::operator=(const BParcel &msg)
{
	if (this == &msg)
		return *this;
	
	BMessage::operator=(msg);

	m_eventMask = msg.m_eventMask;
	m_transit = msg.m_transit;
	m_location = msg.m_location;
	m_screenLocation = msg.m_screenLocation;
	m_routing = msg.m_routing;
	m_dragging = msg.m_dragging;
	m_area = NULL;

	return *this;
}

//----------------------------------------------------------------------------

BParcel::BParcel(const BParcel &msg)
	: BMessage(msg)
{
	m_eventMask = msg.m_eventMask;
	m_transit = msg.m_transit;
	m_location = msg.m_location;
	m_screenLocation = msg.m_screenLocation;
	m_routing = msg.m_routing;
	m_dragging = msg.m_dragging;
	m_area = NULL;
}

//----------------------------------------------------------------------------

BParcel::~BParcel()
{
	if (m_area) {
		m_area->ServerUnlock();
		m_area = NULL;
		set_body(NULL);
	}
}

//----------------------------------------------------------------------------

void	BParcel::PrintToStream() const
{
	DebugOnly(DOut << *(BMessage*)this << endl);
}

//----------------------------------------------------------------------------

void	BParcel::SetFromMessage(const BMessage& msg)
{
	*(BMessage*)this = msg;
}

//----------------------------------------------------------------------------

status_t	BParcel::Flatten(char **result, ssize_t *size) const 
{
	*size = FlattenedSize(); 
	*result = (char*)grMalloc(*size,"flatmessage",MALLOC_CANNOT_FAIL); 
	return Flatten(*result, *size); 
}

//----------------------------------------------------------------------------

BParcel* BParcel::ReplicateInArea() const
{
	BParcel* dup = new BParcel(*this);
	if (dup->body()) {
		dup->body()->Release();
		dup->set_body(NULL);
	}
	
	size_t size = fBody->FullSize();
	dup->m_area = new Area("draggedMessage", size);
	if (dup->m_area && !dup->m_area->BasePtr()) {
		delete dup->m_area;
		dup->m_area = NULL;
	}
	
	if (!dup->m_area) {
		delete dup;
		DebugOnly(DOut << "BParcel: error duplicating into area." << endl);
		return NULL;
	}
	
	dup->m_area->ServerLock();
	
	if (!body()) {
		// If the original message doesn't have a body, we still
		// must have an area for our new message -- so create the
		// area with an "empty" message body.
		BMessageBody* empty = BMessageBody::Create();
		empty->Acquire();
		memcpy(dup->m_area->BasePtr(), empty, size);
		empty->Release();
	} else {
		memcpy(dup->m_area->BasePtr(), body(), size);
	}
	
	dup->set_read_only(true);
	dup->set_body((BMessageBody*)(dup->m_area->BasePtr()));
	
	return dup;
}

const Area* BParcel::BodyArea() const
{
	return m_area;
}
