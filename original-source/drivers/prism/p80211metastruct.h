/* This file is GENERATED AUTOMATICALLY.  DO NOT EDIT OR MODIFY.
* --------------------------------------------------------------------
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

#ifndef _P80211MKMETASTRUCT_H
#define _P80211MKMETASTRUCT_H


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_mibget
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_unk264_t	mibattribute	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_mibget_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_mibset
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_unk264_t	mibattribute	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_mibset_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_powermgmt
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	powermgmtmode	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	wakeup	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	receivedtims	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_powermgmt_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_scan
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	bsstype	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	bssid	__WLAN_ATTRIB_PACK__;
	p80211item_pstr32_t	ssid	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	scantype	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	probedelay	__WLAN_ATTRIB_PACK__;
	p80211item_pstr14_t	channellist	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	minchanneltime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	maxchanneltime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	numbss	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_scan_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_scan_results
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	bssindex	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	bssid	__WLAN_ATTRIB_PACK__;
	p80211item_pstr32_t	ssid	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	bsstype	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	beaconperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dtimperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	timestamp	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	localtime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhdwelltime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhhopset	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhhoppattern	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhhopindex	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dschannel	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpcount	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpmaxduration	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpdurremaining	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	ibssatimwindow	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollreq	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	privacy	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate1	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate2	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate3	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate4	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate5	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate6	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate7	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate8	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_scan_results_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_join
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	bssid	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	joinfailuretimeout	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate1	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate2	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate3	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate4	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate5	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate6	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate7	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate8	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate1	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate2	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate3	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate4	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate5	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate6	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate7	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate8	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_join_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_authenticate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	authenticationtype	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	authenticationfailuretimeout	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_authenticate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_deauthenticate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	reasoncode	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_deauthenticate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_associate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	associatefailuretimeout	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollreq	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	privacy	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	listeninterval	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_associate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_reassociate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	newapaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dot11req_reassociatefailuretimeout	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollreq	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	privacy	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	listeninterval	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_reassociate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_disassociate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	reasoncode	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_disassociate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11req_start
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr32_t	ssid	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	bsstype	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	beaconperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dtimperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpperiod	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpmaxduration	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhdwelltime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhhopset	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	fhhoppattern	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dschannel	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	ibssatimwindow	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	probedelay	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	cfpollreq	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate1	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate2	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate3	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate4	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate5	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate6	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate7	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	basicrate8	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate1	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate2	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate3	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate4	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate5	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate6	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate7	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	operationalrate8	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11req_start_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11ind_authenticate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	authenticationtype	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11ind_authenticate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11ind_deauthenticate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	reasoncode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11ind_deauthenticate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11ind_associate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11ind_associate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11ind_reassociate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11ind_reassociate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_dot11ind_disassociate
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_pstr6_t	peerstaaddress	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	reasoncode	__WLAN_ATTRIB_PACK__;
} p80211msg_dot11ind_disassociate_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_lnxreq_wlansniff
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	enable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	channel	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_lnxreq_wlansniff_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_lnxind_wlansniffrm
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	hosttime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	mactime	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	channel	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	rssi	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	sq	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	signal	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	noise	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	rate	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	istx	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	frmlen	__WLAN_ATTRIB_PACK__;
} p80211msg_lnxind_wlansniffrm_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_readpda
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_unk1024_t	pda	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_readpda_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_auxport_state
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	enable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_auxport_state_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_auxport_read
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	len	__WLAN_ATTRIB_PACK__;
	p80211item_unk1024_t	data	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_auxport_read_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_auxport_write
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	len	__WLAN_ATTRIB_PACK__;
	p80211item_unk1024_t	data	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_auxport_write_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_test_command
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	testcode	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	testparam	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_test_command_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_mmi_read
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	value	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_mmi_read_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_mmi_write
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	data	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_mmi_write_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_ramdl_state
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	enable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	exeaddr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_ramdl_state_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_ramdl_write
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	len	__WLAN_ATTRIB_PACK__;
	p80211item_unk4096_t	data	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_ramdl_write_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_flashdl_state
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	enable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_flashdl_state_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_flashdl_write
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	addr	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	len	__WLAN_ATTRIB_PACK__;
	p80211item_unk4096_t	data	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_flashdl_write_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_mm_state
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	enable	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_mm_state_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_mm_dscpmap
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	dscp	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	macqueue	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_mm_dscpmap_t;
__WLAN_PRAGMA_PACKDFLT__


__WLAN_PRAGMA_PACK1__
typedef struct p80211msg_p2req_dump_state
{
	UINT32		msgcode	__WLAN_ATTRIB_PACK__;
	UINT32		msglen	__WLAN_ATTRIB_PACK__;
	UINT8		devname[WLAN_DEVNAMELEN_MAX]	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	level	__WLAN_ATTRIB_PACK__;
	p80211item_uint32_t	resultcode	__WLAN_ATTRIB_PACK__;
} p80211msg_p2req_dump_state_t;
__WLAN_PRAGMA_PACKDFLT__


#endif
