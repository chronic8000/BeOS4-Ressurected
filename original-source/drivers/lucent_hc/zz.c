#include	<Drivers.h>
#include	<ISA.h>
#include	<PCI.h>
#include	<KernelExport.h>
#include	<driver_settings.h>
#include	<termios.h>
#include	<tty/ttylayer.h>
#include	"zz.h"
#include    "be_defs.h"

#define	NZZ	8
#define	SOFT_MODEM_NO_UART 1

// for soft modem support
// These are global for now as the Lucent code is using globals extensively.  I'll
// have to correct this.

pci_module_info		*pci;
thread_id			tid;
sem_id				loop_sem;
spinlock			gttylock = 0;
struct tty			*gtty;

byte				dp_dsp_isr( void );

extern byte 		Irq;
extern word 		BaseAddress;
extern word 		ComAddress;
extern byte 		VMODEM_Timer_Active;
//extern word myUART_CommReadCount ( void );
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
extern UCHAR READ_UART_REG (ULONG Offset);
extern void WRITE_UART_REG (ULONG Offset, UCHAR data);
extern void turn_off_dsp_ints( void );

// end lucent globals

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
	uint		base,
			intr;
};


static char *devnames[1+NZZ+1] = {
	"config/lucent",
	"ports/lucent",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};
static struct device devices[NZZ] = { 0, 0, 0, 0
};
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

static int32
zz_interrupt( void *p);

/* DSP interrupt service routine */
static int32 DSP_Interrupt ( void * instance_data )
{
	return(dp_dsp_isr()?B_HANDLED_INTERRUPT:B_UNHANDLED_INTERRUPT);
}

void determine_io_addresses(pci_info *p, char bus, char device, char function,
                            int * dsp_address,  int * uart_address)
{
	int i;
//	*dsp_address = (*pci->read_pci_config)(bus, device, function, 0x18, 2 ) - 1;
//	*uart_address = (*pci->read_pci_config)(bus, device, function, 0x14, 2 ) - 1;
	i = 5;
	*dsp_address = 0;
	*uart_address = 0;
	while ( i >= 0)
	{
		if (p->u.h0.base_register_flags[i] & PCI_address_space)
		{
			if (!(*dsp_address))
				*dsp_address = p->u.h0.base_registers[i];
			else
				if(!(*uart_address))
					*uart_address = p->u.h0.base_registers[i];
				else
					dprintf("We should not be able to get here\n");
		}
		i--;
	}
}

/* ----------
	device_probe - find a pci device
----- */
static bool
device_probe (pci_info *p, char *bus, char *device, char *function, bool report)
{
	int i;
	int dsp_io_address, uart_io_address;
	int PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId;
	bool found_device=FALSE;

	for (i = 0; (*pci->get_nth_pci_info) (i, p) == B_NO_ERROR; i++) {
		if (p->vendor_id == LUCENT_VENDOR_ID)
			if ((p->device_id >= LUCENT_DEVICE_ID_LOW) && (p->device_id <= LUCENT_DEVICE_ID_HIGH))
			{
				found_device = TRUE;
				break;
			}
		if (p->vendor_id == XIRCOM_VENDOR_ID)
			if ((p->device_id >= XIRCOM_DEVICE_ID_LOW) && (p->device_id <= XIRCOM_DEVICE_ID_HIGH))
			{
				found_device = TRUE;
				break;
			}
		if (p->vendor_id == XIRCOM_VENDOR_ID)
			if ((p->device_id >= LUCENT_DEVICE_ID_LOW) && (p->device_id <= LUCENT_DEVICE_ID_HIGH))
			{
				found_device = TRUE;
				break;
			}
		}
		
	if (found_device)	
		if (*bus == ANY_BUS || (*bus == p->bus &&
							   *device == p->device &&
							   *function == p->function)) {
			*bus = p->bus;
			*device = p->device;
			*function = p->function;
//			dsp_io_address = (*pci->read_pci_config)(*bus, *device, *function, 0x18, 2 ) - 1;
//			uart_io_address = (*pci->read_pci_config)(*bus, *device, *function, 0x14, 2 ) - 1;
			determine_io_addresses(p, *bus, *device, *function, &dsp_io_address, &uart_io_address);
			if (report)
			{
				dprintf("************* Lucent Soft Modem Driver *************\n");
				dprintf("***** "IDSTRING" *****\n");
#ifdef DEBUGGING
				dprintf("***** compiled on %s at %s *****\n", __DATE__, __TIME__);
				dprintf("***** compiled with Debugging output ******\n");
#endif				
			    PciDeviceId = p->device_id;
				PciVendorId = p->vendor_id;
				PciSDeviceId = p->u.h0.subsystem_id;
				PciSVendorId = p->u.h0.subsystem_vendor_id;
				dprintf(DRIVER_NAME" found device\n                            device details:\n");
			    dprintf(DRIVER_NAME"  Vid:0x%04x  Did:0x%04x\n", PciVendorId, PciDeviceId);
				dprintf(DRIVER_NAME" sVid:0x%04x sDid:0x%04x\n", PciSVendorId, PciSDeviceId);
				dprintf(DRIVER_NAME" bus:0x%02x device:0x%02x function:0x%02x\n",*bus, *device, *function);
				dprintf(DRIVER_NAME" DSP interrupt:0x%0x pin:0x%x\n", p->u.h0.interrupt_line, p->u.h0.interrupt_pin);
				dprintf(DRIVER_NAME" DSP i/o address:0x%04x\n",dsp_io_address);
				dprintf(DRIVER_NAME" UART i/o address:0x%04x\n",uart_io_address);
			}	
		}	
			
	return found_device;
}

/* timer_worker()                                                          */
/* This function is started as a separate thread in init_driver().  It is  */
/* a task time thread so that it can process the DSP_background routines   */
/* which can take several seconds to complete and therefore can not run    */
/* with interrupts diabled.                                                */
int read_count = 0;
int write_count = 0;
static int32 timer_worker(void *p)
{
	cpu_status	ps;
	int64		t;
	status_t	err;
	struct tty			*ltty;
	int loop_count = 0;
	int print_line =0;
	int bytes;
		
	while (1)
	{
		loop_count++;
		err = acquire_sem_etc(loop_sem, 1, B_TIMEOUT, DSP_TIMER_RESOLUTION);
		
		if (err == B_BAD_SEM_ID)
		{
			modem_sleep();

//			io_sleep(); //Turn off the DSP - if you don't you risk not hanging up
			            //the line in certain situations
			return 0;
		}
		
		if ( ( VMODEM_Timer_Active != 0 ) )
		{
			UART_background () ;    // call the Modem background routine
//			status = acquire_sem(uart_sem);
//			vxdUpdateLeads();       // update the virtual UART
//			status = release_sem(uart_sem);
		}
#ifdef foo
		bytes = myUART_CommReadCount();
				
		if ((loop_count > 100) || bytes)
		{
			dprintf("%05d ltmodem - bytes in buffer: %d - IIR: 0x%02x - gtty is %s \n",
			print_line++, bytes, READ_UART_REG(IIR), gtty ? "valid":"NULL");
			if (!bytes)
  				loop_count = 0;
		}
#endif	
		ltty = gtty;

		if (ltty) //gtty is a global set in zz_control.  It is used to pass the correct data to the ISR
		{
   			ps = disable_interrupts();
//			acquire_spinlock(&gttylock);

			zz_interrupt(ltty);

//			release_spinlock(&gttylock);
			restore_interrupts(ps);
		}
	}
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
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **) &pci))
		return B_ERROR;
//	dprintf(DRIVER_NAME" - Init Hardware\nLocating device with vedor id-%04x : device id-%04x\n", LUCENT_VENDOR_ID, LUCENT_DEVICE_ID);
	bus = ANY_BUS;
	device =0;
	function = 0;
	f = device_probe (&p, &bus, &device , &function, FALSE);
	
#define CLIPPER_PARALLEL_HACK	
#ifdef CLIPPER_PARALLEL_HACK 
	{ 
		uint32 c; 
		uint8 b; 
	 
		/* 
		 * enable port 3F0 
		 */ 
		c = pci->read_pci_config(0, 7, 0, 0x85, 1); 
		c |= 2; 
		pci->write_pci_config(0, 7, 0, 0x85, 1, c); 
 
		/* 
		 * disable parallel port 
		 */		 
		pci->write_io_8(0x3F0, 0xe2); 
		b = pci->read_io_8(0x3F1); 
		b |= 3; 
		pci->write_io_8(0x3F1, b); 
 
		 
		/* 
		 * disable 3F0 
		 */ 
		c &= ~2; 
		pci->write_pci_config(0, 7, 0, 0x85, 1, c); 
		 
		 
	} 
#endif 
	put_module(B_PCI_MODULE_NAME);
	
	if (f)
	{
		load_driver_symbols("ltmodem");
	}
	else
		dprintf("lucent hardware not located.\n");
	
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

	x_wakeup();

	i = 0;
	until (streq( n, devnames[i]))
		unless (devnames[++i])
			return (B_ERROR);
	unless (i) {
		*d = 0;
		return (B_OK);
	}
	--i;
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
zz_close( void *d)
{

	struct ddrover	*r;
	status_t	s;

	unless (d)
		return (B_OK);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttyclose)( d, r);
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
zz_free( void *d)
{
	struct ddrover	*r;
	status_t	s;

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

#ifndef SOFT_MODEM_NO_UART
static void
put_register_8 (uint port, uint data)
{
	(*isa->write_io_8)( port, data);
}

static uint
get_register_8 (uint port )
{
	return ((*isa->read_io_8)( port));
}
#else

static void
put_register_8 (uint port, uint data)
{
//	acquire_sem(uart_sem);
	WRITE_UART_REG(port - BaseAddress, data);
//	release_sem(uart_sem);

}

static uint
get_register_8 (uint port )
{
//	acquire_sem(uart_sem);
	return (READ_UART_REG(port - BaseAddress));
//	release_sem(uart_sem);

}

#endif // not SOFT_MODEM_NO_UART

static int32
zz_interrupt( void *p)
{
	uint	iir,
			num_empty_chars_space_in_tx_buff,
			i;

	struct tty *tp = p;
	struct zz *zp = ttoz( ztab, tp);
	uint h = B_UNHANDLED_INTERRUPT;
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);
	loop {
		iir = get_register_8( zp->base+IIR);
#ifdef foo
		//Lucent fix
		if (myUART_CommReadCount())
		{
			iir |= 4; //indicate data
			iir &=~1; //turn off none
		}
		// end Lucent fix
#endif		
//		unless (have_int( iir))
//			break;

		h = B_HANDLED_INTERRUPT;
//		if (THRE_int( iir)){
if (num_empty_chars_space_in_tx_buff=io_get_dte_tx_count()){
			for (i=0; i<num_empty_chars_space_in_tx_buff; ++i) {
				int c = (*tm->ttyout)( tp, &zp->rover);
				if (c < 0) {
					put_register_8( zp->base+IER, IER_MS|IER_LS|IER_RDR);
					break;
				}
				put_register_8( zp->base+THR, c);
			}
		}
		unless (have_int( iir))
			break;

//else - We can process both send and receive
		if ((RBR_int( iir))
		or (timeout_int( iir))) {
			for (i=0; get_register_8( zp->base+LSR) & LSR_DR; ++i)
				(*tm->ttyin)( tp, &zp->rover, get_register_8( zp->base+RBR));
			/*
			 * defend against bogus hardware
			 */
			unless (i)
				get_register_8( zp->base+RBR);
		}
		else if (LSR_int( iir)) {
			uint lsr = get_register_8( zp->base+LSR);
		}
		else if (MSR_int( iir)) {
			uint msr = get_register_8( zp->base+MSR);
			if (msr & MSR_DDCD)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWDCD, msr&MSR_DCD);
			if (msr & MSR_DCTS)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWCTS, msr&MSR_CTS);
		}
	}
	(*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	return (B_HANDLED_INTERRUPT);
}


static bool
zz_service( struct tty *tp, struct ddrover *r, uint com)
{

	cpu_status	ps;
	struct zz *zp = ttoz( ztab, tp);

	switch (com) {
	case TTYENABLE:
		unless ((zp->base = devices[zp-ztab].base)
		and (zp->intr = devices[zp-ztab].intr))
			return (FALSE);
		setmodes( zp);

//		ps = disable_interrupts();
//		acquire_spinlock(&gttylock);
		gtty = tp;
//		release_spinlock(&gttylock);
//		restore_interrupts(ps);

//		install_io_interrupt_handler( zp->intr, zz_interrupt, tp, 0);
		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, get_register_8( zp->base+MSR)&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, get_register_8( zp->base+MSR)&MSR_CTS);
		put_register_8( zp->base+MCR, MCR_DTR|MCR_RTS|MCR_IRQ_ENABLE);
		put_register_8( zp->base+IER, IER_MS|IER_LS|IER_RDR);
		break;
	case TTYDISABLE:
		put_register_8( zp->base+IER, 0);
		put_register_8( zp->base+MCR, 0);
//		ps = disable_interrupts();
//		acquire_spinlock(&gttylock);
		gtty = 0;
//		release_spinlock(&gttylock);
//		restore_interrupts(ps);
//		remove_io_interrupt_handler( zp->intr, zz_interrupt, tp);
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
		(*tm->ttyhwsignal)( tp, r, TTYHWDSR, get_register_8( zp->base+MSR)&MSR_DSR);
		(*tm->ttyhwsignal)( tp, r, TTYHWRI, get_register_8( zp->base+MSR)&MSR_RI);
		
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
	list[j++] = devnames[0];
	for (i=0; i<NZZ; ++i)
		if ((devices[i].base)
		and (i < nel( devnames)))
			if ((platform( ) == B_BEBOX_PLATFORM)
			and (i == 4))
				list[j++] = "ports/com3";
			else if ((platform( ) == B_BEBOX_PLATFORM)
			and (i == 5))
				list[j++] = "ports/com4";
			else
				list[j++] = devnames[i+1];
	list[j] = 0;
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


static const char	DEVCONF[]	= "be_lucent_conf";

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
		dprintf("No Lucent hardware detected\n");
		err = B_ERROR;
		goto error4;
	}

//	BaseAddress = (*pci->read_pci_config)(bus, device, function, 0x18, 2 ) - 1;
//	ComAddress  = (*pci->read_pci_config)(bus, device, function, 0x14, 2 ) - 1;
	determine_io_addresses(&ppci, bus, device, function, (int*)&BaseAddress, (int*)&ComAddress);
										
	Irq = ppci.u.h0.interrupt_line;
	read_eeprom_file();
	PciDeviceId = ppci.device_id;
	PciVendorId = ppci.vendor_id;
	PciSDeviceId = ppci.u.h0.subsystem_id;
	PciSVendorId = ppci.u.h0.subsystem_vendor_id;
	ll_set_pci_id ( PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId ) ;

	loop_sem = create_sem(0, "DSP background");
	if (loop_sem < 0) {
		err = loop_sem;
		goto error4;
	}
	set_sem_owner(loop_sem, B_SYSTEM_TEAM);
	tid = spawn_kernel_thread(timer_worker, "timer",  B_NORMAL_PRIORITY,  0);
	if (tid < 0) {
		err = tid;
		goto error5;
	}
	resume_thread(tid);
//	err = install_io_interrupt_handler(Irq, (interrupt_handler) DSP_Interrupt, &Irq, 0);
	err = install_io_interrupt_handler(Irq, (interrupt_handler) DSP_Interrupt, 0, 0);
	if (err)
		goto error6;

	vxdPortOpen();
	V16550_Init();

	p = find_port( DEVCONF);
	if (p < 0) {
		p = create_port( 1, DEVCONF);
		if (p < B_OK) {
			err = B_ERROR;
			goto error7;
		}
		set_port_owner( p, B_SYSTEM_TEAM);
		{
			static struct device d[] = {
				0, 4, 0, 0
			};
			d[0].base = BaseAddress;
//			dprintf("Setting d[0].base to 0x%x\n", BaseAddress);
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

error7:
	turn_off_dsp_ints();
//	remove_io_interrupt_handler(Irq, (interrupt_handler)DSP_Interrupt, &Irq);
	remove_io_interrupt_handler(Irq, (interrupt_handler)DSP_Interrupt, 0);
error6:
	delete_sem(loop_sem);
	loop_sem = 0;
	wait_for_thread(tid, &status);
error5:
	if (loop_sem)
		delete_sem(loop_sem);
error4:
	put_module(B_TTY_MODULE_NAME);
error3:
	put_module(B_ISA_MODULE_NAME);
error2:
	put_module(B_PCI_MODULE_NAME);
error1:
	dprintf("Leaving init_driver() with error\n");
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

	
	delete_sem(loop_sem);			// this will unblock the thread if it was waiting
	wait_for_thread(tid, &status);	// bad things happen when the driver unloads
									// while a thread is still executing
	turn_off_dsp_ints();
//	if (remove_io_interrupt_handler(Irq, (interrupt_handler)DSP_Interrupt, &Irq) != B_NO_ERROR)
	if (remove_io_interrupt_handler(Irq, (interrupt_handler)DSP_Interrupt, 0) != B_NO_ERROR)
		dprintf(DRIVER_NAME" - Error removing interrupt handler\n");

	put_module( B_TTY_MODULE_NAME);
	put_module( B_ISA_MODULE_NAME);
	put_module( B_PCI_MODULE_NAME);
}

int32	api_version	= B_CUR_DRIVER_API_VERSION;


#ifdef __ZRECOVER
# include <recovery/driver_registry.h>
fixed_driver_info lucent_hc_driver_info=
{
 	"Lucent SoftModem",
    B_CUR_DRIVER_API_VERSION,
    init_hardware,
    publish_devices,
    find_device,
    init_driver,
    uninit_driver
};
REGISTER_STATIC_DRIVER(lucent_hc_driver_info);         
#endif
