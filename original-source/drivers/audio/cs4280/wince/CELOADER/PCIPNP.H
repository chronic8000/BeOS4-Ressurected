//*******************************************************************
//
//    Description: 
//
//    File:  pcipnp.h
//
//    Copyright(c) 1996 Crystal Semiconductor Corp. All Rights Reserved.
//
//    Date:  02/06/97
//*******************************************************************
enum PCIPNP_ERROR
{
    PCI_BIOS_ERROR,
    PCI_BIOS_NOERROR
};

class PciPnP
{
public:
    static PciPnP *CreateInstance(unsigned short wDeviceId, unsigned short wVendorId);
    ~PciPnP();

    DWORD PhysicalAddressBA0();
    DWORD PhysicalAddressBA1();

    BOOL
        MemoryAccessEnable();
        
    PCIPNP_ERROR
        SetMemoryAccess(BOOL bState);
        
    BOOL
        BusMasterEnable();
        
    PCIPNP_ERROR
        SetBusMaster(BOOL bState);
        
    BOOL
        InterruptEnabled();
        
    BYTE
        GetSavedCmdStatusRegister();

    BYTE
        GetSavedInterruptLineRegister();

    unsigned char
        GetBusNumber();

    unsigned char
        GetDevFunctionNumber();

    PCIPNP_ERROR
        RestoreConfigSpaceRegs();
        
    WORD
        DeviceID(){return wDeviceId;}
        
    WORD
        VendorID(){return wVendorId;}

    PCIPNP_ERROR
        GetInterruptLine(DWORD *pdwInterrupt);
        
    PCIPNP_ERROR
        SetInterruptLine(DWORD dwInterrupt);

private:
    PciPnP();
    
    DWORD
        dwBaseAddress0,
        dwBaseAddress1;
    BYTE
        bStatusCommandRegister,
        bInterruptRegister;
    unsigned short
        wDeviceId,
        wVendorId;

    PCI_BUS_DEVICE_FUNCTION
        pciDevice;

    BOOL
        m_bMemoryState;
    BOOL
        m_bBusMasterEnable;
    BOOL
        m_bInterruptEnable;
};
