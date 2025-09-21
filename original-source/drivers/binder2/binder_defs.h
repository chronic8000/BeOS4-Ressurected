
#ifndef BINDER2_DEFS_H
#define BINDER2_DEFS_H

#define HASH_SIZE 256
#define HEAP_SIZE (4 * 1024 * 1024)

#include <support2/SupportDefs.h>
#include <support2/atomic.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <binder2_driver.h>
#include <OS.h>

extern "C" {

#if BINDER_DEBUG_LIB

struct lock {
	sem_id		s;
	long		c;
};

inline int new_lock(lock *l, const char *name)
{
	l->s = create_sem(0,name);
	l->c = 1;
	return 0;
}

inline int free_lock(lock *l)
{
	delete_sem(l->s);
	return 0;
}

#define	LOCK(l)		{ if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s); }
#define	UNLOCK(l)	{ if (l.c > 0) debugger("bad"); else if (atomic_add(&l.c, 1) < 0) release_sem(l.s); }

#define smalloc malloc
#define sfree free
#define ddprintf(a) printf a ; fflush(stdout)

inline int add_debugger_command(char *, int (*)(int , char **), char *) { return 0; };
inline int remove_debugger_command(char *, int (*)(int , char **)) { return 0; };

#define spawn_spawner(a,b) spawn_thread(a,"binder looper spawner",B_NORMAL_PRIORITY,b)
#define uspace_spawn(a,b,c,d) spawn_thread(a,b,c,d)

#define ASSERT(a) if (!(a)) debugger("assertion failed: " #a );
#define UHOH(a) debugger(a);

#else

#warning not in lib mode
#define ddprintf(a) dprintf a

#include "lock.h"
#include "kalloc.h"

#define spawn_spawner(a,b) spawn_kernel_dpc(a,b)
#define uspace_spawn(a,b,c,d) user_spawn_user_thread(a,b,c,d)

#define ASSERT(a)
#define UHOH(a)

#endif

#include <stdio.h>
#define _MALLOC_INTERNAL
#include <malloc.h>

}

#define DEBUG_LEVEL 0
#define DPRINTF(level,a) if (level <= DEBUG_LEVEL) { ddprintf(a); }

class iobuffer;
class binder_node;
class binder_team;
class binder_thread;
class binder_transaction;

enum {
	PRIMARY = 1,
	SECONDARY = 2
};

inline void * operator new(size_t size) {
	return smalloc(size);
}

inline void operator delete(void *p) {
	sfree(p);
}

#endif // BINDER2_DEFS_H
