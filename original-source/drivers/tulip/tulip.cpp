//depot/main/src/drivers/tulip/tulip.cpp#72 - edit change 40193 (text)
/*
 * tulip.cpp
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 *
 * DEC21040/21041/21140 PCI (tulip) ethernet driver
 *
 * 17 jun 96	bat		added 21040 support
 * 03 jun 96	bat		new today
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
#include <module.h>
#include <ByteOrder.h>


#define kDevName "tulip"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4			/* maximum number of driver instances */

/* debug flags */
enum debug_flags {
    ERR       = 0x0001,
    INFO      = 0x0002,
    RX        = 0x0004,    /* dump received frames */
    TX        = 0x0008,    /* dump transmitted frames */
    INTERRUPT = 0x0010,    /* interrupt calls */
    FUNCTION  = 0x0020,    /* function calls */
    PCI_IO    = 0x0040,    /* pci reads and writes */
    SEQ		  = 0x0080,    /* trasnmit & receive TCP/IP sequence sequence numbers */
    WARN	  = 0x0100,    /* Warnings - off on final release */
};
/* diagnostic debug flags - compile in here or set while running with debugger "AcmeRoadRunner" command */
#define DEFAULT_DEBUG_FLAGS ( ERR | INFO )
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN )

void ETHER_DEBUG(int32 debug_mask, int32 enabled, char * format, ...) {
	if (debug_mask & enabled) {
		va_list		args;
		char		s[4096];
		va_start(args, format);
		vsprintf( s, format, args );
		va_end(args);
		dprintf("%s",s);
	}
}

static pci_module_info		*gPCIModInfo;
static char 				*gDevNameList[MAX_CARDS+1];
static pci_info 			*gDevList[MAX_CARDS];
static int32 				gDevState[MAX_CARDS];
sem_id						gDevTxSem[MAX_CARDS];
sem_id						gDevRxSem[MAX_CARDS];
/* dev state flags */
enum DevState {
	DevOpen = 0x1,
	DevHardwarePresent = 0x2
};

static void free_devNameList(void) {

	char * item;
	int i;
	
	for (i=0; (item=gDevNameList[i]); i++)
		free(item);

}

const char** publish_devices(void ) {
	int i, j=0;
	char			devName[64];
	
	dprintf(kDevName ": publish_devices()\n" );

	free_devNameList();	
	
	/* Create device name list*/
	for (i=0; i<MAX_CARDS; i++ )
	{
		if (gDevState[i] & DevHardwarePresent) {
			
			sprintf(devName, "%s%ld", kDevDir, i );
			gDevNameList[j] = strdup(devName);
			if (gDevNameList[j] == NULL) {
				/* really out of memory */
				break;
			}
			j++;
		}
	}
	gDevNameList[j] = NULL;
	return (const char **)gDevNameList;
}

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


}

/*
 * Name them min_c and max_c to not conflict with
 * the C++ definitions of min and max. Metrowerks
 * does not want us to name them Min and Max either.
 */
#ifndef min_c
#define min_c(a,b) ((a)>(b)?(b):(a))
#define max_c(a,b) ((a)>(b)?(a):(b))
#endif /* min_c */

/*
 * Maximum count for avoiding infinite loops
 */
#define MAXBOGUS_ROM		1000000
#define MAXBOGUS_INTERRUPT	1000000
#define MAXBOGUS_RING		1000000

//#define RX_RING_SIZE 0x4	/* small sizes for testing... rx ring size *must* be a power of 2 */
//#define RX_RING_MASK 0x3 	/* ring size - 1 */
#define RX_RING_SIZE 0x20	/* rx ring size *must* be a power of 2 */
#define RX_RING_MASK 0x1F 	/* ring size - 1 */
#define TX_RING_SIZE 0x20 	/* must be a power of 2 - number of tx packets */
#define TX_RING_MASK 0x1F 	/* must be TX_RING_SIZE -1 */

#define MAX_RING_SIZE max_c(RX_RING_SIZE, TX_RING_SIZE)

#define MAX_FRAME_SIZE		1514		/* 1514 + 4 bytes checksum added by card hardware */


#define PACKET_BUF_SIZE		1536		/* our max packet size: even number */
#define SETUP_FRAME_SIZE	192			/* size of setup frame */
#define TULIP_ROM_SIZE 		128			/* size of rom (21041 only) */
#define ANY_BUS 			-1			/* any bus, any device, any func */

/*
 * PCI information
 */
#define VENDOR_DEC		0x1011	/* DEC PCI vendor id */
#define DEC_21040		0x0002	/* 10 Mbit */
#define DEC_21041		0x0014	/* enhanced 10Mbit */
#define DEC_21140		0x0009	/* 10/100 */
#define DEC_21143		0x0019	/* 10/100 auto neg */
#define DEC_21145		0x0039	/* phone lan */
#define VENDOR_INTEL	0x8086
#define VENDOR_LITEON	0x11ad
#define DEV_PNIC		0x0002	
#define DEV_PNIC_II		0xc115	


/* If you're adding a new vendor and device, be sure that hash_dev() doesn't
	collide with an existing table entry. */
enum dev_tbl_index { DEC21040=1, DEC21140=2,DEC21041=4,DEC21143=6,PNIC=9,PNIC_II=0xC, DEC21145=0xE };
static struct vendor_device_info {
	const char *name;
	const unsigned long	vendor_id, device_id, srom_size, csr7_int_mask, csr5_int_clear; /* dev_hash */
} chip_stuff[] = {
	" null",		0,				0,			0,		0,				0,				/* 0 */
	"Dec21040", 	VENDOR_DEC, 	DEC_21040, 	128, 	0x0001ebef,		0x0001ffff,		/* 1 */
	"Dec21140", 	VENDOR_DEC, 	DEC_21140, 	128, 	0x0001ebef,		0x0001ffff,		/* 2 */
	" null ",		0,				0,			0,		0,				0,				/* 3 */
	"Dec21041", 	VENDOR_DEC, 	DEC_21041, 	128, 	0x0001ebef,		0x0001ffff,		/* 4 */
	" null ",		0,				0,			0,		0,				0,				/* 5 */
	"Dec21142/3", 	VENDOR_DEC, 	DEC_21143, 	128, 	0x0801fbff,		0x0C01ffff,		/* 6 */
	" null ",		0,				0,			0,		0,				0,				/* 7 */
	" null ",		0,				0,			0,		0,				0,				/* 8 */
	"LiteOn",		VENDOR_LITEON, 	DEV_PNIC, 	256,	0x0001ebef,		0x0001ffff,		/* 9 */
	" null ",		0,				0,			0,		0,				0,				/* a */
	" null ",		0,				0,			0,		0,				0,				/* b */
	"LiteOnII",		VENDOR_LITEON, 	DEV_PNIC_II, 256,	0x0001ebef,		0x1801ffff,		/* c */
	" null ",		0,				0,			0,		0,				0,				/* d */
	"Dec21145",		VENDOR_INTEL,	DEC_21145,	128,	0x1801fbff,		0x1C01ffff,		/* e */
	" null ",		0,				0,			0,		0,				0				/* f */
};
unsigned char hash_dev(const unsigned int vendor, const unsigned int dev) {
	return ( (((vendor & 0x80) >> 4) | ((dev & 0x18) >> 2) | ((dev & 2) >> 1)) & 0xF);
}

/*
 * CS registers (for 21040)
 */
typedef enum csreg {
	CSR0_BUS_MODE	= 	0x00,		/* bus mode register */
	CSR1_TX_POLL	=	0x08,		/* cause chip to poll for tx register */
	CSR2_RX_POLL	=	0x10,		/* cause chip to poll for rx register */
	CSR3_RX_LIST 	=	0x18,		/* set rx descriptor list register */
	CSR4_TX_LIST 	=	0x20,		/* set tx descriptor list register */
	CSR5_STATUS  	=	0x28,		/* status register */
	CSR6_OP_MODE	=	0x30,		/* operation mode register */
	CSR7_INTR_MASK	=	0x38,		/* interrupt mask register */
	CSR8_MISS_FRAME =	0x40,		/* missed frames */
	CSR9_ETHER_ROM 	=	0x48,		/* ethernet rom register */
	CSR10_RESERVED	=	0x50,		/* boot rom address */
	CSR11_FULL_DUPLEX	=	0x58,	/* full duplex register */
	CSR12_SIA_STATUS =	0x60,		/* serial connection register */
	CSR13_SIA_CONN	=	0x68,		/* serial connection register */
	CSR14_SIA_TX_RX	=	0x70,
	CSR15_SIA_GEN	=	0x78
} csreg_t;

/* the short version */
enum csreg_short {
	CSR0	= 	0x00,		/* bus mode register */
	CSR1	=	0x08,		/* cause chip to poll for tx register */
	CSR2	=	0x10,		/* cause chip to poll for rx register */
	CSR3 	=	0x18,		/* set rx descriptor list register */
	CSR4 	=	0x20,		/* set tx descriptor list register */
	CSR5  	=	0x28,		/* status register */
	CSR6	=	0x30,		/* operation mode register */
	CSR7	=	0x38,		/* interrupt mask register */
	CSR8	=	0x40,		/* missed frames */
	CSR9 	=	0x48,		/* ethernet rom register */
	CSR10	=	0x50,		/* boot rom address */
	CSR11	=	0x58,		/* full duplex register or timer */
	CSR12	=	0x60,		/* serial connection register or gp port*/
	CSR13	=	0x68,		/* serial connection register */
	CSR14	=	0x70,
	CSR15	=	0x78,		/* watchdog timer or gp port */
	CSR19	=	0x98,		/* lite-on */
	CSR23	=	0xb8		/* lite-on phy */

};



/*
 * 21041 diffs with 21040
 */
#define CSR11_GP_TIMER		CSR11_FULL_DUPLEX


/*
 * 21140 diffs with 21041
 */
#define CSR12_GP_PORT		CSR12_SIA_STATUS
#define CSR15_WATCHDOG		CSR15_SIA_GEN


/*
 * Bits for various registers
 */
#define CSR0_reset 		(1 << 0)	/* reset the chip */
#define CSR0_burst1		(1 << 8)	/* burst length = 1 longwords */
#define CSR0_burst2		(2 << 8)	/* burst length = 2 longwords */
#define CSR0_burst4		(4 << 8)	/* burst length = 4 longwords */
#define CSR0_burst8		(8 << 8)	/* burst length = 8 longwords */

#define CSR1_transmit	0x00000000	/* can be anything */

#define CSR2_receive	0x00000000	/* can be anything */

/*
 * CSR5 macros
 */
#define CSR5_error_bits(x)	(((x) >> 23) & 0x7)	/* retrieve error bits */
#define CSR5_tx_state(x)	(((x) >> 20) & 0x7)	/* retrieve tx state */
#define CSR5_rx_state(x)	(((x) >> 17) & 0x7)	/* retrieve rx state */

/*
 * Possible rx states contained in CSR5
 */
#define CSR5_rx_stopped		0x0
#define CSR5_rx_run_fetch	0x1
#define CSR5_rx_run_check	0x2
#define CSR5_rx_run_wait	0x3
#define CSR5_rx_suspended	0x4
#define CSR5_rx_run_close	0x5
#define CSR5_rx_run_flush	0x6
#define CSR5_rx_run_queue	0x7

/*
 * Possible tx states contained in CSR5
 */
#define CSR5_tx_stopped		0x0
#define CSR5_tx_run_fetch	0x1
#define CSR5_tx_run_wait	0x2
#define CSR5_tx_run_read	0x3
#define CSR5_tx_reserved	0x4
#define CSR5_tx_run_setup	0x5
#define CSR5_tx_suspended	0x6
#define CSR5_tx_run_close	0x7

/*
 * CSR5 bits
 */

#define CSR5_normal			(1 << 16)	/* normal interrupt summary */
#define CSR5_abnormal		(1 << 15)	/* abnormal interrupt summary */
#define CSR5_system_error	(1 << 13)	/* system error */
#define CSR5_link_fail		(1 << 12)	/* link fail */
#define CSR5_full_duplex	(1 << 11)	/* full-duplex */
#define CSR5_aui			(1 << 10)	/* aui pin switch */
#define CSR5_rx_watchdog	(1 << 9)	/* rx watchdog timeout */
#define CSR5_rx_stop		(1 << 8)	/* rx stopped */
#define CSR5_rx_buf			(1 << 7)	/* rx buffer unavaible */
#define CSR5_rx_intr		(1 << 6)	/* rx interrupt */
#define CSR5_tx_underflow	(1 << 5)	/* tx buffer underflow */
#define CSR5_link			(1 << 4) 	/* link is up */
#define CSR5_tx_jabber		(1 << 3)	/* tx jabber timeout */
#define CSR5_tx_buf			(1 << 2)	/* tx buffer unavailable */
#define CSR5_tx_stop		(1 << 1)	/* tx stopped */
#define CSR5_tx_intr		(1 << 0)	/* tx interrupt */

/*
 * CSR6 bits
 */
#define CSR6_rx_start		(1 << 1)	/* start receiving mode */
#define CSR6_backoff		(1 << 5)
#define CSR6_promisc 		(1 << 6)	/* set promiscious mode */
#define CSR6_tx_start		(1 << 13)	/* start transmitting mode */
#define CSR6_thresh_lo		(1 << 14)
#define CSR6_thresh_hi		(1 << 15)
#define CSR6_capture		(1 << 17)
#define CSR6_port_sel		(1 << 18)	/* port select */
#define CSR6_scrambler		(1 << 24)
#define CSR6_tx_thresh		(1 << 22)
#define CSR6_pcs			(1 << 23)
#define CSR6_must_be_one	(1 << 25)

#define CSR6_always_flags	(CSR6_must_be_one|CSR6_capture|CSR6_backoff|\
							 CSR6_tx_thresh|CSR6_thresh_hi)

#define CSR6_10_flags (CSR6_tx_thresh|CSR6_thresh_hi|CSR6_always_flags)

#define CSR6_100_flags (CSR6_scrambler|CSR6_pcs|CSR6_port_sel|CSR6_thresh_hi|\
						CSR6_always_flags)

/*
 * CSR7 bits
 */
#define CSR7_normal			(1 << 16)	/* normal interrupt */
#define CSR7_abnormal		(1 << 15)	/* abnormal interrupt */
#define CSR7_system_err 	(1 << 13)	/* system errors */
#define CSR7_link_fail  	(1 << 12)	/* link fail */
#define CSR7_full_duplex	(1 << 11)	/* full duplex */
#define CSR7_aui			(1 << 10)	/* aui/tp switch */
#define CSR7_watchdog		(1 << 9) 	/* receive watchdog */
#define CSR7_rx_stopped		(1 << 8)	/* rx stopped */
#define CSR7_rx_buf			(1 << 7)	/* rx buffer unavailable */
#define CSR7_rx_int			(1 << 6)	/* packet received */
#define CSR7_underflow		(1 << 5)	/* transmit underflow */
#define CSR7_tx_jabber		(1 << 3)	/* tx jabber timeout */
#define CSR7_tx_buffer		(1 << 2)	/* tx buffer unavailable */
#define CSR7_tx_stopped 	(1 << 1)	/* tx stopped */
#define CSR7_tx_int			(1 << 0)	/* tx int */
#define CSR7_mask_all		0xffffffff	/* catch all interrupts */

/*
 * CSR9 bits (valid for 21041 only)
 */
#define CSR9_21040_mode		(1 << 15)	/* operates in 21040 mode? */
#define CSR9_read			(1 << 14)	/* read from rom */
#define CSR9_write			(1 << 13)	/* write to rom */
#define CSR9_srom			(1 << 11)	/* select serial rom */
#define CSR9_data_out		(1 << 3)	/* output data from rom */
#define CSR9_data_in		(1 << 2)	/* input data to rom */
#define CSR9_clock			(1 << 1)	/* clock */
#define CSR9_cs				(1 << 0)	/* chip select */

/*
 * CSR13 bits
 */
#define CSR13_reset			0			/* reset */
#define CSR13_auto_config	(1 << 2)	/* auto config */
#define CSR13_auibnc_config	(1 << 3)	/* aui/bnc config */
#define CSR13_21040_config	0x00000004	/* config for 21040 */
#define CSR13_21041_config	0x0000ef01	/* config for 21041 */

/*
 * CSR14 bits
 */
#define CSR14_reset			0
#define CSR14_auibnc		0xf73d
#define CSR14_10baset		0xff3f
/*
 * CSR15 bits
 */
#define CSR15_reset			0		/* defaults to 10baseT */
#define CSR15_bnc			0x6		/* bnc */
#define CSR15_aui			0xe		/* aui */

/* 
 * Descripter values
 */
#define TDES1_interrupt		(1 << 31)	/* transmit: generate interrupt */
#define TDES1_last			(1 << 30)	/* transmit: have first buffer */
#define TDES1_first			(1 << 29)	/* transmit: have second buffer */
#define TDES1_setup_packet	(1 << 27)	/* transmit: setup packet */
#define RTDES1_end_of_ring	(1 << 25)	/* receive, transmit: end of ring */

#define RTDES0_own_card		(1 << 31)	/* rx, tx: card owns buffer */
#define RTDES0_error		(1 << 15)	/* rx, tx: error */
#define RDES0_last			(1 << 8)	/* rx: last descriptor */
#define RDES0_dribbling		(1 << 2)	/* rx: dribbling bits in packet */

/*
 * Serial rom values
 */
#define SROM_CMD_SHIFT 		6			/* how much to shift SROM cmd */
#define SROM_CMD_READ		6			/* code for SROM read */
#define SROM_CMD_NBITS		11			/* total size of cmd (incl. addr) */

/*
 * For detecting a Znyx card: check the first three address bytes (hack!)
 */
#define ZNYX_ADDR0 0x00
#define ZNYX_ADDR1 0xc0
#define ZNYX_ADDR2 0x95


/*
 * Structure of tulip ring descriptor: as defined by hardware
 */
typedef struct tulip_ring {
	ulong status;		/* little-endian */
	ulong length;		/* little-endian */
	ulong buf1;			/* little-endian */
	ulong buf2;			/* little-endian */
} tulip_ring_t;

/*
 * Packet buffer: aligned on long boundary
 */
typedef union tulip_vbuf {
	long ldata[PACKET_BUF_SIZE / sizeof(long)];
	char data[PACKET_BUF_SIZE];
} tulip_vbuf_t;

/*
 * Physical mappings
 */
typedef struct tulip_pbuf {
	char *first;
	int firstlen;
	char *second;		/* if crosses a page boundary, non-contiguous phys */
	int secondlen;		/* if crosses a page boundary, non-contiguous phys */
} tulip_pbuf_t;

typedef enum _media_t {
	MEDIA_10BASET = 0,
	MEDIA_BNC = 1,
	MEDIA_AUI = 2,
	MEDIA_100BASET = 3
} media_xxx_t;

/* media types as per link code word capapbilites */
static const unsigned long HD10BaseT = 0x20;  
static const unsigned long FD10BaseT = 0x40;  
static const unsigned long HD100BaseTX = 0x80;  
static const unsigned long FD100BaseTX = 0x100;  




#define MAX_MULTI 14 	/* max number of multicast addresses */

#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))



/* For Parsing Srom and setting up the media */

#define MEDIA_LIST_SIZE 7
#define CMD_LIST_SIZE 7

typedef struct chip_command_s {
	char rw;
	unsigned char reg;
	unsigned long command;
	unsigned long delay;
} chip_commands_t;	


enum media_flags {
	CSR13TO15=1L,GP=2L,PHYRESET=4L,CAP=8L,
	NWAY=0x10L,DUP=0x20L,THRESH=0x40L,MINT=0x80L,
	CSR6INFO=0x100L, CSR12_21140=0x200L, NO_MEDIA_INFO=0x8000L,
	MII_PORT=0x10000, SYM_PORT=0x20000,
	TenBaseT_PORT=0x40000, AUI_PORT=0x80000, BNC_PORT=0x100000
};

typedef struct media_s {
	unsigned char leaf_type;
	unsigned char media_code;
	unsigned long media_flags;
	chip_commands_t csr13to15[CMD_LIST_SIZE];
	unsigned char gpCmds;
	unsigned char csr12_21140;
	chip_commands_t gp[CMD_LIST_SIZE];
	unsigned char PhyResetCmds;
	chip_commands_t PhyReset[CMD_LIST_SIZE];
	unsigned short cap, nway, dup, thresh, mint, csr6info;
} media_t;

enum chip_rw { FREE=0, HW_READ=1, HW_WRITE=2};
#define NO_DELAY 0
enum duplex { HALF=0, FULL=1 };





/* 
 * Our private data 
 */

static char pci_name[] = B_PCI_MODULE_NAME;

typedef struct tulip_private {
	/* rings buffers - declare first - BeBox requires 8 byte DMA alignment.  */
	volatile tulip_ring_t rx_ring[RX_RING_SIZE];
	volatile tulip_ring_t tx_ring[TX_RING_SIZE];

	/* Ring physical mappings */
	tulip_ring_t *rx_pring;	
	tulip_ring_t *tx_pring;

	/* Buffer physical mappings */
	tulip_pbuf_t rx_pbuf[RX_RING_SIZE];
	tulip_pbuf_t tx_pbuf[TX_RING_SIZE];

	/* virtual buffers: at end so all the beginning stuff is on one page */
	tulip_vbuf_t rx_vbuf[RX_RING_SIZE];
	tulip_vbuf_t tx_vbuf[TX_RING_SIZE];

	int32			devID; // device identifier: 0-n
	pci_info		*pciInfo;

	ether_address_t myaddr;	/* my address */
	char init;		/* init flag */
	char irq;		/* irq assigned to us by bios */
	char _pad1;
	unsigned int device_id;	/* 21040, 21041, 21140, 21143 */
	unsigned int vendor_id;	/* dec, liteon, ... */
	unsigned char dev_table_index; /* our local hash index */
	
	int32 readLock, writeLock;		/* reentrant reads/writes not allowed */

	area_id area;	/* area we use for this data */
	int blockFlag; 	/* nonblocking mode */
	volatile uint32 tx_ringx;	/* pointer to available tx slot */
	volatile uint32 tx_unack;	/* pointer to unacknoledged tx slot */

	volatile ulong rx_ringx;	/* pointer to next available rx packet */
	volatile ulong rx_acked;	/* pointer to last available rx packet */ 
	int interrupts;	/* interrupts enabled? */
	unsigned long csr5_int_clear;
	unsigned long csr7_int_mask;
	media_xxx_t media_xxx;	/* media type */

	area_id ioarea;	/* area mapping device registers */
	volatile ulong *ioaddr;		/* -> mapped device registers */

	sem_id rx_sem;	/* blocking on received */
	sem_id tx_sem;	/* blocking on transmit */
	sem_id snooze_sem;	/* semaphore to acquire when snoozing for media check*/

	ether_address_t multi[MAX_MULTI];
	unsigned nmulti;
	unsigned suspended;
	
	unsigned long csr6; 		/* saved OP Mode register value */
	uint32 	debug;				/* debug level 0-none, 1-err, 2=info, 3=xmit, 4=rcv */
	unsigned short selected_media;
	unsigned short LinkCodeWord;
	unsigned char chip_timer;	/* general purpose on chip timer, true = popped */
	unsigned char is_mii;		/* boolean - uses MII port */
	unsigned char mii_phy_id;	/* which one */
	media_t * phy_reset_media;	/* save the reset info, you'll need it */
/* config and media information */
#define EEPROM_SIZE 128
	unsigned char eeprom[EEPROM_SIZE];			/* Serial EEPROM contents. */
	unsigned char media_index, media_count;
	unsigned short default_media;
	media_t media[MEDIA_LIST_SIZE];
	uint16 linkStatus;			/* link is up or down */
	uint32 promisc_mode;

} tulip_private_t;

/* for serial debug */
#define ETHER_SERIAL_DEBUG 1
#if ETHER_SERIAL_DEBUG
static int tulip(int argc, char **argv);
tulip_private_t * gdev;
#endif  //ETHER_SERIAL_DEBUG


/* prototypes */
static ulong tulip_inw(tulip_private_t *data, int reg);
static void tulip_outw(tulip_private_t *data, int reg, ulong val);
static long  read_srom( tulip_private_t *data, unsigned char *buf /*, SIZE */ );
static void dump_packet(const char * msg, unsigned char * buf);
static status_t tulip_open(const char *, uint32, void **);
static status_t tulip_close(void *);
static status_t tulip_free(void *);
static status_t tulip_control(void *,uint32,void *,size_t);
static status_t tulip_read(void *,off_t, void *,size_t *);
static status_t tulip_write(void *, off_t, const void *, size_t *);
static status_t tulip_writev( void *cookie, off_t position, const iovec *vec, size_t count, size_t *length );
static int32    tulip_interrupt(void *);                	   /* interrupt handler */
static void    	pci_probe(void);  /* Get pci_info for each device */
static status_t match_pci_info(pci_info *item,uint32 * devIDptr);
static void LinkChange(tulip_private_t *data);
static void setpromisc(tulip_private_t *data, uint32 on);


static device_hooks gDeviceHooks =  {
	tulip_open, 			/* -> open entry point */
	tulip_close,         /* -> close entry point */
	tulip_free,          /* -> free entry point */
	tulip_control,       /* -> control entry point */
	tulip_read,          /* -> read entry point */
	tulip_write,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,                /* -> deselect entry point */
	NULL,               /* -> readv */
	tulip_writev        /* -> writev */
};



/* Driver Entry Points */
status_t
init_hardware (void)
{
	return B_NO_ERROR;
}

#include <cb_enabler.h>

cb_enabler_module_info *cb = NULL;

cb_device_descriptor cb_descrs[] = {
	{ VENDOR_DEC, DEC_21143, 0xff, 0xff, 0xff }
};

static status_t
cb_device_added(pci_info *dev, void **cookie)
{

	dprintf("tulip: device added\n");
	return match_pci_info(dev, (uint32 *)cookie);
}

static void
cb_device_removed(void *cookie)
{
	uint32 devID = (uint32) cookie;
	int32 previous_state;
	
	dprintf("tulip: device %d removed\n", cookie);
	if (devID >= MAX_CARDS) {
		dprintf("cb_device_removed: bad devID %x\n", devID);
		return;
	}
	previous_state = atomic_and(&gDevState[devID],~DevHardwarePresent);
	if ((previous_state & DevOpen) == 0) {
		free(gDevList[devID]);
		gDevList[devID] = NULL;
	} else {
		/* Device is open - hardware is gone: unblock read or writes waiting on semaphores */
		delete_sem(gDevTxSem[devID]);
		//delete_sem(gDevRxSem[devID]);
	}
}


cb_notify_hooks notify_hooks =
{
	cb_device_added,
	cb_device_removed,
};


status_t init_driver(void)
{
	status_t 		status;
	int32			entries;
	int32			i;
	
	dprintf(kDevName ": init_driver ");	

	for (i=0; i<MAX_CARDS; i++) {
		gDevList[i]=NULL;
		gDevState[i] = 0;
		gDevTxSem[i] = -1;
		gDevRxSem[i] = -1;

	}
	
	gDevNameList[0] = NULL;

	if((status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		dprintf(kDevName " Get module failed! %s\n", strerror(status ));
		return status;
	}

	/* Find Lan cards*/
	pci_probe();

	
	if(get_module(CB_ENABLER_MODULE_NAME, (module_info**) &cb) != B_OK) {
		cb = NULL;
	} else {
		cb->register_driver("tulip", cb_descrs, 1);
		cb->install_notify("tulip", &notify_hooks);
	}
			
	return B_OK;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;


	free_devNameList();
		
	/*Free device list*/
	for (i=0; i<MAX_CARDS; i++)
		free(gDevList[i]);

	put_module(B_PCI_MODULE_NAME);
	
	if(cb) {
		cb->uninstall_notify("tulip", &notify_hooks);
		put_module(CB_ENABLER_MODULE_NAME);
	}
}


device_hooks *find_device(const char *name)
{
	return &gDeviceHooks;
}



static status_t match_pci_info(pci_info * item, uint32 *devIDptr) {

	pci_info *new_item;
	uint16 free_slot_index;
	int32 previous_state;
	
	if (((item->vendor_id == VENDOR_DEC) 	&&	(item->device_id == DEC_21040))	 ||
		((item->vendor_id == VENDOR_DEC) 	&&	(item->device_id == DEC_21041))	 ||
		((item->vendor_id == VENDOR_DEC) 	&&	(item->device_id == DEC_21140))	 ||
		((item->vendor_id == VENDOR_DEC) 	&&	(item->device_id == DEC_21143))	 ||
		((item->vendor_id == VENDOR_LITEON) &&	(item->device_id == DEV_PNIC))	 ||
		((item->vendor_id == VENDOR_LITEON) &&	(item->device_id == DEV_PNIC_II))||
		((item->vendor_id == VENDOR_INTEL)  &&	(item->device_id == DEC_21145))) {

		/* check if the device really has an IRQ */
		if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
			dprintf(kDevName " found with invalid IRQ - check IRQ assignement");
			return B_ERROR;
		}
		dprintf(kDevName " %s found at IRQ %x ",
			chip_stuff[hash_dev(item->vendor_id, item->device_id)].name,
			item->u.h0.interrupt_line);	
		
		for (free_slot_index=0; free_slot_index < MAX_CARDS; free_slot_index++) {
			if (gDevList[free_slot_index] == NULL) {
				new_item = (pci_info *)malloc(sizeof(pci_info));
				if (new_item == NULL) {
					return B_NO_MEMORY;
				}
				memcpy(new_item, item, sizeof(pci_info));
				gDevList[free_slot_index] = new_item;
				previous_state = atomic_or(&gDevState[free_slot_index], DevHardwarePresent);
				if (previous_state) {
					dprintf("State warning: %s %s\n", (previous_state & DevOpen)?"Device Already Opened":"",
						(previous_state & DevHardwarePresent)?"Device Hardware Already Present":"");
				}
				if (devIDptr != NULL)
					*devIDptr = free_slot_index;
				return B_OK;
			}		
		}
		dprintf(kDevName ": driver instance exceeds MAX_CARDS %d\n",MAX_CARDS);
	}
	return B_ERROR;
}


static void pci_probe(void)
{
	pci_info	*item;
	int i=0;
	
	item = (pci_info *)malloc(sizeof(pci_info));	
	if (item == NULL) {
		dprintf(kDevName ": pci_probe: failed: no memory\n");
		return;
	}

	while ((gPCIModInfo->get_nth_pci_info(i++, item)) == B_OK) {
		match_pci_info(item,NULL);
	}
	
	free(item);
}

/* 
 * DEC official test pattern: for checking rom
 */
static const char TESTPAT[] = { 0xFF, 0, 0x55, 0xAA, 0xFF, 0, 0x55, 0xAA };

/*
 * Ethernet broadcast address
 */
static const ether_address_t BROADCAST_ADDR = 
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
 * Will work with any compiler
 */
static const union { long l; char b[4]; } ENDIAN_TEST = { 1 };
#define LITTLE_ENDIAN_HARDWARE   ENDIAN_TEST.b[0]

static inline ushort SWAPSHORT(ushort x)
{
	return ((x << 8) | (x >> 8));
}

static inline ulong SWAPLONG(ulong x)
{
	return ((SWAPSHORT(x) << 16) | (SWAPSHORT(x >> 16)));
}

/*
 * swap short, if necessary
 */
static inline ulong swapshort(ushort x) {
	return (LITTLE_ENDIAN_HARDWARE ? x : SWAPSHORT(x));
}

/*
 * swap long, if necessary
 */
static inline ulong swaplong(ulong x) {
	return (LITTLE_ENDIAN_HARDWARE ? x : SWAPLONG(x));
}

/*
 * Modulo addition, useful for ring descriptors
 */
static inline unsigned mod_add(unsigned x, unsigned y, unsigned modulus) {
	return ((x + y) % modulus);
}




static void init_cmd_list(chip_commands_t cmd[]) {
	char j;
	
	for (j=0; j< CMD_LIST_SIZE; j++) {
		cmd[j].rw	= 0;	
	}
}

static void add_cmd(chip_commands_t * cmd, char rw, unsigned char reg, unsigned long command, unsigned long delay ) {
	char j, done = false;
	
	for (j=0; j<CMD_LIST_SIZE; j++) {
		if (cmd[j].rw == 0) {
			cmd[j].rw=rw; cmd[j].reg=reg; cmd[j].command=command; cmd[j].delay=delay;
			done = true;
			break;
		}
	}
	if (!done) dprintf("add_cmd: list full - this should never happen!\n");
};

static void execute_cmds(tulip_private_t *data, chip_commands_t cmd[]) {
	char j; char done=false;
	
	for (j=0; j<CMD_LIST_SIZE; j++) {
		switch (cmd[j].rw) {
		case HW_READ:
			cmd[j].command = tulip_inw(data, cmd[j].reg);
			if (cmd[j].delay) {}; /* execute delay */
			break;
		case HW_WRITE:
			ETHER_DEBUG(WARN, data->debug, "execute_cmds: write %x to csr %d\n",cmd[j].command, cmd[j].reg/8);
			tulip_outw(data, cmd[j].reg, cmd[j].command);
			if (cmd[j].delay) {}; /* execute delay */
			break;
		default:
			done=true;
		}
		if (done) break;
	}
};

#if 0
static long check_vendor(tulip_private_t *data) {
	/* rather than crash trying to parse some strange srom,
	   check the vendor part of the ethernet address */
	

	
		if ((data->myaddr.ebyte[0] == 0x00) &&		/* farallon */
			(data->myaddr.ebyte[1] == 0x00) &&
			(data->myaddr.ebyte[2] == 0xC5)) {
			ETHER_DEBUG(INFO, data->debug, "found Farallon\n");
			return 0;
		}
		if ((data->myaddr.ebyte[0] == 0x00) &&		/* hitachi */
			(data->myaddr.ebyte[1] == 0x00) &&
			(data->myaddr.ebyte[2] == 0xE1)) {
			ETHER_DEBUG(INFO, data->debug, "found Hitatchi\n");
			return 0;
		}
		if ((data->myaddr.ebyte[0] == 0x00) &&		/* addtron */
			(data->myaddr.ebyte[1] == 0x40) &&
			(data->myaddr.ebyte[2] == 0x33)) {
			ETHER_DEBUG(INFO, data->debug, "found Hitatchi\n");
			return 0;
		}
		ETHER_DEBUG(INFO, data->debug, "Unknown Vendor\n");
		return (B_IO_ERROR);
};
#endif

/* From "Digital Semiconductor 214X Serial Rom Format" appendix A */
unsigned short
CalcSromCrc(unsigned char *SromData, short DataSize) {
	const unsigned long POLY = 0x04c11db6L;
	unsigned long crc = 0xffffffff;
	unsigned long FlippedCRC=0;
	unsigned char CurrentByte;
	unsigned int Index, Bit, Msb;
	int i;
	
	for (Index = 0; Index < DataSize; Index++) {
		CurrentByte=SromData[Index];
		for (Bit=0;Bit<8;Bit++) {
			Msb=(crc>>31) &1;
			crc<<=1;
			if (Msb^(CurrentByte&1)) {
				crc^=POLY;
				crc|=0x00000001;
			}
			CurrentByte>>=1;
		}
	}
	for (i=0;i<32;i++) {
		FlippedCRC <<=1;
		Bit=crc&1;
		crc>>=1;
		FlippedCRC+=Bit;
	}
	crc = FlippedCRC ^ 0xFFFFFFFF;
	return (crc & 0xFFFF);
}


static const char * block_format[] = {"21140/A Non-MII", "21140/A MII",
 "21142/3 SIA Media", "21142/3 MII", "21143 SYM Media", "21142/3 General Purpose Reset"};

/* the order of media in the connection type table here is dependent on the hash function that
follows, don't change one without changing the other */
static const char * connection_type[] = {
	"10BaseT","10Base2(BNC)", "10Base5(AUI)", "100BaseTx(SYM)","10BaseT Full Duplex","100Base2Tx Full Duplex","100BaseT4","100BaseFx(Fiber)",
	"100BaseFx Full Duplex","MII10BaseT", "Powerup AutoSense","hash-error1" ,"hash-error2" ,"MII100BaseTx","hash-error3", "MII100BaseT4",
	"10BaseT NWay", "MII100BaseFx Full Duplex", "PhoneLan","hash-err4","hash-err5","100BaseTx Full Duplex","hash-err6","hash-err7",
	"Powerup Dynamic AutoSense","hash-error8","Mii 10BaseT FullDuplex","hash-error9","hash-error10","hash-error11","Mii 100BaseTx Full Duplex","No Media Info"
};

/* The hash function depends on the connection_type table above, and may need to be changed if new
   media are added by Dec - the hash function maps a four digit hex number down to 5 bits */
unsigned char hash_connection_type(unsigned short connection_type) {
	if (connection_type == 0x204) return(4);
	if (connection_type == 0x208) return(8);
	if (connection_type == 0x800) return(0x18);
	return (  ((connection_type & 0x1F) |
			 ((connection_type & 0x100) >> 4) |
			 ((connection_type & 0x200) >> 5) |
			 ((connection_type & 0x800) >> 8) |
			 ((connection_type & 0x8000) >>14)));

}



typedef struct leaf_s {
	unsigned char len;
	unsigned char type;
} leaf_t;

typedef struct block_21140_s {
        unsigned short connection_type;
        unsigned char general_purpose_control_21140;
        unsigned char leaf_count;
} block_21140_t;

typedef struct block_21143_s {
        unsigned short connection_type;
        unsigned char leaf_count;
} block_21143_t;
 
#define LEAF_INFO_OFFSET 27

/*  Parse the serial Rom for config and media settings.
    Dec/Intel publishes a document "214x Serial Rom Format" which describes
	the format.
*/
	
static long
parse_srom(tulip_private_t *data) {
	long err = B_IO_ERROR;
	unsigned char version;
	unsigned short i, j;

	block_21140_t *block;
	leaf_t *leaf;

	
	unsigned short eeprom_size = chip_stuff[data->dev_table_index].srom_size;

	if ((eeprom_size != 128) && (eeprom_size != 256)) {
		dprintf("tulip: bogus srom size %d",eeprom_size);
		return B_ERROR;
	}

	read_srom(data, data->eeprom /* , eeprom_size */ );

/* Leave dump of srom to serial debug on for now, there are a lot of them out there,
	and they vary quite a bit from vendor to vendor -rhs */
		dprintf("parse_eeprom: Dec Ethernet data = ");
		for (j=0; j<eeprom_size; j++) {
			if ((j & 0xf) == 0) dprintf("\n");
			dprintf(" 0x%2.2x,",data->eeprom[j]);
		}
		dprintf("\n");
	
	if ((data->vendor_id == VENDOR_DEC) || (data->vendor_id == VENDOR_INTEL)) {
		unsigned short	calculated, stored;
	
		version = data->eeprom[18];
		if (!((version == 0x01) | (version == 0x03)| (version == 0x04) | (version == 0xA2))) {
			ETHER_DEBUG(ERR, data->debug, "parse_srom: unknown srom format\n");
			return (B_ERROR);
		}

	 	/* check srom crc */
		calculated=CalcSromCrc(data->eeprom, eeprom_size-2);
        stored = data->eeprom[eeprom_size-2] | data->eeprom[eeprom_size-1] << 8;
		if (calculated != stored) {
			ETHER_DEBUG(ERR, data->debug, "eeprom crc error %x <> %x\n",calculated, stored);
//			return (err);  /* some compaq notebooks don't set the checksum correctly */
		}	
	}

	/* get the ethernet hardware address */ 
	for (i = 0x14, j=0; i < 0x1a; i++, j++) {
		data->myaddr.ebyte[j] = data->eeprom[i];
	}

	if ((data->vendor_id != VENDOR_DEC) && (data->vendor_id != VENDOR_INTEL)) return 0;
	
#if 0
	if (err = check_vendor(data)) {		/* we only support a few vendors so far */
		return (err);
	}
#endif
	/* for (each controller) ....*/
	if (data->eeprom[19] > 1) {		/* Multiport board. */
		ETHER_DEBUG(ERR, data->debug, "parse_srom: multiport board not supported yet.\n");
		return (err);
	}

	if ((data->eeprom[LEAF_INFO_OFFSET] == 0) | (data->eeprom[LEAF_INFO_OFFSET] == 0xff)) {
		ETHER_DEBUG(ERR, data->debug, "parse_srom: SROM format is strange\n");
		return(err);
	}

	data->media_index = 0;

	if ((data->dev_table_index == DEC21143) || (data->dev_table_index == DEC21140) || (data->dev_table_index == DEC21145) )
	{
		block = (block_21140_t *)  (((unsigned long) data->eeprom) +  data->eeprom[LEAF_INFO_OFFSET]);
		
		if (data->dev_table_index == DEC21140) {
			 leaf = (leaf_t *) ((unsigned long) block +  4 /*sizeof(block_21140_t) */);
			data->media_count = block->leaf_count;
			ETHER_DEBUG(WARN, data->debug, "Dec21140: gp_control %x - to do: use this value\n",block->general_purpose_control_21140);
		}
		if ((data->dev_table_index == DEC21143) || (data->dev_table_index == DEC21145)) {
			leaf = (leaf_t *) ((unsigned long) block + 3  /*sizeof(block_21143_t) */);
			/* the general_purpose_control field was removed from the middle of the block and 
			   moved into each leaf node as we go from the 21140 to the 21143 - here we use
			   the 21140 struct and fix it up for the 21143 */
				data->media_count = block->general_purpose_control_21140;
		}
		
		data->default_media = swapshort(block->connection_type);
		ETHER_DEBUG(INFO, data->debug, "parse_eeprom: default %x media %s with %d leaf media\n", data->default_media,
			connection_type[hash_connection_type(swapshort(block->connection_type))], data->media_count);
				
		for (data->media_index=0; data->media_index< data->media_count; data->media_index++) 
		{
			media_t * m = &data->media[data->media_index];
			unsigned char  *leaf_data = (unsigned char *)leaf;
			m->media_flags = 0;
			/* compact format? */
			if ((data->device_id == DEC_21140) && ((*leaf_data & 0x80) == 0) ) {
				m->media_code = *leaf_data++ & 0x3f;
				m->media_flags |= GP;
				init_cmd_list(m->gp);
				/* this is actually only 1 byte on the 21140 */
				add_cmd(m->gp, HW_WRITE, CSR15, *leaf_data++,NO_DELAY);		/*gp command*/	
				m->media_flags |= CSR6INFO;
				m->csr6info = swapshort(*(unsigned short *)leaf_data);
				leaf = (leaf_t *) (((unsigned long) leaf) + 4);
				continue;
			}
			/* extended format */
			leaf_data++; leaf_data++;
			switch (leaf->type) {
				case 2: {
					m->leaf_type = leaf->type;
					m->media_code = *leaf_data & 0x3f;
					ETHER_DEBUG(WARN, data->debug, "Leaf (2) media: %s ", connection_type[hash_connection_type(m->media_code)]);
					if (*leaf_data++ & 0x40) {
						m->media_flags |= CSR13TO15;
						init_cmd_list(m->csr13to15);
						add_cmd(m->csr13to15, HW_WRITE, CSR13, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
						add_cmd(m->csr13to15, HW_WRITE, CSR14, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
						add_cmd(m->csr13to15, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
					};
					/* add the general puprpose port control & data commands */
					m->media_flags |= GP;
					init_cmd_list(m->gp);
					add_cmd(m->gp, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data )<<16,NO_DELAY);		/*gp control*/	
					ETHER_DEBUG(WARN, data->debug, "GP control=%x", swapshort(*(unsigned short *)leaf_data)<<16);
					leaf_data++; leaf_data++;
					add_cmd(m->gp, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data)<<16,NO_DELAY);		/* gp data*/
					ETHER_DEBUG(INFO, data->debug, "GP data=%x \n", swapshort(*(unsigned short *)leaf_data)<<16);
					}
				break;
				case 1:	 /* for 21140 extended type 1 */ 
						m->csr12_21140 = block->general_purpose_control_21140;
						m->media_flags |= CSR12_21140;
						ETHER_DEBUG(WARN, data->debug, "Writing GP Control %x to CSR12\n", m->csr12_21140 | 0x100);
						tulip_outw(data,CSR12, m->csr12_21140 | 0x100);

				case 3:	 /* and 21143 extended type 3 */
				{
					unsigned char gpr_len, reset_len;
					data->is_mii = true;
					m->leaf_type = leaf->type;
					leaf_data++;	/* skip over phy number */
					m->media_flags |= ((gpr_len = *leaf_data++) ? GP : 0);  /* Get General Purpose Reset Sequence if present */
					init_cmd_list(m->gp);
					for (j=0; j< gpr_len; j++) {
						add_cmd(m->gp, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
					}
					
					m->media_flags |= ((reset_len = *leaf_data++) ? PHYRESET : 0);
					init_cmd_list(m->PhyReset);
					for (j=0; j< reset_len; j++) {
						add_cmd(m->PhyReset, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
					}
					
					m->media_flags |= ((m->cap = swapshort(*(unsigned short *) leaf_data)) ? CAP : 0);
					leaf_data++; leaf_data++;

					m->media_flags |= ((m->nway = swapshort(*(unsigned short *) leaf_data)) ? NWAY : 0);
					leaf_data++; leaf_data++;

					m->media_flags |= ((m->dup = swapshort(*(unsigned short *) leaf_data)) ? DUP : 0);
					leaf_data++; leaf_data++;

					m->media_flags |= ((m->thresh = swapshort(*(unsigned short *) leaf_data)) ? THRESH : 0);
					leaf_data++; leaf_data++;

					m->media_flags |= ((m->mint =  *leaf_data++) ? MINT : 0);

					if (data->debug  & WARN)  {
						dprintf("MII PHY media Leaf (%d) cap=%x nway=%x dup=%x thresh=%x mint=%x flags=%x ",
						 	leaf->type, m->cap, m->nway, m->dup, m->thresh, m->mint, m->media_flags);
						dprintf("= %s %s %s %s %s %s %s %s %s\n",
							m->media_flags & CSR13TO15 ? "CSR13-15 ":0,
							m->media_flags & GP ? "GP  ":0,
							m->media_flags & PHYRESET ? "PHYRESET ":0,
							m->media_flags & CAP ? "CAP ":0,
							m->media_flags & NWAY ? "NWAY ":0,
							m->media_flags & DUP ? "DUP ":0,
							(m->media_flags & THRESH) ? "THRESH ":0,
							(m->media_flags & MINT) ? "MINT ":0,
							(m->media_flags & CSR6INFO) ? "CSR6INFO ":0 );
					}
				}
				break;
				case 4:
					m->leaf_type = leaf->type;
					m->media_code = *leaf_data++ & 0x3f;
					ETHER_DEBUG(WARN, data->debug, "Leaf (4) SYM media: %s\n", connection_type[hash_connection_type(m->media_code)]);

					/* add the general puprpose port control & data commands */
					m->media_flags |= GP;
					init_cmd_list(m->gp);
					add_cmd(m->gp, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
					leaf_data++; leaf_data++;
					add_cmd(m->gp, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/* gp data*/
					leaf_data++; leaf_data++;
					m->media_flags |= CSR6INFO;
					m->csr6info = swapshort(*(unsigned short *)leaf_data);
					ETHER_DEBUG(WARN, data->debug, "csr6info = %x - check p.42 (7.5.2.1.3) of Dec Srom Doc\n",m->csr6info);
				break;
				case 5: {
					unsigned char j, reset_len;
					ETHER_DEBUG(WARN, data->debug, "Leaf (5) PHY RESET seq\n");
					m->leaf_type = leaf->type;
					m->media_flags |= ((reset_len = *leaf_data++) ? PHYRESET : 0);
					init_cmd_list(m->PhyReset);
					for (j=0; j< reset_len; j++) {
						add_cmd(m->PhyReset, HW_WRITE, CSR15, swapshort(*(unsigned short *)leaf_data),NO_DELAY);		/*gp control*/	
						leaf_data++; leaf_data++;
					}
					
				}
				break;
				case 7: {
					ETHER_DEBUG(INFO, data->debug, "Leaf (7) Undocumented, Ask Intel\n");
					m->leaf_type = leaf->type;
					
				}
				break;
				
				default: dprintf("parse_eeprom: parse error leaf type %x\n",leaf->type);		
			
			
			}	
			leaf = (leaf_t *) (((unsigned long) leaf) + (leaf->len & 0x7f) + 1);
			
		}	
	}
	return 0; /* no error */
}



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
#if TIMING && DEBUG
	sprintf(p, "%10.4f: ", system_time());
	p += strlen(p);
#endif /* TIMING && DEBUG */
	vsprintf(p, format, args);
	va_end(args);
	dprintf("%s", buf);
}


#define TRACE_IO 1
//#define TRACE_IO 0

/*
 * Read PCI register, swap if necessary
 */
static ulong tulip_inw(tulip_private_t *data, int reg) {
	ulong val = swaplong(data->ioaddr[reg / sizeof(ulong)]);
	__eieio();
#if 0
	if ((reg/8 != 9) && (reg/8 !=5))  // suppress eeprom & intrupt io
		dprintf("inw(%d) = %08x\n", reg / 8, val);
#endif /* TRACE_IO */
	return (val);
}


static void decode_csr6(uint32  csr6) {
	dprintf("csr6=%x ", csr6);
	if (csr6 & 1 << 31) dprintf("Special Capture Effect, ");
	if (csr6 & 1 << 30) dprintf("Receive All, ");
	if (csr6 & 1 << 25) dprintf("MBO, ");	/* Must Be One */
	if (csr6 & 1 << 24) dprintf("Scrambler, ");
	if (csr6 & 1 << 23) dprintf("PCS, ");
	if (csr6 & 1 << 22) dprintf("Transmit Threshold, ");
	if (csr6 & 1 << 21) dprintf("Store and Forward, ");
	if (csr6 & 1 << 19) dprintf("Heart Beat Disable, ");
	if (csr6 & 1 << 18) dprintf("Port Select, ");
	if (csr6 & 1 << 17) dprintf("Capture Effect, ");
	if (csr6 & 3 << 14) dprintf("TR=%x " ,csr6 >> 14 & 3);
	if (csr6 & 1 << 13) dprintf("Transmitter on, ");
	if (csr6 & 1 << 12) dprintf("Force Collisions, ");
	if (csr6 & 3 << 10) dprintf("Operating Mode=%x " ,csr6 >> 10 & 3);
	if (csr6 & 1 <<  9) dprintf("Full Duplex, ");
	if (csr6 & 1 <<  7) dprintf("Pass all Multicast, ");
	if (csr6 & 1 <<  6) dprintf("Promiscuous Mode, ");
	if (csr6 & 1 <<  5) dprintf("Start/Stop Backoff Counter, ");
	if (csr6 & 1 <<  3) dprintf("Inverse Filtering , ");
	if (csr6 & 1 <<  2) dprintf("Hosh only Filtering Mode, ");
	if (csr6 & 1 << 1) dprintf("Receiver On, ");
	if (csr6 & 1) dprintf("Hash Perfect Filtering ");
	dprintf("\n");
}


static void decode_csr0(uint32 csr0) {
	dprintf("csr0=%8.8x ", csr0);
	if (csr0 & 1 << 24) dprintf("Write Invalidate, ");
	if (csr0 & 1 << 23) dprintf("Read Line Enable, ");
	if (csr0 & 1 << 21) dprintf("Read Multiple Enable, ");
	if (csr0 & 1 << 20) dprintf("Descriptor Byte Order, ");
	if (csr0 & 7 << 17) dprintf("Tx Auto Polling=%x, " ,csr0 >> 17 & 7);
	if (csr0 & 3 << 14) dprintf("Cache Alignment=%x, " ,csr0 >> 14 & 3);
	if (csr0 & 0x3F << 8) dprintf("Prog Burst Len=%x, " ,csr0 >> 8 & 0x3F);
	if (csr0 & 0x1F << 2) dprintf("Desc Skip Len=%x, " ,csr0 >> 2 & 0x1F);
	if (csr0 & 1 << 1) dprintf("Bus Arbitration,  ");
	if (csr0 & 1) dprintf("Software Reset.");
	dprintf("\n");
}
/*
 * Write PCI register, swap if necessary
 */
static void tulip_outw(tulip_private_t *data, int reg, ulong val) {


	if ((reg/8 == 6) && (data->promisc_mode)) {
		val |= CSR6_promisc;
	}
	data->ioaddr[reg / sizeof(ulong)] = swaplong(val);
	__eieio();

#if TRACE_IO
 	if (reg/8 == 6) {
			decode_csr6(val);
	} else
	if (reg == 0) {
		decode_csr0(val);
	} else  	
	if ((reg/8 == 15) || (reg/8 == 12)) {
		dprintf("outw(%d, %08x)\n", reg / 8, val);
	} else {	
#if 0
		if ((reg/8 != 9) && (reg/8 !=5) && (reg/8 !=1)) 	// suppress eeprom & intr io & xmit demands
			dprintf("outw(%d, %08x)\n", reg / 8, val);
#endif
	}
#endif /* TRACE_IO */
}


void dump_csr(tulip_private_t *data) {

		dprintf("dump_csr: CSR0= %8.8x CSR5= %8.8x CSR6= %8.8x CSR7=%8.8x CSR8=%8.8x\n"
			   "CSR12= %8.8x CSR13= %8.8x CSR14= %8.8x CSR15= %8.8x\n",
				tulip_inw(data,CSR0_BUS_MODE),
				tulip_inw(data,CSR5_STATUS),
				tulip_inw(data,CSR6_OP_MODE),
				tulip_inw(data,CSR7_INTR_MASK),
				tulip_inw(data,CSR8),
				tulip_inw(data,CSR12_SIA_STATUS),
				tulip_inw(data,CSR13_SIA_CONN),
				tulip_inw(data,CSR14_SIA_TX_RX),
				tulip_inw(data,CSR15_SIA_GEN)
			   );
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
	dprintf("%s\n", buf);
}

/* 
 * Does the card own this buffer?
 */
static int
card_owns(
		  volatile tulip_ring_t *ring
		  )
{
	return ((swaplong(ring->status) & RTDES0_own_card));
}

static void
dump_ring(
		  tulip_ring_t *ring,
		  unsigned ringmax
		  )
{
	int i;

	for (i = 0; i < ringmax; i++) {
		dprintf("%d: %08x %08x\n", i, ring->length, ring->status);
		ring++;
	}
}

/*
 * Initialize a ring buffer
 */
static void
fill_ring(
		  unsigned ringx,
		  unsigned ringmax,
		  volatile tulip_ring_t *ring,
		  tulip_pbuf_t *pbuf,
		  ulong length,
		  ulong status,
		  ulong flags
		  )
{
	unsigned secondlen;

	ring = &ring[ringx];
	pbuf = &pbuf[ringx];

	if (ringx == (ringmax - 1)) {
		flags |= RTDES1_end_of_ring;
	}
	
	
	ring->buf1 = swaplong( (uint32) gPCIModInfo->ram_address(pbuf->first));

	if (length <= pbuf->firstlen) {
		ring->length = swaplong(length | flags);
		ring->buf2 = (ulong) NULL;
	} else {


		secondlen = (length - pbuf->firstlen) << 11;
		ring->length = swaplong(pbuf->firstlen | secondlen | flags);
		ring->buf2 = swaplong( (uint32) gPCIModInfo->ram_address(pbuf->second));

	}
	/*
	 * This must be set last (it could change ownership of the buffer)
	 */

	ring->status = swaplong(status);
}

/* 
 * Fill a read ring buffer
 */
static void
fill_ring_read(
			   unsigned ringx,
			   unsigned ringmax,
			   volatile tulip_ring_t *ring,
			   tulip_pbuf_t *pbuf
			   )
{
	fill_ring(ringx, ringmax, ring, pbuf, PACKET_BUF_SIZE,
			  RTDES0_own_card, 0);
}


/*
 * Fill a write ring buffer
 */
static void
fill_ring_write(tulip_private_t * data,
				unsigned ringx,
				unsigned ringmax,
				volatile tulip_ring_t *ring,
				tulip_pbuf_t *pbuf,
				ulong length
				)
{

	if ( data->tx_ringx - data->tx_unack < TX_RING_SIZE/2 ) {		// normal case, no interrupt
		fill_ring(ringx, ringmax, ring, pbuf, length, RTDES0_own_card,
			  TDES1_first | TDES1_last );
	}
	else {
		fill_ring(ringx, ringmax, ring, pbuf, length, RTDES0_own_card,
			  TDES1_first | TDES1_last | TDES1_interrupt);
	}
}

#if DEBUG
static void
error_summary(
			  int thisring, 
			  ulong status
			  )
{
#define doit(bit, msg)  if (status & (1 << bit)) dprintf("%d: %s\n", thisring, msg)
	doit(0, "overflow");
	doit(1, "crc error");
	doit(2, "dribbling");
	doit(4, "receive watchdog");
	doit(5, "frame type");
	doit(6, "collision");
	doit(7, "frame too long");
	doit(8, "last descriptor");
	doit(9, "first descriptor");
	doit(10, "multicast frame");
	doit(11, "runt frame");
	dprintf("%d: data type %d\n", thisring, (status >> 12) & 0x3);
	doit(13, "data type 1");
	doit(14, "length error");
	doit(15, "error summary");
	dprintf("%d: error %04x\n", thisring, (status >> 16) & 0x7fff);
	doit(31, "owner");
#undef doit	
}
#endif /* DEBUG */


static long
sem_count(sem_id sem)
{
	long count;

	get_sem_count(sem, &count);
	return (count);
}

static void
init_read_ring(
			   tulip_private_t *data
			   )
{
	int i;

	/*
	 * Init receive buffers
	 */
	for (i = 0; i < RX_RING_SIZE; i++) {
		fill_ring(i, RX_RING_SIZE, data->rx_ring,
				  data->rx_pbuf, PACKET_BUF_SIZE,
				  RTDES0_own_card, 0);
	}
}

static void
init_write_ring(
				tulip_private_t *data
				)
{
	int i;

	/*
	 * initialize transmit buffers
	 */
	for (i = 0; i < TX_RING_SIZE; i++) {
		fill_ring(i, TX_RING_SIZE, data->tx_ring, data->tx_pbuf, 0, 0, 0);
	}
}

static void
init_rings(
		   tulip_private_t *data
		   )
{

	init_read_ring(data);
	init_write_ring(data);
}


static void dump_rx_ring(tulip_private_t *data, short rx_now) {
	int j;
	
	dprintf("rx_now=%d rx_ringx=%d rx_acked=%d rx_sem=%d\n",
		rx_now, data->rx_ringx, data->rx_acked, sem_count(data->rx_sem));
	for (j=0; j<RX_RING_SIZE; j++) {
		dprintf(" rxb[%d].status=%x ", j, data->rx_ring[j].status);
	}
	dprintf("\n");
}

/*
 * Entry point: read a packet
 */
static status_t 
tulip_read( void *cookie, off_t pos, void *buf, size_t *len)
{
	status_t result;
	tulip_private_t *data = (tulip_private_t *) cookie;
	uint32 rx_now = data->rx_acked & RX_RING_MASK;
	int16		limit = RX_RING_SIZE; 
	uint32 status;
	bool 		done = false;

	while ( (!done) && (limit-- > 0)) {

		// Block until data is available
		if((result = acquire_sem_etc( data->rx_sem, 1, B_CAN_INTERRUPT | data->blockFlag, 0 )) != B_NO_ERROR) {
			*len = 0;
			return result;
		}
		// Protect againsts reentrant read
		if( atomic_or( &data->readLock, 1 ) ) {
			release_sem_etc( data->rx_sem, 1, 0 );
			ETHER_DEBUG(ERR, data->debug, "Reentrant tulip_read()\n");
			*len = 0;
			return B_ERROR;
		}

		status = swaplong(data->rx_ring[rx_now].status);
		if ((status & 0xB8008300) != 0x0300) {
			ETHER_DEBUG(WARN, data->debug, "RX err %x rx_now=%d\n", status, rx_now);
//			dump_rx_ring(data, rx_now);
			// update error statistics
		} else {  // looks good
			uint32 packetlen = status >> 16;
//				dprintf("tulip_rd: packet len=%x len=%x min=%x\n",packetlen, *len, min_c(*len,packetlen));
			packetlen = min_c(*len, packetlen);
			memcpy(buf, &data->rx_vbuf[rx_now], packetlen);

			{ unsigned short  *seq = (unsigned short *) buf;
			ETHER_DEBUG(SEQ,data->debug," R%4.4x ", seq[20]);  /* sequence number */
			}

			if (data->debug & RX) {
				dump_packet("rx:", (unsigned char *)buf);
			}

			/* ARP broadcasts transmitted also show up on the receive queue
			because the LiteOn hardware address filtering doesn't work. If we
			don't filter them here, the net server will think there are
			duplicate IP addresses on the net.		*/
			{	short * my_address = (short *) (data->myaddr.ebyte);
				short * src_hw_addr = (short *) buf;
				if ((my_address[2] == src_hw_addr[ 5]) &&
					(my_address[1] == src_hw_addr[ 4]) &&		
					(my_address[0] == src_hw_addr[ 3]) )	{
					ETHER_DEBUG(WARN, data->debug, "Rx Filter ARP\n");
				} else { // really looks good
					done = true;
					*len = packetlen;
//						dprintf("tulip_rd: OK len %x\n", packetlen);
				} 
			}
		}
		// Another read may now take place
		fill_ring_read(rx_now, RX_RING_SIZE, data->rx_ring, data->rx_pbuf);
		rx_now = data->rx_acked = ((data->rx_acked+1) & RX_RING_MASK);
		data->readLock = 0;
	}
	return (B_OK);
}


static void
tx_intr(tulip_private_t *data)
{
	uint32 spot;
	uint32 status;
	uint16 txcount = 0;
	
   for (spot = data->tx_unack; data->tx_ringx - spot > 0; spot++) {
        status = data->tx_ring[spot & TX_RING_MASK].status;

        if (status & RTDES0_own_card) /* card still owns buffer */
            break;

        if (status & 0x8000) {
            /* to do: update statistic here... */
            ETHER_DEBUG(WARN, data->debug, "xmit err %x\n", status);
        }
		if (status & 1) {
//          ETHER_DEBUG(WARN, data->debug, "xmit deferred %x\n", status);
		}
		
		
		txcount++;   
 	}				
				
	if (txcount) {
		data->tx_unack += txcount;
		release_sem_etc(data->tx_sem, txcount, B_DO_NOT_RESCHEDULE);

	} else {
//       ETHER_DEBUG(WARN, data->debug, "xmit intr but nothing there\n");
	}			
				
}


static void dump_packet(const char * msg, unsigned char * buf) {
/* dump the first 3 x 16 bytes ... */
	dprintf("%s\n%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x \n", msg,
	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
	buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23],
	buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39],
	buf[40], buf[41], buf[42], buf[43], buf[44], buf[45], buf[46], buf[47]);
}



static status_t
tulip_write(void *cookie, off_t pos, const void *buf, size_t *len)
{
	int i;
	int thisring;
	status_t err;
	ulong status;
	long count;
	ulong buflen;
	ulong packetlen;
	int16         tx_now;
	tulip_private_t *data;

	data = (tulip_private_t *) cookie;

	ETHER_DEBUG(FUNCTION, data->debug, ":write buf %x len %d (decimal)\n",buf,*len);

#if 0
	if ((data->linkStatus == 0)) {  	/* Is link ready to use? */
		snooze( 10000 );
		*len = 0;
		return B_ERROR;
	}
#endif

	if( *len > MAX_FRAME_SIZE ) {
		ETHER_DEBUG( ERR, data->debug, " write %d > 1514 tooo long\n",*len);
		*len = MAX_FRAME_SIZE;
	}	
	
	buflen = *len;

	if ((err = acquire_sem_etc(data->tx_sem, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR) {
		*len = 0;
        ETHER_DEBUG(WARN, data->debug, "tulip_write() acq sem failed %x \n", err);
		return err;
	}

	// Protect againsts reentrant write
	if( atomic_or( &data->writeLock, 1 ) ) {
		release_sem_etc( data->tx_sem, 1, 0 );
		*len = 0;
		ETHER_DEBUG(ERR, data->debug, "Reentrant write\n");
		return B_ERROR;
	}

	if ( data->debug & TX ) {
		dump_packet("tx:", (unsigned char *) buf);
	}

	thisring = data->tx_ringx & TX_RING_MASK;
	
	status = swaplong(data->tx_ring[thisring].status);

	if (status & RTDES0_own_card) {	
		/* here we are trying to write into a buffer that is still owned by the card which means
		   the semaphore count or tx_ring is off - this should never happen. */
		ETHER_DEBUG(ERR, data->debug, "Error tx_ring[%d].status = %x sem_count=%d\n",
			thisring, swaplong(data->tx_ring[thisring].status), sem_count(data->tx_sem));
		*len = 0;
		data->writeLock = 0;
		return B_ERROR;
	}

	data->tx_ringx++;
	
	memcpy(&data->tx_vbuf[thisring], buf, buflen);

	fill_ring_write(data, thisring, TX_RING_SIZE, data->tx_ring, data->tx_pbuf, buflen);

	tulip_outw(data, CSR1_TX_POLL, CSR1_transmit);
	*len = buflen;
	data->writeLock = 0;
	return (0);
}


static status_t
tulip_writev(void *cookie, off_t position, const iovec *vec, size_t count, size_t *len)
{
	int i;
	int thisring;
	status_t err;
	ulong status;
	ulong buflen;
	ulong packetlen;
	int16         tx_now;
	tulip_private_t *data;

	data = (tulip_private_t *) cookie;

	ETHER_DEBUG(FUNCTION, data->debug, ":write iovec=%x  count=%d (decimal)\n",vec, count);

	if ((data->linkStatus == 0)) {  	/* Is link ready to use? */
		snooze( 10000 );
		*len = 0;
		return B_ERROR;
	}


	if ((err = acquire_sem_etc(data->tx_sem, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR) {
		*len = 0;
        ETHER_DEBUG(WARN, data->debug, "tulip_write() acq sem failed %x \n", err);
		return err;
	}

	// Protect againsts reentrant write
	if( atomic_or( &data->writeLock, 1 ) ) {
		release_sem_etc( data->tx_sem, 1, 0 );
		*len = 0;
		ETHER_DEBUG(ERR, data->debug, "Reentrant write\n");
		return B_ERROR;
	}

	thisring = data->tx_ringx & TX_RING_MASK;
	
	status = swaplong(data->tx_ring[thisring].status);

	if (status & RTDES0_own_card) {	
		/* here we are trying to write into a buffer that is still owned by the card which means
		   the semaphore count or tx_ring is off - this should never happen. */
		ETHER_DEBUG(ERR, data->debug, "Error tx_ring[%d].status = %x sem_count=%d\n",
			thisring, swaplong(data->tx_ring[thisring].status), sem_count(data->tx_sem));
		*len = 0;
		data->writeLock = 0;
		return B_ERROR;
	}

	data->tx_ringx++;

	// Copy data to buffer
	{	off_t	offset;

	for( i=0, offset=0; i<count; offset += vec->iov_len, i++, vec++ )
		memcpy( ((uint8 *)(&data->tx_vbuf[thisring]))+offset, vec->iov_base, vec->iov_len );

	if ( data->debug & TX ) {
		dump_packet("tx:", (unsigned char *) &data->tx_vbuf[thisring]);
	}
	
	fill_ring_write(data, thisring, TX_RING_SIZE, data->tx_ring, data->tx_pbuf, offset);

	tulip_outw(data, CSR1_TX_POLL, CSR1_transmit);
	*len = offset;
	}
	data->writeLock = 0;
	return (0);
}


/*
 * Initialize everything to ring descriptor zero
 */
static void
reset_to_zero(
			  tulip_private_t *data
			  )
{
	long count;

	/*
	 * Stop transmit and receive
	 */
	tulip_outw(data, CSR6_OP_MODE, 
			   tulip_inw(data, CSR6_OP_MODE) &
			   ~(CSR6_tx_start | CSR6_rx_start | CSR6_promisc));


	init_rings(data);
	data->tx_ringx = 0;
	data->tx_unack = 0;
	data->rx_ringx = 0;
	data->rx_acked = 0;

	count = sem_count(data->tx_sem);
	if (count < TX_RING_SIZE) {
		if (data->debug > 1) dprintf("Reset tx sem: count = %d\n", count);
		release_sem_etc(data->tx_sem, (TX_RING_SIZE - count), 0);
	}
	count = sem_count(data->rx_sem);
	if (count != 0) {
		if (data->debug > 1) dprintf("Reset rx sem: count = %d\n", count);
		acquire_sem_etc(data->tx_sem, count, 0, 0);
	}

	tulip_outw(data, CSR3_RX_LIST, (uint32)gPCIModInfo->ram_address(&data->rx_pring[0]));
	tulip_outw(data, CSR4_TX_LIST, (uint32)gPCIModInfo->ram_address(&data->tx_pring[0]));

	/*
	 * Clear any pending interrupts
	 */
	tulip_outw(data, CSR5_STATUS, 
			   tulip_inw(data, CSR5_STATUS) & data->csr5_int_clear);
	
	if ((data->vendor_id == VENDOR_LITEON) || (data->vendor_id == VENDOR_INTEL)) {
		return;
	}
	
	/*
	 * Restart transmit and receive
	 */
	if (data->device_id == DEC_21140) {
			tulip_outw(data, CSR6_OP_MODE, 
				   0x320e0000 |
				   (CSR6_tx_start | CSR6_rx_start));
	} else {
		tulip_outw(data, CSR6_OP_MODE, 
				   tulip_inw(data, CSR6_OP_MODE) |
				   (CSR6_tx_start | CSR6_rx_start));
		}
}

static const char *
media_type_str(
			   int media_type
			   )
{
	switch (media_type) {
	case MEDIA_BNC:
		return ("BNC");
	case MEDIA_AUI:
		return ("AUI");
	case MEDIA_10BASET:
		return ("10baseT");
	case MEDIA_100BASET:
		return ("100baseT");
	default:
		return ("Unknown");
	}
}

static ulong
csr13_config(
			 tulip_private_t *data
			 )
{
	switch (data->device_id) {
	case DEC_21040:
		return (CSR13_21040_config);

	/* Note that case DEC_21041 never executes because the 21041 is set to
	   run in 21040 mode, and the device_id is reset in hw_init() */
	case DEC_21041:
		return (CSR13_21041_config);
	default:
		if (data->debug > 1) dprintf("What the heck?\n");
		return (0);
	}
}

static media_xxx_t 
next_media_2104x(tulip_private_t *data, media_xxx_t media)
{
	switch (media) {
	default:
	case MEDIA_10BASET:
		return (MEDIA_BNC);
	case MEDIA_BNC:
		return (MEDIA_AUI);
	case MEDIA_AUI:
		return (MEDIA_10BASET);
	}
}

static media_xxx_t 
next_media_2114x(tulip_private_t *data, media_xxx_t media)
{
#if works_100
	switch (media) {
	default:
	case MEDIA_100BASET:
		return (MEDIA_10BASET);
	case MEDIA_10BASET:
		return (MEDIA_100BASET);
	}
#else
	return (MEDIA_10BASET);
#endif
}

static media_xxx_t
next_media(tulip_private_t *data, media_xxx_t media)
{
	switch (data->device_id) {
	case DEC_21040:
	case DEC_21041:
		return (next_media_2104x(data, media));
		break;
	case DEC_21140:
		return (next_media_2114x(data, media));
		break;
	default:
		if (data->debug > 1) dprintf("What the heck?\n");
		return ((media_xxx_t)0);
	}
}

static void
media_set_21040(
				tulip_private_t *data,
				int media_type
				)
{
	tulip_outw(data, CSR11_FULL_DUPLEX, 0x6969); /* XXX: what is this-- full duplex "Magic" setting */
	tulip_outw(data, CSR15_SIA_GEN, CSR15_reset);
	tulip_outw(data, CSR14_SIA_TX_RX, CSR14_reset);
	tulip_outw(data, CSR13_SIA_CONN, CSR13_reset);	
	switch (media_type) {
	case MEDIA_10BASET:
		/* CSR15 defaults to 10baseT */
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_10baset);
		tulip_outw(data, CSR13_SIA_CONN, csr13_config(data));
		break;
	case MEDIA_BNC:
		tulip_outw(data, CSR15_SIA_GEN, CSR15_bnc);
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_auibnc);
		tulip_outw(data, CSR13_SIA_CONN, 
				   csr13_config(data) | CSR13_auibnc_config);
		break;
	case MEDIA_AUI:
		tulip_outw(data, CSR15_SIA_GEN, CSR15_aui);
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_auibnc);
		tulip_outw(data, CSR13_SIA_CONN, 
				   csr13_config(data) | CSR13_auibnc_config);
		break;
	}
}

static void
media_set_21041(
				tulip_private_t *data,
				int media_type
				)
{
	tulip_outw(data, CSR13_SIA_CONN, CSR13_reset);	
	switch (media_type) {
	case MEDIA_10BASET:
		tulip_outw(data, CSR15_SIA_GEN, 0);
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_10baset);
		tulip_outw(data, CSR13_SIA_CONN, csr13_config(data));
		break;
	case MEDIA_BNC:
		tulip_outw(data, CSR15_SIA_GEN, CSR15_bnc);
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_auibnc);
		tulip_outw(data, CSR13_SIA_CONN, 
				   csr13_config(data) | CSR13_auibnc_config);
		break;
	case MEDIA_AUI:
		tulip_outw(data, CSR15_SIA_GEN, CSR15_aui);
		tulip_outw(data, CSR14_SIA_TX_RX,  CSR14_auibnc);
		tulip_outw(data, CSR13_SIA_CONN, 
				   csr13_config(data) | CSR13_auibnc_config);
		break;
	}
}

static int
is_a_znyx(tulip_private_t *data)
{
	/*
	 * This is a pretty gross hack
	 */
	return (data->myaddr.ebyte[0] == ZNYX_ADDR0 &&
			data->myaddr.ebyte[1] == ZNYX_ADDR1 &&
			data->myaddr.ebyte[2] == ZNYX_ADDR2);
}

static void
media_set_21140(
				tulip_private_t *data,
				int media_type
				)
{
	ulong csr6;


	csr6 = tulip_inw(data, CSR6_OP_MODE);
	csr6 &= ~(CSR6_10_flags & CSR6_100_flags);

	if (is_a_znyx(data)) {
		/*
		 * This is specific to Znyx card
		 */
		tulip_outw(data, CSR12_GP_PORT, 0x11); /* pins */
		tulip_outw(data, CSR12_GP_PORT, 0x09);	/* init */
	}
	switch (media_type) {
	case MEDIA_10BASET:
		tulip_outw(data, CSR6_OP_MODE, csr6 | CSR6_10_flags);
		break;
	case MEDIA_100BASET:
		tulip_outw(data, CSR6_OP_MODE, csr6 | CSR6_100_flags);
		break;
	}
}

static void
media_set(
		  tulip_private_t *data,
		  int media_type
		  )
{
	if (data->debug > 1) dprintf("Trying %s\n", media_type_str(media_type));
	switch (data->device_id) {
	case DEC_21040:
		media_set_21040(data, media_type);
		break;
	case DEC_21041:
		media_set_21041(data, media_type);
		break;
	case DEC_21140:
		media_set_21140(data, media_type);
		break;
	default:
		if (data->debug > 1) dprintf("What the heck?\n");
		break;
	}
}

static void
media_init(
		   tulip_private_t *data
		   )
{
	switch (data->device_id) {
	case DEC_21040:
	case DEC_21041:
		data->media_xxx = MEDIA_10BASET;
		break;
	default:
		if (data->debug > 1) dprintf("media_init()default switch!?\n");
	break;
	}
	media_set(data, data->media_xxx);
}

char *
xmitter_state(char state) {
	switch (state) {
		case 0x0: return("stopped");
		case 0x1: return("running fetching descriptor");
		case 0x2: return("running wait for end of xmission");
		case 0x3: return("running queuing to FIFO");
		case 0x5: return("running setup packet");
		case 0x6: return("suspended");
		case 0x7: return("running close xmit descriptor");
		default: return("bad state");	
	}
}

char *
rcver_state(char state) {
	switch (state) {
		case 0x0: return("stopped");
		case 0x1: return("running fetching descriptor");
		case 0x2: return("running checking for end of rcv packet");
		case 0x3: return("running waiting for receive packet");
		case 0x4: return("suspended unavailable rcv buffer");
		case 0x5: return("running closing receive descriptor");
		case 0x6: return("running flusing current frame because rcv buff unavailable");
		case 0x7: return("running queuing received frame from FIFO to receive buffer");
		default: return("bad state");	
	}
}


/* 
 * Check the SIA status register for link heart beats indicating a live media is
 * plugged in and the chip is configured correctly for it.
 */
static int
check_100MB_link_pulse(tulip_private_t *data)
{
	unsigned long	csr5 = tulip_inw(data,CSR5);
	volatile unsigned long	csr12;
	const unsigned long LinkHeartBeat100MB = 0x2;
	const unsigned short MAX_TRIES = 4;
	short j;

	if (data->debug & WARN)  {	
		dprintf("check_100MB_link_pulse: xmitter is %s, receiver is %s\n",
		xmitter_state((csr5 >> 20) & 7),
		  rcver_state((csr5 >> 17) & 7));
	
		dump_csr(data);
	}
	
	for (j=0; j<MAX_TRIES; j++) {
		csr12 = tulip_inw(data,CSR12);
		if ((csr12 & LinkHeartBeat100MB) == 0) {   // polarity may be reversed for some other chips
			ETHER_DEBUG(INFO, data->debug, "check_100MB_link: 100 MB LINK FOUND %d!\n",j);
			return (true);		// found 100MB connection
		} else {
			snooze(400000);
		}
	}
	ETHER_DEBUG(INFO, data->debug, "check_100MB_link: no joy\n");
	return (false);
}
/* 
 * Check the SIA status register for link heart beats indicating a live media is
 * plugged in and the chip is configured correctly for it.
 */
static int
check_10MB_link_pulse(tulip_private_t *data)
{
	unsigned long	csr5 = tulip_inw(data,CSR5);
	volatile unsigned long	csr12;
	const unsigned long LinkHeartBeat10MB = 0x4;
	const unsigned short MAX_TRIES = 3;
	short j;

	if (data->debug & WARN) {
		dprintf("check_10MB_link_pulse: xmitter is %s, receiver is %s\n",
		xmitter_state((csr5 >> 20) & 7),
		  rcver_state((csr5 >> 17) & 7));
	
		dump_csr(data);
	}
	for (j=0; j<MAX_TRIES; j++) {
		csr12 = tulip_inw(data,CSR12);
		if ((csr12 & LinkHeartBeat10MB) == 0) {   // polarity may be reversed for some other chips
			ETHER_DEBUG(INFO, data->debug, "check_10MB_link: 10 MB LINK FOUND %d!\n",j);
			return (true);		// found 100MB connection
		} else {
			snooze(100000);
		}
	}
	ETHER_DEBUG(INFO, data->debug, "check_10MB_link: no joy\n");
	return (false);
}

		  
/* 
 * Send a packet to ourselves and see if our media selection is correct
 */
static int
media_try(
		  tulip_private_t *data
		  )
{
	char buf[17];
	int i;
	int ringx;
	status_t err;
	long modified = 0;
	ulong csr5;
	size_t len;
	double time0;

	ringx = data->tx_ringx;
	memcpy(buf, &data->myaddr, sizeof(data->myaddr));
	memcpy(buf + sizeof(data->myaddr), &data->myaddr, sizeof(data->myaddr));
	buf[13] = 0;
	buf[14] = 3;
	buf[15] = 0;
	buf[16] = 0xe3;
	len = sizeof(buf);


	data->linkStatus = true; // link is up.

	if ((err = tulip_write(data, 0, buf, &len)) < B_NO_ERROR) {
		ETHER_DEBUG(ERR, data->debug, "Can't write to determine media\n");
		return (err);
	}

#define TOO_LONG 200
	time0 = system_time();
	for (i = 0; card_owns(&data->tx_ring[ringx]) && i < TOO_LONG; i++) {
		/* keep waiting */
		snooze(8000);
	}
	ETHER_DEBUG(INFO, data->debug, "media_try: dt=%10.4f\n", (system_time()-time0)/1000);
	/*
	 * Do any cleanup
	 */
	tx_intr(data);

	if (i == TOO_LONG) {
		ETHER_DEBUG(INFO, data->debug, "bogus count exceeded\n");
	}
	csr5 = tulip_inw(data, CSR5_STATUS);
	tulip_outw(data, CSR5_STATUS, csr5 & data->csr5_int_clear);
	if (CSR5_tx_state(csr5) != CSR5_tx_suspended) {
		/*
		 * Stop transmitter and receiver
		 */
		tulip_outw(data, CSR6_OP_MODE, 
				   (tulip_inw(data, CSR6_OP_MODE) & 
					~(CSR6_tx_start | CSR6_rx_start)));

		if (card_owns(&data->tx_ring[ringx])) {
			data->tx_ring[ringx].status = 0;
			tulip_outw(data, CSR5_STATUS, 
					   tulip_inw(data, CSR5_STATUS) & data->csr5_int_clear);

			/*
			 * Reset SIA
			 */
			data->media_xxx = next_media(data, data->media_xxx);
			media_set(data, data->media_xxx);
			modified++;
		}

		/*
		 * Restart transmitter and receiver
		 */
		tulip_outw(data, CSR6_OP_MODE, 
				   (tulip_inw(data, CSR6_OP_MODE) |
					(CSR6_tx_start | CSR6_rx_start)));
	}
	return (modified);
}

/*
 * Get the name of the device
 */
static const char *	
device_id_name(ushort device_id)
{
	switch (device_id) {
	case DEC_21040:
		return ("DEC 21040");
	case DEC_21041:
		return ("DEC 21041");
	case DEC_21140:
		return ("DEC 21140");
	case DEC_21143:
		return ("DEC 21143");
	default:
		return ("Unknown DEC device");
	}
}

/* 
 * Read our ethernet address out of the rom
 */
static long
read_address_21040(
				   tulip_private_t *data
				   )
{
	union {
		short srom[16];
		char crom[32];
	} urom;
	char *rom = urom.crom;
	char *hexof = "0123456789abcdef";
	int i;
	int j;
	long csr;
	int cksum;
	int rom_cksum;
	short *sptr;

	tulip_outw(data, CSR9_ETHER_ROM, 1);	/* reset the rom */
	snooze(1000);	/* give card a chance to read the rom */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < MAXBOGUS_ROM; j++) {
			csr = tulip_inw(data, CSR9_ETHER_ROM);
			if (csr >= 0) {
				break;
			}
		}
		if (j == MAXBOGUS_ROM) {
			return (B_IO_ERROR);
		}
		rom[i] = csr;
	}

	if (memcmp(&rom[24], TESTPAT, 8) != 0) {
		dprintf("rom failure on testpat\n");
		return (B_IO_ERROR);
	}

	/*
	 * Now check the rom address for validity
	 */
	for (i = 0; i < 8; i++) {
		if (rom[i] != rom[15 - i]) {
			dprintf("rom failure: %d %d\n", i, 15 - i);
			return (B_IO_ERROR);
		}
	}
	
	for (i = 0; i < 6; i++) {
		data->myaddr.ebyte[i] = rom[i];
	}
	
	sptr = (short *)&rom[0];
	cksum = (ushort)swapshort(*sptr); sptr++;
	cksum *= 2;
	if (cksum > 65535) {
		cksum -= 65535;
	}
	cksum += (ushort)swapshort(*sptr); sptr++;
	cksum *= 2;
	if (cksum > 65535) {
		cksum -= 65535;
	}
	cksum += (ushort)swapshort(*sptr); sptr++;
	if (cksum > 65535) {
		cksum -= 65535;
	}
	rom_cksum = swapshort(*sptr);
	if (cksum != rom_cksum) {
		dprintf("checksum failure: %04x %04x\n", cksum, rom_cksum);
		return (B_IO_ERROR);
	}
	return (B_NO_ERROR);
}

/*
 * Delay for at least 1 microsecond
 */
#define DELAY() spin(1)

static void
put_bits(
		 tulip_private_t *data,
		 unsigned val,
		 unsigned nbits
		 )
{
	int i;
	unsigned flag;

	for (i = 0; i < nbits; i++) {
		flag = ((val >> ((nbits - 1) - i)) & 1) ? CSR9_data_in : 0;

		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs | flag);
		DELAY();

		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs | CSR9_clock | flag);
		DELAY();

		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs | flag);
		DELAY();
	}
}


static int crc_check(char *data)
{
	int i;
	unsigned val;
	unsigned bit;
	unsigned msb;
	unsigned long crc = 0xffffffff;
	unsigned long flipped = 0;
	const unsigned long POLY = 0x04c11db6; 
	
	for (i = 0; i < 126; i++) {
		val = data[i];
		for (bit = 0; bit < 8; bit++) {
			msb = (crc >> 31) & 1;
			crc <<= 1;
			if (msb ^ (val & 1)) {
				crc ^= POLY;
				crc |= 1;
			}
			val >>= 1;
		}
	}
	for (i = 0; i < 32; i++) {
		flipped <<= 1;
		bit = crc & 1;
		crc >>= 1;
		flipped += bit;
	}
	crc = flipped ^ 0xffffffff;
	if (((crc & 0xff) != (unsigned char)data[126]) ||
		(((crc >> 8) & 0xff) != (unsigned char)data[127])) {
 		dprintf("CRC failure: %02x:%02x %02x:%02x\n", 
				crc & 0xff, (crc >> 8) & 0xff,
				(unsigned char)data[126], 
				(unsigned char)data[127]);
		return (0);
	}
	return (1);
}


/* 
 * Read the 21041 serial rom
 */
static long  read_srom( tulip_private_t *data, unsigned char *buf)
{
	int i;
	int j;
	int dout;
	unsigned short *wbuf = (unsigned short *)buf;
	unsigned short word;
	
	for (i = 0; i < TULIP_ROM_SIZE / 2; i++) {
		tulip_outw(data, CSR9_ETHER_ROM, CSR9_read | CSR9_srom);
		DELAY();
		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs);
		DELAY();
		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs | CSR9_clock);
		DELAY();
		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom | CSR9_cs);
		DELAY();

		/*
		 * Output address & cmd
		 */
		put_bits(data, i | (SROM_CMD_READ << SROM_CMD_SHIFT), SROM_CMD_NBITS);

		word = 0;
		for (j = 0; j < 16; j++) {
			tulip_outw(data, CSR9_ETHER_ROM,
					   CSR9_read | CSR9_srom | CSR9_cs | CSR9_clock);
			DELAY();
			dout = tulip_inw(data, CSR9_ETHER_ROM);
			word = (word << 1) | ((dout & CSR9_data_out) ? 1 : 0);
			tulip_outw(data, CSR9_ETHER_ROM,
					   CSR9_read | CSR9_srom | CSR9_cs);
			DELAY();
		}
		tulip_outw(data, CSR9_ETHER_ROM, 
				   CSR9_read | CSR9_srom);
		DELAY();

		wbuf[i] = swapshort(word);
	}
	if (!crc_check((char *)wbuf)) {
		return (B_ERROR);
	}
	return (B_NO_ERROR);
}

/* 
 * Read our ethernet address out of the rom
 */
static long
read_address_21041(
				   tulip_private_t *data
				   )
{
	unsigned char buf[TULIP_ROM_SIZE];
	status_t	err;
	int i;

	err = read_srom(data, buf);
	if (err < B_NO_ERROR) {
		/*
		 * Failed CRC check: test for broken SROM format (all but first
		 * six bytes of ROM are FF).
		 */
		for (i = 6; i < TULIP_ROM_SIZE; i++) {
			if ((uchar)buf[i] != 0xff) {
				/*
				 * Well, I guess it really is invalid
				 */
				dprintf("Your SROM is corrupted.\n");
				return (err);
			}
		}
		/*
		 * Yes, it's one of those broken ones. 
		 */
		dprintf("Invalid SROM, using it anyway.\n");
		for (i = 0; i < 6; i++) {
			data->myaddr.ebyte[i] = buf[i];
		}
		return (B_NO_ERROR);
	}

	/*
	 * Passed CRC
	 */
	for (i = 0; i < 6; i++) {
		data->myaddr.ebyte[i] = buf[20 + i];
	}
	return (B_NO_ERROR);
}

static long
read_address_LiteOn(tulip_private_t *data)
{
	unsigned char j;
	int32 deadman = 0xfff, data_in;
	uint16  * mac_addr = (uint16 *) data->myaddr.ebyte;
	long addr_ok = B_ERROR;
	
	for (j=0; j < 3; j ++) {
		tulip_outw(data, CSR19, 0x600 + j);
		data_in = -1;
		while ((data_in < 0) && (deadman--)) {
			data_in = tulip_inw(data,CSR9);
		}
		if (deadman == 0) { dprintf("Err Reading Ethernet Address\n"); return (B_ERROR); }
		mac_addr[j] = B_SWAP_INT16(data_in);

		if ((mac_addr[j] != 0) && (mac_addr[j] != 0xffff)) {
			addr_ok = B_NO_ERROR;
		}
	}	
	
	if (addr_ok == B_ERROR) {
		dprintf("Bad Ethernet Mac Address ");
	}
	return (addr_ok);
}



static status_t
read_address(
			 tulip_private_t *data
			 )
{
	switch (data->device_id) {
	case DEC_21040:
		return (read_address_21040(data));
	case DEC_21041:
	case DEC_21140:
		return (read_address_21041(data));
	case DEC_21143:
		return (parse_srom(data));
	default:
		dprintf("Don't know how to read address\n");
		return (B_IO_ERROR);
	}
}
/*
 * Create a setup frame record for the given ethernet address
 */
static void
fill_setup_frame(
				 char *setup_addr,
				 const uchar *thisaddr)
{
#if 0
	dprintf("fill_setup_frame setup_addr=%x this_addr=%x this=%2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x,\n",
		setup_addr, thisaddr, thisaddr[0],thisaddr[1],thisaddr[2],thisaddr[3],thisaddr[4],thisaddr[5]);
#endif
		setup_addr[0] = thisaddr[0];
		setup_addr[1] = thisaddr[1];
		/* 2, 3 don't care */
		setup_addr[4] = thisaddr[2];
		setup_addr[5] = thisaddr[3];
		/* 6, 7 don't care */
		setup_addr[8] = thisaddr[4];
		setup_addr[9] = thisaddr[5];
		/* 10, 11 don't care */
}

/*
 * Intialize the setup frame
 */
static status_t
do_setup_frame(
			   tulip_private_t *data
			   )
{
	int i;
	ulong ringx;
	int nmulti = data->nmulti;
	char *setup_frame;
	
	ringx = atomic_add((long *)&data->tx_ringx, 1);

	setup_frame = (char *)&data->tx_vbuf[ringx];

	fill_setup_frame(&setup_frame[0], &BROADCAST_ADDR.ebyte[0]);

	for (i = 0; i < nmulti; i++) {
		if (data->debug & WARN) print_address(&data->multi[i]);
		fill_setup_frame(&setup_frame[12 * (1 + i)], &data->multi[i].ebyte[0]);
	}

	for (i = nmulti; i < 15; i++)
	{
		fill_setup_frame(&setup_frame[12 * (1 + nmulti + i)], 
						 &data->myaddr.ebyte[0]);
	}

	fill_ring(ringx, TX_RING_SIZE, data->tx_ring, data->tx_pbuf, 
			  SETUP_FRAME_SIZE, RTDES0_own_card, TDES1_setup_packet);
	
	tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 0x2000);  // turn on xmitter
	tulip_outw(data, CSR1_TX_POLL, CSR1_transmit);

	snooze(1000); /* give card a chance to process packet */
	for (i = 0;  card_owns(&data->tx_ring[ringx]) && i < MAXBOGUS_RING; i++) {
		snooze(1);  /* keep waiting */
	}

	if (i == MAXBOGUS_RING) {
		dprintf("setup frame failed: %08x\n", data->tx_ring[ringx].status);
		return (B_IO_ERROR);
	}
	ETHER_DEBUG(WARN, data->debug, "setup frame complete\n");

	return (B_NO_ERROR);
}



/*
 * Handle a receive interrupt
 */
static void
rx_intr(tulip_private_t *data) {

	int16 j, limit, rx_now;
	uint16 rxcount = 0;
	
	rx_now = data->rx_ringx & RX_RING_MASK;

	/* scan the ring buffer for received frames */
//	acquire_spinlock( &data->rx_spinlock );
	limit = ((RX_RING_MASK - (data->rx_ringx - data->rx_acked)) & RX_RING_MASK);
	for ( j=limit; j >= 0; j--) {
		if (data->rx_ring[rx_now].status & RTDES0_own_card) break;  // card owns
		rxcount++;
		rx_now = (rx_now+1) & RX_RING_MASK;
	}

	if (rxcount) { /* signal data has arrived */
		data->rx_ringx = (data->rx_ringx + rxcount) & RX_RING_MASK;
//		release_spinlock( &data->rx_spinlock );
		release_sem_etc( data->rx_sem, rxcount, B_DO_NOT_RESCHEDULE );
	} else { /* got a spurious interrupt, or pointers are out of sync */
//		release_spinlock( &data->rx_spinlock );
		ETHER_DEBUG(WARN, data->debug, "rx_int: sync\n");
//		dump_rx_ring(data, rx_now);
	
	}
}

/* Place an upper bound on the amount of time in interrupt */
const unsigned int MAX_LATENCY = 20;
/*
 * Handle an interrupt
 */
static int32
tulip_interrupt(void *vdata)
{
	tulip_private_t *data = (tulip_private_t *)vdata;
	volatile ulong status;
	int i;
	ulong sum = 0;
	int rx = 0;
	int tx = 0;
	int32 handled = B_UNHANDLED_INTERRUPT;
	const unsigned long GENERAL_TIMER = 1 << 11;
	
	i= MAX_LATENCY;

#define CHECK_INTERRUPTS 0
#if CHECK_INTERRUPTS
	kprintf("ISR_ENTRY csr5=%x & mask %x = %x\n",tulip_inw(data,CSR5),data->csr5_int_clear,tulip_inw(data,CSR5) & data->csr5_int_clear);
#endif

	while ((i--) && ((status = tulip_inw(data, CSR5_STATUS)) & data->csr5_int_clear)) {
		
		if (status == 0xffffffff)  {
			return B_UNHANDLED_INTERRUPT;
		}
		handled = B_INVOKE_SCHEDULER;

#if CHECK_INTERRUPTS
		kprintf("ISR_LOOP status=%x csr5=%x bool=%x csr23=%x\n",status,tulip_inw(data,CSR5),status & data->csr5_int_clear,tulip_inw(data,CSR23));
#endif

		/* Clear interrupt source quickly */
		tulip_outw(data, CSR5_STATUS, status & data->csr5_int_clear);

#if CHECK_INTERRUPTS
kprintf("ISR_CLEAR status=%x csr5=%x bool=%x csr23=%x\n",status,tulip_inw(data,CSR5),tulip_inw(data,CSR5) & data->csr5_int_clear,tulip_inw(data,CSR23));
#endif
		
		/* ignore spurious interrupts, but mark them handled */
		if ((status & data->csr7_int_mask) == 0) break;

		if (status & CSR5_rx_intr) {
			rx++;
			rx_intr(data);
		}
		if (status & CSR5_tx_intr) {
			tx++;
			tx_intr(data);
		}
		if (status & GENERAL_TIMER) {
			ETHER_DEBUG(WARN, data->debug, "TIMERPOP \n");
			data->chip_timer = true;
		}
		if (status & CSR5_tx_stop) {
			ETHER_DEBUG(WARN, data->debug, " TxDead \n");

			tulip_outw(data, CSR6, tulip_inw(data,CSR6) | 0x0002);
			tulip_outw(data, CSR6, tulip_inw(data,CSR6) | 0x2002);
		}
		if (status & CSR5_rx_stop) {
			ETHER_DEBUG(WARN, data->debug, " RxDead \n");
			tulip_outw(data, CSR6, tulip_inw(data,CSR6) | 0x2002);
		}
		if (status & CSR5_tx_underflow) {
			uint32 csr6_now;
			if (((csr6_now = tulip_inw(data,CSR6)) & 0xC000) != 0xC000) {
				ETHER_DEBUG(WARN, data->debug, " tx_underflow incr %x by 0x4000\n", csr6_now);
				tulip_outw(data, CSR6, csr6_now + 0x4000);	/* increase tx threshold */
			} else {
				ETHER_DEBUG(WARN, data->debug, " tx_underflow - Store and Forward\n", csr6_now);
				tulip_outw(data, CSR6, csr6_now | 0x00200000);
			}
			tulip_outw(data, CSR6, tulip_inw(data,CSR6) | 0x0002);
			tulip_outw(data, CSR6, tulip_inw(data,CSR6) | 0x2002);   /* restart transmitter */
		}
		if (status & (CSR5_link | CSR5_link_fail)) {
			ETHER_DEBUG(WARN, data->debug, " link change %x\n", status);
			if (data->device_id == DEC_21143) {
				LinkChange(data);
			}
		}
		
	}
	
	if (i <= 0)
	{
			ETHER_DEBUG(ERR, data->debug, "interrupt: work limit %08x \n", status);
			tulip_outw(data, CSR5, 0xC01ffff);  /* clear all possible */
	}
#if CHECK_INTERRUPTS
	if (handled == B_UNHANDLED_INTERRUPT) {
		dprintf("EXIT:status =%x csr5=%x csr7intmask=%x csr5_int_clear=%x\n", status,
				tulip_inw(data,CSR5), data->csr7_int_mask, data->csr5_int_clear);
	}		
#endif
	return handled;
}



/*
 * Reset the card to get it into a known state
 */
static void
tulip_reset(tulip_private_t *data)
{

	/* select the MII port before reseting - don't ask why */
	if ((data->device_id == DEC_21140) || (data->device_id == DEC_21143)
		|| (data->vendor_id == VENDOR_LITEON)) {
		tulip_outw(data, CSR6, 0x00040000);
	}
snooze(10);
	/* reset the chip */
	tulip_outw(data, CSR0_BUS_MODE, CSR0_reset);
	/* delay for at least 50 PCI bus cycles */
snooze(10);

}

/*
 * Determine which media (10BaseT/AUI) we are attached to
 */
static status_t
media_select(
			 tulip_private_t *data
			 )
{
	int count;
	status_t err;

	for (count = 0; count < 20; count++) {
		err = media_try(data);
		if (err < B_NO_ERROR) {
			ETHER_DEBUG(WARN, data->debug, "Error trying media\n");
			return (err);
		}
		if (err == B_NO_ERROR) {
			ETHER_DEBUG(WARN, data->debug, "No error: we found something\n");
			break;
		}
		ETHER_DEBUG(WARN, data->debug, "Keep on trying...\n");
		if (count > 1) {
			int seconds = min_c(count, 10);
			ETHER_DEBUG(WARN, data->debug, "Is your ethernet cable plugged in?\n");

			err = acquire_sem_etc(data->snooze_sem, 1, B_TIMEOUT, seconds * 1000000);
			if (err != B_TIMED_OUT) {
				/*
				 * The driver is probably closing
				 */
				return (B_ERROR);
			}
			/*
			 * We expect to timeout
			 */
		}
	}
	ETHER_DEBUG(INFO, data->debug, "Link is up\n");
	data->linkStatus = true; // link is up.
	return (B_NO_ERROR);
}

void phy_reset(tulip_private_t *data, media_t *m) {

		/* reset the PHY interface as necessary */
		if(m->media_flags & PHYRESET) { 
			char k=0;
			while (m->PhyReset[k].rw == HW_WRITE) {
				tulip_outw(data,CSR15,m->PhyReset[k].command << 16);
				snooze(40000);
				k++;
			}
		}
}

void gpr_reset(tulip_private_t *data, media_t *m) {

		/* reset the PHY interface as necessary */
		if(m->media_flags & GP) { 
			char k=0;
			while (m->gp[k].rw == HW_WRITE) {
				tulip_outw(data,CSR15,m->gp[k].command << 16);
				k++;
			}
		} else {
			dprintf("gpr_reset: odd, gp reset block w/o flag set\n");
		}
}





/* MII transceiver control section.
   Read and write the MII registers using software-generated serial
   MDIO protocol.  See the MII specifications or DP83840A data sheet
   for details. */

/* The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues or future 66Mhz PCI. */
#define mdio_delay() tulip_inw(data, CSR9_ETHER_ROM)

/* Read and write the MII registers using software-generated serial
   MDIO protocol.  It is just different enough from the EEPROM protocol
   to not share code.  The maxium data clock rate is 2.5 Mhz. */
#define MDIO_SHIFT_CLK	0x10000
#define MDIO_DATA_WRITE0 0x00000
#define MDIO_DATA_WRITE1 0x20000
#define MDIO_ENB		0x00000		/* Ignore the 0x02000 databook setting. */
#define MDIO_ENB_IN		0x40000
#define MDIO_DATA_READ	0x80000

static int mdio_read(tulip_private_t *data, int phy_id, int location)
{
	int i;
	int read_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int retval = 0;

	if (data->dev_table_index == PNIC) {
		int i = 1000;

		tulip_outw(data, 0xA0, 0x60020000 + (phy_id<<23) + (location<<18));

		while (--i > 0)
			if ( ! ((retval = tulip_inw(data, 0xA0)) & 0x80000000)) {
				snooze(1000);
				retval= tulip_inw(data,0xA0);
				return retval & 0xffff;
			}
		return 0xffff;
	}

	/* Establish sync by sending at least 32 logic ones. */ 
	for (i = 32; i >= 0; i--) {
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | MDIO_DATA_WRITE1);
		mdio_delay();
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? MDIO_DATA_WRITE1 : 0;
		
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | dataval);
		mdio_delay();
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | dataval | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB_IN);
		mdio_delay();
		retval = (retval << 1) | ((tulip_inw(data, CSR9_ETHER_ROM) & MDIO_DATA_READ) ? 1 : 0);
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB_IN | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	return (retval>>1) & 0xffff;
}

static void mdio_write(tulip_private_t *data, int phy_id, int location, int value)
{
	int i;
	int cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;
	
#if TRACE_IO
		dprintf("mdio_write: phy=%x loc=%x val=%x\n", phy_id,location,value);
#endif
	if (data->dev_table_index == PNIC) {
		int i = 1000;
		tulip_outw(data, 0xA0, cmd);
		do
			if ( ! (tulip_inw(data, 0xA0) & 0x80000000))
				break;
		while (--i > 0);
			if (i<=0) dprintf("tulip: check mdio_write\n");
		return;
	}

	/* Establish sync by sending 32 logic ones. */ 
	for (i = 32; i >= 0; i--) {
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | MDIO_DATA_WRITE1);
		mdio_delay();
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? MDIO_DATA_WRITE1 : 0;
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | dataval);
		mdio_delay();
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB | dataval | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB_IN);
		mdio_delay();
		tulip_outw(data, CSR9_ETHER_ROM, MDIO_ENB_IN | MDIO_SHIFT_CLK);
		mdio_delay();
	}
	return;
}

void find_mii(tulip_private_t *data) {
	int phy;

	data->is_mii = false;

	for (phy = 0; phy < 32; phy++) {
		int mii_status = mdio_read(data, phy, 1);
		if (mii_status != 0xffff  &&  mii_status != 0x0000) {
			int mii_control = mdio_read(data, phy, 0);
			int mii_reg2 = mdio_read(data, phy, 2);
			int mii_reg3 = mdio_read(data, phy, 3);
			int mii_reg4 = mdio_read(data, phy, 4);
			int mii_reg5 = mdio_read(data, phy, 5);

			/* if too many things are the same, its probably not real */			
			if ((mii_status == mii_control) && (mii_control == mii_reg2) && (mii_reg2 == mii_reg3) &&
				(mii_reg3 == mii_reg4) && (mii_reg4 == mii_reg5)) {
					dprintf("find_mii: suspicious hardware\n");
					continue;
			}
			data->is_mii = true;
			data->mii_phy_id = phy;
			ETHER_DEBUG(INFO, data->debug, "find_mii: Found phy= "
					   "%d, control %4.4x status %4.4x ID_1 %4.4x ID_2 %4.4x autoN %4.4x linkP %4.4x.\n",
					    phy, mii_control, mii_status, mii_reg2, mii_reg3, mii_reg4, mii_reg5);
			break;
		}
	}
}

void dump_mii(tulip_private_t *data) {
	int phy;

	if (data->debug & WARN) {

		for (phy = 0; phy < 32; phy++) {
			int mii_status = mdio_read(data, phy, 1);
			if (mii_status != 0xffff  &&  mii_status != 0x0000) {
								
				int mii_control = mdio_read(data, phy, 0);
				int mii_reg2 = mdio_read(data, phy, 2);
				int mii_reg3 = mdio_read(data, phy, 3);
				int mii_reg4 = mdio_read(data, phy, 4);
				int mii_reg5 = mdio_read(data, phy, 5);
				
				dprintf("dump_mii: phy= "
					   "%d, control %4.4x status %4.4x ID_1 %4.4x ID_2 %4.4x autoN %4.4x linkP %4.4x.\n",
					    phy, mii_control, mii_status, mii_reg2, mii_reg3, mii_reg4, mii_reg5);
			}
		}
	}
}

/*
	Translate the link code word to a speed and duplex setting,
	and dial up the CSR registers.
	On entry data->LinkCodeWord, and data->is_mii must be set.
	On exit data->selected media is set, and chip registers set.
*/
void set_speed_and_duplex(tulip_private_t *data) {
	
	unsigned long duplex = 0;
	
	
	if (data->LinkCodeWord & FD100BaseTX) data->selected_media = FD100BaseTX;
	else if (data->LinkCodeWord & HD100BaseTX) data->selected_media = HD100BaseTX;
	else if (data->LinkCodeWord & FD10BaseT) data->selected_media = FD10BaseT;
	else if (data->LinkCodeWord & HD10BaseT) data->selected_media = HD10BaseT;
	else {
		dprintf("Strange LinkCodeWord %x \n", data->LinkCodeWord);
		data->selected_media = HD10BaseT;	// default to 10MB hd
	};

	if ((data->selected_media == FD10BaseT) || (data->selected_media ==FD100BaseTX))
		duplex = 0x200;
	
	if (data->is_mii) {   // port is MII, dial it up...	
		
		if (data->device_id == DEC_21143 ) {
			
			if ((data->selected_media == FD10BaseT) || (data->selected_media == HD10BaseT)) {
				mdio_write(data,data->mii_phy_id,0,duplex?0x1100:0x1000); // AutoN 100MB FD
				tulip_outw(data,CSR6, 0x82420000 | duplex);  /* SC | MBO | TTM | CA */
				tulip_outw(data, CSR13, 0);
				tulip_outw(data, CSR14, 0x7f3f);	
				tulip_outw(data, CSR15, 8);
				tulip_outw(data, CSR13, 1);
				tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 0x40000 | duplex);
			} else {
				// 100 MB
				mdio_write(data,data->mii_phy_id,0,duplex?0x3100:0x3000); // AutoN 100MB FD
				tulip_outw(data,CSR6, 0x82020000); 
				tulip_outw(data, CSR13, 0);
				tulip_outw(data, CSR14, 0);
				tulip_outw(data,CSR6, 0x820E0000 | duplex);
	
			}
		} else if (data->device_id == DEC_21140) {
//dprintf(" dialing up 21140 - add init seq & reset seq cmd list execution\n");

//			if (m->flags & CSR12_21140) {
//				if (data->debug & WARN) dprintf("set_speed_duplex:setting i/o direction 21140 CSR12 <- %x\n", m->csr12_21140 | 0x100);			
//				tulip_outw(data,CSR12, m->csr12_21140 | 0x100);
//			}
			/* check for phy reset or gpr reset & execute */
			if ((data->selected_media == FD10BaseT) || (data->selected_media == HD10BaseT)) {
				ETHER_DEBUG(WARN, data->debug, "21140 setting 10MB - to do: use value from srom \n\n");
				// 10 MB
//				mdio_write(data,data->mii_phy_id,0,duplex?0x0100:0x0000); // AutoN 100MB FD
				tulip_outw(data,CSR6, 0x320e0000 | duplex);  /*  */
			} else {
				ETHER_DEBUG(WARN, data->debug, "21140 setting 100MB - to do: use value from srom \n");
				// 100 MB
//				mdio_write(data,data->mii_phy_id,0,duplex?0x3100:0x3000); // AutoN 100MB FD
				tulip_outw(data,CSR6, 0x320E0000 | duplex);
			}
			
		
		} else {
			ETHER_DEBUG(ERR, data->debug, "Can't Happen %x\n", data->device_id);
		}
		// go mii port
		// start xmitter & receiver
		tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 0x2002);

//		dump_mii(data);
		return;
	}
	
	// port is SYM, dial it up
	
	if ((data->selected_media == FD10BaseT) || (data->selected_media == HD10BaseT)) {
		tulip_outw(data, CSR6_OP_MODE,(1<<31)|(1<<25)|(1<<13)|(1<<1) | 0x2002 | duplex);
		tulip_outw(data, CSR13_SIA_CONN, 0);
		tulip_outw(data, CSR13_SIA_CONN, 1);	// endable encoder
		tulip_outw(data, CSR14_SIA_TX_RX, 0x7f3f);	
	} else {
		// 100 MB
		tulip_outw(data, CSR6_OP_MODE,(1<<31)|(1<<25)| (1<<24) | (1<<23) | (1<<18) | (1<<17)|(1<<1) | 0x2002 | duplex);
		tulip_outw(data, CSR13_SIA_CONN, 0);
		tulip_outw(data, CSR14_SIA_TX_RX, 0);	
	}
}

/* The link has gone up or down.
   Called from interrupt -
*/
static void LinkChange(tulip_private_t *data) {
	
	ETHER_DEBUG(WARN, data->debug, "LinkChange() to do...\n");
	dump_csr(data);

}


/*
	Start the 21143 autonegotiate, and at the same time start
	the chips on board timer. If the network cable is plugged
	into an interface that supports NWAY, auto negotiate will
	complete and we'll pick the best speed and duplex that both
	link partners support. If the interface we are connected to
	doesn't support NWAY, we will time out, and check the link
	heart beat for 10 or 100 MB speed, and default to half duplex.
	Returns zero if NWay succeded otherwise returns false;
*/
static long AutoNegotiate(tulip_private_t *data) {
	unsigned long SiaStatus;
	const unsigned long NWAY_STATE = 0x7000;  
	
	int limit = 0;

	// set up for 10 MB HD before starting NWAY
	tulip_outw(data, CSR6, 0x82420200);  // start at 10 fd mb
	tulip_outw(data, CSR13, 1);
	tulip_outw(data, CSR14, 0x3ffff); 	// advertise we do all media 100TX-FD ... 10BT HD
// This write to csr15 breaks the 21143 cards - to do - check to see if there is a reason to have it.
//	tulip_outw(data, CSR15, 8); 		
	tulip_outw(data, CSR13, 1);
	tulip_outw(data, CSR12, 0x1301);	// start auto negotiation by setting state
	
	data->chip_timer = false;
	tulip_outw(data, CSR11, 0x1d00);	// about 6 seconds

	while (!data->chip_timer) {			// interrupt routine sets this when the timer pops
		if (limit++ > 8) break;		// limit * snooze value = 1.6 seconds - failsafe in case interrupt timer stalls
		snooze(200000);	
		SiaStatus = tulip_inw(data, CSR12);
		if ((SiaStatus & NWAY_STATE) > 0x4000) {	// reached a completion state
			data->LinkCodeWord = SiaStatus >> 16;
			data->chip_timer = false;				// just in case of race condition
			break;
		} else { if (data->debug & WARN) dump_csr(data); }
	}

	if (data->debug & WARN) {
		dump_csr(data);
		dprintf("Auto: LinkCodeWord=%x chip_timer is %s\n",data->LinkCodeWord, data->chip_timer?"true":"false");
	}
	tulip_outw(data, CSR11, 0);				// turn off timer
	if ((data->chip_timer) || (data->LinkCodeWord == 0)) {
			ETHER_DEBUG(WARN, data->debug, "Link Partner is not NWay\n");
		return (false);
	}

	return (true);
}

/* try each of the media defined in the srom from fastest speed and full duplex down to 
lowest speed */
static long
media_try_2114x(tulip_private_t *data) {
	
	char j;

	// 1st pass thru media looking for phy reset blocks
	for (j=data->media_count-1; j>=0; j--) 			/* step thru the media */
	{
		media_t * m; 
		unsigned char real_media = true;
		
		m = &data->media[j];
		if (m->leaf_type == 5) {
			data->phy_reset_media = m;
			ETHER_DEBUG(WARN, data->debug, "Media leaf type 5 phy reset\n");
			phy_reset(data, m);
			real_media=false; // this leaf node doesn't define a media, its just for initializing the PHY chip
			break;
		}
	}

	// try autonegotiate
	if ((data->device_id == DEC_21143) || (data->device_id == DEC_21145)) {
		if (AutoNegotiate(data)) {	// link partner NWay, yea!
			set_speed_and_duplex(data);
			data->linkStatus = true;
			return 0;  // success
		}
	}
	// try each media one at a time, checking for link heartbeat to verify -- assume half duplex at this point
	for (j=data->media_count-1; j>=0; j--) 			/* step thru the media */
	{
		media_t * m; 
		unsigned char real_media = true;
		
		m = &data->media[j];
		switch (m->leaf_type) {
		case 5:
			// this was done above
			break;
		case 4: 	// SYM media
			{
				const unsigned short ACTIVE_INVALID = 0x8000;
				const unsigned long  CSR6_SETTINGS_MASK = 0x71;			
				const unsigned long  DEFAULT_CSR6 = (1<<31) | (1<<25) | (1<<17) | (1<<15) | (1<<14) | (1<<13) | (1<<1);
				
				if (data->phy_reset_media)		
					phy_reset(data, data->phy_reset_media);
				
				if (m->media_flags & CSR6INFO) {
					/* check active invalid */
					if (m->csr6info & ACTIVE_INVALID)	{
						ETHER_DEBUG(INFO, data->debug, "Active invalid...check duplex & set appropriately\n");
					    /* set csr6 SCR PCS TTM and PS  from the Srom Info */
						tulip_outw(data,CSR6, DEFAULT_CSR6 | ((m->csr6info & 0x71) << 18));		
						tulip_outw(data, CSR13, 0);
						tulip_outw(data, CSR14, 0);
									
					} else {		/* check default_medium, media sense bit & polarity */
						ETHER_DEBUG(INFO, data->debug, "tulip: check media sense bit <GPx> with polarity value <%x>\n",m->csr6info);
					}
				}		
			}		
			break;
		case 3:	/* MII PHY media */
		case 1:	/* MII PHY media  21140  */

//dprintf("remove this dead code\n");
#if 0	
		if (data->debug ) dprintf("clear undefined bits in CSR15\n");
		{ unsigned long csr15 = tulip_inw(data, CSR15);
			tulip_outw(data, CSR15, csr15 & ~(0x800037c0));
		}
#endif
		if (data->debug & WARN ) {
			dprintf("before phy reset\n"); dump_mii(data);
		}
		phy_reset(data, m);
	
		if (data->debug & WARN ) {
			dprintf("after phy reset\n"); dump_mii(data);
		}
		
		{ // find PHY and dump the info...
			int phy;
	
			/* Find any connected MII interfaces. */
			for (phy = 0; phy < 32; phy++) {
				int mii_status = mdio_read(data, phy, 1);
				if (mii_status != 0xffff  &&  mii_status != 0x0000) {
					int mii_control = mdio_read(data, phy, 0);
					int mii_reg2 = mdio_read(data, phy, 2);
					int mii_reg3 = mdio_read(data, phy, 3);
					int mii_reg4 = mdio_read(data, phy, 4);
					int mii_reg5 = mdio_read(data, phy, 5);
	
	
					data->mii_phy_id = phy;
					if (mii_reg5 != 0) {	// link partner found advertising capabilites
						data->LinkCodeWord = mii_reg5;
						set_speed_and_duplex(data);
						data->linkStatus = true;
						return (0);
					} else {				// link partner is not nway, try 100 hd, 10hd
						ETHER_DEBUG(WARN, data->debug, "media_try_2114x: Faking it with just 10hd, add 100hd & test here later\n");
						data->LinkCodeWord = HD10BaseT;
						set_speed_and_duplex(data);
						ETHER_DEBUG(WARN, data->debug, "media_try_2114x: add check for link heartbeat here\n");
						data->linkStatus = true;
						return(0);
					}
				}
			}
		}
		break;
		case 2:	// SIA Media
			ETHER_DEBUG(WARN, data->debug, "leaf 2, try 10MB \n");
			// try 10 MB hd

			if (data->phy_reset_media)		
				phy_reset(data, data->phy_reset_media);

			if (m->media_flags & GP) {
				execute_cmds(data, m->gp);
			}

//			tulip_outw(data, CSR6_OP_MODE,(1<<31)|(1<<25)|(1<<13)|(1<<1) | 0x2002);
			tulip_outw(data, CSR6_OP_MODE,(1<<31)|(1<<25)|(1<<13)|(1<<1));
			tulip_outw(data, CSR13_SIA_CONN, 0);
			snooze(20000);
			tulip_outw(data, CSR13_SIA_CONN, 1);	// endable encoder
			snooze(20000);
			tulip_outw(data, CSR14_SIA_TX_RX, 0x7f3f);	
			break;
		
		case 7: /* ask Intel for documentation, currently we don't know */
			m->media_code = 0; /* force to no-op in next switch(m->media_code) case */
			break;
		default:
			ETHER_DEBUG(WARN, data->debug, "media_try_2114x: unknown media type %x\n", m->leaf_type);
			break;
		}		

		switch (m->media_code) {
		case 0: break; 
		case 3:
		case 5:
			if (check_100MB_link_pulse(data))	{
				data->linkStatus = true;
				return (0);	// success
			}
			break;
		default:
			ETHER_DEBUG(INFO, data->debug, "media_try_2114x: media_code = %x\n",m->media_code);
			if (check_10MB_link_pulse(data)) {
				data->linkStatus = true;
				return (0);	// success
			}
		}

	}	// end of for loop

	return B_NO_ERROR; // link is down but interface intitialized	
}


#define PHONE_LAN true
#ifdef PHONE_LAN

// Commands for SPI
#define SET_WRITE_ENABLE_CMD   	0x00000006
#define CLEAR_WRITE_ENABLE_CMD  0x00000004
#define READ_BYTE_CMD    		0x00000003
#define WRITE_BYTE_CMD 			0x00000002

// CSR9 serial peripheral interface - aka phone lan physical interface
#define SPI_CS    	0x00100000 	// 1 << 20
#define SPI_SCLK   	0x00200000 	// 1 << 21
#define SPI_SI  	22	// serial periperal interface serial in
#define SPI_SO 		23	//                            serial out
#define SR_DI 		2	// serial data in
#define SR_DO  		3	// serial data out

// See Appendix G of the "Intel 21145 Phoneline/Ethernet LAN Controller
// to read/write data from/to the PHY interface.
static void  write_pna_phy(tulip_private_t *data, uchar phy_reg, uchar phy_data )
{
 int   j;

	ETHER_DEBUG(WARN, data->debug, "[%2x]<=%2x  ", phy_reg, phy_data);

 	//SET_WRITE_ENABLE
 	tulip_outw(data, CSR9, 0);                      mdio_delay();
 	tulip_outw(data, CSR9, SPI_CS);                 mdio_delay();
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (((SET_WRITE_ENABLE_CMD >> j) & 1L) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);            mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);   mdio_delay();
	}
	tulip_outw(data, CSR9, 0);                      mdio_delay();
	tulip_outw(data, CSR9, SPI_SCLK);               mdio_delay();

 	// WRITE_BYTE CMD
 	tulip_outw(data, CSR9, 0);                      mdio_delay();
 	tulip_outw(data, CSR9, SPI_CS);                 mdio_delay();
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (uint32) (((WRITE_BYTE_CMD >> j) &1L) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);            mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);   mdio_delay();
	}

	// dial up the register
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (uint32) (((phy_reg >> j) & 1L) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);           mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);  mdio_delay();
	}
		
	// output the data
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (uint32)(((phy_data >> j) & 1L) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);           mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);  mdio_delay();
	}
	tulip_outw(data, CSR9, 0);                     mdio_delay();
	tulip_outw(data, CSR9, SPI_SCLK);
}

static uchar  read_pna_phy(tulip_private_t *data, uchar phy_reg)
{
uint32 Data = 0; uint32 DataBit = 0;
int   j;

 	tulip_outw(data, CSR9, 0);                     mdio_delay();
 	tulip_outw(data, CSR9, SPI_CS);                mdio_delay();
 
	// CLEAR_WRITE_ENABLE
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (((CLEAR_WRITE_ENABLE_CMD >> j) & 0x00000001) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);           mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);  mdio_delay();
	}
	tulip_outw(data, CSR9, 0);                     mdio_delay();
	tulip_outw(data, CSR9, SPI_SCLK);              mdio_delay();

 	// READ_BYTE CMD
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (((READ_BYTE_CMD >> j) & 0x00000001) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);           mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);  mdio_delay();
	}

	// dial up the register
 	for (j=7; j>=0; j--) {
		uint32 cmd_out = SPI_CS | (((phy_reg >> j) & 0x00000001) << SPI_SI);
		tulip_outw(data, CSR9, cmd_out);           mdio_delay();
		tulip_outw(data, CSR9, SPI_SCLK|cmd_out);  mdio_delay();
	}
	
	// read the data
 	for (j=7; j>=0; j--) {
 		tulip_outw(data, CSR9, SPI_CS);            mdio_delay();
		DataBit = tulip_inw(data, CSR9);           mdio_delay();
		Data |= (((DataBit >> SPI_SO) & 0x00000001) << j);
		tulip_outw(data, CSR9, SPI_CS | SPI_SCLK); mdio_delay();
	}
	tulip_outw(data, CSR9, 0);                     mdio_delay();
	tulip_outw(data, CSR9, SPI_SCLK);

	return Data;
}


struct phy_tuple {
		uchar phy_reg;
		uchar phy_value;
};

// These are the power up default settings, we
// run thru the registers and set them anyway
// because we're paranoid.
static const struct phy_tuple home_pna_tbl[] = {
			{0x00, 0x04},		// control lo
			{0x01, 0x00},		// control hi
			{0x04, 0x00},		// interrupt mask
			{0x05, 0x00},		// 
			{0x08, 0x00},		// tx-pcom
			{0x09, 0x00},		// 
			{0x0A, 0x00},		// 
			{0x0B, 0x00},		// 
			{0x12, 0x10},		// noise floor
			{0x13, 0xD0},		// noise ceiling
			{0x14, 0xf4},		// noise attack
			{0x15, 0xff},		// noise event
			{0x1A, 0x14},		// aid interval
			{0x1B, 0x40},		// aid isbi
			{0x1C, 0x2c},		// isbi slow
			{0x1D, 0x1c},		// isbi fast
			{0x1E, 0x04},		// tx pulse width
			{0x1F, 0x44},		// tx pulse cycles
			{0xFF,	  0}		// terminator
};

void init_phonelan_phy(tulip_private_t *data) {
	uchar j;
	
	// dump the registers before if debug is on
	if (data->debug & WARN) {
		dprintf("-----PNA before write------");
		for (j=0; j<32; j++) {
			if (! ( j & 7)) dprintf("\n");
			dprintf("[%2x]=%2x  ",j, read_pna_phy(data, j));		
		}	
		dprintf("\n");
	}
	// initialize the regiesters - if debug is on, show the values
	for (j=0; home_pna_tbl[j].phy_reg != 0xFF; j++) {
		if (data->debug & WARN) if (! (j & 7)) dprintf("\n");
		write_pna_phy(data, home_pna_tbl[j].phy_reg,home_pna_tbl[j].phy_value);		
	}	

	// dump the registers after the write if debugging is on
	if (data->debug & WARN) {
		dprintf("\n-----After PNA write------");
		for (j=0; j<32; j++) {
			if (! ( j & 7)) dprintf("\n");
			dprintf("[%2x]=%2x  ",j, read_pna_phy(data, j));		
		}	
		dprintf("\n");
	}
}

uchar init_phonelan(tulip_private_t *data) {

//	setupHomeRunDefaults( mac, SpeedAndPower ) ;
	init_phonelan_phy(data) ;
//	tulip_outw(data, CSR6,  0x820A0040);
	tulip_outw(data, CSR6,  0x820A0000);
	tulip_outw(data, CSR13, 0x30480000);
	tulip_outw(data, CSR14, 0x00000705);
	tulip_outw(data, CSR15, 0x0000001e);
	tulip_outw(data, CSR13, 0x30480009);	
	
	// add code to verify link is good here...
	return true;
}

long media_try_21145(tulip_private_t *data) {
	
	int j = 0;
	
	if (data->linkStatus) {
		ETHER_DEBUG(WARN, data->debug, "21145 link already up\n");
	}
	
	for (j=data->media_count-1; j>=0; j--) 			/* step thru the media */
	{
		media_t * m = &data->media[j];
		ETHER_DEBUG(INFO, data->debug, "21145 Leaf type %x\n", m->leaf_type);
		switch (m->leaf_type) {
		case 2:
			dprintf("21145 type 2 media\n");
			if (m->media_code == 0x12) {  	  		/* phone lan */
				if (!init_phonelan(data)) break;
			}
			if (m->media_flags & GP) {
				execute_cmds(data, m->gp);			/* initialize the GEP I/O pins */
			}
			ETHER_DEBUG(INFO, data->debug,"21145 phoneLan intitialized OK!\n");
			return true;
		}
		break;	
	}	
	ETHER_DEBUG(INFO, data->debug, "21145 intit FAIL!\n");
	return true;	
}


#endif // PHONE_LAN


/* 
 * Initialize the hardware
 */
static status_t
hw_init(tulip_private_t *data)
{
	int i;
	status_t err;
	short cmd;

	/* enable memory transactions, bus mastering */
	cmd = (gPCIModInfo->read_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, PCI_command, 2);
	(gPCIModInfo->write_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, 
			 PCI_command, 2, cmd | PCI_command_memory | PCI_command_master);

#if 0
	ulong cflt = (gPCIModInfo->read_pci_config)(data->bus, data->device, data->function, 
						   PCI_latency, 1);
	cflt &= 0xff;
	if (cflt < 100) {
		dprintf("Modifying clocks from %d to %d\n", cflt, 100);
		cflt = 100; 
		(gPCIModInfo->write_pci_config)(data->bus, data->device, data->function, 
						 PCI_latency, 1, cflt);
	}
#endif
	if ((data->device_id == DEC_21143) || (data->device_id == DEC_21140)  || (data->device_id == DEC_21145)
		|| (data->vendor_id == VENDOR_LITEON)) // wake from default sleep mode
	{
		long cfdd;  // configuration device and driver area register
		cfdd = (gPCIModInfo->read_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, 0x40, 4);
		
		ETHER_DEBUG(WARN, data->debug, "hw_init:Sleeping if high bits on %x\n", cfdd);
		(gPCIModInfo->write_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, 
					 0x40, 4, cfdd & 0x4FFFFFFF);
		cfdd = (gPCIModInfo->read_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, 0x40, 4);
		ETHER_DEBUG(WARN, data->debug, "hw_init: Woke up 2114x from sleep %x\n", cfdd);

		if (data->vendor_id != VENDOR_LITEON)
			if (parse_srom(data) != 0) {
				ETHER_DEBUG(WARN, data->debug, "add code to set defaults here \n");
		}
	}
	
	/*
	 * reset the card
	 */
	tulip_reset(data);

	/* Macs must run with this burst mode... */
	if (find_area("mac_io") >= 0) {
		tulip_outw(data, CSR0_BUS_MODE, CSR0_burst1);
	}
#if __INTEL__
	/*
	 * Data gets corrupted for burst sizes greater than 1 longword
	 * Something is wrong: Intel should do PCI as good as a BeBox.
	 */
	/*	tulip_outw(data, CSR0_BUS_MODE, CSR0_burst1);
	// I don't beleive this...use a reasonable burst for pentium machines - RHS. */
//	tulip_outw(data, CSR0_BUS_MODE, 0x1A04800);			// this works a little better
	tulip_outw(data, CSR0_BUS_MODE, 0x1A08000);	// Write Invalidate, Read Line Enable, Read Multiple Enable,
												// Cache Alignment=2
#endif /* __INTEL__ */

	switch (data->dev_table_index) {
		case DEC21041:
			if (tulip_inw(data, CSR9_ETHER_ROM) & CSR9_21040_mode) {
				ETHER_DEBUG(WARN, data->debug, "Using 21040 software compatibility mode\n");
				data->device_id = DEC_21040;
				data->dev_table_index = DEC21040;
			} /* fall thru */
		case DEC21040:
			err = read_address(data);
			if (err != B_NO_ERROR) {
				dprintf("read address failed\n");
				return (err);
			}
			tulip_outw(data, CSR3_RX_LIST, (uint32)gPCIModInfo->ram_address(&data->rx_pring[0]));
			tulip_outw(data, CSR4_TX_LIST, (uint32)gPCIModInfo->ram_address(&data->tx_pring[0]));
	
			media_init(data);
			/* CSR15 defaults to 10baseT */
			tulip_outw(data, CSR6_OP_MODE, 
				   ((tulip_inw(data, CSR6_OP_MODE) & ~CSR6_promisc)
					| CSR6_tx_start | CSR6_rx_start));
			
			 /* setup addresses to recognize */
			tulip_outw(data, CSR7_INTR_MASK, CSR7_tx_int);
			err = do_setup_frame(data);
			if (err < B_NO_ERROR) {
				return (err);
			}
			tulip_outw(data, CSR7_INTR_MASK, 0);
			 /* Reset our rings to zero: things work much better this way */
			reset_to_zero(data);
		
			 /* Determine media type */
			err = media_select(data);
			if (err < B_NO_ERROR) {
				dprintf("media select failed\n");
				return (err);
			}
			/* turn on interrupts */
			tulip_outw(data, CSR7_INTR_MASK,
				chip_stuff[data->dev_table_index].csr7_int_mask);
			break;
		case DEC21140:			
		case DEC21143:
			reset_to_zero(data);
			
			tulip_outw(data, CSR5,
				chip_stuff[data->dev_table_index].csr7_int_mask);
			tulip_outw(data, CSR7,
				chip_stuff[data->dev_table_index].csr7_int_mask);
			err = media_try_2114x(data);
			if (err) {
				ETHER_DEBUG(INFO, data->debug, "media_try_2114x failed\n");
				return (B_NO_ERROR);
			}
			err = do_setup_frame(data);
			if (err < B_NO_ERROR) {
				return (err);
			}
			break;
		case DEC21145:
			reset_to_zero(data);
			
			if (media_try_2114x(data) == 0) {
				ETHER_DEBUG(INFO, data->debug, "21145 found link\n");
			}	else {
				media_try_21145(data);
			}			
			tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 2);  /* turn on rcvr */
			tulip_outw(data, CSR1, 0);							/* rcv poll demand */
			
			tulip_outw(data, CSR5,
				chip_stuff[data->dev_table_index].csr7_int_mask); /* interrupts on */
			tulip_outw(data, CSR7,
				chip_stuff[data->dev_table_index].csr7_int_mask);

			err = do_setup_frame(data);							/* turns on xmitter */
			if (err < B_NO_ERROR) {
				return (err);
			}
		
			break;
		case PNIC: 	
		case PNIC_II: 	
			reset_to_zero(data);
			if (data->dev_table_index == PNIC)
				read_address_LiteOn(data);
			else 
				parse_srom(data);
			
			find_mii(data);
			if (data->is_mii) {

				ETHER_DEBUG(WARN, data->debug, "Lite-On MII \n");
				mdio_write(data,data->mii_phy_id,4, 0x01e1); /* advertise all */
				if ((mdio_read(data,data->mii_phy_id,1) & 0x20) == 0)  /* autonegotiate not complete */
				{
					int16 limit;
					mdio_write(data,data->mii_phy_id,0, 0x200); /* restart auto negotiate */
					while (((mdio_read(data,data->mii_phy_id,1) & 0x24) == 0) && (limit++ < 200))  /* auto not complete, no link, 2 sec limit*/
						snooze(10000);
					if (data->debug > 1)
						dprintf("mii - status = %x limit=%d %d ms\n",mdio_read(data,data->mii_phy_id,1),limit, limit * 1000);
				}
				{
					uint16 mii_status = mdio_read(data,data->mii_phy_id,1);
					uint16 link_partner = mdio_read(data,data->mii_phy_id,5);
					
					link_partner &= 0x1F8; 		/* mask off other bits which may or may not be set right */
					if (mii_status & 0x20) {	/* auto neg complete */
						if (link_partner) {		/* partner has advertised his ability */
							ETHER_DEBUG(INFO, data->debug, "found link partner %x\n", link_partner);
							switch(link_partner) {
							case 0x1e0:
								ETHER_DEBUG(WARN, data->debug, "MII 100MB FD\n");
								mdio_write(data,data->mii_phy_id,0, 0x3100);	/* 100 MB FD */
								tulip_outw(data, CSR6, 0x810c0200);
								break;
							case 0x60:
								ETHER_DEBUG(WARN, data->debug, "MII 10MB FD\n");
								mdio_write(data,data->mii_phy_id,0, 0x1100);	/* 10 MB FD */
								tulip_outw(data, CSR6, 0x810c0200);
								break;
							case 0xa0: 
							case 0x80: 
								ETHER_DEBUG(WARN, data->debug, "MII 100 HB FD\n");
								mdio_write(data,data->mii_phy_id,0, 0x3000);	/* 100 MB HD */
								tulip_outw(data, CSR6, 0x810c0000);
								break;
							default :
								ETHER_DEBUG(WARN, data->debug, "tulip: unknown link partner %x \n",link_partner);
								/* fall thru to 10MB HD */
							case 0x20:
								ETHER_DEBUG(WARN, data->debug, "MII 10MB HD\n");
								mdio_write(data,data->mii_phy_id,0, 0x1000);	/* 10 MB HD */
								tulip_outw(data, CSR6, 0x804C0000);
							}
						}
					} else { /* no auto negotiate */
						ETHER_DEBUG(WARN, data->debug, "no autoneg - MII 10MB HD\n");
						mdio_write(data,data->mii_phy_id,0, 0x1000);	/* 10 MB HD */
						tulip_outw(data, CSR6, 0x804C0000);
					}
				}
				tulip_outw(data, CSR15, 1);				/* disable xmit jabber timer */

			} else { /* not mii */
				tulip_outw(data, CSR15, 1);				/* disable xmit jabber timer */
				ETHER_DEBUG(INFO, data->debug, "Lite-On NOT MII \n");
				if ( data->dev_table_index == PNIC_II) {
					if (AutoNegotiate(data)) {
					uint32 link_partner = data->LinkCodeWord & 0x1F8;
						switch(link_partner) {
						case 0x1e0:
							ETHER_DEBUG(WARN, data->debug, "PNIC2 100MB FD\n");
							tulip_outw(data, CSR6, 0x83860200);
							break;
						case 0x60:
							ETHER_DEBUG(WARN, data->debug, "PNIC2 10MB FD\n");
							tulip_outw(data, CSR6, 0x82420200);
							break;
						case 0xa0: 
						case 0x80: 
							ETHER_DEBUG(WARN, data->debug, "PNIC2 100MB HD\n");
							tulip_outw(data, CSR6, 0x83860000);
							break;
						default :
							dprintf("tulip: unknown link partner %x \n",link_partner);
							ETHER_DEBUG(WARN, data->debug, "unknown link partner. set 10MB HD %x \n",link_partner);
							/* fall thru to 10MB HD */
						case 0x20:
							tulip_outw(data, CSR6, 0x82420000);
						}
					} else { /* default to 10 MB HD */
					if (data->debug > 1) dprintf("PNIC2 linkp=0 default to 10MB HD\n");
					ETHER_DEBUG(WARN, data->debug, "PNIC2 linkp=0 default to 10MB HD\n");
						tulip_outw(data, CSR6, 0x82420000);
					}
				} else { /* PNIC */ 
					tulip_outw(data,CSR6, 0x420000);
					tulip_outw(data, CSR12, 0x32);
					tulip_outw(data, CSR23, 0x1b078);
//					tulip_outw(data, CSR23, 0x201b078);
				}
			}
			
			/* turn on interrupts */
			tulip_outw(data, CSR5, chip_stuff[data->dev_table_index].csr7_int_mask);
			tulip_outw(data, CSR7, chip_stuff[data->dev_table_index].csr7_int_mask);

			err = do_setup_frame(data);
			if (err < B_NO_ERROR) {
				dprintf("do_setup_frame() failed\n");
				return (err);
			}
			/* Linksys PNIC I sets itself into promisc mode for no reason - clear it here. */
			tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) &  ~(1<<6));	/* turn off promiscuous */
		
			tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 0x2000);	/* turn on xmitter */
			tulip_outw(data, CSR2, 0);								/* xmit poll demand */

			tulip_outw(data,CSR6, (tulip_inw(data,CSR6)) | 0x2002);  /* turn on xmtr & rcvr */
			tulip_outw(data, CSR1, 0);								/* rcv poll demand */

			break;
		default:
			dprintf("Unknown device: vendor=%x device=%x hash=%x\n",
				data->vendor_id, data->device_id, data->dev_table_index);
	}


	return (B_NO_ERROR);
}

/*
 * Initialize our physical mapping table,
 * given a single physical page 
 */
static void
init_pbufs(
		   tulip_pbuf_t *opbuf,
		   unsigned offset,
		   char *addr,
		   unsigned size
		   )
{
	unsigned thisoffset;
	tulip_pbuf_t *pbuf = opbuf;
	unsigned bytes_used;

	thisoffset = 0;
	while (size > 0 && (thisoffset < (offset + size))) {
		if (offset <= thisoffset) {
			if (pbuf->firstlen < 0) {
				pbuf->first = addr;
				if (size >= PACKET_BUF_SIZE) {
					/*
					 * Intialize first and second buffers
					 */
					pbuf->firstlen = PACKET_BUF_SIZE;
					pbuf->secondlen = 0;
				} else {
					/*
					 * Initialize only first buffer
					 */
					pbuf->firstlen = size;
				}
				bytes_used = pbuf->firstlen;
			} else {
				/*
				 * Initialize only second buffer
				 */
				pbuf->second = addr;
				pbuf->secondlen = PACKET_BUF_SIZE - pbuf->firstlen;
				bytes_used = pbuf->secondlen;
			}
			addr += bytes_used;
			size -= bytes_used;
			thisoffset += bytes_used;
			offset += bytes_used;
			pbuf++;
		} else {
			/*
			 * Skip first buffer
			 */
			thisoffset += pbuf->firstlen;
			if (pbuf->secondlen >= 0) {
				/*
				 * Second buffer is initialized: skip to next descriptor
				 */

				thisoffset += pbuf->secondlen;
				pbuf++;
			}
		}
	}
}

/*
 * Map our virtual buffers into physical buffers, and
 * record the resulting buffers and offsets
 */
static int
map_and_init(
			 volatile tulip_ring_t *ring,
			 unsigned ringsize,
			 tulip_vbuf_t *vbuf,
			 tulip_pbuf_t *pbuf
			 )
{
	physical_entry vtoptable[MAX_RING_SIZE + 1];
	int offset;
	int i;

	get_memory_map(vbuf,
				   ringsize * sizeof(tulip_vbuf_t),
				   &vtoptable[0],
				   ringsize + 1);
	for (i = 0; i < ringsize; i++) {
		pbuf[i].first = pbuf[i].second = NULL;
		pbuf[i].firstlen = pbuf[i].secondlen = -1;
	}

	offset = 0;
	for (i = 0; i < ringsize; i++) {
		if (vtoptable[i].size == 0) {
			break;
		}
#if 0
		dprintf("virt: %08x, phys %08x, size %d\n",
				 vbuf, vtoptable[i].address, vtoptable[i].size);
#endif
		vbuf = (tulip_vbuf_t *)(((ulong)vbuf + B_PAGE_SIZE) & ~(B_PAGE_SIZE - 1));
		init_pbufs(pbuf,
				   offset, 
				   (char *)vtoptable[i].address, 
				   vtoptable[i].size);
		offset += vtoptable[i].size;
	}
	if (offset == 0 || (i == ringsize && vtoptable[i].size != 0)) {
		dprintf("error: offset = %d, i = %d\n", offset, i);
		return (0);
	}
#if 0
	for (i = 0; i < ringsize; i++) {
		dprintf("ring %d first: %08x, %d second: %08x, %d\n",
				i, pbuf[i].first, pbuf[i].firstlen,
				pbuf[i].second, pbuf[i].secondlen);
	}
#endif
	return (1);
}

/*
 * Map ring buffers to physical mem
 */
static tulip_ring_t *
map_ring(
		 volatile tulip_ring_t *ring,
		 ulong ringsize
		 )
{
	physical_entry vtoptable[2];

	get_memory_map((tulip_ring_t *)ring,
				   ringsize * sizeof(tulip_ring_t),
				   &vtoptable[0],
				   ringsize + 1);
	if (vtoptable[0].size < (ringsize * sizeof(tulip_ring_t))) {
		dprintf("error: ring crosses page boundary: %d\n",
				vtoptable[0].size);
		return (NULL);
	}
	return ((tulip_ring_t *)vtoptable[0].address);
}

/* 
 * Get physical mappings for all buffers we will DMA
 */
static int
init_bufs(
		  tulip_private_t *data
		  )
{

	if (!map_and_init(data->rx_ring, RX_RING_SIZE, 
					  data->rx_vbuf, data->rx_pbuf)) {
		return (0);
	}
	if (!map_and_init(data->tx_ring, TX_RING_SIZE, 
					  data->tx_vbuf, data->tx_pbuf)) {
		return (0);
	}
	data->rx_pring = map_ring(data->rx_ring, RX_RING_SIZE);
	if (data->rx_pring == NULL) {
		return (0);
	}
	data->tx_pring = map_ring(data->tx_ring, TX_RING_SIZE);
	if (data->tx_pring == NULL) {
		return (0);
	}
	return (1);
}

static char *
aligned_malloc(size_t size, area_id *areap)
{
	char *base;
	area_id id;

	size = RNDUP(size, B_PAGE_SIZE);
	id = create_area("tulip", (void **) &base, B_ANY_KERNEL_ADDRESS,
					 size,
					 B_FULL_LOCK,
					 B_READ_AREA | B_WRITE_AREA);
	if (id < B_NO_ERROR) {
		return (NULL);
	}
	memset(base, 0, size); /* just for grins */
	*areap = id;
	return (base);
}

/*
 * Uninitialize
 */
static void
undo(
	 tulip_private_t *data
	 )
{

	if (data->init) {

		/*
		 * turn off the card
		 */
		tulip_outw(data, CSR7_INTR_MASK, 0);  /* turn off ints first! */

		tulip_outw(data, CSR6, tulip_inw(data,CSR6) & ~0x2002); /* turn off rx & tx */

		/*
		 * Delete sems and free data
		 */
		if (data->rx_sem >= B_NO_ERROR) {
			delete_sem(data->rx_sem);
			data->rx_sem = B_ERROR;
		}
		if (data->tx_sem >= B_NO_ERROR) {
			delete_sem(data->tx_sem);
			data->tx_sem = B_ERROR;
		}
		if (data->snooze_sem >= B_NO_ERROR) {
			delete_sem(data->snooze_sem);
			data->snooze_sem = B_ERROR;
		}
		if (data->ioarea >= B_NO_ERROR) {
			delete_area(data->ioarea);
			data->ioarea = B_ERROR;
		}
		data->init = FALSE;
	}
}


/*
 * Intialize the given card
 */
static status_t
init_card( tulip_private_t *data )
{
	status_t	status;
	int i;
	ulong phys;
	ulong size;


	phys = data->pciInfo->u.h0.base_registers[1];
	size = data->pciInfo->u.h0.base_register_sizes[1];

	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);	/* round up */

	data->ioarea = map_physical_memory("tulip_regs",
									   /* round down... */
									   (void *)(phys & ~(B_PAGE_SIZE - 1)),
									   size,
									   B_ANY_KERNEL_ADDRESS,
									   B_READ_AREA + B_WRITE_AREA,
									   (void **) &data->ioaddr);

	if (data->ioarea < 0) {
		dprintf("Problems mapping device regs: %08x\n", data->ioarea);
		return (B_ERROR);
	}
	/* correct for offset w/in page */
	data->ioaddr += (phys & (B_PAGE_SIZE - 1)) / sizeof(ulong);
	
	data->init = TRUE;
	data->rx_sem = B_ERROR;
	data->tx_sem = B_ERROR;
	data->snooze_sem = B_ERROR;
	data->vendor_id = data->pciInfo->vendor_id;
	data->device_id = data->pciInfo->device_id;
	data->dev_table_index = hash_dev(data->vendor_id,data->device_id);
	data->csr5_int_clear = chip_stuff[data->dev_table_index].csr5_int_clear;
	data->csr7_int_mask = chip_stuff[data->dev_table_index].csr7_int_mask;
	data->interrupts = 0;
	data->tx_ringx = 0;
	data->tx_unack = 0;
	data->rx_ringx = 0;
	data->rx_acked = 0;
	data->blockFlag = 0;
	data->writeLock = 0;
	data->readLock = 0;
	data->nmulti = 0;
	data->suspended = 0;
	data->is_mii = false;
	data->phy_reset_media = NULL;
	data->debug = DEFAULT_DEBUG_FLAGS;
	data->linkStatus = 0;
	if (!init_bufs(data)) {
		undo(data);
		return (B_ERROR);
	}
	
	data->rx_sem = create_sem(0, "tulip read");
	if (data->rx_sem < B_NO_ERROR) {
		undo(data);
		if (data->debug > 0) dprintf("No more sems!\n");
		return (B_ERROR);
	}
	data->tx_sem = create_sem(TX_RING_SIZE, "tulip write");
	if (data->tx_sem < B_NO_ERROR) {
		undo(data);
		if (data->debug > 0) dprintf("No more sems!\n");
		return (B_ERROR);
	}
	data->snooze_sem = create_sem(0, "tulip snooze");
	if (data->snooze_sem < B_NO_ERROR) {
		undo(data);
		if (data->debug > 0) dprintf("No more sems!\n");
		return (B_ERROR);
	}
	init_rings(data);

	data->irq = data->pciInfo->u.h0.interrupt_line;
	install_io_interrupt_handler(data->irq, tulip_interrupt, data, 0);

#if 0
	set_sem_owner(data->rx_sem, B_SYSTEM_TEAM);
	set_sem_owner(data->tx_sem, B_SYSTEM_TEAM);
#endif
	status = hw_init(data);
	if (status < B_NO_ERROR) {
		remove_io_interrupt_handler(data->irq, tulip_interrupt, data);
		undo(data);
		dprintf("hw_init failed\n");
		return (status);
	}
	
	dprintf("%s found: irq %d ioaddr %08x ethernet address ", 
			chip_stuff[data->dev_table_index].name,
			data->irq, data->ioaddr
			// , media_type_str(data->media_xxx) //check this
			);
	print_address(&data->myaddr);

	return (B_NO_ERROR);
}

static status_t tulip_open(const char *name, uint32 flags, void **cookie) {

	int32				devID;
	status_t			err;
	char 				*devName;
	area_id 			area;
	tulip_private_t 	*data;
	char 				* end_name;
	int32				previous_state;
	
	/* find devID slot number */
	if (strncmp(name, kDevDir, sizeof(kDevDir)-1) != 0)
		return EINVAL;
	
	if (name[sizeof(kDevDir)-1] == '\0')
		return EINVAL;
		
	devID = strtol( name + (sizeof(kDevDir)-1), &end_name, 10);
	
	if ((end_name == NULL) || (end_name[0] != '\0'))  /* no dev number */
		return EINVAL;
	
	if (devID >= MAX_CARDS)
		return EINVAL;

	
	/* Check if the device is busy and set in-use flag if not */
	previous_state = atomic_or(&gDevState[devID], DevOpen);
	if (previous_state & DevOpen) {
		return B_BUSY;
	}
	if ((previous_state & DevHardwarePresent) == 0 ) {
		err = EINVAL;
		goto err1;
	}

	if (gDevList[devID] == NULL) {
		dprintf("PCI Missing! This should never happen\n");
		err = EINVAL;
		goto err1;
	}

	/*	Allocate storage for the cookie*/
	data = (tulip_private_t *)aligned_malloc(sizeof(tulip_private_t), &area);
	if (!data) {
		err=B_NO_MEMORY;
		goto err1;
	}

	data->area = area;
	data->ioarea = -1;
	data->init = FALSE;
	*cookie = (void *)data;
	
	/* Setup the cookie */
	data->pciInfo = gDevList[devID];
	data->devID = devID;

	err = init_card(data);
	if (err) goto err2;
	
	gDevTxSem[devID] = data->tx_sem;
	gDevRxSem[devID] = data->rx_sem;
	
#if ETHER_SERIAL_DEBUG
	gdev = data;
	add_debugger_command (kDevName, tulip, "Ethernet driver Info");
#endif /* ETHER_SERIAL_DEBUG */
		
	return B_NO_ERROR;
err2:
	free(data);
err1:
	previous_state = atomic_and(&gDevState[devID], ~DevOpen);
	if ((previous_state & DevHardwarePresent) == 0) { 	/* hardware was ejected during open ?*/
		free(gDevList[devID]);
		gDevList[devID] = NULL;
	}
	dprintf( "tulip: open failed %s!\n", strerror(err) );
	return err;
}



static status_t tulip_close(void *_data) {
	tulip_private_t *data = (tulip_private_t *) _data;		
	/* Release resources we are holding */

#if ETHER_SERIAL_DEBUG
	remove_debugger_command (kDevName, tulip);
//dprintf("DEBUGGER COMMAND REMOVED\n");
#endif /* ETHER_SERIAL_DEBUG */
	
	delete_sem(data->tx_sem);
	delete_sem(data->rx_sem);
			
	return (B_NO_ERROR);
}


static status_t tulip_free(void * cookie) {
	tulip_private_t *data = (tulip_private_t *) cookie;
	int32 devID;
	int32 previous_state;


    /* Remove Interrupt Handler */
	remove_io_interrupt_handler(data->pciInfo->u.h0.interrupt_line, tulip_interrupt, cookie);

	undo(data);

	/* Squirrel this away since we're about to delete "data" */
	devID = data->devID;

	delete_area(data->area);

	/* Device is now available again */
	previous_state = atomic_and(&gDevState[devID], ~DevOpen);
	if ((previous_state & DevHardwarePresent) == 0) {
		free(gDevList[devID]);
		gDevList[devID] = NULL;
	}

	return 0;
}


static int
domulti(
		tulip_private_t *data,
		char *addr
		)
{
	int i;
	int nmulti = data->nmulti;

	if (nmulti == MAX_MULTI) {
		dprintf("tulip: too many multicast addresses to filter perfectly, receive all\n");
		tulip_outw(data, CSR6, tulip_inw(data, CSR6) | 0x80);
		return (B_NO_ERROR);
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
	do_setup_frame(data);
	return (B_NO_ERROR);
}

static void setpromisc(tulip_private_t *data, uint32 on) {
	
	uint32 saved_csr6 = tulip_inw(data, CSR6);

	if ( on ) {
		dprintf("Enabling promiscuous rx");
		data->promisc_mode = true;		
		tulip_outw(data, CSR6, CSR6_promisc | saved_csr6);
	} else {
		dprintf("Disabling promiscuous rx");
		data->promisc_mode = false;		
		tulip_outw(data, CSR6, ~CSR6_promisc & saved_csr6);
	}

	dprintf(" csr6=%x\n", tulip_inw(data, CSR6));
}


/*
 * Standard driver control function
 */
static status_t
tulip_control(void *cookie, uint32 msg, void *buf, size_t len)
{
	struct {
		char bus;
		char device;
		char function;
	} params;
	tulip_private_t *data;

	data = (tulip_private_t *) cookie;
	switch (msg) {
	case ETHER_INIT:
		return (B_NO_ERROR);
	case ETHER_GETADDR:
		memcpy(buf, &data->myaddr, sizeof(data->myaddr));
		return (B_NO_ERROR);
	case ETHER_NONBLOCK:	
		if( *((int32 *)buf) )
			data->blockFlag = B_TIMEOUT;
		else
			data->blockFlag = 0;
		return B_OK;
		
	case ETHER_ADDMULTI:
		return (domulti(data, (char *)buf));

	case ETHER_SETPROMISC:
		setpromisc(data, *(uint32*) buf);
		return B_NO_ERROR;

	case B_GET_WRITE_STATUS:
		*((int32 *) buf) = ((data->tx_ringx - data->tx_unack) > TX_RING_SIZE) ? 0:1;
		return B_NO_ERROR;
	}
	return (B_ERROR);
}


/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "etherpci arg...",
   for example "etherpci R" to enable a received packet trace.
*/
#if ETHER_SERIAL_DEBUG
static int
tulip(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: tulip { Function_calls | NumberSequence | PCI_IO | Stats | Rx_trace | Tx_trace | Warnings}\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'F':
		case 'f':
			gdev->debug ^= FUNCTION;
			if (gdev->debug & FUNCTION) 
				kprintf("Function() call trace Enabled\n");
			else 			
				kprintf("Function() call trace Disabled\n");
			break; 
		case 'N':
		case 'n':
			gdev->debug ^= SEQ;
			if (gdev->debug & SEQ) 
				kprintf("display received TCP sequence Numbers ON\n");
			else 			
				kprintf("display received TCP sequence Numbers OFF\n");
			break; 
		case 'R':
		case 'r':
			gdev->debug ^= RX;
			if (gdev->debug & RX) 
				kprintf("Receive packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break;
		case 'T':
		case 't':
			gdev->debug ^= TX;
			if (gdev->debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'S':
		case 's':
			kprintf(kDevName " statistics\n");			
//			kprintf("rx_ints %d,  tx_ints %d\n", gdev->rints, gdev->wints);
//			kprintf("resets %d \n", gdev->resets);
//			kprintf("crc_errs %d, frame_errs %d, frames_lost %d\n", gdev->crc_errs, gdev->frame_errs, gdev->frames_lost);
			break; 
		case 'P':
		case 'p':
			gdev->debug ^= PCI_IO;
			if (gdev->debug & PCI_IO) 
				kprintf("PCI IO trace Enabled\n");
			else 			
				kprintf("PCI IO trace Disabled\n");
			break; 
		case 'W':
		case 'w':
			gdev->debug ^= WARN;
			if (gdev->debug & WARN) 
				kprintf("Warning debug output ON\n");
			else 			
				kprintf("Warning debug output OFF\n");
			break; 
		default:
			kprintf("%s",usage);
			return 0;
		}
	}
	
	return 0;
}
#endif /* ETHER_SERIAL_DEBUG */


int32	api_version = B_CUR_DRIVER_API_VERSION;
