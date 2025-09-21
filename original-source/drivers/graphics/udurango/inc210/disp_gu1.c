/*-----------------------------------------------------------------------------
 * DISP_GU1.C
 *
 * Version 2.1 - March 3, 2000
 *
 * This file contains routines for the first generation display controller.  
 *
 * History:
 *    Versions 0.1 through 2.1 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

void gu1_enable_compression(void);		/* private routine definition */
void gu1_disable_compression(void);		/* private routine definition */
void gfx_reset_video(void);				/* private routine definition */

/*-----------------------------------------------------------------------------
 * WARNING!!!! INACCURATE DELAY MECHANISM
 *
 * In an effort to keep the code self contained and operating system 
 * independent, the delay loop just performs reads of a display controller
 * register.  This time will vary for faster processors.  The delay can always
 * be longer than intended, only effecting the time of the mode switch 
 * (obviously want it to still be under a second).  Problems with the hardware
 * only arise if the delay is not long enough.  
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_delay_milliseconds(unsigned long milliseconds)
#else
void gfx_delay_milliseconds(unsigned long milliseconds)
#endif
{
#if 0
	/* ASSUME 300 MHz, 5 CLOCKS PER READ */

	#define READS_PER_MILLISECOND 60000L

	unsigned long loop;
	loop = milliseconds * READS_PER_MILLISECOND;
	while (loop-- > 0)
	{
		READ_REG32(DC_UNLOCK);
	}
#else
	snooze(milliseconds * 1000);
	xprintf(("snooze(%ld);\n", milliseconds * 1000));
#endif
}

/*-----------------------------------------------------------------------------
 * GFX_VIDEO_SHUTDOWN
 *
 * This routine disables the display controller output.
 *-----------------------------------------------------------------------------
 */
void gu1_video_shutdown(void)
{
	unsigned long unlock;
	unsigned long gcfg, tcfg;

	/* DISABLE COMPRESSION */

	gu1_disable_compression();

	/* ALSO DISABLE VIDEO */
	/* Use private "reset video" routine to do all that is needed. */
	/* SC1200, for example, also disables the alpha blending regions. */

	gfx_reset_video();

	/* UNLOCK THE DISPLAY CONTROLLER REGISTERS */

	unlock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

	/* READ THE CURRENT GX VALUES */

    gcfg = READ_REG32(DC_GENERAL_CFG);
    tcfg = READ_REG32(DC_TIMING_CFG);

    /* BLANK THE GX DISPLAY */

    tcfg &= ~(unsigned long)DC_TCFG_BLKE;
    DWRITE_REG32(DC_TIMING_CFG, tcfg);
	gfx_delay_milliseconds(1); /* delay after TIMING_CFG */

    /* DISABLE THE TIMING GENERATOR */

    tcfg &= ~(unsigned long)DC_TCFG_TGEN;						  
    DWRITE_REG32(DC_TIMING_CFG, tcfg);
	gfx_delay_milliseconds(1); /* delay after TIMING_CFG */ 

	/* DELAY: WAIT FOR PENDING MEMORY REQUESTS */ 
	/* This delay is used to make sure that all pending requests to the */ 
	/* memory controller have completed before disabling the FIFO load. */

	gfx_delay_milliseconds(10);

    /* DISABLE DISPLAY FIFO LOAD AND DISABLE COMPRESSION */

    gcfg &= ~(unsigned long)(DC_GCFG_DFLE | DC_GCFG_CMPE | DC_GCFG_DECE);
    DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	DWRITE_REG32(DC_UNLOCK, unlock);
	return;
}

/*-----------------------------------------------------------------------------
 * GFX_SET_SPECIFIED_MODE
 * This routine uses the parameters in the specified display mode structure
 * to program the display controller hardware.  
 *-----------------------------------------------------------------------------
 */
void gu1_set_specified_mode(DISPLAYMODE *pMode, int bpp)
{
	unsigned long unlock, value;
	unsigned long gcfg, tcfg, ocfg, dcfg, vcfg;
	unsigned long size, pitch;

	xprintf(("gu1_set_specified_mode() begins\n"));
	/* DISABLE COMPRESSION */

	gu1_disable_compression();

	/* ALSO DISABLE VIDEO */
	/* Use private "reset video" routine to do all that is needed. */
	/* SC1200, for example, also disables the alpha blending regions. */

	gfx_reset_video();

	/* UNLOCK THE DISPLAY CONTROLLER REGISTERS */

	unlock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

	/* READ THE CURRENT GX VALUES */

    gcfg = READ_REG32(DC_GENERAL_CFG);
    tcfg = READ_REG32(DC_TIMING_CFG);

    /* BLANK THE GX DISPLAY */

    tcfg &= ~(unsigned long)DC_TCFG_BLKE;
    DWRITE_REG32(DC_TIMING_CFG, tcfg);
	gfx_delay_milliseconds(1); /* delay after TIMING_CFG */

    /* DISABLE THE TIMING GENERATOR */

    tcfg &= ~(unsigned long)DC_TCFG_TGEN;						  
    DWRITE_REG32(DC_TIMING_CFG, tcfg);
	gfx_delay_milliseconds(1); /* delay after TIMING_CFG */ 

	/* DELAY: WAIT FOR PENDING MEMORY REQUESTS 
	 * This delay is used to make sure that all pending requests to the 
	 * memory controller have completed before disabling the FIFO load.
     */

	gfx_delay_milliseconds(10);

    /* DISABLE DISPLAY FIFO LOAD AND DISABLE COMPRESSION */

    gcfg &= ~(unsigned long)(DC_GCFG_DFLE | DC_GCFG_CMPE | DC_GCFG_DECE);
    DWRITE_REG32(DC_GENERAL_CFG, gcfg);

	/* CLEAR THE "DCLK_MUL" FIELD */ 

	gcfg &= ~(unsigned long)(DC_GCFG_DDCK | DC_GCFG_DPCK | DC_GCFG_DFCK);
	gcfg &= ~(unsigned long)DC_GCFG_DCLK_MASK;
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);

	/* SET THE DOT CLOCK FREQUENCY */
	/* Mask off the divide by two bit (bit 31) */

	gfx_set_clock_frequency(pMode->frequency & 0x7FFFFFFF);

	/* DELAY: WAIT FOR THE PLL TO SETTLE */
	/* This allows the dot clock frequency that was just set to settle. */

	gfx_delay_milliseconds(10);

	/* SET THE "DCLK_MUL" FIELD OF DC_GENERAL_CFG */
	/* The GX hardware divides the dot clock, so 2x really means that the */ 
	/* internal dot clock equals the external dot clock. */ 

	if (pMode->frequency & 0x80000000) gcfg |= 0x0040;
	else gcfg |= 0x0080;
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	
	/* DELAY: WAIT FOR THE ADL TO LOCK */
	/* This allows the clock generatation within GX to settle.  This is */
	/* needed since some of the register writes that follow require that */
	/* clock to be present. */

	gfx_delay_milliseconds(10);

	/* SET THE GX DISPLAY CONTROLLER PARAMETERS */

	DWRITE_REG32(DC_FB_ST_OFFSET, 0);
	DWRITE_REG32(DC_CB_ST_OFFSET, 0);
	DWRITE_REG32(DC_CURS_ST_OFFSET, 0);

	/* SET LINE SIZE AND PITCH */

	size = pMode->hactive;
	if (bpp > 8) size <<= 1;
	if (size <= 1024) pitch = 1024;
	else pitch = 2048;
	DWRITE_REG32(DC_LINE_DELTA, pitch >> 2);

	/* ADD 2 TO SIZE FOR POSSIBLE START ADDRESS ALIGNMENTS */

	DWRITE_REG32(DC_BUF_SIZE, (size >> 3) + 2);

	/* COMBINE AND SET TIMING VALUES */

	value = (unsigned long) (pMode->hactive - 1) |
		(((unsigned long) (pMode->htotal - 1)) << 16);
	DWRITE_REG32(DC_H_TIMING_1, value);
	value = (unsigned long) (pMode->hblankstart - 1) |
		(((unsigned long) (pMode->hblankend - 1)) << 16);
	DWRITE_REG32(DC_H_TIMING_2, value);
	value = (unsigned long) (pMode->hsyncstart - 1) |
		(((unsigned long) (pMode->hsyncend - 1)) << 16);
	DWRITE_REG32(DC_H_TIMING_3, value);
	DWRITE_REG32(DC_FP_H_TIMING, value);
	value = (unsigned long) (pMode->vactive - 1) |
		(((unsigned long) (pMode->vtotal - 1)) << 16);
	DWRITE_REG32(DC_V_TIMING_1, value);
	value = (unsigned long) (pMode->vblankstart - 1) |
		(((unsigned long) (pMode->vblankend - 1)) << 16);
	DWRITE_REG32(DC_V_TIMING_2, value);
	value = (unsigned long) (pMode->vsyncstart - 1) |
		(((unsigned long) (pMode->vsyncend - 1)) << 16);
	DWRITE_REG32(DC_V_TIMING_3, value);
	DWRITE_REG32(DC_FP_V_TIMING, value);

	/* ALWAYS ENABLE "PANEL" DATA FROM MEDIAGX */
    /* That is really just the 18 BPP data bus to the companion chip */

	ocfg = DC_OCFG_PCKE | DC_OCFG_PDEL | DC_OCFG_PDEH;

	/* SET PIXEL FORMAT */

	if (bpp == 8) ocfg |= DC_OCFG_8BPP;
	else if (bpp == 15) ocfg |= DC_OCFG_555;

	/* ENABLE TIMING GENERATOR, SYNCS, AND FP DATA */

	tcfg = DC_TCFG_FPPE | DC_TCFG_HSYE | DC_TCFG_VSYE | DC_TCFG_BLKE |
		DC_TCFG_TGEN;
	if (pMode->flags & GFX_MODE_INTERLACED) tcfg |= DC_TCFG_INTL;

	/* SET FIFO PRIORITY, DCLK MULTIPLIER, AND FIFO ENABLE */
	/* Always 6/5 for FIFO, 2x for DCLK multiplier. */

#if HALF_SPEED_UPDATE
	gcfg = 0x0000C801;
#else
#if SONY_ROTATED
	gcfg = 0x00007601;
#else
	gcfg = 0x00006501;
#endif
#endif
	/* SET DOT CLOCK MULTIPLIER */
	/* Bit 31 of frequency indicates divide frequency by two */

	if (pMode->frequency & 0x80000000) gcfg |= 0x0040;
	else gcfg |= 0x0080;

	gcfg |= DC_GCFG_VRDY;

	DWRITE_REG32(DC_OUTPUT_CFG, ocfg);
	DWRITE_REG32(DC_TIMING_CFG, tcfg);
	gfx_delay_milliseconds(1); /* delay after TIMING_CFG */ 
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);

	/* DISABLE VIDEO OUTPUT FROM THE CS5530 */
	/* Same register in SC1400 */

	vcfg = READ_VID32(CS5530_VIDEO_CONFIG);
	vcfg &= ~(CS5530_VCFG_VID_EN);
	DWRITE_VID32(CS5530_VIDEO_CONFIG, vcfg);

	/* ENABLE DISPLAY OUTPUT FROM CX5530 */

	dcfg = (GFX_MODE_PANEL_ON & pMode->flags) ?
		(CS5530_DCFG_GV_PAL_BYP |
		
		CS5530_DCFG_PWR_SEQ_DLY_INIT |
		CS5530_DCFG_CRT_SYNC_SKW_INIT |
		
		CS5530_DCFG_XGA_FP |
		
		CS5530_DCFG_FP_DATA_EN |
		CS5530_DCFG_FP_PWR_EN |
		CS5530_DCFG_DAC_PWDNX |
		
		CS5530_DCFG_DAC_BL_EN |
		CS5530_DCFG_VSYNC_EN |
		CS5530_DCFG_HSYNC_EN |
		CS5530_DCFG_DIS_EN)

		:
		(CS5530_DCFG_GV_PAL_BYP |
		
		CS5530_DCFG_PWR_SEQ_DLY_INIT |
		CS5530_DCFG_CRT_SYNC_SKW_INIT |
		
		CS5530_DCFG_DAC_PWDNX |
		
		CS5530_DCFG_DAC_BL_EN |
		CS5530_DCFG_VSYNC_EN |
		CS5530_DCFG_HSYNC_EN |
		CS5530_DCFG_DIS_EN)
		;
#if 0
#if ENABLE_FLAT_PANEL
	//dcfg = 0x002910EF;
	dcfg =
		CS5530_DCFG_GV_PAL_BYP |
		
		CS5530_DCFG_PWR_SEQ_DLY_INIT |
		CS5530_DCFG_CRT_SYNC_SKW_INIT |
		
		CS5530_DCFG_XGA_FP |
		
		CS5530_DCFG_FP_DATA_EN |
		CS5530_DCFG_FP_PWR_EN |
		CS5530_DCFG_DAC_PWDNX |
		
		CS5530_DCFG_DAC_BL_EN |
		CS5530_DCFG_VSYNC_EN |
		CS5530_DCFG_HSYNC_EN |
		CS5530_DCFG_DIS_EN;
#else
	//dcfg = 0x0029002F;
	dcfg =
		CS5530_DCFG_GV_PAL_BYP |
		
		CS5530_DCFG_PWR_SEQ_DLY_INIT |
		CS5530_DCFG_CRT_SYNC_SKW_INIT |
		
		CS5530_DCFG_DAC_PWDNX |
		
		CS5530_DCFG_DAC_BL_EN |
		CS5530_DCFG_VSYNC_EN |
		CS5530_DCFG_HSYNC_EN |
		CS5530_DCFG_DIS_EN;
#endif
#endif

	/* SET APPROPRIATE SYNC POLARITIES */

	if (pMode->flags & GFX_MODE_NEG_HSYNC)
		dcfg |= (GFX_MODE_PANEL_ON & pMode->flags) ?
				CS5530_DCFG_CRT_HSYNC_POL :
				(CS5530_DCFG_CRT_HSYNC_POL | CS5530_DCFG_FP_HSYNC_POL);
#if 0
#if ENABLE_FLAT_PANEL
		dcfg |= CS5530_DCFG_CRT_HSYNC_POL;
#else
		dcfg |= CS5530_DCFG_CRT_HSYNC_POL | CS5530_DCFG_FP_HSYNC_POL;
#endif
#endif
	if (pMode->flags & GFX_MODE_NEG_VSYNC)
		dcfg |= (GFX_MODE_PANEL_ON & pMode->flags) ?
				CS5530_DCFG_CRT_VSYNC_POL :
				(CS5530_DCFG_CRT_VSYNC_POL | CS5530_DCFG_FP_VSYNC_POL);

#if 0
#if ENABLE_FLAT_PANEL
		dcfg |= CS5530_DCFG_CRT_VSYNC_POL;
#else
		dcfg |= CS5530_DCFG_CRT_VSYNC_POL | CS5530_DCFG_FP_VSYNC_POL;
#endif
#endif
	
	DWRITE_VID32(CS5530_DISPLAY_CONFIG, dcfg);

#if 0
	{
	uint32 t;
	t = READ_VID32(CS5530_CRCSIG_TFT_TV);
	xprintf(("CS5530_CRCSIG_TFT_TV: %.8lx\n", t));
#if 0
	WRITE_VID32(CS5530_CRCSIG_TFT_TV, 0x00000080);
	t = READ_VID32(CS5530_CRCSIG_TFT_TV);
	xprintf(("CS5530_CRCSIG_TFT_TV: %.8lx\n", t));
#endif
	}
#endif
	/* RESTORE VALUE OF DC_UNLOCK */

	DWRITE_REG32(DC_UNLOCK, unlock);

	/* ALSO WRITE GP_BLIT_STATUS FOR PITCH AND 8/18 BPP */

	value = 0;
	if (bpp > 8) value |= BC_16BPP;
	if (pitch > 1024) value |= BC_FB_WIDTH_2048;
	WRITE_REG16(GP_BLIT_STATUS, (unsigned short) value);
	xprintf(("end gfx_set_specified_mode()\n"));
	return;

} /* end gfx_set_specified_mode() */

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_MODE
 *
 * This routine sets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be set.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_display_mode(int xres, int yres, int bpp, int hz)
#else
int gfx_set_display_mode(int xres, int yres, int bpp, int hz)
#endif
{
	unsigned int mode;
	unsigned long hz_flag = 0, bpp_flag = 0;

	/* SET FLAGS TO MATCH REFRESH RATE */

#if 0
	if (hz == 60) hz_flag = GFX_MODE_60HZ;
	if (hz == 75) hz_flag = GFX_MODE_75HZ;
	if (hz == 85) hz_flag = GFX_MODE_85HZ;
#else
	if (hz <= 85) hz_flag = GFX_MODE_85HZ;
	if (hz <= 75) hz_flag = GFX_MODE_75HZ;
	if (hz <= 60) hz_flag = GFX_MODE_60HZ;
#endif
	/* SET BPP FLAGS TO LIMIT MODE SELECTION */

	bpp_flag = GFX_MODE_8BPP;
	if (bpp > 8) bpp_flag = GFX_MODE_16BPP;

	/* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */

	for (mode = 0; mode < NUM_DISPLAY_MODES; mode++) {
		if ((DisplayParams[mode].hactive == xres) &&
			(DisplayParams[mode].vactive == yres) &&
			(DisplayParams[mode].flags & hz_flag) &&
			(DisplayParams[mode].flags & bpp_flag)) {

			/* SET THE DISPLAY CONTROLLER FOR THE SELECTED MODE */

			gu1_set_specified_mode(&DisplayParams[mode], bpp);
			return(1);
			}
		} 
	return(0);
} 

DISPLAYMODE gfx_display_mode;

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_TIMINGS
 *
 * This routine sets the display controller mode using the specified timing
 * values (as opposed to using the tables internal to Durango).
 *
 * Always returns 0.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_display_timings(unsigned short bpp, unsigned short flags, 
	unsigned short hactive, unsigned short hblankstart, 
	unsigned short hsyncstart, unsigned short hsyncend,
	unsigned short hblankend, unsigned short htotal,
	unsigned short vactive, unsigned short vblankstart, 
	unsigned short vsyncstart, unsigned short vsyncend,
	unsigned short vblankend, unsigned short vtotal,
	unsigned long frequency)
#else
int gfx_set_display_timings(unsigned short bpp, unsigned short flags, 
	unsigned short hactive, unsigned short hblankstart, 
	unsigned short hsyncstart, unsigned short hsyncend,
	unsigned short hblankend, unsigned short htotal,
	unsigned short vactive, unsigned short vblankstart, 
	unsigned short vsyncstart, unsigned short vsyncend,
	unsigned short vblankend, unsigned short vtotal,
	unsigned long frequency)
#endif
{
	/* SET MODE STRUCTURE WITH SPECIFIED VALUES */

#if 0
	gfx_display_mode.flags = 0;
	if (flags & 1) gfx_display_mode.flags |= GFX_MODE_NEG_HSYNC;
	if (flags & 2) gfx_display_mode.flags |= GFX_MODE_NEG_VSYNC;
	if (flags & 4) gfx_display_mode.flags |= GFX_MODE_INTERLACED;
	if (flags & 8) gfx_display_mode.flags |= GFX_MODE_PANEL_ON;
#else
	gfx_display_mode.flags = flags;
#endif
	gfx_display_mode.hactive = hactive;
	gfx_display_mode.hblankstart = hblankstart;
	gfx_display_mode.hsyncstart = hsyncstart;
	gfx_display_mode.hsyncend = hsyncend;
	gfx_display_mode.hblankend = hblankend;
	gfx_display_mode.htotal = htotal;
	gfx_display_mode.vactive = vactive;
	gfx_display_mode.vblankstart = vblankstart;
	gfx_display_mode.vsyncstart = vsyncstart;
	gfx_display_mode.vsyncend = vsyncend;
	gfx_display_mode.vblankend = vblankend;
	gfx_display_mode.vtotal = vtotal;
	gfx_display_mode.frequency = frequency;

	/* CALL ROUTINE TO SET MODE */

	gu1_set_specified_mode(&gfx_display_mode, bpp);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_pitch
 *
 * This routine returns the current pitch of the frame buffer, in bytes.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_display_pitch(void)
#else
unsigned short gfx_get_display_pitch(void)
#endif
{
	unsigned long value;
	value = (READ_REG32(DC_LINE_DELTA) & 0x03FF) << 2;
	return((unsigned short) value);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_pitch
 *
 * This routine sets the pitch of the frame buffer to the specified value.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_pitch(unsigned short pitch)
#else
void gfx_set_display_pitch(unsigned short pitch)
#endif
{
	unsigned long value, lock;
	value = (READ_REG32(DC_LINE_DELTA) & 0xFFFFF000);
	value |= (pitch >> 2);
	lock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_LINE_DELTA, value);
	DWRITE_REG32(DC_UNLOCK, lock);

	/* ALSO UPDATE PITCH IN GRAPHICS ENGINE */

	value = (unsigned long) READ_REG16(GP_BLIT_STATUS);
	if (pitch > 1024) value |= BC_FB_WIDTH_2048;
	else value &= ~(BC_FB_WIDTH_2048);
	WRITE_REG16(GP_BLIT_STATUS, (unsigned short) value);	
	return;
}

/*---------------------------------------------------------------------------
 * gfx_set_display_offset
 *
 * This routine sets the start address of the frame buffer.  It is 
 * typically used to pan across a virtual desktop (frame buffer larger than 
 * the displayed screen) or to flip the display between multiple buffers.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_offset(unsigned long offset)
#else
void gfx_set_display_offset(unsigned long offset)
#endif
{
	/* UPDATE FRAME BUFFER OFFSET */

	unsigned long lock;
	lock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_FB_ST_OFFSET, offset);
	DWRITE_REG32(DC_UNLOCK, lock);

	/* START ADDRESS EFFECTS DISPLAY COMPRESSION */
	/* Disable compression for non-zero start addresss values. */
	/* Enable compression if offset is zero and comression is intended to */
	/* be enabled from a previous call to "gfx_set_compression_enable". */

	if ((offset == 0) && gfx_compression_enabled)
		gu1_enable_compression();
	else gu1_disable_compression();
	return;
}

/*---------------------------------------------------------------------------
 * gfx_set_display_palette
 *
 * This routine sets the entire palette in the display controller.
 * A pointer is provided to a 256 entry table of 32-bit X:R:G:B values.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_display_palette(unsigned long *palette)
#else
int gfx_set_display_palette(unsigned long *palette)
#endif
{
	unsigned long data, i;
	DWRITE_REG32(DC_PAL_ADDRESS, 0);
	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			/* CONVERT 24 BPP COLOR DATA TO 18 BPP COLOR DATA */

			data = ((palette[i] >> 2) & 0x0003F) | 
				((palette[i] >> 4) & 0x00FC0) |
				((palette[i] >> 6) & 0x3F000);
			DWRITE_REG32(DC_PAL_DATA, data);
		}
	}
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_enable
 *
 * This routine enables or disables the hardware cursor.  
 *
 * WARNING: The cusrsor start offset must be set by setting the cursor 
 * position before calling this routine to assure that memory reads do not
 * go past the end of graphics memory (this can hang GXm).
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_cursor_enable(int enable)
#else
void gfx_set_cursor_enable(int enable)
#endif
{
	unsigned long unlock, gcfg;
	
	/* SET OR CLEAR CURSOR ENABLE BIT */

	unlock = READ_REG32(DC_UNLOCK);
	gcfg = READ_REG32(DC_GENERAL_CFG);
	if (enable) gcfg |= DC_GCFG_CURE;
	else gcfg &= ~(DC_GCFG_CURE);

	/* WRITE NEW REGISTER VALUE */

	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	DWRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_colors
 *
 * This routine sets the colors of the hardware cursor.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#else
void gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#endif
{
	unsigned long value;

	/* SET CURSOR COLORS */

	DWRITE_REG32(DC_PAL_ADDRESS, 0x100);
	value = ((bkcolor & 0x000000FC) >> 2) |
		((bkcolor & 0x0000FC00) >> (2+8-6)) |
		((bkcolor & 0x00FC0000) >> (2+16-12));
	DWRITE_REG32(DC_PAL_DATA, value);
	value = ((fgcolor & 0x000000FC) >> 2) |
		((fgcolor & 0x0000FC00) >> (2+8-6)) |
		((fgcolor & 0x00FC0000) >> (2+16-12));
	DWRITE_REG32(DC_PAL_DATA, value);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_position
 *
 * This routine sets the position of the hardware cusror.  The starting
 * offset of the cursor buffer must be specified so that the routine can 
 * properly clip scanlines if the cursor is off the top of the screen.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot)
#else
void gfx_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot)
#endif
{
	unsigned long unlock;

	short x = (short) xpos - (short) xhotspot;
	short y = (short) ypos - (short) yhotspot;
	short xoffset = 0;
	short yoffset = 0;
	if (x < -31) return;
	if (y < -31) return;
	if (x < 0) { xoffset = -x; x = 0; }
	if (y < 0) { yoffset = -y; y = 0; }
	memoffset += (unsigned long) yoffset << 3;

	/* SET CURSOR POSITION */

	unlock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_CURS_ST_OFFSET, memoffset);
	DWRITE_REG32(DC_CURSOR_X, (unsigned long) x |
		(((unsigned long) xoffset) << 11));
	DWRITE_REG32(DC_CURSOR_Y, (unsigned long) y | 
		(((unsigned long) yoffset) << 11));
	DWRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape32
 *
 * This routine loads 32x32 cursor data into the specified location in 
 * graphics memory.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask)
#else
void gfx_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask)
#endif
{
	int i;
	unsigned long value;
	for (i = 0; i < 32; i++)
	{
		/* CONVERT TO 16 BITS AND MASK, 16 BITS XOR MASK PER DWORD */

		value = (andmask[i] & 0xFFFF0000) | (xormask[i] >> 16);
		WRITE_FB32(memoffset, value);
		memoffset += 4;
		value = (andmask[i] << 16) | (xormask[i] & 0x0000FFFF);
		WRITE_FB32(memoffset, value);
		memoffset += 4;
	}
}

/*---------------------------------------------------------------------------
 * gu1_enable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It enables display compression.
 *---------------------------------------------------------------------------
 */
void gu1_enable_compression(void)
{
	int i;
	unsigned long unlock, gcfg, offset;

	/* DO NOT ENABLE IF START ADDRESS IS NOT ZERO */

	offset = READ_REG32(DC_FB_ST_OFFSET) & 0x003FFFFF;
	if (offset != 0) return;

	/* CLEAR DIRTY/VALID BITS IN MEMORY CONTROLLER */
	/* Software is required to do this before enabling compression. */
	/* Don't want controller to think that old lines are still valid. */

	for (i = 0; i < 1024; i++)
	{
		WRITE_REG32(MC_DR_ADD, i);
		WRITE_REG32(MC_DR_ACC, 0);
	}

	/* TURN ON COMPRESSION CONTROL BITS */

	unlock = READ_REG32(DC_UNLOCK);
	gcfg = READ_REG32(DC_GENERAL_CFG);
	gcfg |= DC_GCFG_CMPE | DC_GCFG_DECE;
	gcfg &= ~DC_GCFG_CIM_MASK;
#if HALF_SPEED_UPDATE
	gcfg |= (1 << DC_GCFG_CIM_POS);
#endif
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	DWRITE_REG32(DC_UNLOCK, unlock);
	xprintf(("compression enabled with: 0x%.08lx\n", gcfg));
}

/*---------------------------------------------------------------------------
 * gu1_disable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It disables display compression.
 *---------------------------------------------------------------------------
 */
void gu1_disable_compression(void)
{
	unsigned long unlock, gcfg;

	/* TURN OFF COMPRESSION CONTROL BITS */

	unlock = READ_REG32(DC_UNLOCK);
	gcfg = READ_REG32(DC_GENERAL_CFG);
	gcfg &= ~(DC_GCFG_CMPE | DC_GCFG_DECE);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	DWRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_enable
 *
 * This routine enables or disables display compression.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_compression_enable(int enable)
#else
int gfx_set_compression_enable(int enable)
#endif
{
	/* SET GLOBAL VARIABLE FOR INDENDED STATE */
	/* Compression can only be enabled for zero start address values. */
	/* Keep state to enable compression on start address changes. */

	gfx_compression_enabled = enable;
	if (enable) gu1_enable_compression();
	else gu1_disable_compression();
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_offset
 *
 * This routine sets the base offset for the compression buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_compression_offset(unsigned long offset)
#else
int gfx_set_compression_offset(unsigned long offset)
#endif
{
	unsigned long lock;
	
	/* MUST BE 16-BYTE ALIGNED FOR GXLV */

	if (offset & 0x0F) return(1);

	/* SET REGISTER VALUE */

	lock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_CB_ST_OFFSET, offset);
	DWRITE_REG32(DC_UNLOCK, lock);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_pitch
 *
 * This routine sets the pitch, in bytes, of the compression buffer. 
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_compression_pitch(unsigned short pitch)
#else
int gfx_set_compression_pitch(unsigned short pitch)
#endif
{
	unsigned long lock, line_delta;
	
	/* SET REGISTER VALUE */

	lock = READ_REG32(DC_UNLOCK);
	line_delta = READ_REG32(DC_LINE_DELTA) & 0xFF800FFF;
	line_delta |= (pitch << 10) & 0x007FF000;
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_LINE_DELTA, line_delta);
	DWRITE_REG32(DC_UNLOCK, lock);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_size
 *
 * This routine sets the line size of the compression buffer, which is the
 * maximum number of bytes allowed to store a compressed line.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_set_compression_size(unsigned short size)
#else
int gfx_set_compression_size(unsigned short size)
#endif
{
	unsigned long lock, buf_size;
	
	/* SET REGISTER VALUE */

	size -= 16;	/* FFB: yes, it looks goofy, but things break if you remove it */
	lock = READ_REG32(DC_UNLOCK);
	buf_size = READ_REG32(DC_BUF_SIZE) & 0xFFFF01FF;
	buf_size |= (((size >> 2) + 1) & 0x7F) << 9;
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_BUF_SIZE, buf_size);
	DWRITE_REG32(DC_UNLOCK, lock);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_enable".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_video_enable(int enable)
#else
void gfx_set_display_video_enable(int enable)
#endif
{
	unsigned long lock, gcfg;
	lock = READ_REG32(DC_UNLOCK);
	gcfg = READ_REG32(DC_GENERAL_CFG);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	if (enable)
		gcfg |= (DC_GCFG_VIDE | DC_GCFG_VRDY);
	else gcfg &= ~(DC_GCFG_VIDE);
	DWRITE_REG32(DC_GENERAL_CFG, gcfg);
	DWRITE_REG32(DC_UNLOCK, lock);
	return;
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_video_size(unsigned short width, unsigned short height)
#else
void gfx_set_display_video_size(unsigned short width, unsigned short height)
#endif
{
	unsigned long lock, size, value;
	size = (unsigned long) (width << 1) * (unsigned long) height;
	lock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	value = READ_REG32(DC_BUF_SIZE) & 0x0000FFFF;
	value |= (((size + 63) >> 6) << 16);  
	DWRITE_REG32(DC_BUF_SIZE, value);
	DWRITE_REG32(DC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_video_offset(unsigned long offset)
#else
void gfx_set_display_video_offset(unsigned long offset)
#endif
{
	unsigned long lock;
	lock = READ_REG32(DC_UNLOCK);
	DWRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
	DWRITE_REG32(DC_VID_ST_OFFSET, offset);
	DWRITE_REG32(DC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_test_timing_active
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_test_timing_active(void)
#else
int gfx_test_timing_active(void)
#endif
{
	if (READ_REG32(DC_TIMING_CFG) & DC_TCFG_TGEN)
		return(1);
	else return(0);
}

/*---------------------------------------------------------------------------
 * gfx_test_vertical_active
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_test_vertical_active(void)
#else
int gfx_test_vertical_active(void)
#endif
{
	if (READ_REG32(DC_TIMING_CFG) & DC_TCFG_VNA)
		return(0);
	else return(1);
}

/*---------------------------------------------------------------------------
 * gfx_wait_vertical_blank
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_wait_vertical_blank(void)
#else
int gfx_wait_vertical_blank(void)
#endif
{
	if (gfx_test_timing_active())
	{
		while(!gfx_test_vertical_active());
		while(gfx_test_vertical_active());
	}
	return(0);
}

/* THE FOLLOWING READ ROUTINES ARE ALWAYS INCLUDED: */
/* gfx_get_hsync_end, gfx_get_htotal, gfx_get_vsync_end, gfx_get_vtotal. */
/* This is because they are used by the video overlay routines. */

/*---------------------------------------------------------------------------
 * gfx_get_hactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_hactive(void)
#else
unsigned short gfx_get_hactive(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_H_TIMING_1) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_hsync_end(void)
#else
unsigned short gfx_get_hsync_end(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_H_TIMING_3) >> 16) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_htotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_htotal(void)
#else
unsigned short gfx_get_htotal(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_H_TIMING_1) >> 16) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vactive(void)
#else
unsigned short gfx_get_vactive(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_V_TIMING_1) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vsync_end(void)
#else
unsigned short gfx_get_vsync_end(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_V_TIMING_3) >> 16) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vtotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vtotal(void)
#else
unsigned short gfx_get_vtotal(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_V_TIMING_1) >> 16) & 0x07FF) + 1));
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_bpp
 *
 * This routine returns the current color depth of the active display.
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_display_bpp(void)
#else
unsigned short gfx_get_display_bpp(void)
#endif
{
	switch(READ_REG32(DC_OUTPUT_CFG) & 3)
	{
		case 0: return(16);
		case 2: return(15);
	}
	return(8);
}

#if GFX_READ_ROUTINES

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

/*---------------------------------------------------------------------------
 * gfx_get_hblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_hblank_start(void)
#else
unsigned short gfx_get_hblank_start(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_H_TIMING_2) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_hsync_start(void)
#else
unsigned short gfx_get_hsync_start(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_H_TIMING_3) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_hblank_end(void)
#else
unsigned short gfx_get_hblank_end(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_H_TIMING_2) >> 16) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vblank_start(void)
#else
unsigned short gfx_get_vblank_start(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_V_TIMING_2) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vsync_start(void)
#else
unsigned short gfx_get_vsync_start(void)
#endif
{
	return((unsigned short)((READ_REG32(DC_V_TIMING_3) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_vblank_end(void)
#else
unsigned short gfx_get_vblank_end(void)
#endif
{
	return((unsigned short)(((READ_REG32(DC_V_TIMING_2) >> 16) & 0x07FF) + 1));
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_display_offset(void)
#else
unsigned long gfx_get_display_offset(void)
#endif
{
	return(READ_REG32(DC_FB_ST_OFFSET) & 0x003FFFFF);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_palette
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu1_get_display_palette(unsigned long *palette)
#else
void gfx_get_display_palette(unsigned long *palette)
#endif
{
	unsigned long i, data;
	DWRITE_REG32(DC_PAL_ADDRESS, 0);
	for (i = 0; i < 256; i++)
	{
		data = READ_REG32(DC_PAL_DATA);
		data = ((data << 2) & 0x000000FC) |
			((data << 4) & 0x0000FC00) |
			((data << 6) & 0x00FC0000);
		palette[i] = data;
	}
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_cursor_enable(void)
#else
unsigned long gfx_get_cursor_enable(void)
#endif
{
	return(READ_REG32(DC_GENERAL_CFG) & DC_GCFG_CURE);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_cursor_offset(void)
#else
unsigned long gfx_get_cursor_offset(void)
#endif
{
	return(READ_REG32(DC_CURS_ST_OFFSET) & 0x003FFFFF);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_position
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_cursor_position(void)
#else
unsigned long gfx_get_cursor_position(void)
#endif
{
	return((READ_REG32(DC_CURSOR_X) & 0x07FF) |
		((READ_REG32(DC_CURSOR_Y) << 16) & 0x03FF0000));
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_clip
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_cursor_clip(void)
#else
unsigned long gfx_get_cursor_clip(void)
#endif
{
	return(((READ_REG32(DC_CURSOR_X) >> 11) & 0x01F) |
		((READ_REG32(DC_CURSOR_Y) << 5) & 0x1F0000));
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_color
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_cursor_color(int color)
#else
unsigned long gfx_get_cursor_color(int color)
#endif
{
	unsigned long data;
	if (color) 
	{
		DWRITE_REG32(DC_PAL_ADDRESS, 0x101);
	}
	else
	{
		DWRITE_REG32(DC_PAL_ADDRESS, 0x100);
	}
	data = READ_REG32(DC_PAL_DATA);
	data = ((data << 6) & 0x00FC0000) |
		((data << 4) & 0x0000FC00) |
		((data << 2) & 0x000000FC);
	return(data);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_get_compression_enable(void)
#else
int gfx_get_compression_enable(void)
#endif
{
	unsigned long gcfg;
	gcfg = READ_REG32(DC_GENERAL_CFG);
	if (gcfg & DC_GCFG_CMPE) return(1);
	else return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_compression_offset(void)
#else
unsigned long gfx_get_compression_offset(void)
#endif
{
	unsigned long offset;
	offset = READ_REG32(DC_CB_ST_OFFSET) & 0x003FFFFF;
	return(offset);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_pitch
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_compression_pitch(void)
#else
unsigned short gfx_get_compression_pitch(void)
#endif
{
	unsigned short pitch;
	pitch = (unsigned short) (READ_REG32(DC_LINE_DELTA) >> 12) & 0x07FF;
	return(pitch << 2);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_size
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu1_get_compression_size(void)
#else
unsigned short gfx_get_compression_size(void)
#endif
{
	unsigned short size;
	size = (unsigned short) ((READ_REG32(DC_BUF_SIZE) >> 9) & 0x7F) - 1;
	return(size << 2);
}

/*-----------------------------------------------------------------------------
 * gfx_get_valid_bit
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu1_get_valid_bit(int line)
#else
int gfx_get_valid_bit(int line)
#endif
{
	int valid;
	DWRITE_REG32(MC_DR_ADD, line);
	valid = READ_REG32(MC_DR_ACC) & 1;
	return(valid);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_display_video_offset(void)
#else
unsigned long gfx_get_display_video_offset(void)
#endif
{
	return(READ_REG32(DC_VID_ST_OFFSET) & 0x001FFFFF);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu1_get_display_video_size(void)
#else
unsigned long gfx_get_display_video_size(void)
#endif
{
	/* RETURN TOTAL SIZE, IN BYTES */

	return((READ_REG32(DC_BUF_SIZE) >> 10) & 0x000FFFC0);
}

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
