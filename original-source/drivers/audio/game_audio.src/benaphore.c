#include <KernelExport.h>
//#include "R3MediaDefs.h"
//#include "gaudio.h"

long   gaudio_benaphore_atom;
sem_id gaudio_benaphore_sem;


long init_benaphore()
{
    gaudio_benaphore_atom= 0;
    gaudio_benaphore_sem = create_sem(0,"gaudio_benaphore");
    if (gaudio_benaphore_sem < B_NO_ERROR)
        return B_ERROR;
    else
        return B_NO_ERROR;
}

void acquire_benaphore()
{
   long previous_value;
   previous_value = atomic_add (&gaudio_benaphore_atom, 1);
   if (previous_value > 0) 
      acquire_sem(gaudio_benaphore_sem);
}
 
void release_benaphore()
{
   long previous_value;
   previous_value = atomic_add (&gaudio_benaphore_atom, -1);
   if (previous_value > 1) 
     release_sem(gaudio_benaphore_sem);
}	

void delete_benaphore()
{
	delete_sem(gaudio_benaphore_sem);			
}	