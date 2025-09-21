
#ifndef BINDER2_THREAD_H
#define BINDER2_THREAD_H

#include "binder_defs.h"
#include "binder_transaction.h"

class binder_thread
{
	public:

				binder_thread *			next;
				int32					virtualThid;
				
										binder_thread(int32 thid, binder_team *team);
										/*	create a binder_thread that is initially
											prepped to send a reply to 'replyTo'. */
										binder_thread(int32 thid, binder_team *team, binder_thread *replyTo);
				 						~binder_thread();

				void					Init(int32 thid, binder_team *team);
				void					Released();

				bool					AttemptAcquire();
				void					Acquire(int32 type);
				void					Release(int32 type);

				void					Reply(binder_transaction *t);
				void					Reflect(binder_transaction *t);
				
				bool					AttemptExecution(binder_transaction *t);
				void					FinishAsync(binder_transaction *t);
				void					Sync();
				
				int32					Thid() const { return m_thid; }
				binder_team*			Team() const { return m_team; }

				int32					PrimaryRefCount() const { return m_primaryRefs; }
				int32					SecondaryRefCount() const { return m_secondaryRefs; }
				
				int32					Control(int32 cmd, void *buffer, int32 size);
				int32					Write(void *buffer, int32 size);
				int32					Read(void *buffer, int32 size);

	private:

				status_t				WaitForReply(iobuffer &io);
				status_t				WaitForRequest(iobuffer &io);
				status_t				ReturnTransaction(iobuffer &io, binder_transaction *t);

				void					WriteReturn(void *buffer, int32 size);

				void					EnqueueTransaction(binder_transaction *t);
				
				int32					m_primaryRefs;
				int32					m_secondaryRefs;
				status_t				m_err;
				int32					m_thid;
				sem_id					m_replySem;
				int32					m_waitForReply;
				binder_transaction *	m_reply;
				int32					m_consume;
				binder_team *			m_team;
				binder_transaction *	m_pendingReply;
				binder_transaction *	m_pendingRefResolution;
				
				// Did the driver spawn this thread?
				bool					m_isSpawned : 1;
				
				// Is this thread running as a looper?
				bool					m_isLooping : 1;
				
				// Has this thread already freed its m_pendingReply?
				bool					m_pendingFreed : 1;
				
				// Set if thread has determined an immediately reply for a
				// bcATTEMPT_ACQUIRE.  In this case, 'short' is true and
				// 'result' is whether it succeeded.
				bool					m_shortAttemptAcquire : 1;
				bool					m_resultAttemptAcquire : 1;
};

#endif // BINDER2_THREAD_H
