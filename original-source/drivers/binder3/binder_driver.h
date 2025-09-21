
#ifndef BINDER2_DRIVER_INTERNAL_H
#define BINDER2_DRIVER_INTERNAL_H

#include "binder_defs.h"

status_t init_binder();
void uninit_binder();

void remove_team(int32 id);
binder_team *get_team(int32 teamID);
binder_team *get_team(int32 teamID, int32 thid, binder_thread *parent);

void set_root(binder_node *n);
binder_node *get_root();

void *shared_malloc(int32 size);
void *shared_realloc(void *p, int32 size);
void shared_free(void *p);

#endif // BINDER2_DRIVER_INTERNAL_H
