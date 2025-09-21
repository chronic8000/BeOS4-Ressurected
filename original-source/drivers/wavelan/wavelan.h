
/*
 * I/O Ctls: Belongs in a Public Header File
 */
enum
{
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* Get ethernet address */
	ETHER_INIT,								/* Set irq and port     */
	ETHER_NONBLOCK,							/* Set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* Add multicast addr   */
	ETHER_REMMULTI,							/* Rem multicast addr   */
	ETHER_SETPROMISC,						/* Set promiscuous      */
	ETHER_GETFRAMESIZE						/* Get frame size       */
};

/*
 * Driver Informations
 */
#define DevName           "WaveLAN"
#define DevDir            "net/" DevName "/"
#define DEVNAME_LENGTH    64
#define MAX_CARDS         4

#define TX_BUFFERS        8
#define RX_BUFFERS        16

#define BUFFER_SIZE       2048L              /* Enough to hold a 1536 Frame     */
#define MAX_FRAME_SIZE    1536               /* 1536                            */
#define MAX_MULTI         32                 /* Hardware Related, Do not change */

#define TRANSMIT_TIMEOUT  ((bigtime_t)10000) /* 1/100 Second                    */

#define MIN_PCI_LATENCY   64

/*
 * PCI Communications
 */
//nv #define IO_PORT_PCI_ACCESS true
//#define MEMORY_MAPPED_PCI_ACCESS true

#if IO_PORT_PCI_ACCESS
#define write8(address, value)			(*pModuleInfo->write_io_8)((address), (value))
#define write16(address, value)			(*pModuleInfo->write_io_16)((address), (value))
#define write32(address, value)			(*pModuleInfo->write_io_32)((address), (value))
#define read8(address)					((*pModuleInfo->read_io_8)(address))
#define read16(address)					((*pModuleInfo->read_io_16)(address))
#define read32(address)					((*pModuleInfo->read_io_32)(address))
#elif MEMORY_MAPPED_PCI_ACCESS /*  MEMORY_MAPPED_PCI_ACCESS */
#define read8(address)   				(*((volatile uint8*)(address)));  __eieio()
#define read16(address)  				(*((volatile uint16*)(address))); __eieio()
#define read32(address) 				(*((volatile uint32*)(address))); __eieio()
#define write8(address, data)  			(*((volatile uint8 *)(address))  = data);   __eieio()
#define write16(address, data) 			(*((volatile uint16 *)(address)) = (data)); __eieio()
#define write32(address, data) 			(*((volatile uint32 *)(address)) = (data)); __eieio()
#else //nv
#define read8(address)   				(1)
#define read16(address)  				(2)
#define read32(address) 				(4)
#define write8(address, data)  			
#define write16(address, data) 			
#define write32(address, data) 			
#endif 

/*
 * Registers Offsets
 */
enum WAVELAN_register_offsets
{
	StationAddr      = 0x00,
	RxConfig         = 0x06,
	TxConfig         = 0x07,
	ChipCmd          = 0x08,
	IntrStatus       = 0x0C,
	IntrEnable       = 0x0E,
	MulticastFilter0 = 0x10,
	MulticastFilter1 = 0x14,
	RxRingPtr        = 0x18,
	TxRingPtr        = 0x1C,
	MIIPhyAddr       = 0x6C,
	MIIStatus        = 0x6D,
	PCIBusConfig     = 0x6E,
	MIICmd           = 0x70,
	MIIRegAddr       = 0x71,
	MIIData          = 0x72,
	Config           = 0x78,
	RxMissed         = 0x7C,
	RxCRCErrs        = 0x7E,
};

/*
 * Command Bits
 */
enum WAVELAN_chip_cmd_bits
{
	CmdInit     = 0x0001,
	CmdStart    = 0x0002,
	CmdStop     = 0x0004,
	CmdRxOn     = 0x0008,
	CmdTxOn     = 0x0010,
	CmdTxDemand = 0x0020,
	CmdRxDemand = 0x0040,
	CmdEarlyRx  = 0x0100,
	CmdEarlyTx  = 0x0200,
	CmdFDuplex  = 0x0400,
	CmdNoTxPoll = 0x0800,
	CmdReset    = 0x8000
};

/*
 * Interrupt Status Bits
 */
enum WAVELAN_intr_status_bits
{
	IntrRxDone          = 0x0001,
	IntrRxErr           = 0x0004,
	IntrRxEmpty         = 0x0020,
	IntrTxDone          = 0x0002,
	IntrTxAbort         = 0x0008,
	IntrTxUnderrun      = 0x0010,
	IntrPCIErr          = 0x0040,
	IntrStatsMax        = 0x0080,
	IntrRxEarly         = 0x0100,
	IntrMIIChange       = 0x0200,
	IntrRxOverflow      = 0x0400,
	IntrRxDropped       = 0x0800,
	IntrRxNoBuf         = 0x1000,
	IntrTxAborted       = 0x2000,
	IntrLinkChange      = 0x4000,
	IntrRxWakeUp        = 0x8000,
	IntrNormalSummary   = 0x0003,
	IntrAbnormalSummary = 0xC260,
};

/*
 * RX Status Bits
 */
enum WAVELAN_rx_status_bits
{
	RxOK       = 0x8000,
	RxWholePkt = 0x0300,
	RxErr      = 0x008F
};

/*
 * Desc Status Bits
 */
enum WAVELAN_desc_status_bits
{
	DescOwn       = 0x80000000,
	DescEndPacket = 0x4000,
	DescIntr      = 0x1000
};

/*
 * PCI ID Information
 */
struct WAVELAN_pci_id_info
{
	const char *name;
	uint16      vendor_id,
		        device_id,
				device_id_mask,
				flags;
	int         io_size;
};

/*
 * Chip Compatibility
 */
enum WAVELAN_chip_capability_flags
{
	CanHaveMII = 1
};

/*
 * Chip Information
 */
struct WAVELAN_chip_info
{
	int io_size;
	int flags;
};

/*
 * RX Descriptor
 */
typedef struct
{
	uint32 rx_status;
	uint32 desc_length;

	uint32 addr;
	uint32 next;
} WAVELAN_rx_desc;

/*
 * TX Descriptor
 */
typedef struct
{
	uint32 tx_status;
	uint32 desc_length;

	uint32 addr;
	uint32 next;
} WAVELAN_tx_desc;

typedef struct
{
	unsigned char ebyte[6];
} ethernet_addr;

/*
 * Private Data
 */
typedef struct
{
	/*
	 * Rings
	 */
	WAVELAN_tx_desc  tx_desc[TX_BUFFERS];  /* TX Frame Descriptor                  */
	WAVELAN_rx_desc  rx_desc[RX_BUFFERS];  /* RX Frame Descriptor                  */
	area_id           tx_buf_area;          /* Transmit Descriptor and Buffer Areas */
	area_id           rx_buf_area;          /* Receive Descriptor and Buffer Areas  */
	uchar            *tx_buf;               /* TX Buffer                            */
	uchar            *rx_buf;               /* RX Buffer                            */

	area_id           cookieID;  /* Cookie Area ID           */
	int32             devID;     /* Device ID                */
	uint32            addr;      /* Base Address of HostRegs */
	pci_info         *pciInfo;   /* PCI Informations         */
	ethernet_addr     macaddr;   /* Ethernet Address         */

	uint8   tx_thresh;
	uint8   rx_thresh;
	uint16 	cmd;
	uint8   interrupted;

	unsigned char duplex_full;   /* Full-Duplex Operation Requested. */
	unsigned char duplex_lock;

	uint8  cur_tx;
	uint8  cur_rx;
	int32  inrw;                /* In Read or Write Function          */
	int    nonblocking;

	/*
	 * MII Transceiver Section
	 */
	int           mii_cnt;          /* MII Device Addresses.    */
	uint16        mii_advertising;  /* NWay Media Advertisement */
	unsigned char mii_phys[8];      /* MII Device Addresses.    */
	
} WAVELAN_private;

/*
 * Variables
 */
extern char            *pDevNameList[MAX_CARDS+1];
extern pci_info        *pDevList    [MAX_CARDS+1];
extern pci_module_info *pModuleInfo;
extern device_hooks     pDeviceHooks; 
extern int32            nOpenMask;

extern struct WAVELAN_pci_id_info pci_tbl[];
extern struct WAVELAN_chip_info   cap_tbl[];

/*
 * Functions
 */

// pci.c
int32    pci_getlist (pci_info *info[], int32 maxEntries);
status_t pci_freelist(pci_info *info[]);

// res.c
status_t res_allocate(wavelan_private *device);
void     res_free    (wavelan_private *device);

// misc.c
uint32   mem_virt2phys(void *addr, unsigned int length);
void     rx_setmode   (WAVELAN_private *device);
void     duplex_check (WAVELAN_private *device);
bool     is_mine      (WAVELAN_private *device, char *addr);

// ring.c
status_t ring_init(WAVELAN_private *device);
void     ring_free(WAVELAN_private *device);

// cmd.c
void     cmd_reset     (WAVELAN_private *device);
void     cmd_getmacaddr(WAVELAN_private *device);

/*
 * Macro
 */
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))
#define __WAVELAN_DEBUG__

#ifdef __WAVELAN_DEBUG__
int debug_printf(char *format, ...);
#endif