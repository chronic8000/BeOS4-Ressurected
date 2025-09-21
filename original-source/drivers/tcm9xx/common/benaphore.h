#ifndef BENAPHORE_H
#define BENAPHORE_H

typedef struct benaphore
{
	sem_id		sem;
	int32		atom;
} benaphore_t;

status_t create_ben( benaphore_t *ben, const char *name );
void delete_ben( benaphore_t *ben );
status_t lock_ben( benaphore_t *ben );
status_t unlock_ben( benaphore_t *ben );

#endif
