
#ifndef BINDER2_NODE_H
#define BINDER2_NODE_H

#include "binder_defs.h"

class binder_node
{
	public:

									binder_node(binder_team *team, void *ptr);
									~binder_node();
									
				void				Released();

				bool				AttemptAcquire();
				void				Acquire(int32 type);
				void				Release(int32 type);

				/*	Prepare to send a transaction.  Returns 'true' if the
					transaction was actually handled now, else 'false'. */
				bool				PreSend(binder_transaction *t);
				/*	Send a transaction to this node.  Set 'did_pre' to true
					if you have already called PreSend() for this transaction. */
				void				Send(binder_transaction *t, bool did_pre);
				
				void *				Ptr();
				binder_team *		Home();

	private:
				int32				m_primaryRefs;
				int32				m_secondaryRefs;
				void *				m_ptr;
				binder_team *		m_team;
};

#endif // BINDER2_NODE_H
