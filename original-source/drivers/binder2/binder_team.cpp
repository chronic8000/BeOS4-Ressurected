
#include "binder_team.h"
#include "binder_thread.h"
#include "binder_driver.h"
#include "binder_node.h"
#include "binder_transaction.h"
#include "iobuffer.h"

//#define DBTRANSACT(x) x
#define DBTRANSACT(x)
//#define DBSPAWN(x) x
#define DBSPAWN(x)

void binder_team::Init(int32 id)
{
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
	m_head = NULL;
	m_tail = &m_head;
	m_needFree = NULL;
	m_eventTransaction = new(0) binder_transaction(0,0,NULL);
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
	m_maxThreads = 0;
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
			lm->node->Release(SECONDARY);
			sfree(lm);
		}
		while ((rm = m_reverseHash[i])) {
			m_reverseHash[i] = rm->next;
			sfree(rm);
		}
	}

	for (int32 i=0;i<m_descriptorCount;i++) {
		if ((n = m_descriptors[i].node))
			n->Release(SECONDARY);
	}
	sfree(m_descriptors);
	
	delete_sem(m_ioSem);
	free_lock(&m_lock);
}

status_t 
binder_team::AddToNeedFreeList(binder_transaction *t)
{
	LOCK(m_lock);
	DPRINTF(5,("AddToNeedFreeList %p\n",t->data));
	t->next = m_needFree;
	m_needFree = t;
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
//			printf("m_primaryRefs=%ld\n",m_primaryRefs);
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
//			printf("m_primaryRefs=%ld\n",m_primaryRefs);
			if (atomic_add(&m_primaryRefs,-1) == 1) Released();
		case SECONDARY:
			if (atomic_add(&m_secondaryRefs,-1) == 1) delete this;
		default: break;
	}
}

int32 
binder_team::Node2Descriptor(binder_node *n)
{
	int32 desc=-1;

	LOCK(m_lock);

	uint32 bucket = (((uint32)n) >> 3) & (HASH_SIZE-1);
	DPRINTF(5,("node(%p) mapping to bucket %ld (value %p)\n",n,bucket,m_reverseHash[bucket]));
	reverse_mapping **head = &m_reverseHash[bucket];
	while (*head && (n < (*head)->node)) head = &(*head)->next;
	if (*head && (n == (*head)->node)) {
		desc = (*head)->descriptor;
		m_descriptors[desc].refs++;
		DPRINTF(5,("node(%p) mapped to descriptor(%ld)\n",n,desc+1));
	} else if (n->AttemptAcquire()) {
		for (int32 i=0;i<m_descriptorCount;i++) {
			if (m_descriptors[i].node == NULL) {
				m_descriptors[i].node = n;
				m_descriptors[i].refs = 1;
				desc = i;
				DPRINTF(5,("node(%p) mapped to NEW descriptor(%ld)\n",n,desc+1));
				break;
			}
		}

		if (desc < 0) {
			int32 newCount = m_descriptorCount*2;
			if (!newCount) newCount = 32;
			m_descriptors = (descriptor*)realloc(m_descriptors,sizeof(descriptor)*newCount);
			for (int32 i=newCount-1;i>=m_descriptorCount;i--) m_descriptors[i].node = NULL;
			desc = m_descriptorCount;
			m_descriptors[desc].node = n;
			m_descriptors[desc].refs = 1;
			m_descriptorCount = newCount;
			DPRINTF(5,("node(%p) mapped to NEW descriptor(%ld)\n",n,desc+1));
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
binder_team::Descriptor2Node(int32 descriptor)
{
	if (descriptor == ROOT_DESCRIPTOR) return get_root();

	descriptor--;

	LOCK(m_lock);

	binder_node *n = NULL;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		(m_descriptors[descriptor].node != NULL)) {
		n = m_descriptors[descriptor].node;
		n->Acquire(PRIMARY);
	}

	UNLOCK(m_lock);
	return n;
}

status_t 
binder_team::Ptr2Node(void *ptr, binder_node **n, iobuffer *io)
{
	LOCK(m_lock);

	uint32 bucket = (((uint32)ptr) >> 3) & (HASH_SIZE-1);
	DPRINTF(5,("ptr(%p) mapping to bucket %ld (value %p)\n",ptr,bucket,m_localHash[bucket]));
	local_mapping **head = &m_localHash[bucket];
	while (*head && (ptr < (*head)->ptr)) head = &(*head)->next;
	if (*head && (ptr == (*head)->ptr)) {
		if ((*head)->node->AttemptAcquire()) {
			*n = (*head)->node;
			DPRINTF(5,("ptr(%p) mapped to node(%p)\n",ptr,*n));
			UNLOCK(m_lock);
			return B_OK;
		} else {
			printf("AttemptAcquire failure in team %p\n", this);
		}
	}

	if (io && (io->remaining() < 8)) {
		UNLOCK(m_lock);
		return B_ERROR;
	}

	local_mapping *newMapping = (local_mapping*)smalloc(sizeof(local_mapping));
	newMapping->ptr = ptr;
	newMapping->node = new binder_node(this,ptr);
	newMapping->node->Acquire(SECONDARY);
	*n = newMapping->node;
	DPRINTF(5,("ptr(%p) mapped to NEW node(%p)\n",ptr,*n));
	(*n)->Acquire(PRIMARY);
	newMapping->next = *head;
	*head = newMapping;
	
	if (io) {
		io->write<int32>(brACQUIRE);
		io->write<void*>(ptr);
	}

	UNLOCK(m_lock);
	return B_OK;
}

bool 
binder_team::RefDescriptor(int32 descriptor)
{
	bool r=false;
	if (descriptor == ROOT_DESCRIPTOR) return false;

	descriptor--;

	LOCK(m_lock);

	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		(m_descriptors[descriptor].node != NULL)) {
		r = true;
//		printf("\n\n\nIncrementing descriptor %ld %ld\n",descriptor,m_descriptors[descriptor].refs);
		m_descriptors[descriptor].refs++;
	}

	UNLOCK(m_lock);
	return r;
}

bool 
binder_team::UnrefDescriptor(int32 descriptor)
{
	bool r=false;
	if (descriptor == ROOT_DESCRIPTOR) return false;

	descriptor--;

	LOCK(m_lock);

	binder_node *n = NULL;
	if ((descriptor >= 0) &&
		(descriptor < m_descriptorCount) &&
		(m_descriptors[descriptor].node != NULL)) {
		r = true;
		DPRINTF(5,("Decrementing descriptor %ld %ld\n",descriptor,m_descriptors[descriptor].refs));
		if (--m_descriptors[descriptor].refs == 0) {
			n = m_descriptors[descriptor].node;
			m_descriptors[descriptor].node = NULL;
		}
	}

	if (n) {
		reverse_mapping *entry,**head = &m_reverseHash[(((uint32)n) >> 3) & (HASH_SIZE-1)];
		while (*head && (n < (*head)->node)) head = &(*head)->next;
		if (*head && (n == (*head)->node)) {
			entry = *head;
			*head = entry->next;
			sfree(entry);
		}
	}

	UNLOCK(m_lock);
	if (n) n->Release(PRIMARY);
	return r;
}

bool 
binder_team::RemoveLocalMapping(void *ptr)
{
	LOCK(m_lock);

	local_mapping *entry=NULL,**head = &m_localHash[(((uint32)ptr) >> 3) & (HASH_SIZE-1)];
	while (*head) {
//		printf("RemoveLocalMapping %08x %08x\n",ptr,(*head)->ptr);
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
		entry->node->Release(SECONDARY);
		sfree(entry);
//		printf("RemoveLocalMapping success\n");
		return true;
	}

	printf("RemoveLocalMapping failed for %p in team %p\n", ptr, this);
	return false;
}

status_t 
binder_team::FreeBuffer(void *ptr)
{
	DPRINTF(5,("FreeBuffer %p\n",ptr));
	LOCK(m_lock);
	binder_transaction **p,*t;
	for (p = &m_needFree; *p && (((void*)(*p)->data) != ptr); p = &(*p)->next);
	if ((t = *p)) *p = t->next;
	UNLOCK(m_lock);

	if (t) {
		delete t;
		return B_OK;
	} else
		debugger("FreeBuffer failed");
	return B_ERROR;
}

bool
binder_team::PreTransact(binder_transaction *t, bool in_transact)
{
	LOCK(m_lock);

	DBTRANSACT(printf("Thread %ld pre-transacting %p to team %p, vthid=%ld\n",
			find_thread(NULL), t, this, t->sender ? t->sender->virtualThid : -1));
	bool needNewThread = false;
	const bool isSync = t->isSync();
	if (isSync) {
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
				return true;
			}
		}
		
		ASSERT(m_syncCount >= 0);
		m_syncCount++;
		needNewThread = !m_nonblockedThreads;
	}
	
	UNLOCK(m_lock);
	
	if (!in_transact) {
		if (isSync) release_sem_etc(m_ioSem,1,B_DO_NOT_RESCHEDULE);
		if (needNewThread) {
			DBSPAWN(printf("*** PRE-TRANSACT NEEDS TO SPAWN NEW THREAD!\n"));
			SpawnThread();
		}
	}
	return false;
}

status_t 
binder_team::Transact(binder_transaction *t, bool did_pre)
{
	if (!did_pre && PreTransact(t, true)) return B_OK;
	
	LOCK(m_lock);

	DBTRANSACT(printf("Thread %ld transacting %p to team %p, vthid=%ld\n",
			find_thread(NULL), t, this, t->sender ? t->sender->virtualThid : -1));
	
	const bool firstTransaction = m_head == NULL;
	*m_tail = t;
	m_tail = &t->next;
	ASSERT(t->next == NULL);
	
	bool needNewThread = false;
	if (t->isSync()) {
		ASSERT(m_syncCount >= 0);
		if (!did_pre) m_syncCount++;
		needNewThread = !m_nonblockedThreads;
	}
	
	DBTRANSACT(printf("Added to team %p queue -- first=%d, needNew=%d, m_nonblockedThreads=%ld\n",
			this, firstTransaction, needNewThread, m_nonblockedThreads));
	
	UNLOCK(m_lock);
	
	if (firstTransaction) release_sem_etc(m_ioSem,1,B_DO_NOT_RESCHEDULE);
	if (needNewThread) {
		DBSPAWN(printf("*** TRANSACT NEEDS TO SPAWN NEW THREAD!\n"));
		SpawnThread();
	}
	return B_OK;
}

status_t 
binder_team::TakeMeOffYourList()
{
	LOCK(m_lock);
	m_nonblockedThreads--;
	DBSPAWN(printf("*** TAKE-ME-OFF-YOUR-LIST %p -- now have %ld nonblocked\n", this, m_nonblockedThreads));
	ASSERT(m_nonblockedThreads >= 0);
	if (!m_nonblockedThreads && m_syncCount) {
		/* Spawn a thread if all blocked and synchronous transaction pending */
		DBSPAWN(printf("*** TAKE-ME-OFF-YOUR-LIST NEEDS TO SPAWN NEW THREAD!\n"));
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
	DBSPAWN(printf("*** PUT-ME-BACK-IN-THE-GAME-COACH %p -- now have %ld nonblocked\n", this, m_nonblockedThreads));
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
	
	do {
//		status_t err = acquire_sem_etc(m_ioSem,1,B_CAN_INTERRUPT|B_RELATIVE_TIMEOUT,m_idleTimeout);
//		status_t err = acquire_sem_etc(m_ioSem,1,B_CAN_INTERRUPT,0);
		if (m_nextSpawn >= B_OK && m_nextSpawn == find_thread(NULL)) {
			DBSPAWN(printf("*** ENTERING SPAWNED THREAD!  Now looping %ld, spawning %ld\n",
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
//			printf("Processing transaction %p, next is %p\n", *tr, (*tr)->next);
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
				DBSPAWN(printf("*** I THINK I WANT TO SPAWN A LOOPER THREAD!\n"));
				SpawnThread();
			}
			
		} else {
			/*	If there are no pending transactins, unlock the team state and
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
					DBSPAWN(printf("*** TIME TO DIE!  waiting=%ld, nonblocked=%ld\n",
							m_waitingThreads, m_nonblockedThreads));
				}
			}
		}
		
	} while (err == B_OK && (*t) == NULL);
	
	DBTRANSACT(if ((*t) != NULL) printf("*** EXECUTING TRANSACTION %p FROM %ld IN %ld\n",
			*t, (*t)->sender ? (*t)->sender->Thid() : -1, find_thread(NULL)));
	
	if ((*t) && (*t)->isSync()) {
		ASSERT(m_syncCount >= 0);
		m_syncCount--;
	} else {
		DBTRANSACT(printf("*** ASYNCHRONOUS!\n"));
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
	DBSPAWN(printf("*** STARTING A LOOPER FOR %p!  Now have %ld waiting, %ld nonblocked.\n",
			this, m_waitingThreads, m_nonblockedThreads));
	UNLOCK(m_lock);
}

void
binder_team::FinishLooper(bool driverSpawned)
{
	LOCK(m_lock);
	m_nonblockedThreads--;
	DBSPAWN(printf("*** FINISHING A LOOPER FOR %p!  Now have %ld waiting, %ld nonblocked, %ld looping.\n",
			this, m_waitingThreads, m_nonblockedThreads, m_loopingThreads));
	/*	XXX We shouldn't spawn more threads when shutting down an application! */
	if (!m_nonblockedThreads && m_syncCount) {
		/* Spawn a thread if all blocked and synchronous transaction pending */
		DBSPAWN(printf("*** FINISH-LOOPER NEEDS TO SPAWN NEW THREAD!\n"));
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
	DBSPAWN(printf("*** SPAWNING A NEW LOOPER!  This will be #%ld, now spawning %ld\n",
			m_loopingThreads, m_spawningThreads));
}

int32 
binder_team::spawner()
{
	status_t err;
	while ((err=acquire_sem(m_spawnerSem)) >= B_OK || err == B_INTERRUPTED) {
		ASSERT(m_nextSpawn == B_BAD_THREAD_ID);
		m_nextSpawn = uspace_spawn(m_entryFunc,"BLooper",B_NORMAL_PRIORITY,m_entryArg);
		atomic_add(&m_loopingThreads, 1);
		DBSPAWN(printf("*** SPAWNER IS SPAWNING %ld\n", m_nextSpawn));
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
