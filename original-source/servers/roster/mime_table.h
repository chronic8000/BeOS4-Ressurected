/*****************************************************************************

	File : mime_table.h

	$Revision: 1.2 $

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef MIME_TABLE_H
#define MIME_TABLE_H

#include <sys/types.h>

#include <SupportDefs.h>
#include <Message.h>
#include <Locker.h>
#include <AppFileInfo.h>

/*------------------------------------------------------------------------*/

enum which_table {
	SUPPORTING_APPS_TABLE = 0,
	FILE_SUFFIX_TABLE = 1
};

class TMimeTable : public BLocker {
public:
						TMimeTable();
						~TMimeTable();

		status_t		SaveTable();
		status_t		DumpTable() const;

		BMessage		*Table(which_table which);
		void			SetDirty(bool dirty = true);
		bool			IsDirty() const;

		status_t		GetSupportingApps(const char *type, BMessage *apps);
		status_t		GetPossibleTypes(const char *suffix, BMessage *types);

		status_t		UpdateFileSuffixes(const char *type,
											BMessage *new_list,
											BMessage *old_list = NULL);
		status_t		UpdateSupportingApps(const char *sig,
											BMessage *new_list,
											bool force = false);

private:
		status_t 		UniqueAdd(BMessage *msg,
								const char *field,
								const char *data);
		status_t 		UniqueRemove(BMessage *msg,
									const char *field,
									const char *data);
		status_t 		RemoveFromAllEntries(BMessage *msg, const char *data);

		BMessage		fTable[2];
		bool			fDirty;
};

extern TMimeTable *gMimeTable;

/*------------------------------------------------------------------------*/

#endif
