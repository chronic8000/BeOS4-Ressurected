/* src/prism2/driver/prism2mgmt.c
*
* Management request handler functions.
*
*
* The functions in this file handle management requests sent from
* user mode.
*
* Most of these functions have two separate blocks of code that are
* compile-time conditional on WLAN_STA and WLAN_AP.  This is used
* to separate out the STA and AP responses to these management primitives.
* It's a choice (good, bad, indifferent?) to have the code in the same 
* place so it's clear that the same primitive  is implemented in both 
* cases but has different behavior.
*
* --------------------------------------------------------------------
*/

/*================================================================*/
/* System Includes */

#define WLAN_DBVAR	prism2_debug


#include <OS.h>

#include "wlan_compat.h"

#include <stdio.h>

#include <config.h>
#include <k_compat.h>
#include <version.h>
#include <cs_types.h>
#include <cs.h>
#include <cistpl.h>
#include <ds.h>
#include <cisreg.h>
#include <driver_ops.h>

/*================================================================*/
/* Project Includes */

#include "version.h"
#include "p80211types.h"
#include "p80211hdr.h"
#include "p80211conv.h"
#include "p80211msg.h"
#include "p80211meta.h"
#include "p80211metastruct.h"
#include "prism.h"



extern pci_module_info				*pci;


/* Converts 802.11 format rate specifications to prism2 */


#define p80211rate_to_p2bit(n)	((((n)&~BIT7) == 2) ? BIT0 : \
				 (((n)&~BIT7) == 4) ? BIT1 : \
				 (((n)&~BIT7) == 11) ? BIT2 : \
				 (((n)&~BIT7) == 22) ? BIT3 : 0)


/*----------------------------------------------------------------
* prism2mgmt_powermgmt
*
* Set the power management state of this station's MAC.
*
----------------------------------------------------------------*/
int x_prism2mgmt_powermgmt( void * msgp )  {
	int 			result = 0;
	p80211msg_dot11req_powermgmt_t	*msg = (p80211msg_dot11req_powermgmt_t *) msgp;
	DBFENTER;
/*
 * Set CNFPMENABLED (on or off)
 * Set CNFMULTICASTRX (if PM on, otherwise clear)
 * Spout a notice stating that SleepDuration and HoldoverDuration and PMEPS
 * also have an impact.
 */
	/* Powermgmt is currently unsupported for STA */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_scan
*
* Initiate a scan for BSSs.
*
 Returns: 
*	0	success and done
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_scan( void *msgp)
{
	int 			result = 0;
	p80211msg_dot11req_scan_t	*msg = msgp;

	DBFENTER;

	result = 0;
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_scan_results
*
* Retrieve the BSS description for one of the BSSs identified in
* a scan.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_scan_results( void *msgp)
{

	int 			result = 0;
	p80211msg_dot11req_scan_results_t	*msg = msgp;
	DBFENTER;

	/* Same situation as scan */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_join
*
* Join a BSS whose BSS description was previously obtained with
* a scan.
----------------------------------------------------------------*/
int prism2mgmt_join( void *msgp)
{

	int 			result = 0;
	p80211msg_dot11req_join_t	*msg = msgp;
	DBFENTER;

	/* TODO: Implement after scan */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_authenticate
*
* Station should be begin an authentication exchange.
----------------------------------------------------------------*/
int prism2mgmt_authenticate( void *msgp)
{

	int 			result = 0;
	p80211msg_dot11req_authenticate_t	*msg = msgp;
	DBFENTER;

	/* TODO: Decide how we're going to handle this one w/ Prism2 */
	/*       It could be entertaining since Prism2 doesn't have  */
	/*       an explicit way to control this */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_deauthenticate
*
* Send a deauthenticate notification.
*
----------------------------------------------------------------*/
int prism2mgmt_deauthenticate( void *msgp)
{
	int 			result = 0;
	p80211msg_dot11req_deauthenticate_t	*msg = msgp;
	DBFENTER;

	/* TODO: Decide how we're going to handle this one w/ Prism2 */
	/*       It could be entertaining since Prism2 doesn't have  */
	/*       an explicit way to control this */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_associate
*
* Associate with an ESS.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_associate(ether_dev_info_t *wlandev, void *msgp)
{
#if defined(WLAN_STA)
	hfa384x_t		*hw = &wlandev->hw;
	int 			result = 0;
	UINT16			reg;
	UINT16			port_type;
	p80211msg_dot11req_associate_t	*msg = msgp;
	DBFENTER;

	/* Set the TxRates */
	reg = 0x000f;
	hfa384x_drvr_setconfig16(hw, HFA384x_RID_TXRATECNTL, &reg);

	/* Set the PortType */
	port_type = 1; /* ess port */
	hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFPORTTYPE, &port_type);

	/* Enable the interrupts */
	reg = HFA384x_INTEN_INFDROP_SET(1) |
		HFA384x_INTEN_INFO_SET(1) |
		HFA384x_INTEN_ALLOC_SET(1) |
		HFA384x_INTEN_TXEXC_SET(1) |
		HFA384x_INTEN_TX_SET(1) |
		HFA384x_INTEN_RX_SET(1);
	wlan_outw_cpu_to_le16(0xffff, HFA384x_EVSTAT(hw->iobase));
	wlan_outw_cpu_to_le16(reg, HFA384x_INTEN(hw->iobase));
		
	/* Enable the Port */
	hfa384x_cmd_enable(hw, 0);
	

	/* Set the resultcode */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_success;

#elif defined(WLAN_AP)
	int 				result = 0;
	p80211msg_dot11req_associate_t	*msg = msgp;
	DBFENTER;

	/* Never supported on AP */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
#else
#error "WLAN_STA or WLAN_AP not defined!"
#endif
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_reassociate
*
* Renew association because of a BSS change.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_reassociate(void *msgp)
{
#if defined(WLAN_STA)
/*	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	hfa384x_t		*hw = priv->hw; */
	int 			result = 0;
	p80211msg_dot11req_reassociate_t	*msg = msgp;
	DBFENTER;

	/* TODO: Not supported yet...not sure how we're going to do it */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
#elif defined(WLAN_AP)
	int 					result = 0;
	p80211msg_dot11req_reassociate_t	*msg = msgp;
	DBFENTER;

	/* Never supported on AP */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
#else
#error "WLAN_STA or WLAN_AP not defined!"
#endif
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_disassociate
*
* Send a disassociation notification.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_disassociate( void *msgp)
{
#if defined(WLAN_STA)
/*	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	hfa384x_t		*hw = priv->hw; */
	int 			result = 0;
	p80211msg_dot11req_disassociate_t	*msg = msgp;
	DBFENTER;

	/* TODO: Not supported yet...not sure how we're going to do it */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
#elif defined(WLAN_AP)
/*	prism2sta_priv_t	*priv = (prism2sta_priv_t*)wlandev->priv;
	hfa384x_t		*hw = priv->hw; */
	int 			result = 0;
	p80211msg_dot11req_disassociate_t	*msg = msgp;
	DBFENTER;

	/* TODO: Not supported yet...not sure how we're going to do it */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
#else
#error "WLAN_STA or WLAN_AP not defined!"
#endif
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_start
*
* Start a BSS.  Any station can do this for IBSS, only AP for ESS.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
int prism2mgmt_start(ether_dev_info_t *wlandev, void *msgp)
{
#if defined(WLAN_STA)
	int 				result = 0;
	p80211msg_dot11req_start_t	*msg = msgp;
	DBFENTER;


	/* Ad-Hoc not quite supported on Prism2 */
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_not_supported;
	result = 0;

#elif defined(WLAN_AP)
	prism2sta_priv_t	*priv = (prism2sta_priv_t*)&wlandev->priv;
	hfa384x_t		*hw = &wlandev->hw;
	int 			result = 0;
	p80211msg_dot11req_start_t	*msg = msgp;
	p80211pstrd_t		*pstr;	
	UINT8			bytebuf[80];
	hfa384x_bytestr_t	*p2bytestr = (hfa384x_bytestr_t*)bytebuf;
	hfa384x_PCFInfo_data_t	*pcfinfo = (hfa384x_PCFInfo_data_t*)bytebuf;
	UINT16			word;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	/* Validate the command, if BSStype=infra is the tertiary loaded? */
	if ( msg->bsstype.data == P80211ENUM_bsstype_independent ) {
		dprintf("prism: prism2mgmt_start() AP driver cannot create IBSS.\n");
		goto failed;
	} else if ( priv->cap_sup_sta.id != 5) {
		drpintf("prism: prism2mgmt_start() AP driver failed to detect AP firmware.\n");
		goto failed;
	}

	/* Set the REQUIRED config items */
	/* SSID */
	pstr = (p80211pstrd_t*)&(msg->ssid.data);
	prism2mgmt_pstr2bytestr(p2bytestr, pstr);
	result = hfa384x_drvr_setconfig( hw, HFA384x_RID_CNFOWNSSID,
				bytebuf, HFA384x_RID_CNFOWNSSID_LEN);
	if ( result ) {
		dprintf("prism: prism2mgmt_start() Failed to set SSID\n");
		goto failed;
	}

	/* bsstype - we use the default in the ap firmware */

	/* beacon period */
	word = msg->beaconperiod.data;
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFAPBCNINT, &word);
	if ( result ) {
		dprintf("prism: prism2mgmt_start() Failed to set beacon period=%d.\n", word);
		goto failed;
	}

	/* dschannel */
	word = msg->dschannel.data;
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFOWNCHANNEL, &word);
	if ( result ) {
		dprintf("prism: prism2mgmt_start() Failed to set channel=%d.\n", word);
		goto failed;
	}
	/* Basic rates */
	word = p80211rate_to_p2bit(msg->basicrate1.data);
	if ( msg->basicrate2.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate2.data);
	}
	if ( msg->basicrate3.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate3.data);
	}
	if ( msg->basicrate4.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate4.data);
	}
	if ( msg->basicrate5.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate5.data);
	}
	if ( msg->basicrate6.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate6.data);
	}
	if ( msg->basicrate7.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate7.data);
	}
	if ( msg->basicrate8.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->basicrate8.data);
	}
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFBASICRATES, &word);
	if ( result ) {
		dprintf("prism: prism2mgmt_start() Failed to set basicrates=%d.\n", word);
		goto failed;
	}

	/* Operational rates (supprates and txratecontrol) */
	word = p80211rate_to_p2bit(msg->operationalrate1.data);
	if ( msg->operationalrate2.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate2.data);
	}
	if ( msg->operationalrate3.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate3.data);
	}
	if ( msg->operationalrate4.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate4.data);
	}
	if ( msg->operationalrate5.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate5.data);
	}
	if ( msg->operationalrate6.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate6.data);
	}
	if ( msg->operationalrate7.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate7.data);
	}
	if ( msg->operationalrate8.status == P80211ENUM_msgitem_status_data_ok ) {
		word |= p80211rate_to_p2bit(msg->operationalrate8.data);
	}
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFSUPPRATES, &word);
	if ( result ) {
		dprintf("prism: Failed to set supprates=%d.\n", word);
		goto failed;
	}
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_TXRATECNTL0, &word);
	if ( result ) {
		dprintf("prism: Failed to set txrates=%d.\n", word);
		goto failed;
	}

	/* ibssatimwindow */
	dprintf("prism: atimwindow not used in Infrastructure mode, ignored.\n");

	/* DTIM period */
	word = msg->dtimperiod.data;
	result = hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFOWNDTIMPER, &word);
	if ( result ) {
		dprintf("prism: prism2mgmt_start() Failed to set dtim period=%d.\n", word);
		goto failed;
	}

	/* probedelay */
	dprintf("prism2mgmt_start: probedelay not supported in prism2, ignored.\n");

	/* cfpollable, cfpollreq, cfpperiod, cfpmaxduration */
	if (msg->cfpollable.data == P80211ENUM_truth_true && 
	    msg->cfpollreq.data == P80211ENUM_truth_true ) {
		dprintf("prism: cfpollable=cfpollreq=true is illegal.\n");
		result = -1;
		goto failed;
	}

	/* read the PCFInfo and update */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CNFAPPCFINFO, 
					pcfinfo, HFA384x_RID_CNFAPPCFINFO_LEN);
	if ( result ) {
		dprintf("prism2mgmt_start: read(pcfinfo) failed, assume it's not supported, pcf settings ignored.\n");
		goto pcf_skip;
	}
	if ((msg->cfpollable.data == P80211ENUM_truth_false && 
	     msg->cfpollreq.data == P80211ENUM_truth_false) ) {
	    	pcfinfo->MediumOccupancyLimit = 0;
		pcfinfo->CFPPeriod = 0;
		pcfinfo->CFPMaxDuration = 0;
		pcfinfo->CFPFlags &= host2hfa384x_16((UINT16)~BIT0);
		
		if ( msg->cfpperiod.data == P80211ENUM_msgitem_status_data_ok ||
		     msg->cfpmaxduration.data == P80211ENUM_msgitem_status_data_ok ) {
			dprintf("Setting cfpperiod or cfpmaxduration when "
				"cfpollable and cfreq are false is pointless.\n");
		}
	}
	if ((msg->cfpollable.data == P80211ENUM_truth_true ||
	     msg->cfpollreq.data == P80211ENUM_truth_true) ) {
		if ( msg->cfpollable.data == P80211ENUM_truth_true) {
			pcfinfo->CFPFlags |= host2hfa384x_16((UINT16)BIT0);
		}

		if ( msg->cfpperiod.status == P80211ENUM_msgitem_status_data_ok) {
			pcfinfo->CFPPeriod = msg->cfpperiod.data;
			pcfinfo->CFPPeriod = host2hfa384x_16(pcfinfo->CFPPeriod);
		}

		if ( msg->cfpmaxduration.status == P80211ENUM_msgitem_status_data_ok) {
			pcfinfo->CFPMaxDuration = msg->cfpmaxduration.data;
			pcfinfo->CFPMaxDuration = host2hfa384x_16(pcfinfo->CFPMaxDuration); 
			pcfinfo->MediumOccupancyLimit = pcfinfo->CFPMaxDuration;
		}
	}
	result = hfa384x_drvr_setconfig(hw, HFA384x_RID_CNFAPPCFINFO, 
					pcfinfo, HFA384x_RID_CNFAPPCFINFO_LEN);
	if ( result ) {
		dprintf("prism: write(pcfinfo) failed.\n");
		goto failed;
	}

pcf_skip:
	/* Enable the interrupts */
	word = HFA384x_INTEN_INFDROP_SET(1) |
		HFA384x_INTEN_INFO_SET(1) |
		HFA384x_INTEN_ALLOC_SET(1) |
		HFA384x_INTEN_TXEXC_SET(1) |
		HFA384x_INTEN_DTIM_SET(1) |
		HFA384x_INTEN_TX_SET(1) |
		HFA384x_INTEN_RX_SET(1);
	wlan_outw_cpu_to_le16(0xffff, HFA384x_EVSTAT(priv->hw->iobase));
	wlan_outw_cpu_to_le16(word, HFA384x_INTEN(hw->iobase));

	/* Set the macmode so the frame setup code knows what to do */
	if ( msg->bsstype.data == P80211ENUM_bsstype_infrastructure ) {
		wlandev->macmode = WLAN_MACMODE_ESS_AP;
		word=2304;  /* lets extend the data length a bit */
		hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFMAXDATALEN, &word);
	}

	/* Set the BSSID to the same as our MAC */
	memcpy( wlandev->bssid, wlandev->netdev->dev_addr, WLAN_BSSID_LEN);

	/* Enable the Port */
	result = hfa384x_cmd_enable(hw, 0);
	if ( result ) {
		dprintf("prism: Enable macport failed, result=%d.\n", result);
		goto failed;
	}
	
	msg->resultcode.data = P80211ENUM_resultcode_success;
	goto done;
failed:
	dprintf("prism: Failed to set a config option, result=%d\n", result);
	msg->resultcode.data = P80211ENUM_resultcode_invalid_parameters;
done:
	result = 0;
#else
#error "WLAN_STA or WLAN_AP not defined!"
#endif
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2mgmt_readpda
*
* Collect the PDA data and put it in the message.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_readpda(ether_dev_info_t *wlandev, void *msgp)
{
	prism2sta_priv_t	*priv = (prism2sta_priv_t*)&wlandev->priv;
	p80211msg_p2req_readpda_t	*msg = msgp;
	DBFENTER;

	/* This driver really shouldn't be active if we weren't able */
	/*  to read a PDA from a card.  Therefore, we assume the pda */
	/*  in priv->pda is good. */
	memcpy( msg->pda.data, priv->pda, HFA384x_PDA_LEN_MAX);
	msg->pda.status = P80211ENUM_msgitem_status_data_ok;

	msg->resultcode.data = P80211ENUM_resultcode_success;
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_auxport_state
*
* Enables/Disables the card's auxiliary port.  Should be called 
* before and after a sequence of auxport_read()/auxport_write() 
* calls.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_auxport_state(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_auxport_state_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	if (msg->enable.data == P80211ENUM_truth_true) {
		if ( hfa384x_cmd_aux_enable(hw) ) {
			msg->resultcode.data = P80211ENUM_resultcode_implementation_failure;
		} else {
			msg->resultcode.data = P80211ENUM_resultcode_success;
		}
	} else {
		hfa384x_cmd_aux_disable(hw);
		msg->resultcode.data = P80211ENUM_resultcode_success;
	}

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_auxport_read
*
* Copies data from the card using the auxport.  The auxport must
* have previously been enabled.  Note: this is not the way to 
* do downloads, see the [ram|flash]dl functions.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_auxport_read(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_auxport_read_t	*msg = msgp;
	UINT32			addr;
	UINT32			len;
	UINT8*			buf;
	UINT32			maxlen = sizeof(msg->data.data);
	DBFENTER;

	if ( hw->auxen ) {
		addr = msg->addr.data;
		len = msg->len.data;
		buf = msg->data.data;
		if ( len <= maxlen ) {  /* max read/write size */
			hfa384x_copy_from_aux(hw, addr, buf, len);
		} else {
			dprintf("prism: Attempt to read > maxlen from auxport.\n");
			msg->resultcode.data = P80211ENUM_resultcode_refused;
		}

	} else {
		msg->resultcode.data = P80211ENUM_resultcode_refused;
	}
	msg->data.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_auxport_write
*
* Copies data to the card using the auxport.  The auxport must
* have previously been enabled.  Note: this is not the way to 
* do downloads, see the [ram|flash]dl functions.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_auxport_write(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_auxport_write_t	*msg = msgp;
	UINT32			addr;
	UINT32			len;
	UINT8*			buf;
	UINT32			maxlen = sizeof(msg->data.data);
	DBFENTER;

	if ( hw->auxen ) {
		addr = msg->addr.data;
		len = msg->len.data;
		buf = msg->data.data;
		if ( len <= maxlen ) {  /* max read/write size */
			hfa384x_copy_to_aux(hw, addr, buf, len);
		} else {
			dprintf("prism: Attempt to write > maxlen from auxport.\n");
			msg->resultcode.data = P80211ENUM_resultcode_refused;
		}

	} else {
		msg->resultcode.data = P80211ENUM_resultcode_refused;
	}
	msg->data.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	DBFEXIT;
	return 0;
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







/*----------------------------------------------------------------
* prism2mgmt_test_command
*
* Puts the card into the desired test mode.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_test_command(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_test_command_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	/* call some routine to execute the test command */

	hfa384x_drvr_test_command(hw, msg->testcode.data, msg->testparam.data);

	msg->resultcode.data = P80211ENUM_resultcode_success;

	DBFEXIT;
	return 0;
}

/*----------------------------------------------------------------
* prism2mgmt_mmi_read
*
* Read from one of the MMI registers.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_mmi_read(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_mmi_read_t	*msg = msgp;

	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	/* call some routine to execute the test command */

	hfa384x_drvr_mmi_read(hw, msg->addr.data);

	/* I'm not sure if this is "architecturally" correct, but it
           is expedient. */

	msg->value.data = (UINT32) hw->resp0;
	msg->resultcode.data = P80211ENUM_resultcode_success;

	DBFEXIT;
	return 0;
}

/*----------------------------------------------------------------
* prism2mgmt_mmi_write
*
* Write a data value to one of the MMI registers.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_mmi_write(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_mmi_write_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;

	/* call some routine to execute the test command */

	hfa384x_drvr_mmi_write(hw, msg->addr.data, msg->data.data);

	msg->resultcode.data = P80211ENUM_resultcode_success;

	DBFEXIT;
	return 0;
}

/*----------------------------------------------------------------
* prism2mgmt_ramdl_state
*
* Establishes the beginning/end of a card RAM download session.
*
* It is expected that the ramdl_write() function will be called 
* one or more times between the 'enable' and 'disable' calls to
* this function.
*
* Note: This function should not be called when a mac comm port 
*       is active.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_ramdl_state(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_ramdl_state_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	if  ( msg->enable.data == P80211ENUM_truth_true ) {
		if ( hfa384x_drvr_ramdl_enable(hw, msg->exeaddr.data) ) {
			msg->resultcode.data = P80211ENUM_resultcode_implementation_failure;
		} else {
			msg->resultcode.data = P80211ENUM_resultcode_success;
		}
	} else {
		hfa384x_drvr_ramdl_disable(hw);
		msg->resultcode.data = P80211ENUM_resultcode_success;

		/*TODO: Reset everything....the MAC just restarted */
		udelay(1000);
		prism2sta_initmac(wlandev);
	}

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_ramdl_write
*
* Writes a buffer to the card RAM using the download state.  This
* is for writing code to card RAM.  To just read or write raw data
* use the aux functions.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_ramdl_write(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_ramdl_write_t	*msg = msgp;
	UINT32			addr;
	UINT32			len;
	UINT8			*buf;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	/* first validate the length */
	if  ( msg->len.data > sizeof(msg->data.data) ) {
		msg->resultcode.status = P80211ENUM_resultcode_invalid_parameters;
		return 0;
	}
	/* call the hfa384x function to do the write */
	addr = msg->addr.data;
	len = msg->len.data;
	buf = msg->data.data;
	if ( hfa384x_drvr_ramdl_write(hw, addr, buf, len) ) {
		msg->resultcode.data = P80211ENUM_resultcode_refused;
		
	}
	msg->resultcode.data = P80211ENUM_resultcode_success;

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_flashdl_state
*
* Establishes the beginning/end of a card Flash download session.
*
* It is expected that the flashdl_write() function will be called 
* one or more times between the 'enable' and 'disable' calls to
* this function.
*
* Note: This function should not be called when a mac comm port 
*       is active.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_flashdl_state(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_flashdl_state_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	if  ( msg->enable.data == P80211ENUM_truth_true ) {
		if ( hfa384x_drvr_flashdl_enable(hw) ) {
			msg->resultcode.data = P80211ENUM_resultcode_implementation_failure;
		} else {
			msg->resultcode.data = P80211ENUM_resultcode_success;
		}
	} else {
		hfa384x_drvr_flashdl_disable(hw);
		msg->resultcode.data = P80211ENUM_resultcode_success;

		/*TODO: Reset everything....the MAC just restarted */
		udelay(1000);
		prism2sta_initmac(wlandev);
	}

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_flashdl_write
*
* 
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_flashdl_write(ether_dev_info_t *wlandev, void *msgp)
{
	hfa384x_t		*hw = &wlandev->hw;
	p80211msg_p2req_flashdl_write_t	*msg = msgp;
	UINT32			addr;
	UINT32			len;
	UINT8			*buf;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	/* first validate the length */
	if  ( msg->len.data > sizeof(msg->data.data) ) {
		msg->resultcode.status = P80211ENUM_resultcode_invalid_parameters;
		return 0;
	}
	/* call the hfa384x function to do the write */
	addr = msg->addr.data;
	len = msg->len.data;
	buf = msg->data.data;
	if ( hfa384x_drvr_flashdl_write(hw, addr, buf, len) ) {
		msg->resultcode.data = P80211ENUM_resultcode_refused;
		
	}
	msg->resultcode.data = P80211ENUM_resultcode_success;

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_mm_dscpmap
*
* Maps a given diffserv level to one of the output queues.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_mm_dscpmap(ether_dev_info_t *wlandev, void *msgp)
{
	prism2sta_priv_t		*priv = (prism2sta_priv_t*)&wlandev->priv;
	p80211msg_p2req_mm_dscpmap_t	*msg = msgp;
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_success;
	priv->qos_dscpmap[msg->dscp.data] = msg->macqueue.data;

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2mgmt_dump_state
*
* Dumps the driver's and hardware's current state via the kernel 
* log at KERN_NOTICE level.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
*	0	success and done
*	<0	success, but we're waiting for something to finish.
*	>0	an error occurred while handling the message.
* Side effects:
*
* Call context:
*	process thread  (usually)
----------------------------------------------------------------*/
int prism2mgmt_dump_state(ether_dev_info_t *wlandev, void *msgp)
{
	prism2sta_priv_t		*priv = (prism2sta_priv_t*)&wlandev->priv;
	hfa384x_t			*hw = &wlandev->hw;
	p80211msg_p2req_dump_state_t	*msg = msgp;
	int				result;
	UINT16				auxbuf[15];
	DBFENTER;

	msg->resultcode.status = P80211ENUM_msgitem_status_data_ok;
	msg->resultcode.data = P80211ENUM_resultcode_success;

	dprintf("prism: driver and hardware state:\n");
	if  ( (result = hfa384x_cmd_aux_enable(hw)) ) {
		dprintf("prism: aux_enable failed, result=%d\n", result);
		goto failed;
	}
	hfa384x_copy_from_aux(hw, 0x01e2, auxbuf, sizeof(auxbuf));
	hfa384x_cmd_aux_disable(hw);
	dprintf("prism: cmac: FreeBlocks=%d\n", auxbuf[5]);
	dprintf("prism:  cmac: IntEn=0x%02x EvStat=0x%02x\n", 
		wlan_inw_le16_to_cpu(HFA384x_INTEN(hw->iobase)),
		wlan_inw_le16_to_cpu(HFA384x_EVSTAT(hw->iobase)));
	dprintf("prism:  drvr: txfid_top=%d stacksize=%d\n",
		priv->txfid_top,PRISM2_FIDSTACKLEN_MAX);

failed:	
	DBFEXIT;
	return 0;
}



