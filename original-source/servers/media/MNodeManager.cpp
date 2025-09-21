/*	MNodeManager.cpp	*/


#include <string.h>

#include <MediaAddOn.h>
#include <Autolock.h>
#include <MediaDefs.h>
#include "MNodeManager.h"
#include "MBufferManager.h"


extern void debug(int detail, const char * fmt, ...);


MNodeManager::MNodeManager() :
	MSavable("MNodeManager", 0),
	mLock("MNodeManager::mLock")
{
	mNextID = 1;
}


MNodeManager::~MNodeManager()
{
}


status_t
MNodeManager::SaveState()
{
	return B_OK;
}


status_t
MNodeManager::LoadState()
{
	return B_OK;
}


status_t
MNodeManager::RegisterNode(
	BMessenger messenger,
	media_node & info, 
	const char * node_name,
	media_node_id * node,
	const char * path,
	int32 id,
	const char * flavor_name,
	media_addon_id addon,
	media_type producer_type,
	media_type consumer_type)
{
	BAutolock l(mLock);
	*node = atomic_add(&mNextID, 1);
	info.node = *node;
	debug(2, "Registering node %d, producer type %x, consumer type %x\n", 
		info.node, producer_type, consumer_type);
	mNodes[*node] = store_type(node_name, messenger, info, path, id,
		flavor_name, addon, producer_type, consumer_type);
	return B_OK;
}


status_t
MNodeManager::UnregisterNode(
	media_node_id node)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr(mNodes.find(node));
	if (ptr == mNodes.end()) return B_ERROR;
	/* delete node from path map if it's likely that it will be there */
	mNodes.erase(node);
	return B_OK;
}


status_t
MNodeManager::GetNodes(
	BMessage & output,
	const char * name)
{
	BAutolock l(mLock);
	if (name == NULL) {
		name = "media_node_id";
	}
	status_t err = B_OK;
	std::map<media_node_id, store_type>::iterator ptr = mNodes.begin();
	while (ptr != mNodes.end()) {
		status_t temp = output.AddInt32(name, (*ptr).first);
		if (temp != B_OK) {
			err = temp;
		}
		ptr++;
	}
	return err;
}


status_t
MNodeManager::GetLiveNode(
	BMessage & output,
	const char * attr_name,
	media_node_id node)
{
	BAutolock l(mLock);
	if (attr_name == NULL) {
		attr_name = "live_nodes";
	}
	status_t err = B_OK;
	std::map<media_node_id, store_type>::iterator ptr(mNodes.find(node));
	if (ptr == mNodes.end()) return B_MEDIA_BAD_NODE;
	BPoint p[4] = { BPoint(320,20), BPoint(20,20), BPoint(220,20), BPoint(120,30) };
	live_node_info live;
	live.node = (*ptr).second.node;
	int k = live.node.kind & 3;
	live.hint_point = p[k];
	p[k].y += 50;
	strncpy(live.name, (*ptr).second.node_name.str(), sizeof(live.name));
	live.name[sizeof(live.name)-1] = 0;
	err = output.AddData(attr_name, B_RAW_TYPE, &live, sizeof(live));
	return err;
}

status_t
MNodeManager::GetLiveNodes(
	BMessage & output,
	const char * attr_name,
	const media_format * input_format,
	const media_format * output_format,
	const char * node_name,
	uint32 node_kinds)
{
	BAutolock l(mLock);
	if (attr_name == NULL) {
		attr_name = "live_nodes";
	}
	status_t err = B_OK;
	std::map<media_node_id, store_type>::iterator ptr = mNodes.begin();
	BPoint p[4] = { BPoint(320,20), BPoint(20,20), BPoint(220,20), BPoint(120,30) };
	int nn_len = node_name ? strlen(node_name) : 0;
	bool wild = false;
	if (node_name && nn_len > 0 && node_name[nn_len-1] == '*') {
		wild = true;
		nn_len--;
	}
	debug(2, "find match for: %s %x %x %x\n", node_name ? node_name : "*",
		node_kinds, input_format ? input_format->type : 0, output_format ? output_format->type: 0);
	while (ptr != mNodes.end()) {
//		status_t temp = output.AddInt32(name, (*ptr).first);
		debug(2, "candidate: %s %x %x %x\n", (*ptr).second.node_name.str(), (*ptr).second.node.kind,
			(*ptr).second.consumer_type, (*ptr).second.producer_type);
		if (nn_len > 0) {
			if (wild) {
				if (strncmp(node_name, (*ptr).second.node_name.str(), nn_len)) {
					debug(2, "Wildcard match disqualifies %s\n", (*ptr).second.node_name.str());
					goto do_next;
				}
			}
			else {
				if (strcmp(node_name, (*ptr).second.node_name.str())) {
					debug(2, "Name match disqualifies %s\n", (*ptr).second.node_name.str());
					goto do_next;
				}
			}
		}
		if (node_kinds) {
			if (((*ptr).second.node.kind & node_kinds) != node_kinds) {
				debug(2, "Node kinds disqualify %s\n", (*ptr).second.node_name.str());
				goto do_next;
			}
		}
		if (input_format) {
			/*** currently, we only filter on the format kind, not the actual format parameters... ***/
			if (input_format->type && (*ptr).second.consumer_type) {
				if (input_format->type != (*ptr).second.consumer_type) {
					debug(2, "Input format disqualifies %s (%x != %x)\n",
						(*ptr).second.node_name.str(), input_format->type,
						(*ptr).second.consumer_type);
					goto do_next;
				}
			}
		}
		if (output_format) {
			/*** currently, we only filter on the format kind, not the actual format parameters... ***/
			if (output_format->type && (*ptr).second.producer_type) {
				if (output_format->type != (*ptr).second.producer_type) {
					debug(2, "Output format disqualifies %s (%x != %x)\n",
						(*ptr).second.node_name.str(), output_format->type,
						(*ptr).second.producer_type);
					goto do_next;
				}
			}
		}
		{
			live_node_info live;
			live.node = (*ptr).second.node;
			int k = live.node.kind & 3;
			live.hint_point = p[k];
			p[k].y += 50;
	#if 0
			static const char * s[4] = {
				"Node", "Producer", "Consumer", "Filter"
			};
			sprintf(live.name, "%s %d", s[k], live.node.node);
	#else
			strncpy(live.name, (*ptr).second.node_name.str(), sizeof(live.name));
			live.name[sizeof(live.name)-1] = 0;
	#endif
			debug(2, "Found a match in %s\n", live.name);
			status_t temp = output.AddData(attr_name, B_RAW_TYPE, &live, sizeof(live));
			if (temp != B_OK) {
				err = temp;
			}
		}
do_next:
		ptr++;
	}
	return err;
}


#if 0
/* only gets active outputs */
status_t
MNodeManager::GetNodeOutputs(
	media_node_id node,
	BMessage & output,
	const char * name)
{
	if (name == NULL) {
		name = "node_outputs";
	}
	std::map<media_node_id, store_type>::iterator iter(mNodes.find(node));
	if (iter == mNodes.end()) {
		return B_MEDIA_BAD_NODE;
	}
	set<media_source>::iterator ptr((*iter).second.outputs.begin());
	status_t error = B_OK;
	while (ptr != (*iter).second.outputs.end()) {
		status_t err = output.AddData(name, B_RAW_TYPE, &*ptr, sizeof(media_source));
		if (err < B_OK) {
			error = err;
		}
		ptr++; //dug
	}
	return error;
}
#endif


#if 0
/* only gets active inputs */
status_t
MNodeManager::GetNodeInputs(
	media_node_id node,
	BMessage & output,
	const char * name)
{
	if (name == NULL) {
		name = "node_inputs";
	}
	std::map<media_node_id, store_type>::iterator iter(mNodes.find(node));
	if (iter == mNodes.end()) {
		return B_MEDIA_BAD_NODE;
	}
	set<media_destination>::iterator ptr((*iter).second.inputs.begin());
	status_t error = B_OK;
	while (ptr != (*iter).second.inputs.end()) {
		//dug
		status_t err = output.AddData(name, B_RAW_TYPE, &*ptr, sizeof(media_destination));
		if (err < B_OK) {
			error = err;
		}
		ptr++; //dug
	}
	return error;
}
#endif


status_t
MNodeManager::FindNode(
	media_node_id node,
	media_node & output)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr == mNodes.end()) return B_MEDIA_BAD_NODE;
	output = (*ptr).second.node;
	return B_OK;
}


status_t
MNodeManager::FindSaveInfo(
	media_node_id node,
	const char ** path,
	int32 * id,
	media_addon_id * addon,
	const char ** flavor_name)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr == mNodes.end()) {
		*path = NULL;
		*id = -1;
		return B_MEDIA_BAD_NODE;
	}
	*path = (*ptr).second.path.str();
	*id = (*ptr).second.id;
	*addon = (*ptr).second.addon;
	*flavor_name = (*ptr).second.flavor_name.str();
	return B_OK;
}


status_t
MNodeManager::FindDormantNodeFor(
	media_node_id node,
	dormant_node_info * output)
{
	BAutolock l(mLock);
	ASSERT(output != NULL);
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr == mNodes.end()) return B_MEDIA_BAD_NODE;
	if ((*ptr).second.addon <= 0) return B_MEDIA_ADDON_FAILED;
	strncpy(output->name, (*ptr).second.node_name.str(), sizeof(output->name));
	output->addon = (*ptr).second.addon;
	output->flavor_id = (*ptr).second.id;
	return B_OK;
}


#if 0
status_t
MNodeManager::FindNodeFor(
	const char * path,
	int32 id,
	media_node & output)
{
	BAutolock l(mLock);
	/*** TODO: Should have cache of resolved nodes, which would be updated in SetDefault() and UnregisterNode() */
	/*** TODO: might want a secondary index later ***/
	for (std::map<media_node_id, store_type>::iterator ptr(mNodes.begin()); 
		ptr != mNodes.end(); ptr++) {
		if (((*ptr).second.id == id) && ((*ptr).second.path == path)) {
			/* what if there is more than one instance? */
			output = (*ptr).second.node;
			return B_OK;
		}
	}
	return B_MEDIA_BAD_NODE;
}


status_t
MNodeManager::FindNodeFor(
	media_addon_id addon,
	int32 id,
	media_node & output)
{
	BAutolock l(mLock);
	/*** TODO: Should have cache of resolved nodes, which would be updated in SetDefault() and UnregisterNode() ***/
	/*** TODO: might want a secondary index later ***/
	for (std::map<media_node_id, store_type>::iterator ptr(mNodes.begin()); 
		ptr != mNodes.end(); ptr++) {
		if (((*ptr).second.id == id) && ((*ptr).second.addon == addon)) {
			/* what if there is more than one instance? */
			output = (*ptr).second.node;
			return B_OK;
		}
	}
	return B_MEDIA_BAD_NODE;
}
#endif


status_t
MNodeManager::FindNodesFor(
	media_addon_id addon,
	int32 id,
	BMessage & out_message,
	const char * msg_name)
{
	status_t err = B_MEDIA_BAD_NODE;
	BAutolock l(mLock);
	for (std::map<media_node_id, store_type>::iterator ptr(mNodes.begin()); 
		ptr != mNodes.end(); ptr++) {
		if (((*ptr).second.id == id) && ((*ptr).second.addon == addon)) {
			out_message.AddInt32(msg_name, (*ptr).second.node.node);
			err = B_OK;
		}
	}
	return err;
}


status_t
MNodeManager::FindNodesForPort(
	port_id port,
	BMessage & out_message,
	const char * msg_name)
{
	status_t err = B_MEDIA_BAD_NODE;
	BAutolock l(mLock);
	for (std::map<media_node_id, store_type>::iterator ptr(mNodes.begin()); 
		ptr != mNodes.end(); ptr++) {
		if ((*ptr).second.node.port == port) {
			out_message.AddInt32(msg_name, (*ptr).second.node.node);
			err = B_OK;
		}
	}
	return err;
}


#if 0
void
MNodeManager::AddInput(
	media_node_id node,
	media_destination input)
{
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr != mNodes.end())
	{
		(*ptr).second.inputs.insert(input);
	}
}


void
MNodeManager::RemoveInput(
	media_node_id node,
	media_destination input)
{
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr != mNodes.end())
	{
		(*ptr).second.inputs.erase(input);
	}
}
#endif


#if 0
void
MNodeManager::AddOutput(
	media_node_id node,
	media_source output)
{
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr != mNodes.end())
	{
		(*ptr).second.outputs.insert(output);
	}
}


void
MNodeManager::RemoveOutput(
	media_node_id node,
	media_source output)
{
	std::map<media_node_id, store_type>::iterator ptr = mNodes.find(node);
	if (ptr != mNodes.end())
	{
		(*ptr).second.outputs.erase(output);
	}
}
#endif


void
MNodeManager::UnregisterTeamNodes(
	team_id team,
	BMessage & output,
	const char * name,
	int32 *nodeCount,
	MBufferManager * bufferManager)
{
	BAutolock l(mLock);
	/*** Unfortunately, we have to do a linear search for nodes belonging to team ***/
//fprintf(stderr, "UnregisterTeamNodes(%d)\n", team);
	int count = 0;
	std::map<media_node_id, store_type>::iterator ptr = mNodes.begin();
	while (ptr != mNodes.end())
	{
//fprintf(stderr, "node %d owner %d team %d\n", (*ptr).second.node.node, (*ptr).second.node.port, (*ptr).second.roster.Team());
		std::map<media_node_id, store_type>::iterator del(ptr);
		ptr++;
		if ((*del).second.roster.Team() == team)
		{
	//	recycle buffers currently held by each of these Nodes
			if ((*del).second.node.port > 0) {
				bufferManager->RecycleBuffersWithOwner((*del).second.node.port, team);
			}
			output.AddInt32(name ? name : "media_node_id", (*del).second.node.node);
			mNodes.erase(del);
			count++;
		}
	}
	if (nodeCount) *nodeCount = count;
	debug(2, "UnregisterTeamNodes deleted %d\n", count);
}


status_t
MNodeManager::IncrementGlobalRefCount(
	media_node_id node)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr(mNodes.find(node));
	if (ptr == mNodes.end()) {
		return B_MEDIA_BAD_NODE;
	}
	
	int32 oldCount = atomic_add(&(*ptr).second.global_ref_count, 1);
	debug(1, "Increment global ref count for node %s(id %d) to %d\n", (*ptr).second.node_name.str(),
		node, oldCount + 1);
	return B_OK;
}


status_t
MNodeManager::DumpGlobalReferences(
	BMessage & into,
	const char * name)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr(mNodes.begin());
	into.AddString(name, "Node\tOwner\tRefs\tName\n");
	while (ptr != mNodes.end()) {
		char line[200];
		sprintf(line, "%d\t%d\t%d\t%s\n", (*ptr).first, (*ptr).second.roster.Team(),
				(*ptr).second.global_ref_count, (*ptr).second.node_name.str());
		into.AddString(name, line);
		ptr++;
	}
	return B_OK;
}

status_t
MNodeManager::DecrementGlobalRefCount(
	media_node_id node,
	BMessage *original_request)
{
	BAutolock l(mLock);
	std::map<media_node_id, store_type>::iterator ptr(mNodes.find(node));
	if (ptr == mNodes.end()) return B_MEDIA_BAD_NODE;
	int32 count = atomic_add(&(*ptr).second.global_ref_count, -1);
	debug(1, "Decrement global ref count for node %s(id %d) to %d\n",
		(*ptr).second.node_name.str(), node, count - 1);
	if (count == 1) {
		// Forward the message to the roster that owns the node.
		(*ptr).second.roster.SendMessage(original_request);
	} else {
		// Respond directly to the roster.  Note that we didn't
		// actually delete the node (as it is still in use by other
		// applications), but its reference has been released.
		status_t error = B_OK;
		port_id p;
		if (original_request->FindInt32("be:reply_port", &p) == B_OK)
			write_port_etc(p, MEDIA_REFERENCE_RELEASED, &error, sizeof(error), B_TIMEOUT,
				DEFAULT_TIMEOUT);
		else
			printf("MNodeManager received bogus release node message\n");
	}

	return B_OK;
}

status_t 
MNodeManager::BroadcastMessage(int32 code, void *data, int32 dataSize, bigtime_t timeout)
{
	int32 i=0;
	port_id *localList = (port_id*)malloc(sizeof(port_id)*mNodes.size());

	{
		BAutolock l(mLock);
		std::map<media_node_id, store_type>::iterator ptr = mNodes.begin();
		while (ptr != mNodes.end()) {
			localList[i++] = (*ptr).second.node.port;
			ptr++;
		}
	}

	/* Do the write_port outside the lock so we don't block anyone */
	for (int32 j=0;j<i;j++)
		write_port_etc(localList[j],code,data,dataSize,B_TIMEOUT,timeout);
	free(localList);
	
	return B_OK;
}
