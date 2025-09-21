/*	MNotifierManager.cpp	*/

#include "debug.h"
#include "MNotifierManager.h"
#include "ServerApp.h"

#include <stdio.h>
#include <string.h>

#include <list>


MNotifierManager::MNotifierManager() :
	MSavable("MNotifierManager", 0)
{
}


MNotifierManager::~MNotifierManager()
{
}


status_t
MNotifierManager::SaveState()
{
	return B_OK;
}


status_t
MNotifierManager::LoadState()
{
	return B_OK;
}


status_t
MNotifierManager::RegisterNotifier(int32 what, BMessenger msgr, const media_node * node)
{
//	printf("RegisterNotifier('%c%c%c%c', %ld)\n", what>>24, what>>16, what>>8, what, node ? node->node : -1);
	Lock();
	bool regen = false;
	if (node != NULL) {
		node_map::iterator ptr2(mNodeNotifiers.lower_bound(*node));
		node_map::iterator end2(mNodeNotifiers.upper_bound(*node));
		bool found2 = false;
		node_map::mapped_type p(what, msgr);
		while (ptr2 != end2) {
			if ((*ptr2).second == p) {
				found2 = true;
//				printf("already there\n");
				break;
			}
			ptr2++;
		}
		if (!found2) {
			mNodeNotifiers.insert(node_map::value_type(*node, node_map::mapped_type(what, msgr)));
			regen = true;
		}
	}
	else {
		what_map::iterator ptr(mNotifiers.lower_bound(what));
		what_map::iterator end(mNotifiers.upper_bound(what));
		bool found = false;
		while (ptr != end) {
			if ((*ptr).second == msgr) {
				found = true;
				break;
			}
			ptr++;
		}
		if (!found) {
			mNotifiers.insert(what_map::value_type(what,msgr));
		}
	}
	status_t err = B_OK;
	if (regen) {
		err = regen_node_list(*node);
	}
	Unlock();
	return err;
}


status_t
MNotifierManager::UnregisterNotifier(int32 what, BMessenger msgr, const media_node * node)
{
	status_t err = B_BAD_INDEX;
	bool regen = false;

	Lock();
	if (node != NULL) {
		node_map::iterator ptr(mNodeNotifiers.find(*node));
		//	remove this pair of recipients from node's list
		while (ptr != mNodeNotifiers.end()) {
			if ((*ptr).first != *node) {
				break;
			}
			node_map::iterator del(ptr);
			ptr++;
			if ((*del).second.second != msgr) {
				continue;
			}
			if (what == (*del).second.first) {
				regen = true;
				mNodeNotifiers.erase(del);
			}
		}
	}
	else if (what != B_MEDIA_WILDCARD) {
		std::multimap<int32,BMessenger>::iterator ptr(mNotifiers.find(what));
		std::multimap<int32,BMessenger>::iterator end(mNotifiers.end());
		while ((ptr != end) && ((*ptr).first == what)) {
			if ((*ptr).second == msgr) {
				mNotifiers.erase(ptr);
				err = B_OK;
				break;
			};
			ptr++;
		};
	}
	else {
		std::multimap<int32,BMessenger>::iterator ptr(mNotifiers.begin());
		std::multimap<int32,BMessenger>::iterator end(mNotifiers.end());
		while (ptr != end) {
			if ((*ptr).second == msgr) {
				mNotifiers.erase(ptr++);
				err = B_OK;
			}
			else {
				ptr++;
			}
		}
	}
	if (regen) {
		err = regen_node_list(*node);
	}
	Unlock();
	return err;
}


status_t
MNotifierManager::BroadcastMessage(
	BMessage * message,
	bigtime_t timeout)
{
	if (((_ServerApp *)be_app)->IsQuitMode()) {
		return B_OK;
	}

	Lock();

	std::list<std::pair<BMessenger, status_t> > bads;

	int32 what = message->what;
	while (1) {
		std::multimap<int32,BMessenger>::iterator ptr(mNotifiers.find(what));
		std::multimap<int32,BMessenger>::iterator end(mNotifiers.end());
		while ((ptr != end) && ((*ptr).first == what)) {
			status_t err;
			err = (*ptr).second.SendMessage(message, (BHandler*)NULL, timeout);
			if (err < B_OK) bads.push_back(std::pair<BMessenger, status_t>((*ptr).second, err));
			ptr++;
		}
		if (what == B_MEDIA_WILDCARD) break;
		what = B_MEDIA_WILDCARD;
	}
	
	/* handle errors */
	std::list<std::pair<BMessenger, status_t> >::iterator a(bads.begin());
	std::list<std::pair<BMessenger, status_t> >::iterator b(bads.end());
	while (a != b) {
		HandleBroadcastError(message, (*a).first, (*a).second, timeout);
		a++;
	}
	Unlock();
	return B_OK;
}


void
MNotifierManager::HandleBroadcastError(
	BMessage * message,
	BMessenger & msgr,
	status_t err,
	bigtime_t timeout)
{
	debug(1, "Broadcast Error %x msg %.4s team %d\n", err, &message->what, msgr.Team());
	if (err == B_TIMED_OUT) {
		/* try again (once) */
		err = msgr.SendMessage(message, (BHandler *)NULL, timeout);
	}
	if (err >= B_OK)
	{
		debug(2, "Recovery status %x\n", err);
		return;
	}
	/* remove the offending notifier */
	debug(2, "Removing the offender\n");
	UnregisterNotifier(B_MEDIA_WILDCARD,msgr, NULL);
}


status_t 
MNotifierManager::regen_node_list(
	const media_node & node)
{
//	printf("regen_node_list(%d)\n", node.node);
	map<BMessenger, std::list<int32> > watched;
	node_map::iterator ptr = mNodeNotifiers.find(node);
	while (ptr != mNodeNotifiers.end()) {
		if ((*ptr).first != node) {
			break;
		}
		watched[(*ptr).second.second].push_back((*ptr).second.first);
		ptr++;
	}
	//	pack up message
	BMessage msg(MEDIA_SET_NODE_WATCH_LIST);
	int cnt = 0;
	msg.AddData("be:node", B_RAW_TYPE, &node, sizeof(node));
	for (map<BMessenger, std::list<int32> >::iterator ptr(watched.begin());
			ptr != watched.end(); ptr++)
	{
		msg.AddMessenger("be:messenger", (*ptr).first);
		char name[20];
		sprintf(name, "be:what:%d", cnt);
		for (std::list<int32>::iterator p2((*ptr).second.begin());
				p2 != (*ptr).second.end(); p2++)
		{
			msg.AddInt32(name, *p2);
		}
		cnt++;
	}
	//	send it off
	get_messenger_a ans;
	ssize_t r = get_node_messenger(node, &ans);
	if (r == B_OK) {
		r = ans.messenger.SendMessage(&msg, (BHandler *)0, DEFAULT_TIMEOUT);
//		printf("Sending new watch list returns 0x%x (%s)\n", r, strerror(r));
	}
//	else {
//		printf("get_node_messenger(%d) returns 0x%x (%s)\n", node.node, r, strerror(r));
//	}
	return r > 0 ? B_OK : r;
}


status_t
MNotifierManager::get_node_messenger(
	const media_node & node,
	get_messenger_a * ans)
{
	get_messenger_q q;
	q.reply = create_port(1, "get_messenger_a");
	if (q.reply < 0) return q.reply;
	q.cookie = find_thread(NULL);
	ssize_t w = write_port_etc(node.port, M_GET_MESSENGER, &q, sizeof(q), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (w < 0) {
		delete_port(q.reply);
		return w;
	}
	int32 code;
	while ((w = read_port_etc(q.reply, &code, ans, sizeof(*ans), B_TIMEOUT, DEFAULT_TIMEOUT)) < 0) {
		if (w != B_INTERRUPTED) break;
	}
	delete_port(q.reply);
	if (w < 0) {
//		fprintf(stderr,
//				"error getting reply from MNotifierManager::get_node_messenger(%ld)\n",
//				node.node);
		return w;
	}
	if (code != M_GET_MESSENGER_REPLY) {
//		fprintf(stderr,
//				"bad reply code 0x%x in MNotifierManager::get_node_messenger(%ld)\n",
//				code, node.node);
		return B_BAD_VALUE;
	}
//	fprintf(stderr, "Received messenger size %d is team %d (%s)\n", w, ans->messenger.Team(), ans->messenger.IsValid() ? "valid" : "invalid");
//	for (int ix=0; ix<5; ix++) {
//		fprintf(stderr, "0x%04x\n", ((int32 *)&ans->messenger)[ix]);
//	}
	return B_OK;
}
