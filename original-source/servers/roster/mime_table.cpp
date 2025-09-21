/*****************************************************************************

	File : mime_table.cpp

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <string.h>

#include <SupportDefs.h>
#include <Volume.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <Autolock.h>
#include <roster_private.h>
#include <private/storage/mime_private.h>
#include "mime_table.h"

/*------------------------------------------------------------------------*/

const char *_MIME_TABLE_	= "__mime_table";

TMimeTable *gMimeTable;

/*------------------------------------------------------------------------*/

TMimeTable::TMimeTable()
	: BLocker("mime_table_lock")
{
	status_t	err;

	fDirty = false;

	fTable[0].what = 'HUNT';
	fTable[1].what = 'FISH';

	BPath	table_path;
	find_directory(_MIME_PARENT_DIR_, &table_path);
	table_path.Append(META_MIME_ROOT);
	table_path.Append(_MIME_TABLE_);

	entry_ref	ref;
	err = get_ref_for_path(table_path.Path(), &ref);
	if (err)
		return;

	BFile		file;
	err = file.SetTo(&ref, O_RDONLY);
	if (err)
		return;

	if (fTable[0].Unflatten(&file) == B_OK)
		fTable[1].Unflatten(&file);

//+	Lock();
//+	DumpTable();
//+	Unlock();
}

/*------------------------------------------------------------------------*/

TMimeTable::~TMimeTable()
{
	SaveTable();
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::SaveTable()
{
	status_t	err;
	BPath		table_path;

	if (!fDirty)
		return B_OK;

	PRINT(("SAVE Mime table path: dirty=%d\n", fDirty));

	find_directory(_MIME_PARENT_DIR_, &table_path, true);
	table_path.Append(META_MIME_ROOT);
	table_path.Append(_MIME_TABLE_);

	entry_ref	ref;
	err = get_ref_for_path(table_path.Path(), &ref);
	if (err)
		return err;

	BFile		file;
	err = file.SetTo(&ref, O_CREAT | O_WRONLY | O_TRUNC);
	if (err)
		return err;

	BAutolock a(this);
	ASSERT(a.IsLocked());

	err = (fTable[0]).Flatten(&file);

	if (!err) {
		err = (fTable[1]).Flatten(&file);
		if (!err)
			fDirty = false;
	}

	return err;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::DumpTable() const
{
	BAutolock a(const_cast<TMimeTable*>(this));
	ASSERT(a.IsLocked());

	SERIAL_PRINT(("--------------------------------\n"));
	SERIAL_PRINT(("--------------------------------\n"));
	SERIAL_PRINT(("Dumping table\n"));
	SERIAL_PRINT(("Dumping table\n"));
	{
		const BMessage	*msg = &(fTable[0]);
		int32	fi = 0;
		int32	c;
		uint32	type;
		const char	*name;
		while (msg->GetInfo(B_STRING_TYPE, fi++, &name, &type, &c) == B_OK) {
			SERIAL_PRINT(("sig: %s\n", name));
			int32		i = 0;
			const char	*data;
			while (msg->FindString(name, i++, &data) == B_OK) {
				SERIAL_PRINT(("	%s\n", data));
			}
		}
	}

	SERIAL_PRINT(("--------------------------------\n"));
	{
		const BMessage	*msg = &(fTable[1]);
		int32	fi = 0;
		int32	c;
		uint32	type;
		const char	*name;
		while (msg->GetInfo(B_STRING_TYPE, fi++, &name, &type, &c) == B_OK) {
			SERIAL_PRINT(("suffix: %s\n", name));
			int32		i = 0;
			const char	*data;
			while (msg->FindString(name, i++, &data) == B_OK) {
				SERIAL_PRINT(("	%s\n", data));
			}
		}
	}
	SERIAL_PRINT(("--------------------------------\n"));

	return B_OK;
}

/*------------------------------------------------------------------------*/

BMessage *TMimeTable::Table(which_table w)
{
	if (!IsLocked()) {
		debugger("must lock mime_table first\n");
		return NULL;
	}
	return &(fTable[w]);
}

/*------------------------------------------------------------------------*/

void TMimeTable::SetDirty(bool dirty)
{
	if (!IsLocked()) {
		debugger("must lock mime_table first\n");
		return;
	}
	fDirty = dirty;
}

/*------------------------------------------------------------------------*/

bool TMimeTable::IsDirty() const
{
	if (!IsLocked()) {
		debugger("must lock mime_table first\n");
		return false;
	}
	return fDirty;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::GetPossibleTypes(const char *suffix, BMessage *msg)
{
//+	PRINT(("Looking for types map from \".%s\"\n", suffix));
	status_t	err = B_ERROR;

	BAutolock al(this);

	int32		i = 0;
	const char	*type;
	BMessage	*table = Table(FILE_SUFFIX_TABLE);

	while (table->FindString(suffix, i++, &type) == B_OK) {
		if (BMimeType::IsValid(type))
			err = msg->AddString("types", type);
	}
	return err;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::GetSupportingApps(const char *type, BMessage *msg)
{
	BMimeType		mtype(type);
	bool			is_super;

//+	PRINT(("Looking for apps that support %s\n", type));

	if (!mtype.IsValid())
		return B_BAD_VALUE;

	is_super = mtype.IsSupertypeOnly();

	BAutolock al(this);

	int32		i = 0;
	const char	*sig;
	BMessage	*table = Table(SUPPORTING_APPS_TABLE);

	while (table->FindString(type, i++, &sig) == B_OK) {
		if (BMimeType::IsValid(sig))
			UniqueAdd(msg, "applications", sig);
	}

	int32	count;
	uint32	t;
	msg->GetInfo("applications", &t, &count);

	if (is_super) {
		// remember the number of super types actually added to list.
		msg->AddInt32("be:super", count);
	} else {
		// remember the number of sub types actually added to list.
		msg->AddInt32("be:sub", count);

		// now look for apps that supprt the super_type
		BMimeType	super;
		const char	*stype;
		mtype.GetSupertype(&super);
		stype = super.Type();

		i = 0;
		while (table->FindString(stype, i++, &sig) == B_OK) {
			if (BMimeType::IsValid(sig))
				// avoid any duplicates by using UniqueAdd.
				UniqueAdd(msg, "applications", sig);
		}

		int32	count2;
		msg->GetInfo("applications", &t, &count2);

		// remember the number of super types actually added to list.
		msg->AddInt32("be:super", count2 - count);
	}
//+	PRINT(("GetSupportingApps(%s)\n", type));
//+	PRINT_OBJECT((*msg));

	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::UniqueAdd(BMessage *msg, const char *field,
	const char *data)
{
	int32	i = 0;
	const	char *cdata;
//+	PRINT(("TRY: Adding %s to entry %s\n", data, field));

	while (msg->FindString(field, i++, &cdata) == B_OK) {
		if (strcasecmp(cdata, data) == 0)
			// found a match so return
			return B_OK;
	}
//+	PRINT(("Adding %s to entry %s\n", data, field));
	msg->AddString(field, data);
	fDirty = true;
	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::UniqueRemove(BMessage *msg, const char *field,
	const char *data)
{
	int32	i = 0;
	const	char *cdata;
//+	PRINT(("TRY: Removing %s from entry %s\n", data, field));

	while (msg->FindString(field, i++, &cdata) == B_OK) {
		if (strcasecmp(cdata, data) == 0) {
			i--;
			msg->RemoveData(field, i);
//+			PRINT(("Removing %s from entry %s\n", data, field));
			fDirty = true;
			return B_OK;
		}
	}
	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::RemoveFromAllEntries(BMessage *msg, const char *str)
{
	int32		j = 0;
	const char		*field;
	type_code	type;
	int32		count;

	while (msg->GetInfo(B_STRING_TYPE, j++, &field, &type, &count) == B_OK) {
		UniqueRemove(msg, field, str);
	}
	return B_OK;
}

/*------------------------------------------------------------------------*/

#if 0
static bool in_list(const char *str, BMessage *list);
bool in_list(const char *str, BMessage *list)
{
	// if there isn't a list then the string isn't in it!
	if (!list)
		return false;

	int		i = 0;
	const char *value;
	while (list->FindString(B_SUPPORTED_MIME_ENTRY, i++, &value) == B_OK) {
		if (strcasecmp(str, value) == 0)
			return true;
	}

	return false;
}
#endif

/*------------------------------------------------------------------------*/

status_t TMimeTable::UpdateSupportingApps(const char *sig,
	BMessage *new_list, bool force)
{
	if (!IsLocked()) {
		debugger("must lock mime_table first\n");
		return B_ERROR;
	}
	/*
	 Be very defensive here. Make sure that all mime_strings are valid.
	*/

	status_t	err;
	if (!BMimeType::IsValid(sig))
		return B_BAD_VALUE;

//+	SERIAL_PRINT(("Updating supporting apps: %s (n=%x)\n", sig,
//+		new_list));
//+	if (new_list) { PRINT_OBJECT((*new_list)); }

	int32		i = 0;
	const char	*type;

	if (force) {
		/*
		 Must remove all existing references to the app_sig "sig" from all
		 the file types in the table.
		*/
		RemoveFromAllEntries(&(fTable[SUPPORTING_APPS_TABLE]), sig);
	}

	i = 0;
	if (new_list) {
		while (new_list->FindString(B_SUPPORTED_MIME_ENTRY, i++, &type)==B_OK) {
			if (!BMimeType::IsValid(type))
				continue;

//+			SERIAL_PRINT(("	adding to type=%s\n", type));
			UniqueAdd(&(fTable[SUPPORTING_APPS_TABLE]), type, sig);
		}

		/*
		 Now look to see if any of the types that this app supports
		 has only 1 supporting app (this app itself). And if that type
		 doesn't have a preferred app. In this case let's
		 make this app the preferred app.
		*/
		i = 0;
		while (new_list->FindString(B_SUPPORTED_MIME_ENTRY, i++, &type)==B_OK) {
			int32		sub_count;
			int32		super_count;
			BMessage	supporting;
			BMimeType	mime(type);
			char		pref_app[B_MIME_TYPE_LENGTH];

			if (mime.GetPreferredApp(pref_app) == B_OK)
				continue;

			err = GetSupportingApps(type, &supporting);
			supporting.FindInt32("be:sub", &sub_count);
			supporting.FindInt32("be:super", &super_count);
			
//+			SERIAL_PRINT(("R, type=%s, (err=%x) sub_count=%d, super_count=%d\n",
//+				type, err, sub_count, super_count));
			if (((sub_count == 1) && (super_count == 0)) ||
				((sub_count == 0) && (super_count == 1))) {
					mime.SetPreferredApp(sig);
			}
		}
	}

	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t TMimeTable::UpdateFileSuffixes(const char *type,
	BMessage *new_list, BMessage *old_list)
{
	if (!IsLocked()) {
		debugger("must lock mime_table first\n");
		return B_ERROR;
	}
	/*
	 Be very defensive here.
	*/

	if (!BMimeType::IsValid(type))
		return B_BAD_VALUE;

//+	PRINT(("Updating file suffixes for type %s (new=%x, old=%x\n", type,
//+		new_list, old_list));
	int32		i = 0;
	const char	*suf;

	while (old_list &&
		old_list->FindString(B_FILE_EXTENSIONS_ENTRY, i++, &suf) == B_OK) {
			UniqueRemove(&(fTable[FILE_SUFFIX_TABLE]), suf, type);
	}

	i = 0;
	while (new_list &&
		new_list->FindString(B_FILE_EXTENSIONS_ENTRY, i++, &suf) == B_OK) {
			UniqueAdd(&(fTable[FILE_SUFFIX_TABLE]), suf, type);
	}
	return B_OK;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
