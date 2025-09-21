/*	MNodeManager.h	*/

#if !defined(M_NODE_MANAGER_H)
#define M_NODE_MANAGER_H

#include <map>
#include <set>
#include <stdlib.h>

#include "trinity_p.h"
#include "MSavable.h"
#include "str.h"


class MBufferManager;

class MNodeManager :
	public MSavable
{
public:
		MNodeManager();
		~MNodeManager();

		status_t SaveState();
		status_t LoadState();

		status_t RegisterNode(
				BMessenger messenger,
				media_node & in_node,
				const char * node_name,
				media_node_id * out_id,
				const char * path,
				int32 id,
				const char * flavor_name,
				media_addon_id addon,
				media_type producer_type,
				media_type consumer_type);

		status_t UnregisterNode(
				media_node_id node);
		status_t GetNodes(
				BMessage & output,
				const char * name = NULL);
		status_t GetLiveNode(
				BMessage & output,
				const char * attr_name,
				media_node_id node);
		status_t GetLiveNodes(
				BMessage & output,
				const char * attr_name,
				const media_format * input_format, 
				const media_format * output_format,
				const char * node_name,
				uint32 node_kinds);
		status_t GetNodeInputs(
				media_node_id node,
				BMessage & output,
				const char * name = NULL);
		status_t GetNodeOutputs(
				media_node_id node,
				BMessage & output,
				const char * name = NULL);
		status_t FindNode(
				media_node_id node,
				media_node & output);
		status_t FindSaveInfo(
				media_node_id node,
				const char ** out_string,
				int32 * out_id,
				media_addon_id * addon,
				const char ** out_flavor_name);
		status_t FindDormantNodeFor(
				media_node_id node,
				dormant_node_info * output);

#if 0
		status_t FindNodeFor(
				const char * path,
				int32 id,
				media_node & output);
		status_t FindNodeFor(
				media_addon_id addon,
				int32 id,
				media_node & output);
#endif
		status_t FindNodesFor(
				media_addon_id addon,
				int32 id,
				BMessage & out_message,
				const char * msg_name);
		status_t FindNodesForPort(
				port_id port,
				BMessage & out_message,
				const char * msg_name);
//		int32 CountInstances(
//				const char * path, 
//				int32 id);
//		status_t FindNodeAddOnInfo(
//				media_node_id node,
//				const char ** out_string,
//				int32 * out_id,
//				BMediaAddOn ** addon,
//				BMediaNode ** instance);

		void AddInput(
				media_node_id node,
				media_destination input);
		void RemoveInput(
				media_node_id node,
				media_destination input);
		void AddOutput(
				media_node_id node,
				media_source output);
		void RemoveOutput(
				media_node_id node,
				media_source output);

		void UnregisterTeamNodes(
				team_id team,
				BMessage & output,
				const char * name,
				int32 *nodeCount,
				MBufferManager * bufferManager);
		status_t IncrementGlobalRefCount(
				media_node_id node);
		status_t DecrementGlobalRefCount(
				media_node_id node,
				BMessage *original_request);

		status_t DumpGlobalReferences(
				BMessage & into,
				const char * name);

		status_t BroadcastMessage(
				int32 code,
				void *data,
				int32 dataSize,
				bigtime_t timeout);
				
inline	void	Lock();
inline	void	Unlock();

private:

		struct store_type{
			str_ref node_name;				/* name Node was created with */
			BMessenger roster;				/* Roster for node */
			media_node node;				/* Node information */
			str_ref path;					/* path for node add-on */
			int32 id;						/* flavor ID within add-on */
			str_ref flavor_name;			/* name of flavor within add-on */
			media_addon_id addon;			/* addon ID of the node's add-on (if any) */
			media_type producer_type;		/* registered type for producers */
			media_type consumer_type;		/* registered type for consumers */
//			set<media_source> outputs;		/* Outputs */
//			set<media_destination> inputs;	/* Inputs */
			int32 global_ref_count;			/* refers to the number of applications */
											/* that have references to this node */
											/* note that an application may reference */
											/* a node multiple times, but the application */
											/* will only be accounted for once in this */
											/* number */
											
													
			store_type()
				{
				}
			~store_type()
				{
				}
			store_type(
					const char * n,
					const BMessenger & a, 
					const media_node & b,
					const char * p,
					int32 i,
					const char * fn,
					media_addon_id ao,
					media_type pt,
					media_type ct) :
					node_name(n),
					roster(a),
					node(b),
					path(p),
					id(i),
					flavor_name(fn),
					addon(ao),
					producer_type(pt),
					consumer_type(ct)
				{
					global_ref_count = 0;
				}
			store_type(
					const store_type & init)
				{
					clone(init);
				}
			store_type & operator=(
					const store_type & init)
				{
					clone(init);
					return *this;
				}
			void clone(
					const store_type & init)
				{
					node_name = init.node_name;
					roster = init.roster;
					node = init.node;
//					outputs = init.outputs;
//					inputs = init.inputs;
					path = init.path;
					id = init.id;
					flavor_name = init.flavor_name;
					addon = init.addon;
					global_ref_count = init.global_ref_count;
					producer_type = init.producer_type;
					consumer_type = init.consumer_type;
				}
		};
		std::map<media_node_id, store_type> mNodes;

		int32 mNextID;
		BLocker mLock;
};


void MNodeManager::Lock()
{
	mLock.Lock();
};

void MNodeManager::Unlock()
{
	mLock.Unlock();
};

#endif /* M_NODE_MANAGER_H */
