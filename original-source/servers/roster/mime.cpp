/*****************************************************************************

	File : mime.cpp

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fs_info.h>
#include <ctype.h>
#include <unistd.h>
#include <fs_attr.h>
#include <Debug.h>
#include <StopWatch.h>
#include <Path.h>
#include <InterfaceDefs.h>

#include <private/storage/mime_private.h>
#include <private/storage/walker.h>
#include "main.h"

#include <AppFileInfo.h>
#include <Mime.h>
#include <Roster.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <File.h>
#include <Resources.h>
#include <Autolock.h>
#include <roster_private.h>
#include <message_strings.h>
#include <fs_index.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <private/storage/mime_private.h>
#include "mime_table.h"

#include <Bitmap.h>

#include <StreamIO.h>

/*------------------------------------------------------------------------*/

void _MEMORY_(const char *label, const char* label2)
{
	struct mstats	ms = mstats();
	syslog(LOG_INFO, "%s (%s): bytes=%x, blocks=%x, free=%x\n",
		label, label2, ms.bytes_used, ms.chunks_used, ms.bytes_free);
}

/*------------------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

BList	TMimeWorker::sList(3);
BLocker	TMimeWorker::sLock("worker_lock");
int32	TMimeWorker::sStdPulseRate = STANDARD_PULSE_RATE;

TMainMimeWorker	*gMainWorker;

status_t _real_update_app_(BAppFileInfo *, const char *, bool);

/*------------------------------------------------------------------------*/

bigtime_t activity_level()
{
	bigtime_t	t = 0;
	system_info	sinfo;
	get_system_info(&sinfo);
	for (int i = 0; i < sinfo.cpu_count; i++)
		t += sinfo.cpu_infos[i].active_time;
	return (t / ((bigtime_t) sinfo.cpu_count));
}

/*------------------------------------------------------------------------*/

status_t	setup_index(dev_t dev, const char *attr_name, int32 type)
{
	status_t	err;
	struct index_info	iinfo;
	err = fs_stat_index(dev, attr_name, &iinfo);
	if (err) {
		fs_create_index(dev, attr_name, type, 0);
	}
	return err;
}

/*------------------------------------------------------------------------*/

status_t	setup_all_indexes(dev_t dev)
{
	return setup_index(dev, B_APP_SIGNATURE_ATTR, B_MIME_STRING_TYPE);
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

TMimeWorker::TMimeWorker(const char *name, int32 prio)
	: BLooper(name, prio)
{
	fPulseRate = STANDARD_PULSE_RATE;
	fPulseNext = STANDARD_PULSE_RATE;
	fTask = NULL;

//+	SERIAL_PRINT(("TMimeWorker\n"));
}

/*------------------------------------------------------------------------*/

TMimeWorker::~TMimeWorker()
{
//+	SERIAL_PRINT(("TMimeWorker::~TMimeWorker\n"));

	BAutolock	a(sLock);
	sList.RemoveItem(this);

	if (fTask) {
		fTask->Suspend();
		fTask = NULL;
	}
}

/*------------------------------------------------------------------------*/


status_t
TMimeWorker::HandleRemoteMimePropertySetCall(BMessage *msg)
{
	const char *type;	
	if (msg->FindString("type", &type) != B_OK) 
		return B_ERROR;
	
	BMimeType mime(type);
	status_t result = mime.InitCheck();
	if (result != B_OK) 
		return result;
	
	switch(msg->what){
		case CMD_MIMETYPE_CREATE:
			{
				bool create;
				if (msg->FindBool("create", &create) != B_OK)
					return B_ERROR;
		
				if (create)
					return mime.Install();
		
				mime._touch_();
				return B_OK;
			}
	
		case CMD_MIMETYPE_DELETE:
			return mime.Delete();
			
		case CMD_MIMETYPE_SET_PREFERRED_APP:
			{
				const char *signature = NULL;
				msg->FindString("signature", &signature);
				return mime.SetPreferredApp(signature);
			}
			
		case CMD_MIMETYPE_SET_ATTR_INFO:
			return mime.SetAttrInfo(msg);
		
		case CMD_MIMETYPE_SET_FILE_EXTENSIONS:
			return mime.SetFileExtensions(msg);
			
		case CMD_MIMETYPE_SET_SHORT_DESCR:
			{
				const char *descr = NULL;
				msg->FindString("descr", &descr);
				return mime.SetShortDescription(descr);
			}
		
		case CMD_MIMETYPE_SET_LONG_DESCR:
			{
				const char *descr = NULL;
				msg->FindString("descr", &descr);
				return mime.SetLongDescription(descr);
			}
		
		case CMD_MIMETYPE_SET_ICON_FOR_TYPE:
			{
				BMessage flat;
				BBitmap *icon;
				const char *set_type;
				int32 which;
				
				if (msg->FindInt32("which", &which) != B_OK)
					return B_ERROR;
				if (msg->FindString("set_type", &set_type) != B_OK)
					set_type = NULL;
				if (msg->FindMessage("icon", &flat) != B_OK) 
					icon = NULL;
				else if(!(icon = (BBitmap*) BBitmap::Instantiate(&flat)))
					return B_ERROR;
		
				result = mime.SetIconForType(set_type, icon, (icon_size)which);
				delete icon;
				return result;
			}
		
		case CMD_MIMETYPE_SET_SUPPORTED_TYPES:
			return mime.SetSupportedTypes(msg);
			
		case CMD_MIMETYPE_SET_APP_HINT:
			{
				entry_ref ref;
				if (msg->FindRef("ref",&ref) != B_OK) 
					return mime.SetAppHint(NULL);
	
				return mime.SetAppHint(&ref);
			}
		
		case CMD_MIMETYPE_SET_SNIFFER_RULE:
			{
				const char *rule = NULL;
				msg->FindString("rule", &rule);
				return mime.SetSnifferRule(rule);
			}
		
		default:
			return B_ERROR;
	}
	return B_OK;
}	
	
void TMimeWorker::MessageReceived(BMessage *msg)
{

	switch (msg->what) {
		case CMD_GET_MIME_HANDLER:
			{
				BMessage reply;
				reply.AddMessenger("messenger", BMessenger(this));
				msg->SendReply(&reply);
				break;
			}

		case CMD_SAVE_MIME_TABLE: 
			gMimeTable->SaveTable();
			break;

		case CMD_DUMP_MIME_TABLE:
			gMimeTable->DumpTable();
			break;
			
		case CMD_UPDATE_MIME_INFO: 
			UpdateMimeInfo(msg);
			Quit();
			break;
			
		case CMD_FIRST_WAKE_UP:
			break;
			
		case B_PULSE:
			Pulse();
			break;
			
		case CMD_GET_SUPPORTING_APPS:
			{
				const char *type;
				BMessage reply;
				if ((msg->FindString("type", &type) == B_OK) &&
					BMimeType::IsValid(type)) {
						gMimeTable->GetSupportingApps(type, &reply);
				}
				msg->SendReply(&reply);
				break;
			}
			
		case CMD_UPDATE_SUPPORTING_APPS:
			{
				const char *sig = NULL;
				BMessage	new_list;
				bool		has_new;
				bool		force = false;
	
				if (msg->FindString("sig", &sig) != B_OK)
					break;
				
				if (!BMimeType::IsValid(sig))
					break;
				has_new = (msg->FindMessage("new", &new_list) == B_OK);
				msg->FindBool("force", &force);
	
				gMimeTable->Lock();
				gMimeTable->UpdateSupportingApps(sig, has_new ? &new_list : NULL,
					force);
				gMimeTable->Unlock();
				break;
			}

		case CMD_UPDATE_FILE_EXTENSION:
			{
				const char *type = NULL;
				BMessage new_list;
				BMessage old_list;
	
				if (msg->FindString("type", &type) != B_OK)
					break;

				if (!BMimeType::IsValid(type))
					break;
				bool has_old = (msg->FindMessage("old", &old_list) == B_OK);
				bool has_new = (msg->FindMessage("new", &new_list) == B_OK);
	
				gMimeTable->Lock();
				gMimeTable->UpdateFileSuffixes(type, has_new ? &new_list : NULL,
					has_old ? &old_list : NULL);
				gMimeTable->Unlock();
				break;
			}		
		
		case CMD_MIMETYPE_CREATE:
		case CMD_MIMETYPE_DELETE:
		case CMD_MIMETYPE_SET_PREFERRED_APP:
		case CMD_MIMETYPE_SET_ATTR_INFO:
		case CMD_MIMETYPE_SET_FILE_EXTENSIONS:
		case CMD_MIMETYPE_SET_SHORT_DESCR:
		case CMD_MIMETYPE_SET_LONG_DESCR:
		case CMD_MIMETYPE_SET_ICON_FOR_TYPE:
		case CMD_MIMETYPE_SET_SUPPORTED_TYPES:
		case CMD_MIMETYPE_SET_APP_HINT:
		case CMD_MIMETYPE_SET_SNIFFER_RULE:
			{
				BMessage reply;
				status_t res = HandleRemoteMimePropertySetCall(msg);
				reply.AddInt32("result", res);
				msg->SendReply(&reply);
				break;
			}
		
		default:						
			_inherited::MessageReceived(msg);
			break;
	}
}

/*------------------------------------------------------------------------*/

bool _supports_attr_(BEntry *entry)
{
	// skip files that are on volumes that don't support attributes
	BVolume v;
	entry->GetVolume(&v);
	return v.KnowsAttr();
}


/*------------------------------------------------------------------------*/

status_t TMimeWorker::UpdateMimeInfo(BMessage *msg)
{
	const char	*str_path;
	bool		recurse = false;
	bool		all = false;
	int32		force = 0;
	bool		meta_update = false;
	status_t	err = B_OK;
	sem_id		hangup_sem = B_BAD_SEM_ID;

	msg->FindBool("recurse", &recurse);
	msg->FindBool("all", &all);
	msg->FindInt32("force", &force);
	msg->FindBool("app_meta_mime", &meta_update);

	if (msg->HasInt32("hangup_sem"))
		msg->FindInt32("hangup_sem", (int32 *) &hangup_sem);

//+	SERIAL_PRINT(("UpdateMimeInfo, all=%d, force=%d, meta=%d, r=%d\n",
//+		all, force, meta_update, recurse));

	if (all) {
		TTask	*task;

		if (meta_update) {
			if (force) {
				// Doing a full rebuild so wipe out the current table
				gMimeTable->Lock();
				BMessage	*t;
				t = gMimeTable->Table(SUPPORTING_APPS_TABLE);
				t->MakeEmpty();
				t = gMimeTable->Table(FILE_SUFFIX_TABLE);
				t->MakeEmpty();
				gMimeTable->Unlock();
			}
			task = new TUpdateAppsTask(-1, 0, force, hangup_sem);
			err = task->Process();
			delete task;
			task = new TUpdateSuffixTask(-1, 0, hangup_sem);
			err = task->Process();
		} else {
			task = new TMimeSetTask(-1, 0, false, force, hangup_sem);
			err = task->Process();
		}

		if (err == B_ENTRY_NOT_FOUND)
			err = B_OK;

		gMimeTable->SaveTable();

		delete task;

		goto done;
	}

	if (meta_update && recurse) {
		err = B_BAD_VALUE;
		goto done;
	}

	if (msg->HasRef("ref"))
		BErr << "*** Received 'ref' update!" << endl;
		
	{
		int32 i = 0;
		while (msg->FindString("path", i++, &str_path) == B_OK) {
			if (recurse) {
	//+			BStopWatch	timer(str_path);
				BPath		path;
				BEntry		entry;
				TNodeWalker	walk(str_path, true);
				while ((err = walk.GetNextEntry(&entry)) == B_OK) {
	
					// skip files that are on volumes that don't support attributes
					if (!_supports_attr_(&entry))
						continue;
	
					entry.GetPath(&path);
	//+				SERIAL_PRINT(("path: %s\n", path.Path()));
					err = _update_mime_info_(path.Path(), force);
				}
				if (err == B_ENTRY_NOT_FOUND)
					err = B_OK;
			} else {
	//+			SERIAL_PRINT(("one path: %s\n", str_path));
				BEntry		entry;
				err = entry.SetTo(str_path);
				if (err)
					continue;
	
				// skip files that are on volumes that don't support attributes
				if (!_supports_attr_(&entry))
					continue;
	
				if (meta_update)
					err = _update_app_(str_path, force);
				else
					err = _update_mime_info_(str_path, force);
			}
		}
	}
	{
		int32 i = 0;
		entry_ref ref;
		while (msg->FindRef("refs", i++, &ref) == B_OK) {
			BPath	path;
			BEntry	entry;
			if (recurse) {
				TNodeWalker	walk(&ref, true);
				while ((err = walk.GetNextEntry(&entry)) == B_OK) {
	
					// skip files that are on volumes that don't support attributes
					if (!_supports_attr_(&entry))
						continue;
	
					entry.GetPath(&path);
//+					SERIAL_PRINT(("ref: %s\n", path.Path()));
					err = _update_mime_info_(path.Path(), force);
				}
				if (err == B_ENTRY_NOT_FOUND)
					err = B_OK;
			} else {
				if ((err = entry.SetTo(&ref)) == B_OK) {
	
					// skip files that are on volumes that don't support attributes
					if (!_supports_attr_(&entry))
						continue;
	
					entry.GetPath(&path);
//+					SERIAL_PRINT(("one ref: %s\n", path.Path()));
					if (meta_update)
						err = _update_app_(path.Path(), force);
					else
						err = _update_mime_info_(path.Path(), force);
				}
			}
		}
	}


done:

	if (msg->IsSourceWaiting()) {
		BMessage reply;
		reply.AddInt32("error", err);
		msg->SendReply(&reply);
	}

	return err;
}

/*------------------------------------------------------------------------*/

TMimeWorker *TMimeWorker::NewWorker(const char *name, int32 prio)
{
	BAutolock	a(TMimeWorker::sLock);
	TMimeWorker	*w;

	// The first worker created is special. 
	if (TMimeWorker::sList.CountItems() == 0)
		w = new TMainMimeWorker(name, prio);
	else
		w = new TMimeWorker(name, prio);
	TMimeWorker::sList.AddItem(w);

	return w;
}

/*------------------------------------------------------------------------*/

void TMimeWorker::TerminateWorkers()
{
	TMimeWorker		*w;
	int				i = 0;

	ASSERT(gMainWorker);

	while (1) {
		{
			BAutolock		a(sLock);
			w = (TMimeWorker *) sList.ItemAt(i);
			if (!w)
				break;
			else if (w == gMainWorker) {
				i += 1;
				continue;			// do main worker last!
			}
		}
		if (w->Lock())
			w->Quit();
	}

	if (gMainWorker->Lock())
		gMainWorker->Quit();
	gMainWorker = NULL;
}

/*------------------------------------------------------------------------*/

void TMimeWorker::PulseAll()
{
	if (gMainWorker)
		gMainWorker->PostMessage(B_PULSE);
}

/*------------------------------------------------------------------------*/

void TMimeWorker::Configure(TTask *task)
{
	fTask = task;
}

/*------------------------------------------------------------------------*/

void TMimeWorker::Pulse()
{
	status_t	err;
//+	fCounter++;
//+	SERIAL_PRINT(("MimeWorker::Pulse \"%s\"\n", Name()));

	if (fTask) {
		err = fTask->Process();
		if (err) {
			ASSERT(gMainWorker);
//+			SERIAL_PRINT(("task \"%s\" returned an err.\n", fTask->Name()));
			BMessage	msg(_CMD_TASK_COMPLETED_);
			msg.AddInt32("worker", (int32) this);
			gMainWorker->PostMessage(&msg, NULL, this);
			fTask = NULL;
		}
	}
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

static struct message_map mime_monitor_map[] = {
	{ B_REQUEST_LAUNCHED, B_META_MIME_CHANGED },
	{ 0, 0}
};

TMainMimeWorker::TMainMimeWorker(const char *name, int32 prio)
	:	TMimeWorker(name, prio),
		fTaskList(10),
		fMonitor(mime_monitor_map),
		fMimeSniffer(true)
{
	ASSERT(!gMainWorker);

	gMainWorker = this;
	fQueryIssueTime = 0;

	fPrevActivity = activity_level();
	fPrevTime = system_time();
	fLastCPUActiveTime = fPrevTime;

	fSubordinate = NULL;
	fSnifferEnabled = true;

	TTask	*task;
	fTaskIndex = 0;
	fSaveTableTicker = 10;

	// make sure that the boot volume has BEOS:APP_SIG as an index
	BVolume	boot;
	fVolRoster.GetBootVolume(&boot);
	if (boot.KnowsQuery())
		setup_all_indexes(boot.Device());

//+	fCurrentTask = task = new TCreateIndexTask();
//+	fTaskList.AddItem(task);

	fCurrentTask = task = new TUpdateAppsTask(DEFAULT_FILES_TO_PROCESS, fPulseRate);
	fTaskList.AddItem(task);

	task = new TMimeSetTask(DEFAULT_FILES_TO_PROCESS, fPulseRate);
	fTaskList.AddItem(task);

	task = new TUpdateSuffixTask(DEFAULT_FILES_TO_PROCESS, fPulseRate);
	fTaskList.AddItem(task);

	gMimeTable = fMimeTable = new TMimeTable();
}

/*------------------------------------------------------------------------*/

TMainMimeWorker::~TMainMimeWorker()
{
	delete gMimeTable;
}

/*------------------------------------------------------------------------*/

void TMainMimeWorker::Pulse()
{
	// ----------
	if (--fSaveTableTicker < 0) {
		fSaveTableTicker = 10;
		// save off this mime table every so often. SaveTable only writes
		// out the file if it is actually dirty.
		gMimeTable->SaveTable();
	}
	// ----------

	bool	is_idle = IdleCheck();
//+	SERIAL_PRINT(("IdleCheck = %d \n", is_idle));
	if (is_idle && !fSubordinate) {
		SERIAL_PRINT(("starting up the \"%s\", time=%d\n",
			fCurrentTask->Name(), time(NULL)));

		ASSERT(fCurrentTask);

		fSubordinate = NewWorker(fCurrentTask->Name(), B_NORMAL_PRIORITY);
		fSubordinate->Configure(fCurrentTask);
		fSubordinate->Run();
	} else {
//+		if (!fSubordinate)
//+			SERIAL_PRINT(("idling (%d)\n", is_idle));
	}

	BAutolock	a(sLock);
	int			i = 0;

	while (TMimeWorker *w = (TMimeWorker *) sList.ItemAt(i++)) {
		if (w == this)
			continue;
		if (is_idle) {
			// system is idle so give all worker a PULSE message
			w->fPulseNext -= sStdPulseRate;
			if (w->fPulseNext <= 0) {
				w->fPulseNext = w->fPulseRate;
				w->PostMessage(B_PULSE);
			}
		} else {
			if (w == fSubordinate) {
				fSubordinate = NULL;
				w->PostMessage(B_QUIT_REQUESTED);
				SERIAL_PRINT(("quitting worker \"%s\", time=%d (activity)\n",
					fCurrentTask->Name(), time(NULL)));
			}
		}
	}
}

void TMainMimeWorker::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case CMD_SNIFFER_CONTROL: {
			bool enable;
			if (msg->FindBool("enable", &enable) == B_OK) {
				SERIAL_PRINT(("CMD_SNIFFER_CONTROL(%d)\n", enable));
				fSnifferEnabled = enable;
			}
			break;
		}
		case CMD_FIRST_WAKE_UP:

			fVolRoster.StartWatching(BMessenger(this));
			msg->SendReply(B_REPLY);
			break;
		case B_NODE_MONITOR:
			{
				int32 opcode;
				if ((msg->FindInt32("opcode", &opcode) == B_OK) &&
					(opcode == B_DEVICE_MOUNTED)) {
					dev_t	dev;
					if (msg->FindInt32("new device", &dev) == B_OK) {
						BVolume	vol(dev);
						if (vol.KnowsQuery())
							setup_all_indexes(dev);
					}
				}
			}
			break;
		case CMD_UPDATE_MIME_INFO:
			{
				bool recurse;
				msg->FindBool("recurse", &recurse);
	
				// If we're recursing then spawn a new thread to do the work
				// so that the TMainMimeWorker isn't busy for too long a time.
				if (recurse) {
					TMimeWorker *w = NewWorker("mime updater", B_NORMAL_PRIORITY);
					w->PostMessage(msg);
					w->Run();
				} else 
					UpdateMimeInfo(msg);
			}
			break;

		case _CMD_TASK_COMPLETED_: {
			SERIAL_PRINT(("worker finished \"%s\", time=%d, worker=%x, %x\n",
				fCurrentTask->Name(), time(NULL), msg->FindInt32("worker"),
				fSubordinate));
//+			PRINT_OBJECT((*msg));
//+			msg->AddInt32("bogus", 1);

			fTaskIndex++;
			fCurrentTask = (TTask *) fTaskList.ItemAt(fTaskIndex);
			if (!fCurrentTask) {
				SERIAL_PRINT(("Reached the end of task list, sleep for a bit\n\n"));

				fTaskIndex = 0;
				fCurrentTask = (TTask *) fTaskList.ItemAt(0);

				// finished the entire list of tasks so we're done
				// for awhile.
				gState->fLastCompleteCycle = time(NULL);
				if (fSubordinate->Lock()) {
					fSubordinate->Quit();
				}
				fSubordinate = NULL;
			} else {
				// get the next task going
				if (fSubordinate && fSubordinate->Lock()) {
					SERIAL_PRINT(("starting up (next) \"%s\", sub=%x, t=%d\n",
						fCurrentTask->Name(), fSubordinate, time(NULL)));
					fSubordinate->Configure(fCurrentTask);
					fSubordinate->Unlock();
				}
			}
			break;
		}
		case CMD_MONITOR_META_MIME:
			{
				bool		start;
				uint32		emask;
				BMessenger	target;
				status_t err = msg->FindBool("start", &start);
				if (!err) {
					err = msg->FindMessenger("target", &target);
					if (!err) {
						err = msg->FindInt32("event_mask", (int32 *) &emask);
						if (start)
							err = fMonitor.Add(target, emask);
						else
							err = fMonitor.Remove(target);
					}
				}
				BMessage reply;
				reply.AddInt32("error", err);
				msg->SendReply(&reply);
				break;
			}
		
		case CMD_META_MIME_CHANGED:
			MetaMimeChanged(msg);
			break;

		case CMD_UPDATE_SNIFFER_RULE:
			{
				const char *type = NULL;
				const char *rule = NULL;

				if (msg->FindString("type", &type) != B_OK)
					break;
				if (msg->FindString("rule", &rule) != B_OK)
					break;
				
				fMimeSniffer.RuleChanged(type, rule);				
				break;
			}


		case CMD_GUESS_MIME_TYPE:
			{
				// snif a file or an in-memory buffer
				entry_ref ref;
				const void *data = NULL;
				int32 size;
				const char *tmp;
				BString path;
				const char *filename;
				BString fileString;
				status_t error = B_OK;

				if (msg->FindRef("refs", &ref) == B_OK) {
					BPath tmp(&ref);
					path = tmp.Path();
				} else if (msg->FindString("path", &tmp) == B_OK) {
					path = tmp;
				} else if (msg->FindString("filename", &filename) == B_OK) {
					fileString.SetTo(filename);
				} else if (msg->FindData("data", B_RAW_TYPE, &data, &size) != B_OK)
					error = B_ERROR;
					
				char type[B_MIME_TYPE_LENGTH];
				if (path.Length()) {
					BEntry entry(&ref);
					error = entry.InitCheck();
					error = _guess_mime_type_(path.String(), type);
				} else if (fileString.Length()) {
					BString resultMime;
					error = gMainWorker->SniffFilename(fileString.String(), resultMime);
					if(error == B_OK)
						resultMime.CopyInto(type, 0, B_MIME_TYPE_LENGTH);
				} else if (data) {
					ASSERT(data);
					BString tmp;
					error = gMainWorker->SniffData(data, size, tmp);
					if (error == B_OK)
						tmp.CopyInto(type, 0, B_MIME_TYPE_LENGTH);
				}
				
				BMessage reply;
				reply.AddInt32("error", error);
				if (error == B_OK)
					reply.AddString("type", type);
				msg->SendReply(&reply);
				break;
			}

		default:
			_inherited::MessageReceived(msg);
	}
}

void 
TMainMimeWorker::MetaMimeChanged(BMessage *message)
{
	BMessage notice(*message);
	notice.what = B_META_MIME_CHANGED;
	fMonitor.Notify(&notice);
}

status_t
TMainMimeWorker::SniffFile(const char *name, BFile *file, BString &type)
{
	bool emptyFile = (file->Seek(0, SEEK_END) <= 0);

	BAutolock lock(this);
	if (!lock.IsLocked())
		return B_ERROR;
	
	SniffBuffer buffer(file);
	if (!emptyFile) {
		// match using the sniffer rules
		const SnifferRuleBase *result = fMimeSniffer.Sniff(&buffer);
		if (result) {
			type = result->Type();
			return B_OK;
		}
	}
	
	// match by suffix, works even on empty files
	const char *suffix = strrchr(name, '.');
	if (suffix != NULL && *(suffix + 1)) {
		BMessage possible;
		if (gMimeTable->GetPossibleTypes(suffix + 1, &possible) == B_OK) {
			const char *tmp;
			if (possible.FindString("types", &tmp) == B_OK) {
				type = tmp;
				return B_OK;
			}
		}
	}

	if (!emptyFile) {
		// as a last step match to text
		//
		// Text is the only sniffer rule matched after using suffixes for matches
		// because a lot of text files will have a specific suffix that will define
		// the file's type, where an algorithmic sniffer would just clasify it as
		// plain text
		AlgorithmicSniffer textSniffer("text/plain", 0.1, &Sniffer::LooksLikeText, 0, 64);
		if (textSniffer.Match(&buffer)) {
			type = "text/plain";
			return B_OK;
		}
	}

	// unknown type, return generic file
	type = "application/octet-stream";

	return B_OK;
}

status_t 
TMainMimeWorker::SniffData(const void *data, int32 size, BString &type)
{
	if (size <= 0)
		return B_ERROR;

	BAutolock lock(this);
	if (!lock.IsLocked())
		return B_ERROR;
	
	// match using the sniffer rules
	SniffBuffer buffer(data, size);
	const SnifferRuleBase *result = fMimeSniffer.Sniff(&buffer);
	if (result) {
		type = result->Type();
		return B_OK;
	}
	
	AlgorithmicSniffer textSniffer("text/plain", 0.1, &Sniffer::LooksLikeText, 0, 64);
	if (textSniffer.Match(&buffer)) {
		type = "text/plain";
		return B_OK;
	}

	// unknown type, return generic file
	type = "application/octet-stream";

	return B_OK;
}

status_t 
TMainMimeWorker::SniffFilename(const char *data, BString &type)
{
	BAutolock lock(this);
	if (!lock.IsLocked())
		return B_ERROR;
	
	// match by suffix
	const char *suffix = strrchr(data, '.');
	if (suffix != NULL && *(suffix + 1)) {
		BMessage possible;
		if (gMimeTable->GetPossibleTypes(suffix + 1, &possible) == B_OK) {
			const char *tmp;
			if (possible.FindString("types", &tmp) == B_OK) {
				type = tmp;
				return B_OK;
			}
		}
	}

	// unknown type, return generic file
	type = "application/octet-stream";

	return B_OK;
}


/*------------------------------------------------------------------------*/

bool TMainMimeWorker::IdleCheck()
{
	if (!fSnifferEnabled)
		return false;

	bool		result = false;
	time_t		now = time(NULL);
	bigtime_t	ui_idle = idle_time();
	time_t		ui_idle_sec = (ui_idle/1000000);
	bigtime_t	act = activity_level();
	bigtime_t	t = system_time();
	bool		ui_is_idle = false;
	bool		cpu_is_idle = false;

	float		p = ((float) (act - fPrevActivity)) /
		((float) (t - fPrevTime));

	if (fSubordinate) {
		p -= (SCAN_USAGE_MIN_THRESHOLD);	// adjust for current sniffers
	}

	fPrevActivity = act;
	fPrevTime = t;
//+	SERIAL_PRINT(("cpu usage = %.2f\n", p));

	if (p > IDLE_USAGE_THRESHOLD) {
		// cpu activity is above the limit
		fLastCPUActiveTime = t;
	} else if (((t - fLastCPUActiveTime)/1000000) > IDLE_TIME_THRESHOLD) {
		cpu_is_idle = true;
	}

	if (ui_idle_sec > IDLE_TIME_THRESHOLD)
		ui_is_idle = true;

	if (((now - gState->fLastCompleteCycle) > MIN_TIME_BETWEEN_SCANS) &&
		ui_is_idle && cpu_is_idle) {
			result = true;
	}

//+	time_t	cpu_idle_sec = (t - fLastCPUActiveTime)/1000000;
//+	SERIAL_PRINT(("IdleCheck = %d (ui_idle=%d, cpu_idle=%d)\n",
//+		result, ui_idle_sec, cpu_idle_sec));
	return result;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

status_t _guess_mime_type_(const char *path, char *type)
{
	BEntry entry(path);
	status_t error = entry.InitCheck();
	if (error != B_OK)
		return error;

	if (entry.IsDirectory())
		// special-case folder
		strcpy(type, FOLDER_MIME_TYPE);
	else if (entry.IsSymLink())
		strcpy(type, "application/x-vnd.Be-symlink");
	else {

		BFile file(&entry, O_RDONLY);
		error = file.InitCheck();
		if (error != B_OK)
			return error;
			
		BString result;
		error = gMainWorker->SniffFile(path, &file, result);
		if (error != B_OK)
			return error;

		result.CopyInto(type, 0, B_MIME_TYPE_LENGTH - 1);

	}
	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t _update_mime_info_(const char *path, int32 forceLevel)
{
	BFile file;
	BNode dirOrLink;
	BNode *node = NULL;

	bool force = forceLevel > 0;
	
	if (path == NULL) 
		return B_ERROR;

	BEntry entry(path);
	status_t err = entry.InitCheck();
	if (err != B_OK)
		return err;

	// only allow strong force (-F) on bfs disks, check if we are on one
	struct stat	sinfo;
	err = entry.GetStat(&sinfo);
	if (err != B_OK)
		return err;

	if (forceLevel > 1) {
		fs_info dinfo;
		err = fs_stat_dev(sinfo.st_dev, &dinfo);
		if (err != B_OK)
			return errno;
	
	 	if (strcmp(dinfo.fsh_name, "bfs") != 0)
			forceLevel = 1;
	}


	// open a BNode for a symlink or directory, BFile O_RDWR for files
	// to ensure the right write mode
	bool isLink = entry.IsSymLink();
	bool isDir = entry.IsDirectory();
	
	if (isLink || isDir) {
		dirOrLink.SetTo(&entry);
		node = &dirOrLink;
	} else {
		file.SetTo(&entry, O_RDWR);
		node = &file;
	}
	
	err = node->InitCheck();
	if (err != B_OK)
		return err;

	// read any type the node already may have
	char currentType[B_MIME_TYPE_LENGTH];
	currentType[0] = '\0';
	bool fresh = (node->ReadAttr(B_MIME_TYPE_ATTR, B_MIME_STRING_TYPE,
		0, currentType, B_MIME_TYPE_LENGTH) <= 0);
	
	// treat the 'octet-stream' or Invalid mime type as a fresh file.
	if (!fresh
		&& (strcasecmp(currentType, "application/octet-stream") == 0
			|| !BMimeType::IsValid(currentType)))
		fresh = true;

	// set up the resulting type
	BString type;
	if (isDir) 
		type = FOLDER_MIME_TYPE;
	else if (isLink) 
		type = "application/x-vnd.Be-symlink";
	else {
		if (fresh || force)
			// ask the sniffer
			err = gMainWorker->SniffFile(path, &file, type);
		else
			// go with the old type
			type = currentType;
	}

	if (err != B_OK)
		return err;

	if (!type.Length()) {
		TRESPASS();
		return B_ERROR;
	}

	if (type.ICompare(B_APP_MIME_TYPE) == 0) {
		//  We're dealing with an application

		BAppFileInfo app_info(&file);
		// move resource info into attributes.

		if (fresh || force) {
			char buf[B_MIME_TYPE_LENGTH];
			// make sure that the file type is saved as a resource
			// SetType will write out info to both places.

			if (forceLevel < 2 && app_info.GetType(buf) == B_OK) {
				// The type attr exists. See if there is already a rsrc. If
				// so then only update the rsrc if they differ.
				char rsrc_type[B_MIME_TYPE_LENGTH];
				app_info.fWhere = B_USE_RESOURCES;
				status_t err = app_info.GetType(rsrc_type);
				app_info.fWhere = B_USE_BOTH_LOCATIONS;

				// If there's no type in rsrc area of if the types differ
				// then call SetType to update things.
				if (err != B_OK || strcmp(rsrc_type, buf) != 0)
					app_info.SetType(buf);
			} else {
				// do a lazy SetType, touching only the part of the resource/attribute
				// pair that is out of sync
				info_location oldWhere = app_info.fWhere;
				info_location writeTo = (info_location)0;
				app_info.fWhere = B_USE_RESOURCES;
				char buf[B_MIME_TYPE_LENGTH];
				if (app_info.GetType(buf) != B_OK || type != buf)
					writeTo = B_USE_RESOURCES;
					
				app_info.fWhere = B_USE_ATTRIBUTES;
				if (app_info.GetType(buf) != B_OK || type != buf)
					writeTo = (info_location)(writeTo | B_USE_ATTRIBUTES);
				
				if (writeTo != (info_location)0) {
					app_info.fWhere = writeTo;
					app_info.SetType(type.String());
				}
				app_info.fWhere = oldWhere;
			}
			
			app_info.UpdateFromRsrc();

			if (fresh) {
				// if no x-bits are set then set x-bits to match read bits
				if (err == B_OK
					&& (sinfo.st_mode & S_IXUSR) == 0
					&& (sinfo.st_mode & S_IXGRP) == 0
					&& (sinfo.st_mode & S_IXOTH) == 0) {
					// no x-bits are set.
					if (sinfo.st_mode & S_IRUSR)		// if user read
						sinfo.st_mode |= S_IXUSR;
					if (sinfo.st_mode & S_IRGRP)		// if group read
						sinfo.st_mode |= S_IXGRP;
					if (sinfo.st_mode & S_IROTH)		// if other read
						sinfo.st_mode |= S_IXOTH;
					chmod(path, sinfo.st_mode);
				}
			}
			force = true;	// for the call to _real_update_app_
		}
		if (forceLevel > 1) {
			// when doing a mimeset -F style update, we have to remove the original
			// metamime so that it can get freshly reinstalled
			
			char sig[B_MIME_TYPE_LENGTH];
			err = app_info.GetSignature(sig);
			if (err == B_OK) {
				BMimeType meta(sig);
				meta.Delete();
			}
		}
		_real_update_app_(&app_info, path, force);

	} else {
		// the 'force' option will NOT change the mime type of a file
		// if it already has one, unless the forceLevel is 2
		if (fresh || forceLevel > 1)
			node->WriteAttr(B_MIME_TYPE_ATTR, B_MIME_STRING_TYPE, 0,
				type.String(), type.Length() + 1);
	}

	return err;
}

/*------------------------------------------------------------------------*/

status_t _real_update_app_(BAppFileInfo *ainfo, const char *path, bool force)
{
	char sig[B_MIME_TYPE_LENGTH];
	BMimeType meta;

	status_t err = ainfo->GetSignature(sig);
	if (!err) 
		meta.SetTo(sig);

	uint32 changes;
	err = ainfo->UpdateMetaMime(path, force, &changes);
	if (err != B_OK)
		return err;

	
	if (changes != 0) {
		BMessage m(B_META_MIME_CHANGED);
		m.AddInt32("be:which", (int32) changes);
		m.AddString("be:type", sig);
		gMainWorker->Monitor()->Notify(&m);
	}

	BMessage supported;
	bool has_new = (ainfo->GetSupportedTypes(&supported) == B_OK);

	gMimeTable->Lock();
	gMimeTable->UpdateSupportingApps(sig, has_new ? &supported : NULL, false);
	gMimeTable->Unlock();

	return err;
}

#endif

/*------------------------------------------------------------------------*/

status_t _update_app_(const char *path, bool force)
{
#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING
//+	SERIAL_PRINT(("Update App: %s (force=%d)\n", path ? path : "null", force));
	status_t	err;
	BFile		file(path, O_RDONLY);

	if ((err = file.InitCheck()) != B_OK)
		return err;

	BAppFileInfo	ainfo(&file);
	if ((err = ainfo.BNodeInfo::InitCheck()) != B_OK) {
//+		SERIAL_PRINT(("error1 = %x (%s)\n", err, strerror(err)));
		return err;
	}

	return _real_update_app_(&ainfo, path, force);
#else
	return B_OK;
#endif
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

