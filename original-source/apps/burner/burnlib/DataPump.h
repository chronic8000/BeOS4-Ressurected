#ifndef _DATA_PUMP_H
#define _DATA_PUMP_H

#include <OS.h>

struct DataBuffer 
{
	const DataBuffer *next;
	const uint32 size;
	const void *data;
	
	uint32 track;
	uint32 frame;
	uint32 count;	
	uint32 blocksize;
	uint32 done;
	
	DataBuffer(DataBuffer *_next, uint32 _size, void *_data) :
		next(_next), size(_size), data(_data) {}
};

class DataPump 
{
public:
	DataPump(uint32 fifosize, uint32 chunksize);
	virtual ~DataPump();

	status_t InitCheck();
	
	void Start(uint32 total_frames);
	void Stop();
	
	virtual status_t Fill(DataBuffer *db) = 0;
	virtual status_t Empty(DataBuffer *db) = 0;
	
	float PercentFifo(void) { return percent_fifo; }	
	float PercentWritten(void) { return percent_written; }	
	
private:
	static status_t _Reader(void *data);
	static status_t _Writer(void *data);

	status_t Reader();
	status_t Writer();
	
	void Release();
	
	thread_id reader_thread;
	thread_id writer_thread;
	port_id reader_port;
	port_id writer_port;
	
	uint32 buffer_size;
	uint32 buffer_count;
	void *data;
	
	uint32 total_frames;
	uint32 written_frames;
	
	volatile float percent_fifo;
	volatile float percent_written;
	
	DataBuffer *dblist;
};

#endif
