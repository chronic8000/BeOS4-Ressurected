/*
    area.c: These function deal with Be areas or shared memory.
                             
	russ 4/28/99	

	(c) 1997-99 Be, Inc. All rights reserved

*/

#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>

#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

/* Get area gives me some memory that's mine all mine. */
uchar* get_area(char* label, size_t size, area_id* areaID)
{
	uchar* base;
	area_id id;

	size = RNDUP(size, B_PAGE_SIZE);
	id = create_area(label,
					(void**)&base,
					B_ANY_KERNEL_ADDRESS,
					size,
					B_FULL_LOCK | B_CONTIGUOUS,
					B_READ_AREA | B_WRITE_AREA);

	if  (id < B_NO_ERROR) 
		return (NULL);

	if  (lock_memory(base, size, B_DMA_IO | B_READ_DEVICE) != B_NO_ERROR) {
		delete_area(id);
		return (NULL);
	}
	
	memset(base, 0, size);
	*areaID = id;
	
	return (base);
}

/* The virt_to_bus function is called after I lay out memory in
   a contiguous area (see get_area above). It gives me physical addresses
   that are known to be contiguous, so I can make some asumptions like;
   1 virtual address 1 physical address entry, actually 2 because
   I need a 0 entry to terminate. */
uint32 virt_to_bus(void* addr, size_t size)
{
	physical_entry 	area_phys_addr[2];		

	get_memory_map(addr, size, &area_phys_addr[0], 1);
	
	return ((uint32)area_phys_addr[0].address);	
}  




