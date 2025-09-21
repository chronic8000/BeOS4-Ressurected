/* Driver for VirtualDSP MidiOxygen44
**
** Copyright 1999, Be Incorporated.  
** All rights reserved.
**
*/

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <USB.h>

#define ddprintf dprintf

#if 0
#define ID "vdsp: "
#else
#define ID "\033[36mvdsp:\033[0m "
#endif

#define VDSP_USB_VENDOR    0x0938
#define VDSP_USB_MO44      0x0001


/* SEND_DEPTH and RECV_DEPTH *must* be powers of two */

#define CHAN_COUNT 4
#define SEND_DEPTH 128
#define RECV_DEPTH 128
#define SEND_MASK (SEND_DEPTH-1)
#define RECV_MASK (RECV_DEPTH-1)
#define XFER_BUFS  2

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct
{
	uchar count;
	uchar data[3];
} chunk;

typedef struct 
{
	sem_id send_count;  /* number of *empty* entries in sendq */
	sem_id recv_count;  /* number of *filled* entries in recvq */

	uint32 sendq_count;
	uint32 sendq_head;
	uint32 sendq_tail;
	uchar sendq_bufr[SEND_DEPTH];
	
	uint32 recvq_count;
	uint32 recvq_head;
	uint32 recvq_tail;
	uchar recvq_bufr[RECV_DEPTH];
	
	uint32 stat_recv;  /* packets received */
	uint32 stat_drop;  /* packets discarded */
	uint32 stat_sent;  /* packets sent */
	
	struct usbdev *dev;
} channel;

/* an ingoing or outgoing transfer buffer used for usb transactions */
typedef struct xfer_buf
{
	struct xfer_buf *next;   /* list link                                */
	struct usbdev *dev;      /* device that owns this buffer             */
 	chunk *ch;	             /* actual data buffer (CHAN_COUNT chunks)   */
} xfer_buf;

typedef struct usbdev {
	struct usbdev *next;
	
	usb_device dev;         /* device handle for the stack, NULL if the */ 
		                     /* device is stale                          */
	
	int open_count;          /* number of open instances                 */ 
	
	usb_pipe out;           /* usb pipe handles for IO                  */ 
	usb_pipe in;

	const usb_configuration_info *conf;
		
	sem_id send_lock;        /* guards send xfer_bufs & enqueuing thereof */
	sem_id recv_lock;
	
	int32 sendq_lock;        /* protects all sendq_{buffer,count} fields  */
	int32 recvq_lock;        /* protects all recvq_{buffer,count} fields  */
	uint32 send_count;       /* total bytes ready to send across all chan */
	
	xfer_buf *send_bufs;     /* lists of buffers not queue'd              */
	xfer_buf *recv_bufs;

	/* no sense mallocing these all individually... */	
	xfer_buf xfer_bufs[XFER_BUFS]; 
	channel chan[CHAN_COUNT];
	
	uchar raw[(XFER_BUFS + 1) * (CHAN_COUNT * 4)];
	
	char name[64];
} usbdev;

static void queue_send(usbdev *dev, xfer_buf *xb);
static void queue_recv(usbdev *dev, xfer_buf *xb);


static void
recv_enqueue(channel *ch, int count, uchar *buf)
{
	ch->recvq_count += count;
	while(count){
		ch->recvq_bufr[ch->recvq_head] = *buf;
		ch->recvq_head = (ch->recvq_head + 1) & SEND_MASK;
		buf++;
		count--;
	}
}

static void
recv_dequeue(channel *ch, int count, uchar *buf)
{
	ch->recvq_count -= count;
	while(count){
		*buf = ch->recvq_bufr[ch->recvq_tail];
		ch->recvq_tail = (ch->recvq_tail + 1) & SEND_MASK;
		buf++;
		count--;
	}
}

static void
send_enqueue(channel *ch, int count, const uchar *buf)
{
	ch->sendq_count += count;
	while(count){
		ch->sendq_bufr[ch->sendq_head] = *buf;
		ch->sendq_head = (ch->sendq_head + 1) & SEND_MASK;
		buf++;
		count--;
	}
}

static void
send_dequeue(channel *ch, int count, uchar *buf)
{
	ch->sendq_count -= count;
	while(count){
		*buf = ch->sendq_bufr[ch->sendq_tail];
		ch->sendq_tail = (ch->sendq_tail + 1) & SEND_MASK;
		buf++;
		count--;
	}
}

#define LOCK(l,ps) { ps = disable_interrupts(); acquire_spinlock(&l); }
#define UNLOCK(l,ps) { release_spinlock(&l); restore_interrupts(ps); }

#define LOCK_RECVQ(dev,ps) LOCK((dev->recvq_lock),ps)
#define LOCK_SENDQ(dev,ps) LOCK((dev->sendq_lock),ps)
#define UNLOCK_RECVQ(dev,ps) UNLOCK((dev->recvq_lock),ps)
#define UNLOCK_SENDQ(dev,ps) UNLOCK((dev->sendq_lock),ps)

static usb_module_info *usb;

static char **dev_names = NULL;
static usbdev *dev_list = NULL;
static int dev_count = 0;
sem_id dev_list_lock = -1;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status_t 
readserial(usb_device dev, char *targ)
{
	static uchar buffer[128];
	static size_t sz;
	int n;
	
	buffer[0] = 0;
	buffer[1] = 0;
	
	sprintf(targ,"midi/mo/");
	if(usb->get_descriptor(dev, USB_DESCRIPTOR_STRING, 5, 0, buffer, 128, &sz) == B_OK){
		sz = (buffer[0] - 2) / 2;
		if(buffer[0] > 32) return -1;		
		for(n = 0; n < sz; n++){ 
			targ[8 + n] = buffer[2 + 2*n];
		}
		targ[8 + n] = '/';
		targ[9 + n] = 0;
		return 0;
	}
	return -1;
}

static usbdev*
add_device(usb_device dev, const usb_configuration_info *conf, int ifc)
{
	int i;
	chunk *ch;
	char name[32];
	uchar *x;
	usbdev *p;
	
	if(p = (usbdev *) malloc(sizeof(usbdev))){
		p->dev = dev;

		if(readserial(dev,p->name)){			
			dprintf(ID "cannot obtain serial number\n");
			return NULL;
		}
		
		sprintf(name,"vdsp:send_lock");
		p->send_lock = create_sem(1, name);
		sprintf(name,"vdsp:recv_lock");
		p->recv_lock = create_sem(1, name);

		p->sendq_lock = 0;
		p->recvq_lock = 0;
				
		p->send_count = 0;
		
		for(i=0;i<CHAN_COUNT;i++){
			sprintf(name,"vdsp:send_count:%d",i);
			p->chan[i].send_count = create_sem(SEND_DEPTH, name);
					
			sprintf(name,"vdsp:recv_count:%d",i);
			p->chan[i].recv_count = create_sem(0, name);
					
			p->chan[i].sendq_head = 0;
			p->chan[i].sendq_tail = 0;
			p->chan[i].sendq_count = 0;
			
			p->chan[i].recvq_head = 0;
			p->chan[i].recvq_tail = 0;
			p->chan[i].recvq_count = 0;
			
			p->chan[i].stat_recv = 0;
			p->chan[i].stat_drop = 0;
			p->chan[i].stat_sent = 0;
					
			p->chan[i].dev = p;
		}
				
		/* ensure that we're 16-byte aligned so we don't cross
			a physical page boundary. XXX assumption of chunk*4 */
		ch = (chunk *) ( (((uint32) p->raw) & 0xFFFFFFF0) + 0x10 );
				
		/* now init the transfer bufs -- split 'em half-n-half between
		   the send and recv lists */
		p->send_bufs = NULL;
		p->recv_bufs = NULL;
				
		for(i=0;i<XFER_BUFS;i++){
			if(i < (XFER_BUFS/2)){
				p->xfer_bufs[i].next = p->send_bufs;
				p->send_bufs = p->xfer_bufs + i;
			} else {
				p->xfer_bufs[i].next = p->recv_bufs;
				p->recv_bufs = p->xfer_bufs + i;
			}
			p->xfer_bufs[i].dev = p;
			p->xfer_bufs[i].ch = ch;
			ch += CHAN_COUNT;
		}
				
		p->open_count = 0;	
			
		p->conf = conf;
			
#if 0
		usb->set_configuration(dev,conf);
		p->in = conf->interface[ifc].active->endpoint[0].handle;
		p->out = conf->interface[ifc].active->endpoint[1].handle;
		dprintf(ID "added 0x%08x / 0x%08x / 0x%08x (/dev/%s])\n",
				p->dev,p->in,p->out,p->name);
#endif		
		return p;
	} else {
		return NULL;
	}
}

static void 
remove_device(usbdev *dev)
{
	int i;
	ddprintf(ID "removed 0x%08x (/dev/%s...)\n",dev,dev->name);
	delete_sem(dev->send_lock); dev->send_lock = -1;
	delete_sem(dev->recv_lock); dev->recv_lock = -1;
	for(i=0;i<CHAN_COUNT;i++){
		delete_sem(dev->chan[i].send_count); 
		dev->chan[i].send_count = -1;
		delete_sem(dev->chan[i].recv_count);
		dev->chan[i].recv_count = -1;
	}
}
	
static status_t 
device_added(usb_device dev, void **cookie)
{
	const usb_device_descriptor *descr = usb->get_device_descriptor(dev);
	const usb_configuration_info *conf;
	usbdev *u;
	
	if(conf = usb->get_nth_configuration(dev,0)){
		if((descr->vendor_id == VDSP_USB_VENDOR) &&
		   (descr->product_id == VDSP_USB_MO44)){				   
		   if((u = add_device(dev, conf, 0))){
			   *cookie = u;
			   acquire_sem(dev_list_lock);
			   u->next = dev_list;
			   dev_list = u;
			   dev_count ++;
			   release_sem(dev_list_lock);
			   
			   return B_OK;
		   }
		}
	}
	return B_ERROR;
}

static status_t 
device_removed(void *cookie)
{
	usbdev *dev,*last;
	acquire_sem(dev_list_lock);
	for(dev = dev_list,last = NULL; dev; dev = dev->next){
		if(dev == ((usbdev*) cookie)){
			if(last){
				last->next = dev->next;
			} else {
				dev_list = dev->next;
			}
			dev->dev = 0;
			remove_device(dev);
			if(!dev->open_count) free(dev);
			break;
		}
		last = dev;
	}
	dev_count --;
	release_sem(dev_list_lock);
	return B_OK;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void 
send_cb(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	xfer_buf *xb;
	usbdev *dev;
	
	xb = (xfer_buf *) cookie;
	dev = xb->dev;

//	dprintf(ID "send cb %d\n",status);
	
	if(status){
		if(dev->dev) dprintf(ID "bad stuff in send cb (%d)\n",status);
	}
	acquire_sem(dev->send_lock);	
	queue_send(dev, xb);
	release_sem(dev->send_lock);
}

static void 
recv_cb(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	cpu_status ps;
	int i,recv_count;
	xfer_buf *xb;
	usbdev *dev;
	chunk *ch;
	channel *chan;
	
	xb = (xfer_buf *) cookie;
	dev = xb->dev;

//	dprintf(ID "recv cb %d\n",status);
	
	if(status || (actual_len != 16)){
		if(dev->dev) dprintf(ID "bad stuff in recv cb (%d) we're toast...\n",status);
	} else {
		LOCK_RECVQ(dev,ps);
		for(i=0;i<CHAN_COUNT;i++){
			ch = xb->ch + i;
			chan = dev->chan + i;
			if(recv_count = ch->count){
//				kprintf("** recv %d on %d\n",recv_count,i);
				if(recv_count > (RECV_DEPTH - chan->recvq_count)){
					recv_count = (RECV_DEPTH - chan->recvq_count);
					chan->stat_drop += ch->count - recv_count;
//					dprintf("VDSP dropped %d bytes\n",chan->stat_drop);
				} 
				recv_enqueue(chan,recv_count,ch->data);
				chan->stat_recv += recv_count;
				release_sem_etc(chan->recv_count,recv_count,B_DO_NOT_RESCHEDULE);
			}
		}
		UNLOCK_RECVQ(dev,ps);
	}
	queue_recv(dev,xb);
}

/* dev->send_lock must be held while this is called */
static void
queue_send(usbdev *dev, xfer_buf *xb)
{
	chunk *ch;
	cpu_status ps;
	channel *chan;
	int i,send_count;

	if(xb){
		xb->next = dev->send_bufs;
		dev->send_bufs = xb;
	} else {
		xb = dev->send_bufs;
	}
	
	if(!dev->dev) return;
	
	while(dev->send_count && xb){
//		dprintf(ID "queue send %x %x %d\n",dev,xb,dev->send_count);
		LOCK_SENDQ(dev,ps);
		for(i=0;i<CHAN_COUNT;i++){
			ch = xb->ch + i;
			chan = dev->chan + i;
			if(send_count = chan->sendq_count){
				if(send_count > 3) send_count = 3;
				send_dequeue(chan,send_count,ch->data);
				ch->count = send_count;
				chan->stat_sent += send_count;
				dev->send_count -= send_count;
				release_sem_etc(chan->send_count,send_count,B_DO_NOT_RESCHEDULE);
			} else {
				ch->count = 0;
			}
		}
		UNLOCK_SENDQ(dev,ps);
		if(usb->queue_bulk(dev->out, xb->ch, CHAN_COUNT * 4, send_cb, xb)){
			dprintf(ID "cannot queue receive\n");
			break;
		}
		xb = dev->send_bufs = xb->next;
	}
}

static void 
queue_recv(usbdev *dev, xfer_buf *xb)
{
//	dprintf(ID "queue recv %x %x\n",dev,xb);
	
	acquire_sem(dev->recv_lock);
	if(xb){
		xb->next = dev->recv_bufs;
		dev->recv_bufs = xb;
	} else {
		xb = dev->recv_bufs;
	}

	if(!dev->dev) {
		release_sem(dev->recv_lock);
		return;
	}
	
	while(xb){
#if 0
		xb->ch[0].count = 3;
		xb->ch[0].data[0] = 0x80;
		xb->ch[0].data[1] = 0x42;
		xb->ch[0].data[2] = 0x42;
#endif
					
		if(usb->queue_bulk(dev->in, xb->ch, CHAN_COUNT * 4, recv_cb, xb)){
			dprintf(ID "cannot queue receive\n");
			break;
		}
		xb = dev->recv_bufs = xb->next;
	}
	release_sem(dev->recv_lock);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status_t
driver_open(const char *dname, uint32 flags, void **cookie)
{	
	int l;	
	status_t res = ENODEV;
	usbdev *dev;
	acquire_sem(dev_list_lock);
	
	for(dev = dev_list; dev; dev = dev->next){
		if(dev->dev){
			l = strlen(dev->name);
			if(!strncmp(dev->name,dname,l)){
				dprintf(ID "opening /dev/%s%02d\n",dev->name,atoi(dname+l));
				dev->open_count++;
				*cookie = dev->chan + atoi(dname + l);
				res = B_OK;
#if 1
				if(dev->open_count == 1) {
#if 1
					usb->set_configuration(dev->dev,dev->conf);
					dev->in = dev->conf->interface[0].active->endpoint[0].handle;
					dev->out = dev->conf->interface[0].active->endpoint[1].handle;
					dprintf(ID "configured 0x%08x / 0x%08x / 0x%08x (/dev/%s])\n",
							dev->dev,dev->in,dev->out,dev->name);
#endif					
					dprintf(ID "firing up listener\n");
					queue_recv(dev,NULL);
				}
#endif
				break;
			}
		}
	}
	
	release_sem(dev_list_lock);
	return res;
}

static status_t
driver_free (void *cookie)
{
	usbdev *dev = ((channel*)cookie)->dev;
	
	acquire_sem(dev_list_lock);
	dev->open_count--;
	if(dev->open_count == 0){
		if(!dev->dev) {
			/* if device no longer exists - destroy it */
			free(dev);
		} else {
			usb_device d = dev->dev;
			
			dev->dev = 0;
			/* cancel any remaining transfers */
			usb->cancel_queued_transfers(dev->in);
			usb->cancel_queued_transfers(dev->out);
			snooze(2000); // wait for it... this is not optimal XXX
			dev->dev = d;
			
#if 1
			usb->set_configuration(dev->dev,NULL); dev->in = 0; dev->out = 0;
#endif
		}
	}
	release_sem(dev_list_lock);
	return B_OK;
}

static status_t
driver_close(void *cookie)
{
	return B_OK;
}



static status_t
driver_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	channel *chan = (channel *) cookie;
	cpu_status ps;
	int sz = *count;
	
//	dprintf(ID "read %x %d %x %d\n",cookie,(int)pos,buf,(int) *count);
	
	if(acquire_sem_etc(chan->recv_count, sz, B_CAN_INTERRUPT, 0)){
		*count = 0;
		if(!chan->dev->dev) return ENODEV;
		return B_ERROR;
	}

	LOCK_RECVQ(chan->dev,ps);
	recv_dequeue(chan,sz,buf);
	UNLOCK_RECVQ(chan->dev,ps);
	
	return B_OK;
}

static status_t
driver_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	channel *chan = (channel *) cookie;
	usbdev *dev = chan->dev;
	cpu_status ps;
	int sz = *count;
	int send_count;	
	chunk data;	
	
//	dprintf(ID "write %x %d %x %d\n",cookie,(int)pos,buf,(int) *count);
	
	while(sz){
		/* packetize the data */
		send_count = sz > 8 ? 8 : sz;
		
		if(acquire_sem_etc(chan->send_count,send_count,B_CAN_INTERRUPT,0)){
			*count = 0;
			if(!dev->dev) return ENODEV;
			return B_ERROR;
		}
		
		LOCK_SENDQ(dev,ps)
		send_enqueue(chan, send_count, buf);
		UNLOCK_SENDQ(dev,ps);
		
		acquire_sem(dev->send_lock);	
		chan->dev->send_count += send_count;
		queue_send(dev,NULL);
		release_sem(dev->send_lock);
		
		buf += send_count;
		sz -= send_count;
	}	
	
	return B_OK;	
}


static status_t
driver_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static device_hooks hooks = {
	driver_open, 			
	driver_close, 			
	driver_free,			
	driver_control, 		
	driver_read,			
	driver_write,
	NULL,
	NULL,
	NULL,
	NULL		 
};

static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};

status_t 
init_hardware(void)
{
	return B_OK;
}                   

usb_support_descriptor supported_devices[] =
{
	{ 0, 0, 0, VDSP_USB_VENDOR, VDSP_USB_MO44 },
};

status_t
init_driver(void)
{
	ddprintf(ID "%s %s, init_driver()\n", __DATE__, __TIME__);
		
#if 1
	if(load_driver_symbols("vdsp") == B_OK) {
		ddprintf(ID "loaded symbols\n");
	} else {
		dprintf(ID "no symbols for you!\n");
	}
#endif
	
	if(get_module(B_USB_MODULE_NAME,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",B_USB_MODULE_NAME);
		return B_ERROR;
	} 
	
	dev_list_lock = create_sem(1,"vdsp:dev_list_lock");
	ddprintf(ID "USBD loaded okay\n");
	usb->register_driver("mo",supported_devices,1,NULL);
	usb->install_notify("mo",&notify_hooks);
	return B_OK;
	
err:
	put_module(B_USB_MODULE_NAME);
	return B_ERROR;
}

void
uninit_driver(void)
{
	int i;
	
	if(dev_names){
		for(i=0;dev_names[i];i++) free(dev_names[i]);
		free(dev_names);
	}
	
	ddprintf(ID "uninit_driver()\n"); 
	usb->uninstall_notify("mo");
	delete_sem(dev_list_lock);
	
	if(dev_list){
		dprintf(ID "EEK. devices still exist at teardown\n");
	}
	
	put_module(B_USB_MODULE_NAME);
}

const char**
publish_devices()
{
	int i,j,l;
	usbdev *dev;
	ddprintf(ID "publish_devices()\n"); 
	
	if(dev_names){
		for(i=0;dev_names[i];i++) free(dev_names[i]);
		free(dev_names);
	}
	
	acquire_sem(dev_list_lock);	
	dev_names = (char**) malloc(sizeof(char*) * (1 + CHAN_COUNT*dev_count));
	for(i = 0, dev = dev_list; dev; dev = dev->next){
		if(dev->dev) {
			for(j=0;j<CHAN_COUNT;j++){
				l = strlen(dev->name);
				dev_names[i] = (char*) malloc(strlen(dev->name) + 3);
				sprintf(dev_names[i],"%s%02d",dev->name,j);
				ddprintf(ID "publishing '%s'\n",dev_names[i]);
				i++;
			}
		}
	}
	dev_names[i] = NULL;
	release_sem(dev_list_lock);
	return (const char **) dev_names;
}

device_hooks*
find_device(const char* name)
{
	return &hooks;
}
