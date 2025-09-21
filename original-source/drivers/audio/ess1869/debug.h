/************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov						*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#ifdef DEBUG
	#define DB_PRINTF(x) dprintf("--->"); dprintf x 
#else
	#define DB_PRINTF(x)
#endif	
