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
//	960405	Tilakraj Roy 	Created
//	960710	Tilakraj Roy	Started adding code for tmman inteface 
//	961008	Tilakraj Roy	Added code for shared memory allocaiton interfaces.
//	961010	Tilakraj Roy	Added code for image loading, running & stopping
//	970806	Tilakraj Roy	Ported to Workstation V4.0
//	982005	Volker Schildwach	Ported to Windwos CE
//	981021	Tilakraj Roy	Changes for integrating into common source base
//      990511  DTO             Ported to pSOS
//      001107  DL              Ported to Linux   
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//				SYSTEM INCLUDE FILES
//////////////////////////////////////////////////////////////////////////////

//#include <psos.h>
//#include <prepc.h>
//#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
//				DRIVER INCLUDE FILES
//////////////////////////////////////////////////////////////////////////////



#include "tmmanapi.h"
#include "tmmanlib.h"
#include "tm_platform.h"
#include "verinfo.h"
#include "tmif.h"

/*
BeIA/OS drivers can see both kernel space and user space. So, there is no need
for this functions. Actually, they are replaced by memory copies. Eventually,
these useless copies will be removed.
*/
#define copy_to_user	memcpy
#define copy_from_user	memcpy

//////////////////////////////////////////////////////////////////////////////
//				MANIFEST CONSTANTS
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//				TYPEDEFS
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//				PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
TMStatus	tmmanKernelModeNegotiateVersion ( tmmanVersion* Version );



//////////////////////////////////////////////////////////////////////////////
//				IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// FUNCTION:     tmmOpen:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
void tmmOpen(ioparms *pIoParms)
{
	UInt32          ClientIdx;
	UInt32          DeviceIdx;
	ClientObject    *pClient;
    
	// search for an empty slot.
	DPF(1,("tmman:tmmanOpen:ClientList:"));
	for (ClientIdx = 0; ClientIdx < TMManGlobal->MaximumClients ; ClientIdx++)
    {
		if ( TMManGlobal->ClientList[ClientIdx] )
        {
			DPF(1,("[#%lx:%lx]", 
				ClientIdx, 
				((ClientObject*)TMManGlobal->ClientList[ClientIdx])->Process ));
			continue;
        }
		else
        {
			break;
        }
    }
    
	DPF(1,("\n"));
    
	if ( ClientIdx == TMManGlobal->MaximumClients )
    {
		DPF(0,("tmman:tmmanOpen:NoMoreClientsFree\n"));
		pIoParms->err = 1;
		return;
    }
    
	TMManGlobal->ClientList[ClientIdx] = memAllocate (
		sizeof(ClientObject) + 
		sizeof(ClientDeviceObject) * (TMManGlobal->DeviceCount - 1)); 
    
	if (TMManGlobal->ClientList[ClientIdx] == Null)
    {
		DPF(0,("tmman:tmmanOpen:ExAllocatePool:FAIL\n"));
		pIoParms->err = 2;
		return;
    }
    
	pClient = TMManGlobal->ClientList[ClientIdx];
    
	pClient->Process = pIoParms->tid;
	
	pClient->DeviceCount = TMManGlobal->DeviceCount;
    
	// initialize the per device data structures
	// BUGCHECK - we have to go by Client->DeviceCount since devices could have
	// gone away Workstation 5.0 problem.
	for (DeviceIdx = 0; DeviceIdx < TMManGlobal->DeviceCount; DeviceIdx++)
    {
		TMManDeviceObject* TMManDevice = TMManGlobal->DeviceList[DeviceIdx];
		UInt32             Length;
		void              *AddrKernel;
		void              *AddrPhys;
        
		pClient->Device[DeviceIdx].Device = TMManDevice;
        
		halGetMMIOInfo (
			TMManDevice->HalHandle,
			(Pointer*)&AddrPhys,
			(Pointer*)&AddrKernel,
			&Length );
        
		pClient->Device[DeviceIdx].MMIOAddrUser   = AddrKernel;
		pClient->Device[DeviceIdx].MMIOAddrPhys   = AddrPhys;
        
		halGetSDRAMInfo (
			TMManDevice->HalHandle,
			(Pointer*)&AddrPhys,
			(Pointer*)&AddrKernel,
			&Length );
		
		pClient->Device[DeviceIdx].SDRAMAddrUser  = AddrKernel;
		pClient->Device[DeviceIdx].SDRAMAddrPhys  = AddrPhys;
		
		pClient->Device[DeviceIdx].MemoryAddrUser = TMManDevice->MemoryBlock;
    }
	
	TMManGlobal->ClientCount++;
	
	pIoParms->err = 0;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmClose:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
void tmmClose(ioparms *pIoParms)
{
	UInt32            ClientIdx;
	UInt32            DeviceIdx;
	ClientObject     *pClient;
	PEPROCESS         Process;
    
	Process       = pIoParms->tid;
	pIoParms->err = 0;
    
	DPF(1,("tmman:tmmanClose:Process[%lx]\n", Process ));
    
	for (ClientIdx = 0; ClientIdx < TMManGlobal->MaximumClients; ClientIdx++)
    {
		if ( !TMManGlobal->ClientList[ClientIdx] )
			continue;
        
		if (((ClientObject*)TMManGlobal->ClientList[ClientIdx])->Process 
			!= Process)
			continue;
		break;
    }
    
	if (ClientIdx == TMManGlobal->MaximumClients)
    {
		DPF(0,("tmman:tmmanClose:PANIC:InvalidHandle:Process[%lx]\n", Process));
		pIoParms->err = 1;
		return;
    }
    
	// assume one open handle per process context		
	// if we have more than slot of a single process than
	// we are smoking something we are not supposed to
	// since tmman32.dll is the only one who calls CreateProcess
	// during ATTACH_PROCESS
    
	pClient = TMManGlobal->ClientList[ClientIdx];
    
	// initialize the per device data structures
	// BUGCHECK - we have to go by Client->DeviceCount 
	for ( DeviceIdx = 0 ; DeviceIdx < TMManGlobal->DeviceCount ; DeviceIdx++ )
    {
		TMManDeviceObject* TMManDevice = TMManGlobal->DeviceList[DeviceIdx];
        
		memoryManagerDestroyMemoryByClient (
			TMManDevice->MemoryManagerHandle, 
			(UInt32)Process );
        
		messageManagerDestroyMessageByClient (
			TMManDevice->MessageManagerHandle, 
			(UInt32)Process );
        
		eventManagerDestroyEventByClient (
			TMManDevice->EventManagerHandle, 
			(UInt32)Process );
        
			/* DL  sgbufferManagerDestroySGBufferByClient (
			TMManDevice->SGBufferManagerHandle, 
			(UInt32)Process );
		*/  
    }
    
    
	memFree ( TMManGlobal->ClientList[ClientIdx] );
    
	TMManGlobal->ClientList[ClientIdx] = Null;
    
	TMManGlobal->ClientCount--;
	
	pIoParms->err = 0;
    
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmCntrl:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
void tmmCntrl(ioparms *pIoParms)
{
	tTMMANIo        *pParamBlock;
	tTMMANIo        tTmmanIo_kernelspace;
	ioparms         IoParms_kernelspace;
	ioparms*        pIoParms_userspace;
	tTMMANIo*       ptTmmanIo;
    
	pIoParms_userspace = pIoParms;
	copy_from_user( &IoParms_kernelspace
		, pIoParms_userspace
		, sizeof(ioparms));
	pIoParms    = &IoParms_kernelspace;
	ptTmmanIo   = (tTMMANIo*)IoParms_kernelspace.in_iopb;
	copy_from_user( &tTmmanIo_kernelspace
		, ptTmmanIo
		, sizeof(tTMMANIo));
	pParamBlock = &tTmmanIo_kernelspace;
	DPF(0,("pIoParms_userspace->err is 0x%lx and IoParms_kernelspace.err is 0x%lx\n",
		pIoParms_userspace->err, IoParms_kernelspace.err));  
	
	// CASE IO Control Code
	switch ( pParamBlock->controlCode )
    {
		// OF Negotiate Version
    case constIOCTLtmmanNegotiateVersion : 
		{
			tmifNegotiateVersion  *TMIF;
			tmifNegotiateVersion  TMIF_kernelspace;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanNegotiateVersion\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifNegotiateVersion)
				);
			TMIF=&TMIF_kernelspace;
			DPF(0,("tmif.c: tmmcntrl: ver.Maj is %ld en ver.Min is %ld\n"
				,TMIF->Version.Major,TMIF->Version.Minor));        
			TMIF->Status = tmmanKernelModeNegotiateVersion ( &TMIF->Version );
			DPF(0,("tmif.c: tmmcntrl: ver.Maj is %ld en ver.Min is %ld and build is %ld\n"
				,TMIF->Version.Major,TMIF->Version.Minor, TMIF->Version.Build)); 
			DPF(0,("tmif.c: tmmcntrl: pParamBlock->pData is 0x%p\n"
				, pParamBlock->pData));
			copy_to_user( pParamBlock->pData
				, TMIF
				, sizeof(tmifNegotiateVersion));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanNegotiateVersion: end\n"));
		}
		break;
		
		
		// OF DSP Open
    case constIOCTLtmmanDSPOpen :
		{
			tmifDSPOpen* TMIF;
			tmifDSPOpen  TMIF_kernelspace;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPOpen\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDSPOpen)
				);
			//tmifDSPOpen* TMIF = (tmifDSPOpen*)(pParamBlock->pData);
			TMIF=&TMIF_kernelspace;
			if ( TMIF->DSPNumber < TMManGlobal->DeviceCount )
			{
				DPF(0,("tmif.c: tmmCntrl: TMIF->DSPHandle is 0x%lx before\n"
					,TMIF->DSPHandle));
				TMIF->DSPHandle = 
					(UInt32)TMManGlobal->DeviceList[TMIF->DSPNumber];
				DPF(0,("tmif.c: tmmCntrl: TMIF->DSPHandle is 0x%lx after\n"
					,TMIF->DSPHandle));
				TMIF->Status = statusSuccess;
			}
			else
			{
				DPF(0,("tmman:TMM_IOControl:DSPOpen:ERROR out of range[%lx,%lx]\n"
					,TMIF->DSPNumber,TMManGlobal->DeviceCount));
				TMIF->Status = statusDSPNumberOutofRange;
			}
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDSPOpen));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPOpen, end\n"));    
		}
		break;
		
		
		// OF DSP Close
    case constIOCTLtmmanDSPClose :
		{
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPClose\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			TMIF=&TMIF_kernelspace;
			//tmifGenericFunction* TMIF=(tmifGenericFunction*)(pParamBlock->pData);
			TMIF->Status = statusSuccess;
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPClose, end\n"));    
			
		}
		break;
		
		
		// OF Get Number of DSPs
    case constIOCTLtmmanDSPGetNum :
		{
			tmifDSPNum* TMIF;
			tmifDSPNum  TMIF_kernelspace;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPGetNum\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDSPNum)
				);
			
			TMIF=&TMIF_kernelspace;
			// tmifDSPNum* TMIF = (tmifDSPNum*)(pParamBlock->pData);        	
			TMIF->DSPCount      = TMManGlobal->DeviceCount;
			DPF(0,("tmif.c:tmmCntrol: TMManGlobal->DeviceCount is %ld\n"
				,TMManGlobal->DeviceCount));
			TMIF->Status = statusSuccess;
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDSPNum));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPGetNum, end\n"));    
		}
		break;
		
        
		// OF Get DSP Info
    case constIOCTLtmmanDSPInfo :
		{
			
			tmifDSPInfo*        TMIF;
			tmifDSPInfo         TMIF_kernelspace;
			TMManDeviceObject*  TMManDevice;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPInfo\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDSPInfo)
				);
			
			TMIF=&TMIF_kernelspace;
			//	tmifDSPInfo*        TMIF = (tmifDSPInfo*)(pParamBlock->pData);
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			
			halGetMMIOInfo (
				TMManDevice->HalHandle,
				(Pointer*)&TMIF->Info.MMIO.PhysicalAddress,
				(Pointer*)&TMIF->Info.MMIO.MappedAddress,
				&TMIF->Info.MMIO.Size );
            
            
			halGetSDRAMInfo (
				TMManDevice->HalHandle,
				(Pointer*)&TMIF->Info.SDRAM.PhysicalAddress,
				(Pointer*)&TMIF->Info.SDRAM.MappedAddress,
				&TMIF->Info.SDRAM.Size );
            
			halGetTMPCIInfo ( 
				TMManDevice->HalHandle,
				&TMIF->Info.TMDeviceVendorID,
				&TMIF->Info.TMSubSystemID,
				&TMIF->Info.TMClassRevisionID );
            
			halGetBridgePCIInfo ( 
				TMManDevice->HalHandle,
				&TMIF->Info.BridgeDeviceVendorID,
				&TMIF->Info.BridgeSubsystemID,
				&TMIF->Info.BridgeClassRevisionID );
            
			TMIF->Info.DSPNumber = TMManDevice->DSPNumber;
			DPF(0,("tmif.c: tmmCntrl: TMIF->Info.DSPNumber is %ld\n"
				,TMIF->Info.DSPNumber));          
			TMIF->Status = statusSuccess;
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDSPInfo));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPInfo, end\n"));    
		}
		break;
		
		
    case	constIOCTLtmmanDSPLoad :
		{
			tmifDSPLoad*        TMIF;
			tmifDSPLoad         TMIF_kernelspace;
			TMManDeviceObject*  TMManDevice;
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPLoad\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDSPLoad)
				);
			
			TMIF=&TMIF_kernelspace;
			// tmifDSPLoad*	TMIF = (tmifDSPLoad*)(pParamBlock->pData);
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			DPF(1,("tmman:IOCTLtmmanDSPLoad called\n"));
			
			// don't do anything in kernel mode for now
            
			// BUGCHECK : set the hal endiannesss here
			// the endian ness swap has to be done here since it is only 
			// during loading that we will know what endianess the target 
			// is running in.
			// DTO: due to HW PCI swapping need to pretend host is 
			//      LittleEndian
			
			halSwapEndianess ( 
				TMManDevice->HalHandle,
				( TMIF->Endianess == constTMManEndianessBig ) );
            
			halSetPeerVersion ( 
				TMManDevice->HalHandle,
				TMIF->PeerMajorVersion,
				TMIF->PeerMinorVersion );
            
			TMIF->Status = 	statusSuccess;
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDSPLoad));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPLoad, end\n"));    
		}
		break;
		
        
		// OF DSP Start
    case constIOCTLtmmanDSPStart :
		{
			
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			/*
			unused variable
			UInt32	             CPURevision;
			UInt32               BoardRevision;
			*/            
			UInt32	             TargetKernelMajorVersion;
			UInt32	             TargetKernelMinorVersion;
			UInt32	             DeviceVendorID;
			UInt32	             SubsystemID;
			UInt32	             ClassRevisionID;
			
			
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPStart\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			TMIF=&TMIF_kernelspace;
			
			// DL: tmifGenericFunction*	TMIF = (tmifGenericFunction*)(pParamBlock->pData);
			TMManDevice = (TMManDeviceObject*)TMIF->Handle;
            
			
            
			DPF(1,("tmman:constIOCTLtmmanDSPStart called\n"));
            
			TMManDevice->Status = constTMManDSPStatusRunning;
            
            
			TMManDevice->SharedData->HostKernelMajorVersion =
				halAccess32( TMManDevice->HalHandle, verGetFileMajorVersion() );
			TMManDevice->SharedData->HostKernelMinorVersion =
				halAccess32( TMManDevice->HalHandle, verGetFileMinorVersion() );
            
			TMManDevice->SharedData->HalShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->HalSharedAddress );
			
			TMManDevice->SharedData->ChannelShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->ChannelSharedAddress );
			
			TMManDevice->SharedData->VIntrShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->VIntrSharedAddress );
			
			TMManDevice->SharedData->EventShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->EventSharedAddress );
			
			TMManDevice->SharedData->DebugShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->DebugSharedAddress );
			
			TMManDevice->SharedData->MemoryShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->MemorySharedAddress );
			
			TMManDevice->SharedData->MemoryBlock = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->MemoryBlockAddress );
			
			TMManDevice->SharedData->NameSpaceShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->NameSpaceSharedAddress );
			
			TMManDevice->SharedData->SGBufferShared = 
				halAccess32( TMManDevice->HalHandle, 
				TMManDevice->SGBufferSharedAddress );
            
			halGetTMPCIInfo ( 
				TMManDevice->HalHandle,
				&DeviceVendorID,
				&SubsystemID,
				&ClassRevisionID );
            
			TMManDevice->SharedData->TMDeviceVendorID = 
				halAccess32( TMManDevice->HalHandle, 
				DeviceVendorID );
			
			TMManDevice->SharedData->TMSubsystemID = 
				halAccess32( TMManDevice->HalHandle, 
				SubsystemID );
			
			TMManDevice->SharedData->TMClassRevisionID = 
				halAccess32( TMManDevice->HalHandle, 
				ClassRevisionID );
			
			halGetBridgePCIInfo ( 
				TMManDevice->HalHandle,
				&DeviceVendorID,
				&SubsystemID,
				&ClassRevisionID );
            
			TMManDevice->SharedData->BridgeDeviceVendorID =
				halAccess32( TMManDevice->HalHandle,
				DeviceVendorID );
            
			TMManDevice->SharedData->BridgeSubsystemID =
				halAccess32( TMManDevice->HalHandle,
				SubsystemID );
            
			TMManDevice->SharedData->BridgeClassRevisionID =
				halAccess32( TMManDevice->HalHandle,
				ClassRevisionID );
            
			TMManDevice->SharedData->TargetTraceBufferSize = 
				halAccess32( TMManDevice->HalHandle, 
				TMManGlobal->TargetTraceBufferSize );
            
			TMManDevice->SharedData->TargetTraceLeveBitmap = 
				halAccess32( TMManDevice->HalHandle, 
				TMManGlobal->TargetTraceLeveBitmap );
            
			TMManDevice->SharedData->TargetTraceType =
				halAccess32( TMManDevice->HalHandle, 
				TMManGlobal->TargetTraceType );
            
				/*            
				KeQuerySystemTime ( &SystemTime );
				
				 TMManDevice->SharedData->DebuggingHi =
				 halAccess32( TMManDevice->HalHandle,
				 SystemTime.HighPart );
				 TMManDevice->SharedData->DebuggingLo =
				 halAccess32( TMManDevice->HalHandle, 
				 SystemTime.LowPart );
			*/            
            
			TMManDevice->SharedData->TMManMailboxCount = 
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->MailboxCount );
            
			TMManDevice->SharedData->TMManChannelCount = 
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->ChannelCount );
            
			TMManDevice->SharedData->TMManVIntrCount =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->VIntrCount );
            
			TMManDevice->SharedData->TMManMessageCount =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->MessageCount );
            
			TMManDevice->SharedData->TMManEventCount = 
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->EventCount );
            
			TMManDevice->SharedData->TMManStreamCount =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->StreamCount );
            
			TMManDevice->SharedData->TMManNameSpaceCount = 
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->NameSpaceCount );
            
			TMManDevice->SharedData->TMManMemoryCount =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->MemoryCount );
            
			TMManDevice->SharedData->TMManMemorySize =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->MemorySize );
            
			TMManDevice->SharedData->TMManSGBufferCount =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->SGBufferCount );
            
			TMManDevice->SharedData->SpeculativeLoadFix =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->SpeculativeLoadFix );
            
			TMManDevice->SharedData->PCIInterruptNumber =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->PCIInterruptNumber );
            
			TMManDevice->SharedData->MMIOInterruptNumber =
				halAccess32( TMManDevice->HalHandle,
				TMManGlobal->MMIOInterruptNumber );
            
			DPF(1,("tmman:tmmanDeviceControl:Start:HostKernelMajorVersion[%lx]\n",
				verGetFileMajorVersion() ));
            
			DPF(1,("tmman:tmmanDeviceControl:Start:HostKernelMinorVersion[%lx]\n",
				verGetFileMinorVersion() ));
            
			halGetPeerVersion ( 
				TMManDevice->HalHandle,
				&TargetKernelMajorVersion,
				&TargetKernelMinorVersion );
            
            
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TargetKernelMajorVersion[%lx]\n",
				TargetKernelMajorVersion));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TargetKernelMinorVersion[%lx]\n",
				TargetKernelMinorVersion));
            
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:HalShared[%lx]\n",
				TMManDevice->HalSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:ChannelShared[%lx]\n",
				TMManDevice->ChannelSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:VIntrShared[%lx]\n",
				TMManDevice->VIntrSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:EventShared[%lx]\n",
				TMManDevice->EventSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:DebugShared[%lx]\n",
				TMManDevice->DebugSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:MemoryShared[%lx]\n",
				TMManDevice->MemorySharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:MemoryBlock[%lx]\n",
				TMManDevice->MemoryBlockAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:NameSpaceShared[%lx]\n",
				TMManDevice->NameSpaceSharedAddress ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:SGBufferShared[%lx]\n",
				TMManDevice->SGBufferSharedAddress ));
            
			halGetTMPCIInfo ( 
				TMManDevice->HalHandle,
				&DeviceVendorID,
				&SubsystemID,
				&ClassRevisionID );
            
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TM:DeviceVendorID[%lx]\n",
				DeviceVendorID));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TM:SubsystemID[%lx]\n",
				SubsystemID));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TM:ClassRevisionID[%lx]\n",
				ClassRevisionID));
			
			halGetBridgePCIInfo ( 
				TMManDevice->HalHandle,
				&DeviceVendorID,
				&SubsystemID,
				&ClassRevisionID );
            
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:Bridge:DeviceVendorID[%lx]\n",
				DeviceVendorID));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:Bridge:SubsystemID[%lx]\n",
				SubsystemID));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:Bridge:ClassRevisionID[%lx]\n",
				ClassRevisionID));
			
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TargetTraceBufferSize[%lx]\n",
				TMManGlobal->TargetTraceBufferSize  ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TargetTraceLeveBitmap[%lx]\n",
				TMManGlobal->TargetTraceLeveBitmap ));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TargetTraceType[%lx]\n",
				TMManGlobal->TargetTraceType ));
            
			//            DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:DebuggingHi[%x]\n",
			//                SystemTime.HighPart ));
			//            DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:DebuggingLo[%x]\n",
			//                SystemTime.LowPart ));
            
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManMailboxCount[%lx]\n",
				TMManGlobal->MailboxCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManChannelCount[%lx]\n",
				TMManGlobal->ChannelCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManVIntrCount[%lx]\n",
				TMManGlobal->VIntrCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManMessageCount[%lx]\n",
				TMManGlobal->MessageCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManEventCount[%lx]\n",
				TMManGlobal->EventCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManStreamCount[%lx]\n",
				TMManGlobal->StreamCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManNameSpaceCount[%lx]\n",
				TMManGlobal->NameSpaceCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManMemoryCount[%lx]\n",
				TMManGlobal->MemoryCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManMemorySize[%lx]\n",
				TMManGlobal->MemorySize));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:TMManSGBufferCount[%lx]\n",
				TMManGlobal->SGBufferCount));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:SpeculativeLoadFix[%lx]\n",
				TMManGlobal->SpeculativeLoadFix));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:PCIInterruptNumber[%lx]\n",
				TMManGlobal->PCIInterruptNumber));
			DPF(1,("tmman:tmmanDeviceControl:constIOCTLtmmanDSPStart:MMIOInterruptNumber[%lx]\n",
				TMManGlobal->MMIOInterruptNumber));
            
			halReset ( TMManDevice->HalHandle );
			channelManagerReset ( TMManDevice->ChannelManagerHandle );
			vintrManagerReset (	TMManDevice->VIntrManagerHandle );
			eventManagerReset (	TMManDevice->EventManagerHandle );
			namespaceManagerReset (	TMManDevice->NameSpaceManagerHandle );
			
			memoryManagerReset ( TMManDevice->MemoryManagerHandle );
			// DL  sgbufferManagerReset ( TMManDevice->SGBufferManagerHandle );
            
            
			TMIF->Status = 	halStartDSP ( TMManDevice->HalHandle );
			
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPStart, end\n")); 
			
			
      }
      break;
	  
	  
      // OF DSP Stop
    case	constIOCTLtmmanDSPStop :
		{
			
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			
			
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			// tmifGenericFunction*    TMIF = (tmifGenericFunction*)(pParamBlock->pData);
			TMManDevice = (TMManDeviceObject*)TMIF->Handle;
			
			TMManDevice->Status = constTMManDSPStatusReset;
			TMIF->Status = halStopDSP ( TMManDevice->HalHandle );
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPStop, end\n"));    
		}
		break;
		
		// OF DSP Reset
    case	constIOCTLtmmanDSPReset :
		{
			tmifGenericFunction*	TMIF =
				(tmifGenericFunction*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = (TMManDeviceObject*)TMIF->Handle;
			
			TMManDevice->Status = constTMManDSPStatusReset;
			TMIF->Status = halResetDSP( TMManDevice->HalHandle);
		}
		break;
		
		// OF Get DSP Internal Info
    case constIOCTLtmmanDSPGetInternalInfo:
		{       
			tmifDSPInternalInfo* TMIF;
			tmifDSPInternalInfo  TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			UInt32               ClientIdx;
			/*
			unused variable
			UInt32               DeviceIdx;
			*/
			/*
			initialization to NULL to remove compiler warning
			I have NOT checked if 'Client' might be used before being initialized
			*/
			ClientObject*        Client = NULL;
			
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPGetInternalInfo\n"));
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDSPInternalInfo)
				);
			TMIF=&TMIF_kernelspace;
			
			// tmifDSPInternalInfo*	TMIF = (tmifDSPInternalInfo*)(pParamBlock->pData);
			TMManDevice = (TMManDeviceObject*)(TMIF->DSPHandle);
			
			
			
			for (ClientIdx = 0; 
			ClientIdx < TMManGlobal->MaximumClients; 
			ClientIdx++ )
			{
				if ( !TMManGlobal->ClientList[ClientIdx] )
					continue;
                
				Client = TMManGlobal->ClientList[ClientIdx];
				
				if ( Client->Process != pIoParms->tid )
					continue;
				break;
			}
            
			if ( ClientIdx == TMManGlobal->MaximumClients )
			{
				DPF(0,("tmman:tmmanDeviceControl:PANIC:tmmanDSPInfo:InvalidHandle:Process[%lx]\n", 
					pIoParms->tid));
				TMIF->Status = statusInvalidHandle;
				break;
			}
            
			TMIF->Info.Flags = TMManDevice->Flags;
			TMIF->Info.CRunTimeHandle = TMManDevice->CRunTimeHandle;
			TMIF->Info.DebugOptionBitmap = 0;
			
			
            
			halGetPeerVersion ( 
				TMManDevice->HalHandle,
				&TMIF->Info.PeerMajorVersion,
				&TMIF->Info.PeerMinorVersion );
			
			// BUGCHECK : should we swap endianess here
			TMIF->Info.TMManSharedPhysicalAddress = 
				TMManDevice->TMManSharedAddress;
			
			TMIF->Info.Memory.MappedAddress = 
				(UInt32)Client->Device[TMManDevice->DSPNumber].MemoryAddrUser;
			TMIF->Info.Memory.PhysicalAddress = 
				(UInt32)((TMManDeviceObject*)Client->Device[TMManDevice->DSPNumber].Device)->MemoryBlockAddress;
			TMIF->Info.Memory.Size = 
				(UInt32)((TMManDeviceObject*)Client->Device[TMManDevice->DSPNumber].Device)->MemoryBlockSize;
            
			TMIF->Status = statusSuccess;
			
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDSPInternalInfo));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanDSPInternalInfo, end\n"));
		}
		break;
		
		
		// OF Set DSP Internal Info
    case constIOCTLtmmanDSPSetInternalInfo :
		{
			tmifDSPInternalInfo*	TMIF =
				(tmifDSPInternalInfo*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = 
				(TMManDeviceObject*)TMIF->DSPHandle;
			
			// this function should change on the alterable fields
			TMManDevice->Flags = TMIF->Info.Flags;
			TMManDevice->CRunTimeHandle = TMIF->Info.CRunTimeHandle;
            
			TMIF->Status = statusSuccess;
		}
		break;
		
		
		// OF Get DSP Status
    case constIOCTLtmmanDSPStatus:
		{
			tmifDSPStatus*	TMIF =
				(tmifDSPStatus*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = 
				(TMManDeviceObject*)TMIF->DSPHandle;
			
			TMIF->DSPStatus = TMManDevice->Status;
			TMIF->Status = statusSuccess;
		}
		break;
		
		// OF Get DSP Endianess
    case constIOCTLtmmanDSPGetEndianess : // vxd callable
		{    
			tmifDSPEndianess*	TMIF =
				(tmifDSPEndianess*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			Bool		SwapEnable;
            
			TMIF->Status = halGetEndianess ( 
				TMManDevice->HalHandle,  
				&SwapEnable );
            
			// DTO: Host Endianness
			TMIF->Endianess = 
				( ( SwapEnable ) ? 
				( constTMManEndianessBig ) : 
            ( constTMManEndianessLittle ) );
		}
		break;
		
		
		//---------------------------------------------------------------------
		//	Message
		//---------------------------------------------------------------------
		
		// OF Message create
    case constIOCTLtmmanMessageCreate : 
		{
			
			tmifMessageCreate*     TMIF;
			tmifMessageCreate      TMIF_kernelspace;
			TMManDeviceObject*     TMManDevice;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifMessageCreate)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifMessageCreate* TMIF = (tmifMessageCreate*)(pParamBlock->pData);*/
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			
			TMIF->Status = messageCreate ( 
				TMManDevice->MessageManagerHandle,
				(Pointer) pIoParms->tid, // client listhead struct (context???)
				TMIF->Name,
				TMIF->SynchObject,
				TMIF->SynchFlags,
				&TMIF->MessageHandle );
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifMessageCreate));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanMessageCreate,end\n"));    
		}
		break;
		
		
		// OF Message Destroy
    case constIOCTLtmmanMessageDestroy :
		{
			tmifGenericFunction*     TMIF;
			tmifGenericFunction      TMIF_kernelspace;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			
			
			/* tmifGenericFunction*	TMIF =
			(tmifGenericFunction*)(pParamBlock->pData); */
            
			TMIF->Status = messageDestroy (
				TMIF->Handle ); 
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanMessageDestroy,end\n"));    
			
		}
		break;
		
		
		// OF Message Send
    case constIOCTLtmmanMessageSend :
		{
			tmifMessageSR*     TMIF;
			tmifMessageSR*     TMIF_kernelspace;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifMessageSR)
				);
			
			TMIF=TMIF_kernelspace;
			/* tmifMessageSR*	TMIF = (tmifMessageSR*)(pParamBlock->pData); */
            
			TMIF->Status = messageSend (
				TMIF->MessageHandle,
				TMIF->Packet );
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifMessageSR));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanMessageSend,end\n"));    
			
		}
		break;
		
		
		// OF Message Receive
    case constIOCTLtmmanMessageReceive :
		{
			tmifMessageSR*     TMIF;
			tmifMessageSR*     TMIF_kernelspace;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifMessageSR)
				);
			
			TMIF=TMIF_kernelspace;
			/* tmifMessageSR*	TMIF = (tmifMessageSR*)(pParamBlock->pData); */
            
			TMIF->Status = messageReceive ( 
				TMIF->MessageHandle,
				TMIF->Packet );
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifMessageSR));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanMessageReceive,end\n"));    
		}
		break;
		
		// tmmMessageWait()
    case constIOCTLtmmanMessageBlock :
		{
			tmifGenericFunction*     TMIF;
			tmifGenericFunction      TMIF_kernelspace;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifGenericFunction*	TMIF =
			(tmifGenericFunction*)(pParamBlock->pData); */
			DPF(0,("tmif.c: case constMessageBlock: messageWait is going to be called.,end\n"));   
			TMIF->Status = messageWait (TMIF->Handle); 
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanMessageBlock,end\n"));    
			
		}
		break;
		
		
		
		
		
		
		//---------------------------------------------------------------------
		//	Event
		//---------------------------------------------------------------------
		
		// OF Event Create
    case constIOCTLtmmanEventCreate :
		{
			tmifEventCreate*     TMIF;
			tmifEventCreate      TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifEventCreate)
				);
			
			TMIF=&TMIF_kernelspace;
			
			
			/* tmifEventCreate*   TMIF = (tmifEventCreate*)(pParamBlock->pData); */
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			
			DPF(1,("constIOCTLtmmanEventCreate called \n"));
			TMIF->Status = eventCreate ( 
				TMManDevice->EventManagerHandle,
				(Pointer)pIoParms->tid, // client listhead struct ???
				TMIF->Name,
				TMIF->SynchObject,
				TMIF->SynchFlags,
				&TMIF->EventHandle );
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifEventCreate));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanEventCreate,end\n"));    
		}
		break;
		
		
		// OF Event Destroy
    case constIOCTLtmmanEventDestroy :
		{        
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			/*
			unused variable
			TMManDeviceObject*   TMManDevice;
			*/
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifGenericFunction*   TMIF = 
			(tmifGenericFunction*)(pParamBlock->pData);*/
            
			TMIF->Status = eventDestroy (TMIF->Handle);
			
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanEventDestroy, end\n")); 
		}
		break;
		
		
		// OF Event Signal
    case constIOCTLtmmanEventSignal :
		{
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifGenericFunction*	TMIF =
			(tmifGenericFunction*)(pParamBlock->pData); */
            
			TMIF->Status = eventSignal (TMIF->Handle);
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanEventSignal, end\n")); 
		}
		break;
		
    case constIOCTLtmmanEventBlock :
		{
			tmifGenericFunction* TMIF;
			tmifGenericFunction  TMIF_kernelspace;
			
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifGenericFunction*	TMIF =
			(tmifGenericFunction*)(pParamBlock->pData); */
			
			TMIF->Status = eventWait(TMIF->Handle);    
			// TMIF->Status = eventSignal (TMIF->Handle);
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanBlock, end\n")); 
		}
		break;
		
		
		//---------------------------------------------------------------------
		//	Debugging 
		//---------------------------------------------------------------------
		
		// OF Get Debug DP Buffers
    case constIOCTLtmmanGetDebugDPBuffers :
		{
			tmifDebugBuffers*    TMIF;
			tmifDebugBuffers     TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			
			/*
			unused variable
			UInt32               ClientIdx;
			UInt32               DeviceIdx;
			*/
			UInt32		     Dummy;
			Bool		     Status;
			UInt32		     SDRAMKernelBaseAddress;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDebugBuffers)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifDebugBuffers*	TMIF = (tmifDebugBuffers*)(pParamBlock->pData);*/
			
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			
			
			
			Status = debugDPBuffers (
				TMManDevice->HalHandle,
				&TMIF->FirstHalfPtr, 
				&TMIF->FirstHalfSize, 
				&TMIF->SecondHalfPtr, 
				&TMIF->SecondHalfSize );
			
			if ( Status == True )
			{
				TMIF->Status = statusSuccess;                
			}
			else
			{
				// we have nothing so exit out
				DPF(0,("tmif.c: case DPBuffers: debugDPBuffers returned an error\n"));
				TMIF->Status = statusDebugNoDebugInformation;
				copy_to_user(pParamBlock->pData
					,TMIF
					,sizeof(tmifDebugBuffers));
				break;
			}
			
			DPF(0,("tmif.c: case: DPBuffers1: TMIF->FirstHalfPtr  is 0x%p\n", TMIF->FirstHalfPtr));
            DPF(0,("tmif.c: case: DPBuffers1: TMIF->FirstHalfSize is 0x%lx\n", TMIF->FirstHalfSize));
			DPF(0,("tmif.c: case: DPBuffers1: TMIF->SecondHalf    is 0x%p\n", TMIF->SecondHalfPtr));
			DPF(0,("tmif.c: case: DPBuffers1: TMIF->SecondHalfPtr is 0x%lx\n", TMIF->SecondHalfSize));
			
			
            
			halGetSDRAMInfo (
				TMManDevice->HalHandle,
				(Pointer*)&Dummy,
				(Pointer*)&SDRAMKernelBaseAddress,
				&Dummy );
			
			if ( TMIF->FirstHalfPtr )
			{
				TMIF->FirstHalfPtr = 
					TMIF->FirstHalfPtr - SDRAMKernelBaseAddress;
			}
			
			if ( TMIF->SecondHalfPtr )
			{
				TMIF->SecondHalfPtr = 
					TMIF->SecondHalfPtr - SDRAMKernelBaseAddress;
			}
			DPF(0,("tmif.c: case: DPBuffers2: TMIF->FirstHalfPtr  is 0x%p\n", TMIF->FirstHalfPtr));
            DPF(0,("tmif.c: case: DPBuffers2: TMIF->FirstHalfSize is 0x%lx\n", TMIF->FirstHalfSize));
			DPF(0,("tmif.c: case: DPBuffers2: TMIF->SecondHalf    is 0x%p\n", TMIF->SecondHalfPtr));
			DPF(0,("tmif.c: case: DPBuffers2: TMIF->SecondHalfPtr is 0x%lx\n", TMIF->SecondHalfSize));
			
			
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDebugBuffers));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanGetDebugDPBuffers, end\n")); 
		}
		break;
		
		
		// OF Get Debug Target Buffers
    case constIOCTLtmmanGetDebugTargetBuffers :
		{
			
			tmifDebugBuffers* TMIF;
			tmifDebugBuffers  TMIF_kernelspace;
			TMManDeviceObject*   TMManDevice;
			
			/*
			unused variable
			UInt32          ClientIdx;
			*/
			UInt32			Dummy;
			Bool			Status;
			UInt32			SDRAMKernelBaseAddress;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDebugBuffers)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifDebugBuffers* TMIF = (tmifDebugBuffers*)(pParamBlock->pData); */
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;    
			
			Status = debugTargetBuffers (
				TMManDevice->HalHandle,
				&TMIF->FirstHalfPtr, 
				&TMIF->FirstHalfSize, 
				&TMIF->SecondHalfPtr, 
				&TMIF->SecondHalfSize );
            
            
			if ( Status == True )
			{
				TMIF->Status = statusSuccess;
			}
			else
			{
				// we have nothing so exit out
				DPF(0, ("tmif.c: case targetBuffers: debugTargetBuffers returned an error.\n"));
				TMIF->Status = statusDebugNoDebugInformation;
				copy_to_user(pParamBlock->pData
					,TMIF
					,sizeof(tmifDebugBuffers));
				break;
			}
			
			halGetSDRAMInfo (
				TMManDevice->HalHandle,
				(Pointer*)&Dummy,
				(Pointer*)&SDRAMKernelBaseAddress,
				&Dummy );
            
			if ( TMIF->FirstHalfPtr )
			{
				TMIF->FirstHalfPtr = 
					TMIF->FirstHalfPtr - SDRAMKernelBaseAddress;
			}
            
			if ( TMIF->SecondHalfPtr )
			{
				TMIF->SecondHalfPtr = 
					TMIF->SecondHalfPtr - SDRAMKernelBaseAddress;
			}
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDebugBuffers));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanGetDebugTargetBuffers, end\n"));     
		}
		break;
		
		
		// OF Get Debug Host Buffers
    case constIOCTLtmmanGetDebugHostBuffers :
		{
			
			tmifDebugBuffers*    TMIF;
			tmifDebugBuffers     TMIF_kernelspace;
			/*
			unused variable
			TMManDeviceObject*   TMManDevice;
			*/
			Bool                 Status;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifDebugBuffers)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifDebugBuffers*  TMIF = (tmifDebugBuffers*)(pParamBlock->pData); */
			
            
			Status = debugHostBuffers (
				&TMIF->FirstHalfPtr, 
				&TMIF->FirstHalfSize, 
				&TMIF->SecondHalfPtr, 
				&TMIF->SecondHalfSize );
			
			TMIF->Status = ( Status == True ) ? 
statusSuccess : statusDebugNoDebugInformation;
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifDebugBuffers));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanGetDebugHostBuffers, end\n"));
		}
		break;
		
		/* OF Debug Get Printf - not implemented in TMMANAPI.c
		case constIOCTLtmmanGetDebugPrintf : 
		{
		//appliction stuff is printed only if level 31 is enabled.
		DPF(31,( (pParamBlock->pData) ));
		}
		break;
		*/
		
		//---------------------------------------------------------------------
		//	Shared Memory
		//---------------------------------------------------------------------
		
		// OF Shared Memory Create
    case constIOCTLtmmanSharedMemoryCreate :
		{
			tmifSharedMemoryCreate*    TMIF;
			tmifSharedMemoryCreate     TMIF_kernelspace;
			TMManDeviceObject*         TMManDevice;
			/*
			unused variable
			Bool                       Status;
			*/
			UInt8*			   KernelMemoryAddress;
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifSharedMemoryCreate)
				);
			
			TMIF=&TMIF_kernelspace;
			
			/* tmifSharedMemoryCreate* TMIF = (tmifSharedMemoryCreate*)(pParamBlock->pData); */
			TMManDevice = (TMManDeviceObject*)TMIF->DSPHandle;
			
			TMIF->Status = memoryCreate(
				TMManDevice->MemoryManagerHandle,
				(Pointer)pIoParms->tid, // client listhead struct
				TMIF->Name, 
				TMIF->Size, 
				(Pointer *)&KernelMemoryAddress,
				&TMIF->SharedMemoryHandle);
            
            
			if ( TMIF->Status == statusSuccess )
			{ 
				DPF(0,("tmif.c: sharedmemCreate: memoryCreate returned successfully.\n"));
				TMIF->Address = (Pointer)KernelMemoryAddress;
			}
			else
			{
				DPF(0,("tmif.c: sharedmemCreate: memoryCreate returned an error 0x%lx.\n", TMIF->Status));
			}
			
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifSharedMemoryCreate));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanSharedMemoryCreate, end\n"));
		}
		break;
		
		
		// OF Shared Memory Destroy
    case constIOCTLtmmanSharedMemoryDestroy :
		{
			tmifGenericFunction*    TMIF;
			tmifGenericFunction     TMIF_kernelspace;
			/*
			unused variable
			TMManDeviceObject*      TMManDevice;
			Bool                    Status;
			*/
			
			copy_from_user(  &TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifGenericFunction)
				);
			
			TMIF=&TMIF_kernelspace;
			
			
			// tmifGenericFunction*	TMIF = (tmifGenericFunction*)(pParamBlock->pData);
            
			TMIF->Status = memoryDestroy(TMIF->Handle);
			copy_to_user(pParamBlock->pData
				,TMIF
				,sizeof(tmifGenericFunction));
			DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanSharedMemoryDestroy, end\n"));
			
		}
		break;
		
		
		// OF Shared Memory Get Address
    case constIOCTLtmmanSharedMemoryGetAddress :
		{
			tmifSharedMemoryAddress*  TMIF;
			tmifSharedMemoryAddress   TMIF_kernelspace;
			/*
			unused variables
			TMManDeviceObject*        TMManDevice;
			Bool                      Status;
			*/
			UInt32                    PhysicalAddress;
			UInt32		          KernelMemoryAddress;
			UInt32		          HalHandle;
			UInt32*                   addressSharedMemory_dummy;
			
			copy_from_user(&TMIF_kernelspace
				, pParamBlock->pData
				, sizeof(tmifSharedMemoryAddress)
				);
			
			
			TMIF=&TMIF_kernelspace;
			
			
			/* tmifSharedMemoryAddress*	TMIF =
			(tmifSharedMemoryAddress*)(pParamBlock->pData); */
            
			TMIF->Status = memoryGetAddress (
				TMIF->SharedMemoryHandle,
				&KernelMemoryAddress );
            
			if ( TMIF->Status == statusSuccess )
			{
				DPF(1,("tmif.c: constSharedMemoryGetAddress: KernelMemoryAddress is 0x%lx\n"
					, KernelMemoryAddress));
				PhysicalAddress = MmGetPhysicalAddress(
					(Pointer)KernelMemoryAddress);
				
				DPF(1,("tmif.c: constSharedMemoryGetAddress: PhysicalAddress is 0x%lx\n"
					, PhysicalAddress));
				addressSharedMemory_dummy= (UInt32 *) PhysicalAddress;
				DPF(1,("tmif.c: constSharedMemoryGetAddress: The shared Memory contains %ld, %ld and %ld\n"
					, addressSharedMemory_dummy[0]
					, addressSharedMemory_dummy[1]
					, addressSharedMemory_dummy[2] ));
				
				addressSharedMemory_dummy[0]=1000;
				addressSharedMemory_dummy[1]=2000;
				addressSharedMemory_dummy[2]=3000;
				
				memoryGetHalHandle ( 
					TMIF->SharedMemoryHandle,
					&HalHandle );
				
				TMIF->PhysicalAddress = halTranslateTargetPhysicalAddress ( 
					HalHandle, 
					PhysicalAddress );
				
				DPF(1,("tmif.c: constSharedMemoryGetAddress: TMIF->PhysicalAddress(bus) is 0x%lx\n"
					, TMIF->PhysicalAddress));
				copy_to_user(pParamBlock->pData
					,TMIF
					,sizeof(tmifGenericFunction));
				DPF(0,("tmif.c:tmmCntrl: case constIOCTLtmmanSharedMemoryDestroy, end\n"));
				
			}
		}
		break;
		
		
		//---------------------------------------------------------------------
		//	DMA Buffer Locking
		//---------------------------------------------------------------------
		
		// OF SG Buffer Create
    case constIOCTLtmmanSGBufferCreate :
		{
            /*
            unused variable
			tmifSGBufferCreate*	TMIF = 
				(tmifSGBufferCreate*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = 
				(TMManDeviceObject*)TMIF->DSPHandle;
            */
            
				/* DL   TMIF->Status = sgbufferCreate (
				TMManDevice->SGBufferManagerHandle,
				(Pointer)pIoParms->tid, // client listhead struct
				TMIF->Name,
				TMIF->MappedAddress,
				TMIF->Size,
				TMIF->Flags,
				&TMIF->SGBufferHandle );
			*/            
		}
		break;
		
		
		// OF SG Buffer Destroy
		/* DL
		case constIOCTLtmmanSGBufferDestroy :
		{
		tmifGenericFunction*	TMIF =
		(tmifGenericFunction*)(pParamBlock->pData);
		
		 TMIF->Status = sgbufferDestroy ( TMIF->Handle );            
		 }
		*/
		break;
		
		/*
		// the following 2 DEVIOCTLS are protected in user 
		// mode by a Global Mutex.
		case constIOCTLtmmanXlateAdapterAddress1 : // vxd callable
		{
		tmifGenericFunction*	TMIF =
		(tmifGenericFunction*)(pParamBlock->pData);
		
		 DWORD		AdapterPhysicalAddress;
		 DWORD       dwSize;
		 
		  //store the tranalted address in a temporary global variable
		  KernelIoControl( ( TMManGlobal->OEMIOCTLBase + TMM_IOCTL_GET_PHYS_ADDRESS_OFFSET ),
		  lpBufOut,
		  dwLenOut,
		  (PVOID)&AdapterPhysicalAddress,
		  sizeof(DWORD),
		  &dwSize);
		  
		   TMManGlobal->XlatedAddress = AdapterPhysicalAddress;
		   // nothing to be returned as the output buffer is METHOD_OUT_DIRECT.
		   *pdwActualOut = dwLenOut;
		   
			}
			break;
			
			 case constIOCTLtmmanXlateAdapterAddress2 : // vxd callable
			 {
			 tmifGenericFunction*	TMIF =
			 (tmifGenericFunction*)(pParamBlock->pData);
			 
			  // retrieved the stored address
			  TMIF->Handle = TMManGlobal->XlatedAddress;
			  TMIF->Status = statusSuccess;
			  
			   *pdwActualOut = sizeof ( tmifGenericFunction );
			   
				}
				break;
				
		*/
		// OF Map SDRAM
    case constIOCTLtmmanMapSDRAM :
		{
			tmifGenericFunction*	TMIF =
				(tmifGenericFunction*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = (TMManDeviceObject*)TMIF->Handle;
			
			TMIF->Status = statusSuccess;
            
			if ( TMManGlobal->MapSDRAM )
				break;
            
			/* DTO: for WinCE we don't need to map SDRAM into user mode here*/
			if ( halMapSDRAM ( TMManDevice->HalHandle ) != True )
			{
				TMIF->Status = statusMemoryUnavailable;
				DPF(0,("tmman:tmmanOpen:halMapAdapterMemory:SDRAM:FAIL\n" ));
				break;
			}
		}
		break;
		
		// OF Unmap SDRAM
    case constIOCTLtmmanUnmapSDRAM :
		{
			tmifGenericFunction*	TMIF =
				(tmifGenericFunction*)(pParamBlock->pData);
			TMManDeviceObject* TMManDevice = (TMManDeviceObject*)TMIF->Handle;
			TMIF->Status = statusSuccess;
			
			if ( TMManGlobal->MapSDRAM )
				break;
			
			/* for WinCE we don't need to unmap SDRAM from user mode here */
			halUnmapSDRAM ( TMManDevice->HalHandle );
		}
		break;
		
    default :
		break;
    }
	// ENDCASE
	
	/* DL: translate things back to user space */
	copy_to_user( pIoParms_userspace
		, pIoParms
		, sizeof(ioparms));
}




//
//		Interface implementations
//		Have to put in code for resources and version.h

TMStatus	tmmanKernelModeNegotiateVersion ( 
											 tmmanVersion* Version )
{
	TMStatus	Status = statusSuccess;
	
	if ( Version->Major < verGetFileMajorVersion() )
    {
		DPF(0,("tmman:tmmanKernelModeNegotiateVersion:MajorVersion:FAIL\n"));
		Status = statusMajorVersionError;
		goto tmifNegotiateVersionExit1;
    }
	
	if ( Version->Minor < verGetFileMinorVersion()  )
    {
		DPF(0,("tmman:tmmanKernelModeNegotiateVersion:MinorVersion:FAIL\n"));
		Status = statusMinorVersionError;
		goto tmifNegotiateVersionExit1;
    }
	
tmifNegotiateVersionExit1 :
    Version->Major = verGetFileMajorVersion();
	Version->Minor = verGetFileMinorVersion();
	Version->Build = verGetFileBuildVersion();
	return Status;
}

