/*************************************************************************

  File Name: OsString.c

  File Description: This file implements all the string/memory utilities.

  Function list:

  GLOBAL:

  STATIC:


  ************************************************************************/
#include "typedefs.h"
#include "ostypedefs.h"
#include "stdio.h"

/********************************************************************************
	
Function Name: OsStrCpy.

Function Description:The function copies a specified a string from one 
					 buffer to another. This function may not work correctly if 
					 the buffers overlap. 

Arguments: BuffDest - Pointer to buffer to contain copied string.
		   BuffSrc - Pointer to buffer with the string to copy.
	
Return Value: A pointer to BuffDest.

********************************************************************************/
PVOID	OsStrCpy(PVOID BuffDest, PVOID BuffSrc)
{
	return strcpy(BuffDest, BuffSrc);
}

/********************************************************************************
	
Function Name: OsStrnCpy.
	
Function Description:The function copies a specified a string from one 
					 buffer to another. This function may not work correctly if 
					 the buffers overlap. 

Arguments: BuffDest - Pointer to buffer to contain copied string.
		   BuffSrc - Pointer to buffer with the string to copy.

Return Value: A pointer to BuffDest.

********************************************************************************/
PVOID	OsStrnCpy(PVOID BuffDest, PVOID BuffSrc, int MaxSize)
{
	return (PVOID)strncpy( BuffDest, BuffSrc, MaxSize);
}


int OsStrCmp(LPCSTR szStr1, LPCSTR szStr2)
{
	return strcmp(szStr1, szStr2);
}

/********************************************************************************
	
Function Name: OsToupper.

Function Description:converts a charecter c to upper case  

Arguments: c - charected to be converted to upper case.
	
Return Value: upper case charecter of c.

********************************************************************************/
int	OsToupper(int ch)
{
       return (ch >='a' && ch <= 'z') ? ch-'a'+'A' : ch;
}
/********************************************************************************
	
Function Name: OsTolower.
Function Description:converts a charecter c to lower case  

Arguments: c - charected to be converted to lower case.
	
Return Value: lower case charecter of c.

********************************************************************************/
int	OsTolower(int ch)
{
       return (ch >='A' && ch <= 'Z') ? ch-'A'+'a' : ch;
}
/********************************************************************************
	
Function Name: OsIsDigit.

Function tests if a charecter is a decimal digit. 

Arguments: c - integer to test.
	
Return Value: non zero value if c is a decimal digit('0' - '9').

********************************************************************************/
int	OsIsDigit(int ch)
{
       return (ch >='0' && ch <='9');
}

/********************************************************************************

Function Name: OsStrLen.
Function Description:The function returns the length of a null-terminated string.
Arguments: String - Pointer to buffer to contain copied string.

Return Value: int - the length of a null-terminated string.
********************************************************************************/
int	OsStrLen(LPCSTR String)
{
	return strlen(String);
}

/*int OsSprintf(LPSTR buffer, LPCSTR format, ...)
{
    int n;

//kprintf("OsSprintf() - 1\n");
	va_list args;
	va_start(args, format);
//    n = vsnprintf(buffer, 512, format, args);
    n = sprintf(buffer, format, args);

	va_end(args);

    return n;
}*/



