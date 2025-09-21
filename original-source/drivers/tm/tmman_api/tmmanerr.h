/*
	(c) Philips Semiconductors - TriMedia Product Group 1997

	TriMedia Manager Status Codes

	FILE	TMManErr.h

	Author : T.Roy (Tilakraj.Roychoudhury@sv.sc.philips.com)

*/

#ifndef _TMMANERR_H_
#define	_TMMANERR_H_

/* 
	Generic ErrorCodes
*/
#ifndef	TMStatus
#define	TMStatus								UInt32
#endif


#ifndef	IN
#define	IN
#endif

#ifndef	OUT
#define	OUT
#endif

#ifndef	statusSuccess
#define	statusSuccess								0x0000
#endif

#define tmmanTMManStatusToDownloaderStatus(x) (((x)!=statusSuccess)?(x-0x70):statusSuccess)

/*
	COMPONENT SPECIFIC CODES
*/
/* 
	User supplied object handle is invalid or the object is corrupted 
*/
#define	statusInvalidHandle							0x0001

/* 
	Dynamic allocation of the given object failed 
*/
#define	statusObjectAllocFail						0x0002

/* 
	Dynamic allocation of the object list structure failed 
*/
#define	statusObjectListAllocFail					0x0003

/* 
	Object insertion failed - count out of range or slot is already occupied 
*/
#define	statusObjectInsertFail						0x0004

/* 
	Function is supported by this platform but has not been implemented 
*/
#define	statusNotImplemented						0x0005

/* 
	Function is not supported by this platform 
*/
#define	statusUnsupportedOnThisPlatform				0x0006

/* 
	DSP Number exceeds the number of TriMedia processors in system 
*/
#define	statusDSPNumberOutofRange					0x0007

/* 
	Major version number is incompatible with this version on TMMan 
*/
#define	statusMajorVersionError						0x0008

/*
	Minor version number is incompatible with this version of TMMan 
*/
#define	statusMinorVersionError						0x0009

/*
	User to Kernel Mode call failed - internal error
*/
#define	statusDeviceIoCtlFail						0x000a

/*
	Memory for the executable file image couldn't be allocated  
*/
#define	statusImageMemoryAllocationFail				0x000b

/*
	Module ID of the component is not recognized
*/
#define	statusUnknownComponent						0x000c

/*
	Critical Section Creation failed - due ot lack of system resources
*/
#define statusCriticalSectionCreateFail				0x000d

/*
	The executable boot image does not match the endianness of the system
*/
#define statusExecutableFileWrongEndianness			0x000e

/*
	Creation of Synchronization Object failed - 
	due to user parameter error or resource limitations
*/
#define statusSynchronizationObjectCreateFail		0x000f

/*
	Creation of Queue Object failed - 
	due to resource limitations
*/
#define statusQueueObjectCreateFail					0x0010

/*
	Write to PCI Config Space Access Failed
*/
#define statusPCIConfigAccessFail					0x0011

/*
	String parameter passed to a function caused a buffer overflow.
*/
#define statusStringBufferOverflow					0x0012

/*
	Incorrect parameter was passed to a function.
*/
#define statusIncorrectParameter					0x0013

/*
	Device has been removed or stopped via PnP.
*/
#define statusHardwareUnavailable					0x0014

/*
	Mapping Memory failed due to lack of free Virtual Address Space.
*/
#define statusOutOfVirtualAddresses					0x0015




/*
	Initialization of Hal failed due to -
	Lack of resources
	Invalid Paremters passed to Hal.
	This is an internal error.
*/

#define statusHalInitializationFail					0x0031

/*
	Interrupt vector is already in use.
*/
#define	statusHalAllocInterruptFail					0x0032


/*
	Interrupt could not be registered	
*/
#define	statusHalConnectInterruptFail				0x0033



#define	statusEventInterruptRegistrationFail		0x0041
/*
	No more shared memory is available in the system,
	to stisfy the request.
*/
#define	statusMemoryUnavailable						0x0051


/*
	There are no more packets to be retrieved from this queue 
*/
#define	statusMessageQueueEmptyError				0x0061


/*
	For the explanation of these error codes look at file TMDownLoader.h 
	use the macro tmmanTMManStatusToDownloaderStatus() for tranalting
	tmman error codes to downloader error codes.
*/
#define statusDownloaderErrorStart					0x0071
#define statusDownloaderErrorEnd					0x008f


/*
	No Debug Information ( Magic String ) was found in SDRAM.
	Either the target did not execute far enough to generate the magic string
	The String was generated but was corrupted.
	SDRAM Persistent information was lost due to prolonged power down.

*/
#define statusDebugNoDebugInformation			0x0091


/*
	All available name space slots are in use.
	Every Resource in TMMan that is shared across the bus uses NameSlots.
*/
#define statusNameSpaceNoMoreSlots					0x00a1

/*
	User supplied name for the object exceeds TMMan impose dlimitation of 12 characters
*/
#define statusNameSpaceLengthExceeded				0x00a2

/*
	User tried to create ( not open ) a name space name that already exists
*/
#define statusNameSpaceNameConflict					0x00a3

/*
	User tried to open a name space that does not exist 
*/
#define statusNameSpaceNameNonExistent				0x00a4



/*
	Virtual Interrupt Object - Memory Allocation Failed 
*/
#define	statusVIntrObjectAllocFail					0x00b1



/*
	Out of Scatter Gather Buffer Slots.
*/
#define	statusSGBufferNoMoreEntries					0x00c1


/*
	Buffer Offset specified to the command falls outside the
	buffers range.
*/
#define	statusSGBufferOffsetOutOfRange				0x00c2


/*
	The size specified falls outside the buffer range calculated
	from the given offset.
*/
#define	statusSGBufferSizeOutOfRange				0x00c3


/*
	Allocation of temporary page table buffer failed.
*/
#define	statusSGBufferTempPageAllocFail				0x00c4


/*
	Attempt to page lock the memory failed.
*/
#define	statusSGBufferPageLockingFail				0x00c5


/*
	Attempt to page lock the memory failed.
*/
#define	statusSGBufferPageTableSizeFail				0x00c6


/*
	Attempt to page lock the memory failed.
*/
#define	statusSGBufferInvalidPageTable				0x00c7


/*
	Channel Manager - Virutal Interrupt  Registration Failed

*/
#define statusChannelInterruptRegistrationFail		0x00d1

/*
	Interproecssor Mailbox is temporarily full -
	This is a contemporary error message, the same call may succeed 
	if tried again.
*/
#define	statusChannelMailboxFullError				0x00d2

/*
	TMMan internal error 
*/
#define statusUnknownErrorCode						0xf000


#endif	/* _TMMANERR_H_ */
