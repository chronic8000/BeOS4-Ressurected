#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#include <iovec.h>

/* functions implemented in block_io.c */

extern status_t init_scratch_buffer();
extern void uninit_scratch_buffer();
extern status_t driver_read(void *cookie, off_t pos, void *buf, size_t *len);
extern status_t driver_write(void *cookie, off_t pos, const void *buf, size_t *len);
extern status_t driver_readv(void *cookie, off_t pos, const iovec *vec,
                             size_t count, size_t *numBytes);
extern status_t driver_writev(void *cookie, off_t pos, const iovec *vec,
                              size_t count, size_t *numBytes);

/* driver specific functions */

extern uint32 get_capacity(void *cookie);
extern uint32 get_blocksize(void *cookie);
extern uint32 get_bad_alignment_mask(void *cookie);
extern uint32 get_max_transfer_size(void *cookie);
extern status_t read_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
                            const iovec *vec, size_t count, uint32 startbyte);
extern status_t write_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
                             const iovec *vec, size_t count, uint32 startbyte);

#endif
