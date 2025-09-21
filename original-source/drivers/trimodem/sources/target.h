#ifndef TARGET_H
#define TARGET_H

#include "product.h"
#include "common.h"
#ifdef TAM
#include "voicedef.h"
#endif        

//
// Forward definition for targetopen
//
class TsHalAPI;

#define DP_PADDING	4
#define DP_START_OFFSET 0

#define INTERRUPT_LED_CONTROL   SWITCH_OFF  /* ON for led_control() to
                                             * be called every 10 ms via
                                             * int instead of every pass
                                             * of the main loop.
                                             */


// DP servicing can be done by HW IRQ or by polling.
#define  IRQ_POLLING             

// BeOS is using IRQ polling
#ifdef __BEOS__
#ifndef IRQ_POLLING
#define IRQ_POLLING
#endif
#endif

// The RING DETECT will be done by polling. 
#define RING_POLLING

// Time between two calls of MainPollingFunc in msec
#define POLLING_TIMEOUT          10


/********************************************
 * DCE module define statement
 */
#define DP_DOES_V14     SWITCH_OFF
#define DP_DOES_HDLC    SWITCH_ON   //bp: for processor usage
#define DP_SUPPORTS_K56
#define DP_SUPPORTS_V90
#define DP_SUPPORTS_V90_FAST_RETRAIN

#define DP_ROCK_PLL_NOT_USED  /* Most phantoms don't have the pump PLL enabled */

// now defined in makefile    NOT TRUE IN WINNT!!
#ifdef _WIN32_WINNT
#define DP_ROCKDL_L8774
#endif  //_WIN32_WINNT

#define DP_LEVEL_ADJUSTMENT  4
//#define DP_SUPPORTS_V90
//#define DP_DIGITAL_SPEAKER

#ifndef INTERNATIONAL_CALLER_ID
/* For now sleep mode interferes with detecting the dual tone alert signal so we are disabling.
   A solution should be found to this problem (probably wakeing the pump when a line polarity reversal is detected) */
#undef  DP_DOES_SLEEP_MODE
#endif

/* end of DCE module */


/********************************************
 * TAM module define statement
 */
#define v253_VLS_POSSIBLE_VALUE  pstrVLSPossValue;
#define v253_VLS_SPEAKERPHONE    "d(0,1,2,4,5,6,7,8,9,11,13)" 
#define v253_VLS_ARRAY_SIZE_SPH    11
#define v253_VLS_ARRAY_SIZE_NO_SPH  2
#define	v253_VLS_NO_SPEAKERPHONE "d(0,1)"
#define v253_VTS_POSSIBLE_VALUE  "(200-3000),(200-3000),(0-400)" /* Must corespond to max min frequency and duration below */
#define v253_VSM_DEFAULT         128
#define v253_VSM_BPS_DEFAULT     7200
#define TAM_VTS_MAX_FREQUENCY    3000
#define TAM_VTS_MIN_FREQUENCY    200
#define v253_TONE_DURATION_MAX   ((UINT32) 400)        /* max duration of a tone in 1/100 th of a second */
#define rck_COMPRESSION          1 /* ROCKWELL_ADPCM, Rockwell cmd support only one compression */
#define v253_VSP_POSSIBLE_VALUE pstrVSPPossibleValues
#define TAM_VBS_POSSIBLE_VALUE    "d8"
#define TAM_VBS_DEFAULT            8
#define	TAM_HANDSET_HOOK_CONTROL
/* end of TAM module */



/********************************************
 * DTE module define statement
 */
#define DCHIP_PCMCIA    SWITCH_OFF

#define MESS_LNT        2000     /* Message buffer length */

#define VXD_IN_USE      SWITCH_ON
#define VXD_ONLY        SWITCH_ON

/* end of DTE module */


/* ############################################*\
 *       TARGET module section                 *
\* ############################################*/

/* if the line current sensing senses current (a line) it has to return
 * not 0. */
#define LCS_TEST        0 /* not available */       /* BOOLEAN */

/* Define according to the signal level received from the line PB9 */
#define RING_TEST       /* not available on this platform */

#define HOOK_PIN_off    /* not available on this platform */
#define HOOK_PIN_on     /* not available on this platform */

#define PULSE_PIN_off   /* not available on this platform */
#define PULSE_PIN_on    /* not available on this platform */

/* result is not 0 if already muted */
#define MUTE_test       /* not used (BOOLEAN)*/
#define MUTE_on         /* not used */
#define MUTE_off        /* not used */

#define SET_FOR_RJ11    /* not used */
#define SET_FOR_RJ13    /* not used */


/****************\
 * V.24 signals *
\****************/

#define TST_PIN_low     /* not available on this platform */
#define TST_PIN_high    /* not available on this platform */


/* ANALOG LOOP test. result is 0 if active */
#define V24_ANL_TEST    1       /*(PDDAT &   BIT_2)*/
/* RDL LOOP test. result is 0 if active */
#define V24_RDL_TEST    1       /*(PDDAT &   BIT_1)*/


/********************************************\
 * help to detect end of DTE and DCE breaks *
\********************************************/

#define BRK_STILL_ON            0x40
#define BRK_LEN_NOT_AVAILABLE   0xffff

#define DTE_RX_STATE    (LCR & BIT_6)
#define DCE_RX_STATE    (BRK_LEN_NOT_AVAILABLE)

/****************************************************\
 * definition used with the ENCRYPTION ZORRO option *
\****************************************************/

#define EMULATOR_OFFSET     0   /* if not emulator */
#define RAM_64K             1
#define RAM_128K            0
#define RAM_256K            0

/* if the RAM_OAG_SIZE change ERAM_SIZE and STACK must be changed in target.equ
 * and in the link files at the instruction RESMEM
 */
#if RAM_256K
#define     RAM_OAG_SIZE                8192
#define     RAM_OAG_BASE_ADDR           (BYTE *)(0x40000 - RAM_OAG_SIZE + EMULATOR_OFFSET)
#endif

#if RAM_128K
#define     RAM_OAG_SIZE                8192
#define     RAM_OAG_BASE_ADDR           (BYTE *)(0x20000 - RAM_OAG_SIZE + EMULATOR_OFFSET)
#endif

#if RAM_64K
#define     RAM_OAG_SIZE                512
#define     RAM_OAG_BASE_ADDR           (BYTE *)(0x10000 - RAM_OAG_SIZE + EMULATOR_OFFSET)
#endif

#define     EE_OAG_SIZE                 00

// Compiler Word alignment
#define __LITTLE_ENDIAN__  

/* functions definitions */

BOOLEAN li_get_lcs(void);
BOOLEAN li_get_ring(void);
void target_set_ring_periods(BYTE minRingPeriod,BYTE maxRingPeriod);
void li_get_ring_int(void);
void li_set_hook(BYTE);
void li_set_mute(BOOLEAN state);
void li_set_pulse(BYTE);
void li_set_rj11(BOOLEAN state);
void target_control(BYTE);
void target_dip_switch(BYTE *);
void target_dp_reset(BOOLEAN);
BYTE target_get_ring(void);
BOOLEAN target_get_TD(void);
void target_local_digital_loop(BOOLEAN);
void target_set_TCI_timer(UINT16);
void target_stop_TCI_timer(void);
void target_set_volume(BYTE);
void break_check(void);

void targetSetCallerId(BOOLEAN);
BOOLEAN targetDetectLinePolarityReversal(void);
void targetResetLPRDetector(void);
void targetLoadCIDImpedances(BOOLEAN);
void targetApplyCurrentWettingPulse(void);

#if VOICE
BOOLEAN target_set_analog(BYTE);
void    target_decrement_playback_volume(void);
void    target_increment_playback_volume(void);
#endif 

CHAR* target_id(void);
CHAR *target_send_dp_version(void);

void data_pump_error(UINT32); 
BOOLEAN targetTestHardware(void);
void target_test_pin(BYTE,BYTE);
void target_message(char *);


#ifdef TAM
BOOLEAN target_set_analog(BYTE device);
void    target_decrement_playback_volume(void);
void    target_increment_playback_volume(void);
#ifdef V253_CMD_SET
PtrVLSReportData target_get_V253_VLS_array(UINT16, UINT16 *);
PtrVSMReportData target_get_V253_VSM_array(UINT16 *);
#endif
#ifdef SPEAKERPHONE
void target_set_speakerphone_mute(BYTE);
void target_set_speakerphone_spk(BYTE);
void target_set_speakerphone_mic(BYTE);
#endif
#ifdef TAM_HANDSET_HOOK_CONTROL
UINT8 	target_get_handset_hook_status(void);
void target_connect_handset_to_dce(BOOLEAN gpout);
#endif
#endif 



DWORD targetGetComMemInterfaceAddr(void);
void targetSetComMemInterfaceAddr(DWORD);

void targetForceOffHook (BOOLEAN );

//WORD targetGetBaseIO(void);    // Base IO of the comm port 
//BYTE targetGetIrq(void);       // Irq to virtualize
void targetClose(void);


#ifdef _WIN32_WINNT

#ifndef GENERIC_MODEM_CORE
// Device names:
extern PWSTR pwstrControllerDeviceName; // for constructor of Controller device class
#endif
extern "C" {
BOOLEAN targetIsDpIrqInstalled(void);
}
#else
BOOLEAN targetIsDpIrqInstalled(void);
#endif  //_WIN32_WINNT

void target_set_country(BYTE);
void target_set_earth_relay(BYTE);
BOOLEAN target_check_country(BYTE);
void target_InstallDpIrq(void (*pCallback)(void));
void target_dpNeedsHookControl(BYTE);
UINT32 target_HwFault(void);

#ifdef EXTERNAL_COUNTRY_DATABASE 
void targetGetCountryInfo(BYTE,COUNTRYCODE **,S_LIMITS **); 
#endif

// value for interrupt request for ISDN and DP
//BYTE target_DP_int(void);    
//BYTE target_ISDN_int(void);  


// message string for RIN3ERR.CPP
extern const CHAR *pstrProductName;


// jfm
void targetDeleteCurrentSregistersFile(void);
void targetDeleteCurrentFaxFrameFile(void);
BOOL targetEnterInterrupt(void);
BOOLEAN targetIsInterruptEnabled( void);
void targetExecuteInterrupt(void);
void targetLeaveInterrupt(void);
void targetOpenProlog(void);
void targetOpenEpilog(void);
void targetDisableInterrupt(void);

#ifdef TAM
extern const CHAR *pstrVLSPossValue;
extern const CHAR *pstrVSPPossibleValues;
#endif

BOOL targetOpen(TsHalAPI* HALPtr);
void targetClose(void);
UINT32 targetGetHwFaultReason(void);

UINT16 target_test_tail(void);


class targetDataPumpServices
{
public:
    // base address control
    UINT32 GetBaseAddress (); 
    // interrupt control
/*    BOOLEAN InstallInterrupt 
            (BOOL (*EnterISRCallbackFnct)(void*),
             void (*ExecuteISRCallbackFnct)(void*));
    void UninstallInterrupt ();
*/    // volume control
    void SetVolume (UINT8 State); 
    // device control
    void SetPower (BOOL PowerOn); 
    void Reset (BOOL State); 
};

extern targetDataPumpServices targetDataPumpHal;



#endif  /* TARGET_H */
