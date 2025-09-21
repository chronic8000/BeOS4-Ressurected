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

//void iad_dprintf(const char * fmt, ...);
//#define dprintf iad_dprintf

void dbprintf(const char* fmt, ...);
void derror(const char* fmt, ...);
void dbnullprintf(const char* fmt, ...);

#define derror dbprintf("** i801 ** "); dbprintf

#if DEBUG_LEVEL == 1
	#define DB_PRINTF(x) {dprintf("********** i801: "); dprintf x;}
	#define ddprintf 	dbprintf("** i801 ** ");dbprintf
	#define ddprintf1	dbprintf("** i801 ** ");dbprintf
#else
	#define DB_PRINTF(x) (void)0
	#define ddprintf	dbnullprintf //pass ddprintf
	#define ddprintf1	dbprintf("** i801 ** ");dbprintf
#endif
