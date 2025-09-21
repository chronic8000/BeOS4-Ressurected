#include "stream.h"
#include "gaudio.h"

int16 playback_streamID = GAME_MAKE_STREAM_ID(0),	/* reserved stream id's for playback */
      capture_streamID = GAME_MAKE_STREAM_ID(1);	/* and capture */


/* initialize data structures */
void stream_init(void)
{
	playback_stream.streamID = GAME_NO_ID;	/* designate as not open */
	playback_stream.pending_close = false;
	capture_stream.streamID = GAME_NO_ID;	/* diddo */
	capture_stream.pending_close = false;
	/* other fields set when stream is opened */
}

/* clean up allocated memory, reset data structures */
void stream_uninit(void)
{
	bufferID_node *save_bufferID_node;
	audiobuffer_node *buffer_node_temp;
	               
	lock();

	/* free all bufferID nodes associated with playback stream */
	while (playback_stream.buffer_list)
	{
		save_bufferID_node = playback_stream.buffer_list;
		playback_stream.buffer_list = playback_stream.buffer_list->next;
		free_bufferID_node(save_bufferID_node);
	}
	
	/* free all bufferID nodes associated with capture stream */
	while (capture_stream.buffer_list)
	{
		save_bufferID_node = capture_stream.buffer_list;
		capture_stream.buffer_list = capture_stream.buffer_list->next;
		free_bufferID_node(save_bufferID_node);
	}

	/* free all buffer nodes associated with playback stream */
	while (playback_stream.queue.head) 
	{
		buffer_node_temp = playback_stream.queue.head;
		playback_stream.queue.head = playback_stream.queue.head->next;
		free_node(buffer_node_temp);
	}

	/* free all buffer nodes associated with capture stream */
	while (capture_stream.queue.head) 
	{
		buffer_node_temp = capture_stream.queue.head;
		capture_stream.queue.head = capture_stream.queue.head->next;
		free_node(buffer_node_temp);
	}
	
	/* reset streams to blank by designating as closed */
	playback_stream.streamID = GAME_NO_ID;	
	playback_stream.pending_close = false;
	capture_stream.streamID = GAME_NO_ID;
	capture_stream.pending_close = false;	

	unlock();
}

/* request a new stream, allocate resources */
stream *make_stream(flow_dir flow_type)
{
	sem_id temp_sem;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("make_stream: Failure! called without protection\n"));
		return NULL;
	}

	/* already in use error */
	if ( (flow_type == dac && (playback_stream.streamID != GAME_NO_ID || playback_stream.pending_close)) ||
	     (flow_type == adc && (capture_stream.streamID != GAME_NO_ID || capture_stream.pending_close)) )	
	{
		unlock();
		DB_PRINTF(("make_stream: Failure! stream already in use\n"));
		lock();
		return NULL;
	}
	
	/* create a semaphore for stream closure */
	unlock();
	if ( (temp_sem = create_sem(0, "stream close sem")) < B_NO_ERROR ) 
	{
		DB_PRINTF(("make_stream: Failure! unable to create semaphore\n"));
		lock();
		return NULL;
	}
	lock();	

	/* set opening state */
	switch (flow_type) 
	{
		case dac:
			playback_stream.streamID = playback_streamID;
			playback_stream.queue.head = NULL;
			playback_stream.queue.tail = NULL;
			playback_stream.state = GAME_STREAM_STOPPED;
			playback_stream.stream_sem = -1;
			playback_stream.stream_close_sem = temp_sem;
			playback_stream.pending_close = false;
			playback_stream.buffer_list = NULL;
			return &playback_stream;
		case adc:
			capture_stream.streamID = capture_streamID;
			capture_stream.queue.head = NULL;
			capture_stream.queue.tail = NULL;
			capture_stream.state = GAME_STREAM_STOPPED;
			capture_stream.stream_sem = -1;
			capture_stream.stream_close_sem = temp_sem;
			capture_stream.pending_close = false;
			capture_stream.buffer_list = NULL;
			return &capture_stream;
	}
	
	return NULL;
}

/* close stream, deallocate resources, reset data structure. 
   should not relinquish lock until necessary changes made unless there is an error.
   if on first call there are still entries queued, will tell caller to acquire the
   close block by setting it's value to the stream closure semaphore.  Caller is then
   intended to call close stream again when it becomes unblocked.
*/
status_t close_stream(gaudio_dev *gaudio, stream *target_stream, uint16 flags)
{
	sem_id save_stream_sem,
	       save_close_sem;
	audiobuffer *delete_list = NULL;
	SGD *target_SGD;
		
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("close_stream: Failure! called without protection\n"));
		return B_ERROR;
	}
	else if (!target_stream || target_stream->streamID == GAME_NO_ID) 
	{
		unlock();
		DB_PRINTF(("close_stream: Failure! invalid parameters\n"));
		lock();
		return B_BAD_VALUE;
	}

	/* when free_buffer_node sees this flag it will perform necessary cleanup chores. 
	   cannot reopen stream until pending_close is lowered.
	   this flag also serves to forbid certain stream operations during closure.
	*/
	target_stream->pending_close = true;
	
	target_SGD = get_SGD(target_stream->streamID);

	/* note: important to release stream semaphore to simulate buffer played. */
	if (flags & GAME_CLOSE_FLUSH_DATA)
	{
		int index;

		/* empty the stream queue */
		while(target_stream->queue.head)
		{
			audiobuffer_node *save_buffer_node = target_stream->queue.head;
			target_stream->queue.head = target_stream->queue.head->next;

			/* release stream semaphore if necessary */
			if (save_buffer_node->flags & GAME_RELEASE_SEM_WHEN_DONE)
				release_sem_etc(save_buffer_node->in_stream->stream_sem, 1, B_DO_NOT_RESCHEDULE);	

			free_buffer_node(save_buffer_node);
		}
		target_stream->queue.tail = NULL;
	
		/* terminate and empty the SGD */
		target_SGD->state.SGD_paused = 0;
		target_SGD->state.SGD_stopped = 0;
		target_SGD->state.SGD_active = 0;

		if (target_SGD == &playback_SGD)
		{
			unlock();
			Stop_Playback_Pcm(gaudio);
			lock();
		}
		else if (target_SGD == &capture_SGD)
		{
			unlock();
			Stop_Capture_Pcm(gaudio);
			lock();
		}

		for (index = 0; index < NSLOTS; index++)
		{
			audiobuffer_node *save_node = target_SGD->nodes_in_SGD[index];
			
			if (!save_node)
				continue;

			target_SGD->nodes_in_SGD[index] = NULL;
			
			/* release stream semaphore if necessary */
			if (save_node->flags & GAME_RELEASE_SEM_WHEN_DONE)
				release_sem_etc(save_node->in_stream->stream_sem, 1, B_DO_NOT_RESCHEDULE);	

			free_node(save_node);
		}
		target_SGD->size = 0;
		target_SGD->readpoint = 0;
	}
	
	/* if there is still data in SGD and requested to block until complete, 
	   then tell caller to acquire stream closure semaphore */
	if ( (flags & GAME_CLOSE_BLOCK_UNTIL_COMPLETE) && (target_SGD && target_SGD->size || target_stream->queue.head)) 
	{
		unlock();
		acquire_sem(target_stream->stream_close_sem);
		lock();
	}
	
	save_close_sem = target_stream->stream_close_sem;
	save_stream_sem = target_stream->stream_sem;
	target_stream->stream_close_sem = -1;
	target_stream->stream_sem = -1;

    /* delete the bufferID nodes and stack audiobuffers for removal after unlock */
	while (target_stream->buffer_list) 
	{	    
		bufferID_node *save_bufferID_node = target_stream->buffer_list;
		int16 temp_buffer_ordinal;
	
		target_stream->buffer_list = target_stream->buffer_list->next;
	
		/* stack buffer to be deleted later */
   		audiobuffers[temp_buffer_ordinal = GAME_BUFFER_ORDINAL(save_bufferID_node->bufferID)]->next = delete_list;
		delete_list = audiobuffers[temp_buffer_ordinal];
		audiobuffers[temp_buffer_ordinal] = NULL;
					
		free_bufferID_node(save_bufferID_node);
	}

	target_stream->pending_close = false;
	target_stream->streamID = GAME_NO_ID;
	
	unlock();
	
	/* now close the buffers */
	while (delete_list)
	{
		audiobuffer *save_buffer = delete_list;
		delete_list = delete_list->next;
		delete_area(save_buffer->areaID);
		DB_PRINTF(("close_stream: deleting buffer %lx\n", save_buffer->bufferID));
		lock();
		free_buffer_entry(save_buffer);
		unlock();
	}
	
	/* delete the stream semaphore if necessary */
	/* should be done at end of function? */
	if ( (save_stream_sem >= B_NO_ERROR) && (flags & GAME_CLOSE_DELETE_SEM_WHEN_DONE) ) 
	{
		DB_PRINTF(("close_stream: deleting stream sem\n"));
		set_sem_owner(save_stream_sem, B_SYSTEM_TEAM);
		delete_sem(save_stream_sem);
	}

	/* stream close blocking semaphore needs to be deleted 
       this is not the same as the stream semaphore */
	delete_sem(save_close_sem);

	DB_PRINTF(("close_stream: reached completion\n"));
	lock();
	return B_OK;
}

/* find and remove and free given bufferID from stream */
status_t remove_bufferID_from_stream(int16 bufferID)
{
	stream *target_stream;
	bufferID_node *scan,
	              *tracker;
	int16 buffer_ordinal = GAME_BUFFER_ORDINAL(bufferID);
	              
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("remove_bufferID_from_stream: Failure! called without protection\n"));
		return B_ERROR;
	}
	else if ( !GAME_IS_BUFFER(bufferID) || !audiobuffers[buffer_ordinal] ||
	          !(target_stream = get_stream(audiobuffers[buffer_ordinal]->streamID)) )	
	{
		unlock();
		DB_PRINTF(("remove_bufferID_from_stream: Failure! invalid parameters\n"));
		lock();
		return B_ERROR;
	}
	
	/* find the given bufferID */
	tracker = NULL;
	scan = target_stream->buffer_list;
	while ( scan && (scan->bufferID != bufferID) )
	{
		tracker = scan;
		scan = scan->next;
	}

	if (!scan)
	{	/* not found */
		return B_ERROR;
	}
	else if (!tracker)
	{	/* is first element */
		target_stream->buffer_list = target_stream->buffer_list->next;
		free_bufferID_node(scan);
		return B_OK;
	}
	
	/* middle of list */
	tracker->next = scan->next;
	free_bufferID_node(scan);
	return B_OK;	
}

/* given a stream ID, return the corresponding stream structure or NULL if invalid */
stream *get_stream(int16 streamID)
{
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("get_stream: Failure! called without protection\n"));
		return NULL;
	}
	else if ( !GAME_IS_STREAM(streamID) )
		return NULL;
	else if ( streamID == playback_stream.streamID )
		return &playback_stream;
	else if ( streamID == capture_stream.streamID )
		return &capture_stream;
	
	return NULL;
}
