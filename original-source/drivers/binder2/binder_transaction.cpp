
#include "binder_transaction.h"
#include "binder_team.h"
#include "binder_thread.h"
#include "binder_node.h"
#include "binder_driver.h"
#include "ValueTransformer.h"

using namespace B::Support2;

struct transform_state {
	binder_team *team;
	iobuffer *io;
};

static status_t binder_refs_2nodes(void *userData, type_code* type, uint32* value)
{
	transform_state *st = (transform_state*)userData;

	if (*type == B_BINDER_HANDLE_TYPE)
		*((binder_node**)value) = st->team->Descriptor2Node(*((int32*)value));
	else if (*type == B_BINDER_TYPE) {
		DPRINTF(5,("B_BINDER_TYPE %p\n",*((void**)value)));
		if (st->team->Ptr2Node(*((void**)value),((binder_node**)value),st->io) != B_OK) return B_ERROR;
	}
	*type = B_BINDER_NODE_TYPE;
	return B_OK;
}

static status_t binder_nodes_2refs(void *userData, type_code* type, uint32* value)
{
	transform_state *st = (transform_state*)userData;

	if (*type == B_BINDER_NODE_TYPE) {
		binder_node *n = *((binder_node**)value);
		if (!n) {
			*type = B_BINDER_HANDLE_TYPE;
			*((int32*)value) = -1;
		} else if (n->Home() == st->team) {
			*type = B_BINDER_TYPE;
			*((void**)value) = n->Ptr();
		} else {
			*type = B_BINDER_HANDLE_TYPE;
			*((int32*)value) = st->team->Node2Descriptor(n);
			n->Release(PRIMARY);
		}
	}
	return B_OK;
}

static status_t free_binder_refs(void *userData, type_code* type, uint32* value)
{
	transform_state *st = (transform_state*)userData;

	if (*type == B_BINDER_HANDLE_TYPE)
		DPRINTF(5,("B_BINDER_HANDLE_TYPE %ld\n",*((int32*)value)));
		if (st->team) {
			st->team->UnrefDescriptor(*((int32*)value));
		}
	else if (*type == B_BINDER_TYPE) {
		if (st->team) {
			binder_node *n;
			if (st->team->Ptr2Node(*((void**)value),&n,NULL) == B_OK) {
				n->Release(PRIMARY); // once for the grab we just did
				n->Release(PRIMARY); // and once for the reference this transaction holds
			}
		}
	} else if (*type == B_BINDER_NODE_TYPE) {
		(*((binder_node**)value))->Release(PRIMARY);
	}
	return B_OK;
}

static ssize_t stub_write_transformed_data(const void* , size_t length)
{
	return length;
}

status_t 
binder_transaction::ConvertToNodes(binder_team *from, iobuffer *io)
{
	if (refFlags()) return B_OK;

	if (team != from) {
		from->Acquire(SECONDARY);
		if (team) team->Release(SECONDARY);
		team = from;
	}

	int32 nv = numValues;
	uint8 *ptr = (uint8*)data;
	ssize_t tmp,used = 0;

	transform_state state;
	state.team = from;
	state.io = io;
	value_transformer xform(&state,binder_refs_2nodes,stub_write_transformed_data);
	while (nv--) {
		tmp = xform.transform(ptr,size-used);
		if (tmp < 0) return tmp;
		used += tmp;
		ptr += tmp;
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

	int32 nv = numValues;
	uint8 *ptr = (uint8*)data;
	ssize_t tmp,used = 0;

	transform_state state;
	state.team = to;
	state.io = NULL;
	value_transformer xform(&state,binder_nodes_2refs,stub_write_transformed_data);
	while (nv--) {
		tmp = xform.transform(ptr,size-used);
		if (tmp < 0) return tmp;
		used += tmp;
		ptr += tmp;
	}

	return B_OK;
}

void *
binder_transaction::operator new(size_t size, int32 dataSize)
{
	return shared_malloc(size+dataSize-1);
}

void 
binder_transaction::operator delete(void *p)
{
	shared_free(p);
}

void binder_transaction::Init()
{
	next = NULL;
	size = 0;
	numValues = 0;
	sender = NULL;
	receiver = NULL;
	target = NULL;
	team = NULL;
	flags = 0;
}

binder_transaction::binder_transaction(int32 _numValues, int32 _size, void *_data)
{
	Init();
	size = _size;
	numValues = _numValues;
	if (_size) memcpy(data,_data,_size);
}

binder_transaction::binder_transaction(int32 unrefType, void *ptr)
{
	Init();
	if (unrefType == SECONDARY)
		flags |= tfDecRefs;
	else if (unrefType == PRIMARY)
		flags |= tfRelease;
	*((void**)data) = ptr;
}

binder_transaction::~binder_transaction()
{
	DPRINTF(5,("freeing buffer %p\n",this));

	int32 nv = numValues;
	uint8 *ptr = (uint8*)data;
	ssize_t tmp,used = 0;

	transform_state state;
	if (team && team->AttemptAcquire())	state.team = team;
	else {
		DPRINTF(5,("team=%p\n",team));
		state.team = NULL;
	}
	state.io = NULL;
	value_transformer xform(&state,free_binder_refs,stub_write_transformed_data);

	while (nv--) {
		tmp = xform.transform(ptr,size-used);
		if (tmp < 0) break;
		used += tmp;
		ptr += tmp;
	}

	if (sender) sender->Release(SECONDARY);
	if (receiver) receiver->Release(SECONDARY);

	if (state.team) state.team->Release(PRIMARY);
	if (team) team->Release(SECONDARY);
	
	if (target) target->Release(PRIMARY);
}
