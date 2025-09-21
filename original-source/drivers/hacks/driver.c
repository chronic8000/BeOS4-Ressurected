#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>

#include "thread.h"
#include "vm.h"

extern thread_id	nextthid;
extern team_id		nexttmid;
extern char			*thread_alloc;
extern char			*team_alloc;

int32	api_version = 2;

bigtime_t		tt[20];

#define ADDRESSABLE(x) (((char *)x - thr->stktop) + cstktop)

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);

static device_hooks hacks_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

status_t
init_driver(void) {
	return B_OK;
}

static char * device_names[] = {
	"misc/hacks",
	NULL
};

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return device_names;
}

device_hooks *
find_device(const char *name) {
	int index = 0;
	while (device_names[index]) {
		if (strcmp(name, device_names[index]) == 0)
			return &hacks_device_hooks;
		index++;
	}
	return NULL;

}

void uninit_driver(void) {
}

static long
child_thread(void *data)
{
	return 0;
}

static sem_id
new_sem(sentry *sem_buf)
{
	int i;
	sem_id sem;
	
	for (i = 0, sem = nextsid; i < nsems; i++, sem++) {
		if (semaph[sidtoslot(sem)] == NULL) {
			semaph[sidtoslot(sem)] = sem_buf;
			nextsid = sem+1;
			return sem;
		}
	}
	
	return B_NO_MORE_SEMS;
}

static sem_id
create_sem_gen(int32 count, const char *name, bool kernel)
{
	sem_id			sem;
	sentry			*sptr;
	cpu_status		ps;
	team_rec		*tmr;
	int				len;

	if (count < 0)
		return B_BAD_VALUE;

	if (name == NULL)
		name = "";
		
	len = min(B_OS_NAME_LENGTH, strlen(name)+1);

	if ((sptr = (sentry *)malloc(sizeof(sentry) + len)) == NULL)
		return B_NO_MEMORY;

	memcpy(sptr->sname, name, len-1);
	sptr->sname[len-1] = '\0';
	
	ps = disable_interrupts();
	acquire_spinlock(&thrlock);

	if ((sem = new_sem(sptr)) < B_NO_ERROR) {
		release_spinlock(&thrlock);
		restore_interrupts(ps);
		free(sptr);
		dprintf("Sem create failure\n");
		return(sem);
	}

	sptr->sid = sem;
	sptr->semcnt = count;
	sptr->queue.head = sptr->queue.tail = NULL;

	/* insert the sem in the team semaphore list */

	if (kernel)
		tmr = team_tab[tmidtoslot(B_SYSTEM_TEAM)];
	else
		tmr = get_cur_thr()->tmr;
	sptr->tmr = tmr;
	sptr->pr_sem = NULL;
	if (tmr->sems)
		tmr->sems->pr_sem = sptr;
	sptr->nx_sem = tmr->sems;
	tmr->sems = sptr;

	release_spinlock(&thrlock);
	restore_interrupts(ps);
	return(sem);
}

static sem_id
create_sem(int32 count, const char *name)
{
	return create_sem_gen(count, name, TRUE);
}

static team_id
newtmid(void)
{
	team_id		i, tmid;
	cpu_status	ps;

	ps = disable_interrupts();
	acquire_spinlock(&thrlock);
	
	for (i=0, tmid = nexttmid; i<nteams; i++, tmid++)
		if (!team_alloc[tmidtoslot(tmid)]) {
			team_alloc[tmidtoslot(tmid)] = TRUE;
			nexttmid = tmid+1;

			release_spinlock(&thrlock);
			restore_interrupts(ps);
			return (tmid);
		}

	release_spinlock(&thrlock);
	restore_interrupts(ps);

	dprintf("KERNEL: Out of team space!\n");
	return(B_ERROR);
}

static thread_id
newthid(void)
{
	thread_id	i, thid;
	cpu_status	ps;

	ps = disable_interrupts();
	acquire_spinlock(&thrlock);
	
	for (i=0, thid = nextthid; i<nthreads; i++, thid++)
		if (!thread_alloc[thidtoslot(thid)]) {
			thread_alloc[thidtoslot(thid)] = TRUE;
			nextthid = thid+1;

			release_spinlock(&thrlock);
			restore_interrupts(ps);
			return (thid);
		}

	release_spinlock(&thrlock);
	restore_interrupts(ps);

	dprintf("KERNEL: Out of thread space!\n");
	return(B_ERROR);
}

/*
 *	parms_size returns the size in bytes needed to copy
 *	the parameters.
 */

static int
parms_size(int argc, char **argv, char **envp)
{
	int		transsz = 0;

	/* Note the limiting use of argc. This is because Benoit insists
	   on passing parameters (not strings) through the argc/argv
	   mechanism. Let's not tell him about envp.
	 */
	if (argv && argc > 0)
		while (*argv && argc--) 
			transsz += strlen(*argv++) + 1 + 4;
	if (envp)
		while (*envp)
			transsz += strlen(*envp++) + 1 + 4;

	/* add the NULLs terminating argv and envp, as well as errno */
	transsz += 2 * sizeof(void *);		
	transsz += sizeof(int);

	/* round up parameter to 8 bytes for efficiency */

	return ((transsz + 7) & ~7);
}

static int
count_parms(char **envp)
{
int	envc = 0;

	if (envp)
		while (*envp++)
			envc += 1;
	return(envc);
}

/*
 *	clone_parms copies the parameters.
 */

static void
clone_parms(	char		*sp,
				char		*ksp,
				int			argc_caller,
				int			envc,
				char		***argvp,
				char		***envpp)
{
	int		argc;
	char	**argv, **envp;
	char	*ostr;
	int		i, ptr_no;
	char	**pzone, *szone;
	
	argc = (argc_caller < 0) ? 0 : argc_caller;
	argv = *argvp;
	envp = *envpp;

	/* we need to copy the parameters argc and argv into	*/
	/* some data space accessible by the new task, since	*/
	/* it doesn't have access to the own copy of the caller.*/
 
	/* copy the parameters to the special zone		*/

	pzone = (char **)ksp;
	szone = ksp + ((envc+argc+1+1) * sizeof(char *));
	ptr_no = 0;

	/* then we do the argv list.  Remember to update the
	 * caller's version of argv so she knows where it is in the new
	 * space.
	 */
	if (argc_caller >= 0)
		*argvp = (char **)((char *)&pzone[ptr_no] + (sp - ksp));
	for (i=0; i<argc; i++) {
		pzone[ptr_no++] = szone + (sp - ksp);
		ostr = argv[i];
		while ((*szone++ = *ostr++) != '\0')
			;
	}
	pzone[ptr_no++] = NULL;
	
	/* Now the same thing for the environment variables.
	 */
	*envpp = (char **)((char *)&pzone[ptr_no] + (sp - ksp));
	for (i=0; i<envc; i++) {
		pzone[ptr_no++] = szone + (sp - ksp);
		ostr = envp[i];
		while ((*szone++ = *ostr++) != '\0')
			;
	}
	pzone[ptr_no++] = NULL;
}


/*
 * create.  once in memory etc. make a thread go.
 */

static thread_id
create(	void		*entry,			/* starting address */
		bool		kernel,			/* kernel thread? */
		team_id		tmid,			/* team of the thread */
		int32		priority,		/* initial priority */
		const char	*name,			/* thread name */
		int32		argc,			/* argument count to pass (< 0 to not copy) */
		char		**argv,			/* an array of arguments */
		char		**envp)			/* enviroment pointer */
{
	char 		newteam;	
	thread_id	thid;
	team_rec	*tmr;
	team_rec	*curtmr;
	thread_rec	*thr, *curthr;
	struct ioctx *par_ioctx;
	char		*sp, *csp;
	char		*va;
	uint		parsz;
	char		sem_name[B_OS_NAME_LENGTH], area_name[B_OS_NAME_LENGTH];
	int			envc;			/* count of environment variables */
	int			err;			/*  return code */
	int			i, j, len;
	cpu_status	ps;
	char		*custktop;
	area_id		cstaid;
bigtime_t		t;

	if (name == NULL)
		name = "naughty_thread";
		
	curthr = get_cur_thr();
	curtmr = curthr->tmr;

	/* tmid can only be one of the following:
	- 0: create a new team and a thread in it.
	- current team: create a thread in the current team.
	- kernel team: create a thread in the system team -- spawn_kernel_thread().
	- any other team: create a thread in the specified team (only for debug nub!)
	*/

	if ((tmid != 0) && (tmid != B_SYSTEM_TEAM) && (tmid != curtmr->tmid) &&
			(entry != (int (*)())debug_nub)) {
		err = B_BAD_TEAM_ID;
		goto error1;
	}

	/* allocate new team if necessary */

	newteam = (tmid == 0);
	tmr = NULL;
	if (newteam) {
		tmr = (team_rec *) malloc(sizeof(team_rec));
		if (tmr == NULL) {
			err = B_NO_MEMORY;
			goto error1;
		}
		memset(tmr, 0, sizeof(team_rec));
		
		tmid = newtmid();
		if (tmid < B_NO_ERROR) {
			err = B_NO_MORE_TEAMS;
			goto error2;
		}

		tmr->tmid = tmid;
		tmr->cnt = 0;
		tmr->db_port = -1;		/* signal not set up for debugging */
		tmr->report_death = TRUE;
		tmr->sems = NULL;
		tmr->ports = NULL;
		tmr->first_thr = NULL;
		tmr->last_thr = NULL;

		tmr->hasmainthread = FALSE;

		tmr->session_id = curtmr->session_id;
		tmr->pgrp_id    = curtmr->pgrp_id;
		tmr->uid        = curtmr->euid;
		tmr->euid       = curtmr->euid;
		tmr->gid        = curtmr->egid;
		tmr->egid       = curtmr->egid;
		tmr->disable_debugger = curtmr->disable_debugger;

    	/* save argc/argv so ps can see it. */

		tmr->argc = argc;
		for(i=0, len=0; (len < sizeof(tmr->args)) && (i < argc); i++) {
			for(j=0; argv[i][j] && (len < sizeof(tmr->args)); j++, len++)
				tmr->args[len] = argv[i][j];

			if ((len < sizeof(tmr->args)) && (i+1 < argc))
				tmr->args[len++] = ' ';
		}

		if (len < sizeof(tmr->args))  /* ensure null termination */
			tmr->args[len] = '\0';
		else
			tmr->args[sizeof(tmr->args)-1] = '\0';

		if (tmr->args[0] == '\0')
			strcpy(tmr->args, "-- NO ARGS --");

		/* create address space */

		tmr->asr = as_init(tmr);
		if (tmr->asr == NULL) {
			err = B_NO_MEMORY;
			goto error3;
		}

		/* Create a death semaphore.. */

		if ((tmr->death_sem = create_sem(0, "Wait for dying teams")) < 0)  {
			err = tmr->death_sem;
			goto error4;
		}
 
		set_sem_owner(tmr->death_sem, B_SYSTEM_TEAM);

		/* register team in team_tab */

		ps = disable_interrupts();
		acquire_spinlock(&thrlock);

		/* keep track of the process hierarchy */
		tmr->parent = curtmr;

		tmr->sibling_next = curtmr->children;
		tmr->sibling_prev = NULL;   /* we're going to become the head */

		if (curtmr->children)
			curtmr->children->sibling_prev = tmr;
		curtmr->children = tmr;

		team_tab[tmidtoslot(tmid)] = tmr;
		tmr->nx_tmr = NULL;
		tmr->pr_tmr = last_tmr;
		if (first_tmr == NULL)
			first_tmr = tmr;
		if (last_tmr != NULL)
			last_tmr->nx_tmr = tmr;
		last_tmr = tmr;

		release_spinlock(&thrlock);
		restore_interrupts(ps);
	}

	/* allocate thread_rec */

	len = (kernel ? sizeof(thread_rec) - sizeof(user_thread_rec_ext) : sizeof(thread_rec));
t = system_time();
	thr = (thread_rec *)malloc(len);
t = system_time() - t;
tt[9] += t;
	if (thr == NULL) {
		err = B_NO_MEMORY;
		goto error5;
	}
t = system_time();
	memset(thr, 0, len);
t = system_time() - t;
tt[0] += t;

	parsz = parms_size(argc, argv, envp);
	envc   = count_parms(envp);

	if (kernel)
		thr->kstksize = KSTK_SIZE + parsz;
	else
		thr->kstksize = KSTK_SIZE;

	/* allocate kernel stack */

t = system_time();
	va = (char *) memalign(KSTK_ALIGN, thr->kstksize);
t = system_time() - t;
tt[10] += t;
	if (!va) {
		err = B_NO_MEMORY;
		goto error6;
	}
	thr->kstktop = va + thr->kstksize;

	cstaid = 0;
	if (!kernel) {

		/* allocate the user stack */

		va = STK_BASE;
		if ((tmid == curtmr->tmid) && !curtmr->hasmainthread)
			thr->u.ustksize = RNDINTUP(PAGESIZE + UMAINSTK_SIZE + parsz);
		else
			thr->u.ustksize = RNDINTUP(PAGESIZE + USTK_SIZE + parsz);
		strncpy(area_name, "user stack of ", B_OS_NAME_LENGTH);
		strnncat(area_name, name, B_OS_NAME_LENGTH, B_OS_NAME_LENGTH);
		thr->u.ustaid = create_area_gen(area_name, tmid, &va, B_BASE_ADDRESS,
			thr->u.ustksize, ATTR_CLEAR | ATTR_TEXT | ATTR_STACK, PROT_ALL, 0);
		if (thr->u.ustaid < B_NO_ERROR) {
			err = B_NO_MEMORY;
			goto error7;
		}
		thr->u.ustktop = va + thr->u.ustksize;

		/* if the thread is not in the current team, then we must clone
		the stack in the current address space before accessing it */

		if (tmid == curtmr->tmid)
			custktop = thr->u.ustktop;
		else {
			va = STK_BASE;
			cstaid = clone_area_gen("", curtmr->tmid, &va, B_BASE_ADDRESS,
				PROT_KRD | PROT_KWR, thr->u.ustaid);
			if (cstaid < B_NO_ERROR) {
				err = B_NO_MEMORY;
				goto error8;
			}
			custktop = va + thr->u.ustksize;
		}

		/* point to start of argv, envp zone (in user stack) */

		sp = thr->u.ustktop - parsz;
		csp = custktop - parsz;

		/*
		allocate the argv/envp zone. doing it manually ensure
		that we won't die later in clone_parms when touching
		the user stack pages.
		*/

		err = lock_memory(csp, parsz, 0);
		if (err != B_NO_ERROR)
			goto error9;

	} else {

		custktop = NULL;

		/* point to start of argv, envp zone (in kernel stack) */
		sp = thr->kstktop - parsz;
		csp = sp;
	}

	clone_parms(sp, csp, argc, envc, &argv, &envp);

	if (!kernel)
		unlock_memory(csp, parsz, 0);

	/* start filling the thread_rec */

	thr->state = B_THREAD_SUSPENDED;
	thr->lprio = priority;
	thr->pprio = scale_priority(priority);
	thr->kernel = kernel;

	thr->parent = curthr->thid;

	if (!kernel) {
		thr->u.allregs = FALSE;
		thr->u.range_head = (range_group *) malloc(sizeof(range_group));
		if (!thr->u.range_head) {
			err = B_NO_MEMORY;
			goto error9;
		}
		thr->u.range_head->count = 0;
		thr->u.range_head->next = NULL;

		/* inherit debug state, but signal not set up to debug yet */

		thr->u.dbflags = 0;
		if (!curthr->kernel) {
			thr->u.dbflags = curthr->u.dbflags & DEBUG_trap_at_start;
			if (!newteam) {
				thr->u.dbflags = curthr->u.dbflags & (DEBUG_block_on_restore | DEBUG_stop_all_threads);
				if (curthr->u.dbflags & DEBUG_syscall_trace_through_spawns) {
					thr->u.dbflags |= (curthr->u.dbflags & DEBUG_USER_FLAGS_MASK);
				}
			}
		}
		thr->u.from_nub_port = -1;
		thr->u.prof_slots = NULL;
		thr->u.prof_num = 0;
		init_perfmon_info(&thr->u.perfmon_info);
	}

t = system_time();
	if ((thr->msgsem = create_sem(1, "msem")) < B_NO_ERROR) {
		err = B_NO_MORE_SEMS;
		goto error10;
	}
	set_sem_owner(thr->msgsem, B_SYSTEM_TEAM);
	thr->hasmsg = 0;
	strncpy(sem_name, "Death of ", B_OS_NAME_LENGTH);
	strnncat(sem_name, name, B_OS_NAME_LENGTH, B_OS_NAME_LENGTH);
	if ((thr->waitsem = create_sem(0, sem_name)) < B_NO_ERROR) {
		err = B_NO_MORE_SEMS;
		goto error11;
	}
	set_sem_owner(thr->waitsem, B_SYSTEM_TEAM);
	thr->waitcnt = 0;
t = system_time() - t;
tt[3] += t;

	strncpy(thr->name, name, B_OS_NAME_LENGTH-1);
	thr->name[B_OS_NAME_LENGTH-1] = '\0';
	
	/* what fds to inherit/share? it depends on what team the new
	thread belongs to.
	- a new team. the new thread inherits the file descriptors of the
	current team.
	- the current team (but not the system team). the new thread shares
	the fds	of the current team.
	- any other team. the new thread does not inherit/share anything. it
	does not have file descriptors either. this is the case for both
	kernel threads created by spawn_kernel_team() and for debug nub threads,
	none of which use the team file descriptors.
	*/

	if (newteam || ((tmid != B_SYSTEM_TEAM) && (curtmr->tmid == tmid)))
		par_ioctx = get_cur_ioctx();
	else
		par_ioctx = NULL;

t = system_time();
	err = new_ioctx(newteam, newteam, par_ioctx, &thr->ioctx);
t = system_time() - t;
tt[2] += t;
	if (err)
		goto error12;

	thr->kernel_time = 0;
	thr->user_time = 0;
	thr->entry = entry;

t = system_time();
	init_alarm(thr);
t = system_time() - t;
tt[7] += t;

	/* find a thread id. setup_thread_ctx may need it */

t = system_time();
	thid = newthid();
t = system_time() - t;
tt[8] += t;
	if (thid == B_ERROR) {
		err = B_NO_MORE_THREADS; 
		goto error13;
	}
	thr->thid = thid;

	/*
	set up signals: inherit the signal handlers from the current thread if
	the newly created thread runs in the same team. otherwise,
	just inherit signals set to SIG_IGN and SIG_DFL in the current thread, the other
	signals being set to their default values.
	*/
	
t = system_time();
    setup_signals((tmid == 0) || (curtmr->tmid != tmid), thr, curthr);
t = system_time() - t;
tt[1] += t;

	/* set up the context for the thread to start. this is processor
	dependent. */

t = system_time();
	err = setup_thread_ctx(thr, custktop, parsz, argc, argv, envp);
t = system_time() - t;
tt[4] += t;
	if (err != B_OK)
		goto error14;

	/* put thr in thread_tab and insert thread in list */

t = system_time();
	ps = disable_interrupts();
	acquire_spinlock(&thrlock);

	tmr = team_tab[tmidtoslot(tmid)];

	/* inherit the signals that were sent to the whole process group. */

	thr->sigpending = curthr->sigpending & curthr->siggroup;
	thr->siggroup = curthr->siggroup;

	tmr->cnt++;
	if (!kernel)
		tmr->hasmainthread = TRUE;

	thread_tab[thidtoslot(thid)] = thr;

	thr->tmr = tmr;
	thr->nx_thr = NULL;
	thr->pr_thr = tmr->last_thr;
	if (tmr->first_thr == NULL)
		tmr->first_thr = thr;
	if (tmr->last_thr != NULL)
		tmr->last_thr->nx_thr = thr;
	tmr->last_thr = thr;

	release_spinlock(&thrlock);
	restore_interrupts(ps);
t = system_time() - t;
tt[5] += t;

	/* the thread is now known by the system */

	if (cstaid > 0)
		delete_area(cstaid);

	/* notify debugger of new team (if any) and new thread */
	/* TO DO: do we need the team notification? should thread notification
	   be predicated on existence of team debugger? */

	if (newteam)
		db_create_team (tmr);
t = system_time();
	db_create_thread (tmr, thr);
t = system_time() - t;
tt[6] += t;

	return(thid);

error14:
	ps = disable_interrupts();
	acquire_spinlock(&thrlock);
	thread_alloc[thidtoslot(thid)] = FALSE;
	release_spinlock(&thrlock);
	restore_interrupts(ps);

error13:
	free_ioctx(thr->ioctx);
error12:
	delete_sem(thr->waitsem);
error11:
	delete_sem(thr->msgsem);
error10:
	if (!kernel)
		free(thr->u.range_head);
error9:
	if (!kernel && (tmid != curtmr->tmid))
		delete_area(cstaid);
error8:
	if (!kernel)
		delete_area(thr->u.ustaid);
error7:
	free(thr->kstktop - thr->kstksize);
error6:
	free(thr);
error5:
	if (newteam) {
		ps = disable_interrupts();
		acquire_spinlock(&thrlock);

		/* unlink ourselved from the children list */
		if (tmr->sibling_next)
			tmr->sibling_next->sibling_prev = tmr->sibling_prev;
		if (tmr->sibling_prev)
			tmr->sibling_prev->sibling_next = tmr->sibling_next;
		else
			curtmr->children = tmr->sibling_next;

		/* unlink ourselves from the global team list */
		if (tmr->nx_tmr)
			tmr->nx_tmr->pr_tmr = tmr->pr_tmr;
		else
			last_tmr = tmr->pr_tmr;
		if (tmr->pr_tmr)
			tmr->pr_tmr->nx_tmr = tmr->nx_tmr;
		else
			first_tmr = tmr->nx_tmr;

		team_tab[tmidtoslot(tmid)] = NULL;
		release_spinlock(&thrlock);
		restore_interrupts(ps);

		if (tmr->death_sem >= 0)
			delete_sem(tmr->death_sem);
	}
error4:
	if (newteam)
		as_close(tmr->asr);
error3:
	if (newteam) {
		ps = disable_interrupts();
		acquire_spinlock(&thrlock);
		team_alloc[tmidtoslot(tmid)] = FALSE;
		release_spinlock(&thrlock);
		restore_interrupts(ps);
	}
error2:
	if (newteam)
		free(tmr);		
error1:
	return (err);
}

static thread_id
spawn_kernel_dpc(thread_func taddr, void *arg)
{
	thread_rec	*cur_thr;

	cur_thr = get_cur_thr();
	return create(taddr, TRUE, cur_thr->tmr->tmid, cur_thr->lprio, NULL, -1, arg, NULL);
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	bigtime_t		t;
	int				max;
	int				i;
	thread_id		children[32];
	thread_rec		*thr;

	t = system_time();
	max = 1;
    
    for(i=0; i < max; i++) {
		children[i] = spawn_kernel_dpc(&child_thread, NULL);
		if (children[i] < B_NO_ERROR) {
		    dprintf("creating child %d failed\n", i);
		    children[i] = -1;
		}
    }

	t = system_time() - t;
	dprintf("took %Ld us (%Ld us / thread)\n", t, t / max);

	dprintf("thr memset: took %Ld us (%Ld us / thread)\n", tt[0], tt[0] / max);
	dprintf("setup_signal: took %Ld us (%Ld us / thread)\n", tt[1], tt[1] / max);
	dprintf("new_ioctx: took %Ld us (%Ld us / thread)\n", tt[2], tt[2] / max);
	dprintf("sem stuff: took %Ld us (%Ld us / thread)\n", tt[3], tt[3] / max);
	dprintf("setup_thread_ctx: took %Ld us (%Ld us / thread)\n", tt[4], tt[4] / max);
	dprintf("critical section: took %Ld us (%Ld us / thread)\n", tt[5], tt[5] / max);
	dprintf("db_create_thread %Ld us (%Ld us / thread)\n", tt[6], tt[6] / max);
	dprintf("init_alarm %Ld us (%Ld us / thread)\n", tt[7], tt[7] / max);
	dprintf("new_thid %Ld us (%Ld us / thread)\n", tt[8], tt[8] / max);
	dprintf("malloc thr %Ld us (%Ld us / thread)\n", tt[9], tt[9] / max);
	dprintf("memalign %Ld us (%Ld us / thread)\n", tt[10], tt[10] / max);
	dprintf("thread_rec: %d bytes\n", sizeof(thread_rec) - sizeof(user_thread_rec_ext));
    
    for(i=0; i < max; i++) {
		if (resume_thread(children[i]) < B_NO_ERROR)
		    dprintf("gack! childredn[%d] couldn't start?\n", i);
    }
	snooze(5000000.0);
	return B_OK;
}

/* ----------
	read_hook - does nothing, gracefully
----- */
static status_t
read_hook (void* dev, off_t pos, void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}


/* ----------
	write_hook - does nothing, gracefully
----- */
static status_t
write_hook (void* dev, off_t pos, const void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}

/* ----------
	close_hook - does nothing, gracefully
----- */
static status_t
close_hook (void* dev)
{
	return B_OK;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	return B_ERROR;
}
