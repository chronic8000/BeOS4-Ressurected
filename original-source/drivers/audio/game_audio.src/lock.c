#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include "lock.h"


static cpu_status former;
static spinlock lockvar = 0;
bool islocked = false;

void lock(void)
{
	//dprintf("lock\n");
	former = disable_interrupts();
	acquire_spinlock(&lockvar);
	islocked = true;
}

void unlock(void)
{
	islocked = false;
	release_spinlock(&lockvar);
	restore_interrupts(former);
	//dprintf("unlock\n");
}

bool is_locked(void)
{
	return islocked;
}
