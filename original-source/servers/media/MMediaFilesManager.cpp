/*	MMediaFilesManager.cpp	*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <File.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <FindDirectory.h>

#include "MediaFiles.h"
#include "MMediaFilesManager.h"
#include "debug.h"

const float MMediaFilesManager::kDefaultAudioGain = 1.0;

static bool
get_path_for_ref(
	const entry_ref * ref,
	char * path)
{
	BEntry ent(ref);
	BPath p;
	if (ent.GetPath(&p) == B_OK) {
		strcpy(path, p.Path());
		return true;
	}
	path[0] = 0;
	return false;
}

MMediaFilesManager::MMediaFilesManager() :
	MSavable("MMediaFilesManager", 0)
{
}

MMediaFilesManager::~MMediaFilesManager()
{
}


status_t 
MMediaFilesManager::SaveState()
{
	status_t err = B_OK;
	BFile * f = OpenFile();
	if (f == NULL) {
		return B_ERROR;
	}
	StDelete<BFile> d(f);

	err = f->SetSize(0);
	if (err < 0) return err;
	int64 magic = 0x18723462ac00150cLL;
	err = f->Write(&magic, sizeof(magic));
	if (err != sizeof(magic)) return err<0 ? err : B_DEVICE_FULL;
	int32 version = 2;
	err = f->Write(&version, sizeof(version));
	if (err != sizeof(version)) return err<0 ? err : B_DEVICE_FULL;
	int32 count = m_types.size();
	err = f->Write(&count, sizeof(count));
	if (err != sizeof(count)) return err<0 ? err : B_DEVICE_FULL;
	int32 length;
	for (std::set<str_ref>::iterator ptr=m_types.begin(); ptr != m_types.end(); ptr++) {
		debug(1, "saving type %s\n", (*ptr).str());
		length = (*ptr).length()+1;
		err = f->Write(&length, sizeof(length));
		if (err != sizeof(length)) return err < 0 ? err : B_DEVICE_FULL;
		err = f->Write((*ptr).str(), length);
		if (err != length) return err < 0 ? err : B_DEVICE_FULL;
		mf_item item;
		item.type = (*ptr);
		for (std::map<mf_item, mf_ref>::iterator it=m_items.lower_bound(item); 
			(it != m_items.end()) && ((*it).first.type == (*ptr)); it++) {
			BPath path;
			BEntry ent(&(*it).second.ref);
			if (ent.GetPath(&path) && ((*it).first.path.str() != 0)) {	//	save the entry, if possible, else use the stored path
				path.SetTo((*it).first.path.str());
			}
			debug(1, "saving %s as %s\n", (*it).first.name.str(), path.Path());
			length = (*it).first.name.length()+1;
			err = f->Write(&length, sizeof(length));
			if (err != sizeof(length)) return err < 0 ? err : B_DEVICE_FULL;
			err = f->Write((*it).first.name.str(), length);
			if (err != length) return err < 0 ? err : B_DEVICE_FULL;
			length = path.Path() ? strlen(path.Path())+1 : 1;
			err = f->Write(&length, sizeof(length));
			if (err != sizeof(length)) return err < 0 ? err : B_DEVICE_FULL;
			err = f->Write((length == 1) ? "" : path.Path(), length);
			if (err != length) return err < 0 ? err : B_DEVICE_FULL;
			err = f->Write(&(*it).second.audio_gain, sizeof(float));
			if (err != sizeof(float)) return err < 0 ? err : B_DEVICE_FULL;
		}
		length = 0;
		err = f->Write(&length, sizeof(length));
		if (err != sizeof(length)) return err < 0 ? err : B_DEVICE_FULL;
	}
	length = 0;
	err = f->Write(&length, sizeof(length));
	if (err != sizeof(length)) return err < 0 ? err : B_DEVICE_FULL;
	debug(1, "done saving files\n");
	SetDirty(false);
	return B_OK;
}

	static status_t
	locate_file(
		const char * name,
		directory_which selector,
		entry_ref * out_ref)
	{
		BPath path;
		status_t err = B_ERROR;
		err = find_directory(selector, &path);
		if (!err) {
			BEntry ent;
			BDirectory dir(path.Path());
			err = B_ENTRY_NOT_FOUND;
			char t_name[256];
			const char * fmts[] = { "%s", "%s.wav", "%s.aiff", 0 };
			for (const char **p = fmts; *p != 0; p++) {
				sprintf(t_name, *p, name);
				err = dir.FindEntry(t_name, &ent);
				if (!err) {
					ent.GetRef(out_ref);
					break;
				}
			}
		}
		else {
			debug(1, "find_directory(%d) failed\n", selector);
		}
		return err;
	}

	static status_t
	find_a_sound(
		const char * best_name,
		entry_ref * out_ref)
	{
		status_t err = B_ENTRY_NOT_FOUND;
		if (err < 0) err = locate_file(best_name, B_BEOS_SOUNDS_DIRECTORY, out_ref);
		if (err < 0) err = locate_file(best_name, B_COMMON_SOUNDS_DIRECTORY, out_ref);
		if (err < 0) err = locate_file(best_name, B_USER_SOUNDS_DIRECTORY, out_ref);
		if (err == B_OK) {
			fprintf(stderr, "Found '%s' for '%s'\n", out_ref->name, best_name);
		}
		return err < 0 ? err : 0;
	}

status_t 
MMediaFilesManager::LoadState()
{
	status_t err = B_OK;
	bool created = false;
	BFile * f = OpenFile(&created);
	if (f == NULL) {
		return B_ERROR;
	}
	StDelete<BFile> d(f);

	int64 magic = 0;
	int32 version = 0;
	int32 count = 0;
	
	err = f->Read(&magic, sizeof(magic));
	if (magic != 0x18723462ac00150cLL) goto badfile;
	err = f->Read(&version, sizeof(version));
	if (version < 1 || version > 2) goto badfile;
	err = f->Read(&count, sizeof(count));
	if (err != sizeof(count)) goto badfile;
	int32 length;
	while (count-- > 0) {
		err = f->Read(&length, sizeof(length));
		if (err != sizeof(length)) goto badfile;
		mf_item item;
		err = f->Read(item.type.resize(length), length);
		if (err != length) goto badfile;
		m_types.insert(item.type);
#if DEBUG
		fprintf(stderr, "%s\n", item.type.str());
#endif
		while (true) {
			entry_ref ref;
			err = f->Read(&length, sizeof(length));
			if (err != sizeof(length)) goto badfile;
			if (length == 0) break;
			err = f->Read(item.name.resize(length), length);
			if (err != length) goto badfile;
			err = f->Read(&length, sizeof(length));
			if (err != sizeof(length)) goto badfile;
			err = f->Read(item.path.resize(length), length);
			if (err != length) goto badfile;
			float audio_gain = 1.0;
			if (version > 1) {
				err = f->Read(&audio_gain, sizeof(float));
				if (err != sizeof(float)) goto badfile;
			}
			err = item.path.str() ?
				get_ref_for_path(item.path.str(), &ref) :
				B_ERROR;
#if DEBUG
			fprintf(stderr, "%s:%s:%s:%.1f\n", item.type.str(), item.name.str(), item.path.str(), audio_gain);
#endif
			if (err < B_OK) {
//				const char * name = strrchr(path.str(), '/');
//				if (name) 
//					name++;
//				else 
//					name = path.str();
//				if ((err = find_a_sound(name, &ref)) < B_OK) {
//					if ((err = find_a_sound(NULL, &ref)) < B_OK) {
//						debug(1, "TRINITY: can't find a file for %s '%s'\n", 
//							item.type.str(), item.name.str());
//					}
//				}
				const char * s = 0;
				if (item.path.str() != 0) s = strrchr(item.path.str(), '/');
				if (s) s++; else s = item.path.str();
				ref.set_name(s ? s : "");
				ref.device = -1;
				ref.directory = -1;
				err = B_OK;
			}
			if (err >= B_OK) {
				m_items[item] = mf_ref(ref, audio_gain);
			}
		}
	}
	err = f->Read(&length, sizeof(length));
	if (err != sizeof(length)) goto badfile;
	create_default_settings();
	return B_OK;
badfile:
	{
		f->SetSize(0);
		f->Sync();
		/* create default settings */
		create_default_settings();
	}
	if (!created) debug(0, "The media server's settings file is damaged. Some settings relating to audio and video have been lost.\n");
#if DEBUG
	for (std::set<str_ref>::iterator x = m_types.begin(); x != m_types.end(); x++) {
		fprintf(stderr, "have type %s\n", (*x).str());
	}
	for (std::map<mf_item, mf_ref>::iterator x = m_items.begin(); x != m_items.end(); x++) {
		fprintf(stderr, "have item %s:%s file %s\n", (*x).first.type.str(), (*x).first.name.str(), 
			(*x).second.ref.name);
	}
#endif
	return err < 0 ? err : B_IO_ERROR;
}

void
MMediaFilesManager::create_default_sound(const char* event, const char* sound)
{
//	PRINT(("+em: create_default_sound('%s', '%s')\n", event, sound));
	mf_item i;
	i.type.set(BMediaFiles::B_SOUNDS);
	i.name.set(event);
	entry_ref ref;
	if (m_items.find(i) == m_items.end()) {
		if (sound && !find_a_sound(sound, &ref)) {
			char str[1024];
			get_path_for_ref(&ref, str);
			i.path.set(str);
		}
		else {
			i.path.set("");
		}
		m_items[i] = ref;
	}
}

void
MMediaFilesManager::create_default_settings()
{
//	PRINT(("+em: MMediaFilesManager::create_default_settings()\n"));
	m_types.insert(BMediaFiles::B_SOUNDS);
	create_default_sound("Beep", "BeBeep");
	create_default_sound("Startup", "BeStartup");
	create_default_sound("Mouse Down");
	create_default_sound("Mouse Up");
	create_default_sound("Key Down");
	create_default_sound("Key Repeat");
	create_default_sound("Key Up");
	create_default_sound("Window Open");
	create_default_sound("Window Close");
	create_default_sound("Window Activated");
	create_default_sound("Window Minimized");
	create_default_sound("Window Restored");
	create_default_sound("Window Zoomed");
	SetDirty(true);
//	PRINT(("+em: MMediaFilesManager::create_default_settings() done.\n"));
#if 0	
	mf_item i;
	i.name.set("Beep");
	entry_ref ref;
	if (m_items.find(i) == m_items.end()) {
		debug(1, "finding Beep sound\n");
		if (!find_a_sound("BeBeep", &ref)) {
			char str[1024];
			get_path_for_ref(&ref, str);
			i.path.set(str);
			m_items[i] = ref;
		}
	}
	i.name.set("Startup");
	if (m_items.find(i) == m_items.end()) {
		debug(1, "finding Startup sound\n");
		if (!find_a_sound("BeStartup", &ref)) {
			char str[1024];
			get_path_for_ref(&ref, str);
			i.path.set(str);
			m_items[i] = ref;
		}
	}
#endif
}


status_t 
MMediaFilesManager::GetTypes(
	BMessage & request,
	BMessage & reply)
{
	&request;
	debug(2, "MMediaFilesManager::GetTypes()\n");
	status_t err = B_OK;
	for (std::set<str_ref>::iterator ptr = m_types.begin(); ptr != m_types.end(); ptr++) {
		err = reply.AddString("be:type", (*ptr).str());
		if (err < B_OK) break;
	}
	return err;
}

status_t 
MMediaFilesManager::AddType(
	BMessage & request,
	BMessage & reply)
{
	&reply;
	const char * type = NULL;
	status_t err = request.FindString("be:type", &type);
	debug(2, "MMediaFilesManager::AddType(%s)\n", type);
	if ((err >= B_OK) && (type != NULL)) {
		SetDirty(true);
		str_ref stype(type);
		m_types.insert(stype);	/* if it's already there, it'll not get replaced, which is B_OK */
	}
	return err;
}

status_t 
MMediaFilesManager::GetItems(
	BMessage & request,
	BMessage & reply)
{
	const char * type = NULL;
	status_t err = request.FindString("be:type", &type);
	debug(2, "MMediaFilesManager::GetItems(%s)\n", type);
	if (err < B_OK) return err;
	if (!type) return B_BAD_VALUE;
	mf_item item;
	item.type = type;
	for (std::map<mf_item, mf_ref>::iterator ptr = m_items.lower_bound(item); 
		(ptr != m_items.end()) && ((*ptr).first.type == item.type); ptr++) {
		if (!(*ptr).first.name.str()) continue;
		err = reply.AddString("be:item", (*ptr).first.name.str());
		if ((*ptr).second.ref.device == -1 &&
			(*ptr).first.path.str() != 0) {
			//	try to resolve again
			get_ref_for_path((*ptr).first.path.str(), &(*ptr).second.ref);
		}
		reply.AddFloat("be:audio_gain", (*ptr).second.audio_gain);
		if (err >= B_OK) reply.AddRef("be:ref", &(*ptr).second.ref);
		if (err < B_OK) break;
	}
	return err;
}

status_t 
MMediaFilesManager::SetItem(
	BMessage & request,
	BMessage & reply)
{
	&reply;
	const char * type = NULL;
	const char * item = NULL;

	entry_ref ref;
	bool setRef = true;
	status_t err = request.FindString("be:type", &type);
	if (err >= B_OK) err = request.FindString("be:item", &item);
	if (err >= B_OK) {
		err = request.FindRef("be:ref", &ref);
		if (err < B_OK) {
			setRef = false;
			err = B_OK;
		}
	}

	float audio_gain = kDefaultAudioGain;
	bool setGain = (request.FindFloat("be:audio_gain", &audio_gain) == B_OK);
//	PRINT(("MMediaFilesManager::SetItem(%s, %s, %s:%s, %.2f:%s)\n", type, item,
//		ref.name, setRef ? "*" : "_",
//		audio_gain, setGain ? "*" : "_"));
	debug(1, "MMediaFilesManager::SetItem(%s, %s, %s)\n", type, item, ref.name);
	if ((err >= B_OK) && ((type == NULL) || (item == NULL))) {
		err = B_BAD_VALUE;
	}
	if (err >= B_OK) {
		SetDirty(true);
		mf_item mfi;
		mfi.type.set(type);
		mfi.name.set(item);
		mf_ref mfr;
		
		std::map<mf_item, mf_ref>::iterator it = m_items.find(mfi);
		if (it != m_items.end()) {
			mfi = (*it).first;
			mfr = (*it).second;
		}
		
//		ASSERT(it != m_items.end());	// +em: what the fuck?

		if (setRef) {
			mfr.ref = ref;
			mfi.path.set(ref.name);
		}
		if (setGain) {
			mfr.audio_gain = audio_gain;
		}
		m_items[mfi] = mfr;
	}
	return err;
}

status_t 
MMediaFilesManager::ClearItem(
	BMessage & request,
	BMessage & reply)
{
	&reply;
	const char * type = NULL;
	const char * item = NULL;
	entry_ref ref;
	status_t err = request.FindString("be:type", &type);
	if (err >= B_OK) err = request.FindString("be:item", &item);
	if (err >= B_OK) err = request.FindRef("be:ref", &ref);
	if ((err >= B_OK) && ((type == NULL) || (item == NULL))) {
		err = B_BAD_VALUE;
	}
	debug(2, "MMediaFilesManager::ClearItem(%s, %s, %s)\n", type, item, ref.name);
	if (err >= B_OK) {
		mf_item mfi;
		mfi.type.set(type);
		mfi.name.set(item);
		std::map<mf_item, mf_ref>::iterator pos = m_items.find(mfi);
		if (pos == m_items.end()) {
			err = B_BAD_INDEX;
		}
		else if ((*pos).second.ref != ref) {
			err = B_ENTRY_NOT_FOUND;
		}
		else {
			SetDirty(true);
			mf_item mfi((*pos).first);
			m_items.erase(pos);
			mfi.path.set("");
			entry_ref ref;
			m_items[mfi] = ref;
		}
	}
	return err;
}

status_t 
MMediaFilesManager::RemoveItem(
	BMessage & request,
	BMessage & reply)
{
	&reply;
	const char * type = NULL;
	const char * item = NULL;
	status_t err = request.FindString("be:type", &type);
	if (err >= B_OK) err = request.FindString("be:item", &item);
	if ((err >= B_OK) && ((type == NULL) || (item == NULL))) {
		err = B_BAD_VALUE;
	}
	debug(2, "MMediaFilesManager::RemoveItem(%s, %s)\n", type, item);
	if (err >= B_OK) {
		mf_item mfi;
		mfi.type.set(type);
		mfi.name.set(item);
		std::map<mf_item, mf_ref>::iterator pos = m_items.find(mfi);
		if (pos == m_items.end()) {
			err = B_BAD_INDEX;
		}
		else {
			SetDirty(true);
			m_items.erase(pos);
		}
	}
	return err;
}

