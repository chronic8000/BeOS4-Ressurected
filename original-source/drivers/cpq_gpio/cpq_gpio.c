/*******************************************************************************
/
/	File:			driver.c
/
/   Description:	The cpqgpio driver allows a client to read 16 bytes of data.
/                   Each byte has the same value (by default 0); the client
/                   can change this value with write or ioctl commands.
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/	This file may be used under the terms of the Be Sample Code License.
/
*******************************************************************************/

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <PCI.h>

#include <malloc.h>
#include <string.h>

#define DPRINTF(a) dprintf("cpqgpio: "); dprintf a

#define MAX_SIZE 16
#define DEVICE_NAME "misc/cpq_gpio"

#define TOUCH(x) ((void)x)


/*********************************/


#define GPI_REG_OFFSET		0x48
#define        GPI0            0x0001
#define        GPI1            0x0002
#define        GPI2            0x0004
#define        GPI3            0x0008
#define        GPI4            0x0010
#define        GPI5            0x0020
#define        GPI6            0x0040
#define        GPI7            0x0080
#define        GPI8            0x0100
#define        GPI9            0x0200
#define        GPI10           0x0400
#define        GPI11           0x0800

#define GPO_REG_OFFSET		0x4C
#define        GPO0            0x00000001
#define        GPO1            0x00000002
#define        GPO2            0x00000004
#define        GPO3            0x00000008
#define        GPO4            0x00000010
#define        GPO5            0x00000020
#define        GPO6            0x00000040
#define        GPO7            0x00000080
#define        GPO8            0x00000100
#define        GPO9            0x00000200
#define        GPO10           0x00000400
#define        GPO11           0x00000800
#define        GPO12           0x00001000
#define        GPO13           0x00002000
#define        GPO14           0x00004000
#define        GPO15           0x00008000
#define        GPO16           0x00010000
#define        GPO17           0x00020000
#define        GPO18           0x00040000
#define        GPO19           0x00080000
#define        GPO20           0x00100000
#define        GPO21           0x00200000
#define        GPO22           0x00400000
#define        GPO23           0x00800000

//************************************************************************************
//Globals Variables
//************************************************************************************
static uint16 PM_IO_BASE_ADDRESS = 0xEE00;
static uint8 debugMode = 1;
static char dbgStr[256];

typedef struct digit_state {
	uchar value;
}DIGIT_STATE;

//************************************************************************************
//Prototypes
//************************************************************************************
uint8 ReadPCIByte( uchar bus, uchar device, uchar function, long offset );
void WritePCIByte( uchar bus, uchar device, uchar function, long offset, long value );
uint16 ReadPCIWord( uchar bus, uchar device, uchar function, long offset );
void WritePCIBWord( uchar bus, uchar device, uchar function, long offset, long value );
void GetPMIOBase();
uint32 GetGPOMask( uint16 GPONum );
void SetGPO( uint16 GPONum, uchar LEDState );

static status_t
cpqgpio_open(const char *name, uint32 mode, void **cookie)
{
	struct digit_state *s;

	TOUCH(name); TOUCH(mode);

	DPRINTF(("open\n"));

	s = (DIGIT_STATE*) malloc(sizeof(*s));
	if (!s)
		return B_NO_MEMORY;

	s->value = 0;

	*cookie = s;

	return B_OK;
}

static status_t
cpqgpio_close(void *cookie)
{
	TOUCH(cookie);

	DPRINTF(("close\n"));

	return B_OK;
}

static status_t
cpqgpio_free(void *cookie)
{
	DPRINTF(("free\n"));

	free(cookie);

	return B_OK;
}

static status_t
cpqgpio_ioctl(void *cookie, uint32 op, void *data, size_t len)
{

	TOUCH(len);

	
	DPRINTF(("ioctl\n"));

	if (op == 0x1000 )
	{
		struct digit_state *s = (struct digit_state *)cookie;
		s->value = *(uchar *)data;
		DPRINTF(("Hotswapp Function 0\n", s->value));

		return B_OK;
	}
	else if( (op & 0xFF00) == 0x6900 )
	{
		struct digit_state *s = (struct digit_state *)cookie;
		DPRINTF(("Toggle GPO(%2d) to %2.2x\n", (uint16)(op & 0x00FF), *(uchar *)data ));
		SetGPO( (uint16)(op & 0x00FF ), *(uchar *)data );
		return B_OK;
	}

	return ENOSYS;
}

static status_t
cpqgpio_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	DPRINTF(("read\n"));

	if (pos >= MAX_SIZE)
		*len = 0;
	else if (pos + *len > MAX_SIZE)
		*len = (size_t)(MAX_SIZE - pos);
	if (*len)
		memset(buffer, s->value, *len);
	return B_OK;
}

static status_t
cpqgpio_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	TOUCH(pos);

	s->value = ((uchar *)buffer)[*len - 1];

	DPRINTF(("write: set digit to %2.2x\n", s->value));

	return B_OK;
}

/***************************/

status_t init_hardware()
{
	uint8 gpioCtrlReg1, gpioCtrlReg3;	
	DPRINTF(("Init_hardware\n"));
   
	GetPMIOBase();
	
	//activate GPO8 for "EMAIL LED"
	gpioCtrlReg1 = ReadPCIByte( 0, 7, 0, 0x74 );
	WritePCIByte( 0, 7, 0, 0x74, (gpioCtrlReg1 | 0x04) );
	gpioCtrlReg3 = ReadPCIByte( 0, 7, 0, 0x76 );
	WritePCIByte( 0, 7, 0, 0x76, (gpioCtrlReg3 & 0xFE) );
	
	return B_OK;
}

const char **publish_devices()
{
	static const char *devices[] = {
		DEVICE_NAME, NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	static device_hooks hooks = {
		&cpqgpio_open,
		&cpqgpio_close,
		&cpqgpio_free,
		&cpqgpio_ioctl,
		&cpqgpio_read,
		&cpqgpio_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}

status_t init_driver()
{
	return B_OK;
}

void uninit_driver()
{
}

uint8 ReadPCIByte( uchar bus, uchar device, uchar function, long offset )
{
	long retVal;
	retVal = read_pci_config( bus, device, function, offset, (long)1 );
	return( (uint8)retVal );
}

void WritePCIByte( uchar bus, uchar device, uchar function, long offset, long value )
{
	long retVal;
	retVal = write_pci_config( bus, device, function, offset, (long)1, value );
	return;
}

uint16 ReadPCIWord( uchar bus, uchar device, uchar function, long offset )
{
	long retVal;
	retVal = read_pci_config( bus, device, function, offset, (long)2 );
	return( (uint16)retVal );
}

void WritePCIWord( uchar bus, uchar device, uchar function, long offset, long value )
{
	long retVal;
	retVal = write_pci_config( bus, device, function, offset, (long)2, value );
	return;
}

void GetPMIOBase()
{

	PM_IO_BASE_ADDRESS = 0;
	
	if( ( ReadPCIWord( 0, 7, 4, 0 ) == 0x1106 ) 
		&& ( ReadPCIWord( 0, 7, 4, 2 ) == 0x3057 ) )
	{
	
		PM_IO_BASE_ADDRESS = ReadPCIWord( 0, 7, 4, 0x48 ) -1;
	}
	
	DPRINTF(("PM IO Base = %4.4X\n", PM_IO_BASE_ADDRESS) );

}


uint32 GetGPOMask( uint16 GPONum )
{
	if( GPONum > 23 )
		return( 0 );
	else
		return( 0x00000001 << (uint32)(GPONum) );
}

// IOCNTRL function 0x6900 - SetGPO
void SetGPO( uint16 GPONum, uchar LEDState )
{
	uint16 GPORegPort;

	uint32 gpoVal, gpoMask;

	GPORegPort = PM_IO_BASE_ADDRESS + GPO_REG_OFFSET;
	gpoMask = GetGPOMask( GPONum );
	
	gpoVal = read_io_32( GPORegPort );
	if( debugMode )
	{
		DPRINTF( ("InGPO REG(%4.4X): %8.8X\n", GPORegPort, gpoVal) );
	}

	if( LEDState )            //old:(gpoVal & 0x01 )
		gpoVal = gpoVal & ~gpoMask;		//; Clear GPOXX
	else
		gpoVal = gpoVal | gpoMask;		//; Set GPOXX
	if( debugMode )
	{
		DPRINTF( ("OutGPO Reg(%4.4X): %8.8X\n", GPORegPort, gpoVal) );
	}
	write_io_32( GPORegPort, gpoVal );  
}


// Start of code for Hot Swapping control
//;******************************************************************************
//;
//;  EnCompactFlashSlot						;*WRJ
//;
//;  This routine will check if a CF card is in the CF slot.  If CF card is present, we will 
//;  enable the CF slot
//;		- Check if CF Card Present
//;			GPI10 = 0 Card Present
//;			GPI10 = 1 No Card
//;		Then if card is present
//;			- turning on power CF slot voltage, GPO9=1
//;			- Asserting a Hardware Reset to the IDE devices, GPO5=1 for 20 uSecs
//;
//;       Entry:  none
//;
//;       Exit:   none
//;
//;------------------------------------------------------------------------------

void GetGPIOControlRegs()
{
	uint8 gpioCtrlReg1;
	uint8 gpioCtrlReg2;
	uint8 gpioCtrlReg3;
   
	gpioCtrlReg1 = ReadPCIByte( 0, 7, 0, 0x74 );
	gpioCtrlReg2 = ReadPCIByte( 0, 7, 0, 0x75 );
	gpioCtrlReg3 = ReadPCIByte( 0, 7, 0, 0x76 );

	DPRINTF(("GPIO Control Reg1 = %2.2X\n", gpioCtrlReg1) );
	DPRINTF(("GPIO Control Reg2 = %2.2X\n", gpioCtrlReg2) );
	DPRINTF(("GPIO Control Reg3 = %2.2X\n", gpioCtrlReg3) );

}


void SetGPIOControlRegs()
{
	uint8 gpioCtrlReg1;
	uint8 gpioCtrlReg3;
   
   //active GPO9 "CF_VOLTAGE"
	gpioCtrlReg1 = ReadPCIByte( 0, 7, 0, 0x74 );
	WritePCIByte( 0, 7, 0, 0x74, (gpioCtrlReg1 | 0x08) );

   //active GPO8 "MAIL LIGHT"
	gpioCtrlReg1 = ReadPCIByte( 0, 7, 0, 0x74 );
	WritePCIByte( 0, 7, 0, 0x74, (gpioCtrlReg1 | 0x04) );
	gpioCtrlReg3 = ReadPCIByte( 0, 7, 0, 0x76 );
	WritePCIByte( 0, 7, 0, 0x76, (gpioCtrlReg3 & 0xFE) );

}



//int CFDoorStatus()
//{
//	uint16 GPIRegPort;
//   uint32 gpiVal;
//
//// NOT IMPLEMENTED
//   return( -1 );
//	
//   // offset GPI3(LID) if configured as SMI 
////	GPIRegPort = PM_IO_BASE_ADDRESS + 0x44; 
//   
//   // offset GPI3(LID) if configured as GPI
//	GPIRegPort = PM_IO_BASE_ADDRESS + GPI_REG_OFF;
//                                                    
//	gpiVal = InpDW( GPIRegPort );
//	if( debugMode )
//   {
//   	printf("\r\n InGPI REG(%4.4X): %8.8X", GPIRegPort, gpiVal );
//   }
//	if( gpiVal & 0x08 )		// Test for GPI3(LID) if configured as SMI 
//   	return( FALSE );
//   else
//   	return( TRUE );
//
//}
//
//
//int CheckCompactFlashPresent()
//{
//	uint16 GPIRegPort;
//   uint32 gpiVal;
//	
//	GPIRegPort = PM_IO_BASE_ADDRESS + GPI_REG_OFFSET;
//
//	//;Check if CF Card is present.
//	gpiVal = read_io_32( GPIRegPort );
//	if( debugMode )
//   {
//		DPRINTF( ("InGPI REG(%4.4X): %8.8X\n", GPIRegPort, gpiVal) );
//   }
//	if( gpiVal & GPI10 )		//; Test for GPI10 
//		return( FALSE );
//	else
//		return( TRUE );
//}


//void myDelay(uint16 milliSecs )
//{
////   clock_t start, end;
////   start = clock();
////
////   snooze(milliSecs);
//
////   end = clock();
////   printf("\r\nThe time was: %f\n", (end - start) / CLK_TCK);
//}



//void ToggleCFVoltageOff()
//{
//  	uint16 GPORegPort;
//   uint32 gpoVal;
//	
//	GPORegPort = PM_IO_BASE_ADDRESS + GPO_REG_OFFSET;
//                       
//	if( debugMode )
//		DPRINTF(("CF Voltage OFF\n"));
//
//   gpoVal = read_io_32( GPORegPort );
//   if( debugMode )
//   {
//     	DPRINTF(("InGPO REG(%4.4X): %2.2X\n", GPORegPort, gpoVal));
//   }
//   gpoVal = gpoVal & ~GPO9;		//; Clear GPO9 to toggle CF_voltage OFF
//	if( debugMode )
//	{
//		DPRINTF( ("OutGPO Reg(%4.4X): %8.8X\n",GPORegPort, gpoVal ) );
//   }
//	write_io_32( GPORegPort, gpoVal ); 
//}
//
//void ToggleCFVoltageOn()
//{
//  	uint16 GPORegPort;
//   uint32 gpoVal;
//	
//	GPORegPort = PM_IO_BASE_ADDRESS + GPO_REG_OFFSET;
//
//	if( debugMode )
//     	DPRINTF(("CF Voltage ON\n"));
//         
//	gpoVal = read_io_32( GPORegPort );
//	if( debugMode )
//  	{
//   		DPRINTF( ("InGPO REG(%4.4X): %8.8X\n",GPORegPort, gpoVal) );
//   }
//                                     
//  	gpoVal = gpoVal | GPO9;		//; Set GPO9 to turn CF Voltage ON
//	if( debugMode )
//  	{
//  		DPRINTF( ("OutGPO Reg(%4.4X): %8.8X\n",GPORegPort, gpoVal) );
//   }
//  	write_io_32( GPORegPort, gpoVal );
//}
//
//
//void IssueDriveReset(uint16 drvNum )
//{
//	union REGS regs;
//
//   regs.h.ah = 0; 
//   regs.h.dl = drvNum;
//	int86( 0x13, &regs, &regs);
//}
//                                      
//void AssertManualRESETDRV()
//{
//  	uint16 GPORegPort;
//	uint8 gpoVal;
//	
//	GPORegPort = PM_IO_BASE_ADDRESS + 0x004C;
//                       
//	//If Card is present and voltage to slot, so assert the -RESET to the IDE devices for 20 uSecs
////	if( CheckCompactFlashPresent() )
////   {
//
//		gpoVal = read_io_8( GPORegPort );
//		if( debugMode )
//		{
//			DPRINTF( ("InGPO REG(%4.4X): %2.2X\n", GPORegPort, gpoVal) );
//		}
//		gpoVal = gpoVal & 0xDF;		//; clear GPO5 to assert a -RESET to the IDE devices
//		if( debugMode )
//		{
//			DPRINTF(("OutGPO Reg(%4.4X): %2.2X\n", GPORegPort, gpoVal) );
//		}
//		if( debugMode )
//			DPRINTF(("Assert RESET-\n"));
//		write_io_8( GPORegPort, gpoVal ); 
//
//		if( debugMode )
//		{
//			DPRINTF(("Wait 20 mSecs\n"));
//		}                         
//	   snooze(20*1000);
//      
//		//Deassert the -RESET to the IDE devices
//		if( debugMode )
//      	DPRINTF(("Deassert RESET\n"));
//		gpoVal = read_io_8( GPORegPort );
//		if( debugMode )
//   	{
// 			DPRINTF( ("InGPO REG(%4.4X): %2.2X\n",GPORegPort, gpoVal) );
//	   }
//                                     
//   	gpoVal = gpoVal | 0x20;		//; Set GPO5 to deassert a RESET- to the IDE devices
//		if( debugMode )
//   	{
// 			DPRINTF( ("OutGPO Reg(%4.4X): %2.2X\n", GPORegPort, gpoVal) );
//	   }
//   	write_io_8( GPORegPort, gpoVal );
////   }
//}


//<EOF>
