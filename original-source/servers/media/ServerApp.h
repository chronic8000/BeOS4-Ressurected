/*	_ServerApp.h	*/

#if !defined(ServerApp_h)
#define ServerApp_h

#include <Application.h>
#include "trinity_p.h"
#include "debug.h"

#include <map.h>
#include <string.h>

class MAppManager;
class MNodeManager;
class MBufferManager;
class MDefaultManager;
class MNotifierManager;
class MMediaFilesManager;
class MFormatRegistry;
#if KEY_SERVER
class KeyServer;
#endif
struct flavor_info;

class _ServerApp :
	public BApplication
{
public:
		_ServerApp();
		~_ServerApp();
virtual		void MessageReceived(
				BMessage * message);
virtual		bool QuitRequested();
virtual		void AboutRequested();
virtual		void ArgvReceived(
				int32 argc,
				char ** argv);
virtual		void ReadyToRun();

		bool IsQuitMode() const;

static	void AddPurgableBufferGroup(
				team_id team,
				area_id orig,
				area_id clone,
				void * addr);
static	void CancelPurgableBufferGroupCleanup(
				area_id clone);

static	status_t DirtyWorkLaunch(void *);

private:

		MAppManager * mAppManager;
		MNodeManager * mNodeManager;
		MBufferManager * mBufferManager;
		MDefaultManager * mDefaultManager;
		MNotifierManager * mNotifierManager;
		MMediaFilesManager * mMediaFilesManager;
		MFormatRegistry * mFormatRegistry;
		BMessenger mAddOnHost;

		struct purgable_buffer_group {
			team_id team;
			area_id	orig;
			area_id clone;
			void * addr;
		};
static	std::list<purgable_buffer_group> sPurgableGroups;
static	BLocker sPurgableGroupLock;

		void MRegisterApp(
				BMessage * message);
		void MUnregisterApp(
				BMessage * message);
		void MRegisterNode(
				BMessage * message);
		void MUnregisterNode(
				BMessage * message);
		void MRegisterBuffer(
				BMessage * message);
		void MReRegisterBuffer(
				BMessage * message);
		void MUnregisterBuffer(
				BMessage * message);
		void MSetDefault(
				BMessage * message);
		void MAcquireNodeReference(
				BMessage * message);
		void MReleaseNodeReference(
			BMessage * message);
		void MRequestNotifications(
				BMessage * message);
		void MCancelNotifications(
				BMessage * message);
		void MSetOutputBuffers(
				BMessage * message);
		void MReclaimOutputBuffers(
				BMessage * message);
		void MOrphanReclaimableBuffers(
				BMessage * message);
		void MSetTimeSource(
				BMessage * message);
#if 0
		void MMakeConnection(
				BMessage * message);
		void MBreakConnection(
				BMessage * message);
#endif
		void MQueryNodes(
				BMessage * message);
#if 0
		void MQueryInputs(
				BMessage * message);
		void MQueryOutputs(
				BMessage * message);
#endif
		void MQueryLatents(
				BMessage * message);
		void MGetDormantNode(
				BMessage * message);
		void MSniffFile(
				BMessage * message);
		void MFormatChanged(
				BMessage * message);
		void MInstantiatePersistentNode(
				BMessage * message);
		void MGetDefaultInfo(
				BMessage * message);
		void MGetRunningDefault(
				BMessage * message);
		void MSetRunningDefault(
				BMessage * message);
		void MSurrenderNode(
				BMessage * message);
		void MBroadcastMessage(
				BMessage * message);
		void MTypeItemOp(
				BMessage * message);
		void MFormatOp(
				BMessage * message);
		void MGetDormantFlavor(
				BMessage * message);
		void MGetDormantFileFormats(
				BMessage * message);
		void MGetRealtimeFlags(
				BMessage * message);
		void MSetRealtimeFlags(
				BMessage * message);
		void MGetLatentInfo(
				BMessage * message);
		void MBufferGroupReg(
				BMessage * message);
		void MFindRunningInstances(
				BMessage * message);
		void MGetNodeID(
				BMessage * message);

		status_t BroadcastCurrentStateTo(
				BMessenger & msgr);


		status_t InvokeNode(
				BMessage & message,	/* path, id, config */
				media_node & out,
				bool force_new = false);
		status_t InvokeNode(
				const char * path,
				int32 id,
				BMessage & message,	/* config */
				media_node & out);
		status_t MakeNode(
				BMediaAddOn * addon,
				flavor_info & info,
				BMessage & message,
				BMediaNode ** out_node);
		void SetQuitMode(
				bool quitting);

		status_t DirtyWork();
		void CleanupPurgedBufferGroup(
				const purgable_buffer_group & grp,
				bool wasOwner);
		status_t _UnregisterApp(team_id team, bool spawnThread);
static	status_t _DoNotify(struct notify_data *data);

		BMessenger & AddOnHost();

		BWindow * m_about;
		bool m_quitting;

#if KEY_SERVER
		KeyServer * m_keyServer;
#endif

		BMessageRunner * mIdleRunner;
};


#if KEY_SERVER

class KeyServer
{
public:

		KeyServer();
		~KeyServer();

		status_t Start(
				const char * thread_name = NULL, 
				int32 prio = 10);
		status_t Stop(
				bool final = true);	//	false if you want to re-start again

private:

static	status_t EnterServer(void * server);
		status_t Server();

		struct key_id {
			media_key_namespace name;
			media_key_type key;
			inline bool operator<(const key_id & other) const
				{
					int s = strcmp(name, other.name);
					if (s < 0) return true;
					if (s > 0) return false;
					return key < other.key;
				}
			inline bool operator==(const key_id & other) const
				{
					return (key == other.key) && !strcmp(name, other.name);
				}
		};
		struct key_value {
			int32 ref_count;
			size_t size;
			type_code type;
		};
		struct key_value_ref {
			key_value * value;

			inline key_value_ref()
				{
					value = NULL;
				}
			inline key_value_ref(const key_value & init)
				{
					value = (key_value *)malloc(sizeof(key_value)+init.size);
					if (value) {
						*value = init;
						value->ref_count = 1;
					}
				}
			inline key_value_ref(const key_value_ref & other)
				{
					value = other.value;
					if (value != NULL) {
						atomic_add(&value->ref_count, 1);
					}
				}
			inline ~key_value_ref()
				{
					if ((value != 0) && (atomic_add(&value->ref_count, -1) == 1)) {
						free(value);
					}
				}
			inline key_value_ref & operator=(const key_value_ref & other)
				{
					if ((value != 0) && (atomic_add(&value->ref_count, -1) == 1)) {
						free(value);
					}
					value = other.value;
					if (value != NULL) {
						atomic_add(&value->ref_count, 1);
					}
					return *this;
				}
			inline void * data()
				{
					if (!value) return 0;
					return &value[1];
				}
			inline size_t size()
				{
					if (!value) return 0;
					return value->size;
				}
			inline type_code type()
				{
					if (!value) return 0;
					return value->type;
				}
		};
		struct pending_key {
			key_id id;
			port_id client;
		};
		typedef std::map<key_id, key_value_ref> key_map;
		key_map m_keys;
		typedef std::map<int32, pair<pending_key, key_value_ref> > pending_map;
		pending_map m_pending;

		thread_id m_thread;
		port_id m_port;
		int32 m_nextKeyCode;

};

#endif	//	KEY_SERVER

#endif	//	ServerApp_h

