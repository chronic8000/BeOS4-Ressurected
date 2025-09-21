/*
	(c) Philips Semiconductors - TriMedia Product Group 1997

	TriMedia Manager Interprocessor Communicaiton Library Interface

	FILE	TMManLib.h

	Author : T.Roy (Tilakraj.Roychoudhury@sv.sc.philips.com)

*/
#ifndef _TMMANLIB_H_
#define	_TMMANLIB_H_


#include "tmtypes.h"
#include "tmmanerr.h"

/*
inclusion for:
- definition of PRINTF
*/
#include "../mydefs.h"

/*
	TYPES
*/

/* 
	Every Object Object has an associated FOURCC ID
	Primarily used for debugging 
*/

#ifndef tmmanOBJECTID 

#define tmmanOBJECTID( ch0, ch1, ch2, ch3 ) ( (UInt32)(UInt8)(ch0) | ( (UInt32)(UInt8)(ch1) << 8 ) | ( (UInt32)(UInt8)(ch2) << 16 ) | ( (UInt32)(UInt8)(ch3) << 24 ) )
#endif

#define		constTMMANHostToTargetIRQ	28	/* IRQ 28 */
#define		constTMMANTargetToHostIRQ	0	/* PCI INT#A */

#define	constDebugMagic				"XXXXXXX:cjp:da:fd:gfp:lcf:rj:troy:wcs:yc:zcw\0"
#define	constDebugTMManMagic		"XXXXXXXX.Roychoudhury@sv.sc.philips.com\0"
#define	constDebugMagicSize			0x40	/* maximum 64 characterrs */

#define constTMManDebugTypeNULL			0
#define constTMManDebugTypeTrace		1
#define constTMManDebugTypeOutput		2
#define constTMManDebugTypeCOM			3	
#define constTMManDebugTypeLPT			4	

#define	constTMManTargetTraceBufferSize				0x1000
//#define	constTMManTargetTraceLeveBitmap				0x00000001
#define	constTMManTargetTraceLeveBitmap				0xffffffff
//#define	constTMManTargetTraceType					constTMManDebugTypeNULL
#define	constTMManTargetTraceType					constTMManDebugTypeTrace

#define	constTMManHostTraceBufferSize				0x1000
#define	constTMManHostTraceLeveBitmap				0x00000001
//#define	constTMManHostTraceType						constTMManDebugTypeNULL
#define	constTMManHostTraceType						constTMManDebugTypeTrace

#define constTMManNameSpaceObjectInvalid		0
#define constTMManNameSpaceObjectVIntr			1
#define constTMManNameSpaceObjectChannel		2
#define constTMManNameSpaceObjectEvent			3
#define constTMManNameSpaceObjectMemory			4	
#define constTMManNameSpaceObjectSGBuffer		5	
#define constTMManNameSpaceObjectDebug			6	
#define constTMManNameSpaceObjectMessage		7	


#define constTMMANOEMIoctlBase					(0x8000)
#define constTMMANPageLockedMemoryBase			(0x80080000)
#define constTMMANPageLockedMemorySize			(0x18000)

#define		constTMMANTM1000VendorID			0x1131
#define		constTMMANTM1000DeviceID			0x5400

#define		constTMMANTM2000VendorID			0x1131
#define		constTMMANTM2000DeviceID			0x5401

#define		constTMMANTM1300VendorID			0x1131
#define		constTMMANTM1300DeviceID			0x5402

#define		constTMMANDECBridgeVendorID			0x1011
#define		constTMMANDECBridgeDeviceID			0x0046



/*
	DATA STRUCTURES
*/

typedef struct tagTMManSharedStruct
{
	volatile	UInt32		HostKernelMajorVersion;
	volatile	UInt32		HostKernelMinorVersion;
	volatile	UInt32		TargetKernelMajorVersion;
	volatile	UInt32		TargetKernelMinorVersion;

	volatile	UInt32		ChannelShared;
	volatile	UInt32		VIntrShared;
	volatile	UInt32		EventShared;
	volatile	UInt32		DebugShared;
	volatile	UInt32		MemoryShared;
	volatile	UInt32		MemoryBlock;
	volatile	UInt32		NameSpaceShared;
	volatile	UInt32		SGBufferShared;
	volatile	UInt32		HalShared;


	volatile	UInt32		Reserved1;
	volatile	UInt32		Reserved2;

	volatile	UInt32		TargetTraceBufferSize;
	volatile	UInt32		TargetTraceLeveBitmap;
	volatile	UInt32		TargetTraceType;

	volatile	UInt32		DebuggingHi;
	volatile	UInt32		DebuggingLo;

	volatile	UInt32		TMManMailboxCount;
	volatile	UInt32		TMManChannelCount;
	volatile	UInt32		TMManVIntrCount;
	volatile	UInt32		TMManMessageCount;
	volatile	UInt32		TMManEventCount;
	volatile	UInt32		TMManStreamCount;
	volatile	UInt32		TMManNameSpaceCount;
	volatile	UInt32		TMManMemoryCount;
	volatile	UInt32		TMManMemorySize;
	volatile	UInt32		TMManSGBufferCount;

	volatile	UInt32		SpeculativeLoadFix;
	volatile	UInt32		PCIInterruptNumber;
	volatile	UInt32		MMIOInterruptNumber;

	volatile	UInt32		TMDeviceVendorID;
	volatile	UInt32		TMSubsystemID;
	volatile	UInt32		TMClassRevisionID;

	volatile	UInt32		BridgeDeviceVendorID;
	volatile	UInt32		BridgeSubsystemID;
	volatile	UInt32		BridgeClassRevisionID;

	volatile	UInt32		Reserved[16];

}	TMManSharedStruct;	
	
typedef	struct	tagGenericObject
{
	UInt32	FourCCID;
	Pointer	SelfRef;
}	GenericObject;

typedef	struct	tagObjectList
{
	UInt32	Count;
	UInt32	FreeCount;
	Pointer	*Objects;
}	ObjectList;

typedef struct	tagchannelManagerParameters
{
	UInt32	ChannelCount; 
	UInt32	MailboxCount;
	UInt32	PacketSize;
	Pointer	SharedData; 
	UInt32	HalHandle;
	UInt32	VIntrManagerHandle;
	UInt32	NameSpaceManagerHandle;
}	channelManagerParameters;

typedef void    ( * ChannelHandler )( 
	UInt32	ChannelHandle,
	Pointer Packet,		
	Pointer Context );	

UInt32	channelSharedDataSize ( 
	UInt32 MailboxCount, 
	UInt32 PacketSize );

TMStatus	channelManagerCreate (
	channelManagerParameters*	Parameters,
	UInt32*	ChannelManagerPointer );

TMStatus	channelCreate(
	UInt32	ChannelManagerHandle,
	String	Name, 
	Pointer Handler, 
	Pointer Context,
	UInt32* ChannelHandlePointer );


TMStatus	channelSend(
	UInt32 ChannelHandle, 
	Pointer Packet );

TMStatus	channelDestroy ( 
	UInt32 ChannelHandle);

TMStatus	channelManagerReset ( 
	UInt32 ChannelManagerHandle );

TMStatus	channelManagerDestroy ( 
	UInt32 ChannelManagerHandle );

/*
	INTERRUPT MODULE
*/



typedef struct tagvintrManagerParameters
{
	UInt32	HalHandle;
	UInt32	VIntrCount;
	Pointer	SharedData; 
}	vintrManagerParameters;

typedef void    ( * VInterruptHandler )( 
	Pointer Context );	

UInt32	vintrSharedDataSize ( 
	UInt32 InterruptCount );

TMStatus	vintrManagerCreate ( 
	vintrManagerParameters* Parameters,
	UInt32* VIntrManagerHandle );

TMStatus	vintrManagerDestroy ( 
	UInt32 vintrManagerHandle );

TMStatus	vintrCreate (
	UInt32 VIntrManagerHandle,
	UInt32	VIntrNumber,
	VInterruptHandler Handler, 
	Pointer Context,
	UInt32 *VIntrHandlePointer);

TMStatus	vintrDestroy ( 
	UInt32 VIntrHandle );

TMStatus	vintrGenerateInterrupt ( 
	UInt32 VIntrHandle );

TMStatus	vintrManagerReset ( 
	UInt32 VIntrHandle );

/*
	HAL FUNCTIONS
*/
typedef struct tagHalControl
{
	UInt32	HostInterruptSpinLock;
	UInt32	TargetInterruptSpinLock;

	UInt32	H2TIntReq;
	UInt32	T2TIntReq;
	UInt32	H2TIntAck;
	UInt32	T2HIntAck;

	UInt32	HostPeakTime;
	UInt32	TargetPeakTime;

	UInt32	HostReserved1;
	UInt32	TargetReserved1;
	UInt32	HostReserved2;
	UInt32	TargetReserved2;

}	HalControl;

typedef void    ( * HalInterruptHandler )( 
	Pointer Context );	

TMStatus	halCreate ( 
	Pointer Paramters,
	UInt32* HalHandlePointer );

TMStatus	halDestroy ( 
	UInt32 HalHandle );

TMStatus	halGetMMIOInfo (
	UInt32	HalHandle,
	Pointer *MMIOPhysical, 
	Pointer *MMIOKernelMapped, 
	UInt32 *MMIOSize );

TMStatus	halGetSDRAMInfo ( 
	UInt32	HalHandle,
	Pointer *SDRAMPhysical, 
	Pointer *SDRAMKernelMapped, 
	UInt32 *SDRAMSize );

TMStatus	halGetTMPCIInfo ( 
	UInt32	HalHandle,
	UInt32* DeviceVendorID,
	UInt32* SubsystemID,
	UInt32* ClassRevisionID );

TMStatus	halGetBridgePCIInfo ( 
	UInt32	HalHandle,
	UInt32* DeviceVendorID,
	UInt32* SubsystemID,
	UInt32* ClassRevisionID );

TMStatus	halInstallHandler ( 
	UInt32	HalHandle,
	HalInterruptHandler Handler, 
	Pointer Context );

TMStatus	halRemoveHandler ( 
	UInt32	HalHandle );

TMStatus	halDisableIRQ ( 
	UInt32	HalHandle );

TMStatus	halEnableIRQ ( 
	UInt32	HalHandle );

TMStatus	halGenerateInterrupt ( 
	UInt32	HalHandle );

TMStatus	halAcknowledgeInterrupt (
	UInt32	HalHandle );

TMStatus	halStartDSP (
	UInt32	HalHandle );

TMStatus	halStopDSP (
	UInt32	HalHandle );

TMStatus	halResetDSP (
	UInt32	HalHandle );

TMStatus	halSwapEndianess ( 
	UInt32	HalHandle,
	Bool	SwapEnable );

TMStatus	halGetEndianess ( 
	UInt32	HalHandle,
	Bool*	SwapEnablePtr );

TMStatus	halReset (
	UInt32	HalHandle );

TMStatus	halSetPeerVersion (
	UInt32	HalHandle, 
	UInt32	MajorVersion,
	UInt32	MinorVersion );

TMStatus	halGetPeerVersion (
	UInt32	HalHandle,
	UInt32*	MajorVersion,
	UInt32*	MinorVersion );

void	halCopyback( 
	Pointer CacheBlock, 
	UInt32 BlockCount );

/* Endianess Swapping Functions */
UInt32	halAccess32( 
	UInt32	HalHandle,
	UInt32 volatile Value );

UInt16	halAccess16 ( 
	UInt32	HalHandle,
	UInt16 volatile Value );

void	halAccessEnable ( 
	UInt32	HalHandle );

void	halAccessDisable ( 
	UInt32	HalHandle );

UInt32	halTranslateTargetPhysicalAddress ( 
	UInt32 HalHandle, 
	UInt32 PhysicalAddress );

/*
	MESSAGE FUNCTIONS
*/
typedef struct	tagmessageManagerParameters
{
	UInt32 MessageCount; 
	UInt32 PacketSize;
	UInt32 HalHandle;
	UInt32 ChannelManagerHandle;
	UInt32 NameSpaceManagerHandle;
}	messageManagerParameters;

TMStatus	messageManagerCreate (
	messageManagerParameters*	Parameters,
	UInt32*	MessageManagerHandlePointer );

TMStatus	messageManagerReset ( 
	UInt32 MessageManagerHandle );

TMStatus	messageManagerDestroy ( 
	UInt32 MessageManagerHandle );

TMStatus	messageManagerDestroyMessageByClient ( 
	UInt32 MessageManagerHandle,
	UInt32 ClientHandle );

TMStatus	messageCreate (
	UInt32	MessageManagerHandle,						
	Pointer	ListHead,
	String	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* MessageHandlePointer );

TMStatus	messageSend( 
	UInt32 MessageHandle, 
	Pointer Data );

TMStatus	messageReceive ( 
	UInt32 MessageHandle,
	Pointer Data );

TMStatus	messageBlock ( 
	UInt32 MessageHandle );

TMStatus	messageDestroy (
	UInt32 MessageHandle );
	
//DL Added This function is normally not part of tmman
TMStatus	messageWait( 
	UInt32 MessageHandle);

/*
	EVENT FUNCTIONS
*/

typedef struct	tageventManagerParameters
{
	UInt32	EventCount; 
	UInt32	VIntrManagerHandle;
	UInt32	HalHandle;
	Pointer	SharedData;
	UInt32 NameSpaceManagerHandle;	 
}	eventManagerParameters;

UInt32	eventSharedDataSize ( 
	UInt32 EventCount );

TMStatus	eventManagerCreate ( 
	eventManagerParameters*	Parameters,
	UInt32*	EventManagerHandlePointer );

TMStatus	eventCreate (
	UInt32	EventManagerHandle,						
	Pointer	ListHead,
	String	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* EventHandlePointer );

TMStatus	eventSignal (
	UInt32 EventHandle );

//DL ??
TMStatus eventWait(UInt32 EventHandle);


TMStatus	eventDestroy (
	UInt32 EventHandle );

TMStatus	eventManagerReset (
	UInt32 EventManagerHandle );

TMStatus	eventManagerDestroy (
	UInt32 EventManagerHandle );

TMStatus	eventManagerDestroyEventByClient ( 
	UInt32 EventManagerHandle,
	UInt32 ClientHandle );

/*
	MEMORY FUNCTIONS
*/

typedef struct tagmemoryManagerParameters
{
	UInt32				MemoryCount;
	UInt32				MemorySize;
	Pointer				MemoryBlock;
	Pointer				SharedData; 
	UInt32				HalHandle;
	UInt32 NameSpaceManagerHandle;
}	memoryManagerParameters;

UInt32	memorySharedDataSize ( 
	UInt32 MemoryCount );

TMStatus	memoryManagerReset ( 
	UInt32 MemoryManagerHandle );

TMStatus	memoryManagerCreate (
	memoryManagerParameters*	Parameters,
	UInt32*	MemoryManagerHandlePointer );

TMStatus	memoryManagerDestroy ( 
	UInt32 MemoryManagerHandle );

TMStatus	memoryManagerDestroyMemoryByClient ( 
	UInt32 MemoryManagerHandle,
	UInt32 ClientHandle );

TMStatus	memoryCreate(
	UInt32	MemoryManagerHandle,
	Pointer	ListHead,
	String	Name,
	UInt32	Length, 
	Pointer* AddressPointer,
	UInt32*	MemoryHandlePointer );

TMStatus	memoryDestroy ( 
	UInt32 MemoryHandle );

TMStatus  memoryOpen ( 
	UInt32 MemoryManagerHandle, 
	Pointer	ListHead,
	String Name,
	UInt32* LengthPtr,
	Pointer* AddressPointer,
	UInt32*	MemoryHandlePointer );

TMStatus  memoryClose (
	UInt32	MemoryHandle );

TMStatus	memoryGetAddress ( 
	UInt32 MemoryHandle,
	UInt32* AddressPointer );

TMStatus	memoryGetHalHandle ( 
	UInt32 MemoryHandle,
	UInt32* HalHandle );

/*
	Scatter Gather Buffer Functions
*/

typedef struct tagsgbufferManagerParameters
{
	UInt32		SGBufferCount;
	UInt32		HalHandle;
	UInt32		MemoryManagerHandle;
	UInt32		NameSpaceManagerHandle;
}	sgbufferManagerParameters;

UInt32	sgbufferSharedDataSize ( 
	UInt32 SGBufferCount );

TMStatus	sgbufferManagerReset ( 
	UInt32 SGBufferManagerHandle );

TMStatus	sgbufferManagerCreate (
	sgbufferManagerParameters*	Parameters,
	UInt32*	SGBufferManagerHandlePointer );

TMStatus	sgbufferManagerDestroy ( 
	UInt32	SGBufferManagerHandle );

TMStatus	sgbufferManagerDestroySGBufferByClient ( 
	UInt32 SGBufferManagerHandle,
	UInt32 ClientHandle );

TMStatus	sgbufferCreate (
	UInt32	SGBufferManagerHandle,
	Pointer	ListHead,
	String	Name,
	UInt32	BufferAddress,
	UInt32	BufferSize,
	UInt32	Flags,
	UInt32*	SGBufferHandlePointer );

TMStatus	sgbufferDestroy ( 
	UInt32 SGBufferHandle );

TMStatus	sgbufferOpen ( 
	UInt32	SGBufferManagerHandle, 
	Pointer	ListHead,
    String	Name,
	UInt32*	EntryCountPointer,
	UInt32* SizePointer,
	UInt32*	SGBufferHandlePointer );

TMStatus	sgbufferClose (
	UInt32	SGBufferHandle );

TMStatus	sgbufferFirstBlock ( 
	UInt32	SGBufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer );

TMStatus	sgbufferNextBlock ( 
	UInt32	SGBufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer );

TMStatus	sgbufferCopy ( 
	UInt32	SGBufferHandle,
	UInt32	Offset,
	UInt32	Address, 
	UInt32	Size, 
	UInt32	Direction );

/*
	NAME SPACE FUNCTIONS
*/

typedef struct	tagnamespaceManagerParameters
{
	UInt32	NameSpaceCount; 
	Pointer	SharedData; 
	UInt32	HalHandle;
}	namespaceManagerParameters;

UInt32	namespaceSharedDataSize ( 
	UInt32 NameSpaceCount );

TMStatus	namespaceManagerCreate ( 
	namespaceManagerParameters* Parameters,
	UInt32* NameSpaceManagerHandle );

TMStatus	namespaceManagerDestroy ( 
	UInt32 NameSpaceManagerHandle );

TMStatus	namespaceManagerReset ( 
	UInt32 NameSpaceManagerHandle  );

TMStatus	namespaceCreate (
	UInt32	NameSpaceManagerHandle,						
	UInt32	NameSpaceObjectType,
	String	NameSpaceName,
	UInt32*	ObjectIdPointer,
	UInt32* NameSpaceHandlePointer );

TMStatus	namespaceDestroy ( 
	UInt32 NameSpaceHandle );


/* MISCLANEOUS PLATFORM SPECIFIC FUNCTIONS */
/* 
	Note that these functions do not operate on a specific
	board object and they don't require any specific board 
	information.
*/

/*	DEBUGGING ROUTINES	*/
/*
	Define the shared Debug structure here, since it will
	be shared by the host and the DSP.
*/

typedef struct tagDebugControl
{
	UInt8	szMagic[constDebugMagicSize]; /* starts at 64 byte boundary */
	UInt32	Wrapped;		/* has wrap-around occured */
	UInt8*	AllocBase;		/* ptr to the unaligned memory for malloc & free */
	UInt32	BufLen;		/* length of trace buffer */
	UInt32	BufPos;		/* position in trace buffer */
	UInt8*	Buffer;		/* pointer to trace buffer */
}	DebugControl;

typedef struct tagdebugParameters
{
    UInt32  TraceBufferSize;
    UInt32  LevelBitmap;
    UInt32	Type;
}	debugParameters;

Bool	debugInit(
	/* platform specific implementation */					  
	debugParameters* Parameters );	

// DTO 4/7/99: 
void	debugPrintf( 
	String FormatString,
	... );

Bool	debugExit( 
	void );

UInt32	debugLevel(
	UInt32 LevelBitmap );

Bool	debugCheckLevel(
	UInt32 Level );

Bool	debugDPBuffers (
	UInt32	HalHandle,
	UInt8 **ppFirstHalfPtr, 
	UInt32 *pFirstHalfSize, 
	UInt8 **ppSecondHalfPtr, 
	UInt32 *pSecondHalfSize );

Bool	debugTargetBuffers (
	UInt32	HalHandle,
	UInt8 **ppFirstHalfPtr, 
	UInt32 *pFirstHalfSize, 
	UInt8 **ppSecondHalfPtr, 
	UInt32 *pSecondHalfSize );

/* get the debug buffer on the host(this)processor */
Bool	debugHostBuffers (
	UInt8 **ppFirstHalfPtr, 
	UInt32 *pFirstHalfSize, 
	UInt8 **ppSecondHalfPtr, 
	UInt32 *pSecondHalfSize );


// DTO 4/7/99: 
#ifdef TMMAN_TARGET
//#define	DPF(_l_,_x_)	if(debugCheckLevel(1<<(_l_))) debugPrintf _x_ 
//#define	DPF(_l_,_x_)	debugPrintf _x_
#else
//DL:#define	DPF(_l_,_x_)	debugPrintf _x_
#define DPF(_l_,_x_)	PRINTF _x_  
#endif


/* Ciritcal Section Exclusion Routines */
/*
	NOTE THAT THESE HAVE TO CLAIMED FOR A VERY SHORT TIME.
	SINCE INTERRUPT HANDLERS MAY BE BLOCKING ON THESE TOO.
	while we access data structures we need to protect them
	from the following conditions :
	1.	Another processor ( on NT SMP )
	2.	Another thread on the same processor ( context switch )
	3.	Our own interrupt on the same processor ( peer interrupt )

	Here are the protection mechanism for the above conditions
	TM1
		1. Not applicable
		2. Not applicable
		3. HAL_disable_interrupt - Hal_enable_interrupt
	TM1-pSOS
		1. Not applicable
		2. App_Model_Suspend_Scheduling
		3. HAL_disable_all_interrupts - if we use this 2. is not required
	NTSMP
		1. & 2. & 3.KeAcquireSpinLock - KeReleaseSpinLock running at DISPATCH_LEVEL

	Win9X	
		1. Not applicable
		2. & 3. pushf cli - popf

	Win32.
		1. Not applicable
		2. & 3.EnterCriticalSection & LeaveCriticalSection
*/
Bool	critsectCreate ( 
	UInt32* CriticalSectionHandlePointer );

Bool	critsectDestroy ( 
	UInt32 CriticalSectionHandle );

Bool	critsectEnter ( 
	UInt32 CriticalSectionHandle, Pointer NestedContext );

Bool	critsectLeave ( 
	UInt32 CriticalSectionHandle, Pointer NestedContext );

/* Synchronization Object Abstraction Functions */
Bool	syncobjCreate ( 
	UInt32 SynchronizationFlags,
	UInt32	OSSynchronizationHandle,
	UInt32 *SynchronizationHandlePointer,
	String  SynchronizationObjectName );

Bool	syncobjSignal ( 
	UInt32 SynchronizationHandle );

Bool	syncobjBlock ( 
	UInt32 SynchronizationHandle );
//DL ??
Bool    syncobjWait( UInt32 SynchronizationHandle);

Bool	syncobjDestroy ( 
	UInt32 SynchronizationHandle );

/*
	FIFO manipulation routines 
*/

/* Queue Abstraction Function - Genrcic Implementation */
Bool	queueCreate ( 
	UInt32	ItemSize, 
	UInt32	HalHandle,
	UInt32* QueueHandlePointer );

Bool	queueDestroy ( 
	UInt32 QueueHandle );

/* 
	retrieves an element without deleting it 
	successive calls the queueRetrieve without calling
	queueDelete returns the same result
*/
Bool	queueRetrieve ( 
	UInt32 QueueHandle, 
	Pointer Item );

Bool	queueInsert ( 
	UInt32 QueueHandle, 
	Pointer Item );

/* destructive retrieve */
Bool	queueDelete ( 
	UInt32 QueueHandle, 
	Pointer Item );

/* page table manipulation functions */
typedef struct	tagPageTableEntry 
{
	UInt32		PhysicalAddress;
	UInt32		RunLength;
}	PageTableEntry;

/*
pagetableGetTempBufferSize

PARAMETERS :

	BufferDescription	
		Pointer to the platform dependent structure that describes the 
		buffer that needs to be page locked and the page table needs to be
		passed to the target.

RETURNS :
	The number of bytes of temporary buffer space require to perform the 
	page locking and retrieve the page table entries.

*/

UInt32	pagetableGetTempBufferSize ( 
	UInt32 BufferAddress, UInt32 BufferSize );
/*
pagetableCreate

PARAMETERS :

	BufferDescription
		Pointer to the platform dependent structure that describes the 
		buffer that needs to be page locked and the page table needs to be
		passed to the target. Note that in case of NT this buffer
		contains a Pointer to the Irp, in case of 95 it contains the 
		BufferAddress and the Size.

	TempBuffer
		Caller allocated this buffer of the size returned by 
		pagetableGetTempBufferSize(). The allocated buffer
		can be freed after the page table has been copied to the
		shared address space.

	PageTablePointer 
		Address of the variable that receives the local page table base 
		address.
		Note that this is different from TempBuffer since , platform specific
		implementations may require to store persistent infomration  as a headre
		before the actual scatter gather table starts. However *PageTablePointer
		will always point to a location within the TempBuffer.

	PageTableEntryCountPointer
		Address of the variable that receives the number of valid page
		table entries starting from *PageTablePointer

	BufferSizePointer
		Address of the variable that receives the Size of the buffer that 
		was just locked. This parameter is passed back because the 
		BufferDescription is assumed to be transparent to sgbuffer.

	BufferAddressPointer
		Address of the variable that receives the address of the buffer that 
		was just locked. This parameter is passed back because the 
		BufferDexcription is assumed to be transparent to sgbuffer.

	PageTableHandlePointer
		Address of the variable that receives the hadnle to the page table 
		that was just allocated. This parameter is required to free the
		page table.
*/
Bool	pagetableCreate ( 
	UInt32 BufferAddress,
	UInt32 BufferSize,
	Pointer TempBuffer,
	UInt32	HalHandle,
	PageTableEntry	**PageTablePointer,
	UInt32 *PageTableEntryCountPointer,
	UInt32 *PageTableHandlePointer );

Bool	pagetableDestroy ( 
	UInt32 PageTableHandle );

	
/*
	MISC C RUN TIME ROUTINES
*/
Pointer	memAllocate( 
	UInt32 Size );

void	memFree( 
	Pointer Memory );

void	memCopy( 
	Pointer Destination, 
	Pointer Source, 
	UInt32 Size );

void    memSet ( 
	Pointer Memory, 
	UInt8  Value, 
	UInt32 Size );

UInt32	strCmp ( 
	Pointer String1, 
	Pointer String2 );

Pointer	strCopy ( 
	Pointer Destination, 
	Pointer Source );

UInt32	strLen ( 
	Pointer String );


/*
	OBJECT LIST MANIPULATION FUNCTIONS 
*/
UInt32 objectlistCreate ( 
	Pointer ObjectListPointer, 
	UInt32 Count );

void objectlistDestroy ( 
	Pointer ObjectListPointer );

UInt32 objectlistInsert ( 
	Pointer ObjectListPointer, 
	Pointer Object,
	UInt32	Number );

Pointer objectlistGetObject (
	Pointer ObjectListPointer, 
	UInt32	Number );

void objectlistDelete ( 
	Pointer ObjectListPointer, 
	Pointer Object,
	UInt32 Number );

/*
	GENERIC OBJECT MANIPULATION 
*/
Pointer	objectAllocate ( 
	UInt32 Size, 
	UInt32 FourCCID );

UInt32	objectValidate ( 
	Pointer Object, 
	UInt32 FourCCID );

void	objectFree ( 
	Pointer Object );


/* DOUBLY LINKED LIST MANIPULATION FUNCTIONS */

typedef struct tagListStruct 
{
   struct	tagListStruct * volatile Flink;
   struct	tagListStruct * volatile Blink;
} ListStruct;


void listInitializeHead ( 
	ListStruct* ListHead );

Bool listIsEmpty (
	ListStruct* ListHead );

ListStruct* listRemoveHead (
	ListStruct* ListHead );

ListStruct* listRemoveTail (
	ListStruct* ListHead );

void listRemoveEntry(
	ListStruct* Entry);

void listInsertTail (
	ListStruct* ListHead,
	ListStruct* Entry ); 

void listInsertHead (
	ListStruct* ListHead,
	ListStruct* Entry );


#endif	/* _TMMANLIB_H_ */
















