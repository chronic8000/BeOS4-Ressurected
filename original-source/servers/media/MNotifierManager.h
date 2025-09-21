/*	MNotifierManager.h	*/

#if !defined(M_NOTIFIER_MANAGER_H)
#define M_NOTIFIER_MANAGER_H

#include <Messenger.h>

#include "MSavable.h"
#include "trinity_p.h"

#include <map>


inline bool operator<(
	const BMessenger & a,
	const BMessenger & b)
{
	uint32 * aa = (uint32 *)&a;	/* layout dependent! */
	uint32 * bb = (uint32 *)&b;	/* layout dependent! */
	if (aa[0] != bb[0]) return aa[0] < bb[0];
	if (aa[2] != bb[2]) return aa[2] < bb[2];
	return aa[1] < bb[1];
}

class MNotifierManager :
	public MSavable
{
public:
		MNotifierManager();
		~MNotifierManager();

		status_t SaveState();
		status_t LoadState();

		status_t RegisterNotifier(
				int32 what,
				BMessenger msgr,
				const media_node * node);
		status_t UnregisterNotifier(
				int32 what,
				BMessenger msgr,
				const media_node * node);

		status_t BroadcastMessage(
				BMessage * message,
				bigtime_t timeout = B_INFINITE_TIMEOUT);

inline	void	Lock();
inline	void	Unlock();

private:

		BLocker mLock;
		typedef std::multimap<int32,BMessenger> what_map;
		what_map mNotifiers;
		typedef std::multimap<media_node, pair<int32, BMessenger> > node_map;
		node_map mNodeNotifiers;

		void HandleBroadcastError(
				BMessage * message, 
				BMessenger & msgr, 
				status_t error,
				bigtime_t timeout);

		status_t regen_node_list(
							const media_node & node);
		status_t get_node_messenger(
							const media_node & node,
							get_messenger_a * ans);
};


void MNotifierManager::Lock()
{
	mLock.Lock();
};

void MNotifierManager::Unlock()
{
	mLock.Unlock();
};


#endif /* M_NOTIFIER_MANAGER_H */
