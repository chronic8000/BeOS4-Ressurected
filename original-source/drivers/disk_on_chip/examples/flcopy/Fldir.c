#include "fsapi.h"
#include "dosformt.h"
#include "blockdev.h"

#include <stdio.h>



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
 *  Function:    "isFlitePath"                                                   *
 *                                                                                           *
 *  Purpose:      Checks program argument : if it is valid flash path                        *
 *                                                                                           *
 *********************************************************************************************/

FLBoolean isFlitePath(char *path)
{
  return (!tffscmp("flite\\\\", path, 7));
}


/****************************************************************************************
 *                                                                                   	*
 *  Function:    "printFileName"                                               		*
 *                                                                                      *
 *  Purpose:     Receives a file name read from the directory entry  			*			
 *		 (11 characters) and prints it, jumping over spaces and			*
 *		 adding the '.' between the name and the extension (if 			*
 *		 exists).								*	
 *                                                                                      *
 ****************************************************************************************/

void printFileName(char *fileName)
{
  int i;

  for(i = 0; i < 8; i++) {
    if (fileName[i] == ' ')
      break;
    printf("%c", fileName[i]);
  }
  if (fileName[8] != ' ')
    printf(".");
  for(i = 8; i < 11; i++) {
    if (fileName[i] == ' ')
      break;
    printf("%c", fileName[i]);
  }
  printf("\n");
}

/*********************************************************************************************
 *                                                                                           *
 *                         M A I N                                                           *
 *                                                                                           *
 *********************************************************************************************/

int cdecl main(int argc,char *argv[])
{
  IOreq ioreq;
  FLStatus status;
  DirectoryEntry dirEntry;
  FLSimplePath dirPath[10];
  int flag = 0;

  if (argc != 2) {
    printf("Usage: fldir flite\\\\directory-path\n");
    exit(1);
  }
  if (!isFlitePath(argv[1])) {
    printf("Usage: fldir flite\\\\directory-path\n");
    exit(1);
  }

  ioreq.irHandle = 0;
  status = flMountVolume(&ioreq);
  errorMessage(status, "flMountVolume");

  ioreq.irData = argv[1] + 7;
  ioreq.irPath = dirPath;
  status = flParsePath(&ioreq);
  errorMessage(status, "flParsePath");

  ioreq.irData = &dirEntry;
  status = flFindFirstFile(&ioreq);
  while (status != flNoMoreFiles) {
    if (flag)
      errorMessage(status, "flFindNextFile");
    else {
      errorMessage(status, "flFindFirstFile");
      flag = 1;
    }
    printFileName(dirEntry.name);
    status = flFindNextFile(&ioreq);
  }

  return 0;
}
