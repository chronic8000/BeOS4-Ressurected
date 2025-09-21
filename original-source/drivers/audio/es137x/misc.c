/************************************************************************/
/*                                                                      */
/*                              misc.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/ 
#include <KernelExport.h>
#include "debug.h"

area_id allocate_memory(const char* name, void ** log_addr, void ** phys_addr, size_t * size)
{
	area_id mem_area;
	physical_entry where; 
	void *addr;
	size_t new_size;

	new_size = (*size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);	/* multiple of page size */
	mem_area = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, 
							new_size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (mem_area < 0) {
		ddprintf("allocate_buffer(): failed!\n");
		return mem_area;
	}
	if (get_memory_map(addr, new_size, &where, 1) < 0) {
		delete_area(mem_area);
		mem_area = B_ERROR;
		ddprintf("allocate_buffer(): get_memory_map() failed!\n");		
		return B_ERROR;
	}
	ddprintf("Created memory area: areaId = %lX, phys addr = %lX, log addr = %lX, size = %lX\n",mem_area,(ulong)(where.address), (ulong)addr, new_size);
	*log_addr = (void *)addr;
	*phys_addr = (void *)where.address;
	*size = new_size; 

	return mem_area;
}


