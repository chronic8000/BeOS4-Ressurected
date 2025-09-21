#include <string.h>
#include <malloc.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <module.h>
#include <PCI.h>
#include <dc10_driver.h>


/*------------------------------------------------------------------------*/

static cpu_status
enter_crit (spinlock *lock)
{
	cpu_status	status;

	status = disable_interrupts ();
	acquire_spinlock (lock);

	return status;
}


/*------------------------------------------------------------------------*/

static void
exit_crit (spinlock *lock, cpu_status status)
{
	release_spinlock (lock);
	restore_interrupts (status);
}

/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/



status_t
init_event(event_t *e, const char *name)
{
	e->blocked_threads = 0;
	e->event_count = 0;
	e->sem = create_sem (0, name);
	if(e->sem < B_NO_ERROR)
		return e->sem;
	return B_NO_ERROR;
}

status_t
uninit_event(event_t *e)
{
	return delete_sem(e->sem);
}

status_t
trigger_event(event_t *e, uint32 flags)
{
	int release_count = e->blocked_threads;
	e->event_count++;
	if(release_count > 0) {
		atomic_add(&e->blocked_threads, -release_count);
		return release_sem_etc(e->sem, release_count, flags);
	}
	else {
		return B_NO_ERROR;
	}
}

status_t
wait_for_event(event_t *e, uint32 flags, bigtime_t timeout)
{
	status_t err;
	uint32		start_event = e->event_count;
retry:
	atomic_add(&e->blocked_threads, 1);
	err = acquire_sem_etc(e->sem, 1, flags, timeout);
	if(err < B_NO_ERROR)
		atomic_add(&e->blocked_threads, -1);
	else if(e->event_count == start_event)
		goto retry;
	return err;
}

/*------------------------------------------------------------------------*/



static status_t dc10_open(const char *, uint32, void **);
static status_t dc10_close(void *);
static status_t dc10_free(void *);
static status_t dc10_ioctl(void *, uint32, void *, size_t);
static status_t dc10_read(void *, off_t, void *, size_t *);
static status_t dc10_write(void *, off_t, const void *, size_t *);



// The published PCI bus interface device names
 
static	char 			*DC10_name[MAX_CARDS + 1];		// device name list 
static	char			names[MAX_CARDS + 1][20]; 

static	int32			open_count[MAX_CARDS];	

static	int32			hw_lock = 0;					// spinlock for hw access 





//semaphore for init_driver and open
static sem_id			init_sem;


static pci_module_info *module;



static dc10_cards cards;


// Detect the presence of the PCI bus manager module
 
status_t init_hardware(void)
{
	status_t err;
	pci_module_info *hmod;
	int index;
	pci_info  info;
	int found;

	err=B_OK;
	index=0;
	found=0;
	
	//dprintf("dc10 init_hardware :: begin\n");
	
	//get PCI module	
	err=get_module(B_PCI_MODULE_NAME, (module_info **) &hmod);
	if(err!=B_OK)
	{
		//dprintf("dc10 init_hardware :: get_module error\n");
		return err;
		
	}
	
	
	
	while(err==B_OK)
	{
		//get nth pci board info
		err=hmod->get_nth_pci_info(index, &info);
		if(err==B_OK)
		{
			//is it a DC10+
			//ZORAN36067 Id
			if((info.device_id==0x6057)&&(info.vendor_id==0x11de))
			{
				found++;
			}
		}
		index++;
	}
	
	//Unset PCI module
	put_module(B_PCI_MODULE_NAME);
	
	//found DC10+
	if(found!=0)
	{
		err=B_OK;
		//dprintf("dc10 init_hardware :: found %d card(s)\n",found);	
	}
	else
	//Not found
	{
		err=B_ERROR;
		//dprintf("dc10 init_hardware :: no cards\n");
	}	
	
	//dprintf("dc10 init_hardware :: fin\n");	
    return err;
}


// Initialize the PCI bus manager module
 
status_t init_driver(void)
{
   	status_t err;
	int index;
	pci_info  info;
	int found;
	int32 len;
	
	//dprintf("dc10 init_driver :: begin\n");
	
	err=B_OK;
	index=0;
	found=0;
	
	for(index=0;index<MAX_CARDS;index++)
	{
		open_count[index]=1;
	}
	index=0;
	
	init_sem = create_sem(1, "DC10 init_sem");
	if (init_sem < 0) 
	{
		//dprintf("dc10 init_driver :: error create sem\n");
		return ENOSYS;
	}
	
	//get pci module
	err=get_module(B_PCI_MODULE_NAME, (module_info **) &module);
	if(err!=B_OK)
	{
		delete_sem(init_sem);
		//dprintf("dc10 init_driver :: error get_module\n");
		return err;
	}
	

	while(err==B_OK)
	{
		//get nth PCI board/chipset
		err=module->get_nth_pci_info(index, &info);
		if(err==B_OK)
		{
			//Is it a dc10+ card
			//Zoran36067 Id
			if((info.device_id==0x6057)&&(info.vendor_id==0x11de))
			{
				//memorize info of the nth dc10+ card
				cards.number=found+1;
				cards.index[found]=index;
				cards.bus[found]=info.bus;
				cards.device[found]=info.device;
				cards.function[found]=info.function;

				
				//name
				// add this card's name to driver's name list 
				DC10_name[found] = names[found];
				strcpy(DC10_name[found], DEVICE_NAME);
				
				len = strlen(DC10_name[found]);
				DC10_name[found][len] = '0' + found;
				DC10_name[found][len + 1] = 0;		
				
				DC10_name[found+1] = NULL;
				
				found++;				
			}
		}
		index++;
	}

	if(found!=0)
	{
		//dprintf("dc10 init_driver :: found %d card(s)\n",found);
		err=B_OK;
	}
	else
	{
		delete_sem(init_sem);
		//dprintf("dc10 init_driver :: no card\n");
		err=B_ERROR;
	}	
    return err;
}


// Uninitialize the PCI bus manager module 
void uninit_driver(void)
{
	//dprintf("dc10 uninit_driver \n");
	delete_sem(init_sem);
 	//unset PCI module
    put_module(B_PCI_MODULE_NAME);
}


// Publish the names of the PCI bus interface device 
const char **publish_devices(void)
{
	//dprintf("dc10 publish_devices\n");
    return (const char **) DC10_name;
}



//------------------------------------------------------------------------//

static int32
dc10_inth (void *cookie)
{
	uint32			mask,mask2,mask3,reg,count;
	dc10_private_config		*pd;
	uint32 * preg;
	
	
	pd = (dc10_private_config *)cookie;

	// let's deal with one interrupting device at a time 
	acquire_spinlock (&hw_lock);
	
	//dprintf("dc10 inth\n");
	
	preg=(uint32*)pd->reg_address;
	preg += 15;
	mask = 1 << 27;
	mask2 = 1 << 29;
	mask3 = 1 << 30;
	count = 0 ;
	
	reg = *preg;
	if((reg & mask3) == mask3)
	{
		count ++;
		//dprintf("dc10 synchro verticale %lu \n",*preg & (~(1 << 30)));
		*preg = *preg | (mask3);
	}
	
	if((reg & mask2) == mask2)
	{
		count ++;
		*preg = *preg | (mask2);
		//dprintf("dc10 error\n");
	}
	
	if((reg & mask) == mask)
	{	
		//dprintf("dc10 thread  ok\n");
		//clear interrupt	
		*preg = *preg | (mask);
		count ++;
		if (pd->mode == 1)
		{	
			//dprintf("thread ok\n");
			pd->frame_number++;
			
			pd->time_stamp = system_time();		
				
			pd->capture_status =  DC10_CAPTURE_OK ;
			
			trigger_event(&pd->sync_event, B_DO_NOT_RESCHEDULE);
				
			release_spinlock (&hw_lock);			
			//return B_INVOKE_SCHEDULER;	
			return B_HANDLED_INTERRUPT;
		}
		else
		{
			release_spinlock (&hw_lock);
			return B_HANDLED_INTERRUPT;
		}
	}
	
	release_spinlock (&hw_lock);
	if(count > 0)
	{
		return B_HANDLED_INTERRUPT;
	}
	
	
	return B_UNHANDLED_INTERRUPT;
	
}

//------------------------------------------------------------------------//



// Open the PCI bus interface device driver

static status_t
dc10_open(const char *name, uint32 flags, void **cookie)
{
	status_t err;
   	dc10_private_config * pd;
   	int32 card;
   	char	s[32];
   	void * address;
   	area_id area;
   	physical_entry table;
   	uint32 * preg;
   	char path[256];
   	
   	//dprintf("/dev/%s open :: begin\n", name);
    *cookie = NULL;
    
 
	
    // do we have a match for name? 
	for (card = 0; card < cards.number; card++)
		if (strcmp(name, DC10_name[card]) == 0)
			break;
	
	// error if not found 		
	if (card == cards.number)
		return B_ERROR;
		
				
    // critical section 
	err = acquire_sem_etc(init_sem, 1, B_TIMEOUT | B_CAN_INTERRUPT, 2000000);
	if (err < B_OK) return err;
	
	// ensure we don't open the driver too many times 
	if (open_count[card] < 1) 
	{
		release_sem(init_sem);
		return B_ERROR;
	}
	
	// allocate dc10_config
	pd = (dc10_private_config *)calloc (1,sizeof(*pd));
	*cookie = (void *)pd;
	
	pd->id=card;
	pd->index=cards.index[card]; 
    pd->bus=cards.bus[card];
    pd->device=cards.device[card];
    pd->function=cards.function[card];
    
    //bus master enable
    //memory access enable
    pd->offset=0x04;
    pd->size=4;
    pd->value=6;
    module->write_pci_config(pd->bus,
                			 pd->device,
                			 pd->function,
                			 pd->offset, 
                			 pd->size,
                			 pd->value);
    
   
	
	//memory base address
	pd->size=4;
	pd->offset=0x10;
	pd->value = module->read_pci_config(pd->bus,
                					 	pd->device,
                     					pd->function,
                     					pd->offset,
                     					pd->size);
	
	
	strcpy(pd->reg_name,"PCI Reg :");
	strcat(pd->reg_name, name);
	
	
	pd->reg_physical_address=(void*)pd->value;
	pd->reg_size=4096;
    pd->reg_flags=B_ANY_KERNEL_ADDRESS;
    pd->reg_protection=B_READ_AREA | B_WRITE_AREA;
    pd->reg_address=(void*)B_PAGE_SIZE;
	
	pd->reg_area_id=map_physical_memory(pd->reg_name,
                				   		pd->reg_physical_address,
               					   		pd->reg_size,
               					   		pd->reg_flags,
                				   		pd->reg_protection,
                				   		&pd->reg_address);
    
    pd->offset=0x3c;
    pd->size=4;
    
    pd->interruptline = module->read_pci_config(pd->bus,
                     					 		pd->device,
                     					 		pd->function,
                     					 		pd->offset,
                     					 		pd->size);
     
                    					 		
    pd->interruptline=pd->interruptline & 255 ;
    //dprintf("/dev/%s dc10 interrupt %ld\n", name,pd->interruptline);
    
    //dprintf("/dev/%s dc10 taille %ld\n", name,B_PAGE_SIZE);
    address=(void *)(257*B_PAGE_SIZE);
    
    strcpy(path,"dc10buffers :");
	strcat(path, name);
    area = create_area(path,
    				   &address,
    				   B_ANY_KERNEL_ADDRESS,
    				   257*B_PAGE_SIZE,
    				   B_CONTIGUOUS|B_FULL_LOCK,
    				   B_READ_AREA|B_WRITE_AREA);							 
	if(area<0)
	{
		//dprintf("/dev/%s open :: erreur création area\n",name);
	}
	else
	{
		//dprintf("/dev/%s open ::  création area\n",name);
	}
	
	err = get_memory_map(address,
                		 257*B_PAGE_SIZE,
                		 &table,
                		 1);
    pd->buffer_area = area;
    pd->buffer_address = address;
    pd->buffer_physical_address = table.address;
    pd->buffer_size = table.size;
    //dprintf("/dev/%s dc10 physical address %ld\n", name,table.address);
    //dprintf("/dev/%s dc10 size %ld %ld\n", name,table.size,257*B_PAGE_SIZE);
    if(err < 0)
    {
    	 //dprintf("/dev/%s open :: err get memory map  \n",name);
	}
	
	
	preg = (uint32 *) address;
	*preg = (uint32) pd->buffer_physical_address + 4*4;
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 6*4;
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 8*4;
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 10*4;
	 
	preg++;
	*preg = (uint32) pd->buffer_physical_address + B_PAGE_SIZE;
	preg++;
	*preg =  ((16*B_PAGE_SIZE) << 1)| 1;
	
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 65*B_PAGE_SIZE;
	preg++;
	*preg =  ((16*B_PAGE_SIZE) << 1)| 1;
	
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 129*B_PAGE_SIZE;
	preg++;
	*preg =  ((16*B_PAGE_SIZE) << 1)| 1;
	
	preg ++;
	*preg = (uint32) pd->buffer_physical_address + 193*B_PAGE_SIZE;
	preg++;
	*preg =  ((16*B_PAGE_SIZE) << 1) | 1;
	
	
	                 					 		
    // create the synchronization semaphore 
	strcpy(s, DC10_name[pd->id]);
	strcat(s, " Frame Sync");                 					 		
    if(init_event(&pd->sync_event, s) < B_NO_ERROR)
	{
		//dprintf(("/dev/%s: couldn't create sync semaphore\n",DC10_name[pd->id]));
		goto err_exit;
	}

	// change sem owner to system 
	set_sem_owner (pd->sync_event.sem, B_SYSTEM_TEAM);
	
	pd->mode=0;
	err=install_io_interrupt_handler (pd->interruptline, dc10_inth, (void *)pd,0);
	if(err<0)
	{
		//dprintf("/dev/%s  interrupt ok\n",name);
		goto err_exit;
	}
	
	open_count[card] -= 1;	
    release_sem(init_sem);
    return B_OK;
    
    err_exit:
	free(cookie);
	uninit_event(&pd->sync_event);
	//dprintf(("error opening /dev/%s\n",DC10_name[pd->id]));
	release_sem(init_sem);
	return B_ERROR;
}


// Close the PCI bus interface device driver
static status_t
dc10_close(void *cookie)
{
	dc10_private_config	*pd;
	pd = (dc10_private_config *)cookie;
	//dprintf("/dev/%s close\n",DC10_name[pd->id]);
    return B_OK;
}

// Release the PCI bus interface device driver

static status_t
dc10_free(void *cookie)
{
	dc10_private_config	*pd;	
	pd = (dc10_private_config *)cookie;
	//dprintf("/dev/%s free\n",DC10_name[pd->id]);
	//remove interrupt
	remove_io_interrupt_handler (pd->interruptline, dc10_inth, (void *)pd);
	
	//unmap memory
	delete_area(pd->reg_area_id);
	delete_area(pd->buffer_area);
	
	uninit_event(&pd->sync_event);
	
	open_count[pd->id] =1;
	free(cookie);
    return B_OK;
}



// Perform a I/O control operation on the PCI bus interface

static status_t
dc10_ioctl(void *cookie, uint32 op, void *data, size_t length)
{
    dc10_config *config;
    dc10_private_config * pd;
	cpu_status	stat;
	uint32 * preg;
	int32 temp;
	bigtime_t		config_time_stamp;
	uint32			config_frame_number ;
	uint32			config_capture_status;
	
	pd=(dc10_private_config*)cookie;	
	//dprintf("/dev/%s ioctl\n",DC10_name[pd->id]);
   
    switch (op) {
		//set the whole info on all the dc10+ cards
        case DC10_GET_INFO:
        	//dprintf("/dev/%s ioctl :: DC10_GET_INFO\n",DC10_name[pd->id]);
            config=(dc10_config*)data;
            config->reg_address =  pd->reg_address;
            config->buffer_physical_address = pd->buffer_physical_address;
            return B_OK;
		case DC10_SYNC:
			//dprintf("/dev/%s ioctl :: DC10_SYNC\n",DC10_name[pd->id]);
			config=(dc10_config*)data;
			config_frame_number = config->frame_number;
					
			temp = wait_for_event(&pd->sync_event, B_RELATIVE_TIMEOUT, config->timeout);
			stat = enter_crit (&hw_lock);
			if (temp == B_TIMED_OUT)
			{
				config_time_stamp = 0;
				config_capture_status = DC10_CAPTURE_TIMEOUT;
			}
			else	
			{
				config_time_stamp = pd->time_stamp;
				config_frame_number = pd->frame_number;
				config_capture_status = pd->capture_status;
				config->time_start = pd->time_start;
			}
			exit_crit (&hw_lock, stat);

			config->time_stamp = config_time_stamp;
			config->frame_number = config_frame_number;
			config->capture_status = config_capture_status;	
			
			preg = (uint32 *) pd->buffer_address;
			config->buffer_info[0] = *preg;
			preg ++;
			config->buffer_info[1] = *preg;
			preg ++;
			config->buffer_info[2] = *preg;
			preg ++;
			config->buffer_info[3] = *preg;
			
						
			return B_OK;
		
	
		case DC10_RESET_FRAME_NUMBER:
			//dprintf("/dev/%s ioctl :: DC10_RESET_FRAME_NUMBER\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			pd->frame_number = 0;
			exit_crit (&hw_lock, stat);
			return B_OK;
		case DC10_SET_MODE:
			//dprintf("/dev/%s ioctl :: DC10_SET_MODE\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			pd->mode = config->mode;
			exit_crit (&hw_lock, stat);
			return B_OK;
		case DC10_GET_MODE:
			//dprintf("/dev/%s ioctl :: DC10_GET_MODE\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			config->mode = pd->mode;
			exit_crit (&hw_lock, stat);
			return B_OK;
		case DC10_GET_BUFFER_INFO:
			//dprintf("/dev/%s ioctl :: DC10_GET_BUFFER_INFO\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			preg = (uint32 *) pd->buffer_address;
			config->buffer_info[0] = *preg;
			preg ++;
			config->buffer_info[1] = *preg;
			preg ++;
			config->buffer_info[2] = *preg;
			preg ++;
			config->buffer_info[3] = *preg;
			exit_crit (&hw_lock, stat);
			return B_OK;
		case DC10_SET_BUFFER:
			//dprintf("/dev/%s ioctl :: DC10_SET_BUFFER\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			preg = (uint32 *) pd->buffer_address;
			preg += config->buffer_reset_index;
			*preg = (uint32)pd->buffer_physical_address + 4*4 + 2*4*config->buffer_reset_index;	
			exit_crit (&hw_lock, stat);
			return B_OK;
		case DC10_WRITE_INDEX:
			//dprintf("/dev/%s ioctl :: DC10_WRITE_BUFFER\n",DC10_name[pd->id]);
			config = (dc10_config*)data;
			stat = enter_crit (&hw_lock);
			pd->buffer_index = config->buffer_index;
			exit_crit (&hw_lock, stat);
			return B_OK;
    }

    return B_DEV_INVALID_IOCTL;
}


// Read data from the PCI bus interface driver 
static status_t
dc10_read(void *cookie, off_t position, void *data, size_t *length)
{
	dc10_private_config * pd;
	void * pt;
	uint32 * preg;
	uint32 l;
	size_t  nb;
	
	
	//dprintf("/dev/%s read\n",DC10_name[pd->id]);
	
	nb = *length;
	
	pd = (dc10_private_config *) cookie;
	if (nb>3)
	{
		*length = 0;
		return B_ERROR;
	}
	preg = (uint32 *) pd->buffer_address;
	l = preg [nb];
	preg += ((1+64*(nb))*B_PAGE_SIZE) / 4 ;
	pt = (void *)preg;
	l = (l & ((1 <<23)-1)) /2; 
	memcpy(data,pt,l);
	* length = l;
	
	return B_OK;
}



// Write data to the PCI bus interface driver
static status_t
dc10_write(void *cookie, off_t position, const void *data, size_t *length)
{
	uint32 * preg;
	dc10_private_config * pd;
	
	//dprintf("/dev/%s write\n",,DC10_name[pd->id]);
	
	pd = (dc10_private_config *) cookie;
			
	if (pd->buffer_index < 4)
	{
		preg = (uint32 *) pd->buffer_address;
		preg += ((1+64*(pd->buffer_index))*B_PAGE_SIZE) / 4 ;
					
		memcpy((void*)preg,data,*length);
				
		preg = (uint32 *) pd->buffer_address;
		preg += pd->buffer_index;
		*preg = (uint32)pd->buffer_physical_address + 4*4 + 2*4*pd->buffer_index;	
	}
	else
	{
		*length = 0;
	}

	return B_OK;
}


// The published PCI bus interface device hooks

static device_hooks dc10_hooks = {
	dc10_open, 
	dc10_close, 
	dc10_free,
    dc10_ioctl, 
    dc10_read, 
    dc10_write
    
};

// Find an specific PCI bus device by name 
device_hooks *find_device(const char *name)
{   
    return &dc10_hooks;
}
