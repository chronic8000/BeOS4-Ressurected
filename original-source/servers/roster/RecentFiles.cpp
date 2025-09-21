
#include <Debug.h>
#include <Path.h>
#include <Message.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <SettingsHandler.h>

#include <algorithm>
#include <vector>

#include "main.h"

// #define _RECENT_FILES_TESTING_

#ifdef DEBUG
	
#if 0
	FILE *logFile = 0;
	
	inline void PrintToLogFile(const char *fmt, ...)
	{
    	va_list ap; 
        va_start(ap, fmt); 
		vfprintf(logFile, fmt, ap);
		va_end(ap); 		
	}

	#define PRINT(_ARGS_)														\
		if (logFile == 0) 														\
			logFile = fopen("/var/log/registrar.log", "a+"); 					\
		if (logFile != 0) {														\
			PrintToLogFile _ARGS_;												\
			fflush(logFile);													\
		}

	#define WRITELOG(_ARGS_)													\
		if (logFile == 0) 														\
			logFile = fopen("/var/log/registrar.log", "a+"); 					\
		if (logFile != 0) {														\
			thread_info info;													\
			get_thread_info(find_thread(NULL), &info);							\
			PrintToLogFile("[t %Ld] \"%s\" (%s:%i) ", system_time(),			\
				info.name, __FILE__, __LINE__);									\
			PrintToLogFile _ARGS_;												\
			fflush(logFile);													\
		}
#endif
	
#else

	#define WRITELOG(_ARGS_)

#endif

StringStash stringStash;


StringStash::StringStash()
	:	stash(100, true)
{
}

struct AddUniqueStringPredicate : UnaryPredicate<BString> {
	AddUniqueStringPredicate(const char *matchThis)
		:	matchThis(matchThis)
		{}
	
	virtual int operator()(const BString *item) const
		{ return item->Compare(matchThis); }
	
	const char *matchThis;
};

const char *
StringStash::AddUnique(const char *str)
{
	AddUniqueStringPredicate pred(str);
	bool alreadyInList;
	int32 index = stash.FindBinaryInsertionIndex(pred, &alreadyInList);
	if (!alreadyInList)
		stash.AddItem(new BString(str), index);
	
	return stash.ItemAt(index)->String();
}


RecentFileItem::RecentFileItem(const char *path, const char *sig, int64 accessCount)
	:	appSigs(5, true)
{
	unresolvedPath = path;

	// enter the string into the stash if needed
	sig = stringStash.AddUnique(sig);
	// add a new entry in the app sig vector
	appSigs.AddItem(new std::pair<const char *, int64>(sig, accessCount));
}

const entry_ref *
RecentFileItem::Ref() const
{
	if (unresolvedPath.Length()) {
		BEntry entry(unresolvedPath.String());
		entry_ref tmp;
		if (entry.GetRef(&tmp) == B_OK) {
			// got it this time
			ref = tmp;
			unresolvedPath = "";
		}
	}
	return &ref;
}

void 
RecentFileItem::GetPath(BString *path) const
{
	if (unresolvedPath.Length())
		*path = unresolvedPath;
	else {
		BPath tmp(&ref);
		*path = tmp.Path();
	}
}


int
MatchStringInPairs(const std::pair<const char *, int64> *item1,
	const std::pair<const char *, int64> *item2)
{
	return strcasecmp(item1->first, item2->first);
}


bool 
RecentFileItem::Access(const char *sig, int64 accessCount)
{
	std::pair<const char *, int64> tmp(sig, accessCount);
	int32 oldCount = appSigs.CountItems();
	std::pair<const char *, int64> *currentItem
		= appSigs.BinaryInsertCopyUnique(tmp, MatchStringInPairs);

	if (oldCount != appSigs.CountItems()) {
		// actually inserted the item
		// this means that the signature might not be in the stash yet, add it
		currentItem->first = stringStash.AddUnique(sig);
			// after adding the signature, the string stays the same
			// but the pointer changes, deal with that
		return true;
	}
	
	// item is already in the list, just update the access count
	currentItem->second = accessCount;
	return false;
}

RecentItemsDict::RecentItemsDict(int32 fileListLimit, int32 folderListLimit,
	int32 appListLimit)
	:	fileItems(fileListLimit, true),
		folderItems(folderListLimit, true),
		recentApps(appListLimit, true),
		fileAccessCount(0),
		folderAccessCount(0),
		fileCount(0),
		folderCount(0),
		fileListLimit(fileListLimit),
		folderListLimit(folderListLimit),
		appListLimit(appListLimit)
{
}

RecentFileItem * 
RecentItemsDict::AccessCommon(const char *path, const char *appSig,
	BObjectList<RecentFileItem> &list, int32 &count, int32 max, int64 &accessCount)
{
	RecentFileItem *item = FindByPath(path, list);
	if (!item) {
		SERIAL_PRINT(("adding new item\n"));
		TrimToCount(list, max, accessCount, count);
		item = new RecentFileItem(path, appSig, accessCount);
		list.AddItem(item);
		count++;
	} else if (item->Access(appSig, accessCount)) {
		// not a new document but a new app sig entry, bump count up
		count++;
		// and trim the list if needed
		TrimToCount(list, max, accessCount, count);
	}
	
	accessCount++;
	return item;
}

void 
RecentItemsDict::AccessFile(const char *path, const char *appSig)
{
	AccessCommon(path, appSig, fileItems, fileCount, fileListLimit, fileAccessCount);
}

void 
RecentItemsDict::AccessFile(const entry_ref *ref, const char *appSig)
{
	BPath path(ref);
	if (!path.Path())
		return;

	AccessCommon(path.Path(), appSig, fileItems, fileCount,
		fileListLimit, fileAccessCount);

	SERIAL_PRINT(("accessed %s for %s\n", path.Path(), appSig));
	Dump("recent file", fileItems);
}

void 
RecentItemsDict::AccessFolder(const entry_ref *ref, const char *appSig)
{
	BPath path(ref);
	if (!path.Path())
		return;

	AccessCommon(path.Path(), appSig, folderItems, folderCount,
		folderListLimit, folderAccessCount);

	SERIAL_PRINT(("accessed %s for %s\n", path.Path(), appSig));
	Dump("recent folder", fileItems);
}

void 
RecentItemsDict::TrimToCount(BObjectList<RecentFileItem> &list, int32 maxCount,
	int64 accessCount, int32 &totalCount)
{
	if (totalCount < maxCount + 100)
		// keep a hysteresis so we don't have to trim all the time
		return;

	int64 threshold = accessCount - (int64)maxCount;
		// any item with access count lower than <threshold> will get deleted
	int32 listCount = list.CountItems();
	
	for (int32 index = 0; index < listCount;) {
		RecentFileItem *item = list.ItemAt(index);
		
		int32 appSigCount = item->appSigs.CountItems();
		for (int32 appSigIndex = 0; appSigIndex < appSigCount; ) {
			std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
			if (appSigItem->second < threshold) {
				// delete the item we found
				delete item->appSigs.RemoveItemAt(appSigIndex);
				totalCount--;
				appSigCount--;
			} else
				appSigIndex++;
		}
		
		if (appSigCount == 0) {
			// no more entries for this entry_ref, get rid of it
			delete list.RemoveItemAt(index);
			listCount--;
		} else
			index++;
	}

	SERIAL_PRINT(("trimmed list to %d\n", totalCount));
	Dump("trimmed", list);
}

RecentFileItem *
RecentItemsDict::FindByRef(const entry_ref *ref, BObjectList<RecentFileItem> &list)
{
	int32 listCount = list.CountItems();
	for (int32 index = 0; index < listCount; index++)
		if (*list.ItemAt(index)->Ref() == *ref)
			return list.ItemAt(index);
	
	return 0;
}

RecentFileItem *
RecentItemsDict::FindByPath(const char *path, BObjectList<RecentFileItem> &list)
{
	int32 listCount = list.CountItems();
	for (int32 index = 0; index < listCount; index++) {
		BString itemPath;
		list.ItemAt(index)->GetPath(&itemPath);
		if (itemPath == path)
			return list.ItemAt(index);
	}
	
	return 0;
}


// sorting predicates
struct SortByAccessCountForAppSignaturePredicate : UnaryPredicate<RecentFileItem> {
	SortByAccessCountForAppSignaturePredicate(const char *appSig, int64 accessCount)
		:	appSig(appSig),
			accessCount(accessCount)
		{}
	
	virtual int operator()(const RecentFileItem *item) const
		{
			int32 appSigCount = item->appSigs.CountItems();
			for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) {
				std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
				if (strcasecmp(appSigItem->first, appSig) == 0)
					return appSigItem->second > accessCount ? -1 : 1;
			}
			TRESPASS();
			return 0;
		}
	
	const char *appSig;
	int64 accessCount;
};

struct SortByAccessCountPredicate : UnaryPredicate<RecentFileItem> {
	SortByAccessCountPredicate(int64 accessCount)
		:	accessCount(accessCount)
		{}
	
	virtual int operator()(const RecentFileItem *item) const
		{
			int64 itemMax = -1;
			int32 appSigCount = item->appSigs.CountItems();
			for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) 
				itemMax = std::max(itemMax, item->appSigs.ItemAt(appSigIndex)->second);
		
			return itemMax > accessCount ? -1 : 1;
		}
	
	int64 accessCount;
};

void
RecentItemsDict::CollectRecentItemsCommon(const BObjectList<RecentFileItem> &list,
	const BObjectList<BMimeType> *mimeTypeList, const char *appSig, int32 count,
	BMessage *result) const
{
	Dump("collect", list);

	BObjectList<RecentFileItem> recentItemsSortedByAccess;

	int32 typeCount = mimeTypeList ? mimeTypeList->CountItems() : 0;
	bool bySignatureOnly = (appSig != 0 && typeCount == 0);
	
	// build a list of recent items sorted by access count
	// this is an optimization that allows us to cut down on disk access
	// needed when determining the file types 
	int32 listCount = list.CountItems();
	for (int32 index = 0; index < listCount; index++) {
		RecentFileItem *item = list.ItemAt(index);
		
		if (bySignatureOnly) {
			// only include items that match the desired app signature
			// sort by the access count for app sig
			int32 appSigCount = item->appSigs.CountItems();
			for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) {
				std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
				if (strcasecmp(appSigItem->first, appSig) == 0) {
					SortByAccessCountForAppSignaturePredicate predicate(appSig, appSigItem->second);
					recentItemsSortedByAccess.BinaryInsert(item, predicate);
				}
			}
		} else {
			// include all items, sort by the highest access count
			int32 appSigCount = item->appSigs.CountItems();
			int64 highestAccessCount = -1;
			for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++)
				highestAccessCount = std::max(highestAccessCount,
					item->appSigs.ItemAt(appSigIndex)->second);

			SortByAccessCountPredicate predicate(highestAccessCount);
			recentItemsSortedByAccess.BinaryInsert(item, predicate);
		}		
	}
	
	Dump(bySignatureOnly ? "sortByAccessSig---------" : "sortByAccess------------",
		recentItemsSortedByAccess);
	
	listCount = recentItemsSortedByAccess.CountItems();
	int32 matchCount = 0;
	for (int32 index = 0; index < listCount; index++) {
		RecentFileItem *item = recentItemsSortedByAccess.ItemAt(index);

		bool match = (!appSig && typeCount == 0);
		
		if (!match && appSig) {
			int32 appSigCount = item->appSigs.CountItems();
			for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) {
				std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
				if (strcasecmp(appSigItem->first, appSig) == 0) {
					match = true;
					break;
				}
			}
		}

		if (!match && typeCount) {
			// find if the type of this document matches any of the
			// mime types in the list
			BNode node(item->Ref());
			BNodeInfo info(&node);
			
			char typeBuffer[256];
			if (info.GetType(typeBuffer) == B_OK) {
				BMimeType nodeMimeType(typeBuffer);
				for (int32 index = 0; index < typeCount; index++) 
					if (mimeTypeList->ItemAt(index)->Contains(&nodeMimeType)) {
						match = true;
						break;
					}
			}
		}

		if (match) {

#if DEBUG
			BPath path(item->Ref());
			SERIAL_PRINT(("adding recent file: %s\n", path.Path()));
#endif
			result->AddRef("refs", item->Ref());
			matchCount++;
			// we have enough not, bail out
			if (matchCount >= count)
				break;
		}
	}
}

void
RecentItemsDict::CollectRecentFiles(const BObjectList<BMimeType> *mimeTypeList,
	const char *appSig, int32 count, BMessage *result) const
{
	SERIAL_PRINT(("collecting recent files for %s\n", appSig ? appSig : ""));
	Dump("recent file", fileItems);

	CollectRecentItemsCommon(fileItems, mimeTypeList, appSig, count, result);
}

void 
RecentItemsDict::CollectRecentFolders(const char *appSig, int32 count,
	BMessage *result) const
{
	SERIAL_PRINT(("2collecting recent folders for %s\n", appSig ? appSig : ""));
	Dump("recent folder", folderItems);

	CollectRecentItemsCommon(folderItems, 0, appSig, count, result);
}



void 
RecentItemsDict::AccessApp(const char *appSignature)
{
	DumpAppList();

	// if app already in the string, remove it and add it at the begining
	int32 count = recentApps.CountItems();
	for (int32 index = 0; index < count; index++) 
		if (recentApps.ItemAt(index)->ICompare(appSignature) == 0) {
			if (index == 0) {
				// already at the first place in the list,
				// we are done
				SERIAL_PRINT(("app %s already first in the list\n", appSignature));
				return;
			}

			// we are in the list, just move the entry first
			recentApps.AddItem(recentApps.RemoveItemAt(index), 0);
			SERIAL_PRINT(("app %s in the list, moved to first slot\n", appSignature));
			return;
		}
	
	// not yet in the list, check that the list is within limits and trim
	// if needed
	if (count > appListLimit) {
		SERIAL_PRINT(("trimming app list, was count %d\n", count));
		delete recentApps.RemoveItemAt(count - 1);
	}
	
	// add new entry
	recentApps.AddItem(new BString(appSignature), 0);
	SERIAL_PRINT(("added new app %s\n", appSignature));

	DumpAppList();
}

void 
RecentItemsDict::CollectRecentApps(int32 count, BMessage *result) const
{
	// return entry_refs for the app signatures in our list
	count = std::min(count, recentApps.CountItems());

#ifndef _RECENT_FILES_TESTING_

	entry_ref ref;
	for (int32 index = 0; index < count; index++) {
		app_info appInfo;
		Busy wait;
		if (gTheRoster->GetAppInfo(recentApps.ItemAt(index)->String(),
			&appInfo, &wait) == B_OK) {
			SERIAL_PRINT(("Collect recent apps: got a running app for %s\n",
				recentApps.ItemAt(index)->String()));
			result->AddRef("refs", &appInfo.ref);
		} else if (BRoster().FindApp(recentApps.ItemAt(index)->String(), &ref) == B_OK) {
			SERIAL_PRINT(("Collect recent apps: found preferred app for %s\n",
				recentApps.ItemAt(index)->String()));
			result->AddRef("refs", &ref);
		} else {
			SERIAL_PRINT(("Collect recent apps: couldn't get app info for %s\n",
				recentApps.ItemAt(index)->String()));
			continue;
		}
	}

#endif
}

#if DEBUG
void
RecentItemsDict::Dump(const char *label, const BObjectList<RecentFileItem> &list) const
{
	int32 listCount = list.CountItems();
	for (int32 index = 0; index < listCount; index++) {
		RecentFileItem *item = list.ItemAt(index);
		BEntry entry(item->Ref());
		BPath path(&entry);
		SERIAL_PRINT(("%s:%s - ", label, path.Path()));
		int32 appSigCount = item->appSigs.CountItems();
		for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) {
			std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
			SERIAL_PRINT(("%s %Ld:", appSigItem->first, appSigItem->second));
		}
		SERIAL_PRINT(("\n"));
	}
}

void 
RecentItemsDict::DumpAppList()
{
	SERIAL_PRINT(("appList------------\n"));
	int32 listCount = recentApps.CountItems();
	for (int32 index = 0; index < listCount; index++) {
		SERIAL_PRINT(("%s\n", recentApps.ItemAt(index)->String()));
	}
}

#endif


inline bool
IsMeta(char ch)
{
	return ch == '(' || ch == ')' || ch == '[' || ch == ']'
		|| ch == '*' || ch == '"' || ch == '\'' || ch == '?'
		|| ch == '^' || ch == '\\' || ch == ' ' || ch == '\t'
		|| ch == '\n' || ch == '\r';
}

static BString &
EscapeMetachars(BString &result, const char *str)
{
	result = "";
	
	const char *from = str;
	for (;;) {
		int32 count = 0;
		while(from[count] && !IsMeta(from[count]) )
			count++;
		result.Append(from, count);
		if (!from[count])
			break;
		char tmp[3] = { '\\', from[count], '\0'};
		result += tmp;
		from += count + 1;
	}
	return result;
}

static BString &
DeEscapeMetachars(BString &result, const char *str)
{
	result = "";
	
	const char *from = str;
	for (;;) {
		int32 count = 0;
		while(from[count] && from[count] != '\\' )
			count++;
		result.Append(from, count);
		if (!from[count])
			break;
		result += from[count + 1];
		from += count + 2;
	}
	return result;
}

const char *
EachArgvGlue(int argc, const char *const *argv, void *params)
{
	return ((RecentItemsDict *)params)->ReadOneArgv(argc, argv);
}

void 
RecentItemsDict::Read(const char *fileName)
{
	SERIAL_PRINT(("=================== reading recent files\n"));
	ArgvParser::EachArgv(fileName, &EachArgvGlue, this);
	Dump("read: recent files", fileItems);
	Dump("read: recent folders", folderItems);
	DumpAppList();
}

const char * 
RecentItemsDict::ReadOneArgv(int , const char *const *argv)
{
	if (!*argv)
		return "command expected";

	bool recentDoc = (strcmp(*argv, "RecentDoc") == 0);
	if (recentDoc || strcmp(*argv, "RecentFolder") == 0) {
	
		// handle recent folder or recent document
		// expect:
		// RecentDoc /boot/home/somefile "application/someApp" 13
		//
		BObjectList<RecentFileItem> *list = recentDoc ? &fileItems : &folderItems;

		if (!*++argv)
			return "document path expected";
		BString path;
		DeEscapeMetachars(path, *argv);

		RecentFileItem *item = FindByPath(path.String(), *list);
		if (item)
			return "item already in list";
		
		if (!*(argv + 1))
			return "at least one app signature expected";
		
		RecentFileItem *newItem = NULL;
		for (;;) {
			const char *signature = *++argv;
			if (!signature)
				break;
	
			if (strlen(signature) > 256)
				return "bad signature";

			if (!*++argv)
				return "access count expected";
	
			int32 accessCount = atoi(*argv);
			bool added = false;
			if (!newItem) {
				newItem = new RecentFileItem(path.String(), signature, accessCount);
				list->AddItem(newItem);
				added = true;
			} else
				// do a unique add
				added = newItem->Access(signature, accessCount);
			
			if (added && recentDoc) {
				if (accessCount > fileAccessCount + 1)
					fileAccessCount = accessCount + 1;
				fileCount++;
			} else if (added) {
				folderCount++;
				if (accessCount > folderAccessCount + 1)
					folderAccessCount = accessCount + 1;
			}
		}

	} else if (strcmp(*argv, "RecentApp") == 0) {
		// handle recent application
		if (!*++argv)
			return "signature expected";

		if (strlen(*argv) > 256)
			return "bad signature";
		
		recentApps.AddItem(new BString(*argv));
	} else
		return "unknown command";
	
	return 0;
}

status_t 
RecentItemsDict::Write(BFile *file, const char *str)
{
	ssize_t result = file->Write(str, strlen(str));
	return result > 0 ? B_OK : (status_t)result;
}

status_t 
RecentItemsDict::Write(BFile *file, const BString &str)
{
	ssize_t result = file->Write(str.String(), str.Length());
	return result > 0 ? B_OK : (status_t)result;
}

status_t 
RecentItemsDict::SaveRecentCommon(BFile *file, const char *label,
	const BObjectList<RecentFileItem> &list)
{
	int32 count = list.CountItems();
	int64 minAccessCount = LONGLONG_MAX;
	
	// find the lowest access count, so we can bump the counts down to start from zero
	for (int32 index = 0; index < count; index++) {
		RecentFileItem *item = list.ItemAt(index);

		int32 appSigCount = item->appSigs.CountItems();
		for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) 
			minAccessCount = std::min(minAccessCount, item->appSigs.ItemAt(appSigIndex)->second);		
	}
	
	// write out the
	for (int32 index = 0; index < count; index++) {
		BString line;
		line << label << " ";

		RecentFileItem *item = list.ItemAt(index);
		BEntry entry(item->Ref());
		BPath path(&entry);
		if (!path.Path())
			// should not happen, bail out
			return B_ENTRY_NOT_FOUND;

		BString escapedPath;
		EscapeMetachars(escapedPath, path.Path());
			// make sure the document path is properly escaped before we write it out
		line << escapedPath;
		
		int32 appSigCount = item->appSigs.CountItems();
		for (int32 appSigIndex = 0; appSigIndex < appSigCount; appSigIndex++) {
			std::pair<const char *, int64> *appSigItem = item->appSigs.ItemAt(appSigIndex);
			line << " \"" << appSigItem->first << "\" " << (appSigItem->second - minAccessCount);
				// write out access count bumped down to a base of zero
		}
		
		line << "\n";
		status_t result = Write(file, line);
		if (result != B_OK)
			return result;
	}
	return B_OK;
}

void 
RecentItemsDict::Save(const char *fileName)
{
	BEntry parentEntry(fileName);
	BDirectory parent;
	parentEntry.GetParent(&parent);
	
	BPath dirPath(&parent, "");
	if (!dirPath.Path()) {
		TRESPASS();
		// aaack! this is fatal, bail out
		return;
	}
	
	create_directory(dirPath.Path(), 0777);
	
	// nuke old settings
	BEntry entry(&parent, fileName);
	entry.Remove();
	
	BFile settings(&entry, O_RDWR | O_CREAT);
	if (settings.InitCheck() != B_OK)
		return;

	if (	Write(&settings, "# Recent documents \n") != B_OK
		|| 	SaveRecentCommon(&settings, "RecentDoc", fileItems) != B_OK
		|| 	Write(&settings, "\n# Recent folders \n") != B_OK
		||	SaveRecentCommon(&settings, "RecentFolder", folderItems) != B_OK
		|| 	Write(&settings, "\n# Recent applications \n") != B_OK)
		return;
		
	int32 count = recentApps.CountItems();
	for (int32 index = 0; index < count; index++) {
		BString line;
		line << "RecentApp " << *recentApps.ItemAt(index) << '\n';
		if (Write(&settings, line) != B_OK)
			return;
	}
	Write(&settings, "\n");
}

// #pragma mark -

// Copy of ArgvParser from libtracker.so, to avoid having to link
// the registrar against libtracker.so

ArgvParser::ArgvParser(const char *name)
	:	fFile(0),
		fBuffer(0),
		fPos(-1),
		fArgc(0),
		fCurrentArgv(0),
		fCurrentArgsPos(-1),
		fSawBackslash(false),
		fEatComment(false),
		fInDoubleQuote(false),
		fInSingleQuote(false),
		fLineNo(0),
		fFileName(name)
{
	fFile = fopen(fFileName, "r");
	if (!fFile) {
		SERIAL_PRINT(("Error opening %s\n", fFileName));
		return;
	}

	fBuffer = new char [kBufferSize];
	fCurrentArgv = new char * [1024];
}


ArgvParser::~ArgvParser()
{
	delete [] fBuffer;

	MakeArgvEmpty();
	delete [] fCurrentArgv;

	if (fFile)
		fclose(fFile);
}

void 
ArgvParser::MakeArgvEmpty()
{
	// done with current argv, free it up
	for (int32 index = 0; index < fArgc; index++)
		delete fCurrentArgv[index];
	
	fArgc = 0;
}

status_t 
ArgvParser::SendArgv(ArgvHandler argvHandlerFunc, void *passThru)
{
	if (fArgc) {
		NextArgv();
		fCurrentArgv[fArgc] = 0;
		const char *result = (argvHandlerFunc)(fArgc, fCurrentArgv, passThru);
		if (result)
			printf("File %s; Line %ld # %s", fFileName, fLineNo, result);
		MakeArgvEmpty();
		if (result)
			return B_ERROR;
	}

	return B_NO_ERROR;
}

void 
ArgvParser::NextArgv()
{
	if (fSawBackslash) {
		fCurrentArgs[++fCurrentArgsPos] = '\\';
		fSawBackslash = false;
	}
	fCurrentArgs[++fCurrentArgsPos] = '\0';
	// terminate current arg pos
	
	// copy it as a string to the current argv slot
	fCurrentArgv[fArgc] = new char [strlen(fCurrentArgs) + 1];
	strcpy(fCurrentArgv[fArgc], fCurrentArgs);
	fCurrentArgsPos = -1;
	fArgc++;
}

void 
ArgvParser::NextArgvIfNotEmpty()
{
	if (!fSawBackslash && fCurrentArgsPos < 0)
		return;

	NextArgv();
}

char 
ArgvParser::GetCh()
{
	if (fPos < 0 || fBuffer[fPos] == 0) {
		if (fFile == 0)
			return EOF;
		if (fgets(fBuffer, kBufferSize, fFile) == 0)
			return EOF;
		fPos = 0;
	}
	return fBuffer[fPos++];
}

status_t 
ArgvParser::EachArgv(const char *name, ArgvHandler argvHandlerFunc, void *passThru)
{
	ArgvParser parser(name);
	return parser.EachArgvPrivate(name, argvHandlerFunc, passThru);
}

status_t 
ArgvParser::EachArgvPrivate(const char *name, ArgvHandler argvHandlerFunc, void *passThru)
{
	status_t result;

	for (;;) {
		char ch = GetCh();
		if (ch == EOF) {
			// done with file
			if (fInDoubleQuote || fInSingleQuote) {
				printf("File %s # unterminated quote at end of file\n", name);
				result = B_ERROR;
				break;
			}
			result = SendArgv(argvHandlerFunc, passThru);
			break;
		}

		if (ch == '\n' || ch == '\r') {
			// handle new line
			fEatComment = false;
			if (!fSawBackslash && (fInDoubleQuote || fInSingleQuote)) {
				printf("File %s ; Line %ld # unterminated quote\n", name, fLineNo);
				result = B_ERROR;
				break;
			}
			fLineNo++;
			if (fSawBackslash) {
				fSawBackslash = false;
				continue;
			}
			// end of line, flush all argv
			result = SendArgv(argvHandlerFunc, passThru);
			if (result != B_NO_ERROR)
				break;
			continue;
		}
		
		if (fEatComment)
			continue;

		if (!fSawBackslash) {
			if (!fInDoubleQuote && !fInSingleQuote) {
				if (ch == ';') {
					// semicolon is a command separator, pass on the whole argv
					result = SendArgv(argvHandlerFunc, passThru);
					if (result != B_NO_ERROR)
						break;
					continue;
				} else if (ch == '#') {
					// ignore everything on this line after this character
					fEatComment = true;
					continue;
				} else if (ch == ' ' || ch == '\t') {
					// space or tab separates the individual arg strings
					NextArgvIfNotEmpty();
					continue;
				} else if (!fSawBackslash && ch == '\\') {
					// the next character is escaped
					fSawBackslash = true;
					continue;
				}
			}
			if (!fInSingleQuote && ch == '"') {
				// enter/exit double quote handling
				fInDoubleQuote = !fInDoubleQuote;
				continue;
			}
			if (!fInDoubleQuote && ch == '\'') {
				// enter/exit single quote handling
				fInSingleQuote = !fInSingleQuote;
				continue;
			}
		} else {
			// we just pass through the escape sequence as is
			fCurrentArgs[++fCurrentArgsPos] = '\\';
			fSawBackslash = false;
		}
		fCurrentArgs[++fCurrentArgsPos] = ch;
	}

	return result;
}

#ifdef _RECENT_FILES_TESTING_

#include <FindDirectory.h>
#include <Path.h>
#include <StopWatch.h>

#define kRosterSettingsDir "Roster"
#define kRosterSettings "RosterSettings"

void
DumpResult(BMessage *result)
{
	for (int32 index = 0; ;index++) {
		entry_ref ref;
		if (result->FindRef("refs", index, &ref) != B_OK)
			break;
			
		BPath path(&ref);
		printf("%s\n", path.Path());
	}
}
int
main()
{
	RecentItemsDict recentItems(200, 40, 20);
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
	
		// make sure there is a directory
		path.Append(kRosterSettingsDir);
		mkdir(path.Path(), 0777);
		path.Append(kRosterSettings);
		
		recentItems.Read(path.Path());
		
		BMessage result;
		{
			BStopWatch watch("by signature ----------------------------");
			recentItems.CollectRecentFiles(NULL, "application/x-vnd.pce-EDDI", 10, &result);
		}
		DumpResult(&result);
		result.MakeEmpty();
		{
			BStopWatch watch("type text ----------------------------");
			BObjectList<BMimeType> types(10, true);
			types.AddItem(new BMimeType("text"));
			recentItems.CollectRecentFiles(&types, NULL, 10, &result);
		}
		DumpResult(&result);
		result.MakeEmpty();
		{
			BStopWatch watch("type and signature ----------------------------");
			BObjectList<BMimeType> types(10, true);
			types.AddItem(new BMimeType("text/plain"));
			types.AddItem(new BMimeType("text/x-sourceCode"));
			recentItems.CollectRecentFiles(&types, "application/x-vnd.pce-EDDI", 10, &result);
		}
		DumpResult(&result);
		result.MakeEmpty();
		{
			BStopWatch watch("match any ----------------------------");
			recentItems.CollectRecentFiles(NULL, NULL, 10, &result);
		}
		DumpResult(&result);
		result.MakeEmpty();
		{
			BStopWatch watch("match folder ----------------------------");
			recentItems.CollectRecentFolders("application/x-vnd.pce-EDDI", 10, &result);
		}
		DumpResult(&result);
	}
}

#endif
