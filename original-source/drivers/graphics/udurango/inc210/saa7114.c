/*-----------------------------------------------------------------------------
 * SAA7114.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the Philips SAA7114 video decoder.
 *  
 * History:
 *    Initial version ported from sample code provided by Philips.
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/*---------------------------*/
/*  TABLE OF DEFAULT VALUES  */
/*---------------------------*/

typedef struct tagGFX_SAA7114_INIT
{
	unsigned char index;
	unsigned char value;

} GFX_SAA7114_INIT;

GFX_SAA7114_INIT gfx_saa7114_init_values[] = {
	{ 0x01, 0x08 }, { 0x02, 0xC0 }, { 0x03, 0x10 }, { 0x04, 0x90 }, 
	{ 0x05, 0x90 }, { 0x06, 0xEB }, { 0x07, 0xE0 }, { 0x08, 0x98 }, 
	{ 0x09, 0x40 }, { 0x0A, 0x80 }, { 0x0B, 0x44 }, { 0x0C, 0x40 }, 
	{ 0x0D, 0x00 }, { 0x0E, 0x89 }, { 0x0F, 0x2E }, { 0x10, 0x0E }, 
	{ 0x11, 0x00 }, { 0x12, 0x00 }, { 0x13, 0x00 }, { 0x14, 0x00 }, 
	{ 0x15, 0x11 }, { 0x16, 0xFE }, { 0x17, 0xC0 }, { 0x18, 0x40 }, 
	{ 0x19, 0x80 }, { 0x30, 0xBC }, { 0x31, 0xDF }, { 0x32, 0x02 }, 
	{ 0x34, 0xCD }, { 0x35, 0xCC }, { 0x36, 0x3A }, { 0x38, 0x03 },
	{ 0x39, 0x10 }, { 0x3A, 0x00 }, { 0x40, 0x00 }, { 0x41, 0xFF }, 
	{ 0x42, 0xFF }, { 0x43, 0xFF }, { 0x44, 0xFF }, { 0x45, 0xFF }, 
	{ 0x46, 0xFF }, { 0x47, 0xFF }, { 0x48, 0xFF }, { 0x49, 0xFF },
	{ 0x4A, 0xFF }, { 0x4B, 0xFF }, { 0x4C, 0xFF }, { 0x4D, 0xFF },
	{ 0x4E, 0xFF }, { 0x4F, 0xFF }, { 0x50, 0xFF }, { 0x51, 0xFF },
	{ 0x52, 0xFF }, { 0x53, 0xFF }, { 0x54, 0xFF }, { 0x55, 0xFF },
	{ 0x56, 0xFF }, { 0x57, 0xFF }, { 0x58, 0x00 }, { 0x59, 0x47 },
	{ 0x5A, 0x06 }, { 0x5B, 0x83 }, { 0x5D, 0x3E }, { 0x5E, 0x00 },
	{ 0x80, 0x10 }, { 0x83, 0x00 }, { 0x84, 0x60 }, { 0x85, 0x00 }, 
	{ 0x86, 0xE5 }, { 0x87, 0x01 }, { 0x88, 0xF8 }, { 0x90, 0x00 }, 
	{ 0x91, 0x08 }, { 0x92, 0x10 }, { 0x93, 0x80 }, { 0x94, 0x00 }, 
	{ 0x95, 0x00 }, { 0x96, 0xD0 }, { 0x97, 0x02 }, { 0x98, 0x00 }, 
	{ 0x99, 0x00 }, { 0x9A, 0xF2 }, { 0x9B, 0x00 }, { 0x9C, 0xD0 }, 
	{ 0x9D, 0x02 }, { 0x9E, 0xF0 }, { 0x9F, 0x00 }, { 0xA0, 0x01 }, 
	{ 0xA1, 0x00 }, { 0xA2, 0x00 }, { 0xA4, 0x80 }, { 0xA5, 0x40 },
	{ 0xA6, 0x40 }, { 0xA8, 0x00 }, { 0xA9, 0x04 }, { 0xAA, 0x00 },
	{ 0xAC, 0x00 }, { 0xAD, 0x02 }, { 0xAE, 0x00 }, { 0xB0, 0x00 },
	{ 0xB1, 0x04 }, { 0xB2, 0x00 }, { 0xB3, 0x04 }, { 0xB4, 0x00 },
	{ 0xB8, 0x00 }, { 0xB9, 0x00 }, { 0xBA, 0x00 }, { 0xBB, 0x00 },
	{ 0xBC, 0x00 }, { 0xBD, 0x00 }, { 0xBE, 0x00 }, { 0xBF, 0x00 },
	{ 0xC0, 0x00 }, { 0xC1, 0x08 }, { 0xC2, 0x00 }, { 0xC3, 0x80 },
	{ 0xC4, 0x00 }, { 0xC5, 0x00 }, { 0xC6, 0xD0 }, { 0xC7, 0x02 },
	{ 0xC8, 0x00 }, { 0xC9, 0x00 }, { 0xCA, 0x06 }, { 0xCB, 0x01 }, 
	{ 0xCC, 0xD0 }, { 0xCD, 0x02 }, { 0xCE, 0x06 }, { 0xCF, 0x01 },
	{ 0xD0, 0x01 }, { 0xD1, 0x00 }, { 0xD2, 0x00 }, { 0xD4, 0x80 },
	{ 0xD5, 0x3F }, { 0xD6, 0x3F }, { 0xD8, 0x00 }, { 0xD9, 0x04 },
	{ 0xDA, 0x00 }, { 0xDC, 0x00 }, { 0xDD, 0x02 }, { 0xDE, 0x00 },
	{ 0xE0, 0x00 }, { 0xE1, 0x04 }, { 0xE2, 0x00 }, { 0xE3, 0x04 },
	{ 0xE4, 0x01 }, { 0xE8, 0x00 }, { 0xE9, 0x00 }, { 0xEA, 0x00 },
	{ 0xEB, 0x00 }, { 0xEC, 0x00 }, { 0xED, 0x00 }, { 0xEE, 0x00 }, 
	{ 0xEF, 0x00 },
};

#define GFX_NUM_SAA7114_INIT_VALUES sizeof(gfx_saa7114_init_values)/sizeof(GFX_SAA7114_INIT)

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_defaults
 *
 * This routine is called to set the initial register values of the 
 * video decoder.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_defaults(void)
#else
int gfx_set_decoder_defaults(void)
#endif
{
	int i;

	/* LOOP THROUGH INDEX/DATA PAIRS IN THE TABLE */

	for (i = 0; i < GFX_NUM_SAA7114_INIT_VALUES; i++)
	{
		gfx_i2c_write(SAA7114_CHIPADDR, 
			gfx_saa7114_init_values[i].index, 
			gfx_saa7114_init_values[i].value);
	}

	/* SUGGESTED SOFTWARE RESET */ 
	
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xC0);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xF0);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_brightness
 *
 * This routine sets the brightness of the video decoder. 
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_brightness(unsigned char brightness)
#else
int gfx_set_decoder_brightness(unsigned char brightness)
#endif
{
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_BRIGHTNESS, brightness); 
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_contrast
 *
 * This routine sets the contrast of the video decoder. 
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_contrast(unsigned char contrast)
#else
int gfx_set_decoder_contrast(unsigned char contrast)
#endif
{
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_CONTRAST, (unsigned char) (contrast >> 1)); 
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_hue
 *
 * This routine sets the hue control of the video decoder. 
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_hue(char hue)
#else
int gfx_set_decoder_hue(char hue)
#endif
{
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HUE, (unsigned char) hue); 
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_saturation
 *
 * This routine sets the saturation adjustment of the video decoder. 
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_saturation(unsigned char saturation)
#else
int gfx_set_decoder_saturation(unsigned char saturation)
#endif
{
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_SATURATION, (unsigned char) (saturation >> 1)); 
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_input_offset
 *
 * This routine sets the size of the decoder input window.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_input_offset(unsigned short x, unsigned short y)
#else
int gfx_set_decoder_input_offset(unsigned short x, unsigned short y)
#endif
{
	/* SET THE INPUT WINDOW OFFSET */

	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_OFFSET_LO, 
		(unsigned char) (x & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_OFFSET_HI, 
		(unsigned char) (x >> 8));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_OFFSET_LO, 
		(unsigned char) (y & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_OFFSET_HI, 
		(unsigned char) (y >> 8));

	/* SUGGESTED SOFTWARE RESET */ 
	
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xC0);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xF0);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_input_size
 *
 * This routine sets the size of the decoder input window.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_input_size(unsigned short width, unsigned short height)
#else
int gfx_set_decoder_input_size(unsigned short width, unsigned short height)
#endif
{
	/* DIVIDE HEIGHT BY TWO FOR INTERLACING */

	height = (height + 1) >> 1;

	/* SET THE INPUT WINDOW SIZE */

	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_INPUT_LO, 
		(unsigned char) (width & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_INPUT_HI, 
		(unsigned char) (width >> 8));
	height += 2; /* Philips sample code adds 2 extra lines for overrun */
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_INPUT_LO, 
		(unsigned char) (height & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_INPUT_HI, 
		(unsigned char) (height >> 8));

	/* SUGGESTED SOFTWARE RESET */ 
	
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xC0);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xF0);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_output_size
 *
 * This routine sets the size of the decoder output window.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_output_size(unsigned short width, unsigned short height)
#else
int gfx_set_decoder_output_size(unsigned short width, unsigned short height)
#endif
{
	/* DIVIDE HEIGHT BY TWO FOR INTERLACING */

	height = (height + 1) >> 1;

	/* SET THE OUTPUT WINDOW SIZE */

	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_OUTPUT_LO, 
		(unsigned char) (width & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_OUTPUT_HI, 
		(unsigned char) (width >> 8));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_OUTPUT_LO, 
		(unsigned char) (height & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VERT_OUTPUT_HI, 
		(unsigned char) (height >> 8));

	/* SUGGESTED SOFTWARE RESET */ 
	
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xC0);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xF0);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_scale
 *
 * This routine sets the scaling of the video decoder.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
#else
int gfx_set_decoder_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
#endif
{
	unsigned char prescale = 0;
	int scale = 0;

	/* SET THE HORIZONTAL PRESCALE */
	/* Downscale from 1 to 1/63 source size. */

	if (dstw) prescale = (unsigned char) srcw/dstw;
	if (!prescale) prescale = 1; 
	if (prescale > 63) return(1);
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HORZ_PRESCALER, prescale);

	/* SET THE HORIZONTAL SCALING */

	if (!dstw) return(1);
	scale = (int) (1024.*((1.*srcw)/(1.*dstw))*(1./prescale));
	if ((scale > 8191) || (scale < 300)) return(1);
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HSCALE_LUMA_LO,
		(unsigned char) (scale & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HSCALE_LUMA_HI,
		(unsigned char) (scale >> 8));
	scale >>= 1;
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HSCALE_CHROMA_LO,
		(unsigned char) (scale & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_HSCALE_CHROMA_HI,
		(unsigned char) (scale >> 8));

	/* SET THE VERTICAL SCALING (INTERPOLATION MODE) */
		
	if (!dsth) return(1);
	scale = (int)((1024 * srch) / dsth);
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_LUMA_LO,
		(unsigned char) (scale & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_LUMA_HI,
		(unsigned char) (scale >> 8));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_CHROMA_LO,
		(unsigned char) (scale & 0x00FF));
	gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_CHROMA_HI,
		(unsigned char) (scale >> 8));

	if (dsth >= (srch >> 1))
	{
		/* USE INTERPOLATION MODE FOR SCALE FACTOR ABOVE 0.5 */

		gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_CONTROL, 0x00);
	}
	else
	{
		/* USE ACCUMULATION MODE FOR DOWNSCALING BY MORE THAN 2x */

		gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_VSCALE_CONTROL, 0x01);

		/* ADJUST CONTRAST AND SATURATION FOR ACCUMULATION MODE */

		if (srch) scale = (64 * dsth) / srch;
		gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_FILTER_CONTRAST, 
			(unsigned char) scale);
		gfx_i2c_write(SAA7114_CHIPADDR, SAA7114_FILTER_SATURATION, 
			(unsigned char) scale);
	}

	/* SUGGESTED SOFTWARE RESET */ 
	
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xC0);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x88, 0xF0);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_decoder_vbi_format
 *
 * This routine programs the decoder to produce the specified format for 
 * the VBI data.
 *
 *     0 = disabled
 *     1 = raw VBI data
 *     2 = US closed caption
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_vbi_format(int format)
#else
int gfx_set_decoder_vbi_format(int format)
#endif
{
	unsigned char data = 0xFF;
	switch(format)
	{
		case 1: data = 0x77; break; /* raw VBI data */
		case 2: data = 0x55; break; /* US closed caption */
	}

	/* PROGRAM LINES 20 AND 21 FOR THE SPECIFIED FORMAT */

	gfx_i2c_write(SAA7114_CHIPADDR, 0x53, data);
	gfx_i2c_write(SAA7114_CHIPADDR, 0x54, data);
	return(0);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_brightness
 *
 * This routine returns the current brightness of the video decoder.  
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char saa7114_get_decoder_brightness(void)
#else
unsigned char gfx_get_decoder_brightness(void)
#endif
{
	unsigned char brightness = 0;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_BRIGHTNESS, &brightness);
	return(brightness);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_contrast
 *
 * This routine returns the current contrast of the video decoder.  
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char saa7114_get_decoder_contrast(void)
#else
unsigned char gfx_get_decoder_contrast(void)
#endif
{
	unsigned char contrast = 0;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_CONTRAST, &contrast);
	contrast <<= 1;
	return(contrast);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_hue
 *
 * This routine returns the current hue of the video decoder.  
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
char saa7114_get_decoder_hue(void)
#else
char gfx_get_decoder_hue(void)
#endif
{
	unsigned char hue = 0;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HUE, &hue);
	return((char)hue);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_saturation
 *
 * This routine returns the current saturation of the video decoder.  
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char saa7114_get_decoder_saturation(void)
#else
unsigned char gfx_get_decoder_saturation(void)
#endif
{
	unsigned char saturation = 0;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_SATURATION, &saturation);
	saturation <<= 1;
	return(saturation);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_input_offset
 *
 * This routine returns the offset into the input window.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long saa7114_get_decoder_input_offset()
#else
unsigned long gfx_get_decoder_input_offset()
#endif
{
	unsigned long value = 0;
	unsigned char data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_OFFSET_LO, &data);
	value = (unsigned long) data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_OFFSET_HI, &data);
	value |= ((unsigned long) data) << 8;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_OFFSET_LO, &data);
	value |= ((unsigned long) data) << 16;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_OFFSET_HI, &data);
	value |= ((unsigned long) data) << 24;
	return(value);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_input_size
 *
 * This routine returns the current size of the input window
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long saa7114_get_decoder_input_size()
#else
unsigned long gfx_get_decoder_input_size()
#endif
{
	unsigned long value = 0;
	unsigned char data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_INPUT_LO, &data);
	value = (unsigned long) data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_INPUT_HI, &data);
	value |= ((unsigned long) data) << 8;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_INPUT_LO, &data);
	value |= ((unsigned long) data) << 17;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_INPUT_HI, &data);
	value |= ((unsigned long) data) << 25;
	return(value);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_output_size
 *
 * This routine returns the current size of the output window.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long saa7114_get_decoder_output_size()
#else
unsigned long gfx_get_decoder_output_size()
#endif
{
	unsigned long value = 0;
	unsigned char data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_OUTPUT_LO, &data);
	value = (unsigned long) data;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_HORZ_OUTPUT_HI, &data);
	value |= ((unsigned long) data) << 8;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_OUTPUT_LO, &data);
	value |= ((unsigned long) data) << 17;
	gfx_i2c_read(SAA7114_CHIPADDR, SAA7114_VERT_OUTPUT_HI, &data);
	value |= ((unsigned long) data) << 25;
	return(value);
}

/*-----------------------------------------------------------------------------
 * gfx_get_decoder_vbi_format
 *
 * This routine returns the current format of the VBI data.
 *-----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int saa7114_get_decoder_vbi_format(void)
#else
int gfx_get_decoder_vbi_format(void)
#endif
{
	unsigned char format = 0, data;
	gfx_i2c_read(SAA7114_CHIPADDR, 0x54, &data);
	if (data == 0x77) format = 1;
	else if (data == 0x55) format = 2;
	return(format);
}

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
