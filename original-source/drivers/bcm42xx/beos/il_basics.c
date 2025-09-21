/*
 * BroadCom bcm42xx PCI 11mbit HPNA phone line driver
 * Basic driver functions
 *
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 * written by Peter Folk, from the BroadCom linux driver
 */

#include <errno.h>
/* BroadCom #includes */
#include <epivers.h>
#include <bcmendian.h>
#include <typedefs.h>
#include <proto/ethernet.h>
#include <proto/ilcp.h>
#include <bcm42xx.h>
#include <il_stats.h>
#include <ilsockio.h>
#include <ilc.h>
#include <il_export.h>  /* declarations for il_... functions */
                        /* called by ilc; defined in the current file */

/* Basic Driver functionality */
#define kDevName "bcm42xx"

#define DEBUG
#define MAX_DEVICES        4
#define SINGLE_OPEN
#define SINGLE_READ
#define SINGLE_WRITE
#define TX_BUFFERS         4
#define RX_BUFFERS         32

#include <Drivers.h>
#include <PCI.h>
#include "common/benaphore.h"
#include "common/fc_lock.h"
#include "common/kbq.h"

typedef struct {
	ilc_info_t	ilc;		// common os-independent data
	area_id		reg_area;	// area for mapping the pci registers
	area_id		ddma_area;	// area for DMA descriptors
	area_id		buf_area;	// area for chip DMA buffers
	benaphore_t	lock;		// per-device perimeter lock
	sem_id		intr_sem;	// Interrupt semaphore
	thread_id	intr_tid;	// Interrupt handler thread
	volatile uint32		intstatus; // last non zero interrupt this device saw
	timer		watchdog;	// Perform periodic tasks
	bool		noblock;	// Blocking reads/writes.
	
	kbq_t		rx_reserve;// Buffers ready to receive data
	kbq_t		tx_reserve;// Buffers ready to be put on the xmit que
	kbq_t		rxq;		// Received buffers waiting to be read()
	kbq_t		txq;		// Write()en buffers waiting to be transmitted
	
	fc_lock		rlock;		// reader lock: sig'd when rxq has new data, from sendup().
	sem_id		wlock;		// writer lock, sig'ed when tx_reserve has space, from txreclaim().
	
	int			alloc_state;// keep track of what we have to deallocate
	
	// Required by driver_basics.h
	int32		devID;		// device identifier: [0,MAX_DEVICES)
	pci_info	*pciInfo;	// info about this device
	uint32		debug;		// debugging level mask
} dev_info_t;
typedef dev_info_t il_info_t;

#include "common/ether_basics.h"
#include "common/driver_basics.c"
#include "common/kb.c"


/* static function prototypes (other than those in driver_basics.h) */
static int32 interrupt_device(void *_il);
static int32 handle_interrupts(void*);
static status_t il_acquire(il_info_t *il);
static status_t il_release(il_info_t *il);
static void watch_device(void *_device, int freq);
static void il_sendnext(il_info_t *il);
static void il_send(il_info_t *il, kbuf_t *b, int txpri, int txbpb);
static void il_intr(il_info_t *il);
static void il_recvpio(il_info_t *il);
static void il_recvdma(il_info_t *il);


static struct ether_addr ether_default = {{ 0, 1, 2, 3, 4, 0 }};


/*
 * check_device
 *
 * Return B_OK iff dev is supported by this driver.
 */
static status_t check_device(const pci_info* dev)
{
	return ilc_chipid(dev->vendor_id, dev->device_id)? B_OK : B_ERROR;
}


static status_t open_device(dev_info_t *dev, uint32 flags)
{
	status_t ret;
	ilc_info_t *ilc = &(dev->ilc);
	
	// Set up PCI registers
	DEBUG_DEVICE(INFO,"open(): setting bus mastering\n", dev);
	set_bus_mastering(dev->pciInfo);
	
	DEBUG_DEVICE(INFO,"open(): mapping PCI registers\n", dev);
	dev->reg_area = map_pci_memory(dev, 0, (void**)&ilc->regs);
	if (dev->reg_area < B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't map register memory!\n",dev);
		return dev->reg_area;
	}
	else
	{
		DEBUG_DEVICE(PCI_IO, "open(): register area mapped to %p (area %d)\n",
			dev, ilc->regs, (int)dev->reg_area);
	}
	++dev->alloc_state;
	
	// Allocate locks
	ret = create_ben(&dev->lock, kDevName " perimeter lock");
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't create perimiter lock!\n",dev);
		return ret;
	}
	++dev->alloc_state;
	
	ret = il_acquire(dev);
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't lock device!\n",dev);
		return ret;
	}
	
	ret = create_fc_lock(&dev->rlock, 0, kDevName " reader lock");
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't create reader lock!\n",dev);
		return ret;
	}
	++dev->alloc_state;
	
	ret = dev->wlock = create_sem(0, kDevName " writer lock");
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't create writer lock!\n",dev);
		return ret;
	}
	++dev->alloc_state;
	
	dev->intr_sem = create_sem(0, kDevName " interrupt trigger");
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't create interrupt-trigger semaphore!\n",dev);
		return ret;
	}
	++dev->alloc_state;
	
	
	// XXX?
	// In the linux driver for ARM chips, they set up the IRQ to happen
	// on the falling edge of GPIO2.  We may need to do that if this
	// driver is to work with a potential BeOS-ARM.
	
	ilc->unit = dev->devID;
	ilc->chiprev = R_REG(&ilc->regs->devstatus) & DS_RI_MASK;
	ilc_chipreset(ilc);
	
	//
	// Set up DMA resources
	//
	{
		int i;
		void *addr;
		physical_entry entry;
		size_t sz = (TX_BUFFERS+RX_BUFFERS)*BUFSZ;
		kbuf_t *b;
		
		#if B_PAGE_SIZE != 4096
		#error We assume areas are mapped on 4096-byte boundries.  \
		       If that is not the case, things here may fail: \
		       Caveat Emptor.
		#endif
		
		//
		// Allocate transmit/receive descriptor rings, locked, 4096-aligned
		//
		
		// we allocate an area big enough to cover the two descriptor
		// rings, which have to each be 4096-byte aligned.  We know that
		// BeOS allocates memory in B_PAGE_SIZE (4096)-byte chunks, so
		// we don't need to specify that the area is to be contiguous
		// (pages always are).  XXX This could change!
		dev->ddma_area = create_area(kDevName " descriptor dma area",
			&addr, B_ANY_KERNEL_ADDRESS, 4096*2, B_FULL_LOCK,
			B_READ_AREA|B_WRITE_AREA);
		if (dev->ddma_area<B_OK)
		{
			DEBUG_DEVICE(ERR, "open(): Couldn't allocate descriptor area (%s)!\n",
				dev, strerror(dev->ddma_area));
			return dev->ddma_area;
		}
		else
		{
			DEBUG_DEVICE(INFO,"open(): created dma-descriptor area %ld\n",
				dev, dev->ddma_area
			);
		}
		++dev->alloc_state;
		
		get_memory_map(addr,4096,&entry,1);
		ASSERT(entry.size == 4096 && ((uint32)entry.address&(4096-1)) == 0);	
		ilc->txd = addr;
		ilc->txdpa = entry.address;
		addr = (char*)addr+4096;
		
		get_memory_map(addr,4096,&entry,1);
		ASSERT(entry.size == 4096 && ((uint32)entry.address&(4096-1)) == 0);	
		ilc->rxd = addr;
		ilc->rxdpa = entry.address;
		
		//
		// Allocate DMA buffers: (TX_BUFFERS + RX_BUFFERS)*BUFSZ bytes, locked
		//
		
		// Buffers can't cross 4096-boundries (according to the port-
		// ing docs).  But this is easy since B_PAGE_SIZE is a multiple
		// of 4096 and BUFSZ divides B_PAGE_SIZE evenly.
		dev->buf_area = create_area(kDevName " buffer dma area", 
			&addr, B_ANY_KERNEL_ADDRESS, sz, B_FULL_LOCK,
			B_READ_AREA|B_WRITE_AREA);
		if (dev->buf_area<B_OK)
		{
			DEBUG_DEVICE(ERR, "open(): Couldn't allocate buffer area (%s)!\n",
				dev, strerror(dev->buf_area));
			return dev->buf_area;
		}
		else
		{
			DEBUG_DEVICE(INFO,"open(): created dma buffer area %ld\n",
				dev, dev->buf_area
			);
		}
		++dev->alloc_state;
		
		
		init_kbq(&dev->txq, kDevName " txq");
		init_kbq(&dev->tx_reserve, kDevName " tx reserve q");
		
		// populate dev->tx_reserve
		for (i=0; i<TX_BUFFERS; ++i, addr=(char*)addr+BUFSZ)
		{
			get_memory_map(addr,BUFSZ,&entry,1);
			ASSERT(entry.size == BUFSZ && ((uint32)entry.address&(BUFSZ-1)) == 0);
			
			b = create_kb(addr,BUFSZ,entry.address);
			if (b==NULL)
			{
				DEBUG_DEVICE(ERR, "open(): out of memory allocating tx buffers!\n", dev);
				return B_NO_MEMORY;
			}
			
			kbq_push_front(&dev->tx_reserve,b);
		}
		DEBUG_DEVICE(INFO,"open(): allocated %d tx buffers\n",dev,i);
		release_sem_etc(dev->wlock,TX_BUFFERS,B_DO_NOT_RESCHEDULE);
		
		
		init_kbq(&dev->rxq, kDevName " rxq");
		init_kbq(&dev->rx_reserve, kDevName " rx reserve q");
		
		// populate dev->rx_reserve
		for (i=0; i<RX_BUFFERS; ++i, addr=(char*)addr+BUFSZ)
		{
			get_memory_map(addr,BUFSZ,&entry,1);
			ASSERT(entry.size == BUFSZ && ((uint32)entry.address&(BUFSZ-1)) == 0);
			
			b = create_kb(addr,BUFSZ,entry.address);
			if (b==NULL)
			{
				DEBUG_DEVICE(ERR, "open(): out of memory allocating rx buffers!\n", dev);
				return B_NO_MEMORY;
			}
			
			kbq_push_front(&dev->rx_reserve,b);
		}
		DEBUG_DEVICE(INFO,"open(): allocated %d rx buffers\n",dev,i);
	}
	
	// set up IRQ-handler thread
	dev->intr_tid = spawn_kernel_thread(
		handle_interrupts, kDevName " interrupt handler", B_REAL_TIME_PRIORITY, dev
	);	                                              
	if (dev->intr_tid < B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): Couldn't spawn the interrupt handler thread!\n",dev);
		return dev->intr_tid;
	}
	else
	{
		DEBUG_DEVICE(INFO, "open(): spawned interrupt handler thread %d\n",
			dev, (int)dev->intr_tid);
		resume_thread(dev->intr_tid);
	}
	++dev->alloc_state;
	
	// set default software intmask - no tx interrupts in dma mode 
	ilc->intmask = I_RI | I_ERRORS;
	
	// Initialize common code
	ret = ilc_attach(ilc);
	if (ret < B_OK)
	{
		DEBUG_DEVICE(ERR, "open(): ilc_attach() failed (%s)\n",
			dev, strerror(ret));
		return ret;
	}
	++dev->alloc_state;
	
	#ifdef IL_STATS
	// init stats infrastructure
	if (ilc_stats_allocinit(ilc, kDevName))
	{
		DEBUG_DEVICE(ERR, "open(): stats_allocinit failed\n", dev);
		return B_NO_MEMORY;
	}
	#endif
	
	// if no sprom, set the local ether address
	if (!(R_REG(&ilc->regs->devstatus) & DS_SP)) {
		bcopy(&ether_default, &ilc->perm_etheraddr, ETHER_ADDR_LEN);
		bcopy(&ilc->perm_etheraddr, &ilc->cur_etheraddr, ETHER_ADDR_LEN);
	}
	
	// get up, get up, get busy
	il_up(dev); // do it
	// c'mon and shake that body
	// ahem, sorry.
	
	/* schedule a one second watchdog timer */
	register_kernel_daemon(watch_device, dev, 10);
	++dev->alloc_state;
	
	il_release(dev);
	
	// We're ready to go!
	return B_OK;
}

/*
 * close_device
 *
 * Tell all pending operations to abort and unblock all blocked ops.
 * DO NOT deallocate anything, as some ops could still be pending.
 */
static status_t close_device(dev_info_t *dev)
{
	/* stop watchdog timer */
	il_acquire(dev);
	
	unregister_kernel_daemon(watch_device, dev);
	
	// turn interrupt into a no-op
	delete_sem(dev->intr_sem);
	
	// make all calls to acquire a lock fail
	delete_ben(&dev->lock);
	
	// unblock any pending read()s or write()s
	delete_sem(dev->wlock);
	delete_fc_lock(&dev->rlock);
	
	dev->ilc.promisc = FALSE;
	il_down(dev, 1);
	
	return B_OK;
}


/*
 * free_device
 *
 * All operations have completed, no more will be called.  De-
 * allocate anything you allocated (in open() or elsewhere).
 */
static status_t free_device(dev_info_t *dev)
{
	// deallocate [tr]x{q,_reserve}
	set_dprintf_enabled(true);
	DEBUG_DEVICE(INFO,"free(): freeing %ld rx buffers\n",dev, dev->rxq.len);
	purge_kbq(&dev->rxq);
	
	DEBUG_DEVICE(INFO,"free(): freeing %ld rx reserves\n",dev, dev->rx_reserve.len);
	purge_kbq(&dev->rx_reserve);
	
	DEBUG_DEVICE(INFO,"free(): freeing %ld tx buffers\n",dev, dev->txq.len);
	purge_kbq(&dev->txq);
	
	DEBUG_DEVICE(INFO,"free(): freeing %ld tx reserves\n",dev, dev->tx_reserve.len);
	purge_kbq(&dev->tx_reserve);
	
	
	switch (dev->alloc_state)
	{
	default:
	case 10: // register kernel daemon
	case 9:
		ilc_detach(&dev->ilc);
	case 8: // spawn interrupt handler
	case 7:
		DEBUG_DEVICE(INFO,"free(): deleting dma buffer area %ld\n",
			dev,dev->buf_area);
		delete_area(dev->buf_area);
	case 6:
		DEBUG_DEVICE(INFO,"free(): deleting dma-descriptor area %ld\n",
			dev,dev->ddma_area);
		delete_area(dev->ddma_area);
	case 1:
		DEBUG_DEVICE(INFO,"free(): deleting register area %ld\n",
			dev,dev->reg_area);
		delete_area(dev->reg_area);
	case 0:
	}
	return B_OK;
}

/*
 * control_device
 *
 * Handle ioctl().
 */
static status_t
control_device(dev_info_t *dev, uint32 msg,void *buf, size_t len)
{
	status_t ret=B_OK;
	
	switch (msg)
	{
	  case ETHER_GETADDR:       /* get ethernet address */
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_GETADDR\n", dev);
		if (buf==NULL)
		{
			DEBUG_DEVICE(ERR,"control() - ETHER_GETADDR: NULL buffer\n", dev);
			return B_ERROR;
		}
		
		/* XXX for some reason BONE doesn't pass a size!
		if (len<sizeof(dev->ilc.cur_etheraddr))
		{
			DEBUG_DEVICE(ERR,"control() - ETHER_GETADDR: "
				"not enough room to store address "
				"(%ld offered, %ld needed)\n", dev, len,
				sizeof(dev->ilc.cur_etheraddr)
			);
			return ENOMEM;
		}
		*/
		
		if (il_acquire(dev)==B_OK)
		{
			memcpy(buf, &dev->ilc.cur_etheraddr, sizeof(dev->ilc.cur_etheraddr));
			il_release(dev);
		}
		return B_OK;
	
	  case ETHER_INIT:          /* set irq and port */
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_INIT\n", dev);
		return B_OK;
	
	  case ETHER_NONBLOCK:      /* set/unset nonblocking mode */
		dev->noblock = FALSE;
		
		while (len-->0)
		{
			if (((const char*)buf)[len])
			{
				dev->noblock = TRUE;
				break;
			}
		}
		// XXX unblock waiting threads
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_NONBLOCK = %d\n",
			dev, dev->noblock);
		return B_OK;
	
	  case ETHER_ADDMULTI:      /* add multicast addr */
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_ADDMULTI\n", dev);
		if (len<sizeof(dev->ilc.multicast[0]))
		{
			DEBUG_DEVICE(ERR,"control() - ETHER_ADDMULTI: "
				"not enough room to read address "
				"(offered %ld, needed %ld)\n", dev, len,
				sizeof(dev->ilc.multicast[0])
			);
			return B_ERROR;
		}
		
		if (il_acquire(dev)==B_OK)
		{
			int i;
			for (i=0; i<dev->ilc.nmulticast; ++i)
			{
				if (!memcmp(&dev->ilc.multicast[i],buf,sizeof(dev->ilc.multicast[1])))
				{
					DEBUG_DEVICE(ERR, "control() - ETHER_ADDMULTI: address added previously\n", dev);
					ret = B_ERROR;
					break;
				}
			}
			
			if (ret==B_OK)
			{
				if (dev->ilc.nmulticast >= MAXMULTILIST)
				{
					DEBUG_DEVICE(INFO,"control() - ETHER_ADDMULTI: "
						"max # of multicast addrs reached; "
						"receiving all multicast frames\n", dev
					);
					// we keep track of how far over we are in allmulti:
					++dev->ilc.allmulti;
				}
				else
				{
					// just 'cuz we're here does not mean that
					// allmulti==0.  We only get that case if we
					// remove any addresses that overshoot this
					// cache.
					
					dev->ilc.multicast[dev->ilc.nmulticast++]
					 = *((struct ether_addr*) buf);
				}
				
				il_init(dev);
			}
			il_release(dev);
		}
		return B_OK;
	
	  case ETHER_REMMULTI:      /* rem multicast addr */
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_REMMULTI\n", dev);
		
		if (il_acquire(dev)==B_OK)
		{
			int i, j;
			for (i=0, j=0; i<dev->ilc.nmulticast; ++i)
			{
				if
				(
				  memcmp(
					&dev->ilc.multicast[i],buf,
					sizeof(dev->ilc.multicast[i])
				  )
				)
				{
					if (i!=j) memcpy(
						&dev->ilc.multicast[j],
						&dev->ilc.multicast[i],
						sizeof(dev->ilc.multicast[i])
					);
					++j;
				}
			}
			if (j<i)
			{
				dev->ilc.nmulticast = j;
			}
			// Otherwise, we didn't find the address in the cache:
			else if (dev->ilc.allmulti)
			{
				// If we exceeded MAXMULTILIST, we have to keep
				// receiving all multicast frames until we have
				// removed as many that aren't in the list as we
				// went over
				--dev->ilc.allmulti;
			}
			else ret = B_ERROR;
			
			il_init(dev);
			il_release(dev);
		}
		return ret;
	
	  case ETHER_SETPROMISC:    /* set promiscuous */
		{
			bool prom = FALSE;
			
			while (len-->0)
			{
				if (((const char*)buf)[len])
				{
					prom = TRUE;
					break;
				}
			}
			
			DEBUG_DEVICE(FUNCTION,"control(): ETHER_SETPROMISC = %d\n", dev, prom);
			if (il_acquire(dev)==B_OK)
			{
				dev->ilc.promisc = prom;
				il_init(dev);
				il_release(dev);
			}
		}
		return B_OK;
	
	  case ETHER_GETFRAMESIZE:  /* get frame size */
		DEBUG_DEVICE(FUNCTION,"control(): ETHER_GETFRAMESIZE\n", dev);
		// XXX
		// if (len<sizeof(uint32)) return ENOMEM;
		*(uint32*)buf = ILINE_MAX_LEN;
		return B_OK;
	
	  default:
	 	if (il_acquire(dev)==B_OK)
	 	{
			ret = ilc_ioctl(&dev->ilc, msg-SIOCDEVPRIVATE, buf);
			il_release(dev);
		}
		return ret;
	}
}

/*
 * read_device
 *
 * Handle read().
 */
static status_t
read_device( dev_info_t *dev, off_t pos, void *buf, size_t *space)
{
	status_t ret = B_OK;
	uint spaceleft;
	kbuf_t *rx;
	
	/* XXX None of the other drivers check for this---why not? */
	if (space==NULL || buf==NULL)
	{
		DEBUG_DEVICE(ERR,"read(): len or buf passed as NULL!\n", dev);
		return B_ERROR;
	}
	spaceleft = *space;
	
	// XXX protect against re-entrant writes
	
	ret = fc_wait(&dev->rlock, (dev->noblock? 0:B_INFINITE_TIMEOUT));
	if (ret < B_OK)
	{
		DEBUG_DEVICE(ERR,"read(): error %ld acquiring lock "
			"for packet recv\n", dev, ret);
		*space=0;
		return ret;
	}
	
	if (il_acquire(dev)==B_OK)
	{
		rx = kbq_front(&dev->rxq);
		/* XXX None of the other drivers check for this---why not? */
		if (*space < rx->len) ret -= 2;
		else if (rx == NULL)  ret -= 1;
		else
		{
			rx = kbq_pop_front(&dev->rxq);
			
			// we copy it now so that we can return it to
			// the free list without reaccquiring the
			// perimiter --- if we switch to the atomic
			// list code (XXX: see bone/modules/util) then
			// we could do the copy outside of the lock...
			
			*space = rx->len;
			memcpy(buf, rx->data, rx->len);
			
			DEBUG_DEVICE(INFO, "returned rxb %p\n", dev,rx);
			kbq_push_front(&dev->rx_reserve,rx);
		}
		il_release(dev);

		if (ret<B_OK)
		{
			switch(ret)
			{
				case B_OK-1: DEBUG_DEVICE(ERR, "read(): huh? there are no frames for me!\n",dev); break;
				case B_OK-2: DEBUG_DEVICE(ERR, "read(): not enough memory for frame\n",dev); break;
			}
			*space = 0;
			return B_ERROR;
		}
	}
	
	
	// XXX
	// should we return more than one packet
	// if more than one is available?
	
	
	return B_OK;
}

/*
 * write_device
 *
 * Handle write().
 */
static status_t
write_device(dev_info_t *dev, off_t pos, const void *buf, size_t *len)
{
	status_t ret = B_OK;
	kbuf_t *b;
	
	/* XXX None of the other drivers check for this---why not? */
	if (len==NULL || buf==NULL)
	{
		DEBUG_DEVICE(ERR,"write(): len or buf passed as NULL!\n", dev);
		return B_ERROR;
	}
	
	if (*len > BUFSZ-TXOFF)
	{
		DEBUG_DEVICE(ERR,"write(): packet too big (%ld vs %d)!\n",
			dev, *len, BUFSZ-TXOFF);
		return E2BIG;
	}
	
	// protect against re-entrant writes
	
	if (il_acquire(dev)==B_OK)
	{
		// grab a buffer
		b = il_pget(dev,TRUE,*len,NULL,NULL);
		if (b)
		{
			memcpy(b->data, buf, *len);
			// ditto XXX: atomic_list here
			
			// put it on the end of dev->txq:
			kbq_push_back(&dev->txq,b);
			
			// Engage!
			il_sendnext(dev);
			// Aye-aye cap'n!  *crunch*
		}
		else
		{
			DEBUG_DEVICE(ERR, "write(): error getting a tx buffer: "
				"%s\n", dev, strerror(errno));
			ret = B_NO_MEMORY;
		}
		il_release(dev);
	}
	
	return ret;
}

/*
 * watchdog daemon
 *
 * kernel_daemon that handles periodic driver functions.
 */
static void
watch_device(void *_device, int freq)
{
	dev_info_t *dev = (dev_info_t*)_device;
	DEBUG_DEVICE(FUNCTION, "entering watch_device()\n", dev);
	
	if (il_acquire(dev)==B_OK)
	{
		ilc_watchdog(&dev->ilc);
		il_txreclaim(dev, FALSE);
		
		/* run the tx queue */
		if (kbq_notempty(&dev->txq)) il_sendnext(dev);
		il_release(dev);
	}
	DEBUG_DEVICE(FUNCTION, "leaving watch_device()\n", dev);
}


static int32
interrupt_device(void *_il)
{
	volatile uint32 intstatus;
	il_info_t *il = (il_info_t*)_il;
	
	if ((intstatus = R_REG(&(il->ilc.regs->intstatus))) == 0) {
		DEBUG_DEVICE(ERR, "zero interrupt!!\n", il);
		return B_UNHANDLED_INTERRUPT;
	}
	
	/* pass the masked interrupt to the thread */
	atomic_or(&il->intstatus, intstatus);
	
	/* clear the PCI interrupt */
	W_REG(&il->ilc.regs->intstatus, intstatus);
	
	/* signal the card */
	release_sem_etc(il->intr_sem, 1, B_DO_NOT_RESCHEDULE);
	
	return B_INVOKE_SCHEDULER;
}

static int32
handle_interrupts(void* _device)
{
	status_t ret = B_OK;
	il_info_t *il = (il_info_t*) _device;
	
	DEBUG_DEVICE(FUNCTION, "interrupt handler thread starting.\n",il);
	
	while ((ret=acquire_sem(il->intr_sem))==B_OK)
	{
		if (il_acquire(il)==B_OK)
		{
			il_intr(il);
			il_release(il);
		}
	}
	
	DEBUG_DEVICE(FUNCTION, "interrupt handler thread stopping.\n",il);
	
	return ret;
}

static int debug_device(dev_info_t* dev, int argc, char** argv)
{
	char buf[8192];
	if (dev == NULL) { kprintf("please specify a device number\n"); return 0; }
	
	ilc_dump(&dev->ilc, buf, sizeof(buf));
	kprintf("%s",buf);
	
	kprintf("tx ");
	kprintf_kbq(&dev->txq);
	
	kprintf("tx_reserve ");
	kprintf_kbq(&dev->tx_reserve);
	
	kprintf("rx ");
	kprintf_kbq(&dev->rxq);
	
	kprintf("rx_reserve ");
	kprintf_kbq(&dev->rx_reserve);
	
	
	return 0;
}

