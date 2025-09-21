#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>

//#define ANNOY(args) dprintf args
#define ANNOY(args) (void)0

#define BYLINE "touch: "
#define DRIVER_NAME "acer_wp_touch"
#define TOUCH_IO_BASE 0x3F8
#define TOUCH_IRQ 4
#define SAMPLE_QUEUE_SIZE 4 // TT: never seems to be more than one in queue

typedef struct {
	bigtime_t time;
	uint16 x;
	uint16 y;
	bool pressed;
} sample_t;

isa_module_info *isa;
sem_id sample_ready_sem;
sample_t sample_queue[SAMPLE_QUEUE_SIZE];
int sample_queue_head;
int sample_queue_tail;
int dropped_samples;
bool pressed;

#if 0
static void dump_sample(const sample_t* const sample)
{
	dprintf(BYLINE "time = %Ld\n", sample->time);
	dprintf(BYLINE "x = %u\n", sample->x);
	dprintf(BYLINE "y = %u\n", sample->y);
	dprintf(BYLINE "pressed = %x\n", sample->pressed);
}
#endif	

static status_t
add_sample(sample_t sample)
{
	if ((sample_queue_tail + 1) % SAMPLE_QUEUE_SIZE == sample_queue_head) {
		dprintf(BYLINE "We've dropped %d samples, Captain!\n", dropped_samples);
		dropped_samples++;
		return B_ERROR;
	}
	
	sample_queue[sample_queue_tail] = sample;
	sample_queue_tail = (sample_queue_tail + 1) % SAMPLE_QUEUE_SIZE;
	
	return B_OK;
}

static int32
touch_interrupt_handler(void *data)
{
	status_t err;
	static int int_count = 0;
	static sample_t prev_sample = { /*time*/-1, 0xFFFF, 0xFFFF, false };
	static sample_t curr_sample;
	uint8 byte;
	
	byte = isa->read_io_8(TOUCH_IO_BASE);
	
	switch (int_count) {
		case 0:
			if (byte != 0xFF) {
				ANNOY((BYLINE
					"0x%02x isn't the byte you're looking for.\n", byte));
				goto err;
			}
			curr_sample.time = system_time();
			curr_sample.pressed = true;
			curr_sample.x = curr_sample.y = 0;
			break;
		case 1:
			//ANNOY((BYLINE "x low byte is %x\n", byte));
			curr_sample.x = byte >> 6;
			break;
		case 2:
			//ANNOY((BYLINE "x high byte is %x\n", byte));
			curr_sample.x |= byte << 2;
			break;
		case 3:
			//ANNOY((BYLINE "y low byte is %x\n", byte));
			curr_sample.y = byte >> 6;
			break;
		case 4:
			//ANNOY((BYLINE "y high byte is %x\n", byte));
			curr_sample.y |= byte << 2;
			//dump_sample(&curr_sample);
			
			if (prev_sample.time == -1 && prev_sample.x == 0xFFFF
					&& prev_sample.y == 0xFFFF) {
				// this is the first sample; move along
				prev_sample = curr_sample;
				break;
			}
			
			// check for magic pen up coordinates
			if (curr_sample.x == 1019 && curr_sample.y == 0) {
				// it's a dummy sample signifying pen up
				prev_sample.pressed = false;
			}
			
			err = add_sample(prev_sample);
			if (err != B_OK) {
				// TT: need to do something to handle this case
				ANNOY((BYLINE "Trouble adding a sample to the queue: 0x%08lx\n",
					err));
			}
			//ANNOY((BYLINE "sample_queue_head = %d\n", sample_queue_head));
			//ANNOY((BYLINE "sample_queue_tail = %d\n", sample_queue_tail));
			
			if (!(curr_sample.x == 1019 && curr_sample.y == 0)) {
				prev_sample = curr_sample;
			}
			
			release_sem_etc(sample_ready_sem, 1, B_DO_NOT_RESCHEDULE);
			break;
	}
	
	int_count = (int_count + 1) % 5;
	
err:
	return B_HANDLED_INTERRUPT;
}

/* driver hooks */

status_t
init_driver(void)
{
	status_t err;

	dprintf(BYLINE "init_driver()\n");
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if (err != B_OK)
		goto err1;

	err = sample_ready_sem = create_sem(0, BYLINE "sample_ready_sem");
	if (err < B_OK)
		goto err2;

	sample_queue_head = 0;
	sample_queue_tail = 0;
	dropped_samples = 0;
	pressed = false;
	
	err = install_io_interrupt_handler(TOUCH_IRQ, touch_interrupt_handler,
		NULL, 0);
	if (err != B_OK)
		goto err3;
		
	return B_OK;
	
err3:
	delete_sem(sample_ready_sem);
err2:
	put_module(B_ISA_MODULE_NAME);
err1:
	dprintf(BYLINE "init_driver() failed with 0x%08lx\n", err);
	return err;
}

void
uninit_driver(void)
{
	dprintf(BYLINE "uninit_driver()\n");
	dprintf(BYLINE "uninit_driver(): %d samples dropped\n", dropped_samples);
	
	remove_io_interrupt_handler(TOUCH_IRQ, touch_interrupt_handler, NULL);
	delete_sem(sample_ready_sem);
	put_module(B_ISA_MODULE_NAME);
}

status_t
touch_open(const char *name, uint32 flags, void **cookie)
{
	return B_OK;
}

status_t
touch_close(void *cookie)
{
	return B_OK;
}

status_t
touch_free(void *cookie)
{
	return B_OK;
}

status_t
touch_control(void *cookie, uint32 op, void *data, size_t len)
{
	switch(op) {
		default:
			dprintf(BYLINE "touch_control() %lx\n", op);
			return B_ERROR;
	}
}

status_t
touch_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	status_t err;

	if (*numBytes < sizeof (sample_t)) {
		*numBytes = 0;
		return B_ERROR;
	}
	
	err = acquire_sem_etc(sample_ready_sem, 1,
		B_CAN_INTERRUPT, 0);
	if (err != B_OK) {
		// trouble acquiring sem
		*numBytes = 0;
		return err;
	} else {
		*((sample_t *)data) = sample_queue[sample_queue_head];
		sample_queue_head = (sample_queue_head + 1) % SAMPLE_QUEUE_SIZE;
	}
	
	*numBytes = sizeof (sample_t);
	
	return B_OK;
}

status_t
touch_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}

static device_hooks touch_device_hooks = {
	&touch_open,
	&touch_close,
	&touch_free,
	&touch_control,
	&touch_read,
	&touch_write,
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	NULL, /* writev */
};

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	"input/touchscreen/" DRIVER_NAME "/0",
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
	dprintf(BYLINE "find_device(%s)\n", name);
	return &touch_device_hooks;
}
