/* ========================================================================== */
#include "mydefs.h"
#include <PCI.h>
#include <Drivers.h>
#include "tmman_drv/tmif.h"
/* ========================================================================== */
extern int32 tm_interrupt_handler(void *dev_id);
/* ========================================================================== */
static status_t
tm_open (const char *name, uint32 flags, void **c)
{
	/* variables */
	ioparms dIoParms;
	
	/* unused parameters */
	(void)flags;
	(void)c;

	/* call tmmOpen */
	PRINTF("###########################tm_open (name=%s)\n", name);
	tmmOpen(&dIoParms);
	PRINTF("###########################tm_open(return) (name=%s)\n", name);
	
	
	return 0;
}
/* ========================================================================== */
static status_t
tm_close (void *d)
{
	/* unused parameters */
	(void)d;
	
	PRINTF("###########################tm_close(do_nothing)\n");

	return 0;
}
/* ========================================================================== */
static status_t
tm_free (void *d)
{
	/* unused parameters */
	(void)d;
	
	PRINTF("###########################tm_free(do_nothing)\n");
	
	return 0;
}
/* ========================================================================== */
static status_t
tm_control(void *cookie, uint32 op, void *data, size_t len)
{
	/* variable */
	tTMMANIo                *pIoParmBlock;
	
	
	/* unused parameters */
	(void)cookie;
	(void)len;
	
	PRINTF("###########################tm_control\n");
	
	switch (op)
	{
		case 488:
			{
				PRINTF("ioctl code of upper libraries (not Be specific)\n");
			}
			break;
		case 489:
			{
				tHalObject* pHal;
				
				/* get pointer on the hal object */
				pHal    = (tHalObject*)( (TMManDeviceObject *)(TMManGlobal->DeviceList[0]) )->HalHandle;

				/* under BeOS/BeIA, user application can see kernel mapped memory */
				*(void **)data	= pHal->pSDRAMAddrKernel;
				
				/* display trace message and return */				
				PRINTF("###########################tm_control(setting SDRAM)\n");
				return 0;
			}
			break;
		case 490:
			{
				tHalObject* pHal;
				
				/* get pointer on the hal object */
				pHal    = (tHalObject*)( (TMManDeviceObject *)(TMManGlobal->DeviceList[0]) )->HalHandle;

				/* under BeOS/BeIA, user application can see kernel mapped memory */
				*(void **)data	= pHal->pMMIOAddrKernel;
				
				/* display trace message and return */				
				PRINTF("###########################tm_control(setting MMIO)\n");
				return 0;
			}
			break;
		default:
			{
				PRINTF("###########################tm_control(do_nothing, invalid op ioctl)\n");
				return 0;
			}
			break;
		
	}
	
	/* make the ioctl */
	tmmCntrl((ioparms *)data);
	
	/* check if something additionnal need to be done */
	pIoParmBlock = (tTMMANIo *) ( ((ioparms *)data)->in_iopb );
	if (pIoParmBlock->controlCode == constIOCTLtmmanSharedMemoryCreate)
	{ 
		PRINTF("tm_main.c: tm_ioctl: case tmifSharedMemoryCreate.\n");
		PRINTF("WARNING: additionnal things need to be done\n");
	}
	
	/* display return value of ioctl execution */
	PRINTF("tm_main.c: tm_ioctl: tm_ioctl returned with ((ioparms *)data)->err is 0x%lx\n", ((ioparms *)data)->err);	
	
	PRINTF("###########################tm_control(return)\n");
	
	
	return 0;
}
/* ========================================================================== */
static status_t
tm_read (void *d_var, off_t pos, void *buf, size_t *len)
{
	/* unused parameters */
	(void)d_var;
	(void)pos;
	(void)buf;
	(void)len;

	return B_ERROR;	
}
/* ========================================================================== */
static status_t
tm_write (void *d_var, off_t pos, const void *buf, size_t *len)
{
	/* unused parameters */
	(void)d_var;
	(void)pos;
	(void)buf;
	(void)len;
	
	return B_ERROR;
}
/* ========================================================================== */
static device_hooks tm_device =
{
	tm_open,		/* -> open entry 	*/
	tm_close,		/* -> close entry 	*/
	tm_free,		/* -> free cookie 	*/
	tm_control,		/* -> control entry */
	tm_read,		/* -> read entry 	*/
	tm_write,		/* -> write entry 	*/
	NULL,  			/* select 			*/
	NULL,  			/* deselect			*/
	NULL,  			/* readv			*/
	NULL   			/* writev			*/
};
/* ========================================================================== */
/* = init_hardware															= */
/* = 	returns: 	  														= */
/* = 			B_OK	 		1+ card found (only first is used)			= */
/* = 			B_ERROR			no card found								= */
/* = 			other != B_OK	cannot access PCI module					= */
/* ========================================================================== */
_EXPORT
status_t 
init_hardware(void)
{
	status_t return_value = B_OK;
	
	pci_module_info *hmod;
	int trimedia_chip;
	
	PRINTF("###########################init_hardware (003)\n");

	PRINTF("{\n");

	/* get PCI module */	
	{	
		return_value = get_module(B_PCI_MODULE_NAME, (module_info **) &hmod);
		if(return_value != B_OK)
		{
			PRINTF("   get_module error\n");
		}
	}	
	
	/* search TriMedia on PCI bus */
	if (return_value == B_OK)
	{
		pci_info 	info;
		int 		index;
		
		trimedia_chip = 0;
		
		for
		(
			index = 0;
			B_OK == hmod->get_nth_pci_info(index, &info);
			index++
		)
		{
			if ((info.vendor_id == 0x1131)&&(info.device_id == 0x5402))
			{
				trimedia_chip++;
				PRINTF("   found a Trimedia board in %dth PCI\n", index);
				PRINTF("   bus       = %3d\n", info.bus);	
				PRINTF("   device    = %3d\n", info.device);	
				PRINTF("   function  = %3d\n", info.function);	
				PRINTF("   interrupt = %3d\n", info.u.h0.interrupt_line);
			}		
		}
	}
	
	/* release PCI module */
	if (return_value == B_OK)
	{
		put_module(B_PCI_MODULE_NAME);
	}
	
	/* check that at least one TriMedia has been found */
	if ((return_value == B_OK)&&(trimedia_chip == 0))
	{
		return_value = B_ERROR;
	}
	else
	{
		/*
		void *mapped_sdram;
		void *mapped_mmio;
		
	    map_physical_memory
	    	(
	    	"tmtest_sdram",
	    	(void *)0xfc000000, 	//pHal->pSDRAMAddrPhysical,
	    	8*1024*1024, 	//pHal->SDRAMLength,
	    	B_ANY_KERNEL_ADDRESS,
	    	B_READ_AREA|B_WRITE_AREA,
	    	&mapped_sdram
	    	);
	    	
	    map_physical_memory
	    	(
	    	"tmtest_mmio",
	    	(void *)0xfe800000, 	//pHal->pSDRAMAddrPhysical,
	    	2*1024*1024, 	//pHal->SDRAMLength,
	    	B_ANY_KERNEL_ADDRESS,
	    	B_READ_AREA|B_WRITE_AREA,
	    	&mapped_mmio
	    	);
		
		//be sure to don't continue driver execution
		return_value = B_ERROR;
		*/
	}
	
	PRINTF("}\n");

	PRINTF("###########################init_hardware (return)\n");
	
	return return_value;	
}
/* ========================================================================== */
_EXPORT
status_t
init_driver()
{
	PRINTF("###########################init_driver\n");
	
	/* init driver (global variable) */
	{
		ioparms IoParms;
		
		IoParms.tid = 100;
		tmmInit(&IoParms);
	}
	
	PRINTF("###########################init_driver(return)\n");
	
	return B_OK;
}
/* ========================================================================== */
_EXPORT
void
uninit_driver()
{
	PRINTF("###########################uninit_driver\n");
	
	/* remove interupt handler */
	{
		tHalObject* pHal;
		UInt8       IrqLine;  
		UInt32      Dummy;
		pHal    = (tHalObject*)( (TMManDeviceObject *)(TMManGlobal->DeviceList[0]) )->HalHandle;
		Dummy   = ((tHalObject*)pHal)->PCIRegisters[constTMMANPCIRegisters-1];
		IrqLine = (UInt8)(Dummy & (0xff));
		
		PRINTF("remove_io_interrupt_handler %d %p %p\n",
			IrqLine,
			tm_interrupt_handler,
			(void *)pHal
		 );
		if (B_OK == remove_io_interrupt_handler
			(
			IrqLine,
			tm_interrupt_handler,
			(void *)pHal
			))
		{
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    	PRINTF("==  IRQ HANDLER         ==\n");
    	PRINTF("==  REMOVED => OK     ==\n");
  		PRINTF("== irq     = %d\n", IrqLine);  	
  		PRINTF("== handler = %p\n", tm_interrupt_handler);  	
  		PRINTF("== data    = %p\n", (void *)pHal);  	
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
			PRINTF("remove_io_interrupt_handler successfull\n");
		}
		else
		{
			PRINTF("remove_io_interrupt_handler failed\n");
		}
		
	}
	PRINTF("WARNING shared memory area must be released\n");
	delete_area(find_area("tm_sdram"));
	delete_area(find_area("tm_mmio"));
	
	PRINTF("###########################uninit_driver(return)\n");
}
/* ========================================================================== */
_EXPORT device_hooks *
find_device(const char *name)
{
	PRINTF("###########################device_hooks(do_nothing)(name=%s)\n", name);

	return &tm_device;
}
/* ========================================================================== */
static char *devices[] =
{
	"tm0",
	NULL
};
/* ========================================================================== */
_EXPORT
const char **
publish_devices()
{
	PRINTF("###########################publish_devices(do_nothing)\n");
	
	return (const char **)devices;
}
/* ========================================================================== */
_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;
/* ========================================================================== */
