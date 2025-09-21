/************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#ifndef DEBUG_LEVEL
	#define DEBUG_LEVEL 0
#endif

void dbprintf(const char* fmt, ...);
void derror(const char* fmt, ...);
void dbnullprintf(const char* fmt, ...);

#define derror dbprintf("** null_audio ** "); dbprintf
#define DB_PRINTF(x) {dprintf("********** null_audio: "); dprintf x;}

#if DEBUG_LEVEL == 1
	#define ddprintf 	dbprintf("** null_audio ** ");dbprintf
	#define ddprintf1	dbprintf("** null_audio ** ");dbprintf
#else
	#if DEBUG_LEVEL == 0
		#define ddprintf	dbnullprintf //pass ddprintf
		#define ddprintf1	dbprintf("** null_audio ** ");dbprintf
	#endif
#endif
