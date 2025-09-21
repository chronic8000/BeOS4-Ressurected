#if !defined(M_SOUND_EVENT_MANAGER_H)
#define M_SOUND_EVENT_MANAGER_H

#include "trinity_p.h"
#include "str.h"
#include <map>
#include <vector>
#include <Locker.h>
#include <OS.h>

class MSoundEventManager
{
public:
	// minimum chunk (in bytes) to load/play at a time
	static const size_t		minPageSize;
	// number of pages to buffer if playing from disk
	static const size_t		loaderThreadPages;
	// number of pages to lock in memory for quick restart
	static const size_t		lockablePages;
	// maximum size of file to completely preload
	static const size_t		maxPreloadSize;
	// fade-out period when stopping or restarting
	static const size_t		fadeFrames;

public:
	MSoundEventManager();
	~MSoundEventManager();

	// a play_sound()-compatible semaphore is created for each sound instance
	// +em: additional (relative) gain control for events?
	status_t Play(const char* event, sem_id* outHandle, bool persist);
	status_t Play(const entry_ref& file, float gain, sem_id* outHandle);
	status_t Stop(sem_id handle);

	// preroll the given event (and make it persistent)
	status_t Preroll(const char* event);

	void EventChanged(const char* event);
	
private:
	class SoundInstance;
	friend class MSoundEventManager::SoundInstance;

	void InstanceDone(SoundInstance* instance);
	
	BLocker					_lock;
	thread_id				_reapThread;
	port_id					_reapPort;

	typedef std::map<sem_id, SoundInstance*> instance_map;
	instance_map			_instances;

	struct event_state
	{
		event_state() {}
		event_state(const char* n, SoundInstance* i)
		{
			strncpy(name, n, _BMediaRosterP::B_MEDIA_ITEM_NAME_LENGTH-1);
			instance = i;
		}
		char name[_BMediaRosterP::B_MEDIA_ITEM_NAME_LENGTH];
		SoundInstance* instance;
	};
	typedef std::vector<event_state> event_vector;
	event_vector			_events;	

	static status_t ReapThreadEntry(void* cookie);
	void ReapThread();

	// internal API (lock required)
	status_t AddInstance(SoundInstance* instance);
	status_t AddEventInstance(const char* event, SoundInstance* instance);
	instance_map::iterator FindInstance(const char* event);
	void RemoveInstance(const instance_map::iterator& it);
};

#endif // M_SOUND_EVENT_MANAGER_H
