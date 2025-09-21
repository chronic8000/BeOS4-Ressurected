/************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov						*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

//#define DEBUG

//extern void iad_dprintf(const char *, ...);
//#define dprintf iad_dprintf

#ifdef DEBUG
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL 1
	#endif
#else
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL 0
	#endif
#endif

#ifdef DEBUG
	#define DB_PRINTF(x) dprintf(__FILE__); dprintf(" "); dprintf x ;
#else
	#define DB_PRINTF(x)
#endif

#define DB_PRINTF1(x)
#define DB_PRINTF2(x)

