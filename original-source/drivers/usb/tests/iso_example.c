#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <byteorder.h>

#include <USB.h>


#define	VENDOR_ID	0x1234
#define DEVICE_ID	0	

static status_t tests_open(const char *name, uint32 flags, void **cookie);
static status_t tests_free (void *cookie);
static status_t tests_close(void *cookie);
static status_t tests_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t tests_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t tests_write(void *cookie, off_t pos, const void *buf, size_t *count);

void cb_notify_in(void *cookie, uint32 status, void *data, uint32 actual_len);
void cb_notify_out(void *cookie, uint32 status, void *data, uint32 actual_len);



#define ddprintf dprintf
//extern int serial_dprintf_enabled;  

#define PDEV_MAX 32
#define TS	4	/* 4 ms buffers */
#define NB	2

/* assumes ~44 kHz 16bit stereo input & output */
#define SAMPLE_SIZE	4  
#define QS_OUT	(44*SAMPLE_SIZE*TS)
#define QS_IN	((44+2)*SAMPLE_SIZE*TS)



int		qs_in = QS_IN;
int		qs_out = QS_OUT;


struct testsdev;
struct iso_channel;	
struct iso_packet;

typedef struct iso_packet
{
	struct iso_packet*		next;
	struct iso_channel*		channel;	
	void*					buffer;
	uint32					status;
	size_t					buffer_size;
	rlea*					rle_array;
} iso_packet;


typedef struct iso_channel
{
	usb_pipe*	ep;
	
	iso_packet*	next_to_queue;
	iso_packet*	current_rw;
	size_t		remain;
	char*		buf_ptr;
	sem_id		num_avalable_packets;
	area_id		buffer_area;
	iso_packet	iso_packets[NB];
} iso_channel;

static void queue_packet_out(iso_channel* ch);
static void queue_packet_in(iso_channel* ch);



typedef struct testsdev {
	int active;
	int open;
	usb_device *dev;
	
	iso_channel	in_channel;
	iso_channel	out_channel;
} testsdev;

static const char *tests_name[PDEV_MAX+1];

static testsdev *pdev[PDEV_MAX];

#define ID "usb_tests: "

static device_hooks tests_devices = {
	tests_open, 			
	tests_close, 			
	tests_free,			
	tests_control, 		
	tests_read,			
	tests_write			 
};

static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;
static int dev_count = 0;
sem_id dev_table_lock = -1;




#if SIN_OUTPUT

double my_sin(double x)	/* 0 <= x <= 1 period is 1.0, quadratic approximation  */
{
	if(x<0.5)
		return 1.0 - 16.0*(x-0.25)*(x-0.25);
	else
		return 16.0*(x-0.75)*(x-0.75) - 1.0;
}



static void init_sin_buf_2(void* buf, size_t num_samples)
{
	int		i;
	int16*	ptr = buf;
	double	x;
	int16	s;
	
	for(i=0; i<num_samples; i++)
	{
		x = i/((double)num_samples);
		s = (int16)(my_sin(x)*((double)0x4000) + 0.49999999);
		ptr[2*i] = ptr[2*i+1] = s; 
	}
}

#define F	57.1234
#define	SF	44100.0


static void
put_sin_buf_2(void* buf, size_t num_samples)
{
	int		i;
	int16*	ptr = buf;
	static double	x = 0.0;
	int16	s;
	
	for(i=0; i<num_samples; i++)
	{
		x += F/SF;
		if(x>1.0)
			 x -= 1.0;
			 
		s = (int16)(my_sin(x)*((double)0x4000) + 0.49999999);
		ptr[2*i] = ptr[2*i+1] = s; 
	}
}

#endif




static void 
dump_rle_array(const rlea* rle_array)
{
    int i;

	dprintf("rle_array = %p, num_valid = %d\n", rle_array, rle_array->num_valid);
    for (i=0; i<rle_array->num_valid; i++)
	{
        dprintf ("%04x %04x\n", rle_array->rles[i].rle_status, rle_array->rles[i].sample_count);
    }
}


static size_t
get_data_size(const rlea* rle_array)
{
    int		i;
	size_t	s = 0;

    for(i=0; i<rle_array->num_valid; i++)
		s += rle_array->rles[i].sample_count;
	
	return s*SAMPLE_SIZE;
}



void
init_iso_channel(iso_channel* ch, usb_pipe* ep, size_t buf_size, bool is_in)
{
	int		pn;
	void*	big_buffer;

	ch->ep = ep;

	ch->buffer_area = create_area("usb_tests_buffer", (void **)&big_buffer, 
		B_ANY_KERNEL_ADDRESS, ((buf_size*NB) + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1), B_CONTIGUOUS, 
		B_READ_AREA | B_WRITE_AREA);
	ddprintf("usb_tests: buffer_area %d @ 0x%08x\n", ch->buffer_area, big_buffer);


	for(pn=0; pn<NB; pn++)
	{
		ch->iso_packets[pn].channel = ch;
		ch->iso_packets[pn].buffer = (char*)big_buffer + buf_size*pn;

#if SIN_OUTPUT
		if(!is_in)
			init_sin_buf_2(ch->iso_packets[pn].buffer, buf_size/SAMPLE_SIZE);
#endif

		if(is_in)
		{
			ch->iso_packets[pn].rle_array = malloc(sizeof(rlea) + (4-1)*sizeof(rle));
			ch->iso_packets[pn].rle_array->length = 4;
		}
		else	
			ch->iso_packets[pn].rle_array = NULL;
			
		ch->iso_packets[pn].next = &ch->iso_packets[(pn+1)%NB];  
	}

	ch->current_rw = ch->next_to_queue = &ch->iso_packets[0];
	if(!is_in)
		ch->current_rw = &ch->iso_packets[NB-1];
	ch->num_avalable_packets = create_sem(0, "iso_channel");
	
	ch->remain = 0;
	ch->buf_ptr = 0;
}


void
uninit_iso_channel(iso_channel* ch)
{
	int	pn;
	
	for(pn=0; pn<NB; pn++)
		free(ch->iso_packets[pn].rle_array);
	
	delete_sem(ch->num_avalable_packets);
	delete_area(ch->buffer_area);
}


void start_iso_channel_in(iso_channel* channel)
{
	int pn;
	
	for(pn=0; pn<NB; pn++)
	{
		queue_packet_in(channel);
	}
}

void start_iso_channel_out(iso_channel* channel)
{
	int pn;
	
	for(pn=0; pn<NB; pn++)
	{
		queue_packet_out(channel);
	}
}


void stop_iso_channel(iso_channel* channel)
{
	usb->cancel_queued_transfers(channel->ep);
}





static void
queue_packet_out(iso_channel* ch)
{
	iso_packet*	packet = ch->next_to_queue;
	status_t	s;	

	//dprintf("queue_isochronous_out(%d, %p, %p)\n", qs_out, packet->buffer, packet->rle_array);
	if(s = usb->queue_isochronous(ch->ep, packet->buffer, qs_out,
		NULL, TS, cb_notify_out, packet))
	{
		dprintf(ID "packet out %p status %d\n", packet, s);
	}
	
	ch->next_to_queue = packet->next;
} 


static void
queue_packet_in(iso_channel* ch)
{
	iso_packet*	packet = ch->next_to_queue;
	status_t	s;	

	//dprintf("queue_isochronous_in(%d, %p, %p)\n", qs_in, packet->buffer, packet->rle_array);
	if(s = usb->queue_isochronous(ch->ep, packet->buffer, qs_in,
		packet->rle_array, TS, cb_notify_in, packet))
	{
		dprintf(ID "packet in %p status %d\n", packet, s);
	}	
	ch->next_to_queue = packet->next;
} 



static void add_device(usb_device *dev, usb_configuration_info *conf)
{
	int i;
	char name[32];
	uchar *x;
	size_t act;
	status_t st;
	usb_pipe	*in, *out;
	
	testsdev *p;
	
	ddprintf("add_device(%p, %p)\n", dev, conf);
	
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(!pdev[i]){
			if(p = (testsdev *) malloc(sizeof(testsdev))){
				p->dev = dev;
				p->active = 1;
				p->open = 0;

				// get a different interface (alternate), alt[0] usually doesn't have any bandwith
				st = usb->set_alt_interface(dev, &conf->interface[0].alt[1]);
				if(st != B_OK)
					dprintf("set_alt_interface(0) returns %x\n", st);

				st = usb->set_alt_interface(dev, &conf->interface[1].alt[1]);
				if(st != B_OK)
					dprintf("set_alt_interface(1) returns %x\n", st);

	
				st = usb->set_configuration(dev,conf);
				if(st != B_OK)
					dprintf("set_configuration() returns %x\n", st);
	
				out = conf->interface[0].active->endpoint[0].handle;
				in = conf->interface[1].active->endpoint[0].handle;
				dprintf(ID "added %p / %p /%p (/dev/misc/usb/%d)\n",
						p->dev, out, in, i);
				pdev[i] = p;
				
				st = usb->set_pipe_policy(in, NB, TS, SAMPLE_SIZE);
				if(st != B_OK)
					dprintf("set_pipe_policy(in) returns %x\n", st);
		
				st = usb->set_pipe_policy(out, NB, TS, SAMPLE_SIZE);
				
				if(st != B_OK)
					dprintf("set_pipe_policy(out) returns %x\n", st);

				init_iso_channel(&p->in_channel, in, qs_in, TRUE);
				init_iso_channel(&p->out_channel, out, qs_out, FALSE);

				start_iso_channel_in(&p->in_channel);
				start_iso_channel_out(&p->out_channel);
			}
			break;
		}
	}
	release_sem(dev_table_lock);
}


static void remove_device(testsdev *p)
{
	int i;
	
	ddprintf("remove_device(%p)\n", p);
	
	for(i=0;i<PDEV_MAX;i++) if(pdev[i] == p) break;
	if(i == PDEV_MAX)
	{
		dprintf(ID "removed nonexistant device!?\n");
		return;
	}
	ddprintf(ID "removed 0x%08x (/dev/misc/usb/%d)\n",p,i);
	snooze(1000*1000); // HACK wait some time to finish callbacks

	uninit_iso_channel(&p->out_channel);
	uninit_iso_channel(&p->in_channel);

	free(p);
	pdev[i] = 0;
	ddprintf("Done remove_device(%p)\n", p);
}


static status_t device_added(const usb_device *dev, void **cookie)
{
	int i;
	usb_configuration_info *conf;
	const usb_device_descriptor*	dev_desc;

	ddprintf("device_added(%p, %p)\n", dev, cookie);

	dev_desc = usb->get_device_descriptor(dev);
	if(dev_desc->vendor_id == VENDOR_ID)
	{
		if(conf = usb->get_nth_configuration(dev,0))
		{
			add_device(dev, conf);
			*cookie = dev;
			return B_OK;
		}
		else
		{
			dprintf(ID "device has no config 0\n");
		}
	}
	return B_ERROR;
}


static status_t device_removed(void *cookie)
{
	usb_device *dev = (usb_device*) cookie;
	int i;

	ddprintf("device_removed(%p)\n", cookie);

	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(pdev[i] && (pdev[i]->dev == dev)){
			pdev[i]->active = 0;
			if(pdev[i]->open == 0) {
				remove_device(pdev[i]);
			} else {
				ddprintf(ID "device /dev/tests/usb/%d still open -- marked for removal\n",i);
			}
		}
	}
	release_sem(dev_table_lock);
	ddprintf("Done device_removed(%p)\n", cookie);
	return B_OK;
}


static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};


_EXPORT
status_t init_hardware(void)
{
	ddprintf(ID "init_hardware()\n");
	return B_OK;
}                   


usb_support_descriptor supported_devices[] =
{
	{ 0, 0, 0, VENDOR_ID, DEVICE_ID}
};

_EXPORT
status_t
init_driver(void)
{
	int i;
	ddprintf(ID "%s %s, init_driver()\n", __DATE__, __TIME__);
	
	//if(!serial_dprintf_enabled)
		//return B_ERROR;
	
	if(load_driver_symbols("usb_tests") == B_OK) {
		ddprintf(ID "loaded symbols\n");
	} else {
		ddprintf(ID "no symbols for you!\n");
	}
	
	if(get_module(usb_name,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",usb_name);
		return B_ERROR;
	} 
	
	dev_table_lock = create_sem(1,"usb_tests:devtab_lock");
	for(i=0;i<PDEV_MAX;i++) pdev[i] = NULL;
	tests_name[0] = NULL;
	
	usb->register_driver("usb_tests",supported_devices,1);
	usb->install_notify("usb_tests",&notify_hooks);
	ddprintf(ID "init_driver() done\n");
	return B_OK;
	
err:
	put_module(usb_name);
	return B_ERROR;
}

_EXPORT
void
uninit_driver(void)
{
	int i;
	
	ddprintf(ID "uninit_driver()\n");

	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(pdev[i]){
			stop_iso_channel(&pdev[i]->in_channel);
			stop_iso_channel(&pdev[i]->out_channel);
		}
	}
	release_sem(dev_table_lock);	
	usb->uninstall_notify("usb_tests");


	delete_sem(dev_table_lock);

	put_module(usb_name);

	for(i=0;tests_name[i];i++)
		free((char*) tests_name[i]);
	
	ddprintf(ID "uninit_driver() done\n");
}



_EXPORT
const char**
publish_devices()
{
	int i,j;
	ddprintf(ID "publish_devices()\n");

	for(i=0;tests_name[i];i++) free((char *) tests_name[i]);
	
	acquire_sem(dev_table_lock);
	for(i=0,j=0;i<PDEV_MAX;i++){
		if(pdev[i] && pdev[i]->active){
			if(tests_name[j] = (char *) malloc(16)){
				sprintf((char *)tests_name[j],"misc/usb/%d",i);
				ddprintf(ID "publishing %s\n",tests_name[j]);
				j++;
			}
		}
	}
	release_sem(dev_table_lock);
	tests_name[j] = NULL;
	return tests_name;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
	return &tests_devices;
}



static status_t
tests_open(const char *dname, uint32 flags, void **cookie)
{	
	int i;
	
	i = atoi(dname + strlen(dname)-1); 
	
	ddprintf(ID "opening \"%s\"\n",dname);
	acquire_sem(dev_table_lock);
	if(pdev[i])
	{
		if(pdev[i]->active)
		{
			pdev[i]->open++;
			*cookie = pdev[i];	
			release_sem(dev_table_lock);
			return B_OK;
		}
		else
		{
			dprintf(ID "device is going offline. cannot open\n");
		}
	}
	release_sem(dev_table_lock);
	return B_ERROR;
}

 
static status_t
tests_close (void *cookie)
{
	return B_OK;
}


static status_t
tests_free(void *cookie)
{
	testsdev *p = (testsdev *) cookie;

	acquire_sem(dev_table_lock);
	p->open--;
	if((p->open == 0) && (p->active == 0)) remove_device(p);
	release_sem(dev_table_lock);
	return B_OK;
}


void cb_notify_in(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	iso_packet* const packet = (iso_packet*) cookie;
	iso_channel*	const channel = packet->channel;
	size_t				data_size;
	int32				sc;

	//dprintf("INcb\n");

	if(status != 0)
		//dprintf(ID "device %p packet %p status %d, actual %d\n", device, packet, status, actual_len);
		dprintf("s%d\n", status);

	packet->status = status;
	if(packet->rle_array->num_valid != 1)
	{
		//dprintf("%d", packet->rle_array->num_valid);
		dump_rle_array(packet->rle_array);
	}
	else
	{
		data_size = get_data_size(packet->rle_array);
#if SYNC_INPUT_OUTPUT	
		qs_out = data_size;
#endif
	}
	
	if(!(status & 0x10))
	{
		queue_packet_in(channel);

		get_sem_count(channel->num_avalable_packets, &sc);	// hack for startup time
		if(sc < NB-1)
			release_sem(channel->num_avalable_packets);
	}
}


void cb_notify_out(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	iso_packet* const packet = (iso_packet*) cookie;
	iso_channel*	const channel = packet->channel;
	int			num_queued_packets;
	static bigtime_t	t, t0;
	static 				c;
	int					i;
	int32				sc;

	//dprintf("OUTcb\n");

	if(status != 0)
		//dprintf(ID "device %p packet %p status %d, actual %d\n", device, packet, status, actual_len);
		dprintf("s%d\n", status);

	packet->status = status;

	packet->buffer_size = qs_out;

	if(!(status & 0x10))
	{

		queue_packet_out(channel);
		channel->current_rw = packet;

		get_sem_count(channel->num_avalable_packets, &sc);	// hack for startup time
		if(sc < 2)
			release_sem(channel->num_avalable_packets);
	}
}



/* ATTENTION: this implementation of read() hook assumes that read buffer size is always bigger
than USB IN buffer size (qs_in). cat and dd work with this implementation. */
static status_t
tests_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	testsdev*		device = (testsdev*) cookie; 
	iso_channel*	channel = &device->in_channel;
	size_t			data_size;
	status_t		st;

	//dprintf(ID "read %d\n", *count);

	acquire_sem_etc(channel->num_avalable_packets, 1, B_RELATIVE_TIMEOUT, 4*1000*1000);

	if(st == B_TIMED_OUT)
	{
		dprintf("st = B_TIMED_OUT\n");
		*count = 0;
		return B_ERROR;
	}

	data_size = get_data_size(channel->current_rw->rle_array);
	data_size = min(data_size, *count);

	memcpy(buf, channel->current_rw->buffer, data_size);
	channel->current_rw = channel->current_rw->next;

	*count = data_size;
	
	return B_OK;
}


static status_t
tests_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	testsdev*		device = (testsdev*) cookie; 
	iso_channel*	channel = &device->out_channel;
	status_t		st;

	//dprintf(ID "write %d at %Ld from %p\n", *count, pos, buf);
	//dprintf("w%d\n", *count, pos, buf);


#if SIN_OUTPUT
	st = acquire_sem_etc(channel->num_avalable_packets, 100000000, B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);

	*count = 0;
	return B_ERROR;
#endif

	if(channel->remain == 0)
	{
		st = acquire_sem_etc(channel->num_avalable_packets, 1, B_RELATIVE_TIMEOUT, 4*1000*1000);
		if(st == B_TIMED_OUT)
		{
			dprintf("st = B_TIMED_OUT\n");
			*count = 0;
			return B_ERROR;
		}
	
		if(channel->current_rw == channel->next_to_queue)
			dprintf("e");
		channel->remain = channel->current_rw->buffer_size;
		channel->buf_ptr = channel->current_rw->buffer;
	}
	
	if(channel->remain >= *count)
	{
		memcpy(channel->buf_ptr, buf, *count);
		channel->remain -= *count;
		channel->buf_ptr += *count;
	}
	else
	{
		memcpy(channel->buf_ptr, buf, channel->remain);
		*count = channel->remain;
		channel->remain = 0;
		channel->buf_ptr = NULL;
	}
	
	return B_OK;
}


static status_t
tests_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}



