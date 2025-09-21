#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "wrapper.h"

status_t add_buffer_to_queue(int16 streamID, int16 bufferID, uint16 flags);
status_t free_buffer_node(audiobuffer_node *node);
audiobuffer_node *dequeue_head(stream *target_stream);

#endif
