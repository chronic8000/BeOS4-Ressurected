#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <vm.h>
#include <byteorder.h>

#include "usbd.h"

#define _USBD_DEBUG 0

#if _USBD_DEBUG
#define ddprintf(aaa...)  dprintf(aaa)
#else
#define ddprintf(aaa...)
#endif

#if __GNUC__
#define ID "usbd: [" __FUNCTION__ "] "
#else
#define ID "usbd: "
#endif

#define PIPE_QUEUE_LOCK_MAX		16		/* Max number threads in a queue function per pipe
										** if more then 16 let things settle down a bit
										*/
#if 0
#define	PHYS_ENTRY_BUFFER_SIZE	34		/* This is for create_phys_entry_list stack buffer */
#endif

#define CHECK_PIPE()			if(SIG_CHECK_PIPE(pipe)) return B_DEV_INVALID_PIPE;


#define PE_ARRAY_NUM 9		/* has to be >= 2 */
typedef struct
{
	physical_entry	pe_array[PE_ARRAY_NUM];
#if _USBD_DEBUG	
	physical_entry	trip;  
#endif
	size_t			pe_num;
	physical_entry* current_pe_ptr;
	uint8*			unmapped_ptr;
	const iovec*	unmapped_iovec;
	

	const iovec*	vecs;
	size_t			vec_num;
	const iovec*	current_iovec;

	uint8*			current_va_ptr;

#if _USBD_DEBUG		
	size_t			remains, remains_to_map;
#endif
} phys_entry_list_state;


typedef	struct copy_back_descriptor copy_back_descriptor;
struct 	copy_back_descriptor
{
	copy_back_descriptor*	next;
	void* 					dest_pa;
	size_t					size;
};

static void * phys_addr(void *x)
{
	physical_entry pe;
	get_memory_map(x, 1, &pe, 1);
	return pe.address;
}




status_t create_pipe(usb_bus *bus, bm_usb_pipe **_pipe, 
					 usb_endpoint_descriptor *descr, 
					 int addr, bool lowspeed, bm_usb_interface * ifc)
{
	status_t status;
	int bandwidth;
	bm_usb_pipe *pipe;
	char buf[B_OS_NAME_LENGTH];
	
	if (addr < 0 || addr > 127)
		return B_BAD_VALUE;
	
	if(!(pipe = (bm_usb_pipe *) malloc(sizeof(bm_usb_pipe)))){
		return B_NO_MEMORY;
	}
	
	status = bus->ops->create_endpoint(bus->hcd,&pipe->ept,descr,addr,lowspeed, FALSE);
	if(status != B_OK){
		dprintf(ID "error in create_endpoint\n");
		goto err1;
	}
	
	bus->ops->get_endpoint_info(pipe->ept, &pipe->iobsz, &bandwidth);
	pipe->iobsz += sizeof(usb_iob);
	
	pipe->timeout = B_INFINITE_TIMEOUT;				
	pipe->ifc = ifc;
	pipe->pool = NULL;
	pipe->pool_lock = 0;
	pipe->addr_flags = descr->endpoint_address;
	pipe->max_packet_size = descr->max_packet_size;
	if(lowspeed) pipe->addr_flags |= PIPE_LOWSPEED;
	
	switch(descr->attributes & 0x03) {
		case 0x00:
			pipe->addr_flags |= PIPE_CONTROL;
			pipe->timeout = 6*1000*1000;
			break;
		case 0x01:
			pipe->addr_flags |= PIPE_ISOCH;
			pipe->iso_idle = TRUE;
			break;
		case 0x02:
			pipe->addr_flags |= PIPE_BULK;
			break;
		case 0x03:
			pipe->addr_flags |= PIPE_INT;
			break;
	}
	
	SIG_SET_PIPE(pipe);
	
	sprintf(buf, "bm_usb_pipe lock");
	status = new_mlock(&pipe->lock, PIPE_QUEUE_LOCK_MAX, buf);
	if (status) {
		dprintf(ID "error in new_mlock\n");
		goto err1;
	}
	
	status = get_usb_id(pipe);
	if (status != B_OK) {
		dprintf(ID "error in get_pipe_id\n");
		goto err2;
	}
	
	*_pipe = pipe;
	return B_OK;

err2:
	free_mlock(&pipe->lock);
err1:
	free(pipe);
	return status;
}

void destroy_pipe(bm_usb_pipe *pipe)
{
	usb_iob		*iob, *next;
	status_t	err;
	
	if(SIG_CHECK_PIPE(pipe)) {
		return;
	}
	
	err = put_usb_id(pipe);
	if (err) {
		dprintf(ID "error in put_usb_id for pipe with id %lu\n", pipe->id);
		return;
	}
	
	/* Make sure no other threads are using this endpoint */
	LOCKM(pipe->lock, PIPE_QUEUE_LOCK_MAX);
	free_mlock(&pipe->lock);
	
	SIG_CLR_PIPE(pipe);
	
	/* Cancel all the existing transfers */
	pipe->ifc->dev->bus->ops->cancel_transfers(pipe->ept);
	
	/* Kill the endpoint in the bus */
	pipe->ifc->dev->bus->ops->destroy_endpoint(pipe->ept);
	
	for(iob = pipe->pool; iob; iob = next){
		next = iob->next;
		free(iob);
	}
	free(pipe);
}

usb_iob *get_iob(bm_usb_pipe *pipe, int can_block)
{
	usb_iob *iob;
	cpu_status ps;
	
	ps = disable_interrupts();
	acquire_spinlock(&pipe->pool_lock);
	
	iob = pipe->pool;
	if(iob) pipe->pool = iob->next;
	
	release_spinlock(&pipe->pool_lock);
	restore_interrupts(ps);

	if(!iob) iob = (usb_iob *) malloc(pipe->iobsz);
	
	iob->next = NULL;
	iob->transfer.workspace_length = pipe->iobsz - sizeof(usb_iob);
	return iob;
}

void put_iob(bm_usb_pipe *pipe, usb_iob *iob)
{
	cpu_status ps;
	
	ps = disable_interrupts();
	acquire_spinlock(&pipe->pool_lock);
	
	iob->next = pipe->pool;
	pipe->pool = iob;
	
	release_spinlock(&pipe->pool_lock);
	restore_interrupts(ps);
}

static void print_phys_entries(const hcd_phys_entry * in)
{
	const hcd_phys_entry * node = in;
	while(node) {
		if (node->packet_buffer) {
			dprintf(ID "address = %p, size = %ld, packet_buffer = %p\n", node->address, node->size, node->packet_buffer);
		} else {
			dprintf(ID "address = %p, size = %ld\n", node->address, node->size);
		}
		node = node->next;
	}
}

#if 0
static void dump_packet_buffer(void * buf)
{
	int foo;
	dprintf(ID "the data is = ");
	for (foo = 0; foo < 8; foo++) {
		dprintf("0x%02x\t", ((uint8 *)buf)[foo]);
	}
	dprintf("\n");
}
#endif


static copy_back_descriptor* get_copy_back_descriptor(usb_bus* bus)
{
	return malloc(sizeof(copy_back_descriptor));	/* FIXME use pool */
}

static void put_copy_back_descriptor(usb_bus* bus, copy_back_descriptor* cbd)
{
	free(cbd);	/* FIXME use pool */
}



static void fill_pe_array(phys_entry_list_state* st)
{
	size_t	mapped_chunk_size;
	physical_entry*	pes = st->pe_array;
	
	ddprintf("fill_pe_array()\n");

	st->current_pe_ptr = st->pe_array;
	st->pe_num = 0;
	while((st->pe_num < PE_ARRAY_NUM) && (st->unmapped_iovec < st->vecs + st->vec_num) )
	{
		ddprintf("get_memory_map(%p, %ld, %p, %ld)\n",
			st->unmapped_ptr, (((uint8*)st->unmapped_iovec->iov_base) + st->unmapped_iovec->iov_len) - st->unmapped_ptr, pes, PE_ARRAY_NUM - st->pe_num);
		
		get_memory_map(st->unmapped_ptr, (((uint8*)st->unmapped_iovec->iov_base) + st->unmapped_iovec->iov_len) - st->unmapped_ptr, pes, PE_ARRAY_NUM - st->pe_num );
		for(mapped_chunk_size=0; (pes < st->pe_array+PE_ARRAY_NUM) && pes->size;  pes++)
		{
			mapped_chunk_size += pes->size; 
			st->pe_num++;
		}

		st->unmapped_ptr += mapped_chunk_size;
#if _USBD_DEBUG	
		st->remains_to_map -= mapped_chunk_size;
#endif
		/* we can't use the last pe in pe_array, unless it's the last last pe */
		if((st->pe_num == PE_ARRAY_NUM) && (st->unmapped_ptr != (uint8*)st->vecs[st->vec_num-1].iov_base + st->vecs[st->vec_num-1].iov_len)  )
		{
			ddprintf("roll back by %ld\n", st->pe_array[PE_ARRAY_NUM-1].size);
			st->unmapped_ptr -= st->pe_array[PE_ARRAY_NUM-1].size;	/* roll back */
#if _USBD_DEBUG	
			st->remains_to_map += st->pe_array[PE_ARRAY_NUM-1].size;
#endif
		}
		
		if(((uint8*)st->unmapped_iovec->iov_base + st->unmapped_iovec->iov_len) - st->unmapped_ptr == 0)	/* the whole iovec is done */
		{		
			st->unmapped_iovec++;	/* next unmaped iovec */
			if(st->unmapped_iovec < st->vecs + st->vec_num)
				st->unmapped_ptr = st->unmapped_iovec->iov_base; 
		}
	}
}


/* can be advanced only up to the end of the current iovec or the current pe. FIXME add assert()s. */
static void advance(phys_entry_list_state* st, size_t	delta)
{
#if 0
	dprintf("advance(%p, %ld)\n", st, delta);
	dprintf("pe_num = %ld\n", st->pe_num);
	dprintf("current_pe_ptr: %p, %p, %ld\n", st->current_pe_ptr, st->current_pe_ptr->address, st->current_pe_ptr->size);
	dprintf("unmapped_ptr = %p\n", st->unmapped_ptr);
	dprintf("unmapped_iovec: %p, %p, %ld\n", st->unmapped_iovec, st->unmapped_iovec->iov_base, st->unmapped_iovec->iov_len);
	dprintf("vecs = %p, vec_num = %ld\n", st->vecs, st->vec_num);
	dprintf("current_iovec: %p, %p, %ld\n", st->current_iovec, st->current_iovec->iov_base, st->current_iovec->iov_len);
	dprintf("current_va_ptr = %p\n\n", st->current_va_ptr);
	dprintf("data end = %p\n", (uint8*)st->vecs[st->vec_num-1].iov_base + st->vecs[st->vec_num-1].iov_len);
	dprintf("remains = %ld\n", st->remains);
	dprintf("remains_to_map = %ld\n", st->remains_to_map);
#endif	

	if(!st->current_va_ptr || !st->current_pe_ptr)
	{
		dprintf("error: st = %p, st->current_va_ptr = %p, st->current_va_ptr = %p ***************************\n",
			st, st->current_va_ptr, st->current_pe_ptr);
		return;
	}
#if _USBD_DEBUG
	st->remains -= delta;
#endif
	
	st->current_pe_ptr->address = ((uint8*)st->current_pe_ptr->address) + delta;
	st->current_pe_ptr->size -= delta;

	if(st->current_pe_ptr->size == 0)
	{
		if((st->current_pe_ptr < st->pe_array + PE_ARRAY_NUM - 2) ||
			(st->pe_num < PE_ARRAY_NUM) ||
			(st->unmapped_ptr == (uint8*)st->vecs[st->vec_num-1].iov_base + st->vecs[st->vec_num-1].iov_len)
			)	/* we need valid current_pe_ptr and valid next pe, PE_ARRAY_NUM has to be >= 2 */
			st->current_pe_ptr++;
		else
			fill_pe_array(st);
	}

	st->current_va_ptr += delta;
	if(st->current_va_ptr == (uint8*)st->current_iovec->iov_base + st->current_iovec->iov_len)
	{
		if(st->current_iovec < st->vecs + st->vec_num-1)
		{
			st->current_iovec++;
			st->current_va_ptr = st->current_iovec->iov_base;
		}
		else
		{
			st->current_va_ptr = NULL;
			st->current_pe_ptr = NULL;
		}
	}
}



static void init_phys_entry_list_state(phys_entry_list_state* st, const iovec vecs[], size_t vec_num)
{
#if _USBD_DEBUG	
	st->trip.address = NULL;
	st->trip.size = 0xdb13;
#endif

	st->current_iovec = st->unmapped_iovec = st->vecs = vecs;
	st->current_va_ptr = st->unmapped_ptr = vecs[0].iov_base;

	st->vec_num = vec_num;

#if _USBD_DEBUG		
	{
		int i;
		size_t	len;
		
		for(len=0,i=0; i<vec_num; i++)
			len += vecs[i].iov_len;
		st->remains = st->remains_to_map = len;
	}
#endif
	fill_pe_array(st);
}

#define current_pe(st)				((st)->current_pe_ptr)
#define	current_va(st)				((st)->current_va_ptr)
#define	next_pe(st)					((st)->current_pe_ptr + 1)
#define	current_pe_is_last_pe(st)	(((st)->current_pe_ptr >= (st)->pe_array + (st)->pe_num-1) && ((st)->unmapped_iovec >= (st)->vecs + (st)->vec_num-1))


static hcd_phys_entry* 	get_and_add_hcd_phys_entry(usb_bus* bus, hcd_phys_entry** head_ptr, hcd_phys_entry** tail_ptr, void* physical_address, size_t size)
{
	hcd_phys_entry* const	hcd_pe = get_phys_entry(bus);

	hcd_pe->next = NULL;
	hcd_pe->address = physical_address;
	hcd_pe->size = size;

	if(*head_ptr == NULL)
		*head_ptr = hcd_pe;
	else
		(*tail_ptr)->next = hcd_pe;

	*tail_ptr = hcd_pe;
	return hcd_pe;
}




static status_t create_phys_entry_list(const bm_usb_pipe * pipe, iovec  vec_in[], size_t count, hcd_phys_entry ** pe_list_head, bool inbound)
{
	size_t					prolog_size = 0;
	size_t					epilog_size;
	usb_bus* const 			bus = pipe->ifc->dev->bus;
	hcd_phys_entry			*head_ptr = NULL, *tail_ptr;
	const size_t			packet_size = pipe->max_packet_size;
	const bool				allow_split_packets = (bus->page_crossings_allowed != 0);
	const physical_entry*	pe;
	phys_entry_list_state	st;
	
	ddprintf("create_phys_entry_list()\n");
	
	init_phys_entry_list_state(&st, vec_in, count);

	while((pe = current_pe(&st)))
	{
		ddprintf("pe = %p, %p, %lu\n", pe, pe->address, pe->size);
		
		epilog_size = (pe->size - prolog_size) % packet_size; 
		if(
			current_pe_is_last_pe(&st)	||	/* just add it OR */
			(epilog_size == 0)			||	/* pe end is packet end  OR */
			(								/* the current pe and the next pe are "mergeable" */
				 allow_split_packets && 													/* AND */
				(((ulong)((uint8*)pe->address + pe->size + 1) & (B_PAGE_SIZE-1)) == 0 ) &&					/* current pe ends on a page boundary AND */
				(((ulong)((uint8*)next_pe(&st)->address + next_pe(&st)->size) & (B_PAGE_SIZE-1)) == 0) &&	/* next pe starts on a page boundary AND */
				(next_pe(&st)->size >= packet_size - epilog_size )							/* next pe has enough data */
			)
		)
		{
			/* add pe */
			get_and_add_hcd_phys_entry(bus, &head_ptr, &tail_ptr, pe->address, pe->size);

			/* next pe */
			advance(&st, pe->size);
			prolog_size = packet_size - epilog_size;
			continue;
		}
		
		/* add prolog and whole packets */
		if(pe->size - epilog_size != 0)
		{
			get_and_add_hcd_phys_entry(bus, &head_ptr, &tail_ptr, pe->address, pe->size - epilog_size);
			advance(&st, pe->size - epilog_size);
		}
		
		/* do packet buffer allocation and data copying */
		{
			uint8* const			pb = get_packet_buffer(bus);	/* FIXME also get phys_address of the packet buffer */
			size_t					pb_remainder = packet_size;
			hcd_phys_entry* const	hcd_pe = get_and_add_hcd_phys_entry(bus, &head_ptr, &tail_ptr, phys_addr(pb), 0/* bogus size, will be set at the end */); /* FIXME use known phys_address of the packet buffer */
			copy_back_descriptor*	cbd_tail_ptr;	
		
			ddprintf("do packet buffer allocation and data copying\n");
		
			hcd_pe->packet_buffer = pb;
			 
			/* map/fill the packet buffer */
			do
			{
				/* for this pe/pe chunk copy data / add copyback descriptor */				
				const size_t	copy_size = (pe->size <= pb_remainder) ? pe->size : pb_remainder;
				
				ddprintf("pe = %p, %p, %lu,  pb_remainder = %ld, copy_size = %ld\n", pe, pe->address, pe->size, pb_remainder, copy_size);
				
				if(inbound)
				{
					copy_back_descriptor* copy_back_descr = get_copy_back_descriptor(bus);
					
					copy_back_descr->dest_pa = pe->address;
					copy_back_descr->size = copy_size;

					/* add to the tail of the copy back descriptor list */
					copy_back_descr->next = NULL;
					if(hcd_pe->copy_back_descr == NULL)	/* first copyback descriptor */
						cbd_tail_ptr = hcd_pe->copy_back_descr = copy_back_descr;
					else
					{
						cbd_tail_ptr->next = copy_back_descr;
						cbd_tail_ptr = copy_back_descr;
					}
					ddprintf("adding cpd: %ld bytes @ %p\n", copy_back_descr->size, copy_back_descr->dest_pa);
				}
				else	/* outbound */
				{
					ddprintf("copying %ld bytes from %p to %p\n", copy_size, current_va(&st), copy_size);
					memcpy(pb + (packet_size - pb_remainder), current_va(&st), copy_size);
				}

				pb_remainder -= copy_size;
				advance(&st, copy_size);

			} while((pb_remainder > 0) && (pe = current_pe(&st)) );
			
			hcd_pe->size = packet_size - pb_remainder;	/* set the correct size */
			prolog_size = 0;
		}
	}	
	*pe_list_head = head_ptr;

#if _USBD_DEBUG	
	if(st.remains || st.remains_to_map)
	{	
		dprintf("remains = %ld\n", st.remains);
		dprintf("remains_to_map = %ld\n", st.remains_to_map);
		kernel_debugger("");
	}
#endif

	ddprintf("create_phys_entry_list() done\n");
	return B_OK;	/* FIXME change to void? */
}



static status_t create_phys_entry_list_for_iso(const bm_usb_pipe * pipe, void* buffer, size_t buffer_size,
	usb_iso_packet_descriptor* packet_descriptors, size_t packet_count,
	hcd_phys_entry ** pe_list_head, bool inbound)
{
	usb_bus* const 			bus = pipe->ifc->dev->bus;
	hcd_phys_entry			*head_ptr = NULL, *tail_ptr;
	const bool				allow_split_packets = (bus->page_crossings_allowed != 0);
	const physical_entry*	pe;
	phys_entry_list_state	st;
	const iovec				buffer_iovec = {buffer, buffer_size};

	if(!allow_split_packets)
	{
		dprintf("USB iso is unimplemented for UHCI\n");
		return B_UNSUPPORTED;	// BUGBUG implement for UHCI
	}
		
	init_phys_entry_list_state(&st, &buffer_iovec, 1);

	while((pe = current_pe(&st)))
	{
		ddprintf("pe = %p, %p, %lu\n", pe, pe->address, pe->size);
		
		/* add pe */
		get_and_add_hcd_phys_entry(bus, &head_ptr, &tail_ptr, pe->address, pe->size);

		/* next pe */
		advance(&st, pe->size);
	}
	
	*pe_list_head = head_ptr;

	return B_OK;
}




static void cleanup_phys_entry_list(usb_bus * bus, const hcd_phys_entry* hcd_phys_entry_list)
{
	const hcd_phys_entry *hcd_pe, *next_hcd_pe;
	
	for(hcd_pe = hcd_phys_entry_list; hcd_pe; hcd_pe = next_hcd_pe )
	{
		if(hcd_pe->packet_buffer != NULL)
		{
			copy_back_descriptor* copy_back_descr, *next_cbd;
			uint8*	src = hcd_pe->packet_buffer;
			
			for(copy_back_descr = hcd_pe->copy_back_descr; copy_back_descr; copy_back_descr = next_cbd)
			{
				ddprintf("copying %ld bytes from %p to %p/%p\n", copy_back_descr->size, src, copy_back_descr->dest_pa, patova(copy_back_descr->dest_pa));
				memcpy(patova(copy_back_descr->dest_pa), src, copy_back_descr->size);
				src += copy_back_descr->size;
				next_cbd = copy_back_descr->next;
				put_copy_back_descriptor(bus, copy_back_descr);
			}
			put_packet_buffer(bus, hcd_pe->packet_buffer);
		}
		next_hcd_pe = hcd_pe->next;
		put_phys_entry(bus, hcd_pe);
	}
}



#if 0
/*	create_phys_entry_list
**	This function takes an iovec and returns a linked list of hcd_phys_entries each of which point to a physical
**	peice of memory for the bus to transfer to/from. packet_size is the maximum packet size for the enpoint the transfer
**	is going to sent to/received from.
*/
static status_t create_phys_entry_list(const bm_usb_pipe * pipe, iovec * vec_in, size_t count, hcd_phys_entry ** pe_out)
{
	int32			i, j, len;
	int32			mapped_bytes;			/* This is the number of bytes we've mapped for the current iovec */
	int32			page_boundries_left;	/* The number of page boundries we have left until we have to split */
	int32			last_packet;			/* A pointer to the end of the last full packet before the next page boundry */
	status_t		err;
	void			* packet_buffer, * buf;
	int32			packet_copy_size = 0;	/* This is used for packet copying logic */
	size_t			size;
	hcd_phys_entry	* cur, * last, * head;
	physical_entry	pe[PHYS_ENTRY_BUFFER_SIZE];
	uint8			* base;
	uint8			* next_page;
	uint32			sofar, total_bytes;
	iovec			* vec;

	int32			packet_size = pipe->max_packet_size;
	usb_bus			* bus = pipe->ifc->dev->bus;
	bool			inbound = pipe->addr_flags & 0x80;
	
	/* XXX temp until DPCs (JWH) */
	iovec			* copy_vec;
	int32			copy_vec_offset = 0;

	ddprintf(ID "---> packet_size = %ld, num_crossable_pages = %ld\n", packet_size, bus->page_crossings_allowed);
	
	dprintf("inbound = %08x, pipe->addr_flags = %08x\n", inbound, pipe->addr_flags);
	
	/* Sanity checking */
	if (!vec_in || count < 1 || !bus || !pe_out) {
		dprintf(ID "error: vec_in = %p, count = %ld, bus = %p, pe_out = %p\n", vec_in, count, bus, pe_out);
		return B_BAD_VALUE;
	}
	
	/* Create the linked list of usb_phys_entries */
	len = total_bytes = 0;
	last = head = packet_buffer = NULL;		
	page_boundries_left = bus->page_crossings_allowed;
	
	for(vec = vec_in, j = 0; j < count; j++, vec++) {
		len = vec->iov_len;
		buf = vec->iov_base;
		mapped_bytes = 0;
		ddprintf(ID "j = %ld, count = %ld, len = %ld\n", j, count, len);
		for(i = PHYS_ENTRY_BUFFER_SIZE + 1; mapped_bytes < len; i++) { /* once per physical entry from get_memory_map */
			/*	XXX Get more memory mappings if needed.
			**	This should not happen, instead we should split the request
			**	up into peices for the bus when we run out of resources,
			**	but we need DPCs to do that so this will work for now. (JWH)
			*/
			if (i >= PHYS_ENTRY_BUFFER_SIZE) {
				err = get_memory_map(((char *)buf + mapped_bytes), len - mapped_bytes, pe, PHYS_ENTRY_BUFFER_SIZE);
				if (err) {
					dprintf(ID "error in get_memory_map\n");
					return B_ERROR;
				}
				i = 0;
			}
			
			/* Set the base for this phys entry */
			base = pe[i].address;
				
			ddprintf("\n" ID "for loop - pe[%ld].address = %p, pe[%ld].size = %ld\n", i, pe[i].address, i, pe[i].size);
	
			sofar = 0;
			while(sofar < pe[i].size) {
				/* We do not need to finish a packet copy */
				if (! packet_buffer) {
					
					/* Find the next page boundry and calculate the last_packet */
					next_page = (uint8*)((uint32)(base + 4096) & ~(4096 - 1));
					size = (next_page - base);
					if(pe[i].size < sofar + size) {
						size = pe[i].size - sofar;
						last_packet = 0;
					} else {
						int32 tmp;
						tmp = (mapped_bytes + size) % packet_size;
						if (tmp) {
							last_packet = size - tmp;
						} else {
							last_packet = 0;
						}
						ddprintf(ID "last_packe					</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/webmd.jpg">
								</TD>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="DEDEAA">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="#CDCD99" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="CDCD99" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/travelscape.jpg">
								</TD>
								<TD BGCOLOR="#CDCD99" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="CDCD99">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/homeport.jpg">
								</TD>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="DEDEAA">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="#CDCD99" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="CDCD99" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/xgames.jpg">
								</TD>
								<TD BGCOLOR="#CDCD99" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="CDCD99" ALIGN="right">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
						</TABLE>
						<!--
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
							END CONTENT SECTION
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						-->
					</TD>
					<TD WIDTH="20"><BR>
					</TD>
					<TD VALIGN="top" ALIGN="left" WIDTH="122">
						<TABLE CELLPADDING=0 CELLSPACING=0>
							<TR>
								<TD>
									<A HREF="http://www.spiegel.com"><IMG SRC="../images/ads/spiegel_11_22_2000_01.png" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.spiegel.com"><IMG SRC="../images/ads/spiegel_11_22_2000_02.png" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.illuminations.com"><IMG SRC="../images/ads/illuminations_11_22_2000_01.jpg" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.costco.com"><IMG SRC="../images/ads/costco_11_22_2000_01.png" BORDER="0"></A>
								</TD>
							</TR>
						</TABLE>
					</TD>
				</TR>
			</TABLE>
		</TD>
	</TR>
</TABLE>

</BODY>
</HTML>PK
     ˚|[*            /  custom/resources/en/netguide/entertainment/ps2/UT	  :ú:©&ù:Ux     PK
     ˚|[*"P^s/  s/  9 > custom/resources/en/netguide/entertainment/ps2/index.htmlUT	  :ú:©&ù:Ux     Be%     BEOS:TYPE MIMS       
text/html <HTML>
<HEAD>
<SCRIPT SRC="../bin/preload_images.js" TYPE="text/javascript" LANGUAGE="javascript"></SCRIPT>
<TITLE>Entertainment</TITLE>
<STYLE TYPE="text/css">
@import url(../../bin/inside.css);
</STYLE>
</HEAD>
<BODY onLoad="preloadTopBarImages()" MARGINWIDTH="0"  MARGINHEIGHT="0" LEFTMARGIN="0" TOPMARGIN="0" BGCOLOR=#FFFFCC>

<TABLE WIDTH=785="0" CELLPADDING=0 CELLSPACING=0>
	<TR>
		<TD VALIGN="top" ALIGN="left" ROWSPAN="2">
			<IMG SRC="../images/corner_entertainment.jpg" WIDTH=161 HEIGHT=108>
			<TABLE BORDER="0" WIDTH=161 CELLPADDING=0 CELLSPACING=0>
				<TR>
					<TD HEIGHT="10">
					</TD>
				</TR>
				<TR>
					<TD WIDTH=161 ALIGN="left">
						<TABLE CLASS="side-text" CELLPADDING=0 CELLSPACING=0 border="0">
							<TR>
								<TD HEIGHT="14" WIDTH="161" BACKGROUND="../../images/side_cat_top.gif">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../../news/index.html">News</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../movie/index.html">Movie Listings</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../tv/index.html">TV Listings</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../pictures/index.html">Sony Pictures</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../music/index.html">Sony Music</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../games/index.html">Games</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BGCOLOR="FFFFFF">
									<FONT COLOR="FFFFFF">...</FONT><A HREF="../ps2/index.html">Sony PS2</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../gameshow/index.html">Game Show Network</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../station/index.html">The Station</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="12" WIDTH="161" BACKGROUND="../../images/side_cat_bot.gif">
								</TD>
							</TR>
						</TABLE>
					</TD>
				</TR>
			</TABLE>
		</TD>
		<TD VALIGN="top" ALIGN="left" COLSPAN="2">
			<TABLE WIDTH=625 BORDER=0 CELLPADDING=0 CELLSPACING=0>
				<TR>
					<TD>
						<IMG SRC="../../images/netguide.gif" WIDTH=55 HEIGHT=32></TD>
					<TD>
						<A HREF="../../news/index.html" OnMouseOver="document.news.src='../../images/r_news.gif'" OnMouseOut="document.news.src='../../images/h_news.gif'"><IMG SRC="../../images/h_news.gif" WIDTH=66 HEIGHT=32 NAME="news" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../finance/index.html" OnMouseOver="document.finance.src='../../images/r_finance.gif'" OnMouseOut="document.finance.src='../../images/h_finance.gif'"><IMG SRC="../../images/h_finance.gif" WIDTH=76 HEIGHT=32 NAME="finance" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../local/index.html" OnMouseOver="document.local.src='../../images/r_local.gif'" OnMouseOut="document.local.src='../../images/h_local.gif'"><IMG SRC="../../images/h_local.gif" WIDTH=64 HEIGHT=32 NAME="local" BORDER="0"></A></TD>
					<TD>
						<IMG SRC="../../images/a_entertainment.gif" WIDTH=125 HEIGHT=32 NAME="entertainment" BORDER="0"></TD>
					<TD>
						<A HREF="../../shopping/index.html" OnMouseOver="document.shopping.src='../../images/r_shopping.gif'" OnMouseOut="document.shopping.src='../../images/h_shopping.gif'"><IMG SRC="../../images/h_shopping.gif" WIDTH=88 HEIGHT=32 NAME="shopping" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../lifestyle/index.html" OnMouseOver="document.lifestyle.src='../../images/r_lifestyle.gif'" OnMouseOut="document.lifestyle.src='../../images/h_lifestyle.gif'"><IMG SRC="../../images/h_lifestyle.gif" WIDTH=82 HEIGHT=32 NAME="lifestyle" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../sports/index.html" OnMouseOver="document.sports.src='../../images/r_sports.gif'" OnMouseOut="document.sports.src='../../images/h_sports.gif'"><IMG SRC="../../images/h_sports.gif" WIDTH=70 HEIGHT=32 NAME="sports" BORDER="0"></A></TD>
				</TR>
			</TABLE>
		</TD>
	</TR>
	<TR>
		<TD VALIGN="top" ALIGN="left" WIDTH="624">
			<TABLE border="0" CELLPADDING=0 CELLSPACING=0>
				<TR>
					<TD WIDTH="20"><BR>
					</TD>
					<TD>

						<!--
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
							START CONTENT SECTION
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						-->
						<TABLE border="0" CELLPADDING=0 CELLSPACING=0 WIDTH="462">
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/homeport.jpg">
								</TD>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="DEDEAA">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="#CDCD99" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="CDCD99" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/homeport.jpg">
								</TD>
								<TD BGCOLOR="#CDCD99" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="CDCD99">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/webmd.jpg">
								</TD>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="DEDEAA">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="#CDCD99" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="CDCD99" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/travelscape.jpg">
								</TD>
								<TD BGCOLOR="#CDCD99" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="CDCD99">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/homeport.jpg">
								</TD>
								<TD BGCOLOR="DEDEAA" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="DEDEAA">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="#CDCD99" HEIGHT="30" COLSPAN="2">
									<P CLASS="content-header">
									Computer Books
									</P>
								</TD>
							</TR>
							<TR>
								<TD BGCOLOR="CDCD99" HEIGHT="130" VALIGN="top" WIDTH="130" ALIGN="right">
									<IMG SRC="../images/dynamic/xgames.jpg">
								</TD>
								<TD BGCOLOR="#CDCD99" HEIGHT="130" VALIGN="top">
									<P CLASS="content">
									We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books. We have a great selection of Computer Books.
									</P>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" COLSPAN="2" BGCOLOR="CDCD99" ALIGN="right">
									<IMG SRC="../../images/go.gif" ALIGN="right">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="15">
								</TD>
							</TR>
						</TABLE>
						<!--
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
							END CONTENT SECTION
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						+++++++++++++++++++++++++++++
						-->
					</TD>
					<TD WIDTH="20"><BR>
					</TD>
					<TD VALIGN="top" ALIGN="left" WIDTH="122">
						<TABLE CELLPADDING=0 CELLSPACING=0>
							<TR>
								<TD>
									<A HREF="http://www.spiegel.com"><IMG SRC="../images/ads/spiegel_11_22_2000_01.png" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.spiegel.com"><IMG SRC="../images/ads/spiegel_11_22_2000_02.png" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.illuminations.com"><IMG SRC="../images/ads/illuminations_11_22_2000_01.jpg" BORDER="0"></A>
								</TD>
							</TR>
							<TR>
								<TD>
									<A HREF="http://www.costco.com"><IMG SRC="../images/ads/costco_11_22_2000_01.png" BORDER="0"></A>
								</TD>
							</TR>
						</TABLE>
					</TD>
				</TR>
			</TABLE>
		</TD>
	</TR>
</TABLE>

</BODY>
</HTML>PK
     ˚|[*            3  custom/resources/en/netguide/entertainment/station/UT	  :ú:©&ù:Ux     PK
     ˚|[*D˙}µs/  s/  = > custom/resources/en/netguide/entertainment/station/index.htmlUT	  :ú:©&ù:Ux     Be%     BEOS:TYPE MIMS       
text/html <HTML>
<HEAD>
<SCRIPT SRC="../bin/preload_images.js" TYPE="text/javascript" LANGUAGE="javascript"></SCRIPT>
<TITLE>Entertainment</TITLE>
<STYLE TYPE="text/css">
@import url(../../bin/inside.css);
</STYLE>
</HEAD>
<BODY onLoad="preloadTopBarImages()" MARGINWIDTH="0"  MARGINHEIGHT="0" LEFTMARGIN="0" TOPMARGIN="0" BGCOLOR=#FFFFCC>

<TABLE WIDTH=785="0" CELLPADDING=0 CELLSPACING=0>
	<TR>
		<TD VALIGN="top" ALIGN="left" ROWSPAN="2">
			<IMG SRC="../images/corner_entertainment.jpg" WIDTH=161 HEIGHT=108>
			<TABLE BORDER="0" WIDTH=161 CELLPADDING=0 CELLSPACING=0>
				<TR>
					<TD HEIGHT="10">
					</TD>
				</TR>
				<TR>
					<TD WIDTH=161 ALIGN="left">
						<TABLE CLASS="side-text" CELLPADDING=0 CELLSPACING=0 border="0">
							<TR>
								<TD HEIGHT="14" WIDTH="161" BACKGROUND="../../images/side_cat_top.gif">
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../../news/index.html">News</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../movie/index.html">Movie Listings</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../tv/index.html">TV Listings</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../pictures/index.html">Sony Pictures</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../music/index.html">Sony Music</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../games/index.html">Games</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../ps2/index.html">Sony PS2</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BACKGROUND="../../images/side_cat.gif">
									<FONT COLOR="C5C5A6">...</FONT><A HREF="../gameshow/index.html">Game Show Network</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="20" WIDTH="161" BGCOLOR="FFFFFF">
									<FONT COLOR="FFFFFF">...</FONT><A HREF="../station/index.html">The Station</A>
								</TD>
							</TR>
							<TR>
								<TD HEIGHT="12" WIDTH="161" BACKGROUND="../../images/side_cat_bot.gif">
								</TD>
							</TR>
						</TABLE>
					</TD>
				</TR>
			</TABLE>
		</TD>
		<TD VALIGN="top" ALIGN="left" COLSPAN="2">
			<TABLE WIDTH=625 BORDER=0 CELLPADDING=0 CELLSPACING=0>
				<TR>
					<TD>
						<IMG SRC="../../images/netguide.gif" WIDTH=55 HEIGHT=32></TD>
					<TD>
						<A HREF="../../news/index.html" OnMouseOver="document.news.src='../../images/r_news.gif'" OnMouseOut="document.news.src='../../images/h_news.gif'"><IMG SRC="../../images/h_news.gif" WIDTH=66 HEIGHT=32 NAME="news" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../finance/index.html" OnMouseOver="document.finance.src='../../images/r_finance.gif'" OnMouseOut="document.finance.src='../../images/h_finance.gif'"><IMG SRC="../../images/h_finance.gif" WIDTH=76 HEIGHT=32 NAME="finance" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../local/index.html" OnMouseOver="document.local.src='../../images/r_local.gif'" OnMouseOut="document.local.src='../../images/h_local.gif'"><IMG SRC="../../images/h_local.gif" WIDTH=64 HEIGHT=32 NAME="local" BORDER="0"></A></TD>
					<TD>
						<IMG SRC="../../images/a_entertainment.gif" WIDTH=125 HEIGHT=32 NAME="entertainment" BORDER="0"></TD>
					<TD>
						<A HREF="../../shopping/index.html" OnMouseOver="document.shopping.src='../../images/r_shopping.gif'" OnMouseOut="document.shopping.src='../../images/h_shopping.gif'"><IMG SRC="../../images/h_shopping.gif" WIDTH=88 HEIGHT=32 NAME="shopping" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../lifestyle/index.html" OnMouseOver="document.lifestyle.src='../../images/r_lifestyle.gif'" OnMouseOut="document.lifestyle.src='../../images/h_lifestyle.gif'"><IMG SRC="../../images/h_lifestyle.gif" WIDTH=82 HEIGHT=32 NAME="lifestyle" BORDER="0"></A></TD>
					<TD>
						<A HREF="../../sports/index.html" OnMouseOver="document.sports.src='../../images/r_sports.gif'" OnMouseOut="document.sports.src='../../images/h_sports.gif'"><IMG SRC="../../images/h_sports.gif" WIDTH=70 HEIGHT=32 NAME="sports" BORDER="0"></A></TD>
				</TR>
			</TABLE>
		</TD>
	</TR>
	<TR>
		<TD VALIG