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
		eprintf(("unknown SR constant %d\n", constant));
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
		eprintf(("unknown SR value %d\n", sr));
	}
	return kHz_44_1; /* a default */
}


