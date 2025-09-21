/*========================================================================
   File			:	OSALstorage.cpp 
 
   Purpose		:	Implementation file for the BeOS OSAL class.  This file 
   					contains all the methods relating to the volatile and
   					non-volatile memory storage functionality inside the 
   					OSAL entity.
   					
   					The rest of the OsalBeOS class's methods are split into 
   					two other files :  OSALcallback.cpp and OSAL_beos.cpp
   					The class itself is defined in Osal_BeOS.hpp
         
   NOTE			:   Volatile storage functionality is not implemented.
   
   Author		:	Bryan Pong, 2000-05

   Modifications:	Added checks so that file creation doesent crash.
   					Added extended comments. ^_^
      				Lee Patekar, 2000-11

             		Copyright (C) 1999 by Trisignal communications Inc.
                            	     All Rights Reserved
  =========================================================================*/
#include <unistd.h>  // For lseek, read, write functions
#include <fcntl.h>   // For open function

#ifdef BE_DEBUG
#include <KernelExport.h>
#endif

#include "TsMemoryUtil.hpp"
#include "HostCommon.h"
#include "Osal_BeOS.hpp"

/*========================================================================
			Constant definitions
   ========================================================================*/

/*========================================================================
			Macro definitions
   ========================================================================*/
#define max(a, b) ((a)<(b))?(b):(a);

/*========================================================================
			Structure definitions
   ========================================================================*/

/*========================================================================
				Public variables definitions
   ========================================================================*/

/*========================================================================
				Private variables definitions
   ========================================================================*/
// Set the default non-volatile file size
static UINT32 NonVStorageSize = 512;

/*========================================================================
			Private methods declarations
   ========================================================================*/


/*=========================================================================

  Function	:	OsalBeOS

  Purpose	:   OsalBeOS class constructor.  It initializes the file table
  				variables and sets the filename (and location) for each type
  				of storage.

  Input		:   nothing

  Output	:   Initializes the file name variables and allocates memory for
  				the file tables.

  Returns	:   Nothing

  NOTE		:	There are four types of storage :
  				MODEM_CONFIG stores the temporary modem configurations,
  				FAX_CONFIG stores the temporary fax configurations,
  				EEPROM stores all the long term settings like the contry,
  				BLACK_LISTING stores all the black listing information.
            
  =========================================================================*/
OsalBeOS::OsalBeOS()
{
    int i;

	#ifdef BE_DEBUG
    dprintf("OsalBeOS constructor ...\n");    
	#endif
    
    // Allocate memory for the file tables that will contain information for]
    // each opened file.
    fileDescriptor   = new int[MAX_STORAGEID];
    totalStorageSize = new UINT16[MAX_STORAGEID];
    fileName         = new char*[MAX_STORAGEID]; 

	// Initialize the file tables so it is empty and ready for use.
    for(i = 0; i < MAX_STORAGEID; i++)
    {
        fileDescriptor[i]   = -1;
        totalStorageSize[i] = 0;
    }
    
    // Set a filename for each type of storage
    fileName[MODEM_CONFIG_STORAGEID]  = "/boot/home/config/settings/trimodem/MODEM_CONFIG";
    fileName[FAX_CONFIG_STORAGEID]    = "/boot/home/config/settings/trimodem/FAX_CONFIG";
    fileName[EEPROM_STORAGEID]        = "/boot/home/config/settings/trimodem/EEPROM";
    fileName[BLACK_LISTING_STORAGEID] = "/boot/home/config/settings/trimodem/BLACK_LISTING";
}


/*=========================================================================

  Function	:	OpenNonVStorage

  Purpose	:   Open access to a region of a Non-volatile storage device 
                already allocated for modem use.  After this call, the 
                device is ready to accept Read/Write requests.

  Input		:   StorageID :

				Identifies the region of non-volatile storage that needs to 
				be made available for access by the GMC.
				
				NVSize :
				
				Provides the total size of the data to be accessed in 
				non-volatile storage.

  Output	:   none.

  Returns	:   0 indicates a failure, otherwise success

  NOTE		:	
            
  =========================================================================*/
BOOL OsalBeOS::OpenNonVStorage(UINT16 StorageID, UINT16 NVSize)
{
    NonVStorageSize = NVSize;
  
	#ifdef BE_DEBUG
    dprintf("Attempting to open file ...\n");
    dprintf("OpenNonVStorage : StorageID : %d\n", StorageID);
    dprintf("NonVStorageSize = %d\n", NVSize);
	#endif
    
    // Only open a file if it hasn't been opened already 
    if(fileDescriptor[StorageID] == -1)
    {   
		#ifdef BE_DEBUG
        dprintf("Before crashing ...\n");
        dprintf("File name : %s\n", fileName[StorageID]);
		#endif
        
        // Try to open the file 
        if((fileDescriptor[StorageID] = open(fileName[StorageID], O_RDWR)) < 0)
        {
            // The file does not exist, so we will create it
			#ifdef BE_DEBUG
            dprintf("The file %s doesn't exist. We create it now.\n", fileName[StorageID]);
			#endif

			// We must make sure the file's directory exist's, if it does 
			// not exist we must create the directory before creating the 
			// file.  If the file isn't created the GMC will exit and the
			// syste may crash.
			mkdir("/boot", 0777);
			mkdir("/boot/home", 0777);
			mkdir("/boot/home/config", 0777);
			mkdir("/boot/home/config/settings", 0777);
			mkdir("/boot/home/config/settings/trimodem", 0777);

			// Now we create the file		
            if((fileDescriptor[StorageID] = open(fileName[StorageID], O_RDWR|O_CREAT)) < 0)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


/*=========================================================================

  Function	:	ReadNonVStorage

  Purpose	:   Read data from a specific region of NV data.  Requests to 
  				read OutBufSize bytes from NV data, starting at usOffset 
  				bytes from the start of NV data, the data to be copied at 
  				the address specified by pOutBuf.  The content of the 
  				address pointed by pBytesRead will be filled with the number
  				of bytes actually transferred to pOutBuf 
                (*pBytesRead <= OutBufSize).

  Input		:   StorageID :
  
  				Indicates which storage file we want to read from.
  				
  				usOffset :
  				
  				Indicates the offset in the file from which we will read.
  				
  				OutBufSize :
  				
  				Indicates the size of the data buffer.
  				
  				OutBufPtr :
  				
  				Points to the buffer where we'll write the data.
  				
  				BytesReadPtr :
  				
  				Points to a variable where we'll write how many bytes were
  				read from the storage device (the file).

  Output	:	The value pointed to by BytesReadPtr will contain the amount
  				of bytes actually read from the device.

  Returns	:   nothing.

  =========================================================================*/
void OsalBeOS::ReadNonVStorage(UINT16  StorageID, 
                               UINT16  usOffset, 
                               UINT16  OutBufSize, 
                               BYTE*   OutBufPtr, 
                               UINT16* BytesReadPtr)
{
    INT16 BytesRead;

    // tell the calling function that no bytes have been read yet.
    *BytesReadPtr = 0;

	#ifdef BE_DEBUG
    dprintf("ReadNonVStorage test.\n");
    dprintf("usOffset : %d\n", usOffset);
    dprintf("Trying to read: %d bytes\n", OutBufSize);
	#endif
    
    // Check to see if the requested device is valid.  If it's valid we can
    // continue with the request, otherwise, we should just exit.
    if(fileDescriptor[StorageID] != -1)
    {
        // Lets make sure the file contains the amount of data we want to
        // read.  If it does not we'll simply exit without reading anything.
        if((unsigned(usOffset + OutBufSize)) <= NonVStorageSize)
        {
			// Now we'll seek to the part of the file we should read from.
			// If the seek fails, we'll exit without reading anything.
            if(lseek(fileDescriptor[StorageID], usOffset, SEEK_SET) < 0)
            {
				#ifdef BE_DEBUG
                dprintf("Seek error\n");
				#endif
                
                return;
            }

			// Now we should be able to read our data.  If the read fails,
			// simply exit the function.  (returning a read value of 0)
            if((BytesRead = read(fileDescriptor[StorageID], OutBufPtr, OutBufSize)) < 0)
            {
				#ifdef BE_DEBUG
                dprintf("Read error\n");
				#endif
				
                return;
            }

			// Tell the calling function how many bytes of data we read.
            *BytesReadPtr = (UINT16)BytesRead;
        }
    }

	// If the storage ID is invalid simply quit.
    else
    {
		#ifdef BE_DEBUG
        dprintf("Attempting to read with an invalid file descriptor\n");
		#endif
        return;
    }
	
	#ifdef BE_DEBUG
    dprintf("ReadNonVStorage. BytesRead : %d\n", *BytesReadPtr);
	#endif
}


/*=========================================================================

  Function	:	WriteNonVStorage

  Purpose	:   Write data to a specific region of NV data.
                Requests to write InBufSize bytes to NV data, starting at 
                usOffset bytes from the start of NV data, the data to be 
                copied being provided at the address specified by pInBuf.  
                The content of the address pointed by pBytesWritten will 
                be filled with the number of bytes actually transferred 
                from pOutBuf (*pBytesWritten <= InBufSize).

  Input		:   StorageID :
  
  				Indicates which storage file we want to write to.
  				
  				usOffset :
  				
  				Indicates the offset in the file from which we will write.
  				
  				InBufSize :
  				
  				Indicates the size of the data to be written to the file.
  				
  				InBufPtr :
  				
  				Points to the buffer containing the data to be written to 
  				the file.
  				
  				BytesWrittenPtr :
  				
  				Points to a variable where we'll write how many bytes were
  				written to the storage device (the file).

  Output	:   The value pointed to by BytesReadPtr will contain the amount
  				of bytes actually written to the device.

  Returns	:   nothing.

  =========================================================================*/
void OsalBeOS::WriteNonVStorage(UINT16  StorageID, 
                                UINT16  usOffset, 
                                UINT16  InBufSize, 
                                BYTE*   InBufPtr, 
                                UINT16* BytesWrittenPtr)
{
    int tmp = 0, BytesWrite;

    // tell the calling function that no bytes have been read yet.
    *BytesWrittenPtr = 0;

	// Lets make sure the file will still have a valid size after we write
	// our data to it.
    if((unsigned(usOffset + InBufSize)) <= NonVStorageSize)
    {
		#ifdef BE_DEBUG
        dprintf("Attempting writing to file %s ...\n", fileName[StorageID]);
        dprintf("Curent total storage area size :%d\n", totalStorageSize[StorageID]);
        dprintf("Offset the file is to be written to : %d\n", usOffset); 
		#endif

		// Now we'll seek to the part of the file we should write to.  If 
		// the seek fails, we'll exit without writing anything.
        if(lseek(fileDescriptor[StorageID], usOffset, SEEK_SET) < 0)
        {
			#ifdef BE_DEBUG
            dprintf("Seek error\n");
			#endif
			
            return;
        }

		// Now we should be able to write our data.  If the write fails,
		// simply exit the function.  (returning a write value of 0)
        if((BytesWrite = write(fileDescriptor[StorageID], InBufPtr, InBufSize)) < 0)
        {
			#ifdef BE_DEBUG
            dprintf("Write error\n");
			#endif
			
            return;
        }

		// Tell the calling function how many bytes of data we wrote.        
        *BytesWrittenPtr = (UINT16)BytesWrite;


		#ifdef BE_DEBUG
        dprintf("BytesWritten :%d\n", *BytesWrittenPtr);
		#endif
		
		// Update the file size in the file tables.  the value in the tables
		// should never be bigger then the maximum allowable file size.
        tmp = totalStorageSize[StorageID];
        totalStorageSize[StorageID] = max(usOffset + InBufSize, tmp);
    }
}


/*=========================================================================

  Function	:	DeleteNonVStorage

  Purpose	:   Deletes the non volatile storage device from the system.  In
  				short, it deletes the file.

  Input		:   StorageID :
  
  				The ID of the storage device to delete.

  Output	:   none

  Returns	:   nothing
            
  =========================================================================*/
void OsalBeOS::DeleteNonVStorage(UINT16 StorageID)
{
    if (fileDescriptor[StorageID] != -1)
    {
        // Removes the file, may need to find the other function though.  We
        // also invalidate the file in the table so it can be opened again.
        rmdir(fileName[StorageID]);
        fileDescriptor[StorageID] = -1;
    }
}


/*=========================================================================

  Function	:	CloseNonVStorage

  Purpose	:   Access to the region in a Non-volatile storage device 
                defined by StorageID will stop until the next call 
                to OpenNonVStorage.  In short, it closes the file (does not
                delete it).

  Input		:   StorageID :
  
  				The ID of the storage device to close.

  Output	:   nothing

  Returns	:   none

  =========================================================================*/
void OsalBeOS::CloseNonVStorage(UINT16 StorageID)
{
    if (fileDescriptor[StorageID] != -1)
    {
        close(fileDescriptor[StorageID]);
        fileDescriptor[StorageID] = -1;
    }
}


/*========================================================================
			Unimplemented methods declarations
   ========================================================================*/

// The following methods are not used in this implementation.  They are here
// to satisfy the linker (they are also here for compatibility and 
// consistency with the rest of the Trisignal code base).

BOOL OsalBeOS::OpenVStorage(UINT16 /*StorageID*/, UINT16 /*VSize*/)
{
	/* Not implemented or used */
	return false;
}

void OsalBeOS::ReadVStorage(UINT16 /* StorageID */,
                                    UINT16  /* usOffset     */, 
                                    UINT16  /* OutBufSize   */, 
                                    BYTE*   /* OutBufPtr    */, 
                                    UINT16* /* BytesReadPtr */)
{
	/* Not implemented or used */
}

void OsalBeOS::WriteVStorage( UINT16 /* StorageID */,
                                      UINT16  /* usOffset        */, 
                                      UINT16  /* InBufSize       */, 
                                      BYTE*   /* InBufPtr        */, 
                                      UINT16* /* BytesWrittenPtr */)
{
	/* Not implemented or used */
}

void OsalBeOS::DeleteVStorage(UINT16 /*StorageID*/)
{
	/* Not implemented or used */
}

void OsalBeOS::CloseVStorage(UINT16 /*StorageID*/)
{
	/* Not implemented or used */
}
