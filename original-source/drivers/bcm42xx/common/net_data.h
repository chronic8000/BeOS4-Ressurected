#ifndef NET_BUF_H
#define NET_BUF_H

#include <iovec.h>
#include <string.h>

// ***
// Mem Pool Allocator
// ***

// Init and Free Memory Pool
status_t init_mempool( void );
void uninit_mempool( void );

// Allocate/Free Buffers
void *new_buf( size_t size ); // 64, 2048 bytes, or malloc( size )
void delete_buf( void *buffer );


#endif
