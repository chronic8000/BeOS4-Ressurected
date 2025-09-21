/*
 * gce.cpp
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 *
 * Grand Central Ethernet: PowerMacintosh on-board Ethernet 
 * Uses AMD CURIO MACE & Grand Central chips
 */
extern "C" {
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <ether_driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <gcentral.h>
#include <dbdma.h>
#include <mapio.h>
}

/*
 * Base address for driver allocated areas
 */
#define DRIVER_BASE 0x10000000

#define MAXBOGUS 10000				/* bogus count to void infinite loops */
#define MAXBIGBOGUS 10000000		/* bigger one */
#define MAXMULTI 16

#define ETHER_MIN_SIZE 60			/* minimize legal enet packet size */

#define DMA_OUTPUT_LAST (DBDMA_OUTPUT_LAST >> 28)	/* output packet */
#define DMA_INPUT_MORE	(DBDMA_INPUT_MORE >> 28)	/* input packet */
#define DMA_STOP		(DBDMA_STOP >> 28)			/* stop DMA */
#define DMA_LOAD_QUAD	(DBDMA_LOAD_QUAD >> 28)		/* load from sys mem */
#define DMA_NOP			(DBDMA_NOP >> 28)			/* nop (branch) */

#define DMA_KEY_STREAM0  (DBDMA_KEY_STREAM0 >> 24)	/* FIFO stream */
#define DMA_KEY_SYSTEM  (DBDMA_KEY_SYSTEM >> 24)	/* memory stream */

#define DEBUG 0
#if DEBUG
#define wprintf tprintf
#define ddprintf tprintf
#else /* DEBUG */
#define wprintf tprintf
#define ddprintf (void)
#endif /* DEBUG */

/*
 * printf, if debugging, include a timestamp
 */		
static void
tprintf(const char *format, ...)
{
	va_list args;
	char buf[100];
	char *p;

	va_start(args, format);
	p = buf;
#if TIMING_PRINT
	sprintf(p, "%10.4f: ", system_time());
	p += strlen(p);
#endif /* TIMING_PRINT */
	vsprintf(p, format, args);
	va_end(args);
	dprintf("%s", buf);
}

#define dprintf !!!dont_do_this!!!

/*
 * MACE chip revision that we support (Am79C940)
 */
#define MACE_CHIP_ID 	0x940

/*
 * Offsets of MACE registers in Grand Central
 */
#define MACE_RCVFIFO	0x000	/* Receive FIFO */
#define MACE_XMTFIFO	0x010	/* Transmit FIFO */
#define MACE_XMTFC		0x020	/* Transmit Frame Control */
#define MACE_XMTFS		0x030	/* Transmit Frame Status */
#define MACE_XMTRC		0x040	/* Transmit Retry Count */
#define MACE_RCVFC		0x050	/* Receive Frame Control */
#define MACE_RCVFS		0x060	/* Receive Frame Status */
#define MACE_FIFOFC		0x070	/* FIFO Frame Count */
#define MACE_IR			0x080	/* Interrupt Register */
#define MACE_IMR		0x090	/* Interrupt Mask Register */
#define MACE_PR			0x0a0	/* Poll Register */
#define MACE_BIUCC		0x0b0	/* BIU Configuration Control */
#define MACE_FIFOCC		0x0c0	/* FIFO Configuration Control */
#define MACE_MACCC		0x0d0	/* MAC Configuration Control */
#define MACE_PLSCC		0x0e0	/* PLS Configuration Control */
#define MACE_PHYCC		0x0f0	/* PHY Configuration Control */
#define MACE_CHIPID1	0x100	/* Chip Identification Register: low bits */
#define MACE_CHIPID2	0x110	/* Chip Identification Register: high bits */
#define MACE_IAC		0x120	/* Internal Address Configuration */
#define MACE_RESERVED1	0x130	/* Reserved */
#define MACE_LADRF		0x140	/* Logical Address Filter */
#define	MACE_PADR		0x150	/* Physical Address */
#define MACE_RESERVED2	0x160	/* Reserved */
#define MACE_RESERVED3	0x170	/* Reserved */
#define MACE_MPC		0x180	/* Missed Packet Count */
#define MACE_RESERVED4	0x190	/* Reserved */
#define MACE_RNTPC		0x1a0	/* Runt Packet Count */
#define MACE_RCVCC		0x1b0	/* Receive Collision Count */
#define MACE_RESERVED5	0x1c0	/* Reserved */
#define MACE_UTR		0x1d0	/* User Test Register */
#define MACE_RTR1		0x1e0	/* Reserved Test Register 1 */
#define MACE_RTR2		0x1f0	/* Reserved Test Register 2 */

#define BIUCC_SWRST		0x01	/* sw reset */

#define FIFOCC_XMTFWU	0x08	/* Transmit FIFO watermark */
#define FIFOCC_RCVFWU	0x04	/* Receive FIFO watermark */
#define FIFOCC_XMTFW_8	0x00	/* xmt: 16 byte watermark */
#define FIFOCC_XMTFW_16	0x40	/* xmt: 16 byte watermark */
#define FIFOCC_RCVFW_64	0x20	/* rcv: 64 byte watermark */

#define IAC_ADDRCHG		0x80	/* enable write mode */
#define IAC_PHYADDR		0x04	/* Reset physical address pointer */
#define IAC_LOGADDR		0x02	/* Reset logical address pointer */

#define PLSCC_ENPLSIO	0x01	/* Enable PLS I/O */
#define PLSCC_AUI		0x00	/* AUI selected */
#define PLSCC_10BASET	0x02	/* 10BaseT selected */
#define PLSCC_DAI		0x04	/* DAI selected */
#define PLSCC_GPSI		0x06	/* GPSI selected */

#define PHYCC_ASEL		0x04	/* auto-select media */

#define MACCC_ENXMT		0x02	/* enable transmit */
#define MACCC_ENRCV		0x01	/* enable receive */

#define IMR_RCVINTM		0x02	/* enable RCV interrupt */
#define IMR_XMTINTM		0x01	/* enable XMT interrupt */

#define IR_RCVINT		0x02	/* RCV interrupt */
#define IR_XMTINT		0x01	/* XMT interrupt */


#define UTR_RTRD		0x40	/* Disable UTR */
#define UTR_LOOPBACK	0x04	/* loopback mode */

#define XMTFC_APADXMT	0x01	/* pad packets to 64 bytes */

#define BIUCC_XMTSP64	0x20	/* 64-byte burst mode ??? */

#define RCVFS_OFLO	(1 << 15)	/* overflow */
#define RCVFS_CLSN	(1 << 14)	/* collision */
#define RCVFS_FRAM	(1 << 13)	/* framing error */
#define RCVFS_FCS	(1 << 12)	/* checksum failure */
#define RCVFS_RUNT	0x00ff0000
#define RCVFS_COLL	0xff000000

#define RCVFC_ASTRP 0x01


#define GC_MACE_ERROR	  (1 << 7)	/* transmit error */
#define GC_MACE_INTERRUPT (1 << 5)	/* transmit interrupt */
#define GC_MACE_EOP		  (1 << 6)	/* End of received packet */


#define RX_INT_TIMEOUT		100000		/* how long to wait for RX interrupt */
#define PACKET_BUF_SIZE		1536		/* our max packet size: even number */

#define NTXPACKETS 1 /* number of TX packet slots */
#define QENTS_PER_TX 6	/* DMA command queue entries for TX */
#define NTXQENTS   ((NTXPACKETS * QENTS_PER_TX) + 1)	/* + 1 for branch */
#define NRXPACKETS 16	/* number of RX packet slots */
#define NRXCHUNKS	NRXPACKETS	/* number of RX DMA command queue entries */
#define NRXQENTS   (NRXCHUNKS + 1)	/* + 1 for branch */

/*
 * Packet buffer
 */
typedef union gce_vbuf {
	char data[PACKET_BUF_SIZE];
} gce_vbuf_t;

/*
 * The Grand Central DMA command structure
 */
typedef struct dma_command {
	uchar cmd : 4;		/* channel command */
	uchar _r1 : 1;		/* reserved */
	uchar key : 3;		/* memory channel */
	uchar _r2 : 2;		/* reserved */
	uchar i   : 2;		/* interrupt policy */
	uchar b   : 2;		/* branch policy */
	uchar w   : 2;		/* wait policy */
	ushort req_count;	/* request count */
	char *address;		/* address of data */
	char *cmd_dep;		/* branch address */
	ushort status;		/* dma status */
	ushort res_count;	/* result count */
} dma_command_t;
	

/*
 * This is guaranteed to be on a page boundary
 */
typedef struct gce_private {
	dma_command_t tx_queue[NTXQENTS];	/* DMA TX queue */
	dma_command_t rx_queue[NRXQENTS];	/* DMA RX queue */
	gce_vbuf_t rx_vbuf[NRXCHUNKS];		/* virtual RX bufffers */
	gce_vbuf_t tx_vbuf[NTXPACKETS];		/* virtual TX buffers */
	area_id area;						/* cookie for our memory (area id) */
	char *physical;						/* physical address for our memory */
	sem_id rx_sem;						/* RX packet available */
	sem_id tx_sem;						/* TX ready to send */
	sem_id wrsem;
	ushort closing;						/* are we closing? */
	ether_address_t myaddress;			/* Our ethernet address */
	unsigned nmulti;					/* number of multicast addresses */
	ether_address_t multi[MAXMULTI];	/* Our multicast address */
	uchar tx_pos;						/* host TX position */
	uchar rx_pos;						/* host RX position */
	uchar rx_stop;						/* RX boundary */
	bool init;							/* driver is initialized? */
	bool nonblocking;					/* non-blocking mode? */
} gce_private_t;

static char	*pa[3];						/* phys addrs controller, dma regs */
static char	*va[3];						/* virt addrs controller, dma regs */
static int	intrs[3];					/* interrupt ids */

/* indices to controller, dma info */
#define CONTROLLER	0
#define TX			1
#define RX			2

/*
 * Round up to the given alignment
 */
#define RNDUP(x, y) ((((ulong)(x) + (y) - 1) & ~((y) - 1)))

/*
 * Swap a short
 */
static inline ushort swapshort(ushort x)
{
	return ((x << 8) | (x >> 8));
}

/*
 * Swap a long
 */
static inline ulong swaplong(ulong x)
{
	return ((swapshort(x) << 16) | (swapshort(x >> 16)));
}

/*
 * Modulo addition, useful for ring descriptors
 */
static inline unsigned mod_add(unsigned x, unsigned y, unsigned modulus) {
	return ((x + y) % modulus);
}

/*
 * Simple minded physical address mapper: can't deal with page boundaries
 */
static char *
physaddr(void* vaddr)
{
	physical_entry pe;

	get_memory_map(vaddr, 1, &pe, 1);
	return ((char *)pe.address);
}

/*
 * Allocate memory on a page boundary
 * The area cookie is used later to free it
 */
static char *
aligned_malloc(
			   size_t size,
			   area_id *areap
			   )
{
	area_id id;
	char *start;

	start = (char *)DRIVER_BASE;
	id = create_area("gce", &start, B_BASE_ADDRESS, 
					 RNDUP(size, B_PAGE_SIZE), 
					 B_CONTIGUOUS,
					 B_READ_AREA|B_WRITE_AREA);
	if (lock_memory(start, RNDUP(size, B_PAGE_SIZE), B_DMA_IO | B_READ_DEVICE) < B_OK) {
		wprintf("Cannot lock memory\n");
		delete_area(id);
		return (NULL);
	}
	if (id < B_OK) {
		return (NULL);
	}
	*areap = id;
	ddprintf("Created area %d, size %d, address %08x\n", 
			 id, RNDUP(size, B_PAGE_SIZE), start);
	return (start);
}

/*
 * Free the memory associated with the area cookie 
 */
static void
aligned_free(area_id area)
{
	ddprintf("Deleting area %d\n", area);
	delete_area(area);
}


/* 
 * Write a dbdma transmit register (32 bits)
 */
static void
gc_write_tx_dma(unsigned offset, ulong out)
{
	__eieio();
	*(vulong *) (va[TX] + offset) = swaplong(out);
	__eieio();
}

/* 
 * Read a dbdma transmit register (32 bits)
 */
static ulong
gc_read_tx_dma(unsigned offset)
{
	ulong reg;
	
	__eieio();
	reg = (swaplong(*((ulong *) (va[TX] + offset))));
	__eieio();
	return (reg);
}

/* 
 * Write a dbdma receive register (32 bits)
 */
static void
gc_write_rx_dma(unsigned offset, ulong out)
{
	__eieio();
	*(vulong *) (va[RX] + offset) = swaplong(out);
	__eieio();
}

/* 
 * Read a dbdma receive register (32 bits)
 */
static ulong
gc_read_rx_dma(unsigned offset)
{
	ulong reg;
	
	__eieio();
	reg = (swaplong(*((ulong *) (va[RX] + offset))));
	__eieio();
	return (reg);
}

/*
 * Read a MACE register
 */
static uchar
mace_gc_read(unsigned offset)
{
	uchar	val;

	val = *(va[CONTROLLER] + offset);
	__eieio();
	return val;
}

/*
 * Get the physical address of a MACE register
 */
static char *
mace_gc_address(unsigned offset)
{
	return va[CONTROLLER] + offset;
}

/* 
 * Write a MACE register
 */
static void
mace_gc_write(unsigned offset, uchar out)
{
	*(va[CONTROLLER] + offset) = out;
	__eieio();
}


/*
 * Read the ethernet address out of the device tree
 */
static int
gce_getaddress(
			   ether_address_t *address
			   )
{
	extern char mace_ethernet_address[6];
	int i;
	int zero = 0;

	for (i = 0; i < 6; i++) {
		if (mace_ethernet_address[i] == 0) {
			zero++;
		}
	}
	if (zero == 6) {
		return (0);
	}
	memcpy(address->ebyte, mace_ethernet_address, 6);
	return (1);
}

/*
 * Swap a DMA command queue entry
 */
static void
swap_qent(
		  dma_command_t *dst,
		  dma_command_t *src
		  )
{
	ulong *srclong = (ulong *)src;
	ulong *dstlong = (ulong *)dst;

	dstlong[0] = swaplong(srclong[0]);
	dstlong[1] = swaplong(srclong[1]);
	dstlong[2] = swaplong(srclong[2]);
	dstlong[3] = swaplong(srclong[3]);
}

#if DEBUG
/* 
 * Print a DMA command queue entry
 */
static void
print_qent(
		   const char *name,
		   dma_command_t *src
		   )
{
	ddprintf("qent %s cmd=%1x,key=%1x,i=%1x,b=%1x,w=%1x,req_count=%04x\n",
			 name,
			 src->cmd, src->key, src->i, src->b, src->w, src->req_count);
	ddprintf("    address=%08x,cmd_dep=%08x,status=%04x,res_count=%04x\n",
			 src->address, src->cmd_dep, src->status, src->res_count);
}
#else
#define print_qent(name, src)
#endif /* DEBUG */

/*
 * Put an entry into the DMA command queue
 */
static void
put_qent(
		 dma_command_t *src,
		 dma_command_t *dst
		 )
{
	__eieio();
	swap_qent(dst, src);
	__eieio();
}


/*
 * Get an entry from the DMA command queue
 */
static void
get_qent(
		 dma_command_t *src,
		 dma_command_t *dst
		 )
{
	__eieio();
	swap_qent(dst, src);
	__eieio();
}

/*
 * Determine the physical address given the virtual address
 */
static inline char *
physical_address(
				 gce_private_t *data,
				 void *offset
				 )
{
	return (data->physical + ((char *)offset - (char *)data));
}

/*
 * Determine the card's RX position
 */
static int 
rx_pos(
	  gce_private_t *data
	  )
{
	return ((gc_read_rx_dma(DBDMA_COMMAND_PTR) -
			 (ulong)physical_address(data, data->rx_queue)) 
			/ sizeof(data->rx_queue[0]));
}


/*
 * Restart read (if the DMA engine stops)
 */
static void
read_restart(
			 gce_private_t *data
			 )
{
	gc_write_rx_dma(DBDMA_CONTROL,
			   (DBDMA_CS_WAKE << 16) | DBDMA_CS_WAKE);
}

static int 
tx_pos(
	  gce_private_t *data
	  )
{
	return ((gc_read_tx_dma(DBDMA_COMMAND_PTR) -
			 (ulong)physical_address(data, data->tx_queue)) 
			/ sizeof(data->tx_queue[0]));
}


/*
 * Restart DMA engine (tx)
 */
static void
tx_dma_restart(
			   gce_private_t *data
			   )
{
	int i;
	dma_command_t qent;

	/*
	 * Stop DMA
	 */
	gc_write_tx_dma(DBDMA_CONTROL, DBDMA_CS_RUN << 16);

	/*
	 * setup dma: tx
	 */
	qent.cmd = DMA_STOP;
	qent.status = ~DBDMA_CS_ACTIVE;
	for (i = 0; i < NTXQENTS - 1; i++) {
		put_qent(&qent, &data->tx_queue[i]);
	}
	qent.cmd = DMA_NOP;
	qent.i = DBDMA_INTERRUPT_NEVER;
	qent.b = DBDMA_BRANCH_ALWAYS;
	qent.w = DBDMA_WAIT_NEVER;
	qent.cmd_dep = physical_address(data, data->tx_queue);
	qent.status = ~DBDMA_CS_ACTIVE;
	put_qent(&qent, &data->tx_queue[NTXQENTS - 1]);
	
	/*
	 * Set initial position
	 */
	data->tx_pos = 0;

	/*
	 * Clear error and interrupt bits
	 */
	gc_write_tx_dma(DBDMA_CONTROL, GC_MACE_ERROR << 16);
	gc_write_tx_dma(DBDMA_CONTROL, GC_MACE_INTERRUPT << 16);

	/*
	 * Set queue pointer
	 */
	gc_write_tx_dma(DBDMA_COMMAND_PTR,
			   (ulong)physical_address(data, data->tx_queue));


	gc_write_tx_dma(DBDMA_WAIT_SEL,
			   ((GC_MACE_INTERRUPT << 16) | GC_MACE_INTERRUPT));

	/*
	 * Start DMA engine
	 */
	gc_write_tx_dma(DBDMA_CONTROL,
			   (DBDMA_CS_RUN << 16) | DBDMA_CS_RUN);
}


/*
 * Restart DMA engine (rx)
 */
static void
rx_dma_restart(
			   gce_private_t *data
			   )
{
	int i;
	dma_command_t qent;

	gc_write_rx_dma(DBDMA_CONTROL, DBDMA_CS_RUN << 16);

	data->rx_pos = 0;
	data->rx_stop = NRXCHUNKS - 1;

	/*
	 * setup dma: rx
	 */
	for (i = 0; i < NRXQENTS - 2; i++) {
		qent.cmd = DMA_INPUT_MORE;
		qent.key = DMA_KEY_STREAM0;
		qent.i = DBDMA_INTERRUPT_TRUE;
		qent.b = DBDMA_BRANCH_NEVER;
		qent.w = DBDMA_WAIT_NEVER;
		qent.req_count = PACKET_BUF_SIZE;
		qent.address = physical_address(data, &data->rx_vbuf[i]);
		qent.status = ~DBDMA_CS_ACTIVE;
		put_qent(&qent, &data->rx_queue[i]);
	}
	qent.cmd = DMA_STOP;
	qent.status = ~DBDMA_CS_ACTIVE;
	put_qent(&qent, &data->rx_queue[NRXQENTS - 2]);

	qent.cmd = DMA_NOP;
	qent.b = DBDMA_BRANCH_ALWAYS;
	qent.cmd_dep = physical_address(data, data->rx_queue);
	qent.status = ~DBDMA_CS_ACTIVE;
	put_qent(&qent, &data->rx_queue[NRXQENTS - 1]);


	gc_write_rx_dma(DBDMA_COMMAND_PTR,
			   (ulong)physical_address(data, data->rx_queue));

	gc_write_rx_dma(DBDMA_INT_SEL,
			   ((GC_MACE_EOP << 16) | GC_MACE_EOP));

	gc_write_rx_dma(DBDMA_CONTROL,
			   (DBDMA_CS_RUN << 16) | DBDMA_CS_RUN);
}

/*
 * Reset the receive side of things
 */
static void
recv_reset(
		   gce_private_t *data
		   )
{
	wprintf("gce reset\n");
	mace_gc_write(MACE_MACCC, MACCC_ENXMT);
	
	mace_gc_write(MACE_FIFOCC, 
				  FIFOCC_XMTFW_8 | FIFOCC_RCVFWU | FIFOCC_RCVFW_64);

	rx_dma_restart(data);

	mace_gc_write(MACE_MACCC, MACCC_ENXMT | MACCC_ENRCV);
}

static int
my_packet(
		  gce_private_t *data,
		  char *addr
		  )
{
	int i;
	const char broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (memcmp(addr, &data->myaddress, sizeof(data->myaddress)) == 0) {
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

static long
sem_count(sem_id sem)
{
	long count;

	get_sem_count(sem, &count);
	return (count);
}

/*
 * Entry point: read a packet
 */
static status_t 
gce_read(
		 void *cookie,
		 off_t _pos,
		 void *vbuf,
		 size_t *lenp
		 )
{
	char *buf = (char *)vbuf;
	int buflen = *lenp;
	gce_private_t *data = (gce_private_t *)cookie;
	dma_command_t qent;
	dma_command_t nqent;
	int pos;
	int stop;
	status_t err;
	long len;
	ulong rs;
	ulong status;
	bigtime_t timeout;

	if (data->closing) {
		return (B_INTERRUPTED);
	}

	/*
	 * Do until a valid packet found
	 */
	for (;;) {
		/*
		 * wait for interrupt
		 */
		if (data->nonblocking && sem_count(data->rx_sem) <= 0) {
			*lenp = 0;
			return (B_OK);
		}
	
		pos = rx_pos(data);
		if (data->rx_pos == pos || pos == (NRXQENTS - 1)) {
			err = acquire_sem_etc(data->rx_sem, 1, B_TIMEOUT, 2000000);
			if (err == B_TIMED_OUT) {
				if (data->closing) {
					return (B_INTERRUPTED);
				}
#if 0
				ddprintf("rx status = %08x, pos %d, exp %d\n",
							gc_read_rx_dma(DBDMA_STATUS),
							rx_pos(data), data->rx_pos);
#endif
				continue;
			}
			if (err < B_OK) {
				return (err);
			}
		}

		/*
		 * Okay, now we have a packet to be processed
		 */
		get_qent(&data->rx_queue[data->rx_pos], &qent);
		if (!(qent.status & DBDMA_CS_ACTIVE)) {
			continue;
		}

		/*
		 * Put the stop mark here
		 */
		nqent = qent;
		nqent.cmd = DMA_STOP;
		nqent.status = ~DBDMA_CS_ACTIVE;
		put_qent(&nqent, &data->rx_queue[data->rx_pos]);
		
		/*
		 * Remove the old stop mark
		 */
		nqent.cmd = DMA_INPUT_MORE;
		nqent.key = DMA_KEY_STREAM0;
		nqent.i = DBDMA_INTERRUPT_TRUE;
		nqent.b = DBDMA_BRANCH_NEVER;
		nqent.w = DBDMA_WAIT_NEVER;
		nqent.req_count = PACKET_BUF_SIZE;
		nqent.address = physical_address(data, &data->rx_vbuf[data->rx_stop]);
		nqent.status = ~DBDMA_CS_ACTIVE;
		put_qent(&nqent, &data->rx_queue[data->rx_stop]);
		read_restart(data);

		pos = data->rx_stop = data->rx_pos++;
		data->rx_pos %= NRXCHUNKS;

		if (qent.status & ~0x9440) {
			ddprintf("%d: read status=%04x, count=%04x\n", pos,
					 qent.status, qent.res_count);
		}

		if (!(qent.status & GC_MACE_EOP)) {
			wprintf("%d: Not EOP!\n", pos);
			continue;
		}
		if (qent.res_count > PACKET_BUF_SIZE) {
			wprintf("res_count too big: %d\n", qent.res_count);
			continue;
		}
		len = PACKET_BUF_SIZE - qent.res_count;
		if (len < 4) {
			wprintf("Warning: short length %d\n", len);
			continue;
		} 
		memcpy(&rs, &data->rx_vbuf[pos].data[len - 4], sizeof(rs));
		rs = swaplong(rs);
		if (rs & RCVFS_OFLO) {
			wprintf("o");
		}
		if (rs & RCVFS_FCS) {
			wprintf("k");
		}
		if (rs & RCVFS_CLSN) {
			wprintf("c");
		}
		if (rs & RCVFS_FRAM) {
			wprintf("f");
		}
		if (rs & 0xf000) {
			recv_reset(data);
			continue;
		}

		if (rs & RCVFS_RUNT) {
			ddprintf("r");
		}
		if (rs & RCVFS_COLL) {
			ddprintf("X");
		}
		rs &= 0xfff;
		len = rs;
		if (len > 1514) {
			len = 1514;
		}
		memcpy(&buf[0], &data->rx_vbuf[pos], len);
		if (my_packet(data, &buf[0])) {
			break;
		}
	}
	*lenp = len;
	return (B_OK);
}

static void
setup_dma(
		  gce_private_t *data,
		  int qpos,
		  unsigned buflen
		  )
{
	dma_command_t qent;

	/*
	 * Setup DMA
	 */
	qent.cmd = DMA_OUTPUT_LAST;
	qent.key = DMA_KEY_STREAM0;
	qent.i = DBDMA_INTERRUPT_NEVER;
	qent.b = DBDMA_BRANCH_NEVER;
	qent.w = DBDMA_WAIT_FALSE;
	qent.req_count = buflen;
	qent.address = physical_address(data, &data->tx_vbuf[0]);
	qent.status = ~DBDMA_CS_ACTIVE;
	qent.cmd_dep = 0;
	put_qent(&qent, &data->tx_queue[qpos]);

	qpos = mod_add(qpos, 1, QENTS_PER_TX);
	qent.cmd = DMA_LOAD_QUAD;
	qent.key = DMA_KEY_SYSTEM;
	qent.i = DBDMA_INTERRUPT_NEVER;
	qent.b = DBDMA_BRANCH_NEVER;
	qent.w = DBDMA_WAIT_NEVER;
	qent.req_count = 1;
	qent.address = physaddr(mace_gc_address(MACE_XMTFS));
	qent.status = ~DBDMA_CS_ACTIVE;
	qent.cmd_dep = 0;
	put_qent(&qent, &data->tx_queue[qpos]);


	qpos = mod_add(qpos, 1, QENTS_PER_TX);
	qent.cmd = DMA_LOAD_QUAD;
	qent.key = DMA_KEY_SYSTEM;
	qent.i = DBDMA_INTERRUPT_NEVER;
	qent.b = DBDMA_BRANCH_NEVER;
	qent.w = DBDMA_WAIT_NEVER;
	qent.req_count = 1;
	qent.address = physaddr(mace_gc_address(MACE_XMTRC));
	qent.status = ~DBDMA_CS_ACTIVE;
	qent.cmd_dep = 0;
	put_qent(&qent, &data->tx_queue[qpos]);


	qpos = mod_add(qpos, 1, QENTS_PER_TX);
	qent.cmd = DMA_LOAD_QUAD;
	qent.key = DMA_KEY_SYSTEM;
	qent.i = DBDMA_INTERRUPT_NEVER;
	qent.b = DBDMA_BRANCH_NEVER;
	qent.w = DBDMA_WAIT_NEVER;
	qent.req_count = 1;
	qent.address = physaddr(mace_gc_address(MACE_FIFOFC));
	qent.status = ~DBDMA_CS_ACTIVE;
	qent.cmd_dep = 0;
	put_qent(&qent, &data->tx_queue[qpos]);


	qpos = mod_add(qpos, 1, QENTS_PER_TX);
	qent.cmd = DMA_LOAD_QUAD;
	qent.key = DMA_KEY_SYSTEM;
	qent.i = DBDMA_INTERRUPT_ALWAYS;
	qent.b = DBDMA_BRANCH_NEVER;
	qent.w = DBDMA_WAIT_NEVER;
	qent.req_count = 1;
	qent.address = physaddr(mace_gc_address(MACE_IR));
	qent.status = ~DBDMA_CS_ACTIVE;
	qent.cmd_dep = 0;
	put_qent(&qent, &data->tx_queue[qpos]);

	qpos = mod_add(qpos, 1, QENTS_PER_TX);
	qent.cmd = DMA_STOP;
	put_qent(&qent, &data->tx_queue[qpos]);
}


void
dumpall(
		gce_private_t *data
		)
{
	int i;
	dma_command_t qent;
	
	ddprintf("status  = %08x\n", gc_read_tx_dma(DBDMA_STATUS));
	ddprintf("control = %08x\n", gc_read_tx_dma(DBDMA_CONTROL));
	ddprintf("waitsel = %08x\n",
			 gc_read_tx_dma(DBDMA_WAIT_SEL));
	
	ddprintf("xmtfs = %02x\n", mace_gc_read(MACE_XMTFS));
	ddprintf("xmtrc = %02x\n", mace_gc_read(MACE_XMTRC));
	ddprintf("fifofc = %02x\n", mace_gc_read(MACE_FIFOFC));
	ddprintf("ir = %02x\n", mace_gc_read(MACE_IR));
	ddprintf("IMR = %02x\n", 
			 mace_gc_read(MACE_IMR));
	ddprintf("UTR = %02x\n", 
			 mace_gc_read(MACE_UTR));
	ddprintf("PLSCC = %02x\n", 
			 mace_gc_read(MACE_PLSCC));
	ddprintf("BIUCC = %02xn",
			 mace_gc_read(MACE_BIUCC));
	
	for (i = 0; i < QENTS_PER_TX; i++) {
		char buf[12];
		get_qent(&data->tx_queue[i], &qent);
		sprintf(buf, "#%d", i);
		print_qent(buf, &qent);
	}
}

/*
 * Entry point: write a packet
 */
static status_t
gce_write(
		  void *cookie,
		  off_t xpos,
		  const void *buf,
		  size_t *lenp
		  )
{
	gce_private_t *data = (gce_private_t *)cookie;
	int buflen = *lenp;
	int pos;
	int qpos;
	status_t	status;
	dma_command_t qent;
	int i;
	bigtime_t timeout;
	
	for (;;) {
		if (data->closing) {
			return (B_INTERRUPTED);
		}

		status = acquire_sem_etc(data->wrsem, 1, B_TIMEOUT, 2000000);
		if (status != B_TIMED_OUT) {
			if (status < B_OK) {
				return (status);
			}
			break;
		}
	}

			
	/*
	 * Wait for slot
	 */
	for (;;) {
		if (data->closing) {
			return (B_INTERRUPTED);
		}
		status = acquire_sem_etc(data->tx_sem, 1, B_TIMEOUT, 2000000);
		if (status == B_TIMED_OUT) {
			wprintf("tx status = %08x\n", gc_read_tx_dma(DBDMA_STATUS));
			wprintf("tx pos: %d %d\n", data->tx_pos, tx_pos(data));
			get_qent(&data->tx_queue[tx_pos(data)], &qent);
			print_qent("q", &qent);
			if (data->tx_pos == tx_pos(data)) {
				wprintf("lost interrupt: %d\n", sem_count(data->tx_sem));
				dumpall(data);
				tx_dma_restart(data);
				release_sem(data->tx_sem);
			}
			continue;
		}
			
		if (status < B_OK) {
			release_sem(data->wrsem);
			return (status);
		}
		if (!(gc_read_tx_dma(DBDMA_STATUS) & DBDMA_CS_DEAD)) {
			break;
		}
		wprintf("tx error: %d %d\n", data->tx_pos, tx_pos(data));

		gc_write_tx_dma(DBDMA_CONTROL, DBDMA_CS_RUN << 16);
		while (gc_read_tx_dma(DBDMA_CONTROL) & DBDMA_CS_RUN) {
			wprintf("waiting for it to stop\n");
			snooze(1000000);
		}
		/* turn off transmitter */
		mace_gc_write(MACE_MACCC, MACCC_ENRCV);

#if 0
		ddprintf("status  = %08x\n", gc_read_tx_dma(DBDMA_STATUS));
		ddprintf("control = %08x\n", gc_read_tx_dma(DBDMA_CONTROL));
		ddprintf("waitsel = %08x\n",
				 gc_read_tx_dma(DBDMA_WAIT_SEL));
#endif
		/*
		 * DO NOT REMOVE THESE PRINTFS!
		 * The registers must be read to finish the transmission
		 */
		wprintf("xmtfs = %02x\n", mace_gc_read(MACE_XMTFS));
		wprintf("xmtrc = %02x\n", mace_gc_read(MACE_XMTRC));
		wprintf("fifofc = %02x\n", mace_gc_read(MACE_FIFOFC));
		wprintf("ir = %02x\n", mace_gc_read(MACE_IR));

#if 0
		for (i = 0; i < QENTS_PER_TX; i++) {
			char buf[12];
			get_qent(&data->tx_queue[i], &qent);
			sprintf(buf, "#%d", i);
			print_qent(buf, &qent);
		}
		ddprintf("reenabling dma engine\n");
#endif
		/*
		 * Set queue pointer
		 */
		gc_write_tx_dma(DBDMA_COMMAND_PTR,
				   (ulong)physical_address(data, data->tx_queue));
		
		get_qent(&data->tx_queue[data->tx_pos], &qent);

		qent.cmd = DMA_STOP;
		qent.status = ~DBDMA_CS_ACTIVE;
		put_qent(&qent, &data->tx_queue[0]);

		gc_write_tx_dma(DBDMA_WAIT_SEL,
				   ((GC_MACE_INTERRUPT << 16) | GC_MACE_INTERRUPT));
		
		/* turn on transmitter */
		mace_gc_write(MACE_MACCC, MACCC_ENXMT | MACCC_ENRCV);

		gc_write_tx_dma(DBDMA_CONTROL,
				   (DBDMA_CS_RUN << 16) | DBDMA_CS_RUN);
		
		snooze(1000);
		data->tx_pos = 0;
		setup_dma(data, 0, qent.req_count);

		gc_write_tx_dma(DBDMA_CONTROL,
				   (DBDMA_CS_WAKE << 16) + DBDMA_CS_WAKE);
	}

	/*
	 * Set positions
	 */
	data->tx_pos = qpos = tx_pos(data);

	/*
	 * copy buffer
	 */
	memcpy(&data->tx_vbuf[0], buf, buflen);

	if (buflen < ETHER_MIN_SIZE) {
		buflen = ETHER_MIN_SIZE;
	}

	setup_dma(data, qpos, buflen);
	
	/*
	 * Start DMA
	 */

	gc_write_tx_dma(DBDMA_CONTROL,
			   (DBDMA_CS_WAKE << 16) + DBDMA_CS_WAKE);

	release_sem(data->wrsem);
	return (B_OK);
}


/* 
 * Get the MACE chip revision
 */
static void
mace_getchipid(
			   ushort *id
			   )
{
	ulong u;
	
	u = mace_gc_read(MACE_CHIPID1);
	*id = u;
	u = mace_gc_read(MACE_CHIPID2);
	*id |=  (u << 8);
	*id &= 0x0fff;	/* forget the version number */
}

/* 
 * Reset the MACE
 */
static int
mace_reset(void)
{
	int bogus;

	mace_gc_write(MACE_MACCC, 0);
	mace_gc_write(MACE_BIUCC, BIUCC_SWRST);
	for (bogus = MAXBOGUS; 
		 bogus > 0 && (mace_gc_read(MACE_BIUCC) & BIUCC_SWRST);
		 bogus--) {
	}
	if (bogus == 0) {
		wprintf("Reset failed\n");
		return (0);
	}
	return (1);
}

/*
 * Transmit DMA interrupt
 */
static int32
tx_dma_intr(void *vdata)
{
	gce_private_t *data = (gce_private_t *)vdata;

	release_sem_etc(data->tx_sem, 1, B_DO_NOT_RESCHEDULE);
	return (B_HANDLED_INTERRUPT);
}

/*
 * Receive DMA interrupt
 */
static int32
rx_dma_intr(void *vdata)
{
	gce_private_t *data = (gce_private_t *)vdata;

	release_sem_etc(data->rx_sem, 1, B_DO_NOT_RESCHEDULE);
	return (B_HANDLED_INTERRUPT);
}

/*
 * open the driver
 */
static status_t
gce_open(
		 const char *name,
		 uint32 flags,
		 void **cookie
		 )
{
	area_id area;
	gce_private_t *data;
	ether_address_t address;

	if (!gce_getaddress(&address)) {
		return (B_ERROR);
	}
	data = (gce_private_t *)aligned_malloc(sizeof(*data), &area);
	if (!data) {
		return (B_NO_MEMORY);
	}

	data->area = area;
	data->init = false;
	*cookie = (void *)data;
	return (B_OK);
}

/*
 * close the driver
 */
static status_t
gce_close(
		  void *cookie
		  )
{
	gce_private_t *data = (gce_private_t *)cookie;

	if (data != NULL && data->init) {
		data->closing++;

		/*
		 * Stop MACE interrupts
		 */
		mace_gc_write(MACE_IMR, 0xff);

		/*
		 * Stop DMA interrupts (tx)
		 */
		gc_write_tx_dma(DBDMA_CONTROL, DBDMA_CS_RUN << 16);
			
		remove_io_interrupt_handler(intrs[TX], tx_dma_intr, data);

		/* 
		 * Stop DMA interrupts (rx)
		 */
		gc_write_rx_dma(DBDMA_CONTROL, DBDMA_CS_RUN << 16);
		remove_io_interrupt_handler(intrs[RX], rx_dma_intr, data);

		/*
		 * Delete sems and free data
		 */
		if (data->rx_sem >= B_OK) {
			release_sem_etc(data->rx_sem, 1000, B_DO_NOT_RESCHEDULE);
			delete_sem(data->rx_sem);
			data->rx_sem = B_ERROR;
		}
		if (data->tx_sem >= B_OK) {
			release_sem_etc(data->tx_sem, 1000, B_DO_NOT_RESCHEDULE);
			delete_sem(data->tx_sem);
			data->tx_sem = B_ERROR;
		}
		if (data->wrsem >= B_OK) {
			release_sem_etc(data->wrsem, 1000, B_DO_NOT_RESCHEDULE);
			delete_sem(data->wrsem);
			data->wrsem = B_ERROR;
		}
		snooze(1000000);	/* give driver a chance to get out */
	}
	return (B_OK);
}

/*
 * Print an ethernet address
 */
static void
print_address(
			  char *format,
			  ether_address_t *addr
			  )
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	wprintf(format, buf);
}


/*
 * Inititalize the driver
 */
static status_t
gce_init(
		 gce_private_t *data
		 )
{
	unsigned short id;
	status_t status;
	int i;

	data->physical = physaddr(data);

	ddprintf("Starting up: size %d, address %08x, phys %08x, area %d\n", 
			 sizeof(*data), 
			 data, data->physical, data->area);

	if (!gce_getaddress(&data->myaddress)) {
		wprintf("Can't find ethernet address.\n");
		return (B_ERROR);
	}

	print_address("Ethernet address: %s\n", &data->myaddress);

	mace_getchipid(&id);
	ddprintf("chip id: %04x\n", id);
	if (id != MACE_CHIP_ID) {
		wprintf("Chip revision not supported: %04x\n", id);
		return (B_ERROR);
	}
	if (!mace_reset()) {
		return (B_ERROR);
	}

	/*
	 * Disable UTR (User Test Register)
	 */
	ddprintf("UTR = %02x, set to %02x\n", 
			 mace_gc_read(MACE_UTR), UTR_RTRD);
	mace_gc_write(MACE_UTR, UTR_RTRD);

	/*
	 * Disable all interrupts
	 */
	ddprintf("IMR = %02x, set to %02x\n", 
			 mace_gc_read(MACE_IMR), 0xff);
	mace_gc_write(MACE_IMR, 0xff);

#if 0
	ddprintf("BIUCC = %02x, set to %02x\n", 
			 mace_gc_read(MACE_BIUCC), BIUCC_XMTSP64);
	mace_gc_write(MACE_BIUCC, BIUCC_XMTSP64);
#endif

#if 0
	/*
	 * The default is correct
	 */
	ddprintf("XMTFC = %02x, set to %02x\n", 
			 mace_gc_read(MACE_XMTFC), XMTFC_APADXMT);
	mace_gc_write(MACE_XMTFC, XMTFC_APADXMT);
#endif

	ddprintf("RCVFC = %02x, set to %02x\n", 
			mace_gc_read(MACE_RCVFC), RCVFC_ASTRP);
	mace_gc_write(MACE_RCVFC, RCVFC_ASTRP);

	/*
	 *  Initialize FIFO
	 */
	ddprintf("FIFOCC = %02x, set to %02x\n", 
			mace_gc_read(MACE_FIFOCC), 
			FIFOCC_XMTFWU | FIFOCC_XMTFW_8 |
			FIFOCC_RCVFWU | FIFOCC_RCVFW_64);
	mace_gc_write(MACE_FIFOCC, 
				  FIFOCC_XMTFWU | FIFOCC_XMTFW_8 |
				  FIFOCC_RCVFWU | FIFOCC_RCVFW_64);


	ddprintf("PLSCC = %02x, set to %02x\n", 
			 mace_gc_read(MACE_PLSCC),
			 PLSCC_GPSI | PLSCC_ENPLSIO);
	mace_gc_write(MACE_PLSCC, PLSCC_GPSI | PLSCC_ENPLSIO);


#if 0
	/*
	 * Auto-select media: not valid for CURIO
	 */
	ddprintf("PHYCC = %02x\n", mace_gc_read(MACE_PHYCC));
	mace_gc_write(MACE_PHYCC, PHYCC_ASEL);
#endif


	/*
	 * Load the physical address
	 */
	mace_gc_write(MACE_IAC, IAC_PHYADDR | IAC_ADDRCHG);
	for (i = 0; i < MAXBIGBOGUS; i++) {
		if (!(mace_gc_read(MACE_IAC) & IAC_ADDRCHG)) {
			break;
		}
	}
	if (i == MAXBIGBOGUS) {
		wprintf("max bogus reached\n");
	}
	for (i = 0; i < 6; i++) {
		mace_gc_write(MACE_PADR, data->myaddress.ebyte[i]);
	}


	/*
	 * Load the logical address (zero initially)
	 */
	data->nmulti = 0;
	mace_gc_write(MACE_IAC, IAC_LOGADDR | IAC_ADDRCHG);
	for (i = 0; i < MAXBIGBOGUS; i++) {
		if (!(mace_gc_read(MACE_IAC) & IAC_ADDRCHG)) {
			break;
		}
	}
	if (i == MAXBIGBOGUS) {
		wprintf("max bogus reached\n");
	}
	for (i = 0; i < 8; i++) {
		mace_gc_write(MACE_LADRF, 0);
	}

	/*
	 * Allocate sems
	 */
	data->rx_sem = create_sem(0, "mace read");
	if (data->rx_sem < B_OK) {
		ddprintf("No more sems!\n");
		return (B_ERROR);
	}
	data->tx_sem = create_sem(1, "mace write");
	if (data->tx_sem < B_OK) {
		ddprintf("No more sems!\n");
		return (B_ERROR);
	}
	data->wrsem = create_sem(1, "gce write");
	
	set_sem_owner(data->rx_sem, B_SYSTEM_TEAM);
	set_sem_owner(data->tx_sem, B_SYSTEM_TEAM);
	set_sem_owner(data->wrsem, B_SYSTEM_TEAM);

	/*
	 * Install interrupts
	 */
	ddprintf("Clearing interrupts...\n");
	ddprintf("XMTFS = %02x\n", mace_gc_read(MACE_XMTFS));
	ddprintf("XMTRC = %02x\n", mace_gc_read(MACE_XMTRC));
	ddprintf("FIFOFC = %02x\n", mace_gc_read(MACE_FIFOFC));
	ddprintf("IR = %02x\n", mace_gc_read(MACE_IR));
	
	install_io_interrupt_handler(intrs[TX], tx_dma_intr, data, 0);
	install_io_interrupt_handler(intrs[RX], rx_dma_intr, data, 0);

	data->closing = 0;
	data->nonblocking = false;
	
	tx_dma_restart(data);
	rx_dma_restart(data);

	/*
	 * Turn on xmt interrupt
	 */
	ddprintf("IMR = %02x, set to %02x\n", 
			 mace_gc_read(MACE_IMR), ~(IMR_XMTINTM));
	mace_gc_write(MACE_IMR, ~(IMR_XMTINTM));

	/*
	 * Start transmitting and receiving
	 */
	mace_gc_write(MACE_MACCC, MACCC_ENXMT | MACCC_ENRCV);
	ddprintf("So far so good\n");

	data->init = true;
	return (B_OK);
}

static status_t
add_multi(
		  gce_private_t *data,
		  const char *addr
		  )
{
	int i;

	for (i = 0; i < data->nmulti; i++) {
		if (memcmp(addr, &data->multi[i], sizeof(data->multi[i])) == 0) {
			return (B_OK);
		}
	}
	if (data->nmulti == MAXMULTI) {
		return (B_ERROR);
	}
	if (data->nmulti == 0) {
		/*
		 * For now, turn on all the multicast groups
		 */
		mace_gc_write(MACE_IAC, IAC_LOGADDR | IAC_ADDRCHG);
		for (i = 0; i < MAXBIGBOGUS; i++) {
			if (!(mace_gc_read(MACE_IAC) & IAC_ADDRCHG)) {
				break;
			}
		}
		if (i == MAXBIGBOGUS) {
			wprintf("max bogus reached\n");
		}
		for (i = 0; i < 8; i++) {
			mace_gc_write(MACE_LADRF, 0xff);
		}
		mace_gc_write(MACE_MACCC, MACCC_ENXMT | MACCC_ENRCV);
	}
	memcpy(&data->multi[data->nmulti++], addr, sizeof(data->multi[i]));
	return (B_OK);
}

static status_t
rem_multi(
		  gce_private_t *data,
		  const char *addr
		  )
{
	int i;

	for (i = 0; i < data->nmulti; i++) {
		if (memcmp(addr, &data->multi[i], sizeof(data->multi[i])) == 0) {
			memcpy(&data->multi[i], &data->multi[data->nmulti - 1], 
				   sizeof(data->multi[i]));
			data->nmulti--;
			return (B_OK);
		}
	}
	return (B_ERROR);
}

/*
 * Standard driver control function
 */
static status_t
gce_control(
			void *cookie,
			uint32 msg,
			void *buf,
			size_t len
			)
{
	gce_private_t *data = (gce_private_t *)cookie;
	int nonblocking;


	switch (msg) {
	case ETHER_INIT:
		return (gce_init(data));

	case ETHER_ADDMULTI:
		if (data->init) {
			return (add_multi(data, (const char *)buf));
		}
		break;

	case ETHER_REMMULTI:
		if (data->init) {
			return (rem_multi(data, (const char *)buf));
		}
		break;

	case ETHER_GETADDR:
		if (data->init) {
			memcpy(buf, &data->myaddress, sizeof(data->myaddress));
			return (B_OK);
		}
		break;

	case ETHER_NONBLOCK:
		if (data->init) {
			memcpy(&nonblocking, buf, sizeof(nonblocking)); 
			data->nonblocking = nonblocking; /* convert to bool */
			return (B_NO_ERROR);
		}
		break;

	}
	return (B_ERROR);
}

static status_t
gce_free(
		   void *cookie
		   )
{
	gce_private_t *data = (gce_private_t *)cookie;
	
	aligned_free(data->area);
	return (B_OK);
}

/*
 * loadable driver stuff
 */
static char *gce_name[] = { "net/gce/0", NULL };

static device_hooks gce_device = {
	gce_open,
	gce_close,
	gce_free,
	gce_control,
	gce_read,
	gce_write
};


const char **
publish_devices(void)
{
	return (gce_name);
}

device_hooks *
find_device(const char *name)
{
	return (&gce_device);
}

status_t
init_driver (void)
{
	ddprintf ("In init_driver");

	if (get_mac_device_info ("mace", 3, &pa[0], &va[0], 3, &intrs[0]) < 0) {
		ddprintf ("No mace chip.  Sorry, brad\n");
		return B_ERROR;
	}
	ddprintf ("MACE ethernet detected\n");
	return B_NO_ERROR;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */

