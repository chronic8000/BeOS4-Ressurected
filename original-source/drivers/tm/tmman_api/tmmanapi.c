/*
	tmmanapi.c
	
	Provides services to pSOS Applications for communicating with the DSP.

	HISTORY
	960417	Tilakraj Roy	Created
	960620	Tilakraj Roy	Started changes for release candicate 3.
	961015	Tilakraj Roy	Adding ArgC & ArgV support to tmDSpLoadExecutable
	970229	Tilakraj Roy	Removed thread and event semantics for NT. Made it KISS
	970812	Tilakraj Roy	Ported to generic TmMan interface V5.0
	980306	Volker Schildwach	Ported to Windows CE
*/

//-----------------------------------------------------------------------------
// Standard include files:
//

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
//-----------------------------------------------------------------------------
// Project include files:
//
#include "tmmanapi.h"
#include "tmif.h"
#include "verinfo.h"

#include "TMDownLoader.h"
#include "TMObj.h"


#include <unistd.h>
#include <errno.h>

//#undef __KERNEL_STRICT_NAMES


//-----------------------------------------------------------------------------
// Types and defines:
//
#define tmmanDownloaderStatusToTMManStatus(x) \
	(((x)!=TMDwnLdr_OK)?(x+0x70):statusSuccess)

//#define DPRINTF(_x_)
#define DPRINTF(_x_)   printf _x_

#define DEV_TMMAN 1             //dummy
//-----------------------------------------------------------------------------
// Global data and external function/data references:
//
UInt8       szTemp[0x80];

static Int         global_file_descriptor;

//int *       user_space_SDRAMAddress;

//-----------------------------------------------------------------------------
// Prototypes:
//

String
errorGetErrorString (
	UInt32  ErrorCode );



//-----------------------------------------------------------------------------
// Exported functions:
//

//-----------------------------------------------------------------------------
//	API IMPLEMENTATION 
//-----------------------------------------------------------------------------

// DSP related API.

void Init_TMMan_API_Lib()
{
    global_file_descriptor = open("/dev/tm0",O_RDWR);
}

UInt32 de_cntrl(UInt32 dev, void * IoParmBlock, void *retVal)
{
   /* IoParmBlock is of the type tTMMANIo */
   int      cmd;
   ioparms *pIoparms;
   int      rc;

	/* unused parameters */
	(void)dev;
	(void)retVal;

   cmd = ioctl(global_file_descriptor //_IOWR(TRIMEDIA_IOC_MAGIC
              , (int) (((tTMMANIo *)IoParmBlock)->controlCode)
              , sizeof(tTMMANIo)) ;
   printf("tmmanapi.c: de_cntrl: cmd is 0x%x and  global_file_descriptor is 0x%x\n"
          ,cmd,global_file_descriptor);
   pIoparms          = (ioparms *)malloc(sizeof(ioparms));
   pIoparms->in_iopb = IoParmBlock;
   pIoparms->tid     = (UInt32)getpid();
   pIoparms->err     = 0;
   
   rc=ioctl(global_file_descriptor, 488 /*cmd*/ ,pIoparms);
   if (rc)
   { 
     printf("tmmanapi.c: de_cntrl: rc is %d\n",rc);
     perror("tmmanapi.c: de_cntrl: ioctl");
   }
 
   return 0;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanNegotiateVersion:
//
// DESCRIPTION:  This function performs a version negotiation with the
//               specified TMMan component.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanNegotiateVersion ( 
	UInt32	       ModuleID, //    IN: Module Identification of the TMMan 
                                 //        component whose version needs to be
		                 //        verified
	tmmanVersion*  Version   // INOUT: Inputs the host version and outputs
    )                            //        the version of the TMMAN component
{
    TMStatus			Status = statusSuccess;
    tmifNegotiateVersion	TMIF;
    tmmanVersion	        ReturnedVersion;
    tTMMANIo                    IoParmBlock;
    UInt32                      retVal;
    
    
    switch ( ModuleID )
    {
        case constTMManModuleHostUser :
        case constTMManDefault :

            ReturnedVersion.Major = verGetFileMajorVersion ();
            ReturnedVersion.Minor = verGetFileMinorVersion ();
            ReturnedVersion.Build = verGetFileBuildVersion ();
        
            if ( Version->Major != verGetFileMajorVersion ()  )
            {
                Status = statusMajorVersionError;
                goto tmmanNegotiateVersionExit1;
            }
        
            if ( Version->Minor != verGetFileMinorVersion () )
            {
                Status = statusMinorVersionError;
                goto tmmanNegotiateVersionExit1;
            }
            break;
        
        case constTMManModuleHostKernel :
	  printf("tmmanapi.c: tmmanDSPNegotiateVersion: case constTMManModuleHostKernel\n");
            TMIF.Version.Major = Version->Major;
            TMIF.Version.Minor = Version->Minor;
            TMIF.Version.Build = Version->Build;
            TMIF.ModuleID = ModuleID;
        
            // perform the version control 
            IoParmBlock.controlCode = constIOCTLtmmanNegotiateVersion;
            IoParmBlock.pData = (void *)(&TMIF);

            // Call the tmman driver
            if (de_cntrl (
                DEV_TMMAN,  
                (void *)&IoParmBlock, 
                (void *)&retVal))
            {
	      printf("tmmanapi.c: tmmanDSPNegotiateVersion:de_cntrl failed.\n");
                TMIF.Status = statusDeviceIoCtlFail;
                return TMIF.Status;
            }            
            Status = TMIF.Status;
            ReturnedVersion.Major = TMIF.Version.Major;
            ReturnedVersion.Minor = TMIF.Version.Minor;
            ReturnedVersion.Build = TMIF.Version.Build;
            printf("tmmanapi.c: tmmanDSPNegotiateVersion: ReturnedVersion.major is %ld"
                  ,ReturnedVersion.Major);
            break;
        
        case constTMManModuleTargetKernel :
        case constTMManModuleTargetUser :

        default :
            Status  = statusUnknownComponent;
            break;
    }
    
    
tmmanNegotiateVersionExit1:
    printf("tmmanapi.c: tmmanDSPNegotiateVersion: Exit1");
    Version->Major = ReturnedVersion.Major;
    Version->Minor = ReturnedVersion.Minor;
    Version->Build = ReturnedVersion.Build;
    return Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPGetNum:
//
// DESCRIPTION:  This function Returns the number of TriMedia processors 
//               installed in the system.
//
// RETURN:       UInt32:  the number of TriMedia
//               
// NOTES:        Can only be 1 under present implmentation
//-----------------------------------------------------------------------------
//
UInt32
tmmanDSPGetNum ( 
    void 
    )
{
    tmifDSPNum        TMIF;
    tTMMANIo          IoParmBlock;
    UInt32            retVal;
    

    IoParmBlock.controlCode = constIOCTLtmmanDSPGetNum;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
        printf("tmmanapi.c:tmmanDSPGetNum: de_cntrl failed.\n");
    }            
    printf("tmmanapi.c: tmmanDSPGetNum: just before return.\n");
    printf("tmmanapi.c: tmmanDSPGetNum: TMIF.DSPCount is %ld\n",TMIF.DSPCount);
    return  TMIF.DSPCount;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPGetInfo:
//
// DESCRIPTION:  This function Retrieves the properties of the specified 
//               TriMedia processor.
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPGetInfo ( 
	UInt32         DSPHandle,   //  IN:  Handle to the DSP 
	tmmanDSPInfo*  DSPInfo      // OUT:  Return DSP Info structure
    )
{

    tmifDSPInfo	     TMIF;
    tTMMANIo         IoParmBlock;
    UInt32           retVal;
    static int       mapped = 0;
    /*
    int              prot   = PROT_READ | PROT_WRITE;
    int              flags  = MAP_SHARED;
    off_t            offset = 0;
    */
    static void      *user_space_SDRAMAddress;
    static void      *user_space_MMIOAddress;

    TMIF.DSPHandle          = DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanDSPInfo;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }
        
    *DSPInfo = TMIF.Info;
    /*DL: now the DSP info contains information about SDRAM and MMIO
      i.e. their physical address and their mapped address. However,
      this mapped addresses is only valid in kernel space, therefore these
      addresses are remapped to user space.
    */
  if (!mapped)
    {
      printf("tmmanapi.c: tmmanPhysicalToMapped: just before mmap.\n");
    	/*
    
      user_space_SDRAMAddress = mmap( 0
                                    , DSPInfo->SDRAM.Size
                                    , prot
                                    , flags
                                    , global_file_descriptor
                                    , offset);
      printf("tmmanapi.c: tmmanGetInfo: mmap returned 0x%x\n"
              ,user_space_SDRAMAddress);
     user_space_MMIOAddress = mmap( 0
                                   , DSPInfo->MMIO.Size
                                   , prot
                                   , flags
                                   , global_file_descriptor
                                   , offset);
      printf("tmmanapi.c: tmmanGetInfo: mmap returned 0x%x\n"
              ,user_space_MMIOAddress);
      	*/
      		
     	ioctl(global_file_descriptor, 489, &user_space_SDRAMAddress);
     	ioctl(global_file_descriptor, 490, &user_space_MMIOAddress);

      mapped = 1;
    }
  DSPInfo->SDRAM.MappedAddress = (unsigned long int)user_space_SDRAMAddress;
  DSPInfo->MMIO.MappedAddress  = (unsigned long int)user_space_MMIOAddress;

    return TMIF.Status;
}



// DSP FUNCTIONS
//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPGetStatus:
//
// DESCRIPTION:  This function gets the current state of the specified 
//               TriMedia Processor
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPGetStatus ( 
    UInt32  DSPHandle,      //  IN:  Handle returned by tmmanDSPOpen
    UInt32* StatusFlags     // OUT:  Status value
    )
{
    tmifDSPStatus     TMIF;
    tTMMANIo          IoParmBlock;
    UInt32            retVal;
    
    
    TMIF.DSPHandle = DSPHandle;
    
    
    IoParmBlock.controlCode = constIOCTLtmmanDSPStatus;
    IoParmBlock.pData       = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
        return TMIF.Status;
    }
    
    *StatusFlags = TMIF.DSPStatus;
    
    return statusSuccess;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPGetEndianess:
//
// DESCRIPTION:  This function gets the current endianess of the specified 
//               TriMedia Processor
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus 
tmmanDSPGetEndianess ( 
    UInt32  DSPHandle,     //  IN:  Handle returned by tmmanDSPOpen
    UInt32* EndianFlags    // OUT:  Endian value
    )
{
    tmifDSPEndianess	TMIF;
    tTMMANIo            IoParmBlock;
    UInt32              retVal;
    
    
    TMIF.DSPHandle          = DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanDSPGetEndianess;
    IoParmBlock.pData       = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
        return TMIF.Status;
    }

    *EndianFlags = TMIF.Endianess;

    return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPGetInternalInfo:
//
// DESCRIPTION:  This function returns various internal TMMan values. It
//               is for internal use only and should not be used by 
//               Applications
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPGetInternalInfo ( 
	UInt32                DSPHandle, //  IN:  Handle returned by tmmanDSPOpen
	tmmanDSPInternalInfo* DSPInfo    // OUT:  DSP Internal info 
    )
{
    tmifDSPInternalInfo	TMIF;
    tTMMANIo            IoParmBlock;
    UInt32              retVal;


    TMIF.DSPHandle = DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanDSPGetInternalInfo;
    IoParmBlock.pData       = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }

	*DSPInfo = TMIF.Info;

    return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPSetInternalInfo:
//
// DESCRIPTION:  This function is used to set the modifable elements
//               of the InternalInfo structure, flags & c-runtime handle
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPSetInternalInfo ( 
	UInt32                DSPHandle,  // IN:  Handle returned by tmmanDSPOpen
	tmmanDSPInternalInfo* DSPInfo     // IN:  DSP Info structure with required
    )                                     //      settings
{
    tmifDSPInternalInfo	 TMIF;
    tTMMANIo             IoParmBlock;
    UInt32               retVal;
    
    
    TMIF.DSPHandle = DSPHandle;
    TMIF.Info      = *DSPInfo;
    
    IoParmBlock.controlCode = constIOCTLtmmanDSPSetInternalInfo;
    IoParmBlock.pData       = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }
    
    return TMIF.Status;
}
	


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPOpen:
//
// DESCRIPTION:  This function opens the given TriMedia Processor.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPOpen ( 
	UInt32  DSPNumber,       //  IN:  Number of the TriMedia processor that 
                                 //       needs to be opened.
	UInt32* DSPHandlePointer // OUT:  Handle for the opened DSP
    )
{
    tmifDSPOpen	      TMIF;
    tTMMANIo          IoParmBlock;
    UInt32            retVal;


    TMIF.DSPNumber          = DSPNumber;

    IoParmBlock.controlCode = constIOCTLtmmanDSPOpen;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }

    *DSPHandlePointer = TMIF.DSPHandle;

    return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPClose:
//
// DESCRIPTION:  This function  closes the given handle to the TriMedia 
//               processor.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPClose ( 
	UInt32 DSPHandle   // IN:  Handle returned by tmmanDSPOpen
    )
{
    tmifGenericFunction	 TMIF;
    tTMMANIo             IoParmBlock;
    UInt32               retVal;

    
    TMIF.Handle	= DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanDSPClose;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }

	return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPMapSDRAM:
//
// DESCRIPTION:  Maps SDRAM into Operating System and Process virtual 
//               address space
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPMapSDRAM ( 
	UInt32   DSPHandle   // IN:  Handle returned by tmmanDSPOpen
    )
{
    tmifGenericFunction	    TMIF;
    tTMMANIo                IoParmBlock;
    UInt32                  retVal;


    TMIF.Handle		    = DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanMapSDRAM;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }

    return TMIF.Status;
}

//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPUnmapSDRAM:
//
// DESCRIPTION:  Unmaps SDRAM from Process virtual address space.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus  
tmmanDSPUnmapSDRAM ( 
	UInt32 DSPHandle   // IN:  Handle returned by tmmanDSPOpen
    )
{
    tmifGenericFunction	    TMIF;
    tTMMANIo                IoParmBlock;
    UInt32                  retVal;


    TMIF.Handle		    = DSPHandle;

    IoParmBlock.controlCode = constIOCTLtmmanUnmapSDRAM;
    IoParmBlock.pData       = (void *)(&TMIF);

    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }

    return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPLoad:
//
// DESCRIPTION:  Load a boot image onto the DSP.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

TMStatus
tmmanDSPLoad ( 
	      UInt32 DSPHandle,   // IN:  Handle returned by tmmanDSPOpen
	      UInt32 LoadAddress, // IN:  Address of SDRAM where the executable should be 
	                          //      downloaded. To use the default values use 
	                          //      constTMManDefault.
	      UInt8* ImagePath    // IN:  String containing image
	      )
{
  tmmanDSPInfo		  DSPInfo;
  tmmanDSPInternalInfo	  DSPInternalInfo;
  TMStatus      	  Status  = statusSuccess;
  tmifDSPLoad		  TMIF;
  TMDwnLdr_Object_Handle  ObjectHandle;
  tTMMANIo                IoParmBlock;
  UInt32                  retVal;

  Int	                  ImageSize;
  Int	                  Alignment;
  UInt32	          AlignedDownloadAddress;
  UInt32	          DownLoaderStatus;
  Endian	          endian;
  UInt32	          TargetVersion;
  UInt32                  SharedPhysicalAddr;

  // registry stuff
  UInt32	          ClockSpeed   =  100000000;
  UInt32	          CacheOption  = TMDwnLdr_LeaveCachingToDownloader;
  //UInt32                CacheOption  = TMDwnLdr_CachesOff;

  //UInt32                HostType     = tmNoHost;
  UInt32                  HostType     = tmWinNTHost;
  UInt32                  INIEndianess = True; // little endian
  //UInt32  INIEndianess = False; // big endian

  //DL:
  /*
  unused variable
  int * user_space_aligned_address;
  */

  /*??? */
  if ( ( Status = tmmanDSPMapSDRAM ( DSPHandle ) ) != statusSuccess )
    {
      DPRINTF (("tmman32:tmmanDSPMapSDRAM::FAIL\n"));
      goto tmmanDSPLoadExit1;		
    }

  tmmanDSPGetInfo ( 
		   DSPHandle, 
		   &DSPInfo );

  DPRINTF(("DSPInfo.SDRAM.MappedAddress:   0x%lx\n", DSPInfo.SDRAM.MappedAddress));
  DPRINTF(("DSPInfo.SDRAM.PhysicalAddress: 0x%lx\n", DSPInfo.SDRAM.PhysicalAddress));
  DPRINTF(("DSPInfo.SDRAM.Size:            0x%lx\n", DSPInfo.SDRAM.Size));

  tmmanDSPGetInternalInfo ( 
			   DSPHandle, 
			   &DSPInternalInfo );
	
  if ( LoadAddress == constTMManDefault )
    {
      LoadAddress = DSPInfo.SDRAM.PhysicalAddress;
      DPRINTF(("Load address = 0x%lx\n", LoadAddress));
    }

  if ( ( DownLoaderStatus = TMDwnLdr_load_object_from_file ( 
							    ImagePath,
							    NULL, 
							    &ObjectHandle ) ) != TMDwnLdr_OK )
    {
      Status = tmmanDownloaderStatusToTMManStatus(DownLoaderStatus);
      DPRINTF(("tmmanDSPLoadExit2: TMDwnLdr_load_object_from_file() error\n"));
      goto tmmanDSPLoadExit2;
    }
  DPRINTF(("Load Object from file success\n"));

  // BEGIN symbol patching
  SharedPhysicalAddr = DSPInternalInfo.TMManSharedPhysicalAddress;
  if ( ( TMDwnLdr_resolve_symbol( 
				 ObjectHandle, 
				 "_TMManShared", 
				 SharedPhysicalAddr ) ) != TMDwnLdr_OK )
    {
      DPRINTF(("tmmanDSPLoad:WARNING:TMDwnLdr_resolve_symbol[_TMManShared]:FAIL\n"));
      HostType = tmNoHost;

      // Target link (host) option incorrect
    }
  DPRINTF(("Resolved symbol [_TMManShared] with 0x%lx\n", 
	   SharedPhysicalAddr));
    


  if ( ( DownLoaderStatus = TMDwnLdr_get_image_size ( ObjectHandle, 
						      &ImageSize, &Alignment 	) )!= TMDwnLdr_OK )
    {
      Status = tmmanDownloaderStatusToTMManStatus(DownLoaderStatus);
      DPRINTF(("tmmanDSPLoadExit3: TMDwnLdr_get_image_size() error\n"));
      goto tmmanDSPLoadExit3; 
    }

  DPRINTF(("TMDwnLdr_get_image_size() success: 0x%x\n", ImageSize));

  AlignedDownloadAddress = ( (LoadAddress + Alignment -1 ) & 
			     ( ~( Alignment - 1 ) ) );

  DPRINTF(("AlignedDownloadAddress = 0x%lx\n", AlignedDownloadAddress));

  if ( ( DownLoaderStatus = TMDwnLdr_relocate  ( 
						ObjectHandle, 
						HostType,
						(Address)DSPInfo.MMIO.PhysicalAddress, // mmio base
						ClockSpeed,
						(Address)AlignedDownloadAddress, //physical adderss of download
						DSPInfo.SDRAM.Size,
						CacheOption ) ) != TMDwnLdr_OK )
    {
      Status = tmmanDownloaderStatusToTMManStatus(DownLoaderStatus);
      DPRINTF(("tmmanDSPLoadExit3: TMDwnLdr_relocate() error: 0x%lx\n", 
	       DownLoaderStatus));
      goto tmmanDSPLoadExit3; 
    }

  DPRINTF(("TMDwnLdr_relocate() success\n"));

  if ( ( DownLoaderStatus = TMDwnLdr_get_endian ( ObjectHandle,
						  &endian )) != TMDwnLdr_OK )
    {
      Status = tmmanDownloaderStatusToTMManStatus(DownLoaderStatus);
      DPRINTF(("tmmanDSPLoadExit3: TMDwnLdr_get_endian() error\n"));
      goto tmmanDSPLoadExit3; 
    }
  DPRINTF(("TMDwnLdr_get_endian() success: %s", (endian ? ("Little Endian\n") : ("Big Endian\n"))));

  // 1 = little endian & 0 = big endian
  // LittleEndian = 1 & BigEndian = 0
  if ( INIEndianess != (unsigned)endian )
    {
      Status = statusExecutableFileWrongEndianness;
      DPRINTF(("tmmanDSPLoadExit3: Wrong Endianess\n"));
      goto tmmanDSPLoadExit3;
    }

  // we know what is the target executables endianess
  // call the kernel mode driver to reconfigure itself in the correct
  // endianess

  TMIF.DSPHandle = DSPHandle;
  TMIF.Endianess = ( endian == LittleEndian ) ? 
    constTMManEndianessLittle : constTMManEndianessBig;

  if ( ( TMDwnLdr_get_contents ( 
				ObjectHandle, 
				"__TMMan_Version",
				&TargetVersion ) )  != TMDwnLdr_OK )
    {
      TMIF.PeerMajorVersion	= constTMManDefaultVersionMajor; 
      TMIF.PeerMinorVersion	= constTMManDefaultVersionMinor;
    }
  else
    {
      // major version = __TMMan_Version[31:16]
      // minor version = __TMMan_Version[15:0]
      TMIF.PeerMajorVersion = ( ( TargetVersion  & 0xffff0000 ) >> 16 ) ;
      TMIF.PeerMinorVersion = TargetVersion & ( 0x0000ffff );

      if ( ( TMIF.PeerMajorVersion != verGetFileMajorVersion() ) || 
	   ( TMIF.PeerMinorVersion != verGetFileMinorVersion() ) )
	{
	  DPRINTF (("Target Executable Version [%ld.%ld] is INCOMPATIBLE with TriMedia Driver Version [%ld.%ld]\n",
		    TMIF.PeerMajorVersion, 
                    TMIF.PeerMinorVersion, 
                    verGetFileMajorVersion(), 
                    verGetFileMinorVersion() ));
	}
    }

  IoParmBlock.controlCode = constIOCTLtmmanDSPLoad;
  IoParmBlock.pData = (void *)(&TMIF);

  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      Status = TMIF.Status = statusDeviceIoCtlFail;
      goto tmmanDSPLoadExit3; 
    }
  //	Status = TMIF.Status;
  //	goto tmmanDSPLoadExit3; 

  DPRINTF(("Driver call success\n"));



  // BUGCHECK - adjust this based on download address
  if ( ( DownLoaderStatus = TMDwnLdr_get_memory_image ( 
						       ObjectHandle, 
						       (UInt8*)tmmanPhysicalToMapped (&DSPInfo.SDRAM, AlignedDownloadAddress)) ) != TMDwnLdr_OK )
    {
      Status = tmmanDownloaderStatusToTMManStatus(DownLoaderStatus);
      DPRINTF(("tmmanDSPLoadExit3: TMDwnLdr_get_memory_image() error\n"));
      goto tmmanDSPLoadExit3; 
    }


  DPRINTF(("tmmanDSPLoad() complete\n"));

  tmmanDSPLoadExit3 :
    TMDwnLdr_unload_object ( ObjectHandle );

  tmmanDSPLoadExit2 :
    tmmanDSPUnmapSDRAM ( DSPHandle );

  tmmanDSPLoadExit1 :
    return Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPStart:
//
// DESCRIPTION:  This function just unresets the DSP. The DSP starts executing 
//               code at SDRAM base.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPStart ( 
	       UInt32 DSPHandle   // IN:  Handle returned by tmmanDSPOpen
	       )
{
  tmifGenericFunction	TMIF;
  tTMMANIo              IoParmBlock;
  UInt32                retVal;
    
  TMIF.Handle = DSPHandle;
    
  IoParmBlock.controlCode = constIOCTLtmmanDSPStart;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPStop:
//
// DESCRIPTION:  Puts the CPU into a reset state.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPStop ( 
	      UInt32 DSPHandle   // IN:  Handle returned by tmmanDSPOpen
	      )
{
  tmifGenericFunction	TMIF;
  tTMMANIo              IoParmBlock;
  UInt32                retVal;
    
  TMIF.Handle             = DSPHandle;      
  IoParmBlock.controlCode = constIOCTLtmmanDSPStop;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }

  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDSPReset:
//
// DESCRIPTION:  Initializes the TriMedia Processor after it has been manually 
//               reset via the reset button or	other means. 
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDSPReset ( 
	       UInt32 DSPHandle   // IN:  Handle returned by tmmanDSPOpen
	       )
{
  tmifGenericFunction	TMIF;
  tTMMANIo            IoParmBlock;
  UInt32              retVal;
    
  TMIF.Handle             = DSPHandle;   
  IoParmBlock.controlCode = constIOCTLtmmanDSPReset;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }

  return TMIF.Status;
}

// DEBUGGING INTERFACES 

//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDebugOptions:
//
// DESCRIPTION:  This function is used to set the Debug Option bitmap
//               
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus  
tmmanDebugOptions (
		   UInt32	DSPHandle,   // IN:  Handle returned by tmmanDSPOpen
		   UInt32	Option       // IN:  New debug option bitmap
		   )
{
  TMStatus	          Status;
  tmmanDSPInternalInfo    DSPInternalInfo;
    
  Status = tmmanDSPGetInternalInfo ( DSPHandle, &DSPInternalInfo );
  if ( Status != statusSuccess )
    return Status;
    
  DSPInternalInfo.DebugOptionBitmap = Option;
    
  return tmmanDSPSetInternalInfo ( DSPHandle, &DSPInternalInfo );
}

//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDebugDPBuffers:
//
// DESCRIPTION:  This function retrieves the target DP debug buffer
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDebugDPBuffers (
		     UInt32	DSPHandle,        //  IN:  Handle returned by tmmanDSPOpen
		     UInt8*	*FirstHalfPtr,    // OUT:  Pointer to the first part of the buffer
		     UInt32*	FirstHalfSizePtr, // OUT:  Size of the first part
		     UInt8*	*SecondHalfPtr,   // OUT:  Pointer to the second part of the buffer
		     UInt32*	SecondHalfSizePtr // OUT:  Size of the second part
		     )
{
  tmifDebugBuffers  TMIF;
  tTMMANIo          IoParmBlock;
  UInt32            retVal;
  tmmanDSPInfo      DSPInfo;
  TMStatus          Status;
  void             *user_space_SDRAMAddress;


  TMIF.DSPHandle = DSPHandle;

  IoParmBlock.controlCode = constIOCTLtmmanGetDebugDPBuffers;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
      return TMIF.Status;
    }

  if ( ( Status  = tmmanDSPGetInfo ( 
				    DSPHandle, 
				    &DSPInfo ) ) != statusSuccess )
    {
      return Status;
    }
  user_space_SDRAMAddress = (void *)DSPInfo.SDRAM.MappedAddress;

  if ( TMIF.FirstHalfPtr )
    {
      /*DL	*FirstHalfPtr		= 
	(UInt8*)((UInt32)DSPInfo.SDRAM.MappedAddress + 
	(UInt32)TMIF.FirstHalfPtr );
      */
      *FirstHalfPtr		= 
	(UInt8*)((UInt32)user_space_SDRAMAddress + 
		 (UInt32)TMIF.FirstHalfPtr );
    }


  if ( TMIF.SecondHalfPtr )
    {
      /*DL	*SecondHalfPtr		= 
	(UInt8*)((UInt32)DSPInfo.SDRAM.MappedAddress + 
	(UInt32)TMIF.SecondHalfPtr );
      */
      *SecondHalfPtr = 
	(UInt8*)((UInt32)user_space_SDRAMAddress + 
		 (UInt32)TMIF.SecondHalfPtr );
    }
       
  printf("tmmanapi.c: DPBuffers: user_space_SDRAMAddress is 0x%p\n", user_space_SDRAMAddress);
  printf("tmmanapi.c: DPBuffers: TMIF.FirstHalfPtr   is 0x%p\n", TMIF.FirstHalfPtr);
  printf("tmmanapi.c: DPBuffers: TMIF.SecondHalfPtr  is 0x%p\n", TMIF.SecondHalfPtr);
  printf("tmmanapi.c: DPBuffers: TMIF.FirstHalfSize  is 0x%lx\n", TMIF.FirstHalfSize);
  printf("tmmanapi.c: DPBuffers: TMIF.SecondHalfSize is 0x%lx\n", TMIF.SecondHalfSize);
  printf("tmmanapi.c: DPBuffers: *FirstHalfPtr       is 0x%p\n", *FirstHalfPtr);
  printf("tmmanapi.c: DPBuffers: *SecondHalfPtr      is 0x%p\n", *SecondHalfPtr);

  *FirstHalfSizePtr	= TMIF.FirstHalfSize; 
  *SecondHalfSizePtr	= TMIF.SecondHalfSize;

  return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDebugHostBuffers:
//
// DESCRIPTION:  This function retrieves the Host debug buffer
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDebugHostBuffers (
		       UInt8*	*FirstHalfPtr,    // OUT:  Pointer to the first part of the buffer
		       UInt32*	FirstHalfSizePtr, // OUT:  Size of the first part
		       UInt8*	*SecondHalfPtr,   // OUT:  Pointer to the second part of the buffer
		       UInt32*	SecondHalfSizePtr // OUT:  Size of the second part
		       )
{
  tmifDebugBuffers  TMIF;
  tTMMANIo          IoParmBlock;
  UInt32            retVal;
    
  IoParmBlock.controlCode = constIOCTLtmmanGetDebugHostBuffers;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN, 
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      *FirstHalfPtr		= TMIF.FirstHalfPtr;
      *FirstHalfSizePtr	        = TMIF.FirstHalfSize; 
      *SecondHalfPtr		= TMIF.SecondHalfPtr;
      *SecondHalfSizePtr	= TMIF.SecondHalfSize;
    }

  return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDebugTargetBuffers:
//
// DESCRIPTION:  This function retrieves the Target 'DPF' debug buffer
//               for the given TriMedia
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDebugTargetBuffers (
			 UInt32	 DSPHandle,        //  IN:  Handle returned by tmmanDSPOpen
			 UInt8*	 *FirstHalfPtr,    // OUT:  Pointer to the first part of the buffer
			 UInt32* FirstHalfSizePtr, // OUT:  Size of the first part
			 UInt8*	 *SecondHalfPtr,   // OUT:  Pointer to the second part of the buffer
			 UInt32* SecondHalfSizePtr // OUT:  Size of the second part
			 )
{
  tmifDebugBuffers  TMIF;
  tTMMANIo          IoParmBlock;
  UInt32            retVal;
  TMStatus          Status;
  tmmanDSPInfo      DSPInfo;
  void             *user_space_SDRAMAddress;
    
  TMIF.DSPHandle = DSPHandle;
    
  IoParmBlock.controlCode = constIOCTLtmmanGetDebugTargetBuffers;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN, 
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
      return TMIF.Status;
    }

  if ( ( Status  = tmmanDSPGetInfo ( 
				    DSPHandle, 
				    &DSPInfo ) ) != statusSuccess )
    {
      return Status;
    }
  user_space_SDRAMAddress = (void *)DSPInfo.SDRAM.MappedAddress;    


  if ( TMIF.FirstHalfPtr )
    {
      /*DL	*FirstHalfPtr		= 
	(UInt8*)((UInt32)DSPInfo.SDRAM.MappedAddress + 
	(UInt32)TMIF.FirstHalfPtr );
      */
      *FirstHalfPtr		= 
	(UInt8*)((UInt32)user_space_SDRAMAddress + 
		 (UInt32)TMIF.FirstHalfPtr );
    }

  if ( TMIF.SecondHalfPtr )
    {
      /*	*SecondHalfPtr		= 
		(UInt8*)((UInt32)DSPInfo.SDRAM.MappedAddress + 
		(UInt32)TMIF.SecondHalfPtr );
      */
      *SecondHalfPtr		= 
	(UInt8*)((UInt32)user_space_SDRAMAddress + 
		 (UInt32)TMIF.SecondHalfPtr );
    }

  printf("tmmanapi.c: TargetBuffers: user_space_SDRAMAddress is 0x%p\n", user_space_SDRAMAddress);
  printf("tmmanapi.c: TargetBuffers: TMIF.FirstHalfPtr   is 0x%p\n", TMIF.FirstHalfPtr);
  printf("tmmanapi.c: TargetBuffers: TMIF.SecondHalfPtr  is 0x%p\n", TMIF.SecondHalfPtr);
  printf("tmmanapi.c: TargetBuffers: TMIF.FirstHalfSize  is 0x%lx\n", TMIF.FirstHalfSize);
  printf("tmmanapi.c: TargetBuffers: TMIF.SecondHalfSize is 0x%lx\n", TMIF.SecondHalfSize);
  printf("tmmanapi.c: TargetBuffers: *FirstHalfPtr       is 0x%p\n", *FirstHalfPtr);
  printf("tmmanapi.c: TargetBuffers: *SecondHalfPtr      is 0x%p\n", *SecondHalfPtr);



  *FirstHalfSizePtr	= TMIF.FirstHalfSize; 
  *SecondHalfSizePtr	= TMIF.SecondHalfSize;
    
  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanDebugPrintf:
//
// DESCRIPTION:  This function is used to print formatted strings via the 
//               debugging subsystem of TMMan.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanDebugPrintf ( Int8 *Form, ... )
{
  UInt8	 DebugBuffer[constTMManDebugStringLength];
  UInt32 Sentinel = 0xdeadbeef;
  UInt32 StringLength;
  UInt8  *Format  = (UInt8 *)Form;

  va_list va;

  tTMMANIo          IoParmBlock;
  UInt32            retVal;

    
  va_start(va, Form);
  vsprintf( DebugBuffer, Format, va);
  va_end(va);
    
  if ( Sentinel != 0xdeadbeef )
    {
      return statusStringBufferOverflow;
    }
    
  StringLength = strlen ( DebugBuffer );
    
  IoParmBlock.controlCode = constIOCTLtmmanDebugPrintf;
  IoParmBlock.pData = (void *)(DebugBuffer);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      return statusDeviceIoCtlFail;
    }
    
  return statusSuccess;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanMappedToPhysical:
//
// DESCRIPTION:  Translates a Operating System Mapped TriMedia SDRAM or MMIO 
//               address to a physical address.
//               For usage information see: tmmanapi.h
//
// RETURN:       UInt32:  The Physical address
//               
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt32
tmmanMappedToPhysical ( 
		       tmmanMemoryBlock* MemoryBlock,  // IN:  Memory block struct containing
		                                       //      physical & mapped base addresses
		       UInt32            MappedAddress // IN:  Mapped address to translate
		       )
{
  return MemoryBlock->PhysicalAddress + 
    (MappedAddress - MemoryBlock->MappedAddress);
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanPhysicalToMapped:
//
// DESCRIPTION:  Translates a SDRAM or MMIO physical address to an operating 
//               system mapped address.
//               For usage information see: tmmanapi.h
//
// RETURN:       UInt32:  The Mapped address
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
UInt32
tmmanPhysicalToMapped ( 
		   tmmanMemoryBlock* MemoryBlock,    // IN:  Memory block struct containing
	                                             //      physical & mapped base addresses
	           UInt32            PhysicalAddress // IN:  Physical address to translate
		       )
{ 
 
  printf("tmmanapi.c: tmmanPhysicalToMapped: MappedAddress = 0x%lx"
         , MemoryBlock->MappedAddress );
  printf("tmmanapi.c: tmmanPhysicalToMapped: PhysicalAddress = 0x%lx"
         , MemoryBlock->PhysicalAddress );
  return MemoryBlock->MappedAddress + 
    (PhysicalAddress - MemoryBlock->PhysicalAddress); 
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanValidateAddressAndLength:
//
// DESCRIPTION:  Checks if the given physical address and length lies between 
//               bound of the given memory block. This function works for 
//               SDRAM and MMIO addresses only.
//               For usage information see: tmmanapi.h
//
// RETURN:       Bool:  True on success and False otherwise 
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
Bool
tmmanValidateAddressAndLength ( 
			tmmanMemoryBlock*  MemoryBlock,  // IN:  Memory block struct containing
			                                 //      physical & mapped base addresses
			UInt32             Address,      // IN:  Physical address that needs to be 
			                                 //      checked.
			UInt32             Length        // IN:  Length of the block that needs to 
			                                 //      be checked.
			       )
{
  // assumes that MMIO and SDRAM are not contigious.
  if ( ( Address >= MemoryBlock->PhysicalAddress ) &&
       ( Address < (MemoryBlock->PhysicalAddress + MemoryBlock->Size - 1) ) )
    {
      if ( ( Address + Length ) <=
	   (MemoryBlock->PhysicalAddress + MemoryBlock->Size - 1))
	return True;
      else
	return False;
    }
  return False;
    
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanTranslateAdapterAddress:
//
// DESCRIPTION:  Not available on this platform
//
// RETURN:       Bool:  always False
//               
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
Bool
tmmanTranslateAdapterAddress ( 
			      UInt32 MappedAddress,
			      UInt32 Length,
			      UInt32 *PhysicalAddressPtr 
			      )
{
	/* unused parameters */
	(void)MappedAddress;
	(void)Length;
	(void)PhysicalAddressPtr;
	
	
  return False; // statusUnsupportedOnThisPlatform
}
  


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanGetErrorString:
//
// DESCRIPTION:  This function returns a string corresponding to the given
//               error code
//               For usage information see: tmmanapi.h
//
// RETURN:       String:  String version of error code
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
String
tmmanGetErrorString (
		     TMStatus	StatusCode  // IN:  error code
		     )
{
  return  errorGetErrorString( StatusCode );
}




//-----------------------------------------------------------------------------
// FUNCTION:     tmmanMessageCreate:
//
// DESCRIPTION:  Creates a bi-directional message channel between the host and 
//               the target processor.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanMessageCreate ( 
		    UInt32	DSPHandle,             //  IN:  Handle returned by tmmanDSPOpen
		    UInt8*	Name,                  //  IN:  Unique name for the msg channel
		    UInt32	SynchronizationHandle, //  IN:  OS specific synchronization object
		    UInt32	SynchronizationFlags,  //  IN:  Sync Handle interperetion flag
		    UInt32*     MessageHandlePointer   // OUT:  Handle to the Msg Channel created
		    )
{
  tmifMessageCreate TMIF;
  tTMMANIo          IoParmBlock;
  UInt32            retVal;


  TMIF.DSPHandle = DSPHandle;

  if ( strlen ( Name ) >= constTMManNameSpaceNameLength )
    {
      return statusNameSpaceLengthExceeded;
    }

  strcpy ( TMIF.Name, Name );
	
  TMIF.SynchObject = SynchronizationHandle;
  TMIF.SynchFlags  = SynchronizationFlags;

  IoParmBlock.controlCode = constIOCTLtmmanMessageCreate;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      *MessageHandlePointer = TMIF.MessageHandle;
    }

  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanMessageDestroy:
//
// DESCRIPTION:  Closes the message channel associated with the given handle
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanMessageDestroy ( 
		     UInt32 MessageHandle //  IN:  Msg Handle returned by tmmanMessageCreate()
		     )
{
  tmifGenericFunction	TMIF;
  tTMMANIo              IoParmBlock;
  UInt32                retVal;

  TMIF.Handle = MessageHandle;

  IoParmBlock.controlCode = constIOCTLtmmanMessageDestroy;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }

  return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanMessageSend:
//
// DESCRIPTION:  This function sends a fixed size data packet of type 
//               tmmanPacket to the TriMedia.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanMessageSend ( 
		  UInt32  MessageHandle, //  IN:  Msg Handle returned by tmmanMessageCreate()
		  void   *DataPointer    //  IN:  tmmanPacket data structure to be sent
		  )
{
  tmifMessageSR    TMIF;
  tTMMANIo         IoParmBlock;
  UInt32           retVal;
    

  TMIF.MessageHandle = MessageHandle;
    
  // the user is supposed to take care of endianess
  memcpy ( TMIF.Packet, DataPointer, constTMManPacketSize );
    
  IoParmBlock.controlCode = constIOCTLtmmanMessageSend;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanMessageReceive:
//
// DESCRIPTION:  This function retrieves a packet from the incoming packet
//               queue.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanMessageReceive ( 
		     UInt32 MessageHandle, //  IN:  Msg Handle returned by tmmanMessageCreate()
		     void  *DataPointer    // OUT:  tmmanPacket received
		     )
{
  tmifMessageSR    TMIF;
  tTMMANIo         IoParmBlock;
  UInt32           retVal;
    
    
  TMIF.MessageHandle = MessageHandle;
    
  IoParmBlock.controlCode = constIOCTLtmmanMessageReceive;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      // the user is supposed to take care of endianess
      memcpy (DataPointer, TMIF.Packet, constTMManPacketSize);
    }
    
  return TMIF.Status;
}





//DL This function is not part of the tmman API. 
TMStatus tmmMessageWait( 
			UInt32 MessageHandle // IN:  Event handle returned by tmmanEventCreate()
			)
{
  tmifGenericFunction  TMIF;
  tTMMANIo             IoParmBlock;
  UInt32               retVal;
  // UInt32            IOCTLtmmanWaitSignal = 0xd0;    

  TMIF.Handle = MessageHandle;    
    
  IoParmBlock.controlCode = constIOCTLtmmanMessageBlock;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}











//  Event API
//-----------------------------------------------------------------------------
// FUNCTION:     tmmanEventCreate:
//
// DESCRIPTION:  This function creates an event object which can be used
//               to signal the TriMedia
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanEventCreate ( 
		  UInt32   DSPHandle,             //  IN:  Handle returned by tmmanDSPOpen
		  UInt8*   Name,                  //  IN:  Unique name for the event
		  UInt32   SynchronizationHandle, //  IN:  OS specific synchronization object
		  UInt32   SynchronizationFlags,  //  IN:  Sync Handle interperetion flag
		  UInt32*  EventHandlePointer     // OUT:  Handle to the event object created
		  )
{
  tmifEventCreate      TMIF;
  tTMMANIo             IoParmBlock;
  UInt32               retVal;
    
    
  TMIF.DSPHandle = DSPHandle;
    
  if ( strlen ( Name ) >= constTMManNameSpaceNameLength )
    {
      return statusNameSpaceLengthExceeded;
    }
    
  strcpy ( TMIF.Name, Name );
  TMIF.SynchObject = SynchronizationHandle;
  TMIF.SynchFlags  = SynchronizationFlags;
    
  IoParmBlock.controlCode = constIOCTLtmmanEventCreate;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      *EventHandlePointer = TMIF.EventHandle;
    }
    
  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanEventDestroy:
//
// DESCRIPTION:  This function removes the Event object associated with the
//               given handle.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanEventDestroy ( 
		   UInt32 EventHandle    // IN:  Event handle returned by tmmanEventCreate()
		   )
{
  tmifGenericFunction TMIF;
  tTMMANIo            IoParmBlock;
  UInt32              retVal;
    
    
  TMIF.Handle = EventHandle;
    
  IoParmBlock.controlCode = constIOCTLtmmanEventDestroy;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanEventSignal:
//
// DESCRIPTION:  This function causes the OS specific synchronization object 
//               on the TriMedia to be signaled.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanEventSignal ( 
		  UInt32 EventHandle // IN:  Event handle returned by tmmanEventCreate()
		  )
{
  tmifGenericFunction  TMIF;
  tTMMANIo             IoParmBlock;
  UInt32               retVal;
    
  TMIF.Handle = EventHandle;
    
    
  IoParmBlock.controlCode = constIOCTLtmmanEventSignal;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}


//DL
TMStatus tmmEventWait( 
		      UInt32 EventHandle // IN:  Event handle returned by tmmanEventCreate()
		      )
{
  tmifGenericFunction  TMIF;
  tTMMANIo             IoParmBlock;
  UInt32               retVal;
  // UInt32               IOCTLtmmanWaitSignal = 0xd0;    

  TMIF.Handle = EventHandle;    
    
  IoParmBlock.controlCode = constIOCTLtmmanEventBlock;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
    
  return TMIF.Status;
}

//-----------------------------------------------------------------------------
// FUNCTION:     tmmanSharedMemoryCreate:
//
// DESCRIPTION:  
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanSharedMemoryCreate ( 
			 UInt32	 DSPHandle,                //  IN:Handle returned by tmmanDSPOpen
			 UInt8*	 Name,                     //  IN:Unique name for the shared mem
			 UInt32	 Length,                   //  IN:Length of memory block to be
			                                   //     allocated
			 UInt32* AddressPointer,           //  IN:Address of the memory location 
			                                   //     where the pointer to the shared
			                                   //     memory will be stored.
			 UInt32* SharedMemoryHandlePointer // OUT:Handle to the shared memory 
			 )                                 //     object created
{
  tmifSharedMemoryCreate  TMIF;
  tTMMANIo                IoParmBlock;
  UInt32                  retVal;
  tmifDSPInfo             DSPInfo;


  TMIF.DSPHandle = DSPHandle;

  if ( strlen ( Name ) >= constTMManNameSpaceNameLength )
    {
      return statusNameSpaceLengthExceeded;
    }

  DSPInfo.DSPHandle = DSPHandle;

  IoParmBlock.controlCode = constIOCTLtmmanDSPInfo;
  IoParmBlock.pData       = (void *)(&DSPInfo);

  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
      return TMIF.Status;
    }


  strcpy ( TMIF.Name, Name );
  TMIF.Size = Length;

  IoParmBlock.controlCode = constIOCTLtmmanSharedMemoryCreate;
  IoParmBlock.pData       = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      printf("de_cntrl for sharedmemory create has returned unsuccessfully.\n");
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      printf("de_cntrl for sharedmemory create has returned successfully.\n");
      printf("de_cntrl: sharedmemory: TMIF.Address is 0x%lx and sharedMemoryHandle is 0x%lx.\n",
               (UInt32)(TMIF.Address), TMIF.SharedMemoryHandle);
      *AddressPointer            = (UInt32)TMIF.Address;
      printf("de_cntrl for sharedmemory create has returned successfully2.\n");
      *SharedMemoryHandlePointer = TMIF.SharedMemoryHandle;
      printf("de_cntrl for sharedmemory create has returned successfully3.\n");
    }

  return TMIF.Status;
}



//-----------------------------------------------------------------------------
// FUNCTION:     tmmanSharedMemoryGetAddress:
//
// DESCRIPTION:  This function retrieves the physical address for the
//               start of the shared memory block
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanSharedMemoryGetAddress ( 
			     UInt32	SharedMemoryHandle,    //  IN:  Handle returned by 
			     //       tmmanSharedMemoryCreate()
			     UInt32*	PhysicalAddressPointer // OUT:  Physical address
			     )
{
  tmifSharedMemoryAddress  TMIF;
  tTMMANIo                 IoParmBlock;
  UInt32                   retVal;


  TMIF.SharedMemoryHandle = SharedMemoryHandle;

  IoParmBlock.controlCode = constIOCTLtmmanSharedMemoryGetAddress;
  IoParmBlock.pData = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }
  else
    {
      *PhysicalAddressPointer = TMIF.PhysicalAddress;
    }

  return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanSharedMemoryDestroy:
//
// DESCRIPTION:  Closes the Shared Memory handle and frees up the shared 
//               memory that was allocated
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanSharedMemoryDestroy ( 
			  UInt32	SharedMemoryHandle     //  IN:  Handle returned by 
			  //       tmmanSharedMemoryCreate()
			  )
{
  tmifGenericFunction  TMIF;
  tTMMANIo             IoParmBlock;
  UInt32       retVal;


  TMIF.Handle = SharedMemoryHandle;

  IoParmBlock.controlCode = constIOCTLtmmanSharedMemoryDestroy;
  IoParmBlock.pData = (void *)(&TMIF);
    
  // Call the tmman driver
  if (de_cntrl (
		DEV_TMMAN,  
		(void *)&IoParmBlock, 
		(void *)&retVal))
    {
      TMIF.Status = statusDeviceIoCtlFail;
    }

  return TMIF.Status;
}




//-----------------------------------------------------------------------------
// FUNCTION:     tmmanSGBufferCreate:
//
// DESCRIPTION:  This function page locks the specified memory and generates 
//               a page frame table that can be used by the target to access 
//               the page locked memory.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        This feature doesn't really make sense under pSOS as it 
//               doesn't use memory pages. However the feature is provided
//               for compatibility.
//-----------------------------------------------------------------------------
//	


/* DL:

TMStatus  tmmanSGBufferCreate ( 
	UInt32	DSPHandle,           //  IN:  Handle returned by tmmanDSPOpen
	UInt8*	Name,                //  IN:  Unique name for SG Buffer Object
	UInt32	MappedAddress,       //  IN:  Address of memory block to be locked
	UInt32	Size,                //  IN:  Size of memory block
	UInt32	Flags,               //  IN:  Object flags
	UInt32* BufferHandlePointer  // OUT:  Handle to SB Buffer object created
    )
{
    tmifSGBufferCreate	TMIF;
    tTMMANIo             IoParmBlock;
    UInt32       retVal;
    
    
    TMIF.DSPHandle = DSPHandle;
        
    if ( strlen ( Name ) >= constTMManNameSpaceNameLength )
    {
        return statusNameSpaceLengthExceeded;
    }
    
    strcpy ( TMIF.Name, Name );
    
    TMIF.Size			= Size;
    TMIF.MappedAddress	= MappedAddress;
    TMIF.Flags			= Flags;
    
    IoParmBlock.controlCode = constIOCTLtmmanSGBufferCreate;
    IoParmBlock.pData = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }
    else
    {
        *BufferHandlePointer = TMIF.SGBufferHandle;
    }
    
    return TMIF.Status;
}


//-----------------------------------------------------------------------------
// FUNCTION:     tmmanSGBufferDestroy:
//
// DESCRIPTION:  Closes the handle tp the page locked memory and unlocks
//               the memory and frees up the page frame tables.
//               For usage information see: tmmanapi.h
//
// RETURN:       TMStatus:  statusSuccess if there is no error 
//               otherwise the appropriate error code.
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
TMStatus
tmmanSGBufferDestroy ( 
	UInt32	BufferHandle  // IN:  Handle to the SG Buffer Object
    )
{
    tmifGenericFunction  TMIF;
    tTMMANIo             IoParmBlock;
    UInt32       retVal;
    
    
    TMIF.Handle = BufferHandle;
    
    IoParmBlock.controlCode = constIOCTLtmmanSGBufferDestroy;
    IoParmBlock.pData = (void *)(&TMIF);
    
    // Call the tmman driver
    if (de_cntrl (
        DEV_TMMAN,  
        (void *)&IoParmBlock, 
        (void *)&retVal))
    {
        TMIF.Status = statusDeviceIoCtlFail;
    }
    
    return TMIF.Status;
}


//
// API Functions Unsupported on the Host
//  
// The following API functions are specified to be only used on the
// Target, as such if they are called from the Host they will always
// return error.
//
TMStatus
tmmanSGBufferOpen ( 
	UInt32	DSPHandle, 
	UInt8*	Name,
	UInt32*	EntryCountPointer,
	UInt32* SizePointer,
	UInt32* BufferHandlePointer )
{
	return statusUnsupportedOnThisPlatform;
}

TMStatus
 tmmanSGBufferClose ( 
	UInt32	BufferHandle )
{
	return statusUnsupportedOnThisPlatform;
}

TMStatus
tmmanSGBufferFirstBlock ( 
	UInt32	BufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer )
{
	return statusUnsupportedOnThisPlatform;
}

TMStatus
tmmanSGBufferNextBlock ( 
	UInt32	BufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer )
{
	return statusUnsupportedOnThisPlatform;
}

TMStatus
tmmanSGBufferCopy ( 
	UInt32	BufferHandle,
	UInt32	Offset,
	UInt32	Address, 
	UInt32	Size, 
	UInt32	Direction )
{
	return statusUnsupportedOnThisPlatform;
}



*/


TMStatus
tmmanSharedMemoryOpen ( 
		       UInt32 DSPHandle,
		       UInt8*	Name,
		       UInt32*	LengthPtr,
		       UInt32*	AddressPointer,
		       UInt32*	SharedMemoryHandlePointer 
		       )
{
	/* unused parameters */
	(void)DSPHandle;
	(void)Name;
	(void)LengthPtr;
	(void)AddressPointer;
	(void)SharedMemoryHandlePointer;
		       
		       
  return statusUnsupportedOnThisPlatform;
}


TMStatus
tmmanSharedMemoryClose (
			UInt32	SharedMemoryHandle )
{
	/* unused parameter */
	(void)SharedMemoryHandle;
	
	
  return statusUnsupportedOnThisPlatform;
}


