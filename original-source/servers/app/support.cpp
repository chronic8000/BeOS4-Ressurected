
//******************************************************************************
//
//	File:		support.cpp
//
//	Description:	Some useful classes/functions
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "gr_types.h"
#include "proto.h"
#include "OS.h"
#include "as_support.h"
#include "as_debug.h"

#include <Debug.h>

#ifdef INTERNAL_LEAKCHECKING

struct ReportRec {
	CallStack stack;
	int32 count;
	int32 size;
	char comment[40];
	ReportRec *next;
};

struct AllocRec {
	void *ptr;
	int32 size;
	char comment[40];
	CallStack stack;
	AllocRec *next;
};

#define ALLOC_HASH 128
static AllocRec * allocHash[ALLOC_HASH];
static uint32 totalAlloced=0;
static sem_id leakyAllocSem=B_BAD_SEM_ID;
static int32 inited=0;

int32 Ptr2Hash(void *ptr)
{
	return (((uint32)ptr) >> 4) % ALLOC_HASH;
};

static void AddPtr(void *ptr, int32 size, char *comment)
{
	int32 hash = Ptr2Hash(ptr);
	AllocRec *r = new AllocRec;
	r->next = allocHash[hash];
	allocHash[hash] = r;
	r->ptr = ptr;
	r->size = size;
	strncpy(r->comment,comment,39);
	r->comment[39] = 0;
	r->stack.Update(0);
};

static int32 RemovePtr(void *ptr, char *comment, CallStack *stack=NULL)
{
	int32 hash = Ptr2Hash(ptr);
	AllocRec *a,**r = &allocHash[hash];
	while (*r && ((*r)->ptr != ptr)) r = &(*r)->next;
	if (!*r) return -1;
	a = *r;
	*r = (*r)->next;
	int32 size = a->size;
	strncpy(comment,a->comment,39);
	comment[39] = 0;
	if (stack) *stack = a->stack;
	delete a;
	return size;
};

static void LockLeaky()
{
	int32 hum = atomic_or(&inited,1);
	if (hum) {
		while (leakyAllocSem==B_BAD_SEM_ID) { snooze(3000); };
	} else {
		for (int32 i=0;i<ALLOC_HASH;i++) allocHash[i] = NULL;
		leakyAllocSem = create_sem(1,"leakyAllocSem");
	};
	acquire_sem(leakyAllocSem);
};

static void UnlockLeaky()
{
	release_sem(leakyAllocSem);
};

char * grLeakyStrdup(const char *srcStr, char *comment, int32 prio)
{
	void *p;

	LockLeaky();
	int32 size = strlen(srcStr)+1;
	AddPtr(p=strdup(srcStr),size,comment);
	totalAlloced += size;
	UnlockLeaky();
	
	return (char*)p;
};

void * grLeakyMalloc(int32 size, char *comment, int32 prio)
{
	void *p;

	LockLeaky();
	AddPtr(p=malloc(size),size,comment);
	totalAlloced += size;
//	DebugPrintf(("LeakyMalloc(%08x,\"%s\",%d): %d bytes outstanding\n",p,comment,size,totalAlloced));
	UnlockLeaky();
	
	return p;
};

int32 LeakyLookup(void *ptr, char *comment, CallStack *stack=NULL)
{
	LockLeaky();
	int32 hash = Ptr2Hash(ptr);
	AllocRec *a,**r = &allocHash[hash];
	while (*r && ((*r)->ptr != ptr)) r = &(*r)->next;
	if (!*r) return -1;
	a = *r;
	int32 size = a->size;
	strncpy(comment,a->comment,39);
	comment[39] = 0;
	if (stack) *stack = a->stack;
	UnlockLeaky();
	return size;
};

void grLeakyFree(void *ptr)
{
	int32 size;
	char comment[40];
	char callstack[1024];
	CallStack stack;

	LockLeaky();
	size = RemovePtr(ptr,comment,&stack);
	if (size < 0) {
		// mathias 12/9/2000: don't complain if it's the NULL ptr. freeing NULL is legal.
		if (ptr)
		{
			stack.Update();
			stack.SPrint(callstack);
			DebugPrintf(("LeakyFree(%08x): not LeakyMalloced! %s\n",ptr,callstack));
		}
	} else {
		totalAlloced -= size;
		// DebugPrintf(("LeakyFree(%08x,\"%s\",%d): %d bytes outstanding\n",ptr,comment,size,totalAlloced));
	}
	UnlockLeaky();

	free(ptr);
};

void * grLeakyCalloc(int32 size1, int32 size2, char *comment, int32 prio)
{
	void *p;
	int32 size = size1*size2;

	LockLeaky();
	AddPtr(p=calloc(size1,size2),size,comment);
	totalAlloced += size;
//	DebugPrintf(("LeakyCalloc(%08x,\"%s\",%d): %d bytes outstanding\n",p,comment,size,totalAlloced));
	UnlockLeaky();
	
	return p;
};

void * grLeakyRealloc(void *ptr, int32 size, char *comment, int32 prio)
{
	void *p;
	int32 oldSize;
	char oldComment[40];

	LockLeaky();
	oldSize = RemovePtr(ptr,oldComment);
	if (oldSize < 0) {
		totalAlloced += size;
		if (ptr != NULL) {
			DebugPrintf(("LeakyRealloc(%08x,\"%s\",%d): (not LeakyMalloced) %d bytes outstanding\n",ptr,comment,size,totalAlloced));
		};
	} else {
		totalAlloced -= oldSize;
		totalAlloced += size;
//		DebugPrintf(("LeakyRealloc(%08x,\"%s\",%d): %d bytes outstanding\n",ptr,comment,size,totalAlloced));
	};
	AddPtr(p=realloc(ptr,size),size,comment);
	UnlockLeaky();
	
	return p;
};

void grLeakyReport(int32 count, char *tag, int32 priority, int32 longReport)
{
	LockLeaky();

	int32 totalReport=0;
	ReportRec *reports=NULL,*r;
	AllocRec *current;
	for (int32 i=0;i<ALLOC_HASH;i++) {
		current = allocHash[i];
		while (current) {
			if (!tag || (strcmp(tag,current->comment) == 0)) {
				for (r=reports;r;r=r->next) {
					if (r->stack == current->stack) {
						r->count++;
						r->size += current->size;
						totalReport += current->size;
						break;
					};
				};
				if (!r) {
					r = new ReportRec;
					r->size = current->size;
					totalReport += current->size;
					r->count = 1;
					r->stack = current->stack;
					strcpy(r->comment,current->comment);
					r->next = reports;
					reports = r;
				};
			};
			current = current->next;
		};
	};
	
	char stack[1024];
	DebugPrintf((    "-- Malloc report ----------------- Total malloced: %d --------------------------\n",totalAlloced));
	ReportRec **highest,**iter;
	while (reports && count--) {
		for (iter=highest=&reports;*iter;iter=&(*iter)->next)
			if ((*iter)->count > (*highest)->count) highest = iter;
		if (!longReport) {
			(*highest)->stack.SPrint(stack);
			DebugPrintf(("   %20s  %10d  %8d    %s\n",(*highest)->comment,(*highest)->size,(*highest)->count,stack));
		} else {
			DebugPrintf(("- %20s --- total bytes = %10d --- total allocs = %8d ------\n",(*highest)->comment,(*highest)->size,(*highest)->count));
			(*highest)->stack.LongPrint();
		};
		r = (*highest)->next;
		delete *highest;
		*highest = r;
	};

	while (reports) {
		r = reports->next;
		delete reports;
		reports = r;
	};

	UnlockLeaky();
};

#endif
