/*-----------------------------------------------------------------------------
 * VID_5530.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the CS5530 video overlay hardware.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

#if 0
/* no longer used now that we calc the parameters on the fly (more acurately, too) */
/*----------------------------------------------------------------------------
 * CS5530 PLL TABLE
 *----------------------------------------------------------------------------
 */
typedef struct tagCS5530PLLENTRY
{
	long frequency;					/* 16.16 fixed point frequency */
	unsigned long pll_value;		/* associated register value */
} CS5530PLLENTRY;

CS5530PLLENTRY CS5530_PLLtable[] = {                                 
	{ 0x00192CCC, 0x31C45801, }, /*  25.1750 */
    { 0x001C526E, 0x20E36802, }, /*  28.3220 */
    { 0x001F8000, 0x33915801, }, /*  31.5000 */
    { 0x00240000, 0x31EC4801, }, /*  36.0000 */
    { 0x00258000, 0x21E22801, }, /*  37.5000 */
    { 0x00280000, 0x33088801, }, /*  40.0000 */
    { 0x002CE666, 0x33E22801, }, /*  44.9000 */
    { 0x00318000, 0x336C4801, }, /*  49.5000 */
    { 0x00320000, 0x23088801, }, /*  50.0000 */
    { 0x00325999, 0x23088801, }, /*  50.3500 */
	{ 0x00360000, 0x3708A801, }, /*  54.0000 */
	{ 0x0038643F, 0x23E36802, }, /*  56.3916 */
	{ 0x0038A4DD, 0x23E36802, }, /*  56.6444 */
	{ 0x003B0000, 0x37C45801, }, /*  59.0000 */
    { 0x003F0000, 0x23EC4801, }, /*  63.0000 */
	{ 0x00410000, 0x37911801, }, /*  65.0000 */
	{ 0x00438000, 0x37963803, }, /*  67.5000 */
	{ 0x0046CCCC, 0x37058803, }, /*  70.8000 */
	{ 0x00480000, 0x3710C805, }, /*  72.0000 */
	{ 0x004B0000, 0x37E22801, }, /*  75.0000 */
	{ 0x004EC000, 0x27915801, }, /*  78.7500 */
	{ 0x00500000, 0x37D8D802, }, /*  80.0000 */
	{ 0x0059CCCC, 0x27588802, }, /*  89.8000 */
	{ 0x005E8000, 0x27EC4802, }, /*  94.5000 */
	{ 0x00630000, 0x27AC6803, }, /*  99.0000 */
	{ 0x00640000, 0x27088801, }, /* 100.0000 */
	{ 0x006C0000, 0x2710C805, }, /* 108.0000 */
	{ 0x00708000, 0x27E36802, }, /* 112.5000 */
	{ 0x00820000, 0x27C58803, }, /* 130.0000 */
	{ 0x00870000, 0x27316803, }, /* 135.0000 */
    { 0x009D8000, 0x2F915801, }, /* 157.5000 */
    { 0x00A20000, 0x2F08A801, }, /* 162.0000 */
	{ 0x00AF0000, 0x2FB11802, }, /* 175.0000 */
	{ 0x00BD0000, 0x2FEC4802, }, /* 189.0000 */
	{ 0x00CA0000, 0x2F963803, }, /* 202.0000 */
	{ 0x00E80000, 0x2FB1B802, }, /* 232.0000 */
};

#define NUM_CS5530_FREQUENCIES sizeof(CS5530_PLLtable)/sizeof(CS5530PLLENTRY)
#endif

static uint8 PD_TABLE[] = {
	0x00, //  0 - UNUSED
	0x0F, //  1
	0x07, //  2
	0x17, //  3
	0x03, //  4
	0x13, //  5
	0x01, //  6
	0x11, //  7
	0x00, //  8
	0x10, //  9
	0x08, // 10
	0x18, // 11
	0x04, // 12
	0x14, // 13
	0x0A, // 14
	0x1A, // 15
	0x05, // 16
	0x15, // 17
	0x02, // 18
	0x12, // 19
	0x09, // 20
	0x19, // 21
	0x0C, // 22
	0x1C, // 23
	0x06, // 24
	0x16, // 25
	0x0B, // 26
	0x1B, // 27
	0x0D, // 28
	0x1D, // 29
	0x0E, // 30
	0x1E  // 31
};

static uint16 N_TABLE[] = {
	0x0000, // NOT USED
	// 1 - 10
	0x3FF, 0x5FF, 0x2FF, 0x57F, 0x2BF, 0x55F, 0x2AF, 0x557, 0x2AB, 0x555,
	// 11 - 20, etc.
	0x2AA, 0x155, 0xAA,  0x455, 0x22A, 0x115, 0x8A,  0x445, 0x222, 0x111,
	0x88,  0x444, 0x622, 0x311, 0x588, 0x6C4, 0x362, 0x1B1, 0xD8,  0x46C,
	0x636, 0x31B, 0x58D, 0x2C6, 0x163, 0xB1,  0x58,  0x42C, 0x616, 0x30B,
	0x585, 0x2C2, 0x161, 0xB0,  0x458, 0x62C, 0x316, 0x18B, 0xC5,  0x62,
	0x431, 0x218, 0x10C, 0x486, 0x643, 0x721, 0x790, 0x3C8, 0x1E4, 0x4F2,
	0x679, 0x73C, 0x39E, 0x1CF, 0xE7,  0x73,  0x39,  0x1C,  0x40E, 0x607,
	0x703, 0x781, 0x7C0, 0x3E0, 0x1F0, 0x4F8, 0x67C, 0x33E, 0x19F, 0xCF,
	0x67,  0x33,  0x19,  0xC,   0x406, 0x603, 0x701, 0x780, 0x3C0, 0x1E0,
	0x4F0, 0x678, 0x33C, 0x19E, 0x4CF, 0x267, 0x533, 0x299, 0x54C, 0x6A6,
	0x353, 0x5A9, 0x2D4, 0x16A, 0x4B5, 0x25A, 0x12D, 0x96,  0x44B, 0x225,
	0x512, 0x689, 0x744, 0x3A2, 0x1D1, 0xE8,  0x474, 0x63A, 0x31D, 0x58E,
	0x6C7, 0x763, 0x7B1, 0x7D8, 0x3EC, 0x1F6, 0x4FB, 0x27D, 0x53E, 0x69F,
	0x74F, 0x7A7, 0x7D3, 0x7E9, 0x7F4, 0x3FA, 0x1FD, 0xFE,  0x47F, 0x23F,
	0x51F, 0x28F, 0x547, 0x2A3, 0x551, 0x2A8, 0x154, 0x4AA, 0x655, 0x72A,
	0x395, 0x5CA, 0x6E5, 0x772, 0x3B9, 0x5DC, 0x6EE, 0x377, 0x5BB, 0x2DD,
	0x56E, 0x6B7, 0x75B, 0x7AD, 0x7D6, 0x3EB, 0x5F5, 0x2FA, 0x17D, 0xBE,
	0x45F, 0x22F, 0x517, 0x28B, 0x545, 0x2A2, 0x151, 0xA8,  0x454, 0x62A,
	0x315, 0x58A, 0x6C5, 0x762, 0x3B1, 0x5D8, 0x6EC, 0x376, 0x1BB, 0xDD,
	0x6E,  0x437, 0x21B, 0x50D, 0x286, 0x143, 0xA1,  0x50,  0x428, 0x614,
	0x30A, 0x185, 0xC2,  0x461, 0x230, 0x118, 0x48C, 0x646, 0x323, 0x591,
	0x2C8, 0x164, 0x4B2, 0x659, 0x72C, 0x396, 0x1CB, 0xE5,  0x72,  0x439,
	0x21C, 0x10E, 0x487, 0x243, 0x521, 0x290, 0x148, 0x4A4, 0x652, 0x329,
	0x594, 0x6CA, 0x365, 0x5B2, 0x6D9, 0x76C, 0x3B6, 0x1DB, 0xED,  0x76,
	0x43B, 0x21D, 0x50E, 0x687, 0x743, 0x7A1, 0x7D0, 0x3E8, 0x1F4, 0x4FA,
	0x67D, 0x73E, 0x39F, 0x5CF, 0x2E7, 0x573, 0x2B9, 0x55C, 0x6AE, 0x357,
	0x5AB, 0x2D5, 0x56A, 0x6B5, 0x75A, 0x3AD, 0x5D6, 0x6EB, 0x775, 0x7BA,
	0x3DD, 0x5EE, 0x6F7, 0x77B, 0x7BD, 0x7DE, 0x3EF, 0x5F7, 0x2FB, 0x57D,
	0x2BE, 0x15F, 0xAF,  0x57,  0x2B,  0x15,  0xA,   0x405, 0x202, 0x101,
	0x80,  0x440, 0x620, 0x310, 0x188, 0x4C4, 0x662, 0x331, 0x598, 0x6CC,
	0x366, 0x1B3, 0xD9,  0x6C,  0x436, 0x61B, 0x70D, 0x786, 0x3C3, 0x5E1,
	0x2F0, 0x178, 0x4BC, 0x65E, 0x32F, 0x597, 0x2CB, 0x565, 0x2B2, 0x159,
	0xAC,  0x456, 0x62B, 0x715, 0x78A, 0x3C5, 0x5E2, 0x6F1, 0x778, 0x3BC,
	0x1DE, 0x4EF, 0x277, 0x53B, 0x29D, 0x54E, 0x6A7, 0x753, 0x7A9, 0x7D4,
	0x3EA, 0x1F5, 0xFA,  0x47D, 0x23E, 0x11F, 0x8F,  0x47,  0x23,  0x11,
	0x8,   0x404, 0x602, 0x301, 0x580, 0x6C0, 0x360, 0x1B0, 0x4D8, 0x66C,
	0x336, 0x19B, 0xCD,  0x66,  0x433, 0x219, 0x50C, 0x686, 0x343, 0x5A1,
	0x2D0, 0x168, 0x4B4, 0x65A, 0x32D, 0x596, 0x6CB, 0x765, 0x7B2, 0x3D9,
	0x5EC, 0x6F6, 0x37B, 0x5BD, 0x2DE, 0x16F, 0xB7,  0x5B,  0x2D,  0x16,
	0x40B, 0x205, 0x502, 0x681, 0x740, 0x3A0, 0x1D0, 0x4E8, 0x674, 0x33A
};
/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void cs5530_reset_video(void)
#else
void gfx_reset_video(void)
#endif
{
	gfx_set_video_enable(0);
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
void cs5530_set_clock_frequency(unsigned long frequency)
#else
void gfx_set_clock_frequency(unsigned long frequency)
#endif
{
	unsigned long value;

	/* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
#if 0
	/* Search the table for the closest frequency (16.16 format). */

	value = CS5530_PLLtable[0].pll_value;
	min = (long) CS5530_PLLtable[0].frequency - frequency;
	if (min < 0L) min = -min;
	for (index = 1; index < NUM_CS5530_FREQUENCIES; index++)
	{
		diff = (long) CS5530_PLLtable[index].frequency - frequency;
		if (diff < 0L) diff = -diff;
		if (diff < min)
		{
			min = diff; 
			value = CS5530_PLLtable[index].pll_value;
		}
	}
	xprintf(("value: %.8lx\ndelta: %ld (%.8lx)\n", value, diff, diff));
#else
	const int32 MHZ_14_318 = 0x000e515a ; // (14.318 * 65535) / 1000;
	const int32 MHZ_140 = 0x008c0000; // 140.0 MHz
	const int32 MHZ_300 = 0x012c0000; // 300.0 MHz
	int32 PD_min, PD_max, PD;
	int32 FD_min, FD_max;
	int32 best_PD = 0, best_FD = 0, best_ID = 0;
	int32 delta;
	int32 best_delta = frequency;
#if LOW_FREQ_SUPPORT
	int32 PD_multiplier = 1;
#endif

	xprintf(("target freq: %.8lx (%.3f)\n", frequency, frequency / 65536.0));
#if LOW_FREQ_SUPPORT
	if (frequency < MHZ_14_318)
	{
		uint32 orig_freq = frequency;
		while (frequency < MHZ_14_318)
		{
			frequency += orig_freq;
			PD_multiplier++;
		}
	}
#endif
	PD_min = 1;
	while ((PD_min * (int32)frequency) < MHZ_140)
		PD_min++;
	PD_max = PD_min + 1;
	while ((PD_max * (int32)frequency) < MHZ_300)
		PD_max++;
	FD_min = (int32)frequency / MHZ_14_318;
	FD_max = FD_min * (PD_max - 1) * 9;
	xprintf(("FD min: %ld, max: %ld\n", FD_min, FD_max));
	xprintf(("PD min: %ld, max: %ld\n", PD_min, PD_max));
	for (PD = PD_min; PD < PD_max; PD++)
	{
		int32 FD;
		for (FD = FD_min; FD < FD_max; FD++)
		{
			int32 ID;
			for (ID = 2; ID < 10; ID++)
			{
				delta = frequency - ((MHZ_14_318 * FD) / (PD * ID));
				if (delta < 0) delta = -delta;
				if (delta < best_delta)
				{
					best_PD = PD;
					best_ID = ID;
					best_FD = FD;
					best_delta = delta;
				}
			}
		}
	}
#if LOW_FREQ_SUPPORT
	// for low freq. cases
	best_PD *= PD_multiplier;
#endif
	xprintf(("best_delta: %ld (%.8lx)\n", best_delta, best_delta));
	xprintf(("FD, PD, ID: %ld, %ld, %ld\n", best_FD, best_PD, best_ID));
	value  = (best_ID - 2) & 0x07;
	value |= (best_FD & 0x01) << 23;
	value |= N_TABLE[best_FD >> 1] << 12;
	value |= PD_TABLE[best_PD] << 24;
	value |= (best_PD & 0x1) << 30; // half clock on odd post dividers
	//value |= 0x20000800; // magic bits?
	value |= 0x00000800; // CLK_ON
	xprintf(("value: %.8lx\n", value));
	xprintf(("freq: %.8lx (%.3f)\n", ((MHZ_14_318 * best_FD) / (best_PD * best_ID)), ((MHZ_14_318 * best_FD) / (best_PD * best_ID)) / 65536.0));
#endif

	/* SET THE DOT CLOCK REGISTER */

	DWRITE_VID32(CS5530_DOT_CLK_CONFIG, value);
	DWRITE_VID32(CS5530_DOT_CLK_CONFIG, value |= 0x80000100); /* set reset/bypass */
	gfx_delay_milliseconds(1); /* wait for PLL to settle */
	DWRITE_VID32(CS5530_DOT_CLK_CONFIG, value & 0x7FFFFFFF); /* clear reset */
	DWRITE_VID32(CS5530_DOT_CLK_CONFIG, value & 0x7FFFFEFF); /* clear bypass */
	// wait for clock to settle
#if 0
	while (!(READ_VID32(CS5530_DOT_CLK_CONFIG) & (1 << 5)))
		/* NOTHING */;
#else
	gfx_delay_milliseconds(1); /* wait for PLL to settle */
#endif
	return;
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_enable
 *
 * This routine enables or disables the video overlay functionality.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_set_video_enable(int enable)
#else
int gfx_set_video_enable(int enable)
#endif
{
	unsigned long vcfg;

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

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	if (enable)
	{
		/* ENABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
		/* Use private routine to abstract the display controller. */

		gfx_set_display_video_enable(1);
		
		/* SET CS5530 BUS CONTROL PARAMETERS */
		/* Currently always high speed, 8-bit interface. */

		vcfg |= CS5530_VCFG_HIGH_SPD_INT;
		vcfg &= ~(CS5530_VCFG_EARLY_VID_RDY | CS5530_VCFG_16_BIT_EN);

		/* ENABLE CS5530 VIDEO OVERLAY */

		vcfg |= CS5530_VCFG_VID_EN;
		DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);
	}
	else
	{
		/* DISABLE CS5530 VIDEO OVERLAY */

		vcfg &= ~CS5530_VCFG_VID_EN;
		DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);

		/* DISABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
		/* Use private routine to abstract the display controller. */

		gfx_set_display_video_enable(0);
	}
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_format
 *
 * Currently only sets 4:2:0 format, Y1 V Y0 U.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_set_video_format(unsigned long format)
#else
int gfx_set_video_format(unsigned long format)
#endif
{
	unsigned long vcfg = 0;

	/* SET THE CS5530 VIDEO INPUT FORMAT */

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	vcfg &= ~(CS5530_VCFG_VID_INP_FORMAT | CS5530_VCFG_4_2_0_MODE);
	vcfg &= ~(CS5530_VCFG_CSC_BYPASS);
	if (format < 4) vcfg |= (format << 2);
	else vcfg |= CS5530_VCFG_CSC_BYPASS;
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);
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
int cs5530_set_video_size(unsigned short width, unsigned short height)
#else
int gfx_set_video_size(unsigned short width, unsigned short height)
#endif
{
	unsigned long size, vcfg;

	/* SET THE CS5530 VIDEO LINE SIZE */

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	vcfg &= ~(CS5530_VCFG_LINE_SIZE_LOWER_MASK | CS5530_VCFG_LINE_SIZE_UPPER);
	size = (width >> 1);
	vcfg |= (size & 0x00FF) << 8;
	if (size & 0x0100) vcfg |= CS5530_VCFG_LINE_SIZE_UPPER;
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);

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
int cs5530_set_video_offset(unsigned long offset)
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
int cs5530_set_video_scale(unsigned short srcw, unsigned short srch, 
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

	/* CALCULATE CS5530 SCALE FACTORS */
	/* No downscaling in CS5530 so force to 1x if attempted. */

	if (srcw < dstw) xscale = (0x2000 * (srcw - 1)) / (dstw - 1);
	else xscale = 0x1FFF;
	if (srch < dsth) yscale = (0x2000 * (srch - 1)) / (dsth - 1);
	else yscale = 0x1FFF;
	DWRITE_VID32(CS5530_VIDEO_SCALE, (yscale << 16) | xscale);

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
int cs5530_set_video_window(short x, short y, unsigned short w, 
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
		line_size = (READ_VID32(CS5530_VIDEO_CONFIG) >> 7) & 0x000001FE;
		if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_LINE_SIZE_UPPER)
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

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	vcfg &= ~CS5530_VCFG_VID_REG_UPDATE;
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);

	/* SET VIDEO POSITION */

	DWRITE_VID32(CS5530_VIDEO_X_POS, (xend << 16) | xstart);
	DWRITE_VID32(CS5530_VIDEO_Y_POS, (yend << 16) | ystart);

	/* SET INITIAL READ ADDRESS AND ENABLE REGISTER UPDATES */

	vcfg &= ~CS5530_VCFG_INIT_READ_MASK;
	vcfg |= (initread << 15) & CS5530_VCFG_INIT_READ_MASK;
	vcfg |= CS5530_VCFG_VID_REG_UPDATE;
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);
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
int cs5530_set_video_color_key(unsigned long key, unsigned long mask, 
	int graphics)
#else
int gfx_set_video_color_key(unsigned long key, unsigned long mask, 
	int graphics)
#endif
{
	unsigned long dcfg = 0;

	/* SET CS5530 COLOR KEY VALUE */

	DWRITE_VID32(CS5530_VIDEO_COLOR_KEY, key);
	DWRITE_VID32(CS5530_VIDEO_COLOR_MASK, mask);

	/* SELECT GRAPHICS OR VIDEO DATA TO COMPARE TO THE COLOR KEY */

	dcfg = READ_VID32(CS5530_DISPLAY_CONFIG);
	if (graphics & 0x01) dcfg &= ~CS5530_DCFG_VG_CK;
	else dcfg |= CS5530_DCFG_VG_CK;
	DWRITE_VID32(CS5530_DISPLAY_CONFIG, dcfg);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_filter
 * 
 * This routine enables or disables the video overlay filters.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_set_video_filter(int xfilter, int yfilter)
#else
int gfx_set_video_filter(int xfilter, int yfilter)
#endif
{
	unsigned long vcfg = 0;

	/* ENABLE OR DISABLE CS5530 VIDEO OVERLAY FILTERS */

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	vcfg &= ~(CS5530_VCFG_X_FILTER_EN | CS5530_VCFG_Y_FILTER_EN);
	if (xfilter) vcfg |= CS5530_VCFG_X_FILTER_EN;
	if (yfilter) vcfg |= CS5530_VCFG_Y_FILTER_EN;
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 * 
 * This routine loads the video hardware palette.  If a NULL pointer is 
 * specified, the palette is bypassed (for CS5530, this means loading the 
 * palette with identity values). 
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_set_video_palette(unsigned long *palette)
#else
int gfx_set_video_palette(unsigned long *palette)
#endif
{
	unsigned long i, entry;

	/* LOAD CS5530 VIDEO PALETTE */

	DWRITE_VID32(CS5530_PALETTE_ADDRESS, 0);
	for (i = 0; i < 256; i++)
	{
		if (palette) entry = palette[i];
		else entry = i | (i << 8) | (i << 16);
		DWRITE_VID32(CS5530_PALETTE_DATA, entry);
	}
	return(0);
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
int cs5530_get_sync_polarities(void)
#else
int gfx_get_sync_polarities(void)
#endif
{
	int polarities = 0;
	if (READ_VID32(CS5530_DISPLAY_CONFIG) & 0x00000100) polarities |= 1;
	if (READ_VID32(CS5530_DISPLAY_CONFIG) & 0x00000200) polarities |= 2;
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
int cs5530_get_video_enable(void)
#else
int gfx_get_video_enable(void)
#endif
{
	if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_VID_EN) return(1);
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_format
 *
 * This routine returns the current video overlay format.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_get_video_format(void)
#else
int gfx_get_video_format(void)
#endif
{
	unsigned long vcfg;
	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	if (vcfg & CS5530_VCFG_CSC_BYPASS) return(4);
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
unsigned long cs5530_get_video_src_size(void)
#else
unsigned long gfx_get_video_src_size(void)
#endif
{
	unsigned long width = 0, height = 0;

	/* DETERMINE SOURCE WIDTH FROM THE CS5530 VIDEO LINE SIZE */

	width = (READ_VID32(CS5530_VIDEO_CONFIG) >> 7) & 0x000001FE;
	if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_LINE_SIZE_UPPER)
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
unsigned long cs5530_get_video_line_size(void)
#else
unsigned long gfx_get_video_line_size(void)
#endif
{
	unsigned long width = 0;

	/* DETERMINE SOURCE WIDTH FROM THE CS5530 VIDEO LINE SIZE */

	width = (READ_VID32(CS5530_VIDEO_CONFIG) >> 7) & 0x000001FE;
	if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_LINE_SIZE_UPPER)
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
unsigned long cs5530_get_video_xclip(void)
#else
unsigned long gfx_get_video_xclip(void)
#endif
{
	unsigned long clip = 0;

	/* DETERMINE SOURCE WIDTH FROM THE CS5530 VIDEO LINE SIZE */

	clip = (READ_VID32(CS5530_VIDEO_CONFIG) >> 14) & 0x000007FC;
	return(clip);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_offset
 *
 * This routine returns the current offset for the video overlay buffer.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long cs5530_get_video_offset(void)
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
unsigned long cs5530_get_video_scale(void)
#else
unsigned long gfx_get_video_scale(void)
#endif
{
	return(READ_VID32(CS5530_VIDEO_SCALE));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 * 
 * This routine returns the size of the displayed video overlay window.
 * NOTE: This is the displayed window size, which may be different from 
 * the real window size if clipped.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long cs5530_get_video_dst_size(void)
#else
unsigned long gfx_get_video_dst_size(void)
#endif
{
	unsigned long xsize, ysize;

	xsize = READ_VID32(CS5530_VIDEO_X_POS);
	xsize = ((xsize >> 16) & 0x3FF) - (xsize & 0x03FF);
	ysize = READ_VID32(CS5530_VIDEO_Y_POS);
	ysize = ((ysize >> 16) & 0x3FF) - (ysize & 0x03FF);
	return((ysize << 16) | xsize);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 * 
 * This routine returns the position of the video overlay window.  The
 * return value is (ypos << 16) | xpos.
 * NOTE: This is the displayed window position, which may be different from 
 * the real window position if clipped.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long cs5530_get_video_position(void)
#else
unsigned long gfx_get_video_position(void)
#endif
{
	unsigned long hadjust, vadjust;
	unsigned long xpos, ypos;

	/* READ HARDWARE POSITION */

	xpos = READ_VID32(CS5530_VIDEO_X_POS) & 0x000003FF;
	ypos = READ_VID32(CS5530_VIDEO_Y_POS) & 0x000003FF;
	
	/* GET ADJUSTMENT VALUES */
	/* Use routines to abstract version of display controller. */

	hadjust = gfx_get_htotal() - gfx_get_hsync_end() - 13;
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
unsigned long cs5530_get_video_color_key(void)
#else
unsigned long gfx_get_video_color_key(void)
#endif
{
	return(READ_VID32(CS5530_VIDEO_COLOR_KEY));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 * 
 * This routine returns the current video color mask value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long cs5530_get_video_color_key_mask(void)
#else
unsigned long gfx_get_video_color_key_mask(void)
#endif
{
	return(READ_VID32(CS5530_VIDEO_COLOR_MASK));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 * 
 * This routine returns 0 for video data compare, 1 for graphics data.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_get_video_color_key_src(void)
#else
int gfx_get_video_color_key_src(void)
#endif
{
	if (READ_VID32(CS5530_DISPLAY_CONFIG) & CS5530_DCFG_VG_CK) return(0);
	return(1);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_filter
 * 
 * This routine returns if the filters are currently enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int cs5530_get_video_filter(void)
#else
int gfx_get_video_filter(void)
#endif
{
	int retval = 0;
	if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_X_FILTER_EN) 
		retval |= 1;
	if (READ_VID32(CS5530_VIDEO_CONFIG) & CS5530_VCFG_Y_FILTER_EN) 
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
unsigned long cs5530_get_clock_frequency(void)
#else
unsigned long gfx_get_clock_frequency(void)
#endif
{
#if 0
	int index;
	unsigned long value, mask;
	mask = 0x7FFFFEDC;
	value = READ_VID32(CS5530_DOT_CLK_CONFIG) & mask;
	for (index = 0; index < NUM_CS5530_FREQUENCIES; index++)
	{
		if ((CS5530_PLLtable[index].pll_value & mask) == value)
			return(CS5530_PLLtable[index].frequency);
	}
	return(0);
#else
	/* back calculate the value from the various bits */
	const uint32 MHZ_14_318 = 0x000e515a ; // (14.318 * 65535) / 1000;
	uint32 value = READ_VID32(CS5530_DOT_CLK_CONFIG);
	uint32 ID = (value & 0x07) + 2;
	uint32 PD = (value >> 24) & 0x1f;
	uint32 plus_one = (value >> 23) & 0x01;
	uint32 FD = (value >> 12) & 0x7ff;
	uint32 index;

	xprintf(("value: 0x%08lx\n", value));
	/* hunt down FD in N_TABLE */
	for (index = 0; index < (sizeof(N_TABLE) / sizeof(N_TABLE[0])); index++)
	{
		if (N_TABLE[index] == FD) break;
	}
	/* bail out if we didn't find a value */
	if (index == (sizeof(N_TABLE) / sizeof(N_TABLE[0])))
	{
		xprintf(("Failed to find N in N_TABLE[]\n"));
		return 0;
	}
	FD = 2 * index + plus_one;
	xprintf(("FD: %lu\n", FD));
	/* hunt down PD in PD_TABLE */
	for (index = 0; index < (sizeof(PD_TABLE) / sizeof(PD_TABLE[0])); index++)
	{
		if (PD_TABLE[index] == PD) break;
	}
	/* bail out if we didn't find a value */
	if (index == (sizeof(PD_TABLE) / sizeof(PD_TABLE[0])))
	{
		xprintf(("Failed to find PD in PD_TABLE[]\n"));
		return 0;
	}
	PD = index;
	xprintf(("PD: %lu\n", PD));
	xprintf(("freq: 0x%08lx\n", ((MHZ_14_318 * FD) / (PD * ID))));
	
	return ((MHZ_14_318 * FD) / (PD * ID));
#endif
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
unsigned long cs5530_read_crc(void)
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

		DWRITE_VID32(CS5530_CRCSIG_TFT_TV, 0);
		DWRITE_VID32(CS5530_CRCSIG_TFT_TV, 1);

		// WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE
		
		while(gfx_test_vertical_active());
		while(!gfx_test_vertical_active());
		while(gfx_test_vertical_active());
		while(!gfx_test_vertical_active());
		crc = READ_VID32(CS5530_CRCSIG_TFT_TV) >> 8;
	}
	return(crc);
}	

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
