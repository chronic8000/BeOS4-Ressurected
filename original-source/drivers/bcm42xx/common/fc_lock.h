#ifndef _FC_LOCK_H
#define _FC_LOCK_H
// ***
// Flow Control Lock
// ***

#include <OS.h>


typedef struct fc_lock
{
	int32			count;
	spinlock		slock;
	int32			waiting;
	sem_id			sem;
} fc_lock;

status_t create_fc_lock( struct fc_lock *fc, int32 count, const char *name );
void delete_fc_lock( struct fc_lock *fc );

bool fc_signal( struct fc_lock *fc, int32 count, int32 sem_flags );
status_t fc_wait( struct fc_lock *fc, bigtime_t timeout );
// Note: fc_wait() is not safe if can be called by multiple threads;
// Use a benaphore lock in this case.
#endif
