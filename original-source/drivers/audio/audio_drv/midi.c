/************************************************************************/
/*                                                                      */
/*                              midi.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#include "device.h"
#include "debug.h"
#include "midi_driver.h"

#include "audio_card.h"


extern generic_mpu401_module * mpu401;


void
midi_interrupt_op(
	int32 op,
	void * data)
{
	midi_dev * midi = (midi_dev *)data;
	if (op == B_MPU_401_ENABLE_CARD_INT) {
		DB_PRINTF(("yamaha: B_MPU_401_ENABLE_CARD_INT\n"));
		(*midi->card->func->enable_midi_receiver_interrupts)(midi->card);
	}
	else if (op == B_MPU_401_DISABLE_CARD_INT) {
		DB_PRINTF(("yamaha: B_MPU_401_DISABLE_CARD_INT\n"));
		(*midi->card->func->disable_midi_receiver_interrupts)(midi->card);
	}
	DB_PRINTF(("yamaha: midi_interrupt_op() done\n"));
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
	int ret = B_OK;
	midi_dev *midi = NULL;
	DB_PRINTF(("midi_open()\n"));

	midi = (midi_dev*)FindDeviceCookie(name);
	*cookie = (void *)midi;
	if(!midi) return ENODEV; 

	if(atomic_add(&midi->count, 1) == 0) (*midi->card->func->enable_midi)(midi->card);		
	ret = (*mpu401->open_hook)(midi->driver, flags, &midi->cookie);
	if (ret != B_OK) atomic_add(&midi->count, -1); //restore value
	return ret;
}


static status_t
midi_close(
	void * cookie)
{
	midi_dev* midi = (midi_dev*)cookie;
	DB_PRINTF((" midi_close()\n"));
	return (*mpu401->close_hook)(midi->cookie);
}


static status_t
midi_free(
	void * cookie)
{
	midi_dev* midi = (midi_dev*)cookie;
	status_t ret;
	DB_PRINTF(("midi_free()\n"));
	ret = (*mpu401->free_hook)(midi->cookie);
	if (atomic_add(&midi->count, -1) == 1) { //last free() call
		DB_PRINTF(("yamaha: midi_free() Last!!!\n"));
		midi->cookie = NULL;
		(midi->card->func->disable_midi)(midi->card);			
	}
	DB_PRINTF(("midi_free() return %ld\n",ret));
	return ret;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	midi_dev* midi = (midi_dev*)cookie;
	status_t ret;
	DB_PRINTF1(("midi_control: iop = &lx\n",iop));
	ret = (*mpu401->control_hook)(midi->cookie, iop, data, len);
	DB_PRINTF1(("midi_control done: iop = &lx\n",iop));
	return ret;
}


static status_t
midi_read(
	void * cookie,
	off_t pos,
	void * ptr,
	size_t * nread)
{
	midi_dev* midi = (midi_dev*)cookie;
	status_t ret;
	DB_PRINTF1(("midi_read(): nread = %ld\n",*nread));
	ret = (*mpu401->read_hook)(midi->cookie, pos, ptr, nread);
	DB_PRINTF1(("midi_read(): nread = %ld return %ld; \n",*nread, ret));
	return ret;
}


static status_t
midi_write(
	void * cookie,
	off_t pos,
	const void * ptr,
	size_t * nwritten)
{
	midi_dev* midi = (midi_dev*)cookie;
	status_t ret;
	DB_PRINTF1(("midi_write(): size = %ld\n",*nwritten));
	ret = (*mpu401->write_hook)(midi->cookie, pos, ptr, nwritten);
	DB_PRINTF1(("midi_write(): nwritten = %ld, return %ld\n",*nwritten,ret));
	return ret;
}


bool midi_interrupt(
	sound_card_t * card)
{
	bool ret;
	midi_dev* midi = &((audio_card_t*)card->host)->midi;
	DB_PRINTF2(("midi_interrupt()\n"));
	if (!midi->driver)  {
		DB_PRINTF(("midi_interrupt(): Error! Midi device was not created! return false;\n"));
		return false;
	}
	ret =(*mpu401->interrupt_hook)(midi->driver);
	DB_PRINTF2(("midi_interrupt(): return %d\n",ret));
	return ret;
}

