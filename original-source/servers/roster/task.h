/*****************************************************************************

	File : task.h

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef TASK_H
#define TASK_H

#include <sys/types.h>
#include <SupportDefs.h>
#include <Locker.h>

#include <private/storage/walker.h>

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

/*------------------------------------------------------------------------*/

class TTask {
public:
virtual	status_t	Process() = 0;
virtual const char	*Name() = 0;

virtual	status_t	Suspend();

virtual				~TTask();

protected:
};

/*------------------------------------------------------------------------*/

class TAutoStoppingTask : public TTask {
public:
					TAutoStoppingTask(int32 fChunkSize,
									int32 pulse,
									sem_id hangup_sem);
virtual				~TAutoStoppingTask();

virtual	status_t	Process();

protected:

virtual status_t	_Process(bigtime_t *t) = 0;
		bool		Hangup();

		TWalker		*fWalker;
		int32		fChunkSize;
		int32		fPulseRate;
		bool		fUnlimited;
		sem_id		fHangupSem;
};

/*------------------------------------------------------------------------*/

class TMimeSetTask : public TAutoStoppingTask {
public:
					TMimeSetTask(int32 fChunkSize,
								int32 pulse,
								bool use_query = true,
								bool force = false,
								sem_id hangup_sem = B_BAD_SEM_ID);
virtual				~TMimeSetTask();

virtual const char	*Name() { return "mime_set"; };
virtual status_t	Suspend();

protected:

virtual status_t	_Process(bigtime_t *t);

		bool		fUseQuery;
		bool		fForce;
};

/*------------------------------------------------------------------------*/

class TUpdateAppsTask : public TAutoStoppingTask {
public:
					TUpdateAppsTask(int32 chunk,
									int32 pulse,
									bool force = false,
									sem_id hangup_sem = B_BAD_SEM_ID);
virtual				~TUpdateAppsTask();

virtual const char	*Name() { return "update_apps"; };

protected:

virtual	status_t	_Process(bigtime_t *et);

private:
		bool		fForce;
};

/*------------------------------------------------------------------------*/

class TUpdateSuffixTask : public TAutoStoppingTask {
public:
					TUpdateSuffixTask(int32 chunk,
									int32 pulse,
									sem_id hangup_sem = B_BAD_SEM_ID);
virtual				~TUpdateSuffixTask();

virtual const char	*Name() { return "update_suffix"; };

protected:

virtual	status_t	_Process(bigtime_t *et);

private:
		BMessage	fTypes;
		int32		fCurIndex;
};

/*------------------------------------------------------------------------*/

#endif

#endif
