
#include "heap.h"
#include "gr_types.h"
#include "proto.h"
#include "OS.h"
#include "as_support.h"
#include "as_debug.h"
#include <Debug.h>

/******* Chunking *********/

Chunker::Chunker(Stash<Chunk> *stash, uint32 size, int32 quantum) :
	m_freeList(NULL), m_stash(stash)
{
	if (quantum < 1) quantum = 1;
	m_quantum = quantum-1;
	m_quantumMask = 0xFFFFFFFF ^ (quantum-1);
	if (size > 0) Free(0,size);
};

Chunker::~Chunker()
{
	Chunk *p=m_freeList,*n;
	while (p) {
		n = p->next;
		m_stash->Free(p);
		p = n;
	};
};

void Chunker::Reset(uint32 size)
{
	Chunk *p=m_freeList,*n;
	while (p) {
		n = p->next;
		m_stash->Free(p);
		p = n;
	};
	m_freeList = NULL;
	if (size > 0) Free(0,size);
};

bool Chunker::Alloc(uint32 start, uint32 size)
{
	Chunk **p=&m_freeList,*it;

	while (*p && (((*p)->start + (*p)->size) < start)) p = &(*p)->next;

	it = *p;
	if (!it || 
		(it->start > start) || 
		((it->start + it->size) < (start+size))) {
		return false;
	};
	
	if (it->start == start) {
		if (it->size == size) {
			*p = it->next;
			m_stash->Free(it);
			return true;
		};
		
		it->size -= size;
		it->start += size;
		return true;
	};
	
	if ((it->start + it->size) == (start + size)) {
		it->size -= size;
		return true;
	};

	uint32 freeStart = it->start;
	uint32 freeSize = it->size;
	it->size = start - freeStart;
	it = m_stash->Alloc();
	it->start = start+size;
	it->size = freeSize - (start - freeStart) - size;
	it->next = (*p)->next;
	(*p)->next = it;
	return true;
};

uint32 Chunker::PreAbuttance(uint32 addr)
{
	Chunk **p=&m_freeList;
	while (*p && (((*p)->start + (*p)->size) != addr)) p = &(*p)->next;
	if (!(*p)) return 0;
	return (*p)->size;
};

uint32 Chunker::PostAbuttance(uint32 addr)
{
	Chunk **p=&m_freeList;
	while (*p && ((*p)->start != addr)) p = &(*p)->next;
	if (!(*p)) return 0;
	return (*p)->size;
};

uint32 Chunker::Alloc(uint32 &size, bool takeBest, uint32 atLeast)
{
	Chunk **p=&m_freeList,*it,**c;
	uint32 s=size,start;
	
	while (*p && ((*p)->size < s)) p = &(*p)->next;
	if (!(*p)) {
		if (takeBest) {
			p=&m_freeList;
			c = NULL;
			s = atLeast-1;
			while (*p) {
				if ((*p)->size > s) {
					s = (*p)->size;
					c = p;
				};
				p = &(*p)->next;
			};
			if (c) {
				size = s;
				it = *c;
				*c = (*c)->next;
				start = it->start;
				m_stash->Free(it);
				return start;
			};
		};
		size = 0;
		return NONZERO_NULL;
	};
	
	it = *p;
	s = it->size;
	start = it->start;
	if (s <= (size + m_quantum)) {
		size = s;
		*p = it->next;
		m_stash->Free(it);
	} else {
		size = (size + m_quantum) & m_quantumMask;
		it->size -= size;
		it->start += size;
	};
	
	return start;
};

void Chunker::Free(uint32 start, uint32 size)
{
	Chunk *n;
	Chunk *dummy = NULL;
	Chunk **p1=&m_freeList,**p2;
	uint32 s=size;

	if (!(*p1)) {
		n = m_stash->Alloc();
		n->start = start;
		n->size = size;
		n->next = m_freeList;
		m_freeList = n;
		return;
	};

	if ((*p1)->start > start) {
		p2 = p1;
		p1 = &dummy;
	} else {
		p2 = &(*p1)->next;
		while (*p2) {
			if ((*p2)->start > start) break;
			p1 = p2;
			p2 = &(*p2)->next;
		};
	};
	
	n = *p1;
	if (n && ((n->start + n->size) == start)) {
		n->size += size;
		if (n->next && (n->next->start == (n->start+n->size))) {
			n = n->next;
			(*p1)->next = n->next;
			(*p1)->size += n->size;
			m_stash->Free(n);
		};
		return;
	};

	n = *p2;
	if (!n || ((start + size) != n->start)) {
		n = m_stash->Alloc();
		n->start = start;
		n->size = size;
		n->next = *p2;
		*p2 = n;
		return;
	};
	
	n->start -= size;
	n->size += size;
};

void Chunker::CheckSanity()
{
	Chunk *p=m_freeList;
	uint32 total=0;
	int32 chunkNum=0;

	while (p->next) {
		if ((p->start + p->size) > p->next->start) {
			DebugPrintf((	"Chunk %d end location (%08x) is greater "
							"than chunk %d start location (%08x)\n",
							chunkNum,(p->start + p->size),
							chunkNum+1,p->next->start));
			SpewChunks();
			exit(1);
		};
		p = p->next;
		chunkNum++;
	};
};

void Chunker::SpewChunks()
{
	Chunk *p=m_freeList;
	uint32 total=0;
	int32 chunkNum=0;

	DebugPrintf(("-------------------------------\n"));

	while (p) {
		DebugPrintf(("chunk[%d] : %08x --> %d\n",
			chunkNum,p->start,p->size));
		p = p->next;
		chunkNum++;
	};

	DebugPrintf(("-------------------------------\n"));
};

uint32 Chunker::TotalChunkSpace()
{
	Chunk *p=m_freeList;
	uint32 total=0;
	
	while (p) {
		total += p->size;
		p = p->next;
	};

	return total;
};

uint32 Chunker::LargestChunk()
{
	Chunk *p=m_freeList;
	Chunk *l=p;
	
	while (p) {
		if (p->size > l->size) l = p;
		p = p->next;
	};

	return l->size;
};

/******* Hashing *********/

HashTable::HashTable(int32 buckets)
{
	m_buckets = buckets;
	m_hashTable = (HashEntry**)grMalloc(sizeof(void*) * m_buckets,"HashTable::m_hashTable",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_buckets;i++) m_hashTable[i] = NULL;
};

HashTable::~HashTable()
{
	grFree(m_hashTable);
};

void HashTable::Insert(uint32 key, void *ptr)
{
	int32 bucket;
	HashEntry *n = m_stash.Alloc();
	n->car = ptr;
	n->key = key;
	bucket = Hash(key);
	n->next = m_hashTable[bucket];
	m_hashTable[bucket] = n;
};

void * HashTable::Retrieve(uint32 key)
{
	uint32 bucket = Hash(key);
	HashEntry **p = &m_hashTable[bucket];
	while (*p && ((*p)->key != key)) p = &(*p)->next;
	if (!(*p)) return NULL;
	return (*p)->car;
};

void HashTable::Remove(uint32 key)
{
	uint32 bucket = Hash(key);
	HashEntry **p = &m_hashTable[bucket],*it;
	while (*p && ((*p)->key != key)) p = &(*p)->next;
	if (!(*p)) return;

	it = *p;
	*p = (*p)->next;
	m_stash.Free(it);
};

uint32 HashTable::Hash(uint32 address)
{
	return address % m_buckets;
};

/***************************/

Hasher::Hasher(Stash<Chunk> *stash)
{
	m_hashTable = (Chunk**)grMalloc(64 * sizeof(Chunk*),"Hasher::m_hashTable",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<64;i++) m_hashTable[i] = NULL;
	m_stash = stash;
};

Hasher::~Hasher()
{
	grFree(m_hashTable);
};

void Hasher::RemoveAll()
{
	for (int32 i=0;i<64;i++) {
		Chunk *p = m_hashTable[i];
		while (p) {
			Chunk *next = p->next;
			m_stash->Free(p);
			p = next;
		};
		m_hashTable[i] = NULL;
	};
};

void Hasher::Insert(uint32 address, uint32 size)
{
	int32 bucket;
	Chunk *n = m_stash->Alloc();
	n->start = address;
	n->size = size;
	bucket = Hash(address);
	n->next = m_hashTable[bucket];
	m_hashTable[bucket] = n;
};

void Hasher::Retrieve(uint32 address, uint32 **ptrToSize)
{
	uint32 bucket = Hash(address);
	Chunk **p = &m_hashTable[bucket],*it;
	while (*p && ((*p)->start != address)) p = &(*p)->next;
	if (!(*p)) {
		if (ptrToSize) *ptrToSize = NULL;
		return;
	};
	
	if (ptrToSize) *ptrToSize = &(*p)->size;
};

uint32 Hasher::Remove(uint32 address)
{
	uint32 bucket = Hash(address);
	Chunk **p = &m_hashTable[bucket],*it;
	while (*p && ((*p)->start != address)) p = &(*p)->next;
	if (!(*p)) return 0;
	
	it = *p;
	*p = it->next;
	bucket = it->size;
	m_stash->Free(it);
	return bucket;
};

uint32 Hasher::Hash(uint32 address)
{
	uint32 h=address,r;
	h = h ^ (h >> 8);
	h = h ^ (h >> 16);
	h = h ^ (h >> 24);
	r = h & 7;
	h = ((h&0xFF)>>(r)) | ((h&0xFF)<<(8-r));
	h &= 63;

	return h;
};

/******* Areas *********/

/*	The net kit creates some areas indiscriminately.  We'll get out of it's
	way while it thrashes around. */
#ifdef REMOTE_DISPLAY
#define AREA_LOW_END	0xb0000000
#else
#define AREA_LOW_END	0x90000000
#endif
#define AREA_HIGH_END	0xe0000000

int32			Area::m_maxThreads;
ThreadInfo **	Area::m_threadInfo;
Benaphore 		Area::m_areaAllocLock;
Stash<Chunk>	Area::m_areaChunkStash;
Chunker			Area::m_areaChunker(&Area::m_areaChunkStash,
					(AREA_HIGH_END-AREA_LOW_END)/B_PAGE_SIZE,1);

void Area::InitThreadInfo()
{
	system_info si;
	get_system_info(&si);
	m_threadInfo = (ThreadInfo**)grMalloc(si.max_threads * sizeof(ThreadInfo*),"threadinfo table",MALLOC_CANNOT_FAIL);
	memset(m_threadInfo,0,si.max_threads * sizeof(ThreadInfo*));
	m_maxThreads = si.max_threads;
}

void Area::BasePtrLock()
{
	int32 i;
	int32 threadID = find_thread(NULL) % m_maxThreads;
	ThreadInfo **ti,**first = m_threadInfo + threadID;

	ti = first;
	while (*ti) {
		for (i=0;i<SIMUL_AREA_LOCKS;i++) {
			if ((*ti)->ids[i] == ID()) { (*ti)->counts[i]++; return; };
		};
		ti = &(*ti)->next;
	};

	ti = first;
	while (1) {
		if (!(*ti)) {
			*ti = (ThreadInfo*)grMalloc(sizeof(ThreadInfo),"threadinfo struct",MALLOC_CANNOT_FAIL);
			for (i=0;i<SIMUL_AREA_LOCKS;i++) (*ti)->ids[i] = -1;
			(*ti)->next = NULL;
		};
		if (!(*ti)) debugger("PANIC!  No space left for new ThreadInfo struct?!?");
		for (i=0;i<SIMUL_AREA_LOCKS;i++) {
			if ((*ti)->ids[i] == -1) {
				m_basePtrLock.ReadLock();
				/*	ID() must be used AFTER we have locked, otherwise it can
					hcnage between the ID() check and the lock, if the area is
					resized and cloned in the process */
				(*ti)->counts[i] = 1;
				(*ti)->ids[i] = ID();
				return;
			};
		};
		ti = &(*ti)->next;
	};
	
	debugger("PANIC!  Should never get here! (BasePtrLock)");
}

void Area::BasePtrUnlock()
{
	int32 i;
	int32 threadID = find_thread(NULL) % m_maxThreads;
	ThreadInfo *ti = m_threadInfo[threadID];

	while (ti) {
		for (i=0;i<SIMUL_AREA_LOCKS;i++) {
			if (ti->ids[i] == ID()) {
				if (!(--ti->counts[i])) {
					ti->ids[i] = -1;
					m_basePtrLock.ReadUnlock();
				};
				return;
			};
		};
		ti = ti->next;
	};
	debugger("PANIC!  Should never get here! (BasePtrUnlock)");
}

void Area::DumpAreas()
{
	m_areaAllocLock.Lock();
	DebugPrint(("Dumping area freelist:\n"));
	m_areaChunker.SpewChunks();
	m_areaAllocLock.Unlock();
};

void Area::RescanAreas()
{
	area_info ai;
	uint32 i=0;

	m_areaChunker.Reset((AREA_HIGH_END-AREA_LOW_END)/B_PAGE_SIZE);

	while (get_next_area_info(0,(int32*)&i,&ai) == B_OK) {
		uint32 start = (uint32)ai.address;
		uint32 size = (uint32)ai.size;
		if ((start+size > AREA_LOW_END) && (start < AREA_HIGH_END)) {
			if (start < AREA_LOW_END) {
				size -= AREA_LOW_END - start;
				start = AREA_LOW_END;
			};
			if (start+size > AREA_HIGH_END)
				size = AREA_HIGH_END - start;
			m_areaChunker.Alloc((start-AREA_LOW_END)/B_PAGE_SIZE,size/B_PAGE_SIZE);
		};
	};
};

area_id Area::CreateArea(	char *name, void **basePtr,
							uint32 flags, uint32 size,
							uint32 locking, uint32 perms)
{
	m_areaAllocLock.Lock();
	status_t r = create_area(name,basePtr,flags,size,locking,perms);
	RescanAreas();
	m_areaAllocLock.Unlock();
	return r;
};

status_t Area::ResizeArea(area_id areaToResize, uint32 newSize)
{
	m_areaAllocLock.Lock();
	status_t r = resize_area(areaToResize,newSize);
	RescanAreas();
	m_areaAllocLock.Unlock();
	return r;
};

status_t Area::DeleteArea(area_id areaToDelete)
{
	m_areaAllocLock.Lock();
	status_t r = delete_area(areaToDelete);
	RescanAreas();
	m_areaAllocLock.Unlock();
	return r;
};

Area::~Area()
{
	m_areaAllocLock.Lock();
	m_basePtrLock.WriteLock();
	delete_area(m_area);
	uint32 start = (((uint32)m_basePtr) - AREA_LOW_END) / B_PAGE_SIZE;
	uint32 size = m_size;
	m_areaChunker.Free(start,size);
	m_areaAllocLock.Unlock();
};

Area::Area(char *name, uint32 size)
{
	m_areaAllocLock.Lock();

	m_size = (size+4095) >> 12;
	uint page = m_areaChunker.Alloc(m_size);
	if (page == NONZERO_NULL) {
		DebugPrintf(("Area::Area(\"%s\",%d): Could not allocate space\n",name,size));
		m_area = B_ERROR;
		m_basePtr = NULL;
		m_areaAllocLock.Unlock();
		return;
	};

	m_basePtr = (void*)((page * B_PAGE_SIZE) + AREA_LOW_END);
	m_area = create_area(	name,&m_basePtr,
							B_EXACT_ADDRESS,m_size*B_PAGE_SIZE,B_NO_LOCK,
							B_READ_AREA|B_WRITE_AREA);

	if (m_area < 0) {
		DebugPrintf(("Area::Area(\"%s\",%d): Kernel doesn't agree!\n",name,size));
		m_basePtr = NULL;
		m_areaAllocLock.Unlock();
		return;
	};

	m_areaAllocLock.Unlock();
};

uint32 Area::ResizeTo(uint32 newSize, bool acceptBest)
{
	area_info ai;
	uint32 i=0,s=newSize/B_PAGE_SIZE;
	void *p;
	area_id a;

	m_areaAllocLock.Lock();

	uint32 start = (((uint32)m_basePtr) - AREA_LOW_END) / B_PAGE_SIZE;
	uint32 size = m_size;

	if (m_areaChunker.PostAbuttance(start+size) >= s-size) {
		if (resize_area(m_area,s*B_PAGE_SIZE) == B_OK) {
			m_size=s;
		} else {
			DebugPrintf(("Area::ResizeTo(): Chunker and kernel don't agree\n",newSize));
		};
		m_areaChunker.Alloc(start+size,s-size);
		m_areaAllocLock.Unlock();
		return m_size*B_PAGE_SIZE;
	};
	
	i = m_areaChunker.Alloc(s,acceptBest,m_size+1);
	if (s == 0) {
		/* There is no larger memory space available.  Fail. */
		DebugPrintf(("Area::ResizeTo(): No larger linear address space available\n"));
		m_areaAllocLock.Unlock();
		return m_size*B_PAGE_SIZE;
	};
	
	p = (void*)(i*B_PAGE_SIZE + AREA_LOW_END);
	get_area_info(m_area,&ai);
	a = clone_area(ai.name,&p,B_EXACT_ADDRESS,B_READ_AREA|B_WRITE_AREA,m_area);
	if (a < 0) {
		/*	This should never happen, unless there's 
			a BIG bug... */
		DebugPrintf(("Area::ResizeTo(): clone_area failed\n"));
		m_areaAllocLock.Unlock();
		return m_size*B_PAGE_SIZE;
	};

	m_areaChunker.Free(start,m_size);
	m_basePtrLock.WriteLock();
		delete_area(m_area);
		m_area = a;
		m_basePtr = p;
	m_basePtrLock.WriteUnlock();

	/*	This should always work, unless someone else is
		creating or resizing areas without locking first. */
	if (resize_area(m_area,s*B_PAGE_SIZE) == B_OK) {
		m_size = s;
	} else {
		DebugPrintf(("Area::ResizeTo(): Can't resize cloned area!\n"));
	};

	m_areaAllocLock.Unlock();

	return m_size*B_PAGE_SIZE;
};

/******* Heaping *********/

uint32 Heap::Alloc(uint32 size)
{
	return (uint32)grMalloc(size,"Heap alloc",MALLOC_CANNOT_FAIL);
};

uint32 Heap::Realloc(uint32 ptr, uint32 newSize)
{
	return (uint32)grRealloc((void*)ptr,newSize,"Heap alloc",MALLOC_CANNOT_FAIL);
};

void Heap::Free(uint32 ptr)
{
	grFree((void*)ptr);
};

void Heap::Lock()
{
}

void Heap::Unlock()
{
}

uint32 Heap::Offset(void *ptr)
{
	return (uint32)ptr;
}

void * Heap::Ptr(uint32 offset)
{
	return (void *)offset;
}

/******************************************/

HeapArea::HeapArea(char *name, uint32 size, uint32 quantum) : 
	Area(name,size), m_freeList(&m_stash,size,quantum), m_allocs(&m_stash)
{
};

uint32 HeapArea::Alloc(uint32 size)
{
	m_heapLock.Lock();

	uint32 s,p,abutt,i;
	s = size;
	p = m_freeList.Alloc(s);
	if (s < size) {
		abutt = m_freeList.PreAbuttance(Size());
		s = size - abutt;
		p = Size();
		if (s % B_PAGE_SIZE) s = ((s/B_PAGE_SIZE)+1)*B_PAGE_SIZE;
		i = ResizeTo(s+p);
		if (i!=(s+p)) {
			/*	Ack!  We weren't able to resize the area, even by moving it,
				to get the needed space.  Oh, well... */
			DebugPrintf(("Heap::Alloc(%d): Can't resize area!\n",size));
			DebugPrintf(("                 abutt=%d, s=%d, p=%d, i=%d\n",abutt,s,p,i));
			DumpFreeList();
			m_heapLock.Unlock();
			return NONZERO_NULL;
		};
		m_freeList.Free(p,s);
		s = size;
		p = m_freeList.Alloc(s);
		if (s < size) {
			/* There's a bug in my code, and for some reason we can't allocate
				the needed space, even through the area was resized okay. */
			DebugPrintf(("Heap::Alloc(%d): Area resized but still can't alloc!\n",size));
			m_heapLock.Unlock();
			return NONZERO_NULL;
		};
	};

	m_allocs.Insert(p,s);
	m_heapLock.Unlock();
	return p;
};

uint32 HeapArea::Realloc(uint32 ptr, uint32 newSize)
{
	m_heapLock.Lock();
	uint32 *size,r = NONZERO_NULL;
	m_allocs.Retrieve(ptr,&size);
	if (*size != 0) {
		int32 quantizedSize =	(newSize + m_freeList.m_quantum) &
								m_freeList.m_quantumMask;
		if (quantizedSize == *size) {
			r = ptr;
			goto leave;
		};

		if (quantizedSize < *size) {
			m_freeList.Free(ptr+quantizedSize,*size-quantizedSize);
			*size = quantizedSize;
		} else {
			uint32 oldSize = m_allocs.Remove(ptr);
			m_freeList.Free(ptr,*size);
			/*	The data @ptr is still valid, because we have the heapLock */
			uint32 newPtr = Alloc(newSize);
			BasePtrLock();
			memmove(Ptr(newPtr),Ptr(ptr),oldSize);
			BasePtrUnlock();
			r = newPtr;
		};
	};

	leave:
	m_heapLock.Unlock();
	return r;
};

void HeapArea::Free(uint32 ptr)
{
	m_heapLock.Lock();
		uint32 size = m_allocs.Remove(ptr);
		if (size != 0) m_freeList.Free(ptr,size);
	m_heapLock.Unlock();
};

void HeapArea::Lock()
{
	BasePtrLock();
}

void HeapArea::Unlock()
{
	BasePtrUnlock();
}

uint32 HeapArea::Offset(void *ptr)
{
	return Ptr2Offset(ptr);
}

void * HeapArea::Ptr(uint32 offset)
{
	return Offset2Ptr(offset);
}

HeapArea::~HeapArea()
{
};

void HeapArea::Reset()
{
	m_heapLock.Lock();
		m_freeList.Reset(Size());
		m_allocs.RemoveAll();
	m_heapLock.Unlock();
};

uint32 HeapArea::FreeSpace()
{
	return m_freeList.TotalChunkSpace();
};

uint32 HeapArea::LargestFree()
{
	return m_freeList.LargestChunk();
};

void HeapArea::CheckSanity()
{
	m_freeList.CheckSanity();
};

void HeapArea::DumpFreeList()
{
	m_freeList.SpewChunks();
};
