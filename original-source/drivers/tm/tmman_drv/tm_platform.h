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
/*
HISTORY
970703  Tilakraj Roy   Created
980604  VS             Ported to Windows CE
990511  DTO            Ported to pSOS
001109  DL             Ported to Linux

*/


#ifndef _PLATFORM_H_ 
#define	_PLATFORM_H_ 

/*----------------------------------------------------------------------------
SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmtypes.h"
#include "tmmanapi.h"


#define BYTE_SWAP4(x) \
	(  ( ( ((UInt32)(x)) & 0x000000ff ) << 24 ) \
	|  ( ( ((UInt32)(x)) & 0x0000ff00 ) <<  8 ) \
	|  ( ( ((UInt32)(x)) & 0x00ff0000 ) >>  8 ) \
	|  ( ( ((UInt32)(x)) & 0xff000000 ) >> 24 ) \
)


#define PEPROCESS       UInt32

#define	constTMMANPCIRegisters			0x10


// required by the hal for bit settings of the MMIO BIU_CTL register
#define constTMManBIU_CTL_SE    (0x0001) // Swap Enable (PCI Little Endian)
#define constTMManBIU_CTL_BO    (0x0002) // Burst Mode Off
#define constTMManBIU_CTL_IE    (0x0100) // ICP DMA Enable
#define constTMManBIU_CTL_HE    (0x0200) // Host Enable
#define constTMManBIU_CTL_CR    (0x0400) // Clear Reset
#define constTMManBIU_CTL_SR    (0x0800) // Set Reset

// Used for enabling/disabling PCI access
#define	constTMManDC_LOCK_CTL_MASK	0x00000060
#define	constTMManDC_LOCK_CTL_POSITION	5	// LEFT SHIFT POSITIONS
#define	constTMManDC_LOCK_CTL_HEN	0x0	// HOLE ENABLE 
#define	constTMManDC_LOCK_CTL_HDS	0x1	// HOLE DISABLE 
#define	constTMManDC_LOCK_CTL_PDS	0x2     // PCI DSIABLE 
#define	constTMManDC_LOCK_CTL_RES	0x3	// RESERVED 


//#define	constTMManHostTraceBufferSize	2048


/*
// defines for kernel mode IOCTL  Calls.
#define TMM_IOCTL_SYSMEM_PAGE_LOCK_OFFSET      0x1  // Lock system mem page(s)
#define TMM_IOCTL_SYSMEM_PAGE_UNLOCK_OFFSET    0x2  // Unlock system mem page(s)
#define TMM_IOCTL_GET_PHYS_ADDRESS_OFFSET      0x3  // Get Physical Address
*/

typedef struct tagDebugObject
{
    UInt32  DBGLevelBitmap;
    UInt32  DBGType;
    UInt8   DBGBuffer[constTMManStringLength];
    UInt8   TraceBufferData[constTMManHostTraceBufferSize]; /*HARDCODED */
    UInt8*  TraceBuffer;
    UInt32  TracePosition;
    UInt32  TraceLength;
    Bool    TraceWrapped;
}  DebugObject;



typedef struct tagClientDeviceObject
{
	void*			Device;
	
    // Mapped PCI addresses
	UInt8*			SDRAMAddrUser;
	UInt8*			MMIOAddrUser;
    // Physical PCI addressesS
	UInt8*			SDRAMAddrPhys;
	UInt8*			MMIOAddrPhys;
	
    // this is pagelocked memory accessible to the host and the target
	UInt8*			MemoryAddrUser; 
	
}	ClientDeviceObject;


typedef struct tagClientObject
{
	PEPROCESS	       	Process;
	void*		       	PhysicalMemoryObject;
	UInt32			DeviceCount;
	ClientDeviceObject	Device[1];
}	ClientObject;



typedef struct  tagGlobalObject
{
	UInt32			MaximumDevices;
	UInt32			DeviceCount;
	void*			DeviceList[constTMMANMaximumDeviceCount];
	
	UInt32			MaximumClients;
	UInt32			ClientCount;
	void*			ClientList[constTMMANMaximumClientCount];
	
	DebugObject		Debug;
	
	UInt32			BitFlags;
	
	UInt32			XlatedAddress;
	
	// settings read from the registry 
	UInt32			TargetTraceBufferSize;
	UInt32			TargetTraceLeveBitmap;
	UInt32			TargetTraceType;
	UInt32			MemorySize;
	UInt32			MailboxCount;
	UInt32			ChannelCount;
	UInt32			VIntrCount;
	UInt32			MessageCount;
	UInt32			EventCount;
	UInt32			StreamCount;
	UInt32			NameSpaceCount;
	UInt32			MemoryCount;
	UInt32			SGBufferCount;    
	UInt32			SpeculativeLoadFix;
	UInt32			PCIInterruptNumber;
	UInt32			MMIOInterruptNumber;
	UInt32                  MapSDRAM;
	UInt32                  OEMIOCTLBase;
	UInt32                  PageLockedMemoryBase;
	UInt32                  PageLockedMemorySize;
	
}   GlobalObject;




typedef struct taghalParameters
{
    Pointer             SharedData;
    UInt32              SpeculativeLoadFix;
    UInt32              PCIIrq;
    UInt32              TMIrq;
	
	/*
    ULONG		TMDeviceVendorID;
    ULONG	      	TMSubsystemID;
    ULONG     		TMClassRevisionID;
	
	 ULONG     		BridgeDeviceVendorID;
	 ULONG		BridgeSubsystemID;
	 ULONG		BridgeClassRevisionID;
	 
	  ULONG	        BusNumber;
	  PCI_SLOT_NUMBER	SlotNumber;
	  
	   ULONG		DSPNumber;
	   
		ULONG		InterruptLevel;
		ULONG		InterruptVector;
		ULONG		InterruptAffinity;
		
		 PHYSICAL_ADDRESS    MMIOAddrPhysical;
		 ULONG               MMIOLength;
		 
		  PHYSICAL_ADDRESS    SDRAMAddrPhysical;
		  ULONG               SDRAMLength;
		  
		   ULONG		CPURevision;
		   ULONG		BoardRevision;
		   
			PDEVICE_OBJECT      DeviceObject;
			PDRIVER_OBJECT      DriverObject;
			
			 ULONG		PCIRegisters[constTMMANPCIRegisters];
	*/
}	halParameters;



typedef struct tagTMManDeviceObject
{
    /* generic part */
    // user set Flags - for storing device specific information
    UInt32	Flags;
    
    // handle to the CRT Object
    // just stored here by tmman32 for later retrieval
    // not interpreted in kernel mode.
    UInt32	CRunTimeHandle;
    
    // current state of the DSP
    // LOADED, RESET, RUNNING, DEAD, DEFUNCT
    UInt32	Status; 
    
    UInt32	DSPNumber;
    
    // why the fuck is this here
    UInt8*	SharedMapped;
    
    TMManSharedStruct*		SharedData;
    
    // handle to all the component objects
    UInt32	HalHandle;
    UInt32	ChannelManagerHandle;
    UInt32	VIntrManagerHandle;
    UInt32	EventManagerHandle;
    UInt32	MessageManagerHandle;
    UInt32	MemoryManagerHandle;
    UInt32	SGBufferManagerHandle;
    UInt32	NameSpaceManagerHandle;
    
    Pointer	HalSharedData;
    Pointer	MemorySharedData;
    Pointer	NameSpaceSharedData;
    UInt32	ChannelSharedData;
    UInt32	VIntrSharedData;
    UInt32	EventSharedData;
    
    UInt32	SGBufferSharedData;
    
    UInt32	SharedDataHandle;
    
    Pointer	MemoryBlock;
    UInt32	MemoryBlockSize;
    
    
    // physical addresses needed by the loader for relocation
    UInt32	HalSharedAddress;
    UInt32	EventSharedAddress;
    UInt32	ChannelSharedAddress;
    UInt32	VIntrSharedAddress;
    UInt32	DebugSharedAddress;
    UInt32	MemorySharedAddress;
    UInt32	MemoryBlockAddress;
    UInt32	NameSpaceSharedAddress;
    UInt32	SGBufferSharedAddress;
    UInt32	TMManSharedAddress;
    
    // DTO: not needed
    // hardware initialization parameters
    // halParameters	HalParameters;
    
    
}	TMManDeviceObject;

extern GlobalObject *TMManGlobal;

// BOOLEAN	pnpFindPCIDevices ( USHORT wVendor, USHORT wDevice );

Bool  halMapSDRAM ( 
				   UInt32 HalHandle );

Bool  halUnmapSDRAM ( 
					 UInt32 HalHandle );


Pointer  MmAllocateContiguousMemory (
									 UInt32 Size);

Bool  MmFreeContiguousMemory(
							 Pointer BaseAddress);

UInt32  MmGetPhysicalAddress (
							  Pointer BaseAddress);



/*DL: moved from tmhal.c */
typedef struct
{
    GenericObject       Object;
	
    UInt32              TMDeviceVendorID;
    UInt32              TMSubsystemID;
    UInt32              TMClassRevisionID;
	
    UInt32              BridgeDeviceVendorID;
    UInt32              BridgeSubsystemID;
    UInt32              BridgeClassRevisionID;
	
    UInt32*             pMMIOAddrPhysical; 
    UInt32              MMIOLength;
    UInt8*              pMMIOAddrKernel;
	
    UInt32*             pSDRAMAddrPhysical;
    UInt32              SDRAMLength;
    UInt8*              pSDRAMAddrKernel;
	
    UInt32              PCIRegisters[constTMMANPCIRegisters];
	
    UInt32              SelfInterrupt; // from Target -> Host 
    UInt32              PeerInterrupt; // Host -> Target 
	
    HalInterruptHandler	Handler;
    Pointer		Context;
	
    HalControl*         pControl;
    UInt32              SpeculativeLoadFix;
    Bool                Swapping;	
	
    UInt32              PeerMajorVersion;
    UInt32              PeerMinorVersion;
	
    UInt32              SDRAMMapCount;
	
} tHalObject;



#endif /*_PLATFORM_H_*/








