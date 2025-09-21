/************************************************************************/
/*                                                                      */
/*                              misc.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include <KernelExport.h>
#include "debug.h"
#include "sound.h"

area_id allocate_memory(const char* name, void ** log_addr, void ** phys_addr, size_t * size)
{
	area_id mem_area;
	physical_entry where; 
	void *addr;
	size_t new_size;
	
	/* Attempt to use old buffer */
/*
	sprintf(name, "%s buf", "ymf724/1"); 
	mem_area = find_area(name);
	if (mem_area >= 0) {
		ddprintf("allocate_buffer(): testing old bufer...\n");
		if ((get_area_info(mem_area, &ainfo) == B_OK) &&
			(ainfo.size >= (*size)) &&
			(get_memory_map(ainfo.address, ainfo.size, &where, 1) == B_OK) &&
			(ainfo.lock >= B_FULL_LOCK) && (where.size >= (*size))
			) {
		  ddprintf("allocate_buffer(): using existing area - physical %x, logical %x\n", where.address, ainfo.address);
		  *log_addr = (void *)ainfo.address;
		  *phys_addr = (void *)where.address;
		  *size = ainfo.size; 
		  return mem_area;
		}
	}
	ddprintf("allocate_buffer(): old buffer test failed! Creating new buffer ...\n");
	if (mem_area >= 0) {
		delete_area(mem_area); 
		mem_area = -1;
	}
*/
	new_size = (*size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);	/* multiple of page size */
	mem_area = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, 
							new_size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (mem_area < 0) {
		ddprintf("allocate_buffer(): failed!\n");
		return mem_area;
	}
	if (get_memory_map(addr, new_size, &where, 1) < 0) {
		delete_area(mem_area);
		mem_area = B_ERROR;
		ddprintf("allocate_buffer(): get_memory_map() failed!\n");		
		return B_ERROR;
	}
	ddprintf("Created memory area: areaId = %lX, phys addr = %lX, log addr = %lX, size = %lX\n",mem_area,(ulong)(where.address), (ulong)addr, new_size);
	*log_addr = (void *)addr;
	*phys_addr = (void *)where.address;
	*size = new_size; 

	return mem_area;
}

static void EnableInterrupts(cpu_status* cp, bool enable)
{
	static int32 disabled = 0;
	if(enable) /*restore_interrupts()*/
	{
		if(atomic_add(&disabled,-1) == 1) restore_interrupts(*cp);
		return;
	}
	else /*disable_interrupts()*/
	{
		if(atomic_add(&disabled,1) == 0) *cp = disable_interrupts();
		return;
	}
}

cpu_status DisableInterrupts()
{	
	cpu_status ret;
	EnableInterrupts(&ret, false);
	return ret;
}

void RestoreInterrupts(cpu_status cp)
{
	cpu_status ret;
	EnableInterrupts(&ret, true);
	return;
}

int32 sr_from_constant(int constant)
{
	switch (constant) {
	case kHz_8_0: return 8000;
	case kHz_5_51: return 5510;
	case kHz_16_0: return 16000;
	case kHz_11_025: return 11025;
	case kHz_27_42: return 27420;
	case kHz_18_9: return 18900;
	case kHz_32_0: return 32000;
	case kHz_22_05: return 22050;
	case kHz_37_8: return 37800;
	case kHz_44_1: return 44100;
	case kHz_48_0: return 48000;
	case kHz_33_075: return 33075;
	case kHz_9_6: return 9600;
	case kHz_6_62: return 6620;
	default:
		derror("unknown SR constant %d\n", constant);
	}
	return 44100; /* a default */
}

int constant_from_sr(int32 sr)
{
	/* round to nearest multiple of 5 */
	sr = (sr+2)/5;
	sr *= 5;

	switch (sr) {
	case 8000: return kHz_8_0;
	case 5510: return kHz_5_51;
	case 16000: return kHz_16_0;
	case 11025: return kHz_11_025;
	case 27420: return kHz_27_42;
	case 18900: return kHz_18_9;
	case 32000: return kHz_32_0;
	case 22050: return kHz_22_05;
	case 37800: return kHz_37_8;
	case 44100: return kHz_44_1;
	case 48000: return kHz_48_0;
	case 33075: return kHz_33_075;
	case 9600: return kHz_9_6;
	case 6620: return kHz_6_62;
	default:
		derror("unknown SR value %d\n", sr);
	}
	return kHz_44_1; /* a default */
}


