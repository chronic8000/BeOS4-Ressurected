/* ++++++++++
	fx_driver.c

This is a small driver to return information on the PCI bus and allow
an app to find memory mappings for devices. The only really complicated
part of this driver is the code to switch back to VGA mode in uninit,
largely this code is taken from the Glide Sources

+++++ */

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <graphics_p/common/MemMgr/MemMgrKrnl.h>
#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include "driver.h"

/*------------------------------------------------------------------------*/

#define DEVICE_NAME 		"radeon/"
#define NUM_OPENS			10		  // Max. number concurrent opens for device
#define GET_NTH_PCI_INFO	(*pci_bus->get_nth_pci_info)

typedef struct
{
	uint32 CardNumber;
	uint32 MemMgrKey;
	area_id AreaPciMem;
	area_id AreaPciMemNC;
	uint32 *pciBuffer;
	uint32 *pciBufferNC;
	uint32 gartOffset;
} per_open_data;

_EXPORT status_t init_hardware (void);
_EXPORT status_t init_driver (void);
_EXPORT void uninit_driver (void);
_EXPORT const char **publish_devices (void);
_EXPORT device_hooks *find_device (const char *);

static long fx_open (const char *name, ulong flags, void **cookie);
static long fx_close (void *cookie);
static long fx_free (void *cookie);
static long fx_control (void *cookie, ulong msg, void *buf, size_t len);
static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes);
static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes);

static int radeondump (int argc, char **argv);

extern uint8 SetVideoMode (uint32 xRes, uint32 yRes, uint32 refresh, uint32 loadClut);

pci_module_info *pci_bus;
DriverInfo radeon_di;

static spinlock splok;
//static uint32 ncalls;
uint32 io_base_offset;

#if 0
static int32 num_cards;
static pci_info pciinfo[MAX_CARDS];	/* pci info */
static area_id thdfx_area[MAX_CARDS];	/* mapped Bt848 registers */
static area_id thdfx_wc_area[MAX_CARDS];	/* mapped Bt848 registers (with Write-Combining) */
static area_id thdfx_uc_area[MAX_CARDS];	/* mapped Bt848 registers (with Uncacheable) */
static uint32 *thdfx_base[MAX_CARDS];
static char *thdfx_name[MAX_CARDS + 1];	/* device name list */
static char names[MAX_CARDS + 1][32];
static int32 open_count[MAX_CARDS];	/* # opens we can do */
static long open_lock[MAX_CARDS];	/* spinlock for open/close */
static uint32 slot[MAX_CARDS];  /* PCI Slot Number */
static uint32 *linear_address[MAX_CARDS];	/* Linear address for cards being mapped */
#endif

//static uint32      p6FenceVar;

static device_hooks thdfx_device_hooks = {
	fx_open,
	fx_close,
	fx_free,
	fx_control,
	fx_read,
	fx_write
};

void resetPassThru (int board_num);

/****************************************************************************
 * Map device memory into local address space.
 */
static status_t map_device ( DeviceInfo *di )
{
	status_t retval;
	int32 len;
	uint32 temp;
	char name[B_OS_NAME_LENGTH];

	dprintf (("Radeon: map_device - ENTER\n"));
	
	/*  Create a goofy basename for the areas  */
	sprintf (name, "%04X_%04X_%02X%02X%02X", di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	len = strlen (name);
	strcpy( di->ci->devname, name );

	/* Map registers. */
	dprintf(("Radeon: map_device - Mapping Registers, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[2],  di->PCI.u.h0.base_register_sizes[2]));
	strcpy (name + len, " regs");
	retval = map_physical_memory (name, (void *) di->PCI.u.h0.base_registers_pci[2],
								di->PCI.u.h0.base_register_sizes[2], B_ANY_KERNEL_ADDRESS,
								B_READ_AREA | B_WRITE_AREA, (void **) &di->ci->CardRegs );
								
	if (retval < 0)
		goto err_base0;			  /*  Look down  */
	di->AreaRegs = retval;
	dprintf(( "Radeon: map_device - Mapping Registers, di->AreaRegs = 0x%x\n   CardRegs= 0x%x", di->AreaRegs, di->ci->CardRegs ));

	
	// Before we can determine the virtual address of the frame buffer,
	// we need to know how much memory is installed.  Read CONFIG_MEMSIZE
	temp = *((uint32 *)(di->ci->CardRegs + CONFIG_MEMSIZE));
	dprintf(( "Radeon: CONFIG_MEMSIZE = %x \n", temp ));
	
	// the memory size is in bits [28:0], so we'll mask off [31:29]
	di->info.memsize = temp & 0x1FFFFFFF;
	dprintf(( "Radeon: Memory size %ld \n", di->info.memsize ));

	/* Map framebuffer RAM */
	dprintf(("Radeon: map_device - Mapping Frame Buffer, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[0],  di->PCI.u.h0.base_register_sizes[0]));
	strcpy (name + len, " framebuffer");
	retval = map_physical_memory (name, (void *) di->PCI.u.h0.base_registers_pci[0],
									di->PCI.u.h0.base_register_sizes[0],
#ifdef __INTEL__
									B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
									B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
									B_READ_AREA | B_WRITE_AREA, (void **) &di->ci->CardMemory);
#ifdef __INTEL__
	if (retval < 0)
	{
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		retval = map_physical_memory( name, (void *) di->PCI.u.h0.base_registers_pci[1],
			di->PCI.u.h0.base_register_sizes[1],
			B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void **) &di->ci->CardMemory );
	}
#endif

	if (retval < 0)
		goto err_base1;			  /*  Look down  */

	di->AreaFrameBuffer = retval;
	dprintf(( "Radeon: map_device - Mapping Frame Buffer, di->AreaFrameBuffer = 0x%x\n", di->AreaFrameBuffer ));
	dprintf(( "Radeon: map_device - Mapping Frame Buffer, di->ci->CardMemory = %p\n", di->ci->CardMemory ));

	// Store the offset for the I/O registers
	di->ci->IO_Base = di->PCI.u.h0.base_registers_pci[2];

	if (retval < 0)
	{
		delete_area (di->AreaFrameBuffer);
		di->AreaFrameBuffer = -1;
		di->ci->CardMemory = NULL;
	 err_base1:
		delete_area (di->AreaRegs);
		di->AreaRegs = -1;
		di->ci->CardRegs = NULL;
	 err_base0:
	dprintf(("Radeon: map_device - Error Mapping Device returning 0x%x\n", retval));
		return (retval);
	}

	// Store the PCI register values
//	for (i = 0; i <= 2; i++)
//	{
//		di->ci->ci_base_registers_pci[i] = (uint8 *) di->PCI.u.h0.base_registers_pci[i];
//	}
	dprintf(("Radeon: map_device - EXIT\n"));
	return (B_OK);
}

static void unmap_device( DeviceInfo *di)
{
	dprintf (("Radeon: unmap_device - ENTER di->name =  %s\n", di->Name));

	if (di->AreaFrameBuffer >= 0)
	{
		delete_area (di->AreaFrameBuffer);
		di->AreaFrameBuffer = -1;
		di->ci->CardMemory = NULL;
	}
	if (di->AreaRegs >= 0)
	{
		delete_area (di->AreaRegs);
		di->AreaRegs = -1;
		di->ci->CardRegs = NULL;
	}

	dprintf(("Radeon: unmap_device - EXIT\n"));
}


/*
 * This is actually a misnomer; very little initialization is done here; it
 * just detects if there's a card installed that we can handle.
 */
status_t init_hardware (void)
{
	int8 found_one;
	dprintf (( "Radeon: init_hardware \n" ));
	found_one = Card_Detect();
	return (found_one ? B_OK : B_ERROR);
}

/*------------------------------------------------------------------------*/

status_t init_driver (void)
{
	dprintf(( "Radeon: init_driver - Enter \n" ));

	/*  Find all of our supported devices  */
	if( !Radeon_Probe() )
		return B_ERROR;

	radeon_di.LockSem = create_sem( 1, "Radeon Driver Sem" );
	if( radeon_di.LockSem < B_OK )
		return B_ERROR;

	/*  Initialize spinlock  */
	splok = 0;

	dprintf (("Radeon: init_driver - EXIT\n"));
	return (B_OK);
}

#if 0
/****************************************************************************
 * Interrupt management.
 */
static void clearinterrupts (register struct thdfx_card_info *ci)
{
	/*
	 * Enable VBLANK interrupt (and *only* that).
	 */
	ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl &= ~(1 << 8);	/*  Clear Vertical Sync (Rising Edge) Interrupt generated flag  */
	ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl |= 1 << 31;	/*  Write '1' to Extern pin pci_inta (active  low) to clear external PCI Interrupt  */
}

static void enableinterrupts (register struct thdfx_card_info *ci, int enable)
{
// dprintf(("Radeon: enableinterrupts - ENTER ci = 0x%x, enable = %d\n", ci, enable));
	if (enable)
		ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl |= 1 << 2;	/* Enable Vertical Sync (Rising Edge) Interrupts */
	else
		ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl &= ~(1 << 2);
// dprintf(("Radeon: enableinterrupts - EXIT\n"));
}

static int32 thdfx_interrupt (void *data)
{
	register devinfo *di;
	register thdfx_card_info *ci;
	register uint32 bits;
	bool resched;

	resched = false;

	di = data;
	ncalls++;
	if (!(di->di_IRQEnabled))
		return (B_UNHANDLED_INTERRUPT);

	ci = di->di_thdfxCardInfo;
	bits = ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl;

	if (!(bits & (1 << 8)))
		return (B_UNHANDLED_INTERRUPT);	/* Not a Vertical Sync Interrupt */

	/*  Clear the interrupt bit.  */
	ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl = (bits & (~(1 << 8))) | (1 << 31);

	if (thdfxdd)
		thdfxdd->dd_NInterrupts++;
	di->di_NInterrupts++;
	di->di_LastIrqMask = bits;

	/*
	 * Most common case (not to mention the only case) is VBLANK;
	 * handle it first.
	 */
	if (bits && (1 << 8))
	{
		register int32 *flags;

		di->di_NVBlanks++;
		flags = &ci->ci_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE)
		{
			/*
			 * Change the framebuffer base address here.
			 */
			uint32 fb_offset, stride;
			uint32 regBase;

			if (ci->ci_DisplayAreaX == 0)
			{
				regBase = io_base_offset;
				fb_offset = (uint32) ci->ci_FBBase - (uint32) ci->ci_BaseAddr1;
				stride = ci->ci_BytesPerRow;
				fb_offset += ci->ci_DisplayAreaY * stride;
				ISET32 (vidDesktopStartAddr, (fb_offset & SST_VIDEO_START_ADDR) <<
						  SST_VIDEO_START_ADDR_SHIFT);
			}
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR)
		{
			/*
			 * Move the hardware cursor.
			 */
			uint32 regBase = io_base_offset;
			ISET32 (hwCurLoc,
					  ((ci->ci_MousePosX - ci->ci_MouseHotX +
						 64) << SST_CURSOR_X_SHIFT) | ((ci->ci_MousePosY - ci->ci_MouseHotY +
																  64) << SST_CURSOR_Y_SHIFT));
		}

		/*  Release the vblank semaphore.  */
		if (ci->ci_VBlankLock >= 0)
		{
			int32 blocked;

			if (get_sem_count (ci->ci_VBlankLock, &blocked) == B_OK && blocked < 0)
			{
				release_sem_etc (ci->ci_VBlankLock, -blocked, B_DO_NOT_RESCHEDULE);
				resched = true;
			}
		}
	}

#if 0
	if (bits)
	{
		/*  Where did this come from?  */
		dprintf (
					("Radeon: thdfx_interrupt -  Weird Radeon interrupt (tell awk): bits = 0x%08lx\n",
					 bits));
	}
#endif
	return resched ? (B_INVOKE_SCHEDULER) : (B_HANDLED_INTERRUPT);
}
#endif

static status_t initGART( DeviceInfo *di, per_open_data *pod )
{
//	CardInfo *ci = di->ci;
	int32 n = (( PRIMARY_PCI_MEM_SIZE + B_PAGE_SIZE - 1) / B_PAGE_SIZE) +1;
	physical_entry *pe;
	int32 i,k;
	int32 off = pod->gartOffset / 4096;
	
dprintf(("Radeon: initGART - n=%ld, off=%ld \n", n, off ));
	pe = malloc( n * sizeof(physical_entry) );
	if( !pe )
		return B_ERROR;

//dprintf(("Radeon: initGART - pe=%p \n", pe ));
	get_memory_map( pod->pciBuffer, PRIMARY_PCI_MEM_SIZE, pe, n );
	
	i=0;
	while( (i < n) && (pe[i].size > 0) )
	{
//dprintf(("Radeon: initGART entry %ld  %p, %ld \n", i, pe[i].address, pe[i].size ));
		i++;
	}

	if( i == n )
	{
		// Should never happen because our table is pages+1 in size
		free(pe);
		return B_ERROR;
	}
	
//dprintf(("Radeon: initGART map returned ok \n" ));
	i=0;
	k=0;
	while( k < (n-1) )
	{
		di->Gart[k+off] = (uint32) pe[i].address;
//		dprintf(( "Gart[0x%x] = %p \n", k+off, di->Gart[k+off] ));
		k++;
		pe[i].size -= 4096;
		((uint8 *)pe[i].address) += 4096;
		if( !pe[i].size )
			i++;
	}
	
	free(pe);
//dprintf(("Radeon: initGART ok \n" ));
	return B_OK;
	
}

static void destroyDMABuffer( DeviceInfo *di, per_open_data *pod )
{
	if( pod->AreaPciMem >= 0 )
		delete_area( pod->AreaPciMem );
	if( pod->AreaPciMemNC >= 0 )
		delete_area( pod->AreaPciMemNC );
	pod->AreaPciMem = -1;
	pod->AreaPciMemNC = -1;
	pod->pciBuffer = 0;
	pod->pciBufferNC = 0;
}

static status_t createDMABuffer( DeviceInfo *di, per_open_data *pod, int32 size )
{
	status_t retval;
	physical_entry entry;
	int n;
	uint32 off;

	n = ( (size*2) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area ("Radeon DMA buffer",
					(void **) &pod->pciBuffer,
					B_ANY_KERNEL_ADDRESS,
					n * B_PAGE_SIZE,
					B_FULL_LOCK | B_CONTIGUOUS,
					B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		dprintf(("Radeon: createDMABuffer - Failed to create area \n", di));
		destroyDMABuffer( di, pod );
		return B_ERROR;
	}
	pod->AreaPciMem = retval;

	// Lets do out fancy 2^n alignment
	get_memory_map (pod->pciBuffer, 4096, &entry, 1);
	off = (uint32)entry.address;
	off = (off + size-1) & ((0xffffffff) - (size-1));
	off = off - ((uint32)entry.address);

	((uint8 *)entry.address) += off;
	pod->pciBuffer += off/4;
	
	/*  Create uncached mapping for pci DMA buffer image.  (Uh, Cyril...)  */
	retval = map_physical_memory ("Radeon DMA buffer WC mapping",
								(void *) entry.address,
								size,
								B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
								B_READ_AREA | B_WRITE_AREA,
								(void **) &pod->pciBufferNC );
	if (retval < 0)
	{
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		dprintf(("Radeon: createDMABuffer - WC map failed \n"));
		retval = map_physical_memory("Radeon DMA buffer Non-WC",
								(void *)entry.address,
								size,
								B_ANY_KERNEL_BLOCK_ADDRESS,
								B_READ_AREA | B_WRITE_AREA,
								(void **) &pod->pciBufferNC );
	}

	if (retval < 0)
	{
		dprintf(("Radeon: createDMABuffer - Non-WC map failed \n"));
		destroyDMABuffer( di, pod );
		return B_ERROR;
	}

	pod->AreaPciMemNC = retval;

	return B_OK;
}


/****************************************************************************
 * Device hook (and support) functions.
 */
static status_t firstopen( DeviceInfo *di, per_open_data *pod )
{
//	thread_id thid;
//	thread_info thinfo;
	CardInfo *ci;
	physical_entry entry;
	status_t retval = B_OK;
	int n;
//	uint32 off;
	char shared_name[B_OS_NAME_LENGTH];

dprintf(("Radeon: firstopen - ENTER - di = 0x%08lx\n", di));

dprintf (("XXXXXXXXXX 2 radeon_di.Device[0].isMobility %x \n", radeon_di.Device[0].isMobility));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	n = (sizeof (CardInfo) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name, (void **) &di->ci,
		B_ANY_KERNEL_ADDRESS, n * B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		dprintf(("Radeon: firstopen - Failed to create card info area \n", di));
		return (retval);
	}
	di->AreaCardInfo = retval;
	di->videoOverideID = -1;
	ci = di->ci;
	memset( ci, 0, sizeof (CardInfo) );
	
	memcpy( &ci->pll, &di->pll, sizeof( ci->pll ) );
	ci->isMobility = di->isMobility;
	dprintf(("Radeon: isMobility %x %x\n",ci->isMobility,di->isMobility));

#if 0
	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (ci, thinfo.team)) < 0)
	{
//    dprintf(("Radeon: firstopen - init_locks returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->di_thdfxCardInfo_AreaID >= 0)
		{
			delete_area (di->di_thdfxCardInfo_AreaID);
			di->di_thdfxCardInfo_AreaID = -1;
			di->di_thdfxCardInfo = NULL;
		}
	}
#endif


dprintf(("Radeon: firstopen mapping device \n" ));
	/*  Map the card's resources into memory.  */
	if ((retval = map_device (di)) < 0)
	{
		dprintf(("Radeon: firstopen map_device() returned 0x%x\n", retval));
//		dispose_locks (ci);
		if (di->AreaCardInfo >= 0)
		{
			delete_area (di->AreaCardInfo);
			di->AreaCardInfo = -1;
			di->ci = NULL;
			return B_ERROR;
		}
	}

	// Initialize ci_regs so that the interrupt handler works correctly
//	ci->ci_Regs = (thdfxRegs *) ci->ci_BaseAddr0;

#if 0
	/*  Install interrupt handler  */
	if ((di->di_PCI.u.h0.interrupt_line != 0) &&
		 (di->di_PCI.u.h0.interrupt_line != 0xff) &&
		 ((retval
			=
			install_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line, thdfx_interrupt, (void *) di,
													0)) == B_OK))
	{
//    dprintf(("Radeon: firstopen - install_io_interrupt_handle() returned 0x%x, set IRQF__ENABLED\n", retval));
		ci->ci_IRQFlags |= IRQF__ENABLED;
	}
	else
	{
//    dprintf(("Radeon: firstopen - install_io_interrupt_handle() returned 0x%x, not setting IRQF__ENABLED\n", retval));
		retval = B_OK;
	}

	clearinterrupts (ci);

	/* notify the interrupt handler that this card is a candidate */
	if (ci->ci_IRQFlags & IRQF__ENABLED)
	{
		di->di_IRQEnabled = 1;
		enableinterrupts (ci, TRUE);
	}

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->ci_DevName, di->di_Name);

#endif

dprintf(("Radeon: 2 \n"));

	/*
	 * Create the Gart area.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X GART",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	n = ((32 * 1024) + B_PAGE_SIZE -1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name, (void **) &di->Gart,
		B_ANY_KERNEL_ADDRESS, n * B_PAGE_SIZE,
		B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		dprintf(("Radeon: firstopen - Failed to create GART area \n", di));
		return (retval);
	}
	di->AreaGart = retval;
	memset( di->Gart, 0, 32 * 1024 );

dprintf(("Radeon: 3 \n"));

	if( createDMABuffer( di, pod, PRIMARY_PCI_MEM_SIZE ) < B_OK )
	{
		return B_ERROR;
	}
	di->ci->pciMemBase = pod->pciBufferNC;
	pod->gartOffset = 0;
	initGART( di, pod );	

	dprintf(("Radeon: initGART enabling \n" ));
	get_memory_map( di->Gart, 4096, &entry, 1 );
	WRITE_REG_32( AIC_PT_BASE, ((uint32)entry.address) & 0xFFFFF000 );
	WRITE_REG_32( AIC_LO_ADDR, CARD_PCI_BASE & 0xfffff000 );
	WRITE_REG_32( AIC_HI_ADDR, (CARD_PCI_BASE + (8 * 1024 * 1024) -1) & 0xfffff000 );
	WRITE_REG_32( AIC_CTRL, 1 );

#if 0
	/*
	 * Create the RingBuffer area.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X ring",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	n = ( (PRIMARY_PCI_MEM_SIZE*2) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name, (void **) &di->pciBuffer,
		B_ANY_KERNEL_ADDRESS, n * B_PAGE_SIZE,
		B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0)
	{
		dprintf(("Radeon: firstopen - Failed to create RingBuffer area \n", di));
		return (retval);
	}
	di->AreaPciMem = retval;

	// Lets do out fancy 2^n alignment
	get_memory_map (di->pciBuffer, 4096, &entry, 1);
	off = (uint32)entry.address;
	off = (off + PRIMARY_PCI_MEM_SIZE-1) & ((0xffffffff) - (PRIMARY_PCI_MEM_SIZE-1));
	off = off - ((uint32)entry.address);

	((uint8 *)entry.address) += off;
	di->pciBuffer += off/4;

dprintf(("Radeon: firstopen - pciBuffer=%p  size=%x \n", di->pciBuffer, PRIMARY_PCI_MEM_SIZE ));
	memset( di->pciBuffer, 0, PRIMARY_PCI_MEM_SIZE );

dprintf(("Radeon: firstopen - calling initGART  gart=%p  pciMem=%p \n", di->Gart, di->pciBuffer));

	/*  Create uncached mapping for pci DMA buffer image.  (Uh, Cyril...)  */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X ring 2nd map",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	retval = map_physical_memory (shared_name, (void *) entry.address, PRIMARY_PCI_MEM_SIZE,
						B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
				      B_READ_AREA | B_WRITE_AREA, (void **) &di->ci->pciMemBase);
	if (retval < 0)
	{
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
dprintf(("Radeon: firstopen - WC map failed \n"));
		retval = map_physical_memory( shared_name, (void *)entry.address,
			PRIMARY_PCI_MEM_SIZE,
			B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void **) &di->ci->pciMemBase );
	}

//	if (retval < 0)
//		goto err;	/*  Look down  */
	di->AreaPciMemNC = retval;
#endif

dprintf(("Radeon: firstopen Creating memMgr area \n" ));
	/* Create the memMgr area */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X MemMgr",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	if (__mem_Initialize (&di->ma, 32 * 1024 * 1024, 4096, shared_name) < B_OK)
	{
		dprintf(("Radeon: firstopen __mem_Initialize failed! \n" ));
		retval = B_ERROR;
		di->ma = 0;
//		ci->ci_memArea = -1;
	}
	else
	{
		dprintf(("Radeon: firstopen __mem_Initialize area = 0x%x \n", di->ma->areaID ));
//		ci->ci_memArea = di->ma->areaID;
//		di->ma->baseOffset = (uint32) di->ci->CardMemory;
	}

	for( n=1; n<16; n++ )
		di->pciBufferMap[n] = 0;

	/*  All done, return the status.  */
	dprintf(("Radeon: firstopen returning B_OK\n"));
	return B_OK;
}

static void lastclose( DeviceInfo *di)
{
// dprintf(("Radeon: lastclose - ENTER\n"));

//	di->di_IRQEnabled = 0;

//	enableinterrupts (di->di_thdfxCardInfo, FALSE);
//	clearinterrupts (di->di_thdfxCardInfo);
//	remove_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line, thdfx_interrupt, di);

	unmap_device (di);
//	dispose_locks (di->di_thdfxCardInfo);

//	delete_area (di->di_thdfxCardInfo_AreaID);
//	di->di_thdfxCardInfo_AreaID = -1;
//	di->di_thdfxCardInfo = NULL;

// dprintf(("Radeon: lastclose - EXIT\n"));
}

static long fx_open (const char *name, ulong flags, void **cookie)
{
	int n;
	DeviceInfo *di;
	status_t retval = B_OK;
	per_open_data *pod;

	dprintf (("\nRadeon: fx_open - ENTER, name is %s  \n", name ));

	acquire_sem( radeon_di.LockSem );

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0; radeon_di.DevNames[n] && strcmp (name, radeon_di.DevNames[n]); n++)
		;
	di = &radeon_di.Device[n];
	dprintf (("\nRadeon: fx_open - n=%ld  opened=%ld \n", n, di->Opened ));

	/*  Squirrel this away.  */
	pod = (per_open_data *) malloc (sizeof (per_open_data));
	*cookie = pod;
	pod->CardNumber = n;
	pod->AreaPciMem = -1;
	pod->AreaPciMemNC = -1;
	pod->pciBuffer = 0;

	if (di->Opened == 0)
	{
		if ((retval = firstopen (di,pod)) != B_OK)
		{
			dprintf(( "Radeon: fx_open - firstopen() != B_OK !\n" ));
			goto err;
		}
	}

	di->ci->mc_Users ++;

	/*  Mark the device open.  */
	di->Opened++;

	pod->MemMgrKey = __mem_GetNewID (di->ma);
	if( di->Opened == 1 )
		di->pciBufferMap[0] = pod->MemMgrKey;

	/*  End of critical section.  */
 err:
	release_sem( radeon_di.LockSem );

	/*  All done, return the status.  */
	dprintf(("Radeon: fx_open - returning 0x%08x\n", retval));
	return (retval);
}

static long fx_close (void *_cookie)
{
//	per_open_data *pod = (per_open_data *) _cookie;
//	devinfo *di = pod->di;

	dprintf(("Radeon: fx_close \n"));
	//put_module(pci_name);

	return B_NO_ERROR;
}

static long fx_free (void *cookie)
{
	per_open_data *pod = (per_open_data *) cookie;
	DeviceInfo *di = &radeon_di.Device[pod->CardNumber];
	CardInfo *ci = di->ci;
	int32 n;

dprintf(("Radeon: fx_free - pod->MemMgrKey = %ld \n", pod->MemMgrKey ));

	acquire_sem( radeon_di.LockSem );
	__mem_RemoveIndex (di->ma, pod->MemMgrKey);
	di->ci->mc_Users --;

	if (di->videoOverideID == pod->MemMgrKey)
	{
		Radeon_SetDisplayMode( di, &di->mode_Primary );
		WRITE_REG_32( CRTC_OFFSET, ci->AllocFrameBuffer->address );
		di->videoOverideID = -1;
	}
	
	for( n=0; n<16; n++ )
	{
		if( di->pciBufferMap[n] == pod->MemMgrKey )
		{
dprintf(("Radeon: fx_free - destroyDMABuffer n=%ld \n", n));
			destroyDMABuffer( di, pod );
			di->pciBufferMap[n] = 0;
			break;
		}
	}

	/*  Mark the device available.  */
	if (--di->Opened == 0)
		lastclose (di);

	release_sem( radeon_di.LockSem );
dprintf(("Radeon: fx_free - EXIT\n"));

	return (B_OK);
}

static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes)
{
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes)
{
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static long fx_control (void *_cookie, ulong msg, void *buf, size_t len)
{
	per_open_data *pod = (per_open_data *) _cookie;
	DeviceInfo *di = &radeon_di.Device[pod->CardNumber];
	CardInfo *ci = di->ci;

//	thdfx_card_info *ci = di->di_thdfxCardInfo;
	status_t retval = B_DEV_INVALID_IOCTL;

dprintf(( "Radeon: fx_control - ENTER  msg = %x \n", msg ));

	switch (msg)
	{
		/*
		 * Stuff the accelerant needs to do business.
		 */
		/*
		 * The only PUBLIC ioctl.
		 */
	case B_GET_ACCELERANT_SIGNATURE:
		dprintf(("Radeon: fx_control - B_GET_ACCELERANT_SIGNATURE\n"));
		strcpy ((char *) buf, "radeon.accelerant");
		retval = B_OK;
		break;

	case B_GET_3D_SIGNATURE:
		dprintf (("Radeon: fx_control - B_GET_3D_SIGNATURE\n"));
		strcpy ((char *) buf, "radeon.3da");
		retval = B_OK;
		break;


	case RADEON_IOCTL_MODE_COUNT:
		((uint32 *)buf)[0] = Radeon_GetModeCount(di);
		retval = B_OK;
		break;
		
	case RADEON_IOCTL_GET_MODE_LIST:
		memcpy (buf, Radeon_GetModeList(di),
			Radeon_GetModeCount(di) * sizeof (display_mode));
		retval = B_OK;
		break;
		
	case RADEON_IOCTL_RESET_VIDEO_MODE:
		dprintf(("RADEON_IOCTL_RESET_VIDEO_MODE \n" ));
		Radeon_SetDisplayMode( di, &di->mode_Primary );
		WRITE_REG_32( CRTC_OFFSET, ci->AllocFrameBuffer->address );
		di->videoOverideID = -1;
		retval = B_OK;
		dprintf(("RADEON_IOCTL_RESET_VIDEO_MODE OK\n" ));
		break;

	case RADEON_IOCTL_PROPOSE_DISPLAY_MODE:
		break;

	case RADEON_IOCTL_GET_DMA_BUFFER:
		{
			radeon_getDMAbuffer *b = (radeon_getDMAbuffer *) buf;
			int32 n;
			dprintf(("RADEON_IOCTL_GET_DMA_BUFFER \n" ));
			for( n=1; n<16; n++ )
			{
				if( (!di->pciBufferMap[n]) || (di->pciBufferMap[n] == pod->MemMgrKey) )
					break;
			}
			if( n == 16 )
				break;
			dprintf(("RADEON_IOCTL_GET_DMA_BUFFER n=%ld \n", n ));
				
			if( !di->pciBufferMap[n] )
			{
				if( createDMABuffer( di, pod, 1024 * 512 ) >= B_OK )
				{
					pod->gartOffset = n * 512*1024;
					initGART( di, pod );
				}
				else
				{
					break;
				}
			}
			b->addr = pod->pciBufferNC;
			b->cardOff = pod->gartOffset + CARD_PCI_BASE;
			b->size = 1024 * 512;
			dprintf(("RADEON_IOCTL_GET_DMA_BUFFER addr=%p  off=%p  size=%ld \n", b->addr, b->cardOff, b->size ));
			di->pciBufferMap[n] = pod->MemMgrKey;
			retval = B_OK;
		}
		break;
		

	case RADEON_IOCTL_SET_DISPLAY_MODE:
		{
			display_mode *mode = (display_mode *) buf;
			dprintf (("Radeon: set_display_mode  %d,%d \n", mode->virtual_width, mode->virtual_height ));
			
			if( Radeon_SetDisplayMode( di, mode ) )
			{
				memcpy( &di->mode_Primary, buf, sizeof(display_mode));
				retval = B_OK;
			}
			break;
		}
		
	case RADEON_IOCTL_SET_2ND_VIDEO_MODE:
		dprintf(("RADEON_IOCTL_SET_2ND_VIDEO_MODE \n" ));
		{
			radeon_set2ndvideomode *s = (radeon_set2ndvideomode *)buf;
			if( Radeon_Set2NdMode( di, s->w, s->h, s->hz, s->is32bpp ) )
			{
				di->videoOverideID = pod->MemMgrKey;
				retval = B_OK;
			}
		}
		dprintf(("RADEON_IOCTL_SET_2ND_VIDEO_MODE OK \n" ));
		break;

	case RADEON_IOCTL_GET_DISPLAY_MODE:
		memcpy( buf, &di->mode_Primary, sizeof(display_mode));
		retval = B_OK;
		break;
		
	case RADEON_IOCTL_GET_FB_CONFIG:
	case RADEON_IOCTL_GET_PIXEL_CLOCK_LIMITS:
	case RADEON_IOCTL_SET_INDEXED_COLORS:
		break;
	
	case RADEON_IOCTL_GET_DPMS_MODE:
		((uint32 *)buf)[0] = di->dpms_mode;
		retval = B_OK;
		break;

	case RADEON_IOCTL_SET_DPMS_MODE:
		Radeon_SetDPMS( di->ci, ((uint32 *)buf)[0] );
		di->dpms_mode = ((uint32 *)buf)[0];
		retval = B_OK;
		break;

		/*
		 * PRIVATE ioctl from here on.
		 */
	case RADEON_IOCTL_GETGLOBALS:
		{
			radeon_getglobals *gg = (radeon_getglobals *) buf;
			dprintf(("Radeon: fx_control - THDFX_IOCTL_GETGLOBALS\n"));
			/*
			 * Check protocol version to see if client knows how to talk
			 * to us.
			 */
			if (gg->ProtocolVersion == RADEON_IOCTLPROTOCOL_VERSION)
			{
				gg->GlobalArea = di->AreaCardInfo;
				retval = B_OK;
			}
		}
		break;

#if 0
	case THDFX_IOCTL_CHECKFORROOM:
		{
			uint32 regBase = io_base_offset;
			CHECKFORROOM;
			break;
		}

		/* I2C Routines */
	case THDFX_IOCTL_READ_I2C_REG:
		{
#define	I2C_REG_VAL	((int *) buf)
			int32 LTemp;
			LTemp = ci->ci_Regs->regs_io_u.regs_io.vidSerialParallelPort;
			LTemp &= (SST_SERPAR_I2C_SCK_IN | SST_SERPAR_I2C_DSA_IN);
			LTemp = LTemp >> 26;
			*I2C_REG_VAL = LTemp;
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_I2C_REG:
		{
#define	I2C_REG_VAL	((int *) buf)
			int32 LTemp;
			cpu_status cs;
			LTemp = ci->ci_Regs->regs_io_u.regs_io.vidSerialParallelPort;
			LTemp &= ~(SST_SERPAR_I2C_SCK_OUT | SST_SERPAR_I2C_DSA_OUT);	// clear all I2C bits
			LTemp |= (*I2C_REG_VAL) << 24;	// Shift in the new values for clock & data
			ci->ci_Regs->regs_io_u.regs_io.vidSerialParallelPort = LTemp;
			// Clock must be held for at least 5us so we spin here with interrupts disabled
			cs = disable_interrupts ();
			spin (5);
			restore_interrupts (cs);
			retval = B_OK;
			break;
		}
#endif
	case MEM_IOCTL_GET_MEMMGR_ID:
	{
		uint32 *d = (uint32 *)buf;
		d[0] = pod->MemMgrKey;
		dprintf(("Radeon: fx_control - MEM_IOCTL_GET_MEMMGR_ID  MemMgrKey = 0x%x \n", pod->MemMgrKey ));
		retval = B_OK;
		break;
	}

	case MEM_IOCTL_GET_MEMMGR_AREA:
	{
		uint32 *d = (uint32 *)buf;
		dprintf(("Radeon: fx_control - MEM_IOCTL_GET_MEMMGR_AREA \n"));
		d[0] = di->ma->areaID;
		retval = B_OK;
		break;
	}

	case MEM_IOCTL_SET_SEM:
	{
		dprintf(("Radeon: fx_control - MEM_IOCTL_SET_SEM \n"));
		__mem_SetSem( di->ma, pod->MemMgrKey, buf );
		retval = B_OK;
		break;
	}

	case MEM_IOCTL_RELEASE_SEMS:
	{
		dprintf(("Radeon: fx_control - MEM_IOCTL_RELEASE_SEMS \n"));
		__mem_ReleaseSems( di->ma );
		retval = B_OK;
		break;
	}
	
	default:
		dprintf (("Radeon: fx_control - Unknown ioctl = 0x%x\n", msg));
		break;
	}
	
	dprintf(("Radeon: fx_control - EXIT, returning 0x%x\n", retval));
	return (retval);
}

device_hooks *find_device (const char *name)
{
	int index;

	dprintf(("Radeon: find_device - Enter name=%s\n", name));

	for (index = 0; radeon_di.DevNames[index]; index++)
	{
		if (strcmp (name, radeon_di.DevNames[index]) == 0)
		{
			dprintf(("Radeon: find_device - Match Found - returning device hooks\n"));
			return &thdfx_device_hooks;
		}
	}
	dprintf(("Radeon: find_device - Failed to find match - returning NULL\n"));
	return (NULL);
}

void uninit_driver (void)
{
// dprintf(("Radeon: uninit_driver - Enter\n"));

#if defined(THDFX_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
//	remove_debugger_command ("thdfxdump", thdfxdump);
#endif
// dprintf(("Radeon: uninit_driver - Exit\n"));
}

const char **publish_devices (void)
{
	const char **dev_list = NULL;
	/* return the list of supported devices */
// dprintf(("Radeon: publish_devices\n"));
	dev_list = (const char **) radeon_di.DevNames;
	return (dev_list);
}

/****************************************************************************
 * Debuggery.
 */

#if defined(THDFX_DEBUG)   || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
static int radeondump (int argc, char **argv)
{
//	int i, k;
	uint32 readPtrL1, readPtrL2, hwreadptr/*, status*/;
	readPtrL1 = 1;
	readPtrL2 = 2;
	hwreadptr = 0;

	kprintf ("Radeon kernel driver\nThere are %d device(s)\n", radeon_di.DeviceCount );
	kprintf ("Driver-wide Senahpore: %d\n", radeon_di.LockSem );
//	kprintf ("Total calls to IRQ handler: %ld\n", ncalls);
//	kprintf ("Total relevant interrupts: %ld\n", thdfxdd->dd_NInterrupts);

/*
	for (i = 0; i < thdfxdd->dd_NDevs; i++)
	{
		devinfo *di = &thdfxdd->dd_DI[i];
		thdfx_card_info *ci = di->di_thdfxCardInfo;

		kprintf ("Device %s\n", di->di_Name);
		kprintf ("\tTotal interrupts: %ld\n", di->di_NInterrupts);
		kprintf ("\tVBlanks: %ld\n", di->di_NVBlanks);
		kprintf ("\tLast IRQ mask: 0x%08lx\n", di->di_LastIrqMask);
		if (ci)
		{
			kprintf ("  thdfx_card_info: 0x%08lx\n", ci);
			kprintf ("  Base register 0,1: 0x%08lx, 0x%08lx\n", ci->ci_BaseAddr0, ci->ci_BaseAddr1);
			kprintf ("  IRQFlags: 0x%08lx\n", ci->ci_IRQFlags);

			kprintf ("  ci_AllocFrameBuffer \n");
			__mem_Kprint_alloc (ci->ci_AllocFrameBuffer);

			kprintf ("  ci_AllocSentinal \n");
			__mem_Kprint_alloc (ci->ci_AllocSentinal);

			kprintf ("  ci_AllocCursor \n");
			__mem_Kprint_alloc (ci->ci_AllocCursor);

			for (k = 0; k < 2; k++)
			{
				kprintf ("  ci_AllocFifo %x \n", k);
				__mem_Kprint_alloc (ci->ci_AllocFifo[k]);

				do
				{
					if (k == 1)
						readPtrL1 = ci->ci_Regs->regs_agp_u.regs_agp.cmdFifo1.readPtrL;
					else if (k == 0)
						readPtrL1 = ci->ci_Regs->regs_agp_u.regs_agp.cmdFifo0.readPtrL;
					status = ci->ci_Regs->regs_3d_u.regs_3d.status;
					if (k == 1)
						readPtrL2 = ci->ci_Regs->regs_agp_u.regs_agp.cmdFifo1.readPtrL;
					else if (k == 0)
						readPtrL2 = ci->ci_Regs->regs_agp_u.regs_agp.cmdFifo0.readPtrL;
				}
				while (readPtrL1 != readPtrL2);

				hwreadptr = (((uint32) ci->ci_CmdTransportInfo[k].fifoStart) +
								 readPtrL2 - (uint32) ci->ci_CmdTransportInfo[k].fifoOffset);
				kprintf ("  ci_CmdTransportInfo[%d]: HW Read Ptr = 0x%08lx\n", k, hwreadptr);
			}
		}
	}
	*/
	kprintf ("\n");
	return (B_OK);
}

#endif
