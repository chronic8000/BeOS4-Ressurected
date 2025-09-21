
#include "binder_thread.h"
#include "binder_team.h"
#include "binder_driver.h"
#include "binder_node.h"
#include "binder_transaction.h"
#include "iobuffer.h"

#include <stdarg.h>

//#define DBTRANSACT(x) x
#define DBTRANSACT(x)
//#define DBSPAWN(x) x
#define DBSPAWN(x)
//#define DBREFS(x) x
#define DBREFS(x)
//#define DBREAD(x) x
#define DBREAD(x)

/* These are various way assertions can be compiled into the driver. */

#if 0
#define BND_ASSERT(cond, msg)									\
	if (!(cond)) {												\
		char buf[256];											\
		sprintf(buf, "Binder Assertion Failed: " #cond "\n");	\
		sprintf(buf+strlen(buf), msg);							\
		debugger(buf);											\
	}															\
/**/
#endif

binder_thread::binder_thread(int32 thid, binder_team *team)
{
	Init(thid,team);
}

binder_thread::binder_thread(int32 thid, binder_team *team, binder_thread *replyTo)
{
	Init(thid,team);
	m_pendingReply = new(0) binder_transaction(0,0,NULL,0,NULL);
	m_pendingReply->setRootObject(true);
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
	m_waitForReply = 0;
	m_reply = NULL;
	m_consume = 0;
	m_thid = thid;
	m_team = team;
	m_team->Acquire(SECONDARY);
	m_pendingReply = NULL;
	m_pendingRefResolution = NULL;
	m_isSpawned = false;
	m_isLooping = false;
	m_pendingFreed = false;
	m_shortAttemptAcquire = false;
	sprintf(buffer, "thread %ld: binder state lock", thid);
}

binder_thread::~binder_thread()
{
	ddprintf("*** DESTROYING THREAD %p (%ld)\n", this, m_thid);
	if (m_isLooping) m_team->FinishLooper(m_isSpawned);
	m_team->Release(SECONDARY);
	m_team = NULL;
}

void
binder_thread::Released()
{
	ddprintf("binder_thread::Released() of %p\n", this);
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

	delete_sem(m_replySem); m_replySem = B_BAD_SEM_ID;
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
	
	int32 cmd,target;
	void *ptr;
	iobuffer io(_buffer,_size);
	
	while (1) {
		if (m_consume) {
			m_consume -= io.drain(m_consume);
			io.markConsumed();
		}
		target = -1;
		if (!io.read<int32>(&cmd)) goto finished;
//		ddprintf("cmd: %ld\n",cmd);
		switch (cmd) {
			case bcINCREFS: {
				if (!io.read<int32>(&target)) goto finished;
				DBREFS(ddprintf("bcINCREFS of %ld\n", target));
				m_team->RefDescriptor(target, SECONDARY);
				io.markConsumed();
			} break;
			case bcACQUIRE: {
				if (!io.read<int32>(&target)) goto finished;
				DBREFS(ddprintf("bcACQUIRE of %ld\n", target));
				m_team->RefDescriptor(target, PRIMARY);
				io.markConsumed();
			} break;
			case bcATTEMPT_ACQUIRE: {
				int32 priority;
				if (!io.read<int32>(&priority)) goto finished;
				if (!io.read<int32>(&target)) goto finished;
				DBREFS(ddprintf("bcATTEMPT_ACQUIRE of %ld\n", target));
				binder_node *node;
				if (m_team->AttemptRefDescriptor(target, &node)) {
					DBREFS(ddprintf("Immediate Success!\n"));
					if (m_shortAttemptAcquire) debugger("Already have AttemptAcquire result!");
					m_shortAttemptAcquire = true;
					m_resultAttemptAcquire = true;
				} else if (node) {
					// Need to wait for a synchronous acquire attempt
					// on the remote node.  Note that the transaction has
					// special code to understand a tfAttemptAcquire, taking
					// ownership of the secondary reference on 'node'.
					DBREFS(ddprintf("Sending off to owner!\n"));
					binder_transaction *t = new
						binder_transaction(binder_transaction::tfAttemptAcquire, node->Ptr());
					t->next = m_pendingRefResolution;
					m_pendingRefResolution = t;
					t->priority = (int16)priority;
					t->target = node;
					t->setInline(true);
				} else {
					DBREFS(ddprintf("Immediate Failure!\n"));
					if (m_shortAttemptAcquire) debugger("Already have AttemptAcquire result!");
					m_shortAttemptAcquire = true;
					m_resultAttemptAcquire = false;
				}
				io.markConsumed();
			} break;
			case bcACQUIRE_RESULT: {
				int32 result;
				if (!io.read<int32>(&result)) goto finished;
				io.markConsumed();
				binder_transaction *t = new(sizeof(result), 0)
					binder_transaction(0, sizeof(result), &result, 0, NULL);
				t->next = m_pendingRefResolution;
				m_pendingRefResolution = t;
				t->setAcquireReply(true);
				t->setInline(true);
			} break;
			case bcRELEASE: {
				if (!io.read<int32>(&target)) goto finished;
				DBREFS(ddprintf("bcRELEASE of %ld\n", target));
				m_team->UnrefDescriptor(target, PRIMARY);
				io.markConsumed();
			} break;
			case bcDECREFS: {
				if (!io.read<int32>(&target)) goto finished;
				DBREFS(ddprintf("bcDECREFS of %ld\n", target));
				m_team->UnrefDescriptor(target, SECONDARY);
				io.markConsumed();
			} break;
			case bcFREE_BUFFER: {
				if (!io.read<void*>(&ptr)) goto finished;
				if (m_pendingReply && m_pendingReply->Data() == ptr) {
					// Data freed before reply sent.  Remember this to free
					// the transaction when we finally get its reply.
					m_pendingFreed = true;
				} else {
					m_team->FreeBuffer(ptr);
				}
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
			case bcREPLY: {
				binder_transaction_data tr;
				if (!io.read(&tr)) goto finished;
				if (tr.flags & tfInline) {
					ddprintf("inline transactions not supported yet\n");
					m_consume = tr.data_size - sizeof(tr.data);
					io.markConsumed();
				} else {
					io.markConsumed();
/*					
					if (tr.data_size && !is_valid_range(tr.data.ptr.buffer, tr.data_size, PROT_UWR)) {
						m_err = B_BAD_VALUE;
						goto finished;
					}
					if (tr.offsets_size && !is_valid_range(tr.data.ptr.offsets, tr.offsets_size, PROT_UWR)) {
						m_err = B_BAD_VALUE;
						goto finished;
					}
*/
					binder_transaction *t = new(tr.data_size, tr.offsets_size)
						binder_transaction(tr.code, tr.data_size, tr.data.ptr.buffer,
											tr.offsets_size, tr.data.ptr.offsets);
					t->next = m_pendingRefResolution;
					m_pendingRefResolution = t;

					t->flags = tr.flags;
					t->priority = (int16)tr.priority;
					t->setReply(cmd == bcREPLY);
					#if 1
					if (m_pendingReply && m_pendingReply->isRootObject()) {
						if (!t->isRootObject()) {
							ddprintf("*** Bad reply flags = 0x%08x\n", t->flags);
							debugger("EXPECTING ROOT REPLY!");
						}
					} else if (m_pendingReply && t->isRootObject()) {
						debugger("UNEXPECTED ROOT REPLY!");
					}
					#endif
					if (cmd == bcTRANSACTION) {
						target = tr.target.handle;
						t->target = m_team->Descriptor2Node(target, t);
						DPRINTF(5,("converted descriptor %ld to node %p\n",target,t->target));
						if (!t->target) debugger("NULL");
					}
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
				/*	This thread is going to loop, but it's not one of the
					driver's own loopers. */
				ASSERT(m_isLooping == false);
				m_isLooping = true;
				m_team->StartLooper();
				io.markConsumed();
			} break;
			case bcEXIT_LOOPER: {
				/*	End of a looper that is not the driver's own. */
				DBSPAWN(ddprintf("*** THREAD %ld RECEIVED bcEXIT_LOOPER\n", m_thid));
				ASSERT(m_isLooping == true);
				m_isLooping = false;
				m_team->FinishLooper(false);
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
			case binder_transaction::tfAttemptAcquire: {
				io.write<int32>(brATTEMPT_ACQUIRE);
				io.write<int32>(t->priority);
			} break;
			case binder_transaction::tfRelease: io.write<int32>(brRELEASE); break;
			case binder_transaction::tfDecRefs: io.write<int32>(brDECREFS); break;
		}
		io.write<void*>(*((void**)t->Data()));
		freeImmediately = t->refFlags() != binder_transaction::tfAttemptAcquire;
	} else if (t->isAcquireReply()) {
		io.write<int32>(brACQUIRE_RESULT);
		io.write<int32>(*((int32*)t->Data()));
		freeImmediately = true;
	} else {
		binder_transaction_data tr;
		tr.flags = t->flags;
		tr.priority = t->priority;
		tr.data_size = t->data_size;
		tr.offsets_size = t->offsets_size;
		tr.data.ptr.buffer = t->Data();
		tr.data.ptr.offsets = t->Offsets();
		if (t->isReply()) {
			tr.target.ptr = NULL;
			tr.code = 0;
			io.write<int32>(brREPLY);
		} else {
			tr.target.ptr = t->target->Ptr();
			tr.code = t->code;
			io.write<int32>(brTRANSACTION);
		}
		io.write(tr);
	}

	if (freeImmediately) {
		DPRINTF(5,("delete %p\n",t));
		delete t;
	} else {
		t->receiver = this;
		Acquire(SECONDARY);
		if (t->sender) {
			/*	A synchronous transaction blocks this thread until
				the receiver completes. */
			DPRINTF(5,("binder_thread %p: enqueueing reply %p, last %p\n", this, t, m_pendingReply));
			if (m_pendingFreed) debugger("m_pendingFreed was not cleared!");
			t->next = m_pendingReply;
			m_pendingReply = t;
			if (virtualThid) {
				if (t->sender->virtualThid) {
					if (t->sender->virtualThid != virtualThid)
						debugger("Bad virtualThid from sender!");
				} else if (t->sender->m_thid != virtualThid) {
					debugger("My virtualThid is different than sender thid!");
				}
			}
			if (t->sender->virtualThid)
				virtualThid = t->sender->virtualThid;
			else
				virtualThid = t->sender->m_thid;
		} else {
			/*	A reply transaction just waits until the receiver is done with
				its data. */
			DPRINTF(5,("binder_thread: return reply transaction %p\n", t));
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

	while ((err=acquire_sem(m_replySem)) == B_INTERRUPTED) ;

	DBTRANSACT(ddprintf("*** Thread %ld received direct %p!  wait=%ld, isAnyReply=%d\n",
			find_thread(NULL), m_reply, m_waitForReply, m_reply->isAnyReply()));

	if (m_isLooping) m_team->PutMeBackInTheGameCoach();

	if ((t = m_reply)) {
		// If this is a reply, handle it.  Otherwise it is a reflected
		// transaction, and we must adjust our current thread priority
		// to match it.  The caller is responsible for restoring its
		// thread priority once done with the reflect.
		if (t->isAnyReply()) m_waitForReply--;
		else set_thread_priority(m_thid, t->priority);
		m_reply = NULL;
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
			
	status_t err = m_team->WaitForRequest(this, &t);
	if (err) return err;
	ASSERT(t);

	return ReturnTransaction(io,t);
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
		if (m_shortAttemptAcquire) {
			DBREAD(ddprintf("Thread %ld already has reply!\n", find_thread(NULL)));
			m_shortAttemptAcquire = false;
			io.write<int32>(brACQUIRE_RESULT);
			io.write<int32>(m_resultAttemptAcquire);
			io.markConsumed();
		} else if ((t = m_pendingRefResolution)) {
			DBREAD(ddprintf("Thread %ld performing ref resolution!\n", find_thread(NULL)));
			bool releaseTeam = false;
			err = t->ConvertToNodes(m_team,&io);
			io.markConsumed();
			if (err < 0) goto finished;
			if (io.remaining() < 4) goto finished;
			m_pendingRefResolution = t->next;
			releaseTeam = (t->isRootObject());
			const bool isInline = t->isInline();
			if (t->isAnyReply()) {
				replyTo = m_pendingReply;
				if (replyTo) {
					m_pendingReply = replyTo->next;
					if (!m_pendingReply) virtualThid = 0;
					
					/* If this is a successful bcATTEMPT_ACQUIRE, then take
						care of reference counts now.
					*/
					if (t->isAcquireReply() && (*(int32*)t->Data()) != 0) {
						replyTo->sender->Team()->ForceRefNode(replyTo->target, &io);
					}
					
					if (replyTo->isRootObject()) {
						if (!t->isRootObject()) debugger("EXPECTING ROOT REPLY!");
					} else if (replyTo->refFlags()&binder_transaction::tfAttemptAcquire) {
						if (!t->isAcquireReply()) debugger("EXPECTING ACQUIRE REPLY!");
					} else {
						if (t->isRootObject() || t->isAcquireReply())
							debugger("EXPECTING REGULAR REPLY!");
					}
					//BND_ASSERT(t == NULL, "This is a test assertion in %p", this);
					
					DBTRANSACT(ddprintf("*** Thread %ld is replying to %p with %p!  wait=%ld\n",
							find_thread(NULL), replyTo, t, m_waitForReply));
					replyTo->sender->Reply(t);
					if (replyTo->isInline() || replyTo->isRootObject()) {
						delete replyTo;
					} else {
						DPRINTF(5,("binder_thread: finish reply request %p\n", replyTo));
						if (m_pendingFreed) {
							m_pendingFreed = false;
							delete replyTo;
						} else {
							replyTo->DetachTarget();
							m_team->AddToNeedFreeList(replyTo);
						}
					}
				} else {
					ddprintf("********** Nowhere for reply to go!!!!!!!!!!!\n");
					if (!t->isRootObject()) debugger("Unexpected reply!");
					releaseTeam = false;
				}
			} else {
				t->sender = this;
				Acquire(SECONDARY);
				m_waitForReply++;
				DBTRANSACT(ddprintf("*** Thread %ld going to wait for reply to %p!  now wait=%ld\n",
						find_thread(NULL), t, m_waitForReply));
				//if (m_waitForReply == 2) debugger("ARGH!");
				//printf("t->target = %p\n",t->target);
				t->target->Send(t);
			}
			if (!isInline) io.write<int32>(brTRANSACTION_COMPLETE);
			io.markConsumed();
			if (releaseTeam) {
				ddprintf("releaseTeam\n");
				m_team->Release(PRIMARY);
			}
		} else if (m_waitForReply) {
			DBREAD(ddprintf("Thread %ld waiting for reply!\n", find_thread(NULL)));
			err = WaitForReply(io);
			if (err == ENOBUFS) goto finished;
			if (err) return err;
		} else {
			DBREAD(ddprintf("Thread %ld waiting for request!\n", find_thread(NULL)));
			if (virtualThid != 0) debugger("Waiting for transaction with vthid != 0");
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
	DBTRANSACT(ddprintf("*** Thread %ld (vthid %ld) sending to %ld (vthid %ld)!  wait=%ld, isReply=%d, isAcquireReply=%d\n",
				find_thread(NULL), t->sender ? t->sender->virtualThid : -1,
				m_thid, virtualThid, m_waitForReply, t->isReply(), t->isAcquireReply()));
	if (m_reply != NULL) debugger("Already have reply!");
	if (m_waitForReply <= 0) debugger("Not waiting for a reply!");
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
