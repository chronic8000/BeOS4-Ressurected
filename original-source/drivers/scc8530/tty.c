

#include	<KernelExport.h>
#include	<Drivers.h>
#include	<signal.h>
#include	<termios.h>
#include	"rico.h"
#include	"str.h"
#include	"wchan.h"
#include	"tty.h"


#define	BSIZE		1000
#define	ILOWATER	(BSIZE * 1/4)
#define	IHIWATER	(BSIZE * 3/4)
#define	OHIWATER	(BSIZE - 1)


const static struct termios TDEFAULT = {
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
	ctrl( 'Q'),		/* c_cc[VSTART] */
	ctrl( 'S'),		/* c_cc[VSTOP] */
	0			/* c_cc[VSUSP] */
};
const static struct winsize WDEFAULT = {
	24, 80, 240, 800
};


void	*malloc( );
bool	osync( struct tty *),
	terminated( struct tty *);


static struct tty	*active;


/*
 * To do:
 *	software flow control
 *	SETA* needs needs flushing & waiting behavior
 *	SETA* needs correct RTS response (evaliresume)
 */
long
ttyopen( struct ttyfile *tf, struct wchan *w, bool (*s)( struct tty *, uint))
{
	struct tty	*tp	= tf->tty;

	if (tp->flags & TTYEXCLUSIVE)
		return (B_BUSY);
	if (tf->flags & O_EXCL) {
		if (tp->nopen)
			return (B_BUSY);
		tp->flags |= TTYEXCLUSIVE;
	}
	unless (tp->nopen++) {
		unless ((salloc( &tp->istr, BSIZE))
		and (salloc( &tp->ostr, BSIZE))) {
			cleartty( tp);
			return (B_ERROR);
		}
		tp->next = active;
		active = tp;
		tp->w = w;
		tp->t = TDEFAULT;
		tp->wsize = WDEFAULT;
		tp->flags |= TTYREADING;
		tp->service = s;
		unless ((*tp->service)( tp, TTYENABLE)) {
			cleartty( tp);
			return (ENODEV);
		}
	}
	unless (tf->flags & O_NONBLOCK)
		until (tp->flags & TTYCARRIER)
			unless (wsleep( tp->w)) {
				unless (--tp->nopen) {
					(*tp->service)( tp, TTYDISABLE);
					cleartty( tp);
				}
				return (B_ERROR);
			}
	return (B_OK);
}


long
ttyclose( struct ttyfile *tf)
{
	struct tty	*tp	= tf->tty;

	loop {
		unless (tp->nopen == 1)
			break;
		if (scount( &tp->ostr)) {
			unless (wsleep( tp->w))
				break;
		}
		else {
			if ((*tp->service)( tp, TTYOSYNC)) {
				wsnooze( tp->w, 50000);
				break;
			}
			unless (wsnooze( tp->w, 10000))
				break;
		}
	}
	if (tp->nopen == 1) {
		struct wchan *w = tp->w;
		(*tp->service)( tp, TTYDISABLE);
		cleartty( tp);
		wsnooze( w, 200000);
	}
	else
		--tp->nopen;
	return (B_OK);
}


long
ttyread( struct ttyfile *tf, char buf[], size_t *count)
{
	bigtime_t	lastt	= system_time( );
	uint		lastn	= 0,
			i	= 0;
	struct tty	*tp	= tf->tty;

	loop {
		uint n = scount( &tp->istr);
		if (tp->t.c_lflag & ICANON) {
			if (terminated( tp)) {
				n = min( n, *count);
				while (i < n) {
					uint c = sgetb( &tp->istr);
					if (c == tp->t.c_cc[VEOF])
						break;
					buf[i++] = c;
					if (c == '\n')
						break;
				}
				iresume( tp);
				*count = i;
				return (B_OK);
			}
		}
		else {
			unless (n < *count) {
				while (i < *count)
					buf[i++] = sgetb( &tp->istr);
				iresume( tp);
				*count = i;
				return (B_OK);
			}
			if (tp->t.c_cc[VMIN]) {
				unless (n < tp->t.c_cc[VMIN]) {
					while (i < n)
						buf[i++] = sgetb( &tp->istr);
					iresume( tp);
					*count = i;
					return (B_OK);
				}
				if ((lastn)
				and (tp->t.c_cc[VTIME])) {
					if (lastt+tp->t.c_cc[VTIME]*100000 < system_time( )) {
						while (i < n)
							buf[i++] = sgetb( &tp->istr);
						iresume( tp);
						*count = i;
						return (B_OK);
					}
				}
			}
			else {
				if (n) {
					while (i < n)
						buf[i++] = sgetb( &tp->istr);
					iresume( tp);
					*count = i;
					return (B_OK);
				}
				unless (system_time( ) < lastt+tp->t.c_cc[VTIME]*100000) {
					*count = 0;
					return (B_OK);
				}
			}
		}
		unless (n == lastn) {
			lastn = n;
			lastt = system_time( );
		}
		unless (tp->flags & TTYCARRIER) {
			*count = 0;
			return (B_ERROR);
		}
		if (tf->flags & O_NONBLOCK) {
			*count = 0;
			return (EAGAIN);
		}
		unless (wsleeptimeout( tp->w, 100000)) {
			*count = 0;
			return (B_INTERRUPTED);
		}
	}
	*count = 0;
	return (B_OK);
}


long
ttywrite( struct ttyfile *tf, const char buf[], size_t *count)
{
	struct tty	*tp	= tf->tty;
	uint		n,
			i;

	n = 0;
	while ((tp->flags & TTYCARRIER)
	and (n < *count)) {
		if (scount( &tp->ostr) < OHIWATER) {
			emit( tp, buf[n++]);
			continue;
		}
		unless (wsleep( tp->w)) {
			*count = n;
			return (B_INTERRUPTED);
		}
	}
	if (*count = n)
		return (B_OK);
	return (B_ERROR);
}


long
ttycontrol( struct ttyfile *tf, ulong com, void *buf, size_t len)
{
	struct termio	*tio	= buf;
	struct tty	*tp	= tf->tty;

	switch (com) {
	case TCGETA:
		*tio = tp->t;
		break;
	case TCSETA:
	case TCSETAF:
	case TCSETAW:
		if (tio->c_cflag == tp->t.c_cflag)
			tp->t = *tio;
		else {
			unless (osync( tp))
				return (B_INTERRUPTED);
			tp->t = *tio;
			(*tp->service)( tp, TTYSETMODES);
		}
		evalwritable( tp);
		evalcarrier( tp);
		break;
	case TCWAITEVENT:
		return (waitevent( tp, buf));
	case TCFLSH:
		switch ((int)buf) {
		case 0:
			sclear( &tp->istr);
			iresume( tp);
			break;
		case 1:
			sclear( &tp->ostr);
		case 2:
			sclear( &tp->istr);
			iresume( tp);
		}
		break;
	case TCSBRK:
		unless (osync( tp))
			return (B_INTERRUPTED);
		(*tp->service)( tp, TTYSETBREAK);
		wsnooze( tp->w, 250000);
		(*tp->service)( tp, TTYCLRBREAK);
		break;
	case TCXONC:
	case TCQUERYCONNECTED:
		break;
	case TCSETDTR:
		if (*(bool *)buf)
			(*tp->service)( tp, TTYSETDTR);
		else
			(*tp->service)( tp, TTYCLRDTR);
		break;
	case TCSETRTS:
		unless (tp->flags & TTYFLOWFORCED)
			if (*(bool *)buf)
				(*tp->service)( tp, TTYIRESUME);
			else
				(*tp->service)( tp, TTYISTOP);
		break;
	case TCGETBITS:
		if (buf) {
			uint i = 0;
			(*tp->service)( tp, TTYGETSIGNALS);
			if (tp->flags & TTYHWCTS)
				i |= TCGB_CTS;
			if (tp->flags & TTYHWDSR)
				i |= TCGB_DSR;
			if (tp->flags & TTYHWDCD)
				i |= TCGB_DCD;
			if (tp->flags & TTYHWRI)
				i |= TCGB_RI;
			*(uint *)buf = i;
		}
		break;
	case TIOCGWINSZ:
		*(struct winsize *)buf = tp->wsize;
		break;
	case TIOCSWINSZ:
	case 'wsiz':
		unless ((tp->wsize.ws_row == ((struct winsize *)buf)->ws_row)
		and (tp->wsize.ws_col == ((struct winsize *)buf)->ws_col)) {
			tp->wsize = *(struct winsize *)buf;
			if (tp->pgid)
				send_signal( -tp->pgid, SIGWINCH);
		}
		break;
	case B_SET_BLOCKING_IO:
		tf->flags &= ~ O_NONBLOCK;
		break;
	case B_SET_NONBLOCKING_IO:
		tf->flags |= O_NONBLOCK;
		break;
	case 'pgid':
		tp->pgid = (pid_t) buf;
		break;
	case 'ichr':
		while (tp->flags & TTYCARRIER) {
			if (scount( &tp->istr) >= *(uint *)buf) {
				*(uint *)buf = scount( &tp->istr);
				return (B_OK);
			}
			unless (wsleep( tp->w))
				return (B_INTERRUPTED);
		}
		return (B_ERROR);
	case 'ochr':
		while (scount( &tp->ostr)+*(uint *)buf > OHIWATER)
			unless ((tp->flags & TTYCARRIER)
			and (wsleep( tp->w)))
				return (B_ERROR);
		*(uint *)buf = OHIWATER - scount( &tp->ostr);
	}
	return (B_OK);
}


static
waitevent( struct tty *tp, uint *nbyte)
{
	uint	n;

	bigtime_t tfin = 0;
	if (tp->t.c_cc[VTIME])
		tfin = system_time( ) + tp->t.c_cc[VTIME]*100000;
	until (n = scount( &tp->istr)) {
		bigtime_t t = 0;
		if (tfin) {
			bigtime_t tcur = system_time( );
			unless (tcur < tfin)
				break;
			t = tfin - tcur;
		}
		unless (wsleeptimeout( tp->w, t))
			return (B_INTERRUPTED);
	}
	if (nbyte)
		*(uint *)nbyte = n;
	return (n);
}


void
ttyin( struct tty *tp, int c)
{
	uint	i;

	if ((c == tp->t.c_cc[VINTR])
	and (tp->t.c_lflag & ISIG)) {
		sclear( &tp->istr);
		iresume( tp);
		if (tp->pgid)
			send_signal( -tp->pgid, SIGINT);
	}
	else if ((c == tp->t.c_cc[VQUIT])
	and (tp->t.c_lflag & ISIG)) {
		sclear( &tp->istr);
		iresume( tp);
		if (tp->pgid)
			send_signal( -tp->pgid, SIGQUIT);
	}
	else if ((c == tp->t.c_cc[VKILL])
	and (tp->t.c_lflag & ICANON))
		loop {
			c = sunputb( &tp->istr);
			if (c == EOF)
				break;
			if (c == '\n') {
				sputb( &tp->istr, c);
				break;
			}
			echo( tp, '\b');
			echo( tp, ' ');
			echo( tp, '\b');
		}
	else if ((c == tp->t.c_cc[VERASE])
	and (tp->t.c_lflag & ICANON)) {
		c = sunputb( &tp->istr);
		unless (c == EOF)
			if (c == '\n')
				sputb( &tp->istr, c);
			else {
				echo( tp, '\b');
				echo( tp, ' ');
				echo( tp, '\b');
			}
	}
	else if ((c == tp->t.c_cc[VEOF])
	and (tp->t.c_lflag & ICANON))
		sputb( &tp->istr, c);
	else {
		if ((c == '\t')
		and (tp->t.c_lflag & ICANON))
			echo( tp, c);
		else {
			if ((c == '\r')
			and (tp->t.c_iflag & ICRNL))
				c = '\n';
			echo( tp, c);
		}
		sputb( &tp->istr, c);
	}
	wakeup( tp->w);
	if (((tp->flags&TTYFLOWFORCED) || (tp->t.c_cflag&RTSFLOW))
	and (scount( &tp->istr) >= IHIWATER)
	and (tp->flags & TTYREADING)) {
		tp->flags &= ~ TTYREADING;
		(*tp->service)( tp, TTYISTOP);
	}
}


ttyout( struct tty *tp)
{

	if ((scount( &tp->ostr))
	and (tp->flags & TTYWRITABLE))
		return (sgetb( &tp->ostr));
	tp->flags &= ~TTYWRITING;
	wakeup( tp->w);
	return (-1);
}


void
ttyhwsignal( struct tty *tp, int sig, bool asserted)
{

	switch (sig) {
	case TTYHWDCD:
		if (asserted)
			tp->flags |= TTYHWDCD;
		else
			tp->flags &= ~ TTYHWDCD;
		evalcarrier( tp);
		break;
	case TTYHWCTS:
		if (asserted)
			tp->flags |= TTYHWCTS;
		else
			tp->flags &= ~ TTYHWCTS;
		evalwritable( tp);
	case TTYHWDSR:
		if (asserted)
			tp->flags |= TTYHWDSR;
		else
			tp->flags &= ~ TTYHWDSR;
		break;
	case TTYHWRI:
		if (asserted)
			tp->flags |= TTYHWRI;
		else
			tp->flags &= ~ TTYHWRI;
	}
}


static
evalcarrier( struct tty *tp)
{

	if ((tp->t.c_cflag & CLOCAL)
	or (tp->flags & TTYHWDCD))
		tp->flags |= TTYCARRIER;
	else
		tp->flags &= ~ TTYCARRIER;
	wakeup( tp->w);
}


static
evalwritable( struct tty *tp)
{

	if (((tp->flags&TTYFLOWFORCED) || (tp->t.c_cflag&CTSFLOW))
	and (not (tp->flags & TTYHWCTS)))
		tp->flags &= ~ TTYWRITABLE;
	else
		tp->flags |= TTYWRITABLE;
	ostart( tp);
}


static
iresume( struct tty *tp)
{

	if ((not (tp->flags & TTYREADING))
	and (scount( &tp->istr) <= ILOWATER)) {
		tp->flags |= TTYREADING;
		(*tp->service)( tp, TTYIRESUME);
	}
}


static bool
osync( struct tty *tp)
{

	loop {
		if (scount( &tp->ostr)) {
			unless (wsleep( tp->w))
				return (FALSE);
			continue;
		}
		unless ((*tp->service)( tp, TTYOSYNC)) {
			unless (wsnooze( tp->w, 10000))
				return (FALSE);
			continue;
		}
		return (TRUE);
	}
}


static bool
terminated( struct tty *tp)
{
	uint	i;

	uint n = scount( &tp->istr);
	uchar *buf = sgetm( &tp->istr, n);
	sungetm( &tp->istr, n);
	for (i=0; i<n; ++i)
		if ((buf[i] == '\n')
		or (buf[i] == tp->t.c_cc[VEOF]))
			return (TRUE);
	return (FALSE);
}


static
echo( struct tty *tp, uint c)
{

	if (tp->t.c_lflag & ECHO)
		emit( tp, c);
}


static
emit( struct tty *tp, uint c)
{

	if ((c == '\n')
	and (tp->t.c_oflag & OPOST)
	and (tp->t.c_oflag & ONLCR))
		sputb( &tp->ostr, '\r');
	sputb( &tp->ostr, c);
	ostart( tp);
}


static
ostart( struct tty *tp)
{

	if ((not (tp->flags&TTYWRITING))
	and (tp->flags & TTYWRITABLE)
	and (scount( &tp->ostr))) {
		tp->flags |= TTYWRITING;
		(*tp->service)( tp, TTYOSTART);
	}
}


static
cleartty( struct tty *tp)
{
	struct tty	*p;

	if (tp == active)
		active = tp->next;
	else
		for (p=active; p->next; p=p->next)
			if (tp == p->next) {
				p->next = tp->next;
				break;
			}
	sfree( &tp->istr);
	sfree( &tp->ostr);
	memset( tp, 0, sizeof *tp);
}
