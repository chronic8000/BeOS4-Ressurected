/*  OSLESS - GMC architecture : 
 
    Because product.h is included throughout the GMC files, 
    it cannot refer to any OS-specific headerf files or structures. 
 
*/ 
#ifndef PRODUCT_H 
#define PRODUCT_H

//  ROCKV90
#ifdef __BEOS__ 
#define GENERIC_MODEM_CORE   //bp, need to remove later
#endif

//define if you want the eeprom to be into the windows 95 or NT registry instead of in a file 
#define EE_REG 
#undef SAVE_TEMP_MODEM_CONFIG
#define USE_RESOURCE_ASSIGNMENT 
#define RING3ERR 
 
//HCF is a PCI, PnP device.   
#define THIS_IS_A_PCI_PNP_DEVICE
#define THIS_IS_A_PCI_DEVICE
 
#define PCI_PNP_VENDOR_ID    0x14F1
#ifdef __BEOS__ /* used on sony platform */
#define PCI_PNP_VENDOR_ID2   0x127A
#endif
//List of device Id supported.   
#define PCI_PNP_DEVICE_ID    0x1035
#ifdef __BEOS__ /* used on sony platform */
#define PCI_PNP_DEVICE_ID2   0x1085
#else
#define PCI_PNP_DEVICE_ID2   0x1036
#endif

#define __LITTLE_ENDIAN__ 
#define VXD_PASSIVE         SWITCH_ON 
#define DEBUG_TRAP_DISPLAY  SWITCH_OFF  // Allows display in Trap() function of Comx 
#define V8BIS 
#define EARLY_SNR_CHECK // this is define to allow line_monitor to shift down the line rate 
                        // in the first three second of a connection or after a retrain. 
 
 
#include "common.h" 
 
/* SECTION 1: this section defines the type of product beeing built */ 
 
//#define  KEEP_LAST_SETTINGS 
 
//S registers tmp file , Windows 95 
#define CURRENT_S_REGISTERS_FILE  "phantom.tmp" 
#define CURRENT_DIS_FRAME_FILE    "phantom2.tmp"    
 
 
 
#define ROC_PARALLEL_MODE 
#define MODEM            
#define IRQ_SHARED //different from france setup
 
//add country definitions 
#define DEFAULT_COUNTRY    USA         /* definition in country.h */ 
//#define DEFAULT_COUNTRY    JAPAN         /* definition in country.h */ 
#define USE_COUNTRY_SREG_DEFAULT 
#undef  EXTERNAL_COUNTRY_DATABASE 
#undef  EXTERNAL_FILTER_DATABASE 
#undef  INTERNATIONAL_CALLER_ID 
 
          
#define INCLUDE_USA   
#define INCLUDE_GERMANY    
#define INCLUDE_UK         
#define INCLUDE_FRANCE     
#define INCLUDE_SWEDEN     
#define INCLUDE_SWISS      
#define INCLUDE_JAPAN      
#define INCLUDE_AUSTRIA    
#define INCLUDE_BELGIUM    
#define INCLUDE_BULGARIA      
#define INCLUDE_DENMARK    
#define INCLUDE_FINLAND    
#define INCLUDE_NORWAY    
#define INCLUDE_ITALY    
#define INCLUDE_CANADA 
#define INCLUDE_NETHERLANDS 
#ifdef USE_SMART_DAA 
#define INCLUDE_AUSTRALIA 
#define INCLUDE_KOREA 
#define INCLUDE_MALAYSIA 
#define INCLUDE_SINGAPORE 
#define INCLUDE_NEWZEALAND 
#define INCLUDE_TBR21 
#endif 
 
#define INCLUDE_IRELAND   
#define INCLUDE_PORTUGAL  
#define INCLUDE_VENEZUELA   
#define INCLUDE_ARGENTINA  
#define INCLUDE_BRAZIL  
#define INCLUDE_HONGKONG 
#define INCLUDE_MEXICO 
#define INCLUDE_POLAND 
#define INCLUDE_SOUTHAFRICA 
#define INCLUDE_GREECE 
#define INCLUDE_HUNGARY 
#define INCLUDE_CZECH 
#define INCLUDE_THAILAND 
#define INCLUDE_INDIA 
#define INCLUDE_INDONESIA 
#define INCLUDE_ISRAEL 
#define INCLUDE_PHILIPPINES 
#define INCLUDE_ROMANIA 
#define INCLUDE_RUSSIA 
#define INCLUDE_CHINA 
 
#define MODEM_CALLER_ID                 /* Support caller ID display */ 
#define MODEM_CALLER_ID_LOG             /* Support caller ID display log */ 
  
#define V25BIS          SWITCH_OFF 
#undef  V25TER 
#ifndef _WIN32_WINNT 
#define MODEM_DIAGNOSTICS                 /* Support for unimodem diagnostic command */
#endif
/* MODEM_DIAGNOSTICS needs the universal diagnostic collection mechanism */
#ifdef MODEM_DIAGNOSTICS
#define DIAGNOSTICS
#endif

  
#define DP_DETECTS_RING SWITCH_OFF 
#define DP_PULSES       SWITCH_ON 
 
#define CALLING_TONE    SWITCH_OFF 
  
#define SYNC_ENABLED    SWITCH_OFF 
  
#define V13_ENABLED     SWITCH_OFF 
  
#define V24_TESTS       SWITCH_ON 
#define DSR_DCD         SWITCH_OFF  /* &S2 command enable. DSR follows DCD */ 
  
#define FAX_MODE        SWITCH_ON 
  
#define TIES            SWITCH_ON   /* If a TIES ESCAPE SEQUENCE selected */ 
#define EFLASH          SWITCH_OFF  /* If FLASH eprom upgrade functions included */ 
  
#define VOICEVIEW       SWITCH_OFF   /* If VoiceView protocol is supported*/ 
  
#define OFF_LINE_4800HZ SWITCH_OFF   /* set this switch ON if a 4800 Hz timer 
                                     * interrupt is needed while off line. 
                                     * Otherwise the timer stays at 100Hz. 
                                     * 4800Hz timer interrupt is needed when 
                                     * the pulse dialing is done by the IMP 
                                     * and to control some features of certain 
                                     * data pumps such as dtmf dialing with the 
                                     * SGS and 68356 UDS data pumps and ring 
                                     * detect with the 68356 UDS's. When this 
                                     * switch is ON make sure that the target 
                                     * is able to generate a 4800 Hz periodic 
                                     * interrupt. 
                                     */ 
 
#define FIXED_PACING_TIMEOUT    SWITCH_OFF  // Pcing timeout not fixed at 10ms  
  
#define CMD_BUF_LNT     128     /* Command buffer length */ 
  
/* encryption options */ 
  
#define ENCRYPT                         SWITCH_OFF  /* v42bis encryption */ 
#define ENCRYPTION_ZORRO                SWITCH_OFF 
#define ENCRYPTION_BY_INTERRUPT         SWITCH_OFF 
#define SM_PORT_ENABLED                 SWITCH_OFF 
#define SPECIFIC_FAX_IDENT              SWITCH_ON   /* enable fax_mode parameter in get_own_ident() */ 
#define MAX_KEY_NEG_FRAME               256         /* maximum size of the frame received by get_frame() in V.42 */ 
#define DLCI_KEY_NEG_CODE               32          /* DCLI code use for special frame transmission */ 
#define EC_FRAME_ENABLED                SWITCH_ON 
#define LENGTH_OF_EC_FRAME_BUFFER       256         /* maximum size of the frame transmetted and received with ec frames (have to be <= 256) */ 
#define EC_FRAME_TIMOUT                 T3SEC 
#define MAX_FRAME_SIZE                  72          /* maximum size of frame info (NSF,NSS and XID) */ 
#define OAG_COMMAND_CHAR                '$'         /* character use in encryption commands */ 
  
  
/* SECTION 2: this section contains ECDC definitions */ 
  
#define ECDC_VERSION    00 
#define ECDC_REVISION   00 
#define ECDC_EDITION    00 
#define MNP10           SWITCH_OFF 
#define SREJECT         SWITCH_ON 
#define DPO_MANDATORY   SWITCH_OFF 
#define RAM_CLR_LOOP    SWITCH_OFF 
  
/* SECTION 2: this section defines the memory allocated to data */ 
/* transfer, compression and v.42 selective reject */ 
/************\ 
* DISCUSSION * 
\************/ 
/* The V.42 protocol requires a certain amount of dedicated Random      */ 
/* Access Memory. The the quantity is not limitated by the protocol so  */ 
/* it would be possible to use a large amount of memory to satisfy      */ 
/* every possible combination  of parameters. The MMA code has enforced */ 
/* some restrictions on parameters that affect memory size as follows:  */ 
/*      maximum packet size (n401) to 256 bytes.                        */ 
/*      maximum number of packets not acknowledged (k) to 31.           */ 
/* The minimum memory required to fully implement the above would be    */ 
/* the sum of :                                                         */ 
/*      a) the length of the buffer that holds unacknowledged data (TM) */ 
/*         n401 x k = 7936 bytes.                                       */ 
/*      b) the length of the selective reject buffer (SREJ)             */ 
/*         (n401 + frame header) x (k -1) = 259 x 30 = 7770 bytes       */ 
/*      c) the memory required for v42bis. Suggested dictionary is 2k   */ 
/*         2048 x number of tables = 2048 x 18 = 36864 bytes            */ 
/* The above items are the part of memory that can be varied without    */ 
/* affecting the reliability of V.42. By varying them you can alter the */ 
/* throughput but you will never get errors. After calculation we see   */ 
/* that V.42 and V.42bis can take up to 52.6 kilobytes of memory plus a */ 
/* fixed portion of about 3 kilobytes. This amount can be excessive for */ 
/* some systems so MMA has given the opportunity to the host to define  */ 
/* the amount used by the variable sections dicussed above. The         */ 
/* following paragraphs describe the impact and methods of varying      */ 
/* these parameters.                                                    */ 
  
  
/*********************************************************\ 
* buffer from DTE or compressor to error correction  (TM) * 
\*********************************************************/ 
/* TM_LNT depends on the amout of data the host wishes to transmit      */ 
/* pending acknowledgement. The error correction modules will work      */ 
/* error free as long as TM_LNT is not nul (0) but the throughput could */ 
/* be affected if the value is too low. We suggest a buffer of 2560     */ 
/* bytes which contains 10 frames of 256 characters so 2.5 kbytes could */ 
/* be sent without acknowledge. Of course the maximum unacknowledged    */ 
/* data sent is primarely governed by the parameters 'k' and 'n401' but */ 
/* is also limited by the amount of data that the buffer TM can hold.   */ 
/* + 1 frame for compressor */ 
  
  
/* this is for regular TM buff to accept 8 frames of 256 bytes */ 
/* #define TM_LNT 2304 */ 
/* this is for large TM buff to accept 32 frames of 256 bytes */ 
/*#define TM_LNT 8448  */   /* length of TM buffer */ 
/* this is for large TM buff to accept 15 frames of 256 bytes */ 
/*#define TM_LNT 4096   */ /* length of TM buffer */ 
/* this is for TM buff to accept 5 frames of 256 bytes */ 
#define TM_LNT 4096     /* length of TM buffer */ 
#define MNP_TM_LNT 4096 /* length of TM buffer when in MNP (uses v42bis dictionnary space */ 
  
  
/**************************************\ 
* selective reject buffer length (SREJ)* 
\**************************************/ 
/* If selective reject is not used SREJ_LNT can be 0. Otherwise the     */ 
/* minimum value of SREJ_LNT depends on the maximum values, permitted   */ 
/* by the host, that the parameters 'k' and 'n401' can take. This value */ 
/* is calculated as follow : SREJ_LNT = (k -1) x (n401 + packet header) */ 
/* where packet header equals 3.                                        */ 
/* example: k = 31, n401 = 256 : SREJ_LNT = 7770                        */ 
#if    SREJECT 
/*#define  SREJ_LNT 3626        */    /* 14 frames maximum */ 
#define  SREJ_LNT 1036      /* 4 frames maximum */ 
#else 
#define  SREJ_LNT 842   /* at least 842 cause v42anx uses this memory space 
                         * for header_tx, octet_buffer and tm_frame_length 
                         */ 
#endif 
  
  
/* SECTION 3: other buffer size definitions */ 
  
/*#define RR_LNT                1536 */ /* EC receive buffer size (from DCE) */ 
#define RR_LNT          4096     /* EC receive buffer size (from DCE) */ 
  
  
/* the DSP 56000 always work on 24 bits . thus we need to apply a modulo 
 * operation on some variables. processors like the 68000 will automatically 
 * apply the modulo when working on 8 bits variables 
 */ 
#ifdef  DSP56K 
#define MODULO256       &  255 
#else 
#define MODULO256   /* do nothing */ 
#endif 
  
  
#define start_of_V42_ram    (BYTE*)&TM_CNT 
#define end_of_V42_ram      (BYTE*)&TxBreakLength 
#define start_of_MNP_ram    (BYTE*)(&outstanding_la + 1) 
#define end_of_MNP_ram      (BYTE*)&max_retransmit_lr 
  
  
  
/* v42bis */ 
/******************\ 
* dictionary size  * 
\******************/ 
/* Some hosts prefer to reduce the dictionary size of v.42bis to allow  */ 
/* memory space for either TM or SREJ. The optimum dictionary size for  */ 
/* v.42bis is 2 Kbytes                                                  */ 
/* this is for 2K dictionary */ 
#define DSIZE_MAX      2048         /* dictionnary size */ 
/* this is for 1K dictionary */ 
/*#define DSIZE_MAX    1024         dictionnary size */ 
  
  
  
#define NO_CLBROTHER    SWITCH_OFF      /* if TRUE the c_lbrother buffer is not 
                                           created. This provides a ram save of 
                                           4k for a 2k dictionary. When both 
                                           the NO_CLBROTHER and NO_DLBROTHER 
                                           switches are turned ON then the code 
                                           runs 2.5% slower because of extra 
                                           search required when 
                                           the lbrother buffers are missing. */ 
#define NO_DLBROTHER    SWITCH_OFF      /* if TRUE the d_lbrother buffer is not 
                                           created. This provides a ram save of 
                                           4k for a 2k dictionary. When only 
                                           the NO_DLBROTHER switch is turned 
                                           ON the impact on processing power is 
                                           minimal because extra search is only 
                                           performed when the decompressor is 
                                           in transparent mode. */ 
  
/* Calulate the total space used for the V42bis dictionary. This depends on */ 
/* DSIZE_MAX and the values of the code switches DSP56K, and NO_CLBROTHER.  */ 
/* The dictionary uses a memory space labelled common_buffer[COMMON_BUF_SZ] */ 
/* which may be positioned anywhere in RAM (see V42bis.c). In situations    */ 
/* where this memory space is overlayed with other variables (e.g. in       */ 
/* VOICEVIEW), common_buffer[] is left undefined in 'C' and defined         */ 
/* publicly by the linker.                                                  */ 
  
/* Voiceview uses common_buffer[], so we don't want to define it here */ 
#if (VOICEVIEW == SWITCH_OFF) 
    #define COMMON_BUF_DEF_IN_C 
#endif 
  
#ifdef DSP56K 
    #if (NO_CLBROTHER) 
    #define COMMON_BUF_SZ  8*DSIZE_MAX 
    #else 
    #define COMMON_BUF_SZ  9*DSIZE_MAX 
    #endif 
#else 
    #if (NO_CLBROTHER) 
    #define COMMON_BUF_SZ  14*DSIZE_MAX 
    #else 
    #define COMMON_BUF_SZ  16*DSIZE_MAX 
    #endif 
#endif 
  
  
/* SECTION 4: this section defines the type fax beeing built */ 
  
/* version of fax modem */ 
  
/*--------------------------- standard fax ----------------------------------*\ 
 * 
 *  FAX_CLASS_1 -> 0 : disable TIA/EIA 578. 
 *                 1 : enable TIA/EIA 578. 
 * 
 *  FAX_CLASS_2 -> 0 : disable fax class 2 and fax ECM. 
 *                 1 : enable fax class 2. 
 * 
 *  ENABLE_VERSION_592 -> 0 : enable TIA/EIA 2388. 
 *                        1 : enable TIA/EIA 592. 
 * 
 *  POLLING  -> 0 : disable polling. 
 *              1 : enable polling (not working). 
 * 
 *  INTERRUPTION  -> 0 : disable interruption. 
 *                   1 : enable interruption (not working). 
 * 
 *  CONTROL_DCE_DTE_SPEED  ->  0 : the dce-dte speed is always 19200 (data). 
 *                             1 : the dce-dte speed can be change with the 
 *                                 AT+FX command. 
 * 
 *  V17_ENABLED   -> 0 : disable V17 (even for data pump that support V17). 
 *                   1 : enable V17 (for data pump that support V17). 
 * 
 *  MIN_TIME_BETWEEN_DCS_TRAIN ->   This is the time (in 1/100 of s)between the 
 *                                  transmission of the DCS frame and the beginning 
 *                                  of the training sequence. The value specified 
 *                                  t30 is 75ms (8) but a lot of fax machine dont 
 *                                  work with that value. 
 * 
 *  TRAINING_LENGTH ->              Length of the training sequence. The value 
 *                                  specified in the t.30 is 1.5 s(150). 
 * 
 * 
 *  MIN_TIME_BETWEEN_CFR_PAGE ->    This is the time (in 1/100 of s)between the 
 *                                  reception of the CFR frame and the beginning 
 *                                  of the page transmission.  The value specified 
 *                                  t30 is 75ms (8) but a lot of fax machine dont 
 *                                  work with that value. 
 * 
\*---------------------------------------------------------------------------*/ 
  
#define  FAX_CLASS_1                  SWITCH_ON
#define  FAX_CLASS_2                  SWITCH_ON
#define  ENABLE_VERSION_592           SWITCH_OFF 
#define  POLLING                      SWITCH_OFF 
#define  INTERRUPTION                 SWITCH_OFF 
#define  CONTROL_DCE_DTE_SPEED        SWITCH_OFF 
#define  V17_ENABLED                  SWITCH_ON 
#define  MIN_TIME_BETWEEN_DCS_TRAIN     30 
#define  TRAINING_LENGTH                150 
#define  MIN_TIME_BETWEEN_CFR_PAGE      50 
  
/*---------------------- error correction fax -------------------------------*\ 
 *                                                                           * 
 *  ERROR_CORRECTION -> 0 : disable ECM.                                     * 
 *                      1 : enable ECM. (fax class 2 must be enabled)        * 
 *                                                                           * 
 *  EC_NOT_INIT      -> 0 : if ECM is enable the eroor correction will be    * 
 *                          enabled when the modem is turn on.               * 
 *                      1 : the ECM mode have to be turn on with the command * 
 *                          AT+FDCC=,,,,,1.                                  * 
 *                                                                           * 
 *  TRANSPARENT_MODE -> 0 : If the fax software dont send 6xEOL at the end   * 
 *                          of a page it will be add to the page.            * 
 *                      1 : All the data is transmitted with no              * 
 *                          modifications                                    * 
 *                                                                           * 
 *  NB_BYTE_IN_TEMP_EC_BUFF ->   This constant define the number of byte of  * 
 *                               the temporary buffer.  When receiving data  * 
 *                               a part of the information is keep in the    * 
 *                               temporary buffer in case of interruption    * 
 *                               of data (waiting for a bloc, data error).   * 
 *                                                                           * 
 *  DELAY_BEFORE_TRANSMISSION -> Indicate the number of time to wait (10 ms) * 
 *                               before transmitting a byte from the         * 
 *                               temporary buffer when there is interruption * 
 *                               of data.                                    * 
 *                                                                           * 
 *  NB_BYTE_IN_TEMP_EC_BUFF and DELAY_BEFORE_TRANSMISSION are use to adapt   * 
 *  the ECM code to existing fax softwares that can't allow complete         * 
 *  interruption of data.                                                    * 
 *                                                                           * 
\*---------------------------------------------------------------------------*/ 
  
#define  ERROR_CORRECTION           SWITCH_OFF 
#define  EC_NOT_INIT                SWITCH_OFF 
#define  TRANSPARENT_MODE           SWITCH_OFF 
#define  NB_BYTE_IN_TEMP_EC_BUFF    200 
#define  DELAY_BEFORE_TRANSMISSION  150 
  
  
/*----------------------- other specifications ------------------------------*\ 
 *                                                                           * 
 *   VOICE  -> enable voice mode                                             * 
 *                                                                           * 
 *   MANUFACTURER -> string for AT+FMFR?                                     * 
 *   MODEL        -> string for AT+FMDL?                                     * 
 *   REVISION     -> string for AT+FREV?                                     * 
 *                                                                           * 
\*---------------------------------------------------------------------------*/ 
  		// VOICE not defined in france implementation
#define  VOICE                 SWITCH_ON /* voice with rockwell set of commands */ 
#define  VOICE_BUFFER_SIZE     "1024" 
#define  TYPE_OF_COMPRESSION   "IMA ADPCM"
#ifndef __BEOS__ /* BeOS is using IRQ polling, so don't need this option */
#define  TAM                    /* Tapeless answering machine */ 
#define  V253_CMD_SET
#endif
#define  SPEAKERPHONE  
#define	 TAM_HANDSET_HOOK_CONTROL
 
#define  MANUFACTURER          "Trisignal"     /* manufacturer */ 
#define  MODEL                 "Phantom"         /* model */ 
#define  REVISION              "REV 0.001"     /* revision */ 
#define  SERIAL_NUMBER_ID      "0"               
#define  OBJECT_ID             "object id as in ITU X.208"         
  
/*---------------------- for debugging only ---------------------------------*\ 
 * 
 *  DEBUG_FAX  ->  FALSE :  normal mode (no debugging) 
 *                 TRUE  :  enable special message with command AT+FBUG 
 * 
 *                 AT+FBUG = 0 : no debug message 
 *                           1 : reporting frame transmission and reception 
 *                               with +FHT and +FHR messages (can make 
 *                               communication fail in ECM mode) 
 *                          *2 : reporting the different phase called while 
 *                               in communication (report at the end). 
 *                          *3 : reporting frame transmitted and received 
 *                               at the end of the communication (better for 
 *                               ECM mode). 
 * 
 *  NB_PHASE_MEMORY  : indicate the number of phase or frame that can be 
 *                     store.  This is a circular buffer only the "x" last 
 *                     operation are reported. 
 * 
\*---------------------------------------------------------------------------*/ 
  
#define DEBUG_FAX               SWITCH_ON 
  
#if DEBUG_FAX 
#define  NB_PHASE_MEMORY           100  /* nb of phase that can be store in memory */ 
#else 
#define  NB_PHASE_MEMORY           1    /* no debug mode */ 
#endif 
  
  
/* SECTION 5: defines fixed location.*/ 
 
#define AMOUNT_OF_SREG 80        /* Total amount of SREG */ 
  
/* S registers minimum range  
*/ 
#define S_RANGE_MIN \
0x00,   /* S0_COM_MIN  */ \
0x00,   /* S1_COM_MIN  */ \
0x00,   /* S2_COM_MIN  */ \
0x00,   /* S3_COM_MIN  */ \
0x00,   /* S4_COM_MIN  */ \
0x00,   /* S5_COM_MIN  */ \
0x02,   /* S6_COM_MIN  */ \
0x01,   /* S7_COM_MIN  */ \
0x00,   /* S8_COM_MIN  */ \
0x01,   /* S9_COM_MIN  */ \
0x01,   /* S10_COM_MIN */ \
0x32,   /* S11_COM_MIN */ \
0x00,   /* S12_COM_MIN */ \
0x00,   /* S13_COM_MIN */ \
0x00,   /* S14_COM_MIN */ \
0x00,   /* S15_COM_MIN */ \
0x00,   /* S16_COM_MIN */ \
0x00,   /* S17_COM_MIN */ \
0x00,   /* S18_COM_MIN */ \
0x00,   /* S19_COM_MIN */ \
0x01,   /* S20_COM_MIN */ \
0x00,   /* S21_COM_MIN */ \
0x00,   /* S22_COM_MIN */ \
0x00,   /* S23_COM_MIN */ \
0x00,   /* S24_COM_MIN */ \
0x00,   /* S25_COM_MIN */ \
0x00,   /* S26_COM_MIN */ \
0x00,   /* S27_COM_MIN */ \
0x00,   /* S28_COM_MIN */ \
0x00,   /* S29_COM_MIN */ \
0x00,   /* S30_COM_MIN */ \
0x00,   /* S31_COM_MIN */ \
0x00,   /* S32_COM_MIN */ \
0x0A,   /* S33_COM_MIN */ \
0x00,   /* S34_COM_MIN */ \
0x00,   /* S35_COM_MIN */ \
0x00,   /* S36_COM_MIN */ \
0x00,   /* S37_COM_MIN */ \
0x00,   /* S38_COM_MIN */ \
0x00,   /* S39_COM_MIN */ \
0x00,   /* S40_COM_MIN */ \
0x00,   /* S41_COM_MIN */ \
0x00,   /* S42_COM_MIN */ \
0x00,   /* S43_COM_MIN */ \
0x00,   /* S44_COM_MIN */ \
0x00,   /* S45_COM_MIN */ \
0x00,   /* S46_COM_MIN */ \
0x00,   /* S47_COM_MIN */ \
0x00,   /* S48_COM_MIN */ \
0x00,   /* S49_COM_MIN */ \
0x00,   /* S50_MODEM_MIN */ \
0x00,   /* S51_MODEM_MIN */ \
0x00,   /* S52_MODEM_MIN */ \
0x00,   /* S53_MODEM_MIN */ \
0x00,   /* S54_MODEM_MIN */ \
0x00,   /* S55_MODEM_MIN */ \
0x00,   /* S56_MODEM_MIN */ \
0x00,   /* S57_MODEM_MIN */ \
0x00,   /* S58_MODEM_MIN */ \
0x00,   /* S59_MODEM_MIN */ \
0x00,   /* S60_MODEM_MIN */ \
0x00,   /* S61_MODEM_MIN */ \
0x00,   /* S62_MODEM_MIN */ \
0x00,   /* S63_MODEM_MIN */ \
0x00,   /* S64_MODEM_MIN */ \
0x00,   /* S65_MODEM_MIN */ \
0x00,   /* S66_MODEM_MIN */ \
0x00,   /* S67_MODEM_MIN */ \
0x00,   /* S68_MODEM_MIN */ \
0x00,   /* S69_MODEM_MIN */ \
0x00,   /* S70_MODEM_MIN */ \
0x00,   /* S71_MODEM_MIN */ \
0x00,   /* S72_MODEM_MIN */ \
0x00,   /* S73_MODEM_MIN */ \
0x00,   /* S74_MODEM_MIN */ \
0x00,   /* S75_MODEM_MIN */ \
0x00,   /* S76_MODEM_MIN */ \
0x00,   /* S77_MODEM_MIN */ \
0x00,   /* S78_MODEM_MIN */ \
0x00,   /* S79_MODEM_MIN */ 
  
  
/* S registers maximum range */ 
  
#define S_RANGE_MAX \
0xFF,   /* S0_COM_MAX  */ \
0x00,   /* S1_COM_MAX  */ \
0xFF,   /* S2_COM_MAX  */ \
0x7F,   /* S3_COM_MAX  */ \
0x7F,   /* S4_COM_MAX  */ \
0xFF,   /* S5_COM_MAX  */ \
0xFF,   /* S6_COM_MAX  */ \
0xFF,   /* S7_COM_MAX  */ \
0xFF,   /* S8_COM_MAX  */ \
0xFF,   /* S9_COM_MAX  */ \
0xFF,   /* S10_COM_MAX */ \
0xFF,   /* S11_COM_MAX */ \
0xFF,   /* S12_COM_MAX */ \
0xFF,   /* S13_COM_MAX */ \
0xFF,   /* S14_COM_MAX */ \
0xFF,   /* S15_COM_MAX */ \
0xFF,   /* S16_COM_MAX */ \
0xFF,   /* S17_COM_MAX */ \
0xFF,   /* S18_COM_MAX */ \
0xFF,   /* S19_COM_MAX */ \
0x2C,   /* S20_COM_MAX */ \
0xFF,   /* S21_COM_MAX */ \
0xFF,   /* S22_COM_MAX */ \
0xFF,   /* S23_COM_MAX */ \
0xFF,   /* S24_COM_MAX */ \
0xFF,   /* S25_COM_MAX */ \
0xFF,   /* S26_COM_MAX */ \
0xFF,   /* S27_COM_MAX */ \
0xFF,   /* S28_COM_MAX */ \
0xFF,   /* S29_COM_MAX */ \
0xFF,   /* S30_COM_MAX */ \
0xFF,   /* S31_COM_MAX */ \
0xFF,   /* S32_COM_MAX */ \
0x23,   /* S33_COM_MAX */ \
0x09,   /* S34_COM_MAX */ \
0x32,   /* S35_COM_MAX */ \
0xFF,   /* S36_COM_MAX */ \
0xFF,   /* S37_COM_MAX */ \
0xFF,   /* S38_COM_MAX */ \
0xFF,   /* S39_COM_MAX */ \
0xFF,   /* S40_COM_MAX */ \
0x0A,   /* S41_COM_MAX */ \
0xFF,   /* S42_COM_MAX */ \
0xFF,   /* S43_COM_MAX */ \
0xFF,   /* S44_COM_MAX */ \
0xFF,   /* S45_COM_MAX */ \
0x7F,   /* S46_COM_MAX */ \
0x7F,   /* S47_COM_MAX */ \
0x7F,   /* S48_COM_MAX */ \
0xFF,   /* S49_COM_MAX */ \
0xFF,   /* S50_MODEM_MAX */ \
0xFF,   /* S51_MODEM_MAX */ \
0xFF,   /* S52_MODEM_MAX */ \
0xFF,   /* S53_MODEM_MAX */ \
0xFF,   /* S54_MODEM_MAX */ \
0xFF,   /* S55_MODEM_MAX */ \
0xFF,   /* S56_MODEM_MAX */ \
0xFF,   /* S57_MODEM_MAX */ \
0xFF,   /* S58_MODEM_MAX */ \
0xFF,   /* S59_MODEM_MAX */ \
0xFF,   /* S60_MODEM_MAX  */ \
0xFF,   /* S61_MODEM_MAX  */ \
0xFF,   /* S62_MODEM_MAX  */ \
0xFF,   /* S63_MODEM_MAX  */ \
0xFF,   /* S64_MODEM_MAX  */ \
0xFF,   /* S65_MODEM_MAX  */ \
0xFF,   /* S66_MODEM_MAX  */ \
0xFF,   /* S67_MODEM_MAX  */ \
0xFF,   /* S68_MODEM_MAX  */ \
0xFF,   /* S69_MODEM_MAX  */ \
0xFF,   /* S70_MODEM_MAX  */ \
0xFF,   /* S71_MODEM_MAX  */ \
0xFF,   /* S72_MODEM_MAX  */ \
0xFF,   /* S73_MODEM_MAX  */ \
0xFF,   /* S74_MODEM_MAX  */ \
0xFF,   /* S75_MODEM_MAX  */ \
0xFF,   /* S76_MODEM_MAX  */ \
0xFF,   /* S77_MODEM_MAX  */ \
0xFF,   /* S78_MODEM_MAX  */ \
0xFF,   /* S79_MODEM_MAX  */ 
  
  
/* S registers factory default profile 0 */ 
#define S_FACT_DEF_0 \
0x00,   /* S0_COM_F0  */ \
0x00,   /* S1_COM_F0  */ \
0x2B,   /* S2_COM_F0  */ \
0x0D,   /* S3_COM_F0  */ \
0x0A,   /* S4_COM_F0  */ \
0x08,   /* S5_COM_F0  */ \
0x02,   /* S6_COM_F0  */ \
0x1E,   /* S7_COM_F0  */ \
0x02,   /* S8_COM_F0  */ \
0x01,   /* S9_COM_F0  */ \
0x0E,   /* S10_COM_F0 */ \
0x5F,   /* S11_COM_F0 */ \
0x32,   /* S12_COM_F0 */ \
0x00,   /* S13_COM_F0 */ \
0x8A,   /* S14_COM_F0 */ \
0x00,   /* S15_COM_F0 */ \
0x00,   /* S16_COM_F0 */ \
0x00,   /* S17_COM_F0 */ \
0x00,   /* S18_COM_F0 */ \
0x00,   /* S19_COM_F0 */ \
0x03,   /* S20_COM_F0 */ \
0x30,   /* S21_COM_F0 */ \
0x76,   /* S22_COM_F0 */ \
0x17,   /* S23_COM_F0 */ \
0x9C,   /* S24_COM_F0 */ \
0x05,   /* S25_COM_F0 */ \
0x01,   /* S26_COM_F0 */ \
0x40,   /* S27_COM_F0 */ \
0xC3,   /* S28_COM_F0 */ \
0x00,   /* S29_COM_F0 */ \
0x0F,   /* S30_COM_F0 */ \
0x00,   /* S31_COM_F0 */ \
0x00,   /* S32_COM_F0 */ \
0x1A,   /* S33_COM_F0 */ \
0x00,   /* S34_COM_F0 */             /* 0xff is no autologon */      \
0x0A,   /* S35_COM_F0 */ \
0xEE,   /* S36_COM_F0 */ \
0xff,   /* S37_COM_F0 */ \
0x14,   /* S38_COM_F0 */ \
0x80,   /* S39_COM_F0 */ \
0x00,   /* S40_COM_F0 */ \
0x00,   /* S41_COM_F0 */ \
0xEB,   /* S42_COM_F0 */ \
0x1E,   /* S43_COM_F0 */ \
0x00,   /* S44_COM_F0 */ \
0x00,   /* S45_COM_F0 */ \
0x0D,   /* S46_COM_F0 */ \
0x11,   /* S47_COM_F0 */ \
0x13,   /* S48_COM_F0 */ \
0x03,   /* S49_COM_F0 */ \
0x00,   /* S50_MODEM_F0 */ \
0x00,   /* S51_MODEM_F0 */ \
0xBF,   /* S52_MODEM_F0 */ \
0xFF,   /* S53_MODEM_F0 */ \
0x07,   /* S54_MODEM_F0 */ \
0x00,   /* S55_MODEM_F0 */ \
0x00,   /* S56_MODEM_F0 */ \
0x00,   /* S57_MODEM_F0 */ \
0x00,   /* S58_MODEM_F0 */ \
0x00,   /* S59_MODEM_F0 */ \
0xFF,   /* S60_MODEM_F0 */ \
0x03,   /* S61_MODEM_F0 */ \
0x00,   /* S62_MODEM_F0 */ \
0x00,   /* S63_MODEM_F0 */ \
0x00,   /* S64_MODEM_F0 */ \
0x00,   /* S65_MODEM_F0 */ \
0x0F,   /* S66_MODEM_F0 */ \
0xFA,   /* S67_MODEM_F0 */ \
0x00,   /* S68_MODEM_F0 */ \
0x00,   /* S69_MODEM_F0 */ \
0x00,   /* S70_MODEM_F0 */ \
0x00,   /* S71_MODEM_F0 */ \
0x00,   /* S72_MODEM_F0 */ \
0x00,   /* S73_MODEM_F0 */ \
0x00,   /* S74_MODEM_F0 */ \
0x00,   /* S75_MODEM_F0 */ \
0x00,   /* S76_MODEM_F0 */ \
0x00,   /* S77_MODEM_F0 */ \
0x00,   /* S78_MODEM_F0 */ \
0x00    /* S79_MODEM_F0 */                                            
  
/* S registers factory default profile 1 */ 
#define S_FACT_DEF_1 \
0x00,   /* S0_COM_F1  */ \
0x00,   /* S1_COM_F1  */ \
0x2B,   /* S2_COM_F1  */ \
0x0D,   /* S3_COM_F1  */ \
0x0A,   /* S4_COM_F1  */ \
0x08,   /* S5_COM_F1  */ \
0x02,   /* S6_COM_F1  */ \
0x1E,   /* S7_COM_F1  */ \
0x02,   /* S8_COM_F1  */ \
0x01,   /* S9_COM_F1  */ \
0x0E,   /* S10_COM_F1 */ \
0x5F,   /* S11_COM_F1 */ \
0x32,   /* S12_COM_F1 */ \
0x00,   /* S13_COM_F1 */ \
0x8A,   /* S14_COM_F1 */ \
0x00,   /* S15_COM_F1 */ \
0x00,   /* S16_COM_F1 */ \
0x00,   /* S17_COM_F1 */ \
0x00,   /* S18_COM_F1 */ \
0x00,   /* S19_COM_F1 */ \
0x03,   /* S20_COM_F1 */ \
0x30,   /* S21_COM_F1 */ \
0x76,   /* S22_COM_F1 */ \
0x17,   /* S23_COM_F1 */ \
0x9C,   /* S24_COM_F1 */ \
0x05,   /* S25_COM_F1 */ \
0x01,   /* S26_COM_F1 */ \
0x40,   /* S27_COM_F1 */ \
0xC3,   /* S28_COM_F1 */ \
0x00,   /* S29_COM_F1 */ \
0x0F,   /* S30_COM_F1 */ \
0x00,   /* S31_COM_F1 */ \
0x00,   /* S32_COM_F1 */ \
0x1A,   /* S33_COM_F1 */ \
0x00,   /* S34_COM_F1 */ \
0x0A,   /* S35_COM_F1 */ \
0x66,   /* S36_COM_F1 */ \
0xff,   /* S37_COM_F1 */ \
0x14,   /* S38_COM_F1 */ \
0x80,   /* S39_COM_F1 */ \
0x00,   /* S40_COM_F1 */ \
0x00,   /* S41_COM_F1 */ \
0xEB,   /* S42_COM_F1 */ \
0x1E,   /* S43_COM_F1 */ \
0x00,   /* S44_COM_F1 */ \
0x00,   /* S45_COM_F1 */ \
0x0D,   /* S46_COM_F1 */ \
0x11,   /* S47_COM_F1 */ \
0x13,   /* S48_COM_F1 */ \
0x03,   /* S49_COM_F1 */ \
0x00,   /* S50_MODEM_F1 */ \
0x00,   /* S51_MODEM_F1 */ \
0xBF,   /* S52_MODEM_F1 */ \
0xFF,   /* S53_MODEM_F1 */ \
0x07,   /* S54_MODEM_F1 */ \
0x00,   /* S55_MODEM_F1 */ \
0x00,   /* S56_MODEM_F1 */ \
0x00,   /* S57_MODEM_F1 */ \
0x00,   /* S58_MODEM_F1 */ \
0x00,   /* S59_MODEM_F1 */ \
0xFF,   /* S60_MODEM_F1 */ \
0x03,   /* S61_MODEM_F1 */ \
0x00,   /* S62_MODEM_F1 */ \
0x00,   /* S63_MODEM_F1 */ \
0x00,   /* S64_MODEM_F1 */ \
0x00,   /* S65_MODEM_F1 */ \
0x0F,   /* S66_MODEM_F1 */ \
0xFA,   /* S67_MODEM_F1 */ \
0x00,   /* S68_MODEM_F1 */ \
0x00,   /* S69_MODEM_F1 */ \
0x00,   /* S70_MODEM_F1 */ \
0x00,   /* S71_MODEM_F1 */ \
0x00,   /* S72_MODEM_F1 */ \
0x00,   /* S73_MODEM_F1 */ \
0x00,   /* S74_MODEM_F1 */ \
0x00,   /* S75_MODEM_F1 */ \
0x00,   /* S76_MODEM_F1 */ \
0x00,   /* S77_MODEM_F1 */ \
0x00,   /* S78_MODEM_F1 */ \
0x00    /* S79_MODEM_F1 */                                            
  
/* S registers factory default profile 2 */ 
#define S_FACT_DEF_2 \
0x00,   /* S0_COM_F2  */ \
0x00,   /* S1_COM_F2  */ \
0x2B,   /* S2_COM_F2  */ \
0x0D,   /* S3_COM_F2  */ \
0x0A,   /* S4_COM_F2  */ \
0x08,   /* S5_COM_F2  */ \
0x02,   /* S6_COM_F2  */ \
0x1E,   /* S7_COM_F2  */ \
0x02,   /* S8_COM_F2  */ \
0x01,   /* S9_COM_F2  */ \
0x0E,   /* S10_COM_F2 */ \
0x5F,   /* S11_COM_F2 */ \
0x32,   /* S12_COM_F2 */ \
0x00,   /* S13_COM_F2 */ \
0x8A,   /* S14_COM_F2 */ \
0x00,   /* S15_COM_F2 */ \
0x00,   /* S16_COM_F2 */ \
0x00,   /* S17_COM_F2 */ \
0x00,   /* S18_COM_F2 */ \
0x00,   /* S19_COM_F2 */ \
0x03,   /* S20_COM_F2 */ \
0x30,   /* S21_COM_F2 */ \
0x76,   /* S22_COM_F2 */ \
0x17,   /* S23_COM_F2 */ \
0x98,   /* S24_COM_F2 */ \
0x05,   /* S25_COM_F2 */ \
0x01,   /* S26_COM_F2 */ \
0x40,   /* S27_COM_F2 */ \
0xC3,   /* S28_COM_F2 */ \
0x00,   /* S29_COM_F2 */ \
0x0F,   /* S30_COM_F2 */ \
0x00,   /* S31_COM_F2 */ \
0x00,   /* S32_COM_F2 */ \
0x1A,   /* S33_COM_F2 */ \
0x00,   /* S34_COM_F2 */             /* 0xff is no autologon */      \
0x0A,   /* S35_COM_F2 */ \
0x22,   /* S36_COM_F2 */ \
0xff,   /* S37_COM_F2 */ \
0x14,   /* S38_COM_F2 */ \
0x80,   /* S39_COM_F2 */ \
0x00,   /* S40_COM_F2 */ \
0x00,   /* S41_COM_F2 */ \
0xEB,   /* S42_COM_F2 */ \
0x1E,   /* S43_COM_F2 */ \
0x00,   /* S44_COM_F2 */ \
0x00,   /* S45_COM_F2 */ \
0x0D,   /* S46_COM_F2 */ \
0x11,   /* S47_COM_F2 */ \
0x13,   /* S48_COM_F2 */ \
0x03,   /* S49_COM_F2 */ \
0x00,   /* S50_MODEM_F2 */ \
0x00,   /* S51_MODEM_F2 */ \
0x3F,   /* S52_MODEM_F2 */ \
0xFF,   /* S53_MODEM_F2 */ \
0x07,   /* S54_MODEM_F2 */ \
0x00,   /* S55_MODEM_F2 */ \
0x00,   /* S56_MODEM_F2 */ \
0x00,   /* S57_MODEM_F2 */ \
0x00,   /* S58_MODEM_F2 */ \
0x00,   /* S59_MODEM_F2 */ \
0xFF,   /* S60_MODEM_F2 */ \
0x03,   /* S61_MODEM_F2 */ \
0x00,   /* S62_MODEM_F2 */ \
0x00,   /* S63_MODEM_F2 */ \
0x00,   /* S64_MODEM_F2 */ \
0x00,   /* S65_MODEM_F2 */ \
0x0F,   /* S66_MODEM_F2 */ \
0xFA,   /* S67_MODEM_F2 */ \
0x00,   /* S68_MODEM_F2 */ \
0x00,   /* S69_MODEM_F2 */ \
0x00,   /* S70_MODEM_F2 */ \
0x00,   /* S71_MODEM_F2 */ \
0x00,   /* S72_MODEM_F2 */ \
0x00,   /* S73_MODEM_F2 */ \
0x00,   /* S74_MODEM_F2 */ \
0x00,   /* S75_MODEM_F2 */ \
0x00,   /* S76_MODEM_F2 */ \
0x00,   /* S77_MODEM_F2 */ \
0x00,   /* S78_MODEM_F2 */ \
0x00    /* S79_MODEM_F2 */                                            
 
#define I0_RESPONSE     "932" 
/* watch out for DOS FAX LIGTH install programm and I3 response ..... */ 
#define FW_VERSION      "56K Conexant HCF  Modem, rev 1.1" 
#define DP_VERSION      target_send_dp_version() 
#define I4_0_RESPONSE   "a097840F284C6403F" 
#define I4_1_RESPONSE   "bF60430000" 
#define I4_2_RESPONSE   "r1031111111010000" 
#define I4_3_RESPONSE   "r3000111010000000" 
 
#define UNICODE_HAL_SERVICE_NAME        L"Hcf_SpCtl" 
#define UNICODE_SERIAL_SERVICE_NAME     L"Hcf_Spser" 
#define UNICODE_PnP_ID                  L"NSC0000" 
#define UNICODE_COMPANY_NAME            L"Trisignal" 
#define UNICODE_PRODUCT_NAME            L"Phantom PCI" 
#define UNICODE_SERIAL_DEVICE_NAME      L"TrisSer0" 
#define UNICODE_CTL_DEVICE_NAME         L"TrisCTL0" 

/*  //in france implementation only
#define DEF_NUM_MEMORIES 2  
#define SERIAL_POOL_TAG_NAME        'SIRT'
#define CONTROLLER_POOL_TAG_NAME    'CIRT'
#define SERIAL_DEV_NAME             L"3ComSerialDevice"
#define CONTROLLER_DEV_NAME         L"3ComControllerDevice"
*/ 
#endif  //PRODUCT_H 
