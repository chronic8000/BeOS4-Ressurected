#include <KernelExport.h>
#include <pcmcia/cs_timer.h>

static struct timer_list timer_root = {
    &timer_root, &timer_root, 0, 0, NULL
};

static spinlock list_lock = 0;
static sem_id timer_sem;
static thread_id thread;

static inline cpu_status lock(void)
{
    cpu_status flags = disable_interrupts();
    acquire_spinlock(&list_lock);
    return flags;
}

static inline void unlock(cpu_status flags)
{
    release_spinlock(&list_lock);
    restore_interrupts(flags);
}

static int32 timer_thread(void *data)
{
	struct timer_list *p;
	bigtime_t now, next;
	cpu_status flags;
	status_t err;

	do {
		do {
			now = system_time();
			next = now + 60*1000000;
			flags = lock();
			for (p = timer_root.next; p != &timer_root; p = p->next) {
				if (p->expires <= now) break;
				if (p->expires < next)
					next = p->expires;
			}
			if (p != &timer_root) {
				p->prev->next = p->next;
				p->next->prev = p->prev;
				p->prev = p->next = NULL;
				unlock(flags);
				p->function(p->data);
			} else {
				unlock(flags);
			}
		} while (p != &timer_root);
		err = acquire_sem_etc(timer_sem, 1, B_TIMEOUT, next-now);
	} while(err == B_NO_ERROR || err == B_TIMED_OUT);
    return B_OK;
}

void my_add_timer(struct timer_list *t)
{
    u_long flags = lock();
    t->prev = timer_root.prev;
    t->next = &timer_root;
    timer_root.prev->next = t;
    timer_root.prev = t;
    unlock(flags);
    /* Be lazy about scheduling the timer thread */
    if (timer_sem > 0)
	release_sem_etc(timer_sem, 1, B_DO_NOT_RESCHEDULE);
}

void my_del_timer(struct timer_list *t)
{
    u_long flags = lock();
    if (t->prev && t->next) {
	t->prev->next = t->next;
	t->next->prev = t->prev;
	t->prev = t->next = NULL;
    }
    unlock(flags);
}

void init_timer(void)
{
    timer_sem = create_sem(0, "cs_timer");
    if (timer_sem > 0) {
	set_sem_owner(timer_sem, B_SYSTEM_TEAM);
	thread = spawn_kernel_thread(timer_thread, "cs_timer",
				     B_NORMAL_PRIORITY, NULL);
        if (thread > 0) resume_thread(thread);
    }
}

void stop_timer(void)
{
	status_t dummy;

	if (timer_sem > 0) {
		delete_sem(timer_sem);
	}
	if (thread > 0)
		wait_for_thread(thread, &dummy);
}
