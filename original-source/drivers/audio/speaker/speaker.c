
#include "speaker.h"

#include <string.h>
#include <stdio.h>

#if !defined(_DRIVERS_H)
#include <Drivers.h>
#endif /* _DRIVERS_H */
#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */
#include <ISA.h>
#if !defined(_SUPPORT_DEFS_H)
#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */


#define EXPORT __declspec(export)
#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf (void)
#endif

typedef struct speaker_config {
	bigtime_t			byte_rate;
} speaker_config;


typedef struct speaker_dev {
	int32				open_count;
	sem_id				speaker_sem;
	speaker_config		config;
} speaker_dev;



EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);


extern device_hooks speaker_hooks;

static char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;

speaker_dev device;
char * names[2] = {
	"audio/pcbeep/1",
	NULL
};

static speaker_config default_speaker = {
	1000LL,
};


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	ddprintf("speaker: init_hardware()\n");

	return B_OK;
}


status_t
init_driver(void)
{
	uint16	count;
	
	ddprintf("speaker: init_driver()\n");

	if (get_module(isa_name, (module_info **) &isa))
		return B_ERROR;

	memset(&device, 0, sizeof(device));
	device.config = default_speaker;
	device.speaker_sem = -1;

	/* put HW in the known state: Mode 3, disabled, 880 Hz */
	(*isa->write_io_8)(SPEAKER_ISA_PORT, (*isa->read_io_8)(SPEAKER_ISA_PORT) & ~3 );	/* disable */ 
	(*isa->write_io_8)(0x43, 0xB6); 		/* Timer 2, Mode 3 */
	count = 1193180/880;					/* 880 Hz */
	(*isa->write_io_8)(0x42, count & 0xFF);	/* LSB count */
	(*isa->write_io_8)(0x42, count >> 8);	/* MSB count */

	return B_OK;
}


void
uninit_driver(void)
{
	/* disable output */
	(*isa->write_io_8)(SPEAKER_ISA_PORT, (*isa->read_io_8)(SPEAKER_ISA_PORT) & ~3 );

	memset(&device, 0, sizeof(device));
	put_module(isa_name);
}


const char **
publish_devices(void)
{
	return names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	for (ix=0; names[ix]; ix++) {
		if (!strcmp(name, names[ix])) {
			return &speaker_hooks;
		}
	}
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;

static status_t speaker_open(const char *name, uint32 flags, void **cookie);
static status_t speaker_close(void *cookie);
static status_t speaker_free(void *cookie);
static status_t speaker_control(void *cookie, uint32 op, void *data, size_t len);
static status_t speaker_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t speaker_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks speaker_hooks = {
    &speaker_open,
    &speaker_close,
    &speaker_free,
    &speaker_control,
    &speaker_read,
    &speaker_write,
	NULL,
	NULL
};


#define MIN_BYTE_RATE 20


static status_t 
configure_speaker(
	speaker_dev * speaker,
	speaker_config * config,
	bool force)
{
	status_t err = B_OK;

	ddprintf("speaker: configure_speaker()\n");

	/* argument checking */

	if (config->byte_rate < MIN_BYTE_RATE) {
		config->byte_rate = MIN_BYTE_RATE;
	}

	/* configure device */

	if (force || (config->byte_rate != speaker->config.byte_rate)) {
		speaker->config.byte_rate = config->byte_rate;
	}

	return err;
}


static status_t
speaker_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	*cookie = NULL;
	if (atomic_add(&device.open_count, 1) == 0) {
		device.speaker_sem = create_sem(1, name);
		if (device.speaker_sem < 0) {
			device.open_count = 0;
			return device.speaker_sem;
		}
	}
	*cookie = &device;
	return B_OK;
}


static status_t
speaker_close(
	void * cookie)
{
	speaker_dev * speaker = (speaker_dev *)cookie;

	if (atomic_add(&speaker->open_count, -1) == 1) 	{
		delete_sem(speaker->speaker_sem);
	}
	return B_OK;
}


static status_t
speaker_free(
	void * cookie)
{
	return B_OK;	/* already done in close */
}


static status_t
speaker_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	speaker_dev * speaker = (speaker_dev *)cookie;
	status_t err = B_BAD_VALUE;
	speaker_config config = speaker->config;

	switch (iop) {
	case SPEAKER_SET_BYTE_RATE:
		memcpy(&config.byte_rate, data, sizeof(long long));
		err = B_OK;
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK) {
		err = configure_speaker(speaker, &config, false);
	}
	return err;
}


static status_t
speaker_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
#pragma unused(cookie)
#pragma unused(pos)
#pragma unused(data)
#pragma unused(nread)
	return EPERM;
}

#define SPIN_THRESHOLD 200


static status_t
speaker_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	uchar * ptr = (uchar *)data;
	int towr = *nwritten;
	status_t err;
	speaker_dev * dev = (speaker_dev *)cookie;
	uchar old;

	ddprintf("speaker: speaker_write()\n");

	*nwritten = 0;
again:
	if ((err = acquire_sem_etc(dev->speaker_sem, 1, B_CAN_INTERRUPT, 0)) == 
		B_INTERRUPTED) {
		goto again;
	}
	if (err < B_OK) {
		return err;
	}
	old = (*isa->read_io_8)(SPEAKER_ISA_PORT)&~3;
	while (towr > 0) {
		(*isa->write_io_8)(SPEAKER_ISA_PORT, old|(3&*(ptr++)));
		towr--;
		(*nwritten)++;
		if (!towr) {
			break;
		}
		if (dev->config.byte_rate < SPIN_THRESHOLD) {
			spin(dev->config.byte_rate);	/* spin with interrupts enabled */
		}
		else {
			if (acquire_sem_etc(dev->speaker_sem, 1, B_TIMEOUT|B_CAN_INTERRUPT, 
				dev->config.byte_rate) < B_OK) {
				break;
			}
		}
	}
	release_sem(dev->speaker_sem);
	return B_OK;
}

