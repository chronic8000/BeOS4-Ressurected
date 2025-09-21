/*
	verinfo .c 

	960926	Tilakraj Roy	Created
	970812	Tilakraj Roy	Ported to generic interface

	universal verson information.
	versions should only be retrieved using these functions
	since these versions are automatically updated by build.exe
	after every nmake.
*/

#include "verinfo.h"


UInt32	verGetFileMajorVersion ( void )
{
	return FILE_VER_MAJ;
}

UInt32	verGetFileMinorVersion ( void )
{
	return FILE_VER_MIN;
}

UInt32	verGetFileBuildVersion ( void )
{
	return FILE_VER_BLD;
}

UInt32	verGetProductMajorVersion ( void )
{
	return PROD_VER_MAJ;
}

UInt32	verGetProductMinorVersion ( void )
{
	return PROD_VER_MIN;
}


Pointer	verGetFileVersionString ( void )
{
	return (Pointer)FILE_VER_STR;
}

Pointer	verGetProductVersionString ( void )
{
	return (Pointer)PROD_VER_STR;
}


