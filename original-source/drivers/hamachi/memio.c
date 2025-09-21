/*
    memio.c: All these functions are used to access I/O memory areas. 
                             
	russ 4/28/99	

	(c) 1997-99 Be, Inc. All rights reserved

*/

#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>


/* The following inline functions may be called after calling
   map_physical_memory(). These are like their Linux counterparts 
   after calling the Linux function vremap(). */
inline uint8 readb(char* address) 
{	
	uint8 result;
	
	result = *((uint8*)(address));
	
	return result;
}

inline uint16 readw(char* address) 
{
	
	uint16 result;

	result = *((uint16*)(address));
		
	return result;
}

inline uint32 readl(char* address) 
{	
	uint32 result;
	
	result = *((uint32*)(address));	
	
	return result;
}

inline void writeb(uint8 value,  char* address) 
{
	*((uint8*)(address)) = value;
}


inline void writew(uint16 value,  char* address) 
{
	*((uint16*)(address)) = value;
}


inline void writel(uint32 value,  char* address)  
{
	*((uint32*)(address)) = value;
}





