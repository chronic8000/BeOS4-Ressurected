#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <byteorder.h>

#include <USB.h>

static status_t tests_open(const char *name, uint32 flags, void **cookie);
static status_t tests_free (void *cookie);
static status_t tests_close(void *cookie);
static status_t tests_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t tests_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t tests_write(void *cookie, off_t pos, const void *buf, size_t *count);

#define ddprintf dprintf 

#define PDEV_MAX 32
#define QS	(8*4096) 

typedef struct {
	int active;
	int open;
	usb_device *dev;
	//usb_endpoint *out;
	usb_endpoint *in;
	sem_id lock, done;
	area_id hack_id;
	void *hack;
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

static void add_device(usb_device *dev, usb_configuration_info *conf, int ifc)
{
	int i;
	char name[32];
	uchar *x;
	size_t act;
	
	testsdev *p;
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(!pdev[i]){
			if(p = (testsdev *) malloc(sizeof(testsdev))){
				usb->get_device(dev);
				p->dev = dev;
				sprintf(name,"usb_tests:%d:lock",i);
				p->lock = create_sem(1,name);
				sprintf(name,"usb_tests:%d:done",i);
				p->done = create_sem(0,name);
				p->active = 1;
				p->open = 0;
				p->hack_id = create_area("usb_tests_hack", (void **)&(p->hack), 
										 B_ANY_KERNEL_ADDRESS, (QS + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1), B_CONTIGUOUS, 
										 B_READ_AREA | B_WRITE_AREA);
				ddprintf("usb_tests:%d: %d @ 0x%08x\n",i,p->hack_id,p->hack);
				
				usb->set_configuration(dev,conf);
				p->in = conf->interface[ifc].endpoint[0].handle;
				dprintf(ID "added 0x%08x / 0x%08x (/dev/misc/usb/%d)\n",
						p->dev,p->in,i);
				pdev[i] = p;
			}
			break;
		}
	} 
	release_sem(dev_table_lock);
}

static void remove_device(testsdev *p)
{
	int i;
	for(i=0;i<PDEV_MAX;i++) if(pdev[i] == p) break;
	if(i == PDEV_MAX) dprintf(ID "removed nonexistant device!?\n");
	ddprintf(ID "removed 0x%08x (/dev/misc/usb/%d)\n",p,i);
	usb->put_device(p->dev);
	delete_area(p->hack_id);
	delete_sem(p->lock);
	delete_sem(p->done);
	free(p);
	pdev[i] = 0;
}

static void notify(usb_device *dev, uint32 status)
{
	int i;
	usb_configuration_info *conf;
	const usb_device_descriptor*	dev_desc;
	
	ddprintf(ID "notify 0x%08x / %d\n",dev,status);
	
	switch(status)
	{
	case USB_DEVICE_ADDED:
		dev_desc = usb->get_device_descriptor(dev);
		if((dev_desc->vendor_id == 0x6666) && (dev_desc->product_id == 0x6969))
		{
			if(conf = usb->get_nth_configuration(dev,0))
			{
				add_device(dev, conf, 0);
				return;
			}
			else
			{
				dprintf(ID "device has no config 0\n");
			}
		}
		break;

	case USB_DEVICE_REMOVED:
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
		break;
	} 
} 

_EXPORT
status_t init_hardware(void)
{
	ddprintf(ID "init_hardware()\n");
	return B_OK;
}                   


usb_support_descriptor supported_devices[] =
{
	{ USB_SUPPORT_DESCRIPTOR, 0, 0, 0, 0}//0x6666, 0x6969 }
};

_EXPORT
status_t
init_driver(void)
{
	int i;
	ddprintf(ID "%s %s, init_driver()\n", __DATE__, __TIME__);
		
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
	usb->install_notify("usb_tests",notify);
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
	usb->uninstall_notify("usb_tests");
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(pdev[i]){
			remove_device(pdev[i]);
		}
	}
	release_sem(dev_table_lock);	
	delete_sem(dev_table_lock);
	put_module(usb_name);
	for(i=0;tests_name[i];i++) free((char*) tests_name[i]);
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

static void
soft_reset(testsdev *d)
{
	size_t act;
	if(usb->send_request(d->dev,0x23,2,0,0,0,NULL,0,&act)){
		dprintf(ID "SOFT_RESET fails\n");
	} else {
		dprintf(ID "SOFT_RESET is happy enough\n");
	}
	
}

static status_t
tests_open(const char *dname, uint32 flags, void **cookie)
{	
	int i;
	i = atoi(dname + 12); /* strlen("tests/usb/") == 12 */
	
	ddprintf(ID "opening \"%s\"\n",dname);
	acquire_sem(dev_table_lock);
	if(pdev[i]){
		if(pdev[i]->active){
			pdev[i]->open++;
			*cookie = pdev[i];	
			release_sem(dev_table_lock);
			soft_reset(pdev[i]);
			return B_OK;
		} else {
			dprintf(ID "device is going offline. cannot open\n");
		}
	}
	release_sem(dev_table_lock);
	return B_ERROR;
}
 
static status_t
tests_free (void *cookie)
{
	return B_OK;
}

static status_t
tests_close(void *cookie)
{
	testsdev *p = (testsdev *) cookie;
	acquire_sem(dev_table_lock);
	p->open--;
	if((p->open == 0) && (p->active == 0)) remove_device(p);
	release_sem(dev_table_lock);
	return B_OK;
}


void cb_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	testsdev *p = (testsdev *) cookie;
	release_sem(p->done);
	if(status != 0)
	dprintf(ID "dev 0x%08x status %d, actual %d\n",cookie,status,actual_len);
}


static status_t
tests_read(void *cookie, off_t pos, void *buf, size_t *count)
{
#if 1
	static bigtime_t	t_sum = 0;
	static	unsigned	call_count = 0, read_sum = 0;
	bigtime_t			t0;

	testsdev *p = (testsdev *) cookie;
	
	size_t remain_count = *count;
	size_t qs;
	status_t s;
	
	//dprintf(ID "dev 0x%08x read %d\n", cookie, remain_count);
	
	
	acquire_sem(p->lock);
	t0 = system_time();
	*count = 0;
	while(remain_count > 0)
	{
		qs = (remain_count>QS) ? QS : remain_count;

		if(s = usb->queue_bulk(p->in, p->hack, qs, cb_notify, p))
		{
			*count = 0;
			release_sem(p->lock);
			dprintf(ID "dev 0x%08x status %d\n", cookie, s);
			return B_ERROR;
		}
		acquire_sem(p->done);	/* wait untill callback */

		memcpy(buf, p->hack, qs);

		remain_count -= qs;
		buf += qs;
		(*count) += qs; 
	}
	t_sum += system_time()-t0;
	read_sum += *count; 
	call_count++;
	
	{
		static uint16	magic;
		static bool		first_read = TRUE;
	
		int	i;
		uint8* ptr = buf;
		
		ptr -= *count;

		if(first_read)
		{
			first_read = FALSE;
			magic = B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr));
			--magic;
		}
		
		for(; ptr<(uint8*)buf; ptr += 64)
		{
			//dprintf("%04x ", B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
			if(++magic != B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)) )
			{
				dprintf("Error: buf = 0x%08x, ptr = 0x%08x, expected %04x, got %04x\n", ((uint8*)buf) - (*count), ptr, magic, B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
				kernel_debugger("\n");
			}
		}
	}

	release_sem(p->lock);
	if(read_sum > 200*1024)
	{
		dprintf("Read rate %Ld kB/s\n", (1000*read_sum)/t_sum);
		t_sum = 0;
		read_sum = 0;
		call_count = 0;
	}
	return B_OK;

#else
	*count = 0;
	return B_OK;
#endif
}


static status_t
tests_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	testsdev *p = (testsdev *) cookie;
	int sz = *count;
	status_t s;
#if 0	
	dprintf(ID "dev 0x%08x write %d\n",cookie,sz);
	
	acquire_sem(p->lock);
	while(sz > 0){
		memcpy(p->hack,buf,sz>4096?4096:sz);
		if(s = usb->queue_bulk(p->out, p->hack, sz>4096?4096:sz, cb_notify, p)){
			*count = 0;
			release_sem(p->lock);
			dprintf(ID "dev 0x%08x status %d\n",cookie,s);
			return B_ERROR;
		}
		acquire_sem(p->done);
		sz -= 4096;
		buf += 4096;
	}
	release_sem(p->lock);
#endif
	return B_OK;
}


static status_t
tests_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}



