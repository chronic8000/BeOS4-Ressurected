/* ========================================================================== */
#include "be_files.h"
/* ========================================================================== */
#include <stdio.h>
/* ========================================================================== */
Int32 be_close ( Lib_IODriver fd )
{
	Int32 retval;

	retval = fclose((FILE *)fd);
	//printf("be_close retval=%ld\n", retval);
	
	return retval;
}
/* ========================================================================== */
Int32 be_read  ( Lib_IODriver fd, Pointer buffer, Int32 size)
{
	Int32 retval;

	retval = fread(buffer, 1, size, (FILE *)fd);
	//printf("be_read size=%ld retval=%ld\n", size, retval);
	
	return retval;
}
/* ========================================================================== */
Int32 be_write ( Lib_IODriver fd, Pointer buffer, Int32 size)
{
	Int32 retval;

	retval = fwrite(buffer, 1, size, (FILE *)fd);
	//printf("be_write size=%ld retval=%ld\n", size, retval);
	
	return retval;
}
/* ========================================================================== */
Int32 be_seek  ( Lib_IODriver fd, Int32 offset,   Lib_IOD_SeekMode mode)
{
	Int32 retval;

	int origin = SEEK_SET;
	
	switch(mode)
	{
		case Lib_IOD_SEEK_SET:
			origin = SEEK_SET;
			break;
		case Lib_IOD_SEEK_CUR:
			origin = SEEK_CUR;
			break;
		case Lib_IOD_SEEK_END:
			origin = SEEK_END;
			break;
		default:
			printf("be_seek UNKNOWN MODE\n");
	}

	retval = fseek((FILE *)fd, offset, origin);
	//printf("be_seek offset=%ld retval=%ld\n", offset, retval);
	
	return retval;
}
/* ========================================================================== */
