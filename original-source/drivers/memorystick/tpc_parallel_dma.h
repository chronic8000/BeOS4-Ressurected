#ifndef TPC_PARALLEL_DMA_H
#define TPC_PARALLEL_DMA_H

#include <OS.h>
#include <ISA.h>

typedef struct {
	int             dma_channel;
	void           *dma_buffer;
	void           *dma_buffer_physical;
	area_id         dma_area;
	bool            dmainuse;

	size_t          last_size;
	size_t          last_to_device;
} dma_info_t;

status_t create_dma_area(dma_info_t *info);
status_t delete_dma_area(dma_info_t *info);
status_t start_dma(dma_info_t *info, size_t size, bool to_device);
status_t abort_dma(dma_info_t *info);
status_t finish_dma(dma_info_t *info);

//long make_isa_dma_table_vec(const iovec *vec, size_t veccount,
//                            uint32 startbyte, uint32 blocksize,
//                            long size, ulong nbit, isa_dma_entry *l, long nlist);

#endif

