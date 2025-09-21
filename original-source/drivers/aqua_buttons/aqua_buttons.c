#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <scsi.h>
#include <malloc.h>
#include <ByteOrder.h>
#include <PCI.h>

/*

Aqua buttons:

SCROLL_UP  GPIO0  (observed GPIO1-4)
SCROLL_DN  GPIO1  .
SCROLL_LT  GPIO2  .
SCROLL_RT  GPIO3  .
ENTER      GPIO14
Function   GPIO15 (observed GPIO6)

*/

typedef struct {
	bigtime_t  time;
	uint8      buttons;
	uint8      reserved[3+4];
} sample_t;

#define SAMPLE_QUEUE_SIZE 16
pci_module_info *pci;
uint8            bus, device, function;
timer            button_timer;
sem_id           sample_ready_sem;
sample_t         sample_queue[SAMPLE_QUEUE_SIZE];
int              sample_queue_head;
int              sample_queue_tail;
int              dropped_samples;

static status_t
add_sample(sample_t sample)
{
	if((sample_queue_tail + 1) % SAMPLE_QUEUE_SIZE == sample_queue_head) {
		dropped_samples++;
		return B_ERROR;
	}
	sample_queue[sample_queue_tail] = sample;
	sample_queue_tail = (sample_queue_tail + 1) % SAMPLE_QUEUE_SIZE;
	release_sem_etc(sample_ready_sem, 1, B_DO_NOT_RESCHEDULE);
	return B_NO_ERROR;
}

static int32
poll_buttons(timer *t)
{
	static sample_t prev_sample;
	static sample_t curr_sample;

	status_t err;
	uint8 gpi_7_0;
	uint8 gpi_8_15;

	gpi_7_0 = pci->read_pci_config(bus, device, function, 0x97, 1);
	gpi_8_15 = pci->read_pci_config(bus, device, function, 0x98, 2);
	curr_sample.time = system_time();
	curr_sample.buttons = ((~gpi_7_0  & 0x1e) >> 1)
	                    | ((~gpi_7_0  & 0x40) >> 1)
	                    | ((~gpi_8_15 & 0x40) >> 2);
	//dprintf("poll_buttons: got 0x%02x (0x%02x 0x%02x)\n", curr_sample.buttons,
	//        gpi_7_0, gpi_8_15);
	if(curr_sample.buttons != prev_sample.buttons) {
		err = add_sample(curr_sample);
		if(err == B_NO_ERROR)
			prev_sample = curr_sample;
	}
	return B_HANDLED_INTERRUPT;
}

/* driver hooks */

status_t
init_driver()
{
	status_t err;
	pci_info info;
	int32    pci_index = 0;
	
	dprintf("aqua_buttons: init_driver\n");
	err = get_module(B_PCI_MODULE_NAME, (module_info **)&pci);
	if(err != B_NO_ERROR)
		goto err1;

	while(pci->get_nth_pci_info(pci_index++, &info) == B_NO_ERROR) {
		if(info.vendor_id == 0x10b9 &&
		   info.device_id == 0x7101 &&
		   info.function == 0) {
			bus = info.bus;
			device = info.device;
			function = info.function;
			goto found_card;
		}
	}
	err = B_ERROR;
	goto err2;

found_card:
	err = sample_ready_sem = create_sem(0, "aqua_buttons_intsem");
	if(err < B_NO_ERROR)
		goto err2;

	sample_queue_head = 0;
	sample_queue_tail = 0;
	dropped_samples = 0;
	
	err = add_timer(&button_timer, &poll_buttons, 100000, B_PERIODIC_TIMER);

	if(err != B_NO_ERROR)
		goto err3;
		
	return B_NO_ERROR;

//err4:
//	cancel_timer(&button_timer);
err3:
	delete_sem(sample_ready_sem);
err2:
	put_module(B_PCI_MODULE_NAME);
err1:
	dprintf("aqua_buttons: init_driver failed\n");
	return err;
}

void
uninit_driver()
{
	dprintf("aqua_buttons: uninit_driver\n");
	dprintf("aqua_buttons: uninit_driver: %d sampled dropped\n", dropped_samples);
	cancel_timer(&button_timer);
	delete_sem(sample_ready_sem);
	put_module(B_PCI_MODULE_NAME);
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
	//status_t err;
	switch(op) {
		default:
			dprintf("aqua_buttons: control %lx\n", op);
			return B_ERROR;
	}
}

status_t
aqua_buttons_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	status_t err;
	
	if(*numBytes < sizeof(sample_t)) {
		*numBytes = 0;
		return B_ERROR;
	}
	
	err = acquire_sem_etc(sample_ready_sem, 1, B_CAN_INTERRUPT, 0);
	if(err != B_NO_ERROR) {
		*numBytes = 0;
		return err;
	}
	else {
		*((sample_t *)data) = sample_queue[sample_queue_head];
		sample_queue_head = (sample_queue_head + 1) % SAMPLE_QUEUE_SIZE;
	}
	
	//dprintf("got buttons time %Ld: 0x%02x\n",
	//        ((sample_t *)data)->time,
	//        ((sample_t *)data)->buttons);
	*numBytes = sizeof(sample_t);
	return B_NO_ERROR;
}

status_t
aqua_buttons_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	*numBytes = 0;
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
	"input/buttons/aqua_buttons/0",
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
