#include <malloc.h>
#include <SupportDefs.h>
#include <KernelExport.h>

extern void my_free(void *myp);
extern void *my_malloc(size_t size);
extern void my_memory_used();

static int32 memallocated = 0;

void my_free(void *myp)
{
	int i;
	int badbytes = 0;
	char *p = (char*)myp - 4096;
	int size = *(uint32*)p;
	
//	dprintf(DEBUGPREFIX "myfree: 0x%08x\n", myp);

	i = 8;
	while(i<4096) {
		badbytes += p[i++] != '_';
		badbytes += p[i++] != 'M';
		badbytes += p[i++] != 'A';
		badbytes += p[i++] != 'L';
		badbytes += p[i++] != 'L';
		badbytes += p[i++] != 'O';
		badbytes += p[i++] != 'C';
		badbytes += p[i++] != '_';
	}
	while(i<(4096*2)) {
		badbytes += (p+size)[i++] != '_';
		badbytes += (p+size)[i++] != 'M';
		badbytes += (p+size)[i++] != 'A';
		badbytes += (p+size)[i++] != 'L';
		badbytes += (p+size)[i++] != 'L';
		badbytes += (p+size)[i++] != 'O';
		badbytes += (p+size)[i++] != 'C';
		badbytes += (p+size)[i++] != '_';
	}
	atomic_add(&memallocated, -size);
	if(size == 0) {
		dprintf(DEBUGPREFIX "myfree: 0x%08x, size=0, already free or corrupted\n", p+4096, badbytes, memallocated);
		return;
	}
	if(badbytes) {
		dprintf(DEBUGPREFIX "myfree: 0x%08x, bad bytes: %d, mem used = %d\n", p+4096, badbytes, memallocated);
		return;
	}
	for(i=0; i<size;i+=4) {
		((uint32*)myp)[i/4] = 0xbadfeed0;
	}
	*(uint32*)p = 0;
	free(p);
}

void *my_malloc(size_t size)
{
	char *p;
	int i;
	p = malloc(size+4096*2);
	if(p == NULL)
		return NULL;

	i = 0;
	while(i<4096) {
		p[i++] = '_';
		p[i++] = 'M';
		p[i++] = 'A';
		p[i++] = 'L';
		p[i++] = 'L';
		p[i++] = 'O';
		p[i++] = 'C';
		p[i++] = '_';
	}
	*((uint32*)p) = size;
	while(i<(4096*2)) {
		(p+size)[i++] = '_';
		(p+size)[i++] = 'M';
		(p+size)[i++] = 'A';
		(p+size)[i++] = 'L';
		(p+size)[i++] = 'L';
		(p+size)[i++] = 'O';
		(p+size)[i++] = 'C';
		(p+size)[i++] = '_';
	}
	atomic_add(&memallocated, size);
//	dprintf(DEBUGPREFIX "mymalloc(%d): at 0x%08x mem used = %d\n", size, p+4096, memallocated);
	return p+4096;
}

void
my_memory_used()
{
	if(memallocated != 0)
		dprintf(DEBUGPREFIX "memory used: %d\n", memallocated);
}
