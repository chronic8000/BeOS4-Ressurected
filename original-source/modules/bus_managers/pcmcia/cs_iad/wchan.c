

#include	<KernelExport.h>
#include	"wchan.h"

static void queue(struct wchan *, struct wlink *);
static void dequeue(struct wchan *, struct wlink *);

struct wlink {
	sem_id		sem;
	struct wlink	*next;
};

static struct wlink	*alloclink( struct wchan *);
static void		freelink( struct wlink *);
void *malloc( );


void
wacquire( struct wchan *w)
{

	if (w->bg) {
		static spinlock l;
		cpu_status ps = disable_interrupts( );
		acquire_spinlock( &l);
		if ( !(w->sem) )
			w->sem = create_sem( 1, "wchan0");
		release_spinlock( &l);
		restore_interrupts( ps);
		acquire_sem( w->sem);
	}
	else {
		cpu_status ps = disable_interrupts( );
		acquire_spinlock( &w->sl);
		w->ps = ps;
	}
}


void
wrelease( struct wchan *w)
{

	if (w->bg)
		release_sem( w->sem);
	else {
		cpu_status ps = w->ps;
		release_spinlock( &w->sl);
		restore_interrupts( ps);
	}
}


bool
wsleep( struct wchan *w)
{

	return (wsleeptimeout( w, 0));
}


bool
wsleeptimeout( struct wchan *w, bigtime_t t)
{
	struct wlink	*l;
	status_t	s;

	unsigned f = B_CAN_INTERRUPT;
	if ( !(l = alloclink( w)) )
		return (TRUE);		/* strange, but true */
	queue( w, l);

	wrelease( w);
	if (t)
		f |= B_TIMEOUT;
	s = acquire_sem_etc( l->sem, 1, f, t);
	wacquire( w);

	dequeue( w, l);
	freelink( l);
	return (s==B_OK || s==B_TIMED_OUT);
}


void
wakeup( struct wchan *w)
{
	struct wlink	*l;

	for (l=w->threads; l; l=l->next)
		release_sem_etc( l->sem, 1, B_DO_NOT_RESCHEDULE);
	w->threads = 0;
}


bool
wsnooze( struct wchan *w, bigtime_t usec)
{
	bool	b;

	wrelease( w);
	b = snooze( usec);
	wacquire( w);
	return (b == B_OK);
}


static void
queue( struct wchan *w, struct wlink *l)
{

	l->next = w->threads;
	w->threads = l;
}


static void
dequeue( struct wchan *w, struct wlink *l)
{
	struct wlink	*p;

	if (p = w->threads)
		if (p == l)
			w->threads = l->next;
		else
			while (p->next) {
				if (p->next == l) {
					p->next = l->next;
					break;
				}
				p = p->next;
			}
}


static struct wlink	*freelinks;


static struct wlink	*
alloclink( struct wchan *w)
{
	struct wlink	*l;

	if (l = freelinks) {
		freelinks = l->next;
		return (l);
	}
	wrelease( w);
	if (l = malloc( sizeof *l)) {
		l->sem = create_sem( 0, "wchan");
		if (l->sem < 0)
			free( l);
		else
			freelink( l);
	}
	snooze( 10000);
	wacquire( w);
	return (0);
}


static void
freelink( struct wlink *l)
{

	l->next = freelinks;
	freelinks = l;
}
