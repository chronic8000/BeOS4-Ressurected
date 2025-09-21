/*-----------------------------------------------------------------------------
 * DURANGO.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This is the main file used to add Durango graphics support to a software 
 * project.  The main reason to have a single file include the other files
 * is that it centralizes the location of the compiler options.  This file
 * should be tuned for a specific implementation, and then modified as needed
 * for new Durango releases.  The releases.txt file indicates any updates to
 * this main file, such as a new definition for a new hardware platform. 
 *
 * In other words, this file should be copied from the Durango source files
 * once when a software project starts, and then maintained as necessary.  
 * It should not be recopied with new versions of Durango unless the 
 * developer is willing to tune the file again for the specific project.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* COMPILER OPTIONS
 * These compiler options specify how the Durango routines are compiled 
 * for the different hardware platforms.  For best performance, a driver 
 * would build for a specific platform.  The "dynamic" switches are set 
 * by diagnostic applications such as Darwin that will run on a variety
 * of platforms and use the appropriate code at runtime.  Each component
 * may be separately dynamic, so that a driver has the option of being 
 * tuned for a specific 2D accelerator, but will still run with a variety
 * of chipsets. 
 */
#define GFX_DISPLAY_DYNAMIC			1	/* runtime selection */
#define GFX_DISPLAY_GU1				1	/* 1st generation display controller */
#define GFX_DISPLAY_GU2				1	/* 2nd generation display controller */

#define GFX_2DACCEL_DYNAMIC			1	/* runtime selection */			
#define GFX_2DACCEL_GU1				1	/* 1st generation 2D accelerator */
#define GFX_2DACCEL_GU2				1   /* 2nd generation 2D accelerator */

#define GFX_VIDEO_DYNAMIC			1	/* runtime selection */
#define GFX_VIDEO_CS5530			1   /* support for CS5530 */
#define GFX_VIDEO_SC1400			1   /* support for SC1400 */
#define GFX_VIDEO_SC1200			1   /* support for SC1200 */

#define GFX_VIP_DYNAMIC				1	/* runtime selection */
#define GFX_VIP_SC1400				1   /* support for SC1400 */
#define GFX_VIP_SC1200				1   /* support for SC1200 */

#define GFX_DECODER_DYNAMIC			1	/* runtime selection */
#define GFX_DECODER_SAA7114			1	/* Philips SAA7114 decoder */

#define GFX_TV_DYNAMIC				1	/* runtime selection */
#define GFX_TV_GEODE				1   /* Geode integrated TV encoder */
#define GFX_TV_FS451				1   /* Focus Enhancements FS451 */

#define GFX_I2C_DYNAMIC				1	/* runtime selection */
#define GFX_I2C_ACCESS				1   /* support for ACCESS.BUS */
#define GFX_I2C_GPIO				1   /* support for CS5530 GPIOs */

#define GFX_VGA_DYNAMIC				1	/* runtime selection */
#define GFX_VGA_GU1					1   /* 1st generation graphics unit */

/* ROUTINES TO READ VALUES
 * These are routines used by Darwin or other diagnostics to read the 
 * current state of the hardware.  Display drivers or embedded applications can 
 * reduce the size of the Durango code by not including these routines. 
 */
#define GFX_READ_ROUTINES			1	/* add routines to read values */

/* VARIABLES USED FOR RUNTIME SELECTION
 * If part of the graphics subsystem is declared as dynamic, then the 
 * following variables are used to specify which platform has been detected.
 * The varibles are set in the "gfx_detect_cpu" routine.  The values should 
 * be bit flags to allow masks to be used to check for multiple platforms.
 */

#if GFX_DISPLAY_DYNAMIC
int gfx_display_type = 0;
#endif

#if GFX_2DACCEL_DYNAMIC
int gfx_2daccel_type = 0;
#endif

#if GFX_VIDEO_DYNAMIC
int gfx_video_type = 0;
#endif

#if GFX_VIP_DYNAMIC
int gfx_vip_type = 0;
#endif

#if GFX_DECODER_DYNAMIC
int gfx_decoder_type = 0;
#endif

#if GFX_TV_DYNAMIC
int gfx_tv_type = 0;
#endif

#if GFX_I2C_DYNAMIC
int gfx_i2c_type = 0;
#endif

#if GFX_VGA_DYNAMIC
int gfx_vga_type = 0;
#endif

/* HEADER FILE FOR DURANGO ROUTINE DEFINITIONS
 * Needed since some of the Durango routines call other Durango routines.
 * Also defines the size of chipset array (GFX_CSPTR_SIZE).
 */
#include "gfx_rtns.h"		/* routine definitions */

/* DEFINE POINTERS TO MEMORY MAPPED REGIONS
 * These pointers are used by the Durango routines to access the hardware. 
 * The variables must be set by the project's initialization code after
 * mapping the regions in the appropriate manner. 
 */
unsigned char *gfx_regptr = (unsigned char *) 0x40000000;
unsigned char *gfx_fbptr = (unsigned char *) 0x40800000;
unsigned char *gfx_vidptr = (unsigned char *) 0x40010000;
unsigned char *gfx_vipptr = (unsigned char *) 0x40015000;

/* HEADER FILE FOR GRAPHICS REGISTER DEFINITIONS 
 * This contains only constant definitions, so it should be able to be 
 * included in any software project as is.
 */
#include "gfx_regs.h"		/* graphics register definitions */

/* HEADER FILE FOR REGISTER ACCESS MACROS
 * This file contains the definitions of the WRITE_REG32 and similar macros
 * used by the Durango routines to access the hardware.  The file assumes 
 * that the environment can handle 32-bit pointer access.  If this is not
 * the case, or if there are special requirements, then this header file 
 * should not be included and the project must define the macros itself.
 * (A project may define WRITE_REG32 to call a routine, for example).
 */	
#include "gfx_defs.h"		/* register access macros */

/* IO MACROS AND ROUTINES
 * These macros must be defined before the initialization or I2C 
 * routines will work properly. 
 */

unsigned char gfx_inb(unsigned short port)
{
	/* ADD OS SPECIFIC IMPLEMENTATION */
	return(0);
}

unsigned long gfx_ind(unsigned short port)
{
	/* ADD OS SPECIFIC IMPLEMENTATION */
	return(0);
}

void gfx_outb(unsigned short port, unsigned char data)
{
	/* ADD OS SPECIFIC IMPLEMENTATION */
}

void gfx_outd(unsigned short port, unsigned long data)
{
	/* ADD OS SPECIFIC IMPLEMENTATION */
}

#define INB(port) gfx_inb(port)
#define IND(port) gfx_ind(port)
#define OUTB(port, data) gfx_outd(port, data)
#define OUTD(port, data) gfx_outd(port, data)

/* INITIALIZATION ROUTINES 
 * These routines are used during the initialization of the driver to 
 * perform such tasks as detecting the type of CPU and video hardware.  
 * The routines require the use of IO, so the above IO routines need 
 * to be implemented before the initialization routines will work
 * properly.
 */

#include "gfx_init.c"    

/* INCLUDE DISPLAY CONTROLLER ROUTINES 
 * These routines are used if the display mode is set directly.  If the 
 * project uses VGA registers to set a display mode, then these files
 * do not need to be included.
 */
#include "gfx_mode.h"		/* display mode tables */
#include "gfx_disp.c"		/* display controller routines */

/* INCLUDE GRAPHICS ENGINE ROUTINES 
 * These routines are used to program the 2D graphics accelerator.  If
 * the project does not use graphics acceleration (direct frame buffer
 * access only), then this file does not need to be included. 
 */
#include "gfx_rndr.c"		/* graphics engine routines */

/* INCLUDE VIDEO OVERLAY ROUTINES
 * These routines control the video overlay hardware. 
 */
#include "gfx_vid.c"		/* video overlay routines */

/* VIDEO PORT AND VIDEO DECODER ROUTINES
 * These routines rely on the I2C routines.
 */
#include "gfx_vip.c"		/* video port routines */
#include "gfx_dcdr.c"		/* video decoder routines */

/* I2C BUS ACCESS ROUTINES
 * These routines are used by the video decoder and possibly an 
 * external TV encoer. 
 */
#include "gfx_i2c.c"		/* I2C bus access routines */

/* TV ENCODER ROUTINES
 * This file does not need to be included if the system does not
 * support TV output.
 */
#include "gfx_tv.c"		    /* TV encoder routines */

/* VGA ROUTINES
 * This file is used if setting display modes using VGA registers.
 */
#include "gfx_vga.c"		/* VGA routines */

/* END OF FILE */
