/*	MDefaultManager.cpp	*/


#include "MDefaultManager.h"

#include <File.h>

extern void debug(int detail, const char * fmt, ...);

MDefaultManager::MDefaultManager() :
	MSavable("MDefaultManager", 0),
	mRealtimeFlags(0)
{
}


MDefaultManager::~MDefaultManager()
{
}


status_t
MDefaultManager::SaveState()
{
	status_t err = B_OK;
	BFile * f = OpenFile();
	if (f == NULL)
		return B_ERROR;

	StDelete<BFile> d(f);

	err = f->SetSize(0);
	if (err < 0) return err;
	int64 magic = 0x18723462ab00150bLL;
	err = f->Write(&magic, sizeof(magic));
	if (err < 0) return err;
	int32 version = 2;
	err = f->Write(&version, sizeof(version));
	if (err < 0) return err;
	int32 count = mDefaults.size();
	err = f->Write(&count, sizeof(count));
	if (err < 0) return err;
	int32 separator = 'sepx';
	for (
		std::map<media_node_id, BMessage>::iterator ptr = mDefaults.begin();
		ptr != mDefaults.end();
		ptr++)
	{
		err = f->Write(&separator, sizeof(separator));
		if (err < 0) return err;
		err = f->Write(&(*ptr).first, sizeof((*ptr).first));
		if (err < 0) return err;
		err = (*ptr).second.Flatten(f);
		if (err < 0) return err;
	}
	int32 terminator = 'term';
	err = f->Write(&terminator, sizeof(terminator));
	if (err < 0)
		return err;

	//	BIG FAT WARNING:
	//	The realtime flags always have to be the VERY LAST
	//	four bytes of the file. No exceptions. They're gotten
	//	early in initialization of global objects by
	//	rtm_create_pool() in libgamesound and libmedia.
	terminator = 'mflg';
	err = f->Write(&terminator, sizeof(terminator));
	if (err < 0) return err;
	err = f->Write(&mRealtimeFlags, sizeof(mRealtimeFlags));
	if (err < 0)
		return err;
	
	f->Sync();
	SetDirty(false);
	return B_OK;
}

status_t
MDefaultManager::LoadState()
{
	BFile * f = OpenFile();
	if (!f) return B_ERROR;

	StDelete<BFile> d(f);

	int64 magic;
	status_t err = f->Read(&magic, sizeof(magic));
	if (err < 0) return err;
	if (magic != 0x18723462ab00150bLL) return B_ERROR;
	int32 version;
	err = f->Read(&version, sizeof(version));
	if (err < 0) return err;
	if (version < 1 || version > 2) return B_ERROR;
	int32 count = 0;
	err = f->Read(&count, sizeof(count));
	if (err < 0) return err;
	/* so we now have a reasonable idea that the file is OK */
	mDefaults.erase(mDefaults.begin(), mDefaults.end());
	for (int ix=0; ix<count; ix++)
	{
		int32 separator;
		err = f->Read(&separator, sizeof(separator));
		if (err < 0) return err;
		if (separator != 'sepx') return B_ERROR;
		media_node_id node;
		err = f->Read(&node, sizeof(node));
		if (err < 0) return err;
		BMessage msg;
		err = msg.Unflatten(f);
		if (err < 0) return err;
		mDefaults[node] = msg;
	}
	int32 terminator;
	err = f->Read(&terminator, sizeof(terminator));
	if (err < 0) return err;
	if (terminator != 'term') return B_ERROR;
	/* now we have a reasonable idea that this file contained what it was supposed to */

	bool rtf_set = false;
	if (version > 1) {
		//	check marker for settings flags
		err = f->Read(&terminator, sizeof(terminator));
		if (err < 4) return err < B_OK ? err : B_ERROR;
		if (terminator != 'mflg') return B_ERROR;
		//	these settings flags are always last
		err = f->Read(&mRealtimeFlags, sizeof(mRealtimeFlags));
		if (err < 4) return (err < B_OK) ? err : B_ERROR;
		rtf_set = true;
	}
	if (!rtf_set) {
		system_info info;
		get_system_info(&info);
		if (info.max_pages*B_PAGE_SIZE >= 63*1024*1024L) {
			mRealtimeFlags = B_MEDIA_REALTIME_ALLOCATOR | B_MEDIA_REALTIME_AUDIO;
		}
		else {
			mRealtimeFlags = 0;
		}
	}

	return B_OK;
}


status_t
MDefaultManager::SetDefault(
	media_node_id for_what,
	BMessage & info)
{
	SetDirty(true);
	mDefaults[for_what].MakeEmpty();
	mDefaults[for_what] = info;
	return B_OK;
}


status_t
MDefaultManager::GetDefault(
	media_node_id for_what,
	BMessage & info)
{
	std::map<media_node_id, BMessage>::iterator ptr = mDefaults.find(for_what);
	if (ptr == mDefaults.end())	{
		return B_MEDIA_BAD_NODE;
	}
	info = (*ptr).second;
	return B_OK;
}


status_t
MDefaultManager::RemoveDefault(
	media_node_id node)
{
	std::map<media_node_id, BMessage>::iterator ptr = mDefaults.find(node);
	if (ptr == mDefaults.end())
	{
		return B_MEDIA_BAD_NODE;
	}
	SetDirty(true);
	mDefaults.erase(node);
	return B_OK;
}


status_t
MDefaultManager::SetRunningDefault(
	media_node_id what,
	const media_node & node)
{
	debug(2, "Set running default for %d to %d\n", what, node.node);
	mRunning[what] = node;
	return B_OK;
}


status_t
MDefaultManager::GetRunningDefault(
	media_node_id what,
	media_node & node)
{
	debug(2, "MDefaultManager::GetRunningDefault(%d)\n", what);
	std::map<media_node_id, media_node>::iterator ptr(mRunning.find(what));
	if (ptr == mRunning.end()) {
		debug(1, "GetRunningDefault(%d) Failed\n", what);
		return B_MEDIA_BAD_NODE;
	}

	node = (*ptr).second;
	debug(2, "GetRunningDefault Got %d\n", node.node);
	return B_OK;
}


status_t
MDefaultManager::RemoveRunningDefault(
	const media_node & node)
{
	debug(2, "MDefaultManager::RemoveRunningDefault(%d)\n", node.node);
	status_t err = B_MEDIA_BAD_NODE;
	for (std::map<media_node_id, media_node>::iterator ptr(mRunning.begin()); ptr != mRunning.end();) {
		if ((*ptr).second == node) {
			std::map<media_node_id, media_node>::iterator del(ptr);
			ptr++;
			mRunning.erase(del);
			err = B_OK;
		}
		else {
			ptr++;
		}
	}
	return err;
}

uint32 
MDefaultManager::GetRealtimeFlags()
{
	return mRealtimeFlags;
}

void 
MDefaultManager::SetRealtimeFlags(uint32 flags)
{
	mRealtimeFlags = flags;
	SetDirty(true);
	(void)SaveState();	//	force immediate write
}

