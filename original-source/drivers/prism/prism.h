
/* prism.h
   Copyright 2001 Be Inc., All rights reserved.
   by hank@sackett.net
*/

#ifndef __prism__
#define __psism__

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

#include "hfa384x.h"
#include "prism2mgmt.h"
#include "p80211netdev.h"
#include "fclock.h"

/*
 *PCMCIA, ISA, PCI bus related code
 */
/* driver does port io */
#define write8(address, value)			(*pci->write_io_8)((address), (value))
#define write16(address, value)			(*pci->write_io_16)((address), (value))
#define write32(address, value)			(*pci->write_io_32)((address), (value))
#define read8(address)					((*pci->read_io_8)(address))
#define read16(address)					((*pci->read_io_16)(address))
#define read32(address)					((*pci->read_io_32)(address))


#define BAP_TIMEOUT	5000	/* timeout is approx. BAP_TIMEOUT*2 us */

// A few details needed for WEP (Wireless Equivalent Privacy)
#define MAX_KEY_SIZE 16                 // 128 bits
#define MIN_KEY_SIZE  5                 // 40 bits RC4 - WEP
#define MAX_KEYS      4                 // 4 different keys
#define IW_ESSID_MAX_SIZE	100
#define MAX_STRING_SIZE	100





#define RX_BUFFERS 			16		/* MUST be a power of 2 */
#define TX_BUFFERS			4
#define BUFFER_SIZE			2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE 		1514

/* per driver instance globals */
typedef struct
{
	uchar			*rx_buf[RX_BUFFERS];	/*receive buffer - no alignemt issues, declared first anywar */
	ushort			rx_len[RX_BUFFERS];
	area_id			rx_buf_area;			/* Receive Descriptor and Buffer Areas  */
	volatile int32	rx_received, rx_acked;	/* rx frame indexes: rx_received for ISR; rx_acked for read_hook */

	sem_id			ilock;					/* read semaphore */
	sem_id			write_lock;				/* re-entrant write protection */
	fc_lock			olock;					/* write semapbore */
	int32			readLock, writeLock;	/* reentrant reads/writes not allowed */

	area_id			cookieID;  				/* Cookie Area ID           */
	int32			devID;    				/* Device ID                */
	uint32			addr;      				/* Base Address of HostRegs */
	pci_info		*pciInfo;  				/* PCI Informations         */
	dev_link_t		*pcmcia_info;			/* for pcmcia preinit... */
	ushort			irq;					/* device interrupt line */
	uchar			mac_addr[6];   			/* Ethernet Address */

	uchar			key[MAX_KEY_SIZE];		/* WEP encryption key */
	uchar			default_key_index;
	uchar 			wep_invoked;			/* encryption turned on? */
	char  			network_name[IW_ESSID_MAX_SIZE+1];	/* SSID	*/
	uint16 			roam_mode;			
	uint8   		tx_thresh;
	
	uint8   		rx_thresh;
	uint16 			cmd;
	uint8   		interrupted;

	uint8		 	duplex_full;			/* Full-Duplex Operation Requested. */
	uint8			duplex_lock;

	int32			inrw;					/* In Read or Write Function          */
	int				blockFlag;
	uint32			debug;					/* global debug flag */

	wlandevice_t		wlandev;
	hfa384x_t			hw;
	prism2sta_priv_t	priv;
	
	int32 			signal_strength;		/* Current signal strength */
#if 0
	char  station_name[IW_ESSID_MAX_SIZE+1];	/* Name of station			*/
	int32 port_type;							/* Port-type				*/
	int32 channel;								/*	Channel [3]				*/
	int32 ap_density;							/* AP density [1]			*/
	int32 medium_reservation;					/* RTS threshold			*/
	int32 frag_threshold;						/* Fragmentation threshold 	*/
	int32 transmit_rate;						/* Transmit rate			*/
	char  key[MAX_KEY_SIZE+2];					/* 40 bit security key + "0x"*/
#endif
	
} ether_dev_info_t;

#define ETHER_DEBUG(device, flags, args...) if (device->debug & flags) dprintf(args)

/* for serial debug command*/
#define DEBUGGER_COMMAND true




/*=============================================================*/
/*---prism2 station Function Declarations -----------------------------------*/
/*=============================================================*/

int prism2sta_initmac(ether_dev_info_t *wlandev);

int prism2mgmt_mibset_mibget(ether_dev_info_t *wlandev, void *msgp);
int prism2mgmt_powermgmt(void *msgp);
int prism2mgmt_scan(void *msgp);
int prism2mgmt_scan_results(void *msgp);
int prism2mgmt_join( void *msgp);
int prism2mgmt_authenticate( void *msgp);
int prism2mgmt_deauthenticate(void *msgp);
int prism2mgmt_associate(ether_dev_info_t *wlandev, void *msgp);
int prism2mgmt_reassociate( void *msgp);
int prism2mgmt_disassociate( void *msgp);
int prism2mgmt_start(ether_dev_info_t *device, void *msgp);
int prism2mgmt_readpda(ether_dev_info_t *device, void *msgp);
int prism2mgmt_auxport_state(ether_dev_info_t *device, void *msgp);
int prism2mgmt_auxport_read(ether_dev_info_t *device, void *msgp);
int prism2mgmt_auxport_write(ether_dev_info_t *device, void *msgp);
int prism2mgmt_test_command(ether_dev_info_t *device, void *msgp);
int prism2mgmt_mmi_read(ether_dev_info_t *device, void *msgp);
int prism2mgmt_mmi_write(ether_dev_info_t *device, void *msgp);
int prism2mgmt_ramdl_state(ether_dev_info_t *device, void *msgp);
int prism2mgmt_ramdl_write(ether_dev_info_t *device, void *msgp);
int prism2mgmt_flashdl_state(ether_dev_info_t *device, void *msgp);
int prism2mgmt_flashdl_write(ether_dev_info_t *device, void *msgp);
int prism2mgmt_mm_state(ether_dev_info_t *device, void *msgp);
int prism2mgmt_mm_dscpmap(ether_dev_info_t *device, void *msgp);
int prism2mgmt_dump_state(ether_dev_info_t *device, void *msgp);

void prism2sta_int_dtim(ether_dev_info_t *device);
void prism2sta_int_infdrop(ether_dev_info_t *device);
void prism2sta_int_info(ether_dev_info_t *device);
void prism2sta_int_txexc(ether_dev_info_t *device);
void prism2sta_int_tx(ether_dev_info_t *device);
void prism2sta_int_rx(ether_dev_info_t *device);
int prism2sta_int_rx_typedrop( ether_dev_info_t *device, UINT16 fc);
void prism2sta_int_alloc(ether_dev_info_t *device);




#endif /* __prism__ */

