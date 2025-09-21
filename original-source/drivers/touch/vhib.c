/**************** Copyright (c) 1999 National Semiconductor *****************
*
* File:         VHIB.c
*
* Summary:      Virtual Host Interface Bus
*
* Contents:     Contains functions used to read/write the WebPad HIB
*
* Terminology:	PMC - Pad Management Controller (Atmel 4434)
*				HIB - Host Interface Bus - Nibble bus interface between the
*						Cx5530 and the PMC.
*
*				EC  - National COP8 - Embedded Controller for Draco
*
*  For evaluation purposes only - not for production usage
*
****************************************************************************/
#include <OS.h>
#include <KernelExport.h>


#include	"pmutils.h"		// pm_config_set
#if 0
#include	<smisys.h>		// write_Cx5530_8
#include	<grappa.h>		// Cx5530 equates
#include	<defines.h>		// IRQ defines
#include	<pmutils.h>
#include	<vuart.h>		// include virtual UART defines
#include	<pm.h>			// pm_info structure definition
#include	<pmapm.h>
#endif
#include 	"pmccmds.h"		// PMC command set
#include "gpio.h"

typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;

//extern	int	penIndexUART;				// index into storagePen

extern	void updateF3BAR(unsigned int, unsigned char);
extern	unsigned short apm_report_event (unsigned short);
extern	unsigned short audio_fifo_active(void);

//extern	ApmInfo apm_info;


//
// Externs which are in VUART.C
//
extern	int		checkIfUARTIntsAreEnabled(unsigned int);
extern	void	updateUARTInterruptIdentification(unsigned int, unsigned int);

//
// The following equates define GPIO pin usage
//
#define	UC5530HSI 		0x01		// GPIO 0 - Output - Always
#define	UCSMI			0x02		// GPIO 1 - Input - Generates SMI
#define UCRESET			0x04		// GPIO 2 - Reset the PMC when high
#define	AUDAMPSHDN 		0x08		// GPIO 3 - Output - Always
#define	UC5530D0		0x10		// GPIO 4 - Bi-directional
#define	UC5530D1		0x20		// GPIO 5 - Bi-directional
#define	UC5530D2 		0x40		// GPIO 6 - Bi-directional
#define	UC5530D3 		0x80		// GPIO 7 - Bi-directional

//
// edge detection equates
//
#define	CLEAR_UC5530HSI		0x00
#define	SET_UC5530HSI  		0x01

#define	WAIT_UCSMI_GOING_LOW	0
#define	WAIT_UCSMI_GOING_HIGH	UCSMI

//
// Delay routine timeout values
//
#define DELAY_TIME_UCSMI		500
#define DELAY_TIME_UC5530D0		200

//
// Boolean equates
//
#define	TRUE	1
#define	FALSE	0

//
// IRQ assignments and notify types.
// NOTE: If the IRQ assignment changes, then TSC_UART and MISC_UART MUST be updated also
//
#define	TSC_IRQ				0x10		// IRQ 4 (COM1) is used for touch-screen packets only
#define	MISC_IRQ			0x08		// IRQ 3 (COM2) is used for all other info from PMC

//
// These equates are used by vuart.c to determine
//  which UART (COM1/COM2) is being addressed (equates are in vuart.h)
//
#define	TSC_UART			COM1		// These value are actually indexes into a 16 byte
#define	MISC_UART			COM2		//  array.  This is somewhat hardcoded.....

//
// These equates are used by notifyOS() to
//	determine how to notify OS.
//
#define	NOTIFY_VIA_TSC_IRQ					0x01 	// Interrupt to OS is because of Touch-Screen
#define	NOTIFY_VIA_BUTTON_IRQ				0x02 	// Interrupt to OS is because of button data

// Not used on Draco
//#define	NOTIFY_VIA_MISC_IRQ				0x03 	// Interrupt to OS is because of MISC data

//
// APM Notification types all start with 'A' (for APM)
//
#define	NOTIFY_VIA_APM_GET_POWER_STATUS		0xAA	// APM - Get Power Status Notification

//
// APM OEM Notification types all start with '8' (8x per APM Spec)

// Not used on Draco...
// #define NOTIFY_VIA_APM_OEM_GET_REVISION		0x80

//
// This notification type can be used if, for some reason,
//  the PMC has data but the O/S doesn't want to know about it
//
#define NOTIFY_VIA_DONT						0xFF 	// default case  - don't do anything


// Contrast & Brightness Equates for Draco

//#define MAX_CONTRAST 	0xFF		// HW version 1.3
//#define MIN_CONTRAST 	0x00
//#define C_DELTA 		0x01
//#define DEF_CONTRAST	0xFF
//#define DEF_BRIGHT	0xFF

// For HW version 1.4 and later use:

#define MAX_CONTRAST 	0xD0		//	was 0xC8
//#define MIN_CONTRAST 	0xA0		//  was 0xB0
#define C_DELTA 		0x04
//#define DEF_CONTRAST	0xB8
//#define DEF_BRIGHT		0x00
#define MIN_CONTRAST  0x70
#define DEF_CONTRAST	0x88
#define DEF_BRIGHT	0x90



//
// Data storage area
//
int		storageButton;				// last known state of the buttons [up/down]
int		storagePen[5];				// last known X/Y coordinate information [location 0 is not used]
int		storageTemp[2];				// 2 bytes of temp info

unsigned char storageBATT[2];		// 2 bytes battery status
//      	  storageBATT[0] = status (full, low, critical, etc)
//      	  storageBATT[1] = gas guage 0..100%

int		storagePMCRevision[3];		// Contains the major and 2 minor digits of the PMC revision
int		storageSysStatus;			// 1 byte System Status
int		storagePWMStatus[2];		// 2 bytes PWM data
int		storageDCStatus;			// 1 byte AC adapter status
int		storageTime[4];				// 4 bytes system timer tick
int		storageECUART;				// 1 byte EC UART data
int		storageSID[8];				// 8 bytes of Serial ID


int		penIndex=0;					// index into storagePen
int		penDataReady;				// 1 = data packet is ready for UART to send to O/S
int		notifyType;					// see notify types listed above
int		firstTime=TRUE;				// if firstTime==TRUE, we need to configure 5530 for IRQ generation
int		weAreNested=FALSE;			// Semaphore for making we don't re-enter ourselves because
									// of an audio SMI.

int		postComplete=FALSE;			// POST Complete indicator

unsigned char HIB_Buffer[16];		// Buffer for transfers to/from COP8
unsigned char *HIB_Ptr;				// ptr to above


unsigned char UserButton;			// User button press during POST

//
// Function proto-types
//

void	dispatchHIBProcess(unsigned char);
int		sendCommandToPMC(unsigned char);
int		handleCommandFromPMC();
int		receiveCommandFromPMC(int *);
void	cleanUpAfterOurSelves(void);
void	parseCommandFromPMC(unsigned int);
int		negotiateForBus(void);
void	getOnTheBus(void);
void	getOffTheBus(void);
int		notifyOS(int);
void	updateUART_DataPending(unsigned int, unsigned int);
void	writeNibbleOnHIB(unsigned char);
void	generateInterruptToPMC(char);
int		waitForUCSMI(char);
void	configureHIBInterface(void);
void	enterPreCommandState(void);
void    Delay10uSec(void);
void	disableGPIOFromGeneratingAnSMI(void);
void	enableGPIOToGenerateAnSMI(void);
void	irq_set(int, int);
int		WEBPAD_Hook(unsigned int, unsigned int);
void 	showDebug(int);

// Draco...

int		sendPacketToEC(unsigned char *);
void	getSYSStat(void);
void	getECver(void);
void 	getPWM(void);
void 	getBATT(void);
void 	InitPWM(void);
void 	getSID(void);
int 	IncBrightness(void);
int 	DecBrightness(void);
int 	IncContrast(void);
int 	DecContrast(void);
int		SetPWM(unsigned char, unsigned char);

unsigned int HIB_SendByte(int data);
unsigned char readEC(void);

void	delay5us(void);
void	getPacketFromEC(void);

int 	waitSMIpulse(void);

void 	PowerDown(void);
void 	TempInit(void);
void 	TempWarm(void);
void 	TempHot(void);
void 	TempOK(void);
void 	Delay1Mil(void);

// ...Draco

/****************************************************************
* dispatchHIBProcess(unsigned char packetRequest) - Entry point for handling
*	all requests to talk on the HIB and to retrieve data read.
*
* Input  - packetRequest - Indicates which packet is being requested.
*
****************************************************************/
void dispatchHIBProcess(unsigned char packetRequest)
{

// If semaphore set, we got here because SMIENTRY.ASM saw activity on
// our GPIO line and thought it should call us again. This can happen
// if we get an audio SMI while in this code.

	if (weAreNested == TRUE)
		return;

// If semaphore clear we got here due to APM activity.
// Set semaphore and enable nesting.


	disableGPIOFromGeneratingAnSMI();	// Disable SMI generation on GPIO
	weAreNested = TRUE;
	enable_nesting();					// otherwise we have audio dropouts

	switch (packetRequest) {
		case HOST_TO_PMC_NOP:

// Draco Commands...

		case HOST_TO_PMC_RQST:
		case HOST_TO_PMC_SET_HIB:
		case HOST_TO_PMC_SET_SYSMODE:
		case HOST_TO_PMC_SET_BUTTONS:
		case HOST_TO_PMC_SET_TEMPS:
		case HOST_TO_PMC_SET_PWMS:
		case HOST_TO_PMC_UART_WRITE:

			sendCommandToPMC(packetRequest);
			break;

		default:
			if ((packetRequest & 0x80) == HOST_TO_PMC_SYSTEM_CONTROL)
				sendCommandToPMC(packetRequest);
			break;
	}

// We're done, clean up...

	disable_nesting();
	weAreNested = FALSE;

}

/****************************************************************
* sendCommandToPMC(char commandRequest) - Initiates a request to the
*		PMC after successfully negotiating for the HIB.
*
*	Input - commandRequest - PMC command in nibble format
*
*	Output - Returns TRUE (non-zero) if request was sent to PMC.
*			 Returns FALSE if request could not be sent.
*
****************************************************************/
int	sendCommandToPMC(unsigned char commandRequest)
{
unsigned int status;

	enterPreCommandState();

    while (negotiateForBus() == FALSE)
		Delay10uSec();

	status = HIB_SendByte(commandRequest);

    cleanUpAfterOurSelves();
    return (status);
}

///////////////////////////////////////////////////////////////////////
//
// unsigned int HIB_SendByte(int data) - send a byte to EC across HIB
//
///////////////////////////////////////////////////////////////////////

unsigned int HIB_SendByte(int data)
{
unsigned int status;


    writeNibbleOnHIB ((unsigned char)(data & 0xF0));
    delay5us();
    generateInterruptToPMC(SET_UC5530HSI);

    status = waitForUCSMI(WAIT_UCSMI_GOING_HIGH);
    if (status == FALSE)
    	goto get_out;

    writeNibbleOnHIB((unsigned char)((data & 0x0F)<<4));
    delay5us();

get_out:

    generateInterruptToPMC(CLEAR_UC5530HSI);
    status = waitForUCSMI(WAIT_UCSMI_GOING_LOW);
if(status == FALSE)
	dprintf("HIB_SendByte( 0x%x )\n", data);
	return(status);
}

#if 0
/****************************************************************
* handleCommandFromPMC() - Handles reading in a byte from the PMC.
*
*    Input - None
*
*    Output - Returns TRUE (non-zero) if request was received from PMC.
*	      Returns FALSE if no command was received.
*
*    Note - This routine is called from two places:
*
*    1) dispatchHIBProcess() - calls receiveCommandFromPMC() after it has
*       sent a request to the PMC and is waiting for a response.
*
*    2) SMI dispatch table - The PMC can initiate sending data which
*       it does by generating an SMI.  The SMI dispatch handler
*       calls receiveCommandFromPMC() to handle the request.
*
****************************************************************/
int	handleCommandFromPMC()
{
  unsigned int data, status;

    if (weAreNested == TRUE)
    	return (FALSE);

    disableGPIOFromGeneratingAnSMI();
    weAreNested = TRUE;
	enable_nesting();

    status = waitForUCSMI(WAIT_UCSMI_GOING_LOW);

    if (status == FALSE)
        return(status);

    getOffTheBus();					// D3..D0 become inputs
    notifyType = NOTIFY_VIA_DONT;	// default if something goes wrong
    status = FALSE;					// default status

    // 1st byte is command and data length

    if (receiveCommandFromPMC(&data)) {
		parseCommandFromPMC(data);		// decode command
		notifyOS(notifyType);			// Tell OS what's going on
		status = TRUE;
    }

    // get nibble bus ready for next x-action

    cleanUpAfterOurSelves();

    // Disable nesting now that we're done.

	disable_nesting();
	weAreNested = FALSE;

    return (status);

}
#endif

/****************************************************************
* cleanUpAfterOurSelves() - handles getting the nibble bus back into
*		the correct state.  Failure to do this may result in not
*		receiving any more commands from the PMC.
*
*	Input - None
*
*	Output - None
*
****************************************************************/
void	cleanUpAfterOurSelves()
{
	enterPreCommandState();			// get HIB ready for next transaction
	enableGPIOToGenerateAnSMI();	// Enable SMI generation on GPIO
}

/****************************************************************
* parseCommandFromPMC() - Once the 1st byte is read from the PMC,
*		this routine handles reading the rest of the data and
*		updating the global data with the new info.
*
*	Input - data - the 1st byte read from the PMC.  The upper nibble
*					contains the COMMAND.  The lower nibble the CMD length
*
*	Output - varies with command
*
****************************************************************/

#if 0
void parseCommandFromPMC(unsigned int data)
{
int i;
unsigned char temp;

	switch (data & 0xF0) {

		case PMC_TO_HOST_NOP:

			break;

		case PMC_TO_HOST_SYS_STAT:

			if (!(receiveCommandFromPMC(&data)))
				goto PMC_TIMEOUT_ERROR;
			storageSysStatus = data;
		 	break;

		case PMC_TO_HOST_BUTT_STAT:

			if (!(receiveCommandFromPMC(&data)))
				goto PMC_TIMEOUT_ERROR;
			storageButton = data;
			notifyType = NOTIFY_VIA_BUTTON_IRQ;

            // Check for User Button 1 during POST

			if ((storageButton & 0x10) == 0x10)
				UserButton = TRUE;

			// Check for power button

			if (((storageButton & 0x40) == 0x40) && (postComplete == TRUE)) {
				disable_nesting();
            	if (apm_report_event(APM_EVENT_USER_SYSTEM_SUSPEND_REQUEST_NOTIFICATION) == 0xFFFF)
					PowerDown();
				enable_nesting();					
			}
            break;

		case PMC_TO_HOST_PEN_STAT:

 			for (penIndex = 1; penIndex <= 4; penIndex++) {

				if (!(receiveCommandFromPMC(&data))) {
			    	penDataReady = FALSE;		// Invalidate buffer contents
			    	goto PMC_TIMEOUT_ERROR;		//  and get out
				}

                storagePen[penIndex] = data;	// contents are don't-care, so over right
			}
			penDataReady = TRUE;				// signal VUART.C that data is available
			notifyType = NOTIFY_VIA_TSC_IRQ;	// signal NotifyOS that data is available
			break;


		case PMC_TO_HOST_TEMP_STAT:

			for (i = 0; i < 2; i++) {
				if (!(receiveCommandFromPMC(&data)))
					goto PMC_TIMEOUT_ERROR;
				storageTemp[i] = data;
			}

			temp = storageTemp[0] + storageTemp[1];

			if (temp == 3)			// 1 warm + 1 normal
				TempWarm();
			else if (temp >= 4)		// 1 hot + 1 normal
				TempHot();
			else
				TempOK();

			break;

		case PMC_TO_HOST_PWM_STAT:

			for (i = 0; i < 2; i++) {
				if (!(receiveCommandFromPMC(&data)))
					goto PMC_TIMEOUT_ERROR;
				storagePWMStatus[i] = data;
			}
			break;

		case PMC_TO_HOST_BATT_STAT:

        	for(i = 0; i < 2; i++) {
				if (!(receiveCommandFromPMC(&data)))
					goto PMC_TIMEOUT_ERROR;
				storageBATT[i] = (unsigned char) data;
			}

            notifyType = NOTIFY_VIA_APM_GET_POWER_STATUS;
			break;

		case PMC_TO_HOST_DC_STAT:

			if (!(receiveCommandFromPMC(&data)))
				goto PMC_TIMEOUT_ERROR;

			storageDCStatus = data;
			notifyType = NOTIFY_VIA_APM_GET_POWER_STATUS;
			break;

		case PMC_TO_HOST_VERSION:

			for (i = 0; i < 3; i++) {
				if (!(receiveCommandFromPMC(&data)))
                	goto PMC_TIMEOUT_ERROR;
				storagePMCRevision[i] = data;
			}
			break;

		case PMC_TO_HOST_TIME_STAT:

			for (i = 0; i < 5; i++) {
				if (!(receiveCommandFromPMC(&data)))
					goto PMC_TIMEOUT_ERROR;
				storageTime[i] = data;
			}
			break;

		case PMC_TO_HOST_UART_READ:

			if (!(receiveCommandFromPMC(&data)))
				goto PMC_TIMEOUT_ERROR;
			storageECUART = data;
			break;

		case PMC_TO_HOST_SERIAL_ID:

			for (i = 0; i < 8; i++) {
				if (!(receiveCommandFromPMC(&data)))
					goto PMC_TIMEOUT_ERROR;
				storageSID[i] = data;
			}
			break;

		default:
			break;

	} // end of switch

//..fall through

PMC_TIMEOUT_ERROR:

	enterPreCommandState();
	return;
}

/****************************************************************
* receiveCommandFromPMC(int byteRead) - Initiates a request
*		to the PMC after successfully negotiating for the HIB.
*
*	Input - byteRead - pointer to where to store the data
*
*	Output - Returns TRUE (non-zero) if request was sent to PMC.
*			 Returns FALSE if request could not be sent.
*
****************************************************************/
int	receiveCommandFromPMC(int *byteRead)
{
  unsigned int temp, status;

	status = FALSE;		// default status

// Wait for UC$SMI HIGH to indicate 1st nibble is on the bus

	if (!waitForUCSMI(WAIT_UCSMI_GOING_HIGH))
		goto PMC_TIMEOUT_ERROR;

// Read 1st nibble from HIB (Command)

	temp = PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0;

// Indicate we read the 1st nibble

	generateInterruptToPMC(SET_UC5530HSI);

// Wait for UC$SMI to fall indicating 2nd nibble is on the bus

	if (!waitForUCSMI(WAIT_UCSMI_GOING_LOW))
		goto PMC_TIMEOUT_ERROR;

// Read 2nd nibble from HIB (Length)

	temp |= (PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0) >> 4;

// Indicate we read the 2nd nibble

	generateInterruptToPMC(CLEAR_UC5530HSI);

	*byteRead = temp;		// stuff byte read into passed pointer
	status = TRUE;

PMC_TIMEOUT_ERROR:

	return (status);
}

#endif

/****************************************************************
* negotiateForBus() - Attempts to gain control of the HIB by placing
*		a request for control on the bus and waiting for the PMC
*		to grant request.
*
* 	Input - None
*
* 	Output -  Returns TRUE(non-zero) if bus has been aquired
*		      Returns FALSE if bus is not available.
*
****************************************************************/
int	negotiateForBus()
{
    unsigned int delayCount = 500;		// wait up to 5ms

// Pulse interrupt line on the PMC

//dprintf("negotiateForBus()\n");
    generateInterruptToPMC(SET_UC5530HSI);
    Delay10uSec();
    generateInterruptToPMC(CLEAR_UC5530HSI);

// Wait for UC$5530D0 to be released by PMC

    do {
	if (PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & UC5530D0)
	    goto weGotIt;
	Delay10uSec();

    } while (delayCount--);			// timeout

// Punt after timeout - fixes hib crash when mp3 playing and
// user tries setting contrast/brightness simultaneously

    enterPreCommandState();
//dprintf("negotiateForBus() failed\n");
    return(FALSE);

weGotIt:

//dprintf("negotiateForBus() ok\n");
    getOnTheBus();			// D3..D0 become outputs
    return (TRUE);			// Return success - we got control of the bus
}

/****************************************************************
* getOnTheBus() - make nibble bus lines outputs.
*
*	Input - None
*
*	Output - None
*
****************************************************************/
void	getOnTheBus()
{
	pm_config_set(GSMI_PM_GPIO_PIN_DIRECTION, 0x00, UC5530D3|UC5530D2|UC5530D1|UC5530D0);
}

/****************************************************************
* getOffTheBus() - make nibble bus lines inputs.
*
*	Input - None
*
*	Output - None
*
****************************************************************/
void	getOffTheBus()
{
	pm_config_set(GSMI_PM_GPIO_PIN_DIRECTION, UC5530D3|UC5530D2|UC5530D1|UC5530D0, 0);
	Delay10uSec();
}

#if 0
/****************************************************************
* notifyOS(int) - Configures the chipset to generate
*		the specified IRQ upon termination of the SMI.
*
*	Input - IRQ - specifies which IRQ is to be generated.
*
* 	Output -  Returns TRUE if the input parameter was valid and the
*	 		  chipset was configured correctly.
*
*			  Returns FALSE if an error was detected.
*
****************************************************************/
int	notifyOS(int notifyType)
{

	switch (notifyType) {

		case NOTIFY_VIA_TSC_IRQ:
			{
				updateUART_DataPending(TSC_IRQ, TSC_UART);
			} break;

		case NOTIFY_VIA_BUTTON_IRQ:
			{
				updateUART_DataPending(MISC_IRQ, MISC_UART);
			} break;

		case NOTIFY_VIA_APM_GET_POWER_STATUS:
			{
			// disable nesting for now.  Posting an APM event causes an INT15
			// callback to XpressROM.  SMI executes continues after XpressROM
			// executes an RSM opcode.  If nesting is enable, we can blow up the
			// nesting levels.
			// NOTE: apm_report_event() handles making sure APM is connected.

				disable_nesting();
				apm_report_event(APM_EVENT_POWER_STATUS_CHANGE_NOTIFICATION);
				enable_nesting();
			} break;

		case NOTIFY_VIA_DONT:
		default:
			break;

	}
	return (TRUE);
}

/****************************************************************
* updateUART_DataPending(irqType) - updates the UART to indicate
*		that there is data available.
*
*	Input - Which UART should be updated.
*
* 	Output -  None
*
****************************************************************/
void updateUART_DataPending(unsigned int irqType, unsigned int uartAddress)
{

//
// check if UART has been configured to generate INTs (RxRDY must be = 1)
//

	if (checkIfUARTIntsAreEnabled(uartAddress))
	{

	// Disable nesting else audio might clobber our IRQ.

		disable_nesting();

		updateF3BAR(AU_HW_IRQ_ENABLE, (unsigned char) irqType);
		irq_set(AU_HW_IRQ_CONTROL, irqType);

		enable_nesting();
	}

//
// even if INTs are disabled, we still need to update the ISR for polling mode
//

	updateUARTInterruptIdentification(uartAddress, UART_INTERRUPT_ID_RECEIVED_DATA);

}
#endif
/****************************************************************
* writeNibbleOnHIB(commandRequest) - writes the passed nibble onto
*		the HIB.
*
*	Input - Nibble to write
*
* 	Output -  None
*
*	Note - commandRequest is AND'd with 0xF0 to make sure that the
*			lower nibble is always '0'.  If bit 2 ever gets set, the
*			micro will reset which results in system power being
*			shut off.  This code makes sure we don't shoot ourselves
*			in the foot!
*
****************************************************************/
void writeNibbleOnHIB(unsigned char commandRequest)
{

    pm_config_set(GSMI_PM_GPIO_PIN_DATA, 0xF0,
    		  (unsigned char)(commandRequest & 0xF0));

}

/****************************************************************
* generateInterruptToPMC() - Generates interrupt request to PMC
*
*	Input - direction : 1 = set output line
*						0 = clear output line
*
* 	Output -  None
*
****************************************************************/
void	generateInterruptToPMC(char direction)
{

	if (direction)
		pm_config_set(GSMI_PM_GPIO_PIN_DATA, 0x00, UC5530HSI);
	else
		pm_config_set(GSMI_PM_GPIO_PIN_DATA, UC5530HSI, 0x00);

}

/****************************************************************
* disableGPIOFromGeneratingAnSMI() - disables edge sense and
*			disables which GPI generates an SMI
*
*
****************************************************************/
void disableGPIOFromGeneratingAnSMI()
{

	pm_config_set(GSMI_PM_GPIO_CONTROL, UCSMI, 0x00);

}

/****************************************************************
* enableGPIOToGenerateAnSMI() - configure edge sense and
*			configure which GPI generates an SMI
*
*
****************************************************************/
void enableGPIOToGenerateAnSMI()
{

	pm_config_set(GSMI_PM_GPIO_CONTROL, 0x00, UCSMI);

}

/****************************************************************
* waitForUCSMI(char) - waits for a defined period of time for the
*	UC$SMI line to transition.
*
*	Input - Edge to look for
*
* 	Output -  Returns TRUE (NON-ZERO) if the edge was detected
*			  Returns FALSE if timeout occured before edge
*
****************************************************************/
int	waitForUCSMI(char edge)
{
  unsigned int delayCount=DELAY_TIME_UCSMI;

	do
	{	if ((PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & UCSMI) == edge)
			return(TRUE);
	    Delay10uSec();
	}
	while (delayCount--);

	return (FALSE);

}

int     waitForUCSMI_2(char edge)
{
	if ((PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & UCSMI) == edge)
		return(TRUE);
	else
		return (FALSE);
}

/****************************************************************
* configureHIBInterface() - correctly configures the GPIO lines
*	for use as the HIB
*
*	Input - none
*
*	Output - none
*
****************************************************************/
void configureHIBInterface()
{

	disableGPIOFromGeneratingAnSMI();

	penIndex = 0;
	penDataReady = FALSE;
//	penIndexUART = 0;
	firstTime = TRUE;
	storagePMCRevision[0] = 0;

	postComplete = FALSE;
	UserButton = FALSE;

	storageBATT[0] = 0;
	storageBATT[1] = 0;

// configure alternative usage properly

    pm_config_set(0x42, 0xC0, 0);		// disable USB SMI causes
    pm_config_set(0x43, 0x44, 0x00); 	// configure alternative usage for GPIOs


// Enter into quiescent state so that the HIB is ready to receive

	enterPreCommandState();

// Draco Initialization...

	getSYSStat();

dprintf("call InitPWM\n");
	InitPWM();					// set initial brightness/contrast
dprintf("InitPWM returned\n");
	enterPreCommandState();		// was FF,FF; b8,00 saves most power

dprintf("call getECver\n");
	getECver();            		// save EC version info
	enterPreCommandState();

//dprintf("call getBATT\n");
//	getBATT();					// save current battery status
//	enterPreCommandState();

dprintf("getSID\n");
	getSID();					// save Serial ID
	enterPreCommandState();

	//TempInit();					// Init temperature stuff

//	updateF3BAR(AU_HW_IRQ_ENABLE, MISC_IRQ);		???? AH
//	updateF3BAR(AU_HW_IRQ_ENABLE, TSC_IRQ);         ???? AH

// configure Edge sense and configure which GPI generates an SMI

	SetHIB_Enable();

	enableGPIOToGenerateAnSMI();

// Enter into quiescent state so that the HIB is ready to receive

	enterPreCommandState();

}


/****************************************************************
* enterPreCommandState(void) - Configures the chipset to be in the
*		known "safe" state. This state allows the PMC to initiate
*		a transation.
*
*	Input - None
*
* 	Output -  None
*
****************************************************************/
void	enterPreCommandState(void)
{

unsigned char temp;

//  1) Tri-state D2,D1,D0 and SMI (make them inputs),
//  2) make D3 & HOSTIRQ outputs

// HOSTIRQ, D3 are outputs

	pm_config_set(GSMI_PM_GPIO_PIN_DIRECTION, UC5530D2|UC5530D1|UC5530D0|UCSMI,
                                              UC5530D3|UC5530HSI);

// Set HOSTIRQ and D3 LOW

	pm_config_set(GSMI_PM_GPIO_PIN_DATA, UC5530D3|UC5530HSI, 0);

	Delay10uSec();
	Delay10uSec();

// We're supposed to wait for SMI and D0 low

//dprintf("enterPreCommandState wait for UCSMI low\n");
	while (waitForUCSMI(WAIT_UCSMI_GOING_LOW) == FALSE) {
    	Delay10uSec();
	}

//dprintf("enterPreCommandState wait for UC5530D0 low\n");
	do {
		Delay10uSec();
		temp = PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0;
	} while((temp & UC5530D0) != 0);
//dprintf("enterPreCommandState done\n");

}

#if 0
/****************************************************************
* WEBPAD_Hook() - Handles all of the PMC->APM and APM->PMC translations.
*
* 	Input - apmFunction - represents which APM routine is calling.
*		    info - Register BX as set by the O/S.
*
* 	Output - Depends on function.
*
****************************************************************/
int	WEBPAD_Hook(unsigned int apmFunction, unsigned int info)
{
//unsigned long temp;

	switch (apmFunction) {

		case 0x0A :
			{

// Webpad 1.0 always has 1 battery unit which can not be removed.
// Draco can be run on AC with battery removed.

		   		pm_info.power.battery_units = 1;

				if (storageDCStatus == 1) 				 // AC adapter present
					pm_info.power.ac_line_status = 0x01; // indicate on AC
				else
					pm_info.power.ac_line_status = 0x00; // indicate on DC

				switch (storageBATT[0]) {

                	// FULL
                	case 4:
                	{   pm_info.power.battery_status  = 0x00;
                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0x09;
						else
			   				pm_info.power.battery_flag = 0x01;
			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent =  storageBATT[1];
                    } break;

                	// NOTLOW
                	case 3:
                	{   pm_info.power.battery_status  = 0x00;

                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0x09;
						else
			   				pm_info.power.battery_flag = 0x01;

			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent =  storageBATT[1];
                    } break;

                	// LOWBAT
                	case 2:
                	{   pm_info.power.battery_status  = 0x01;
                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0x0A;
						else
			   				pm_info.power.battery_flag = 0x02;
			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent =  storageBATT[1];

		                disable_nesting();
		                apm_report_event(APM_EVENT_BATTERY_LOW_NOTIFICATION);
		                enable_nesting();

                    } break;

                	// VLOWBAT
                	case 1:
                	{   pm_info.power.battery_status  = 0x01;
			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent = storageBATT[1];

                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0x0A;
						else {
			   				pm_info.power.battery_flag = 0x02;

                        	disable_nesting();
                     		if (apm_report_event(APM_EVENT_CRITICAL_SYSTEM_SUSPEND_NOTIFICATION) == 0xFFFF)
		                		PowerDown();
                            enable_nesting();
                            
						} //

                    } break;

                	// CRITICAL
                	case 0:
                	{   pm_info.power.battery_status  = 0x02;
			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent = storageBATT[1];

                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0x0C;
						else {
				   			pm_info.power.battery_flag = 0x04;

				   			disable_nesting();
                     		if (apm_report_event(APM_EVENT_CRITICAL_SYSTEM_SUSPEND_NOTIFICATION) == 0xFFFF)
		                		PowerDown();
							enable_nesting();		                		
						}
                    } break;

                    // No Battery present or unidentified type
                    case 0xFF:
					{	pm_info.power.battery_status  = 0xFF;
                		if (storageDCStatus == 1)	 // AC adapter present
                        	pm_info.power.battery_flag = 0xFF;
						else
			   				pm_info.power.battery_flag = 0x80; // no battery
			   			pm_info.power.battery_time = 0xFFFF;
		                pm_info.power.battery_percent =  0xFF;
		   				pm_info.power.battery_units = 0;	// nobody home
                    } break;
				}

			} break;

// Draco Specific APM OEM Extensions

		case 0x80:  // APM OEM extensions.
			{
				switch (info) {

					case 0x80:	// Return COP8 Version info
                        {
                        	SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							TRAP_REGS.r_bh = storagePMCRevision[0];	// major rev number
							TRAP_REGS.r_ch = storagePMCRevision[1];	// 1st digit of minor #
							TRAP_REGS.r_cl = storagePMCRevision[2];	// 2nd digit of minor #
						} break;

					case 0x7F:	// APM OEM Extension Install Check
                        {
                        	SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							TRAP_REGS.r_bx = 0x4358;	// "CX"

                            // We also use this function to indicate
                            // POST has been completed...

                            	postComplete = TRUE;

							//  Init APM variables ???
							//
							//	apm_info.connect=1;
							//  apm_info.enabled=1;
							//  apm_info.engaged=1;
							//  apm_info.driver_version = 0x0102;

						} break;

					case 0x7E:	// Init PWM data
						{
                        	InitPWM();
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							TRAP_REGS.r_bx = 0;		// OEM ID should be ?????
						} break;

					case 0x7D:  // Return current PWM Values
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							TRAP_REGS.r_ch = storagePWMStatus[0];	// Brightness
							TRAP_REGS.r_cl = storagePWMStatus[1];	// Contrast
						} break;

					case 0x7C:  // Set PWM Values
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							SetPWM(TRAP_REGS.r_cl, TRAP_REGS.r_ch);	// Contrast/Brightness
						} break;

					case 0x7B:  // Increment Brightness
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                			IncBrightness();
						} break;

					case 0x7A:  // Decrement Brightness
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                			DecBrightness();
						} break;

					case 0x79:  // Increment Contrast
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                            IncContrast();
						} break;

					case 0x78:  // Decrement Contrast
						{
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                            DecContrast();
						} break;

					case 0x77:	// Init serial ID
                    	{
                    		getSID();
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
							TRAP_REGS.r_bx = 0;		// OEM ID should be ?????
                    	} break;

					case 0x76:  // Return Serial ID
						{
							TRAP_REGS.r_ah = storageSID[1];
							TRAP_REGS.r_al = storageSID[0];
							TRAP_REGS.r_bh = storageSID[3];
							TRAP_REGS.r_bl = storageSID[2];
							TRAP_REGS.r_ch = storageSID[5];
							TRAP_REGS.r_cl = storageSID[4];
							TRAP_REGS.r_dh = storageSID[7];
							TRAP_REGS.r_dl = storageSID[6];
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
						} break;


					case 0x75:	// go to Soft Off state
                    	{
                            PowerDown();
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                    	} break;

					case 0x74:	// User button status during POST
                    	{
							TRAP_REGS.r_al = UserButton;
							SMI_HEADER.r_eflags &= ~SMI_EFLAGS_CF;
                    	} break;

					default:
						{
							TRAP_REGS.r_ah = 0x09;
							SMI_HEADER.r_eflags |= SMI_EFLAGS_CF;
						} break;

				}
			} break;

		default:
			{
			} break;
	}

	return (TRUE);
}

#endif

/****************************************************************
* showDebug() - sends diag codes to POST card.
*
*	Input - code - value to show on POST card.
*
****************************************************************/
#if 0
void showDebug(int code)
{
	outp(0x84, code);
}
#endif

// Draco...

/////////////////////////////////////////////////////////////////////
//	int sendPacketToEC(char * packet) - Send multiple bytes to EC
//
//	Input:	packet = ptr to packet (array of bytes) to send
//
//	Output:	Returns TRUE if packet successfully sent to EC,
//			Returns FALSE if HIB_SendByte returns FALSE to us.
//
/////////////////////////////////////////////////////////////////////

int	sendPacketToEC(unsigned char * packet)
{
unsigned char i, length, data;
int status;

    length = *packet & 0x0F;	// length is low nibble of 1st byte

    enterPreCommandState();

    while(negotiateForBus() == FALSE)		// fix hib crash
    	Delay10uSec();

	for( i = 0; i < length; i++) {
    	data = *packet;
        status = HIB_SendByte(data);
		packet++;
	}

	enterPreCommandState();

	return(status);

}

////////////////////////////////////////////////////////////////
// unsigned char readEC(void)
//
//	Output - Returns byte read from HIB
//
//////////////////////////////////////////////////////////////////

unsigned char readEC_2()
{
	unsigned char temp;
	bigtime_t t1 = system_time();
	while(waitForUCSMI_2(WAIT_UCSMI_GOING_HIGH) == FALSE) {
		if(system_time() - t1 > 200) {
			return 0xff;
		}
		spin(2);
	}
	temp = PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0;
	generateInterruptToPMC(SET_UC5530HSI);
	while(waitForUCSMI(WAIT_UCSMI_GOING_LOW) == FALSE) {
		if(system_time() - t1 > 100000) {
			dprintf("readEC: timeout\n");
			return 0xff;
		}
		Delay10uSec();
	}
	temp |= (PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0) >> 4;
	generateInterruptToPMC(CLEAR_UC5530HSI);
	return temp;
}

unsigned char readEC(void)
{
unsigned char temp;
bigtime_t t1 = system_time();


// SMI HIGH indicates 1st nibble is on the bus

	while(waitForUCSMI(WAIT_UCSMI_GOING_HIGH) == FALSE) {
		if(system_time() - t1 > 100000) {
			dprintf("readEC: timeout 1nb\n");
			return 0xff;
		}
		Delay10uSec();
    }

// Read 1st nibble

	temp = PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0;

// ACK 1st nibble

	generateInterruptToPMC(SET_UC5530HSI);

// SMI low indicates 2nd nibble is on the bus

	while(waitForUCSMI(WAIT_UCSMI_GOING_LOW) == FALSE) {
		if(system_time() - t1 > 100000) {
			dprintf("readEC: timeout 2nb\n");
			return 0xff;
		}
		Delay10uSec();
	}

// Read 2nd nibble

	temp |= (PCIC_IN_8(grappa_PCI_config_addr, GSMI_PM_GPIO_PIN_DATA) & 0xF0) >> 4;

// ACK the 2nd nibble

	generateInterruptToPMC(CLEAR_UC5530HSI);

	return(temp);
}


/////////////////////////////////////////////////////////////////////
//	getPacketFromEC(unsigned char cmdlen) - Get multiple bytes from EC
//
//	Input:  cmdlen = command/length byte from EC
//
//	Output: data put in global HIB_Buffer array
//
/////////////////////////////////////////////////////////////////////

void getPacketFromEC(void)
{
unsigned char i, length, data;

	enterPreCommandState();
    getOffTheBus();
	data = readEC();
    HIB_Buffer[0] = data;

    length = data & 0x0F;	// length is low nibble of 1st byte

	for (i = 1; i < length; i++) {
		HIB_Buffer[i] = readEC();
	}

    enterPreCommandState();

}

void getPacketFromEC_2()
{
	unsigned char i, length, data;
	getOffTheBus();
	data = readEC_2();

	HIB_Buffer[0] = data;
	if(data == 0xff)
		return;
	length = data & 0x0F;
	for (i = 1; i < length; i++) {
		HIB_Buffer[i] = readEC();
	}
	//enterPreCommandState();
}




void
getSYSStat()
{
	HIB_Buffer[0] = 0x12;
	HIB_Buffer[1] = 0x31;
	HIB_Ptr = &HIB_Buffer[0];

//dprintf("getSYSStat: sendPacketToEC\n");
	sendPacketToEC(HIB_Ptr);
//dprintf("getSYSStat: waitSMIpulse\n");
	waitSMIpulse();
//dprintf("getSYSStat: getPacketFromEC\n");
	getPacketFromEC();
	dprintf("sysstat: 0x%02x 0x%02x\n", HIB_Buffer[0], HIB_Buffer[1]);
	cleanUpAfterOurSelves();
}

/////////////////////////////////////////////////////////////////////
//
//	void getECver(void) - get COP8 version info
//
/////////////////////////////////////////////////////////////////////

void getECver(void)
{

	HIB_Buffer[0] = 0x12;		// Request to EC, 2 bytes
    HIB_Buffer[1] = 0x13;		// Get Version info, 3 bytes
    HIB_Ptr = &HIB_Buffer[0];

dprintf("getECver: sendPacketToEC\n");
	sendPacketToEC(HIB_Ptr);
//dprintf("getECver: waitSMIpulse\n");
	waitSMIpulse();
//dprintf("getECver: getPacketFromEC\n");
	getPacketFromEC();
dprintf("getECver: done\n");


dprintf("getECver: got 0x%02x 0x%02x 0x%02x 0x%02x\n", HIB_Buffer[0], HIB_Buffer[1], HIB_Buffer[2], HIB_Buffer[3]);
	storagePMCRevision[0] = HIB_Buffer[1];
	storagePMCRevision[1] = HIB_Buffer[2];
	storagePMCRevision[2] = HIB_Buffer[3];

	cleanUpAfterOurSelves();

}


uint8 lastPenXh = 0;
uint8 lastPenYh = 0;
bigtime_t lastPenTime = 0;

void
readPenStatus(char *status)
{
	int		i;
	int		got_data = false;
	bigtime_t t1;

    HIB_Buffer[0] = 0x12;		// Request to EC, 2 bytes
    HIB_Buffer[1] = 0x74;		// Get Pen status, 4 bytes
//HIB_Buffer[1] = 0x41;

    HIB_Ptr = &HIB_Buffer[0];
snooze(3000);
i=0;
retry:
//	disable_nesting();
//	sendPacketToEC(HIB_Ptr);
//	if(waitSMIpulse2() == TRUE) {
		got_data = TRUE;
		t1 = system_time();
		getPacketFromEC_2();
//	if(i==1)
//		dprintf("getPacketFromEC took %Ld us\n", system_time()-t1);
//	}
//	enable_nesting();
if(HIB_Buffer[0] != 0x75 || (HIB_Buffer[1] & 0x0f) || (HIB_Buffer[3] & 0x0f)) {
	if(HIB_Buffer[0] == 0x42) {
		uint8 buttons = HIB_Buffer[1];
		dprintf("button 0x%02x\n", HIB_Buffer[1]);
		if(buttons & 0x01)
			IncBrightness();
		if(buttons & 0x02)
			DecBrightness();
		if(buttons & 0x04)
			DecContrast();
		if(buttons & 0x08)
			IncContrast();
		status[0] = 0xfe;
		status[1] = 0xfe;
		status[2] = 1;
		status[3] = buttons;
		goto done;
	}
	i++;
	if(i< 50) {
		snooze(3000);
		goto retry;
	}
	got_data = FALSE;
}
else {
//	dprintf("getPacketFromEC good sample took %Ld us\n", system_time()-t1);
//	dprintf("readPenStatus: got 0x%02x command from EC\n", HIB_Buffer[0]);
}

	if(got_data == FALSE) {
//		dprintf("readPenStatus: SMI timeout\n");
		status[0] = 0xfe;
		status[1] = 0xfe;
		status[2] = 0;
		status[3] = 0;
		lastPenTime = 0;
	}
	else {
		if(system_time() - lastPenTime < 1000000) {
			int dx = (int)lastPenXh - (int)(HIB_Buffer[2]);
			int dy = (int)lastPenYh - (int)(HIB_Buffer[4]);
			if(dx < -4 || dx > 4 || dy < -4 || dy > 4) {
				i++; 
				if(i< 100) {
				goto retry;
				}
			}
		}
		lastPenXh = HIB_Buffer[2];
		lastPenYh = HIB_Buffer[4];
		lastPenTime = system_time();
		for(i=0; i<4; i++)
			status[i] = HIB_Buffer[1+i];
	}
done:
	cleanUpAfterOurSelves();
}

/////////////////////////////////////////////////////////////////////
//
// void InitPWM(void) - Default PWM settings (Contrast/Brightness)
//
/////////////////////////////////////////////////////////////////////

void InitPWM(void)
{
	SetPWM(DEF_CONTRAST, DEF_BRIGHT);		// HW version dependent
}


/////////////////////////////////////////////////////////////////////
//
// int getPWM(void) - get current PWM (Brightness/Contrast) status
//
/////////////////////////////////////////////////////////////////////

void getPWM(void)
{

    HIB_Buffer[0] = 0x12;		// Request to EC, 2 bytes
    HIB_Buffer[1] = 0x62;		// Get PWM status, 2 bytes
    HIB_Ptr = &HIB_Buffer[0];

	disable_nesting();
	sendPacketToEC(HIB_Ptr);
	waitSMIpulse();
	getPacketFromEC();
	enable_nesting();

	storagePWMStatus[0] = HIB_Buffer[1];	// brightness
    storagePWMStatus[1] = HIB_Buffer[2];	// contrast

	cleanUpAfterOurSelves();

}


/////////////////////////////////////////////////////////////////////
//
// void getBATT(void) - get current battery status
//
/////////////////////////////////////////////////////////////////////

void getBATT(void)
{

	HIB_Buffer[0] = 0x12;				// Request to EC, 2 bytes
    HIB_Buffer[1] = 0x82;				// Get battery status, 2 bytes
    HIB_Ptr = &HIB_Buffer[0];

	sendPacketToEC(HIB_Ptr);
	waitSMIpulse();
	getPacketFromEC();

dprintf("getBATT: got 0x%02x 0x%02x 0x%02x\n", HIB_Buffer[0], HIB_Buffer[1], HIB_Buffer[2]);
	storageBATT[0] = HIB_Buffer[1];		// status
    storageBATT[1] = HIB_Buffer[2];		// gas guage

	cleanUpAfterOurSelves();

}



int
SetHIB_Enable()
{
	HIB_Buffer[0] = 0x22;
	HIB_Buffer[1] = 1;
	 HIB_Ptr = &HIB_Buffer[0];
	disable_nesting();
	sendPacketToEC(HIB_Ptr);
	enable_nesting();
                      
	cleanUpAfterOurSelves();
	return 1;
}

/////////////////////////////////////////////////////////////////////
//
// int SetPWM(unsigned char contrast, unsigned char brightness)
//
/////////////////////////////////////////////////////////////////////

int SetPWM(unsigned char contrast, unsigned char brightness)
{
int status;

	if (contrast > MAX_CONTRAST)			// HW dependent
		contrast = MAX_CONTRAST;

	if (contrast < MIN_CONTRAST)
		contrast = MIN_CONTRAST;

	HIB_Buffer[0] = 0x63;					// Set_PWM cmd, 3 bytes
    HIB_Buffer[1] = contrast;
    HIB_Buffer[2] = brightness;

    HIB_Ptr = &HIB_Buffer[0];

dprintf("SetPWM(0x%02x, 0x%02x)\n", contrast, brightness);
//dprintf("SetPWM: sendPacketToEC\n");
	disable_nesting();
    status = sendPacketToEC(HIB_Ptr);
//dprintf("SetPWM: sendPacketToEC returned\n");
	enable_nesting();

    if (status == TRUE) {
    	storagePWMStatus[0] = HIB_Buffer[2];	// update internally too
    	storagePWMStatus[1] = HIB_Buffer[1];
    }

	cleanUpAfterOurSelves();
    return(status);

}

/////////////////////////////////////////////////////////////////////
//
// int getSID(void) - get Serial ID
//
/////////////////////////////////////////////////////////////////////

void getSID(void)
{
int i;

	HIB_Buffer[0] = 0x12;		// Request to EC, 2 bytes
    HIB_Buffer[1] = 0xB8;		// Get Serial ID, 8 bytes
    HIB_Ptr = &HIB_Buffer[0];

    disable_nesting();
    sendPacketToEC(HIB_Ptr);
    waitSMIpulse();
	getPacketFromEC();

    for(i=0; i<8; i++)
    	storageSID[i] = HIB_Buffer[i+1];

	enable_nesting();

	cleanUpAfterOurSelves();

}



/////////////////////////////////////////////////////////////////////
//
// int IncBrightness(void) - increase brightness PWM
//
/////////////////////////////////////////////////////////////////////

int IncBrightness(void)
{

    if((storagePWMStatus[0] -= 16) < 0x80)	// increment current value
	    storagePWMStatus[0] = 0x80;			//  and save it

	return(SetPWM((byte) storagePWMStatus[1], (byte) storagePWMStatus[0]));
}

/////////////////////////////////////////////////////////////////////
//
// int DecBrightness(void) - decrease brightness PWM
//
/////////////////////////////////////////////////////////////////////

int DecBrightness(void)
{

    if((storagePWMStatus[0] += 16) > 0xf0)	// decrement current value
	    storagePWMStatus[0] = 0xf0;			//  and save it

	return(SetPWM((byte) storagePWMStatus[1], (byte) storagePWMStatus[0]));

}

/////////////////////////////////////////////////////////////////////
//
// int IncContrast(void) - increase contrast PWM
//
/////////////////////////////////////////////////////////////////////

int DecContrast(void)
{

    if((storagePWMStatus[1] += C_DELTA) > MAX_CONTRAST)	// increment current value
	    storagePWMStatus[1] = MAX_CONTRAST;			//  and save it

	return(SetPWM((byte) storagePWMStatus[1], (byte) storagePWMStatus[0]));

}

/////////////////////////////////////////////////////////////////////
//
// int DecContrast(void) - decrease contrast PWM
//
/////////////////////////////////////////////////////////////////////
int IncContrast(void)
{

    if((storagePWMStatus[1] -= C_DELTA) < MIN_CONTRAST)	// decrement current value
	    storagePWMStatus[1] = MIN_CONTRAST;			//  and save it

	return(SetPWM((byte) storagePWMStatus[1], (byte) storagePWMStatus[0]));

}

/////////////////////////////////////////////////////////////////////
//
// void PowerDown(void) - transition to soft off state
//
/////////////////////////////////////////////////////////////////////

extern unsigned short apm_report_event(unsigned short);
extern int bios_screen_control(unsigned char);
extern int ds_flat_panel_power_down(void);

#if 0
void PowerDown(void)
{
int i;

	disable_nesting();

//    ds_flat_panel_power_down();				// blank the screen

// Make audio an input to reduce popping

	pm_config_set(GSMI_PM_GPIO_PIN_DIRECTION, AUDAMPSHDN, 0);

    outp(0x80, 0xB0);

// Disable HIB - don't want any incoming transactions...

	HIB_Buffer[0] = 0x22;					// Host Set HIB
    HIB_Buffer[1] = 0x00;					// Disabled
    HIB_Ptr = &HIB_Buffer[0];

    while(sendPacketToEC(HIB_Ptr) == FALSE)
    	Delay10uSec();

    outp(0x80, 0xB1);
	for(i = 0; i < 1000; i++)				// wait while caps drain...
		Delay1Mil();

    outp(0x80, 0xB2);
	HIB_Buffer[0] = 0x32;					// Set_SysMode cmd, 1 bytes
    HIB_Buffer[1] = 0x02;					// Mode = SOFTOFF
    HIB_Ptr = &HIB_Buffer[0];

    while(sendPacketToEC(HIB_Ptr) == FALSE)
    	Delay10uSec();


die:
	goto die;

}

#endif


#if 0
////////////////////////////////////////////////////////////////////
//
// void TempWarm(void) - enable suspend modulation to cool off
//
////////////////////////////////////////////////////////////////////

void TempWarm(void)
{
// Enable CPU suspend modulation

	smm_info.pm_suspmod |=  GPMSMI_ENABLE_SUSPEND_MOD;
	pm_config_wr_8(GSMI_PM_SUSPEND_CONFIGURATION, smm_info.pm_suspmod);

}

////////////////////////////////////////////////////////////////////
//
// void TempHot(void) - Shut Down to cool off
//
////////////////////////////////////////////////////////////////////

void TempHot(void)
{

	disable_nesting();
	if (apm_report_event(APM_EVENT_CRITICAL_SYSTEM_SUSPEND_NOTIFICATION) == 0xFFFF)
		PowerDown();
	enable_nesting();

}

////////////////////////////////////////////////////////////////////
//
// void TempOK(void) - return to normal operation
//
////////////////////////////////////////////////////////////////////

void TempOK(void)
{
// Disable CPU suspend modulation

	smm_info.pm_suspmod &=  ~GPMSMI_ENABLE_SUSPEND_MOD;
	pm_config_wr_8(GSMI_PM_SUSPEND_CONFIGURATION, smm_info.pm_suspmod);

}

////////////////////////////////////////////////////////////////////
//
// void TempInit(void) - initialize temperature related stuff
//
////////////////////////////////////////////////////////////////////

void TempInit(void)
{
unsigned char local_ccr2;

// Initialize CPU suspend modulation

	pm_config_wr_16(GSMI_PM_SUSPEND_MOD_OFF_COUNT,   0x0402); // 33% duty cycle
	pm_config_wr_8(GSMI_PM_VIDEO_SPEEDUP_TIMER_COUNT, 50);	  // ms of video speedup
	pm_config_wr_8(GSMI_PM_IRQ_SPEEDUP_TIMER_COUNT,    4);	  // ms of IRQ speedup

	// Video Speedup
	ENABLE_PM_SET(GPMSMI_ENABLE_VIDEO_SPEEDUP | GPMSMI_ENABLE_IRQ_SPEEDUP);

 	smm_info.pm_suspmod |=  GPMSMI_ENABLE_SMI_SPEEDUP;

	// Enable Suspend-on-HLT (for APM's CPU_Idle function)

	local_ccr2 = GET_CYRIX_REG(CCR2) | CCR2_HALT | CCR2_SUSP;
	PUT_CYRIX_REG(CCR2, local_ccr2 );

	smm_info.pm_suspmod &=  ~GPMSMI_ENABLE_SUSPEND_MOD;
	pm_config_wr_8(GSMI_PM_SUSPEND_CONFIGURATION, smm_info.pm_suspmod);

}
#endif

////////////////////////////////////////////////////////////////////
//
//	int waitSMIpulse(void) - less forgiving than waitForUCSMI
//
////////////////////////////////////////////////////////////////////

int waitSMIpulse(void)
{
bigtime_t t1 = system_time();

    while(waitForUCSMI(WAIT_UCSMI_GOING_LOW)==FALSE)
	if(system_time() - t1 > 1000000) {
		dprintf("waitSMIpulse timeout low 1\n");
		return FALSE;
	}
    while(waitForUCSMI(WAIT_UCSMI_GOING_HIGH)==FALSE)
	if(system_time() - t1 > 1000000) {
		//dprintf("waitSMIpulse timeout high\n");
		return FALSE;
	}
    while(waitForUCSMI(WAIT_UCSMI_GOING_LOW)==FALSE)
	if(system_time() - t1 > 1000000) {
		dprintf("waitSMIpulse timeout low 2\n");
		return FALSE;
	}
//dprintf("waitSMIpulse done\n");

	return(TRUE);

}


int waitSMIpulse3()
{
	bigtime_t t1 = system_time();
	bigtime_t t2, t3;
	while(waitForUCSMI(WAIT_UCSMI_GOING_LOW)==FALSE)
		if(system_time() - t1 > 1000000) {
			dprintf("waitSMIpulse2 timeout low 1\n");
			return FALSE;
		}
	t2 = system_time();
	while(waitForUCSMI(WAIT_UCSMI_GOING_HIGH)==FALSE)
		if(system_time() - t1 > 1000000) {
			dprintf("waitSMIpulse timeout high\n");
			return FALSE;
		}
	t2 = system_time();
	while(waitForUCSMI(WAIT_UCSMI_GOING_LOW)==FALSE)
		if(system_time() - t1 > 1000000) {
			dprintf("waitSMIpulse timeout low 2\n");
			return FALSE;
		}
	t3 = system_time();
	dprintf("got SMI pulse %Ld us width after %Ld us\n", t3-t2, t2-t1);
	return TRUE;
}

int
waitSMIpulse2()
{
	uint8 val;
	bigtime_t t1 = system_time();
	while(((val = PCIC_IN_8(0, 0xf7)) & 0x40) == 0) {
		if(system_time() - t1 > 1000000) {
			dprintf("waitSMIpulse2 timeout val 0x%02x\n", val);
			return FALSE;

		}
	}
	dprintf("got SMI val 0x%02x\n", val);
	return TRUE;
}


////////////////////////////////////////////////////////////////////
//
//	void delay5us(void) - the name says it all...
//
////////////////////////////////////////////////////////////////////

void delay5us(void)
{
	spin(5);
}

void Delay10uSec()
{
	spin(10);
}

////////////////////////////////////////////////////////////////////
//
//	void Delay1Mil(void) - the name says it all...
//
////////////////////////////////////////////////////////////////////

//void Delay1Mil(void)
//{
//	snooze(1000);
//}

