/*****************************************************************************\
 * NE2000 Compatible Ethernet driver
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 * written by Chris Liscio
\*****************************************************************************/

/*****************************************************************************\
 * IMPLEMENTATION NOTES:
 * 
 * General:
 * This driver (unlike the former implementation) is mostly bus-agnostic.  It
 * supports PCI, ISA, and ISAPnP cards.  It can later be extended to support
 * cardbus or even PCMCIA hardware quite easily.  The driver itself strives
 * to act like the hardware in that it tries to retain half-duplex behavior
 * in order to prevent read/write overlaps.  As a result, the read_hook and
 * write_hook both share a hardware lock to ensure no overlap.
 *
 * Receiving:
 * Upon a receive interrupt, the driver is signalled to allow the read_hook
 * to read a packet from the card's remote DMA channel.  The read_hook then
 * calls copy_packets to grab packets off the card's buffer.
 * 
 * Transmitting:
 * This driver uses a double-buffering scheme to speed up packet transmits
 * considerably.  The double buffering allows the write hook to be called
 * even when the transmitter is busy writing the packet out the wire.  This
 * scheme uses a few locking techniques to ensure that nothing on the
 * card gets overwritten before it's finished going out the wire.  There are
 * separate locks for the hardware and the two actual buffers.
 * 
 * Cleanup:
 * When the card receives an overwrite warning interrupt, the driver signals
 * the cleanup_thread to perform a NatSemi-perscribed cleanup routine which
 * calls for copying packets out of the card's ring to free up space.  This
 * driver only copies one packet out, throws it away, and allows
 * the driver to resume its normal operation.
\*****************************************************************************/

/*****************************************************************************\
 * Includes
\*****************************************************************************/

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <ether_driver.h>
#include <stdarg.h>
#include <PCI.h>
#include <ISA.h>
#include <driver_settings.h>
#include <isapnp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
/* fc_lock.h added rather than pasting a bunch of code */
#include "fc_lock.h"

/* PCMCIA required defines */
#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

/*****************************************************************************\
 * Compillation Flags
\*****************************************************************************/
/* Check for certain braindead cards (especially the D-Link DE660 pcmcia card) which
 * don't correctly write out their dma write addresses */
/* #define MOUT_SANITY_CHECK */
/* Fix the NS8390 read-before-write bug which pops up on certain cards.  Keep disabled
 * if no problems exist.  If problems are found, we're going to have to just check
 * against specific cards in the ne2k_mout routine */
/* #define NS8390_RW_BUGFIX */

/*****************************************************************************\
 * Globals/Defines for driver instances
\*****************************************************************************/

#define kDevName 			"ne2k"
#define kDevDir 			"net/" kDevName "/"
#define MAX_CARDS 			 4		   /* maximum number of driver instances */

/*****************************************************************************\
 * Debug flag defines
\*****************************************************************************/

#define	ERR				0x0001
#define INFO				0x0002
#define RX				0x0004		/* dump received frames */
#define TX				0x0008		/* dump transmitted frames */
#define INTERRUPT			0x0010		/* interrupt calls */
#define FUNCTION			0x0020		/* function calls */
#define PCI_IO				0x0040		/* pci reads and writes */
#define SEQ				0x0080		/* tx & rx TCP/IP sequence numbers */
#define WARN				0x0100		/* Warnings - off on final release */
#define OTHER				0x0200		/* Other stuff */
#define TIMER				0x0400		/* Timer information */
#define ABSOLUTE_INSANITY		0xFFFF		/* ALL Interrupts */

/* Feel free to change these defines to suit your debugging needs */
//#define DEFAULT_DEBUG_FLAGS (ABSOLUTE_INSANITY)
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | FUNCTION )
//#define DEFAULT_DEBUG_FLAGS (INTERRUPT)
#define DEFAULT_DEBUG_FLAGS ERR
//#define DEFAULT_DEBUG_FLAGS 0

#ifdef DEBUG_DRIVER
#define ETHER_DEBUG(id, flags, args...) if (flags & gDev[id].debug) dprintf(kDevName ": " args)
#define DEBUGGER_COMMAND 		1		/* Enable the debugger command */
#else
#define ETHER_DEBUG(id, flags, args...)
#endif
/* For the hard-core debug people ONLY */
//#define ETHER_DEBUG(id, flags, args...) dprintf(kDevName ": " args)

/*****************************************************************************\ 
 * Hardware specific register, descriptors, & defines 
\*****************************************************************************/

#define NE2K__MAX_FRAME_SIZE		1514	/* 1500 + 4 bytes checksum added by card hardware */
#define NE2K__MIN_FRAME_SIZE		60

#define NE2K__MAX_MULTI			32
#define NE2K__ETHER_ADDR_LEN		6

/* Page locations */
#define NE2K__NE2000_BUFFER_START	0x4000
#define NE2K__NE2000_BUFFER_LENGTH	0x4000

/* Number of TX pages (double for double buffering) */
#define NE2K__TX_SINGLE_PAGE		6
//#define NE2K__TX_PAGES		NE2K__TX_SINGLE_PAGE
#define NE2K__TX_PAGES			NE2K__TX_SINGLE_PAGE * 2
#define NE2K__TX_TIMEOUT		40000	/* 40ms should suffice here */

/* Number of buffered rx packets to store */
#define NE2K__RX_BUFFERS		16
#define NE2K__RX_BUFFERS_MSK		(NE2K__RX_BUFFERS - 1)

#define NE2K__MAX_WORK			100
#define NE2K__PAGE_SIZE			256			/* The page size, in bytes */

/* page-independent registers */
#define NE2K_CR				0x00
#define NE2K_DATA			0x10		/* NatSemi-defined Port Window offset */
#define NE2K_RESET			0x1F		/* Read to Reset, Write to Clear */

/* Remember, 0 - Low, 1 - High byte */

/* Page 0 Registers - READ */
#define NE2K0_CLDA0_R			0x01
#define NE2K0_CLDA1_R			0x02
#define NE2K0_BNRY_R			0x03
#define NE2K0_TSR_R			0x04
#define NE2K0_NCR_R			0x05
#define NE2K0_FIFO_R			0x06
#define NE2K0_ISR_R			0x07
#define NE2K0_CRDA0_R			0x08
#define NE2K0_CRDA1_R			0x09
#define NE2K0_RSRV0_R			0x0A
#define NE2K0_RSRV1_R			0x0B
#define NE2K0_RSR_R			0x0C
#define NE2K0_CNTR0_R			0x0D
#define NE2K0_CNTR1_R			0x0E
#define NE2K0_CNTR2_R			0x0F
/* Page 0 Registers - WRITE */
#define NE2K0_PSTART_W			0x01
#define NE2K0_PSTOP_W			0x02
#define NE2K0_BNRY_W			0x03
#define NE2K0_TPSR_W			0x04
#define NE2K0_TBCR0_W			0x05
#define NE2K0_TBCR1_W			0x06
#define NE2K0_ISR_W			0x07
#define NE2K0_RSAR0_W			0x08
#define NE2K0_RSAR1_W			0x09
#define NE2K0_RBCR0_W			0x0A
#define NE2K0_RBCR1_W			0x0B
#define NE2K0_RCR_W			0x0C
#define NE2K0_TCR_W			0x0D
#define NE2K0_DCR_W			0x0E
#define NE2K0_IMR_W			0x0F

/* Page 1 Registers - READ */
#define NE2K1_PAR0_R			0x01
#define NE2K1_PAR1_R			0x02
#define NE2K1_PAR2_R			0x03
#define NE2K1_PAR3_R			0x04
#define NE2K1_PAR4_R			0x05
#define NE2K1_PAR5_R			0x06
#define NE2K1_CURR_R			0x07
#define NE2K1_MAR0_R			0x08
#define NE2K1_MAR1_R			0x09
#define NE2K1_MAR2_R			0x0A
#define NE2K1_MAR3_R			0x0B
#define NE2K1_MAR4_R			0x0C
#define NE2K1_MAR5_R			0x0D
#define NE2K1_MAR6_R			0x0E
#define NE2K1_MAR7_R			0x0F
/* Page 1 Registers - WRITE */
#define NE2K1_PAR0_W			0x01
#define NE2K1_PAR1_W			0x02
#define NE2K1_PAR2_W			0x03
#define NE2K1_PAR3_W			0x04
#define NE2K1_PAR4_W			0x05
#define NE2K1_PAR5_W			0x06
#define NE2K1_CURR_W			0x07
#define NE2K1_MAR0_W			0x08
#define NE2K1_MAR1_W			0x09
#define NE2K1_MAR2_W			0x0A
#define NE2K1_MAR3_W			0x0B
#define NE2K1_MAR4_W			0x0C
#define NE2K1_MAR5_W			0x0D
#define NE2K1_MAR6_W			0x0E
#define NE2K1_MAR7_W			0x0F

/* Page 2 Registers - READ */
#define NE2K2_PSTART_R			0x01
#define NE2K2_PSTOP_R			0x02
#define NE2K2_RNPP_R			0x03
#define NE2K2_TPSR_R			0x04
#define NE2K2_LNPP_R			0x05
#define NE2K2_ACU_R			0x06
#define NE2K2_ACL_R			0x07
#define NE2K2_RSRV0_R			0x08
#define NE2K2_RSRV1_R			0x09
#define NE2K2_RSRV2_R			0x0A
#define NE2K2_RSRV3_R			0x0B
#define NE2K2_RCR_R			0x0C
#define NE2K2_TCR_R			0x0D
#define NE2K2_DCR_R			0x0E
#define NE2K2_IMR_R			0x0F
/* Page 2 Registers - WRITE */
#define NE2K2_CLDA0_W			0x01
#define NE2K2_CLDA1_W			0x02
#define NE2K2_RNPP_W			0x03
#define NE2K2_RSRV0_W			0x04
#define NE2K2_LNPP_W			0x05
#define NE2K2_ACU_W			0x06
#define NE2K2_ACL_W			0x07
#define NE2K2_RSRV1_W			0x08
#define NE2K2_RSRV2_W			0x09
#define NE2K2_RSRV3_W			0x0A
#define NE2K2_RSRV4_W			0x0B
#define NE2K2_RSRV5_W			0x0C
#define NE2K2_RSRV6_W			0x0D
#define NE2K2_RSRV7_W			0x0E
#define NE2K2_RSRV8_W			0x0F

/* Register Masks */
#define NE2K_CR_PS_MSK			0xC0
#define NE2K_CR_RD_MSK			0x38

/* Register Values */
#define NE2K_CR_STOP			0x01
#define NE2K_CR_START			0x02
#define NE2K_CR_TXP			0x04
#define NE2K_CR_RD_NA			0x00
#define NE2K_CR_RD_RR			0x08
#define NE2K_CR_RD_RW			0x10
#define NE2K_CR_RD_SP			0x18
#define NE2K_CR_RD_ARD			0x20
#define NE2K_CR_PS_P0			0x00
#define NE2K_CR_PS_P1			0x40
#define NE2K_CR_PS_P2			0x80
#define NE2K_CR_PS_P3			0xC0

#define NE2K0_DCR_WTS_BYTE		0x00
#define NE2K0_DCR_WTS_WORD		0x01
#define NE2K0_DCR_BOS_LEND		0x00
#define	NE2K0_DCR_BOS_BEND		0x02
#define NE2K0_DCR_LAS_D16B		0x00
#define NE2K0_DCR_LAS_S32B		0x04
#define NE2K0_DCR_LS_LOOPB		0x00
#define NE2K0_DCR_LS_NORML		0x08
#define NE2K0_DCR_ARM_SCNE		0x00
#define NE2K0_DCR_ARM_SCEX		0x10	// Not to be used on PPC
#define NE2K0_DCR_FTS_1WD		0x00
#define NE2K0_DCR_FTS_2WD		0x20
#define NE2K0_DCR_FTS_4WD		0x40
#define NE2K0_DCR_FTS_6WD		0x60

#define NE2K0_TCR_CRC_APP		0x00
#define NE2K0_TCR_CRC_INH		0x01
#define NE2K0_TCR_LB_NORM		0x00
#define NE2K0_TCR_LB_INNLB		0x02
#define NE2K0_TCR_LB_INELB		0x04
#define NE2K0_TCR_LB_EXTLB		0x06
#define NE2K0_TCR_ATD_OFF		0x00
#define NE2K0_TCR_ATD_ON		0x08
#define NE2K0_TCR_OFST_NORM		0x00
#define NE2K0_TCR_OFST_MOD		0x10

#define NE2K0_RCR_SEP_RER		0x00
#define NE2K0_RCR_SEP_REA		0x01
#define NE2K0_RCR_AR_F64R		0x00
#define NE2K0_RCR_AR_F64A		0x02
#define NE2K0_RCR_AB_BCR		0x00
#define NE2K0_RCR_AB_BCA		0x04
#define NE2K0_RCR_AM_MCNC		0x00
#define NE2K0_RCR_AM_MCC		0x08
#define NE2K0_RCR_PRO_OFF		0x00
#define NE2K0_RCR_PRO_ON		0x10
#define NE2K0_RCR_MON_BUF		0x00
#define NE2K0_RCR_MON_NBUF		0x20

#define NE2K0_ISR_INT_MSK		0x7F
#define NE2K0_ISR_PRX			0x01
#define NE2K0_ISR_PTX			0x02
#define NE2K0_ISR_RXE			0x04
#define NE2K0_ISR_TXE			0x08
#define NE2K0_ISR_OVW			0x10
#define NE2K0_ISR_CNT			0x20
#define NE2K0_ISR_RDC			0x40
#define NE2K0_ISR_RST			0x80

#define NE2K0_IMR_PRXE			0x01
#define NE2K0_IMR_PTXE			0x02
#define NE2K0_IMR_RXEE			0x04
#define NE2K0_IMR_TXEE			0x08
#define NE2K0_IMR_OVWE			0x10
#define NE2K0_IMR_CNTE			0x20
#define NE2K0_IMR_RDCE			0x40
#define NE2K0_IMR_ALL			0x7F

#define NE2K0_RSR_PRX			0x01
#define NE2K0_RSR_CRC			0x02
#define NE2K0_RSR_FAE			0x04
#define NE2K0_RSR_FO			0x08
#define NE2K0_RSR_MPA			0x10
#define NE2K0_RSR_PHY			0x20
#define NE2K0_RSR_DIS			0x40
#define NE2K0_RSR_DFR			0x80

#define NE2K0_TSR_PTX			0x01
#define NE2K0_TSR_ND			0x02
#define NE2K0_TSR_COL			0x04
#define NE2K0_TSR_ABT			0x08
#define NE2K0_TSR_CRS			0x10
#define NE2K0_TSR_FU			0x20
#define NE2K0_TSR_CDH			0x40
#define NE2K0_TSR_OWC			0x80

/****************************************************************************\
 * Useful Structures
\****************************************************************************/

/* The 8390 specific per-packet-header format. */
typedef struct _ne2k_packet_header {
	uint8	status; /* status */
	uint8	next;   /* pointer to next packet. */
	uint16	count; /* header + packet length in bytes */
} ne2k_packet_header;

/****************************************************************************\
 * Per driver instance globals 
\****************************************************************************/

typedef struct {
	int32			devID; 						/* device identifier: 0-n */

	/* Ethernet Card-Specific "Swappable Defines" */
	int				buffer_start;				/* The start of the on-card buffer */
	int				buffer_length;				/* The length of the on-card buffer */

	int				num_pages;					/* Number of total pages on the card buffer */
	int				rx_start_page, rx_stop_page;/* rx buffer boundaries */
	int				tx_start_page;				/* tx buffer start page */
	vint32			rx_current_page;			/* rx current page pointer */

	/* Useless? */
	int				ring_buf_start, ring_buf_size;/* Total buffer start and size */

	fc_lock_t		fc_rx_lock;					/* fc locks for tx/rx */

	sem_id			sem_tx_lock;				/* tx lock semaphore */
	sem_id			sem_reset_lock;				/* Reset thread trigger semaphore */
	sem_id			sem_hw_lock;				/* total hardware lock semaphore */
	sem_id			sem_rdma_done;				/* dma complete semaphore */

	spinlock		spin_page_lock;				/* Spinlock to protect page switches */
	spinlock		spin_trig_lock;				/* tx_trigger lock */
	spinlock		spin_dbuf_lock;				/* double buffer lock */

	vint32			dbuf_read_index;			/* double buffer read index (0/1) */
	vint32			dbuf_write_index;			/* double buffer write index (0/1) */
	vuint32			dbuf_pending;				/* pending buffer count */
	vint32			dbuf_size[2];				/* The size of both buffered packets */

	uint32			base; 						/* Base address of card registers */
	char*			isa_mem_base;				/* The base of ISA memory */

	thread_id		cleanup_thread_id;			/* Cleanup thread identifier */

	vint32			dmaLock;					/* atomic_or DMA Lock for protection */

	vint32			blockFlag;					/* sync or async io calls */

	ether_address_t		mac_address;			/* my ethernet address */
	ether_address_t		multi[NE2K__MAX_MULTI];	/* multicast addresses */
	int32			multi_count;

	vint32			status_ovw;					/* Overwrite mode status */
	vint32			status_rdc;					/* remote DMA complete status */

	/* Necessary statistics that need to be read by the card */
	vint32			stat_rx_frame_errors;
	vint32			stat_rx_crc_errors;
	vint32			stat_rx_missed_errors;
#ifdef STAT_COUNTING
	/* Statistics counters */
	vint32			stat_rx_packets;
	vint32			stat_rx_size_errors;
	vint32			stat_rx_fifo_errors;
	vint32			stat_rx_bogus_errors;
	vint32			stat_rx_resets;

	vint32			stat_tx_packets;
	vint32			stat_tx_errors;
	vint32			stat_tx_aborted_errors;
	vint32			stat_tx_carrier_errors;
	vint32			stat_tx_fifo_errors;
	vint32			stat_tx_heartbeat_errors;
	vint32			stat_tx_window_errors;
	vint32			stat_tx_triggers;
	vint32			stat_tx_buffered;
	vint32			stat_tx_busy_bail;
	vint32			stat_tx_not_pending_bail;
	vint32			stat_tx_ovw_resends;

	vint32			stat_rdma_conflicts;
	vint32			stat_rdma_timeouts;

	vint32			stat_ovw_rw_busy_defer;
	vint32			stat_ovw_reentry_defer;

	vint32			stat_collisions;
	
	vint32			stat_int_calls;
	vint32			stat_ints;
	vint32			stat_ints_dropped;
	vint32			stat_int_prx;
	vint32			stat_int_ptx;
	vint32			stat_int_rxe;
	vint32			stat_int_txe;
	vint32			stat_int_ovw;
	vint32			stat_int_cnt;
	vint32			stat_int_rdc;
	vint32			stat_int_rst;
#endif // STAT_COUNTING
} device_info_t;

/*****************************************************************************\
 * Global variables
\*****************************************************************************/

static pci_module_info		*gModInfo;				/* Global Module info (bus-independent...) */
static pci_module_info		*gPCIModInfo;				/* PCI-Specific module info */
static isa_module_info		*gISAModInfo;				/* ISA-Specific module info */
static char 			*gDevNameList[MAX_CARDS + 1];		/* Stores a list of device names */
static int32			gOpenMask = 0;				/* Global device open mask */
static int32			gDevID = 0;				/* Global device ID index */
static int32			gDevCount = 0;				/* Global device count */
static vint32			gCleanupBlockFlag = 0;			/* Blocking flag for the cleanup thread */

/* Global device info - one for every device */
struct global_dev_info {
	pci_info*	pciInfo;	/* pciInfo structure pointer */
	device_info_t*	devInfo;	/* Maintain the dev pointer for debug command */
	dev_link_t*	devLink;	/* PCMCIA dev_link_t structure pointer */
	uint32		flags;		/* ISA/PCI/ISAPnP/PCMCIA bus? */
	uint16		cardType;	/* Card type from the PCI or ISA card enums */
	uint16		debug;		/* Per driver debug level */
	uint16		port;		/* Port number */
	uint16		irq;		/* irq level */
} gDev[MAX_CARDS + 1];

/****************************************************************************\
 * I/O Routines
\****************************************************************************/

/*  We use io port pci bus access here by default. */
/* This should keep the general module info in mind and we'll define gModInfo later... */
#define write8(address, value)			(*gModInfo->write_io_8)((address), (value))
#define write16(address, value)			(*gModInfo->write_io_16)((address), (value))
#define write32(address, value)			(*gModInfo->write_io_32)((address), (value))
#define read8(address)				((*gModInfo->read_io_8)(address))
#define read16(address)				((*gModInfo->read_io_16)(address))
#define read32(address)				((*gModInfo->read_io_32)(address))

/****************************************************************************\
 * Function Prototypes 
\****************************************************************************/

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

/* Driver Entry Points */
_EXPORT status_t init_hardware(void );
_EXPORT status_t init_driver(void );
_EXPORT void uninit_driver(void );
_EXPORT const char** publish_devices(void );
_EXPORT device_hooks *find_device(const char *name );
static int32 interrupt_hook(void *_device);

/* Hook Functions */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie, uint32 msg, void *buf, size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf, size_t *len);
static status_t write_hook(void *_device,off_t pos, const void *buf, size_t *len);

/* PCI-Related Functions */
static int32 get_pci_list(struct global_dev_info info[], int32 maxEntries);
static status_t free_pci_list(struct global_dev_info info[]);				/* Free storage used by pci_info list */

/* ISA-Related Functions */
static int32 get_isa_list(struct global_dev_info info[], int32 maxEntries, int32 curEntries);
static int32 load_ne2k_settings(struct global_dev_info info[], int32 maxEntries, int32 curEntries);

/* Probing Functions */
static int32 probe_ne2000(device_info_t *device);

/* Card I/O Functions */
static void ne2k_min(device_info_t *device, uint8 *dst, uint32 src, uint32 len);
static status_t ne2k_mout(device_info_t *device, uint32 dst, uint8 *src, uint32 len);

/* Helper Functions */
static uchar log2(uint32 mask);
static void print_address(device_info_t *device);					/* Prints the mac address of the card */
static void dump_packet( const char * msg, uint8 * buf, uint16 size); /* Dump a packet to the debugger */
static status_t init_ne2k(device_info_t *device);					/* Initializes the ne2k (like the NS spec sez..) */
static status_t enable_addressing(device_info_t *device); 			/* enable pci/isa io address space for device */
static status_t allocate_resources(device_info_t *device);			/* allocate semaphores & spinlocks */
static void  free_resources(device_info_t *device);				/* deallocate semaphores & spinlocks */
static status_t domulti( device_info_t *device, char *addr );
static void setpromisc(device_info_t * device, uint32 on);
static status_t ne2k_trigger_send(device_info_t *device);

/* PCMCIA Specific Functions */
status_t ne2k_pcmcia_init_driver(void);
void ne2k_pcmcia_uninit_driver(void);
static void ne2k_pcmcia_config(dev_link_t *link);
static void ne2k_pcmcia_release(u_long arg);
static int ne2k_pcmcia_event(event_t event, int priority, event_callback_args_t *args);
static dev_link_t *ne2k_pcmcia_attach(void);
static void ne2k_pcmcia_detach(dev_link_t *);

/* Interrupt functions */
static int32 ne2k_interrupt_prx(device_info_t *device);
static int32 ne2k_interrupt_ptx(device_info_t *device);
static int32 ne2k_interrupt_ovw(device_info_t *device);
static int32 ne2k_interrupt_txe(device_info_t *device);

/* Cleanup thread function */
static status_t ne2k_cleanup_thread(void *_device);

/* Serial debug command*/
#if DEBUGGER_COMMAND
static int 		ne2k(int argc, char **argv);
#endif

static device_hooks gDeviceHooks =  {
	open_hook,	/* -> open entry point */
	close_hook,	/* -> close entry point */
	free_hook,	/* -> free entry point */
	control_hook,	/* -> control entry point */
	read_hook,	/* -> read entry point */
	write_hook,	/* -> write entry point */
	NULL,		/* -> select entry point */
	NULL,		/* -> deselect entry point */
	NULL,		/* -> readv */
	NULL		/* -> writev */
};

/****************************************************************************\
 * PCI Device list
\****************************************************************************/

/* Card flags for the gDev structure */
enum {
	NE2K__BUS_MSK		=0x00000007,
	NE2K__BUS_ISA		=0x00000001,
	NE2K__BUS_PCI		=0x00000002,
	NE2K__BUS_PCMCIA	=0x00000004,
	NE2K__BUS_RESERVED	=0x00000008,
	NE2K__STOP_PG_0x60	=0x00000020,
	NE2K__32BIT_IO_ONLY	=0x00000040,
	NE2K__16BIT_IO_ONLY	=0x00000080,
	NE2K__FORCE_FDX		=0x00000200,
	NE2K__REALTEK_FDX	=0x00000400,
	NE2K__HOLTEK_FDX	=0x00000800,
	NE2K__CARD_OPENED	=0x00001000,
	NE2K__CARD_INSERTED	=0x00002000,
};

enum ne2k_pci_chipsets {
	Realtek_RTL_8029 = 0,
	Winbond_89C940,
	Compex_RL2000,
	KTI_ET32P2,
	NetVin_NV5000SC,
	Via_86C926,
	SureCom_NE34,
	Winbond_W89C940F,
	Holtek_HT80232,
	Holtek_HT80229,
	NE2K__CHIPSET_COUNT,
};

static struct {
	char	*name;
	uint16	dev_vendor;
	uint16	dev_device;
	int	dev_flags;
} pci_clone_list[] = {
	{ "Realtek RTL8029",	0x10EC, 0x8029,	NE2K__REALTEK_FDX },
	{ "Winbond 89C940",	0x1050, 0x0940, 0 },
	{ "Compex RL2000",	0x11F6, 0x1401, 0 },
	{ "KTI ET32P2",		0x8E2E, 0x3000, 0 },
	{ "NetVin NV5000SC",	0x4A14, 0x5000, 0 },
	{ "VIA 86C926",		0x1106, 0x0926, NE2K__16BIT_IO_ONLY },
	{ "SureCom NE34",	0x10BD, 0x0E34,	0 },
	{ "WinBond W89C940F",	0x1050, 0x5A5A, 0 },
	{ "Holtek HT80232",	0x12C3, 0x0058, NE2K__16BIT_IO_ONLY | NE2K__HOLTEK_FDX },
	{ "Holtek HT80229",	0x12C3, 0x5598, NE2K__32BIT_IO_ONLY | NE2K__HOLTEK_FDX | NE2K__STOP_PG_0x60 },
	{ 0,0,0,0 }
};

/****************************************************************************\
 * Driver Entry Points
\****************************************************************************/

const char **publish_devices(void) {
	int	i, nd;
	char	devName[64];

	ETHER_DEBUG( gDevID, FUNCTION, "publish_devices ENTRY\n");

	/* Since PCMCIA cards are around, we have to make this publish_devices routine as 'dynamic' 
	 * as possible and recalculate the names every time this function is called.  */

	for (nd = i = 0; nd < MAX_CARDS; nd++) {
		if ( (gDev[nd].flags & NE2K__BUS_MSK) == 0 ) {
			continue;	// Bus type not set => no card present
		}
		sprintf(devName, "%s%d", kDevDir, i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
		i += 1;
	}
	gDevNameList[i] = NULL;

	ETHER_DEBUG( gDevID, FUNCTION, "publish_devices EXIT\n");

	return (const char **)gDevNameList;
}

status_t init_hardware(void) {
	return B_NO_ERROR;
}

status_t init_driver() {
	status_t 		PCIStatus, ISAStatus;
	int			bus_type = 0;
	uint32			PCIentries = 0;
	uint32			ISAentries = 0;
	int32			i,nd;
	char			devName[64];

	ETHER_DEBUG(gDevID, FUNCTION, "init_driver ENTRY\n");

	load_driver_symbols( "ne2k" );

	/* Initialize all the global debug flags */
	for (i=0; i < MAX_CARDS; i++) {
		gDev[i].cardType = 0;
		gDev[i].devInfo = NULL;
		gDev[i].devLink = NULL;
		gDev[i].flags = 0;
		gDev[i].debug = DEFAULT_DEBUG_FLAGS;
	}
	
	/* This function will probably have to follow a formula as follows:
		First search for a PCI card,
		then search for a PNP ISA card.
		search for a plain ISA card
		and finally set up free slots for PCMCIA
	*/

	/* First grab the two structures we need (one for the PCI and one for the ISA bus ) */
	if((PCIStatus = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		ETHER_DEBUG(gDevID, ERR, "Get PCI module failed! %s\n", strerror(PCIStatus));
	}
	else if (PCIStatus == B_OK) {
		bus_type |= NE2K__BUS_PCI;
	}

	/* Apparently, if this is a non-PnP card, then this following line is all we need to do
	   here in this function... */
	if((ISAStatus = get_module( B_ISA_MODULE_NAME, (module_info **)&gISAModInfo )) != B_OK) {
		ETHER_DEBUG(gDevID, ERR, "Get ISA module failed! %s\n", strerror(ISAStatus));
	}
	else if (ISAStatus == B_OK) {
		bus_type |= NE2K__BUS_ISA;
	}

	if (!bus_type) {
		ETHER_DEBUG(gDevID, ERR, "Did I miss the bus?!...(No bus modules were found)\n");
		return B_ERROR;
	}

	/* From here, we ONLY look at either PCI or ISA (or cardbus...later). */
	if ( bus_type & NE2K__BUS_PCI ) {
		/* Find Lan cards on the PCI bus */
		if ((PCIentries = get_pci_list(gDev, MAX_CARDS)) == 0) {
			ETHER_DEBUG(gDevID, INFO, " not found on PCI bus\n");
			free_pci_list(gDev);
			put_module(B_PCI_MODULE_NAME);
		}
		ETHER_DEBUG(gDevID, INFO, "%ld entries found on the PCI bus\n", PCIentries);
	}
	if ( bus_type & NE2K__BUS_ISA ) {
		/* Find Lan cards on the ISA bus */
		if ((ISAentries = get_isa_list(gDev, MAX_CARDS, PCIentries)) == 0) {
			ETHER_DEBUG(gDevID, INFO, kDevName " not found on ISA bus\n");
			/* No need to free any isa lists...no memory taken */
			put_module(B_ISA_MODULE_NAME);
		}
		ETHER_DEBUG(gDevID, INFO, "%ld entries found on the ISA bus\n", ISAentries);
	}

	/* Initialize the PCMCIA driver modules */
	if ( ne2k_pcmcia_init_driver() != B_OK ) {
		ETHER_DEBUG(gDevID, INFO, "Unable to configure PCMCIA bus\n");
	}

	/* Returning an error when gDevCount==0 unloads the driver, then
	   the system will crash when a card is inserted and card services calls back
		the unloaded ne2k_pcmcia_attach() that was registered by ne2k_pcmcia_init_driver(). */
	if ((gDevCount) == 0) {
		ETHER_DEBUG(gDevID, ERR, " not found on PCI or ISA bus\n");
//		return B_ERROR;
	}

	/* At this point, all the IRQs, DMAs, and such should be set up in ISA and PCMCIA buses */

	/* Set up the device name list for all _present_ devices */
	for (nd = i = 0; nd < MAX_CARDS; nd++) {
		if ( (gDev[nd].flags & NE2K__BUS_MSK) == 0 ) {
			continue;	// Bus type not set => no card present
		}
		sprintf(devName, "%s%ld", kDevDir, i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
		i += 1;
	}
	gDevNameList[i] = NULL;

	ETHER_DEBUG(gDevID, INFO, "Init Ok. Found %ld devices.\n", gDevCount );

	ETHER_DEBUG(gDevID, FUNCTION, ": init_driver EXIT\n");

	return B_OK;
}

void uninit_driver(void) {
	int32 	i;
	void 	*item;

	ETHER_DEBUG(gDevID, FUNCTION, "uninit_driver ENTRY\n");

	/* Free device name list */
	for (i=0; (item = gDevNameList[i]); i++)
		free(item);
	
	/* Free device list (only necessary for the PCI bus) */
	free_pci_list(gDev);

	put_module(B_PCI_MODULE_NAME);

	ne2k_pcmcia_uninit_driver();

	ETHER_DEBUG(gDevID, FUNCTION, "uninit_driver EXIT\n");
}

device_hooks *find_device(const char *name) {
	int32 	i;
	char 	*item;

	ETHER_DEBUG(gDevID, FUNCTION, "find_device %s\n", name);

	/* Find device name */
	for (i=0; (item=gDevNameList[i]); i++) {
		if (strcmp(name, item) == 0) {
			return &gDeviceHooks;
		}
	}
	ETHER_DEBUG(gDevID, FUNCTION, "find_device - Device not found\n");
	return NULL; /*Device not found */
}

/* Open/Initialize the NIC */
static status_t open_hook(const char *name, uint32 flags, void **cookie) {
	int32 			devID;
	int32 			mask;
	status_t 		status = 0;
	char*			devName;
	device_info_t*		device;

	(void)flags;

	ETHER_DEBUG(gDevID, FUNCTION, "open_hook ENTRY\n");

	for (devID = 0; (devName = gDevNameList[devID]); devID++) {
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;
		
	mask = 1 << devID; /* Check if device is busy and set in-use flag if not */
	if (atomic_or(&gOpenMask, mask) &mask) {
		ETHER_DEBUG(gDevID, ERR, "Device Is Busy\n");
		return B_BUSY;
	}
	
	/* Allocate the storage space for per-driver instance global data */
	if (!(*cookie = device = (device_info_t *)malloc(sizeof(device_info_t)))) {
		ETHER_DEBUG(gDevID, ERR, "No Memory for instance global data!\n");
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(device_info_t));

	/* Initialize any necessary variables to be safe */
	device->devID = devID;

	ETHER_DEBUG(device->devID, INFO, "device=%p\n", device);
	
#if DEBUGGER_COMMAND
	gDev[devID].devInfo = device;	/* We only need to hook this pointer for the debugger command */
	add_debugger_command(kDevName, ne2k, "ne2k Driver Info");
#endif

	/* Allocate any needed resources for the global driver object */
	if (allocate_resources(device) != B_OK) {
		ETHER_DEBUG(device->devID, ERR, "Error allocating resources!\n");
		goto err1;
	}

	/* enable access to the card's address space (and determine whether or not the card is isa/pci/etc... */
	if ((status = enable_addressing(device)) !=  B_OK)
		goto err1;
	
	if ( gDev[device->devID].flags & NE2K__BUS_PCI) {
		ETHER_DEBUG(device->devID, INFO, "setting instance-specific bus type to PCI\n");
		// Set global module info to PCI type
		gModInfo = (pci_module_info*)gPCIModInfo;
	} else { // Since ISA ~= PCMCIA
		ETHER_DEBUG(device->devID, INFO, "setting instance-specific bus type to ISA\n");
		// Set global module info to ISA type  (what happens when there is a PCI *AND* ISA card present?!)
		// Do the write_io_* routines all work the same in ISA and PCI buses?
		gModInfo = (pci_module_info*)gISAModInfo;
	}

	if ((status = probe_ne2000(device)) != B_OK) {
		ETHER_DEBUG(device->devID, ERR, "probe error - no cards found!\n");
		goto err1;
	}

	// Turn on full duplex, if permitted
	if ( gDev[device->devID].flags & NE2K__REALTEK_FDX ) {
		ETHER_DEBUG( device->devID, INFO, "Enabling Realtek Full Duplexing\n" );
		write8( device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P3 );
		write8( device->base + 0x20, read8( device->base + 0x20 ) | 0xC0 );
	} else if ( gDev[device->devID].flags & NE2K__HOLTEK_FDX ) {
		ETHER_DEBUG( device->devID, INFO, "Enabling Holtek Full Duplexing\n" );
		write8( device->base + 0x20, read8( device->base + 0x20 ) | 0xC0 );
	}

	/* Perform the common NE2000 initialization process */
	if ((status = init_ne2k(device)) != B_OK) {
		ETHER_DEBUG(device->devID, ERR, "error during natsemi-defined initialization\n");
		goto err1;
	}

	/* Install cleanup kernel thread */
	device->cleanup_thread_id = spawn_kernel_thread(ne2k_cleanup_thread, "ne2k_cleanup_thread", B_NORMAL_PRIORITY, (void *)device);
	resume_thread(device->cleanup_thread_id);

	/* Setup interrupts */
	install_io_interrupt_handler(gDev[device->devID].irq, interrupt_hook, *cookie, 0 );

	gDev[device->devID].flags |= NE2K__CARD_OPENED;
	
	ETHER_DEBUG(device->devID, INFO, "open ok \n");

	ETHER_DEBUG(device->devID, FUNCTION, "open_hook EXIT \n");
	
	return B_OK;
	
err1:
	free_resources(device);
	free(device);	
	
err0:
	atomic_and(&gOpenMask, ~mask);
	ETHER_DEBUG(gDevID, ERR, "open failed!\n");
	return status;	
}

static status_t init_ne2k(device_info_t *device) {
	int i;

	ETHER_DEBUG(device->devID, FUNCTION, "init_ne2k ENTRY\n");

	/* INIT SEQUENCE : (from NS datasheet) */
	/* 1.Program CR for Page 0 */
	write8(device->base + NE2K_CR, NE2K_CR_PS_P0 | NE2K_CR_STOP | NE2K_CR_RD_ARD);
	
	/* 2.Initialize DCR */
	write8(device->base + NE2K0_DCR_W, NE2K0_DCR_WTS_WORD | NE2K0_DCR_BOS_LEND | 
						NE2K0_DCR_LAS_D16B | NE2K0_DCR_LS_LOOPB | 
						NE2K0_DCR_ARM_SCNE | NE2K0_DCR_FTS_2WD);
	
	/* 3.Clear RBCR0, RBCR1 (i.e. Init the Byte Count Registers */
	write8(device->base + NE2K0_RBCR0_W, 0);
	write8(device->base + NE2K0_RBCR1_W, 0);

	/* 4.Initialize RCR (Receive Configuration Register) */
	write8(device->base + NE2K0_RCR_W, NE2K0_RCR_SEP_RER | NE2K0_RCR_AR_F64R |
						NE2K0_RCR_AB_BCA  | NE2K0_RCR_AM_MCNC |
						NE2K0_RCR_PRO_OFF | NE2K0_RCR_MON_BUF);

	/* 5.Place ST-NIC in LOOPBACK mode 1 or 2 (mode 1 in this case) */
	write8(device->base + NE2K0_TCR_W, NE2K0_TCR_LB_INNLB);

	/* 6.Initialize RX and TX Buffer Rings (BNDRY, PSTART, PSTOP) */
	write8(device->base + NE2K0_TPSR_W, device->tx_start_page);
	write8(device->base + NE2K0_PSTART_W, device->rx_start_page);
	write8(device->base + NE2K0_PSTOP_W, device->rx_stop_page);
	write8(device->base + NE2K0_BNRY_W, device->rx_stop_page - 1);
	device->rx_current_page = device->rx_start_page;

	ETHER_DEBUG(device->devID, INFO, "tx_start_page is %2.2x, rx_start_page is %2.2x, rx_stop_page is %2.2x\n", 
		device->tx_start_page, device->rx_start_page, device->rx_stop_page);

	/* 7.Clear ISR by writing 0x0FF to it */
	write8(device->base + NE2K0_ISR_W, 0xFF);
	
	/* 8.Initialize IMR */
	write8(device->base + NE2K0_IMR_W, 0x00);

	/* 9.Program CR for Page 1, Init PAR0-5, MAR0-5, CURR */
	write8(device->base + NE2K_CR, NE2K_CR_PS_P1 | NE2K_CR_STOP | NE2K_CR_RD_ARD);
	for (i = 0; i < NE2K__ETHER_ADDR_LEN; i++) {
		write8(device->base + NE2K1_PAR0_W + i, device->mac_address.ebyte[i]);
	}
	for (i = 0; i < NE2K__ETHER_ADDR_LEN; i++) {
		write8(device->base + NE2K1_MAR0_W + i, 0xFF);
	}
	write8(device->base + NE2K1_CURR_W, device->rx_start_page);

	write8(device->base + NE2K_CR, NE2K_CR_PS_P0 | NE2K_CR_RD_ARD);
	/* Clear interrupts again (just in case) and enable the mask */
	write8(device->base + NE2K0_ISR_W, 0xFF);
	write8(device->base + NE2K0_IMR_W, NE2K0_IMR_ALL);

	/* 10.Put ST-NIC in START mode (and back to page 0) */
	write8(device->base + NE2K_CR, NE2K_CR_PS_P0 | NE2K_CR_START | NE2K_CR_RD_ARD);
	
	/* 11.Initialize TCR */
	write8(device->base + NE2K0_TCR_W, NE2K0_TCR_CRC_APP | NE2K0_TCR_LB_NORM | 
									   NE2K0_TCR_ATD_OFF | NE2K0_TCR_OFST_NORM);
	write8(device->base + NE2K0_RCR_W, NE2K0_RCR_AB_BCA);

	/* END INIT SEQUENCE (from NS datasheet) */
	ETHER_DEBUG(device->devID, FUNCTION, "init_ne2k EXIT\n");

	return B_OK;
}

static int32 get_pci_list(struct global_dev_info info[], int32 maxEntries) {
	status_t	status;
	int32		i = 0, entries = 0;
	int32		foundID;
	pci_info	*item;

	ETHER_DEBUG(gDevID, FUNCTION, "get_pci_list ENTRY\n");
	
	item = (pci_info *)malloc(sizeof(pci_info));
	
	for (i=0, entries=0; entries<maxEntries; i++) {
		if ((status = gPCIModInfo->get_nth_pci_info(i, item)) != B_OK)
			break;

		for ( foundID=0; foundID < NE2K__CHIPSET_COUNT; foundID += 1 ) {
			if (( item->vendor_id == pci_clone_list[foundID].dev_vendor ) &&
				( item->device_id == pci_clone_list[foundID].dev_device ))
				break;
		}

		if ( foundID == NE2K__CHIPSET_COUNT ) // Device didn't match
			continue;

		/* check if the device really has an IRQ */
		if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
			ETHER_DEBUG(gDevID, ERR, "found with invalid IRQ - check IRQ assignment\n");
			continue;
		}
		ETHER_DEBUG( gDevID, INFO, "%s devID# %lu found at IRQ %x \n", pci_clone_list[foundID].name, 
			entries, item->u.h0.interrupt_line);
		info[entries].cardType = foundID;
		info[entries].port = 0;			/* Not necessary for PCI */
		info[entries].flags |= pci_clone_list[foundID].dev_flags;
		info[entries].flags |= NE2K__BUS_PCI;
		info[entries].pciInfo = item;
		info[entries].irq = info[entries].pciInfo->u.h0.interrupt_line;
		entries +=1 ;
		gDevCount += 1;
		item = (pci_info *)malloc(sizeof(pci_info));
	}

	info[entries].pciInfo = NULL;
	free(item);

	ETHER_DEBUG(gDevID, FUNCTION, "get_pci_list EXIT\n");
	return entries;
}

static status_t free_pci_list(struct global_dev_info info[]) {
	int32 		i;
	pci_info	*item;

	ETHER_DEBUG(gDevID, FUNCTION, "free_pci_list ENTRY\n");
	
	for (i=0; (item=info[i].pciInfo); i++) {
		if (item) free(item);
	}

	ETHER_DEBUG(gDevID, FUNCTION, "free_pci_list EXIT\n");
	return B_OK;
}

static int32 get_isa_list(struct global_dev_info info[], int32 maxEntries, int32 curEntries) {
	status_t	status;
	int32 		i;
	uint64 		cookie = 0;
	int32 		entries = 0;
	int32		temp_entries = 0;
	uint32		pid1, pid2, pid3;
	struct isa_device_info isaInfo;
	config_manager_for_driver_module_info *cfg_mgr;

	ETHER_DEBUG(gDevID, FUNCTION, "get_isa_list ENTRY\n");

	status = get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&cfg_mgr);

	if ( status == B_OK ) {
		MAKE_EISA_PRODUCT_ID((EISA_PRODUCT_ID *)&pid1, 'A', 'X', 'E', 0x220, 1);	
		MAKE_EISA_PRODUCT_ID((EISA_PRODUCT_ID *)&pid2, 'D', 'L', 'K', 0x220, 1);
		MAKE_EISA_PRODUCT_ID((EISA_PRODUCT_ID *)&pid3, 'A', 'N', 'X', 0x310, 1);

		/* start the entries at curEntries since the PCI scan is done prior to this function */
		for (i = 0, entries=curEntries; entries < maxEntries; i++) {
			struct device_configuration *config;
			resource_descriptor resourceDesc;
			status_t size;

			if (cfg_mgr->get_next_device_info(B_ISA_BUS, &cookie, &isaInfo.info, sizeof(isaInfo)) != B_OK)
				break;

			/* match the vendor and device id */
			if ((!EQUAL_EISA_PRODUCT_ID((EISA_PRODUCT_ID)pid1, (EISA_PRODUCT_ID)isaInfo.vendor_id)))
				 if (!EQUAL_EISA_PRODUCT_ID((EISA_PRODUCT_ID)pid2, (EISA_PRODUCT_ID)isaInfo.vendor_id))
					 if (!EQUAL_EISA_PRODUCT_ID((EISA_PRODUCT_ID)pid3, (EISA_PRODUCT_ID)isaInfo.vendor_id))
						 if (!(0x6000d041 == isaInfo.vendor_id))	/* realtek 8019AS */
							continue;

			/* found a card */
			size = cfg_mgr->get_size_of_current_configuration_for(cookie);
			if (size < 0) {
				ETHER_DEBUG(gDevID, ERR, "Error getting size of configuration %lx\n", size);
				continue;
			}

			config = malloc(size);
			if (!config) {
				ETHER_DEBUG(gDevID, ERR, "Out of memory\n");
				break;
			}

			status = cfg_mgr->get_current_configuration_for(cookie, config, size);
			if (status < B_OK) {
				ETHER_DEBUG(gDevID, ERR, "Error getting current configuration\n");
				free(config);
				continue;
			}

			/* sanity check card */
			if ((cfg_mgr->count_resource_descriptors_of_type(config, B_IRQ_RESOURCE) != 1) ||
					(cfg_mgr->count_resource_descriptors_of_type(config, B_DMA_RESOURCE) != 0) ||
					(cfg_mgr->count_resource_descriptors_of_type(config, B_IO_PORT_RESOURCE) != 1) ||
					(cfg_mgr->count_resource_descriptors_of_type(config, B_MEMORY_RESOURCE) != 0)) {
				ETHER_DEBUG(gDevID, ERR, "Bad resource counts\n");
				free(config);
				continue;
			}

			status = cfg_mgr->get_nth_resource_descriptor_of_type(config, 0, B_IRQ_RESOURCE, &resourceDesc, sizeof(resourceDesc));
			if (status < 0) {
				ETHER_DEBUG(gDevID, ERR, "Error getting IRQ %lx\n", status);
				free(config);
				continue;
			}

			gDev[entries].irq = log2(resourceDesc.d.m.mask);
			
			status = cfg_mgr->get_nth_resource_descriptor_of_type(config, 0, B_IO_PORT_RESOURCE, &resourceDesc, sizeof(resourceDesc));
			free(config);
			if (status < 0) {
				ETHER_DEBUG(gDevID, ERR, "Error getting I/O range %lx\n", status);
				continue;
			}

			if (resourceDesc.d.r.len != 0x20) {
				ETHER_DEBUG(gDevID, ERR, "I/O range must be 0x20 not %lx\n", resourceDesc.d.r.len);
				continue;
			}

			gDev[entries].port = resourceDesc.d.r.minbase;
			gDev[entries].flags |= NE2K__BUS_ISA;
			ETHER_DEBUG(gDevID, INFO, "device %d found at irq %d port %x\n", (int)entries, (int)gDev[curEntries].irq, (int)gDev[curEntries].port);
			entries++;
			gDevCount += 1;
		}
		put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
	} else {
		ETHER_DEBUG(gDevID, ERR, "Get config mgr module failed %s\n",strerror(status ));
	}

	/* Load any settings from the kernel settings file...if none are found
	   or if the maximum was already detected, then entries should remain 
	   the same... */
	temp_entries = load_ne2k_settings(info, maxEntries, entries);
	
	entries += temp_entries - curEntries;

	ETHER_DEBUG(gDevID, FUNCTION, "get_isa_list EXIT\n");

	return entries;
}

static status_t enable_addressing(device_info_t *device) {
	ETHER_DEBUG(device->devID, FUNCTION, "enable_addressing ENTRY\n");

	if ( gDev[device->devID].flags & NE2K__BUS_PCI ) {
			/* Turn on I/O port decode, Memory Address Decode, and Bus Mastering */
		gPCIModInfo->write_pci_config(gDev[device->devID].pciInfo->bus, gDev[device->devID].pciInfo->device, 
			gDev[device->devID].pciInfo->function, PCI_command, 2,
			PCI_command_io | PCI_command_memory | PCI_command_master |
			gPCIModInfo->read_pci_config( gDev[device->devID].pciInfo->bus, gDev[device->devID].pciInfo->device, 
			gDev[device->devID].pciInfo->function, PCI_command, 2));

		device->base = gDev[device->devID].pciInfo->u.h0.base_registers[0];
	} else {
		/* This works for both ISA, ISAPnP, and PCMCIA the same */
		device->base = gDev[device->devID].port;
	}

	ETHER_DEBUG(device->devID, INFO, "base=%lx\n", device->base);

	ETHER_DEBUG(device->devID, FUNCTION, "enable_addressing EXIT\n");

	return B_OK;
}


/* The new Linksys and netgear 10/100 cards have the mac address
   in a different eeprom location.
*/
uint32 get_np100_mac_addr(device_info_t *data)
{
	int32 i;
	uint8 sum;

	for (sum = 0, i = 0x14; i < 0x1c; i++)
		sum += read8(data->base + i);
	if (sum != 0xff) {
#if 0
		dprintf("get_mac_addr() failed sum=%x ");
		for (i=0;i<0x1c;i++)
			dprintf("%2.2x%c",read8(data->base + i), ((i&0xf)==0) ? '\n' : ' ');
#endif
		return 0;
	}
	for (i = 0; i < 6; i++)
		data->mac_address.ebyte[i] = read8(data->base + 0x14 +i );
	return true;

#if 0
	dprintf("NE2k: get_np100_mac_addr got ");
	for (i=0; i<6; i++)
			dprintf("%2.2x%c",data->mac_address.ebyte[i], (i==5) ? '\n' : ' ');
#endif

}

/*
 * Determine if we have an ne2000 card
 */
static int32 probe_ne2000(device_info_t *device)
{
	int32 			i = 0;
	uint32 			j = 0;
	int 			reg = 0;
	uint8 			SA_prom[32];			/* Station Address PROM storage */
	bool sa_prom_doubled = true;

	ETHER_DEBUG(device->devID, FUNCTION, "probe_ne2000 ENTRY\n");

	device->buffer_start = NE2K__NE2000_BUFFER_START;
	device->buffer_length = NE2K__NE2000_BUFFER_LENGTH;

	/*   -First calculate the logical page start and number of pages */
	device->tx_start_page = ((device->buffer_start + device->buffer_length) >> 8) - NE2K__TX_PAGES;
	device->num_pages = device->buffer_length >> 8;
	/*   -Then calculate the first and last receive page */
	device->rx_start_page = device->buffer_start >> 8;
	device->rx_stop_page = (device->rx_start_page + device->num_pages) - NE2K__TX_PAGES;
	/*   -And finally calculate the actual buffer start and its size - seems useless */
	device->ring_buf_start = (device->rx_start_page << 8);
	device->ring_buf_size = ((device->num_pages - NE2K__TX_PAGES) << 8);

	/* Reset the chip */
	reg = read8(device->base + NE2K_RESET);
	snooze(2000);
	write8(device->base + NE2K_RESET, reg);
	snooze(2000);
	write8(device->base + NE2K_CR, NE2K_CR_STOP | NE2K_CR_RD_ARD | NE2K_CR_PS_P0);

	i = 10000;
	while ((read8(device->base + NE2K0_ISR_R) & NE2K0_ISR_RST) == 0 && i-- > 0);
	if (i < 0) {
		ETHER_DEBUG(device->devID, INFO, "Soft Reset Failed -- Ignoring\n");
	}

	write8(device->base + NE2K0_ISR_W, 0xff);
	write8(device->base + NE2K0_DCR_W, NE2K0_DCR_LAS_D16B);
	write8(device->base + NE2K_CR, NE2K_CR_STOP | NE2K_CR_RD_ARD | NE2K_CR_PS_P0);
	snooze(2000);
	reg = read8(device->base + NE2K_CR);
	if (reg != (NE2K_CR_STOP | NE2K_CR_RD_ARD | NE2K_CR_PS_P0)) {
		ETHER_DEBUG(device->devID, ERR, "Set Command Register FAILED!: %02x != %02x\n", reg, NE2K_CR_RD_ARD | NE2K_CR_STOP); 
		return B_ERROR;
	}

	write8(device->base + NE2K0_TCR_W, 0);
	write8(device->base + NE2K0_RCR_W, NE2K0_RCR_MON_NBUF);
	write8(device->base + NE2K0_PSTART_W, device->rx_start_page);
	write8(device->base + NE2K0_PSTOP_W, device->rx_stop_page);
	write8(device->base + NE2K0_BNRY_W, device->rx_stop_page-1);
	write8(device->base + NE2K0_IMR_W, 0);
	write8(device->base + NE2K0_ISR_W, 0);
	write8(device->base + NE2K_CR, NE2K_CR_STOP | NE2K_CR_RD_ARD | NE2K_CR_PS_P1); 
	write8(device->base + NE2K1_CURR_W, device->rx_start_page);
	write8(device->base + NE2K_CR, NE2K_CR_STOP | NE2K_CR_RD_ARD | NE2K_CR_PS_P0);

	/* stop chip */
	write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P0 | NE2K_CR_STOP);

	/* Get the mac address from the card */
	write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P0 | NE2K_CR_START); /* Select page 0*/
	write8(device->base + NE2K0_RBCR0_W, sizeof(SA_prom));	/* Set RBCRLow to 32 since we're downloading the PROM */
	write8(device->base + NE2K0_RBCR1_W, 0x00); 			/* Set RBCRHi */
	write8(device->base + NE2K0_RSAR0_W, 0x00);				/* DMA starting at 0x0000. */
	write8(device->base + NE2K0_RSAR1_W, 0x00);
	write8(device->base + NE2K_CR, NE2K_CR_RD_RR | NE2K_CR_START);

	/* Some broken PCI cards don't respect the byte-wide request in the program 
	 * seq above, and hence don't have doubled up values. */
	for(j = 0; j < sizeof(SA_prom); j+=2) {
		SA_prom[j] = read8(device->base + NE2K_DATA);
		SA_prom[j+1] = read8(device->base + NE2K_DATA);
		if (SA_prom[j] != SA_prom[j+1])
			sa_prom_doubled = false;
	}

	if (sa_prom_doubled) {
		for (j = 0; j < 16; j++) {
			SA_prom[j] = SA_prom[j+j];
		}
	}

	if (get_np100_mac_addr(device) == true)	{}
	/* else if (get_other_mac_addr(device) == true) {}  */	/* there are others */
	else {
		for(j = 0; j < NE2K__ETHER_ADDR_LEN; j++) {			/* default */
			device->mac_address.ebyte[j] = SA_prom[j];
		}
	}

	/* We want this line visible regardless of the debug level */
	if ( gDev[device->devID].flags & NE2K__BUS_PCI ) {
		dprintf( "ne2k: %s found at irq %d with address - ", 
			pci_clone_list[gDev[device->devID].cardType].name, gDev[device->devID].irq );
	} else {
		dprintf( "ne2k: ISA/PCMCIA card found at irq %d with address - ", gDev[device->devID].irq );
	}

	print_address(device);

	/*
	 * just get rid of Sony braindead designers...
	 * if the MAC address happens to be the bogus one
	 * that a lot of Sony prototypes share... just
	 * choose a random one... chances of address collision
	 * are way smaller than using the Sony specified one
	 *
	 * [ we just hack the data->myaddr as this address
	 *   is later programmed into the NIC
	 * ]
	 */
	{
		unsigned char broken_sony_MAC[]= { 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff };
		if(memcmp(device->mac_address.ebyte, broken_sony_MAC, 6)== 0) {
			int seed;
			int i;
			int ps;

			dprintf("Oh man! Looks like you have a broken Sony box...\n");
			dprintf("Hacking a new MAC address for you.\n");

			/*
			 * prepare a semi random seed by reading the cmos clock & other garbage (not SMP safe)
			 */
			ps= disable_interrupts();
				seed= 0;
				for(i= 0; i< 10; i++){
					int top= (seed>>28)&0x0f;

					write8(0x70, i);
					seed<<= 4;
					seed^= top*top/3;
					seed^= read8(0x71);
				}
			restore_interrupts(ps);
			seed^= system_time();
			srand(seed);

			/*
			 * now get a new address
			 */
			for(i= 0; i< 6; i++) {
				device->mac_address.ebyte[i]= rand()%0xFF;
			}

			/*
			 * strip off some bits for making sure we don't get a
			 * mcast address.
			 */
			device->mac_address.ebyte[0]= 0x00;
			dprintf("ne2000 ethernet card hacked with address - ");
			print_address(device);
		}
	}

	ETHER_DEBUG(device->devID, FUNCTION, "probe_ne2000 EXIT\n");
	return B_OK;
}

/* Load the settings from the kernel settings file */
static int32 load_ne2k_settings(struct global_dev_info info[], int32 maxEntries, int32 curEntries) {
	void *handle;
	const driver_settings *settings;
	uint8 found = 0;
	int entries = 0;
	int i = 0;
	int j = 0;
	
	ETHER_DEBUG(gDevID, FUNCTION, "load_ne2k_settings ENTRY with %ld entries\n", curEntries);

	handle = load_driver_settings(kDevName);
	settings = get_driver_settings(handle);
	if (settings) {
		entries = settings->parameter_count;
		/* Basically, if we exceed the max. number of cards, we don't care
		   about any of the other entries in the settings file */
		for (i=0;((i<entries) && ((curEntries + i) <= maxEntries));i++) {
			driver_parameter *parameter = settings->parameters + i;

			/* Unless there's some sort of crazy PCI or CARDBUS device that requires you
			   to manually specify port/irq/etc, this should be fine */
			ETHER_DEBUG(gDevID, INFO, "loading kernel settings for device (%ld)\n", (curEntries + i));
			info[curEntries + i].flags |= NE2K__BUS_ISA;
			info[curEntries + i].irq = 0;
			info[curEntries + i].port = 0;

			for (j=0; j < parameter->parameter_count; j++) {
				driver_parameter *temp_param = parameter->parameters + j;

				if (strcmp(temp_param->name, "irq") == 0) {
					/* Irq numbers are typically in decimal...aren't they?  I don't
					   remember ever having to enter an irq of E before... */
					info[curEntries + i].irq = strtoll(temp_param->values[0], NULL, 10);
					ETHER_DEBUG(gDevID, INFO, " -> forcing irq to 0x%x\n", info[curEntries + i].irq);
					found |= 0x1;
				} 
				else if( strcmp(temp_param->name, "port") == 0) {
					/* Ports however, are always hex... */
					info[curEntries + i].port = strtoll(temp_param->values[0], NULL, 16);
					ETHER_DEBUG(gDevID, INFO, " -> forcing port to 0x%x\n", info[curEntries+i].port);
					found |= 0x2;
				}
			}
			gDevCount += 1;		/* Another device was successfully added */
		}
	}
	unload_driver_settings(handle);
	if ((found == 1) || (found == 2)) ETHER_DEBUG(gDevID, ERR, "must specify port AND irq\n");

	if (found == 0x3) {
		if (i<entries) {
			ETHER_DEBUG(gDevID, INFO, "Too many cards were specified in kernel settings file!\n");
		}
		return (i);
	} else {
		return 0;
	}

	ETHER_DEBUG(gDevID, FUNCTION, "load_ne2k_settings EXIT\n");
}

static uchar log2(uint32 mask) {
  uchar value;

  mask >>= 1;
  for (value = 0; mask; value++)
	mask >>= 1;

  return value;
}

/* Print the ethernet address */
static void print_address(device_info_t *device) {
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", device->mac_address.ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", device->mac_address.ebyte[5]);
	dprintf("%s\n", buf);
}

/* Allocate and initialize fc_locks, semaphores, and spinlocks. */
static status_t allocate_resources(device_info_t *device) {
	status_t result = B_OK;

	/* Setup Semaphores */
	if((result = create_fc_lock( &device->fc_rx_lock, 0, kDevName " rx" )) < 0 ) {
		dprintf(kDevName " create fc_rx_lock failed %lx \n", result);
		return (result);
	}

	/* intialize tx semaphore with 2 spaces for the double buffering */
	if ((device->sem_tx_lock = create_sem(2, kDevName " tx" )) < 0 ) {
		delete_fc_lock(&device->fc_rx_lock);
		dprintf(kDevName " create sem_tx_lock failed %x \n", (int)result);
		return (B_ERROR);
	}

	/* Initialize reset semaphore (for reset triggering) */
	if((device->sem_reset_lock = create_sem(0, kDevName " reset" )) < 0 ) {
		delete_fc_lock(&device->fc_rx_lock);
		delete_sem(device->sem_tx_lock);
		dprintf(kDevName " create sem_reset_lock failed %x \n", (int)result);
		return (B_ERROR);
	}

	/* intialize hardware semaphore */
	if ((device->sem_hw_lock = create_sem(1, kDevName " hw" )) < B_OK ) {
		delete_fc_lock(&device->fc_rx_lock);
		delete_sem(device->sem_tx_lock);
		delete_sem(device->sem_reset_lock);
		dprintf(kDevName " create sem_hw_lock failed %lx \n", result);
		return (B_ERROR);
	}

	/* intialize rdma semaphore */
	if ((device->sem_rdma_done = create_sem(0, kDevName " rdma" )) < B_OK ) {
		delete_fc_lock(&device->fc_rx_lock);
		delete_sem(device->sem_tx_lock);
		delete_sem(device->sem_reset_lock);
		delete_sem(device->sem_hw_lock);
		dprintf(kDevName " create sem_rdma_lock failed %lx \n", result);
		return (B_ERROR);
	}

	device->spin_page_lock = 0;

	return B_OK;
}

/* Free semaphores and fc_locks */
static void free_resources(device_info_t *device) {
	status_t rc;

	delete_fc_lock(&device->fc_rx_lock);
	delete_sem(device->sem_tx_lock);
	delete_sem(device->sem_reset_lock);
	delete_sem(device->sem_hw_lock);
	delete_sem(device->sem_rdma_done);

	/* the cleanup thread must be dead before the driver is unloaded */
	wait_for_thread(device->cleanup_thread_id, &rc);
}

/* Display packet to debugger */
static void dump_packet( const char * msg, uint8 * buf, uint16 size) {
	uint16 j;
	
	dprintf("%s dumping %x size %d \n", msg, (int)buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
	dprintf("\n");
}

/* Set the boundary register, both on the card and internally
 * NOTE: you cannot make the boundary = current register on the card,
 * so we actually set it one behind.
 */
static void ne2k_set_boundary(device_info_t *device, uint8 next_page) {
	cpu_status	cpu;

	cpu = disable_interrupts();
	acquire_spinlock( &device->spin_page_lock );

	if (next_page != device->rx_start_page) {
		write8(device->base + NE2K0_BNRY_W, next_page - 1);
	} else {
		/* since it's a ring buffer */
		write8(device->base + NE2K0_BNRY_W, device->rx_stop_page - 1);
	}
	device->rx_current_page = next_page;

	release_spinlock( &device->spin_page_lock );
	restore_interrupts( cpu );
}

/* 
 * ne2k_copy_packets()
 * Copy a packet from the ring buffer to the specified memory buffer 
 * Passing in a null buf pointer indicates that we are in cleanup mode and don't
 * care about saving the packets copied out of the card
 */
static int32 ne2k_copy_packets(device_info_t *device, uint8 *buf) {
	uint8			rxing_page, this_frame, next_frame;
	uint16			current_offset;
	int			retry_count = 1;
	int			rx_pkt_count = 0;
	int			num_rx_pages = device->rx_stop_page - device->rx_start_page;
	ne2k_packet_header	pkt_head;
	int			pkt_len = 0;
	cpu_status		cpu;

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_copy_packets ENTRY\n");

	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_page_lock);
	/* Get the rx page (incoming packet pointer). */
	write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P1);
	rxing_page = read8(device->base + NE2K1_CURR_R);
	write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P0);
	release_spinlock(&device->spin_page_lock);
	restore_interrupts(cpu);

	if ((rxing_page < device->rx_start_page) || (rxing_page >= device->rx_stop_page)) {
		ETHER_DEBUG(device->devID, ERR, "rxing_page out of range (%2.2x)\n", rxing_page);
		rxing_page = device->rx_start_page;
	}

	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_page_lock);
	/* Remove one frame from the ring.  Boundary is always a page behind. */
	this_frame = read8(device->base + NE2K0_BNRY_R) + 1;
	release_spinlock(&device->spin_page_lock);
	restore_interrupts(cpu);

	if (this_frame >= device->rx_stop_page) {
		this_frame = device->rx_start_page;
	}

	/* Someday we'll omit the previous, iff we never get this message.
	   (There is at least one clone claimed to have a problem.)  */
	if ((gDev[device->devID].debug & ERR) && this_frame != device->rx_current_page) {
		ETHER_DEBUG(device->devID, ERR, "mismatched read page pointers %2x vs %2x.\n", 
			(int)this_frame, (int)device->rx_current_page);
	}

	if (this_frame == rxing_page)
		goto ncp_done;

	current_offset = this_frame << 8;


retry_get_packet:
	ne2k_min(device, (uint8 *)&pkt_head, current_offset, sizeof(ne2k_packet_header));

	pkt_len = pkt_head.count - sizeof(ne2k_packet_header);
	
	if ((pkt_head.next < device->rx_start_page) || (pkt_head.next >= device->rx_stop_page)) {
		ETHER_DEBUG(device->devID, ERR, "bad next packet -> pkt_head.next=%2.2x, rx_start_page=%2.2x, rx_stop_page=%2.2x\n", 
			pkt_head.next, device->rx_start_page, device->rx_stop_page);
		ETHER_DEBUG(device->devID, ERR, "tx_start_page=%2.2x\n", device->tx_start_page);
		ne2k_set_boundary(device, rxing_page);
		this_frame = device->rx_current_page;
#ifdef STAT_COUNTING
		device->stat_rx_bogus_errors++;
#endif
		goto ncp_done;
	}

	next_frame = this_frame + 1 + ((pkt_len+4)>>8);

	/* Check for bogus frame pointers that don't seem to point correctly within
	 * the receive ring buffer (it happens sometimes) */
	if (pkt_head.next != next_frame
		&& pkt_head.next != (next_frame + 1)
		&& pkt_head.next != (next_frame - num_rx_pages)
		&& pkt_head.next != (next_frame + 1 - num_rx_pages))
	{
		if (retry_count-- > 0) {
			/* Sometimes, retrying the read gets the 'correct' packet header
			 * in the case where the card is in a weird 'stuck' state.  This
			 * seems to fix the problem 100% of the time */
			ETHER_DEBUG(device->devID, INFO, "retrying packet header read\n");
			goto retry_get_packet;
		}

		/* But in case the 'fix' doesn't work ... */
		ETHER_DEBUG(device->devID, ERR, "bogus frame pointers -> next_frame=%2.2x pkt_head.next=%2.2x ", 
			next_frame, pkt_head.next);
		ETHER_DEBUG(device->devID, ERR, "this_frame=%2.2x, rxing_page=%2.2x\n", this_frame, rxing_page);
		ETHER_DEBUG(device->devID, ERR, "offending packet: status=%2.2x next=%2.2x size=%d(0x%x)\n", 
			pkt_head.status, pkt_head.next, pkt_head.count, pkt_head.count); 
		/* ...Just skip over all the received packets on the ring since we
		 * don't have a reliable next frame pointer mechanism */
		ne2k_set_boundary(device, rxing_page);
		this_frame = device->rx_current_page;
#ifdef STAT_COUNTING
		device->stat_rx_bogus_errors++;
#endif
		goto ncp_done;
	}

	if (!(pkt_head.status & NE2K0_RSR_PRX)) { /* Packet's not intact */
		ETHER_DEBUG(device->devID, (INFO | ERR), " packet not intact: status=%2.2x nxpg=%2.2x size=%d\n", 
			pkt_head.status, pkt_head.next, pkt_head.count); 

		if (pkt_head.status & NE2K0_RSR_FO) {
			/* Update the stats */
#ifdef STAT_COUNTING
			device->stat_rx_fifo_errors++;
#endif
		}
		pkt_len = 0;
		ne2k_set_boundary(device, pkt_head.next);
		this_frame = device->rx_current_page;
		goto ncp_done;
	}

	if (pkt_len < NE2K__MIN_FRAME_SIZE  ||  pkt_len > NE2K__MAX_FRAME_SIZE) {
		ETHER_DEBUG(device->devID, (INFO|ERR), "bogus packet size: %d, status=%2.2x nxpg=%2.2x.\n", 
			pkt_head.count, pkt_head.status, pkt_head.next);
		/* Update the stats */
#ifdef STAT_COUNTING
		device->stat_rx_size_errors++;
#endif
		pkt_len = 0;
		ne2k_set_boundary(device, pkt_head.next);
		this_frame = device->rx_current_page;
		goto ncp_done;
	}
		
	/* Grab the rest of the frame from the remote DMA channel */
	ne2k_min(device, buf, current_offset + sizeof(ne2k_packet_header), pkt_len);

	/* Update the statistics */
#ifdef STAT_COUNTING
	device->stat_rx_packets++;
#endif

	/* Update the recently added packet count */
	rx_pkt_count++;

	/* Set the boundary register (and current rx pointer) to the next packet */
	ne2k_set_boundary(device, pkt_head.next);
	this_frame = device->rx_current_page;

ncp_done:
	/* It's possible to have too few interrupts and more packets left on the card. 
	 * In that case, we have to check if there are more packets we don't already
	 * know about and signal to get them off */
	if (rxing_page != this_frame) {
		if ( !( device->status_ovw ) ) {
			ETHER_DEBUG(device->devID, INFO, "Extra packet being signalled for receive\n");
			if (device->fc_rx_lock.count < 1) {
				fc_lock(&device->fc_rx_lock);
				fc_signal(&device->fc_rx_lock, 1, B_DO_NOT_RESCHEDULE);
			}
		} else {
			ETHER_DEBUG( device->devID, FUNCTION, "ne2k_copy_packets EXIT (more packets)\n");
			return -1;	/* Signals that there are more packets in ovw servicing */
		}
	}

	if ((pkt_len < 0) || (rx_pkt_count <= 0))
		pkt_len = 0;

	ETHER_DEBUG(device->devID, INFO, "%d packets were grabbed from the on-card ring buffer\n", rx_pkt_count);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_copy_packets EXIT (pkt_len=%d)\n", pkt_len);

	return pkt_len;
}

/* HACK HACK HACK HACK HACK HACK HACK */
/* End of Hack */
/* HACK HACK HACK HACK HACK HACK HACK */
#if 0

// fold 32-bin checksum into 16-bit checksum
static inline uint32 fold( uint32 sum )
{
	while( sum & 0xFFFF0000 )
		sum = (sum & 0xFFFF) + (sum >> 16);
		
	return sum;
}
static uint16 chksum( uint8 *next, uint32 len )
{
	uint32 			count;
	uint8			*end;
	register uint32 sum = 0;
	bool			swap = false;
	
	// If 32 bytes or more, do fast unrolled loop
	if( (count = len >> 5) != 0 )
	{
		// Align
		if( (uint32)next & 0x03 )
		{
			// Align to even boundry
			if( ((uint32)next) & 0x01 )
			{
				sum += ((uint16)*next) << 8;
				next++;
				len--;
				swap = true;
			}
			
			// Align to long word boundry
			if( (uint32)next & 0x02 )
			{
				sum += *((uint16 *)next)++;
				len -= 2;
			}
			
			// Do we still have enough bytes after alignment for unrolled loop?
			if( (count = len >> 5) == 0 )
				goto align_bail;
		}
		
		asm ("
			clc
			head:
			adcl (%1), %0
			adcl 4(%1), %0
			adcl 8(%1), %0
			adcl 12(%1), %0
			adcl 16(%1), %0
			adcl 20(%1), %0
			adcl 24(%1), %0
			adcl 28(%1), %0
			lea 32(%1), %1
			loop head
			adcl $0, %0
			"
			: "=r" (sum), "=r" (next)
			: "r" (sum), "r" (next), "c" (count)
		);
		
		if( sum & 0x80000000 )
			sum = fold( sum );
	}
	align_bail:
	
	// Sum remaining bytes
	end = next + (len & 0x001E);
	while( next < end )
		sum += *((uint16 *)next)++;
	
	// Sum last byte if odd
	if( len & 0x0001 )
		sum += *((uint8 *)next);
	
	if( swap )
		return B_SWAP_INT16( ((uint16)fold( sum )) );
	else
		return fold( sum );
}
static inline int16 cksum_add( uint16 a, uint16 b ) {
	uint32 sum;
	sum = a + b;
	// Catch overflow
	if ( sum & 0xFFFF0000 )
		sum++;
	return sum;
}
uint16 get_checksum( void *buf, size_t len, uint16 prev_sum) {
	uint16 sum;
	sum = cksum_add( ~chksum( (uint8*)buf, len), prev_sum);
	return sum;
}
uint16 get_data_checksum(void *buf, uint32 offset, uint32 bytes)
{
	uint32 		sum = 0;
	sum = chksum( ((uint8 *)(buf)) + offset, bytes );
	sum = ~fold( sum );
	return sum;
}
#endif
/* HACK HACK HACK HACK HACK HACK HACK */
/* End of Hack */
/* HACK HACK HACK HACK HACK HACK HACK */

/* Read Hook Function */
static status_t read_hook(void *_device, off_t pos, void *buf, size_t *len) {
	device_info_t	*device = (device_info_t *)_device;
	uint8* 			the_buf = (uint8*) buf;

	(void)pos;

	ETHER_DEBUG(device->devID, FUNCTION, "read_hook ENTRY\n");

	/* Block until data is available in the card's ring */
	if(fc_wait(&device->fc_rx_lock, device->blockFlag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT, 1) != B_NO_ERROR) {
		ETHER_DEBUG(device->devID, FUNCTION, "read_hook EXIT (premature exit before rx_lock freed)\n");
		*len = 0;
		return B_ERROR;
	}
	fc_unlock(&device->fc_rx_lock);

	/* Block until the hardware lock is free */
	if(acquire_sem_etc(device->sem_hw_lock, 1, B_RELATIVE_TIMEOUT, ((device->blockFlag & B_TIMEOUT)? 0 : B_INFINITE_TIMEOUT)) != B_NO_ERROR) {
		ETHER_DEBUG(device->devID, FUNCTION, "read_hook EXIT (waiting for hw lock)\n");
		fc_lock(&device->fc_rx_lock);
		fc_signal(&device->fc_rx_lock, 1, B_DO_NOT_RESCHEDULE);
		*len = 0;
		return B_OK;
	}

	ETHER_DEBUG(device->devID, INFO, "successfully acquired hw_lock in read_hook\n");

	/* Copy the packets from the card into our buffer */
	*len = ne2k_copy_packets(device, buf);

	if (gDev[device->devID].debug & RX) {
		if (*len)
			dump_packet("rx", buf, *len);
	}

	/* XXX */
#if 0
	if ( (*len > 0) & (*len != 0xFFFFFFFF) ) {
		/* -- NASTY HACK -- Check TCP packet for checksum before going out */
		if ( the_buf[23] == 0x06 ) {
			if ( *len > 60 ) {
				uint16 ipHeadSize = (14 + ((the_buf[14] & 0xF) * 4));
				uint16 pLen = ((*len)-ipHeadSize);
				uint8 pHeader[] = { the_buf[26], the_buf[27], the_buf[28], the_buf[29], 
									the_buf[30], the_buf[31], the_buf[32], the_buf[33],
									0x00,	/* zero */
									0x06,	/* proto */
									(pLen>>8), (pLen&0xff) };	/* size of packet(minus Ethernet and IP header) */
				/* Calculate the checksum of the packet before it goes out... */
				/* (the_buf[34] is where TCP data starts) */
				uint16 sum = ~get_checksum( &pHeader, sizeof(pHeader), get_data_checksum( &the_buf[34], 0, pLen ) );
				if (sum != 0) {
					ETHER_DEBUG( device->devID, ERR, "**HACK** CHECKSUM ERROR **HACK**\n" );
					ETHER_DEBUG( device->devID, ERR, "* Packet Size:   %8.8d      *\n", (int)*len );
					ETHER_DEBUG( device->devID, ERR, "* Buffer Address:%8.8x      *\n", (int)the_buf );
					ETHER_DEBUG( device->devID, ERR, "* Sum:           %4.4x          *\n", sum );
					ETHER_DEBUG( device->devID, ERR, "* pLen:(Check)   %4.4x          *\n", pLen );
					ETHER_DEBUG( device->devID, ERR, "* status_ovw     %1.1x             *\n", (int)device->status_ovw );
					ETHER_DEBUG( device->devID, ERR, "* read_index     %1.1x             *\n", (int)device->dbuf_read_index );
					ETHER_DEBUG( device->devID, ERR, "* write_index    %1.1x             *\n", (int)device->dbuf_write_index );
					ETHER_DEBUG( device->devID, ERR, "********************************\n" );
					//dump_packet("rx", buf, *len);
				}
			} // if packet's not too small...
		} // if protocol is TCP
	} // if ( *len )
#endif 

	if ( device->status_ovw ) {
		/* If the OVW interrupt has happened at some time during this function, 
		 * the packet may very well be a garbage packet.  We must throw it away
		 * at this point since we're unsure of its validity. */
		*len = 0;
		ETHER_DEBUG(device->devID, ERR, "Chucking packet in read_hook at %Ld.\n", system_time() );
	}

	/* Let the world have access to the hardware again */
	release_sem_etc(device->sem_hw_lock, 1, B_DO_NOT_RESCHEDULE);

	ETHER_DEBUG(device->devID, FUNCTION, "read_hook EXIT\n");
	return B_OK;
}


static void ne2k_min(device_info_t *device, uint8 *dst, uint32 src, uint32 len)
{
	uint32		i = 0;
	cpu_status	cpu;
	uint32		word = 0;
	uint16		hword = 0;
	uint8		byte = 0;

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_min_ne2000 ENTRY\n");

	if(atomic_or(&device->dmaLock, 0x01)) {
		/* If we get here...we're toast! */
		ETHER_DEBUG(device->devID, ERR, "dmaing conflict in ne2k_min_ne2000\n");
#ifdef STAT_COUNTING
		device->stat_rdma_conflicts++;
#endif
		return;
	}

	if ( gDev[device->devID].flags & NE2K__32BIT_IO_ONLY ) {
		/* Round up for word-wide writes */
		len = ( len + 3 ) & 0xFFFFFFFC;
	}

	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_page_lock);

	write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P0 | NE2K_CR_START);
	write8(device->base + NE2K0_RBCR0_W, len & 0xff);
	write8(device->base + NE2K0_RBCR1_W, len >> 8);
	write8(device->base + NE2K0_RSAR0_W, src & 0xff);
	write8(device->base + NE2K0_RSAR1_W, src >> 8);
	write8(device->base + NE2K_CR, NE2K_CR_RD_RR | NE2K_CR_START);

	release_spinlock(&device->spin_page_lock);
	restore_interrupts(cpu);

	if (  ( gDev[device->devID].flags & NE2K__BUS_PCI ) 
	&& ( !( gDev[device->devID].flags & NE2K__16BIT_IO_ONLY ) ) ) {
		for ( i = 0; i < ( len >> 2 ); i += 1 ) {
			word = B_LENDIAN_TO_HOST_INT32( read32( device->base + NE2K_DATA) );
			if (dst) ((uint32*)dst)[i] = word;
		}
		if ( len & 0x03 ) {
			if (dst) dst += len & ~0x03;
			if ( len & 0x02 ) {
				hword = B_LENDIAN_TO_HOST_INT16( read16( device->base + NE2K_DATA ) );
				if (dst) *((uint16*)dst)++ = hword;
			}
			if ( len & 0x01 )
				byte = read8( device->base + NE2K_DATA );
				if (dst) *dst = byte;
		}
	} else {
		for ( i = 0; i < ( len >> 1 ); i += 1 ) {
			hword = B_LENDIAN_TO_HOST_INT16( read16(device->base + NE2K_DATA) );
			if (dst) ((uint16*)dst)[i] = hword;
		}
		/* Catch the odd input byte */
		if ( len & 0x01 ) {
			byte = read8( device->base + NE2K_DATA );
			if (dst) dst[len-1] = byte;
		}
	}

	atomic_and(&device->dmaLock, ~0x01);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_min_ne2000 EXIT\n");
}


/* Hard reset the card.  Wait until the reset is complete and continue */
static void ne2k_reset(device_info_t *device) {
	uint8 temp_reg;

	temp_reg = read8(device->base + NE2K_RESET);
	snooze(2000);
	write8(device->base + NE2K_RESET, temp_reg);
	snooze(2000);
}

/* Write data to the remote DMA channel */
static status_t ne2k_mout(device_info_t *device, uint32 dst, uint8 *src, uint32 len) {
	uint32 		i;
	int			was_reset = 0;
	cpu_status	cpu;

#ifdef MOUT_SANITY_CHECK
	int			retries = 0;
#endif

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_mout_ne2000 ENTRY\n");

	if(atomic_or(&device->dmaLock, 0x02)) {
		/* If we get here...we're toast! */
		ETHER_DEBUG(device->devID, ERR, "dmaing conflict in ne2k_mout_ne2000\n");
#ifdef STAT_COUNTING
		device->stat_rdma_conflicts++;
#endif
		return B_ERROR;
	}

	if ( gDev[device->devID].flags & NE2K__32BIT_IO_ONLY ) {
		/* Round up for word-wide writes */
		len = ( len + 3 ) & 0xFFFFFFFC;
	} else {
		/* Round up to an even count, for the 16bit writes */
		if (len & 0x01) len += 1;
	}

#ifdef MOUT_SANITY_CHECK
mout_sanity_retry:
#endif

	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_page_lock);

	/* We should already be in page 0, but to be safe... */
	write8(device->base + NE2K_CR, NE2K_CR_PS_P0 | NE2K_CR_START | NE2K_CR_RD_ARD);


	/* The "read-before-write" write8 code that follows causes the Linksys NP100 
	 * pcmcia cards to prepend 16 bytes of trash data to the start of every frame 
	 * transmitted!  Is is actually needed by any hardware? */
#ifdef NS8390_RW_BUGFIX
	/* Handle the read-before-write bug -- the NatSemi method doesn't seem to work.
	 * Actually, this doesn't always work either, but it's better than nothing */
	write8(device->base + NE2K0_RBCR0_W, 0x42);
	write8(device->base + NE2K0_RBCR1_W, 0x00);
	write8(device->base + NE2K0_RSAR0_W, 0x42);
	write8(device->base + NE2K0_RSAR1_W, 0x00);
	write8(device->base + NE2K_CR, NE2K_CR_RD_RR | NE2K_CR_START);
#endif

	/* Now the normal output. */
	write8(device->base + NE2K0_RBCR0_W, len & 0xff);
	write8(device->base + NE2K0_RBCR1_W, len >> 8);
	write8(device->base + NE2K0_RSAR0_W, dst & 0xff);
	write8(device->base + NE2K0_RSAR1_W, dst >> 8);
	write8(device->base + NE2K_CR, NE2K_CR_RD_RW | NE2K_CR_START);

	release_spinlock(&device->spin_page_lock);
	restore_interrupts(cpu);

	if ( len == 1 ) {
		/* Pseudo-Assertion */
		ETHER_DEBUG( device->devID, ERR, "SERIOUS ERROR: length = 1 in ne2k_mout_ne2000\n" );
	}

	if ( ( gDev[device->devID].flags & NE2K__BUS_PCI ) 
		&& ( !( gDev[device->devID].flags & NE2K__16BIT_IO_ONLY ) ) ) {
		for ( i = 0; i < ( len >> 2 ); i += 1 ) {
			write32( device->base + NE2K_DATA, B_HOST_TO_LENDIAN_INT32( ( (uint32*)src )[i] ) );
		}
		if ( len & 2 ) {
			src += ( len & ~2 );
			write16( device->base + NE2K_DATA, B_HOST_TO_LENDIAN_INT16( ( *(uint16*)src ) ) );
		}
	} else {
		for ( i = 0; i < (len >> 1); i += 1 ) {
			write16( device->base + NE2K_DATA, B_HOST_TO_LENDIAN_INT16( ( (uint16*)src )[i] ) );
		}
	}

#ifdef MOUT_SANITY_CHECK
	/* This was for the ALPHA version of the Linux ISA driver only, but enough people have 
	 * encountered problems and so it is still there.  Also, my own tests verified that this
	 * makes things work a little better by ensuring that everything's being written correctly.
	 * Unfortunately, even this test isn't perfect.  The D-Link DE-660 pcmcia cards don't
	 * correctly advance the dma write counter (for some reason, in the last 4 bytes) and
	 * this test seems to fail always with that card... */ 
	int addr, tries = 20;
	do { 
		cpu = disable_interrupts();
		acquire_spinlock(&device->spin_page_lock);
		/* DON'T check for NE2K_ISR_RDC' here -- it's broken! Check the "DMA" address instead. */ 
		int high = read8(device->base + NE2K0_CRDA1_R); 
		int low = read8(device->base + NE2K0_CRDA0_R); 
		release_spinlock(&device->spin_page_lock);
		restore_interrupts(cpu);
		addr = (high << 8) + low;
		if ( (dst + len) == addr) {
			break; 
		}
	} while ( --tries > 0 );
	if ( tries <= 0 ) {
		ETHER_DEBUG( device->devID, ERR, "dma mout address mismatch, %#4.4x (expected)," 
			"  %#4.4x (actual).\n", (dst + len), addr); 
		if ( retries++ == 0 )
			goto mout_sanity_retry;
	} 
#endif
	
	/* Wait on the semaphore which signals when DMA operations have completed */
	if ( acquire_sem_etc( device->sem_rdma_done, 1, B_RELATIVE_TIMEOUT, 10000 ) != B_NO_ERROR ) {
		ETHER_DEBUG(device->devID, ERR, "timeout waiting for RDC! (status_ovw %d)\n", (int)device->status_ovw);

		if ( ( atomic_and( &device->dmaLock,  ~0x02 ) & 0x02 ) == 0 ) {
			if ( acquire_sem_etc( device->sem_rdma_done, 1, B_RELATIVE_TIMEOUT, 10000 ) != B_NO_ERROR ) {
				ETHER_DEBUG(device->devID, ERR, "timeout waiting for RDC! dmaLock wrong\n");
			} else {
				ETHER_DEBUG(device->devID, ERR, "timeout waiting for RDC! recovered\n");
			}
		}
		else {
			ETHER_DEBUG(device->devID, ERR, "timeout waiting for RDC! reset\n");
#ifdef STAT_COUNTING
			device->stat_rdma_timeouts++;
#endif
			/* Jumpstart the card since it must be dead */
			ne2k_reset(device);
			/* XXX SPINLOCK THIS!!! */
			init_ne2k(device);

#if 0
			/* XXX What do we do here...we don't want to depend on the fc_locks' counts?!?! */
			fc_lock( &device->fc_tx_lock );
			if ( device->fc_tx_lock.count < 1 ) {
				/* Since the operation timed out, and the packets might be broke, we can signal
				 * for the next waiting transmit to continue... */
				fc_signal( &device->fc_tx_lock, 1, B_DO_NOT_RESCHEDULE );
				ETHER_DEBUG( device->devID, ERR, "tx_lock readjusted due to wonkiness\n" );
			} else {
				fc_unlock( &device->fc_tx_lock );
			}
#endif

			was_reset = 1;
		}
	}

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_mout_ne2000 EXIT\n");

	/* We want to indicate an error when the card was reset because the packets
	 * may be invalid and useless after resetting */
	if (was_reset)
		return B_ERROR;
	else
		return B_OK;
}

/* Trigger a send in hardware */
static status_t ne2k_trigger_send(device_info_t *device) {
	uint8 temp_reg;
	cpu_status cpu;

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_trigger_send ENTRY\n");

#ifdef STAT_COUNTING
	device->stat_tx_triggers++;
#endif

	/* Ensure that the trigger_send calls are serialized, as they are called
	 * both inside and outside of interrupt time */
	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_trig_lock);

	acquire_spinlock(&device->spin_page_lock);
	/* Trigger a send from DMA -> TX FIFO on the card */
	write8(device->base + NE2K0_TBCR0_W, (uint8)(device->dbuf_size[device->dbuf_read_index] & 0xff));
	write8(device->base + NE2K0_TBCR1_W, (uint8)(device->dbuf_size[device->dbuf_read_index] >> 8));
	write8(device->base + NE2K0_TPSR_W, device->tx_start_page + ((device->dbuf_read_index==0x01)?NE2K__TX_SINGLE_PAGE:0));
	/* Must preserve any occuring remote DMA transfers, if any */
	temp_reg = (read8(device->base + NE2K_CR) & NE2K_CR_RD_MSK);
	write8(device->base + NE2K_CR, temp_reg | NE2K_CR_TXP);
	release_spinlock(&device->spin_page_lock);

	/* Lock access to double buffer status variables */
	acquire_spinlock(&device->spin_dbuf_lock);
	device->dbuf_size[device->dbuf_read_index] = 0;
	device->dbuf_read_index ^= 0x01;
	release_spinlock(&device->spin_dbuf_lock);

	release_spinlock(&device->spin_trig_lock);
	restore_interrupts(cpu);

	/* When displaying the information, be aware that we flipped the bit
	 * before writing the data out.  We want to keep these sorts of calls
	 * outside our spinlocks */
	ETHER_DEBUG(device->devID, INFO, "triggered write from page %ld @ %d\n", (device->dbuf_read_index ^ 0x01),
		device->tx_start_page + ((device->dbuf_read_index==0x00)?NE2K__TX_SINGLE_PAGE:0));

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_trigger_send EXIT\n");

	return B_OK;
}

/* Write Hook */
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *len) {
	device_info_t	*device = (device_info_t *)_device;
	int32		buffer_length;
	status_t	status;
	uint16	 	output_page;
	cpu_status	cpu;
	int			tx_busy = 0;

	(void)pos;

	ETHER_DEBUG(device->devID, FUNCTION, "write_hook ENTRY\n");
	
	if ( *len > NE2K__MAX_FRAME_SIZE ) {
		ETHER_DEBUG(device->devID, ERR, ": write %ld > 1514 too long\n",*len);
		*len = NE2K__MAX_FRAME_SIZE;
	}
	
	if ( *len < NE2K__MIN_FRAME_SIZE ) {
		*len = NE2K__MIN_FRAME_SIZE;
	}

	buffer_length = *len;

	/* Block until at least one buffer is free (or we timeout) */
	status = acquire_sem_etc(device->sem_tx_lock, 1, B_RELATIVE_TIMEOUT, device->blockFlag & B_TIMEOUT ? 0 : NE2K__TX_TIMEOUT);

	if ( status == B_TIMED_OUT ) {
	   	ETHER_DEBUG( device->devID, ERR, "Transmitter timeout in write_hook -> Resignalling transmit locks\n" );
		release_sem_etc(device->sem_tx_lock, 1, B_DO_NOT_RESCHEDULE );
	} else if ( status != B_NO_ERROR ) {
		ETHER_DEBUG(device->devID, FUNCTION, "write_hook EXIT (prematurely killed before sem_tx_lock freed)\n");
		*len = 0;
		return B_ERROR;
	}

	/* We want to spinlock all accesses to all indices and counters relating to
	 * our double buffers. */
	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_dbuf_lock);
	if (device->dbuf_write_index == 0) {
		output_page = (device->tx_start_page << 8);
	} else {
		output_page = ((device->tx_start_page + NE2K__TX_SINGLE_PAGE) << 8);
	}
	device->dbuf_size[device->dbuf_write_index] = buffer_length;
	device->dbuf_write_index ^= 0x01;
	release_spinlock(&device->spin_dbuf_lock);
	restore_interrupts(cpu);
#ifdef STAT_COUNTING
	device->stat_tx_buffered++;
#endif

retry:
	/* Block until hardware is free */
	if(acquire_sem_etc(device->sem_hw_lock, 1, B_RELATIVE_TIMEOUT, device->blockFlag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT) != B_NO_ERROR) {
		ETHER_DEBUG(device->devID, FUNCTION, "write_hook EXIT (premature exit before hardware freed)\n");
		*len = 0;
		return B_ERROR;
	}

	/* Put data in the card's ring buffer and re-try if unsuccessful */
	status = ne2k_mout(device, output_page, (uint8 *)buf, buffer_length);

	if (status != B_NO_ERROR) {
		release_sem_etc(device->sem_hw_lock, 1, B_DO_NOT_RESCHEDULE);
		goto retry;
	}

	/* If the transmitter is already busy */
	cpu = disable_interrupts();
	acquire_spinlock( &device->spin_page_lock );
	tx_busy = ( read8(device->base + NE2K_CR) & NE2K_CR_TXP );
	release_spinlock( &device->spin_page_lock );
	restore_interrupts( cpu );

	if ( tx_busy ) {
		cpu = disable_interrupts();
		acquire_spinlock(&device->spin_dbuf_lock);
		/* Push the packet into a pending state */
		atomic_add( &device->dbuf_pending, 1 );
		release_spinlock(&device->spin_dbuf_lock);
		restore_interrupts(cpu);
#ifdef STAT_COUNTING
		device->stat_tx_busy_bail++;
#endif
	} else {
		/* Trigger a send on the card */
		ne2k_trigger_send(device);
	}

	if (gDev[device->devID].debug & TX) {
		dump_packet("tx", (uint8 *) buf, buffer_length);
	}

	release_sem_etc(device->sem_hw_lock, 1, B_DO_NOT_RESCHEDULE);

	ETHER_DEBUG(device->devID, FUNCTION, "write_hook EXIT\n");
	return B_OK;

}

static void setpromisc(device_info_t *device, uint32 on) {
	cpu_status	cpu;

	cpu = disable_interrupts();
	acquire_spinlock( &device->spin_page_lock );

	if (on) {
		write8(device->base + NE2K0_RCR_W, NE2K0_RCR_AB_BCA | NE2K0_RCR_PRO_ON);
	} else {
		/* when we shut promiscuous mode off, we don't want to kill the state
		 * of multicast mode */
		if (device->multi_count) {
			write8(device->base + NE2K0_RCR_W, NE2K0_RCR_AB_BCA | NE2K0_RCR_PRO_OFF | NE2K0_RCR_AM_MCC);
		} else {
			write8(device->base + NE2K0_RCR_W, NE2K0_RCR_AB_BCA | NE2K0_RCR_PRO_OFF);
		}
	}

	release_spinlock( &device->spin_page_lock );
	restore_interrupts( cpu );
}

static status_t domulti( device_info_t *device, char *addr )
{
	int 		i = 0;
	int 		num_multi = device->multi_count;
	cpu_status	cpu;

	if (num_multi == NE2K__MAX_MULTI) {
		return B_ERROR;
	}

	for (i = 0; i < num_multi; i++) {
		if (memcmp(&device->multi[i], addr, sizeof(device->multi[i])) == 0) {
			break;
		}
	}
	if (i == num_multi) {
		/* Only copy if it isn't there already */
		memcpy(&device->multi[i], addr, sizeof(device->multi[i]));
		device->multi_count++;
	}
	if (device->multi_count == 1) {
		ETHER_DEBUG(device->devID, INFO, "Enabling multicast\n");
		cpu = disable_interrupts();
		acquire_spinlock( &device->spin_page_lock );
		write8(device->base + NE2K0_RCR_W, NE2K0_RCR_AB_BCA | NE2K0_RCR_AM_MCC);
		release_spinlock( &device->spin_page_lock );
		restore_interrupts( cpu );
	}

	return B_NO_ERROR;
}

/* Standard driver control function */
static status_t control_hook(void * cookie, uint32 msg, void *buf, size_t len) {
	device_info_t *device = (device_info_t *) cookie;

	(void)len;

	ETHER_DEBUG(device->devID, FUNCTION, "control %x\n",(int)msg);

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			ETHER_DEBUG(device->devID, INFO, "Requesting ethernet address in control_hook\n");
			for (i=0; i<6; i++)
				((uint8 *)buf)[i] = device->mac_address.ebyte[i];
			return B_OK;
		}
		case ETHER_INIT:
			ETHER_DEBUG(device->devID, INFO, "Requesting init in control_hook\n");
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			ETHER_DEBUG(device->devID, INFO, "Requesting frame size in control_hook\n");
			*(uint32 *)buf = NE2K__MAX_FRAME_SIZE;
			return B_OK;
			
		case ETHER_ADDMULTI:
			ETHER_DEBUG(device->devID, INFO, "add multicast address in control_hook\n");
			domulti(device, (char *) buf);
			return B_OK;
		
		case ETHER_SETPROMISC:
			ETHER_DEBUG(device->devID, INFO, "set promiscuous mode in control_hook\n");
			setpromisc(device, *(uint32 *)buf );
			return B_OK;
			
		case ETHER_NONBLOCK:
			ETHER_DEBUG(device->devID, INFO, "ether_nonblock in control_hook\n");
			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_OK;
			
		default:
			ETHER_DEBUG(device->devID, INFO, "Unknown IOConTroL %x\n",(int)msg);
			return B_ERROR;
	}

}

static status_t free_hook(void *_device) {
	device_info_t *device = (device_info_t *) _device;

	ETHER_DEBUG(device->devID, FUNCTION, "free %x\n",(int)device);

	/* Device is now available again */
	atomic_and(&gOpenMask, ~(1 << device->devID));

	free(device);
	return 0;
}


static status_t close_hook(void *_device) {
	device_info_t	*device = (device_info_t*)_device;
	cpu_status		cpu;

	/* Force pending reads and writes to terminate */
	device->blockFlag = B_TIMEOUT;

	/* Stop the chip */
	cpu = disable_interrupts();
	acquire_spinlock(&device->spin_page_lock);
	write8(device->base + NE2K_CR, NE2K_CR_STOP);
	release_spinlock(&device->spin_page_lock);
	restore_interrupts( cpu );
	snooze(2000);

	free_resources(device);

	/* Clean up */
	remove_io_interrupt_handler(gDev[device->devID].irq, interrupt_hook, device);
	
#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, ne2k);
#endif
	gDev[device->devID].flags &= ~NE2K__CARD_OPENED;

	return (B_NO_ERROR);
}

/* Interrupt handling function */
static int32 interrupt_hook(void *_device) {
	device_info_t 	*device = (device_info_t *) _device;
	vint32 		handled = B_UNHANDLED_INTERRUPT;
	vint32		invoke_scheduler = 0;
	int32		loop_count;
	uint8		tempReg;
	vuint8 		status;

	ETHER_DEBUG(device->devID, FUNCTION, "interrupt_hook ENTRY\n");

	/* We have to lock all hardware accesses during interrupt time since
	 * there is a page switch buried in the overwrite servicing which
	 * executes concurrently with the interrupt handler */

	/* read (which clears)  interrupt indicators */
	acquire_spinlock(&device->spin_page_lock);
	status = read8(device->base + NE2K0_ISR_R);
	release_spinlock(&device->spin_page_lock);

#ifdef STAT_COUNTING
	device->stat_int_calls++;
#endif
	
	if (status == 0) {
		if ( gDev[device->devID].flags & NE2K__BUS_PCI ) {
			/* shared interrupts go to next devices ISR */
			return B_UNHANDLED_INTERRUPT;
		} else {
			ETHER_DEBUG( device->devID, INTERRUPT, "interrupt_hook called with ISR=%#2.2x, ovw=%ld\n", status, device->status_ovw );
			/* We may have already handled the interrupt.  This is obviously 
			 * NOT interrupt sharing-friendly...but ISA interrupt sharing don't
			 * work so well anyway (if at all) */
			return B_HANDLED_INTERRUPT;
		}
	} 

	/* On the ISA bus, we have to be aware of level-triggered interrups and
	 * therefore, we must handle multiple interrupt register changes in one
	 * call.  On the PCI bus, we don't come across this problem since the
	 * interrupts are edge-triggered and are generated as the state changes
	 * on the card */
	if (gDev[device->devID].flags & (NE2K__BUS_ISA | NE2K__BUS_PCMCIA) ) {
		loop_count = NE2K__MAX_WORK;
	} else {
		loop_count = 1;
	}

	while (loop_count-- > 0) {
#ifdef STAT_COUNTING
		if ( status & !NE2K0_ISR_RST ) 
			device->stat_ints++;	// We only want to cound 'good' interrupts
#endif

		/* You don't want to over-service the OVW interrupt...it'll tax the system since
		 * the OVW bit is held high until it is acknowledged (in a separate thread, at
		 * a later time). */
		if( status & NE2K0_ISR_RST ) {
			/* Apparently writing to this bit in the ISR has no effect */
			if ( status & NE2K0_ISR_OVW ) {	/* Overwrite warning */
				handled = B_HANDLED_INTERRUPT;
				/* Acknowledge the interrupt to avoid re-signalling */
				acquire_spinlock(&device->spin_page_lock);
				write8( device->base + NE2K0_ISR_W, NE2K0_ISR_OVW );
				release_spinlock(&device->spin_page_lock);
				if ( !device->status_ovw ) {
					ETHER_DEBUG(device->devID, ERR, "Interrupt ISR_OVW at %Ld, ISR=%2.2x\n", system_time(), status);
#ifdef STAT_COUNTING
					device->stat_int_ovw++;
#endif
					if (ne2k_interrupt_ovw(device) == B_INVOKE_SCHEDULER)
						invoke_scheduler = 1;
				}
			}
			ETHER_DEBUG( device->devID, INTERRUPT, "the ne2k was reset" );
		}

		/* The PRX and RXE interrupts used to have an if/else relationship with the OVW
		 * interrupt until I discovered that there were RXE interrupts often coming up
		 * along with the OVW interrupts... */
		if (status & NE2K0_ISR_PRX) {					/* Packet received */
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_PRX\n");
#ifdef STAT_COUNTING
			device->stat_int_prx++;
#endif
			if (ne2k_interrupt_prx(device) == B_INVOKE_SCHEDULER) {
				invoke_scheduler = 1;
			} else {
				handled = B_HANDLED_INTERRUPT;
			}
		} else if (status & NE2K0_ISR_RXE) {				/* Packet received with errors */
			/* just count the stats */
			acquire_spinlock(&device->spin_page_lock);
			tempReg = read8( device->base + NE2K0_RSR_R );
			write8(device->base + NE2K0_ISR_W, NE2K0_ISR_RXE);
			release_spinlock(&device->spin_page_lock);
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_RXE (RSR=%#2.2x)\n", tempReg);
#ifdef STAT_COUNTING
			device->stat_int_rxe++;
#endif
			handled = B_HANDLED_INTERRUPT;
		}

		if (status & NE2K0_ISR_PTX) {					/* Packet transmitted */
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_PTX\n");
			if ( (handled = ne2k_interrupt_ptx(device)) == B_INVOKE_SCHEDULER ) {
				invoke_scheduler = 1;
			}
#ifdef STAT_COUNTING
			device->stat_int_ptx++;
#endif
		} else if (status & NE2K0_ISR_TXE) {				/* Packet transmitted with errors */
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_TXE\n");
			handled = ne2k_interrupt_txe(device);
#ifdef STAT_COUNTING
			device->stat_int_txe++;
#endif
		}

		if (status & NE2K0_ISR_CNT) {					/* Counter overflow */
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_CNT\n");
			/* Remember to block in case of page switching! */
			acquire_spinlock(&device->spin_page_lock);
			device->stat_rx_frame_errors += read8(device->base + NE2K0_CNTR0_R);
			device->stat_rx_crc_errors += read8(device->base + NE2K0_CNTR1_R);
			device->stat_rx_missed_errors += read8(device->base + NE2K0_CNTR2_R);
			write8(device->base + NE2K0_ISR_W, NE2K0_ISR_CNT );
			release_spinlock(&device->spin_page_lock);
			handled = B_HANDLED_INTERRUPT;
#ifdef STAT_COUNTING
			device->stat_int_cnt++;
#endif
		}

		if (status & NE2K0_ISR_RDC) { 					/* Remote DMA Complete */
			ETHER_DEBUG(device->devID, INTERRUPT, "Interrupt ISR_RDC\n");
			handled = B_HANDLED_INTERRUPT;

			/* If we're writing to the dma channel, we must be waiting on this semaphore*/
			if ( atomic_and( &device->dmaLock,  ~0x02 ) & 0x02 ) {
				release_sem_etc( device->sem_rdma_done, 1, B_DO_NOT_RESCHEDULE );
				invoke_scheduler = 1;
			}

			acquire_spinlock(&device->spin_page_lock);
			write8(device->base + NE2K0_ISR_W, NE2K0_ISR_RDC);
			release_spinlock(&device->spin_page_lock);
			
#ifdef STAT_COUNTING
			device->stat_int_rdc++;
#endif
		}

		acquire_spinlock(&device->spin_page_lock);
		status = read8(device->base + NE2K0_ISR_R);
		release_spinlock(&device->spin_page_lock);
	}

	/* On the ISA bus in particular, the card sometimes enters a state where
	 * the interrupt register gets in a 'locked' state and the register just
	 * doesn't seem to go low.  In that case, we fire off the overwrite
	 * servicing thread to clean things up for us */
	if ((status & ~NE2K0_ISR_RST) && ( gDev[device->devID].flags & (NE2K__BUS_ISA | NE2K__BUS_PCMCIA))) {
		acquire_spinlock(&device->spin_page_lock);
		write8( device->base + NE2K_CR, (NE2K_CR_RD_ARD | NE2K_CR_PS_P0 | NE2K_CR_START) );
		release_spinlock(&device->spin_page_lock);
		/* 0xff is an okay status since it indicates insertion/removal */
		if ( status != 0xff ) {
			ETHER_DEBUG(device->devID, INTERRUPT, "Too much work done at interrupt time!\n");
		}
		/* Ack most interrupts */
		acquire_spinlock(&device->spin_page_lock);
		write8( device->base + NE2K0_ISR_W, NE2K0_ISR_INT_MSK );
		release_spinlock(&device->spin_page_lock);
	}

	ETHER_DEBUG(device->devID, FUNCTION, "interrupt_hook EXIT %s\n", (handled==B_UNHANDLED_INTERRUPT)?"(Interrupt Not Handled.)":"(B_HANDLED_INTERRUPT)");

	/* As far as the return values are concerned, B_HANDLED_INTERRUPT is
	 * returned when any of the interrupts are serviced above.  The only
	 * time B_UNHANDLED_INTERRUPT would be returned is if none of the if
	 * statements for the interrupt handling get hit. That's never going
	 * to be the case though since if there's no interrupt status in the
	 * register, interrupt_hook returns right when that happens. In the special
	 * case where B_INVOKE_SCHEDULER must be called after all things are
	 * processed, then a flag is set (invoke_scheduler) and B_INVOKE_SCHEDULER
	 * is returned upon exit as required. */
	if ( invoke_scheduler ) {
		return B_INVOKE_SCHEDULER;
	} else {
		if ( handled == B_UNHANDLED_INTERRUPT ) {
			ETHER_DEBUG( device->devID, ERR, "returning B_UNHANDLED_INTERRUPT\n");
		}
		return handled;
	}
}

int32 ne2k_interrupt_prx(device_info_t *device) {
	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_prx ENTRY\n");

	/* Signal to the receive thread that a packet to be buffered is on the 
	 * card's ring buffer */

	fc_lock(&device->fc_rx_lock);
	fc_signal(&device->fc_rx_lock, 1, B_DO_NOT_RESCHEDULE);

	acquire_spinlock(&device->spin_page_lock);
	write8(device->base + NE2K0_ISR_W, NE2K0_ISR_PRX );
	release_spinlock(&device->spin_page_lock);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_prx EXIT (B_HANDLED_INTERRUPT)\n");
	return B_INVOKE_SCHEDULER;
}

static int32 ne2k_interrupt_ptx(device_info_t *device) {
	uint8 			status;

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_ptx ENTRY\n");

	/* Read the status register */
	acquire_spinlock(&device->spin_page_lock);
 	status = read8(device->base + NE2K0_TSR_R);
	release_spinlock(&device->spin_page_lock);

	acquire_spinlock(&device->spin_page_lock);
	write8(device->base + NE2K0_ISR_W, NE2K0_ISR_PTX);
	release_spinlock(&device->spin_page_lock);

	acquire_spinlock(&device->spin_dbuf_lock);
	/* Now, if the next packet is pending, we'll do a back-to-back transmit trigger 
	 * on this puppy for that extra speed... */
	if (device->dbuf_pending) {
		release_spinlock(&device->spin_dbuf_lock);
		/* Trigger a send on the card */
		ne2k_trigger_send(device);
		acquire_spinlock(&device->spin_dbuf_lock);
		atomic_add( &device->dbuf_pending, -1 );
		release_spinlock(&device->spin_dbuf_lock);
	} else {
		release_spinlock(&device->spin_dbuf_lock);
#ifdef STAT_COUNTING
		device->stat_tx_not_pending_bail++;
#endif
	}
	release_sem_etc(device->sem_tx_lock, 1, B_DO_NOT_RESCHEDULE);

	if (status & NE2K0_TSR_COL) {
		/* We had a collision */
#ifdef STAT_COUNTING
		device->stat_collisions++;
#endif
		ETHER_DEBUG(device->devID, INFO, "TX Collision!! tx_status=%2.2x, status_ovw=%ld\n", status, device->status_ovw);
	}

	if ((status & NE2K0_TSR_PTX) || (status & NE2K0_TSR_ND)) {
		/* The packet went through okay (or non-deferral...) */
		ETHER_DEBUG(device->devID, INFO, "Packet Transmitted. tx_status=%2.2x\n", status);
#ifdef STAT_COUNTING
		device->stat_tx_packets++;
#endif

	} else {
		ETHER_DEBUG(device->devID, INFO, "One more TX Error!! tx_status=%2.2x\n", status);
#ifdef STAT_COUNTING
		device->stat_tx_errors++;
		if (status & NE2K0_TSR_ABT) device->stat_tx_aborted_errors++;
		if (status & NE2K0_TSR_CRS) device->stat_tx_carrier_errors++;
		if (status & NE2K0_TSR_FU)  device->stat_tx_fifo_errors++;
		if (status & NE2K0_TSR_CDH) device->stat_tx_heartbeat_errors++;
		if (status & NE2K0_TSR_OWC) device->stat_tx_window_errors++;
#endif
	}

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_ptx EXIT\n");
	return B_INVOKE_SCHEDULER;
}

/* A transmitter error has happened. Most likely excess collisions (which
 * is a fairly normal condition). If the error is one where the Tx will
 * have been aborted, we try and send another one right away, instead of
 * letting the failed packet sit and collect dust in the Tx buffer. This
 * is a much better solution as it avoids kernel based Tx timeouts, and
 * an unnecessary card reset. */
static int32 ne2k_interrupt_txe(device_info_t *device) {
	uint8 	tx_status; 
	uint8 	tx_was_aborted;

	acquire_spinlock(&device->spin_page_lock);
	tx_status = read8(device->base + NE2K0_TSR_R);
	release_spinlock(&device->spin_page_lock);

	tx_was_aborted = tx_status & (NE2K0_TSR_ABT | NE2K0_TSR_FU);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_txe ENTRY\n");

	ETHER_DEBUG(device->devID, ERR, "transmitter error: ");
	if ((tx_status & NE2K0_TSR_ABT) && (gDev[device->devID].debug & ERR))
		dprintf( "aborted ");
	if ((tx_status & NE2K0_TSR_ND) && (gDev[device->devID].debug & ERR))
		dprintf( "non-deferral ");
	if ((tx_status & NE2K0_TSR_CRS) && (gDev[device->devID].debug & ERR))
		dprintf( "lost-carrier ");
	if ((tx_status & NE2K0_TSR_FU) && (gDev[device->devID].debug & ERR))
		dprintf( "FIFO-underrun ");
	if ((tx_status & NE2K0_TSR_CDH) && (gDev[device->devID].debug & ERR))
		dprintf("lost-heartbeat ");
	if (gDev[device->devID].debug & ERR)
		dprintf("\n");

	if (tx_was_aborted) {
		ne2k_interrupt_ptx(device);
	}

	/* Note: NCR reads zero on 16 collisions so we add them
	 * in by hand. Somebody might care... */
	if (tx_status & NE2K0_TSR_ABT) {
#ifdef STAT_COUNTING
		device->stat_collisions += 16;
#endif
	}

	acquire_spinlock(&device->spin_page_lock);
	write8(device->base + NE2K0_ISR_W, NE2K0_ISR_TXE);
	release_spinlock(&device->spin_page_lock);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_txe EXIT\n");

	return B_HANDLED_INTERRUPT;
}

static int32 ne2k_interrupt_ovw(device_info_t *device) {
	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_ovw ENTRY\n");

	if (atomic_or(&device->status_ovw, 0x01))
	{
		ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_ovw EXIT (reentry)\n");
#ifdef STAT_COUNTING
		device->stat_ovw_reentry_defer++;
#endif
		return B_HANDLED_INTERRUPT;
	}

	/* Trigger the cleanup thread */
	release_sem_etc(device->sem_reset_lock, 1, B_DO_NOT_RESCHEDULE);

	ETHER_DEBUG(device->devID, FUNCTION, "ne2k_interrupt_ovw EXIT\n");

	return B_INVOKE_SCHEDULER;
}

static status_t ne2k_cleanup_thread(void *_device) {
	device_info_t	*device		= (device_info_t *)_device;
	int				was_txing	= 0;
	int 			must_resend	= 0;
	int				sem_cleanup_count;
	cpu_status		cpu;

	while (1) {
		/* Wait to be 'triggered' */
		if(acquire_sem_etc(device->sem_reset_lock, 1, B_RELATIVE_TIMEOUT, ((gCleanupBlockFlag & B_TIMEOUT)? 0 : B_INFINITE_TIMEOUT)) != B_NO_ERROR) {
			ETHER_DEBUG(device->devID, FUNCTION, "cleanup_thread disturbed while waiting for reset lock\n");
			return B_OK;
		}

		/* Block until hardware is free */
		if(acquire_sem_etc(device->sem_hw_lock, 1, B_RELATIVE_TIMEOUT, ((gCleanupBlockFlag & B_TIMEOUT)? 0 : B_INFINITE_TIMEOUT)) != B_NO_ERROR) {
			ETHER_DEBUG(device->devID, FUNCTION, "cleanup_thread disturbed while waiting for hw lock\n");
			release_sem_etc(device->sem_reset_lock, 1, B_DO_NOT_RESCHEDULE);
			return B_OK;
		}

		/* don't reset more than once */
		sem_cleanup_count = 1000;
		while(sem_cleanup_count-- > 0 && acquire_sem_etc(device->sem_reset_lock, 1, B_RELATIVE_TIMEOUT, 0) == B_NO_ERROR)
			; /* Do nothing */

#ifdef STAT_COUNTING
		device->stat_rx_resets++;
#endif

		/* XXX */
		ETHER_DEBUG( device->devID, ERR, "Cleaning up the OVW at %Ld\n", system_time() );

		/* Record whether a Tx was in progress and then issue the stop command. */
		cpu = disable_interrupts();
		acquire_spinlock( &device->spin_page_lock );
		was_txing = read8(device->base + NE2K_CR) & NE2K_CR_TXP;
		write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_STOP | NE2K_CR_PS_P0);
		release_spinlock( &device->spin_page_lock );
		restore_interrupts( cpu );

		/* Wait a full Tx time (1.2ms) + some guard time, NS says 1.6ms total.
		 * Early datasheets said to poll the reset bit, but now they say that
		 * it "is not a reliable indicator and subsequently should be ignored."
		 * The Linux guys wait at least 10ms at interrupt time.  We the Be guys 
		 * find that silly and snooze in a separate thread. */
		snooze(10000);

		cpu = disable_interrupts();
		acquire_spinlock( &device->spin_page_lock );
		/* Reset RBCR[01] back to zero as per magic incantation. */
		write8(device->base + NE2K0_RBCR0_W, 0x00);
		write8(device->base + NE2K0_RBCR1_W, 0x00);

		/* See if any Tx was interrupted or not. According to NS, this
		 * step is vital, and skipping it will cause no end of havoc. */
		if (was_txing) { 
			uint8 tx_completed = read8(device->base + NE2K0_ISR_R) & (NE2K0_ISR_PTX | NE2K0_ISR_TXE);
			if (!tx_completed) must_resend = 1;
		}

		/* Have to enter loopback mode and then restart the NIC before
		 * you are allowed to slurp packets up off the ring. */
		write8(device->base + NE2K0_TCR_W, NE2K0_TCR_LB_INNLB);
		write8(device->base + NE2K_CR, NE2K_CR_RD_ARD | NE2K_CR_PS_P0 | NE2K_CR_START);
		release_spinlock( &device->spin_page_lock );
		restore_interrupts( cpu );
		
		ETHER_DEBUG(device->devID, INFO, "overwrite service -> roto-rooting packets from receive ring\n");

		/* Roto root the packets out of the card's buffer...by passing a null 
		 * destination buffer, we are just saying that we don't want to save
		 * the packets but just want to read them out of the card...to make
		 * the hardware gods happy */
		ne2k_copy_packets(device, NULL);
		
		cpu = disable_interrupts();
		acquire_spinlock( &device->spin_page_lock );
		if ( read8(device->base + NE2K0_ISR_R) & ~NE2K0_ISR_OVW ) {
			ETHER_DEBUG( device->devID, INFO, "Other Interrupts have occured since overwrite servicing started\n" );
		}

		/* Clear the OVW interrupt */
		write8(device->base + NE2K0_ISR_W, NE2K0_ISR_OVW);

		/* Leave loopback mode, and resend any packet that got stopped. */
		write8(device->base + NE2K0_TCR_W, NE2K0_TCR_LB_NORM);

		release_spinlock( &device->spin_page_lock );
		restore_interrupts( cpu );

		if ( must_resend ) {
			ETHER_DEBUG(device->devID, ERR, "Resending packet in OVW servicing\n");
#ifdef STAT_COUNTING
			device->stat_tx_ovw_resends++;
#endif
			cpu = disable_interrupts();
			acquire_spinlock(&device->spin_dbuf_lock);
			if (device->dbuf_pending) {
				release_spinlock(&device->spin_dbuf_lock);
				restore_interrupts(cpu);
				/* Trigger a send on the card */
				ne2k_trigger_send(device);
				cpu = disable_interrupts();
				acquire_spinlock(&device->spin_dbuf_lock);
				atomic_add( &device->dbuf_pending, -1 );
				release_spinlock(&device->spin_dbuf_lock);
				restore_interrupts(cpu);
			} else {
				release_spinlock(&device->spin_dbuf_lock);
				restore_interrupts(cpu);
#ifdef STAT_COUNTING
				device->stat_tx_not_pending_bail++;
#endif
			}
			release_sem_etc(device->sem_tx_lock, 1, B_DO_NOT_RESCHEDULE);
		}

		/* We're now no longer servicing this interrupt */
		atomic_and(&device->status_ovw, ~0x01);

		/* Let everyone have access to the hardware again */
		release_sem(device->sem_hw_lock);
	}

	return B_OK;

}


/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "ne2k arg...",
   for example "AcmeEthernet R" to enable a received packet trace.
*/
#if DEBUGGER_COMMAND
static int ne2k(int argc, char **argv) {
	uint16 	i,j;
	char 	tempchar;
	int		id;
	const char * usage = "usage: ne2k { 0-9 | Function_calls | Interrupts | Number_frames | Stats | Rx_trace | Tx_trace | infO }\n";	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			tempchar = *argv[j];
			id = atoi(&tempchar);
			if ((id < MAX_CARDS) && (gDev[id].devInfo != NULL))
			{
				if ( gDev[id].flags & NE2K__BUS_PCI ) {
					kprintf("Current card set to %d (PCI %s)\n", id, pci_clone_list[gDev[id].cardType].name);
				} else if ( gDev[id].flags & NE2K__BUS_PCMCIA ) {
					kprintf("Current card set to %d (PCMCIA)\n", id);
				} else {
					kprintf("Current card set to %d (ISA)\n", id);
				}
				gDevID = id;
			}
			else
			{
				kprintf("Card %d doesn't exist!\n", id);
			}
			break;
		case 'F':
		case 'f':
			gDev[gDevID].debug ^= FUNCTION;
			if (gDev[gDevID].debug & FUNCTION) 
				kprintf("Function call trace Enabled\n");
			else 			
				kprintf("Function call trace Disabled\n");
			break; 
		case 'I':
		case 'i':

			gDev[gDevID].debug ^= INTERRUPT;
			if (gDev[gDevID].debug & INTERRUPT) 
				kprintf("INTERRUPT messages Enabled\n");
			else 			
				kprintf("INTERRUPT messages Disabled\n");
			break;
		case 'N':
		case 'n':
			gDev[gDevID].debug ^= SEQ;
			if (gDev[gDevID].debug & SEQ) 
				kprintf("Sequence numbers packet trace Enabled\n");
			else 			
				kprintf("Sequence numbers packet trace Disabled\n");
			break; 
		case 'O':
		case 'o':
			gDev[gDevID].debug ^= INFO;
			if (gDev[gDevID].debug & INFO) 
				kprintf("INFO messages Enabled\n");
			else 			
				kprintf("INFO messages Disabled\n");
			break;
		case 'R':
		case 'r':
			gDev[gDevID].debug ^= RX;
			if (gDev[gDevID].debug & RX) 
				kprintf("Receive packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break;
		case 'T':
		case 't':
			gDev[gDevID].debug ^= TX;
			if (gDev[gDevID].debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'S':
		case 's':
			kprintf(kDevName " STATISTICS (Card %ld):\n", gDevID);			
			kprintf("------------\n");
#ifdef STAT_COUNTING
			kprintf("General: \n");
			kprintf("received %ld, transmitted %ld\n", gDev[gDevID].devInfo->stat_rx_packets, 
				gDev[gDevID].devInfo->stat_tx_packets);
			kprintf("------------\n");
			kprintf("Interrupts: \n");
			kprintf("total ints %ld: prx %ld, ptx %ld, rxe %ld, txe %ld\n", gDev[gDevID].devInfo->stat_ints, 
				gDev[gDevID].devInfo->stat_int_prx, gDev[gDevID].devInfo->stat_int_ptx,
				gDev[gDevID].devInfo->stat_int_rxe, gDev[gDevID].devInfo->stat_int_txe);
			kprintf("ovw %ld, cnt %ld, rdc %ld, rst %ld\n",
				gDev[gDevID].devInfo->stat_int_ovw, gDev[gDevID].devInfo->stat_int_cnt,
				gDev[gDevID].devInfo->stat_int_rdc, gDev[gDevID].devInfo->stat_int_rst);
			kprintf("ints dropped %ld\n", gDev[gDevID].devInfo->stat_ints_dropped);
			kprintf("------------\n");
			kprintf("RX Errors: \n");
			kprintf("rx fifo errors %ld, rx frame errors %ld, rx crc errors %ld\n", 
				gDev[gDevID].devInfo->stat_rx_fifo_errors, gDev[gDevID].devInfo->stat_rx_frame_errors, 
				gDev[gDevID].devInfo->stat_rx_crc_errors);
			kprintf("rx missed errors %ld, rx resets %ld, rx size errors %ld, rx bogus errors %ld\n", 
				gDev[gDevID].devInfo->stat_rx_missed_errors, gDev[gDevID].devInfo->stat_rx_resets,
				gDev[gDevID].devInfo->stat_rx_size_errors, gDev[gDevID].devInfo->stat_rx_bogus_errors);
			kprintf("------------\n");
			kprintf("TX Errors: %ld\n", gDev[gDevID].devInfo->stat_tx_errors);
			kprintf("tx aborted errors %ld, tx carrier errors %ld, tx fifo errors %ld\n", 
				gDev[gDevID].devInfo->stat_tx_aborted_errors, gDev[gDevID].devInfo->stat_tx_carrier_errors, 
				gDev[gDevID].devInfo->stat_tx_fifo_errors);
			kprintf("tx heartbeat errors %ld, tx window errors %ld, collisions %ld\n", 
				gDev[gDevID].devInfo->stat_tx_heartbeat_errors, gDev[gDevID].devInfo->stat_tx_window_errors,
				gDev[gDevID].devInfo->stat_collisions);
			kprintf("tx ovw resends %d\n", (int)gDev[gDevID].devInfo->stat_tx_ovw_resends);
			kprintf("------------\n");
			kprintf("TX Info:\n");
			kprintf("tx buffered %ld, tx triggers %ld\n", gDev[gDevID].devInfo->stat_tx_buffered, 
				gDev[gDevID].devInfo->stat_tx_triggers);
			kprintf("tx not pending bail %ld, tx busy bail %ld, pending %ld\n", 
				gDev[gDevID].devInfo->stat_tx_not_pending_bail, gDev[gDevID].devInfo->stat_tx_busy_bail, 
				gDev[gDevID].devInfo->dbuf_pending);
			kprintf("------------\n");
			kprintf("RDMA Info:\n");
			kprintf("rdma conflicts %ld, rdma_timeouts %ld\n", gDev[gDevID].devInfo->stat_rdma_conflicts, 
				gDev[gDevID].devInfo->stat_rdma_timeouts);
			kprintf("------------\n");
			kprintf("Other Info:\n");
			kprintf("ovw rw busy defer %ld, ovw reentry defer %ld\n", 
				gDev[gDevID].devInfo->stat_ovw_rw_busy_defer, 
				gDev[gDevID].devInfo->stat_ovw_reentry_defer);
			kprintf(" status_ovw %ld\n", gDev[gDevID].devInfo->status_ovw);
			kprintf("read index %ld, write index %d\n", gDev[gDevID].devInfo->dbuf_read_index,
				(int)gDev[gDevID].devInfo->dbuf_write_index);
			kprintf("------------\n");
#else
			kprintf("Stats counting DISABLED!\n");
			kprintf("(You must recompile the driver with STAT_COUNTING defined to get this feature)\n");
			kprintf("------------\n");
#endif // STAT_COUNTING
			break; 
		default:
			kprintf("%s",usage);
			return 0;
		}
	}
	return 0;
}
#endif /* DEBUGGER_COMMAND */

/*** PCMCIA STUFF ***/

static dev_info_t dev_info = "ne2k";
   
typedef struct local_info_t {
	dev_node_t	node;
	int		devID;
	int		stop;
} local_info_t;

/*
  BeOS bus manager declarations: we use the shorthand references to
  bus manager functions to make the code somewhat more portable across
  platforms.
*/
static cs_client_module_info		*cs = NULL;
static ds_module_info			*ds = NULL;
#define CardServices			cs->_CardServices
#define cs_add_timer			cs->_add_timer
#define register_pccard_driver		ds->_register_pccard_driver
#define unregister_pccard_driver	ds->_unregister_pccard_driver

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret) {
	error_info_t err;
	err.func = func; err.retcode = ret;
	CardServices(ReportError, handle, &err);
}

/*======================================================================

  ne2k_pcmcia_attach() creates an "instance" of the driver, allocating
  local data structures for one device.  The device is registered
  with Card Services.

  The dev_link structure is initialized, but we don't actually
  configure the card at this point -- we wait until we receive a
  card insertion event.
======================================================================*/

static dev_link_t *ne2k_pcmcia_attach(void) {
	client_reg_t 	client_reg;
	dev_link_t	*link;
	local_info_t	*local;
	int		ret, i;
	
	/* Find a free slot in the device table */
	for (i = 0; i < MAX_CARDS; i++) {
		if ( (gDev[i].flags & NE2K__BUS_MSK) != 0 ) continue;
		if ( gDev[i].devLink == NULL ) break;		/* Free slot found */
	}
	if ( i == MAX_CARDS ) {
		ETHER_DEBUG( gDevID, ERR, "ne2k_pcmcia_attach - no free gDev slots!\n" );
		return NULL;
	}

	/* Set the bus type of the card */
	gDev[i].flags |= NE2K__BUS_PCMCIA;
	
	/* Initialize the dev_link_t structure */
	link = malloc(sizeof(struct dev_link_t));
	memset(link, 0, sizeof(struct dev_link_t));
	gDev[i].devLink = link;
	gDevCount += 1;
	link->release.function = &ne2k_pcmcia_release;
	link->release.data = (u_long)link;

	/* Interrupt setup */
	link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
	link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
	link->irq.IRQInfo2 = 0xdeb8;
	link->irq.Handler = NULL;
	
	/*
	  General socket configuration defaults can go here.  In this
	  client, we assume very little, and rely on the CIS for almost
	  everything.  In most clients, many details (i.e., number, sizes,
	  and attributes of IO windows) are fixed by the nature of the
	  device, and can be hard-wired here.
	*/
	link->conf.Attributes = 0;
	link->conf.Vcc = 50;
	link->conf.IntType = INT_MEMORY_AND_IO;

	/* Allocate space for private device-specific data */
	local = malloc(sizeof(local_info_t));
	memset(local, 0, sizeof(local_info_t));
	link->priv = local;
	local->devID = i;	/* Store the device ID for the gDev references */

	/* Register with Card Services */
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &ne2k_pcmcia_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;
	ret = CardServices(RegisterClient, &link->handle, &client_reg);

	if (ret != 0) {
		cs_error(link->handle, RegisterClient, ret);
		ne2k_pcmcia_detach(link);
		return NULL;
	}

	return link;
} 

/*======================================================================

  This deletes a driver "instance".  The device is de-registered
  with Card Services.  If it has been released, all local data
  structures are freed.  Otherwise, the structures will be freed
  when the device is released.

======================================================================*/

static void ne2k_pcmcia_detach(dev_link_t *link)
{
	int i;
	
	if(link == NULL) {
		dprintf("dummy_detach called with NULL argumant\n");
		return;
	}

	/* Locate device structure */
	for (i = 0; i < MAX_CARDS; i++)
		if (gDev[i].devLink == link) break;
	if (i == MAX_CARDS)
		return;

	/*
	 * If the device is currently configured and active, we won't
	 * actually delete it yet.  Instead, it is marked so that when
	 * the release() function is called, that will trigger a proper
	 * detach(). */

	if (link->state & DEV_CONFIG) {
		link->state |= DEV_STALE_LINK;
		return;
	}

	/* Break the link with Card Services */
	if (link->handle)
		CardServices(DeregisterClient, link->handle);
	
	/* Unlink device structure, free pieces */
	gDev[i].devLink = NULL;
	gDev[i].flags = 0;	/* Our way of saying there is no device present */

	if (link->priv)
		free(link->priv);
	free(link);

	gDev[i].flags &= ~NE2K__CARD_INSERTED;

	gDevCount -= 1;
} 

/*======================================================================

  ne2k_pcmcia_config() is scheduled to run after a CARD_INSERTION event
  is received, to configure the PCMCIA socket, and to make the
  device available to the system.
  
======================================================================*/

#define CS_CHECK(fn, h, a) \
while ((last_ret=CardServices(last_fn=(fn),h,a))!=0) goto cs_failed
#define CS_CHECK2(fn, h, a, b) \
while ((last_ret=CardServices(last_fn=(fn),h,a,b))!=0) goto cs_failed

#define CFG_CHECK(fn, h, a) \
if (CardServices(fn, h, a) != 0) goto next_entry
#define CFG_CHECK2(fn, h, a, b) \
if (CardServices(fn, h, a, b) != 0) goto next_entry

static void ne2k_pcmcia_config(dev_link_t *link) {
	client_handle_t handle;
	tuple_t tuple;
	cisparse_t parse;
	local_info_t *dev;
	int last_fn, last_ret, nd;
	u_char buf[64];
	
	handle = link->handle;
	dev = link->priv;
	
	/*
	   This reads the card's CONFIG tuple to find its configuration
	   registers.
	*/
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK2(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];
	
	/* Configure card */
	link->state |= DEV_CONFIG;
	
	/*
	  In this loop, we scan the CIS for configuration table entries,
	  each of which describes a valid card configuration, including
	  voltage, IO window, memory window, and interrupt settings.
	
	  We make no assumptions about the card to be configured: we use
	  just the information available in the CIS.  In an ideal world,
	  this would work for any PCMCIA card, but it requires a complete
	  and accurate CIS.  In practice, a driver usually "knows" most of
	  these things without consulting the CIS, and most client drivers
	  will only use the CIS to fill in implementation-defined details.
	*/
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t dflt;
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);

		memset(&dflt, 0, sizeof(cistpl_cftable_entry_t));

		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK2(ParseTuple, handle, &tuple, &parse);
		
		link->conf.ConfigIndex = cfg->index;
		
		/* Does this card need audio output? */
		if (cfg->flags & CISTPL_CFTABLE_AUDIO) {
			link->conf.Attributes |= CONF_ENABLE_SPKR;
			link->conf.Status = CCSR_AUDIO_ENA;
		}
	
		/* Use power settings for Vcc and Vpp if present */
		/*  Note that the CIS values need to be rescaled */
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM)) {
			link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		} else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM)) {
			link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;
		}
		
		if (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM)) {
			link->conf.Vpp1 = link->conf.Vpp2 =
				cfg->vpp1.param[CISTPL_POWER_VNOM]/10000;
		} else if (dflt.vpp1.present & (1<<CISTPL_POWER_VNOM)) {
			link->conf.Vpp1 = link->conf.Vpp2 =
				dflt.vpp1.param[CISTPL_POWER_VNOM]/10000;
		}
		
		/* Do we need to allocate an interrupt? */
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1) {
			link->conf.Attributes |= CONF_ENABLE_IRQ;
		}
		
		/* IO window settings */
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
/* The newer LinkSys Card (EC2T) doesn't work in CISTPL_IO_16BIT, it requires */
/* IO_DATA_PATH_WIDTH_AUTO to access the pc cards io space. AUTO causes the DLINK*/
/* card to run about 5% slower, and oddly, the older Linksys card to run about*/
/* 3% faster. It may make sense to pick the DATA_PATH_WIDTH differently for different*/
/* cards, for now, one size fits all, AUTO.*/
#if 0
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
#endif
			link->io.BasePort1 = io->win[0].base;
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}
		
		/* This reserves IO space but doesn't actually enable it */
		CFG_CHECK(RequestIO, link->handle, &link->io);
		
		/* If we got this far, we're cool! */
		break;
		
	next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT) dflt = *cfg;
		CS_CHECK(GetNextTuple, handle, &tuple);
	}
		
	/*
	  Allocate an interrupt line.  Note that this does not assign a
	  handler to the interrupt, unless the 'Handler' member of the
	  irq structure is initialized.
	*/
	if (link->conf.Attributes & CONF_ENABLE_IRQ){
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	}
	
	/*
	   This actually configures the PCMCIA socket -- setting up
	   the I/O windows and the interrupt mapping, and putting the
	   card and host interface into "Memory and IO" mode.
	*/
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);
	
	/*
	  At this point, the dev_node_t structure(s) need to be
	  initialized and arranged in a linked list at link->dev.
	*/
	for (nd = 0; nd < MAX_CARDS; nd++){
		if (gDev[nd].devLink == link) break;
	}
	
	sprintf(dev->node.dev_name, "net/ne2k/%d", nd);
	dev->node.major = dev->node.minor = 0;
	link->dev = &dev->node;
	
	/* Finally, report what we've done */
	dprintf("%s: index 0x%02x: Vcc %d.%d", 
			dev->node.dev_name, link->conf.ConfigIndex,
			link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1) {
		dprintf(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	}
	if (link->conf.Attributes & CONF_ENABLE_IRQ) {
		dprintf(", irq %d", link->irq.AssignedIRQ);
	}
	if (link->io.NumPorts1) {
		dprintf(", io 0x%04x-0x%04x", link->io.BasePort1,
				link->io.BasePort1+link->io.NumPorts1-1);
	}
	dprintf("\n");

	/* Put our base io port and irq in the gDev Structure */
	gDev[dev->devID].port = link->io.BasePort1;
	gDev[dev->devID].irq = link->irq.AssignedIRQ;
	
	gDev[dev->devID].flags |= NE2K__CARD_INSERTED;

	link->state &= ~DEV_CONFIG_PENDING;
	return;

cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	ne2k_pcmcia_release((u_long)link);
} 

/*======================================================================

  After a card is removed, ne2k_pcmcia_release() will unregister the
  device, and release the PCMCIA configuration.  If the device is
  still open, this will be postponed until it is closed.
  
======================================================================*/

static void ne2k_pcmcia_release(u_long arg)
{
	dev_link_t	*link = (dev_link_t *)arg;
	local_info_t	*dev = link->priv;
	device_info_t	*device = gDev[dev->devID].devInfo;

	/* If the device is currently in use, we won't release until it
	   is actually closed, because until then, we can't be sure that
	   no one will try to access the device or its data structures.  */
	if (link->open) {
		link->state |= DEV_STALE_CONFIG;
		return;
	}

	/* Unlink the device chain */
	link->dev = NULL;

	/*In a normal driver, additional code may be needed to release
	  other kernel data structures associated with this device. */
	
	/* Don't bother checking to see if these succeed or not */
	if (link->win) {
		CardServices(ReleaseWindow, link->win);
	}
	CardServices(ReleaseConfiguration, link->handle);
	if (link->io.NumPorts1){
		CardServices(ReleaseIO, link->handle, &link->io);
	}
	if (link->irq.AssignedIRQ){
		CardServices(ReleaseIRQ, link->handle, &link->irq);
	}
	link->state &= ~DEV_CONFIG;

	/* If the card is still opened - delete the appropriate semaphores in 
	 * order to block the read/write calls until the driver is properly closed */
	if ( gDev[dev->devID].flags & NE2K__CARD_OPENED ) {
		ETHER_DEBUG( gDevID, ERR, "card still open upon removal, cleaning up...\n" );
		delete_sem( device->sem_tx_lock );
		delete_sem( device->sem_hw_lock );
		delete_sem( device->sem_reset_lock );
	} else {
		/* Signal that the gDev slot is now free */
		gDev[dev->devID].flags = 0;
	}

	gDev[dev->devID].flags &= ~NE2K__CARD_INSERTED;
	
	if (link->state & DEV_STALE_LINK) ne2k_pcmcia_detach(link);
}

/*======================================================================

  The card status event handler.  Mostly, this schedules other
  stuff to run after an event is received.

  When a CARD_REMOVAL event is received, we immediately set a
  private flag to block future accesses to this device.  All the
  functions that actually access the device should check this flag
  to make sure the card is still present.
======================================================================*/

static int ne2k_pcmcia_event(event_t event, int priority, event_callback_args_t *args)
{
	dev_link_t *link = args->client_data;
	(void)priority;

	switch (event) {
		case CS_EVENT_CARD_REMOVAL:
			link->state &= ~DEV_PRESENT;
			if (link->state & DEV_CONFIG) {
				((local_info_t *)link->priv)->stop = 1;
				link->release.expires = RUN_AT(HZ/20);
				cs_add_timer(&link->release);
			}
			break;
		case CS_EVENT_CARD_INSERTION:
			link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
			ne2k_pcmcia_config(link);
			break;
		case CS_EVENT_PM_SUSPEND:
			link->state |= DEV_SUSPEND;
			/* Fall through... */
		case CS_EVENT_RESET_PHYSICAL:
			/* Mark the device as stopped, to block IO until later */
			((local_info_t *)link->priv)->stop = 1;
			if (link->state & DEV_CONFIG)
				CardServices(ReleaseConfiguration, link->handle);
			break;
			
		case CS_EVENT_PM_RESUME:
			link->state &= ~DEV_SUSPEND;
			/* Fall through... */
				
		case CS_EVENT_CARD_RESET:
			if (link->state & DEV_CONFIG) {
				CardServices(RequestConfiguration, link->handle, &link->conf);
			}
			((local_info_t *)link->priv)->stop = 0;		
			break;
	}
	return 0;
}

status_t ne2k_pcmcia_init_driver(void) {
	servinfo_t serv;

	ETHER_DEBUG( gDevID, FUNCTION, "ne2k_pcmcia_init_driver ENTRY\n");
	
	if (get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs) != B_OK) goto fail1;
	if (get_module(DS_MODULE_NAME, (struct module_info **)&ds) != B_OK) goto fail2;

	CardServices(GetCardServicesInfo, &serv);
	if (serv.Revision != CS_RELEASE_CODE) goto fail3;

	/* When this is called, Driver Services will "attach" this driver
	   to any already-present cards that are bound appropriately. */
	register_pccard_driver(&dev_info, &ne2k_pcmcia_attach, &ne2k_pcmcia_detach);

	ETHER_DEBUG( gDevID, FUNCTION, "ne2k_pcmcia_init_driver EXIT (okay)\n");

	return B_OK;
	
fail3:	
	put_module(DS_MODULE_NAME);
fail2:
	put_module(CS_CLIENT_MODULE_NAME);
fail1:
	ETHER_DEBUG( gDevID, FUNCTION, "ne2k_pcmcia_init_driver EXIT (bad)\n");

	return B_ERROR;
}

void ne2k_pcmcia_uninit_driver(void) {
	int i;
	if(cs && ds){
		unregister_pccard_driver(&dev_info);
		for (i = 0; i < MAX_CARDS; i++)
		if (gDev[i].devLink) {
		if (gDev[i].devLink->state & DEV_CONFIG)
			ne2k_pcmcia_release((u_long)gDev[i].devLink);
			ne2k_pcmcia_detach(gDev[i].devLink);
		}
		put_module(DS_MODULE_NAME);
		put_module(CS_CLIENT_MODULE_NAME);
	}
}


#ifdef __ZRECOVER
# include <recovery/driver_registry.h>
fixed_driver_info ne2k_driver=
{
 	"NE2000 driver",
	B_CUR_DRIVER_API_VERSION,
	init_hardware,
	publish_devices,
	find_device,
	init_driver,
	uninit_driver
};
REGISTER_STATIC_DRIVER(ne2k_driver);
#endif
