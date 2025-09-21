
#ifndef BINDER2_TEAM_H
#define BINDER2_TEAM_H

#include "binder_defs.h"

// This "error" is returned by WaitForRequest() when a timed event
// is scheduled to happen.
enum {
	REQUEST_EVENT_READY = 1
};

class binder_team
{
	public:
				int32					teamID;
				binder_team *			next;

										binder_team(int32 id);
										binder_team(int32 id, int32 mainThid, binder_thread *parent);
					 					~binder_team();

				void 					Init(int32 id);
				void 					Released();

				bool					AttemptAcquire();
				void					Acquire(int32 type);
				void					Release(int32 type);

				binder_thread *			OpenThread(int32 thid);
				void					RemoveThread(binder_thread *t);
	
				status_t				WaitForRequest(	binder_thread* who,
														binder_transaction **t);
				
				/*	Call when a thread receives its bcREGISTER_LOOPER command. */
				void					StartLooper();
				/*	Call when exiting a thread who has been told bcREGISTER_LOOPER. */
				void					FinishLooper(bool driverSpawned);
				
				int32					SetWakeupTime(bigtime_t time);
				int32					SetIdleTimeout(bigtime_t timeDelta);
				int32					SetReplyTimeout(bigtime_t timeDelta);

				/*	Call to place a transaction in to this team's queue. */
				status_t				Transact(binder_transaction *t);

				status_t				AddToNeedFreeList(binder_transaction *t);
				status_t				FreeBuffer(void *p);

				bool					RefDescriptor(int32 descriptor, int32 type);
				bool					UnrefDescriptor(int32 descriptor, int32 type);
				bool					RemoveLocalMapping(void *ptr);

				/*	Attempt to acquire a primary reference on the given descriptor.
					The result will be true if this succeeded, in which case you
					can just continue with it.  If the result is false, then
					'out_target' may be set to the binder_node the you are making
					the attempt on.  You can execute a transaction to the node
					to attempt the acquire on it, and -must- release a SECONDARY
					reference on the node which this function acquired. */
				bool					AttemptRefDescriptor(int32 descriptor,
															 binder_node **out_target);
				
				/*	Forcibly increment the primary reference count of the given,
					in response to a successful AttemptAcquire(). */
				void					ForceRefNode(binder_node *node, iobuffer *io);
				
				int32					Node2Descriptor(binder_node *node, bool ref = true);
				binder_node * 			Descriptor2Node(int32 descriptor, const void* id);
				status_t 				Ptr2Node(void *ptr, binder_node **n, iobuffer *io, const void* id);

				status_t				SetThreadEntry(thread_entry thread, void *ptr);
				
				status_t				TakeMeOffYourList();
				status_t				PutMeBackInTheGameCoach();

				void					SpawnThread();
				
	private:
	
				int32 					spawner();
		static	int32 					spawner_launcher(binder_team *team);
		static	status_t				binder_refs_2nodes(void *userData, type_code* type, uint32* value);
		static	status_t				binder_nodes_2refs(void *userData, type_code* type, uint32* value);
		static	status_t				free_binder_refs(void *userData, type_code* type, uint32* value);


				struct descriptor {
					binder_node *node;
					int32 priRef;
					int32 secRef;
				};

				struct reverse_mapping {
					binder_node *node;
					int32 descriptor;
					reverse_mapping *next;
				};

				struct local_mapping {
					void *ptr;
					binder_node *node;
					local_mapping *next;
				};
		
		static	int32					g_nextRequestID;
				int32 					m_primaryRefs;
				int32 					m_secondaryRefs;
				lock					m_lock;
				binder_thread *			m_mainThread;
				binder_thread *			m_threads;
				bigtime_t				m_wakeupTime;
				bigtime_t				m_idleTimeout;
				bigtime_t				m_replyTimeout;
				sem_id					m_spawnerSem;
				sem_id					m_ioSem;
				int32					m_syncCount;
				int32					m_freeCount;
				binder_transaction *	m_head;
				binder_transaction **	m_tail;
				binder_transaction *	m_needFree;
				binder_transaction *	m_eventTransaction;
				bool					m_eventInQueue;
				local_mapping *			m_localHash[HASH_SIZE];
				reverse_mapping *		m_reverseHash[HASH_SIZE];
				descriptor *			m_descriptors;
				int32					m_descriptorCount;
				thread_entry			m_entryFunc;
				void *					m_entryArg;
				int32					m_nonblockedThreads;
				int32					m_waitingThreads;
				int32					m_maxThreads;
				int32					m_idlePriority;
				int32					m_loopingThreads;
				int32					m_spawningThreads;
				thread_id				m_nextSpawn;
//				spinlock				m_scheduleLock;
};

#endif // BINDER2_TEAM_H
