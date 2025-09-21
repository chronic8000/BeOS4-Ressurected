#ifndef CFS_COMPRESS_H
#define CFS_COMPRESS_H

#include <SupportDefs.h>

void cfs_compress(const void *src_buffer, size_t src_len,
                  void *dest_buffer, size_t *dest_len_ptr);
status_t cfs_decompress(const void *src_buffer, size_t src_len,
                        void *dest_buffer, size_t dest_len);

#endif

