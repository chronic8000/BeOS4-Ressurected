/************************************************************************/
/*                                                                      */
/*                             queue.c 	                                */
/*                      												*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include "queue.h"

/* place the specified buffer into the specified stream queue */
status_t add_buffer_to_queue(int16 streamID, int16 bufferID, uint16 flags)
{	
	audiobuffer *target_buffer;
	audiobuffer_node *new_node;
	stream *target_stream;
	int16 bufferID_ordinal = GAME_BUFFER_ORDINAL(bufferID);
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("add_buffer_to_queue: Failure! called without protection\n"));
		return B_ERROR;
	}
	
	target_stream = get_stream(streamID);
	target_buffer = audiobuffers[bufferID_ordinal];
	
	if ( !target_stream || target_stream->streamID == GAME_NO_ID ||
	      target_stream->pending_close || !GAME_IS_BUFFER(bufferID) ||
	     !target_buffer ) 
	{	/* streamID or bufferID invalid for operation */
		unlock();
		DB_PRINTF(("add_buffer_to_stream: Failure! invalid parameters\n"));
		lock();
		return B_ERROR;
	} 
	else if ( (target_buffer->streamID != streamID) &&
	          (target_buffer->streamID != GAME_NO_ID) ) 
	{	/* buffer not allowed to be queued on specified stream */
		unlock();
		DB_PRINTF(("add_buffer_to_stream: Failure: illegal to add buffer to stream\n"));
		lock();
		return B_ERROR;		
	} 

	/* this call may give up the lock; this is acceptable at this stage */
	if ( !(new_node = alloc_buffer_node()) ) 
		return B_NO_MEMORY;

	/* increase enqueue count for the buffer */
	target_buffer->enqueue_count++;

	/* copy necessary info to the node */
	new_node->entry = *target_buffer;
	new_node->flags = flags;
	new_node->in_stream = target_stream;

	/* now insert on tail of stream queue */
	new_node->next = NULL;
	
	if (!target_stream->queue.head) 
	{
		target_stream->queue.head = new_node;
		target_stream->queue.tail = new_node;
		return B_OK;
	}
	
	target_stream->queue.tail->next = new_node;
	target_stream->queue.tail = new_node;
	return B_OK;
}

/* free specified node. 
   note: cleanup chores for stream closure are done here. 
   under normal operation should not give up the lock at any point. 
   can be called from interrupt handler, DON"T BLOCK! */
status_t free_buffer_node(audiobuffer_node *node)
{
	int16 bufferID_ordinal;
	SGD *target_SGD;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("free_buffer_node: Failure! called without protection\n"));
		return B_ERROR;
	}
	else if (!node) 
	{	/* normal use, no debug message */
		return B_ERROR;	
	} 
	
	/* release stream semaphore if necessary */
	if (node->flags & GAME_RELEASE_SEM_WHEN_DONE)
		release_sem_etc(node->in_stream->stream_sem, 1, B_DO_NOT_RESCHEDULE);	
	
	audiobuffers[bufferID_ordinal = GAME_BUFFER_ORDINAL(node->entry.bufferID)]->enqueue_count--;
	
	target_SGD = get_SGD(node->in_stream->streamID);

	/* if pending close and playing finish, release the stream closure semaphore */
	if ( node->in_stream->pending_close && !node->in_stream->queue.head &&
	     target_SGD && !target_SGD->size )
		release_sem_etc(node->in_stream->stream_close_sem, 1, B_DO_NOT_RESCHEDULE);	
	
	free_node(node);
	return B_OK;
}

/* remove and return next buffer from streams queue */
audiobuffer_node *dequeue_head(stream *target_stream)
{
	audiobuffer_node *result;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("dequeue_head: Failure! called without protection\n"));
		return NULL;
	}

	if (!target_stream) 
	{
		unlock();
		DB_PRINTF(("dequeue_head: Failure! stream does not exist\n"));
		lock();
		return NULL;
	} 
	else if (!target_stream->queue.head)	/* normal operation, no debug message */
		return NULL;
		
	result = target_stream->queue.head;		

	/* don't remove from list if looping node 
	   in this way, dequeue_head will continuously fetch the looping buffer */
	if ( (result->flags & GAME_BUFFER_LOOPING) && !target_stream->pending_close ) 
		return result;
				
	/* move head over + empty list check */
	if ( !(target_stream->queue.head = target_stream->queue.head->next) )
		target_stream->queue.tail = NULL;

	result->next = NULL;
	return result;
}
