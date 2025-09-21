/*	MAppManager.h	*/

#if !defined(M_APP_MANAGER_H)
#define M_APP_MANAGER_H

#include "MSavable.h"
#if !defined(_OS_H)
#include <OS.h>
#endif /* _OS_H */
#if !defined(_MESSENGER_H)
#include <Messenger.h>
#endif /* _MESSENGER_H */
#include <Locker.h>

#include <map>


class BMessenger;
class BMessage;

class MAppManager :
	public MSavable
{
public:
		MAppManager();
		~MAppManager();

		status_t	SaveState();
		status_t	LoadState();

inline	void		Lock();
inline	void		Unlock();

		int32		CountTeams();
		team_id		GetNthTeam(int32 n);
		bool		HasTeam(
						team_id team);

		status_t	RegisterTeam(
						team_id team,
						BMessenger msgr);
		status_t	UnregisterTeam(
						team_id team);

		status_t	BroadcastMessage(
						BMessage * message,
						bigtime_t timeout);

private:

		BLocker		mLock;
#if 0
		int32 		mTeamLinearListSize;
		team_id *	mTeamLinearList;
#endif
		std::map<team_id, BMessenger> mTeams;

#if 0
		void UpdateLinearList();
#endif
		void HandleBroadcastError(
				BMessage * message,
				BMessenger & msgr,
				status_t err,
				bigtime_t timeout);
};

void
MAppManager::Lock()
{
	mLock.Lock();
}

void
MAppManager::Unlock()
{
	mLock.Unlock();
}

#endif /* M_APP_MANAGER_H */
