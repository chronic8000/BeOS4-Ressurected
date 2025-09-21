/*-----------------------------------------------------------------------------
 * GFX_TV.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to program the TV encoder.  
 *
 * Routines:
 * 
 *    gfx_set_tv_enable
 *    gfx_set_tv_format
 *    gfx_set_tv_cc_enable
 *    gfx_set_tv_cc_data
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* INCLUDE SUPPORT FOR GEODE TV ENCODER, IF SPECIFIED */

#if GFX_TV_GEODE
#include "tv_geode.c"
#endif

/* INCLUDE SUPPORT FOR FS451 TV ENCODER, IF SPECIFIED */

#if GFX_TV_FS451
#include "tv_fs451.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either Geode or FS451 TV encoder routines. */

#if GFX_I2C_DYNAMIC

/*-----------------------------------------------------------------------------
 * gfx_set_tv_enable
 *-----------------------------------------------------------------------------
 */
int gfx_set_tv_enable(int enable)
{
	int retval = -1;
#if GFX_TV_GEODE
	if (gfx_tv_type & GFX_TV_TYPE_GEODE)
		retval = geode_set_tv_enable(enable);
#endif 
#if GFX_TV_FS451
	if (gfx_tv_type & GFX_TV_TYPE_FS451)
		retval = fs451_set_tv_enable(enable);
#endif 
	return(retval);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_defaults
 *-----------------------------------------------------------------------------
 */
int gfx_set_tv_defaults(int format)
{
	int retval = -1;
#if GFX_TV_GEODE
	if (gfx_tv_type & GFX_TV_TYPE_GEODE)
		retval = geode_set_tv_defaults(format);
#endif 
#if GFX_TV_FS451
	if (gfx_tv_type & GFX_TV_TYPE_FS451)
		retval = fs451_set_tv_defaults(format);
#endif 
	return(retval);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_cc_enable
 *-----------------------------------------------------------------------------
 */
int gfx_set_tv_cc_enable(int enable)
{
	int retval = -1;
#if GFX_TV_GEODE
	if (gfx_tv_type & GFX_TV_TYPE_GEODE)
		retval = geode_set_tv_cc_enable(enable);
#endif 
#if GFX_TV_FS451
	if (gfx_tv_type & GFX_TV_TYPE_FS451)
		retval = fs451_set_tv_cc_enable(enable);
#endif 
	return(retval);
}

/*-----------------------------------------------------------------------------
 * gfx_set_tv_cc_data
 *
 * This routine writes the two specified characters to the CC data register 
 * of the TV encoder.
 *-----------------------------------------------------------------------------
 */
int gfx_set_tv_cc_data(unsigned char data1, unsigned char data2)
{
	int retval = -1;
#if GFX_TV_GEODE
	if (gfx_tv_type & GFX_TV_TYPE_GEODE)
		retval = geode_set_tv_cc_data(data1, data2);
#endif 
#if GFX_TV_FS451
	if (gfx_tv_type & GFX_TV_TYPE_FS451)
		retval = fs451_set_tv_cc_data(data1, data2);
#endif 
	return(retval);
}

#endif /* GFX_TV_DYNAMIC */

/* END OF FILE */
