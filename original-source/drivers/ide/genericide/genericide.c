/* ISA IDE module
*/

#include <IDE.h>
#include <ISA.h>
#include <KernelExport.h>
#include <string.h>

static isa_module_info		*isa;

typedef struct {
	uint16	command_block_base_addr;
	uint16	control_block_register_addr;
	int		intnum;
	int		interrupt_gate;
	sem_id	mutex;
	sem_id	intsem;
} bus_info_t;

#define NUM_BUSSES 1
static bus_info_t bus_info[NUM_BUSSES] = {
//	{ 0x1f0, 0x3f6, 14, 1, -1, -1},
	{ 0x170, 0x376, 15, 1, -1, -1}
};

static uint32
get_nth_cookie(uint32 bus)
{
	return bus;
}

static uint32
get_bus_count()
{
	return NUM_BUSSES;
}

static int32
get_abs_bus_num(uint32 cookie)
{
	switch(bus_info[cookie].command_block_base_addr) {
		case 0x1f0:	return 0;
		case 0x170:	return 1;
		default:	return -1;
	}
}

static status_t
acquire_bus(uint32 cookie)
{
	return acquire_sem(bus_info[cookie].mutex);
}

static status_t
release_bus(uint32 cookie)
{
	return release_sem_etc(bus_info[cookie].mutex, 1, B_DO_NOT_RESCHEDULE);
}

static status_t
write_command_block_regs(uint32 cookie, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = bus_info[cookie].command_block_base_addr;

	for(i=0; i<7; i++) {
		if((1<<i) & mask) {
//			dprintf("IDE ISA -- write_command_block_regs: "
//			        "write r[%d] = 0x%02x\n", i, tf->raw.r[i]);
			isa->write_io_8(ioaddr+1+i, tf->raw.r[i]);
		}
	}
	return B_NO_ERROR;
}

static status_t
read_command_block_regs(uint32 cookie, ide_task_file *tf, ide_reg_mask mask)
{
	int i;
	uint16 ioaddr = bus_info[cookie].command_block_base_addr;

	for(i=0; i<7; i++) {
		if((1<<i) & mask) {
			tf->raw.r[i] = isa->read_io_8(ioaddr+1+i);
//			dprintf("IDE ISA -- read_command_block_regs: "
//			        "read r[%d] = 0x%02x\n", i, tf->raw.r[i]);
		}
	}
	return B_NO_ERROR;
}


static uint8
get_altstatus(uint32 cookie)
{
	uint16 altstatusaddr = bus_info[cookie].control_block_register_addr;
	return isa->read_io_8(altstatusaddr);
}

static void
write_device_control(uint32 cookie, uint8 val)
{
	uint16 device_control_addr = bus_info[cookie].control_block_register_addr;
	isa->write_io_8(device_control_addr, val);
}

static void
write_pio_16(uint32 cookie, uint16 *data, uint16 count)
{
	uint16 ioaddr = bus_info[cookie].command_block_base_addr;
	uint16 *dp = data;
	uint16 *ep = data+count;
	
	while(dp < ep)
		isa->write_io_16(ioaddr, *(dp++));
}

static void
read_pio_16(uint32 cookie, uint16 *data, uint16 count)
{
	uint16 ioaddr = bus_info[cookie].command_block_base_addr;
	uint16 *dp = data;
	uint16 *ep = data+count;
	
	while(dp < ep)
		*(dp++) = isa->read_io_16(ioaddr);
}

static status_t
intwait(uint32 cookie, bigtime_t timeout)
{
	status_t err;
//	dddprintf("IDE ISA -- ide_wait\n");
//	int32 count = -99;
//	get_sem_count(bus_info[cookie].intsem, &count);
//	dprintf("IDE ISA -- intwait: semcount = %d, intgate = %d\n",
//	        count, bus_info[cookie].interrupt_gate);
	err = acquire_sem_etc(bus_info[cookie].intsem,1,B_TIMEOUT,timeout);

	/* ---
		The Maxtor 7420AV (and maybe others?) seem to have some sort of
		watchdog timer, and if the host does not start doing something to
		the drive within 4-5 millisecs of an interrupt, it interrupts AGAIN!
		The interrupt_gate flag prevents getting more than one interrupt for
		the same 'event'.
	--- */

	if(err == B_NO_ERROR) {
		bus_info[cookie].interrupt_gate = 1;
	}
	else {
		//dprintf("IDE ISA -- intwait: error = %s\n", strerror(err));
	}
//	dddprintf("IDE ISA -- ide_intwait: done err=%x\n", err);
	return err;
}

static bool
ide_inthand(bus_info_t *bus_info_p)
{	
	// read STATUS to clear interrupt
	isa->read_io_8(bus_info_p->command_block_base_addr+7);

	//dprintf("IDE ISA -- ide_inthand\n");

	if (bus_info_p->interrupt_gate) {		/* see comment in intwait() */
		bus_info_p->interrupt_gate = 0;
		release_sem_etc (bus_info_p->intsem,1,B_DO_NOT_RESCHEDULE);
	}
	return true;
}

static status_t
prepare_dma(uint32 cookie, void *buffer, size_t size)
{
	return B_ERROR;
}

static status_t
init()
{
	int i;
	status_t err;
	static status_t uninit();
	
	dprintf("IDE ISA -- init\n");
	err = get_module("bus_managers/isa", &(module_info *)isa);
	if (err)
		return err;

	for(i=0; i<NUM_BUSSES; i++) {
		bus_info[i].mutex = -1;
		bus_info[i].intsem = -1;
	}
	for(i=0; i<NUM_BUSSES; i++) {
		const char *mutexname[] = { "idebus0mutex", "idebus1mutex" };
		const char *intsemname[] = { "idebus0intsem", "idebus1intsem" };
		err = bus_info[i].mutex = create_sem(1, mutexname[i]);
		if(err < B_NO_ERROR) goto err;
		err = bus_info[i].intsem = create_sem(0, intsemname[i]);
		if(err < B_NO_ERROR) goto err;
		install_io_interrupt_handler(bus_info[i].intnum, (interrupt_handler)ide_inthand,
		                             &bus_info[i], 0);
	}
	return B_NO_ERROR;

err:
	uninit();
	return err;
}

static status_t
uninit()
{
	int i;
	
	dprintf("IDE ISA -- uninit\n");
	if (isa)
		put_module("bus_managers/isa");
	for(i=0; i<NUM_BUSSES; i++) {
		if(bus_info[i].mutex >= 0)
			delete_sem(bus_info[i].mutex);
		if(bus_info[i].intsem >= 0)
			delete_sem(bus_info[i].intsem);
		remove_io_interrupt_handler(bus_info[i].intnum,
		                            (interrupt_handler)ide_inthand,
									&bus_info[i]);
	}
	return B_NO_ERROR;
}

static status_t
std_ops(int32 op)
{
	status_t err;
	
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		return -1;
	}
}

static status_t
rescan()
{
	return 0;
}

ide_bus_info genericide_module = {
	{
		{
			"busses/ide/genericide",
			0,
			&std_ops
		},
		&rescan
	},
	&get_nth_cookie,
	&get_bus_count,
	&get_abs_bus_num,
	&acquire_bus,
	&release_bus,
	&write_command_block_regs,
	&read_command_block_regs,
	&get_altstatus,
	&write_device_control,
	&write_pio_16,
	&read_pio_16,
	&intwait,
	&prepare_dma
};

_EXPORT 
ide_bus_info *modules[] = {
	&genericide_module,
	NULL
};

