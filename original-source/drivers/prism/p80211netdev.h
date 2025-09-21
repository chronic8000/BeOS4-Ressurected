/*
* This file declares the structure type that represents each wlan
* interface.  
*
* --------------------------------------------------------------------
*/



#ifndef _P80211NETDEV_
#define _P80211NETDEV_


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
//#include <ether_driver.h>
#include <driver_settings.h>

#define WLAN_DEVICE_CLOSED	0
#define WLAN_DEVICE_OPEN	1

#define WLAN_MACMODE_NONE	0
#define WLAN_MACMODE_IBSS_STA	1
#define WLAN_MACMODE_ESS_STA	2
#define WLAN_MACMODE_ESS_AP	3


/* Received frame statistics */
typedef struct p80211_frmrx_t
{
	uint32	mgmt;
	uint32	assocreq;
	uint32	assocresp;
	uint32	reassocreq;
	uint32	reassocresp;
	uint32	probereq;
	uint32	proberesp;
	uint32	beacon;
	uint32	atim;
	uint32	disassoc;
	uint32	authen;
	uint32	deauthen;
	uint32	mgmt_unknown;
	uint32	ctl;
	uint32	pspoll;
	uint32	rts;
	uint32	cts;
	uint32	ack;
	uint32	cfend;
	uint32	cfendcfack;
	uint32	ctl_unknown;
	uint32	data;
	uint32	dataonly;
	uint32	data_cfack;
	uint32	data_cfpoll;
	uint32	data__cfack_cfpoll;
	uint32	null;
	uint32	cfack;
	uint32	cfpoll;
	uint32	cfack_cfpoll;
	uint32	data_unknown;
} p80211_frmrx_t;


/* WLAN device type */
typedef struct wlandevice
{
	char		*name;	/* Dev name, assigned in call to register_wlandev */
	uint32		state;	/* Device Interface state (open/closed) */

	/* Hardware config */
	unsigned int		irq;
	unsigned int		iobase;
	unsigned int		membase;

	/* Config vars */
	unsigned int		ethconv;

	/* 802.11 State */
	unsigned char		bssid[WLAN_BSSID_LEN];
	uint32				macmode;

	/* Request/Confirm i/f state (used by p80211) */
	sem_id 				request_pending;  /* one at a time */
	p80211msg_t			*curr_msg;

	void 				*netdev;	/* ptr to prism device struct */


#if 0
	wait_queue_head_t	reqwq;

	/* Indication i/f state */
		/* netlink socket */
		/* queue for indications waiting for cmd completion */
	/* Linux netdevice and support */
	netdevice_t		*netdev;	/* ptr to linux netdevice */
//	struct net_device_stats linux_stats;



	void				*priv;		/* private data for MSD */
	struct wlandevice	*next;		/* link for list of devices */
#endif

	/* 802.11 device statistics */
	
	struct p80211_frmrx_t	rx;

} wlandevice_t;


/*================================================================*/
/* Externs */

/*================================================================*/
/* Function Declarations */

void	p80211netdev_startup(void);
int	wlan_setup(wlandevice_t *wlandev);
int	wlan_unsetup(wlandevice_t *wlandev);
int	register_wlandev(wlandevice_t *wlandev);
int	unregister_wlandev(wlandevice_t *wlandev);
void	p80211netdev_rx(wlandevice_t *wlandev, wlan_pb_t *pb);





/*
* src/include/wlan/p80211netdev.h
*
* WLAN net device structure and functions
*
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

#endif
