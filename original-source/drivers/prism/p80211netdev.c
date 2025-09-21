/* p80211knetdev.c
*
*  Kernel net device interface
*
* The functions required fo network device are defined here.
*
* --------------------------------------------------------------------
*/



/*================================================================*/
/* Project Includes */

#include "p80211types.h"
#include "p80211hdr.h"
#include "p80211conv.h"
#include "p80211mgmt.h"
#include "p80211msg.h"
#include "p80211netdev.h"
#include "p80211ioctl.h"
#include "p80211req.h"

/*================================================================*/
/* Local Constants */

#define MAX_WLAN_DEVICES	4	/* At most 3 non-intefering DS cards */

/*================================================================*/
/* Local Macros */


/*================================================================*/
/* Local Types */

/*================================================================*/
/* Local Static Definitions */

static wlandevice_t	*wlandev_index[MAX_WLAN_DEVICES];

/*================================================================*/
/* Local Function Declarations */

/* Support functions */
static int wlandev_get_index(wlandevice_t  *wlandev);
static void wlandev_clear_index(wlandevice_t  *wlandev);

/* netdevice method functions */
static int p80211knetdev_init( netdevice_t *netdev);
static struct net_device_stats* p80211knetdev_get_stats(netdevice_t *netdev);
static int p80211knetdev_open( netdevice_t *netdev);
static int p80211knetdev_stop( netdevice_t *netdev );
static int p80211knetdev_hard_start_xmit( struct sk_buff *skb, netdevice_t *netdev);
static void p80211knetdev_set_multicast_list(netdevice_t *dev);
static int p80211knetdev_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);

/*================================================================*/
/* Function Definitions */

/*----------------------------------------------------------------
* p80211knetdev_startup
*
* Initialize the wlandevice/netdevice part of 802.11 services at 
* load time.
*
* Arguments:
*	none
*
* Returns: 
*	nothing
----------------------------------------------------------------*/
void p80211netdev_startup(void)
{
	DBFENTER;

	memset( wlandev_index, 0, sizeof(wlandev_index));

	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* p80211knetdev_init
*
* Init method for a Linux netdevice.  Called in response to
* register_netdev.
*
* Arguments:
*	none
*
* Returns: 
*	nothing
----------------------------------------------------------------*/
int p80211knetdev_init( netdevice_t *netdev)
{
	DBFENTER;
	/* Called in response to register_netdev */
	/* This is usually the probe function, but the probe has */
	/* already been done by the MSD and the create_kdev */
	/* function.  All we do here is return success */
	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* p80211knetdev_get_stats
*
* Statistics retrieval for linux netdevices.  Here we're reporting
* the Linux i/f level statistics.  Hence, for the primary numbers,
* we don't want to report the numbers from the MIB.  Eventually,
* it might be useful to collect some of the error counters though.
*
* Arguments:
*	netdev		Linux netdevice
*
* Returns: 
*	the address of the statistics structure
----------------------------------------------------------------*/
struct net_device_stats*
p80211knetdev_get_stats(netdevice_t *netdev)
{
	wlandevice_t	*wlandev = (wlandevice_t*)netdev->priv;
	DBFENTER;

	/* TODO: review the MIB stats for items that correspond to 
		linux stats */

	DBFEXIT;
	return &(wlandev->linux_stats);
}


/*----------------------------------------------------------------
* p80211knetdev_open
*
* Linux netdevice open method.  Following a successful call here,
* the device is supposed to be ready for tx and rx.  In our
* situation that may not be entirely true due to the state of the
* MAC below.
*
* Arguments:
*	netdev		Linux network device structure
*
* Returns: 
*	zero on success, non-zero otherwise
----------------------------------------------------------------*/
int p80211knetdev_open( netdevice_t *netdev )
{
	int 		result = 0; /* success */
	wlandevice_t	*wlandev = (wlandevice_t*)(netdev->priv);

	DBFENTER;

	/* Tell the MSD to open */
	if ( wlandev->open != NULL) {
		result = (*(wlandev->open))(wlandev);
		if ( result == 0 ) {
#if ( LINUX_VERSION_CODE < WLAN_KVERSION(2,3,43) )
			/* set the device flags */
			netdev->tbusy = 0;
			netdev->interrupt = 0;
			netdev->start = 1;
#else
			netif_start_queue(netdev);
#endif
		}
	} else {
		result = -EAGAIN;
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* p80211knetdev_stop
*
* Linux netdevice stop (close) method.  Following this call,
* no frames should go up or down through this interface.
*
* Arguments:
*	netdev		Linux network device structure
*
* Returns: 
*	zero on success, non-zero otherwise
----------------------------------------------------------------*/
int p80211knetdev_stop( netdevice_t *netdev )
{
	int		result = 0;
	wlandevice_t	*wlandev = (wlandevice_t*)(netdev->priv);

	DBFENTER;

	if ( wlandev->close != NULL ) {
		result = (*(wlandev->close))(wlandev);
	}

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
	/* To make sure noone tries to send us something, just force */
	/*  tbusy to true */
	netdev->tbusy = 1;
	netdev->start = 0;
#else
	netif_stop_queue(netdev);
#endif

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* p80211netdev_rx
*
* Frame receive function called by the mac specific driver.
*
* Arguments:
*	wlandev		WLAN network device structure
*	pb		WLAN packet buffer containing an 802.11
*			frame.
* Returns: 
*	nothing
* Side effects:
*	The pb is freed by this function, caller shouldn't access
*	it after this call.
----------------------------------------------------------------*/
void p80211netdev_rx(wlandevice_t *wlandev, wlan_pb_t *pb)
{
	netdevice_t	*dev = wlandev->netdev;
        struct sk_buff  *skb;

        DBFENTER;

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
        if (dev->start) {
#else
	if (dev->flags & IFF_UP ) {
#endif
		if ( pb->p80211_payloadlen == 0 ) {
			/* Do nothing, converting and passing up zero 
			   length frame is pointless 
			*/
		} else if ( p80211pb_p80211_to_ether(wlandev, wlandev->ethconv, pb) == 0 ) {
                        /* Mark last reception */
                        dev->last_rx = jiffies;
                        /* take ownership of skb from pb */
                        skb = (struct sk_buff*)pb->ethhostbuf;
                        pb->ethhostbuf = NULL;
                        pb->ethfree = NULL;
                        skb->dev = dev;
                        skb->protocol = eth_type_trans( skb, dev);
			wlandev->linux_stats.rx_packets++;
                        netif_rx(skb);
                } else {
			WLAN_LOG_DEBUG0(1, "p80211_to_ether failed.\n");
		}
        }

        p80211pb_free(pb);

        DBFEXIT;
} 


/*----------------------------------------------------------------
* p80211knetdev_hard_start_xmit
*
* Linux netdevice method for transmitting a frame.
*
* Arguments:
*	skb	Linux sk_buff containing the frame.
*	netdev	Linux netdevice.
*
* Side effects:
*	If the lower layers report that buffers are full. netdev->tbusy
*	will be set to prevent higher layers from sending more traffic.
*
*	Note: If this function returns non-zero, higher layers retain
*	      ownership of the skb.
*
* Returns: 
*	zero on success, non-zero on failure.
----------------------------------------------------------------*/
int p80211knetdev_hard_start_xmit( struct sk_buff *skb, netdevice_t *netdev)
{
	int		result = 0;
	int		txresult = -1;
	wlan_pb_t	*pb;
	wlandevice_t	*wlandev = (wlandevice_t*)netdev->priv;

	DBFENTER;

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
        if (netdev->start == 0) {
#else
	if ((netdev->flags & IFF_UP) == 0 ) {
#endif
		return 1;
	}

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
	if ( test_and_set_bit(0, (void*)&(netdev->tbusy)) != 0 ) {
		/* We've been called w/ tbusy set, has the tx */
		/* path stalled?   */
		WLAN_LOG_DEBUG0(1, "called when tbusy set\n");
		return 1;
	} 

#else
	netif_stop_queue(netdev);

	/* No timeout handling here, 2.3.38+ kernels call the 
	   timeout function directly.
	*/
#endif

	/* particularly handy in the above case */
	netdev->trans_start = jiffies;

	/* Check to see that a valid mode is set */
	switch( wlandev->macmode ) {
	case WLAN_MACMODE_IBSS_STA: 
	case WLAN_MACMODE_ESS_STA:
	case WLAN_MACMODE_ESS_AP:
		break;
	default:
		/* Mode isn't set yet, just drop the frame and return success */
		/* TODO: we need a saner way to handle this */
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
		netdev->tbusy = 0;
#else
		netif_wake_queue(netdev);
#endif
		dev_kfree_skb(skb);
		WLAN_LOG_NOTICE0("Tx attempt prior to association, frame dropped.\n");
		return 0;
		break;
	}
		
	/* OK, now we setup the ether to 802.11 conversion */
	pb = p80211pb_alloc();
	if ( pb == NULL ) {
		return 1;
	}

	pb->ethhostbuf = skb;
	pb->ethfree = p80211pb_freeskb;
	pb->ethbuf = skb->data;
	pb->ethbuflen = skb->len > 1514 ? 1514 : skb->len;
	pb->ethfrmlen = pb->ethbuflen;
	pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;

	if ( p80211pb_ether_to_p80211(wlandev, wlandev->ethconv, pb) != 0 ) {
		/* convert failed */
		WLAN_LOG_DEBUG1(1, 
			"ether_to_80211(%d) failed.\n", 
			wlandev->ethconv);
		/* Free the pb, but not the skb */
		pb->ethhostbuf = NULL;
		p80211pb_free(pb);
		result = 1;
	} else {
		if ( wlandev->txframe == NULL ) {
			return 1;
		}
		wlandev->linux_stats.tx_packets++;

		txresult = (*(wlandev->txframe))(wlandev, pb);

		if ( txresult == 0) {
  				/* success and more buf */
			/* avail, re: hw_txdata */
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			netdev->tbusy = 0;
#else
			netif_wake_queue(netdev);
#endif
			result = 0;
		} else if ( txresult == 1 ) { 
			/* success, no more avail */
			WLAN_LOG_DEBUG0(3, "txframe success, no more bufs\n");
			/* netdev->tbusy = 1;  don't set here, irqhdlr */
			/*   may have already cleared it */
			result = 0;
		} else if ( txresult == 2 ) { 
			/* alloc failure, drop frame */
			WLAN_LOG_DEBUG0(3, "txframe returned alloc_fail\n");
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			netdev->tbusy = 0;
#else
			netif_wake_queue(netdev);
#endif
			/* Free the pb, but not the skb */
			pb->ethhostbuf = NULL;
			p80211pb_free(pb);
			result = 1;
		} else { 
			/* buffer full or queue busy */
			WLAN_LOG_DEBUG0(3, "txframe returned full or busy\n");
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			netdev->tbusy = 0;
#else
			netif_wake_queue(netdev);
#endif
			/* Free the pb, but not the skb */
			pb->ethhostbuf = NULL;
			p80211pb_free(pb);
			result = 1;
		}
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* p80211knetdev_set_multicast_list
*
* Called from higher lavers whenever there's a need to set/clear
* promiscuous mode or rewrite the multicast list.
*
* Arguments:
*	none
*
* Returns: 
*	nothing
----------------------------------------------------------------*/
void p80211knetdev_set_multicast_list(netdevice_t *dev)
{
	DBFENTER;
	
	/* TODO: Construct the appropriate MIB set command */
	/*  and invoke MIB set via the macmgmt i/f */

	/* Since there's no promisc in the MIB, we need a custom */
	/* i/f for set/clear promisc, function or linux MIB? */

	DBFEXIT;
}


/*----------------------------------------------------------------
* p80211knetdev_do_ioctl
*
* Handle an ioctl call on one of our devices.  Everything Linux
* ioctl specific is done here.  Then we pass the contents of the
* ifr->data to the request message handler.
*
* Arguments:
*	dev	Linux kernel netdevice
*	ifr	Our private ioctl request structure, typed for the
*		generic struct ifreq so we can use ptr to func
*		w/o cast.
*
* Returns: 
*	zero on success, a negative errno on failure.  Possible values:
*		-EBUSY	cmd already in progress
*		-ETIME	p80211 cmd timed out (MSD may have its own timers)
*		-EFAULT memory fault copying msg from user buffer
*		-ENOMEM unable to allocate kernel msg buffer
*		-ENOSYS	bad magic, it the cmd really for us?
*		-EINTR	sleeping on cmd, awakened by signal, cmd cancelled.
*
* Call Context:
*	Process thread (ioctl caller).  TODO: SMP support may require
*	locks.
----------------------------------------------------------------*/
int p80211knetdev_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	int			result = 0;
	p80211ioctl_req_t	*req = (p80211ioctl_req_t*)ifr;
	wlandevice_t		*wlandev = (wlandevice_t*)dev->priv;
	UINT8			*msgbuf;
	DBFENTER;

	WLAN_LOG_DEBUG2(2, "rx'd ioctl, cmd=%d, len=%d\n", cmd, req->len);

	
	/* Test the magic, assume ifr is good if it's there */
	if ( req->magic != P80211_IOCTL_MAGIC ) {
		return -ENOSYS;
	}

	if ( cmd == P80211_IFTEST ) {
		return 0;
	} else if ( cmd != P80211_IFREQ ) {
		return -ENOSYS;
	}

	/* Allocate a buf of size req->len */
	if ((msgbuf = kmalloc( req->len, GFP_KERNEL))) {
		if ( copy_from_user( msgbuf, req->data, req->len) ) {
			result = -EFAULT;
		} else {
			result = p80211req_dorequest( wlandev, msgbuf);
		}

		if ( result == 0 ) {
			if ( copy_to_user( req->data, msgbuf, req->len)) {
				result = -EFAULT;
			}
		}
	} else {
		result = -ENOMEM;
	}

	DBFEXIT;
	return result; /* If allocate,copyfrom or copyto fails, return errno */
}


/*----------------------------------------------------------------
* wlan_setup
*
* Roughly matches the functionality of ether_setup.  Here
* we set up any members of the wlandevice structure that are common
* to all devices.  Additionally, we allocate a linux 'struct device'
* and perform the same setup as ether_setup.
*
* Note: It's important that the caller have setup the wlandev->name
*	ptr prior to calling this function.
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	zero on success, non-zero otherwise.
* Call Context:
*	Should be process thread.  We'll assume it might be
*	interrupt though.  When we add support for statically
*	compiled drivers, this function will be called in the 
*	context of the kernel startup code.
----------------------------------------------------------------*/
int wlan_setup(wlandevice_t *wlandev)
{
	int		result = 0;
	netdevice_t	*dev;

	DBFENTER;

	if (wlandev->name == NULL ) {
		WLAN_LOG_ERROR0("called without wlandev->name set.\n");
		result = 1;
	} else {
		/* Set up the wlandev */
		wlandev->state = WLAN_DEVICE_CLOSED;
		wlandev->ethconv = WLAN_ETHCONV_RFC1042;
		wlandev->macmode = WLAN_MACMODE_NONE;

		init_waitqueue_head(&wlandev->reqwq);

		/* Allocate and initialize the struct device */
		dev = kmalloc(sizeof(netdevice_t), GFP_ATOMIC);
		if ( dev == NULL ) {
			WLAN_LOG_ERROR0("Failed to alloc netdev.\n");
			result = 1;
		} else {
			memset( dev, 0, sizeof(netdevice_t));
			wlandev->netdev = dev;
			dev->priv = wlandev;
			dev->hard_start_xmit =	&p80211knetdev_hard_start_xmit;
			dev->get_stats =	&p80211knetdev_get_stats;
			dev->do_ioctl = 	&p80211knetdev_do_ioctl;
			dev->set_multicast_list = &p80211knetdev_set_multicast_list;
			dev->init =		&p80211knetdev_init;
			dev->open =		&p80211knetdev_open;
			dev->stop =		&p80211knetdev_stop;

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			dev->tbusy = 1;
#endif
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,99) )
			dev->name = wlandev->name;
#endif
			ether_setup(dev);
		}
	}

	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* wlan_unsetup
*
* This function is paired with the wlan_setup routine.  It should
* be called after unregister_wlandev.  Basically, all it does is
* free the 'struct device' that's associated with the wlandev.
* We do it here because the 'struct device' isn't allocated 
* explicitly in the driver code, it's done in wlan_setup.  To
* do the free in the driver might seem like 'magic'.
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	zero on success, non-zero otherwise.
* Call Context:
*	Should be process thread.  We'll assume it might be
*	interrupt though.  When we add support for statically
*	compiled drivers, this function will be called in the 
*	context of the kernel startup code.
----------------------------------------------------------------*/
int wlan_unsetup(wlandevice_t *wlandev)
{
	int		result = 0;

	DBFENTER;

	if (wlandev->netdev == NULL ) {
		WLAN_LOG_ERROR0("called without wlandev->netdev set.\n");
		result = 1;
	} else {
		kfree_s(wlandev->netdev, sizeof(netdevice_t));
		wlandev->netdev = NULL;
	}

	DBFEXIT;
	return 0;
}



/*----------------------------------------------------------------
* register_wlandev
*
* Roughly matches the functionality of register_netdev.  This function
* is called after the driver has successfully probed and set up the
* resources for the device.  It's now ready to become a named device
* in the Linux system.
*
* First we allocate a name for the device (if not already set), then
* we call the Linux function register_netdevice.
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	zero on success, non-zero otherwise.
* Call Context:
*	Can be either interrupt or not.
----------------------------------------------------------------*/
int register_wlandev(wlandevice_t *wlandev)
{
	int		i = -1;
	netdevice_t	*dev = wlandev->netdev;

	DBFENTER;
	rtnl_lock();

	if ( dev->name != NULL && 
		(dev->name[0] == '\0' || dev->name[0] == ' ') ) {
		i = wlandev_get_index(wlandev);
	}

#if ( LINUX_VERSION_CODE >= WLAN_KVERSION(2,3,99) )
	strcpy(dev->name, wlandev->name);
#endif

	if (register_netdevice(dev)) {
		if ( i >= 0 ) {
			wlandev_clear_index(wlandev);
		}
		rtnl_unlock();
		return -EIO;
	}

	rtnl_unlock();
	MOD_INC_USE_COUNT;
	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* unregister_wlandev
*
* Roughly matches the functionality of unregister_netdev.  This
* function is called to remove a named device from the system.
*
* First we tell linux that the device should no longer exist.
* Then we remove it from the list of known wlan devices.
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	zero on success, non-zero otherwise.
* Call Context:
*	Can be either interrupt or not.
----------------------------------------------------------------*/
int unregister_wlandev(wlandevice_t *wlandev)
{
	DBFENTER;
	rtnl_lock();
	unregister_netdevice(wlandev->netdev);
	wlandev_clear_index(wlandev);
	rtnl_unlock();
	MOD_DEC_USE_COUNT;
	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* wlandev_get_index
*
* Allocates a device number and constructs the name for the given 
* wlandev.  
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	The device number on success, -1 otherwise
* Side effects:
*	The name is constructed in the space pointed to by wlandev->name.
*	It had _better_ be a valid pointer.
* Call Context:
*	Can be either interrupt or not.
----------------------------------------------------------------*/
int wlandev_get_index(wlandevice_t  *wlandev)
{
	int	i;

	DBFENTER;
	for  ( i = 0; i < MAX_WLAN_DEVICES; i++) {
		if ( wlandev_index[i] == NULL ) {
			sprintf(wlandev->name, "wlan%d", i);
			WLAN_LOG_DEBUG1(1,"Loading device '%s'...\n", wlandev->name);
			wlandev_index[i] = wlandev;
			return i;
		}
	}
	DBFEXIT;
	return -1;
}


/*----------------------------------------------------------------
* wlandev_clear_index
*
* Frees a previously allocated device number.
*
* Arguments:
*	wlandev		ptr to the wlandev structure for the
*			interface.
* Returns: 
*	nothing
* Side effects:
*	none
* Call Context:
*	Can be either interrupt or not.
----------------------------------------------------------------*/
void wlandev_clear_index(wlandevice_t  *wlandev)
{
	int	i;
	DBFENTER;
	for  ( i = 0; i < MAX_WLAN_DEVICES; i++) {
		if ( wlandev_index[i] == wlandev ) {
			wlandev_index[i] = NULL;
		}
	}
	DBFEXIT;
	return;
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