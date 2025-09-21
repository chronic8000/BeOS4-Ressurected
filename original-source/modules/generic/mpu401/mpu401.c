
/*	Generic MPU-401 MIDI driver module	*/
/*	For use by sound card drivers that publish a midi device.	*/

#include <OS.h>
#include <Drivers.h>
#include <ISA.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include "midi_driver.h"

const char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;

#if defined(__powerc) && defined(__MWERKS__)
#define EIEIO() __eieio()
#else
#define EIEIO()
#endif

// QUEUE_SIZE must be a power of 2
// 512 bytes == ~163 ms of MIDI data before we wrap
#define QUEUE_SIZE 512
#define QUEUE_MASK (QUEUE_SIZE - 1)

#define OUTB(p,v) ((port->write_func)?(port->write_func(port->r_cookie,port->p,v)):(port->write_io_8(port->p,v)))
#define INB(p) ((port->read_func)?(port->read_func(port->r_cookie,port->p)):(port->read_io_8)(port->p))

#define LOCK_HW(cp)  (cp = disable_interrupts(), acquire_spinlock(&port->lock))
#define UNLOCK_HW(cp) (release_spinlock(&port->lock), restore_interrupts(cp))

#if DEBUG
#define ddprintf(x) dprintf x
#else
#define ddprintf(x)
#endif

#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

typedef struct 
{
	void (*interrupt_op)(int32 op, void *card);
	void *interrupt_card;
	
	uint8  (*read_io_8)(int mapped_io_addr);
	void   (*write_io_8)(int mapped_io_addr, uint8 value);
	uchar  (*read_func)(void * r_cookie, int port);
	void   (*write_func)(void * r_cookie, int port, uchar value);
	void   *r_cookie;
	uint32 command_port;
	uint32 status_port;
	uint32 data_port;	
	
	spinlock lock;
	
	uchar queue[QUEUE_SIZE];
	uint32 queue_head;
	uint32 queue_tail;
	uint32 queue_count;   /* bytes unused in the midi read fifo */

	uint32 open_count;
	int read_sem;
	int write_sem;
	
	bigtime_t timeout;
	
} mpu401_dev;

static status_t midi_open(void * storage, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);

static status_t
midi_open(void *storage, uint32 flags, void **cookie)
{
	int ix;
	mpu401_dev *port = NULL;
	uint8 byte;
	cpu_status cp;
	
	*cookie = port = (mpu401_dev *) storage;
	
	dprintf("mpu401: midi_open()\n");
	
	LOCK_HW(cp);
	port->open_count++;
	if(port->open_count == 1){
		/* enable interrupts */
		ddprintf(("mpu401: enable interrupts...\n"));
		/* reset UART */
		dprintf("mpu401: reset MPU401\n");
		OUTB(command_port, 0xff);
		EIEIO();
		spin(3);
		/* read back status */
		byte = INB(data_port);
		ddprintf(("port cmd ack is %x\n", byte));
		
		/* reset UART */
		dprintf("mpu401: enable UART mode\n");
		OUTB(command_port, 0x3f);
		EIEIO();
		spin(3);
		/* read back status */
		byte = INB(data_port);
		dprintf("port cmd ack is %x\n", byte);
		(*port->interrupt_op)(B_MPU_401_ENABLE_CARD_INT, port->interrupt_card);		
	}
	UNLOCK_HW(cp);

	dprintf("mpu401: 0x%04x midi_open() done (count = %d)\n", port, port->open_count);
	return B_OK;
}


static status_t
midi_close(void *cookie)
{
	mpu401_dev *port = (mpu401_dev *)cookie;
	cpu_status cp;

	LOCK_HW(cp);
	port->open_count--;
	if(port->open_count == 0){
		ddprintf(("mpu401: disable interrupts...\n"));
		(*port->interrupt_op)(B_MPU_401_DISABLE_CARD_INT, port->interrupt_card);
	}
	UNLOCK_HW(cp);

	dprintf("mpu401: midi_close() done (count = %d)\n", port->open_count);
	return B_OK;
}


static status_t
midi_free(void *cookie)
{
	return B_OK;
}


static status_t
midi_control(void *cookie, uint32 iop, void *data, size_t len)
{
	mpu401_dev *port = (mpu401_dev *)cookie;

	ddprintf(("midi_control()"));

	switch (iop) {
	case B_MIDI_GET_READ_TIMEOUT:
	case B_MIDI_GET_READ_TIMEOUT_OLD:
		ddprintf((" get timeout\n"));
		memcpy(data, &port->timeout, sizeof(bigtime_t));
		return B_OK;
		
	case B_MIDI_SET_READ_TIMEOUT:
	case B_MIDI_SET_READ_TIMEOUT_OLD:
		ddprintf((" set timeout %Ld\n", *((bigtime_t*) data)));
		memcpy(&port->timeout, data, sizeof(bigtime_t));
		return B_OK;
		
	default:
		ddprintf((" huh? (%ld)\n", iop));
		return B_BAD_VALUE;
	}
}


static status_t
midi_read(void *cookie, off_t pos, void * ptr, size_t *nread)
{
	mpu401_dev *port = (mpu401_dev *) cookie;
	status_t err = 0;
	cpu_status cp;
	int count = *nread;
	uchar *x = (uchar *) ptr;
	

	ddprintf(("midi_read(%d)\n", *nread, *nread - count));
	while(count){
		if(acquire_sem_etc(port->read_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, port->timeout)){
			err = -1;
			break;
		}
		LOCK_HW(cp);
		*x = port->queue[port->queue_head & QUEUE_MASK];
		port->queue_head++;
		port->queue_count++;
		UNLOCK_HW(cp);
		count--;
		x++;
	}
	ddprintf(("midi_read(%d, %d)\n", *nread, *nread - count));

	*nread -= count;
	return err;
}


static status_t
midi_write(void *cookie, off_t pos, const void * ptr, size_t *nwritten)
{
	mpu401_dev *port = (mpu401_dev *) cookie;
	size_t count = *nwritten;
	cpu_status cp;
	uchar *x = (uchar *) ptr;
	int times = 0;
	
	acquire_sem(port->write_sem); /* make midi writes atomic */
	while(count && (times < 10)){
		LOCK_HW(cp);
		if(INB(status_port) & 0x40){
			UNLOCK_HW(cp);
			times++;
			snooze(1000);
		} else {
			OUTB(data_port, *x);
			UNLOCK_HW(cp);
			times = 0;
			x++;
			count--;
		}
	}
	release_sem(port->write_sem);
	
/*	ddprintf(("%s: midi_write()\n", ((mpu401_dev *)cookie)->name)); /* */

	*nwritten = *nwritten - count;
	return times == 10 ? B_ERROR : B_OK;
}

bool
midi_interrupt(void *d)
{
	int bytes = 0;
	mpu401_dev *port = (mpu401_dev *)d;
	
	acquire_spinlock(&port->lock);
	
	if((INB(status_port) & 0x80) == 0x00) {
		uint8 midi_byte = INB(data_port);	/* get data */
		bytes = 1;
		
		if(port->queue_count){
			/* still space in the queue... add a byte and advance the tail */
			port->queue[port->queue_tail & QUEUE_MASK] = midi_byte;
			port->queue_tail++;
			port->queue_count--;
			release_sem_etc(port->read_sem, 1, B_DO_NOT_RESCHEDULE);
		} else if(midi_byte != MIDI_ACTIVE_SENSE) {
			/* no more space in the queue ... 
				replace the head and move forward */
			port->queue[port->queue_tail & QUEUE_MASK] = midi_byte;
			port->queue_tail++;
			port->queue_head++;
		}		
	}
	
	release_spinlock(&port->lock);
	return bytes ? true : false;
}

static status_t
create_device(
	int _port,
	void **out_storage,
	uint32 workarounds, 
	void (*card_interrupt_op)(int32 op, void *user_card),
	void *user_card)
{
	mpu401_dev *port;
	char name[32];
	uchar byte;
	
	ddprintf(("mpu401: create_device()\n"));
	
	if(!(port = (mpu401_dev *) malloc(sizeof(mpu401_dev)))) return ENOMEM;
	
	*out_storage = NULL;
	
	sprintf(name, "mpu401:%04x:read_sem", _port & 0xffff);	
	if((port->read_sem = create_sem(0,name)) < 0){		
		status_t err = port->read_sem;
		free(port);
		return err;
	}
	
	sprintf(name, "mpu401:%04x:write_sem", _port & 0xffff);
	if((port->write_sem = create_sem(1,name)) < 0){		
		status_t err = port->write_sem;
		delete_sem(port->read_sem);
		free(port);
		return err;
	}

	*out_storage = port;
	port->queue_head = 0;
	port->queue_tail = 0;
	port->queue_count = QUEUE_SIZE;
	
	port->lock = 0;
	port->read_io_8 = isa->read_io_8;
	port->write_io_8 = isa->write_io_8;
	port->timeout = B_INFINITE_TIMEOUT;
	port->status_port = _port + 1;
	port->command_port = _port + 1;
	port->data_port = _port;
	port->interrupt_op = card_interrupt_op;
	port->interrupt_card = user_card;
	port->open_count = 0;
	port->r_cookie = 0;
	port->read_func = 0;
	port->write_func = 0;

	ddprintf(("mpu401: create_device() done\n"));

	return B_OK;
}

static status_t
create_device_alt(
	int _port,
	void * r_cookie,
	uchar (*read_func)(void *, int),
	void (*write_func)(void *, int, uchar),
	void **out_storage,
	uint32 workarounds, 
	void (*card_interrupt_op)(int32 op, void *user_card),
	void *user_card)
{
	mpu401_dev *port;
	char name[32];
	uchar byte;
	
	ddprintf(("mpu401: create_device_alt()\n"));

	if(!(port = (mpu401_dev *) malloc(sizeof(mpu401_dev)))) return ENOMEM;
	
	*out_storage = NULL;
	
	sprintf(name, "mpu401a:%04x:read_sem", _port & 0xffff);
	if((port->read_sem = create_sem(0,name)) < 0){		
		status_t err = port->read_sem;
		free(port);
		return err;
	}
	
	sprintf(name, "mpu401a:%04x:write_sem", _port & 0xffff);
	if((port->write_sem = create_sem(1,name)) < 0){		
		status_t err = port->write_sem;
		delete_sem(port->read_sem);
		free(port);
		return err;
	}

	*out_storage = port;
	port->queue_head = 0;
	port->queue_tail = 0;
	port->queue_count = QUEUE_SIZE;
	
	port->lock = 0;
	port->read_io_8 = isa->read_io_8;
	port->write_io_8 = isa->write_io_8;
	port->timeout = B_INFINITE_TIMEOUT;
	port->status_port = _port + 1;
	port->command_port = _port + 1;
	port->data_port = _port;
	port->interrupt_op = card_interrupt_op;
	port->interrupt_card = user_card;
	port->open_count = 0;
	port->r_cookie = r_cookie;
	port->read_func = read_func;
	port->write_func = write_func;

	ddprintf(("mpu401: create_device_alt() done\n"));

	return B_OK;
}

static status_t
delete_device(
	void * storage)
{
	mpu401_dev *port = (mpu401_dev *)storage;
	
	ddprintf(("mpu401x: delete_device()\n"));
	delete_sem(port->read_sem);
	delete_sem(port->write_sem);
	free(storage);
	return B_OK;
}

static status_t
init()
{
	ddprintf(("midi module init()\n"));
	isa = NULL;
	return get_module(isa_name, (module_info **)&isa);
}

static status_t
uninit()
{
	ddprintf(("midi module uninit()\n"));
	if (isa) put_module(isa_name);
	return B_OK;
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		return -1;
	}
}


static generic_mpu401_module g_mpu401[2] = {
	{
		{
			B_MPU_401_MODULE_NAME_V1,
			0,
			std_ops
		},
		create_device,
		delete_device,
		midi_open,
		midi_close,
		midi_free,
		midi_control,
		midi_read,
		midi_write,
		midi_interrupt
	},
	{
		{
			B_MPU_401_MODULE_NAME,
			0,
			std_ops
		},
		create_device,
		delete_device,
		midi_open,
		midi_close,
		midi_free,
		midi_control,
		midi_read,
		midi_write,
		midi_interrupt,
		create_device_alt
	},
};

_EXPORT generic_mpu401_module *modules[] = {
		&g_mpu401[0],
		&g_mpu401[1],
        NULL
};

