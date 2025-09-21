/*---------------------------------------------------------------------------
 * GFX_RNDR.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to program the 2D acceleration hardware:
 * 
 *    gfx_set_bpp          
 *    gfx_set_blt_buffers   
 *    gfx_set_solid_pattern  
 *    gfx_set_mono_pattern
 *    gfx_set_solid_source  
 *    gfx_set_mono_source
 *    gfx_set_raster_operation
 *    gfx_pattern_fill
 *    gfx_screen_to_screen_blt
 *    gfx_screen_to_screen_xblt
 *    gfx_color_bitmap_to_screen_blt
 *    gfx_color_bitmap_to_screen_xblt
 *    gfx_mono_bitmap_to_screen_blt
 *    gfx_bresenham_line 
 *    gfx_wait_until_idle   
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */
           
/* STATIC VARIABLES */

unsigned short GFXbpp = 16;
unsigned short GFXbb0Base = 0x800;
unsigned short GFXbb1Base = 0xB30;
unsigned short GFXbufferWidthPixels = 400;

unsigned short GFXpatternFlags = 0;
unsigned short GFXsourceFlags = 0;
unsigned long GFXsavedColor = 0;
unsigned short GFXsavedRop = 0;
unsigned short GFXusesDstData = 0;

/* INCLUDE SUPPORT FOR FIRST GENERATION, IF SPECIFIED. */

#if GFX_2DACCEL_GU1
#include "rndr_gu1.c"
#endif

/* INCLUDE SUPPORT FOR SECOND GENERATION, IF SPECIFIED. */

#if GFX_2DACCEL_GU2
#include "rndr_gu2.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either first or second generation routines. */

#if GFX_2DACCEL_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_set_bpp
 *---------------------------------------------------------------------------
 */
void gfx_set_bpp(unsigned short bpp)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_bpp(bpp);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_bpp(bpp);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_solid_source
 *---------------------------------------------------------------------------
 */
void gfx_set_solid_source(unsigned long color)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_solid_source(color);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_solid_source(color);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_mono_source
 *---------------------------------------------------------------------------
 */
void gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
	unsigned short transparent)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_mono_source(bgcolor, fgcolor, transparent);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_mono_source(bgcolor, fgcolor, transparent);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_solid_pattern
 *---------------------------------------------------------------------------
 */
void gfx_set_solid_pattern(unsigned long color)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_solid_pattern(color);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_solid_pattern(color);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_mono_pattern
 *---------------------------------------------------------------------------
 */
void gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor, 
	unsigned long data0, unsigned long data1, unsigned char transparent)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_mono_pattern(bgcolor, fgcolor, data0, data1, transparent);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_mono_pattern(bgcolor, fgcolor, data0, data1, transparent);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_raster_operation
 *---------------------------------------------------------------------------
 */
void gfx_set_raster_operation(unsigned char rop)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_set_raster_operation(rop);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_set_raster_operation(rop);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_pattern_fill
 *---------------------------------------------------------------------------
 */
void gfx_pattern_fill(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_pattern_fill(x, y, width, height);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_pattern_fill(x, y, width, height);
	#endif
}
	
/*---------------------------------------------------------------------------
 * gfx_screen_to_screen_blt
 *---------------------------------------------------------------------------
 */
void gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_screen_to_screen_blt(srcx, srcy, dstx, dsty, width, height);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_screen_to_screen_blt(srcx, srcy, dstx, dsty, width, height);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_screen_to_screen_xblt
 *---------------------------------------------------------------------------
 */
void gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned long color)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_screen_to_screen_xblt(srcx, srcy, dstx, dsty, width, height, color);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_screen_to_screen_xblt(srcx, srcy, dstx, dsty, width, height, color);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_color_bitmap_to_screen_blt
 *---------------------------------------------------------------------------
 */
void gfx_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_color_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
			data, pitch);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_color_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
			data, pitch);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_color_bitmap_to_screen_xblt
 *---------------------------------------------------------------------------
 */
void gfx_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch, 
	unsigned long color)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_color_bitmap_to_screen_xblt(srcx, srcy, dstx, dsty, width, height,
			data, pitch, color);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_color_bitmap_to_screen_xblt(srcx, srcy, dstx, dsty, width, height,
			data, pitch, color);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_mono_bitmap_to_screen_blt
 *---------------------------------------------------------------------------
 */
void gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_mono_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
			data, pitch);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_mono_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
			data, pitch);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_bresenham_line
 *---------------------------------------------------------------------------
 */
void gfx_bresenham_line(unsigned short x, unsigned short y, 
		unsigned short length, unsigned short initerr, 
		unsigned short axialerr, unsigned short diagerr, 
		unsigned short flags)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_bresenham_line(x, y, length, initerr, axialerr, diagerr, flags);
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_bresenham_line(x, y, length, initerr, axialerr, diagerr, flags);
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_wait_until_idle
 *---------------------------------------------------------------------------
 */
void gfx_wait_until_idle(void)
{
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		gu1_wait_until_idle();
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		gu2_wait_until_idle();
	#endif
}

/*---------------------------------------------------------------------------
 * gfx_test_blt_pending
 *---------------------------------------------------------------------------
 */
int gfx_test_blt_pending(void)
{
	int retval = 0;
	#if GFX_2DACCEL_GU1
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
		retval = gu1_test_blt_pending();
	#endif
	#if GFX_2DACCEL_GU2
	if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
		retval = gu2_test_blt_pending();
	#endif
	return(retval);
}

#endif /* GFX_2DACCEL_DYNAMIC */

/* END OF FILE */

