
#include "binder_team.h"
#include "binder_thread.h"
#include "binder_driver.h"
#include "binder_node.h"
#include "binder_transaction.h"
#include "iobuffer.h"

#if BINDER_DEBUG_LIB
#include <stdlib.h>
#endif

//#define DBTRANSACT(x) x
#define DBTRANSACT(x)
//#define DBSPAWN(x) x
#define DBSPAWN(x)

void binder_team::Init(int32 id)
{
	ddprintf("************* Creating binder_team %p (id %ld) *************\n", this, id);
	next = NULL;
	teamID = id;
	m_primaryRefs = 0;
	m_secondaryRefs = 0;
	new_lock(&m_lock, "binder connection lock");
	m_threads = NULL;
	m_wakeupTime = B_INFINITE_TIMEOUT;
	m_idleTimeout = 5000000;
	m_replyTimeout = 5000000;
	m_spawnerSem = create_sem(0,"spawnsem");
	m_ioSem = create_sem(0,"binder I/O");
	m_syncCount = 0;
	m_freeCount = 0;
	m_head = NULL;
	m_tail = &m_head;
	m_needFree = NULL;
	m_eventTransaction = new(0) binder_transaction(0,0,NULL,0,NULL);
	m_eventTransaction->setEvent(true);
	m_eventInQueue = false;
	for (int32 i=0;i<HASH_SIZE;i++) {
		m_localHash[i] = NULL;
		m_reverseHash[i] = NULL;
	}
	m_descriptors = NULL;
	m_descriptorCount = 0;
	m_waitingThreads = 0;
	m_nonblockedThreads = 0;
	#if BINDER_DEBUG_LIB
		const char* env = getenv("BINDER_MAX_ASYNC_THREADS");
		m_maxThreads = env ? atoi(env) : 0;
		env = getenv("BINDER_IDLE_PRIORITY");
		m_idlePriority = env ? atoi(env) : 0;
		if (m_idlePriority < 0) m_idlePriority = B_REAL_TIME_PRIORITY;
		else if (m_idlePriority > B_REAL_TIME_PRIORITY) m_idlePriority = B_REAL_TIME_PRIORITY;
	#else
		m_maxThreads = 0;
		m_idlePriority = B_REAL_TIME_PRIORITY;
	#endif
	m_loopingThreads = 0;
	m_spawningThreads = 0;
	m_nextSpawn = B_BAD_THREAD_ID;
	m_mainThread = NULL;
	Acquire(PRIMARY);
}

binder_team::binder_team(int32 id)
{
	Init(id);
}

binder_team::binder_team(int32 id, int32 mainThid, binder_thread *parent)
{
	Init(id);
	m_mainThread = new binder_thread(mainThid,this,parent);
}

binder_team::~binder_team()
{
	delete m_eventTransaction;
}

binder_thread *
binder_team::OpenThread(int32 thid)
{
	binder_thread *t = NULL;
	if (m_mainThread && (m_mainThread->Thid() == thid)) {
		t = m_mainThread;
		m_mainThread = NULL;
	} else
		t = new binder_thread(thid,this);

	t->Acquire(PRIMARY);
	LOCK(m_lock);
	t->next = m_threads;
	m_threads = t;
	UNLOCK(m_lock);
	return t;
}

void
binder_team::RemoveThread(binder_thread *t)
{
	LOCK(m_lock)
	binder_thread **thread;
	for (thread = &m_threads; *thread && *thread != t; thread = &(*thread)->next)
		;
	ASSERT(*thread);
	*thread = (*thread)->next;
	UNLOCK(m_lock);
}

void 
binder_team::Released()
{
	DPRINTF(5,("binder_team::Released\n"));
	remove_team(teamID);
	delete_sem(m_spawnerSem);

	LOCK(m_lock);

	binder_transaction *cmd;
	local_mapping *lm;
	reverse_mapping *rm;
	binder_node *n;

	while ((cmd = m_head)) {
		m_head = cmd->next;
		delete cmd;
	}

	while ((cmd = m_needFree)) {
		m_needFree = cmd->next;
		delete cmd;
	}

	for (int32 i=0;i<HASH_SIZE;i++) {
		while ((lm = m_localHash[i])) {
			m_localHash[i] = lm->next;
			lm->node->Release(SECONDARY, this);
			sfree(lm);
		}
		while ((rm = m_reverseHash[i])) {
			m_reverseHash[i] = rm->next;
			sfree(rm);
		}
	}

	for (int32 i=0;i<m_descriptorCount;i++) {
		if ((n = m_descriptors[i].node)) {
			if (m_descriptors[i].priRef) n->Release(PRIMARY, this);
			if (m_descriptors[i].secRef) n->Release(SECONDARY, this);
		}
	}
	sfree(m_descriptors);
	
	delete_sem(m_ioSem);
	free_lock(&m_lock);
}

status_t 
binder_team::AddToNeedFreeList(binder_transaction *t)
{
	LOCK(m_lock);
	DPRINTF(5,("AddToNeedFreeList %p for team %p\n",t->data,this));
	t->next = m_needFree;
	m_needFree = t;
	m_freeCount++;
	UNLOCK(m_lock);
	return B_OK;
}

bool 
binder_team::AttemptAcquire()
{
	int32 current = m_primaryRefs;
	while (current > 0 && !cmpxchg32(&m_primaryRefs, &current, current+1));
	if (current <= 0) return false;
	atomic_add(&m_secondaryRefs,1);
	return true;
}

void 
binder_team::Acquire(int32 type)
{
	switch (type) {
		case PRIMARY:
//			ddprintf("m_primaryRefs=%ld\n",m_primaryRefs);
			atomic_add(&m_primaryRefs,1);
		case SECONDARY:
			atomic_add(&m_secondaryRefs,1);
		default: break;
	}
}

void 
binder_team::Release(int32 type)
{
	switch (type) {
		case PRIMARY:
//			ddprintf("m_primaryRefs=%ld\n",m_primaryRefs);
			if (atomic_add(&m_primaryRefs,-1) == 1) Released();
		case SECONDARY:
			if (atomic_add(&m_secondaryRefs,-1) == 1) delete this;
		default: break;
	}
}

int32 
binder_team::Node2Descriptor(binder_node *n, bool ref)
{
	int32 desc=-1;

	LOCK(m_lock);

	uint32 bucket = (((uint32)n) >> 3) & (HASH_SIZE-1);
	DPRINTF(5,("node(%p) mapping to descr bucket %ld (value %p) in team %p\n",n,bucket,m_reverseHash[bucket],this));
	reverse_mapping **head = &m_reverseHash[bucket];
	while (*head && (n < (*head)->node)) head = &(*head)->next;
	if (*head && (n == (*head)->node)) {
		desc = (*head)->descriptor;
		DPRINTF(5,("node(%p) found map to descriptor(%ld), pri=%ld\n",n,desc+1,m_descriptors[desc].priRef));
		if (!ref || m_descriptors[desc].priRef > 0 || n->AttemptAcquire(PRIMARY, this)) {
			if (ref) {
				DPRINTF(2,("Incrementing descriptor %ld primary: pri=%ld sec=%ld in team %p\n",
							desc+1, m_descriptors[desc].priRef,m_descriptors[desc].secRef,this));
				m_descriptors[desc].priRef++;
			}
			DPRINTF(5,("node(%p) mapped to descriptor(%ld) in team %p\n",n,desc+1,this));
		} else {
			// No longer exists!
			desc = -1;
		}
	} else if (ref && n->AttemptAcquire(PRIMARY, this)) {
		for (int32 i=0;i<m_descriptorCount;i++) {
			if (m_descriptors[i].node == NULL) {
				DPRINTF(2,("Initializing descriptor %ld: pri=%ld sec=%ld in team %p\n",
							i+1, m_descriptors[i].priRef,m_descriptors[i].secRef,this));
				m_descriptors[i].node = n;
				m_descriptors[i].priRef = 1;
				m_descriptors[i].secRef = 0;
				desc = i;
				DPRINTF(5,("node(%p) mapped to NEW descriptor(%ld) in team %p\n",n,desc+1,this));
				break;
			}
		}

		if (desc < 0) {
			int32 newCount = m_descriptorCount*2;
			if (!newCount) newCount = 32;
			m_descriptors = (descriptor*)realloc(m_descriptors,sizeof(descriptor)*newCount);
			for (int32 i=newCount-1;i>=m_descriptorCount;i--) m_descriptors[i].node = NULL;
			desc = m_descriptorCount;
			DPRINTF(2,("Initializing descriptor %ld: pri=%ld sec=%ld in team %p\n",
						desc+1, m_descriptors[desc].priRef,m_descriptors[desc].secRef,this));
			m_descriptors[desc].node = n;
			m_descriptors[desc].priRef = 1;
			m_descriptors[desc].secRef = 0;
			m_descriptorCount = newCount;
			DPRINTF(5,("node(%p) mapped to NEW descriptor(%ld) in team %p\n",n,desc+1,this));
		}

		reverse_mapping *map = (reverse_mapping*)smalloc(sizeof(reverse_mapping));
		map->node = n;
		map->descriptor = desc;
		map->next = *head;
		*head = map;
	}
	
	UNLOCK(m_lock);
	if (desc == -1) return -1;
	return desc+1;
}

binder_node *
binder_team::Descriptor2Node(int32 descriptor, const void* id)
{
	if (descriptor == ROOT_DESCRIPTOR) return get_root();

	descriptor--;

	LOCK(m_lock);

	binder_node *n = NULL;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		(m_descriptors[descriptor].node != NULL) &&
		(m_descriptors[descriptor].priRef > 0)) {
		n = m_descriptors[descriptor].node;
		n->Acquire(PRIMARY, id);
	} else {
		DPRINTF(5,("Descriptor2Node failed: desc=%ld, max=%ld, node=%p, pri=%ld\n",
			descriptor+1, m_descriptorCount,
			(descriptor >= 0 && descriptor < m_descriptorCount) ? m_descriptors[descriptor].node : NULL,
			(descriptor >= 0 && descriptor < m_descriptorCount) ? m_descriptors[descriptor].priRef : 0));
	}

	UNLOCK(m_lock);
	return n;
}

status_t 
binder_team::Ptr2Node(void *ptr, binder_node **n, iobuffer *io, const void* id)
{
	LOCK(m_lock);

	uint32 bucket = (((uint32)ptr) >> 3) & (HASH_SIZE-1);
	DPRINTF(5,("ptr(%p) mapping to ptr bucket %ld (value %p) in team %p\n",ptr,bucket,m_localHash[bucket],this));
	local_mapping **head = &m_localHash[bucket];
	while (*head && (ptr < (*head)->ptr)) head = &(*head)->next;
	if (*head && (ptr == (*head)->ptr)) {
		if ((*head)->node->AttemptAcquire(PRIMARY, id)) {
			*n = (*head)->node;
			DPRINTF(5,("ptr(%p) mapped to node(%p) in team %p\n",ptr,*n,this));
			UNLOCK(m_lock);
			return B_OK;
		} else if ((*head)->node->AttemptAcquire(SECONDARY, id)) {
			*n = (*head)->node;
			DPRINTF(5,("ptr(%p) mapped to node(%p) FROM SECONDARY in team %p\n",ptr,*n,this));
			/*	Other teams have a secondary reference on this node, but no
				primary reference.  We need to make the node alive again, and
				tell the calling team that the driver now has a primary
				reference on it.  The two calls below will force a new primary
				reference on the node, and remove the secondary reference we
				just acquired above.  All the trickery with the secondary reference
				is protection against a race condition where another team removes
				the last secondary reference on the object, while we are here
				trying to add one.
			*/
			DPRINTF(5,("Apply a new primary reference to node (%p) in team %p\n",*n,this));
			(*n)->Acquire(PRIMARY, id);
			(*n)->Release(SECONDARY, id);
			if (io) {
				io->write<int32>(brACQUIRE);
				io->write<void*>(ptr);
			}
			UNLOCK(m_lock);
			return B_OK;
		}
	}

	if (io && (io->remaining() < 8)) {
		UNLOCK(m_lock);
		return B_ERROR;
	}

	local_mapping *newMapping = (local_mapping*)smalloc(sizeof(local_mapping));
	newMapping->ptr = ptr;
	newMapping->node = new binder_node(this,ptr);
	*n = newMapping->node;
	DPRINTF(5,("ptr(%p) mapped to NEW node(%p) in team %p\n",ptr,*n,this));
	(*n)->Acquire(PRIMARY, id);
	newMapping->next = *head;
	*head = newMapping;
	
	if (io) {
		io->write<int32>(brACQUIRE);
		io->write<void*>(ptr);
		io->write<int32>(brINCREFS);
		io->write<void*>(ptr);
	}

	UNLOCK(m_lock);
	return B_OK;
}

bool 
binder_team::RefDescriptor(int32 descriptor, int32 type)
{
	bool r=false;
	if (descriptor == ROOT_DESCRIPTOR) return false;

	descriptor--;

	LOCK(m_lock);

	struct descriptor *d;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		((d=&m_descriptors[descriptor])->node != NULL)) {
		r = true;
		DPRINTF(2,("Incrementing descriptor %ld %s: pri=%ld sec=%ld in team %p\n",
					descriptor+1, type == PRIMARY ? "primary" : "secondary", d->priRef,d->secRef,this));
		if (type == PRIMARY) {
			if (d->priRef > 0) d->priRef++;
			else {
				debugger("No primary references exist for descriptor!");
				r = false;
			}
		} else if (type == SECONDARY) {
			if (d->secRef == 0 && d->node) d->node->Acquire(SECONDARY, this);
			d->secRef++;
		}
	}

	UNLOCK(m_lock);
	return r;
}

bool 
binder_team::UnrefDescriptor(int32 descriptor, int32 type)
{
	bool r=false;
	if (descriptor == ROOT_DESCRIPTOR) return false;

	descriptor--;

	LOCK(m_lock);

	binder_node *n = NULL;
	struct descriptor *d;
	bool remove = false;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		((d=&m_descriptors[descriptor])->node != NULL)) {
		r = true;
		DPRINTF(2,("Decrementing descriptor %ld %s: pri=%ld sec=%ld in team %p\n",
					descriptor+1, type == PRIMARY ? "primary" : "secondary", d->priRef,d->secRef,this));
		if (type == PRIMARY) {
			if (--d->priRef == 0) n = d->node;
		} else {
			if (--d->secRef == 0) n = d->node;
		}
		DPRINTF(2,("Descriptor %ld is now: pri=%ld sec=%ld in team %p\n",
					descriptor+1, d->priRef,d->secRef,this));
		if (n && d->priRef <= 0 && d->secRef <= 0) {
			d->node = NULL;
			remove = true;
		}
	}

	if (remove) {
		reverse_mapping *entry,**head = &m_reverseHash[(((uint32)n) >> 3) & (HASH_SIZE-1)];
		while (*head && (n < (*head)->node)) head = &(*head)->next;
		if (*head && (n == (*head)->node)) {
			entry = *head;
			*head = entry->next;
			sfree(entry);
		}
	}

	UNLOCK(m_lock);
	if (n) n->Release(type, this);
	return r;
}

bool 
binder_team::RemoveLocalMapping(void *ptr)
{
	LOCK(m_lock);

	DPRINTF(5,("RemoveLocalMapping %p in team %p\n", ptr, this));
	local_mapping *entry=NULL,**head = &m_localHash[(((uint32)ptr) >> 3) & (HASH_SIZE-1)];
	while (*head) {
//		ddprintf("RemoveLocalMapping %08x %08x\n",ptr,(*head)->ptr);
		if (ptr >= (*head)->ptr) break;
		head = &(*head)->next;
	}

//	while (*head && (ptr <= (*head)->ptr)) head = &(*head)->next;
	if (*head && (ptr == (*head)->ptr)) {
		entry = *head;
		*head = entry->next;
	}

	UNLOCK(m_lock);
	
	if (entry) {
		sfree(entry);
//		ddprintf("RemoveLocalMapping success\n");
		return true;
	}

	ddprintf("RemoveLocalMapping failed for %p in team %p\n", ptr, this);
	return false;
}

bool 
binder_team::AttemptRefDescriptor(int32 descriptor, binder_node **out_target)
{
	bool r=false;
	if (descriptor == ROOT_DESCRIPTOR) return false;

	descriptor--;

	LOCK(m_lock);

	struct descriptor *d;
	binder_node *n = NULL;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		((d=&m_descriptors[descriptor])->node != NULL)) {
		r = true;
		DPRINTF(2,("Attempt incrementing descriptor %ld primary: pri=%ld sec=%ld in team %p\n",
					descriptor+1, d->priRef,d->secRef,this));
		if (d->priRef > 0 || (d->node && d->node->AttemptAcquire(PRIMARY, this))) {
			d->priRef++;
		} else {
			// If no primary references currently exist, we can't
			// succeed.  Instead return the node this attempt was
			// made on.
			r = false;
			if ((n=d->node) != NULL) n->Acquire(SECONDARY, this);
		}
	}

	UNLOCK(m_lock);
	
	*out_target = n;
	return r;
}

void
binder_team::ForceRefNode(binder_node *node, iobuffer */*io*/)
{
	const int32 descriptor = Node2Descriptor(node, false) - 1;
	
	LOCK(m_lock);

	struct descriptor *d;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		((d=&m_descriptors[descriptor])->node != NULL)) {
		DPRINTF(2,("Force incrementing descriptor %ld: pri=%ld sec=%ld in team %p\n",
					descriptor+1, d->priRef, d->secRef,this));
		if (d->priRef == 0) node->Acquire(PRIMARY, this);
		d->priRef++;
	} else {
		debugger("ForceRefNode() got invalid descriptor!");
	}

	UNLOCK(m_lock);
}

status_t 
binder_team::FreeBuffer(void *ptr)
{
	LOCK(m_lock);
	binder_transaction **p,*t;
	for (p = &m_needFree; *p && (((void*)(*p)->data) != ptr); p = &(*p)->next);
	if ((t = *p)) *p = t->next;
	if (t) m_freeCount--;
	UNLOCK(m_lock);

	if (t) {
		DPRINTF(5,("FreeBuffer %p in team %p, now have %ld\n",ptr,this,m_freeCount));
		delete t;
		return B_OK;
	} else
		debugger("FreeBuffer failed");
	return B_ERROR;
}

status_t 
binder_team::Transact(binder_transaction *t)
{
	LOCK(m_lock);

	DBTRANSACT(ddprintf("Thread %ld transacting %p to team %p, vthid=%ld\n",
			find_thread(NULL), t, this, t->sender ? t->sender->virtualThid : -1));
	
	/*	First check if the target team is already waiting on a reply from
		this thread.  If so, we must reflect this transaction directly
		into the thread that is waiting for us.
	*/
	if (t->sender && t->sender->virtualThid) {
		binder_thread *thread;
		for (thread = m_threads;
			 thread &&
			 	(thread->virtualThid != t->sender->virtualThid) &&
			 	(thread->Thid() != t->sender->virtualThid);
			 thread = thread->next);

		if (thread) {
			UNLOCK(m_lock);
			DBTRANSACT(printf("Thread %ld reflecting %p!\n", find_thread(NULL), t));
			thread->Reflect(t);
			return B_OK;
		}
	}
	
	const bool firstTransaction = m_head == NULL;
	const bool needNewThread = !m_nonblockedThreads;
	*m_tail = t;
	m_tail = &t->next;
	ASSERT(t->next == NULL);
	
	ASSERT(m_syncCount >= 0);
	m_syncCount++;
	
	DBTRANSACT(ddprintf("Added to team %p queue -- first=%d, needNew=%d, m_nonblockedThreads=%ld\n",
			this, firstTransaction, needNewThread, m_nonblockedThreads));
	
	UNLOCK(m_lock);
	
	if (firstTransaction) release_sem_etc(m_ioSem,1,B_DO_NOT_RESCHEDULE);
	if (needNewThread) {
		DBSPAWN(ddprintf("*** TRANSACT NEEDS TO SPAWN NEW THREAD!\n"));
		SpawnThread();
	}
	return B_OK;
}

status_t 
binder_team::TakeMeOffYourList()
{
	LOCK(m_lock);
	m_nonblockedThreads--;
	DBSPAWN(ddprintf("*** TAKE-ME-OFF-YOUR-LIST %p -- now have %ld nonblocked\n", this, m_nonblockedThreads));
	ASSERT(m_nonblockedThreads >= 0);
	if (!m_nonblockedThreads && m_syncCount) {
		/* Spawn a thread if all blocked and synchronous transaction pending */
		DBSPAWN(ddprintf("*** TAKE-ME-OFF-YOUR-LIST NEEDS TO SPAWN NEW THREAD!\n"));
		SpawnThread();
	}
	UNLOCK(m_lock);
	return B_OK;
}

status_t 
binder_team::PutMeBackInTheGameCoach()
{
	LOCK(m_lock);
	ASSERT(m_nonblockedThreads >= 0);
	m_nonblockedThreads++;
	DBSPAWN(ddprintf("*** PUT-ME-BACK-IN-THE-GAME-COACH %p -- now have %ld nonblocked\n", this, m_nonblockedThreads));
	UNLOCK(m_lock);
	return B_OK;
}

status_t 
binder_team::WaitForRequest(binder_thread* /*who*/, binder_transaction **t)
{
	/*	Eventually, we want to use our own synchronization, because we want a thread
		stack, not a queue.  For now, though, we just use a semaphore.  Wait on it
		until our timeout hits or we get a transaction request. */

	status_t err = B_OK;
	
	bigtime_t idleTime = system_time() + m_idleTimeout;
	
	LOCK(m_lock);
	m_waitingThreads++;
	
	set_thread_priority(find_thread(NULL), m_idlePriority);
	
	do {
//		status_t err = acquire_sem_etc(m_ioSem,1,B_CAN_INTERRUPT|B_RELATIVE_TIMEOUT,m_idleTimeout);
//		status_t err = acquire_sem_etc(m_ioSem,1,B_CAN_INTERRUPT,0);
		if (m_nextSpawn >= B_OK && m_nextSpawn == find_thread(NULL)) {
			DBSPAWN(ddprintf("*** ENTERING SPAWNED THREAD!  Now looping %ld, spawning %ld\n",
					m_loopingThreads, m_spawningThreads));
			m_nextSpawn = B_BAD_THREAD_ID;
			if (atomic_add(&m_spawningThreads, -1) > 1)
				release_sem_etc(m_spawnerSem,1,B_DO_NOT_RESCHEDULE);
		}
		const bigtime_t now = system_time();
		
		/*	Look for a pending request to service. */
		binder_transaction** tr = &m_head;
		while ((*tr) != NULL) {
			if (!(*tr)->isEvent()) {
				/*	If the next is a normal transaction, just use it. */
				break;
			} else if (now >= m_wakeupTime) {
				/*	This is an event transaction, and time for the event
					to occur.  Use it. */
				break;
			}
			tr = &(*tr)->next;
		}
		
		if ((*t = *tr) != NULL) {
//			ddprintf("Processing transaction %p, next is %p\n", *tr, (*tr)->next);
			*tr = (*t)->next;
			if (m_tail == &(*t)->next) m_tail = tr;
			(*t)->next = NULL;
			if ((*t)->isEvent()) {
				ASSERT(*t == m_eventTransaction);
				
				/*	Tell caller to process an event. */
				err = REQUEST_EVENT_READY;
				*t = NULL;
				
				/*	Clear out current event information. */
				m_eventInQueue = false;
				m_wakeupTime = B_INFINITE_TIMEOUT;
			}
			
			/*	Spawn a new looper thread if there are no more waiting
				and we have not yet reached our limit. */
			if (m_waitingThreads <= 1 && m_spawningThreads <= 0
					&& m_loopingThreads < m_maxThreads) {
				DBSPAWN(ddprintf("*** I THINK I WANT TO SPAWN A LOOPER THREAD!\n"));
				SpawnThread();
			}
			
		} else {
			/*	If there are no pending transactions, unlock the team state and
				wait for next thing to do. */
			const bigtime_t eventTime = m_wakeupTime;
			
			UNLOCK(m_lock);
			do {
				err = acquire_sem_etc(	m_ioSem,1,B_CAN_INTERRUPT|B_ABSOLUTE_TIMEOUT,
										eventTime < idleTime ? eventTime : idleTime);
			} while (err == B_INTERRUPTED);
			LOCK(m_lock);
			
			if (err == B_TIMED_OUT || err == B_WOULD_BLOCK) {
				if (now < idleTime) {
					/*	Don't allow looper to exit if we timed out before the idle
						time was up. */
					err = B_OK;
				} else if (m_waitingThreads <= 1 || m_nonblockedThreads <= 1) {
					/*	Don't allow looper to exit if there are no other threads
						waiting or if this is the only non-blocked thread.  In
						this case the idle time also must be reset. */
					idleTime = system_time() + m_idleTimeout;
					err = B_OK;
				} else {
					DBSPAWN(ddprintf("*** TIME TO DIE!  waiting=%ld, nonblocked=%ld\n",
							m_waitingThreads, m_nonblockedThreads));
				}
			}
		}
		
	} while (err == B_OK && (*t) == NULL);
	
	DBTRANSACT(if ((*t) != NULL) ddprintf("*** EXECUTING TRANSACTION %p FROM %ld IN %ld\n",
			*t, (*t)->sender ? (*t)->sender->Thid() : -1, find_thread(NULL)));
	
	if ((*t)) {
		ASSERT(m_syncCount >= 0);
		set_thread_priority(find_thread(NULL), (*t)->priority);
		m_syncCount--;
	} else {
		DBTRANSACT(ddprintf("*** ASYNCHRONOUS!\n"));
		// For now, all asynchronous messages run at priority 10.
		set_thread_priority(find_thread(NULL), 10);
	}
	
	m_waitingThreads--;
	UNLOCK(m_lock);
	
	return err;
}

void
binder_team::StartLooper()
{
	LOCK(m_lock);
	m_nonblockedThreads++;
	DBSPAWN(ddprintf("*** STARTING A LOOPER FOR %p!  Now have %ld waiting, %ld nonblocked.\n",
			this, m_waitingThreads, m_nonblockedThreads));
	UNLOCK(m_lock);
}

void
binder_team::FinishLooper(bool driverSpawned)
{
	LOCK(m_lock);
	m_nonblockedThreads--;
	DBSPAWN(ddprintf("*** FINISHING A LOOPER FOR %p!  Now have %ld waiting, %ld nonblocked, %ld looping.\n",
			this, m_waitingThreads, m_nonblockedThreads, m_loopingThreads));
	/*	XXX We shouldn't spawn more threads when shutting down an application! */
	if (!m_nonblockedThreads && m_syncCount) {
		/* Spawn a thread if all blocked and synchronous transaction pending */
		DBSPAWN(ddprintf("*** FINISH-LOOPER NEEDS TO SPAWN NEW THREAD!\n"));
		SpawnThread();
	}
	UNLOCK(m_lock);
	
	if (driverSpawned) {
		atomic_add(&m_loopingThreads, -1);
		ASSERT(m_loopingThreads >= 0);
	}
}

int32 
binder_team::SetWakeupTime(bigtime_t time)
{
	LOCK(m_lock);
	if (time != m_wakeupTime) {
		const bool earlier = time < m_wakeupTime;
		m_wakeupTime = time;
		if (time != B_INFINITE_TIMEOUT) {
			if (!m_eventInQueue) {
				ASSERT(m_eventTransaction->next == NULL);
				*m_tail = m_eventTransaction;
				m_tail = &m_eventTransaction->next;
				m_eventInQueue = true;
			}
			/*	If the wakeup time is earlier than what was last set, make sure
				everyone sees it. */
			if (earlier) {
				release_sem_etc(m_ioSem,m_waitingThreads,B_DO_NOT_RESCHEDULE);
			}
		}
	}
	UNLOCK(m_lock);
	return B_OK;
}

int32 
binder_team::SetIdleTimeout(bigtime_t timeDelta)
{
	m_idleTimeout = timeDelta;
	return B_OK;
}

int32 
binder_team::SetReplyTimeout(bigtime_t timeDelta)
{
	m_replyTimeout = timeDelta;
	return B_OK;
}

status_t 
binder_team::SetThreadEntry(thread_entry thread, void *ptr)
{
	m_entryFunc = thread;
	m_entryArg = ptr;
	Acquire(SECONDARY);
	resume_thread(spawn_spawner((thread_entry)spawner_launcher,this));
	return B_OK;
}

void
binder_team::SpawnThread()
{
	if (atomic_add(&m_spawningThreads, 1) == 0) {
		release_sem_etc(m_spawnerSem,1,B_DO_NOT_RESCHEDULE);
	}
	DBSPAWN(ddprintf("*** SPAWNING A NEW LOOPER!  This will be #%ld, now spawning %ld\n",
			m_loopingThreads, m_spawningThreads));
}

int32 
binder_team::spawner()
{
	static int32 nextThreadID = 1;
	status_t err;
	while ((err=acquire_sem(m_spawnerSem)) >= B_OK || err == B_INTERRUPTED) {
		ASSERT(m_nextSpawn == B_BAD_THREAD_ID);
		char spawnName[32];
		sprintf(spawnName, "BLooper #%ld", atomic_add(&nextThreadID, 1));
		m_nextSpawn = uspace_spawn(m_entryFunc,spawnName,B_NORMAL_PRIORITY,m_entryArg);
		atomic_add(&m_loopingThreads, 1);
		DBSPAWN(ddprintf("*** SPAWNER IS SPAWNING %ld\n", m_nextSpawn));
		resume_thread(m_nextSpawn);
	}
	Release(SECONDARY);
	return 0;
}

int32 
binder_team::spawner_launcher(binder_team *team)
{
	return team->spawner();
}
