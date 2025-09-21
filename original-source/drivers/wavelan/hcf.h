
#ifndef HCF_H
#define HCF_H 1

/*************************************************************************************************************
*
* FILE	 : hcf.h *************** 2.0 *************************************************************************
*
* DATE   : $Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR : Nico Valster
*
* DESC   : Definitions and Prototypes for MSF as well as HCF sources
*
*			Customizable via HCFCFG.H
*
*
**************************************************************************************************************
Instructions to convert HCF.H to HCF.INC by means of H2INC

Use a command line which defines the specific macros and command line options
needed to build the C-part, e.g. for the DOS ODI driver
		`h2inc /C /Ni /Zp /Zn hcf	 hcf.h`


**************************************************************************************************************
* COPYRIGHT (c) 1996, 1997, 1998, 1999, 2000 by Lucent Technologies.	 All Rights Reserved.
**************************************************************************************************************/

/**************************************************************************************************************
*
* CHANGE HISTORY
*
  961018 - NV
	Original Entry

$Log: hcf.h,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.34   11 Feb 2000 14:19:22   NVALST
 * sync with home
 * 
 *    Rev 1.33   11 Feb 2000 10:17:54   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.32   08 Feb 2000 09:28:32   NVALST
 * passed to reinier
 * 
 *    Rev 1.31   04 Feb 2000 14:44:42   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.30   04 Feb 2000 14:10:14   NVALST
 * intermediate, backup before inserting CDOC comments
 * 
 *    Rev 4.6   01 Feb 2000 12:02:28   NVALST
 * 
 *    Rev 4.6   28 Jan 2000 15:39:42   NVALST
 * 
 *    Rev 1.29   28 Jan 2000 13:27:42   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 17:23:54   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 15:27:54   NVALST
 * 
 *    Rev 1.8   03 Nov 1999 16:43:02   NVALST
 * 
 *    Rev 4.2   10 Sep 1999 11:28:42   NVALST
 * 
 *    Rev 1.187   10 Sep 1999 11:25:12   NVALST
 * 
 *    Rev 4.2   09 Sep 1999 13:34:26   NVALST
 * 
 *    Rev 1.186   07 Sep 1999 13:23:36   NVALST
 * 
 *    Rev 4.1   31 Aug 1999 12:14:40   NVALST
 * 
 *    Rev 1.181   30 Aug 1999 11:58:42   NVALST
 * 
 *    Rev 4.0   07 Jul 1999 10:09:22   NVALST
 * 
 *    Rev 1.168   07 Jul 1999 09:45:06   NVALST
 * 
 *    Rev 2.22   29 Jun 1999 11:32:36   NVALST
 * 
 *    Rev 1.167   24 Jun 1999 11:19:52   NVALST
 * 
 *    Rev 2.21   23 Jun 1999 09:27:36   NVALST
 * 
 *    Rev 1.166   22 Jun 1999 14:24:28   NVALST
 * 
****************************************************************************/




#include "hcfcfg.h"	// System Constants to be defined by the MSF-programmer to tailor the HCF
#include "mdd.h"	// Include file common for HCF, MSF, UIL, USF

/************************************************************************************************************/
/******************   H C F  F U N C T I O N   P A R A M E T E R	 ****************************************/
/************************************************************************************************************/

//offsets for hcf_put_data and hcf_get_data
				

// 802.3/E-II/802.11 offsets to access Hermes control fields  ;?BElong in MDD,H ???????????????????????????????
#define HFS_STAT				-0x2E	//0x0000
#define 	HFS_STAT_ERR		RX_STAT_ERR	//link "natural" HCF name to "natural" MSF name

#define HFS_Q_INFO				-0x28	//0x0006
#define HFS_TX_CNTL				-0x22	//0x000C
#define HFS_FRAME_CNTL			-0x20	//0x000E
#define HFS_ID					-0x1E	//0x0010

// 802.11 relative offsets to access 802.11 header fields
#define HFS_ADDR_1				0x00	//0x0012
#define HFS_ADDR_2				0x06	//0x0018
#define HFS_ADDR_3				0x0C	//0x001E
#define HFS_SEQ_CNTL			0x12	//0x0024
#define HFS_ADDR_4				0x14	//0x0026
#define HFS_DAT_LEN				0x1A	//0x002C

// 802.3 / E-II relative offsets to access 802.3 header fields
#define HFS_ADDR_DEST			0x00	//0x002E
#define HFS_ADDR_SRC			0x06	//0x0034
#define HFS_LEN					0x0C	//0x003A
#define HFS_DAT					0x0E	//0x003C

// E-II relative offsets to access SNAP header fields
#define HFS_TYPE				0x14	//0x0042	//Eternet-II type in 1042/Bridge-Tunnel encapsulated frame
#define HFS_WDM_TYPE			0x17	//0x0045	//PDU type in WDM frame


//#define HCF_ACT_INT_PENDING	0x0001		//interrupt pending, return status HCF_ACT_INT_OFF



/*************************************************************************************************************/
/****************   H C F  F U N C T I O N   R E T U R N   C O D E S   ***************************************/
/*************************************************************************************************************/

//Debug Purposes only				!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define HREG_EV_TICK		0x8000	//WMAC Controller Auxiliary Timer Tick
#define HREG_EV_RES			0x4000	//WMAC Controller H/W error (Wait Time-out)
#define HREG_EV_INFO_DROP	0x2000	//WMAC did not have sufficient RAM to build Unsollicited Frame
#define HREG_EV_NO_CARD		0x0800	/* PSEUDO event: card removed											  */
#define HREG_EV_DUIF_RX     0x0400  /* PSEUDO event: WMP frame received										  */
#define HREG_EV_INFO		0x0080	//WMAC Controller Asynchronous Information Frame
#define HREG_EV_CMD			0x0010	//WMAC Controller Command completed, Status and Response avaialble
#define HREG_EV_ALLOC		0x0008	//WMAC Controller Asynchronous part of Allocation/Reclaim completed
#define HREG_EV_TX_EXC		0x0004	//WMAC Controller Asynchronous Transmission unsuccessful completed
#define HREG_EV_TX			0x0002	//WMAC Controller Asynchronous Transmission successful completed
#define HREG_EV_RX			0x0001	//WMAC Controller Asynchronous Receive Frame



//=========================================  T A L L I E S  ===================================================

typedef struct CFG_HERMES_TALLIES_STRCT {  //Hermes Tallies (IFB substructure)
  hcf_32	TxUnicastFrames;
  hcf_32	TxMulticastFrames;
  hcf_32	TxFragments;
  hcf_32	TxUnicastOctets;
  hcf_32	TxMulticastOctets;
  hcf_32	TxDeferredTransmissions;
  hcf_32	TxSingleRetryFrames;
  hcf_32	TxMultipleRetryFrames;
  hcf_32	TxRetryLimitExceeded;
  hcf_32	TxDiscards;
  hcf_32	RxUnicastFrames;
  hcf_32	RxMulticastFrames;
  hcf_32	RxFragments;
  hcf_32	RxUnicastOctets;
  hcf_32	RxMulticastOctets;
  hcf_32	RxFCSErrors;
  hcf_32	RxDiscards_NoBuffer;
  hcf_32	TxDiscardsWrongSA;
  hcf_32	RxWEPUndecryptable;
  hcf_32	RxMsgInMsgFragments;
  hcf_32	RxMsgInBadMsgFragments;
  hcf_32	RxDiscards_WEPICVError;
  hcf_32	RxDiscards_WEPExcluded;
}CFG_HERMES_TALLIES_STRCT;

#if 1																								//<HCFL@
typedef struct CFG_HCF_TALLIES_STRCT {  //HCF Tallies (IFB substructure)
  hcf_32	NoBufInq;					//No buffer available reported by Inquiry command
  hcf_32	NoBufInfo;  				//No buffer available for unsollicited Notify frame
  hcf_32	NoBufMB;					//No space available in MailBox
  hcf_32	MiscErr;					/*Miscellanuous errors
  										@* time out on BAP Initialization
  										@* time out on Tx Frame Allocation
  										@* time out on AUX port En-/Disabling
  										@* time out on completion synchronous part Hermes Command
  										@* synchronously completed Hermes Command doesn't match original command
  										@* Request to put zero-length MailBox Info block
  										@* IFBTickIni based protection counter expired
  										 */
  hcf_32	EngCnt;						/*Engineering Count
  										@* Data Corruption Workaround catched
  										 */
}CFG_HCF_TALLIES_STRCT;
#else																								//@HCFL>
typedef struct CFG_HCF_TALLIES_STRCT {  //HCF Tallies (IFB substructure) (HCF-light "suffers" to get HCFLIB "right"
  hcf_32	dummy[5];
}CFG_HCF_TALLIES_STRCT;
#endif																								//<HCFL>


//Note this way to define CFG_TALLIES_STRCT_SIZE implies that all tallies must keep the same (hcf_32) size
#define		HCF_NIC_TAL_CNT	(sizeof(CFG_HERMES_TALLIES_STRCT)/ sizeof(hcf_32))
#define		HCF_HCF_TAL_CNT	(sizeof(CFG_HCF_TALLIES_STRCT)   / sizeof(hcf_32))
#define		HCF_TOT_TAL_CNT	(HCF_NIC_TAL_CNT + HCF_HCF_TAL_CNT)

/************************************************************************************************************/
/***********   W C I    F U N C T I O N S    P R O T O T Y P E S   ******************************************/
/************************************************************************************************************/

#if 1 //<HCFL@
#define IFB_VERSION 7		 			/* initially 0, to be incremented by every IFB layout change		*/
#else //@HCFL>
#define IFB_VERSION 0x87	 			/* initially 80, to be incremented by every IFB layout change		*/
#endif //<HCFL>


/* identifier IFB_STRCT on typedef line needed to get the individual fields in the MS Browser DataBase	*/
typedef struct IFB_STRCT{               //I/F Block
/************************************************************************************************************/
/* part I (survives hcf_initialize)  ************************************************************************/
/************************************************************************************************************/
  hcf_io		IFB_IOBase;				/* I/O address of Hermes chip as passed by MSF at hcf_connect call	*/
#if defined HCF_PORT_IO
  hcf_16		IFB_IOBase_pad;			// Optional field, makes IFB-layout independent of IFB_IOBase size
#endif //HCF_PORT_IO
  hcf_16		IFB_IORange;			// I/O Range used by Hermes chip
  hcf_8			IFB_Version;			/* initially 0, to be incremented by every IFB layout change		*/
  hcf_8			IFB_Slack_2;			/* align/slack space												*/
  hcf_8			IFB_HCFVersionMajor;	// Major version of the HCF.0x01 for this release
  hcf_8			IFB_HCFVersionMinor;	/* Minor version of the HCF.  Incremented for each coding maintenance
  										 * cycle. 0x01 for the Initial release								*/
#if defined HINC  																					//<HCFL@
  hcf_32		IFB_NIC_Tallies[HCF_NIC_TAL_CNT];	//Hermes tallies
  hcf_32		IFB_HCF_Tallies[HCF_HCF_TAL_CNT];	//HCF tallie
#else																								//@HCFL>
  CFG_HERMES_TALLIES_STRCT	IFB_NIC_Tallies;	//Hermes tallies
  CFG_HCF_TALLIES_STRCT		IFB_HCF_Tallies;	//HCF tallies
#endif // HINC  																					//<HCFL>	
  hcf_16		IFB_CardStat;			/*                													*/
  hcf_16 		IFB_PortStat;			// bit flags representing individual enabled ports			//<HCFL>
  hcf_16		IFB_FrameType;			/*                											<HCFL>	*/
  wci_recordp	IFB_MBp;				/*                											<HCFL@ 	*/
  hcf_16		IFB_MBSize;				/*                												 	*/
  hcf_16		IFB_MBWp;				/*                													*/
  hcf_16		IFB_MBRp;				/*                													*/
  hcf_16		IFB_MBInfoLen;			/*                											@HCFL>	*/
  hcf_16		IFB_FSBase;				// frame type dependent offset (HFS_ADDR_1_ABS or HFS_ADDR_DEST_ABS)
  hcf_16		IFB_RxFence;			// frame type dependent gap fence (HFS_ADDR_DEST_ABS or HFS_LEN_ABS)
  hcf_16		IFB_IntOffCnt;			/*                													*/
  hcf_32	 	IFB_TickIni;			/* initialization of S/W counter based protection loop
  										 * Note that via IFB_DefunctStat time outs in cmd_wait and in
  										 * hcfio_string block all Hermes access till the next hcf_initialize
  										 * so functions which call a mix of cmd_wait and hcfio_string only
  										 * need to check the return status of the last call					*/
  hcf_16		IFB_Magic;				/*                													*/


  hcf_16		IFB_LogLvl;				// Log Filtering
  MSF_LOG_RTNP	IFB_LogRtn;				// MSF Log Call back routine 										 <HCFL>
  MSF_ASSERT_RTNP	IFB_AssertRtn;		// MSF Assert Call back routine (inspired by GEF, DrDobbs Nov 1998 ) <HCFL>
  hcf_16		IFB_MSFType;			/* Emergency escape controlled via hcf_put_info, used for:
  										 *  - hybrid AP/STA:  0 = STA, 1 = AP 
  										 */
  hcf_8 		IFB_TxCntl[2];			/* contents of HFS_TX_CNTL field of TFS
  										 * 0: StrucType,TxEx,TxOK   1: MACPort								*/
/************************************************************************************************************/
/* part II (cleared or re-initialized at hcf_initialize)   *************************************************/
/************************************************************************************************************/
  hcf_8  		IFB_PIFRscInd;			/*                   //;?Q:int better than hcf_8 A: No!				*/
  hcf_8			IFB_DUIFRscInd;			/* Value indicating the command resource availability for the
  										 * Driver-Utility I/F (i.e. hcf_send_diag_msg).						*/
  										/* Values: */
  										/* * No command resource		0									*/
  										/* * Command resource available	01h-FFh								*/
  hcf_8  		IFB_NotifyRscInd;		/*                   //;?Q:int better than hcf_8 A: No!				*/
  hcf_8			IFB_Slack_6;			/* align/slack space												*/
  hcf_16		IFB_PIF_FID;			/*                													*/
  hcf_16		IFB_DUIF_FID;			/* field which contains FID value identifying the Tx Frame Structure,
  										 * to be used by hcf_send_diag_msg									*/
  hcf_16		IFB_Notify_FID;			/* field which contains FID value identifying the Notify Frame Struct
  										 * to be used by hcf_put_info in case of Notify type codes			*/
  hcf_16		IFB_RxFID;				/*                													*/
  hcf_16		IFB_MB_FID;				/* pass appropriate FID to hcf_put_mb_info							*/
  hcf_16		IFB_TxFrameType;		/*                													*/
//  hcf_16		IFB_RxLen;				/*                													*/
  hcf_16		IFB_RxStat;				/*                													*/
  hcf_16		IFB_UnloadIdx;			/*                													*/
  hcf_16		IFB_PIFLoadIdx;			/*                													*/
  hcf_16		IFB_Slack_7;			/* align/slack space												*/
  hcf_16		IFB_BAP_0[2];			/* offset
  										 * RID/FID															*/
  hcf_16		IFB_BAP_1[2];			/* offset
  										 * RID/FID															*/
/* the fields after this text must stay concatenated because of their (mis-)use as LTV record	<HCFL@		*/
  hcf_16		IFB_DLTarget[2];		// 32-bits (don't confuse with FAR) Pointer to NV-RAM download target
#if defined HCF_STA			
  hcf_16		IFB_DLPage;				/* Download Buffer Page, must be concatenated with IFB_DLTarget		*/
  hcf_16		IFB_DLOffset;			/* Download Buffer Offset, must be concatenated with IFB_DLPage		*/
  hcf_16		IFB_DLLen;				/* Download Buffer Length, must be concatenated with IFB_DLOffset	*/
#else
  hcf_16		IFB_Slack_8[3];			/* align/slack space												*/
#endif //HCF_STA
  /* the fields above this text must stay concatenated because of their (mis-)use as LTV record	@HCFL>		*/
  hcf_16		IFB_RxLen;				/*   moved to see wheter someone corrupts it uinder BeOS             													*/
  hcf_16		IFB_IntEnMask;			/*                													*/
  hcf_16		IFB_DefunctStat;		/* BAP initialization or Cmd Completion failed once
  										 * see remark at IFB_TickIni definition								*/
  hcf_16		IFB_ErrCmd;				// contents Status reg when error bits and/or mismatch in cmd_wait
  hcf_16		IFB_ErrQualifier;		// contents Resp0  reg when error bits and/or mismatch in cmd_wait
  hcf_16		IFB_EngFlg;				/* Engineering Flags
  										 *	0x0001	WMP filtering on PDU type								*/
  hcf_16		IFB_AssertTrace;		/* bit based trace off all hcf_.... invokations				<HCFL@	*/
  										/* keep IFB_CfgTbl and IFB_End adjacent,
  										 * see clearing IFB in hcf_disable									*/
  hcf_16		IFB_AssertLvl;			// Assert Filtering, Not yet implemented
  hcf_16		IFB_WarningInfo[2];		/* [1] HCF controlled warning bits
  										 *	0 - 0x0001 : Power management
  										 *  0 - 0x0002 : unused
  										 :
  										 *  0 - 0x8000 : unused
  										 * [2] MSF controlled warning bits
  										 */                    										//@HCFL>	
  hcf_16		IFB_LinkStat;			/* Link Status
										 * 1 Connected, 2 Disconnected,
										 * 3 AP change, 4 AP out of range, 5 AP in range					*/
  hcf_16		IFB_PRICfg;				// Primary		Top/Bottom Supplier Range					<HCFL@
  hcf_16		IFB_HSICfg;				// H/W-S/W I/F	Top/Bottom Supplier Range
  hcf_16		IFB_STACfg;				// Station		Top/Bottom Supplier Range					@HCFL>
  hcf_16		IFB_CfgTblSize;
/************************************************************************************************************/
/* part III (survives hcf_initialize)  **********************************************************************/
/************************************************************************************************************/
#if HCF_MAX_CONFIG != 0                                                                             //<HCFL@	
  hcf_16		IFB_CfgTbl[HCF_MAX_CONFIG];    /* concatenated data of all hcf_put_record calls		*/
#endif //HCF_MAX_CONFIG																				//@HCFL>	

}IFB_STRCT;

typedef struct ASSERT_STRCT {
	hcf_16	len;					//length of assert_strct
	hcf_16	trace;					//trace log copy from IFB
	hcf_16	qualifier;				//qualifier from entry parameter
	hcf_16	line_number;			//line number from entry parameter
	char val[1];
} ASSERT_STRCT;



typedef IFB_STRCT*	IFBP;


EXTERN_C int	hcf_action			(IFBP ifbp, hcf_action_cmd cmd );
EXTERN_C void	hcf_assert			(IFBP ifbp, struct ASSERT_STRCT* assert_strct, unsigned int line_number, int q );
EXTERN_C void	hcf_connect			(IFBP ifbp, hcf_io io_base );
EXTERN_C int	hcf_debug_trigger	(IFBP ifbp, int where, int what );
EXTERN_C int	hcf_disable			(IFBP ifbp, hcf_16 port );
EXTERN_C void	hcf_disconnect		(IFBP ifbp );
EXTERN_C int	hcf_enable			(IFBP ifbp, hcf_16 port );
EXTERN_C int	hcf_get_info		(IFBP ifbp, LTVP ltvp );
EXTERN_C int	hcf_get_data		(IFBP ifbp, int offset, wci_bufp bufp, int len );
EXTERN_C int	hcf_service_nic		(IFBP ifbp );
EXTERN_C void	hcf_put_data		(IFBP ifbp, wci_bufp bufp, int len, hcf_16 port );
EXTERN_C int	hcf_put_info		(IFBP ifbp, LTVP ltvp );
EXTERN_C int	hcf_put_header		(IFBP ifbp, hcf_16 offset, wci_bufp bufp, int len, hcf_8 check );
EXTERN_C int	hcf_send			(IFBP ifbp, hcf_16 type );
EXTERN_C int	hcf_send_diag_msg	(IFBP ifbp, hcf_16 type, wci_bufp bufp, int len );




#endif  /* HCF_H */

