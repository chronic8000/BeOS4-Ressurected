//******************************************************************************
//
//	File:		main.c
//
//	Description:	.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#include <OS.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "lock.h"
#include "system.h"

#include <Debug.h>
#include <ctype.h>

#ifdef LEAKCHECKING
	#define DEBUG 1
	#include "support_p/LeakChecking.h"
	#undef DEBUG
#endif

/*-------------------------------------------------------------*/
extern "C" 	void	init_memory();
extern "C"	void	_kdprintf_(const char *, ...);
extern		void	init_debugger();

/*-------------------------------------------------------------*/

char app_server_executable[80];
char app_server_name[80] = "picasso";
char settingsFileName[128];

#ifdef MINI_SERVER
#include <Application.h>
#endif

int main(int argc, char **argv)
{
	TServer *server;

	strcpy(app_server_executable,argv[0]);

	if (argc>1) {
		strcpy(app_server_name,argv[1]);
		sprintf(settingsFileName,"app_server_settings.%s",app_server_name);
	} else {
		strcpy(app_server_name,"picasso");
		sprintf(settingsFileName,"app_server_settings");
	};

	#ifdef MINI_SERVER
		char sig[255];
		sprintf(sig,"application/x-vnd.Be-app_server-%s",app_server_name);
		BApplication app(sig);
	#endif

	if (find_thread(app_server_name) > 0) {
		printf("app_server '%s' is already running!\n",app_server_name);
		return -1;
	};

	#ifdef LEAKCHECKING
		SetNewLeakChecking(true); 
		SetMallocLeakChecking(true);
	#endif

	init_regions();
	init_system();
	#if AS_DEBUG
		init_debugger();
	#endif

	server = new TServer;
	server->run();

	return 0;
}

/*-------------------------------------------------------------*/

void xprintf(char *format, ...)
{
	va_list args;
	char buf[255];

	va_start(args,format);
	vsprintf (buf, format, args);
	va_end(args);
	_kdprintf_(buf);
}

