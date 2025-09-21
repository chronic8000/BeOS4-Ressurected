/*-----------------------------------------------------------------------------
 * GFX_VIP.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to control the video input port (VIP).
 * 
 *    gfx_set_vip_enable
 *    gfx_set_vip_base
 *    gfx_set_vip_pitch
 *    gfx_set_vbi_enable
 *    gfx_set_vbi_base
 *    gfx_set_vbi_pitch
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *
 *    gfx_get_vip_enable
 *    gfx_get_vip_base
 *    gfx_get_vip_pitch
 *    gfx_get_vbi_enable
 *    gfx_get_vbi_base
 *    gfx_get_vbi_pitch
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* INCLUDE SUPPORT FOR SC1400, IF SPECIFIED. */

#if GFX_VIP_SC1400
#include "vip_1400.c"
#endif

/* INCLUDE SUPPORT FOR SC1200, IF SPECIFIED. */

#if GFX_VIP_SC1200
#include "vip_1200.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either CS5530, SC1400, or SC1200 routines. */

#if GFX_VIP_DYNAMIC

/*-----------------------------------------------------------------------------
 * gfx_set_vip_enable
 *-----------------------------------------------------------------------------
 */
int gfx_set_vip_enable(int enable)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vip_enable(enable);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vip_enable(enable);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_vip_base
 *-----------------------------------------------------------------------------
 */
int gfx_set_vip_base(unsigned long even, unsigned long odd)
{	
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vip_base(even, odd);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vip_base(even, odd);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_vip_pitch
 *-----------------------------------------------------------------------------
 */
int gfx_set_vip_pitch(unsigned long pitch)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vip_pitch(pitch);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vip_pitch(pitch);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_vip_enable
 *-----------------------------------------------------------------------------
 */
int gfx_set_vbi_enable(int enable)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vbi_enable(enable);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vbi_enable(enable);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_vbi_base
 *-----------------------------------------------------------------------------
 */
int gfx_set_vbi_base(unsigned long even, unsigned long odd)
{	
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vbi_base(even, odd);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vbi_base(even, odd);
	#endif
	return(status);
}

/*-----------------------------------------------------------------------------
 * gfx_set_vbi_pitch
 *-----------------------------------------------------------------------------
 */
int gfx_set_vbi_pitch(unsigned long pitch)
{
	int status = GFX_STATUS_UNSUPPORTED;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		status = sc1400_set_vbi_pitch(pitch);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		status = sc1200_set_vbi_pitch(pitch);
	#endif
	return(status);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*-----------------------------------------------------------------------------
 * gfx_get_vip_enable
 *-----------------------------------------------------------------------------
 */
int gfx_get_vip_enable(void)
{
	int enable = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		enable = sc1400_get_vip_enable();
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		enable = sc1200_get_vip_enable();
	#endif
	return(enable);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vip_base
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vip_base(int odd)
{
	unsigned long base = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		base = sc1400_get_vip_base(odd);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		base = sc1200_get_vip_base(odd);
	#endif
	return(base);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vip_pitch
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vip_pitch(void)
{
	unsigned long pitch = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		pitch = sc1400_get_vip_pitch();
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		pitch = sc1200_get_vip_pitch();
	#endif
	return(pitch);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vbi_enable
 *-----------------------------------------------------------------------------
 */
int gfx_get_vbi_enable(void)
{
	int enable = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		enable = sc1400_get_vbi_enable();
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		enable = sc1200_get_vbi_enable();
	#endif
	return(enable);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vbi_base
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vbi_base(int odd)
{
	unsigned long base = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		base = sc1400_get_vbi_base(odd);
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		base = sc1200_get_vbi_base(odd);
	#endif
	return(base);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vbi_pitch
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vbi_pitch(void)
{
	unsigned long pitch = 0;
	#if GFX_VIP_SC1400
	if (gfx_vip_type == GFX_VIP_TYPE_SC1400)
		pitch = sc1400_get_vbi_pitch();
	#endif
	#if GFX_VIP_SC1200
	if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
		pitch = sc1200_get_vbi_pitch();
	#endif
	return(pitch);
}

#endif /* GFX_READ_ROUTINES */

#endif /* GFX_VIP_DYNAMIC */

/* END OF FILE */

