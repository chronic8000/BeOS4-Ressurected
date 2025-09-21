/* src/prism2/include/prism2/prism2mgmt.h
*
* Declares the mgmt command handler functions
* This file contains the constants and data structures for interaction
* with the hfa384x Wireless LAN (WLAN) Media Access Contoller (MAC).  
* The hfa384x is a portion of the Harris PRISM(tm) WLAN chipset.
*
* [Implementation and usage notes]
*
* [References]
*   CW10 Programmer's Manual v1.5
*   IEEE 802.11 D10.0
*
*    --------------------------------------------------------------------
*/

#ifndef _PRISM2MGMT_H
#define _PRISM2MGMT_H


/*=============================================================*/
/*------ Constants --------------------------------------------*/

#define	MAX_GRP_ADDR		(32)
#define	MAX_PRISM2_GRP_ADDR	(16)
#define P2_DSCP_MAX		(64)
#define PRISM2_FIDSTACKLEN_MAX	(10)

#define MM_SAT_PCF		(BIT14)
#define MM_GCSD_PCF		(BIT15)
#define MM_GCSD_PCF_EB		(BIT14 | BIT15)

/*=============================================================*/
/*------ Macros -----------------------------------------------*/


/*=============================================================*/
/*------ Types and their related constants --------------------*/

typedef struct prism2sta_priv
{

#if 0
	dev_node_t	node;

	/* Structure for MAC data */
	hfa384x_t	*hw;
#endif

	/* Component Identities */
	hfa384x_compident_t	ident_nic;
	hfa384x_compident_t	ident_pri_fw;
	hfa384x_compident_t	ident_sta_fw;
	hfa384x_compident_t	ident_ap_fw;
	UINT16			mm_mods;

	/* Supplier compatibility ranges */
	hfa384x_caplevel_t	cap_sup_mfi;
	hfa384x_caplevel_t	cap_sup_cfi;
	hfa384x_caplevel_t	cap_sup_pri;
	hfa384x_caplevel_t	cap_sup_sta;
	hfa384x_caplevel_t	cap_sup_ap;

	/* Actor compatibility ranges */
	hfa384x_caplevel_t	cap_act_pri_cfi; /* pri f/w to controller interface */
	hfa384x_caplevel_t	cap_act_sta_cfi; /* sta f/w to controller interface */
	hfa384x_caplevel_t	cap_act_sta_mfi; /* sta f/w to modem interface */
	hfa384x_caplevel_t	cap_act_ap_cfi;  /* ap f/w to controller interface */
	hfa384x_caplevel_t	cap_act_ap_mfi;  /* ap f/w to modem interface */

	/* PDA */
	UINT8			pda[HFA384x_PDA_LEN_MAX];
	hfa384x_pdrec_t		*pdrec[HFA384x_PDA_RECS_MAX];
	UINT			npdrec;

	/* MAC buffer ids */
	spinlock	txfid_lock;
	UINT		txfid_top;
	UINT16		txfid_stack[PRISM2_FIDSTACKLEN_MAX];
	UINT16		infofid;

	/* The following are dot11StationConfigurationTable mibitems
	maintained by the driver; there are no Prism2 RID's */
	UINT32		dot11_desired_bss_type;
	UINT32		dot11_disassoc_reason;
	UINT8		dot11_disassoc_station[WLAN_ADDR_LEN];
	UINT32		dot11_deauth_reason;
	UINT8		dot11_deauth_station[WLAN_ADDR_LEN];
	UINT32		dot11_auth_fail_status;
	UINT8		dot11_auth_fail_station[WLAN_ADDR_LEN];

	/* Group Addresses - right now, there are up to a total
	of MAX_GRP_ADDR group addresses */
	UINT8		dot11_grp_addr[MAX_GRP_ADDR][WLAN_ADDR_LEN];
	UINT		dot11_grpcnt;

	/* State variables */
	UINT		qos_enable;		/* C bool */
	UINT8		qos_dscpmap[P2_DSCP_MAX];
	UINT		priv_invoked;		/* C bool */
	UINT		exclude_unencrypt;	/* C bool */
} prism2sta_priv_t;


/*=============================================================*/
/*------ Static variable externs ------------------------------*/

extern int	prism2_debug;

/*---------------------------------------------------------------
* conversion functions going between wlan message data types and
* Prism2 data types 
---------------------------------------------------------------*/
/* byte area conversion functions*/
void prism2mgmt_pstr2bytearea(UINT8 *bytearea, p80211pstrd_t *pstr);
void prism2mgmt_bytearea2pstr(UINT8 *bytearea, p80211pstrd_t *pstr, int len);

/* byte string conversion functions*/
void prism2mgmt_pstr2bytestr(hfa384x_bytestr_t *bytestr, p80211pstrd_t *pstr);
void prism2mgmt_bytestr2pstr(hfa384x_bytestr_t *bytestr, p80211pstrd_t *pstr);

/* integer conversion functions */
void prism2mgmt_prism2int2p80211int(UINT16 *prism2int, UINT32 *wlanint);
void prism2mgmt_p80211int2prism2int(UINT16 *prism2int, UINT32 *wlanint);

/* enumerated integer conversion functions */
void prism2mgmt_prism2enum2p80211enum(UINT16 *prism2enum, UINT32 *wlanenum, UINT16 rid);
void prism2mgmt_p80211enum2prism2enum(UINT16 *prism2enum, UINT32 *wlanenum, UINT16 rid);

/* functions to convert a bit area to/from an Operational Rate Set */
void prism2mgmt_get_oprateset(UINT16 *rate, p80211pstrd_t *pstr);
void prism2mgmt_set_oprateset(UINT16 *rate, p80211pstrd_t *pstr);

/* functions to convert Group Addresses */
void prism2mgmt_get_grpaddr(UINT32 did, 
	p80211pstrd_t *pstr, prism2sta_priv_t *priv );
int prism2mgmt_set_grpaddr(UINT32 did, 
	UINT8 *prism2buf, p80211pstrd_t *pstr, prism2sta_priv_t *priv );
int prism2mgmt_get_grpaddr_index( UINT32 did );

/*=============================================================*/
/*--- Inline Function Definitions (if supported) --------------*/
/*=============================================================*/
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


#endif
