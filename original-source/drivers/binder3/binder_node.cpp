
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
	DPRINTF(5,("binder_node %p: binder_node() team=%p, ptr=%p\n",this,team,ptr));
	m_primaryRefs = m_secondaryRefs = 0;
	m_team = team;
	m_ptr = ptr;
	if (m_team) m_team->Acquire(PRIMARY);
}

binder_node::~binder_node()
{
	DPRINTF(5,("binder_node %p: ~binder_node()\n", this));
	if (m_team) {
		DPRINTF(5,("binder_node %p: DecRefs ptr=%p\n",this,m_ptr));
		m_team->RemoveLocalMapping(m_ptr);
		m_team->Transact(new binder_transaction(binder_transaction::tfDecRefs,m_ptr));
		m_team->Release(PRIMARY);
	}
}

void 
binder_node::Released()
{
	DPRINTF(5,("binder_node %p: Released() ptr=%p\n",this,m_ptr));
	if (m_team) {
		DPRINTF(5,("m_secondaryRefs=%ld\n",m_secondaryRefs));
		m_team->Transact(new binder_transaction(binder_transaction::tfRelease,m_ptr));
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

void 
binder_node::Send(binder_transaction *t)
{
	m_team->Transact(t);
}

bool 
binder_node::AttemptAcquire(int32 type, const void* id)
{
	(void)id;
	int32 current;
	switch (type) {
		case PRIMARY:
			current = m_primaryRefs;
			while (current > 0 && !cmpxchg32(&m_primaryRefs, &current, current+1));
			if (current <= 0) {
				DPRINTF(5,("binder_node %p: attempt acquire PRIMARY failed with %ld for %p; id %p\n",
							this, current, m_ptr, id));
				return false;
			}
			DPRINTF(10,("binder_node %p: attempt acquire PRIMARY succeeded (was %ld); id %p\n",
						this, current, id));
			atomic_add(&m_secondaryRefs,1);
			break;
		case SECONDARY:
			current = m_secondaryRefs;
			while (current > 0 && !cmpxchg32(&m_secondaryRefs, &current, current+1));
			if (current <= 0) {
				DPRINTF(5,("binder_node %p: attempt acquire SECONDARY failed with %ld for %p; id %p\n",
							this, current, m_ptr, id));
				return false;
			}
			DPRINTF(10,("binder_node %p: attempt acquire SECONDARY succeeded (was %ld); id %p\n",
						this, current, id));
			break;
	}
	return true;
}

void 
binder_node::Acquire(int32 type, const void* id)
{
	(void)id;
	int32 old;
	switch (type) {
		case PRIMARY:
			old = atomic_add(&m_primaryRefs,1);
			DPRINTF(10,("binder_node %p: primary ref acquired (was %ld) id %p\n",this,old,id));
		case SECONDARY:
			old = atomic_add(&m_secondaryRefs,1);
			DPRINTF(10,("binder_node %p: secondary ref acquired (was %ld) id %p\n",this,old,id));
		default: break;
	}
}

void 
binder_node::Release(int32 type, const void* id)
{
	(void)id;
	int32 old;
	switch (type) {
		case PRIMARY:
			old = atomic_add(&m_primaryRefs,-1);
			DPRINTF(10,("binder_node %p: primary ref released id (was %ld) id %p\n",this,old,id));
			if (old == 1) Released();
		case SECONDARY:
			old = atomic_add(&m_secondaryRefs,-1);
			if (old == 1) delete this;
			DPRINTF(10,("binder_node %p: secondary ref released (was %ld) id %p\n",this,old,id));
		default: break;
	}
}
