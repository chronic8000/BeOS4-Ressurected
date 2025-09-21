#ifndef PARALLEL_H
#define PARALLEL_H


/*
 *  For debugging
 */

#define D(x) ;
#define DD(x) ;
#define DDD(x) ;

#define DEBUG 0
#if DEBUG >= 3
#	undef DDD
#	define DDD(x) (x)
#endif

#if DEBUG >= 2
#	undef DD
#	define DD(x) (x)
#endif

#if DEBUG >= 1
#	undef D
#	define D(x) (x)
#endif

#define E(x)	(x)

#if DEBUG_PORT_LOG
#	define bug pprintf
#else
#	define bug dprintf
#endif


#define M_RDONLY_MASK		0x01
#define M_WRONLY_MASK		0x02
#define M_RDWR_MASK			(M_RDONLY_MASK|M_WRONLY_MASK)
#define M_IRQ_NONE			-1


#define kDeviceBaseName	"parallel/parallel"
#define kDongleBaseName	"misc/dongle/parallel"


#define read_io_8				(*isa->read_io_8)
#define write_io_8				(*isa->write_io_8)
#define make_isa_dma_table		(*isa->make_isa_dma_table)
#define start_isa_dma			(*isa->start_isa_dma)
#define start_scattered_isa_dma	(*isa->start_scattered_isa_dma)
#define lock_isa_dma_channel	(*isa->lock_isa_dma_channel)
#define unlock_isa_dma_channel	(*isa->unlock_isa_dma_channel)
extern isa_module_info *isa;


/* 82091 multi I/O definitions (BeBox) */
enum
{
	MIO_INDEX = 0x398,	/* Index register */
	MIO_DATA = 0x399,	/* Data register */
	PCFG1 = 0x20,		/* Parallel port configuration register */
	PCFG2 = 0x21		/* Parallel port power management register */
};

/* Parallel port definitions */
enum
{
	INIT_DELAY			= 1000,					/* Init delay (us) */
	NUM_PORTS 			= 4,					/* Nb max of ports supported */
	DMA_TRANSFERT_SIZE	= 4*B_PAGE_SIZE,		/* Max xfert buffer for ISA DMA */
	TMP_BUFFER_SIZE		= B_PAGE_SIZE,			/* Temp buffer size */

	PDATA		= 0,			/* Data register */
	ECPAFIFO	= 0,			/* ECP address FIFO */
	PSTAT		= 1,			/* Status register */
	PCON		= 2,			/* Control register */
	SDFIFO		= 0x400,		/* Data FIFO */
	ECPDFIFO	= 0x400,		/* ECP data FIFO */
	ECPCFGA		= 0x400,		/* ECP configuration register A */
	ECPCFGB		= 0x401,		/* ECP configuration register B */
	ECR			= 0x402,		/* Extended control register */

	S_PIRQ	= 0x04,				/* PSTAT bits */
	S_FAULT	= 0x08,
	S_SEL	= 0x10,
	S_PERR	= 0x20,
	S_ACK	= 0x40,
	S_BUSY	= 0x80,

	C_STROBE	= 0x01,			/* PCON bits */
	C_AUTOFD	= 0x02,
	C_INIT		= 0x04,
	C_SELIN		= 0x08,
	C_IRQEN		= 0x10,
	C_DIR		= 0x20,

	EC_FIFOES		= 0x01,		/* ECR bits */
	EC_FIFOFS		= 0x02,
	EC_SERVICEINTR	= 0x04,
	EC_DMAEN		= 0x08,
	EC_ERRINTR		= 0x10,

	EC_ISA_COMPATIBLE	= 0x00,
	EC_PS2				= 0x20,
	EC_ISA_FIFO			= 0x40,
	EC_ECP				= 0x60,
	EC_TEST				= 0xC0,
	EC_CONFIG			= 0xE0
};


/* IRQ handler modes */
typedef enum
{
	IRQMODE_INACTIVE,		/* Do nothing */
	IRQMODE_FIFO_WRITE,		/* Send data via FIFO, release irq_sem when transfer is complete */
	IRQMODE_FIFO_READ,		/* Receive data via FIFO, release irq_sem when transfer is complete */
	IRQMODE_DMA,			/* Release irq_sem when DMA is complete */
	IRQMODE_WRITE_ACK		/* Release irq_sem on ACK */
} parallel_irq_mode;


/* Per-port information (also used as cookie) */
enum
{
	IEEE1284_COMPATIBILITY	= -1,
	IEEE1284_NIBBLE			= 0x00,
	IEEE1284_BYTE			= 0x01,
	IEEE1284_ECP			= 0x10,
	IEEE1284_ECP_RLE		= 0x30,
	IEEE1284_EPP			= 0x40
};


/* Device data */

typedef struct
{
	bool dma_available;
	int dma_channel;
	int32 dma_in_use;
	area_id dma_table_area;
	isa_dma_entry *dma_table;
	isa_dma_entry *dma_table_phys;
} dma_info;

typedef volatile struct
{
	bool exists;			/* Flag: Does port exist? */
	int32 type;				/* Port type (PAR_TYPE_* flags) */

	int io_base;			/* Port I/O base address */
	int irq_num;			/* Port IRQ number */

	int32 open;				/* counts how many times the driver has been opened */
	int32 open_mode;		/* O_xxx flags */

	parallel_mode mode;		/* Operating mode (PAR_MODE_*) */
	int ieee1284_mode;		/* current ieee1284 mode */

	bool ecp_read_mode;		/* Flag: ECP read mode (true) or write mode (false) */
	bigtime_t timeout;		/* ECP read timeout */
	bigtime_t failure;		/* Failure timeout */

	parallel_irq_mode irq_mode;	/* IRQ handler mode (see above) */
	bool error;				/* Flag from IRQ handler: error occurred */
	sem_id irq_sem;			/* Signal from IRQ handler */
	sem_id hw_lock;			/* Parallel port arbitration */

	void *buffer;			/* Pointer to buffer in kernel space (accessible by interrupt) for FIFO transfers */

	uint8 *data;			/* Data pointer, length and "actual" counter for FIFO interrupt modes */
	size_t length;
	size_t actual;
	int fifosize;			/* M.AG. 02/22/1999 */
	int threshold_read;
	int threshold_write;

	dma_info	device_dma;
} par_port_info;


/* user data */
typedef volatile struct
{
	int32			open_mode;
	par_port_info	*d;
} user_info;


typedef struct
{
	dma_info 		parallel_dma;
	par_port_info 	parallel_info[NUM_PORTS];
	const char 		*parallel_name_list[(NUM_PORTS*2)+1];
} device_datas_t;

extern device_datas_t *device_data;

#define dma 		(device_data->parallel_dma)
#define pinfo 		(device_data->parallel_info)
#define name_list	(device_data->parallel_name_list)


/* prototypes */
int pprintf(const char* format, ...);
status_t init_open_device	(par_port_info	*d);
status_t do_dev_open(par_port_info *d, uint32 flags, void **cookie);
status_t dev_free(void *cookie);

#endif



