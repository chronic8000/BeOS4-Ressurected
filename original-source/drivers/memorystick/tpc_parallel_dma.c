#include "tpc_parallel_dma.h"
#include <KernelExport.h>

const size_t dma_buffer_size = B_PAGE_SIZE;
extern isa_module_info *isa;

status_t
create_dma_area(dma_info_t *info)
{
	const char *debugprefix = "parallelms -- create_dma_table_area: ";
	status_t		err;
	physical_entry	dmab_physical_entry;

	/* the area doesn't exist, try to create it */
	err = info->dma_area =
		create_area("PARALLEL_MEMORY_STICK_DMA_AREA", &info->dma_buffer,
		            B_ANY_KERNEL_ADDRESS, dma_buffer_size, B_LOMEM,
		            B_READ_AREA | B_WRITE_AREA);
	if(err < B_NO_ERROR) {
		dprintf("%scouldn't create dma area\n", debugprefix);
		goto err0;
	}
	info->dmainuse = false;
	info->last_size = 0;

	err = get_memory_map(info->dma_buffer, dma_buffer_size,
	                     &dmab_physical_entry, 1);
	if(err != B_NO_ERROR) {
		dprintf("%sdma table area created & locked,\n"
		        "couldn't get memory map\n", debugprefix);
		goto err1;
	}
	info->dma_buffer_physical = dmab_physical_entry.address;

	err = isa->lock_isa_dma_channel(info->dma_channel);
	if (err != B_NO_ERROR) {
		dprintf("%scould not lock dma channel %d\n", debugprefix, info->dma_channel);
		goto err1;
	}

	return B_NO_ERROR;

err1:
	delete_area(info->dma_area);
err0:
	return err;
}

/* -----
   delete_dma_table_area deletes a dma table area
----- */

status_t
delete_dma_area(dma_info_t *info)
{
	if(info->dma_area < 0)
		return B_ERROR;
	isa->unlock_isa_dma_channel(info->dma_channel);
	return delete_area(info->dma_area);
}

status_t 
start_dma(dma_info_t *info, size_t size, bool to_device)
{
	const char *debugprefix = "parallelms -- start_dma: ";
	status_t err;
	//bigtime_t t1;

	if(info->dma_area < 0) {
		dprintf("%sno dma area\n", debugprefix);
		return B_ERROR;
	}
	if(size > dma_buffer_size) {
		dprintf("%stransfer size %ld, too big (max %ld\n)", debugprefix,
		        size, dma_buffer_size);
		return B_ERROR;
	}
	//t1 = system_time();
	if(info->last_size != size || info->last_to_device != to_device) {
		//err = isa->start_isa_dma(info->dma_channel, info->dma_buffer_physical, size,
		//                         to_device ? 0x08 : 0x04, 0x00);
		err = isa->start_isa_dma(info->dma_channel, info->dma_buffer_physical, size,
		                         to_device ? 0x18 : 0x14, 0x00);
	}
	//dprintf("startdma: lock took %d us\n", system_time() - t1);
	info->last_size = size;
	info->last_to_device = to_device;
	//dprintf("startdma: start_isa_dma took %d us\n", system_time() - t1);
	info->dmainuse = true;
	//dprintf("dma size %d started\n", size);
	return B_NO_ERROR;
}

extern void get_dma_count_and_status (int channel, int *pcount, int *pstatus);

status_t
finish_dma(dma_info_t *info)
{
	const char *debugprefix = "parallelms -- finish_dma: ";
	int	dmastatus;
	int	dmacount;
	if(info->dma_area < 0) {
		return B_ERROR;
	}
	if(!info->dmainuse) {
		dprintf("%sdma was not started\n", debugprefix);
		return B_ERROR;
	}
	info->dmainuse = false;
	return B_NO_ERROR;
//	dprintf("IDE BEBOX -- finish_dma\n");

	{
//	bigtime_t t1 = system_time();
	//get_dma_count_and_status(info->dma_channel, &dmacount, &dmastatus);
	dmastatus = isa->read_io_8(0x08);
	dmacount = -1;
//	dprintf("get_dma_count_and_status %Ld us\n", system_time()-t1);
//	while((dmastatus & (1 << (info->dma_channel & 3))) == 0 &&
//	      system_time() - t1 < 1000) {
//		spin(2);
//		dmastatus = isa->read_io_8(0x08);
//	}
	}

	if((dmastatus & (1 << (info->dma_channel & 3))) == 0) {
		dprintf("%scount = %d, status = 0x%02x\n",
		        debugprefix, dmacount, dmastatus);
		info->last_size = 0;
		return B_INTERRUPTED;
	}

	return B_NO_ERROR;
}

status_t 
abort_dma(dma_info_t *info)
{
	info->dmainuse = false;
	info->last_size = 0;
	return B_NO_ERROR;
}

