
//******************************************************************************
//
//	File:			eventport.cpp
//	Description:	Special shared memory-based port
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#include "eventport.h"
#include "parcel.h"
#include "server.h"
#include "enums.h"
#include "window.h"

#include <MessageBody.h>

#define P(a)
//DebugPrintf(a)
#define MANY_EVENTS 48
#define TOO_MANY_EVENTS 64

enum {
	epEvents			= 0x0000FFFF,
	epWindowResized		= 0x00010000,
	epWindowMoved 		= 0x00020000,
	epViewsMoveSized	= 0x00040000,
	epBlockClient		= 0x00080000,
	epDoNotNotify		= 0x00100000
};

struct Event {
	uint32			next;
	int16			eventType;
	int16			transit;
	TWindow *		window;
	message_header	header;
	uint8			theRest[1];
};

sem_id SEventPort::ClientBlock()
{
	return m_clientBlock;
};

uint32 SEventPort::AtomicVar()
{
	return m_atomic_offset;
};

uint32 SEventPort::FirstEvent()
{
	uint32 offs;
	LockAddresses();
	offs = Ptr2Offset(&m_first->next);
	UnlockAddresses();
	return offs;
};

void SEventPort::LockAddresses()
{
	m_addressesLocked = true;
	m_rwHeap->Lock();
	m_roHeap->Lock();
	m_first = (Event*)m_roHeap->Ptr(m_first_offset);
	m_last = (Event*)m_roHeap->Ptr(m_last_offset);
	m_atomic = (int32*)m_rwHeap->Ptr(m_atomic_offset);
};

void SEventPort::UnlockAddresses()
{
	Event *e;
	for (int32 i=0;i<m_recyclingBin.CountItems();i++) {
		e = (Event*)m_roHeap->Ptr(m_recyclingBin[i]);
		if (e->window) e->window->ServerUnlock();
	};

	m_first_offset = m_roHeap->Offset(m_first);
	m_last_offset = m_roHeap->Offset(m_last);
	m_roHeap->Unlock();
	m_rwHeap->Unlock();
	m_addressesLocked = false;
	for (int32 i=0;i<m_recyclingBin.CountItems();i++)
		m_roHeap->Free(m_recyclingBin[i]);
	m_recyclingBin.SetItems(0);
};

void SEventPort::PrepareEvent(Event *eventPtr, BParcel *msg, int32 size, TWindow *windowTarget)
{
	msg->Flatten((char*)&eventPtr->header,size);
	eventPtr->transit = -1;
	eventPtr->window = windowTarget;
	if (windowTarget) windowTarget->ServerLock();
	switch (msg->what) {
		case B_MOUSE_MOVED:
		{	
			int32 transit = eventPtr->transit = msg->FindInt32("be:transit");
			eventPtr->eventType = 	(transit==B_OUTSIDE)?evMouseMoved_Outside:
									(transit==B_INSIDE)?evMouseMoved_Inside:
									evMouseMoved_EnterExit;
			break;
		}
		case B_MODIFIERS_CHANGED:
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_KEY_DOWN:
		case B_KEY_UP:		eventPtr->eventType = evInput; break;
		default:
		{
			P(("NonDroppable '%.4s' filed\n",&msg->what));
			eventPtr->eventType = evNonDroppable;
		}
	};
};

void SEventPort::SendEvent(BParcel *msg, uint32 target, bool preferred,
						   TWindow *windowTarget)
{
	if (!this) return;
	
	// This is low-overhead -- just adds a reference to the message data.
	BParcel msgCopy(*msg);
	if (target == NO_TOKEN) target = msg->Target(&preferred);
	if (target == NO_TOKEN) {
		// If there is no target token anywhere, just take the default
		// token so at least someone will receive it.
		target = m_token;
	}
	
	msgCopy.SetTarget(target, preferred);
	
	int32 size = msgCopy.FlattenedSize();
	uint32 event = NewEvent(size+12);

	m_portLock.Lock();
	LockAddresses();
		Event *eventPtr = (Event*)Offset2Ptr(event);
		eventPtr->next = NONZERO_NULL;
		PrepareEvent(eventPtr,&msgCopy,size,windowTarget);
		P(("Event out : '%.4s'(%d) ... %d\n",msgCopy.what,eventPtr->eventType,m_privateQueueCount));
		SendEvent(event);
	UnlockAddresses();
	m_portLock.Unlock();
};

void SEventPort::ReplaceEvent(
	BParcel *msg, uint32 *msgQueueIteration, uint32 *lastMsgToken, TWindow *windowTarget)
{
	if (!this) return;

	int32 size = msg->FlattenedSize();
	uint32 event = NewEvent(size+12);
	uint32 oldEvent;
	Event *eventPtr;
	
	P(("ReplaceEvent\n"));

	m_portLock.Lock();
	LockAddresses();

	eventPtr = (Event*)Offset2Ptr(event);
	eventPtr->next = NONZERO_NULL;
	PrepareEvent(eventPtr,msg,size,windowTarget);

	if ((*lastMsgToken != NONZERO_NULL) && (*msgQueueIteration == m_iteration)) {
		/*	The last movesize event this view sent might still be
			in the queue.  If so, we want to remove it before adding
			a new one. */
		uint32 oldAtomic = atomic_or(m_atomic,epBlockClient);
		if (oldAtomic) {
			/*	We're on the right iteration, and we've just successfully
				blocked the client from grabbing the queue while we play
				with it.  Now we'll remove the old event. */

			P(("Trying to replace message... (%08x)\n",oldAtomic));

			bool gotIt=false;
			uint32 *ptr,nextPtr;
			m_last = m_first;
			ptr = &m_first->next;
			while (*ptr != NONZERO_NULL) {
				Event *tmpEvent = (Event*)Offset2Ptr(*ptr);
				if (*ptr == *lastMsgToken) {
					nextPtr = tmpEvent->next;
					RecycleEvent(*ptr);
					*ptr = nextPtr;
					gotIt = true;
				} else {
					m_last = tmpEvent;
					ptr = &tmpEvent->next;
				};
			};
			if (!gotIt) {
				P(("Couldn't find previous message!\n"));
			};
			m_last->next = event;
			m_last = eventPtr;
			P(("Down to %d, %d\n",m_eventCount[evNonDroppable],m_privateQueueCount));
			if (!(atomic_and(m_atomic,~epEvents) & epBlockClient) ||
				!(atomic_or(m_atomic,m_privateQueueCount) & epBlockClient) ||
				!(atomic_and(m_atomic,~epBlockClient) & epBlockClient)) {
				P(("Release\n"));
				/*	The client has grabbed our events and is blocking.
					But it grabbed a bogus event count, so we have to
					update it.  It'll check after it's done blocking. */
				*m_atomic = m_privateQueueCount;
				release_sem(m_clientBlock);
			};
		} else { // fall through to the normal case
			*m_atomic = 0;
			P(("Fall through\n"));
			SendEvent(event);
		};
	} else {
		SendEvent(event);
	};

	*lastMsgToken = event;
	*msgQueueIteration = m_iteration;

	UnlockAddresses();
	m_portLock.Unlock();
};

uint32 SEventPort::SendEvent(uint32 newEvent)
{
	uint32 returnValue;
	Event *newEventMsg = (Event*)Offset2Ptr(newEvent);
	m_last->next = newEvent;
	returnValue = Ptr2Offset(m_last);
	int32 old = atomic_add(m_atomic,1);
	if (old & epEvents) {
		m_privateQueueCount++;
		m_eventCount[newEventMsg->eventType]++;
		P(("SEventPort::SendEvent: m_eventCount[newEventMsg->eventType]=%d\n",m_eventCount[newEventMsg->eventType]));
		if ((m_privateQueueCount - m_eventCount[evNonDroppable]) >= TOO_MANY_EVENTS) {
			/*	We're going to try to remove some events.  First we have to
				make sure the client won't be screwing with them while we are. */
			uint32 oldAtomic = atomic_or(m_atomic,epBlockClient);
			if (oldAtomic) {
				// Try to shorten the event list
				CleanupEvents();
				if (!(atomic_and(m_atomic,~epEvents) & epBlockClient) ||
					!(atomic_or(m_atomic,m_privateQueueCount) & epBlockClient) ||
					!(atomic_and(m_atomic,~epBlockClient) & epBlockClient)) {
					/*	The client has grabbed our events and is blocking.
						But it grabbed a bogus event count, so we have to
						update it.  It'll check after it's done blocking. */
					*m_atomic = m_privateQueueCount;
					release_sem(m_clientBlock);
				};
				/* CleanupEvents sets m_last to the right value */
			} else {
				/*	The client has grabbed the pending events and started
					processing them.  We'll be satisfied that the pipe is
					flowing and not care about trying to remove any events, so we
					don't have to do any further locking.  We don't have to worry
					about releasing the block, either, because we are guarenteed
					that there will only ever be one pending SEventPort processing
					request in the regular port.  Just clear the blocking bit. */
				*m_atomic = 0;
				m_last = newEventMsg;
			};
		} else {
			m_last = newEventMsg;
		};
	} else {
		/*	We have a bunch of events that we've marked as belonging
			to the client which we can now dispose of, since the client
			has grabbed the next set, proclaiming themselves done with the
			last bunch. */
		uint32 nextPtr,ptr = m_clientQueueFirst;
		while (m_clientQueueCount--) {
			nextPtr = ((Event*)Offset2Ptr(ptr))->next;
			RecycleEvent(ptr);
			ptr = nextPtr;
		};
		m_clientQueueFirst = m_first_offset;
		m_clientQueueCount = m_privateQueueCount;
		m_first = m_last;
		m_privateQueueCount = 1;
		m_iteration++;
		for (int32 i=0;i<evNumEventTypes;i++) m_eventCount[i] = 0;
		m_eventCount[newEventMsg->eventType]++;
		P(("SEventPort::SendEvent: client grabbed 'em\n"));

		// Tell the client we've got something for them
		m_pending = 1;
		m_last = newEventMsg;
	};

	NotifyPending();
	return returnValue;
};

void SEventPort::RemoveEvents(uint32 eventTypeMask, int32 numToRemove)
{
	Event *msg;
	uint32 *ptr,nextPtr;
	int32 count = 0;
	int32 index = m_privateQueueCount;

	m_last = m_first;
	ptr = &m_first->next;
	while (index--) {
		msg = (Event*)Offset2Ptr(*ptr);
		if (numToRemove && ((1<<msg->eventType) & eventTypeMask)) {
			nextPtr = msg->next;
			RecycleEvent(*ptr);
			*ptr = nextPtr;
			numToRemove--;
		} else {
			m_last = msg;
			ptr = &msg->next;
			count++;
		};
	};

	P(("Counted %d\n",count));
	m_privateQueueCount = count;
};

int32 SEventPort::RemoveEnterExitEvents(int32 available, int32 numToRemove)
{
	Event *msg;
	uint32 *ptr,nextPtr;
	int32 count = 0;
	int32 index = m_privateQueueCount;
	int32 numRemoved = 0;

	if (numToRemove > available) numToRemove = available;
	if (numToRemove % 2) {
		numToRemove++;
		if (numToRemove > available) numToRemove -= 2;
	};

	m_last = m_first;
	ptr = &m_first->next;
	
	reloop:
	while (index--) {
		msg = (Event*)Offset2Ptr(*ptr);
		if (numToRemove && (msg->transit == B_ENTERED)) {
			Event *msg2=msg;
			int32 target = *((uint32*)(&msg2->theRest[1]));
			int32 index2 = index;
			uint32 *ptr2 = &msg2->next;
			while (index2--) {
				msg2 = (Event*)Offset2Ptr(*ptr2);
				if ((msg2->transit == B_EXITED) && (*((uint32*)(&msg2->theRest[1])) == target)) {
					nextPtr = msg2->next;
					RecycleEvent(*ptr2);
					*ptr2 = nextPtr;
					nextPtr = msg->next;
					RecycleEvent(*ptr);
					*ptr = nextPtr;
					index--;
					numToRemove-=2;
					numRemoved += 2;
					goto reloop;
				};
				ptr2 = &msg2->next;
			};
			/*	Couldn't find the matching exit, so leave it alone */
		};
		m_last = msg;
		ptr = &msg->next;
		count++;
	};

	m_privateQueueCount = count;
	return numRemoved;
};

void SEventPort::CleanupEvents()
{
	/*	We want to go through the list of events we've compiled and throw some out. */

	int32 reduce = m_privateQueueCount - m_eventCount[evNonDroppable];
	uint32 eventTypeMask = 0;
	int32 et=0,count=0;

	P(("CleanupEvents %d %d %d\n",reduce,m_privateQueueCount,m_eventCount[evNonDroppable]));
	P(("counts: %d,%d,%d,%d,%d\n",
		m_eventCount[evMouseMoved_Outside],
		m_eventCount[evMouseMoved_Inside],
		m_eventCount[evMouseMoved_EnterExit],
		m_eventCount[evInput],
		m_eventCount[evNonDroppable]));
	
	while (	(et < evMouseMoved_EnterExit) &&
			((reduce - m_eventCount[et]) >= (MANY_EVENTS-1))) {
		if (m_eventCount[et]) {
			P(("At %d, throwing away all %d events of type %d... ",m_privateQueueCount,m_eventCount[et],et));
			eventTypeMask |= (1<<et);
			count += m_eventCount[et];
			reduce -= m_eventCount[et];
			m_eventCount[et++] = 0;
		} else {
			P(("No events of type %d to throw away\n",et));
			et++;
		};
	};
		
	if (eventTypeMask) {
		RemoveEvents(eventTypeMask,count);
		P(("(now at %d)\n",m_privateQueueCount));
	};
	
	if (reduce >= MANY_EVENTS) {
		if ((et == evMouseMoved_EnterExit) && m_eventCount[et]) {
			P(("At %d, trying to throw away %d mouse enter/exits out of %d... ",
				m_privateQueueCount,reduce-MANY_EVENTS+1,m_eventCount[et]));
			count = RemoveEnterExitEvents(m_eventCount[et],reduce-MANY_EVENTS+1);
			P(("(%d, now at %d)\n",count,m_privateQueueCount));
			reduce -= count;
			m_eventCount[evMouseMoved_EnterExit] -= count;
			if (reduce < MANY_EVENTS) return;
			et++;
		};
		count = m_eventCount[et];
		if ((reduce - count) < MANY_EVENTS) count = reduce-MANY_EVENTS+1;
		P(("Throwing away %d events of type %d\n",count,et));
		RemoveEvents((1<<et),count);
		m_eventCount[et] -= count;
		reduce -= count;
	};
	
	if (reduce >= MANY_EVENTS) {
		P(("Still have %d events, should have < %d\n",reduce,MANY_EVENTS));
	};
};

void SEventPort::NotifyPending()
{
	if (!m_pending) return;

	BParcel msg(_EVENTS_PENDING_);
	msg.SetTarget(m_token, false);
	status_t err = msg.WritePort(m_realPort, m_tag, B_TIMEOUT, 0);
	
	if (err == B_OK) m_pending = 0;

	if (err == B_OK) {
		P(("Notified client\n"));
	};
};

uint32 SEventPort::NewEvent(int32 size)
{
	return m_roHeap->Alloc(size);
};

void SEventPort::RecycleEvent(uint32 ptr)
{
	if (m_addressesLocked)
		m_recyclingBin.AddItem(ptr);
	else {
		debugger("Cannot call RecycleEvent outside of the lock!");
//		m_roHeap->Free(ptr);
	};
};

SEventPort::SEventPort(Heap *rwHeap, Heap *roHeap, port_id realPort, int32 token)
{
	uint32 msgOffs;

	m_rwHeap = rwHeap;
	m_roHeap = roHeap;
	m_rwHeap->ServerLock();
	m_roHeap->ServerLock();
	m_realPort = realPort;
	m_token = token;
	m_tag = 1;
	m_pending = 0;
	m_clientBlock = create_sem(0,"SEventPort::clientBlock");
	
	m_addressesLocked = false;
	m_clientQueueFirst = NONZERO_NULL;
	m_clientQueueCount =
	m_iteration =
	m_privateQueueCount = 0;
	for (int32 i=0;i<evNumEventTypes;i++) m_eventCount[i] = 0;
	m_atomic_offset = m_rwHeap->Alloc(4);
	msgOffs = NewEvent(sizeof(Event));
	LockAddresses();
		m_first = m_last = (Event*)Offset2Ptr(msgOffs);
		m_first->window = NULL;
		*m_atomic = 0;
	UnlockAddresses();
};

SEventPort::~SEventPort()
{
	m_portLock.Lock();
	
	m_rwHeap->Free(m_atomic_offset);
	LockAddresses();
	uint32 nextPtr,ptr = m_clientQueueFirst;
	while (m_clientQueueCount--) {
		nextPtr = ((Event*)Offset2Ptr(ptr))->next;
		RecycleEvent(ptr);
		ptr = nextPtr;
	};
	ptr = m_first_offset;
	while (m_privateQueueCount--) {
		nextPtr = ((Event*)Offset2Ptr(ptr))->next;
		RecycleEvent(ptr);
		ptr = nextPtr;
	};
	UnlockAddresses();
	m_rwHeap->ServerUnlock();
	m_roHeap->ServerUnlock();
	delete_sem(m_clientBlock);
};

void SEventPort::SetRealPort(port_id thePort)
{
	m_realPort = thePort;
}

void SEventPort::SetTag(int32 tag)
{
	m_tag = tag;
}

//------------------------------------------------------------------------------

CEventPort::CEventPort(
	int32 *atomic,
	void **nextBunch,
	sem_id clientBlock)
{
	InitObject(atomic,nextBunch,clientBlock);
};

CEventPort::CEventPort(SEventPort *port)
{
	InitObject((int32*)port->AtomicVar(),(void**)port->FirstEvent(),port->ClientBlock());
};

void CEventPort::InitObject(int32 *atomic, void **nextBunch, sem_id clientBlock)
{
	m_atomic = atomic;
	m_clientBlock = clientBlock;
	m_nextBunch = nextBunch;
	m_eventsQueued = 0;
	m_pending = false;
}

CEventPort::~CEventPort()
{
};

int32 CEventPort::Messages()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	return m_eventsQueued;
};

void CEventPort::ProcessPending()
{
	uint32 oldValue;
	status_t err;
	
	if (m_eventsQueued) {
		m_pending = true;
		return;
	};
	
	m_pending = false;
	oldValue = atomic_and(m_atomic,0);
	while (oldValue & epBlockClient) {
		do {
			err = acquire_sem(m_clientBlock);
		} while (err == B_INTERRUPTED);
		oldValue = atomic_and(m_atomic,0);
	};
	m_eventsQueued = oldValue & epEvents;
};

BParcel * CEventPort::GenerateMessage(TWindow **window)
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	if (!m_eventsQueued) return NULL;

	Event *msg = (Event *)(*m_nextBunch);
	m_nextBunch = (void**)(&msg->next);
	m_eventsQueued--;

	BParcel *bm = new BParcel();
	bm->Unflatten((char*)&msg->header);
	if (window) *window = msg->window;
	if (msg->window) msg->window->ServerLock();
	return bm;
};

//------------------------------------------------------------------------------

#ifdef REMOTE_DISPLAY

EventPipe::EventPipe(
	uint32	atomic,
	uint32	nextBunch,
	Heap *	rwHeap,
	Heap *	roHeap,
	sem_id clientBlock)
{
	m_atomicOffs = atomic;
	m_clientBlock = clientBlock;
	m_nextBunchOffs = nextBunch;
	m_eventsQueued = 0;
	m_rwHeap = rwHeap;
	m_roHeap = roHeap;
	m_pending = false;
};

EventPipe::~EventPipe()
{
};

void EventPipe::LockAddresses()
{
	m_rwHeap->BasePtrLock();
	m_roHeap->BasePtrLock();
	m_atomic = (int32*)m_rwHeap->Ptr(m_atomicOffs);
	m_nextBunch = (uint32*)m_roHeap->Ptr(m_nextBunchOffs);
};

void EventPipe::UnlockAddresses()
{
	m_nextBunchOffs = m_roHeap->Offset(m_nextBunch);
	m_roHeap->BasePtrUnlock();
	m_rwHeap->BasePtrUnlock();
};

int32 EventPipe::Messages()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	return m_eventsQueued;
};

void EventPipe::ProcessPending()
{
	uint32 oldValue;
	status_t err;
	
	if (m_eventsQueued) {
		m_pending = true;
		return;
	};

	LockAddresses();
	
	m_pending = false;
	oldValue = atomic_and(m_atomic,0);
	while (oldValue & epBlockClient) {
		do {
			err = acquire_sem(m_clientBlock);
		} while (err == B_INTERRUPTED);
		oldValue = atomic_and(m_atomic,0);
	};
	m_eventsQueued = oldValue & epEvents;

	UnlockAddresses();
};

char * EventPipe::GenerateMessage(int32 *size)
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	if (!m_eventsQueued) return NULL;

	Event *msg = (Event*)m_roHeap->Ptr(*m_nextBunch);
	m_nextBunch = &msg->next;
	m_eventsQueued--;

	if (size) *size = msg->header.size;
	return (char*)&msg->header;
};

#endif // REMOTE_DISPLAY
