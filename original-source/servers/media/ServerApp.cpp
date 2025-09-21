/*	_ServerApp.cpp	*/

/*	_ServerApp cannot be on the initiating end of synchronous transactions,	*/
/*	else deadlocks will occur. The AddOnHost() can initiate synchronous	*/
/*	transactions to the server, as can any random Node.	*/

#include <Application.h>
#include <Alert.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <View.h>
#include <MediaAddOn.h>
#include <MessageRunner.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <alloca.h>

#include "ServerApp.h"
#include "MAppManager.h"
#include "MNodeManager.h"
#include "MBufferManager.h"
#include "MDefaultManager.h"
#include "MNotifierManager.h"
#include "MMediaFilesManager.h"
#include "MFormatRegistry.h"
#include "HostCmd.h"
#include "TextScrollView.h"
#include "shared_buffer.h"



#define REALLY_QUIT 'rqt!'
#define SYNC_IDLE 'sid!'
#define SYNC_IDLE_INTERVAL 3000000LL


const char * ADDON_HOST_NAME = "media_addon_server";
const char * about_text = "#000000media_server\n©1998-1999 Be Incorporated.\nAll rights reserved.\n"
#if DEBUG
	"#ff0000DEBUG VERSION "
#endif
	"Built " __DATE__ " " __TIME__ 
	"\n\n\n"
	"#000000Relentless persecution by:\n"
	"\n"
	"Stephen Beaulieu\n"
	"Jeff Bush\n"
	"Pavel Cisler\n"
	"Marc Ferguson\n"
	"Dominique Giampaolo\n"
	"Arve Hjonnevag\n"
	"George Hoffman\n"
	"Ficus Kirkpatrick\n"
	"Cyril Meurillon\n"
	"Steven Olson\n"
	"Jean-Baptiste Queru\n"
	"Pierre Raynaud-R.\n"
	"Steve Sakoman\n"
	"Nathan Schrenk\n"
	"Brian Swetland\n"
	"Christopher Tate\n"
	"Victor Tsou\n"
	"Dug Wright\n"
	"Jon Wätte\n"
	"\n"
	"…and a horde of others.\n"
	"\n"
	"#800000Pounds of gratitude!\n"
;


static bool g_about_host;


std::list<_ServerApp::purgable_buffer_group> _ServerApp::sPurgableGroups;
BLocker _ServerApp::sPurgableGroupLock("PurgableGroupLock");


_ServerApp::_ServerApp() :
	BApplication(B_MEDIA_SERVER_SIGNATURE)
{
#if KEY_SERVER
	m_keyServer = new KeyServer;
	m_keyServer->Start();
#endif
	m_about = NULL;
	m_quitting = false;
	debug(2, "_ServerApp::_ServerApp()\n");

	mFormatRegistry = new MFormatRegistry;
	mAppManager = new MAppManager;
	mNodeManager = new MNodeManager;
	mBufferManager = new MBufferManager;
	mDefaultManager = new MDefaultManager;
	mNotifierManager = new MNotifierManager;
	mMediaFilesManager = new MMediaFilesManager;

	mFormatRegistry->LoadState();
	mAppManager->LoadState();
	mNodeManager->LoadState();
	mBufferManager->LoadState();
	mDefaultManager->LoadState();
	mNotifierManager->LoadState();
	mMediaFilesManager->LoadState();

	be_roster->StartWatching(BMessenger(this), B_REQUEST_QUIT);

	mIdleRunner = 0;
}


_ServerApp::~_ServerApp()
{
	delete mIdleRunner;

	debug(2, "_ServerApp::~_ServerApp()\n");
	mAppManager->SaveState();
	mNodeManager->SaveState();
	mBufferManager->SaveState();
	mDefaultManager->SaveState();
	mNotifierManager->SaveState();
	mMediaFilesManager->SaveState();
	mFormatRegistry->SaveState();

	delete mAppManager;
	delete mNodeManager;
	delete mBufferManager;
	delete mDefaultManager;
	delete mNotifierManager;
	delete mMediaFilesManager;
	delete mFormatRegistry;
#if KEY_SERVER
	delete m_keyServer;
#endif
}


void
_ServerApp::ReadyToRun()
{
	BApplication::ReadyToRun();
	set_thread_priority(find_thread(NULL), 20);
	if (!AddOnHost().IsValid()) {
		debug(1, "media_server: Cannot load media add-ons!\n");
	}
}


	static const char *
	msg_what(
		type_code what)
	{
		static struct {
			type_code what;
			const char * str;
		} table[] = {
			{ MEDIA_REGISTER_APP, "MEDIA_REGISTER_APP" },
			{ MEDIA_UNREGISTER_APP, "MEDIA_UNREGISTER_APP" },
			{ MEDIA_REGISTER_NODE, "MEDIA_REGISTER_NODE" },
			{ MEDIA_UNREGISTER_NODE, "MEDIA_UNREGISTER_NODE" },
			{ MEDIA_REGISTER_BUFFER, "MEDIA_REGISTER_BUFFER" },
			{ MEDIA_SET_DEFAULT, "MEDIA_SET_DEFAULT" },
			{ MEDIA_ACQUIRE_NODE_REFERENCE, "MEDIA_ACQUIRE_NODE_REFERENCE" },
			{ MEDIA_REQUEST_NOTIFICATIONS, "MEDIA_REQUEST_NOTIFICATIONS" },
			{ MEDIA_CANCEL_NOTIFICATIONS, "MEDIA_CANCEL_NOTIFICATIONS" },
			{ MEDIA_MAKE_CONNECTION, "MEDIA_MAKE_CONNECTION" },
			{ MEDIA_BREAK_CONNECTION, "MEDIA_BREAK_CONNECTION" },
			{ MEDIA_SET_OUTPUT_BUFFERS, "MEDIA_SET_OUTPUT_BUFFERS" },
			{ MEDIA_RECLAIM_OUTPUT_BUFFERS, "MEDIA_RECLAIM_OUTPUT_BUFFERS" },
			{ MEDIA_ORPHAN_RECLAIMABLE_BUFFERS, "MEDIA_ORPHAN_RECLAIMABLE_BUFFERS" },
			{ MEDIA_SET_TIMESOURCE, "MEDIA_SET_TIMESOURCE" },
			{ MEDIA_QUERY_NODES, "MEDIA_QUERY_NODES" },
			{ MEDIA_QUERY_INPUTS, "MEDIA_QUERY_INPUTS" },
			{ MEDIA_QUERY_OUTPUTS, "MEDIA_QUERY_OUTPUTS" },
			{ MEDIA_QUERY_LATENTS, "MEDIA_QUERY_LATENTS" },
			{ MEDIA_SNIFF_FILE, "MEDIA_SNIFF_FILE" },
			{ MEDIA_FORMAT_CHANGED, "MEDIA_FORMAT_CHANGED" },
			{ MEDIA_INSTANTIATE_PERSISTENT_NODE, "MEDIA_INSTANTIATE_PERSISTENT_NODE" },
			{ MEDIA_GET_DEFAULT_INFO, "MEDIA_GET_DEFAULT_INFO" },
			{ MEDIA_SET_RUNNING_DEFAULT, "MEDIA_SET_RUNNING_DEFAULT" },
			{ MEDIA_RELEASE_NODE_REFERENCE, "MEDIA_RELEASE_NODE_REFERENCE" },
			{ MEDIA_BROADCAST_MESSAGE, "MEDIA_BROADCAST_MESSAGE" },
			{ MEDIA_TYPE_ITEM_OP, "MEDIA_TYPE_ITEM_OP" },
			{ MEDIA_FORMAT_OP, "MEDIA_FORMAT_OP" },
			{ MEDIA_ERROR_MESSAGE, "MEDIA_ERROR_MESSAGE" },
			{ MEDIA_GET_DORMANT_NODE, "MEDIA_GET_DORMANT_NODE" },
			{ MEDIA_GET_DORMANT_FLAVOR, "MEDIA_GET_DORMANT_FLAVOR" },
			{ REALLY_QUIT, "REALLY_QUIT" },
			{ SYNC_IDLE, "SYNC_IDLE" },
			{ MEDIA_GET_REALTIME_FLAGS, "MEDIA_GET_REALTIME_FLAGS" },
			{ MEDIA_SET_REALTIME_FLAGS, "MEDIA_SET_REALTIME_FLAGS" },
			{ MEDIA_GET_LATENT_INFO, "MEDIA_GET_LATENT_INFO" },
			{ MEDIA_GET_RUNNING_DEFAULT, "MEDIA_GET_RUNNING_DEFAULT" },
			{ MEDIA_FIND_RUNNING_INSTANCES, "MEDIA_FIND_RUNNING_INSTANCES" }
		};
		for (int ix=0; ix<sizeof(table)/sizeof(table[0]); ix++) {
			if (what == table[ix].what) {
				return table[ix].str;
			}
		}
		static char cc[20];
		sprintf(cc, "code<%x>", what);
		return cc;	/* since it's only called with the app lock held, this is OK */
	}

void
_ServerApp::MessageReceived(
	BMessage * message)
{
	debug(2, "_ServerApp::MessageReceived(%s)\n", msg_what(message->what));
	try {
		switch (message->what) {
		case SYNC_IDLE:
			if (mDefaultManager->Dirty()) {
				mDefaultManager->SaveState();
			}
			if (mMediaFilesManager->Dirty()) {
				mMediaFilesManager->SaveState();
			}
			delete mIdleRunner;
			mIdleRunner = 0;
			break;
		case 'Dump':
			fprintf(stderr, "This may crash...\n");
			mBufferManager->PrintToStream();
			fprintf(stderr, "Whew, it didn't crash!\n");
			break;
		case '_rfd': {
			BMessage reply;
			reply.AddInt32("error", mNodeManager->DumpGlobalReferences(reply, "data"));
			message->SendReply(&reply);
			} break;
		case B_SOME_APP_QUIT:
			MUnregisterApp(message);
			break;
		case MEDIA_REGISTER_APP:
			MRegisterApp(message);
			break;
		case MEDIA_UNREGISTER_APP:
			MUnregisterApp(message);
			break;
		case MEDIA_REGISTER_NODE:
			MRegisterNode(message);
			break;
		case MEDIA_UNREGISTER_NODE:
			MUnregisterNode(message);
			break;
		case MEDIA_REGISTER_BUFFER:
			MRegisterBuffer(message);
			break;
		case MEDIA_SET_DEFAULT:
			MSetDefault(message);
			break;
		case MEDIA_REQUEST_NOTIFICATIONS:
			MRequestNotifications(message);
			break;
		case MEDIA_CANCEL_NOTIFICATIONS:
			MCancelNotifications(message);
			break;
		case MEDIA_SET_OUTPUT_BUFFERS:
			MSetOutputBuffers(message);
			break;
		case MEDIA_RECLAIM_OUTPUT_BUFFERS:
			MReclaimOutputBuffers(message);
			break;
		case MEDIA_ORPHAN_RECLAIMABLE_BUFFERS:
			MOrphanReclaimableBuffers(message);
			break;
		case MEDIA_SET_TIMESOURCE:
			MSetTimeSource(message);
			break;
#if 0
		case MEDIA_MAKE_CONNECTION:
			MMakeConnection(message);
			break;
		case MEDIA_BREAK_CONNECTION:
			MBreakConnection(message);
			break;
#endif
		case MEDIA_QUERY_NODES:
			MQueryNodes(message);
			break;
#if 0
		case MEDIA_QUERY_INPUTS:
			MQueryInputs(message);
			break;
		case MEDIA_QUERY_OUTPUTS:
			MQueryOutputs(message);
			break;
#endif
		case MEDIA_QUERY_LATENTS:
			MQueryLatents(message);
			break;
		case MEDIA_GET_DORMANT_NODE:
			MGetDormantNode(message);
			break;
		case MEDIA_SNIFF_FILE:
			MSniffFile(message);
			break;
		case MEDIA_FORMAT_CHANGED:
			MFormatChanged(message);
			break;
		case MEDIA_INSTANTIATE_PERSISTENT_NODE:
			MInstantiatePersistentNode(message);
			break;
		case MEDIA_GET_DEFAULT_INFO:
			MGetDefaultInfo(message);
			break;
		case MEDIA_SET_RUNNING_DEFAULT:
			MSetRunningDefault(message);
			break;
		case MEDIA_GET_RUNNING_DEFAULT:
			MGetRunningDefault(message);
			break;
		case MEDIA_ACQUIRE_NODE_REFERENCE:
			MAcquireNodeReference(message);
			break;
		case MEDIA_RELEASE_NODE_REFERENCE:
			MReleaseNodeReference(message);
			break;
		case MEDIA_BROADCAST_MESSAGE:
			MBroadcastMessage(message);
			break;
		case MEDIA_TYPE_ITEM_OP:
			MTypeItemOp(message);
			break;
		case MEDIA_FORMAT_OP:
			MFormatOp(message);
			break;
		case MEDIA_GET_DORMANT_FLAVOR:
			MGetDormantFlavor(message);
			break;
		case MEDIA_GET_DORMANT_FILE_FORMATS:
			MGetDormantFileFormats(message);
			break;
		case MEDIA_GET_REALTIME_FLAGS:
			MGetRealtimeFlags(message);
			break;
		case MEDIA_SET_REALTIME_FLAGS:
			MSetRealtimeFlags(message);		
			break;
		case MEDIA_GET_LATENT_INFO:
			MGetLatentInfo(message);
			break;
		case MEDIA_BUFFER_GROUP_REG:
			MBufferGroupReg(message);
			break;
		case MEDIA_FIND_RUNNING_INSTANCES:
			MFindRunningInstances(message);
			break;
		case MEDIA_GET_NODE_ID:
			MGetNodeID(message);
			break;
		case REALLY_QUIT:
			Quit();	/*NORETURN*/
			break;
		default:
			debug(1, "_ServerApp::MessageReceived('%c%c%c%c') unknown\n", 
				message->what>>24, message->what>>16, message->what>>8, message->what);
			BApplication::MessageReceived(message);
			break;
		}
	}
	catch (...)
	{
		debug(0, "media_server: Unexpected exception thrown in _ServerApp::MessageReceived\n");
	}
	if (mDefaultManager->Dirty() || mMediaFilesManager->Dirty()) {
		if (mIdleRunner == 0) {
			BMessage msg(SYNC_IDLE);
			mIdleRunner = new BMessageRunner(BMessenger(this), &msg, SYNC_IDLE_INTERVAL, 1);
		}
	}
}


static status_t
host_quit_waiter(
	void * id)
{
	status_t status;
	int32 tries = 0;
	thread_info tinfo;
	static bool do_again = true;
again:
	status_t err = get_thread_info((thread_id) id, &tinfo);
	if (err == EINTR) {
		fprintf(stderr, "host_quit_waiter: EINTR\n");
		goto again;
	}
	if (err == B_OK) {
		snooze(230000);
#if DEBUG
		if (do_again) goto again;
#else
		if (tries++ < 30) {
			goto again;
		}
		debug(1, "media_addon_server seems hung; using The Force.\n");
		send_signal((thread_id) id, 9);
#endif
	}
	ASSERT(be_app != NULL);
	be_app->PostMessage(REALLY_QUIT);
	return B_OK;
}


bool
_ServerApp::QuitRequested()
{
	debug(2, "_ServerApp::QuitRequested()\n");
	SetQuitMode(true);
	thread_id host_id = find_thread(ADDON_HOST_NAME);
	if (host_id > 0) {
		debug(1, "waiting for thread %d\n", host_id);
		resume_thread(spawn_thread(host_quit_waiter, "_host_quit_waiter_", B_LOW_PRIORITY, (void *)host_id));
		if (mAddOnHost.IsValid()) {
			mAddOnHost.SendMessage(ADDON_HOST_QUIT);
		}
		else {
			send_signal(host_id, SIGHUP);
		}
	}
	else {
		PostMessage(REALLY_QUIT);
	}
	return false;
}


void
_ServerApp::MRegisterApp(
	BMessage * message)
{
	BMessenger ret = message->ReturnAddress();
	team_id team = ret.Team();
	debug(1, "Register App (team %d)\n", team);
	if (team > 0) {
		BMessenger roster;
		status_t error = message->FindMessenger("roster", &roster);
		if (error == B_OK) {
			error = mAppManager->RegisterTeam(team, roster);
		}
		BMessage reply;
		reply.AddInt32("error", error);
		message->SendReply(&reply);
	/* BROADCAST current buffers to new participant */
		BMessage notification(B_MEDIA_BUFFER_CREATED);
		mBufferManager->AddBuffersTo(&notification);
	/* What if this notification times out? Code on app side should know */
	/* to clone in a buffer that's new to it when it sees it. */
		roster.SendMessage(&notification);
	}
	else {
		debug(1, "Spurious MRegisterApp call [%x]\n", team);
	}
}

struct notify_data {
	_ServerApp *me;
	int32 msgCount;
	BMessage *msg[1];
};

status_t
_ServerApp::_DoNotify(notify_data *data)
{
	for (int32 i=0;i<data->msgCount;i++) {
		BMessage *msg = data->msg[i];
		switch (msg->what) {
			case B_MEDIA_NODE_DELETED: {
				data->me->mNotifierManager->BroadcastMessage(msg, DEFAULT_TIMEOUT);
				node_died_q *q = (node_died_q*)malloc((4 + sizeof(media_node_id)*MAX_NODE_DIED_Q_COUNT));
				int32 node_id,j=0;
				q->nodeCount = 0;
				while (msg->FindInt32("media_node_id",j,&node_id)==B_OK) {
					q->nodes[q->nodeCount++] = node_id;
					if (q->nodeCount == MAX_NODE_DIED_Q_COUNT) {
						data->me->mNodeManager->BroadcastMessage(M_NODE_DIED,q,(4 + sizeof(media_node_id)*q->nodeCount),1000000);
						q->nodeCount = 0;
					};
					j++;
				};
				if (q->nodeCount) data->me->mNodeManager->BroadcastMessage(M_NODE_DIED,q,(4 + sizeof(media_node_id)*q->nodeCount),1000000);
				free(q);
			} break;
			case B_MEDIA_BUFFER_DELETED: {
				data->me->mNotifierManager->BroadcastMessage(msg, DEFAULT_TIMEOUT);
				data->me->mAppManager->BroadcastMessage(msg, DEFAULT_TIMEOUT);
			} break;
		};
		delete msg;
	};
	
	free(data);
	return B_OK;
};

status_t
_ServerApp::_UnregisterApp(team_id team, bool spawnThread)
{
	int32 deadCountNodes = 0,msgSize,j;
	status_t error = B_ERROR;
	debug(1, "Unregister App (team %d)\n", team);

	if ((team > 0) && mAppManager->HasTeam(team)) {
		BMessage *nodeNotification = new BMessage(B_MEDIA_NODE_DELETED);

		mNodeManager->UnregisterTeamNodes(team, *nodeNotification, "media_node_id",&deadCountNodes, mBufferManager);
		mBufferManager->PurgeTeamBufferGroups(team);
		error = mAppManager->UnregisterTeam(team);

		if (deadCountNodes) {
			notify_data *data = (notify_data*)malloc(sizeof(notify_data)+sizeof(BMessage*));
			data->me = this;
			data->msgCount = 0;
			if (deadCountNodes) data->msg[data->msgCount++] = nodeNotification; else delete nodeNotification;
			if (spawnThread)
				resume_thread(spawn_thread((thread_entry)_DoNotify,"notification",B_NORMAL_PRIORITY,data));
			else
				_DoNotify(data);
		};
	}
//	else {	//	this is now OK, since we call _UnregisterApp for any SOME_APP_DIED message
//		debug(1, "Spurious _UnregisterApp call [%x]\n", team);
//	}

	return error;
}

void
_ServerApp::MUnregisterApp(
	BMessage * message)
{
	if (message->what == B_SOME_APP_QUIT) {
		int32 team = 0;
		message->FindInt32("be:team", &team);
		status_t err = _UnregisterApp(team, true);
	}
	else {
		BMessenger ret = message->ReturnAddress();
		BMessage reply;
		reply.AddInt32("error", _UnregisterApp(ret.Team(),true));
		message->SendReply(&reply);
	}
}

void
_ServerApp::MRegisterNode(
	BMessage * message)
{
	BMessenger ret = message->ReturnAddress();
	team_id team = ret.Team();
	debug(1, "Register Node (team %d)\n", team);
	if (team > 0) {
		media_node_id node = -1;
		BMessenger roster;
		message->FindMessenger("media_roster", &roster);
		media_node *info = NULL;
		int32 size = 0;
		message->FindData("media_node", B_RAW_TYPE, (const void **)&info, &size);
		const char * node_name = NULL;
		message->FindString("be:node_name", &node_name);
		const char * path = NULL;
		int32 id = 0;
		const char * flavor_name = NULL;
		media_addon_id addon = 0;
		message->FindString("be:_path", &path);
		message->FindInt32("be:_internal_id", &id);
		message->FindString("be:_flavor_name", &flavor_name);
		message->FindInt32("be:_addon_id", &addon);
		status_t error = B_BAD_VALUE;
		media_type producer_type;
		media_type consumer_type;
		if (message->FindInt32("be:producer_type", (int32*)&producer_type))
			producer_type = B_MEDIA_NO_TYPE;
		if (message->FindInt32("be:consumer_type", (int32*)&consumer_type))
			consumer_type = B_MEDIA_NO_TYPE;
		if (info != NULL) {
			error = mNodeManager->RegisterNode(roster, *info, node_name, &node, path, id, 
				flavor_name, addon, producer_type, consumer_type);
		}
		BMessage reply;
		reply.AddInt32("media_node_id", node);
		reply.AddInt32("error", error);
		message->SendReply(&reply);
	/* Tell the world that a new node is around. */
		if (error == B_OK) {
			BMessage notification(B_MEDIA_NODE_CREATED);
			notification.AddInt32("media_node_id", node);
			mNotifierManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
		}
	}
	else {
		debug(1, "Spurious MRegisterNode call [%x]\n", team);
	}
}


void
_ServerApp::MUnregisterNode(
	BMessage * message)
{
	BMessenger ret = message->ReturnAddress();
	team_id team = ret.Team();
	debug(1, "Unregister Node (team %d)\n", team);
	if (team > 0) {
		media_node_id node = -1;
		message->FindInt32("media_node_id", &node);
		status_t error = mNodeManager->UnregisterNode(node);
		/*** should really remove node from Running Defaults, too! ***/
		BMessage reply;
		reply.AddInt32("error", error);
		message->SendReply(&reply);
	/* Tell everybody that the node is no longer around */
		if (error == B_OK)
		{
			BMessage notification(B_MEDIA_NODE_DELETED);
			notification.AddInt32("media_node_id", node);
			mNotifierManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
		}
	}
	else {
		debug(1, "Spurious MUnregisterNode call [%x]\n", team);
	}
}


void
_ServerApp::MRegisterBuffer(
	BMessage * message)
{
	status_t err;
	ssize_t size;
	BMessage reply;
	buffer_clone_info * ptr;
	team_id team = message->ReturnAddress().Team();

	if (message->FindData("clone_info", B_RAW_TYPE, (const void **)&ptr, &size)) return;
	reply.AddInt32("error",err=mBufferManager->RegisterBuffer(team, ptr, &ptr->buffer, media_source::null));
	if (err == B_OK) reply.AddInt32("media_buffer_id", ptr->buffer);

	if (message->SendReply(&reply) == B_OK) {
		/* DISPERSE buffer creation to all teams */
		BMessage notification(B_MEDIA_BUFFER_CREATED);
		notification.AddData("clone_info", B_RAW_TYPE, ptr, sizeof(*ptr));
		mAppManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
	}
}


void
_ServerApp::MSetDefault(
	BMessage * message)
{
	/* save default node for given value */
	int32 what;
	media_node_id id;
	status_t error = message->FindInt32("default", &what);
	bool dormant = false;
	if (error == B_OK) {
		error = message->FindInt32("media_node_id", &id);
		if ((error != B_OK) && message->HasString("be:_path") && message->HasInt32("be:_internal_id")) {
			dormant = true;
			error = B_OK;
		}
	}
	const char * inp_name = 0;
	int32 inp_id = -1;
	if (!error) {
		message->FindInt32("destination", &inp_id);
		message->FindString("name", &inp_name);
	}
	const char * path = 0;
	int32 iid;
	media_addon_id addon = 0;
	const char * flavor = 0;
	if (dormant) {
		error = message->FindString("be:_path", &path);
		if (!error) {
			error = message->FindInt32("be:_internal_id", &iid);
		}
		if (!error) {
			(void) message->FindString("be:_flavor_name", &flavor);
		}
		debug(2, "Default is dormant: Path %d, iid %d\n", path, iid);
	}
	else if (error == B_OK) {
		debug(2, "_ServerApp::MSetDefault %d = id %d\n", what, id);
		error = mNodeManager->FindSaveInfo(id, &path, &iid, &addon, &flavor);
	}
	if (error == B_OK) {
		if (path == NULL) {
			error = B_MEDIA_ADDON_FAILED;
		}
	}
	if (error == B_OK) {
		BMessage msg;
		msg.AddInt32("be:_default", what);
		msg.AddString("be:_path", path);
		msg.AddInt32("be:_internal_id", iid);
		msg.AddInt32("be:_addon_id", addon);	//	addon is not used here
		if (flavor) msg.AddString("be:_flavor_name", flavor);
		if (inp_id > -1) msg.AddInt32("be:_input_id", inp_id);
		if (inp_name != 0) msg.AddString("be:_input_name", inp_name);
		mDefaultManager->RemoveDefault(what);
		error = mDefaultManager->SetDefault(what, msg);
	}
#if 0
	if (error == B_OK) {
		//	set running default
		media_node node;
		error = mNodeManager->FindNode(id, node);
		if (error == B_OK) {
			error = mDefaultManager->SetRunningDefault(what, node);
		}
		if (error == B_OK) {
			BMessage notification(B_MEDIA_DEFAULT_CHANGED);
			notification.AddInt32("be:default", what);
			notification.AddData("be:node", B_RAW_TYPE, &node, sizeof(node));
			mAppManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
		}
	}
#endif
	BMessage reply;
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


void
_ServerApp::MAcquireNodeReference(
	BMessage * message)
{
	media_node_id node;
	media_node clone;
	status_t error = message->FindInt32("media_node_id", &node);
	if (error == B_OK) {
		if (node < 0) {
			debugger("Someone tried to acquire w/ default node ID");
			error = B_MEDIA_BAD_NODE;
		} else {
			error = mNodeManager->FindNode(node, clone);
			if (error == B_OK)
				mNodeManager->IncrementGlobalRefCount(node);
		}
	} else {
		debug(1, "AcquireNodeReference message didn't have media node id\n");
	}
	
	BMessage reply;
	if (error == B_OK)
		error = reply.AddData("media_node", B_RAW_TYPE, &clone, sizeof(clone));

	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


void
_ServerApp::MRequestNotifications(
	BMessage * message)
{
	BMessenger msgr;
	status_t err = B_OK;
	BMessenger ret = message->ReturnAddress();
	team_id team = ret.Team();
	int32 what;
	debug(1, "Request Notifications (team %d)\n", team);
	if (team > 0) {
		if ((err = message->FindMessenger("messenger", &msgr)) >= B_OK) {
			err = message->FindInt32("which",&what);
			const media_node * node = NULL;
			ssize_t size;
			message->FindData("node", B_RAW_TYPE, (const void **)&node, &size);
			if (!err) {
				err = mNotifierManager->RegisterNotifier(what,msgr, node);
				if (!err) err = BroadcastCurrentStateTo(msgr);
			};
		}
		BMessage reply;
		reply.AddInt32("error", err);
		message->SendReply(&reply);
	}
	else
	{
		debug(1, "Spurious MRequestNotifications call [%x]\n", team);
	}
}


void
_ServerApp::MCancelNotifications(
	BMessage * message)
{
	BMessenger msgr;
	status_t err = B_OK;
	BMessenger ret = message->ReturnAddress();
	team_id team = ret.Team();
	int32 what;
	debug(1, "Cancel Notifications (team %d)\n", team);
	if (team > 0)
	{
		if (message->FindInt32("which",&what)) what = B_MEDIA_WILDCARD;
		const media_node * node = NULL;
		ssize_t size;
		message->FindData("node", B_RAW_TYPE, (const void **)&node, &size);
		if ((err = message->FindMessenger("messenger", &msgr)) >= B_OK) {
			err = mNotifierManager->UnregisterNotifier(what,msgr, node);
		};
		BMessage reply;
		reply.AddInt32("error", err);
		message->SendReply(&reply);
	}
	else
	{
		debug(1, "Spurious MCancelNotifications call [%x]\n", team);
	}
}

void
_ServerApp::MSetOutputBuffers(
	BMessage * message)
{
	status_t err;
	int32 buf_count;
	bool reclaim = false;
	const media_source *output;
	ssize_t size;
	type_code type;
	media_buffer_id * ptr = NULL;
	set_buffers_q cmd;
	const void * data = NULL;
	BMessage reply;

	if (!(err=message->FindData("output", B_RAW_TYPE, (const void **)&output, &size)) &&
		!(err=message->FindBool("will_reclaim", &reclaim)) &&
		!(err=message->GetInfo("media_buffer_id", &type, &buf_count))) {
		if ((size != sizeof(*output)) || (type != B_INT32_TYPE) || (buf_count < 1)) {
			err = B_BAD_VALUE;
		} else {
			port_info pi;
			if (!(err=get_port_info(output->port,&pi))) {

				if (buf_count > MAX_BUFFER_COUNT) buf_count = MAX_BUFFER_COUNT;
				for (int32 i=0; i<buf_count; i++) {
					message->FindInt32("media_buffer_id", i, &cmd.buffers[i]);
					mBufferManager->AcquireBuffer(pi.team, cmd.buffers[i], *output);
					debug(2, "Transferred buffer %d [%s]\n", cmd.buffers[i], reclaim ? "will reclaim" : "for good");
				};
				
				cmd.count = 			buf_count;
				cmd.will_reclaim = 		reclaim;
				cmd.user_data = NULL;	message->FindPointer("be:user_data",(void**)&cmd.user_data);
				cmd.cookie = -1; 		message->FindInt32("be:cookie", &cmd.cookie);
				cmd.change_tag = -1;	message->FindInt32("be:change_tag", &cmd.change_tag);
				cmd.where = *output;
	
				if (!(err=message->FindData("be:destination", B_RAW_TYPE, &data, &size) && (data != NULL)))
					memcpy(&cmd.destination, data, sizeof(cmd.destination));
	
				reply.AddInt32("command", BP_SET_BUFFERS);
				reply.AddData("data", B_RAW_TYPE, &cmd, sizeof(cmd));
				reply.AddInt32("port", output->port);
			};
		};
	};

	reply.AddInt32("error", err);
	message->SendReply(&reply);
}

void
_ServerApp::MReclaimOutputBuffers(
	BMessage * message)
{
	status_t err;
	set_buffers_q cmd;
	int32 ownerArea=-1,count=-1,nodeCount=0,j;
	team_id team = message->ReturnAddress().Team();
	type_code type = (type_code)-1;
	int32 * ids = NULL;
	BMessage reply;
	
	if (!(err=message->FindInt32("recycle", &ownerArea)) &&
		!(err=message->GetInfo("media_buffer_id", &type, &count))) {
		ids = new int32[count];
		for (int32 i=0; i<count; i++) message->FindInt32("media_buffer_id", i, &ids[i]);
		mBufferManager->ReclaimBuffers(ids,count,team,ownerArea);
		/*	Apps need to call SetOutputBuffersFor() with NULL on the output before	*/
		/*	they try to reclaim buffers. Else the reclaim should (and will) block	*/
		/*	until the current user is done. */
		delete[] ids;
	};

	if (err < 0) reply.AddInt32("error", err);
	debug(2, "MReclaimOutputBuffers returns %x\n", err);
	message->SendReply(&reply);
}


void
_ServerApp::MOrphanReclaimableBuffers(
	BMessage * message)
{
	debug(1, "_ServerApp::MOrphanReclaimableBuffers()\n");
	int32 id;
	team_id team = message->ReturnAddress().Team();

	BMessage bad_bufs(B_MEDIA_BUFFER_DELETED);
	bool wasOwner = message->FindBool("was_owner");
	for (int32 ix=0;message->FindInt32("buffers", ix, &id) == B_OK; ix++)
		mBufferManager->ReleaseBuffer(team,id,wasOwner,&bad_bufs);

	/* send reply, even though there's nothing in it */
	BMessage reply;
	message->SendReply(&reply);

	/* notify everyone about the new state of affairs */
	if (bad_bufs.HasInt32("media_buffer_id")) {
		mAppManager->BroadcastMessage(&bad_bufs, DEFAULT_TIMEOUT);
	}
}


void
_ServerApp::MSetTimeSource(
	BMessage * message)
{
	media_node_id node;
	media_node_id time_source;
	status_t error;

	error = message->FindInt32("media_node_id", &node);
	if (error == B_OK)
	{
		error = message->FindInt32("time_source", &time_source);
	}
	media_node mnode;
	if (error == B_OK) {
		error = mNodeManager->FindNode(node, mnode);
	}
	/*** Should we remember about whom is using what time source? ***/
	/*** 1: this is good if the time source goes away ***/
	/*** 2: the time source knows anyway ***/
	BMessage reply;
	if (error == B_OK)
	{
		set_time_source_q cmd;
		cmd.time_source = time_source;
		reply.AddInt32("command", M_SET_TIMESOURCE);
		reply.AddData("data", B_RAW_TYPE, &cmd, sizeof(cmd));
		reply.AddInt32("port", mnode.port);
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


#if 0
void
_ServerApp::MMakeConnection(
	BMessage * message)
{
	ssize_t size;
	media_input * input = NULL;
	status_t error = message->FindData("input", B_RAW_TYPE, (const void **)&input, &size);
	media_output * output = NULL;
	if (error == B_OK) {
		error = message->FindData("output", B_RAW_TYPE, (const void **)&output, &size);
	}
	media_format * format = NULL;
	if (error == B_OK) {
		error = message->FindData("format", B_RAW_TYPE, (const void **)&format, &size);
	}
	/* record that a connection was made */
	if (error == B_OK) {
		mNodeManager->AddInput(input->node.node, input->destination);
		mNodeManager->AddOutput(output->node.node, output->source);
	}
	BMessage reply;
	reply.AddInt32("error", error);
	message->SendReply(&reply);
	/* broadcast to interested parties */
	if (error == B_OK) {
		BMessage notification(B_MEDIA_CONNECTION_MADE);
		notification.AddData("output", B_RAW_TYPE, output, sizeof(*output));
		notification.AddData("input", B_RAW_TYPE, input, sizeof(*input));
		notification.AddData("format", B_RAW_TYPE, format, sizeof(*format));
		mNotifierManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
	}
}
#endif


#if 0
void
_ServerApp::MBreakConnection(
	BMessage * message)
{
	ssize_t size;
	media_destination * destination = NULL;
	status_t error = message->FindData("destination", B_RAW_TYPE, (const void **)&destination, &size);
	media_node_id destination_node;
	if (error == B_OK)
	{
		error = message->FindInt32("destination_node", &destination_node);
	}
	media_source * source = NULL;
	if (error == B_OK)
	{
		error = message->FindData("source", B_RAW_TYPE, (const void **)&source, &size);
	}
	media_node_id source_node;
	if (error == B_OK)
	{
		error = message->FindInt32("source_node", &source_node);
	}
	/* record that a connection was broken */
	if (error == B_OK)
	{
		mNodeManager->RemoveInput(destination_node, *destination);
		mNodeManager->RemoveOutput(source_node, *source);
	}
	BMessage reply;
	reply.AddInt32("error", error);
	message->SendReply(&reply);
	/* broadcast to interested parties */
	if (error == B_OK)
	{
		BMessage notification(B_MEDIA_CONNECTION_BROKEN);
		notification.AddData("destination", B_RAW_TYPE, destination, sizeof(*destination));
		notification.AddData("source", B_RAW_TYPE, source, sizeof(*source));
		mNotifierManager->BroadcastMessage(&notification, DEFAULT_TIMEOUT);
	}
}
#endif


void
_ServerApp::MQueryNodes(
	BMessage * message)
{
	BMessage reply;
	media_format * in_fmt = NULL;
	media_format * out_fmt = NULL;
	const char * node_name = NULL;
	uint32 node_kinds = 0;
	ssize_t size;
	media_node_id node = 0;
	if (message->FindData("be:input_format", B_RAW_TYPE, (const void **)&in_fmt, &size))
		in_fmt = NULL;
	if (message->FindData("be:output_format", B_RAW_TYPE, (const void **)&out_fmt, &size))
		out_fmt = NULL;
	if (message->FindString("be:name", &node_name))
		node_name = NULL;
	if (message->FindInt32("be:node_kinds", (int32 *)&node_kinds))
		node_kinds = 0;
	status_t error;
	if (!message->FindInt32("be:node", &node)) {
		error = mNodeManager->GetLiveNode(reply, "live_nodes", node);
	}
	else {
		error = mNodeManager->GetLiveNodes(reply, "live_nodes", in_fmt, out_fmt, node_name, node_kinds);
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


#if 0
void
_ServerApp::MQueryInputs(
	BMessage * message)
{
	BMessage reply;
	int32 node;
	status_t error = message->FindInt32("media_node_id", &node);
	if (error == B_OK)
	{
		error = mNodeManager->GetNodeInputs(node, reply, "node_inputs");
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}
#endif


#if 0
void
_ServerApp::MQueryOutputs(
	BMessage * message)
{
	BMessage reply;
	int32 node;
	status_t error = message->FindInt32("media_node_id", &node);
	if (error == B_OK)
	{
		error = mNodeManager->GetNodeOutputs(node, reply, "node_outputs");
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}
#endif


void
_ServerApp::MQueryLatents(
	BMessage * message)
{
	/* forward to add-on host */
	status_t err = AddOnHost().SendMessage(message);
	debug(2, "MQueryLatents SendMessage() returns %x\n", err);
}


void
_ServerApp::MGetDormantNode(
	BMessage * message)
{
	media_node_id node;
	status_t err = message->FindInt32("be:_node", &node);
	ASSERT(err <= B_OK);
	BMessage reply;
	dormant_node_info info;
	if (err == B_OK) {
		err = mNodeManager->FindDormantNodeFor(node, &info);
	}
	ASSERT(err <= B_OK);
	if (err == B_OK) {
		err = reply.AddData("be:_dormant_node_info", B_RAW_TYPE, &info, sizeof(info));
	}
	reply.AddInt32("error", err);
	message->SendReply(&reply);
}


void
_ServerApp::MSniffFile(
	BMessage * message)
{
#if 0	/* We don't want to sniff live nodes, because that might interrupt running streams */
	/* You can always use the explicit by-node-id sniffing interface for that. */
	entry_ref file;
	status_t err;
	BMessage reply;
	int sniffed = 0;
	uint32 kinds;
	if ((err = message->FindInt32("kinds", (int32*)&kinds)) == B_OK) {
		if ((err = message->FindRef("refs", sniffed, &file)) >= B_OK) {
			/* All we want to do here is to return nodes that can sniff -- the roster will do the sniffing. */
			/* Conveniently, this greatly reduces the amount of time that the server is locked up in this */
			/* query and spends the time where it should be spent -- in the calling app. */
			mNodeManager->GetLiveNodes(reply, "be:sniff_nodes", NULL, NULL, NULL, kinds | B_FILE_INTERFACE);
			sniffed++;
		}
	}
	if ((err == B_BAD_INDEX) && (sniffed > 0)) {
		err = B_OK;
	}
	if (err != B_OK) {
		reply.AddInt32("error", err);
		message->SendReply(&reply);
	}
	else {	/* forward list of files to addon roster for additional sniffing */
		message->AddMessage("be:_initial_reply", &reply);
#endif
		AddOnHost().SendMessage(message);
#if 0
	}
#endif
}


void
_ServerApp::MFormatChanged(
	BMessage * message)
{
	media_source * source;
	media_destination * destination;
	media_format * format;
	ssize_t size;
	status_t err = message->FindData("be:_source", B_RAW_TYPE, (const void **)&source, &size);
	if (err >= B_OK) {
		err = message->FindData("be:_destination", B_RAW_TYPE, (const void **)&destination, &size);
	}
	if (err >= B_OK) {
		err = message->FindData("be:_format", B_RAW_TYPE, (const void **)&format, &size);
	}
//	This message is now asynchronous
//	BMessage reply;
//	reply.AddInt32("error", err);
//	message->SendReply(&reply);
	if (err >= B_OK) {
		BMessage notify(B_MEDIA_FORMAT_CHANGED);
		notify.AddData("be:source", B_RAW_TYPE, source, sizeof(media_source));
		notify.AddData("be:destination", B_RAW_TYPE, destination, sizeof(media_destination));
		notify.AddData("be:format", B_RAW_TYPE, format, sizeof(media_format));
		mNotifierManager->BroadcastMessage(&notify, DEFAULT_TIMEOUT);
	}
}


void
_ServerApp::MInstantiatePersistentNode(
	BMessage * message)
{
	AddOnHost().SendMessage(message);
}


void
_ServerApp::MGetDefaultInfo(
	BMessage * message)
{
	BMessage reply;
	status_t err;
	media_node_id node;
	err = message->FindInt32("be:node", &node);
	if (err == B_OK) 
		err = mDefaultManager->GetDefault(node, reply);
	
	//	If there is a running default, add node ID to information.
	//	Note that this Node may be different than the saved default info.
	if (err == B_OK) {
		media_node the_def;
		if (mDefaultManager->GetRunningDefault(node, the_def) == B_OK)  {
			debug(2, "_ServerApp: real node for default %d is %d\n",
				the_def.node, node);
			reply.AddInt32("be:node", the_def.node);
		}
	}
	
	reply.AddInt32("error", err);
	message->SendReply(&reply);
}

void
_ServerApp::MGetRunningDefault(
	BMessage * message)
{
	debug(2, "_ServerApp::MGetRunningDefault\n");
	BMessage reply;
	media_node_id node;
	status_t err = message->FindInt32("be:node", &node);
	media_node realNode;
	if (err == B_OK) {
		debug(2, "Asking default manager for real node for default %d\n",
			node);
		err = mDefaultManager->GetRunningDefault(node, realNode);
	}
		
	if (err == B_OK) {
		debug(2, "Setting up reply: %d\n", realNode.node);
		reply.AddInt32("be:node", realNode.node);
	}

	reply.AddInt32("error", err);
	message->SendReply(&reply);
}


void
_ServerApp::MSetRunningDefault(
	BMessage * message)
{
	status_t err;
	media_node_id what;
	media_node node;
	err = message->FindInt32("be:node", &what);
	if (err == B_OK) {
		void * ptr = NULL;
		ssize_t size;
		err = message->FindData("be:info", B_RAW_TYPE, (const void **)&ptr, &size);
		if (err == B_OK) {
			memcpy(&node, ptr, sizeof(node));
		}
	}
	if (err == B_OK) {
		err = mDefaultManager->SetRunningDefault(what, node);
	}
	debug(1, "_ServerApp::MSetRunningDefault(%d, %d): %s\n", what, node.node, strerror(err));
	BMessage reply;
	reply.AddInt32("error", err);
	message->SendReply(&reply);
}

void 
_ServerApp::MGetRealtimeFlags(BMessage *message)
{
	BMessage reply;
	reply.AddInt32("be:flags", mDefaultManager->GetRealtimeFlags());
	message->SendReply(&reply);
}

void 
_ServerApp::MSetRealtimeFlags(BMessage *message)
{
	BMessage reply;
	uint32 new_flags;
	if (message->FindInt32("be:flags", (int32*) &new_flags) == B_OK) 
		mDefaultManager->SetRealtimeFlags(new_flags);	
	else
		reply.AddInt32("error", B_ERROR);
	
	message->SendReply(&reply);
}


void
_ServerApp::MReleaseNodeReference(
	BMessage * message)
{
	media_node_id node_id;
	ssize_t size;
	status_t err = message->FindInt32("be:node_id", &node_id);
	if (err == B_OK) {
		// The node manager will respond to the application that requested
		// the deletion, waiting for the destructor if it is invoked.
		mNodeManager->DecrementGlobalRefCount(node_id, message);
	} else
		printf("Media server received bogus release node message\n");
}


void
_ServerApp::MBroadcastMessage(
	BMessage * message)
{
	if (IsQuitMode()) {
		return;
	}
	message->FindInt32("be:old_what", (int32*)&message->what);
	mNotifierManager->BroadcastMessage(message, DEFAULT_TIMEOUT);
}


void
_ServerApp::MTypeItemOp(
	BMessage * message)
{
	BMessage reply;
	int32 op;
	status_t err = message->FindInt32("be:operation", &op);
	if (!err) {
		bool dirty = false;

		switch (op) {
		case _BMediaRosterP::B_OP_GET_TYPES:
			err = mMediaFilesManager->GetTypes(*message, reply);
			break;
		case _BMediaRosterP::B_OP_ADD_TYPE:
			err = mMediaFilesManager->AddType(*message, reply);
			break;
		case _BMediaRosterP::B_OP_GET_ITEMS:
			err = mMediaFilesManager->GetItems(*message, reply);
			break;
		case _BMediaRosterP::B_OP_SET_ITEM:
			err = mMediaFilesManager->SetItem(*message, reply);
			dirty = true;
			break;
		case _BMediaRosterP::B_OP_CLEAR_ITEM:
			err = mMediaFilesManager->ClearItem(*message, reply);
			dirty = true;
			break;
		case _BMediaRosterP::B_OP_REMOVE_ITEM:
			err = mMediaFilesManager->RemoveItem(*message, reply);
			dirty = true;
			break;
		default:
			debug(1, "Unknown operation in MTypeItemOp(%d)\n", op);
			err = B_BAD_VALUE;
		}

		if (dirty)
		{
			BMessage m(MEDIA_SOUND_EVENT_CHANGED);
			const char* event = 0;
			err = message->FindString("be:item", &event);
			if (err == B_OK)
			{
				// translate to HostAppese
				m.AddString("be:media_name", event);
				AddOnHost().SendMessage(&m);
			}
		}
	}
	if (err < B_OK) {
		reply.AddInt32("error", err);
	}
	message->SendReply(&reply);
}


void
_ServerApp::MFormatOp(
	BMessage * message)
{
	int32 op = 0;
	status_t err = message->FindInt32("be:_op", &op);
	BMessage reply;
	switch (op) {
	case MF_SET_FORMAT: {
		const void * description = 0;
		const void * format = 0;
		ssize_t size;
		media_format_description * desc;
		int32 count = 0;
		type_code type;
		media_format fmt;
		int32 flags = 0;
		if ((err = message->GetInfo("be:_description", &type, &count)) == B_OK) {
			message->FindInt32("be:_make_flags", &flags);
			if (!message->FindData("be:_format", B_RAW_TYPE, &format, &size)) {
				fmt = *reinterpret_cast<media_format *>(const_cast<void *>(format));
			}
			desc = new media_format_description[count];
			for (int ix=0; ix<count; ix++) {
				(void)message->FindData("be:_description", B_RAW_TYPE, ix, &description, &size);
				desc[ix] = *reinterpret_cast<media_format_description *>(const_cast<void *>(description));
			}
			err = mFormatRegistry->AddFormats(desc, count, &fmt, flags);
			delete[] desc;
			if (!err) {
				reply.AddData("be:_format", B_RAW_TYPE, &fmt, sizeof(fmt));
			}
		}
		} break;
#if 0
	case MF_SET_FORMAT: {
		void * description;
		void * format;
		ssize_t size;
		media_format_description desc;
		media_format fmt;
		err = message->FindData("be:_description", B_RAW_TYPE, (const void **)&description, &size);
		if (!err) {
			err = message->FindData("be:_format", B_RAW_TYPE, (const void **)&format, &size);
		}
		if (!err) {
			memcpy(&desc, description, sizeof(media_format_description));
			memcpy(&fmt, format, sizeof(media_format));
			err = mFormatRegistry->AddFormat(desc, fmt);
		}
		if (!err) {
			reply.AddData("be:_format", B_RAW_TYPE, &fmt, sizeof(fmt));
		}
		} break;
#endif
	case MF_GET_FORMATS: {
		err = mFormatRegistry->GetFormats(reply, "be:_format", "be:_description", "be:_priority", "be:_addons");
		} break;
	case MF_BIND_FORMATS: {
		ssize_t size = 0;
		int32 count = 0;
		type_code type = 0;
		const void * fmt = 0;
		const char * addon = 0;
		if ((err = message->GetInfo("be:_format", &type, &count)) == B_OK) {
			message->FindString("be:_path", &addon);
			media_format * desc = new media_format[count];
			for (int ix=0; ix<count; ix++) {
				(void)message->FindData("be:_format", B_RAW_TYPE, ix, (const void**)&fmt, &size);
				desc[ix] = *reinterpret_cast<const media_format *>(fmt);
			}
			err = mFormatRegistry->BindFormats(desc, count, addon);
			delete[] desc;
		}
		} break;
	default:
		err = B_BAD_VALUE;
	}
	if (err < B_OK) {
		reply.AddInt32("error", err);
	}
	message->SendReply(&reply);
}

void
_ServerApp::MGetDormantFlavor(
	BMessage * message)
{
	AddOnHost().SendMessage(message);
}


void
_ServerApp::MGetDormantFileFormats(
	BMessage * message)
{
	AddOnHost().SendMessage(message);
}

void 
_ServerApp::MGetLatentInfo(BMessage *message)
{
	AddOnHost().SendMessage(message);
}

void
_ServerApp::MBufferGroupReg(BMessage * message)
{
	team_id team;
	if (0 > message->FindInt32("be:_team", &team)) {
		debug(0, "The media_server received a buffer group registration message with no team information. "
			"You are probably running a mismatched server/library combination. Please re-install the BeOS, "
			"or see http://www.be.com/ for an update.");
	}
	else {
		mBufferManager->RegisterBufferGroup(message, "be:_register_buf", team);
		mBufferManager->UnregisterBufferGroup(message, "be:_unregister_buf", team);
	}
}

void
_ServerApp::MFindRunningInstances(BMessage * msg)
{
	BMessage reply;

	media_addon_id addon;
	int32 flavor;
	status_t err = B_OK;
	err = msg->FindInt32("be:_addon_id", &addon);
	if (err	== B_OK)
		err = msg->FindInt32("be:_flavor_id", &flavor);
	if (err == B_OK)
		err = mNodeManager->FindNodesFor(addon, flavor, reply, "be:_node_id");
	reply.AddInt32("error", err);
	msg->SendReply(&reply);
}

void
_ServerApp::MGetNodeID(BMessage * msg)
{
	BMessage reply;

	port_id port;
	status_t err = B_OK;
	err = msg->FindInt32("be:port", &port);
	if (err == B_OK)
		err = mNodeManager->FindNodesForPort(port, reply, "be:node_id");
	reply.AddInt32("error", err);
	msg->SendReply(&reply);
}



status_t	/* this is the node state for listeners, which is distinct from the buffer state for rosters */
_ServerApp::BroadcastCurrentStateTo(
	BMessenger & msgr)
{
	BMessage notification(B_MEDIA_NODE_CREATED);
	mNodeManager->GetNodes(notification);
	return msgr.SendMessage(&notification);
}


BMessenger &
_ServerApp::AddOnHost()
{
	if (!mAddOnHost.IsValid()) {
		static int warned = 0;
		team_id team = -1;
		entry_ref ent;
		app_info info;
		if (warned) {
			warned = (warned + 1) & 0xf;	/* re-try every 16 times */
			return mAddOnHost;
		}
		if (be_app->GetAppInfo(&info) < B_OK) {
			debug(0, "The media server cannot get running app_info -- please restart your machine.");
			if (!warned) warned = 1;
			return mAddOnHost;
		}
		ent = info.ref;
		ent.set_name(ADDON_HOST_NAME);
		BPath path;
		BEntry e(&ent);
		e.GetPath(&path);
		debug(2, "host: %s\n", path.Path());
		char * argv[6];
		int argc = 0;
		const char * debugger_prog = getenv("ADDON_HOST_DEBUGGER");
		if (debugger_prog) {
			struct stat st;
			if (!stat(debugger_prog, &st) && !get_ref_for_path(debugger_prog, &ent)) {
				argv[argc++] = (char *)path.Path();
#if 0
				char * temp = (char *)alloca(strlen(path.Path()));
				strcpy(temp, path.Path());
				*strrchr(temp, '/') = 0;
				chdir(temp);
#endif
			}
			else {
				fprintf(stderr, "can't find debugger: %s\n", debugger_prog);
				debugger_prog = NULL;
			}
		}
		if (g_debug) {
			argv[argc++] = "--debug";
		}
		if (g_about_host) {
			argv[argc++] = "--about";
		}
		argv[argc] = NULL;
		status_t err = be_roster->Launch(&ent, argc, argv, &team);
		if (err < B_OK) {
do_warn:
			debug(0, "Cannot start the Media Addon Host!\n%s [%x]\nTry to restart media services in the Media preference panel. "
				"If you get this message again, your machine may need re-starting, you may be out of memory, or you may need "
				"to install an updated version of BeOS.", 
				strerror(err), err);
			return mAddOnHost;
		}
		if (debugger_prog) {
			thread_info trinfo;
			int cnt = 0;
			while (get_thread_info(find_thread(ADDON_HOST_NAME), &trinfo) < B_OK) {
				if (cnt++ > 100) {
					team = -1;
					err = -1;
					goto do_warn;
				}
				snooze(200000);
			}
			team = trinfo.team;
			fprintf(stderr, "Just about now would be a good time to press 'Run'\n");
			cnt = 0;
			bigtime_t old_time = trinfo.user_time + trinfo.kernel_time;
			while (true) {
				if (get_thread_info(trinfo.thread, &trinfo) < B_OK) {
					team = -1;
					err = -1;
					goto do_warn;
				}
				if (trinfo.user_time + trinfo.kernel_time > old_time + 20) {
					break;
				}
				if (cnt++ > 100) {
					team = -1;
					err = -1;
					goto do_warn;
				}
				snooze(200000);
			}
		}
		debug(2, "Team is %d\n", team);
		int32 tries = 0;
		int32 max_tries = 8;
		if (debugger_prog) max_tries = 25;
try_again:
		mAddOnHost = BMessenger(NULL, team, &err);
		if (err < B_OK) {
			tries++;
			if (tries < max_tries) {
				debug(2, "AddonHost not found, re-trying\n");
				snooze(120000 * tries);
				goto try_again;
			}
			goto do_warn;
		}
	}
	return mAddOnHost;
}


void
_ServerApp::AboutRequested()
{
	if (m_about->Lock()) {	
		m_about->Activate(true);
		m_about->Unlock();
		return;
	}
	m_about = new BWindow(BRect(0,0,383,127), "About media_server", B_TITLED_WINDOW, 
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
	BView * bg = new BView(m_about->Bounds(), "background", B_FOLLOW_NONE, B_WILL_DRAW);
	m_about->AddChild(bg);
	bg->SetViewColor(255,255,255);
	BBitmap * picture = BTranslationUtils::GetBitmap("media_about.tga");
#if defined(_PR3_COMPATIBLE_)
	if (picture)
		bg->SetViewBitmap(picture, B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
#endif
	BRect r = bg->Bounds();
	r.right -= 150;
	r.bottom -= 20;
	TextScrollView * tv = new TextScrollView(r, "about text", B_FOLLOW_NONE);
	bg->AddChild(tv);
	tv->SetText(about_text);
	tv->MoveTo(139,10);
	{
		BScreen scrn;
		r = scrn.Frame();
	}
	m_about->MoveTo((r.Width()-m_about->Frame().Width())/2, 
		(r.Height()-m_about->Frame().Height())/3);
	m_about->Show();
}


bool
_ServerApp::IsQuitMode() const
{
	return m_quitting;
}

void
_ServerApp::SetQuitMode(
	bool quitting)
{
	m_quitting = quitting;
}






static int
do_arg(
	int left,
	char ** args)
{
	left = left;
	if (!strcmp(*args, "--debug")) {
		g_debug = true;
		debug(1, "debugging enabled\n");
	}
	else if (!strcmp(*args, "--help")) {
		fprintf(stderr, "media_server options:\n");
		fprintf(stderr, "--help         print this help and exit\n");
		fprintf(stderr, "--debug        turn on debugging output\n");
		fprintf(stderr, "--about        show about box\n");
		fprintf(stderr, "--quit         tell the server to quit\n");
	}
	else if (!strcmp(*args, "--about")) {
		be_app->PostMessage(B_ABOUT_REQUESTED);
	}
	else if (!strcmp(*args, "--quit")) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	else if (!strcmp(*args, "--about-host")) {
		g_about_host = true;
	}
	else {
		fprintf(stderr, "unknown option: %s (use --help for help)\n",
			*args);
	}
	return 0;
}


sem_id quit_sem;

static void
die_gracefully(
	int sig)
{
	signal(sig, die_gracefully);
	release_sem(quit_sem);
}

#define DEATH_POLLING_INTERVAL 1000000

status_t
_ServerApp::DirtyWorkLaunch(void *server)
{
	return ((_ServerApp*)server)->DirtyWork();
};


void
_ServerApp::AddPurgableBufferGroup(
	team_id team,
	area_id orig,
	area_id clone,
	void * addr)
{
	purgable_buffer_group pbg = { team, orig, clone, addr };
	if (sPurgableGroupLock.Lock()) {
#if DEBUG
		int count = 0;
		for (std::list<purgable_buffer_group>::iterator ptr(sPurgableGroups.begin());
				ptr != sPurgableGroups.end(); ptr++) {
			if ((*ptr).clone == clone) count++;
		}
		assert(count == 0);	//	adding the same group twice means trouble in the caller
#endif
		sPurgableGroups.push_back(pbg);
		sPurgableGroupLock.Unlock();
	}
}


void
_ServerApp::CancelPurgableBufferGroupCleanup(
	area_id clone)
{
	if (sPurgableGroupLock.Lock()) {
		for (std::list<purgable_buffer_group>::iterator ptr(sPurgableGroups.begin());
				ptr != sPurgableGroups.end(); ptr++) {
			if ((*ptr).clone == clone) {
				sPurgableGroups.erase(ptr);
				break;
			}
		}
		sPurgableGroupLock.Unlock();
	}
}


status_t
_ServerApp::DirtyWork()
{
	team_id poll;
	team_info ti;
	int32 i,j,deadCount,msgSize,msgCount,teamIndex = 0;

	signal(SIGINT, die_gracefully);
	signal(SIGHUP, die_gracefully);
	while (acquire_sem_etc(quit_sem,1,B_TIMEOUT,DEATH_POLLING_INTERVAL) == B_TIMED_OUT) {
		if (sPurgableGroupLock.Lock()) {
			std::list<pair<purgable_buffer_group, bool> > bing;	//	when you hear the 'bing', you know it's done
			std::list<purgable_buffer_group>::iterator ptr(sPurgableGroups.begin());
			while (ptr != sPurgableGroups.end()) {
				_shared_buffer_list & sbl = *(_shared_buffer_list *)(*ptr).addr;
				//	It's OK to nuke a buffer group that's just waiting for buffers
				//	to be returned to it from a previous SetOutputBuffersFor().
				if (sbl.flags & SB_FLAG_WAITING) {
					bing.push_back(pair<purgable_buffer_group, bool>(*ptr, false));
					sPurgableGroups.erase(ptr++);
				}
				else {
				//	If, however, it has been actively doling out buffers, we need
				//	to make sure all of those buffers have been recycled before we
				//	kick it out. Else we would pull the rug out from under whomever
				//	is currently using that buffer (like the Mixer).
					int32 count = 0;
					get_sem_count(sbl.reclaimSem, &count);
					if (count >= sbl.bufferCount) {
						bing.push_back(pair<purgable_buffer_group, bool>(*ptr, true));
						sPurgableGroups.erase(ptr++);
					}
					else {
						++ptr;
					}
				}
			}
			sPurgableGroupLock.Unlock();
			std::list<pair<purgable_buffer_group, bool> >::iterator del(bing.begin());
			while (del != bing.end()) {
				CleanupPurgedBufferGroup((*del).first, (*del).second);
				del++;
			}
		}
	};
	be_app->PostMessage(B_QUIT_REQUESTED);
	debug(1, "send_quit() triggered\n");
	return 0;
}

//	called from the cleanup thread
void
_ServerApp::CleanupPurgedBufferGroup(
	const purgable_buffer_group & pbg,
	bool wasOwner)
{
	BMessage gonners(B_MEDIA_BUFFER_DELETED);
	mBufferManager->CleanupPurgedBufferGroup(pbg.team, pbg.orig, pbg.clone, pbg.addr, wasOwner, gonners);

	if (gonners.HasInt32("media_buffer_id")) {
		mAppManager->BroadcastMessage(&gonners, DEFAULT_TIMEOUT);
	}
}

void
_ServerApp::ArgvReceived(
	int32 argc,
	char ** argv)
{
	for (int ix=1; ix<argc; ix++) {
		ix += do_arg(argc-ix, &argv[ix]);
	}
}


int
main(
	int argc,
	char * argv[])
{
	setpgid(0,0);
	bool yes = true;
	BMediaRoster::MediaFlags((media_flags)B_MEDIA_CAP_SET_SERVER, &yes, sizeof(yes));

	for (int ix=1; ix<argc; ix++) {
		if (!strcmp(argv[ix], "--debug")) {
			g_debug = true;	/* make sure debugging works always if requested */
		}
		else if (!strcmp(argv[ix], "--about-host")) {
			g_about_host = true;
		}
	}
	debug(1, "Creating application\n");
	_ServerApp app;
	debug(1, "Starting application\n");

	thread_id thread;
	quit_sem = create_sem(0, "quit_sem");
	resume_thread(thread = spawn_thread(_ServerApp::DirtyWorkLaunch, "dirty_work", B_NORMAL_PRIORITY, &app));

	signal(SIGHUP, die_gracefully);
	signal(SIGINT, die_gracefully);

	app.Run();
	debug(1, "Application done, exiting\n");

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	delete_sem(quit_sem);
	status_t status;
	wait_for_thread(thread, &status);

	return 0;
}



//#pragma mark KeyServer

#if KEY_SERVER

KeyServer::KeyServer()
{
	m_nextKeyCode = 0;
	m_port = create_port(10, REGISTRY_PORT_NAME);
	m_thread = -1;
	m_nextKeyCode = keyFirstDataCommand;
}

KeyServer::~KeyServer()
{
	(void)Stop(true);
}

status_t 
KeyServer::Start(const char *thread_name, int32 prio)
{
	if (m_thread > 0) return EALREADY;
	if (m_port < 0) return m_port;
	m_thread = spawn_thread(EnterServer, thread_name ? thread_name : "_media_reg_",
			prio, this);
	if (m_thread < 0) return m_thread;
	return resume_thread(m_thread);
}

status_t 
KeyServer::Stop(bool final)
{
	if (m_thread < 0) return B_BAD_THREAD_ID;
	status_t err;
	if (final) {
		err = close_port(m_port);
	}
	else {
		err = write_port(m_port, keyStopServing, NULL, 0);
	}
	if (err < B_OK) return err;
	status_t s;
	err = wait_for_thread(m_thread, &s);
	if (err < B_OK) return err;
	return B_OK;
}

status_t 
KeyServer::EnterServer(void * server)
{
	return reinterpret_cast<KeyServer *>(server)->Server();
}

status_t 
KeyServer::Server()
{
	bool running = true;
	key_reply rep;
	key_request req;
	key_id id;
	void * block = 0;
	size_t size = 0;

	while (running) {
		//	find biggest pending block
		pending_map::iterator pb(m_pending.begin());
		pending_map::iterator cb(m_pending.end());
		while (pb != m_pending.end()) {
			if ((cb == m_pending.end()) || ((*pb).second.second.size() > (*cb).second.second.size())) {
				cb = pb;
			}
			pb++;
		}
		//	if none, set up req
		if (cb != m_pending.end()) {
			block = (*cb).second.second.data();
			size = (*cb).second.second.size();
		}
		else {
			block = &req;
			size = sizeof(req);
		}
		//	get request
		int32 code;
		status_t err = read_port(m_port, &code, block, size);
		if (err == B_INTERRUPTED) continue;
		if (err < B_OK) break;
		if (&req != block) req = *(key_request *)block;
		//	dispatch it
		switch (code) {
		case keyRegistration: {
				rep.registration.key = req.registration.key;
				id.key = req.registration.key;
				strncpy(id.name, req.registration.name, sizeof(id.name));
				id.name[sizeof(id.name)-1] = 0;
				key_map::iterator i(m_keys.find(id));
				if (i != m_keys.end()) {
					code = B_KEY_ALREADY_REGISTERED;
					rep.registration.data_code = 0;
				}
				else {
					//	is there a duplicate among already pending?
					code = B_OK;
					for (pending_map::iterator ptr(m_pending.begin()); ptr != m_pending.end(); ptr++) {
						if ((*ptr).second.first.id == id) {
							code = B_KEY_ALREADY_REGISTERED;
							break;
						}
					}
					if (code == B_OK) {
						//	else insert it
						key_value init;
						init.size = req.registration.size;
						init.type = req.registration.type;
						key_value_ref ref(init);
						if (ref.data() == 0) {
							code = B_NO_MEMORY;
							rep.registration.data_code = 0;
						}
						else {
							rep.registration.data_code = atomic_add(&m_nextKeyCode, 1);
							pending_key pending;
							pending.id = id;
							pending.client = req.registration.client;
							m_pending.insert(pending_map::value_type(rep.registration.data_code,
								pending_map::mapped_type(pending, ref)));
							code = B_OK;
						}
					}
				}
				(void)write_port_etc(req.registration.client, code, &rep.registration, sizeof(rep.registration), B_TIMEOUT, KEY_TIMEOUT);
			}
			break;
		case keyRemoval: {
				rep.removal.key = req.removal.key;
				id.key = req.removal.key;
				strncpy(id.name, req.removal.name, sizeof(id.name));
				id.name[sizeof(id.name)-1] = 0;
				key_map::iterator i(m_keys.find(id));
				if (i == m_keys.end()) {
					code = B_KEY_NOT_FOUND;
				}
				else {
					code = B_OK;
					if (req.removal.remove_all) {
						while ((i != m_keys.end()) && !strcmp((*i).first.name, req.removal.name)) {
							m_keys.erase(i++);
						}
					}
					else {
						m_keys.erase(i);
					}
				}
				(void)write_port_etc(req.removal.client, code, &rep.removal, sizeof(rep.removal), B_TIMEOUT, KEY_TIMEOUT);
			}
			break;
		case keyIteration: {
				rep.iteration.key = req.iteration.key;
				id.key = req.iteration.key;
				strncpy(id.name, req.iteration.name, sizeof(id.name));
				id.name[sizeof(id.name)-1] = 0;
				key_map::iterator i(m_keys.lower_bound(id));
				if ((i != m_keys.end()) && !strcmp((*i).first.name, req.iteration.name)) {
					code = B_OK;
					rep.iteration.key = (*i).first.key;
					rep.iteration.type = (*i).second.type();
					rep.iteration.size = (*i).second.size();
				}
				else {
					code = B_KEY_NOT_FOUND;
				}
				(void)write_port_etc(req.iteration.client, code, &rep.iteration, sizeof(rep.iteration), B_TIMEOUT, KEY_TIMEOUT);
			}
			break;
		case keyRetrieval: {
				rep.retrieval.key = req.retrieval.key;
				id.key = req.retrieval.key;
				strncpy(id.name, req.retrieval.name, sizeof(id.name));
				id.name[sizeof(id.name)-1] = 0;
				key_map::iterator i(m_keys.find(id));
				if (i != m_keys.end()) {
					type_code that_type = (*i).second.type();
					if (req.retrieval.type != that_type) {
						code = B_KEY_WRONG_TYPE;
					}
					else if (req.retrieval.size < (*i).second.size()) {
						code = B_KEY_TOO_SMALL;
					}
					else {
						code = B_OK;
						//	send off the data
						(void)write_port_etc(req.retrieval.client, code, (*i).second.data(), (*i).second.size(), B_TIMEOUT, KEY_TIMEOUT);
					}
				}
				else {
					code = B_KEY_NOT_FOUND;
				}
				if (code != B_OK) {
					(void)write_port_etc(req.retrieval.client, code, &rep.retrieval, sizeof(rep.retrieval), B_TIMEOUT, KEY_TIMEOUT);
				}
			}
			break;
		case keySignoff: {
				//	clear out pending insertions for this client
				pending_map::iterator ptr(m_pending.begin());
				while (ptr != m_pending.end()) {
					if ((*ptr).second.first.client == req.signoff.client) {
						m_pending.erase(ptr++);
					}
					else {
						ptr++;
					}
				}
			}
			break;
		case keyStopServing: {
				//	clear out all pending insertions`
				m_pending.clear();
				running = false;
			}
			break;
		default:
			if (code >= keyFirstDataCommand) {
				//	find the pending key insertion
				pending_map::iterator ptr(m_pending.find(code));
				//	and insert it
				if (ptr != m_pending.end()) {
					if ((*ptr).second.second.data() != block) {
						memcpy((*ptr).second.second.data(), block, err);
					}
					//	make sure this thing isn't already in the list!
					assert(m_keys.find((*ptr).second.first.id) == m_keys.end());
					m_keys.insert(key_map::value_type((*ptr).second.first.id,
							(*ptr).second.second));
					m_pending.erase(ptr);
				}
				else {
					//	bad juju!
				}
			}
			else {
				//	unknown message
			}
			break;
		}
	}
	return B_OK;
}

#endif	//	KEY_SERVER
