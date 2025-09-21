/*****************************************************************************

	File : Timr.cpp

	Written by: Peter Potrebic

	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>
#include <OS.h>
#include <string.h>
#include <Messenger.h>
#include <Message.h>
#include <Autolock.h>

#include "Timer.h"

/*---------------------------------------------------------------*/

struct _task_ {
					_task_(const BMessenger &target, BMessage *msg,
						const BMessenger &reply, bigtime_t ival, int32 count);
virtual				~_task_();

		timer_token	token;
		bigtime_t	interval;
		bigtime_t	next_delivery;
		int32		count;
		BMessenger	reply;
		BMessenger	target;
		BMessage	*message;
		_task_		*next;
		_task_		*prev;

static	_task_		*First();

private:
friend	class TTimer;

		status_t	schedule();
		status_t	remove(bool decrement);
static	void		dump();

static	_task_		*find(timer_token token);
static	bigtime_t	calc_sleep();
static	_task_		*sFirst;
};

_task_ *_task_::sFirst = NULL;

/*---------------------------------------------------------------*/

const uint32	_B_SCHEDULE	 = 'scch';
const uint32	_B_TICKLE	 = 'tkle';

/*---------------------------------------------------------------*/

timer_token	TTimer::sNextToken = 0;
BLocker		TTimer::sTokenLock(true);

/*---------------------------------------------------------------*/

TTimer::TTimer(bigtime_t granularity)
{
	fSem = create_sem(0, "timer");
	if (granularity < 0)
		granularity = 0;
	fGranularity = granularity;
	fThread = spawn_thread(TTimer::_entry_, "timer_thread",
		B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
}

/*---------------------------------------------------------------*/

TTimer::~TTimer()
{
}

/*---------------------------------------------------------------*/

thread_id TTimer::Thread()
{
	return fThread;
}

/*---------------------------------------------------------------*/

status_t TTimer::_entry_(void *arg)
{
	TTimer	*me = (TTimer *) arg;
	me->Run();
	delete me;
	return B_OK;
}

/*---------------------------------------------------------------*/

void TTimer::Run()
{
	sTokenLock.Lock();
	fRunning = true;

	while (fRunning) {
		sTokenLock.Unlock();
		action a = Sleep();
		sTokenLock.Lock();
		switch (a) {
			case _B_PROCESS: {
				Process();
				break;
			}
			default: {
				break;
			}
		}
	}
}

/*---------------------------------------------------------------*/

void TTimer::Process()
{
	_task_		*task = _task_::First();
	_task_		*next;

//+	_task_::dump();
	if (!task) {
//+		TRACE();
		return;
	}

	bigtime_t	prime = task->next_delivery;
	status_t	err;

	// walk forward in list to include all scheduled tasks that are
	// within "fGranularity" microsecs of the "prime" time
	while (task) {
		if (task->next_delivery > prime + fGranularity) {
			break;
		}
//+		PRINT(("%Ld: ", system_time() / 1000000));
//+		PRINT_OBJECT((*task->message));
		task->message->ReplaceInt64("when", system_time());
		err = task->target.SendMessage(task->message, task->reply, 0);
		if (err == B_WOULD_BLOCK)
			err = B_OK;
		task->remove(true);
		next = task->next;
		if (err || (task->schedule() != B_OK)) {
			delete task;
		}
		task = next;
	}

}

/*---------------------------------------------------------------*/

bigtime_t TTimer::CalculateSleepTime()
{
	bigtime_t	s = _task_::calc_sleep();
	return s;
}

/*---------------------------------------------------------------*/

TTimer::action TTimer::Sleep()
{
	bigtime_t	s = CalculateSleepTime();
	status_t	err;
	action		act;

//+	PRINT(("going to sleep (s=%Ld)\n", s));
	err = acquire_sem_etc(fSem, 1, B_TIMEOUT | B_CAN_INTERRUPT, s);

	if (err == B_OK) 
		// we actually acquired the semaphore!.
		// Probably because a task was added or deleted.
		// We're woken up so that we reschedule ourselves for the new
		// wakeup time taking into account the addition/deletion.
		act = _B_SKIP;
	else if (err == B_TIMED_OUT || err == B_WOULD_BLOCK) 
		// timer ran out so there is some processing to do!
		act = _B_PROCESS;
	else 
		// some other error... just continue
		act = _B_SKIP;

//+	PRINT(("woke up (%d) (err=%x (%s))\n", act, err, strerror(err)));
	return act;
}

/*---------------------------------------------------------------*/

timer_token TTimer::Add(const BMessenger &target, const BMessage &msg, 
	const BMessenger &reply, bigtime_t interval, int32 count)
{
	BMessage *message = new BMessage(msg);
	return Add(target, message, reply, interval, count);
}

/*---------------------------------------------------------------*/

timer_token TTimer::Add(const BMessenger &target, BMessage *msg, 
	const BMessenger &reply, bigtime_t interval, int32 count)
{
	BAutolock	alock(sTokenLock);

	if (interval == 0) {
		return B_BAD_VALUE;
	} else if (interval < fGranularity) {
		interval = fGranularity;
	}

	SERIAL_PRINT(("Timer::Add: team=%d, what=%.4s, int=%Ld, count=%d\n",
		target.Team(), (char*) &(msg->what), interval, count));

	// add dummy "when" field.
	msg->AddInt64("when", 0);

	_task_	*t = new _task_(target, msg, reply, interval, count);

	timer_token tok = B_BAD_VALUE;
	if (t->schedule() == B_OK) {
		tok = t->token;
		Wakeup();
	} else {
		delete t;
	}

	return tok;
}

/*---------------------------------------------------------------*/

status_t TTimer::Remove(timer_token token)
{
	BAutolock	alock(sTokenLock);

	status_t	err = B_BAD_VALUE;
	_task_		*t = _task_::find(token);
	if (t) {
		t->remove(false);
		delete t;
		err = B_OK;
	}
	Wakeup();
	return err;
}

/*---------------------------------------------------------------*/

status_t TTimer::GetInfo(timer_token token, bigtime_t *interval, int32 *count) const
{
	BAutolock	alock(sTokenLock);

	status_t	err = B_BAD_VALUE;
	_task_		*t = _task_::find(token);
	if (t) {
		*interval = t->interval;
		*count = t->count;
		err = B_OK;
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t TTimer::Adjust(timer_token token, bool reset_i, bigtime_t interval,
	bool reset_c, int32 count)
{
	BAutolock	alock(sTokenLock);

	status_t	err = B_BAD_VALUE;
	_task_		*t = _task_::find(token);
	if (t) {
		if (reset_c) {
			ASSERT(count != 0);
			t->count = count;
		}

		if (reset_i) {
			t->remove(false);
			t->interval = interval;
			t->schedule();
			Wakeup();
		}
		err = B_OK;
	}
	return err;
}

/*---------------------------------------------------------------*/

status_t TTimer::Stop()
{
	fRunning = false;
	return Wakeup();
}

/*---------------------------------------------------------------*/

status_t TTimer::Wakeup()
{
	return release_sem(fSem);
}

/*---------------------------------------------------------------*/

timer_token TTimer::NextToken()
{
	BAutolock	alock(sTokenLock);
	return sNextToken++;
}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/

_task_::_task_(const BMessenger &tar, BMessage *msg,
	const BMessenger &rep, bigtime_t ival, int32 c)
{
	interval = ival;
	next_delivery = -1;
	count = c;
	target = tar;
	reply = rep;
	message = msg;
	next = prev = NULL;
	token = TTimer::NextToken();
}

/*---------------------------------------------------------------*/

_task_::~_task_()
{
	delete message;
}

/*---------------------------------------------------------------*/

status_t	_task_::schedule()
{
	if (count == 0) {
		// this task is finished
		return B_ERROR;
	}

	ASSERT(!this->prev);
	ASSERT(!this->next);

	// find where this _task_ fits in "time"
	next_delivery = system_time()+interval;

	_task_	*cur = sFirst;

//+	PRINT(("SCHEDULE: Msg=%.4s, ival=%8Ld, count=%4d\n",
//+		(char*) &(this->message->what), this->interval, this->count));

	if (!sFirst) {
		sFirst = this;
//+		PRINT(("\nSchedule \n")); _task_::dump();
		return B_OK;
	}

	while (cur) {
		if (cur->next_delivery > this->next_delivery) {
			// new task should go before "cur"
			this->prev = cur->prev;
			if (prev)
				prev->next = this;
			cur->prev = this;
			this->next = cur;
			if (cur == sFirst)
				sFirst = this;
			break;
		}

		if (cur->next) {
			cur = cur->next;
			// and continue with the loop
		} else {
			// the new task goes at the end of the list!
			cur->next = this;
			this->prev = cur;
			break;
		}
	}
//+	PRINT(("\nSchedule:\n")); _task_::dump();
	return B_OK;
}

/*---------------------------------------------------------------*/

_task_	*_task_::find(timer_token token)
{
	_task_	*cur = sFirst;

	while (cur) {
		if (cur->token == token)
			return cur;
		cur = cur->next;
	}
	return cur;
}

/*---------------------------------------------------------------*/

void _task_::dump()
{
	_task_	*cur = sFirst;

	PRINT(("---------------------\n"));
	while (cur) {
		PRINT(("Msg=%.4s, time=%8Ld, ival=%8Ld, count=%4d\n",
			(char*) &(cur->message->what), cur->next_delivery,
			cur->interval, cur->count));
		cur = cur->next;
	}
	PRINT(("---------------------\n"));
}

/*---------------------------------------------------------------*/

status_t	_task_::remove(bool decrement)
{
	if (this == sFirst)
		sFirst = this->next;

	if (this->prev) {
		this->prev->next = this->next;
	}
	if (this->next) {
		this->next->prev = this->prev;
	}

	// if this task has a limited number of iterations, decrement the counter
	if (decrement && (count > 0)) {
		count--;
	}
	this->next = NULL;
	this->prev = NULL;

	return B_OK;
}

/*---------------------------------------------------------------*/

_task_	*_task_::First()
{
	return sFirst;
}


/*---------------------------------------------------------------*/

bigtime_t	_task_::calc_sleep()
{
	_task_		*task = First();

	if (!task) {
//+		PRINT(("calc_sleep - empty\n"));
		return B_INFINITE_TIMEOUT;
	}

	bigtime_t	now = system_time();
	bigtime_t	d = task->next_delivery - now;
//+	PRINT(("calc_sleep = now=%Ld, next=%Ld\n", now, task->next_delivery));
	if (d < 0) {
//+		TRACE();
		d = 0;
	}

	return d;
}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
