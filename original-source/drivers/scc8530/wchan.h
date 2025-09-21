

struct wchan {
	bool		bg;
	sem_id		sem;
	spinlock	sl;
	cpu_status	ps;
	struct wlink	*threads;
};

#define	wbackground( w)	((w)->bg = TRUE)

bool	wsleep( struct wchan *),
	wsleeptimeout( struct wchan *, bigtime_t),
	wsnooze( struct wchan *, bigtime_t);
void	wakeup( struct wchan *),
	wacquire( struct wchan *),
	wrelease( struct wchan *);
