

#include	<KernelExport.h>
#include	<Drivers.h>
#include	<termios.h>
#include	<tty/ttylayer.h>


#define	NPTY	(nel( names) / 2)

/* GRRRRRRRRRR. pointer arithmetic has to be done on aligned pointers!!! */

/* #define	ttop( p, t)	(& (p) [(struct pty *)(t)-(p)]) */

#define       ttop(p,t) ((p) + ((((uint32)(t))-((uint32)(p)))/sizeof(struct pty)))


struct side {
	struct tty	tty;
	bool		dtr;
};
struct pty {
	struct ddomain	dd;
	struct side	master,
			slave;
};


/*
 * This naming scheme reduces clutter in /dev, while minimizing the pain when
 * porting programs from BSD & Linux.  Behold:
 *
 *	BSD	/dev/ptyp0
 *	BeOS	/dev/pt/p0
 */
static const char *names[] = {
	"pt/p0", "pt/p1", "pt/p2", "pt/p3", "pt/p4", "pt/p5", "pt/p6", "pt/p7",
	"pt/p8", "pt/p9", "pt/pa", "pt/pb", "pt/pc", "pt/pd", "pt/pe", "pt/pf",
	"pt/q0", "pt/q1", "pt/q2", "pt/q3", "pt/q4", "pt/q5", "pt/q6", "pt/q7",
	"pt/q8", "pt/q9", "pt/qa", "pt/qb", "pt/qc", "pt/qd", "pt/qe", "pt/qf",
	"pt/r0", "pt/r1", "pt/r2", "pt/r3", "pt/r4", "pt/r5", "pt/r6", "pt/r7",
	"pt/r8", "pt/r9", "pt/ra", "pt/rb", "pt/rc", "pt/rd", "pt/re", "pt/rf",
	"pt/s0", "pt/s1", "pt/s2", "pt/s3", "pt/s4", "pt/s5", "pt/s6", "pt/s7",
	"pt/s8", "pt/s9", "pt/sa", "pt/sb", "pt/sc", "pt/sd", "pt/se", "pt/sf",
	"tt/p0", "tt/p1", "tt/p2", "tt/p3", "tt/p4", "tt/p5", "tt/p6", "tt/p7",
	"tt/p8", "tt/p9", "tt/pa", "tt/pb", "tt/pc", "tt/pd", "tt/pe", "tt/pf",
	"tt/q0", "tt/q1", "tt/q2", "tt/q3", "tt/q4", "tt/q5", "tt/q6", "tt/q7",
	"tt/q8", "tt/q9", "tt/qa", "tt/qb", "tt/qc", "tt/qd", "tt/qe", "tt/qf",
	"tt/r0", "tt/r1", "tt/r2", "tt/r3", "tt/r4", "tt/r5", "tt/r6", "tt/r7",
	"tt/r8", "tt/r9", "tt/ra", "tt/rb", "tt/rc", "tt/rd", "tt/re", "tt/rf",
	"tt/s0", "tt/s1", "tt/s2", "tt/s3", "tt/s4", "tt/s5", "tt/s6", "tt/s7",
	"tt/s8", "tt/s9", "tt/sa", "tt/sb", "tt/sc", "tt/sd", "tt/se", "tt/sf",
	0
};
static tty_module_info	*tm;
static struct pty	ptab[NPTY];

static bool	pty_service( struct tty *, struct ddrover *, uint);
static void	setmastermodes( struct ttyfile *, struct ddrover *);
static void	acquire( struct ddrover *, struct tty *);
extern void	*malloc( );


static status_t
pty_open( const char *n, uint32 f, void **d)
{
	struct ttyfile	*tf;
	struct pty	*p;
	struct ddrover	*r;
	int		i;
	status_t	s;

	i = 0;
	until (streq( n, names[i]))
		unless (names[++i])
			return (B_ERROR);
	p = ptab + i%NPTY;
	s = ENOMEM;
	if (tf = malloc( sizeof *tf)) {
		*d = tf;
		tf->flags = f;
		if (r = (*tm->ddrstart)( 0)) {
			if (i < NPTY) {
				tf->tty = &p->master.tty;
				acquire( r, tf->tty);
				if ((p->master.tty.nopen)
				or (p->slave.tty.nopen))
					s = EIO;
				else {
					s = (*tm->ttyopen)( tf, r, pty_service);
					if (s == B_OK)
						setmastermodes( tf, r);
				}
			}
			else {
				tf->tty = &p->slave.tty;
				acquire( r, tf->tty);
				unless (p->master.tty.nopen)
					s = EIO;
				else
					s = (*tm->ttyopen)( tf, r, pty_service);
			}
			(*tm->ddrdone)( r);
		}
		unless (s == B_OK)
			free( tf);
	}
	return (s);
}


static status_t
pty_close( void *d)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	acquire( r, ((struct ttyfile *)d)->tty);
	s = (*tm->ttyclose)( d, r);
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
pty_free( void *d)
{
	struct ddrover	*r;
	status_t	s;

	if (r = (*tm->ddrstart)( 0)) {
		acquire( r, ((struct ttyfile *)d)->tty);
		s = (*tm->ttyfree)( d, r);
		(*tm->ddrdone)( r);
	}
	else
		s = ENOMEM;
	free( d);
	return (s);
}


static status_t
pty_read( void *d, off_t o, void *buf, size_t *count)
{
	struct tty	*tp	= ((struct ttyfile *)d)->tty;
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	acquire( r, tp);
	s = (*tm->ttyread)( d, r, buf, count);
	if ((s == EIO)
	and (tp == &(ttop( ptab, tp))->slave.tty))
		s = B_OK;
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
pty_write( void *d, off_t o, const void *buf, size_t *count)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	acquire( r, ((struct ttyfile *)d)->tty);
	s = (*tm->ttywrite)( d, r, buf, count);
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
pty_control( void *d, uint32 com, void *buf, size_t len)
{
	struct ttyfile	*tf	= d;
	struct tty	*tp	= tf->tty;
	struct pty	*p	= ttop( ptab, tp);
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	switch (com) {
	case TCGETA:
	case TCSETA:
	case TCSETAF:
	case TCSETAW:
	case 'pgid':
	case 'wsiz':
	case TIOCSWINSZ:
		if (tp == &p->master.tty) {
			struct ttyfile slave = *tf;
			slave.tty = &p->slave.tty;
			if (p->slave.dtr) {
				acquire( r, slave.tty);
				s = (*tm->ttycontrol)( &slave, r, com, buf, len);
			}
			else
				s = B_ERROR;
			break;
		}
	default:
		acquire( r, tp);
		s = (*tm->ttycontrol)( tf, r, com, buf, len);
	}
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
pty_select( void *d, uint8 event, uint32 ref, selectsync *sync)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttyselect)( d, r, event, ref, sync);
	(*tm->ddrdone)( r);
	return (s);
}


static status_t
pty_deselect( void *d, uint8 event, selectsync *sync)
{
	struct ddrover	*r;
	status_t	s;

	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttydeselect)( d, r, event, sync);
	(*tm->ddrdone)( r);
	return (s);
}


static bool
pty_service( struct tty *tp, struct ddrover *r, uint com)
{
	struct side	*src,
			*dst;

	struct pty *p = ttop( ptab, tp);
	if (tp == &p->master.tty) {
		src = &p->master;
		dst = &p->slave;
	}
	else {
		src = &p->slave;
		dst = &p->master;
	}
	(*tm->ttyilock)( &src->tty, r, TRUE);
	if (dst->dtr)
		(*tm->ttyilock)( &dst->tty, r, TRUE);
	switch (com) {
	case TTYENABLE:
		src->dtr = TRUE;
		(*tm->ttyhwsignal)( &src->tty, r, TTYHWCTS, TRUE);
		if (src == &p->master) {
			if (p->slave.dtr)
				(*tm->ttyhwsignal)( &p->slave.tty, r, TTYHWDCD, TRUE);
			(*tm->ttyhwsignal)( &p->master.tty, r, TTYHWDCD, TRUE);
		}
		else {
			if (p->master.dtr)
				(*tm->ttyhwsignal)( &p->slave.tty, r, TTYHWDCD, TRUE);
		}
		ttyforceflow( &src->tty);
		break;
	case TTYDISABLE:
		src->dtr = FALSE;
		if (dst->dtr)
			(*tm->ttyhwsignal)( &dst->tty, r, TTYHWDCD, FALSE);
		break;
	case TTYSETMODES:
		if ((not (src->tty.t.c_cflag&CBAUD))
		and (dst->dtr))
			(*tm->ttyhwsignal)( &dst->tty, r, TTYHWDCD, FALSE);
		break;
	case TTYOSTART:
		loop {
			int c = (*tm->ttyout)( &src->tty, r);
			if (c < 0)
				break;
			if (dst->dtr)
				(*tm->ttyin)( &dst->tty, r, c);
		}
		break;
	case TTYISTOP:
		if (dst->dtr)
			(*tm->ttyhwsignal)( &dst->tty, r, TTYHWCTS, FALSE);
		break;
	case TTYIRESUME:
		if (dst->dtr)
			(*tm->ttyhwsignal)( &dst->tty, r, TTYHWCTS, TRUE);
		break;
	case TTYSETDTR:
		if (dst->dtr)
			(*tm->ttyin)( &dst->tty, r, 003);
	}
	(*tm->ttyilock)( &src->tty, r, FALSE);
	if (dst->dtr)
		(*tm->ttyilock)( &dst->tty, r, FALSE);
	return (TRUE);
}


static void
setmastermodes( struct ttyfile *tf, struct ddrover *r)
{
	static struct termios MDEFAULT = {
		0,			/* c_iflag */
		0,			/* c_oflag */
		B19200|CS8|CREAD|HUPCL,	/* c_cflag */
		0,			/* c_lflag */
		0,			/* c_line */
		0,			/* c_ixxxxx */
		0,			/* c_oxxxxx */
		ctrl( 'C'),		/* c_cc[VINTR] */
		ctrl( '\\'),		/* c_cc[VQUIT] */
		ctrl( 'H'),		/* c_cc[VERASE] */
		ctrl( 'U'),		/* c_cc[VKILL] */
		1,			/* c_cc[VMIN] */
		10,			/* c_cc[VTIME] */
		0,			/* c_cc[VEOL2] */
		0,			/* c_cc[VSWTCH] */
		ctrl( 'S'),		/* c_cc[VSTART] */
		ctrl( 'Q'),		/* c_cc[VSTOP] */
		0			/* c_cc[VSUSP] */
	};

	(*tm->ttycontrol)( tf, r, TCSETA, &MDEFAULT, sizeof MDEFAULT);
}


static void
acquire( struct ddrover *r, struct tty *tp)
{
	struct pty	*p	= ttop( ptab, tp);

	(*tm->ddacquire)( r, &p->dd);
	if (tp == &p->master.tty)
		(*tm->ttyilock)( &p->slave.tty, r, TRUE);
	else
		(*tm->ttyilock)( &p->master.tty, r, TRUE);
}


static device_hooks device = {
	pty_open, pty_close, pty_free, pty_control, pty_read, pty_write, pty_select, pty_deselect
};


const char	**
publish_devices( )
{

	return (names);
}


device_hooks	*
find_device( const char *s)
{

	return (&device);
}


status_t
init_driver( )
{
	uint	i;

	unless (get_module( B_TTY_MODULE_NAME, (module_info **)&tm) == B_OK)
		return (B_ERROR);
	for (i=0; i<nel( ptab); ++i) {
		ddbackground( &ptab[i].dd);
		(*tm->ttyinit)( &ptab[i].master.tty, TRUE);
		(*tm->ttyinit)( &ptab[i].slave.tty, TRUE);
	}
	return (B_OK);
}


void
uninit_driver( )
{

	put_module( B_TTY_MODULE_NAME);
}


int32	api_version	= B_CUR_DRIVER_API_VERSION;
