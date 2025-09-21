#ifndef __STREAMS_H__
#define __STREAMS_H__

#include "wrapper.h"
#include "gaudio.h"

void stream_init(void);
void stream_uninit(void);
stream *make_stream(flow_dir flow_type);
status_t close_stream(gaudio_dev *gaudio, stream *target_stream, uint16 flags);
status_t remove_bufferID_from_stream(int16 bufferID);
stream *get_stream(int16 streamID);

#endif