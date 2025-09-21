/*-----------------------------------------------------------------------------
 * GFX_DCDR.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the video decoder.
 * 
 *    gfx_set_decoder_defaults
 *    gfx_set_decoder_brightness
 *    gfx_set_decoder_contrast	
 *    gfx_set_decoder_hue
 *    gfx_set_decoder_saturation
 *    gfx_set_decoder_input_offset
 *    gfx_set_decoder_input_size
 *    gfx_set_decoder_output_size
 *    gfx_set_decoder_scale
 *    gfx_set_decoder_vbi_format
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *
 *    gfx_get_decoder_brightness
 *    gfx_get_decoder_contrast
 *    gfx_get_decoder_hue
 *    gfx_get_decoder_saturation
 *    gfx_get_decoder_input_offset
 *    gfx_get_decoder_input_size
 *    gfx_get_decoder_output_size
 *    gfx_get_decoder_vbi_format
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* INCLUDE SUPPORT FOR PHILIPS SAA7114 DECODER, IF SPECIFIED */

#if GFX_DECODER_SAA7114
#include "saa7114.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call various decoders.  Currently only the Pillips */
/* decoder is supported, but still organized to easily expand later. */

#if GFX_DECODER_DYNAMIC

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_defaults
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_defaults(void)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_defaults();
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_brightness
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_brightness(unsigned char brightness)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_brightness(brightness);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_contrast
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_contrast(unsigned char contrast)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_contrast(contrast);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_hue
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_hue(char hue)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_hue(hue);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_saturation
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_saturation(unsigned char saturation)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_saturation(saturation);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_input_offset
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_input_offset(unsigned short x, unsigned short y)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_input_offset(x, y);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_input_size
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_input_size(unsigned short width, unsigned short height)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_input_size(width, height);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_output_size
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_output_size(unsigned short width, unsigned short height)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_output_size(width, height);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_scale
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_scale(srcw, srch, dstw, dsth);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_vbi_format
 *-----------------------------------------------------------------------------
 */
int gfx_set_decoder_vbi_format(int format)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		status = saa7114_set_decoder_vbi_format(format);
	#endif
	return(status);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_brightness
 *-----------------------------------------------------------------------------
 */
unsigned char gfx_get_decoder_brightness(void)
{
	unsigned char brightness = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		brightness = saa7114_get_decoder_brightness();
	#endif
	return(brightness);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_contrast
 *-----------------------------------------------------------------------------
 */
unsigned char gfx_get_decoder_contrast(void)
{
	unsigned char contrast = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		contrast = saa7114_get_decoder_contrast();
	#endif
	return(contrast);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_hue
 *-----------------------------------------------------------------------------
 */
char gfx_get_decoder_hue(void)
{
	unsigned char hue = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		hue = saa7114_get_decoder_hue();
	#endif
	return((char)hue);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_saturation
 *-----------------------------------------------------------------------------
 */
unsigned char gfx_get_decoder_saturation(void)
{
	unsigned char saturation = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		saturation = saa7114_get_decoder_saturation();
	#endif
	return(saturation);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_input_offset
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_decoder_input_offset()
{
	unsigned long offset = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		offset = saa7114_get_decoder_input_offset();
	#endif
	return(offset);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_input_size
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_decoder_input_size()
{
	unsigned long size = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		size = saa7114_get_decoder_input_size();
	#endif
	return(size);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_output_size
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_decoder_output_size()
{
	unsigned long size = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		size = saa7114_get_decoder_output_size();
	#endif
	return(size);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_vbi_format
 *-----------------------------------------------------------------------------
 */
int gfx_get_decoder_vbi_format(void)
{
	int format = 0;
	#if	GFX_DECODER_SAA7114
	if (gfx_decoder_type == GFX_DECODER_SAA7114)
		format = saa7114_get_decoder_vbi_format();
	#endif
	return(format);
}

#endif /* GFX_READ_ROUTINES */

#endif /* GFX_DECODER_DYNAMIC */

/* END OF FILE */
