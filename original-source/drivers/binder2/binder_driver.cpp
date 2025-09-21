
#include "binder_driver.h"
#include "binder_team.h"
#include "binder_node.h"

static area_id			area;
static void *			base;
static malloc_state		heapMS;
static malloc_funcs		heapMF;
static int32			heapUsed;
static lock				teamLock;
static binder_team *	teamHash[HASH_SIZE];
static binder_node *	rootNode;
static lock				rootNodeLock;
static sem_id			rootNodeSem;

static void *morecore(ptrdiff_t increment, malloc_state *)
{
	if ((heapUsed + increment) > HEAP_SIZE) {
		debugger("binder_driver out of heap!");
		return NULL;
	}
	void *r = ((uint8*)base) + heapUsed;
	heapUsed += increment;
	return r;
}

status_t init_binder()
{
	base = NULL;
	memset(&heapMS,sizeof(heapMS),0);
	memset(&heapMF,sizeof(heapMF),0);
	heapMF.morecore = morecore;
	heapMS.malloc_sem = create_sem(0, "binder malloc");
	heapMS.malloc_lock = 0;
	heapUsed = 0;
	new_lock(&teamLock,"binder team lock");
	rootNode = NULL;
	new_lock(&rootNodeLock,"binder root node lock");
	rootNodeSem = create_sem(0,"root node sem");
#if BINDER_DEBUG_LIB
	area = create_area("binder I/O",&base,B_ANY_ADDRESS,HEAP_SIZE,B_NO_LOCK,B_READ_AREA);
#else
	area = create_area("binder I/O",&base,B_ANY_KERNEL_ADDRESS,HEAP_SIZE,B_NO_LOCK,B_READ_AREA);
#endif
	for (int32 i=0;i<HASH_SIZE;i++) teamHash[i] = NULL;
	return B_OK;
}

void uninit_binder()
{
	delete_sem(teamLock.s);
	delete_sem(heapMS.malloc_sem);
	delete_area(area);
}

void remove_team(int32 id)
{
	LOCK(teamLock);
	binder_team **t,*theTeam;
	for (t = &teamHash[id & (HASH_SIZE-1)]; *t && ((*t)->teamID != id); t = &(*t)->next);
	if ((theTeam = *t)) *t = theTeam->next;
	UNLOCK(teamLock);

	if (theTeam) theTeam->Release(SECONDARY);
}

void set_root(binder_node *n)
{
	LOCK(rootNodeLock);
	binder_node *old = rootNode;
	rootNode = n;
	UNLOCK(rootNodeLock);
	if (old) old->Release(PRIMARY);
	else delete_sem(rootNodeSem);
}

binder_node *get_root()
{
	if (!rootNode) acquire_sem(rootNodeSem);
	LOCK(rootNodeLock);
	binder_node *n = rootNode;
	n->Acquire(PRIMARY);
	UNLOCK(rootNodeLock);
	return n;
}

binder_team *get_team(int32 teamID, int32 thid, binder_thread *parent)
{
	binder_team *t;
	LOCK(teamLock);

	for (t = teamHash[teamID & (HASH_SIZE-1)]; t && (t->teamID != teamID); t = t->next);
	
	if (!t) {
		if (thid)	t = new binder_team(teamID,thid,parent);
		else		t = new binder_team(teamID);
		t->Acquire(SECONDARY);
		t->next = teamHash[teamID & (HASH_SIZE-1)];
		teamHash[teamID & (HASH_SIZE-1)] = t;
	}

	t->Acquire(SECONDARY);
	UNLOCK(teamLock);
	
	return t;
}

binder_team *get_team(int32 teamID)
{
	return get_team(teamID,0,NULL);
}

#if BINDER_DEBUG_LIB

void * shared_malloc(int32 size)
{
	return malloc(size);
}

void * shared_realloc(void *ptr, int32 size)
{
	return realloc(ptr,size);
}

void shared_free(void *ptr)
{
	free(ptr);
}

#else

void * shared_malloc(int32 size)
{
	return _malloc(size, &heapMS, &heapMF);
}

void * shared_realloc(void *ptr, int32 size)
{
	return _realloc(ptr,size, &heapMS, &heapMF);
}

void shared_free(void *ptr)
{
	_free(ptr, &heapMS, &heapMF);
}

#endif
