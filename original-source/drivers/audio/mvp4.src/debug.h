/************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#ifdef DEBUG
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL 1
	#endif
#endif

void iad_dprintf(const char * fmt, ...);
//#define dprintf iad_dprintf

void dbprintf(const char* fmt, ...);
void derror(const char* fmt, ...);
void dbnullprintf(const char* fmt, ...);

#define derror dbprintf("** mvp4 ** "); dbprintf
#define DB_PRINTF(x) {dprintf("********** mvp4: "); dprintf x;}

#if DEBUG_LEVEL == 1
	#define ddprintf 	dbprintf("** mvp4 ** ");dbprintf
	#define ddprintf1	dbprintf("** mvp4 ** ");dbprintf
#else
	#if DEBUG_LEVEL == 0
		#define ddprintf	dbnullprintf //pass ddprintf
		#define ddprintf1	dbprintf("** mvp4 ** ");dbprintf
	#endif
#endif
