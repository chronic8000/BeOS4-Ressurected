/*
 * $Log:   V:/Flite/archives/FLite/examples/flcopy/Flcopy.c_V  $
 * 
 *    Rev 1.1   Jun 16 1999 18:41:44   marinak
 * Work with multiple drives
 * Fix infinite loop bug
 * 
 *    Rev 1.0   23 May 1999 14:19:00   marina
 * Initial revision.
 */

#include "fsapi.h"
#include "blockdev.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

typedef unsigned long FHANDLE;

typedef struct {
  unsigned driveNo;
  FHANDLE fp;
  FLStatus(*openFile)(FHANDLE* handle, char* path, unsigned flags);
  FLStatus(*closeFile)(FHANDLE* handle);
  FLStatus(*getFileLen)(FHANDLE* handle, unsigned long* len);
  FLStatus(*writeFile)(FHANDLE* handle, unsigned char* buff, unsigned len);
  FLStatus(*readFile)(FHANDLE* handle, unsigned char* buff, unsigned len);
}FileOperations;


#define MAX_STRING_LEN	280
char OutString[MAX_STRING_LEN];


#define errorMessage(status,fliteFunction) \
  if (status != flOK) { \
    PrintMessage("Error: %s returned status: %d\n", fliteFunction, status); \
    return status; \
  }


/*-------------------------------------------------------------------------------------------*/
/*                      P r i n t M e s s a g e                                              */
/*                                                                                           */
/*  Outputs message                                                                          */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*    formatString: message string                                                           */
/*                                                                                           */    
/*-------------------------------------------------------------------------------------------*/

void PrintMessage(const char *formatString, ...)
{
  va_list argptr;
  va_start(argptr, formatString);
  vsprintf(OutString, formatString, argptr);
  printf(OutString);
  va_end(argptr);
}


void USAGE(void)
{
  PrintMessage("Usage:\n  FLCOPY [\h] source_root destination_root\n");
  PrintMessage("source_root,destination_root are each a valid DOS directory path,\n");
  PrintMessage("    or a FLite path specified as  FLITE\\\\<path>.\n");
  PrintMessage("    Default FLite drive No is 0. If, for example, you want to work with\n");
  PrintMessage("    FLite drive No 1 then type FLITE1\\\\<path>.\n");
}

/*-------------------------------------------------------------------------------------------*/
/*                      D O S o p e n F i l e                                                */
/*                                                                                           */
/*  Opens file on the hard disk                                                              */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     path:      Full path of the file to open(create)                                      */
/*     flags:     OPEN_FOR_READ or OPEN_FOR_WRITE                                            */
/*  Returns:                                                                                 */
/*     handle:    File pointer of the opened file                                            */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus DOSopenFile(FHANDLE* handle, char* path,unsigned flags)
{
  switch(flags) {
    case OPEN_FOR_READ:
      *handle = (FHANDLE)fopen(path,"rb");  /* Open  file on the hard disk volume in read
                                          only mode. Binary mode is needed for consistent
                                          data flow between flash and hard disk*/
      if(*handle==(unsigned long)NULL) {
        PrintMessage("ERROR: File %s does no exists\n", path);
        return flFileNotFound;
      }
      break;

    case OPEN_FOR_WRITE:
      *handle = (FHANDLE)fopen(path,"wb");
      if(*handle==(unsigned long)NULL) {
        PrintMessage("ERROR open file %s\n", path);
        return flGeneralFailure;
      }
      break;

    default:
      return flBadParameter;
  }
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                      D O S c l o s e F i l e                                              */
/*                                                                                           */
/*  Closes file on the hard disk                                                             */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to close                                                   */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus DOScloseFile(FHANDLE* handle)
{
  if((*handle)==(unsigned long)NULL)
    return flGeneralFailure;
  if(fclose((FILE*)(*handle))!=0)
    return flGeneralFailure;
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                      D O S w r i t e F i l e                                              */
/*                                                                                           */
/*  Writes file on the hard disk                                                             */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to write                                                   */
/*     buff:      Data to write                                                              */
/*     len:       Number of bytes to write                                                   */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus DOSwriteFile(FHANDLE* handle, unsigned char* buff, unsigned len)
{
  if((*handle)==(unsigned long)NULL)
    return flGeneralFailure;

  if(fwrite(buff,1,len,((FILE*)(*handle)))!=len)
    return flGeneralFailure;
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                      D O S r e a d F i l e                                                */
/*                                                                                           */
/*  Reads file from the hard disk                                                            */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to read                                                    */
/*     buff:      Destination buffer                                                         */
/*     len:       Number of bytes to read                                                    */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus DOSreadFile(FHANDLE* handle, unsigned char* buff, unsigned len)
{
  if((*handle)==(unsigned long)NULL)
    return flGeneralFailure;

  if(fread(buff,1,len,((FILE*)(*handle)))!=len)
    return flGeneralFailure;
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                      D O S g e t F i l e L e n                                            */
/*                                                                                           */
/*  Returns length of the file                                                               */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file                                                            */
/*  Returns:                                                                                 */
/*     len:       Length of the file in byte units                                           */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus DOSgetFileLen(FHANDLE* handle, unsigned long* len)
{
  if((*handle)==(unsigned long)NULL)
    return flGeneralFailure;

  if(fseek(((FILE*)(*handle)),0L,SEEK_END)!=0)
    return flGeneralFailure;
  *len = ftell((FILE*)(*handle));
  if(*len == -1)
    return flGeneralFailure;
  if(fseek(((FILE*)(*handle)),0L,SEEK_SET)!=0)
    return flGeneralFailure;
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  F L i t e O p e n F i l e                                                */
/*                                                                                           */
/*  Opens file on the flash device                                                           */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     path:      Full path of the file to open(create)                                      */
/*     flags:     OPEN_FOR_READ or OPEN_FOR_WRITE                                            */
/*                                                                                           */
/*  Returns:                                                                                 */
/*     handle:    File pointer of the opened file                                            */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus FLiteOpenFile(FHANDLE* handle, char* path,unsigned flags)
{
  FLSimplePath flPath[50];
  FLStatus status;
  IOreq ioreq;
  
  if(isdigit(path[5])) {
    ioreq.irHandle = (int)(path[5]-'0');
    ioreq.irData = path+8;
  }
  else {
    ioreq.irHandle = 0;
    ioreq.irData = path+7;
  }
  /* Parse the program argument to DOS path */
  ioreq.irPath = flPath;
  status = flParsePath(&ioreq);
  errorMessage(status, "flParsePath");

  ioreq.irFlags = flags;
  status = flOpenFile(&ioreq);
  errorMessage(status, "flOpenFile");
  *handle = ioreq.irHandle;
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  F L i t e C l o s e F i l e                                              */
/*                                                                                           */
/*  Closes file on the flash device                                                          */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to close                                                   */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus FLiteCloseFile(FHANDLE* handle)
{
  FLStatus status;
  IOreq ioreq;

  ioreq.irHandle = (FLHandle)(*handle);
  status = flCloseFile(&ioreq);
  errorMessage(status, "flCloseFile");
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  F L i t e W r i t e F i l e                                              */
/*                                                                                           */
/*  Writes file on the flash device                                                          */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to write                                                   */
/*     buff:      Data to write                                                              */
/*     len:       Number of bytes to write                                                   */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/


FLStatus FLiteWriteFile(FHANDLE* handle, unsigned char* buff, unsigned len)
{
  FLStatus status;
  IOreq ioreq;

  ioreq.irHandle = (FLHandle)(*handle);
  ioreq.irLength = len;         
  ioreq.irData = buff;
  status = flWriteFile(&ioreq);
  errorMessage(status, "flWriteFile");
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  F L i t e R e a d F i l e                                                */
/*                                                                                           */
/*  Reads file from the flash device                                                         */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file to read                                                    */
/*     buff:      Destination buffer                                                         */
/*     len:       Number of bytes to read                                                    */
/*  Returns:                                                                                 */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus FLiteReadFile(FHANDLE* handle, unsigned char* buff, unsigned len)
{
  FLStatus status;
  IOreq ioreq;

  ioreq.irHandle = (FLHandle)(*handle);
  ioreq.irLength = len;         
  ioreq.irData = buff;
  status = flReadFile(&ioreq);
  errorMessage(status, "flReadFile");
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  F L i t e G e t F i l e L e n                                            */
/*                                                                                           */
/*  Returns length of the file                                                               */
/*                                                                                           */ 
/*  Parameters:                                                                              */
/*     handle:    Pointer on file                                                            */
/*  Returns:                                                                                 */
/*     len:       Length of the file in byte units                                           */
/*     FLStatus	: 0 on success, otherwise failed                                             */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus FLiteGetFileLen(FHANDLE* handle, unsigned long* len)
{
  FLStatus status;
  IOreq ioreq;

  ioreq.irHandle = (unsigned)(*handle);
  ioreq.irLength = 0;
  ioreq.irFlags = SEEK_END;    
  status = flSeekFile(&ioreq);
  errorMessage(status,"flSeekFile");
  *len = ioreq.irLength;
  ioreq.irLength = 0;
  ioreq.irFlags = SEEK_START;
  status = flSeekFile(&ioreq);
  errorMessage(status,"flSeekFile");
  return flOK;
}


/*-------------------------------------------------------------------------------------------*/
/*                  I n i t D O S F i l e O p e r a t i o n s                                */
/*                                                                                           */
/*  Initializes FileOperations structure for working with hard disk                          */
/*                                                                                           */ 
/*  Returns:                                                                                 */
/*     fOp:       Initialized structure                                                      */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

void InitDOSFileOperations(FileOperations* fOp)
{
  fOp->openFile = DOSopenFile;
  fOp->closeFile = DOScloseFile;
  fOp->writeFile = DOSwriteFile;
  fOp->readFile = DOSreadFile;
  fOp->getFileLen = DOSgetFileLen;
  fOp->driveNo = DRIVES+1;
}


/*-------------------------------------------------------------------------------------------*/
/*                  I n i t F L i t e F i l e O p e r a t i o n s                            */
/*                                                                                           */
/*  Initializes FileOperations structure for working with flash device                       */
/*                                                                                           */ 
/*  Returns:                                                                                 */
/*     fOp:       Initialized structure                                                      */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

void InitFLiteFileOperations(FileOperations* fOp)
{
  fOp->openFile = FLiteOpenFile;
  fOp->closeFile = FLiteCloseFile;
  fOp->writeFile = FLiteWriteFile;
  fOp->readFile = FLiteReadFile;
  fOp->getFileLen = FLiteGetFileLen;
}


/*-------------------------------------------------------------------------------------------*/
/*                                                                                           */
/*                      i s F l i t e P a t h                                                */
/*                                                                                           */
/*  Checks program argument : if it is valid flash path                                      */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLBoolean isFlitePath(char *path)
{
  if(strlen(path)<7)
    return FALSE;
  return (!tffscmp("flite", path, 5));
}


/*-------------------------------------------------------------------------------------------*/
/*                                                                                           */
/*                         M A I N                                                           */
/*                                                                                           */
/*-------------------------------------------------------------------------------------------*/

FLStatus __cdecl main(int argc,char *argv[])
{
  IOreq ioreq;
  FileOperations fromFile,toFile;
  FLStatus status;
  unsigned char buff[512];
  unsigned long len=0;
  unsigned readNow;

  /*Check validity of program arguments*/
  if ((argc != 3) ||
      ((argv[1][0]=='//') && (argv[1][1]=='h'))) {
    USAGE();
    return flBadParameter;
  }

  /*Open files on the flash and the hard disk respectively to program parameters*/
  if(isFlitePath(argv[1])) {
    /* Is source a flash*/
    if(isdigit(argv[1][5]))
      fromFile.driveNo = (int)(argv[1][5]-'0');
    else
      fromFile.driveNo = 0;

    /*Mount the volume*/
    ioreq.irHandle = fromFile.driveNo;
    status = flMountVolume(&ioreq);
    errorMessage(status, "flMountVolume");

    InitFLiteFileOperations(&fromFile);
  }
  else
    InitDOSFileOperations(&fromFile);

  if(isFlitePath(argv[2])) {
    /* Is destination a flash*/
    if(isdigit(argv[2][5]))
      toFile.driveNo = (int)(argv[2][5]-'0');
    else
      toFile.driveNo = 0;

    if(toFile.driveNo != fromFile.driveNo) {
      /* If destination volume is not mounted yet then...*/
      /*Mount the volume*/
      ioreq.irHandle = toFile.driveNo;
      status = flMountVolume(&ioreq);
      errorMessage(status, "flMountVolume");
    }

    InitFLiteFileOperations(&toFile);
  }
  else
    InitDOSFileOperations(&toFile);

  checkStatus(fromFile.openFile(&(fromFile.fp),argv[1],OPEN_FOR_READ));
  checkStatus(toFile.openFile(&(toFile.fp),argv[2],OPEN_FOR_WRITE));
  
  checkStatus(fromFile.getFileLen(&(fromFile.fp),&len));
  readNow = 512;
  status = flOK;
  while((len>0) && (status==flOK)) {
    if(readNow>len) 
      readNow = (unsigned)len;
    len-=readNow;

    checkStatus(fromFile.readFile(&(fromFile.fp),buff,readNow));
    checkStatus(toFile.writeFile(&(toFile.fp),buff,readNow));
  }

  checkStatus(fromFile.closeFile(&(fromFile.fp)));
  checkStatus(toFile.closeFile(&(toFile.fp)));
  return flOK;
}
