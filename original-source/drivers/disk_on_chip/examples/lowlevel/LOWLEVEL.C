#include "fatlite.h"

#include <stdio.h>
#include <mem.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>



/*********************************************************************************************
 *                                                                                           *
 *  Function:    "errorMessage"                                                              *
 *                                                                                           *
 *  Purpose:      Checks system status at the current moment and outputs error information   *
 *                   if error has occured.                                                   *
 *                                                                                           *
 *   Inputs:       status = The result of the execution of the last action                   *
 *                 fliteFunction = Pointer to null terminated string which contains the name *
 *                 of the last action                                                        *
 *                                                                                           *
 *********************************************************************************************/

void errorMessage(FLStatus status, char *fliteFunction)
{
  if (status != flOK) {
    printf("Error: %s returned status: %d\n", fliteFunction, status);
    exit(1);
  }
}


/*********************************************************************************************
 *                                                                                           *
 *                         M A I N                                                           *
 *                                                                                           *
 *********************************************************************************************/

void cdecl main(int argc,char *argv[])
{
  IOreq ioreq;
  FLStatus status;
  int i,j,driveNo;
  unsigned char buff[SECTOR_SIZE];

  /*Check validity of program arguments*/
  if (argc < 2) {
    printf("Usage: lowlevel [drive no]\n");
    exit(1);
  }
  driveNo = (*argv[1])-((int)'0');

  /*Mount the volume*/
  ioreq.irHandle = driveNo;
  status = flMountVolume(&ioreq);
  errorMessage(status, "flMountVolume");

  for(i=0;i<SECTOR_SIZE;i++)
    buff[i]= (i & 0xff);
  
  for(i=100;i<110;i++) {
    ioreq.irHandle = driveNo;
    ioreq.irSectorCount = 1;
    ioreq.irData = buff;
    ioreq.irSectorNo = i;
    
    /* Write logical sector number i */
    status = flAbsWrite(&ioreq);
    errorMessage(status, "flAbsWrite");
  }

  for(i=100;i<110;i++) {
    for(j=0;j<SECTOR_SIZE;j++)
      buff[j]= 123;

    ioreq.irHandle = driveNo;
    ioreq.irSectorCount = 1;
    ioreq.irData = buff;
    ioreq.irSectorNo = i;
    
    /* Read logical sector number i */    
    status = flAbsRead(&ioreq);
    errorMessage(status, "flAbsRead");

    for(j=0;j<SECTOR_SIZE;j++)
      if(buff[j] != (j & 0xff))
        printf("Wrong data!\n");
  }
  printf("Low level test was succed!\n");
}

