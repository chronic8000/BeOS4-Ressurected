/* src/include/wlan/p80211conv.h
*
* Ether/802.11 conversions and packet buffer routines
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
*
* This file declares the functions, types and macros that perform 
* Ethernet to/from 802.11 frame conversions.  Additionally,  the 
* functions for manipulating the wlan_pb_t packet buffer 
* structures are here.
*
* --------------------------------------------------------------------
*/

#ifndef _LINUX_P80211CONV_H

/*================================================================*/
/* Constants */

#define WLAN_ETHADDR_LEN	6
#define WLAN_IEEE_OUI_LEN	3

#define WLAN_ETHCONV_ENCAP	1
#define WLAN_ETHCONV_RFC1042	2
#define WLAN_ETHCONV_8021h	3

#define WLAN_MIN_ETHFRM_LEN	60
#define WLAN_MAX_ETHFRM_LEN	1518
#define WLAN_ETHHDR_LEN		16


/*================================================================*/
/* Macros */


/*================================================================*/
/* Types */

/* buffer free method pointer type */
typedef void (* freebuf_method_t)(void *buf, int size);

/* local ether header type */
typedef struct wlan_ethhdr
{
	UINT8	daddr[WLAN_ETHADDR_LEN]		__WLAN_ATTRIB_PACK__;
	UINT8	saddr[WLAN_ETHADDR_LEN]		__WLAN_ATTRIB_PACK__;
	UINT16	type						__WLAN_ATTRIB_PACK__;
} wlan_ethhdr_t;

/* local llc header type */
typedef struct wlan_llc
{
	UINT8	dsap						__WLAN_ATTRIB_PACK__;
	UINT8	ssap						__WLAN_ATTRIB_PACK__;
	UINT8	ctl							__WLAN_ATTRIB_PACK__;
} wlan_llc_t;

/* local snap header type */
typedef struct wlan_snap
{
	UINT8	oui[WLAN_IEEE_OUI_LEN]		__WLAN_ATTRIB_PACK__;
	UINT16	type						__WLAN_ATTRIB_PACK__;
} wlan_snap_t;

/*-------------------------------------------------------------
* NOTE: 
* TX wlan_pb_t containing ethfrm from OS:
* 	ethhostbuf	Points to a host OS TX buffer _structure_ 
*			(if it has one)	on hosts that use a raw byte 
*			array for their pkt	buffers, this and ethbuf 
*			will be the same.
*	ethbuf		Points to the raw byte array containing the
*			pkt to be transmitted.  May be longer than the
*			pkt itself.
*	ethbuflen 	The size of the raw byte array containing the pkt.
*	ethfrmlen	The size of the pkt in ethbuf.
*	eth_hdr		Points at the ethhdr in ethbuf
*	eth_llc		not used
*	eth_snap	not used
*	eth_payload	not used
*	eth_payloadlen	not used
*		
*	p80211hostbuf	Points to a host OS TX buffer _structure_
*			(if it has one).  On hosts that use a raw byte
*			array for pkt buffers, this and p80211buf
*			will be the same.
*	p80211buf	Points to the raw byte array containing the 
*			802.11 header and any additional llc/snap
*			headers required as a result of conversion.
*	p80211buflen	The size of the raw byte array containing the 802.11
*			header [+ llc [+ snap]].
*	p80211frmlen	The size of the 802.11 header [+ llc [+ snap]].
*	p80211_hdr	Points at the 802.11 header in p80211buf.
*	p80211_llc	Points at the llc header in p80211buf, if present.  
*			NULL otherwise.
*	p80211_snap	Points at the snap header in p80211buf, if present. 
*			NULL otherwise.
*	p80211_payload	Points at the portion of the ethbuf that we're
*			making the payload (after llc and snap) of 
*			the 802.11 frame.
*	p80211_payloadlen	Contains the size of the payload data we're
*			grabbing from the ethbuf.
*
* MSDs should use the rules above when accessing a frame handed down for TX
* by p80211.  
*
* TX wlan_pb_t from within the MSD (AP forwards, mgmt frames, or other MSD 
* generated frames):
*	ethhostbuf		not used
*	ethbuf			not used
*	ethbuflen 		not used
*	ethfrmlen		not used
*	eth_hdr			not used
*	eth_llc			not used
*	eth_snap		not used
*	eth_payload		not used
*	eth_payloadlen	not used
*		
*	p80211hostbuf	Points to a host OS TX buffer _structure_
*			(if it has one).  On hosts that use a raw byte
*			array for pkt buffers, this and p80211buf
*			will be the same.
*	p80211buf	Points to the raw byte array containing the 
*			received 802.11 frame.
*	p80211buflen	The size of the raw byte array containing the 802.11
*			frame.
*	p80211frmlen	The size of the 802.11 frame.
*	p80211_hdr	Points at the 802.11 header in p80211buf.
*	p80211_llc	not used
*	p80211_snap	not used
*	p80211_payload	Points at the first byte after the 802.11 mac
*			header in p80211buf
*	p80211_payloadlen	Contains the size of the payload data in the 
*			802.11 frame.  Does NOT include the wep IV, wep ICV,
*			or FCS.
*
* RX wlan_pb_t:
*	ethhostbuf	Points to a host OS TX buffer _structure_ 
*			(if it has one)	on hosts that use a raw byte 
*			array for their pkt buffers, this and ethbuf 
*			will be the same.
*	ethbuf		Points to the raw byte array containing the
*			pkt to be transmitted.  May be longer than the
*			pkt itself.
*	ethbuflen 	The size of the raw byte array containing the pkt.
*	ethfrmlen	The size of the pkt in ethbuf.
*	eth_hdr		Points at the ethhdr in ethbuf
*	eth_llc		Points at the location in ethbuf where an llc header 
*			might be.
*	eth_snap	Points at the location in ethbuf where a snap header
*			might  be.
*	eth_payload	Points at the first byte after the ethhdr in ethbuf.
*
*	p80211hostbuf	Points to a host OS TX buffer _structure_
*			(if it has one).  On hosts that use a raw byte
*			array for pkt buffers, this and p80211buf
*			will be the same.
*	p80211buf	Points to the raw byte array containing the 
*			received 802.11 frame.
*	p80211buflen	The size of the raw byte array containing the 802.11
*			frame.
*	p80211frmlen	The size of the 802.11 frame.
*	p80211_hdr	Points at the 802.11 header in p80211buf.
*	p80211_llc	Points at the location in p80211buf where an llc
*			header might be.
*	p80211_snap	Points at the location in p80211buf where a snap
*			header might be.
*	p80211_payload	Points at the first byte after the 802.11 mac
*			header in p80211buf
*	p80211_payloadlen	Contains the size of the payload data in the 
*			802.11 frame.  Does NOT include the wep IV, wep ICV,
*			or FCS.
*
* NOTES:
* - The two TX types are distiguished by the value of the
*   ethhostbuf member.  Internal TX pb's will set ethhostbuf
*   to NULL.
* - The iscrypt field indicates the encryption status of a
*   802.11 frame in a pb.  If a frame is received encrypted
*   iscrypt should remain true until the frame is decrypted.
*   If an outgoing frame is being encrypted, iscrypt should
*   be set after the encryption is complete.  The ISWEP bit
*   of the frame header should mirror the value of iscrypt.
* - The wep_iv and wep_icv fields are stored in IEEE byte
*   order (little endian).
* - The free methods are ptrs to functions that will deal
*   with the deallocation of the ethhostbuf and p80211hostbuf.
*   They are called by the pbfree function.
-------------------------------------------------------------*/

typedef struct wlan_pb
{
	freebuf_method_t	ethfree;
	void			*ethhostbuf;		
	UINT8			*ethbuf;
	UINT			ethbuflen;
	UINT			ethfrmlen;
	wlan_ethhdr_t		*eth_hdr;
	wlan_llc_t		*eth_llc;
	wlan_snap_t		*eth_snap;
	UINT8			*eth_payload;
	UINT			eth_payloadlen;

	freebuf_method_t	p80211free;
	void			*p80211hostbuf;
	UINT8			*p80211buf;
	UINT			p80211buflen;
	UINT			p80211frmlen;
	p80211_hdr_t		*p80211_hdr;
	wlan_llc_t		*p80211_llc;
	wlan_snap_t		*p80211_snap;
	UINT8			*p80211_payload;
	UINT			p80211_payloadlen;

	UINT			wep_iscrypt;
	UINT32			wep_iv;
	UINT32			wep_icv;
} wlan_pb_t;

/* Circular include trick */
struct wlandevice;

/*================================================================*/
/* Externs */

/*================================================================*/
/*Function Declarations */

int p80211pb_ether_to_p80211( struct wlandevice *wlandev, 
				UINT32 ethconv, wlan_pb_t *pb);
int p80211pb_p80211_to_ether( struct wlandevice *wlandev, 
				UINT32 ethconv, wlan_pb_t *pb);
void p80211pb_freeskb( void *buf, int size);
void p80211pb_kfree_s( void *buf, int size);
wlan_pb_t* p80211pb_alloc(void);
wlan_pb_t* p80211pb_alloc_p80211(wlan_pb_t *pb, UINT len);
void p80211pb_free(wlan_pb_t* pb);
int p80211_stt_findproto(UINT16 proto);
int p80211_stt_addproto(UINT16 proto);

#endif
