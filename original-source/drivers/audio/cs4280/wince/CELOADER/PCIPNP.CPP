//*******************************************************************
//
//    Description: Gets physical address for clyde
//
//    File:  pcipnp.cpp
//
//    Copyright(c) 1996-1998 Crystal Semiconductor Corp. All Rights Reserved.
//
//    Date:  02/06/97
//*******************************************************************


//#include "constants.h"
#include "hwdefs.h"
#include "pcibios.h"
#include "pcipnp.h"

//
// Interrupt Line and Bridge Bits
//
#define INTERRUPT_INTA              0x00000100L
#define NULL    0

//============================================================================
// PciPnP::CreateInstance
//----------------------------------------------------------------------------
//============================================================================
PciPnP *
PciPnP::CreateInstance(unsigned short wDeviceId, unsigned short wVendorId)
{
    PciPnP *
        pInstance;
//    int rv;

    //
    // Create the new object.
    //
    if ((pInstance = new PciPnP()) == NULL)
    {
        return (NULL);
    }
    
    pInstance->wDeviceId = wDeviceId;
    pInstance->wVendorId = wVendorId;
    

        /* Check for 32-Bit Bios support */
        if( !PCIBios32BitSupport( ) )
                return( NULL ) ;

        /* Check for PCI Bios present */
//      if( PCIBiosPresent( &Regs ) )
//              return( NULL ) ;

        /* Check for error condition */
//      if( Regs.h.ah != 0x00 || Regs.e.edx != 0x020494350 )
//              return( NULL ) ;

    //
    // Look for One of the family of SoundFusion devices
    //
    if (FindPCIDevice(&(pInstance->pciDevice), wDeviceId, wVendorId))
    {
        delete pInstance;
        return (NULL);
    }
    
    //
    // Read and save base address 0
    //
    if (ReadConfigDWORD(pInstance->pciDevice.bBusNum,
        pInstance->pciDevice.bDevFuncNum, 
        PCICONFIG_BA0, &(pInstance->dwBaseAddress0)))
    {
        delete pInstance;
        return (NULL);
    }

    //
    // Read and save base address 1
    //
    if (ReadConfigDWORD(pInstance->pciDevice.bBusNum,
        pInstance->pciDevice.bDevFuncNum,
        PCICONFIG_BA1, &(pInstance->dwBaseAddress1)))
    {
        delete pInstance;
        return (NULL);
    }

    //
    // Save the curent state of the Status and Command register.
    //
    if (ReadConfigBYTE(pInstance->pciDevice.bBusNum,
        pInstance->pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, &(pInstance->bStatusCommandRegister)))
    {
        delete pInstance;
        return (NULL);
    }

    //
    // Save the current state of the Interrupt Line Register.
    //
    if (ReadConfigBYTE(pInstance->pciDevice.bBusNum,
        pInstance->pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, &(pInstance->bInterruptRegister)))
    {
        delete pInstance;
        return(FALSE);
    }
    
    return(pInstance);
}    

//============================================================================
// PciPnP::PciPnP
//----------------------------------------------------------------------------
//============================================================================
PciPnP::PciPnP()
{
}    

//============================================================================
// PciPnP::~PciPnP
//----------------------------------------------------------------------------
//============================================================================
PciPnP::~PciPnP()
{
}

//============================================================================
// PciPnP::PhysicalAddressBA0
//----------------------------------------------------------------------------
//============================================================================
DWORD
PciPnP::PhysicalAddressBA0()
{
    return(dwBaseAddress0);
}

//============================================================================
// PciPnP::GetSavedCmdStatusRegister
//----------------------------------------------------------------------------
//============================================================================
BYTE
PciPnP::GetSavedCmdStatusRegister()
{
    return(bStatusCommandRegister);
}    

//============================================================================
// PciPnP::GetInterruptLineRegister
//----------------------------------------------------------------------------
//============================================================================
BYTE
PciPnP::GetSavedInterruptLineRegister()
{
    return(bInterruptRegister);
}        
        
//============================================================================
// PciPnP::GetBusNumber
//----------------------------------------------------------------------------
//============================================================================
unsigned char
PciPnP::GetBusNumber()
{
    return(pciDevice.bBusNum);
}

//============================================================================
// PciPnP::GetDevFunctionNumber
//----------------------------------------------------------------------------
//============================================================================
unsigned char
PciPnP::GetDevFunctionNumber()
{
    return(pciDevice.bDevFuncNum);
}    

//============================================================================
// PciPnP::PhysicalAddressBA1
//----------------------------------------------------------------------------
//============================================================================
DWORD
PciPnP::PhysicalAddressBA1()
{
    return(dwBaseAddress1);
}

//============================================================================
// PciPnP::MemoryAccessEnable
//----------------------------------------------------------------------------
//============================================================================
BOOL
PciPnP::MemoryAccessEnable()
{
    BYTE
        bRegister;

    //
    // Read the base address 0
    //
    if(ReadConfigBYTE( pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, &bRegister))
    {
        return(FALSE);
    }
    
    return((bRegister & PSC_MEMORY_SPACE_ENABLE) ? TRUE:FALSE);
}

//============================================================================
// PciPnP::GetInterruptLine
//----------------------------------------------------------------------------
//============================================================================
PCIPNP_ERROR
PciPnP::GetInterruptLine(DWORD *pdwInterrupt)
{
    //
    // Read the base address 0
    //
    if (ReadConfigDWORD(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, pdwInterrupt))
    {
        return(PCI_BIOS_ERROR);
    }

    *pdwInterrupt &= PI_LINE_MASK;

    return(PCI_BIOS_NOERROR);
}
        
//============================================================================
// PciPnP::SetInterruptLine
//----------------------------------------------------------------------------
// PCI BIOS Set the PCI hardware interrupt and change config space to show
// this change to other devices.
//============================================================================
PCIPNP_ERROR
PciPnP::SetInterruptLine(DWORD dwInterrupt)
{
    DWORD
        dwRegister;   

    //
    // Call PCI BIOS Set Hardware Interrupt
    //
//    if (SetHardwareInterrupt(&pciDevice, (BYTE)dwInterrupt))
//    {
//        return(PCI_BIOS_ERROR);
//    }

    //
    // Read config space and get the current PCI Interrupt Line value
    //
    if (ReadConfigDWORD(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, &dwRegister))
    {
        return(PCI_BIOS_ERROR);
    }

    dwRegister &= ~PI_LINE_MASK;

    //
    // Set the Interrupt Line to our interrupt
    //
    dwInterrupt = (dwInterrupt & PI_LINE_MASK) | dwRegister;

    if (WriteConfigDWORD(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, dwInterrupt))
    {
        return(PCI_BIOS_ERROR);
    }

    return(PCI_BIOS_NOERROR);
}

//============================================================================
// PciPnP::InterruptEnabled
//----------------------------------------------------------------------------
// Checks to see if we have a valid interrupt configuration.
//============================================================================
BOOL
PciPnP::InterruptEnabled()
{
    DWORD
        dwRegister;   

    //
    // Read config space and get the current PCI Interrupt State
    //
    if (ReadConfigDWORD(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, &dwRegister))
    {
        return(FALSE);
    }

    //
    // If we are not configured to use INTA fail.
    //
    if ((dwRegister &  PI_PIN_MASK ) != INTERRUPT_INTA)
    {
        return(FALSE);
    }

    //
    // If we do not have an interrupt assigned to us then fail.
    //
    if ((dwRegister & PI_LINE_MASK) == 0xFF)
    {
        return(FALSE);
    }
    
    return(TRUE);
}

//============================================================================
// PciPnP::SetMemoryAccess
//----------------------------------------------------------------------------
//============================================================================
PCIPNP_ERROR
PciPnP::SetMemoryAccess(BOOL bState)
{
    BYTE
        bRegister;

    //
    // Read the Status Command register from config space.
    //
    if (ReadConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, &bRegister))
    {
        return(PCI_BIOS_ERROR);
    }

    //
    // Set the status command register accordingly.
    //
    bRegister = bState? 
                   (bRegister | (BYTE)PSC_MEMORY_SPACE_ENABLE):
                   (bRegister & (BYTE)(~PSC_MEMORY_SPACE_ENABLE));

    if (WriteConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, bRegister))
    {
        return(PCI_BIOS_ERROR);
    }
    return(PCI_BIOS_NOERROR);
}

//============================================================================
// PciPnP::BusMasterEnable
//----------------------------------------------------------------------------
//============================================================================
BOOL
PciPnP::BusMasterEnable()
{
    BYTE
        bRegister;

    //
    // Read the base address 0
    //
    if (ReadConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, &bRegister))
    {
        return(FALSE);
    }
    
    return((bRegister & PSC_BUS_MASTER_ENABLE) ?TRUE:FALSE);
}

//============================================================================
// PciPnP::SetBusMaster
//----------------------------------------------------------------------------
//============================================================================
PCIPNP_ERROR
PciPnP::SetBusMaster(BOOL bState)
{
    BYTE
        bRegister;

    //
    // Read the base address 0
    //
    if (ReadConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, &bRegister))
    {
        return(PCI_BIOS_ERROR);
    }

    //
    // Set the status command register accordingly.
    //
    bRegister = bState? 
                   (bRegister | (BYTE)PSC_BUS_MASTER_ENABLE):
                   (bRegister & (BYTE)~PSC_BUS_MASTER_ENABLE);
    
    if (WriteConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, bRegister))
    {
        return(PCI_BIOS_ERROR);
    }
    return(PCI_BIOS_NOERROR);
}

//============================================================================
// PciPnP::RestoreConfigSpaceRegs
//----------------------------------------------------------------------------
//============================================================================
PCIPNP_ERROR
PciPnP::RestoreConfigSpaceRegs()
{
    PCIPNP_ERROR 
        bFailed=PCI_BIOS_NOERROR;
    //
    // Save the curent state of the Status and Command register.
    //
    if (WriteConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_STATUS_COMMAND, bStatusCommandRegister))
    {
        bFailed = PCI_BIOS_ERROR;
    }

    //
    // Save the current state of the Interrupt Line Register.
    //
    if (WriteConfigBYTE(pciDevice.bBusNum, pciDevice.bDevFuncNum,
        PCICONFIG_INTERRUPT, bInterruptRegister))
    {
        bFailed = PCI_BIOS_ERROR;
    }
    return(bFailed);
}    
