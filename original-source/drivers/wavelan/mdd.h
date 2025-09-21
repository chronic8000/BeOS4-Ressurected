#ifndef MDD_
#define MDD_H 1

/*************************************************************************************************************
*
* FILE	 : mdd.h
*
* DATE   : $Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR : Nico Valster
*
* DESC   : Definitions and Prototypes for HCF, MSF, UIL as well as USF sources
*
*
*
* Implementation Notes
*
 -	Typ rather than type is used as field names in structures like CFG_CIS_STRCT because type leads to
 	conflicts with MASM when the H-file is converted to an INC-file
*
**************************************************************************************************************
Instructions to convert MDD.H to MDD.INC by means of H2INC

Use a command line which defines the specific macros and command line options
needed to build the C-part, e.g. for the DOS ODI driver
		`h2inc /C /Ni /Zp /Zn mdd	 mdd.h`


**************************************************************************************************************
* COPYRIGHT (c) 1998, 1999, 2000 by Lucent Technologies.	 All Rights Reserved.
*************************************************************************************************************/

/*************************************************************************************************************
*
* CHANGE HISTORY
*
  961018 - NV
	Original Entry, split of from HCF.H
$Log: mdd.h,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.34   11 Feb 2000 14:19:18   NVALST
 * sync with home
 * 
 *    Rev 1.33   11 Feb 2000 10:17:46   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.32   08 Feb 2000 09:28:22   NVALST
 * passed to reinier
 * 
 *    Rev 1.31   04 Feb 2000 14:44:32   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.30   04 Feb 2000 14:10:06   NVALST
 * intermediate, backup before inserting CDOC comments
 * 
 *    Rev 4.6   01 Feb 2000 12:02:36   NVALST
 * 
 *    Rev 4.6   28 Jan 2000 15:39:54   NVALST
 * 
 *    Rev 1.29   28 Jan 2000 13:27:46   NVALST
 * 
 *    Rev 4.5   30 Nov 1999 15:12:44   NVALST
 * 
 *    Rev 1.27   27 Jan 2000 10:53:34   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 17:23:58   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 15:27:56   NVALST
 * 
 *    Rev 1.8   03 Nov 1999 16:43:02   NVALST
 * 
 *    Rev 4.2   10 Sep 1999 11:28:44   NVALST
 * 
 *    Rev 1.187   10 Sep 1999 11:25:12   NVALST
 * 
 *    Rev 4.2   09 Sep 1999 13:34:28   NVALST
 * 
 *    Rev 1.186   07 Sep 1999 13:23:36   NVALST
 * 
 *    Rev 4.1   31 Aug 1999 12:14:42   NVALST
 * 
 *    Rev 1.181   30 Aug 1999 11:58:40   NVALST
 * 
 *    Rev 4.0   07 Jul 1999 10:09:24   NVALST
 * 
 *    Rev 2.22   29 Jun 1999 11:32:38   NVALST
 * 
*************************************************************************************************************/


/**************************************************************************************************************

*
* ToDo
*
 1:	justify the "DON'T EVER MOVE" clauses, e.g. on HCF_ACT_DIAG
 2:	re-think the issues around MSF_ASSERT_RTN proto type. Once the thought crosssed my mind that the problems
 	could be resolved completely within COM-modules 

*
* Implementation Notes
*
 -	
	
*
* Miscellaneous Notes
*
 -	


*************************************************************************************************************/
/******************************      M A C R O S     ********************************************************/

/* min and max macros */
#if !defined(max)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif


/*************************************************************************************************************/

/****************************** General define ***************************************************************/

#define MAC_ADDR_SIZE			6
#define GROUP_ADDR_SIZE			(32 * MAC_ADDR_SIZE)
#define STAT_NAME_SIZE			32



//IFB field related
//		IFB_CardStat
#define CARD_STAT_PRESENT				0x8000U	/* Debug Only !!
												 * MSF defines card as being present
												 * controls whether hcf-function is allowed to do I/O		*/
#define CARD_STAT_ENABLED				0x4000U	/* Debug Only !!
												 * one or more MAC Ports enabled							*/
#define CARD_STAT_INCOMP_PRI			0x2000U	// hcf_disable did not detect compatible primary functions
#define CARD_STAT_INCOMP_STA			0x1000U	// hcf_disable did not detect compatible station functions
#define CARD_STAT_INCOMP_FEATURE		0x0800U	/* Incompatible feature (prevents enable)
												 *  - HCF_CRYPT_RESTRAINT_BIT
												 * !! NOTE !! this bit is only cleared as side effect of 
												   !! HCF_ACT_CARD_IN. Dynamically clearing it if a non-
												   !! violating feature is set, will not work properly if
												   !! - some time in the future -, a 2nd feature with this
												   !! characteristics is created. Since Utilities protect
												   !! "intelligent" against this problem and Driver Config
												   !! files can not change dynamically, this is not a serious
												   !! limitation
                                                 */
#define CARD_STAT_AP					0x0200U	// Hermaphrodite in AP mode
#define CARD_STAT_DEFUNCT				0x0100U	// HCF is in Defunct mode
//		IFB_RxStat
#define RX_STAT_ERR						0x0003U	//Error mask
#define 	RX_STAT_UNDECR				0x0002U	//Non-decryptable encrypted message
#define 	RX_STAT_FCS_ERR				0x0001U	//FCS error

/****************************** Xxxxxxxx *********************************************************************/

enum /*hcf_stat*/ {
//	HCF_FAILURE				= 0xFF,	/* An (unspecified) failure, 0xFF is choosen to have a non-ubiquitous value
//	                                 *	Note that HCF_xxxx errors which can end up in the CFG_DIAG LTV should
//	                                 *	never exceed 0xFF, because the high order byte of VAL[0] is reserved
//	                                 *	for Hermes errors
//	                                 */
	HCF_SUCCESS				= 0x00,	// 0x00: OK
	//								// gap for ODI related status and obsolete 
	//								// obsolete HCF_ERR_DIAG_0
	HCF_ERR_DIAG_1			= 0x03,	// 0x03: HCF noticed an error after succesful diagnose command
	HCF_ERR_TIME_OUT,               // 0x04: Expected Hermes event did not occure in expected time
	HCF_ERR_NO_NIC,					/* 0x05: card not found (usually yanked away during hcfio_in_string
											 Also: card is either absent or disabled while it should be neither		*/
	HCF_ERR_BUSY,					// 0x06: hcf_send_diag_msg called while PIF_RscInd == 0
	HCF_ERR_SEQ_BUG,				// 0x07: other cmd than the expected completed, probably HCF-bug
	HCF_ERR_LEN,					// 0x08: buffer size insufficient
									//		  -	IFB_ConfigTable too small										<HCFL>
									//		  -	hcf_get_info buffer has a size of 0 or 1 or less than needed
									//			to accomodate all data
									//		  -	hcf_put_info: CFG_DLNV_DATA exceeds intermediate buffer size	<HCFL>
	HCF_ERR_INCOMP_PRI,				// 0x09: primary functions are not compatible
	HCF_ERR_INCOMP_STA,				// 0x0A: primary functions are compatible, station functions are not
	HCF_ERR_INCOMP_FEATURE,			/* 0x0B: Incompatible feature (prevents enable)
									 *  - HCF_CRYPT_RESTRAINT_BIT
                                     */
	HCF_ERR_DCWA,					// DCWA (Data Corruption Work Around) catched (primarily in hcf_put_data process)
//	                                /* begin of range to skip for backward compatibilty problem circumvention		*/
//	HCF_UIL_BCK_COMP_1		= 0x0B,	// 0x0B was: UIL_ERR_NOT_CONNECTED
//	HCF_UIL_BCK_COMP_2		= 0X0C,	// 0x0C was: UIL_ERR_PIF_CONFLICT
//	HCF_UIL_BCK_COMP_3		= 0X0D,	// 0x0D was: UIL_ERR_NO_DRV
//	                                /* end of range to skip for backward compatibilty problem circumvention			*/
	HCF_ERR_MAX,					// xxxx: start value for UIL-range
	HCF_ERR_DEFUNCT			= 0x80,	// BIT, reflecting that the HCF is in defunct mode (bits 0x7F reflect cause)
	HCF_ERR_DEFUNCT_INI,			// Timeout on Busy bit drop before writing Hermes Initialize command
	HCF_ERR_DEFUNCT_AUX,			// Timeout on acknowledgement on en/disabling AUX registers
	HCF_ERR_DEFUNCT_TIMER,			// Timeout on timer calibration during initialization process
	HCF_ERR_DEFUNCT_TIME_OUT,		// Timeout on Busy bit drop during BAP setup
	HCF_ERR_DEFUNCT_ALLOC,			// Allocation failed during initialization process
	HCF_ERR_DEFUNCT_CMD_SEQ,		// Hermes and HCF are out of sync in issuing/processing commands
	HCF_ERR_DEFUNCT_DL,				// DCWA did not succeed within retry limit
	HCF_ERR_DEFUNCT_FREEZE,			// freeze the F/W execution (intentionally via HCF_ACT_FREEZE)
	HCF_ERR_CMD				= 0x0100,	//"insignificant" error (see W2DN264)
	HCF_ERR_INQUIRE			= 0x0511,	// Inquiry failed due to lack of Hermes Buffer space
	
//enum /*uil_stat*/ {
//	UIL_FAILURE				= HCF_FAILURE,
	UIL_SUCCESS				= HCF_SUCCESS,
	UIL_ERR_DIAG_1			= HCF_ERR_DIAG_1,
	UIL_ERR_TIME_OUT		= HCF_ERR_TIME_OUT,
	UIL_ERR_NO_NIC			= HCF_ERR_NO_NIC,
	UIL_ERR_BUSY			= HCF_ERR_BUSY,
	UIL_ERR_SEQ_BUG			= HCF_ERR_SEQ_BUG,
	UIL_ERR_LEN				= HCF_ERR_LEN,
	UIL_ERR_PIF_CONFLICT	= 0x40,				//this is the lower boundary of UIL errors without HCF-pendant
	UIL_ERR_INCOMP_DRV,
	UIL_ERR_NO_DRV,
	UIL_ERR_DOS_CALL,
	UIL_ERR_IN_USE,
	UIL_ERR_NSTL,
	UIL_ERR_WRONG_IFB,
	                                /* begin of range to skip for backward compatibilty problem circumvention*/
	HCF_UIL_BCK_COMP_4		= 103,	// 0x67 was: UIL_ERR_INCOMP_DRV
	HCF_UIL_BCK_COMP_5		= 105	// 0x69 was: UIL_ERR_DOS_CALL
	                                /* end of range to skip for backward compatibilty problem circumvention	*/
//} /* uil_stat */
};

#define	HCF_INT_PENDING			1	// (ODI initiated) return status of hcf_act( HCF_ACT_INT_OFF )



/* hard coded values (e.g. for HCF_ACT_TALLIES and HCF_ACT_INT_OFF) are needed for HCFL							*/
typedef enum  { /*hcf_action_cmd*/
	HCF_ACT_802_3_PURE,					//00 activate 802.3 frame mode without encapsulation			//<HCFL>
	HCF_ACT_802_3,						//01 activate 802.3 frame mode with encapsulation           	//<HCFL>
	HCF_ACT_802_11,						//02 ativate 802.11 frame mode	                				//<HCFL>
										/*	gap left over by swapping 3 frame mode action with 4 INT_OFF/_ON
										 *	CARD_IN/_OUT. This was done to have HCFL default automagically
										 *	to HCF_ACT_802_3_PURE
										 *	This gap available for future features								*/
	HCF_ACT_SPARE_03,					//03 gap available for future features
										/* DUI code 0x04 -> DON'T EVER MOVE 									*/
	HCF_ACT_DIAG = 0x04,				//04 Hermes Diagnose command
										/* DUI code 0x05 -> DON'T EVER MOVE 									*/
	HCF_ACT_TALLIES = 0x05,				//05 Hermes Inquire Tallies (F100) command
	HCF_ACT_SCAN,						//06 Hermes Inquire Scan (F101) command
	HCF_ACT_TICK_OFF,					//07 disable TimerTick interrupt  	           					//<HCFL>
	HCF_ACT_TICK_ON,					//08 enable TimerTick interrupt		            				//<HCFL>
	HCF_ACT_SPARE_09,					//09 gap original occupied by frame modes,        				//<HCFL>
	HCF_ACT_SPARE_0A,					//0A available for future features                				//<HCFL>
										/* DUI code 0x0B -> DON'T EVER MOVE 									*/
	HCF_ACT_BLOCK = 0x0B,				//0B DUI code, just a place holder should never be passed to HCF//<HCFL>
										/* DUI code 0x0C -> DON'T EVER MOVE 									*/
	HCF_ACT_UNBLOCK = 0x0C,				//0C DUI code, just a place holder should never be passed to HCF//<HCFL>
	HCF_ACT_INT_OFF = 0x0D,				//0D Disable Interrupt generation
	HCF_ACT_INT_ON,						//0E Enable Interrupt generation
	HCF_ACT_INT_FORCE_ON,				//0F Enforce Enable Interrupt generation
	HCF_ACT_CARD_IN,					//10 MSF reported Card insertion
	HCF_ACT_CARD_OUT,  					//11 MSF reported Card removal
	HCF_ACT_FREEZE  					//12 freeze the F/W execution
/*	HCF_ACT_MAX							// xxxx: start value for UIL-range, NOT to be passed to HCF
 *										Too bad, there was originally no spare room created to use
 *										HCF_ACT_MAX as an equivalent of HCF_ERR_MAX. Since creating
 *										this room in retrospect would create a backward incompatibilty
 *										we will just have to live with the haphazard sequence of
 *										UIL- and HCF specific codes. Theoretically this could be
 *										corrected when and if there will ever be an overall
 *										incompatibilty introduced for another reason
 */										
} hcf_action_cmd;

#if 1		                                                                           	//<HCFL@
typedef enum {	/*hcf_send_type */
	HCF_PORT_0,	// MAC Ports Subsection
	HCF_PORT_1,
	HCF_PORT_2,
	HCF_PORT_3,
	HCF_PORT_4,
	HCF_PORT_5,
	HCF_PORT_6,
	HCF_PUT_DATA_RESET	= 0x0007	//cleanup hcf_put_data/_send housekeeping without actual send
				// All other subsections disappeared in time due to requirement changes
} hcf_send_type;
#else                                                                        			//@HCFL>
#define	HCF_PORT_0 0						// HCF-light supports only a single MAC Port

#endif                                                                       			//<HCFL>


//-------------------------------------------------------------------------------------- <HCFL@
// UIL management function to be passed to WaveLAN/IEEE Drivers in DUI_STRCT field fun
//--------------------------------------------------------------------------------------------------------------
typedef enum {
	UIL_FUN_CONNECT,						// Perform connect command
	UIL_FUN_DISCONNECT,						// Perform disconnect command
	UIL_FUN_ACTION,							// Perform UIL Action command.
	UIL_FUN_SEND_DIAG_MSG,					// Send a diagnostic message.
	UIL_FUN_GET_INFO,						// Retrieve information from NIC.
	UIL_FUN_PUT_INFO						// Put information on NIC.
} uil_fun;                                                                              //@HCFL>


typedef enum  { /*uil_action_cmd*/                                                      //<HCFL@
	UIL_ACT_DIAG	= HCF_ACT_DIAG,
	UIL_ACT_TALLIES	= HCF_ACT_TALLIES,
	UIL_ACT_SCAN	= HCF_ACT_SCAN,
	UIL_ACT_BLOCK	= HCF_ACT_BLOCK,
	UIL_ACT_UNBLOCK	= HCF_ACT_UNBLOCK,
	UIL_ACT_APPLY	= 0x80
} uil_action_cmd;                                                                       //@HCFL>



#define	RESTRAINT_SET   				0
#define	RESTRAINT_RESET					1

#define HCF_RESTRAINT_PM				0x0001
#define HCF_RESTRAINT_CRYPT_FW			0x0002
#define HCF_RESTRAINT_CRYPT_HW			0x0004

#define MSF_RESTRAINT_CRYPT_PARM		0x0001
#define MSF_RESTRAINT_DRV_CFG			0x0002
#define MSF_RESTRAINT_NIC_CHANGE		0x0004
#define MSF_RESTRAINT_APFW_INCOMP		0x0008
#define MSF_RESTRAINT_CHANNELS_INCOMP	0x0010	/* driver can not work with the channels supported by the card.
												 * Channels 1-11 must be at least available.
												 * occures for old Japanese cards and France cards					*/

/*============================================================= HCF_USB Defined USB_RECORDS	=========================*/
//<HCFL@
#define CFG_TX_FRAME					0x0700 		//Tx frame
#define CFG_RX_FRAME					0x0701 		//Rx frame
#define CFG_ACTION						0x0702 		//Action frame


//@HCFL>

/*============================================================= HCF Defined RECORDS	=========================*/
//<HCFL@
#define CFG_PROD_DATA					0x0800 		//Plug Data
#define CFG_AUX_DATA					0x0801		//Aux Data
#define CFG_AUX_DATA_SUPER_SPECIAL		0x0802		//Aux Data intended for froozen F/W
	
	
#define CFG_NULL						0x0820		//Empty Mail Box Info Block
#define CFG_MB_INFO						0x0820		//Mail Box Info Block
#define CFG_DIAG						0x0821		//Diagnose result
#define CFG_WMP							0x0822		//WaveLAN Management Protocol
#define CFG_RESTRAINT_INFO				0x0823		//accumulated set of restraint bits detected by driver (HCF + MSF)
//@HCFL>
#define CFG_IFB							0x0824		//byte wise copy of IFB
#define CFG_DRV_INFO					0x0825		//Driver Information structure (see CFG_DRV_INFO_STRCT for details)
#define CFG_DRV_IDENTITY				0x0826		//driver identity (see CFG_DRV_IDENTITY_STRCT for details)
//<HCFL@
#define CFG_DRV_SUP_RANGE				0x0827      //Supplier range of driver - utility I/F
#define CFG_DRV_ACT_RANGE_PRI			0x0828      //(Acceptable) Actor range for Primary Firmware - driver I/F
#define CFG_DRV_ACT_RANGE_STA			0x0829      //(Acceptable) Actor range for Station Firmware - driver I/F
#define CFG_DRV_ACT_RANGE_HSI  			0x082A      //(Acceptable) Actor range for H/W - driver I/F
#define CFG_DRV_LOG						0x082B		//driver HCF activity log
#define CFG_TX_ERR_NOTIFY				0x082C		//erronuous transmissions log

#define CFG_UNK							0x082F		//Unknown, used a.o. in "flows which should not occure"
//@HCFL>
#define CFG_REG_MB						0x0830		//Register Mail Box
//<HCFL@
#define CFG_MB_ASSERT					0x0831		//MSF originated Assert information
#define CFG_REG_ASSERT_RTNP				0x0832		//(de-)register MSF Assert Callback routine
#define CFG_TALLIES_VACATE				0x0833		//Read and Clear Communications Tallies
#define CFG_CNTL_RESTRAINT				0x0834		//set/reset bits in MSF-controlled field of CFG_RESTRAINT_INFO
#define CFG_CNTL_MSF_TYPE				0x0835		//control emergency escape IFB_MSFType 
#define CFG_CNTL_ENG_FLG				0x0836		//control Engineering Flag settings
#define CFG_CNTL_TX_FLG					0x0837		//control TxControl field of Tx frames
#define CFG_REG_LOG_RTNP				0x0840		//(de-)register MSF Log Callback routine
#define CFG_DLNV_START					0x0850		//Setup for Download to non-volatile RAM
#define CFG_DLNV_DATA					0x0851		//Download data
#define CFG_DLV_START					0x0852		//Setup for Download to volatile RAM
#define CFG_DLV_ADDR					0x0853		//Download RAM Addressing information
#define CFG_DLV_DATA					0x0854		//Download RAM Data
#define CFG_DL_STOP						0x0855		//Cleanup Download, non-volatile as well as volatile
#define CFG_DL_STAT						0x0856		//Download result
	
//@HCFL>

/*============================================================= UTILITY - MSF FRAMES	=====================*/

#define	CFG_ENCRYPT_STRING				0x0900
#define	CFG_AP_MODE						0x0901


/*============================================================= CELL WAVE ===================================*/
//		CellWave Range					0x10xx

/*============================================================= INFORMATION FRAMES		=====================*/
#define CFG_INFO_FRAME_MIN				0xF000		//lowest value representing an Informatio Frame
#define CFG_NOTIFY						0xF000		//Handover Address							<HCFL>
	
#define CFG_TALLIES						0xF100		//Communications Tallies
#define CFG_SCAN						0xF101		//Scan results
	                        	
#define CFG_LINK_STAT 					0xF200		//Link Status
#define CFG_ASSOC_STAT					0xF201		//Association Status						<HCFL>
#define CFG_SECURITY_STAT				0xF202		//Security Status							<HCFL>
	
/*============================================================= CONFIGURATION RECORDS	=====================*/
/*============================================================= mask 0xFCxx				=====================*/						
//	NETWORK PARAMETERS, STATIC CONFIGURATION ENTITIES
//FC05, FC0A, FC0B, FC0C, FC0D: SEE W2DN149
	
#define CFG_RID_CFG_MIN					0xFC00		//lowest value representing a Configuration RID
#define CFG_CNF_PORT_TYPE				0xFC00		//[STA] Connection control characteristics
#define CFG_CNF_OWN_MAC_ADDR			0xFC01		//[STA] MAC Address of this node
#define CFG_CNF_DESIRED_SSID			0xFC02		/*[STA] Service Set identification for connection
													 *originally known as static entity with name 
													 *	CFG_CNF_DESIRED_SSID
													 *when type changed to dynamic, name incorrectly changed to 
													 *	CNF_DESIRED_SSID
													 *finally name changed to (the "correct")
													 *	CFG_DESIRED_SSID
													 *CNF_DESIRED_SSID will be discarded some time in the future
													 */
#define CFG_CNF_OWN_CHANNEL				0xFC03		//Communication channel for BSS creation
#define CFG_CNF_OWN_SSID				0xFC04		//IBSS creation (STA) or ESS (AP) Service Set Ident
#define CFG_CNF_OWN_ATIM_WINDOW			0xFC05		//[STA] ATIM Window time for IBSS creation
#define CFG_CNF_SYSTEM_SCALE			0xFC06		//System Scale that specifies the AP density
#define CFG_CNF_MAX_DATA_LEN			0xFC07		//Maximum length of MAC Frame Body data
#define CFG_CNF_WDS_ADDR				0xFC08		//[STA] MAC Address of corresponding WDS Link node
#define CFG_CNF_PM_ENABLED				0xFC09		//[STA] Switch for ESS Power Management (PM) On/Off
#define CFG_CNF_PM_EPS					0xFC0A		//[STA] Switch for ESS PM EPS/PS Mode
#define CFG_CNF_MCAST_RX				0xFC0B		//[STA] Switch for ESS PM Multicast reception On/Off
#define CFG_CNF_MAX_SLEEP_DURATION		0xFC0C		//[STA] Maximum sleep time for ESS PM
#define CFG_CNF_HOLDOVER_DURATION		0xFC0D		//[STA] Holdover time for ESS PM
#define CFG_CNF_OWN_NAME				0xFC0E		//Identification text for diagnostic purposes
	
#define CFG_CNF_OWN_DTIM_PERIOD			0xFC10		//[AP] Beacon intervals between successive DTIMs		<HCFL@
#define CFG_CNF_WDS_ADDR1				0xFC11		//[AP] Port 1 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_WDS_ADDR2				0xFC12		//[AP] Port 2 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_WDS_ADDR3				0xFC13		//[AP] Port 3 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_WDS_ADDR4				0xFC14		//[AP] Port 4 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_WDS_ADDR5				0xFC15		//[AP] Port 5 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_WDS_ADDR6				0xFC16		//[AP] Port 6 MAC Adrs of corresponding WDS Link node
#define CFG_CNF_MCAST_PM_BUF			0xFC17		//[AP] Switch for PM buffereing of Multicast Messages
#define CFG_CNF_REJECT_ANY				0xFC18		//[AP] Switch for PM buffereing of Multicast Messages	@HCFL>
#define CFG_CNF_ENCRYPTION				0xFC20		//select en/de-cryption of Tx/Rx messages
#define CFG_CNF_AUTHENTICATION			0xFC21		//selects Authentication algorithm						<HCFL>
#define CFG_CNF_EXCL_UNENCRYPTED		0xFC22		//[AP] Switch for 'clear-text' rx message acceptance	<HCFL>
#define CFG_CNF_MCAST_RATE				0xFC23		//Transmit Data rate for Multicast frames				<HCFL>
#define CFG_CNF_INTRA_BSS_RELAY			0xFC24		//[AP] Switch for IntraBBS relay						<HCFL>
#define CFG_CNF_MICRO_WAVE				0xFC25		//MicroWave (Robustness)
	
	
//	NETWORK PARAMETERS, DYNAMIC CONFIGURATION ENTITIES
#define CFG_DESIRED_SSID				0xFC02		//[STA] Service Set identification for connection
#define CFG_GROUP_ADDR					0xFC80		//[STA] Multicast MAC Addresses for Rx-message
#define CFG_CREATE_IBSS					0xFC81		//[STA] Switch for IBSS creation On/Off
#define CFG_FRAGMENTATION_THRH			0xFC82		//[STA] Fragment length for unicast Tx-message
#define CFG_RTS_THRH					0xFC83		//[STA] Frame length used for RTS/CTS handshake
#define CFG_TX_RATE_CONTROL				0xFC84		//[STA] Data rate control for message transmission
#define CFG_PROMISCUOUS_MODE			0xFC85		//[STA] Switch for Promiscuous mode reception On/Off
	
#define CFG_FRAGMENTATION_THRH0   		0xFC90		//[AP] Port 0 fragment length for unicast Tx-message	<HCFL@
#define CFG_FRAGMENTATION_THRH1   		0xFC91		//[AP] Port 1 fragment length for unicast Tx-message
#define CFG_FRAGMENTATION_THRH2   		0xFC92		//[AP] Port 2 fragment length for unicast Tx-message
#define CFG_FRAGMENTATION_THRH3   		0xFC93		//[AP] Port 3 fragment length for unicast Tx-message
#define CFG_FRAGMENTATION_THRH4   		0xFC94		//[AP] Port 4 fragment length for unicast Tx-message
#define CFG_FRAGMENTATION_THRH5   		0xFC95		//[AP] Port 5 fragment length for unicast Tx-message
#define CFG_FRAGMENTATION_THRH6   		0xFC96		//[AP] Port 6 fragment length for unicast Tx-message
#define CFG_RTS_THRH0					0xFC97		//[AP] Port 0 frame length for RTS/CTS handshake
#define CFG_RTS_THRH1					0xFC98		//[AP] Port 1 frame length for RTS/CTS handshake
#define CFG_RTS_THRH2					0xFC99		//[AP] Port 2 frame length for RTS/CTS handshake
#define CFG_RTS_THRH3					0xFC9A		//[AP] Port 3 frame length for RTS/CTS handshake
#define CFG_RTS_THRH4					0xFC9B		//[AP] Port 4 frame length for RTS/CTS handshake
#define CFG_RTS_THRH5					0xFC9C		//[AP] Port 5 frame length for RTS/CTS handshake
#define CFG_RTS_THRH6					0xFC9D		//[AP] Port 6 frame length for RTS/CTS handshake
#define CFG_TX_RATE_CONTROL0			0xFC9E		//[AP] Port 0 data rate control for transmission
#define CFG_TX_RATE_CONTROL1			0xFC9F		//[AP] Port 1 data rate control for transmission
#define CFG_TX_RATE_CONTROL2			0xFCA0		//[AP] Port 2 data rate control for transmission
#define CFG_TX_RATE_CONTROL3			0xFCA1		//[AP] Port 3 data rate control for transmission
#define CFG_TX_RATE_CONTROL4			0xFCA2		//[AP] Port 4 data rate control for transmission
#define CFG_TX_RATE_CONTROL5			0xFCA3		//[AP] Port 5 data rate control for transmission
#define CFG_TX_RATE_CONTROL6			0xFCA4		//[AP] Port 6 data rate control for transmission		@HCFL>
#define CFG_CNF_DEFAULT_KEYS			0xFCB0		//defines set of encryption keys
#define CFG_CNF_TX_KEY_ID				0xFCB1		//select key for encryption of Tx messages


//	BEHAVIOR PARAMETERS	
#define CFG_TICK_TIME					0xFCE0		//[PRI] Auxiliary Timer tick interval
#define CFG_RID_CFG_MAX					0xFCFF		//highest value representing an Configuration RID


/*============================================================= INFORMATION RECORDS 	=====================*/
/*============================================================= mask 0xFDxx				=====================*/
//	NIC INFORMATION	
#define CFG_RID_INF_MIN					0xFD00		//lowest value representing an Information RID
#define CFG_MAX_LOAD_TIME				0xFD00	//[PRI] Maximum response time of the Download command.
#define CFG_DL_BUF						0xFD01	//[PRI] Download buffer location and size.
#define CFG_PRI_IDENTITY				0xFD02  //[PRI] Primary Functions firmware identification.
#define CFG_PRI_SUP_RANGE				0xFD03	//[PRI] Primary Functions interface; Supplier compatibility range.
#define CFG_CFI_ACT_RANGES_PRI			0xFD04  //[PRI] Controller interface; Actor compatibility ranges.

#define CFG_HSI_SUP_RANGE				0xFD09	//H/W - S/W I/F supplier range
#define CFG_NIC_SERIAL_NUMBER			0xFD0A  //[PRI] Network Interface Card serial number.
#define CFG_NIC_IDENTITY				0xFD0B  //[PRI] Network Interface Card identification.
#define CFG_MFI_SUP_RANGE				0xFD0C  //[PRI] Modem interface, Supplier compatibility range.
#define CFG_CFI_SUP_RANGE				0xFD0D  //[PRI] Controller interface; Supplier compatibility range.
#define CFG_CHANNEL_LIST				0xFD10	//Allowed communication channels.
#define CFG_REG_DOMAINS					0xFD11	//List of intended regulatory domains.
#define CFG_TEMP_TYPE  					0xFD12	//Hardware temperature range code.
#define CFG_CIS							0xFD13	//PC Card Standard Card Information Structure
#define CFG_CARD_PROFILE				0xFD14	//Card Profile

#define CFG_STA_IDENTITY				0xFD20   //[STA] Station Functions firmware identification.
#define CFG_STA_SUP_RANGE				0xFD21	 //[STA] Station Functions interface; Supplier compatibility range.
#define CFG_MFI_ACT_RANGES_STA			0xFD22   //[STA] Modem interface; Actor compatibility ranges.
#define CFG_CFI_ACT_RANGES_STA			0xFD23   //[STA] Controller interface; Actor compatibility ranges.

//	MAC INFORMATION
#define CFG_PORT_STAT					0xFD40		//[STA] Actual MAC Port connection control status
#define CFG_CURRENT_SSID				0xFD41		//[STA] Identification of the actually connected SS
#define CFG_CURRENT_BSSID				0xFD42		//[STA] Identification of the actually connected BSS
#define CFG_COMMS_QUALITY				0xFD43		//[STA] Quality of the Basic Service Set connection
#define CFG_CURRENT_TX_RATE				0xFD44		//[STA] Actual transmit data rate
#define CFG_OWN_BEACON_INTERVAL			0xFD45		//Beacon transmit interval time for BSS creation
#define CFG_CUR_SCALE_THRH				0xFD46		//Actual System Scale thresholds settings
#define CFG_PROTOCOL_RSP_TIME			0xFD47		//Max time to await a response to a request message
#define CFG_SHORT_RETRY_LIMIT			0xFD48		//Max number of transmit attempts for short frames
#define CFG_LONG_RETRY_LIMIT			0xFD49		//Max number of transmit attempts for long frames
#define CFG_MAX_TX_LIFETIME				0xFD4A		//Max transmit frame handling duration
#define CFG_MAX_RX_LIFETIME				0xFD4B		//Max received frame handling duration
#define CFG_CF_POLLABLE					0xFD4C		//[STA] Contention Free pollable capability indication
#define CFG_AUTHENTICATION_ALGORITHMS	0xFD4D		//Available Authentication Algorithms indication
#define CFG_AUTHENTICATION_TYPE			0xFD4E		//Available Authentication Types indication
#define CFG_PRIVACY_OPTION_IMPLEMENTED	0xFD4F		//WEP Option availability indication
#define CFG_CURRENT_REMOTE_RATES		0xFD50		//CurrentRemoteRates
#define CFG_CURRENT_USED_RATES			0xFD51		//CurrentUsedRates
	
#define CFG_CURRENT_TX_RATE1			0xFD80		//		// 	[AP] Actual Port 1 transmit data rate	<HCFL@
#define CFG_CURRENT_TX_RATE2			0xFD81		//[AP] Actual Port 2 transmit data rate
#define CFG_CURRENT_TX_RATE3			0xFD82		//[AP] Actual Port 3 transmit data rate
#define CFG_CURRENT_TX_RATE4			0xFD83		//[AP] Actual Port 4 transmit data rate
#define CFG_CURRENT_TX_RATE5			0xFD84		//[AP] Actual Port 5 transmit data rate
#define CFG_CURRENT_TX_RATE6			0xFD85		//[AP] Actual Port 6 transmit data rate
#define CFG_OWN_MACADDRESS				0xFD86		//[AP] Unique local node MAC Address
#define CFG_PCF_INFO					0xFD87		//[AP] Point Coordination Function capability info	@HCFL>

//	MODEM INFORMATION	
#define CFG_PHY_TYPE					0xFDC0		//		// 	Physical layer type indication
#define CFG_CURRENT_CHANNEL				0xFDC1		//Actual frequency channel used for transmission
#define CFG_CURRENT_POWER_STATE			0xFDC2		//Actual power consumption status
#define CFG_CCAMODE						0xFDC3		//Clear channel assessment mode indication
#define CFG_CCATIME						0xFDC4		//Clear channel assessment time
#define CFG_MAC_PROCESSING_DELAY		0xFDC5		//MAC processing delay time
#define CFG_SUPPORTED_DATA_RATES		0xFDC6		//Data rates capability information

#define CFG_RID_INF_MAX					0xFDFF		//highest value representing an Information RID

//} hcf_info_type;




/*************************************************************************************************************/

/****************************** S T R U C T U R E   D E F I N I T I O N S ************************************/

typedef struct LTV_STRCT {	//used for all "minimal" LTV records
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	val[1];					//do not change this, some dynamic structures are defined based on this !!
}LTV_STRCT;

typedef LTV_STRCT FAR *	LTVP;

#define CFG_MAX_LOAD_TIME_STRCT	LTV_STRCT   //<HCFL>

typedef struct CFG_DRV_INFO_STRCT {		//CFG_DRV_INFO (0x0825) driver information
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_8	driver_name[8];			//Driver name, 8 bytes, right zero padded
	hcf_16	driver_version;			//BCD 2 digit major and 2 digit minor driver version
	hcf_16	HCF_version;   			//BCD 2 digit major and 2 digit minor HCF version
	hcf_16	driver_stat;			//
	hcf_16	IO_address;				//IOAddress used by NIC
	hcf_16	IO_range;				//IORange used by NIC
	hcf_16	IRQ_number;				//Interrupt used by NIC
	hcf_16	card_stat;				/*NIC status
									@*	0x8000	Card present
									@*	0x4000	Card Enabled
									@*	0x2000	Driver incompatible with NIC Primary Functions
									@*	0x1000	Driver incompatible with NIC Station Functions
									@*	0x007F	HCF Internal Flags (i.e. pseudo Asynchronous command code)  */
	hcf_16	frame_type;				/*Frame type
									@*	0x000	802.3
									@*	0x008	802.11														*/
	hcf_32	drv_info;				/*driver specific info
									 * CE: virtual I/O base													*/
}CFG_DRV_INFO_STRCT;


typedef struct CFG_IDENTITY_STRCT {		//CFG_DRV_IDENTITY (0x0826), ...... 
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
									//The following fields are defined in DS 010335
	hcf_16	comp_id;                //  component id
	hcf_16	variant;				//  variant (01..99)
	hcf_16	version_major;			//  major version (01..99)
	hcf_16	version_minor;			//  minor version (01..99)
} CFG_IDENTITY_STRCT;

#define COMP_ID_MINIPORT_NDIS_31	41				//Windows 9x/NT Miniport NDIS 3.1
#define COMP_ID_PACKET				42				//Packet
#define COMP_ID_ODI_16				43				//DOS ODI
#define COMP_ID_ODI_32				44				//32-bits ODI
#define COMP_ID_MAC_OS				45				//Macintosh OS
#define COMP_ID_WIN_CE				46				//Windows CE Miniport
#define COMP_ID_LINUX_PD			47				/*Linux, full source code in Public Domain, HCF-light based
													 *thanks to Andreas Neuhaus								*/
#define COMP_ID_MINIPORT_NDIS_50	48				//Windows 9x/NT Miniport NDIS 5.0
#define COMP_ID_LINUX_LIB			49				//Linux, HCF-library based
#define COMP_ID_QNX					50				//QNX
#define COMP_ID_MINIPORT_NDIS_40	51				//Windows 9x/NT Miniport NDIS 4.0
#define COMP_ID_BEOS				77				// Why wait? Go BeOS!
#define COMP_ID_AP1					81				//WaveLAN/IEEE AP
#define COMP_ID_EC					83				//WaveLAN/IEEE Ethernet Converter






typedef struct CFG_RANGE_SPEC_STRCT {		    //<HCFL@
	hcf_16	number;
	hcf_16	bottom;
	hcf_16	top;
} CFG_RANGE_SPEC_STRCT; //@HCFL>

typedef struct CFG_RANGES_STRCT {			//CFG_DRV_SUP_RANGE (0x0827), CFG_DRV_ACT_RANGES_STA (0x0829)  	//<HCFL@
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	
	hcf_16	role;
	hcf_16	id;
	CFG_RANGE_SPEC_STRCT	variant[1];
} CFG_RANGES_STRCT; //@HCFL>

typedef struct CFG_RANGE2_STRCT {			//CFG_DRV_ACT_RANGES_PRI (0x0828)								//<HCFL@
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	
	hcf_16	role;
	hcf_16	id;
	CFG_RANGE_SPEC_STRCT	variant[2];
} CFG_RANGE2_STRCT; //@HCFL>

typedef struct CFG_RANGE3_STRCT {			//CFG_DRV_ACT_RANGES_PRI (0x0828)								//<HCFL@
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	
	hcf_16	role;
	hcf_16	id;
	CFG_RANGE_SPEC_STRCT	variant[3];
} CFG_RANGE3_STRCT; //@HCFL>

typedef struct CFG_RANGE20_STRCT {			//CFG_DRV_ACT_RANGES_PRI (0x0828)								//<HCFL@
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	
	hcf_16	role;
	hcf_16	id;
	CFG_RANGE_SPEC_STRCT	variant[20];		//should suffice for a while
} CFG_RANGE20_STRCT; //@HCFL>

#define COMP_ROLE_SUPL	00				//supplier
#define COMP_ROLE_ACT	01				//actor

#define COMP_ID_MFI		01				//Modem		 		- Firmware	I/F
#define COMP_ID_CFI		02				//Controller		- Firmware	I/F
#define COMP_ID_PRI		03				//Primary Firmware	- Driver	I/F
#define COMP_ID_STA		04				//Station Firmware	- Driver	I/F
#define COMP_ID_DUI		05				//Driver			- Utility	I/F
#define COMP_ID_HSI		06				//H/W               - Driver	I/F


																											//<HCFL@
typedef struct CFG_RESTRAINT_INFO_STRCT {	//CFG_RESTRAINT_INFO (0x0823) accumulated set of warnings detected by driver
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//CFG_RESTRAINT_INFO (0x0823) identification
	hcf_16	val[2];					/* val[0]: First 16 warnings
									 *	0x0001: CFG_CNF_PM_ENABLED (0xFC09) while not supported by firmware	
									 *
									 * val[1]: Second 16 warnings
									 */
} CFG_RESTRAINT_INFO_STRCT;


//@HCFL>
typedef struct CFG_REG_MB_STRCT {			//0x0830		//Register Mail Box
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//CFG_REG_MB (0x0830) identification
	hcf_16	FAR * mbp;				//pointer to MailBox
	hcf_16	mbsize;					//size of MailBox in words
} CFG_REG_MB_STRCT;
//<HCFL@

typedef struct CFG_CNTL_RESTRAINT_STRCT {	//CFG_CNTL_RESTRAINT (0x0834) accumulated set of warnings detected by driver
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//CFG_CNTL_RESTRAINT (0x0834) identification
	hcf_16	action;					// set/rest command
	hcf_16	stat[1];				/* individual restraint bits
									 * MSF_CRYPT_RESTRAINT_BIT	0x0001
									 */
} CFG_CNTL_RESTRAINT_STRCT;
																										//@HCFL>

/*	the "right" prototype would be
	typedef void (FAR MSF_ASSERT_RTN)( wci_bufp, unsigned int, hcf_16, int );
	rather than 
	typedef void (MSF_ASSERT_RTN)( wci_bufp, unsigned int, hcf_16, int );
	However, this results in sheer unsurmountable problems to build and link TSRs
	using Microsoft Visual C 1.5 compiler and linker.
	Since most users define FAR empty anyway, it does not matter for most users
	The exceptions are: DOS ODI, Packet Driver, WCITST, AccessPoint
	Since C is so nice to clear the stack by the caller, the FAR/NEAR issue does not cause
	problems in the call/return mechanism
	By carefully misleading the compiler in the msf_assert routine, we can compensate
	for the offset difference of 2 of the parameters in case of a far or near call
 */
typedef void (/*FAR*/ MSF_ASSERT_RTN)( wci_bufp, hcf_16, hcf_16, int );
typedef MSF_ASSERT_RTN FAR * MSF_ASSERT_RTNP;

typedef struct CFG_REG_ASSERT_RTNP_STRCT  {	//CFG_REG_ASSERT_RTNP (0x0832)	(de-)register MSF Callback routines
	hcf_16			len;			//default length of RID
	hcf_16			typ;			//CFG_REG_ASSERT_RTNP (0x0832) identification
	hcf_16			lvl;			//Assert level filtering (not yet implemented)
	MSF_ASSERT_RTNP	rtnp;			//address of MSF_ASSERT_RTN (native Endian format)
} CFG_REG_ASSERT_RTNP_STRCT;

																										//<HCFL@
typedef void (/*FAR*/ MSF_LOG_RTN)( int rtn_id, int val_1, int val_2);
typedef MSF_LOG_RTN FAR * MSF_LOG_RTNP;

typedef struct CFG_REG_LOG_RTNP_STRCT  {	//CFG_REG_LOG_RTNP (0x0837)	(de-)register MSF Log Callback routines
	hcf_16			len;			//default length of RID
	hcf_16			typ;			//CFG_REG_LOG_RTNP (0x0837) identification
	hcf_16			lvl;			//Log level filtering
	MSF_LOG_RTNP	rtnp;			//address of MSF_LOG_RTN (native Endian format)
} CFG_REG_MSF_LOG_RTNP_STRCT;
																										//@HCFL>

																										//<HCFL@
//typedef struct CFG_DLNV_START_STRCT {	//CFG_DLNV_START (0x0850) Setup for Download to non-volatile RAM
typedef struct CFG_DL_START_STRCT {	/* CFG_DLNV_START (0x0850) Setup for Download to non-volatile RAM
									 * CFG_DLV_START (0x0852) Setup for Download to volatile RAM			*/
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	low_addr;				//Low order part of 32-bits non-volatile NIC RAM address
	hcf_16	high_addr;				//High order part of 32-bits non-volatile NIC RAM address
} CFG_DL_START_STRCT;
																										//@HCFL>

typedef struct CFG_CNF_ENCRYPTION_STRCT {	//CFG_CNF_ENCRYPTION (0xFC20) select en/de-cryption of Tx/Rx messages
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
    hcf_16  encryption_enabled;     //Encryption Enabled
} CFG_CNF_ENCRYPTION_STRCT;

typedef struct CFG_CNF_AUTHENTICATION_STRCT {//CFG_CNF_AUTHENTICATION (0xFC21) selects Authentication algorithm
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
    hcf_16  authentication_type;   // Authentication type
} CFG_CNF_AUTHENTICATION_STRCT;

//CFG_CNF_DEFAULT_KEYS (0xFCB0) defines set of encryption keystypedef struct 
//typedef struct CFG_CNF_DEFAULT_KEYS_STRCT { //CFG_CNF_DEFAULT_KEYS (0xFCB0) defines set of encryption keys
//    hcf_16  len;                    //default length of RID
//    hcf_16  typ;                    //RID identification as defined by Hermes
//    hcf_16  len_0;              	//length of 1st key
//    hcf_8   key_0[14];              //1st encryption key
//    hcf_16  len_1;              	//length of 2nd key
//    hcf_8   key_1[14];              //2nd encryption key
//    hcf_16  len_2;              	//length of 3rd key
//    hcf_8   key_2[14];              //3rd encryption key
//    hcf_16  len_3;              	//length of 4th key
//    hcf_8   key_3[14];              //4th encryption key
//} CFG_CNF_DEFAULT_KEYS_STRCT;

typedef struct KEY_STRCT {
    hcf_16  len;	              	//length of key
    hcf_8   key[14];				//encryption key
} KEY_STRCT;

typedef struct CFG_CNF_DEFAULT_KEYS_STRCT {	//CFG_CNF_DEFAULT_KEYS (0xFCB0) defines set of encryption keys
	hcf_16		len;				//default length of RID
	hcf_16		typ;				//RID identification as defined by Hermes
	KEY_STRCT	key[4];				//encryption keys
} CFG_CNF_DEFAULT_KEYS_STRCT;

typedef struct CFG_CNF_TX_KEY_ID_STRCT {	//CFG_CNF_TX_KEY_ID (0xFCB1) select key for encryption of Tx messages
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	tx_key_id;				//specifies which key of CFG_CNF_DEFAULT_KEYS is to be used for Tx-encryption
} CFG_CNF_TX_KEY_ID_STRCT;

																										//<HCFL@
typedef struct CFG_DL_BUF_STRCT {		//CFG_DL_BUF (0xFD01) Download buffer location and size [PRI]
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	buf_page;				//Page Address of the Intermediate Download Buffer
	hcf_16	buf_offset;				//Offset of the Intermediate Download Buffer
	hcf_16	buf_len;				//Size of the Intermediate Download Buffer in bytes
} CFG_DL_BUF_STRCT;


typedef struct CFG_MEM_SIZES_STRCT {	//CFG_MEM_SIZES (0xFD02) Volatile and non-volatile memory sizes [PRI]
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	mem_size_volatile;
	hcf_16	mem_size_non_volatile;
}CFG_MEM_SIZES_STRCT;
																										//@HCFL>
typedef struct CFG_NIC_SERIAL_NUMBER_STRCT {	//CFG_NIC_SERIAL_NUMBER (0xFD0A) NIC Serial number
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	number[6];
}CFG_NIC_SERIAL_NUMBER_STRCT;

typedef struct CFG_REG_DOMAINS_STRCT {	//CFG_REG_DOMAINS (0xFD11) List of intended regulatory domains.
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	num_domains;			//Number of domains in list
	hcf_8	domain[10];				//List of domains
}CFG_REG_DOMAINS_STRCT;


typedef struct CFG_CIS_STRCT {			//CFG_CIS (0xFD13) PC Card Standard Card Information Structure
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	cis[240];				//Compact CIS Area, a linked list of tuples
}CFG_CIS_STRCT;


typedef struct CFG_CARD_PROFILE_STRCT {		//CFG_CARD_PROFILE (0xFD14) CardProfile
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16  profile_code;				//a code specifying the card profile
	hcf_16  capability_options;			//capability options, used to be known as wep_options
	hcf_16  allowed_data_rates;			//allowed data rates
} CFG_CARD_PROFILE_STRCT;


typedef struct CFG_COMMS_QUALITY_STRCT {//CFG_COMMS_QUALITY (0xFD43) Quality of the Basic Service Set connection [STA]
	hcf_16	len;					//length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	coms_qual;              //Communication Quality of the BSS the station is connected to
	hcf_16	signal_lvl;				//Average Signal Level of the BSS the station is connected to
	hcf_16	noise_lvl;				//Average Noise Level of the currently used Frequency Channel
}CFG_COMMS_QUALITY_STRCT;



typedef struct CFG_CUR_SCALE_THRH_STRCT {//CFG_CUR_SCALE_THRH (0xFD46) Actual System Scale thresholds
	hcf_16	len;					//default length of RID [STA: 6  AP: 4]
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	energy_detect_thrh;		//Receiver H/W Energy Detect Threshold
	hcf_16	carrier_detect_thrh;	//Receiver H/W Carrier Detect Threshold
	hcf_16	defer_thrh;				//Receiver H/W Defer Threshold
	hcf_16	cell_search_thrh;		//Firmware Roaming Cell Search Threshold [STA]
	hcf_16	out_of_range_thrh;		//Firmware Roaming Out of Range Threshold [STA]
	hcf_16	delta_snr;				//Firmware Roaming Delta SNR value [STA]
}CFG_CUR_SCALE_THRH_STRCT;


typedef struct CFG_PCF_INFO_STRCT {		//CFG_PCF_INFO (0xFD87) Point Coordination Function capability info [AP]
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
    hcf_16  mediumOccupancyLimit;
    hcf_16  CFPPeriod;
    hcf_16  CFPMaxDuration;
}CFG_PCF_INFO_STRCT;


typedef struct CFG_MAC_ADDR_STRCT{			//0xFC01	[STA] MAC Address of this node.
											//0xFC08	STA] MAC Address of corresponding WDS Link node.
											//0xFC11	[AP] Port 1 MAC Adrs of corresponding WDS Link node
											//0xFC12	[AP] Port 2 MAC Adrs of corresponding WDS Link node
											//0xFC13	[AP] Port 3 MAC Adrs of corresponding WDS Link node
											//0xFC14	[AP] Port 4 MAC Adrs of corresponding WDS Link node
											//0xFC15	[AP] Port 5 MAC Adrs of corresponding WDS Link node
											//0xFC16	[AP] Port 6 MAC Adrs of corresponding WDS Link node
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	mac_addr[3];
}CFG_MAC_ADDR_STRCT;

typedef struct CFG_GROUP_ADDR_STRCT{			//0xFC80		//[STA] Multicast MAC Addresses for Rx-message
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	mac_addr[GROUP_ADDR_SIZE/2];
}CFG_GROUP_ADDR_STRCT;




typedef struct CFG_ID_STRCT {				//0xFC02	[STA] Service Set identification for connection.
											//0xFC04	IBSS creation (STA) or ESS (AP) Service Set Ident
											//0xFC0E	Identification text for diagnostic purposes.
	hcf_16	len;					//default length of RID
	hcf_16	typ;					//RID identification as defined by Hermes
	hcf_16	id[17];
}CFG_ID_STRCT;


																										//<HCFL@

typedef struct CFG_AUTHENTICATION_STRCT {   // 0xFD4C 0xFD4D 0xFD4E Authentication Algorithm
    hcf_16  len;                            // default length of RID
    hcf_16  typ;                            // RID identification as defined by Hermes
    hcf_16  authentication_type;             // Authentication type
    hcf_16  type_enabled;                    // Type Enabled
} CFG_AUTHENTICATION_STRCT;
																										//@HCFL>

typedef struct CFG_SUPPORTED_DATA_RATES_STRCT {
    hcf_16  len;                            // default length of RID
    hcf_16  typ;                            // RID identification as defined by Hermes
    hcf_16  rates[5];                       // Data rates
} CFG_SUPPORTED_DATA_RATES_STRCT;

typedef struct DUI_STRCT {																				//<HCFL@
	void  FAR	*ifbp;	/* Pointer to IFB
						 *	returned from MSF to USF by uil_connect
				 		 *	passed from USF to MSF as a "magic cookie" by all other UIL function calls
				 		 */
	hcf_16		stat;		// status returned from MSF to USF
	hcf_16		fun;		// command code from USF to MSF
	LTV_STRCT	ltv;	/* LTV structure
			 			 *** during uil_put_info:
			 			 *	  the L, T and V-fields carry information from USF to MSF
			 			 *** during uil_get_info:
						 *	  the L and T fields carry information from USF to MSF
						 *	  the L and V-fields carry information from MSF to USF
			 			 */
} DUI_STRCT;


typedef DUI_STRCT FAR *	DUIP;
																										//@HCFL>

#endif // MDD_H

