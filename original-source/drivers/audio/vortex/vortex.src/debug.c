/************************************************************************/
/*                                                                      */
/*                              debug.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <KernelExport.h>

void dbprintf(const char* fmt, ...)
{

#ifdef DEBUG
	char msg[256];
	va_list ap;
	
	dprintf ("*** thread_id = %lX ***",find_thread(NULL));
	va_start(ap,fmt);
	vsprintf(msg,fmt,ap);
	dprintf (msg);
	va_end(ap);
#endif

}


void dbnullprintf(const char* fmt, ...){}

