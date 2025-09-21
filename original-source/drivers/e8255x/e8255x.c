 /* e8255x.c
 * Copyright (c) 1998 Be, Inc.	All Rights Reserved 
 *
 * Ethernet driver: handles PCI Intel EtherExpress Pro 100 cards
 * based on the Intel 8255x Lan controller chip
 *
 */
 
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include "e8255x.h"
#include <ether_driver.h>
#include <stdarg.h>
#include <PCI.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <SupportDefs.h>

#define DEBUG 1
#define speedo_debug 6

/* A few values that may be tweaked. */
/* The ring sizes should be a power of two for efficiency. */
#define TX_RING_SIZE	16		/* Effectively 2 entries fewer. */
#define RX_RING_SIZE	16
/* Size of an pre-allocated Rx buffer: <Ethernet MTU> + slack.*/
#define PKT_BUF_SZ		1536

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  ((800*100)/1000)


/* A few user-configurable values that apply to all boards.
   First set are undocumented and spelled per Intel recommendations. */

static int congenb = 0;		/* Enable congestion control in the DP83840. */
static int txfifo = 8;		/* Tx FIFO threshold in 4 byte units, 0-15 */
static int rxfifo = 8;		/* Rx FIFO threshold, default 32 bytes. */
/* Tx/Rx DMA burst length, 0-127, 0 == no preemption, tx==128 -> disabled. */
static int txdmacount = 128;
static int rxdmacount = 0;

/* Set the copy breakpoint for the copy-only-tiny-buffer Rx method.
   Lower values use more memory, but are faster. */
static int rx_copybreak = 200;

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;

/* Maximum number of multicast addresses to filter (vs. rx-all-multicast) */
static int multicast_filter_limit = 64;

/*
 * New bus manager stuff.
 */
char				*pci_name = B_PCI_MODULE_NAME;
pci_module_info		*pci;



static sem_id locksem = B_ERROR;

static void Lock(void)
{
	if (locksem == B_ERROR) {
		locksem = create_sem(1, "lock");
	}
	acquire_sem(locksem);
}

static void Unlock(void)
{
	release_sem(locksem);
}
 


/*
 * Timer 
 */


#define jiffies (100 * system_time() / 1000000)
struct timer_list {
	struct timer_list *next;
	struct timer_list *prev;
	unsigned long expires;
	unsigned long data;
	void (*function)(unsigned long);
};

typedef struct timer_stuff {
	struct timer_list *timer;
	sem_id sem;
	struct timer_stuff *next;
} timer_stuff_t;

static timer_stuff_t *timers;

static void add_timer(struct timer_list * timer);
static int  del_timer(struct timer_list * timer);


#define init_timer(timer) \
	(timer)->next = NULL; \
	(timer)->prev = NULL;


status_t
timer_thread(void *arg)
{
	timer_stuff_t *stuff = (timer_stuff_t *)arg;
	status_t res;
	timer_stuff_t **p;
	bigtime_t expires;
	bigtime_t now;
	bigtime_t wait;
	
	expires = (stuff->timer->expires * 1000000) / 100;
	now = system_time();
	if (expires > now) {
		wait = expires - now;
	} else {
		wait = 0;
	}
	kprintf("Timer thread waiting for %d microseconds\n", wait);
	if (wait) {
		res = acquire_sem_etc(stuff->sem, 1, B_TIMEOUT, wait);
	} else {
		res = B_TIMED_OUT;
	}
	if (res == B_TIMED_OUT) {
		stuff->timer->function(stuff->timer->data);
	}
	kprintf("Timer thread done\n");
	Lock();
	delete_sem(stuff->sem);
	for (p = &timers; p && *p; p = &(*p)->next) {
		if (*p == stuff) {
			kprintf("timer_thread: freeing %x\n",stuff);
			free((unsigned char *)stuff);
			*p = (*p)->next;
			break;
		}
	}
	Unlock();
	kprintf("Timer thread exiting\n");
	return (0);
}


/*
 * Timer uses semaphore for locking, don't call from interrupt
 */
void add_timer(
			   struct timer_list *timer
			   )
{
	thread_id id;
	timer_stuff_t *ntimer;
	ntimer = (void *)malloc(sizeof(*ntimer));
	kprintf("add_timer: Is malloc OK ??? ntimer =%x\n",ntimer);
	ntimer->timer = timer;
	ntimer->sem = create_sem(0, "timer sem");
	Lock();
	ntimer->next = timers;
	timers = ntimer;
	Unlock();

	kprintf("Adding timer\n");
	id = spawn_kernel_thread(timer_thread, "timer", B_NORMAL_PRIORITY, 
							 (void *)ntimer);
	resume_thread(id);
}

int del_timer(struct timer_list *timer)
{
	timer_stuff_t *stuff;

	kprintf("Deleting timer\n");
	Lock();
	for (stuff = timers; stuff; stuff = stuff->next) {
		if (stuff->timer == timer) {
			release_sem(stuff->sem);
			Unlock();
			return (0); /* ??? */
		}
	}
	Unlock();
	return (-1); /* ??? */
}


/*------------------------------------------------------------------------*/

static cpu_status
enter_crit (spinlock *lock)
{
	cpu_status	status;

	status = disable_interrupts ();
	acquire_spinlock (lock);

	return status;
}


/*------------------------------------------------------------------------*/

static void
exit_crit (spinlock *lock, cpu_status status)
{
	release_spinlock (lock);
	restore_interrupts (status);
}

/*------------------------------------------------------------------------*/



/* How to wait for the command unit to accept a command.
   Typically this takes 0 ticks. */
static inline void wait_for_cmd_done(long cmd_ioaddr)
{
	int wait = 100;
	do   ;
	while(inb(cmd_ioaddr) && --wait >= 0);
}

/* Operational parameter that usually are not changed. */

/* The rest of these values should never change. */

/* Offsets to the various registers.
   All accesses need not be longword aligned. */
enum speedo_offsets {
	SCBStatus = 0, SCBCmd = 2,	/* Rx/Command Unit command and status. */
	SCBPointer = 4,				/* General purpose pointer. */
	SCBPort = 8,				/* Misc. commands and operands.  */
	SCBflash = 12, SCBeeprom = 14, /* EEPROM and flash memory control. */
	SCBCtrlMDI = 16,			/* MDI interface control. */
	SCBEarlyRx = 20,			/* Early receive byte count. */
};
/* Commands that can be put in a command list entry. */
enum commands {
	CmdNOp = 0, CmdIASetup = 1, CmdConfigure = 2, CmdMulticastList = 3,
	CmdTx = 4, CmdTDR = 5, CmdDump = 6, CmdDiagnose = 7,
	CmdSuspend = 0x4000,		/* Suspend after completion. */
	CmdIntr = 0x2000,			/* Interrupt after completion. */
	CmdTxFlex = 0x0008,			/* Use "Flexible mode" for CmdTx command. */
};

/* The SCB accepts the following controls for the Tx and Rx units: */
#define	 CU_START		0x0010
#define	 CU_RESUME		0x0020
#define	 CU_STATSADDR	0x0040
#define	 CU_SHOWSTATS	0x0050	/* Dump statistics counters. */
#define	 CU_CMD_BASE	0x0060	/* Base address to add to add CU commands. */
#define	 CU_DUMPSTATS	0x0070	/* Dump then reset stats counters. */

#define	 RX_START	0x0001
#define	 RX_RESUME	0x0002
#define	 RX_ABORT	0x0004
#define	 RX_ADDR_LOAD	0x0006
#define	 RX_RESUMENR	0x0007
#define INT_MASK	0x0100
#define DRVR_INT	0x0200		/* Driver generated interrupt. */


/* The parameters for a CmdConfigure operation.
   There are so many options that it would be difficult to document each bit.
   We mostly use the default or recommended settings. */
const char i82557_config_cmd[22] = {
	22, 0x08, 0, 0,  0, 0x80, 0x32, 0x03,  1, /* 1=Use MII  0=Use AUI */
	0, 0x2E, 0,  0x60, 0,
	0xf2, 0x48,   0, 0x40, 0xf2, 0x80, 		/* 0x40=Force full-duplex */
	0x3f, 0x05, };
const char i82558_config_cmd[22] = {
	22, 0x08, 0, 1,  0, 0x80, 0x22, 0x03,  1, /* 1=Use MII  0=Use AUI */
	0, 0x2E, 0,  0x60, 0x08, 0x88,
	0x68, 0, 0x40, 0xf2, 0xBD, 		/* 0xBD->0xFD=Force full-duplex */
	0x31, 0x05, };

/* PHY media interface chips. */
static const char *phys[] = {
	"None", "i82553-A/B", "i82553-C", "i82503",
	"DP83840", "80c240", "80c24", "i82555",
	"unknown-8", "unknown-9", "DP83840A", "unknown-11",
	"unknown-12", "unknown-13", "unknown-14", "unknown-15", };
enum phy_chips { NonSuchPhy=0, I82553AB, I82553C, I82503, DP83840, S80C240,
					 S80C24, I82555, DP83840A=10, };
static const char is_mii[] = { 0, 1, 1, 0, 1, 1, 0, 1 };


/* The Speedo3 Rx and Tx frame/buffer descriptors. */
struct descriptor {			/* A generic descriptor. */
	int16 status;		/* Offset 0. */
	int16 command;		/* Offset 2. */
	uint32 link;					/* struct descriptor *  */
	unsigned char params[0];
};

/* The Speedo3 Rx and Tx buffer descriptors. */
struct RxFD {					/* Receive frame descriptor. */
	int32 status;
	uint32 link;				/* struct RxFD */
	uint32 rx_buf_addr;			/* physical address */
	uint16 count;
	uint16 size;
};

/* Selected elements of the RxFD.status word. */
enum RxFD_bits {
	RxComplete=0x8000, RxOK=0x2000,
	RxErrCRC=0x0800, RxErrAlign=0x0400, RxErrTooBig=0x0200, RxErrSymbol=0x0010,
	RxEth2Type=0x0020, RxNoMatch=0x0004, RxNoIAMatch=0x0002,
};

struct TxFD {						/* Transmit frame descriptor set. */
	int32 status;
	uint32 link;					/* void * */
	uint32 tx_desc_addr;			/* Always points to the tx_buf_addr element. */
	int32 count;					/* # of TBD (=1), Tx start thresh., etc. */
	/* This constitutes two "TBD" entries -- we only use one. */
	uint32 tx_buf_addr0;			/* void *, frame to be transmitted.  */
	int32 tx_buf_size0;				/* Length of Tx frame. */
	uint32 tx_buf_addr1;			/* void *, frame to be transmitted.  */
	int32 tx_buf_size1;				/* Length of Tx frame. */
};

/* Elements of the dump_statistics block. This block must be lword aligned. */
struct speedo_stats {
	uint32 tx_good_frames;
	uint32 tx_coll16_errs;
	uint32 tx_late_colls;
	uint32 tx_underruns;
	uint32 tx_lost_carrier;
	uint32 tx_deferred;
	uint32 tx_one_colls;
	uint32 tx_multi_colls;
	uint32 tx_total_colls;
	uint32 rx_good_frames;
	uint32 rx_crc_errs;
	uint32 rx_align_errs;
	uint32 rx_resource_errs;
	uint32 rx_overrun_errs;
	uint32 rx_colls_errs;
	uint32 rx_runt_errs;
	uint32 done_marker;
};

struct enet_statistics{
	uint32 tx_packets;
	uint32 rx_packets;
	uint32 tx_errors;
	uint32 rx_errors;
	// ...
};

struct rx_frame_buf {
	struct RxFD rxfd;
	unsigned char rx_data[PKT_BUF_SZ];
};
struct tx_frame_buf {
	struct TxFD txfd;
	unsigned char tx_data[PKT_BUF_SZ];
};
//---------------------------------------------------------------------------
typedef struct e8255x_private {

	int	init;				/* already inited? */
	short irq_level;		/* our IRQ level */
	long ioaddr;			/* our I/O port address */
	int boundary;			/* boundary register value (mirrored) */
	uint8 myaddr[6];		/* my ethernet address */
	unsigned nmulti;		/* number of multicast addresses */
	ether_address_t multi[MAX_MULTI];	/* multicast addresses */
	int32	hw_lock;					/* spinlock for hw access */
	sem_id rx_sem;			/* ethercard io, except interrupt handler */
	sem_id tx_sem;			/* ethercard io, except interrupt handler */
	int nonblocking;		/* non-blocking mode */
	volatile int interrupted;   /* interrupted system call */
	sem_id inrw;			/* in read or write function */
	sem_id ilock;			/* waiting for input */
	sem_id olock;	 		/* waiting to output */
#if !__INTEL__
	area_id ioarea;
#endif		
	char devname[8];			/* Used only for kernel debugging. */
	const char *product_name;
	struct descriptor  *last_cmd;	/* Last command sent. */
	struct tx_frame_buf tx_ringp[TX_RING_SIZE];
	struct rx_frame_buf rx_ringp[RX_RING_SIZE];
	struct RxFD *last_rxf;	/* Last command sent. */
	struct enet_statistics stats;
	struct speedo_stats lstats;
	struct timer_list timer;	/* Media selection timer. */
	long last_rx_time;			/* Last Rx, in jiffies, to handle Rx hang. */
	unsigned int cur_rx, cur_tx;		/* The next free ring entry */
	unsigned int dirty_rx, dirty_tx;	/* The ring entries to be free()ed. */
	struct descriptor config_cmd;	/* A configure command, with header... */
	uint8 config_cmd_data[22];			/* .. and setup parameters. */
	int mc_setup_frm_len;			 	/* The length of an allocated.. */
	struct descriptor *mc_setup_frm; 	/* ..multicast setup frame. */
	char rx_mode;						/* Current PROMISC/ALLMULTI setting. */
	unsigned int tx_full:1;				/* The Tx queue is full. */
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int default_port:1;		/* Last dev->if_port value. */
	unsigned int rx_bug:1;				/* Work around receiver hang errata. */
	unsigned int rx_bug10:1;			/* Receiver might hang at 10mbps. */
	unsigned int rx_bug100:1;			/* Receiver might hang at 100mbps. */
	unsigned short phy[2];				/* PHY media interfaces available. */
	int32 SelfTest[6];					/* physically mapped memory that the chip writes into */
	bool tbusy;							/* for tx mutual exclusion - should be a semaphore */
	unsigned char mc_count;				/* multi cast count */					
	unsigned int trans_start;			/* time transmission of a frame started */
} e8255x_private_t;

static uchar * phys_start_area;
static uchar * virt_start_area;
static area_id 	e8255x_area;

void *
bus_to_virt(
				ulong ph_addr
				)
{
	ulong	offset;

	offset = ph_addr - (ulong) phys_start_area;

	if ((ph_addr < (ulong) phys_start_area) || (offset > sizeof (e8255x_private_t))) {
//#if PVMEM_ERR
		kprintf("bus_to_virt: Out of bounds: PA=0x%08x, %x\n", ph_addr, sizeof (e8255x_private_t));
//#endif
		return (virt_start_area);	// wrong addr but legal
	}

	return (virt_start_area + offset);
}


ulong
virt_to_bus(
				void * v_addr
				)
{
	ulong	offset;

	offset = (ulong) v_addr - (ulong) virt_start_area;

	if ((v_addr < virt_start_area) || (offset > sizeof (e8255x_private_t))) {
//#if PVMEM_ERR
		kprintf("v2b: Out of bounds: VA=0x%08x\n", v_addr);
//#endif
		return ((ulong) phys_start_area);	// wrong addr but legal
	}

	return ((ulong) phys_start_area + offset);
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static void *
get_area(size_t size,area_id *areap){
	uchar * base;
	area_id id;

	size = RNDUP(size, B_PAGE_SIZE);
	id = create_area("e8255x_lcw",
					&base,
					B_ANY_KERNEL_ADDRESS,
					size,
					B_FULL_LOCK | B_CONTIGUOUS,
					B_READ_AREA | B_WRITE_AREA);
	if (id < B_NO_ERROR) {
		kprintf("get_area: Can't create area, terminate\n");
		return (NULL);
	}
	if (lock_memory(base, size, B_DMA_IO | B_READ_DEVICE) != B_NO_ERROR) {
		delete_area(id);
		kprintf("get_area: Can't lock area\n");
		return (NULL);
	}
	memset(base, NULL, size);
	*areap = id;
	return (base);
}

void 
init_area(void * base) {
	physical_entry 	area_phys_addr[2];		// need only the start

	// Get location & size of phys blocks into pbtable[]
	get_memory_map(base,					// vbuf to translate
					B_PAGE_SIZE -1,			// size of vbuf
					&area_phys_addr[0],		// tbl to fill out
					1);						// #of entries

	// We're relying on the fact that both virt area
	// and it's phys map are contiguous as advertised.
	// If this is untrue, use the scheme similar to rings.
	virt_start_area = base;
	phys_start_area = (uchar *) (area_phys_addr[0].address);
}


/*
 * One for each IRQ
 */
static e8255x_private_t *ether_data[NIRQS];
static long ports_for_irqs[NIRQS];





/*
 * output_wait wakes up when the card is ready to transmit another packet
 */
#define output_wait(data, t) acquire_sem_etc(data->olock, 1, B_TIMEOUT, t)
#define output_unwait(data, c)	release_sem_etc(data->olock, c, B_DO_NOT_RESCHEDULE)

/*
 * input_wait wakes up when the card has at least one packet on it
 */
#define input_wait(data)		acquire_sem(data->ilock)
#define input_unwait(data, c)		release_sem_etc(data->ilock, c, B_DO_NOT_RESCHEDULE)


/*
 * How many waiting for input?
 */
static long
input_count(e8255x_private_t *data)
{
	long count;

	get_sem_count(data->ilock, &count);
	return (count);
}

#if STAY_ON_PAGE_0

#define INTR_LOCK(data, expression) (expression)

#else /* STAY_ON_PAGE_0 */

/*
 * Spinlock for negotiating access to card with interrupt handler
 */
#define intr_lock(data)			acquire_spinlock(&data->intrlock)
#define intr_unlock(data)		release_spinlock(&data->intrlock)

/*
 * The interrupt handler must lock all calls to the card
 * This macro is useful for that purpose.
 */
#define INTR_LOCK(data, expression) (intr_lock(data), (expression), intr_unlock(data))

#endif /* STAY_ON_PAGE_0 */



/* Serial EEPROM section.
   A "bit" grungy, but we work our way through bit-by-bit :->. */
/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x01	/* EEPROM shift clock. */
#define EE_CS			0x02	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x04	/* EEPROM chip data in. */
#define EE_WRITE_0		0x01
#define EE_WRITE_1		0x05
#define EE_DATA_READ	0x08	/* EEPROM chip data out. */
#define EE_ENB			(0x4800 | EE_CS)

/* Delay between EEPROM clock transitions.
   This will actually work with no delay on 33Mhz PCI.  */
#define eeprom_delay(nanosec)		spin(1);

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD	(5 << 6)
#define EE_READ_CMD		(6 << 6)
#define EE_ERASE_CMD	(7 << 6)

static int read_eeprom(long ioaddr, int location)
{
	int i;
	unsigned short retval = 0;
	int ee_addr = ioaddr + SCBeeprom;
	int read_cmd = location | EE_READ_CMD;

	outw(EE_ENB & ~EE_CS, ee_addr);
	outw(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outw(EE_ENB | dataval, ee_addr);
		eeprom_delay(100);
		outw(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay(150);
	}
	outw(EE_ENB, ee_addr);

	for (i = 15; i >= 0; i--) {
		outw(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay(100);
		retval = (retval << 1) | ((inw(ee_addr) & EE_DATA_READ) ? 1 : 0);
		outw(EE_ENB, ee_addr);
		eeprom_delay(100);
	}

	/* Terminate the EEPROM access. */
	outw(EE_ENB & ~EE_CS, ee_addr);

kprintf ("read_ee: io=%x  loc=%x  val=%x\n",ioaddr, location, retval);
	return retval;
}

static int mdio_read(long ioaddr, int phy_id, int location)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	outl(0x08000000 | (location<<16) | (phy_id<<21), ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			kprintf(" mdio_read() timed out with val = %8.8x.\n", val);
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

static int mdio_write(long ioaddr, int phy_id, int location, int value)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	outl(0x04000000 | (location<<16) | (phy_id<<21) | value,
		 ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			kprintf(" mdio_write() timed out with val = %8.8x.\n", val);
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}


/*
 * Print an ethernet address
 */
static void
print_address(
			  ether_address_t *addr
			  )
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	kprintf("%s\n", buf);
}


/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void
speedo_init_rx_ring(e8255x_private_t *sp)
{
	struct RxFD *rxf, *last_rxf = NULL;
	int i;

	sp->cur_rx = 0;

	for (i = 0; i < RX_RING_SIZE; i++) {

		rxf = &(sp->rx_ringp[i].rxfd);
		if (last_rxf)
			last_rxf->link = virt_to_bus(rxf);
		last_rxf = rxf;
		rxf->status = 0x00000001; 			/* '1' is flag value only. */
		rxf->link = 0;						/* None yet. */
		/* This field unused by i82557, we use it as a consistency check. */
		rxf->rx_buf_addr = 0xffffffff;
		rxf->count = 0;
		rxf->size = PKT_BUF_SZ;
	}
	sp->dirty_rx = 0;
	/* Mark the last entry as end-of-list. */
	last_rxf->status = 0xC0000002; 			/* C is last & suspend, '2' is flag value only. */
	sp->last_rxf = last_rxf;
}

/* Set or clear the multicast filter for this adaptor.
   This is very ugly with Intel chips -- we usually have to execute an
   entire configuration command, plus process a multicast command.
   This is complicated.  We must put a large configuration command and
   an arbitrarily-sized multicast command in the transmit list.
   To minimize the disruption -- the previous command might have already
   loaded the link -- we convert the current command block, normally a Tx
   command, into a no-op and link it to the new command.
*/
static void
set_rx_mode(e8255x_private_t *sp)
{
	long ioaddr = sp->ioaddr;
	char new_rx_mode;
	unsigned long flags;
	int entry, i;


#ifdef LATER
//	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous. */
//		new_rx_mode = 3;
//	} else if ((dev->flags & IFF_ALLMULTI)  ||
//			   dev->mc_count > multicast_filter_limit) {
//		new_rx_mode = 1;
//	} else
		new_rx_mode = 0;

	if (sp->cur_tx - sp->dirty_tx >= TX_RING_SIZE - 1) {
	  /* The Tx ring is full -- don't add anything!  Presumably the new mode
		 is in config_cmd_data and will be added anyway. */
		sp->rx_mode = -1;
		return;
	}

	if (new_rx_mode != sp->rx_mode) {
		/* We must change the configuration. Construct a CmdConfig frame. */
		memcpy(sp->config_cmd_data, i82558_config_cmd,
			   sizeof(i82558_config_cmd));
		sp->config_cmd_data[1] = (txfifo << 4) | rxfifo;
		sp->config_cmd_data[4] = rxdmacount;
		sp->config_cmd_data[5] = txdmacount + 0x80;
		sp->config_cmd_data[15] |= (new_rx_mode & 2) ? 1 : 0;
		sp->config_cmd_data[19] |= sp->full_duplex ? 0x40 : 0;
		sp->config_cmd_data[21] = (new_rx_mode & 1) ? 0x0D : 0x05;
		if (sp->phy[0] & 0x8000) {			/* Use the AUI port instead. */
		  sp->config_cmd_data[15] |= 0x80;
		  sp->config_cmd_data[8] = 0;
		}
		save_flags(flags);
		cli();

		/* Fill the "real" tx_ring frame with a no-op and point it to us. */
		entry = sp->cur_tx++ % TX_RING_SIZE;
		sp->tx_skbuff[entry] = 0;	/* Nothing to free. */
		sp->tx_ring[entry].status = CmdNOp << 16;
		sp->tx_ring[entry].link = virt_to_bus(&sp->config_cmd);
		sp->config_cmd.status = 0;
		sp->config_cmd.command = CmdSuspend | CmdConfigure;
		sp->config_cmd.link =
		  virt_to_bus(&(sp->tx_ring[sp->cur_tx % TX_RING_SIZE]));
		sp->last_cmd->command &= ~CmdSuspend;
		/* Immediately trigger the command unit resume. */
		wait_for_cmd_done(ioaddr + SCBCmd);
		outw(CU_RESUME, ioaddr + SCBCmd);
		sp->last_cmd = &sp->config_cmd;
		restore_flags(flags);
//		if (speedo_debug > 5) 
		{
			int i;
			kprintf("setrx_mode: CmdConfig frame in entry %d.\n", entry);
			for(i = 0; i < 32; i++)
				kprintf(" %2.2x", ((unsigned char *)&sp->config_cmd)[i]);
			kprintf(".\n");
		}
	}

	if (new_rx_mode == 0  &&  sp->mc_count < 4) {
		/* The simple case of 0-3 multicast list entries occurs often, and
		   fits within one tx_ring[] entry. */
		struct dev_mc_list *mclist;
		uint16 *setup_params, *eaddrs;

		save_flags(flags);
		cli();
		entry = sp->cur_tx++ % TX_RING_SIZE;
		sp->tx_skbuff[entry] = 0;
		sp->tx_ring[entry].status = (CmdSuspend | CmdMulticastList) << 16;
		sp->tx_ring[entry].link =
		  virt_to_bus(&sp->tx_ring[sp->cur_tx % TX_RING_SIZE]);
		sp->tx_ring[entry].tx_desc_addr = 0; /* Really MC list count. */
		setup_params = (uint16 *)&sp->tx_ring[entry].tx_desc_addr;
		*setup_params++ = sp->mc_count*6;
		/* Fill in the multicast addresses. */
		for (i = 0, mclist = dev->mc_list; i < sp->mc_count;
			 i++, mclist = mclist->next) {
			eaddrs = (u16 *)mclist->dmi_addr;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
		}

		sp->last_cmd->command &= ~CmdSuspend;
		/* Immediately trigger the command unit resume. */
		wait_for_cmd_done(ioaddr + SCBCmd);
		outw(CU_RESUME, ioaddr + SCBCmd);
		sp->last_cmd = (struct descriptor *)&sp->tx_ring[entry];
		restore_flags(flags);
	} else if (new_rx_mode == 0) {
		struct dev_mc_list *mclist;
		u16 *setup_params, *eaddrs;
		struct descriptor *mc_setup_frm = sp->mc_setup_frm;
		int i;

		if (sp->mc_setup_frm_len < 10 + sp->mc_count*6
			|| sp->mc_setup_frm == NULL) {
			/* Allocate a new frame, 10bytes + addrs, with a few
			   extra entries for growth. */
			if (sp->mc_setup_frm)
				kfree(sp->mc_setup_frm);
			sp->mc_setup_frm_len = 10 + sp->mc_count*6 + 24;
			sp->mc_setup_frm = kmalloc(sp->mc_setup_frm_len, GFP_ATOMIC);
			if (sp->mc_setup_frm == NULL) {
			  kprintf(": Failed to allocate a setup frame.\n",
					 dev->name);
				sp->rx_mode = -1; /* We failed, try again. */
				return;
			}
		}
		mc_setup_frm = sp->mc_setup_frm;
		/* Construct the new setup frame. */
		if (speedo_debug > 1)
			kprintf("setrx_mode: Constructing a setup frame at %p, "
				   "%d bytes.\n",
				   sp->mc_setup_frm, sp->mc_setup_frm_len);
		mc_setup_frm->status = 0;
		mc_setup_frm->command = CmdSuspend | CmdIntr | CmdMulticastList;
		/* Link set below. */
		setup_params = (u16 *)&mc_setup_frm->params;
		*setup_params++ = sp->mc_count*6;
		/* Fill in the multicast addresses. */
		for (i = 0, mclist = dev->mc_list; i < sp->mc_count;
			 i++, mclist = mclist->next) {
			eaddrs = (u16 *)mclist->dmi_addr;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
		}

		/* Disable interrupts while playing with the Tx Cmd list. */
		save_flags(flags);
		cli();
		entry = sp->cur_tx++ % TX_RING_SIZE;

		if (speedo_debug > 5)
			kprintf(" CmdMCSetup frame length %d in entry %d.\n",
				   sp->mc_count, entry);

		/* Change the command to a NoOp, pointing to the CmdMulti command. */
		sp->tx_skbuff[entry] = 0;
		sp->tx_ring[entry].status = CmdNOp << 16;
		sp->tx_ring[entry].link = virt_to_bus(mc_setup_frm);

		/* Set the link in the setup frame. */
		mc_setup_frm->link =
		  virt_to_bus(&(sp->tx_ring[sp->cur_tx % TX_RING_SIZE]));

		sp->last_cmd->command &= ~CmdSuspend;
		/* Immediately trigger the command unit resume. */
		wait_for_cmd_done(ioaddr + SCBCmd);
		outw(CU_RESUME, ioaddr + SCBCmd);
		sp->last_cmd = mc_setup_frm;
		restore_flags(flags);
		if (speedo_debug > 1)
			kprintf("setrx_mode Last command at %p is %4.4x.\n",
				   sp->last_cmd, sp->last_cmd->command);
	}

	sp->rx_mode = new_rx_mode;

#endif LATER
}

/* Media monitoring and control. */
static void speedo_timer(e8255x_private_t *sp)
{
	int tickssofar = jiffies - sp->last_rx_time;

//	if (speedo_debug > 3) {
		kprintf( "speedo_timer: Media selection tick, status %4.4x.\n",
			    inw(sp->ioaddr + SCBStatus));
//	}
	if (sp->rx_bug) {
		if (tickssofar > 2*100  || sp->rx_mode < 0) {
			/* We haven't received a packet in a Long Time.  We might have been
			   bitten by the receiver hang bug.  This can be cleared by sending
			   a set multicast list command. */
			set_rx_mode(sp);
		}
		/* We must continue to monitor the media. */
		sp->timer.expires = 2*100; 			/* 2.0 sec. */
		add_timer(&sp->timer);
	}
}



/*
 * Copy a packet from the ethernet card
 */
static int
copy_packet(e8255x_private_t *data,unsigned char *ether_buf, int buflen)
{
	unsigned offset;
	int len;
	int rlen;
	int ether_len = 0;
	int didreset = 0;
	int resend = 0;

	return (ether_len);
}




/*
 * Initialize the ethernet card
 */
static char
init(
	 e8255x_private_t *sp
	 )
{
	char *product;
	int i;
	uint16 eeprom[0x40];

	kprintf("init: sp=%x ioaddr=%x\n",sp,sp->ioaddr);

	/* Read the station address EEPROM before doing the reset.
	   Perhaps this should even be done before accepting the device,
	   then we wouldn't have a device name with which to report the error. */
	{
		uint16 sum = 0;
		int j;
		for (j = 0, i = 0; i < 0x40; i++) {
			uint16 value = read_eeprom(sp->ioaddr, i);
//			kprintf(" %x ",value);
//			if ((j && 0xf)== 0) kprintf ("\n");
			eeprom[i] = value;
			sum += value;
			if (i < 3) {
				sp->myaddr[j++] = value;
				sp->myaddr[j++] = value >> 8;
			}
		}
		if (sum != 0xBABA)
			kprintf("e8255x init: Invalid EEPROM checksum %#4.4x, "
				   "check settings before activating this device!\n",
				 sum);
		/* Don't  unregister_netdev(dev);  as the EEPro may actually be
		   usable, especially if the MAC address is set later. */
	}

	/* Reset the chip: stop Tx and Rx processes and clear counters.
	   This takes less than 10usec and will easily finish before the next
	   action. */
	outl(0, sp->ioaddr + SCBPort);

	if (eeprom[3] & 0x0100)
		product = "OEM i82557/i82558 10/100 Ethernet";
	else
		product = "Intel EtherExpress Pro 10/100";

	kprintf("init: %s at ioaddr %#3lx, ",  product, sp->ioaddr);

	for (i = 0; i < 5; i++)
		kprintf("%2.2X:", sp->myaddr[i]);

#ifndef kernel_bloat
	/* OK, this is pure kernel bloat.  I don't like it when other drivers
	   waste non-pageable kernel space to emit similar messages, but I need
	   them for bug reports. */
	{
		const char *connectors[] = {" RJ45", " BNC", " AUI", " MII"};
		/* The self-test results must be paragraph aligned. */
		int32 *volatile self_test_results;
		int boguscnt = 16000;	/* Timeout for set-test. */
		if (eeprom[3] & 0x03)
			kprintf("  Receiver lock-up bug exists -- enabling"
				   " work-around.\n");
		kprintf("  Board assembly %4.4x%2.2x-%3.3d, Physical"
			   " connectors present:",
			   eeprom[8], eeprom[9]>>8, eeprom[9] & 0xff);
		for (i = 0; i < 4; i++)
			if (eeprom[5] & (1<<i))
				kprintf(connectors[i]);
		kprintf("\n Primary interface chip %s PHY #%d.\n",
			   phys[(eeprom[6]>>8)&15], eeprom[6] & 0x1f);
		if (eeprom[7] & 0x0700)
			kprintf("    Secondary interface chip %s.\n",
				   phys[(eeprom[7]>>8)&7]);
		if (((eeprom[6]>>8) & 0x3f) == DP83840
			||  ((eeprom[6]>>8) & 0x3f) == DP83840A) {
			int mdi_reg23 = mdio_read(sp->ioaddr, eeprom[6] & 0x1f, 23) | 0x0422;
			if (congenb)
			  mdi_reg23 |= 0x0100;
			kprintf("  DP83840 specific setup, setting register 23 to %4.4x.\n",
				   mdi_reg23);
			mdio_write(sp->ioaddr, eeprom[6] & 0x1f, 23, mdi_reg23);
		}
//		if ((option >= 0) && (option & 0x70)) {
//			kprintf("  Forcing %dMbs %s-duplex operation.\n",
//				   (option & 0x20 ? 100 : 10),
//				   (option & 0x10 ? "full" : "half"));
//			mdio_write(sp->ioaddr, eeprom[6] & 0x1f, 0,
//					   ((option & 0x20) ? 0x2000 : 0) | 	/* 100mbps? */
//					   ((option & 0x10) ? 0x0100 : 0)); /* Full duplex? */
//		}

		/* Perform a system self-test. */
		self_test_results = (int32*) ((((long) sp->SelfTest) + 15) & ~0xf);
		self_test_results[0] = 0;
		self_test_results[1] = -1;
		outl(virt_to_bus(self_test_results) | 1, sp->ioaddr + SCBPort);
		do {
			spin(10);
		} while (self_test_results[1] == -1  &&  --boguscnt >= 0);

		if (boguscnt < 0) {		/* Test optimized out. */
			kprintf("Self test failed, status %8.8x:\n"
				    " Failure to initialize the i82557.\n"
				   	" Verify that the card is a bus-master"
				    " capable slot.\n",
				   self_test_results[1]);
		} else
			kprintf( "  General self-test: %s.\n"
				    "  Serial sub-system self-test: %s.\n"
				    "  Internal registers self-test: %s.\n"
				    "  ROM checksum self-test: %s (%#8.8x).\n",
				   self_test_results[1] & 0x1000 ? "failed" : "passed",
				   self_test_results[1] & 0x0020 ? "failed" : "passed",
				   self_test_results[1] & 0x0008 ? "failed" : "passed",
				   self_test_results[1] & 0x0004 ? "failed" : "passed",
				   self_test_results[0]);
	}
#endif  /* kernel_bloat */

	outl(0, sp->ioaddr + SCBPort);


	sp->phy[0] = eeprom[6];
	sp->phy[1] = eeprom[7];
	sp->rx_bug = (eeprom[3] & 0x03) == 3 ? 0 : 1;

	if (sp->rx_bug)
		kprintf("  Receiver lock-up workaround activated.\n");

// to do: Make probe and Init functions separate...
//	MOD_INC_USE_COUNT;

	/* Retrigger negotiation to reset previous errors. */
	if ((sp->phy[0] & 0x8000) == 0) {
		int phy_addr = sp->phy[0] & 0x1f;
		/* Use 0x3300 for restarting NWay, other values to force xcvr:
		   0x0000 10-HD
		   0x0100 10-FD
		   0x2000 100-HD 
		   0x2100 100-FD
		   0x
		*/
#ifdef notdef
		int mii_ctrl[8] =
		{ 0x3300, 0x3100, 0x0000, 0x0100, 0x2000, 0x2100, 0x0400, 0x3100};
		mdio_write(ioaddr, phy_addr, 0, mii_ctrl[dev->default_port & 7]);
#else
		mdio_write(sp->ioaddr, phy_addr, 0, 0x3300);
#endif
	}

	/* Load the statistics block address. */
	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outl(virt_to_bus(&sp->lstats), sp->ioaddr + SCBPointer);
	outw(INT_MASK | CU_STATSADDR, sp->ioaddr + SCBCmd);
	sp->lstats.done_marker = 0;

	speedo_init_rx_ring(sp);
	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outl(0, sp->ioaddr + SCBPointer);
	outw(INT_MASK | RX_ADDR_LOAD, sp->ioaddr + SCBCmd);

	/* Todo: verify that we must wait for previous command completion. */
	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outl(virt_to_bus(sp->rx_ringp[0].rx_data), sp->ioaddr + SCBPointer);
	outw(INT_MASK | RX_START, sp->ioaddr + SCBCmd);

	/* Fill the first command with our physical address. */
	{
		uint16 *eaddrs = (uint16 *)sp->myaddr;
		uint16 *setup_frm = (uint16 *)&(sp->tx_ringp[0].txfd.tx_desc_addr);

		/* Avoid a bug(?!) here by marking the command already completed. */
		sp->tx_ringp[0].txfd.status = ((CmdSuspend | CmdIASetup) << 16) | 0xa000;
		sp->tx_ringp[0].txfd.link = virt_to_bus(&(sp->tx_ringp[1]));
		*setup_frm++ = eaddrs[0];
		*setup_frm++ = eaddrs[1];
		*setup_frm++ = eaddrs[2];
	}
	sp->last_cmd = (struct descriptor *)&sp->tx_ringp[0].txfd;
	sp->cur_tx = 1;
	sp->dirty_tx = 0;
	sp->tx_full = 0;

	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outl(0, sp->ioaddr + SCBPointer);
	outw(INT_MASK | CU_CMD_BASE, sp->ioaddr + SCBCmd);

//	dev->if_port = sp->default_port;

	sp->tbusy = 0;

	/* Start the chip's Tx process and unmask interrupts. */
	/* Todo: verify that we must wait for previous command completion. */
	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outl(virt_to_bus(&sp->tx_ringp[0]), sp->ioaddr + SCBPointer);
	outw(CU_START, sp->ioaddr + SCBCmd);

	/* Setup the chip and configure the multicast list. */
	sp->mc_setup_frm = NULL;
	sp->mc_setup_frm_len = 0;
	sp->rx_mode = -1;			/* Invalid -> always reset the mode. */
	set_rx_mode(sp);

		kprintf( "init: status = %8.8x.\n",
			   inw(sp->ioaddr + SCBStatus));

	/* Set the timer.  The timer serves a dual purpose:
	   1) to monitor the media interface (e.g. link beat) and perhaps switch
	   to an alternate media type
	   2) to monitor Rx activity, and restart the Rx process if the receiver
	   hangs. */
	init_timer(&sp->timer);
	sp->timer.expires = (24*100)/10; 			/* 2.4 sec. */
	sp->timer.data = (unsigned long)sp;
	sp->timer.function = (void (*)(unsigned long))&speedo_timer;					/* timer handler */
	add_timer(&sp->timer);

	wait_for_cmd_done(sp->ioaddr + SCBCmd);
	outw(CU_DUMPSTATS, sp->ioaddr + SCBCmd);
	return 0;

}


e8255x_rx(e8255x_private_t *sp)
{
	int entry = sp->cur_rx % RX_RING_SIZE;
	int status;
	int rxcount = 0;
	int rx_work_limit = sp->dirty_rx + RX_RING_SIZE - sp->cur_rx;

	if (speedo_debug > 4)
		kprintf(" In e8255x_rx().\n");
	/* If we own the next entry, it's a new packet. Send it up. */
	while ((status = sp->rx_ringp[entry].rxfd.status) & RxComplete) {

		if (speedo_debug > 4)
			kprintf("e8255x_rx() status %8.8x len %d.\n", status,
				   sp->rx_ringp[entry].rxfd.count & 0x3fff);
		if ((status & (RxErrTooBig|RxOK)) != RxOK) {
			if (status & RxErrTooBig)
				kprintf("e8255x_rx: Ethernet frame overran the Rx buffer, "
					   "status %8.8x!\n", status);
			else if ( ! (status & 0x2000)) {
				/* There was a fatal error.  This *should* be impossible. */
				sp->stats.rx_errors++;
				kprintf("e8255x_rx: Anomalous event in e8255x_rx(), "
					   "status %8.8x.\n", status);
			}
		} else {
			int pkt_len = sp->rx_ringp[entry].rxfd.count & 0x3fff;
			sp->stats.rx_packets++;
			rxcount++;
			release_sem_etc(sp->rx_sem, rxcount, B_DO_NOT_RESCHEDULE);

		}
		entry = (++sp->cur_rx) % RX_RING_SIZE;
		if (--rx_work_limit < 0)
			break;
	}

	return 0;
}


/*
 * Handle ethernet interrupts
 */
static int32
e8255x_interrupt(void *_data)
{
	e8255x_private_t *sp = (e8255x_private_t *) _data;
	unsigned char isr;
	int wakeup_reader = 0;
	int wakeup_writer = 0;
	int32 handled = B_UNHANDLED_INTERRUPT;
	
	long ioaddr, boguscnt = max_interrupt_work;
	unsigned short status;

	acquire_spinlock (&sp->hw_lock);

	ioaddr = sp->ioaddr;

	do {
		status = inw(ioaddr + SCBStatus);
		/* Acknowledge all of the current interrupt sources ASAP. */
		outw(status & 0xfc00, ioaddr + SCBStatus);
		
		if (speedo_debug > 4)
			kprintf( "e8255x_int:  status=%#4.4x.\n", status);

		if ((status & 0xfc00) == 0)
			break;
		
		handled = B_HANDLED_INTERRUPT;
		if (status & 0x4000) {	 /* Packet received. */
			e8255x_rx(sp);
		}
		if (status & 0x1000) {
		  if ((status & 0x003c) == 0x0028) /* No more Rx buffers. */
			outw(RX_RESUMENR, ioaddr + SCBCmd);
		  else if ((status & 0x003c) == 0x0008) { /* No resources (why?!) */
			/* No idea of what went wrong.  Restart the receiver. */
			outl(virt_to_bus(&(sp->rx_ringp[sp->cur_rx % RX_RING_SIZE])),
				 ioaddr + SCBPointer);
			outw(RX_START, ioaddr + SCBCmd);
		  }
		  sp->stats.rx_errors++;
		}

		/* User interrupt, Command/Tx unit interrupt or CU not active. */
		if (status & 0xA400) {
			unsigned int dirty_tx = sp->dirty_tx;
			unsigned int txcount = 0;

			while (sp->cur_tx - dirty_tx > 0) {
				int entry = dirty_tx % TX_RING_SIZE;
				int status = sp->tx_ringp[entry].txfd.status;

				if (speedo_debug > 5)
					kprintf(" scavenge candidate %d status %4.4x.\n",
						   entry, status);
				if ((status & 0x8000) == 0)
					break;			/* It still hasn't been processed. */
				txcount++;
				release_sem_etc(sp->tx_sem, txcount, B_DO_NOT_RESCHEDULE);
				dirty_tx++;
			}

			sp->dirty_tx = dirty_tx;
		}

		if (--boguscnt < 0) {
			kprintf("%ISR: Too much work at interrupt, status=0x%4.4x.\n", status);
			/* Clear all interrupt sources. */
			outl(0xfc00, ioaddr + SCBStatus);
			break;
		}
	} while (1);

	release_spinlock (&sp->hw_lock);
	if (speedo_debug > 3)
		kprintf("%ISR: exiting interrupt, status=%#4.4x.\n", inw(ioaddr + SCBStatus));
	return handled;

}


/*
 * Implements the read() system call to the ethernet driver
 */
static status_t 
e8255x_read(
		   void *_data,
		   off_t pos,
		   void *buf,
		   size_t *len
		   )
{
	e8255x_private_t *data = (e8255x_private_t *) _data;
	ulong buflen;
	int packet_len;

	if (!data->init) {
		return (B_ERROR);
	}
	buflen = *len;
	atomic_add(&data->inrw, 1);
	if (data->interrupted) {
		atomic_add(&data->inrw, -1);
		return (B_INTERRUPTED);
	}
	do {
		if (!data->nonblocking) {
			input_wait(data);
		}
		if (data->interrupted) {
			atomic_add(&data->inrw, -1);
			return (B_INTERRUPTED);
		}
		packet_len = copy_packet(data, (unsigned char *)buf, buflen);
	} while (!data->nonblocking && packet_len == 0 /*&& !my_packet(data, buf) */);
	atomic_add(&data->inrw, -1);
	*len = packet_len;


	return 0;
}

/*
 * implements the write() system call to the ethernet
 */
static status_t
e8255x_write(
			void *_data,
			off_t pos,
			const void *buf,
			size_t *len
			)
{
	e8255x_private_t *sp = (e8255x_private_t *) _data;
	int status;
	long ioaddr = sp->ioaddr;
	int entry;
	ulong buflen;
	status_t err;
	
	if (!sp->init) {
		return (B_ERROR);
	}

	buflen = *len;
	
	err = acquire_sem(sp->tx_sem);
	if (err < B_NO_ERROR) {
		kprintf("e8255x_write: %08x\n", err);
		return (err);
	}

	entry = sp->cur_tx++ % TX_RING_SIZE;

	/* Todo: be a little more clever about setting the interrupt bit. */
	sp->tx_ringp[entry].txfd.status =
		(CmdSuspend | CmdTx | CmdTxFlex) << 16;
	sp->tx_ringp[entry].txfd.link =
	virt_to_bus(&sp->tx_ringp[sp->cur_tx % TX_RING_SIZE]);
	sp->tx_ringp[entry].txfd.tx_desc_addr =
	virt_to_bus(&sp->tx_ringp[entry].txfd.tx_buf_addr0);
	/* The data region is always in one buffer descriptor, Tx FIFO
		   threshold of 256. */
	sp->tx_ringp[entry].txfd.count = 0x01208000;
	sp->tx_ringp[entry].txfd.tx_buf_addr0 = virt_to_bus(sp->tx_ringp[entry].tx_data);
	//	sp->tx_ring[entry].tx_buf_size0 = skb->len;
	sp->tx_ringp[entry].txfd.tx_buf_size0 = buflen;
	/* Todo: perhaps leave the interrupt bit set if the Tx queue is more
		  than half full.  Argument against: we should be receiving packets
		  and scavenging the queue.  Argument for: if so, it shouldn't
		  matter. */
	sp->last_cmd->command &= ~(CmdSuspend | CmdIntr);
		sp->last_cmd = (struct descriptor *)&sp->tx_ringp[entry].txfd;
	/* Trigger the command unit resume. */
	wait_for_cmd_done(ioaddr + SCBCmd);
	outw(CU_RESUME, ioaddr + SCBCmd);



	/* Leave room for set_rx_mode() to fill two entries. */
	if (sp->cur_tx - sp->dirty_tx > TX_RING_SIZE - 3)
		sp->tx_full = 1;

	sp->trans_start = jiffies;

	return 0;
}


static status_t
e8255x_open(const char *name, uint32 flags, void **_cookie) {
	e8255x_private_t **cookie = (e8255x_private_t **) _cookie;
	e8255x_private_t *data;

	kprintf("e8255x_open\n");

	// 99% of this is tx & rx buffer space, just allocate it all in one place
	// it makes debugging easier too.
	data = (e8255x_private_t *) get_area( (size_t) sizeof(e8255x_private_t), &e8255x_area);
	if (!data) {
		kprintf("e8255x_open: No Memory!!\n");
		return B_NO_MEMORY;

	}
	init_area(data);

	data->init = FALSE;
	*cookie = data;
	return B_NO_ERROR;
}


static status_t
e8255x_close(void *_data) {
	e8255x_private_t *data = (e8255x_private_t *) _data;
	
	
	if (!data->init) {	// We didn't find a card: No need to do anything
		return (B_NO_ERROR);
	}
	 
	/*
	 * Stop the chip
	 */
	kprintf("e8255x_close: reset chip here\n");
	
	remove_io_interrupt_handler(data->irq_level, e8255x_interrupt, data);


	delete_sem(data->rx_sem);
	delete_sem(data->tx_sem);
	
	/*
	 * Reset to notify others that this port and irq is available again
	 */
	ether_data[data->irq_level] = NULL;
	return (B_NO_ERROR);
}

static status_t
e8255x_free(
		   void *_data
		  )
{
	e8255x_private_t *data = (e8255x_private_t *) _data;
	
	kprintf("e8255x_free: %x \n",e8255x_area);
	delete_area(e8255x_area);

	return 0;
}


static int
e8255x_init(
		   e8255x_private_t *data,
		   int ioaddr,
		   int irq_level,
		   area_id ioarea
		   )
{
	int i;


	/*
	 * Can't init twice
	 */
	if (data->init) {
		kprintf("already inited!\n");
		return (B_ERROR);
	}

	/*
	 * Sanity checking: did you forget to specify a port?
	 */
	if (ioaddr == 0) {
		kprintf("port == 0\n");
		return (B_ERROR);
	} 
	/*
	 * Sanity check: is irq taken?
	 */
	if (ether_data[irq_level]) {
		kprintf("invalid irq: %d in use\n", irq_level);
		return (B_ERROR);
	}
	ether_data[irq_level] = data;

	data->ioaddr = ioaddr;

	kprintf("etherpci: probing at address %03x (irq %d).\n", ioaddr, irq_level);


	// create locks
	
	data->hw_lock = 0;					/* spinlock for hw access */

	data->rx_sem = B_ERROR;
	data->tx_sem = B_ERROR;
	
	data->tx_sem = create_sem(1, "e8255x tx");
	if (data->tx_sem < B_NO_ERROR) {
		kprintf("No tx sems!\n");
		return (B_ERROR);
	}
	data->rx_sem = create_sem(1, "e8255x rx");
	if (data->rx_sem < B_NO_ERROR) {
		kprintf("No tx sems!\n");
		return (B_ERROR);
	}

	data->irq_level = irq_level;
	data->nonblocking = 0;
	data->init = TRUE;
	init(data);
	ports_for_irqs[irq_level] = ioaddr;
	install_io_interrupt_handler(irq_level, e8255x_interrupt, data, 0);
	return (B_NO_ERROR);
}

status_t find_e8255x_card(short *irq_level, long **ioaddr, area_id *ioarea)
{
	// Temporary vars
	int				i;
	
	// General vars
	unsigned short	cmd;
	pci_info		info;
#if !__INTEL__
	unsigned long	phys;
	unsigned long	size;
#endif

	//
	// Look for one of the supported NE2000 PCI ethernet cards.
	for( i = 0; ; i++ ) {
		//
		// Get the card info.
		if ((*pci->get_nth_pci_info)(i, &info) != B_NO_ERROR)
			break;
					// We found a card.
			kprintf("find_e8255x_card: VendId=0x%04x DevId=0x%04x, Bus=%d, Dev=%d, Fn=%d\n",
						info.vendor_id, info.device_id, info.bus, info.device, info.function);
			

		//
		// See if this matches one of our cards.
		if( ((info.vendor_id == 0x8086) && (info.device_id == 0x1229))		// Intel 82557
			|| ((info.vendor_id == 0x8086) && (info.device_id == 0x122a))	// Intel 82558 !!! Just a guess, verify!!!!
		){
			//
			//
			// Make sure that we haven't already seen this card.
			if( ether_data[info.u.h0.interrupt_line] ) {
				kprintf("e8255x: card already initialized; skipping\n");
				continue;
			}
			
			//
			// Set up the card.
			(*irq_level) = info.u.h0.interrupt_line;
#if __INTEL__
			(*ioaddr) = (long*)info.u.h0.base_registers[1];

#else
			// (PowerPC machines)
			// Map some physical block(s) for i/o regs
			phys = info.u.h0.base_registers[0];
			size = info.u.h0.base_register_sizes[0];
			size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);	// round up

			(*ioarea) = map_physical_memory("e8255x_regs",
											(void *)(phys & ~(B_PAGE_SIZE - 1)),
											size,
											B_ANY_KERNEL_ADDRESS,
											B_READ_AREA + B_WRITE_AREA,
											ioaddr);
			if( (*ioarea) < 0 ) {
				kprintf("e8255x: problems mapping device regs: %08x\n", (*ioarea));
				return (B_ERROR);
			}
			
			//
			// Correct for offset w/in page
			(*ioaddr) += (phys & (B_PAGE_SIZE - 1)) / sizeof(ulong);
		
			kprintf(" ioaddr=%08x, ioarea=%08x\n", (*ioaddr), (*ioarea));
#endif
			kprintf("e8255x ioaddr=%08x, irq_level=%08x\n", (*ioaddr), (*irq_level));
		
			// Remove I/O space marker bit 
			if ((long)*ioaddr & 3) {
				*ioaddr = (long *) ((long) *ioaddr & ~3);
				kprintf("e8255x fixing ioaddr=%08x\n", (*ioaddr));
			}
			//
			// Enable io transactions and bus mastering
			cmd = (*pci->read_pci_config)(info.bus, info.device, info.function, PCI_command, 2);
			
			(*pci->write_pci_config)(info.bus, info.device, info.function, 
				PCI_command, 2, cmd | PCI_command_io | 0x4);		// 4 = Bus Master
			
			//
			// check latency
			cmd = (*pci->read_pci_config)(info.bus, info.device, info.function,PCI_latency, 1);
			cmd &= 0xff;
			kprintf("latency is %x \n", cmd);
			if (cmd < 64) {
				kprintf("latency unreasonably low changing %x to %x\n", cmd, 64);
				cmd = 64; 
				(*pci->write_pci_config)(info.bus, info.device, info.function,PCI_latency, 1, cmd);
			}

			//
			// We found a card; exit.
			return B_NO_ERROR;
		}
	}

	//
	// We couldn't find a supported card.
	kprintf("etherpci: no supported NE2000 PCI cards found\n");
	return B_ERROR;
}


/*
 * Standard driver control function
 */
static status_t
e8255x_control(
			void *_data,
			uint32 msg,
			void *buf,
			size_t len
			)
{
	e8255x_private_t *data = (e8255x_private_t *) _data;
	short irq_level;
	long *ioaddr;
	area_id ioarea;

	kprintf("RHS e8255x _control %x\n",msg);

	switch (msg) {
	case ETHER_INIT:
		//
		// Find a card.
		if( find_e8255x_card(&irq_level, &ioaddr, &ioarea) )
			return B_ERROR;
		
		//
		// Init the card.
		return (e8255x_init(data, (long)ioaddr, irq_level, ioarea));

	case ETHER_GETADDR:
		if (data == NULL) {
			return (B_ERROR);
		}
		memcpy(buf, &data->myaddr, sizeof(data->myaddr));
		return (B_NO_ERROR);

	case ETHER_NONBLOCK:
		if (data == NULL) {
			return (B_ERROR);
		}
		memcpy(&data->nonblocking, buf, sizeof(data->nonblocking));
		return (B_NO_ERROR);

	case ETHER_ADDMULTI:
		return (domulti(data, (char *)buf));
	}
	return B_ERROR;
}



static int
domulti(
		e8255x_private_t *data,
		char *addr
		)
{
	return (B_NO_ERROR);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */

status_t
init_driver()
{
	return get_module(pci_name, (module_info **)&pci);
}

void
uninit_driver()
{
	put_module(pci_name);
}


/* -----
	driver-related structures
----- */

static char *e8255x_name[] = { "net/e8255x", NULL };

static device_hooks e8255x_device =  {
	e8255x_open, 			/* -> open entry point */
	e8255x_close,			/* -> close entry point */
	e8255x_free,			/* -> free entry point */
	e8255x_control, 		/* -> control entry point */
	e8255x_read,			/* -> read entry point */
	e8255x_write,			/* -> write entry point */
	NULL,					/* -> select entry point */
	NULL					/* -> deselect entry point */
};


const char **
publish_devices()
{
	return e8255x_name;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;

device_hooks *
find_device(const char *name)
{
	return &e8255x_device;
}
