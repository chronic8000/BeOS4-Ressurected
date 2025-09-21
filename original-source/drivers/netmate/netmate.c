// CATC NetMate Link USB Ethernet Adapter

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ether_driver.h>
#include <USB.h>
#include <ByteOrder.h>

static status_t netmate_open(const char *name, uint32 flags, void **cookie);
static status_t netmate_free (void *cookie);
static status_t netmate_close(void *cookie);
static status_t netmate_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t netmate_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t netmate_write(void *cookie, off_t pos, const void *buf, size_t *count);
static void intr_notify(void *cookie, uint32 status, void *data, uint32 actual_len);
static void read_notify(void *cookie, uint32 status, void *data, uint32 actual_len);
static void write_notify(void *cookie, uint32 status, void *data, uint32 actual_len);
static unsigned long crcgen( unsigned char * dataPtr, unsigned int byteLen);
static void HashMacAddress( unsigned char Address[6], unsigned char Hash[64]);

#define ddprintf dprintf

#define PDEV_MAX 32

#define BUF_SIZE 1536 	// 1518 + 2 + rounded up to 64-byte boundary
#define USB_FRAGMENT 	64
#define INTR_SIZE 2 		// two bytes needed

#define TX_BUFFERS 1
#define RX_BUFFERS 32 	/* must be a power of 2 */
#define RX_MASK	(RX_BUFFERS-1)
#define USB_BUFFERS 1

#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

typedef struct {
	uchar  *data;
	uint16 busy, size;
} rx_descriptor;


typedef struct {
	int active;
	int open;
	
	usb_device *dev;
	usb_pipe *bulk_out;
	usb_pipe *bulk_in;
	usb_pipe *intr_ep;

	sem_id done;
	sem_id tx_done, rx_done;
	
	area_id buf_area_id, rx_buf_area_id;
	void *buf_area;

	uchar *tx_buf;

	/* rx buffers and  counters for reassembling usb fragments into ethernet frames */
	void *rx_buf;								/* rx buffers */
	rx_descriptor rx_desc[RX_BUFFERS];
	uint16 rx_accumulated, rx_len, rx_dump;					/* residual data from last usb rx buffer */
	volatile int16 rx_producer, rx_consumer;

	uchar *usb_buf;							/* usb buffer & current offset */
	uchar *intr_data;

	int32			readLock, writeLock, readnotifyLock;		/* to prevent reentrant read & write */
	
	uint32 blocking_flag;

	ether_address_t station_address;
	// multicast filter
	uchar mc_filter[64];
		
} netmatedev;

static void soft_reset(netmatedev *d);
static void toggle_led(netmatedev *d);
static void print_address(ether_address_t *addr);
static ether_address_t* get_station_address(netmatedev *d);
static status_t set_station_address(netmatedev *d);
static status_t set_promisc(netmatedev *d, uint32 on_off);


static const char *netmate_name[PDEV_MAX+1];

static netmatedev *pdev[PDEV_MAX];

#define ID "netmate: "


static device_hooks netmate_devices = {
	netmate_open, 			
	netmate_close, 			
	netmate_free,			
	netmate_control, 		
	netmate_read,			
	netmate_write			 
};

static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;
static int dev_count = 0;
sem_id dev_table_lock = -1;

static status_t add_device(usb_device *dev, usb_configuration_info *conf, int ifc)
{
	int i;
	char name[32];
	uchar *x;
	size_t act;
	status_t err;
	
	netmatedev *p;
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(!pdev[i]){
			if(p = (netmatedev *) malloc(sizeof(netmatedev))){
				int size, j;

				p->dev = dev;

				sprintf(name,"netmate:%d:tx_done",i);
				p->tx_done = create_sem(1,name);
				sprintf(name,"netmate:%d:rx_done",i);
				p->rx_done = create_sem(0,name);

				p->readLock = p->writeLock = p->readnotifyLock = 0;

				p->active = 1;
				p->open = 0;
				p->blocking_flag = 0;

				size = (BUF_SIZE * (TX_BUFFERS + USB_BUFFERS)) + 128;
				size = (size+(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

				/* allocate interrupt, tx and usb buffers */
				if ((p->buf_area_id = create_area("netmate_buffer_area", (void **)&(p->buf_area), 
							B_ANY_KERNEL_ADDRESS, RNDUP(size, B_PAGE_SIZE),
							B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA)) < 0) {
					dprintf("Netmate: create_area Error %x\n", err);
					delete_sem(p->tx_done);
					err = p->buf_area_id;
					free(p);
					return err;
				}
				p->intr_data = p->buf_area + 64;
				p->tx_buf = p->intr_data + 64;
				p->usb_buf = p->tx_buf + BUF_SIZE;
				
				/* allocate rx buffers */
				size = RNDUP(BUF_SIZE * RX_BUFFERS, B_PAGE_SIZE);
				if ((p->rx_buf_area_id = create_area("netmate_rx_buffers", &p->rx_buf,
					B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA )) < 0) {
					dprintf(" create rx buffer area failed %x \n", p->rx_buf_area_id);
					delete_area(p->buf_area_id);
					delete_sem(p->tx_done);
					err = p->rx_buf_area_id;
					free(p);
					return err;
				}
				/* initialize rx_buffer descriptors */
				for (j=0;j<RX_BUFFERS; j++) {
					p->rx_desc[j].data = (char *) (p->rx_buf + j * BUF_SIZE);
					p->rx_desc[j].busy = false;
					p->rx_desc[j].size = 0;
				}
				
				dprintf("netmate buf %x rx_buf %x\n", p->buf_area, p->rx_buf);


				// get a different interface (alternate)
				usb->set_alt_interface(dev, conf->interface->alt + 1);
				
				// set up endpoints
				usb->set_configuration(dev,conf);
				p->bulk_out = conf->interface[ifc].active->endpoint[0].handle;
				p->bulk_in = conf->interface[ifc].active->endpoint[1].handle;
				p->intr_ep = conf->interface[ifc].active->endpoint[2].handle;


				dprintf(ID "added 0x%08x / 0x%08x (interface %d) (/dev/net/netmate/%d)\n",
						p->dev,p->bulk_out,ifc,i);
				pdev[i] = p;
				if(x = (char *) malloc(256)){
					ether_address_t *tmp = get_station_address(pdev[i]);
					memcpy(&pdev[i]->station_address, tmp, sizeof(ether_address_t));
					if (tmp == NULL) {
						dprintf( ID "get_station_address fails\n");
					}
					else {
						dprintf(ID "station address: "); print_address(&pdev[i]->station_address);
					}
					free(x);
				}
				release_sem(dev_table_lock);
				return B_OK;				
			}
			break;
		}
	} 
	release_sem(dev_table_lock);
	return B_ERROR;
}

static void remove_device(netmatedev *p)
{
	int i;
	for(i=0;i<PDEV_MAX;i++) if(pdev[i] == p) break;
	if(i == PDEV_MAX) dprintf(ID "removed nonexistant device!?\n");
	ddprintf(ID "removed 0x%08x (/dev/net/netmate/%d)\n",p,i);
	delete_area(p->buf_area_id);
	delete_area(p->rx_buf_area_id);
	delete_sem(p->tx_done);
	delete_sem(p->rx_done);
	free(p);
	pdev[i] = 0;
}

static status_t 
notify_added(const usb_device *dev, void **cookie)
{
	int i;
	usb_configuration_info *conf;
	ddprintf(ID "notify added 0x%08x\n",dev);
	
	if(conf = (usb_configuration_info *)usb->get_nth_configuration(dev,0)){
		for(i=0;i<conf->interface_count;i++){
			ddprintf(ID "adding interface %d\n", i);
			*cookie = (void *)dev;
			return add_device((usb_device *)dev, conf, i);
		}
	} else {
		dprintf(ID "device has no config 0\n");
	}	
	return B_ERROR;
}

static status_t 
notify_removed(void *cookie)
{
	usb_device *dev = (usb_device*) cookie;
	int i;
		
	ddprintf(ID "notify removed 0x%08x\n",dev);
	
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(pdev[i] && (pdev[i]->dev == dev)){
			pdev[i]->active = 0;
			if(pdev[i]->open == 0) {
				remove_device(pdev[i]);
			} else {
				ddprintf(ID "device /dev/net/netmate/%d still open -- marked for removal\n",i);
			}
		}
	}
	release_sem(dev_table_lock);
	return B_OK;

}

static usb_notify_hooks notify_hooks =
{
	&notify_added,
	&notify_removed
};


_EXPORT
status_t init_hardware(void)
{
	ddprintf(ID "init_hardware()\n");
	return B_OK;
}                   


usb_support_descriptor supported_devices[] =
{
	{ 0, 0, 0, 0x0423, 0x000c }
};

_EXPORT
status_t
init_driver(void)
{
	int i;
	ddprintf(ID "%s %s, init_driver()\n", __DATE__, __TIME__);
		
	if(load_driver_symbols("netmate") == B_OK) {
		ddprintf(ID "loaded symbols\n");
	} else {
		ddprintf(ID "no symbols for you!\n");
	}
	
	if(get_module(usb_name,(module_info**) &usb) != B_OK){
		dprintf(ID "cannot get module \"%s\"\n",usb_name);
		return B_ERROR;
	} 
	
	dev_table_lock = create_sem(1,"netmate:devtab_lock");
	for(i=0;i<PDEV_MAX;i++) pdev[i] = NULL;
	netmate_name[0] = NULL;
	
	usb->register_driver("netmate",supported_devices,1,NULL);
	usb->install_notify("netmate",&notify_hooks);
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
	usb->uninstall_notify("netmate");
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++){
		if(pdev[i]){
			ddprintf(ID "huh what %d\n",i);
		}
	}
	release_sem(dev_table_lock);	
	delete_sem(dev_table_lock);
	put_module(usb_name);
	for(i=0;netmate_name[i];i++) free((char*) netmate_name[i]);
	ddprintf(ID "uninit_driver() done\n");
}

_EXPORT
const char**
publish_devices()
{
	int i,j;
//ddprintf(ID "publish_devices()\n"); 
	dprintf("publish_devices() netmate\n"); 
	for(i=0;netmate_name[i];i++) free((char *) netmate_name[i]);
	
	acquire_sem(dev_table_lock);
	for(i=0,j=0;i<PDEV_MAX;i++){
		if(pdev[i] && pdev[i]->active){
			if(netmate_name[j] = (char *) malloc(16)){
				sprintf((char *)netmate_name[j],"net/netmate/%d",i);
				ddprintf(ID "publishing %s\n",netmate_name[j]);
				j++;
			}
		}
	}
	release_sem(dev_table_lock);
	netmate_name[j] = NULL;
	return netmate_name;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
	return &netmate_devices;
}

static void
soft_reset(netmatedev *d)
{
	size_t act;
	if(usb->send_request(d->dev,0x40,0xf4,0,0,0,NULL,0,&act)){
		dprintf(ID "SOFT_RESET fails\n");
	} else {
		dprintf(ID "SOFT_RESET is happy enough\n");
	}
}

static void
toggle_led(netmatedev *d)
{
	size_t act;
	if(usb->send_request(d->dev,0x40,0xfa,0x08,0x81,0,NULL,0,&act)){
		dprintf(ID "TOGGLE_LED fails\n");
	} else {
		dprintf(ID "TOGGLE_LED is happy enough\n");
	}
}

static void
print_address(ether_address_t *addr)
{
	int i;
	char buf[3 * 6 + 1];
	for (i = 0; i < 5; i++)
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	dprintf("%s\n", buf);
}

#if 0
static void
dump_packet(const char * msg, unsigned char * buf) {
/* dump the first 3 x 16 bytes ... */
	dprintf("%s\n%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x \n", msg,
	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
	buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23],
	buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39],
	buf[40], buf[41], buf[42], buf[43], buf[44], buf[45], buf[46], buf[47]);
}
#endif


void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d \n", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
	dprintf("\n");
}


void dump_rx_desc(netmatedev *p) {
	
		int j;
		for (j=0;j<RX_BUFFERS;j++) {
			dprintf("rx_desc[%d] data=%8.8x size=%8.8x free=%2.2x\n",
				j,p->rx_desc[j].data, p->rx_desc[j].size, p->rx_desc[j].busy);
		}
	
}

static ether_address_t*
get_station_address(netmatedev *d)
{
	size_t act;
	ether_address_t *sa = (ether_address_t*)d->buf_area;

	if(usb->send_request(d->dev,0xc0,0xf2,0,0,6,sa,sizeof(ether_address_t),&act)){
		dprintf(ID "get_station_address fails\n");
		return (ether_address_t*)NULL;
	} 
	return sa;
}

static status_t
set_station_address(netmatedev *d)
{
	size_t act;
	ether_address_t *sa = &d->station_address;
	int i;
	
	dprintf(ID "set_station_address ");
	
	for (i = 0; i < 6; i++) {
		if(usb->send_request(d->dev, 0x40, 0xfa, (char)(sa->ebyte[i]), 0x67-i, 0, NULL, 0, &act))
		{
			dprintf(ID "set_station_address fails\n");
			return B_ERROR;
		} else {
			dprintf("%2.2x%c", (uchar)sa->ebyte[i], (i==5)?'\n':':');
		}
	}

	return B_OK;
}

static status_t set_promisc(netmatedev *d, uint32 on_off) {
	
	uint32 data_buffer;
	uchar mask[64];
	status_t result;
	uint16 j;
	size_t act;

	dprintf("set promisc\n");

	for ( j=0; j<63; j++) mask[j] = 0xff;
	
	data_buffer = 0;
	/* get register - data buffer*/
	if(result = usb->send_request(d->dev, 0xc0, 0xfb, 0, 0x60, 1, &data_buffer, 0, &act)) {
		dprintf("set reg failed %x\n", result);
	} else {
		dprintf("get register data_buf=%x\n", data_buffer);
		return (B_ERROR);
	}
	
	
	if (on_off) {
		dprintf("promiscuous mode ON - receive all frames\n");


		/* set mask */
//		if (result = usb->send_request(d->dev, 0x40, 0xfc, 0, 0x7a80, 64, mask, 0, &act))
		if (result = usb->send_request(d->dev, 0x40, 0xfc, 0, 0xfa80, 64, mask, 0, &act))
		{
			dprintf("set mask failed %x\n", result);
		} else {
			dprintf("set mask ok\n");
			return (B_ERROR);
		}
		
		/* set register - data buffer*/
		data_buffer |= 0x10;
		if(result = usb->send_request(d->dev, 0x40, 0xfa, data_buffer, 0x60, 0, NULL, 0, &act)) {
			dprintf("set reg failed %x\n", result);
		} else {
			dprintf("get register data_buf=%x\n", data_buffer);
			return (B_ERROR);
		}
	
	} else {
		dprintf("promiscuous mode off... to do...\n");
	}

	return (B_OK);
}

static status_t
netmate_open(const char *dname, uint32 flags, void **cookie)
{	
	
	int i;
	status_t err;


	i = atoi(dname + strlen("net/netmate/")); 

	ddprintf(ID "opening \"%s\"\nnetmate version %s %s\n",dname, __DATE__ , __TIME__ );
	acquire_sem(dev_table_lock);
	if(pdev[i]){
		if(pdev[i]->active){
			size_t act;
			
			pdev[i]->open++;
			*cookie = pdev[i];	
			release_sem(dev_table_lock);

			soft_reset(pdev[i]);

			set_station_address(pdev[i]);

			/* set up receiver */
			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0x0c,0x21,0,NULL,0,&act)){
				dprintf(ID "set r/w combined mode failed at 0x21\n");
				return B_ERROR;
			}
			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0x08,0x20,0,NULL,0,&act)){
				dprintf(ID "set r/w combined mode failed at 0x20\n");
				return B_ERROR;
			}

#if 0		// for some reason this causes a segment violation!
			{ 


				if(usb->send_request(pdev[i]->dev,0xc0,0xfb,0,
						0x60,1,(char *)pdev[i]->buf_area,1,&act)){
					dprintf(ID "read rx filter (for no obvious reason) failed\n");
					return B_ERROR;
				} else { dprintf("rx filter is %x\n", *(char *)pdev[i]->buf_area); }
			}
#endif

			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0xb,0x60,0,NULL,0,&act))
			{
				dprintf(ID "set rx exact match failed\n");
				return B_ERROR;
			}
			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0x19,0x22,0,NULL,0,&act)){
				dprintf(ID "set r/w combined mode at 0x22 failed\n");
				return B_ERROR;
			}
			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0x4,0x25,0,NULL,0,&act)){
				dprintf(ID "set frame burst size failed\n");
				return B_ERROR;
			}


			if(usb->send_request(pdev[i]->dev,0x40,0xfa,0x8,0x81,0,NULL,0,&act)){
				dprintf(ID "turning on LED failed\n");
				return B_ERROR;
			}

			/* set multicast filter */
			{   short j;
				uchar bcast[6];
				uchar * mc_filter = pdev[i]->mc_filter;
				
				/* add broadcast address to the rx filter */
				for (j=0; j<64; j++) mc_filter[j] = 0;
				for (j=0; j<6; j++) bcast[j] = 0xff;  
				HashMacAddress(bcast, mc_filter);
				
				if(usb->send_request(pdev[i]->dev,0x40,0xfc,0x0,0x7a80,
					64,(void *) mc_filter , 64, &act)){

					dprintf(ID "enabling receiver fails: act = %d\n", act);
					return B_ERROR;
				}
			}

			pdev[i]->rx_len = pdev[i]->rx_accumulated = pdev[i]->rx_dump = 0;
			pdev[i]->rx_producer = pdev[i]->rx_consumer = 0;

			// enable interrupts for rx and link status
			if(usb->queue_interrupt(pdev[i]->intr_ep, pdev[i]->intr_data, 2, intr_notify, pdev[i])){
				dprintf(ID "couldn't queue interrupt\n");
			}
			
			/* prime the pump for the first receive */
			dprintf("USB queue bulk_in - prime\n");
			if(err = usb->queue_bulk(pdev[i]->bulk_in, pdev[i]->usb_buf, BUF_SIZE, read_notify, pdev[i])) {
				dprintf(ID "dev open 0x%08x Error\n",err);
				return err;
			}

			return B_OK;
		} else {
			dprintf(ID "device is going offline. cannot open\n");
		}
	}
	release_sem(dev_table_lock);
	return B_ERROR;
}
 
static status_t
netmate_free (void *cookie)
{
	netmatedev *p = (netmatedev *) cookie;

	soft_reset(p);
	
	acquire_sem(dev_table_lock);
	p->open--;

	usb->cancel_queued_transfers(p->bulk_in);
	usb->cancel_queued_transfers(p->bulk_out);
	snooze(2000); // wait for it... this is not optimal XXX


	if((p->open == 0) && (p->active == 0)) remove_device(p);
	release_sem(dev_table_lock);
	return B_OK;
}

static status_t
netmate_close(void *cookie)
{
	return B_OK;
}

static status_t
netmate_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	netmatedev *p = (netmatedev *) cookie;
	status_t err;
	uint16 rx_len;
	
//	dprintf(ID "entering read()\n");

	if((err = acquire_sem_etc( p->rx_done, 1, B_CAN_INTERRUPT | p->blocking_flag, 0 )) != B_NO_ERROR) {
		*count = 0;
		return err;
	}

	if( atomic_or( &p->readLock, 1 ) ) {
		release_sem_etc( p->rx_done, 1, 0 );
		dprintf("Netmate: reentrant read\n");
		*count = 0;
		return B_ERROR;
	}

//	dprintf("NetmateRead() rx_desc[%4.4x] data=%x buf_len = %d\n",
//		p->rx_consumer, p->rx_desc[p->rx_consumer].data, p->rx_desc[p->rx_consumer].size);
//	dump_rx_desc(p);
	 	
	if (p->rx_desc[ p->rx_consumer].size > *count) {
		dprintf("RX len err %d asked for < %d received\n", *count, p->rx_desc[p->rx_consumer].size);	 
	 	p->rx_desc[p->rx_consumer].size = *count;
	 }
	if (p->rx_desc[p->rx_consumer].busy == false) {  /* rx_reset can cause this */
		dprintf("rx_[%d].busy = false frame dropped\n",  p->rx_consumer);
		*count = 0;
		p->readLock = 0;
		return B_OK;
	}
	memcpy(buf, p->rx_desc[p->rx_consumer].data, p->rx_desc[p->rx_consumer].size);
	*count = p->rx_desc[p->rx_consumer].size;
	p->rx_desc[p->rx_consumer].busy = false;
	p->rx_consumer = (p->rx_consumer+1) & RX_MASK;
	
	/* dump ping sequence numbers */
//	{ unsigned short  *seq = (unsigned short *) buf;
//		dprintf(" R%4.4x ", B_BENDIAN_TO_HOST_INT16(seq[20]));  /* sequence number of a ping frame */
//	}

//	dump_packet(ID "read", buf, *count);
//	dump_packet(ID "read", buf, 48);
	p->readLock = 0;
	return B_OK;
}

void reset_rx(netmatedev * p ) {
	int j;
	
	p->rx_len = p->rx_accumulated = p->rx_dump = 0;
	p->rx_producer = p->rx_consumer = 0;
	for (j=0; j< RX_BUFFERS; j++) {
		p->rx_desc[j].busy = false;
		p->rx_desc[j].size = 0;
	}
	dprintf("reset_rx() -- Yikes!!! \n");
}

void dump_usb(unsigned char * data, unsigned char * offset, uint16 len ) {
	int j, here;
	dprintf("USB dump %x offset %x len %x offset", data, offset, len, offset-data);
	
	here = offset-data;
	for (j=0; j<len; j++) {
		if ((j & 0xF) == 0) dprintf("\n %4.4x: ",j);	
		if ( j==here) dprintf(" ->");
		dprintf("%2.2x ", data[j]);
	}
	dprintf("\n");
}

/* read_notify() reassembles usb fragments (64 byte chunks) into ethernet frames and
   signals netmate_read when an ethernet frame has been assembled.
 */
void read_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	netmatedev *p = (netmatedev *) cookie;
	int16 rx_len, rx_len_padded;
	char * usb_cur;
	status_t err;

	
	int16 residual = actual_len;
	
//	dprintf("RN+++ rx_acc=%d actual=%d\n", p->rx_accumulated, actual_len);	

	if (status) {
		dprintf("read_notify STATUS %x\n", status);
		return;
	}
	if (actual_len == 0) {
//		dprintf("read_notify 0 len\n");
		if(err = usb->queue_bulk(p->bulk_in, p->usb_buf, BUF_SIZE, read_notify, p)) {
			dprintf(ID "read_notify() cant queue partial usb read rx  status %d\n",err);
		}
		return;
	}

	if( atomic_or( &p->readnotifyLock, 1 ) ) {
		dprintf("Netmate: reentrant read_notify()\n"); // this should never happen
		return;
	}

	usb_cur = (char *) data;
//dump_usb(data, usb_cur, actual_len);
	
	do {
		if (p->rx_accumulated == 0) { /* no history, ethernet data starts at top of usb buffer */
		
			rx_len = *((uint16 *) usb_cur);		 /* get the ethernet frame length */
			if (rx_len > 1518) {  /* lost the header,  probably interpreting frame data as a lenght --- bail */
				dprintf("read_notify() out of sync rx_len=%x data=%x usb_cur=%x acutal_len=%x residual=%x p->rxlen=%x\n",
					rx_len, data, usb_cur, actual_len, residual, p->rx_len );
				dump_usb(data, usb_cur, actual_len);
				reset_rx(p);
				break;
			}
			rx_len_padded = RNDUP(rx_len + 2, USB_FRAGMENT);

			if ((residual - rx_len_padded) >= 0) {	/* we have a whole ethernet frame */
				residual -=  rx_len_padded;
//				dprintf("rx whole %d prod=%d cons=%d\n", rx_len, p->rx_producer, p->rx_consumer);
				if (p->rx_desc[p->rx_producer].busy) {
					dprintf("rx overflow prod=%d consumer=%d\n", p->rx_producer, p->rx_consumer);
//					dump_rx_desc(p);
					reset_rx(p);
					break;
				} else {
					memcpy(p->rx_desc[p->rx_producer].data, usb_cur + 2, rx_len);
					p->rx_desc[p->rx_producer].size = rx_len;
					p->rx_desc[p->rx_producer].busy = true;
				
					p->rx_producer = (p->rx_producer + 1 ) & RX_MASK;
					release_sem_etc( p->rx_done, 1, B_DO_NOT_RESCHEDULE );
				}
			} else { /* ethernet frame spans the usb buffer - first part at end of usb buffer*/
				p->rx_accumulated = residual - 2;
				residual = 0;
				p->rx_len = rx_len;
//				dprintf("rx first part %d prod=%d cons=%d\n", p->rx_accumulated,p->rx_producer, p->rx_consumer);
//				dump_usb(data, usb_cur, actual_len);
				if (p->rx_desc[p->rx_producer].busy) {
					dprintf("dumping 1st half\n");
					p->rx_dump = true; /* dump the 2nd part of the buffer */
				} else {
					p->rx_desc[p->rx_producer].busy = true;
					memcpy(p->rx_desc[p->rx_producer].data, usb_cur+2, p->rx_accumulated);
					break;
				}
			}
				
			usb_cur += rx_len_padded;
	
		} else {		/* ethernet frame spans usb buffer - pick up residual beginning of usb buffer */

			uint16 len_2nd_part = p->rx_len - p->rx_accumulated;			
			if (len_2nd_part > 1520) {
				dprintf("read_notify() residual len err %x\n",len_2nd_part);
				dump_usb(data, usb_cur, actual_len);
			}
//			dprintf("r_n() 2nd half %d check p->rx_len = %d\n", len_2nd_part, p->rx_len);
			if (p->rx_dump) {
				dprintf("dumping 2nd half\n");
				p->rx_dump = false;
			} else {
				memcpy(p->rx_desc[p->rx_producer].data + p->rx_accumulated, usb_cur, len_2nd_part);
				p->rx_desc[p->rx_producer].size = p->rx_len;
				p->rx_producer = (p->rx_producer + 1 ) & RX_MASK;
				release_sem_etc( p->rx_done, 1, B_DO_NOT_RESCHEDULE );
			}
			p->rx_accumulated = 0;
			residual -= RNDUP(len_2nd_part, USB_FRAGMENT);
			usb_cur += RNDUP(len_2nd_part, USB_FRAGMENT);
		}

	} while (residual > 0);

//	dprintf("RN---\n");
	if(err = usb->queue_bulk(p->bulk_in, p->usb_buf, BUF_SIZE, read_notify, p)) {
		dprintf(ID "read_notify() cant queue partial usb read rx  status %d\n",err);
	}
	p->readnotifyLock = 0; // allow another read_notify()
}

void write_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	netmatedev *p = (netmatedev *) cookie;
	
	release_sem(p->tx_done);
//	dprintf(ID "write_notify: dev 0x%08x status %d, actual %d\n",cookie,status,actual_len);
}

void intr_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	netmatedev *p = (netmatedev *) cookie;

//	dprintf(ID "intr_notify: dev 0x%08x status %d, actual %d, data %x\n",cookie,status,actual_len, *(unsigned short *)data);


	/* it would be nice to have code here to detect link changes (cable plugged in or unplugged)
	   but it generates a lot of queue_dpc() errors from usb - fix someday...
	 */
#if FIXED_QUEUE_DPC_ERRORS
	if(usb->queue_interrupt(p->intr_ep, p->intr_data, 2, intr_notify, p)){
		dprintf(ID "intr_notify seemed to fail to queue an interrupt\n");
	}
#endif
}

static status_t
netmate_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	netmatedev *p = (netmatedev *) cookie;
	int sz = *count + 2; // packet + length
	status_t s, status;
	
//	dprintf(ID "write: dev 0x%08x write %d\n",cookie,sz);
	
	if( (status = acquire_sem_etc(p->tx_done, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR) {
		*count = 0;
		return status;
	}

	if( atomic_or(&p->writeLock, 1)) {	/* Protect againsts reentrant write */
		release_sem_etc(p->tx_done, 1, 0);
		dprintf("Netmate: reentrant write\n");
		*count = 0;
		return B_ERROR;
	}

	// copy the data - the first two bytes are the length
	memcpy(p->tx_buf + 2,buf,sz>(BUF_SIZE-2)?(BUF_SIZE-2):sz);
	(p->tx_buf)[0] = (uchar)(*count % 256);
	(p->tx_buf)[1] = (uchar)(*count >> 8);

	if(s = usb->queue_bulk(p->bulk_out, p->tx_buf, sz>BUF_SIZE?BUF_SIZE:sz, write_notify, p)){
		*count = 0;
		dprintf(ID "write error: dev 0x%08x status %d\n",cookie,s);
		p->writeLock = 0;	/* another write may now happen */
		return B_ERROR;
	}

//	dump_packet(ID "tx", p->tx_buf, *count+2);
	
	p->writeLock = 0;	/* another write may now happen */

	return B_OK;
}


static status_t
netmate_control (void *cookie, uint32 msg, void *arg, size_t len)
{
	netmatedev *p = (netmatedev *) cookie;

	struct {
		char bus;
		char device;
		char function;
	} params;

	switch (msg) {
	case ETHER_GETADDR:
		memcpy(arg, &p->station_address, sizeof(ether_address_t));
		return (B_NO_ERROR);
	case ETHER_INIT:
		params.bus = 0;
		params.device = 0;
		params.function = 0;
		memcpy(&params, arg, sizeof(params));
		return (B_NO_ERROR);
	case ETHER_NONBLOCK:
		if (*((int32*)arg))
			p->blocking_flag = B_TIMEOUT;
		else
			p->blocking_flag = 0;
		return (B_NO_ERROR);
	case ETHER_ADDMULTI:
		dprintf(ID "unimplemented ioctl(): ETHER_ADDMULTI\n");	
		return (B_ERROR);
		break;
	case ETHER_GETFRAMESIZE:
		*(uint32 *)arg = 1514;
		return B_NO_ERROR;
	default:
		dprintf(ID "unknown ioctl(): msg=%d\n", msg);	
	}

	return B_ERROR;
}

enum hash_values {
	CRCVAL=0x02608edb,
	CRCCHECK=0xc704dd7b
};

static unsigned long crcgen( unsigned char * dataPtr, unsigned int byteLen) {
	unsigned int byteLoop, bitLoop;
	unsigned long crc = 0xFFFFFFFF;
	unsigned char curByte, crcBit;

	for (byteLoop = 0; byteLoop < byteLen; byteLoop++) {
		curByte = *dataPtr++;
		for (bitLoop = 0; bitLoop < 8; bitLoop++, curByte>>=1) {
			crcBit = (curByte & 0x01) ^ ((crc & 0x80000000) >> 31);
			crc = (crc & 0x7fffffff) ^ ((crcBit)?CRCVAL:0);
			crc=(crc<<1) | crcBit;
		}
	}
	return crc;
}

static void HashMacAddress( unsigned char Address[6], unsigned char Hash[64]) {
	unsigned int loop;
	unsigned short hashIndex = 0;
	unsigned long crc;
	
	crc = crcgen(Address,6);
	
	for  (loop = 0; loop<9; loop++) {
		hashIndex = (hashIndex<<1)|((crc&(1<<(loop+23)))!=0);
	}
	Hash[(hashIndex & 0x1F8)>>3] |= (1<<(hashIndex & 0x7));
}	


