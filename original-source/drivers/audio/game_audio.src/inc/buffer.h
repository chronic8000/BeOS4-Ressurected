#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "wrapper.h"

void buffer_init(void);
void buffer_uninit(void);
audiobuffer *make_buffer(stream *target_stream, size_t size);
status_t alloc_buffer(audiobuffer *target_buffer, size_t size);
status_t close_buffer(int16 bufferID);
int16 get_next_bufferID(void);

#endif