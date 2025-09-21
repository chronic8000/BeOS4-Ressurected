/* includes */
#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>
#include <multi_audio.h>

#include "xpress.h"

extern struct _PRD_data prd_table_pb[];
extern struct _PRD_data prd_table_cap[];

/* forward looking statements protected */
/* by the safe harbor act.              */
status_t free_buffer( void *, void *);
inline void start_stream_dma( xpress_dev *, struct _stream_data * );
inline void stop_stream_dma( open_xpress * , struct _stream_data * , bool );
inline void remove_queue_item(xpress_dev *,struct _stream_data *, sem_id *, int16 *);
inline void load_prd( struct _stream_data * , queue_item *);


/* functions */
void
lock_queue(open_xpress * card)
{
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(card->device->spinlock));
	card->device->cp = cp;
}

void
unlock_queue(open_xpress * card)
{
	cpu_status cp = card->device->cp;
	release_spinlock(&(card->device->spinlock));
	restore_interrupts(cp);
}


status_t
lock_stream(open_xpress * card)
{
	if (atomic_add(&((xpress_dev *)card->device)->stream_ben, 1) > 0) {
		return acquire_sem(((xpress_dev *)card->device)->stream_sem);
	}	
	else {
		return B_OK;
	}		
}
void
unlock_stream(open_xpress * card)
{
	if (atomic_add(&((xpress_dev *)card->device)->stream_ben, -1) > 1)
		release_sem_etc(((xpress_dev *)card->device)->stream_sem, 1, B_DO_NOT_RESCHEDULE);
}


#if DEBUG
void dump_queue(
				queue_item * q)
{
	while (q != NULL) {
		dprintf("dump_queue: current: %p next %p flags %lx idx %d buf %d in_prd %d\n",
				q, q->next, q->q_flags, q->phys_idx, q->buffer, q->in_prd);
		q = q->next;		
	}	
}

void dump_prd()
{
	int16 i;
	for (i = 0; i < PRD_DEPTH + 1; i++) {
		dprintf("dump_prd_pb: buff_phys_addr %p size %d flags %x\n",
				prd_table_pb[i].buff_phys_addr, prd_table_pb[i].size, prd_table_pb[i].flags);
		dprintf("dump_prd_cap: buff_phys_addr %p size %d flags %x\n",
				prd_table_cap[i].buff_phys_addr, prd_table_cap[i].size, prd_table_cap[i].flags);
	}	
}

void dump_entry( physical_entry * p, int16 x)
{
	while(x--) {
		dprintf("\tdump_pe: address %p size %ld\n",p->address, p->size);
		p++;
	}	
}

void dump_buffer( struct _buffer_data * b)
{
	dprintf("dump_buffer: pe %p aid %ld offset %ld size %ld stream %d entries %d refcnt %d\n",
				b->phys_entry, (uint32)b->area, b->offset, b->byte_size, b->stream, b->num_entries, b->ref_count);
	dump_entry(b->phys_entry, b->num_entries);			
}
#endif




status_t
allocate_stream(
			void * cookie,
			void * data)
{
	/* before calling this function, you first */
	/* validated the input data and locked the */
	/* resource list, right??                  */

	open_xpress * card = (open_xpress *)cookie;
	game_open_stream * pgos = (game_open_stream *) data;
	xpress_dev * pdev  = card->device;
	status_t err = B_OK;
		
	/* DAC */
	if (GAME_IS_DAC(pgos->adc_dac_id)){
		game_dac_info * pdac = &(pdev->dac_info[GAME_DAC_ORDINAL(pgos->adc_dac_id)]);
		/* stream available ? */
		ddprintf(("xpress: gos dac\n"));
		if (pdac->cur_stream_count >= pdac->max_stream_count) {
			pgos->out_stream_id = GAME_NO_ID;
			ddprintf(("xpress: gos dac -- out of streams\n"));
			err = B_ERROR;
		}
		else {
			/* can we support the property request? */		
			/* right now, we don't support anything */
			if ( pgos->request ) {
				/* something is unsupported */
				ddprintf(("xpress: stream support 0x%x requested, failing.\n",pgos->request));
				ddprintf(("xpress: I should fail you, but I can't until you change the snd_server code.\n"));
//				pgos->out_stream_id = GAME_NO_ID;
//				err = B_BAD_VALUE;
			}
//			else
			{
				int16 i;
				pdac->cur_stream_count++;
				for (i = 0; i < X_MAX_STREAMS; i++) {
					if (!(pdev->stream_data[i].my_flags & X_ALLOCATED)) break;
				}
				KASSERT((i != X_MAX_STREAMS));

				pgos->out_stream_id = GAME_MAKE_STREAM_ID(i);
				pdev->stream_data[i].my_flags |= X_ALLOCATED; /* or init to allocated + stopped */
				pdev->stream_data[i].gos_stream_sem = pgos->stream_sem;
				pdev->stream_data[i].gos_adc_dac_id = pgos->adc_dac_id;
				pdev->stream_data[i].prd_table = &prd_table_pb[0];
				pdev->stream_data[i].cookie = cookie;
				pdev->stream_data[i].frame_count = 0;
				pdev->stream_data[i].timestamp = 0LL;
				/* reset the semaphore */
				delete_sem(pdev->stream_data[i].all_done_sem);
				/* the 0 really should be ix */
				{
					char name[32];
					sprintf(name, "xpress %d done_sem%d", 0,i);
					pdev->stream_data[i].all_done_sem = create_sem(0, name);
				}
				MEM_WR32(pdev->f3 + 0x24, pdev->stream_data[i].prd_table[PRD_DEPTH].buff_phys_addr);
				err = B_OK;		
			}		
		}
	}
	else {
		game_adc_info * padc = &(pdev->adc_info[GAME_ADC_ORDINAL(pgos->adc_dac_id)]);
		/* stream available ? */
		ddprintf(("xpress: gos adc \n"));
		if (padc->cur_stream_count >= padc->max_stream_count) {
			pgos->out_stream_id = GAME_NO_ID;
			ddprintf(("xpress: gos adc -- out of streams\n"));
			err = B_ERROR;
		}
		else {
			/* can we support the property request? */		
			/* right now, we don't support anything */
			if ( pgos->request ) {
				/* something is unsupported */
				ddprintf(("xpress: stream support 0x%x requested, failing.\n",pgos->request));
				ddprintf(("xpress: YOU BOZO! This is handled by the codec!!\n"));
				ddprintf(("xpress: I should fail you, but I can't until you change the code!\n"));
//				pgos->out_stream_id = GAME_NO_ID;
//				err = B_BAD_VALUE;
			}
//			else
			{
				int16 i;
				padc->cur_stream_count++;
				for (i = 0; i < X_MAX_STREAMS; i++) {
					if (!(pdev->stream_data[i].my_flags & X_ALLOCATED)) break;
				}
				KASSERT((i != X_MAX_STREAMS));
				pgos->out_stream_id = GAME_MAKE_STREAM_ID(i);
				pdev->stream_data[i].my_flags |= X_ALLOCATED; /* or init to allocated + stopped */
				pdev->stream_data[i].gos_stream_sem = pgos->stream_sem;
				pdev->stream_data[i].gos_adc_dac_id = pgos->adc_dac_id;
				pdev->stream_data[i].prd_table = &prd_table_cap[0];
				pdev->stream_data[i].cookie = cookie;
				/* reset the semaphore */
				delete_sem(pdev->stream_data[i].all_done_sem);
				/* the 0 really should be ix */
				{
					char name[32];
					sprintf(name, "xpress %d done_sem%d", 0,i);
					pdev->stream_data[i].all_done_sem = create_sem(0, name);
				}
				MEM_WR32(pdev->f3 + 0x2C, pdev->stream_data[i].prd_table[PRD_DEPTH].buff_phys_addr);
				err = B_OK;		
			}		
		}
	}
	return err;
}

status_t
close_stream(
			void * cookie,
			void * data)
{
	/* THE RESOURCE LIST IS NOT LOCKED, RIGHT?    */

	/* this pathetic attempt at a close routine   */
	/* blocks until everything is freed.          */
	/* consequently, we don't want to wait with   */
	/* with the list locked                       */
	
	/* future versions should block ONLY if the   */
	/* blocking flag is set.                      */
	
	/* also, stream should have been validated previously */
	
	open_xpress * card = (open_xpress *)cookie;
	game_close_stream * pgcs = (game_close_stream *) data;
	struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgcs->stream)]);
	queue_item * pqi = NULL;
	queue_item * t_pqi = NULL;
	int16 i;
	status_t err = B_OK;
	sem_id release_sem[PRD_DEPTH];
	int16 release_me = 0;


	/* has the dma stopped? */
	/* stop the dma */
	lock_stream(card);	
		if (pgcs->flags & GAME_CLOSE_FLUSH_DATA){
			ddprintf(("FLUSHING data on close\n"));
			stop_stream_dma(card, psd, true);
		}
		else {
			ddprintf(("NOT flushing data on close\n"));	
			stop_stream_dma(card, psd, false);	
		}			
	unlock_stream(card);	
	
	/* free the buffers bound to the stream */
	/* free the queue containers (queue_items) */

	/* move the items from the buffer_queue to the free queue */
	/* make sure to decrement buffers as they are completed   */
	lock_stream(card);
		/* because the dma has stopped & closing is still set, I */
		/* really don't need the lock_queue                      */
		lock_queue(card);
		{
			pqi = psd->stream_buffer_queue;
			while (pqi != NULL) {		

				/* remove the queue item */
				remove_queue_item( card->device, psd, &release_sem[0], &release_me);
				if (release_me >= PRD_DEPTH) {
					/* doesn't matter here because close doesn't release sems */
					release_me = 0;
				}
				pqi = psd->stream_buffer_queue;
			}
			/* zero the registers */
			if (GAME_IS_DAC(psd->gos_adc_dac_id)) {
				/* clear pb register */
				MEM_WR8(card->device->f3 + 0x20, 0x0);
				i = MEM_RD8(card->device->f3 + 0x21);
			}
			else if (GAME_IS_ADC(psd->gos_adc_dac_id)) {
				/* clear cap register */
				MEM_WR8(card->device->f3 + 0x28, 0x0);
				i = MEM_RD8(card->device->f3 + 0x29);
			}
		}
		unlock_queue(card);
		/* we could release the sems here...  */
		/* but we won't because this is close */

		/* free all buffers associated with the stream (bound) */
		for (i=0; i < X_MAX_BUFFERS; i++) {
			if (card->device->buffer_data[i].stream == pgcs->stream) {
				game_close_stream_buffer gcsb;
				gcsb.info_size = sizeof(game_close_stream_buffer);
				gcsb.buffer = GAME_MAKE_BUFFER_ID(i);
				free_buffer((void *)card, (void *) &gcsb);
			}
		}
		/* free all queue items */
		pqi = psd->free_list;
		while (pqi != NULL) {
			t_pqi = pqi->next;
			free(pqi);
			pqi = t_pqi;
		}
		/* decrement the current stream count */
			if (GAME_IS_DAC(psd->gos_adc_dac_id)) {
				game_dac_info * pdac = &(card->device->dac_info[GAME_DAC_ORDINAL(psd->gos_adc_dac_id)]);
				pdac->cur_stream_count--;
				ddprintf(("xpress: decrement dac %d\n",psd->gos_adc_dac_id));
			}
			else if (GAME_IS_ADC(psd->gos_adc_dac_id)) {
				game_adc_info * padc = &(card->device->adc_info[GAME_ADC_ORDINAL(psd->gos_adc_dac_id)]);
				padc->cur_stream_count--;
				ddprintf(("xpress: decrement adc %d\n",psd->gos_adc_dac_id));
			}

	unlock_stream(card);	

	if (pgcs->flags & GAME_CLOSE_DELETE_SEM_WHEN_DONE) {
		/* make sure we are all done before releasing... */
		delete_sem(card->device->stream_data[GAME_STREAM_ORDINAL(pgcs->stream)].gos_stream_sem);
		card->device->stream_data[GAME_STREAM_ORDINAL(pgcs->stream)].gos_stream_sem = B_ERROR;
	}
	
	return err;
}

status_t
allocate_buffer(
			void * cookie,
			void * data,
			uint16 stream_buffer_idx)
{
	/* before calling this function, you first */
	/* locked the resource list, right??       */
	/* if calling from allocate_stream_buffer  */
	/* you validated the params, didn't you?   */
	
	open_xpress * card = (open_xpress *)cookie;
	game_open_stream_buffer * pgosb = (game_open_stream_buffer *) data;
	char aname[B_OS_NAME_LENGTH];
	area_id tmp_area = B_ERROR;
	void * tmp_buf = NULL;
	uint32 page_bytes = 0;
	physical_entry * tmp_phys = NULL;
	physical_entry * t_p = NULL;
	int16   i;
	uint16  x = 0;
	status_t err = B_OK;

	/* try to allocate the memory--we know a buffer is available */
	/* and our resource list is now locked......                 */
//	KASSERT((pgosb->byte_size >= X_MIN_BUFF_ALLOC));	
	sprintf(aname,"xstream:%.4x",pgosb->stream);
	page_bytes = ((((pgosb->byte_size)-1)/B_PAGE_SIZE) + 1)*B_PAGE_SIZE;
	tmp_area = create_area(
						aname,
						(void **)&tmp_buf,
						B_ANY_KERNEL_ADDRESS,
						page_bytes, 
						B_FULL_LOCK,
						0 /* B_READ_AREA | B_WRITE_AREA */);

	if (tmp_area >=B_OK) {
		/* do we have enough memory left over for the mapping??? */
		tmp_phys = (physical_entry *)malloc((page_bytes / B_PAGE_SIZE)*sizeof(physical_entry));
		if (tmp_phys != NULL) {
			/* we HAVE the memory */
			/* fill in the table  */
			get_memory_map(tmp_buf, page_bytes, tmp_phys, page_bytes/B_PAGE_SIZE);
			/* walk the chain and then realloc to save memory */
			t_p = tmp_phys;
			x   = 0;
			while ((t_p->size != 0) && (x < page_bytes/B_PAGE_SIZE)) {
				x++;
				t_p++;
			}
			KASSERT((x));
			t_p = realloc(tmp_phys, x * sizeof(physical_entry));
			if (t_p == NULL) {
				/* really weird, fail */
				free(tmp_phys);
				delete_area(tmp_area);
				err = ENOMEM; 
				dprintf("xpress: error mapping memory\n");
			}
		}
		else {
			/* not enough memory, so back off on */
			/* our request until can allocate it */
			/* THEN, we have to see if we have   */
			/* enough for the mapping. we do that*/
			/* by walking the map......          */
			/* do that in rev. 2. For now, just  */
			/* return enomem                     */
			delete_area(tmp_area);
			err = ENOMEM; 
			dprintf("xpress: error mapping memory\n");
		}
	}
	else {
	dprintf("xpress: create area failed on %ld bytes\n",page_bytes);
	 err = ENOMEM;
	}
	/* fill in structs.... */ 
	if (err == B_OK){
		struct _buffer_data * pbd = NULL;
		/* find the free buffer...it's the one with area_id == B_ERROR */
		for (i = 0; i < X_MAX_BUFFERS; i++) {
			if (card->device->buffer_data[i].area == B_ERROR) {
				pbd = &(card->device->buffer_data[i]);
				break;
			}	
		}
		KASSERT((pbd != NULL));	
		pgosb->area  = tmp_area;
		pgosb->offset= 0;
		pgosb->buffer= GAME_MAKE_BUFFER_ID(pbd->idx);

		pbd->stream = pgosb->stream;
		pbd->ref_count = 0; /* increment ref count in queue_buffer */
		pbd->area	= pgosb->area;
		pbd->offset = 0;
		pbd->byte_size = pgosb->byte_size;
		pbd->virt_addr = tmp_buf;

		pbd->phys_entry = t_p;
		pbd->num_entries= x;

		if (pgosb->stream != GAME_NO_ID) {
			struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgosb->stream)]);
			psd->bind_count++;		
		}

		card->device->current_buffer_count++;
	}
	else {
		pgosb->area  = B_ERROR;
		pgosb->offset= 0;
		pgosb->buffer= GAME_NO_ID;
	}
	
	ddprintf(("xpress: allocate buffer, stream: %x area: %lx\n",pgosb->stream, pgosb->area));	

	return err;
}

inline void
fill_pqi(
		queue_item *pqi,
		uint32 	 	flags,
		uint32 *	v_addr,
		void *   	p_addr,
		int16    	buffer,
		uint16   	size)
{
	pqi->q_flags 	= flags;
	pqi->virt_addr 	= v_addr;
	pqi->phys_addr 	= p_addr;
	// no phys_idx...
	pqi->buffer		= buffer;
	pqi->in_prd 	= -1;
	pqi->size		= size;		
}		 

inline void 
load_prd(
		struct _stream_data * pstream,
		queue_item * t_pqi)
{

	while ((pstream->prd_add < pstream->prd_remove + PRD_DEPTH - PRD_FREE_SLOTS) &&
			(t_pqi != NULL)) {

		while( (t_pqi != NULL) && (t_pqi->in_prd != -1) ) {
			t_pqi = t_pqi->next;
			
		}
		if (t_pqi !=  NULL) {
			t_pqi->in_prd = pstream->prd_add % PRD_DEPTH;

			pstream->prd_table[t_pqi->in_prd].buff_phys_addr = t_pqi->phys_addr; 
			pstream->prd_table[t_pqi->in_prd].size           = t_pqi->size;

			if (t_pqi->q_flags & SUPER_SECRET_CLEAR_EOP) {
				pstream->prd_table[t_pqi->in_prd].flags          = 0;
			}
			else {
				pstream->prd_table[t_pqi->in_prd].flags          = X_EOP;
			}
			pstream->prd_add++;
		}
	}
}

status_t
queue_buffer(
			void * cookie,
			void * data)
{
	/* before calling this function, you first */
	/* locked the resource list, right??       */
	open_xpress * card = (open_xpress *)cookie;
	game_queue_stream_buffer * pgqsb = (game_queue_stream_buffer *) data;

	struct _stream_data * pstream = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgqsb->stream)]);
	struct _buffer_data * pbuffer = &(card->device->buffer_data[GAME_BUFFER_ORDINAL(pgqsb->buffer)]);

	uint16 qi_needed  = 0;
	uint16 tqi_needed = 0;
	uint16 extra_qis  = 0;
	
	queue_item * pqi = NULL;
	queue_item * t_pqi = NULL;

	status_t err = B_OK;


	/* we have three cases:                                         */
	/* 1) the dreaded PING-PONG LOOPING buffer                      */
	/* 2) queueing a buffer with fewer than allocated bytes, and    */
	/* 3) queueing a buffer with allocated bytes.                   */	
	
	if (pgqsb->flags & (GAME_BUFFER_LOOPING | GAME_BUFFER_PING_PONG)) {
		/* split up the buffer into additional queue items if needed */
		extra_qis =  (pgqsb->frame_count / pgqsb->page_frame_count)- 1;
		ddprintf(("PING_PONG extra qis %d\n",extra_qis));	
	}

	/* Danger, you have hardcoded the conversion between */
	/* frames and bytes................................. */
	/* DANGER DANGER DANGER DANGER DANGER DANGER DANGER  */
	if ( pbuffer->byte_size  != (pgqsb->frame_count << 2)) {
		uint32 total = 0;
		physical_entry * pe = pbuffer->phys_entry;
		
		ddprintf(("Houston, bytes %ld fc * 4 %ld\n",pbuffer->byte_size, pgqsb->frame_count << 2));	
		/* it's not a full buffer, so we must walk the */
		/* list to know how many entries we need.      */
		/* DANGER DANGER DANGER DANGER DANGER DANGER DANGER  */
		while ((total < (pgqsb->frame_count << 2)) && (qi_needed < pbuffer->num_entries)) {
			qi_needed++;
			total += pe->size;
			pe++;
		}
	}
	else {
		qi_needed = pbuffer->num_entries;
	}
	
	qi_needed += extra_qis;
	tqi_needed = qi_needed;
	 
	/* check free list first */
	lock_queue(card);
		pqi = t_pqi = pstream->free_list;
		while (t_pqi && qi_needed) {
			tqi_needed--;
			if (tqi_needed == 0) {
				pstream->free_list = t_pqi->next;
				t_pqi->next = NULL;
				goto alldone;
			}
			t_pqi = t_pqi->next;
		}
		pstream->free_list = t_pqi;
alldone:
	unlock_queue(card);
	
	while (tqi_needed) {
		t_pqi = malloc(sizeof(struct queue_item));
		if ( t_pqi == NULL) {
			err = ENOMEM;
			/* free the rest of the list */
			while (pqi) {
				t_pqi = pqi->next;
				free(pqi);
				pqi = t_pqi;
			}
			break;
		}
		/* add to list head */
		t_pqi->next = pqi;
		pqi = t_pqi;
		tqi_needed--;	
	}
		
	if (err == B_OK) {
		int16 i;
		uint32 total = 0;
		physical_entry * pe = pbuffer->phys_entry;

		/* prepare queue items for insertion */
		t_pqi = pqi;

		/* split prd's if needed */
		{   uint16 page_frame_count = 0;
			uint16 pe_bytes_left	= pe->size;
			uint16 pfc_bytes_left 	= 0;
			uint16 size = 0;
			uint32 flags = pgqsb->flags;
			queue_item	* prev_pqi	= NULL;


			if (pgqsb->page_frame_count == 0 ) {
				/* DANGER DANGER */
				page_frame_count = pe->size >> 2;
			}
			else {
				page_frame_count = pgqsb->page_frame_count;
			}
			
			/* DANGER DANGER */
			pfc_bytes_left = page_frame_count << 2;

			for (i = 0; i < qi_needed; i++) {
				/* clear old flags */				
				flags = pgqsb->flags;			
			
				/* check to see if the page_frame_count left is greater than the pe size */
				if (pfc_bytes_left >= pe_bytes_left) {
					size = pe_bytes_left;
				}
				else {	
					size = pfc_bytes_left;	
				}	
				if (i == 0) {
					flags |= SUPER_SECRET_START_OF_BUFFER;
				}
				
				if ((pgqsb->frame_count << 2) == total + size) {
					flags |= SUPER_SECRET_END_OF_BUFFER;
				}
				
				if ((pfc_bytes_left == size) &&
					(pgqsb->flags & (GAME_BUFFER_LOOPING | GAME_BUFFER_PING_PONG)) ) {
					flags |= GAME_RELEASE_SEM_WHEN_DONE;
				}

				/* small buffers should be handled gracefully */
				if ((size < X_MIN_BUFF_ALLOC) && (prev_pqi)) {
					/* clear the EOP flag, should be set on next_buffer */
					prev_pqi->q_flags |= SUPER_SECRET_CLEAR_EOP;
				}
#if 0
				fill_pqi(	t_pqi,
							flags,
							(uint32 *)((uint32)(pbuffer->virt_addr) + total),
							(void *) ((uint32)(pe->address) + (pe->size - pe_bytes_left)),
							pgqsb->buffer,
							size);
#else
				t_pqi->q_flags 	= flags;
				t_pqi->virt_addr 	= (uint32 *)((uint32)(pbuffer->virt_addr) + total);
				t_pqi->phys_addr 	= (void *) ((uint32)(pe->address) + (pe->size - pe_bytes_left));
				// no phys_idx...
				t_pqi->buffer		= pgqsb->buffer;
				t_pqi->in_prd 	= -1;
				t_pqi->size		= size;		
#endif
				if (flags & SUPER_SECRET_END_OF_BUFFER) {
					break;
				}
				
				/* update the values */							
				pfc_bytes_left -= size;
				pe_bytes_left  -= size;
				
				total  += size;
				prev_pqi= t_pqi;
				t_pqi 	= t_pqi->next;

				if (!pe_bytes_left) {
					pe++;
					pe_bytes_left = pe->size;
				}
				if (!pfc_bytes_left) {
					pfc_bytes_left = page_frame_count << 2;
				}
			}
		}
		
		lock_queue(card);

			/* what if we have more queue items than entries?? */
			/* we need to make sure they get freed!!           */
			/* remove assert when the code following it works  */
//			KASSERT((qi_needed == pbuffer->num_entries));
							
			if ( (i != qi_needed) && (t_pqi->next != NULL) ) {
				/* free the extra queue items*/
				if (pstream->free_list != NULL) {
					queue_item * tt_pqi = pstream->free_list;
					
					while (tt_pqi->next != NULL) {
						tt_pqi = tt_pqi->next;
					}
					tt_pqi->next = t_pqi->next;		
				}
				else {
					pstream->free_list = t_pqi->next;
				}
				t_pqi->next = NULL;
			}


			/* insert into end of queue */
			t_pqi = pstream->stream_buffer_queue;
			if (t_pqi != NULL) {
				while (t_pqi->next != NULL) {
					t_pqi = t_pqi->next;
				}
				t_pqi->next = pqi;
			}
			else {
				pstream->stream_buffer_queue = pqi;				
			}

			/* this buffer has been inserted on a queue */
			/* increment the reference count            */
			pbuffer->ref_count++;			
			
			/* update the prds */
			if (pstream->state != GAME_STREAM_PAUSED)
			{
				load_prd( pstream, pstream->stream_buffer_queue);
			}			
			if ((pstream->my_flags & X_DMA_STOPPED) && pstream->state == GAME_STREAM_RUNNING) {
				/* adjust the PRD pointer... */
				/* if it stopped, we hit an unexpected (!closing) eot     */
				/* in the int handler. The int handler will restart the   */
				/* dma if there are data items queued. Therefore, we have */
				/* new data on the queue. Set the prd register to the     */
				/* first item on the queue.                               */

				t_pqi = pstream->stream_buffer_queue;
				IKASSERT((t_pqi != NULL));
				
				if (GAME_IS_DAC(pstream->gos_adc_dac_id)) {
					MEM_WR32(card->device->f3 + 0x24, ((uchar *)(pstream->prd_table[PRD_DEPTH].buff_phys_addr)) + (t_pqi->in_prd * sizeof(struct _PRD_data)));
				}
				else {
					MEM_WR32(card->device->f3 + 0x2c, ((uchar *)(pstream->prd_table[PRD_DEPTH].buff_phys_addr)) + (t_pqi->in_prd * sizeof(struct _PRD_data)));
				}	
//kprintf("@");
				start_stream_dma(card->device, pstream);
			}
		unlock_queue(card);
	}
	return err;
}

status_t
free_buffer(
			void * cookie,
			void * data)
{
	/* before calling this function, you first */
	/* locked the resource list, right??       */
	open_xpress * card = (open_xpress *)cookie;
	game_close_stream_buffer * pgcsb = (game_close_stream_buffer *) data;
	struct _buffer_data * pbd = &(card->device->buffer_data[GAME_BUFFER_ORDINAL(pgcsb->buffer)]);
	status_t err = B_OK;

	ddprintf(("xpress: free buffer stream:%x  area:%lx\n",pbd->stream, pbd->area));

	/* make sure buffers are not queued by checking the ref count                 */
	/* ref count can only be incremented if _both_ stream and queue are locked    */
	/* we have locked the stream, so ref_count can't be incremented in the middle */
	/* it could be decremented, but that's not a problem...                       */
	if (pbd->ref_count || pbd->area == B_ERROR ) {
		err = B_ERROR;
	}
	
	if (err == B_OK) {
		/* remove it from the buffer list... 			*/
		/* we do this by decrementing bound buffers, 	*/
		/* decrementing buffer_count, freeing the area,	*/
		/* and then setting the area id to B_ERROR      */
		if (pbd->stream != GAME_NO_ID) {
			struct _stream_data * psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pbd->stream)]);
			if (psd->bind_count) {
				psd->bind_count--;
			}
		}
		pbd->stream = GAME_NO_ID;
		/* ref count must be zero */
		delete_area(pbd->area);
		pbd->area       = B_ERROR;
		pbd->offset     = 0;
		pbd->byte_size  = 0;
		pbd->virt_addr	= NULL;
		pbd->num_entries= 0;
		pbd->phys_entry = NULL; /* mapping removed with delete_area */
		card->device->current_buffer_count--; /* should NOT go negative...   */
	}
	return err;
}

status_t
run_streams(
			void * cookie,
			void * data,
			int i)
{
	/* before calling this function, you first */
	/* locked the resource list, right??       */
	open_xpress * card = (open_xpress *)cookie;
	game_run_streams * pgrs = (game_run_streams *) data;
	struct _stream_data * psd = NULL;
	queue_item * t_pqi = NULL;
	sem_id release_sem[PRD_DEPTH];
	int16 release_me = 0;
	uint32 frame_cvsr = 0;
	status_t err = B_OK;
	
	if (GAME_IS_STREAM(pgrs->streams[i].stream) && (pgrs->streams[i].stream != GAME_NO_ID) ) {
		psd = &(card->device->stream_data[GAME_STREAM_ORDINAL(pgrs->streams[i].stream)]);
		
		/* not opened or closing is an error */
		if (!(psd->my_flags & X_ALLOCATED) || (psd->my_flags & X_CLOSING) ) {
			err = B_ERROR;
		}
		
		/* some more validation */
		if( GAME_IS_DAC(psd->gos_adc_dac_id) ) {
			frame_cvsr = (uint32) card->device->dac_info[GAME_DAC_ORDINAL(psd->gos_adc_dac_id)].cur_cvsr;
		}
		else if ( GAME_IS_ADC(psd->gos_adc_dac_id) ) {
			frame_cvsr = (uint32) card->device->adc_info[GAME_ADC_ORDINAL(psd->gos_adc_dac_id)].cur_cvsr;
		}
		else {
			err = B_ERROR;
		}	
			
		
		
		if (err == B_OK) {
			switch (pgrs->streams[i].state) {
			case GAME_STREAM_PAUSED:
				/* valid transition? */
				if (psd->state == GAME_STREAM_STOPPED ) {
					ddprintf(("xpress: run stream can't pause a stopped stream\n"));
					err = B_BAD_VALUE;
				}
				else {
					uint32 bytes_left = 0;
					lock_queue(card);
						psd->state = GAME_STREAM_PAUSED;
						{
							t_pqi = psd->stream_buffer_queue;
							while ((t_pqi != NULL) && (t_pqi->in_prd != -1)) {
								bytes_left += t_pqi->size;
								t_pqi = t_pqi->next;
							}
						}
					unlock_queue(card);
					ddprintf(("xpress: PAUSE\n"));
					pgrs->streams[i].out_timing.at_time = system_time() + (((uint64)(bytes_left >> 2) * 1000000LL)/(uint64)frame_cvsr);
					pgrs->streams[i].out_timing.cur_frame_rate = (float) frame_cvsr;
					pgrs->streams[i].out_timing.frames_played = psd->frame_count + (bytes_left >> 2);
					err = B_OK;
				}
			break;
			
			case GAME_STREAM_STOPPED:
				stop_stream_dma(card, psd, true);
				lock_queue(card);
				{
					uint8 i;
					queue_item * pqi = psd->stream_buffer_queue;
					while (pqi != NULL) {

						/* remove the queue item */
						remove_queue_item( card->device, psd, &release_sem[0], &release_me);
						if (release_me >= PRD_DEPTH) {
							/* this is ugly. the stream lock should keep us safe... */
							unlock_queue(card);
							while (release_me--) {
								if (release_sem[release_me] != B_ERROR) {
									release_sem_etc(release_sem[release_me], 1, B_DO_NOT_RESCHEDULE);
								}
							}	
							lock_queue(card);
							release_me = 0;
						}
						pqi = psd->stream_buffer_queue;
					}
					/* zero the registers */
					if (GAME_IS_DAC(psd->gos_adc_dac_id)) {
						/* clear pb register */
						MEM_WR8(card->device->f3 + 0x20, 0x0);
						i = MEM_RD8(card->device->f3 + 0x21);
						MEM_WR32(card->device->f3 + 0x24, psd->prd_table[PRD_DEPTH].buff_phys_addr);
					}
					else if (GAME_IS_ADC(psd->gos_adc_dac_id)) {
						/* clear cap register */
						MEM_WR8(card->device->f3 + 0x28, 0x0);
						i = MEM_RD8(card->device->f3 + 0x29);
						MEM_WR32(card->device->f3 + 0x2C, psd->prd_table[PRD_DEPTH].buff_phys_addr);
					}
					/* reset the PRD's... */
					psd->my_flags = X_ALLOCATED | X_DMA_STOPPED;
					psd->prd_add 	= 0;
					psd->prd_remove = 0;
					psd->frame_count= 0;
					psd->timestamp	= 0LL;
				}
				unlock_queue(card);

				/* reset the semaphore */
				delete_sem(psd->all_done_sem);
				/* the 0 really should be ix */
				{
					char name[32];
					sprintf(name, "xpress %d done_sem%d", 0,GAME_STREAM_ORDINAL(pgrs->streams[i].stream));
					psd->all_done_sem = create_sem(0, name);
				}
				pgrs->streams[i].out_timing.at_time = system_time();
				pgrs->streams[i].out_timing.cur_frame_rate = (float) frame_cvsr;
				pgrs->streams[i].out_timing.frames_played = psd->frame_count;
			break;
			
			case GAME_STREAM_RUNNING:
				if (psd->state != GAME_STREAM_RUNNING) {
					lock_queue(card);
						if (psd->stream_buffer_queue != NULL) {
							if (psd->state == GAME_STREAM_PAUSED) {
								/* adjust the PRD pointer... */
								/* if it paused, we hit an "unexpected" (!closing) eot    */
								/* in the int handler. Set the prd register to the        */
								/* first item on the queue.                               */
								
								t_pqi = psd->stream_buffer_queue;
								IKASSERT((t_pqi != NULL));
								
								/* the prd's are not filled if PAUSE is set */
								/* we need to fill them now */
								load_prd(psd, t_pqi);								
							}
							
							start_stream_dma(card->device, psd);
						}
						else {
							psd->state = GAME_STREAM_RUNNING;
							psd->my_flags |= X_DMA_STOPPED;
						}					
					unlock_queue(card);
					err = B_OK; /* already set */
				}
				else {
					err = B_BAD_VALUE;
				}
				pgrs->streams[i].out_timing.at_time = system_time();
				pgrs->streams[i].out_timing.cur_frame_rate = (float) frame_cvsr;
				pgrs->streams[i].out_timing.frames_played = psd->frame_count;

			break;
			default:
				dprintf("xpress: unknown run_state\n");
				err = B_BAD_VALUE;
			break;					
			} /* switch */
		} /* B_OK */
	}
	
	return err;
}

inline void
start_stream_dma(	xpress_dev * pdev,
					struct _stream_data * psd )
{
	/* MUST be called with the queue locked!!! */
	
	psd->state = GAME_STREAM_RUNNING;
	psd->my_flags &= ~X_DMA_STOPPED;
	
	/* start it up */	
	/* is it pb or cap? */
	if (GAME_IS_DAC(psd->gos_adc_dac_id)) {
		/* bus master 0 */
		MEM_WR8(pdev->f3 + 0x20, 0x1);
	}
	else {
		/* bus master 1 */
		MEM_WR8(pdev->f3 + 0x28, 0x9);
	}
}

inline void
stop_stream_dma(	open_xpress * card,
					struct _stream_data * psd,
					bool flush)
{
	/* MUST be called with the stream locked */
	/* DO NOT call with the queue locked!!!  */
	lock_queue(card);
		psd->state = GAME_STREAM_STOPPED;
		if (flush) {
			int16 i;	
			/* stop dma by setting prd entries to EOT */
			for (i = 0; i < PRD_DEPTH; i++) {
				psd->prd_table[i].flags          = X_EOT;
				psd->prd_table[i].buff_phys_addr = 0; 
				psd->prd_table[i].size           = 0; 
			}
		}
		psd->my_flags |= X_DMA_DONE_SEM;
	unlock_queue(card);
	if (!(psd->my_flags & X_DMA_STOPPED)) {
		acquire_sem_etc(psd->all_done_sem, 1, B_CAN_INTERRUPT , 0);
	}
}


inline void
remove_queue_item(
				xpress_dev * pdev,
				struct _stream_data * pbstream,
				sem_id * prelease_sem,
				int16 *  prelease_me)
{
	/* yo, 808. This function must be called with the QUEUE locked!! */
	queue_item * pqi = NULL;
	struct _buffer_data * pbuffer = NULL;

	pqi = pbstream->stream_buffer_queue;
	if (pqi != NULL) {
		pbstream->stream_buffer_queue = pqi->next;
		
		/* is this an end of buffer? */
		if (pqi->q_flags & SUPER_SECRET_END_OF_BUFFER ) {
			pbuffer = &(pdev->buffer_data[GAME_BUFFER_ORDINAL(pqi->buffer)]);
			/* decrement the ref count when dequeueing a _buffer_ */
			/* decrement IF we're not looping OR we are stopping  */
			if (!(pqi->q_flags & GAME_BUFFER_LOOPING) || (pbstream->state == GAME_STREAM_STOPPED))
				pbuffer->ref_count--;
									
		}

		/* this can now happen at any time... */
		if (pqi->q_flags & GAME_RELEASE_SEM_WHEN_DONE) {
			prelease_sem[(*prelease_me)++] = pbstream->gos_stream_sem;
		}


		/* if we are looping (but not stopping), reinsert... */
		if ((pqi->q_flags & GAME_BUFFER_LOOPING) && (pbstream->state != GAME_STREAM_STOPPED)) {
			queue_item * t_pqi = pqi;
			
			while (t_pqi->next != NULL) {
				t_pqi = t_pqi->next;
			}
			t_pqi->next = pqi;
			pqi->next = NULL;
			pqi->in_prd = -1;
		}
		else {
			/* otherwise put it on the free list */
			pqi->next = pbstream->free_list;
			pbstream->free_list = pqi;
		}
		
		

	} /* if buffer_queue != NULL */
}					