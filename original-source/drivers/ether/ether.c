/*
 * ether.c
 * Copyright (c) 1995 Be, Inc.	All Rights Reserved 
 *
 * Ethernet driver: handles NE2000 and 3C503 cards
 *
 * Modification History (most recent first):
 *
 * 16 jan 96	bat		major rework for settable irqs & ports
 * 11 may 95	bat		can either stay on page 0 or switch pages with ifdef
 * 03 may 95	bat		make it stay on page 0
 * 23 mar 95	bat		ported to kernel
 * ?? ??? ??	bat		new today
 */
#include <OS.h>
#include <KernelExport.h>
#include <ISA.h>
#include <Drivers.h>
#include "ether_private.h"
#include <ether_driver.h>
#include <stdarg.h>

/*
 * Define STAY_ON_PAGE_0 if you want the driver to stay on page 0 all the
 * time.  This is faster than if it has to switch to page 1 to get the
 * current register to see if there are any more packets.  Reason: the
 * interrupt handler does not have to do any locking if it knows it is
 * always on page 0.  I'm not sure why the speed difference is so dramatic,
 * so perhaps there is actually a bug in page-changing version.
 *
 * STAY_ON_PAGE_0 uses a rather dubious technique to avoid changing the
 * register page and it may not be 100% reliable.  The technique used is to
 * make sure all ring headers are zeroed out before packets are received
 * into them.  Then, if you detect a non-zero ring header, you can be pretty
 * sure that it is another packet. 
 * 
 * We never read the "might-be-a-packet" immediately.  Instead, we just
 * release a semaphore so that the next read will occur later on enough
 * so that the ring header information should be completely filled in.
 * 
 */
#define STAY_ON_PAGE_0	0


/*
 * We only care about these interrupts in our driver
 */
#define INTS_WE_CARE_ABOUT (ISR_RECEIVE | ISR_RECEIVE_ERROR |\
							ISR_TRANSMIT | ISR_TRANSMIT_ERROR)


typedef struct ether_private {
	int	init;				/* already inited? */
	short irq_level;		/* our ISA IRQ level */
	short port;				/* our ISA I/O port number */
	unsigned long mem;		/* our ISA mem address */
	cardtype_t cardtype;	/* card-type: 3com or ne2000 */
	int boundary;			/* boundary register value (mirrored) */
	ether_address_t myaddr;	/* my ethernet address */
	unsigned nmulti;		/* number of multicast addresses */
	ether_address_t multi[MAX_MULTI];	/* multicast addresses */
	sem_id iolock;			/* ethercard io, except interrupt handler */
	int nonblocking;		/* non-blocking mode */
#if !STAY_ON_PAGE_0
	spinlock intrlock;		/* ethercard io, including interrupt handler */
#endif /* !STAY_ON_PAGE_0 */
	volatile int interrupted;   /* interrupted system call */
	sem_id inrw;			/* in read or write function */
	sem_id ilock;			/* waiting for input */
	sem_id olock;	 		/* waiting to output */

	/*
	 * Various statistics
	 */
	volatile ints;	/* total number of interrupts */
	volatile rints;	/* read interrupts */
	volatile wints;	/* write interrupts */
	volatile reads;	/* reads */
	volatile writes;	/* writes */
	volatile resets;	/* resets */
	volatile rerrs;	/* read errors */
	volatile werrs;	/* write errors */
	volatile interrs;/* unknown interrupts */
	volatile frame_errs;	/* frame alignment errors */
	volatile crc_errs;	/* crc errors */
	volatile frames_lost;/* frames lost due to buffer problems */

	/*
	 * Most recent values of the error statistics to detect any changes in them
	 */
	int rerrs_last;	
	int werrs_last;	
	int interrs_last;
	int frame_errs_last;
	int crc_errs_last;
	int frames_lost_last;

	/*
	 * These values are set once and never looked at again.  They are
	 * almost as good as constants, but they differ for the various
	 * cards, so we can't set them now.
	 */
	int ETHER_BUF_START;
	int ETHER_BUF_SIZE;
	int EC_VMEM_PAGE;
	int EC_VMEM_NPAGES;
	int EC_RXBUF_START_PAGE;
	int EC_RXBUF_END_PAGE;
	int EC_RINGSTART;
	int EC_RINGSIZE;
#if !__INTEL__
	char *ISA_IO_BASE;
#endif
	char *ISA_MEM_BASE;
} ether_private_t;

/*
 * One for each IRQ
 */
static ether_private_t *ether_data[NIRQS];
static short ports_for_irqs[NIRQS];


static int noprintf(const char *fmt, ...) { return (1); }

static isa_module_info	*isa;

/*
 * io_lock gets you exclusive access to the card, except that 
 * the interrupt handler can still run.
 * There is probably no need to io_lock() a 3com card, so look into
 * removing it for that case.
 */
#define io_lock(data)			acquire_sem(data->iolock)
#define io_unlock(data)			release_sem_etc(data->iolock, 1, B_DO_NOT_RESCHEDULE)

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


#if __INTEL__
const char		isa_name[] = B_ISA_MODULE_NAME;
#endif


/*
 * How many waiting for input?
 */
static long
input_count(ether_private_t *data)
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


/*
 * Calculate various constants
 * These must be done at runtime, since 3com and ne2000 cards have different
 * values.
 */
static void
calc_constants(ether_private_t *data)
{
	data->EC_VMEM_PAGE = (data->ETHER_BUF_START >> EC_PAGE_SHIFT);
	data->EC_VMEM_NPAGES = (data->ETHER_BUF_SIZE >> EC_PAGE_SHIFT);

	data->EC_RXBUF_START_PAGE = (data->EC_VMEM_PAGE + 6);
	data->EC_RXBUF_END_PAGE = (data->EC_VMEM_PAGE + data->EC_VMEM_NPAGES);

	data->EC_RINGSTART = (data->EC_RXBUF_START_PAGE << EC_PAGE_SHIFT);
	data->EC_RINGSIZE = ((data->EC_VMEM_NPAGES - 6) << EC_PAGE_SHIFT);
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
	wprintf("%s\n", buf);
}


#define threecom_addr(data, x)  ETHER_MEM(data, (x) - data->ETHER_BUF_START)


/*
 * Copy using shorts to insure 16-bit transfers on the ISA bus
 * 
 * Assumptions: everything is short-aligned, len is short-multiple
 */
static void
shortcopy(
		  void *vdst,
		  const void *vsrc,
		  int len
		  )
{
	const short *src = (short *)vsrc;
	short *dst = (short *)vdst;
	const short *done = (const short *)((char *)vsrc + len);

	while (src < done) {
		*dst++ = *src++;
	}
}

/*
 * Get data from the 3com card
 */
static void
ether_min_3com(
			   ether_private_t *data,
			   unsigned char *dst, unsigned src, unsigned len)
{
	shortcopy(dst, (void *)threecom_addr(data, src), len);
}

/*
 * Put data on the 3com card
 */
static void
ether_mout_3com(
				ether_private_t *data,
				unsigned dst, const unsigned char *src, unsigned len)
{
	shortcopy((void *)threecom_addr(data, dst), src, len);
}



/*
 * Get the isr register
 */
static unsigned char
getisr(ether_private_t *data)
{
	unsigned char isr;

	isr = ether_inb(data, EN0_ISR);
	return (isr);
}

/*
 * Set the isr register
 */
static void
setisr(ether_private_t *data, unsigned char isr)
{
	ether_outb(data, EN0_ISR, isr);
}

/*
 * Wait for the DMA to complete
 */
static int
wait_for_dma_complete(
					  ether_private_t *data,
					  unsigned short addr,
					  unsigned short size
					  )
{
	unsigned short where;
	int bogus;

	//dprintf("wait_for_dma_complete : addr 0x%x size %d\n",addr,size);
#define MAXBOGUS 20

	/*
	 * This is the expected way to wait for DMA completion, which
	 * is in fact incorrect. I think ISR_DMADONE gets set too early.
	 */
	bogus = 0;
	while (!(getisr(data) & ISR_DMADONE) && ++bogus < MAXBOGUS) {
		/* keep waiting */
	}
	if (bogus >= MAXBOGUS) {
		wprintf("Bogus alert: waiting for ISR\n");
	}

	/*
	 * This is the workaround
	 */
	bogus = 0;
	do {
		where = (ether_inb(data, EN0_RADDRHI) << 8) | ether_inb(data, EN0_RADDRLO);
	} while (where < addr + size && ++bogus < MAXBOGUS);
	if (bogus >= MAXBOGUS) {
		/*
		 * On some cards, the counters will never clear. 
		 * So only print this message when debugging.
		 */
		ddprintf("Bogus alert: waiting for counters to zero\n");
		//dprintf("wait_for_dma_complete : addr 0x%x size %d\n",addr,size);
		//dprintf("where is 0x%x\n", where);
		return -1;
	}
	
	setisr(data, ISR_DMADONE);
	ether_outb(data, EN_CCMD, ENC_NODMA);
	return 0;
}

/*
 * Get data from the ne2000 card
 */
static void
ether_min_ne2000(
				 ether_private_t *data, 
				 unsigned char *dst, 
				 unsigned src, 
				 unsigned len
				 )
{
	int i;


	if (len & 1) {
		len++;
	}
	ether_outb(data, EN0_RCNTLO, len & 0xff);
	ether_outb(data, EN0_RCNTHI, len >> 8); 
	ether_outb(data, EN0_RADDRLO, src & 0xff); 
	ether_outb(data, EN0_RADDRHI, src >> 8); 
	ether_outb(data, EN_CCMD, ENC_DMAREAD);
	for (i = 0; i < len; i += 2) {
		unsigned short word;

		word = ether_inw(data, NE_DATA);
#if __INTEL__
		dst[i + 1] = word >> 8;
		dst[i + 0] = word & 0xff;
#else
		dst[i] = word >> 8;
		dst[i + 1] = word & 0xff;
#endif
	}
	wait_for_dma_complete(data, src, len);
}

/*
 * Put data on the ne2000 card
 */
static void
ether_mout_ne2000(
				  ether_private_t *data, 
				  unsigned dst, 
				  const unsigned char *src, 
				  unsigned len
				  )
{
	int i;
	int tries = 1;

	
	// This loop is for a bug that showed up with the old ISA 3com cards
	// that were in the original BeBox.  Sometimes the dma status would just
	// stop on some part of the buffer, never finishing.
	// If we notice this error, we redo the dma transfer.
again:
#define MAXTRIES 2
	if (tries > MAXTRIES) {
		dprintf("ether_mout_ne2000 : tried %d times, stopping\n");
		return;
	}
	
	if (len & 1) {
		len++;
	}
	ether_outb(data, EN0_RCNTHI, len >> 8);
	ether_outb(data, EN0_RADDRLO, dst & 0xff);
	ether_outb(data, EN0_RADDRHI, dst >> 8);

	/*
	 * The 8390 hardware has documented bugs in it related to DMA write.
	 * So, we follow the documentation on how to work around them.
	 */

	/*
	 * Step 1: You must put a non-zero value in this register. We use 2.
	 */
	if ((len & 0xff) == 0) {
		ether_outb(data, EN0_RCNTLO, 2);
	}  else {
		ether_outb(data, EN0_RCNTLO, len & 0xff);
	}

#if you_want_to_follow_step2_even_though_it_hangs
	ether_outb(data, EN_CCMD, ENC_DMAREAD);				/* Step 2 */
#endif

	ether_outb(data, EN_CCMD, ENC_DMAWRITE);

	for (i = 0; i < len; i += 2) {
		unsigned short word;

#if __INTEL__
		word = (src[i + 1] << 8) | src[i + 0];
#else
		word = (src[i] << 8) | src[i + 1];
#endif
		ether_outw(data, NE_DATA, word);
	}
	if ((len & 0xff) == 0) {
		/*
		 * Write out the two extra bytes
		 */
		ether_outw(data, NE_DATA, 0);
		len += 2;
	}
	if (wait_for_dma_complete(data, dst, len) != 0) {
		tries++;
		goto again;
	}
	//if (tries != 1) { dprintf("wait_for_dma worked after %d tries\n", tries); }
}

/*
 * Get data from the ethernet card
 */
static void
ether_min(
		  ether_private_t *data,
		  unsigned char *dst, 
		  unsigned src, 
		  unsigned len
		  )
{
	switch (data->cardtype) {
	case ECARD_NE2000:
		ether_min_ne2000(data, dst, src, len);
		break;
	case ECARD_3COM:
		ether_min_3com(data, dst, src, len);
		break;
	}
}

/*
 * Put data on the ethernet card
 */
static void
ether_mout(
		   ether_private_t *data, 
		   unsigned dst, 
		   const unsigned char *src, 
		   unsigned len)
{
	switch (data->cardtype) {
	case ECARD_NE2000:
		ether_mout_ne2000(data, dst, src, len);
		break;
	case ECARD_3COM:
		ether_mout_3com(data, dst, src, len);
		break;
	}
}


static char *
mapit(const char *area_name)
{
	area_id id;
	area_info info;

	id = find_area(area_name);
	if (id < B_NO_ERROR) {
#if __INTEL__
		if (strcmp(area_name, "isa_mem") == 0) {
			void *addr;

			id = map_physical_memory("isa_mem", (void *)0x00000000,
									 0xe0000, B_ANY_KERNEL_ADDRESS,
									 B_WRITE_AREA | B_READ_AREA,
									 &addr);
			if (id < B_NO_ERROR) {
				return (NULL);
			}
			return (addr);
		}
#endif /* __INTEL__ */
		return (NULL);
	}
	if (get_area_info(id, &info) < B_NO_ERROR) {
		return (NULL);
	}
	return ((char *)info.address);
}

/*
 * Determine if we have a 3com card
 */
static int
probe_3com(
		   ether_private_t *data
		   )
{
	int i;
	int reg;
	unsigned char test[EC_PAGE_SIZE];
	short port = data->port;
	int failed;

	if (data->mem == 0) {
		ddprintf("Must set memory loc for 3com\n");
		return (0);
	}
	data->ISA_MEM_BASE = mapit("isa_mem");
	if (data->ISA_MEM_BASE == NULL) {
		wprintf("Can't map ISA mem!\n");
		return (0);
	}
	data->ETHER_BUF_START = ETHER_BUF_START_3COM;
	data->ETHER_BUF_SIZE = ETHER_BUF_SIZE_3COM;
	calc_constants(data);
	data->cardtype = ECARD_3COM;

	/* reset bit on */
	ether_outb(data, E33G_CNTRL, ECNTRL_RESET | ECNTRL_ONBOARD);
	
	/* reset bit off (is this necessary?) */
	ether_outb(data, E33G_CNTRL, ECNTRL_ONBOARD);
	
	/* get address */
	ether_outb(data, E33G_CNTRL, ECNTRL_SAPROM | ECNTRL_ONBOARD);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		data->myaddr.ebyte[i] = ether_inb(data, i);
	}

	/* set to thin-net (this is not necessary) */
	ether_outb(data, E33G_CNTRL, ECNTRL_ONBOARD);
	
	/* turn off interrupts */
	ether_outb(data, E33G_GACFR, EGACFR_IRQOFF);
	
	/* stop chip */
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE0);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP);
	
	/* test register */
	reg = ether_inb(data, EN_CCMD);
	if (reg != (ENC_NODMA|ENC_STOP)) {
		ddprintf("command register failed: %02x != %02x\n", reg, ENC_NODMA|ENC_STOP);
		return (0);
	}

	/* test memory */
	for (i = 0; i < sizeof(test); i++) {
		test[i] = i;
	}
	ether_mout(data, data->ETHER_BUF_START, (unsigned char *)&test[0], sizeof(test));
	memset(&test, 0, sizeof(test));
	ether_min(data, (unsigned char *)&test[0], data->ETHER_BUF_START, sizeof(test));
	failed = 0;
	for (i = 0; i < sizeof(test); i++) {
		if (test[i] != i) {
			ddprintf("memory test failed: %02x %02x\n", i, test[i]);
			failed++;
		}
	}
	if (failed) {
		return (0);
	}

	wprintf("3c503 ethernet card found - ");
	print_address(&data->myaddr);
	return (1);
}

#if STAY_ON_PAGE_0
/* 
 * Zero out the headers in the ring buffer
 */
static void
ringzero(
		 ether_private_t *data,
		 unsigned boundary,
		 unsigned next_boundary
		 )
{
	ring_header ring;
	int i;
	int pages;
	unsigned offset;

	ring.count = 0;
	ring.next_packet = 0;
	ring.status = 0;

	if (data->boundary < next_boundary) {
		pages = next_boundary - data->boundary;
	} else {
		pages = (data->EC_RINGSIZE >> EC_PAGE_SHIFT) - (data->boundary - next_boundary);
	}

	for (i = 0; i < pages; i++) {
		offset = data->boundary << EC_PAGE_SHIFT;
		ether_mout(data, offset, (unsigned char *)&ring, sizeof(ring));
		data->boundary++;
		if (data->boundary >= data->EC_RXBUF_END_PAGE) {
			data->boundary = data->EC_RXBUF_START_PAGE;
		}
	}
}
#endif /* STAY_ON_PAGE_0 */

/*
 * Determine if we have an ne2000 card
 */
static int
probe_ne2000(
			 ether_private_t *data
			 )
{
	int i;
	int reg;
	unsigned char test[EC_PAGE_SIZE];
	short waddr[ETHER_ADDR_LEN];
	short port = data->port;

	data->ETHER_BUF_START = ETHER_BUF_START_NE2000;
	data->ETHER_BUF_SIZE = ETHER_BUF_SIZE_NE2000;
	calc_constants(data);
	data->cardtype = ECARD_NE2000;

	reg = ether_inb(data, NE_RESET);
	snooze(2000);
	ether_outb(data, NE_RESET, reg);
	snooze(2000);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP | ENC_PAGE0);
	i = 10000;
	while ((ether_inb(data, EN0_ISR) & ISR_RESET) == 0 && i-- > 0);
	if (i < 0) {
		dprintf("reset failed -- ignoring\n");
	}
	ether_outb(data, EN0_ISR, 0xff);
	ether_outb(data, EN0_DCFG, DCFG_BM16);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP | ENC_PAGE0);
	snooze(2000);
	reg = ether_inb(data, EN_CCMD);
	if (reg != (ENC_NODMA|ENC_STOP|ENC_PAGE0)) {
		ddprintf("command register failed: %02x != %02x\n", reg, ENC_NODMA|ENC_STOP); 
		return (0);
	}
	ether_outb(data, EN0_TXCR, 0);
	ether_outb(data, EN0_RXCR, ENRXCR_MON);
	ether_outb(data, EN0_STARTPG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN0_STOPPG, data->EC_RXBUF_END_PAGE);
	ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE);
	ether_outb(data, EN0_IMR, 0);
	ether_outb(data, EN0_ISR, 0);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE1 | ENC_STOP);
	ether_outb(data, EN1_CURPAG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE0 | ENC_STOP);

	/* stop chip */
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE0);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP);

	memset(&waddr[0], 0, sizeof(waddr));
	ether_min(data, (unsigned char *)&waddr[0], 0, sizeof(waddr));

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		data->myaddr.ebyte[i] = ((unsigned char *)&waddr[0])[2*i];
	}

	/* test memory */
	for (i = 0; i < sizeof(test); i++) {
		test[i] = i;
	}
	ether_mout(data, data->ETHER_BUF_START, (unsigned char *)&test[0], sizeof(test));
	memset(&test, 0, sizeof(test));
	ether_min(data, (unsigned char *)&test[0], data->ETHER_BUF_START, sizeof(test));
	for (i = 0; i < sizeof(test); i++) {
		if (test[i] != i) {
			ddprintf("memory test failed: %02x %02x\n", i, test[i]);
			return (0);
		}
	}

	wprintf("ne2000 ethernet card found - ");
	print_address(&data->myaddr);
	return (1);
}

/*
 * See if we have an ethernet card
 */
static int
probe(
	  ether_private_t *data
	  )
{
#if !__INTEL__
	data->ISA_IO_BASE = mapit("isa_io");
	if (data->ISA_IO_BASE == NULL) {
		wprintf("Can't map ISA IO!\n");
		return(0);
	}
#endif
	ddprintf("probing for ne2000\n");
	if (probe_ne2000(data)) {
		return (1);
	}
	ddprintf("probing for 3com\n");
	if (probe_3com(data)) {
		return (1);
	}
	return (0);
}

/* 
 * extra initialization for the 3com card
 */
static void
init_3com(
		  ether_private_t *data
		  )
{
	ether_outb(data, E33G_VP2, 0xff);
	ether_outb(data, E33G_VP1, 0xff);
	ether_outb(data, E33G_VP0, 0x0);
	ether_outb(data, E33G_STARTPG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, E33G_STOPPG, data->EC_RXBUF_END_PAGE);
	if (data->irq_level == 9) {
		ether_outb(data, E33G_IDCFR, ETHER_IRQ2);
	} else {
		ether_outb(data, E33G_IDCFR, (ETHER_IRQ2 >> 2) << data->irq_level);
	}
    ether_outb(data, E33G_NBURST, 0x08);
	ether_outb(data, E33G_DMAAH, 0x20);
	ether_outb(data, E33G_DMAAL, 0x0);
}


/*
 * Initialize the ethernet card
 */
static void
init(
	 ether_private_t *data
	 )
{
	int i;
	
	if (data->cardtype == ECARD_3COM) {
		init_3com(data);
	}
	

#if STAY_ON_PAGE_0
	/*
	 * Set all the ring headers to zero
	 */
	ringzero(data, data->EC_RXBUF_START_PAGE, data->EC_RXBUF_END_PAGE);
#endif /* STAY_ON_PAGE_0 */
	

	/* initialize data configuration register */
	ether_outb(data, EN0_DCFG, DCFG_BM16);
	
	/* clear remote byte count registers */
	ether_outb(data, EN0_RCNTLO, 0x0);
	ether_outb(data, EN0_RCNTHI, 0x0);
	
	/* initialize receive configuration register */
	ether_outb(data, EN0_RXCR, ENRXCR_BCST);
	
	/* get into loopback mode */
	ether_outb(data, EN0_TXCR, TXCR_LOOPBACK);
	
	/* set boundary, page start and page stop */
	ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE);
	data->boundary = data->EC_RXBUF_START_PAGE;
	ether_outb(data, EN0_STARTPG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN0_STOPPG, data->EC_RXBUF_END_PAGE);

	/* set transmit page start register */
	ether_outb(data, EN0_TPSR, data->EC_VMEM_PAGE);
	
	/* clear interrupt status register */
	ether_outb(data, EN0_ISR, 0xff);
	
	/* initialize interrupt mask register */
	ether_outb(data, EN0_IMR, INTS_WE_CARE_ABOUT);
	
	/* set page 1 */
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE1);
	
	/* set physical address */
	for (i = 0; i < 6; i++) {
		ether_outb(data, EN1_PHYS + i, data->myaddr.ebyte[i]);
	}
	
	/* set multicast address */
	for (i = 0; i < 8; i++) {
		ether_outb(data, EN1_MULT+i, 0xff); 
	}
	data->nmulti = 0;
	
	/* set current pointer */
	ether_outb(data, EN1_CURPAG, data->EC_RXBUF_START_PAGE);


	/* start chip */
	ether_outb(data, EN_CCMD, ENC_START | ENC_PAGE0 | ENC_NODMA);
	
	/* initialize transmit configuration register */
	ether_outb(data, EN0_TXCR, 0x00);
	if (data->cardtype == ECARD_3COM) {
		/* turn on interrupts */
		ether_outb(data, E33G_GACFR, EGACFR_NORM);
	}
}



/* 
 * Copy data from the card's ring buffer
 */
static void
ringcopy(
		 ether_private_t *data,
		 unsigned char *ether_buf,
		 int offset,
		 int len
		 )
{
	int roffset;
	int rem;

	roffset = offset - data->EC_RINGSTART;
	rem = data->EC_RINGSIZE - roffset;
	if (len > rem) {
		ether_min(data, &ether_buf[0], offset, rem);
		ether_min(data, &ether_buf[rem], data->EC_RINGSTART, len - rem);
	} else {
		ether_min(data, &ether_buf[0], offset, len);
	}
}


/*
 * Set the boundary register, both on the card and internally
 * NOTE: you cannot make the boundary = current register on the card,
 * so we actually set it one behind.
 */
static void
setboundary(ether_private_t *data, unsigned char nextboundary)
{
	if (nextboundary != data->EC_RXBUF_START_PAGE) {
		ether_outb(data, EN0_BOUNDARY, nextboundary - 1);
	} else {
		/* since it's a ring buffer */
		ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE - 1);
	}
	data->boundary = nextboundary;
}

/*
 * Start resetting the chip, because of ring overflow
 */
static int 
reset(ether_private_t *data)
{
	unsigned char cmd;
	unsigned char isr;
	int resend;

	cmd = ether_inb(data, EN_CCMD);
	ether_outb(data, EN_CCMD, ENC_STOP);
	snooze(2000);
	ether_outb(data, EN0_RCNTLO, 0x0);
	ether_outb(data, EN0_RCNTHI, 0x0);
	if ((cmd & ENC_TRANS) == 0) {
		resend = 0;
	} else {
		isr = getisr(data);
		if (isr & (ISR_TRANSMIT | ISR_TRANSMIT_ERROR)) {
			resend = 0;
		} else {
			resend = 1;
		}
	}
	/* get into loopback mode */
	ether_outb(data, EN0_TXCR, TXCR_LOOPBACK);
	ether_outb(data, EN_CCMD, ENC_START);
	return (resend);
}

/*
 * finish the reset
 */
static void
finish_reset(
			 ether_private_t *data,
			 int resend
			 )
{
	setisr(data, ISR_OVERWRITE);
	ether_outb(data, EN0_TXCR, 0x00);

	if (resend) {
		if (data->cardtype == ECARD_NE2000) {
			wprintf("uh-oh, ne2000 can't resend\n");
			/* will we get an interrupt? */
		} else {
			ether_outb(data, EN_CCMD, ENC_TRANS | ENC_START | ENC_PAGE0 | ENC_NODMA);
		}
	}
}


static bool ether_interrupts[NIRQS] = {
	FALSE,	/* 0 timer */
	FALSE,	/* 1 keyboard */
	FALSE,	/* 2 routed to 9 */
	TRUE,	/* 3 com2 */
	TRUE,	/* 4 com1 */
	TRUE,	/* 5 WE CAN USE THIS */
	FALSE,	/* 6 floppy */
	TRUE,	/* 7 parallel port */
	FALSE,	/* 8 real-time clock */
	TRUE,	/* 9 WE CAN USE THIS */
	TRUE,	/* 10 WE CAN USE THIS */
	TRUE,	/* 11 WE CAN USE THIS */
	TRUE,	/* 12 mouse */
	TRUE,	/* 13 WE CAN USE THIS */
	FALSE,	/* ide */
	TRUE	/* 15 WE CAN USE THIS */
};


/*
 * Handle ethernet interrupts
 */
static int32
ether_interrupt(void *_data)
{
	ether_private_t *data = (ether_private_t *) _data;
	unsigned char isr;
	int wakeup_reader = 0;
	int wakeup_writer = 0;
	int32 handled = B_UNHANDLED_INTERRUPT;
	
	data->ints++;
	for (INTR_LOCK(data, isr = getisr(data)); 
		 isr & INTS_WE_CARE_ABOUT; 
		 INTR_LOCK(data, isr = getisr(data))) {
		if (isr & ISR_RECEIVE) {
			data->rints++;
			wakeup_reader++;
			INTR_LOCK(data, setisr(data, ISR_RECEIVE));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_TRANSMIT_ERROR) {
			data->werrs++;
			INTR_LOCK(data, setisr(data, ISR_TRANSMIT_ERROR));
			wakeup_writer++;
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_TRANSMIT) {
			data->wints++;
			INTR_LOCK(data, setisr(data, ISR_TRANSMIT));
			wakeup_writer++;
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_RECEIVE_ERROR) {
			INTR_LOCK(data, data->frame_errs += ether_inb(data, EN0_CNTR0));
			INTR_LOCK(data, data->crc_errs += ether_inb(data, EN0_CNTR1));
			INTR_LOCK(data, data->frames_lost += ether_inb(data, EN0_CNTR2));
			isr &= ~ISR_RECEIVE_ERROR;
			INTR_LOCK(data, setisr(data, ISR_RECEIVE_ERROR));
			handled = B_HANDLED_INTERRUPT;
		}
		if (isr & ISR_DMADONE) {
			isr &= ~ISR_DMADONE;	/* handled elsewhere */
			handled = B_HANDLED_INTERRUPT;
		}
		if (isr & ISR_OVERWRITE) {
			isr &= ~ISR_OVERWRITE; /* handled elsewhere */
			handled = B_HANDLED_INTERRUPT;
		}
		if (isr) {
			/*
			 * If any other interrupts, just clear them (hmmm....)
			 * ??? this doesn't seem right - HB
			 */
			INTR_LOCK(data, setisr(data, isr));
			data->interrs++;
		}
	}
	if (wakeup_reader) {
		input_unwait(data, 1);
	}
	if (wakeup_writer) {
		output_unwait(data, 1);
	}

	return handled;
}

/*
 * Check to see if there are any new errors
 */
static void
check_errors(ether_private_t *data)
{
#define DOIT(stat, message) \
	if (stat > stat##_last) { \
		stat##_last = stat; \
		wprintf(message, stat##_last); \
	}

	DOIT(data->rerrs, "Receive errors now %d\n");
	DOIT(data->werrs, "Transmit errors now %d\n");
	DOIT(data->interrs, "Interrupt errors now %d\n");
	DOIT(data->frames_lost, "Frames lost now %d\n");
#undef DOIT
#if 0
	/* 
	 * these are normal errors because collisions are normal
	 * so don't make a big deal about them.
	 */
	DOIT(data->frame_errs, "Frame alignment errors now %d\n");
	DOIT(data->crc_errs, "CRC errors now %d\n");
#endif
}

/*
 * Find out if there are any more packets on the card
 */
static int
more_packets(
			 ether_private_t *data,
			 int didreset
			 )
{
#if STAY_ON_PAGE_0

	unsigned offset;
	ring_header ring;
	
	offset = data->boundary << EC_PAGE_SHIFT;
	ether_min(data, (unsigned char *)&ring, offset, sizeof(ring));
	return (ring.status && ring.next_packet && ring.count);

#else /* STAY_ON_PAGE_0 */

	cpu_status ps;
	unsigned char cur;

	/*
	 * First, get the current registe
	 */
	ps = disable_interrupts();
	intr_lock(data);
	ether_outb(data, EN_CCMD, ENC_PAGE1);
	cur = ether_inb(data, EN1_CURPAG);
	ether_outb(data, EN_CCMD, ENC_PAGE0);
	intr_unlock(data);
	restore_interrupts(ps);

	/*
	 * Then return the result
	 * Must use didreset since cur == boundary in
	 * an overflow situation.
	 */
	return (didreset || cur != data->boundary);

#endif	/* STAY_ON_PAGE_0 */
}

/*
 * Copy a packet from the ethernet card
 */
static int
copy_packet(
			ether_private_t *data,
			unsigned char *ether_buf,
			int buflen
			)
{
	ring_header ring;
	unsigned offset;
	int len;
	int rlen;
	int ether_len = 0;
	int didreset = 0;
	int resend = 0;

	io_lock(data);
	check_errors(data);

	/*
	 * Check for overwrite error first
	 */
	if (getisr(data) & ISR_OVERWRITE) {
		wprintf("starting ether reset!\n");
		data->resets++;
		resend = reset(data);
		didreset++;
	}
	
	if (more_packets(data, didreset)) do {
		/*
		 * Read packet ring header
		 */
		offset = data->boundary << EC_PAGE_SHIFT;
		ether_min(data, (unsigned char *)&ring, offset, sizeof(ring));

		len = swapshort(ring.count);

		if (!(ring.status & RSR_INTACT)) {
			wprintf("packet not intact! (%02x,%u,%02x) (%d)\n",
					ring.status,
					ring.next_packet,
					ring.count,
					data->boundary);
			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}
		if (ring.next_packet < data->EC_RXBUF_START_PAGE || 
			ring.next_packet >= data->EC_RXBUF_END_PAGE) {
			wprintf("ether_read: bad next packet! (%02x,%u,%02x) (%d)\n", 
					ring.status,
					ring.next_packet,
					ring.count,
					data->boundary);
			data->rerrs++;
			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}

		len = swapshort(ring.count);
		rlen = len - 4;
		if (rlen < ETHER_MIN_SIZE || rlen > ETHER_MAX_SIZE) {
			wprintf("ether_read: bad length! (%02x,%u,%02x) (%d)\n", 
					ring.status,
					ring.next_packet,
					ring.count,
					data->boundary);
			data->rerrs++;
			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}
			
	
		if (rlen > buflen) {
			rlen = buflen;
		}
		ringcopy(data, ether_buf, offset + 4, rlen);

#if STAY_ON_PAGE_0
		ringzero(data, data->boundary, ring.next_packet);
#endif /* STAY_ON_PAGE_0 */

		ether_len = rlen;
		setboundary(data, ring.next_packet);
		data->reads++;

		
	} while (0);

 done:
	if (didreset) {
		wprintf("finishing ether reset!\n");
		finish_reset(data, resend);
	}

	if (input_count(data) <= 0 && more_packets(data, didreset)) {
		/*
		 * Looks like there is another packet
		 * So, make sure they get woken up
		 */
		input_unwait(data, 1);
	}

	io_unlock(data);
	return (ether_len);
}

static int
my_packet(
		  ether_private_t *data,
		  char *addr
		  )
{
	int i;
	const char broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (memcmp(addr, &data->myaddr, sizeof(data->myaddr)) == 0) {
		return (1);
	}
	if (memcmp(addr, broadcast, sizeof(broadcast)) == 0) {
		return (1);
	}
	for (i = 0; i < data->nmulti; i++) {
		if (memcmp(addr, &data->multi[i], sizeof(data->multi[i])) == 0) {
			return (1);
		}
	}
	return (0);
}

/*
 * Implements the read() system call to the ethernet driver
 */
static status_t 
ether_read(
		   void *_data,
		   off_t pos,
		   void *buf,
		   size_t *len
		   )
{
	ether_private_t *data = (ether_private_t *) _data;
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
	} while (!data->nonblocking && packet_len == 0 && !my_packet(data, buf));
	atomic_add(&data->inrw, -1);
	*len = packet_len;
	return 0;
}


/*
 * Check the status of the last packet transmitted
 */
static void
check_transmit_status(ether_private_t *data)
{
	unsigned char status;

	status = ether_inb(data, EN0_TPSR);
	if (status & (TSR_ABORTED | TSR_UNDERRUN)) {
		wprintf("transmit error: %02x\n", status);
	}
#if 0
	if (data->wints + data->werrs != data->writes) {
		wprintf("Write interrupts %d, errors %d, transmits %d\n",
				data->wints, data->werrs, data->writes);
	}
#endif
}


/*
 * implements the write() system call to the ethernet
 */
static status_t
ether_write(
			void *_data,
			off_t pos,
			const void *buf,
			size_t *len
			)
{
	ether_private_t *data = (ether_private_t *) _data;
	ulong buflen;
	int status;

	//dprintf("ether_write : %d bytes\n",*len);

	if (!data->init) {
		return (B_ERROR);
	}
	
	buflen = *len;
	atomic_add(&data->inrw, 1);
	if (data->interrupted) {
		atomic_add(&data->inrw, -1);
		return (B_INTERRUPTED);
	}
	/*
	 * Wait for somebody else (if any) to finish transmitting 
	 */
	status = output_wait(data, ETHER_TRANSMIT_TIMEOUT);
	if (status < B_NO_ERROR || data->interrupted) {
		atomic_add(&data->inrw, -1);
		return (status);
	}

	io_lock(data);
	check_errors(data);

	if (data->writes > 0) {
		check_transmit_status(data);
	}
	ether_mout(data, data->ETHER_BUF_START, (const unsigned char *)buf, buflen);
	if (buflen < ETHER_MIN_SIZE) {
		/*
		 * Round up to ETHER_MIN_SIZE
		 */
		buflen = ETHER_MIN_SIZE;
	}
	ether_outb(data, EN0_TCNTLO, (char)(buflen & 0xff));
	ether_outb(data, EN0_TCNTHI, (char)(buflen >> 8));
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_TRANS);
	data->writes++;
	io_unlock(data);
	atomic_add(&data->inrw, -1);
	*len = buflen;	
	return 0;
}

long
inrw(ether_private_t *data)
{
	return (data->inrw);
}

static status_t
ether_open(
		   const char *name,
		   uint32 flags,
		   void **_cookie
           )
{
	ether_private_t **cookie = (ether_private_t **) _cookie;
	ether_private_t *data;

	data = (ether_private_t *) malloc(sizeof(ether_private_t));
	if (!data)
		return B_NO_MEMORY;

	memset(data, 0, sizeof(ether_private_t));
	data->init = FALSE;
	*cookie = data;
	return B_NO_ERROR;
}


static status_t
ether_close(
		   void *_data
		   )
{
	ether_private_t *data = (ether_private_t *) _data;
	if (!data->init) {
		/*
		 * We didn't find a card: No need to do anything
		 */
		return (B_NO_ERROR);
	}

	/* 
	 * Force pending reads and writes to terminate
	 */
	io_lock(data);
	data->interrupted = 1;
	input_unwait(data, 1);
	output_unwait(data, 1);
	io_unlock(data);
	while (inrw(data)) {
		snooze(1000000);
		wprintf("ether: still waiting for read/write to finish\n");
	}

	/*
	 * Stop the chip
	 */
	ether_outb(data, EN_CCMD, ENC_STOP);
	snooze(2000);
	
	/*
	 * And clean up
	 */
	remove_io_interrupt_handler(data->irq_level, ether_interrupt, data);
	delete_sem(data->iolock);
	delete_sem(data->ilock);
	delete_sem(data->olock);
	
	
	/*
	 * Reset all the statistics
	 */
	data->ints = 0;	
	data->rints = 0;	
	data->rerrs = 0;	
	data->wints = 0;	
	data->werrs = 0;	
	data->reads = 0;	
	data->writes = 0;	
	data->interrs = 0;
	data->resets = 0;
	data->frame_errs = 0;
	data->crc_errs = 0;
	data->frames_lost = 0;
	
	data->rerrs_last = 0;	
	data->werrs_last = 0;	
	data->interrs_last = 0;
	data->frame_errs_last = 0;
	data->crc_errs_last = 0;
	data->frames_lost_last = 0;
	
	/*
	 * Reset to notify others that this port and irq is available again
	 */
	ether_data[data->irq_level] = NULL;
	return (B_NO_ERROR);
}

static status_t
ether_free(
		   void *_data
		  )
{
	ether_private_t *data = (ether_private_t *) _data;
	free(data);
	return 0;
}


static int
ether_init(		 
		   ether_private_t *data,
		   int port,
		   int irq_level,
		   unsigned long mem
		   )
{
	int i;

	/*
	 * Can't init twice
	 */
	if (data->init) {
		ddprintf("already inited!\n");
		return (B_ERROR);
	}
	/*
	 * Sanity checking: did you forget to specify a port?
	 */
	if (port == 0) {
		ddprintf("port == 0\n");
		return (B_ERROR);
	}

	/*
	 * Sanity checking: valid irq?
	 */
	if (irq_level < 0 || irq_level >= NIRQS) {
		ddprintf("invalid irq: %d out of range\n", irq_level);
		return (B_ERROR);
	}
	if (mem < ISAMEM_MIN || mem > ISAMEM_MAX) {
		ddprintf("invalid mem: %08x out of range\n", mem);
		return (B_ERROR);
	}

	/*
	 * IRQ 9 is same as IRQ2: fix it here
	 */
	if (irq_level == 2) {
		irq_level = 9;
	}

	if (!ether_interrupts[irq_level]) {
		ddprintf("invalid irq: %d not available\n", irq_level);
		return (B_ERROR);
	}
	
	/*
	 * Sanity check: is irq taken?
	 */
	if (ether_data[irq_level]) {
		ddprintf("invalid irq: %d in use\n", irq_level);
		return (B_ERROR);
	}
	ether_data[irq_level] = data;

	/*
	 * Sanity check: is port taken?
	 */
	for (i = 0; i < NIRQS; i++) {
		if (!ether_data[i])
			continue;
		if (ether_data[i]->port == port) {
			ddprintf("invalid port: %03x in use\n", port);
			return (B_ERROR);
		}
		if (mem != 0 && ether_data[i]->mem == mem) {
			ddprintf("invalid memory: %08x in use\n", mem);
		}
	}

	data->port = port;
	data->mem = mem;
	wprintf("ether: probing at port %03x, mem %08x (irq %d).\n", port, mem, irq_level);
	if (!probe(data)) {
		data->port = 0;
		ddprintf("probe failed\n");
		return (B_ERROR);
	}
	data->iolock = create_sem(1, "ethercard io");
	set_sem_owner(data->iolock, B_SYSTEM_TEAM);
	data->olock = create_sem(1, "ethercard output");
	set_sem_owner(data->olock, B_SYSTEM_TEAM);
	data->ilock = create_sem(0, "ethercard input");
	set_sem_owner(data->ilock, B_SYSTEM_TEAM);
	data->interrupted = 0;
	data->inrw = 0;
	data->irq_level = irq_level;
	data->nonblocking = 0;
	data->init = TRUE;
	init(data);
	ports_for_irqs[irq_level] = port;
	install_io_interrupt_handler(irq_level, ether_interrupt, data, 0);
	return (B_NO_ERROR);
}


static int
domulti(
		ether_private_t *data,
		char *addr
		)
{
	int i;
	int nmulti = data->nmulti;

	if (nmulti == MAX_MULTI) {
		return (B_ERROR);
	}
	for (i = 0; i < nmulti; i++) {
		if (memcmp(&data->multi[i], addr, sizeof(data->multi[i])) == 0) {
			break;
		}
	}
	if (i == nmulti) {
		/*
		 * Only copy if it isn't there already
		 */
		memcpy(&data->multi[i], addr, sizeof(data->multi[i]));
		data->nmulti++;
	}
	if (data->nmulti == 1) {
		wprintf("Enabling multicast\n");
		ether_outb(data, EN0_RXCR, ENRXCR_BCST | ENRXCR_MCST);
	}
	return (B_NO_ERROR);
}

/*
 * Standard driver control function
 */
static status_t
ether_control(
			void *_data,
			uint32 msg,
			void *buf,
			size_t len
			)
{
	ether_private_t *data = (ether_private_t *) _data;
	struct {
		short port;
		short irq_level;
		unsigned long mem;
	} params;

	switch (msg) {
	case ETHER_INIT:
		memcpy(&params, buf, sizeof(params));
		return (ether_init(data, params.port, params.irq_level, params.mem));
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


/*
 * Local variables:
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */

/* -----
	init_hardware - check if ISA bus exists
----- */
status_t
init_hardware (void)
{
#if __INTEL__
	return B_NO_ERROR;
#else
	return mapit("isa_io") ? B_NO_ERROR : B_ERROR;
#endif
}

status_t 
init_driver()
{
#if __INTEL__
	return get_module(isa_name, (module_info **)&isa);
#else
	return B_NO_ERROR;
#endif
}

void
uninit_driver()
{
#if __INTEL__
	put_module(isa_name);
#endif
}

/* -----
	driver-related structures
----- */

static char *ether_name[] = { "net/ether/0", NULL };

static device_hooks ether_device =  {
	ether_open, 		/* -> open entry point */
	ether_close,		/* -> close entry point */
	ether_free,			/* -> free entry point */
	ether_control, 		/* -> control entry point */
	ether_read,			/* -> read entry point */
	ether_write,		/* -> write entry point */
	NULL,				/* -> select entry point */
	NULL				/* -> deselect entry point */
};


const char **
publish_devices()
{
	return ether_name;
}

device_hooks *
find_device(const char *name)
{
	return &ether_device;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
