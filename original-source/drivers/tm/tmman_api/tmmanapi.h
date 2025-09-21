/*
	(c) Philips Semiconductors - TriMedia Product Group 1999

	TriMedia Manager Application Programming Interface

	FILE	TMManAPI.h

	Author : T.Roy (Tilakraj.Roychoudhury@sv.sc.philips.com)

	Histroy
		970613	T.Roy		Second Revision
		980601	T.Roy		Documentation Updates
		981207	T.Roy		Added WinCE & Speculative Load stuff.
		990201	T.Roy		Added section on SDRAM Mapping and Synch Flags.
        990511  DTO         Ported to pSOS

*/
/*
	IMPLEMENTATION NOTES 

	Throughout this header file function descriptions refer to these
	NOTES.

	Synchronization Handle

		The caller creates these handles via calls to the operating system
		specific functions like CreateEvent() or AppSem_create() etc. The
		caller is also responsible for freeing these handles. The following
		section lists the operating systems supported by TMMan, the 
		functions used by the caller to allocate these handles 
		and the functions TMMan uses to signal these handles. 

		Win95 Kernel Mode
			Creation - CreateEvent()
			Signaling - VWIN32_SetWin32Event()
			Closing - CloseHandle()

		WinNT KernelMode
			Creation - CreateEvent()
			Signaling - KeSignalEvent()
			Closing - CloseHandle()

		pSOS
			Creation - AppSem_create() + AppSem_p()
			Signaling - AppSem_v
			Closing - AppSem_delete()

		Standalone ( no oprating system )
			Creation - AppSem_create() + AppSem_p()
			Signaling - AppSem_v
			Closing - AppSem_delete()

		
		Note that under Windows operating systems 
		(WinNT, Win95, Win98, WinCE) this event 
		has to be created as an Auto Reset Event.


	Synchronization Flags

		constTMManModuleHostKernel
			Indicates that the Host module calling the required function is 
			running in Kernel Mode. If this flag is specified TMMan 
			interprets the SynchronizationHandle parameter as a handle
			to a Win32 Synchroniztion Object. Typically WinNT/Win9X Device 
			Drivers.
			
		constTMManModuleHostUser
			Indicates that the Host module calling the required function 
			running in User Mode. Typically WinNT/Win9X/WinCE DLLs or Applications.

	
		constTMManModuleTargetKernel
			On the target there is no distinction between user and kernel mode.
			If this flag is specified TMMan interprets SynchronizationHandle as
			an AppSem type of synchronization object. 
	
	
		constTMManModuleTargetUser
			On the target there is no distinction between user and kernel mode.
			If this flag is specified TMMan interprets SynchronizationHandle as
			an AppSem type of synchronization object. 


	Object Names

		Every TMMan object has a name associated with it. This name is used
		to form a binding between the host and target counterparts of the 
		objects. This object name is a unique user supplied name that can 
		be 12 characters long (maximum). The names are case sensitive.
		The host has to create the named object before the target can find 
		it otherwise the named object creation on the target will fail.
		These names do not have to be unique across objects. i.e. an
		event and a message channel can use the same name.


	Scatter Gather Buffer Locking 

		The tmmanSGBufferXXX functions are applicable to systems in which the
		host processor supports virtual memory.	If an application running on 
		the host allocates a buffer which the target processor needs to access
		(read from or write to ), scatter gather locking has to be performed.
		Scatter Gather locking a buffer ensures that the memory allocated to 
		that buffer does not get paged out. This locking operation also 
		generates a scatter gather list that is used by the target to access
		the memory allocated to the buffer which is fragmented in physical 
		address space. 

	Debug Buffer Pointers

  		The tmmanDebugXXXBuffers functions return 2 sets of pointers and sizes.
		These pointers point to a circular buffer on in SDRAM or in PC memory.
		These pointers that track the current state of the debug buffers are 
		constantly changing. This function returns a snapshot of these pointers.
		The contents of the circular wrap around buffer has to be accessed in
		two parts via the FirstHalfPtr and the  SecondHalfPtr. If the buffer
		has not wrapped around at the instance this call is made the 
		FirstHalfPtr will we Null. And only the SecondHalfPtr will point to 
		valid contents. To print the contents of the buffer the code should 
		look like this.

		if ( FirstHalfPtr )
		{
			Print ( FirstHalfPtr, FirstHalfBufferSize );
		}

		Print ( SecondHalfPtr, SecondHalfBufferSize );

	Speculatvie Load Fix

		The TriMedia compiler supports speculative loading i.e. values in 
		registers are used as pointers and are dereferenced to load data 
		from memory in advance. 

		Speculative load can lead to loads from SDRAM MMIO or over the PCI bus. 
		On certain Pentium II machines having the Intel 440LX and the 440BX 
		chipsets load access across the PCI bus cause the machine to lock up. 
		These PCI accesses could be targeted at PC memory, adapter memory 
		(like VGA frame buffer), or unclaimed PCI physical address space.

		The TriMedia processor has a hardware feature that allows all PCI 
		access (expect SDRAM and MMIO accesses) to be disabled. This feature 
		can be enabled and disabled at run time. 

		The TriMedia Manager disables PCI accesses at startup. However when 
		it needs to access PC memory for host communication functions, 
		it enables PCI accesses, performs the accesses and then disables 
		PCI accesses.

		Similarly User programs that allocate and use shared memory should 
		use the pciMemoryReadUIntXX() & pciMemoryWriteUIntXX() functions 
		to access shared memory on the host, instead of reading and writing
		memory by de-referencing pointers directly. These functions are
		documented in TCS\include\tm1\tmPCI.h.

		
		The option (Speculative Load Fix) is turned off by default, it can 
		be turned on by the following registry entry.

		[HKEY_LOCAL_MACHINE\\SOFTWARE\\PhilipsSemiconductors\\TriMedia\\TMMan]
		"SpeculativeLoadFix"=dword:00000001

		Existing application that use shared memory will break when this 
		fix is turned on.


	Big Endian Execution
		The TriMedia processor can execute in both big endian mode and little 
		endian mode. TMMan supports execution of little endian as well as big 
		endian executables on the TriMedia processor. 

		The host processor on the PC always executes in little endian mode 
		however the TriMedia processor can execute in big endian mode. 
		In such a case every access made to shared memory have to be swapped 
		to maintain data consistency. 

		The TriMedia processor always accesses data in native endianess i.e. 
		in either big endian or little endian mode. It is the host 
		applications responsibility to swap data types if the target is 
		running is an endianess different than the host. The example 
		programs i.e. memory, message, tmapi, sgbuffer take care of 
		endianess issues using macros for SWAPPING. Other user applicaitons 
		should do the same.

		To Execute Big Endian Examples set the following flag in 
		%windir%\\tmman.ini
		
		[TMMan]
		Endian=1 ; LITTLE Endian - This is the default
		Endian=0 ; BIG Endian

		The machine need not be rebooted after this change.

	WinCE Issues

		Under other Win32 platforms like WinNT, Win95 & Win98, Kernel Mode 
		Drivers can signal user mode event via handles. Under WinCE the 
		TMMan Drivers run in user mode within the driver.exe process. 
		The only way it can set an event in user mode is by obtaining a 
		handle to the event via a same name, that the user application, used
		to create the event. Hence applicaitons that, use TMMan Event and 
		Messaging Interfaces, and run under Win CE have to take care of the
		following :

		All Win32 events have to be named events.
		All event names have to be unique, even across applications. As 
		events are created in the global Win32 namespace.

		Note that these restrictions do NOT apply to other Win32 platforms.


	StatusCodes

		All the TMManAPI functions return statusSuccess on successful 
		completion.	Callers can retrieve a textual description of the 
		failure codes by calling tmmanGetErrorString. All error codes are 
		documented in the file TMManErr.h.


	SDRAM Mapping

		Due to limited amount of virtual address space on some Windows 
		Platforms, simultaneous mapping of SDRAM and MMIO spaces of 
		multiple TM processors may fail. A solution to this problem
		is to defer the mapping of SDRAM till the time when it will be 
		actually used and then unmapping it immediately so as to free
		Virtual Address Space for mapping other SDRAMs of the other
		TriMedia processors in that machine. For this purpose, two
		API calls have been added tmmanDSPMapSDRAM, tmmanDSPUnmapSDRAM.
		To disable SDRAM mapping at initialization ( default is to map
		all of SDRAM ) the following registry key has to be set :
		HKLM\SOFTWARE\PhilipsSemiconductors\TriMedia\TMMan
		MapSDRAM=0
		When SDRAM MApping is disabled, calls to tmmanDSPDSPInfo will 
		return invalid values in the tmmanMemoryBlock.SDRAM.MappedAddress 
		field. Other calls that make use of this field will also fail, i.e.
		tmmanMappedToPhysical
		tmmanPhysicalToMapped
		tmmanValidateAddressAndLength
		tmmanTranslateAdapterAddress
		tmmanDebugDPBuffers
		tmmanDebugTargetBuffers	
		Hence calls to these functions have to be wrapped with calls to 
		tmmanDSPMapSDRAM and tmmanDSPUnmapSDRAM.
		Also note the c Run Time library server DLL ( tmcrt.dll ) will not
		work anymore since it depends on the entire SDRAM to be accessible 
		all the times. So target executables have to be compiled with -host
		nohost or a dummy version of the host_comm.o have to be linked in
		if -host Windows is used. Also note that TriMedia DLLs  will not 
		work since loading DLLs require file I/O.
*/

#ifndef  _TMMANAPI_H_
#define  _TMMANAPI_H_
	
/*-----------------------------includes-------------------------------------*/

#ifdef	__TCS__
#include <tmlib/tmtypes.h>
#include <WinNT/tmmanerr.h>
#else
#include "tmtypes.h"
#include "tmmanerr.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------defines---------------------------------------*/

#ifndef	constTMManDefault
#define constTMManDefault			(0xffffffff)
#endif

/* 
	maximum boards in one system 
*/
#define	constTMMANMaximumDeviceCount	10 


/* 
	maximum number of processes that can access tmman simultaneously 
*/
#define	constTMMANMaximumClientCount	20 


#define constTMManModuleHostKernel		(0x1)

#define constTMManModuleHostUser		(0x2)

/*
	Applications should not use this flag, 
	it is for internal use of TMMan only.
*/
#define constTMManModuleHostCallabck	(0x3)

#define constTMManModuleTargetKernel	(0x4)

#define constTMManModuleTargetUser		(0x5)

/*
	Applicaitons should not use this flag, 
	it is for internal use of TMMan only.
*/
#define constTMManModuleTargetCallback	(0x6)


/* TMMan Major  Version - compatible modules have the same version */
#define constTMManDefaultVersionMajor				5
/* TMMan Minor  Version - compatible modules have the same version */
#define constTMManDefaultVersionMinor				2


/* Status of the TriMedia Processor cannot be determined */ 
#define constTMManDSPStatusUnknown			0
/* TriMedia processor is in reset state - not executing any code */
#define constTMManDSPStatusReset			1
/* TriMedia processor is out of reset - executing code */
#define constTMManDSPStatusRunning			2

/* Target Executable Endianess unknown */
#define	constTMManEndianessUnknown		0x0
/* Target Executable - Little Endianess - Intel Comaptible */
#define	constTMManEndianessLittle		0x1
/* Target Executable - Big Endianess */
#define	constTMManEndianessBig			0x2

/*
	compile time tunable parameters

	IMPORTANT : if any of the following parameters are changed
	tmman and all applications that use tmman including the 
	C Run Time Library Server has to be recompiled 
*/

/* number of arguments that can be passed via the tmmanPacket structure */
#define	constTMManPacketArgumentCount		0x5	

/* internal space allocated for storing names and paths */
#define	constTMManPathLength				256
#define	constTMManStringLength				256
#define	constTMManDebugStringLength			1024

/* maximum number of cmd line args */
#define	constTMManMaximumArgumentCount		128		

/* maximum total cmd line arg buffer size */
#define constTMManMaximumArgumentBuffer		1024	

/* maximum number of TriMedia processors supported */
#define	constTMManBoardCount				constTMMANMaximumDeviceCount

#define constTMManNameSpaceNameLength		16


/* 
	run time tunable parameters

	These parameters will be read by TMMan at the time of starting up.
	values shown here are default values.
	On a Windows machine these parameters will be read form the registry.
*/

/* default shared memory size */
#define	constTMManMemorySize				0x10000	
/* default maximum number of interprocessor mailboxes */
#define	constTMManMailboxCount				0x40
/* default maximum number of interprocessor channels */
#define	constTMManChannelCount				0x10
/* default maximum number of virtual (multiplexed) interrupt lines */
#define	constTMManVIntrCount				0x04
/* default maximum number of message queues <=  constTMManChannelCount */
#define	constTMManMessageCount				0x10
/* default maximum number of interprocessor events */
#define	constTMManEventCount				0x10
/* default maximum number of streams - not yet supported */
#define	constTMManStreamCount				0x10
/* default maximum number is namespace slots that can be used */
#define	constTMManNameSpaceCount			0x40
/* default maximum number of shared memory blocks that can be allocaed */
#define	constTMManMemoryCount				0x40
/* default maximum number of buffer that can scatter gather locked */
#define	constTMManSGBufferCount				0x20


#define constTMManRegistryPath				TEXT("SOFTWARE\\PhilipsSemiconductors\\TriMedia\\TMMan")

/*------------------------------types---------------------------------------*/

/*
		DATA STRUCTURES
*/
typedef struct	tagtmmanPacket
{
	/* application specific arguments - TMMan does not modify or interpret these */
	UInt32	Argument[constTMManPacketArgumentCount];

	/* DO NOT USE THIS FIELD - TMMan overwrites it */
	unsigned long Reserved; 

}	tmmanPacket;

#define	constTMManPacketSize				sizeof(tmmanPacket) 

typedef struct	tagtmmanVersion
{
	/* Major Version number of the specified TMMan component */
	UInt32	Major;
	
	/* Minor Version Number of the specified TMMan component */
	UInt32	Minor;
	
	/* Build version number of the specified TMMan component */
	UInt32	Build;

}	tmmanVersion;

typedef struct	tagtmmanMemoryBlock
{

	/* Operating System Mapped Address corresponding to PhysicalAddress */
	UInt32	MappedAddress;

	/* Physical Address of the SDRAM or MMIO Window */
	UInt32	PhysicalAddress;

	/* Size of the SDRAM or MMIO Window */
	UInt32	Size;

}	tmmanMemoryBlock;

typedef struct	tagtmmanDSPInfo
{

	/* Address information about SDRAM */
	tmmanMemoryBlock	SDRAM;
	
	/* Address information about MMIO */
	tmmanMemoryBlock	MMIO;

	/* Version of the CPU - PCIConfig->RevisionID field */
	/*UInt32			CPUVersion; - old fields don't use */

	/* TriMedia PCI Class and Revision ID - for CPU version*/
	UInt32				TMClassRevisionID;

	/* Version of the board - PCIConfig->SubSystemVendor ID */
	/*UInt32			BoardVersion; - old fields don't use*/

	/* TriMedia PCI Subsystem & Subsystem Vendor ID - same as Board Revision */
	UInt32				TMSubSystemID;

	/* DSP Number - depends on the order this device was found on the PCI bus */
	UInt32				DSPNumber;


	/* TriMedia PCI Device and Vendor ID - TM1000 / TM2000 support */
	UInt32				TMDeviceVendorID;

	/* Bridge PCI Device and Vendor ID - non-transparent bridge support */
	UInt32				BridgeDeviceVendorID;

	/* Dridge PCI Class and Revision ID - for CPU version*/
	UInt32				BridgeClassRevisionID;

	/* Bridge PCI Subsystem & Subsystem Vendor ID - non-transparent bridge support */
	UInt32				BridgeSubsystemID;

	/* Reserved for future use */
	UInt32				Reserved[8];

}	tmmanDSPInfo;


/* this structure is used internally by tmman - not for applications use */
typedef struct	tagtmmanDSPInternalInfo
{
	/* only this field can be set from user mode */
	UInt32	Flags; 
	UInt32	CRunTimeHandle;
	UInt32	DebugOptionBitmap;

	/* all the fields below are read only */

	/* debugging */

	/* peer version information */
	UInt32	PeerMajorVersion;
	UInt32	PeerMinorVersion;

	/* required for symbol patching */
	UInt32	TMManSharedPhysicalAddress;

	tmmanMemoryBlock	Memory;

}	tmmanDSPInternalInfo;




/*
		FUNCTION PROTOTYPES
*/



/*

tmmanNegotiateVersion

	Called by the application to perform a version negotiation with 
	the different components of TMMan. The application should fill 
	up the fields of the version structure with the 
	constTMManDefaultVersionXXXX constants defined in this file, before 
	calling the function. If TMMan cannot handle the version passed in 
	this structure it will return an error. Otherwise it will return 
	a success status. Note that both in case of a failure or success
	TMMan will write its current version information in the structure
	pointed to by the version parameter.

	An application can restrict it self to run with a SPECIFIC VERSION
	of TMMan by doing either on the following.
	Not Proceeding of this function returns a failure
	OR
	Not proceeding based on the TMMan version returned by this function.

Paremters :

	ModuleID
		Module Identification of the TMMan component whose version needs to be
		verified.
		constTMManModuleHostKernel
		constTMManModuleHostUser
		constTMManModuleTargetKernel
		constTMManModuleTargetUser

	Version
		Pointer to the tmmanVersion  structure with its Major and Minor fields 
		filled up.

Return :
	statusMajorVersionError
		Caller provided major version was less than the major version of the
		given module.

	statusMinorVersionError
  		Caller provided minor version was less than the minor version of the
		given module.

	statusUnknownComponent
		Caller provided ModuleID was outside the range suported on this 
		platform. i.e. if this function is called on the host with 
		constTMManModuleTargetKernel or constTMManModuleTargetUser.


*/
TMStatus  tmmanNegotiateVersion ( 
	UInt32	ModuleID, 
	tmmanVersion* Version );

/*
tmmanGetErrorString

	Returns the string corresponding to the specified error code.
	
Parametrers :

	StatusCode
		Status Code that needs to be converted to a string.

Returns :

	Pointer
		Pointer to a Null terminated string describing the error.

	Null
		If the error code is invalid.
*/
String tmmanGetErrorString (
	TMStatus	StatusCode );




/*

tmmanDSPGetNum

	Returns the number of TriMedia processors installed in the system.
	
Parameters :
	
	None

Return :

	Number of TriMedia processors installed in the system.

*/
UInt32		tmmanDSPGetNum ( 
	void );




/* DSP Interfaces */
/*
tmmanDSPGetInfo
	
	Retrieves the properties of the specified TriMedia processor.

Parameters :
	
	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

	DSPInfo
		Pointer to the structure where the TriMedia processor related 
		information will be returned.
	
Return:

	statusInvalidHandle
			Handle to the DSP is corrupted.

*/
TMStatus  tmmanDSPGetInfo ( 
	UInt32 DSPHandle, 
	tmmanDSPInfo* DSPInfo );


/*
tmmanDSPGetStatus

	Get the current state of the specified TriMedia Processor

Parameters :
	
	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

	StatusFlags
		Pointer to the location where the status flags will be stored.
		The status flags can be one of the following.

		constTMManDSPStatusUnknown
		constTMManDSPStatusReset
		constTMManDSPStatusRunning

Return :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.
*/
TMStatus  tmmanDSPGetStatus ( 
	UInt32 DSPHandle, 
	UInt32* StatusFlags );


/*
tmmanDSPMapSDRAM

	Maps SDRAM into Operating System and Process virtual address space
	See NOTES on SDRAM Mapping.

Parameters :
	
	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

Return :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusOutOfVirtualAddresses
		There are no more free Page Table Entries to map this memory.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.
*/
TMStatus  tmmanDSPMapSDRAM ( 
	UInt32 DSPHandle );



/*
tmmanDSPUnmapSDRAM

	Unmaps SDRAM from Process virtual address space. If all instances 
	of  SDRAM for this processor have been unmapped then the OS 
	mapping is also undone.	See NOTES on SDRAM Mapping. Note that
	tmmanDSPMapSDRAM and tmmanDSPUnmapSDRAM have to be called in pairs.

Parameters :
	
	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 
Return :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.
*/
TMStatus  tmmanDSPUnmapSDRAM ( 
	UInt32 DSPHandle );


/*
tmmanDSPGetEndianess

	Get the current endianess of the specified TriMedia Processor

Parameters :
	
	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

	EndianessFlags
		Pointer to the location where the endianness flags will be stored.
		The endianness flags can be one of the following.

		constTMManEndianessUnknown
		constTMManEndianessLittle
		constTMManEndianessBig

Return :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.
*/
TMStatus  tmmanDSPGetEndianess ( 
	UInt32 DSPHandle, 
	UInt32* EndianessFlags );


/*
tmmanDSPOpen

	Opens the given TriMedia Processor. This call simply increments an 
	internal reference count. It does not perform physical detection 
	of the processor. All TriMedia processors are detected when TMMan 
	is loaded.

Parameters :

	DSPNumber 
		Number of the TriMedia processor that needs to be opened. Note 
		that this count reflects the order in which the TriMedia processor
		was detected by TMMan. This is generally dependent on the PCI
		slot in which the TriMedia board is sitting.

	DSPHandlePointer
		Address of the memory location where the handle to the DSP will
		be stored. All future references to the board have to be made via
		the handle.

Returns :

	statusDSPNumberOutofRange
		The DSPNumber parameter does not lie within 0 and 
		tmmanDSPGetNum() - 1.

*/
TMStatus  tmmanDSPOpen ( 
	UInt32 DSPNumber, 
	UInt32* DSPHandlePointer );


/*
tmmanDSPClose

	Closes the given handle to the TriMedia processor. This call simply 
	decrements an internal  reference count. The caller	will be able 
	to use the handle even after closing it. The handle to the DSP 
	remains valid as long as the TriMedia processor, the handle refers 
	to, exists in the system.


Parametrs :
	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().

Returns :
	statusInvalidHandle	
		Handle to the DSP is corrupted.

*/
TMStatus  tmmanDSPClose ( 
	UInt32 DSPHandle );


/*
tmmanDSPLoad

	Load a boot image onto the DSP. This image has to be compiled with
	the -btype boot flag.

Parametrs :
	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	LoadAddress
		Address of SDRAM where the executable should be downloaded.
		To use the default values use constTMManDefault.

	ImagePath
		Path to the executable file image. This image should have 
		a boot image not a task.

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

	statusOutOfVirtualAddresses
		There are no more free Page Table Entries to map SDRAM for
		image download.

	statusExecutableFileWrongEndianness
		The endianness of the excutable file is not the same as that
		specified in the INI file or registry.

	statusDownloaderXXX
		Range of TMDownloader error codes. For explanation of these error
		codes refer to TMDownloader.h

*/
TMStatus  tmmanDSPLoad ( 
	UInt32 DSPHandle, 
	UInt32 LoadAddress,
	UInt8* ImagePath );


/*
tmmanDSPStart

	This function just unresets the DSP. The DSP starts executing 
	code at SDRAM base. See NOTES on C Run Time.

Parametrs :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanDSPStart ( 
	UInt32 DSPHandle );

/*
tmmanDSPStop

	Puts the CPU into a reset state. 
	Resets all the peripherals via MMIO registers.
	Resets shared data structures TMMan uses across the bus.

Parameters :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanDSPStop ( 
	UInt32 DSPHandle );

/*
tmmanDSPReset

	Initializes the TriMedia Processor after it has been manually reset 
	via the reset button or	other means. 
	This function can additionally preform a hardware reset of the TriMedia
	processor provided the necessary hardware modifications have been made
	to the IREF board.

Parameters :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanDSPReset ( 
	UInt32 DSPHandle );


/* ADDRESS TRANSLATION MACROS */

/*
tmmanMappedToPhysical

	Translates a Operating System Mapped SDRAM or MMIO address to a 
	physical address. This function can translate SDRAM and MMIO address
	only.

Parameters :

	MemoryBlock
		Pointer to a SDRAM or MMIO memory block structure, that will be used 
		for translating the address. The contents of this structure can be 
		retrieved by calling tmmanDSPGetInfo.

	MappedAddress
		The platform specific translated ( mapped ) address.

Return :

	Returns the physical address corresponding to the given mapped address.

*/
UInt32	tmmanMappedToPhysical ( 
	tmmanMemoryBlock* MemoryBlock, 
	UInt32 MappedAddress );


/*
tmmanPhysicalToMapped

	Translates a SDRAM or MMIO physical address to an operating system 
	mapped address. This function can translate SDRAM and MMIO addresses
	only.

Parameters :

	MemoryBlock
		Pointer to a SDRAM or MMIO memory block structure, that will be used 
		for translating the address. The contents of this structure can be 
		retrieved by calling tmmanDSPGetInfo.

	PhysicalAddress
		The platform specific MMIO or SDRAM physical address.

Return :

	Returns the mapped address corresponding to the given physical address.
*/
UInt32	tmmanPhysicalToMapped ( 
	tmmanMemoryBlock* MemoryBlock, 
	UInt32 PhysicalAddress );


/*
tmmanValidateAddressAndLength

	Checks if the given physical address and length lies between bound of the 
	given memory block. This function works for SDRAM and MMIO addresses only.

Parameters :

	MemoryBlock
		Pointer to a SDRAM or MMIO memory block structure, that will be 
		used for translating the address. The contents of this structure 
		can be retrieved by calling tmmanDSPGetInfo.
	
	Address
			Physical address that needs to be checked.
			
	Length
			Length of the block that needs to be checked.

Return :

	True 
		If the address and length describes a block lying within the range 
		specified by MemoryBlock.
	Flase
		if the address and length describes a block lying outside the 
		range specified by MemoryBlock.
*/
Bool    tmmanValidateAddressAndLength ( 
	tmmanMemoryBlock* MemoryBlock, 
	UInt32 Address, 
	UInt32 Length );



/*
tmmanTranslateAdapterAddress

	This function call uses the TMMan Kernel Mode Driver to translatea a
	adapter mapped address to a physical address that can be used by the 
	TM processor to access that address. 
	NOTE : This call can only be used to translate PHYSICAL ADAPTER MEMORY
	addresses(physical memory that is guaranteed to be page locked and 
	contiguous). 
	As the address range is assumed to be contiguous the length
	of memory range passed to this function does not need to be the entire
	memory range that needs to be accessed.
	
Parameters :

	
	MappedAddress
			OS Mapped memory address that needs to be translated.
			
	Length
			Length of the block that needs to be translated. This 
			parameter does not need to encompass the entire
			memory range.

	PhysicalAddressPtr
			Address of the memory loction where the tranalated physical 
			address will be stored.

Return :

	True 
		If the address and length could be tranalated successfully to
		a adapter physical address.
	Flase
		If the address tranaltion failed.
*/
Bool    tmmanTranslateAdapterAddress ( 
	UInt32 MappedAddress,
	UInt32 Length,
	UInt32 *PhysicalAddressPtr );


/* 
	Message Interfces 
*/

/* 

tmmanMessageCreate


	Creates a bi-directional message channel between the host and 
	the target processor. This message channel can be used to 
	send fixed size packets of type tmmanPacket from one processor
	to another. The message packets are copied across the PCI bus
	via shared mailboxes. Every message channel has its own private 
	queue where incoming packets from the other processor are 
	temporarily buffered.
	When a packet arrives from the other processor the caller supplied
	OS synchronization object will be signaled. The caller can use 
	native OS primitives to block on this object or on multiple 
	objects as required. Note however that due to the relative speed
	of the two processors there may not be a one to one correspondence
	between the number of times the object is actually signaled and the
	number of packets in the incoming queue.


Paremters :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this message channel. See
		NOTES on Object Names.

	SynchronizationHandle
		Pointer to OS specific synchronization object. See NOTES on
		Synchronization Handle.

	SynchronizationFlags
		This parameter describes how TMMan should interpret the 
		SynchronizationHandle parameter. See NOTES on Synchronization
		Flags. It can have one of the following values.

			constTMManModuleHostKernel
			constTMManModuleHostUser
			constTMManModuleTargetKernel
			constTMManModuleTargetUser

	MessageHandlePointer
		Address of the location where the pointer to the message channel 
		will be stored.

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more message channels free.

	statusNameSpaceNoMoreSlots
		Out of name space slots - internal error

	statusNameSpaceLengthExceeded
		Name is more than 12 characters long.

	statusNameSpaceNameConflict
		The user assigned name already exists in TMMan name space.

	statusSynchronizationObjectCreateFail
		The synchronization flags were invalid or memory could not 
		be allocated for the Synchronization object.

	statusQueueObjectCreateFail
		Creation of the queue to buffer incoming packets failed.


*/
TMStatus  tmmanMessageCreate ( 
	UInt32	DSPHandle,
	UInt8*	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* MessageHandlePointer );
/*

tmmanMessageDestroy

	Closes the message channel handle returned by tmmanMessageCreate().
	Only the message channel and the queue are freed. The caller is 
	responsible for freeing the OS synchronization object that was 
	supplied to tmmanMessageCreate().


Parametrers :

	MessageHandle
		Handle to the message channel returned by 
		tmmanMessageCreate().

Returns :

	statusInvalidHandle
		Handle to the message channel is corrupted or has already 
		been closed.

*/

TMStatus  tmmanMessageDestroy ( 
	UInt32 MessageHandle );

/*
tmmanMessageSend

	This function sends a fixed size data packet of type tmmanPacket
	to the peer processor.	This functions returns an error if 
	there is no space in the interprocessor mailbox to send packets.
	However this may be a temporary condition and caller should retry 
	sending the packet after a timeout. Packets on a certain message
	channel are guaranteed to arrive, in the order sent, on peer 
	processor.
	

Parameters :

	MessageHandle
		Handle to the message channel returned by 
		tmmanMessageCreate().

	DataPointer 
		Pointer to the tmmanPacket data structure. Once this call
		returns successfully the data structure can be reused.

Returns :

	statusInvalidHandle
		Handle to the message channel is corrupted or has already 
		been closed.

	statusChannelMailboxFullError
		The interprocessor mailbox is temporarily full, this is
		a temporary condition. The user us permitted to supposed
		to retry the call only when this error code is returned.
		i.e the caller should not do the following
		while ( tmmanMessageSend ( Handle, &Packet ) != statusSuccess );
		it should rather do the following
		while ( tmmanMessageSend ( Handle, &Packet ) 
			== statusChannelMailboxFullError );

*/

TMStatus  tmmanMessageSend ( 
	UInt32 MessageHandle, 
	void *DataPointer );


/*
tmmanMessageReceive

	This function retrieves a packet from the incoming packet
	queue. This is a non-blocking function, so if there are no 
	packets	in the queue this function returns immediately with
	an error code. A synchronization object may be signaled 
	once for multiple packets. The caller should call this 
	function repeatedly, until it fails, to retrieve all packets 
	that have arrived.
	
Parametrers :

	MessageHandle
		Handle to the message channel returned by 
		tmmanMessageCreate().

	DataPointer 
		Pointer to the tmmanPacket data structure. If this call succeeds
		the tmmanPacket structure contains a valid packet.


Returns :

	statusInvalidHandle
		Handle to the message channel is corrupted or has already 
		been closed.

	statusMessageQueueEmptyError
		There are no pending packets in the incoming message queue 
		for this message channel.


*/
TMStatus  tmmanMessageReceive ( 
	UInt32 MessageHandle, 
	void *DataPointer );


/* 
	Event Functions 
*/

/*

tmmanEventCreate

	Events provide an interprocessor signaling mechanism. It enables
	one processor to signal an event that will cause another processor
	to unblock if it is waiting on that event. The caller of this function 
	should use the native OS dependent Synchronization primitives to
	create a OS synchronization object and pass the handle to this 
	function. Due to the relative speeds of the two processors there 
	may not be a one to one correspondence between	the number of 
	times one processors signals the event and the number of times 
	the event actually gets signaled on the peer processor.

Paremters :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this event. See NOTES on
		Object Names.

	SynchronizationHandle
		Pointer to OS specific synchronization object. See NOTES on
		Synchroniztoin Handle.

	SynchronizationFlags
		This parameter describes how TMMan should interpret the 
		SynchronizationHandle parameter. See NOTES on Synchronization
		Flags. It can have one of the following values.

			constTMManModuleHostKernel
			constTMManModuleHostUser
			constTMManModuleTargetKernel
			constTMManModuleTargetUser

	EventHandlePointer
		Address of the location where the pointer to the event	will be 
		stored.

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more events free.

	statusDeviceIoCtlFail
		Internal Error

	statusNameSpaceNoMoreSlots
		Out of name space slots - internal error

	statusNameSpaceLengthExceeded
		Name is more than 12 characters

	statusNameSpaceNameConflict
		The user assigned name already exists in TMMan name space.

	statusSynchronizationObjectCreateFail
		The synchronization flags were invalid or memory could not 
		be allocated for the Synchronization object.

*/

TMStatus	tmmanEventCreate (
	UInt32	DSPHandle,						
	UInt8*	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* EventHandlePointer );

/*
tmmanEventSignal

	This function causes the OS specific synchronization object 
	on the peer processor to be signaled.


Parameters :

	EventHandle
		Handle to the event returned by tmmanEventCreate().

Returns :

	statusInvalidHandle
		Handle to the event is corrupted or has already 
		been closed.

*/

TMStatus	tmmanEventSignal (
	UInt32 EventHandle );

/*
tmmanEventDestroy

	Closes the Event Handle and frees up the resources
	allocated by TMMan for this object. It is the callers
	responsibility to free the OS synchronization object
	that was passed to the tmmanEventCreate() function.	

Parametrers :

	EventHandle
		Handle to the event returned by tmmanEventCreate().

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

*/

TMStatus	tmmanEventDestroy (
	UInt32 EventHandle );

/*
	Shared Memory
*/

/*

tmmanSharedMemoryCreate


	Allocates a block of shared memory and returns a pointer to the 
	memory block. This memory is allocated out of contiguous, page 
	locked memory on the host processor. Shared memory can only be 
	allocated on the host but can be accessed from the target. Note
	that this is a very expensive system resource and should be used
	sparingly. The memory block returned is always aligned on a 32
	bit boundary. TMMan allocates a region of shared memory for every
	board present in the system ( at startup ) and then suballocates 
	blocks from this region when this function is called.

Parametrers :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this object. See NOTES on
		Object Names.

	Length
		Length of the required shared memory block in bytes.

	AddressPointer

		Address of the memory location where the pointer to
		the shared memory will be stored. This pointer can 
		be used by the host directly to access the allocated
		memory.

	SharedMemoryHandlePointer
		Address of the location where the handle to the shared memory
		will be stored. This handle is required to free this resource.
		

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more shared memory slots free.

	statusDeviceIoCtlFail
		Internal Error

	statusNameSpaceNoMoreSlots
		Out of name space slots - internal error

	statusNameSpaceLengthExceeded
		Name is more than 12 characters

	statusNameSpaceNameConflict
		The user assigned name already exists in TMMan name space.

	statusMemoryUnavailable
		No more shared memory available.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanSharedMemoryCreate ( 
	UInt32	DSPHandle, 
	UInt8*	Name,
	UInt32	Length,
	UInt32*	AddressPointer,
	UInt32* SharedMemoryHandlePointer );


/*
tmmanSharedMemoryDestroy


	Closes the Shared Memory handle and frees up the shared 
	memory that was allocated via the call to 
	tmmanSharedMemoryCreate(). This fucntion should be called
	by the host processor only.

Parametrers :

	SharedMemoryHandle
		Handle to the shared memory block returned by 
		tmmanSharedMemoryCreate().

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanSharedMemoryDestroy ( 
	UInt32	SharedMemoryHandle );


/*
tmmanSharedMemoryOpen

	This function opens a handle to a shared memory resource 
	created on the host. This function does not actually 
	allocate any memory, it returns a handle to an existing
	shared memory block, that has been already allocated on
	the host. This function should be called on the target
	processor only.

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this object. See NOTES on
		Object Names.

	LengthPointer
		Address of the memory location where the Length of the 
		shared memory block identified by name will be stored.

	AddressPointer

		Address of the memory location where the pointer to
		the shared memory will be stored. This pointer can 
		be used by the target directly to access the allocated
		memory.

	SharedMemoryHandlePointer
		Address of the location where the handle to the shared memory
		will be stored. This handle is required to free references
		to this resource.
		
Returns :
	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more shared memory slots free.

	statusNameSpaceLengthExceeded
		Name is more than 12 characters

	statusNameSpaceNameNonExistent
		The user provided name does not exists in TMMan name space.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus  tmmanSharedMemoryOpen ( 
	UInt32 DSPHandle, 
	UInt8*	Name,
	UInt32* LengthPointer,
	UInt32*	AddressPointer,
	UInt32*	SharedMemoryHandlePointer );

/*

tmmanSharedMemoryClose

	Closes the shared memory Handle and frees up the resources
	allocated by TMMan for this object. This function does not
	free the shared memory. The shared memory has to be freed 
	by the host. This function should be called from the target
	processor only.

Parametrers :

	SharedMemoryHandle
		Handle to the event returned by tmmanSharedMemoryOpen ().

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus  tmmanSharedMemoryClose (
	UInt32	SharedMemoryHandle );

								 
/* 
	Buffer Locking 
*/

/*

tmmanSGBufferCreate

	This function page locks the specified memory and generates a page frame
	table that can be used by the target to access the page locked memory.
	This function is only supported on hosts that have virtual memory. This
	function can only be called by the host processor. 
	
Parametrers :

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this object. See NOTES on
		Object Names.

	MappedAddress
		Host address of the block of memory that needs to be page locked.
		This parameter is typically the return value of malloc().
		
	Size
		Size of the memory in bytes.

	Flags
		Flags can have one or more the following values.
		constTMManSGBufferRead
			Buffer is going to read into ( Incoming Data )
		constTMManSGBufferWrite
			Buffer is going to be written from ( Outgoing Data ).
		
	BufferHandlePointer
		Address of the location where the handle to the page locked
		memory will be stored. This handle is required to unlock the
		page locked memory.

Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more shared memory slots free.

	statusDeviceIoCtlFail
		internal Error

	statusNameSpaceNoMoreSlots
		Out of name space slots - internal error

	statusNameSpaceLengthExceeded
		Name is more than 12 characters

	statusNameSpaceNameConflict
		The user assigned name already exists in TMMan name space.

	statusMemoryUnavailable
		No more shared memory available to copy the page frame table.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.
*/

TMStatus  tmmanSGBufferCreate ( 
	UInt32	DSPHandle, 
	UInt8*	Name,
	UInt32	MappedAddress,
	UInt32	Size,
	UInt32	Flags,
	UInt32* BufferHandlePointer );

/*
tmmanSGBufferDestroy

	Closes the handle tp the page locked memory and unlocks
	the memory and frees up the page frame tables. This function
	should be called by the host processor only.

Parametrers :

	BufferHandle
		Handle to the event returned by tmmanSharedMemoryOpen ().

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusUnsupportedOnThisPlatform
		If this function is called on the target.

*/
TMStatus  tmmanSGBufferDestroy ( 
	UInt32	BufferHandle );


/*

tmmanSGBufferOpen

	Opens a handle to the block of memory that was page locked on the
	host. This function should be called by the target processor only.

	DSPHandle
		Handle to the TriMedia processor returned by tmmanDSPOpen().
	
	Name
		Unique caller supplied name for this object. See NOTES on
		Object Names.

	EntryCountPointer
		Address of the memory location where the count of the PTE 
		entries is stored by this function.

	SizePointer
		Address of the memory location where the size of the buffer 
		is stored.

	BufferHandlePointer
		Address of the location where the handle to the scatter
		gather buffer will be stored. This handle is required to 
		free references	to this resource.
		
Returns :

	statusInvalidHandle	
		Handle to the DSP is corrupted.

	statusObjectAllocFail
		Object memory allocation failed.

	statusObjectListAllocFail
		No more shared memory slots free.

	statusNameSpaceLengthExceeded
		Name is more than 12 characters

	statusNameSpaceNameNonExistent
		The user provided name does not exists in TMMan name space.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/
TMStatus  tmmanSGBufferOpen ( 
	UInt32	DSPHandle, 
	UInt8*	Name,
	UInt32*	EntryCountPointer,
	UInt32* SizePointer,
	UInt32* BufferHandlePointer );


/*
tmmanSGBufferClose

	Closes the reference to the scatter gather page locked 
	memory. This function does not unlock the memory pages.
	This function should be called from the target processor only.

Parametrers :

	BufferHandle
		Handle to the buffer returned by tmmanSGBufferOpen().

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus  tmmanSGBufferClose ( 
	UInt32	BufferHandle );

/*

tmmanSGBufferFirstBlock


	Returns the description of the first contiguous run of the 
	page locked memory on the host. The description consists
	of the offset of the block from the beginning of the memory,
	pointer of the block that the target processor can use to 
	access the memory, size of the block. Calls have to be made
	to the tmmmanSGBufferNextBlock to get description of 
	subsequent blocks.

Parametrers :

	BufferHandle
		Handle to the buffer returned by tmmanSGBufferOpen().
		
	OffsetPointer 
		Address of the memory location, where the offset of the
		block from the beginning of the memory that was page locked 
		on the host, will be stored.

	AddressPointer
		Address of the memory location, where the pointer to the
		memory block will be stored.

	SizePointer
		Address of the memory location where the size of the block
		will be stored.

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus	tmmanSGBufferFirstBlock ( 
	UInt32	BufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer );

/*

tmmanSGBufferNextBlock

	Returns the description of consecutive runs of contiguous memory
	from the page frame table referred to by BufferHandle. Note that
	tmmanSGBufferFirstBlock functions should be called at least once
	prior to calling this function.


Parametrers :

	BufferHandle
		Handle to the buffer returned by tmmanSGBufferOpen().
		
	OffsetPointer 
		Address of the memory location, where the offset of the
		block from the beginning of the memory that was page locked 
		on the host, will be stored.

	AddressPointer
		Address of the memory location, where the pointer to the
		memory block will be stored.

	SizePointer
		Address of the memory location where the size of the block
		will be stored.

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already 
		been closed.

	statusSGBufferNoMoreEntries
		There are no more entries in the page frame table. To 
		restart parsing of the page frame table, call 
		tmmanSGBufferFirstBlock() followed by calls to 
		tmmanSGBufferBextBlock().

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus	tmmanSGBufferNextBlock ( 
	UInt32	BufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer );

/*
tmmanSGBufferCopy

	The function copies the contents of the page locked memory 
	on the host to/from another block of memory on the target.
	It uses the c run time routine memcpy() to perform the actual
	copying operation. If the caller needs the copying to be 
	done via DMA transfer, then the tmmanSGBufferFirstBlock()
	and tmmanSGBufferNextBlock should be used instead.


Parametrers :

	BufferHandle
		Handle to the buffer returned by tmmanSGBufferOpen().

	Offset 
		Offset from the beginning of memory where the copying has to start.

	Address
		Pointer to the buffer on the target processor from/to where will be
		copied.

	Size
		Number of bytes to copy.

	Direction
		Direction of copy.
		
		True 
			Copy from host memory to target memory.

		False
			Copy from target to host memory. 
		

Returns :

	statusInvalidHandle
		Handle to the object is corrupted or has already been closed.

	statusSGBufferOffsetOutOfRange
		The Offset supplied to this function is out of range of the
		page locked host buffer.
			

	statusSGBufferSizeOutOfRange
		The size passed to this function is greater than the amount of
		page locked memory available from the given offset.

	statusUnsupportedOnThisPlatform
		If this function is called on the host.

*/

TMStatus	tmmanSGBufferCopy ( 
	UInt32	BufferHandle,
	UInt32	Offset,
	UInt32	Address, 
	UInt32	Size, 
	UInt32	Direction );

/* DEBUG SUPPORT */

/*
tmmanDebugDPBuffers

	This function retrieves pointer to the circular wrap around buffers,
	where the TriMedia processor dumps debug messages.  
	This function is current callable only from the host and it retrieves
	debug information generated by the TriMedia processor. 
	Debug information printed via the DP macros are retrieved via this 
	function. 	See NOTES on Debug Buffer Pointers.

Parametrers :

	DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

	FirstHalfPtr
		Address of the memory location where the pointer to the first half
		of the buffer will be stored.


	FirstHalfSizePtr
		Address of the memory location where the size of the first half 
		buffer will be stored.

	SecondHalfPtr
		Address of the memory location where the pointer to the second half
		of the buffer will be stored.

	SecondHalfSizePtr
		Address of the memory location where the size of the second half 
		buffer will be stored.

Returns :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusDebugNoPeerDebugInformation
		This function scans through the entire SDRAM to search for a magic 
		header that identifies valid debug information. This error code
		denotes that the magic header does not exist or had been corrupted.

	statusUnsupportedOnThisPlatform
		If this function is called on the target. 
*/

TMStatus	tmmanDebugDPBuffers (
	UInt32	DSPHandle, 
	UInt8*	*FirstHalfPtr, 
	UInt32*	FirstHalfSizePtr, 
	UInt8*	*SecondHalfPtr, 
	UInt32*	SecondHalfSizePtr );

/*
tmmanDebugHostBuffers

	This function retrieves pointer to the circular wrap around buffers,
	where the host processor dumps debug messages.  
	This function is current callable only from the host and it retrieves
	debug information generated by the host component of TMMan. The are no
	application callable functions that can dump data into these buffers.
	TMMan(host) uses this buffer to print internal debug information.
	See NOTES on Debug Buffer Pointers.

Parameters :

	FirstHalfPtr
		Address of the memory location where the pointer to the first half
		of the buffer will be stored.


	FirstHalfSizePtr
		Address of the memory location where the size of the first half 
		buffer will be stored.

	SecondHalfPtr
		Address of the memory location where the pointer to the second half
		of the buffer will be stored.

	SecondHalfSizePtr
		Address of the memory location where the size of the second half 
		buffer will be stored.

Returns :

	statusNotImplemented	
		This function will be implemented in a future release.
		Currently all TMMan (host) debug messages are printed to the host
		debugger(WinDBG or NTIce).

  	statusUnsupportedOnThisPlatform
		If this function is called on the target. 

*/

TMStatus	tmmanDebugHostBuffers (
	UInt8*	*FirstHalfPtr, 
	UInt32*	FirstHalfSizePtr, 
	UInt8*	*SecondHalfPtr, 
	UInt32*	SecondHalfSizePtr );


/*
tmmanDebugTargetBuffers

	This function retrieves pointer to the circular wrap around buffers,
	where the target processor dumps debug messages.  
	This function is current callable only from the host and it retrieves
	debug information generated by the target component of TMMan. 
	Applications running on the target can call the tmmanDebugPrintf 
	fucntion to print information into these buffers.
	TMMan(target) uses this buffer to print internal debug information.
	See NOTES on Debug Buffer Pointers.

  DSPHandle
		Handle to the DSP returned by tmmanDSPOpen. 

	FirstHalfPtr
		Address of the memory location where the pointer to the first half
		of the buffer will be stored.


	FirstHalfSizePtr
		Address of the memory location where the size of the first half 
		buffer will be stored.

	SecondHalfPtr
		Address of the memory location where the pointer to the second half
		of the buffer will be stored.

	SecondHalfSizePtr
		Address of the memory location where the size of the second half 
		buffer will be stored.

Returns :

	statusInvalidHandle
		Handle to the DSP is corrupted.

	statusDebugNoPeerDebugInformation
		This function scans through the entire SDRAM to search for a magic 
		header that identifies valid debug information. This error code
		denotes that the magic header do not exist or has been corrupted.

   	statusUnsupportedOnThisPlatform
		If this function is called on the target. 

*/

TMStatus	tmmanDebugTargetBuffers (
	UInt32	DSPHandle, 
	UInt8*	*FirstHalfPtr, 
	UInt32*	FirstHalfSizePtr, 
	UInt8*	*SecondHalfPtr, 
	UInt32*	SecondHalfSizePtr );


/*
tmmanDebugPrintf

	This function is used to print formatted strings via the debugging
	subsystem of TMMan. The implementation of this function is platform
	specific. On the host this functions prints out strings to the debug
	windows. On the target this function prints strings to the debug
	trace buffers. The maximum length of the string can be 1024 bytes.
	Applications on the TriMedia processor should use the DP macros to
	print debugging information.

Parametrers :

	Format
		printf style format specifier
	...
		printf style arguments.

Returns :

	Number of items printed.
*/

UInt32 tmmanDebugPrintf ( Int8 *Format, ... );


/* TMMan Internal Functions - should not be called by applications */

Bool	tmmanGetImageInfo (
	Int8*	ImagePath,
	UInt32*	TypePtr,
	UInt32*	EndianPtr );

Bool	tmmanGetTCSPath ( 
	Int8*	TCSPathBuffer,
	UInt32	TCSPathBufferLength );

TMStatus  tmmanDSPGetInternalInfo ( 
	UInt32 DSPHandle, 
	tmmanDSPInternalInfo* pDSPCaps );

TMStatus  tmmanDSPSetInternalInfo ( 
	UInt32 DSPHandle, 
	tmmanDSPInternalInfo* pDSPCaps );

void	tmmanExit (  
	UInt32 DSPNumber );

Pointer	tmmanInit ( 
	UInt32 DSPNumber, 
	Pointer Configuration );

#ifdef __cplusplus
};
#endif	/* __cplusplus */

#endif /* _TMMANAPI_H_ */



