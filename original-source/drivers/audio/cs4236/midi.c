
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "private.h"


#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

static status_t midi_open(const char *name, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);
static status_t midi_timed_read(midi_dev * port, midi_timed_data * data);
static status_t midi_timed_write(midi_dev * port, midi_timed_data * data);
static status_t midi_write_thread(void * arg);
static status_t midi_write_sync(midi_dev * port);
static status_t midi_write_clear(midi_dev * port);


device_hooks midi_hooks = {
    &midi_open,
    &midi_close,
    &midi_free,
    &midi_control,
    &midi_read,
    &midi_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};

static midi_cfg default_midi = {
	B_MIDI_DEFAULT_TIMEOUT
};


typedef struct {
  midi_dev* port;
  uint32 flags;
} midi_cookie;


static uchar
midi_command_write(midi_dev* port, uchar command)
{
  bigtime_t bail = system_time() + 1000;

  while (0x40 & READ_IO_8(port->base + 1))
	if (system_time() > bail) {
	  dprintf (CHIP_NAME ":  MPU-401 command write timed out.\n");
	  return 0;
	}

  ddprintf("MPU-401 writing 0x%02x\n", command);
  WRITE_IO_8(port->base + 1, command);

  /* The need for this is purely empirical */
  if (command == 0xff)
	WRITE_IO_8(port->base + 1, command);

  bail = system_time() + 1000;
  while (0x80 & READ_IO_8(port->base + 1))
	if (system_time() > bail) {
	  dprintf (CHIP_NAME ":  MPU-401 command ack timed out.\n");
	  return 0;
	}

  return READ_IO_8(port->base);
}


static status_t
midi_command(midi_dev* port, uchar command)
{
  uint8 ack;
  cpu_status ps = lock_spinner(&port->port_lock);
  ack = midi_command_write(port, command);
  unlock_spinner(&port->port_lock, ps);
  return (ack == 0xfe ? B_NO_ERROR : B_ERROR);
}


static status_t 
configure_midi(midi_dev* port, midi_cfg* config, bool force)
{
  ddprintf("%s: configure_midi()\n", port->name);

  /* configure device */

  if (force || (config->timeout != port->config.timeout))
	port->config.timeout = config->timeout;

  return B_OK;
}


static status_t
midi_open(const char* name, uint32 flags, void** cookie)
{
  int i;
  midi_dev* port = NULL;
  midi_cookie* mc;

  ddprintf(CHIP_NAME ": midi_open()\n");

  *cookie = NULL;
  for (i = 0; i < num_cards; i++)
	if (!strcmp(name, cards[i].midi.name))
	  break;
  if (i >= num_cards)
	return ENODEV;

  port = &cards[i].midi;

  ddprintf("Midi Base Address: %x\n", port->base);
  if (port->base == 0)
	return ENODEV;
  mc = (midi_cookie*) malloc(sizeof(midi_cookie));
  if (mc == NULL)
	return ENODEV;

  if (atomic_add(&port->open_count, 1) == 0) {
	/* initialize device first time */
	if (get_module(B_MIDI_PARSER_MODULE_NAME, (module_info **)&port->parser) < 0) {
	  dprintf(CHIP_NAME ": midi_open() can't get_module(%s)\n", B_MIDI_PARSER_MODULE_NAME);
	  goto bad_mojo;
	}

	port->parser_state = 0;
	port->card = &cards[i];
	port->port_lock = 0;
	if (midi_command(port, 0xff) != B_NO_ERROR)
	  goto bad_mojo;
	if (midi_command(port, 0x3f) != B_NO_ERROR)
	  goto bad_mojo;
	port->in_buffer = (unsigned char*) malloc(MIDI_BUF_SIZE);
	port->in_timing = (bigtime_t*) malloc((MIDI_BUF_SIZE/2)*sizeof(bigtime_t));
	if (!port->in_timing || !port->in_buffer)
	  goto bad_mojo0;

	port->in_back_count = 0;
	port->in_front_count = 0;
	port->read_cnt = 0;
	port->read_sem = create_sem(0, port->name);
	if (port->read_sem < 0) {
	  goto bad_mojo1;
bad_mojo6:	/* we really should support condition variables... */
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
	  free(mc);
	  atomic_add (&port->open_count, -1);
	  ddprintf(CHIP_NAME ": midi_open(): failed\n");
	  return ENODEV;
	}
	set_sem_owner(port->read_sem, B_SYSTEM_TEAM);
	port->write_cnt = 1;
	port->write_sem = create_sem(1, port->name);
	if (port->write_sem < 0)
	  goto bad_mojo2;
	set_sem_owner(port->write_sem, B_SYSTEM_TEAM);
	port->read_excl_cnt = 1;
	port->read_excl_sem = create_sem(0, port->name);
	if (port->read_excl_sem < 0)
	  goto bad_mojo3;
	set_sem_owner(port->read_excl_sem, B_SYSTEM_TEAM);
	configure_midi(port, &default_midi, true);
	port->write_sleep = create_sem(0, port->name);
	if (port->write_sleep < 0)
	  goto bad_mojo4;
	set_sem_owner(port->write_sleep, B_SYSTEM_TEAM);
	port->write_sync_cnt = 0;
	port->write_sync_sem = create_sem(0, port->name);
	if (port->write_sync_sem < 0)
	  goto bad_mojo5;
	set_sem_owner(port->write_sync_sem, B_SYSTEM_TEAM);
	port->write_thread = spawn_kernel_thread(midi_write_thread, port->name, 100, port);
	if (port->write_thread < 1)
	  goto bad_mojo6;
/*	set_thread_owner(port->write_thread, B_SYSTEM_TEAM); /* */
	resume_thread(port->write_thread);
  }

  *cookie = mc;
  mc->port = port;
  mc->flags = flags;

  if ((flags & O_RWMASK) != O_WRONLY)
	if (atomic_add(&port->reader_count, 1) == 0) {
	  cpu_status cp;
	  ddprintf("installing IRQ%d handler for %s MIDI\n", port->IRQ, port->card->name);
	  install_io_interrupt_handler(port->IRQ, midi_interrupt, port->card, 0);
	  cp = disable_interrupts();
	  midi_interrupt(port->card);	/* prime the pump */
	  restore_interrupts(cp);
	}

  ddprintf("%s: midi_open() done (count = %d)\n", port->name, port->open_count);

  return B_OK;
}


static status_t
midi_close(void* cookie)
{
  midi_cookie* mc = (midi_cookie*) cookie;
  midi_dev* port = mc->port;

  ddprintf("%s: midi_close()\n", port->name);

  if ((mc->flags & O_RWMASK) != O_WRONLY)
	if (atomic_add(&port->reader_count, -1) == 1) {
	  ddprintf("uninstalling IRQ%d handler for %s MIDI\n", port->IRQ, port->card->name);
	  remove_io_interrupt_handler(port->IRQ, midi_interrupt, port->card);
	}

  if (atomic_add(&port->open_count, -1) == 1) {

	ddprintf("%s: %x: last close detected\n", port->name, system_time());

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
//	send_signal(SIGHUP, port->write_thread);
//  wait_for_thread(port->write_thread, &status);	/* deadlock with psycho_killer! */
	snooze(50000);	/* give thread time to die */
	port->write_thread = -1;
  }

  return B_OK;
}


static status_t
midi_free(void* cookie)
{
  midi_wr_q * ent;
  midi_dev* port = ((midi_cookie*)cookie)->port;

  ddprintf("%s: midi_free()\n", port->name);

  /* turn off UART mode */
  midi_command(port, 0xff);		/* reset */

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

  free(cookie);
  return B_OK;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	midi_dev* port = ((midi_cookie*)cookie)->port;
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
	midi_dev* port = ((midi_cookie*)cookie)->port;
	midi_timed_data data;
	status_t err;

/*	ddprintf("%s: midi_read(%x, %x)\n", port->name, ptr, nread); /* */

	data.size = *nread;
	data.data = ptr;
	err = midi_timed_read(port, &data);
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
	midi_dev* port = ((midi_cookie*)cookie)->port;
	midi_timed_data data;
	status_t err;

/*	ddprintf("%s: midi_write()\n", port->name); /* */

	data.when = 0;	/* that's now */
	data.size = *nwritten;
	data.data = (unsigned char *)ptr;	/* skanky cast */
	err = midi_timed_write(port, &data);
	*nwritten = data.size;
	return err;
}


status_t
midi_timed_read(
	midi_dev * port,
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
			ddprintf("%s: read_excl_cnt returning %x\n", port->name, err);
			return err;
		}
	}
	while ((first || (tocopy > 0)) && (out < end)) {
		bigtime_t time;
	verify:
		cp = disable_interrupts();
		acquire_spinlock(&port->port_lock);
		if (port->in_back_count >= port->in_front_count) {	/* wait for data */
			ddprintf("%s: waiting for data\n", port->name);
			atomic_add(&port->read_cnt, 1);
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);
			err = acquire_sem_etc(port->read_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout);
			if (err < B_OK) {
				data->size = out-data->data;
				if (atomic_add(&port->read_excl_cnt, 1) < 0) {
					release_sem(port->read_excl_sem);
				}
				ddprintf("%s: returning error %x\n", port->name, err);
				return err;
			}
			ddprintf("%s: got data\n", port->name);
			goto verify;
		}
		pos = atomic_add(&port->in_back_count, 1) & (MIDI_BUF_SIZE-1);	/* remove byte */
		byte = port->in_buffer[pos];
		if (byte & 0x80) {
			the_cmd = byte;
		}
		if (first) {
			if (byte & 0x80) {
				time = port->in_timing[pos/2];	/* every two bytes share a timing slot */
			}
			else {
				time = port->in_last_time;
			}
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
	midi_dev * port,
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
	midi_dev * port)
{
	status_t err;

	ddprintf("%s: midi_write_sync()\n", port->name);

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
		ddprintf("%s: nothing in the write queue\n", port->name);
		return B_OK;
	}
	atomic_add(&port->write_sync_cnt, 1);	/* register our interest */
	if (atomic_add(&port->write_cnt, 1) < 0) {	/* let others get lock */
		release_sem(port->write_sem);
	}
	if ((err = acquire_sem_etc(port->write_sync_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, port->config.timeout)) != B_OK) {
		atomic_add(&port->write_sync_cnt, -1);	/* we didn't get the sem */
	}
	ddprintf("%s: write_sync returns %x\n", port->name, err);
	return err;
}


status_t
midi_write_clear(
	midi_dev * port)
{
	status_t err;

	ddprintf("%s: midi_write_clear()\n", port->name);

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
	midi_dev * port = (midi_dev *)arg;
	status_t err;
	bigtime_t now, then;

	ddprintf("%s: midi_write_thread(%x)\n", port->name, arg);

	while (1) {
		midi_wr_q * ent;
		ddprintf("%s: midi_write_thread loop", port->name);
		if (atomic_add(&port->write_cnt, -1) < 1) {	/* acquire port data lock */
			ddprintf("%s: waiting for write lock\n", port->name);
			err = acquire_sem_etc(port->write_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("%s: a damsel in distress (%d %d %x)\n", port->name, 
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
				ddprintf("%s: signalling write queue empty\n", port->name);
			}
			atomic_add(&port->write_sync_cnt, 1); /* undo damage */
		}
		if (now < then-SNOOZE_GRANULARITY) {
			if (atomic_add(&port->write_cnt, 1) < 0) {	/* don't lock when sleeping */
				release_sem(port->write_sem);
			}
			//ddprintf("%s: snoozing in write\n", port->name);
			err = acquire_sem_etc(port->write_sleep, 1, B_TIMEOUT|B_CAN_INTERRUPT, then-now);
			if (err < 0 && err != B_TIMED_OUT) {
				ddprintf("%s: I decided to end the pain [%x]\n", port->name, err);
				break;	/* die */
			}
			continue;	/* re-schedule */
		}
		else {
			//ddprintf("%s: scheduled event found\n", port->name);
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
			cp = disable_interrupts();	/* lock the MIDI port port */
			acquire_spinlock(&port->port_lock);
			while (ptr < end) {
				unsigned char status = READ_IO_8(port->base+1);
				unsigned char midi_byte;
				while (status & 0x40) {
					release_spinlock(&port->port_lock);
					restore_interrupts(cp);
					err = acquire_sem_etc(port->write_sleep, 1, B_TIMEOUT|B_CAN_INTERRUPT, 2000LL);	/* 8 MIDI bytes */
					if ((err < 0) && (err != B_TIMED_OUT)) {
						ddprintf("%s: in trouble while writing [%x]\n", port->name, err);
						free(ent);
						goto out_of_here;	/* we're not holding any locks */
					}
					cp = disable_interrupts(cp);	/* re-acquire the lock */
					acquire_spinlock(&port->port_lock);
					status = READ_IO_8(port->base+1);
				}
				midi_byte = *(ptr++);
				WRITE_IO_8(port->base, midi_byte);	/* push the byte out */
			}
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);
			free(ent);	/* ent was useful, but is now spent */
		}
	}
out_of_here:
	return B_OK;
}


int32
midi_interrupt(void* port)
{
	sb_card* dev = (sb_card*) port;
	int32 handled = B_UNHANDLED_INTERRUPT;
	int fifobytes = 0;

	//ddprintf(CHIP_NAME ": midi_interrupt()\n");

	acquire_spinlock(&dev->midi.port_lock);

	if (dev->midi.write_thread <= 0) {
		int cnt = 0;
		kprintf(CHIP_NAME ": sprurious MIDI interrupt!\n");
		while ((READ_IO_8(dev->midi.base+1) & 0x80) == 0x00
			   && cnt++ < 10) {
			handled = B_HANDLED_INTERRUPT;
			(void) READ_IO_8(dev->midi.base);	/* discard data */
		}
		release_spinlock(&dev->midi.port_lock);
		return handled;
	}

	while ((READ_IO_8(dev->midi.base+1) & 0x80) == 0x00) {
		int32 what;
		uint8 midi_byte;
		handled = B_HANDLED_INTERRUPT;
		midi_byte = READ_IO_8(dev->midi.base);	/* get data */
/*		ddprintf("midi_byte is %x\n", midi_byte); /* */
		if (dev->midi.in_front_count-dev->midi.in_back_count >= MIDI_BUF_SIZE) {
			if (midi_byte == MIDI_ACTIVE_SENSE) {
				continue; /* discard useless data */
			}
			/* remove back-edge data */
			atomic_add(&dev->midi.in_back_count, 1);
		}
		what = atomic_add(&dev->midi.in_front_count, 1);
		if (midi_byte & 0x80) {	/* every two bytes share one timing slot */
			dev->midi.in_timing[(what&(MIDI_BUF_SIZE-1))/2] = system_time();
		}
		dev->midi.in_buffer[what&(MIDI_BUF_SIZE-1)] = midi_byte;
		fifobytes++;
	}

	if (fifobytes) {
		if (atomic_add(&dev->midi.read_cnt, -1) > 0) {
			release_sem_etc(dev->midi.read_sem, 1, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			atomic_add(&dev->midi.read_cnt, 1);	/* undo damage */
		}
	}

	release_spinlock(&dev->midi.port_lock);
	return handled;
}

