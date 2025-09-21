#include "SGD.h"

uint32 noise_buffer;			/* to fill empty SGD slots */
area_id noise_area;				/* areaID of the noise buffer */

/* initialize data structures + create and write SGD's to hardware */
status_t init_SGD(sound_card_t *card)
{
	int index;
	uint32 size;
	physical_entry where_playback,
	               where_capture;
		
	if (!card) 
	{
		DB_PRINTF(("init_SGD_table: Failure! parameter 'card' is NULL\n"));
		return B_ERROR;
	}
	
	/* set noise buffer */
	if ( !(noise_buffer = create_noise_buffer()) )
		return B_ERROR;
		
	/* set the size of the SGD's rounded to multiple of page size */
	size = NSLOTS * sizeof(SGD_Table_t);
	size = (size + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);

	if ( (playback_SGD.SGDTableAreaId = create_area(
		"sgd area", (void *)&playback_SGD.start, B_ANY_KERNEL_ADDRESS, 
		size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		DB_PRINTF(("init_SGD_table: Failure! could not create playback SGD area\n"));
		delete_area(noise_area);
		return B_ERROR;
	}

	if ( (capture_SGD.SGDTableAreaId = create_area(
		"sgd area", (void *)&capture_SGD.start, B_ANY_KERNEL_ADDRESS, 
		size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		DB_PRINTF(("init_SGD_table: Failure! could not create capture SGD area\n"));
		delete_area(playback_SGD.SGDTableAreaId);
		delete_area(noise_area);
		return B_ERROR;
	}

	if (get_memory_map(playback_SGD.start, size, &where_playback, 1) < 0) 
	{
		delete_area(playback_SGD.SGDTableAreaId);
		delete_area(capture_SGD.SGDTableAreaId);
		delete_area(noise_area);		
		DB_PRINTF(("init_SGD_table: Failure! failed to obtain physical mapping for playback SGD\n"));
		playback_SGD.start = NULL;
		return B_ERROR;
	}
		
	if (get_memory_map(capture_SGD.start, size, &where_capture, 1) < 0) 
	{
		delete_area(playback_SGD.SGDTableAreaId);
		delete_area(capture_SGD.SGDTableAreaId);
		delete_area(noise_area);		
		DB_PRINTF(("init_SGD_table: Failure! failed to obtain physical mapping for capture SGD\n"));
		capture_SGD.start = NULL;
		return B_ERROR;
	}
				
	(*card->func->GDT_playback_setup)(card, (uint32) where_playback.address);
	(*card->func->GDT_capture_setup)(card, (uint32) where_capture.address);
		
	playback_SGD.size = 0;
	playback_SGD.readpoint = 0;
	playback_SGD.state.SGD_active = 0;
	playback_SGD.state.SGD_stopped = 0;
	playback_SGD.state.SGD_paused = 0;
	capture_SGD.size = 0;
	capture_SGD.readpoint = 0;
	capture_SGD.state.SGD_active = 0;
	capture_SGD.state.SGD_stopped = 0;
	capture_SGD.state.SGD_paused = 0;

	/* set SGD table to known initial state */
	for (index = 0; index < NSLOTS; index++) 
	{
		/* these entries should never be played */
		playback_SGD.start[index].BaseAddr = noise_buffer; 
		playback_SGD.start[index].count = B_PAGE_SIZE;   
		playback_SGD.start[index].stop = 1;
		playback_SGD.start[index].flag = 1;
		playback_SGD.start[index].eol = 0;
		playback_SGD.nodes_in_SGD[index] = NULL;
		capture_SGD.start[index].BaseAddr = noise_buffer; 
		capture_SGD.start[index].count = B_PAGE_SIZE;   
		capture_SGD.start[index].stop = 1;
		capture_SGD.start[index].flag = 1;
		capture_SGD.start[index].eol = 0;
		capture_SGD.nodes_in_SGD[index] = NULL;
		buffers_of_DAC[index] = 0;
		buffers_of_ADC[index] = 0;
	}		
	playback_SGD.start[NSLOTS - 1].eol = 1;
	capture_SGD.start[NSLOTS - 1].eol = 1;

	return B_OK;
}

/* reset data structures and release allocated memory */
void uninit_SGD(void)
{
	int index;

	/* free buffer nodes in both SGD's */
	lock();
	for (index = 0; index < NSLOTS; index++) 
	{
		free_node(playback_SGD.nodes_in_SGD[index]);
		free_node(capture_SGD.nodes_in_SGD[index]);
	}
	unlock();

	delete_area(playback_SGD.SGDTableAreaId);
	delete_area(capture_SGD.SGDTableAreaId);
	delete_area(noise_area);
}

/* on initial call setup noise buffer, called through init_SGD */
uint32 create_noise_buffer(void)
{
	physical_entry where;
	void *addr;
		
	if ( (noise_area = create_area("noise area", (void *)&addr, B_ANY_KERNEL_ADDRESS, 
				B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0) 
	{
		DB_PRINTF(("create_noise_buffer: failed to allocate noise buffer\n"));
		return (uint32) NULL;
	}

	/* fill noise buffer with noise */
	memset(addr, 0x00, B_PAGE_SIZE);

	if (get_memory_map(addr, B_PAGE_SIZE, &where, 1) < 0) 
	{
		delete_area(noise_area);
		DB_PRINTF(("create_noise_buffer: failed to allocate noise buffer\n"));
		return (uint32) NULL;
	}
	return (uint32) where.address;
}

/* fill the SGD table from the specified queue as much as possible */
status_t fill_table_from_queue(int16 streamID)
{
	audiobuffer_node *loadentry;
	stream *target_stream = get_stream(streamID);
	SGD *target_SGD = get_SGD(streamID);
	int16 *target_buffer_table = get_buffer_table(streamID);
		
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("fill_table_from_queue: Failure! called without protection\n"));
		return B_ERROR;
	}
	else if (!target_stream) 
	{
		unlock();
		DB_PRINTF(("fill_table_from_queue: Failure! invalid streamID\n"));
		lock();
		return B_ERROR;
	} 
	
	while (target_SGD->size < NSLOTS) 
	{ 
	    int load_point, prev_point; 

		if ( !(loadentry = dequeue_head(target_stream)) )
			break;
         
        load_point =  (target_SGD->readpoint + target_SGD->size) % NSLOTS;
		prev_point = (target_SGD->readpoint + target_SGD->size - 1) % NSLOTS;
		
		if (prev_point < 0)
			prev_point = NSLOTS - 1;
		
		/* if previous entry is a real buffer with stop flag, remove that stop flag */
		/* and place a stop flag on the entry currently being added */
		if (target_buffer_table[prev_point])
			target_SGD->start[prev_point].stop = 0;
	
		/* debugging purposes, keep track of bufferID's in SGD */
		target_buffer_table[load_point] = loadentry->entry.bufferID;

		/* the buffer added here is now the last buffer in the system so set the stop flag */
		target_SGD->start[load_point].BaseAddr = (uint32) loadentry->entry.phys_addr;
		target_SGD->start[load_point].count = loadentry->entry.size;
		target_SGD->start[load_point].stop = 1;
		target_SGD->start[load_point].flag = 1;
		target_SGD->nodes_in_SGD[load_point] = loadentry;
		target_SGD->size++;
	}
}

/* intended to be used to erase last played buffer (on interrupt) */
/* returns the stop status of the cleared entry */
/* and fills buffer info into cleared_entry_info */
bool clear_current_entry(flow_dir flow_type, audiobuffer_node *cleared_entry_info)
{
	bool stop_status;
	stream *target_stream = NULL;
	SGD *target_SGD = NULL;
	int16 *target_buffer_table = NULL;
	audiobuffer_node *save_buffer_node;
	unsigned char save_readpoint;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("clear_current_entry: Failure! called without protection\n"));
		return false;
	}

	if (flow_type == dac) 
	{
		target_stream = &playback_stream;
		target_SGD = &playback_SGD;
		target_buffer_table = buffers_of_DAC;
	} 
	else if (flow_type == adc) 
	{
		target_stream = &capture_stream;
		target_SGD = &capture_SGD;
		target_buffer_table = buffers_of_ADC;
	}		
	
	if ( !target_stream || !cleared_entry_info ) 
	{
		unlock();
		DB_PRINTF(("clear_current_entry: Failure! invalid parameters\n"));
		lock();
		return B_ERROR;
	}
	
	save_readpoint = target_SGD->readpoint;
	
	/* precaution of clearing on empty, in practice, should not occur */
	if (!target_SGD->nodes_in_SGD[target_SGD->readpoint]) 
	{
		unlock();
		DB_PRINTF(("clear_current_entry: called but no valid buffer at readpoint\n"));
		lock();
		cleared_entry_info->entry.phys_addr = (void *) target_SGD->start[target_SGD->readpoint].BaseAddr;
		cleared_entry_info->entry.areaID = noise_area;
		cleared_entry_info->entry.size = target_SGD->start[target_SGD->readpoint].count;
		cleared_entry_info->entry.bufferID = GAME_NO_ID;
		cleared_entry_info->entry.streamID = GAME_NO_ID;
		cleared_entry_info->entry.enqueue_count = 0;
		cleared_entry_info->entry.next = NULL;
		cleared_entry_info->flags = 1;
		cleared_entry_info->in_stream = NULL;
		cleared_entry_info->next = NULL;
	}
	else
	{
		*cleared_entry_info = *(target_SGD->nodes_in_SGD[target_SGD->readpoint]);
		free_buffer_node(target_SGD->nodes_in_SGD[target_SGD->readpoint]);
	}
	
	/* store buffer and stop status before delete for return */
	stop_status = target_SGD->start[target_SGD->readpoint].stop;

	/* clear the SGD of the entry */
	target_SGD->start[target_SGD->readpoint].BaseAddr = noise_buffer;
	target_SGD->start[target_SGD->readpoint].count = B_PAGE_SIZE;
	target_SGD->start[target_SGD->readpoint].stop = 1;
	target_SGD->start[target_SGD->readpoint].flag = 1;

	target_SGD->nodes_in_SGD[target_SGD->readpoint] = NULL;	
	target_buffer_table[target_SGD->readpoint] = 0;
	return stop_status;
}

/* move the readpoint over */
status_t increment_readpoint(flow_dir flow_type)
{
	SGD *target_SGD = NULL;
	
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("increment_readpoint: Failure! called without protection\n"));
	}

	if (flow_type == dac)
		target_SGD = &playback_SGD;
	else if (flow_type == adc)
		target_SGD = &capture_SGD;

	target_SGD->readpoint++;
	target_SGD->readpoint %= NSLOTS;

	if ((target_SGD->size--) < 0) 
	{
		unlock();
		DB_PRINTF(("increment_readpoint: called when table size is 0\n"));
		lock();
		target_SGD->size = 0;
		target_SGD->readpoint = 0;
		return B_ERROR;
	}

	return B_OK;
}

/* return SGD corresponding to streamID or NULL if invalid */
SGD *get_SGD(int16 streamID)
{
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("get_SGD: Failure! called without protection\n"));
		return NULL;
	}
	else if ( !GAME_IS_STREAM(streamID) )
		return NULL;
	else if ( streamID == playback_stream.streamID )
		return &playback_SGD;
	else if ( streamID == capture_stream.streamID )
		return &capture_SGD;
	
	return NULL;
}

/* return buffer table corresponding to streamID or NULL if invalid */
int16 *get_buffer_table(int16 streamID)
{
	if ( !is_locked() ) 
	{	/* verify holding lock */
		DB_PRINTF(("get_buffer_table: Failure! called without protection\n"));
		return NULL;
	}
	else if ( !GAME_IS_STREAM(streamID) )
		return NULL;
	else if ( streamID == playback_stream.streamID )
		return buffers_of_DAC;
	else if ( streamID == capture_stream.streamID )
		return buffers_of_ADC;
	
	return NULL;
}

/* debugging purposes, display sgd table */
void print_SGD(flow_dir flow_type)
{
	int index;
	SGD *target_SGD = NULL;
	int16 *target_buffer_table = NULL;

	if (flow_type == dac) {
		target_SGD = &playback_SGD;
		target_buffer_table = buffers_of_DAC;
		DB_PRINTF(("DAC SGD Table:\n"));
	} else if (flow_type == adc) {
		target_SGD = &capture_SGD;
		target_buffer_table = buffers_of_ADC;
		DB_PRINTF(("ADC SGD Table:\n"));
	}
	
	if (!target_SGD) {
		DB_PRINTF(("print_SGD: Failure! unrecognized flow_dir\n"));
		return;
	}

	for (index = 0; index < NSLOTS; index++) {
		DB_PRINTF(("Slot %3d: BufferID: %lx, Stop Status=%d\n", 
			index, target_buffer_table[index], target_SGD->start[index].stop));
	}
	DB_PRINTF(("Readpoint %d\n", target_SGD->readpoint));
}
