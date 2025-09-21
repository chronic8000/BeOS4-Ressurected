//*******************************************************************
//
//    Description: Functions to access pci bios information
//
//    File:  pcibios.cpp
//
//    Copyright(c) 1996 Crystal Semiconductor Corp. All Rights Reserved.
//
//    Date:  02/01/97
//*******************************************************************

// Definitions 

// Function Prototypes

// Include files 
#include <stdio.h>
#include "hwdefs.h"
#include "pcibios.h"
#include "malloc.h"

static DWORD Entry;


// Disable warnings due to inline assembly.
#pragma warning(disable:4704)



//*****************************************************************************
//                                                                            *
//   short has_pci_bios(void);
//   
//   RETURNS: 1 - TRUE
//            0 - FALSE
//                                                                            *
//*****************************************************************************
extern "C" short
has_pci_bios(void)
{
    unsigned char  Verify_ah, Verify_al, Verify_bh, Verify_bl;
    unsigned char  Verify_cl, Verify_cy;
    unsigned long  Verify_edx;

    //************ Determine if machine has a PCI BIOS and display result.
    //************ Use DOS int 1Ah fun 01 to test for PCI BIOS presence.
    _asm 
    {
        mov  ax, 0b101h                 // PCI presence detection code.
        int  01ah                       // Go ask.
        mov  Verify_ah, ah              // Save ah.(return code)
        mov  Verify_al, al              // Save bus info.(Hdwe mechanisms).
        mov  Verify_bh, bh              // Save major version.
        mov  Verify_bl, bl              // Save minor version.
        mov  Verify_cl, cl              // Save number of last bus in the system.
        mov  word ptr Verify_edx, dx    // Save PCI signature low word.
        _emit 0x66                      //*:
        _emit 0xc1                      //***: " ror edx, 16 "
        _emit 0xca                      //***: (inline asm can't use edx).
        _emit 0x10                      //*:
        mov  word ptr Verify_edx+2, dx  // Save PCI signature high word
        mov  al,1                       // Assume carry set, BIOS not present.
        jc   noBIOS                     // Jump if assumption true.
        mov  al,0                       // Set "BIOS-present" state indicator.
        noBIOS:
        mov  Verify_cy, al              // Save it.
    } // end _asm


    //************ Display results of determination.
    if( Verify_edx==0x20494350 && Verify_ah==0 && Verify_cy==0)
        return(1); 
    return(0);
}

//***************************************************************************
//                                                                          * 
//   short  find_pci_device(PCI_CLASS_RETURN_INFO *ri, 
//                              unsigned short device_id, 
//                              unsigned short vendor_id)
//
//   INPUTS:
//          device_id
//          vendor_id
//
//   RETURNS:
//          ri - Contains PCI Bus Number, Device Number, Function Number
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
find_pci_device(PCI_BUS_DEVICE_FUNCTION *ri, unsigned short device_id, 
                       unsigned short vendor_id)
{
    unsigned short return_value;
    __asm   
    {
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_FIND_PCI_DEVICE     // al = Find a certain pci device
        mov     cx, device_id               // cx = Device ID 
        mov     dx, vendor_id               // dx = Vendor ID
        mov     si, 0                       // The Function number is 0
        int     INTERRUPT_PCIBIOS

        push edi
        mov edi, ri
        mov     [di+0], bh   // Store the bus number
        mov     [di+1], bl   // Store device and function number
        pop edi

        pushf
        pop     return_value                // Return the carry flag
    }
    return(return_value &1);
}
    
//***************************************************************************
//                                                                          * 
//
//   short ReadConfigDWORD(BYTE bBusNum,
//                        BYTE bDevFunc,
//                        unsigned short register_number, 
//                        DWORD *value_read);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to read
//          value_read - Value read back
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
ReadConfigDWORD(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                     DWORD *value_read)
{
    short    return_value;

    __asm   
    {
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_READ_CONFIG_DWORD   // al = Read a dword of pci space
        
        mov     bh, bBusNum                 // put the bus number in bh

                                            // put the device and function 
                                            // number in bl.
        mov     bl, bDevFunc
        
        mov     di, register_number         // Put the register number in di
//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx

        pushf
        pop     return_value                // return the value of the carry 
                                            // flag
        mov     edi, value_read              // put the value of ecx in 
//        mov     word ptr [di], cx           // *value_read
//        _emit 0x66                          //*:
//        _emit 0xc1                          //***: " ror ecx, 16 "
//       _emit 0xc9                          //***: (inline asm can't use ecx).
//        _emit 0x10                          //*:
//        mov     word ptr [di+2], cx
        mov     dword ptr [edi], ecx           // *value_read

    }
    return(return_value &1);
}

//***************************************************************************
//                                                                          * 
//
//   short ReadConfigWORD (BYTE bBusNum,
//                        BYTE bDevFunc,
//                        unsigned short register_number, 
//                        unsigned short *value_read);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to read
//          value_read - Value read back
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
ReadConfigWORD(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                     unsigned short *value_read)
{
    short    return_value;

    __asm   
    {
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_READ_CONFIG_WORD    // al = Read a dword of pci space
        
        mov     bh, bBusNum                 // put the bus number in bh

                                            // put the device and function 
                                            // number in bl.
        mov     bl, bDevFunc
        
        mov     di, register_number         // Put the register number in di
//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx

        pushf
        pop     return_value                // return the value of the carry 
                                            // flag


        mov     edi, value_read              // put the value of ecx in 
        mov     word ptr [edi], cx        // *value_read
    }
    return(return_value &1);
} 

//***************************************************************************
//                                                                          * 
//
//   short ReadConfigDWORD(BYTE bBusNum,
//                        BYTE bDevFunc,
//                        unsigned short register_number, 
//                        DWORD *value_read);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to read
//          value_read - Value read back
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
ReadConfigBYTE(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                     BYTE *value_read)
{
    short    return_value;

    __asm   
    {
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_READ_CONFIG_BYTE    // al = Read a byte of pci space

        mov     bh, bBusNum                 // put the bus number in bh

                                            // put the device and function 
                                            // number in bl.
        mov     bl, bDevFunc

        mov     di, register_number         // Put the register number in di
//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx
        pushf
        pop     return_value                // return the value of the carry 
                                            // flag


        mov     edi, value_read              // put the value of ecx in 
        mov     byte ptr [edi], cl        // *value_read
    }
    return(return_value &1);
} 

//***************************************************************************
//                                                                          * 
//
//   short WriteConfigDWORD(BYTE bBusNum,
//                         BYTE bDevFunc,
//                         unsigned short register_number, 
//                         DWORD dwValue);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to write
//          dwValue - Value to write
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
WriteConfigDWORD(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                      DWORD dwValue)
{
    short    return_value;

    __asm   
    {                                        
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_WRITE_CONFIG_DWORD  // al = Read a dword of pci space
        
        mov     bh, bBusNum                 // put the bus number in bh

                                            // put the device and function 
                                            // number in bl.
        mov     bl, bDevFunc

        mov     di, register_number         // Put the register number in di

        // This is the same as:
         mov     ecx, dwValue
//        mov     cx, word ptr dwValue + 2
//        _emit 0x66                      //*:
//        _emit 0xc1                      //***: " ror ecx, 16 "
//        _emit 0xc9                      //***: (inline asm can't use ecx).
//        _emit 0x10                      //*:
//        mov     cx, word ptr dwValue 

//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx
        pushf
        pop     return_value                 // return the value of the carry 
                                             // flag
    }
    return(return_value &1);
}

//***************************************************************************
//                                                                          * 
//
//   short WriteConfigWORD(BYTE bBusNum,
//                        BYTE bDevFunc,
//                        unsigned short register_number, 
//                        unsigned short wValue);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to write
//          wValue - Value to write
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
WriteConfigWORD(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                      unsigned short wValue)
{
    short    return_value;

    __asm   
    {                                        
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_WRITE_CONFIG_WORD   // al = Read a dword of pci space
        
        mov     bh, bBusNum                 // put the bus number in bh
                                            
        mov     bl, bDevFunc                // put the device and function 
                                            // number in bl.
                                            
        mov     di, register_number         // Put the register number in di

        mov     cx, word ptr wValue         // Value to Write

//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx
        
        pushf
        pop     return_value                // return the value of the carry
                                            // flag
    }
    return(return_value &1);
}

//***************************************************************************
//                                                                          * 
//
//   short WriteConfigBYTE(BYTE bBusNum, 
//                        BYTE bDevFunc,
//                        unsigned short register_number, 
//                        BYTE bValue);
//
//   INPUTS:
//          bBusNum -  Bus Number
//          bDevFunc - Device/Function number
//          register_number - Register to write
//          bValue - Value to write
//
//   RETURNS:
//          value_read
//      
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short
WriteConfigBYTE(BYTE bBusNum, BYTE bDevFunc, unsigned short register_number, 
                      BYTE bValue)
{
    short    return_value;

    __asm   
    {                                        
        mov     ah, PCI_FUNCTION_ID         // ah = pci function id
        mov     al, PCI_WRITE_CONFIG_BYTE   // al = Read a dword of pci space
        
        mov     bh, bBusNum                 // put the bus number in bh

                                            // put the device and function 
                                            // number in bl.
        mov     bl, bDevFunc

        mov     di, register_number         // Put the register number in di

        mov     cl, byte ptr bValue         // value to write 

//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx
        
        pushf
        pop     return_value                // return the value of the carry 
                                            // flag
    }
    return(return_value &1);
}

//***************************************************************************
//                                                                          * 
//
//   short GetIrqRoutingOptions(IRQRoutingOptionsBuffer *pRouteBuffer);
//
//   INPUTS:
//          pRouteBuffer - The address of the buffer to store the PCI 
//              interrupt routing options.
//
//   RETURNS:
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" BYTE
GetIrqRoutingOptions(IRQRoutingOptionsBuffer far *pRouteBuffer)
{
    BYTE
        bReturnVal;
    unsigned short
        wFlags,
        wRoutingOptsSeg,
        wRoutingOptsOff;
        
    wRoutingOptsSeg = (unsigned short)( ( ( DWORD ) pRouteBuffer & 0xffff0000 ) >> 16 ) ;
    wRoutingOptsOff = (unsigned short)( ( DWORD ) pRouteBuffer & 0x0000ffff ) ;

    _asm
    {
        push    es
        push    ds
        push    di
        
        mov     ax, 0f000h
        mov     ds, ax
        mov     ax, wRoutingOptsOff
        mov     di,ax
        mov     ax, wRoutingOptsSeg
        mov     es,ax

        mov     ah, PCI_FUNCTION_ID             // ah = pci function id
        mov     al, GET_IRQ_ROUTING_OPTIONS     // al = get routing options        
        mov     bx, 0h
        
//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx

        pop     di
        pop     ds
        pop     es
        pushf   
        pop     wFlags      
        mov     bReturnVal, ah
    }
    return(bReturnVal);
}     

extern "C" BYTE
GetHardwareInt(BYTE bBusNum, BYTE bDevFuncNum)
{
    IRQRoutingOptionsBuffer
        irqBuffer;

    IRQRoutingTableEntry
        far *pData;
    void
        *pTemp;

    irqBuffer.wBufferSize = 0;
    irqBuffer.pData = 0;
  
    if (GetIrqRoutingOptions(&irqBuffer) != PCI_BUFFER_TOO_SMALL)
    {
        return(1);
    }

    //
    // Get Some memory
    //
    if ((pTemp = malloc(irqBuffer.wBufferSize)) == 0)
    {
        return(1);
    }

    irqBuffer.pData = (IRQRoutingTableEntry far *)pTemp;
    
    if (GetIrqRoutingOptions(&irqBuffer) != PCI_SUCCESSFUL)
    {
        return(1);
    }

    for (pData = (IRQRoutingTableEntry far *)(irqBuffer.pData);
        pData < (IRQRoutingTableEntry far *)(irqBuffer.pData) + 
        (irqBuffer.wBufferSize)/sizeof(IRQRoutingTableEntry);
        pData += 1 )
    {
        if (( bBusNum == pData->bPciBusNum ) && 
            ( bDevFuncNum == pData->bPCIDevNum))
        {
            return(pData->bLinkInt_A);
        }
    }    
    
    free(pTemp);
    
    return(0);
}    
//***************************************************************************
//                                                                          * 
//
//   short SetHardwareInterrupt(BYTE bBusNum, BYTE bDevFuncNum, BYTE bInt);
//
//   INPUTS:
//          pi - Contains PCI Bus Number, Device Number, Function Number
//          bInt - The interrupt to switch to.
//
//   RETURNS:
//          1 - Error
//          0 - No Error
//
//                                                                          *
//***************************************************************************
extern "C" short 
SetHardwareInterrupt(BYTE bBusNum, BYTE bDevFuncNum, BYTE bInt)
{
    BYTE
        bReturnVal;
    unsigned short
        wFlags;

    _asm
    {
        push    ds
        push    di
        
        mov     ax, 0f000h
        mov     ds, ax

        mov     ah, PCI_FUNCTION_ID             // ah = pci function id
        mov     al, PCI_SET_INTERRUPT           // al = set interrupt        

        mov     bh, bBusNum                     // put the bus number in bh

                                                // put the device and function
                                                // number in bl.
        mov     bl, bDevFuncNum

        mov     ch, bInt
        mov     cl, 0ah                         // INTA
        
//        int     INTERRUPT_PCIBIOS
        mov     edx, Entry
        push    cs
        call    edx

        pop     di
        pop     ds

        pushf   
        pop     wFlags      
        mov     bReturnVal, ah
    }
    return(bReturnVal);
}     


/*
 * PCIBios32BitSupport
 *
 * Scan memory for 32-bit PCI Bios support.  Function returns FALSE if support
 * was not found and TRUE if it was.  Note that this function will return FALSE
 * if multiple entry points are found to the 32-bit PCI Bios support. 
 */
int     PCIBios32BitSupport( void )
{
        DWORD   Index,
                Index2,
                Length,
                Addr,
                ServiceEntry,
                PCIBiosEntry,
                NewPCIBiosEntry ;
        BYTE    *StartAddr;
        BYTE    CheckSum ;
        
 //       X86REG  Regs ;
        int     ServiceDir,
                Support ;

        /* Setup for routine */
        ServiceDir = FALSE ;
        Support = FALSE ;
        PCIBiosEntry = 0 ;
        
        /* Display message */
        printf( "\nScanning For BIOS32 Service Directory\n" );
//        printf( "%s", Line );

        /* Scan memory for Bios32 service directory */
        for( Addr = 0xe0000, Index  = 0 ; Addr  < 0xffff0 ;
                Addr += 0x10, Index++ )  {

                /* Possibly found service directory */
                StartAddr = ( BYTE * ) Addr ;
                if( StartAddr[0] == 0x5f && StartAddr[1] == 0x33 &&
                        StartAddr[2] == 0x32 && StartAddr[3] == 0x5f )  {

                        /* Compute structure length */
                        Length =  ( DWORD ) StartAddr[9] ;
                        Length *= 16 ;

                        /* Compute checksum */
                        CheckSum = 0 ;
                        for( Index2 = 0 ; Index2 < Length ; Index2++ )
                                CheckSum += StartAddr[Index2] ;

                        /* Valid checksum */
                        if( !CheckSum )  {

                                /* Set flag */
                                ServiceDir = TRUE ;

                                /* Display message */
                                printf( "Found BIOS32 Service At Address:  %08lxh\n",
                                        StartAddr ) ;
                                printf( "Determining PCI BIOS 32-Bit Entry Address\n" ) ;
                
                                /* Determine PCI Bios 32-bit entry address */   
                                ServiceEntry = *( DWORD * )( StartAddr + 4 ) ;
                              NewPCIBiosEntry = FindPCIBios32Entry( ( DWORD * ) ServiceEntry ) ;

                                /*
                                 * Couldn't find entry, display message, and 
                                 * get next service directory
                                 */
                              if( !NewPCIBiosEntry )  {
                                      printf( "\tPCIDiag Error:  Unable To Determine Entry Address\n" ) ;
                                      printf( "Done.\n" );
                                      continue ;
                              }

                                /* Warn if we have possible multiple entry points */
                              if( PCIBiosEntry )  {
                                      if( PCIBiosEntry != NewPCIBiosEntry )  {
                                              printf( "\tPCIDiag Error:  Multiple Entry Addresses For 32-Bit PCI Bios\n" ) ;
                                              Support = FALSE ;
                                      }
                                      printf( "Done.\n" ) ;
                              }
                              else  {
                                      PCIBiosEntry = NewPCIBiosEntry ;
                                        Support = TRUE ;
                                        printf( "Done.\n" );
                              }
                        }
                //      else
                //              break ;
                }
        }
      Entry = PCIBiosEntry ;

        if( !ServiceDir )
                printf( "PCIDiag Error:  Unable To Find BIOS32 Service Directory\n" );
//        printf( "%s", Line );

        return( Support ) ;
}

/*
 * FindPCIBios32Entry
 *
 * Finds the 32-bit entry point to the PCI Bios by calling the BIOS32 Service
 * Directory with the identifier for the PCI Bios services.  Note that this
 * routine returns a physical address as the entry point.  So, if we make
 * multiple calls to this routine, then we had better get the same physical
 * address.
 */

DWORD   FindPCIBios32Entry( DWORD   *EntryPoint )
{
    DWORD   PCIEntry ;
    DWORD   PCIEntryBase ;
    DWORD   PCIEntryOffset ;
    DWORD   PCIEntryLength ;
    unsigned char return_code;
    // Setup for 32-bit call 

    _asm
    {
        pushf
        push    eax
        push    ebx
        push    ecx
        push    edx
        
        mov     eax, 0x49435024
        mov     ebx, 0

        mov     edx, EntryPoint
        push    cs
        call    edx

        mov     return_code, al
        mov     PCIEntryBase, ebx
        mov     PCIEntryLength, ecx
        mov     PCIEntryOffset, edx

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        popf
    }

        // Check return value 
        if( return_code == 0x80 || return_code == 0x81 )  {
                printf( "\tPCI BIOS 32-Bit Services are not present\n" ) ;
                return( 0 ) ;
        }

        // Compute PCI entry point and display 
        PCIEntry = PCIEntryBase + PCIEntryOffset ;
        printf( "\tPCI BIOS 32-Bit Service Entry Address:  %08lxh\n",
                PCIEntry ) ;
        printf( "\tPCI BIOS 32-Bit Service Length:         %08lxh\n",
                PCIEntryLength ) ;

        return( PCIEntry ) ;
}


/*
 * PCIBiosPresent
 */
/*
int     PCIBiosPresent( Regs )
X86REG  *Regs ;
{
        X86REG  InRegs, Pm32OutRegs ;
        int     Rv;

        InRegs.h.ah = PCI_FUNCTION_ID ;
        InRegs.h.al = PCI_BIOS_PRESENT ;
        Rv = BIOSInt( &InRegs, &Pm32OutRegs, Entry, PROT32MODE ) ;
        ValidateReg( &InRegs, &Pm32OutRegs, PCI_BIOS_PRESENT,
                PCI_BIOS_PRESENT_M, PROT32MODE ) ;

        memcpy( Regs, &Pm32OutRegs, sizeof( X86REG ) ) ;

        return( Rv ) ;
}
*/

/*
 * FindPCIDevice
 */

int FindPCIDevice(PCI_BUS_DEVICE_FUNCTION *ri, unsigned short DevId, unsigned short VenId )
{
    unsigned short return_value;

    _asm
    {
        pushf
        push    ax
        push   ebx
        push    cx
        push    dx
        push    si
        
        mov     ah, PCI_FUNCTION_ID           // 0xb1
        mov     al, PCI_FIND_PCI_DEVICE           // 0x02
        mov     cx, DevId
        mov     dx, VenId
        mov     si, 0

        mov     ebx, Entry
        push    cs
        call    ebx

        push    edi
        mov     edi, ri
        mov     byte ptr [edi+0], bh   // Store the bus number
        mov     byte ptr [edi+1], bl   // Store device and function number
        pop     edi

        pushf
        pop     return_value                // Return the carry flag

        pop    si
        pop    dx
        pop    cx
        pop   ebx
        pop    ax

    }
    return(return_value &1);
}

/*
 * ValidateReg
 *
 * Verifies that only the registers specified in the compare mask are
 * different between the two register structures passed to the function.
 * If they are not, an error message is displayed.
 */
/*
void    ValidateReg( InRegs, OutRegs, Function, CompareMask)
X86REG  *InRegs ;
X86REG  *OutRegs ;
BYTE    Function ;
DWORD   CompareMask ;
{
        BYTE    Failure ;
        DWORD   Reg ;

        // Assume that there is an unexpected register inconsistency 
        Failure = TRUE ;

        //
        // Check each register as necessary to validate that only the
        // registers contained in the compare mask have changed during the
        // call.  Mask may not be handled right, so force it.
        // 
        while( Failure )  {

                Reg = InRegs->e.eax ^ OutRegs->e.eax ;
                if( Reg )  {
                        if( !( ( CompareMask & AL_M ) ^ AL_M ) )
                                Reg &= LBYTE_M ;
                        if( !( ( CompareMask & AH_M ) ^ AH_M ) )
                                Reg &= HBYTE_M ;
                        if( !( ( CompareMask & EAX_M ) ^ EAX_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }

                Reg = InRegs->e.ebx ^ OutRegs->e.ebx ;
                if( Reg )  {
                        if( !( ( CompareMask & BL_M ) ^ BL_M ) )
                                Reg &= LBYTE_M ;
                        if( !( ( CompareMask & BH_M ) ^ BH_M ) )
                                Reg &= HBYTE_M ;
                        if( !( ( CompareMask & EBX_M ) ^ EBX_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }

                Reg = InRegs->e.ecx ^ OutRegs->e.ecx ;
                if( Reg )  {
                        if( !( ( CompareMask & CL_M ) ^ CL_M ) )
                                Reg &= LBYTE_M ;
                        if( !( ( CompareMask & CH_M ) ^ CH_M ) )
                                Reg &= HBYTE_M ;
                        if( !( ( CompareMask & ECX_M ) ^ ECX_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }

                Reg = InRegs->e.edx ^ OutRegs->e.edx ;
                if( Reg )  {
                        if( !( ( CompareMask & DL_M ) ^ DL_M ) )
                                Reg &= LBYTE_M ;
                        if( !( ( CompareMask & DH_M ) ^ DH_M ) )
                                Reg &= HBYTE_M ;
                        if( !( ( CompareMask & EDX_M ) ^ EDX_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }

                Reg = InRegs->e.esi ^ OutRegs->e.esi ;
                if( Reg )  {
                        if( !( ( CompareMask & SI_M ) ^ SI_M ) )
                                Reg &= LWORD_M ;
                        if( !( ( CompareMask & ESI_M ) ^ ESI_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }

                Reg = InRegs->e.edi ^ OutRegs->e.edi ;
                if( Reg )  {
                        if( !( ( CompareMask & DI_M ) ^ DI_M ) )
                                Reg &= LWORD_M ;
                        if( !( ( CompareMask & EDI_M ) ^ EDI_M ) )
                                Reg &= HWORD_M ;
                        if( Reg )
                                break ;
                }
                Failure = FALSE ;
        }

        //
        //Register was changed which should have remained constant so
        //display all the registers.
        //
        if( Failure )  {
                if( !( ErrorCount & SUP_VALID ) )  {

                printf( "\nPCIDiag Warning:\n\tUnexpected Register Change During " ) ;
                }
        }
}
*/



