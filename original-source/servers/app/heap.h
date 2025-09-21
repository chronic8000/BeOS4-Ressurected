
//******************************************************************************
//
//	File:			heap.h
//	Description:	Fun with heaps!
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#ifndef HEAP_H
#define HEAP_H

#include "as_support.h"
#include "lock.h"
#include "atom.h"

#define SIMUL_AREA_LOCKS 6
struct ThreadInfo {
	uint8		counts[SIMUL_AREA_LOCKS];
	area_id		ids[SIMUL_AREA_LOCKS];
	ThreadInfo *next;
};

struct Chunk {
	uint32 start,size;
	Chunk *next;
};

#define NONZERO_NULL	0xFFFFFFFF

class HeapArea;

template <class t>
class Stash {

	public:

							Stash() { m_theStash = NULL; };
							~Stash()
							{
								for (int32 n = m_freedom.CountItems()-1;n>=0;n--)
									free(m_freedom[n]);
							};
							
		t * 				Alloc()
		{
			t *n = m_theStash;
			if (n) {
				m_theStash = n->next;
				return n;
			};
			
			int32 count = 1 << ((m_freedom.CountItems()+2)<<1);
			n = (t*)malloc(sizeof(t) * count);
			m_freedom.AddItem(n);
			for (;count;count--) {
				n->next = m_theStash;
				m_theStash = n;
				n++;
			};
		
			n = m_theStash;
			m_theStash = n->next;
			return n;
		};

		void				Free(t *n)
		{
			n->next = m_theStash;
			m_theStash = n;
		};

	private:
		t *					m_theStash;
		BArray<t*>			m_freedom;
};

class Chunker {
	public:
							Chunker(Stash<Chunk> *stash, uint32 size=0, int32 chunkQuantum=1);
							~Chunker();
		uint32				Alloc(uint32 &size, bool takeBest=false, uint32 atLeast=1);
		bool				Alloc(uint32 start, uint32 size);
		void				Free(uint32 start, uint32 size);
		
		uint32				PreAbuttance(uint32 addr);
		uint32				PostAbuttance(uint32 addr);
		uint32				TotalChunkSpace();
		uint32				LargestChunk();
		void				CheckSanity();
		void				SpewChunks();

		void				Reset(uint32 size);

	private:
		friend				class HeapArea;

		Chunk *				m_freeList;
		Stash<Chunk> *		m_stash;
		uint32				m_quantum;
		uint32				m_quantumMask;
};

struct HashEntry {
	void *car;
	uint32 key;
	HashEntry *next;
};

class HashTable {
	public:
							HashTable(int32 buckets=64);
							~HashTable();
							
		void				Insert(uint32 key, void *ptr);
		void *				Retrieve(uint32 key);
		void				Remove(uint32 key);
	
	private:

		uint32				Hash(uint32 address);

		int32				m_buckets;
		HashEntry **		m_hashTable;
		Stash<HashEntry>	m_stash;
};

class Hasher {
	public:
							Hasher(Stash<Chunk> *stash);
							~Hasher();
							
		void				Insert(uint32 address, uint32 size);
		void				Retrieve(uint32 key, uint32 **ptrToSize=NULL);
		uint32				Remove(uint32 key);
		void				RemoveAll();
	
	private:

		uint32				Hash(uint32 address);

		Chunk **			m_hashTable;
		Stash<Chunk> *		m_stash;
};

class Area : virtual public SAtom {

	public:
								Area(char *name, uint32 size);
		virtual					~Area();

		inline uint32			Ptr2Offset(void *ptr) const
								{ return (((uint32)ptr) - ((uint32)m_basePtr)); };
		inline void *			Offset2Ptr(uint32 offset) const
								{ return (void*)(((uint32)m_basePtr) + offset); };
		inline	void *			BasePtr() const { return m_basePtr; };
		inline	area_id			ID() const { return m_area; };
		inline	uint32			Size()  const { return m_size * B_PAGE_SIZE; };
		uint32					ResizeTo(uint32 newSize, bool acceptBest=false);

				void			BasePtrLock();
				void			BasePtrUnlock();

		/*	Stuff below MUST be used for creation, resizing, and deletion
			of areas which do don't use this object to represent them. */
		static	area_id			CreateArea(	char *name, void **basePtr,
											uint32 flags, uint32 size,
											uint32 locking, uint32 perms);
		static	status_t		ResizeArea(	area_id areaToResize,
											uint32 newSize);
		static	status_t		DeleteArea(	area_id areaToDelete);
		static inline void		GlobalAreaLock() { m_areaAllocLock.Lock(); };
		static inline void		GlobalAreaUnlock() { m_areaAllocLock.Unlock(); };
		static inline void		GlobalAreaUnlockRescan() { RescanAreas(); m_areaAllocLock.Unlock(); };
		static	void			InitThreadInfo();

		static	void			DumpAreas();

	private:

		static	void			RescanAreas();

				area_id			m_area;
				void *			m_basePtr;
				uint32			m_size;
				RWLock			m_basePtrLock;

		static	int32			m_maxThreads;
		static	ThreadInfo **	m_threadInfo;
		static	Benaphore		m_areaAllocLock;
		static	Stash<Chunk>	m_areaChunkStash;
		static	Chunker			m_areaChunker;
};

class Heap : virtual public SAtom {
	public:
							Heap() {};
		virtual				~Heap() {};

		virtual	uint32		Alloc(uint32 size);
		virtual	uint32		Realloc(uint32 ptr, uint32 newSize);
		virtual	void		Free(uint32 ptr);
		inline	void		Free(void *ptr) { Free(Offset(ptr)); };

		virtual	void		Lock();
		virtual	void		Unlock();

		virtual uint32		Offset(void *ptr);
		virtual void *		Ptr(uint32 offset);
};

class HeapArea : public Area, public Heap {

	public:
							HeapArea(char *name, uint32 size, uint32 quantum=32);
		virtual				~HeapArea();

				void		Reset();
		virtual	uint32		Alloc(uint32 size);
		virtual	uint32		Realloc(uint32 ptr, uint32 newSize);
		virtual	void		Free(uint32 ptr);
		inline	void		Free(void *ptr) { Free(Offset(ptr)); };

		virtual	void		Lock();
		virtual	void		Unlock();

		virtual uint32		Offset(void *ptr);
		virtual void *		Ptr(uint32 offset);

				uint32		FreeSpace();
				uint32		LargestFree();
				void		CheckSanity();
				void		DumpFreeList();
	
	private:

		Stash<Chunk>		m_stash;
		Chunker				m_freeList;
		Hasher				m_allocs;
		Benaphore			m_heapLock;
};

#endif
