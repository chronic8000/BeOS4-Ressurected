#include <OS.h>
#include "benaphore.h"

status_t create_ben( benaphore_t *ben, const char *name )
{
	if( (ben->sem = create_sem( 0, name )) < 0 )
		return ben->sem;
	ben->atom = 0;
	return B_OK;
}

void delete_ben( benaphore_t *ben )
{
	delete_sem( ben->sem );
}

status_t lock_ben( benaphore_t *ben )
{
	if( atomic_add( &ben->atom, 1 ) > 0 )
	{
		status_t status;
		
		if( (status = acquire_sem( ben->sem )) < 0 )
			unlock_ben( ben );
		return status;
	}
	else
		return B_OK;
}

status_t unlock_ben( benaphore_t *ben )
{
	if( atomic_add( &ben->atom, -1 ) > 1 )
		return release_sem_etc( ben->sem, 1, B_DO_NOT_RESCHEDULE );
	return B_OK;
}
