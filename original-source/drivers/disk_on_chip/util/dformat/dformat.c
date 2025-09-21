#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dformat.h"
//-------------------------------------------------------------------
#define VERSION_NAME TEXT("Version 1.0(ES)")
#define COPYRIGHT    TEXT("Copyright (C) M-Systems, 1992-1999")
//-------------------------------------------------------------------
extern unsigned long window;
Anand *getAnandRec(unsigned drive);
NFDC21Vars *getNFDC21Vars(unsigned drive);
#ifndef SIGN_CHECK
unsigned char defaultSignature[] = { 'B','I','P','O' };
#endif // ! SIGN_CHECK
//-------------------------------------------------------------------
#if (OS_APP == OS_WINCE)
DOC_Parameters curDocParams;
//---------------------------------------------------------
void getDocParameters(void FAR2 *docParams)
{ // Return DOC_Parameters to caller.
  DOC_Parameters *docParamPtr = (DOC_Parameters FAR2 *)docParams;
  docParamPtr->docAccess  = curDocParams.docAccess;
  docParamPtr->docShift   = curDocParams.docShift;
  docParamPtr->docAddress = curDocParams.docAddress;
}
//---------------------------------------------------------
int getArgFromStr(wchar_t *aa[], wchar_t *str)
{ // Put into 'aa' pointers to arguments from 'str' string.
  char sfl;
  int  i,j,len,sw;

  for(j=i=0,len=STRLEN(str),sfl=FALSE;( i < len );i++) {
    if( !isspace(str[i]) ) {            // word
      if( !sfl ) {                      // start of the word
	sfl = TRUE;
	sw = i;
      }
    }
    else {                              // space
      if( sfl ) {                       // end of word
	str[i] = TEXT('\0');
	aa[j++] = &(str[sw]);
	sfl = FALSE;
      }
    }
  }
  if( sfl ) aa[j++] = &(str[sw]);
  aa[j] = NULL;
  return(j);
}
#endif // OS_WINCE
/*-------------------------------------------------------------------*/
unsigned short getNFTLUnitSizeBits(PhysicalInfo FAR2 *phinfo)
{
  long size;
  unsigned short nftlUnitSizeBits, mediaNoOfUnits;

  for(nftlUnitSizeBits=0,size=1L;( size < phinfo->unitSize );
      nftlUnitSizeBits++, size <<= 1);

  mediaNoOfUnits = (unsigned short)(phinfo->mediaSize >> nftlUnitSizeBits);

  /* Adjust unit size so header unit fits in one unit */
  while(mediaNoOfUnits * sizeof(ANANDPhysUnit) + SECTOR_SIZE > (1UL << nftlUnitSizeBits)) {
    nftlUnitSizeBits++;
    mediaNoOfUnits >>= 1;
  }
  return(nftlUnitSizeBits);
}
/*-------------------------------------------------------------------*/
void initRandomNum(void)
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  srand((unsigned int)clock());            // Init random sequence
#endif // OS_DOS || OS_QNX
}
//-------------------------------------------------------------------
int getRandomNum(void)
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( rand() );
#endif // OS_DOS || OS_QNX
#if (OS_APP == OS_WINCE)
  return( (int)Random() );
#endif // OS_WINCE
}
//-------------------------------------------------------------------
FILE_HANDLE fileOpen( wchar_t *fileName, unsigned fileAttr )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( open(fileName, fileAttr) );
#endif // OS_DOS
#if (OS_APP == OS_WINCE)
  return( CreateFile(fileName, fileAttr, 0, NULL, OPEN_EXISTING,
		     FILE_ATTRIBUTE_NORMAL, NULL) );
#endif // OS_WINCE
}
//-----------------------------------------------
int fileClose( FILE_HANDLE fileHandle )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( close(fileHandle) );
#endif // OS_DOS
#if (OS_APP == OS_WINCE)
  return( (int)CloseHandle(fileHandle) );
#endif // OS_WINCE
}
//-----------------------------------------------
int fileRead( FILE_HANDLE fileHandle, void *buf, unsigned blockSize )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( read(fileHandle, buf, blockSize) );
#endif // OS_DOS
#if (OS_APP == OS_WINCE)
  DWORD readNow;
  ReadFile(fileHandle,(LPVOID)buf,(DWORD)blockSize,(LPDWORD)&readNow,NULL);
  return((int)readNow);
#endif // OS_WINCE
}
//-----------------------------------------------
long fileLseek( FILE_HANDLE fileHandle, long offset, int moveMethod )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( lseek(fileHandle, offset, moveMethod) );
#endif // OS_DOS
#if (OS_APP == OS_WINCE)
  DWORD curPtr;
  curPtr = SetFilePointer(fileHandle,offset,NULL,(DWORD)moveMethod);
  return((long)curPtr);
#endif // OS_WINCE
}
//-----------------------------------------------
long fileLength( FILE_HANDLE fileHandle )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( filelength(fileHandle) );
#endif // OS_DOS
#if (OS_APP == OS_WINCE)
  DWORD fileSize;
  return( GetFileSize( fileHandle, (LPDWORD)&fileSize) );
#endif // OS_WINCE
}
//-------------------------------------------------------------------
#ifdef WRITE_EXB_IMAGE
/*-------------------------------------------------------------------
// Purpose:  Write boot image (collection of BIOS extention-style code
//           modules) to boot area taking care of bad blocks. Write SPL
//           in way compatible with IPL.
// NOTE   :  This method is for DOC 2000 and Millennium only.
//-------------------------------------------------------------------*/
FLStatus writeExbDriverImage( FILE_HANDLE fileExb, unsigned short FAR2 *realBootBlocks,
	      unsigned char GlobalExbFlags, unsigned short flags, int signOffset,
              Anand FAR2 *anandPtr, ANANDPhysUnit FAR2 *physicalUnits )
{
  unsigned short unitSizeBits      = anandPtr->unitSizeBits;
  unsigned short erasableBlockBits = anandPtr->erasableBlockSizeBits;
  unsigned short blockInUnitBits   = unitSizeBits - erasableBlockBits;
  // blocks in boot partition including bad blocks
  unsigned short bootBlocks  = anandPtr->bootUnits << blockInUnitBits;
  long blockSize   = 1L << erasableBlockBits;
  unsigned char splFactor = (flags & BIG_PAGE) && !(flags & MDOC_ASIC) ? SPL_FACTOR : SPL_FACTOR - 1;
  long splHeaderAddr = (flags & MDOC_ASIC) ? IPL_SIZE : 0L;
  long bootFileSize;
  // mCnt - number of code modules in bootimage file
  unsigned short i, mCnt, splSizeInFlash, datapiece, byteCnt, heapSize;
  WorkPlanEntry  work[MAX_CODE_MODULES];
  unsigned char buf[BLOCK];
  CardAddress addr, startAddr;
  BIOSHeader  hdr;
  IOreq ioreq;

  // make sure that SPL resides in "good" region of boot area
  splSizeInFlash = SPL_SIZE * splFactor;

  for(addr=0;( addr < splSizeInFlash ); addr += blockSize)
    if( IS_BAD(physicalUnits[(unsigned short)(addr >> unitSizeBits)]) )
      return(flGeneralFailure);         // bad blocks in SPL area

  // set SPL entry in workplan
  if( flags & MDOC_ASIC ) {
    work[0].fileOff   = 0L;
    work[0].len       = SPL_SIZE;
  }
  else {
    work[0].fileOff   = IPL_SIZE;
    work[0].len       = SPL_SIZE - IPL_SIZE;
  }
  work[0].flashAddr = (CardAddress)0;

  // find out sizes of code modules in bootimage file
  bootFileSize = fileLength(fileExb);

  for(mCnt=1;( (work[mCnt-1].fileOff+work[mCnt-1].len) < bootFileSize );mCnt++) {
    work[mCnt].fileOff = work[mCnt-1].fileOff + work[mCnt-1].len;
    fileLseek(fileExb, work[mCnt].fileOff, FILE_BEGIN);
    fileRead(fileExb, &hdr, sizeof(hdr));
    work[mCnt].len = (unsigned short)hdr.lenMod512 * BLOCK;
    if( ((hdr.signature[0] != 0x55) || (hdr.signature[1] != 0xAA)) ||
	(work[mCnt].fileOff + work[mCnt].len > bootFileSize) )
      return(flBadLength);               // bad code module header
  }
  // last code module is just 4K pad so ignore it
  mCnt -= 1;

  // try to accomodate all code modules in "good" regions of boot area
  startAddr = splSizeInFlash;

  for(i=1;( i < mCnt );i++) {
    for(addr=startAddr;( addr < startAddr + work[i].len );addr += BLOCK) {
      // find nearest "good" region
      if( IS_BAD(physicalUnits[(unsigned short)(addr >> unitSizeBits)]) ) { // bad unit encountered
	startAddr = addr + BLOCK;                // start search over
      }
      if( addr >= ((CardAddress)bootBlocks << erasableBlockBits) )
        return(flVolumeTooSmall);                // out of boot area
    }
    work[i].flashAddr = startAddr;
    startAddr        += work[i].len;
  } // all the code modules have been accomodated
  // set real number of blocks for image
  *realBootBlocks = 1 + ((((SPL_FACTOR - splFactor) * SPL_SIZE) + startAddr - 1) >> erasableBlockBits);
//  printf("RealBootBlocks = %d\n",*realBootBlocks);
  // erase entire boot area
  for(i=0;( i < (*realBootBlocks) ); i++)
    if( !IS_BAD(physicalUnits[i >> blockInUnitBits]) ) {  // skip bad units
      ioreq.irHandle = 0;
      ioreq.irUnitNo = i;
      ioreq.irUnitCount = 1;
      checkStatus( flPhysicalErase(&ioreq) );
    }

  // write SPL to flash in a fashion compatible with IPL
  tffsset (buf, 0xff, sizeof(buf) );

  for(i=0;( i < mCnt ); i++) {
    fileLseek(fileExb, work[i].fileOff, FILE_BEGIN);

    addr = work[i].flashAddr;
    datapiece = (i == 0 ? BLOCK/splFactor : BLOCK);
    for(byteCnt=0;( byteCnt < work[i].len ); byteCnt += datapiece, addr += BLOCK) {
      fileRead(fileExb, buf, datapiece);

      if( addr == splHeaderAddr ) {                   // SPL header
	// Change splHeader.biosHdr.lenMod512 to cover the whole boot area
	SplHeader FAR2 *p = (SplHeader FAR2 *)buf;

	unsigned char tmp     = p->biosHdr.lenMod512;
	p->biosHdr.lenMod512  = ((unsigned long)(*realBootBlocks) << erasableBlockBits) / BLOCK;
	p->chksumFix         -= (p->biosHdr.lenMod512 - tmp);

	// generate random run-time ID and write it into splHeader.
	initRandomNum();                              // Init random sequence
        p->runtimeID[0] = (unsigned char)getRandomNum();
        p->runtimeID[1] = (unsigned char)getRandomNum();
        p->chksumFix -= (unsigned char)(p->runtimeID[0]);
        p->chksumFix -= (unsigned char)(p->runtimeID[1]);

	// calculate TFFS heap size and write it into splHeader.
        heapSize = 3 * anandPtr->noOfUnits + 2 * KBYTE;
        toUNAL2(p->tffsHeapSize, heapSize);
        p->chksumFix -= (unsigned char)(heapSize);
        p->chksumFix -= (unsigned char)(heapSize >> 8);
      }
      else
	if( (i == 1) && (byteCnt == 0) ) {
	  // put QUIET into SS header
	  SSHeader FAR2 *ssp = (SSHeader FAR2 *)buf;

	  ssp->exbFlags  |= GlobalExbFlags;
	  ssp->chksumFix -= GlobalExbFlags;
	}
	else
	  if( (i == 2) && (byteCnt == 0) ) {
	    // put "install as first drive" & QUIET mark into TFFS header
	    TffsHeader FAR2 *p = (TffsHeader FAR2 *)buf;

	    p->exbFlags  |= GlobalExbFlags;
	    p->chksumFix -= GlobalExbFlags;
	  }
      ioreq.irHandle = 0;
      if( signOffset == SIGN_OFFSET ) // if singOffset == 8, use EDC
	ioreq.irFlags = EDC;
      else ioreq.irFlags = 0;
      ioreq.irAddress = addr;
      ioreq.irByteCount = BLOCK;
      ioreq.irData = (void FAR1 *)buf;
      checkStatus( flPhysicalWrite(&ioreq) );
    }
  }
  return(flOK);
}
#endif // WRITE_EXB_IMAGE
/*-----------------------------------------------------------------------
// Purpose:  Write boot loader image (collection of binary data) to boot
//           area taking care of bad blocks. Marking is made in next way:
//           ZZZZXXXX, where ZZZZ - 4 letters (default is BIPO), and
//           XXXX - is serial unit number (from 0000 to FFFF). Last unit
//           that contains data is FFFF, and if boot loader area contains
//           more units, all other units will have FFFF number in signature.
// NOTE   :  This method is for DOC 2000 only.
// Parameters:
//           file        - open file descriptor of the image
//           startBlock     - start to write from this block of partition
//           areaLen        - length of the writing chunk
//           sigh           - pointer to signature
//           anandPtr       - pointer to Anand algorithm structure
//           physicalUnits  - pointer to bad block table array
// Returns:
//           flNoSpaceInVolume - not enough space to write image
//           flWriteFault      - error writing to media
//           flOK              - success
//-----------------------------------------------------------------------*/
FLStatus writeBootAreaFile( FILE_HANDLE file, unsigned short startBlock,
              unsigned long areaLen, unsigned char FAR2 *sign, int signOffset,
	      Anand FAR2 *anandPtr, ANANDPhysUnit FAR2 *physicalUnits )
{
  unsigned short unitSizeBits      = anandPtr->unitSizeBits;
  unsigned short erasableBlockBits = anandPtr->erasableBlockSizeBits;
  unsigned short blockInUnitBits   = unitSizeBits - erasableBlockBits;
  unsigned short blockInUnit       = 1 << blockInUnitBits;
  // amount of blocks to write
  long writeBlocks = 1 + ((areaLen - 1) >> erasableBlockBits);
  // blocks in boot partition including bad blocks
  long bootBlocks  = anandPtr->bootUnits << blockInUnitBits;
  long blockSize   = 1L << erasableBlockBits;
  char stopFlag    = FALSE;
  CardAddress addr, i0, j;
  IOreq ioreq;
  int  readFlag;
  unsigned short iBlock, freeBlocks, wBlock;
  unsigned char buf[BLOCK], signature[SIGNATURE_LEN+1];

  // check for space in boot area partition
  freeBlocks = 0;
  for(iBlock=startBlock;( iBlock < bootBlocks ); iBlock += blockInUnit)
    if( !IS_BAD(physicalUnits[iBlock >> blockInUnitBits]) ) // skip bad units
      freeBlocks += blockInUnit;

  if( freeBlocks < writeBlocks )
    return(flNoSpaceInVolume);

  if( FILE_OPEN_ERROR(file) )                         // mark only
    stopFlag = TRUE;

  tffscpy(signature, sign, SIGNATURE_NAME);

  for(wBlock=0,iBlock=startBlock;( wBlock < writeBlocks );iBlock++) {
//    printf("Block = %d\n",iBlock);
    if( !IS_BAD(physicalUnits[iBlock >> blockInUnitBits]) ) { // skip bad units

      ioreq.irHandle = 0;                   // erase block
      ioreq.irUnitNo = iBlock;
      ioreq.irUnitCount = 1;
      checkStatus( flPhysicalErase(&ioreq) );

      if( !stopFlag )                  // write block from file
	for(i0=0;( i0 < blockSize );i0+=BLOCK) {
          readFlag = fileRead(file, buf, BLOCK);
	  if( readFlag > 0 ) {
            ioreq.irHandle = 0;             // write buffer
            if( signOffset == SIGN_OFFSET ) // if singOffset == 8, use EDC
              ioreq.irFlags = EDC;
            else ioreq.irFlags = 0;
	    ioreq.irAddress = ((CardAddress)iBlock << erasableBlockBits) + i0;
            ioreq.irByteCount = BLOCK;      // maybe readFlag, but no EDC
	    ioreq.irData = (void FAR1 *)buf;
	    checkStatus( flPhysicalWrite(&ioreq) );
	  }
	  else {
	    stopFlag = TRUE;
	    if( i0 == 0 )                   // change previous signature
	      tffscpy(signature+SIGNATURE_NAME,"FFFF",SIGNATURE_NUM);
	    break;
	  }
	}
      if( wBlock != 0 ) {                   // skip first time
	ioreq.irHandle = 0;                 // write signature
	ioreq.irFlags = EXTRA;
        ioreq.irAddress = addr + signOffset;
	ioreq.irByteCount = SIGNATURE_LEN;
	ioreq.irData = (void FAR1 *)signature;
	checkStatus( flPhysicalWrite(&ioreq) );
//	printf("Sign: %s\n",signature);
      }
      if( wBlock == (writeBlocks - 1) )     // last block
	stopFlag = TRUE;
      if( stopFlag )                        // create current signature
	tffscpy(signature+SIGNATURE_NAME,"FFFF",SIGNATURE_NUM);
      else {
        for(i0=wBlock,j=SIGNATURE_LEN;( j > SIGNATURE_NAME );j--) {
	  signature[j-1] = (unsigned char)((i0 % 10) + '0');
	  i0 /= 10;
	}
      }
      addr = (CardAddress)iBlock << erasableBlockBits;
      wBlock++;                             // another block written
    }
  }
  // write last signature
  ioreq.irHandle = 0;                       // write signature
  ioreq.irFlags = EXTRA;
  ioreq.irAddress = addr + signOffset;
  ioreq.irByteCount = SIGNATURE_LEN;
  ioreq.irData = (void FAR1 *)signature;
  checkStatus( flPhysicalWrite(&ioreq) );
//  printf("Sign: %s\n",signature);
  return(flOK);
}
//-------------------------------------------------------------------
void USAGE(void)
{
  PRINTF(TEXT("Usage: dformat [-s:{ bootimage | ! }] [-l:length] [-w:window] [-n:signature]\n"));
  PRINTF(TEXT("         -s:bootimage - write bootimage file to DiskOnChip\n"));
  PRINTF(TEXT("         -s:!         - mark bootimage area\n"));
  PRINTF(TEXT("         -l:length    - bootimage area length\n"));
  PRINTF(TEXT("         -w:window    - use window hex address explicitely\n"));
  PRINTF(TEXT("         -n:signature - signature for bootimage area (default - BIPO)\n"));
  PRINTF(TEXT("         -o:offset    - offset of signature for bootimage area (default - 8)\n"));
  PRINTF(TEXT("         -h | ?       - print help screen\n"));
#ifdef WRITE_EXB_IMAGE
  PRINTF(TEXT("         -e:exbimage  - write exbimage file to DiskOnChip (DOS Driver)\n"));
  PRINTF(TEXT("         -f           - make DiskOnChip the first disk in the system\n"));
#endif // WRITE_EXB_IMAGE
#if (OS_APP == OS_WINCE)
  PRINTF(TEXT("         -a:access    - width of the data access to the DiskOnChip\n"));
  PRINTF(TEXT("         -p:shift     - physical address shift before virtual converting (default - 8)\n"));
#endif // OS_WINCE
}
void Print_Ver(void)
{
  PRINTF(TEXT("DiskOnChip 2000 Formatter for %s, %s\n"),OS_NAME,VERSION_NAME);
  PRINTF(TEXT("%s\n\n"),COPYRIGHT);
}
//-------------------------------------------------------------------
#if (OS_APP == OS_DOS)
int cdecl main( int argc, char **argv )
#endif // OS_DOS
#if (OS_APP == OS_QNX)
int main( int argc, char **argv )
#endif // OS_QNX
#if (OS_APP == OS_WINCE)
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPTSTR lpCmdLine, int nCmdShow )
#endif // OS_WINCE
{
  IOreq ioreq;
  FLStatus status;
  PhysicalInfo phinfo;
  FILE_HANDLE fileBoot;
  int  i, j;
  unsigned short startBDKBlock = 0, nftlUnitSizeBits, mediaNoOfUnits;
  int  signOffset = SIGN_OFFSET, sflag = LEAVE_BOOT;
  FormatParams formatparams = STD_FORMAT_PARAMS;
  long bootFileSize = -1L;              // leave previous boot partition
  long tmpSize, bootUnits, exbFileSize = 0L;
  unsigned char sign[SIGNATURE_LEN];
  wchar_t *err;
  Anand FAR2 *anandPtr;
  ANANDPhysUnit FAR2 *physicalUnits;
#ifdef WRITE_EXB_IMAGE
  unsigned short flags;
  unsigned char exbFlags = 0;
  FILE_HANDLE fileExb;
  NFDC21Vars FAR2 *nfdcVars;
#endif // WRITE_EXB_IMAGE

#if (OS_APP == OS_DOS)
  fileBoot = -1;
#endif // OS_DOS

#if (OS_APP == OS_WINCE)
  int  argc;
  wchar_t *argv[20];
  int  docAccessType, physicalShift;
  // Convert lpCmdLine -> argc, argv
  argc = getArgFromStr(argv,lpCmdLine);
  fileBoot = NULL;
  docAccessType = DOC_ACCESS_TYPE;
  physicalShift = DOC_PHYSICAL_SHIFT;
#endif // OS_WINCE

  tffsset(sign,0xFF,SIGNATURE_LEN);     // set 0xFF signature
#ifndef SIGN_CHECK
  tffscpy(sign,defaultSignature,SIGNATURE_NAME);
#endif // ! SIGN_CHECK

  Print_Ver();

  for(i=ARG_START;( i < argc );i++) {           // Command Line Processing
    PRINTF(TEXT("%s\n"),argv[i]);
    if( *argv[i] == TEXT('-') )
      switch( toupper(*(argv[i]+1)) ) {
	case 'S':                        // BootImage file
           if( *(argv[i]+3) == TEXT('!') ) {
             sflag = ERASE_BOOT;
             bootFileSize = 0L;
           }
	   else {
	     sflag = WRITE_BOOT;
             fileBoot = fileOpen(argv[i]+3,O_RDONLY | O_BINARY);
             if( FILE_OPEN_ERROR(fileBoot) ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             tmpSize = fileLength(fileBoot);
             PRINTF(TEXT("File Size = %ld\n"),tmpSize);
             if( tmpSize == 0L ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             if( bootFileSize < tmpSize )
               bootFileSize = tmpSize;
           }
	   break;

	case 'W':                        // Window flag
	   window = STRTOL(argv[i]+3,&err,0);
	   PRINTF(TEXT("Window = %lx\n"),window);
	   break;

	case 'L':                        // Length flag
	   sflag = WRITE_BOOT;
           tmpSize = STRTOL(argv[i]+3,&err,0);
           if( bootFileSize < tmpSize )
             bootFileSize = tmpSize;
           PRINTF(TEXT("Boot Size = %ld\n"),tmpSize);
           break;

        case 'N':                       // Signature flag
//	   tffscpy(sign, argv[i]+3,SIGNATURE_NAME);
           for(j=0;( j < SIGNATURE_NAME );j++)
	     sign[j] = (unsigned char)(*(argv[i]+3+j));
	   break;

        case 'O':                       // Signature Offset flag
           signOffset = (int)STRTOL(argv[i]+3,&err,0);
           if( signOffset != SIGN_OFFSET )
             signOffset = 0;
           PRINTF(TEXT("Sign Offset = %d\n"),signOffset);
           break;

#ifdef WRITE_EXB_IMAGE
        case 'E':                       // Write EXB driver image
           if( *(argv[i]+3) == TEXT('!') ) {
             exbFileSize = 0L;
           }
           else {
	     sflag = WRITE_BOOT;
             fileExb = fileOpen(argv[i]+3,O_RDONLY | O_BINARY);
             if( FILE_OPEN_ERROR(fileExb) ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             tmpSize = fileLength(fileExb);
             PRINTF(TEXT("File Size = %ld\n"),tmpSize);
             exbFileSize = tmpSize;
           }
           break;

        case 'F':                       // Set 'Install_First' flag
           exbFlags |= INSTALL_FIRST;
	   break;
#endif // WRITE_EXB_IMAGE

#if (OS_APP == OS_WINCE)
        case 'A':                       // DiskOnChip data access flag
           docAccessType = (int)STRTOL(argv[i]+3,&err,0);
           PRINTF(TEXT("Doc Access Type = %d\n"),docAccessType);
           break;

        case 'P':                       // Physical address shift flag
           physicalShift = (int)STRTOL(argv[i]+3,&err,0);
           if( physicalShift != DOC_PHYSICAL_SHIFT )
             physicalShift = 0;
           PRINTF(TEXT("Physical Address Shift = %d\n"),physicalShift);
           break;
#endif // OS_WINCE

	case 'H':
	case '?':
	   USAGE();
	   return(flOK);

	default:
	   PRINTF(TEXT("Illegal option %s\n"),argv[i]);
	   USAGE();
	   return(flUnknownCmd);
      }
    else {
      PRINTF(TEXT("Illegal option %s\n"),argv[i]);
      USAGE();
      return(flUnknownCmd);
    }
  }

#ifdef SIGN_CHECK
  if( sflag == WRITE_BOOT )                        // signature check
    for(i=0;( i < SIGNATURE_NAME );i++)
      if( sign[i] == 0xFF ) {
	PRINTF(TEXT("Illegal signature: %s\n"),sign);
	return(flBadLength);
      }
#endif // SIGN_CHECK

#if (OS_APP == OS_WINCE)
  curDocParams.docAccess  = docAccessType;
  curDocParams.docShift   = physicalShift;
  curDocParams.docAddress = window;
#endif // OS_WINCE

  // Low level checking
  ioreq.irHandle = 0;
  ioreq.irData = &phinfo;
  if( (status = flGetPhysicalInfo(&ioreq)) != flOK ) {
    PRINTF(TEXT("DiskOnChip not found\n"));
    return(status);
  }

#ifdef FORMAT
  ioreq.irHandle = 0;
#if (OS_APP == OS_QNX)
  ioreq.irFlags = TL_FORMAT_ONLY;
#else
  ioreq.irFlags = TL_FORMAT;
#endif // OS_QNX
  /* maybe problem for 32 Kb units */
  nftlUnitSizeBits = getNFTLUnitSizeBits((PhysicalInfo FAR2 *)&phinfo);
  if( exbFileSize > 0 )
    exbFileSize = ((long)(1 + ((exbFileSize - 1) >> nftlUnitSizeBits))) << nftlUnitSizeBits;
  if( bootFileSize > 0 )
    bootFileSize = ((long)(1 + ((bootFileSize - 1) >> nftlUnitSizeBits))) << nftlUnitSizeBits;
  formatparams.bootImageLen = bootFileSize + exbFileSize;
  /* use boot area correction for big boot area */
  bootUnits = (unsigned short)(formatparams.bootImageLen >> nftlUnitSizeBits);
  mediaNoOfUnits = (unsigned short)(phinfo.mediaSize >> nftlUnitSizeBits);

  if( bootUnits >= mediaNoOfUnits ) {
    PRINTF(TEXT("Medium too small for given boot area!\n"));
    flExit();
    return(flVolumeTooSmall);
  }
  /* add 1% of bootUnits to transfer units */
  formatparams.percentUse -= (bootUnits) / (mediaNoOfUnits - bootUnits);
  ioreq.irData = &formatparams;
  PRINTF(TEXT("Formatting..."));
  status = flFormatVolume(&ioreq);
  if( status != flOK ) {
    PRINTF(TEXT("\nFormatting failed!\n"));
    flExit();
    return(status);
  }
#else // FORMAT
  ioreq.irHandle = 0;
  PRINTF(TEXT("Mounting..."));
  status = flAbsMountVolume(&ioreq);

  if( status != flOK ) {
    PRINTF(TEXT("\nMounting failed!\n"));
    flExit();
    return(status);
  }
#endif // ! FORMAT

  PRINTF(TEXT("Ok\n"));
/* ioreq.irHandle = 0;
  status = flSectorsInVolume(&ioreq);
  PRINTF(TEXT("Sectors in Volume = %ld\n"),ioreq.irLength); */

  anandPtr = getAnandRec(0);
  PRINTF(TEXT("Num of units = %u\n"),anandPtr->noOfUnits);
  // get bad blocks table
#ifdef MALLOC
  physicalUnits = (ANANDPhysUnit FAR2 *)MALLOC(anandPtr->noOfUnits * sizeof(ANANDPhysUnit));
  if( physicalUnits == NULL )
    return(flNotEnoughMemory);
  tffscpy( physicalUnits, anandPtr->physicalUnits,
	   anandPtr->noOfUnits * sizeof(ANANDPhysUnit));
#else
  physicalUnits = anandPtr->physicalUnits;
#endif
  // disMount before low_level operations
  ioreq.irHandle = 0;
  flDismountVolume(&ioreq);            // deletes bad block table

  if( sflag == WRITE_BOOT ) {
#ifdef WRITE_EXB_IMAGE
    if( exbFileSize > 0L ) {
      PRINTF(TEXT("Write Exb Image\n"));
      nfdcVars = getNFDC21Vars(0);
      /* flash.flags for BIG_PAGE, FULL_PAGE, var.flags for MDOC_ASIC */
      flags = anandPtr->flash.flags | nfdcVars->flags;
      if( (status = writeExbDriverImage( fileExb, (unsigned short FAR2 *)&startBDKBlock,
                      exbFlags, flags, signOffset, anandPtr, physicalUnits)) != flOK ) {
        PRINTF(TEXT("Exb Image Writing Fault\n"));
      }
      PRINTF(TEXT("Exb Image Size in Blocks = %d\n"),startBDKBlock);
    }
#endif // WRITE_EXB_IMAGE
    if( (status == flOK) && (bootFileSize > 0L) ) {
      PRINTF(TEXT("Write Boot Image\n"));
      if( (status = writeBootAreaFile( fileBoot, startBDKBlock, bootFileSize,
                      sign, signOffset, anandPtr, physicalUnits )) != flOK ) {
        PRINTF(TEXT("Boot Image Writing Fault\n"));
      }
    }
  }
#ifdef WRITE_EXB_IMAGE
  if( !FILE_OPEN_ERROR(fileExb) )
    fileClose(fileExb);
#endif // WRITE_EXB_IMAGE
  if( !FILE_OPEN_ERROR(fileBoot) )
    fileClose(fileBoot);
#ifdef MALLOC
  FREE(physicalUnits);
#endif
  flExit();
  if( status == flOK )
    PRINTF(TEXT("Ok\n"));
  return(status);
}
