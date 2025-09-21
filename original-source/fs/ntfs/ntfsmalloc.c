#ifdef NTFS_MALLOC

#include "ntfs.h"

int32 memory_malloced;
int32 outstanding_mallocs;

void *ntfs_malloc(size_t size)
{
	void *temp = (void *)malloc(size);

	if(temp) {
		outstanding_mallocs++;
		DEBUGPRINT(MALLOC, 7, ("ntfs_malloc: block of %ld size allocated, %d blocks allocated. ptr = 0x%x\n", size, (int)outstanding_mallocs, (unsigned int)temp));
	} else {
		DEBUGPRINT(MALLOC, 7, ("ntfs_malloc: malloc returned a NULL after trying to allocate %ld bytes.\n", size));
	}

	return temp;	
}

void ntfs_free(void *ptr)
{
	outstanding_mallocs--;
	DEBUGPRINT(MALLOC, 7, ("ntfs_free: freeing at 0x%x, now %d blocks allocated.\n", (unsigned int)ptr, (int)outstanding_mallocs));
	free(ptr);
}

#endif
