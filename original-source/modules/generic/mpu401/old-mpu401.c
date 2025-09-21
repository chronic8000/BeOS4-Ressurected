/*	Generic MPU-401 MIDI driver module	*/
/*	For use by sound card drivers that publish a midi device.	*/

#include <Drivers.h>
#include <ISA.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include "midi_driver.h"

const char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;

#define PCI_IO_WR(p,v) (*isa->write_io_8)(p,v)
#define PCI_IO_RD(p) (*isa->read_io_8)(p)
#if defined(__powerc) && defined(__MWERKS__)
#define EIEIO() __eieio()
#else
#define EIEIO()
#endif

#if DEBUG
#define KTRACE() kprintf("%s:%d\n", __FILE__, __LINE__)
#define ddprintf dprintf
#define KPRINTF kprintf
#else
#define KTRACE()
#define ddprintf
#define KPRINTF
#endif

typedef struct {
	long long	timeout;
} midi_cfg;

/* buffer size must be power of 2 */
/* we allocate MIDI_BUF_SIZE*5 bytes of memory, so be gentle */
/* 256 bytes is about 90 ms of MIDI data; if we can't serve MIDI in 90 ms */
/* we have bigger problems than overrun dropping bytes... */
#define MIDI_BUF_SIZE 256

typedef struct _midi_wr_q {
	bigtime_t	when;
	struct _midi_wr_q	*next;
	size_t		size;
	unsigned char	data[4];	/* plus more, possibly */
} midi_wr_q;

typedef struct {
	uint32		cookie;
	midi_wr_q	*write_queue;
	uchar		*in_buffer;
	bigtime_t	*in_timing;
	midi_parser_module_info
			*parser;
	bigtime_t	in_last_time;
	uint32		parser_state;
	int32		in_back_count;	/* cyclic buffer */
	int32		in_front_count;
	sem_id		init_sem;
	int32		open_count;
	int32		port_lock;
	int32		read_excl_cnt;
	sem_id		read_excl_sem;
	int			port;
	int32		read_cnt;
	sem_id		read_sem;
	int32		write_cnt;
	sem_id		write_sem;
	int32		write_sync_cnt;
	sem_id		write_sync_sem;
	thread_id	write_thread;
	sem_id		write_sleep;
	midi_cfg	config;
	void		(*interrupt_op)(int32 op, void * card);
	void *		interrupt_card;
	sem_id		write_quit_sem;
	uint32		workarounds;
} mpu401_dev;



#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

static status_t midi_open(void * storage, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);
static status_t midi_timed_read(mpu401_dev * port, midi_timed_data * data);
static status_t midi_timed_write(mpu401_dev * port, midi_timed_data * data);
static status_t midi_write_thread(void * arg);
static status_t midi_write_sync(mpu401_dev * port);
static status_t midi_write_clear(mpu401_dev * port);


static midi_cfg default_midi = {
	B_MIDI_DEFAULT_TIMEOUT
};


static status_t 
configure_midi(
	mpu401_dev * port,
	midi_cfg * config,
	bool force)
{
	status_t err = B_OK;

	ddprintf("mpu401: configure_midi()\n");

	/* configure device */

	if (force || (config->timeout != port->config.timeout)) {
		port->config.timeout = config->timeout;
	}

	return err;
}


static status_t
midi_open(
	void * storage,
	uint32 flags,
	void ** cookie)
{
	int ix;
	mpu401_dev * port = NULL;
	char name2[64];
	thread_id the_thread = 0;

	ddprintf("mpu401: midi_open()\n");

	*cookie = port = (mpu401_dev *)storage;
	ddprintf("mpu401: device = %x\n", port);

	if (acquire_sem(port->init_sem) < B_OK)
		return B_ERROR;
	if (atomic_add(&port->open_count, 1) == 0) {

		/* initialize device first time */
		ddprintf("mpu401: initialize device\n");

		if (get_module(B_MIDI_PARSER_MODULE_NAME, (module_info **)&port->parser) < 0) {
			dprintf("mpu401(): midi_open() can't get_module(%s)\n", B_MIDI_PARSER_MODULE_NAME);
			goto bad_mojo;
		}
		port->parser_state = 0;
		port->in_buffer = (unsigned char *)malloc(MIDI_BUF_SIZE);
		port->in_timing = (bigtime_t *)malloc(MIDI_BUF_SIZE*sizeof(bigtime_t));
		if (!port->in_timing || !port->in_buffer) {
			goto bad_mojo0;
		}
		ddprintf("MIDI port port is %x\n", port->port);
		port->port_lock = 0;
		port->in_back_count = 0;
		port->in_front_count = 0;
		port->read_cnt = 0;
		port->read_sem = create_sem(0, "mpu401 read");
		if (port->read_sem < 0) {
			goto bad_mojo1;
	/* we really should support condition variables... */
bad_mojo7:
			delete_sem(port->write_quit_sem);
bad_mojo6:
			delete_sem(port->write_sync_sem);
bad_mojo5:
			delete_sem(port->write_sleep);
bad_mojo4:
			delete_sem(port->read_excl_sem);
bad_mojo3:
			delete_sem(port->write_sem);
bad_mojo2:
			delete_sem(port->read_sem);
bad_mojo1:
			free(port->in_buffer);
			free(port->in_timing);
bad_mojo0:
			put_module(B_MIDI_PARSER_MODULE_NAME);
bad_mojo:
			port->parser = NULL;
			port->parser_state = 0;
			port->in_buffer = NULL;
			port->in_timing = NULL;
			port->open_count = 0;
			port->read_sem = -1;
			port->write_sem = -1;
			port->read_excl_sem = -1;
			port->write_sleep = -1;
			port->write_sync_sem = -1;
			return ENODEV;
		}
		set_sem_owner(port->read_sem, B_SYSTEM_TEAM);
		port->write_cnt = 1;
		port->write_sem = create_sem(1, "mpu401 write");
		if (port->write_sem < 0) {
			goto bad_mojo2;
		}
		set_sem_owner(port->write_sem, B_SYSTEM_TEAM);
		port->read_excl_cnt = 1;
		port->read_excl_sem = create_sem(0, "mpu401 read excl");
		if (port->read_excl_sem < 0) {
			goto bad_mojo3;
		}
		set_sem_owner(port->read_excl_sem, B_SYSTEM_TEAM);
		port->write_sleep = create_sem(0, "mpu401 write sleep");
		if (port->write_sleep < 0) {
			goto bad_mojo4;
		}
		set_sem_owner(port->write_sleep, B_SYSTEM_TEAM);
		port->write_sync_cnt = 0;
		port->write_sync_sem = create_sem(0, "mpu401 write sync");
		if (port->write_sync_sem < 0) {
			goto bad_mojo5;
		}
		set_sem_owner(port->write_sync_sem, B_SYSTEM_TEAM);
		port->write_quit_sem = create_sem(0, "mpu401 write quit");
		if (port->write_quit_sem < 0) {
			goto bad_mojo6;
		}
		set_sem_owner(port->write_quit_sem, B_SYSTEM_TEAM);
		sprintf(name2, "%.23s writer", "mpu401");
		ddprintf("spawning %s\n", name2);
		port->write_thread = spawn_kernel_thread(midi_write_thread, name2, 100, port);
		if (port->write_thread < 1) {
			goto bad_mojo7;
		}
		configure_midi((mpu401_dev *)*cookie, &default_midi, true);
/*		set_thread_owner(port->write_thread, B_SYSTEM_TEAM); /* */
		/* reset UART */
		ddprintf("mpu401: reset UART\n");
		PCI_IO_WR(port->port+1, 0x3f);
		EIEIO();
		spin(1);
		/* read back status */
		port->in_buffer[0] = PCI_IO_RD(port->port);
		ddprintf("port cmd ack is %x\n", (unsigned char)port->in_buffer[0]);
		(*port->interrupt_op)(B_MPU_401_ENABLE_CARD_INT, port->interrupt_card);
		ddprintf("after interrupt_op\n");
		the_thread = port->write_thread;
	}
	release_sem(port->init_sem);

	if (the_thread > 0) {
		ddprintf("mpu401: resuming thread\n");
		resume_thread(the_thread);
	}
	dprintf("mpu401: midi_open() done (count = %d)\n", port->open_count);

	return B_OK;
}


static status_t
midi_close(
	void * cookie)
{
	mpu401_dev * port = (mpu401_dev *)cookie;
	sem_id to_del = -1;
	cpu_status cp;

	ddprintf("mpu401: midi_close()\n");

	acquire_sem(port->init_sem);

	if (atomic_add(&port->open_count, -1) == 1) {

		dprintf("mpu401: %Lx: last close detected\n", system_time());

		KTRACE();

		/* un-wedge readers and writers */
		delete_sem(port->write_sem);
		port->write_sem = -1;
		delete_sem(port->read_sem);
		port->read_sem = -1;
		delete_sem(port->read_excl_sem);
		port->read_excl_sem = -1;
		delete_sem(port->write_sleep);
		port->write_sleep = -1;
		delete_sem(port->write_sync_sem);
		port->write_sync_sem = -1;
	}

	release_sem(port->init_sem);

	return B_OK;
}


static status_t
midi_free(
	void * cookie)
{
	midi_wr_q * ent;
	mpu401_dev * port = (mpu401_dev *)cookie;
	cpu_status cp;

	ddprintf("mpu401: midi_free()\n");

	acquire_sem(port->init_sem);

	if (port->open_count == 0) {
		if (port->write_thread >= 0) {
			KTRACE();
			dprintf("mpu401: %x: midiport open_count %d in midi_free()\n", system_time(), port->open_count);
			cp = disable_interrupts();
			acquire_spinlock(&port->port_lock);
	
			/* turn off UART mode */
			PCI_IO_WR(port->port+1, 0xff); /* reset */
	
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);

	/*		send_signal(SIGHUP, port->write_thread); /* */
	/*		wait_for_thread(port->write_thread, &status);	/* deadlock with psycho_killer? */ /* */
			acquire_sem(port->write_quit_sem);		/* make sure writer thread is gone */
			delete_sem(port->write_quit_sem);
			snooze(50000);					/* give thread time to die */
			port->write_thread = -1;
			/* delete all queued entries that we didn't write */
			for (ent = port->write_queue; ent!=NULL;) {
				midi_wr_q * del = ent;
				ent = ent->next;
				free(del);
			}
			port->write_queue = NULL;
			free(port->in_buffer);
			port->in_buffer = NULL;
			free(port->in_timing);
			port->in_timing = NULL;
			put_module(B_MIDI_PARSER_MODULE_NAME);

			ddprintf("mpu401: turning off card interrupts\n");
			(*port->interrupt_op)(B_MPU_401_DISABLE_CARD_INT, port->interrupt_card);
			ddprintf("after interrupt_op\n");
		}
	}

	release_sem(port->init_sem);

	return B_OK;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	mpu401_dev * port = (mpu401_dev *)cookie;
	status_t err = B_BAD_VALUE;
	midi_cfg config = port->config;

/*	ddprintf("%s: midi_control()\n", port->name); /* */

	switch (iop) {
	case B_MIDI_GET_READ_TIMEOUT:
	case B_MIDI_GET_READ_TIMEOUT_OLD:
		memcpy(data, &config.timeout, sizeof(long long));
		return B_OK;
	case B_MIDI_SET_READ_TIMEOUT:
	case B_MIDI_SET_READ_TIMEOUT_OLD:
		memcpy(&config.timeout, data, sizeof(long long));
		err = B_OK;
		break;
	case B_MIDI_TIMED_READ:
		return midi_timed_read(port, (midi_timed_data *)data);
	case B_MIDI_TIMED_WRITE:
		return midi_timed_write(port, (midi_timed_data *)data);
	case B_MIDI_WRITE_SYNC:
		return midi_write_sync(port);
	case B_MIDI_WRITE_CLEAR:
		return midi_write_clear(port);
	case 0x10203040:
		dprintf("SPECIAL: %s\n", data);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK) {
		cpu_status cp;
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->port_lock);
		err = configure_midi(port, &config, false);
		release_spinlock(&port->port_lock);
		restore_interrupts(cp);
	}
	return err;
}


static status_t
midi_read(
	void * cookie,
	off_t pos,
	void * ptr,
	size_t * nread)
{
	midi_timed_data data;
	status_t err;

/*	ddprintf("%s: midi_read(%x, %x)\n", ((mpu401_dev *)cookie)->name, ptr, nread); /* */

	data.size = *nread;
	data.data = ptr;
	err = midi_timed_read((mpu401_dev *)cookie, &data);
	*nread = data.size;
	return err;
}


static status_t
midi_write(
	void * cookie,
	off_t pos,
	const void * ptr,
	size_t * nwritten)
{
	midi_timed_data data;
	status_t err;

/*	ddprintf("%s: midi_write()\n", ((mpu401_dev *)cookie)->name); /* */

	data.when = 0;	/* that's now */
	data.size = *nwritten;
	data.data = (unsigned char *)ptr;	/* skanky cast */
	err = midi_timed_write((mpu401_dev *)cookie, &data);
	*nwritten = data.size;
	return err;
}


status_t
midi_timed_read(
	mpu401_dev * port,
	midi_timed_data * data)
{
	cpu_status cp;
	status_t err;
	unsigned char * out = data->data;
	unsigned char * end = out+data->size;
	int pos;
	int tocopy = 0;
	unsigned char byte;
	int first = 1;
	uint8 the_cmd = 0;

/*	ddprintf("%s: midi_timed_read()\n", port->name); /* */

	if (atomic_add(&port->read_excl_cnt, -1) < 1) {	/* each read must be complete */
		err = acquire_sem_etc(port->read_excl_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
		if (err < B_OK) {
			atomic_add(&port->read_excl_cnt, 1);	/* prevent a lock leak... */
			ddprintf("mpu401: read_excl_cnt returning %x\n", err);
			return err;
		}
	}
	while ((first || (tocopy > 0)) && (out < end)) {
		bigtime_t time = 0;
	verify:
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->port_lock);
		if (port->in_back_count >= port->in_front_count) {	/* wait for data */
			ddprintf("mpu401: waiting for data\n");
			atomic_add(&port->read_cnt, 1);
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);
			err = acquire_sem_etc(port->read_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
			if (err < B_OK) {
				data->size = out-data->data;
				if (atomic_add(&port->read_excl_cnt, 1) < 0) {
					release_sem(port->read_excl_sem);
				}
				ddprintf("mpu401: returning error %x\n", err);
				return err;
			}
			ddprintf("mpu401: got data\n");
			goto verify;
		}
		pos = atomic_add(&port->in_back_count, 1) & (MIDI_BUF_SIZE-1);	/* remove byte */
		byte = port->in_buffer[pos];
		if (byte & 0x80) {
			the_cmd = byte;
		}
		if (first) {
			time = port->in_timing[pos];	/* every two bytes share a timing slot */
		}
		release_spinlock(&port->port_lock);
		restore_interrupts(cp);
		if (port->parser_state || !tocopy) {
			tocopy = (*port->parser->parse)(&port->parser_state, byte, end-out);
			if (first && tocopy) {
				first = 0;
				data->when = time;
			}
		}
/*		ddprintf("tocopy = %d (byte %02x)\n", tocopy, byte); /* */
		if (tocopy != 0) {
			*(out++) = byte;
			tocopy--;
		}
		else {
/*			ddprintf("%s: nothing to copy\n", port->name); /* */
			atomic_add(&port->in_back_count, -1); /* undo the getting of byte */
		}
	}
	port->in_last_time = data->when;
/*	restore_interrupts(cp);	/* re-enable port */ /* */
	data->size = out-data->data;
	if (atomic_add(&port->read_excl_cnt, 1) < 0) {	/* let the next guy in (if any) */
		release_sem(port->read_excl_sem);
	}
	return B_OK;
}


status_t
midi_timed_write(
	mpu401_dev * port,
	midi_timed_data * data)
{
	status_t err = B_OK;
	midi_wr_q * ent, ** qhdr;
	
	if (atomic_add(&port->write_cnt, -1) < 1) {	/* lock data lock */
		status_t err = acquire_sem_etc(port->write_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
		if (err < B_OK) {
			atomic_add(&port->write_cnt, 1);
			data->size = 0;
			return err;
		}
	}
	ent = (midi_wr_q *)malloc(sizeof(midi_wr_q)+data->size-4);	/* allocate queue entry */
	if (!ent) {
		err = ENOMEM;
		goto bad_juju;
	}
	memcpy(ent->data, data->data, data->size);
	ent->size = data->size;
	ent->when = data->when;
	qhdr = &port->write_queue;
	while (*qhdr != NULL) {	/* locate where it goes */
		if ((*qhdr)->when > ent->when) {
			break;
		}
		qhdr = &(*qhdr)->next;
	}
	ent->next = *qhdr;	/* enqueue */
	*qhdr = ent;
bad_juju:
	if (atomic_add(&port->write_cnt, 1) < 0) {	/* unlock */
		release_sem(port->write_sem);
	}
	release_sem(port->write_sleep);	/* re-schedule */
	return err;
}


status_t
midi_write_sync(
	mpu401_dev * port)
{
	status_t err;

	ddprintf("mpu401: midi_write_sync()\n");

	if (atomic_add(&port->write_cnt, -1) < 1) {	/* need write lock */
		err = acquire_sem_etc(port->write_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
		if (err < B_OK) {
			atomic_add(&port->write_cnt, 1);
			return err;
		}
	}
	if (port->write_queue == NULL) {	/* nothing to write? */
		if (atomic_add(&port->write_cnt, 1) < 0) {	/* release lock */
			release_sem(port->write_sem);
		}
		ddprintf("mpu401: nothing in the write queue\n");
		return B_OK;
	}
	atomic_add(&port->write_sync_cnt, 1);	/* register our interest */
	if (atomic_add(&port->write_cnt, 1) < 0) {	/* let others get lock */
		release_sem(port->write_sem);
	}
	if ((err = acquire_sem_etc(port->write_sync_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout)) != B_OK) {
		atomic_add(&port->write_sync_cnt, -1);	/* we didn't get the sem */
	}
	ddprintf("mpu401: write_sync returns %x\n", err);
	return err;
}


status_t
midi_write_clear(
	mpu401_dev * port)
{
	status_t err;

	ddprintf("mpu401: midi_write_clear()\n");

	if (atomic_add(&port->write_cnt, -1) < 1) {	/* need write lock */
		err = acquire_sem_etc(port->write_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
		if (err < B_OK) {
			atomic_add(&port->write_cnt, 1);
			return err;
		}
	}
	while (port->write_queue != NULL) {	/* clear up the mess */
		midi_wr_q * next = port->write_queue->next;
		ddprintf("%s: removing %d bytes\n", port->write_queue->size);
		free(port->write_queue);
		port->write_queue = next;
	}
	if (atomic_add(&port->write_cnt, 1) < 0) {	/* let others get lock */
		release_sem(port->write_sem);
	}
	return B_OK;
}


status_t
midi_write_thread(
	void * arg)
{
	mpu401_dev * port = (mpu401_dev *)arg;
	status_t err;
	bigtime_t now, then;

	ddprintf("mpu401: midi_write_thread(%x)\n", arg);

	while (1) {
		midi_wr_q * ent;
		ddprintf("mpu401: midi_write_thread loop\n");
		if (atomic_add(&port->write_cnt, -1) < 1) {	/* acquire port data lock */
			ddprintf("mpu401: waiting for write lock\n");
			err = acquire_sem_etc(port->write_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("mpu401: a damsel in distress (%d %d %x)\n", 
					port->write_thread, port->write_sem, port);
				port->write_thread = -1;
				return err;
			}
		}
		now = system_time();
		if (port->write_queue) {
			then = port->write_queue->when;
		}
		else {
			then = now+B_MIDI_DEFAULT_TIMEOUT;
			while (atomic_add(&port->write_sync_cnt, -1) > 0) {	/* check for write_sync */
				release_sem(port->write_sync_sem);
				ddprintf("mpu401: signalling write queue empty\n");
			}
			atomic_add(&port->write_sync_cnt, 1); /* undo damage */
		}
		if (now < then-SNOOZE_GRANULARITY) {
			if (atomic_add(&port->write_cnt, 1) < 0) {	/* don't lock when sleeping */
				release_sem(port->write_sem);
			}
			ddprintf("mpu401: snoozing in write\n");
			err = acquire_sem_etc(port->write_sleep, 1, B_TIMEOUT|B_CAN_INTERRUPT, then-now);
			if (err && err != B_TIMED_OUT) {
				ddprintf("mpu401: I decided to end the pain [%x]\n", err);
				break;	/* die */
			}
			continue;	/* re-schedule */
		}
		else {
			ddprintf("mpu401: scheduled event found\n");
		}
		ent = port->write_queue;
		if (ent) {
			port->write_queue = ent->next;	/* dequeue */
		}
		if (atomic_add(&port->write_cnt, 1) < 0) {	/* don't lock data while writing */
			release_sem(port->write_sem);
		}
		if (ent) {
			unsigned char * ptr = ent->data;
			unsigned char * end = ptr+ent->size;
			cpu_status cp;
			KTRACE();
			cp = disable_interrupts();	/* lock the MIDI port port */
			acquire_spinlock(&port->port_lock);
			while (ptr < end) {
				unsigned char status = PCI_IO_RD(port->port+1);
				unsigned char midi_byte;
				while (status & 0x40) {
					release_spinlock(&port->port_lock);
					restore_interrupts(cp);
					err = acquire_sem_etc(port->write_sleep, 1, B_TIMEOUT|B_CAN_INTERRUPT, 2000LL);	/* 8 MIDI bytes */
					if ((err < 0) && (err != B_TIMED_OUT)) {
						ddprintf("mpu401: in trouble while writing [%x]\n", err);
						free(ent);
						goto out_of_here;	/* we're not holding any locks */
					}
					cp = disable_interrupts(cp);	/* re-acquire the lock */
					acquire_spinlock(&port->port_lock);
					status = PCI_IO_RD(port->port+1);
				}
				midi_byte = *(ptr++);
				PCI_IO_WR(port->port, midi_byte);	/* push the byte out */
			}
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);
			free(ent);	/* ent was useful, but is now spent */
		}
	}
out_of_here:
	release_sem(port->write_quit_sem);
	return B_OK;
}



bool
midi_interrupt(
	void * d)
{
	mpu401_dev * dev = (mpu401_dev *)d;
	bool ret = false;
	int fifobytes = 0;
	/* KTRACE(); /* */
	acquire_spinlock(&dev->port_lock);
	/* as long as the FIFO is not empty... */
#if 0
	ddprintf("mpu401: midi_interrupt()\n");
#endif
	if (dev->write_thread <= 0) {
		int cnt = 0;
		kprintf("mpu401: sprurious MIDI interrupt!\n");
		while (((PCI_IO_RD(dev->port+1)&0x80) == 0x00) && (cnt++ < 10)) {
			(void)PCI_IO_RD(dev->port);	/* discard data */
		}
		release_spinlock(&dev->port_lock);
		return false;
	}
	while ((PCI_IO_RD(dev->port+1)&0x80) == 0x00) {
		int32 what;
		uint8 midi_byte;
		midi_byte = PCI_IO_RD(dev->port);	/* get data */
/*		ddprintf("midi_byte is %x\n", midi_byte); /* */
		if (dev->in_front_count-dev->in_back_count >= MIDI_BUF_SIZE) {
			if (midi_byte == MIDI_ACTIVE_SENSE) {
				continue; /* discard useless data */
			}
			/* remove back-edge data */
			atomic_add(&dev->in_back_count, 1);
		}
		what = atomic_add(&dev->in_front_count, 1);
		if (midi_byte & 0x80) {
			dev->in_timing[what&(MIDI_BUF_SIZE-1)] = system_time();
		}
		dev->in_buffer[what&(MIDI_BUF_SIZE-1)] = midi_byte;
		fifobytes++;
	}
	if (fifobytes) {
		if (atomic_add(&dev->read_cnt, -1) > 0) {
			release_sem_etc(dev->read_sem, 1, B_DO_NOT_RESCHEDULE);
			ret = true;
		}
		else {
			atomic_add(&dev->read_cnt, 1);	/* undo damage */
		}
	}
	release_spinlock(&dev->port_lock);
	return ret;
}



static status_t
create_device(
	int port,
	void ** out_storage,
	uint32 workarounds, 
	void (*card_interrupt_op)(int32 op, void * user_card),
	void * user_card)
{
	mpu401_dev * dd;
	char name[32];

	KPRINTF("mpu401: create_device()\n");
	*out_storage = malloc(sizeof(mpu401_dev));
	if (!*out_storage) {
		return ENOMEM;
	}
	dd = (mpu401_dev *)*out_storage;
	memset(dd, 0, sizeof(*dd));
	dd->cookie = 'mpdv';
	dd->workarounds = workarounds;
	sprintf(name, "mpu401 %x init_sem", port & 0xffff);
	dd->init_sem = create_sem(1, name);
	if (dd->init_sem < B_OK) {
		status_t err = dd->init_sem;
		free(*out_storage);
		*out_storage = NULL;
		return err;
	}
	set_sem_owner(dd->init_sem, B_SYSTEM_TEAM);
	dd->config.timeout = B_INFINITE_TIMEOUT;
	dd->port = port;
	dd->interrupt_op = card_interrupt_op;
	dd->interrupt_card = user_card;
	return B_OK;
}

static status_t
delete_device(
	void * storage)
{
	mpu401_dev * dd = (mpu401_dev *)storage;
	ddprintf("mpu401: delete_device()\n");
	if (dd) {
		if (dd->cookie != 'mpdv') {
			dprintf("bad pointer to mpu401:delete_device(): %x\n", dd);
		}
		else {
			delete_sem(dd->init_sem);
			free(storage);
		}
	}
	return B_OK;
}



static status_t
init()
{
	isa = NULL;
	return get_module(isa_name, (module_info **)&isa);
}

static status_t
uninit()
{
	if (isa) {
		put_module(isa_name);
	}
	return B_OK;
}

static status_t
std_ops(int32 op)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		/* do nothing */
		;
	}
	return -1;
}


static generic_mpu401_module g_mpu401 = {
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
	midi_interrupt
};

_EXPORT generic_mpu401_module *modules[] = {
        &g_mpu401,
        NULL
};

