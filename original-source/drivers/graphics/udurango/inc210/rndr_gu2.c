/*---------------------------------------------------------------------------
 * RNDR_GU2.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to program the 2D acceleration hardware for
 * the second generation graphics unit (???).
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
 *    Not started.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */
           
/*---------------------------------------------------------------------------
 * GFX_SET_BPP
 *
 * This routine sets the bits per pixel value in the graphics engine.
 * It is also stored in a static variable to use in the future calls to 
 * the rendering routines.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void gu2_set_bpp(unsigned short bpp)
#else
void gfx_set_bpp(unsigned short bpp)
#endif
{
	GFXbpp = bpp;
}

/*
//---------------------------------------------------------------------------
// GFX_SET_SOLID_SOURCE
//
// This routine is used to specify a solid source color.  For the Xfree96
// display driver, the source color is used to specify a planemask and the 
// ROP is adjusted accordingly.
//---------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_set_solid_source(unsigned long color)
#else
void gfx_set_solid_source(unsigned long color)
#endif
{
	/* CLEAR TRANSPARENCY FLAG */

	GFXsourceFlags = 0;

	/* ### ADD ### IMPLEMENTATION */
}

/*
//---------------------------------------------------------------------------
// GFX_SET_MONO_SOURCE
//
// This routine is used to specify the monochrome source colors.  
// It must be called *after* loading any pattern data (those routines 
// clear the source flags).
//---------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
	unsigned short transparent)
#else
void gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
	unsigned short transparent)
#endif
{
	/* SET TRANSPARENCY FLAG */

	GFXsourceFlags = transparent ? 1 : 0;

	/* ### ADD ### IMPLEMENTATION */
}

/*
//---------------------------------------------------------------------------
// GFX_SET_SOLID_PATTERN
//
// This routine is used to specify a solid pattern color.  It is called 
// before performing solid rectangle fills or more complicated BLTs that 
// use a solid pattern color. 
//
// The driver should always call "gfx_load_raster_operation" after a call 
// to this routine to make sure that the pattern flags are set appropriately.
//---------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_set_solid_pattern(unsigned long color)
#else
void gfx_set_solid_pattern(unsigned long color)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

	/* SET PATTERN FLAGS */

	GFXpatternFlags = 0;

	/* ### ADD ### IMPLEMENTATION */
}

/*
//---------------------------------------------------------------------------
// GFX_SET_MONO_PATTERN
//
// This routine is used to specify a monochrome pattern. 
//---------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor, 
	unsigned long data0, unsigned long data1, unsigned char transparent)
#else
void gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor, 
	unsigned long data0, unsigned long data1, unsigned char transparent)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

	/* SET PATTERN FLAGS */

	/* ### ADD ### IMPLEMENTATION */
}

/*
//---------------------------------------------------------------------------
// GFX_SET_RASTER_OPERATION
//
// This routine loads the specified raster operation.  It sets the pattern
// flags appropriately.
//---------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_set_raster_operation(unsigned char rop)
#else
void gfx_set_raster_operation(unsigned char rop)
#endif
{
	/* SAVE ROP FOR LATER COMPARISONS */

	GFXsavedRop = rop;
	
	/* SET FLAG INDICATING ROP REQUIRES DESTINATION DATA */
	/* True if even bits (0:2:4:6) do not equal the correspinding */
	/* even bits (1:3:5:7). */

	GFXusesDstData = ((rop & 0x55) ^ ((rop >> 1) & 0x55));

	/* ### ADD ### IMPLEMENTATION */
}

/*
//----------------------------------------------------------------------------
// GFX_PATTERN_FILL
//
// This routine is used to fill a rectangular region.  The pattern must 
// be previously loaded using one of GFX_load_*_pattern routines.  Also, the 
// raster operation must be previously specified using the 
// "GFX_load_raster_operation" routine.
//
//      X               screen X position (left)
//      Y               screen Y position (top)
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_pattern_fill(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
#else
void gfx_pattern_fill(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// SCREEN TO SCREEN BLT
//
// This routine should be used to perform a screen to screen BLT when the 
// ROP does not require destination data.
//
//      SRCX            screen X position to copy from
//      SRCY            screen Y position to copy from
//      DSTX            screen X position to copy to
//      DSTY            screen Y position to copy to
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height)
#else
void gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// SCREEN TO SCREEN TRANSPARENT BLT
//
// This routine should be used to perform a screen to screen BLT when a 
// specified color should by transparent.  The only supported ROP is SRCCOPY.
//
//      SRCX            screen X position to copy from
//      SRCY            screen Y position to copy from
//      DSTX            screen X position to copy to
//      DSTY            screen Y position to copy to
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//      COLOR           transparent color
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned long color)
#else
void gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned long color)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// COLOR BITMAP TO SCREEN BLT
//
// This routine transfers color bitmap data to the screen.  For most cases,
// when the ROP is SRCCOPY, it may be faster to write a separate routine that
// copies the data to the frame buffer directly.  This routine should be 
// used when the ROP requires destination data.
//
// Transparency is handled by another routine.
//
//      SRCX            X offset within source bitmap
//      SRCY            Y offset within source bitmap
//      DSTX            screen X position to render data
//      DSTY            screen Y position to render data
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//      *DATA           pointer to bitmap data
//      PITCH           pitch of bitmap data (bytes between scanlines)
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
#else
void gfx_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// COLOR BITMAP TO SCREEN TRANSPARENT BLT
//
// This routine transfers color bitmap data to the screen with transparency.
// The transparent color is specified.  The only supported ROP is SRCCOPY, 
// meaning that transparency cannot be applied if the ROP requires 
// destination data (this is a hardware restriction).
//
//      SRCX            X offset within source bitmap
//      SRCY            Y offset within source bitmap
//      DSTX            screen X position to render data
//      DSTY            screen Y position to render data
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//      *DATA           pointer to bitmap data
//      PITCH           pitch of bitmap data (bytes between scanlines)
//      COLOR           transparent color
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch, 
	unsigned long color)
#else
void gfx_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch, 
	unsigned long color)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// MONOCHROME BITMAP TO SCREEN BLT
//
// This routine transfers monochrome bitmap data to the screen.  
//
//      SRCX            X offset within source bitmap
//      SRCY            Y offset within source bitmap
//      DSTX            screen X position to render data
//      DSTY            screen Y position to render data
//      WIDTH           width of rectangle, in pixels
//      HEIGHT          height of rectangle, in scanlines
//      *DATA           pointer to bitmap data
//      PITCH           pitch of bitmap data (bytes between scanlines)
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
#else
void gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}    

/*
//----------------------------------------------------------------------------
// BRESENHAM LINE
//
// This routine draws a vector using the specified Bresenham parameters.  
// Currently this file does not support a routine that accepts the two 
// endpoints of a vector and calculates the Bresenham parameters.  If it 
// ever does, this routine is still required for vectors that have been 
// clipped.
//
//      X               screen X position to start vector
//      Y               screen Y position to start vector
//      LENGTH          length of the vector, in pixels
//      INITERR         Bresenham initial error term
//      AXIALERR        Bresenham axial error term
//      DIAGERR         Bresenham diagonal error term
//      FLAGS           VM_YMAJOR, VM_MAJOR_INC, VM_MINOR_INC
//----------------------------------------------------------------------------
*/
#if GFX_2DACCEL_DYNAMIC
void gu2_bresenham_line(unsigned short x, unsigned short y, 
		unsigned short length, unsigned short initerr, 
		unsigned short axialerr, unsigned short diagerr, 
		unsigned short flags)
#else
void gfx_bresenham_line(unsigned short x, unsigned short y, 
		unsigned short length, unsigned short initerr, 
		unsigned short axialerr, unsigned short diagerr, 
		unsigned short flags)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * GFX_WAIT_UNTIL_IDLE
 *
 * This routine waits until the graphics engine is idle.  This is required
 * before allowing direct access to the frame buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void gu2_wait_until_idle(void)
#else
void gfx_wait_until_idle(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
}

/*---------------------------------------------------------------------------
 * GFX_TEST_BLT_PENDING
 *
 * This routine returns 1 if a BLT is pending, meaning that a call to 
 * perform a rendering operation would stall.  Otherwise it returns 0.
 * It is used by Darwin during random testing to only start a BLT 
 * operation when it knows the Durango routines won't spin on graphics
 * (so Darwin can continue to do frame buffer reads and writes).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
int gu2_test_blt_pending(void)
#else
int gfx_test_blt_pending(void)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/* END OF FILE */

