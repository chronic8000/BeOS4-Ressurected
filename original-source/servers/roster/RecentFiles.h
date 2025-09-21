#ifndef __RECENT_FILES__
#define __RECENT_FILES__

#include <Entry.h>
#include <ObjectList.h>
#include <String.h>
#include <functional>
#include <utility>
#include <stdarg.h>
#include <stdio.h>

class BMessage;
class BFile;

class StringStash {
public:
	StringStash();
	const char *AddUnique(const char *);
private:
	BObjectList<BString> stash;
};

struct CompareStrings : public std::binary_function<const char *, const char *, bool> {
	bool operator()(const char *& x, const char *& y) const
		{ return strcmp(x, y) == -1; }
};

struct RecentFileItem {
	RecentFileItem(const char *, const char *sig, int64 accessCount);
	bool Access(const char *sig, int64 accessCount);
		// return true if new app signature got entered

	const entry_ref *Ref() const;
	void GetPath(BString *) const;

private:
	mutable entry_ref ref;
	mutable BString unresolvedPath;

public:
	BObjectList<std::pair<const char *, int64> > appSigs;
};

class RecentItemsDict {
public:
	RecentItemsDict(int32 fileListLimit, int32 folderListLimit, int32 appListLimit);
	
	void AccessFile(const entry_ref *, const char *appSig);
	void AccessFolder(const entry_ref *, const char *appSig);
	void AccessApp(const char *appSignature);

	void CollectRecentFiles(const BObjectList<BMimeType> *mimeTypeList, const char *appSig,
		int32 count, BMessage *result) const;
	void CollectRecentFolders(const char *appSig, int32 count, BMessage *result) const;	
	void CollectRecentApps(int32 count, BMessage *result) const;

	void Read(const char *);
	void Save(const char *);

private:
	// settings Read/Write stuff
	const char *ReadOneArgv(int argc, const char *const *argv);
	status_t SaveRecentCommon(BFile *, const char *label,
		const BObjectList<RecentFileItem> &list);
	static status_t Write(BFile *, const char *);
	static status_t Write(BFile *, const BString &);
	
	void CollectRecentItemsCommon(const BObjectList<RecentFileItem> &list,
		const BObjectList<BMimeType> *mimeTypeList, const char *appSig,
		int32 count, BMessage *result) const;

	void AccessFile(const char *, const char *appSig);
	RecentFileItem *AccessCommon(const char *ref, const char *appSig,
		BObjectList<RecentFileItem> &list, int32 &count, int32 max,
		int64 &accessCount);
	RecentFileItem *FindByRef(const entry_ref *, BObjectList<RecentFileItem> &);
	RecentFileItem *FindByPath(const char *, BObjectList<RecentFileItem> &);

	void TrimToCount(BObjectList<RecentFileItem> &, int32 count, int64 accessCount,
		 int32 &totalCount);

#if DEBUG
	void Dump(const char *, const BObjectList<RecentFileItem> &) const;
	void DumpAppList();
#else
	void Dump(const char *, const BObjectList<RecentFileItem> &) const {}
	void DumpAppList() {}
#endif

	// dictionaries
	BObjectList<RecentFileItem> fileItems;
	BObjectList<RecentFileItem> folderItems;
	BObjectList<BString> recentApps;

	// string stash
	StringStash stringStash;
	
	// limits, counts, etc.
	int64 fileAccessCount;
	int64 folderAccessCount;
	int32 fileCount;
	int32 folderCount;
	int32 fileListLimit;
	int32 folderListLimit;
	int32 appListLimit;

	char *writeBuffer;

friend const char *EachArgvGlue(int , const char *const *, void *);
};


#endif
