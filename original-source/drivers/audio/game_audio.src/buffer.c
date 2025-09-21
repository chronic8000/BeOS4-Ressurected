#include "buffer.h"

/* initialize data structures here */
void buffer_init(void)
{
	/* null-out the audiobuffer allocation table */
	memset(audiobuffers, 0, GAME_MAX_BUFFER_COUNT * sizeof(audiobuffer *));		
}

/* free all audio buffers */
void buffer_uninit(void)
{
	int index = 0;

	lock();

	while (index < GAME_MAX_BUFFER_COUNT) 
	{
		free_buffer_entry(audiobuffers[index]);
		audiobuffers[index++] = NULL;
	}

	unlock();
}

/* request a new buffer, find a free bufferID and allocate */
/* target_stream is NULL if requested buffer is to be "wildcard" */
audiobuffer *make_buffer(stream *target_stream, size_t size)
{
	int16 new_bufferID,
	      new_bufferID_ordinal;
	audiobuffer *new_audiobuffer,
	             new_buffer_area;
	bufferID_node *new_bufferID_node;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("make_buffer: Failure! called without protection\n"));
		return NULL;
	}
	else if ( target_stream && (target_stream->pending_close || target_stream->streamID == GAME_NO_ID) ) 
	{	/* stream in illegal state for buffer creation */
		return NULL;
	} 
	else if ( (new_bufferID = get_next_bufferID()) == GAME_NO_ID ) 
	{	/* verify a free bufferID was found */
		unlock();
		DB_PRINTF(("make_buffer: Unable to obtain free bufferID\n"));
		lock();
		return NULL;
	}
	
	/* reserve memory for the audiobuffer allocation table */
	if ( !(new_audiobuffer = alloc_buffer_entry()) ) 
	{
		unlock();
		DB_PRINTF(("make_buffer: Failure! alloc_buffer_entry returned NULL\n"));
		lock();
		return NULL;
	}

	/* reserve memory for new bufferID node */
	if ( !(new_bufferID_node = alloc_bufferID_node()) )
	{
		unlock();
		DB_PRINTF(("make_buffer: Failure! alloc_bufferID_node returned NULL\n"));
		lock();
		free_buffer_entry(new_audiobuffer);
		return NULL;
	}
	
	unlock();

	/* set aside area for buffer */
	if ( alloc_buffer(&new_buffer_area, size) < B_NO_ERROR ) 
	{
		DB_PRINTF(("make_buffer: Failure on alloc_buffer\n"));		
		lock();
		free_buffer_entry(new_audiobuffer);
		free_bufferID_node(new_bufferID_node);
		return NULL;	
	}

	/* at this point have obtained necessary memory, lock and finish */
	lock();
	
	/* assign allocated memory to table while inside lock */
	audiobuffers[new_bufferID_ordinal = GAME_BUFFER_ORDINAL(new_bufferID)] = new_audiobuffer;
		
	/* assign allocated buffer area */
	audiobuffers[new_bufferID_ordinal]->phys_addr = new_buffer_area.phys_addr;
	audiobuffers[new_bufferID_ordinal]->areaID = new_buffer_area.areaID;

	/* some default initialization values + set bufferID */
	audiobuffers[new_bufferID_ordinal]->size = size;
	audiobuffers[new_bufferID_ordinal]->bufferID = new_bufferID;
	audiobuffers[new_bufferID_ordinal]->enqueue_count = 0;
	audiobuffers[new_bufferID_ordinal]->next = NULL;
	
	/* set stream ID and add bufferID to stream if not wildcard buffer */
	if (target_stream)
	{
		audiobuffers[new_bufferID_ordinal]->streamID = target_stream->streamID;
		new_bufferID_node->bufferID = new_bufferID;
		new_bufferID_node->next = target_stream->buffer_list;
		target_stream->buffer_list = new_bufferID_node;
	}
	else
	/* wildcard buffer is not associated with a stream */
	{
		audiobuffers[new_bufferID_ordinal]->streamID = GAME_NO_ID;
		free_bufferID_node(new_bufferID_node);
	}
	
	return audiobuffers[new_bufferID_ordinal];
}

/* create requested physical space for audiobuffer */
status_t alloc_buffer(audiobuffer *target_buffer, size_t size)
{
	void *addr;
	physical_entry where;

	if (!target_buffer || size < 1024)
		return B_BAD_VALUE;
		
	/* create physical buffer, round size to multiple of page size */
	size = (size + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);		

	if ( (target_buffer->areaID = create_area("audiobuffer", &addr, 	
			B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < B_NO_ERROR) 
	{
		DB_PRINTF(("alloc_buffer: Unable to create area\n"));
		return B_NO_MEMORY;
	}

	/* get the physical address */
	if (get_memory_map(addr, size, &where, 1) < B_NO_ERROR) 
	{
		delete_area(target_buffer->areaID);
		DB_PRINTF(("alloc_buffer: Error on get_memory_map\n"));
		return B_NO_MEMORY;
	}
	
	target_buffer->phys_addr = where.address;
	return B_OK;
}

/* close buffer, free memory, remove association with stream */
status_t close_buffer(int16 bufferID)
{
	int16 bufferID_ordinal;
	audiobuffer *save_buffer;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("close_buffer: Failure! called without protection\n"));
		return B_ERROR;
	}
	else if (!GAME_IS_BUFFER(bufferID) || 
	         !audiobuffers[bufferID_ordinal = GAME_BUFFER_ORDINAL(bufferID)] || 
	          audiobuffers[bufferID_ordinal]->enqueue_count) 
	{	/* invalid bufferID */
		unlock();
		DB_PRINTF(("close_buffer: Failure! invalid bufferID\n"));
		lock();
		return B_ERROR;
	} 
	
	/* save, make changes now, perform forbidden functions outside of lock */
	save_buffer = audiobuffers[bufferID_ordinal];
	audiobuffers[bufferID_ordinal] = NULL;
	remove_bufferID_from_stream(save_buffer->bufferID);
	free_buffer_entry(save_buffer);
	
	unlock();	
	delete_area(save_buffer->areaID);
	lock();
	return B_OK;
}

/* find a free bufferID */
int16 get_next_bufferID(void)
{
	static int16 bufferID_count = 0;	/* to distribute bufferID's */
	int counter = 0;					/* to count out search */
	int16 new_bufferID;

	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("get_next_bufferID: Failure! called without protection\n"));
		return B_ERROR;
	}

	/* find a valid bufferID */ 
	while (audiobuffers[bufferID_count] && counter <= GAME_MAX_BUFFER_COUNT) 
	{
		bufferID_count = (bufferID_count + 1) % GAME_MAX_BUFFER_COUNT;
		counter++;
	} 

	/* if couldn't find a free ID */
	if (counter > GAME_MAX_BUFFER_COUNT)
		return GAME_NO_ID;
		
	new_bufferID = GAME_MAKE_BUFFER_ID(bufferID_count);
	bufferID_count = (bufferID_count + 1) % GAME_MAX_BUFFER_COUNT;
	return new_bufferID;
}
