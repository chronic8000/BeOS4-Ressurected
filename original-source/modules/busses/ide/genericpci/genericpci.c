/* PCI IDE module
*/

#include <IDE.h>
#include <PCI.h>
#include <KernelExport.h>
#include <string.h>
#include <malloc.h>
#include <lendian_bitfield.h>
#include <ByteOrder.h>
#include <boot.h>
#include <driver_settings.h>
#include <stdlib.h>

#include <checkmalloc.h>

#define DEBUG_INTERRUPTS 1
bool pci_ide_debug_interrupts = false;

static pci_module_info	*pci;

typedef struct {
	uint32	address;		// bit 0 reserved
	uint16	byte_count;		// bit 0 reserved
	uint8	reserved_1;
	LBITFIELD8_2(
			reserved_2 : 7,
	        eot        : 1
	);
} prd_t;
const size_t 	prd_table_size = B_PAGE_SIZE;

typedef struct bus_info_s {
	struct bus_info_s *next;
	uint16	command_block_base_addr;
	uint16	control_block_register_addr;
	uint16	busmaster_base_addr;
	uint16	busmaster_status_addr;
	int		intnum;
	int		interrupt_gate;
	sem_id	mutex;
	bool	mutex_shared;
	sem_id	intsem;
	
	uint8	busmasterstatus;
	bool	shared_interrupt;
	uint32	bad_alignment_mask;

	area_id	prd_table_area;
	prd_t	*prd_table;
	uint32	prd_table_physical;

	bool dmainuse;
	bool dma_started;

	bool buslock;

	/* dma_mode */

	status_t (*get_dma_mode)(struct bus_info_s *bi, bool device1, uint32 *mode);
	status_t (*set_dma_mode)(struct bus_info_s *bi, bool device1, uint32 mode);
	
	interrupt_handler handler;
	struct {
		uint8	bus;
		uint8	device;
		uint8	function;
	} pci_dev;
	bool secondary;

} bus_info;

static int buscount;
static bus_info *busses;

static bus_info *
get_nth_cookie(uint32 bus)
{
	int i;
	bus_info *cookie = busses;
	
	for(i=0; i<bus; i++) {
		if(cookie == NULL) {
			dprintf("IDE PCI -- get_nth_cookie: no cookie for bus %ld\n", bus);
			return NULL;
		}
		cookie = cookie->next;
	}
	//dprintf("IDE PCI -- get_nth_cookie: cookie for bus %d is 0x%08x\n",
	//        bus, cookie);
	return cookie;
}

static uint32
get_bus_count()
{
	return buscount;
}

static int32
get_abs_bus_num(bus_info *bi)
{
	switch(bi->command_block_base_addr) {
		case 0x1f0:	return 0;
		case 0x170:	return 1;
		default:	return -1;
	}
}

static status_t
acquire_bus(bus_info *bi)
{
	status_t err;
#if 0
	err = acquire_sem_etc(bi->mutex, 1, B_CAN_INTERRUPT, 0);
	/* cache and filesystem does not handle interrupted read and write */
#else
	err = acquire_sem(bi->mutex);
#endif
	if(err == B_NO_ERROR)
		bi->buslock = true;
	return err;
}

static status_t
release_bus(bus_info *bi)
{
	bi->buslock = false;
	return release_sem_etc(bi->mutex, 1, B_DO_NOT_RESCHEDULE);
}

static status_t
write_command_block_regs(bus_info *bi, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = bi->command_block_base_addr;

	if(!bi->buslock) {
		dprintf("IDE PCI -- write_command_block_regs: bus not locked\n");
		return B_ERROR;
	}

	if((mask & ide_mask_command) && bi->dmainuse && bi->dma_started) {
		dprintf("IDE PCI -- write_command_block_regs: "
		        "error, only one command allowed\n"
		        "                                     "
		        "before calling finish_dma\n");
		return B_NOT_ALLOWED;
	}
	
	for(i=0; i<7; i++) {
		if((1<<i) & mask) {
			//dprintf("IDE PCI -- write_command_block_regs: "
			//        "write r[%d] = 0x%02x\n", i, tf->raw.r[i]);
			pci->write_io_8(ioaddr+1+i, tf->raw.r[i]);
		}
	}

	return B_NO_ERROR;
}

static status_t
read_command_block_regs(bus_info *bi, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = bi->command_block_base_addr;

	if(!bi->buslock) {
		dprintf("IDE PCI -- read_command_block_regs: bus not locked\n");
		return B_ERROR;
	}

	for(i=0; i<7; i++) {
		if((1<<i) & mask) {
			tf->raw.r[i] = pci->read_io_8(ioaddr+1+i);
			//dprintf("IDE PCI -- read_command_block_regs: "
			//        "read r[%d] = 0x%02x\n", i, tf->raw.r[i]);
		}
	}
	return B_NO_ERROR;
}

static uint8
get_altstatus(bus_info *bi)
{
	uint16 altstatusaddr = bi->control_block_register_addr;
	if(!bi->buslock) {
		dprintf("IDE PCI -- get_altstatus: bus not locked\n");
	}
	return pci->read_io_8(altstatusaddr);
}

static void
write_device_control(bus_info *bi, uint8 val)
{
	uint16 device_control_addr = bi->control_block_register_addr;
	if(!bi->buslock) {
		dprintf("IDE PCI -- write_device_control: bus not locked\n");
		return;
	}
	pci->write_io_8(device_control_addr, val);
}

static void
write_pio_16(bus_info *bi, uint16 *data, uint16 count)
{
	uint16 ioaddr = bi->command_block_base_addr;
	uint16 *dp = data;
	uint16 *ep = data+count;
	
	if(!bi->buslock) {
		dprintf("IDE PCI -- write_pio_16: bus not locked\n");
		return;
	}
	while(dp < ep)
		pci->write_io_16(ioaddr, *(dp++));
}

static void
read_pio_16(bus_info *bi, uint16 *data, uint16 count)
{
	uint16 ioaddr = bi->command_block_base_addr;
	uint16 *dp = data;
	uint16 *ep = data+count;
	
	if(!bi->buslock) {
		dprintf("IDE PCI -- read_pio_16: bus not locked\n");
		return;
	}
	while(dp < ep)
		*(dp++) = pci->read_io_16(ioaddr);
}

static status_t
intwait(bus_info *bi, bigtime_t timeout)
{
	status_t err;
//	int32 count = -99;
//	dprintf("IDE PCI -- intwait\n");
//	get_sem_count(bi->intsem, &count);
//	dprintf("IDE PCI -- intwait: semcount = %d, intgate = %d\n",
//	        count, bi->interrupt_gate);
	if(!bi->buslock) {
		dprintf("IDE PCI -- intwait: bus not locked\n");
		return B_ERROR;
	}
	err = acquire_sem_etc(bi->intsem,1,B_TIMEOUT,timeout);

	/* ---
		The Maxtor 7420AV (and maybe others?) seem to have some sort of
		watchdog timer, and if the host does not start doing something to
		the drive within 4-5 millisecs of an interrupt, it interrupts AGAIN!
		The interrupt_gate flag prevents getting more than one interrupt for
		the same 'event'.
	--- */

	if(err == B_NO_ERROR) {
		bi->interrupt_gate = 1;
//		dprintf("IDE PCI -- intwait: got int\n");
	}
	else {
//		dprintf("IDE PCI -- intwait: error = %s\n", strerror(err));
	}
//	dprintf("IDE PCI -- ide_intwait: done err=%x\n", err);
	return err;
}

static int32
inthand(bus_info *bus_info_p)
{
	uint16		sa = bus_info_p->busmaster_status_addr;
	
#if DEBUG_INTERRUPTS
	if(pci_ide_debug_interrupts)
		dprintf("IDE PCI -- ide_inthand: int for %x\n",
		        bus_info_p->command_block_base_addr);
#endif

	if(bus_info_p->shared_interrupt) {
		uint8	bmstatus = pci->read_io_8(sa);		// get status
		if(bmstatus & 4) {
			bus_info_p->busmasterstatus = bmstatus;	// save status
			pci->write_io_8(sa, bmstatus);	// clear interrupt and error bit
#if DEBUG_INTERRUPTS
			if(pci_ide_debug_interrupts)
				dprintf("IDE PCI -- ide_inthand: got int for %x, intgate %d\n",
				        bus_info_p->command_block_base_addr,
				        bus_info_p->interrupt_gate);
#endif
		}
		else {
#if DEBUG_INTERRUPTS
			if(pci_ide_debug_interrupts)
				dprintf("IDE PCI -- ide_inthand: "
				        "not for this bus %x (st = 0x%02x)\n",
				        bus_info_p->command_block_base_addr, bmstatus);
#endif
			return B_UNHANDLED_INTERRUPT;
		}
	}
	else {
		if(!bus_info_p->buslock) {
#if DEBUG_INTERRUPTS
			if(pci_ide_debug_interrupts)
				dprintf("IDE PCI -- ide_inthand: not for this bus %x\n",
				        bus_info_p->command_block_base_addr);
#endif
			return B_UNHANDLED_INTERRUPT;
		}
		if(bus_info_p->dmainuse) {
			uint16 ba = bus_info_p->busmaster_base_addr;
			/* stop bus master */
			pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) & 0xfe);
			/* get status */
			bus_info_p->busmasterstatus = pci->read_io_8(ba+2);
			/* clear interrupt and error bit */
			pci->write_io_8(ba+2, bus_info_p->busmasterstatus);
		}
	}
	/* read STATUS to clear interrupt */
	pci->read_io_8(bus_info_p->command_block_base_addr+7);

	if (bus_info_p->interrupt_gate) {		/* see comment in intwait() */
		bus_info_p->interrupt_gate = 0;
		release_sem_etc (bus_info_p->intsem,1,B_DO_NOT_RESCHEDULE);
	}
	else {
		return B_UNHANDLED_INTERRUPT;
	}
#if DEBUG_INTERRUPTS
	if(pci_ide_debug_interrupts)
		dprintf("IDE PCI -- ide_inthand: interrupt handled\n");
#endif
	return B_INVOKE_SCHEDULER;
}

#if __POWERPC__
static int32
dual_inthand(bus_info *bus_info_p)
{
	int32 rv;
	rv = inthand(bus_info_p);
	if(rv == B_UNHANDLED_INTERRUPT) {
		rv = inthand(bus_info_p->next);
	}
	return rv;
}
#endif

static size_t
update_prd_table(bus_info *bi, const iovec *vec, size_t veccount,
                 uint32 startbyte, uint32 blocksize, size_t size)
{
	const int 		max_table = 32;
	const uint32	max_size = 0x10000;	/* max size per PRD */
	const uint32	boundary = 0x10000;	/* regions cannot cross this boundary */

	physical_entry	p[max_table];		/* table of physical blocks */
	int				taken = 0;			/* # bytes set up in dma list so far */
	int				i;					/* loop counter */

	prd_t			*prd = bi->prd_table;
	prd_t			*prdend = bi->prd_table+(prd_table_size/sizeof(prd_t))-1; 

//dprintf("IDE PCI -- update_prd_table: vec 0x%08x, %d, startbyte %d, size %d\n",
//        vec, veccount, startbyte, size);

	for (;;) {
		while(veccount > 0 && vec->iov_len <= startbyte) {
			startbyte -= vec->iov_len;
			vec++;
			veccount--;
		}
		if(veccount == 0) {
			dprintf("IDE PCI -- update_prd_table: should not be here\n");
			return taken;
		}
		if((((uint32)vec->iov_base+startbyte) & bi->bad_alignment_mask) ||
		   ((vec->iov_len-startbyte) & bi->bad_alignment_mask)) {
			dprintf("IDE PCI -- update_prd_table: bad alignment\n");
			return taken;
		}
		get_memory_map((uint8*)vec->iov_base+startbyte,
		               min(vec->iov_len-startbyte, size), p, max_table);
		for (i = 0; i < max_table; i++) {			/* for each phys block */
			int	psize;		/* size current physical block */
			
			psize = p[i].size;
			startbyte += psize;
			if(psize == 0)
				break;
			while (psize > 0) {							/* for each PRD */
				uint32	paddress;
				int count;		/* # usable bytes from current block */

				paddress = (uint32)pci->ram_address(p[i].address);
				count = min (psize, max_size);
				
				/* now check we don't cross any restricted boundary */
				if ((paddress & ~(boundary-1)) !=
				    ((paddress+count-1) & ~(boundary-1)))
					 count = boundary - (paddress & (boundary-1));
				
				prd->address = B_HOST_TO_LENDIAN_INT32(paddress);
				/* map 64k xfer to 0 transfer_count */
				prd->byte_count = B_HOST_TO_LENDIAN_INT16(count & 0xfffe);
				
				//dprintf("IDE PCI -- update_prd_table: "
				//        "added addr 0x%08x, byte count 0x%04x\n",
				//        prd->address, prd->byte_count);

				taken += count;
				size -= count;

				if ( (size == 0) || (prd == prdend) ) {
					int strip = taken % blocksize;
					while(strip > 0) {
						if(prd->byte_count > strip) {
							//dprintf("IDE PCI -- update_prd_table: "
							//        "resized addr 0x%08x, byte count "
							//        "0x%04x to 0x%04x\n",
							//        prd->address, prd->byte_count,
							//		prd->byte_count-strip);
							prd->byte_count -= strip;
							taken -= strip;
							strip = 0;
						}
						else {
							//dprintf("IDE PCI -- update_prd_table: "
							//        "removed addr 0x%08x, byte count 0x%04x\n",
							//        prd->address, prd->byte_count);
							strip -= prd->byte_count;
							taken -= prd->byte_count;
							prd--;
						}
					}
					prd->eot = 1;
					return taken;
				}
				prd->eot = 0;

				prd++;
				p[i].address = (char *) p[i].address + count;
				psize -= count;
			}
		}
	}
}

static status_t
prepare_dma(bus_info *bi, const iovec *vec, size_t veccount, uint32 startbyte,
            uint32 blocksize, size_t *numBytes, bool to_device)
{
	uint16		ba = bi->busmaster_base_addr;
	uint32		reserved;
	size_t		newsize;

	if(!bi->buslock) {
		dprintf("IDE PCI -- prepare_dma: bus not locked\n");
		return B_ERROR;
	}
	if(ba == 0) {
		// not dma capable
		return B_NOT_ALLOWED;
	}
	
	//dprintf("IDE PCI -- prepare_dma: 0x%08x, size=%d\n", buffer, *size);
	if(bi->dmainuse) {
		dprintf("IDE PCI -- prepare_dma: already active\n");
		return B_ERROR;
	}
	bi->dmainuse = true;
	bi->dma_started = false;
	bi->busmasterstatus = 0;

	/* how many bytes can the next dma table really give us? */
	newsize = update_prd_table(bi, vec, veccount, startbyte,
	                           blocksize, *numBytes);
	if(*numBytes != newsize) {
		if(newsize < *numBytes && newsize > 0) {
			//dprintf("IDE PCI -- prepare_dma: Warning, PRD table exhausted\n");
		}
		else {
			dprintf("IDE PCI -- prepare_dma: Error, PRD table wrong\n");
			return B_ERROR;
		}
	}
	*numBytes = newsize;

	/* load the table address */
	reserved = B_LENDIAN_TO_HOST_INT32(pci->read_io_32(ba+0x04)) & 0x03;
	pci->write_io_32(ba+0x04,
	                 B_HOST_TO_LENDIAN_INT32(
	                    (uint32)pci->ram_address((void*)bi->prd_table_physical)
	                  | reserved));
	if(to_device) {
		/* set the bus master read mode  */
		pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) & 0xf7);
	}
	else {
		/* set the bus master write mode  */
		pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) | 0x8);
	}
#if 0 /* moved to inthand */
	/* clear int and err bits */
	pci->write_io_8(ba+0x02, pci->read_io_8(ba+0x02) | 0x06);
#endif
#if 0 /* moved to start_dma */
	/* launch the dma on the pci side */
	pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) | 0x01);
#endif
	return B_NO_ERROR;
}

static status_t
start_dma(bus_info *bi)
{
	uint16		ba = bi->busmaster_base_addr;
	//dprintf("IDE PCI -- start dma\n");

	if(!bi->dmainuse) {
		dprintf("IDE PCI -- start_dma: start what???\n");
		return B_ERROR;
	}

	bi->busmasterstatus = 1;
	bi->dma_started = true;
	/* launch the dma on the pci side */
	pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) | 0x01);
	return B_NO_ERROR;
}

static status_t
finish_dma(bus_info *bi)
{
	uint16	ba = bi->busmaster_base_addr;
	if(!bi->buslock) {
		dprintf("IDE PCI -- finish_dma: bus not locked\n");
		return B_ERROR;
	}
	if(ba == 0) {
		return B_NOT_ALLOWED;
	}
	if(!bi->dmainuse) {
		dprintf("IDE PCI -- finish_dma: finish what???\n");
		return B_ERROR;
	}
	bi->dmainuse = false;
	if(!bi->dma_started) {
		dprintf("IDE PCI -- finish_dma: dma was never started\n");
		return B_ERROR;
	}
	//dprintf("IDE PCI -- finish_dma\n");

	/*turn off the start/stop bit in the cmd reg. */
	pci->write_io_8(ba+0x0, pci->read_io_8(ba+0x0) & 0xfe);
	bi->dma_started = false;
	if(bi->busmasterstatus & 2)
		dprintf("IDE PCI -- finish_dma: memory transfer error\n");
		
	switch(bi->busmasterstatus & 0x5) {
		case 0:
			dprintf("IDE PCI -- finish_dma: DMA transfer failed\n");
			return B_ERROR;
		case 1:
			dprintf("IDE PCI -- finish_dma: DMA transfer aborted\n");
			return B_ERROR;
		case 4:
			return B_NO_ERROR;
	    case 5:
			// XXX what to do here?
			//dprintf("IDE PCI -- finish_dma: PRD size > device transfer size!\n");
			//return B_NO_ERROR;
			return B_INTERRUPTED;
	}
	return B_NO_ERROR;
}

static uint32
get_bad_alignment_mask(bus_info *bi)
{
	return bi->bad_alignment_mask;
}

static status_t
get_dma_mode(bus_info *bi, bool device1, uint32 *mode)
{
	if(bi->get_dma_mode)
		return bi->get_dma_mode(bi, device1, mode);
	return B_NOT_ALLOWED;
}

static status_t
set_dma_mode(bus_info *bi, bool device1, uint32 mode)
{
	return B_NOT_ALLOWED;
}

static uint32
recovery_time_to_dmamode(int recovery_time)
{
	if(recovery_time <= 1) {
		return 2;
	}
	else if(recovery_time <= 3) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Intel PIIX4 */
static status_t
get_dma_mode_piix4(bus_info *bi, bool device1, uint32 *mode)
{
	bool udma = false;
	uint16 idetim_reg =
		pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
		                     bi->pci_dev.function, bi->secondary ? 0x42 : 0x40,
		                     2);
	uint8 udmactl_reg =
		pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
		                     bi->pci_dev.function, 0x48, 1);
	
	udma = (udmactl_reg & ((device1 ? 2 : 1) << (bi->secondary ? 2 : 0))) != 0;
	
	if(udma) {
		uint16 udmatim_reg =
			pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
			                     bi->pci_dev.function, 0x4a, 2);
		*mode = 0x10 |
			((udmatim_reg >> (bi->secondary ? 8 : 0 + device1 ? 4 : 0)) & 3);
	}
	else {
		int recovery_time = 4;
		int sample_point = 5;
		if(device1) {
			if(idetim_reg & 0x4000) {
				uint8 sidetim_reg =
					pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
					                     bi->pci_dev.function, 0x44, 1);
				if(bi->secondary)
					sidetim_reg >>= 4;
				recovery_time = 4 - (sidetim_reg & 0x3);
				sample_point = 5 - ((sidetim_reg>>2) & 0x3);
			}
			else {
				recovery_time = 4 - ((idetim_reg>>8) & 0x3);
				sample_point = 5 - ((idetim_reg>>12) & 0x3);
			}
		}
		else {
			recovery_time = 4 - ((idetim_reg>>8) & 0x3);
			sample_point = 5 - ((idetim_reg>>12) & 0x3);
		}
		*mode = recovery_time_to_dmamode(recovery_time);
	}
	return B_NO_ERROR;
}

/* VIA 571 */
static status_t
get_dma_mode_via571(bus_info *bi, bool device1, uint32 *mode)
{
	int shift = (device1 ? 0 : 8) + (bi->secondary ? 0 : 16);

	uint8 timing_control =
		pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
		                     bi->pci_dev.function, 0x48, 4) >> shift;
	uint8 udma_timing_control =
		pci->read_pci_config(bi->pci_dev.bus, bi->pci_dev.device,
		                     bi->pci_dev.function, 0x50, 4) >> shift;
	
	if(udma_timing_control & 0x40) {
		int udma_mode = 2 - (udma_timing_control & 3);
		if(udma_mode < 0) {
			udma_mode = 0;
		}
		*mode = 0x10 | udma_mode;
	}
	else {
		*mode = recovery_time_to_dmamode(timing_control & 0xf);
	}
	
	return B_NO_ERROR;
}

/* -----
   create_prd_table_area creates and locks an area that will be used to store
   physical region descriptor tables. This area must be locked in physical
   memory to be accessed by dma controlers
----- */

static status_t
create_prd_table_area (bus_info *bi)
{
	status_t		err;
	physical_entry	prdt_physical_entry;

	bi->dmainuse = false;
	/* the area doesn't exist, try to create it */
	err = bi->prd_table_area =
		create_area( "PCI_IDE_PRD_TABLE_AREA", (void**)&bi->prd_table,
		             B_ANY_KERNEL_ADDRESS,
		             prd_table_size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(err < B_NO_ERROR) {
		dprintf("IDE PCI -- create_prd_table_area: "
		        "couldn't create dma table area\n");
		goto err0;
	}
	memset(bi->prd_table, 0, prd_table_size); // clear reserved bits

	err = lock_memory(bi->prd_table, prd_table_size, B_DMA_IO | B_READ_DEVICE);
	if(err != B_NO_ERROR) {
		dprintf("IDE PCI -- create_prd_table_area: "
		        "dma table couldn't be locked.\n");
		goto err1;
	}

	err = get_memory_map(bi->prd_table, prd_table_size, &prdt_physical_entry,1);
	if(err != B_NO_ERROR) {
		dprintf("IDE PCI -- create_prd_table_area: "
		        "dma table area created & locked,\n"
		        "                                  "
		        "couldn't get memory map\n");
		goto err2;
	}
	bi->prd_table_physical = (uint32)prdt_physical_entry.address;

	return B_NO_ERROR;

err2:
	unlock_memory(bi->prd_table, prd_table_size, B_DMA_IO | B_READ_DEVICE);
err1:
	delete_area(bi->prd_table_area);
err0:
	return err;
}

/* -----
   delete_prd_table_area deletes a PRD table area
----- */

static int
delete_prd_table_area (bus_info *bi)
{
	if(bi->prd_table_area < 0)
		return B_ERROR;
	unlock_memory(bi->prd_table, prd_table_size, B_DMA_IO | B_READ_DEVICE);
	return delete_area(bi->prd_table_area);
}

#if __INTEL__
static status_t
new_isa_device(uint16 command_block_base_addr,
               uint16 control_block_register_addr,
               int intnum)
{
	bus_info *newbus;
	status_t err;

	newbus = malloc(sizeof(bus_info));
	if(newbus == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}
	newbus->get_dma_mode = NULL;
	newbus->set_dma_mode = NULL;

	newbus->command_block_base_addr = command_block_base_addr;
	newbus->control_block_register_addr = control_block_register_addr;
	newbus->intnum = intnum;

	newbus->shared_interrupt = false;
	newbus->busmaster_base_addr = 0;
	newbus->busmaster_status_addr = 0;
	newbus->bad_alignment_mask = 0;
	newbus->prd_table_area = -1;
	newbus->dmainuse = false;
	newbus->mutex_shared = false;
	err = newbus->mutex = create_sem(1, "pciidebusmutex");
	if(err < B_NO_ERROR)
		goto err2;
	err = newbus->intsem = create_sem(0, "pciideintsem");
	if(err < B_NO_ERROR)
		goto err3;
	newbus->interrupt_gate = 1;
	newbus->handler = (interrupt_handler)inthand;
	err = install_io_interrupt_handler(newbus->intnum,
	                                   newbus->handler,
	                                   newbus, 0);
	if(err != B_NO_ERROR) {
		dprintf("IDE PCI -- new_isa_device: "
		        "could not install interrupt handler\n");
		goto err4;
	}
	newbus->buslock = true; // no need to lock the bus here
	write_device_control(newbus, 0x08); // enable interrupts
	newbus->buslock = false;

	// add bus
	newbus->next = busses;
	busses = newbus;
	buscount++;
	return B_NO_ERROR;
	
err4:
	delete_sem(newbus->intsem);
err3:
	delete_sem(newbus->mutex);
err2:
	free(newbus);
err1:
	dprintf("IDE PCI -- new_isa_device: failed to add ISA IDE bus\n");
	return err;
}

static status_t
add_isa_devices(bool primary_controller_found, bool secondary_controller_found)
{
	int isa_bus_count = 0;
	void *settingshandle;
	const char *val;
	status_t err = B_ERROR;
	
	settingshandle = load_driver_settings("ide");
	val = get_driver_parameter(settingshandle, "isa_bus_count", "-1", "-1");
	isa_bus_count = strtoull(val, NULL, 0);
	unload_driver_settings(settingshandle);
	if(isa_bus_count < 0) {
		if(!primary_controller_found)
			dprintf("IDE PCI -- add_isa_devices: "
			        "no primary PCI controller found, assume ISA exists\n");
		isa_bus_count = primary_controller_found ? 0 : 1;
	}

	if(isa_bus_count >= 1) {
		if(!primary_controller_found) {
			dprintf("IDE PCI -- add_isa_devices: add primary ISA controller\n");
			err = new_isa_device(0x1f0, 0x3f6, 14);
		}
	}
	if(isa_bus_count >= 2) {
		if(!secondary_controller_found) {
			status_t tmperr;
			dprintf("IDE PCI -- add_isa_devices: "
			        "add secondary ISA controller\n");
			tmperr = new_isa_device(0x170, 0x376, 15);
			if(tmperr != B_NO_ERROR && err == B_NO_ERROR)
				err = tmperr;
		}
	}
	if(buscount > 0)
		return B_NO_ERROR;
	else
		return err;
}
#endif

static status_t
find_devices()
{
	int			i;
	pci_info	h;
	status_t	err;
	bool		primary_controller_found = false;
	bool		secondary_controller_found = false;
	
	for(i=0; pci->get_nth_pci_info(i,&h) == B_NO_ERROR; i++ ) {
		uint32 full_id = h.vendor_id << 16 | h.device_id;
		bool is_ide_controller;
		
		switch(full_id) {
			case 0x11910005: /* acard ATP850UF */
			case 0x105a4d33: /* Promise FastTrack */
			case 0x105a4d38: /* Promise Ultra66 */
				is_ide_controller = true;
				break;
			default:
				is_ide_controller =
					h.class_base == 0x01 &&
					h.class_sub  == 0x01;
		}
		
		if(is_ide_controller) {
			bool brokendma = false;
			bool cannot_share_interrupt = false;
			bool busses_independent = true;
			uint32 dma_bad_alignment_mask = 0x1;

			bus_info *newbus[2];
			int i;

			for(i=0; i<2; i++) {
				newbus[i] = NULL;
			}
			for(i=0; i<2; i++) {
				newbus[i] = malloc(sizeof(bus_info));
				if(newbus[i] == NULL) {
					err = B_NO_MEMORY;
					goto err1;
				}
				newbus[i]->get_dma_mode = NULL;
				newbus[i]->set_dma_mode = NULL;
				newbus[i]->pci_dev.bus = h.bus;
				newbus[i]->pci_dev.device = h.device;
				newbus[i]->pci_dev.function = h.function;
				newbus[i]->secondary = i == 1;
			}

//			dprintf("IDE PCI -- find_devices: found PCI IDE controller, "
//			        "Vendor=0x%04x, Device=0x%04x, Revision=0x%02x\n",
//			        h.vendor_id, h.device_id, h.revision);
//			dprintf("IDE PCI -- find_devices: vendor = 0x%04x\n", );
//			dprintf("IDE PCI -- find_devices: device = 0x%04x\n", );
//			dprintf("IDE PCI -- find_devices: revision = 0x%04x\n", );

			switch(h.vendor_id) {
				case 0x1095: {
					dprintf("IDE PCI -- find_devices: found CMD ide "
					        "controller, disable interrupt sharing\n");
					cannot_share_interrupt = true;
				} break;
			}

			switch(full_id) {
				case 0x10395513: { /* SiS 5597/5598 */
					if(h.revision <= 5) {
						brokendma = true;
						dprintf("IDE PCI -- find_devices: "
						        "old SiS chipset, disable dma\n");
					}
					else {
						if(h.revision == 0xd0) {
							dprintf("IDE PCI -- find_devices: "
							        "SiS5597 or SiS5598 chipset\n");
						}
						else {
							dprintf("IDE PCI -- find_devices: SiS chipset\n");
						}
					}
				} break;
				
				case 0x80861230: { /* intel 82371FB (PIIX) */
					dprintf("IDE PCI -- find_devices: "
					        "intel 82371FB (PIIX) chipset\n");
				} break;
				
				case 0x80867010: { /* intel 82371SB (PIIX3) */
					dprintf("IDE PCI -- find_devices: "
					        "intel 82371SB (PIIX3) chipset\n");
				} break;
				
				case 0x80867111: { /* intel 82371AB (PIIX4) */
					dprintf("IDE PCI -- find_devices: "
					        "intel 82371AB (PIIX4) chipset\n");
					newbus[0]->get_dma_mode = get_dma_mode_piix4;
					newbus[1]->get_dma_mode = get_dma_mode_piix4;
				} break;
				
				case 0x11060571: { /* VIA */
					dprintf("IDE PCI -- find_devices: "
					        "VIA chipset (571)\n");
					newbus[0]->get_dma_mode = get_dma_mode_via571;
					newbus[1]->get_dma_mode = get_dma_mode_via571;
				} break;
				
				case 0x10b95229: { /* Ali Aladdin */
					dprintf("IDE PCI -- find_devices: "
					        "aladdin chipset\n");
					/* enable atapi dma if bios failed to do so */
					pci->write_pci_config(h.bus, h.device, h.function, 0x53, 1,
						pci->read_pci_config(h.bus, h.device, h.function,
						                     0x53, 1) | 0x01);
				} break;

				case 0x11910005: { /* acard ATP850UF */
					/* SIIG UltraIDE Pro PCI uses this chip */
					dprintf("IDE PCI -- find_devices: "
					        "found ACARD ATP850UF ide controller\n");
					h.class_api = 0x85;
					cannot_share_interrupt = true;
					dma_bad_alignment_mask = 0x7;
				} break;

				case 0x105a4d33: { /* Promise FastTrack */
					dprintf("IDE PCI -- find_devices: "
					        "found PROMISE FastTrack IDE RAID controller\n");
					h.class_api = 0x85;
					dma_bad_alignment_mask = 0x3;
				} break;

				case 0x105a4d38: { /* Promise PDC 20262 */
					dprintf("IDE PCI -- find_devices: "
					        "found PROMISE Ultra66 controller\n");
					h.class_api = 0x85;
					dma_bad_alignment_mask = 0x3;
				} break;

				case 0x10780102: { /* WEB PAD */
					dprintf("IDE PCI -- find_devices: "
					        "found WEB PAD IDE Controller\n");
					cannot_share_interrupt = true;
				} break;

				default: {
					if( (h.class_api & 0x80) == 0x80 ) {
						if(h.vendor_id == 0x1045) {
							brokendma = true;
							dprintf("IDE PCI -- find_devices: "
							        "OPTi chipset detected, disable dma\n");
						}
						else {
							dprintf("IDE PCI -- find_devices: "
							        "DMA capable IDE controller found\n");
						}
					}
					else {
						dprintf("IDE PCI -- find_devices: "
						        "IDE controller found\n");
					}
				}
			}

			if( (h.class_api & 0x1) == 0) {
				//dprintf("IDE PCI -- find_devices: "
				//        "IDE primary controller found\n");
				primary_controller_found = true;
				newbus[0]->command_block_base_addr = 0x1f0;
				newbus[0]->control_block_register_addr = 0x3f6;
				newbus[0]->intnum = 14;
			}
			else {
				newbus[0]->command_block_base_addr =
					h.u.h0.base_registers[0];
				newbus[0]->control_block_register_addr =
					h.u.h0.base_registers[1] + 2;
				newbus[0]->intnum = h.u.h0.interrupt_line;;
			}
			if( (h.class_api & 0x4) == 0) {
				//dprintf("IDE PCI -- find_devices: "
				//        "IDE secondary controller found\n");
				secondary_controller_found = true;
				newbus[1]->command_block_base_addr = 0x170;
				newbus[1]->control_block_register_addr = 0x376;
				newbus[1]->intnum = 15;
			}
			else {
				newbus[1]->command_block_base_addr =
					h.u.h0.base_registers[2];
				newbus[1]->control_block_register_addr =
					h.u.h0.base_registers[3] + 2;
				newbus[1]->intnum = h.u.h0.interrupt_line;;
			}

			newbus[0]->shared_interrupt = false;
			newbus[1]->shared_interrupt = false;
			if( !brokendma && ((h.class_api & 0x80) == 0x80) ) {
				dprintf("IDE PCI -- find_devices: controller supports DMA\n");
				newbus[0]->busmaster_base_addr = h.u.h0.base_registers[4];
				newbus[1]->busmaster_base_addr = h.u.h0.base_registers[4]+8;
				newbus[0]->busmaster_status_addr = h.u.h0.base_registers[4]+2;
				newbus[1]->busmaster_status_addr = h.u.h0.base_registers[4]+8+2;
				newbus[0]->bad_alignment_mask = dma_bad_alignment_mask;
				newbus[1]->bad_alignment_mask = dma_bad_alignment_mask;
				if(create_prd_table_area(newbus[0]) != B_NO_ERROR) {
					dprintf("IDE PCI -- find_devices: disabled dma\n");
					newbus[0]->prd_table_area = -1;
					newbus[0]->busmaster_base_addr = 0;
				}
				if(create_prd_table_area(newbus[1]) != B_NO_ERROR) {
					dprintf("IDE PCI -- find_devices: disabled dma\n");
					newbus[1]->prd_table_area = -1;
					newbus[1]->busmaster_base_addr = 0;
				}
				if(cannot_share_interrupt) {
					if(newbus[0]->intnum == newbus[1]->intnum) {
						busses_independent = false;
					}
				}
				else {
					newbus[0]->shared_interrupt = true;
					newbus[1]->shared_interrupt = true;
				}
				
				if(pci->read_io_8(h.u.h0.base_registers[4]) & 0x80) {
					/* simplex only */
					dprintf("IDE PCI -- find_devices: simplex controller\n");
					busses_independent = false;
				}
			}
			else {
				for(i=0; i<2; i++) {
					newbus[i]->busmaster_base_addr = 0;
					newbus[i]->busmaster_status_addr = 0;
					newbus[i]->prd_table_area = -1;
					newbus[i]->dmainuse = false;
					newbus[i]->bad_alignment_mask = 0;
				}
			}

			for(i=0; i<2; i++) {
				newbus[i]->mutex = -1;
				newbus[i]->mutex_shared = false;
				newbus[i]->buslock = false;
				newbus[i]->intsem = -1;
			}
			err = newbus[0]->mutex = create_sem(1, "pciidebusmutex");
			if(err < B_NO_ERROR)
				goto err2;
			if(busses_independent) {
				err = newbus[1]->mutex = create_sem(1, "pciidebusmutex");
				if(err < B_NO_ERROR)
					goto err2;
			}
			else {
				newbus[1]->mutex = newbus[0]->mutex;
				newbus[1]->mutex_shared = true;
			}
			for(i=0; i<2; i++) {
				err = newbus[i]->intsem = create_sem(0, "pciideintsem");
				if(err < B_NO_ERROR)
					goto err2;

				/* clear any pending interrupts */
				newbus[i]->buslock = true; // no need to lock the bus here
				write_device_control(newbus[i], 0x0a); // disable interrupts
				pci->read_io_8(newbus[i]->command_block_base_addr+7);
				newbus[i]->buslock = false;
			}
			for(i=1; i>=0; i--) {
				newbus[i]->interrupt_gate = 1;
				newbus[i]->buslock = true; // no need to lock the bus here
#if __POWERPC__
				if(newbus[0]->intnum == newbus[1]->intnum) {
					if(i==0) {
						newbus[i]->next = busses;
						newbus[i]->handler = (interrupt_handler)dual_inthand;
						err = install_io_interrupt_handler(newbus[i]->intnum,
						                                   newbus[i]->handler,
						                                   newbus[i], 0);
					}
					else {
						newbus[i]->handler = NULL;
						err = NULL;
					}
				}
				else
#endif
				{
					newbus[i]->handler = (interrupt_handler)inthand;
					err = install_io_interrupt_handler(newbus[i]->intnum,
				                                       newbus[i]->handler,
				                                       newbus[i], 0);
				}
				if(err != B_NO_ERROR) {
					dprintf("IDE PCI -- find_devices: "
					        "could not install interrupt handler\n");
					delete_sem(newbus[i]->mutex);
					delete_sem(newbus[i]->intsem);
					free(newbus[i]);
					dprintf("IDE PCI -- find_devices: skipped bus\n");
					continue;
				}
				write_device_control(newbus[i], 0x08); // enable interrupts
				newbus[i]->buslock = false;

				// add bus
				newbus[i]->next = busses;
				busses = newbus[i];
				buscount++;
				//dprintf("IDE PCI -- find_devices: added bus,\n"
				//        "                         cmdblk: 0x%04x\n"
				//        "                         ctlreg: 0x%04x\n",
				//        newbus[i]->command_block_base_addr,
				//        newbus[i]->control_block_register_addr);
			}

			continue;

err2:
			for(i=0; i<2; i++) {
				if(newbus[i]->mutex >= 0 && !newbus[i]->mutex_shared)
					delete_sem(newbus[i]->mutex);
				if(newbus[i]->intsem >= 0)
					delete_sem(newbus[i]->intsem);
			}
			delete_prd_table_area(newbus[0]);
			delete_prd_table_area(newbus[1]);
err1:
			for(i=0; i<2; i++) {
				if(newbus[i]) free(newbus[i]);
			}
			return err;
		}
	}
#if __INTEL__
	err = add_isa_devices(primary_controller_found, secondary_controller_found);
	return err;
#endif
	return B_NO_ERROR;

}


static status_t
init()
{
	status_t err;
	static status_t uninit();
	
	//dprintf("IDE PCI -- init\n");
	buscount = 0;
	busses = NULL;

	err = get_module(B_PCI_MODULE_NAME, (module_info **)&pci);
	if (err)
		return err;

	err = find_devices();
	if (err)
		goto err;

	//dprintf("IDE PCI -- init: done\n");
	return B_NO_ERROR;

err:
	uninit();
	return err;
}

static status_t
uninit()
{
	status_t err;
	
	//dprintf("IDE PCI -- uninit\n");
	if (pci)
		put_module(B_PCI_MODULE_NAME);
	while(busses != NULL) {
		bus_info *bus = busses;
		busses = bus->next;

		bus->buslock = true; // no need to lock the bus here
		write_device_control(bus, 0x0a); // disable interrupts
		bus->buslock = false;

		//dprintf("IDE PCI -- uninit: remove inthand %d\n", bus->intnum);
		if (bus->handler != NULL) {
			err = remove_io_interrupt_handler(bus->intnum,
		                                  bus->handler, bus);
			if(err != B_NO_ERROR) {
				dprintf("IDE PCI -- uninit: "
				        "remove inthand (int %d) failed for bus at 0x%x\n",
				        bus->intnum, bus->command_block_base_addr);
			}
		}
		if(bus->mutex >= 0 && !bus->mutex_shared)
			delete_sem(bus->mutex);
		if(bus->intsem >= 0)
			delete_sem(bus->intsem);
		if(bus->prd_table_area >= 0)
			delete_prd_table_area(bus);
		free(bus);
	}
#ifdef CHECKMALLOC
	my_memory_used();
#endif
	return B_NO_ERROR;
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		return -1;
	}
}

static status_t
rescan()
{
	return 0;
}

static ide_bus_info genericpci_module = {
	{
		{
			"busses/ide/genericpci/v0.6",
			0,
			&std_ops
		},
		&rescan
	},
	(uint32 (*) (uint32))						&get_nth_cookie,
	&get_bus_count,
	(int32 (*) (uint32))						&get_abs_bus_num,
	(status_t (*) (uint32))						&acquire_bus,
	(status_t (*) (uint32))						&release_bus,
	(status_t (*) (uint32,
	 ide_task_file*,ide_reg_mask mask))			&write_command_block_regs,
	(status_t (*)(uint32,
	 ide_task_file*,ide_reg_mask mask))			&read_command_block_regs,
	(uint8 (*) (uint32))						&get_altstatus,
	(void (*) (uint32,uint8))					&write_device_control,
	(void (*) (uint32,uint16*,uint16))			&write_pio_16,
	(void (*) (uint32,uint16*,uint16))			&read_pio_16,
	(status_t (*) (uint32,bigtime_t))			&intwait,
	(status_t (*)(uint32,const iovec *,size_t,
	              uint32,uint32,size_t*,bool))	&prepare_dma,
	(status_t (*)(uint32))						&start_dma,
	(status_t (*)(uint32))						&finish_dma,
	(uint32 (*)(uint32))						&get_bad_alignment_mask,
	(status_t (*)(uint32,bool,uint32*))			&get_dma_mode,
	(status_t (*)(uint32,bool,uint32))			&set_dma_mode
};

#ifdef __ZRECOVER
# include <recovery/module_registry.h>
  REGISTER_STATIC_MODULE(genericpci_module);
#else
# if !_BUILDING_kernel && !BOOT
 _EXPORT 
  ide_bus_info *modules[] = {
	&genericpci_module,
	NULL
 };
# endif
#endif
