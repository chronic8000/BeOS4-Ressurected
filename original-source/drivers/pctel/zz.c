#include	<Drivers.h>
#include	<ISA.h>
#include	<PCI.h>
#include	<KernelExport.h>
#include	<driver_settings.h>
#include	<termios.h>
#include	<tty/ttylayer.h>
#include	"zz.h"
#include    "be_defs.h"
#include    "pctel.h"
#include    "halwraps.h"
#include    "vuart.h"

#define	NZZ	8
#define	SOFT_MODEM 1

pci_module_info		*pci;
spinlock			gttylock = 0;
struct tty			*gtty;

#define UART_DLM	1
#define UART_SCR	7
#define UART_LCR_DLAB	0x80

// start pctel globals
extern unsigned long GlobalTimer;

/* get_uart is array [10] of function pointers to functions taking void 
   and returning unsigned char */
static unsigned char (*get_uart[10])(void) =
{
  get_uart_rx,
  get_uart_ier,
  get_uart_iir,
  get_uart_lcr,
  get_uart_mcr,
  get_uart_lsr,
  get_uart_msr,
  get_uart_scr,
  get_uart_dll,
  get_uart_dlm
};

/* put_uart is array [10] of function pointers to functions taking unsigned
   char and returning void */
static void (*put_uart[10])(unsigned char) = 
{
  put_uart_tx,
  put_uart_ier,
  put_uart_iir,
  put_uart_lcr,
  put_uart_mcr,
  put_uart_lsr,
  put_uart_msr,
  put_uart_scr,
  put_uart_dll,
  put_uart_dlm
};

static int country_code = 0;

// end pctel globals

// start pctel locals
static int pctel_autodetect_irq;
static unsigned long pctel_autodetect_port;
// end pctel locals

static int32 pctel_interrupt(void* data);

/* register offsets for the 16550A UART
 */
#define	RBR	0
#define	THR	0
#define	DLL	0
#define	DLH	1
#define	IER	1
#define	ISR	1
#define	FCR	2
#define	IIR	2
#define	LCR	3
#define	MCR	4
#define	LSR	5
#define	MSR	6

/* register bits
 */
#define	LSR_DR			(1 << 0)
#define	LSR_THRE		(1 << 5)
#define	LSR_TSRE		(1 << 6)
#define LCR_7BIT		0x02
#define LCR_8BIT		0x03
#define LCR_2STOP		(1 << 2)
#define LCR_PARITY_ENABLE	(1 << 3)
#define LCR_PARITY_EVEN		(1 << 4)
#define LCR_PARITY_MARK		(1 << 5)
#define LCR_BREAK		(1 << 6)
#define	LCR_DLAB		(1 << 7)
#define	IER_RDR			(1 << 0)
#define	IER_THRE		(1 << 1)
#define	IER_LS			(1 << 2)
#define	IER_MS			(1 << 3)
#define MCR_DTR			(1 << 0)
#define MCR_RTS			(1 << 1)
#define MCR_IRQ_ENABLE		(1 << 3)
#define	MSR_DCTS		(1 << 0)
#define	MSR_DDCD		(1 << 3)
#define	MSR_CTS			(1 << 4)
#define	MSR_DSR			(1 << 5)
#define	MSR_RI			(1 << 6)
#define	MSR_DCD			(1 << 7)
#define	FCR_ENABLE		(1 << 0)
#define	FCR_RX_RESET		(1 << 1)
#define	FCR_TX_RESET		(1 << 2)
#define	FCR_RX_TRIGGER_8	0x80

/* interrupt identification
 */
#define have_int( iir)		(not ((iir)&0x01))
#define THRE_int( iir)		(((iir)&0x0F) == 0x02)
#define RBR_int( iir)		(((iir)&0x0F) == 0x04)
#define	timeout_int( iir)	(((iir)&0x0F) == 0x0C)
#define LSR_int( iir)		(((iir)&0x0F) == 0x06)
#define MSR_int( iir)		(((iir)&0x0F) == 0x00)


#define ttoz( p, t)	(& (p) [((uint)(t)-(uint)(p))/sizeof( *(p))])

struct zz {
	struct tty	tty;
	struct ddrover	rover;
	uint base, intr;
};


static char *devnames[1+NZZ+1] = {
	"config/pctel",
	"ports/pctel",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};
static struct device devices[NZZ] = { 0, 0, 0, 0};

static isa_module_info	*isa;
static tty_module_info	*tm;
static struct zz	ztab[NZZ];
static struct ddomain	zdd;

static bool	zz_service( struct tty *, struct ddrover *, uint);
static status_t	readconf( off_t, uchar *, size_t *);
static status_t	writeconf( off_t, const uchar *, size_t *);
static void	setmodes( struct zz *);
static bool	debugging( );
void		*malloc( );
static int32 zz_interrupt( void *p);

/* ----------
	device_probe - find a pci device
----- */
static bool
device_probe (pci_info *p, char *bus, char *device, char *function, bool report)
{
	int i;
	int dsp_io_address, uart_io_address;
	int PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId;

	for (i = 0; (*pci->get_nth_pci_info) (i, p) == B_NO_ERROR; i++) 
		if (((p->vendor_id == PCTEL_VENDOR_ID) && ( (p->device_id ==PCTEL_DEVICE_ID)
		                                            ||(p->device_id==PCTEL_DEVICE_ID+1) ))
		    || ((p->vendor_id == CMEDIA_VENDOR_ID) && (p->device_id == CMEDIA_DEVICE_ID)))
		{
			*bus = p->bus;
			*device = p->device;
			*function = p->function;
			dsp_io_address = (*pci->read_pci_config)(*bus, *device, *function, 0x18, 2 ) - 1;

			if (report)
			{
				dprintf("************* PCTel Soft Modem Driver **************\n");
				dprintf("***** by Eric Bentley (Eric_Bentley@iname.com) *****\n");
					
			    PciDeviceId = p->device_id;
				PciVendorId = p->vendor_id;
				PciSDeviceId = p->u.h0.subsystem_id;
				PciSVendorId = p->u.h0.subsystem_vendor_id;
				dprintf(DRIVER_NAME" found device\n                     device details:\n");
			    dprintf(DRIVER_NAME"  Vid:0x%04x  Did:0x%04x\n", PciVendorId, PciDeviceId);
				dprintf(DRIVER_NAME" sVid:0x%04x sDid:0x%04x\n", PciSVendorId, PciSDeviceId);
				dprintf(DRIVER_NAME" bus:0x%02x device:0x%02x function:0x%02x\n",*bus, *device, *function);
				dprintf(DRIVER_NAME" DSP interrupt:0x%0x pin:0x%x\n", p->u.h0.interrupt_line, p->u.h0.interrupt_pin);
				dprintf(DRIVER_NAME" DSP i/o address:0x%04x\n",p->u.h0.base_registers[0]);
			}
			return TRUE;

		} // for
		
	return FALSE;
}


/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	bool		f;
	pci_info	p;
	char bus, device, function;
//	dprintf (DRIVER_NAME" - Init Hardware\n");
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **) &pci))
		return B_ERROR;

	bus = ANY_BUS;
	device =0;
	function = 0;
	f = device_probe (&p, &bus, &device , &function, TRUE);
	put_module(B_PCI_MODULE_NAME);
	
	if (f)
	{
		load_driver_symbols("pctel");
	}
	
	return (f ? B_NO_ERROR : B_ERROR);
}

static status_t
zz_open( const char *n, uint32 f, void **d)
{
	struct ttyfile	*tf;
	struct zz	*zp;
	struct ddrover	*r;
	int		i,
			s;
	status_t	err;
	 
	
//	dprintf(DRIVER_NAME" -  device open() %s\n", n );

	i = 0;
	until (streq( n, devnames[i]))
		unless (devnames[++i])
			return (B_ERROR);
	unless (i) {
		*d = 0;
		return (B_OK);
	}
	--i;

    /////////////////////////////////////////////////////////////////////////
//	dprintf("Hooking IRQ %d\n", pctel_autodetect_irq);
	err = install_io_interrupt_handler(pctel_autodetect_irq, (interrupt_handler)pctel_interrupt, 0, 0);
	if (err)
		dprintf("Problems Hooking IRQ %d\n", pctel_autodetect_irq);
	
	PctelInitVUartVars();
	PctelInitCtrlVars(country_code);
	dprintf(DRIVER_NAME" - calling HAL Init\n");
	HAL_Init(pctel_autodetect_port);
	/////////////////////////////////////////////////////////////////////////
	
	s = ENOMEM;
	zp = ztab + i;
	if (tf = malloc( sizeof *tf)) {
		*d = tf;
		tf->tty = &zp->tty;
		tf->flags = f;
		if (r = (*tm->ddrstart)( 0)) {
			(*tm->ddacquire)( r, &zdd);
			if (debugging( i))
				s = B_BUSY;
			else
			{
				s = (*tm->ttyopen)( tf, r, zz_service);
			}
			(*tm->ddrdone)( r);
		}
		unless (s == B_OK)
			free( tf);
	}

	return (s);
}

static status_t
zz_close(void *d)
{
	struct ddrover	*r;
	status_t	s;
	
//	dprintf (DRIVER_NAME" - device close()\n");
	
	unless (d)
		return (B_OK);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttyclose)( d, r);
	(*tm->ddrdone)( r);

	return (s);
}

static status_t
zz_free(void *d)
{
	struct ddrover	*r;
	status_t	s;
	
   HAL_Deinit(pctel_autodetect_port);
//   dprintf("removing IRQ handler for %d\n",pctel_autodetect_irq);
   if (remove_io_interrupt_handler(pctel_autodetect_irq, pctel_interrupt, 0)
																!= B_NO_ERROR)
		dprintf(DRIVER_NAME" - Error removing interrupt handler\n");
	 
	
//	dprintf (DRIVER_NAME" - device free()\n");
	unless (d)
		return (B_OK);
	if (r = (*tm->ddrstart)( 0)) {
		s = (*tm->ttyfree)( d, r);
		(*tm->ddrdone)( r);
	}
	else
		s = ENOMEM;
	free( d);
	
	return (s);
}

static status_t
zz_read( void *d, off_t o, void *buf, size_t *count)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	if (d)
		s = (*tm->ttyread)( d, r, buf, count);
	else {
		(*tm->ddacquire)( r, &zdd);
		s = readconf( o, buf, count);
	}
	(*tm->ddrdone)( r);
	return (s);
}

static status_t
zz_write( void *d, off_t o, const void *buf, size_t *count)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	if (d)
		s = (*tm->ttywrite)( d, r, buf, count);
	else {
		(*tm->ddacquire)( r, &zdd);
		s = writeconf( o, buf, count);
	}
	(*tm->ddrdone)( r);
	return (s);
}

static status_t
zz_control( void *d, uint32 com, void *buf, size_t len)
{
	struct ddrover	*r;
	status_t	s;

	unless (d)
		return (ENODEV);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttycontrol)( d, r, com, buf, len);
	(*tm->ddrdone)( r);
	return (s);
}

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
extern UCHAR READ_UART_REG (ULONG Offset);
extern void WRITE_UART_REG (ULONG Offset, UCHAR data);

static void
put_register_8 (uint offset, uint value)
{
	if ((COM_Vlcr & UART_LCR_DLAB) && (offset <= UART_DLM))
		offset += (UART_SCR+1);
	offset-=pctel_autodetect_port;
//	dprintf(DRIVER_NAME" - wrote :0x%04x to offset : 0x%04x\n",value, offset);
	(*put_uart[offset])(value);

}

static uint
get_register_8 (uint offset )
{
	uint rc;

	if ((COM_Vlcr & UART_LCR_DLAB) && (offset <= UART_DLM))
		offset += (UART_SCR+1);
	offset-=pctel_autodetect_port;
	rc = (*get_uart[offset])();

	return(rc);
}

static int32
zz_interrupt(void *p)
{
	uint	iir, lsr,
		i;

	struct tty *tp = p;
	struct zz *zp = ttoz( ztab, tp);
	uint h = B_UNHANDLED_INTERRUPT;
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);

		h = B_HANDLED_INTERRUPT;
		lsr = get_register_8(zp->base+LSR);

		if (lsr & LSR_DR)
		{
			for (i=0; get_register_8( zp->base+LSR) & LSR_DR; ++i)
			{
				char t;
				t = get_register_8( zp->base+RBR);
//				dprintf("Here is the char: 0x%02x - %c\n", t, t);
				(*tm->ttyin)( tp, &zp->rover, t);
			}
		}
		{
			uint msr = get_register_8( zp->base+MSR);
			if (msr & MSR_DDCD)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWDCD, msr&MSR_DCD);
			if (msr & MSR_DCTS)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWCTS, msr&MSR_CTS);
		}
		if (lsr & LSR_THRE)
			for (i=0; i<16; ++i)
            {
				int c = (*tm->ttyout)( tp, &zp->rover);
				if (c < 0) {
					break;
				}
				if (c>=0)
					put_register_8( zp->base+THR, c);
			}
			
	(*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	return (B_HANDLED_INTERRUPT);
}


static bool
zz_service( struct tty *tp, struct ddrover *r, uint com)
{
	uint msr;
	cpu_status	ps;
	struct zz *zp = ttoz( ztab, tp);

	switch (com) {
	case TTYENABLE:
		unless ((zp->base = devices[zp-ztab].base)
		and (zp->intr = devices[zp-ztab].intr))
			return (FALSE);
		setmodes( zp);

		ps = disable_interrupts();
		acquire_spinlock(&gttylock);
		gtty = tp;
		release_spinlock(&gttylock);
		restore_interrupts(ps);

		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, get_register_8( zp->base+MSR)&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, get_register_8( zp->base+MSR)&MSR_CTS);
		put_register_8( zp->base+MCR, MCR_DTR|MCR_RTS|MCR_IRQ_ENABLE);
		put_register_8( zp->base+IER, IER_MS|IER_LS|IER_RDR);
		break;
	case TTYDISABLE:
		put_register_8( zp->base+IER, 0);
		put_register_8( zp->base+MCR, 0);
		ps = disable_interrupts();
		acquire_spinlock(&gttylock);
		gtty = 0;
		release_spinlock(&gttylock);
		restore_interrupts(ps);
		break;
	case TTYSETMODES:
		setmodes( zp);
		break;
	case TTYISTOP:
		put_register_8( zp->base+MCR, get_register_8( zp->base+MCR)&~MCR_RTS);
		break;
	case TTYIRESUME:
		put_register_8( zp->base+MCR, get_register_8( zp->base+MCR)|MCR_RTS);
		break;
	case TTYOSTART:
		put_register_8( zp->base+IER, IER_MS|IER_LS|IER_THRE|IER_RDR);
		break;
	case TTYOSYNC:
		return ((get_register_8( zp->base+LSR)&LSR_THRE+LSR_TSRE) == LSR_THRE+LSR_TSRE);
	case TTYSETBREAK:
		put_register_8( zp->base+LCR, get_register_8( zp->base+LCR)|LCR_BREAK);
		break;
	case TTYCLRBREAK:
		put_register_8( zp->base+LCR, get_register_8( zp->base+LCR)&~LCR_BREAK);
		break;
	case TTYSETDTR:
		put_register_8( zp->base+MCR, get_register_8( zp->base+MCR)|MCR_DTR);
		break;
	case TTYCLRDTR:
		put_register_8( zp->base+MCR, get_register_8( zp->base+MCR)&~MCR_DTR);
		break;
	case TTYGETSIGNALS:
		msr = (*isa->read_io_8)( zp->base+MSR);
		/* Checking Data Carrier Detect on the PCTEL softmodem will cause
		   +++ATH (line hang up) shortly after ppp traffic flows */ 
		/* 	(*tm->ttyhwsignal)( tp, r, TTYHWDCD, msr&MSR_DCD); */
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, msr&MSR_CTS);
		(*tm->ttyhwsignal)( tp, r, TTYHWDSR, msr&MSR_DSR);
		(*tm->ttyhwsignal)( tp, r, TTYHWRI, msr&MSR_RI);
		break;
	}
	return (TRUE);
}


/*
 * The 8 MHz clock is used on serial[34] of the BeBox:
 *	divisor = 8000000 / 16 / baud
 *
 * 24 MHz is used for other BeBox ports, and all PC ports:
 *	divisor = 24000000 / 13 / 16 / baud
 *
 * Speeds off by 7% are not expected to work.
 */
static void
setmodes( struct zz *zp)
{
	static ushort divisors[][2] = {
		/* 24,	8 Mhz */
		0,	0,		/* 0 baud */
		2304,	10000,		/* 50 baud */
		1536,	6667,		/* 75 baud */
		1047,	4545,		/* 110 baud */
		857,	3717,		/* 134 baud */
		768,	3333,		/* 150 baud */
		512,	2500,		/* 200 baud */
		384,	1667,		/* 300 baud */
		192,	833,		/* 600 baud */
		96,	417,		/* 1200 baud */
		64,	277,		/* 1800 baud */
		48,	208,		/* 2400 baud */
		24,	104,		/* 4800 baud */
		12,	52,		/* 9600 baud */
		6,	26,		/* 19200 baud */
		3,	13,		/* 38400 baud */
		2,	9,		/* 57600 baud (3.7% error with 8 MHz */
		1,	4,		/* 115200 baud (7.8% error with 8 MHz) */
		0,	2,		/* 230400 baud (7.8% error with 8 MHz) */
		4,	16		/* 31250 baud (8.3% error with 24 MHz) */
	};

	struct tty *tp = &zp->tty;
	uint speed = tp->t.c_cflag & CBAUD;
	uint clock = 0;
	if ((zp->base==0x380 || zp->base==0x388)
	and (platform( ) == B_BEBOX_PLATFORM))
		clock = 1;
	if (speed < nel( divisors)) {
		uint divisor = divisors[speed][clock];
		uint lcr = LCR_7BIT;
		if (tp->t.c_cflag & CS8)
			lcr = LCR_8BIT;
		if (tp->t.c_cflag & CSTOPB)
			lcr |= LCR_2STOP;
		if (tp->t.c_cflag & PARENB)
			lcr |= LCR_PARITY_ENABLE;
		unless (tp->t.c_cflag & PARODD)
			lcr |= LCR_PARITY_EVEN;
		if (speed == B0)
			put_register_8( zp->base+MCR, get_register_8( zp->base+MCR)&~MCR_DTR);
		else {
			if (speed <= B1200)
				put_register_8( zp->base+FCR, FCR_ENABLE|FCR_RX_RESET|FCR_TX_RESET);
			else
				put_register_8( zp->base+FCR, FCR_ENABLE|FCR_RX_RESET|FCR_TX_RESET|FCR_RX_TRIGGER_8);
			put_register_8( zp->base+LCR, LCR_DLAB);
			put_register_8( zp->base+DLL, divisor & 0x00ff);
			put_register_8( zp->base+DLH, divisor >> 8);
		}
		put_register_8( zp->base+LCR, lcr);
	}
}

static bool
debugging( uint i)
{

	if (platform( ) == B_BEBOX_PLATFORM) {
		if (devices[i].base == 0x388)
			return (debug_output_enabled( ));
	}
	else {
		static uint32 base = -1UL;
		if (base == -1UL) {
			void *handle;
			handle = load_driver_settings("kernel");
			base = strtoul(get_driver_parameter(handle, "serial_debug_port",
					"0x3f8", "0x3f8"), NULL, 0);
			unload_driver_settings(handle);
		}
		if (devices[i].base == base)
			return (debug_output_enabled( ));
	}
	return (FALSE);
}

static status_t
readconf( off_t o, uchar *buf, size_t *count)
{

	uint n = 0;
	while ((n < *count)
	and (o+n < sizeof( devices))) {
		buf[n] = ((uchar *)devices)[(uint)o+n];
		++n;
	}
	*count = n;
	return (B_OK);
}

static status_t
writeconf( off_t o, const uchar *buf, size_t *count)
{

	uint n = 0;
	while ((n < *count)
	and (o+n < sizeof( devices))) {
		((uchar *)devices)[(uint)o+n] = buf[n];
		++n;
	}
	*count = n;
	return (B_OK);
}

const char	**
publish_devices( )
{
	static char	*list[1+NZZ+1];
	uint		i;
	uint j = 0;

//	dprintf (DRIVER_NAME" - Publish Devices\n");
//	dprintf(DRIVER_NAME" - devnames[0] = %s\n", devnames[0]);
//	dprintf(DRIVER_NAME" - devnames[1] = %s\n", devnames[1]);

	list[j++] = devnames[0];
	for (i=0; i<NZZ; ++i)
		if ((devices[i].base)
		and (i < nel( devnames)))
				list[j++] = devnames[i+1];
				
	list[j] = 0;
	
//	dprintf(DRIVER_NAME" - publishing %s\n", list[0]);
//	dprintf(DRIVER_NAME" - publishing %s\n", list[1]);

	return ((const char **)list);
}

device_hooks	*
find_device( const char * dev)
{
	static device_hooks dh = {
		zz_open, zz_close, zz_free, zz_control, zz_read, zz_write
	};

	dev = dev;
	return (&dh);
}

static const char	DEVCONF[]	= "be_pctel_conf";

static int32 pctel_interrupt(void* data)
{
  	static union i387_union i387;
  	int32 handled;
  	cpu_status	ps;
	
 	 if (HAL_DoInterrupt()){
    	handled = B_HANDLED_INTERRUPT;
 	}
  	else{
		handled = B_UNHANDLED_INTERRUPT;
	}

  	/* make sure to update GlobalTimer */
  	GlobalTimer = HAL_GetTimer();
	
	/* Russ- Not needed on Be. This comes from Linux port.
  	__asm__ __volatile__("fnsave %0\n\t"
					   "fwait"
					   : "=m" (i387));
	*/

  	dspMain();
 	modem_main();

    /*
  	__asm__ __volatile__("frstor %0": :"m" (i387));
    */

	ps = disable_interrupts();
	acquire_spinlock(&gttylock);

	if (gtty) //gtty is a global set in zz_control.  It is used to pass the correct data to the ISR
	{
		zz_interrupt(gtty);
	}

	release_spinlock(&gttylock);
	restore_interrupts(ps);

	return(handled);
}

status_t
init_driver(void)
{
	bool		f;
	pci_info	ppci;
	char		bus, device, function;
	int			PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId;
	port_id		p;
	status_t	s;
	uint		i;
	int32		_;
	status_t	err;
	status_t	status;

//	dprintf(DRIVER_NAME" - Init Driver()\n");

	err = get_module(B_PCI_MODULE_NAME, (module_info **)&pci);
	if (err)
		goto error1;
	err = get_module( B_ISA_MODULE_NAME, (module_info **)&isa);
	if (err)
		goto error2;
	err = get_module( B_TTY_MODULE_NAME, (module_info **)&tm);
	if (err)
		goto error3;

	bus = ANY_BUS;
	device =0;
	function = 0;
	f = device_probe (&ppci, &bus, &device , &function, FALSE);
	if (!f) {
		err = B_ERROR;
		goto error4;
	}

	pctel_autodetect_irq = ppci.u.h0.interrupt_line;
	pctel_autodetect_port = ppci.u.h0.base_registers[0];
	PciDeviceId = ppci.device_id;
	PciVendorId = ppci.vendor_id;
	PciSDeviceId = ppci.u.h0.subsystem_id;
	PciSVendorId = ppci.u.h0.subsystem_vendor_id;

	p = find_port( DEVCONF);
	if (p < 0) {
		p = create_port( 1, DEVCONF);
		if (p < B_OK) {
			err = B_ERROR;
			dprintf(DRIVER_NAME" - error in init driver()\n");
		}
		set_port_owner( p, B_SYSTEM_TEAM);
		{
			static struct device d[] = {
				0, 4, 0, 0
			};
			d[0].base = pctel_autodetect_port;

			memcpy( devices, d, sizeof d);
		}
	}

	if (port_count( p))
		read_port( p, &_, devices, sizeof devices);
	publish_devices( );
	ddbackground( &zdd);
	for (i=0; i<nel( ztab); ++i)
		(*tm->ttyinit)( &ztab[i].tty, FALSE);
	return B_OK;

error4:
//	dprintf(DRIVER_NAME" - error 4 in init driver()\n");
	put_module(B_TTY_MODULE_NAME);
error3:
//	dprintf(DRIVER_NAME" - error 3 in init driver()\n");
	put_module(B_ISA_MODULE_NAME);
error2:
//	dprintf(DRIVER_NAME" - error 2 in init driver()\n");
	put_module(B_PCI_MODULE_NAME);
error1:
//	dprintf(DRIVER_NAME" - error 1 in init driver()\n");
	return err;
}

void
uninit_driver(void)
{
	port_id		p;
	status_t	status;

	p = find_port( DEVCONF);
	unless (p < 0)
		write_port( p, 0, devices, sizeof devices);

	put_module( B_TTY_MODULE_NAME);
	put_module( B_ISA_MODULE_NAME);
	put_module( B_PCI_MODULE_NAME);
}

