
#include "binder_transaction.h"
#include "binder_team.h"
#include "binder_thread.h"
#include "binder_node.h"
#include "binder_driver.h"

#include <support2/TypeConstants.h>

enum {
	B_BINDER_NODE_TYPE		= '_BND'
};

using namespace B::Support2;

#if TRACK_TRANSACTIONS
static int32 g_numTransactions = 0;
static lock g_trackLock = { -1, 0 };
static binder_transaction* g_activeChain = NULL;

static void add_to_active(binder_transaction* who)
{
	if (g_trackLock.s < B_OK) new_lock(&g_trackLock, "transaction tracking lock");
	LOCK(g_trackLock);
	who->chain = g_activeChain;
	g_activeChain = who;
	UNLOCK(g_trackLock);
}

static void remove_from_active(binder_transaction* who)
{
	LOCK(g_trackLock);
	
	binder_transaction** pos = &g_activeChain;
	while (*pos && *pos != who) pos = &(*pos)->chain;
	if (*pos) *pos = (*pos)->chain;
	
	pos = &g_activeChain;
	if (*pos) {
		DPRINTF(5,("Remaining active transactions:\n"));
		while (*pos) {
			DPRINTF(5,("\tBuffer %p (addr %p)\n", (*pos)->Data(), *pos));
			pos = &(*pos)->chain;
		}
	} else {
		DPRINTF(5,("No more active transactions!\n"));
	}
	
	UNLOCK(g_trackLock);
}
#endif

status_t 
binder_transaction::ConvertToNodes(binder_team *from, iobuffer *io)
{
	if (refFlags()) return B_OK;

	if (team != from) {
		from->Acquire(SECONDARY);
		if (team) team->Release(SECONDARY);
		team = from;
	}

	if (offsets_size > 0) {
		// This function is called before any references have been acquired.
		if ((flags&tfPrivReferenced)) debugger ("Uh-oh!");
		flags |= tfPrivReferenced;
	
		uint8 *ptr = Data();
		const size_t *off = (const size_t*)(ptr + data_size);
		const size_t *offEnd = off + (offsets_size/sizeof(size_t));
		uint32 *type;
		binder_node **object;
		while (off < offEnd) {
			type = (uint32*)(ptr + *off++);
			object = (binder_node**)(ptr + *off++);
			if (*type == B_BINDER_HANDLE_TYPE)
				// Retrieve node and acquire reference.
				*object = from->Descriptor2Node(*((int32*)object),this);
			else if (*type == B_BINDER_TYPE) {
				DPRINTF(5,("ConvertToNodes B_BINDER_TYPE %p\n",*((void**)object)));
				// Lookup node and acquire reference.
				if (from->Ptr2Node(*((void**)object),object,io,this) != B_OK) return B_ERROR;
			} else {
				debugger("Bad binder offset given to transaction!");
			}
			*type = B_BINDER_NODE_TYPE;
		}
	}
	
	return B_OK;
}

status_t 
binder_transaction::ConvertFromNodes(binder_team *to)
{
	if (refFlags()) return B_OK;

	if (team != to) {
		to->Acquire(SECONDARY);
		if (team) team->Release(SECONDARY);
		team = to;
	}

	if (offsets_size > 0) {
		// This function is called after references have been acquired.
		if (!(flags&tfPrivReferenced)) debugger ("Uh-oh!");
	
		uint8 *ptr = Data();
		const size_t *off = (const size_t*)(ptr + data_size);
		const size_t *offEnd = off + (offsets_size/sizeof(size_t));
		uint32 *type;
		binder_node **object;
		while (off < offEnd) {
			type = (uint32*)(ptr + *off++);
			object = (binder_node**)(ptr + *off++);
			if (*type == B_BINDER_NODE_TYPE) {
				binder_node *n = *object;
				if (!n) {
					*type = B_BINDER_HANDLE_TYPE;
					*((int32*)object) = -1;
				} else if (n->Home() == to) {
					*type = B_BINDER_TYPE;
					*((void**)object) = n->Ptr();
					// Keep a reference on the node so that it doesn't
					// go away until this transaction completes.
				} else {
					*type = B_BINDER_HANDLE_TYPE;
					*((int32*)object) = to->Node2Descriptor(n);
					// We now have a reference on the node through the
					// target team's descriptor, so remove our own ref.
					n->Release(PRIMARY, this);
				}
			} else {
				debugger("Bad binder offset given to transaction!");
			}
		}
	}

	return B_OK;
}

void
binder_transaction::DetachTarget()
{
	if (sender) {
		sender->Release(SECONDARY);
		sender = NULL;
	}
	if (receiver) {
		receiver->Release(SECONDARY);
		receiver = NULL;
	}

	if (target) {
		target->Release(refFlags() == tfAttemptAcquire ? SECONDARY : PRIMARY,this);
		target = NULL;
	}
}

void *
binder_transaction::operator new(size_t size, int32 dataSize, int32 offsetsSize)
{
	return shared_malloc(size+dataSize+offsetsSize-1);
}

void 
binder_transaction::operator delete(void *p)
{
	shared_free(p);
}

void binder_transaction::Init()
{
	next = NULL;
	offsets_size = 0;
	data_size = 0;
	sender = NULL;
	receiver = NULL;
	target = NULL;
	team = NULL;
	flags = 0;
	priority = 10;
	code = 0;
}

binder_transaction::binder_transaction(uint32 _code, size_t _dataSize, const void *_data,
	size_t _offsetsSize, const void *_offsetsData)
{
#if TRACK_TRANSACTIONS
	DPRINTF(5,("creating complex buffer %p (addr %p); now have %ld outstanding\n",
			Data(), this, atomic_add(&g_numTransactions,1)+1));
	add_to_active(this);
#endif
	Init();
	code = _code;
	data_size = _dataSize;
	offsets_size = _offsetsSize;
	if (_dataSize) memcpy(Data(),_data,_dataSize);
	if (_offsetsSize) memcpy(Data()+_dataSize,_offsetsData,_offsetsSize);
}

binder_transaction::binder_transaction(uint16 refFlags, void *ptr)
{
#if TRACK_TRANSACTIONS
	DPRINTF(5,("creating simple buffer %p (addr %p); now have %ld outstanding\n",
			Data(), this, atomic_add(&g_numTransactions,1)+1));
	add_to_active(this);
#endif
	Init();
	if ((refFlags&(~tfRefTransaction)) != 0) debugger("Stop that!");
	flags |= refFlags;
	*((void**)Data()) = ptr;
}

binder_transaction::~binder_transaction()
{
#if TRACK_TRANSACTIONS
	DPRINTF(5,("freeing buffer %p (addr %p); now have %ld outstanding\n",
			Data(), this, atomic_add(&g_numTransactions,-1)-1));
	remove_from_active(this);
#endif

	binder_team *owner = NULL;
	if (offsets_size > 0) {
		if (!(flags&tfPrivReferenced)) debugger("Uh-oh!");
		if (team && team->AttemptAcquire()) owner = team;
		if (!owner) debugger("No owner!");
		uint8 *ptr = Data();
		const size_t *off = (const size_t*)(ptr + data_size);
		const size_t *offEnd = off + (offsets_size/sizeof(size_t));
		uint32 *type;
		binder_node **object;
		while (off < offEnd) {
			type = (uint32*)(ptr + *off++);
			object = (binder_node**)(ptr + *off++);
			if (*type == B_BINDER_HANDLE_TYPE)
				DPRINTF(5,("Delete binder_transaction B_BINDER_HANDLE_TYPE %ld\n",*((int32*)object)));
				if (owner) {
					owner->UnrefDescriptor(*((int32*)object), PRIMARY);
				}
			else if (*type == B_BINDER_TYPE) {
				if (owner) {
					binder_node *n;
					if (owner->Ptr2Node(*object,&n,NULL,this) == B_OK) {
						n->Release(PRIMARY,this); // once for the grab we just did
						n->Release(PRIMARY,this); // and once for the reference this transaction holds
					} else {
						debugger("Can't find node!");
					}
				}
			} else if (*type == B_BINDER_NODE_TYPE) {
				(*object)->Release(PRIMARY,this);
			}
		}
	}
	
	if (owner) owner->Release(PRIMARY);
	if (team) team->Release(SECONDARY);
	
	DetachTarget();
}
