/******************************************************************************
/
/	File:			parcel.h
/	Description:	A special derivatice of BMessage that is app_server
/					friendly.  This use to be our own whole copy of
/					BMessage, but that sucks.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
 ******************************************************************************/

#ifndef PARCEL_H
#define PARCEL_H

#include <AppDefs.h>
#include <Message.h>
#include <OS.h>

#include "basic_types.h"
#include "atom.h"

class Area;

class BParcel : public SAtom, public BMessage {

public:
		BParcel *	next;

					BParcel();
					BParcel(uint32 what);
					BParcel(const BParcel &a_message);
virtual				~BParcel();

		BParcel		&operator=(const BParcel &msg);

/* This version of PrintToStream() uses DebugPrintf() to write; if you
   call PrintToStream() on the BMessage superclass, your output will
   go to BOut (stdout). */
		void		PrintToStream() const;

/* "Up-convert" a BMessage to a BParcel */
		void		SetFromMessage(const BMessage& msg);
		
/* Version of Flatten() that dynamically allocates its buffer.  In most
   situations, this should be replaced with WritePort(). */
		status_t	Flatten(char **outBuffer, ssize_t *outSize) const;
		status_t	Flatten(char *buffer, ssize_t size) const
			{ return BMessage::Flatten(buffer, size); }
		status_t	Flatten(BDataIO *stream, ssize_t *size = NULL) const
			{ return BMessage::Flatten(stream, size); }

/* The app_server is special and gets to use these whenever it wants. */
		status_t	StartWriting(BPrivate::message_write_context* context,
								  bool include_target=true, bool fixed_size=true) const
					{ return start_writing(context, include_target, fixed_size); }
		void		FinishWriting(BPrivate::message_write_context* context) const
					{ finish_writing(context); }

/* These functions are used to return a copy of the parcel, with its
   body data in a shared area. */
		BParcel*	ReplicateInArea() const;
		const Area*	BodyArea() const;

/* Special app server extensions (for speed) */

inline	uint32		EventMask() const;
inline	fpoint		Location() const;
inline	point		ScreenLocation() const;
inline	int32		Transit() const;
		int32		Target(bool* preferred) const;
inline	uint32		Routing() const;
inline	BParcel *	Dragging() const;
inline	bool		HasLocation() const;

inline	void		SetEventMask(uint32 em);
inline	void		SetLocation(fpoint p);
inline	void		SetScreenLocation(point p);
inline	void		SetTransit(uint32 t);
		void		SetTarget(int32 target, bool preferred);
inline	void		SetRouting(uint32 r);
inline	void		SetDragging(BParcel *dragging);

/* Handling of the app_server's own peculiar types */
		status_t	AddRect(const char *name, frect a_rect)
			{ return BMessage::AddRect(name, *(BRect*)&a_rect); }
		status_t	AddPoint(const char *name, fpoint a_point)
			{ return BMessage::AddPoint(name, *(BPoint*)&a_point); }
		status_t	FindRect(const char *name, frect *rect) const
			{ return BMessage::FindRect(name, (BRect*)rect); }
		status_t	FindRect(const char *name, int32 index, frect *rect) const
			{ return BMessage::FindRect(name, index, (BRect*)rect); }
		status_t	FindPoint(const char *name, fpoint *pt) const
			{ return BMessage::FindPoint(name, (BPoint*)pt); }
		status_t	FindPoint(const char *name, int32 index, fpoint *pt) const
			{ return BMessage::FindPoint(name, index, (BPoint*)pt); }
		status_t	ReplaceRect(const char *name, frect a_rect)
			{ return BMessage::ReplaceRect(name, *(BRect*)&a_rect); }
		status_t	ReplaceRect(const char *name, int32 index, frect a_rect)
			{ return BMessage::ReplaceRect(name, index, *(BRect*)&a_rect); }
		status_t	ReplacePoint(const char *name, fpoint a_point)
			{ return BMessage::ReplacePoint(name, *(BPoint*)&a_point); }
		status_t	ReplacePoint(const char *name, int32 index, fpoint a_point)
			{ return BMessage::ReplacePoint(name, index, *(BPoint*)&a_point); }

private:
		void		init_data();
		
		uint32		m_eventMask;
		int32		m_transit;
		fpoint		m_location;
		point		m_screenLocation;
		uint32		m_routing;
		BParcel *	m_dragging;
		Area *		m_area;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

uint32 BParcel::EventMask() const
{
	return m_eventMask;
};

fpoint BParcel::Location() const
{
	return m_location;
};

point BParcel::ScreenLocation() const
{
	return m_screenLocation;
};

int32 BParcel::Transit() const
{
	return m_transit;
};

uint32 BParcel::Routing() const
{
	return m_routing;
};

BParcel * BParcel::Dragging() const
{
	return m_dragging;
};

bool BParcel::HasLocation() const
{
	return (m_location.h < 1e10);
};

void BParcel::SetLocation(fpoint p)
{
	m_location = p;
};

void BParcel::SetScreenLocation(point p)
{
	m_screenLocation = p;
};

void BParcel::SetRouting(uint32 r)
{
	m_routing = r;
};

void BParcel::SetEventMask(uint32 em)
{
	m_eventMask = em;
};

void BParcel::SetTransit(uint32 t)
{
	m_transit = t;
};

void BParcel::SetDragging(BParcel *drag)
{
	m_dragging = drag;
};

#endif
