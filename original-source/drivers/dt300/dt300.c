#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>

#include "dt300.h"

static isa_module_info *isa;

#define inb(addr) (isa->read_io_8(addr))
#define outb(addr,val) (isa->write_io_8(addr,val))

#define	DATA_PORT		0x820
#define COMMAND_PORT	0x920

#define DATA_PORT_BUFFER_FULL_INTR	0x01	/* bit in DATA_PORT+1 */

#define ENTER_COMMAND_PHASE  0xE0
#define EC_COMMAND_SEND      0xE1
#define ENTER_RECEIVE_PHASE  0xE2
#define EC_RECEIVE_DATA      0xE3


#define EC_NOP			0x00
#define EC_VERSION		0x01
#define EC_RES_01		0x02
#define EC_SYS_STAT		0x03
#define EC_BTN_STAT		0x04
#define EC_TEMP_STAT	0x05
#define EC_PWM_STAT		0x06
#define EC_PEN_STAT		0x07
#define	EC_BATT_STAT	0x08 

#define EC_ERROR_REPORT	0x10
#define	RECEIVE_EC_DATA 0x20


#define TURN_OFF_BACKLIGHT	0xB0
#define TURN_ON_BACKLIGHT	0xB1

#if 1
#define disable_draco_interrupt()	outb(DATA_PORT+1, inb(DATA_PORT+1) & (~DATA_PORT_BUFFER_FULL_INTR))
#define enable_draco_interrupt()	outb(DATA_PORT+1, inb(DATA_PORT+1) |   DATA_PORT_BUFFER_FULL_INTR)
#else
#define disable_draco_interrupt()	outb(DATA_PORT+1, 0)
#define enable_draco_interrupt()	outb(DATA_PORT+1, 1)
#endif


typedef struct event {
	enum { NONE=0, PENDOWN, PENUP, BUTTONS } type;
	uint16 x;
	uint16 y;
} event;

#define MAXEVENTS 64

static event event_queue[MAXEVENTS];
static int read_pos = 0;
static spinlock read_pos_lock = 0;
static int write_pos = 0;
static sem_id event_sem = -1;
static int last_x = 0;
static int last_y = 0;
static int skipped_count = 0;
static spinlock skipped_count_lock = 0;


#define turn_off_backlight()	outb(COMMAND_PORT, TURN_OFF_BACKLIGHT);
#define turn_on_backlight()		outb(COMMAND_PORT, TURN_ON_BACKLIGHT);

#define PENUP_TIMEOUT	75000
#define	SKIP_DELTA		900
#define	MAX_SKIPS		3
#define DT300_DEBUG		0

static status_t
read_data_from_ec(uint8 ec_command, void* data, size_t* data_len) 
{
	cpu_status	ps;
	uint8		status;
	size_t		max_data_len;
	int			i;
	status_t	ret = B_ERROR;
	
	ps = disable_interrupts();
	disable_draco_interrupt();
	
	outb(COMMAND_PORT, ENTER_RECEIVE_PHASE);
	outb(DATA_PORT, ec_command);
	outb(COMMAND_PORT, EC_RECEIVE_DATA);
	status = inb(DATA_PORT);
	//dprintf("dt300: status0 = %02x\n", status);
	switch(status & 0xf0)
	{
	case RECEIVE_EC_DATA:
		/* read ALL data from EC */
		max_data_len = *data_len;
		for(i=0; i < ((status & 0x0f)-1); i++)
		{
			uint8	b = inb(DATA_PORT);
			//dprintf("%02x ", b);
			if(i<max_data_len)
				((uint8*)data)[i] = b;
		}
		//sdprintf("\n");
		if(i<max_data_len)
			*data_len = i;
		ret = B_OK;
		break;
		
	case EC_ERROR_REPORT:
		switch(status & 0x0f)
		{
			default:
				dprintf("DT300: read status %02x\n", status);
		}
		break;
	
	default:
		dprintf("DT300: read status %02x\n", status);
	}
	
	enable_draco_interrupt();
	restore_interrupts(ps);
	return ret;
}



static status_t 
driver_open(const char *name, uint32 flags, void **cookie)
{
	return B_OK;
}

static status_t 
driver_close(void *cookie)
{
	return B_OK;
}

static status_t
driver_free(void *cookie)
{
	return B_OK;
}

static status_t 
driver_control(void *cookie, uint32 op, void* data, size_t len)
{
	status_t	ret = B_BAD_INDEX;
	size_t		dl;
	
	switch(op)
	{
	case DT300_READ_BATTERY_STATUS:
		dl = 1;
		ret = read_data_from_ec(EC_BATT_STAT, data, &dl);		
		break;

	case DT300_TURN_OFF_BACKLIGHT:
		turn_off_backlight();		
		ret = B_OK;
		break;

	case DT300_TURN_ON_BACKLIGHT:
		turn_on_backlight();		
		ret = B_OK;
		break;
	}
	
	return ret;
}

#if DT300_DEBUG
void
dump_event(event *e)
{
	switch(e->type) {
	case NONE:
		dprintf("dt300: N\n");
		return;
	case PENDOWN:
		dprintf("dt300: D, x = %hu\ty = %hu\n", e->x, e->y);
		return;
	case PENUP:
		dprintf("dt300: PENUP\n");
		return;
	case BUTTONS:
		dprintf("dt300: B\n");
		return;
	}
	return;
}
#endif

static status_t 
driver_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	cpu_status	ps;
	char		event_buf[sizeof(event)];

	/* Make sure reader is looking for the event struct */
	if(*numBytes == sizeof(event)) {
		/* Wait on event_sem for the driver to release it upon interrupt */
		if(acquire_sem_etc(event_sem, 1, B_CAN_INTERRUPT, 0) == B_OK) {
			/* The interrupt handler may be changing read_pos to avoid overflow */
			ps = disable_interrupts();
			acquire_spinlock(&read_pos_lock);
			/* Copy to a locked buffer to avoid VM faults */
			memcpy(event_buf, event_queue + read_pos, sizeof(event));
			read_pos++;
			if(read_pos == MAXEVENTS) read_pos = 0;
			release_spinlock(&read_pos_lock);
			restore_interrupts(ps);
			
			/* VM faults are ok outside the critical section */
			memcpy(data, event_buf, sizeof(event));
#if DT300_DEBUG
			dump_event((event *)data);
#endif
			/* Check for a new event within PENUP_TIMEOUT microseconds, catches bad PENUPs in a dragging stream */
			if (((event *)data)->type == PENUP) {
				if (acquire_sem_etc(event_sem, 1, B_RELATIVE_TIMEOUT, PENUP_TIMEOUT) != B_TIMED_OUT) {
					/* Skip PENUP and grab the next event for the reader */
					
					/* The interrupt handler may be changing read_pos to avoid overflow */
					ps = disable_interrupts();
					acquire_spinlock(&read_pos_lock);
					/* Copy to a locked buffer to avoid VM faults */
					memcpy(event_buf, event_queue + read_pos, sizeof(event));
					read_pos++;
					if(read_pos == MAXEVENTS) read_pos = 0;
					release_spinlock(&read_pos_lock);
					restore_interrupts(ps);
					
					/* VM faults are ok outside the critical section */
					memcpy(data, event_buf, sizeof(event));
#if DT300_DEBUG
					dump_event((event *)data);
#endif
				} else {
					/* Real PENUP */
					ps = disable_interrupts();
					acquire_spinlock(&skipped_count_lock);
					skipped_count = 0;
					release_spinlock(&skipped_count_lock);
					restore_interrupts(ps);
				}
			}
			*numBytes = sizeof(event);
			return B_NO_ERROR;
		}
	}	

	*numBytes = 0;
	return B_ERROR;
}

static status_t 
driver_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	cpu_status ps;
	uchar b,c;
	if(*numBytes != 2){
		*numBytes = 0;
		return B_ERROR;
	}
	b = ((uchar*)data)[0];
	c = ((uchar*)data)[1];
	ps = disable_interrupts();
	disable_draco_interrupt();
	outb(COMMAND_PORT, ENTER_COMMAND_PHASE);
	outb(DATA_PORT, 0x06);
	outb(DATA_PORT, b);
	outb(DATA_PORT, c);
	outb(COMMAND_PORT, EC_COMMAND_SEND);
	enable_draco_interrupt();
	restore_interrupts(ps);
	return 2;
}


static device_hooks driver_hooks = {
	driver_open,
	driver_close,
	driver_free,
	driver_control,
	driver_read,
	driver_write,
	NULL,
	NULL,
	NULL,
	NULL	
};

_EXPORT status_t
init_hardware(void)
{
	return B_OK;
}

_EXPORT const char **
publish_devices()
{
	static const char *names[] = { "misc/dt300", NULL };
	return names;
}

_EXPORT device_hooks *
find_device(const char *name)
{
	return &driver_hooks;
}

/*
 * IRQ 9 is triggered by the VCOM port when a button event is waiting
 */
int32 do_irq9(void *data)
{
	uchar a;
	int rel_sem;

	a = inb(DATA_PORT);
	if(a & 0x80) {
		event_queue[write_pos].type = BUTTONS;
		event_queue[write_pos].x = a & 0x7f;

		write_pos++;
		if(write_pos == MAXEVENTS) write_pos = 0;
		
		/* Avoid buffer overflow */
		rel_sem = TRUE;
		acquire_spinlock(&read_pos_lock);
		if (write_pos == read_pos) {
			read_pos++;
			if (read_pos == MAXEVENTS) read_pos = 0;
			rel_sem = FALSE;;
		}
		release_spinlock(&read_pos_lock);

		if (rel_sem) {
			release_sem_etc(event_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;
		}
	}
	return B_HANDLED_INTERRUPT;
}

/*
 * IRQ 10 is triggered by the VCOM port when a PENUP or PENDOWN message is waiting
 */
int32 do_irq10(void *data)
{
	uchar a, b, c, d, e;
	int dx, dy, rel_sem;
	
	a = inb(COMMAND_PORT);	
	b = inb(COMMAND_PORT);	
	c = inb(COMMAND_PORT);	
	d = inb(COMMAND_PORT);	
	e = inb(COMMAND_PORT);	
	/*
	 *	Pen data packet format:
	 *
	 *	Pen down:
	 *	a = 0xff
	 *	b = low byte of X
	 *	c = high byte of X
	 *	d = low byte of Y
	 *	e = high byte of Y
	 *
	 *	Pen up:
	 *	a = 0xff
	 *	b = 0xfe
	 *	c = 0xfe
	 *	d = 0x00
	 *	e = 0x00
	 */

	/* packet sanity checking */
	if (a != 0xff) {
#if DT300_DEBUG
		dprintf("dt300: bad packet: %02x %02x %02x %02x %02x\n", a, b, c, d, e);
#endif
		return B_HANDLED_INTERRUPT;
	}

	/* PENUP packet */
	if((b == 0xfe) && (c == 0xfe) && (d == 0x00) && (e == 0x00)){ 
		event_queue[write_pos].type = PENUP;
		event_queue[write_pos].x = 0;
		event_queue[write_pos].y = 0;
	} else {
	/* PENDOWN packet */
		event_queue[write_pos].type = PENDOWN;
		event_queue[write_pos].x = b | (c << 8);
		event_queue[write_pos].y = d | (e << 8);
		dx = event_queue[write_pos].x - last_x;
		dy = event_queue[write_pos].y - last_y;
		
		/* skip this event if it isn't close enough to the previous one, and the MAX_SKIPS threshold hasn't been reached */
		acquire_spinlock(&skipped_count_lock);
		if((skipped_count && (skipped_count < MAX_SKIPS+1)) && (dx < -SKIP_DELTA || dx > SKIP_DELTA || dy < -SKIP_DELTA || dy > SKIP_DELTA))
		{
#if DT300_DEBUG
			dprintf("dt300: +++ x = %hu\tlx = %hu\tdx = %d\ty = %hu\tly = %hu\tdy = %d\n", event_queue[write_pos].x, last_x, dx, event_queue[write_pos].y, last_y, dy);
#endif
			skipped_count++;
			release_spinlock(&skipped_count_lock);
			return B_HANDLED_INTERRUPT;
		}
		skipped_count = 1;
		release_spinlock(&skipped_count_lock);
		last_x = event_queue[write_pos].x;
		last_y = event_queue[write_pos].y;
	}

	write_pos++;
	if(write_pos == MAXEVENTS) write_pos = 0;
	
	/* Avoid buffer overflow */
	rel_sem = TRUE;
	acquire_spinlock(&read_pos_lock);
	if (write_pos == read_pos) {
		read_pos++;
		if (read_pos == MAXEVENTS) read_pos = 0;
		rel_sem = FALSE;
	}
	release_spinlock(&read_pos_lock);
	
	/* Don't release the semaphore if we removed an event from the queue */
	if (rel_sem) {
		release_sem_etc(event_sem, 1, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER; /* Give the reader an attempt at running */
	}
	return B_HANDLED_INTERRUPT;
}

_EXPORT status_t
init_driver(void)
{
	status_t err;
	err = get_module(B_ISA_MODULE_NAME,(module_info**)&isa);
	event_sem = create_sem(0,"dt300:event");
	if(err) return err;
	//load_driver_symbols("dt300");

	install_io_interrupt_handler(10, do_irq10, NULL, 0);	
	install_io_interrupt_handler(9, do_irq9, NULL, 0);	
	outb(COMMAND_PORT+1, 1);
	enable_draco_interrupt();
	return B_OK;
}

_EXPORT void
uninit_driver(void)
{
	disable_draco_interrupt();
	outb(COMMAND_PORT+1, 0);
	remove_io_interrupt_handler(10, do_irq10, NULL);
	remove_io_interrupt_handler(9, do_irq9, NULL);
	delete_sem(event_sem);
	put_module(B_ISA_MODULE_NAME);
}

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;

