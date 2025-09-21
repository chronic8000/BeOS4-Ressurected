/*
tmif.h

	950928	TR	Created.
	960610	TR	Major revamp to include all the new tmman interfaces.
	980306	Volker Schildwach	Ported to Windows CE (changed tmifDSPLoad struct)
    990511  DTO Ported to pSOS.
	
*/
#ifndef TMIF_H
#define TMIF_H

#ifdef __cplusplus
extern "C" {
#endif
	
	//#include <psos.h>
	
#include <tmmanapi.h>
#include <tmmanlib.h>
	
#include "tm_platform.h"
/*
	//#define     tmmanIOCTLCode(f,m)                 \
	//    CTL_CODE ( FILE_DEVICE_UNKNOWN, 0x800 | ( f), (m) , FILE_ANY_ACCESS )
*/	
	
	
	//DL
#define TRIMEDIA_IOC_MAGIC 'k'
	
	// functions that map directly to the tmman interace
#define	constIOCTLtmmanNegotiateVersion		( 0x01 )
	
#define	constIOCTLtmmanDSPGetNum			( 0x10 )
#define	constIOCTLtmmanDSPOpen				( 0x11 )
#define	constIOCTLtmmanDSPClose				( 0x12 )
#define	constIOCTLtmmanDSPLoad				( 0x13 )
#define	constIOCTLtmmanDSPStart				( 0x14 )
#define	constIOCTLtmmanDSPStop				( 0x15 )
#define	constIOCTLtmmanDSPReset				( 0x16 )
#define	constIOCTLtmmanDSPInfo				( 0x17 )
#define	constIOCTLtmmanDSPStatus			( 0x18 )
#define	constIOCTLtmmanDSPGetInternalInfo	( 0x19 )
#define	constIOCTLtmmanDSPSetInternalInfo	( 0x1a )
#define	constIOCTLtmmanDSPGetEndianess		( 0x1b )
	
#define	constIOCTLtmmanMessageCreate		( 0x30 )
#define	constIOCTLtmmanMessageDestroy		( 0x31 )
#define	constIOCTLtmmanMessageSend			( 0x32 )
#define	constIOCTLtmmanMessageReceive		( 0x33 )
#define	constIOCTLtmmanMessageBlock			( 0x34 )
	
#define	constIOCTLtmmanSharedMemoryCreate	( 0x40 )
#define	constIOCTLtmmanSharedMemoryDestroy	( 0x41 )
#define	constIOCTLtmmanSharedMemoryOpen		( 0x42 )
#define	constIOCTLtmmanSharedMemoryClose	( 0x43 )
#define	constIOCTLtmmanSharedMemoryGetAddress ( 0x44 )
	
#define	constIOCTLtmmanSGBufferCreate		( 0x50 )
#define	constIOCTLtmmanSGBufferDestroy		( 0x51 )
#define	constIOCTLtmmanSGBufferOpen			( 0x52 )
#define	constIOCTLtmmanSGBufferClose		( 0x53 )
#define	constIOCTLtmmanSGBufferFirstBlock	( 0x54 )
#define	constIOCTLtmmanSGBufferNextBlock	( 0x55 )
#define	constIOCTLtmmanSGBufferCopy			( 0x56 )
	
#define	constIOCTLtmmanNameSpaceCreate		( 0x60 )
#define	constIOCTLtmmanNameSpaceDestroy		( 0x61 )
#define	constIOCTLtmmanNameSpaceOpen		( 0x62 )
#define	constIOCTLtmmanNameSpaceClose		( 0x63 )
	
#define	constIOCTLtmmanGetParameter			( 0x70 )
#define	constIOCTLtmmanSetParameter			( 0x71 )
	
#define	constIOCTLtmmanStreamCreate			( 0x80 )
#define	constIOCTLtmmanStreamDestroy		( 0x81 )
#define	constIOCTLtmmanPosition				( 0x82 )
#define	constIOCTLtmmanAddBuffer			( 0x83 )
	
#define	constIOCTLtmmanGetDebugDPBuffers	( 0x90 )
#define	constIOCTLtmmanGetDebugHostBuffers	( 0x91 )
#define	constIOCTLtmmanGetDebugTargetBuffers ( 0x92 )
#define	constIOCTLtmmanDebugPrintf           ( 0x93 )
	
#define	constIOCTLtmmanEventCreate			( 0xa0 )
#define	constIOCTLtmmanEventDestroy			( 0xa1 )
#define	constIOCTLtmmanEventSignal			( 0xa2 )
#define	constIOCTLtmmanEventBlock			( 0xa3 )
	
#define	constIOCTLtmmanXlateAdapterAddress1	( 0xb0 )
#define	constIOCTLtmmanXlateAdapterAddress2	( 0xb1 )
	
#define constIOCTLtmmanMapSDRAM             ( 0xb2 )
#define constIOCTLtmmanUnmapSDRAM           ( 0xb3 )
	
	// functions that do not directly map to the tmman intterface
	
	//
	//			STRUCTURES REQURIED FOR API MARSHALING
	//
	
	
	//DL:
	typedef struct tagioparms{
		UInt32 tid;
		UInt32 err;
		void * in_iopb;
	} ioparms;
	
	
	
	
	
	typedef struct tagtmifGenericFunction
	{
        TMStatus		Status;
		UInt32			Handle;
	}   tmifGenericFunction;
	
	
	typedef	struct tagtmifNegotiateVersion
	{
		TMStatus		Status;
		UInt32			ModuleID;
		tmmanVersion		Version;
	}	tmifNegotiateVersion;
	
	typedef	struct tagtmifDSPNum
	{
		TMStatus	Status;
		UInt32	DSPCount;
	}	tmifDSPNum;
	
	typedef	struct tagtmifDSPOpen
	{
		TMStatus	Status;
		UInt32	DSPNumber;
		UInt32	DSPHandle;
		
	}	tmifDSPOpen;
	
	typedef	struct tagtmifDSPInternalInfo
	{
		TMStatus				Status;
		UInt32					DSPHandle;
		tmmanDSPInternalInfo	Info;
	}	tmifDSPInternalInfo;
	
	typedef	struct tagtmifDSPStatus
	{
		TMStatus	Status;
		UInt32	DSPHandle;
		UInt32	DSPStatus;
	}	tmifDSPStatus;
	
	typedef	struct tagtmifDSPInfo
	{
		TMStatus		Status;
		UInt32			DSPHandle;
		tmmanDSPInfo	Info;
	}	tmifDSPInfo;
	
	typedef	struct tagtmifDSPEndianess
	{
		TMStatus		Status;
		UInt32			DSPHandle;
		UInt32			Endianess;
	}	tmifDSPEndianess;
	
	
	/* CLOSE -> Generic */
	
	typedef	struct tagtmifDSPLoad
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt32		Endianess;
		UInt32		PeerMajorVersion;
		UInt32		PeerMinorVersion;
#ifdef UNDER_CE
		UInt32		LoadAddress;
		char		ImagePath[256];
#endif // UNDER_CE
	}	tmifDSPLoad;
	
	/* RUN , STOP , RESET -> Generic */
	
	typedef struct tagtmifMessageCreate
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt8		Name[constTMManNameSpaceNameLength];
		UInt32		SynchObject;
		UInt32		SynchFlags;
		UInt32		MessageHandle;						
	}	tmifMessageCreate;
	
	typedef struct tagtmifMessageSR
	{
		TMStatus	Status;
		UInt32	MessageHandle;
		UInt8	Packet[constTMManPacketSize];
	}	tmifMessageSR;
	
	/* MSGDESTROY -> Generic */
	
	typedef struct tagtmifEventCreate
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt8		Name[constTMManNameSpaceNameLength];
		UInt32		SynchObject;
		UInt32		SynchFlags;
		UInt32		EventHandle;						
	}	tmifEventCreate;
	
	/* EVENTSIGNAL, EVENTDESTROY ->Generic */
	
	typedef struct tagtmifSharedMemoryCreate
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt8		Name[constTMManNameSpaceNameLength];
		UInt32		Size;
		Pointer		Address;
		Pointer		PhysicalAddress;
		UInt32		SharedMemoryHandle;
	}	tmifSharedMemoryCreate;
	
	typedef struct tagtmifSharedMemoryAddress
	{
		TMStatus	Status;
		UInt32		SharedMemoryHandle;
		UInt32		PhysicalAddress;
	}	tmifSharedMemoryAddress;
	
	
	/* SHMEMDESTROY - > GENERIC */
	
	typedef struct tagtmifSGBufferCreate
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt8		Name[constTMManNameSpaceNameLength];
		UInt32		MappedAddress;
		UInt32		Size;
		UInt32		Flags;
		UInt32		SGBufferHandle;
	}	tmifSGBufferCreate;
	
	/* BUFFERDESTROY -> Generic */
	
	typedef struct tagtmifDebugBuffers
	{
		TMStatus	Status;
		UInt32		DSPHandle;
		UInt8*		FirstHalfPtr;
		UInt32		FirstHalfSize;
		UInt8*		SecondHalfPtr;
		UInt32		SecondHalfSize;
	}	tmifDebugBuffers;
	
	
	typedef struct
	{
		UInt32   controlCode;
		void    *pData; 
		
	}tTMMANIo;
	
	
	
	
	
	//DL
	//typedef int UUInt32;
	
	
	void  tmmInit(ioparms  *pIoParms);
	void  tmmOpen(ioparms  *pIoParms);
	void  tmmClose(ioparms  *pIoParms);
	void  tmmCntrl(ioparms  *pIoParms);
	
	//DL
	
	
	
#ifdef __cplusplus
};
#endif	/* __cplusplus */

#endif  // TMIF_H
