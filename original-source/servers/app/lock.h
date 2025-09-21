
//******************************************************************************
//
//	File:			lock.h
//	Description:	Storage place of all things locking-related
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#ifndef LOCK_H
#define LOCK_H

#include <SupportDefs.h>
#include <OS.h>
#include "system.h"
#include "as_debug.h"

#define DEFAULT_BENAPHORE_NAME		"some benaphore"
#define DEFAULT_RWLOCK_NAME			"some RWLock"
#define MAX_READERS					100000

#define HEAVY_DUTY_DEBUG			AS_DEBUG

class CallStack;

/*	Benaphore implementing a simple lock */
class Benaphore {
	public:

				Benaphore(const char *name = DEFAULT_BENAPHORE_NAME);
				~Benaphore();
	
		void	Lock();
		void	Unlock();

	private:
	
		int32	count;
		sem_id	sem;
};

/*	A counted benaphore (nested locking allowed) */
class CountedBenaphore {
	public:

				CountedBenaphore(const char *name = DEFAULT_BENAPHORE_NAME);
				~CountedBenaphore();
	
		int32	Lock();
		void	Lock(int32 pid);
		void	Unlock();
		int32	LockedBy();

	private:
	
		int32	count;
		sem_id	sem;
		int32	owner;
		int32	nested;
};

/*	This class implements a one writer/many readers lock which is
	very fast for readers (a single atomic_add() when contested by 
	nobody or only other readers) and somewhat slower for writers
	(two atomic_adds when uncontested by other writers).  Starvation
	should never occur for either readers or writers. */
class RWLock {
	public:
					RWLock(const char *name = DEFAULT_RWLOCK_NAME);
					~RWLock();
	
		void		WriteLock();
		void		WriteUnlock();
		void		ReadLock();
		void		ReadUnlock();
		void		DowngradeWriteToRead();

	private:
	
		int32		m_count;
		sem_id		m_readerSem;
		sem_id		m_writerSem;
		Benaphore	m_writeLock;
};

/*	Note: the SuperDoubleExtraSpecialRWLock does NOT support
	locking read-only first and then transitioning to read-write. */
class SuperDoubleExtraSpecialRWLock {
	public:
					SuperDoubleExtraSpecialRWLock(const char *name = DEFAULT_RWLOCK_NAME);
					~SuperDoubleExtraSpecialRWLock();
	
		void		WriteLock(int32 pid);
		int32		WriteLock();
		void		WriteUnlock();
		void		ReadLock(int32 pid);
		int32		ReadLock();
		void		ReadUnlock(int32 pid);
		int32		ReadUnlock();
		void		DowngradeWriteToRead();
		
		void		DumpInfo();
		#if HEAVY_DUTY_DEBUG
		void		ClearThread(int32 thread);
		#endif

		int32		LockedBy()		{ return m_owner; };
		int32		Count()			{ return m_count; };
		int32		Nesting()		{ return m_nested; };
		int32		ReaderSem()		{ return m_readerSem; };
		int32		WriterSem()		{ return m_writerSem; };

	private:
	
		int32		m_count;
		sem_id		m_readerSem;
		sem_id		m_writerSem;
		int32		m_owner;
		int32		m_nested;
		Benaphore	m_writeLock;

		#if HEAVY_DUTY_DEBUG
		CallStack *	m_threadStacks;
		int32 *		m_readLocked;
		int32 *		m_waitReadLock;
		int32 *		m_actualThread;
		int32		m_maxThreads;
		#endif
};

inline void	CountedBenaphore::Lock(int32 pid)
{
	if (pid == owner) { nested++; return; };
	if (atomic_add(&count,1) >= 1) acquire_sem(sem);
	owner = pid;
	nested = 1;
};

inline int32 CountedBenaphore::Lock()
{
	int32 pid;
	Lock(pid=xgetpid());
	return pid;
};

inline int32 CountedBenaphore::LockedBy()
{
	return owner;
};

inline void CountedBenaphore::Unlock()
{
	if (!(--nested)) {
		owner = -1;
		if (atomic_add(&count,-1) > 1) release_sem_etc(sem,1,B_DO_NOT_RESCHEDULE);
	};
};

inline void	Benaphore::Lock()
{
	if (atomic_add(&count,1) >= 1)
		acquire_sem(sem);
};

inline void	Benaphore::Unlock()
{
	if (atomic_add(&count,-1) > 1)
		release_sem_etc(sem,1,B_DO_NOT_RESCHEDULE);
};

inline void RWLock::WriteLock()
{
	m_writeLock.Lock();
	int32 readers = atomic_add(&m_count,-MAX_READERS);
	if (readers > 0) acquire_sem_etc(m_writerSem,readers,0,0);
};

inline void RWLock::WriteUnlock()
{
	int32 readersWaiting = atomic_add(&m_count,MAX_READERS) + MAX_READERS;
	if (readersWaiting) release_sem_etc(m_readerSem,readersWaiting,B_DO_NOT_RESCHEDULE);
	m_writeLock.Unlock();
};

inline void RWLock::ReadLock()
{
	if (atomic_add(&m_count,1) < 0)
		acquire_sem(m_readerSem);
};

inline void RWLock::ReadUnlock()
{
	if (atomic_add(&m_count,-1) < 0)
		release_sem_etc(m_writerSem,1,B_DO_NOT_RESCHEDULE);
};

inline void RWLock::DowngradeWriteToRead()
{
	int32 readersWaiting = atomic_add(&m_count,MAX_READERS-1) + MAX_READERS;
	if (readersWaiting) release_sem_etc(m_readerSem,readersWaiting,B_DO_NOT_RESCHEDULE);
	m_writeLock.Unlock();
}

extern SuperDoubleExtraSpecialRWLock GlobalRegionsLock;
extern Benaphore GlobalScreenLock;
extern Benaphore GlobalAccelLock;

inline int32 wait_regions()
{
	return GlobalRegionsLock.WriteLock();
};

inline void wait_regions_hinted(int32 pid)
{
	GlobalRegionsLock.WriteLock(pid);
};

inline void signal_regions()
{
	GlobalRegionsLock.WriteUnlock();
};

inline void get_screen()
{
	GlobalScreenLock.Lock();
};

inline void release_screen()
{
	GlobalScreenLock.Unlock();
};

#endif
