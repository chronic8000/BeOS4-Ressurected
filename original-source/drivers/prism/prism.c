/*                	
 * prism.c 
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 * hank@sackett.net
 */

/* set INTERRUPT_BROKEN nonzero if the BIOS is broken and doesn't
   route PCMCIA hardware interrutps */
#define INTERRUPTS_BROKEN 0

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <module.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <driver_settings.h>


#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

#define DOING_DPC 0
#if DOING_DPC
#include <dpc.h>
#endif

#include "wlan_compat.h"

#include "version.h"
#include "p80211types.h"
#include "p80211hdr.h"
#include "p80211mgmt.h"
#include "p80211conv.h"
#include "p80211msg.h"
#include "p80211netdev.h"
#include "hfa384x.h"
#include "p80211metastruct.h"

#include "net_data.h"

#define kDevName "prism"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			

// this is a hack for testing
//int open_ref_count = 0;

#define PRISM2STA_MAGIC             (0x4a2d)
 
#if INTERRUPTS_BROKEN
typedef struct {
	timer      t;
	void       *cookie;
} isr_timer_t;

isr_timer_t isr_timer;

static int32 isr_timer_hook(timer *t);
#endif


/* Ethernet ioctrl opcodes */
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_TIMESTAMP,						/* set timestamp option; type int */
	ETHER_IOVEC								/* report if iovecs are supported */
};


/* debug flags */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define BUS_IO    0x0040		/* reads and writes across the [pci,isa,pcmcia,cardbus] bus */
#define SEQ		  0x0080		/* trasnmit & receive TCP/IP sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off for final release */

/* diagnostic debug flags - compile in here or set while running with debugger "3c589" command */
#define DEBUG_FLAGS ( ERR | INFO | WARN  )



/* pci module globals */
char						*pci_name = B_PCI_MODULE_NAME;
pci_module_info				*pci;

#if DOING_DPC
/* dpc module globals */
dpc_module_info* 			dpc_mod;
char *						dpc_name = kDevName " dpc";
dpc_thread_handle           dpct;
#endif


/* keep track of all the cards */
static char 				**names = NULL;			/* what's published */

#define MAX_CARDS			4
static dev_link_t 			*devs[MAX_CARDS] = { 0,0,0,0 };
static int              	ndevs = 0;

/* pcmcia private info */
typedef struct local_info_t {
    dev_node_t	node;
    int			stop;
	sem_id  	read_sem;	/* realease to unblock reader on card removal */
} local_info_t;


/* Driver Entry Points */
status_t init_hardware(void );
status_t init_driver(void );
void uninit_driver(void );
const char** publish_devices(void );
device_hooks *find_device(const char *name );

/*================================================================*/
/* Local Constants */


#include "prism.h"


ether_dev_info_t * gdev;



/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len);
static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length );
static int32    interrupt_hook(void *_device);                	   /* interrupt handler */

static status_t setpromisc(ether_dev_info_t * device, uint32 On);        /* set hardware to receive all packets */
static status_t domulti(ether_dev_info_t *device, uint8 *addr);		   /* add multicast address to hardware filter list */
static void     reset_device(ether_dev_info_t *device);                  /* reset the lan controller (NIC) hardware */
static void 	dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
static void 	dump_rx_desc(ether_dev_info_t *device);				   /* dump hardware receive descriptor structures */
static void 	dump_tx_desc(ether_dev_info_t *device);				   /* dump hardware transmit descriptor structures */
static status_t allocate_sems(ether_dev_info_t *device);     	   /* allocate semaphores & spinlocks */
static void     delete_sems(ether_dev_info_t *device);                /* deallocate semaphores & spinlocks */

static int		prism(int argc, char **argv);						/* debug command */
static status_t init_ring_buffers(ether_dev_info_t *device);
static void		free_ring_buffers(ether_dev_info_t *device);
static void 	load_prism_settings(ether_dev_info_t *device);


static void	prism2sta_inf_handover(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_tallies(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_scanresults(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_chinforesults(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_linkstatus(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_assocstatus(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_authreq(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_psusercnt(ether_dev_info_t *device, hfa384x_InfFrame_t *inf);


status_t pc_init_driver(void);
void pc_uninit_driver(void);
const char **pc_publish_devices(void);


static device_hooks gDeviceHooks =  {
	open_hook, 			/* -> open entry point */
	close_hook,         /* -> close entry point */
	free_hook,          /* -> free entry point */
	control_hook,       /* -> control entry point */
	read_hook,          /* -> read entry point */
	write_hook,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,               /* -> deselect entry point */
	NULL,               /* -> readv */
	writev_hook         /* -> writev */
};


inline int txfid_stackempty(prism2sta_priv_t *priv)
{
	int result;
	cpu_status former = disable_interrupts();

	acquire_spinlock(&priv->txfid_lock);
	result = (priv->txfid_top == 0);
	release_spinlock(&priv->txfid_lock);
	restore_interrupts(former);
	
	return result;
}

inline void txfid_push(prism2sta_priv_t *priv, UINT16 val)
{
	cpu_status former = disable_interrupts();

	acquire_spinlock(&priv->txfid_lock);
	if ( priv->txfid_top < PRISM2_FIDSTACKLEN_MAX ) {
		priv->txfid_stack[priv->txfid_top] = val;
		priv->txfid_top++;
	}
	release_spinlock(&priv->txfid_lock);
	restore_interrupts(former);
	return;
}

inline UINT16 txfid_pop(prism2sta_priv_t *priv)
{
	UINT16	result;
	
	cpu_status former = disable_interrupts();

	acquire_spinlock(&priv->txfid_lock);
	if ( priv->txfid_top > 0 ) {
		priv->txfid_top--;
		result = priv->txfid_stack[priv->txfid_top];
	} else {
		result = 0;
	}
	release_spinlock(&priv->txfid_lock);
	restore_interrupts(former);
	return result;
}

/*----------------------------------------------------------------
* prism2mgmt_pstr2bytestr
*
* Convert the pstr data in the WLAN message structure into an hfa384x
* byte string format.
*
* Arguments:
*	bytestr		hfa384x byte string data type
*	pstr		wlan message data
*
* Returns: 
*	Nothing
*
----------------------------------------------------------------*/

void prism2mgmt_pstr2bytestr(hfa384x_bytestr_t *bytestr, p80211pstrd_t *pstr)
{
	bytestr->len = host2hfa384x_16((UINT16)(pstr->len));
	memcpy(bytestr->data, pstr->data, pstr->len);
}




/* Driver Entry Points */


status_t init_hardware (void)
{
	return B_NO_ERROR;
}


status_t init_driver(void)
{
	status_t 		status;
	

	status = get_module(pci_name, (module_info **)&pci);
	if(status != B_NO_ERROR) {
		dprintf(kDevName ": pci module not found\n");
		goto err1;
	}

#if DOING_DPC
	status = get_module(B_DPC_MODULE_NAME,(module_info**) &dpc_mod);
	if(status != B_NO_ERROR){
		dprintf(kDevName ": DPC module not found\n");
		goto err2;
	}
	status = dpc_mod->create_dpc_thread(dpct, dpc_name, B_REAL_TIME_PRIORITY);
	if (status != B_NO_ERROR) {
		dprintf(kDevName ": create_dpc_tread failed\n");
		goto err3;
	}
#endif

	if ((status = pc_init_driver()) != B_OK) {
		goto err3;
	};

	gdev = NULL;
	add_debugger_command (kDevName, prism, "Prism 802.11 Wireless");

	return B_NO_ERROR;
	
err3:
#if DOING_DPC
	put_module(B_DPC_MODULE_NAME);
#endif
err2:
	put_module(pci_name);
err1:
	dprintf("pc_init_card failed %lx\n", status);
	return status;
}

void uninit_driver(void)
{
	remove_debugger_command (kDevName, prism);

#if DOING_DPC
	put_module(B_DPC_MODULE_NAME);	

	if (B_NO_ERROR != dpc_mod->delete_dpc_thread(dpct)) {
		dprintf(kDevName ": delete_dpc_thread failed\n");
	}
#endif

	pc_uninit_driver();
}


device_hooks *find_device(const char *name)
{
	return &gDeviceHooks;
}

void rc_error(uint16 err, uint16 line) {
	dprintf(kDevName ": RC err %d line %d\n",err, line);
}

static status_t open_hook(const char *name, uint32 flags, void **cookie) {

	int 				nd;
	ether_dev_info_t 	*device;
	hfa384x_t *hw;


	/* allocate memory for each piece of hardware */
	if ((device = (ether_dev_info_t *)memalign( 8, sizeof(ether_dev_info_t) )) == NULL) {
		dprintf (kDevName ": memalign(%ld) out of memory\n", sizeof(ether_dev_info_t));
		goto err0;
	}

	for (nd = 0; nd < MAX_CARDS; nd++) {
		dprintf("match[%d] %s %s \n", nd, name, devs[nd]->dev? devs[nd]->dev->dev_name : "Null");
		if ((devs[nd] != NULL) &&
		 (devs[nd]->dev != NULL) &&
		 (strcmp(devs[nd]->dev->dev_name, name) == 0))
        	break;
 	}
	if (nd == MAX_CARDS) {
		dprintf(kDevName ": %d device limit exceeded\n", MAX_CARDS);
		goto err1;
	}

	if((devs[nd]->state & DEV_PRESENT) == 0) {
		dprintf("%s: tried to open device that is gone\n", devs[nd]->dev->dev_name);
		goto err1;
	}
 
 	*cookie = device;
	memset(device, 0, sizeof(ether_dev_info_t));
	device->pcmcia_info = devs[nd];
	device->debug = DEBUG_FLAGS;
	hw = &device->hw;
	device->wlandev.netdev = device;
	
	dprintf("%s opened: irq %d   iobase 0x%04x \n",
		devs[nd]->dev->dev_name,devs[nd]->irq.AssignedIRQ, devs[nd]->io.BasePort1);
		
	if ((devs[nd]->irq.AssignedIRQ == 0) ||	(devs[nd]->irq.AssignedIRQ == 0xff)) {
		dprintf(kDevName ": IRQ assignment ERROR - check BIOS and device IRQ assignements\n");
		goto err1;
	}
	
	device->irq = devs[nd]->irq.AssignedIRQ;
			
	if (allocate_sems(device) != B_OK)
		goto err1;	
		
	gdev = device; 

	if (init_ring_buffers(device) != B_OK)
		goto err2;	
		
	device->wlandev.state = WLAN_DEVICE_CLOSED;
	device->wlandev.ethconv = WLAN_ETHCONV_RFC1042;
	device->wlandev.macmode = WLAN_MACMODE_NONE;

	hfa384x_inithw(hw, devs[nd]->irq.AssignedIRQ,devs[nd]->io.BasePort1);


	if ( prism2sta_initmac(device) != 0 ) {
		WLAN_LOG_ERROR0("MAC Initialization failed.\n");
		goto err3;
	}

		
	if (init_mempool() != B_OK)
		goto err3;

	device->roam_mode = 2; // default setting
	
	load_prism_settings(device);


	{
		uint16 result;
		p80211msg_dot11req_associate_t msg;

		uchar bytestring[MAX_STRING_SIZE];
		int16 len, residual;
		
		result = hfa384x_drvr_setconfig16( &device->hw, HFA384x_RID_CNFROAMINGMODE, &device->roam_mode);
		dprintf("set roam mode %d result = %x\n", device->roam_mode, result);
		
		/* configure the network name */
		len = strlen(device->network_name);
		*(uint16 *) bytestring = B_HOST_TO_LENDIAN_INT16(len);
		memcpy(&bytestring[2],device->network_name, len);
		residual = HFA384x_RID_CNFDESIREDSSID - (len + 2);

		if (residual > 0) {
			memset(&bytestring[2+len], 0, residual);
		}


		result = hfa384x_drvr_setconfig( &device->hw, HFA384x_RID_CNFDESIREDSSID,
			bytestring, HFA384x_RID_CNFDESIREDSSID_LEN);
		dprintf("load_prism_settings: Set Network Name %s result = %x\n",device->network_name,result);

		/* set key0 for wep encryption */
		if  (device->wep_invoked) {
			char *	p;
			unsigned char key[HFA384x_RID_CNFWEPDEFAULTKEY_LEN];  /*HFA384x_RID_CNFWEPDEFAULTKEY_LEN = 6! */
			char	temp[3];
			int  	j, k=0;
			unsigned short 	wordbuf[50];
			
			memset(key, 0, sizeof(key) );
			temp[2] = 0;
			
			/* set default key  */
			memset(wordbuf, 0, sizeof(wordbuf) );
//				set_value = 0;	/* to do: should range from 0-3 as per kernel settings file DefaultKeyIndex */
//				prism2mgmt_p80211int2prism2int(wordbuf, &set_value);
			result = hfa384x_drvr_setconfig16( hw, HFA384x_RID_CNFWEPDEFAULTKEYID,wordbuf);
			if (result != 0) dprintf (kDevName ": setting default key index failed %x \n", result);

			result = hfa384x_drvr_getconfig16( hw, HFA384x_RID_CNFWEPDEFAULTKEYID,wordbuf);
			if (result != 0) dprintf (kDevName ": getting default key index failed %x \n", result);
//dprintf("got default key id %4.4x %4.4x\n", wordbuf[0],wordbuf[1]);

			/* set privacy invoked true */
			memset(wordbuf, 0, sizeof(wordbuf) );
//				set_value =  HFA384x_WEPFLAGS_PRIVINVOKED;
//				prism2mgmt_p80211int2prism2int(wordbuf, &set_value);
			wordbuf[0] = HFA384x_WEPFLAGS_PRIVINVOKED;
			result = hfa384x_drvr_setconfig16( hw, HFA384x_RID_CNFWEPFLAGS,wordbuf);
			if (result != 0) dprintf (kDevName ": setting wep encryption on failed %x \n", result);

			memset(wordbuf, 0, sizeof(wordbuf) );
			result = hfa384x_drvr_getconfig16( hw, HFA384x_RID_CNFWEPFLAGS,wordbuf);
			if (result != 0) dprintf (kDevName ": setting wep encryption on failed %x \n", result);
//dprintf("got wep invoked value %4.4x %4.4x\n", wordbuf[0],wordbuf[1]);
			
			/* set encryption key */
			p = device->key;
			/* convert ascii key to hex  */
			for (j=(min(10, strlen(device->key))); j>0; j=j-2) {
				temp[0] = *p++; temp[1]=*p++;
				key[k++] = strtol(temp, NULL, 16);
			}

			key[HFA384x_RID_CNFWEPDEFAULTKEY_LEN - 1] = 0x00;
			result = hfa384x_drvr_setconfig( hw, HFA384x_RID_CNFWEPDEFAULTKEY0,
				key, HFA384x_RID_CNFWEPDEFAULTKEY_LEN);


			if (result == 0) {
				dprintf("WEP on key0 = %2.2x %2.2x %2.2x %2.2x %2.2x\n",
					key[0],key[1],key[2],key[3],key[4]);
			} else {
				dprintf("WEP was on but setting key failed result=%x!!!!\n",result);
			}

#if 0
			{ uchar junk[40];
			memset(junk, 0, sizeof(junk) );
			result = hfa384x_drvr_getconfig( hw, HFA384x_RID_CNFWEPDEFAULTKEY0,junk, HFA384x_RID_CNFWEPDEFAULTKEY_LEN);

			dprintf("got wep key value %2.2x %2.2x %2.2x %2.2x %2.2x\n", junk[0],junk[1],junk[2],junk[3],junk[4]);
			}
#endif

		}

		device->wlandev.macmode = WLAN_MACMODE_ESS_STA; // for the moment force this setting!!!!


#if INTERRUPTS_BROKEN
#undef add_timer /* name clash with pcmcia/k_compat.h header */
{	status_t err;

		dprintf("ITERRRUPTS ARE BROKEN ON THIS MACHINE POLLING INSTEAD :(\n");
		isr_timer.cookie = device;
		err = add_timer(&isr_timer.t, isr_timer_hook, 1000, B_PERIODIC_TIMER);
		if(err != B_NO_ERROR)
			goto err3;
}
#else
		install_io_interrupt_handler( devs[nd]->irq.AssignedIRQ, interrupt_hook, *cookie, 0 );
#endif

		result = prism2mgmt_associate(device, &msg);
		dprintf("associate request returns %x\n",result);

	}

  	return B_OK;           

	err3:	
		free_ring_buffers(device);
	err2:
		delete_sems(device);
	err1:
		free(device);	
	err0:
		return B_ERROR;
}


static status_t close_hook(void *_device) {
	ether_dev_info_t *device = (ether_dev_info_t *) _device;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ": close_hook\n");
	
//	if (rc = hcf_disable(&device->ifb, 0)) rc_error(rc, __LINE__);
//	if (rc = hcf_action(&device->ifb, HCF_ACT_INT_OFF)) rc_error(rc, __LINE__);

	dprintf(kDevName ": config_device CLOSE - card out?\n");

//	hcf_disconnect(&device->ifb);

	delete_sems(device);

//	open_ref_count--;
		
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	ether_dev_info_t *device = (ether_dev_info_t *) cookie;
	
	dprintf(kDevName ": free_hook %p irq=%ld\n",  device, device->hw.irq);

#if INTERRUPTS_BROKEN
	cancel_timer(&isr_timer.t);
#else
	remove_io_interrupt_handler( device->irq, interrupt_hook, cookie );
#endif
	
	free_ring_buffers(device);

	gdev = NULL;
	
	uninit_mempool();
		
	free(device);

	return 0;
}


/* Standard driver control function */
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len)
{
	ether_dev_info_t *device = (ether_dev_info_t *) cookie;

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
/*			ETHER_DEBUG(device, INFO, kDevName ": control %x ether_getaddr\n",msg); */
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->mac_addr[i];
			}
			return B_NO_ERROR;
		}
		case ETHER_INIT:
			ETHER_DEBUG(device, INFO, kDevName ": control %x init\n", (int) msg);
			return B_NO_ERROR;
			
		case ETHER_GETFRAMESIZE:
			ETHER_DEBUG(device, INFO, kDevName ": control get_framesize %d\n", MAX_FRAME_SIZE);
			*(uint32 *)buf = MAX_FRAME_SIZE;
			return B_NO_ERROR;
			
		case ETHER_ADDMULTI:
			ETHER_DEBUG(device, INFO, kDevName ": control %x add multi\n", (int) msg);
			return (domulti(device, (unsigned char *)buf));
		
		case ETHER_SETPROMISC:
			ETHER_DEBUG(device, INFO, kDevName ": control %x set promiscuous\n", (int) msg);
			return setpromisc(device, *(uint32 *)buf);
		
		case ETHER_NONBLOCK:
//			ETHER_DEBUG(device, INFO, kDevName ": control %x blocking\n",msg);

			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_NO_ERROR;

		case ETHER_TIMESTAMP:
			ETHER_DEBUG(device, INFO, kDevName ": control %x ether time stamp\n", (int) msg);
			return B_ERROR;
	
		case ETHER_IOVEC:
			ETHER_DEBUG(device, INFO, kDevName ": control %x iovec\n", (int) msg);
			return B_ERROR;

		default:
			ETHER_DEBUG(device, ERR, kDevName ": unknown iocontrol %x\n", (int) msg);
			return B_ERROR;
	}
}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	ether_dev_info_t  *device = (ether_dev_info_t *) _device;
	status_t    status;
	uint32  buflen;
	buflen = *len;
	*len = 0;
		
	/* Block until data is available */
	if ((status = acquire_sem_etc(device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0)) != B_NO_ERROR) {
/*		dprintf(kDevName ": read_hook %x on acquire ilock\n",status); */
		if (status == B_WOULD_BLOCK)
			return B_OK;
		else
			return status;
	}
	/* Prevent reentrant read */
	if (atomic_or(&device->readLock, 1 )) {
		release_sem_etc( device->ilock, 1, 0 );
		dprintf(kDevName ": read_hook, reentrant read!! Yike\n");
		return B_NO_ERROR;
	}

/* spin lock */	
	device->rx_acked = (device->rx_acked + 1) & (RX_BUFFERS-1);
/* spin unlock */
if (device->rx_len[device->rx_acked] > 1514) {
//	dprintf("len err %d  truncating to 1514\n", device->rx_len[device->rx_acked]);
	memcpy(buf, device->rx_buf[device->rx_acked], 1514);
	//dump_packet("rx", buf, device->rx_len[device->rx_acked]);
	*len = 1514;
} else  if (device->rx_len[device->rx_acked] < 10) {
//	dprintf("len err %d  too small!\n", device->rx_len[device->rx_acked]);
	memcpy(buf, device->rx_buf[device->rx_acked],  device->rx_len[device->rx_acked]);
	//dump_packet("rx", buf, device->rx_len[device->rx_acked]);
	*len =  device->rx_len[device->rx_acked];
} else {
	memcpy(buf, device->rx_buf[device->rx_acked], device->rx_len[device->rx_acked]);
	*len = device->rx_len[device->rx_acked];
}	

	if (device->debug & RX) {
		if (*len)
			dump_packet("rx", buf, *len);
		else
			dprintf("read_hook:exit 0 len\n");
	}
	/* set seq on with the debugger command and  ping to see if frames are coming in order */
	if (device->debug & SEQ) {
		unsigned short  *seq = (unsigned short *) buf;
		dprintf(" R%4.4x ", seq[20]);  /* sequence number */
	}
	

	device->readLock = 0; /* release reantrant lock */
	return B_OK;
}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len)
{
	ether_dev_info_t  *device = (ether_dev_info_t *) _device;
	UINT16				proto;
	int					result;
	int16        		frame_size;
	status_t     	 	status;
	wlan_pb_t			*pb;
	hfa384x_tx_frame_t	txdesc;
	UINT8				dscp;
	UINT16				macq = 0;
	UINT16				fid;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ":write_hook %x len %x\n", (int) buf, (int) *len);
 
	if( *len > MAX_FRAME_SIZE ) {
		ETHER_DEBUG(device, ERR, kDevName ": write %d > MAX_FRAME_SIZE tooo long\n",(int) *len);
		*len = MAX_FRAME_SIZE;
	}
	frame_size = *len;

	if( (status = acquire_sem_etc( device->write_lock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR ) {
		return status;	
	}
//retry:
	if( (status = fc_wait( &device->olock, B_INFINITE_TIMEOUT )) != B_NO_ERROR ) {
		*len = 0;
		dprintf(kDevName ": fc_wait error %lx\n on tx",  status); 
		goto err;
	}

	/* Prevent reentrant write */
	if (atomic_or( &device->writeLock, 1 )) {
		dprintf(kDevName ": write_hook: reentrant write\n"); /* this should never happen */
		fc_signal(&device->olock, 1, B_DO_NOT_RESCHEDULE);
		*len = 0;
		status = B_ERROR;
		goto err;
	}

	//dprintf(kDevName ": write_hook before pb_alloc()\n");
	/* OK, now we setup the ether to 802.11 conversion */
	pb = p80211pb_alloc();
	if ( pb == NULL ) {
		dprintf(kDevName "write_hook() failed No TX buffers\n");
		goto err1;
	}

	pb->ethhostbuf = (void *) buf;
	pb->ethfree = p80211pb_freeskb;
	pb->ethbuf = (uint8 *) buf;
	pb->ethbuflen = *len > 1514 ? 1514 : *len;
	pb->ethfrmlen = pb->ethbuflen;
	pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;

	if ( p80211pb_ether_to_p80211(&device->wlandev, device->wlandev.ethconv, pb) != 0 ) {
		/* convert failed */
		WLAN_LOG_DEBUG1("ether_to_80211(%d) failed.\n", device->wlandev.ethconv);
		goto err2;
	}
//		wlandev->linux_stats.tx_packets++;

	/* Build Tx frame structure */
	/* Set up the control field */
	memset(&txdesc, 0, sizeof(txdesc));

/* Tx complete and Tx exception disable per dleach.  Might be causing buf depletion */		

	txdesc.tx_control = 
		HFA384x_TX_MACPORT_SET(0) | HFA384x_TX_STRUCTYPE_SET(1) | 
		HFA384x_TX_TXEX_SET(1) | HFA384x_TX_TXOK_SET(1);	

	txdesc.tx_control = host2hfa384x_16(txdesc.tx_control);

	/* Set up the 802.11 header */
	if ( device->wep_invoked) {
		pb->p80211_hdr->a3.fc |= host2ieee16(WLAN_SET_FC_ISWEP(1));
	}
	memcpy(&(txdesc.frame_control), pb->p80211_hdr, WLAN_HDR_A3_LEN);

	/* Set the len, complicated because of pieces in pb */
	txdesc.data_len = pb->p80211buflen - WLAN_HDR_A3_LEN; /* llc+snap? */
	txdesc.data_len += pb->p80211_payloadlen;
	txdesc.data_len = host2hfa384x_16(txdesc.data_len);

	/* Allocate FID */
	fid = txfid_pop(&device->priv);
	
	if (fid == 0) {
		dprintf(kDevName ": no TX on_card buffers\n");
		goto err2;
	}

	/* Copy descriptor part to FID */

#if 0
dprintf("write_hook : copytobap txdesc=%x bytes=%d fid=%d\n", (unsigned int) &txdesc, (int) sizeof(txdesc),(int) fid);
{ int j; for (j=0;j<sizeof(txdesc);j++) {
	dprintf("%2.2x%c", *(unsigned char *) ((int) &txdesc + j), ((j&0xf)==0xf)?'\n':' ');
	}
}
dprintf("\n");
#endif

	result = hfa384x_copy_to_bap(&device->hw, 
			device->hw.bap, fid, 0, &txdesc, sizeof(txdesc));
	if ( result ) {
		WLAN_LOG_DEBUG3("copy_to_bap(%04x, 0, %d) failed, result=0x%x\n", 
			fid, (unsigned int) sizeof(txdesc),result);
		goto err2;
	}

	/* Copy 802.11 data to FID */
	if ( pb->p80211buflen > WLAN_HDR_A3_LEN ) { /* copy llc+snap hdr */
		
	
#if 0
dprintf("write_hook: copytobap 802 data %x bytes=%x offset=%d\n",
		pb->p80211buf + WLAN_HDR_A3_LEN,
		pb->p80211buflen-WLAN_HDR_A3_LEN,
		(unsigned int) sizeof(txdesc));
{ int j; for (j=0;j<pb->p80211buflen - WLAN_HDR_A3_LEN;j++) {
	dprintf("%2.2x%c", *(unsigned char *) ((int) pb->p80211buf+WLAN_HDR_A3_LEN + j), ((j&0xf)==0xf)?'\n':' ');
	}
}
dprintf("\n");
	
#endif
	
		result = hfa384x_copy_to_bap( &device->hw, device->hw.bap, fid, sizeof(txdesc),
				(void *)((int) (pb->p80211buf) + WLAN_HDR_A3_LEN),
				pb->p80211buflen - WLAN_HDR_A3_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG4(
				"copy_to_bap(%04x, %d, %d) failed, result=0x%x\n", 
				fid, (unsigned int) sizeof(txdesc),pb->p80211buflen - WLAN_HDR_A3_LEN,result);
			goto err2;
		}
	}
#if 0
dprintf("write_hook : copytobap payload %x  %x bytes offset=%x\n",
		(unsigned int) pb->p80211_payload, pb->p80211_payloadlen,
			(int) sizeof(txdesc) + (int) pb->p80211buflen - (int) WLAN_HDR_A3_LEN );
{ int j; for (j=0;j<pb->p80211_payloadlen;j++) {
	dprintf("%2.2x%c", *(unsigned char *) ((int)  pb->p80211_payload+ j), ((j&0xf)==0xf)?'\n':' ');
	}
}
dprintf("\n");
#endif
	

	result = hfa384x_copy_to_bap( &device->hw, 
		     device->hw.bap, 
		     fid, 
		     sizeof(txdesc) + pb->p80211buflen - WLAN_HDR_A3_LEN, 
		     pb->p80211_payload, 
		     pb->p80211_payloadlen);
	
	if ( result ) {
		WLAN_LOG_DEBUG4(
			"copy_to_bap(%04x, %d, %d) failed, result=0x%x\n", 
			fid, (unsigned int) sizeof(txdesc) + pb->p80211buflen - WLAN_HDR_A3_LEN, 
	 		pb->p80211_payloadlen,result);
		goto err2;
	}

	/*  Determine if we're doing MM queuing */
	if ( device->priv.qos_enable && device->priv.mm_mods ) {
		/* Find protocol field */
		if ( pb->p80211buflen == WLAN_HDR_A3_LEN ) {
			proto = ntohs(*((UINT16*)(pb->p80211_payload+6)));
		} else {
			proto = ntohs(*((UINT16*)(pb->p80211buf+24+6)));
		}
		/* Is it IP? */
		if (proto == 0x0800 ) {
			/* Get the DSCP */
			if ( pb->p80211buflen == WLAN_HDR_A3_LEN ) {
				/* LLC+SNAP is in payload buf */
				dscp = pb->p80211_payload[3+5+1];
			} else {
				/* LLC+SNAP is in header buf */
				dscp = pb->p80211_payload[1];
			}
			dscp &= ~(BIT0 | BIT1); /* RFC bit 6 and 7 */
			dscp >>= 2;
			macq = device->priv.qos_dscpmap[dscp];
		} else {
			macq = 1;	/* best effort */
		}
	}

	/* Issue Tx command */
	result = hfa384x_cmd_transmit(&device->hw, HFA384x_TXCMD_RECL, macq, fid);
		
	if ( result != 0 ) {
		WLAN_LOG_DEBUG2("cmd_tx(%04x) failed, result=%d", fid, result);
		goto err2;
	}
		
	if (device->debug & TX) {
		dump_packet("tx", (unsigned char *) buf, frame_size);
	}

	/* Free the pb */
	pb->ethhostbuf = NULL;
	p80211pb_free(pb);

	/* Another write may now take place */
	release_sem(device->write_lock);
	device->writeLock = 0;

	return B_OK;

err2:
	p80211pb_free(pb);
err1:
	device->writeLock = 0;
	status = B_ERROR;
err:
	release_sem(device->write_lock);
	return status;
}

static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length ) {

	return B_ERROR;
};

static long
sem_count(sem_id sem)
{
	long count;

	get_sem_count(sem, &count);
	return (count);
}


#if INTERRUPTS_BROKEN
static int32
isr_timer_hook(timer *t)
{
	isr_timer_t *isr = (isr_timer_t *)t;
	interrupt_hook(isr->cookie);
	return B_HANDLED_INTERRUPT;
}
#endif


/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{

	ether_dev_info_t *device = (ether_dev_info_t *) _device;
	hfa384x_t		*hw = &device->hw;

	int32			reg;
	int			ev_read = 0;

	int32 handled = B_UNHANDLED_INTERRUPT;
	
	
	//dprintf(kDevName ": ISR\n");
	
		
	/* Check swsupport reg magic # for card presence */
	reg = wlan_inw_le16_to_cpu(HFA384x_SWSUPPORT0(hw->iobase));
	if ( reg != PRISM2STA_MAGIC) {
		WLAN_LOG_DEBUG0("no magic.  Card removed?.\n");
		return handled;
	}

	/* Set the BAP context */
	hw->bap = HFA384x_BAP_INT;

	/* read the EvStat register for interrupt enabled events */
	reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));

	if (device->debug & INTERRUPT) { dprintf("prism ISR %lx\n", reg); }
	if (reg) {
		handled = B_HANDLED_INTERRUPT;
	} else {
		dprintf("ISR - Not our interrupt?!?\n");
		return B_UNHANDLED_INTERRUPT;
	}

	do {

		/* Handle the events */
		if ( HFA384x_EVSTAT_ISINFDROP(reg) ){
			prism2sta_int_infdrop(device);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_INFDROP_SET(1), 
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISINFO(reg) ){
			prism2sta_int_info(device);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_INFO_SET(1),
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISTXEXC(reg) ){
			prism2sta_int_txexc(device);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_TXEXC_SET(1),
				HFA384x_EVACK(hw->iobase));
			fc_signal( &device->olock, 1, B_DO_NOT_RESCHEDULE );

		}
	
		if ( HFA384x_EVSTAT_ISTX(reg) ){
			int result;
			uint16 status;
			uint16 fid;
			fid = wlan_inw_le16_to_cpu(HFA384x_TXCOMPLFID(hw->iobase));
			result =hfa384x_copy_from_bap(hw, hw->bap, fid, 0, &status, sizeof(status));
			if ( result ) {
				WLAN_LOG_DEBUG3("copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
					fid, (unsigned int) sizeof(status), result);
			} else {
//				WLAN_LOG_DEBUG2("Tx Complete, status=0x%04x fid=%d\n", hfa384x2host_16(status), fid);
			
				if( fc_signal( &device->olock, 1, B_DO_NOT_RESCHEDULE ) ) {
					handled = B_INVOKE_SCHEDULER;
				} else {
					handled = B_HANDLED_INTERRUPT;
				}
				wlan_outw_cpu_to_le16(HFA384x_EVACK_TX_SET(1),
					HFA384x_EVACK(hw->iobase));
			}
		}
	
		if ( HFA384x_EVSTAT_ISRX(reg) ) {
			prism2sta_int_rx(device);
			handled = B_INVOKE_SCHEDULER;
			wlan_outw_cpu_to_le16(HFA384x_EVACK_RX_SET(1), HFA384x_EVACK(hw->iobase));
		}
		
		if ( HFA384x_EVSTAT_ISALLOC(reg) ) {
			prism2sta_int_alloc(device);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_ALLOC_SET(1),
				HFA384x_EVACK(hw->iobase));
		}

		if ( HFA384x_EVSTAT_ISDTIM(reg) ) {
			prism2sta_int_dtim(device);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_DTIM_SET(1),
				HFA384x_EVACK(hw->iobase));
		}

		/* allow the evstat to be updated after the evack */
		udelay(20);

		/* Check swsupport reg magic # for card presence */
		reg = wlan_inw_le16_to_cpu(HFA384x_SWSUPPORT0(hw->iobase));
		if ( reg != PRISM2STA_MAGIC) {
			WLAN_LOG_DEBUG0("no magic.  Card removed?\n");
			return handled;
		}

		/* read the EvStat register for interrupt enabled events */
		reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
		ev_read++;

	} while ( (HFA384x_EVSTAT_ISINFDROP(reg) || HFA384x_EVSTAT_ISINFO(reg) ||
		HFA384x_EVSTAT_ISTXEXC(reg) || HFA384x_EVSTAT_ISTX(reg) ||
		HFA384x_EVSTAT_ISRX(reg) || HFA384x_EVSTAT_ISALLOC(reg) ||
		HFA384x_EVSTAT_ISDTIM(reg)) &&
		ev_read < 10);

	/* Clear the BAP context */
	hw->bap = HFA384x_BAP_PROC;
	
	return handled;

}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))



/* set hardware so all packets are received. */
static status_t setpromisc(ether_dev_info_t * device, uint32 On) {
	uint32 result;
	uint16 promisc = On ? 1:0;
		
	if ((result = hfa384x_drvr_setconfig16( &device->hw,HFA384x_RID_PROMISCMODE, &promisc))) {
		dprintf("set promisc failed %lx\n", result);
	} else {
		dprintf("Promiscuous Rx %s\n", promisc ? "on":"off");
	}
	return(B_OK);
}

static status_t domulti(ether_dev_info_t *device, uint8 *addr) {
	
	ETHER_DEBUG(device, FUNCTION,kDevName ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
					addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	
	return (B_NO_ERROR);
}

static void reset_device(ether_dev_info_t *device) {
	ETHER_DEBUG(device, FUNCTION,kDevName ": reset_device reset the NIC hardware\n"); 
};

static void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d \n", msg, (int) buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
	dprintf("\n");
}

static void dump_rx_desc(ether_dev_info_t *device) {
#if 0
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("rx desc %8.8x \n", device->rx_desc);
	for (j=0; j < RX_BUFFERS; j++ ) {
		kprintf("rx_desc[%2.2d]=...\n", j);
	}
#endif
}

static void dump_tx_desc(ether_dev_info_t *device) {
#if 0
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("tx desc %8.8x \n", device->tx_desc);
	
	for (j=0; j < TX_BUFFERS; j++ ) {
		kprintf("tx_desc[%2.2d]...\n",j);
	}
#endif
}


/*
* Copyright (C) 1999 AbsoluteValue Systems, Inc.  All Rights Reserved.
* --------------------------------------------------------------------
*
* linux-wlan
*
*   The contents of this file are subject to the Mozilla Public
*   License Version 1.1 (the "License"); you may not use this file
*   except in compliance with the License. You may obtain a copy of
*   the License at http://www.mozilla.org/MPL/
*
*   Software distributed under the License is distributed on an "AS
*   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*   implied. See the License for the specific language governing
*   rights and limitations under the License.
*
*   Alternatively, the contents of this file may be used under the
*   terms of the GNU Public License version 2 (the "GPL"), in which
*   case the provisions of the GPL are applicable instead of the
*   above.  If you wish to allow the use of your version of this file
*   only under the terms of the GPL and not to allow others to use
*   your version of this file under the MPL, indicate your decision
*   by deleting the provisions above and replace them with the notice
*   and other provisions required by the GPL.  If you do not delete
*   the provisions above, a recipient may use your version of this
*   file under either the MPL or the GPL.
*
* --------------------------------------------------------------------
*
* Inquiries regarding the linux-wlan Open Source project can be
* made directly to:
*
* AbsoluteValue Systems Inc.
* info@linux-wlan.com
* http://www.linux-wlan.com
*
* --------------------------------------------------------------------
*
* Portions of the development of this software were funded by 
* Intersil Corporation as part of PRISM(R) chipset product development.
*
* --------------------------------------------------------------------
*/




/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "prism arg...",
   for example "prism R" to enable a received packet trace.
*/
static int
prism(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: prism { Globals | Interrupts | Number_rx_sequence | Stats | Rx_trace | Tx_trace }\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	if (gdev == NULL) {
		kprintf(kDevName ": not open\n");
		return 0;
	}
	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'G':
		case 'g':
			{
				int32 semcount;
				get_sem_count(gdev->ilock, &semcount);  
				kprintf(kDevName ": RX semcount=%ld, rcvd=%ld, acked=%ld total_bufs=%d readLock=%ld\n",
						semcount, gdev->rx_received , gdev->rx_acked, RX_BUFFERS, gdev->readLock);
				kprintf(kDevName ": TX fc_count=%ld, \n", gdev->olock.count );
#if 0
				{ int x;
				for (x=0; x < RX_BUFFERS; x++) {
					kprintf("rx_len[%2.2d] = %6.6x\n", x, gdev->rx_len[x]); 
				}
				for (x=0; x < RX_BUFFERS; x++) {
					dump_packet("\nrx", gdev->rx_buf[x], 48);
				}				}
#endif
			}
			break; 
		case 'N':
		case 'n':
			gdev->debug ^= SEQ;
			if (gdev->debug & SEQ) 
				kprintf("Sequence numbers packet trace Enabled\n");
			else 			
				kprintf("Sequence numbers packet trace Disabled\n");
			break; 
		case 'R':
		case 'r':
			gdev->debug ^= RX;
			if (gdev->debug & RX) 
				kprintf("Receive packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break;
		case 'T':
		case 't':
			gdev->debug ^= TX;
			if (gdev->debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'S':
		case 's':
			kprintf(kDevName " statistics - to do\n");			
			break; 
		default:
			kprintf("%s",usage);
			return 0;
		}
	}
	
	return 0;
}
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t init_ring_buffers(ether_dev_info_t *device)
{
	uint32 	size;
	uint16 i;

	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(device, ERR, kDevName " create rx buffer area failed %x \n", (int) device->rx_buf_area);
		return device->rx_buf_area;
	}

	/* init rx buffer */
	for ( i = 0; i < RX_BUFFERS; i++) {
		device->rx_buf[i] = device->rx_buf[0] + (i * BUFFER_SIZE);
//		dprintf("rx buf[%d]=%lx\n",i,device->rx_buf[i]);
	}

	/* initialize frame indexes */
	device->rx_received = device->rx_acked = 0;

	return B_OK;
}

static void free_ring_buffers(ether_dev_info_t *device) {

		delete_area(device->rx_buf_area);
}

static void load_prism_settings(ether_dev_info_t *device) {

	void *handle;
	const driver_settings *settings;
	int entries = 0;
	int i = 0;
	int j = 0;
	
	handle = load_driver_settings(kDevName);
	settings = get_driver_settings(handle);
	if (settings) {
		entries = settings->parameter_count;
		/* Basically, if we exceed the max. number of cards, we don't care
		   about any of the other entries in the settings file */
		for (i=0; i<entries; i++) {
			driver_parameter *parameter = settings->parameters + i;

			dprintf(kDevName ": loading kernel settings for device (%d)\n",  i);

			for (j=0; j < parameter->parameter_count; j++) {
				driver_parameter *temp_param = parameter->parameters + j;
				/* Wired Equivalent Privacy (WEP) encryption key */
				if ((strcmp(temp_param->name, "key") == 0) || (strcmp(temp_param->name, "key0") == 0) )
				{
					if (strlen(temp_param->values[0]) > MAX_KEY_SIZE) {
						dprintf("key toooo long %s\n",temp_param->values[0]);
					} else {
						device->wep_invoked = true;
						strcpy( device->key, temp_param->values[0]);
					}
				} 
				if (strcmp(temp_param->name, "default_key_index") == 0) 
				{
					if (strlen(temp_param->values[0]) > 1) {
						dprintf(kDevName ": default_key out of range\n");
					} else {
						device->default_key_index = strtoll(temp_param->values[0], NULL, 0);
						if (device->default_key_index > 3) {
							dprintf("default_key_index %d invalid - setting to zero\n",
								device->default_key_index);
							device->default_key_index = 0;
						}
					}
				} 
				if (strcmp(temp_param->name, "network_name") == 0) /* network name */
				{
					if (strlen(temp_param->values[0]) > MAX_STRING_SIZE) {
						dprintf("network name toooo long %s\n", temp_param->values[0]);
					} else {
						strcpy( device->network_name, temp_param->values[0]);
					}
				} 

				if (strcmp(temp_param->name, "roam_mode") == 0) 
				{
					if (strlen(temp_param->values[0]) > MAX_STRING_SIZE) {
						dprintf(kDevName ": roam_mode tooo long\n");
					} else {
						device->roam_mode = strtoll(temp_param->values[0], NULL, 0);
					}
				} 
#if 0
				if (strcmp(temp_param->name, "port_type") == 0) 
				{
					device->port_type = strtoll(temp_param->values[0], NULL, 10);
					dprintf(kDevName ": port_type=%d\n", (int) device->port_type);
				} 
				if (strcmp(temp_param->name, "mode") == 0) 
				{
					device->channel = strtoll(temp_param->values[0], NULL, 10);
					dprintf(kDevName ": channel %d \n", (int) device->channel);
				} 
#endif
			}
		}
	} else {
		dprintf("ERROR: boot/home/config/settings/kernel/driver/"kDevName " settings file not found\n");
	}
	unload_driver_settings(handle);
}


/*
 * Allocate and initialize semaphores and spinlocks.
 */
static status_t allocate_sems(ether_dev_info_t *device) {
	status_t result;

	/* Setup Semaphores */
	if ((device->ilock = create_sem(0, kDevName " rx")) < 0) {
		dprintf(kDevName " create rx sem failed %x \n", (int) device->ilock);
		return (device->ilock);
	}

	if ((device->write_lock = create_sem(1, kDevName " write_lock")) < 0) {
		dprintf(kDevName " create write_lock sem failed %x \n", (int) device->ilock);
		delete_sem(device->ilock);
		return (device->write_lock);
	}


	((local_info_t *)device->pcmcia_info->priv)->read_sem = device->ilock;
	
	/* Intialize tx fc_lock to zero.
	   The semaphore is used to block the write_hook until free space
	   in on the card is available.
	 */
	if((result = create_fc_lock( &device->olock, TX_BUFFERS-1, kDevName " tx" )) < 0 ) {
		dprintf(kDevName " create tx fc_lock failed %lx \n",  result);
		delete_sem(device->ilock);
		delete_sem(device->write_lock);
		return (result);
	}
	
	device->readLock = device->writeLock = 0;

	device->blockFlag = 0;
		
	return (B_OK);
}

static void delete_sems(ether_dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->write_lock);
		delete_fc_lock(&device->olock);
}

/*** PCMCIA STUFF ***/

/*====================================================================*/

/*
   The event() function is this driver's Card Services event handler.
   It will be called by Card Services when an appropriate card status
   event is received.  The config() and release() entry points are
   used to configure or release a socket, in response to card
   insertion and ejection events.  They are invoked from the prism
   event handler. 
*/

static void prism_config(dev_link_t *link);
static void pcmcia_release(u_long arg);
static int pcmcia_event(event_t event, int priority,
		       event_callback_args_t *args);

/*
   The attach() and detach() entry points are used to create and destroy
   "instances" of the driver, where each instance represents everything
   needed to manage one actual PCMCIA card.
*/

static dev_link_t *pcmcia_attach(void);
static void pcmcia_detach(dev_link_t *);

/*
   The dev_info variable is the "key" that is used to match up this
   device driver with appropriate cards, through the card configuration
   database.
*/
static dev_info_t dev_info = kDevName;

/*
   An array of "instances" of the prism device.  Each actual PCMCIA
   card corresponds to one device instance, and is described by one
   dev_link_t structure (defined in ds.h).
*/


/*
   A dev_link_t structure has fields for most things that are needed
   to keep track of a socket, but there will usually be some device
   specific information that also needs to be kept track of.  The
   'priv' pointer in a dev_link_t structure can be used to point to
   a device-specific private data structure, like this.

   A driver needs to provide a dev_node_t structure for each device
   on a card.  In some cases, there is only one device per card (for
   example, ethernet cards, modems).  In other cases, there may be
   many actual or logical devices (SCSI adapters, memory cards with
   multiple partitions).  The dev_node_t structures need to be kept
   in a linked list starting at the 'dev' field of a dev_link_t
   structure.  We allocate them in the card's private data structure,
   because they generally shouldn't be allocated dynamically.

   In this case, we also provide a flag to indicate if a device is
   "stopped" due to a power management event, or card ejection.  The
   device IO routines can use a flag like this to throttle IO to a
   card that is not ready to accept it.
*/
   

/*
  BeOS bus manager declarations: we use the shorthand references to
  bus manager functions to make the code somewhat more portable across
  platforms.
*/
static cs_client_module_info *cs = NULL;
static ds_module_info *ds = NULL;
#define CardServices		cs->_CardServices
#define register_pccard_driver	ds->_register_pccard_driver
#define unregister_pccard_driver ds->_unregister_pccard_driver

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err;
    err.func = func; err.retcode = ret;
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    pcmcia_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

    The dev_link structure is initialized, but we don't actually
    configure the card at this point -- we wait until we receive a
    card insertion event.
    
======================================================================*/

static dev_link_t *pcmcia_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    local_info_t *local;
#if 0
	ether_dev_info_t *dev;
#endif
    int ret, i;
	    
   	dprintf(kDevName ": pcmcia_attach\n");

    /* Find a free slot in the device table */
    for (i = 0; i < MAX_CARDS; i++) {
		if (devs[i] == NULL) break;
	    if (i == MAX_CARDS) {
			dprintf(kDevName ": device limit %d exceede\n", MAX_CARDS);
			return NULL;
   		}
    }
    /* Initialize the dev_link_t structure */
    link = malloc(sizeof(struct dev_link_t));
  	if (link == NULL) return NULL;
	memset(link, 0, sizeof(struct dev_link_t));
    devs[i] = link;
    ndevs++;
    link->release.function = &pcmcia_release;
    link->release.data = (u_long)link;


    /* Interrupt setup */
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
	link->irq.IRQInfo2 = 0xdeb8;
    link->irq.Handler = NULL;
    
    /*
      General socket configuration defaults can go here.  In this
      client, we assume very little, and rely on the CIS for almost
      everything.  In most clients, many details (i.e., number, sizes,
      and attributes of IO windows) are fixed by the nature of the
      device, and can be hard-wired here.
    */
    link->conf.Attributes = 0;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;

    /* Allocate space for private device-specific data */
    local = malloc(sizeof(local_info_t));
    if (local == NULL) goto err1;
	memset(local, 0, sizeof(local_info_t));
    link->priv = local;

#if 0
    /* Allocate driver instance (per card) globals */
    dev = malloc(sizeof(ether_dev_info_t));
    if (dev == NULL) goto err2;
	memset(dev, 0, sizeof(ether_dev_info_t));
    local->dev = dev;
#endif

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &pcmcia_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
		cs_error(link->handle, RegisterClient, ret);
		pcmcia_detach(link);
		goto err3;
    }

    return link;

	err3:
#if 0
		free(dev);
	err2:
#endif
		free(local);
	err1:
		free(link);
		return NULL;

} /* pcmcia_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void pcmcia_detach(dev_link_t *link)
{
    int i;
    
    dprintf(kDevName ": pcmcia_detach(0x%p)\n", link);
    
	if (link == NULL) {
		dprintf("pcmcia_detach called with NULL argumant\n");
		return;
	}

    /* Locate device structure */
    for (i = 0; i < MAX_CARDS; i++)
		if (devs[i] == link) break;
    if (i == MAX_CARDS)
		return;

 dprintf(kDevName ": pcmcia_detach link %x\n", (int) link); 
	 /*
       If the device is currently configured and active, we won't
       actually delete it yet.  Instead, it is marked so that when
       the release() function is called, that will trigger a proper
       detach().
    */
    if (link->state & DEV_CONFIG) {
		dprintf( kDevName ": detach postponed, '%s' " "still locked\n", link->dev->dev_name);
		link->state |= DEV_STALE_LINK;
		return;
    }

    /* Break the link with Card Services */
    if (link->handle)
		CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free pieces */
    devs[i] = NULL;
	if (link->priv)
		free(link->priv);
    free(link);
    ndevs--;
} /* pcmcia_detach */

/*======================================================================

    prism_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, h, a) \
while ((last_ret=CardServices(last_fn=(fn),h,a))!=0) goto cs_failed
#define CS_CHECK2(fn, h, a, b) \
while ((last_ret=CardServices(last_fn=(fn),h,a,b))!=0) goto cs_failed

#define CFG_CHECK(fn, h, a) \
if (CardServices(fn, h, a) != 0) goto next_entry
#define CFG_CHECK2(fn, h, a, b) \
if (CardServices(fn, h, a, b) != 0) goto next_entry

static void prism_config(dev_link_t *link)
{
	client_handle_t handle = link->handle;
	tuple_t tuple;
	cisparse_t parse;
	local_info_t *dev = link->priv;
	int last_fn, last_ret, nd;
	u_char buf[64];
	win_req_t req;
	memreq_t map;
	
	
	dprintf("prism_config(0x%p)\n", link);
	
	/*
	   This reads the card's CONFIG tuple to find its configuration
	   registers.
	*/
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK2(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];
	
	/* Configure card */
	link->state |= DEV_CONFIG;
	
	/*
	  In this loop, we scan the CIS for configuration table entries,
	  each of which describes a valid card configuration, including
	  voltage, IO window, memory window, and interrupt settings.
	
	  We make no assumptions about the card to be configured: we use
	  just the information available in the CIS.  In an ideal world,
	  this would work for any PCMCIA card, but it requires a complete
	  and accurate CIS.  In practice, a driver usually "knows" most of
	  these things without consulting the CIS, and most client drivers
	  will only use the CIS to fill in implementation-defined details.
	*/
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t dflt = { 0 };
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK2(ParseTuple, handle, &tuple, &parse);
		
		if (cfg->index == 0) goto next_entry;
		link->conf.ConfigIndex = cfg->index;
		
		/* Does this card need audio output? */
		if (cfg->flags & CISTPL_CFTABLE_AUDIO) {
			link->conf.Attributes |= CONF_ENABLE_SPKR;
			link->conf.Status = CCSR_AUDIO_ENA;
		}
	
		/* Use power settings for Vcc and Vpp if present */
		/*  Note that the CIS values need to be rescaled */
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;

		if (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			                  cfg->vpp1.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			                  dflt.vpp1.param[CISTPL_POWER_VNOM]/10000;
	
		/* Do we need to allocate an interrupt? */
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1)
			link->conf.Attributes |= CONF_ENABLE_IRQ;

		/* IO window settings */
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
			link->io.BasePort1 = io->win[0].base;
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}
		
		/* This reserves IO space but doesn't actually enable it */
		CFG_CHECK(RequestIO, link->handle, &link->io);
		
		/*
		  Now set up a common memory window, if needed.  There is room
		  in the dev_link_t structure for one memory window handle,
		  but if the base addresses need to be saved, or if multiple
		  windows are needed, the info should go in the private data
		  structure for this device.
		
		  Note that the memory window base is a physical address, and
		  needs to be mapped to virtual space with ioremap() before it
		  is used.
		*/
		if ((cfg->mem.nwin > 0) || (dflt.mem.nwin > 0)) {
			cistpl_mem_t *mem = (cfg->mem.nwin) ? &cfg->mem : &dflt.mem;
			req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM;
			req.Base = mem->win[0].host_addr;
			req.Size = mem->win[0].len;
			req.AccessSpeed = 0;
			link->win = (window_handle_t)link->handle;
			CFG_CHECK(RequestWindow, &link->win, &req);
			map.Page = 0; map.CardOffset = mem->win[0].card_addr;
			CFG_CHECK(MapMemPage, link->win, &map);
		}
		/* If we got this far, we're cool! */
		break;
		
	next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		CS_CHECK(GetNextTuple, handle, &tuple);
	}
		
	/*
	  Allocate an interrupt line.  Note that this does not assign a
	  handler to the interrupt, unless the 'Handler' member of the
	  irq structure is initialized.
	*/
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	
	/*
	   This actually configures the PCMCIA socket -- setting up
	   the I/O windows and the interrupt mapping, and putting the
	   card and host interface into "Memory and IO" mode.
	*/
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);
	
	/*
	  At this point, the dev_node_t structure(s) need to be
	  initialized and arranged in a linked list at link->dev.
	*/
	for (nd = 0; nd < MAX_CARDS; nd++)
		if (devs[nd] == link) break;
	sprintf(dev->node.dev_name, "net/" kDevName "/%d", nd);
	dev->node.major = dev->node.minor = 0;
	link->dev = &dev->node;
	
	/* Finally, report what we've done */
	dprintf("%s: index 0x%02x: Vcc %d.%d",
	       dev->node.dev_name, link->conf.ConfigIndex,
	       link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1)
		dprintf(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		dprintf(", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
	dprintf(", io 0x%04x-0x%04x", link->io.BasePort1,
	       link->io.BasePort1+link->io.NumPorts1-1);
	if (link->io.NumPorts2)
		dprintf(" & 0x%04x-0x%04x", link->io.BasePort2,
	link->io.BasePort2+link->io.NumPorts2-1);
	if (link->win)
		dprintf(", mem 0x%06lx-0x%06lx", req.Base,
	req.Base+req.Size-1);
	dprintf("\n");
	
	link->state &= ~DEV_CONFIG_PENDING;
	return;

cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	pcmcia_release((u_long)link);
} /* prism_config */

/*======================================================================

    After a card is removed, pcmcia_release() will unregister the
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void pcmcia_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;

   	dprintf(kDevName ": pcmcia_release(0x%p)\n", link);

    /*
       If the device is currently in use, we won't release until it
       is actually closed, because until then, we can't be sure that
       no one will try to access the device or its data structures.
    */
    if (link->open) {
	dprintf(kDevName ": prism_cs: release postponed, '%s' still open\n",
		  link->dev->dev_name);
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    /* Unlink the device chain */
    link->dev = NULL;

    /*
      In a normal driver, additional code may be needed to release
      other kernel data structures associated with this device. 
    */
    
    /* Don't bother checking to see if these succeed or not */
    if (link->win)
	CardServices(ReleaseWindow, link->win);
    CardServices(ReleaseConfiguration, link->handle);
    if (link->io.NumPorts1)
	CardServices(ReleaseIO, link->handle, &link->io);
    if (link->irq.AssignedIRQ)
	CardServices(ReleaseIRQ, link->handle, &link->irq);
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	pcmcia_detach(link);
    
} /* pcmcia_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.

    When a CARD_REMOVAL event is received, we immediately set a
    private flag to block future accesses to this device.  All the
    functions that actually access the device should check this flag
    to make sure the card is still present.
    
======================================================================*/

static int pcmcia_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    dprintf(kDevName ": pcmcia_event(0x%06x) link%x\n", (int) event, (int) link);
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
		link->state &= ~DEV_PRESENT;
		release_sem_etc(((local_info_t *)link->priv)->read_sem, 1, B_DO_NOT_RESCHEDULE);
		if (link->state & DEV_CONFIG) {
		    ((local_info_t *)link->priv)->stop = 1;
			link->release.expires = RUN_AT(HZ/20);
	   		cs->_add_timer(&link->release);
		}
		break;
    case CS_EVENT_CARD_INSERTION:
		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		prism_config(link);
		break;
    case CS_EVENT_PM_SUSPEND:
		link->state |= DEV_SUSPEND;
		/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
		/* Mark the device as stopped, to block IO until later */
		((local_info_t *)link->priv)->stop = 1;
		if (link->state & DEV_CONFIG)
		    CardServices(ReleaseConfiguration, link->handle);
		break;
    case CS_EVENT_PM_RESUME:
		link->state &= ~DEV_SUSPEND;
		/* Fall through... */
 	   case CS_EVENT_CARD_RESET:
		if (link->state & DEV_CONFIG)
	    CardServices(RequestConfiguration, link->handle, &link->conf);
		((local_info_t *)link->priv)->stop = 0;
		/*
		  In a normal driver, additional code may go here to restore
		  the device state and restart IO. 
		*/
		break;
    }
    return 0;
} /* pcmcia_event */


status_t pc_init_driver(void)
{
    servinfo_t serv;
    int i;
    
    for(i=0;i<MAX_CARDS;i++) devs[i] = NULL;
    
    if (get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs) != B_OK) {
		goto err1;
    }
	if (get_module(DS_MODULE_NAME, (struct module_info **)&ds) != B_OK) {
		goto err2;
	}
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
		goto err3;
	}
    /* When this is called, Driver Services will "attach" this driver
       to any already-present cards that are bound appropriately. */
    register_pccard_driver(&dev_info, &pcmcia_attach, &pcmcia_detach);
    return B_OK;
err3:	
	put_module(DS_MODULE_NAME);
err2:
	put_module(CS_CLIENT_MODULE_NAME);
err1:
	return B_ERROR;
}


void pc_uninit_driver(void)
{
    int i;
    if(cs && ds){
        unregister_pccard_driver(&dev_info);
        for (i = 0; i < MAX_CARDS; i++)
        if (devs[i]) {
            if (devs[i]->state & DEV_CONFIG)
            pcmcia_release((u_long)devs[i]);
            pcmcia_detach(devs[i]);
        }
		if (names != NULL) free(names);
		if (ds) put_module(DS_MODULE_NAME);
		if (cs) put_module(CS_CLIENT_MODULE_NAME);
    }
}


const char **publish_devices(void)
{
    int i, nd;

	if (names != NULL) free(names);
	names = malloc((ndevs+1) * sizeof(char *));
	for (nd = i = 0; nd < MAX_CARDS; nd++) {
		if (devs[nd] == NULL) continue;
		names[i++] = devs[nd]->dev->dev_name;
	}
	names[i] = NULL;
	return (const char **)names;
}



/*----------------------------------------------------------------
* hfa384x_inithw
----------------------------------------------------------------*/
void hfa384x_inithw( hfa384x_t *hw, UINT irq, UINT iobase)
{
	DBFENTER;
	memset(hw, 0, sizeof(hfa384x_t));
	hw->irq = irq;
	hw->iobase = iobase;
	hw->bap = HFA384x_BAP_PROC;

	/* initialize spinlocks, which is done in the memset above */
	//hw->baplock= 0 hw->cmdlock = 0;
	
	DBFEXIT;
	return;
}

/*----------------------------------------------------------------
* hfa384x_drvr_getconfig
----------------------------------------------------------------*/
int hfa384x_drvr_getconfig(hfa384x_t *hw, UINT16 rid, void *buf, UINT16 len)
{
	int 		result = 0;
	hfa384x_rec_t	rec;
	DBFENTER;
	result = hfa384x_cmd_access( hw, 0, rid);
	if ( result ) {
		dprintf("Call to hfa384x_cmd_access failed\n");
		goto fail;
	}

	result = hfa384x_copy_from_bap( hw, hw->bap, rid, 0, &rec, sizeof(rec));
	if ( result ) {
		dprintf("Call to hfa384x_copy_from_bap failed\n");
		goto fail;
	}

	/* Validate the record length */
	if ( ((hfa384x2host_16(rec.reclen)-1)*2) != len ) {  /* note body len calculation in bytes */
		dprintf("RID len mismatch, rid=0x%04x hlen=%d fwlen=%d\n",
			rid, len, (hfa384x2host_16(rec.reclen)-1)*2);
		result = -B_ERROR;
		goto fail;
	}
	result = hfa384x_copy_from_bap( hw, hw->bap, rid, sizeof(rec), buf, len);
fail:
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_getconfig16
----------------------------------------------------------------*/
int hfa384x_drvr_getconfig16(hfa384x_t *hw, UINT16 rid, void *val)
{
	int		result = 0;
	DBFENTER;
	result = hfa384x_drvr_getconfig(hw, rid, val, sizeof(UINT16));
	if ( result == 0 ) {
		*((UINT16*)val) = hfa384x2host_16(*((UINT16*)val));
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_getconfig32
*
* Performs the sequence necessary to read a 32 bit config/info item
* and convert it to host order.
*----------------------------------------------------------------*/
int hfa384x_drvr_getconfig32(hfa384x_t *hw, UINT16 rid, void *val)
{
	int		result = 0;
	DBFENTER;
	result = hfa384x_drvr_getconfig(hw, rid, val, sizeof(UINT32));
	if ( result == 0 ) {
		*((UINT32*)val) = hfa384x2host_32(*((UINT32*)val));
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_setconfig
----------------------------------------------------------------*/
int hfa384x_drvr_setconfig(hfa384x_t *hw, UINT16 rid, void *buf, UINT16 len)
{
	int		result = 0;
	hfa384x_rec_t	rec;
	DBFENTER;
	rec.rid = host2hfa384x_16(rid);
	rec.reclen = host2hfa384x_16((len/2) + 1); /* note conversion to words, +1 for rid field */
	/* write the record header */
	result = hfa384x_copy_to_bap( hw, hw->bap, rid, 0, &rec, sizeof(rec));
	if ( result ) {
		dprintf("Failure writing record header\n");
		goto fail;
	}
	/* write the record data (if there is any) */
	if ( len > 0 ) {
		result = hfa384x_copy_to_bap( hw, hw->bap, rid, sizeof(rec), buf, len);
		if ( result ) {
			dprintf("Failure writing record data\n");
			goto fail;
		}
	}
	result = hfa384x_cmd_access( hw, 1, rid);
fail:
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_setconfig16
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
*
----------------------------------------------------------------*/
int hfa384x_drvr_setconfig16(hfa384x_t *hw, UINT16 rid, UINT16 *val)
{
	int		result = 0;
	DBFENTER;
	*val = host2hfa384x_16(*val);
	result = hfa384x_drvr_setconfig(hw, rid, val, sizeof(UINT16));
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_setconfig32
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
----------------------------------------------------------------*/
int hfa384x_drvr_setconfig32(hfa384x_t *hw, UINT16 rid, UINT32 *val)
{
	int		result = 0;
	DBFENTER;
	*val = host2hfa384x_32(*val);
	result = hfa384x_drvr_setconfig(hw, rid, val, sizeof(UINT32));
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_readpda
*
* Performs the sequence to read the PDA space.  Note there is no
* drvr_writepda() function.  Writing a PDA is
* generally implemented by a calling component via calls to 
* cmd_download and writing to the flash download buffer via the 
* aux regs.

* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
*	-ETIMEOUT	timout waiting for the cmd regs to become
*			available, or waiting for the control reg
*			to indicate the Aux port is enabled.
*	-B_ERROR	the buffer does NOT contain a valid PDA.
*			Either the card PDA is bad, or the auxdata
*			reads are giving us garbage.
* Call context:
*	process thread or non-card interrupt.
----------------------------------------------------------------*/
int hfa384x_drvr_readpda(hfa384x_t *hw, void *buf, UINT len)
{
	int		result = 0;
	UINT16		*pda = buf;
	int		pdaok = 0;
	int		morepdrs = 1;
	int		currpdr = 0;	/* word offset of the current pdr */
	int		i;
	UINT16		pdrlen;		/* pdr length in bytes, host order */
	UINT16		pdrcode;	/* pdr code, host order */
	UINT32		cardaddr[] = {HFA384x_PDA_BASE, HFA384x_PDA_BOGUS_BASE};
	DBFENTER;
	/* Check for aux available */
	result = hfa384x_cmd_aux_enable(hw);
	if ( result ) {
		dprintf(kDevName ": aux_enable() failed. result=%d\n", result);
		goto failed;
	}

	/* Read the pda from each known address.  */
//	The first "known address" looks bogus on beos - although it looks ok on linux -
//	In any case the 2nd one overwrites the first (buf)- there is no point in
//	reading the first address space (i=0).
//	for ( i = 0; i < (sizeof(cardaddr)/sizeof(cardaddr[0])); i++)
	for ( i = 0; i < (sizeof(cardaddr)/sizeof(cardaddr[0])); i++)
	{
		hfa384x_copy_from_aux(hw, cardaddr[i], buf, len);
		/* Test for garbage */
		pdaok = 1;	/* intially assume good */
		morepdrs = 1;
		while ( pdaok && morepdrs ) {
			pdrlen = hfa384x2host_16(pda[currpdr]) * 2;
			pdrcode = hfa384x2host_16(pda[currpdr+1]);
			/* Test the record length */

			if ( pdrlen > HFA384x_PDR_LEN_MAX || pdrlen == 0) {
				dprintf("pdrlen invalid=%d\n", pdrlen);
				pdaok = 0;
				break;
			}
			/* Test the code */
			if ( !hfa384x_isgood_pdrcode(pdrcode) ) {
				dprintf("pdrcode invalid=%d\n", pdrcode);
				pdaok = 0;
				break;
			}
			/* Test for completion */
			if ( pdrcode == HFA384x_PDR_END_OF_PDA) {
				morepdrs = 0;
			}
	
			/* Move to the next pdr (if necessary) */
			if ( morepdrs ) {
				/* note the access to pda[], we need words here */
				currpdr += hfa384x2host_16(pda[currpdr]) + 1;
			}
		}	
		if ( pdaok ) {
			break;
		} 
	}
	result = pdaok ? 0 : -B_ERROR;

	if ( result ) {
		dprintf("Failure: pda is not okay\n");
	}

	hfa384x_cmd_aux_disable(hw);
failed:
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_ramdl_enable
*
* Begins the ram download state.  Checks to see that we're not
* already in a download state and that a port isn't enabled.
* Sets the download state and calls cmd_download with the 
* ENABLE_VOLATILE subcommand and the exeaddr argument.
*
* Arguments:
*	hw		device structure
*	exeaddr		the card execution address that will be 
*                       jumped to when ramdl_disable() is called
*			(host order).
*
* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
----------------------------------------------------------------*/
int hfa384x_drvr_ramdl_enable(hfa384x_t *hw, UINT32 exeaddr)
{
	int		result = 0;
	UINT16		lowaddr;
	UINT16		hiaddr;
	int		i;
	DBFENTER;
	/* Check that a port isn't active */
	for ( i = 0; i < HFA384x_PORTID_MAX; i++) {
		if ( hw->port_enabled[i] ) {
			dprintf("Can't download with a port enabled.\n");
			return -EINVAL; 
		}
	}

	/* Check that we're not already in a download state */
	if ( hw->dlstate != HFA384x_DLSTATE_DISABLED ) {
		dprintf("Download state not disabled.\n");
		return -EINVAL;
	}

	/* Retrieve the buffer loc&size and timeout */
	if ( (result = hfa384x_drvr_getconfig(hw, HFA384x_RID_DOWNLOADBUFFER, 
				&(hw->bufinfo), sizeof(hw->bufinfo))) ) {
		return result;
	}
	hw->bufinfo.page = hfa384x2host_16(hw->bufinfo.page);
	hw->bufinfo.offset = hfa384x2host_16(hw->bufinfo.offset);
	hw->bufinfo.len = hfa384x2host_16(hw->bufinfo.len);
	if ( (result = hfa384x_drvr_getconfig16(hw, HFA384x_RID_MAXLOADTIME, 
				&(hw->dltimeout))) ) {
		return result;
	}
	hw->dltimeout = hfa384x2host_16(hw->dltimeout);

#if 0
	dprintf(kDevName ": ramdl_enable, exeaddr=0x%08x\n", exeaddr);
	hw->dlstate = HFA384x_DLSTATE_RAMENABLED;
#endif
	

	/* Enable the aux port */
	if ( (result = hfa384x_cmd_aux_enable(hw)) ) {
		dprintf("Aux enable failed, result=%d.\n", result);
		return result;
	}

	/* Call the download(1,addr) function */
	lowaddr = (UINT16)(exeaddr & 0x0000ffff);
	hiaddr =  (UINT16)(exeaddr >> 16);

	result = hfa384x_cmd_download(hw, HFA384x_PROGMODE_RAM, 
			lowaddr, hiaddr, 0);
	if ( result == 0) {
		/* Set the download state */
		hw->dlstate = HFA384x_DLSTATE_RAMENABLED;
	} else {
		dprintf("cmd_download(0x%04x, 0x%04x) failed, result=%d.\n",
				lowaddr,hiaddr, result);
		/* Disable  the aux port */
		hfa384x_cmd_aux_disable(hw);
	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_ramdl_disable
*
* Ends the ram download state.
----------------------------------------------------------------*/
int hfa384x_drvr_ramdl_disable(hfa384x_t *hw)
{
	DBFENTER;
	/* Check that we're already in the download state */
	if ( hw->dlstate != HFA384x_DLSTATE_RAMENABLED ) {
		return -EINVAL;
	}

	/* There isn't much we can do at this point, so I don't */
	/*  bother  w/ the return value */
	hfa384x_cmd_download(hw, HFA384x_PROGMODE_DISABLE, 0, 0 , 0);
	hw->dlstate = HFA384x_DLSTATE_DISABLED;

	/* Disable the aux port */
	hfa384x_cmd_aux_disable(hw);

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* hfa384x_drvr_ramdl_write
*
* Performs a RAM download of a chunk of data. First checks to see
* that we're in the RAM download state, then uses the aux functions
* to 1) copy the data, 2) readback and compare.  The download
* state is unaffected.  When all data has been written using
* this function, call drvr_ramdl_disable() to end the download state
* and restart the MAC.
*
* Arguments:
*	hw		device structure
*	daddr		Card address to write to. (host order)
*	buf		Ptr to data to write.
*	len		Length of data (host order).
*
* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
----------------------------------------------------------------*/
int hfa384x_drvr_ramdl_write(hfa384x_t *hw, UINT32 daddr, void* buf, UINT32 len)
{
	int		result = 0;
	UINT8		*verbuf;
	DBFENTER;
	/* Check that we're in the ram download state */
	if ( hw->dlstate != HFA384x_DLSTATE_RAMENABLED ) {
		return -EINVAL;
	}

	dprintf("Writing %ld bytes to ram @0x%06lx\n", len, daddr);
#if 0
WLAN_HEX_DUMP(1, "dldata", buf, len);
#endif
	/* Copy the data via the aux port */
	hfa384x_copy_to_aux(hw, daddr, buf, len);

	/* Create a buffer for the verify */
	verbuf = (unsigned char *) malloc(len);
	if (verbuf == NULL ) return 1;

	/* Read back and compare */
	hfa384x_copy_from_aux(hw, daddr, verbuf, len);

	if ( memcmp(buf, verbuf, len) ) {
		dprintf("ramdl verify failed!\n");
		result = -EINVAL;
	}
	free(verbuf);
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_flashdl_enable
----------------------------------------------------------------*/
int hfa384x_drvr_flashdl_enable(hfa384x_t *hw)
{
	int		result = 0;
	int		i;

	DBFENTER;
	/* Check that a port isn't active */
	for ( i = 0; i < HFA384x_PORTID_MAX; i++) {
		if ( hw->port_enabled[i] ) {
			dprintf("called when port enabled.\n");
			return -EINVAL; 
		}
	}

	/* Check that we're not already in a download state */
	if ( hw->dlstate != HFA384x_DLSTATE_DISABLED ) {
		return -EINVAL;
	}

	/* Retrieve the buffer loc&size and timeout */
	if ( (result = hfa384x_drvr_getconfig(hw, HFA384x_RID_DOWNLOADBUFFER, 
				&(hw->bufinfo), sizeof(hw->bufinfo))) ) {
		return result;
	}
	hw->bufinfo.page = hfa384x2host_16(hw->bufinfo.page);
	hw->bufinfo.offset = hfa384x2host_16(hw->bufinfo.offset);
	hw->bufinfo.len = hfa384x2host_16(hw->bufinfo.len);
	if ( (result = hfa384x_drvr_getconfig16(hw, HFA384x_RID_MAXLOADTIME, 
				&(hw->dltimeout))) ) {
		return result;
	}
	hw->dltimeout = hfa384x2host_16(hw->dltimeout);

	/* Enable the aux port */
	if ( (result = hfa384x_cmd_aux_enable(hw)) ) {
		return result;
	}

	hw->dlstate = HFA384x_DLSTATE_FLASHENABLED;
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_drvr_flashdl_disable
*
* Ends the flash download state.  Note that this will cause the MAC
* firmware to restart.
----------------------------------------------------------------*/
int hfa384x_drvr_flashdl_disable(hfa384x_t *hw)
{
	DBFENTER;
	/* Check that we're already in the download state */
	if ( hw->dlstate != HFA384x_DLSTATE_FLASHENABLED ) {
		return -EINVAL;
	}

	/* There isn't much we can do at this point, so I don't */
	/*  bother  w/ the return value */
	hfa384x_cmd_download(hw, HFA384x_PROGMODE_DISABLE, 0, 0 , 0);
	hw->dlstate = HFA384x_DLSTATE_DISABLED;

	/* Disable the aux port */
	hfa384x_cmd_aux_disable(hw);

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* hfa384x_drvr_flashdl_write
----------------------------------------------------------------*/
int hfa384x_drvr_flashdl_write(hfa384x_t *hw, UINT32 daddr, void* buf, UINT32 len)
{
	int		result = 0;
	UINT8		*verbuf;
	UINT32		dlbufaddr;
	UINT32		currlen;
	UINT32		currdaddr;
	UINT16		destlo;
	UINT16		desthi;
	int		nwrites;
	int		i;

	DBFENTER;
	/* Check that we're in the flash download state */
	if ( hw->dlstate != HFA384x_DLSTATE_FLASHENABLED ) {
		return -EINVAL;
	}

	dprintf(kDevName ": Download %ld bytes to flash @0x%06lx\n", len, daddr);

	/* For convenience */
	dlbufaddr = (hw->bufinfo.page << 7) | hw->bufinfo.offset;
	verbuf = (UINT8 *) malloc(hw->bufinfo.len);


	dprintf(kDevName ": dlbuf@0x%06lx len=%d to=%d\n", dlbufaddr, hw->bufinfo.len, hw->dltimeout);

	/* Figure out how many times to to the flash prog */
	nwrites = len / hw->bufinfo.len;
	nwrites += (len % hw->bufinfo.len) ? 1 : 0;

	if ( verbuf == NULL ) {
		dprintf("Failed to allocate flash verify buffer\n");
		return 1;
	}
	/* For each */
	for ( i = 0; i < nwrites; i++) {
		/* Get the dest address and len */
		currlen = (len - (hw->bufinfo.len * i)) > hw->bufinfo.len ?
				hw->bufinfo.len : 
				(len - (hw->bufinfo.len * i));
		currdaddr = daddr + (hw->bufinfo.len * i);
		destlo = currdaddr & 0x0000ffff;
		desthi = currdaddr >> 16;
		dprintf("Writing %ld bytes to flash @0x%06lx\n", currlen, currdaddr);
#if 0
WLAN_HEX_DUMP(1, "dldata", buf+(hw->bufinfo.len*i), currlen);
#endif
		/* Set the download mode */
		result = hfa384x_cmd_download(hw, HFA384x_PROGMODE_NV, 
				destlo, desthi, currlen);
		if ( result ) {
			dprintf("download(NV,lo=%x,hi=%x,len=%lx) cmd failed. Aborting d/l\n",
				destlo, desthi, currlen);
			free(verbuf);
			return result;
		}
		/* copy the data to the flash buffer */
		hfa384x_copy_to_aux(hw, dlbufaddr, 
					(char *) buf + (hw->bufinfo.len*i), currlen);
		/* set the download 'write flash' mode */
		result = hfa384x_cmd_download(hw, HFA384x_PROGMODE_NVWRITE, 0,0,0);
		if ( result ) {
			dprintf("download(NVWRITE,lo=%x,hi=%x,len=%lx) cmd failed. Aborting d/l\n",
				destlo, desthi, currlen);
			free(verbuf);
			return result;
		}
		/* readback and compare, if fail...bail */
		hfa384x_copy_from_aux(hw, currdaddr, verbuf, currlen);

		if ( memcmp((char *)buf+(hw->bufinfo.len*i), verbuf, currlen) ) {
			free(verbuf);
			return -EINVAL;
		}
	}

	/* Leave the firmware in the 'post-prog' mode.  flashdl_disable will */
	/*  actually disable programming mode.  Remember, that will cause the */
	/*  the firmware to effectively reset itself. */
	
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_initialize
*
* Issues the initialize command and sets the hw->state based
* on the result.
*
----------------------------------------------------------------*/
int hfa384x_cmd_initialize(hfa384x_t *hw)
{
	int result = 0;
	int i;
	DBFENTER;
	result = hfa384x_docmd_wait(hw, HFA384x_CMDCODE_INIT, 0,0,0);
	if ( result == 0 ) {
		hw->state = HFA384x_STATE_INIT;
		for ( i = 0; i < HFA384x_NUMPORTS_MAX; i++) {
			hw->port_enabled[i] = 0;
		}
	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_enable
*
* Issues the enable command to enable communications on one of 
* the MACs 'ports'.  Only macport 0 is valid  for stations.
* APs may also enable macports 1-6.  Only ports that are currently
* disabled may be enabled.
----------------------------------------------------------------*/
int hfa384x_cmd_enable(hfa384x_t *hw, UINT16 macport)
{
	int	result = 0;
	UINT16	cmd;

	DBFENTER;
	if ((!hw->isap && macport != 0) || 
	    (hw->isap && !(macport <= HFA384x_PORTID_MAX)) ||
	    (hw->port_enabled[macport]) ){
		result = -EINVAL;
	} else {
		cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_ENABLE) |
			HFA384x_CMD_MACPORT_SET(macport);
		result = hfa384x_docmd_wait(hw, cmd, 0,0,0);
		if ( result == 0 ) {
			hw->port_enabled[macport] = 1;
		}
	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_disable
*
* Issues the disable command to stop communications on one of 
* the MACs 'ports'.  Only macport 0 is valid  for stations.
* APs may also disable macports 1-6.  Only ports that have been
* previously enabled may be disabled.
*
----------------------------------------------------------------*/
int hfa384x_cmd_disable(hfa384x_t *hw, UINT16 macport)
{
	int	result = 0;
	UINT16	cmd;

	DBFENTER;
	if ((!hw->isap && macport != 0) || 
	    (hw->isap && !(macport <= HFA384x_PORTID_MAX)) ||
	    !(hw->port_enabled[macport]) ){
		result = -EINVAL;
	} else {
		cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_DISABLE) |
			HFA384x_CMD_MACPORT_SET(macport);
		result = hfa384x_docmd_wait(hw, cmd, 0,0,0);
		if ( result == 0 ) {
			hw->port_enabled[macport] = 0;
		}
	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_diagnose
*
* Issues the diagnose command to test the: register interface,
* MAC controller (including loopback), External RAM, Non-volatile
* memory integrity, and synthesizers.  Following execution of this
* command, MAC/firmware are in the 'initial state'.  Therefore,
* the Initialize command should be issued after successful
* completion of this command.  This function may only be called
* when the MAC is in the 'communication disabled' state.
----------------------------------------------------------------*/
#define DIAG_PATTERNA ((UINT16)0xaaaa)
#define DIAG_PATTERNB ((UINT16)~0xaaaa)

int hfa384x_cmd_diagnose(hfa384x_t *hw)
{
	int	result = 0;
	UINT16	cmd;

	DBFENTER;
	cmd = HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_DIAG);
	result = hfa384x_docmd_wait(hw, cmd, DIAG_PATTERNA, DIAG_PATTERNB, 0);
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_allocate
*
* Issues the allocate command instructing the firmware to allocate
* a 'frame structure buffer' in MAC controller RAM.  This command
* does not provide the result, it only initiates one of the f/w's
* asynchronous processes to construct the buffer.  When the 
* allocation is complete, it will be indicated via the Alloc
* bit in the EvStat register and the FID identifying the allocated
* space will be available from the AllocFID register.  Some care
* should be taken when waiting for the Alloc event.  If a Tx or 
* Notify command w/ Reclaim has been previously executed, it's
* possible the first Alloc event after execution of this command
* will be for the reclaimed buffer and not the one you asked for.
* This case must be handled in the Alloc event handler.
*
----------------------------------------------------------------*/
int hfa384x_cmd_allocate(hfa384x_t *hw, UINT16 len)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	if ( (len % 2) || 
	     len < HFA384x_CMD_ALLOC_LEN_MIN || 
	     len > HFA384x_CMD_ALLOC_LEN_MAX ) {
		result = -EINVAL;
	} else {
		cmd = HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_ALLOC);
		result = hfa384x_docmd_wait(hw, cmd, len, 0, 0);
	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_transmit
*
* Instructs the firmware to transmit a frame previously copied
* to a given buffer.  This function returns immediately, the Tx
* results are available via the Tx or TxExc events (if the frame
* control bits are set).  The reclaim argument specifies if the 
* FID passed will be used by the f/w tx process or returned for
* use w/ another transmit command.  If reclaim is set, expect an 
* Alloc event signalling the availibility of the FID for reuse.
*
* Arguments:
*	hw		device structure
*	reclaim		[0|1] indicates whether the given FID will
*			be handed back (via Alloc event) for reuse.
*			(host order)
*	qos		[0-3] Value to put in the QoS field of the 
*			tx command, identifies a queue to place the 
*			outgoing frame in.
*			(host order)
*	fid		FID of buffer containing the frame that was
*			previously copied to MAC memory via the bap.
*			(host order)
*
* Returns: 
*	0		success
*	>0		f/w reported failure - f/w status code
*	<0		driver reported error (timeout|bad arg)
*
* Side effects:
*	hw->resp0 will contain the FID being used by async tx
*	process.  If reclaim==0, resp0 will be the same as the fid
*	argument.  If reclaim==1, resp0 will be the different and
*	is the value to watch for in the Tx|TxExc to indicate completion
*	of the frame passed in fid.
---------------------------------------------------------------*/
int hfa384x_cmd_transmit(hfa384x_t *hw, UINT16 reclaim, UINT16 qos, UINT16 fid)
{
	int	result = 0;
	UINT16	cmd;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_TX) |
		HFA384x_CMD_RECL_SET(reclaim) |
		HFA384x_CMD_QOS_SET(qos);
	result = hfa384x_docmd_wait(hw, cmd, fid, 0, 0);
	
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_clearpersist
*
* Instructs the firmware to clear the persistence bit in a given
* FID.  This has the effect of telling the firmware to drop the
* persistent frame.  The FID must be one that was previously used
* to transmit a PRST frame.
----------------------------------------------------------------*/
int hfa384x_cmd_clearpersist(hfa384x_t *hw, UINT16 fid)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_CLRPRST);
	result = hfa384x_docmd_wait(hw, cmd, fid, 0, 0);
	
	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* hfa384x_cmd_notify
*
* Sends an info frame to the firmware to alter the behavior
* of the f/w asynch processes.  Can only be called when the MAC
* is in the enabled state.
*
*
* Side effects:
*	hw->resp0 will contain the FID being used by async notify
*	process.  If reclaim==0, resp0 will be the same as the fid
*	argument.  If reclaim==1, resp0 will be the different.
----------------------------------------------------------------*/
int hfa384x_cmd_notify(hfa384x_t *hw, UINT16 reclaim, UINT16 fid)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_NOTIFY) |
		HFA384x_CMD_RECL_SET(reclaim);
	result = hfa384x_docmd_wait(hw, cmd, fid, 0, 0);
	
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_inquiry
*
* Requests an info frame from the firmware.  The info frame will
* be delivered asynchronously via the Info event.
*
----------------------------------------------------------------*/
int hfa384x_cmd_inquiry(hfa384x_t *hw, UINT16 fid)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_INQ);
	result = hfa384x_docmd_wait(hw, cmd, fid, 0, 0);
	
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_access
*
* Requests that a given record be copied to/from the record 
* buffer.  If we're writing from the record buffer, the contents
* must previously have been written to the record buffer via the 
* bap.  If we're reading into the record buffer, the record can
* be read out of the record buffer after this call.
----------------------------------------------------------------*/
int hfa384x_cmd_access(hfa384x_t *hw, UINT16 write, UINT16 rid)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_ACCESS) |
		HFA384x_CMD_WRITE_SET(write);
	result = hfa384x_docmd_wait(hw, cmd, rid, 0, 0);
	
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_download
*
* Sets the controls for the MAC controller code/data download
* process.  The arguments set the mode and address associated 
* with a download.  Note that the aux registers should be enabled
* prior to setting one of the download enable modes.
*
* Arguments:
*	hw		device structure
*	mode		0 - Disable programming and begin code exec
*			1 - Enable volatile mem programming
*			2 - Enable non-volatile mem programming
*			3 - Program non-volatile section from NV download
*			    buffer. 
*			(host order)
*	lowaddr		
*	highaddr	For mode 1, sets the high & low order bits of 
*			the "destination address".  This address will be
*			the execution start address when download is
*			subsequently disabled.
*			For mode 2, sets the high & low order bits of 
*			the destination in NV ram.
*			For modes 0 & 3, should be zero. (host order)
*	codelen		Length of the data to write in mode 2, 
*			zero otherwise. (host order)
----------------------------------------------------------------*/
int hfa384x_cmd_download(hfa384x_t *hw, UINT16 mode, UINT16 lowaddr, 
				UINT16 highaddr, UINT16 codelen)
{
	int	result = 0;
	UINT16	cmd;
	DBFENTER;
	cmd =	HFA384x_CMD_CMDCODE_SET(HFA384x_CMDCODE_DOWNLD) |
		HFA384x_CMD_PROGMODE_SET(mode);
#if defined(WLAN_STA)
	result = hfa384x_dl_docmd_wait(hw, cmd, lowaddr, highaddr, codelen);
#elif defined(WLAN_AP)
	result = hfa384x_docmd_wait(hw, cmd, lowaddr, highaddr, codelen);
#else
#error "WLAN_STA or WLAN_AP not defined!"
#endif
	
	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* hfa384x_cmd_aux_enable
*
* Goes through the process of enabling the auxilary port.  This 
* is necessary prior to raw reads/writes to card data space.  
* Direct access to the card data space is only used for downloading
* code and debugging.
* Note that a call to this function is required before attempting
* a download.
*
* Arguments:
*	hw		device structure
*
* Returns: 
*	0		success
*	>0		f/w reported failure - f/w status code
*	<0		driver reported error (timeout)
*
* Side effects:
*
* Call context:
*	process thread 
----------------------------------------------------------------*/
int hfa384x_cmd_aux_enable(hfa384x_t *hw)
{
	int				result = -ETIMEDOUT;
	bigtime_t		timeout;
	uint16			reg = 0;
	bigtime_t 		start_time;
	cpu_status		former;
	int 			not_busy=1;
	
	DBFENTER;

dprintf("check hfa384x_cmd_aux_enable for timing....\n");
	start_time = system_time();

	/* Check for existing enable */
	if ( hw->auxen ) {
		hw->auxen++;
		dprintf("aux_cmd_not enabled!!!!!!!!!!!\n");
		return 0;
	}

	/* acquire the semaphore - one command at a time */
	former = disable_interrupts();
	acquire_spinlock(&hw->cmdlock);



	/* wait for cmd the busy bit to clear */	
	timeout = system_time() + 1000L;
	reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
	while ( (HFA384x_CMD_ISBUSY(reg)) && 
	(not_busy = (system_time() < timeout)) ) {
		reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
	}
	if (!not_busy) dprintf(kDevName ": stuck busy in hfa384x_cd_aux_enable - 1\n");
	
	if (!HFA384x_CMD_ISBUSY(reg)) {
		/* busy bit clear, it's OK to write to ParamX regs */
		wlan_outw_cpu_to_le16(HFA384x_AUXPW0,
			HFA384x_PARAM0(hw->iobase));
		wlan_outw_cpu_to_le16(HFA384x_AUXPW1,
			HFA384x_PARAM1(hw->iobase));
		wlan_outw_cpu_to_le16(HFA384x_AUXPW2,
			HFA384x_PARAM2(hw->iobase));

		/* Set the aux enable in the Control register */
		wlan_outw_cpu_to_le16(HFA384x_CONTROL_AUX_DOENABLE, 
			HFA384x_CONTROL(hw->iobase));

		/* Now wait for completion */
		timeout = system_time() + 1000;
		reg = wlan_inw_le16_to_cpu(HFA384x_CONTROL(hw->iobase));
		while ( ((reg & (BIT14|BIT15)) != HFA384x_CONTROL_AUX_ISENABLED) &&
			(not_busy = (system_time() < timeout)) ){
			spin(10);
			reg = wlan_inw_le16_to_cpu(HFA384x_CONTROL(hw->iobase));
		}
		if (!not_busy) dprintf(kDevName ": stuck busy in hfa384x_cd_aux_enable - 1\n");
		
		if ( (reg & (BIT14|BIT15)) == HFA384x_CONTROL_AUX_ISENABLED ) {
			result = 0;
			hw->auxen++;
		}
	} else {
		dprintf("hfa384x_cmd_aux_enable: FAILED!!!!!!!!\n");
	}

	release_spinlock(&hw->cmdlock);
	restore_interrupts(former);

	dprintf("aux_cmd took %Ld uS\n", (system_time() - start_time));

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_cmd_aux_disable
*
* Goes through the process of disabling the auxilary port 
* enabled with aux_enable().
*
* Arguments:
*	hw		device structure
*
* Returns: 
*	0		success
*	>0		f/w reported failure - f/w status code
*	<0		driver reported error (timeout)
*
----------------------------------------------------------------*/
int hfa384x_cmd_aux_disable(hfa384x_t *hw)
{
	int		result = -ETIMEDOUT;
	UINT32		timeout;
	UINT16		reg = 0;

	DBFENTER;

	/* See if there's more than one enable */
	if (hw->auxen) hw->auxen--;
	if (hw->auxen) return 0;

	/* Clear the aux enable in the Control register */
	wlan_outw(0, HFA384x_PARAM0(hw->iobase));
	wlan_outw(0, HFA384x_PARAM1(hw->iobase));
	wlan_outw(0, HFA384x_PARAM2(hw->iobase));
	wlan_outw_cpu_to_le16(HFA384x_CONTROL_AUX_DODISABLE, 
		HFA384x_CONTROL(hw->iobase));

	/* Now wait for completion */
	timeout = system_time() + 1000000;  // 1 sec limit !!!!
	reg = wlan_inw_le16_to_cpu(HFA384x_CONTROL(hw->iobase));
	while ( ((reg & (BIT14|BIT15)) != HFA384x_CONTROL_AUX_ISDISABLED) &&
		(system_time() < timeout) ){
		spin(10);
		reg = wlan_inw_le16_to_cpu(HFA384x_CONTROL(hw->iobase));
	}
	if ((reg & (BIT14|BIT15)) == HFA384x_CONTROL_AUX_ISDISABLED ) {
		result = 0;
	}
	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* hfa384x_drvr_test_command
*
* Write test commands to the card.  Some test commands don't make
* sense without prior set-up.  For example, continous TX isn't very
* useful until you set the channel.  That functionality should be
* enforced at a higher level.
*
* Arguments:
*	hw		device structure
*	test_mode	The test command code to use.
*       test_param      A parameter needed for the test mode being used.
*
* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
*
* Side effects:
*
* Call context:
*	process thread 
----------------------------------------------------------------*/
int hfa384x_drvr_test_command(hfa384x_t *hw, UINT32 test_mode, 
			      UINT32 test_param)
{
	int		result = 0;
	UINT16	cmd = (UINT16) test_mode;
	UINT16 param = (UINT16) test_param;
	DBFENTER;

	/* Do i need a host2hfa... conversion ? */
	result = hfa384x_docmd_wait(hw, cmd, param, 0, 0);

	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* hfa384x_drvr_mmi_read
*
* Read mmi registers.  mmi is intersil-speak for the baseband
* processor registers.
*
* Arguments:
*	hw		device structure
*	register	The test register to be accessed (must be even #).
*
* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
*
* Side effects:
*
* Call context:
*	process thread 
----------------------------------------------------------------*/
int hfa384x_drvr_mmi_read(hfa384x_t *hw, UINT32 addr)
{
	int		result = 0;
	UINT16	cmd_code = (UINT16) 0x30;
	UINT16 param = (UINT16) addr;
	DBFENTER;

	/* Do i need a host2hfa... conversion ? */
	result = hfa384x_docmd_wait(hw, cmd_code, param, 0, 0);

	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* hfa384x_drvr_mmi_write
*
* Read mmi registers.  mmi is intersil-speak for the baseband
* processor registers.
*
* Arguments:
*	hw		device structure
*	addr            The test register to be accessed (must be even #).
*       data            The data value to write to the register.
*
* Returns: 
*	0		success
*	>0		f/w reported error - f/w status code
*	<0		driver reported error
*
* Side effects:
*
* Call context:
*	process thread 
----------------------------------------------------------------*/

int 
hfa384x_drvr_mmi_write(hfa384x_t *hw, UINT32 addr, UINT32 data)
{
	int		result = 0;
	UINT16	cmd_code = (UINT16) 0x31;
	UINT16 param0 = (UINT16) addr;
	UINT16 param1 = (UINT16) data;
	DBFENTER;

	dprintf(kDevName ": mmi write[0x%08lx] = 0x%08lx\n", addr, data);

	/* Do i need a host2hfa... conversion ? */
	result = hfa384x_docmd_wait(hw, cmd_code, param0, param1, 0);

	DBFEXIT;
	return result;
}


/* TODO: determine if these will ever be needed */
#if 0
int hfa384x_cmd_readmif(hfa384x_t *hw)
{
	DBFENTER;
	DBFEXIT;
	return 0;
}


int hfa384x_cmd_writemif(hfa384x_t *hw)
{
	DBFENTER;
	DBFEXIT;
	return 0;
}
#endif


/*----------------------------------------------------------------
* hfa384x_copy_from_bap
*
* Copies a collection of bytes from the MAC controller memory via
* one set of BAP registers.
*
* Arguments:
*	hw		device structure
*	bap		[0|1] which BAP to use
*	id		FID or RID, destined for the select register (host order)
*	offset		An _even_ offset into the buffer for the given
*			FID/RID.  We haven't the means to validate this,
*			so be careful. (host order)
*	buf		ptr to array of bytes
*	len		length of data to transfer in bytes
*
* Returns: 
*	0		success
*	>0		f/w reported failure - value of offset reg.
*	<0		driver reported error (timeout|bad arg)
*
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
int hfa384x_copy_from_bap(hfa384x_t *hw, UINT16 bap, UINT16 id, UINT16 offset,
				void *buf, UINT len)
{
	int		result = 0;
	UINT8		*d = (UINT8*)buf;
	UINT		selectreg;
	UINT		offsetreg;
	UINT		datareg;
	UINT		i;
	UINT16		reg;
	cpu_status former; 

	DBFENTER;

	/* Validate bap, offset, buf, and len */
	if ( (bap > 1) || 
	     (offset > HFA384x_BAP_OFFSET_MAX) || 
	     (offset % 2) ||
	     (buf == NULL) ||
	     (len > HFA384x_BAP_DATALEN_MAX) ){
	     	result = -EINVAL;
			dprintf(kDevName ": hfa384x_copy_from_bap ERR! bap=%d offset=%d len=%d\n", bap, offset, len);
	} else {


		former = disable_interrupts();
		acquire_spinlock( &(hw->baplock));


		selectreg = (bap == 1) ? 
			HFA384x_SELECT1(hw->iobase) : 
			HFA384x_SELECT0(hw->iobase) ;
		offsetreg = (bap == 1) ? 
			HFA384x_OFFSET1(hw->iobase) : 
			HFA384x_OFFSET0(hw->iobase) ;
		datareg =   (bap == 1) ? 
			HFA384x_DATA1(hw->iobase) : 
			HFA384x_DATA0(hw->iobase) ;
		/* Obtain lock */
		
		
		/* Write id to select reg */
		wlan_outw_cpu_to_le16(id, selectreg);
		/* Write offset to offset reg */
		wlan_outw_cpu_to_le16(offset, offsetreg);
		/* Wait for offset[busy] to clear (see BAP_TIMEOUT) */
		i = 0; 
		do {
			reg = wlan_inw_le16_to_cpu(offsetreg);
			if ( i > 0 ) spin(2);
			i++;
			if(i > 50000) {
				dprintf("holding spinlock tooooooo long in hfa384x_copy_from_bap()\n");
				result = B_ERROR;
				goto done;
			}
		} while ( i < BAP_TIMEOUT && HFA384x_OFFSET_ISBUSY(reg));
		if ( HFA384x_OFFSET_ISBUSY(reg) ){
			/* If timeout, return -ETIMEDOUT */
			result = reg;
		} else if ( HFA384x_OFFSET_ISERR(reg) ){
			/* If offset[err] == 1, return -EINVAL */
			result = reg;
		} else {
			result = 0;
			/* Read even(len) buf contents from data reg */
			
			for ( i = 0; i < (len & 0xfffe); i+=2 ) {
				*(UINT16*)(&(d[i])) = wlan_inw(datareg);
			}
			/* If len odd, handle last byte */
			if ( len % 2 ){
				reg = wlan_inw(datareg);
				d[len-1] = ((UINT8*)(&reg))[0];
			}
		}
		/* Release lock */
done:
		release_spinlock( &(hw->baplock));
		restore_interrupts(former);

#if 0
dprintf("copy_from_bap bap=%x id=%x len=%x offset=%x buf=%x\n",bap, id, len, offset, (unsigned int)buf);
	for (i=0; i<len; i++) {
		dprintf("%2.2x%c", *(unsigned char *) ((int)buf + (int)i ), ((i&0xf)==0xf)?'\n':' ');
	}
dprintf("\n");
#endif




	}
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* hfa384x_copy_to_bap
*
* Copies a collection of bytes to the MAC controller memory via
* one set of BAP registers.
*
* Arguments:
*	hw		device structure
*	bap		[0|1] which BAP to use
*	id		FID or RID, destined for the select register (host order)
*	offset		An _even_ offset into the buffer for the given
*			FID/RID.  We haven't the means to validate this,
*			so be careful. (host order)
*	buf		ptr to array of bytes
*	len		length of data to transfer (in bytes)
*
* Returns: 
*	0		success
*	>0		f/w reported failure - value of offset reg.
*	<0		driver reported error (timeout|bad arg)
*
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
int hfa384x_copy_to_bap(hfa384x_t *hw, UINT16 bap, UINT16 id, UINT16 offset,
				void *buf, UINT len)
{
	int		result = 0;
	UINT8		*d = (UINT8*)buf;
	UINT		selectreg;
	UINT		offsetreg;
	UINT		datareg;
	UINT		i;
	UINT16		reg;
	UINT16		savereg;
	cpu_status 	former;


	/* Validate bap, offset, buf, and len */
	if ( (bap > 1) || 
	     (offset > HFA384x_BAP_OFFSET_MAX) || 
	     (offset % 2) ||
	     (buf == NULL) ||
	     (len > HFA384x_BAP_DATALEN_MAX) ){
	     	result = -EINVAL;
			dprintf(kDevName ": hfa384x_copy_to_bap failed bap=%d offset=%d len=%d\n",
				bap, offset, len);
	} else {

#if 0
dprintf("copy_to_bap bap=%x id=%x len=%x offset=%x buf=%x\n",bap, id, len, offset, (unsigned int)buf);
	for (i=0; i<len; i++) {
		dprintf("%2.2x%c", *(unsigned char *) ((int)buf + (int)i ), ((i&0xf)==0xf)?'\n':' ');
	}
dprintf("\n");
#endif

		former = disable_interrupts();
		acquire_spinlock( &(hw->baplock));

		selectreg = (bap == 1) ? HFA384x_SELECT1(hw->iobase) : HFA384x_SELECT0(hw->iobase) ;
		offsetreg = (bap == 1) ? HFA384x_OFFSET1(hw->iobase) : HFA384x_OFFSET0(hw->iobase) ;
		datareg =   (bap == 1) ? HFA384x_DATA1(hw->iobase) : HFA384x_DATA0(hw->iobase) ;
		/* init command should be handled seperately - it taks 60 mS
		   spinlock should not be held that long */
		
		/* Write id to select reg */
		wlan_outw_cpu_to_le16(id, selectreg);
		spin(10);
		/* Write offset to offset reg */
		wlan_outw_cpu_to_le16(offset, offsetreg);
	
		/* this code needs to be checked for reasonable timeouts & spins */
		/* Wait for offset[busy] to clear (see BAP_TIMEOUT) */
		i = 0; 
		do {
			reg = wlan_inw_le16_to_cpu(offsetreg);
			if ( i > 0 ) spin(2);
			i++;
			if (i>50000) {
				dprintf("Holding Spin lock tooooo long in hfa384x_copy_to_bap\n");
				result = B_ERROR;
				goto done;
			}
		} while ( i < BAP_TIMEOUT && HFA384x_OFFSET_ISBUSY(reg));
		if ( HFA384x_OFFSET_ISBUSY(reg) ){
			/* If timeout, return reg */
			dprintf("copy_to_bap busy result=%x\n", reg);
			result = reg;
		} else if ( HFA384x_OFFSET_ISERR(reg) ){
			/* If offset[err] == 1, return reg */
			dprintf("copy_to_bap offset err result=%x\n", reg);
			result = reg;
		} else {
			result = 0;
			/* Write even(len) buf contents to data reg */
			for ( i = 0; i < (len & 0xfffe); i+=2 ) {
				wlan_outw( *(UINT16*)(&(d[i])), datareg);
			}
			/* If len odd, handle last byte */
			if ( len % 2 ){
				savereg = wlan_inw(datareg);
				wlan_outw_cpu_to_le16((offset+(len&0xfffe)), offsetreg);
				/* Wait for offset[busy] to clear (see BAP_TIMEOUT) */
				i = 0; 
				do {
					reg = wlan_inw_le16_to_cpu(offsetreg);
					if ( i > 0 ) spin(2);
					i++;
				} while ( i < BAP_TIMEOUT && HFA384x_OFFSET_ISBUSY(reg));
				if ( HFA384x_OFFSET_ISBUSY(reg) ){
					/* If timeout, return -ETIMEDOUT */
					result = reg;
					dprintf("copy_to_bap busy2 result=%x\n", result);
				} else {
					((UINT8*)(&savereg))[0] = d[len-1];
					wlan_outw( savereg, datareg);
				}
			}
		}
		/* Release lock */
done:
		release_spinlock(&(hw->baplock));
		restore_interrupts(former);
	}
//	dprintf("hfa384x_copy_to_bap returns %x\n", result);
	return result;
}


/*----------------------------------------------------------------
* hfa384x_copy_from_aux
*
* Copies a collection of bytes from the controller memory.  The
* Auxiliary port MUST be enabled prior to calling this function.
* We _might_ be in a download state.
*
* Arguments:
*	hw		device structure
*	cardaddr	address in hfa384x data space to read
*	buf		ptr to destination host buffer
*	len		length of data to transfer (in bytes)
*
* Returns: 
*	nothing
*
* Side effects:
*	buf contains the data copied
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
void hfa384x_copy_from_aux(hfa384x_t *hw, UINT32 cardaddr, void *buf, UINT len)
{
	UINT16		currpage;
	UINT16		curroffset;
	UINT		i = 0;

	DBFENTER;

	if ( !(hw->auxen) ) {
		dprintf(kDevName ": Attempt to read 0x%04lx when aux not enabled\n",cardaddr);
		return;
			
	}
	/* Build appropriate aux page and offset */
	currpage = HFA384x_AUX_MKPAGE(cardaddr);
	curroffset = HFA384x_AUX_MKOFF(cardaddr);
	wlan_outw_cpu_to_le16(currpage, HFA384x_AUXPAGE(hw->iobase));
	wlan_outw_cpu_to_le16(curroffset, HFA384x_AUXOFFSET(hw->iobase));
	spin(5);	/* beat */

	/* read the data */
	while ( i < len) {
		*((UINT16*)((char *)buf+i)) = wlan_inw(HFA384x_AUXDATA(hw->iobase));
		i+=2;
		curroffset+=2;
		if ( curroffset > HFA384x_AUX_OFF_MAX ) {
			currpage++;
			curroffset = 0;
			wlan_outw_cpu_to_le16(currpage, HFA384x_AUXPAGE(hw->iobase));
			wlan_outw_cpu_to_le16(curroffset, HFA384x_AUXOFFSET(hw->iobase));
			spin(5);	/* beat */
		}
	}

#if 0
dprintf("copy_from_aux cardaddr=%x buf=%x len=%x\n",cardaddr, (unsigned int)buf, len);
	for (i=0; i<len; i++) {
		dprintf("%2.2x%c", *(unsigned char *) ((int)buf + (int)i ), ((i&0xf)==0xf)?'\n':' ');
	}
dprintf("\n");
snooze(3000000); // give time for printout
#endif
	DBFEXIT;
}


/*----------------------------------------------------------------
* hfa384x_copy_to_aux
*
* Copies a collection of bytes to the controller memory.  The
* Auxiliary port MUST be enabled prior to calling this function.
* We _might_ be in a download state.
*
* Arguments:
*	hw		device structure
*	cardaddr	address in hfa384x data space to read
*	buf		ptr to destination host buffer
*	len		length of data to transfer (in bytes)
*
* Returns: 
*	nothing
*
* Side effects:
*	Controller memory now contains a copy of buf
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
void hfa384x_copy_to_aux(hfa384x_t *hw, UINT32 cardaddr, void *buf, UINT len)
{
	UINT16		currpage;
	UINT16		curroffset;
	UINT		i = 0;

	DBFENTER;

	if ( !(hw->auxen) ) {
		dprintf(kDevName ": Attempt to read 0x%04lx when aux not enabled\n",cardaddr);
		return;
	}

dprintf("copy_to_aux cardaddr=%lx buf=%p len=%lx\n",cardaddr, buf, (long unsigned int) len);
	for (i=0; i<len; i++) {
		dprintf("%2.2x%c", *(unsigned char *) ((int)buf + (int)i ), ((i&0xf)==0xf)?'\n':' ');
	}
dprintf("\n");
	i=0;


	/* Build appropriate aux page and offset */
	currpage = HFA384x_AUX_MKPAGE(cardaddr);
	curroffset = HFA384x_AUX_MKOFF(cardaddr);
	wlan_outw_cpu_to_le16(currpage, HFA384x_AUXPAGE(hw->iobase));
	wlan_outw_cpu_to_le16(curroffset, HFA384x_AUXOFFSET(hw->iobase));
	spin(5);	/* beat */

	/* write the data */
	while ( i < len) {
		wlan_outw( *((UINT16*)((char *)buf+i)), HFA384x_AUXDATA(hw->iobase));
		i+=2;
		curroffset+=2;
		if ( curroffset > HFA384x_AUX_OFF_MAX ) {
			currpage++;
			curroffset = 0;
			wlan_outw_cpu_to_le16(currpage, HFA384x_AUXPAGE(hw->iobase));
			wlan_outw_cpu_to_le16(curroffset, HFA384x_AUXOFFSET(hw->iobase));
			spin(5);	/* beat */
		}
	}
	DBFEXIT;
}


/*----------------------------------------------------------------
* hfa384x_cmd_wait
*
* Waits for availability of the Command register, then
* issues the given command.  Then polls the Evstat register
* waiting for command completion.  Timeouts shouldn't be 
* possible since we're preventing overlapping commands and all
* commands should be cleared and acknowledged.
*
* Arguments:
*	wlandev		device structure
*	cmd		Command in host order
*	parm0		Parameter0 in host order
*	parm1		Parameter1 in host order
*	parm2		Parameter2 in host order
*
* Returns: 
*	0		success
*	-1	timed out waiting for register ready or
*			command completion
*	1		command indicated error, Status and Resp0-2 are
*			in hw structure.
*
* Side effects:
*	
*
* Call context:
*	process thread 
----------------------------------------------------------------*/
int hfa384x_docmd_wait( hfa384x_t *hw, UINT16 cmd, UINT16 parm0, UINT16 parm1, UINT16 parm2)
{
	int		result=-1;
	#define 		WAIT_LIMIT 100000
	unsigned int 	limit = WAIT_LIMIT;
	unsigned short	reg = 0;
	bigtime_t		time;
	unsigned int	dt;
	cpu_status		former;


	time = system_time();

	/* acquire the spinlock - one command at a time */
	former = disable_interrupts();
	acquire_spinlock(&hw->cmdlock);
	/* wait for the busy bit to clear */	
	reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
	while ( HFA384x_CMD_ISBUSY(reg) && (--limit)) {
		reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
		spin(10);
	}
	if (limit != WAIT_LIMIT) {
		dprintf("docmd_wait: Busy on entry %d * 10=uS     cmd=%x\n", WAIT_LIMIT - limit, cmd);
		if (limit == 0)	{
			dprintf("docmd_wait: SPIN OUT!!!!!!!!!!!!!!!!\n");
			result = -1;
			goto done;
		}
	}
	/* busy bit clear, write command */
	wlan_outw_cpu_to_le16(parm0, HFA384x_PARAM0(hw->iobase));
	wlan_outw_cpu_to_le16(parm1, HFA384x_PARAM1(hw->iobase));
	wlan_outw_cpu_to_le16(parm2, HFA384x_PARAM2(hw->iobase));
	wlan_outw_cpu_to_le16(cmd, HFA384x_CMD(hw->iobase));
	hw->lastcmd = cmd;

	/* Now wait for completion */
	limit = WAIT_LIMIT * 20;
	reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
	while ( (!HFA384x_EVSTAT_ISCMD(reg)) && (--limit) ) {
		spin(10);
		reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
	}
	/* for development - remove later */

	if (limit == 0) {
		dprintf("docmd_wait cmd %x FAILURE - SPIN OUT!!!!!!\n", cmd);
		result = -1; 
		goto done;
	}	
	if ( HFA384x_EVSTAT_ISCMD(reg) ) {
		result = 0;
		hw->status = wlan_inw_le16_to_cpu(HFA384x_STATUS(hw->iobase));
		hw->resp0 = wlan_inw_le16_to_cpu(HFA384x_RESP0(hw->iobase));
		hw->resp1 = wlan_inw_le16_to_cpu(HFA384x_RESP1(hw->iobase));
		hw->resp2 = wlan_inw_le16_to_cpu(HFA384x_RESP2(hw->iobase));
		wlan_outw_cpu_to_le16(HFA384x_EVACK_CMD, 
			HFA384x_EVACK(hw->iobase));
		result = HFA384x_STATUS_RESULT_GET(hw->status);
	} 

done:
	release_spinlock(&hw->cmdlock);
	restore_interrupts(former);

	dt = system_time () - time;
//	dprintf("do_cmd_wait %x took %d uSec! result=%x\n", cmd , dt,result);

	return result;

}

#if defined(WLAN_STA)
/*----------------------------------------------------------------
* hfa384x_dl_cmd_wait
*
* Waits for availability of the Command register, then
* issues the given command.  Then polls the Evstat register
* waiting for command completion.  Timeouts shouldn't be 
* possible since we're preventing overlapping commands and all
* commands should be cleared and acknowledged.
*
* This routine is only used for downloads.  Since it doesn't lock out
* interrupts the system response is much better.
*
* Arguments:
*	wlandev		device structure
*	cmd		Command in host order
*	parm0		Parameter0 in host order
*	parm1		Parameter1 in host order
*	parm2		Parameter2 in host order
*
* Returns: 
*	0		success
*	-ETIMEDOUT	timed out waiting for register ready or
*			command completion
*	>0		command indicated error, Status and Resp0-2 are
*			in hw structure.
*
* Side effects:
*	
*
* Call context:
*	process thread 
----------------------------------------------------------------*/
int hfa384x_dl_docmd_wait( hfa384x_t *hw, UINT16 cmd, UINT16 parm0, UINT16 parm1, UINT16 parm2)
{
	int		result = -ETIMEDOUT;
	bigtime_t		timeout;
	UINT16			reg = 0;

	DBFENTER;
	/* wait for the busy bit to clear */	
	timeout = system_time() + 1000000;
	reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
	while ( HFA384x_CMD_ISBUSY(hfa384x2host_16(reg)) && 
	( system_time() <  timeout) ) {
		reg = wlan_inw_le16_to_cpu(HFA384x_CMD(hw->iobase));
	}
	if (!HFA384x_CMD_ISBUSY(hfa384x2host_16(reg))) {
		/* busy bit clear, write command */
		wlan_outw_cpu_to_le16(host2hfa384x_16(parm0), HFA384x_PARAM0(hw->iobase));
		wlan_outw_cpu_to_le16(host2hfa384x_16(parm1), HFA384x_PARAM1(hw->iobase));
		wlan_outw_cpu_to_le16(host2hfa384x_16(parm2), HFA384x_PARAM2(hw->iobase));
		hw->lastcmd = cmd;
		wlan_outw_cpu_to_le16(host2hfa384x_16(cmd), HFA384x_CMD(hw->iobase));

		/* Now wait for completion */
		if ( (HFA384x_CMD_CMDCODE_GET(hw->lastcmd) == HFA384x_CMDCODE_DOWNLD) ) { 
			/* dltimeout is in ms */
			timeout = hw->dltimeout * 1000;
			if ( timeout > 0 ) {
				timeout += system_time();
			} else {
				timeout = system_time() + 1000000;
			}
		} else {
			timeout = system_time() + 1000000;
		}
		reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
		while ( !HFA384x_EVSTAT_ISCMD(hfa384x2host_16(reg)) &&
		(system_time() <  timeout) ){
			spin(10);
			reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
		}
		if ( HFA384x_EVSTAT_ISCMD(hfa384x2host_16(reg)) ) {
			result = 0;
			hw->status = wlan_inw_le16_to_cpu(HFA384x_STATUS(hw->iobase));
			hw->status = hfa384x2host_16(hw->status);
			hw->resp0 = wlan_inw_le16_to_cpu(HFA384x_RESP0(hw->iobase));
			hw->resp0 = hfa384x2host_16(hw->resp0);
			hw->resp1 = wlan_inw_le16_to_cpu(HFA384x_RESP1(hw->iobase));
			hw->resp1 = hfa384x2host_16(hw->resp1);
			hw->resp2 = wlan_inw_le16_to_cpu(HFA384x_RESP2(hw->iobase));
			hw->resp2 = hfa384x2host_16(hw->resp2);
			wlan_outw_cpu_to_le16(host2hfa384x_16(HFA384x_EVACK_CMD), 
				HFA384x_EVACK(hw->iobase));
			result = HFA384x_STATUS_RESULT_GET(hw->status);
		}
	}

	DBFEXIT;
	return result;
}



/* some constants and macros */

#define PRISM2STA_MAGIC		(0x4a2d)
#define	INFOFRM_LEN_MAX		sizeof(hfa384x_ScanResults_t)


#define PRISM2_INFOBUF_MAX	(sizeof(hfa384x_HandoverAddr_t))
#define PRISM2_TXBUF_MAX	(sizeof(hfa384x_tx_frame_t) + \
				WLAN_DATA_MAXLEN - \
				WLAN_WEP_IV_LEN - \
				WLAN_WEP_ICV_LEN)



/*----------------------------------------------------------------
* prism2sta_initmac
*
* Issue the commands to get the MAC controller into its intial
* state.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	0	success
*	>0	f/w reported error
*	<0	driver reported error
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_initmac(ether_dev_info_t *device)
{
	int 			result = 0;
	prism2sta_priv_t	*priv = &device->priv;
	hfa384x_t		*hw = &device->hw;
	UINT16			reg;
	int			i;
	int			j;
	UINT8			snum[12];
	DBFENTER;



	/* call initialize */
	result = hfa384x_cmd_initialize(hw);
	if (result != 0) {
		WLAN_LOG_ERROR0("Initialize command failed.\n");
		goto failed;
	}

	/* Initialize FID stack */
	priv->txfid_lock=0;	/* initialize tx spinlock */
	priv->txfid_top = 0;
	memset(priv->txfid_stack, 0, sizeof(priv->txfid_stack));

	/* make sure interrupts are disabled and any layabout events cleared */
	wlan_outw_cpu_to_le16(0, HFA384x_INTEN(hw->iobase));
	wlan_outw_cpu_to_le16(0xffff, HFA384x_EVACK(hw->iobase));

	/* Read the PDA */
	result = hfa384x_drvr_readpda(hw, priv->pda, HFA384x_PDA_LEN_MAX);

	if ( result != 0) {
		WLAN_LOG_DEBUG0("drvr_readpda() failed\n");
		goto failed;
	}

	/* Allocate tx and notify FIDs */
	/* First, tx */
	for ( i = 0; i < PRISM2_FIDSTACKLEN_MAX; i++) {
		result = hfa384x_cmd_allocate(hw, PRISM2_TXBUF_MAX);
		if (result != 0) {
			WLAN_LOG_ERROR0("Allocate(tx) command failed.\n");
			goto failed;
		}
		j = 0;
		do {
			reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
			udelay(10);
			j++;
		} while ( !HFA384x_EVSTAT_ISALLOC(reg) && j < 50); /* 50 is timeout */
		if ( j >= 50 ) {
			WLAN_LOG_ERROR0("Timed out waiting for evalloc(tx).\n");
			result = -ETIMEDOUT;
			goto failed;
		}
		priv->txfid_stack[i] = wlan_inw_le16_to_cpu(HFA384x_ALLOCFID(hw->iobase));
		reg = HFA384x_EVACK_ALLOC_SET(1);
		wlan_outw_cpu_to_le16( reg, HFA384x_EVACK(hw->iobase));
		WLAN_LOG_DEBUG2("priv->txfid_stack[%d]=0x%04x\n",i,priv->txfid_stack[i]);
	}
	priv->txfid_top = PRISM2_FIDSTACKLEN_MAX;

	/* Now, the info frame fid */
	result = hfa384x_cmd_allocate(hw, PRISM2_INFOBUF_MAX);
	if (result != 0) {
		goto failed;
		WLAN_LOG_ERROR0("Allocate(tx) command failed.\n");
	}
	i = 0;
	do {
		reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
		udelay(10);
		i++;
	} while ( !HFA384x_EVSTAT_ISALLOC(reg) && i < 50); /* 50 is timeout */
	if ( i >= 50 ) {
		WLAN_LOG_ERROR0("Timed out waiting for evalloc(info).\n");
		result = -ETIMEDOUT;
		goto failed;
	}
	priv->infofid = wlan_inw_le16_to_cpu(HFA384x_ALLOCFID(hw->iobase));
	reg = HFA384x_EVACK_ALLOC_SET(1);
	wlan_outw_cpu_to_le16( reg, HFA384x_EVACK(hw->iobase));
	WLAN_LOG_DEBUG1("priv->infofid=0x%04x\n", priv->infofid);

	/* Collect version and compatibility info */
	/*  Some are critical, some are not */
	/* NIC identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_NICIDENTITY,
			&priv->ident_nic, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve NICIDENTITY\n");
		goto failed;
	}
//snooze(1000000); //give time for dprintfs
	/* get all the nic id fields in host byte order */
	priv->ident_nic.id = hfa384x2host_16(priv->ident_nic.id);
	priv->ident_nic.variant = hfa384x2host_16(priv->ident_nic.variant);
	priv->ident_nic.major = hfa384x2host_16(priv->ident_nic.major);
	priv->ident_nic.minor = hfa384x2host_16(priv->ident_nic.minor);

	WLAN_LOG_INFO4( "ident: nic h/w: id=0x%02x %d.%d.%d\n",
			priv->ident_nic.id, priv->ident_nic.major,
			priv->ident_nic.minor, priv->ident_nic.variant);

	/* Primary f/w identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRIIDENTITY,
			&priv->ident_pri_fw, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve PRIIDENTITY\n");
		goto failed;
	}

	/* get all the private fw id fields in host byte order */
	priv->ident_pri_fw.id = hfa384x2host_16(priv->ident_pri_fw.id);
	priv->ident_pri_fw.variant = hfa384x2host_16(priv->ident_pri_fw.variant);
	priv->ident_pri_fw.major = hfa384x2host_16(priv->ident_pri_fw.major);
	priv->ident_pri_fw.minor = hfa384x2host_16(priv->ident_pri_fw.minor);

	WLAN_LOG_INFO4( "ident: pri f/w: id=0x%02x %d.%d.%d\n",
			priv->ident_pri_fw.id, priv->ident_pri_fw.major,
			priv->ident_pri_fw.minor, priv->ident_pri_fw.variant);

	/* Station (Secondary?) f/w identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STAIDENTITY,
			&priv->ident_sta_fw, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve STAIDENTITY\n");
		goto failed;
	}

	/* get all the station fw id fields in host byte order */
	priv->ident_sta_fw.id = hfa384x2host_16(priv->ident_sta_fw.id);
	priv->ident_sta_fw.variant = hfa384x2host_16(priv->ident_sta_fw.variant);
	priv->ident_sta_fw.major = hfa384x2host_16(priv->ident_sta_fw.major);
	priv->ident_sta_fw.minor = hfa384x2host_16(priv->ident_sta_fw.minor);

	/* strip out the 'special' variant bits */
	priv->mm_mods = priv->ident_sta_fw.variant & (BIT14 | BIT15);
	priv->ident_sta_fw.variant &= ~((UINT16)(BIT14 | BIT15));

	if  ( priv->ident_sta_fw.id == 0x1f ) {
		WLAN_LOG_INFO4( 
			"ident: sta f/w: id=0x%02x %d.%d.%d\n",
			priv->ident_sta_fw.id, priv->ident_sta_fw.major,
			priv->ident_sta_fw.minor, priv->ident_sta_fw.variant);
	} else {
		WLAN_LOG_INFO4(
			"ident:  ap f/w: id=0x%02x %d.%d.%d\n",
			priv->ident_sta_fw.id, priv->ident_sta_fw.major,
			priv->ident_sta_fw.minor, priv->ident_sta_fw.variant);
	}
	switch (priv->mm_mods)
	{
		case MM_SAT_PCF: 
			WLAN_LOG_INFO0("     :  includes SAT PCF \n"); break;
		case MM_GCSD_PCF: 
			WLAN_LOG_INFO0("     :  includes GCSD PCF\n"); break;
		case MM_GCSD_PCF_EB: 
			WLAN_LOG_INFO0("     :  includes GCSD PCF + EB\n"); break;
	}

	/* Compatibility range, Modem supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_MFISUPRANGE,
			&priv->cap_sup_mfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve MFISUPRANGE\n");
		goto failed;
	}
//snooze(1000000); //give time for dprintfs

	/* get all the Compatibility range, modem interface supplier
	fields in byte order */
	priv->cap_sup_mfi.role = hfa384x2host_16(priv->cap_sup_mfi.role);
	priv->cap_sup_mfi.id = hfa384x2host_16(priv->cap_sup_mfi.id);
	priv->cap_sup_mfi.variant = hfa384x2host_16(priv->cap_sup_mfi.variant);
	priv->cap_sup_mfi.bottom = hfa384x2host_16(priv->cap_sup_mfi.bottom);
	priv->cap_sup_mfi.top = hfa384x2host_16(priv->cap_sup_mfi.top);

	WLAN_LOG_INFO5(
		"MFI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_sup_mfi.role, priv->cap_sup_mfi.id,
		priv->cap_sup_mfi.variant, priv->cap_sup_mfi.bottom,
		priv->cap_sup_mfi.top);

	/* Compatibility range, Controller supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CFISUPRANGE,
			&priv->cap_sup_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve CFISUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, controller interface supplier
	fields in byte order */
	priv->cap_sup_cfi.role = hfa384x2host_16(priv->cap_sup_cfi.role);
	priv->cap_sup_cfi.id = hfa384x2host_16(priv->cap_sup_cfi.id);
	priv->cap_sup_cfi.variant = hfa384x2host_16(priv->cap_sup_cfi.variant);
	priv->cap_sup_cfi.bottom = hfa384x2host_16(priv->cap_sup_cfi.bottom);
	priv->cap_sup_cfi.top = hfa384x2host_16(priv->cap_sup_cfi.top);

	WLAN_LOG_INFO5( 
		"CFI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_sup_cfi.role, priv->cap_sup_cfi.id,
		priv->cap_sup_cfi.variant, priv->cap_sup_cfi.bottom,
		priv->cap_sup_cfi.top);

	/* Compatibility range, Primary f/w supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRISUPRANGE,
			&priv->cap_sup_pri, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve PRISUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, primary firmware supplier
	fields in byte order */
	priv->cap_sup_pri.role = hfa384x2host_16(priv->cap_sup_pri.role);
	priv->cap_sup_pri.id = hfa384x2host_16(priv->cap_sup_pri.id);
	priv->cap_sup_pri.variant = hfa384x2host_16(priv->cap_sup_pri.variant);
	priv->cap_sup_pri.bottom = hfa384x2host_16(priv->cap_sup_pri.bottom);
	priv->cap_sup_pri.top = hfa384x2host_16(priv->cap_sup_pri.top);

	WLAN_LOG_INFO5(
		"PRI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_sup_pri.role, priv->cap_sup_pri.id,
		priv->cap_sup_pri.variant, priv->cap_sup_pri.bottom,
		priv->cap_sup_pri.top);
	
	/* Compatibility range, Station f/w supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STASUPRANGE,
			&priv->cap_sup_sta, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve STASUPRANGE\n");
		goto failed;
	}
//snooze(1000000); //give time for dprintfs

	/* get all the Compatibility range, station firmware supplier
	fields in byte order */
	priv->cap_sup_sta.role = hfa384x2host_16(priv->cap_sup_sta.role);
	priv->cap_sup_sta.id = hfa384x2host_16(priv->cap_sup_sta.id);
	priv->cap_sup_sta.variant = hfa384x2host_16(priv->cap_sup_sta.variant);
	priv->cap_sup_sta.bottom = hfa384x2host_16(priv->cap_sup_sta.bottom);
	priv->cap_sup_sta.top = hfa384x2host_16(priv->cap_sup_sta.top);

	if ( priv->cap_sup_sta.id == 0x04 ) {
		WLAN_LOG_INFO5(
		"STA:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_sup_sta.role, priv->cap_sup_sta.id,
		priv->cap_sup_sta.variant, priv->cap_sup_sta.bottom,
		priv->cap_sup_sta.top);
	} else {
		WLAN_LOG_INFO5(
		"AP:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_sup_sta.role, priv->cap_sup_sta.id,
		priv->cap_sup_sta.variant, priv->cap_sup_sta.bottom,
		priv->cap_sup_sta.top);
	}

	/* Compatibility range, primary f/w actor, CFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRI_CFIACTRANGES,
			&priv->cap_act_pri_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		dprintf("Failed to retrieve PRI_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, primary f/w actor, CFI supplier
	fields in byte order */
	priv->cap_act_pri_cfi.role = hfa384x2host_16(priv->cap_act_pri_cfi.role);
	priv->cap_act_pri_cfi.id = hfa384x2host_16(priv->cap_act_pri_cfi.id);
	priv->cap_act_pri_cfi.variant = hfa384x2host_16(priv->cap_act_pri_cfi.variant);
	priv->cap_act_pri_cfi.bottom = hfa384x2host_16(priv->cap_act_pri_cfi.bottom);
	priv->cap_act_pri_cfi.top = hfa384x2host_16(priv->cap_act_pri_cfi.top);
//snooze(1000000); //give time for dprintfs

	WLAN_LOG_INFO5(
		"PRI-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_act_pri_cfi.role, priv->cap_act_pri_cfi.id,
		priv->cap_act_pri_cfi.variant, priv->cap_act_pri_cfi.bottom,
		priv->cap_act_pri_cfi.top);
	
	/* Compatibility range, sta f/w actor, CFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_CFIACTRANGES,
			&priv->cap_act_sta_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		dprintf("Failed to retrieve STA_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, station f/w actor, CFI supplier
	fields in byte order */
	priv->cap_act_sta_cfi.role = hfa384x2host_16(priv->cap_act_sta_cfi.role);
	priv->cap_act_sta_cfi.id = hfa384x2host_16(priv->cap_act_sta_cfi.id);
	priv->cap_act_sta_cfi.variant = hfa384x2host_16(priv->cap_act_sta_cfi.variant);
	priv->cap_act_sta_cfi.bottom = hfa384x2host_16(priv->cap_act_sta_cfi.bottom);
	priv->cap_act_sta_cfi.top = hfa384x2host_16(priv->cap_act_sta_cfi.top);
//snooze(1000000); //give time for dprintfs

	WLAN_LOG_INFO5(
		"STA-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_act_sta_cfi.role, priv->cap_act_sta_cfi.id,
		priv->cap_act_sta_cfi.variant, priv->cap_act_sta_cfi.bottom,
		priv->cap_act_sta_cfi.top);

	/* Compatibility range, sta f/w actor, MFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_MFIACTRANGES,
			&priv->cap_act_sta_mfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		dprintf("Failed to retrieve STA_MFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, station f/w actor, MFI supplier
	fields in byte order */
	priv->cap_act_sta_mfi.role = hfa384x2host_16(priv->cap_act_sta_mfi.role);
	priv->cap_act_sta_mfi.id = hfa384x2host_16(priv->cap_act_sta_mfi.id);
	priv->cap_act_sta_mfi.variant = hfa384x2host_16(priv->cap_act_sta_mfi.variant);
	priv->cap_act_sta_mfi.bottom = hfa384x2host_16(priv->cap_act_sta_mfi.bottom);
	priv->cap_act_sta_mfi.top = hfa384x2host_16(priv->cap_act_sta_mfi.top);

	WLAN_LOG_INFO5(
		"STA-MFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_act_sta_mfi.role, priv->cap_act_sta_mfi.id,
		priv->cap_act_sta_mfi.variant, priv->cap_act_sta_mfi.bottom,
		priv->cap_act_sta_mfi.top);

	/* Serial Number */
	/*TODO: print this out as text w/ hex for nonprint */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_NICSERIALNUMBER,
			snum, 12);
	if ( !result ) {
		int i;
		WLAN_LOG_INFO0("Prism2 card SN: ");
		for ( i=0; i < 12; i++) {
			dprintf("%02x ", snum[i]);
		}
		dprintf("\n");
	} else {
		dprintf("Failed to retrieve Prism2 Card SN\n");
		goto failed;
	}
//snooze(1000000); //give time for dprintfs

	/* Collect the MAC address */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CNFOWNMACADDR, 
		device->mac_addr, WLAN_ADDR_LEN);
	if ( result != 0 ) {
		dprintf("Failed to retrieve mac address\n");
		goto failed;
	}

	/* Retrieve the maximum frame size */
	hfa384x_drvr_getconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	WLAN_LOG_DEBUG1("F/W default max frame data size=%d\n", reg);

	reg=WLAN_DATA_MAXLEN;
	hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	hfa384x_drvr_getconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	WLAN_LOG_DEBUG1("F/W max frame data size after set=%d\n", reg);

	/* TODO: Set any internally managed config items */
	
	/* Set swsupport regs to magic # for card presence detection */
	wlan_outw_cpu_to_le16( PRISM2STA_MAGIC, HFA384x_SWSUPPORT0(hw->iobase));

	goto done;
failed:
//snooze(1000000); //give time for dprintfs
	WLAN_LOG_ERROR1("Failed, result=%d\n", result);
done:
	DBFEXIT;
	return result;
}




/*----------------------------------------------------------------
* prism2sta_int_dtim
*
* Handles the DTIM early warning event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_dtim(ether_dev_info_t *device)
{
#if 0
	prism2sta_priv_t	*priv = &device->priv;
	hfa384x_t		*hw = &device->hw;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG0("DTIM event, currently unhandled.\n");
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_int_infdrop
*
* Handles the InfDrop event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_infdrop(ether_dev_info_t *device)
{
#if 0
	prism2sta_priv_t	*priv = &device->priv;
	hfa384x_t		*hw = &device->hw;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG0("Info frame dropped due to card mem low.\n");
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_int_info
*
* Handles the Info event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_info(ether_dev_info_t *device)
{
	hfa384x_t		*hw = &device->hw;
	UINT16			reg;
	hfa384x_InfFrame_t	inf;
	int			result;
	DBFENTER;
	/* Retrieve the FID */
	reg = wlan_inw_le16_to_cpu(HFA384x_INFOFID(hw->iobase));

	/* Retrieve the length */
	result = hfa384x_copy_from_bap( hw, 
		hw->bap, reg, 0, &inf.framelen, sizeof(UINT16));
	if ( result ) {
		WLAN_LOG_DEBUG3( "copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			reg, (unsigned int) sizeof(inf), result);
		goto failed;
	}
	inf.framelen = hfa384x2host_16(inf.framelen);

	/* Retrieve the rest */
	result = hfa384x_copy_from_bap( hw, 
		hw->bap, reg, sizeof(UINT16),
		&(inf.infotype), inf.framelen * sizeof(UINT16));
	if ( result ) {
		WLAN_LOG_DEBUG3("copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			reg, (unsigned int) sizeof(inf), result);
		goto failed;
	}
	inf.infotype = hfa384x2host_16(inf.infotype);
//	WLAN_LOG_DEBUG2("rx infframe, len=%d type=0x%02x\n", inf.framelen, inf.infotype);
	/* Dispatch */
	switch ( inf.infotype ) {
		case HFA384x_IT_HANDOVERADDR:
			prism2sta_inf_handover(device, &inf);
			break;
		case HFA384x_IT_COMMTALLIES:
			prism2sta_inf_tallies(device, &inf);
			break;
		case HFA384x_IT_SCANRESULTS:
			prism2sta_inf_scanresults(device, &inf);
			break;
		case HFA384x_IT_CHINFORESULTS:
			prism2sta_inf_chinforesults(device, &inf);
			break;
		case HFA384x_IT_LINKSTATUS:
			prism2sta_inf_linkstatus(device, &inf);
			break;
		case HFA384x_IT_ASSOCSTATUS:
			prism2sta_inf_assocstatus(device, &inf);
			break;
		case HFA384x_IT_AUTHREQ:
			prism2sta_inf_authreq(device, &inf);
			break;
		case HFA384x_IT_PSUSERCNT:
			prism2sta_inf_psusercnt(device, &inf);
			break;
		default:
			WLAN_LOG_WARNING1(
				"Unknown info type=0x%02x\n", inf.infotype);
			break;
	}

failed:
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_int_txexc
*
* Handles the TxExc event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_txexc(ether_dev_info_t *device) {

	hfa384x_t		*hw = &device->hw;
	UINT16			status;
	UINT16			fid;
	int				result = 0;


	fid = wlan_inw_le16_to_cpu(HFA384x_TXCOMPLFID(hw->iobase));
	result = hfa384x_copy_from_bap(hw, hw->bap, fid, 0, &status, sizeof(status));
	if ( result ) {
		dprintf(kDevName ":copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n",
			fid, (int) sizeof(status),  result);
		goto failed;
	}
	status = hfa384x2host_16(status);
	dprintf(kDevName ": TxExc status=0x%x.\n", status);
failed:
}


/*----------------------------------------------------------------
* prism2sta_int_rx
*
* Handles the Rx event.
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_rx(ether_dev_info_t *device)
{
	hfa384x_t				*hw = &device->hw;
	prism2sta_priv_t		*priv = &device->priv;
	wlandevice_t			*wlandev = &device->wlandev;
	hfa384x_rx_frame_t		rxdesc;
	uint16 					rxfid;
	int 					result;
	wlan_pb_t		*pb;


	/* Get the FID */
	rxfid = wlan_inw_le16_to_cpu(HFA384x_RXFID(hw->iobase));
	/* Get the descriptor (including headers) */


	result = hfa384x_copy_from_bap(hw, HFA384x_BAP_INT, rxfid, 0, &rxdesc, sizeof(rxdesc));
	if ( result ) {
		WLAN_LOG_DEBUG4(
			"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
			rxfid, 0, (unsigned int) sizeof(rxdesc), (unsigned int) result);
		goto failed;
	}

	/* Byte order convert once up front. */
	rxdesc.status =	hfa384x2host_16(rxdesc.status);
	rxdesc.time =	hfa384x2host_32(rxdesc.time);
	rxdesc.data_len = hfa384x2host_16(rxdesc.data_len);

	//dprintf("rxdesc.data_len=%x\n", rxdesc.data_len);
	/* Now handle frame based on port# */
	switch( HFA384x_RXSTATUS_MACPORT_GET(rxdesc.status) )
	{
	case 0:
		/* Allocate the buffer, note CRC (aka FCS). pballoc */
		/* assumes there needs to be space for one */

		pb = p80211pb_alloc_p80211(NULL, 
			rxdesc.data_len + WLAN_HDR_A3_LEN + WLAN_CRC_LEN+6);
		if ( pb == NULL ) {
			dprintf("read_hook: pballoc failed.\n");
			goto failed;
		}

		/* Copy the 802.11 hdr to the buffer */
		result = hfa384x_copy_from_bap(hw, HFA384x_BAP_INT, rxfid, 
			HFA384x_RX_80211HDR_OFF, pb->p80211_hdr, WLAN_HDR_A3_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG4( 
				"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
				rxfid, HFA384x_RX_80211HDR_OFF, WLAN_HDR_A3_LEN,result);
			goto failed_cleanup;
		}

		if (prism2sta_int_rx_typedrop(device, ieee2host16(pb->p80211_hdr->a3.fc))) {
			WLAN_LOG_WARNING0("Unhandled frame type, dropped.\n");
			goto failed_cleanup;
		}


		/* If exclude and we receive an unencrypted, drop it */
		if ( priv->exclude_unencrypt && 
			!WLAN_GET_FC_ISWEP(ieee2host16(pb->p80211_hdr->a3.fc))) {
			p80211pb_free(pb);
			dprintf(kDevName ": receive_hook excluded encrypted frame dropped %x\n",
				WLAN_GET_FC_ISWEP(ieee2host16(pb->p80211_hdr->a3.fc)));
			goto failed_cleanup;
		}

		/* Copy the payload data to the buffer */
		if ( rxdesc.data_len > 0 ) {
			result = hfa384x_copy_from_bap(hw, 
				HFA384x_BAP_INT, rxfid, HFA384x_RX_DATA_OFF+6, 
				pb->p80211_payload, rxdesc.data_len);
			if ( result ) {
				WLAN_LOG_DEBUG4(
					"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
					rxfid, HFA384x_RX_DATA_OFF, rxdesc.data_len,result);
				p80211pb_free(pb);
				goto failed_cleanup;
			}
		}	else { /* not rx data */
			goto failed_cleanup;
		}

		device->rx_received = (device->rx_received + 1) & (RX_BUFFERS-1);
		if ( device->rx_received  == device->rx_acked ) { /* no space left in ring? */
			dprintf("rx overflow - dropping frame %d\n", (int) device->rx_received);
			device->rx_received = (device->rx_received - 1) & (RX_BUFFERS-1);	/* back up modulo RX_BUFFERS */
			goto failed_cleanup;
		}
		pb->ethhostbuf = device->rx_buf[device->rx_received]; /* this is where the frame gets copied by p80211pb_p80211_to_ether */

		/* Set the length */
		pb->p80211frmlen = WLAN_HDR_A3_LEN + rxdesc.data_len + WLAN_CRC_LEN;

		if ( pb->p80211_payloadlen == 0 ) {
			dprintf(kDevName ":rx payloadlen 0 \n");
		} else if ( p80211pb_p80211_to_ether(wlandev, wlandev->ethconv, pb) == 0 ) {
			/* limit size */
			if (pb->p80211_payloadlen + 6 > 1514) {
//				dprintf(kDevName ": rx frame tooo big - truncating frame: %d bytes\n", pb->p80211_payloadlen);
				pb->p80211_payloadlen = 1514 - 6;
			}
			//dprintf("read_hook: buf=%x len=%x\n", buf, pb->p80211_payloadlen);
			device->rx_len[device->rx_received] = pb->p80211_payloadlen+6;          
			pb->ethhostbuf = NULL;
			pb->ethfree = NULL;
//			wlandev->linux_stats.rx_packets++;
        } else {
			WLAN_LOG_DEBUG0("p80211_to_ether failed.\n");
		}

		release_sem_etc(device->ilock,1, B_DO_NOT_RESCHEDULE);
		p80211pb_free(pb);

		break;
	default:
		WLAN_LOG_WARNING1("Received frame on unsupported port=%d\n",
			HFA384x_RXSTATUS_MACPORT_GET(rxdesc.status) );
		break;
	}
	goto done;

failed_cleanup:
	p80211pb_free(pb);
failed:
	device->readLock = 0;		
//	dprintf(kDevName ": rx int : failed\n");
	return;

done:
}


/*----------------------------------------------------------------
* prism2sta_int_rc_typedrop
*
* Classifies the frame, increments the appropriate counter, and
* returns 0|1 indicating whether the driver should handle or
* drop the frame
*
* Arguments:
*	wlandev		wlan device structure
*	fc		frame control field
*
* Returns: 
*	zero if the frame should be handled by the driver,
*	non-zero otherwise.
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
int prism2sta_int_rx_typedrop( ether_dev_info_t *device, UINT16 fc)
{
	UINT16	ftype;
	UINT16	fstype;
	wlandevice_t *wlandev = &device->wlandev;

	int	drop = 0;
	/* Classify frame, increment counter */
	ftype = WLAN_GET_FC_FTYPE(fc);
	fstype = WLAN_GET_FC_FSTYPE(fc);
	switch ( ftype ) {
	case WLAN_FTYPE_MGMT:
		WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd mgmt:");
		wlandev->rx.mgmt++;
		switch( fstype ) {
		case WLAN_FSTYPE_ASSOCREQ:
			dprintf("assocreq");
			wlandev->rx.assocreq++;
			break;
		case WLAN_FSTYPE_ASSOCRESP:
			dprintf("assocresp");
			wlandev->rx.assocresp++;
			break;
		case WLAN_FSTYPE_REASSOCREQ:
			dprintf("reassocreq");
			wlandev->rx.reassocreq++;
			break;
		case WLAN_FSTYPE_REASSOCRESP:
			dprintf("reassocresp");
			wlandev->rx.reassocresp++;
			break;
		case WLAN_FSTYPE_PROBEREQ:
			dprintf("probereq");
			wlandev->rx.probereq++;
			break;
		case WLAN_FSTYPE_PROBERESP:
			dprintf("proberesp");
			wlandev->rx.proberesp++;
			break;
		case WLAN_FSTYPE_BEACON:
			dprintf("beacon");
			wlandev->rx.beacon++;
			break;
		case WLAN_FSTYPE_ATIM:
			dprintf("atim");
			wlandev->rx.atim++;
			break;
		case WLAN_FSTYPE_DISASSOC:
			dprintf("disassoc");
			wlandev->rx.disassoc++;
			break;
		case WLAN_FSTYPE_AUTHEN:
			dprintf("authen");
			wlandev->rx.authen++;
			break;
		case WLAN_FSTYPE_DEAUTHEN:
			dprintf("deauthen");
			wlandev->rx.deauthen++;
			break;
		default:
			dprintf("unknown");
			wlandev->rx.mgmt_unknown++;
			break;
		}
		dprintf("\n");
		drop = 1;
		break;

	case WLAN_FTYPE_CTL:
		WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd ctl:");
		wlandev->rx.ctl++;
		switch( fstype ) {
		case WLAN_FSTYPE_PSPOLL:
			dprintf("pspoll");
			wlandev->rx.pspoll++;
			break;
		case WLAN_FSTYPE_RTS:
			dprintf("rts");
			wlandev->rx.rts++;
			break;
		case WLAN_FSTYPE_CTS:
			dprintf("cts");
			wlandev->rx.cts++;
			break;
		case WLAN_FSTYPE_ACK:
			dprintf("ack");
			wlandev->rx.ack++;
			break;
		case WLAN_FSTYPE_CFEND:
			dprintf("cfend");
			wlandev->rx.cfend++;
			break;
		case WLAN_FSTYPE_CFENDCFACK:
			dprintf("cfendcfack");
			wlandev->rx.cfendcfack++;
			break;
		default:
			dprintf("unknown");
			wlandev->rx.ctl_unknown++;
			break;
		}
		dprintf("\n");
		drop = 1;
		break;

	case WLAN_FTYPE_DATA:
		wlandev->rx.data++;
		switch( fstype ) {
		case WLAN_FSTYPE_DATAONLY:
			wlandev->rx.dataonly++;
			break;
		case WLAN_FSTYPE_DATA_CFACK:
			wlandev->rx.data_cfack++;
			break;
		case WLAN_FSTYPE_DATA_CFPOLL:
			wlandev->rx.data_cfpoll++;
			break;
		case WLAN_FSTYPE_DATA_CFACK_CFPOLL:
			wlandev->rx.data__cfack_cfpoll++;
			break;
		case WLAN_FSTYPE_NULL:
			WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd data:null\n");
			wlandev->rx.null++;
			break;
		case WLAN_FSTYPE_CFACK:
			WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd data:cfack\n");
			wlandev->rx.cfack++;
			break;
		case WLAN_FSTYPE_CFPOLL:
			WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd data:cfpoll\n");
			wlandev->rx.cfpoll++;
			break;
		case WLAN_FSTYPE_CFACK_CFPOLL:
			WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd data:cfack_cfpoll\n");
			wlandev->rx.cfack_cfpoll++;
			break;
		default:
			dprintf("unknown");
			wlandev->rx.data_unknown++;
			break;
		}

		break;
	}
	return drop;
}


/*----------------------------------------------------------------
* prism2sta_int_alloc
*
* Handles the Alloc event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_int_alloc(ether_dev_info_t *device)
{

	prism2sta_priv_t	*priv = &device->priv;
	hfa384x_t		*hw = &device->hw;


	/* Handle the reclaimed FID */
	/*   collect the FID and push it onto the stack */
	txfid_push(priv, wlan_inw_le16_to_cpu(HFA384x_ALLOCFID(hw->iobase)));
	
//	wlandev->netdev->tbusy = 0;
//	dprintf("PRISM: netdev->tbusy = 0; mark_bh(NET_BH)" );
	return;
}


/*----------------------------------------------------------------
* prism2sta_inf_handover
*
* Handles the receipt of a Handover info frame. Should only be present
* in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_handover(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
//	WLAN_LOG_DEBUG0("received infoframe:HANDOVER (unhandled)\n");
}


/*----------------------------------------------------------------
* prism2sta_inf_tallies
*
* Handles the receipt of a CommTallies info frame. 
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_tallies(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
//	WLAN_LOG_DEBUG0("received infoframe:COMMTALLIES (unhandled)\n");
}


/*----------------------------------------------------------------
* prism2sta_inf_scanresults
*
* Handles the receipt of a Scan Results info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_scanresults(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{

	hfa384x_t		*hw = &device->hw;
	int			nbss;
	hfa384x_ScanResult_t	*sr = &(inf->info.scanresult);
	int			i;
	hfa384x_JoinRequest_data_t	joinreq;
	int			result;
	DBFENTER;

	/* Get the number of results, first in bytes, then in results */
	nbss = (inf->framelen * sizeof(UINT16)) - 
		sizeof(inf->infotype) -
		sizeof(inf->info.scanresult.scanreason);
	nbss /= sizeof(hfa384x_ScanResultSub_t);

	/* Print em */
	WLAN_LOG_DEBUG2("rx scanresults, reason=%d, nbss=%d:\n",
		inf->info.scanresult.scanreason, nbss);
	for ( i = 0; i < nbss; i++) {
		WLAN_LOG_DEBUG4("chid=%d anl=%d sl=%d bcnint=%d\n",
			sr->result[i].chid,
			sr->result[i].anl,
			sr->result[i].sl,
			sr->result[i].bcnint);
		WLAN_LOG_DEBUG2("  capinfo=0x%04x proberesp_rate=%d\n",
			sr->result[i].capinfo,
			sr->result[i].proberesp_rate);
			device->signal_strength = sr->result[i].sl;
	}
	/* issue a join request */
	joinreq.channel = sr->result[0].chid;
	memcpy( joinreq.bssid, sr->result[0].bssid, WLAN_BSSID_LEN);
	result = hfa384x_drvr_setconfig( hw, 
			HFA384x_RID_JOINREQUEST,
			&joinreq, HFA384x_RID_JOINREQUEST_LEN);
	if (result) {
		WLAN_LOG_ERROR1("setconfig(joinreq) failed, result=%d\n", result);
	}
	
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_inf_chinforesults
*
* Handles the receipt of a Channel Info Results info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_chinforesults(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
	WLAN_LOG_DEBUG0("received infoframe:CHINFO (unhandled)\n");
}


/*----------------------------------------------------------------
* prism2sta_inf_linkstatus
*
* Handles the receipt of a Link Status info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_linkstatus(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
	wlandevice_t 		*wlandev = &device->wlandev;
	hfa384x_t		*hw = &device->hw;
	UINT16			portstatus;
	int			result;
	DBFENTER;
	/* Convert */
	inf->info.linkstatus.linkstatus = 
		hfa384x2host_16(inf->info.linkstatus.linkstatus);
	/* Handle */
	switch (inf->info.linkstatus.linkstatus) {
	case HFA384x_LINK_NOTCONNECTED:
		WLAN_LOG_DEBUG0("linkstatus=NOTCONNECTED (unhandled)\n");
		break;
	case HFA384x_LINK_CONNECTED:
		WLAN_LOG_DEBUG0("linkstatus=CONNECTED\n");
		/* Collect the BSSID, and set state to allow tx */
		result = hfa384x_drvr_getconfig(hw, 
				HFA384x_RID_CURRENTBSSID,
				wlandev->bssid, WLAN_BSSID_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG2(
				"getconfig(0x%02x) failed, result = %d\n",
				HFA384x_RID_CURRENTBSSID, result);
			goto failed;
		}

		/* Collect the port status */
		result = hfa384x_drvr_getconfig16(hw, 
				HFA384x_RID_PORTSTATUS, &portstatus);
		if ( result ) {
			WLAN_LOG_DEBUG2(
				"getconfig(0x%02x) failed, result = %d\n",
				HFA384x_RID_PORTSTATUS, result);
			goto failed;
		}
		portstatus = hfa384x2host_16(portstatus);
		wlandev->macmode = 
			portstatus == HFA384x_PSTATUS_CONN_IBSS ?
			WLAN_MACMODE_IBSS_STA : WLAN_MACMODE_ESS_STA;
		break;
	case HFA384x_LINK_DISCONNECTED:
		WLAN_LOG_DEBUG0("linkstatus=DISCONNECTED (unhandled)\n");
		break;
	case HFA384x_LINK_AP_CHANGE:
		WLAN_LOG_DEBUG0("linkstatus=AP_CHANGE (unhandled)\n");
		break;
	case HFA384x_LINK_AP_OUTOFRANGE:
		WLAN_LOG_DEBUG0("linkstatus=AP_OUTOFRANGE (unhandled)\n");
		break;
	case HFA384x_LINK_AP_INRANGE:
		WLAN_LOG_DEBUG0("linkstatus=AP_INRANGE (unhandled)\n");
		break;
	case HFA384x_LINK_ASSOCFAIL:
		WLAN_LOG_DEBUG0("linkstatus=ASSOCFAIL (unhandled)\n");
		break;
	default:
		WLAN_LOG_WARNING1( 
			"unknown linkstatus=0x%02x\n", inf->info.linkstatus.linkstatus);
		break;
	}

failed:
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_inf_assocstatus
*
* Handles the receipt of a Association Status info frame. Should 
* only be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_assocstatus(ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
	WLAN_LOG_DEBUG0("received infoframe:ASSOCSTATUS (unhandled)\n");
}


/*----------------------------------------------------------------
* prism2sta_inf_authreq
*
* Handles the receipt of a Authentication Request info frame. Should 
* only be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_authreq( ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
	WLAN_LOG_DEBUG0("received infoframe:AUTHREQ (unhandled)\n");
}


/*----------------------------------------------------------------
* prism2sta_inf_psusercnt
*
* Handles the receipt of a PowerSaveUserCount info frame. Should 
* only be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
static void prism2sta_inf_psusercnt( ether_dev_info_t *device, hfa384x_InfFrame_t *inf)
{
	WLAN_LOG_DEBUG0("received infoframe:PSUSERCNT (unhandled)\n");
}






#endif











