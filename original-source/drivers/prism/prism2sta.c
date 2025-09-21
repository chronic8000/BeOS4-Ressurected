/* src/prism2/driver/prism2sta.c
*
* Implements the station functionality for prism2
*/

/*================================================================*/
/* Project Includes */


#include "p80211types.h"
#include "p80211hdr.h"
#include "p80211mgmt.h"
#include "p80211conv.h"
#include "p80211msg.h"
#include "p80211netdev.h"
#include "p80211req.h"
#include "p80211metadef.h"
#include "p80211metastruct.h"
#include "hfa384x.h"
#include "prism2mgmt.h"


/*================================================================*/
/* Local Constants */

#define PRISM2STA_MAGIC		(0x4a2d)
#define	INFOFRM_LEN_MAX		sizeof(hfa384x_ScanResults_t)

/*================================================================*/
/* Local Macros */

#define PRISM2_INFOBUF_MAX	(sizeof(hfa384x_HandoverAddr_t))
#define PRISM2_TXBUF_MAX	(sizeof(hfa384x_tx_frame_t) + \
				WLAN_DATA_MAXLEN - \
				WLAN_WEP_IV_LEN - \
				WLAN_WEP_ICV_LEN)

/*================================================================*/
/* Local Types */


int		init_module(void);
void		cleanup_module(void);
dev_link_t	*prism2sta_attach(void);
static void	prism2sta_detach(dev_link_t *link);
static void	prism2sta_config(dev_link_t *link);
static void	prism2sta_release(UINT32 arg);
static int 	prism2sta_event (event_t event, int priority, event_callback_args_t *args);

static int	prism2sta_open(wlandevice_t *wlandev);
static int	prism2sta_close(wlandevice_t *wlandev);
static void	prism2sta_reset(wlandevice_t *wlandev );
static int	prism2sta_txframe(wlandevice_t *wlandev, wlan_pb_t *pb);
static int	prism2sta_mlmerequest(wlandevice_t *wlandev, p80211msg_t *msg);

static void 	prism2sta_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs);
static void	prism2sta_int_dtim(wlandevice_t *wlandev);
static void	prism2sta_int_infdrop(wlandevice_t *wlandev);
static void	prism2sta_int_info(wlandevice_t *wlandev);
static void	prism2sta_int_txexc(wlandevice_t *wlandev);
static void	prism2sta_int_tx(wlandevice_t *wlandev);
static void	prism2sta_int_rx(wlandevice_t *wlandev);
static int	prism2sta_int_rx_typedrop( wlandevice_t *wlandev, UINT16 fc);
static void	prism2sta_int_alloc(wlandevice_t *wlandev);

static void	prism2sta_inf_handover(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_tallies(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_scanresults(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_chinforesults(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_linkstatus(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_assocstatus(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_authreq(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_psusercnt(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);

/*================================================================*/
/* Function Definitions */

inline int txfid_stackempty(prism2sta_priv_t *priv)
{
	UINT32	flags;
	/* paranoid? */
	int result;
	spin_lock_irqsave(&(priv->txfid_lock),flags);
	result = (priv->txfid_top == 0);
	spin_unlock_irqrestore(&(priv->txfid_lock), flags);
	return priv->txfid_top == 0;
}

inline void txfid_push(prism2sta_priv_t *priv, UINT16 val)
{
	UINT32	flags;
	spin_lock_irqsave(&(priv->txfid_lock),flags);
	if ( priv->txfid_top < PRISM2_FIDSTACKLEN_MAX ) {
		priv->txfid_stack[priv->txfid_top] = val;
		priv->txfid_top++;
	}
	spin_unlock_irqrestore(&(priv->txfid_lock), flags);
	return;
}

inline UINT16 txfid_pop(prism2sta_priv_t *priv)
{
	UINT16	result;
	UINT32	flags;
	spin_lock_irqsave(&(priv->txfid_lock),flags);
	if ( priv->txfid_top > 0 ) {
		priv->txfid_top--;
		result = priv->txfid_stack[priv->txfid_top];
	} else {
		result = 0;
	}
	spin_unlock_irqrestore(&(priv->txfid_lock), flags);
	return result;
}


/*----------------------------------------------------------------
* dmpmem
*
* Debug utility function to dump memory to the kernel debug log.
*
* Arguments:
*	buf	ptr data we want dumped
*	len	length of data
*
* Returns: 
*	nothing
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
inline void dmpmem(void *buf, int n)
{
	int c;
	for ( c= 0; c < n; c++) {
		if ( (c % 16) == 0 ) printk(KERN_DEBUG"dmp[%d]: ", c);
		printk("%02x ", ((UINT8*)buf)[c]);
		if ( (c % 16) == 15 ) printk("\n");
	}
	if ( (c % 16) != 0 ) printk("\n");
}

/*----------------------------------------------------------------
* cs_error
*
* Utility function to print card services error messages.
*
* Arguments:
*	handle	client handle identifying this CS client
*	func	CS function number that generated the error
*	ret	CS function return code
*
* Returns: 
*	nothing
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
static void cs_error(client_handle_t handle, int func, int ret)
{
#if CS_RELEASE_CODE < 0x2911
	CardServices(ReportError, dev_info, (void *)func, (void *)ret);
#else
	error_info_t err = { func, ret };
	CardServices(ReportError, handle, &err);
#endif
}


/*----------------------------------------------------------------
* prism2sta_open
*
* WLAN device open method.  Called from p80211netdev when kernel 
* device open (start) method is called in response to the 
* SIOCSIIFFLAGS ioctl changing the flags bit IFF_UP 
* from clear to set.
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
int prism2sta_open(wlandevice_t *wlandev)
{
	int			result = 0;
	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	int			i;
	DBFENTER;

	/* Make sure at least 1 port is enabled */
	for ( i = 0; i < HFA384x_NUMPORTS_MAX; i++) {
		if ( priv->hw->port_enabled[i] != 0 ) break;
	}
	if ( i >= HFA384x_NUMPORTS_MAX ) {
		result = -ENOMEDIUM;
	}
	MOD_INC_USE_COUNT;

	/* We don't currently have to do anything else.  */
	/* The setup of the MAC should have been done previously */
	/* via the mlme commands. */
	/* Higher layers know we're ready from dev->start==1 and */
	/* dev->tbusy==0.  Our rx path knows to pass up received */
	/* frames because of dev->flags&IFF_UP is true. */
	
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_close
*
* WLAN device close method.  Called from p80211netdev when kernel 
* device close method is called in response to the 
* SIOCSIIFFLAGS ioctl changing the flags bit IFF_UP 
* from set to clear.
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
int prism2sta_close(wlandevice_t *wlandev)
{
	DBFENTER;

	MOD_DEC_USE_COUNT;

	/* We don't currently have to do anything else.  */
	/* Higher layers know we're not ready from dev->start==0 and */
	/* dev->tbusy==1.  Our rx path knows to not pass up received */
	/* frames because of dev->flags&IFF_UP is false. */
	/* We leave the port enabled in case someone needs to receive */
	/* Info frames or send Notify frames.  All rx data frames after this */
	/* will be dropped. */

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2sta_reset
*
* Not currently implented.
*
* Arguments:
*	wlandev		wlan device structure
*	none
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
void prism2sta_reset(wlandevice_t *wlandev )
{
	DBFENTER;
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_txframe
*
* Takes a frame from p80211 and queues it for transmission.
*
* Arguments:
*	wlandev		wlan device structure
*	pb		packet buffer struct.  Contains an 802.11
*			data frame.
* Returns: 
*	0		Success and more buffs available
*	1		Success but no more buffs
*	2		Allocation failure
*	4		Buffer full or queue busy
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_txframe(wlandevice_t *wlandev, wlan_pb_t *pb )
{
	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	hfa384x_tx_frame_t	txdesc;
	UINT16			proto;
	UINT8			dscp;
	UINT16			macq = 0;
	UINT16			fid;
	int			result;
	DBFENTER;

#if defined(DETHER) 
printk(KERN_DEBUG"ethfrm[x] - [%d]:\n", pb->ethbuflen);
dmpmem(pb->ethbuf, pb->ethbuflen);
#endif

	/* Build Tx frame structure */
	/* Set up the control field */
	memset(&txdesc, 0, sizeof(txdesc));

/* Tx complete and Tx exception disable per dleach.  Might be causing buf depletion */
#if 1		
	txdesc.tx_control = 
		HFA384x_TX_MACPORT_SET(0) | HFA384x_TX_STRUCTYPE_SET(1) | 
		HFA384x_TX_TXEX_SET(1) | HFA384x_TX_TXOK_SET(1);	
#else
	txdesc.tx_control = 
		HFA384x_TX_MACPORT_SET(0) | HFA384x_TX_STRUCTYPE_SET(1);

#endif
	txdesc.tx_control = host2hfa384x_16(txdesc.tx_control);

	/* Set up the 802.11 header */
	if ( priv->priv_invoked ) {
		pb->p80211_hdr->a3.fc |= host2ieee16(WLAN_SET_FC_ISWEP(1));
	}
	memcpy(&(txdesc.frame_control), pb->p80211_hdr, WLAN_HDR_A3_LEN);

	/* Set the len, complicated because of pieces in pb */
	txdesc.data_len = pb->p80211buflen - WLAN_HDR_A3_LEN; /* llc+snap? */
	txdesc.data_len += pb->p80211_payloadlen;
	txdesc.data_len = host2hfa384x_16(txdesc.data_len);

	/* Allocate FID */
	fid = txfid_pop(priv);

	/* Copy descriptor part to FID */
#if defined(DDESC)
printk(KERN_DEBUG "Desc[0] - [%d]: \n", sizeof(txdesc));
dmpmem(&txdesc, sizeof(txdesc));
#endif
	result = hfa384x_copy_to_bap(hw, 
			hw->bap, 
			fid, 
			0, 
			&txdesc, 
			sizeof(txdesc));
	if ( result ) {
		WLAN_LOG_DEBUG3(1, "copy_to_bap(%04x, 0, %d) failed, result=0x%x\n", 
			fid, 
			sizeof(txdesc),
			result);
		result = 3;
		goto failed;
	}

	/* Copy 802.11 data to FID */
	if ( pb->p80211buflen > WLAN_HDR_A3_LEN ) { /* copy llc+snap hdr */
#if defined(DLLC)
printk(KERN_DEBUG "Hdr[%d] - [%d]:\n", sizeof(txdesc), pb->p80211buflen - WLAN_HDR_A3_LEN);
dmpmem(pb->p80211buf + WLAN_HDR_A3_LEN, pb->p80211buflen - WLAN_HDR_A3_LEN);
#endif
		
		result = hfa384x_copy_to_bap( hw, 
				     hw->bap, 
				     fid,
				     sizeof(txdesc),
				     pb->p80211buf + WLAN_HDR_A3_LEN,
				     pb->p80211buflen - WLAN_HDR_A3_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG4(1,
				"copy_to_bap(%04x, %d, %d) failed, result=0x%x\n", 
				fid, 
				sizeof(txdesc),
				pb->p80211buflen - WLAN_HDR_A3_LEN,
				result);
			result = 3;
			goto failed;
		}
	}

#if defined(D80211)
printk(KERN_DEBUG "Data[%d] - [%d]:\n", sizeof(txdesc) + pb->p80211buflen - WLAN_HDR_A3_LEN, pb->p80211_payloadlen);
dmpmem(pb->p80211_payload, pb->p80211_payloadlen);
#endif

	result = hfa384x_copy_to_bap( hw, 
		     hw->bap, 
		     fid, 
		     sizeof(txdesc) + pb->p80211buflen - WLAN_HDR_A3_LEN, 
		     pb->p80211_payload, 
		     pb->p80211_payloadlen);
	
	if ( result ) {
		WLAN_LOG_DEBUG4(1,
			"copy_to_bap(%04x, %d, %d) failed, result=0x%x\n", 
			fid,
		 	sizeof(txdesc) + pb->p80211buflen - WLAN_HDR_A3_LEN, 
	 		pb->p80211_payloadlen,
			result);
		result = 3;
		goto failed;
	}

	/*  Determine if we're doing MM queuing */
	if ( priv->qos_enable && priv->mm_mods ) {
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
			macq = priv->qos_dscpmap[dscp];
		} else {
			macq = 1;	/* best effort */
		}
	}

	/* Issue Tx command */
	result = hfa384x_cmd_transmit(hw, HFA384x_TXCMD_RECL, macq, fid);
		
	if ( result != 0 ) {
		WLAN_LOG_DEBUG2(1,"cmd_tx(%04x) failed, result=%d", 
			fid, result);
		result = 2;
		goto failed;
	}
	
	/* indicate we haven't any buffers, int_alloc will clear */
	result = txfid_stackempty(priv) ? 1 : 0;
failed:
	p80211pb_free(pb);

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_mlmerequest
*
* wlan command message handler.  All we do here is pass the message
* over to the prism2sta_mgmt_handler.
*
* Arguments:
*	wlandev		wlan device structure
*	msg		wlan command message
* Returns: 
*	0		success
*	<0		successful acceptance of message, but we're
*			waiting for an async process to finish before
*			we're done with the msg.  When the asynch
*			process is done, we'll call the p80211 
*			function p80211req_confirm() .
*	>0		An error occurred while we were handling
*			the message.
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_mlmerequest(wlandevice_t *wlandev, p80211msg_t *msg)
{
	int result = 0;
	DBFENTER;

	switch( msg->msgcode )
	{
	case DIDmsg_dot11req_mibget :
		WLAN_LOG_DEBUG0(2,"Received mibget request\n");
		result = prism2mgmt_mibset_mibget(wlandev, msg);
		break;
	case DIDmsg_dot11req_mibset :
		WLAN_LOG_DEBUG0(2,"Received mibset request\n");
		result = prism2mgmt_mibset_mibget(wlandev, msg);
		break;
	case DIDmsg_dot11req_powermgmt :
		WLAN_LOG_DEBUG0(2,"Received powermgmt request\n");
		result = prism2mgmt_powermgmt(wlandev, msg);
		break;
	case DIDmsg_dot11req_scan :
		WLAN_LOG_DEBUG0(2,"Received scan request\n");
		result = prism2mgmt_scan(wlandev, msg);
		break;
	case DIDmsg_dot11req_scan_results :
		WLAN_LOG_DEBUG0(2,"Received scan_results request\n");
		result = prism2mgmt_scan_results(wlandev, msg);
		break;
	case DIDmsg_dot11req_join :
		WLAN_LOG_DEBUG0(2,"Received join request\n");
		result = prism2mgmt_join(wlandev, msg);
		break;
	case DIDmsg_dot11req_authenticate :
		WLAN_LOG_DEBUG0(2,"Received authenticate request\n");
		result = prism2mgmt_authenticate(wlandev, msg);
		break;
	case DIDmsg_dot11req_deauthenticate :
		WLAN_LOG_DEBUG0(2,"Received mlme deauthenticate request\n");
		result = prism2mgmt_deauthenticate(wlandev, msg);
		break;
	case DIDmsg_dot11req_associate :
		WLAN_LOG_DEBUG0(2,"Received mlme associate request\n");
		result = prism2mgmt_associate(wlandev, msg);
		break;
	case DIDmsg_dot11req_reassociate :
		WLAN_LOG_DEBUG0(2,"Received mlme reassociate request\n");
		result = prism2mgmt_reassociate(wlandev, msg);
		break;
	case DIDmsg_dot11req_disassociate :
		WLAN_LOG_DEBUG0(2,"Received mlme disassociate request\n");
		result = prism2mgmt_disassociate(wlandev, msg);
		break;
	case DIDmsg_dot11req_start :
		WLAN_LOG_DEBUG0(2,"Received mlme start request\n");
		result = prism2mgmt_start(wlandev, msg);
		break;
	case DIDmsg_p2req_readpda :
		WLAN_LOG_DEBUG0(2,"Received mlme readpda request\n");
		result = prism2mgmt_readpda(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_state :
		WLAN_LOG_DEBUG0(2,"Received mlme auxport_state request\n");
		result = prism2mgmt_auxport_state(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_read :
		WLAN_LOG_DEBUG0(2,"Received mlme auxport_read request\n");
		result = prism2mgmt_auxport_read(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_write :
		WLAN_LOG_DEBUG0(2,"Received mlme auxport_write request\n");
		result = prism2mgmt_auxport_write(wlandev, msg);
		break;
	case DIDmsg_p2req_test_command :
		WLAN_LOG_DEBUG0(0,"Received mlme test_command request\n");
		result = prism2mgmt_test_command(wlandev, msg);
		break;
	case DIDmsg_p2req_mmi_read :
		WLAN_LOG_DEBUG0(0,"Received mlme mmi_read request\n");
		result = prism2mgmt_mmi_read(wlandev, msg);
		break;
	case DIDmsg_p2req_mmi_write :
		WLAN_LOG_DEBUG0(0,"Received mlme mmi_write request\n");
		result = prism2mgmt_mmi_write(wlandev, msg);
		break;
	case DIDmsg_p2req_ramdl_state :
		WLAN_LOG_DEBUG0(2,"Received mlme ramdl_state request\n");
		result = prism2mgmt_ramdl_state(wlandev, msg);
		break;
	case DIDmsg_p2req_ramdl_write :
		WLAN_LOG_DEBUG0(2,"Received mlme ramdl_write request\n");
		result = prism2mgmt_ramdl_write(wlandev, msg);
		break;
	case DIDmsg_p2req_flashdl_state :
		WLAN_LOG_DEBUG0(2,"Received mlme flashdl_state request\n");
		result = prism2mgmt_flashdl_state(wlandev, msg);
		break;
	case DIDmsg_p2req_flashdl_write :
		WLAN_LOG_DEBUG0(2,"Received mlme flashdl_write request\n");
		result = prism2mgmt_flashdl_write(wlandev, msg);
		break;
	case DIDmsg_p2req_mm_dscpmap :
		WLAN_LOG_DEBUG0(2,"Received mlme mm_dscpmap request\n");
		result = prism2mgmt_mm_dscpmap(wlandev, msg);
		break;
	case DIDmsg_p2req_dump_state :
		WLAN_LOG_DEBUG0(2,"Received mlme dump_state request\n");
		result = prism2mgmt_dump_state(wlandev, msg);
		break;
	default:
		WLAN_LOG_WARNING1("Unknown mgmt request message 0x%08lx", msg->msgcode);
		break;
	}

	DBFEXIT;
	return result;
}


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
int prism2sta_initmac(wlandevice_t *wlandev)
{
	int 			result = 0;
	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	UINT16			reg;
	int			i;
	int			j;
	UINT8			snum[12];
	DBFENTER;



!!!check useage of wlandev - should it be ether_dev or wlandev_t???
	/* call initialize */
	result = hfa384x_cmd_initialize(hw);
	if (result != 0) {
		WLAN_LOG_ERROR0("Initialize command failed.\n");
		goto failed;
	}

	/* Initialize FID stack */
	spin_lock_init(&(priv->txfid_lock));
	priv->txfid_top = 0;
	memset(priv->txfid_stack, 0, sizeof(priv->txfid_stack));

	/* make sure interrupts are disabled and any layabout events cleared */
	wlan_outw_cpu_to_le16(0, HFA384x_INTEN(hw->iobase));
	wlan_outw_cpu_to_le16(0xffff, HFA384x_EVACK(hw->iobase));

	/* Read the PDA */
	result = hfa384x_drvr_readpda(hw, priv->pda, HFA384x_PDA_LEN_MAX);

	if ( result != 0) {
		WLAN_LOG_DEBUG0(2,"drvr_readpda() failed\n");
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
		WLAN_LOG_DEBUG2(1,"priv->txfid_stack[%d]=0x%04x\n",i,priv->txfid_stack[i]);
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
	WLAN_LOG_DEBUG1(1,"priv->infofid=0x%04x\n", priv->infofid);

	/* Collect version and compatibility info */
	/*  Some are critical, some are not */
	/* NIC identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_NICIDENTITY,
			&priv->ident_nic, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve NICIDENTITY\n");
		goto failed;
	}

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
		WLAN_LOG_ERROR0("Failed to retrieve PRI_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, primary f/w actor, CFI supplier
	fields in byte order */
	priv->cap_act_pri_cfi.role = hfa384x2host_16(priv->cap_act_pri_cfi.role);
	priv->cap_act_pri_cfi.id = hfa384x2host_16(priv->cap_act_pri_cfi.id);
	priv->cap_act_pri_cfi.variant = hfa384x2host_16(priv->cap_act_pri_cfi.variant);
	priv->cap_act_pri_cfi.bottom = hfa384x2host_16(priv->cap_act_pri_cfi.bottom);
	priv->cap_act_pri_cfi.top = hfa384x2host_16(priv->cap_act_pri_cfi.top);

	WLAN_LOG_INFO5(
		"PRI-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_act_pri_cfi.role, priv->cap_act_pri_cfi.id,
		priv->cap_act_pri_cfi.variant, priv->cap_act_pri_cfi.bottom,
		priv->cap_act_pri_cfi.top);
	
	/* Compatibility range, sta f/w actor, CFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_CFIACTRANGES,
			&priv->cap_act_sta_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve STA_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, station f/w actor, CFI supplier
	fields in byte order */
	priv->cap_act_sta_cfi.role = hfa384x2host_16(priv->cap_act_sta_cfi.role);
	priv->cap_act_sta_cfi.id = hfa384x2host_16(priv->cap_act_sta_cfi.id);
	priv->cap_act_sta_cfi.variant = hfa384x2host_16(priv->cap_act_sta_cfi.variant);
	priv->cap_act_sta_cfi.bottom = hfa384x2host_16(priv->cap_act_sta_cfi.bottom);
	priv->cap_act_sta_cfi.top = hfa384x2host_16(priv->cap_act_sta_cfi.top);

	WLAN_LOG_INFO5(
		"STA-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		priv->cap_act_sta_cfi.role, priv->cap_act_sta_cfi.id,
		priv->cap_act_sta_cfi.variant, priv->cap_act_sta_cfi.bottom,
		priv->cap_act_sta_cfi.top);

	/* Compatibility range, sta f/w actor, MFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_MFIACTRANGES,
			&priv->cap_act_sta_mfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR0("Failed to retrieve STA_MFIACTRANGES\n");
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
			printk("%02x ", snum[i]);
		}
		printk("\n");
	} else {
		WLAN_LOG_ERROR0("Failed to retrieve Prism2 Card SN\n");
		goto failed;
	}

	/* Collect the MAC address */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CNFOWNMACADDR, 
		wlandev->netdev->dev_addr, WLAN_ADDR_LEN);
	if ( result != 0 ) {
		WLAN_LOG_ERROR0("Failed to retrieve mac address\n");
		goto failed;
	}

	/* Retrieve the maximum frame size */
	hfa384x_drvr_getconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	WLAN_LOG_DEBUG1(1,"F/W default max frame data size=%d\n", reg);

	reg=WLAN_DATA_MAXLEN;
	hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	hfa384x_drvr_getconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &reg);
	WLAN_LOG_DEBUG1(1,"F/W max frame data size after set=%d\n", reg);

	/* TODO: Set any internally managed config items */

	/* Set swsupport regs to magic # for card presence detection */
	wlan_outw_cpu_to_le16( PRISM2STA_MAGIC, HFA384x_SWSUPPORT0(hw->iobase));

	goto done;
failed:
	WLAN_LOG_ERROR1("Failed, result=%d\n", result);
done:
	DBFEXIT;
	return result;
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
void prism2sta_inf_handover(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:HANDOVER (unhandled)\n");
	DBFEXIT;
	return;
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
void prism2sta_inf_tallies(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:COMMTALLIES (unhandled)\n");
	DBFEXIT;
	return;
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
void prism2sta_inf_scanresults(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
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
	WLAN_LOG_DEBUG2(1,"rx scanresults, reason=%d, nbss=%d:\n",
		inf->info.scanresult.scanreason, nbss);
	for ( i = 0; i < nbss; i++) {
		WLAN_LOG_DEBUG4(1, "chid=%d anl=%d sl=%d bcnint=%d\n",
			sr->result[i].chid,
			sr->result[i].anl,
			sr->result[i].sl,
			sr->result[i].bcnint);
		WLAN_LOG_DEBUG2(1, "  capinfo=0x%04x proberesp_rate=%d\n",
			sr->result[i].capinfo,
			sr->result[i].proberesp_rate);
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
void prism2sta_inf_chinforesults(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:CHINFO (unhandled)\n");
	DBFEXIT;
	return;
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
void prism2sta_inf_linkstatus(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	UINT16			portstatus;
	int			result;
	DBFENTER;
	/* Convert */
	inf->info.linkstatus.linkstatus = 
		hfa384x2host_16(inf->info.linkstatus.linkstatus);
	/* Handle */
	switch (inf->info.linkstatus.linkstatus) {
	case HFA384x_LINK_NOTCONNECTED:
		WLAN_LOG_DEBUG0(1,"linkstatus=NOTCONNECTED (unhandled)\n");
		break;
	case HFA384x_LINK_CONNECTED:
		WLAN_LOG_DEBUG0(1,"linkstatus=CONNECTED\n");
		/* Collect the BSSID, and set state to allow tx */
		result = hfa384x_drvr_getconfig(hw, 
				HFA384x_RID_CURRENTBSSID,
				wlandev->bssid, WLAN_BSSID_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG2(1,
				"getconfig(0x%02x) failed, result = %d\n",
				HFA384x_RID_CURRENTBSSID, result);
			goto failed;
		}

		/* Collect the port status */
		result = hfa384x_drvr_getconfig16(hw, 
				HFA384x_RID_PORTSTATUS, &portstatus);
		if ( result ) {
			WLAN_LOG_DEBUG2(1,
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
		WLAN_LOG_DEBUG0(1,"linkstatus=DISCONNECTED (unhandled)\n");
		break;
	case HFA384x_LINK_AP_CHANGE:
		WLAN_LOG_DEBUG0(1,"linkstatus=AP_CHANGE (unhandled)\n");
		break;
	case HFA384x_LINK_AP_OUTOFRANGE:
		WLAN_LOG_DEBUG0(1,"linkstatus=AP_OUTOFRANGE (unhandled)\n");
		break;
	case HFA384x_LINK_AP_INRANGE:
		WLAN_LOG_DEBUG0(1,"linkstatus=AP_INRANGE (unhandled)\n");
		break;
	case HFA384x_LINK_ASSOCFAIL:
		WLAN_LOG_DEBUG0(1,"linkstatus=ASSOCFAIL (unhandled)\n");
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
void prism2sta_inf_assocstatus(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:ASSOCSTATUS (unhandled)\n");
	DBFEXIT;
	return;
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
void prism2sta_inf_authreq( wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:AUTHREQ (unhandled)\n");
	DBFEXIT;
	return;
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
void prism2sta_inf_psusercnt( wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG0(2,"received infoframe:PSUSERCNT (unhandled)\n");
	DBFEXIT;
	return;
}

/*----------------------------------------------------------------
* prism2sta_interrupt
*
* Driver interrupt handler.
*
* Arguments:
*	irq		irq number
*	dev_id		pointer to the device
*	regs		registers
*
* Returns: 
*	nothing
*
* Side effects:
*	May result in a frame being passed up the stack or an info
*	frame being handled.  
*
* Call context:
*	Ummm, could it be interrupt?
----------------------------------------------------------------*/
void prism2sta_interrupt IRQ(int irq, void *dev_id, struct pt_regs *regs)
{
	int			reg;
	wlandevice_t		*wlandev = (wlandevice_t*)dev_id;
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	int			ev_read = 0;
	DBFENTER;

	/* Check swsupport reg magic # for card presence */
	reg = wlan_inw_le16_to_cpu(HFA384x_SWSUPPORT0(hw->iobase));
	if ( reg != PRISM2STA_MAGIC) {
		WLAN_LOG_DEBUG1(2, "irq=%d, no magic.  Card removed?.\n", irq);
		return;
	}

	/* Set the BAP context */
	hw->bap = HFA384x_BAP_INT;

	/* read the EvStat register for interrupt enabled events */
	reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
	ev_read++;

	do {

		/* Handle the events */
		if ( HFA384x_EVSTAT_ISINFDROP(reg) ){
			prism2sta_int_infdrop(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_INFDROP_SET(1), 
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISINFO(reg) ){
			prism2sta_int_info(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_INFO_SET(1),
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISTXEXC(reg) ){
			prism2sta_int_txexc(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_TXEXC_SET(1),
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISTX(reg) ){
			prism2sta_int_tx(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_TX_SET(1),
				HFA384x_EVACK(hw->iobase));
		}
	
		if ( HFA384x_EVSTAT_ISRX(reg) ){
			prism2sta_int_rx(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_RX_SET(1),
				HFA384x_EVACK(hw->iobase));
		}
		
		if ( HFA384x_EVSTAT_ISALLOC(reg) ){
			prism2sta_int_alloc(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_ALLOC_SET(1),
				HFA384x_EVACK(hw->iobase));
		}

		if ( HFA384x_EVSTAT_ISDTIM(reg) ){
			prism2sta_int_dtim(wlandev);
			wlan_outw_cpu_to_le16(HFA384x_EVACK_DTIM_SET(1),
				HFA384x_EVACK(hw->iobase));
		}

		/* allow the evstat to be updated after the evack */
		udelay(20);

		/* Check swsupport reg magic # for card presence */
		reg = wlan_inw_le16_to_cpu(HFA384x_SWSUPPORT0(hw->iobase));
		if ( reg != PRISM2STA_MAGIC) {
			WLAN_LOG_DEBUG1(2, "irq=%d, no magic.  Card removed?.\n", irq);
			return;
		}

		/* read the EvStat register for interrupt enabled events */
		reg = wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase));
		ev_read++;

	} while ( (HFA384x_EVSTAT_ISINFDROP(reg) || HFA384x_EVSTAT_ISINFO(reg) ||
		HFA384x_EVSTAT_ISTXEXC(reg) || HFA384x_EVSTAT_ISTX(reg) ||
		HFA384x_EVSTAT_ISRX(reg) || HFA384x_EVSTAT_ISALLOC(reg) ||
		HFA384x_EVSTAT_ISDTIM(reg)) &&
		ev_read < prism2_irq_evread_max);

	/* Clear the BAP context */
	hw->bap = HFA384x_BAP_PROC;

	DBFEXIT;
	return;
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
void prism2sta_int_dtim(wlandevice_t *wlandev)
{
#if 0
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG0(3, "DTIM event, currently unhandled.\n");
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
void prism2sta_int_infdrop(wlandevice_t *wlandev)
{
#if 0
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG0(3, "Info frame dropped due to card mem low.\n");
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
void prism2sta_int_info(wlandevice_t *wlandev)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
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
		WLAN_LOG_DEBUG3(1, 
			"copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			reg, sizeof(inf), result);
		goto failed;
	}
	inf.framelen = hfa384x2host_16(inf.framelen);

	/* Retrieve the rest */
	result = hfa384x_copy_from_bap( hw, 
		hw->bap, reg, sizeof(UINT16),
		&(inf.infotype), inf.framelen * sizeof(UINT16));
	if ( result ) {
		WLAN_LOG_DEBUG3(1, 
			"copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			reg, sizeof(inf), result);
		goto failed;
	}
	inf.infotype = hfa384x2host_16(inf.infotype);
	WLAN_LOG_DEBUG2(1, "rx infframe, len=%d type=0x%02x\n", inf.framelen, inf.infotype);
	/* Dispatch */
	switch ( inf.infotype ) {
		case HFA384x_IT_HANDOVERADDR:
			prism2sta_inf_handover(wlandev, &inf);
			break;
		case HFA384x_IT_COMMTALLIES:
			prism2sta_inf_tallies(wlandev, &inf);
			break;
		case HFA384x_IT_SCANRESULTS:
			prism2sta_inf_scanresults(wlandev, &inf);
			break;
		case HFA384x_IT_CHINFORESULTS:
			prism2sta_inf_chinforesults(wlandev, &inf);
			break;
		case HFA384x_IT_LINKSTATUS:
			prism2sta_inf_linkstatus(wlandev, &inf);
			break;
		case HFA384x_IT_ASSOCSTATUS:
			prism2sta_inf_assocstatus(wlandev, &inf);
			break;
		case HFA384x_IT_AUTHREQ:
			prism2sta_inf_authreq(wlandev, &inf);
			break;
		case HFA384x_IT_PSUSERCNT:
			prism2sta_inf_psusercnt(wlandev, &inf);
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
void prism2sta_int_txexc(wlandevice_t *wlandev)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	UINT16			status;
	UINT16			fid;
	int			result = 0;
	DBFENTER;
	/* Collect the status and display */
	fid = wlan_inw_le16_to_cpu(HFA384x_TXCOMPLFID(hw->iobase));
	result = hfa384x_copy_from_bap(hw, hw->bap, fid, 0, &status, sizeof(status));
	if ( result ) {
		WLAN_LOG_DEBUG3(1, 
			"copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			fid, sizeof(status), result);
		goto failed;
	}
	status = hfa384x2host_16(status);
	WLAN_LOG_DEBUG1(3, "TxExc status=0x%x.\n", status);
failed:
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_int_tx
*
* Handles the Tx event.
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
void prism2sta_int_tx(wlandevice_t *wlandev)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	UINT16			fid;
	UINT16			status;
	int			result = 0;
	DBFENTER;
	fid = wlan_inw_le16_to_cpu(HFA384x_TXCOMPLFID(hw->iobase));
	result =hfa384x_copy_from_bap(hw, hw->bap, fid, 0, &status, sizeof(status));
	if ( result ) {
		WLAN_LOG_DEBUG3(1, 
			"copy_from_bap(0x%04x, 0, %d) failed, result=0x%x\n", 
			fid, sizeof(status), result);
		goto failed;
	}
	status = hfa384x2host_16(status);
	WLAN_LOG_DEBUG1(4, "Tx Complete, status=0x%04x\n", status);
failed:
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_int_rx
*
* Handles the Rx event.
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
void prism2sta_int_rx(wlandevice_t *wlandev)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;
	UINT16			rxfid;
	hfa384x_rx_frame_t	rxdesc;
	wlan_pb_t		*pb;
	int			result;

	DBFENTER;

	/* Get the FID */
	rxfid = wlan_inw_le16_to_cpu(HFA384x_RXFID(hw->iobase));
	/* Get the descriptor (including headers) */
	result = hfa384x_copy_from_bap(hw, 
			hw->bap, 
			rxfid, 
			0, 
			&rxdesc, 
			sizeof(rxdesc));
	if ( result ) {
		WLAN_LOG_DEBUG4(1, 
			"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
			rxfid, 
			0, 
			sizeof(rxdesc),
			result);
		goto failed;
	}

	/* Byte order convert once up front. */
	rxdesc.status =	hfa384x2host_16(rxdesc.status);
	rxdesc.time =	hfa384x2host_32(rxdesc.time);
	rxdesc.data_len = hfa384x2host_16(rxdesc.data_len);

#if 0
printk(KERN_DEBUG"rxf(%d): ",rxlen);
for (i=0; i<pb->p80211frmlen; i++) {
	printk("%x ",pb->p80211buf[i]);
}
printk("\n");
#endif

	/* Now handle frame based on port# */
	switch( HFA384x_RXSTATUS_MACPORT_GET(rxdesc.status) )
	{
	case 0:
		/* Allocate the buffer, note CRC (aka FCS). pballoc */
		/* assumes there needs to be space for one */
		pb = p80211pb_alloc_p80211(NULL, 
			rxdesc.data_len + WLAN_HDR_A3_LEN + WLAN_CRC_LEN);
		if ( pb == NULL ) {
			WLAN_LOG_DEBUG0(1, "pballoc failed.\n");
			goto failed;
		}
	
		/* Copy the 802.11 hdr to the buffer */
		result = hfa384x_copy_from_bap(hw, hw->bap, rxfid, 
			HFA384x_RX_80211HDR_OFF, pb->p80211_hdr, WLAN_HDR_A3_LEN);
		if ( result ) {
			WLAN_LOG_DEBUG4(1, 
				"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
				rxfid, 
				HFA384x_RX_80211HDR_OFF, 
				WLAN_HDR_A3_LEN,
				result);
			goto failed_cleanup;
		}

		if (prism2sta_int_rx_typedrop(wlandev, ieee2host16(pb->p80211_hdr->a3.fc))) {
			WLAN_LOG_WARNING0("Unhandled frame type, dropped.\n");
			goto failed_cleanup;
		}

		/* If exclude and we receive an unencrypted, drop it */
		if ( priv->exclude_unencrypt && 
			!WLAN_GET_FC_ISWEP(ieee2host16(pb->p80211_hdr->a3.fc))) {
			p80211pb_free(pb);
			goto failed_cleanup;
		}

		/* Copy the payload data to the buffer */
		if ( rxdesc.data_len > 0 ) {
			result = hfa384x_copy_from_bap(hw, 
				hw->bap, rxfid, HFA384x_RX_DATA_OFF, 
				pb->p80211_payload, rxdesc.data_len);
			if ( result ) {
				WLAN_LOG_DEBUG4(1, 
					"copy_from_bap(0x%04x, %d, %d) failed, result=0x%x\n", 
					rxfid, 
					HFA384x_RX_DATA_OFF, 
					rxdesc.data_len,
					result);
				p80211pb_free(pb);
				goto failed_cleanup;
			}
		}
		/* Set the length */
		pb->p80211frmlen = WLAN_HDR_A3_LEN + rxdesc.data_len + WLAN_CRC_LEN;
		/* Call p80211netdev_rx */
		p80211netdev_rx(wlandev, pb);
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

done:
	DBFEXIT;
	return;
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
int prism2sta_int_rx_typedrop( wlandevice_t *wlandev, UINT16 fc)
{
	UINT16	ftype;
	UINT16	fstype;
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
			printk("assocreq");
			wlandev->rx.assocreq++;
			break;
		case WLAN_FSTYPE_ASSOCRESP:
			printk("assocresp");
			wlandev->rx.assocresp++;
			break;
		case WLAN_FSTYPE_REASSOCREQ:
			printk("reassocreq");
			wlandev->rx.reassocreq++;
			break;
		case WLAN_FSTYPE_REASSOCRESP:
			printk("reassocresp");
			wlandev->rx.reassocresp++;
			break;
		case WLAN_FSTYPE_PROBEREQ:
			printk("probereq");
			wlandev->rx.probereq++;
			break;
		case WLAN_FSTYPE_PROBERESP:
			printk("proberesp");
			wlandev->rx.proberesp++;
			break;
		case WLAN_FSTYPE_BEACON:
			printk("beacon");
			wlandev->rx.beacon++;
			break;
		case WLAN_FSTYPE_ATIM:
			printk("atim");
			wlandev->rx.atim++;
			break;
		case WLAN_FSTYPE_DISASSOC:
			printk("disassoc");
			wlandev->rx.disassoc++;
			break;
		case WLAN_FSTYPE_AUTHEN:
			printk("authen");
			wlandev->rx.authen++;
			break;
		case WLAN_FSTYPE_DEAUTHEN:
			printk("deauthen");
			wlandev->rx.deauthen++;
			break;
		default:
			printk("unknown");
			wlandev->rx.mgmt_unknown++;
			break;
		}
		printk("\n");
		drop = 1;
		break;

	case WLAN_FTYPE_CTL:
		WLAN_LOG_WARNING0("prism2sta_int_rx(): rx'd ctl:");
		wlandev->rx.ctl++;
		switch( fstype ) {
		case WLAN_FSTYPE_PSPOLL:
			printk("pspoll");
			wlandev->rx.pspoll++;
			break;
		case WLAN_FSTYPE_RTS:
			printk("rts");
			wlandev->rx.rts++;
			break;
		case WLAN_FSTYPE_CTS:
			printk("cts");
			wlandev->rx.cts++;
			break;
		case WLAN_FSTYPE_ACK:
			printk("ack");
			wlandev->rx.ack++;
			break;
		case WLAN_FSTYPE_CFEND:
			printk("cfend");
			wlandev->rx.cfend++;
			break;
		case WLAN_FSTYPE_CFENDCFACK:
			printk("cfendcfack");
			wlandev->rx.cfendcfack++;
			break;
		default:
			printk("unknown");
			wlandev->rx.ctl_unknown++;
			break;
		}
		printk("\n");
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
			printk("unknown");
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
void prism2sta_int_alloc(wlandevice_t *wlandev)
{
	prism2sta_priv_t	*priv = wlandev->priv;
	hfa384x_t		*hw = priv->hw;

	DBFENTER;

	/* Handle the reclaimed FID */
	/*   collect the FID and push it onto the stack */
	txfid_push(priv, wlan_inw_le16_to_cpu(HFA384x_ALLOCFID(hw->iobase)));
	/*   clear tbusy (if set) */

#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
	wlandev->netdev->tbusy = 0;
	mark_bh(NET_BH);
#else
	netif_wake_queue(wlandev->netdev);
#endif

	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_attach
*
* Half of the attach/detach pair.  Creates and registers a device
* instance with Card Services.  In this case, it also creates the
* wlandev structure and device private structure.  These are 
* linked to the device instance via its priv member.
*
* Arguments:
*	none
*
* Returns: 
*	A valid ptr to dev_link_t on success, NULL otherwise
*
* Side effects:
*	
*
* Call context:
*	process thread (insmod/init_module/register_pccard_driver)
----------------------------------------------------------------*/
dev_link_t *prism2sta_attach(void)
{
	client_reg_t		client_reg;
	int			ret;
	dev_link_t		*link;
	wlandevice_t		*wlandev;
	prism2sta_priv_t	*priv;


	DBFENTER;

	/* Create the PC card device object. */
	link = kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);
	if ( link == NULL ) {
		return NULL;
	}
	memset(link, 0, sizeof(struct dev_link_t));
	link->release.function = &prism2sta_release;
	link->release.data = (u_long)link;
	link->conf.IntType = INT_MEMORY_AND_IO;

	/* Create the network device object. */
	wlandev = kmalloc(sizeof(wlandevice_t), GFP_KERNEL);
	if ( wlandev == NULL ) {
		kfree_s(link, sizeof(dev_link_t));
		return NULL;
	}
	memset(wlandev, 0, sizeof(wlandevice_t));

	/* Make up a device private data structure. */
	wlandev->priv = kmalloc(sizeof(prism2sta_priv_t), GFP_KERNEL);
	if ( wlandev->priv == NULL ) {
		kfree_s(wlandev, sizeof(wlandevice_t));
		kfree_s(link, sizeof(dev_link_t));
		return NULL;
	}
	memset(wlandev->priv, 0, sizeof(prism2sta_priv_t));

	/* Make up a hw data structure. */
	priv = (prism2sta_priv_t*)wlandev->priv;
	priv->hw = kmalloc(sizeof(hfa384x_t), GFP_KERNEL);
	if ( priv->hw == NULL ) {
		kfree_s(wlandev->priv, sizeof(prism2sta_priv_t));
		kfree_s(wlandev, sizeof(wlandevice_t));
		kfree_s(link, sizeof(dev_link_t));
		return NULL;
	}
	memset(priv->hw, 0, sizeof(hfa384x_t));

	/* Set our entries in the wlandev */
	wlandev->open = &prism2sta_open;
	wlandev->close = &prism2sta_close;
	wlandev->reset = &prism2sta_reset;
	wlandev->txframe = &prism2sta_txframe;
	wlandev->mlmerequest = &prism2sta_mlmerequest;

	/* Set up the remaining entries in the wlan common way */
	wlandev->name = ((prism2sta_priv_t*)wlandev->priv)->node.dev_name;
	wlan_setup(wlandev);

	link->priv = wlandev;
#if CS_RELEASE_CODE > 0x2911
	link->irq.Instance = wlandev;
#endif

	/* Link in to the list of devices managed by this driver */
	link->next = dev_list;
	dev_list = link;	

	/* Register with Card Services */
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_REQUEST |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &prism2sta_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;

	ret = CardServices(RegisterClient, &link->handle, &client_reg);
	if (ret != 0) {
		cs_error(link->handle, RegisterClient, ret);
		prism2sta_detach(link);
		return NULL;
	}

	return link;
}


/*----------------------------------------------------------------
* prism2sta_detach
*
* Remove one of the device instances managed by this driver.
*   Search the list for the given instance, 
*   check our flags for a waiting timer'd release call
*   call release
*   Deregister the instance with Card Services
*   (netdevice) unregister the network device.
*   unlink the instance from the list
*   free the link, priv, and priv->priv memory
* Note: the dev_list variable is a driver scoped static used to
*	maintain a list of device instances managed by this
*	driver.
*
* Arguments:
*	link	ptr to the instance to detach
*
* Returns: 
*	nothing
*
* Side effects:
*	the link structure is gone, the netdevice is gone
*
* Call context:
*	Might be interrupt, don't block.
----------------------------------------------------------------*/
void prism2sta_detach(dev_link_t *link)
{
	dev_link_t		**linkp;
	UINT32			flags;
	wlandevice_t		*wlandev;
	prism2sta_priv_t	*priv;

	DBFENTER;

	/* Locate device structure */
	for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next) {
		if (*linkp == link) break;
	}

	if (*linkp != NULL) {
		/* Get rid of any timer'd release call */	
		save_flags(flags);
		cli();
		if (link->state & DEV_RELEASE_PENDING) {
			del_timer(&link->release);
			link->state &= ~DEV_RELEASE_PENDING;
		}
		restore_flags(flags);
		
		/* If link says we're still config'd, call release */
		if (link->state & DEV_CONFIG) {
			prism2sta_release((u_long)link);
			if (link->state & DEV_STALE_CONFIG) {
				link->state |= DEV_STALE_LINK;
				return;
			}
		}
		
		/* Tell Card Services we're not around any more */
		if (link->handle) {
			CardServices(DeregisterClient, link->handle);
		}	

		/* Unlink device structure, free bits */
		*linkp = link->next;
		if ( link->priv != NULL ) {
			wlandev = (wlandevice_t*)link->priv;
			if (link->dev != NULL) {
				unregister_wlandev(wlandev);
			}
			wlan_unsetup(wlandev);
			if (wlandev->priv) {
				priv = (prism2sta_priv_t*)wlandev->priv;
				if ( priv->hw )
					kfree_s(priv->hw, sizeof(hfa384x_t));
				kfree_s(wlandev->priv, sizeof(prism2sta_priv_t));
			}
			kfree_s(link->priv, sizeof(wlandevice_t));
		}
		kfree_s(link, sizeof(struct dev_link_t));
	}

	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_config
*
* Half of the config/release pair.  Usually called in response to
* a card insertion event.  At this point, we _know_ there's some
* physical device present.  That means we can start poking around
* at the CIS and at any device specific config data we want.
*
* Note the gotos and the macros.  I recoded this once without
* them, and it got incredibly ugly.  It's actually simpler with
* them.
*
* Arguments:
*	link	the dev_link_t structure created in attach that 
*		represents this device instance.
*
* Returns: 
*	nothing
*
* Side effects:
*	Resources (irq, io, mem) are allocated
*	The pcmcia dev_link->node->name is set
*	(For netcards) The device structure is finished and,
*	  most importantly, registered.  This means that there
*	  is now a _named_ device that can be configured from
*	  userland.
*
* Call context:
*	May be called from a timer.  Don't block!
----------------------------------------------------------------*/
#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed;

#if defined(WLAN_INCLUDE_DEBUG)
#define CFG_CHECK(fn, args...) \
if ((last_ret=CardServices(last_fn=(fn), args))!=0) {  \
	WLAN_LOG_DEBUG0(1,"CFG_CHECK failed\n"); \
	cs_error(link->handle, last_fn, last_ret); \
	goto next_entry; \
}
#else
#define CFG_CHECK(fn, args...) if (CardServices(fn, args)!=0) goto next_entry;
#endif 

void prism2sta_config(dev_link_t *link)
{
	client_handle_t		handle;
	wlandevice_t		*wlandev;
	prism2sta_priv_t	*priv;
	int			last_fn;
	int			last_ret;
	tuple_t			tuple;
	cisparse_t		parse;
	config_info_t		socket_config;
	UINT8			buf[64];
	int			i;

	DBFENTER;

	handle = link->handle;
	wlandev = (wlandevice_t*)link->priv;

	/* Collect the config register info */
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];
	
	/* Configure card */
	link->state |= DEV_CONFIG;

	/* Acquire the current socket config (need Vcc setting) */
	CS_CHECK(GetConfigurationInfo, handle, &socket_config);

	/* Loop through the config table entries until we find one that works */
	/* Assumes a complete and valid CIS */
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t dflt = { 0 };
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK(ParseTuple, handle, &tuple, &parse);

		if (cfg->index == 0) goto next_entry;
		link->conf.ConfigIndex = cfg->index;

		/* Use power settings for Vcc and Vpp if present */
		/* Note that the CIS values need to be rescaled */
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM)) {
			WLAN_LOG_DEBUG0(1, "vcc set from VNOM\n");
			link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		} else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM)) {
			WLAN_LOG_DEBUG0(1, "vcc set from VNOM\n");
			link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;
		} else if ((cfg->vcc.present & (1<<CISTPL_POWER_VMAX)) &&
			   (cfg->vcc.present & (1<<CISTPL_POWER_VMIN)) ) {
			WLAN_LOG_DEBUG0(1, "vcc set from avg(VMIN,VMAX)\n");
			link->conf.Vcc = 
				((cfg->vcc.param[CISTPL_POWER_VMAX] +
				cfg->vcc.param[CISTPL_POWER_VMIN]) / 2) / 10000;
		} else if ((dflt.vcc.present & (1<<CISTPL_POWER_VMAX)) &&
			   (dflt.vcc.present & (1<<CISTPL_POWER_VMIN)) ) {
			WLAN_LOG_DEBUG0(1, "vcc set from avg(VMIN,VMAX\n");
			link->conf.Vcc = 
				((dflt.vcc.param[CISTPL_POWER_VMAX] +
				dflt.vcc.param[CISTPL_POWER_VMIN]) / 2) / 10000;
		}

		if ( link->conf.Vcc >= 45 && link->conf.Vcc <= 55 ) {
			link->conf.Vcc = 50;
		} else {
			link->conf.Vcc = 33; /* default to 3.3v (safer)*/
		}

		if ( link->conf.Vcc >= socket_config.Vcc ) {
			link->conf.Vcc = socket_config.Vcc;
		} else {		
			goto next_entry;
		}
			
		WLAN_LOG_DEBUG1(1, "link->conf.Vcc=%d\n", link->conf.Vcc);

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
			if  ( link->io.BasePort1 != 0 ) {
				WLAN_LOG_WARNING1(
				"Brain damaged CIS: hard coded iobase="
				"0x%x, try letting pcmcia_cs decide...\n",
				link->io.BasePort1 );
				link->io.BasePort1 = 0;
			}
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}

		/* This reserves IO space but doesn't actually enable it */
		CFG_CHECK(RequestIO, link->handle, &link->io);

		/* If we got this far, we're cool! */
		break;

next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		CS_CHECK(GetNextTuple, handle, &tuple);
	}

	/* Allocate an interrupt line.  Note that this does not assign a */
	/* handler to the interrupt, unless the 'Handler' member of the */
	/* irq structure is initialized. */
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
	{
		link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
		link->irq.IRQInfo1 = IRQ_INFO2_VALID | IRQ_LEVEL_ID;
		if (irq_list[0] == -1)
			link->irq.IRQInfo2 = irq_mask;
		else
			for (i=0; i<4; i++)
				link->irq.IRQInfo2 |= 1 << irq_list[i];
		link->irq.Handler = prism2sta_interrupt;
		link->irq.Instance = wlandev;
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	}

	/* This actually configures the PCMCIA socket -- setting up */
	/* the I/O windows and the interrupt mapping, and putting the */
	/* card and host interface into "Memory and IO" mode. */
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);

	/* Fill the netdevice with this info */
	wlandev->netdev->irq = link->irq.AssignedIRQ;
	wlandev->netdev->base_addr = link->io.BasePort1;

	/* Report what we've done */
	printk(KERN_INFO "%s: index 0x%02x: Vcc %d.%d", 
		dev_info, link->conf.ConfigIndex, 
		link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1)
		printk(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		printk(", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
		printk(", io 0x%04x-0x%04x", link->io.BasePort1, link->io.BasePort1+link->io.NumPorts1-1);
	if (link->io.NumPorts2)
		printk(" & 0x%04x-0x%04x", link->io.BasePort2, link->io.BasePort2+link->io.NumPorts2-1);
	printk("\n");

	link->state &= ~DEV_CONFIG_PENDING;

	/* Register the network device and get assigned a name */
	if (register_wlandev(wlandev) != 0) {
		WLAN_LOG_NOTICE0("prism2sta_cs: register_wlandev() failed.\n");
		goto failed;
	}

	priv = (prism2sta_priv_t*)wlandev->priv;/* collect the device priv ptr */
	link->dev = &priv->node;		/* now pcmcia knows the device name */

	/* Any device custom config/query stuff should be done here */
	/* For a netdevice, we should at least grab the mac address */

	return;
cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	WLAN_LOG_ERROR0("NextTuple failure? It's probably a Vcc mismatch.\n");

failed:
	prism2sta_release((UINT32)link);
	return;
}




/*----------------------------------------------------------------
* prism2sta_release
*
* Half of the config/release pair.  Usually called in response to 
* a card ejection event.  Checks to make sure no higher layers
* are still (or think they are) using the card via the link->open
* field.  
*
* NOTE: Don't forget to increment the link->open variable in the 
*  device_open method, and decrement it in the device_close 
*  method.
*
* Arguments:
*	arg	a generic 32 bit variable...we assume it's a 
*               ptr to a dev_link.
*
* Returns: 
*	nothing
*
* Side effects:
*	All resources should be released after this function
*	executes and finds the device !open.
*
* Call context:
*	Possibly in a timer context.  Don't do anything that'll
*	block.
----------------------------------------------------------------*/
void prism2sta_release(UINT32 arg)
{
        dev_link_t	*link = (dev_link_t *)arg;

	DBFENTER;

        if (link->open) {
                WLAN_LOG_DEBUG1(1, "prism2sta_cs: release postponed, '%s' still open\n",
                          link->dev->dev_name);
                link->state |= DEV_STALE_CONFIG;
                return;
        }

        CardServices(ReleaseConfiguration, link->handle);
        CardServices(ReleaseIO, link->handle, &link->io);
        CardServices(ReleaseIRQ, link->handle, &link->irq);

        link->state &= ~(DEV_CONFIG | DEV_RELEASE_PENDING);

	DBFEXIT;
}



/*----------------------------------------------------------------
* prism2sta_event
*
* Handler for card services events.
*
* Arguments:
*	event		The event code
*	priority	hi/low - REMOVAL is the only hi
*	args		ptr to card services struct containing info about
*			pcmcia status
*
* Returns: 
*	Zero on success, non-zero otherwise
*
* Side effects:
*	
*
* Call context:
*	Both interrupt and process thread, depends on the event.
----------------------------------------------------------------*/
static int prism2sta_event (event_t event, int priority, event_callback_args_t *args)
{
	int		result = 0;
	dev_link_t	*link = (dev_link_t *) args->client_data;
	wlandevice_t	*wlandev = (wlandevice_t*)link->priv;
	DBFENTER;

	switch (event)
	{
	case CS_EVENT_CARD_INSERTION:
		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		prism2sta_config(link);
		if ((link->state & DEV_CONFIG) == 0 ) {
			wlandev->netdev->irq = 0;
			WLAN_LOG_WARNING1("%s: Initialization failed!\n", dev_info);
		} else {
			hfa384x_inithw(
				((prism2sta_priv_t*)wlandev->priv)->hw, 
				wlandev->netdev->irq,
				wlandev->netdev->base_addr);
			if ( prism2sta_initmac(wlandev) != 0 ) {
				WLAN_LOG_ERROR0("MAC Initialization failed.\n");
			}
			result = 1;
		}
		break;

	case CS_EVENT_CARD_REMOVAL:
		link->state &= ~DEV_PRESENT;
		if (link->state & DEV_CONFIG)
		{
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			wlandev->netdev->tbusy = 1;
			wlandev->netdev->start = 0;
#else
			netif_stop_queue(wlandev->netdev);
#endif
			link->release.expires = RUN_AT(HZ/20);
			add_timer(&link->release);
		}
		break;
	case CS_EVENT_RESET_REQUEST:
		WLAN_LOG_NOTICE0(
			"prism2 card reset not supported "
			"due to post-reset user mode configuration "
			"requirements.\n");
		WLAN_LOG_NOTICE0(
			"  From user mode, use "
			"'cardctl suspend;cardctl resume' "
			"instead.\n");
		result = 1;
		break;
	case CS_EVENT_RESET_PHYSICAL:
	case CS_EVENT_CARD_RESET:
		WLAN_LOG_WARNING0("Received CS_EVENT_RESET_xxx, should not "
			"be possible since REQUEST is denied.\n");
		break;

	case CS_EVENT_PM_SUSPEND:
		link->state |= DEV_SUSPEND;
		if (link->state & DEV_CONFIG)
		{
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
			wlandev->netdev->tbusy = 1;
			wlandev->netdev->start = 0;
#else
			netif_stop_queue(wlandev->netdev);
#endif
			CardServices(ReleaseConfiguration, link->handle);
		}
		break;

	case CS_EVENT_PM_RESUME:
		link->state &= ~DEV_SUSPEND;
		if (link->state & DEV_CONFIG)
		{
			CardServices(RequestConfiguration, link->handle, &link->conf);
			hfa384x_inithw(
				((prism2sta_priv_t*)wlandev->priv)->hw, 
				wlandev->netdev->irq,
				wlandev->netdev->base_addr);
			if ( prism2sta_initmac(wlandev) != 0 ) {
				WLAN_LOG_ERROR0("MAC Initialization failed.\n");
				result = 1;
			} else {
#if (LINUX_VERSION_CODE < WLAN_KVERSION(2,3,38) )
				wlandev->netdev->tbusy = 0;
				wlandev->netdev->start = 1;
#else
				netif_start_queue(wlandev->netdev);
#endif
			}
		}
		break;
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* init_module
*
* Module initialization routine, called once at module load time.
* This one simulates some of the pcmcia calls.
*
* Arguments:
*	none
*
* Returns: 
*	0	- success 
*	~0	- failure, module is unloaded.
*
* Side effects:
*	TODO: define
*
* Call context:
*	process thread (insmod or modprobe)
----------------------------------------------------------------*/
int init_module(void)
{
	int		result = 0;
	servinfo_t	serv;

        DBFENTER;

        WLAN_LOG_NOTICE1("%s Loaded\n", version);
        WLAN_LOG_NOTICE1("dev_info is: %s\n", dev_info);

	CardServices(GetCardServicesInfo, &serv);
	if ( serv.Revision != CS_RELEASE_CODE )
	{
		printk(KERN_WARNING"%s: CardServices release does not match!\n", dev_info);
	}

	/* This call will result in a call to prism2sta_attach */
	/*   and eventually prism2sta_detach */
	register_pcmcia_driver( &dev_info, &prism2sta_attach, &prism2sta_detach);

        DBFEXIT;
        return result;
}


/*----------------------------------------------------------------
* cleanup_module
*
* Called at module unload time.  This is our last chance to
* clean up after ourselves.
*
* Arguments:
*	none
*
* Returns: 
*	nothing
*
* Side effects:
*	TODO: define
*
* Call context:
*	process thread
*
----------------------------------------------------------------*/
void cleanup_module(void)
{

        DBFENTER;

	unregister_pcmcia_driver( &dev_info);

/*
TODO: fix this to clean up leftover pcmcia and netdev instances
	dev_link_t *link = dev_list;
	while (link) {
		wlandevice_t *wlandev = (wlandevice_t *)link->priv;
		if (link->dev != NULL) {
		unregister_wlandev(wlandev);
		}
		wlan_unsetup(wlandev);
		if (wlandev->priv) {
			kfree_s(wlandev->priv, sizeof(prism2sta_priv_t));
		}
		kfree_s(wlandev, sizeof(wlandevice_t));
		kfree_s(link, sizeof(struct dev_link_t));
	}
*/
        printk(KERN_NOTICE "%s Unloaded\n", version);

        DBFEXIT;
        return;
}

/*
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
* This file implements the module and linux pcmcia routines for the
* prism2 driver.
*
* --------------------------------------------------------------------
*/
