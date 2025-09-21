#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <scsi.h>
#include <malloc.h>
#include <ByteOrder.h>
#include <ISA.h>

/*

Aqua buttons:

SCROLL_UP  GPIO0  (observed GPIO1-4)
SCROLL_DN  GPIO1  .
SCROLL_LT  GPIO2  .
SCROLL_RT  GPIO3  .
ENTER      GPIO14
Function   GPIO15 (observed GPIO6)

*/

static uint8            bus, device, function;
static timer            button_timer;
static sem_id           sample_ready_sem;
static int              sample_queue_head;
static int              sample_queue_tail;
static int              dropped_samples;


status_t
init_driver()
{
	return B_OK;
}

void
uninit_driver()
{
}

status_t
aqua_buttons_open(const char *name, uint32 flags, void **cookie)
{
	return B_NO_ERROR;
}

status_t
aqua_buttons_close(void *cookie)
{
	return B_NO_ERROR;
}

status_t
aqua_buttons_free(void *cookie)
{
	return B_NO_ERROR;
}

status_t
aqua_buttons_control(void *cookie, uint32 op, void *data, size_t len)
{
	return B_ERROR;
}

status_t
aqua_buttons_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	return B_ERROR;
}

status_t
aqua_buttons_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	return B_ERROR;
}

static device_hooks aqua_buttons_device_hooks = {
	&aqua_buttons_open,
	&aqua_buttons_close,
	&aqua_buttons_free,
	&aqua_buttons_control, /* control */
	&aqua_buttons_read,
	&aqua_buttons_write, /* write */
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	NULL, /* writev */
};

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	NULL,
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
	dprintf("aqua_buttons: find device, %s\n", name);
	return &aqua_buttons_device_hooks;
}

status_t
init_hardware()
{
	int err;
	isa_module_info *isa;

	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR) {
		return B_ERROR;
	}

	if((isa->read_io_8(0xe4) & 0x01) == 0) {
		device_name_list[0]= "misc/fic_video_switch/pal";
	} else {
		device_name_list[0]= "misc/fic_video_switch/ntsc";
	}

	put_module(B_ISA_MODULE_NAME);

	return B_OK;
}
