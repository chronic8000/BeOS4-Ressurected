
#include "binder_thread.h"
#include "binder_team.h"
#include "binder_driver.h"
#include "binder_node.h"
#include "binder_transaction.h"
#include "iobuffer.h"

//#define DBTRANSACT(x) x
#define DBTRANSACT(x)
//#define DBSPAWN(x) x
#define DBSPAWN(x)

binder_thread::binder_thread(int32 thid, binder_team *team)
{
	Init(thid,team);
}

binder_thread::binder_thread(int32 thid, binder_team *team, binder_thread *replyTo)
{
	Init(thid,team);
	m_pendingReply = new(0) binder_transaction(0,0,NULL);
	m_pendingReply->setSync(true);
	m_pendingReply->sender = replyTo;
	replyTo->Acquire(SECONDARY);
}

void binder_thread::Init(int32 thid, binder_team *team)
{
	char buffer[64];
	next = NULL;
	virtualThid = 0;
	m_primaryRefs = 0;
	m_secondaryRefs = 0;
	m_err = 0;
	sprintf(buffer, "thread %ld: binder reply sem", thid);
	m_replySem = create_sem(0,buffer);
	sprintf(buffer, "thread %ld: binder sync sem", thid);
	m_syncSem = create_sem(0,buffer);
	m_waitForReply = 0;
	m_reply = NULL;
	m_consume = 0;
	m_thid = thid;
	m_team = team;
	m_team->Acquire(SECONDARY);
	m_pendingReply = NULL;
	m_pendingRefResolution = NULL;
	m_pendingComplete = NULL;
	m_isSpawned = false;
	m_isLooping = false;
	sprintf(buffer, "thread %ld: binder state lock", thid);
	new_lock(&m_lock, buffer);
	m_currentAsync = NULL;
	m_finalAsync = NULL;
	m_syncPoint = NULL;
}

binder_thread::~binder_thread()
{
	printf("*** DESTROYING THREAD %p (%ld)\n", this, m_thid);
	if (m_isLooping) m_team->FinishLooper(m_isSpawned);
	m_team->Release(SECONDARY);
	m_team = NULL;
}

void
binder_thread::Released()
{
	printf("binder_thread::Released() of %p\n", this);
	binder_transaction *cmd;

	m_team->RemoveThread(this);
	
	while ((cmd = m_pendingRefResolution)) {
		m_pendingRefResolution = cmd->next;
		delete cmd;
	}

	while ((cmd = m_pendingReply)) {
		m_pendingReply = cmd->next;
		delete cmd;
	}

	while ((cmd = m_pendingComplete)) {
		m_pendingComplete = cmd->next;
		delete cmd;
	}

	while ((cmd = m_sendQueue.dequeue())) {
		delete cmd;
	}
	
	delete_sem(m_syncSem); m_syncSem = B_BAD_SEM_ID;
	delete_sem(m_replySem); m_replySem = B_BAD_SEM_ID;
	free_lock(&m_lock);
}

void
binder_thread::FinishAsync(binder_transaction *t)
{
	LOCK(m_lock);
	DBTRANSACT(printf("Thread %ld finishing %p in %ld\n", m_thid, t, find_thread(NULL)));
	ASSERT(m_currentAsync == t);
	if (t == m_finalAsync) m_finalAsync = NULL;
	if (t == m_syncPoint) {
		DBTRANSACT(printf("*** Thread %ld hit sync point at %p in %ld!\n",
				m_thid, t, find_thread(NULL)));
		m_syncPoint = NULL;
		release_sem_etc(m_syncSem, 1, B_DO_NOT_RESCHEDULE);
	}
	if (t == m_currentAsync) {
		t = m_currentAsync = m_sendQueue.dequeue();
		DBTRANSACT(printf("Thread %ld retrieving queued %p, next is %p\n",
				m_thid, t, t ? t->next : NULL));
		if (t && t->isSync()) {
			/*	A queued synchronous transaction will not have FinishAsync()
				called for it, so remove all traces of it now.  The thread is
				already waiting for it, anyway, so it doesn't matter. */
			m_currentAsync = NULL;
			DBTRANSACT(printf("Thread %ld forgetting about sync transaction %p\n",
					m_thid, t));
		}
	} else {
		t = NULL;
	}
	UNLOCK(m_lock);
	
	/*	Start processing next queued up transaction. */
	if (t) {
		DBTRANSACT(printf("Thread %ld sending queued %p in %ld\n",
				m_thid, t, find_thread(NULL)));
		ASSERT(t->sender == this);
		t->next = NULL;
		t->target->Send(t, true);
	}
}

void
binder_thread::Sync()
{
	DBTRANSACT(printf("Thread %ld performing Sync(): cur=%p, final=%p, head=%p\n",
		find_thread(NULL), m_currentAsync, m_finalAsync, m_sendQueue.head));
	
	if (m_finalAsync == NULL) return;
	
	LOCK(m_lock);
	if (m_finalAsync == NULL) {
		UNLOCK(m_lock);
		return;
	}
	
	ASSERT(m_syncPoint == NULL);
	m_syncPoint = m_finalAsync;
	DBTRANSACT(printf("*** Thread %ld syncing to %p!\n", find_thread(NULL), m_syncPoint));
	UNLOCK(m_lock);
	
	if (m_isLooping) m_team->TakeMeOffYourList();
	
	while (acquire_sem(m_syncSem) == B_INTERRUPTED) ;
	ASSERT(m_syncPoint == NULL);
	
	DBTRANSACT(printf("*** Thread %ld now synced!\n", find_thread(NULL)));
	
	if (m_isLooping) m_team->PutMeBackInTheGameCoach();
}

int32 
binder_thread::Control(int32 cmd, void *buffer, int32 size)
{
	switch (cmd) {
/*		case BINDER_SET_ROOT: {
			if (size < 4) return B_BAD_VALUE;
			binder_node *n = NULL;
			if (!m_team->Ptr2Node(*((void**)buffer),&n,NULL)) set_root(n);
			return B_OK;
		} break;
		case BINDER_WAIT_FOR_ROOT: {
			if (size < 4) return B_BAD_VALUE;
			binder_node *n = get_root();
			n->Release(PRIMARY);
			return B_OK;
		} break;
*/
		case BINDER_SET_WAKEUP_TIME:
			if (size < 8) return B_BAD_VALUE;
			return m_team->SetWakeupTime(*((int64*)buffer));
		case BINDER_SET_IDLE_TIMEOUT:
			if (size < 8) return B_BAD_VALUE;
			return m_team->SetIdleTimeout(*((int64*)buffer));
		case BINDER_SET_REPLY_TIMEOUT:
			if (size < 8) return B_BAD_VALUE;
			return m_team->SetReplyTimeout(*((int64*)buffer));
		default : break;
	}
	
	return B_BAD_VALUE;
}

int32 
binder_thread::Write(void *_buffer, int32 _size)
{
	int32 me = find_thread(NULL);
	if (me != m_thid) return B_ERROR;
	if (m_err) return m_err;
	
	uint8 numValues,flags;
	int32 cmd,target,size;
	void *ptr;
	binder_node *targetNode;
	iobuffer io(_buffer,_size);
	
	while (1) {
		if (m_consume) {
			m_consume -= io.drain(m_consume);
			io.markConsumed();
		}
		target = -1;
		if (!io.read<int32>(&cmd)) goto finished;
//		printf("cmd: %ld\n",cmd);
		switch (cmd) {
			case bcINCREFS: {
				if (!io.read<int32>(&target)) goto finished;
				io.markConsumed();
			} break;
			case bcACQUIRE: {
				if (!io.read<int32>(&target)) goto finished;
				m_team->RefDescriptor(target);
				io.markConsumed();
			} break;
			case bcRELEASE: {
				if (!io.read<int32>(&target)) goto finished;
				m_team->UnrefDescriptor(target);
				io.markConsumed();
			} break;
			case bcDECREFS: {
				if (!io.read<int32>(&target)) goto finished;
				io.markConsumed();
			} break;
			case bcFREE_BUFFER: {
				if (!io.read<void*>(&ptr)) goto finished;
				m_team->FreeBuffer(ptr);
				io.markConsumed();
			} break;
			case bcTRANSACTION_COMPLETE: {
				ASSERT(m_pendingComplete != NULL);
				if (m_pendingComplete && m_pendingComplete->sender)
					m_pendingComplete->sender->FinishAsync(m_pendingComplete);
				binder_transaction* t = m_pendingComplete;
				m_pendingComplete = t->next;
				DBTRANSACT(printf("*** COMPLETED TRANSACTION %p, NEXT IS %p\n",
						t, m_pendingComplete));
				m_team->AddToNeedFreeList(t);
				io.markConsumed();
			} break;
			case bcRESUME_THREAD: {
				int32 thid,tid;
				if (!io.read<int32>(&thid)) goto finished;
				#if BINDER_DEBUG_LIB
				if (!io.read<int32>(&tid)) goto finished;
				#else
				thread_info tinfo;
				get_thread_info(thid,&tinfo);
				tid = tinfo.team;
				#endif
				binder_team *team = get_team(tid,thid,this);
				team->Release(SECONDARY);
				m_waitForReply++;
				resume_thread(thid);
				io.markConsumed();
			} break;
			case bcSET_THREAD_ENTRY: {
				void *func;
				if (!io.read<void*>(&func)) goto finished;
				if (!io.read<void*>(&ptr)) goto finished;
				m_team->SetThreadEntry((thread_entry)func,ptr);
				io.markConsumed();
			} break;
			case bcTRANSACTION:
				if (!io.read<int32>(&target)) goto finished;
			case bcREPLY: {
				if (!io.read<uint8>(&numValues)) goto finished;
				if (!io.read<uint8>(&flags)) goto finished;
				if (!io.read<int32>(&size)) goto finished;
				if (flags & tfInline) {
					ddprintf(("inline transactions not supported yet\n"));
					m_consume = size;
					io.markConsumed();
				} else {
					if (!io.read<void*>(&ptr)) goto finished;
					io.markConsumed();
/*					
					if (!is_valid_range(ptr, size, PROT_UWR)) {
						m_err = B_BAD_VALUE;
						goto finished;
					}
*/
					binder_transaction *t = new(size) binder_transaction(numValues,size,ptr);
					t->next = m_pendingRefResolution;
					m_pendingRefResolution = t;

					t->flags = flags;
					t->setReply(cmd == bcREPLY);
					if (cmd == bcTRANSACTION) t->target = m_team->Descriptor2Node(target);
				}
			} break;
			case bcREGISTER_LOOPER: {
				ASSERT(m_isSpawned == false);
				ASSERT(m_isLooping == false);
				m_isSpawned = true;
				m_isLooping = true;
				m_team->StartLooper();
				io.markConsumed();
			} break;
			case bcENTER_LOOPER: {
				/*	This thread is going to loop, but it's not own of the
					driver's own loopers. */
				ASSERT(m_isLooping == false);
				m_isLooping = true;
				m_team->StartLooper();
				io.markConsumed();
			} break;
			case bcEXIT_LOOPER: {
				/*	End of a looper that is not the driver's own. */
				DBSPAWN(printf("*** THREAD %ld RECEIVED bcEXIT_LOOPER\n", m_thid));
				ASSERT(m_isLooping == true);
				m_isLooping = false;
				m_team->FinishLooper(false);
				io.markConsumed();
			} break;
			case bcSYNC: {
				DBTRANSACT(printf("*** Thread %ld requested a Sync()\n", m_thid));
				Sync();
				io.markConsumed();
			} break;
			default: {
				UHOH("Bad command value on Write()");
			} break;
		}
	}

finished:
	return io.consumed();
}

status_t
binder_thread::ReturnTransaction(iobuffer &io, binder_transaction *t)
{
	if (io.remaining() < 18) return ENOBUFS;

	t->ConvertFromNodes(m_team);

	bool freeImmediately = false;
	
	if (t->refFlags()) {
		switch (t->refFlags()) {
			case binder_transaction::tfRelease: io.write<int32>(brRELEASE); break;
			case binder_transaction::tfDecRefs: io.write<int32>(brDECREFS); break;
		}
		io.write<void*>(*((void**)t->data));
		freeImmediately = true;
	} else {
		if (t->isReply()) io.write<int32>(brREPLY);
		else {
			io.write<int32>(brTRANSACTION);
			io.write<void *>(t->target->Ptr());
		}
		io.write<int8>(t->numValues);
		io.write<int8>(t->flags);
		io.write<int32>(t->size);
		io.write<void*>(t->data);
	}

	if (freeImmediately)
		delete t;
	else {
		t->receiver = this;
		Acquire(SECONDARY);
		if (t->isSync() && t->sender) {
			/*	A synchronous transaction blocks this thread until
				the receiver completes. */
			t->next = m_pendingReply;
			m_pendingReply = t;
			if (t->sender->virtualThid)
				virtualThid = t->sender->virtualThid;
			else
				virtualThid = t->sender->m_thid;
		} else if (!t->isReply()) {
			/*	An asynchronous non-reply transaction blocks other transactions
				from its sender until this one completes. */
			DBTRANSACT(printf("*** TRANSACTION %p NOW PENDING COMPLETION\n", t));
			t->next = m_pendingComplete;
			m_pendingComplete = t;
		} else {
			/*	A reply transaction just waits until the receiver is done with
				its data. */
			m_team->AddToNeedFreeList(t);
		}
	}

	io.markConsumed();
	
	return B_OK;
}

status_t
binder_thread::WaitForReply(iobuffer &io)
{
	status_t err;
	binder_transaction *t = NULL;
	if (io.remaining() < 18) return ENOBUFS;

	if (m_isLooping) m_team->TakeMeOffYourList();

	DBTRANSACT(printf("*** Thread %ld waiting for reply!\n", find_thread(NULL)));
	
	while ((err=acquire_sem(m_replySem)) == B_INTERRUPTED) ;

	DBTRANSACT(printf("*** Thread %ld received reply %p!\n", find_thread(NULL), m_reply));
	
	if (m_isLooping) m_team->PutMeBackInTheGameCoach();

	if ((t = m_reply)) {
		if (t->isReply()) m_waitForReply--;
		return ReturnTransaction(io,t);
	}
	UHOH("replySem released without reply available");
	return B_ERROR;
}

status_t
binder_thread::WaitForRequest(iobuffer &io)
{
	binder_transaction *t = NULL;
	if (io.remaining() < 18) return ENOBUFS;

	/*
		Before waiting for more transactions, we must make sure that all
		of this thread's asynchronous transactions have been processed.
		This is to avoid the following deadlock situation:
		(1)	Thread A processes a request, during which it sends some
			asynchronous transactions, then waits for another request.
		(2)	Thread B receives one of the above transactions and in the
			course of processing it needs to send a synchronous
			transaction to the team of thread A.
		(3)	Thread A receives the synchronous transaction from B and
			starts to process it.  In the course of doing so, it must
			send a synchronous transaction to some other team.
		(4)	We now have a deadlock.  No more transactions from A will
			be processed until the existing asynchronous transaction
			completes.  But that won't happen until B's synchronous
			transaction completes, which won't happen until A's new
			synchronous transaction completes.
		Yes, it was as hard to figure out this deadlock as it sounds.
		
		One way to avoid the Sync() here would be to remove transaction
		ordering from the binder_thread, in to a "binder_context".
		The thread could then just make a new context instead of doing
		a Sync() on the current context.
	*/
	Sync();
			
	status_t err = m_team->WaitForRequest(this, &t);
	if (err) return err;
	ASSERT(t);

	return ReturnTransaction(io,t);
}

void
binder_thread::EnqueueTransaction(binder_transaction *t)
{
	if (t->target) {
		/*	Quick case -- this is a synchronous transaction, and there are
			no pending asynchronous transactions. */
		if (m_currentAsync == NULL) {
			DBTRANSACT(printf("Thread %ld sending immediate %p, sync=%d\n",
					find_thread(NULL), t, t->isSync()));
			if (!t->isSync()) {
				ASSERT(t->sender == this);
				m_currentAsync = m_finalAsync = t;
			}
			t->target->Send(t, false);
			return;
		}
		
		if (t->target->PreSend(t)) return;
		
		/*	We need to enqueue the transaction / synchronize with other
			transactions. */
		LOCK(m_lock);
		if (m_currentAsync) {
			DBTRANSACT(printf("Thread %ld enqueueing %p, sync=%d\n",
					find_thread(NULL), t, t->isSync()));
			ASSERT(t->sender == this);
			m_sendQueue.enqueue(t);
			if (!t->isSync()) m_finalAsync = t;
			t = NULL;
		} else if (!t->isSync()) {
			ASSERT(t->sender == this);
			m_currentAsync = m_finalAsync = t;
		}
		UNLOCK(m_lock)
		
		/*	No more transactions were pending by the time we got around to locking
			the queue, so send this one now. */
		if (t) {
			DBTRANSACT(printf("Thread %ld sending missed immediate %p, sync=%d\n",
					find_thread(NULL), t, t->isSync()));
			t->target->Send(t, true);
		}
	}
}

int32 
binder_thread::Read(void *buffer, int32 size)
{
	status_t err = B_OK;
	int32 me = find_thread(NULL);
	if (me != m_thid) return B_ERROR;

	binder_transaction *t,*replyTo;
	iobuffer io(buffer,size);

	while (!io.consumed()) {
		if ((t = m_pendingRefResolution)) {
			bool releaseTeam = false;
			err = t->ConvertToNodes(m_team,&io);
			io.markConsumed();
			if (err < 0) goto finished;
			if (io.remaining() < 4) goto finished;
			m_pendingRefResolution = t->next;
			releaseTeam = (t->isRootObject());
			if (t->isReply()) {
				replyTo = m_pendingReply;
				if (replyTo) {
					m_pendingReply = replyTo->next;
					if (!m_pendingReply) virtualThid = 0;
					replyTo->sender->Reply(t);
					delete replyTo;
				}
			} else {
				t->sender = this;
				Acquire(SECONDARY);
				if (t->isSync()) m_waitForReply++;
				EnqueueTransaction(t);
			}
			io.write<int32>(brTRANSACTION_COMPLETE);
			io.markConsumed();
			if (releaseTeam) {
				printf("releaseTeam\n");
				m_team->Release(PRIMARY);
			}
		} else if (m_waitForReply) {
			err = WaitForReply(io);
			if (err == ENOBUFS) goto finished;
			if (err) return err;
		} else {
			err = WaitForRequest(io);
			if (err == ENOBUFS) goto finished;
			else if (err == REQUEST_EVENT_READY) {
				io.write<int32>(brEVENT_OCCURRED);
				io.markConsumed();
			} else if (err == B_TIMED_OUT) {
				if (m_isLooping) {
					m_team->FinishLooper(m_isSpawned);
					m_isLooping = false;
				}
				if (m_isSpawned) io.write<int32>(brFINISHED);
				else io.write<int32>(brOK);
				io.markConsumed();
			} else if (err == B_BAD_SEM_ID) {
				io.write<int32>(brFINISHED);
				io.markConsumed();
			} else if (err < B_OK) {
				io.write<int32>(brERROR);
				io.write<int32>(err);
				io.markConsumed();
			}
		}
	}

finished:
	return io.consumed();
}

void 
binder_thread::Reply(binder_transaction *t)
{
	m_reply = t;
	release_sem(m_replySem);
}

void 
binder_thread::Reflect(binder_transaction *t)
{
	Reply(t);
}

bool 
binder_thread::AttemptAcquire()
{
	int32 current = m_primaryRefs;
	while (current > 0 && !cmpxchg32(&m_primaryRefs, &current, current+1));
	if (current <= 0) return false;
	atomic_add(&m_secondaryRefs,1);
	return true;
}

void 
binder_thread::Acquire(int32 type)
{
	switch (type) {
		case PRIMARY:
			atomic_add(&m_primaryRefs,1);
		case SECONDARY:
			atomic_add(&m_secondaryRefs,1);
		default: break;
	}
}

void 
binder_thread::Release(int32 type)
{
	switch (type) {
		case PRIMARY:
			if (atomic_add(&m_primaryRefs,-1) == 1) Released();
		case SECONDARY:
			if (atomic_add(&m_secondaryRefs,-1) == 1) {
				delete this;
			}
		default: break;
	}
}
