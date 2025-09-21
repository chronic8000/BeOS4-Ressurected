#ifndef __MEM_MANAGE_H__
#define __MEM_MANAGE_H__

#include "wrapper.h"

audiobuffer *alloc_buffer_entry(void);
void free_buffer_entry(audiobuffer *entry);
bufferID_node *alloc_bufferID_node(void);
void free_bufferID_node(bufferID_node *node);
audiobuffer_node *alloc_buffer_node(void);		
void free_node(audiobuffer_node *node);
void mem_uninit(void);

#endif