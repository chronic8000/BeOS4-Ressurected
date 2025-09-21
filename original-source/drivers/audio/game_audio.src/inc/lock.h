#ifndef __LOCK_H__
#define __LOCK_H__

#include "wrapper.h"

void lock(void);
void unlock(void);
bool is_locked(void);

#endif