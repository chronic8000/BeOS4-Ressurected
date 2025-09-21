/*****************************************************************************

	File : Timer.h

	Written by: Peter Potrebic

	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef TIMER_H
#define TIMER_H

#include <Messenger.h>
#include <Message.h>
#include <Locker.h>

/* --------------------------------------------------------------- */

typedef	int32 timer_token;

/* --------------------------------------------------------------- */

class TTimer {
public:
					TTimer(bigtime_t granularity);


		timer_token	Add(const BMessenger &target,
						BMessage *msg,
						const BMessenger &reply,
						bigtime_t interval,
						int32 count);
		timer_token	Add(const BMessenger &target,
						const BMessage &msg,
						const BMessenger &reply,
						bigtime_t interval,
						int32 count);
		status_t	Remove(timer_token t);
		status_t	Adjust(timer_token t, bool reset_i, bigtime_t interval,
						bool reset_c, int32 count);
		status_t	GetInfo(timer_token t, bigtime_t *interval,
						int32 *count) const;

		status_t	Stop();
		thread_id	Thread();

static	timer_token	NextToken();

protected:

virtual	void		Process();
virtual	bigtime_t	CalculateSleepTime();

private:

static	status_t	_entry_(void *arg);
static	timer_token	sNextToken;
static	BLocker		sTokenLock;

		enum action {
			_B_PROCESS,
			_B_SKIP
		};

virtual				~TTimer();


		void		Run();
		action		Sleep();
		status_t	Wakeup();

		sem_id		fSem;
		thread_id	fThread;
		bigtime_t	fGranularity;
		bool		fRunning;
};

/* --------------------------------------------------------------- */

#endif
