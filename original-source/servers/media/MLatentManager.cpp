/*	MLatentManager.cpp	*/

/*** We must somehow remember add-ons across invocations; at least ***/
/*** for enable info. Would be cool to do configuration as well. ***/


#include "MLatentManager.h"
#include <Application.h>
#include <NodeMonitor.h>
#include <Locker.h>
#include <Autolock.h>
#include <trinity_p.h>
#include <File.h>
#include <stdio.h>
#include <string.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <Node.h>
#include <Mime.h>
#include <set>

#include <driver_settings_p.h>	//	for safe mode

#include "trinity_p.h"

#if NDEBUG
#define FPRINTF
#else
#define FPRINTF fprintf
#endif

#define FPrintf fprintf

#define IMAGES FPRINTF


extern int debug_level;

/* When do we emit an advisory, and when do we stop auto_starting an add-on? */
#define SUSPICION_COUNT 19		/* advise when add-on starts >= 49 nodes */
#define ABANDON_COUNT 49		/* give up when add-on starts >= 119 nodes */


const char *kSystemAddOnDir = "/system/add-ons/media";
const char *kUserAddOnDir = "/boot/home/config/add-ons/media";

addon_info::addon_info()
{
	enabled = true;
}

addon_info::~addon_info()
{
}

addon_flavor_info::addon_flavor_info()
{
	enabled = true;
}

addon_flavor_info::~addon_flavor_info()
{
}



static flavor_info & assign_flavor(flavor_info & a, const flavor_info & i);
static void dispose_flavor(flavor_info & a); /* only call this if initialized with operator=()! */

flavor_info &
assign_flavor(flavor_info & a, const flavor_info & i)
{
	a.internal_id = i.internal_id;
	a.name = strdup(i.name);
	a.info = strdup(i.info);
	a.kinds = i.kinds;
	a.flavor_flags = i.flavor_flags;
	a.possible_count = i.possible_count;
	a.in_format_count = i.in_format_count;
	ASSERT(!(i.in_formats && !i.in_format_count));
	a.in_formats = a.in_format_count ? new media_format[a.in_format_count] : NULL;
	if (a.in_formats) {
		memcpy((void *)a.in_formats, i.in_formats, sizeof(media_format)*a.in_format_count);
	}
	a.out_format_count = i.out_format_count;
	ASSERT(!(i.out_formats && !i.out_format_count));
	a.out_formats = a.out_format_count ? new media_format[a.out_format_count] : NULL;
	if (a.out_formats) {
		memcpy((void *)a.out_formats, i.out_formats, sizeof(media_format)*a.out_format_count);
	}
	memcpy(a._reserved_, i._reserved_, sizeof(a._reserved_));
	return a;
}


void
dispose_flavor(
	flavor_info & a)
{
	free((void*)a.name);
	free((void*)a.info);
	delete[] a.in_formats;
	delete[] a.out_formats;
}




#pragma mark --- flavor_info ---


x_flavor_info &
x_flavor_info::operator=(const flavor_info & i)
{
	enabled = true;
	assign_flavor(*this, i);
	return *this;
}


x_flavor_info &
x_flavor_info::operator=(const x_flavor_info & i)
{
	enabled = i.enabled;
	assign_flavor(*this, i);
	return *this;
}






#pragma mark --- latent_info ---




bool operator==(const addon_ref & a, const addon_ref & b)
{
	return a.path == b.path;
}

bool operator<(const addon_ref & a, const addon_ref & b)
{
	return a.path < b.path;
}

bool operator==(const addon_flavor_ref & a, const addon_flavor_ref & b)
{
	return (a.path == b.path) && (a.id == b.id);
}

bool operator<(const addon_flavor_ref & a, const addon_flavor_ref & b)
{
	if (a.path < b.path) return true;
	if ((a.path == b.path) && (a.id < b.id)) return true;
	return false;
}


addon_ref::addon_ref()
{
}

addon_ref::addon_ref(const addon_ref & r) :
	path(r.path)
{
}

addon_ref::addon_ref(const char * str) :
	path(str)
{
}

addon_ref::~addon_ref()
{
}

addon_ref & addon_ref::operator=(const addon_ref & ref)
{
	path = ref.path;
	return *this;
}

addon_flavor_ref::addon_flavor_ref()
{
	id = 0;
}

addon_flavor_ref::addon_flavor_ref(const addon_flavor_ref & r) :
	path(r.path)
{
}

addon_flavor_ref::addon_flavor_ref(str_ref & p, int32 i) :
	path(p)
{
	id = i;
}

addon_flavor_ref::~addon_flavor_ref()
{
}

addon_flavor_ref & addon_flavor_ref::operator=(const addon_flavor_ref & ref)
{
	path = ref.path;
	id = ref.id;
	return *this;
}




latent_info::latent_info(
	bool e,
	bool f,
	image_id l,
	BMediaAddOn * a,
	int32 c,
	const char * p,
	media_addon_id maid)
{
	enabled = e;
	auto_start = f;
	loaded = l;
	IMAGES(stderr, "%s: loaded = %d\n", p ? p : "(null)", loaded);
	add_on = a;
	flavors = new std::vector<x_flavor_info>(c);
	path = p;
//FPrintf(stderr, "refcount for %s is now %d\n", path.str(), *ref_count);
	id = maid;
	memset(&(*flavors)[0], 0, c*sizeof(x_flavor_info));
	ref_count = new int32(1);
}


latent_info::latent_info()
{
	ref_count = new int32(1);
	flavors = NULL;
	enabled = false;
	loaded = -1;
	IMAGES(stderr, "NULL: loaded = -1\n");
	add_on = NULL;
	id = -1;
//	path = NULL;
}


latent_info::~latent_info()
{
	if (atomic_add(ref_count, -1) == 1) {
//FPrintf(stderr, "refcount for %s is now %d\n", path.str(), *ref_count);
		delete flavors;
		delete ref_count;
		delete add_on;
		if (loaded >= 0) unload_add_on(loaded);	/* may return error if AddOn already unloads */
		IMAGES(stderr, "unloaded %d\n", loaded);
//		free(path);
	}
	else {
//FPrintf(stderr, "refcount for %s is now %d\n", path.str(), *ref_count);
	}
}


latent_info::latent_info(const latent_info & i) :
	path(i.path)
{
	ref_count = i.ref_count;
	atomic_add(ref_count, 1);
//FPrintf(stderr, "refcount for %s is now %d\n", i.path.str(), *ref_count);
	flavors = i.flavors;
	enabled = i.enabled;
	auto_start = i.auto_start;
	add_on = i.add_on;
	loaded = i.loaded;
	IMAGES(stderr, "%s: loaded = %d\n", path.str(), loaded);
	id = i.id;
//	path = i.path;
}


latent_info &
latent_info::operator=(const latent_info & i)
{
	if (i.flavors == flavors)
		/* do nothing */;
	else {
		if (atomic_add(ref_count, -1) == 1) {
//FPrintf(stderr, "refcount for %s is now %d\n", path.str(), *ref_count);
			delete flavors;
			delete ref_count;
			delete add_on;
			if (loaded >= 0) unload_add_on(loaded);	/* may return error if AddOn already unloads */
			IMAGES(stderr, "%s: unloaded %d\n", path.str(), loaded);
//			free(path);
		}
		ref_count = i.ref_count;
		atomic_add(ref_count, 1);
//FPrintf(stderr, "refcount for %s is now %d\n", i.path.str(), *ref_count);
		flavors = i.flavors;
		enabled = i.enabled;
		auto_start = i.auto_start;
		loaded = i.loaded;
		IMAGES(stderr, "%s: loaded = %d\n", i.path.str(), loaded);
		add_on = i.add_on;
		path = i.path;
		id = i.id;
	}
	return *this;
}

void
latent_info::clear_flavors()
{
	(*flavors).clear();
}

bool
latent_info::operator==(const latent_info & i)
{
	/* the ref_count pointer is used as identity token */
	return (i.ref_count == ref_count);
}


bool
latent_info::operator<(const latent_info & i)
{
	return path < i.path;
}







#pragma mark --- MLatentManager ---




MLatentManager::MLatentManager() :
	MSavable("MLatentManager", 0),
	BHandler("addon watcher")
{
	fAddonID = 0;
	be_app->AddHandler(this);
}


MLatentManager::~MLatentManager()
{
}


const char *
MLatentManager::FindFlavorName(
	const char * path,
	int32 internal_id)
{
	BAutolock _lock(mLock);

	LatentMap::iterator ptr(mLatents.begin());
	while (ptr != mLatents.end()) {
		const char * lp = (*ptr).second.path.str();
		if (lp && !strcmp(path, lp)) {
			for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
				if ((*(*ptr).second.flavors)[ix].internal_id == internal_id)
					return (*(*ptr).second.flavors)[ix].name;
			}
		}
	}
	return NULL;
}

void 
MLatentManager::RescanFlavors(media_addon_id id)
{
	BAutolock _lock(mLock);

	LatentMap::iterator ptr(mLatents.find(id));
	if (ptr == mLatents.end()) {
		return;
	}
	latent_info info((*ptr).second);
	info.clear_flavors();
	int count = info.add_on->CountFlavors();
	status_t err = B_OK;
	for (int ix=0; ix<count; ix++) {
		const flavor_info * flavor;
		err = info.add_on->GetFlavorAt(ix, &flavor);
		if (err == B_OK) {
			x_flavor_info x;
			x = *flavor;
			(*info.flavors).push_back(x);
			GetToKnowFlavor((*info.flavors)[ix], info.path);
		}
		else
			break;
	}
	mLatents[id] = info;
}


void 
MLatentManager::MessageReceived(BMessage *message)
{
	if (message->what == B_NODE_MONITOR)
		HandleNodeMonitor(message);
	else
		BHandler::MessageReceived(message);
}

status_t
MLatentManager::SaveState()
{
	BAutolock _lock(mLock);

	status_t err = B_OK;
	BFile * f = OpenFile();
	if (f == NULL)
		return B_ERROR;

	StDelete<BFile> d(f);

	/* Performance: temporarily write to memory before writing it all to disk */
	BMallocIO io;

	err = io.SetSize(0);
	if (err < 0) return err;
	int64 magic = 0x18743462ab00150bLL;
	err = io.Write(&magic, sizeof(magic));
	if (err < 0) return err;
	int32 version = 1;
	err = io.Write(&version, sizeof(version));
	if (err < 0) return err;
	int32 count = mLatents.size();
	err = io.Write(&count, sizeof(count));
	if (err < 0) return err;
	int32 separator = 'sepx';
	for (
		std::map<media_addon_id, latent_info>::iterator ptr = mLatents.begin();
		ptr != mLatents.end();
		ptr++)
	{
		err = io.Write(&separator, sizeof(separator));
		if (err < sizeof(separator)) return err < 0 ? err : B_ERROR;
		int32 len = (*ptr).second.path.length();
		err = io.Write(&len, sizeof(len));
		if (err < sizeof(len)) return err < 0 ? err : B_ERROR;
		err = io.Write((*ptr).second.path.str(), len);
		if (err < len) return err < 0 ? err : B_ERROR;
		err = io.Write(&(*ptr).second.enabled, sizeof(bool));
		if (err < sizeof(bool)) return err < 0 ? err : B_ERROR;
		int32 a_count = (*ptr).second.flavors->size();
		err = io.Write(&a_count, sizeof(a_count));
		if (err < sizeof(a_count)) return err < 0 ? err : B_ERROR;
		for (int ix=0; ix<(*ptr).second.flavors->size(); ix++)
		{
			if ((err = io.Write(&(*(*ptr).second.flavors)[ix].internal_id, sizeof(int32))) < sizeof(int32))
				return err < 0 ? err : B_ERROR;
			if ((err = io.Write(&(*(*ptr).second.flavors)[ix].enabled, sizeof(bool))) < sizeof(bool))
				return err < 0 ? err : B_ERROR;
		}
		if (err < 0) return err;
	}
	int32 terminator = 'term';
	err = io.Write(&terminator, sizeof(terminator));
	if (err < sizeof(terminator)) return err < 0 ? err : B_ERROR;
	err = f->WriteAt(0, io.Buffer(), io.Position());
	if (err < io.Position()) return err < 0 ? err : B_ERROR;
	err = f->SetSize(io.Position());
	if (err < B_OK) return err;
	f->Sync();
	return B_OK;
}


status_t
MLatentManager::LoadState()
{
	BAutolock _lock(mLock);

	BFile * f = OpenFile();
	if (f == NULL) return B_ERROR;

	StDelete<BFile> d(f);

	f->Seek(0, SEEK_END);
	off_t file_size = f->Position();
	if (file_size > 0x100000) return B_BAD_VALUE;
	char * data = (char *)malloc(file_size);
	if (!data) return B_NO_MEMORY;

	BMemoryIO io(data, file_size);

	status_t err = f->ReadAt(0, data, file_size);
	if (err < file_size) return err < 0 ? err : B_ERROR;
	int64 magic;
	err = io.Read(&magic, sizeof(magic));
	if (err < sizeof(magic)) return err < 0 ? err : B_ERROR;
	if (magic != 0x18743462ab00150bLL) return B_ERROR;
	int32 version;
	err = io.Read(&version, sizeof(version));
	if (err < sizeof(version)) return err < 0 ? err : B_ERROR;
	if (version < 1 || version > 1) return B_ERROR;
	int32 count = 0;
	err = io.Read(&count, sizeof(count));
	if (err < sizeof(count)) return err < 0 ? err : B_ERROR;
	/* so we now have a reasonable idea that the file is OK */
	mAddOnInfo.clear();
	mAddOnFlavorInfo.clear();
	for (int ix=0; ix<count; ix++)
	{
		int32 separator;
		err = io.Read(&separator, sizeof(separator));
		if (err < sizeof(separator)) return err < 0 ? err : B_ERROR;
		if (separator != 'sepx') return B_ERROR;
		int32 len = 0;
		err = io.Read(&len, sizeof(len));
		if (err < sizeof(len)) return err < 0 ? err : B_ERROR;
		char str[1024];
		err = io.Read(str, len);
		if (err < len) return err < 0 ? err : B_ERROR;
		str[len] = 0;
		bool e;
		err = io.Read(&e, sizeof(e));
		if (err < sizeof(e)) return err < 0 ? err : B_ERROR;
		addon_ref a_r(str);
		addon_info a_i;
		a_i.enabled = e;
		mAddOnInfo[a_r] = a_i;	/* this line causes a warning */
		int32 count2 = 0;
		err = io.Read(&count2, sizeof(count2));
		if (err < sizeof(int32)) return err < 0 ? err : B_ERROR;
		for (int iy=0; iy<count2; iy++)
		{
			int32 id;
			err = io.Read(&id, sizeof(id));
			if (err < sizeof(int32)) return err < 0 ? err : B_ERROR;
			err = io.Read(&e, sizeof(e));
			if (err < sizeof(bool)) return err < 0 ? err : B_ERROR;
			addon_flavor_ref a_f_r(a_r.path, id);
			addon_flavor_info a_f_i;
			a_f_i.enabled = e;
			mAddOnFlavorInfo[a_f_r] = a_f_i;	/* this line causes a warning */
		}
		// mLatents[node] = msg;
	}
	int32 terminator;
	err = io.Read(&terminator, sizeof(terminator));
	if (err < 0) return err < 0 ? err : B_ERROR;
	if (terminator != 'term') return B_ERROR;
	/* now we have a reasonable idea that this file contained what it was supposed to */
	return B_OK;
}


//	secret call

extern "C" status_t _kget_safemode_option_(const char * name, char * data, uint32 * size);

status_t
MLatentManager::DiscoverAddOns()
{
	BAutolock _lock(mLock);

	entry_ref ref;
	status_t accum = B_OK;
	status_t err;
	char data = 0;
	uint32 len = sizeof(data);

	//	check for disabled user add-ons
	if (_kget_safemode_option_(SAFEMODE_DISABLE_USER_ADDONS, &data, &len)) {
		data = 0;
	} else {
		data = 1;
	}
	if (!data && (B_OK == get_ref_for_path(kUserAddOnDir, &ref)))
		err = LoadDirectory(ref, true);
	if (err) accum = err;
	if (B_OK == get_ref_for_path(kSystemAddOnDir, &ref))
		err = LoadDirectory(ref, true);
	if (err) accum = err;
	return accum;
}


static bool
ok_to_recurse(const char * name)
{
	if (!name) return false;
	int len = strlen(name);
	if (len < 1) return false;
	if ((name[0] == '(') && (name[len-1] == ')')) return false;
	static const char * forbidden[] = {
		"decoders", "encoders", "extractors", "writers"
	};
	for (int ix=0; ix<sizeof(forbidden)/sizeof(forbidden[0]); ix++) {
		if (!strcasecmp(forbidden[ix], name)) return false;
	}
	return true;	//	so it's OK
}


status_t
MLatentManager::LoadDirectory(
	entry_ref & directory,
	bool recursive)
{
	BAutolock _lock(mLock);

	BDirectory dir(&directory);
	if (dir.InitCheck() != B_OK)
		return dir.InitCheck();

	node_ref dirNode;
	if (dir.GetNodeRef(&dirNode) == B_OK) {
		status_t err = watch_node(&dirNode, B_WATCH_DIRECTORY, this);
		if (err != B_OK) 
			FPrintf(stderr, "LoadDirectory(): Error %s watching node\n", strerror(err));
	}	
	
	BEntry ent;
	BPath path;
	status_t err2 = B_OK, err;
	while (dir.GetNextEntry(&ent) == B_OK) {
		if (ent.IsDirectory()) {
			entry_ref ref;
			if (recursive && !ent.GetRef(&ref) && ok_to_recurse(ref.name)) {
				err = LoadDirectory(ref, recursive);
			}
		}
		else {
			if (B_OK <= ent.GetPath(&path)) {
				if (debug_level) {
					FPrintf(stderr, "addon_host: trying load_add_on(%s)\n", path.Path());
				}
				err = LoadAddOn(path.Path());
				dlog("load_add_on(%s) returns %x (%s)", path.Leaf(), err, strerror(err));
				if (debug_level) {
					FPrintf(stderr, "addon_host: load_add_on(%s) returns %x (%s)\n", path.Leaf(), err, strerror(err));
				}
			}
			else {
				if (debug_level) {
					FPrintf(stderr, "addon_host: cannot get path for ref in %s\n", directory.name);
				}
			}
		}
		if (err) err2 = err;
	}
	return err2;
}


extern "C" {
	typedef BMediaAddOn * (*make_media_addon_func)(image_id you);
}


void
MLatentManager::GetToKnowFlavor(
	x_flavor_info & info,
	str_ref & path)
{
	BAutolock _lock(mLock);

	addon_flavor_ref a_f_r(path, info.internal_id);
	std::map<addon_flavor_ref, addon_flavor_info>::iterator ptr(mAddOnFlavorInfo.find(a_f_r));
	if (ptr == mAddOnFlavorInfo.end()) {
		info.enabled = true;
	}
	else {
		info.enabled = (*ptr).second.enabled;
	}
}


/*** Problem: ***/
/*** We can't remember the disable state of specific flavors within a disabled add-on ***/
/*** unless we load that add-on and ask it for flavors. But disabling an add-on should ***/
/*** really disable it and not call any code within it, so we currently live with this ***/
/*** limitation. ***/
status_t
MLatentManager::LoadAddOn(
	const char * path)
{
	BAutolock _lock(mLock);

	addon_ref a_r(path);
	std::map<addon_ref, addon_info>::iterator ptr(mAddOnInfo.find(a_r));
	if ((ptr != mAddOnInfo.end()) && ((*ptr).second.enabled == false)) {
		/* insert latent_info for disabled item */
		media_addon_id id = ++fAddonID;
		latent_info l_info(false, false, -1, NULL, 0, path, id);
		mLatents[id] = l_info;
		/* don't load disabled add-on, but pretend everything's OK */
		dlog("Add-on is disabled");
		return B_OK;
	}
	image_id loaded = load_add_on(path);
	if (loaded < 0) return loaded;

	node_ref imgnode;
	if (BNode(path).GetNodeRef(&imgnode) == B_OK) {
		status_t err = watch_node(&imgnode, B_WATCH_STAT, this);
		if (err != B_OK) 
			FPrintf(stderr, "LoadAddOn: Error %s watching node\n", strerror(err));
	}

	IMAGES(stderr, "loaded add-on %s: %d\n", path, loaded);
	BMediaAddOn * addon = NULL;
	make_media_addon_func make_media_addon;
	status_t err = get_image_symbol(loaded, "make_media_addon", B_SYMBOL_TYPE_ANY, (void **)&make_media_addon);
	const flavor_info * flavor = NULL;
	int32 count;
	if (err < 0) goto bad;
	addon = (*make_media_addon)(loaded);
	if (!addon) {
		err = B_MEDIA_ADDON_FAILED;
		goto bad;
	}
	addon->SetOwner(mNotifyHook, mNotifyCookie);
	count = addon->CountFlavors();
	if (count <= 0) {
		const char * error = NULL;
		err = addon->InitCheck(&error);
		if (error != NULL) dlog("addon_host error: %s", error);
		goto bad;
	}
	{
		int32 id = ++fAddonID;
		addon->_m_addon = id;
#if 0	//	disable autostart
		latent_info info(true, false, loaded, addon, count, path, id);
#else
		latent_info info(true, addon->WantsAutoStart(), loaded, addon, count, path, id);
#endif
		for (int ix=0; ix<count; ix++)
		{
			err = addon->GetFlavorAt(ix, &flavor);
			if (err == B_OK)
			{
				(*info.flavors)[ix] = *flavor;
				GetToKnowFlavor((*info.flavors)[ix], a_r.path);
			}
			else
				goto bad;
		}
		mLatents[id] = info;
		dlog("Loaded add-on %.42s (#%d)", path, mLatents.size());
		/* Yoo-hoo! We're done. */
	}
bad:
	if (err < 0) {
		if (addon != NULL) delete addon;
		if (loaded >= 0) unload_add_on(loaded);
		IMAGES(stderr, "failure, unload: %d", loaded);
	}
	return err;
}

status_t 
MLatentManager::UnloadAddOn(media_addon_id id)
{
	BAutolock _lock(mLock);

	LatentMap::iterator i = mLatents.find(id);
	if (i != mLatents.end()) {
		if (debug_level > 1) ("Unloading add on\n");

		// Node this should check some kind of ref count to make sure
		// there are no instances of this node...
		if ((*i).second.loaded > 0)
			unload_add_on((*i).second.loaded);
		
		delete (*i).second.add_on;
		
		// mAddOnInfo
		// mAddOnFlavorInfo
		mLatents.erase(i);
		return B_OK;
	}

	return B_ERROR;	
}




status_t
MLatentManager::GetAddonID(
	const char * path,
	media_addon_id * out_id)
{
	BAutolock _lock(mLock);

	/*** Linear search -- we might do better later by using a reverse index ***/
	std::map<media_addon_id, latent_info>::iterator ptr(mLatents.begin());
	while (ptr != mLatents.end()) {
		if ((*ptr).second.path == path) {
			*out_id = (*ptr).first;
			return B_OK;
		}
		ptr++;
	}
	return B_ERROR;
}


status_t
MLatentManager::FindLatent(
	media_addon_id in_id,
	latent_info ** out_info)
{
	BAutolock _lock(mLock);

	std::map<media_addon_id, latent_info>::iterator ptr(mLatents.find(in_id));
	if (ptr == mLatents.end()) return B_MEDIA_BAD_NODE;
	*out_info = &(*ptr).second;
	return B_OK;
}


struct dormant_and_flavor {
	dormant_node_info dormant;
	x_flavor_info * flavor;
};

static float
special_count(
	const media_format & f)
{
	int score = 0;
	switch (f.type) {
	case B_MEDIA_RAW_AUDIO:
#define W(x) if (f.u.raw_audio. x > media_raw_audio_format::wildcard. x) score++;
		W(format);
		score *= 10;
		W(frame_rate);
		W(channel_count);
		W(byte_order);
		W(buffer_size);
#undef W
		break;
	case B_MEDIA_RAW_VIDEO:
#define W(x) if (f.u.raw_video. x > media_raw_video_format::wildcard. x) score++;
		W(display.format);
		W(display.bytes_per_row);
		score *= 5;
		W(display.line_width);
		W(display.line_count);
		W(display.pixel_offset);
		W(display.line_offset);
		W(field_rate);
		W(interlace);
		W(first_active);
		W(last_active);
		W(orientation);
		W(pixel_width_aspect);
		W(pixel_height_aspect);
#undef W
		break;
	case B_MEDIA_ENCODED_AUDIO:
		if (f.u.encoded_audio.encoding > media_encoded_audio_format::wildcard.encoding) score++;
		score *= 10;
		if (f.u.encoded_audio.bit_rate > media_encoded_audio_format::wildcard.bit_rate) score++;
		if (f.u.encoded_audio.frame_size > media_encoded_audio_format::wildcard.frame_size) score++;
#define W(x) if (f.u.encoded_audio.output. x > media_raw_audio_format::wildcard. x) score++;
		W(frame_rate);
		W(channel_count);
		W(format);
		W(byte_order);
		W(buffer_size);
#undef W
		break;
	case B_MEDIA_ENCODED_VIDEO:
		if (f.u.encoded_video.encoding > media_encoded_video_format::wildcard.encoding) score++;
		score *= 10;
		if (f.u.encoded_video.avg_bit_rate > media_encoded_video_format::wildcard.avg_bit_rate) score++;
		if (f.u.encoded_video.max_bit_rate > media_encoded_video_format::wildcard.max_bit_rate) score++;
		if (f.u.encoded_video.frame_size > media_encoded_video_format::wildcard.frame_size) score++;
		if (f.u.encoded_video.forward_history > media_encoded_video_format::wildcard.forward_history) score++;
		if (f.u.encoded_video.backward_history > media_encoded_video_format::wildcard.backward_history) score++;
#define W(x) if (f.u.encoded_video.output. x > media_raw_video_format::wildcard. x) score++;
		W(display.format);
		W(display.bytes_per_row);
		score *= 2;
		W(display.line_width);
		W(display.line_count);
		W(display.pixel_offset);
		W(display.line_offset);
		W(field_rate);
		W(interlace);
		W(first_active);
		W(last_active);
		W(orientation);
		W(pixel_width_aspect);
		W(pixel_height_aspect);
#undef W
		break;
	default:
		if (f.type > 0) {
			for (const uchar * ptr = (const uchar *)&f; ptr < (const uchar *)(1+&f); ptr++) {
				if (*ptr != 0) {
					score++;
				}
			}
			return (float)score/sizeof(media_format);
		}
		break;
	}
	return (float)score;
}

static float
score_flavor(
	x_flavor_info & f,
	bool input,
	bool output)
{
	float score = 0.0;	//	goes DOWN with more specialization
	int count = 1;
	if (!input && !output) {
		input = output = true;
	}
	if (input) {
		for (int ix=0; ix<f.in_format_count; ix++) {
			score -= special_count(f.in_formats[ix]);
		}
		count += f.in_format_count;
	}
	if (output) {
		for (int ix=0; ix<f.out_format_count; ix++) {
			score -= special_count(f.out_formats[ix]);
		}
		count += f.out_format_count;
	}
	return score/count;
}

struct most_special_first {
	most_special_first() : input(true), output(true)
		{
		}
	most_special_first(bool i, bool o) : input(i), output(o)
		{
		}
	bool operator()(const dormant_and_flavor & a, const dormant_and_flavor & b)
		{
			float score_a;
			float score_b;
			score_a = score_flavor(*a.flavor, input, output);
			score_b = score_flavor(*b.flavor, input, output);
			return score_a < score_b;
		}
	bool input;
	bool output;
};


status_t
MLatentManager::QueryLatents(
	const media_format * input,
	const media_format * output,
	const char * name,
	BMessage & reply,
	int max_hits,
	uint64 require_kinds,
	uint64 deny_kinds)
{
	BAutolock _lock(mLock);

	dlog("QueryLatents(%x, %x, %s, %x, %d, %d, %d) among %d", 
		input, output, name ? name : "(no name)", &reply, max_hits, 
		require_kinds, deny_kinds, mLatents.size());

	std::map<media_addon_id, latent_info>::iterator ptr(mLatents.begin());
	std::map<media_addon_id, latent_info>::iterator end(mLatents.end());
	int out_hits = 0;
	int len = name ? strlen(name) : 0;
	if (len < 1) name = NULL;
	bool wildcard = name && (name[len-1] == '*');
#if DEBUG
	uint64 validate_mask = require_kinds & (B_PHYSICAL_INPUT|B_PHYSICAL_OUTPUT);
	ASSERT(!((validate_mask == B_PHYSICAL_INPUT) && (input != NULL)));
	ASSERT(!((validate_mask == B_PHYSICAL_OUTPUT) && (output != NULL)));
#endif
#if DEBUG
	char str[200];
	if (input) {
		string_for_format(*input, str, 200);
		str[48] = 0;
		dlog("input: %s", str);
	}
	if (output) {
		string_for_format(*output, str, 200);
		str[48] = 0;
		dlog("output: %s", str);
	}
#endif
	most_special_first msf(input != 0, output != 0);
	std::multiset<dormant_and_flavor, most_special_first> out_stuff(msf);
	while (ptr != end)
	{
		dlog("%s has %d flavors", (*ptr).second.path.str(), (*ptr).second.flavors->size());
		for (int ix=0; ix<(*ptr).second.flavors->size(); ix++)
		{
			bool good = true;
			/* check if they're bad */
			if (good && (((*(*ptr).second.flavors)[ix].kinds & require_kinds) != require_kinds)) {
				good = false;
				if (!good) {
					dlog("Latent match declined because of requested kinds");
				}
			}
			if (good && (((*(*ptr).second.flavors)[ix].kinds & deny_kinds) != 0)) {
				good = false;
				if (!good) {
					dlog("Latent match declined because of denied kinds");
				}
			}
			if (good && name) {
				if (wildcard) {
					if (strncmp(name, (*(*ptr).second.flavors)[ix].name, len-1)) {
						dlog("Latent match declined because of name (wildcard)");
						continue;
					}
				}
				else {
					if (strcmp(name, (*(*ptr).second.flavors)[ix].name)) {
						dlog("Latent match declined because of name (exact)");
						continue;
					}
				}
			}
			if (good && input) {
				bool match = false;
				for (int iz=0; iz<(*(*ptr).second.flavors)[ix].in_format_count; iz++) {
					if (format_is_compatible(*input, (*(*ptr).second.flavors)[ix].in_formats[iz])) {
						match = true;
						break;
					}
					else {
						uint32 type = (*(*ptr).second.flavors)[ix].in_formats[iz].type;
						dlog("bad format: %c%c%c%c", type>>24, type>>16, type>>8, type);
					}
				}
				good = match;
				if (!good) {
					dlog("Latent match declined because of input format (count %d)",
						(*(*ptr).second.flavors)[ix].in_format_count);
				}
			}
			if (good && output) {
				bool match = false;
				for (int iz=0; iz<(*(*ptr).second.flavors)[ix].out_format_count; iz++) {
					if (format_is_compatible(*output, (*(*ptr).second.flavors)[ix].out_formats[iz])) {
						match = true;
						break;
					}
					else {
						uint32 type = (*(*ptr).second.flavors)[ix].out_formats[iz].type;
						dlog("bad format: %c%c%c%c", type>>24, type>>16, type>>8, type);
					}
				}
				good = match;
				if (!good) {
					dlog("Latent match declined because of output format (count %d)",
						(*(*ptr).second.flavors)[ix].in_format_count);
				}
			}
			/* so it must be a hit */
			if (good) {
				dormant_and_flavor daf;
				daf.dormant.addon = (*ptr).second.id;
				daf.dormant.flavor_id = (*(*ptr).second.flavors)[ix].internal_id;
				strncpy(daf.dormant.name, (*(*ptr).second.flavors)[ix].name, sizeof(daf.dormant.name));
				daf.dormant.name[sizeof(daf.dormant.name)-1] = 0;
				daf.flavor = &((*(*ptr).second.flavors)[ix]);
				out_stuff.insert(daf);
				out_hits++;
			}
		}
		ptr++;
	}
	std::multiset<dormant_and_flavor, most_special_first>::iterator optr(out_stuff.begin());
	for (int xbt=0; xbt<out_hits; xbt++) {
		if (optr == out_stuff.end()) {
			break;
		}
		if (xbt < max_hits) {
			reply.AddData("be:dormant_node_info", B_RAW_TYPE, &(*optr).dormant, sizeof(dormant_node_info));
			dlog("Latent match made for %s", (*optr).dormant.name);
		}
		else {
			dlog("Latent match made, but no space");
		}
		optr++;
	}
	dlog("A total of %d hits", out_hits);
	reply.AddInt32("be:out_hits", out_hits);
	return B_OK;
}


status_t
MLatentManager::SniffRef(
	const entry_ref & ref,
	uint64 kinds,
	BMessage & out_message,
	const char * attr_node,
	const char * attr_mime)
{
	BAutolock _lock(mLock);

	float prev_max = -1.0;
	int32 prev_id = -1;
	int32 prev_internal = 0;
	char mime_type[256];
	char the_name[B_MEDIA_NAME_LENGTH];

	for (std::map<media_addon_id, latent_info>::iterator ptr(mLatents.begin()); 
		ptr != mLatents.end(); ptr++) {
		if (!(*ptr).second.enabled) {
			continue;
		}
		bool sniff = false;	/* filter out if it can't produce what we want */
		for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
			if (((*(*ptr).second.flavors)[ix].kinds & kinds) != kinds) {
				continue;
			}
			sniff = true;
			break;
		}
		if (sniff) {
			int32 new_internal = 0;
			float new_max = -1;
			BMimeType new_type;
			status_t err = (*ptr).second.add_on->SniffRef(ref, &new_type, &new_max, 
				&new_internal);
			if ((err == B_OK) && (new_max > prev_max)) {
				prev_max = new_max;
				prev_internal = new_internal;
				prev_id = (*ptr).second.id;
				strcpy(mime_type, new_type.Type());
				strcpy(the_name, (*(*ptr).second.flavors)[new_internal].name);
				dlog("Sniffing %s found type %s cap %g addon %s\n", ref.name, new_type.Type(), 
					new_max, (*(*ptr).second.flavors)[new_internal].name);
			}
		}
	}
	if (prev_id == -1) {
		return B_MEDIA_NO_HANDLER;
	}
	dormant_node_info hit;
	hit.addon = prev_id;
	hit.flavor_id = prev_internal;
	strcpy(hit.name, the_name);
	status_t err = out_message.AddData(attr_node, B_RAW_TYPE, &hit, sizeof(hit));
	if (err >= B_OK) {
		err = out_message.AddString(attr_mime, mime_type);
	}
	return err;
}


status_t
MLatentManager::SniffType(
	const char * in_type,
	uint64 kinds,
	BMessage & out_message,
	const char * attr_node)
{
	BAutolock _lock(mLock);

	float prev_max = -1.0;
	int32 prev_id = -1;
	int32 prev_internal = 0;
	char the_name[B_MEDIA_NAME_LENGTH];
	BMimeType type(in_type);

	for (std::map<media_addon_id, latent_info>::iterator ptr(mLatents.begin()); 
		ptr != mLatents.end(); ptr++) {
		if (!(*ptr).second.enabled) {
			continue;
		}
		bool sniff = false;	/* filter out if it can't produce what we want */
		for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
			if (((*(*ptr).second.flavors)[ix].kinds & kinds) != kinds) {
				continue;
			}
			sniff = true;
			break;
		}
		if (sniff) {
			int32 new_internal = 0;
			float new_max = -1;
			status_t err = (*ptr).second.add_on->SniffTypeKind(type, kinds, &new_max, &new_internal, NULL);
			if (err == -2)	//	magic value! old Node compiled with R4 headers
			{
				//	this match is suspect, so only let it through if it's of the right kind
				err = (*ptr).second.add_on->SniffType(type, &new_max, &new_internal);
				if (err == B_OK)
				{
					for (int ix=0; ix<(*ptr).second.flavors->size(); ix++)
					{
						if (((*(*ptr).second.flavors)[ix].internal_id == new_internal) &&
								((kinds & (*(*ptr).second.flavors)[ix].kinds) != kinds))
						{
							err = B_ERROR;
							break;
						}
					}
				}
			}
			if ((err == B_OK) && (new_max > prev_max)) {
				bool found = false;
				for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
					if ((*(*ptr).second.flavors)[ix].internal_id == new_internal) {
						strcpy(the_name, (*(*ptr).second.flavors)[ix].name);
						found = true;
						break;
					}
				}
				if (found) {
					prev_max = new_max;
					prev_internal = new_internal;
					prev_id = (*ptr).second.id;
					dlog("Sniffing %s found cap %g addon %s\n", type.Type(), 
						new_max, (*(*ptr).second.flavors)[new_internal].name);
				}
				else {
					FPrintf(stderr, "media_addon_server warning: sniffing '%s' found bogus flavor %d\n",
							(*ptr).second.path.str(), new_internal);
				}
			}
		}
	}
	if (prev_id == -1) {
		return B_MEDIA_NO_HANDLER;
	}
	dormant_node_info hit;
	hit.addon = prev_id;
	hit.flavor_id = prev_internal;
	strcpy(hit.name, the_name);
	status_t err = out_message.AddData(attr_node, B_RAW_TYPE, &hit, sizeof(hit));
	return err;
}


status_t
MLatentManager::GetFlavorFor(
	const dormant_node_info & info,
	dormant_flavor_info * out_flavor,
	char * outPath,
	size_t outPathSize)
{
	BAutolock _lock(mLock);

	LatentMap::iterator ptr(mLatents.find(info.addon));
	if (ptr == mLatents.end()) {
		return B_MEDIA_BAD_NODE;
	}
	for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
		if ((*(*ptr).second.flavors)[ix].internal_id == info.flavor_id) {
			*out_flavor = (*(*ptr).second.flavors)[ix];
			out_flavor->node_info = info;
			strncpy(outPath, (*ptr).second.path.str(), outPathSize);
			outPath[outPathSize-1] = 0;
			return B_OK;
		}
	}
	return B_MEDIA_BAD_NODE;
}


status_t
MLatentManager::GetFileFormatsFor(
	const dormant_node_info & info,
	media_file_format * out_read_formats,
	int32 in_read_count,
	int32 * out_read_count,
	media_file_format * out_write_formats,
	int32 in_write_count,
	int32 * out_write_count)
{
	BAutolock _lock(mLock);

	std::map<media_addon_id, latent_info>::iterator ptr(mLatents.find(info.addon));
	if (ptr == mLatents.end()) {
		return B_MEDIA_BAD_NODE;
	}
	return (*ptr).second.add_on->GetFileFormatList(info.flavor_id, out_read_formats,
		in_read_count, out_read_count, out_write_formats, in_write_count, out_write_count,
		NULL);
}

static const char *
find_flavor_name(
	int count,
	int32 id,
	x_flavor_info * info)
{
	for (int ix=0; ix<count; ix++)
	{
		if (info[ix].internal_id == id)
		return info[ix].name;
	}
	return NULL;
}


void
MLatentManager::SetNotify(
	status_t (*hook)(void *, BMediaAddOn *),
	void * cookie)
{
	mNotifyHook = hook;
	mNotifyCookie = cookie;
}


status_t
MLatentManager::AutoStartNodes()
{
	if (debug_level > 0) FPrintf(stderr, "Starting nodes with auto start flag\n");

	BAutolock _lock(mLock);

	// Step through addons and start nodes that have the auto-start
	// flag set.
	for (LatentMap::iterator addonIter = mLatents.begin();
		addonIter != mLatents.end(); addonIter++) {
		latent_info &latent = (*addonIter).second;
		if (latent.auto_start) {
			int32 count = 0;
			bool moreNodes = true;
			status_t err = B_OK;
			while (moreNodes) {
				if (count == SUSPICION_COUNT) {
					// There seem to be an awful lot of these nodes...
					// something is probably wrong.
					char addonName[PATH_MAX];
					GetAddonName(latent.loaded, addonName);
					char msg[300];
					sprintf(msg, "The media add-on '%s' is starting a great number of media Nodes. "
						"If problems using media functions arise, try disabling it by moving it out "
						"of the add-ons/media folder.", addonName);
					((_BMediaRosterP*) BMediaRoster::Roster())->MediaErrorMessage(msg);
				} else if (count == ABANDON_COUNT) {
					AddOnError(latent.loaded, B_ERROR, "reached abandon count for node");
					break;
				}

				BMediaNode *node = NULL;
				int32 internal_id = -1;
				err = latent.add_on->AutoStart(count, &node, &internal_id, &moreNodes);
				if (err < B_OK) {
					AddOnError(latent.loaded, err, "error autostarting node");
					break;
				}
					
				count++;
				if (node == NULL) {
					AddOnError(latent.loaded, B_ERROR, "instantiate failed\n");
					break;
				}

				((_BMediaRosterP*) BMediaRoster::Roster())->RegisterNode(node);
				if (debug_level > 1) FPrintf(stderr, "Auto started node %d\n", node->ID());
				ASSERT(mDefaultNodes.find(node->ID()) == mDefaultNodes.end());
				mDefaultNodes[node->ID()] = node;
			}
		}
	}

	return B_OK;
}

static const char * defname(media_node_id id)
{
	switch (id) {
	case DEFAULT_AUDIO_INPUT:
		return "audio input";
		break;
	case DEFAULT_AUDIO_OUTPUT:
		return "audio output";
		break;
	case DEFAULT_AUDIO_MIXER:
		return "audio mixer";
		break;
	case DEFAULT_VIDEO_INPUT:
		return "video input";
		break;
	case DEFAULT_VIDEO_OUTPUT:
		return "video output";
		break;
	}
	return "unknown default";
}

status_t
MLatentManager::StartDefaultNodes()
{
	if (debug_level > 0) FPrintf(stderr, "Starting default nodes\n");

	BAutolock _lock(mLock);

	status_t err = B_OK;
	status_t ret = B_OK;
	media_node_id defaults[] = {
		DEFAULT_AUDIO_INPUT,
		DEFAULT_AUDIO_OUTPUT,
		DEFAULT_AUDIO_MIXER, 
		DEFAULT_VIDEO_INPUT,
		DEFAULT_VIDEO_OUTPUT };
		
	for (int ix = 0; ix < sizeof(defaults) / sizeof(defaults[0]); ix++) {
		BMessage reply;
		err = BMediaRoster::Roster()->GetDefaultInfo(defaults[ix], reply);
		if (debug_level > 1) FPrintf(stderr, "GetDefaultInfo(%s) returns %x\n", defname(defaults[ix]), err);
		const char *path = NULL;
		int32 internal_id = 0;
		if (err == B_OK) {
			err = reply.FindString("be:_path", &path);
			if (debug_level > 1) FPrintf(stderr, "Path: %s\n", path);
		}

		if (err == B_OK) {
			err = reply.FindInt32("be:_internal_id", &internal_id);
			if (debug_level > 1) FPrintf(stderr, "ID: %d\n", internal_id);
		}

		const char * flavor_name = NULL;
		if (err == B_OK) {
			reply.FindString("be:_flavor_name", &flavor_name);
			if (debug_level > 1) FPrintf(stderr, "Flavor: %d/%s\n", internal_id, flavor_name);
			if (flavor_name)
				err = TranslateIDAndFlavor(path, &internal_id, flavor_name);
		}

		if (err == B_OK) {
			media_node_id node_id;
			media_node node;
			err = reply.FindInt32("be:node", &node_id);	/* there is a node ID if something's running */
			if (err != B_OK) {
				media_addon_id id = 0;
				err = GetAddonID(path, &id);
				if (debug_level > 1) FPrintf(stderr, "GetAddonID(%s) returns %ld (%s)\n", path, id, strerror(err));
				if (err == B_OK) {
					err = BMediaRoster::CurrentRoster()->GetInstancesFor(id, internal_id, &node_id);
					if (debug_level > 1) FPrintf(stderr, "GetInstancesFor(%ld, %ld) returns %ld (%s)\n", id, internal_id, node_id, strerror(err));
				}
				if (err == B_OK) {
					err = BMediaRoster::CurrentRoster()->GetNodeFor(node_id, &node);
					if (debug_level > 1) FPrintf(stderr, "GetNodeFor(%ld) returns %s\n", node_id, strerror(err));
				}
				if (err == B_OK) {
					err = BMediaRoster::Roster()->SetRunningDefault(defaults[ix], node);
					if (debug_level > 1) FPrintf(stderr, "SetRunningDefault %s node %d status %x\n",
							defname(defaults[ix]), node.node, err);
				}
			}
			if (err != B_OK) {
				media_addon_id id = 0;
				err = GetAddonID(path, &id);
				if (err == B_OK) {
					BMediaNode * nod = NULL;
					std::map<media_node_id, BMediaNode *>::iterator ptr;

					for (	ptr=mDefaultNodes.begin();
							ptr != mDefaultNodes.end();
							ptr++) {
						int32 flavor;
						BMediaAddOn *addon = (*ptr).second->AddOn(&flavor);
						if (	addon && (addon->AddonID() == id) &&
								(flavor == internal_id)) {
							nod = (*ptr).second;
							break;
						}
					}
					if (nod == NULL)
						err = B_MEDIA_BAD_NODE;
					else
						node_id = nod->ID();
				}

				if (debug_level > 1) FPrintf(stderr, "FindFlavor(%d, %d) finds node %d, err %x\n", id, internal_id, node, err);
			}

			if (debug_level > 1) FPrintf(stderr, "Default %s Node: %d (err 0x%x)\n", defname(defaults[ix]), node_id, err);
			if (err != B_OK) {
				BMessage config;
				media_node mnode;
				if (debug_level > 1) FPrintf(stderr, "Calling InstantiateLatent(%s, %d)\n", path, internal_id);
				err = InstantiateLatent(path, internal_id, config, mnode);

				// Note that the media_addon_server owns the default nodes.
				BMediaRoster::Roster()->GetNodeFor(mnode.node, &mnode);
				if (debug_level > 0) FPrintf(stderr, "StartDefaultNodes instantiated node %d\n", mnode.node);
				if (err == B_OK) {
					err = BMediaRoster::Roster()->SetRunningDefault(defaults[ix], mnode);
					if (debug_level > 1) FPrintf(stderr, "SetRunningDefault %s node %d status %x\n",
							defname(defaults[ix]), mnode.node, err);
				}
			}
		}
		else {
			if (debug_level > 1) FPrintf(stderr, "Default info for node %s did not lead to existing Node or Add-On\n", defname(defaults[ix]));
		}

		if (err != B_OK) {
			if (debug_level > 0) FPrintf(stderr, "MLatentManager: cannot find default %s [%x] -- will try finding one\n",
					defname(defaults[ix]), err);
			err = FindTemporaryDefault(defaults[ix], err);
			if (err < ret) ret = err;
		}
	}

	return ret;
}

status_t
MLatentManager::TranslateIDAndFlavor(
	const char * path,
	int32 * io_id,
	const char * flavor_name)
{
	media_addon_id addon = -1;
	status_t err = GetAddonID(path, &addon);
	if (err < B_OK)
		return err;

	//	Given an ID and a name from a previous run, find the
	//	ID of the best match in the current run. If both name
	//	and ID match, it's a winner. If there is a name-only
	//	match, select that as best backup case. If there is
	//	only an ID match, use that as a last resort match.

	latent_info * info = NULL;
	err = FindLatent(addon, &info);
	if (err < B_OK)
		return err;
	if (!info->enabled)
		return B_MEDIA_ADDON_DISABLED;

	int32 out_flavor = -1;
	for (int ix=0; ix<info->flavors->size(); ix++) {
		bool strmatch = !strcmp(flavor_name, (*info->flavors)[ix].name);
		if (strmatch)	//	name is a good indication
		{
			if (!(*info->flavors)[ix].enabled)
				continue;
			out_flavor = (*info->flavors)[ix].internal_id;
		}
		if ((*info->flavors)[ix].internal_id == *io_id) {
			if (!(*info->flavors)[ix].enabled)
				continue;
			if (strmatch)	//	everything matches; take it!
				break;
			if (out_flavor == -1)	//	if this is the first or only match
				out_flavor = (*info->flavors)[ix].internal_id;
		}
	}
	if (out_flavor == -1)
		return B_NAME_NOT_FOUND;
	*io_id = out_flavor;
	return B_OK;
}

void 
MLatentManager::GetAddonName(image_id imgid, char *name)
{
	image_info imgInfo;
	if (imgid > 0 && get_image_info(imgid, &imgInfo) == B_OK)
		strcpy(name, imgInfo.name);
	else
		strcpy(name, "<unknown>");
}

void 
MLatentManager::AddOnError(image_id imgid, status_t error, const char *msg)
{
	image_info imgInfo;
	strcpy(imgInfo.name, "<unknown>");
	if (imgid > 0)
		get_image_info(imgid, &imgInfo);

	if (debug_level > 0) FPrintf(stderr, "%s: addon %s, error %s\n", msg, imgInfo.name, strerror(error));
}


status_t
MLatentManager::InstantiateLatent(
	BMessage & message,
	media_node & out)
{
	BAutolock _lock(mLock);

	/* from the saved information in the message, find the node handler and invoke it */
	const char * path;
	int32 id;
	status_t err = message.FindString("be:_path", &path);
	if (err < B_OK)
		return err;
	
	err = message.FindInt32("be:_internal_id", &id);
	if (err < B_OK)
		return err;
	
	return InstantiateLatent(path, id, message, out);
}


status_t
MLatentManager::InstantiateLatent(
	const char * path,
	int32 id,
	BMessage & message,
	media_node & out)
{
	BAutolock _lock(mLock);

	media_addon_id addon;
	status_t err = GetAddonID(path, &addon);
	if (err < B_OK)
		return err;
	
	latent_info * info = NULL;
	err = FindLatent(addon, &info);
	if (err < B_OK)
		return err;
	
	if (!info->enabled)
		return B_MEDIA_ADDON_DISABLED;
	
	err = B_MEDIA_BAD_NODE;
	int32 count = 0;
	for (int ix=0; ix < info->flavors->size(); ix++) {
		if ((*info->flavors)[ix].internal_id == id) {
			if (!(*info->flavors)[ix].enabled)
				return B_MEDIA_ADDON_DISABLED;
			
			if (((*info->flavors)[ix].possible_count > 0)
				&& ((*info->flavors)[ix].possible_count <= count))
				return B_MEDIA_ADDON_RESTRICTED;

			BMediaNode * node = NULL;
			err = MakeNode(info->add_on, (*info->flavors)[ix], message, &node);
			if (err == B_OK)
				err = ((_BMediaRosterP*) BMediaRoster::Roster())->RegisterUnownedNode(node);

			if (err == B_OK) {
				out = node->Node();
			} else {
				if (debug_level > 0) FPrintf(stderr, "InstantiateLatent: error registering node.  releasing\n");
				if (node)
					node->Release();
			}
			break;
		}
	}

	return err;
}


status_t
MLatentManager::MakeNode(
	BMediaAddOn * addon,
	flavor_info & info,
	BMessage & message,
	BMediaNode ** out_node)
{
	BAutolock _lock(mLock);

	status_t err = B_OK;
	*out_node = NULL;
	*out_node = addon->InstantiateNodeFor(&info, &message, &err);
	if (err < B_OK && *out_node) {
		if (debug_level > 1) FPrintf(stderr, "MakeNode: error registering node.  releasing\n");
		(*out_node)->Release();
		*out_node = NULL;
	}

	return err;
}

status_t
MLatentManager::FindTemporaryDefault(
	media_node_id what,
	status_t err)
{
	BAutolock _lock(mLock);

	media_format * input = NULL;
	media_format * output = NULL;
	media_format audio_fmt;
	media_format video_fmt;
	audio_fmt.type = B_MEDIA_RAW_AUDIO;
	audio_fmt.u.raw_audio = media_raw_audio_format::wildcard;
	video_fmt.type = B_MEDIA_RAW_VIDEO;
	video_fmt.u.raw_video = media_raw_video_format::wildcard;
	uint64 require_kinds = 0;

	switch (what) {
	case DEFAULT_AUDIO_INPUT:
		output = &audio_fmt;
		require_kinds = B_BUFFER_PRODUCER | B_PHYSICAL_INPUT;
		break;
	case DEFAULT_AUDIO_OUTPUT:
		input = &audio_fmt;
		require_kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT;
		break;
	case DEFAULT_AUDIO_MIXER:
		input = &audio_fmt;
		output = &audio_fmt;
		require_kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER;
		break;
	case DEFAULT_VIDEO_INPUT:
		output = &video_fmt;
		require_kinds = B_BUFFER_PRODUCER | B_PHYSICAL_INPUT;
		break;
	case DEFAULT_VIDEO_OUTPUT:
		input = &video_fmt;
		require_kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT;
		break;
	default:
		return err;
	}
	bool gotLive = false;
	{
		int32 count = 64;
		live_node_info * info = new live_node_info[count];
	again:
		int32 old_count = count;
		err = BMediaRoster::Roster()->GetLiveNodes(
			info, &count, input, output, NULL, require_kinds);
		if (!err && (count > old_count)) {
			delete[] info;
			count = count+50;	/* there might have been more created just now... */
			info = new live_node_info[count];
			goto again;
		}
		if (debug_level > 1) FPrintf(stderr, "FindTemporaryDefault() returns %d items for %x\n", count, what);
		if (!err) {
			for (int ix=0; ix<count; ix++) {
				//fixme: figure out whether this node should be disqualified from being a default
				if ((count > 1) && !strncasecmp(info[ix].name, "none ", 5)) {
					fprintf(stderr, "Node '%s' disqualified as temporary default (there is no cause for alarm).\n", info[ix].name);
					fprintf(stderr, "Please choose a permanent default in the Media prefs panel.\n");
					err = B_ERROR;	//	disqualify this node
				}
				else {
					err = BMediaRoster::Roster()->SetRunningDefault(what, info[ix].node);
				}
				if (err >= B_OK) {
					if (debug_level > 0) FPrintf(stderr, "Found live node %s for %s (require_kinds %Lx)\n",
							info[ix].name, defname(what), require_kinds);
					gotLive = true;
					break;
				}
			}
		}
		delete[] info;
	}
	if (!gotLive) {
		//	check the list of latent nodes for a match
		LatentMap::iterator ptr(mLatents.begin());
		//	this code could be shared with QueryLatents (except we do less)
		while (!gotLive && (ptr != mLatents.end())) {
			if (!(*ptr).second.enabled) {
				continue;
			}
			for (int ix=0; ix<(*ptr).second.flavors->size(); ix++) {
				bool good = true;
				/* check if they're bad */
				if (good && (((*(*ptr).second.flavors)[ix].kinds & require_kinds) != require_kinds)) {
					good = false;
				}
				if (good && !strncasecmp((*(*ptr).second.flavors)[ix].name, "none ", 5)) {
					fprintf(stderr, "Latent Node '%s' disqualified as temporary default (there is no cause for alarm).\n", (*(*ptr).second.flavors)[ix].name);
					fprintf(stderr, "Please choose a permanent default in the Media prefs panel.\n");
					good = false;	//	disqualify this node
				}
				x_flavor_info * in_flavor = NULL;
				if (good && input) {
					bool match = false;
					for (int iz=0; iz<(*(*ptr).second.flavors)[ix].in_format_count; iz++) {
						if (format_is_compatible(*input, (*(*ptr).second.flavors)[ix].in_formats[iz])) {
							in_flavor = &(*(*ptr).second.flavors)[ix];
							match = true;
							break;
						}
					}
					good = match;
				}
				x_flavor_info * out_flavor = NULL;
				if (good && output) {
					bool match = false;
					for (int iz=0; iz<(*(*ptr).second.flavors)[ix].out_format_count; iz++) {
						if (format_is_compatible(*output, (*(*ptr).second.flavors)[ix].out_formats[iz])) {
							out_flavor = &(*(*ptr).second.flavors)[ix];
							match = true;
							break;
						}
					}
					good = match;
				}
				if (good && ((in_flavor != 0) || (out_flavor != 0))) {
					//	invoke it
					BMediaNode * node = NULL;
					BMessage nothing;
					err = MakeNode((*ptr).second.add_on, in_flavor ? *in_flavor : *out_flavor,
							nothing, &node);
					if (debug_level > 0) FPrintf(stderr, "Temporary MakeNode(addon %s) for %s returns %s\n",
							(*ptr).second.path.str(), defname(what), strerror(err));
					if (err == B_OK) {
						err = ((_BMediaRosterP *)BMediaRoster::CurrentRoster())->RegisterUnownedNode(node);
					}
					if (err == B_OK) {
						BMediaRoster::CurrentRoster()->SetRunningDefault(what, node->Node());
					}
					else {
						if (node) node->Release();
						if (debug_level > 0) FPrintf(stderr, "Temporary MakeNode(addon %s) for %s failed\n",
								(*ptr).second.path.str(), defname(what));
					}
					gotLive = (err == B_OK);
					break;
				}
			}
			ptr++;
		}
	}
	return err;
}

void 
MLatentManager::HandleNodeMonitor(BMessage *message)
{
	BAutolock _lock(mLock);

	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK) {
		if (debug_level > 0) FPrintf(stderr, "HandleNodeMonitor couldn't find opcode\n");
		return ;
	}
	
	switch (opcode) {
		case B_ENTRY_CREATED: {
			const char *name;
			ino_t dir;
			dev_t dev;
			if (message->FindString("name", &name) != B_OK ||
				message->FindInt64("directory", &dir) != B_OK ||
				message->FindInt32("device", &dev) != B_OK) {
				if (debug_level > 0) FPrintf(stderr, "Error finding data in entry created message\n");
				return ;
			}
			
			entry_ref ref(dev, dir, name);
			HandleEntryCreated(&ref);
			break;
		}

		case B_ENTRY_REMOVED: {
			ino_t node;
			dev_t dev;
			if (message->FindInt64("node", &node) != B_OK ||
				message->FindInt32("device", &dev) != B_OK) {
				if (debug_level > 0) FPrintf(stderr, "Error finding data in entry removed message\n");
				return ;
			}
			
			node_ref nref;
			nref.device = dev;
			nref.node = node;
			HandleEntryRemoved(&nref);
			break;
		}

		case B_ENTRY_MOVED: {
			const char *name;
			ino_t fromdir;
			ino_t todir;
			ino_t node;
			dev_t dev;
			if (message->FindString("name", &name) != B_OK ||
				message->FindInt64("from directory", &fromdir) != B_OK ||			
				message->FindInt64("to directory", &todir) != B_OK ||
				message->FindInt64("node", &node) != B_OK ||
				message->FindInt32("device", &dev) != B_OK) {
				if (debug_level > 0) FPrintf(stderr, "Error finding data in entry moved message\n");
				return ;
			}
			
			entry_ref newEntry(dev, todir, name);		
			HandleEntryMoved(fromdir, node, dev, &newEntry);
			break;
		}

		case B_STAT_CHANGED: {
			ino_t node;
			dev_t dev;
			if (message->FindInt64("node", &node) != B_OK ||
				message->FindInt32("device", &dev) != B_OK) {
				if (debug_level > 0) FPrintf(stderr, "Error finding data in entry moved message\n");
				return ;
			}
			
			node_ref nref;
			nref.node = node;
			nref.device = dev;
			HandleEntryChanged(&nref);
			break;
		}
	}
}

void 
MLatentManager::HandleEntryCreated(entry_ref *newEntry)
{
	if (debug_level > 1) FPrintf(stderr, "New add on\n");
	BEntry entry(newEntry);
	struct stat st;
	entry.GetStat(&st);
	if (S_ISDIR(st.st_mode)) {
		LoadDirectory(*newEntry, true);
	} else {
		BPath path;
		if (entry.GetPath(&path) == B_OK)
			LoadAddOn(path.Path());
	}
}

void 
MLatentManager::HandleEntryRemoved(node_ref *node)
{
	if (debug_level > 1) FPrintf(stderr, "Addon deleted\n");
	// recurse tree



}

void 
MLatentManager::HandleEntryChanged(node_ref *node)
{
	if (debug_level > 1) FPrintf(stderr, "Addon updated\n");
	latent_info info;
	if (FindLatent(node, &info) == B_OK) {
		if (debug_level > 1) FPrintf(stderr, "Updating addon %s\n", info.path.str());
//		UnloadAddOn(info.id);
//		LoadAddOn(info.path.str());
	}
}

void 
MLatentManager::HandleEntryMoved(ino_t oldDir, ino_t node, dev_t device, entry_ref *newEntry)
{
	bool newIsInPath = IsEntryUnder(kSystemAddOnDir, newEntry) ||
		IsEntryUnder(kUserAddOnDir, newEntry);

	entry_ref oldDirEntry(device, oldDir, ".");
	bool oldIsInPath = IsEntryUnder(kSystemAddOnDir, &oldDirEntry) ||
		IsEntryUnder(kUserAddOnDir, &oldDirEntry);

	if (newIsInPath && !oldIsInPath) {
		HandleEntryCreated(newEntry);
	} else if (!newIsInPath && oldIsInPath) {
		node_ref nref;
		nref.node = node;
		nref.device = device;
		HandleEntryRemoved(&nref);
	} else if (newIsInPath && oldIsInPath) {
		if (debug_level > 1) FPrintf(stderr, "Addon moved within media addon tree.\n");
		node_ref nref;
		nref.node = node;
		nref.device = device;
		latent_info linfo;
		if (FindLatent(&nref, &linfo) == B_OK) {
			// Update path.  Note that this doesn't recurse
			BEntry entr(newEntry);
			BPath path;
			if (entr.GetPath(&path) == B_OK) {
				if (debug_level > 1) FPrintf(stderr, "Update path from %s to %s\n", linfo.path.str(), path.Path());
				linfo.path = path.Path();	
			}
		}
	} else {
		if (debug_level > 1) FPrintf(stderr, "??? got node monitor for something I don't care about...\n");
		// Could be a problem for symlinks.
	}
}

bool 
MLatentManager::IsEntryUnder(const char *dirPathStr, entry_ref *targetEntryRef)
{
	BEntry dirEntry(dirPathStr, true);
	BPath dirPath(&dirEntry);

	BEntry targetEntry(targetEntryRef, true);
	BPath targetPath;
	if (targetEntry.GetPath(&targetPath) != B_OK) {
		if (debug_level > 0) FPrintf(stderr, "IsEntryUnder: Couldn't get paths\n");
		return false;
	}

	while (true) {
		if (dirPath == targetPath)
			return true;

		BPath temp;
		if (targetPath.GetParent(&temp) != B_OK)
			break;

		targetPath = temp;
	}
	
	return false;
}

status_t
MLatentManager::FindLatent(node_ref *loc, latent_info *out_latent)
{
	for (LatentMap::iterator i = mLatents.begin(); i != mLatents.end(); i++) {
		BNode node((*i).second.path.str());
		node_ref thisRef;
		if (node.GetNodeRef(&thisRef) == B_OK) {
			if (thisRef.device == loc->device &&
				thisRef.node == loc->node) {
				*out_latent = (*i).second;
				return B_OK;
			}
		}
	}
	
	return B_ERROR;
}

