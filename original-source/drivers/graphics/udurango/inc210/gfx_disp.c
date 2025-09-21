/*-----------------------------------------------------------------------------
 * GFX_DISP.C
 *
 * Version 2.1 - March 3, 2000
 *
 * This file contains routines to program the display controller.  
 *
 * The "disp_gu1.c" and "disp_gu2.c" files implement the following routines:
 * 
 *    gfx_set_display_mode
 *	  gfx_set_display_timings
 *    gfx_get_display_pitch
 *    gfx_set_display_pitch
 *    gfx_set_display_offset
 *    gfx_set_display_palette
 *    gfx_set_cursor_enable
 *    gfx_set_cursor_colors
 *    gfx_set_cursor_position
 *	  gfx_set_cursor_shape32
 *    gfx_set_compression_enable
 *    gfx_set_compression_offset
 *    gfx_set_compression_pitch
 *    gfx_set_compression_size
 *    gfx_test_timing_active
 *    gfx_test_vertical_active
 *    gfx_wait_vertical_blank
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *	
 *	  gfx_get_hactive
 *    gfx_get_hblank_start
 *    gfx_get_hsync_start
 *    gfx_get_hsync_end
 *    gfx_get_hblank_end
 *    gfx_get_htotal
 *    gfx_get_vactive
 *    gfx_get_vblank_start
 *    gfx_get_vsync_start
 *    gfx_get_vsync_end
 *    gfx_get_vblank_end
 *    gfx_get_vtotal
 *    gfx_get_display_bpp
 *    gfx_get_display_offset
 *    gfx_get_display_palette
 *    gfx_get_cursor_enable
 *    gfx_get_cursor_base
 *    gfx_get_cursor_position
 *    gfx_get_cursor_offset
 *    gfx_get_cursor_color
 *    gfx_get_compression_enable
 *    gfx_get_compression_offset
 *    gfx_get_compression_pitch
 *    gfx_get_compression_size
 *    gfx_get_valid_bit
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

int gfx_compression_enabled = 0;
DISPLAYMODE gfx_display_mode;

/* INCLUDE SUPPORT FOR FIRST GENERATION, IF SPECIFIED. */

#if GFX_DISPLAY_GU1
#include "disp_gu1.c"
#endif

/* INCLUDE SUPPORT FOR SECOND GENERATION, IF SPECIFIED. */

#if GFX_DISPLAY_GU2
#include "disp_gu2.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either first or second generation routines. */

#if GFX_DISPLAY_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_set_display_mode
 *---------------------------------------------------------------------------
 */
int gfx_set_display_mode(int xres, int yres, int bpp, int hz)
{
	int retval = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		retval = gu1_set_display_mode(xres, yres, bpp, hz);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		retval = gu2_set_display_mode(xres, yres, bpp, hz);
	#endif
	return(retval);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_timings
 *---------------------------------------------------------------------------
 */
int gfx_set_display_timings(unsigned short bpp, unsigned short flags, 
	unsigned short hactive, unsigned short hblankstart, 
	unsigned short hsyncstart, unsigned short hsyncend,
	unsigned short hblankend, unsigned short htotal,
	unsigned short vactive, unsigned short vblankstart, 
	unsigned short vsyncstart, unsigned short vsyncend,
	unsigned short vblankend, unsigned short vtotal,
	unsigned long frequency)
{
	int retval = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		retval = gu1_set_display_timings(bpp, flags, 
			hactive, hblankstart, hsyncstart, hsyncend, hblankend, htotal,
			vactive, vblankstart, vsyncstart, vsyncend, vblankend, vtotal,
			frequency);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		retval = gu2_set_display_timings(bpp, flags, 
			hactive, hblankstart, hsyncstart, hsyncend, hblankend, htotal,
			vactive, vblankstart, vsyncstart, vsyncend, vblankend, vtotal,
			frequency);
	#endif
	return(retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_pitch
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_display_pitch(void)
{
	unsigned short pitch = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		pitch = gu1_get_display_pitch();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		pitch = gu2_get_display_pitch();
	#endif
	return(pitch);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_pitch
 *---------------------------------------------------------------------------
 */
void gfx_set_display_pitch(unsigned short pitch)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_display_pitch(pitch);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_display_pitch(pitch);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_display_offset
 *---------------------------------------------------------------------------
 */
void gfx_set_display_offset(unsigned long offset)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_display_offset(offset);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_display_offset(offset);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_display_palette
 *---------------------------------------------------------------------------
 */
int gfx_set_display_palette(unsigned long *palette)
{
	int status;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_set_display_palette(palette);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_set_display_palette(palette);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_enable
 *---------------------------------------------------------------------------
 */
void gfx_set_cursor_enable(int enable)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_cursor_enable(enable);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_cursor_enable(enable);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_colors
 *---------------------------------------------------------------------------
 */
void gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_cursor_colors(bkcolor, fgcolor);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_cursor_colors(bkcolor, fgcolor);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_position
 *---------------------------------------------------------------------------
 */
void gfx_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_cursor_position(memoffset, xpos, ypos, xhotspot, yhotspot);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_cursor_position(memoffset, xpos, ypos, xhotspot, yhotspot);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape
 *---------------------------------------------------------------------------
 */
void gfx_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_cursor_shape32(memoffset, andmask, xormask);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_cursor_shape32(memoffset, andmask, xormask);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_enable
 *---------------------------------------------------------------------------
 */
int gfx_set_compression_enable(int enable)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_set_compression_enable(enable);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_set_compression_enable(enable);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_offset
 *---------------------------------------------------------------------------
 */
int gfx_set_compression_offset(unsigned long offset)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_set_compression_offset(offset);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_set_compression_offset(offset);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_pitch
 *---------------------------------------------------------------------------
 */
int gfx_set_compression_pitch(unsigned short pitch)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_set_compression_pitch(pitch);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_set_compression_pitch(pitch);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_size
 *---------------------------------------------------------------------------
 */
int gfx_set_compression_size(unsigned short size)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_set_compression_size(size);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_set_compression_size(size);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_enable".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
void gfx_set_display_video_enable(int enable)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_display_video_enable(enable);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_display_video_enable(enable);
	#endif
	return;
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
void gfx_set_display_video_size(unsigned short width, unsigned short height)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_display_video_size(width, height);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_display_video_size(width, height);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
void gfx_set_display_video_offset(unsigned long offset)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_set_display_video_offset(offset);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_set_display_video_offset(offset);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_test_timing_active
 *---------------------------------------------------------------------------
 */
int gfx_test_timing_active(void)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_test_timing_active();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_test_timing_active();
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_test_vertical_active
 *---------------------------------------------------------------------------
 */
int gfx_test_vertical_active(void)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_test_vertical_active();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_test_vertical_active();
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_wait_vertical_blank
 *---------------------------------------------------------------------------
 */
int gfx_wait_vertical_blank(void)
{
	int status = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		status = gu1_wait_vertical_blank();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		status = gu2_wait_vertical_blank();
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_delay_milleseconds
 *---------------------------------------------------------------------------
 */
void gfx_delay_milliseconds(unsigned long milliseconds)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_delay_milliseconds(milliseconds);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_delay_milliseconds(milliseconds);
	#endif
}

/* THE FOLLOWING READ ROUTINES ARE ALWAYS INCLUDED: */
/* gfx_get_hsync_end, gfx_get_htotal, gfx_get_vsync_end, gfx_get_vtotal. */
/* This is because they are used by the video overlay routines. */

/*---------------------------------------------------------------------------
 * gfx_get_hactive
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_hactive(void)
{
	unsigned short hactive = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		hactive = gu1_get_hactive();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		hactive = gu2_get_hactive();
	#endif
	return(hactive);
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_end
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_hsync_end(void)
{
	unsigned short hsync_end = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		hsync_end = gu1_get_hsync_end();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		hsync_end = gu2_get_hsync_end();
	#endif
	return(hsync_end);
}

/*---------------------------------------------------------------------------
 * gfx_get_htotal
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_htotal(void)
{
	unsigned short htotal = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		htotal = gu1_get_htotal();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		htotal = gu2_get_htotal();
	#endif
	return(htotal);
}

/*---------------------------------------------------------------------------
 * gfx_get_vactive
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vactive(void)
{
	unsigned short vactive = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vactive = gu1_get_vactive();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vactive = gu2_get_vactive();
	#endif
	return(vactive);
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_end
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vsync_end(void)
{
	unsigned short vsync_end = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vsync_end = gu1_get_vsync_end();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vsync_end = gu2_get_vsync_end();
	#endif
	return(vsync_end);
}

/*---------------------------------------------------------------------------
 * gfx_get_vtotal
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vtotal(void)
{
	unsigned short vtotal = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vtotal = gu1_get_vtotal();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vtotal = gu2_get_vtotal();
	#endif
	return(vtotal);
}

/*---------------------------------------------------------------------------
 *  gfx_get_display_bpp
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_display_bpp(void)
{
	unsigned short bpp = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		bpp = gu1_get_display_bpp();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		bpp = gu2_get_display_bpp();
	#endif
	return(bpp);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_hblank_start
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_hblank_start(void)
{
	unsigned short hblank_start = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		hblank_start = gu1_get_hblank_start();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		hblank_start = gu2_get_hblank_start();
	#endif
	return(hblank_start);
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_start
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_hsync_start(void)
{
	unsigned short hsync_start = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		hsync_start = gu1_get_hsync_start();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		hsync_start = gu2_get_hsync_start();
	#endif
	return(hsync_start);
}

/*---------------------------------------------------------------------------
 * gfx_get_hblank_end
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_hblank_end(void)
{
	unsigned short hblank_end = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		hblank_end = gu1_get_hblank_end();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		hblank_end = gu2_get_hblank_end();
	#endif
	return(hblank_end);
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_start
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vblank_start(void)
{
	unsigned short vblank_start = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vblank_start = gu1_get_vblank_start();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vblank_start = gu2_get_vblank_start();
	#endif
	return(vblank_start);
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_start
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vsync_start(void)
{
	unsigned short vsync_start = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vsync_start = gu1_get_vsync_start();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vsync_start = gu2_get_vsync_start();
	#endif
	return(vsync_start);
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_end
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_vblank_end(void)
{
	unsigned short vblank_end = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		vblank_end = gu1_get_vblank_end();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		vblank_end = gu2_get_vblank_end();
	#endif
	return(vblank_end);
}

/*---------------------------------------------------------------------------
 *  gfx_get_display_offset
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_display_offset(void)
{
	unsigned long offset = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		offset = gu1_get_display_offset();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		offset = gu2_get_display_offset();
	#endif
	return(offset);
}

/*---------------------------------------------------------------------------
 *  gfx_get_display_palette
 *---------------------------------------------------------------------------
 */
void gfx_get_display_palette(unsigned long *palette)
{
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		gu1_get_display_palette(palette);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		gu2_get_display_palette(palette);
	#endif
}

/*---------------------------------------------------------------------------
 *  gfx_get_cursor_enable
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_cursor_enable(void)
{
	unsigned long enable = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		enable = gu1_get_cursor_enable();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		enable = gu2_get_cursor_enable();
	#endif
	return(enable);
}

/*---------------------------------------------------------------------------
 *  gfx_get_cursor_offset
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_cursor_offset(void)
{
	unsigned long base = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		base = gu1_get_cursor_offset();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		base = gu2_get_cursor_offset();
	#endif
	return(base);
}

/*---------------------------------------------------------------------------
 *  gfx_get_cursor_position
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_cursor_position(void)
{
	unsigned long position = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		position = gu1_get_cursor_position();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		position = gu2_get_cursor_position();
	#endif
	return(position);
}

/*---------------------------------------------------------------------------
 *  gfx_get_cursor_clip
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_cursor_clip(void)
{
	unsigned long offset = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		offset = gu1_get_cursor_clip();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		offset = gu2_get_cursor_clip();
	#endif
	return(offset);
}

/*---------------------------------------------------------------------------
 *  gfx_get_cursor_color
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_cursor_color(int index)
{
	unsigned long color = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		color = gu1_get_cursor_color(index);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		color = gu2_get_cursor_color(index);
	#endif
	return(color);
}

/*---------------------------------------------------------------------------
 *  gfx_get_compression_enable
 *---------------------------------------------------------------------------
 */
int gfx_get_compression_enable(void)
{
	int enable = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		enable = gu1_get_compression_enable();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		enable = gu2_get_compression_enable();
	#endif
	return(enable);
}

/*---------------------------------------------------------------------------
 *  gfx_get_compression_offset
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_compression_offset(void)
{
	unsigned long offset = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		offset = gu1_get_compression_offset();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		offset = gu2_get_compression_offset();
	#endif
	return(offset);
}

/*---------------------------------------------------------------------------
 * gfx_get_compression_pitch
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_compression_pitch(void)
{
	unsigned short pitch = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		pitch = gu1_get_compression_pitch();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		pitch = gu2_get_compression_pitch();
	#endif
	return(pitch);
}

/*---------------------------------------------------------------------------
 * gfx_get_compression_size
 *---------------------------------------------------------------------------
 */
unsigned short gfx_get_compression_size(void)
{
	unsigned short size = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		size = gu1_get_compression_size();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		size = gu2_get_compression_size();
	#endif
	return(size);
}

/*---------------------------------------------------------------------------
 * gfx_get_valid_bit
 *---------------------------------------------------------------------------
 */
int gfx_get_valid_bit(int line)
{
	int valid = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		valid = gu1_get_valid_bit(line);
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		valid = gu2_get_valid_bit(line);
	#endif
	return(valid);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_offset
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_display_video_offset(void)
{
	unsigned long offset = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		offset = gu1_get_display_video_offset();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		offset = gu2_get_display_video_offset();
	#endif
	return(offset);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_size
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_display_video_size(void)
{
	unsigned long size = 0;
	#if GFX_DISPLAY_GU1
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU1)
		size = gu1_get_display_video_size();
	#endif
	#if GFX_DISPLAY_GU2
	if (gfx_display_type & GFX_DISPLAY_TYPE_GU2)
		size = gu2_get_display_video_size();
	#endif
	return(size);
}

#endif /* GFX_READ_ROUTINES */

#endif /* GFX_DISPLAY_DYNAMIC */

/* END OF FILE */
