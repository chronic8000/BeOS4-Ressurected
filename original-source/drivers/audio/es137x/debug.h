/************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#ifdef DEBUG
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL 0
	#endif
#define DB_PRINTF(x) {dprintf("********** es1370_audio: "); dprintf x;}
#else
#define DB_PRINTF(x)
#endif

void dbprintf(const char* fmt, ...);
void derror(const char* fmt, ...);
void dbnullprintf(const char* fmt, ...);

#define derror dbprintf("** es1370_audio ** "); dbprintf

#if DEBUG_LEVEL == 1
	#define ddprintf 	dbprintf("** es1370_audio ** ");dbprintf
	#define ddprintf1	dbprintf("** es1370_audio ** ");dbprintf
#else
	#if DEBUG_LEVEL == 0
		#define ddprintf	dbnullprintf //pass ddprintf
		#define ddprintf1	dbprintf("** es1370_audio ** ");dbprintf
	#endif
#endif
