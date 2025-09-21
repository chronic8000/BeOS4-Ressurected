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

static int port;

void
iad_dprintf(const char * fmt, ...)
{
	if (port <= 0) {
		port = find_port("mvp4 logger");
	}
	if (port > 0) {
		char msg[256];
		va_list ap;
		va_start(ap, fmt);
		vsprintf(msg, fmt, ap);
		va_end(ap);
		write_port(port, 0, msg, strlen(msg));
	}
}

void dbprintf(const char* fmt, ...)
{

#ifdef DEBUG
	char msg[256];
	va_list ap;
	
	sprintf (msg, "*** thread_id = %lX ***",find_thread(NULL));
	va_start(ap,fmt);
	vsprintf(&msg[strlen(msg)],fmt,ap);
	dprintf ("%s", msg);
	va_end(ap);
#endif

}


void dbnullprintf(const char* fmt, ...){}

