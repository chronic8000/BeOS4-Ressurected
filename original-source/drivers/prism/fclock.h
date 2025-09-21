#ifndef __fclock__
#define __fclock__
typedef struct fc_lock
{
	int32			count;
	spinlock		slock;
	int32			waiting;
	sem_id			sem;
} fc_lock;

status_t create_fc_lock( struct fc_lock *fc, int32 count, const char *name );
void delete_fc_lock( struct fc_lock *fc );
// Note: fc_wait() is not safe if can be called by multiple threads;
// Use a benaphore lock in this case.
status_t fc_wait( struct fc_lock *fc, bigtime_t timeout );
bool fc_signal( struct fc_lock *fc, int32 count, int32 sem_flags );

#endif /* __fclock__ */
