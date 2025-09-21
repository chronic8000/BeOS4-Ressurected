
#ifndef HCFDEFC_
#define HCFDEFC_H 1


/*************************************************************************************************
*
* FILE	 : HCFDEFC.H *************** 2.0 *********************
*
* DATE   : $Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR : Nico Valster
*
* DESC   : Definitions and Prototypes for HCF only
*
**************************************************************************************************
* COPYRIGHT (c) 1996, 1997, 1998 by Lucent Technologies.	 All Rights Reserved.
*************************************************************************************************/


/****************************************************************************
$Log: hcfdef.h,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.34   11 Feb 2000 14:19:20   NVALST
 * sync with home
 * 
 *    Rev 1.33   11 Feb 2000 10:17:48   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.32   08 Feb 2000 09:28:28   NVALST
 * passed to reinier
 * 
 *    Rev 1.31   04 Feb 2000 14:44:34   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.30   04 Feb 2000 14:10:08   NVALST
 * intermediate, backup before inserting CDOC comments
 * 
 *    Rev 4.6   01 Feb 2000 12:02:32   NVALST
 * 
 *    Rev 4.6   28 Jan 2000 15:39:48   NVALST
 * 
 *    Rev 1.29   28 Jan 2000 13:27:46   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 17:23:56   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 15:27:56   NVALST
 * 
 *    Rev 1.8   03 Nov 1999 16:43:02   NVALST
 * 
 *    Rev 4.2   10 Sep 1999 11:28:44   NVALST
 * 
 *    Rev 1.187   10 Sep 1999 11:25:12   NVALST
 * 
 *    Rev 4.2   09 Sep 1999 13:34:26   NVALST
 * 
 *    Rev 1.186   07 Sep 1999 13:23:34   NVALST
 * 
 *    Rev 4.1   31 Aug 1999 12:14:42   NVALST
 * 
 *    Rev 1.181   30 Aug 1999 11:58:42   NVALST
 * 
 *    Rev 4.0   07 Jul 1999 10:09:24   NVALST
 * 
 *    Rev 1.168   07 Jul 1999 09:45:06   NVALST
 * 
 *    Rev 2.22   29 Jun 1999 11:32:36   NVALST
 * 
 *    Rev 1.167   24 Jun 1999 11:19:54   NVALST
 * 
 *    Rev 2.21   23 Jun 1999 09:27:42   NVALST
 * 
****************************************************************************/


/****************************************************************************
*
* CHANGE HISTORY
*
  961018 - NV
	Original Entry

**************************************************************************************************/

/************************************************************************************************/
/*********************************  P R E F I X E S  ********************************************/
/************************************************************************************************/
//IFB_		Interface Block
//HCMD_		Hermes Command
//HFS_		Hermes (Transmit/Receive) Frame Structure
//HREG_		Hermes Register

/*************************************************************************************************/


/************************************************************************************************/
/********************************* GENERAL EQUATES **********************************************/
/************************************************************************************************/

#if ! defined STATIC	//;? change to HCF_STATIC some day
#if defined _DEBUG || defined OOL
#define STATIC
#else
#define STATIC		//;?static
#endif //_DEBUG
#endif // STATIC


#define AUX_MAGIC_0				0xFE01
#define AUX_MAGIC_1				0xDC23
#define AUX_MAGIC_2				0xBA45
#define HCF_MAGIC				0x7D37	// "}7" Handle validation
#define DIAG_MAGIC				0x5A5A

#define	PLUG_DATA_OFFSET        0x390000L


#define ONE_SECOND				977		// 977 times a Hermes Timer Tick of 1K microseconds ~ 1 second
#define INI_TICK_INI			0x40000L

#define IO_IN					0		//hcfio_in_string
#define IO_OUT					1		//hcfio_out_string
#define IO_OUT_CHECK			2		//enable Data Corruption Detect on hcfio_out_string

#define INI_COMPLETE			0
#define INI_PARTIAL				1
#define INI_PRIM				2

#define CARD_STAT_ENA_PRES		(CARD_STAT_ENABLED|CARD_STAT_PRESENT)
#define CARD_STAT_PRI_PRES		(CARD_STAT_PRESENT|CARD_STAT_INCOMP_PRI)
#define CARD_STAT_PRI_STA_PRES	(CARD_STAT_PRI_PRES|CARD_STAT_INCOMP_STA)

#define DO_ASSERT				( ifbp->IFB_Magic != HCF_MAGIC && ifbp->IFB_Magic == HCF_MAGIC )	//FALSE without the nasty compiler warning

// trace codes used to trace hcf_.... via IFB_AssertTrace and hcf_debug_trigger
// HCF_ASSERT_MISC is used as an escape to trace other routines via hcf_debug_trigger
#define HCF_ASSERT_ACTION			0x0001
//#define HCF_ASSERT_CONNECT		no use to trace this
#define HCF_ASSERT_DISABLE			0x0002
#define HCF_ASSERT_DISCONNECT		0x0004
#define HCF_ASSERT_ENABLE			0x0008
#define HCF_ASSERT_GET_DATA			0x0010
#define HCF_ASSERT_GET_INFO			0x0020
#define HCF_ASSERT_INITIALIZE		0x0040
#define HCF_ASSERT_RESEND			0x0080
#define HCF_ASSERT_SERVICE_NIC		0x0100
#define HCF_ASSERT_PUT_DATA			0x0200
#define HCF_ASSERT_PUT_INFO			0x0400
#define HCF_ASSERT_PUT_HDR			0x0800
#define HCF_ASSERT_SEND				0x1000
#define HCF_ASSERT_SEND_DIAG_MSG	0x2000
#define HCF_ASSERT_INT_OFF			0x4000
#define HCF_ASSERT_MISC				0x8000	


// trace codes used to trace hcf_.... via IFB_LogRtn
#define HCF_LOG_CMD_WAIT			0x0001
#define HCF_LOG_BAP_SETUP			0x0002
#define HCF_LOG_AUX_CNTL			0x0004
#define HCF_LOG_SERVICE_NIC			0x0008
#define HCF_LOG_TEMP				0x0040
#define HCF_LOG_DEFUNCT				0x0080


#define	CFG_CONFIG_RID_MASK			0xFC00		//CONFIGURATION RECORDS

#define BAP_0					HREG_DATA_0		//Tx-related register set for WMAC buffer access
#define BAP_1					HREG_DATA_1		//non Tx-related register set for WMAC buffer access
/************************************************************************************************/
/***************************** STRUCTURES *******************************************************/
/************************************************************************************************/


//************************* Hermes Receive/Transmit Frame Structures
//HFS_STAT
//see MMD.H for HFS_STAT_ERR
#define 	HFS_STAT_MSG_TYPE	0xE000	//Hermes reported Message Type
#define 	HFS_STAT_1042		0x2000	//RFC1042 Encoded
#define 	HFS_STAT_TUNNEL		0x4000	//Bridge-Tunnel Encoded
#define 	HFS_STAT_WMP_MSG	0x6000	//WaveLAN-II Management Protocol Frame

//HFS_TX_CNTL
#if defined HCF_TX_ERR_NOTIFY
#define 	HFS_TX_OK			0x0002	//Generate TxExc event when transmission fails
#define 	HFS_TX_EX			0x0004	//Generate Tx event when transmission succeeds
#else
#if ! defined HCFCFG_H
error;		//include file sequence is apparently reorganized and violates assumption on which this logic was based
#endif //HCFCFG_H
#define 	HFS_TX_OK			0x0000	//do NOT Generate TxExc event when transmission fails
#define 	HFS_TX_EX			0x0000	//do NOT Generate Tx event when transmission succeeds
#endif //HCF_TX_ERR_NOTIFY
#define 	HFS_TX_CNTL_STRCT_TYPE	0x0018	//StrucType subfield
#define 	HFS_TX_CNTL_802_3	0x0000	//802.3 format
#define 	HFS_TX_CNTL_802_11	0x0008	//802.11 format


//************************* Hermes Register Offsets and Command bits
#define HREG_IO_RANGE			0x40		//I/O Range used by Hermes


//************************* Command/Status
#define HREG_CMD				0x00		//
#define 	HCMD_CMD_CODE			0x3F
#define HREG_PARAM_0			0x02		//
#define HREG_PARAM_1			0x04		//
#define HREG_PARAM_2			0x06		//
#define HREG_STAT				0x08		//
#define 	HREG_STAT_CMD_CODE		0x003F	//
#define		HREG_STAT_DIAG_ERR		0x0100
#define		HREG_STAT_INQUIRE_ERR	0x0500
#define 	HREG_STAT_CMD_RESULT	0x7F00	//
#define HREG_RESP_0				0x0A		//
#define HREG_RESP_1				0x0C		//
#define HREG_RESP_2				0x0E		//


//************************* FID Management
#define HREG_INFO_FID			0x10		//
#define HREG_RX_FID				0x20		//
#define HREG_ALLOC_FID  		0x22		//
#define HREG_TX_COMPL_FID  		0x24		//


//************************* BAP
#define HREG_SELECT_0			0x18		//
#define HREG_OFFSET_0			0x1C		//
//#define 	HREG_OFFSET_BUSY		0x8000	// use HCMD_BUSY
#define 	HREG_OFFSET_ERR			0x4000	//
//rsrvd #define 	HREG_OFFSET_DATA_OFFSET	0x0FFF	//

#define HREG_DATA_0				0x36		//
//rsrvd #define HREG_SELECT_1	0x1A		//
#define HREG_OFFSET_1			0x1E		//

#define HREG_DATA_1				0x38		//


//************************* Event
#define HREG_EV_STAT			0x30		//
#define HREG_INT_EN				0x32		//
#define HREG_EV_ACK				0x34		//


//************************* Host Software
#define HREG_SW_0				0x28		//
#define HREG_SW_1				0x2A		//
#define HREG_SW_2				0x2C		//
//rsrvd #define HREG_SW_3		0x2E		//
//************************* Control and Auxiliary Port

#define HREG_CNTL				0x14		//
#define		HREG_CNTL_AUX_ENA_STAT	0xC000
#define		HREG_CNTL_AUX_DIS_STAT	0x0000
#define		HREG_CNTL_AUX_ENA_CNTL	0x8000
#define		HREG_CNTL_AUX_DIS_CNTL	0x4000
#define		HREG_CNTL_AUX_DSD		0x2000
#define		HREG_CNTL_AUX_ENA		(HREG_CNTL_AUX_ENA_CNTL | HREG_CNTL_AUX_DIS_CNTL )
#define HREG_SPARE				0x16		//
#define HREG_AUX_PAGE			0x3A		//
#define HREG_AUX_OFFSET			0x3C		//
#define HREG_AUX_DATA			0x3E		//


/************************************************************************************************/
/***************************** END OF STRUCTURES ***********************************************/
/************************************************************************************************/


/************************************************************************************************/
/**********************************  EQUATES  ***************************************************/
/************************************************************************************************/

// Tx/Rx frame Structure
//
#define HFS_STAT_ABS		(0x2E + HFS_STAT)    		//0x0000
#define HFS_Q_INFO_ABS		(0x2E + HFS_Q_INFO)			//0x0006
#define HFS_TX_CNTL_ABS		(0x2E + HFS_TX_CNTL)		//0x000C
#define HFS_FRAME_CNTL_ABS	(0x2E + HFS_FRAME_CNTL)		//0X000E
#define HFS_ID_ABS			(0x2E + HFS_ID)				//0X0010

#define HFS_ADDR_1_ABS		(0x12 + HFS_ADDR_1)  		//0x0012
#define HFS_ADDR_2_ABS		(0x12 + HFS_ADDR_2)  		//0x0018
#define HFS_ADDR_3_ABS		(0x12 + HFS_ADDR_3)  		//0x001E
#define HFS_SEQ_CNTL_ABS	(0x12 + HFS_SEQ_CNTL)		//0x0024
#define HFS_ADDR_4_ABS		(0x12 + HFS_ADDR_4) 		//0x0026
#define HFS_DAT_LEN_ABS		(0x12 + HFS_DAT_LEN)		//0x002C

#define HFS_ADDR_DEST_ABS   (0x2E + HFS_ADDR_DEST)		//0x002E
#define HFS_ADDR_SRC_ABS    (0x2E + HFS_ADDR_SRC)		//0x0034
#define HFS_LEN_ABS	       	(0x2E + HFS_LEN)			//0x003A
#define HFS_DAT_ABS	       	(0x2E + HFS_DAT)			//0x003C
#define HFS_TYPE_ABS	    (0x2E + HFS_TYPE)			//0x0042	Eternet-II type in 1042/Bridge-Tunnel encapsulated frame
#define HFS_WDS_TYPE_ABS	(0x2E + HFS_WDM_TYPE)		//0x0045	PDU type in WDM frame

#define HFS_802_11_GAP		(HFS_DAT_ABS  - HFS_ADDR_DEST_ABS)
#define HFS_E_II_GAP       	(HFS_TYPE_ABS - HFS_LEN_ABS)

#if 0																					//<HCFL>
#define KLUDGE_MARGIN		8							//safety margin for Tx Data Corruption WorkAround (DCWA)
#else																					//<HCFL@
#define KLUDGE_MARGIN		8 + sizeof(snap_header) 	//DCWA + additional space for E-II encapsulation
#endif // 0																				//@HCFL>

#define HFS_TX_ALLOC_SIZE	HCF_MAX_MSG + HFS_DAT_ABS + KLUDGE_MARGIN

// IFB field related
//		IFB_TxFrameType
//#define ENC_TX_802_3           	0x00
//#define ENC_TX_802_11         	0x11
#define ENC_TX_E_II				0x0E	//encapsulation flag

// SNAP header for E-II Encapsulation
#define ENC_TX_1042             0x00
#define ENC_TX_TUNNEL           0xF8

// Hermes Command Codes and Qualifier bits
#define 	HCMD_BUSY			0x8000	// Busy bit, applicable for all commands
#define 	HCMD_RECL			0x0100	// Reclaim bit, applicable for Tx and Inquire

#define HCMD_INI				0x0000	//
#define 	HCMD_INI_0x0100		0x0100	//
#define HCMD_ENABLE				0x0001	//
#define HCMD_DISABLE			0x0002	//
#define HCMD_DIAG				0x0003	//
#define HCMD_ALLOC				0x000A	//
#define HCMD_TX					0x000B	//
#define HCMD_NOTIFY				0x0010	//
#define HCMD_INQUIRE			0x0011	//
#define HCMD_ACCESS				0x0021	//
#define 	HCMD_ACCESS_WRITE		0x0100	//
#define HCMD_PROGRAM			0x0022	//
#define 	HCMD_PROGRAM_DISABLE				0x0000	//
#define 	HCMD_PROGRAM_ENABLE_VOLATILE	 	0x0100	//
#define 	HCMD_PROGRAM_ENABLE_NON_VOLATILE	0x0200	//
#define 	HCMD_PROGRAM_NON_VOLATILE			0x0300	//


//Configuration Management
//
#define CFG_DRV_ACT_RANGE_PRI_1_BOTTOM	1	// Default Bottom Compatibility for Primary Firmware - driver I/F
#define CFG_DRV_ACT_RANGE_PRI_1_TOP		2	// Default Top    Compatibility for Primary Firmware - driver I/F
										   	// Hermes needs 2 as top, if top == 1, Shark only

#define CFG_DRV_ACT_RANGE_PRI_2_BOTTOM	1	// Default Bottom Compatibility for Primary Firmware - driver I/F
#define CFG_DRV_ACT_RANGE_PRI_2_TOP		1	// Default Top    Compatibility for Primary Firmware - driver I/F

#define CFG_DRV_ACT_RANGE_HSI_0_BOTTOM	1	// Default Bottom Compatibility for H/W - driver I/F
#define CFG_DRV_ACT_RANGE_HSI_0_TOP		1	// Default Top    Compatibility for H/W - driver I/F

#define CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	1	// Default Bottom Compatibility for H/W - driver I/F
#define CFG_DRV_ACT_RANGE_HSI_1_TOP		2	// Default Top    Compatibility for H/W - driver I/F

#define CFG_DRV_ACT_RANGE_HSI_2_BOTTOM	1	// Default Bottom Compatibility for H/W - driver I/F
#define CFG_DRV_ACT_RANGE_HSI_2_TOP		1	// Default Top    Compatibility for H/W - driver I/F

#define CFG_DRV_ACT_RANGE_STA_1_BOTTOM	6	// Default Bottom Compatibility for Station Firmware - driver I/F
#define CFG_DRV_ACT_RANGE_STA_1_TOP		8	// Default Top    Compatibility for Station Firmware - driver I/F

//---------------------------------------------------------------------------------------------------------------------
#if defined HCF_CFG_PRI_1_TOP									// Top Compatibility for Primary Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_PRI_1_BOTTOM <= HCF_CFG_PRI_1_TOP && HCF_CFG_PRI_1_TOP <= CFG_DRV_ACT_RANGE_PRI_1_TOP
#undef CFG_DRV_ACT_RANGE_PRI_1_TOP
#define CFG_DRV_ACT_RANGE_PRI_1_TOP	HCF_CFG_PRI_1_TOP
#else
error;
#endif
#endif // HCF_CFG_PRI_1_TOP

#if defined HCF_CFG_PRI_1_BOTTOM                       			// Bottom Compatibility for Primary Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_PRI_1_BOTTOM <= HCF_CFG_PRI_1_BOTTOM && HCF_CFG_PRI_1_BOTTOM <= CFG_DRV_ACT_RANGE_PRI_1_TOP
#undef CFG_DRV_ACT_RANGE_PRI_1_BOTTOM	
#define CFG_DRV_ACT_RANGE_PRI_1_BOTTOM	HCF_CFG_PRI_1_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_PRI_1_BOTTOM


#if defined HCF_CFG_PRI_2_TOP									// Top Compatibility for Primary Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_PRI_2_BOTTOM <= HCF_CFG_PRI_2_TOP && HCF_CFG_PRI_2_TOP <= CFG_DRV_ACT_RANGE_PRI_2_TOP
#undef CFG_DRV_ACT_RANGE_PRI_2_TOP
#define CFG_DRV_ACT_RANGE_PRI_2_TOP	HCF_CFG_PRI_2_TOP
#else
error;
#endif
#endif // HCF_CFG_PRI_2_TOP

#if defined HCF_CFG_PRI_2_BOTTOM                       			// Bottom Compatibility for Primary Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_PRI_2_BOTTOM <= HCF_CFG_PRI_2_BOTTOM && HCF_CFG_PRI_2_BOTTOM <= CFG_DRV_ACT_RANGE_PRI_2_TOP
#undef CFG_DRV_ACT_RANGE_PRI_2_BOTTOM	
#define CFG_DRV_ACT_RANGE_PRI_2_BOTTOM	HCF_CFG_PRI_2_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_PRI_2_BOTTOM


//---------------------------------------------------------------------------------------------------------------------
#if defined HCF_CFG_HSI_0_TOP 									// Top Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_0_BOTTOM <= CF_CFG_HSI_0_TOP && HCF_CFG_HSI_0_TOP <= CFG_DRV_ACT_RANGE_HSI_0_TOP
#undef CFG_DRV_ACT_RANGE_HSI_0_TOP	
#define CFG_DRV_ACT_RANGE_HSI_0_TOP	HCF_CFG_HSI_0_TOP
#else
error;
#endif
#endif // HCF_CFG_HSI_0_TOP

#if defined HCF_CFG_HSI_1_BOTTOM								// Bottom Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_1_BOTTOM <= HCF_CFG_HSI_1_BOTTOM && HCF_CFG_HSI_1_BOTTOM <= CFG_DRV_ACT_RANGE_HSI_1_TOP
#undef CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	
#define CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	HCF_CFG_HSI_1_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_HSI_1_BOTTOM


#if defined HCF_CFG_HSI_2_TOP 									// Top Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_2_BOTTOM <= CF_CFG_HSI_2_TOP && HCF_CFG_HSI_2_TOP <= CFG_DRV_ACT_RANGE_HSI_2_TOP
#undef CFG_DRV_ACT_RANGE_HSI_2_TOP	
#define CFG_DRV_ACT_RANGE_HSI_2_TOP	HCF_CFG_HSI_2_TOP
#else
error;
#endif
#endif // HCF_CFG_HSI_2_TOP

#if defined HCF_CFG_HSI_1_BOTTOM								// Bottom Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_1_BOTTOM <= HCF_CFG_HSI_1_BOTTOM && HCF_CFG_HSI_1_BOTTOM <= CFG_DRV_ACT_RANGE_HSI_1_TOP
#undef CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	
#define CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	HCF_CFG_HSI_1_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_HSI_1_BOTTOM


#if defined HCF_CFG_HSI_1_TOP 									// Top Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_1_BOTTOM <= CF_CFG_HSI_1_TOP && HCF_CFG_HSI_1_TOP <= CFG_DRV_ACT_RANGE_HSI_1_TOP
#undef CFG_DRV_ACT_RANGE_HSI_1_TOP	
#define CFG_DRV_ACT_RANGE_HSI_1_TOP	HCF_CFG_HSI_1_TOP
#else
error;
#endif
#endif // HCF_CFG_HSI_1_TOP

#if defined HCF_CFG_HSI_1_BOTTOM								// Bottom Compatibility for HSI I/F
#if CFG_DRV_ACT_RANGE_HSI_1_BOTTOM <= HCF_CFG_HSI_1_BOTTOM && HCF_CFG_HSI_1_BOTTOM <= CFG_DRV_ACT_RANGE_HSI_1_TOP
#undef CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	
#define CFG_DRV_ACT_RANGE_HSI_1_BOTTOM	HCF_CFG_HSI_1_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_HSI_1_BOTTOM


//---------------------------------------------------------------------------------------------------------------------
#if defined HCF_CFG_STA_1_TOP                  					// Top Compatibility for Station Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_STA_1_BOTTOM <= HCF_CFG_STA_1_TOP && HCF_CFG_STA_1_TOP <= CFG_DRV_ACT_RANGE_STA_1_TOP
#undef CFG_DRV_ACT_RANGE_STA_1_TOP	
#define CFG_DRV_ACT_RANGE_STA_1_TOP	HCF_CFG_STA_1_TOP
#else
error;
#endif
#endif // HCF_CFG_STA_TOP

#if defined HCF_CFG_STA_1_BOTTOM                       			// Bottom Compatibility for Station Firmware - driver I/F
#if CFG_DRV_ACT_RANGE_STA_1_BOTTOM <= HCF_CFG_STA_1_BOTTOM && HCF_CFG_STA_1_BOTTOM <= CFG_DRV_ACT_RANGE_STA_1_TOP
#undef CFG_DRV_ACT_RANGE_STA_1_BOTTOM	
#define CFG_DRV_ACT_RANGE_STA_1_BOTTOM	HCF_CFG_STA_1_BOTTOM
#else
error;
#endif
#endif // HCF_CFG_STA_BOTTOM



// Miscellanuos
//
#define REV_OFFSET 13					// offset of Major version within the PVCS generated Version String
#define	ENG_FLG_PDU_FILT	0x0001		// DISable filtering WMP messages on PDU typelg


/************************************************************************************************/
/**********************************  END OF EQUATES  ********************************************/
/************************************************************************************************/


/************************************************************************************************/
/**************************************  MACROS  ************************************************/
/************************************************************************************************/

/************************************************************************************************
	DEBUG_INT is an undocumented feature to assist the HCF debugger
	By expanding INT_3 to either an "int 3" or a NOP, it is very easy to check
	by means of a binary file compare whether a "debug" version really corresponds
	with a "non-debug" version.
	The "int 3" is only applicable to 80x86 environment. For ease of implementation, the
	DEBUG_INT feauture is only supported in the MSVC environment
*/
#if defined (_MSC_VER)
#ifdef DEBUG_INT
#pragma message( "HCFDEF.H WARNING: int 3, should be removed before releasing" )
#define INT_3 __asm int 3
#else	/*DEBUG_INT*/
#define INT_3 __asm nop
#endif	/*DEBUG_INT*/
#else	/*_MSC_VER*/
#define INT_3
#endif	/*_MSC_VER*/

#define MUL_BY_2( x )	( (x) << 1 )	//used to multiply by 2
#define DIV_BY_2( x )	( (x) >> 1 )	//used to divide by 2


#if ! defined HCF_ASSERT
#define HCFASSERT( x, q )
#define HCFASSERTLOGENTRY(q)
#define HCFASSERTLOGEXIT(q)
#else
#define HCFASSERT(x,q) if (!(x)) {hcf_assert( ifbp, (struct ASSERT_STRCT*)&assert_strct, __LINE__, q );}
#define HCFASSERTLOGENTRY(q) {ifbp->IFB_AssertTrace |= q;}
#define HCFASSERTLOGEXIT(q)  {ifbp->IFB_AssertTrace &= ~q;}
#endif

#if ! defined HCF_LOG
#define HCFLOG( ifbp, x, v1, v2 )
#else
#define HCFLOG( ifbp, x,v1, v2) {log_rtn( ifbp, x, v1, v2 );}
#endif

#if defined HCF_PROFILING || defined HCF_DEBUG
#define HCF_DEBUG_TRIGGER(ifbp, where, what )	{hcf_debug_trigger( ifbp, where, what );}
#else
#define HCF_DEBUG_TRIGGER(ifbp, where, what )
#endif // defined HCF_PROFILING || defined HCF_DEBUG



/************************************************************************************************/
/**************************************  END OF MACROS  *****************************************/
/************************************************************************************************/

/************************************************************************************************/
/***************************************  PROTOTYPES  *******************************************/
/************************************************************************************************/

int 	hcfio_string( IFBP ifbp, int bap, hcf_16 fid, hcf_16 offset, wci_bufp pc_addr, int wlen, int blen, int type );
void	log_rtn( IFBP ifbp, int rtn_id, int val_1, int val_2);         					//<HCFL>

#endif	//HCFDEFC_H
