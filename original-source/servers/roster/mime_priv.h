/*****************************************************************************

	File : mime_priv.h

	Written by: Peter Potrebic

	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef MIME_H
#define MIME_H
 
#include <Debug.h>
#include <List.h>
#include <Locker.h>
#include <VolumeRoster.h>
#include <Message.h>

#include <MimeSniffer.h>
#include <String.h>

#include <Looper.h>
#include <roster_private.h>
#include <mime_table.h>
#include <private/storage/walker.h>
#include "task.h"
#include "Monitor.h"

using namespace BPrivate;

/*------------------------------------------------------------------------*/

enum {
	_CMD_TASK_COMPLETED_ = CMD_FIRST_PRIVATE_MIME
};

//+#define IDLE_TIME_THRESHOLD			(60)
#define IDLE_TIME_THRESHOLD			(1200)
#define IDLE_USAGE_THRESHOLD		(0.25 * 1)
#define SCAN_USAGE_THRESHOLD		(0.20 * 1)
#define SCAN_USAGE_MIN_THRESHOLD	(0.10 * 1)
#define STANDARD_PULSE_RATE			5
//+#define MIN_TIME_BETWEEN_SCANS		(60)
#define MIN_TIME_BETWEEN_SCANS		(1800)
#define	DEFAULT_FILES_TO_PROCESS	10

/*------------------------------------------------------------------------*/

status_t _guess_mime_type_(const char *path, char *type);
status_t _update_app_(const char *path, bool force);
status_t _update_mime_info_(const char *path, int32 forceLevel);
	// forceLevel - 0: don't touch if BEOS:TYPE present
	//				1: update but don't touch BEOS:TYPE itself
	//				2: update everything including BEOS:TYPE

/*------------------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

class TMimeWorker;
class TMainMimeWorker;

/*------------------------------------------------------------------------*/

class TMimeWorker : public BLooper {
public:
static	TMimeWorker	*NewWorker(const char *, int32);
static	void		PulseAll();
static	void		TerminateWorkers();

virtual	void		MetaMimeChanged(BMessage *) {}

protected:
virtual void		MessageReceived(BMessage *);
private:
		typedef BLooper inherited;

friend	class TMainMimeWorker;
typedef BLooper _inherited;

					TMimeWorker(const char *name, int32 priority);
					~TMimeWorker();

		status_t	UpdateMimeInfo(BMessage *msg);
		void		Configure(TTask *task);

		status_t	HandleRemoteMimePropertySetCall(BMessage *);

virtual	void		Pulse();

		void		UpdateFiles();

static	BList		sList;	// global list of BMimeWorkers
static	BLocker		sLock;
static	int32		sStdPulseRate;

		int32		fPulseRate;
		int32		fPulseNext;
		TTask		*fTask;
};

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

class TMainMimeWorker : public TMimeWorker {
public:
	TMonitor *Monitor() { return &fMonitor; };
	virtual	void MetaMimeChanged(BMessage *);

	status_t SniffFile(const char *name, BFile *, BString &type);
	status_t SniffData(const void *, int32, BString &type);
	status_t SniffFilename(const char *, BString &type);
	
protected:
	virtual void MessageReceived(BMessage *msg);

	
private:
	typedef TMimeWorker	_inherited;
	
	friend TMimeWorker *TMimeWorker::NewWorker(const char *, int32);
	
	TMainMimeWorker(const char *name, int32 priority);
	~TMainMimeWorker();
	
	virtual	void Pulse();
	bool IdleCheck();
	
	
	TMimeWorker		*fSubordinate;
	TTask			*fCurrentTask;
	BList			fTaskList;
	int32			fTaskIndex;
	time_t			fQueryIssueTime;
	TMimeTable		*fMimeTable;
	BVolumeRoster	fVolRoster;
	int32			fSaveTableTicker;
	bigtime_t		fPrevActivity;
	bigtime_t		fPrevTime;
	bigtime_t		fLastCPUActiveTime;
	TMonitor		fMonitor;
	bool			fSnifferEnabled;
	Sniffer			fMimeSniffer;
};

extern TMainMimeWorker	*gMainWorker;

#endif

/*------------------------------------------------------------------------*/

#endif
