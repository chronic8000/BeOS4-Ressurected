
#ifndef BINDER2_NODE_H
#define BINDER2_NODE_H

#include "binder_defs.h"

class binder_node
{
	public:

									binder_node(binder_team *team, void *ptr);
									~binder_node();
									
				void				Released();

				/*	Super-special AttemptAcquire() that also lets you attempt
					to acquire a secondary ref.  But note that binder_team is
					the ONLY one who can attempt a secondary, ONLY while holding
					its lock, for the simple reason that binder_node's destructor
					unregisters itself from the team.  In other words, it's a
					dihrty hawck.
				*/
				bool				AttemptAcquire(int32 type, const void* id);
				
				void				Acquire(int32 type, const void* id);
				void				Release(int32 type, const void* id);

				/*	Send a transaction to this node. */
				void				Send(binder_transaction *t);
				
				void *				Ptr();
				binder_team *		Home();

	private:
				int32				m_primaryRefs;
				int32				m_secondaryRefs;
				void *				m_ptr;
				binder_team *		m_team;
};

#endif // BINDER2_NODE_H
