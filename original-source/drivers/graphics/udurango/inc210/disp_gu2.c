/*-----------------------------------------------------------------------------
 * DISP_GU2.C
 *
 * Version 2.1 - March 3, 2000
 *
 * This file contains routines for the second generation display controller.  
 *
 * History:
 *    Versions 0.1 through 2.1 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

void gu2_enable_compression(void);		/* private routine definition */
void gu2_disable_compression(void);		/* private routine definition */

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
void gu2_delay_milliseconds(unsigned long milliseconds)
#else
void gfx_delay_milliseconds(unsigned long milliseconds)
#endif
{
	/* ASSUME 300 MHz, 5 CLOCKS PER READ */

	#define READS_PER_MILLISECOND 60000L

	unsigned long loop;
	loop = milliseconds * READS_PER_MILLISECOND;
	while (loop-- > 0)
	{
		READ_REG32(DC_UNLOCK);
	}
}

/*-----------------------------------------------------------------------------
 * gu2_set_specified_mode (private routine)
 * This routine uses the parameters in the specified display mode structure
 * to program the display controller hardware.  
 *-----------------------------------------------------------------------------
 */
void gu2_set_specified_mode(DISPLAYMODE *pMode, int bpp)
{
	/* ### ADD ### IMPLEMENTATION */
}

/*----------------------------------------------------------------------------
 * gfx_set_display_mode
 *
 * This routine sets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be set.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_set_display_mode(int xres, int yres, int bpp, int hz)
#else
int gfx_set_display_mode(int xres, int yres, int bpp, int hz)
#endif
{
	int mode;
	unsigned long hz_flag = 0, bpp_flag = 0;

	/* SET FLAGS TO MATCH REFRESH RATE */

	if (hz == 60) hz_flag = GFX_MODE_60HZ;
	if (hz == 75) hz_flag = GFX_MODE_75HZ;
	if (hz == 85) hz_flag = GFX_MODE_85HZ;

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

			gu2_set_specified_mode(&DisplayParams[mode], bpp);
			return(1);
			}
		} 
	return(0);
} 

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
int gu2_set_display_timings(unsigned short bpp, unsigned short flags, 
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

	gfx_display_mode.flags = 0;
	if (flags & 1) gfx_display_mode.flags |= GFX_MODE_NEG_HSYNC;
	if (flags & 2) gfx_display_mode.flags |= GFX_MODE_NEG_VSYNC;
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

	gu2_set_specified_mode(&gfx_display_mode, bpp);
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_pitch
 *
 * This routine returns the current pitch of the frame buffer, in bytes.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_display_pitch(void)
#else
unsigned short gfx_get_display_pitch(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_pitch
 *
 * This routine sets the pitch of the frame buffer to the specified value.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu2_set_display_pitch(unsigned short pitch)
#else
void gfx_set_display_pitch(unsigned short pitch)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
void gu2_set_display_offset(unsigned long offset)
#else
void gfx_set_display_offset(unsigned long offset)
#endif
{
	/* ### ADD ### IMPLEMENTATION */

	/* START ADDRESS EFFECTS DISPLAY COMPRESSION */
	/* Disable compression for non-zero start addresss values. */
	/* Enable compression if offset is zero and comression is intended to */
	/* be enabled from a previous call to "gfx_set_compression_enable". */

	if ((offset == 0) && gfx_compression_enabled)
		gu2_enable_compression();
	else gu2_disable_compression();
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
int gu2_set_display_palette(unsigned long *palette)
#else
int gfx_set_display_palette(unsigned long *palette)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
void gu2_set_cursor_enable(int enable)
#else
void gfx_set_cursor_enable(int enable)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_colors
 *
 * This routine sets the colors of the hardware cursor.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu2_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#else
void gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
void gu2_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot)
#else
void gfx_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape
 *
 * This routine loads cursor data into the cursor buffer in graphics memory.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu2_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask)
#else
void gfx_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gu2_enable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It enables display compression.
 *---------------------------------------------------------------------------
 */
void gu2_enable_compression(void)
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gu2_disable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It disables display compression.
 *---------------------------------------------------------------------------
 */
void gu2_disable_compression(void)
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_enable
 *
 * This routine enables or disables display compression.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_set_compression_enable(int enable)
#else
int gfx_set_compression_enable(int enable)
#endif
{
	/* SET GLOBAL VARIABLE FOR INDENDED STATE */
	/* Compression can only be enabled for non-zero start address values. */
	/* Keep state to enable compression on start address changes. */

	gfx_compression_enabled = enable;
	if (enable) gu2_enable_compression();
	else gu2_disable_compression();
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_offset
 *
 * This routine sets the base offset for the compression buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_set_compression_offset(unsigned long offset)
#else
int gfx_set_compression_offset(unsigned long offset)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_pitch
 *
 * This routine sets the pitch, in bytes, of the compression buffer. 
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_set_compression_pitch(unsigned short pitch)
#else
int gfx_set_compression_pitch(unsigned short pitch)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
int gu2_set_compression_size(unsigned short size)
#else
int gfx_set_compression_size(unsigned short size)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
void gu2_set_display_video_enable(int enable)
#else
void gfx_set_display_video_enable(int enable)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
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
void gu2_set_display_video_size(unsigned short width, unsigned short height)
#else
void gfx_set_display_video_size(unsigned short width, unsigned short height)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu2_set_display_video_offset(unsigned long offset)
#else
void gfx_set_display_video_offset(unsigned long offset)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

#if GFX_DISPLAY_DYNAMIC
int gu2_test_timing_active(void)
#else
int gfx_test_timing_active(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

#if GFX_DISPLAY_DYNAMIC
int gu2_test_vertical_active(void)
#else
int gfx_test_vertical_active(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	if (READ_REG32(DC_TIMING_CFG) & DC_TCFG_VNA)
	return(1);
}

#if GFX_DISPLAY_DYNAMIC
int gu2_wait_vertical_blank(void)
#else
int gfx_wait_vertical_blank(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/* THE FOLLOWING READ ROUTINES ARE ALWAYS INCLUDED: */
/* gfx_get_hsync_end, gfx_get_htotal, gfx_get_vsync_end, gfx_get_vtotal.
/* This is because they are used by the video overlay routines. */

/*---------------------------------------------------------------------------
 * gfx_get_hactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_hactive(void)
#else
unsigned short gfx_get_hactive(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_hsync_end(void)
#else
unsigned short gfx_get_hsync_end(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_htotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_htotal(void)
#else
unsigned short gfx_get_htotal(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vactive(void)
#else
unsigned short gfx_get_vactive(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vsync_end(void)
#else
unsigned short gfx_get_vsync_end(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vtotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vtotal(void)
#else
unsigned short gfx_get_vtotal(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_bpp
 *
 * This routine returns the current color depth of the active display.
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_display_bpp(void)
#else
unsigned short gfx_get_display_bpp(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
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
unsigned short gu2_get_hblank_start(void)
#else
unsigned short gfx_get_hblank_start(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_hsync_start(void)
#else
unsigned short gfx_get_hsync_start(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_hblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_hblank_end(void)
#else
unsigned short gfx_get_hblank_end(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vblank_start(void)
#else
unsigned short gfx_get_vblank_start(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vsync_start(void)
#else
unsigned short gfx_get_vsync_start(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_vblank_end(void)
#else
unsigned short gfx_get_vblank_end(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_display_offset(void)
#else
unsigned long gfx_get_display_offset(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_palette
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void gu2_get_display_palette(unsigned long *palette)
#else
void gfx_get_display_palette(unsigned long *palette)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_cursor_enable(void)
#else
unsigned long gfx_get_cursor_enable(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_cursor_offset(void)
#else
unsigned long gfx_get_cursor_offset(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_position
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_cursor_position(void)
#else
unsigned long gfx_get_cursor_position(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_cursor_clip(void)
#else
unsigned long gfx_get_cursor_clip(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_color
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_cursor_color(int color)
#else
unsigned long gfx_get_cursor_color(int color)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_get_compression_enable(void)
#else
int gfx_get_compression_enable(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_compression_offset(void)
#else
unsigned long gfx_get_compression_offset(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_pitch
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_compression_pitch(void)
#else
unsigned short gfx_get_compression_pitch(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_size
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short gu2_get_compression_size(void)
#else
unsigned short gfx_get_compression_size(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_valid_bit
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int gu2_get_valid_bit(int line)
#else
int gfx_get_valid_bit(int line)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_display_video_offset(void)
#else
unsigned long gfx_get_display_video_offset(void);
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long gu2_get_display_video_size(void)
#else
unsigned long gfx_get_display_video_size(void);
#endif
{
	/* ### ADD ### IMPLEMENTATION - return TOTAL size */
	return(0);
}

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
