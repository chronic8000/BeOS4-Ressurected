/*	MAppManager.cpp	*/


#include "MAppManager.h"
#include "trinity_p.h"
#include "Autolock.h"

#include <list>

extern void debug(int detail, const char * fmt, ...);


MAppManager::MAppManager() :
	MSavable("MAppManager", 0)
{
}


MAppManager::~MAppManager()
{
}


status_t
MAppManager::SaveState()
{
	return B_OK;
}


status_t
MAppManager::LoadState()
{
	return B_OK;
}

bool
MAppManager::HasTeam(
	team_id team)
{
	return (mTeams.find(team) != mTeams.end());
}

status_t
MAppManager::RegisterTeam(
	team_id team,
	BMessenger roster)
{
	Lock();
	if (mTeams.find(team) != mTeams.end())
	{
		Unlock();
		return B_MEDIA_APP_ALREADY_REGISTERED;
	}
	mTeams[team] = roster;
	Unlock();
	return B_OK;
}


status_t
MAppManager::UnregisterTeam(
	team_id team)
{
	Lock();
	std::map<team_id, BMessenger>::iterator iter(mTeams.find(team));
	if (iter == mTeams.end())
	{
		Unlock();
		return B_MEDIA_APP_NOT_REGISTERED;
	}
	mTeams.erase(iter);
	Unlock();
	return B_OK;
}


status_t
MAppManager::BroadcastMessage(
	BMessage * message,
	bigtime_t timeout)
{
	Lock();

	std::map<team_id, BMessenger>::iterator ptr(mTeams.begin());
	std::map<team_id, BMessenger>::iterator end(mTeams.end());
	std::list<std::pair<BMessenger, status_t> > bads;

	while (ptr != end)
	{
		status_t err;
		err = (*ptr).second.SendMessage(message, (BHandler*)NULL, timeout);
		if (err < B_OK)
		{
			bads.push_back(std::pair<BMessenger, status_t>((*ptr).second, err));
		}
		ptr++;
	}
	/* handle errors */
	std::list<std::pair<BMessenger, status_t> >::iterator a(bads.begin());
	std::list<std::pair<BMessenger, status_t> >::iterator b(bads.end());
	while (a != b)
	{
		HandleBroadcastError(message, (*a).first, (*a).second, timeout);
		a++;
	}

	Unlock();
	return B_OK;
}


void
MAppManager::HandleBroadcastError(
	BMessage * message,
	BMessenger & msgr,
	status_t err,
	bigtime_t timeout)
{
	team_id team = msgr.Team();
	debug(1, "APPMANAGER: Broadcast Error %x msg %.4s team %d\n", err, &message->what, team);
	if (err == B_TIMED_OUT)
	{
		/* try again (once) */
		err = msgr.SendMessage(message, (BHandler *)NULL, timeout);
	}
	if (err >= B_OK)
	{
		debug(2, "Recovery status %x\n", err);
		return;
	}
	/* the offender will stay in the loop if the error was timeout */
/*
	// This app will get removed by the dirty_work troller, which will do cleanup.
	if (err < B_OK && err != B_TIMED_OUT)
	{
		debug(1, "Server drops team %d\n", team);
		mTeams.erase(team);
	}
*/
}
