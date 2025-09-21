/*****************************************************************************

	File : task.cpp

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <string.h>
#include <roster_private.h>
#include <StopWatch.h>
#include <Path.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <private/storage/mime_private.h>
#include <Mime.h>

#include "mime_priv.h"
#include "main.h"
#include "task.h"

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

/*------------------------------------------------------------------------*/

TTask::~TTask()
{
}

/*------------------------------------------------------------------------*/

status_t TTask::Suspend()
{
	return B_OK;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

TAutoStoppingTask::TAutoStoppingTask(int32 chunk, int32 pulse, sem_id hangup)
	: TTask()
{
	fChunkSize = chunk;
	fPulseRate = pulse;
	fWalker = NULL;
	fUnlimited = (chunk < 0);
	fHangupSem = hangup;
}

/*------------------------------------------------------------------------*/

TAutoStoppingTask::~TAutoStoppingTask()
{
//+	PRINT(("TAutoStoppingTask::~TAutoStoppingTask\n"));
	if (fWalker) {
		delete fWalker;
		fWalker = NULL;
	}
}

/*------------------------------------------------------------------------*/

status_t TAutoStoppingTask::Process()
{
	bigtime_t	t;
	status_t	err = _Process(&t);

	if (err < 0) {
		delete fWalker;
		fWalker = NULL;
		return err;
	}

	float		p = ((float) t) / (float) (fPulseRate * 1000000);

//+	PRINT(("processed %d files (tid=%d, walker=%x), %.2f, %Ld usecs\n",
//+		fChunkSize, find_thread(NULL), fWalker, p, t));

	if (p > SCAN_USAGE_THRESHOLD) {
		int32	n = (int32)(fChunkSize * (SCAN_USAGE_THRESHOLD / p));
//+		PRINT(("Taking too much time (old=%d, new=%d)\n", fChunkSize, n));
		fChunkSize = n;
	} else if (p < SCAN_USAGE_MIN_THRESHOLD) {
		float ave = (SCAN_USAGE_THRESHOLD + SCAN_USAGE_MIN_THRESHOLD) / 2.0;
		int32 n = (int32)(fChunkSize * (ave / p));
//+		PRINT(("Taking too little time (old=%d, new=%d)\n", fChunkSize, n));
		fChunkSize = n;
	}
	return err;
}

/*------------------------------------------------------------------------*/

bool TAutoStoppingTask::Hangup()
{
	if (fHangupSem < 0)
		return false;
	int32	c;

	// If the sem no longer exists then abort by returning true
	return (get_sem_count(fHangupSem, &c) != B_OK);
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

TMimeSetTask::TMimeSetTask(int32 chunk, int32 pulse, bool query, bool force,
	sem_id hangup)
	: TAutoStoppingTask(chunk, pulse, hangup)
{
	fUseQuery = query;
	fForce = force;
}

/*------------------------------------------------------------------------*/

TMimeSetTask::~TMimeSetTask()
{
//+	PRINT(("TMimeSetTask::~TMimeSetTask\n"));
}

/*------------------------------------------------------------------------*/

status_t TMimeSetTask::Suspend()
{
//+	PRINT(("TMimeSetTask::Suspend\n"));

	if (fWalker) {
//+		BEntry	entry;
//+		if (fWalker->GetNextEntry(&entry) == B_OK) {
//+			BPath	pp;
//+			time_t	mt;
//+			entry.GetPath(&pp);
//+			entry.GetModificationTime(&mt);
//+			PRINT((" 	next file: %s, mod_time=%d\n", pp.Path(), mt));
//+
//+			gState->fQueryTime = mt;
//+		}
	}

	return B_OK;
}

/*------------------------------------------------------------------------*/

status_t TMimeSetTask::_Process(bigtime_t *et)
{
	BEntry		entry;
	status_t	err = B_OK;
	int32		i = fChunkSize;

	if (!fWalker) {
		if (fUseQuery) {
			char	buf[256];
			sprintf(buf, "last_modified > %ld\n", gState->fQueryTime);
			gState->fQueryIssueTime = time(NULL);
//+			PRINT(("##Issuing Query(%s) at %d\n",
//+				buf, gState->fQueryIssueTime));
			fWalker = new TQueryWalker(buf);
		} else {
//+			PRINT(("##Walking all nodes at %d\n", time(NULL)));
			fWalker = new TVolWalker(true, true);
		}
	}

	BStopWatch	timer("processing", true);

	while (fUnlimited || i--) {
		if (Hangup()) {
			err = B_ERROR;
			break;
		}

		if ((err = fWalker->GetNextEntry(&entry)) == B_OK) {
			BPath	pp;
			entry.GetPath(&pp);
			_update_mime_info_(pp.Path(), fForce);
//+			PRINT((" _processed file: %s\n", pp.Path()));
		} else {
			// query is complete! So the next time we make a query it
			// should be based on the time that this query was issued
//+			PRINT(("	finished (%d items processed)\n", fChunkSize - (i+1)));
			gState->fQueryTime = gState->fQueryIssueTime;
			break;
		}
	}

	*et = timer.ElapsedTime();
	return err;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

TUpdateAppsTask::TUpdateAppsTask(int32 chunk, int32 pulse, bool force,
	sem_id hangup)
	: TAutoStoppingTask(chunk, pulse, hangup)
{
	fForce = force;
}

/*------------------------------------------------------------------------*/

TUpdateAppsTask::~TUpdateAppsTask()
{
}

/*------------------------------------------------------------------------*/

status_t TUpdateAppsTask::_Process(bigtime_t *et)
{
	BEntry		entry;
	status_t	err = B_OK;
	int32		i = fChunkSize;

	if (!fWalker) {
		char	buf[256];
		sprintf(buf, "BEOS:APP_SIG = *");
//+		PRINT(("Issuing query (%s)\n", buf));
		fWalker = new TQueryWalker(buf);
	}

	BStopWatch	timer("processing", true);

	while (fUnlimited || i--) {
		if (Hangup()) {
			err = B_ERROR;
			break;
		}

		if ((err = fWalker->GetNextEntry(&entry)) == B_OK) {
			BPath	pp;
			entry.GetPath(&pp);
			err = _update_app_(pp.Path(), fForce);
//+			PRINT(("app: %s (err=%x)\n", pp.Path(), err));
		} else {
//+			PRINT(("	finished (%d items processed)\n", fChunkSize - (i+1)));
			break;
		}
	}

	*et = timer.ElapsedTime();
	return err;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

TUpdateSuffixTask::TUpdateSuffixTask(int32 chunk, int32 pulse, sem_id hangup)
	: TAutoStoppingTask(chunk, pulse, hangup)
{
	fCurIndex = 0;
}

/*------------------------------------------------------------------------*/

TUpdateSuffixTask::~TUpdateSuffixTask()
{
}

/*------------------------------------------------------------------------*/

status_t TUpdateSuffixTask::_Process(bigtime_t *et)
{
//+	BEntry		entry;
	status_t	err = B_OK;
	int32		i = fChunkSize;

	if (fCurIndex == 0) {
		BMimeType::GetInstalledTypes(&fTypes);
	}

	BStopWatch	timer("processing", true);

	const char	*type;
	while (fUnlimited || i--) {
		if (Hangup()) {
			err = B_ERROR;
			break;
		}

		if (fTypes.FindString("types", fCurIndex++, &type) == B_OK) {
			BMimeType	mtype(type);
			if (!mtype.IsValid())
				continue;
			BMessage	suffixes;
			if (mtype.GetFileExtensions(&suffixes) == B_OK) {
				gMimeTable->Lock();
				gMimeTable->UpdateFileSuffixes(type, &suffixes, NULL);
				gMimeTable->Unlock();
			}

//+			PRINT(("meta_mime: %s\n", type));
		} else {
//+			PRINT(("	finished (%d items processed)\n", fChunkSize - (i+1)));
			fCurIndex = 0;
			fTypes.MakeEmpty();
			err = B_ENTRY_NOT_FOUND;
			break;
		}
	}

	*et = timer.ElapsedTime();
	return err;
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

#endif
