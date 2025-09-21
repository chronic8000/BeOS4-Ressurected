/*---------------------------------------------------------------------------- 
COPYRIGHT (c) 1997 by Philips Semiconductors

 THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED AND COPIED IN 
 ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH A LICENSE AND WITH THE 
 INCLUSION OF THE THIS COPY RIGHT NOTICE. THIS SOFTWARE OR ANY OTHER COPIES 
 OF THIS SOFTWARE MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER
 PERSON. THE OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED. 
 
  THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT ANY PRIOR NOTICE
  AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY Philips Semiconductor. 
  
   PHILIPS ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF THIS SOFTWARE
   ON PLATFORMS OTHER THAN THE ONE ON WHICH THIS SOFTWARE IS FURNISHED.
----------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////////////////////////
//	HISTORY
//
//	960510	Tilakraj Roy 	  Created
//	979527  Tilakraj Roy      Ported to Windows NT with TMMan V4.0 interfaces
//	970806	Tilakraj Roy      Ported to Windows NT with TMMan V5.0 interfaces
//	982005	Volker Schildwach Ported to Windwos CE
//	981021	Tilakraj Roy	  Changes for integrating into common source base
//      990511                    Ported to pSOS
//
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Standard include files:
//



//-----------------------------------------------------------------------------
// Project include files:
//
#include "tmtypes.h"
#include "tmmanlib.h"
#include "tmmanerr.h"
#include "mmio.h"
/*
platform.h has been renamed to tm_platform.h because of naming conflict with
some BeIA internal header
*/
#include "tm_platform.h"
/*
included for:
- defines of debug printf in kernel mode (serial output)
- access to pci functions through functions similiar in syntax to their linux counterparts
*/
#include "../mydefs.h"

#include <PCI.h>	/* Be specific, access to the PCI module */

#include <errno.h>


//-----------------------------------------------------------------------------
// Types and defines:
//

#define SDRAM_LENGTH	(8 * 1024 * 1024)
#define MMIO_LENGTH	(2 * 1024 * 1024)



#define kPciVendorPhilips    (0x1131)	/* VENDOR_ID of TriMedia reference board */
#define kPciDeviceTriMedia   (0x5402)	/* DEVICE_ID of TriMedia reference board */

#define	kHalFourCC  tmmanOBJECTID ( 'H', 'A', 'L', ' ' )

typedef UInt16 (* tInterruptHandler)(UInt32 context);

/* DL moved to platform.h
typedef struct
{
GenericObject    Object;

 UInt32      TMDeviceVendorID;
 UInt32      TMSubsystemID;
 UInt32      TMClassRevisionID;
 
  UInt32      BridgeDeviceVendorID;
  UInt32      BridgeSubsystemID;
  UInt32      BridgeClassRevisionID;
  
   UInt32*     pMMIOAddrPhysical; 
   UInt32      MMIOLength;
   UInt8*      pMMIOAddrKernel;
   
    UInt32*     pSDRAMAddrPhysical;
    UInt32      SDRAMLength;
    UInt8*      pSDRAMAddrKernel;
	
	 UInt32      PCIRegisters[constTMMANPCIRegisters];
	 
	  UInt32      SelfInterrupt;     // from Target -> Host 
	  UInt32      PeerInterrupt;     // Host -> Target 
	  
	   HalInterruptHandler	Handler;
	   Pointer				Context;
	   
		HalControl* pControl;
		UInt32      SpeculativeLoadFix;
		Bool        Swapping;	
		
		 UInt32      PeerMajorVersion;
		 UInt32      PeerMinorVersion;
		 
		  UInt32      SDRAMMapCount;
		  
		   } tHalObject;
		   
*/

//-----------------------------------------------------------------------------
// Global data and external function/data references:
//
// DL: extern tPci  gPci; // PCI HW API handle


//-----------------------------------------------------------------------------
// Prototypes:
//

static void halDumpObject(UInt32 HalHandle);

UInt16 halHardwareInterruptHandler (UInt32 context);


/*
removed the static, so the function can be accessed from the core BeOS/BeIA driver
to remove the irq handler
static
*/
int32 tm_interrupt_handler(void *dev_id)
{
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    	PRINTF("==  IRQ HANDLER         ==\n");
    	PRINTF("==  CALLED              ==\n");
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    	
	PRINTF("tmhal.c: tm_interrupt_handle: halHardwareInterruptHandler is going to be called.\n");
	halHardwareInterruptHandler((UInt32)dev_id);
	PRINTF("tmhal.c: tm_interrupt_handle: halHardwareInterruptHandler has returned.\n");
	
	return B_UNHANDLED_INTERRUPT;
}

void  TMInterruptInstall(tInterruptHandler interrupt_handler, UInt32 pHal)
{
	UInt8  IrqLine;
	UInt32 Dummy;
	long    result;
	Dummy   = ((tHalObject*)pHal)->PCIRegisters[constTMMANPCIRegisters-1];
	IrqLine = (UInt8)(Dummy & (0xff));
	//IrqLine = (UInt8)(Dummy >> 24);
	PRINTF("tmhal.c: TMInterruptInstall: IrqLine is %d\n",IrqLine);
	/*
	result = request_irq(IrqLine, tm_interrupt_handler, SA_INTERRUPT | SA_SHIRQ,	
		"trimedia",(void *)pHal);
	*/
	/*
	We have the following function pointers:
		- 1 halHardwareInterruptHandler
		- 2 interrupt_handler
		- 3 tm_interrupt_handler
	Which are:	
		- 3 calls 1 and output some trace (see beginning of this file)
		- 2 is the parameter of this function (see this function definition)
	We use 3 instead of 2 because it has the right BeOs/BeIA signature.
	We check that 1 and 2 are the same. Because, otherwise, we change behaviour
	by calling 3 instead of using 2
	*/
	if ((void *)interrupt_handler != (void *)halHardwareInterruptHandler)
	{
		PRINTF("WARNING: tmhal.c/TMInterruptInstall %p != %p\n",
			interrupt_handler,
			tm_interrupt_handler);
	}
	result = install_io_interrupt_handler
		(
		IrqLine,				//interrupt_number
		tm_interrupt_handler,	//handler
		(void *)pHal,			//data
		0);						//flags
	if (result != B_OK)
    {
		PRINTF("tmhal.c: TMInterruptInstall: Can't get assigned IRQ\n");
    }
    else
    {
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    	PRINTF("==  IRQ HANDLER         ==\n");
    	PRINTF("==  INSTALLED => OK     ==\n");
  		PRINTF("== irq     = %d\n", IrqLine);  	
  		PRINTF("== handler = %p\n", tm_interrupt_handler);  	
  		PRINTF("== data    = %p\n", (void *)pHal);  	
    	PRINTF("==========================\n");
    	PRINTF("==========================\n");
    }
}







//-----------------------------------------------------------------------------
// Exported functions:
//

//-----------------------------------------------------------------------------
// FUNCTION:     halCreate:
//
// DESCRIPTION:  This function allocates and configures the Hal object.
//               It uses the PCI HW API to find the first TriMedia device
//               on the PCI bus. The object is configured using the PCI
//               device configuration space and the input parameters.
//               It initialises the TriMedia device and installs an
//               interrupt handler.
//               
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        The first access to the MMIO BIU_CTL register needs
//               to be byte swapped because of a byte swap on the PCI
//-----------------------------------------------------------------------------
//
TMStatus
halCreate ( 
		   Pointer       pParams,      //  IN:  configuration parameters
		   UInt32       *pHalHandle    // OUT:  Handle to Hal Object
		   )
{
    tHalObject    *pHal;
    UInt8          idx;
    //UInt32         dummy;
	
    //DL
    //struct pci_dev *tm_device;
    pci_info *tm_device;
    pci_info  tm_device_info;
	
    PRINTF("HalCreate has just been called.\n");
    // Create the Hal object
   	if ((pHal = (tHalObject *)objectAllocate (
        sizeof(tHalObject), 
        kHalFourCC)) == Null)
	{
		DPF(0,("tmman:halCreate:objectAllocate:FAIL\n" ));
		return statusObjectAllocFail; 
	}
	
    // Get address of TriMedia Configuration Space Header
	
    //tm_device = NULL; //DL: if you do not give this command 
	//    pci_find_device will give a segmentation fault
	
    DPF(0,("pci_find_device is about to be called.\n"));	
    tm_device = beia_tm_pci_find_device(kPciVendorPhilips,kPciDeviceTriMedia, &tm_device_info);
    if(!tm_device)
    {	
        DPF(0,("\nhalCreate Error: PciSearchDevice()"));
        return statusHalInitializationFail;
    }
	
	//DL: on the algor board PMON does not write the right interrupt line
	// therefore we do it ourselves:
	
	/*
    beia_tm_pci_read_config_dword(tm_device, (constTMMANPCIRegisters -1) * (sizeof(UInt32)), &dummy);
    dummy = dummy | P4032_P5064INT_PCI3;
    beia_tm_pci_write_config_dword(tm_device, (constTMMANPCIRegisters -1) * (sizeof(UInt32)), dummy);
	*/
	PRINTF("tmhal.c/halCreate : lines commented out because P4032_P5064INT_PCI3 is not defined\n");
	
    // Store the PCI Configuration Space Header registers (16)
    for (idx = 0; idx < constTMMANPCIRegisters; idx++)
    {
		beia_tm_pci_read_config_dword( tm_device
			,(idx * sizeof(UInt32))
			, &(pHal->PCIRegisters[idx])  );
    }
	
	
	
	
    // Store the Class Revision Id
    /*
    pci_read_config_dword( tm_device
		, PCI_CLASS_REVISION
		, &(pHal->TMClassRevisionID) );
	*/
	pHal->TMClassRevisionID = tm_device->revision;
    DPF(0,("pci_read_config_dword, PCI_CLASS_REVISION has returned.\n"));	
	
    // Store the (Subsys Id,Subsys Vendor Id)
    /*
    pci_read_config_dword( tm_device
		, PCI_SUBSYSTEM_VENDOR_ID
		, &(pHal->TMSubsystemID));
	*/
	pHal->TMSubsystemID = tm_device->u.h0.subsystem_vendor_id;
    DPF(0,("pci_read_config_dword, PCI_SUBSYSTEM_VENDOR_ID has returned.\n"));	
	
    // Store the Device Vendor Id
    /*
    beia_tm_pci_read_config_dword(tm_device,PCI_VENDOR_ID,&(pHal->TMDeviceVendorID) );
    */
    pHal->TMDeviceVendorID = tm_device->vendor_id;
	
    DPF(0,("pci_read_config_dword, PCI_VENDOR_ID has returned.\n"));	
	// Initialise Bridge info (not used in this release)
    pHal->BridgeDeviceVendorID  = 0;
    pHal->BridgeSubsystemID     = 0;
    pHal->BridgeClassRevisionID = 0;
	
    // Store SDRAM PCI Address
    /*
    pci_read_config_dword(tm_device, PCI_BASE_ADDRESS_0, &(pHal->pSDRAMAddrPhysical) );
    */
    pHal->pSDRAMAddrPhysical = (void *)tm_device->u.h0.base_registers[0];
    DPF(0,("pci_read_config_dword, PCI_BASE_ADDRESS_0 has returned and pHal->pSDRAMAddrPhysical is 0x%p.\n"
		,  pHal->pSDRAMAddrPhysical));	
    // Store MMIO PCI Address
    /*
    pci_read_config_dword(tm_device,PCI_BASE_ADDRESS_1,&(pHal->pMMIOAddrPhysical)  );
    */
    pHal->pMMIOAddrPhysical = (void *)tm_device->u.h0.base_registers[1];
    DPF(0,("pci_read_config_dword, PCI_BASE_ADDRESS_1 has returned and pHal->pMMIOAddrPhysical is 0x%p.\n"
		, pHal->pMMIOAddrPhysical));	
	
    // Store length of MMIO area & SDRAM area
    pHal->MMIOLength  = MMIO_LENGTH;
    pHal->SDRAMLength = SDRAM_LENGTH;
	
    // Get the SDRAM MIPS to PCI address mapping
	
    DPF(0,("tmhal.c: halCreate: ioremap_nocache is about to be called\n"));
    /* delete_area to be added*/
    PRINTF("==========00\n");
    PRINTF("call to 'map_physical_memory(%s,%p,0x%lx,%x,%x,%p)'\n",
    	"tm_sdram",
    	pHal->pSDRAMAddrPhysical,
    	pHal->SDRAMLength,
    	B_ANY_KERNEL_ADDRESS,
    	B_READ_AREA|B_WRITE_AREA,
    	(void **)&pHal->pSDRAMAddrKernel
    	);
    map_physical_memory
    	(
    	"tm_sdram",
    	pHal->pSDRAMAddrPhysical,
    	pHal->SDRAMLength,
    	B_ANY_KERNEL_ADDRESS,
    	B_READ_AREA|B_WRITE_AREA,
    	(void **)&pHal->pSDRAMAddrKernel
    	);
    PRINTF("result = %p\n", pHal->pSDRAMAddrKernel);
    PRINTF("==========01\n");
    /*
    pHal->pSDRAMAddrKernel = (UInt32 *) ioremap_nocache(pHal->pSDRAMAddrPhysical,
		pHal->SDRAMLength);
	*/
    PRINTF("tmhal.c: halCreate: ioremap_nocache has returned\n");	
    if (!pHal->pSDRAMAddrKernel)
    {
        DPF(0,("\nhalCreate Error: PciGetDeviceMapping() = "));
        return statusHalInitializationFail;
    }
	
    DPF(0,("tmhal.c: halCreate: pHal->pSDRAMKernel is 0x%p\n.",
		pHal->pSDRAMAddrKernel));
	
    // Get the MMIO MIPS to PCI address mapping
    /* delete_area to be added*/
    PRINTF("==========10\n");
    PRINTF("call to 'map_physical_memory(%s,%p,0x%lx,%x,%x,%p)'\n",
    	"tm_mmio",
    	pHal->pMMIOAddrPhysical,
    	pHal->MMIOLength,
    	B_ANY_KERNEL_ADDRESS,
    	B_READ_AREA|B_WRITE_AREA,
    	(void **)&pHal->pMMIOAddrKernel
    	);
    map_physical_memory
    	(
    	"tm_mmio",
    	pHal->pMMIOAddrPhysical,
    	pHal->MMIOLength,
    	B_ANY_KERNEL_ADDRESS,
    	B_READ_AREA|B_WRITE_AREA,
    	(void **)&pHal->pMMIOAddrKernel
    	);
    PRINTF("result = %p\n", pHal->pMMIOAddrKernel);
    PRINTF("==========11\n");
    /*
    pHal->pMMIOAddrKernel = (UInt32 *) ioremap_nocache(pHal->pMMIOAddrPhysical,
		pHal->MMIOLength);
	*/
    if (!pHal->pMMIOAddrKernel)
    {
		DPF(0,("\nhalCreate Error: PciGetDeviceMapping() = "));
		return statusHalInitializationFail;
    }
    DPF(0,("tmhal.c: halCreate: pHal->pMMIOAddrKernel is 0x%p\n."
		, pHal->pMMIOAddrKernel));
	
	
    // Store the interrupt values
    pHal->SelfInterrupt = ((halParameters *)pParams)->PCIIrq;    // is  0 see tmmanlib.h
    pHal->PeerInterrupt = ((halParameters *)pParams)->TMIrq;     // is 28 see tmmanlib.h
	
    // Store the Hal Control param
    pHal->pControl           = (HalControl *)(((halParameters *)pParams)->SharedData);
    pHal->Swapping           = False; // TriMedia little endian, MIPS big endian
    pHal->Handler            = Null;
    pHal->Context            = Null;
    pHal->SpeculativeLoadFix = ((halParameters *)pParams)->SpeculativeLoadFix;
	
    // Initialise the TriMedia BIU Control Register
	
    
    /*
    cd: removed tempo
    DPF(8, ("tmhal.c: halCreate: before writing 0 BIU_CTL: 0x%lx\n", *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)));  
	*/
	
    /*if (( *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) & 
	(constTMManBIU_CTL_SE | constTMManBIU_CTL_BO | 
	constTMManBIU_CTL_HE) ) == 0x0 )
    */
	
	/* cd: set to 0 to go in KNOWN STATE */
	if (0x0 != *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL))
	{
	    *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) = 0x0;
	    spin(100);
	}
    
    
    /*
    cd: removed tempo
    DPF(8, ("tmhal.c: halCreate: after writing 0 BIU_CTL: 0x%lx\n"
		, *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)));
	*/
    if (1)
    {
		UInt32  SwappedBIUControl;
		UInt8   TempByte;
		
        SwappedBIUControl = ( constTMManBIU_CTL_SE | constTMManBIU_CTL_BO | 
			constTMManBIU_CTL_HE | constTMManBIU_CTL_SR );
        
        // do a dword swap (TriMedia has already started in Big Endian)
		TempByte = ((UInt8 *)&SwappedBIUControl)[0];
        ((UInt8 *)&SwappedBIUControl)[0] = ((UInt8 *)&SwappedBIUControl)[3];
        ((UInt8 *)&SwappedBIUControl)[3] = TempByte;
        TempByte = ((UInt8 *)&SwappedBIUControl)[1];
        ((UInt8 *)&SwappedBIUControl)[1] = ((UInt8 *)&SwappedBIUControl)[2];
        ((UInt8 *)&SwappedBIUControl)[2] = TempByte;
        // Set the desired BIU control.
        *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) = SwappedBIUControl;
        spin(100);
    }
    /*
    cd: removed tempo
    DPF(8, ("tmhal.c: halCreate: after BIU_CTL: 0x%lx\n"
		, *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)));
	*/
	
    
//?????SHOULD BE COMMENTED OUT (DON'T APPEAR ON WINDOWS PCI CAPTURE)
//seems to be to enable burst mode ???
/*
    dummy = (*(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL))  & (~constTMManBIU_CTL_BO);
    *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) = dummy;
	
    DPF(8, ("tmhal.c: halCreate: after BIU_CTL: 0x%lx and SwappedBIUControl is 0x%08lx\n"
		, *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)
		, dummy ));
	
    dummy = ( constTMManBIU_CTL_SE | constTMManBIU_CTL_BO | 
		constTMManBIU_CTL_HE | constTMManBIU_CTL_SR );
	
    *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) = dummy;
	
    DPF(8, ("tmhal.c: halCreate: after BIU_CTL: 0x%lx and SwappedBIUControl is 0x%08lx\n"
		, *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)
		, dummy ));
*/
//?????SHOULD BE COMMENTED OUT (DON'T APPEAR ON WINDOWS PCI CAPTURE)
	
    // Set the SDRAM size
    *(UInt32 *)(pHal->pMMIOAddrKernel + DRAM_LIMIT) = 
        (UInt32)(pHal->pSDRAMAddrPhysical + pHal->SDRAMLength);
	
    // Set the cacheable size
    *(UInt32 *)(pHal->pMMIOAddrKernel + DRAM_CACHEABLE_LIMIT) = 
        (UInt32)(pHal->pSDRAMAddrPhysical + pHal->SDRAMLength);
	
	
    *(UInt32 *)(pHal->pMMIOAddrKernel + ICLEAR) = (UInt32)(0x0);
    *(UInt32 *)(pHal->pMMIOAddrKernel + IMASK)  = (UInt32)(~0x0);
	
    // Register the interrupt handler
    TMInterruptInstall( halHardwareInterruptHandler, (UInt32)pHal);
	
    *pHalHandle = (UInt32)pHal;
	
    halDumpObject(*pHalHandle);
	
    return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halDestroy:
//
// DESCRIPTION:  This function deletes the Hal object.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halDestroy ( 
			UInt32 HalHandle    // IN:  Handle to Hal Object
			)
{
	tHalObject*     pHal = (tHalObject*)HalHandle;
	
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halDestroy:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
	
	objectFree ( pHal );
	
	return statusSuccess;;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halReset:
//
// DESCRIPTION:  This function resets the interrupt Mutex variables
//
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halReset (
		  UInt32	HalHandle     // IN:  Handle to Hal Object
		  )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halReset:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	pHal->pControl->HostInterruptSpinLock = 
		halAccess32 ( HalHandle, False );
    
	pHal->pControl->TargetInterruptSpinLock = 
		halAccess32 ( HalHandle, False );
    
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halSetPeerVersion:
//
// DESCRIPTION:  This function stores the given Peer version number
//
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus	halSetPeerVersion (
							   UInt32  HalHandle,    // IN: Handle to Hal Object
							   UInt32  MajorVersion, // IN: Peer major version number
							   UInt32  MinorVersion  // IN: Peer minor version number
							   )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halSetPeerVersion:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	pHal->PeerMajorVersion = MajorVersion;
	pHal->PeerMinorVersion = MinorVersion;
    
	return statusSuccess;
}



//-----------------------------------------------------------------------------
// FUNCTION:     halGetPeerVersion:
//
// DESCRIPTION:  The function returns the stored Peer version
//
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGetPeerVersion (
				   UInt32	HalHandle,     //  IN:  Handle to Hal Object
				   UInt32*	pMajorVersion, // OUT:  Major version 
				   UInt32*	pMinorVersion  // OUT:  Minor version
				   )
{
	tHalObject*   pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGetPeerVersion:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	*pMajorVersion = pHal->PeerMajorVersion;
	*pMinorVersion = pHal->PeerMinorVersion;
    
	return statusSuccess;
}

//-----------------------------------------------------------------------------
// FUNCTION:     halStartDSP:
//
// DESCRIPTION:  This function starts the TriMedia. It takes clears
//               any pending interrupts and takes the TriMedia out of
//               reset.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halStartDSP (
			 UInt32	HalHandle     // IN:  Handle to Hal Object
			 )
{
	tHalObject  *pHal = (tHalObject*)HalHandle;
	UInt32       dummy;
	UInt32      *buffer;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halStartDSP:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	// IF the PCI aperture disable is required
	if ( pHal->SpeculativeLoadFix )
    {
		/* disable the PCI apperture */
		*(UInt32 *)(pHal->pMMIOAddrKernel + DC_LOCK_CTL) = 
		/* read the value of DC_LOCK_CTL - retain all bits 
		* except bits 5 & 6 */
		(((*(UInt32 *)(pHal->pMMIOAddrKernel + DC_LOCK_CTL)) & 
		(~constTMManDC_LOCK_CTL_MASK)) | 
		/* or it with the new values of bits 5 & 6 */
		(constTMManDC_LOCK_CTL_MASK  & 
		((constTMManDC_LOCK_CTL_PDS) << constTMManDC_LOCK_CTL_POSITION)));
    }
	// ELSE
	else
    {
		/* enable the PCI apperture */
		*(UInt32 *)(pHal->pMMIOAddrKernel + DC_LOCK_CTL) = 
		/* read the value of DC_LOCK_CTL - 
		* retain all bits except bits 5 & 6 */
		(((*(UInt32 *)(pHal->pMMIOAddrKernel + DC_LOCK_CTL)) & 
		(~constTMManDC_LOCK_CTL_MASK)) | 
		/* OR it with the new values of bits 5 & 6 */
		(constTMManDC_LOCK_CTL_MASK  & 
		((constTMManDC_LOCK_CTL_HDS) << constTMManDC_LOCK_CTL_POSITION)));
    }
	// ENDIF
	
	// clear the IMask & IClear register 
	*(UInt32*)(pHal->pMMIOAddrKernel + ICLEAR) = (UInt32)(0x0);
	*(UInt32*)(pHal->pMMIOAddrKernel + IMASK)  = (UInt32)(~0x0);
	
	DPF(8, ("tmhal.c: halStartDSP: before DRAM_BASE: 0x%lx and MMIO_BASE is 0x%lx\n", *(UInt32*)(pHal->pMMIOAddrKernel + 0x100000), *(UInt32*)(pHal->pMMIOAddrKernel + 0x100400)));
	
	
	DPF(8, ("tmhal.c: halStartDSP: before BIU_CTL: 0x%lx and ~constTMManBIU_CTL_SR is 0x%lx and constTMManBIU_CTL_CR is 0x%x\n", *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL), (~((UInt32)constTMManBIU_CTL_SR)), constTMManBIU_CTL_CR));
	
	
	dummy = ( *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) ) 
		& (~((UInt32)constTMManBIU_CTL_SR));
	
	// Clear Reset
	*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) = dummy;
	
	DPF(8, ("tmhal.c: halStartDSP: in between BIU_CTL: 0x%lx and dummy is 0x%lx\n", *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL), dummy));
	
	
	dummy = ( *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) ) 
		| ((UInt32)constTMManBIU_CTL_CR) ;
	
	*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) = dummy;
	
	//  *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) |= ((UInt32)constTMManBIU_CTL_CR);
	
	DPF(8, ("tmhal.c: halStartDSP: after BIU_CTL: 0x%lx and dummy is 0x%lx\n", *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL),dummy));
	
	spin(1000);
	
	DPF(0,("pHal->MMIOAddrKernel is 0x%p\n",(pHal->pMMIOAddrKernel) ));
	buffer = (UInt32 *)(pHal->pMMIOAddrKernel);
	DPF(0,("buffer1 is 0x%p\n", buffer ));
	buffer = buffer + 0x103038;   // INT_CTL
	DPF(0,("buffer2 is 0x%p\n", buffer ));
	DPF(0,("tmhal.c: halStartDSP: before buffer is 0x%p and is 0x%p\n"
        , buffer,(UInt32*)(pHal->pMMIOAddrKernel + 0x103038) ));
	//dummy = *buffer;
	dummy = ( *(UInt32*)(pHal->pMMIOAddrKernel + 0x103038) );
	DPF(0,("tmhal.c: halStartDSP: before INT_CTL: 0x%lx\n", dummy));
	dummy = dummy | 0x16L;
	//*buffer = dummy; 
	//*(UInt32 *)(pHal->pMMIOAddrKernel + 0x103038) = dummy;
	dummy = dummy | 0x1L;
	//*(UInt32 *)(pHal->pMMIOAddrKernel + 0x103038) = dummy;   
	DPF(0,("tmhal.c: halStartDSP: after INT_CTL: 0x%lx\n"
        ,*(UInt32 *)(pHal->pMMIOAddrKernel + 0x103038)));
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halStopDSP:
//
// DESCRIPTION:  This function puts the TriMedia into the reset state.
//               It also resets the peripherals.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
			 // RETURN:       TMStatus:  always returns success
			 //
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halStopDSP (
			UInt32	HalHandle     // IN:  Handle to Hal Object
			)
{
	tHalObject*   pHal = (tHalObject*)HalHandle;
	UInt32        Idx;
	UInt32        dummy;
    
	char * buffer;
	
	buffer = (char *)(pHal->pSDRAMAddrKernel);
	
	DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPStop\n"));
	DPF(0,("tmif.c: buffer is 0x%x, 0x%x 0x%x,",buffer[0], buffer[1],buffer[2]));
	
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halStopDSP:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	// biu reset BIU_CTL
	dummy = *(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL);
	DPF(0,("tmman: halStopDSP: before BIU_CTL is 0x%lx", dummy)); 
	dummy = dummy & (~constTMManBIU_CTL_CR);
	dummy = dummy | constTMManBIU_CTL_SR;
	*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) = dummy;
	DPF(0,("tmman: halStopDSP: after BIU_CTL is 0x%lx",*(UInt32 *)(pHal->pMMIOAddrKernel + BIU_CTL) )); 
	/*DL  *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) &= (~constTMManBIU_CTL_CR);
	*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) |= constTMManBIU_CTL_SR;
	*/
	
	// reset the peripherals
	// audio out AO_CTL
	*(UInt32*)( pHal->pMMIOAddrKernel + AO_CTL ) = 0x08000000;
    
	// audio in AI_CTL
	*(UInt32*)( pHal->pMMIOAddrKernel +AI_CTL ) = 0x08000000;
    
	// video in VI_CTL
	*(UInt32*)( pHal->pMMIOAddrKernel +VI_CTL ) = 0x08000000;
    
	// video out VO_CTL
	*(UInt32*)( pHal->pMMIOAddrKernel +VO_CTL ) = 0x08000000;
    
	//ssi SSI_CTL
	*(UInt32*)( pHal->pMMIOAddrKernel +SSI_CTL ) = 0xc0000000;
    
	//icp ICP_SR
	for ( Idx= 0 ; Idx < 10 ; Idx ++ )
    {
		// do it 10 times just to make sure ....
		if ( *( pHal->pMMIOAddrKernel + ICP_SR ) & 0x01 ) 
			break;
		// changed on usman's request TRC970225
		*( pHal->pMMIOAddrKernel + ICP_SR )  = 0x80;
    }
    
	// disable 
	dummy = *(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) & ~(0x00000001);
	*(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) = dummy;
	PRINTF("tmhal.c/line 748 : added a probably missing = dummy");
	/*DL    *(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) &= ~(0x00000001); */
	//enable
	dummy = *(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) | 0x00000001;
	*(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) = dummy;
	/*DL *(UInt32*)( pHal->pMMIOAddrKernel + IIC_CTL ) |= 0x00000001; */
    
	// reset the VLD once
	*(UInt32*)( pHal->pMMIOAddrKernel + VLD_COMMAND ) = 0x00000401;
    
    
	//jtag JTAG_CTL
	//*((PDWORD)(this->MMIO.pSpace + 0x103808 )) = 0x0;
	
	DPF(8, ("tmhal.c: halStopDSP: BIU_CTL: 0x%lx\n"
		, *(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL)));
	
	
	//DL: We are also going to reset the SDRAM memory ????????
	// this piece of code probably belongs somewhere else 
	for(Idx=0;Idx<0x200000;Idx++)
    {
		((UInt32*)(pHal->pSDRAMAddrKernel))[Idx]=0x0;
    }
	
	return statusSuccess;
	
}


//-----------------------------------------------------------------------------
// FUNCTION:     halResetDSP:
//
// DESCRIPTION:  This function resets the TriMedia by copying the
//               stored PCI registers back in to the PCI 
//               configuration space. It also reinitialises the BIU_CTL
//               if necessary.
//               
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
			// RETURN:       TMStatus:  always returns success
			//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halResetDSP ( 
			 UInt32  HalHandle     // IN:  Handle to Hal Object
			 )
{
	tHalObject   *pHal;
	UInt8         idx;
	
	/*
	struct pci_dev *tm_device;
	*/
    pci_info *tm_device;
    pci_info  tm_device_info;
	
	pHal = (tHalObject*)HalHandle;
	
	// Get address of TriMedia Configuration Space Header
	
	tm_device = beia_tm_pci_find_device(kPciVendorPhilips,kPciDeviceTriMedia,&tm_device_info);
	if(!tm_device)
    {
		DPF(0,("\nhalResetDSP Error: PciSearchDevice()"));
		return statusHalInitializationFail;
    }
	
	// we write the command status at the very end.
	for (idx = 3; idx < (constTMMANPCIRegisters); idx++)
    {
		beia_tm_pci_write_config_dword( tm_device
			,(idx * sizeof(UInt32))
			, (pHal->PCIRegisters[idx])  );
    }
	
	// Write the command status register 
	beia_tm_pci_write_config_dword( tm_device
		,(1 * sizeof(UInt32))
		, (pHal->PCIRegisters[1])  );
	
	
	// IF the BIU_CTL register is not initialised
	if ((*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) &
		(constTMManBIU_CTL_SE | constTMManBIU_CTL_BO | constTMManBIU_CTL_HE) ) == 0x0 )
    {
		// virgin biu control
		UInt32	SwappedBIUControl;
		UInt8	TempByte;
		
		SwappedBIUControl = 
			( constTMManBIU_CTL_SE | constTMManBIU_CTL_BO | constTMManBIU_CTL_HE | 
			constTMManBIU_CTL_SR );
		
		// do a dword swap
		TempByte = ((UInt8*)&SwappedBIUControl)[0];
		((UInt8*)&SwappedBIUControl)[0] = ((UInt8*)&SwappedBIUControl)[3];
		((UInt8*)&SwappedBIUControl)[3] = TempByte;
		
		TempByte = ((UInt8*)&SwappedBIUControl)[1];
		((UInt8*)&SwappedBIUControl)[1] = ((UInt8*)&SwappedBIUControl)[2];
		((UInt8*)&SwappedBIUControl)[2] = TempByte;
		*(UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL) = SwappedBIUControl;
    }
	// ENDIF
	
	// set the cache details every time this function is called
	*(UInt32*)(pHal->pMMIOAddrKernel + DRAM_LIMIT) = 
		(UInt32)(pHal->pSDRAMAddrPhysical) + pHal->SDRAMLength; 
	*(UInt32*)(pHal->pMMIOAddrKernel + DRAM_CACHEABLE_LIMIT) = 
		(UInt32)(pHal->pSDRAMAddrPhysical) + pHal->SDRAMLength;
	
	*(UInt32*)(pHal->pMMIOAddrKernel + ICLEAR) = (UInt32)(0x0);
	*(UInt32*)(pHal->pMMIOAddrKernel + IMASK) = (UInt32)(~0x0);
	
	DPF(0,("\nReset DSP complete\n"));
	
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halGetMMIOInfo:
//
// DESCRIPTION:  This function returns the physical and mapped addresses
//               of the TriMedia MMIO registers space.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGetMMIOInfo (
				UInt32	HalHandle,           //  IN:  Handle to Hal Object
				Pointer *pMMIOPhysical,      // OUT:  PCI address of MMIO space
				Pointer *pMMIOKernelMapped,  // OUT:  Host mapped address of MMIO space
				UInt32  *pMMIOSize           // OUT:  Size of MMIO space
				)
{
	tHalObject* pHal = (tHalObject*)HalHandle;
	
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGetMMIOInfo:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
	
	*pMMIOPhysical     = pHal->pMMIOAddrPhysical;
	*pMMIOKernelMapped = pHal->pMMIOAddrKernel;
	*pMMIOSize         = pHal->MMIOLength;
	
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halGetSDRAMInfo:
//
// DESCRIPTION:  This function returns the physical and mapped addresses
//               of the TriMedia SDRAM.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGetSDRAMInfo ( 
				 UInt32	HalHandle,           //  IN:  Handle to Hal Object
				 Pointer *pSDRAMPhysical,     // OUT:  PCI address of start of SDRAM
				 Pointer *pSDRAMKernelMapped, // OUT:  Host mapped address of SDRAM
				 UInt32  *pSDRAMSize          // OUT:  Size of SDRAM
				 )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
	
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGetSDRAMInfo:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
	
	*pSDRAMPhysical     = pHal->pSDRAMAddrPhysical;
	*pSDRAMKernelMapped = pHal->pSDRAMAddrKernel;
	*pSDRAMSize         = pHal->SDRAMLength;
	
	return statusSuccess;
}

//-----------------------------------------------------------------------------
// FUNCTION:     halGetTMPCIInfo:
//
// DESCRIPTION:  This function returns the DeviceVendor Id, Subsystem Id
//               & Class Revision Id registers of the stored PCI configuration
//               space.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGetTMPCIInfo ( 
				 UInt32	HalHandle,         //  IN:  Handle to Hal Object
				 UInt32* pDeviceVendorID,  // OUT:  Device & Vendor Id register
				 UInt32* pSubsystemID,     // OUT:  Subsystem Id register
				 UInt32* pClassRevisionID  // OUT:  Class revision Id register
				 )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGetTMPCIInfo:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	*pDeviceVendorID  = pHal->TMDeviceVendorID;
	*pSubsystemID     = pHal->TMSubsystemID;
	*pClassRevisionID = pHal->TMClassRevisionID;
    
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halGetBridgePCIInfo:
//
// DESCRIPTION:  This function returns the DeviceVendor Id, Subsystem Id
//               & Class Revision Id registers of the stored Bridge PCI 
//               configuration space.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus	halGetBridgePCIInfo ( 
								 UInt32  HalHandle,       //  IN:  Handle to Hal Object
								 UInt32* DeviceVendorID,  // OUT:  Bridge Device & Vendor Id register
								 UInt32* SubsystemID,     // OUT:  Bridge Subsystem Id register
								 UInt32* ClassRevisionID  // OUT:  Bridge Class revision Id register
								 )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGetBridgePCIInfo:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	*DeviceVendorID  = pHal->BridgeDeviceVendorID;
	*SubsystemID     = pHal->BridgeSubsystemID;
	*ClassRevisionID = pHal->BridgeClassRevisionID;
    
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halInstallHandler:
//
// DESCRIPTION:  This function stores the given interrupt handler. This will
//               then be called from the Hal Hardware interrupt handler.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halInstallHandler ( 
				   UInt32	             HalHandle,    // IN:  Handle to Hal Object
				   HalInterruptHandler  Handler,      // IN:  Handler function
				   Pointer              pContext      // IN:  Context for handler
				   )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
    
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halInstallHandler:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	pHal->Handler = Handler;
	pHal->Context = pContext;
	
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halRemoveHandler:
//
// DESCRIPTION:  This function deletes the currently installed handler.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halRemoveHandler ( 
				  UInt32	HalHandle     // IN:  Handle to Hal Object
				  )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halRemoveHandler:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	pHal->Handler = NULL;
	return statusSuccess;
}


//-----------------------------------------------------------------------------
//  Unused
TMStatus	halDisableInterrupts ( 
								  UInt32	HalHandle,
								  UInt32* Saved );

TMStatus	halRestoreInterrupts ( 
								  UInt32	HalHandle,
								  UInt32* Saved );
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// FUNCTION:     halDisableIRQ:
//
// DESCRIPTION:  This function clears the appropriate interrupt enable bit 
//               of the INT_CTL MMIO register.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halDisableIRQ ( 
			   UInt32	HalHandle     // IN:  Handle to Hal Object
			   )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
	UInt32      InterruptControl;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halDisableIRQ:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
	InterruptControl = *(pHal->pMMIOAddrKernel + INT_CTL);
    
	// here SelfInterrupt is set to PCI Interrupt A (value 0)
	// INT#A = 0, INT#B = 1, INT#C = 2, INT#D = 3
	// disable interrupts from the target by INT_CTL->IE off.
	InterruptControl &= ~( 0x10 << ( pHal->SelfInterrupt ) ); //????
    
	*(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL) = InterruptControl;
    
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halEnableIRQ:
//
// DESCRIPTION:  This function sets the appropriate interrupt enable bit 
//               of the INT_CTL MMIO register.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halEnableIRQ ( 
			  UInt32	HalHandle     // IN:  Handle to Hal Object
			  )
{
	tHalObject*   pHal = (tHalObject*)HalHandle;
	UInt32        InterruptControl;
    
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halEnableIRQ:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
	InterruptControl = *(pHal->pMMIOAddrKernel + INT_CTL);
	InterruptControl |= ( 0x10 << ( pHal->SelfInterrupt ) );
	*(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL) = InterruptControl;
    
	return statusSuccess;
}





//-----------------------------------------------------------------------------
// FUNCTION:     halGenerateInterrupt:
//
// DESCRIPTION:  This function sets the Target interrupt bit in the 
//               IPENDING MMIO register thereby triggering an interrupt
//               on the TriMedia.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGenerateInterrupt ( 
					  UInt32	HalHandle     // IN:  Handle to Hal Object
					  )
{
	tHalObject* pHal = (tHalObject*)HalHandle;
	
	
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmman:halGenerateInterrupt:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	DPF(0,("tmman:halGenerateInterrupt1:IPENDING[%lx]:ICLEAR[%lx]:IMASK[%lx]\n",
		*(UInt32*)(pHal->pMMIOAddrKernel + IPENDING),
		*(UInt32*)(pHal->pMMIOAddrKernel + ICLEAR),
		*(UInt32*)(pHal->pMMIOAddrKernel + IMASK ) ));	
    
	
	*(UInt32 *)(pHal->pMMIOAddrKernel + IPENDING) = ( 1<< pHal->PeerInterrupt); 
	
    
	DPF(0,("tmman:halGenerateInterrupt2:IPENDING[%lx]:ICLEAR[%lx]:IMASK[%lx]\n",
		*(UInt32*)(pHal->pMMIOAddrKernel + IPENDING),
		*(UInt32*)(pHal->pMMIOAddrKernel + ICLEAR),
		*(UInt32*)(pHal->pMMIOAddrKernel + IMASK ) ));	
    
	
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halAcknowledgeInterrupt:
//
// DESCRIPTION:  This function clears the interrupt bit in the INT_CTL
//               which then clears the PCI interrupt.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus	halAcknowledgeInterrupt (
									 UInt32	HalHandle     // IN:  Handle to Hal Object
									 )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
	UInt32       InterruptControl;
    
	DPF(0,("tmhal.c: halAcknowlegdgeInterrupt: begin of function.\n"));
	if ( objectValidate ( pHal, kHalFourCC ) != True )
    {
		DPF(0,("tmhal.c: :halAcknowledgeInterrupt:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
    }
    
	/* FOR TM1 */
	/*
    reset	MMIO->dwInterruptControl:INT(3..0)
    reset	MMIO->dwInterruptControl:IE(7..4)
	*/
	
	while ( 1 )
    {
		if ( halAccess32 ( HalHandle, 
			pHal->pControl->TargetInterruptSpinLock  ) == False )
        {
			pHal->pControl->HostInterruptSpinLock = 
				halAccess32 ( HalHandle, True );
            
			if ( halAccess32 ( HalHandle, 
				pHal->pControl->TargetInterruptSpinLock  ) == True )
            {
				DPF(0,("tmhal.c: halAck: TargetInterruptSpinLock is True.\n"));
				pHal->pControl->HostInterruptSpinLock = 
					halAccess32 ( HalHandle, False  );			
            }
			else
            {
				DPF(0,("tmhal.c: halAck: TargetInterruptSpinLock is False.\n"));
				InterruptControl = *(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL);
				// here SelfInterrupt indicates PCI Interrupt A
				// INT#A = 0, INT#B = 1, INT#C = 2, INT#D = 3
				
				DPF(0,("tmhal.c: halAck: InterruptControl before is 0x%lx\n"
                    , InterruptControl)); 
				InterruptControl &=
					( ~( 1 << ( pHal->SelfInterrupt ) )  &
					~( 0x10 << ( pHal->SelfInterrupt ) ) );
				/*DL: pHal->SelfInterrupt has value 0*/
				DPF(0,("tmhal.c: halAck: InterruptControl after is 0x%lx\n"
					, InterruptControl));
				*(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL) = InterruptControl;
				
				pHal->pControl->HostInterruptSpinLock = 
					halAccess32 ( HalHandle, False  );
				break;
            }
            
        }
    }
    
	return statusSuccess;
}



//-----------------------------------------------------------------------------
// FUNCTION:     halHardwareInterruptHandler:
//
// DESCRIPTION:  This function is called when the PCI INT#A interrupt occurs.
//               It clears the interrupt and calls the installed Hal interrupt
//               handler
//
// RETURN:       UInt16:  always returns 0 
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt16 
halHardwareInterruptHandler (
							 UInt32  Context    // IN:  Handle to Hal Object
							 )
{
	// we should be getting the board handle from the IRQHandle
	tHalObject* pHal = (tHalObject*)Context;
	UInt32	   InterruptControl;
	
	InterruptControl = *(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL);
	
	if ((InterruptControl & 0xf0) == 0xf0)
    {
		// test mode
		DPF(0,("TriMedia TEST interrupt was generated!!\nWARNING: it will not be handled!!\n"));
		DPF(0,("INT_CTL=0x%lx\n", *(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL)));
		
		*(UInt32 *)(pHal->pMMIOAddrKernel + INT_CTL) = 0;
		
		return 0;
    }
	
	DPF(0,("tmhal.c:halHardwareInterruptHandler:InterruptControl is 0x%lx\n"
        ,InterruptControl));
	
	
	
	
	// here SelfInterrupt indicates PCI Interrupt A
	// INT#A = 0, INT#B = 1, INT#C = 2, INT#D = 3
	if ( 
		(  InterruptControl &
		( (1 << pHal->SelfInterrupt)  | (0x10 << pHal->SelfInterrupt) ) ) !=
		(unsigned)(    (1 << pHal->SelfInterrupt)  | (0x10 << pHal->SelfInterrupt)   ) 
		)
    {
		DPF(0,("tmhal.c: halHarIntHan: the compare value is 0x%x",
			(1 << pHal->SelfInterrupt)  | (0x10 << pHal->SelfInterrupt) ));
		DPF(0,("tmhal.c: halHardwareInterruptHandler: TRIMEDIA INTERRUPT ABORTED!! \n"));
		return 0;
    }
	
	halAcknowledgeInterrupt ( (UInt32)pHal ); 
	
	if (pHal->Handler)
    {
		pHal->Handler(pHal->Context);
    }
	
	return 0;
}

//-----------------------------------------------------------------------------
// FUNCTION:     halAccess32:
//
// DESCRIPTION:  This function returns the given 32-bit value in a Endian form 
//               it can read by the target or the host. If the Host and Target 
//               are running on a different Endianness then the value will be
//               byte swapped, otherwise the returned value will be the same
//               as the input one.
//
// RETURN:       UInt32:  Value byte-swapped if necessary
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt32	
halAccess32( 
			UInt32	        HalHandle,    // IN:  Handle to Hal Object
			UInt32 volatile Value         // IN:  Value to be converted if necessary
			)
{
	UInt32  SwappedValue;
	UInt32  TempValue;
	
	TempValue = Value;
	/* we don't validate the object for efficiency reasons */
	if ( ((tHalObject*)HalHandle)->Swapping )
    {
		((UInt8*)&SwappedValue)[3] = ((UInt8*)&TempValue)[0];
		((UInt8*)&SwappedValue)[2] = ((UInt8*)&TempValue)[1];
		((UInt8*)&SwappedValue)[1] = ((UInt8*)&TempValue)[2];
		((UInt8*)&SwappedValue)[0] = ((UInt8*)&TempValue)[3];
		DPF(0,("tmhal.c: halAccess32: [%lx->%lx]\n",TempValue, SwappedValue )); 
		return ( SwappedValue );
    }
	else
    {
		return Value;
    }
}

//-----------------------------------------------------------------------------
// FUNCTION:     halAccess16:
//
// DESCRIPTION:  This function returns the given 16-bit value in a Endian form 
//               it can read by the target or the host. If the Host and Target 
//               are running on a different Endianness then the value will be
//               byte swapped, otherwise the returned value will be the same
//               as the input one.
//
// RETURN:       UInt32:  Value byte-swapped if necessary
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt16
halAccess16 ( 
			 UInt32	     HalHandle,    // IN:  Handle to Hal Object
			 UInt16 volatile Value         // IN:  Value to be converted if necessary
			 )
{
	/* we don't validate the object for efficiency reasons */
	UInt16  SwappedValue;
	UInt16  TempValue;
	
	TempValue = Value;
	if ( ((tHalObject*)HalHandle)->Swapping )
    {
		((UInt8*)&SwappedValue)[1] = ((UInt8*)&TempValue)[0];
		((UInt8*)&SwappedValue)[0] = ((UInt8*)&TempValue)[1];
		DPF(0,("tmhal.c: halAccess16: [%x->%x]\n",TempValue, SwappedValue )); 
		return ( SwappedValue );
    }
	else
    {
		return Value;
    }
}


//-----------------------------------------------------------------------------
// FUNCTION:     halCopyback:
//
// DESCRIPTION:  Not used
//
// RETURN:       void
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
void	halCopyback( 
					Pointer CacheBlock, 
					UInt32 BlockCount )
{
	/*  no implementation for this platform */
	(void) CacheBlock;
	(void) BlockCount;
}

void	halAccessEnable ( 
						 UInt32	HalHandle )
{
	/*  no implementation for this platform */
	(void) HalHandle;
}

void	halAccessDisable ( 
						  UInt32	HalHandle )
{
	/*  no implementation for this platform */
	(void)HalHandle;
}




//-----------------------------------------------------------------------------
// FUNCTION:     halSwapEndianess:
//
// DESCRIPTION:  This function enables/disables the Endianness swapping
//
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halSwapEndianess ( 
				  UInt32  HalHandle,    // IN:  Handle to Hal Object
				  Bool	  SwapEnable    // IN:  True for Swap enabling
				  )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
	pHal->Swapping    = SwapEnable;
	DPF(8,("tmman:halSwapEndianess:Swapping[%x]\n", SwapEnable )); 
    
	return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     halGetEndianess:
//
// DESCRIPTION:  This function returns the current Swap setting.
//
// RETURN:       TMStatus:  always returns success
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
halGetEndianess ( 
				 UInt32  HalHandle,    //  IN:  Handle to Hal Object
				 Bool*   pSwapEnable   // OUT:  Current Swap setting
				 )
{
	tHalObject*  pHal = (tHalObject*)HalHandle;
	*pSwapEnable      = pHal->Swapping;
    
	return statusSuccess;
}



//-----------------------------------------------------------------------------
// FUNCTION:     halMapSDRAM:
//
// DESCRIPTION:  This function increments the SDRAM Map count if necessary
//
// RETURN:       Bool:  always returns True.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
Bool
halMapSDRAM( 
			UInt32	HalHandle     // IN:  Handle to Hal Object
			)
{
	tHalObject*   pHal = (tHalObject*)HalHandle;
	
	if ( TMManGlobal->MapSDRAM )
    {
		return True;
    }
	
	if ( pHal->SDRAMMapCount != 0 )
    {
		pHal->SDRAMMapCount++;
		return True;
    }
	
	
	pHal->SDRAMMapCount++;
	
	return True;
}

//-----------------------------------------------------------------------------
// FUNCTION:     halUnmapSDRAM:
//
// DESCRIPTION:  This function decrements the SDRAM Map count if necessary
//
// RETURN:       Bool:  True if count is not 0, False otherwise.
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
Bool 
halUnmapSDRAM( 
			  UInt32	HalHandle     // IN:  Handle to Hal Object
			  )
{
	tHalObject*   pHal = (tHalObject*)HalHandle;
    
	if ( TMManGlobal->MapSDRAM )
    {
		return True;
    }
    
	//unmatched Unmapping
	if ( pHal->SDRAMMapCount == 0 )
    {
		return False;
    }
    
	if ( pHal->SDRAMMapCount > 1 )
    {
		pHal->SDRAMMapCount--;
		return True;
    }
    
	pHal->SDRAMMapCount = 0;
    
	return True;
}




//-----------------------------------------------------------------------------
// FUNCTION:     halTranslateTargetPhysicalAddress:
//
// DESCRIPTION:  This function just returns the given address on this
//               platform.
// 
// RETURN:       UInt32:   the address
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt32	halTranslateTargetPhysicalAddress ( 
										   UInt32 HalHandle,  // IN:  Handle to Hal Object
										   UInt32 PhysicalAddress )
{
	//DL:	tHalObject* pHal = (tHalObject*)HalHandle;
	(void)HalHandle;
	
	PRINTF("halTranslateTargetPhysicalAddress %lx\n",PhysicalAddress);
	
	return ( PhysicalAddress );
}


//-----------------------------------------------------------------------------
// Internal functions:
//

//-----------------------------------------------------------------------------
// FUNCTION:     halDumpObject:
//
// DESCRIPTION:  This function just dumps the contents of the Hal object
//               to the Host debug buffer.
//
// RETURN:       void 
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
static void 
halDumpObject(
			  UInt32  HalHandle     // IN:  Handle to Hal Object
			  )
{
	UInt8         idx;
	tHalObject   *pHal;
	UInt32       *pMMIOBaseReg;
	
	pHal = (tHalObject*)HalHandle;
	
	DPF(0, ("HalHandle: 0x%lx\n", HalHandle));
	DPF(0,("MMIO: Len=0x%lx, Addr=0x%p\n", 
		pHal->MMIOLength, 
		pHal->pMMIOAddrKernel));
	DPF(0,("SDRAM: Len=0x%lx, Addr=0x%p\n", 
		pHal->SDRAMLength, 
		pHal->pSDRAMAddrKernel));
	
	
	pMMIOBaseReg = (UInt32*)(pHal->pMMIOAddrKernel + MMIO_BASE);
	DPF(8, ("MMIO Base Offset: 0x%x\n", MMIO_BASE));
	DPF(8, ("MMIO Base Reg Addr: 0x%p\n", pMMIOBaseReg));
	DPF(8, ("MMIO_BASE: 0x%lx\n", *pMMIOBaseReg));
    
	DPF(0,("Device Vendor Id: 0x%lx\n", 
		pHal->TMDeviceVendorID));
	
	DPF(0,("Subsystem Id: 0x%lx\n", 
		pHal->TMSubsystemID));
	
	DPF(0,("Class Revision Id: 0x%lx\n", 
		pHal->TMClassRevisionID));
	
	DPF(0,("PCI Registers: \n"));
	for (idx = 0; idx < constTMMANPCIRegisters; idx++)
    {
		DPF(0,("\t0x%lx\n", pHal->PCIRegisters[idx]));
    }
	
	DPF(8, ("MMIO Values:\n"));
	DPF(8, ("\tBIU_CTL:    0x%lx\n", *((UInt32*)(pHal->pMMIOAddrKernel + BIU_CTL   ))));
	DPF(8, ("\tDRAM_BASE:  0x%lx\n", *((UInt32*)(pHal->pMMIOAddrKernel + DRAM_BASE ))));
	DPF(8, ("\tMMIO_BASE:  0x%lx\n", *((UInt32*)(pHal->pMMIOAddrKernel + MMIO_BASE ))));
	DPF(8, ("\tDRAM_LIMIT: 0x%lx\n", *((UInt32*)(pHal->pMMIOAddrKernel + DRAM_LIMIT))));
	DPF(8, ("\tAO_FREQ:    0x%lx\n", *((UInt32*)(pHal->pMMIOAddrKernel + AO_FREQ   ))));
	
}





















