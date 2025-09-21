/************************************************************************/
/*                                                                      */
/*                            mem_manage.c 	                            */
/*                      												*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include "mem_manage.h"

/* free stacks for various items to be allocated, start empty */
static audiobuffer *free_buffer_list = NULL;
static bufferID_node *free_bufferID_node_list = NULL;
static audiobuffer_node *free_buffer_node_list = NULL;

/* release all memory, should be called last of uninit functions */
void mem_uninit(void)
{
	audiobuffer *save_buffer;
	bufferID_node *save_bufferID_node;
	audiobuffer_node *save_node;
	
	lock();

	/* release all buffer entries */
	while (free_buffer_list)
	{
		save_buffer = free_buffer_list;
		free_buffer_list = free_buffer_list->next;
		unlock();
		free(save_buffer);
		lock();
	}
	free_buffer_list = NULL;

	/* release all bufferID nodes */
	while (free_bufferID_node_list)
	{
		save_bufferID_node = free_bufferID_node_list;
		free_bufferID_node_list = free_bufferID_node_list->next;
		unlock();
		free(save_bufferID_node);
		lock();
	}
	free_bufferID_node_list = NULL;

	/* release all nodes */
	while (free_buffer_node_list)
	{
		save_node = free_buffer_node_list;
		free_buffer_node_list = free_buffer_node_list->next;
		unlock();
		free(save_node);
		lock();
	}
	free_buffer_node_list = NULL;

    unlock();
}

/* allocate memory for a buffer entry.
   if no nodes in free_buffer_node_list, malloc a new one. */
audiobuffer *alloc_buffer_entry(void)
{
	audiobuffer *new_buffer_entry;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("alloc_buffer_entry: Failure! called without protection\n"));
		return NULL;
	}

	/* in case nothing to recycle, have to malloc a new entry */
	if ( !free_buffer_list )
	{
		unlock();
		new_buffer_entry = (audiobuffer *) malloc( sizeof(audiobuffer) );
		new_buffer_entry->next = NULL;
		lock();
		return new_buffer_entry;
	}
	
	/* otherwise take the first entry off the free stack */
	new_buffer_entry = free_buffer_list;
	free_buffer_list = free_buffer_list->next;
	new_buffer_entry->next = NULL;
	return new_buffer_entry;
}

/* recycle the entry to the free list for future use */
void free_buffer_entry(audiobuffer *entry)
{
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("free_buffer_entry: Failure! called without protection\n"));
		return ;
	}
	else if ( !entry )
		return;
		
	/* push onto free stack */
	entry->next = free_buffer_list;
	free_buffer_list = entry;
}

/* allocate memory for a bufferID node.
   if no nodes in free_bufferID_node_list, malloc a new one. */
bufferID_node *alloc_bufferID_node(void)
{
	bufferID_node *new_bufferID_node;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("alloc_bufferID_node: Failure! called without protection\n"));
		return NULL;
	}

	/* in case nothing to recycle, have to malloc a new bufferID node */
	if ( !free_bufferID_node_list )
	{
		unlock();
		new_bufferID_node = (bufferID_node *) malloc( sizeof(bufferID_node) );
		new_bufferID_node->next = NULL;
		lock();
		return new_bufferID_node;
	}
	
	/* otherwise take the first entry off the free stack */
	new_bufferID_node = free_bufferID_node_list;
	free_bufferID_node_list = free_bufferID_node_list->next;
	new_bufferID_node->next = NULL; 
	return new_bufferID_node;
}

/* recycle the entry to the free list for future use */
void free_bufferID_node(bufferID_node *node)
{
	bufferID_node *bufferID_scan;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("free_bufferID_node: Failure! called without protection\n"));
		return ;
	}
	else if ( !node )
		return;
			
	/* push onto free stack */
	node->next = free_bufferID_node_list;
	free_bufferID_node_list = node;
}

/* allocate memory for a buffer node.
   if no nodes in free_buffer_node_list, malloc a new one. */
audiobuffer_node *alloc_buffer_node(void)
{
	audiobuffer_node *new_node;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("alloc_buffer_node: Failure! called without protection\n"));
		return NULL;
	}

	/* in case nothing to recycle, have to malloc a new node */
	if ( !free_buffer_node_list )
	{
		unlock();
		new_node = (audiobuffer_node *) malloc( sizeof(audiobuffer_node) );
		new_node->next = NULL;
		lock();
		return new_node;
	}
	
	/* otherwise take the first entry off the free stack */
	new_node = free_buffer_node_list;
	free_buffer_node_list = free_buffer_node_list->next;
	new_node->next = NULL;
	return new_node;
}	

/* recycle the node to the free list for future use */
void free_node(audiobuffer_node *node)
{
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("free_node: Failure! called without protection\n"));
		return ;
	}
	else if ( !node )
		return;
		
	/* push onto free stack */
	node->next = free_buffer_node_list;
	free_buffer_node_list = node;
}

