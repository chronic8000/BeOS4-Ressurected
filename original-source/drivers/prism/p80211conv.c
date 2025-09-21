/* src/p80211/p80211conv.c
*
* Ether/802.11 conversions and packet buffer routines
*
*/

/*================================================================*/
/* Project Includes */
#include "wlan_compat.h"
#include "p80211types.h"
#include "p80211hdr.h"
#include "p80211conv.h"
#include "p80211mgmt.h"
#include "p80211msg.h"
#include "p80211netdev.h"
#include "p80211ioctl.h"
#include "p80211req.h"
#include "prism.h"
#include "net_data.h"

/*================================================================*/
/* Local Static Definitions */

static uint8	oui_rfc1042[] = {0x00, 0x00, 0x00};
static uint8	oui_8021h[] = {0x00, 0x00, 0xf8};


/*----------------------------------------------------------------
* p80211pb_ether_to_80211
*
* Uses the contents of the ether frame and the etherconv setting
* to build the elements of the 802.11 frame.  
*
* We don't actually set 
* up the frame header here.  That's the MAC's job.  We're only handling
* conversion of DIXII or 802.3+LLC frames to something that works
* with 802.11.
*
* Assume the following are already set:
*   pb->ethfree
*   pb->ethhostbuf
*   pb->ethbuf;
*   pb->ethbuflen
*   pb->eth_hdr
*	returns: zero on success, non-zero on failure
*
* Arguments:
*	ethconv		Conversion type to perform
*	pb		Packet buffer containing the ether frame
*
* Returns: 
*	0 on success, non-zero otherwise
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
int p80211pb_ether_to_p80211( wlandevice_t *wlandev, UINT32 ethconv, wlan_pb_t *pb)
{
	UINT16	proto;
	UINT16	fc;
	UINT8	*a1 = NULL;
	UINT8	*a2 = NULL;
	UINT8	*a3 = NULL;
	
	ether_dev_info_t *device = (ether_dev_info_t *) wlandev->netdev;
	DBFENTER;

	if ( ethconv == WLAN_ETHCONV_ENCAP ) { /* simplest case */
		/* here, we don't care what kind of ether frm. Just stick it */
		/*  in the 80211 payload */
		dprintf("prism: p80211bp_ether_to_p80211 ENCAP\n");
		pb->p80211hostbuf = new_buf( WLAN_HDR_A3_LEN);
		if ( pb->p80211hostbuf == NULL ) {
			return 1;
			WLAN_LOG_DEBUG0("Failed to alloc hostbuf1\n");
		}
		pb->p80211buflen = WLAN_HDR_A3_LEN;
		pb->p80211free = p80211pb_kfree_s;
		pb->p80211buf = (UINT8*)(pb->p80211hostbuf);
		pb->p80211_hdr = (p80211_hdr_t*)pb->p80211buf;
		pb->p80211_payload = pb->ethbuf;
		pb->p80211_payloadlen = pb->ethbuflen;
	} else {
		/* step 1: classify ether frame, DIX or 802.3? */
		proto = ntohs(pb->eth_hdr->type);
		if ( proto <= 1500 ) { /* codes <= 1500 reserved for 802.3 lengths */
			dprintf("prism: p80211bp_ether_to_p80211 802.3\n");
			/* it's 802.3, pass ether payload unchanged,  */
			/*   leave off any PAD octets.  */
			pb->p80211hostbuf = new_buf( WLAN_HDR_A3_LEN );
			if ( pb->p80211hostbuf == NULL ) {
				WLAN_LOG_DEBUG0("Failed to alloc hostbuf2\n");
				return 1;
			}
			pb->p80211buflen = WLAN_HDR_A3_LEN;
			pb->p80211free = p80211pb_kfree_s;
			pb->p80211buf = (UINT8*)(pb->p80211hostbuf);
			pb->p80211_hdr = (p80211_hdr_t*)pb->p80211buf;

			/* setup the payload ptrs */
			pb->p80211_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
			pb->p80211_payloadlen = ntohs(pb->eth_hdr->type);
		} else {
			/* it's DIXII, time for some conversion */
//dprintf("prism : p80211bp_ether_to_p80211 DIXII\n");
			pb->p80211hostbuf =
				new_buf(WLAN_HDR_A3_LEN+sizeof(wlan_llc_t)+sizeof(wlan_snap_t));
			if ( pb->p80211hostbuf == NULL ) {
				WLAN_LOG_DEBUG0("Failed to alloc hostbuf3\n");
				return 1;
			}
			pb->p80211buflen = 
						WLAN_HDR_A3_LEN + 
						sizeof(wlan_llc_t) +
						sizeof(wlan_snap_t);
			pb->p80211free = p80211pb_kfree_s;
			pb->p80211buf = (UINT8*)pb->p80211hostbuf;
			pb->p80211_hdr = (p80211_hdr_t*)pb->p80211buf;
			pb->p80211_llc = (wlan_llc_t*)(pb->p80211buf + WLAN_HDR_A3_LEN);
			pb->p80211_snap = (wlan_snap_t*)(((UINT8*)pb->p80211_llc) + sizeof(wlan_llc_t));

			/* setup the LLC header */
			pb->p80211_llc->dsap = 0xAA;	/* SNAP, see IEEE 802 */
			pb->p80211_llc->ssap = 0xAA;
			pb->p80211_llc->ctl = 0x03;
		
			/* setup the SNAP header */
			pb->p80211_snap->type = htons(proto);
			if ( ethconv == WLAN_ETHCONV_8021h && 
				 p80211_stt_findproto(proto) ) {
				memcpy( pb->p80211_snap->oui, oui_8021h, WLAN_IEEE_OUI_LEN);
			} else {
				memcpy( pb->p80211_snap->oui, oui_rfc1042, WLAN_IEEE_OUI_LEN);
			}

			/* setup the payload ptrs */
			pb->p80211_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
			pb->p80211_payloadlen = pb->ethbuflen - sizeof(wlan_ethhdr_t);

		}
	}

	/* Set up the 802.11 header */
	/* It's a data frame */
	fc = host2ieee16( WLAN_SET_FC_FTYPE(WLAN_FTYPE_DATA) |  
			WLAN_SET_FC_FSTYPE(WLAN_FSTYPE_DATAONLY));
	pb->p80211_hdr->a3.dur = 0;
	pb->p80211_hdr->a3.seq = 0;


//dprintf("p80211conv: p80211pb_ether_to_p80211: check mac_addr == dev_addr? %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
//	 device->mac_addr[0],device->mac_addr[1],device->mac_addr[2],
//	 device->mac_addr[3],device->mac_addr[4],device->mac_addr[5]);
//dprintf("p80211conv: p80211pb_ether_to_p80211: mac_mode=%x\n",wlandev->macmode);

	switch ( wlandev->macmode ) {
	case WLAN_MACMODE_IBSS_STA: 
		a1 = pb->eth_hdr->daddr;	/*dest*/
		a2 = device->mac_addr;	/*src*/
		a3 = wlandev->bssid;		/*bssid*/
		break;
	case WLAN_MACMODE_ESS_STA:
		fc |= host2ieee16(WLAN_SET_FC_TODS(1));
		a1 = wlandev->bssid;		/*bssid*/
		a2 = device->mac_addr;	/*src*/
		a3 = pb->eth_hdr->daddr;	/*dest*/
		break;
	case WLAN_MACMODE_ESS_AP:
		fc |= host2ieee16(WLAN_SET_FC_FROMDS(1));
		a1 = pb->eth_hdr->daddr;
		a2 = wlandev->bssid;
		a3 = pb->eth_hdr->saddr;
		break;
	default:
		WLAN_LOG_ERROR0("Error: Converting eth to wlan in unknown mode.\n");
		return 1;
		break;
	}
	pb->p80211_hdr->a3.fc = fc;
	memcpy( pb->p80211_hdr->a3.a1, a1, WLAN_ADDR_LEN); 
	memcpy( pb->p80211_hdr->a3.a2, a2, WLAN_ADDR_LEN);
	memcpy( pb->p80211_hdr->a3.a3, a3, WLAN_ADDR_LEN);

	/* Note that lower layers may set more bits in the fc field */

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* p80211pb_80211_to_ether
*
* Uses the contents of a received 802.11 frame and the etherconv 
* setting to build an ether frame.
*
* This function extracts the src and dest address from the 802.11
* frame to use in the construction of the eth frame.
*
* This function _will_ set the ethfree member.  If a caller wants
* to allow some other component (a higher layer?) take responsiblity
* for freeing the ethhostbuf, the caller should set ethfree to NULL.
*
* Assume the following are already set:
*	pb->p80211free
*	pb->p80211hostbuf
*	pb->p80211buf
*	pb->p80211buflen
*	pb->p80211_hdr
*	pb->p80211_payload
*	pb->p80211_payloadlen
*
* Arguments:
*	ethconv		Conversion type to perform
*	pb		Packet buffer containing the 802.11 frame
*
* Returns: 
*	0 on success, non-zero otherwise
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
int p80211pb_p80211_to_ether( wlandevice_t *wlandev, UINT32 ethconv, wlan_pb_t *pb)
{
	UINT8			*daddr = NULL;
	UINT8			*saddr = NULL;
	wlan_ethhdr_t	*ethhdr;
	UINT			llclen;		/* 802 LLC+data length */
	UINT			dixlen;		/* DIX data length */
	UINT			buflen;		/* full frame length, including PAD */
	UINT16			fc;

	/* setup some vars for convenience */
	fc = ieee2host16(pb->p80211_hdr->a3.fc);
	if ( (WLAN_GET_FC_TODS(fc) == 0) && (WLAN_GET_FC_FROMDS(fc) == 0) ) {
//dprintf("_to_ether: case1\n");
		daddr = pb->p80211_hdr->a3.a1;
		saddr = pb->p80211_hdr->a3.a2;
	} else if( (WLAN_GET_FC_TODS(fc) == 0) && (WLAN_GET_FC_FROMDS(fc) == 1) ) {
//dprintf("_to_ether: case2\n");
		daddr = pb->p80211_hdr->a3.a1;
		saddr = pb->p80211_hdr->a3.a3;
	} else if( (WLAN_GET_FC_TODS(fc) == 1) && (WLAN_GET_FC_FROMDS(fc) == 0) ) {
//dprintf("_to_ether: case3\n");
		daddr = pb->p80211_hdr->a3.a3;
		saddr = pb->p80211_hdr->a3.a2;
	} else {
		WLAN_LOG_ERROR0("HDR_A4 detected! A4 currently not supported.\n");
		/* set some bogus pointers so at least we won't crash */
		daddr = pb->p80211_hdr->a3.a1;
		saddr = pb->p80211_hdr->a3.a2;
	}

	ethhdr = (wlan_ethhdr_t*)(pb->p80211_payload);
	pb->p80211_llc = (wlan_llc_t*)(((UINT8*)pb->p80211_hdr) + sizeof(p80211_hdr_a3_t));
	pb->p80211_snap = (wlan_snap_t*)(((UINT8*)pb->p80211_llc) + sizeof(wlan_llc_t));

	/* Test for the various encodings */
	if ( memcmp( daddr, ethhdr->daddr, WLAN_ETHADDR_LEN) == 0 && 
		memcmp(  saddr, ethhdr->saddr, WLAN_ETHADDR_LEN) == 0 ) {
		/* ENCAP */
		/* Test for an overlength frame */
		if ( pb->p80211frmlen > WLAN_HDR_A3_LEN+WLAN_CRC_LEN+WLAN_MAX_ETHFRM_LEN) {
			/* A bogus length ethfrm has been encap'd. */
			/* Is someone trying an oflow attack? */
			dprintf("p802frmlen %d > %d\n", pb->p80211frmlen,WLAN_HDR_A3_LEN+WLAN_CRC_LEN+WLAN_MAX_ETHFRM_LEN);
			return 1;
		}
		dprintf("_to_ether ENCAP- fix rx frame stuff!!!!!!!!!!!!!\n");
		return 	1;
		/* allocate space and setup host buffer */
		buflen = llclen = pb->p80211frmlen - WLAN_HDR_A3_LEN - WLAN_CRC_LEN;
		pb->ethhostbuf = new_buf(buflen + 2); /* +2 is attempt to align IP header */
		if ( pb->ethhostbuf == NULL ) {
			dprintf("p80211_to_ether: ethhostbuf is NULLLL!!!\n");
			return 1;
		}
		pb->ethfree = p80211pb_freeskb;
		pb->ethbuf = pb->ethhostbuf;
		pb->ethbuflen = buflen;

		/* setup the pointers */
		pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;
		pb->eth_llc = (wlan_llc_t*)(pb->ethbuf + sizeof(wlan_ethhdr_t));
		pb->eth_snap = (wlan_snap_t*)
			(pb->ethbuf + sizeof(wlan_ethhdr_t) + sizeof(wlan_llc_t));
		pb->eth_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
		pb->eth_payloadlen = buflen - sizeof(wlan_ethhdr_t);

		/* now copy the data from the 80211 frame */
		memcpy( pb->ethbuf, pb->p80211_payload, buflen);	/* copy the data */
	} else if (	pb->p80211_llc->dsap == 0xaa &&
				pb->p80211_llc->ssap == 0xaa &&
				pb->p80211_llc->ctl == 0x03 &&
				memcmp( pb->p80211_snap->oui, oui_rfc1042, WLAN_IEEE_OUI_LEN) == 0 &&
				ethconv == WLAN_ETHCONV_8021h && 
				p80211_stt_findproto(ieee2host16(pb->p80211_snap->type)) ) {
		/* it's a SNAP + RFC1042 frame && protocol is in STT */
		/* build 802.3 + RFC1042 */
		
		/* Test for an overlength frame */
		if ( pb->p80211_payloadlen > WLAN_MAX_ETHFRM_LEN - WLAN_ETHHDR_LEN ) {
			/* A bogus length ethfrm has been sent. */
			/* Is someone trying an oflow attack? */
			dprintf("_to_ether: %d > %doverlen frame\n",pb->p80211_payloadlen,WLAN_MAX_ETHFRM_LEN - WLAN_ETHHDR_LEN);
			return 1;
		}

		//dprintf("_to_ether SNAP\n");
		dprintf("_to_ether SNAP- fix rx frame stuff!!!!!!!!!!!!!\n");
		return 	1;
		llclen = pb->p80211_payloadlen;
		buflen = wlan_max( llclen + sizeof(wlan_ethhdr_t), WLAN_MIN_ETHFRM_LEN);
		pb->ethhostbuf = new_buf(buflen + 2); /* +2 is attempt to align IP header */
		if ( pb->ethhostbuf == NULL ) return 1;
		pb->ethbuf = pb->ethhostbuf;
		pb->ethbuflen = buflen;
/*		memset( pb->ethbuf, 0, buflen);	*/		/* zero for possible PAD */

		/* set up the pointers */
		pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;
		pb->eth_llc = (wlan_llc_t*)(pb->ethbuf + sizeof(wlan_ethhdr_t));
		pb->eth_snap = (wlan_snap_t*)
			(pb->ethbuf + sizeof(wlan_ethhdr_t) + sizeof(wlan_llc_t));
		pb->eth_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
		pb->eth_payloadlen = llclen;

		/* set up the 802.3 header */	
		pb->eth_hdr->type = htons(pb->eth_payloadlen);
		memcpy( pb->eth_hdr->daddr, daddr, WLAN_ETHADDR_LEN);
		memcpy( pb->eth_hdr->saddr, saddr, WLAN_ETHADDR_LEN);

		/* now copy the data from the 80211 frame */
		memcpy( pb->eth_payload, pb->p80211_payload, pb->p80211_payloadlen);
	} else if ( pb->p80211_llc->dsap == 0xaa &&
			pb->p80211_llc->ssap == 0xaa &&
			pb->p80211_llc->ctl == 0x03 ) {
		/* it's an 802.1h frame || (an RFC1042 && protocol is not in STT) */
		/* build a DIXII + RFC894 */
dprintf("_to_ether: DIXII fix rx frame stuff!!!!!!!!!11111\n");
return 1;
		dixlen = pb->p80211_payloadlen - sizeof(wlan_llc_t) - sizeof(wlan_snap_t);

		/* Test for an overlength frame */
		if ( dixlen + WLAN_ETHHDR_LEN > WLAN_MAX_ETHFRM_LEN) {
			/* A bogus length ethfrm has been sent. */
			/* Is someone trying an oflow attack? */
			return 1;
		}

		dixlen = pb->p80211_payloadlen - sizeof(wlan_llc_t) - sizeof(wlan_snap_t);
		buflen = wlan_max( dixlen + sizeof(wlan_ethhdr_t), WLAN_MIN_ETHFRM_LEN);
		pb->ethhostbuf = new_buf(buflen + 2); /* +2 is attempt to align IP header */
		if ( pb->ethhostbuf == NULL ) return 1;
		pb->ethbuf = pb->ethhostbuf;
		pb->ethbuflen = buflen;
/*		memset( pb->ethbuf, 0, buflen);	*/		/* zero for possible PAD */

		/* set up the pointers */
		pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;
		pb->eth_llc = NULL;
		pb->eth_snap = NULL;
		pb->eth_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
		pb->eth_payloadlen = dixlen;

		/* make sure the llc and snap ptrs are set */
		pb->p80211_llc = (wlan_llc_t*)pb->p80211_payload;
		pb->p80211_snap = (wlan_snap_t*)
			(pb->p80211_payload + sizeof(wlan_llc_t));

		/* set up the DIXII header */	
		pb->eth_hdr->type = pb->p80211_snap->type;
		memcpy( pb->eth_hdr->daddr, daddr, WLAN_ETHADDR_LEN);
		memcpy( pb->eth_hdr->saddr, saddr, WLAN_ETHADDR_LEN);

		/* now copy the data from the 80211 frame */
		memcpy( pb->eth_payload, 
				pb->p80211_payload + sizeof(wlan_llc_t) + sizeof(wlan_snap_t), 
				dixlen);
	} else {
		/* any NON-ENCAP */
		/* it's a generic 80211+LLC or IPX 'Raw 802.3' */
		/*  build an 802.3 frame */
		/* allocate space and setup hostbuf */

		/* Test for an overlength frame */
		if ( pb->p80211_payloadlen > WLAN_MAX_ETHFRM_LEN - WLAN_CRC_LEN) {
			dprintf("_to_ether:Encap  %d > %d overlen frame\n",pb->p80211_payloadlen,WLAN_MAX_ETHFRM_LEN - WLAN_CRC_LEN);
			/* A bogus length ethfrm has been sent. */
			/* Is someone trying an oflow attack? */
			return 1;
		}
		llclen = pb->p80211_payloadlen;
		buflen = wlan_max( llclen + sizeof(wlan_ethhdr_t), WLAN_MIN_ETHFRM_LEN);

		//dprintf("_to_ether: NON-ENCAP llclen=%d , buflen=%d, hdr=%d llc_t=%d\n",
		//	llclen,buflen,sizeof(wlan_ethhdr_t),sizeof(wlan_llc_t));

#if 0 /* we use buffer passed into read hook here */
		pb->ethhostbuf = new_buf(buflen + 2); /* +2 is attempt to align IP header */
		if ( pb->ethhostbuf == NULL ) {
			dprintf("no bufs\n");
			return 1;
		}
#endif
		pb->ethbuf = pb->ethhostbuf;
		pb->ethbuflen = buflen;
/*		memset( pb->ethbuf, 0, buflen);	*/		/* zero for possible PAD */

		/* set up the pointers */
		pb->eth_hdr = (wlan_ethhdr_t*)pb->ethbuf;
		pb->eth_llc = (wlan_llc_t*)(pb->ethbuf + sizeof(wlan_ethhdr_t));
		pb->eth_snap = (wlan_snap_t*)
			(pb->ethbuf + sizeof(wlan_ethhdr_t) + sizeof(wlan_llc_t));
		pb->eth_payload = pb->ethbuf + sizeof(wlan_ethhdr_t);
		pb->eth_payloadlen = llclen;

		/* set up the 802.3 header */	
		pb->eth_hdr->type = htons(pb->eth_payloadlen);
		memcpy( pb->eth_hdr->daddr, daddr, WLAN_ETHADDR_LEN);
		memcpy( pb->eth_hdr->saddr, saddr, WLAN_ETHADDR_LEN);

		/* now copy the data from the 80211 frame */
		memcpy( pb->ethbuf+WLAN_ETHADDR_LEN+WLAN_ETHADDR_LEN, pb->p80211_payload, pb->p80211_payloadlen+6);
	}
	return 0;
}


/*----------------------------------------------------------------
* p80211pb_freeskb
*
* Free method for wlan_pb's that have skbs in them.  Called
* via ptr from p80211pb_free.  
*
* Arguments:
*	buf	Ptr to an skb
*	size	Passed in to match the ptr declaration.
*		Unused in this function.
*
* Returns: 
*	nothing
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
void p80211pb_freeskb( void *buf, int size )
{
	dprintf("P80211pb_freeskb!!!\n");
	//free( buf );
	delete_buf( buf );
}


/*----------------------------------------------------------------
* p80211pb_pbfreeskb
*
* Free method for wlan_pb's that linux kmalloc'd buffers in them.  
* Called via ptr from p80211pb_free.  
*
* Arguments:
*	buf	Ptr to buffer
*	size	Size of buffer
*
* Returns: 
*	nothing
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
void p80211pb_kfree_s( void *buf, int size)
{
	delete_buf( buf );
}


/*----------------------------------------------------------------
* p80211pb_alloc
*
* Allocs and zeros a wlan_pb structure.  Largely here for symmetry
* with the pbfree routine.  Also handy w/ ports to platforms where
* we have fixed alloc pools.
*
* Arguments:
*	none
*
* Returns: 
*	The address of a zeroed wlan_pb on success, NULL otherwise.
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
wlan_pb_t* p80211pb_alloc(void)
{
	wlan_pb_t	*pb;
	pb = new_buf( sizeof(wlan_pb_t));
	if ( pb != NULL ) {
		memset(pb, 0, sizeof(wlan_pb_t));
	}
	return pb;
}


/*----------------------------------------------------------------
* p80211pb_alloc_p80211
*
* Allocates a buffer for an 80211 frame and sets the ptrs in a 
* given pb.  Primarily used by the receive path.  Handled
* here so that the allocation used and the free method are set
* in one place (helps with portability?).
*
* Arguments:
*	pb	ptr to a wlan_pb_t previously allocated w/ p80211pb_alloc
*		if NULL, calls p80211pb_alloc.
*	size	length of the 802.11 buffer to allocate.  Note that
*		on some platforms w/ fixed size buffer pools, the size
*		will be ignored for allocation but _will_ be set in
*		the pb structure.
*
* Returns: 
*	The address of the given or allocated wlan_pb on success, 
*	NULL otherwise.
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
wlan_pb_t* p80211pb_alloc_p80211(wlan_pb_t *pb, UINT len)
{
	if ( pb == NULL ) {
		pb = p80211pb_alloc();
	}
	if ( pb != NULL ) {
		pb->p80211hostbuf = new_buf( len );
		if ( pb->p80211hostbuf != NULL ) {
			pb->p80211free = p80211pb_kfree_s;
			pb->p80211buf = (UINT8*)pb->p80211hostbuf;
			pb->p80211buflen = len;
			pb->p80211frmlen = len;		/* initially assume frm is same as buf */
			pb->p80211_hdr = (p80211_hdr_t*)pb->p80211buf;
			pb->p80211_payload = pb->p80211buf + WLAN_HDR_A3_LEN;
			pb->p80211_payloadlen = pb->p80211buflen - WLAN_HDR_A3_LEN - WLAN_CRC_LEN;
		}
	}
	return pb;
}


/*----------------------------------------------------------------
* p80211pb_free
*
* Frees the ethhostbuf and the p80211hostbuf elements of a wlan_pb
* if there is a free method for each.  Then frees the wlan_pb itself.
*
* Arguments:
*	pb	ptr to a wlan_pb_t previously allocated w/ p80211pb_alloc
*		or p80211pb_alloc_p80211.
*
* Returns: 
*	nothing
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
void p80211pb_free(wlan_pb_t* pb)
{
	if ( pb->ethhostbuf != NULL && pb->ethfree != NULL)
	{
		(*(pb->ethfree))(pb->ethhostbuf, pb->ethbuflen);
	}
	if ( pb->p80211hostbuf != NULL && pb->p80211free != NULL)
	{
		(*(pb->p80211free))(pb->p80211hostbuf, pb->p80211buflen);
	}
	delete_buf(pb);
}


/*----------------------------------------------------------------
* p80211_stt_findproto
*
* Searches the 802.1h Selective Translation Table for a given 
* protocol.
*
* Arguments:
*	proto	protocl number (in host order) to search for.
*
* Returns: 
*	1 - if the table is empty or a match is found.
*	0 - if the table is non-empty and a match is not found.
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
int p80211_stt_findproto(UINT16 proto)
{
	/* Always return found for now.  This is the behavior used by the */
	/*  Zoom Win95 driver when 802.1h mode is selected */
	/* TODO: If necessary, add an actual search we'll probably
		 need this to match the CMAC's way of doing things.
		 Need to do some testing to confirm.
	*/

	return 1;
}


#if 0
/* MSM: This function is currently unused.  Not sure if we'll ever need it. */
/*----------------------------------------------------------------
* p80211_stt_addproto
*
* Add a protocol to the 802.1h Selective Translation Table.
*
* Arguments:
*	proto	protocl number (in host order) to search for.
*
* Returns: 
*	nothing
*	
* Call context:
*	May be called in interrupt or non-interrupt context
----------------------------------------------------------------*/
int p80211_stt_addproto(UINT16 proto)
{
	return;
}

#endif

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
*
* This file defines the functions that perform Ethernet to/from
* 802.11 frame conversions.  Additionally,  the functions for
* manipulating the wlan_pb_t packet buffer structures are here.
*
* --------------------------------------------------------------------
*/
