
#include <OS.h>
#include <KernelExport.h>
#include "fclock.h"

status_t create_fc_lock( struct fc_lock *fc, int32 count, const char *name )
{
	if( (fc->sem = create_sem( 0, name )) < 0 )
		return fc->sem;
	set_sem_owner( fc->sem, B_SYSTEM_TEAM );
	fc->count = count;
	fc->slock = 0;
	fc->waiting = 0;
	return B_OK;
}

void delete_fc_lock( struct fc_lock *fc )
{
	delete_sem( fc->sem );
}

status_t fc_wait( struct fc_lock *fc, bigtime_t timeout )
{
	cpu_status	cpu;
	
	// Lock while performing test & set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	
	// Test flow control condition and block if required 
	while( fc->count <= 0 )
	{
		status_t 	status;
		
		// Set waiting flag and release lock
		fc->waiting = 1;
		release_spinlock( &fc->slock );
		restore_interrupts( cpu );
		
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
	}
	fc->count--; // Decrement count
	
	// Release lock and return B_OK because condition has been met
	release_spinlock( &fc->slock );
	restore_interrupts( cpu );
	return B_OK;
}

bool fc_signal( struct fc_lock *fc, int32 count, int32 sem_flags )
{
	cpu_status	cpu;
	bool		sig;
	
	// Lock while performing test and set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	
	fc->count += count; // Increment count
	
	// If someone is waiting and the condion is met, clear waiting flag and set sig flag
	if( fc->waiting )
	{
		fc->waiting = 0;
		sig = true;
	}
	else
		sig = false;
	
	release_spinlock( &fc->slock );
	restore_interrupts( cpu );
	
	// We do the actual signaling outside of the lock
	if( sig )
		release_sem_etc( fc->sem, 1, sem_flags );
		
	return sig;
}
