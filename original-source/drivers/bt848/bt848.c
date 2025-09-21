/*------------------------------------------------------------------------
	bt848.c

	-- DO NOT DISTRIBUTE OUTSIDE BE INC --
	
	This driver developed with information received
	from Brooktree under nondisclosure
	 
	Device driver for Brooktree Bt848 video capture chip.

------------------------------------------------------------------------*/

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <byteorder.h>
#include <bt848_driver.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "bt848.h"

/*------------------------------------------------------------------------*/

/*#define BT848_DEBUG*/
#ifdef	BT848_DEBUG
#define ddprintf(x) dprintf x
#else
#define ddprintf(x)
#endif

/*------------------------------------------------------------------------*/

typedef struct {
	int32 	blocked_threads;
	sem_id	sem;
	uint32	event_count;
} event_t;

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
	int		start_event = e->event_count;
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

#define R4
#ifdef R4

_EXPORT status_t init_driver(void);
_EXPORT void uninit_driver(void);
_EXPORT const char ** publish_devices(void);
_EXPORT device_hooks * find_device(const char *);

static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;
static sem_id			init_sem;

#define GET_NTH_PCI_INFO	(*pci->get_nth_pci_info)
#define WRITE_PCI_CONFIG	(*pci->write_pci_config)
#define RAM_ADDRESS			(*pci->ram_address)

#else

#define GET_NTH_PCI_INFO	get_nth_pci_info
#define WRITE_PCI_CONFIG	write_pci_config
#define RAM_ADDRESS			ram_address

#endif

/*------------------------------------------------------------------------
	definitions, private types 
------------------------------------------------------------------------*/
#define DRIVER_VERSION	4
#define DEVICE_NAME		"video/bt848/"
	
#define RING_SIZE	3			/* maximum number of buffers in capture ring buffer */ 
#define NUM_OPENS	1			/* the max # of concurrent opens for this driver */
#define	MAX_CARDS	4			/* maximum number of cards */
#define CODE_SIZE	4096*8		/* memory alloted for risc program per field */

#define	CONTINUOUS_FRAME	1
#define SINGLE_FIELD_EVEN	2
#define SINGLE_FIELD_ODD	3
#define SINGLE_FRAME		4
#define CONTINUOUS_EVEN		5
#define CONTINUOUS_ODD		6

#define	NTSC_VDELAY			26
#define PAL_VDELAY			34
#define DMA_CTL				0x0f

#define	VERTICAL_MAX		576
#define	B_NTSC_VBI_SAMPLES	1600
#define B_PAL_VBI_SAMPLES	2000

/* typedef for private data area */

typedef struct {
	uint8	e_crop;		/* bit 9-8: o_vdelay, e_active, e_hdelay, e_hactive */
	uint8	e_vdelay_lo;
	uint8	e_vactive_lo;
	uint8	e_hdelay_lo;
	uint8	e_hactive_lo;
	uint8	o_crop;
	uint8	o_vdelay_lo;
	uint8	o_vactive_lo;
	uint8	o_hdelay_lo;
	uint8	o_hactive_lo;
	
	uint8	e_hscale_hi;
	uint8	e_hscale_lo;
	uint8	o_hscale_hi;
	uint8	o_hscale_lo;
	
	uint8	e_vscale_hi;	/* bit 5-0 */
	uint8	e_vscale_lo;
	uint8	o_vscale_hi;	/* bit 5-0 */
	uint8	o_vscale_lo;
	uint8	e_vtc;
	uint8	o_vtc;

	/* cb sense ?? */

	uint8	color_fmt;
	uint8	color_ctl;	/* bit 3-0 */
} bt848_state;

typedef struct 
{
	uint32			id;					/* card number  */
	uint32			*Bt848;				/* pointer to bt848 h/w */
	uint32			deviceid;
	//sem_id			sync_sem;			/* capture complete sem */
	event_t			sync_event;			/* capture complete */
	
	int32			mode;
	
	area_id			pri_risc_area[RING_SIZE];
	area_id			alt_risc_area[RING_SIZE];
	uint32			*pri_risc_phys_memory[RING_SIZE];
	uint32			*pri_risc_log_memory[RING_SIZE];
	uint32			*alt_risc_phys_memory[RING_SIZE];
	uint32			*alt_risc_log_memory[RING_SIZE];
	int32			pri_number_buffers;
	int32			alt_number_buffers;
	
	bool			primary;			/* risc program - primary or alternate */
	bool			swap;				/* swap programs at next interrupt */
	bt848_state		pri_state;
	bt848_state		alt_state;
	int32			number_buffers;		/* in the ring */
	int32			current_buffer;		/* in process */
	int32			last_buffer;		/* most recently filled */

	uint32			frame_number;
	bigtime_t		time_stamp;
	uint32			capture_status;
	
	uint32			pri_capctl;
	uint32			alt_capctl;
	bool			active;
	
//	uint32			odd[VERTICAL_MAX/2];
//	uint32			even[VERTICAL_MAX/2];
	
	uint32			error;
	
} priv_data;

/*------------------------------------------------------------------------
	globals for this driver
------------------------------------------------------------------------*/

#ifdef R4
		int32			api_version = B_CUR_DRIVER_API_VERSION;
#endif
	
static	int32			hw_lock = 0;					/* spinlock for hw access */
static	int32			num_cards;
static	pci_info		pciinfo[MAX_CARDS];				/* pci info */
static	area_id			Bt848_area[MAX_CARDS];			/* mapped Bt848 registers */
static	uint32			*Bt848_base[MAX_CARDS];
static	int32			open_count[MAX_CARDS];			/* # opens we can do */
static	long			open_lock[MAX_CARDS];			/* spinlock for open/close */
static	char 			*Bt848_name[MAX_CARDS + 1];		/* device name list */
static	char			names[MAX_CARDS + 1][20]; 

extern uint32 _swap_int32(uint32 uarg);
static uint32 read_bt848_reg(uint32 *bt848,uint32 reg); 
static void write_bt848_reg(uint32 *bt848,uint32 reg, uint32 value); 
static void create_risc(bool primary, uint32 buffer_num, uint32 *bt848, priv_data *pd, bt848_config *config);
static void run_risc(bool primary, priv_data *pd);
static void halt_risc(priv_data *pd);
static void init_regs(uint32 *bt848, bt848_config *config);
static void update_state(uint32 *bt848, bt848_state *state);
static void init_state(bt848_state *state, bt848_config *config);
int32 I2CRead(uint32 *bt848, int32 addr, uint32 speed);
int32 I2CWrite1(uint32 *bt848, int32 addr, int32 dat1, uint32 speed);
int32 I2CWrite2(uint32 *bt848, int32 addr, int32 dat1, int32 dat2, uint32 speed);
int32 SelectChannel(uint32 *bt848, int32 channel, int32 locale, int32 brand, int32 speed);
int32 AudioInit(uint32 *bt848);
int32 SelectAudio(uint32 *bt848, uchar brand, uchar source);

/*------------------------------------------------------------------------*/

status_t
init_driver (void)
{
	int32 i, len;

	init_sem = create_sem(1, "Bt848 init_sem");
	if (init_sem < 0) return ENOSYS;

#ifdef BT848_DEBUG
	set_dprintf_enabled(TRUE);
#endif

#ifdef R4
	if (get_module(pci_name, (module_info **) &pci)) {
		delete_sem(init_sem);
		return ENOSYS;
	}
#endif

	for (i=0; i < MAX_CARDS; i++)
	{
		Bt848_area[i] = -1;
		open_count[i] = NUM_OPENS;
		open_lock[i] = 0;
		Bt848_name[i] = 0;
	}
			
	num_cards=0;
	for (i = 0; ; i++) 
	{	
		if (GET_NTH_PCI_INFO (i, &pciinfo[num_cards]) != B_NO_ERROR)
		{
			break;
		}
		if ((pciinfo[num_cards].vendor_id == VENDORID) && 
			((pciinfo[num_cards].device_id == DEVICEID) ||
			 (pciinfo[num_cards].device_id == DEVICEID2) ||
			 (pciinfo[num_cards].device_id == DEVICEID3) ||
			 (pciinfo[num_cards].device_id == DEVICEID4))) 
		{		
			/* map the bt848 registers */
			Bt848_area[num_cards] = map_physical_memory("Bt848 registers",
									(void *)(pciinfo[num_cards].u.h0.base_registers[0] & ~(B_PAGE_SIZE - 1)),
									pciinfo[num_cards].u.h0.base_register_sizes[0],
									B_ANY_KERNEL_ADDRESS,
									B_READ_AREA + B_WRITE_AREA,
									(void *)&Bt848_base[num_cards]);
									
			if ((int32)Bt848_area[num_cards] < 0) 
			{
				ddprintf(("Problems mapping /dev/video/nubt848/%ld device registers\n",num_cards));
				delete_sem(init_sem);
				return B_ERROR;
			}
			
			/* correct for offset within page */
			Bt848_base[num_cards] += (pciinfo[num_cards].u.h0.base_registers[0] & (B_PAGE_SIZE -1))/sizeof(uint32);
								
			/* enable bt848 for bus initiator and memory space accesses	*/
			WRITE_PCI_CONFIG(pciinfo[num_cards].bus,pciinfo[num_cards].device,pciinfo[num_cards].function,0x04,4,0x02800006);
			
			/* add this card's name to driver's name list */
			Bt848_name[num_cards] = names[num_cards];
			strcpy(Bt848_name[num_cards], DEVICE_NAME);
			len = strlen(Bt848_name[num_cards]);
			Bt848_name[num_cards][len] = '0' + num_cards;
			Bt848_name[num_cards][len + 1] = 0;		
			ddprintf(("/dev/%s: %s %s base address: %p\n", Bt848_name[num_cards], __DATE__, __TIME__,Bt848_base[num_cards]));

			Bt848_name[++num_cards] = NULL;
			if (num_cards == MAX_CARDS)
			{
				ddprintf(("Too many Bt848 cards installed\n"));
				break;
			}
		}
	}
	
	if (num_cards == 0) {
		delete_sem(init_sem);
		return ENODEV;
	}
	else 
		return B_NO_ERROR;
}

/*------------------------------------------------------------------------*/

void
uninit_driver (void)
{
	uint32 i;
	
	for (i=0; i<num_cards; i++)
	{
		delete_area(Bt848_area[i]);
	}
	
	put_module(pci_name);
	delete_sem(init_sem);
}

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

static int32
Bt848_inth (void *cookie)
{
	uint32			val,mask;
	//int32			count;
	priv_data		*pd;
	
	pd = (priv_data *)cookie;

	/* let's deal with one interrupting device at a time */
	acquire_spinlock (&hw_lock);
	val = read_bt848_reg(pd->Bt848,INT_STAT);
	mask = read_bt848_reg(pd->Bt848,INT_MASK);
		
	if ((val & mask) != 0) 
	{
		/* OK, it's one of ours	*/												
		if ((val & BT848_RISC_IRQ) == BT848_RISC_IRQ) 
		{	
			/* it's a risc interrupt, ie end of capture */
			write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0d);	/* stop risc procesor */
			write_bt848_reg(pd->Bt848,INT_STAT,val);				/* clear interrupt */
			/* restart risc program if we're in continuous mode */
			if (pd->mode == BT848_CONTINUOUS) 
			{
				/* advance to next buffer, update last buffer pointer */
				pd->last_buffer = pd->current_buffer;

				/* we're being asked to switch to alternate programs */
				if (pd->swap)
				{
					//ddprintf(("pd->swap_config = %p\n", pd->swap_config));
					pd->primary = !pd->primary;
					update_state(pd->Bt848, pd->primary ?
						&pd->pri_state : &pd->alt_state);
					pd->number_buffers = pd->primary ?
						pd->pri_number_buffers :
						pd->alt_number_buffers;
#if 0
					if (pd->primary)
						ddprintf(("swap to primary\n"));
					else
						ddprintf(("swap to alt\n"));
#endif
					
					/* clear swap request to acknowledge */
					pd->swap = false;
				}
				
				if (++(pd->current_buffer) >= pd->number_buffers)
					pd->current_buffer = 0;
					
				/* restart risc program */
				if (pd->primary) {
					if(pd->current_buffer < pd->pri_number_buffers) {
						write_bt848_reg(pd->Bt848,RISC_STRT_ADD,
						  (uint32)pd->pri_risc_phys_memory[pd->current_buffer]);
					}
					else {
						ddprintf(("/dev/%s: bad pri buffer %ld of %ld\n",
						          Bt848_name[pd->id], pd->current_buffer,
						          pd->pri_number_buffers));
					}
				}
				else {
					if(pd->current_buffer < pd->alt_number_buffers) {
						write_bt848_reg(pd->Bt848,RISC_STRT_ADD,
						  (uint32)pd->alt_risc_phys_memory[pd->current_buffer]);
					}
					else {
						ddprintf(("/dev/%s: bad alt buffer %ld of %ld\n",
						          Bt848_name[pd->id], pd->current_buffer,
						          pd->alt_number_buffers));
					}
				}
				
				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL);
			}			
			pd->frame_number++;
			pd->time_stamp = system_time();
			pd->capture_status = BT848_CAPTURE_OK;
#if 0
			get_sem_count(pd->sync_sem, &count);
			
			/* if we've got clients waiting on us, turn them loose */
			if (count < 0)
			{
				release_sem_etc(pd->sync_sem, -count, B_DO_NOT_RESCHEDULE);
			}
#endif			
			release_spinlock (&hw_lock);
			trigger_event(&pd->sync_event, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;
		}
		else 
		{
			/* bad news interrrupt */
			write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0c);	/* stop risc procesor, reset fifo */
			write_bt848_reg(pd->Bt848,INT_STAT,val);				/* clear interrupt */
			pd->capture_status = val & mask;

#if 0
			get_sem_count(pd->sync_sem, &count);
			
			/* unblock our clients so they can deal with the error */
			if (count < 0)
				release_sem_etc(pd->sync_sem, -count, B_DO_NOT_RESCHEDULE);
#endif
			trigger_event(&pd->sync_event, B_DO_NOT_RESCHEDULE);
			
			if(val & 0x00040000) {	/* reserved opcode */
				int offset = read_bt848_reg(pd->Bt848,RISC_COUNT) -
					(pd->primary ?
						(uint32)pd->pri_risc_phys_memory[pd->current_buffer] :
						(uint32)pd->alt_risc_phys_memory[pd->current_buffer]);
				ddprintf(("/dev/%s: bad RISC instruction offset %d\n",
				          Bt848_name[pd->id], offset));
				ddprintf(("/dev/%s: base 0x%08lx curr 0x%08lx\n",
					Bt848_name[pd->id], read_bt848_reg(pd->Bt848,RISC_STRT_ADD),
					read_bt848_reg(pd->Bt848,RISC_COUNT)));
				ddprintf(("/dev/%s: b# %ld / %ld, pri 0x%08lx alt 0x%08lx\n",
				    Bt848_name[pd->id], pd->current_buffer,
					pd->number_buffers,
					(uint32)pd->pri_risc_phys_memory[pd->current_buffer],
					(uint32)pd->pri_risc_phys_memory[pd->current_buffer]));
				if(offset > 0 && offset < CODE_SIZE*2) {
					uint32 *addr;
					if(pd->primary)
						addr = pd->pri_risc_log_memory[pd->current_buffer];
					else
						addr = pd->alt_risc_log_memory[pd->current_buffer];
					addr += offset/4;
					ddprintf(("/dev/%s: opcode 0x%08lx\n", Bt848_name[pd->id],
					          *addr));
				}
			}
			/* restart risc program ? */
			else if(pd->active && pd->mode == BT848_CONTINUOUS) {
				//ddprintf(("/dev/%s: restart capture\n", Bt848_name[pd->id]));
				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0c);
				write_bt848_reg(pd->Bt848,CAP_CTL,0x00);
				
				if(pd->primary)
					write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->pri_risc_phys_memory[0]);
				else
					write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->alt_risc_phys_memory[0]);
							
				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0d);	
				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL);
				
				if(pd->primary)
					write_bt848_reg(pd->Bt848,CAP_CTL,pd->pri_capctl);
				else
					write_bt848_reg(pd->Bt848,CAP_CTL,pd->alt_capctl);

#if 0

				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0c);
				if (pd->primary)
					write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->pri_risc_phys_memory[pd->current_buffer]);
				else
					write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->alt_risc_phys_memory[pd->current_buffer]);
				write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL);
#endif
			}
			
			release_spinlock (&hw_lock);
			ddprintf(("/dev/%s: unexpected interrupt %lx %lx %lx\n",
			          Bt848_name[pd->id], val, mask,
			          read_bt848_reg(pd->Bt848,RISC_COUNT)));
			return B_HANDLED_INTERRUPT;
		}
	}
	
	release_spinlock (&hw_lock);
	return B_UNHANDLED_INTERRUPT;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_open (const char* name, uint32 flags, void **cookie)
{
	priv_data	*pd;
	int32		i, card, len;
	status_t	err;
	char		s[32];
	physical_entry	table[16];

	/* Error if no cards found */	
	if (num_cards == 0)	
		return B_ERROR;
		
	/* do we have a match for name? */
	for (card = 0; card < num_cards; card++)
		if (strcmp(name, Bt848_name[card]) == 0)
			break;

	/* error if not found */		
	if (card == num_cards)
		return B_ERROR;
					
	/* critical section */
	err = acquire_sem_etc(init_sem, 1, B_TIMEOUT | B_CAN_INTERRUPT, 2000000);
	if (err < B_OK) return err;
	
	/* ensure we don't open the driver too many times */
	if (open_count[card] < 1) 
	{
		release_sem(init_sem);
		return B_ERROR;
	}
	
	/* allocate some private storage */
	pd = (priv_data *)calloc (1,sizeof(*pd));
	*cookie = (void *)pd;

	/* initialize it */
	pd->id				= card;
	pd->Bt848			= Bt848_base[card];
	pd->deviceid		= pciinfo[card].device_id;
	pd->primary			= true;
	pd->swap			= false;
	pd->current_buffer	= 0;
	//pd->sync_sem		= -1;
	pd->sync_event.sem = -1;
	pd->pri_capctl		= 0x03;
	pd->alt_capctl		= 0x03;
	pd->active          = false;

	/*	set up required resources */
	if (open_count[pd->id] == NUM_OPENS) 
	{
		pd->pri_number_buffers = 0;
		pd->alt_number_buffers = 0;
		for (i = 0; i < RING_SIZE; i++)
		{
			/* create area for primary risc program */
			strcpy(s, "/dev/");
			strcat(s, Bt848_name[card]);
			strcat(s," Pri #");
			s[len = strlen(s)] = '0' + i;
			s[len+1] = 0;
			
			pd->pri_risc_area[i] = create_area(s, (void *)&pd->pri_risc_log_memory[i],
						B_ANY_KERNEL_ADDRESS, CODE_SIZE * 2, B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);	
			if (pd->pri_risc_area[i] == B_ERROR ||
				pd->pri_risc_area[i] == B_BAD_VALUE ||
				pd->pri_risc_area[i] == B_NO_MEMORY) 
			{
				ddprintf(("/dev/%s: allocation error for primary risc program "
				          "# %ld\n", Bt848_name[pd->id], i));
				goto err_exit;
			}
			
			/* look up its physical address */
			get_memory_map(pd->pri_risc_log_memory[i],CODE_SIZE*2,table,16);
			pd->pri_risc_phys_memory[i] = (uint32 *) RAM_ADDRESS(table[0].address);		
			/*ddprintf(("/dev/%s #%d: pri risc logical = %08x physical = %08x\n", Bt848_name[pd->id], i,
						pd->pri_risc_log_memory[i],pd->pri_risc_phys_memory[i])); */		
			if (lock_memory (pd->pri_risc_log_memory[i],CODE_SIZE*2,B_DMA_IO | B_READ_DEVICE) == B_ERROR)
			{
				ddprintf(("/dev/%s: lock memory failed\n", Bt848_name[pd->id]));
				goto err_exit;
			}
	
			/* create area for alternate risc program */
			strcpy(s, "/dev/");
			strcat(s, Bt848_name[card]);
			strcat(s," Alt #");
			s[len = strlen(s)] = '0' + i;
			s[len+1] = 0;
			
			pd->alt_risc_area[i] = create_area(s, (void *)&pd->alt_risc_log_memory[i],
						B_ANY_KERNEL_ADDRESS, CODE_SIZE * 2, B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);	
			if (pd->alt_risc_area[i] == B_ERROR ||
				pd->alt_risc_area[i] == B_BAD_VALUE ||
				pd->alt_risc_area[i] == B_NO_MEMORY) 
			{
				ddprintf(("/dev/%s: allocation error for alternate risc program"
				          " # %ld\n", Bt848_name[pd->id], i));
				goto err_exit;
			}
			
			/* look up its physical address */
			get_memory_map(pd->alt_risc_log_memory[i],CODE_SIZE*2,table,16);
			pd->alt_risc_phys_memory[i] = (uint32 *) RAM_ADDRESS(table[0].address);		
			/*ddprintf(("/dev/%s #%d: alt risc logical = %08x physical = %08x\n", Bt848_name[pd->id], i, 
						pd->alt_risc_log_memory[i], pd->alt_risc_phys_memory[i])); */	
			if (lock_memory (pd->alt_risc_log_memory[i],CODE_SIZE*2,B_DMA_IO | B_READ_DEVICE) == B_ERROR)
			{
				ddprintf(("/dev/%s: lock memory failed\n", Bt848_name[pd->id]));
				goto err_exit;
			}
			
			/* do a software reset so we start clean with each open */
			write_bt848_reg(pd->Bt848, SRESET, 0x58);		
		}
		
		/* create the synchronization semaphore */
		strcpy(s, Bt848_name[pd->id]);
		strcat(s, " Frame Sync");
		//pd->sync_sem = create_sem (0, s);
		//if (pd->sync_sem < B_NO_ERROR)
		if(init_event(&pd->sync_event, s) < B_NO_ERROR)
		{
			ddprintf(("/dev/%s: couldn't create sync semaphore\n",
			          Bt848_name[pd->id]));
			goto err_exit;
		}

		/* change sem owner to system */
		//set_sem_owner (pd->sync_sem, B_SYSTEM_TEAM);
		set_sem_owner (pd->sync_event.sem, B_SYSTEM_TEAM);
		
		install_io_interrupt_handler (pciinfo[pd->id].u.h0.interrupt_line, Bt848_inth, (void *)pd,0);
	}
	
	open_count[card] -= 1;
	
	ddprintf(("successful open of /dev/%s\n", Bt848_name[pd->id]));
	release_sem(init_sem);
	return B_NO_ERROR;

err_exit:
	free(cookie);
	uninit_event(&pd->sync_event);
	//if (pd->sync_sem >= 0)
	//	delete_sem (pd->sync_sem);
	ddprintf(("error opening /dev/%s\n",Bt848_name[pd->id]));
	release_sem(init_sem);
	return B_ERROR;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_close (void *cookie)
{

	return B_NO_ERROR;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_free(void *cookie)
{
	int			i;
	priv_data	*pd;
	
	pd = (priv_data *)cookie;
	
	open_count[pd->id] += 1;
	
	if (open_count[pd->id] >= NUM_OPENS) 
	{	
		halt_risc(pd);
		remove_io_interrupt_handler (pciinfo[pd->id].u.h0.interrupt_line,
					Bt848_inth, (void *)pd);
		//delete_sem (pd->sync_sem);
		uninit_event(&pd->sync_event);
		for (i = 0; i < RING_SIZE; i++)
		{
			delete_area(pd->pri_risc_area[i]);
			delete_area(pd->alt_risc_area[i]);
		}
		open_count[pd->id] = NUM_OPENS;		/* just in case... */
	}
	free(cookie);
	return 0;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_read (void *cookie, off_t pos, void *buf, size_t *count)
{
	int32		i;
	priv_data	*pd;
	char	s[128];
	char	*b;
	
	pd = (priv_data *)cookie;
	b = (char *)buf;
	strcpy(s,"Bt848 driver:  ");
	strcat(s,__DATE__);
	strcat(s," ");
	strcat(s,__TIME__);
	strcat(s,"\n");
	
	i=0;
	if ( (pos > strlen(s)) | (*count == 0) ) 
	{
		*count = 0;
		return 0;
	}
	
	while ( ((*b++ = s[pos+i]) != 0) & (i < (uint32)*count) )
		i++;
		
	*count = (size_t) i; 
	return i;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_write (void *cookie, off_t pos, const void *buf, size_t *count)
{
	return 0;
}

/*------------------------------------------------------------------------*/

static int32
Bt848_control (void *cookie, uint32 msg, void *buf, size_t len)
{
	cpu_status		status;
	bt848_config	*config;
	bt848_buffer	*buffer;
	int32 			control,source,format;
	int32			i,temp,speed,count;
	priv_data		*pd;	

	pd = (priv_data *)cookie;	
	config = (bt848_config *) buf;
	buffer = (bt848_buffer *) buf;

	switch (msg) 
	{
		case BT848_INIT_REGS:				/* Initialize Bt848 */
			init_regs(pd->Bt848,config);
			break;
		
		case BT848_BUILD_PRI:				/* Build primary risc program */
			if(config->number_buffers > RING_SIZE) {
				ddprintf(("bt848: error in BT848_BUILD_PRI, "
				          "number_buffers = %ld\n", config->number_buffers));
				return B_ERROR;
			}
			pd->pri_number_buffers = 0;
			for (i = 0; i < config->number_buffers; i++)
			{
				create_risc(true, i, pd->Bt848, pd, config);
				if (pd->error == 1)
					return B_ERROR; /* quit on error */
			}
			pd->pri_number_buffers = config->number_buffers;
			init_state(&pd->pri_state, config);

			break;
		
		case BT848_BUILD_ALT:				/* Build secondary risc program */
			if(config->number_buffers > RING_SIZE) {
				ddprintf(("bt848: error in BT848_BUILD_ALT, "
				          "number_buffers = %ld\n", config->number_buffers));
				return B_ERROR;
			}
			pd->alt_number_buffers = 0;
			for (i = 0; i < config->number_buffers; i++)
			{
				create_risc(false, i, pd->Bt848,pd,config);
				if (pd->error == 1)
					return B_ERROR; /* quit on error */
			}
			pd->alt_number_buffers = config->number_buffers;
			init_state(&pd->alt_state, config);
			break;
	
		case BT848_START_PRI:				/* if no error, start video capture with primary */
			if (pd->error == 0)
			{
				if(config->number_buffers != pd->pri_number_buffers) {
					ddprintf(("bt848: error in BT848_START_PRI, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->pri_number_buffers));
					return B_ERROR;
				}
				pd->number_buffers = config->number_buffers;
				pd->frame_number = config->frame_number = 0;
				pd->primary = true;
				pd->current_buffer = 0;
				update_state(pd->Bt848, &pd->pri_state);
				run_risc(true, pd);
			}
			else
				return B_ERROR;
			break;
			
		case BT848_START_ALT:				/* if no error, start video capture with alternate */
			if (pd->error == 0)
			{
				if(config->number_buffers != pd->alt_number_buffers) {
					ddprintf(("bt848: error in BT848_START_ALT, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->alt_number_buffers));
					return B_ERROR;
				}
				pd->number_buffers = config->number_buffers;
				pd->frame_number = config->frame_number = 0;
				pd->primary = false;
				pd->current_buffer = 0;
				update_state(pd->Bt848, &pd->alt_state);
				run_risc(false, pd);
			}
			else
				return B_ERROR;
			break;
			
		case BT848_RESTART_PRI:				/* start video capture, don't reset frame counter */
			if (pd->error == 0)
			{
				if(config->number_buffers != pd->pri_number_buffers) {
					ddprintf(("bt848: error in BT848_RESTART_PRI, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->pri_number_buffers));
					return B_ERROR;
				}
			    pd->frame_number = config->frame_number;
				pd->primary = true;
				pd->number_buffers = config->number_buffers;
				pd->current_buffer = 0;
				update_state(pd->Bt848, &pd->pri_state);
				run_risc(true, pd);
			}
			else
				return B_ERROR;
			break;
			
		case BT848_RESTART_ALT:				/* start video capture, don't reset frame counter */
			if (pd->error == 0)
			{
				if(config->number_buffers != pd->alt_number_buffers) {
					ddprintf(("bt848: error in BT848_RESTART_ALT, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->alt_number_buffers));
					return B_ERROR;
				}
			    pd->frame_number = config->frame_number;
				pd->primary = false;
				pd->number_buffers = config->number_buffers;
				pd->current_buffer = 0;
				update_state(pd->Bt848, &pd->alt_state);
				run_risc(false, pd);
			}
			else
				return B_ERROR;
			break;

		case BT848_SWITCH_PRI:
		case BT848_SWITCH_ALT: {
			int32			last_buffer;
			bigtime_t		time_stamp;
			uint32			frame_number;
			uint32			capture_status;

			if(msg == BT848_SWITCH_PRI) {
				if(pd->primary) {
					ddprintf(("bt848: error in BT848_SWITCH_PRI, pri active\n"));
					return B_ERROR;
				}
				if(config->number_buffers != pd->pri_number_buffers) {
					ddprintf(("bt848: error in BT848_SWITCH_PRI, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->pri_number_buffers));
					return B_ERROR;
				}
			}
			else {
				if(!pd->primary) {
					ddprintf(("bt848: error in BT848_SWITCH_ALT, alt active\n"));
					return B_ERROR;
				}
				if(config->number_buffers != pd->alt_number_buffers) {
					ddprintf(("bt848: error in BT848_SWITCH_ALT, "
					          "number_buffers = %ld not %ld\n",
					          config->number_buffers, pd->alt_number_buffers));
					return B_ERROR;
				}
			}

			count = 50;
			pd->swap = true;
			while ((--count > 0) && pd->swap)
				snooze(1000);
			if (pd->swap)
			{
				pd->swap = false;
				halt_risc(pd);
				return B_ERROR;
			}
			status = enter_crit (&hw_lock);
			last_buffer = pd->last_buffer;
			time_stamp = pd->time_stamp;
			frame_number = pd->frame_number;
			capture_status = pd->capture_status;
			exit_crit (&hw_lock, status);

			config->last_buffer = last_buffer;
			config->time_stamp = time_stamp;
			config->frame_number = frame_number;
			config->capture_status = capture_status;

			} break;

		case BT848_STOP:				/* stop video capture */
			halt_risc(pd);	
			break;

		case BT848_GET_LAST: {
			int32			last_buffer;
			bigtime_t		time_stamp;
			uint32			frame_number;
			uint32			capture_status;

			/* don't let interrupt world change things until we copy the data */
			status = enter_crit (&hw_lock);
			last_buffer = pd->last_buffer;
			time_stamp = pd->time_stamp;
			frame_number = pd->frame_number;
			capture_status = pd->capture_status;
			exit_crit (&hw_lock, status);

			config->last_buffer = last_buffer;
			config->time_stamp = time_stamp;
			config->frame_number = frame_number;
			config->capture_status = capture_status;
			} break;
			
		case BT848_GET_NEXT:
		case BT848_SYNC: {
			int32			last_buffer;
			bigtime_t		time_stamp;
			uint32			frame_number;
			uint32			capture_status;

			//acquire_sem(pd->sync_sem);
			wait_for_event(&pd->sync_event, 0, 0);
			/* don't let interrupt world change things until we copy the data */
			status = enter_crit (&hw_lock);
			last_buffer = pd->last_buffer;
			time_stamp = pd->time_stamp;
			frame_number = pd->frame_number;
			capture_status = pd->capture_status;
			exit_crit (&hw_lock, status);

			config->last_buffer = last_buffer;
			config->time_stamp = time_stamp;
			config->frame_number = frame_number;
			config->capture_status = capture_status;
			} break;
			
		case BT848_GET_NEXT_TIMEOUT:
		case BT848_SYNC_TIMEOUT: {
			uint32			config_status = config->status;
			int32			config_last_buffer = config->last_buffer;
			bigtime_t		config_time_stamp;
			uint32			config_frame_number = config->frame_number;
			uint32			config_capture_status;

			//temp = acquire_sem_etc(pd->sync_sem, 1, B_TIMEOUT, config->timeout);
			temp = wait_for_event(&pd->sync_event, B_TIMEOUT, config->timeout);
			status = enter_crit (&hw_lock);
			if (temp == B_TIMED_OUT)
			{
				config_status = read_bt848_reg(pd->Bt848, INT_STAT) & 0x00ffffff;
				write_bt848_reg(pd->Bt848,INT_STAT,config_status);	/* clear int bits */
				config_status |= (read_bt848_reg(pd->Bt848, DSTATUS) << 24);
				//ddprintf(("Bt848 sync timeout, status = %x\n",config_status));
				config_time_stamp = 0;
				config_capture_status = BT848_CAPTURE_TIMEOUT;
			}
			else	
			{
				config_last_buffer = pd->last_buffer;
				config_time_stamp = pd->time_stamp;
				config_frame_number = pd->frame_number;
				config_capture_status = pd->capture_status;
			}
			exit_crit (&hw_lock, status);

			config->status = config_status;
			config->last_buffer = config_last_buffer;
			config->time_stamp = config_time_stamp;
			config->frame_number = config_frame_number;
			config->capture_status = config_capture_status;
			} break;
			
		case BT848_SELECT_VIDEO_SOURCE:
			source = config->video_source & 0x03;
			format = config->video_format & 0x07;
			write_bt848_reg(pd->Bt848,IFORM,0x18 + (source << 5) + format);

			control = 0x80;						/* sync threshold hi, agc enabled, normal clock */
												/* enable luma adc, enable chroma adc */

			if (config->video_source < 4)		/* disable chroma A/D if composite */
				control = control + 0x02;
	
			write_bt848_reg(pd->Bt848,ADC,control);						
	
			if (config->video_source > 3)		/* enable chroma A/D if svideo */
			{
				write_bt848_reg(pd->Bt848,E_CONTROL,read_bt848_reg(pd->Bt848,E_CONTROL) | 0x40);												
				write_bt848_reg(pd->Bt848,O_CONTROL,read_bt848_reg(pd->Bt848,O_CONTROL) | 0x40);
			}
			else
			{
				write_bt848_reg(pd->Bt848,E_CONTROL,read_bt848_reg(pd->Bt848,E_CONTROL) & 0xbf);												
				write_bt848_reg(pd->Bt848,O_CONTROL,read_bt848_reg(pd->Bt848,O_CONTROL) & 0xbf);
			}
			
			break;

		case BT848_BRIGHTNESS:
			control =  (((int32)config->brightness * 127)/100) & 0xff;	
			write_bt848_reg(pd->Bt848, BRIGHT, control);
			break;
			
		case BT848_CONTRAST:	
			control = ( ((int32)config->contrast*216)/100 + 0xd8) & 0x1ff;			
			write_bt848_reg(pd->Bt848,CONTRAST_LO,control & 0xff);
			write_bt848_reg(pd->Bt848,O_CONTROL,(read_bt848_reg(pd->Bt848, O_CONTROL)&0xfb) + ((control & 0x100) >> 6));
			write_bt848_reg(pd->Bt848,E_CONTROL,(read_bt848_reg(pd->Bt848, E_CONTROL)&0xfb) + ((control & 0x100) >> 6));
			break;
			
		case BT848_HUE:	
			control =  (((int32)config->hue * 127)/100) & 0xff;	
			write_bt848_reg(pd->Bt848,HUE,control);			
			break;
			
		case BT848_SATURATION:
			/* do U component first	*/		
			if (config->video_format == BT848_SECAM)
				control = ( ((int32)config->saturation*135)/100 + 135) & 0x1ff;			
			else
				control = ( ((int32)config->saturation*254)/100 + 254) & 0x1ff;			
			write_bt848_reg(pd->Bt848,SAT_U_LO,control & 0xff);		
			write_bt848_reg(pd->Bt848,E_CONTROL,(read_bt848_reg(pd->Bt848, E_CONTROL)&0xfd) + ((control & 0x100) >> 7));
			write_bt848_reg(pd->Bt848,O_CONTROL,(read_bt848_reg(pd->Bt848, O_CONTROL)&0xfd) + ((control & 0x100) >> 7));
			/* do V component */
			if (config->video_format == BT848_SECAM)
				control = ( ((int32)config->saturation*133)/100 + 133) & 0x1ff;
			else					
				control = ( ((int32)config->saturation*180)/100 + 180) & 0x1ff;
			write_bt848_reg(pd->Bt848,SAT_V_LO,control & 0xff);
			write_bt848_reg(pd->Bt848,E_CONTROL,(read_bt848_reg(pd->Bt848, E_CONTROL)&0xfe) + ((control & 0x100) >> 8));
			write_bt848_reg(pd->Bt848,O_CONTROL,(read_bt848_reg(pd->Bt848, O_CONTROL)&0xfe) + ((control & 0x100) >> 8));
			break;

		case BT848_ALLOCATE_BUFFER:
			buffer->buffer_area = create_area("Bt848 buffer", &buffer->buffer_address,
				B_ANY_ADDRESS, buffer->buffer_size, B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);
			if (buffer->buffer_area == B_ERROR ||
				buffer->buffer_area == B_BAD_VALUE ||
				buffer->buffer_area == B_NO_MEMORY ) 
			{
				ddprintf(("error in BT848_ALLOCATE_BUFFER\n"));
				return B_ERROR;
			}
			
			if (lock_memory(buffer->buffer_address, buffer->buffer_size, B_DMA_IO | B_READ_DEVICE) == B_ERROR) 
			{
				ddprintf(("error locking Bt848buffer\n"));
				return B_ERROR;
			}
			break;	
	
		case BT848_FREE_BUFFER:
			unlock_memory(buffer->buffer_address, buffer->buffer_size, B_DMA_IO | B_READ_DEVICE);
			delete_area(buffer->buffer_area);
			break;
			
		case BT848_I2CREAD:	
			switch(pd->deviceid)
			{
				case DEVICEID3:
				case DEVICEID4:
					speed = 0x80;
					break;
				case DEVICEID:
				case DEVICEID2:
				default:
					speed = 0x50;
					break; 
			}	
			temp = I2CRead(pd->Bt848, config->i2c_address, speed);
			switch(temp) 
			{
				case BT848_I2C_DONE_ERROR:
				case BT848_I2C_RACK_ERROR:
					config->i2c_status = (char)temp;
					break;
				default:
					config->i2c_data1 = (char)temp;
					config->i2c_status = BT848_I2C_SUCCESS;
					break;						
			}
			break;
			
		case BT848_I2CWRITE1:		
			switch(pd->deviceid)
			{
				case DEVICEID3:
				case DEVICEID4:
					speed = 0x80;
					break;
				case DEVICEID:
				case DEVICEID2:
				default:
					speed = 0x50;
					break; 
			}	
			config->i2c_status = (char) I2CWrite1(pd->Bt848, config->i2c_address, config->i2c_data1, speed);
			break;
	
		case BT848_I2CWRITE2:		
			switch(pd->deviceid)
			{
				case DEVICEID3:
				case DEVICEID4:
					speed = 0x80;
					break;
				case DEVICEID:
				case DEVICEID2:
				default:
					speed = 0x50;
					break; 
			}	
			config->i2c_status = (char) I2CWrite2(pd->Bt848, config->i2c_address, config->i2c_data1, config->i2c_data2, speed);
			break;

		case BT848_WRITE_GPIO_OUT_EN:
			write_bt848_reg(pd->Bt848,GPIO_OUT_EN,config->gpio_out_en);
			break;	
				
		case BT848_WRITE_GPIO_IN_EN:
			write_bt848_reg(pd->Bt848,GPIO_REG_INP,config->gpio_in_en);
			break;
					
		case BT848_READ_GPIO_DATA:
			config->gpio_data = read_bt848_reg(pd->Bt848, GPIO_DATA);
			break;
					
		case BT848_WRITE_GPIO_DATA:
			write_bt848_reg(pd->Bt848,GPIO_DATA,config->gpio_data);
			break;		

		case BT848_STATUS:	
			config->status = read_bt848_reg(pd->Bt848, INT_STAT) & 0x00ffffff;
			write_bt848_reg(pd->Bt848,INT_STAT,config->status);	/* clear int bits */
			config->status = config->status | (read_bt848_reg(pd->Bt848, DSTATUS) << 24);
			break;
															
		case BT848_GAMMA:
			if (config->gamma)
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xef) | 0x10);
			else
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xef));
			break;	
				
		case BT848_ERROR_DIFFUSION:
			if (config->error_diffusion)
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xdf));
			else
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xdf) | 0x20);
			break;	
				
		case BT848_LUMA_CORING:
			if (config->luma_coring)
				write_bt848_reg(pd->Bt848,OFORM,(read_bt848_reg(pd->Bt848, OFORM)&0x9f) | 0x40);
			else
				write_bt848_reg(pd->Bt848,OFORM,(read_bt848_reg(pd->Bt848, OFORM)&0x9f));
			break;	
				
		case BT848_LUMA_COMB:
			if (config->luma_comb)
			{
				write_bt848_reg(pd->Bt848,E_VSCALE_HI,(read_bt848_reg(pd->Bt848, E_VSCALE_HI)&0x7f) | 0x80);
				write_bt848_reg(pd->Bt848,O_VSCALE_HI,(read_bt848_reg(pd->Bt848, O_VSCALE_HI)&0x7f) | 0x80);
			}
			else
			{
				write_bt848_reg(pd->Bt848,E_VSCALE_HI,(read_bt848_reg(pd->Bt848, E_VSCALE_HI)&0x7f));
				write_bt848_reg(pd->Bt848,O_VSCALE_HI,(read_bt848_reg(pd->Bt848, O_VSCALE_HI)&0x7f));
			}
			break;	
				
		case BT848_CHROMA_COMB:
			if (config->chroma_comb)
			{
				write_bt848_reg(pd->Bt848,E_VSCALE_HI,(read_bt848_reg(pd->Bt848, E_VSCALE_HI)&0xbf) | 0x40);
				write_bt848_reg(pd->Bt848,O_VSCALE_HI,(read_bt848_reg(pd->Bt848, O_VSCALE_HI)&0xbf) | 0x40);
			}
			else
			{
				write_bt848_reg(pd->Bt848,E_VSCALE_HI,(read_bt848_reg(pd->Bt848, E_VSCALE_HI)&0xbf));
				write_bt848_reg(pd->Bt848,O_VSCALE_HI,(read_bt848_reg(pd->Bt848, O_VSCALE_HI)&0xbf));
			}
			break;	
				
		case BT848_COLOR_BARS:
			if (config->color_bars)
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xbf) | 0x40 );
			else
				write_bt848_reg(pd->Bt848,COLOR_CTL,(read_bt848_reg(pd->Bt848, COLOR_CTL)&0xbf));
			break;	
				
		case BT848_READ_I2C_REG:
			config->i2c_register = read_bt848_reg(pd->Bt848, I2C);
			break;
					
		case BT848_WRITE_I2C_REG:
			write_bt848_reg(pd->Bt848, I2C, config->i2c_register);
			break;		

		case BT848_VERSION:
			config->device_id = pd->deviceid;
			config->version = DRIVER_VERSION;
			break;
			
		case BT848_PLL:
			if (config->pll)
			{
				write_bt848_reg(pd->Bt848, PLL_XCI, 0x8e);
				write_bt848_reg(pd->Bt848, PLL_F_HI, 0xdc);
				write_bt848_reg(pd->Bt848, PLL_F_LO, 0xf9);
				write_bt848_reg(pd->Bt848, TGCTRL, 0x08);
			}
			else
			{
				write_bt848_reg(pd->Bt848, TGCTRL, 0x00);
				write_bt848_reg(pd->Bt848, PLL_XCI, 0x00);
				write_bt848_reg(pd->Bt848, PLL_F_HI, 0x00);
				write_bt848_reg(pd->Bt848, PLL_F_LO, 0x00);
			}
			break;	
				
		default:
			ddprintf(("bad ioctl\n"));
			return B_ERROR;
	}
	return B_NO_ERROR;
}

/*------------------------------------------------------------------------*/

const char **
publish_devices()
{
	return (const char **) Bt848_name;
}

/*------------------------------------------------------------------------*/

device_hooks	Bt848_device = 
{
	Bt848_open, 	/* -> open entry point */
	Bt848_close, 	/* -> close entry point */
	Bt848_free,		/* -> free entry point */
	Bt848_control, 	/* -> control entry point */
	Bt848_read,		/* -> read entry point */
	Bt848_write		/* -> write entry point */
};

/*------------------------------------------------------------------------*/

device_hooks*
find_device(const char* name)
{
	return &Bt848_device;
}

/*------------------------------------------------------------------------*/

static uint32 
read_bt848_reg(uint32 *bt848,uint32 reg) 
{

	uint32 value;
	
	value = bt848[reg/4];
	
	return(B_LENDIAN_TO_HOST_INT32(value));
}

/*------------------------------------------------------------------------*/

static void 
write_bt848_reg(uint32 *bt848,uint32 reg, uint32 value) 
{
			
	bt848[reg/4] = B_HOST_TO_LENDIAN_INT32(value);
	
	return;
}

/*------------------------------------------------------------------------*/

static uint8
get_swap(uint32 color_format)
{
	switch(color_format) 
	{
		case BT848_RGB32:	return 0x5;
		case BT848_RGB24:	return 0x0;
		case BT848_RGB16:
		case BT848_RGB15:
		case BT848_YUV422:	return 0x1;
		case BT848_YUV411:	return 0x1;
		case BT848_RGB8:
		case BT848_Y8:		return 0x0;
		default:			return 0x0;
	}
}

static void 
init_regs(uint32 *bt848,bt848_config *config) 
{
	uint32	source,format;
	uint32	control, e_control, o_control;
	uint32	vbi_samples;

	format = config->video_format & 0x07;
	write_bt848_reg(bt848, IFORM, read_bt848_reg(bt848, IFORM) | 0x18 | format);

	/*
	control setup	
	*/
	source = config->video_source;
	
	control = 0x80;							/* sync threshold hi, agc enabled, normal clock */
											/* enable luma adc, enable chroma adc */
											
	if (source < 4)							/* disable chroma A/D if composite */
		control = control + 0x02;
	
	write_bt848_reg(bt848,ADC,control);						
	
	o_control = read_bt848_reg(bt848,O_CONTROL) & 0xff;
	e_control = read_bt848_reg(bt848,E_CONTROL) & 0xff;

	if ( ((config->o_x_size <=320) & ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))) |
		 ((config->o_x_size <=360) & (format >= BT848_PAL_BDGHI)) )
		o_control |= 0x00;					/* enable luma decimation */
	else
		o_control |= 0x20;					/* disable luma decimation */
			
	if ( ((config->e_x_size <=320) & ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))) |
		 ((config->e_x_size <=360) & (format >= BT848_PAL_BDGHI)) )
		e_control |= 0x00;					/* enable luma decimation */
	else
		e_control |= 0x20;					/* disable luma decimation */
			
	if (source > 3)							/* enable chroma A/D if svideo */
	{				
		o_control |= 0x40;
		e_control |= 0x40;	
	}
													
	write_bt848_reg(bt848,O_CONTROL,o_control);
	write_bt848_reg(bt848,E_CONTROL,e_control);

	write_bt848_reg(bt848,O_SCLOOP,0x40);	/* automatic horizontal low pass filter */
	write_bt848_reg(bt848,E_SCLOOP,0x40);	/* chroma agc enabled, low color detection & removal disabled */												
												
	if (config->luma_coring)
		write_bt848_reg(bt848,OFORM,(read_bt848_reg(bt848, OFORM)&0x9f) | 0x40);
	else
		write_bt848_reg(bt848,OFORM,(read_bt848_reg(bt848, OFORM)&0x9f));
	
	write_bt848_reg(bt848,TDEC,config->decimate);

	if ((format == BT848_NTSC_M) | 
		(format == BT848_NTSC_JAPAN) | 
		(format == BT848_PAL_M)) 
	{
		write_bt848_reg(bt848,ADELAY,0x68);		/* NTSC agc delay */
		write_bt848_reg(bt848,BDELAY,0x5d);		/* NTSC burst delay */		
	}
	else 
	{
		if (format == BT848_SECAM)
		{
			write_bt848_reg(bt848,ADELAY,0x7f);		/* SECAM agc delay */
			write_bt848_reg(bt848,BDELAY,0xA0);		/* SECAM burst delay */
		}
		else
		{
			write_bt848_reg(bt848,ADELAY,0x7f);		/* PAL agc delay */
			write_bt848_reg(bt848,BDELAY,0x72);		/* PAL burst delay */
		}
	}

	if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))
		vbi_samples = B_NTSC_VBI_SAMPLES;
	else
		vbi_samples = B_PAL_VBI_SAMPLES;
		 
	write_bt848_reg(bt848,VBI_PACK_SIZE,(vbi_samples/4) & 0xff);		/* VBI capture: number words*/
	write_bt848_reg(bt848,VBI_PACK_DEL,(((vbi_samples/4)& 0x1ff) >> 8) +  (0 << 2));

	if (config->luma_comb)
	{
		write_bt848_reg(bt848,E_VSCALE_HI,(read_bt848_reg(bt848, E_VSCALE_HI)&0x7f) | 0x80);
		write_bt848_reg(bt848,O_VSCALE_HI,(read_bt848_reg(bt848, O_VSCALE_HI)&0x7f) | 0x80);
	}
	else
	{
		write_bt848_reg(bt848,E_VSCALE_HI,(read_bt848_reg(bt848, E_VSCALE_HI)&0x7f));
		write_bt848_reg(bt848,O_VSCALE_HI,(read_bt848_reg(bt848, O_VSCALE_HI)&0x7f));
	}

	if (config->chroma_comb)
	{
		write_bt848_reg(bt848,E_VSCALE_HI,(read_bt848_reg(bt848, E_VSCALE_HI)&0xbf) | 0x40);
		write_bt848_reg(bt848,O_VSCALE_HI,(read_bt848_reg(bt848, O_VSCALE_HI)&0xbf) | 0x40);
	}
	else
	{
		write_bt848_reg(bt848,E_VSCALE_HI,(read_bt848_reg(bt848, E_VSCALE_HI)&0xbf));
		write_bt848_reg(bt848,O_VSCALE_HI,(read_bt848_reg(bt848, O_VSCALE_HI)&0xbf));
	}
	
	/*write_bt848_reg(bt848,INT_MASK,0x000fb000 | BT848_RISC_IRQ); */	/* enable RISC interrupt PICKY DRIVER*/
	write_bt848_reg(bt848,INT_MASK,0x000f0000 | BT848_RISC_IRQ);		/* enable RISC interrupt */
	write_bt848_reg(bt848,INT_STAT,0xfffff);							/* clear any existing interrupts */
}

static void
update_state(uint32 *bt848, bt848_state *state) 
{
	write_bt848_reg(bt848, O_HACTIVE_LO, state->o_hactive_lo);
	write_bt848_reg(bt848, E_HACTIVE_LO, state->e_hactive_lo);
	write_bt848_reg(bt848, O_CROP, state->o_crop);
	write_bt848_reg(bt848, E_CROP, state->e_crop);
	/* write hi byte of hscale */
	write_bt848_reg(bt848, O_HSCALE_HI, state->o_hscale_hi);
	write_bt848_reg(bt848, E_HSCALE_HI, state->e_hscale_hi);
	/* write lo byte of hscale */
	write_bt848_reg(bt848, O_HSCALE_LO, state->o_hscale_lo);
	write_bt848_reg(bt848, E_HSCALE_LO, state->e_hscale_lo);
	/* write lo byte of hdelay */
	write_bt848_reg(bt848, O_HDELAY_LO, state->o_hdelay_lo);
	write_bt848_reg(bt848, E_HDELAY_LO, state->e_hdelay_lo);
 	/* write lo bits of vactive */
	write_bt848_reg(bt848, O_VACTIVE_LO, state->o_vactive_lo);
 	write_bt848_reg(bt848, E_VACTIVE_LO, state->e_vactive_lo);
	/* write lo bits of vdelay (make sure the value is even) */
	write_bt848_reg(bt848, O_VDELAY_LO, state->o_vdelay_lo);
	write_bt848_reg(bt848, E_VDELAY_LO, state->e_vdelay_lo);

	write_bt848_reg(bt848, O_VSCALE_HI,
		(read_bt848_reg(bt848, O_VSCALE_HI) & 0xe0) |
		(state->o_vscale_hi & 0x1f));
	write_bt848_reg(bt848, E_VSCALE_HI,
		(read_bt848_reg(bt848, E_VSCALE_HI) & 0xe0) |
		(state->e_vscale_hi & 0x1f));
	write_bt848_reg(bt848, O_VSCALE_LO, state->o_vscale_lo);
	write_bt848_reg(bt848, E_VSCALE_LO, state->e_vscale_lo);

	write_bt848_reg(bt848, O_VTC, state->o_vtc);
	write_bt848_reg(bt848, E_VTC, state->e_vtc);

	write_bt848_reg(bt848, COLOR_FMT, state->color_fmt);
	write_bt848_reg(bt848, COLOR_CTL,
		(read_bt848_reg(bt848, COLOR_CTL)&0xf0) | (state->color_ctl & 0x0f));
}

static void 
init_state(bt848_state *state, bt848_config *config) 
{
	uint32	e_hactive, o_hactive, e_htotal, o_htotal;
	uint32	e_hscale, o_hscale, e_hdelay, o_hdelay, vdelay, vactive;
	uint32	e_vscale, o_vscale;
	uint32	format, color_format;
	uint32	e_control, o_control;
	uint32	e_swap, o_swap;

	format = config->video_format & 0x07;

	/* 
	horizontal configuration 
	*/
	
	o_hactive = config->o_x_size & 0x3ff;
	e_hactive = config->e_x_size & 0x3ff;
	/* write lo byte of hactive */
	state->o_hactive_lo = o_hactive & 0xff;
	state->e_hactive_lo = e_hactive & 0xff;
	/* write hi bits of hactive */
	state->o_crop = (state->o_crop & 0xfc) + (o_hactive >> 8);
	state->e_crop = (state->e_crop & 0xfc) + (e_hactive >> 8);

	if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))
	{
		o_htotal = (o_hactive * 1000)/800;
		e_htotal = (e_hactive * 1000)/800;
	}
	else
	{
		o_htotal = (o_hactive * 1000)/790;
		e_htotal = (e_hactive * 1000)/790;
	}	
	
	if (o_htotal != 0)
	{
		if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN)) 
			o_hscale = ((910*4096)/o_htotal)-4096;
		else 
			o_hscale = ((1135*4096)/o_htotal)-4096;
	}
	else
	{
		ddprintf(("zero length odd x length!\n"));
		return;
	}
					
	if (e_htotal != 0)
	{
		if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN)) 
			e_hscale = ((910*4096)/e_htotal)-4096;
		else 
			e_hscale = ((1135*4096)/e_htotal)-4096;
	}
	else
	{
		ddprintf(("zero length even x length!\n"));
		return;
	}
	
	/* write hi byte of hscale */
	state->o_hscale_hi = (o_hscale & 0xff00) >> 8;
	state->e_hscale_hi = (e_hscale & 0xff00) >> 8;
	/* write lo byte of hscale */
	state->o_hscale_lo = o_hscale & 0xff;
	state->e_hscale_lo = e_hscale & 0xff;

	if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN)) 
	{
		o_hdelay = ((o_hactive*156)/754 + config->h_delay_adj) & 0x3fe;
		e_hdelay = ((e_hactive*156)/754 + config->h_delay_adj) & 0x3fe;	
	}
	else 
	{
		o_hdelay = ((o_hactive*213)/922 + config->h_delay_adj) & 0x3fe;
		e_hdelay = ((e_hactive*213)/922 + config->h_delay_adj) & 0x3fe;	
	}

	/* write lo byte of hdelay */
	state->o_hdelay_lo = o_hdelay & 0xff;
	state->e_hdelay_lo = e_hdelay & 0xff;
	/* write hi bits of hdelay */
	state->o_crop = (state->o_crop & 0xf3) + ((o_hdelay & 0x300) >> 6);
	state->e_crop = (state->e_crop & 0xf3) + ((e_hdelay & 0x300) >> 6);
	
	/* 
	vertical configuration 
	*/

	if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))
		vactive = 480;
	else
		vactive = 576;
 
 	/* write lo bits of vactive */
	state->o_vactive_lo = vactive & 0xff;
	state->e_vactive_lo = vactive & 0xff;
	/* write hi bits of vactive */
	state->o_crop = (state->o_crop & 0xcf) + ((vactive & 0x300) >> 4);
	state->e_crop = (state->e_crop & 0xcf) + ((vactive & 0x300) >> 4);

	if ((format == BT848_NTSC_M) | (format == BT848_NTSC_JAPAN))
		vdelay = NTSC_VDELAY + config->v_delay_adj;
	else
		vdelay = PAL_VDELAY + config->v_delay_adj;

	/* write lo bits of vdelay (make sure the value is even) */
	state->o_vdelay_lo = vdelay & 0xfe;
	state->e_vdelay_lo = vdelay & 0xfe;
	/* write hi bits of vdelay */
	state->e_crop = (state->e_crop & 0x3f) + ((vdelay & 0x300) >> 2);
	state->o_crop = (state->o_crop & 0x3f) + ((vdelay & 0x300) >> 2);
	
	/* vertical scaling */

	if (config->o_y_size == 0)
	{
		ddprintf(("zero length odd y size\n"));
		return;
	}
		
	if (config->o_y_size > vactive/2) 
	{	/* if > CIF use both fields for scaling */
		o_vscale = (0x10000 - ((vactive * 512)/config->o_y_size - 512))&0x1fff;
		o_control = 0x20;	/* set Interlaced bit */
	}
	else 
	{	/* if <= CIF use single fields for scaling */
		o_vscale = (0x10000 - ((vactive * 256)/config->o_y_size - 512))&0x1fff;
		o_control = 0x00;	/* clear interlaced bit */
	}
	
	if (config->e_y_size == 0)
	{
		ddprintf(("zero length even y size\n"));
		return;
	}

	if (config->e_y_size > vactive/2) 
	{	/* if > CIF use both fields for scaling */
		e_vscale = (0x10000 - ((vactive * 512)/config->e_y_size - 512))&0x1fff;
		e_control = 0x20;	/* set Interlaced bit */
	}
	else 
	{	/* if <= CIF use single fields for scaling */
		e_vscale = (0x10000 - ((vactive * 256)/config->e_y_size - 512))&0x1fff;
		e_control = 0x00;	/* clear interlaced bit */
	}
	
	state->o_vscale_hi = o_control + (o_vscale >> 8);
	state->e_vscale_hi = e_control + (e_vscale >> 8);
	state->o_vscale_lo = o_vscale & 0xff;
	state->e_vscale_lo = e_vscale & 0xff;

	if ( config->e_x_size > 360)
		e_control = 0x00;				/* 2 tap interpolation */
	else 
	{
		if ( config->e_x_size > 180)
			e_control = 0x01;			/* 3 tap interpolation */
		else
			e_control = 0x02;			/* 4 tap interpolation */
	}
	state->e_vtc = e_control;
	
	if ( config->o_x_size > 360)
		o_control = 0x00;				/* 2 tap interpolation */
	else 
	{
		if ( config->o_x_size > 180)
			o_control = 0x01;			/* 3 tap interpolation */
		else
			o_control = 0x02;			/* 4 tap interpolation */
	}		
	state->o_vtc = o_control;

	/* set color format */
	color_format = config->e_color_format | (config->o_color_format << 4);
	state->color_fmt = color_format & 0xff;

	e_swap = config->e_endian == BT848_BIG ? get_swap(config->e_color_format) : 0;
	o_swap = config->o_endian == BT848_BIG ? get_swap(config->o_color_format) : 0;
	
	state->color_ctl = (o_swap << 1) | e_swap;
}

/*------------------------------------------------------------------------*/

typedef struct {
	uint32	*start;
	uint32	*end;
	uint32	*max_end;
	uint32	pstart;
	bool	failure;
} risc_prog_t;

static status_t
clear_risc(risc_prog_t *risc_prog, uint32 *start, uint32 *pstart, uint32 size)
{
	risc_prog->start = start;
	risc_prog->end = start;
	risc_prog->max_end = start+size;
	risc_prog->pstart = (uint32)pstart;
	risc_prog->failure = false;
	return B_NO_ERROR;
}

static status_t
add_risc(risc_prog_t *risc_prog, uint32 word)
{
	if(risc_prog->end >= risc_prog->max_end) {
		ddprintf(("bt848: add_risc program buffer full\n"));
		risc_prog->failure = true;
		return B_ERROR;
	}
	*risc_prog->end = B_HOST_TO_LENDIAN_INT32(word);
	risc_prog->end++;
	return B_NO_ERROR;
}

static status_t
add_sync(risc_prog_t *risc_prog, bool odd)
{
	if(risc_prog->end + 4 > risc_prog->max_end) {
		risc_prog->failure = true;
		return B_ERROR;
	}

	/* SYNC, disable fdsr errors, VRESET following even/odd field */
	add_risc(risc_prog, SYNC | RESYNC | (odd ? VRE : VRO));
	/* reserved word, no function */
	add_risc(risc_prog, 0);
	return B_NO_ERROR;
}

static status_t
add_vbi(risc_prog_t *risc_prog, void **vbi, int lines, int vbi_samples)
{
	int i;
	status_t err;
	
	if(vbi == NULL) {
		risc_prog->failure = true;
		return B_ERROR;
	}

	/* SYNC, disable fdsr errors, FIFO mode packed data to follow */
	add_risc(risc_prog, SYNC | RESYNC | FM1);
	/* reserved word, no function */
	err = add_risc(risc_prog, 0);

	for (i = 0; i < lines; i++) 
	{
		physical_entry table[1];

		get_memory_map(vbi[i], vbi_samples, table, 1);
		if (table[0].size != vbi_samples) {
			ddprintf(("bt848 vbi: logical = %p, physical = %p, "
			          "size %ld != %d\n", vbi[i], table[0].address,
			          table[0].size, vbi_samples));
			risc_prog->failure = true;
			return B_ERROR;
		}

		add_risc(risc_prog, WRITE | SOL | EOL | vbi_samples);
		err = add_risc(risc_prog, (uint32)RAM_ADDRESS(table[0].address));
	}
	return err;
}

#if 1

typedef struct {
	physical_entry table[16];
	int currindex;
	uint32	paddr;
	uint32	laddr;
	uint32	size;
} address_table_t;

static status_t
get_next_addr(address_table_t *t, uint32 laddr, uint32 *paddr, uint32 *size,
              uint32 maxsize)
{
	int i;
	//dprintf("get_next_addr t = %p laddr 0x%08lx paddrp %p sizep %p maxsize %ld\n",
	//	t, laddr, paddr, size, maxsize);
	//dprintf("get_next_addr size %ld\n", *size);
	if(maxsize < *size) {
		ddprintf(("bt848 get_next_addr size %ld > msize %ld\n",
		          *size, maxsize));
		return B_ERROR;
	}
	if(laddr > t->laddr) {
		uint32 offset = laddr - t->laddr;
		if(offset >= t->size) {
			t->size = 0;
		}
		else {
			t->size -= offset;
			t->laddr += offset;
			while(t->table[t->currindex].size <= offset) {
				if(t->currindex >= 15) {
					ddprintf(("bt848 internal error in get_next_addr rem size "
					          "%ld + %ld\n", t->size,offset));
					return B_ERROR;
				}
				offset -= t->table[t->currindex].size;
				t->currindex++;
				t->paddr = (uint32)RAM_ADDRESS(t->table[t->currindex].address);
			}
			t->table[t->currindex].size -= offset;
			t->paddr += offset;
		}
	}
	if(t->size == 0) {
		if(laddr < 0x80000000) {
			ddprintf(("bt848 bad pixel buffer 0x%08lx\n", laddr));
			return B_ERROR;
		}
		get_memory_map((void*)laddr, maxsize, t->table, 16);
		if(t->table[0].size < *size &&
		   t->table[0].size + t->table[1].size < *size) {
			ddprintf(("bt848 odd: logical = 0x%08lx, physical = %p, "
			          "size %ld < %ld\n", laddr, t->table[0].address,
			          t->table[0].size, *size));
			return B_ERROR;
		}
		t->laddr = laddr;
		t->paddr = (uint32)RAM_ADDRESS(t->table[0].address);
		t->currindex = 0;
		t->size = 0;
		for(i=0; i<16; i++) {
			if(t->table[i].size == 0)
				break;
			t->size += t->table[i].size;
		}
		if(t->size > maxsize) {
			ddprintf(("bt848 memmap size %ld > maxsize %ld\n",
			          t->size, maxsize));
			return B_ERROR;
		}
		//ddprintf(("bt848 logical = 0x%08lx, physical = 0x%08lx, size %ld of %ld\n",
		//         laddr, t->paddr, t->table[0].size, t->size));
	}
	//ddprintf(("t->size = %d\n", t->size));
	if(t->table[t->currindex].size > t->size) {
		ddprintf(("bt848 internal error in get_next_addr rem size %ld < curr "
		          "%ld\n", t->size, t->table[t->currindex].size));
		return B_ERROR;
	}

	if(t->table[t->currindex].size <= *size) {
		*size = t->table[t->currindex].size;
		*paddr = t->paddr;
		t->currindex++;
		t->paddr = (uint32)RAM_ADDRESS(t->table[t->currindex].address);
	}
	else {
		t->table[t->currindex].size -= *size;
		*paddr = t->paddr;
		t->paddr += *size;
	}
	if(*size > t->size) {
		ddprintf(("bt848 internal error in get_next_addr rem size %ld < %ld\n",
		          t->size,*size));
		return B_ERROR;
	}
	if(*size == 0) {
		ddprintf(("bt848 get_next_addr returning 0, remsize %ld\n", t->size));
		return B_ERROR;
	}
	t->size -= *size;
	//ddprintf(("t->size = %d\n", t->size));
	t->laddr += *size;
	//dprintf("get_next_addr laddr 0x%08lx -> paddrp 0x%08lx size %ld\n",
	//        laddr, *paddr, *size);

	return B_NO_ERROR;
}

static status_t
add_packed(risc_prog_t *risc_prog, int x_size, int y_size, int bits_per_pixel,
           void **lines, uint16 address_type, int16 **clips)
{
	int i;
	status_t err = B_NO_ERROR;
	uint32 firstladdr = (uint32)lines[0];
	uint32 lastladdr = (uint32)lines[y_size-1]+bits_per_pixel*x_size/8;

	//bigtime_t t1 = system_time();

	address_table_t table;

	table.laddr = 0;
	table.paddr = 0;
	if(address_type == BT848_LOGICAL) {
		table.size = 0;
	}
	else if(address_type == BT848_PHYSICAL) {
		table.size = ~0;
		table.currindex = 0;
		table.table[0].size = ~0;
	}
	else {
		ddprintf(("bt848 bad address type\n"));
		risc_prog->failure = true;
		return B_ERROR;
	}

	/* SYNC, disable fdsr errors, FIFO mode packed data to follow */
	add_risc(risc_prog, SYNC | RESYNC | FM1);
	/* reserved word, no function */
	err = add_risc(risc_prog, 0);

	for(i=0; i < y_size; i++) {
		int byte_count = (x_size * bits_per_pixel + 7)/8;
		uint32 laddr = (uint32)lines[i];
		uint32 paddr = 0;
		int16 *clip = clips[i];
		int j = 0;
		uint32 sol_flag = SOL;
		uint32 eol_flag = 0;
		
		if(laddr < firstladdr || laddr >= lastladdr) {
			ddprintf(("bt848 bad line address: lines has to be in order\n"));
			risc_prog->failure = true;
			return B_ERROR;
		}
		
		//bool current_target_valid = false;
		
		//if(i > 10) {
		//	risc_prog->failure = true;
		//	return err;
		//}

		while(clip[j] != 0) {
			int clip_bytes = (abs(clip[j]) * bits_per_pixel + 7)/8;
			byte_count -= clip_bytes;
			if(byte_count <= 0)
				eol_flag = EOL;
			if(byte_count < 0) {
				int k=0;
				ddprintf(("bt848 bad clip: wider than line\n"));
				ddprintf(("line %d clip: \n", i));
				while(k<j) {
					ddprintf(("%d ", clip[k]));
					k++;
				}
				ddprintf((": %d : ", clip[k]));
				k++;
				while(clip[k] != 0) {
					ddprintf(("%d ", clip[k]));
					k++;
				}
				ddprintf(("\n"));
				clip_bytes += byte_count;
				byte_count = 0;
			}
			if(clip[j] > 0) {
				uint32 oldpaddr = paddr;
				uint32 psize = clip_bytes;
				err = get_next_addr(&table, laddr, &paddr, &psize,
				                    lastladdr-laddr);
				if(err != B_NO_ERROR) {
					risc_prog->failure = true;
					return err;
				}
				
				if(clip_bytes <= psize) {
					if(oldpaddr == paddr) {
						//dprintf("bt848: WRITEC %d (@ 0x%08x), sol %d eol %d\n",
						//        clip_bytes, paddr, sol_flag, eol_flag);
						err = add_risc(risc_prog, WRITEC | eol_flag | clip_bytes);
					}
					else {
						//dprintf("bt848: WRITE %d @ 0x%08x, sol %d eol %d\n",
						//        clip_bytes, paddr, sol_flag, eol_flag);
						err = add_risc(risc_prog, WRITE | sol_flag | eol_flag | clip_bytes);
						err = add_risc(risc_prog, paddr);
					}
					paddr += clip_bytes;
					laddr += clip_bytes;
				}
				else {
					if(oldpaddr == paddr) {
						//dprintf("bt848: line %d WRITEC %d (@ 0x%08x), sol %d\n",
						//        i, psize, paddr, sol_flag);
						err = add_risc(risc_prog, WRITEC | psize);
					}
					else {
						//dprintf("bt848: line %d WRITE %d @ 0x%08x, sol %d\n",
						//        i, psize, paddr, sol_flag);
						err = add_risc(risc_prog, WRITE | sol_flag | psize);
						err = add_risc(risc_prog, paddr);
					}
					sol_flag = 0;
					paddr += psize;
					laddr += psize;

					psize = clip_bytes-psize;
					err = get_next_addr(&table, laddr, &paddr, &psize,
					                    lastladdr-laddr);
					if(err != B_NO_ERROR) {
						risc_prog->failure = true;
						return err;
					}
					//dprintf("bt848: split buffer at %p, size %d\n", paddr, psize);

					//dprintf("bt848: line %d WRITE %d @ 0x%08x, sol %d eol %d\n",
					//        i, psize, paddr, sol_flag, eol_flag);

					err = add_risc(risc_prog, WRITE | sol_flag | eol_flag |
					                          psize);
					err = add_risc(risc_prog, paddr);
					paddr += psize;
					laddr += psize;
				}
			}
			else {
				err = add_risc(risc_prog, SKIP | sol_flag | eol_flag | clip_bytes);
				laddr += clip_bytes;
				paddr += clip_bytes;
			}
			sol_flag = 0;
			if (eol_flag == EOL)
				break;
			j++;
		}
	}
	//dprintf("bt848: %Ld us to create prog (get_memory_map: %Ld)\n",
	//        system_time()-t1, memmaptime);
	return err;
}

#else

static status_t
add_packed(risc_prog_t *risc_prog, int x_size, int y_size, int bits_per_pixel,
           void **lines, uint16 address_type, int16 **clips)
{
	int i;
	status_t err = B_NO_ERROR;

	bigtime_t t1 = system_time();
	bigtime_t memmaptime = 0;

	/* SYNC, disable fdsr errors, FIFO mode packed data to follow */
	add_risc(risc_prog, SYNC | RESYNC | FM1);
	/* reserved word, no function */
	err = add_risc(risc_prog, 0);

	for(i=0; i < y_size; i++) {
		int byte_count = (x_size * bits_per_pixel + 7)/8;
		int16 *clip = clips[i];
		int j = 0;
		uint32 sol_flag = SOL;
		uint32 eol_flag = 0;
		uint32	paddr;
		uint32	psize;
		bool current_target_valid = false;

		physical_entry table[2];

		if(address_type == BT848_LOGICAL) {
			if(lines[i] < (void*)0x80000000) {
				ddprintf(("bt848 bad pixel buffer %p\n", lines[i]));
				risc_prog->failure = true;
				return B_ERROR;
			}
			{
			bigtime_t mt1 = system_time();
			get_memory_map(lines[i], byte_count, table, 2);
			memmaptime += system_time() - mt1;
			}
			if(table[0].size != byte_count &&
			   table[0].size + table[1].size != byte_count) {
				ddprintf(("bt848 odd: logical = %p, physical = %p, "
				          "size %ld != %d\n", lines[i], table[0].address,
				          table[0].size, byte_count));
				risc_prog->failure = true;
				return B_ERROR;
			}
			paddr = (uint32)RAM_ADDRESS(table[0].address);
			psize = table[0].size;
			if (i == 0) {
				//ddprintf(("bt848 odd: logical = %p, physical = %08x\n",
				//          lines[i], paddr));
			}
		}
		else {
			paddr = (uint32)lines[i];
			psize = byte_count;
		}

		while(clip[j] != 0) {
			int clip_bytes = (abs(clip[j]) * bits_per_pixel + 7)/8;
			byte_count -= clip_bytes;
			if(byte_count <= 0)
				eol_flag = EOL;
			if(byte_count < 0) {
				ddprintf(("bt848 bad clip: wider than line\n"));
				clip_bytes += byte_count;
				byte_count = 0;
			}
			if(clip[j] > 0) {
				if(clip_bytes <= psize) {
					if(current_target_valid) {
						//dprintf("bt848: WRITEC %d\n", clip_bytes);
						err = add_risc(risc_prog, WRITEC | eol_flag | clip_bytes);
					}
					else {
						err = add_risc(risc_prog, WRITE | sol_flag | eol_flag | clip_bytes);
						err = add_risc(risc_prog, paddr);
						current_target_valid = true;
					}
					paddr += clip_bytes;
					psize -= clip_bytes;
				}
				else {
					if(psize > 0) {
						if(current_target_valid) {
							//dprintf("bt848: WRITEC %d\n", psize);
							err = add_risc(risc_prog, WRITEC | psize);
						}
						else {
							err = add_risc(risc_prog, WRITE | sol_flag | psize);
							err = add_risc(risc_prog, paddr);
						}
						sol_flag = 0;
					}
					//dprintf("bt848: split buffer at %p, size %d\n", paddr, psize);
					paddr = (uint32)RAM_ADDRESS(table[1].address);
					err = add_risc(risc_prog, WRITE | sol_flag | eol_flag |
					                          (clip_bytes-psize));
					err = add_risc(risc_prog, paddr);
					current_target_valid = true;
					paddr += clip_bytes - psize;
					psize = table[1].size - (clip_bytes - psize);
				}
			}
			else {
				err = add_risc(risc_prog, SKIP | sol_flag | eol_flag | clip_bytes);
				if(clip_bytes > psize) {
					paddr = (uint32)RAM_ADDRESS(table[1].address) - psize + clip_bytes;
					psize = table[1].size - (clip_bytes - psize);
					current_target_valid = false;
				}
				else {
					paddr += clip_bytes;
					psize -= clip_bytes;
				}
			}
			sol_flag = 0;
			if (eol_flag == EOL)
				break;
			j++;
		}
	}
	dprintf("bt848: %Ld us to create prog (get_memory_map: %Ld)\n",
	        system_time()-t1, memmaptime);
	return err;
}
#endif
static status_t
add_end(risc_prog_t *risc_prog)
{
	status_t err;
	add_risc(risc_prog, JUMP | IRQ);				/* JUMP to halt */
	add_risc(risc_prog, (uint32)(risc_prog->end + 1) - (uint32)risc_prog->start
	                          + risc_prog->pstart);
	add_risc(risc_prog, 0);		/* reserved opcode -> halt */
#if 0
	add_risc(risc_prog, SYNC | RESYNC | VRE);	/* Generate Interrupt, use SYNC to reduce bus traffic */
	add_risc(risc_prog, 0);							/* over just a JUMP to self with IRQ */
	add_risc(risc_prog, JUMP);				/* JUMP to SYNC with INT set */
	err = add_risc(risc_prog, (uint32)(risc_prog->end - 3)
	                          - (uint32)risc_prog->start
	                          + risc_prog->pstart);
#endif
	return err;
}

static void 
create_risc(bool primary, uint32 buffer_num, uint32 *bt848, priv_data *pd, bt848_config *config)
{
	int32 			i, vactive, vdelay, capctl;
	int16			e_no_clip[2], o_no_clip[2];
	bt848_cliplist	e_clip,o_clip;
	risc_prog_t		risc_prog[1];
	uint32			capture_mode = 0, vbi_samples;
	int32			e_bits_per_pixel, o_bits_per_pixel;
	uint32			*risc_phys_memory;
	uint32			*risc_log_memory;

	pd->error = 0;
	
	if (primary)
	{
		risc_phys_memory = pd->pri_risc_phys_memory[buffer_num];
		risc_log_memory =  pd->pri_risc_log_memory[buffer_num];
	}
	else
	{
		risc_phys_memory = pd->alt_risc_phys_memory[buffer_num];
		risc_log_memory =  pd->alt_risc_log_memory[buffer_num];
	}
	
	capctl = 0;
	if ((config->video_format == BT848_NTSC_M) | (config->video_format == BT848_NTSC_JAPAN))
	{
		vactive = 480;
		vdelay = NTSC_VDELAY;
		vbi_samples = B_NTSC_VBI_SAMPLES;
	}
	else
	{
		vactive = 576;
		vdelay = PAL_VDELAY;
		vbi_samples = B_PAL_VBI_SAMPLES;
	}
		
	/* determine size of pixels */
	
	switch(config->o_color_format) 
	{
		case BT848_RGB32:
			o_bits_per_pixel = 32;
			break;
		case BT848_RGB24:
			o_bits_per_pixel = 24;
			break;
		case BT848_RGB16:
		case BT848_RGB15:
		case BT848_YUV422:
			o_bits_per_pixel = 16;
			break;
		case BT848_YUV411:
			o_bits_per_pixel = 12;
			break;
		case BT848_RGB8:
		case BT848_Y8:
			o_bits_per_pixel = 8;
			break;
		default:
			o_bits_per_pixel = 8;
			pd->error = 1;
			break;
	}
	
	switch(config->e_color_format) 
	{
		case BT848_RGB32:
			e_bits_per_pixel = 32;
			break;
		case BT848_RGB24:
			e_bits_per_pixel = 24;
			break;
		case BT848_RGB16:
		case BT848_RGB15:
		case BT848_YUV422:
			e_bits_per_pixel = 16;
			break;
		case BT848_YUV411:
			e_bits_per_pixel = 12;
			break;
		case BT848_RGB8:
		case BT848_Y8:
			e_bits_per_pixel = 8;
			break;
		default:
			e_bits_per_pixel = 8;
			pd->error = 1;
			break;
	}
	
	/* if o_line is not null, build array of odd scan line physical pointers */

	if (config->o_line != 0) 
	{
		/* enable capture of odd lines */
		capctl = capctl | 0x03;

		/* if o_clip is a null, no clipping is desired, build simple clip list */
		if (config->o_clip == 0) 
		{
			o_no_clip[0] = config->o_x_size;
			o_no_clip[1] = 0;
			for (i=0; i < config->o_y_size; i++) 
			{
				o_clip[i] = o_no_clip;
			}		
		}
		else 
		{
			for (i=0; i < config->o_y_size; i++) 
			{
				o_clip[i] = (short *)((void **)config->o_clip)[i];
			}		
		}
		
	}
	
	/* if e_line is not null, build array of even scan line physical pointers */
	if (config->e_line != 0) 
	{
		/* enable capture of even lines */
		capctl = capctl | 0x03;

		/* if e_clip is a null, no clipping is desired, build simple clip list */
		if (config->e_clip == 0) 
		{
			e_no_clip[0] = config->e_x_size;
			e_no_clip[1] = 0;
			for (i=0; i < config->e_y_size; i++) 
			{
				e_clip[i] = e_no_clip;
			}		
		}
		else 
		{
			for (i=0; i < config->e_y_size; i++) 
			{
				e_clip[i] = (short *)((void **)config->e_clip)[i];
			}		
		}
	
	}

	/* build array of odd field vbi info pointers */
	if (config->o_vbi != 0) 
	{
		/* enable capture of odd vbi info */
		capctl = capctl | 0x08;
	}
	
	/* build array of even field vbi info pointers */
	if (config->e_vbi != 0) 
	{
		/* enable capture of even vbi info */
		capctl = capctl | 0x04;
	}
	
	if (primary)
		pd->pri_capctl = capctl;
	else
		pd->alt_capctl = capctl;

	/* determine capture mode */
	
	switch (config->capture_mode) 
	{
		case BT848_CONTINUOUS:
			pd->mode = BT848_CONTINUOUS;
			if ((config->e_line != 0) &
				(config->o_line != 0))
				capture_mode = CONTINUOUS_FRAME;
			else 
			{
				if ((config->e_line != 0) &
					(config->o_line == 0))
					capture_mode = CONTINUOUS_EVEN;
				else 
				{
					if ((config->e_line == 0) &
						(config->o_line != 0))
						capture_mode = CONTINUOUS_ODD;
					else
						pd->error = 1;
				}
			}
			break;
		case BT848_SINGLE:
			pd->mode = BT848_SINGLE;
			if ((config->e_line != 0) &
				(config->o_line != 0))
				capture_mode = SINGLE_FRAME;
			else 
			{
				if ((config->e_line != 0) &
					(config->o_line == 0))
					capture_mode = SINGLE_FIELD_EVEN;
				else 
				{
					if ((config->e_line == 0) &
						(config->o_line != 0))
						capture_mode = SINGLE_FIELD_ODD;
					else
						pd->error = 1;
				}
			}
			break;
		default:
			pd->error = 1;;
			break;		
	}
	
	/* bail out if we had an error prior to this point */
	if (pd->error == 1)
		return;
	 
	switch(capture_mode) 
	{
		case CONTINUOUS_FRAME:
		case SINGLE_FRAME:
			clear_risc(risc_prog, risc_log_memory, risc_phys_memory, CODE_SIZE/4);
			add_sync(risc_prog, true); /* odd field */

			if(config->o_vbi)
				add_vbi(risc_prog, config->o_vbi[buffer_num],
				        vdelay-10, vbi_samples);
			
			add_packed(risc_prog, config->o_x_size, config->o_y_size,
			           o_bits_per_pixel,
			           config->o_line[buffer_num], config->o_address_type,
			           o_clip);

#if 0
			add_risc(risc_prog, JUMP);					/* JUMP to address in next word */
			add_risc(risc_prog, (uint32)(risc_phys_memory + CODE_SIZE/4));	/* Start of even field program (PCI address) */
			
			/* *risc_prog++=B_HOST_TO_LENDIAN_INT32(JUMP | IRQ); */		/* JUMP to self with INT set */
			/* *risc_prog = B_HOST_TO_LENDIAN_INT32((uint32)((risc_prog-1) - risc_log_memory + risc_phys_memory)); */

			if(risc_prog->failure)
				pd->error = true;
			
			clear_risc(risc_prog, risc_log_memory + CODE_SIZE/4,
			           risc_phys_memory + CODE_SIZE/4, CODE_SIZE/4);
#endif
			add_sync(risc_prog, false); /* even field */

			if(config->e_vbi != NULL)
				add_vbi(risc_prog, config->e_vbi[buffer_num],
				        vdelay-10, vbi_samples);

			add_packed(risc_prog, config->e_x_size, config->e_y_size,
			           e_bits_per_pixel,
			           config->e_line[buffer_num], config->e_address_type,
			           e_clip);

			/* *risc_prog++=B_HOST_TO_LENDIAN_INT32(JUMP | IRQ);			/ *JUMP to Start of even field program */
			/* *risc_prog = B_HOST_TO_LENDIAN_INT32((uint32)(risc_phys_memory+2)); */
			
			add_end(risc_prog);

			if(risc_prog->failure)
				pd->error = true;

			break;
		
		case CONTINUOUS_EVEN:
		case SINGLE_FIELD_EVEN:
			add_sync(risc_prog, false); /* even field */

			if(config->e_vbi != NULL)
				add_vbi(risc_prog, config->e_vbi[buffer_num],
				        vdelay-10, vbi_samples);

			add_packed(risc_prog, config->e_x_size, config->e_y_size,
			           e_bits_per_pixel,
			           config->e_line[buffer_num], config->e_address_type,
			           e_clip);

			if (config->o_vbi != NULL) {
				add_sync(risc_prog, true); /* odd field */
				add_vbi(risc_prog, config->o_vbi[buffer_num],
				        vdelay-10, vbi_samples);
			}

			/* *risc_prog++=B_HOST_TO_LENDIAN_INT32(JUMP); */			/* JUMP to start */
			/* *risc_prog = B_HOST_TO_LENDIAN_INT32((uint32)(risc_phys_memory + 2)); */
			
			add_end(risc_prog);

			if(risc_prog->failure)
				pd->error = true;

			break;

		case CONTINUOUS_ODD:
		case SINGLE_FIELD_ODD:
			clear_risc(risc_prog, risc_log_memory, risc_phys_memory, CODE_SIZE/4);
			add_sync(risc_prog, true); /* odd field */

			if(config->o_vbi)
				add_vbi(risc_prog, config->o_vbi[buffer_num],
				        vdelay-10, vbi_samples);
			
			add_packed(risc_prog, config->o_x_size, config->o_y_size,
			           o_bits_per_pixel,
			           config->o_line[buffer_num], config->o_address_type,
			           o_clip);

			if (config->e_vbi != NULL) {
				add_sync(risc_prog, false); /* even field */
				add_vbi(risc_prog, config->e_vbi[buffer_num],
				        vdelay-10, vbi_samples);
			}

			/* *risc_prog++=B_HOST_TO_LENDIAN_INT32(JUMP); */			/* JUMP to start */
			/* *risc_prog = B_HOST_TO_LENDIAN_INT32((uint32)(risc_phys_memory + 2)); */
			
			add_end(risc_prog);

			if(risc_prog->failure)
				pd->error = true;

			break;
			
		default:
			clear_risc(risc_prog, risc_log_memory, risc_phys_memory, CODE_SIZE/4);
			add_end(risc_prog);
	}
}

/*------------------------------------------------------------------------*/

static void 
run_risc(bool primary, priv_data *pd)
{
	cpu_status status;

	status = enter_crit (&hw_lock);
	
	write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0c);
	write_bt848_reg(pd->Bt848,CAP_CTL,0x00);
	
	if (primary)
		write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->pri_risc_phys_memory[0]);
	else
		write_bt848_reg(pd->Bt848,RISC_STRT_ADD,(uint32)pd->alt_risc_phys_memory[0]);
				
	write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0d);	
	write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL);
	
	if (primary)
		write_bt848_reg(pd->Bt848,CAP_CTL,pd->pri_capctl);
	else
		write_bt848_reg(pd->Bt848,CAP_CTL,pd->alt_capctl);

	pd->active = true;

	exit_crit (&hw_lock, status);
}

/*------------------------------------------------------------------------*/

static void 
halt_risc(priv_data *pd)
{
	cpu_status status;
	status = enter_crit (&hw_lock);
	
	pd->active = false;

	write_bt848_reg(pd->Bt848,GPIO_DMA_CTL,DMA_CTL & 0x0c);
	write_bt848_reg(pd->Bt848,CAP_CTL,0x00);

	exit_crit (&hw_lock, status);
}

/*------------------------------------------------------------------------*/

int32 
I2CRead(uint32 *bt848, int32 addr, uint32 speed) 
{
	volatile uint32 status;
	uint32 value,cnt;
	
	write_bt848_reg(bt848, INT_STAT, BT848_I2CDONE | BT848_RACK);	/* clear interrupts */
	write_bt848_reg(bt848,I2C,(addr << 25) | 0x01000003 | speed);
	
	snooze(250);	
	cnt =1000;
	status = read_bt848_reg(bt848,INT_STAT);
	while ((status & BT848_I2CDONE) != BT848_I2CDONE) 
	{
		snooze(1000);
		status = read_bt848_reg(bt848,INT_STAT);
		if (cnt-- == 0) 
		{
			return BT848_I2C_DONE_ERROR;
		}
	}
	
	if ((status & BT848_RACK) == BT848_RACK) 
	{
		value = read_bt848_reg(bt848,I2C);
		return (value & 0xff00) >> 8;		
	}
	else
		return BT848_I2C_RACK_ERROR;

}

/*------------------------------------------------------------------------*/

int32
I2CWrite1(uint32 *bt848, int32 addr, int32 dat1, uint32 speed) 
{
	volatile uint32 status;
	uint32 cnt;
	
	write_bt848_reg(bt848, INT_STAT, BT848_I2CDONE | BT848_RACK);	/* clear interrupts */
	write_bt848_reg(bt848, I2C, (addr << 25) | ((dat1 & 0xff) << 16) | 0x03 | speed);
		
	snooze(250);	
	cnt = 1000;
	status = read_bt848_reg(bt848,INT_STAT);
	while ((status & BT848_I2CDONE) != BT848_I2CDONE) 
	{
		snooze(1000);
		status = read_bt848_reg(bt848,INT_STAT);
		if (cnt-- == 0) 
		{
			return BT848_I2C_DONE_ERROR;
		}
	}
	
	if ((status & BT848_RACK) == BT848_RACK) 
	{
		return BT848_I2C_SUCCESS;		
	}
	else 
	{
		return BT848_I2C_RACK_ERROR;
	}
}


/*------------------------------------------------------------------------*/

int32
I2CWrite2(uint32 *bt848, int32 addr, int32 dat1, int32 dat2, uint32 speed) 
{
	volatile uint32 status;
	uint32 cnt;
	
	write_bt848_reg(bt848, INT_STAT, BT848_I2CDONE | BT848_RACK);	/* clear interrupts */
	write_bt848_reg(bt848, I2C, (addr << 25) | ((dat1 & 0xff) << 16) | ((dat2 & 0xff) << 8) | 0x07 | speed);
		
	snooze(250);	
	cnt = 1000;
	status = read_bt848_reg(bt848,INT_STAT);
	while ((status & BT848_I2CDONE) != BT848_I2CDONE) 
	{
		snooze(1000);
		status = read_bt848_reg(bt848,INT_STAT);
		if (cnt-- == 0) 
		{
			return BT848_I2C_DONE_ERROR;
		}
	}
	
	if ((status & BT848_RACK) == BT848_RACK) 
	{
		return BT848_I2C_SUCCESS;		
	}
	else 
	{
		return BT848_I2C_RACK_ERROR;
	}
}

