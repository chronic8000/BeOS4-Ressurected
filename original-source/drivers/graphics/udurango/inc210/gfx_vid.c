/*-----------------------------------------------------------------------------
 * GFX_VID.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the video overlay window.
 *
 * Video overlay routines:
 * 
 *    gfx_set_clock_frequency
 *    gfx_set_video_enable
 *    gfx_set_video_format
 *    gfx_set_video_size
 *    gfx_set_video_offset
 *    gfx_set_video_scale
 *    gfx_set_video_window
 *    gfx_set_video_color_key
 *    gfx_set_video_filter
 *    gfx_set_video_palette
 * 
 * Alpha blending routines (SC1200 ONLY):
 *
 *    gfx_select_alpha_region
 *	  gfx_set_alpha_enable
 *	  gfx_set_alpha_size
 *    gfx_set_alpha_value
 *    gfx_set_alpha_priority
 *    gfx_set_alpha_color
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *
 *    gfx_get_sync_polarities
 *    gfx_get_video_enable
 *    gfx_get_video_format
 *    gfx_get_video_src_size
 *    gfx_get_video_line_size
 *    gfx_get_video_xclip
 *    gfx_get_video_offset
 *    gfx_get_video_scale
 *    gfx_get_video_dst_size
 *    gfx_get_video_position
 *    gfx_get_video_color_key
 *    gfx_get_video_color_key_mask
 *    gfx_get_video_color_key_src
 *    gfx_get_video_filter
 *    gfx_get_clock_frequency
 *    gfx_read_crc
 *
 * Alpha blending read routines (SC1200 ONLY):
 *
 *    gfx_get_alpha_enable
 *    gfx_get_alpha_size
 *    gfx_get_alpha_value
 *    gfx_get_alpha_priority
 *    gfx_get_alpha_color
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* STATIC VARIABLES FOR VIDEO OVERLAY CONTROL */
/* These are saved to allow these routines to do clipping. */

unsigned long gfx_vid_offset = 0;   /* copy from last gfx_set_video_offset */
unsigned short gfx_vid_srcw = 0;	/* copy from last gfx_set_video_scale  */
unsigned short gfx_vid_srch = 0;	/* copy from last gfx_set_video_scale  */
unsigned short gfx_vid_dstw = 0;	/* copy from last gfx_set_video_scale  */
unsigned short gfx_vid_dsth = 0;	/* copy from last gfx_set_video_scale  */
short gfx_vid_xpos = 0;				/* copy from last gfx_set_video_window */
short gfx_vid_ypos = 0;				/* copy from last gfx_set_video_window */
unsigned short gfx_vid_width = 0;	/* copy from last gfx_set_video_window */
unsigned short gfx_vid_height = 0;	/* copy from last gfx_set_video_window */

int gfx_alpha_select = 0;			/* currently selected alpha region */

/* INCLUDE SUPPORT FOR CS5530, IF SPECIFIED. */

#if GFX_VIDEO_CS5530
#include "vid_5530.c"
#endif

/* INCLUDE SUPPORT FOR SC1400, IF SPECIFIED. */

#if GFX_VIDEO_SC1400
#include "vid_1400.c"
#endif

/* INCLUDE SUPPORT FOR SC1200, IF SPECIFIED. */

#if GFX_VIDEO_SC1200
#include "vid_1200.c"
#endif

/*---------------------------------------------------------------------------
 * gfx_select_alpha_region
 * 
 * This routine selects which alpha region should be used for future 
 * updates.  The SC1200, for example, has 3 alpha windows available.
 *---------------------------------------------------------------------------
 */

int gfx_select_alpha_region(int region)
{
	gfx_alpha_select = region;
	return(GFX_STATUS_OK);
}

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either CS5530, SC1400, or SC1200 routines. */

#if GFX_VIDEO_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
void gfx_reset_video(void)
{
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		cs5530_reset_video();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		sc1400_reset_video();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_reset_video();
	#endif
}

/*-----------------------------------------------------------------------------
 * gfx_set_clock_frequency
 *-----------------------------------------------------------------------------
 */
void gfx_set_clock_frequency(unsigned long frequency)
{
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		cs5530_set_clock_frequency(frequency);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		sc1400_set_clock_frequency(frequency);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_set_clock_frequency(frequency);
	#endif
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_enable
 *-----------------------------------------------------------------------------
 */
int gfx_set_video_enable(int enable)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_enable(enable);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_enable(enable);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_enable(enable);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_format
 *-----------------------------------------------------------------------------
 */
int gfx_set_video_format(unsigned long format)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_format(format);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_format(format);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_format(format);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_size
 *-----------------------------------------------------------------------------
 */
int gfx_set_video_size(unsigned short width, unsigned short height)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_size(width, height);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_size(width, height);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_size(width, height);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_video_offset
 *-----------------------------------------------------------------------------
 */
int gfx_set_video_offset(unsigned long offset)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_offset(offset);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_offset(offset);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_offset(offset);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_scale
 *---------------------------------------------------------------------------
 */
int gfx_set_video_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_scale(srcw, srch, dstw, dsth);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_scale(srcw, srch, dstw, dsth);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_scale(srcw, srch, dstw, dsth);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_window
 *---------------------------------------------------------------------------
 */
int gfx_set_video_window(short x, short y, unsigned short w, 
	unsigned short h)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_window(x, y, w, h);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_window(x, y, w, h);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_window(x, y, w, h);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_color_key
 *---------------------------------------------------------------------------
 */
int gfx_set_video_color_key(unsigned long key, unsigned long mask, 
	int graphics)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_color_key(key, mask, graphics);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_color_key(key, mask, graphics);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_color_key(key, mask, graphics);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_filter
 *---------------------------------------------------------------------------
 */
int gfx_set_video_filter(int xfilter, int yfilter)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_filter(xfilter, yfilter);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_filter(xfilter, yfilter);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_filter(xfilter, yfilter);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 *---------------------------------------------------------------------------
 */
int gfx_set_video_palette(unsigned long *palette)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		status = cs5530_set_video_palette(palette);
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		status = sc1400_set_video_palette(palette);
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_video_palette(palette);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_enable
 *---------------------------------------------------------------------------
 */
int gfx_set_alpha_enable(int enable)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_alpha_enable(enable);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_size
 *---------------------------------------------------------------------------
 */
int gfx_set_alpha_size(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_alpha_size(x, y, width, height);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_value
 *---------------------------------------------------------------------------
 */
int gfx_set_alpha_value(unsigned char alpha, char delta)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_alpha_value(alpha, delta);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_priority
 *---------------------------------------------------------------------------
 */
int gfx_set_alpha_priority(int priority)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_alpha_priority(priority);
	#endif
	return(status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color
 *---------------------------------------------------------------------------
 */
int gfx_set_alpha_color(unsigned long color)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		status = sc1200_set_alpha_color(color);
	#endif
	return(status);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_sync_polarities
 *---------------------------------------------------------------------------
 */
int gfx_get_sync_polarities(void)
{
	int polarities = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		polarities = cs5530_get_sync_polarities();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		polarities = sc1400_get_sync_polarities();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		polarities = sc1200_get_sync_polarities();
	#endif
	return(polarities);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_enable
 *-----------------------------------------------------------------------------
 */
int gfx_get_video_enable(void)
{
	int enable = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		enable = cs5530_get_video_enable();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		enable = sc1400_get_video_enable();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		enable = sc1200_get_video_enable();
	#endif
	return(enable);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_format
 *-----------------------------------------------------------------------------
 */
int gfx_get_video_format(void)
{
	int format = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		format = cs5530_get_video_format();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		format = sc1400_get_video_format();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		format = sc1200_get_video_format();
	#endif
	return(format);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_src_size
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_video_src_size(void)
{
	unsigned long size = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		size = cs5530_get_video_src_size();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		size = sc1400_get_video_src_size();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		size = sc1200_get_video_src_size();
	#endif
	return(size);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_line_size
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_video_line_size(void)
{
	unsigned long size = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		size = cs5530_get_video_line_size();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		size = sc1400_get_video_line_size();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		size = sc1200_get_video_line_size();
	#endif
	return(size);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_xclip
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_video_xclip(void)
{
	unsigned long size = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		size = cs5530_get_video_xclip();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		size = sc1400_get_video_xclip();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		size = sc1200_get_video_xclip();
	#endif
	return(size);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_offset
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_video_offset(void)
{
	unsigned long offset = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		offset = cs5530_get_video_offset();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		offset = sc1400_get_video_offset();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		offset = sc1200_get_video_offset();
	#endif
	return(offset);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_scale
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_video_scale(void)
{
	unsigned long scale = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		scale = cs5530_get_video_scale();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		scale = sc1400_get_video_scale();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		scale = sc1200_get_video_scale();
	#endif
	return(scale);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_video_dst_size(void)
{
	unsigned long size = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		size = cs5530_get_video_dst_size();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		size = sc1400_get_video_dst_size();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		size = sc1200_get_video_dst_size();
	#endif
	return(size);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_video_position(void)
{
	unsigned long position = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		position = cs5530_get_video_position();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		position = sc1400_get_video_position();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		position = sc1200_get_video_position();
	#endif
	return(position);
}
	
/*---------------------------------------------------------------------------
 * gfx_get_video_color_key
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_video_color_key(void)
{
	unsigned long key = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		key = cs5530_get_video_color_key();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		key = sc1400_get_video_color_key();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		key = sc1200_get_video_color_key();
	#endif
	return(key);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_video_color_key_mask(void)
{
	unsigned long mask = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		mask = cs5530_get_video_color_key_mask();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		mask = sc1400_get_video_color_key_mask();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		mask = sc1200_get_video_color_key_mask();
	#endif
	return(mask);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 *---------------------------------------------------------------------------
 */
int gfx_get_video_color_key_src(void)
{
	int src = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		src = cs5530_get_video_color_key_src();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		src = sc1400_get_video_color_key_src();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		src = sc1200_get_video_color_key_src();
	#endif
	return(src);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_filter
 *---------------------------------------------------------------------------
 */
int gfx_get_video_filter(void)
{
	int filter = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		filter = cs5530_get_video_filter();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		filter = sc1400_get_video_filter();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		filter = sc1200_get_video_filter();
	#endif
	return(filter);
}

/*---------------------------------------------------------------------------
 * gfx_get_clock_frequency
 *---------------------------------------------------------------------------
 */
unsigned long gfx_get_clock_frequency(void)
{
	unsigned long frequency = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		frequency = cs5530_get_clock_frequency();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		frequency = sc1400_get_clock_frequency();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		frequency = sc1200_get_clock_frequency();
	#endif
	return(frequency);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc
 *---------------------------------------------------------------------------
 */
unsigned long gfx_read_crc(void)
{
	unsigned long crc = 0;
	#if GFX_VIDEO_CS5530
	if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
		crc = cs5530_read_crc();
	#endif
	#if GFX_VIDEO_SC1400
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1400)
		crc = sc1400_read_crc();
	#endif
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		crc = sc1200_read_crc();
	#endif
	return(crc);
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_enable
 *---------------------------------------------------------------------------
 */
void gfx_get_alpha_enable(int *enable)
{
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_get_alpha_enable(enable);
	#endif
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_size
 *---------------------------------------------------------------------------
 */
void gfx_get_alpha_size(unsigned short *x, unsigned short *y, 
	unsigned short *width, unsigned short *height)
{
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_get_alpha_size(x, y, width, height);
	#endif
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_value
 *---------------------------------------------------------------------------
 */
void gfx_get_alpha_value(unsigned char *alpha, char *delta)
{
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_get_alpha_value(alpha, delta);
	#endif
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_priority
 *---------------------------------------------------------------------------
 */
void gfx_get_alpha_priority(int *priority)
{
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_get_alpha_priority(priority);
	#endif
	return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_color
 *---------------------------------------------------------------------------
 */
void gfx_get_alpha_color(unsigned long *color)
{
	#if GFX_VIDEO_SC1200
	if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
		sc1200_get_alpha_color(color);
	#endif
	return;
}

#endif /* GFX_READ_ROUTINES */

#endif /* GFX_VIDEO_DYNAMIC */

/* END OF FILE */
