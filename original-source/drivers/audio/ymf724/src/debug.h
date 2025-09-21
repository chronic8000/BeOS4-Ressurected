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
#endif

void dbprintf(const char* fmt, ...);
void derror(const char* fmt, ...);
void dbnullprintf(const char* fmt, ...);

#define derror dbprintf("** ymf724 ** "); dbprintf

#if DEBUG_LEVEL == 1
	#define ddprintf 	dbprintf("** ymf724 ** ");dbprintf
	#define ddprintf1	dbprintf("** ymf724 ** ");dbprintf
#else
	#if DEBUG_LEVEL == 0
		#define ddprintf	dbnullprintf //pass ddprintf
		#define ddprintf1	dbprintf("** ymf724 ** ");dbprintf
	#endif
#endif
