
#include "binder_node.h"
#include "binder_team.h"
#include "binder_transaction.h"

/*
binder_node *
binder_node::New(binder_team *team, void *ptr)
{
	binder_node *n = (binder_node*)smalloc(sizeof(binder_node));
	n->binder_node();
	return n;
}

void 
binder_node::Delete(binder_node *n)
{
	n->~binder_node();
	sfree(n);
}
*/
binder_node::binder_node(binder_team *team, void *ptr)
{
//	ddprintf(("binder_node::binder_node(0x%08x,0x%08x)\n",team,ptr));
	m_primaryRefs = m_secondaryRefs = 0;
	m_team = team;
	m_ptr = ptr;
	if (m_team) m_team->Acquire(PRIMARY);
}

binder_node::~binder_node()
{
//	ddprintf(("binder_node::~binder_node()\n"));
	if (m_team) m_team->Release(PRIMARY);
}

void 
binder_node::Released()
{
//	printf("binder_node::Released %08x %08x\n",this,m_ptr);
	if (m_team) {
//		printf("m_secondaryRefs1=%ld\n",m_secondaryRefs);
		m_team->RemoveLocalMapping(m_ptr);
//		printf("m_secondaryRefs2=%ld\n",m_secondaryRefs);
		m_team->Transact(new binder_transaction(PRIMARY,m_ptr), false);
	}
}

void *
binder_node::Ptr()
{
	return m_ptr;
}

binder_team *
binder_node::Home()
{
	return m_team;
}

bool
binder_node::PreSend(binder_transaction *t)
{
	return m_team->PreTransact(t);
}

void 
binder_node::Send(binder_transaction *t, bool did_pre)
{
	m_team->Transact(t, did_pre);
}

bool 
binder_node::AttemptAcquire()
{
	int32 current = m_primaryRefs;
	while (current > 0 && !cmpxchg32(&m_primaryRefs, &current, current+1));
	if (current <= 0) {
		ddprintf(("AttemptAcquire failed with %ld for %p\n",current, m_ptr));
		return false;
	}
	atomic_add(&m_secondaryRefs,1);
	return true;
}

void 
binder_node::Acquire(int32 type)
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
binder_node::Release(int32 type)
{
	int32 old;
	switch (type) {
		case PRIMARY:
			old = atomic_add(&m_primaryRefs,-1);
//			printf("binder_node: primary ref released (was %ld)\n",old);
			if (old == 1) Released();
		case SECONDARY:
			if (atomic_add(&m_secondaryRefs,-1) == 1) delete this;
		default: break;
	}
}
