#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>


#include <perfmon_kernel.h>

#include <perfmon_cpu.h>

#define ddprintf 


status_t pmcpu_open(const char *name, uint32 flags, void **cookie);
status_t pmcpu_free (void *cookie);
status_t pmcpu_close(void *cookie);
status_t pmcpu_control(void *cookie, uint32 msg, void *buf, size_t size);
status_t pmcpu_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t pmcpu_write(void *cookie, off_t pos, const void *buf, size_t *count);



static bool already_opened = FALSE;
static uint32	perfmon_msr0 = 0, perfmon_msr1 = 0; 
static uint32	perfmon_setup_msr0 = 0, perfmon_setup_msr1 = 0;
static uint32	perfmon_counter_msr0 = 0, perfmon_counter_msr1 = 0; 
static bool		use_rdpmc = FALSE;


static const char *pmcpu_name[] = {
	PERFMON_CPU_DEVICE_NAME,
	NULL
};


device_hooks pmcpu_devices = {
	pmcpu_open, 			
	pmcpu_close, 			
	pmcpu_free,			
	pmcpu_control, 		
	pmcpu_read,			
	pmcpu_write,
	NULL,
	NULL			 
};

int32	api_version = B_CUR_DRIVER_API_VERSION;


sem_id open_lock_sem;
const char open_lock_sem_name[] = "be_perfmon_cpu_lock";


static status_t
lock_open(void)
{
	return acquire_sem(open_lock_sem);
}

static status_t
unlock_open(void)
{
	return release_sem(open_lock_sem);
}



status_t 
init_hardware(void)
{
	system_info	info;

	ddprintf("pmcpu driver: %s %s, init_hardware()\n", __DATE__, __TIME__);
	
	if(get_system_info(&info) != B_OK)
		return B_ERROR;
	
	switch(info.cpu_type) 
	{
	case B_CPU_INTEL_PENTIUM_PRO:
	case B_CPU_INTEL_PENTIUM_II_MODEL_3:
	case B_CPU_INTEL_PENTIUM_II_MODEL_5:
	case B_CPU_INTEL_PENTIUM_MMX:
	case B_CPU_INTEL_PENTIUM:
	case B_CPU_INTEL_PENTIUM75:
	case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
	case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
	case B_CPU_CYRIX_6x86MX:
		return B_OK;
		
	default:
		return ENODEV;
	}	
}


status_t
init_driver(void)
{
	system_info	info;

	ddprintf("pmcpu driver: %s %s, init_driver()\n", __DATE__, __TIME__);
	
	if((open_lock_sem = create_sem(1, open_lock_sem_name)) < B_NO_ERROR)
		return B_ERROR;
	
	if(get_system_info(&info) != B_OK)
		return B_ERROR;
	
	switch(info.cpu_type) 
	{
	case B_CPU_INTEL_PENTIUM_PRO:
	case B_CPU_INTEL_PENTIUM_II_MODEL_3:
	case B_CPU_INTEL_PENTIUM_II_MODEL_5:
		perfmon_setup_msr0 = 0x186;
		perfmon_setup_msr1 = 0x187;
		use_rdpmc = TRUE;
		break;

	case B_CPU_INTEL_PENTIUM_MMX:
	case B_CPU_INTEL_PENTIUM:
	case B_CPU_INTEL_PENTIUM75:
	case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
	case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
	case B_CPU_CYRIX_6x86MX:
		perfmon_setup_msr0 = 0x11;
		perfmon_setup_msr1 = 0;
		perfmon_counter_msr0 = 0x12;
		perfmon_counter_msr1= 0x13;
		use_rdpmc = FALSE;
		break;

	default:	/* taken care of in hardware_init() */
		dprintf("pmcpu driver: somebody installed a new CPU type 0x%x in the system without rebooting it!\n", info.cpu_type);
		return B_ERROR;
	}		

	return B_OK;
}


void
uninit_driver(void)
{
	ddprintf("pmcpu driver: uninit_driver()\n"); 
	delete_sem(open_lock_sem);
}


const char**
publish_devices()
{
	ddprintf("pmcpu driver: publish_devices()\n"); 
	return pmcpu_name;
}


device_hooks*
find_device(const char* name)
{
	ddprintf("pmcpu driver: find_device()\n"); 
	return &pmcpu_devices;
}


status_t
pmcpu_open(const char *dname, uint32 flags, void **cookie)
{
	status_t	ret = B_OK;

	ddprintf("pmcpu driver: open(%s)\n", dname); 
	
	lock_open();
	
	if(already_opened)
		ret = B_ERROR;
	else
		already_opened = TRUE;
	
	unlock_open();
	
    return ret;
}
 
 
status_t
pmcpu_free (void *cookie)
{
	ddprintf("pmcpu driver: free()\n"); 	

	already_opened = FALSE;

	return B_NO_ERROR;
}


status_t
pmcpu_close(void *cookie)
{
	ddprintf("pmcpu driver: close()\n"); 
			
	return B_NO_ERROR;
}




status_t
pmcpu_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	perfmon_cpu_counters* counters = (perfmon_cpu_counters*) buf;

	
	if((*count) < sizeof(perfmon_cpu_counters))
	{
		*count = 0;
		return B_ERROR;
	}

	if(use_rdpmc)
	{
		counters->counter0 = read_pmc(0);
		counters->counter1 = read_pmc(1); 
	}
	else
	{
		counters->counter0 = read_msr(perfmon_counter_msr0);
		counters->counter1 = read_msr(perfmon_counter_msr1); 
	}

	*count = sizeof(perfmon_cpu_counters);
	
	return B_OK;
}


static bool 
is_valid_setup_perfmon_msr(uint32 msr)
{
	return ((msr == 0) || (msr == perfmon_setup_msr0) || (msr == perfmon_setup_msr1) );
}


status_t
pmcpu_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	const perfmon_cpu_control* control = (const perfmon_cpu_control*) buf;
		
	if((*count) != sizeof(perfmon_cpu_control))
	{
		*count = 0;
		return B_ERROR;
	}	

	/* allow only perfmon MSRs */
	if(!(is_valid_setup_perfmon_msr(control->reg0) && is_valid_setup_perfmon_msr(control->reg0)) )	
	{
		*count = 0;
		return B_ERROR;
	}
		

	if(control->reg0 != 0)
		write_msr(control->reg0, control->reg0_val);
	
	if(control->reg1 != 0)
		write_msr(control->reg1, control->reg1_val);
	
	*count = sizeof(perfmon_cpu_control);
	
	return B_OK;
}


status_t
pmcpu_control (void *cookie, uint32 ioctl, void *arg1, size_t len)
{
	return B_ERROR;
}





