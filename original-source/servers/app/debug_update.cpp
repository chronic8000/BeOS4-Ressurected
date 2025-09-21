//******************************************************************************
//
//	File:		debug_update.c
//
//	Description:	Gather debug information about the update mechanism.
//	
//	Written by:	Pierre Raynaud-Richard
//
//	Copyright 1998, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/
   
#include <OS.h>
#ifndef	_STDLIB_H
#include <stdlib.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif

#include "as_debug.h"

#ifndef _DEBUG_UPDATE_H
#include "debug_update.h"
#endif

typedef struct {
	TWindow		*window;
	bool		enabled;
	bool		enable_ref;
	uint8		update_status;
	bigtime_t	first;
	bigtime_t	ref;
	int32		count[DUP_LEVEL_COUNT];
	bigtime_t	total[DUP_LEVEL_COUNT];
} dup_entry;

static	int32		list_count, list_max;
static	int32		dup_locker;
static	sem_id		dup_sem;
static	dup_entry	*list;
static	thread_id	dup_dumper;

void dup_lock() {
	if (atomic_add(&dup_locker, 1) > 0)
		acquire_sem(dup_sem);
}

void dup_unlock() {
	if (atomic_add(&dup_locker, -1) > 1)
		release_sem(dup_sem);
}

int32 dup_dumperR(void *data) {
	bool		do_it;
	int32		i, j;
	TWindow		*w;

	while (true) {
		snooze(DUP_DELAY);
		
		dup_lock();
		
		do_it = false;
		for (i=0; i<list_count; i++) {
			w = list[i].window;
			for (j=1; j<DUP_LEVEL_COUNT; j++)
				if (list[i].count[j] != 0) {
					do_it = true;
					break;
				}
		}

		if (do_it) {
			xprintf("####################################\n");
			for (i=0; i<list_count; i++) {
				w = list[i].window;
				xprintf("%32s ->", w->get_name());
				for (j=1; j<DUP_LEVEL_COUNT; j++) {
					if (list[i].count[j] == 0)
						xprintf(" #####");
					else
						xprintf(" %5d", ((int32)list[i].total[j]/((int32)list[i].count[j]*100)));
					list[i].total[j] = 0LL;
					list[i].count[j] = 0;
				}
				xprintf("\n");
				list[i].enabled = false;
				list[i].enable_ref = false;
				list[i].update_status = 0;
			}
		}
	
		dup_unlock();
	}
}

static int32 window_index(TWindow *w) {
	int32		i;
	
	for (i=0; i<list_count; i++)
		if (list[i].window == w)
			return i;
	return -1;
}

void dup_initR() {
	int     i, j;

	xprintf("Debug Update Init called ###################################\n");
	dup_sem = create_sem(0, "dup sem");
	dup_locker = 0;
	
	list_max = 32;
	list_count = 0;
	list = (dup_entry*)dbMalloc(sizeof(dup_entry) * list_max);
	for (i=0; i<list_max; i++) {
		list[i].enabled = false;
		list[i].enable_ref = false;
		list[i].update_status = 0;
		for (j=1; j<DUP_LEVEL_COUNT; j++) {
			list[i].total[j] = 0LL;
			list[i].count[j] = 0;
		}
	}	
	
	dup_dumper = spawn_thread(dup_dumperR, "dup dumper", B_URGENT_DISPLAY_PRIORITY, 0L);
	resume_thread(dup_dumper);
}

void dup_endR() {
	dup_lock();
	dbFree(list);
	kill_thread(dup_dumper);
	delete_sem(dup_sem);
}

void dup_create_windowR(TWindow *w) {
	int32		i, j;

	dup_lock();
	
	if (list_count == list_max) {
		list_max += 32;
		list = (dup_entry*)dbRealloc(list, sizeof(dup_entry) * list_max);
		for (i=list_max-32; i<list_max; i++) {
			list[i].enabled = false;	
			list[i].enable_ref = false;	
			list[i].update_status = 0;
			for (j=1; j<DUP_LEVEL_COUNT; j++) {
				list[i].total[j] = 0LL;
				list[i].count[j] = 0;
			}
		}
	}
	list[list_count].window = w;
	for (j=1; j<DUP_LEVEL_COUNT; j++) {
		list[list_count].count[j] = 0;
		list[list_count].total[j] = 0;
	}
	list_count++;

	dup_unlock();
}

void dup_release_windowR(TWindow *w) {
	int32		index;

	dup_lock();
	index = window_index(w);
	if (index == -1) {
		xprintf("### ERROR ### -> Try to release an unknown window: %x\n", w);	
		xprintf("named %s\n", w->get_name());
	}
	else {
		if (index != (list_count-1))
			memcpy(list+index, list+(list_count-1), sizeof(dup_entry));
		list_count--;
	}	
	dup_unlock();
}

void dup_tag_windowR(TWindow *w, int32 level) {
	int32		index;
	bigtime_t	time;
	dup_entry	*e;

	dup_lock();
	index = window_index(w);
	if (index == -1) {
		xprintf("### ERROR ### -> Try to tag (%d) an unknown window %x\n", level, w);
		xprintf("named %s\n", w->get_name());
	}		
	else {
		e = list+index;
		if ((!e->enabled) && (level == DUP_SUBMIT_UPDATE)) {
			e->enabled = true;
			e->first = 0LL;
		}
		if (e->enabled) {
			time = system_time();
			if (level == DUP_SUBMIT_UPDATE) {
				if (e->first == 0LL) {
					e->first = time;
					e->update_status = 0;
				}
			}
			else {
				if (level == DUP_TRIGGER_UPDATE) {
					if (e->first != 0LL) {
						e->ref = e->first;
						e->enable_ref = e->enabled;
					}
					e->first = 0LL;
				}
				if (e->enable_ref) {
					e->total[level] += (time - e->ref);
					e->count[level]++;
				}
			}	
		}
	}	
	dup_unlock();
}

void dup_first_tag_windowR(TWindow *w, int32 status) {
	uint8		status_mask;
	int32		index;
	bigtime_t	time;
	dup_entry	*e;

	dup_lock();
	index = window_index(w);
	if (index == -1) {
		xprintf("### ERROR ### -> Try to first tag (%d) an unknown window %x\n", status, w);
		xprintf("named %s\n", w->get_name());
	}		
	else {
		e = list+index;
		if (e->enabled) {
			time = system_time();
			status_mask = (~e->update_status)&status;
			e->update_status |= status;
			if (status_mask & 2) {
				e->total[DUP_NOTIFY_DONE_FLAG] += (time - e->first);
				e->count[DUP_NOTIFY_DONE_FLAG]++;
			}
			if (status_mask & 1) {
				e->total[DUP_DELAY_UPDATE] += (time - e->first);
				e->count[DUP_DELAY_UPDATE]++;
			}
		}
	}	
	dup_unlock();
}

void dup_count_tag_windowR(TWindow *w, int32 level, int32 count) {
	int32		index;
	dup_entry	*e;

	dup_lock();
	index = window_index(w);
	if (index == -1) {
		xprintf("### ERROR ### -> Try to count tag (%d) an unknown window %x\n", level, w);
		xprintf("named %s\n", w->get_name());
	}		
	else {
		e = list+index;
		if (e->enabled) {
			e->total[level] += count*100;
			e->count[level]++;
		}
	}	
	dup_unlock();
}

void dup_counter_windowR(TWindow *w, int32 level) {
	int32		index;
	dup_entry	*e;

	dup_lock();
	index = window_index(w);
	if (index == -1) {
		xprintf("### ERROR ### -> Try to count tag (%d) an unknown window %x\n", level, w);
		xprintf("named %s\n", w->get_name());
	}		
	else {
		e = list+index;
		if (e->enabled) {
			e->total[level] += 100;
			e->count[level] = 1;
		}
	}	
	dup_unlock();
}
