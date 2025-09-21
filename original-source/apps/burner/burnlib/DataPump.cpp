#include <stdio.h>
#include "DataPump.h"

#include <string.h>
#include <stdlib.h>

DataPump::DataPump(uint32 fifo_size, uint32 chunk_size)
{
	uint i;
	uint32 x;
		
	buffer_size = chunk_size;
	buffer_count = fifo_size / chunk_size;
	
	data = malloc(fifo_size);
	
	reader_thread = spawn_thread(DataPump::_Reader, "DataPump:Reader", 
								 B_URGENT_DISPLAY_PRIORITY + 1, this);
	writer_thread = spawn_thread(DataPump::_Writer, "DataPump:Writer", 
								 B_URGENT_DISPLAY_PRIORITY + 1, this);
	reader_port = create_port(buffer_count, "DataPump:ReaderQueue");
	writer_port = create_port(buffer_count, "DataPump:WriterQueue");

	x = (uint32) data;
	dblist = NULL;
	for(i = 0; i < buffer_count; i++){
		DataBuffer *db = new DataBuffer(dblist, chunk_size, (void *) x);
		dblist = db;
		x += chunk_size;
		write_port(reader_port, 0, &db, 4);
	}	
	
	percent_fifo = 0.0;
	percent_written = 0.0;
	
	total_frames = 0;
	written_frames = 0;
}

void 
DataPump::Release()
{
	const DataBuffer *db, *next;
	
	if(data) {
		free(data);
		data = NULL;
	}
	if(reader_port >= 0){
		delete_port(reader_port);
		reader_port = -1;
	}
	if(writer_port >= 0){
		delete_port(writer_port);
		writer_port = -1;
	}
	
	for(db = dblist; db; db = next){
		next = db->next;
		delete db;
	}
	dblist = NULL;
}


DataPump::~DataPump()
{
	Release();
}

status_t 
DataPump::InitCheck()
{
	return data ? B_OK : B_ERROR;
}

void 
DataPump::Start(uint32 total_frames)
{
	this->total_frames = total_frames;
	
//	fprintf(stderr,"starting reader...\n");
	resume_thread(reader_thread);
	while(port_count(reader_port) && (percent_fifo < 0.999)) snooze(250000);
//	fprintf(stderr,"starting writer...\n");
	resume_thread(writer_thread);
}

void 
DataPump::Stop()
{
}

status_t 
DataPump::_Reader(void *data)
{
	return ((DataPump*) data)->Reader();
}

status_t 
DataPump::_Writer(void *data)
{
	return ((DataPump*) data)->Writer();
}

status_t
DataPump::Reader()
{
	DataBuffer *db;
	int32 code;
	uint32 count;
	uint32 done = 0;
	
//	fprintf(stderr,"DP:Reader()\n");
	while(!done){
		if(read_port(reader_port, &code, &db, 4) != 4) break;
		db->done = 0;
		
		if(Fill(db)) {
			break;
		} else {
			done = db->done;
			if(done){
				percent_fifo = 1.0;
			} else {
				count = port_count(reader_port);
				percent_fifo = ((float) (buffer_count-count)) / ((float) buffer_count);
			}
		}
		if(write_port(writer_port, 0, &db, 4)) break;
	}
//	fprintf(stderr,"DP:Reader() done\n");		
	return 0;
}

status_t
DataPump::Writer()
{
	DataBuffer *db;
	int32 code;
	uint32 done = 0;
//	fprintf(stderr,"DP:Writer()\n");
	
	while(!done){
		if(read_port(writer_port, &code, &db, 4) != 4) break;
		if(Empty(db)) {
			break;
		} else {
			done = db->done;
			if(done){
				percent_written = 1.0;
			} else {
				written_frames += db->count;
				percent_written = ((float) written_frames) / ((float) total_frames);
			}
		}		
		if(write_port(reader_port, 0, &db, 4)) break;
	}			
//	fprintf(stderr,"DP:Writer() done\n");
	return 0;
}

