
/**************************************************************************************************************
*
* FILE   :	HCF.CPP *************** 2.0 ***********************************************************************
*
* DATE    :	$Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR :	Nico Valster
*
* DESC   :	HCF Routines (callable via the Wireless Connection I/F or WCI)
*			Local Support Routines for above procedures
*
*			Customizable via HCFCFG.H, which is included by HCF.H
*
***************************************************************************************************************
* COPYRIGHT (c) 1994, 1995		 				by AT&T.	 			All Rights Reserved.
* COPYRIGHT (c) 1996, 1997, 1998, 1999, 2000	by Lucent Technologies.	All Rights Reserved.
*
* At the sole discretion of Lucent Technologies parts of this source may be extracted
* and placed in the Public Domain under the GPL.
* This extraction takes place by means of an AWK-script acting on embedded tags.
* "<HCFL>", "<HCFL@" and "@HCFL>" are used as tags.
* The AWK script is:
 *	BEGIN { c = 0 }
 *	{ if ( c == 0 ) i = 1}			#if in @HCF_L>..@HCF_L< block, skip
 *	{ if (name != FILENAME ) name = FILENAME }
 *	/HCFASSERT/ { i = 0 }			#skip all HCFASSERT macro calls
 *	/hcf_debug_trigger/ { i = 0 }		#skip all hcf_debug_trigger calls
 *	/<HCFL>/ { i = 0 }			#skip all lines containing @HCF_L@
 *	/<HCFL@/ { c++ }			#nesting level of HCF_L increases
 *	/@HCFL>/ { c--; i = 0 }			#nesting level of HCF_L decreaeses
 *	
 *	{ if ( i && c == 0  ) { print ; hcf_l_cnt++ } }
 *	#{ if ( i ) { print ; hcf_l_cnt++ } }
 *	#{if ( c == 0 ) { printf("N%d", c) ; hcf_l_cnt++ } }
 *	#{if ( c == 1 ) { printf("E%d", c) ; hcf_l_cnt++ } }
 *	
 *	#END { printf("%s:: HCF lines: %d, HCF_Light lines: %d", name, NR, hcf_l_cnt ) }
*
* The source file containing the "<HCFL>", "<HCFL@" and "@HCFL>" tags is explicitely not in the Public Domain
* and is not in any sense derived from the extracted source.
*
**************************************************************************************************************/





/**************************************************************************************************************
*
* CHANGE HISTORY
*

  960702 - NV
	Original Entry - derived from WaveLAN-I HCF 2.12

$Log: hcf.c,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.31   11 Feb 2000 14:19:30   NVALST
 * sync with home
 * 
 *    Rev 1.30   11 Feb 2000 10:18:12   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.29   08 Feb 2000 09:28:38   NVALST
 * passed to reinier
 * 
 *    Rev 1.28   04 Feb 2000 14:44:48   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.27   04 Feb 2000 14:10:22   NVALST
 * intermediate, backup before inserting CDOC comments
 * 
 *    Rev 4.6   01 Feb 2000 12:02:24   NVALST
 * 
 *    Rev 4.6   28 Jan 2000 15:39:40   NVALST
 * 
 *    Rev 1.26   28 Jan 2000 13:27:36   NVALST
 * 
 *    Rev 4.5   30 Nov 1999 15:12:42   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 17:23:54   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 15:27:54   NVALST
 * 
 *    Rev 1.8   03 Nov 1999 16:42:58   NVALST
 * 
 *    Rev 4.2   10 Sep 1999 11:28:40   NVALST
 * 
 *    Rev 1.188   10 Sep 1999 11:25:08   NVALST
 * 
 *    Rev 4.2   09 Sep 1999 13:34:24   NVALST
 * 
 *    Rev 1.187   07 Sep 1999 13:23:32   NVALST
 * 
 *    Rev 4.1   31 Aug 1999 12:14:40   NVALST
 * 
 *    Rev 1.182   30 Aug 1999 11:58:38   NVALST
 * 
 *    Rev 4.0   07 Jul 1999 10:09:20   NVALST
 * 
 *    Rev 1.169   07 Jul 1999 09:45:02   NVALST
 * 
 *    Rev 2.22   29 Jun 1999 11:32:34   NVALST
 * 
 *    Rev 1.168   24 Jun 1999 11:19:48   NVALST
 * 
**************************************************************************************************************/


/**************************************************************************************************************

*
* ToDo
*
 1:	For all/most functions, update "MSF-accessible fields of Result Block:" entry
 2: Use the "numbered comments" in the NARRATIVE consistently, i.e. hcf_put_info
 3: hcf_put_data, hcf_send, hcf_send_diag_msg
	once the dust is settled whether hcf_put_data or hcf_send is the appropriate place is to specify port,
	it can be considered whether part of the hcf_send_diag_msg and hcf_send can be isolated in a common
	routine.
 4:	hcf_send_diag_msg:
  	- what are the appropriate return values
 5: When optimization seems to become an issue, the numerous hcfio_string() calls in hcf_service_nic
	may be suitable candidates to get some attention 	
 6:	review/walkthough potential problems witrh re-entrancy when HCF_ACT_INT_OFF is called from task level
	and being interrupted

*
* Implementation Notes
*
 -	C++ style cast is not used to keep DA-C happy
 -	a leading marker of //! is used. The purpose of such a sequence is to help the
	(maintenance) programmer to understand the flow
 	An example in hcf_action( HCF_ACT_802_3 ) is
	//!		ifbp->IFB_RxFence = 0;
	which is superfluous because IFB_RxFence gets set at every hcf_service_nic but
	it shows to the (maintenance) programmer it is an intentional omission at
	the place where someone could consider it most appropriate at first glance
 -	using near pointers in a model where ss!=ds is an invitation for disaster, so be aware of how you specify
 	your model and how you define variables which are used at interrupt time
 -	Once the comment "the value of -1 for parameter len is meaningless but it guarantees that the next call
 	to bap_ini is interpreted as an initial call, causing the BAP to be really initialized." was considered
 	useful information. Does this trick still lingers somewhere;?
 -	remember that sign extension on 32 bit platforms may cause problems unless code is carefully constructed,
 	e.g. use "(hcf_16)~foo" rather than "~foo"

	
*
* Miscellaneous Notes
*
 -	AccessPoint performance could be improved by adding a hcf_send_pif_msg equivalent of hcf_send_diag_msg


*************************************************************************************************************/

#include "hcf.h"				// HCF and MSF common include file
#include "hcfdef.h"				// HCF specific include file

/*************************************************************************************************************/
/***************************************  PROTOTYPES  ********************************************************/
/*************************************************************************************************************/
// moving these prototypes to HCFDEF.H turned out to be less attractive in the HCF-light generation
STATIC hcf_16		alloc( IFBP ifbp, int len );
STATIC int			hcf_aux_cntl( IFBP ifbp, hcf_16 cmd );
STATIC int			calibrate( IFBP ifbp );
STATIC hcf_16		check_comp( IFBP ifbp, CFG_RANGES_STRCT *p, hcf_16 type );              //<HCFL>
STATIC int			cmd_wait( IFBP ifbp, hcf_16 cmd_code, int par_0 );
#if !defined HCF_DOWNLOAD_OFF
STATIC int			download(IFBP ifbp, LTVP ltvp );              							//<HCFL>
STATIC int			download_data( IFBP ifbp, LTVP ltvp, hcf_16 offset, hcf_16 page );
#endif // HCF_DOWNLOAD_OFF
STATIC void			enable_int(IFBP ifbp, int event );
STATIC int			enc_test( wci_recordp type );              								//<HCFL>
#if !defined HCF_MB_OFF
STATIC void			get_info_mb( IFBP ifbp, LTVP ltvp );              						//<HCFL>
#endif // HCF_MB_OFF
STATIC int			hcf_initialize( IFBP ifbp, int cntl );
STATIC int			ini_hermes( IFBP ifbp, int cntl );
STATIC void	 		isr_info( IFBP ifbp );
STATIC wci_recordp	mb_idx_inc( IFBP ifbp, wci_recordp ip );              					//<HCFL>
#if defined _M_I86TM                    													//<HCFL@
STATIC void			patch_catch( void );
#endif //_M_I86TM                       													//@HCFL>
STATIC int			put_info( IFBP ifbp, LTVP ltvp	);
#if !defined HCF_MB_OFF
STATIC void			put_info_mb( IFBP ifbp, hcf_16 type, wci_recordp bufp, hcf_16 len );	//<HCFL>
#endif // HCF_MB_OFF
#if HCF_MAX_CONFIG != 0																		//<HCFL@
static void			put_info_tbl( IFBP ifbp, LTVP ltvp	);
#endif // HCF_MAX_CONFIG																	//@HCFL>


#if defined HCF_PROFILING || defined HCF_DEBUG
void ppdbgClear( unsigned char maskbits );
void ppdbgPulse( unsigned char maskbits );
void ppdbgSet( unsigned char maskbits );

#define LPT1     0x03BC
#define LPT2     0x0378
#define LPT3     0x0278

#define PORT     LPT2
#endif // HCF_PROFILING || HCF_DEBUG


#if 0x511 != (HCMD_INQUIRE | HREG_STAT_INQUIRE_ERR)
error; /* time to panick, did you also modify HCF_ERR_INQUIRE to reflect the apparently redefined Hermes - Host I/F 
		* I would rather have had an expression like
		* #if HCF_ERR_INQUIRE != (HCMD_INQUIRE | HREG_STAT_INQUIRE_ERR)
		* but C apparently does not handle comapring enums with #define as I would like it
		*/
#endif



/**************************************************************************************************************
******************************* D A T A    D E F I N I T I O N S **********************************************
**************************************************************************************************************/

STATIC hcf_8 BASED hcf_rev[] = "HCF$Revision: 1.1 $\n";

#ifdef HCF_ASSERT       /*<HCFL@*/

STATIC struct {
	hcf_16	len;					//length of assert_strct
	hcf_16	trace;					//trace log copy from IFB
	hcf_16	qualifier;				//qualifier from entry parameter
	hcf_16	line_number;			//line number from entry parameter
	char val[sizeof(__FILE__)];
	char align;
} BASED assert_strct = { 3 + (sizeof(__FILE__)+1)/sizeof(hcf_16), 0, 0, 0, __FILE__, '\0' };
						//3:  trace, qualifier, line_number
						//+1: padding which MAY be needed depending on lenght file-name
#endif // HCF_ASSERT

/* translation table to control the exceptions to tunnel-bridging */
STATIC hcf_16 BASED enc_trans_tbl[] = {	0x80F3, 	//AppleTalk Address Resolution Protocol (AARP)
										0x8137		//IPX
					  };

/* SNAP header to be inserted in Ethernet-II frames */
STATIC  hcf_8 BASED snap_header[] = { 0, 0, 						//filler, real value put by hcf_send
									  0xAA, 0xAA, 0x03, 0x00, 0x00,	//5 bytes signature +
									  0x00 };						//1 byte work storage (protocol identifier)



#if defined MSF_COMPONENT_ID
#if MSF_COMPONENT_ID != COMP_ID_LINUX_PD && MSF_COMPONENT_ID !=	COMP_ID_LINUX_LIB
CFG_IDENTITY_STRCT BASED cfg_drv_identity = {
	sizeof(cfg_drv_identity)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_IDENTITY,			// (0x0826)
	MSF_COMPONENT_ID,
	MSF_COMPONENT_VAR,
	MSF_COMPONENT_MAJOR_VER,
	MSF_COMPONENT_MINOR_VER
} ;
#endif // MSF_COMPONENT_ID != COMP_ID_LINUX_PD && MSF_COMPONENT_ID !=	COMP_ID_LINUX_LIB

CFG_RANGES_STRCT BASED cfg_drv_sup_range = {
	sizeof(cfg_drv_sup_range)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_SUP_RANGE,			// (0x0827)
	
	COMP_ROLE_SUPL,
	COMP_ID_DUI,
	{{	DUI_COMPAT_VAR,
		DUI_COMPAT_BOT,
		DUI_COMPAT_TOP
	}}
} ;

// TEMPORARILY FOR win2000	
//struct CFG_RANGES_STRCT BASED cfg_drv_act_range_pri = {
//	sizeof(cfg_drv_act_range_pri)/sizeof(hcf_16) - 1,	//length of RID
//	CFG_DRV_ACT_RANGE_PRI,		// (0x0828)
//	COMP_ROLE_ACT,
//	COMP_ID_PRI,
//	{
//     {	1, //CFG_DRV_ACT_PRI_VAR1			//variant[1] - Variant number
//		CFG_DRV_ACT_RANGE_PRI_BOTTOM,       //           - Bottom Compatibility
//		CFG_DRV_ACT_RANGE_PRI_TOP           /*           - Top Compatibility
//											 *				Hermes needs 2 as top, if top == 1, Shark only
//											 */
//	 }
//    }
//};

// TEMPORARILY FOR Engineering during Primary Variant 2 development
struct CFG_RANGE2_STRCT BASED cfg_drv_act_range_pri = {
	sizeof(cfg_drv_act_range_pri)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGE_PRI,		// (0x0828)
	
	COMP_ROLE_ACT,
	COMP_ID_PRI,
	{
/* replace hard coded values with terms of the nature CFG_DRV_ACT_PRI_BOTTOM_VAR2
 * this has its consequences for the MSF-customization of config management.
 * to be worked out soon
 */
     {	2, 									//variant[2] - Variant number
		CFG_DRV_ACT_RANGE_PRI_2_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGE_PRI_2_TOP   		//          - Top Compatibility
	 },
     {	1, 									//variant[1] - Variant number
		CFG_DRV_ACT_RANGE_PRI_1_BOTTOM,    	//           - Bottom Compatibility
		CFG_DRV_ACT_RANGE_PRI_1_TOP         //           - Top Compatibility
	 }
    }
} ;


CFG_RANGES_STRCT BASED cfg_drv_act_range_sta = {
	sizeof(cfg_drv_act_range_sta)/sizeof(hcf_16) - 1,	//length of RID
	CFG_DRV_ACT_RANGE_STA,		// (0x0829)
	
	COMP_ROLE_ACT,
	COMP_ID_STA,
	{
     {	1,                      			//variant[1] - Variant number
		CFG_DRV_ACT_RANGE_STA_1_BOTTOM,		//           - Bottom Compatibility
		CFG_DRV_ACT_RANGE_STA_1_TOP			//           - Top Compatibility
	 }
    }
} ;

// !!!!!see note below!!!!!!!!!!
struct CFG_RANGE3_STRCT BASED cfg_drv_act_range_hsi = {
	sizeof(cfg_drv_act_range_hsi)/sizeof(hcf_16) - 1,	/*length of RID
														 * if not all three HCF_HSI_VAR_# are defined, this
														 * structure will contain 1 or more CFG_RANGE_SPEC_STRCT
														 * with all zeros. A zero for top and bottom are invalid
														 * hence no match with any existing valid F/W definition
														 */
	CFG_DRV_ACT_RANGE_HSI,		// (0x082A)
	
	COMP_ROLE_ACT,
	COMP_ID_HSI,
	{
#if defined HCF_HSI_VAR_0				// Controlled deployment
     {	0,     				       			//variant[1] - Variant number
		CFG_DRV_ACT_RANGE_HSI_0_BOTTOM,		//           - Bottom Compatibility	!!!!!see note below!!!!!!!!!!
		CFG_DRV_ACT_RANGE_HSI_0_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },	/* either I understand less of the C-syntax then I thought I understood, or Microsoft VC 1.52
	 				 * etc >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
#endif	 
#if defined HCF_HSI_VAR_1				// WaveLAN
     {	1,            						//variant[1] - Variant number
		CFG_DRV_ACT_RANGE_HSI_1_BOTTOM,		//           - Bottom Compatibility	!!!!!see note below!!!!!!!!!!
		CFG_DRV_ACT_RANGE_HSI_1_TOP			//           - Top Compatibility
	 },
#else
	 { 0, 0, 0 },	 
#endif	 
#if defined HCF_HSI_VAR_2				// HomeLAN
     {	2,            						//variant[1] - Variant number
		CFG_DRV_ACT_RANGE_HSI_2_BOTTOM,		//           - Bottom Compatibility	!!!!!see note below!!!!!!!!!!
		CFG_DRV_ACT_RANGE_HSI_2_TOP			//           - Top Compatibility
	 }
#else
	 { 0, 0, 0 }
#endif	 
    }
} ;

#endif // MSF_COMPONENT_ID

/*@HCFL>*/

/*
	The below table accessed via a computed index was the original implementation for hcf_get_info with
	CFG_DRV_IDENTITY, CFG_DRV_SUP_RANGE, CFG_DRV_ACT_RANGE_PRI, CFG_DRV_ACT_RANGE_STA, CFG_DRV_ACT_RANGE_HSI
	as type. However it was reported that the 68K compiler for MAC OS is unable to initialize pointers.
	Accepting this story at face value, the HCF is coded around this problem by implementing a direct access..
	To save part of the invested effort, the original table is kept as comment.

STATIC LTV_STRCT*   BASED xxxx[ ] = {
	(LTV_STRCT*)&cfg_drv_identity,      //CFG_DRV_IDENTITY              0x0826
	(LTV_STRCT*)&cfg_drv_sup_range,     //CFG_DRV_SUP_RANGE             0x0827
	(LTV_STRCT*)&cfg_drv_act_range_pri, //CFG_DRV_ACT_RANGE_PRI         0x0828
	(LTV_STRCT*)&cfg_drv_act_range_sta  //CFG_DRV_ACT_RANGE_STA         0x0829
	(LTV_STRCT*)&cfg_drv_act_range_hsi	//CFG_DRV_ACT_RANGE_HSI			0x082A
  };
*/


/**************************************************************************************************************
************************** T O P   L E V E L   H C F   R O U T I N E S ****************************************
**************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: hcf_action                                                                                2000/02/04 14:15
USERS: hcf_connect                                      hcf_disable                                      
       hcf_get_info                                     enable_int                                       
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                OUT_PORT_WORD                                    
       IN_PORT_WORD                                     hcf_initialize                                   
       hcf_enable                                       put_info_mb                                      
       cmd_wait                                         enable_int                                       
       HCFASSERTLOGEXIT                                 
DEFIN: AUX_MAGIC_1                                      CARD_STAT_AP                                     
       CARD_STAT_DEFUNCT                                CARD_STAT_ENABLED                                
       CARD_STAT_ENA_PRES                               CARD_STAT_PRESENT                                
       CFG_DIAG                                         CFG_SCAN                                         
       CFG_TALLIES                                      DO_ASSERT                                        
       HCF_ASSERT_ACTION                                HCF_INT_PENDING                                  
       HCF_MAGIC                                        HCMD_CMD_CODE                                    
       HCMD_DIAG                                        HCMD_INQUIRE                                     
       HFS_ADDR_1_ABS                                   HFS_ADDR_DEST_ABS                                
       HFS_TX_CNTL_802_11                               HFS_TX_CNTL_802_3                                
       HREG_CMD                                         HREG_EV_STAT                                     
       HREG_EV_TICK                                     HREG_INT_EN                                      
       HREG_PARAM_0                                     HREG_PARAM_1                                     
       HREG_SPARE                                       INI_COMPLETE                                     
       INI_PARTIAL                                      
GLOBL: HCF_ACT_802_11                                   HCF_ACT_802_3                                    
       HCF_ACT_802_3_PURE                               HCF_ACT_CARD_IN                                  
       HCF_ACT_CARD_OUT                                 HCF_ACT_DIAG                                     
       HCF_ACT_FREEZE                                   HCF_ACT_INT_FORCE_ON                             
       HCF_ACT_INT_OFF                                  HCF_ACT_INT_ON                                   
       HCF_ACT_SCAN                                     HCF_ACT_TALLIES                                  
       HCF_ACT_TICK_OFF                                 HCF_ACT_TICK_ON                                  
       HCF_ERR_DEFUNCT_FREEZE                           HCF_ERR_MAX                                      
       HCF_PORT_0                                       HCF_PORT_6                                       
       HCF_SUCCESS                                      UIL_ERR_PIF_CONFLICT                             
PARAM: action                                           ifbp                                             
       ifbp->IFB_AssertTrace                            ifbp->IFB_CardStat                               
       ifbp->IFB_DefunctStat                            ifbp->IFB_FSBase                                 
       ifbp->IFB_FrameType                              ifbp->IFB_IOBase                                 
       ifbp->IFB_IntEnMask                              ifbp->IFB_IntOffCnt                              
       ifbp->IFB_Magic                                  ifbp->IFB_PortStat                               
       ifbp->IFB_RxFence                                ifbp->IFB_TickIni                                
       ifbp->IFB_TxCntl                                 
LOCAL: i                                                j                                                
       prot_cnt                                         rc                                               
       scratch                                          
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_action
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			NW4
.APPLICATION	Card configuration
.DESCRIPTION	Changes the run-time Card behavior

.ARGUMENTS
  int hcf_action( IFBP ifbp, hcf_action_cmd action )

.RETURNS
	-- value --				-- parameter --
    HCF_SUCCESS 			all (including invalid)
    IFB_IntOffCnt     		HCF_ACT_INT_FORCE_ON, HCF_ACT_INT_ON
	HCF_INT_PENDING			HCF_ACT_INT_OFF, interrupt pending
	>>hcf_initialize		HCF_ACT_CARD_IN
    >>cmd_wait				HCF_ACT_SCAN, HCF_ACT_TALLIES

.NARRATIVE

 Parameters:
  ifbp		address of the Interface Block
  action	number identifying the type of change

  o HCF_ACT_INT_ON		enable interrupt generation by WaveLAN NIC
  o HCF_ACT_INT_OFF		disable interrupt generation by WaveLAN NIC
  o HCF_ACT_CARD_IN		MSF reported Card insertion
  o HCF_ACT_CARD_OUT	MSF reported Card removal
  o HCF_ACT_DIAG		Hermes Diagnose command                                 	//<HCFL>
  o HCF_ACT_TICK_ON		enable TimerTick interrupt              					//<HCFL>
  o HCF_ACT_TICK_OFF	disable TimerTick interrupt                                 //<HCFL>
  o HCF_ACT_802_3		activate 802.3 frame mode               					//<HCFL>
  o HCF_ACT_802_11		activate 802.11 frame mode              					//<HCFL>
  o HCF_ACT_802_3_PURE	activate 802.3 frame mode without encapsulation				//<HCFL>
  o HCF_ACT_TALLIES		Hermes Inquire Tallies (F100) command						//<HCFL>
  o HCF_ACT_SCAN		Hermes Inquire Scan (F101) command          				//<HCFL>
  o HCF_ACT_ASSERT_OFF	de-activate Assert reporting								//<HCFL>
  o HCF_ACT_ASSERT_ON	activate Assert reporting									//<HCFL>

 Returns:
  o	HCF_ACT_INT_OFF
		0: no interrupt pending
		1: interrupt pending
  o HCF_ACT_DIAG
    always returns HCF_SUCCESS, more info in MailBox
  o HCF_ACT_IN
    reports on the same conditions as hcf_enable, as a consequence of the start of day
    diagnose/selftest strategy
  o HCF_ACT_INT_FORCE_ON, HCF_ACT_INT_ON
    as an aid during debugging, IFB_IntOffCnt is reported
  o HCF_ACT_SCAN, HCF_ACT_TALLIES
    arbitrarily choosen to pass on the return code of cmd_wait, for all purposes as
    currently understood, the return code could just as well  be HCF_SUCCESS
  o	all other
		0 (((however, see the special treatment for HCF_ACT_INT_ON)))

 Remarks:
  o	HCF_ACT_INT_OFF/HCF_ACT_INT_ON codes may be nested but must be balanced. The INT_OFF/INT_ON housekeeping
	is initialized by hcf_connect with a call of hcf_action with INT_OFF, causing the interrupt generation
	mechanism to be disabled at first. This suits MSF implementation based on a polling strategy. An MSFT
	based on a interrupt strategy must call hcf_action with INT_ON in its initialization logic.

  o To prevent I/O while the I/O space is no longer owned by the HCF, due to a card swap, no I/O is allowed
	when the CARD_STAT_PRESENT bit of IFB_CardStat is off.

.DIAGRAM
 0:	<HCFL@
 	This assert checks against re-entrancy. Re-entrancy could be caused by a MSF logic at task-level calling
 	hcf_functions without shielding with HCF_ACT_ON/_OFF. However the HCF_ACT_INT_OFF action itself can
 	per definition not be protected this way. Based on code inspection, it can be conclude, that there is no
 	re-entrancy PROBLEM in this particular flow. Therefore the re-entrancy assert explicitely allows for it.
 	Note that there was a report of an MSF which ran into this assert.
 1:	This assert checks whether the HCF-specific codes have not grown into the area set aside for the DUI.
	This is implemented as runtime code because the preprocessor can not handle code like
	"#if (HCF_ERR_MAX) >= (UIL_ERR_PIF_CONFLICT)
	 error++;
	 #endif"
	with HCF_ERR_MAX and UIL_ERR_PIF_CONFLICT being enumerations.
	The "i = HCF_ERR_MAX" is just a kludge to get around compiler/lint warnings "conditional expression 
	is constant". The desirable statement would be something of the nature:
	HCFASSERT( HCF_ERR_MAX < UIL_ERR_PIF_CONFLICT, HCF_ERR_MAX )									@HCFL>
 2: IFB_IntOffCnt is used to balance the INT_OFF and INT_ON calls.
 4: Disabling of the interrupts is simply achieved by writing a zero to the Hermes IntEn register
 5: To be able to return the information to the MSF whether an interrupt is actually pending, the Hermes
	EvStat register is sampled and compared against the current IFB_IntEnMask value
 6:	Originally the construction "if ( ifbp->IFB_IntOffCnt-- <= 1 )" was used in stead of
 	"if ( --ifbp->IFB_IntOffCnt == 0 )". This serviced to get around the unsigned logic, but as additional
 	"benefit" it seemed the most optimal "fail safe" code (in the sense of shortest/quickest path in error
 	free flows, fail safe in the sense of too many INT_ON invocations compared to INT_OFF). However when a
 	real life MSF programmer ran to a MSF sequence problem, exactly causing that problem, he was annoyed
 	with this fail safe code. As a consequence it is taken out. As a side-effect of this unhappy MSF programmer
 	adventures to find his problem, the return status is defined to reflect the IFBIntOffCnt, Note that this
 	is solely intended for aid debugging, no MSF logic should depend on this feature, No garuantees for the
 	future are given.
 	Enabling of the interrupts is achieved by writing the contents of IFB_IntEnMask to the Hermes IntEn
 	register.
 7:	Since the card is present again, it must be re-initialized. Since this may be another card we may as well
 	clear all bits in IFB_CardStat and set only the "present" bit.
 	<HCFL> Based on IFB_portStat, all the ports which were enabled at card removal time, are re-enabled.
 	The first call to hcf_enable will restore the contents of HREG_INT_EN register taking the
 	HCF_ACT_IN_ON/OFF history in account.
 8:	<HCFL@: The IFB_PortStat bit corresponding with the port that must be re-enabled, must be cleared before
 	calling hcf_enable, because otherwise calling hcf_enable would have no effect. Note that hcf_enable will
 	set that bit again.  																			@HCFL>
 9:	The MSF must call hcf_action with HCF_ACT_CARD_OUT when the MSF detects a card removal (e.g. when the MSF
	is notified by the CAD). As a minimum, the "present" bit in IFB_CardStat must be reset, however since
	the card insertion will clear all other bits, the simplest solution appears to clear all of IFB_CardStat.
	This is O.K. in case of the HCF, but wrong for HCF-light which does not have IFB_PortStat.
	As a result of the resetting of the CARD_STAT_PRESENT bit, no hcf-function except hcf_action with
	HCF_ACT_CARD_IN results in card I/O anymore. However hcf_functions may still perform their other
	activities, e.g. hcf_get_info_mb still supplies a MBIB if one is available.
	As a result of the resetting of the CARD_STAT_INI ((;?990701: out of date))bit, the call to 
	hcf_initialize by hcf_action with HCF_ACT_CARD_IN results in re-initialization of the NIC.
	<HCFL@
10:	No test of IFB_CardStat on CARD_STAT_PRESENT is needed, because this is done by hcf_initialize
11:	after a successful hcf_initialize there is no need to wait for something or other because the cmd_wait in
	hcf_initialize assures that the Hermes is "READY" for the next command. Again note that hcf_initialize protects
	against NIC absence.
	The command is written in line to the Hermes command register rather than using cmd_wait because the
	response time of the Diagnose command is too large. Note that the synchronous Hermes Diagnose command is
	artificially turned into an a-synchronous command at the WCI I/F. The HCF "knows" that a pseude a-synchronous
	command is active based on the low order bits of IFB_CardStat.
12:	tallying of "No inquire space" is done by cmd_wait
	The HCF_ACT_SCAN and HCF_ACT_TALLIES activity are merged by "cleverly" manipulating action so action gets
	the value CFG_SCAN and CFG_TALLIES respectively.
13:	HCF_ACT_TICK_ON/OFF to enable/disable TimerTick interrupt are primarily intended to satisfy the NDIS
	requirement to generate an interrupt on request, a strategy which is believed to serve as a mechanism to
	pull yourselves on your hair out of the swamp.
	It turned out to be convenient as well to debug the ISR in the early stages of creating a driver.
	There is no lower level support routine to remove bits from IFB_IntEnMask, which is resolved in
	HCF_ACT_INT_OFF by clearing this bit explicitly IFB_IntEnMask and let enable_int handle the actual
	update of the Hermes register by adding "zero" to IFB_IntEnMask.
15:	HCF_ACT_802_11/3/3_PURE to set the HCF into either 802.11 frame mode, 802.3 frame mode with encapsulation
	or 802.3 frame mode without encapsulation. This influences where hcf_put_data writes the data into the
	Tx Frame Structure and how hcf_get_data gathers the data from the Rx Frame Structure. It also
	has some impact on the Ethernet-II encapsulation.
//18:	HCF_ACT_ASSERT_OFF/_ON disable or enable respectively the assert mechanism. This feature is not yet
//	crystalized as it seems to compete with the mechanism offered by CFG_REG_MSF_ASSERT
20: do not HCFASSERT( rc, rc ) since rc == HCF_INT_PENDING is no error, all erroneous rc are already
	asserted elsewhere
	@HCFL>
.ENDOC				END DOCUMENTATION


**************************************************************************************************************/
int hcf_action( IFBP ifbp, 					//address of the Interface Block
				hcf_action_cmd action		/*number identifying the type of change
											  <HCFL@
											  <p>o HCF_ACT_INT_ON	enable interrupt generation by WaveLAN NIC
											  <p>o HCF_ACT_INT_OFF	disable interrupt generation by WaveLAN NIC
											  <p>o HCF_ACT_CARD_IN	MSF reported Card insertion
											  <p>o HCF_ACT_CARD_OUT	MSF reported Card removal
											  <p>o HCF_ACT_DIAG		Hermes Diagnose command
											  <p>o HCF_ACT_TALLIES	Hermes Inquire Tallies (F100) command
											  <p>o HCF_ACT_SCAN		Hermes Inquire Scan (F101) command
											  <p>o HCF_ACT_TICK_ON	enable TimerTick interrupt
											  <p>o HCF_ACT_TICK_OFF	disable TimerTick interrupt
											  <p>o HCF_ACT_802_3		activate 802.3 frame mode
											  <p>o HCF_ACT_802_11	activate 802.11 frame mode
											  @HCFL>
											*/
 				) {

int		rc = HCF_SUCCESS;
hcf_32	prot_cnt = ifbp->IFB_TickIni;														//<HCFL>
hcf_16	i, j;																				//<HCFL>
hcf_16	scratch[2];

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_ACTION, action );									//<HCFL@	/* 0 */
//test purpose	HCFASSERT( 0, action )
	HCFASSERT( action == HCF_ACT_INT_OFF || ( ifbp->IFB_AssertTrace & HCF_ASSERT_ACTION) == 0,
			   action/*ifbp->IFB_AssertTrace*/ )
	HCFASSERTLOGENTRY( HCF_ASSERT_ACTION )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )
	HCFASSERT( ( i = HCF_ERR_MAX ) < UIL_ERR_PIF_CONFLICT, HCF_ERR_MAX )					//@HCFL>	/* 1 */
	
	switch (action) {
	  case HCF_ACT_INT_OFF:						// Disable Interrupt generation
		ifbp->IFB_IntOffCnt++;																			/* 2 */
		if ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) {
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_INT_EN, 0 ); 										/* 4 */
			if ( IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT ) & ifbp->IFB_IntEnMask ) {				/* 5 */
				rc = HCF_INT_PENDING;
			}
		}
		break;
		
	case HCF_ACT_INT_FORCE_ON:					// Enable Interrupt generation
		ifbp->IFB_IntOffCnt = 1;																		/* . */
		//Fall through in HCF_ACT_INT_ON

	  case HCF_ACT_INT_ON:						// Enable Interrupt generation
		HCFASSERT( ifbp->IFB_IntOffCnt, ifbp->IFB_IntOffCnt )
		if ( --ifbp->IFB_IntOffCnt == 0 ) {																/* 6 */
			if ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) {
				OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_INT_EN, ifbp->IFB_IntEnMask );
			}
		}
		rc = ifbp->IFB_IntOffCnt;
		break;
		
	  case  HCF_ACT_CARD_IN:					// MSF reported Card insertion							/* 7 */
#if 0   //<HCFL>
		scratch[0] = ifbp->IFB_CardStat & CARD_STAT_ENABLED; //;?looses most bits, especially CARD_STAT_INCOMP_FEATURE
#else	//<HCFL@	
		ifbp->IFB_CardStat = CARD_STAT_PRESENT; //needed to get hcf_initialize to garuanteed to do something
#endif  //@HCFL>
		ifbp->IFB_CardStat |= CARD_STAT_PRESENT;//needed to get hcf_initialize to garuanteed to do something
		rc = hcf_initialize ( ifbp, INI_COMPLETE );
#if 0   //<HCFL>
		if ( scratch[0] ) (void)hcf_enable( ifbp, 0 );
#else   //<HCFL@
		for ( j = 1, i = HCF_PORT_0; i <= HCF_PORT_6; j <<=1, i++ ) {
			if ( ifbp->IFB_PortStat & j ) {
				ifbp->IFB_PortStat ^= j;																/* 8 */
				(void)hcf_enable( ifbp, (hcf_16)i );
			}
		}
#endif  //@HCFL>
		break;
	
	  case 	HCF_ACT_CARD_OUT:  					// MSF reported Card removal							/* 9 */
		ifbp->IFB_CardStat &= ( CARD_STAT_ENABLED | CARD_STAT_AP );
		break;

	  case 	HCF_ACT_FREEZE:  					// freeze the F/W execution                           //<HCFL@	
		ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_FREEZE;	  
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SPARE, 0x0001 );
		while ( prot_cnt && ( rc = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_SPARE ) ) == 0x0001 ) prot_cnt--;
		ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
		break;                                                                                        //@HCFL>	
		
	  case 	HCF_ACT_DIAG:						// Hermes Diagnose command					//<HCFL@	/*10 */
		scratch[1] = 0;
		scratch[0] = (hcf_16)hcf_initialize( ifbp, INI_PARTIAL );		//add to desc: stops if card not present
		if ( scratch[0] == HCF_SUCCESS ) {
			ifbp->IFB_CardStat |= HCMD_DIAG;
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_0, AUX_MAGIC_1 );								/*11 */
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_1, (hcf_16)~AUX_MAGIC_1 );
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_CMD, HCMD_DIAG );
		}
#if !defined HCF_MB_OFF
		else
		{	//;?either HCF_ERR_TIME_OUT, HCF_FAILURE or HCF_ERR_NO_NIC
 			//no need to find another reporting mechanism as long as the results of the "real" test go by MBIB		
			put_info_mb( ifbp, CFG_DIAG, scratch, sizeof(scratch)/sizeof(hcf_16) + 1);
#endif // HCF_MB_OFF
		}
		break;																				//@HCFL>
		
	  case 	HCF_ACT_SCAN:						// Hermes Inquire Scan (F101) command		//<HCFL@	/*12 */
	  case 	HCF_ACT_TALLIES:					// Hermes Inquire Tallies (F100) command				/*12 */
		if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {
		  rc = cmd_wait( ifbp, HCMD_INQUIRE, action == HCF_ACT_TALLIES ? CFG_TALLIES : CFG_SCAN );
		}  		
		break;
		
	  case 	HCF_ACT_TICK_ON:					// enable TimerTick interrupt				//<HCFL@	/*13 */
		enable_int( ifbp, HREG_EV_TICK );
		break;																				//@HCFL>
		
	  case 	HCF_ACT_TICK_OFF:					// disable TimerTick interrupt              //<HCFL@
		ifbp->IFB_IntEnMask &= (hcf_16)~HREG_EV_TICK;
		enable_int( ifbp, 0 );
		break;																				//@HCFL>
		
//<HCFL@
	  case 	HCF_ACT_802_3:						// activate 802.3 frame mode with E-II encapsualtion	/*15 */
	  case 	HCF_ACT_802_3_PURE:					// activate 802.3 frame mode without E-II encapsualtion
		ifbp->IFB_FrameType = action;
		ifbp->IFB_FSBase = HFS_ADDR_DEST_ABS;
//!		ifbp->IFB_RxFence = 0;					 //gets set in first hcf_service_nic
		ifbp->IFB_TxCntl[0] &= ~HFS_TX_CNTL_STRCT_TYPE;
//!		ifbp->IFB_TxCntl[0] |= HFS_TX_CNTL_802_3;
		break;
		
	  case 	HCF_ACT_802_11:						// activate 802.11 frame mode
		ifbp->IFB_FrameType = action;  //;?ENC_802_11;
		ifbp->IFB_FSBase = HFS_ADDR_1_ABS;
		ifbp->IFB_RxFence = HFS_ADDR_DEST_ABS; //;?gets lost in first hcf_service_nic
		ifbp->IFB_TxCntl[0] &= ~HFS_TX_CNTL_STRCT_TYPE;
		ifbp->IFB_TxCntl[0] |= HFS_TX_CNTL_802_11;
		break;
//@HCFL>

	  default:
		HCFASSERT( DO_ASSERT, action )
		break;
	}
	//! do not HCFASSERT( rc == HCF_SUCCESS, rc )														/* 20*/
	HCFASSERTLOGEXIT( HCF_ASSERT_ACTION )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_ACTION, -1 );
	return rc;
}/* hcf_action */


/*<HCFL@*/
/*******************************************************************************************************************

.MODULE			hcf_assert
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			NW4
.APPLICATION	
.DESCRIPTION	filters assert on level and interfaces to the MSF supplied assert routine
.ARGUMENTS
  void hcf_assert( IFBP ifbp, wci_bufp file_name, unsigned int line_number, int q )
.RETURNS
  void

.NARRATIVE

 Parameters:
  ifbp			address of the Interface Block
  file_name		(far) pointer to string representing the file name containing the line which caused the assert
  line_number	line number of the line which caused the assert
  q				qualifier, additional information which may give a clue about the problem

.DIAGRAM
.ENDOC				END DOCUMENTATION

**************************************************************************************************************/
#ifdef HCF_ASSERT
void hcf_assert( IFBP ifbp, struct ASSERT_STRCT* assert_strct, unsigned int line_number, int q ) {

hcf_16	run_time_flag = ifbp->IFB_AssertLvl;

#if defined __BEOS__

	dprintf( "HCF_ASSERT: file: %s line: %04d  T: %04X Q: %04X\n",
			 (wci_bufp)assert_strct->val, line_number, ifbp->IFB_AssertTrace, q );
#endif

//    OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_1, HCF_MAGIC );
	if ( run_time_flag /* > ;?????? */ ) { //prevent recursive behaviour, later to be extended to level filtering
		if ( ifbp->IFB_AssertRtn ) ifbp->IFB_AssertRtn( (wci_bufp)assert_strct->val, line_number, ifbp->IFB_AssertTrace, q );
#if HCF_DEBUG
		if ( ifbp->IFB_IOBase ) {	//;? without this it crashes if hcf_disable at entry ASSERT catches
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_2, line_number );
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_2, ifbp->IFB_AssertTrace );
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_2, q );
		}		
#endif // HCF_DEBUG		

#if !defined HCF_MB_OFF
		ifbp->IFB_AssertLvl = 0;									// prevent recursive behaviour
		assert_strct->trace = ifbp->IFB_AssertTrace;
		assert_strct->qualifier = (hcf_16)q;
		assert_strct->line_number = (hcf_16)line_number;
		put_info_mb( ifbp, CFG_MB_ASSERT, (wci_recordp)&assert_strct->trace, assert_strct->len );
		ifbp->IFB_AssertLvl = run_time_flag;						// restore appropriate filter level
#endif // HCF_MB_OFF
	}
}/* hcf_assert */
#endif // HCF_ASSERT
    /*@HCFL>*/

/*FF***************************************************************************************************************
FUNCT: hcf_connect                                                                               2000/02/04 14:15
CALLS: HCFASSERT                                        hcf_action                                       
DEFIN: HCF_ALIGN                                        HCF_MAGIC                                        
       HCF_MAX_CONFIG                                   HREG_IO_RANGE                                    
       IFB_VERSION                                      INI_TICK_INI                                     
       REV_OFFSET                                       
GLOBL: HCF_ACT_802_3                                    HCF_ACT_INT_OFF                                  
       hcf_rev                                          
PARAM: ifbp                                             ifbp->IFB_CfgTblSize                             
       ifbp->IFB_HCFVersionMajor                        ifbp->IFB_HCFVersionMinor                        
       ifbp->IFB_IOBase                                 ifbp->IFB_IORange                                
       ifbp->IFB_Magic                                  ifbp->IFB_TickIni                                
       ifbp->IFB_Version                                io_base                                          
LOCAL: q                                                
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			hcf_connect
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	Initializes Card and HCF housekeeping

.ARGUMENTS
  void hcf_connect( IFBP ifbp, hcf_io io_base )

.RETURNS
	void

.NARRATIVE

 Parameters:
	ifbp		address of the Interface Block
	io_base		I/O Base address of the NIC


  MSF-accessible fields of Result Block:
	IFB_IOBase				entry parameter io_base
	IFB_IORange				HREG_IO_RANGE (0x40)
	IFB_HCFVersionMajor		the major part of the PVCS maintained version number
	IFB_HCFVersionMinor		the minor part of the PVCS maintained version number
	IFB_Version				version of the IFB layout (0x01 for this release)
	
  Hcf_connect grants access right for the HCF to the IFB and initializes the HCF housekeeping part of the
  IFB. Hcf_connect does not perform any I/O.

  The HCF-Version fields are set dynamically, because I do not know of any C mechanism to have the compiler
  and the version control system (PVCS) cooperate to achieve this at compile time.  The HCFVersions fields are
  constructed by collecting and shifting the low order nibbles of the PVCS controlled ASCII representation.
  Note that the low order nibble of a space (0x20) nicely coincides with the low order nibble of an ASCII '0'
  (0x30). Also note that the code breaks when major or minor number exceeds 99.


.DIAGRAM
 1:	patch_catch is called as early in the flow as the C-entry code allows to help the HCF debugger as much as
	possible.  The philosophy behind patch_catch versus a simple direct usage of the INT_3 macro is explained
	in the description of patch_catch
 2:	The IFB is zero-filled.
 	This presets IFB_CardStat and IFB_TickIni at appropriate values for hcf_initialize.
10: In addition to the MSF readable fields mentioned in the description section, the following HCF specific
	fields are given their actual value:
	  -	a number of fields as side effect of the calls of hcf_action (see item 14)
	  -	IFB_Magic
	IFB_VERSION, which reflects the version of the IFB layout, is defined in HCF.H
12:	JUst to be on the safe side: default IFB_MSFType to 1 in case of an AP. An hermafrodyte is responsible
	to set it "right"
14:	Hcf_connect defaults to "no interrupt generation" (by calling hcf_action with the appropriate parameter),
	"802.3 frame type" and "no card present" (implicitly achieved by the zero-filling of the IFB).
	Depending on HCFL, the 802.3 frame type is either initialized in line or by calling hcf_action.
	
.NOTICE
  If io_base ever needs to be dynamic, it may be more logical to pass
	- io_base at hcf_enable or
	- have a separate hcf_put_config command or
	- demand a hcf_disconnect - hcf_connect sequence
	
.NOTICE
  On platforms where the NULL-pointer is not a bit-pattern of all zeros, the zero-filling of the IFB results
  in an seemingly incorrect initialization of IFB_MBp. The implementation of the MailBox manipulation in
  put_mb_info protects against the absence of a MailBox based on IFB_MBSize, IFB_MBWp and ifbp->IFB_MBRp. This
  has ramifications on the initialization of the MailBox via hcf_put_info with the CFG_REG_MB type.

  //<HCFL@
  It is UNLIKELY that the space allocated for the IFB contains coincidentally  HCF_MAGIC at IFB_Magic, so the
  small chance that HCFASSERT( ifbp->IFB_Magic != HCF_MAGIC ) catches erroneously on an un-initialized memory
  area is considered remote and not to outweigh the benefits of catching calling hcf_connect twice.

  Asserting on re-entrancy of hcf_connect by means of
  "HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_CONNECT) == 0, 0 )" is not useful because IFB contents
  are undefined

  Hcf_debug_trigger, which is used to profile the HCF, is not called at entry and exit of hcf_connect because
  hcf_connect has no interaction with the Hermes, so timing is not very interesting. Also constructions like
  "hcf_debug_trigger( ifbp, HCF_ASSERT_CONNECT, ifbp->IFB_IOBase = io_base );" would be needed because the IFB
  is not yet initialized and we want to prevent code pollution in release builds (where the logic to profile
  the HCF should be removed), so the setting of IFB_IOBase must be integrated into the hcf_debug_trigger call.

  //@HCFL>
.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
void hcf_connect( IFBP ifbp, 					//address of the Interface Block
				  hcf_io io_base				//I/O Base address of the NIC
				) {

hcf_8 *q;

#if defined _M_I86TM
	patch_catch();																			//<HCFL>	/* 1 */
#endif // _M_I86TM
	
	HCFASSERT( ((hcf_16)ifbp & (HCF_ALIGN-1) ) == 0, (hcf_16)ifbp )
																							//<HCFL>   
	HCFASSERT( ifbp->IFB_Magic != HCF_MAGIC, 0 )												/* see NOTICE*/
	HCFASSERT( io_base, 0 )
	HCFASSERT( (io_base & 0x003F) == 0, io_base )

	for ( q = (hcf_8*)(ifbp+1); q > (hcf_8*)ifbp; *--q = 0) /*NOP*/;									/* 2 */

	ifbp->IFB_Version	= IFB_VERSION;					  												/* 10*/
	ifbp->IFB_IOBase	= io_base;
	ifbp->IFB_IORange	= HREG_IO_RANGE;
	ifbp->IFB_Magic		= HCF_MAGIC;
	ifbp->IFB_HCFVersionMajor	= (hcf_8)( (hcf_rev[REV_OFFSET] << 4) | (hcf_rev[REV_OFFSET+1] & 0x0F) );
	ifbp->IFB_HCFVersionMinor	= (hcf_8)( hcf_rev[REV_OFFSET+4] == ' ' ?
								  		   hcf_rev[REV_OFFSET+3] & 0x0F :
								  		   (hcf_rev[REV_OFFSET+3] << 4) | (hcf_rev[REV_OFFSET+4] & 0x0F) );
	ifbp->IFB_TickIni = INI_TICK_INI;							//initialize at best guess before calibration
	ifbp->IFB_CfgTblSize = HCF_MAX_CONFIG;															/*<HCFL>*/
#if defined HCF_AP	&& ! defined HCF_STA															/*<HCFL@*/
	ifbp->IFB_MSFType = 1;																				/* 12*/
#endif // HCF_AP && ! HCF_STA																		/*@HCFL>*/
								  		   
	(void)hcf_action(ifbp, HCF_ACT_INT_OFF );															/* 14*/
#if 1	/*<HCFL@*/
	(void)hcf_action(ifbp, HCF_ACT_802_3 );															/*<HCFL>*/
#else	/*@HCFL>*/
    ifbp->IFB_FSBase = HFS_ADDR_DEST_ABS;
#endif	//<HCFL>
	return;
}/* hcf_connect	*/


    //<HCFL@
/*******************************************************************************************************************


.MODULE			hcf_debug_trigger
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION	Trace Analyzer support

.ARGUMENTS
  int hcf_debug_trigger( IFBP ifbp, int where, int what )

.RETURNS
  0 (FALSE)

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  where	
  what	

  Hcf_debug_trigger writes information to the S/W support Hermes register. These registers are R/W
  registers which do not influence the bahavior of the Hermes. The Host will read back whatever value
  the host has lastly written to it. These registers are made available to have a means of communication
  for two concurrent S/W modules which intend to access the same NIC but are inaware of one another, an
  example of this used to be the WaveLAN-I Netware server drivers. In addition these registers can be
  used to write arbitrary information to be picked up via a StateAnalyzer hooked up to a FAT NIC. This way
  we can follow the flow through the HCF and profile the HCF code.


.DIAGRAM

.ENDOC				END DOCUMENTATION
*/
/* -------------------------------------------------------------------------------------------------------------------*/
#if defined HCF_PROFILING || defined HCF_DEBUG
int hcf_debug_trigger( IFBP ifbp, int where, int what ) {

#if defined __BEOS__

	dprintf( "HCF_DEBUG: where: %04x  what: %04x\n", where, what );
#endif

#if 1	//"normal"trace support
	if ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) {	
		OUT_PORT_WORD(ifbp->IFB_IOBase + HREG_SW_2, where );
		OUT_PORT_WORD(ifbp->IFB_IOBase + HREG_SW_1, what );
	}
#else	// Podalyzer trace support
	if ( what & 0x8000 ) ppdbgClear( what & 0x7F );
	else ppdbgSet( what & 0x7F );
#endif	//trace support

	return 0;			//return 0 to assure catching HCF_ASSERT

} /* hcf_debug_trigger */
#endif // HCF_PROFILING || HCF_DEBUG

//@HCFL>



/*FF***************************************************************************************************************
FUNCT: hcf_disable                                                                               2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                cmd_wait                                         
       hcf_action                                       HCFASSERTLOGEXIT                                 
DEFIN: CARD_STAT_ENABLED                                CARD_STAT_PRESENT                                
       HCF_ASSERT_DISABLE                               HCF_MAGIC                                        
       HCMD_CMD_CODE                                    HCMD_DISABLE                                     
GLOBL: HCF_ACT_INT_OFF                                  HCF_ERR_NO_NIC                                   
       HCF_PORT_0                                       HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_IntOffCnt                              
       ifbp->IFB_Magic                                  ifbp->IFB_PortStat                               
       port                                             
LOCAL: p_bit                                            rc                                               
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			hcf_disable
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION    Disables data transmission and reception
.ARGUMENTS
  int hcf_disable( IFBP ifbp, hcf_16 port )

.RETURNS
	HCF_SUCCESS
	>>cmd_wait
	
.NARRATIVE

  Parameters:
	ifbp		address of the Interface Block
	port		single port to be disabled (range HCF_PORT_0 through HCF_PORT_6)			<HCFL@

  MSF-accessible fields of Result Block:
   	IFB_CardStat  -	reset CARD_STAT_ENABLED bit iff at completion no port enabled anymore

  hcf_disable disables a particular port. If this results in no ports are enabled anymore,
  	NIC Interrupt Generation is disabled												@HCFL>

  Condition Settings:
	Card Interrupts	  - Unchanged
					  -	Disabled (Note that the value of IFB_IntOffCnt is unchanged)
						if last port changes from enabled to disabled					<HCFL>
					    					

.NARRATIVE																				<HCFL@
 1:	This assert checks against re-entrancy. Re-entrancy could be caused by a MSF logic at task-level calling
 	hcf_functions without shielding with HCF_ACT_ON/_OFF. When an interrupt occurs, the ISR could (either
 	directly or indirectly) cause re-entering of the interrrupted HCF-routine.
	Note that there is no embargo on calling hcf_diable for a port which is already disabled, hence no
	"HCFASSERT( ifbp->IFB_PortStat & p_bit, port )" should be added to the code.
 2:	determine the bit-flag to represent the specified port in IFB_PortStat.
	Note that as a side effect of this shift no range check on port is needed. Out of range values result
	in a p-bit value which is garuanteed not to be present in IFB_PortStat, hence hcf_disable returns
	immediately
 7: If the requested port is enabled and if the NIC is present, the Hermes Disable command is executed.
	If CARD_STAT_PRESENT is off, part of hcf_disable must be skipped to prevent I/O because the I/O space
	may no longer owned by the HCF, due to a card swap.
10: Even when the NIC is not present, the status bit in IFB_PortStat corresponding with the port parameter
	must be cleared.
12:	When the last port changes from enabled to disabled, the NIC status as reflected by IFB_CardStat must
	change to disabled and the interrupt generation must be disabled.
	Note that since the effect of interrupt disabling has no net effect on IFB_IntOffCnt, this code may
	be called not only at the transition from enabled to disabled but whenever all ports are disabled
40:	Disable the interrupt generation facility without disturbing the MSF controlled IFB_IntOffCnt by
  	decrementing IFB_IntOffCnt to compensate side effect of ACT_INT_OFF action.
  	Note that this is just a fale-safe programming issue. The WCI already requires that the NIC interrupts
  	are disabled when hcf_disable is called.
	                                                                                     @HCFL>
.NOTICE
 o  hcf_disable may disable the card interrupts, however it does NOT influence IFB_IntOffCnt.
	This way it is symmetrical with hcf_enable, which does NOT enable the card interrupts.	
	
**************************************************************************************************************/
int hcf_disable( IFBP ifbp, hcf_16 port ) {

int		rc;
hcf_16	p_bit;																						//<HCFL>

//<HCFL@
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_DISABLE, port );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_DISABLE) == 0, ifbp->IFB_AssertTrace )				/* 1 */
	HCFASSERTLOGENTRY( HCF_ASSERT_DISABLE )	
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
#if defined HCF_AP
	HCFASSERT( /* HCF_PORT_0 <= port && */ port <= HCF_PORT_6 , port )  //;?this could be refined for hermaphrodyte 
#else	
	HCFASSERT( HCF_PORT_0 == port, port )
#endif //HCF_AP
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )

	rc = HCF_SUCCESS;
	p_bit = (hcf_16)(0x0001 << port);
	
	if ( ifbp->IFB_PortStat & p_bit && ifbp->IFB_CardStat & CARD_STAT_PRESENT ) {						/* 7 */
//@HCFL>
		rc = cmd_wait( ifbp, HCMD_DISABLE | (port << 8 ), 0 );
//<HCFL@
	}		
	ifbp->IFB_PortStat &= (hcf_16)~p_bit;																/* 10*/
	if ( ifbp->IFB_PortStat == 0 ) {
//@HCFL>
		ifbp->IFB_CardStat &= (hcf_16)~CARD_STAT_ENABLED;
		(void)hcf_action( ifbp, HCF_ACT_INT_OFF );														/* 40 */
		ifbp->IFB_IntOffCnt--;
	}   //<HCFL>
	HCFASSERT( rc == HCF_SUCCESS || rc == HCF_ERR_NO_NIC, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_DISABLE )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_DISABLE, -1 );
	return rc;
}/* hcf_disable */



/*FF***************************************************************************************************************
FUNCT: hcf_disconnect                                                                            2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                ini_hermes                                       
DEFIN: HCF_ASSERT_DISCONNECT                            HCF_MAGIC                                        
       HCMD_CMD_CODE                                    INI_COMPLETE                                     
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_Magic                                  
LOCAL: q                                                
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_disconnect
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			NW4
.APPLICATION	Card Connection for WaveLAN based drivers and utilities
.DESCRIPTION
  Disable transmission and reception, release the IFB
.ARGUMENTS
  void hcf_disconnect( IFBP ifbp )
.RETURNS
  void

.NARRATIVE
  Parameters:
	ifbp		address of the Interface Block

  Description:
	Brings the NIC in quiescent state by calling ini_hermes, thus preventing any interrupts in the future.

  MSF-accessible fields of Result Block:
  	IFB_CardStat	cleared

.DIAGRAM
 1:	Ini_hermes() is called to bring NIC in quiescent state. Since the MSF is apparently trying to recover
	from a serious problem or going out of business anyway, there is not much point in passing on the return
	status of ini_hermes()
 hcf_initialize gives a justification to execute the Hermes Initialize command only when really needed.
 	Despite this basic philosophy and although the HCF can determine whether the NIC is initialized based
 	on IFB_CardStat, the minimal set of actions to initialize the Hermes is always done by calling
 	ini_hermes.
 5:	clear all IFB fields
 	The clearing of IFB_Magic causes HCFASSERT to catch on all subsequent hcf-functions except hcf_connect.
 	The clearing of IFB_CardStat prevents I/O on any subsequent hcf_function

.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
void hcf_disconnect( IFBP ifbp ) {

hcf_8 *q;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_DISCONNECT, 0 );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_DISCONNECT) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_DISCONNECT )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )

	(void)ini_hermes( ifbp, INI_COMPLETE );																		/* 1 */

	for ( q = (hcf_8*)(ifbp+1); q > (hcf_8*)ifbp; *--q = 0) /*NOP*/;									/* 5 */

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_DISCONNECT, -1);
	return;
}/* hcf_disconnect */


/*FF***************************************************************************************************************
FUNCT: hcf_enable                                                                                2000/02/04 14:15
USERS: hcf_action                                       
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                cmd_wait                                         
       enable_int                                       HCFASSERTLOGEXIT                                 
DEFIN: CARD_STAT_ENABLED                                CARD_STAT_INCOMP_FEATURE                         
       CARD_STAT_INCOMP_PRI                             CARD_STAT_INCOMP_STA                             
       CARD_STAT_PRESENT                                HCF_ASSERT_ENABLE                                
       HCF_MAGIC                                        HCMD_CMD_CODE                                    
       HCMD_ENABLE                                      HREG_EV_ALLOC                                    
       HREG_EV_INFO                                     HREG_EV_RX                                       
GLOBL: HCF_ERR_INCOMP_FEATURE                           HCF_ERR_INCOMP_PRI                               
       HCF_ERR_INCOMP_STA                               HCF_ERR_NO_NIC                                   
       HCF_PORT_0                                       HCF_PORT_6                                       
       HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_Magic                                  
       ifbp->IFB_PortStat                               port                                             
LOCAL: p_bit                                            rc                                               
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_enable
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION    Enables data transmission and reception
.ARGUMENTS
  int hcf_enable( IFBP ifbp, hcf_16 port )
.RETURNS
	HCF_ERR_NO_NIC	
	HCF_ERR_INCOMP_PRI	
	HCF_ERR_INCOMP_STA	
	HCF_ERR_INCOMP_FEATURE	
	HCF_SUCCESS
	>>cmd_wait


.NARRATIVE
  Parameters:
  	ifbp	address of the Interface Block
	port	single port to be enabled (range HCF_PORT_0 through HCF_PORT_6)			<HCFL>

  Description:

  Condition Settings:
	Card Interrupts: Off if IFB_IntOffCnt > 0; On if IFB_IntOffCnt == 0
					 (Note that the value of IFB_IntOffCnt is unchanged)

	hcf_enable takes successively the following actions:
4/6:determine the bit-flag to represent the specified port in IFB_PortStat.								<HCFL@
 	To make the WCI fail-safe, invalid port numbers are silently rejected and HCF_ERR_NO_NIC is returned
 	as status.
 	If the MSF+HCF consider themselves incompatible with Primary or Station Supplier in the Hermes, 
 	hcf_enable() does not enable the Hermes. As a side effect of this, other hcf-routines, like hcf_put_data()
 	don't need to check for CARD_STAT_INCOMP_PRI etc, but only for CARD_STAT_ENABLED					@HCFL>
 6:	If the requested port is disabled and if the NIC is present, the Hermes Enable command is executed.
	If CARD_STAT_PRESENT is off, the body of hcf_enable must be skipped to prevent I/O because the I/O space
	may no longer owned by the HCF, due to a card swap.
	The IFB_IntEnMask is set to allow Info events, Receive events and Allocate events to generate interrupts
	and effectuated if appropriate based on IFB_IntOffCnt by calling enable_int.
	Note that this is a dummy action on subsequent enables.												<HCFL>
	Note that since the effect of interrupt enabling has no effect on IFB_IntOffCnt, this code may
	be called not only at the transition from disabled to enabled but whenever a port is enabled.
12:	When the port successfully changes from disabled to enabled - including the case when no NIC is
	present - , the NIC status as reflected by IFB_CardStat must change to enabled
	and the status bit in IFB_PortStat corresponding with the port parameter must be set.				<HCFL>

.DIAGRAM

.NOTICE
  When the Hermes enable cmd is given, the static configuration of the Hermes is done.
  <HCFL@: In case of the first port to be enabled, this is the NIC wide as well as the particular port
  configuration. If already one or more ports are enabled, only the static configuration of that particular
  port is done.																							@HCFL>
.ENDOC				END DOCUMENTATION

-------------------------------------------------------------------------------------------------------------*/
int hcf_enable( IFBP ifbp, hcf_16 port ) {

int	rc;
int	p_bit;																					//<HCFL>

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_ENABLE, port );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_ENABLE) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_ENABLE )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
#if defined HCF_AP
	HCFASSERT( /* HCF_PORT_0 <= port && */ port <= HCF_PORT_6 , port )
#else	
	HCFASSERT( HCF_PORT_0 == port, port )
#endif //HCF_AP
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )

	p_bit = 0x0001 << port;																	//<HCFL>	/* 4 */
	HCFASSERT( (ifbp->IFB_PortStat ^ p_bit) & p_bit, port )		//assert on enabling an enabled port
	
	if ( (ifbp->IFB_CardStat & CARD_STAT_PRESENT) == 0
		 ||	port > HCF_PORT_6 																//<HCFL>
	   ) { rc = HCF_ERR_NO_NIC;	}	/* 6 */
	else if ( ifbp->IFB_CardStat & CARD_STAT_INCOMP_PRI ) rc = HCF_ERR_INCOMP_PRI;			//<HCFL>
	else if ( ifbp->IFB_CardStat & CARD_STAT_INCOMP_STA ) rc = HCF_ERR_INCOMP_STA;          //<HCFL>
	else if ( ifbp->IFB_CardStat & CARD_STAT_INCOMP_FEATURE ) rc = HCF_ERR_INCOMP_FEATURE;  //<HCFL>
	else {
		rc = HCF_SUCCESS;
		if ( ( ifbp->IFB_PortStat & p_bit ) == 0 ) {	//if this is the initial enable 	//<HCFL>
			rc = cmd_wait( ifbp, HCMD_ENABLE | ( port << 8 ), 0 );
			if ( rc == HCF_SUCCESS ) enable_int( ifbp, HREG_EV_INFO | HREG_EV_RX | HREG_EV_ALLOC );		/* 8 */
		}																					//<HCFL>
	}
	if ( rc == HCF_SUCCESS || rc == HCF_ERR_NO_NIC ) {
		ifbp->IFB_PortStat |= p_bit;														 //<HCFL>	/* 9 */
		ifbp->IFB_CardStat |= CARD_STAT_ENABLED;
	}
	HCFASSERT( rc == HCF_SUCCESS || rc == HCF_ERR_NO_NIC, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_ENABLE )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_ENABLE, -1 );
	return rc;

}/* hcf_enable */


/*FF***************************************************************************************************************
FUNCT: hcf_get_data                                                                              2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                min                                              
       hcfio_string                                     HCFASSERTLOGEXIT                                 
DEFIN: BAP_1                                            CARD_STAT_ENABLED                                
       CARD_STAT_ENA_PRES                               HCF_ALIGN                                        
       HCF_ASSERT_GET_DATA                              HCF_MAGIC                                        
       HCF_MAX_MSG                                      HCMD_CMD_CODE                                    
       HFS_802_11_GAP                                   HFS_E_II_GAP                                     
       HFS_STAT                                         IO_IN                                            
GLOBL: HCF_ACT_802_11                                   HCF_ERR_NO_NIC                                   
       HCF_SUCCESS                                      
PARAM: bufp                                             ifbp                                             
       ifbp->IFB_AssertTrace                            ifbp->IFB_CardStat                               
       ifbp->IFB_FSBase                                 ifbp->IFB_FrameType                              
       ifbp->IFB_IntOffCnt                              ifbp->IFB_Magic                                  
       ifbp->IFB_RxFID                                  ifbp->IFB_RxFence                                
       len                                              offset                                           
LOCAL: rc                                               tlen                                             
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_get_data
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION
	Obtains received message data parts from NIC RAM
.ARGUMENTS
  int hcf_get_data( IFBP ifbp, int offset, wci_bufp bufp, int len )
  Card Interrupts disabled
.RETURNS
	HCF_SUCCESS     	O.K
	HCF_ERR_NO_NIC		NIC removed during data copying process
	>>hcfio_string

.NARRATIVE
	parameters:
		ifbp		address of the Interface Block
		offset		offset (in bytes) in buffer in NIC RAM to start copy process
		len			length (in bytes) of data to be copied
		bufp		char pointer, address of buffer in PC RAM

When hcf_service_nic reports the availability of data, hcf_get_data can be called to copy that data
from NIC RAM to PC RAM.

Hcf_get_data copies the number of bytes requested by the parameter len from NIC RAM to PC RAM.  If
len is larger than the (remaining) length of the message, undefined data is appended to the message.
	
Hcf_get_data starts the copy process at the offset requested by the parameter offset, e.g offset
HFS_ADDR_DEST will start copying from the Destination Address, the very begin of the 802.3
framemessage.  At the WCI,the offset specification is geared to "normal" data communication, i.e
depending on IFB_FrameType, Offset == 0 corresponds with the start of the 802.3 MAC header or the
802.11 Header, negative offset correspond with the control structures.  For non-contiguos access of
specific fields, the MSF is shielded of this by meamns of macro definitions.  The HCF transforms the
WCI positive and negative offsets in to the positive-only offsets used at the Host I/F.
	
In case of a fragmented PC RAM buffer, it is the responsibility of the MSF, to specify as offset the
cumulative values of the len parameters of the preceeding hcf_get_data calls.  This I/F gives a MSF
the facility to read (part of) a message and then read it again.
	
Contiguous hcf_get_data calls skip part of the NIC RAM in the follwing cases:
  - IFB_FrameType == 802.11. In this case the 802.3 header is skipped
  - Encapsulated frame. If hcf_service_nic() detected that the frame should be decapsulated, the SNAP
    header is skipped
	
The skip machanism is controlled by IFB_RxFence.  In case of 802.11 this field is set statically by
hcf_action( HCF_ACT_802_11 ).  In case of 802.3 this field is set dynamically by hcf_service_nic().
	
.DIAGRAM

 2: Check on CARD_STAT_PRESENT suffices to suppress all I/O in case card is removed.  A
check on CARD_STAT_ENABLED is not sufficient, because these days CARD_STAT_PRESENT and
CARD_STAT_ENABLED are independent.  Note that in the original design the NIC could not be enabled
unless present.  An alternative would be to check for an exact match on CARD_STAT_PRESENT and
CARD_STAT_ENABLED like:
	if ( ( ifbp->IFB_CardStat & (CARD_STAT_PRESENT|CARD_STAT_ENABLED) ) ==
		 (CARD_STAT_PRESENT|CARD_STAT_ENABLED) )
		
 4: seperate negative offsets (to access control structure information) from positive offsets (to
	access data)
 6: determine the 802.3/11 dependent absolute offset to add to the (relative) positive offset
	specified by the MSF
	Note that from here on offset is positive, so it can be cast to hcf_16 in the call of
	hcfio_string without problems

8: check whether a part of the data must be skipped.  In case of 802.11 the 802.3 MAC header must be
skipped (IFB_RxFence set by hcf_cation).  In case of encapsualted Ethernet-II, the SNAP header must
be skipped (IFB_RxFEnce set by hcf_service_nic).

10:	gather the part before the area to be skipped

12: set the absolute index in the NIC RAM frame structure just behond the data which must be
skipped.  Note that if in point 8 not sufficient data is read to reach this point, len == 0, so the
read in the next point does not have any effect

14: gather the data/information in case of:
	- control structure access (negative offset)
	- no data to be skipped (plain 802.3)
	- data after the area to be skipped (802.11 after reading the 802.11 header, E-II after
	  reading the MAC header).  In both of these cases, len may have become zero (hence no "real"
	  read) depending on the value of the parameters offset and len

16: hcf_get_data passes the hcfio_string return status on to its caller.  See hcfio_string for more
details.  may be either HCF_ERR_TIME_OUT, ref

.NOTICE
IFB_FSBase is used at so many places, that the strategy to turn it into a (redundant) entity
by its own is considered advantaguous compared with determining its value at evry place where it is
needed.
	
It could be considered to ACK the Hermes Rx Event as soon as the MSF has read the last byte to
optimize the flow ( a better chance to get new Rx data in the next pass through hcf_service_nic ).
In that case IFB_RxFID and IFB_RxLen should be set to 0 and it may be advisable to test on
IFBR_RxLen ratjer than IFB_RxFID in hcf_service_nic.

.ENDOC				END DOCUMENTATION


-------------------------------------------------------------------------------------------------------------*/
int hcf_get_data( IFBP ifbp, int offset, wci_bufp bufp, int len ) {

int rc = HCF_ERR_NO_NIC;
int	tlen;																								//<HCFL>

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_GET_DATA, len );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_GET_DATA) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_GET_DATA )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ((long)bufp & (HCF_ALIGN-1) ) == 0, (hcf_16)(long)bufp )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_CardStat & CARD_STAT_ENABLED, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_IntOffCnt, ifbp->IFB_IntOffCnt )
	HCFASSERT( ifbp->IFB_RxFID, 0 )
	HCFASSERT( HFS_STAT <= offset && offset < HCF_MAX_MSG, offset )
	HCFASSERT( HFS_STAT <= offset+len && offset+len <= HCF_MAX_MSG, offset )

	if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {                            /* 2 */
		rc = HCF_SUCCESS;
		if ( offset < 0 ) offset -= HFS_STAT;															/* 4 */
		else {
			offset += ifbp->IFB_FSBase;																	/* 6 */
			if ( ifbp->IFB_RxFence ) {       												//<HCFL@	/* 8 */
				if ( ( tlen = ifbp->IFB_RxFence - offset )  > 0 ) {										/*10 */
					tlen = min( len, tlen );
					rc = hcfio_string( ifbp, BAP_1, ifbp->IFB_RxFID, (hcf_16)offset, bufp, 0, tlen, IO_IN );
					len		-= tlen;
					bufp	+= tlen;
					offset	+= tlen;
				}
				offset += ifbp->IFB_FrameType == HCF_ACT_802_11 ? HFS_802_11_GAP : HFS_E_II_GAP;		/*12 */
			}       																		//@HCFL>
		}
		if ( rc == HCF_SUCCESS ) {																		/*14 */
			rc = hcfio_string( ifbp, BAP_1, ifbp->IFB_RxFID, (hcf_16)offset, bufp, 0, len, IO_IN );
		}
	}
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_GET_DATA )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_GET_DATA, -1 );
	return rc;																							/*16 */
}/* hcf_get_data */


/*FF***************************************************************************************************************
FUNCT: hcf_get_info                                                                              2000/02/04 14:15
USERS: hcf_initialize                                   check_comp                                       
       put_info                                         
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                get_info_mb                                      
       min                                              hcf_action                                       
       hcf_aux_cntl                                     OUT_PORT_WORD                                    
       IN_PORT_STRING                                   cmd_wait                                         
       hcfio_string                                     MUL_BY_2                                         
       HCFASSERTLOGEXIT                                 
DEFIN: BAP_1                                            CARD_STAT_PRESENT                                
       CFG_AUX_DATA                                     CFG_AUX_DATA_SUPER_SPECIAL                       
       CFG_DRV_ACT_RANGE_HSI                            CFG_DRV_ACT_RANGE_PRI                            
       CFG_DRV_ACT_RANGE_STA                            CFG_DRV_IDENTITY                                 
       CFG_DRV_SUP_RANGE                                CFG_IFB                                          
       CFG_MB_INFO                                      CFG_PROD_DATA                                    
       CFG_RESTRAINT_INFO                               CFG_RID_CFG_MIN                                  
       CFG_TALLIES                                      CFG_TALLIES_VACATE                               
       DO_ASSERT                                        FAR                                              
       HCF_ASSERT_GET_INFO                              HCF_ASSERT_PUT_INFO                              
       HCF_MAGIC                                        HCF_TOT_TAL_CNT                                  
       HCMD_ACCESS                                      HCMD_CMD_CODE                                    
       HREG_AUX_DATA                                    HREG_AUX_OFFSET                                  
       HREG_AUX_PAGE                                    HREG_CNTL_AUX_DIS_CNTL                           
       HREG_CNTL_AUX_ENA_CNTL                           IO_IN                                            
       NULL                                             PLUG_DATA_OFFSET                                 
GLOBL: HCF_ACT_TALLIES                                  HCF_ERR_LEN                                      
       HCF_ERR_NO_NIC                                   HCF_SUCCESS                                      
       cfg_drv_act_range_hsi                            cfg_drv_act_range_pri                            
       cfg_drv_act_range_sta                            cfg_drv_identity                                 
       cfg_drv_sup_range                                
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_IOBase                                 
       ifbp->IFB_Magic                                  ifbp->IFB_NIC_Tallies                            
       ifbp->IFB_WarningInfo                            ltvp                                             
       ltvp->len                                        ltvp->typ                                        
       ltvp->val                                        
LOCAL: bq                                               cp                                               
       i                                                io_port                                          
       len                                              p                                                
       q                                                rc                                               
       type                                             
******************************************************************************************************************/
/**************************************************************************************************************


.MODULE			hcf_get_info
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION
	Obtains transient and persistent configuration information from the
	Card and from the HCF
.ARGUMENTS
  int hcf_get_info( IFBP ifbp, LTVP ltvp )
  Card Interrupts disabled
.RETURNS
	-- value --			-- parameter --
    HCF_ERR_LEN
    HCF_SUCCESS
    HCF_ERR_NO_NIC		all cases except the "HCF embedded" pseudo RIDs
	>>hcf_aux_cntl		CFG_PROD_DATA, CFG_AUX_DATA
	>>hcfio_string		>= CFG_RID_CFG_MIN

.NARRATIVE
	parameters:
		ifbp	address of the Interface Block
		ltvp	address of LengthTypeValue structure specifying the "what" and the "how much" of the 
				information to be collected from the HCF or from the Hermes

.NOTICE

  "HCF embedded" pseudo RIDs:
    CFG_MB_INFO, CFG_TALLIES_VACATE, CFG_TALLIES, CFG_RESTRAINT_INFO, CFG_IFB,
    CFG_DRV_IDENTITY, CFG_DRV_SUP_RANGE, CFG_DRV_ACT_RANGE_PRI, CFG_DRV_ACT_RANGE_STA,
    CFG_DRV_ACT_RANGE_HSI
    Note the HCF_ERR_LEN is NOT adequately set, when L >= 2 but less than needed


 Remarks: Transfers operation information and transient and persistent
 	configuration information from the Card and from the HCF to the MSF.
	The exact layout of the provided data structure
	depends on the action code. Copying stops if either the complete
	Configuration Information is copied or if the number of bytes indicated
	by len is copied.  Len acts as a safe guard against Configuration
	Information blocks which have different sizes for different Hermes
	versions, e.g. when later versions support more tallies than earlier
	versions. It is a consious decision that unused parts of the PC RAM buffer are not cleared.

 Remarks: The only error against which is protected is the "Read error"
	as result of Card removal. Only the last hcf_io_string need
	to be protected because if the first fails the second will fail
	as well. Checking for cmd_wait errors is supposed superfluous because
	problems in cmd_wait are already caught or will be caught by
	hcf_enable.
	
	CFG_MB_INFO: copy the oldest MailBox Info Block or the "null" block if none available. <HCFL>
	
 3:	tallying of "No inquire space" is done by cmd_wait

 Note:
	the codes for type are "cleverly" chosen to be identical to the RID
	
	//<HCFL@
    The mechanism to HCF_ASSERT on invalid typ-codes in the LTV record is based on the following strategy:
	  - during the pseudo-asynchronous Hermes commands (diagnose, download)	only CFG_MB_INFO is acceptable
	  -	some codes (e.g. CFG_TALLIES) are explicitly handled by the HCF which implies that these codes
		are valid
	  - all other codes in the range 0xFC00 through 0xFFFF are passed to the Hermes.  The Hermes returns an
	  	LTV record with a zero value in the L-field for all Typ-codes it does not recognize. This is
	  	defined and intended behavior, so HCF_ASSERT does not catch on this phenomena.
	  -	all remaining codes are invalid and cause an ASSERT.
    //@HCFL>

12:	see NOTICE	
17:	The return status of cmd_wait and the first hcfio_in_string can be ignored, because when one fails, the
 	other fails via the IFB_DefunctStat mechanism
20:	"HCFASSERT( rc == HCF_SUCCESS, rc )" is not suitable because this will always trigger as side effect of
	the HCFASSERT in hcf_put_info which calls hcf_get_info to figure out whether the RID exists at all.
	
	
.NOTICE	The protection in hcf_get_info() is less restrictive than the protection in hcf_put_info(). This is
	one of those cases that are impossible to decide at forehand what is the beststrategy. The current
	choice implies that an Information RID (e.g. MaxTransmitLifetime, 0xFD4a) can not be changed into
	a Configuration RID in a transparent fashion for the HCF	
	
		
**************************************************************************************************************/
int hcf_get_info(IFBP ifbp, LTVP ltvp ) {

int				rc = HCF_ERR_LEN;
hcf_io			io_port;																							//<HCFL>
hcf_16			i, len;
hcf_16			type;						//don't change type to unsigned cause of "is it a RID" test
hcf_16 			*q;							//source pointer (Tally-part of IFB)
wci_recordp		p = ltvp->val;				//destination word pointer (in LTV record)
wci_bufp		cp = (wci_bufp)ltvp->val;	//destination char pointer (in LTV record)								//<HCFL>
#if defined MSF_COMPONENT_ID
hcf_16 FAR 		*bq = NULL;					//source pointer (Identity or Range records)	;?why bq and not e.g. wq//<HCFL>
#endif // MSF_COMPONENT_ID

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_GET_INFO, ltvp->typ );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_GET_INFO) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_GET_INFO )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ltvp, 0 )
	HCFASSERT( ltvp->len > 1, ltvp->len )
	HCFASSERT( (ifbp->IFB_CardStat & HCMD_CMD_CODE) == 0 || ltvp->typ == CFG_MB_INFO, ifbp->IFB_CardStat )
	
	len = ltvp->len;
	type = ltvp->typ;
	
	if ( len ) {
	
		rc = HCF_SUCCESS;
		switch ( type ) {
#if !defined HCF_MB_OFF
		  case CFG_MB_INFO:											//Get Mail Box Info Block       //<HCFL@
		  	get_info_mb( ifbp, ltvp );
			break;                                                                                  //@HCFL>
#endif // HCF_MB_OFF
			
#if MSF_COMPONENT_ID != COMP_ID_AP1
		  case CFG_TALLIES_VACATE:                                                                   //<HCFL>
		  case CFG_TALLIES:																				/* 3 */
			ltvp->len = len = min( len, (hcf_16)(HCF_TOT_TAL_CNT + HCF_TOT_TAL_CNT + 1) );
			q = (hcf_16*)&ifbp->IFB_NIC_Tallies;
			while ( --len ) {
				*p++ = *q;
				if ( type == CFG_TALLIES_VACATE ) *q = 0;                                           //<HCFL>
				q++;
			}
			(void)hcf_action( ifbp, HCF_ACT_TALLIES );
			break;
#endif //COMP_ID_AP1

		  case CFG_RESTRAINT_INFO:                                                                  //<HCFL@
			ltvp->len = len = min( len, (hcf_16)(sizeof(ifbp->IFB_WarningInfo)/sizeof(hcf_16) + 1) );
		  	if ( len <= sizeof(ifbp->IFB_WarningInfo)/sizeof(hcf_16) ) rc = HCF_ERR_LEN;
		  	q = ifbp->IFB_WarningInfo;
	        while ( --len ) *p++ = *q++;
			break;																					//@HCFL>
			
		  case CFG_IFB:                                                                             //<HCFL@
		  	ltvp->len = len = min( len, (hcf_16)(sizeof(IFB_STRCT)/sizeof(hcf_16) + 1) );
		  	if ( len <= sizeof(IFB_STRCT)/ sizeof(hcf_16) ) rc = HCF_ERR_LEN;
		  	q = (hcf_16*)ifbp;
	        while ( --len ) *p++ = *q++;
			break;                                                                                  //@HCFL>
			
//<HCFL@
#if defined MSF_COMPONENT_ID
//another 60 bytes waisted because the 68K compiler for MAC OS is reported to be unable to initialize pointers
#if MSF_COMPONENT_ID != COMP_ID_LINUX_PD && MSF_COMPONENT_ID !=	COMP_ID_LINUX_LIB		//;?add this to documentation/comment
		  case CFG_DRV_IDENTITY:				//0x0826
			bq = (hcf_16 FAR *)&cfg_drv_identity;
			goto get_info_L50;
#endif // MSF_COMPONENT_ID != COMP_ID_LINUX_PD && MSF_COMPONENT_ID !=	COMP_ID_LINUX_LIB
		  case CFG_DRV_SUP_RANGE:				//0x0827
			bq = (hcf_16 FAR *)&cfg_drv_sup_range;
			goto get_info_L50;
		  case CFG_DRV_ACT_RANGE_PRI:			//0x0828
			bq = (hcf_16 FAR *)&cfg_drv_act_range_pri;
			goto get_info_L50;
		  case CFG_DRV_ACT_RANGE_STA:			//0x0829
			bq = (hcf_16 FAR *)&cfg_drv_act_range_sta;
			goto get_info_L50;
		  case CFG_DRV_ACT_RANGE_HSI:			//0x082A
			bq = (hcf_16 FAR *)&cfg_drv_act_range_hsi;
get_info_L50:
			len = min( len, *bq );
			ltvp->len = len;
			bq += 2;								//skip L and T
			while ( --len ) *p++ = *bq++;
			break;
#endif // MSF_COMPONENT_ID
//@HCFL>			
			
		  default:
			if ( ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) == 0 ) {
				rc = HCF_ERR_NO_NIC;
				break;
			}
//<HCFL@				
			switch ( type ) {
#if !defined HCF_DOWNLOAD_OFF
			  case CFG_PROD_DATA:
				//don't use *(long)&ltvp->value[0] = PLUG_DATA_OFFSET to prevent endianess problems
				ltvp->val[0] = (hcf_16)(PLUG_DATA_OFFSET & 0x7E);
				ltvp->val[1] = PLUG_DATA_OFFSET >> 7;
				/* Fall through in case CFG_AUX_DATA */
			  case CFG_AUX_DATA:
				rc = hcf_aux_cntl( ifbp, HREG_CNTL_AUX_ENA_CNTL );
			  case CFG_AUX_DATA_SUPER_SPECIAL:                                   					 //<HCFL>
				HCFASSERT( len > 4, len )
				if ( rc == HCF_SUCCESS ) {
					OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_PAGE, ltvp->val[1] );
					OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_OFFSET, ltvp->val[0] );
					cp = (wci_bufp)ltvp->val;
					--len;								//predecrement compensates for space occupied by T
					io_port = ifbp->IFB_IOBase + HREG_AUX_DATA;
					IN_PORT_STRING( io_port, cp, len );
					if ( type != CFG_AUX_DATA_SUPER_SPECIAL )	{		                             //<HCFL>
						rc = hcf_aux_cntl( ifbp, HREG_CNTL_AUX_DIS_CNTL );
					}                                            		                             //<HCFL>
				}
				break;
#endif //!defined HCF_DOWNLOAD_OFF
					
			  default:											//all "unknown" ones are passed to Hermes
//@HCFL>
				if ( type < CFG_RID_CFG_MIN ) {															/* 12*/
					HCFASSERT( DO_ASSERT, type )
					ltvp->len = 0;
				} else {
					rc = cmd_wait( ifbp, HCMD_ACCESS, type );											/* 17*/
					if ( rc == HCF_SUCCESS ) 
						rc = hcfio_string( ifbp, BAP_1, type, 0, (wci_bufp)&i, 1, sizeof(hcf_16), IO_IN );
					ltvp->len = min( i, len );
					if ( rc == HCF_SUCCESS ) 
						rc = hcfio_string( ifbp, BAP_1, type, sizeof(hcf_16), (wci_bufp)&ltvp->typ, 
										   1, MUL_BY_2(ltvp->len), IO_IN );
					if ( rc == HCF_SUCCESS && i > len ) rc = HCF_ERR_LEN;
				}
			}
		}
	}
	HCFASSERT( rc == HCF_SUCCESS || 
			   ( rc == HCF_ERR_LEN && ifbp->IFB_AssertTrace & HCF_ASSERT_PUT_INFO ), rc )					/* 20*/
	HCFASSERTLOGEXIT( HCF_ASSERT_GET_INFO )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_GET_INFO, -1 );
	return rc;

}/* hcf_get_info */



/*FF***************************************************************************************************************
FUNCT: hcf_initialize                                                                            2000/02/04 14:15
USERS: hcf_action                                       download                                         
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERTLOGENTRY                                
       HCFASSERT                                        ini_hermes                                       
       calibrate                                        check_comp                                       
       hcf_get_info                                     alloc                                            
       HCFLOG                                           put_info                                         
       HCFASSERTLOGEXIT                                 
DEFIN: CARD_STAT_AP                                     CARD_STAT_DEFUNCT                                
       CARD_STAT_INCOMP_PRI                             CARD_STAT_INCOMP_STA                             
       CARD_STAT_PRESENT                                CFG_DL_BUF                                       
       CFG_HSI_SUP_RANGE                                CFG_PRI_SUP_RANGE                                
       CFG_STA_SUP_RANGE                                HCF_ASSERT_INITIALIZE                            
       HCF_LOG_DEFUNCT                                  HCF_MAGIC                                        
       HCMD_CMD_CODE                                    HFS_TX_ALLOC_SIZE                                
       INI_PARTIAL                                      
GLOBL: HCF_ERR_DEFUNCT_ALLOC                            HCF_ERR_INCOMP_PRI                               
       HCF_ERR_INCOMP_STA                               HCF_ERR_NO_NIC                                   
       HCF_SUCCESS                                      cfg_drv_act_range_hsi                            
       cfg_drv_act_range_pri                            cfg_drv_act_range_sta                            
PARAM: cntl                                             ifbp                                             
       ifbp->IFB_CardStat                               ifbp->IFB_CfgTbl                                 
       ifbp->IFB_CfgTblSize                             ifbp->IFB_DLTarget                               
       ifbp->IFB_DUIFRscInd                             ifbp->IFB_DUIF_FID                               
       ifbp->IFB_DefunctStat                            ifbp->IFB_HSICfg                                 
       ifbp->IFB_Magic                                  ifbp->IFB_PIFLoadIdx                             
       ifbp->IFB_PIFRscInd                              ifbp->IFB_PIF_FID                                
       ifbp->IFB_PRICfg                                 ifbp->IFB_STACfg                                 
LOCAL: p                                                q                                                
       rc                                               
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			hcf_initialize
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION    ..........., in addition, a light-weight diagnostic test of the NIC
.ARGUMENTS
  int hcf_initialize( IFBP ifbp, int cntl )

.RETURNS
	>>ini_hermes
	>>calibrate
	HCF_ERR_INCOMP_PRI
	>>hcf_get_info
	HCF_ERR_INCOMP_STA
	HCF_ERR_DEFUNCT_ALLOC
	>>put_info
   	

.NARRATIVE

  Parameters:
	ifbp		address of the Interface Block
	cntl		INI_PARTIAL, minimal initilization as needed for download and before diagnose
				INI_COMPLETE, complete initialization
				
  Condition Settings:
	Card Interrupts: Disabled as side effect of ini_hermes() (Note that the value of IFB_IntOffCnt is unchanged)

  In fact hcf_initialize is an hcf-support routine, given an hcf_... name just in case we want to
  export it over the WCI later
  
  hcf_initialize will successively:
  -	initialize the NIC by calling ini_hermes
  -	calibrate the S/W protection timer against the Hermes Timer by calling calibrate
  - check HSI/Primary compatibility with the HCF
  - collect the parameters to be used in the non-volatile download process
  - check Station compatibility with the HCF
  - pre-allocate 3 buffers in NIC RAM
  - re-configure the Hermes based on IFB_CfgTbl


.NARRATIVE
 2:	Clear all fields of the IFB except those which need to survive hcf_initialize. This is intended to make
 	the behavior and - as a consequence - the debugging of the HCF more reproduceable.
	Especially IFB_DefunctStat must be cleared (new round, new chances)
 4: If the card is not present, return to the caller with appropriate error status. 
 6:	Initialize the Hermes (to the extend possible via the Host I/F) by calling ini_hermes(). The interrupt 
 	generation facility is disabled as side effect of ini_hermes(). If this initialization fails, return 
 	to the caller with appropriate error status.
 	Note that ini_hermes checks for Card Presence, so hcf_initialize does not need to do this.
 8: drop all error status bits in IFB_CardStat since they are expected to be re-evaluated.
10:	Calibrate the S/W time-out protection mechanism by calling calibrate().If this calibration fails, return 
 	to the caller with appropriate error status. 
14:	check_comp() is called to check the Supplier Range of the Host-S/W I/F (HSI) and the Primary Firmware 
	and checked against the Top and Bottom level supported by this HCF.
	If either of these tests fails, the CARD_STAT_INCOMP_PRI bit of IFB_CardStat is set and control returns
 	to the caller with appropriate error status. 
28: <HCFL@ The available CONCATENATED storage starting at IFB_DLTarget is used to create an LTV-record
	to retrieve the non-volatile download characteristics Download buffer location (IFB_DLPage, IFB_DLOffset)
	and size IFB_DLLen. This information is used in the non-volatile RAM download.
	In case of a Primary Incompatibility, the IFB_DLTarget LTV is not retrieved, causing IFB_DLLen to stay
	zero, implicitly making NV-RAM download impossible.
	Incompatible Station firmware - as opposed to Incompatible Primary Firmware - does not preclude NV-RAM
	download to allow upgrade of NIC with broken or incompatible station firmware. For this reason, the
	Statin F/W Compatibility test must follow download info retrieval. If this retrieval fails, return 
 	to the caller with appropriate error status.													@HCFL>
30: The necessary initialization of the NIC as well as the necessary information gathering for download
	is now done, so skip all station and configuration stuff if we are preparing for download
32: In case of a HCF compiled for station functionality, the Supplier Range of the Station Firmware function
	is retrieved from the Hermes and checked against the Top and Bottom level supported by this HCF.
	Note that the Firmware can have multiple Variants, but that the HCF currently only supports a single
	variant.
34:	pre-allocate 3 buffers in NIC RAM
	 - Tx Frame Structure for the protocol stack
	 - Tx Frame Structure for the utility (;? could be restricted to STA)
	   Note that this subsequent allocate is only performed if the preceding one succeeded
	   If both allocates succeeded, the corresponding Resource Indicators are set
 		The "ASSERT( ifbp->IFB_PIFLoadIdx = 0xFFFF )" does never catches, but initializes IFB_PIFLoadIdx 
 		to be tested by an ASSERT in hcf_put_data if and only the ASSERT mechanism is enabled.
	 - Information Frame Structure for the Notify command (AP only)
	   If the allocate succeeded, the corresponding Resource Indicators is set
	   After two successive allocates, it is assumed that the third is "garuanteed" to succeed, so no bother 
	   to pass the return status on to the caller. Note that even if the third allocate() fails, no 
	   catastrophes will occur because IFB_NotifyRscInd stays zero.
40:	re-configure the Hermes based on IFB_CfgTbl
	
.NOTICE
 o  hcf_initialize disables the card interrupts as side effect of ini_hermes(), however it does NOT influence 
 	IFB_IntOffCnt. This way it is symmetrical with hcf_enable, which does NOT enable the card interrupts.	

**************************************************************************************************************/
int hcf_initialize( IFBP ifbp, int cntl ) {

int		rc;
hcf_16	*p;																								//<HCFL>
hcf_8	*q;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_INITIALIZE, 4 );
	HCFASSERTLOGENTRY( HCF_ASSERT_INITIALIZE )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )

	for ( q = (hcf_8*)&ifbp->IFB_PIFRscInd; q < (hcf_8*)&ifbp->IFB_CfgTblSize; *q++ = 0) /*NOP*/;		/* 2 */
//!	if ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) 														/* 4 */
	do { 	//pseudo goto-less, accept "warning: conditional expression is constant"
//		if ( (rc = ini_hermes( ifbp, cntl ) ) != HCF_SUCCESS ) break;									/* 6 */
		rc = ini_hermes( ifbp, cntl );									/* 6 */
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_INITIALIZE, rc );
		if ( rc  != HCF_SUCCESS ) break;									/* 6 */
		ifbp->IFB_CardStat &= ( CARD_STAT_PRESENT | CARD_STAT_AP );										/* 8 */
		if ( (rc = calibrate( ifbp ) ) != HCF_SUCCESS ) break;   										/* 10*/
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_INITIALIZE, rc );
#if defined MSF_COMPONENT_ID 	//;?interesting question at which level HCFL interacts		//<HCFL@	/* 14*/
		ifbp->IFB_HSICfg = check_comp( ifbp, (CFG_RANGES_STRCT*)&cfg_drv_act_range_hsi, CFG_HSI_SUP_RANGE );
		// cast of cfg_drv_act_range_hsi is needed to get around Microsoft compiler problem in DOS ODI
		ifbp->IFB_PRICfg = check_comp( ifbp, (CFG_RANGES_STRCT*)&cfg_drv_act_range_pri, CFG_PRI_SUP_RANGE );
		if ( ifbp->IFB_PRICfg == 0 || ifbp->IFB_HSICfg == 0 ) {
			ifbp->IFB_CardStat |= CARD_STAT_INCOMP_PRI;
			rc = HCF_ERR_INCOMP_PRI;
			break;
		}		
#endif // MSF_COMPONENT_ID
#if defined HCF_STA
		ifbp->IFB_DLTarget[0] = 4;																		/* 28*/
		ifbp->IFB_DLTarget[1] = CFG_DL_BUF;
		rc =  hcf_get_info( ifbp, (LTV_STRCT*)&ifbp->IFB_DLTarget[0] );
#if defined HCF_BIG_ENDIAN
		ifbp->IFB_DLPage	= CNV_LITTLE_TO_INT( ifbp->IFB_DLPage );
		ifbp->IFB_DLOffset	= CNV_LITTLE_TO_INT( ifbp->IFB_DLOffset  );
		ifbp->IFB_DLLen		= CNV_LITTLE_TO_INT( ifbp->IFB_DLLen );
#endif // HCF_BIG_ENDIAN				
		if ( rc != HCF_SUCCESS ) break;
#endif //HCF_STA
		if ( cntl == INI_PARTIAL ) break;																/* 30*/
#if defined MSF_COMPONENT_ID && defined HCF_STA
		ifbp->IFB_STACfg = check_comp( ifbp, (CFG_RANGES_STRCT*)&cfg_drv_act_range_sta, CFG_STA_SUP_RANGE );
		// cast of cfg_drv_act_range_sta is needed to get around Microsoft compiler problem in DOS ODI
#if defined HCF_AP
		if ( ifbp->IFB_MSFType == 0 ) {				//escape in case of AP on hermaphrodite AP/STA
#endif //HCF_AP
			if ( ifbp->IFB_STACfg == 0 ) {
				ifbp->IFB_CardStat |= CARD_STAT_INCOMP_STA;
				rc = HCF_ERR_INCOMP_STA;
				break;																	    			/* 32*/
			}
#if defined HCF_AP
		}
#endif //HCF_AP
				
#endif //MSF_COMPONENT_ID && HCF_STA
																										//@HCFL>
		if (    ( ifbp->IFB_PIF_FID  = alloc( ifbp, HFS_TX_ALLOC_SIZE ) ) == 0  						/* 34*/
			 || ( ifbp->IFB_DUIF_FID = alloc( ifbp, HFS_TX_ALLOC_SIZE ) ) == 0       	 				//<HCFL>
			)  {
			rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_ALLOC;
			HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
			ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
		} else {
			ifbp->IFB_PIFRscInd = ifbp->IFB_DUIFRscInd = 1;
			HCFASSERT( ifbp->IFB_PIFLoadIdx |= 0xFFFF, 0 )
#if defined HCF_AP																						//<HCFL@
			if ( ( ifbp->IFB_Notify_FID = alloc( ifbp, HCF_MAX_NOTIFY + 4 ) ) != 0 )
				ifbp->IFB_NotifyRscInd = 1;
#endif //HCF_AP																							//@HCFL>
	
#if HCF_MAX_CONFIG != 0  																				//<HCFL@
			p = ifbp->IFB_CfgTbl;																		/* 40*/
			while ( *p && ( rc = put_info( ifbp, (LTVP)p ) ) == HCF_SUCCESS  ) p += *p + 1;
#endif //HCF_MAX_CONFIG																					//@HCFL>
		}
	} while ( 0 ); //pseudo goto-less, accept "warning: conditional expression is constant"
	HCFASSERT( rc == HCF_SUCCESS || rc == HCF_ERR_NO_NIC, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_INITIALIZE )	
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_INITIALIZE, 0x8004 );
	return rc;
}/* hcf_initialize */


/*FF***************************************************************************************************************
FUNCT: hcf_put_data                                                                              2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                hcfio_string                                     
       min                                              enc_test                                         
       HCFASSERTLOGEXIT                                 
DEFIN: BAP_0                                            CARD_STAT_ENABLED                                
       CARD_STAT_ENA_PRES                               ENC_TX_1042                                      
       ENC_TX_E_II                                      ENC_TX_TUNNEL                                    
       HCF_ALIGN                                        HCF_ASSERT_PUT_DATA                              
       HCF_MAGIC                                        HCF_MAX_MSG                                      
       HCMD_CMD_CODE                                    HFS_DAT_ABS                                      
       HFS_DAT_LEN_ABS                                  HFS_LEN_ABS                                      
       HFS_TX_ALLOC_SIZE                                HFS_TX_CNTL_ABS                                  
       HFS_TYPE_ABS                                     IO_OUT                                           
       KLUDGE_MARGIN                                    
GLOBL: HCF_ACT_802_11                                   HCF_ACT_802_3                                    
       HCF_PORT_6                                       HCF_PUT_DATA_RESET                               
       enc_trans_tbl                                    snap_header                                      
PARAM: bufp                                             ifbp                                             
       ifbp->IFB_AssertTrace                            ifbp->IFB_CardStat                               
       ifbp->IFB_FSBase                                 ifbp->IFB_FrameType                              
       ifbp->IFB_IntOffCnt                              ifbp->IFB_Magic                                  
       ifbp->IFB_PIFLoadIdx                             ifbp->IFB_PIFRscInd                              
       ifbp->IFB_PIF_FID                                ifbp->IFB_PortStat                               
       ifbp->IFB_TxCntl                                 ifbp->IFB_TxFrameType                            
       len                                              port                                             
LOCAL: idx                                              tlen                                             
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_put_data
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.ARGUMENTS
  void hcf_put_data( IFBP ifbp, wci_bufp bufp, int len, hcf_16 port )
  Card Interrupts disabled

.RETURNS
  void

.DESCRIPTION	Transfers (part of) transmit message to the NIC and handles
	the Ethernet-II encapsulation if applicable
.NARRATIVE
	parameters:
		ifbp		address of the Interface Block
		bufp		char pointer, address of buffer in PC RAM
		len			length (in bytes) of data to be copied
		port		HCF_PORT_0 - HCF_PORT_6 .........;?
					HCF_PUT_DATA_RESET


	Refer to hcf_service;?non-existing reference, for a concise description about possible
	relation/sequencing of hcf_put_data in the Interrupt Service Routine

	Hcf_put_data() does not need a return value to report unexpected NIC removal, because the MSF and higher 
	layers must be able to cope anyway with the NIC being removed after a successful completion of 
	hcf_put_data() + hcf_send() but before the actual transmission took place. If hcfio_string() fails, 
	this will result in a failing hcf_send, so it will be reported to the MSF with some delay.
	
	In essence, hcf_put_data copies the number of bytes specified by parameter len from the location in PC
	RAM specified by bufp to the NIC RAM buffer associated with the Protocol Stack dedicated FID.
	The first call succeeding hcf_send (or hcf_enable), writes the first byte at offset HFS_ADDR_DEST in
	the transmit data buffer, successive hcf_put_data calls continue where the preceeding hcf_put_data stopped.
	
	IFB_FrameType determines whether the message in the PC RAM buffer is interpreted as an 802.3 or 802.11
	frame.  This influences:
 	o the position where the first byte of the initial hcf_put_data is stored
 	o Only in case of the 802.3 frame type, hcf_put_data checks whether the frame is an Ethernet-II rather
 	  than an "official" 802.3 frame. The E-II check is based on the length/type field in the MAC header. If
 	  this field has a value larger than 1500, E-II is assumed. The implementation of this test fails if the
	  length/type field is split over 2 hcf_put_data calls.
	  If E-II is recognized, the length field HFS_LEN_ABS is skipped for the time being and a SNAP header is
	  inserted starting at HFS_DAT_ABS. This SNAP header represents either RFC1042 or Bridge-Tunnel
	  encapsulation, depending on whether the type is absent or present in enc_trans_tbl.
 	o In case of the 802.11 frame type, hcf_put_data checks whether the complete header + length field is
 	  written (note that part of the header may be written by previous hcf_put_data calls and part may be
 	  written by this call).  If so, the next byte is written at HFS_DAT_ABS (the 802.3 header area is skipped)

	It is allowed to write the 802.3 header, 802.11 header and/or data in fragments, e.g. the first
	hcf_put_data call writes 18 bytes starting at location HFS_ADDR_1_ABS and the second call writes 6 more
	bytes starting at location HFS_ADDR_4. Although Address part #4 is not present in some 802.11 headers,
	all 4 addressing parts and the length field must be written in case of 802.11. Once the complete header
	is written, the data part is written starting from offset HFS_DAT_ABS.

	Hcf_put_data does not check for transmit buffer overflow because the Hermes does this protection.
	In case of a transmit buffer overflow, the surplus which does not fit in the buffer is simply dropped.
	Note that this possibly results in the transmission of incomplete frames.

.DIAGRAM
1*: If the card is not present, prevent all I/O because "our" I/O base may have been given away to someone
	else in a CS/SS environment.  Also no I/O should be performed if the NIC is not enabled. However
	an MSF which calls hcf_put_data while the NIC is not enabled probably contains a logic flaw (or has a
	strategy which was not foreseen at the time this was written)
10* HCF_PUT_DATA_RESET discards all the data put by preceeding hcf_put_data calls and resets the HCF
	housekeeping just the same as after an hcf_send triggered allocate event.
	Note: To make the WCI fail-safe, invalid port numbers are silently rejected by treating them as
	HCF_PUT_DATA_RESET. Note that the assumption is that this should never ever occure in a debugged MSF and
	that during debugging the ASSERT is sufficient support to help the MSF programmer.
2*:	This statement is only true at the first hcf_put_data call after an hcf_send result or hcf_enable
	The housekeeping is done.
 	o the PIFRscInd is cleared, so the MSF can not begin another hcf_put_data/hcf_send sequence
	before completing the current one
 	o the Tx Encoding flag (TxFrameType) is cleared
 	o the index to the first free position in the FID (IFB_PIFLoadIdx) is initialized based on IFB_FSBase.
 	  IFB_FSBase is initialized when hcf_action is called with HCF_ACT_802_3 or HCF_ACT_802_11
3*:	Pay Attention: it may seem attractive to change this code, e.g. to save the superfluous call to
	hcfio_out_string when the Destination and Source address are written by the preceeding call and the
	current call starts at the length/type field. However this code is "reasonably carefully" crafted
	to take in account all boundary conditions. It is very easy to make a change which does not work under
	all feasible split ups of the message in fragments.
	First IFB_PIFLoadIdx is checked.
	  - If IFB_PIFLoadIdx points past HFS_LEN_ABS, the preceeding call(s) to hcf_put_data already passed the
	  length/type field. As a consequence the fragment can be concatenated to the data already copied to
	  NIC RAM.
	  - If IFB_PIFLoadIdx does not point past HFS_LEN_ABS, the current fragment may or may not contain part of
	  the Destination and/or Source Address and it may or may not contain the length/type field.
	  If the fragment contains addressing information or -in case of 802.11- length info , this information
	  is copied/concatenated to the NIC RAM buffer. The working variables (pointer and length of fragment) as
	  well as the IFB_PIFLoadIdx are adjusted.
	The semi-obscure differences in the boundary testing are caused by:
	  o 802.11: the "below the boundary" area is Addr1, Addr2, Addr3, Ctrl, Adrr4 + DataLen and the "above"
	  	area is the "real" data
	  o 802.3: the "below the boundary" area is DestAddr + SrcAddr and the "above" area is the length +
	  	"real" data
	  o E-II: the "below the boundary" area is DestAddr + SrcAddr, then there is a "virtual" area with the
	  	SNAP header (which will in the end include the HCF calculated length)  and the "above" area is the
	  	"protocol stack length" (is in reality the type code) + "real" data
4*:	If there is still data left, IFB_PIFLoadIdx may need adjustment (802.11 and E-II encapsulation).  Again
	note that this length check is crucial to prevent mis-manipulation of IFB_PIFLoadIdx in case the header
	is written in multiple fragments.
	In case of 802.3, the E-II check is done. In case of E-II, the encapsulation type (RFC1042 versus
	Bridge-Tunnel) is determined and the corresponding SNAP header is written to NIC RAM and
	IFB_PIFLoadIdx is adjusted.
6*:	All data which is not already copied under 3*, is copied here.
	In case of 802.11, the HFS_DAT field is the first field written by this code.
	In case of 802.3, the HFS_LEN field is the first field written by this code.
	In case of E-II encapsulation, the HFS_TYPE field is the first field written by this code.
	Note that in case of E-II encapsulation, the HFS_LEN field is not written by hcf_put_data at all, but by
	hcf_send because the data length is not	known till all fragments have been processed.
	
.NOTE	
	The possible split of a single hcf_put_data call into 2 calls to hcfio_out_string results in 2 calls
	to bap_ini, which may be unexpected while you are debugging, but is never the less the intended behavior.
	Along the same line a call of hcfio_out_string with a value of 0 for parameter len may be unexpected, e.g.
	when the len parameter of hcf_put_data is either 1 or 2, the latter depending on the odd/even aspects of
	IFB_PIFLoadIdx.	
	
	The HCFASSERT on the combination of IFB_PIFLoadIdx and IFB_PIFRscInd is prepared by the (never catching)
	HCFASSERT in hcf_send
	
	
.NOTE
	The test on PIFRscInd to distinguish the initial hcf_put_data from subsequent calls is not thread safe.
	It is assumed that threaded MSFs have their own mechanism to assure that hcf_put_data calls belonging to
	a single frame are atomic with respect to each other. It is also assumed that the MSF takes care that
	the hcf_put_data calls of multiple frames do not run concurrent
	
	
.ENDOC				END DOCUMENTATION


-------------------------------------------------------------------------------------------------------------*/
void hcf_put_data( IFBP ifbp, wci_bufp bufp, int len, hcf_16 port ) {

hcf_16	idx;				//working index into Tx Frame structure, presume MSF control
int		tlen;				//working type/length of frame, working length for partial copy of frame	/<HCFL>	

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_DATA, len );                                    		//<HCFL@
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_PUT_DATA) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_PUT_DATA )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ((long)bufp & (HCF_ALIGN-1) ) == 0, (hcf_16)(long)bufp )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_CardStat & CARD_STAT_ENABLED, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_PIF_FID, 0 )
	HCFASSERT( ( ifbp->IFB_PIFLoadIdx == 0xFFFF && ifbp->IFB_PIFRscInd != 0 )	||
				 ( ifbp->IFB_PIFLoadIdx != 0xFFFF && ifbp->IFB_PIFRscInd == 0 )		, ifbp->IFB_PIFLoadIdx )
	HCFASSERT( ifbp->IFB_PIFRscInd != 0 ||
			   ( ifbp->IFB_PIFRscInd == 0 && ifbp->IFB_TxCntl[1] == port) , port )
	HCFASSERT( port <= HCF_PORT_6, port )
	HCFASSERT( ifbp->IFB_PortStat & (0x0001 << port), port )				//assert on using an enabled port
	HCFASSERT( ifbp->IFB_IntOffCnt, 0 )
	HCFASSERT( bufp || len == 0, len )	//;?not the most logical choice perhaps, but one hour before vacation the most safe
	HCFASSERT( 0 <= len && len <= HCF_MAX_MSG, len )                                        //@HCFL>

	if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {								/* 1 */
		if ( port >= HCF_PUT_DATA_RESET	) ifbp->IFB_PIFRscInd = 1;                             /*<HCFL>*/	/* 10*/
		else {                                                                                 /*<HCFL>*/
			if ( ifbp->IFB_PIFRscInd ) {																	/* 2 */
				ifbp->IFB_PIFRscInd = 0;
				ifbp->IFB_TxFrameType = 0;  //<HCFL>
				ifbp->IFB_PIFLoadIdx = ifbp->IFB_FSBase;    //;?<HCF_L> should result in 0
//* the next statements potentially intefere with TxErr logging and potentially with new allocate strategy
//* as a consequence leave them out till it is proven that the performance impact is relevant
//*				if ( ifbp->IFB_TxCntl[1] != port ) {    /*<HCFL@*/
//*					ifbp->IFB_TxCntl[1] = (hcf_8)port;
//*/
					(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, HFS_TX_CNTL_ABS,	//justify the "void"
									  (wci_bufp)ifbp->IFB_TxCntl, 0, 2, IO_OUT );							/* 7 */
// see comment above				}   /*@HCFL>*/
			}
			HCFASSERT( ifbp->IFB_PIFLoadIdx + len <= HFS_TX_ALLOC_SIZE	- KLUDGE_MARGIN, 0 )
			
			if ( ifbp->IFB_PIFLoadIdx <= HFS_LEN_ABS ) {										/*<HCFL@*/	/* 3*/
				if ( ifbp->IFB_FrameType == HCF_ACT_802_11 ) {
					tlen = min( len, HFS_DAT_LEN_ABS + 2 - (int)(ifbp->IFB_PIFLoadIdx) );
				} else {
					tlen = min( len, HFS_LEN_ABS - (int)(ifbp->IFB_PIFLoadIdx) );
				}
				(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, ifbp->IFB_PIFLoadIdx, bufp, 0,tlen,IO_OUT );
				ifbp->IFB_PIFLoadIdx += (hcf_16)tlen;
				bufp += tlen;
				len = len - tlen;
				if (  len > 0 ) {																			/* 4 */
					if ( ifbp->IFB_FrameType == HCF_ACT_802_11 ) {
						ifbp->IFB_PIFLoadIdx = HFS_DAT_ABS;
					} else {
						if ( ifbp->IFB_FrameType == HCF_ACT_802_3 && 
						( tlen = enc_test( (wci_recordp)bufp ) ) != 0 ) {
							ifbp->IFB_TxFrameType = ENC_TX_E_II;
							snap_header[sizeof(snap_header) - 1] = ENC_TX_1042;
							if ( tlen <= sizeof(enc_trans_tbl)/sizeof(enc_trans_tbl[0]) )
								snap_header[sizeof(snap_header) - 1] = ENC_TX_TUNNEL;
							/* it can be deducted that IFB_PIFLoadIdx == HFS_DAT_ABS at this point, so that would
							 * be an alternative to use in the next hcfio_string()								*/
							(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, ifbp->IFB_PIFLoadIdx,
												snap_header, 0, sizeof(snap_header), IO_OUT );
							ifbp->IFB_PIFLoadIdx = HFS_TYPE_ABS;
						}
					}
				}
			}                                                                                          	/*@HCFL>*/
			idx = ifbp->IFB_PIFLoadIdx;
			ifbp->IFB_PIFLoadIdx += (hcf_16)len;
			(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, idx, bufp, 0, len, IO_OUT);		/* 6 */
		}																								/*<HCFL>*/
	}
	HCFASSERTLOGEXIT( HCF_ASSERT_PUT_DATA )	
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_DATA, -1 );
	return;
}/* hcf_put_data */


//<HCFL@
#if defined HCF_CELLWAVE
/*******************************************************************************************************************


.MODULE			hcf_put_header
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.ARGUMENTS
  int hcf_put_header( IFBP ifbp, int_offset, wci_bufp bufp, int len, hcf_8 check )
  Card Interrupts disabled

.RETURNS
	HCF_ERR_NO_NIC	
	>>hcfio_string	
	HCF_ERR_DCWA	

.DESCRIPTION	Transfers in a non-sequential fashion, a single block of control information to the Card.

.NARRATIVE
	parameters:
		ifbp		address of the Interface Block
		offset		offset (in bytes) in relative to Destination Address field of Tx FRame
		bufp		char pointer, address of buffer in PC RAM
		len			length (in bytes) of data to be copied
		check		value which should be contained in the byte immediately after the header
		
		
		
		which is garuanteed not to be present in the original dummy header
					HCF_PUT_DATA_RESET
	
	Hcf_put_header copies the number of bytes specified by parameter len from the location in PC
	RAM specified by bufp to the position specified by offset + HFS_ADDR_DEST_ABS in the NIC RAM buffer 
	associated with the Protocol Stack dedicated FID.
	Hcf_put_header must be called after the last hcf_put_data needed to write the actual frame to the transmit
	data buffer and preceeding the hcf_send call. Only a single hcf_put_header call is allowed.
	Hcf_put_header overwrites the data which was written at the corresponding locations by hcf_put_data.
	
	First hcf_put_header checks whether the preceding hcf_put_data call(s) suffered a Data Corruption. If so,
	hcf_put_header returns without writing the header with a return value of ;?HCF_FAILURE.
	Next hcf_put_header copies an even number of bytes (specified by datlen) from Host RAM (specified by
	bufp ) to the Transmit Frame Structure at the even offset (specified by offset) relative to HFS_ADDR_DEST_ABS

	
.DIAGRAM
1*: If the card is not present or not enabled, prevent all I/O. This is especially important in case the card
	is not present, because in that case "our" I/O base may have been given away to someone
	else in a CS/SS environment.  Also no I/O should be performed if the NIC is not enabled. However
	an MSF which calls hcf_put_data while the NIC is not enabled probably contains a logic flaw (or has a
	strategy which was not foreseen at the time this was written)
2*:	Perform the DataCorruptionWorkAround check
4*:	write the header over the garbage header which was written by the preceeding hcf_put_data() calls.
	Note that the original garbage header should NOT contain check, the first byte of the data after the 
	header written by hcf_put_header(). Return status is ignored, since the preceeding hcf_iostring did
	not fail and a failing hcf_iostring should also result in failing hcf_send
6*:	Perform a (minimized) DataCorruptionWorkAround check by reading back the first byte after the garbage
	header as written by the preceeding hcf_put_data() calls. Since the HCF has no knowledge what that byte
	should be, it is the responsibilty of the MSF to supply this value.
	If the test fails, retry up to two times.
	Note that hcfio_string can not be used for the DCWA check because that would corrupt the data after the
	header
8*:	Mimick the hcfio_string behavior

	
.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
int	hcf_put_header(IFBP ifbp, hcf_16 offset, wci_bufp bufp, int len, hcf_8 check ) {

int 	rc = HCF_ERR_NO_NIC;
int		cnt = 3;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_HDR, len );
	HCFASSERT( ((long)bufp & (HCF_ALIGN-1) ) == 0, (hcf_16)(long)bufp )
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_PUT_HDR) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_PUT_HDR )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_CardStat & CARD_STAT_ENABLED, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_PIF_FID, 0 )
	HCFASSERT( ifbp->IFB_PIFRscInd == 0, ifbp->IFB_PIFRscInd )
	HCFASSERT( ifbp->IFB_IntOffCnt, 0 )
	HCFASSERT( bufp != NULL, 0 )
	HCFASSERT( len, 0)
	HCFASSERT( 0 <= len && len <= HCF_MAX_MSG, len )
	HCFASSERT( 0 <= offset && offset <= HCF_MAX_MSG, len )
	HCFASSERT( offset + len <= HFS_TX_ALLOC_SIZE - KLUDGE_MARGIN, offset + len )


	if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {							/* 1 */
																										/* 2 */
		rc = hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, ifbp->IFB_PIFLoadIdx, NULL, 0, 0, IO_OUT_CHECK );
		
		if ( rc == HCF_SUCCESS ) {
			do {
				(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, offset + HFS_ADDR_DEST_ABS,			/* 4 */
									bufp, 0, len, IO_OUT );
			} while ( ( rc = IN_PORT_BYTE( ifbp->IFB_IOBase + HREG_DATA_0 ) ^ check )!=0 && cnt-- );	/* 6 */
			if ( rc ) {																					/* 8 */
				rc = HCF_ERR_DCWA;
				ifbp->IFB_HCF_Tallies.EngCnt++;
			}
		}			
		if ( rc != HCF_SUCCESS ) ifbp->IFB_PIFRscInd = 1;  //;?or should this be more restrictive like HCF_ERR_DCWA
	}
	HCFASSERT( rc == HCF_SUCCESS, rc );
	HCFASSERTLOGEXIT( HCF_ASSERT_PUT_HDR )	
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_HDR, -1 );
	return rc;
} /* hcf_put_header */
#endif //HCF_CELLWAVE
//@HCFL>


/**************************************************************************************************************


.MODULE			hcf_put_info
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION
	Transfers operation information and transient and persistent configuration information to the Card.
.ARGUMENTS
  int hcf_put_info( IFBP ifbp, LTVP ltvp )
  Card Interrupts disabled
.RETURNS
	-- value --				-- parameter --
	HCF_SUCCESS				CFG_TICK_TIME (but Nothing happens)
	HCF_SUCCESS				<= CFG_RID_CFG_MIN
	HCF_SUCCESS				>= CFG_RID_CFG_MAX
	>>download				CFG_DLNV_START <= ..... <= CFG_DL_STOP
	>>hcfio_string			CFG_NOTIFY (on non-AP (~station); always HCF_SUCCESS)
	>>cmd_wait				CFG_NOTIFY (on non-AP (~station); always HCF_SUCCESS)
	HCF_SUCCESS				<= CFG_RID_CFG_MIN <= .... <= CFG_RID_CFG_MAX
	put_info				<= CFG_RID_CFG_MIN <= .... <= CFG_RID_CFG_MAX

.NARRATIVE
	parameters:
		ifbp		address of the Interface Block
		ltvp		specifies the RID (as defined by Hermes I/F) or pseudo-RID (as defined by WCI)

.NOTICE
 Remarks:
	The Hermes puts additional sequence constraints on the usage of hcf_put_info,
	e.g. CFG_NOTIFY is only allowed after hcf_enable

 Remarks:  In case of Hermes Configuration LTVs, the codes for the type are "cleverly" chosen to
	be identical to the RID. Hermes Configuration information is copied from the provided data 
	structure into the Card.
	In case of HCF Configuration LTVs, the type values are choosen in a range which does not overlap
	the RID-range.
	
	//<HCFL@
 Remarks:  In order to make the changes sustain over activities like hcf_diagnose and recovery from PCMCIA 
 	card insertion, Hermes Configuration LTVs are saved in the IF-block.

    The mechanism to HCF_ASSERT on invalid typ-codes in the LTV record is based on the following strategy:
	  -	some codes (e.g. CFG_REG_MB) are explicitly handled by the HCF which implies that these codes
		are valid
	  - all other codes are passed to the Hermes.  Before the put action is executed, hcf_get_info is called
	  	with an LTV record with a value of 1 in	the L-field and the intended put action type in the Typ-code
	  	field. If the put action type is valid, it must definitely be valid as a get action type code, so
	  	the HCF_ASSERT logic of hcf_get_info should not catch.
    //@HCFL>	
	
.DIAGRAM
02:	see NOTICE in hcf_get_info() <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<///////??????????????????????????
    //<HCFL@
19:	if HCF_ASSERT is not defined, the else clause absorbs the "return rc;" statement. This is plainly not the
	intention, hence the semi-colon after the first HCFASSERT, so there will be at least an empty statement
	after the else.
	Note that I consider it an error of those compilers who do not flag this problem, e.g. MSVC 4 as used
	for the Miniport (jumps over the mov ax, [bp-n] without complaining about "no return value"), MSVC 1.5 as
	used for the DOS ODI (inserts a NOP and has all if/if_else branches jump to the mov ax, [bp-6], which is
	what I wanted but not what I asked for), Borland 3.1 as used for WavePoint (which even gets confused to
	the level where it leaves out necessary instructions in the if_else branches, skips the mov ax, [bp-n]
	without complaints)
	//@HCFL>

.NOTICE
	Future enhancements in the functionality offered by the WCI and/or implementation aspects of the HCF
	may warrant filtering on the type-field of the LTV to recognize non-MSF accessible records, e.g. CFG_TICK
	
**************************************************************************************************************/

int hcf_put_info( IFBP ifbp, LTVP ltvp ) {

int	rc = HCF_SUCCESS;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_INFO, ltvp->typ );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_PUT_INFO) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_PUT_INFO )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ltvp, 0 )
	HCFASSERT( ltvp->len - 1 < 0x8000, ltvp->len )
	
											//all codes between 0xFC00 and 0xFCFF are passed to Hermes with 
											//exception of CFG_TICK_TIME to suppress tampering with the NIC timer
	if ( CFG_RID_CFG_MIN <= ltvp->typ && ltvp->typ <= CFG_RID_CFG_MAX && ltvp->typ != CFG_TICK_TIME ) {	/* 2 */
		rc = put_info( ifbp, ltvp );
#if HCF_MAX_CONFIG != 0																				//<HCFL@
		put_info_tbl( ifbp, ltvp );
#endif // HCF_MAX_CONFIG																			//@HCFL>
#if !defined HCF_DOWNLOAD_OFF                         												//<HCFL@
	} else if ( CFG_DLNV_START <= ltvp->typ && ltvp->typ <= CFG_DL_STOP ) {
		rc = download( ifbp, ltvp );                                                                //@?HCF?L>
#endif // HCF_DOWNLOAD_OFF
	} else switch (ltvp->typ) {
#if !defined __BEOS__ && defined HCF_ASSERT  //<HCFL?@
 	  case CFG_REG_ASSERT_RTNP:											//Register MSF Routines
		ifbp->IFB_AssertRtn = ((CFG_REG_ASSERT_RTNP_STRCT FAR*)ltvp)->rtnp;
		ifbp->IFB_AssertLvl = ((CFG_REG_ASSERT_RTNP_STRCT FAR *)ltvp)->lvl;
		ifbp->IFB_AssertLvl = 1;										//;? not yet supported so default
		break;
	  case CFG_REG_LOG_RTNP:											//Register MSF Routines
		ifbp->IFB_LogRtn = ((CFG_REG_LOG_RTNP_STRCT FAR*)ltvp)->rtnp;
		ifbp->IFB_LogLvl = ((CFG_REG_LOG_RTNP_STRCT FAR *)ltvp)->lvl;
		break;
		
#endif // HCF_ASSERT    //@?HCFL>
	  case CFG_REG_MB:													//Register MailBox          //<?HCFL?@
	  	HCFASSERT( (*(long FAR *)ltvp->val & 01) == 0, ltvp->val[0] )
		ifbp->IFB_MBp = *(wci_recordp FAR *)&ltvp->val[0];
		ifbp->IFB_MBSize = ifbp->IFB_MBp == NULL ? 0 : ltvp->val[2];	/* if no MB present, size must be zero
																		 * for put_info_mb to work correctly */
		ifbp->IFB_MBWp = ifbp->IFB_MBRp	= 0;
		ifbp->IFB_MBInfoLen = 0;
		ifbp->IFB_HCF_Tallies.NoBufMB = 0;														//<HCFL?>
		HCFASSERT( ifbp->IFB_MBSize >= 60 || ifbp->IFB_MBp == NULL, ifbp->IFB_MBSize )
		break;                                                                                  //@?HCF?L>
	  case CFG_CNTL_RESTRAINT:																	//<HCFL@
		HCFASSERT( ltvp->val[0] == RESTRAINT_RESET || ltvp->val[0] == RESTRAINT_SET, ltvp->val[0] )
		if ( ltvp->val[0] == RESTRAINT_RESET ) {
	  		ifbp->IFB_WarningInfo[1] &= ~ltvp->val[1];
	  	} else {
	  		ifbp->IFB_WarningInfo[1] |= ltvp->val[1];
	  	}
		break;																					//@HCFL>
#if !defined HCF_MB_OFF
	  case CFG_MB_ASSERT:                                                                       //<?HCFL?@
		put_info_mb( ifbp, ltvp->typ, ltvp->val, ltvp->len /*- 1*/ );
		break;                                                                                  //@?HCF?L>
#endif // HCF_MB_OFF
#if defined HCF_AP
	  case CFG_NOTIFY:
		HCFASSERT( ifbp->IFB_CardStat & CARD_STAT_ENABLED, ifbp->IFB_CardStat )
        HCFASSERT( ifbp->IFB_NotifyRscInd, ltvp->typ )
	
		if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {
	        ifbp->IFB_NotifyRscInd = 0;
			if ( ( rc = hcfio_string( ifbp, BAP_1, ifbp->IFB_Notify_FID, 0, (wci_bufp)ltvp,
										  2, MUL_BY_2(ltvp->len + 1 ), IO_OUT ) ) == HCF_SUCCESS ) {
				 rc = cmd_wait( ifbp, HCMD_NOTIFY + HCMD_RECL, ifbp->IFB_Notify_FID );
			}
		}
		break;
#endif // HCF_AP
#if defined HCF_STA && defined HCF_AP
	  case CFG_CNTL_MSF_TYPE:
		ifbp->IFB_CardStat &= ~CARD_STAT_AP;
		ifbp->IFB_MSFType = ltvp->val[0];
		if ( ifbp->IFB_MSFType ) ifbp->IFB_CardStat |= CARD_STAT_AP;
		ifbp->IFB_CfgTbl[0] = 0;						//flush Configuration
		break;
#endif // HCF_STA && HCF_AP
	  case CFG_CNTL_ENG_FLG:
		ifbp->IFB_EngFlg = ltvp->val[0];
		break;
#if defined HCF_TX_ERR_NOTIFY
	  case CFG_CNTL_TX_FLG:
		ifbp->IFB_TxCntl[0] = (hcf_8)ltvp->val[0];
		break;
#endif //HCF_TX_ERR_NOTIFY
	  default:
		HCFASSERT( DO_ASSERT, ltvp->typ )
         /*NOP*/;															      		//@HCFL>	/* 19*/
	}	
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_PUT_INFO )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_PUT_INFO, -1 );
	return rc;
}/* hcf_put_info */


/*FF***************************************************************************************************************
FUNCT: hcf_send                                                                                  2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                hcfio_string                                     
       CNV_INT_TO_BIG_NP                                cmd_wait                                         
       HCFASSERTLOGEXIT                                 
DEFIN: BAP_0                                            CARD_STAT_ENABLED                                
       CARD_STAT_ENA_PRES                               DO_ASSERT                                        
       ENC_TX_E_II                                      HCF_ASSERT_SEND                                  
       HCF_MAGIC                                        HCMD_CMD_CODE                                    
       HCMD_RECL                                        HCMD_TX                                          
       HFS_DAT_ABS                                      HFS_LEN_ABS                                      
       IO_IN                                            IO_OUT                                           
       IO_OUT_CHECK                                     NULL                                             
GLOBL: HCF_ERR_NO_NIC                                   HCF_PORT_6                                       
       HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_BAP_0                                  ifbp->IFB_CardStat                               
       ifbp->IFB_FSBase                                 ifbp->IFB_Magic                                  
       ifbp->IFB_PIFLoadIdx                             ifbp->IFB_PIFRscInd                              
       ifbp->IFB_PIF_FID                                ifbp->IFB_TxCntl                                 
       ifbp->IFB_TxFrameType                            port                                             
LOCAL: rc                                               
******************************************************************************************************************/
/******************************************************************************************************************

.MODULE			hcf_send
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION	Transmit a message on behalf of the protocol stack
.ARGUMENTS
  int hcf_send( IFBP ifbp , hcf_16 port )
  Card Interrupts disabled

.RETURNS
	HCF_ERR_NO_NIC
	>>hcfio_string	

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  type

	Hcf_send transmits the Protocol Stack message loaded in NIC RAM by the
	preceeding hcf_put_data calls.
	
	Hcf_send() does not NEED a return value to report NIC absence or removal during the execution of hcf_send(),
	because the MSF and higher layers must be able to cope anyway with the NIC being removed after a successful 
	completion of hcf_put_data() + hcf_send() but before the actual transmission took place. To accomodate
	user expectations the current implementation does report NIC absence.

.DIAGRAM
1:	The actual data length (the number of bytes in the Tx Frame structure
	following the 802.3/802.11 Header Info blocks, is determined by
	IFB_PIFLoadIdx, the index in the Transmit Frame Structure to store the
	"next" byte. Note that this value must be compensated for the header info
	by subtracting HFS_DAT.
2/3:TxFrameType - which is based on the preceding hcf_put_data calls - defines
	whether the actual data length is written to the 802.11 or 802.3 Header Info
	block.
2:	In case of 802.11, the entry parameter type is augmented to reflect	802.11
	before it is written to the Control Field block.
3:	In case of 802.3, the actual length must be converted from the native
	format of the Host (Little Endian in case of an 80x86) to Big Endian before
	it is written to the 802.3 Header Info block.
4:	The actual send+reclaim command is performed by the routine send.
7:	The return status of hcfio_in_string can be ignored, because when it fails, cmd_wait will fail via the
 	IFB_DefunctStat mechanism
17:	The return status of cmd_wait and the first hcfio_in_string can be ignored, because when one fails, the
 	other fails via the IFB_DefunctStat mechanism
19:	The "HCFASSERT( ifbp->IFB_PIFLoadIdx = 0xFFFF )" does never catches, but initializes IFB_PIFLoadIdx to
    be tested by an HCFASSERT in hcf_put_data if and only the ASSERT mechanism is enabled
.NOTICE
  ;?This comment is definitely out of date
  The choice to let hcf_send calculate the actual data length as
  IFB_PIFLoadIdx - HFS_DAT, implies that hcf_put_data with the HFS_LUCENT
  mechanism MUST be used to write the Data Info Block. A change in this I/F
  will impact hcf_send as well.
  An alternative would be to have a parameter datlen. If datlen is zero, the
  current behavior is used. If datlen has a non-zero value, its value is used
  as the actual data length (without validating against HCF_MAX_MSG and without
  validating the total number of bytes put by hcf_put_data).

.NOTICE
  hcf_put_data/send leave the responsibility to only send messages on enabled ports at the MSF level.
  This is considered the strategy which is sufficiently adequate for all "robust" MSFs, have the least
  processor utilization and being still acceptable robust at the WCI !!!!!
.ENDOC				END DOCUMENTATION

------------------------------------------------------------------------------------------------------------*/
int hcf_send( IFBP ifbp , hcf_16 port ) {	//;?note that port is unused due to ambivalence about what the "right" I/F is

int	rc = HCF_ERR_NO_NIC;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SEND, port );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_SEND) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_SEND )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, ifbp->IFB_CardStat )
	HCFASSERT( ifbp->IFB_CardStat & CARD_STAT_ENABLED, ifbp->IFB_CardStat )
	HCFASSERT( port <= HCF_PORT_6, port )
	HCFASSERT( ifbp->IFB_TxCntl[1] == port , port )

	if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {                    /* 1 */	
		/* H/W Pointer problem detection */	
		if ( ifbp->IFB_BAP_0[0] != ifbp->IFB_FSBase ) {	//;?<HCF _L> should BE HARD CODED, also add to send diag msg/* 30*/
														// ;? WHAT IS THE PURPOSE OF THIS if <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			rc = hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, ifbp->IFB_PIFLoadIdx, NULL, 0, 0, IO_OUT_CHECK );
			
			if ( rc != HCF_SUCCESS ) {
	 			HCFASSERT( DO_ASSERT, 0 )
				ifbp->IFB_PIFRscInd = 1;
			}
		}
		if ( rc == HCF_SUCCESS ) {
			if ( /*ifbp->IFB_FrameType == ENC_802_11 || */ ifbp->IFB_TxFrameType == ENC_TX_E_II ) {		/* 2 */
				ifbp->IFB_PIFLoadIdx -= HFS_DAT_ABS;		//actual length of frame					/* 1 */
//__BEOS__				CNV_INT_TO_BIG_NP(&ifbp->IFB_PIFLoadIdx);  //;?is it worthwhile to have this additional macro
				ifbp->IFB_PIFLoadIdx = CNV_INT_TO_BIG(ifbp->IFB_PIFLoadIdx);  //;?BEOS can't handle the / as line contunation (apparently)
				(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, HFS_LEN_ABS,
										(wci_bufp)&ifbp->IFB_PIFLoadIdx, 0, 2, IO_OUT );				/* 7 */
			}
			rc = cmd_wait( ifbp, HCMD_TX + HCMD_RECL, ifbp->IFB_PIF_FID );								/* 17*/
						//;? HOW TO TREAT RSC_IND <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			HCFASSERT( (ifbp->IFB_PIFLoadIdx |= 0xFFFF) , 0 )											/* 19*/
		}
		/* reset the BAP pointer for the Tx Framestructure, note that we do access only the BAP but no NIC RAM
		 * so there is no problem despite the fact that the Hermes send command relinguished control of the 
		 * Tx FID to the Hermes
		 */
		(void)hcfio_string( ifbp, BAP_0, ifbp->IFB_PIF_FID, ifbp->IFB_FSBase, NULL, 0, 0, IO_IN );
	}
	HCFASSERTLOGEXIT( HCF_ASSERT_SEND )
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SEND, -1 );
	return rc;
} /* hcf_send */


//<HCFL@
/*FF***************************************************************************************************************
FUNCT: hcf_send_diag_msg                                                                         2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                hcfio_string                                     
       cmd_wait                                         HCFASSERTLOGEXIT                                 
DEFIN: BAP_0                                            CARD_STAT_ENA_PRES                               
       HCF_ALIGN                                        HCF_ASSERT_SEND_DIAG_MSG                         
       HCF_MAGIC                                        HCF_MAX_MSG                                      
       HCMD_CMD_CODE                                    HCMD_RECL                                        
       HCMD_TX                                          HFS_ADDR_DEST_ABS                                
       IO_OUT_CHECK                                     
GLOBL: HCF_ERR_BUSY                                     HCF_ERR_NO_NIC                                   
       HCF_SUCCESS                                      
PARAM: bufp                                             dummy                                            
       ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_DUIFRscInd                             
       ifbp->IFB_DUIF_FID                               ifbp->IFB_Magic                                  
       len                                              
LOCAL: rc                                               
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			hcf_send_diag_msg
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION	Transmit a message on behalf of the Driver-Utility I/F
.ARGUMENTS
  int hcf_send_diag_msg( IFBP ifbp, hcf_16 dummy, wci_bufp bufp, int len )
  Card Interrupts disabled

.RETURNS
	HCF_ERR_NO_NIC	
	HCF_ERR_BUSY	;?more appropriate value needed
	>>hcfio_string	
	>>cmd_wait	

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  dummy
  bufp
  len

	Hcf_send_diag_msg transmits the message

.DIAGRAM

 2:

 4: Based on the assumption that hcf_send_diag_msg is called at a low frequency, HFS_TX_CNTL_ABS is written
  	on each call rather than using an IFB-field to remember the previous value and update only if needed

.NOTICE
	When dummy would be replace
.ENDOC				END DOCUMENTATION

-------------------------------------------------------------------------------------------------------------*/
int hcf_send_diag_msg( IFBP ifbp, hcf_16 dummy, wci_bufp bufp, int len ) {

int rc = HCF_ERR_NO_NIC;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SEND_DIAG_MSG, len );
	HCFASSERT( ( ifbp->IFB_AssertTrace & HCF_ASSERT_SEND_DIAG_MSG) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_SEND_DIAG_MSG )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ((long)bufp & (HCF_ALIGN-1) ) == 0, (hcf_16)(long)bufp )
	HCFASSERT( bufp, 0 )
	HCFASSERT( 14 <= len && len <= HCF_MAX_MSG, len )
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0, 0 )
	HCFASSERT( ifbp->IFB_DUIFRscInd, 0 )
//	HCFASSERT( ifbp->IFB_FrameType == HCF_ACT_802_3, 0 )  Not supported when HCF is in 802.11 mode

	HCFASSERT( dummy == 0, dummy )	//used to be WDS-port and/or 802.3/11 type indicator but no longer supported

	if ( (ifbp->IFB_CardStat & CARD_STAT_ENA_PRES) == CARD_STAT_ENA_PRES ) {
		rc = HCF_ERR_BUSY; //;?more appropriate value needed
		if ( ifbp->IFB_DUIFRscInd ) {																	/* 2 */
			rc = hcfio_string( ifbp, BAP_0, ifbp->IFB_DUIF_FID, HFS_ADDR_DEST_ABS, bufp, 0, len, IO_OUT_CHECK );
			if ( rc == HCF_SUCCESS ) {
				ifbp->IFB_DUIFRscInd = 0;
				rc = cmd_wait( ifbp, HCMD_TX + HCMD_RECL, ifbp->IFB_DUIF_FID );			
			}
		}
	}
	HCFASSERTLOGEXIT( HCF_ASSERT_SEND_DIAG_MSG )
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SEND_DIAG_MSG, -1 );
	return rc;
} /* hcf_send_diag_msg */
//@HCFL>


/*FF***************************************************************************************************************
FUNCT: hcf_service_nic                                                                           2000/02/04 14:15
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       HCFASSERTLOGENTRY                                IN_PORT_WORD                                     
       OUT_PORT_WORD                                    HCFLOG                                           
       isr_info                                         hcfio_string                                     
       put_info_mb                                      DIV_BY_2                                         
       enc_test                                         HCFASSERTLOGEXIT                                 
DEFIN: BAP_1                                            CARD_STAT_DEFUNCT                                
       CARD_STAT_PRESENT                                CFG_WMP                                          
       HCF_ASSERT_SERVICE_NIC                           HCF_LOG_SERVICE_NIC                              
       HCF_MAGIC                                        HCF_MAX_MSG                                      
       HFS_ADDR_DEST_ABS                                HFS_DAT                                          
       HFS_DAT_LEN_ABS                                  HFS_LEN                                          
       HFS_LEN_ABS                                      HFS_STAT_1042                                    
       HFS_STAT_ABS                                     HFS_STAT_ERR                                     
       HFS_STAT_MSG_TYPE                                HFS_STAT_TUNNEL                                  
       HFS_STAT_WMP_MSG                                 HFS_TYPE                                         
       HFS_TYPE_ABS                                     HFS_WDS_TYPE_ABS                                 
       HREG_ALLOC_FID                                   HREG_EV_ACK                                      
       HREG_EV_ALLOC                                    HREG_EV_CMD                                      
       HREG_EV_DUIF_RX                                  HREG_EV_INFO                                     
       HREG_EV_INFO_DROP                                HREG_EV_NO_CARD                                  
       HREG_EV_RES                                      HREG_EV_RX                                       
       HREG_EV_STAT                                     HREG_EV_TICK                                     
       HREG_RX_FID                                      HREG_SW_0                                        
       IO_IN                                            NULL                                             
GLOBL: HCF_ACT_802_3                                    enc_trans_tbl                                    
       ifbp->IFB_HCF_Tallies.NoBufInfo                  snap_header                                      
PARAM: ifbp                                             ifbp->IFB_AssertTrace                            
       ifbp->IFB_CardStat                               ifbp->IFB_DUIFRscInd                             
       ifbp->IFB_DUIF_FID                               ifbp->IFB_DefunctStat                            
       ifbp->IFB_EngFlg                                 ifbp->IFB_FrameType                              
       ifbp->IFB_IOBase                                 ifbp->IFB_IntEnMask                              
       ifbp->IFB_IntOffCnt                              ifbp->IFB_MB_FID                                 
       ifbp->IFB_Magic                                  ifbp->IFB_NotifyRscInd                           
       ifbp->IFB_Notify_FID                             ifbp->IFB_PIFRscInd                              
       ifbp->IFB_PIF_FID                                ifbp->IFB_RxFID                                  
       ifbp->IFB_RxFence                                ifbp->IFB_RxLen                                  
       ifbp->IFB_RxStat                                 
LOCAL: rc                                               sample                                           
       stat                                             tmp                                              
       type                                             
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_service_nic
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.APPLICATION	Data Transfer Function for WaveLAN based drivers and utilities
.DESCRIPTION	Provides received message, handles (most) Hermes events
.ARGUMENTS
  int hcf_service_nic( IFBP ifbp )
  Card Interrupts disabled

.RETURNS
	HREG_EV_NO_CARD		pseudo event, returned when no NIC or when Defunct
	xxxx				accumulated EvStat register + simulated HREG_EV_DUIF_RX


.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block

  MSF-accessible fields of Result Block
	IFB_RxLen			0 or Frame size as reported by LAN Controller
	IFB_RxStat
	IFB_MBInfoLen
	IFB_PIFRscInd
	IFB_DUIFRscInd
	IFB_NotifyRscInd
	IFB_HCF_Tallies

	hcf_service_nic is primarily intended to be part of the Interrupt Service Routine.
	hcf_service_nic is presumed to neither interrupt other HCF-tasks nor to be interrupted by other HCF-tasks.
	A way to achieve this is to precede hcf_service_nic as well as all other HCF-tasks with a call to
	hcf_action to disable the card interrupts and, after all work is completed, with a call to hcf_action to
	restore (which is not necessarily the same as enabling) the card interrupts.
	In case of a polled environment, it is assumed that the MSF programmer is sufficiently familiar with the
	specific requirements of that environment to translate the interrupt strategy to a polled strategy.
	
	hcf_service_nic services the following Hermes events:
		HREG_EV_INFO		Asynchronous Information Frame
		HREG_EV_INFO_DROP	WMAC did not have sufficient RAM to build Unsolicited Information Frame
		HREG_EV_ALLOC		Asynchronous part of Allocation/Reclaim completed
		HREG_EV_RX			the detection of the availability of received messages
                            including WaveLAN Management Protocol (WMP) message processing  //<HCFL>

	//<HCFL@
    A received message can be either "NOS" messages or "WMP" (aka DUIF) messages.
	A WMP message is recognized by the Hermes and reported as such in the Status field of the Rx Frame
	Structure. A number of WMP messages are handled by the Hermes and are never passed to the Host at all.
	The error free WMP messages which are passed to the Host are stored in the MailBox (assuming one with
	sufficient free space is available). As a side effect, when IFB_MBInfoLen is zero, it is updated to
	reflect the size of the MailBox Information Block used to store the WMP message.
	;? If Monitor Mode is active, a WMP message SHOULD be considered a NOS message. Current implementation
	moves all ERRORFREE messages (also those addressed to other stations) into the MailBox. Frames
	containing errors are passed to the MSF as is (without decapsulation). Note that the
	Hermes does NOT pass those WMP frames to the host to which the Hermes generated a response.

	An available NOS messages is a message which:
	 -	is received by the LAN Controller with an OK status and which is not an WMP message
	 -	All messages (;?including WMP messages and messages with an error status) when Monitor Mode is enabled
    //@HCFL>
	If a message is available, its length is reflected by the IFB_RxLen field of the IFB. This length
	reflects the data itself and the Destination Address, Source Address and DataLength/Type field but not the
	SNAP-header in case of decapsulation by the HCF).
	If no message is available, IFB_RxLen is zero.

  **Rx Buffer free strategy
	When hcf_service_nic reports the availability of a message, the MSF can access that message or parts
	thereof, by means of hcf_get_data calls till the next call of hcf_service_nic. Therefore it must be
	prevented that the LAN Controller writes new data in the buffer associated with the last hcf_service_nic
	report.
	As a consequence hcf_service_nic is the only procedure which can free receive buffers for re-use by the
	LAN Controller. Freeing a buffer is done implicitly by acknowledging the Rx event to the Hermes. The
	strategy of hcf_service_nic is to free the buffer it has reported as containing an available message in
	the preceeding call (assuming there was an available message).
	A consequence of this strategy is that the Interrupt Service Routine of the MSF must repeatedly call
	hcf_service_nic till hcf_service_nic returns "no message available". It can be reasoned that
	hcf_action( INT_ON ) should not be given before the MSF has completely processed a reported Rx-frame. The
	reason is that the INT_ON action is guaranteed to cause a (Rx-)interrupt (the MSF is processing a
	Rx-frame, hence the Rx-event bit in the Hermes register must be active). This interrupt will cause
	hcf_service_nic to be called, which will cause the ack-ing of the "last" Rx-event to the Hermes,
	causing the Hermes to discard the associated NIC RAM buffer.


.DIAGRAM
 2: IFB_RxLen and IFB_RxStat must be cleared before the NIC presence check otherwise these values may stay
 	non-zero if the NIC is pulled out at an inconvenient moment
 4: If the card is not present, prevent all I/O because "our" I/O base may have been given away to someone
	else in a CS/SS environment.
	The MSF may have considerable latency in informing the HCF of the card removal by means of hcf_action().
	To prevent that hcf_service_nic reports bogus information to the MSF with all - possibly difficult to
	debug - undesirable side effects, hcf_service_nic pays performance wise the prize to use the momentanuous
	NIC presence test by checking the contents of the Hermes register HREG_SW_0 against the value HCF_MAGIC.
 6:	The return status of hcf_service_nic is defined as reflecting all interrupt causes this call has run into,
 	hence an accumulator is needed. This return status services ONLY to help the MSF programmer to debug the
 	total system.
 	When the card is removed, the pseudo event HREG_EV_NO_CARD is reported.
 	NOTE, the HREG_EV_NO_CARD bit is explicitly not intended for the MSF to detect NIC removal. The MSF must
 	use its own - environment specific - means for that.
    //<HCFL@
    The reception of a WMP message is reported by means of the pseudo-event HREG_EV_DUIF_RX
    This bit is set in addition to the HREG_EV_RX bit.
 8:	hcf_service_nic loops to handle frame reception which is handled at the HCF level without being reported
	to the MSF. This are currently only the WMP frames which are stored in the MailBox.
    //@HCFL>
10:	ack the "old" Rx-event. See "Rx Buffer free strategy" above for more explanation.
    IFB_RxFID, IFB_RxLen and IFB_RxStat must be cleared to bring both the internal HCF house keeping as the
    information supplied to the MSF in the state "no frame received"
12:	The event status register of the Hermes is sampled and all non-Rx activities are handled.
 	The non-Rx activities are:
	 -	Info drop events are handled by incrementing a tally
	 -	LinkEvent (including solicited and unsolicited tallies) are handled by procedure isr_info.
	 -	Alloc, handled by setting the resource indicator.
		<HCFL@  The corresponding FID register is sampled, and based on this FID, either
		the IFB_PIFRscInd, the IFB_DUIFRscInd or the IFB_NotifyRscInd is raised.
	 	Note that no ASSERT is performed to check whether the RscInd corresponding with the sampled
	 	HREG_ALLOC_FID has a zero value. It is felt that this obscures the code to the reader while adding
	 	little practical value
		Note: during the exceution of alloc(), Card Interrupts are disabled as a side effect of 
		hcf_initialiaze(). This prevents that the ISR can get control while IFB_PIF_FID and IFB_DUIF_FID are
		not yet defined.																			@HCFL>
13:	Asserting on RscInd == 0 does not seem worth the effort	 	
14:	All the non-Rx/non-Cmd activities are acknowledged. Combining all these acknowledgements to a single
	place, is considered an optimization.
	Note that the Rx-acknowledgement is explicitly not included, as justified in "Rx Buffer free strategy" above.
	Note that the Cmd-acknowledgement is explicitly not included, because all command handling is handled
	in line.
	<HCFL@  Refer to get_info_mb for more details on  acknowledgement of the pseudo-asynchronous commands
	(diagnose and download non-volatile ).															@HCFL>
16:	The handling of the non-Rx activities, may have bought the Hermes sufficient time to raise an Rx event
	in the event status register (assuming an other Rx event was pending and this loop through hcf_service_nic
	acknowledged an Rx event). Therefore the event status register is sampled again. If a frame is available,
	the FID of that frame and the characteristics (status and length) are read from the NIC. These values are,
	after Endianess conversion if needed, stored in IFB_RxStat and IFB_RxLen. IFB_RxLen is also adjusted for
	the size of the 802.3 MAC header. ;? Note: Whether this adjustment is the correct/most optimal for 802.11
	is debatable, however it is paramount that IFB_RxFID and IFB_RxLEN must either be both zero or both
	non-zero to get a coherent behavior of the MSF+HCF.
18:	If the Hermes frame status reflects an error - which can only occure in promiscuous mode - the frame
	is not further processed and control is passed back to the MSF
20:	<HCFL@  WMP messages are processed by copying them to the MailBox. Accu is updated to reflect the reception
	of the WMP frame to the debugger and sample is modified to go once more through the loop in the hope
	to process the next pending Rx frame.
	Note that in case of promiscuous mode, the HFS_CNTL_STAT_WMP_MSG is never set.  				@HCFL>
22: Both in 802.11 mode and 802.3_pure mode, the frame is not further processed (no decapsulation) and
	control is passed back to the MSF.
	In 802.3 mode the HCF checks whether decapsulation is needed (i.e. the Hermes reported Tunnel
	encapsulation or the Hermes reported 1042 Encapsulation and the frame type does not match one of the
	values in enc_trans_tbl.
	The IFB_RxFence field, which plays a role in the decapsulation process, is initialized.
	The actual decapsulation takes place on the fly in hcf_get_data, based on the value of IFB_RxFence.
	Note that in case of decapsulation the SNAP header is not passed to the MSF, hence IFB_RxLen must be
	compensated for the SNAP header length
24:	If a WMP frame is received, another loop through hcf_service_nic is made, acknowledging     //<HCFL>
	this WMP frame as a side effect.                                                            //<HCFL>
	
	
	
xx: Decapsulation is NOT done in case of:
	- WMP messages transferred to the MailBox
	- IFB_FrameType == HCF_ACT_802_3_PURE or HCF_ACT_802_11
	- in case of eronuoes frame in promiscuous mode
	Decapsulation is done in case of error free frame in promiscuous mode
	
.NOTICES
    To make it possible to discriminate between a message without payload (only MAC addresses and implicit
    the length) it is a convenient I/F to add 14 ( space occupied by MAC addresses and Length) to the
    Hermes reported payload. So the maintenance programmer should be forwarned when considering changing
    this strategy. Also the impact on 802.11 should be considered ;?
	
	Possibly the two consecutive hcf_io_string() calls to gather IFB_RxLen and IFB_RxStat could be
	optimized by using a read/discard/read sequence. Superficially measurements of BAP setup times
	suggest that this optimization is hardly worthwhile (if at all)
	
.ENDOC				END DOCUMENTATION

-------------------------------------------------------------------------------------------------------------*/
int	hcf_service_nic(IFBP ifbp ) {

hcf_16	rc = HREG_EV_NO_CARD;
hcf_16	sample, tmp;
hcf_16	stat, type;                                                                              //<HCFL>

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SERVICE_NIC, 0 );
	HCFASSERT( (ifbp->IFB_AssertTrace & HCF_ASSERT_SERVICE_NIC) == 0, ifbp->IFB_AssertTrace )
	HCFASSERTLOGENTRY( HCF_ASSERT_SERVICE_NIC )
	HCFASSERT( ifbp->IFB_Magic == HCF_MAGIC, ifbp->IFB_Magic )
	HCFASSERT( ifbp->IFB_IntOffCnt, 0 )

    ifbp->IFB_RxLen = ifbp->IFB_RxStat = 0;																		/* 2 */
	if ( ifbp->IFB_CardStat & CARD_STAT_DEFUNCT ) { 
		ifbp->IFB_IntEnMask = 0; //disable interrupts till MSF recovers by means of hcf_enable via hcf_act_card_in
		rc = ifbp->IFB_DefunctStat;   //;? wRONG, RC IS not AN ERROR CODE IN THE REST OF THIS ROUTINE, USE A "free" BIT
    } else if ( //ifbp->IFB_DefunctStat == HCF_SUCCESS  		&& 
		 ifbp->IFB_CardStat & CARD_STAT_PRESENT 	&& 
		 IN_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_0) == HCF_MAGIC ) {	/* 4 */
		rc = 0;																								/* 6 */
		do {    //mis-indentation to cater for HCF-light layout                         	    	//<HCFL>	/* 8 */
#if 1 //uncouple Rx and Tx for BeOS		
		if ( ifbp->IFB_RxFID ) {								    											/*10 */
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, HREG_EV_RX );
			HCFLOG( ifbp, HCF_LOG_SERVICE_NIC, 0x0001, ifbp->IFB_RxFID );
			ifbp->IFB_RxFID = ifbp->IFB_RxLen = ifbp->IFB_RxStat = 0;
		}
#endif 	//uncouple Rx and Tx for BeOS			
		sample = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT );												/*12*/
		HCFLOG( ifbp, HCF_LOG_SERVICE_NIC, 0x0002, sample );
		rc |= sample;
		
		if ( sample & HREG_EV_INFO_DROP )	ifbp->IFB_HCF_Tallies.NoBufInfo++;						//<HCFL>
		if ( sample & HREG_EV_INFO )		isr_info( ifbp );
#if defined HCF_TX_ERR_NOTIFY && !defined HCF_MB_OFF
 		if ( sample & (HFS_TX_EX | HFS_TX_OK) ) {		/* Note: Acknowledgement is handled regardless of HCF_TX_ERR_NOTIFY and 
												 * HCF_MB_OFF at point 14 below									*/
			ifbp->IFB_MB_FID = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_TX_COMPL_FID );	//get the FID and set I/F to put_info_mb
			put_info_mb( ifbp, CFG_TX_ERR_NOTIFY, NULL, HFS_DAT_ABS/2 + 1 );			  //;?check this length
		}		
#endif // HCF_TX_ERR_NOTIFY && ! HCF_MB_OFF
		
		if ( sample & HREG_EV_ALLOC ) {
#if 0																								//<HCFL>
            ifbp->IFB_PIFRscInd = 1;
#else																								//<HCFL@
			tmp = IN_PORT_WORD(ifbp->IFB_IOBase + HREG_ALLOC_FID );
			if ( tmp == ifbp->IFB_PIF_FID ) ifbp->IFB_PIFRscInd = 1;											/*13 */
			else if ( tmp == ifbp->IFB_DUIF_FID ) ifbp->IFB_DUIFRscInd = 1;
				else {
				HCFASSERT( tmp == ifbp->IFB_Notify_FID, tmp )
				ifbp->IFB_NotifyRscInd = 1;
				}
#endif																								//@HCFL>
		}
		tmp = sample & (hcf_16)~(HREG_EV_RX | HREG_EV_CMD );													/*14 */
		if ( tmp ) OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, tmp );
		
		sample = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT );												/*16 */
		if ( sample & HREG_EV_RX ) {
			ifbp->IFB_RxFID = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_RX_FID);
			HCFLOG( ifbp, HCF_LOG_SERVICE_NIC, 0x0004, ifbp->IFB_RxFID );
			HCFASSERT( ifbp->IFB_RxFID, 0 )
			(void)hcfio_string(ifbp, BAP_1, ifbp->IFB_RxFID, HFS_STAT_ABS, (wci_bufp)&ifbp->IFB_RxStat, 1, 2, IO_IN );
			//;? IF hcfio_string fails, stat is garbage with all consequences thereof
			stat = ifbp->IFB_RxStat & (HFS_STAT_MSG_TYPE | HFS_STAT_ERR);							//<HCFL>
			(void)hcfio_string(ifbp, BAP_1, ifbp->IFB_RxFID, HFS_DAT_LEN_ABS, (wci_bufp)&ifbp->IFB_RxLen, 1,2,IO_IN );
			ifbp->IFB_RxLen += HFS_DAT;
			HCFASSERT( ifbp->IFB_RxLen <= HCF_MAX_MSG + sizeof(snap_header), ifbp->IFB_RxLen )
			if ( ( stat & HFS_STAT_ERR ) == 0 ) {													//<HCFL@	/*18 */
#if !defined HCF_MB_OFF
				if ( stat == HFS_STAT_WMP_MSG ) {																/*20 */
					sample = HREG_EV_DUIF_RX;
					rc |= HREG_EV_DUIF_RX | HREG_EV_RX;
					(void)hcfio_string(ifbp, BAP_1, ifbp->IFB_RxFID, HFS_WDS_TYPE_ABS, (wci_bufp)&type, 0,2,IO_IN );
														// if type filtering is not explicitely disabled 
					 if ( ( type & 0x80 ) == 0  || ( ifbp->IFB_EngFlg & 0x0001 ) ) {		//;?what has EngFlg here to do
	 					ifbp->IFB_MB_FID = ifbp->IFB_RxFID;
						put_info_mb( ifbp, CFG_WMP, NULL, (hcf_16)DIV_BY_2(ifbp->IFB_RxLen + HFS_ADDR_DEST_ABS + 2) );  //;?check this length
					}
				} else
#endif // HCF_MB_OFF
				if ( ifbp->IFB_FrameType == HCF_ACT_802_3 ) {											/*22 */
					ifbp->IFB_RxFence = 0;
					if ( stat == HFS_STAT_1042 ) {
						(void)hcfio_string(ifbp, BAP_1, ifbp->IFB_RxFID, HFS_TYPE_ABS, (wci_bufp)&type, 0, 2, IO_IN );
						if ( enc_test( &type ) <= sizeof(enc_trans_tbl)/sizeof(enc_trans_tbl[0]) ) {
							stat = 0;
						}
					}
					if ( stat == HFS_STAT_TUNNEL || stat == HFS_STAT_1042 ) {
						ifbp->IFB_RxLen -= (HFS_TYPE - HFS_LEN);
						ifbp->IFB_RxFence = HFS_LEN_ABS;
					}
				}
			}																						//@HCFL>
		}
		} while ( sample & HREG_EV_DUIF_RX );   //mis-indentation to cater for HCF-light layout     //<HCFL>	/* 24 */
	}
	HCFASSERT( !( rc & ~( HREG_EV_TICK | HREG_EV_RES | HREG_EV_INFO_DROP | HREG_EV_DUIF_RX | 
							HREG_EV_NO_CARD | HREG_EV_INFO | HREG_EV_CMD | HREG_EV_ALLOC | HREG_EV_RX ) ), rc )  	//<HCFL>
	HCFASSERTLOGEXIT( HCF_ASSERT_SERVICE_NIC )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_SERVICE_NIC, -1 );
	return rc;
}/* hcf_service_nic */



/**************************************************************************************************************
************************** H C F   S U P P O R T   R O U T I N E S ********************************************
**************************************************************************************************************/



/*FF***************************************************************************************************************
FUNCT: alloc                                                                                     2000/02/04 14:15
USERS: hcf_initialize                                   
CALLS: HCF_DEBUG_TRIGGER                                cmd_wait                                         
       IN_PORT_WORD                                     OUT_PORT_WORD                                    
       hcfio_string                                     DIV_BY_2                                         
       HCFASSERT                                        
DEFIN: BAP_0                                            HCF_ASSERT_MISC                                  
       HCMD_ALLOC                                       HREG_ALLOC_FID                                   
       HREG_EV_ACK                                      HREG_EV_ALLOC                                    
       HREG_EV_STAT                                     IO_OUT                                           
GLOBL: HCF_SUCCESS                                      ifbp->IFB_HCF_Tallies.MiscErr                    
PARAM: ifbp                                             ifbp->IFB_IOBase                                 
       ifbp->IFB_TickIni                                len                                              
LOCAL: fid                                              prot_cnt                                         
       zero                                             
******************************************************************************************************************/
/******************************************************************************************************************


.MODULE			alloc
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCFR routines
.DESCRIPTION	allocates a (TX or Notify) FID in NIC RAM and clears it

.ARGUMENTS
  hcf_16 alloc( IFBP ifbp, int len )

.RETURNS
  0:	failure
  <>0:	fid value

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  len	length in bytes of FID to be allocated

  ASSERTS:                                                                                  //<HCFL@
  o fid == 0, which indicates cmd_wait failed or the alloc event did not occur in time
  o prot_cnt == 0, which indicates the alloc event did not occur in time                    //@HCFL>


.DIAGRAM
 1:	execute the allocate command by calling cmd_wait
 2: wait till either the alloc event or a time-out occures
 3: if the alloc event occures,
 	- read the FID to return it to the caller of alloc
 	- acknowledge the alloc event
 	- clear the storage allocated in NIC RAM (see notice below)
 4: The return status of alloc is used to carry the fid-value. This, together with the observation that
	already multiple successful interactions (e.g. hcf_get_info() ) with the Hermes took place before 
	alloc() is called, warrant the ignoring of the return status of hcfio_string.
 8:	since alloc is only called after an Hermes initialize command but before the Hermes enable, a failing
	allocation is considered a H/W failure, hence the Miscellaneous Error tally is incremented
	
.NOTICE
 o  Clearing the FID is not only an aesthetical matter, it is also the cheapest (code-size) way to enforce
 	  -	correlation between IFB_TxCntl (which is cleared by hcf_disable) and the field HFS_TX_CNTL_ABS of a
 		IFB_PIF_FID
 	  -	zero value of the field HFS_TX_CNTL_ABS of a IFB_DUIF_FID (hcf_send_diag_msg only supports port 0)
 	
.DIAGRAM
.ENDOC				END DOCUMENTATION
*/
/*-----------------------------------------------------------------------------------------------------------*/
hcf_16 alloc( IFBP ifbp, int len ) {

hcf_32 prot_cnt = ifbp->IFB_TickIni;
hcf_16 fid		= 0;
hcf_16 zero		= 0;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0004, len );
	if ( cmd_wait( ifbp, HCMD_ALLOC, len ) == HCF_SUCCESS ) {											/* 1 */
		while ( prot_cnt ) {                                                       						/* 2 */
			prot_cnt--;
			if ( IN_PORT_WORD(ifbp->IFB_IOBase + HREG_EV_STAT ) & HREG_EV_ALLOC ) {						/* 3 */
				fid = IN_PORT_WORD(ifbp->IFB_IOBase + HREG_ALLOC_FID );			
				OUT_PORT_WORD(ifbp->IFB_IOBase + HREG_EV_ACK, HREG_EV_ALLOC );
				(void)hcfio_string( ifbp, BAP_0, fid, 0, (wci_bufp)&zero, 0, 2, IO_OUT );				/* 4 */
//				(void)hcfio_string( ifbp, BAP_0, fid, 0, NULL, 0, 0, IO_OUT ); BAD idea, does not wait for BAP busy drop
				len = DIV_BY_2( len );
				while ( --len ) OUT_PORT_WORD( ifbp->IFB_IOBase + BAP_0, 0 );
//				while ( len-- ) OUT_PORT_WORD( ifbp->IFB_IOBase + BAP_0, 0 );
				break;
			}
		}
	}
	if ( prot_cnt == 0 ) ifbp->IFB_HCF_Tallies.MiscErr++;									//<HCFL>	/* 8 */
	HCFASSERT( prot_cnt, len )
	HCFASSERT( fid, len )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0004, -1 );
	return fid;
}/* alloc */


//<HCFL@/
/*FF***************************************************************************************************************
FUNCT: hcf_aux_cntl                                                                              2000/02/04 14:15
USERS: hcf_get_info                                     download                                         
CALLS: OUT_PORT_WORD                                    IN_PORT_WORD                                     
DEFIN: AUX_MAGIC_0                                      AUX_MAGIC_1                                      
       AUX_MAGIC_2                                      CARD_STAT_DEFUNCT                                
       HREG_CNTL                                        HREG_CNTL_AUX_DIS_STAT                           
       HREG_CNTL_AUX_DSD                                HREG_CNTL_AUX_ENA                                
       HREG_CNTL_AUX_ENA_CNTL                           HREG_CNTL_AUX_ENA_STAT                           
       HREG_PARAM_0                                     HREG_PARAM_1                                     
       HREG_PARAM_2                                     
GLOBL: HCF_ERR_DEFUNCT_AUX                              HCF_SUCCESS                                      
       ifbp->IFB_HCF_Tallies.MiscErr                    
PARAM: cmd                                              ifbp                                             
       ifbp->IFB_CardStat                               ifbp->IFB_DefunctStat                            
       ifbp->IFB_IOBase                                 ifbp->IFB_TickIni                                
LOCAL: io_port                                          prot_cnt                                         
       rc                                               
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			hcf_aux_cntl
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION	Prepares/terminates access via AUX-mechanism to the NIC RAM.

.ARGUMENTS
  int hcf_aux_cntl( IFBP ifbp, int cmd )

.RETURNS
	HCF_SUCCESS	
	HCF_ERR_DEFUNCT_AUX	
	
.NARRATIVE
 
 Parameters:
  ifbp	address of the Interface Block
  cmd
 		HREG_CNTL_AUX_ENA_CNTL	enables AUX-port access mechanism
 		HREG_CNTL_AUX_DIS_CNTL	disables ,,  ,,     ,,      ,,
 		
  Description:
	Prepares/terminates access via AUX-mechanism to the NIC RAM.
	
.DIAGRAM

 5:	the code "OUT_PORT_WORD( i, IN_PORT_WORD( i ) & ~HREG_CNTL_AUX_ENA | HREG_CNTL_AUX_ENA_CNTL );"
 	could just as well have been written as:
				t = IN_PORT_WORD( i ) & ~HREG_CNTL_AUX_ENA;
				OUT_PORT_WORD( i, t | HREG_CNTL_AUX_ENA_CNTL );
	The compiler is in "default release" configuration (minimal size, full optimization)
	clever enough to see that in the "clear" form, t is overwritten with TickIni after
	its only use in OUT_PORT_WORD so it is not saved in [bp-2] (which happens to be
	the location of t).  However the compiler moves the value of t into register cx
	and out again for no good reason. In the obscure form the compiler leaves the
	value of t in register ax throughout the process.  I guess this is typical Micro$oft.
	

	
.ENDOC				END DOCUMENTATION
*/
/*-----------------------------------------------------------------------------------------------------------*/
#if !defined HCF_DOWNLOAD_OFF /*;?  || !defined HCF_MB_OFF */
int hcf_aux_cntl( IFBP ifbp, hcf_16 cmd ) {

int		rc = HCF_SUCCESS;
hcf_32	prot_cnt = ifbp->IFB_TickIni;
hcf_io	io_port = ifbp->IFB_IOBase + HREG_CNTL;

	OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_0, AUX_MAGIC_0 );
	OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_1, AUX_MAGIC_1 );
	OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_2, AUX_MAGIC_2 );
	OUT_PORT_WORD( io_port, 
				   ( IN_PORT_WORD( io_port ) & (hcf_16)~(HREG_CNTL_AUX_ENA|HREG_CNTL_AUX_DSD) ) | cmd );/* 5 */
	cmd = (hcf_16)(cmd & HREG_CNTL_AUX_ENA_CNTL ? HREG_CNTL_AUX_ENA_STAT : HREG_CNTL_AUX_DIS_STAT);
	while ( prot_cnt && (IN_PORT_WORD( io_port ) & HREG_CNTL_AUX_ENA) != cmd ) prot_cnt--;
	if ( prot_cnt == 0 ) {
		ifbp->IFB_HCF_Tallies.MiscErr++;
		rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_AUX;
		ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
	}
	return rc;
}/* aux_cntl */
#endif //!defined HCF_DOWNLOAD_OFF /*;?  || !defined HCF_MB_OFF */
//@HCFL>



/*FF***************************************************************************************************************
FUNCT: calibrate                                                                                 2000/02/04 14:15
USERS: hcf_initialize                                   
CALLS: OUT_PORT_WORD                                    IN_PORT_WORD                                     
       max                                              HCFLOG                                           
       HCFASSERT                                        
DEFIN: CARD_STAT_DEFUNCT                                DO_ASSERT                                        
       HCF_LOG_DEFUNCT                                  HCF_PROT_TIME                                    
       HREG_EV_ACK                                      HREG_EV_STAT                                     
       HREG_EV_TICK                                     INI_TICK_INI                                     
GLOBL: HCF_ERR_DEFUNCT_TIMER                            HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_CardStat                               
       ifbp->IFB_DefunctStat                            ifbp->IFB_IOBase                                 
       ifbp->IFB_TickIni                                
LOCAL: cnt                                              prot_cnt                                         
       rc                                               
******************************************************************************************************************/
/******************************************************************************************************************

.MODULE			calibrate
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCFR routines
.DESCRIPTION	calibrates the S/W protection counter against the Hermes Timer tick

.ARGUMENTS
  int calibrate( IFBP ifbp )
	
.RETURNS
	HCF_SUCCESS	
	HCF_ERR_DEFUNCT_TIMER	

.NARRATIVE
 
 Parameters:
  ifbp	address of the Interface Block
 		
  Description: calibrates the S/W protection counter against the Hermes Timer tick
	IFB_TickIni is the value used to initialize the S/W protection counter in a way which makes the
	expiration period more or less independent of the processor speed. If IFB_TickIni is not yet 
	calibrated, it is done now. This calibration is "reasonably" accurate because the Hermes is in a 
	quiet state as a result of the Initialize command. 
	
.DIAGRAM
	
 1:	IFB_TickIni is initialized at INI_TICK_INI by hcf_connect. If calibrate fails, IFB_TickIni is changed
 	to 0, If calibrate succeeds, it is garuanteed to be changed to a non-zero value different of 
 	INI_TICK_INI. As a consequence there will be exactly 1 shot at calibration per hcf_connect (see also
 	the NOTICE below)
 2:	Initiliaze the "maximum" value. Calibration is done 10 times, to diminish the effects of jitter and 
 	interence. Each of the 10 cycles is limited to at most INI_TICK_INI samples if the TimerTick status
 	of the Hermes. Since the start of calibrate is unrelated to the Hermes Internal Timer, the first 
 	interval may last from 0 to the normal interval, all subsequent intervals should be the full length of 
 	the Hermes Tick interval. The Hermes Timer Tick is not reprogrammed by the HCF, hence it is running
 	at the default of 10 k microseconds.
 3: Acknowledge the Timer Tick Event. For the first time through the loop there may or there may not be 
 	a Timer Tick Event, this does not influence the observation made under 2 above.
 4:	If a Timer Tick Event occured, update the "maximum" value accordingly. 
	If no Timer Tick Event occured before the proterction counter expired, reset IFB_TickIni to zero and
	exit the calibrate routine, thus rendering the Hermes inoperable, as explained in the NOTICE below.
	To cope with the situation that the Timer Tick Event is ALWAYS present (e.g. due to an error, we read
	0xFFFF continuously), prot_cnt is checked for zero.
 8:	Set the final protection timer represented by IFB_TickIni at the HCFCFG.H defined HCF_PROT_TIME 
 	(default 256) times the 10 k microseconds (default approximately 2.5 seconds)
.NOTICE
 o  Although there are a number of viewpoints possible, calibrate() uses now as error strategy that a single
	failure of the Hermes TimerTick is considered fatal. By resetting IFB_TickIni to zero in case of such a
	failure, neither cmd_wait nor hcfio_string will be able to exceute
 o  The Hermes does not restart nor reset its timing logic at an Initialize command. This implies that the
 	very first timing period measured after an Initialize command may be anything between 0 and whatever
 	the Hermes Timer was programmed by means of CFG_TICK_TIME. In the past the HCF programmed the Hermes
 	timer to 1 second. This definitely jeopardizes the calibration as implemented in calibrate(). 
 	By executing calibrate() only once in the lifetime of the driver would (supposedly) solve the problem.
 	This solution is not robust, due to the fragilIty of the Hermes implementation of the Initialize, Disable 
 	and Enable commands as well as due to MSF sequences of hcf_connect, hcf_disconnect.
 o	There is no hard and concrete time-out value defined for Hermes activities. The 2.5 seconds is believed
 	to be sufficiently "relaxed" for real life and to be sufficiently short to be still usefull in an 
 	environment with humans.
 o	To diminish the chance that in a pre-emptive environment IFB_TickIni is calibrated too low because the HCF
	just happens to loose control during this calibration, the calibration is performed 10 times and the
	largest value is used.
 o	In case of failure, IFB_TickIni ends up as INI_TICK_INI, which renders the HCF inoperable
 	
 	
.ENDOC				END DOCUMENTATION
*/
/*-----------------------------------------------------------------------------------------------------------*/
int calibrate( IFBP ifbp ) {

int		cnt = 10;
int		rc = HCF_SUCCESS;
hcf_32	prot_cnt;

	if ( ifbp->IFB_TickIni == INI_TICK_INI ) {															/* 1 */
		ifbp->IFB_TickIni = 0;																			/* 2 */
			while ( rc == HCF_SUCCESS && --cnt ) {
				prot_cnt = 0;
				OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, HREG_EV_TICK );							/* 3 */
				while ( (IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT ) & HREG_EV_TICK) == 0 &&
						++prot_cnt < INI_TICK_INI ) /*NOP*/;
				if ( prot_cnt && prot_cnt != INI_TICK_INI ) {											/* 4 */
					ifbp->IFB_TickIni = max( ifbp->IFB_TickIni, prot_cnt);
				} else {
					ifbp->IFB_TickIni = 0;
					rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_TIMER;
					HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
					ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
					HCFASSERT( DO_ASSERT, 0 )
				}
			}
		ifbp->IFB_TickIni *= HCF_PROT_TIME;																/* 8 */
	}
	return rc;
} /* calibrate */



//<HCFL@
/*FF***************************************************************************************************************
FUNCT: check_comp                                                                                2000/02/04 14:15
USERS: hcf_initialize                                   
CALLS: hcf_get_info                                     CNV_LITTLE_TO_INT                                
GLOBL: HCF_SUCCESS                                      p->variant.bottom                                
       p->variant.number                                p->variant.top                                   
PARAM: ifbp                                             p                                                
       p->len                                           type                                             
LOCAL: cnt                                              i                                                
       x                                                x.len                                            
       x.typ                                            x.variant.bottom                                 
       x.variant.number                                 x.variant.top                                    
******************************************************************************************************************/
/******************************************************************************************************************

.MODULE			check_comp
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION	Checks compatibility of the HCF as actor with a specific the NIC supplier feature

.ARGUMENTS
  int check_comp( IFBP ifbp, CFG_RANGES_STRCT *p, hcf_16 type )
	
.RETURNS
  0		incompatible
  <>0	low order byte: Bottom		high order byte: High

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  *p	address of the actor specification
  type	supplier specification

 Description: Check_comp is a support routine to check the compatibility of the HCF as actor 
  with a specific feature supplied by the NIC
  It is used for the following features
  o cfg_drv_act_range_pri
  o cfg_drv_act_range_hsi
  o cfg_drv_act_range_sta

 Remarks:
  o For backward compatibilty with NICs which do not support the HSISupRange LTV, the HCF will fake an
  	HSISupRange if the Hermes indicates "not available"

.DIAGRAM
 1:	0xFFFF as initial value for the unsigned variable i does garuantee that if the "else" part is not
 	executed because the Hermes does not support the LTV, the return value of check_comp is false.
 2:	The supplier information is requested by building an LTV with the appropriate Type field (as specified
 	by parameter type) is build and passing this LTV to hcf_get_info
 4:	Only in the case of HSISupRange, the value of i is changed from 0xFFFF to 0, garuanteeing that the return
 	value of check_comp changes from false to true. This is done to resolve lack of support of this particular
 	feature by old NICs.
	As a negative consequence of this choice to cope with old NICs, this logic to fake acceptance of those old
	NICs, must be removed when the Bottom Compatibilty is raised above 1.
	The "dynamic" approach to replace the statement "if ( type == CFG_HSI_SUP_RANGE ) i = 0;" with
	"if ( type == CFG_HSI_SUP_RANGE && cfg_drv_sup_range_hsi.variant[0].bottom == 1 ) i = 0;"
	is not suitable because cfg_drv_sup_range_hsi does not exist when MSF_COMPONENT_ID is not defined.
 6:	Each of the actor variants is checked against the (single) supplier bottom-top range till either an
 	acceptable match is found or all actor variants are tried 	
 8: depending on whether a match was found or not (or i manipulated in case of a non-supported LTV),
 	true or false is returned
 	
	If the primary firmware does not supply this RID or supplies the "old" HardwareStructure Info, the
	Primary Compatibility check is skipped. These conditions are recognized based on the length field supplied
	by the Hermes.
 	
 	
.ENDOC				END DOCUMENTATION
*/
/*-----------------------------------------------------------------------------------------------------------*/
hcf_16 check_comp( IFBP ifbp, CFG_RANGES_STRCT *p, hcf_16 type ) {

CFG_RANGES_STRCT	  x;
hcf_16				  i = 0xFFFF;																		/* 1 */
hcf_16				  cnt;

	x.len = 6;																							/* 2 */
	x.typ = type;
	x.variant[0].number = x.variant[0].top = x.variant[0].bottom = 0;
	
	if ( hcf_get_info( ifbp, (LTVP)&x ) == HCF_SUCCESS ) {
#if HCF_HSI_VAR != 0					// do not fake HSI for Controlled Deployment 
										// when Primary Variant 1 is no longer supported, faking can be removed
		if ( x.len == 0 && type == CFG_HSI_SUP_RANGE ) {
			x.variant[0].number = x.variant[0].top = x.variant[0].bottom = CNV_INT_TO_LITTLE(1);		/* 4 */
		}
#endif //HCF_HSI_VAR
		for ( cnt = 0, i = 4; i < p->len; cnt++, i += 3 ) {												/* 6 */
			if ( p->variant[cnt].number == CNV_LITTLE_TO_INT(x.variant[0].number)	&&
				 p->variant[cnt].bottom <= CNV_LITTLE_TO_INT(x.variant[0].top)	 	&&
				 p->variant[cnt].top    >= CNV_LITTLE_TO_INT(x.variant[0].bottom)
			   ) break;
		}
	}
	if ( i >= p->len ) i = 0;																			/* 8 */
	else i = CNV_LITTLE_TO_INT(x.variant[0].bottom) | (CNV_LITTLE_TO_INT(x.variant[0].top) << 8);
	
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC|0x1000, i );
	
	
	return i;
}/* check_comp */
//@HCFL>




/*FF***************************************************************************************************************
FUNCT: cmd_wait                                                                                  2000/02/04 14:15
USERS: hcf_action                                       hcf_disable                                      
       hcf_enable                                       hcf_get_info                                     
       hcf_send                                         hcf_send_diag_msg                                
       alloc                                            download                                         
       ini_hermes                                       put_info                                         
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       IN_PORT_WORD                                     HCFLOG                                           
       OUT_PORT_WORD                                    
DEFIN: CARD_STAT_DEFUNCT                                DO_ASSERT                                        
       HCF_ASSERT_MISC                                  HCF_LOG_CMD_WAIT                                 
       HCF_LOG_DEFUNCT                                  HCMD_BUSY                                        
       HCMD_CMD_CODE                                    HCMD_INI                                         
       HCMD_PROGRAM                                     HREG_CMD                                         
       HREG_EV_ACK                                      HREG_EV_CMD                                      
       HREG_EV_STAT                                     HREG_PARAM_0                                     
       HREG_RESP_0                                      HREG_STAT                                        
       HREG_STAT_CMD_CODE                               
GLOBL: HCF_ERR_CMD                                      HCF_ERR_DEFUNCT_CMD_SEQ                          
       HCF_ERR_TIME_OUT                                 HCF_SUCCESS                                      
       ifbp->IFB_HCF_Tallies.MiscErr                    
PARAM: cmd_code                                         ifbp                                             
       ifbp->IFB_CardStat                               ifbp->IFB_DefunctStat                            
       ifbp->IFB_ErrCmd                                 ifbp->IFB_ErrQualifier                           
       ifbp->IFB_IOBase                                 ifbp->IFB_TickIni                                
       par_0                                            
LOCAL: prot_cnt                                         rc                                               
       stat                                             
******************************************************************************************************************/
/**************************************************************************************************************


.MODULE			cmd_wait
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCFR routines
.DESCRIPTION	Executes synchronous Hermes Command and waits for Command Completion

.ARGUMENTS
  int cmd_wait( IFBP ifbp, int cmd_code, int par_0 )

.RETURNS
	IFB_DefunctStat	
	HCF_ERR_TIME_OUT	
	HCF_ERR_DEFUNCT_CMD_SEQ	
	-- HCF_ERR_INQUIRE	
	HCF_SUCCESS	
	HCF_ERR_TO_BE_ADDED	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	
.NARRATIVE

 Parameters:
  ifbp		address of the Interface Block
  cmd_code	
  par_0 	
 		
  Description: Executes synchronous Hermes Command and waits for Command Completion
  
.DIAGRAM

 1:	the test on rc checks whether a BAP initialization or a call to cmd_wait did ever fail. If so, the Hermes
	is assumed inoperable/defect, and all subsequent bap_ini/cmd_wait calls are nullified till hcf_disable
	clears the IFB_DefunctStat field.
 	
 2:	Based on the Hermes design, the read of the busy bit is superfluous because we wait for the Cmd bit in
	the Event Status register.

 3:	If Status register and command code don't match either the Hermes and Host are out of sync ( a fatal
 	error) and/or error bits are reported via the Status Register. The latter is explicitly ignored, except for:
 	- diagnose (does not use cmd_wait,	pseudo-asynchronous command which is given in-line in hcf_action 
 	  and the respons is sampled in get_info_mb)
 	- non-volatile programming command (pseudo-asynchronous command like diagnose)
 	
 	Errors reported via the Status Register should be caused by sequence violations in Hermes command 
 	sequences and hence these bugs should have been found during engineering testing. Since there is no 
 	strategy to cope with this problem, it might as well be ignored at run time.Note that for any 
 	particular situation where a strategy is formulated to handle the consequences of a particular bug 
 	causing a particular Error situation reported via the Status Register, the bug should be removed 
 	rather than adding logic to cope with the consequences of the bug.
	There have been HCF versions where an error report via the Status Register even brought the HCF in
	defunct mode (although it was not yet named like that at that time). This is particular undesirable
	behavior for a general library. A call of hcf_get_info with a non-existing RID type, already causes 
	an error report in the Status Register
	Simply reporting the error (as "interesting") is debatable. There also have been HCF versions with this
	strategy using the "vague" HCF_FAILURE code,
	An exercition to categorizing the return codes, caused this strategy to be changed to report this 
	situation with its own reurn code (HCF_ERR_CMD). Not that there have been HCFs which ignored this situation

 5:	When the Hermes reports on another command than the Host just issued, the two are apparently out of
 	sync and all bets are off about the consequences. Therefore this situation is treated the same as an
 	Hermes failure as indicated by time-out (blocking all further bap_ini/cmd_wait calls till hcf_disable.


	
.NOTICE
	Due to the general HCF strategy to wait for command completion, a 2nd command can never be excuted
	overlapping a previous command. As a consequence the Hermes requirement that no Inquiry command may be
	executed if there is still an unacknowledged Inquiry command outstanding, is automatically met.
	However, there are two pseudo-asynchronous commands (Diagnose and Download) which do not adhere to this
	general HCF strategy. In that case we rely on the MSF to do not overlap these commands, but no protection
	is offered by the HCF
	Note that this I/F violation by the MSF is catched by the HCFASSERT mechanism
.ENDOC				END DOCUMENTATION
*/
/* -------------------------------------------------------------------------------------------------------------------*/

int cmd_wait( IFBP ifbp, hcf_16 cmd_code, int par_0 ) {

int		rc = ifbp->IFB_DefunctStat;
hcf_32	prot_cnt = ifbp->IFB_TickIni;
hcf_16	stat;

    //<HCFL@
	//although the ASSERT below makes the equivalent ASSERTS at the individual hcf_functions superfluous
	//they are also present at (most of) the individual functions to ease the ASSERT interpretation
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0008, cmd_code );
	HCFASSERT( ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == 0 ||
			   ( ( cmd_code & HCMD_CMD_CODE ) == HCMD_PROGRAM &&
				 ( ifbp->IFB_CardStat & HCMD_CMD_CODE ) == HCMD_PROGRAM
			   ),
			   ifbp->IFB_CardStat
			 )
	//assure we are not sneaking up on someone
	HCFASSERT( (IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) & HCMD_BUSY) == 0, cmd_code	)
	HCFASSERT( (IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) & HCMD_BUSY) == 0,
			    IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD )								)
	//@HCFL>

	if ( rc == HCF_SUCCESS ) {																				/* 1 */
		HCFLOG( ifbp, HCF_LOG_CMD_WAIT, cmd_code, par_0 );
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_0, par_0 );
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_CMD, cmd_code );
		if ( cmd_code == HCMD_INI ) prot_cnt <<= 3;
		while (prot_cnt && (IN_PORT_WORD(ifbp->IFB_IOBase + HREG_EV_STAT) & HREG_EV_CMD) == 0 ) prot_cnt--;	/* 2 */
		stat = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_STAT );
		HCFLOG( ifbp, HCF_LOG_CMD_WAIT, IN_PORT_WORD(ifbp->IFB_IOBase + HREG_EV_STAT), stat );  //;?sub_optimal, EV_STAT could have changed
		if ( prot_cnt == 0 ) {
			ifbp->IFB_HCF_Tallies.MiscErr++;																//<HCFL>
			rc = HCF_ERR_TIME_OUT;
			HCFASSERT( DO_ASSERT, cmd_code )
		} else {
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, HREG_EV_CMD );
			if ( stat != ( cmd_code & HREG_STAT_CMD_CODE ) ) {												/* 3 */
				rc = HCF_ERR_CMD;
				ifbp->IFB_ErrCmd = stat;
				ifbp->IFB_ErrQualifier = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_RESP_0 );
				if ( (stat ^ cmd_code) & HREG_STAT_CMD_CODE ) {												/* 5 */
					rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_CMD_SEQ;
					HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
					ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
				}
				ifbp->IFB_HCF_Tallies.MiscErr++;															//<HCFL>
				HCFASSERT( DO_ASSERT, cmd_code )
				HCFASSERT( DO_ASSERT, ifbp->IFB_ErrCmd )
				HCFASSERT( DO_ASSERT, ifbp->IFB_ErrQualifier )
			}
		}
        //assure no one sneaked up on us
		HCFASSERT( IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) == (hcf_16)cmd_code, cmd_code )
		HCFASSERT( IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) == (hcf_16)cmd_code,
				   IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD )						)
        //@HCFL>
	}
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0008, -1 );
	return rc;
}/* cmd_wait */


//<HCFL@
/*FF***************************************************************************************************************
FUNCT: download                                                                                  2000/02/04 14:15
USERS: hcf_put_info                                     
CALLS: hcf_initialize                                   hcf_aux_cntl                                     
       MUL_BY_2                                         OUT_PORT_WORD                                    
       cmd_wait                                         download_data                                    
       HCFASSERT                                        HCFASSERTLOGEXIT                                 
DEFIN: CFG_DLNV_DATA                                    CFG_DLNV_START                                   
       CFG_DL_STOP                                      DO_ASSERT                                        
       HCF_ASSERT_PUT_INFO                              HCMD_CMD_CODE                                    
       HCMD_PROGRAM                                     HCMD_PROGRAM_DISABLE                             
       HCMD_PROGRAM_ENABLE_NON_VOLATILE                 HCMD_PROGRAM_NON_VOLATILE                        
       HREG_CMD                                         HREG_CNTL_AUX_DIS_CNTL                           
       HREG_CNTL_AUX_ENA_CNTL                           HREG_PARAM_1                                     
       HREG_PARAM_2                                     INI_COMPLETE                                     
       INI_PARTIAL                                      
GLOBL: HCF_ERR_LEN                                      HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_CardStat                               
       ifbp->IFB_DLLen                                  ifbp->IFB_DLOffset                               
       ifbp->IFB_DLPage                                 ifbp->IFB_DLTarget                               
       ifbp->IFB_IOBase                                 ltvp                                             
       ltvp->len                                        ltvp->typ                                        
       ltvp->val                                        
LOCAL: cnt                                              i                                                
       j                                                rc                                               
******************************************************************************************************************/
/***********************************************************************************************************************

.MODULE			download
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	

.ARGUMENTS
  int download( IFBP ifbp, LTVP ltvp )

.RETURNS
	-- value --						-- parameter --
	>>hcf_initialize (INI_PARTIAL)	CFG_DLV_START (on AP only)
	>>hcf_aux_cntl					CFG_DLV_START (on AP only)
	>>cmd_wait						CFG_DLV_START (on AP only)
	HCF_SUCCESS						CFG_DLV_ADDR (on AP only)
	>>download_data					CFG_DLV_DATA (on AP only)
	>>hcf_initialize (INI_PARTIAL)	CFG_DLNV_START (on station only)
	>>hcf_aux_cntl					CFG_DLNV_START (on station only)
	HCF_ERR_LEN						CFG_DLNV_DATA (on station only)
	>>cmd_wait						CFG_DLNV_DATA (on station only)
	>>download_data					CFG_DLNV_DATA (on station only)
	>>cmd_wait						CFG_DL_STOP
	>>hcf_aux_cntl					CFG_DL_STOP
	>>hcf_initialize (INI_COMPLETE)	CFG_DL_STOP

.NARRATIVE

 Parameters:
	ifbp		address of the Interface Block
	ltvp

 Remarks: To get the contents of the IntEn register changed is a two step process:

 	o change the "shadow" in IFB_IntEnMask

 	o call hcf_action to actually copy the shadow to the IntEn register.

 	To prevent a change in the "Card Interrupt En/Disabled state, a balancing
 	pair of HCF_ACT_INT_OFF and HCF_ACT_INT_ON must be used. To prevent
 	a temporary enabling as undesirable side effect, the first call must be
 	HCF_ACT_INT_OFF.
 	Note that at the very first interrupt, hcf_service_nic causes the removal of
 	the Tick and Cmd bit in the IntEn register.

    //<HCFL@
19:	if HCF_ASSERT is not defined, the else clause absorbs the "return rc;" statement. This is plainly not the
	intention, hence the semi-colon after the first HCFASSERT, so there will be at least an empty statement
	after the else.
	//@HCFL>
 	
 	
.DIAGRAM
***********************************************************************************************************************/
#if !defined HCF_DOWNLOAD_OFF
int download(IFBP ifbp, LTVP ltvp ) {

int			cnt = 3;
hcf_16		i = ltvp->len - 1;
int			rc = HCF_SUCCESS;
hcf_16		j;



	switch (ltvp->typ) {												// function dispatch table
/**********************************    V O L A T I L E    D O N W L O A D   **********************************/
#if defined HCF_AP
	  case CFG_DLV_START:												// Setup Download RAM
		/* this HCFASSERT never catches, the side effects are needed	*/
		HCFASSERT( ( ifbp->IFB_DLTarget[0] = ifbp->IFB_DLTarget[1] = 0 ) == 0, 0 )

		rc = hcf_initialize( ifbp, INI_PARTIAL );
		if ( rc == HCF_SUCCESS ) rc = hcf_aux_cntl( ifbp, HREG_CNTL_AUX_ENA_CNTL );
		if ( rc == HCF_SUCCESS ) {
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_1, ltvp->val[1] );
			rc = cmd_wait( ifbp, HCMD_PROGRAM_ENABLE_VOLATILE | HCMD_PROGRAM, ltvp->val[0] );
		}
		break;
	  case CFG_DLV_ADDR:												//Download RAM Addressing information
		ifbp->IFB_DLTarget[0] = ltvp->val[0];
		ifbp->IFB_DLTarget[1] = ltvp->val[1];
		break;
	  case CFG_DLV_DATA:												//Download RAM Data
		HCFASSERT( ifbp->IFB_DLTarget[0] | ifbp->IFB_DLTarget[1], 0 )
		
		rc = download_data( ifbp,ltvp, ifbp->IFB_DLTarget[0] & 0x007E, 
							(ifbp->IFB_DLTarget[0]>>7) + (ifbp->IFB_DLTarget[1]<<9) );
		break;
#endif // HCF_AP
/****************************      N O N - V O L A T I L E    D O N W L O A D   ******************************/
#if defined HCF_STA
	  case CFG_DLNV_START:												//Setup Download NV-RAM
		if ( ( ifbp->IFB_CardStat & HCMD_PROGRAM ) == 0 ) {												/* 4 */
			rc = hcf_initialize( ifbp, INI_PARTIAL );
			if ( rc == HCF_SUCCESS ) {
				ifbp->IFB_CardStat |= HCMD_PROGRAM;
				rc = hcf_aux_cntl( ifbp, HREG_CNTL_AUX_ENA_CNTL );
			}
		}
		ifbp->IFB_DLTarget[0] = ltvp->val[0];
		ifbp->IFB_DLTarget[1] = ltvp->val[1];
		break;
	  case CFG_DLNV_DATA:											//Download data
		j = MUL_BY_2( i );
		if ( j > ifbp->IFB_DLLen ) rc = HCF_ERR_LEN;
		else {
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_2, j );
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_PARAM_1, ifbp->IFB_DLTarget[1] );	//high order
//																						Target[0] low order
			rc = cmd_wait( ifbp, HCMD_PROGRAM_ENABLE_NON_VOLATILE | HCMD_PROGRAM, ifbp->IFB_DLTarget[0] );
			if ( rc == HCF_SUCCESS ) {
				rc = download_data( ifbp, ltvp, ifbp->IFB_DLOffset, ifbp->IFB_DLPage );
			}
			if ( rc == HCF_SUCCESS ) {
																			//artificially a-synchronous
				OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_CMD, HCMD_PROGRAM | HCMD_PROGRAM_NON_VOLATILE );
			}				
		}
		break;
#endif // HCF_STA
/***********************************      C O M M O N    D O N W L O A D   ***********************************/
	  case CFG_DL_STOP:														//Cleanup Download
		ifbp->IFB_CardStat &= (hcf_16)~HCMD_CMD_CODE;		//;?The purpose of this statement escapes me today (99feb05)
															//unless it is to keep cmd_wait happy (no ASSERTs etc)
															//Just to stay on the safe side, I'll leave the code in for now
															//At 99feb05 I added "ifbp->IFB_CardStat = CARD_STAT_PRESENT;"
															//to hcf_initialize, rendering this code superfluous anyway

		rc = cmd_wait( ifbp, HCMD_PROGRAM | HCMD_PROGRAM_DISABLE, 0 );
		cnt = hcf_aux_cntl( ifbp, HREG_CNTL_AUX_DIS_CNTL );
		if ( rc == HCF_SUCCESS ) rc = cnt;
//;?	cnt = hcf_disable( ifbp, HCF_PORT_ALL );	//to achieve a.o. cmd_wait( ifbp, HCMD_INI, NULL );
		cnt = hcf_initialize( ifbp, INI_COMPLETE );
		//;?restore Hermes Configuration <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		if ( rc == HCF_SUCCESS /*&& cnt != HCF_ERR_INCOMP_STA*/ ) rc = cnt;
		break;
	  default:
	  	HCFASSERT( DO_ASSERT, 0 )
         /*NOP*/;																                /* 19*/
	  	//!rc = HCF_SUCCESS;
	}
	HCFASSERT( rc == HCF_SUCCESS, rc )
	HCFASSERTLOGEXIT( HCF_ASSERT_PUT_INFO )  //;?why is this needed, why is this not handled by put_info itself
	return rc;
}/* download */



/*FF***************************************************************************************************************
FUNCT: download_data                                                                             2000/02/04 14:15
USERS: download                                         
CALLS: HCFASSERT                                        OUT_PORT_WORD                                    
       OUT_PORT_STRING                                  CNV_LITTLE_TO_INT                                
       IN_PORT_WORD                                     MUL_BY_2                                         
DEFIN: HREG_AUX_DATA                                    HREG_AUX_OFFSET                                  
       HREG_AUX_PAGE                                    
GLOBL: HCF_ERR_DEFUNCT_DL                               HCF_SUCCESS                                      
       ifbp->IFB_HCF_Tallies.EngCnt                     
PARAM: ifbp                                             ifbp->IFB_DLTarget                               
       ifbp->IFB_IOBase                                 ltvp                                             
       ltvp->len                                        ltvp->val                                        
       offset                                           page                                             
LOCAL: cnt                                              cp                                               
       io_port                                          j                                                
       len                                              rc                                               
       tl                                               wp                                               
******************************************************************************************************************/
/***********************************************************************************************************************

.MODULE			download_data
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	

.ARGUMENTS
  int download_data( IFBP ifbp, LTVP ltvp, hcf_16 offset, hcf_16 page )

.RETURNS
	HCF_SUCCESS	
	HCF_ERR_DEFUNCT_DL	DCWA catches 3 times

.NARRATIVE

 Parameters:
	ifbp		address of the Interface Block
	ltvp
	offset
	page
 	
.DIAGRAM
***********************************************************************************************************************/
int download_data( IFBP ifbp, LTVP ltvp, hcf_16 offset, hcf_16 page ) {

int			cnt		= 3;
hcf_16		len		= ltvp->len - 1;
wci_bufp	cp;
hcf_io		io_port;
hcf_16		j;
wci_recordp	wp;
int			rc;
hcf_32		tl;						//to ease the 32 bits calculation

	HCFASSERT( ifbp->IFB_DLTarget[0] | ifbp->IFB_DLTarget[1], 0 )
	do {
		rc = HCF_SUCCESS;
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_OFFSET, offset );
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_PAGE, page );
		
		io_port	= ifbp->IFB_IOBase + HREG_AUX_DATA;
		cp = (wci_bufp)ltvp->val;
		j = len;								//OUT_PORT_STRING macro may modify its parameters												
		OUT_PORT_STRING( io_port, cp, j );		//!!!WORD length,  cp MUST be a char pointer
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_OFFSET, offset );
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_AUX_PAGE, page );
		wp = ltvp->val;
		j = len;
		while ( j-- && rc == HCF_SUCCESS ) {
			if ( CNV_LITTLE_TO_INT(*wp) != IN_PORT_WORD( io_port ) ) {
				rc = HCF_ERR_DEFUNCT_DL;
				ifbp->IFB_HCF_Tallies.EngCnt++;
			}
			wp++;
		}
	} while ( cnt-- && rc != HCF_SUCCESS );
	tl = (hcf_32)MUL_BY_2( len );
	tl += ifbp->IFB_DLTarget[0] + ((hcf_32)ifbp->IFB_DLTarget[1] << 16);
	ifbp->IFB_DLTarget[0] = (hcf_16)tl;
	ifbp->IFB_DLTarget[1] = (hcf_16)(tl >> 16);
	return rc;
}/* download_data */
#endif // HCF_DOWNLOAD_OFF
//@HCFL>


/*FF***************************************************************************************************************
FUNCT: enable_int                                                                                2000/02/04 14:15
USERS: hcf_action                                       hcf_enable                                       
CALLS: hcf_action                                       
GLOBL: HCF_ACT_INT_OFF                                  HCF_ACT_INT_ON                                   
PARAM: event                                            ifbp                                             
       ifbp->IFB_IntEnMask                              
******************************************************************************************************************/
/***********************************************************************************************************************


.MODULE			enable_int
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	Enables a specific Hermes interrupt

.ARGUMENTS
  void enable_int( IFBP ifbp, int event )

.RETURNS
	void

.NARRATIVE

 Parameters:
	ifbp		address of the Interface Block
  	event		Hermes event to be enabled as interrupt source

 Remarks: To get the contents of the IntEn register changed is a two step process:

 	o change the "shadow" in IFB_IntEnMask

 	o call hcf_action to actually copy the shadow to the IntEn register.

 	To prevent a change in the "Card Interrupt En/Disabled state, a balancing
 	pair of HCF_ACT_INT_OFF and HCF_ACT_INT_ON must be used. To prevent
 	a temporary enabling as undesirable side effect, the first call must be
 	HCF_ACT_INT_OFF.
 	Note that at the very first interrupt, hcf_service_nic causes the removal of
 	the Tick and Cmd bit in the IntEn register.
 	
 	
.DIAGRAM
***********************************************************************************************************************/
//#pragma  Reminder2( "enable_int: shouldn't this be used in more places" )
void	enable_int(IFBP ifbp, int event ) {

	ifbp->IFB_IntEnMask |= event;
	(void)hcf_action( ifbp, HCF_ACT_INT_OFF );
	(void)hcf_action( ifbp, HCF_ACT_INT_ON );
	return;

}/* enable_int */



//<HCFL@
/*FF***************************************************************************************************************
FUNCT: enc_test                                                                                  2000/02/04 14:15
USERS: hcf_put_data                                     hcf_service_nic                                  
CALLS: CNV_BIG_TO_INT                                   
GLOBL: enc_trans_tbl                                    
PARAM: type                                             
LOCAL: i                                                idx                                              
******************************************************************************************************************/
/**************************************************************************************************************


.MODULE			enc_test
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	test whether len/type is E-II and whether it is contained in enc_trans_tbl

.ARGUMENTS
  int enc_test( wci_recordp type )

.RETURNS
	0		len/type is "len" ( <= 1500 )
	1..n	len/type is "type" and contained in enc_trans_tbl
	n+1		len/type is "type" but not contained in enc_trans_tbl
			-- (where n is the number of elements in enc_trans_tbl)

.NARRATIVE

 Parameters:
	type	address of len/type (in "network" Endian format)

 1:	initialization at -1 to compensate for the "+1" kludge/trick at the return statement
 2: Endianess independent conversion of type/len from 802.3 format to int. Note that wci_recordp is an
 	hcf_16 pointer so things work out allright for BigEndian platforms (where CNV_BIG_TO_INT is a dummy
 	macro). Note that the use of CNV_BIG_TO_INT has the "right effects" but is debatable on easthetical grounds
 3:	the litmus test to distinguish type and len.
 	The hard code "magic" value of 1500 is intentional and should NOT be replaced by a mnemonic and definitely
 	NOT be HCF_MAX_MSG because it is not related at all to the maximum frame size supported  by the Hermes.
 4:	the chosen initialization of idx and the pre-increment of idx, result in comparing i against the
	appropriate entries of enc_trans_tbl and -in conjunction with the "+1" kludge/trick at the return
	statement- in the appropriate return value
	
.NOTICE
 0	Parameter type is the address of len/type in "network" Endian format, which is BigEndian. 

**************************************************************************************************************/
int enc_test( wci_recordp type ) {

int		idx = -1;																						/* 1 */
hcf_16	i = CNV_BIG_TO_INT( *type );																	/* 2 */

	if ( i > 1500 ) 																					/* 3 */
		while ( ++idx < sizeof(enc_trans_tbl)/sizeof(enc_trans_tbl[0]) &&								/* 4 */		
				i != enc_trans_tbl[idx] ) /*NOP*/;
	return idx + 1;
} /* enc_test */
//@HCFL>

//<HCFL@
/*FF***************************************************************************************************************
FUNCT: get_info_mb                                                                               2000/02/04 14:15
USERS: hcf_get_info                                     
CALLS: IN_PORT_WORD                                     OUT_PORT_WORD                                    
       HCFASSERT                                        put_info_mb                                      
       min                                              mb_idx_inc                                       
DEFIN: AUX_MAGIC_1                                      CFG_DIAG                                         
       CFG_DL_STAT                                      CFG_NULL                                         
       CFG_UNK                                          DO_ASSERT                                        
       FAR                                              HCMD_CMD_CODE                                    
       HCMD_PROGRAM                                     HREG_EV_ACK                                      
       HREG_EV_CMD                                      HREG_EV_STAT                                     
       HREG_RESP_0                                      HREG_RESP_1                                      
       HREG_STAT                                        
GLOBL: HCF_ERR_DIAG_1                                   HCF_ERR_SEQ_BUG                                  
PARAM: bufp                                             bufp->len                                        
       bufp->typ                                        bufp->val                                        
       ifbp                                             ifbp->IFB_CardStat                               
       ifbp->IFB_IOBase                                 ifbp->IFB_MBInfoLen                              
       ifbp->IFB_MBRp                                   ifbp->IFB_MBp                                    
LOCAL: i                                                len                                              
       p                                                q                                                
       scratch                                          tlen                                             
******************************************************************************************************************/
/*******************************************************************************************************************


.MODULE			get_info_mb
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION

.ARGUMENTS
  void get_info_mb( IFBP ifbp, LTV_STRCT FAR * bufp )

.RETURNS
  void

.NARRATIVE

 Parameters:
  ifbp		address of the Interface Block
  bufp	
 		
  Description: 


.DIAGRAM


	Hcf_get_mb_info copies the contents of the oldest MailBox Info block in the MailBox
	to PC RAM. If len is less than the size of the MailBox Info block, only as much as
	fits in the PC RAM buffer is copied. After the copying the MailBox Read pointer is
	updated to point to the next MailBox Info block, hence the remainder of an "oversized"
	MailBox Info block is lost. The truncation of the MailBox Info block is NOT reflected in
	the return status. Note that hcf_get_info guarantees the length of the PC RAM buffer meets the
	minimum requirements of at least 2, so no PC RAM buffer overrun.

	Calling hcf_get_mb_info when their is no MailBox Info block available or
	when there is no MailBox at all, results in a "NULL" MailBox Info block.


 4:	Only the Diagnose-in-progress flag is cleared. In case of "UNK" it is difficult to construct the "correct"
	strategy, because this should never occur in the first place.  In case of Download, the action is
	considered terminated by the DL_STOP cmd
 	Another strategy would be to clear the cmd code if and only if the expected code
 	matches the actual code. Since in case of a mismatch something is wrong in the
 	design, no recognized criteria are present to prefer one strategy above the other.
 	
 	
	Diagnose Result MailBox Info Block:
	offset	value	description
	0x0000	0x0000	HCF did not detect an error. Note that the HCF and Hermes error
					detection are independent
			0x0001	HCF detected a mismatch in the parameter/response registers. This
					is only checked if the Hermes did not detect an error
	0x0001			contents of Hermes status register
					bits 0x003F: Hermes Diagnose Command Code (0x0003)
					bits 0x7F00: Hermes Error Code (0x0000: OK)
	0x0002			contents of Hermes Response 0 register
					Qualifier if Hermes Status register (offset 0x0001) reflects an error
					Irrelevant if Hermes Status register reflects O.K
	0x0003			contents of Hermes Response 1 register (irrelevant)
	0x0004			contents of Hermes Response 2 register (irrelevant)


 	


.ENDOC				END DOCUMENTATION
*/
/* -------------------------------------------------------------------------------------------------------------------*/
#if !defined HCF_MB_OFF
void get_info_mb( IFBP ifbp, LTV_STRCT FAR * bufp ) {

hcf_16		i, len, tlen;
wci_recordp	q;									//source pointer (in MailBox or Tally-part of IFB)
wci_recordp	p = bufp->val;						//destination pointer (in LTV record)

	len = bufp->len;
	if ( ( i = ifbp->IFB_CardStat & HCMD_CMD_CODE ) != 0 						&&
    	 IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT ) & HREG_EV_CMD 		) {
		hcf_16 scratch[2];
		scratch[0] = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_STAT ); //;?HCMD_DIAG or HCMD_PROGRAM
		OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, HREG_EV_CMD );		
		scratch[1] = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_RESP_0 );
		if ( ( scratch[0] & HCMD_CMD_CODE ) == i ) {; //;?HCMD_DIAG or HCMD_PROGRAM
			scratch[0] &= (hcf_16)~HCMD_CMD_CODE;
			if ( i == HCMD_PROGRAM ) {
				i = CFG_DL_STAT;
			} else /*if ( i == HCMD_DIAG )*/ {
				if ( scratch[0] == 0 ) {
					if ( scratch[1] != (hcf_16)~AUX_MAGIC_1 ||
						 IN_PORT_WORD( ifbp->IFB_IOBase + HREG_RESP_1 ) != AUX_MAGIC_1 ) {
						scratch[0] = HCF_ERR_DIAG_1;
					}
					scratch[1] = 0;
				}
				ifbp->IFB_CardStat &= (hcf_16)~HCMD_CMD_CODE;			/* 4 */  //;?bad location, move it up the 
																//logical "if"-level	
																//feels dangerous because that may interfere
																//with download
				i = CFG_DIAG;
				//;?restore Hermes Configuration <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			}
		} else {
			HCFASSERT( DO_ASSERT, scratch[0] )
			i = CFG_UNK;
			scratch[1] = HCF_ERR_SEQ_BUG;
			ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_CMD_SEQ;
			HCFLOG( ifbp, HCF_LOG_DEFUNCT, HCF_ERR_SEQ_BUG, 0 );
			ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
		}
		put_info_mb( ifbp, i, scratch, sizeof(scratch)/sizeof(hcf_16) + 1 );
	}				
	tlen = ifbp->IFB_MBp ? ifbp->IFB_MBp[ifbp->IFB_MBRp] : 0;
	p--;			                                		//destination pointer (in LTV record)
	len = min( len, tlen );									//length oldest MB_Info block as far as fits in bufp
	bufp->len = len;										//length oldest MB_Info block as far as fits in bufp
	if ( len == 0 ) {
		//#pragma  Reminder2( "hcf_get_info: no MB_Info block available -> L=0, T=CFG_NULL, no value" )
		bufp->len = 1;
		bufp->typ = CFG_NULL;
	} else {
		q = mb_idx_inc( ifbp, &ifbp->IFB_MBRp );
		while ( tlen-- ) {									//process all the bytes of the MB_Info block
			if ( len ) {
				len--;
				*p++ = *q;
			}
			q = mb_idx_inc( ifbp, &ifbp->IFB_MBRp );
		}
	    ifbp->IFB_MBInfoLen = *q;							//update MailBox Info length report to MSF
	}
}/* get_info_mb */
#endif // HCF_MB_OFF
//@HCFL>


/*FF***************************************************************************************************************
FUNCT: ini_hermes                                                                                2000/02/04 14:15
USERS: hcf_disconnect                                   hcf_initialize                                   
CALLS: HCF_DEBUG_TRIGGER                                IN_PORT_WORD                                     
       HCFASSERT                                        HCFLOG                                           
       cmd_wait                                         OUT_PORT_WORD                                    
DEFIN: CARD_STAT_DEFUNCT                                CARD_STAT_PRESENT                                
       HCF_ASSERT_MISC                                  HCF_LOG_DEFUNCT                                  
       HCF_MAGIC                                        HCMD_BUSY                                        
       HCMD_INI                                         HCMD_INI_0x0100                                  
       HREG_CMD                                         HREG_EV_ACK                                      
       HREG_SW_0                                        INI_PRIM                                         
GLOBL: HCF_ERR_DEFUNCT_INI                              HCF_ERR_NO_NIC                                   
       HCF_ERR_TIME_OUT                                 HCF_SUCCESS                                      
PARAM: cntl                                             ifbp                                             
       ifbp->IFB_CardStat                               ifbp->IFB_DefunctStat                            
       ifbp->IFB_IOBase                                 ifbp->IFB_TickIni                                
LOCAL: prot_cnt                                         rc                                               
******************************************************************************************************************/
/****************************************************************************************************************************


.MODULE			ini_hermes
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION

.ARGUMENTS
  int ini_hermes( IFBP ifbp )

.RETURNS
	HCF_ERR_NO_NIC
	HCF_ERR_TIME_OUT	!!!!SUPERFLUOUS can be removed from implemenation
	HCF_ERR_DEFUNCT_INI	
	>>cmd_wait	

.NARRATIVE

 Parameters:
  ifbp		address of the Interface Block
 		
  Description: 

	;?As side effect of the Hermes Initialize command, the interrupts are disabled


.DIAGRAM
 4:	Preset the return code at Time out. Since ini_hermes is the part of the initialization of the card and 
 	since the strategy is to detect problems as a side effect of "necessary" actions to initialize the card, 
 	ini_hermes has, in deviation of the cmd_wait strategy, an "wait for busy bit drop" befor the Hermes 
 	Initilalize command is executed.
	The additional complication that no calibrated value for the protection count can be assumed since calibrate()
	may not yet have determined a calibrated value (a catch 22) is handled by the initial value set at INI_TICK_INI 
	by hcf_connect). This approach is considered safe, because:
	 o the HCF does not use the pipeline mechanism of Hermes commands.
	 o the likelihood of failure (the only time when protection count is relevant) is small.
	 o the time will be sufficiently large on a fast machine (busy bit drops on good NIC before counter expires)
	 o the time will be sufficiently small on a slow machine (counter expires on bad NIC before the enduser
	   switches the power off in despair
	The time needed to wrap a 32 bit counter around is longer than many humans want to wait, hence the more or 
	less arbitrary value of 0x40000L is chosen, assuming it does not take too long on an XT and is not too short on 
	a scream-machine.
	Note that early models of the Hermes needed some arbitrary read before the first write activity to operate
	stable (ref tracker 27). This busy drop wait achieves that function as a side effect.
	If the Hermes passes the superficial health test of waiting for the Busy bit drop, the Hermes Initialize 
	command is executed. The Initialize command acts as the "best possible" reset under HCF control. A more 
	"severe/reliable" reset is under MSF control via the COR register.
18:	Initialize the Hermes. The results of this way of initialization depends on the capabilities of the Hermes
  	firmware. Therefore, the Hermes H/W controlled bits, like those in the event status register are not guaranteed
  	to be cleared as result of the initialize command. However it is guaranteed that no more events are in the
    pipeline. Ack-ing indiscriminately all events resolves this problem. An alternative would be to use the
    resulting value of "IN_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_STAT )" rather than 0xFFFF to specifically
    only reset those events which are set. This strategy is considered to be only more aesthetically pleasing
    (if that).
20:	Perform some housekeeping tasks
	Write HCF_MAGIC as signature to S/W support register 0.  This signature is used to detect card removal
	wherever the presence of the card is critical while HCF may not yet have been informed by the MSF of the
	removal of the card.   	
	

.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
int ini_hermes( IFBP ifbp, int cntl ) {

int		rc = HCF_ERR_NO_NIC;
hcf_32	prot_cnt  = ifbp->IFB_TickIni;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0010, 0 );
    if ( ifbp->IFB_CardStat & CARD_STAT_PRESENT ) {
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0010, 1 );
		rc = HCF_ERR_TIME_OUT;																		/* 4 */
		while (prot_cnt && IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) & HCMD_BUSY ) prot_cnt--;
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0010, 2 );
		HCFASSERT( prot_cnt, IN_PORT_WORD(ifbp->IFB_IOBase + HREG_CMD ) )
		if ( prot_cnt == 0 ) {
			rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_INI;
			HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
			ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
		} else {
			rc = cmd_wait( ifbp, cntl == INI_PRIM ? HCMD_INI | HCMD_INI_0x0100 : HCMD_INI, 0 );		/* 18*/
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_EV_ACK, 0xFFFF );
			OUT_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_0, HCF_MAGIC );								/* 20*/
		}
	}
	HCFASSERT( rc == HCF_SUCCESS || rc == HCF_ERR_NO_NIC, rc )
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0010, -1 );
	return rc;
}/* ini_hermes */



/*FF***************************************************************************************************************
FUNCT: isr_info                                                                                  2000/02/04 14:15
USERS: hcf_service_nic                                  
CALLS: HCF_DEBUG_TRIGGER                                IN_PORT_WORD                                     
       HCFLOG                                           hcfio_string                                     
       put_info_mb                                      
DEFIN: BAP_1                                            CFG_LINK_STAT                                    
       CFG_TALLIES                                      HCF_ASSERT_MISC                                  
       HCF_LOG_SERVICE_NIC                              HCF_NIC_TAL_CNT                                  
       HREG_INFO_FID                                    IO_IN                                            
       NULL                                             
PARAM: ifbp                                             ifbp->IFB_IOBase                                 
       ifbp->IFB_LinkStat                               ifbp->IFB_MB_FID                                 
       ifbp->IFB_NIC_Tallies                            
LOCAL: info                                             p                                                
       tmp                                              
******************************************************************************************************************/
/****************************************************************************************************************************


.MODULE			isr_info
.LIBRARY 		HCF_SUP
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION

.ARGUMENTS
  void isr_info( IFBP ifbp )

.RETURNS
  void

.NARRATIVE

 Parameters:
  ifbp		address of the Interface Block
  		
  Description: 


 1:	First the FID number corresponding with the InfoEvent is determined and next the length of the 
 	T-field + the length of the Value-field in words are fetched into scratch buffer info.
 2: In case of tallies, the 16 bits Hermes values are accumulated in the IFB into 32 bits values. Info[0]
 	is (expected to be) HCF_NIC_TAL_CNT + 1. The contraption "while ( info[0]-- >1 )" rather than 
 	"while ( --info[0] )" is used because it is dangerous to determine the length of the Value field by 
 	decrementing info[0]. As a result of a bug in some version of the Hermes, info[0] may be 0, resulting 
 	in a very long loop in the predecrement logic.
 	In case of the Lucent AccessPoint, the tallies are put in the MailBox rather than in the IFB.
 4: In case of a link status frame, the information is copied to the IFB field IFB_linkStat as well as
 	copied to the MailBox. 
 	Although it may seem attractive to use "ifbp->IFB_LinkStat = IN_PORT_WORD( ifbp->IFB_IOBase + BAP_1 );"
 	rather than "(void)hcfio_string(ifbp, BAP_1, tmp, 0, (wci_bufp)ifbp->IFB_LinkStat, 0, 2, IO_IN );"
 	this invalidate the information in the IFB_BAP_1 structure. The subsequent put_info_mb() will then 
 	behave incorrectly
 6:	All other than Tallies (including "unknown" ones and link status which is also put in IFB_LinkStat) are 
 	put in the MailBox.  Although put_info_mb is robust against a len-parameter with value zero, it accepts 
 	any bogus value for the type-parameter.
	
.DIAGRAM

.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
void isr_info( IFBP ifbp ) {

hcf_16	info[2], tmp;
hcf_32	*p;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x000C, 0 );                          						/* 1 */
	tmp = IN_PORT_WORD( ifbp->IFB_IOBase + HREG_INFO_FID );
	HCFLOG( ifbp, HCF_LOG_SERVICE_NIC, 0x0010, tmp );
	(void)hcfio_string(ifbp, BAP_1, tmp, 0, (wci_bufp)info, 2, sizeof(info), IO_IN );
	
#if MSF_COMPONENT_ID != COMP_ID_AP1																		 //<HCFL>
	if ( info[1] == CFG_TALLIES ) {
		if ( info[0] > HCF_NIC_TAL_CNT ) info[0] = HCF_NIC_TAL_CNT + 1;										/* 2 */
		p = (hcf_32*)&ifbp->IFB_NIC_Tallies;
		while ( info[0]-- >1 ) *p++ += IN_PORT_WORD( ifbp->IFB_IOBase + BAP_1 );	//request may return zero length
	}
	else 																								 //<HCFL@
#endif //COMP_ID_AP1
	{	
		if ( info[1] == CFG_LINK_STAT ) {
			(void)hcfio_string(ifbp, BAP_1, tmp, 4, (wci_bufp)&ifbp->IFB_LinkStat, 1, 2, IO_IN );			/* 4 */
		}
#if !defined HCF_MB_OFF
		ifbp->IFB_MB_FID = tmp;                     														/* 6 */
		put_info_mb( ifbp, info[1], NULL, info[0] );
#endif // HCF_MB_OFF
	}																									 //@HCFL>
	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x000C, -1 );
	return;
}/* isr_info */



//<HCFL@
/*FF***************************************************************************************************************
FUNCT: log_rtn                                                                                   2000/02/04 14:15
CALLS: ifbp->IFB_LogRtn                                 
PARAM: ifbp                                             ifbp->IFB_LogLvl                                 
       ifbp->IFB_LogRtn                                 rtn_id                                           
       val_1                                            val_2                                            
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			log_rtn
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCF routines
.DESCRIPTION	
.ARGUMENTS
  log_rtn( HCF_LOG_DEFUNCT, rc, 0 )

.RETURNS

.NARRATIVE

 Parameters:
   		
 Description: 


.DIAGRAM

.NOTICE
 ENDOC				END DOCUMENTATION

----------------------------------------------------------------------------------------------------------------------*/
void log_rtn( IFBP ifbp, int rtn_id, int val_1, int val_2) {


	if ( rtn_id & ifbp->IFB_LogLvl ) {
		ifbp->IFB_LogRtn( rtn_id, val_1, val_2);
	}
} /* log_rtn */
//@HCFL>



//<HCFL@
/*FF***************************************************************************************************************
FUNCT: mb_idx_inc                                                                                2000/02/04 14:15
USERS: get_info_mb                                      put_info_mb                                      
PARAM: ifbp                                             ifbp->IFB_MBSize                                 
       ifbp->IFB_MBp                                    ip                                               
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			mb_idx_inc
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Support for HCFR routines
.DESCRIPTION	
.ARGUMENTS
  wci_recordp mb_idx_inc( IFBP ifbp, wci_recordp ip )

.RETURNS

.NARRATIVE

 Parameters:
  ifbp	address of the Interface Block
  ip	
 		
  Description: 


.DIAGRAM

.NOTICE
  wci_recordp is used as type for parameter i, because the "natural" size for i is an hcf_16 (based on its
  ultimate source in the IFB being either IFB_MBWp or IFB_MBRp) but since i may have been allocated at the
  stack on interrupt time, a FAR pointer is needed in the segmented3d environments
.ENDOC				END DOCUMENTATION

----------------------------------------------------------------------------------------------------------------------*/
wci_recordp mb_idx_inc( IFBP ifbp, wci_recordp ip ) {

		*ip = (*ip + 1 ) % ifbp->IFB_MBSize;
		return &ifbp->IFB_MBp[*ip];
}  /* mb_idx_inc */
//@HCFL>


//<HCFL@
#if defined _M_I86TM
/****************************************************************************************************************************


.MODULE				patch_catch
.LIBRARY 			HCFS_SUP
.TYPE 				function
.SYSTEM				msdos
.SYSTEM				unix
.APPLICATION		Support for HCFS routines
.DESCRIPTION
	debug and field support escape mechanism
.ARGUMENTS
  void patch_catch( void )
	
.RETURNS
  void
  
.NARRATIVE

 Parameters:
  N.A
   		
 Description: 
	Patch_catch is intended to have an escape mechanism to help diagnose
	problems in the field with released S/W.
	Patch_catch builds on the macro INT_3.
	INT_3 in its turn is controlled by DEBUG_INT.
	When DEBUG_INT is defined, e.g. in case of the Microsoft compiler by
	specifying "/D DEBUG_INT" in the command line, INT_3 expands to an
	"int 3", otherwise into a "nop".
	Released HCF-based programs should never specify DEBUG_INT. The macro
	DEBUG_INT is intended for engineering testing during HCF-development only.
	By succeeding the INT_3 macro with the obscure "dec ax etc" sequence,
	80x86 code is generated with a bit pattern identical to the string
	"HCF-II". In the loadable file of a program which is compiled without
	DEBUG_INT, the routine patch_catch can easily be located by searching for
	the string "HCF-II". By patching the 0x90 ("nop") immediately preceeding
	this string with 0xCC (int 3"), an experienced support engineer - possibly
	with support per telephone from a WCND engineer - can get a handle to
	intercept the program execution.
	Note that the effect of the nonsense "dec ax etc" instruction sequence, is
	effectively nullified by the entry- and exit code generated by the compiler
	
.ENDOC				END DOCUMENTATION

----------------------------------------------------------------------------------------------------------------------*/
void patch_catch( void ) {
	INT_3;
#if defined (_MSC_VER)
	__asm {
		dec 	ax
		inc 	bx
		inc 	si
		sub 	ax, 'II'
	}
#endif //_MSC_VER
	return;
}
#endif //_M_I86TM
//@HCFL>




#if !defined __BEOS__
#if defined HCF_PROFILING || defined HCF_DEBUG

void ppdbgSet( unsigned char maskbits ) {

    __asm {

        mov dx, PORT
        mov ah, maskbits

        pushf
        cli             ; we want this atomic

        in al, dx
        or al, ah
        out dx, al

        popf
    }
} // ppdbgSet


void ppdbgClear( unsigned char maskbits ) {


    __asm {
        mov dx, PORT
        mov ah, maskbits
        not ah

        pushf
        cli             ; we want this atomic

        in al, dx
        and al, ah
        out dx, al

        popf
    }
} //ppdbgClear

void ppdbgPulse( unsigned char maskbits ) {

    __asm {
        mov dx, PORT
        mov ah, maskbits

        pushf
        cli             ; we want this atomic

        in al, dx
        xor al, ah
        out dx, al
        xor al, ah
        out dx, al

        popf
    }
} //ppdbgPulse
#endif // HCF_PROFILING || HCF_DEBUG
#endif // __BEOS__


/*FF***************************************************************************************************************
FUNCT: put_info                                                                                  2000/02/04 14:15
USERS: hcf_initialize                                   hcf_put_info                                     
CALLS: hcf_get_info                                     CNV_LITTLE_TO_INT                                
       hcfio_string                                     MUL_BY_2                                         
       cmd_wait                                         
DEFIN: BAP_1                                            CARD_STAT_INCOMP_FEATURE                         
       CARD_STAT_INCOMP_PRI                             CARD_STAT_INCOMP_STA                             
       CARD_STAT_PRESENT                                CFG_CARD_PROFILE                                 
       CFG_CNF_ENCRYPTION                               CFG_CNF_PM_ENABLED                               
       CFG_TX_RATE_CONTROL                              HCF_RESTRAINT_CRYPT_FW                           
       HCF_RESTRAINT_CRYPT_HW                           HCF_RESTRAINT_PM                                 
       HCMD_ACCESS                                      HCMD_ACCESS_WRITE                                
       IO_OUT_CHECK                                     
GLOBL: HCF_ERR_INCOMP_FEATURE                           HCF_ERR_NO_NIC                                   
       HCF_SUCCESS                                      
PARAM: ifbp                                             ifbp->IFB_CardStat                               
       ifbp->IFB_MSFType                                ifbp->IFB_STACfg                                 
       ifbp->IFB_WarningInfo                            ltvp                                             
       ltvp->len                                        ltvp->typ                                        
       ltvp->val                                        
LOCAL: cnt                                              rc                                               
       x                                                x.capability_options                             
       x.len                                            x.typ                                            
******************************************************************************************************************/
/***********************************************************************************************************************


.MODULE			put_info
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	stores Hermes configuration information in the CfgTbl of the IFB

.ARGUMENTS
  int put_info( IFBP ifbp, LTVP ltvp )

.RETURNS
	HCF_ERR_NO_NIC	
    0 (Restraint violation)
	>>hcfio_string	
	>>cmd_wait	

.NARRATIVE

 Parameters:
	ifbp		address of the Interface Block
	ltvp		address in NIC RAM where LVT-records are located

 Remarks:
	In case of HCF-light, it may seem that it would be an improvement to integrate this code into
	hcf_put_info, however in the full blown HCF there is more logic involved in the handling of
	configuration, which makes this integration unattractive.
	<HCFL@: Safekeeps Hermes configuration information. This information is used to restore the Hermes
	configuration after download and diagnose
	The Configuration information is a series of LTV-records, terminated with an L-field with a zero-value.
	@HCFL>

.NARRATIVE
	All the validation/adaptation logic is skipped in case of HCF-light
 2: When Encryption is enabled while the NIC (note that in this context NIC should be read as "real" H/W or 
 	P4 RID, which is something else than F/W, see also the Encryption related test on F/W level further on) 
 	does not support it, the configuration is dropped, the HCF blocks enabling and a warning bit is set 
 	for retrieval by the WaveMngrClient
	Note: do not change the HCF to do: ifbp->IFB_CardStat &= ~CARD_STAT_INCOMP_FEATURE;
		as an equivalent of what is done in the other cases cases in this function. That would cause this 
		bit to disappear if - in the future, in case of multiple potential causes - the last check is O.K. 
		regardless of the other checks.
		The CARD_STAT_INCOMP_FEATURE bit disappears only as side effect of HCF_ACT_CARD_IN
	Note also that the so-called P4 check should precede F/W compatibilty check otherwise the user is first 
	told: upgrade your F/W and after the upgrade: your H/W is incapable		
 4:	If the RID indicates enabling of Encryption, structure x is set up
 	Note that x can not be initialized at its defintion to prevent certain compilers to generate a reference
 	to the C-lib memmove function.
 	Note also that the options field must be presetted at zero, to assure that if the RID not available, 
 	encryption can't be enabled
 	Finally note that the return status is ignored because we are pursuing other things while the implementation
 	is resistent against this problem
 6:	In order to keep a resemblance to structure code, the DCWA loop counter is (mis-) used to convey the 
 	information to the "real" body of put_info that no actual configuration should take place.
 8:	The remainder of validation/adaptation logic is skipped in case of an AP because in case of an AP there 
 	is nodynamic binding between F/W and Host S/W. There is an IFB+MSFTYpe based escape to cater for AP mode 
 	on hermaphrodite AP/STA 	
10:	When Power Managemnent is enabled, the configuration is dropped and a warning bit is set for retrieval
	by the WaveMngrClient. Since there is a single bit uniquely associated with this problem it can be
	pre-cleared and set if the RID tries to set PM on a NIC with unsuitable F/W.
12:	When a RateControl above 3 is selected, the configuration is silently dropped  on a NIC with unsuitable F/W.
	Note that this "silently" is even more silent than the others because not even a bit in IFB_WarningInfo
	is set.
14: When Encryption is enabled while the NIC supports it but the F/W does not, the configuration is dropped,
	the HCF blocks enabling and a warning bit is set for retrieval by the WaveMngrClient
	Note: see the notes in the Encryption case above
20:	do not write RIDs to NICs which are incompatible with respect to Primary or Station Firmware level	<HCFL>
22: only write RIDs when the NIC is present
24: <HCFL@: If the RID does not exist, the L-field is set to zero. If the RID exists, the L-field is left at
 	one																									@HCFL>
26: write the RID to the NIC using the DataCorruptionWorkAround, up to a maximum of three times	
28:	If the RID is written successful, pass it to the NIC by means of an Access Write command
**************************************************************************************************************/
int put_info( IFBP ifbp, LTVP ltvp	) {

int						cnt = 3;
int						rc = HCF_ERR_INCOMP_FEATURE;
CFG_CARD_PROFILE_STRCT	x;																				//<HCFL@

	if ( ltvp->typ == CFG_CNF_ENCRYPTION ) {																/* 2 */
		ifbp->IFB_WarningInfo[0] &= ~(HCF_RESTRAINT_CRYPT_FW |  HCF_RESTRAINT_CRYPT_HW);
		if ( ltvp->val[0] != 0 ) {  																		/* 4 */
			x.len = 4;
			x.typ = CFG_CARD_PROFILE;
			x.capability_options = 0;
			(void)hcf_get_info( ifbp, (LTVP)&x );
			if ( (CNV_LITTLE_TO_INT(x.capability_options) & 0x01) == 0) {
				ifbp->IFB_WarningInfo[0] |= HCF_RESTRAINT_CRYPT_HW;
				ifbp->IFB_CardStat |= CARD_STAT_INCOMP_FEATURE;
				cnt = 0;																					/* 6 */
			}
		}
	}
	
#if defined HCF_STA																							/* 8 */
	if ( ifbp->IFB_MSFType == 0 ) {
		if ( ifbp->IFB_STACfg < 0x0300 ) {																	/* 10*/
			if ( ltvp->typ == CFG_CNF_PM_ENABLED ) {
				ifbp->IFB_WarningInfo[0] &= ~HCF_RESTRAINT_PM;
				if ( ltvp->val[0] != 0 ) {
					ifbp->IFB_WarningInfo[0] |= HCF_RESTRAINT_PM;
					cnt = 0;
				}
			}
		}
		if ( ifbp->IFB_STACfg < 0x0400 ) {																	/* 12 */
			if ( ltvp->typ == CFG_TX_RATE_CONTROL )
				if ( CNV_LITTLE_TO_INT(ltvp->val[0]) >= 0x04 ) {
					cnt = 0;
			}
		}
		if ( ltvp->typ == CFG_CNF_ENCRYPTION ) {															/* 14*/
			if ( ifbp->IFB_STACfg < 0x0600 ) {
				ifbp->IFB_WarningInfo[0] &= ~HCF_RESTRAINT_CRYPT_FW;
				if ( ltvp->val[0] != 0 ) {
					ifbp->IFB_WarningInfo[0] |= HCF_RESTRAINT_CRYPT_FW;
					ifbp->IFB_CardStat |= CARD_STAT_INCOMP_FEATURE;
					cnt = 0;
				}
			}
		}
	}                                                                                                 
#endif // HCF_STA                   	                                                                  //@HCFL>

	if ( (ifbp->IFB_CardStat & (CARD_STAT_INCOMP_PRI|CARD_STAT_INCOMP_STA) ) == 0 )				//<HCFL>	/* 20*/
	if ( (ifbp->IFB_CardStat & CARD_STAT_PRESENT) == 0 ) rc = HCF_ERR_NO_NIC; 	//misindent to accomodate HCF-light	/* 22*/
	else {
#if defined HCF_ASSERT  //<HCFL@
{
		x.len = 2;
		x.typ = ltvp->typ;
		hcf_get_info( ifbp, (LTVP)&x );
		HCFASSERT( x.len || ltvp->typ == CFG_CNF_DEFAULT_KEYS, ltvp->typ )									/* 24*/
}
#endif //HCF_ASSERT //@HCFL>
		 while ( cnt-- && rc != HCF_SUCCESS ) {																/* 26*/
			rc = hcfio_string( ifbp, BAP_1,
							   ltvp->typ, 0, (wci_bufp)ltvp, 2, MUL_BY_2(ltvp->len + 1), IO_OUT_CHECK );
		}
		if ( rc == HCF_SUCCESS ) rc = cmd_wait( ifbp, HCMD_ACCESS + HCMD_ACCESS_WRITE, ltvp->typ );			/* 28*/
	}	
	
	return rc;
}/* put_info */


//<HCFL@
/*FF***************************************************************************************************************
FUNCT: put_info_mb                                                                               2000/02/04 14:15
USERS: hcf_action                                       hcf_put_info                                     
       hcf_service_nic                                  get_info_mb                                      
       isr_info                                         
CALLS: HCFASSERT                                        mb_idx_inc                                       
       min                                              hcfio_string                                     
       MUL_BY_2                                         
DEFIN: BAP_1                                            CFG_INFO_FRAME_MIN                               
       DO_ASSERT                                        IO_IN                                            
GLOBL: ifbp->IFB_HCF_Tallies.NoBufMB                    
PARAM: bufp                                             ifbp                                             
       ifbp->IFB_MBInfoLen                              ifbp->IFB_MBRp                                   
       ifbp->IFB_MBSize                                 ifbp->IFB_MBWp                                   
       ifbp->IFB_MB_FID                                 ifbp->IFB_MBp                                    
       len                                              type                                             
LOCAL: i                                                offset                                           
       p                                                tlen                                             
******************************************************************************************************************/
/*******************************************************************************************************************

.MODULE			put_info_mb
.LIBRARY
.TYPE 			function
.SYSTEM
.APPLICATION	Support for HCF routines
.DESCRIPTION
	copies either a PC-RAM located buffer or a RID/FID as a single Info block into the MailBox
.ARGUMENTS
  void put_info_mb( IFBP ifbp, hcf_16 type, wci_recordp bufp, hcf_16 len )  
  Card Interrupts disabled

.RETURNS
  void
  			   
.NARRATIVE

 parameters:
	ifbp		address of the Interface Block
	type		in case of a V record in PC RAM:
				CFG_DIAG, CFG_WMP, CFG_UNK, CFG_DL_STAT or CFG_MB_ASSERT
				in case of RID/FID:
				CFG_SCAN, CFG_LINK_STAT, CFG_ASSOC_STAT
	bufp		pointer to a V record in PC RAM or don't care in case of RID/FID
	len			length in words of T field + V record in PC RAM

  Description: 
	Note that the definition of len may not be 100% intuitive at first glance, but it is historically grown
	(in relation ship with the use of put_info_mb to copy LTVs directly from NIC RAM rather than PC RAM) and
	changing is expected to give more problems than solve them.
	
	If the data does not fit (including no MailBox is available), the IFB_MBTally is incremented and an 
	HCF_ASSERT catches, but no error status is returned.
	Calling put_info_mb when their is no MailBox available, is considered a design error in the MSF.
	

	Note that there is always at least 1 word of unused space in the mail box.
	As a consequence:
	- no problem in pointer arithmetic (MB_RP == MB_WP means unambiguously mail box is completely empty
	- There is always free space to write an L field with a value of zero after each MB_Info block.  This
	  allows for an easy scan mechanism in the "get MB_Info block" logic.

.DIAGRAM
 2:	First the free space in the MailBox is calculated (2a: free part from Write Ptr to Read Ptr, 
 	2b: free part turns out to wrap around) . If this space suffices to store the number of words
	reflected by len (T-field + Value-field) plus the additional MailBox Info L-field + a trailing 0 to act
	as the L-field of a trailing dummy or empty LTV record, then a MailBox Info block is build in the MailBox
	consisting of
	  -	the value len in the first word
	  -	type in the second word
	  - a copy of the contents of bufp/RID/FID in the second and higher word

	There is a simple mapping from the publicized WCI I/F for hcf_put_info to put_info_mb
	The WMP frame mode is recognized based on the IFB_MB_FID field of the IFB being non-zero.
		
	In the WMP mode, the MailBox Info block is build by means of input from NIC RAM rather than copied from a
	buffer. Since the (future) size of WMP frames is not known, it is considered to be an unattractive choice
	to copy a WMP frame first into a temporary buffer and then use the "standard" put_info_mb to copy the
	contents of the temporary buffer into the MailBox. Note that there are more arguments for the chosen
	implementation, e.g. efficiency
	
 4: Since put_info_mb() can more or less directly be called from the MSF level, the I/F must be robust
	against out-of-range variables. As failsafe coding, the MB update is skipped by changing tlen to 0 if 
	len == 0; This will indirectly cause an assert as result of the violation of the next if clause.
 6:	Check whether the free space in MailBox suffizes (this covers the complete absence of the MailBox).
	Note that len is unsigned, so even MSF I/F violation works out O.K.
	The '2' in the expression "len+2" is used because 1 word is needed for L itself and 1 word is needed
	for the zero-sentinel
 8:	update MailBox Info length report to MSF with "oldest" MB Info Block size

 5:	In case of an Hermes Information frame ( CFG_SCAN, CFG_LINK_STAT and CFG_ASSOC_STAT, as reflected by
	"type >= CFG_INFO_FRAME_MIN"), the L and T are already copied into the MailBox, hence reading the
	Information frame must start at offset 4.
 	In case of an Hermes Receive frame ( CFG_WMP ), reading must start at offset 0.
!!!	;? now we are in trouble, if this "if" clause was skipped (because the ReadPointer is larger than the
	WritePointer), this type dependent code is not executed. Simply copying this code to the 2nd hcfio_string
	does not work either because then it may be executed twice ;?


.NOTE
	boundary testing depends on the fact that IFB_MBSize is guaranteed to be zero if no MailBox is present,
	and to a lesser degree, that IFB_MBWp = IFB_MBRp = 0
	
.ENDOC				END DOCUMENTATION
-------------------------------------------------------------------------------------------------------------*/
#if !defined HCF_MB_OFF
void put_info_mb( IFBP ifbp, hcf_16 type, wci_recordp bufp, hcf_16 len ) {

hcf_16		i = ifbp->IFB_MBWp;                      	//byte offset to start writing in MailBox
wci_recordp	p = &ifbp->IFB_MBp[i];	         			//destination pointer (in MailBox)
hcf_16		tlen;										//free length/working length/offset in WMP frame
hcf_16		offset = (hcf_16)(type >= CFG_INFO_FRAME_MIN ? 4 : 0);	/*When copying from NIC RAM, skip LT of 
														  LTV but not the first 4 bytes of WMP frames			*/

	HCFASSERT( ifbp->IFB_MBp != 0, 0 )
	HCFASSERT( ifbp->IFB_MBSize, 0 )

	if ( ifbp->IFB_MBRp > ifbp->IFB_MBWp ) tlen = ifbp->IFB_MBRp - ifbp->IFB_MBWp;						/* 2a*/
	else tlen = ifbp->IFB_MBSize + ifbp->IFB_MBRp - ifbp->IFB_MBWp;										/* 2b*/
	
	if ( len == 0 ) tlen = 0;																			/* 4 */
	if ( len + 2 >= tlen ){																				/* 6 */
		HCFASSERT( DO_ASSERT, len )
		ifbp->IFB_HCF_Tallies.NoBufMB++;
	} else {
	    if ( ifbp->IFB_MBInfoLen == 0 )  ifbp->IFB_MBInfoLen = len;										/* 8 */
		*p = len--;										/*write Len (= size of T+V in words to MB_Info block
														 *and compensate len for this						*/
		p = mb_idx_inc( ifbp, &i );
		*p = type;										/*write Type to MB_Info block						*/
		p = mb_idx_inc( ifbp, &i );

		if ( i > ifbp->IFB_MBRp ) {						/*space from i till end of mail box is free			*/
			tlen = min( len, ifbp->IFB_MBSize - i );	/*data size (first part) which fits till end of MB	*/
			i += tlen;									/*index after first (possibly only) part is written	*/
			len -= tlen;								/*amount of data (if any) to write after wraparound	*/
			if ( ifbp->IFB_MB_FID ) {
				tlen += tlen;							/*adapt to byte logic used in hcfio_in_string		*/
				(void)hcfio_string( ifbp, BAP_1, ifbp->IFB_MB_FID, offset, (wci_bufp)p, 0, tlen, IO_IN );/* 5*/
				offset += tlen;
			} else {
				while ( tlen-- ) *p++ = *bufp++;		/*process first part/all bytes of the MB_Info block */
			}
			if ( len ) {								/*check for wrap around								*/
				i = 0;
				p = ifbp->IFB_MBp;
			}
		}
		i += len;										/*update WritePointer of MailBox 					*/
		if ( i == ifbp->IFB_MBSize ) i = 0;
		ifbp->IFB_MBWp = i;
		ifbp->IFB_MBp[i] = 0;							//to assure get_info_mb stops
		
		if ( ifbp->IFB_MB_FID ) {
			(void)hcfio_string( ifbp, BAP_1, ifbp->IFB_MB_FID, offset, (wci_bufp)p, 0, MUL_BY_2( len ), IO_IN);
			ifbp->IFB_MB_FID = 0;
		} else {
			while ( len-- ) *p++ = *bufp++;				/*process all (remaining) bytes of the MB_Info block*/
		}
	}
}/* put_info_mb */
#endif// HCF_MB_OFF
//@HCFL>



//<HCFL@
/*FF***************************************************************************************************************
FUNCT: put_info_tbl                                                                              2000/02/04 14:15
USERS: hcf_put_info                                     
CALLS: HCFASSERT                                        
DEFIN: CFG_CNF_DEFAULT_KEYS                             DO_ASSERT                                        
       HCF_MAX_CONFIG                                   
PARAM: ifbp                                             ifbp->IFB_CfgTbl                                 
       ltvp                                             ltvp->len                                        
       ltvp->typ                                        
LOCAL: i                                                p1                                               
       p2                                               wp                                               
******************************************************************************************************************/
/**************************************************************************************************************

.MODULE			put_info_tbl
.LIBRARY 		HCF
.TYPE 			function
.SYSTEM			msdos
.SYSTEM			unix
.SYSTEM			NW4
.APPLICATION	Card Initialization Group for WaveLAN based drivers and utilities
.DESCRIPTION	stores Hermes configuration information in the CfgTbl of the IFB

.ARGUMENTS
  void put_info_tbl( IFBP ifbp, LTVP ltvp )

.RETURNS
	void

.NARRATIVE

 Parameters:
  	ifbp		address of the Interface Block
  	ltvp		address of LTV in PC RAM to be stored in CfgTbl

.NARRATIVE
 Safekeeps Hermes configuration information. This information is used to restore the Hermes configuration 
 after download and diagnose.
 The Configuration information is a series of LTV-records, terminated with an L-field with a zero-value.
 This zero terminator is used to guard the scanning through the table.

.DIAGRAM
 2:	scan for the end of the configuration information within the table or till the same configuration
	as specified by the parameter ltvp is found
 4:	if the action code is already present in CfgTbl, move the remaining LTV-records of the CfgTbl forward, 
	thus overwriting the old record corresponfing with parameter ltvp.
 6:	move one LTV at a time. Note that i, reflecting the total size of the LTV, is one more than the value
	of L (which reflects the size of T+V )
 8: test for sufficient space in CfgTbl. If too little space, this is silently ignored, no action (except
	an Assert) 
10:	copy type, len and RID contents to CfgTbl
12:	For safety reason, make sure there is a zero-terminator. This  allows the MSF to shrink the multicast 
	table etc.
**************************************************************************************************************/
#if HCF_MAX_CONFIG != 0
void put_info_tbl( IFBP ifbp, LTVP ltvp	) {

hcf_16		i;
hcf_16		*p1 = ifbp->IFB_CfgTbl;
hcf_16		*p2;
wci_recordp	wp;

	while ( *p1 != 0  && ltvp->typ != *(p1+1) ) p1 = p1 + *p1 + 1;										/* 2*/
	if ( *p1 ) {																						/* 4*/
		p2 = p1 + *p1 + 1;												/* p2 := next LTV record of CfgTbl	*/
		while ( (i = *p2) != 0 ) {																		/* 6*/
			i++;
			while ( i-- ) *p1++ = *p2++;
		}
	}
															/* p1 addresses the first free location of CfgTbl*/
	i = ltvp->len + 1;																					/* 8*/
	if ( p1 + i >= &ifbp->IFB_CfgTbl[HCF_MAX_CONFIG] ) {
		HCFASSERT( DO_ASSERT, ltvp->typ )
	} else {
		wp = (wci_recordp)ltvp;																			/*10*/
#if !defined HCF_IFB_SECURE		
		if ( ltvp->typ != CFG_CNF_DEFAULT_KEYS ) 	/* suppress storing the keys in IFB unless IFN is secure*/
#endif //HCF_IFB_SECURE
		{
			*p1++ = --i;
			while ( i-- ) *p1++ = *++wp;
			*p1 = 0;																					/*12*/
		}
	}
}/* put_info_tbl */
#endif //HCF_MAX_CONFIG
//@HCFL>



