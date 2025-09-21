/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/OsString_ex.h_v   1.3   20 Apr 2000 14:19:30   woodrw  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			OsString.h	

File Description:	This file implements all the string/memory utilities.	

*****************************************************************************************/


/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2000                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                             Personal Computing Division                           ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
*****************************************************************************************/


#define OSSPRINTF(x)	sprintf(x)
/********************************************************************************

Function Name: OsSprintf.

Function Description: System independent wrapper on ssprintf();

********************************************************************************/
//int OsSprintf(LPSTR buffer, LPCSTR format, ...);
#define OsSprintf sprintf

/********************************************************************************

Function Name: OsStrCpy.

Function Description:The function copies a specified a string from one 
					 buffer to another. This function may not work correctly if 
					 the buffers overlap. 

Arguments: BuffDest - Pointer to buffer to contain copied string.
		   BuffSrc - Pointer to buffer with the string to copy.

Return Value: A pointer to BuffDest.

********************************************************************************/
PVOID	OsStrCpy(PVOID BuffDest, PVOID BuffSrc);


/********************************************************************************

Function Name: OsStrnCpy.

Function Description:The function copies a specified a string from one 
					 buffer to another. The function does not copy more 
					 than MaxSize bytes.The behavior of strncpy is 
					 undefined if the source and destination strings overlap

Arguments: BuffDest - Pointer to buffer to contain copied string.
		   BuffSrc - Pointer to buffer with the string to copy.
		   MaxSize - Max chars to copy

Return Value: A pointer to BuffDest.

********************************************************************************/
PVOID	OsStrnCpy(PVOID BuffDest, PVOID BuffSrc, int MaxSize);


/********************************************************************************

Function Name: OsStrCmp.

Function Description: Compare two strings. See ANSI-C strcmp() for description

********************************************************************************/
int OsStrCmp(LPCSTR szStr1, LPCSTR szStr2);


/********************************************************************************

Function Name: OsToupper.

Function Description:converts a charecter c to upper case  

Arguments: c - charected to be converted to upper case.

Return Value: upper case charecter of c.

********************************************************************************/
int	OsToupper(int c);


/********************************************************************************

Function Name: OsTolower.

Function Description:converts a charecter c to lower case  

Arguments: c - charected to be converted to lower case.

Return Value: lower case charecter of c.

********************************************************************************/
int	OsTolower(int c);


/********************************************************************************

Function Name: OsIsDigit.

Function tests if a charecter is a decimal digit. 

Arguments: c - integer to test.

Return Value: non zero value if c is a decimal digit('0' - '9').

********************************************************************************/
int	OsIsDigit(int c);


/********************************************************************************

Function Name: OsStrLen.

Function Description:The function returns the length of a null-terminated string.

Arguments: String - Pointer to buffer to contain copied string.

Return Value: int - the length of a null-terminated string.

********************************************************************************/
int	OsStrLen(LPCSTR String);

