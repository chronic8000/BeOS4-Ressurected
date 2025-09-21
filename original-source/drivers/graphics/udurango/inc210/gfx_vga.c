/*-----------------------------------------------------------------------------
 * GFX_VGA.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines to interface to the VGA registers.  Some
 * operating systems require mode sets be done through VGA, rather than
 * directly using the "gfx_set_display_mode" routine.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* INCLUDE SUPPORT FOR FIRST GENERATION, IF SPECIFIED. */

#if GFX_VGA_GU1
#include "vga_gu1.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either first or second generation routines. */

#if GFX_VGA_DYNAMIC

#endif /* GFX_DISPLAY_DYNAMIC */

/* END OF FILE */
