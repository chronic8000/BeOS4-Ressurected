
/**************************************************************************************************************
*
* FILE	  : hcfio.cpp
*
* DATE    : $Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR  : Nico Valster
*
* DESC    : WCI-II HCF I/O Support Routines
*           These routines are isolated in their own *.CPP file to facilitate porting
*
*           Customizable via HCFCFG.H which is included by HCF.H
*
***************************************************************************************************************
* COPYRIGHT (c) 1994, 1995		 				by AT&T.	 			All Rights Reserved.
* COPYRIGHT (c) 1996, 1997, 1998, 1999, 2000	by Lucent Technologies.	All Rights Reserved.
**************************************************************************************************************/

/****************************************************************************
$Log: hcfio.c,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.34   11 Feb 2000 14:19:28   NVALST
 * sync with home
 * 
 *    Rev 1.33   11 Feb 2000 10:18:10   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.32   08 Feb 2000 09:28:36   NVALST
 * passed to reinier
 * 
 *    Rev 1.31   04 Feb 2000 14:44:46   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.30   04 Feb 2000 14:10:20   NVALST
 * intermediate, backup before inserting CDOC comments
 * 
 *    Rev 4.6   01 Feb 2000 12:02:34   NVALST
 * 
 *    Rev 4.6   28 Jan 2000 15:39:52   NVALST
 * 
 *    Rev 1.29   28 Jan 2000 13:27:38   NVALST
 * 
 *    Rev 4.5   30 Nov 1999 15:12:44   NVALST
 
 *    Rev 1.27   27 Jan 2000 10:53:30   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 17:23:56   NVALST
 * 
 *    Rev 4.4   05 Nov 1999 15:27:56   NVALST
 * 
 *    Rev 1.8   03 Nov 1999 16:42:56   NVALST
 * 
 *    Rev 4.2   10 Sep 1999 11:28:44   NVALST
 * 
 *    Rev 1.188   10 Sep 1999 11:25:10   NVALST
 * 
 *    Rev 4.2   09 Sep 1999 13:34:28   NVALST
 * 
 *    Rev 1.187   07 Sep 1999 13:23:34   NVALST
 * 
 *    Rev 4.1   31 Aug 1999 12:14:42   NVALST
 *
 *    Rev 1.182   30 Aug 1999 11:58:38   NVALST
 *    Rev 4.0   07 Jul 1999 10:09:24   NVALST
 *
 *    Rev 1.169   07 Jul 1999 09:45:04   NVALST
 *    Rev 2.22   29 Jun 1999 11:32:36   NVALST
 *
 *    Rev 1.168   24 Jun 1999 11:19:48   NVALST
 *    Rev 2.21   23 Jun 1999 09:27:42   NVALST
 *
 *    Rev 1.167   22 Jun 1999 14:24:22   NVALST
 *    Rev 2.20   21 Jun 1999 15:19:22   NVALST
 *
 *    Rev 1.164   17 Jun 1999 13:43:12   NVALST
 *    Rev 2.18   19 May 1999 12:34:00   NVALST
 *
 *    Rev 1.161   18 May 1999 10:12:26   NVALST
 *    Rev 2.17   12 May 1999 12:11:42   NVALST
 *
 *    Rev 1.160   12 May 1999 11:37:24   NVALST
 *    Rev 2.16   21 Apr 1999 09:50:00   NVALST
 *
 *    Rev 1.146   20 Apr 1999 16:05:08   NVALST
 *    Rev 2.15   26 Mar 1999 16:41:34   NVALST
 *
 *    Rev 1.128   26 Mar 1999 14:15:22   NVALST
**************************************************************************************************************/


/**************************************************************************************************************
*
* CHANGE HISTORY
*
  961121 - NV
    Original Entry

*
* ToDo
*

 - the CNV_LITTLE_TO_INT does have the desired effect on all platforms, but it's naming is
   misleading, so revisit all these CNV macros to assure the right name is used at the right
   place. Hint: introduce CNV_HOST_TO_NETWORK names if appropriate

*
* Implementation Notes
*
 -	C++ style cast is not used to keep DA-C happy
 -	a leading marker of //! is used. The purpose of such a sequence is to help the
	(maintenance) programmer to understand the flow
 	An example in hcf_action( HCF_ACT_802_3 ) is
	//!				rc = HCF_SUCCESS;
	which is superfluous because rc is known to have that value at that point but
	it shows to the (maintenance) programmer it is an intentional omission at
	the place where someone could consider it most appropriate at first glance
**************************************************************************************************************/


#include "hcf.h"
#include "hcfdef.h"

#ifdef HCF_ASSERT       /*<HCFL@*/
static struct {
	hcf_16	len;					//length of assert_strct
	hcf_16	trace;					//trace log copy from IFB
	hcf_16	qualifier;				//qualifier from entry parameter
	hcf_16	line_number;			//line number from entry parameter
	char val[sizeof(__FILE__)];
	char align;
} BASED assert_strct = { 3 + (sizeof(__FILE__)+1)/sizeof(hcf_16), 0, 0, 0, __FILE__, '\0' };
						//see hcf.c declaration of assert_strct for expanation
									
#endif // HCF_ASSERT /*@HCFL>*/


/*  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	
 *	Refer to HCFCFG.H for more information on the routines ips and ops (short for InPutString
 *	and OutPutString)
 *	
 *  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined HCF_STRICT
void ips( hcf_io prt, wci_bufp dst, int n) {

	while ( n-- ) {
		*(hcf_16 FAR*)dst = IN_PORT_WORD( prt );
		dst += 2;
	}
} // ips

void ops( hcf_io prt, wci_bufp src, int n) {

	while ( n-- ) {
		OUT_PORT_WORD( prt, *(hcf_16 FAR*)src );
		src  += 2;
	}
} // ops
#endif // HCF_STRICT
/*FF***************************************************************************************************************
FUNCT: hcfio_string                                                                              2000/02/04 14:15
USERS: hcf_get_data                                     hcf_get_info                                     
       hcf_put_data                                     hcf_send                                         
       hcf_send_diag_msg                                hcf_service_nic                                  
       alloc                                            isr_info                                         
       put_info                                         put_info_mb                                      
CALLS: HCF_DEBUG_TRIGGER                                HCFASSERT                                        
       IN_PORT_WORD                                     HCFLOG                                           
       OUT_PORT_WORD                                    IN_PORT_BYTE                                     
       DIV_BY_2                                         IN_PORT_STRING                                   
       CNV_LITTLE_TO_INT                                OUT_PORT_STRING                                  
       OUT_PORT_BYTE                                    
DEFIN: BAP_0                                            CARD_STAT_DEFUNCT                                
       DO_ASSERT                                        HCF_ALIGN                                        
       HCF_ASSERT_MISC                                  HCF_LOG_BAP_SETUP                                
       HCF_LOG_DEFUNCT                                  HCF_MAGIC                                        
       HCMD_BUSY                                        HREG_DATA_0                                      
       HREG_OFFSET_0                                    HREG_OFFSET_1                                    
       HREG_OFFSET_ERR                                  HREG_SELECT_0                                    
       HREG_SW_0                                        IO_IN                                            
       IO_OUT_CHECK                                     
GLOBL: HCF_ERR_DCWA                                     HCF_ERR_DEFUNCT_TIME_OUT                         
       HCF_ERR_NO_NIC                                   HCF_SUCCESS                                      
       ifbp->IFB_HCF_Tallies.EngCnt                     ifbp->IFB_HCF_Tallies.MiscErr                    
PARAM: bap                                              fid                                              
       ifbp                                             ifbp->IFB_BAP_0                                  
       ifbp->IFB_BAP_1                                  ifbp->IFB_CardStat                               
       ifbp->IFB_DefunctStat                            ifbp->IFB_IOBase                                 
       ifbp->IFB_PIFRscInd                              ifbp->IFB_TickIni                                
       offset                                           pc_addr                                          
       tot_len                                          type                                             
       word_len                                         
LOCAL: cp                                               i                                                
       io_port                                          p1                                               
       prot_cnt                                         rc                                               
       tlen                                             wp                                               
******************************************************************************************************************/
/***************************************** DOCZ Header ********************************************************


.MODULE         hcfio_string
.MODULE         hcf_io_string   //;?just for not yet up too speed AWK script
.LIBRARY        HCF_SUP
.TYPE           function
.SYSTEM         msdos
.SYSTEM         unix
.SYSTEM         NW4
.APPLICATION    I/O Support for HCF routines
.DESCRIPTION    read/write string with specified length from/to WaveLAN NIC RAM to/from PC RAM


int hcfio_string( IFBP ifbp, int bap, hcf_16 fid, hcf_16 offset, wci_bufp pc_addr,
				  int word_len, int tot_len, int type );
.ARGUMENTS
  IFBP			ifbp			address of I/F Block
  int			bap				BAP0/1
  int			fid				FID/RID
  int			offset			offset in FID/RID
  wci_bufp		pc_addr			begin address in PC RAM to write to or read from
  int			word_len		number of leading words of which the Endianess must be converted
  int			tot_len			number of bytes to write or read
  int			type            action code
								  -	IO_IN			read from NIC RAM
								  -	IO_OUT			write to NIC RAM
								  -	IO_OUT_CHECK	Data Corruption Detect (possibly combined with IO_OUT)

.RETURNS
	HCF_SUCCESS     	O.K
	HCF_ERR_TIME_OUT    BAP can not be initialized
	HCF_ERR_NO_NIC		card is removed
	HCF_ERR_DCWA		Data Corruption Detection catched

.NARRATIVE

  hcfio_string has the following tasks:
  -	copy data from NIC RAM to Host RAM or vice versa
  - optionally convert the data or part of the data from/to Little Endian format (as used by the NIC)
  	to/from the Native Endian format (as used by the Host)
  -	check for Data Corruption in the data written to the NIC
	
  Data is a string with specified length copied from/to a specified offset in a specified Receive Frame
  Structure (FID), Transmit Frame Structure (FID) or Record (RID) to/from a Host RAM buffer with a specified
  begin address.
  A length of 0 can be specified, resulting in no data transfer. This feature accomodates MSFs in certain
  Host environments (i.e. ODI) and it is used in the Data Corruption detection.
  Which Buffer Acces Path (BAP0 or BAP1) is used, is defined by a parameter.
  A non-zero return status indicates:
  -	the selected BAP could not properly be initialized
  -	the card is removed before completion of the data transfer
  - the Data Corruption Detection triggered
  - the NIC is considered inoperational due to a time-out of some Hermes activity in the past
  In all other cases, a zero is returned.
  If a card removal is returned, the MSF has the option to drop the message or recover in any other way it
  sees fit.
  BAP Initialization failure indicates an H/W error which is very likely to signal complete H/W failure. Once
  a BAP Initialization failure has occured all subsequent interactions with the Hermes will return a "defunct" 
  status till the Hermes is re-initialized by means of an hcf_action with HCF_ACT_CARD_IN parameter.

  Prepares access via BAP-mechanism to a FID or a RID. A FID or RID is the handle supplied by the Hermes to 
  the Host to access the buffer structures located in NIC RAM
  A BAP is a set of registers (Offset, Select and Data) offering read/write access to a particular FID or RID.
  This access is based on a auto-increment feature.
  There are two BAPs. The identification of the BAP by bap_id is constructed to make the implementation 
  convenient. Bap_id identifies the Hermes data register (either Data0 or Data1) to be used. This is the 
  entity that is more often addressed than the two other registers associated with a BAP. Note that the 
  offsets between the members of the 3 pairs (HREG_SELECT_0/_1, HREG_OFFSET_0/_1 and HREG_DATA_0/_1) is the 
  same. so the HCF can use arithmetic like "bap_id - HREG_DATA_0 + HREG_SELECT_0" to identify either 
  HREG_SELECT_0 or HREG_SELECT_1 depending on bap_id.
  
  The BAP-mechanism is based on the Busy bit in the Offset register (see the Hermes definition). The waiting 
  for Busy must occur between writing the Offset register and accessing the Data register. The implementation 
  to wait for the Busy bit drop implies that the requirement that the Busy bit is low  before the Select 
  register is written, is automatically met. The wait for Busy bit drop is protected by a loop counter, 
  which is initialized with IFB_TickIni, which is calibrated for a 2.5 second interval.

  The DataCorruptionWorkAround scheme is based on writing fragmented but contiguous data to a single
  FID or RID without (re-)initializing the BAP for the subsequent accesses. By logging which FID/RID value
  and which offset value was used by BAP_0 and BAP_1 respectively, it can be determined whether a call to
  bap_ini represents the "next" fragment of the "initial" fragment.  The offset-field of the offset/RID/FID
  tuple in the IFB represents the location just beyond the last access. This location is determined as
  the sum of the offset and len parameter.


.DIAGRAM

 2:	the test on rc checks whether the HCF went into "defunct" mode ( e.g. BAP initialization or a call to 
 	cmd_wait did ever fail).
 4:	The PCMCIA card can be removed in the middle of the transfer. By depositing a "magic number" in the
	HREG_SW_0 register of the Hermes at initialization time and by verifying this location reading the 
	string, it can be determined whether the card is still present and the return status is set accordingly.
 5:	DCWA W2DN176B1 "another layer of workaround"
 6:	Assuming no problem was detected in the previous step (indicated by prot_cnt), the test on offset and fid 
 	in the IFB_BAP_<n> structure corresponding with the BAP entry parameter assures that the BAP is only 
 	initialized if the current set of parameters specifies a location wich is not consecutive with the last 
 	read/write access. If initialization is needed, then:
	  -	the select register is set
	  -	the offset register is set
	  -	the IFB_BAP_<n> structure is initialized
	  - the offset register is monitored till a successful condition (no busy bit) is detected or till the 
	    protection counter (calibrated at approx 2.5 second) expires
	If the counter expires, this is reflected in IFB_DefunctStat, so all subsequent calls to hcfio_string
	fail immediately ( see step 1)
 8: type == IO_IN and len == 0 is used as a way to set the BAP for the future, e.g. at the end of hcf_send	
10:	HREG_OFFSET_ERR is ignored as error because it can be induced by the MSF level, e.g. by calling
	hcf_get_data while there is no Rx-FID available. Since this is an MSF-error which is caught by ASSERT,
	there is no run-time action required by the HCF. Lumping it in with the Busy bit error, as has been done
	in the past turns out to be a disaster or a life saver, just depending on what the cause of the error
	is. Since no prediction can be done about the future, it is "felt" to be the best strategy to ignore
	this error. One day the code was accompanied by the following comment:
	//	ignore HREG_OFFSET_ERR, someone (supposedly the MSF programmer ;) made a bug. Since we don't know
	//	what is going on, we might as well go on - under management pressure - by ignoring it
12:	a non-dropping Busy bit is considered to be a such serious problem that NIC access is blocked till
	the next hcf_action( HCF_ACT_CARD_IN)
14:	the offset register in the IFB_BAP_<n> structure is updated to be used as described in item 3 above
 	on the next call
16:	The NIC I/F is optimized for word transfer but it can only handle word transfer at a word boundary in 
	NIC RAM. Therefore an additional test must be done to handle the read preparation in case the begin 
	address in NIC RAM is odd.
    This situation is handled by first reading a single byte and secondly reading a string of WORDS with a
    BYTE length of the requested length minus 1.
	NOTE: MACRO IN_PORT_STRING possibly modifies p (depending on how the MSF-programmer chooses to construct
	this macro, so pc_addr can not be used as parameter
18:	At completion of the word based transfer, a test is made to determine whether 1 additional byte must be
	read (odd length starting at even address or even length starting at odd boundary)
	Note that the value of pc_addr+tot_len is not changed by the compensating manipulation performed by the 
	byte manipulation for an odd offset value (in 16)
20: finally the optionally conversion of the first words from Little Endian to Native Endian format is done.
22: As for the IO_IN part, the logic must first cope with odd begin addresses in NIC RAM and the bulk of the
	transfer is done via OUT_PORT_STRING. Due to a flaw in the Hermes, writing the high byte corrupts the
	low byte. As a work around, the HCF reads the low byte deposited in NIC RAM by the previous
	hcfio_string, merges that byte with the first byte of the current Host RAM buffer into a word and
	writes that word to NIC RAM via OUT_PORT_WORD. Since OUT_PORT_WORD converts from Native Endian to
	Little Endian, while at this point of the procedure the Host buffer must have the correct Endianess,
	the macro CNV_LITTLE_TO_INT counteracts this unwanted adjustment of OUT_PORT_WORD.
24: The (optional) conversion of the first words from Native Endian to Little Endian format is done.
	This implies that Endian conversion can ONLY take place at word boundaries.
	Note that the difference with the IO_IN part of the logic is based on optimization considerations (both
	speed and size) and the boundary condition to return the output buffer unchanged to the caller
26: At completion of the word based transfer, a test is made to determine whether 1 additional byte must be
	written
	Note that the value of pc_addr+tot_len is not changed by the compensating manipulation performed by the 
	byte manipulation for an odd offset value (in 22)
30: The case of Data Corruption Detection:
	First the NIC RAM pointer is aligned on a word boundary to cope with the problem of an odd number of
	bytes written before. This is done by skipping one additional byte before the Data Corruption
	Detection Pattern is appended to the data already written to the NIC RAM.
	Then 8 bytes fixed pattern is written. The justification of this is given in the NOTICE section below
32: In order to read the pattern back, the BAP must be initialized to address the begin of the pattern.
	BE AWARE: Although the select register does not change, so according to the Hermes - Host I/F
	specification only the offset register needs to be written, followed by a wait for successful
	completion, thus turns out to suffer from a race condition inside the Hermes which is described as
	DCWAWA in W2DN176. Workaround is to write both the Select register and the Offset register. With that 
	strategy the wait signal on the bus will prevent the race condition.
	BE AWARE 2: Although the write of the select register is a 100% correct solution, it turned out to be
	difficult to implement in the (non-HCF) based automatic test S/W. Therefor the HCF is changed to use a
	read rather than a write of the select register, which has the same effect based on the wait signal
	To assure that the compiler does not optimize this good-for-nothing read away, it is added to prot_cnt.
40: To recognize the case that the NIC is removed during execution of hcfio_string, the same check as in
	step 2 is done again.
46: A (relative) cheap way to prevent that failing I/O results in run-away behavior because garbage on the
	stack used as a local variable passed to hcfio_string is interpreted by the caller of hcfio_string
	irrespective of the return status van hcfio_string (e.g. hcf_service_nic has this behaviour).
	Note that in the case of odd buffer addresses, the last byte is not cleared due to the decrement of
	totlen in the read process for the first byte.
99:	In the past, one of the asserts in bap_ini (which no longer exists now it is assimilated in hcfio_string)
	catched if "offset" was invalid. By calling bap_ini with the original (offset + length), bap_ini would
	catch when the MSF passes the RID/FID boundary during the read process. It turned out that this feature
	did obscure the tracing during debugging so much that its total effect on the debugging process was
	considered detrimental, however re-institution can be considered depending on the bug you are chasing


.NOTICE
The problem is that without known cause, the Hermes internal NIC RAM pointer misses its auto-increment causing
two successive writes to NIC RAM to address the same location. As a consequence word <n> is overwritten and
a word <n+j> is written at position <n+j-1>. Since the Hermes is unaware of this, nothing in the system is
going to catch this, e.g. the frame is received on the other side with correct FCS. As a workaround, the HCF
keeps track of where the NIC RAM pointer SHOULD be. After all hcf_put_data calls are done, in other words,
when hcf_send is called, the HCF writes a number of words - the kludge pattern - after the MSF-data. Then it
sets the NIC RAM pointer by means of a re-initialization of the BAP to where this kludge pattern SHOULD be and
reads the first word. This first word must match the kludge pattern, otherwise, apparently, the auto-increment
failed. We need to write more than 1 word otherwise if the previous use of that piece of NIC RAM would have
left by chance the right "kludge" value just after the newly but incorrectly put data, the test would not
trigger. By overwriting the next 3 words as well, we assume the likelihood of this to be sufficiently small.
The frequency observed of this problem varies from 1 out of 1000 frames till to low to be noticeable. Just to
be on the safe side we assume that the chance of an error is 10E-3. By writing 4 words we reduce the
likelihood of an unnoticed problem to 10E-12, but of course this becomes tricky because we don't know whether
the chances are independent. Note that the HCF notion of the NIC RAM pointer is a "logical" view in the
RID/FID address space, while the Hermes has a completely different physical address value for this pointer,
however that difference does not influence above reasoning.

.NOTE
  Depending on the selected optimization options, using "register int reg" causes some obscure
  optimization with si. As a result the code is longer. Therefore, do not invest time to optimize this code
  that way during a "maintenance cycle".

.NOTE
  The IN_/OUT_PORT_WORD_/STRING macros are MSF-programmer defined and may or may not have side effects
  on their parameters, therefore they can not handle expressions like "len/2". As a solution all these macros
  must be called via "simple" variables, with no side effects like ++ and their contents is unpredictable
  at completion of the macro

.NOTE
  The implementation is choosen to have input, output and BAP setup all rolled into one monolithic
  function rather than a more ameniable hcfio_in_string, hcfio_out_string and bap_ini to minimize the
  stack usage during Interrupt Service (especially relevant for DOS drivers)

.NOTE
  The local variable io_port corresponds with a register of the appropriate BAP. This is possible by the
  intentional choice of the addresses of the individual registers of the two BAPs and the macro used to
  specify whether BAP_0 or BAP_1 should be used. The value of io_port is changed in the flow of hcfio_string
  because, depending on the context, io_port is most optimal addressing the offset register or the data register.

.NOTE 1
 When hcf_get_info is called for a non-existing RID, hcfio_string will be called with a request to read
 0 bytes (being the Hermes reported RID lenght) and 1 word to be converted to Little Endian (being the
 assumed type). This cause this ASSERT to trigger, but that is considered an acceptable nuisance.

.NOTE 2
  writing/reading words which must be converted from Big to Little Endian or vice versa, takes place only
  initial, never at odd NIC RAM addresses (presumably not at odd PC addresses either, but that assumption
  is not asserted)

 .ENDOC                          END DOCUMENTATION

-------------------------------------------------------------------------------------------------------------*/


int hcfio_string( IFBP ifbp, int bap, hcf_16 fid,
				  hcf_16 offset, wci_bufp pc_addr, int word_len, int tot_len, int type ) {

hcf_io		io_port = ifbp->IFB_IOBase + bap;										//io_port = data register
hcf_32		prot_cnt = ifbp->IFB_TickIni>>1;
hcf_16  	*p1 = bap == BAP_0 ? ifbp->IFB_BAP_0 : ifbp->IFB_BAP_1;
wci_bufp	cp;
wci_recordp	wp = (wci_recordp)pc_addr;
int			rc;
int			i, tlen;

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0021, fid );
//nv-	HCFASSERT( MUL_BY_2(word_len) <= tot_len, word_len )
	HCFASSERT( word_len == 0 || ((long)pc_addr & (HCF_ALIGN-1) ) == 0, (hcf_16)(long)pc_addr )	/* see note 1 */
	HCFASSERT( word_len == 0 || (offset & 01) == 0, offset )									/* see note 2 */

	if ( ( rc = ifbp->IFB_DefunctStat ) == HCF_SUCCESS ) {												/* 2 */
	    if ( IN_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_0 ) != HCF_MAGIC ) rc =  HCF_ERR_NO_NIC;			/* 4 */
	}	
	if ( rc == HCF_SUCCESS ) {
		/* make sure all preceeding BAP manipulation is settled */
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x0024 );
		HCFLOG( ifbp, HCF_LOG_BAP_SETUP, type, offset );
		while ( prot_cnt &&
				( IN_PORT_WORD( ifbp->IFB_IOBase + HREG_OFFSET_0 ) & HCMD_BUSY ||
				  IN_PORT_WORD( ifbp->IFB_IOBase + HREG_OFFSET_1 ) & HCMD_BUSY )
			  ) prot_cnt--;																				/* 5 */
		HCFLOG( ifbp, HCF_LOG_BAP_SETUP, fid, prot_cnt == 0 ? -1 : tot_len );
		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8024 );
//		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, prot_cnt );
		HCFASSERT( prot_cnt, IN_PORT_WORD( ifbp->IFB_IOBase + HREG_OFFSET_0 ) )
//		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8025 );
		HCFASSERT( prot_cnt, IN_PORT_WORD( ifbp->IFB_IOBase + HREG_OFFSET_1 ) )
	
		if ( prot_cnt && ( offset != *p1 || fid != *(p1+1) ) ) {										/* 6 */
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8026 );
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, fid );
			io_port = io_port - HREG_DATA_0 + HREG_SELECT_0;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, io_port );
//			OUT_PORT_WORD( io_port - HREG_DATA_0 + HREG_SELECT_0, fid );
			OUT_PORT_WORD( io_port, fid );
			io_port = io_port + HREG_DATA_0 - HREG_SELECT_0;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8027 );	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			io_port = io_port - HREG_DATA_0 + HREG_OFFSET_0;
//			OUT_PORT_WORD( io_port - HREG_DATA_0 + HREG_OFFSET_0, offset & 0xFFFE );
			OUT_PORT_WORD( io_port, offset & 0xFFFE );
////		io_port = io_port + HREG_DATA_0 - HREG_OFFSET_0;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8028 );
			*p1 = offset;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8029 );
			*(p1+1) = fid;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x802A );
			prot_cnt = ifbp->IFB_TickIni;
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x002B );
			while ( tot_len && prot_cnt && IN_PORT_WORD( io_port /* - HREG_DATA_0 + HREG_OFFSET_0*/) & HCMD_BUSY )
				prot_cnt--;																				/* 8 */
//			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x802C );
			HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 0x10, prot_cnt == 0 );
			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x8028 );
			HCFASSERT( ( IN_PORT_WORD( io_port /*- HREG_DATA_0 + HREG_OFFSET_0*/ ) & HREG_OFFSET_ERR ) == 0,
					   IN_PORT_WORD( io_port  /* - HREG_DATA_0 + HREG_OFFSET_0*/) )		//HCF_ASSERT		/* 10/
			io_port = io_port + HREG_DATA_0 - HREG_OFFSET_0;					   
		}
//		HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC, 0x802F );
		if ( prot_cnt == 0 ) {
			rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_TIME_OUT;										/* 12*/
			HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
			ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
				ifbp->IFB_HCF_Tallies.MiscErr++;													//<HCFL>
			HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0022, 0 )
//+++++		HCFASSERT( DO_ASSERT, IN_PORT_WORD( io_port - HREG_DATA_0 + HREG_OFFSET_0) )
		}
		*p1 += (hcf_16)tot_len;																			/* 14*/
	}
//-	io_port += HREG_DATA_0 - HREG_OFFSET_0;										     // io_port = data register
	if ( rc == HCF_SUCCESS && type == IO_IN ) { 														//input
		if ( tot_len ) {
			if ( offset & 0x01 ) { /*odd	*/															/* 16*/
				*pc_addr++ = IN_PORT_BYTE( io_port+1 );
				tot_len--;				//pc_addr + tot_len is immutable
			}
			tlen = DIV_BY_2( tot_len );
			cp = pc_addr;
			IN_PORT_STRING( io_port, cp, tlen );
			if ( tot_len & 1 ) *(pc_addr + tot_len - 1) = IN_PORT_BYTE( io_port );						/* 18*/
#if defined HCF_BIG_ENDIAN
			HCFASSERT( MUL_BY_2(word_len) <= tot_len, word_len )
			while ( word_len-- ) {
				CNV_LITTLE_TO_INT_NP( wp );																/* 20*/
				wp++;
			}
#endif //HCF_BIG_ENDIAN
		HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 0x11, tlen );
		}
	}
	if ( rc == HCF_SUCCESS && type != IO_IN ) {											  //output and/or check
		if ( tot_len && offset & 0x01 ) {																/* 22*/
			OUT_PORT_WORD( io_port, CNV_LITTLE_TO_INT( *pc_addr<<8 | IN_PORT_BYTE( io_port ) ) );
			pc_addr++;
			tot_len--;				//pc_addr + tot_len is immutable
		}
		tlen = DIV_BY_2( tot_len );
#if defined HCF_BIG_ENDIAN
		HCFASSERT( MUL_BY_2(word_len) <= tot_len, word_len )
		tlen -= word_len;
		while ( word_len-- ) {                                                                     		/* 24*/
			OUT_PORT_WORD( io_port, *(wci_recordp)pc_addr );
			pc_addr += 2;
		}
#endif //HCF_BIG_ENDIAN
		cp = pc_addr;
		OUT_PORT_STRING( io_port, cp, tlen );
		if ( tot_len & 1 ) OUT_PORT_BYTE( io_port, *(pc_addr + tot_len - 1) );							/* 26*/
		HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 0x11, tlen );
		if ( type == IO_OUT_CHECK ) {																	/* 30*/
			if ( *p1 & 0x01 ) (void)IN_PORT_WORD( io_port );	//align on word boundary
			OUT_PORT_WORD( io_port, 0xCAFE );
			OUT_PORT_WORD( io_port, 0xABBA );
			OUT_PORT_WORD( io_port, 0xDEAD );
			OUT_PORT_WORD( io_port, 0xD00F );
			prot_cnt = ifbp->IFB_TickIni + IN_PORT_WORD( io_port - HREG_DATA_0 + HREG_SELECT_0 );		/* 32*/
			OUT_PORT_WORD( io_port - HREG_DATA_0 + HREG_OFFSET_0, (*p1 + 1)&0xFFFE );
//--			HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 2, 0xCAFE );
			while ( prot_cnt && IN_PORT_WORD(io_port - HREG_DATA_0 + HREG_OFFSET_0) & HCMD_BUSY ) prot_cnt--;
//--			HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 2, prot_cnt == 0 );
//			again: ignore HREG_OFFSET_ERR
			if ( prot_cnt == 0 ) {
				rc = ifbp->IFB_DefunctStat = HCF_ERR_DEFUNCT_TIME_OUT;
				HCFLOG( ifbp, HCF_LOG_DEFUNCT, rc, 0 );
				ifbp->IFB_CardStat |= CARD_STAT_DEFUNCT;
				ifbp->IFB_HCF_Tallies.MiscErr++;														//<HCFL>
				HCFASSERT( DO_ASSERT, IN_PORT_WORD(io_port - HREG_DATA_0 + HREG_OFFSET_0) )
			}
			if ( IN_PORT_WORD( io_port ) != 0xCAFE ) {
//--				HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 2, 2 );
	 			HCFASSERT( DO_ASSERT, IN_PORT_WORD( io_port ) )		//!be aware of the autoincrement side effect
	 			rc = HCF_ERR_DCWA;
				ifbp->IFB_HCF_Tallies.EngCnt++;															//<HCFL>
				ifbp->IFB_PIFRscInd = 1;		//;? how to handsle error
//!			} else {
//!				rc = HCF_SUCCESS;
		    HCFLOG( ifbp, HCF_LOG_BAP_SETUP, 0x12, rc == HCF_SUCCESS );
			}
		}
	}
	if ( rc == HCF_SUCCESS ) {																			/* 40*/
	    if ( IN_PORT_WORD( ifbp->IFB_IOBase + HREG_SW_0 ) != HCF_MAGIC ) rc =  HCF_ERR_NO_NIC;
	}	
	if ( rc != HCF_SUCCESS && type == IO_IN) {															/* 46*/
		for ( i = tot_len; i; i-- ) *pc_addr++ = 0;
	}

	HCF_DEBUG_TRIGGER( ifbp, HCF_ASSERT_MISC | 0x0021, -1 );
//!	ASSERT( bap_ini( ifbp, bap, fid, (offset + len) & 0xFFFE) == HCF_SUCCESS )							/* 99*/
    return rc;
}/* hcfio_string */

