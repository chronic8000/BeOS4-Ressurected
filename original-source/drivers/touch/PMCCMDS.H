/**************** Copyright (c) 1999 National Semiconductor *****************
*
* File:         PMCcmds.h
*
* Summary:      PMC command set defines
*
* Contents:     Contains defines used with the PMC on WebPad
*
*  For evaluation purposes only - not for production usage
*
****************************************************************/

//
// Host-to-PMC commands
//
//#define HOST_TO_PMC_NOP			0X00		// NOP Command
//#define HOST_TO_PMC_BATT_STAT		0x10		// Battery Voltage + DC Sense
//#define HOST_TO_PMC_BUTT_STAT		0x20		// Button Status (bit 6:0)
//#define HOST_TO_PMC_PEN_STAT 		0x30		// Pen X/Y info
//#define HOST_TO_PMC_reserved1    	0x40		// available command
//#define HOST_TO_PMC_TEMP_STAT		0x50		// Temperature Status
//#define HOST_TO_PMC_STANDBY		0x60		// Request to Standby PMC
//#define HOST_TO_PMC_REVISION		0X70		// Request PMC revision info

#define HOST_TO_PMC_SYSTEM_CONTROL 0x80		// Data contains embedded command

#define	HOST_TO_PMC_NOP				0x00		// NOP Command
#define HOST_TO_PMC_RQST			0x10		// HOST->EC Request
#define	HOST_TO_PMC_SET_HIB			0x20		// Enable/Disable HIB
#define	HOST_TO_PMC_SET_SYSMODE		0x30		// Set System Mode
#define HOST_TO_PMC_SET_BUTTONS    	0x40		// Set Typematic parms
#define	HOST_TO_PMC_SET_TEMPS		0x50		// Set Temperature trip points
#define HOST_TO_PMC_SET_PWMS		0x60		// Set PWM duty cycles
#define HOST_TO_PMC_RESERVED		0X70		// Reserved 07..0E
#define HOST_TO_PMC_UART_WRITE		0xF0		// Write byte to EC UART

//
// PMC-to-Host commands
//
//#define	PMC_TO_HOST_NOP				0x00		// NOP Command
//#define	PMC_TO_HOST_BUTT_STAT		0x10		// Button Status (bit 6:0)
//#define	PMC_TO_HOST_PEN_STAT		0x20		// Pen X/Y info
//#define	PMC_TO_HOST_OVERTEMP1		0x30		// Temperature Status
//#define	PMC_TO_HOST_BATT_STAT 		0x40		// Battery Voltage + DC Sense
//#define	PMC_TO_HOST_REVISION		0x50		// PMC Revision String
//#define	PMC_TO_HOST_RESERVED2		0x60		//
//#define	PMC_TO_HOST_RESERVED3		0x70		// available command

#define	PMC_TO_HOST_NOP			0x00		// NOP Command
#define	PMC_TO_HOST_VERSION		0x10		// Version info
#define	PMC_TO_HOST_RESERVED	0x20		//
#define	PMC_TO_HOST_SYS_STAT	0x30		// System mode
#define	PMC_TO_HOST_BUTT_STAT	0x40		// Button status
#define	PMC_TO_HOST_TEMP_STAT	0x50		// Temperature status
#define	PMC_TO_HOST_PWM_STAT	0x60		// PWM status
#define	PMC_TO_HOST_PEN_STAT	0x70		// Touch screen Pen data
#define	PMC_TO_HOST_BATT_STAT	0x80		// Battery status
#define	PMC_TO_HOST_DC_STAT		0x90		// AC adapter status
#define	PMC_TO_HOST_TIME_STAT	0xA0		// System timer tick
#define PMC_TO_HOST_SERIAL_ID	0xB0		// Seial ID data
//								0xC0..0xE0 reserved
#define	PMC_TO_HOST_UART_READ	0xF0		// Read byte from EC UART


