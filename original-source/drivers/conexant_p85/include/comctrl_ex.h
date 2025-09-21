/****************************************************************************************
 *                     Version Control Information										*
 *                                                                                      *
 * $Header:   P:/d942/octopus/archives/Include/ComCtrl_Ex.h_v   1.6   13 Jun 2000 15:08:24   lisitse  $
 * 
*****************************************************************************************/

/****************************************************************************************

File Name:			ComCtrl_Ex.h	

File Description:	Com Controller object data structures

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

#ifndef __COMCTRL_EX_H__
#define __COMCTRL_EX_H__


/* Current ComController version */
#define	COMCTRL_MAJOR_VERSION	0
#define	COMCTRL_MINOR_VERSION	1

#define COMCTRL_VERSION (((COMCTRL_MAJOR_VERSION) << 16) | (COMCTRL_MINOR_VERSION))


/* Supported Configure codes        */
typedef enum 
{
    COMCTRL_CONFIG_DEVICE_ID,           /* [in] PVOID               - required before Init()*/
    COMCTRL_CONFIG_EVENT_HANDLER,       /* [in] PPORT_EVENT_HANDLER - optional              */

	COMCTRL_CONFIG_LAST /* Dummy */

}COMCTRL_CONFIG_CODE;

/* 
Supported Monitor codes          
*/
typedef enum 
{
    COMCTRL_MONITOR_STATUS,             /* [out] UINT32 - combination of status bits STS_XXX */
    COMCTRL_MONITOR_DEVICEID,           /* Device Node */
	COMCTRL_MONITOR_LAST /* Dummy */

}COMCTRL_MONITOR_CODE;

/* 
Supported Control codes
*/
typedef enum 
{
    COMCTRL_CONTROL_SETRTS,           /* Set RTS high                   - no extra data                     */
    COMCTRL_CONTROL_CLRRTS,           /* Set RTS low                    - no extra data                     */
    COMCTRL_CONTROL_SETDTR,           /* Set DTR high                   - no extra data                     */
    COMCTRL_CONTROL_CLRDTR,           /* Set DTR low                    - no extra data                     */
    
    COMCTRL_CONTROL_SET_XON,
    COMCTRL_CONTROL_SET_XOFF,
    COMCTRL_CONTROL_SEND_XON,
    COMCTRL_CONTROL_SEND_XOFF,
    
    COMCTRL_CONTROL_RESETDEV,         /* Reset device if possible       - no extra data                     */
    COMCTRL_CONTROL_CHAR,             /* Send immediate character.      - [IN] (char)pData - char to send   */

    COMCTRL_CONTROL_INIT_STATE,

    COMCTRL_CONTROL_PORTCONFIG,       /* Set Port Configuration         - [IN] (PPORT_CONFIG)pData          */
    
    COMCTRL_CONTROL_SLEEP,
    COMCTRL_CONTROL_WAKEUP,

	COMCTRL_CONTROL_LAST              /* Dummy */

}COMCTRL_CONTROL_CODE;


/*
Port Configuration bits - specify which fields of PORT_CONFIG structure are valid
*/
#define PC_DTE_SPEED        0x00000001
#define PC_PARITY           0x00000010
#define PC_DATA_BITS        0x00000020

#define PC_XINPUT           0x00000100
#define PC_XOUTPUT          0x00000200
#define PC_ERROR            0x00000400
#define PC_SKIPNULL         0x00000800
#define PC_CTS				0x00001000
#define PC_RTS				0x00002000

#define PC_XON_CHAR         0x00010000
#define PC_XOFF_CHAR        0x00020000
#define PC_ERROR_CHAR       0x00040000
#define PC_PEALT_CHAR     	0x00080000

#define PC_EVERYTHING       0xFFFFFFFF

/*
Port Configuration structure (used in COMCTRL_CONTROL_PORTCONFIG control)
TODO(?): structure may be compacted (replace BOOLs by bitfields)
*/
typedef struct tagPORT_CONFIG
{
    UINT32          dwValidFileds;  // Combination of PortConfiguration bits

    UINT32          dwDteSpeed;

    enum
    {
        PC_PARITY_NONE,
        PC_PARITY_ODD,
        PC_PARITY_EVEN,
        PC_PARITY_MARK,
        PC_PARITY_SPACE
    }               eParity;        // Parity check

    enum
    {
        PC_DATABITS_7,
        PC_DATABITS_8
    }               eDataBits;      // Data bits - only 7 and 8 are enabled

    BOOL            fXOutput;       // Xoff/Xon enabled for output
    BOOL            fXInput;        // Xoff/Xon enabled for input
    BOOL            fError;         // Replace chars with parity error by ErrorChar
    BOOL            fNull;          // Skip NULL characters
    BOOL            fCTS;           // Use CTS for Tx flow control
    BOOL            fRTS;           // Use RTS for Rx flow control

    char            cXonChar;
    char            cXoffChar;
    char            cErrorChar;
    char            cPEAltChar;

}   PORT_CONFIG, *PPORT_CONFIG;


/*
Port Event Handler - used in Configure(COMCTRL_CONFIG_EVENT_HANDLER)
*/
typedef struct tagPORT_EVENT_HANDLER
{
    void    (*pfnCallback) (PVOID pRef, UINT32 dwEventMask);
    PVOID   pRef;

} PORT_EVENT_HANDLER, *PPORT_EVENT_HANDLER;


/********************************************************************
// Bitmaks definition
********************************************************************/ 

/*
// Modem status bits
*/ 
#define	COMCTRL_STS_CTS_HOLD    0x00000001	/* Transmit is on CTS hold                  */
#define	COMCTRL_STS_DSR_HOLD    0x00000002	/* Transmit is on DSR hold                  */
#define	COMCTRL_STS_RLSD_HOLD   0x00000004	/* Transmit is on RLSD hold                 */
#define	COMCTRL_STS_XOFF_HOLD   0x00000008	/* Received handshake                       */
#define	COMCTRL_STS_XOFF_SENT   0x00000010	/* Issued handshake                         */
#define	COMCTRL_STS_EOF         0x00000020	/* EOF character found                      */
#define	COMCTRL_STS_TXIMMED     0x00000040	/* Immediate character being transmitted    */


/*
// Event bits
*/
#define	COMCTRL_EVT_RXCHAR      0x00000001	/* Any Character received                  */
#define	COMCTRL_EVT_RXFLAG      0x00000002	/* Received certain character              */
#define	COMCTRL_EVT_TXCHAR      0x00000004	/* Any character transmitted               */
#define	COMCTRL_EVT_TXEMPTY     0x00000008  /* Transmit Queue Empty                    */
#define	COMCTRL_EVT_CTS         0x00000010  /* CTS changed state                       */
#define	COMCTRL_EVT_CTSS        0x00000020  /* CTS state                               */
#define	COMCTRL_EVT_DSR         0x00000040	/* DSR changed state                       */
#define	COMCTRL_EVT_DSRS        0x00000080	/* DSR state                               */
#define	COMCTRL_EVT_RLSD        0x00000100	/* RLSD changed state                      */
#define	COMCTRL_EVT_RLSDS       0x00000200	/* RLSD state                              */
#define	COMCTRL_EVT_BREAK       0x00000400	/* BREAK received                          */
#define	COMCTRL_EVT_ERR         0x00000800	/* Line status error occurred              */
#define	COMCTRL_EVT_RING        0x00001000	/* Ring signal detected                    */
#define	COMCTRL_EVT_PARITY_ERR  0x00002000	/* Parity Error occured                    */
#define	COMCTRL_EVT_RXOVRN	    0x00004000	/* DTE Rx buffer overrun detected          */



/*
Interface definition
*/
PVOID ComCtrlGetInterface(void);

typedef struct I_COM_CTRL_TAG
{
	/************ Common Interface functions. ************/

	UINT32		(*GetInterfaceVersion)  (void);
	HANDLE		(*Create)               (void);
	COM_STATUS	(*Destroy)              (HANDLE hInst);
	COM_STATUS	(*Init)                 (HANDLE hInst);
	COM_STATUS	(*Close)                (HANDLE hInst);
	COM_STATUS	(*Configure)            (HANDLE hInst, COMCTRL_CONFIG_CODE  eCode,  PVOID pConfig);
	COM_STATUS	(*Monitor)              (HANDLE hInst, COMCTRL_MONITOR_CODE eCode, PVOID pMonitor);
	COM_STATUS	(*Control)              (HANDLE hInst, COMCTRL_CONTROL_CODE eCode, PVOID pControl);

	/************ Specific Interface functions. ************/

    UINT32      (*Read)   (HANDLE hInst, PVOID pBuf, UINT32 dwSize);
    UINT32      (*Write)  (HANDLE hInst, PVOID pBuf, UINT32 dwSize);

} I_COM_CTRL_MGR_T, *PI_COM_CTRL_T;



#endif      /* #ifndef __MODEM_H__  */
