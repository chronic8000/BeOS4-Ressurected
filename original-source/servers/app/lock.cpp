//******************************************************************************
//
//	File:			lock.cpp
//	Description:	Storage place of all things locking-related
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#include <unistd.h>
#include <stdarg.h>
#include "as_support.h"
#include "lock.h"
#include "string.h"

SuperDoubleExtraSpecialRWLock GlobalRegionsLock("region_sem");
Benaphore GlobalScreenLock("screen_sem");
Benaphore GlobalAccelLock("accel_engine_lock");

Benaphore::Benaphore(const char *name)
{
	count = 0;
	sem = create_sem(0,name);
};

Benaphore::~Benaphore()
{
	delete_sem(sem);
};

CountedBenaphore::CountedBenaphore(const char *name)
{
	count = 0;
	sem = create_sem(0,name);
	owner = -1;
	nested = 0;
};

CountedBenaphore::~CountedBenaphore()
{
	delete_sem(sem);
};

RWLock::RWLock(const char *name)
{
	char s[256];
	m_count = 0;
	strcpy(s,name); strcat(s," readerSem");
	m_readerSem = create_sem(0,s);
	strcpy(s,name); strcat(s," writerSem");
	m_writerSem = create_sem(0,s);
};

RWLock::~RWLock()
{
	delete_sem(m_readerSem);
	delete_sem(m_writerSem);
};

SuperDoubleExtraSpecialRWLock::SuperDoubleExtraSpecialRWLock(const char *name)
	: m_writeLock(name)
{
	char s[256];
	m_count = 0;
	m_owner = -1;
	m_nested = 0;
	strcpy(s,name); strcat(s," readerSem");
	m_readerSem = create_sem(0,s);
	strcpy(s,name); strcat(s," writerSem");
	m_writerSem = create_sem(0,s);

#if HEAVY_DUTY_DEBUG
	system_info si;
	get_system_info(&si);
	m_maxThreads = si.max_threads;
	m_threadStacks = (CallStack*)dbMalloc(sizeof(CallStack)*m_maxThreads);
	m_readLocked = (int32*)dbMalloc(sizeof(int32)*m_maxThreads);
	m_waitReadLock = (int32*)dbMalloc(sizeof(int32)*m_maxThreads);
	m_actualThread = (int32*)dbMalloc(sizeof(int32)*m_maxThreads);
	for (int32 i=0;i<m_maxThreads;i++) {
		m_readLocked[i] = 0;
		m_waitReadLock[i] = 0;
		m_actualThread[i] = -1;
	};
#endif
};

#if HEAVY_DUTY_DEBUG
void SuperDoubleExtraSpecialRWLock::ClearThread(int32 thread)
{
	m_readLocked[thread%m_maxThreads] = 0;
	m_waitReadLock[thread%m_maxThreads] = 0;
	m_actualThread[thread%m_maxThreads] = thread;
};
#endif

void SuperDoubleExtraSpecialRWLock::DumpInfo()
{
	int32 owner = GlobalRegionsLock.LockedBy();
	int32 count = GlobalRegionsLock.Count();

#if HEAVY_DUTY_DEBUG
	if (owner != -1)
		DebugPrintf(("region_sem is write locked by thread %d\n",owner));
	else
		DebugPrintf(("region_sem is write locked by nobody\n"));

	int32 waitReadCount=0,readCount=0;
	for (int32 i=0;i<m_maxThreads;i++) {
		if (m_waitReadLock[i]) waitReadCount++;
		if (m_readLocked[i]) readCount++;
	};
	if (waitReadCount) {
		DebugPrintf(("There are %d threads waiting to acquire it read-only\n",waitReadCount));
		for (int32 i=0;i<m_maxThreads;i++) {
			if (m_waitReadLock[i]) {
				DebugPrintf(("  Thread %d (%d): ",m_actualThread[i],m_waitReadLock[i]));
				m_threadStacks[i].Print();
				DebugPrintf(("\n"));
			};
		};
		
	};
	if (readCount) {
		DebugPrintf(("It is read-locked by %d threads\n",readCount));
		for (int32 i=0;i<m_maxThreads;i++) {
			if (m_readLocked[i]) {
				DebugPrintf(("  Thread %d (%d): ",m_actualThread[i],m_readLocked[i]));
				m_threadStacks[i].Print();
				DebugPrintf(("\n"));
			};
		};
	};
#else
	if (owner != -1) {
		DebugPrintf(("region_sem is write locked by thread %d\n",owner));
		DebugPrintf(("There are %d threads waiting to acquire it read-only\n",count+MAX_READERS));
	} else {
		DebugPrintf(("region_sem is write locked by nobody\n"));
		DebugPrintf(("It is read-locked by %d threads\n",count));
	};
#endif
	DebugPrintf(("nesting is at %d\n",GlobalRegionsLock.Nesting()));
	get_sem_count(GlobalRegionsLock.ReaderSem(),&count);
	DebugPrintf(("readerSem count is %d\n",count));
	get_sem_count(GlobalRegionsLock.WriterSem(),&count);
	DebugPrintf(("writerSem count is %d\n",count));
};

void SuperDoubleExtraSpecialRWLock::WriteLock(int32 pid)
{
	if (pid == m_owner) { m_nested++; return; };

#if HEAVY_DUTY_DEBUG
//	DebugPrintf(("SuperDoubleExtraSpecialRWLock::WriteLock(%d)\n",pid));
	if (m_readLocked[pid%m_maxThreads]) {
		DebugPrintf(("Thread %d write locking, but read lock count is already %d!\n",pid,m_readLocked[pid%m_maxThreads]));
		DebugPrintf(("Stack crawl of original lock: "));
		m_threadStacks[pid%m_maxThreads].Print();
		DebugPrintf(("\nStack crawl of this lock attempt: "));
		CallStack stack;
		stack.Update();
		stack.Print();
		DebugPrintf(("\n"));
		debugger("Read then write lock");

		m_writeLock.Lock();
		m_owner = pid;
		m_nested = 1;
		int32 readers = atomic_add(&m_count,-MAX_READERS);
		if (readers > 0) acquire_sem_etc(m_writerSem,readers-m_readLocked[pid%m_maxThreads],0,0);
		m_readLocked[pid%m_maxThreads] = 0;

		return;
	};
#endif

	m_writeLock.Lock();
	m_owner = pid;
	m_nested = 1;
	int32 readers = atomic_add(&m_count,-MAX_READERS);
	if (readers > 0) acquire_sem_etc(m_writerSem,readers,0,0);
};

int32 SuperDoubleExtraSpecialRWLock::WriteLock()
{
	int32 pid = xgetpid();
	WriteLock(pid);
	return pid;
};

void SuperDoubleExtraSpecialRWLock::WriteUnlock()
{
#if AS_DEBUG
	if (m_owner != xgetpid()) debugger("Write unlock called but I'm not the owner!\n");
#endif
	if (!(--m_nested)) {
		int32 readersWaiting = atomic_add(&m_count,MAX_READERS) + MAX_READERS;
		if (readersWaiting) release_sem_etc(m_readerSem,readersWaiting,B_DO_NOT_RESCHEDULE);
		m_owner = -1;
		m_writeLock.Unlock();
	};
};

void SuperDoubleExtraSpecialRWLock::DowngradeWriteToRead()
{
#if AS_DEBUG
	if (m_nested > 1) debugger("Downgrade requested but lock is nested!\n");
	m_readLocked[xgetpid()%m_maxThreads]++;
#endif
	if (!(--m_nested)) {
		int32 readersWaiting = atomic_add(&m_count,MAX_READERS+1) + MAX_READERS;
		if (readersWaiting) release_sem_etc(m_readerSem,readersWaiting,B_DO_NOT_RESCHEDULE);
		m_owner = -1;
		m_writeLock.Unlock();
	};
};

void SuperDoubleExtraSpecialRWLock::ReadLock(int32 pid)
{
	if (pid == m_owner) { m_nested++; return; };

#if HEAVY_DUTY_DEBUG
//	DebugPrintf(("SuperDoubleExtraSpecialRWLock::ReadLock(%d)\n",pid));
	if (m_readLocked[pid%m_maxThreads]) {
		DebugPrintf(("Thread %d read locking, but lock count is already %d!\n",pid,m_readLocked[pid%m_maxThreads]));
		DebugPrintf(("Stack crawl of original lock: "));
		m_threadStacks[pid%m_maxThreads].Print();
		DebugPrintf(("\nStack crawl of this lock attempt: "));
		CallStack stack;
		stack.Update();
		stack.Print();
		DebugPrintf(("\n"));
		m_readLocked[pid%m_maxThreads]++;
		debugger("Multiple read lock");
		return;
	};
	m_waitReadLock[pid%m_maxThreads]++;
	m_threadStacks[pid%m_maxThreads].Update();
#endif

	if (atomic_add(&m_count,1) < 0)
		acquire_sem(m_readerSem);

#if HEAVY_DUTY_DEBUG
	m_waitReadLock[pid%m_maxThreads]--;
	m_readLocked[pid%m_maxThreads]++;
#endif
};

int32 SuperDoubleExtraSpecialRWLock::ReadLock()
{
	int32 pid;
#if !HEAVY_DUTY_DEBUG
	if (m_owner == -1)
		pid = -2;
	else
#endif
		pid = xgetpid();
	ReadLock(pid);
	return pid;
};

void SuperDoubleExtraSpecialRWLock::ReadUnlock(int32 pid)
{
	if (pid == m_owner) { m_nested--; return; };

#if HEAVY_DUTY_DEBUG
	if (--m_readLocked[pid%m_maxThreads]) {
		DebugPrintf(("Thread %d unrolling multiple read-lock down to %d\n,",pid,m_readLocked[pid]));
		return;
	};
#endif

	if (atomic_add(&m_count,-1) < 0)
		release_sem_etc(m_writerSem,1,B_DO_NOT_RESCHEDULE);
};

int32 SuperDoubleExtraSpecialRWLock::ReadUnlock()
{
	int32 pid = xgetpid();
	ReadUnlock(pid);
	return pid;
};

SuperDoubleExtraSpecialRWLock::~SuperDoubleExtraSpecialRWLock()
{
	delete_sem(m_readerSem);
	delete_sem(m_writerSem);
#if HEAVY_DUTY_DEBUG
	dbFree(m_threadStacks);
	dbFree(m_readLocked);
	dbFree(m_waitReadLock);
	dbFree(m_actualThread);
#endif
};
