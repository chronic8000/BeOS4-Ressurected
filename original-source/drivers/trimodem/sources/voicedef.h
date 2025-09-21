#ifndef VOICEDEF_H
#define VOICEDEF_H
#include "common.h"
#include "product.h"
// #include "target.h"

typedef struct ToneToGenerateTag {
    UINT8    type;
    UINT32  duration;
    union ToneTag {
        struct dualtag {
            UINT16 freq1;
            UINT16 freq2;
            }   dual;
        char    dtmf;
        }   tone;
    } ToneToGenerate;

#define v253_MAX_EVENT_BIT_INDEX                   34
#define v253_ENVENT_ARRAY_SIZE                      5

typedef struct VLSReportTag 
{
    UINT8     LableNum;   
    CHAR      *LabelStr;  /* e.g. "1" */
    CHAR      *Devices;   /* e.g. "L" */
    UINT8     TransmitEvent[v253_ENVENT_ARRAY_SIZE];
    UINT8     ReceiveEvent[v253_ENVENT_ARRAY_SIZE];
    UINT8     IdleEvent[v253_ENVENT_ARRAY_SIZE];
    UINT8     Modes;     /* Mask of the modes( command, receive, transmit, duplex, speakerphone) */    
}   VLSReportData, *PtrVLSReportData;                           /* accesible in that VLS setting */


typedef struct VSMReportTag 
{
    UINT8     LableNum;   /* Numeric possibly target dependent*/
    UINT8     TCICompId;  /* Numeric unique identifier defined by Trisignal */
    UINT8     BitsPerSample;
    UINT8     TimeEventsPeriod;
    CHAR      *PossibleValue;
    CHAR      *PossibleValueReport; 
}   VSMReportData, *PtrVSMReportData;

/* Compression unique identifier defined by Trisignal, TCICompId member of VSMReportData*/
#define UNSIGNED_LINEAR_PCM  		0   
#define ROCKWELL_ADPCM       		1
#define IMA_ADPCM            		2
#define MU_LAW_PCM			 		3
#define A_LAW_PCM			 	   	4
#define UNSIGNED_LINEAR_PCM_16BITS 	5
				   
/* Index for v253ParVSM[] array*/
#define v253_COMPRESSION_ID         0
#define v253_SAMPLING_RATE          1
#define v253_SILENCE_COMPRESSION    2
#define v253_SILENCE_EXPANSION      3

/* Index for v253ParVSD[] array*/
#define v253_SILENCE_SENSITIVITY    0
#define v253_SILENCE_PERIOD         1


#define TONE_BUFFER_LENTH 128
#define TONE_BUFFER  ((ToneToGenerate *) (VOICE_BUFFER + VOICE_BUFF_LENGTH))
#define GENERATE_SILENCE                (UINT8) 0
#define GENERATE_DTMF                   (UINT8) 1
#define GENERATE_DUALTONE               (UINT8) 2
#define GENERATE_FLASH                  (UINT8) 3

/* variable use exclusively for voice */
#ifdef V253_CMD_SET
extern VLSReportData v253CurrentVlsStruct;
extern VSMReportData v253CurrentVsmStruct;
#endif
extern BOOLEAN       v253TIA695Set;


extern BOOLEAN  dle_etx_found;      /* if DLE ETX found while filling 1st half of voice tx buffer */
extern BOOLEAN     dle_p_found;     /* if DLE p found while moving data from DTE to TM */
extern BOOLEAN     dle_r_found;     /* if DLE r found while moving data from DTE to TM */
extern BOOLEAN     dle_can_found;   /* if DLE CAN found while moving data from DTE to TM */
extern BOOLEAN     dle_ETX_found;      /* if DLE ETX found while moving data from DTE to TM */
extern BOOLEAN  v_ans_tone_found;   /* if answer tone found while inquiering ringback signal */
extern BOOLEAN  v_originate_call;   /* if the call was originated by local modem */
extern BOOLEAN  voice_detected;     /* when in VRX this variable become TRUE when detecting voice */
extern BOOLEAN  MuteOn;
extern BOOLEAN  RecSpeakerphoneOn;
extern BOOLEAN  CommanModeAndVconSent; /* In ON_LINE_VOICE_COMMAND_1 Indicate that command mode */
                                       /* has been set and Vnin send by the VOICE_RECEIVE_MODE_3 */

extern BOOLEAN  voice_receive;      /* indicate if we are receiving voice data, set by the pump */
extern BOOLEAN  voice_transmit;     /* indicate if we are transmitting voice data, set by the pump  */
extern BOOLEAN  voice_spkphone;     /* indicate if we in speaker phone set by the pump, set by the pump   */
extern UINT8    tone_gen_phase;     /* indicate if we are generating tone */
extern UINT8    previous_tone;      /* contain the value of the previous tone detected by the tone detector */
extern UINT8    v_prev_byte;        /* previous BYTE transmitted */
extern UINT8    v_byte_memory;
extern UINT8    voice_deadman_time; /* time without receiving data that the modem abort the communication */
extern UINT8    CurToneDetectCaps;  /* Current tone detection capabilities */
extern UINT8	saved_dtmf_status;

extern CHAR    *ptr_VTS_string;     /* pointer use to read the VTS command */
extern UINT32   tone_timer;         /* count the time when generating a tone */
extern UINT32   voice_timer;        /* indicate if we are detecting a tone */
extern UINT32   silence_timer;      /* use in silence detection in VRX command */
extern UINT32   silence_detect_timer;   /* use in detection of busy tones */
extern UINT32   energy_timer;
extern UINT32   deadman_timer;          /* use to determinate if the modem is dead state */
extern UINT32   receive_mark_timer;    	/* use in #VTM command */

#ifdef TAM_HANDSET_HOOK_CONTROL
extern UINT8    handset_hook_status;	/* status off the handset hook, 0  */
#endif

/* Parameter for each voice command */

/* AT command parameters common to both V.253 and Rockwell command types */
extern UINT8     par_CID;
extern UINT8     par_VRA;
extern UINT8    par_VGR;
extern UINT8    par_VGT;

#ifdef V253_CMD_SET
/* AT command parameters specific to V.253 */
extern UINT8    v253ParVDID;
extern UINT8    v253ParVDR[];
extern UINT8    v253ParVDT[];
extern UINT8    v253ParVDX;
extern UINT8    v253ParVEM;
extern UINT8    v253ParVGM;
extern UINT8    v253ParVGR;
extern UINT8    v253ParVGT;
extern UINT8    v253ParVGS;
extern UINT16   v253ParVIT;
extern UINT8    v253ParVLS;
extern UINT8    v253ParVNH;
extern UINT8    v253ParVRN;
extern UINT8    v253ParVRID;
extern UINT8    v253ParVSD[];
extern UINT16   v253ParVSM[];
extern UINT8    v253ParVSP;
extern UINT8    v253ParVTD;
#endif

/* AT command parameters specific to Rockwell command types */
extern UINT8    par_BDR;
extern UINT8    par_VBS;
extern UINT8    par_VBT;
extern UINT8    par_VLS;
extern UINT8    par_VRN;
extern UINT8    par_VSD;
extern UINT8    par_VSK;
extern UINT8    par_VSP;
extern UINT8    par_VSS;
extern UINT8    par_VTD[3];
extern UINT8    par_VTIM;
extern UINT16   par_VSR;
extern UINT32   par_RG;
extern UINT32   par_TL;
#ifdef SPEAKERPHONE
extern UINT8       par_SPK[3];
#endif


extern UINT8    *voice_rx_read;
extern UINT8    *voice_rx_write;
extern UINT8    *voice_rx_topp;
extern volatile UINT16   voice_rx_cntr;
extern UINT8    *voice_tx_read;
extern UINT8    *voice_tx_write;
extern UINT8    *voice_tx_topp;
extern volatile UINT16   voice_tx_cntr;



extern ToneToGenerate *tone_tx_read;
extern ToneToGenerate *tone_tx_write;
extern ToneToGenerate *tone_tx_top;
extern UINT16          tone_tx_cntr;     
#endif