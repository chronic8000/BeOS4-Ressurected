
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "ess1938.h"

extern generic_mpu401_module * mpu401;

void
midi_interrupt_op(
	int32 op,
	void * data)
{

	midi_dev * port = (midi_dev *)data;
	if (op == B_MPU_401_ENABLE_CARD_INT) {
		cpu_status cp;
		cp = disable_interrupts();
		acquire_spinlock(&port->card->spinlock);
		increment_interrupt_handler(port->card);
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
						port->card->sbbase + MIXER_DATA,
						MASTER_VOL_CTRL,
						0x40,
						0x40);
		/* IMR_ISR mask should ALREADY be enabled */				
		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);
		ddprintf("ess_1938: MPU enable ints\n");
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT) {
		/* turn off MPU interrupts */
		cpu_status cp;
		cp = disable_interrupts();
		acquire_spinlock(&port->card->spinlock);
		set_indirect(	port->card->sbbase + MIXER_COMMAND,
						port->card->sbbase + MIXER_DATA,
						MASTER_VOL_CTRL,
						0x00,
						0x40);
		/* IMR_ISR left enabled */
		/* remove interrupt handler if necessary */
		decrement_interrupt_handler(port->card);
		release_spinlock(&port->card->spinlock);
		restore_interrupts(cp);
		ddprintf("ess_1938: MPU disable ints\n");
	}
}

static status_t midi_open(const char *name, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);


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

static status_t
midi_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	int ret;

	ddprintf("ess_1938: midi_open()\n");

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].midi.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		ddprintf("ess_1938: Midi bad device\n");
		return ENODEV;
	}

	ret = (*mpu401->open_hook)(cards[ix].midi.driver, flags, cookie);
	if (ret >= B_OK) {
		cards[ix].midi.cookie = *cookie;
		atomic_add(&cards[ix].midi.count, 1);
	}
	ddprintf("ess_1938: mpu401: open returns %x\n", ret);
	return ret;
}


static status_t
midi_close(
	void * cookie)
{
	ddprintf("ess_1938: midi_close()\n");
	return (*mpu401->close_hook)(cookie);
}


static status_t
midi_free(
	void * cookie)
{
	int ix;
	status_t f;
	ddprintf("ess_1938: midi_free()\n");
	f = (*mpu401->free_hook)(cookie);
	for (ix=0; ix<num_cards; ix++) {
		if (cards[ix].midi.cookie == cookie) {
			if (atomic_add(&cards[ix].midi.count, -1) == 1) {
				cards[ix].midi.cookie = NULL;
				ddprintf("ess_1938: midi_free cleared %x card %d\n", cookie, ix);
			}
			break;
		}
	}
	return f;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	return (*mpu401->control_hook)(cookie, iop, data, len);
}


static status_t
midi_read(
	void * cookie,
	off_t pos,
	void * ptr,
	size_t * nread)
{
	return (*mpu401->read_hook)(cookie, pos, ptr, nread);
}


static status_t
midi_write(
	void * cookie,
	off_t pos,
	const void * ptr,
	size_t * nwritten)
{
	return (*mpu401->write_hook)(cookie, pos, ptr, nwritten);
}


bool
midi_interrupt(
	ess1938_dev * dev)
{
	if (!dev->midi.driver) return false;
	return (*mpu401->interrupt_hook)(dev->midi.driver);
}

