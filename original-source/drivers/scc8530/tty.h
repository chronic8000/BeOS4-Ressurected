

struct tty {
	struct tty	*next;
	uint		nopen,
			flags;
	struct wchan	*w;
	pid_t		pgid;
	struct termios	t;
	struct str	istr,
			ostr;
	struct winsize	wsize;
	bool		(*service)( struct tty *, uint);
};
struct ttyfile {
	struct tty	*tty;
	uint		flags;
};


/* flags
 */
#define	TTYCARRIER	(1 << 0)
#define	TTYWRITABLE	(1 << 1)
#define	TTYWRITING	(1 << 2)
#define	TTYREADING	(1 << 3)
#define	TTYOSTOPPED	(1 << 4)
#define	TTYBLOCKABLE	(1 << 5)
#define	TTYEXCLUSIVE	(1 << 6)
#define	TTYHWDCD	(1 << 7)
#define	TTYHWCTS	(1 << 8)
#define	TTYHWDSR	(1 << 9)
#define	TTYHWRI		(1 << 10)
#define	TTYFLOWFORCED	(1 << 11)

/* device commands
 */
#define	TTYENABLE	0
#define	TTYDISABLE	1
#define	TTYSETMODES	2
#define	TTYOSTART	3
#define	TTYOSYNC	4
#define	TTYISTOP	5
#define	TTYIRESUME	6
#define	TTYSETBREAK	7
#define	TTYCLRBREAK	8
#define	TTYSETDTR	9
#define	TTYCLRDTR	10
#define	TTYGETSIGNALS	11


#define	ttyforceflow( t)	((t)->flags |= TTYFLOWFORCED)


long	ttyopen( struct ttyfile *, struct wchan *, bool (*)( struct tty *, uint)),
	ttyclose( struct ttyfile *),
	ttyread( struct ttyfile *, char *, size_t *),
	ttywrite( struct ttyfile *, const char *, size_t *),
	ttycontrol( struct ttyfile *, ulong, void *, size_t);
void	ttyhwsignal( struct tty *, int, bool),
	ttyin( struct tty *, int);
int	ttyout( struct tty *);
