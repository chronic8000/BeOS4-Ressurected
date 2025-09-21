#include "cfs_compress.h"
#include "cfs_debug.h"
#include "zlib.h"

void 
cfs_compress(const void *src_buffer, size_t src_len,
             void *dest_buffer, size_t *dest_len_ptr)
{
	*dest_len_ptr = src_len-1;
	if(compress(dest_buffer, dest_len_ptr, src_buffer, src_len) != Z_OK) {
		memcpy(dest_buffer, src_buffer, src_len);
		*dest_len_ptr = src_len;
	}
}

status_t 
cfs_decompress(const void *src_buffer, size_t src_len,
               void *dest_buffer, size_t dest_len)
{
	size_t uncompressed_size = dest_len;
	if(src_len > dest_len) {
		PRINT_ERROR(("cfs_decompress: compressed size %ld is bigger than "
		             "uncompressed size %ld\n", src_len, dest_len));
	}
	if(src_len == dest_len) {
		memcpy(dest_buffer, src_buffer, src_len);
		return B_NO_ERROR;
	}
	if(uncompress(dest_buffer, &uncompressed_size, src_buffer, src_len) != Z_OK) {
		PRINT_ERROR(("cfs_decompress: could not uncompress data\n"));
		return B_ERROR;
	}
	if(uncompressed_size != dest_len) {
		PRINT_ERROR(("cfs_decompress: could not uncompress all data\n"));
		return B_ERROR;
	}
	return B_NO_ERROR;
}

