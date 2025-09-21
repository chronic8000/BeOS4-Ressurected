// ***
// Flow Control Lock
// ***

typedef struct fc_lock_t
{
	vint32			count;
	spinlock		slock;
	cpu_status		cpu;
	int32			waiting;
	sem_id			sem;
} fc_lock_t;

static  status_t create_fc_lock( fc_lock_t *fc, int32 count, const char *name );
static  void delete_fc_lock( fc_lock_t *fc );

static  status_t fc_wait( fc_lock_t *fc, bigtime_t timeout , int32 decrement_count );
static  void fc_unlock( fc_lock_t *fc );

static  void fc_lock( fc_lock_t *fc );
static  bool fc_signal( fc_lock_t *fc, int32 count, int32 sem_flags );


static status_t create_fc_lock( fc_lock_t *fc, int32 count, const char *name )
{
	if( (fc->sem = create_sem( 0, name )) < 0 )
		return fc->sem;
	set_sem_owner( fc->sem, B_SYSTEM_TEAM );
	fc->count = count;
	fc->slock = 0;
	fc->waiting = 0;
	fc->cpu = 0;
	return B_OK;
}

static void delete_fc_lock( fc_lock_t *fc )
{
	delete_sem( fc->sem );
}

static status_t fc_wait( fc_lock_t *fc, bigtime_t timeout, int32 decrement_count )
{
	cpu_status	cpu;
	
	// Lock while performing test & set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	fc->cpu = cpu;
	
	// Test flow control condition and block if required 
	while( fc->count <= 0 )
	{
		status_t 	status;
		
		// Set waiting flag and release lock
		fc->waiting = 1;
		release_spinlock( &fc->slock );
		restore_interrupts( fc->cpu );
		
		// Wait for signal
		if( (status = acquire_sem_etc( fc->sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, timeout )) != B_OK )
		{
			// Clear bits
			fc->waiting = 0;
			return status;
		}
		
		// Lock and test again
		cpu = disable_interrupts();
		acquire_spinlock( &fc->slock );
		fc->cpu = cpu;
	}
	fc->count -= decrement_count; // Decrement count
	
	return B_OK;
}

static bool fc_signal( fc_lock_t *fc, int32 count, int32 sem_flags )
{
	int32		waiting;
	
	fc->count += count; // Increment count
	
	waiting = fc->waiting;
	fc->waiting = 0;
	
	release_spinlock( &fc->slock );
	restore_interrupts( fc->cpu );
	
	// We do the actual signaling outside of the lock
	if( waiting )
		release_sem_etc( fc->sem, waiting, sem_flags );
		
	return (waiting != 0);
}

static void fc_unlock( fc_lock_t *fc )
{
	// Release lock and return B_OK because condition has been met
	release_spinlock( &fc->slock );
	restore_interrupts( fc->cpu );
}

static void fc_lock( fc_lock_t *fc )
{
	cpu_status	cpu;
	
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	fc->cpu = cpu;
}
