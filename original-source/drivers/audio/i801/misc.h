/************************************************************************/
/*                                                                      */
/*                              misc.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#if !defined(_misc_h)
#define _misc_h

#include <SupportDefs.h>

area_id allocate_memory(const char* name, void ** log_addr, void ** phys_addr, size_t * size);

/* convert between sound.h constants and sample rates in Hz */
int32 sr_from_constant(int constant);
int constant_from_sr(int32 sr);

#endif	/* _misc_h */
