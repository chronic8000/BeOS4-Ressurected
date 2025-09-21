/****************************************************************************************
*                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/ConfigCodes.h_v   1.10   28 Jun 2000 13:42:16   lisitse  $
*
*****************************************************************************************/

 
/****************************************************************************************

File Name:			ConfigCodes.h	

File Description:	Configuration Manager Enumerated Code Types

*****************************************************************************************/


/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2000                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                             Personal Computing Division                           ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
*****************************************************************************************/




#ifndef __CONFIGCODES_H__
#define __CONFIGCODES_H__

enum tagCFGMGR_CODE
{

    CFGMGR_MS_PARAMS,                               /* +ms parameters */
    CFGMGR_EC_PARAMS,                               /* PEC_USER_CONFIG EC parameters */
    CFGMGR_COMPRESSION_PARAMS,                      /* PDS_USER_CONFIG compression parameters */
    CFGMGR_RETRAIN_ENABLED,                         /* */

    CFGMGR_CADENCE_PARAMS,                          /* CADENCE_CFG * - params of the cadence */

    /* AT-parser to LMON */
    CFGMGR_FORCE_RENEG_UP,                          /* UINT8            */
    CFGMGR_FORCE_RENEG_DOWN,                        /* UINT8            */
    CFGMGR_IN_V34FAX,                               /* UINT8            */

    /* S-Registers  - UINT8 */
	CFGMGR_SREG,									/* S0 */
	CFGMGR_NRINGS_TO_ANSWER     = CFGMGR_SREG+0,	/* Number of rings before modem answers the call */
	CFGMGR_RING_COUNTER         = CFGMGR_SREG+1,    /* Number of rings detected */
	CFGMGR_ESCAPE_CHAR          = CFGMGR_SREG+2,    /* Escape character (+) */
	CFGMGR_CR_CHAR              = CFGMGR_SREG+3,    /* Line Termination character (CR) */
	CFGMGR_LF_CHAR              = CFGMGR_SREG+4,    /* Response Formatting character (LF) */
	CFGMGR_BS_CHAR              = CFGMGR_SREG+5,	/* Command Line Editing character (BS) */
	CFGMGR_BLIND_DIAL_WAIT_TIME = CFGMGR_SREG+6,    /* Time, in seconds, the modem waits before starting to dial 
													   after going off hook when blind dialing. */
	CFGMGR_CARRIER_WAIT_TIME    = CFGMGR_SREG+7,    /* Time, in seconds, the modem waits for carrier.
													   If enabled by country options, time, in seconds, 
													   the modem waits for dialtone after 'W' dial modifier */
	CFGMGR_PAUSE_DELAY_TIME     = CFGMGR_SREG+8,    /* Time, in seconds, the modem waits after ',' dial modifier */
	CFGMGR_CARDET_RESPONSE_TIME = CFGMGR_SREG+9,    /* Time, in 0.1 seconds modem waits before reporting carrier */
	CFGMGR_HANGUP_DELAY_TIME    = CFGMGR_SREG+10,	/* Time, in tenths of a second, the modem waits before 
													   hanging up after a loss of carrier */
	CFGMGR_DTMF_DURATION        = CFGMGR_SREG+11,   /* Duration, in milliseconds, of tones in DTMF dialing */
	CFGMGR_ESCAPE_PROMPT_DELAY  = CFGMGR_SREG+12,   /* Time, in fiftieths of a second, between receiving +++
													   and sending OK to DTE */
	CFGMGR_TEST_TIMER           = CFGMGR_SREG+18,   /* Time, in seconds, that the modem conducts a test (&T1) */
	CFGMGR_FLASH_DELAY_TIME     = CFGMGR_SREG+29,   /* Time, in hundredths of a second, the modem goes on-hook 
													   after flash (!) modifier */
	CFGMGR_INACTIVITY_TIME      = CFGMGR_SREG+30,   /* Time, in tens of seconds, the modem stays online when no
													   data is sent or received */
    CFGMGR_DATA_COMPRESSION     = CFGMGR_SREG+46,   /* Selection of compression */
	CFGMGR_EXTENDED_RESULT_CODES = CFGMGR_SREG+95,/* Bits to override some of the W control options */
    CFGMGR_SREG_LAST = CFGMGR_SREG+255,				/* S-register window end */


	/* Call Progress control codes */
	CFGMGR_DIALSTRING,								/* Pointer to a dial string     LPCSTR */
	CFGMGR_PULSE_DIAL_CONFIG,						/* Pulse dial parameters    */
	CFGMGR_TONE_DIAL_CONFIG,						/* Tone dial parameters     */
	CFGMGR_DIALTONE_WAIT_TIME,						/* Time in tenths of a second the modem waits for dialtone */
	CFGMGR_DIAL_MODE,								/* Dial mode (tone or pulse      DIAL_MODE */
	CFGMGR_USE_S7_WHEN_W,							/* if TRUE, use S7 for timeout after W modifier in dial string
													   if FALSE, use CFGMGR_DIALTONE_WAIT_TIME value */
    CFGMGR_INVERT_CALLING_TONE,                     /* If TRUE - invert calling tone fro this call */

    CFGMGR_CALL_PROGRESS_TONE_DEBOUNCE,				/* Debounce times for call progress tones */
	CFGMGR_CALL_MODE,     				            /* CALL_MODE*                   */
    CFGMGR_CALL_PROGRESS_TIMING,					/* TIMING_CONFIG*. Delays       */
	CFGMGR_CALL_PROGRESS_FLAGS,						/* CALL_PROGRESS_FLAGS*         */
	CFGMGR_V8BIS_CONTROL,							/* V8BIS_OPERATION_CONTROL*		*/
	CFGMGR_V8BIS_RESULT,							/* PV8BIS_RESULT				*/
    CFGMGR_FIRST_CONNECTION_AFTER_TRAINING,         /* ??? */
    CFGMGR_PREFERRED_FLEX_VERSION,                  /* Preferred Flex version       */
    CFGMGR_V90_ENABLED,                             /* V.90 is enabled              */
    CFGMGR_V70_ALLOWED,                             /* V.70 is allowed              */

    /* AtParser Result Messages  - LPCSTR */
    CFGMGR_ATRESULT_OK,                            
    CFGMGR_ATRESULT_CONNECT,
    CFGMGR_ATRESULT_RING,
    CFGMGR_ATRESULT_NOCARRIER,
    CFGMGR_ATRESULT_ERROR,
    CFGMGR_ATRESULT_NODIALTONE,
    CFGMGR_ATRESULT_BUSY,
    CFGMGR_ATRESULT_NOANSWER,
    CFGMGR_ATRESULT_DELAYED,
    CFGMGR_ATRESULT_BLACKLISTED,
    CFGMGR_ATRESULT_FAX,
    CFGMGR_ATRESULT_DATA,
    CFGMGR_ATRESULT_FCERROR,

    /* MCR Messages  - LPCSTR */
    CFGMGR_MCR_B103,
	CFGMGR_MCR_B212,
	CFGMGR_MCR_V21,
	CFGMGR_MCR_V22,
	CFGMGR_MCR_V22B,
	CFGMGR_MCR_V23,
	CFGMGR_MCR_V32,         // = 300
	CFGMGR_MCR_V32B,
	CFGMGR_MCR_V34,
	CFGMGR_MCR_K56,
	CFGMGR_MCR_V90,

    CFGMGR_ER_NONE,
    CFGMGR_ER_LAPM,
    CFGMGR_ER_ALT,

    CFGMGR_DR_ALT,
    CFGMGR_DR_V42B,
    CFGMGR_DR_NONE,


    /* ATIn Messages - LPCSTR */
    CFGMGR_ATMESSAGE_I0,
    CFGMGR_ATMESSAGE_I1,
    CFGMGR_ATMESSAGE_I2,
    CFGMGR_ATMESSAGE_I3,
    CFGMGR_ATMESSAGE_I4,
    CFGMGR_ATMESSAGE_I5,
    CFGMGR_ATMESSAGE_I6,
    CFGMGR_ATMESSAGE_I7,
    CFGMGR_ATMESSAGE_I8,
    CFGMGR_ATMESSAGE_I9,
    CFGMGR_ATMESSAGE_I10,
    CFGMGR_ATMESSAGE_I11,
    CFGMGR_ATMESSAGE_I12,

    /* AT+GXX Messages  - LPCSTR    */
    CFGMGR_ATMESSAGE_MANUFACTURER,      /* AT+GMI responce  */
    CFGMGR_ATMESSAGE_MODEL,             /* AT+GMM responce  */
    CFGMGR_ATMESSAGE_REVISION,          /* AT+GMR responce  */
    CFGMGR_ATMESSAGE_SERIAL_NUM,        /* AT+GSN responce  */
    CFGMGR_ATMESSAGE_GOI,               /* AT+GOI responce  */
    CFGMGR_ATMESSAGE_CAPABILITIES,      /* AT+GCAP responce */

    /* AT+FMX Messages  - LPCSTR    */
    CFGMGR_FAXMESSAGE_MANUFACTURER,     /* AT+FMI responce  */
    CFGMGR_FAXMESSAGE_MODEL,            /* AT+FMM responce  */
    CFGMGR_FAXMESSAGE_REVISION,         /* AT+FMR responce  */

    /* DTE configuration    */
    CFGMGR_DTE_ECHO,                    /* UINT8            */
    CFGMGR_DTE_CONFIG,                  /* DTE_USER_CONFIG  */

    CFGMGR_SYNC_MODE,                   /* AT&Q             */

    CFGMGR_RLSD_BEHAVIOR,               /* UINT8            */
    CFGMGR_DTR_BEHAVIOR,                /* UINT8            */

    /* Speaker Control */
    CFGMGR_SPEAKER_VOLUME,              /* UINT8            */
    CFGMGR_SPEAKER_CONTROL,             /* UINT8            */

    /* Pulse Dial Make/Break Ratio */
    CFGMGR_PULSE_MAKE_BREAK,            /* UINT8            */

    /* Ring burst frequencies */
    CFGMGR_RING_BURST,                  /* PRING_BURST_PARAMS       */

    /* Answer tone detector params                                  */
    CFGMGR_ANSWER_TONE_DETECTOR,        /* PANSWER_TONE_PARAMS      */
    /* Bell answer (2225) tone detector params                      */
    CFGMGR_BELL_TONE_DETECTOR,          /* PANSWER_TONE_PARAMS      */
    /* DTMF generator parameters                                    */
    CFGMGR_DTMF_GENERATOR,              /* PDTMF_GEN_PARAMS         */

    /* Modulation Report                                            */
    CFGMGR_MODULATION_REPORT,           /* UINT8                    */

    /* Cadences                                                     */
    CFGMGR_CADENCE_BASE,
    CFGMGR_BUSY_TONE_CADENCE = CFGMGR_CADENCE_BASE,
    CFGMGR_RING_CADENCE,
    CFGMGR_REORDER_TONE_CADENCE,
    CFGMGR_SDIAL_TONE_CADENCE,
    CFGMGR_FAX_CALL_TONE_CADENCE,

    /* FAX Service Class 1 Configuration Values                     */
    CFGMGR_FAX_AUTO_ANSWER,             /* UINT8                    */

    /*  OEM flags                                                   */
    CFGMGR_OEM_FLAGS,                   /* OEMFlagStructure         */
    CFGMGR_OEM_FILTERS,                 /* OEMFilterStructure       */
    CFGMGR_OEM_SPKR_MUTE_DELAY,         /* UINT16 SpkrMuteDelay     */
	CFGMGR_OEM_READ_MASK,				/* UINT16 OEMReadMask       */
    CFGMGR_OEM_THRESHOLD,				/* OEMThresholdStruct       */

    CFGMGR_COUNTRY_CODE,                /* current country code     */
                                        /* Note: DO NOT USE THIS CODE TO CHANGE THE COUNTRY !!! */
                                        /* USE CONFIGMGR_CONTROL_CHANGE_COUNTRY CONTROL CODE INSTEAD */
    CFGMGR_PREVIOUS_COUNTRY_CODE,       /* previous country code    */
    CFGMGR_COUNTRY_NAME,                /* name of current country  */
    CFGMGR_COUNTRY_CODE_LIST,           /* list of supported country codes */
    CFGMGR_COUNTRY_STRUCT,              /* read the whole country structure */
    CFGMGR_COUNTRY_BLIST,              
	CFGMGR_FILTERS,						/* CntryTxlevelStructure    */
	CFGMGR_DTMF,						/* CntryDTMFStructure       */
	CFGMGR_RING_PARAMS,					/* Ring Detector timing		*/
	CFGMGR_RLSDOFFSET,					/* RLSDXrthOffset           */
	CFGMGR_THRESHOLD,
	CFGMGR_TXLEVELS,					/* CntryTxlevelStructure    */
	CFGMGR_RELAYS,						/* Relays[LAST_RELAY_ENTRY] */
	CFGMGR_SPEED_ADJUST,				/* CntryAgressSpeedIndexStructure a.k.a CountrySpeedAdjust */
    CFGMGR_SREG_LIMITS,                 /* limits for S-registers   */
    CFGMGR_PR_IGNORE_TIME,              /* ignore polarity reversal for specified non-zero time */
    CFGMGR_CID_TYPE,                    /* CID type */

    CFGMGR_PROFILE_STORED,        		/* PPROFILE_DATA            */
    CFGMGR_PROFILE_FACTORY,        		/* PPROFILE_DATA            */

    CFGMGR_POUND_UD,                    /* #UD/#UG structure        */

    CFGMGR_ATPARSER_ONLINE,              /* UINT8                   */
    CFGMGR_BLACK_LIST,                  /*  Black List structure    */

    CFGMGR_LAST

};




#endif      /* #ifndef __CONFIGCODES_H__ */
