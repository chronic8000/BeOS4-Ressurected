/*****************************************************************************
 *	Filename: PCI_Functions.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Description: 
 *****************************************************************************/

#ifndef _INCLUDE_PCI_FUNCTIONS_H_
#define _INCLUDE_PCI_FUNCTIONS_H_

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


//PCI FUNCTIONS

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
		dprintf ("InGPO REG(%4.4X): %8.8X\n", GPORegPort, gpoVal);
	}

	if( LEDState )            //old:(gpoVal & 0x01 )
		gpoVal = gpoVal & ~gpoMask;		//; Clear GPOXX
	else
		gpoVal = gpoVal | gpoMask;		//; Set GPOXX
	if( debugMode )
	{
		dprintf("OutGPO Reg(%4.4X): %8.8X\n", GPORegPort, gpoVal);
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

	dprintf("GPIO Control Reg1 = %2.2X\n", gpioCtrlReg1);
	dprintf("GPIO Control Reg2 = %2.2X\n", gpioCtrlReg2);
	dprintf("GPIO Control Reg3 = %2.2X\n", gpioCtrlReg3);

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


#endif //_INCLUDE_PCI_FUNCTIONS_H_

