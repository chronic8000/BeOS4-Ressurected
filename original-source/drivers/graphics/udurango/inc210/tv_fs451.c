/*-----------------------------------------------------------------------------
 * TV_FS451.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to program the FS451 TV encoder.
 *
 * History:
 *    Not started.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * gfx_set_tv_defaults
 *
 * This routine sets all of the TV encoder registers to default values for 
 * the specified format.  Currently only NTSC is supported.
 *-----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int fs451_set_tv_defaults(int format)
#else
int gfx_set_tv_defaults(int format)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_enable
 *
 * This routine enables or disables the TV output.
 *-----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int fs451_set_tv_enable(int enable)
#else
int gfx_set_tv_enable(int enable)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_cc_enable
 *
 * This routine enables or disables the use of the hardware CC registers 
 * in the TV encoder.
 *-----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int fs451_set_tv_cc_enable(int enable)
#else
int gfx_set_tv_cc_enable(int enable)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_cc_data
 *
 * This routine writes the two specified characters to the CC data register 
 * of the TV encoder.
 *-----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int fs451_set_tv_cc_data(unsigned char data1, unsigned char data2)
#else
int gfx_set_tv_cc_data(unsigned char data1, unsigned char data2)
#endif
{
	/* ### ADD ### IMPLEMENTATION */
	return(0);
}

/* END OF FILE */
