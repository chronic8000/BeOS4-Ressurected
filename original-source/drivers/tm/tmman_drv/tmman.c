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
970804	Tilakraj Roy	   Created from vtmman sources for NT Workstation
982005	Volker Schildwach  Ported to WinCE User Mode DLL
990511  DTO                Ported to pSOS
001108                     Ported to Linux
*/

//-----------------------------------------------------------------------------
// Standard include files:
//
#include <stdio.h>
// DL: #include <psos.h>

//-----------------------------------------------------------------------------
// Project include files:
//


#include "tmif.h"
#include "tm_platform.h"
#include "tmmanlib.h"

//-----------------------------------------------------------------------------
// Types and defines:
//

//-----------------------------------------------------------------------------
// Global data and external function/data references:
//
GlobalObject *TMManGlobal;


//-----------------------------------------------------------------------------
// Prototypes:
//
static void  tmmanCreateGlobalObj(void);

//-----------------------------------------------------------------------------
// Exported functions:
//
//-----------------------------------------------------------------------------
// FUNCTION:     tmmInit:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
void  tmmInit(ioparms  *pIoParms)
{
	debugParameters               DebugParameters;
	TMManDeviceObject            *TMManDevice;
	UInt32                        TMManControlSize;
	UInt32                        HalControlSize;
	UInt32                        NameSpaceControlSize;
	UInt32                        MemoryControlSize;
	UInt32                        TotalSize;
	TMStatus                      Status;
	channelManagerParameters	ChannelParameters;
	vintrManagerParameters	VIntrParameters;
	eventManagerParameters    	EventParameters;
	messageManagerParameters	MessageParameters;
	memoryManagerParameters   	MemoryParameters;
	namespaceManagerParameters	NameSpaceParameters;
	sgbufferManagerParameters	SGBufferParameters;
	halParameters                 HalParameters;
	
	
	// Create & configure the TMMAN Global Object
	tmmanCreateGlobalObj();
	// Initialize the debugging subsystem so that we can start dumping stuff
	DebugParameters.TraceBufferSize = constTMManHostTraceBufferSize;
	DebugParameters.LevelBitmap     = 0x1;
	DebugParameters.Type            = constTMManDebugTypeTrace;
	debugInit (&DebugParameters);
    
	DPF(8, ("\nDPF::::::::::tmmInit Started.\n"));
	
	// all this has to be done on a device specific basis
	TMManDevice = (TMManDeviceObject *)memAllocate(sizeof(TMManDeviceObject));
	
	if ( TMManDevice == Null)
    {
		DPF(0,("tmman:tmmInit:memAlloc:TMManDevice:FAIL\n"));
		goto tmmInitExit1;			
    }
	// for back reference
	TMManDevice->Status    = constTMManDSPStatusReset;
	TMManDevice->DSPNumber = TMManGlobal->DeviceCount;
	
	// allocate shared memory for TMMan Control, Hal, NameSpace, Memory 
	// allocating these toghether provides optimum usage of Contiguous Memory
	// calculate the total size required
	TMManControlSize     = sizeof ( TMManSharedStruct );
	HalControlSize       = sizeof ( HalControl );
	NameSpaceControlSize = namespaceSharedDataSize ( TMManGlobal->NameSpaceCount );
	MemoryControlSize    = memorySharedDataSize ( TMManGlobal->MemoryCount );
	
	TotalSize = TMManControlSize + HalControlSize + NameSpaceControlSize + 
		MemoryControlSize + TMManGlobal->MemorySize;
	
	// any shared memory that is allocate outside memoryCreate should be done
	// via MmAllocateContiguousMemory instead of ExAllocatePool
	// Volker : added memory pool size to allocate all at one place
	TMManDevice->SharedData = (TMManSharedStruct *)
		MmAllocateContiguousMemory (TotalSize);
	
	DPF(0,("tmman.c:tmmInit:TMManDevice->SharedData(virtual and cached) is 0x%p\n" 
        ,TMManDevice->SharedData));
	
	
	if ( TMManDevice->SharedData == Null )
    {
		DPF(0,("tmman:tmmInit:MmAllocateContiguousMemory:SharedData:FAIL\n"));
		goto tmmInitExit2;
    }
	
	// DL: on MIPS we want this memory to be noncached
	// The vtonocache function is defined in include/asm/io.h

	/*
	according to Dimitri, this call is useless: CPU cache is flused/invalidated
	when a read/write occurs.
	
	TMManDevice->SharedData = vtonocache(TMManDevice->SharedData);
	*/
	
	
	TMManDevice->TMManSharedAddress = MmGetPhysicalAddress(TMManDevice->SharedData);
	
	DPF(0,("tmman.c:tmmInit:TMManDevice->SharedData(virtual and uncached) is 0x%p\n"
        ,TMManDevice->SharedData));
	DPF(0,("tmman.c: tmmInit: TMManDevice->TMManSharedAddress(phys) is 0x%lx\n"
        ,TMManDevice->TMManSharedAddress));
	
	
	//
	// HAL object
	//
	//	NOTE : halInitization has to be done before anything else since the
	//	endianness fucntions are in Hal 
	
	TMManDevice->HalSharedData =
		( ((UInt8*)TMManDevice->SharedData) + TMManControlSize );
	
	TMManDevice->HalSharedAddress = MmGetPhysicalAddress( 
		TMManDevice->HalSharedData );
	
	HalParameters.SpeculativeLoadFix = TMManGlobal->SpeculativeLoadFix;
	HalParameters.SharedData         = TMManDevice->HalSharedData;
	HalParameters.PCIIrq             = TMManGlobal->PCIInterruptNumber;
	HalParameters.TMIrq              = TMManGlobal->MMIOInterruptNumber;
	DPF(0,("tmman.c:tmmInit: HalCreate is about to be called.\n"));
	if ((Status = halCreate(
		(Pointer)&HalParameters,
		&TMManDevice->HalHandle)) != statusSuccess)
    {
		DPF(0,("tmman:tmmInit:halCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit3;
    }
	DPF(8, ("tmmInit: Hal Create Success\n"));
	
	
	//
	// name space object
	//
	NameSpaceParameters.HalHandle       = TMManDevice->HalHandle;
	NameSpaceParameters.NameSpaceCount  = TMManGlobal->NameSpaceCount;
	NameSpaceParameters.SharedData      =
		TMManDevice->NameSpaceSharedData  = 
		( ((UInt8*)TMManDevice->SharedData) + 
		TMManControlSize + HalControlSize );
	
	TMManDevice->NameSpaceSharedAddress = MmGetPhysicalAddress( 
		NameSpaceParameters.SharedData);
	
	if ( ( Status = namespaceManagerCreate ( 
		&NameSpaceParameters,
		&TMManDevice->NameSpaceManagerHandle ) ) 
		!= statusSuccess )
    {
		DPF(0,("tmman:tmmInit:namespaceManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit4;
    }
	
	DPF(8, ("tmmInit: namespaceManagerCreate success\n"));
	
	
	//
	// memory object
	//
	
	MemoryParameters.MemoryCount = TMManGlobal->MemoryCount;
	MemoryParameters.MemorySize  = TMManGlobal->MemorySize;
	MemoryParameters.HalHandle   = TMManDevice->HalHandle;
	MemoryParameters.NameSpaceManagerHandle = 
		TMManDevice->NameSpaceManagerHandle;
	MemoryParameters.SharedData = 
		TMManDevice->MemorySharedData = 
		( ((UInt8*)TMManDevice->SharedData) + 
		TMManControlSize + HalControlSize  + NameSpaceControlSize );
	
	TMManDevice->MemorySharedAddress = MmGetPhysicalAddress(
		MemoryParameters.SharedData);
	
	DPF(0,("tmman.c: tmminit(): TMManDevice->MemorySharedAddress is 0x%lx\n", TMManDevice->MemorySharedAddress));   
	
	// memory block is alread allocated
	MemoryParameters.MemoryBlock =
		TMManDevice->MemoryBlock =
		( ((UInt8*)TMManDevice->SharedData) + 
		TMManControlSize + HalControlSize  + NameSpaceControlSize + MemoryControlSize);
	
	TMManDevice->MemoryBlockSize    = MemoryParameters.MemorySize;
	TMManDevice->MemoryBlockAddress = MmGetPhysicalAddress (
		TMManDevice->MemoryBlock );
	
	
	DPF(0,("tmman.c: tmminit(): TMManDevice->MemoryBlockAddress is 0x%lx\n", TMManDevice->MemoryBlockAddress));
	
	if ( memoryManagerCreate ( 
		&MemoryParameters,
		&TMManDevice->MemoryManagerHandle ) != statusSuccess )
    {
		DPF(0,("tmman:tmmInit:memoryManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit6;
    }
	
	DPF(8, ("tmmInit: memoryManagerCreate success\n"));
	
	
	//
	// virtual interrupt object
	//
	VIntrParameters.HalHandle  = TMManDevice->HalHandle;
	VIntrParameters.VIntrCount = TMManGlobal->VIntrCount;
	
	Status = memoryCreate(
		TMManDevice->MemoryManagerHandle,
		(Pointer)pIoParms->tid,    // Client Handle, process id
		"TMManVIntr", 
		vintrSharedDataSize ( TMManGlobal->VIntrCount ), 
		&VIntrParameters.SharedData,
		&TMManDevice->VIntrSharedData );
	
	if ( Status != statusSuccess )
    {
		DPF(0,("tmman:tmmInit:memoryCreate:VIntr:FAIL[%lx]\n", Status));
		goto tmmInitExit7;
    }
	
	TMManDevice->VIntrSharedAddress = MmGetPhysicalAddress( 
							 VIntrParameters.SharedData);
	
	if ( ( Status = vintrManagerCreate ( 
		&VIntrParameters,
		&TMManDevice->VIntrManagerHandle ) ) != statusSuccess )
    {
		DPF(0,("tmman:tmmInit:vintrManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit8;
    }
	DPF(8, ("tmmInit: vintrManagerCreate success\n"));
	
	
	//
	// interprocessor mailbox object
	//
	
	ChannelParameters.ChannelCount           = TMManGlobal->ChannelCount; 
	ChannelParameters.MailboxCount           = TMManGlobal->MailboxCount;
	ChannelParameters.PacketSize             = constTMManPacketSize;
	ChannelParameters.HalHandle              = TMManDevice->HalHandle;
	ChannelParameters.VIntrManagerHandle     = TMManDevice->VIntrManagerHandle;
	ChannelParameters.NameSpaceManagerHandle = TMManDevice->NameSpaceManagerHandle;
	
	Status = memoryCreate(
		TMManDevice->MemoryManagerHandle,
		(Pointer)pIoParms->tid,    // Client Handle, process id
		"TMManChannel", 
		channelSharedDataSize( TMManGlobal->MailboxCount
		, constTMManPacketSize ),
		&ChannelParameters.SharedData,
		&TMManDevice->ChannelSharedData );
	
	if ( Status != statusSuccess )
    {
		DPF(0,("tmman:tmmInit:memoryCreate:Channel:FAIL[%lx]\n", Status));
		goto tmmInitExit9;
    }
	
	TMManDevice->ChannelSharedAddress = MmGetPhysicalAddress(
		ChannelParameters.SharedData);
	
	if (( Status = channelManagerCreate ( 
		&ChannelParameters,
		&TMManDevice->ChannelManagerHandle )) 
		!= statusSuccess )
    {
		DPF(0,("tmman:tmmInit:channelManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit10;
    }
	DPF(8, ("tmmInit: channelManagerCreate success\n"));
	
	
	//
	// interprocessor event object
	//
	EventParameters.EventCount             = TMManGlobal->EventCount; 
	EventParameters.HalHandle              = TMManDevice->HalHandle;
	EventParameters.VIntrManagerHandle     = TMManDevice->VIntrManagerHandle;
	EventParameters.NameSpaceManagerHandle = TMManDevice->NameSpaceManagerHandle;
	
	Status = memoryCreate(
		TMManDevice->MemoryManagerHandle,
		(Pointer)pIoParms->tid,
		"TMManEvent", 
		eventSharedDataSize ( TMManGlobal->EventCount ), 
		&EventParameters.SharedData,
		&TMManDevice->EventSharedData );
	
	if ( Status != statusSuccess )
    {
		DPF(0,("tmman:tmmInit:memoryCreate:Event:FAIL[%lx]\n", Status));
		goto tmmInitExit11;
    }
	
	TMManDevice->EventSharedAddress = MmGetPhysicalAddress(
							 EventParameters.SharedData);
	
	if (( Status = eventManagerCreate ( 
		&EventParameters,
		&TMManDevice->EventManagerHandle )) 
		!= statusSuccess )
    {
		DPF(0,("tmman:tmmInit:eventManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit12;
    }
	DPF(8, ("tmmInit: eventManagerCreate success\n"));
	
	
	//
	// interprocessor message packet object
	//
	MessageParameters.MessageCount           = TMManGlobal->MessageCount; 
	MessageParameters.PacketSize             = constTMManPacketSize;
	MessageParameters.HalHandle              = TMManDevice->HalHandle;
	MessageParameters.ChannelManagerHandle   = TMManDevice->ChannelManagerHandle;
	MessageParameters.NameSpaceManagerHandle = TMManDevice->NameSpaceManagerHandle;
	
	
	if (( Status = messageManagerCreate ( 
		&MessageParameters,
		&TMManDevice->MessageManagerHandle )) 
		!= statusSuccess )
    {
		DPF(0,("tmman:tmmInit:messageManagerCreate:FAIL[%lx]\n", Status));
		goto tmmInitExit13;
    }
	DPF(8, ("tmmInit: messageManagerCreate success\n"));
	
	//
	// scatter gather buffer object
	//
	SGBufferParameters.SGBufferCount          = TMManGlobal->SGBufferCount;
	SGBufferParameters.HalHandle              = TMManDevice->HalHandle;
	SGBufferParameters.MemoryManagerHandle    = TMManDevice->MemoryManagerHandle;
	SGBufferParameters.NameSpaceManagerHandle = TMManDevice->NameSpaceManagerHandle;
	
	/* DL : We have not yet ported the scatter gather buffer mechanism to
	linux, so we comment it out.
	if (( Status =  sgbufferManagerCreate ( 
	&SGBufferParameters,
	&TMManDevice->SGBufferManagerHandle )) != statusSuccess )
	{
	DPF(0,("tmman:tmmInit:sgbufferManagerCreate:FAIL[%x]\n", Status));
	goto tmmInitExit14;
	}
	*/
	
	DPF(8, ("tmmInit: sgbufferManagerCreate success\n"));
	
	
	TMManGlobal->DeviceList[TMManGlobal->DeviceCount] = TMManDevice;
	TMManGlobal->DeviceCount++;
	
	DPF(8, ("\nInitialisation complete\n"));
    
	pIoParms->err = 0;
	return;
	
	
/*
unused label
tmmInitExit14:
*/
	messageManagerDestroy ( TMManDevice->MessageManagerHandle );
	
tmmInitExit13:
	eventManagerDestroy ( TMManDevice->EventManagerHandle );
	
tmmInitExit12:
	memoryDestroy( TMManDevice->EventSharedData );
	
tmmInitExit11:
	channelManagerDestroy ( TMManDevice->ChannelManagerHandle );
	
tmmInitExit10:
	memoryDestroy( TMManDevice->ChannelSharedData );
	
tmmInitExit9:
	vintrManagerDestroy ( TMManDevice->VIntrManagerHandle );
	
tmmInitExit8:
	memoryDestroy( TMManDevice->VIntrSharedData );
	
tmmInitExit7:
	memoryManagerDestroy ( TMManDevice->MemoryManagerHandle );
	
tmmInitExit6:
	MmFreeContiguousMemory ( TMManDevice->MemoryBlock );
	
/*
unused label:
tmmInitExit5:
*/
	namespaceManagerDestroy ( TMManDevice->NameSpaceManagerHandle );
	
tmmInitExit4:
	halDestroy ( TMManDevice->HalHandle );
	
tmmInitExit3:
	MmFreeContiguousMemory ( TMManDevice->SharedData );
	
tmmInitExit2:
	memFree ( TMManDevice );
	
tmmInitExit1:
	
	pIoParms->err = 1;
	return;
}


//-----------------------------------------------------------------------------
// Internal functions:
//

//-----------------------------------------------------------------------------
// FUNCTION:     tmmanCreateGlobalObj:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
static void  tmmanCreateGlobalObj(void)
{
	UInt32              Idx;
	
	// Create TMMan Global object
	if (    (TMManGlobal = (GlobalObject*)memAllocate (sizeof(GlobalObject))) 
		== Null
		)
    {
		//exit(-1);
		DPF(0, ("tmman.c: tmmanCreateGlobalObj: could not allocate memory"));
    }
    
	// Initialise TMMan Global object
	TMManGlobal->TargetTraceBufferSize = constTMManTargetTraceBufferSize;
	TMManGlobal->TargetTraceLeveBitmap = 0x0;
	TMManGlobal->TargetTraceType       = constTMManTargetTraceType;
	TMManGlobal->MemorySize            = constTMManMemorySize;
	TMManGlobal->MailboxCount          = constTMManMailboxCount;
	TMManGlobal->ChannelCount          = constTMManChannelCount;
	TMManGlobal->VIntrCount            = constTMManVIntrCount;
	TMManGlobal->MessageCount          = constTMManMessageCount;
	TMManGlobal->EventCount            = constTMManEventCount;
	TMManGlobal->NameSpaceCount        = constTMManNameSpaceCount;
	TMManGlobal->MemoryCount           = constTMManMemoryCount;
	TMManGlobal->SGBufferCount         = constTMManSGBufferCount;
	TMManGlobal->StreamCount           = constTMManStreamCount;
	TMManGlobal->SpeculativeLoadFix    = 0;
	TMManGlobal->PCIInterruptNumber    = constTMMANTargetToHostIRQ;   
	// DL: =  0 (PCI INTA = 0)
	TMManGlobal->MMIOInterruptNumber   = constTMMANHostToTargetIRQ;  // = 28
	TMManGlobal->MapSDRAM              = True;
	
	// DTO: specific to WinCE (not really needed)
	TMManGlobal->OEMIOCTLBase         = constTMMANOEMIoctlBase;
	TMManGlobal->PageLockedMemoryBase = constTMMANPageLockedMemoryBase;
	TMManGlobal->PageLockedMemorySize = constTMMANPageLockedMemorySize;
	
	TMManGlobal->BitFlags              = 0;
	TMManGlobal->MaximumDevices        = constTMMANMaximumDeviceCount;
	TMManGlobal->MaximumClients        = constTMMANMaximumClientCount;
	TMManGlobal->DeviceCount           = 0;
	TMManGlobal->ClientCount           = 0;
    
	for ( Idx = 0 ; Idx < TMManGlobal->MaximumDevices ; Idx++ )
    {
		TMManGlobal->DeviceList[Idx] = Null;
    }
    
	for ( Idx = 0 ; Idx < TMManGlobal->MaximumClients ; Idx++ )
    {
		TMManGlobal->ClientList[Idx] = Null;
    }
}



