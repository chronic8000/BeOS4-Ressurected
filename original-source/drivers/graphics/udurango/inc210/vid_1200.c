/*-----------------------------------------------------------------------------
 * VID_1200.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the SC1200 video overlay hardware.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* GLOBAL VARIABLES */
/* Needed to save intended color key values since the color key */
/* registers are required to truly disable the video window. */

unsigned long sc1200_color_key = 0;
unsigned long sc1200_color_mask = 0;
int sc1200_alpha_enable[3];

/*----------------------------------------------------------------------------
 * SC1200 PLL TABLE
 *----------------------------------------------------------------------------
 */

typedef struct tagSC1200PLL
{
	long frequency;					/* 16.16 fixed point frequency */
	unsigned long clock_select;		/* clock select register (0x2C) */
} SC1200PLL;

SC1200PLL gfx_sc1200_clock_table[] = {                                 
	{ 0x00192CCC, 0x00000000 }, /*  25.1750 */
    { 0x001F8000, 0x00010000 }, /*  31.5000 */
    { 0x00240000, 0x00020000 }, /*  36.0000 */
    { 0x00280000, 0x00030000 }, /*  40.0000 */
    { 0x00318000, 0x00050000 }, /*  49.5000 */
    { 0x00320000, 0x00040000 }, /*  50.0000 */
	{ 0x00384000, 0x00060000 }, /*  56.2500 */
	{ 0x00410000, 0x00080000 }, /*  65.0000 */
	{ 0x004E8000, 0x000A0000 }, /*  78.5000 */
	{ 0x005E8000, 0x000B0000 }, /*  94.5000 */
	{ 0x006C0000, 0x000C0000 }, /* 108.0000 */
	{ 0x00870000, 0x000D0000 }, /* 135.0000 */
};

#define NUM_SC1200_FREQUENCIES sizeof(gfx_sc1200_clock_table)/sizeof(SC1200PLL)

/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_reset_video(void)
#else
void gfx_reset_video(void)
#endif
{
	gfx_set_video_enable(0);
	gfx_select_alpha_region(0);
	gfx_set_alpha_enable(0);
	gfx_select_alpha_region(1);
	gfx_set_alpha_enable(0);
	gfx_select_alpha_region(2);
	gfx_set_alpha_enable(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_clock_frequency
 *
 * This routine sets the clock frequency, specified as a 16.16 fixed point
 * value (0x00318000 = 49.5 MHz).  It will set the closest frequency found
 * in the lookup table.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_set_clock_frequency(unsigned long frequency)
#else
void gfx_set_clock_frequency(unsigned long frequency)
#endif
{
	int index;
	unsigned long value;
	long min, diff;

	/* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
	/* Search the table for the closest frequency (16.16 format). */

	value = gfx_sc1200_clock_table[0].clock_select;
	min = (long) gfx_sc1200_clock_table[0].frequency - frequency;
	if (min < 0L) min = -min;
	for (index = 1; index < NUM_SC1200_FREQUENCIES; index++)
	{
		diff = (long) gfx_sc1200_clock_table[index].frequency - frequency;
		if (diff < 0L) diff = -diff;
		if (diff < min)
		{
			min = diff; 
			value = gfx_sc1200_clock_table[index].clock_select;
		}
	}

	/* SET THE DOT CLOCK REGISTER */

	WRITE_VID32(SC1200_VID_MISC, 0x00001000);
	WRITE_VID32(SC1200_VID_CLOCK_SELECT, value);
	return;
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_enable
 *
 * This routine enables or disables the video overlay functionality.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_enable(int enable)
#else
int gfx_set_video_enable(int enable)
#endif
{
	int i;
	unsigned long alpha, vcfg;

	/* WAIT FOR VERTICAL BLANK TO START */
	/* Otherwise a glitch can be observed. */

	if (gfx_test_timing_active())
	{
		if (!gfx_test_vertical_active())
		{
			while(!gfx_test_vertical_active());
		}
		while(gfx_test_vertical_active());
	}
	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	if (enable)
	{
		/* ENABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
		/* Use private routine to abstract the display controller. */

		gfx_set_display_video_enable(1);
		
		/* SET SC1200 BUS CONTROL PARAMETERS */
		/* Currently always high speed, 8-bit interface. */

		vcfg |= SC1200_VCFG_HIGH_SPD_INT;
		vcfg &= ~(SC1200_VCFG_EARLY_VID_RDY | SC1400_VCFG_16_BIT_EN);

		/* ENABLE SC1200 VIDEO OVERLAY */

		vcfg |= SC1200_VCFG_VID_EN;
		WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

		/* RESTORE SC1200 COLOR KEY VALUE */

		WRITE_VID32(SC1200_VIDEO_COLOR_KEY, sc1200_color_key);
		WRITE_VID32(SC1200_VIDEO_COLOR_MASK, sc1200_color_mask);

		/* RESTORE ALPHA ENABLE FOR ALL 3 REGIONS */

		for (i = 0; i < 3; i++)
		{
			if (sc1200_alpha_enable[i])
			{
				alpha = READ_VID32(SC1200_ALPHA_CONTROL_1 + (i << 4));
				alpha |= (1L << 16);
				WRITE_VID32(SC1200_ALPHA_CONTROL_1 + (i << 4), alpha);
			}
		}
	}
	else
	{
		/* DISABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
		/* Use private routine to abstract the display controller. */

		gfx_set_display_video_enable(0);
		
		/* DISABLE SC1200 VIDEO OVERLAY */
		/* Really only blanks video - hence use of color key. */

		vcfg &= ~SC1200_VCFG_VID_EN;
		WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

		/* USE COLOR KEY VALUE TO REALLY DISABLE WINDOW */
		/* Key and mask set to 0xFFFFFF will disable window */

		WRITE_VID32(SC1200_VIDEO_COLOR_KEY, 0xFFFFFF);
		WRITE_VID32(SC1200_VIDEO_COLOR_MASK, 0xFFFFFF);

		/* DISABLE ALL 3 ALPHA REGIONS */

		for (i = 0; i < 3; i++)
		{
			alpha = READ_VID32(SC1200_ALPHA_CONTROL_1 + (i << 4));
			alpha &= ~(1 << 16);
			WRITE_VID32(SC1200_ALPHA_CONTROL_1 + (i << 4), alpha);
		}
	}
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_format
 *
 * Currently only sets 4:2:2 format, Y1 V Y0 U.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_format(unsigned long format)
#else
int gfx_set_video_format(unsigned long format)
#endif
{
	unsigned long ctrl, vcfg = 0;

	/* SET THE SC1200 VIDEO INPUT FORMAT */

	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	vcfg &= ~(SC1200_VCFG_VID_INP_FORMAT | SC1200_VCFG_4_2_0_MODE);
	vcfg &= ~(SC1200_VCFG_CSC_BYPASS);
	if (format < 4) vcfg |= (format << 2);
	WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

	/* SET THE VIDEO COLOR SPACE CONVERSION BIT */
	/* ### Currently always set for 4:2:2 YUV */

	ctrl = READ_VID32(SC1200_VID_ALPHA_CONTROL);
	ctrl |= (1 << 10); /* set bit 10 */
	WRITE_VID32(SC1200_VID_ALPHA_CONTROL, ctrl);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_size
 *
 * This routine specifies the size of the source data.  It is used only 
 * to determine how much data to transfer per frame, and is not used to 
 * calculate the scaling value (that is handled by a separate routine).
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_size(unsigned short width, unsigned short height)
#else
int gfx_set_video_size(unsigned short width, unsigned short height)
#endif
{
	unsigned long size, vcfg;

	/* SET THE SC1200 VIDEO LINE SIZE */

	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	vcfg &= ~(SC1200_VCFG_LINE_SIZE_LOWER_MASK | SC1200_VCFG_LINE_SIZE_UPPER);
	size = (width >> 1);
	vcfg |= (size & 0x00FF) << 8;
	if (size & 0x0100) vcfg |= SC1200_VCFG_LINE_SIZE_UPPER;
	WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

	/* SET TOTAL VIDEO BUFFER SIZE IN DISPLAY CONTROLLER */
	/* Use private routine to abstract the display controller. */

	gfx_set_display_video_size(width, height);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_offset
 *
 * This routine sets the starting offset for the video buffer when only 
 * one offset needs to be specified.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_offset(unsigned long offset)
#else
int gfx_set_video_offset(unsigned long offset)
#endif
{
	/* SAVE VALUE FOR FUTURE CLIPPING OF THE TOP OF THE VIDEO WINDOW */

	gfx_vid_offset = offset;

	/* SET VIDEO BUFFER OFFSET IN DISPLAY CONTROLLER */
	/* Use private routine to abstract the display controller. */

	gfx_set_display_video_offset(offset);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_scale
 * 
 * This routine sets the scale factor for the video overlay window.  The 
 * size of the source and destination regions are specified in pixels.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
#else
int gfx_set_video_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
#endif
{
	unsigned long xscale, yscale;

	/* SAVE PARAMETERS */
	/* These are needed for clipping the video window later. */

	gfx_vid_srcw = srcw;
	gfx_vid_srch = srch;
	gfx_vid_dstw = dstw;
	gfx_vid_dsth = dsth;

	/* CALCULATE SC1200 SCALE FACTORS */
	/* No downscaling in SC1200 so force to 1x if attempted. */

	if (srcw < dstw) xscale = (0x2000 * (srcw - 1)) / (dstw - 1);
	else xscale = 0x1FFF;
	if (srch < dsth) yscale = (0x2000 * (srch - 1)) / (dsth - 1);
	else yscale = 0x1FFF;
	WRITE_VID32(SC1200_VIDEO_SCALE, (yscale << 16) | xscale);

	/* CALL ROUTINE TO UPDATE WINDOW POSITION */
	/* This is required because the scale values effect the number of */
	/* source data pixels that need to be clipped, as well as the */
	/* amount of data that needs to be transferred. */

	gfx_set_video_window(gfx_vid_xpos, gfx_vid_ypos, gfx_vid_width,
		gfx_vid_height);
	return(0);
}


/*---------------------------------------------------------------------------
 * gfx_set_video_window
 * 
 * This routine sets the position and size of the video overlay window.  The 
 * position is specified in screen relative coordinates, and may be negative.  
 * The size of destination region is specified in pixels.  The line size
 * indicates the number of bytes of source data per scanline.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_window(short x, short y, unsigned short w, 
	unsigned short h)
#else
int gfx_set_video_window(short x, short y, unsigned short w, 
	unsigned short h)
#endif
{
	unsigned long vcfg = 0;
	unsigned long hadjust, vadjust;
	unsigned long initread;
	unsigned long xstart, ystart, xend, yend;
	unsigned long offset, line_size;

	/* SAVE PARAMETERS */
	/* These are needed to call this routine if the scale value changes. */

	gfx_vid_xpos = x;
	gfx_vid_ypos = y;
	gfx_vid_width = w;
	gfx_vid_height = h;

	/* GET ADJUSTMENT VALUES */
	/* Use routines to abstract version of display controller. */

	hadjust = gfx_get_htotal() - gfx_get_hsync_end() - 13;
	vadjust = gfx_get_vtotal() - gfx_get_vsync_end() + 1;

	if (x > 0) 
	{
		/* NO CLIPPING ON LEFT */

		xstart = x + hadjust;
		initread = 0;
	}
	else 
	{
		/* CLIPPING ON LEFT */
		/* Adjust initial read for scale, checking for divide by zero */

		xstart = hadjust;
		initread = -x;
		if (gfx_vid_dstw) 
			initread = ((-x) * gfx_vid_srcw) / gfx_vid_dstw;
		else initread = 0;
	}
	
	/* CLIPPING ON RIGHT */

	xend = x + w;
	if (xend > gfx_get_hactive()) xend = gfx_get_hactive();
	xend += hadjust;

	/* CLIPPING ON TOP */

	offset = gfx_vid_offset;
	if (y >= 0) 
	{
		ystart = y + vadjust;
	}
	else 
	{
		ystart = vadjust;
		line_size = (READ_VID32(SC1200_VIDEO_CONFIG) >> 7) & 0x000001FE;
		if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_LINE_SIZE_UPPER)
			line_size += 512;
		if (gfx_vid_dsth)
			offset = gfx_vid_offset + (line_size << 1) * 
				(((-y) * gfx_vid_srch) / gfx_vid_dsth);
	}

	/* CLIPPING ON BOTTOM */

	yend = y + h;
	if (yend >= gfx_get_vactive()) yend = gfx_get_vactive();
	yend += vadjust;

	/* SET VIDEO BUFFER OFFSET IN DISPLAY CONTROLLER */
	/* Use private routine to abstract the display controller. */

	gfx_set_display_video_offset(offset);

	/* DISABLE REGISTER UPDATES */

	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	vcfg &= ~SC1200_VCFG_VID_REG_UPDATE;
	WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

	/* SET VIDEO POSITION */

	WRITE_VID32(SC1200_VIDEO_X_POS, (xend << 16) | xstart);
	WRITE_VID32(SC1200_VIDEO_Y_POS, (yend << 16) | ystart);

	/* SET INITIAL READ ADDRESS AND ENABLE REGISTER UPDATES */

	vcfg &= ~SC1200_VCFG_INIT_READ_MASK;
	vcfg |= (initread << 15) & SC1200_VCFG_INIT_READ_MASK;
	vcfg |= SC1200_VCFG_VID_REG_UPDATE;
	WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_color_key
 * 
 * This routine specifies the color key value and mask for the video overlay
 * hardware.  To disable color key, the color and mask should both be set to 
 * zero.  The hardware uses the color key in the following equation:
 *
 * ((source data) & (color key mask)) == ((color key) & (color key mask))
 *
 * The source data can be either graphics data or video data.  The bluescreen
 * parameter is set to have the hardware compare video data and clear to
 * comapare graphics data.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_color_key(unsigned long key, unsigned long mask, 
	int graphics)
#else
int gfx_set_video_color_key(unsigned long key, unsigned long mask, 
	int graphics)
#endif
{
	unsigned long dcfg = 0;

	/* SET SC1200 COLOR KEY VALUE */

	WRITE_VID32(SC1200_VIDEO_COLOR_KEY, key);
	WRITE_VID32(SC1200_VIDEO_COLOR_MASK, mask);

	/* SAVE COLOR KEY VALUE */
	/* Needed to restore values if window disabled then enabled again. */

	sc1200_color_key = key;
	sc1200_color_mask = mask;

	/* SELECT GRAPHICS OR VIDEO DATA TO COMPARE TO THE COLOR KEY */

	dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);
	if (graphics & 0x01) dcfg &= ~SC1200_DCFG_VG_CK;
	else dcfg |= SC1200_DCFG_VG_CK;
	WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_filter
 * 
 * This routine enables or disables the video overlay filters.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_filter(int xfilter, int yfilter)
#else
int gfx_set_video_filter(int xfilter, int yfilter)
#endif
{
	unsigned long vcfg = 0;

	/* ENABLE OR DISABLE SC1200 VIDEO OVERLAY FILTERS */

	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	vcfg &= ~(SC1200_VCFG_X_FILTER_EN | SC1200_VCFG_Y_FILTER_EN);
	if (xfilter) vcfg |= SC1200_VCFG_X_FILTER_EN;
	if (yfilter) vcfg |= SC1200_VCFG_Y_FILTER_EN;
	WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 * 
 * This routine loads the video hardware palette.  If a NULL pointer is 
 * specified, the palette is bypassed (for SC1200, this means loading the 
 * palette with identity values). 
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_video_palette(unsigned long *palette)
#else
int gfx_set_video_palette(unsigned long *palette)
#endif
{
	unsigned long i, entry;

	/* LOAD SC1200 VIDEO PALETTE */

	WRITE_VID32(SC1200_PALETTE_ADDRESS, 0);
	for (i = 0; i < 256; i++)
	{
		if (palette) entry = palette[i];
		else entry = i | (i << 8) | (i << 16);
		WRITE_VID32(SC1200_PALETTE_DATA, entry);
	}
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_enable
 * 
 * This routine enables or disables the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_alpha_enable(int enable)
#else
int gfx_set_alpha_enable(int enable)
#endif
{
	unsigned long address = 0, value = 0;
	if (gfx_alpha_select > 2) return(GFX_STATUS_UNSUPPORTED);
	
	/* SAVE ALPHA ENABLE */
	/* Used to restore state on a video disable/enable combination. */

	sc1200_alpha_enable[gfx_alpha_select] = enable;

	address = SC1200_ALPHA_CONTROL_1 + (gfx_alpha_select << 4);
	value = READ_VID32(address);
	if (enable) value |= SC1200_ACTRL_WIN_ENABLE;
	else value &= ~(SC1200_ACTRL_WIN_ENABLE);
	WRITE_VID32(address, value);
	return(GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_size
 * 
 * This routine sets the size of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_alpha_size(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
#else
int gfx_set_alpha_size(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
#endif
{
	unsigned long address = 0;

	/* ADJUST POSITIONS */

	x += gfx_get_htotal() - gfx_get_hsync_end() - 2;
	y += gfx_get_vtotal() - gfx_get_vsync_end() + 1;

	if (gfx_alpha_select > 2) return(GFX_STATUS_UNSUPPORTED);
	address = SC1200_ALPHA_XPOS_1 + (gfx_alpha_select << 4);
	WRITE_VID32(address, (unsigned long) x | 
		((unsigned long) (x + width) << 16));
	WRITE_VID32(address + 4, (unsigned long) y | 
		((unsigned long) (y + height) << 16));
	return(GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_value
 * 
 * This routine sets the alpha value for the currently selected alpha
 * region.  It also specifies an increment/decrement value for fading.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_alpha_value(unsigned char alpha, char delta)
#else
int gfx_set_alpha_value(unsigned char alpha, char delta)
#endif
{
	unsigned long address = 0, value = 0;
	if (gfx_alpha_select > 2) return(GFX_STATUS_UNSUPPORTED);
	address = SC1200_ALPHA_CONTROL_1 + (gfx_alpha_select << 4);
	value = READ_VID32(address);
	value &= SC1200_ACTRL_WIN_ENABLE; /* keep only enable bit */
	value |= (unsigned long) alpha;
	value |= ((unsigned long) delta) << 8;
	value |= SC1200_ACTRL_LOAD_ALPHA;
	WRITE_VID32(address, value);
	return(GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_priority
 * 
 * This routine sets the priority of the currently selected alpha region.
 * A higher value indicates a higher priority.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_alpha_priority(int priority)
#else
int gfx_set_alpha_priority(int priority)
#endif
{
	unsigned long pos = 0, value = 0;
	if (priority > 3) return(GFX_STATUS_BAD_PARAMETER);
	if (gfx_alpha_select > 2) return(GFX_STATUS_UNSUPPORTED);
	value = READ_VID32(SC1200_VID_ALPHA_CONTROL);
	pos = 16 + (gfx_alpha_select << 1);
	value &= ~(0x03 << pos);
	value |= priority << pos;
	WRITE_VID32(SC1200_VID_ALPHA_CONTROL, value);
	return(GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color
 * 
 * This routine sets the color register for the currently selected alpha 
 * region.  Bit 24 is set to enable the color register.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_set_alpha_color(unsigned long color)
#else
int gfx_set_alpha_color(unsigned long color)
#endif
{
	unsigned long address = 0;
	if (gfx_alpha_select > 2) return(GFX_STATUS_UNSUPPORTED);
	address = SC1200_ALPHA_COLOR_1 + (gfx_alpha_select << 4);
	WRITE_VID32(address, color);
	return(GFX_STATUS_OK);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_sync_polarities
 *
 * This routine returns the polarities of the sync pulses:
 *     Bit 0: Set if negative horizontal polarity.
 *     Bit 1: Set if negative vertical polarity.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_get_sync_polarities(void)
#else
int gfx_get_sync_polarities(void)
#endif
{
	int polarities = 0;
	if (READ_VID32(SC1200_DISPLAY_CONFIG) & 0x00000100) polarities |= 1;
	if (READ_VID32(SC1200_DISPLAY_CONFIG) & 0x00000200) polarities |= 2;
	return(polarities);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_enable
 *
 * This routine returns the value "one" if video overlay is currently enabled,
 * otherwise it returns the value "zero".
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_get_video_enable(void)
#else
int gfx_get_video_enable(void)
#endif
{
	if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_VID_EN) return(1);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_format
 *
 * This routine returns the current video overlay format.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_get_video_format(void)
#else
int gfx_get_video_format(void)
#endif
{
	unsigned long vcfg;
	vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
	if (vcfg & SC1200_VCFG_CSC_BYPASS) return(4);
	else return((vcfg >> 2) & 3);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_src_size
 *
 * This routine returns the size of the source video overlay buffer.  The 
 * return value is (height << 16) | width.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_src_size(void)
#else
unsigned long gfx_get_video_src_size(void)
#endif
{
	unsigned long width = 0, height = 0;

	/* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

	width = (READ_VID32(SC1200_VIDEO_CONFIG) >> 7) & 0x000001FE;
	if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_LINE_SIZE_UPPER)
		width += 512;

	if (width)
	{
		/* DETERMINE HEIGHT BY DIVIDING TOTAL SIZE BY WIDTH */
		/* Get total size from display controller - abtracted. */

		height = gfx_get_display_video_size() / (width << 1);
	}
	return((height << 16) | width);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_line_size
 *
 * This routine returns the line size of the source video overlay buffer, in 
 * pixels.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_line_size(void)
#else
unsigned long gfx_get_video_line_size(void)
#endif
{
	unsigned long width = 0;

	/* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

	width = (READ_VID32(SC1200_VIDEO_CONFIG) >> 7) & 0x000001FE;
	if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_LINE_SIZE_UPPER)
		width += 512;
	return(width);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_xclip
 *
 * This routine returns the number of bytes clipped on the left side of a 
 * video overlay line (skipped at beginning).
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_xclip(void)
#else
unsigned long gfx_get_video_xclip(void)
#endif
{
	unsigned long clip = 0;

	/* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

	clip = (READ_VID32(SC1200_VIDEO_CONFIG) >> 14) & 0x000007FC;
	return(clip);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_offset
 *
 * This routine returns the current offset for the video overlay buffer.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_offset(void)
#else
unsigned long gfx_get_video_offset(void)
#endif
{
	return(gfx_get_display_video_offset());
}

/*---------------------------------------------------------------------------
 * gfx_get_video_scale
 * 
 * This routine returns the scale factor for the video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_scale(void)
#else
unsigned long gfx_get_video_scale(void)
#endif
{
	return(READ_VID32(SC1200_VIDEO_SCALE));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 * 
 * This routine returns the size of the displayed video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_dst_size(void)
#else
unsigned long gfx_get_video_dst_size(void)
#endif
{
	unsigned long xsize, ysize;

	xsize = READ_VID32(SC1200_VIDEO_X_POS);
	xsize = ((xsize >> 16) & 0x3FF) - (xsize & 0x03FF);
	ysize = READ_VID32(SC1200_VIDEO_Y_POS);
	ysize = ((ysize >> 16) & 0x3FF) - (ysize & 0x03FF);
	return((ysize << 16) | xsize);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 * 
 * This routine returns the position of the video overlay window.  The
 * return value is (ypos << 16) | xpos.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_position(void)
#else
unsigned long gfx_get_video_position(void)
#endif
{
	unsigned long hadjust, vadjust;
	unsigned long xpos, ypos;

	/* READ HARDWARE POSITION */

	xpos = READ_VID32(SC1200_VIDEO_X_POS) & 0x000003FF;
	ypos = READ_VID32(SC1200_VIDEO_Y_POS) & 0x000003FF;
	
	/* GET ADJUSTMENT VALUES */
	/* Use routines to abstract version of display controller. */

	hadjust = gfx_get_htotal() - gfx_get_hsync_end() - 14;
	vadjust = gfx_get_vtotal() - gfx_get_vsync_end() + 1;
	xpos -= hadjust;
	ypos -= vadjust;
	return((ypos << 16) | (xpos & 0x0000FFFF));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key
 * 
 * This routine returns the current video color key value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_color_key(void)
#else
unsigned long gfx_get_video_color_key(void)
#endif
{
	return(READ_VID32(SC1200_VIDEO_COLOR_KEY));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 * 
 * This routine returns the current video color mask value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_video_color_key_mask(void)
#else
unsigned long gfx_get_video_color_key_mask(void)
#endif
{
	return(READ_VID32(SC1200_VIDEO_COLOR_MASK));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 * 
 * This routine returns 0 for video data compare, 1 for graphics data.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_get_video_color_key_src(void)
#else
int gfx_get_video_color_key_src(void)
#endif
{
	if (READ_VID32(SC1200_DISPLAY_CONFIG) & SC1200_DCFG_VG_CK) return(0);
	return(1);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_filter
 * 
 * This routine returns if the filters are currently enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int sc1200_get_video_filter(void)
#else
int gfx_get_video_filter(void)
#endif
{
	int retval = 0;
	if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_X_FILTER_EN) 
		retval |= 1;
	if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_Y_FILTER_EN) 
		retval |= 2;
	return(retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_clock_frequency
 *
 * This routine returns the current clock frequency in 16.16 format.
 * It reads the current register value and finds the match in the table.
 * If no match is found, this routine returns 0.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_get_clock_frequency(void)
#else
unsigned long gfx_get_clock_frequency(void)
#endif
{
	int index;
	unsigned long value, mask;
	mask = 0x007FFF0F;
	value = READ_VID32(SC1200_VID_CLOCK_SELECT) & mask;
	for (index = 0; index < NUM_SC1200_FREQUENCIES; index++)
	{
		if ((gfx_sc1200_clock_table[index].clock_select & mask) == value)
			return(gfx_sc1200_clock_table[index].frequency);
	}
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc
 *
 * This routine returns the hardware CRC value, which is used for automated 
 * testing.  The value is like a checksum, but will change if pixels move
 * locations.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long sc1200_read_crc(void)
#else
unsigned long gfx_read_crc(void)
#endif
{
	unsigned long crc = 0xFFFFFFFF;
	if (gfx_test_timing_active())
	{
		// WAIT UNTIL ACTIVE DISPLAY

		while(!gfx_test_vertical_active());

		// RESET CRC DURING ACTIVE DISPLAY

		WRITE_VID32(SC1200_VID_CRC, 0);
		WRITE_VID32(SC1200_VID_CRC, 1);

		// WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE
		
		while(gfx_test_vertical_active());
		while(!gfx_test_vertical_active());
		while(gfx_test_vertical_active());
		while(!gfx_test_vertical_active());
		crc = READ_VID32(SC1200_VID_CRC) >> 8;
	}
	return(crc);
}	

/*---------------------------------------------------------------------------
 * gfx_get_alpha_enable
 * 
 * This routine returns 1 if the selected alpha window is currently 
 * enabled, or 0 if it is currently disabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_get_alpha_enable(int *enable)
#else
void gfx_get_alpha_enable(int *enable)
#endif
{
	unsigned long value = 0;
	*enable = 0;
	if (gfx_alpha_select <= 2)
	{
		value = READ_VID32(SC1200_ALPHA_CONTROL_1 + (gfx_alpha_select << 4));
		if (value & SC1200_ACTRL_WIN_ENABLE) *enable = 1;
	}
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_size
 * 
 * This routine returns the size of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_get_alpha_size(unsigned short *x, unsigned short *y, 
	unsigned short *width, unsigned short *height)
#else
void gfx_get_alpha_size(unsigned short *x, unsigned short *y, 
	unsigned short *width, unsigned short *height)
#endif
{
	unsigned long value = 0;
	*x = 0;
	*y = 0;
	*width = 0;
	*height = 0;
	if (gfx_alpha_select <= 2)
	{
		value = READ_VID32(SC1200_ALPHA_XPOS_1 + (gfx_alpha_select << 4));
		*x = (unsigned short) (value & 0x000007FF);
		*width = (unsigned short) ((value >> 16) & 0x000007FF) - *x;
		value = READ_VID32(SC1200_ALPHA_YPOS_1 + (gfx_alpha_select << 4));
		*y = (unsigned short) (value & 0x000007FF);
		*height = (unsigned short) ((value >> 16) & 0x000007FF) - *y;
	}
	*x -= gfx_get_htotal() - gfx_get_hsync_end() - 2;
	*y -= gfx_get_vtotal() - gfx_get_vsync_end() + 1;
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_value
 * 
 * This routine returns the alpha value and increment/decrement value of 
 * the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_get_alpha_value(unsigned char *alpha, char *delta)
#else
void gfx_get_alpha_value(unsigned char *alpha, char *delta)
#endif
{
	unsigned long value = 0;
	*alpha = 0;
	*delta = 0;
	if (gfx_alpha_select <= 2)
	{
		value = READ_VID32(SC1200_ALPHA_CONTROL_1 + (gfx_alpha_select << 4));
		*alpha = (unsigned char) (value & 0x00FF);
		*delta = (char) ((value >> 8) & 0x00FF);
	}
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_priority
 * 
 * This routine returns the priority of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_get_alpha_priority(int *priority)
#else
void gfx_get_alpha_priority(int *priority)
#endif
{
	unsigned long pos = 0, value = 0;
	*priority = 0;
	if (gfx_alpha_select <= 2)
	{
		value = READ_VID32(SC1200_VID_ALPHA_CONTROL);
		pos = 16 + (gfx_alpha_select << 1);
		*priority = (int) ((value >> pos) & 3);
	}
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_color
 * 
 * This routine returns the color register value for the currently selected 
 * alpha region.  Bit 24 is set if the color register is enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void sc1200_get_alpha_color(unsigned long *color)
#else
void gfx_get_alpha_color(unsigned long *color)
#endif
{
	*color = 0;
	if (gfx_alpha_select <= 2)
	{
		*color = READ_VID32(SC1200_ALPHA_COLOR_1 + (gfx_alpha_select << 4));
	}
	return;
}

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
