#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>
//#include <malloc.h>

#define ID	"mk712: "

static isa_module_info *isa;

#define inb(addr) (isa->read_io_8(addr))
#define outb(addr,val) (isa->write_io_8(addr,val))

#define MK712_IO_BASE 0x260
#define MK712_IRQ 10

typedef struct event {
	bigtime_t  time;
	uint16     x;
	uint16     y;
	bool       pressed;
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


#define PENUP_TIMEOUT	75000
#define	SKIP_DELTA		900
#define	MAX_SKIPS		3
#define _DEBUG			0

static status_t driver_open(const char *name, uint32 flags, void **cookie)
{
	return B_OK;
}

static status_t driver_close(void *cookie)
{
	return B_OK;
}

static status_t driver_free(void *cookie)
{
	return B_OK;
}

static status_t driver_control(void *cookie, uint32 op, void* data, size_t len)
{
	//status_t err;
	switch(op) {
		default:
			dprintf("mk712: control %lx\n", op);
			return B_ERROR;
	}
}

static status_t driver_read(void *cookie, off_t position, void *data, size_t *numBytes)
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
			if(read_pos == MAXEVENTS) {
				read_pos = 0;
			}
			release_spinlock(&read_pos_lock);
			restore_interrupts(ps);
			
			/* VM faults are ok outside the critical section */
			memcpy(data, event_buf, sizeof(event));
			
			/* Check for a new event within PENUP_TIMEOUT microseconds, catches bad PENUPs in a dragging stream */
			if (((event *)data)->pressed == false) {
				if (acquire_sem_etc(event_sem, 1, B_RELATIVE_TIMEOUT, PENUP_TIMEOUT) != B_TIMED_OUT) {
					/* Skip PENUP and grab the next event for the reader */
					
					/* The interrupt handler may be changing read_pos to avoid overflow */
					ps = disable_interrupts();
					acquire_spinlock(&read_pos_lock);
					/* Copy to a locked buffer to avoid VM faults */
					memcpy(event_buf, event_queue + read_pos, sizeof(event));
					read_pos++;
					if(read_pos == MAXEVENTS) {
						read_pos = 0;
					}
					release_spinlock(&read_pos_lock);
					restore_interrupts(ps);
					
					/* VM faults are ok outside the critical section */
					memcpy(data, event_buf, sizeof(event));
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
			return B_OK;
		}
	}	

	*numBytes = 0;
	return B_ERROR;
}

static status_t driver_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


int32 mk712_interrupt_handler(void *data)
{
	status_t	err;
	uint8		status_low;
	uint8		status_high;
	int32		dx;
	int32		dy;
	bool		rel_sem;
	
	/* Read the status */
	status_low = inb(MK712_IO_BASE);
	
	if ((status_low & 0x80) == 0) {
		return B_UNHANDLED_INTERRUPT;
	}
	
	status_high = inb(MK712_IO_BASE + 1);
	
	event_queue[write_pos].time = system_time();
	
	/* packet sanity checking */
	if((status_low & 0x7f) == 0x00 && status_high == 0x00) {
		/* Pen down */
		event_queue[write_pos].pressed = false;
		event_queue[write_pos].x = 0;
		event_queue[write_pos].y = 0;
	} else if((status_low & 0x7f) == 0x30 && status_high == 0x0c) {
		/* Pen up */
		event_queue[write_pos].pressed = true;
		event_queue[write_pos].x  = inb(MK712_IO_BASE + 2);
		event_queue[write_pos].x |= inb(MK712_IO_BASE + 3) << 8;
		event_queue[write_pos].y  = inb(MK712_IO_BASE + 4);
		event_queue[write_pos].y |= inb(MK712_IO_BASE + 5) << 8;
		dx = event_queue[write_pos].x - last_x;
		dy = event_queue[write_pos].y - last_y;
		
		/* skip this event if it isn't close enough to the previous one, and the MAX_SKIPS threshold hasn't been reached */
		acquire_spinlock(&skipped_count_lock);
		
		if((skipped_count && (skipped_count < MAX_SKIPS+1)) && (dx < -SKIP_DELTA || dx > SKIP_DELTA || dy < -SKIP_DELTA || dy > SKIP_DELTA))
		{
#if _DEBUG
			dprintf(ID "+++ x = %hu\tlx = %hu\tdx = %d\ty = %hu\tly = %hu\tdy = %d\n", event_queue[write_pos].x, last_x, dx, event_queue[write_pos].y, last_y, dy);
#endif
			skipped_count++;
			release_spinlock(&skipped_count_lock);
			return B_HANDLED_INTERRUPT;
		}
		
		skipped_count = 1;
		release_spinlock(&skipped_count_lock);
		last_x = event_queue[write_pos].x;
		last_y = event_queue[write_pos].y;
	} else {
		dprintf(ID "error: sl 0x%02x sh 0x%02x\n", status_low, status_high);
		goto err;
	}
	
	write_pos++;
	if(write_pos == MAXEVENTS) {
		write_pos = 0;
	}
	
	/* Avoid buffer overflow, drop the oldest events */
	rel_sem = true;
	acquire_spinlock(&read_pos_lock);
	if (write_pos == read_pos) {
		read_pos++;
		if (read_pos == MAXEVENTS) {
			read_pos = 0;
		}
		rel_sem = false;
	}
	release_spinlock(&read_pos_lock);
	
	/* Don't release the semaphore if we removed an event from the queue */
	if (rel_sem) {
		release_sem_etc(event_sem, 1, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER; /* Give the reader an attempt at running */
	}
	
err:
	return B_HANDLED_INTERRUPT;
}

status_t init_driver(void)
{
	status_t	err;
	
	dprintf(ID "init_driver\n");
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR) {
		goto err1;
	}
	
	err = event_sem = create_sem(0, ID"event_sem");
	if(err < B_NO_ERROR) {
		goto err2;
	}
	
	err = install_io_interrupt_handler(MK712_IRQ, mk712_interrupt_handler, NULL, 0);
	if(err != B_NO_ERROR) {
		goto err3;
	}
	
	outb(MK712_IO_BASE + 6, 0x53);
	outb(MK712_IO_BASE + 7, 60);  /* 51 samples per second */
	//outb(MK712_IO_BASE + 7, 255); /* 13 samples per second */
	
	return B_OK;
	
err3:
	delete_sem(event_sem);
err2:
	put_module(B_ISA_MODULE_NAME);
err1:
	dprintf(ID "init_driver failed\n");
	return err;
}


void uninit_driver(void)
{
	dprintf(ID "uninit_driver: %d events skipped\n", skipped_count);
	outb(MK712_IO_BASE + 6, 0x00);
	remove_io_interrupt_handler(MK712_IRQ, mk712_interrupt_handler, NULL);
	delete_sem(event_sem);
	put_module(B_ISA_MODULE_NAME);
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

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	"input/touchscreen/mk712/0",
	NULL
};

const char **
publish_devices()
{
	return device_name_list;
}

device_hooks *
find_device(const char *name)
{
	dprintf(ID "find device, %s\n", name);
	return &driver_hooks;
}
