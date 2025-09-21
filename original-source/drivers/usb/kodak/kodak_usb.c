#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <USB.h>


int32 api_version = B_CUR_DRIVER_API_VERSION;


#define ID "\033[32mkodak:\033[30m "

#define BASENAME "camera/usb/kodak"

#define TMP_BUF_SIZE (4*1024)

typedef struct device
{
	struct device *next;
	
	int num;
	int active;
	int open;
	usb_device dev;
	usb_pipe out;
	usb_pipe in;
	sem_id lock;
	sem_id done;
	area_id hack_id;
	void	 *hack;
	size_t actual_len;
	uint32 dev_status;
	bigtime_t	 timeout;
} device;

static usb_module_info *usb;
static sem_id dev_list_lock = -1;
static device *device_list;
static int device_count = 0;
static char **device_names = NULL;

usb_support_descriptor supported_devices[] = {
	{ 0, 0, 0, 0x040a, 0 }
};

static uint32 device_numbers = 0;

static int get_number(void)
{
	uint32 n;
	int i;
	for(i=0;i<31;i++){
		n = 1 << i;
		if((device_numbers & n) == 0){
			device_numbers |= n;
			return i;
		}
	}
	return -1;
}

static void put_number(int i)
{
	device_numbers &= (~(1 << i));
}

static device* 
add_device(usb_device dev, const usb_configuration_info *conf, int ifc)
{
	unsigned int i,j;
	char name[32];
	uchar *x;
	size_t act;	
	device *p;

	if((i = get_number()) < 0) return NULL;
	
	if((p = (device *)malloc(sizeof(device)))) {
		p->num = i;
		p->dev = dev;
		sprintf(name,"usb_camera:%d:lock",i);
		p->lock = create_sem(1,name);
		sprintf(name,"usb_camera:%d:done",i);
		p->done = create_sem(0,name);
		p->active = 1;
		p->open = 0;
		p->hack_id = create_area("usb_camera_hack", (void **)&(p->hack),
								 B_ANY_KERNEL_ADDRESS, TMP_BUF_SIZE, B_CONTIGUOUS, 
								 B_READ_AREA | B_WRITE_AREA);
				
		usb->set_configuration(dev,conf);

		/* find the bulk out endpoint */
		for (j=0 ; j<conf->interface[ifc].active->endpoint_count ; j++) {
			if(!(conf->interface[ifc].active->endpoint[j].descr->endpoint_address & 0x80)) {
				p->out = conf->interface[ifc].active->endpoint[j].handle;
				break;
			}
		}

		/* find the bulk in endpoint */
		for (j=0 ; j<conf->interface[ifc].active->endpoint_count ; j++) {
			if((conf->interface[ifc].active->endpoint[j].descr->endpoint_address & 0x80)) {
				p->in = conf->interface[ifc].active->endpoint[j].handle;
				break;
			}
		}
		
		dprintf(ID "added device /dev/camera/usb/kodak/%d\n",p->num);
	} 
	return p;
}

static void remove_device(device *p)
{
	dprintf(ID "remove %x\n");
	put_number(p->num);
	
	delete_area(p->hack_id);
	delete_sem(p->lock);
	delete_sem(p->done);
	free((void *)p);
}

static status_t device_added(const usb_device dev, void **cookie)
{
	device *d;
	unsigned int i;
	const usb_configuration_info *conf;
	
	if((conf = usb->get_nth_configuration(dev,0))){
		d = add_device(dev, conf, 0);
		if(d){
			acquire_sem(dev_list_lock);
			d->next = device_list;
			device_list = d;
			device_count++;
			release_sem(dev_list_lock);
			*cookie = d;
			return B_OK;
		}
	} 
	
	return B_ERROR;
}

static status_t device_removed(void *cookie)
{
	device *d,*last;
	
	dprintf(ID "removing device /dev/camera/usb/kodak/%d\n",d->num);

	acquire_sem(dev_list_lock);
	for(last = NULL, d = device_list; d; d = d->next){
		if(d == (device*) cookie){
			if(last){
				last->next = d->next;
			} else {
				device_list = d->next;
			}
			device_count--;
			d->active = 0;
			if(d->open == 0) {
				remove_device(d);
			} else {
				dprintf(ID "still open - defering...\n");
			}
			break;
		}
		last = d;
	}
	release_sem(dev_list_lock);
	return B_OK;
}

static status_t
device_open(const char *dname, uint32 flags, void **cookie)
{	
	status_t res = B_ERROR;
	device *d;
	int i;

	i = atoi(dname + strlen(BASENAME) + 1);

	acquire_sem(dev_list_lock);
	for(d = device_list; d; d = d->next){
		if((d->num == i) && d->active && (d->open == 0)){
			d->open++;
			*cookie = d;
			res = B_OK;
			break;
		}
	}
	release_sem(dev_list_lock);
	return res;
}


static status_t
device_free(void *cookie)
{
	/* cookie is shared. never free it here */
	return B_OK;
}

static status_t
device_close(void *cookie)
{
	device *d = (device*) cookie;
	
	acquire_sem(dev_list_lock);
	d->open--;
	if((d->open == 0) && (d->active == 0)){
		dprintf(ID "removing on close\n");
		remove_device(d);
	}
	release_sem(dev_list_lock);
	return B_OK;
}


static void 
cb_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	device *d = (device*) cookie;
	d->actual_len = actual_len;
	d->dev_status = status;
	release_sem(d->done);
}

static status_t
device_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	device *d = (device*) cookie;
	int sz = *count;
	status_t s = B_OK;
	
	*count = 0;
	if(d->active == 0) return ENODEV;
	
	acquire_sem(d->lock);

//	dprintf(ID "read %d\n",sz);	
	while(sz) {
		int xfer = sz > TMP_BUF_SIZE ? TMP_BUF_SIZE : sz;
		d->actual_len = 0;
		d->dev_status = 0;
		
		if ((s = usb->queue_bulk(d->in, d->hack, xfer, cb_notify, (void *)d)) != B_OK) {
			s = EIO;
			break;
		}

		if ((s = acquire_sem_etc(d->done, 1, B_CAN_INTERRUPT, 0)) != B_OK) {
			//XXX
			break;
		}

		if (d->dev_status) {
			s = EIO;
			break;
		}
			
		if (d->actual_len == 0) break; /* Nothing read */
		
		/* Move the data in the user's buffer */
		memcpy(buf, d->hack, d->actual_len);
		(uint8 *)buf += d->actual_len;
		*count += d->actual_len;
		sz -= d->actual_len;		
	}

	release_sem(d->lock);
	return s;
}

static status_t
device_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	device *d = (device*) cookie;
	int sz = *count;
	status_t s = B_OK;
	
	*count = 0;
	if(d->active == 0) return ENODEV;
	
	acquire_sem(d->lock);
	
//	dprintf(ID "write %d\n",sz);	
	while(sz) {
		int xfer = sz > TMP_BUF_SIZE ? TMP_BUF_SIZE : sz;
		d->actual_len = 0;
		d->dev_status = 0;
		
		/* Move the data in the user's buffer */
		memcpy(d->hack, buf, xfer);
		if ((s = usb->queue_bulk(d->out, d->hack, xfer, cb_notify, (void *)d)) != B_OK) {
			s = EIO;
			break;
		}

		if ((s = acquire_sem_etc(d->done, 1, B_CAN_INTERRUPT, 0)) != B_OK) {
			//XXX
			break;
		}

		if (d->dev_status) {
			s = EIO;
			break;
		}
			
		if (d->actual_len == 0) break; /* Nothing read */
		
		(uint8 *)buf += d->actual_len;
		*count += d->actual_len;
		sz -= d->actual_len;		
	}

	release_sem(d->lock);
	return s;
}

static status_t
device_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}


static device_hooks _device_hooks = {
	&device_open, 			
	&device_close, 			
	&device_free,			
	&device_control, 		
	&device_read,			
	&device_write			 
};


static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};


_EXPORT status_t 
init_hardware(void)
{
	return B_OK;
}


_EXPORT status_t
init_driver(void)
{
	int i;
	load_driver_symbols("kodak_usb");
	
	if(get_module(B_USB_MODULE_NAME,(module_info**) &usb) != B_OK){
		return B_ERROR;
	} 
	
	dev_list_lock = create_sem(1,"usb_camera:devlist_lock");
	
	usb->register_driver("kodak_usb",supported_devices,1, NULL);
	usb->install_notify("kodak_usb",&notify_hooks);
	return B_OK;
}

_EXPORT void
uninit_driver(void)
{
	int i;

	usb->uninstall_notify("kodak_usb");
	if(device_count){
		dprintf(ID "aiiie, count = %d\n",device_count);
	}
	delete_sem(dev_list_lock);
	put_module(B_USB_MODULE_NAME);
	if(device_names){
		for(i=0;device_names[i];i++) free(device_names[i]);
		free(device_names);
	}
}

_EXPORT const char**
publish_devices()
{
	device *d;
	int i;
	
	acquire_sem(dev_list_lock);
	if(device_names){
		for(i=0;device_names[i];i++) free(device_names[i]);
		free(device_names);
	}	
	device_names = (char**) malloc(sizeof(char*) * (device_count + 1));
	
	for(i=0, d = device_list; d; d = d->next){
		if(d->active){
			device_names[i] = (char*) malloc(strlen(BASENAME)+8);
			sprintf(device_names[i],"%s/%d",BASENAME,i);
			i++;
		}
	}
	release_sem(dev_list_lock);
	device_names[i] = NULL;
	
	return (const char**) device_names;
}

_EXPORT device_hooks*
find_device(const char* name)
{
	return &_device_hooks;
}


