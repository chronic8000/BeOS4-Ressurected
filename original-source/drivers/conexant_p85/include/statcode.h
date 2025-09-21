/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/StatCode.h_v   1.6   07 Jun 2000 13:23:54   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			StatCode.h

File Description:		

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


#ifndef __STAT_CODE_H__
#define __STAT_CODE_H__

typedef enum {
    
	STATUS_RX_LINE_SIGNAL_DETECT,	/* UINT32 Polarity				    */
	STATUS_PROTOCOL_CONNECT,		/* PROTOCOL_INFO *					*/
	STATUS_CTS,						/* UINT32* Polarity  (Clear To Send)*/
	STATUS_TONE_DETECTED,			/* UINT32* Polarity 				*/
	STATUS_CADENCE_DETECTED,		/* None                             */

    /* tone detectors notification events */
    STATUS_ANSWER_TONE,             /* UINT32 Polarity					*/
    STATUS_BELL_TONE,               /* UINT32 Polarity					*/

	/* Call Progress */
	STATUS_CP_RING,                 /* None	*/
	STATUS_CP_AUTOANSWER,			/* Auto answer */
    STATUS_CP_NODIALTONE,           /* None */
	STATUS_CP_NOCARRIER,            /* None	*/
	STATUS_CP_NOANSWER,             /* None	*/
	STATUS_CP_BUSY,                 /* None	*/
	STATUS_CP_CARRIER,              /* CARRIER_REPORT */
	STATUS_CP_DIALCOMPLETE,			/* BOOL (True if semicolon, false if waiting for connect */
    STATUS_CP_HANGUP,               /* Hang up complete */
    /* statuses for +a8e V8 progress indication */
    STATUS_A8E_JM,                  /* char[10]                         */
    STATUS_A8E_V8_COMPLETE,         /* UINT32                           */
    STATUS_A8E_ANSWER,              /* ANSWER_TYPE                      */
    STATUS_A8E_CI,                  /* UINT32                           */

    /*  Fax call progress */
    STATUS_V21_FLAGS_DETECTED,      /*  FLAGDT  */
    STATUS_PN_SUCCESS,              /*  PNSUC   */

    /* Dialer */
    STATUS_DIGIT_DIALED,            /* None                             */

    /* LMON */
    STATUS_LMON_HANGUP_RESUME_PARSE,
    STATUS_LMON_HANGUP,
    STATUS_REMOTE_HANGUP,

	STATUS_RING_BURST,				/* UINT32 Polarity	*/
	STATUS_CALLPROGRESS_TONE,		/* UINT32 Polarity */
	STATUS_CALLPROGRESS_ALT_TONE,	/* UINT32 Polarity */

	STATUS_TONE_1100,
	STATUS_TONE_2130,
	STATUS_TONE_2312,
	STATUS_TONE_2750,
	STATUS_TONE_2912,
	STATUS_VOICETONEACALLPROG,
	STATUS_VOICETONEB_1100,
	STATUS_VOICETONEC_1300,
	STATUS_VOICETONEC_2100,
	STATUS_VOICETONEB_2225,
//	STATUS_V8BTONEC_2100,       // rmy - replaced by STATUS_ANSWER_TONE
//	STATUS_V8BTONEB_2225,       // rmy - replaced by STATUS_BELL_TONE
	STATUS_CI_CENTER_TONEB,
	STATUS_CI_SIDE_TONEC,
	STATUS_VOICEVIEW_TONEC_2912,
	STATUS_VOICEVIEW_TONEB_2312,
	STATUS_VOICEVIEW_TONEB_2750,
	STATUS_VOICEVIEW_TONEA_2130,
	STATUS_VOICEVIEW_TONEA_NOT_ST,
	STATUS_TONEC_1650,          // dec:46
	STATUS_CReSEG1_2002,
    STATUS_V8BIS_TONE,
    STATUS_V8BIS_NEW_FRAME,         /* BOOL - TRUE when Rx PIM got a new frame  */
    STATUS_V8BIS_BAD_FRAME,         /* BOOL - TRUE when Rx PIM got bad frame    */
    STATUS_V8BIS_COMPLETE,          /* V8BIS_RESULT*                            */
    STATUS_V8BIS_DETECTED,          /* Sent when remote V8bis is detected       */
    STATUS_V8BIS_MSG_SENT,          /* Sent by Tx PIM when message has been sent*/

	// BEGIN, DEVICE MANAGER SPECIFIC STATUSES
    STATUS_TX_EMPTY,
	STATUS_DOWNLOAD_REQUEST,
	STATUS_CARRIER_LOST,
    STATUS_CLEAR_TO_SEND,
	STATUS_RETRAIN_DETECTED,		/* UINT32* Polarity					*/
	STATUS_RENEG_DETECTED,			/* UINT32* Polarity					*/
	STATUS_CLEARDOWN_DETECTED,		/* UINT32* Polarity					*/

	STATUS_S_SEQ_DETECTED,			/* UINT32* Polarity,V.32,V.32b,V.34 */
	STATUS_S_NEGSEQ_DETECTED,		/* UINT32* Polarity,V.32,V.32b,V.34 */
	STATUS_R_SEQ_DETECTED,			/* UINT32* Polarity,V.32,V.32b		*/
	// END, DEVICE MANAGER SPECIFIC STATUSES
	HDLC_SYNC_DETECTED,             /* UINT32* Polarity					*/

    STATUS_RETRAIN_COMPLETE,        /* BOOL fSuccess                    */

	STATUS_SYNC_DETECTION,			/* UINT32* Polarity					*/

    /* V8 events */
	STATUS_V8_CI_DETECTED,			/* None                             */
	STATUS_V8_CM_DETECTED,			/* None                             */
	STATUS_V8_CJ_DETECTED,			/* None                             */
	STATUS_V8_JM_DETECTED,			/* None                             */
	STATUS_V8_NO_MODES_COMMON,		/* None                             */
	STATUS_V8_ANSam_DETECTED,		/* None                             */

    STATUS_ATDTE_RX_CHAR,           /* None                                  */
    STATUS_ATDTE_TX_CHAR,           /* BOOL - TRUE if TxQueue is empty       */
    STATUS_ATDTE_PORT_STATUS,       /* UINT32 - port status bits combination */
    STATUS_ATDTE_CTS,               /* BOOL */
    STATUS_ATDTE_DTR,               /* BOOL */
	STATUS_ATDTE_CD,				/* BOOL */
	STATUS_ATDTE_RX_OVRN,			/* None */

    STATUS_EC_ACTIVE,               /* UINT32 Polarity                  */
    STATUS_V42_OPENED,              /* UINT32 Polarity                  */

    STATUS_FAX_OK,
    STATUS_FAX_ERROR,
    STATUS_FAX_NO_CARRIER,
    STATUS_FAX_FAX,
    STATUS_FAX_FCERROR,
	STATUS_FAX_CONNECT,             /*  None                            */
    STATUS_FAX_V21_DET,
    STATUS_FAX_START,

    STATUS_REMOTE_FAX_DETECTED,     /* Originating fax detected in FAA  */
    STATUS_REMOTE_DATA_DETECTED,    /* Originating data detected in FAA */
    STATUS_CP_MODE_SELECTED,        /* FAA mode selection report to AT parser */

	STATUS_LAL_SETUP_COMPLETE,      //pq
	STATUS_LAL_TERMINATED,     //pq
    
    STATUS_OVP,

    STATUS_LAST,
	STATUS_INVALID = (unsigned)-1

} STATUS_CODE;

#endif
