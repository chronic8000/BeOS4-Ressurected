

#include	<KernelExport.h>
#include	<ISA.h>
#include	<driver_settings.h>
#include	<nukernel/inc/fsproto.h>
#include	<os_p/interrupts.h>
#include	<driver/serial_module.h>
#include	<tty/ttylayer.h>
#include	"zz.h"


#define	NZZ	8


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
	uint		base;
	uint		intr;
};


static char *devnames[1+NZZ+1] = {
	"config/serial",
	"ports/serial1",
	"ports/serial2",
	"ports/serial3",
	"ports/serial4",
	"ports/serial5",
	"ports/serial6",
	"ports/serial7",
	"ports/serial8",
	0
};
static struct device devices[NZZ] = {
	0x03F8, B_INT_COM1,
	0x02F8, B_INT_COM2,
};
static serial_info	*pcmcia;
static bool		do_not_save[NZZ];
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


static status_t
zz_open( const char *n, uint32 f, void **d)
{
	struct ttyfile	*tf;
	struct zz	*zp;
	struct ddrover	*r;
	int		i,
			s;

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
				s = (*tm->ttyopen)( tf, r, zz_service);
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


static status_t
zz_select( void *d, uint8 event, uint32 ref, selectsync *sync)
{
	struct ddrover	*r;
	status_t	s;

	unless (d)
		return (ENODEV);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttyselect)( d, r, event, ref, sync);
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
zz_deselect( void *d, uint8 event, selectsync *sync)
{
	struct ddrover	*r;
	status_t	s;

	unless (d)
		return (ENODEV);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttydeselect)( d, r, event, sync);
	(*tm->ddrdone)( r);
	return (s);
}


static int32
zz_interrupt( void *p)
{
	uint	iir,
		i;

	struct tty *tp = p;
	struct zz *zp = ttoz( ztab, tp);
	uint h = B_UNHANDLED_INTERRUPT;
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);
	loop {
		iir = (*isa->read_io_8)( zp->base+IIR);
		unless (have_int( iir))
			break;
		h = B_HANDLED_INTERRUPT;
		if (THRE_int( iir))
			for (i=0; i<16; ++i) {
				int c = (*tm->ttyout)( tp, &zp->rover);
				if (c < 0) {
					(*isa->write_io_8)( zp->base+IER, IER_MS|IER_LS|IER_RDR);
					break;
				}
				(*isa->write_io_8)( zp->base+THR, c);
			}
		else if ((RBR_int( iir))
		or (timeout_int( iir))) {
			for (i=0; (*isa->read_io_8)( zp->base+LSR) & LSR_DR; ++i)
				(*tm->ttyin)( tp, &zp->rover, (*isa->read_io_8)( zp->base+RBR));
			/*
			 * defend against bogus hardware
			 */
			unless (i)
				(*isa->read_io_8)( zp->base+RBR);
		}
		else if (LSR_int( iir)) {
			uint lsr = (*isa->read_io_8)( zp->base+LSR);
		}
		else if (MSR_int( iir)) {
			uint msr = (*isa->read_io_8)( zp->base+MSR);
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
	uint	msr;

	struct zz *zp = ttoz( ztab, tp);
	switch (com) {
	case TTYENABLE:
		unless ((zp->base = devices[zp-ztab].base)
		and (zp->intr = devices[zp-ztab].intr))
			return (FALSE);
		setmodes( zp);
		install_io_interrupt_handler( zp->intr, zz_interrupt, tp, 0);
		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, (*isa->read_io_8)( zp->base+MSR)&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, (*isa->read_io_8)( zp->base+MSR)&MSR_CTS);
		(*isa->write_io_8)( zp->base+MCR, MCR_DTR|MCR_RTS|MCR_IRQ_ENABLE);
		(*isa->write_io_8)( zp->base+IER, IER_MS|IER_LS|IER_RDR);
		break;
	case TTYDISABLE:
		(*isa->write_io_8)( zp->base+IER, 0);
		(*isa->write_io_8)( zp->base+MCR, 0);
		remove_io_interrupt_handler( zp->intr, zz_interrupt, tp);
		break;
	case TTYSETMODES:
		setmodes( zp);
		break;
	case TTYISTOP:
		(*isa->write_io_8)( zp->base+MCR, (*isa->read_io_8)( zp->base+MCR)&~MCR_RTS);
		break;
	case TTYIRESUME:
		(*isa->write_io_8)( zp->base+MCR, (*isa->read_io_8)( zp->base+MCR)|MCR_RTS);
		break;
	case TTYOSTART:
		(*isa->write_io_8)( zp->base+IER, IER_MS|IER_LS|IER_THRE|IER_RDR);
		break;
	case TTYOSYNC:
		return (((*isa->read_io_8)( zp->base+LSR)&LSR_THRE+LSR_TSRE) == LSR_THRE+LSR_TSRE);
	case TTYSETBREAK:
		(*isa->write_io_8)( zp->base+LCR, (*isa->read_io_8)( zp->base+LCR)|LCR_BREAK);
		break;
	case TTYCLRBREAK:
		(*isa->write_io_8)( zp->base+LCR, (*isa->read_io_8)( zp->base+LCR)&~LCR_BREAK);
		break;
	case TTYSETDTR:
		(*isa->write_io_8)( zp->base+MCR, (*isa->read_io_8)( zp->base+MCR)|MCR_DTR);
		break;
	case TTYCLRDTR:
		(*isa->write_io_8)( zp->base+MCR, (*isa->read_io_8)( zp->base+MCR)&~MCR_DTR);
		break;
	case TTYGETSIGNALS:
		msr = (*isa->read_io_8)( zp->base+MSR);
		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, msr&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, msr&MSR_CTS);
		(*tm->ttyhwsignal)( tp, r, TTYHWDSR, msr&MSR_DSR);
		(*tm->ttyhwsignal)( tp, r, TTYHWRI, msr&MSR_RI);
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
			(*isa->write_io_8)( zp->base+MCR, (*isa->read_io_8)( zp->base+MCR)&~MCR_DTR);
		else {
			if (speed <= B1200)
				(*isa->write_io_8)( zp->base+FCR, FCR_ENABLE|FCR_RX_RESET|FCR_TX_RESET);
			else
				(*isa->write_io_8)( zp->base+FCR, FCR_ENABLE|FCR_RX_RESET|FCR_TX_RESET|FCR_RX_TRIGGER_8);
			(*isa->write_io_8)( zp->base+LCR, LCR_DLAB);
			(*isa->write_io_8)( zp->base+DLL, divisor & 0x00ff);
			(*isa->write_io_8)( zp->base+DLH, divisor >> 8);
		}
		(*isa->write_io_8)( zp->base+LCR, lcr);
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
	static char const	*list[1+NZZ+1];
	uint	i;
	uint	j;

	if(pcmcia){
		int n = 0;
		/* release any old pcmcia-related entries */
		for (i=0; i<NZZ; i++){
			if(do_not_save[i]){
				devices[i].base = 0;
				devices[i].intr = 0;
				do_not_save[i] = 0;
			}
		}
		/* find any new pcmcia related entries */
		for (i=0; i<NZZ; i++){
			if(devices[i].base == 0){
				if(pcmcia->get_nth_device(n,&devices[i].base,&devices[i].intr)){
					/* no more devices to be had */
					break; 
				} else {
					/* got one - make sure we don't auto-save on exit */
					do_not_save[i] = 1;
					n++;
				}
			}
		}
	}
	j = 0;
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
	return (list);
}


device_hooks	*
find_device( const char * dev)
{
	static device_hooks dh = {
		zz_open, zz_close, zz_free, zz_control, zz_read, zz_write, zz_select, zz_deselect
	};

	dev = dev;
	return (&dh);
}


static const char	DEVCONF[]	= "be_serial_conf";


status_t
init_driver( )
{
	port_id		p;
	status_t	s;
	uint		i;
	int32		_;

	if (platform( ) == B_MAC_PLATFORM)
		return (B_ERROR);
	p = find_port( DEVCONF);
	if (p < 0) {
		p = create_port( 1, DEVCONF);
		if (p < B_OK)
			return (B_ERROR);
		set_port_owner( p, B_SYSTEM_TEAM);
		if (platform( ) == B_BEBOX_PLATFORM) {
			static struct device d[] = {
				0x3F8, B_INT_COM1,
				0x2F8, B_INT_COM2,
				0x380, B_INT_SERIAL3,
				0x388, B_INT_SERIAL4,
				0x3E8, B_INT_COM1,
				0x2E8, B_INT_COM2
			};
			memcpy( devices, d, sizeof d);
		}
		else
			configure( devices, NZZ);
	}
	s = get_module( B_ISA_MODULE_NAME, (module_info **) &isa);
	unless (s == B_OK)
		return (s);
	s = get_module( B_TTY_MODULE_NAME, (module_info **)&tm);
	unless (s == B_OK) {
		put_module( B_ISA_MODULE_NAME);
		return (s);
	}
	get_module( "busses/serial/pcmcia/v0" , (module_info**)&pcmcia);
	if (port_count( p))
		read_port( p, &_, devices, sizeof devices);
	publish_devices( );
	ddbackground( &zdd);
	for (i=0; i<nel( ztab); ++i)
		(*tm->ttyinit)( &ztab[i].tty, FALSE);
	return (s);
}


void
uninit_driver( )
{
	port_id	p;
	uint	i;

	for (i=0; i<NZZ; ++i)
		if (do_not_save[i]) {
			devices[i].base = 0;
			devices[i].intr = 0;
		}
	p = find_port( DEVCONF);
	unless (p < 0)
		write_port( p, 0, devices, sizeof devices);
	put_module( B_TTY_MODULE_NAME);
	put_module( B_ISA_MODULE_NAME);
	if (pcmcia)
		put_module( "busses/serial/pcmcia/v0");
}


int32	api_version	= B_CUR_DRIVER_API_VERSION;
