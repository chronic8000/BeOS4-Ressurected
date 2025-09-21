
#include <OS.h>
#include <KernelExport.h>
#include "dpc.h" 

/* internal types for Global Events / DPCs */

typedef struct
{
	void*				data; 
	dpc_handler*		callback;
} dpc_entry;

#define	MAX_NUM_DPCS	32

typedef struct 
{
	dpc_entry		dpcs[MAX_NUM_DPCS]; 
	thread_id		work_thread;
	sem_id			wakeup_sem;	
	spinlock		lock_sl;
	cpu_status		lock_is;
	bool			quit;
} dpc_table;


/*****************************************************************/
/* Global Events / DPCs implementation                           */
/*****************************************************************/

static void 
lock_dpc_table(dpc_table* dtable)
{
	cpu_status ps = disable_interrupts();
	acquire_spinlock(&dtable->lock_sl);
	dtable->lock_is = ps;
}

static void 
unlock_dpc_table(dpc_table* dtable)
{
	cpu_status ps = dtable->lock_is;
	release_spinlock(&dtable->lock_sl);
	restore_interrupts(ps);
}

static void
main_loop(dpc_table*	dtable)
{
	int		i;
	for(i=0; i<MAX_NUM_DPCS; i++)
	{
		dpc_entry* dpc = &dtable->dpcs[i];	

		if(dpc->callback != NULL)
		{
			dpc_entry	dpc_copy;	
			/* 
			 Atomically copy dpc entry. Callback can take too much time to keep dpc_table locked
			 with disabled interrupts. Disabling interrups is requered because dpc_table can be
			 modified by Schedule_Global_Event() inside or outside of an ISR. 
			 It is possible to lock only individual dpc_entries and not the whole dpc_table but
			 it willl require putting spinlock and cpu_status in each dpc_entry.
			 */
			lock_dpc_table(dtable);
				dpc_copy = *dpc;
				dpc->callback	= NULL;	/* small lie from Cancel_Global_Event()(unimlemented for now) point of view */
				dpc->data 		= NULL; /* only for debug */
			unlock_dpc_table(dtable);
			
			if(dpc_copy.callback != NULL) /* Check callback again. It may be changed by Cancel_Global_Event() */ 
				dpc_copy.callback(dpc_copy.data);
		}
	} 	
}


/* ATTENSION: this thread *may* run with very high real-time priority */
static int32 
dpc_thread_func(void * data)
{
	dpc_table*	dtable = (dpc_table*)data;
		
	while(!dtable->quit)
	{
		main_loop(dtable);
		acquire_sem(dtable->wakeup_sem);
	}

	main_loop(dtable);

	return B_OK;
}

static status_t
create_dpc_thread( dpc_thread_handle* dpct_handle,  const char* name, int32 priority)
{
	char		name_buf[32];
	dpc_table*	dtable;
	status_t	ret = B_OK;

	dtable = (dpc_table*)calloc(1, sizeof(dpc_table));
	if(dtable == NULL)
	{
		ret = B_NO_MEMORY;
		goto err0;
	}

	dtable->quit = FALSE;
	dtable->lock_sl = 0;
		
	sprintf(name_buf, "%.16s_wakeup_sem", name);
	if ((dtable->wakeup_sem = create_sem(0, name_buf)) < B_OK)
	{ 
		ret = dtable->wakeup_sem;	
		goto err1;
	}
	set_sem_owner(dtable->wakeup_sem, B_SYSTEM_TEAM);
	
	dtable->work_thread = spawn_kernel_thread(dpc_thread_func, name, priority, dtable);
	if(dtable->work_thread < B_OK)
	{
		ret = dtable->work_thread;
		goto err2; 
	}
		
	resume_thread(dtable->work_thread); /* FIXME error handling */
	
	/* copy the valid created handle back to the caller */
	*dpct_handle = dtable;  
	
	return ret;	
	
err2:
	delete_sem(dtable->wakeup_sem);
err1:
	free(dtable);
err0:
	return ret;
}



static status_t
delete_dpc_thread( dpc_thread_handle dpct_handle)
{
	dpc_table* dtable  = (dpc_table*)dpct_handle;  
	status_t		unused_exit_status;
	
	if(dtable == NULL)
		return B_OK;
		
	dtable->quit = TRUE;
	/* wakeup work thread to tell it that it's time to quit */
	release_sem(dtable->wakeup_sem);
	/* wait it to die */
	wait_for_thread(dtable->work_thread, &unused_exit_status);
	
	/* it's now safe to release the resources */
	delete_sem(dtable->wakeup_sem);
	free(dtable);
	
	return B_OK;
}


/* async API !!! -- usually but not always is called from an ISR */
static dpc_handle 
queue_dpc(dpc_thread_handle dpct_handle, dpc_handler* proc, void* cookie)
{
	int			i;
	dpc_entry*	dpc; 
	dpc_table*	dtable = (dpc_table*)dpct_handle; 

	lock_dpc_table(dtable);
	for(i=0; i<MAX_NUM_DPCS; i++)
	{
		dpc = &dtable->dpcs[i];	
		
		if(dpc->callback == NULL)
		{
			dpc->callback		= proc;
			dpc->data			= cookie;
			break;
		}
	} 	
	unlock_dpc_table(dtable);

	if(i != MAX_NUM_DPCS)
	{
		/* wakeup work thread to tell that it has job to do */
		release_sem_etc(dtable->wakeup_sem, 1, B_DO_NOT_RESCHEDULE);
		return dpc;
	}
	else
	{
#ifdef DEBUG
		dprintf("Can't queue_dpc %08x\n", proc);
#endif
		return NULL;
	}
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return B_OK;
	case B_MODULE_UNINIT:
		return B_OK;
	default:
		return -1;
	}
}
static dpc_module_info dpc_module = 
{
	{
		B_DPC_MODULE_NAME,
		0,
		std_ops
	},
	create_dpc_thread,
	delete_dpc_thread,
	queue_dpc
};

_EXPORT dpc_module_info *modules[] =
{
	&dpc_module,
	NULL
};

