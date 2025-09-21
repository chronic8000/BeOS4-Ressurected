
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "opti_private.h"

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

extern generic_mpu401_module * mpu401;

void
midi_interrupt_op(
	int32 op,
	void * data)
{
	midi_dev * port = (midi_dev *)data;
	ddprintf("port = %x\n", port);
	if (op == B_MPU_401_ENABLE_CARD_INT) {
	  ddprintf(CHIP_NAME " : B_MPU_401_ENABLE_CARD_INT\n");
	  if (atomic_add(&port->midi_inth_count, 1) == 0) {
		ddprintf("installing IRQ%d handler for %s\n", port->IRQ, port->name);
		install_io_interrupt_handler(port->IRQ, midi_int_handler, port->card, 0);
	  }
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT) {
	  ddprintf(CHIP_NAME " : B_MPU_401_DISABLE_CARD_INT\n");
	  if (atomic_add(&port->midi_inth_count, -1) == 1) {
		ddprintf("uninstalling IRQ%d handler for %s\n", port->IRQ, port->name);
		remove_io_interrupt_handler(port->IRQ, midi_int_handler, port->card);
	  }
	}
	ddprintf(CHIP_NAME " : midi_interrupt_op() done\n");
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

	ddprintf(CHIP_NAME " : midi_open()\n");

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].midi.name)) {
			break;
		}
	}
	if (ix >= num_cards) {
		ddprintf("bad device\n");
		return ENODEV;
	}

	ddprintf(CHIP_NAME " : mpu401: %x  open(): %x  driver: %x\n", mpu401, mpu401->open_hook, cards[ix].midi.driver);
	ret = (*mpu401->open_hook)(cards[ix].midi.driver, flags, cookie);
	if (ret >= B_OK) {
		cards[ix].midi.cookie = *cookie;
		atomic_add(&cards[ix].midi.count, 1);
	}
	ddprintf(CHIP_NAME " : mpu401: open returns %x\n", ret);
	return ret;
}


static status_t
midi_close(
	void * cookie)
{
	ddprintf(CHIP_NAME " : midi_close()\n");
	return (*mpu401->close_hook)(cookie);
}


static status_t
midi_free(
	void * cookie)
{
	int ix;
	status_t f;
	ddprintf(CHIP_NAME " : midi_free()\n");
	f = (*mpu401->free_hook)(cookie);
	for (ix=0; ix<num_cards; ix++) {
		if (cards[ix].midi.cookie == cookie) {
			if (atomic_add(&cards[ix].midi.count, -1) == 1) {
				cards[ix].midi.cookie = NULL;
				ddprintf("cleared %x card %d\n", cookie, ix);
			}
			break;
		}
	}
	ddprintf(CHIP_NAME " : midi_free() done\n");
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


int32
midi_int_handler(void* data)
{
  sb_card* sb = (sb_card*) data;

  if (sb->midi.driver)
	if ((*mpu401->interrupt_hook)(sb->midi.driver))
	  return B_INVOKE_SCHEDULER;

  return B_UNHANDLED_INTERRUPT;
}
