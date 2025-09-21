
//******************************************************************************
//
//	File:			eventport.h
//	Description:	Special shared memory-based port
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#ifndef EVENTPORT_H
#define EVENTPORT_H

#include <SupportDefs.h>
#include <OS.h>
#include "heap.h"
#include "lock.h"
#include "token.h"

struct Event;
class BParcel;
class TWindow;

enum EventType {
	evMouseMoved_Outside = 0,
	evMouseMoved_Inside,
	evMouseMoved_EnterExit,
	evInput,
	evNonDroppable,
	evNumEventTypes
};

class SEventPort {

	public:

							SEventPort(Heap *rwHeap, Heap *roHeap,
									   port_id realPort, int32 token=NO_TOKEN);
							~SEventPort();

		uint32				FirstEvent();
		uint32				AtomicVar();
		sem_id				ClientBlock();

		void				SendEvent(
								BParcel *msg,
								uint32 target=NO_TOKEN,
								bool preferred=false,
								TWindow *windowTarget = NULL);
		void				ReplaceEvent(
								BParcel *msg,
								uint32 *msgQueueIteration, uint32 *lastMsgToken,
								TWindow *windowTarget = NULL);

		void				SetRealPort(port_id thePort);
		void				SetTag(int32 tag);
		
	private:

		void				PrepareEvent(Event *eventPtr, BParcel *msg, int32 size, TWindow *windowTarget);
		uint32				NewEvent(int32 size);
		void				RecycleEvent(uint32 ptr);
		uint32				SendEvent(uint32 msg);
		void				CleanupEvents();
		void				NotifyPending();

		void				RemoveEvents(uint32 eventTypeMask, int32 numToRemove);
		int32				RemoveEnterExitEvents(int32 available, int32 numToRemove);

		void				LockAddresses();
		void				UnlockAddresses();
		void *				Offset2Ptr(uint32 offset) { return m_roHeap->Ptr(offset); };
		uint32				Ptr2Offset(void *ptr) { return m_roHeap->Offset(ptr); };

		Benaphore			m_portLock;
	
		Heap *				m_rwHeap;
		Heap *				m_roHeap;
		port_id				m_realPort;
		int32				m_token;
		int32				m_tag;
		sem_id				m_clientBlock;
		int32				m_pending;

		uint32				m_first_offset;
		uint32				m_last_offset;
		uint32				m_atomic_offset;

		Event *				m_first;
		Event *				m_last;
		int32 *				m_atomic;

		uint32				m_clientQueueFirst;
		int32				m_clientQueueCount;
		int32				m_privateQueueCount;

		int32				m_eventCount[evNumEventTypes];
		uint32				m_iteration;
		BArray<uint32>		m_recyclingBin;
		bool				m_addressesLocked;
};

class CEventPort {

	public:

							CEventPort(
								int32 *atomic,
								void **nextBunch,
								sem_id clientBlock);
							CEventPort(SEventPort *port);
							~CEventPort();

		void				ProcessPending();
		BParcel *			GenerateMessage(TWindow **target=NULL);
		int32				Messages();

	private:

		void				InitObject(int32 *atomic, void **nextBunch, sem_id clientBlock);
	
		int32 *				m_atomic;
		sem_id				m_clientBlock;
		void **				m_nextBunch;
		int32				m_eventsQueued;
		bool				m_pending;
};

#ifdef REMOTE_DISPLAY

class EventPipe {

	public:

							EventPipe(
								uint32 atomic,
								uint32 nextBunch,
								Heap *rwHeap,
								Heap *roHeap,
								sem_id clientBlock);
							~EventPipe();

		void				ProcessPending();
		char *				GenerateMessage(int32 *size=NULL);
		int32				Messages();
		void 				LockAddresses();
		void				UnlockAddresses();

		EventPipe *			next;
		int32				clientTag;

		Heap *				m_rwHeap;
		Heap *				m_roHeap;
		uint32				m_atomicOffs;
		int32 *				m_atomic;
		uint32				m_nextBunchOffs;
		uint32 *			m_nextBunch;
		int32				m_eventsQueued;
		sem_id				m_clientBlock;
		bool				m_pending;
};

#endif // REMOTE_DISPLAY

#endif
