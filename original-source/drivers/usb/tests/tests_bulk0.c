#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <byteorder.h>

#include <USB.h>

#define	VENDOR_ID	0x6666
#define DEVICE_ID	0x6969	

#define AS			(4*1024*1024)

static status_t tests_open(const char *name, uint32 flags, void **cookie);
static status_t tests_free (void *cookie);
static status_t tests_close(void *cookie);
static status_t tests_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t tests_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t tests_write(void *cookie, off_t pos, const void *buf, size_t *count);

#define ddprintf dprintf 

#define PDEV_MAX 32
#define QS	(16*1024+256)//(64*64)
#define	MAX_BUFFERS	3
#define IOVEC_NUM 	7


typedef struct {
	int active;
	int open;
	usb_device *dev;
	//usb_pipe *out;
	//usb_pipe *in;
	usb_id in;
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


static void dump_memory_map(void* addr, size_t size)
{
	/* KISS version */
	physical_entry	pe;
	uint8* ptr = addr;
	
	dprintf("Memory map of %ld bytes @ %p:\n", size, addr);
	while(size)
	{
		get_memory_map(ptr, size, &pe, 1);
		dprintf("\t%ld @ %p/%p\n", pe.size, ptr, pe.address);
		ptr += pe.size;
		size -= pe.size;
	}
}

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
				p->dev = dev;
				sprintf(name,"usb_tests:%d:lock",i);
				p->lock = create_sem(1,name);
				sprintf(name,"usb_tests:%d:done",i);
				//p->done = create_sem(0,name);
				p->done = create_sem(MAX_BUFFERS, name);
				p->active = 1;
				p->open = 0;
				p->hack_id = create_area("usb_tests_hack", (void **)&(p->hack), 
										 B_ANY_KERNEL_ADDRESS,
										AS,	//(QS + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1),
										 B_FULL_LOCK, //B_CONTIGUOUS, 
										 B_READ_AREA | B_WRITE_AREA);
				dprintf("usb_tests:%d: %d @ 0x%08x\n",i,p->hack_id,p->hack);
				//dump_memory_map(p->hack, AS);
				
				usb->set_configuration(dev,conf);
				p->in = conf->interface[ifc].active->endpoint[0].handle;
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
	delete_area(p->hack_id);
	delete_sem(p->lock);
	delete_sem(p->done);
	free(p);
	pdev[i] = 0;
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
			add_device(dev, conf, 0);
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


#if 0
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
#endif


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
		
#if 0		
	if(load_driver_symbols("usb_tests") == B_OK) {
		ddprintf(ID "loaded symbols\n");
	} else {
		ddprintf(ID "no symbols for you!\n");
	}
#endif
	
	if(get_module(usb_name,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",usb_name);
		return B_ERROR;
	} 
	
	dev_table_lock = create_sem(1,"usb_tests:devtab_lock");
	for(i=0;i<PDEV_MAX;i++) pdev[i] = NULL;
	tests_name[0] = NULL;
	
	usb->register_driver("usb_tests",supported_devices,1, NULL);
	usb->install_notify("usb_tests", &notify_hooks);
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

/*
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
*/

static status_t
tests_open(const char *dname, uint32 flags, void **cookie)
{	
	int i;
	i = atoi(dname + strlen("misc/usb/")); 
	
	ddprintf(ID "opening \"%s\"\n",dname);
	acquire_sem(dev_table_lock);
	if(pdev[i]){
		if(pdev[i]->active){
			pdev[i]->open++;
			*cookie = pdev[i];	
			release_sem(dev_table_lock);
			//soft_reset(pdev[i]);
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


#if 0
static status_t
tests_read(void *cookie, off_t pos, void *buf, size_t *count)
{
#if 1
	static bigtime_t	t_sum = 0;
	static	unsigned	call_count = 0, read_sum = 0;
	bigtime_t			t0;

	int8*				hack_ptr;
	unsigned			queue_count;	

	testsdev *p = (testsdev *) cookie;
	
	size_t remain_count;
	size_t qs;
	status_t s, wait_status;
	status_t	ret = B_OK;
	
	//dprintf(ID "dev 0x%08x read %d\n", cookie, *count);
	
	/* set read buffer to the known pattern for debugging/testing */
	memset(buf, 0xFF, *count);
	memset(p->hack, 0xAA, AS);
	
	//*count = 4096;
	remain_count = *count;
	
	//acquire_sem(p->lock);
	t0 = system_time();
	*count = 0;
	
	hack_ptr = p->hack;
	queue_count = 0; 
	while(remain_count > 0 && queue_count < MAX_BUFFERS )
	{
		//snooze(rand()%2100);
		qs = (remain_count>QS) ? QS : remain_count;

		if(s = usb->queue_bulk(p->in, hack_ptr, qs, cb_notify, p))
		{
			*count = 0;
			//release_sem(p->lock);
			dprintf(ID "dev 0x%08x status %d\n", cookie, s);
			return B_ERROR;
		}
		hack_ptr += qs; 
		queue_count++;

		remain_count -= qs;
		(*count) += qs; 
	}

	wait_status = acquire_sem_etc(p->done, queue_count, B_CAN_INTERRUPT | B_TIMEOUT, (*count)*2000 );	/* wait untill ALL callbacks */
	switch(wait_status)
	{
	case B_INTERRUPTED:
		dprintf("Interrupted, cleaning up...\n");
		usb->cancel_queued_transfers(p->in);
		dprintf("Done cleaning\n");
		*count = 0;
		return B_INTERRUPTED;
	
	case B_TIMED_OUT:
		dprintf("Timeout, cleaning up...\n");
		dprintf("queue_count = %u, count = %u\n", queue_count, *count);
		//kernel_debugger("\n");
		usb->cancel_queued_transfers(p->in);
		dprintf("Done cleaning\n");
		ret = B_ERROR;
	}

	t_sum += system_time()-t0;
	read_sum += *count; 
	call_count++;
	
	memcpy(buf, p->hack, *count);
	
	{
		static uint16	magic;
		static bool		first_read = TRUE;
	
		int	i;
		uint8* ptr = buf;

		if(first_read)
		{
			first_read = FALSE;
			magic = B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr));
			--magic;
			dprintf("Initial magic is %04x\n", magic);
		}
		
		//dprintf("Testing... ");
		for(; ptr<(uint8*)buf+(*count); ptr += 64)
		{
			//dprintf("%04x ", B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
			
			if(++magic != B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)) )
			{
				ret = B_ERROR;
				dprintf("Error: buf = 0x%08x, ptr = 0x%08x, expected %04x, got %04x\n", buf, ptr, magic, B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
				//kernel_debugger("\n");
				//dprintf("%08x %04x %04x      ", ptr, magic, B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
				for(i=0,ptr = buf; ptr<(uint8*)buf+(*count); ptr += 64,i++)
				{
					if((i & ((QS-1)/64)) == 0)
						dprintf("\n");
					if((i & 0xF) == 0)
						dprintf("\n%04x:  ", i);

					dprintf("%04x ", B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
				}
				dprintf("\n");
				kernel_debugger("\n");
				break;			
			}
		}
		//dprintf("done\n");
	}

	//release_sem(p->lock);
	if(read_sum > 500*1024)
	{
		dprintf("Read rate %Ld kB/s\n", (1000*read_sum)/t_sum);
		t_sum = 0;
		read_sum = 0;
		call_count = 0;
	}
	if(ret != B_OK)
		*count = 0;
	return ret;

#else
	*count = 0;
	return B_OK;
#endif
}
#endif



bool packet_has_errors(const uint8* packet_ptr, uint16 magic)
{
	const uint32 uc_magic = B_BENDIAN_TO_HOST_INT16(magic) | ((~B_BENDIAN_TO_HOST_INT16(magic)) << 16);
	unsigned i;
	
	if(uc_magic != *(const uint32*)packet_ptr)
	{
		dprintf("Error @ %p, magic: expected %08x, got %08x\n", packet_ptr, uc_magic, *(uint32*)packet_ptr);
		return TRUE;
	}
	for(i=4; i<64; i++)
	{
		if( packet_ptr[i] != i)
		{
			dprintf("Error @ %p, expected %02x, got %02x\n", i, packet_ptr[i]);
			return TRUE;
		}
	}
	return FALSE;
}



static status_t
tests_read(void *cookie, off_t pos_, void *buf_, size_t *count)
{
	status_t 	ret = B_ERROR;
	unsigned	read_sum;
	unsigned	read_total;
	bigtime_t	t, t0;
	int			read_count;

	uint8	*start_ptr, *end_ptr;
	uint8	*cbuf, *tbuf; 

	size_t qs;
	status_t s, wait_status;

	testsdev * const p = (testsdev *) cookie;
	
	dprintf(ID "dev 0x%08x read %d\n", cookie, *count);
	
	srand(0);
	
	*count = 0;
	
	cbuf = tbuf = start_ptr =	((uint8*)(p->hack)) + B_PAGE_SIZE + 5 ;
	end_ptr = (uint8*)(p->hack) + AS - B_PAGE_SIZE;  

	
	t0 = system_time();
	read_total = read_sum = 0;
	read_count = -MAX_BUFFERS;
	qs = QS;
	while(1)
	{
		if(snooze_etc(300, B_SYSTEM_TIMEBASE, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT) != B_OK)
		{
			dprintf("Interrupted, cleaning up...\n");
			usb->cancel_queued_transfers(p->in);
			dprintf("Done cleaning\n");
			ret = B_INTERRUPTED;
			goto end;
		}
	
		wait_status = acquire_sem_etc(p->done, 1, B_CAN_INTERRUPT | B_TIMEOUT, qs*2000*200 );	
		switch(wait_status)
		{
		case B_INTERRUPTED:
			dprintf("Interrupted, cleaning up...\n");
			usb->cancel_queued_transfers(p->in);
			dprintf("Done cleaning\n");
			ret = B_INTERRUPTED;
			goto end;

		case B_TIMED_OUT:
			dprintf("Timeout, cleaning up...\n");
			//kernel_debugger("\n");
			usb->cancel_queued_transfers(p->in);
			dprintf("Done cleaning\n");
			ret = B_TIMED_OUT;
			goto end;
		}

		read_count++;

	
		if(read_count > 0)
		{
			read_total += qs;			
			
			/* do test of data @ tbuf */
			{
				static uint16	magic;
				static bool		first_read = TRUE;
			
				int	i;
				uint8* ptr = tbuf;
		
				if(first_read)
				{
					first_read = FALSE;
					magic = B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr));
					--magic;
					dprintf("Initial magic is %04x\n", magic);
				}
				
				//dprintf("Testing... ");
				for(; ptr<(uint8*)tbuf+ qs; ptr += 64)
				{
					if(packet_has_errors(ptr, ++magic))
					{
						dprintf("buf = %p, ptr = %p\n", tbuf, ptr);
						dprintf("read_count = %d, read_total = %u/0x%08x, pages = %u\n", read_count, read_total, read_total, read_total/B_PAGE_SIZE);						
						//kernel_debugger("\n");
						//dprintf("%08x %04x %04x      ", ptr, magic, B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
						for(i=0,ptr = tbuf; ptr<(uint8*)tbuf + qs; ptr += 64,i++)
						{
							if((i & ((QS-1)/64)) == 0)
								dprintf("\n");
							if((i & 0xF) == 0)
								dprintf("\n%04x:  ", i);
		
							dprintf("%04x ", B_BENDIAN_TO_HOST_INT16(*((uint16*)ptr)));
						}
						dprintf("\n");
						kernel_debugger("\n");
						
						goto end;			
					}
				}
				//dprintf("done\n");
			}

			tbuf += qs; 
			if((tbuf + qs) >= end_ptr)
				tbuf = start_ptr;
				
			read_sum += qs; 
			if(read_sum > 1000*1024)
			{
				t = system_time();
				dprintf("%Ld B/ms\n", (1000*read_sum)/(t-t0));
				read_sum = 0;
				t0 = t;
			}
		}


		{
			iovec vecs[IOVEC_NUM];
			uint8*	vptr = cbuf;
			size_t	remainder = qs;
			int		v, i;

			for(v=0; (v<IOVEC_NUM-1) && (remainder != 0) ; v++)
			{
				const size_t s = rand() % remainder + 1;
				vecs[v].iov_base = vptr;
				vecs[v].iov_len	= s;
				vptr += s;
				remainder -= s;
			}
			if(remainder != 0)
			{
				vecs[v].iov_base = vptr;
				vecs[v].iov_len = remainder;
				v++;
			}
			/*
			for(i=0; i<v; i++)
				dprintf("%ld ", vecs[i].iov_len);			
			dprintf("\n");
			*/
			
			memset(cbuf, 0xf5, qs);

			//if(s = usb->queue_bulk(p->in, cbuf, qs, cb_notify, p))
			if(s = usb->queue_bulk_v(p->in, &vecs, v, cb_notify, p))
			{
				dprintf(ID "dev 0x%08x status %d\n", cookie, s);
				goto end;
			}
			memset(vecs, 0x0, sizeof(vecs));
		}

		cbuf += qs; 
		if((cbuf + qs) >= end_ptr)
			cbuf = start_ptr;

	}
	
end:
	dprintf(ID "dev 0x%08x read %d done\n", cookie, *count);
	return B_ERROR;
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



