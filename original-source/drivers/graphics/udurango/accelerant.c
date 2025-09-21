#include <BeBuild.h>
#include <tv_out.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>

#include "durango_private.h"
#include <video_overlay.h>

#if DEBUG > 0
#ifndef SERIAL_DEBUG
#define xprintf(a) printf a
#define yprintf(a)
#else
extern void _kdprintf_(char *, ...);
#define xprintf(a) _kdprintf_ a
#define yprintf(a)
#endif
#if 0
//#define DWRITE_REG32(a, b, c) { xprintf((#b " P: %.08lx ", READ_REG32(a, b))); WRITE_REG32(a, b, c); xprintf(("W: %.08lx  R: %.08lx\n", c, READ_REG32(a, b))); }
#define DWRITE_REG32(a, b) { xprintf(("WRITE_REG32("#a ", 0x%.08lx);\n", b)); WRITE_REG32(a, b); }
//#define DWRITE_REG32(a, b) { xprintf(("WRITE_REG32("#a ", 0x%.08lx);\n", b)); WRITE_REG32(a, b); xprintf((" READ_REG32("#a", 0x%.08lx)\n\n", READ_REG32(a))); }
#define DWRITE_VID32(a, b) { xprintf(("WRITE_VID32("#a ", 0x%.08lx);\n", b)); WRITE_VID32(a, b); }
//#define DWRITE_VID32(a, b) { xprintf(("WRITE_VID32("#a ", 0x%.08lx);\n", b)); WRITE_VID32(a, b); xprintf((" READ_VID32("#a", 0x%.08lx)\n\n", READ_VID32(a))); }
#else
#define DWRITE_REG32(a, b) WRITE_REG32(a, b)
#define DWRITE_VID32(a, b) WRITE_VID32(a, b)
#endif
#else
#define xprintf(a)
#define yprintf(a)
#define DWRITE_REG32(a, b) WRITE_REG32(a, b)
#define DWRITE_VID32(a, b) WRITE_VID32(a, b)
#endif

#if CH7005_SUPPORT
#undef ENABLE_FLAT_PANEL
#define ENABLE_FLAT_PANEL 1
#define MASTER_MODE 1
#define DOUBLE_CLOCK 0
#define DOT_CRAWL_REDUCTION 0
#define POSITIVE_EDGE_SELECT 0
#define OVERRIDE_TV_SETTINGS 1
#else
#define DOUBLE_CLOCK 0
#endif

static accelerant_info	*ai;
static shared_info		*si;
static display_mode		*mode_list;
static uint32			mode_count;
static int				file_handle;
static engine_token durango_engine_token = { 1, B_2D_ACCELERATION, NULL };
static int accelerantIsClone = 0;
//static int can_do_overlays;

#if 1

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
#define GFX_DISPLAY_DYNAMIC			0	/* runtime selection */
#define GFX_DISPLAY_GU1				1	/* 1st generation display controller */
#define GFX_DISPLAY_GU2				0	/* 2nd generation display controller */

#define GFX_2DACCEL_DYNAMIC			0	/* runtime selection */			
#define GFX_2DACCEL_GU1				1	/* 1st generation 2D accelerator */
#define GFX_2DACCEL_GU2				0   /* 2nd generation 2D accelerator */

#define GFX_VIDEO_DYNAMIC			0	/* runtime selection */
#define GFX_VIDEO_CS5530			1   /* support for CS5530 */
#define GFX_VIDEO_SC1400			0   /* support for SC1400 */
#define GFX_VIDEO_SC1200			0   /* support for SC1200 */

#define GFX_VIP_DYNAMIC				0	/* runtime selection */
#define GFX_VIP_SC1400				0   /* support for SC1400 */
#define GFX_VIP_SC1200				0   /* support for SC1200 */

#define GFX_DECODER_DYNAMIC			0	/* runtime selection */
#define GFX_DECODER_SAA7114			0	/* Philips SAA7114 decoder */

#define GFX_TV_DYNAMIC				0	/* runtime selection */
#define GFX_TV_GEODE				0   /* Geode integrated TV encoder */
#define GFX_TV_FS451				0   /* Focus Enhancements FS451 */

#define GFX_I2C_DYNAMIC				0	/* runtime selection */
#define GFX_I2C_ACCESS				0   /* support for ACCESS.BUS */
#define GFX_I2C_GPIO				0   /* support for CS5530 GPIOs */

#define GFX_VGA_DYNAMIC				0	/* runtime selection */
#define GFX_VGA_GU1					0   /* 1st generation graphics unit */

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

#if 0
/* IO MACROS AND ROUTINES
 * These macros must be defined before the initialization or I2C 
 * routines will work properly. 
 */
extern int write_isa_io (int pci_bus, void *addr, int size, uint32 val);
extern int read_isa_io (int pci_bus, void *addr, int size);

unsigned char gfx_inb(unsigned short port)
{
	return (uint8)read_isa_io(0, (void *)(uint32)port, 1);
}

#if 0
unsigned long gfx_ind(unsigned short port)
{
	/* ADD OS SPECIFIC IMPLEMENTATION */
	return(0);
}
#endif

void gfx_outb(unsigned short port, unsigned char data)
{
	write_isa_io(0, (void *)(uint32)port, 1, data);
}

#if 0
void gfx_outd(unsigned short port, unsigned long data)
{
	write_isa_io(0, (void *)port, 2, data);
}
#endif

#define INB(port) gfx_inb(port)
#define IND(port) gfx_ind(port)
#define OUTB(port, data) gfx_outb(port, data)
#define OUTD(port, data) gfx_outd(port, data)

/* INITIALIZATION ROUTINES 
 * These routines are used during the initialization of the driver to 
 * perform such tasks as detecting the type of CPU and video hardware.  
 * The routines require the use of IO, so the above IO routines need 
 * to be implemented before the initialization routines will work
 * properly.
 */

//#include "gfx_init.c"
#else
#define INB(x)
#endif

#if 1
/* INCLUDE DISPLAY CONTROLLER ROUTINES 
 * These routines are used if the display mode is set directly.  If the 
 * project uses VGA registers to set a display mode, then these files
 * do not need to be included.
 */
#include "gfx_mode.h"		/* display mode tables */
#include "gfx_disp.c"		/* display controller routines */
#endif

#if 1
/* INCLUDE GRAPHICS ENGINE ROUTINES 
 * These routines are used to program the 2D graphics accelerator.  If
 * the project does not use graphics acceleration (direct frame buffer
 * access only), then this file does not need to be included. 
 */
#include "gfx_rndr.c"		/* graphics engine routines */
#else
#define GFX_WAIT_PENDING while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_PENDING) { INB(0x80); }
#define GFX_WAIT_BUSY while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_BUSY) { INB(0x80); }
#endif

/* INCLUDE VIDEO OVERLAY ROUTINES
 * These routines control the video overlay hardware. 
 */
#include "gfx_vid.c"		/* video overlay routines */

#if 0
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

#endif
/* VGA ROUTINES
 * This file is used if setting display modes using VGA registers.
 */
#include "gfx_vga.c"		/* VGA routines */

/* END OF FILE */
#endif

#if 0
// a routine that sets the video mode via hard-wired values ala the BIOS for the SONY rotated box
#include "sony_hack.c"
#endif

static status_t durango_init(int fd);
static void durango_uninit(void);

static void scrn2scrn(engine_token *et, blit_params *list, uint32 count);
static void rectfill16(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
static void spanfill16(engine_token *et, uint32 color, uint16 *list, uint32 count);

static status_t DURANGO_ALLOC_MEM(BMemSpec *ms);
static status_t DURANGO_FREE_MEM(BMemSpec *ms);

#if CLONE_SUPPORT
static void durango_set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
#endif
static uint32 durango_accelerant_mode_count(void);
static status_t durango_get_mode_list(display_mode *modes);
status_t spaces_to_mode_list(uint32 spaces, uint32 flags);
static status_t durango_set_mode(display_mode *dm);
static status_t durango_get_mode(display_mode *dm);
static status_t durango_get_fb_config(frame_buffer_config *a_frame_buffer);
#if CLONE_SUPPORT
static status_t durango_move_display_area(uint16 h_display_start, uint16 v_display_start);
#endif
static status_t durango_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
static void durango_move_cursor(uint16 x, uint16 y);
static void durango_show_cursor(bool is_visible);
static status_t setcursorcolors (uint32 blackcolor, uint32 whitecolor);

static uint32 durango_get_engine_count(void);
static status_t durango_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
static status_t durango_release_engine(engine_token *et, sync_token *st);
static void durango_wait_engine_idle(void);
static status_t durango_get_sync_token(engine_token *et, sync_token *st);
static status_t durango_sync_to_token(sync_token *st);

static status_t durango_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate);
static void make_mode_list(void);

#if CH7005_SUPPORT
static status_t GET_TV_OUT_ADJUSTMENTS_FOR_MODE(const display_mode *dm, uint32 *which, tv_out_adjustment_limits *adj);
static status_t GET_TV_OUT_ADJUSTMENTS(uint32 *which, tv_out_adjustments *adj);
static status_t SET_TV_OUT_ADJUSTMENTS(uint32  which, tv_out_adjustments *adj);
#endif

static uint32 OVERLAY_COUNT(const display_mode *dm);
static const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm);
static uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space);
static const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height);
static status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *ob);
static status_t GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
static overlay_token ALLOCATE_OVERLAY(void);
static status_t RELEASE_OVERLAY(overlay_token ot);
static status_t CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);

#if DEBUG > 0

static const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32);
		s2s(B_RGBA32);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15);
		s2s(B_RGBA15);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YCbCr422);
		s2s(B_YCbCr420);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

static void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	xprintf(("  pixel_clock: %ldKHz\n", t->pixel_clock));
	xprintf(("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total));
	xprintf(("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total));
	xprintf((" timing flags:"));
	if (t->flags & B_BLANK_PEDESTAL) xprintf((" B_BLANK_PEDESTAL"));
	if (t->flags & B_TIMING_INTERLACED) xprintf((" B_TIMING_INTERLACED"));
	if (t->flags & B_POSITIVE_HSYNC) xprintf((" B_POSITIVE_HSYNC"));
	if (t->flags & B_POSITIVE_VSYNC) xprintf((" B_POSITIVE_VSYNC"));
	if (t->flags & B_SYNC_ON_GREEN) xprintf((" B_SYNC_ON_GREEN"));
	if (!t->flags) xprintf((" (none)\n"));
	else xprintf(("\n"));
	xprintf((" refresh rate: %4.2fKHz H, %4.2f Hz V\n", ((double)t->pixel_clock / (double)t->h_total), ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total)));
	xprintf(("  color space: %s\n", spaceToString(dm->space)));
	xprintf((" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height));
	xprintf(("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start));

	xprintf(("   mode flags: %.08lx\n\t", dm->flags));
#define DUMPMASKFLAG(mask, value) if ((dm->flags & (uint32)(mask)) == (uint32)(value)) xprintf((" "#value));
#define DUMPFLAG(value) DUMPMASKFLAG(value, value)
	DUMPFLAG(B_SCROLL);
	DUMPFLAG(B_8_BIT_DAC);
	DUMPFLAG(B_HARDWARE_CURSOR);
	DUMPFLAG(B_PARALLEL_ACCESS);
	DUMPFLAG(B_SUPPORTS_OVERLAYS);
#define TVOUTFLAG(x) DUMPMASKFLAG(B_TV_OUT_MASK, x)
	TVOUTFLAG(B_TV_OUT_NONE);
	TVOUTFLAG(B_TV_OUT_NTSC);
	TVOUTFLAG(B_TV_OUT_NTSC_J);
	TVOUTFLAG(B_TV_OUT_PAL);
	TVOUTFLAG(B_TV_OUT_PAL_M);
	if (!dm->flags) xprintf((" (none)\n"));
	else xprintf(("\n"));
}

void paint_for_blit(display_mode *dm, frame_buffer_config *fbc) {
	dump_mode(dm);
	switch (dm->space & ~0x3000) {
	case B_CMAP8: {
		int16 x, y;
		uint8 *fb = (uint8 *)fbc->frame_buffer;
		xprintf((" frame buffer is 8bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xff;
			}
			fb += fbc->bytes_per_row;
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = x;	// 0
			}
			fb += fbc->bytes_per_row;
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		xprintf((" frame buffer is 16bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x7777;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		uint16 pixel;
		xprintf((" frame buffer is 15bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0x7fff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0x7fff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 42; y++) {
			pixel = 0x7777;
			if (y != 40)
			for (x = 0; x < 42; x++) {
				if (x != 40) fb[x] = pixel += 0x0011;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE: {
		int x, y;
		uint32 *fb = (uint32 *)fbc->frame_buffer;
		xprintf((" frame buffer is 32bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffffffff;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffffffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77777777;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	default:
		xprintf(("YIKES! frame buffer shape unknown!\n"));
	}
	xprintf(("paint_for_blit() completed\n"));
}

#endif

#define SET_AI_FROM_SI(si) ((accelerant_info *)(si + 1))

#if CLONE_SUPPORT
static ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
	return MAX_DURANGO_DEVICE_NAME_LENGTH;
}

static void GET_ACCELERANT_CLONE_INFO(void *data) {
	/* a null terminated string with the device name */
	durango_device_name dn;
	status_t result;
	
	dn.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dn.name = (char *)data;
	result = ioctl(file_handle, DURANGO_DEVICE_NAME, &dn, sizeof(dn));
	xprintf(("GET_ACCELERANT_CLONE_INFO - ioctl(DURANGO_DEVICE_NAME) returns %ld\n", result));
}

static status_t CLONE_ACCELERANT(void *data) {
	status_t result = B_OK;
	durango_get_private_data gpd;
	char path[MAXPATHLEN];
	int fd;

	/* the data is the device name */
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);
	xprintf(("CLONE_ACCELERANT: opening %s\n", path));
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0) {
		result = fd;
		xprintf(("Open failed: %d/%d (%s)\n", result, errno, strerror(errno)));
		goto error0;
	}

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	result = durango_init_common(fd);
	if (result == B_OK) goto error0;
	
error1:
	close(fd);
error0:
	xprintf(("CLONE_ACCELERANT returns %.8lx\n", result));
	return result;
}
#endif

static status_t durango_init_common(int fd)
{
	status_t result;
	durango_get_private_data gpd;

	/* memorise the file descriptor */
	file_handle = fd;
	/* set the magic number so the driver knows we're for real */
	gpd.magic = DURANGO_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(file_handle, DURANGO_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	xprintf(("durango_init_common - ioctl(DURANGO_GET_PRIVATE_DATA) returns %ld\n", result));
	if (result != B_OK) return result;
	
	/* transfer the info to our globals */
	si = gpd.si;
	xprintf(("si %p\n", si));
	ai = SET_AI_FROM_SI(si);
	xprintf(("ai %p\n", ai));
	gfx_regptr = (uchar *)si->regs;
	gfx_vidptr = (uchar *)si->dac;
	gfx_fbptr = si->framebuffer;
	xprintf(("durango_init_common() fini\n"));
	return B_OK;
}

static status_t durango_init(int fd) {
	status_t result;
	BMemSpec *ms;

	result = durango_init_common(fd);
	if (result != B_OK) goto err0;

	ai->fbc.frame_buffer = ai->fbc.frame_buffer_dma = 0;
	memset(&(ai->fb_spec), 0, sizeof(ai->fb_spec));
	memset(&(ai->compression_spec), 0, sizeof(ai->compression_spec));
	memset(&(ai->ovl_buffer_specs[0]), 0, sizeof(ai->ovl_buffer_specs));
	memset(&(ai->ovl_buffers[0]), 0, sizeof(ai->ovl_buffers));
	memset(&(ai->ovl_tokens[0]), 0, sizeof(ai->ovl_tokens));

	// init the memspec
	ms = &(ai->cursor.cursor_spec);
	memset(ms, 0, sizeof(*ms));
	// cursor data takes 256 bytes
	ms->ms_MR.mr_Size = 256;
	// aligned on 1KB boundary
	ms->ms_AddrCareBits = (1024 - 1);
	ms->ms_AddrStateBits = 0;
	ms->ms_AllocFlags = B_MEMSPECF_FROMTOP;
	result = DURANGO_ALLOC_MEM(ms);
	if (result != B_OK) goto err0;
	ai->cursor.data = (uint32 *)((uint8*)(si->framebuffer) + ms->ms_MR.mr_Offset);

	/* make mode list from display table */
	make_mode_list();

	/* a little more initialization */
	INIT_BEN(ai->engine);
#if 0
	if (ai->engine.sem < 0) {
		result = ai->engine.sem;
		free(mode_list);
		mode_list = 0;
		mode_count = 0;
	}
#endif
	ai->fifo_count = 0;
	setcursorcolors(0x00000000, 0x00ffffff);

#if CH7005_SUPPORT
	{
	display_mode dm;
	dm.timing.pixel_clock = 30209;
	dm.timing.h_display = 640;
	dm.timing.v_display = 480;
	dm.flags = B_TV_OUT_NTSC; // how do we choose PAL vs NTSC vs neither at this point?
	durango_set_mode(&dm);
	}
#else
	durango_set_mode(mode_list);
#endif
#if 0
	// just here to test the hard-wired values for the SONY BIOS
	sony_hack();
	{ int i;
	for (i = 0; i < (1024 * 800); i++)
		gfx_fbptr[i] = 0x55;
	}
#endif
err0:
	xprintf(("durango_init() completes with result %ld\n", result));
	return result;
}

void durango_uninit(void) {
	xprintf(("durango_uninit()\n"));
	if (accelerantIsClone == 0)
	{
		xprintf(("primary accelerant deleting benaphore\n"));
		DELETE_BEN(ai->engine);
		if (mode_list) free(mode_list);
		mode_list = 0;
		mode_count = 0;
	}
	else
	{
		xprintf(("Clone is closing driver handle\n"));
		close(file_handle);
	}
	xprintf(("durango_uninit() completes OK\n"));
}

/* the standard entry point */
void *	get_accelerant_hook(uint32 feature, void *data) {
	/* avoid unused warning */
	(void)data;

	switch (feature) {
		case B_INIT_ACCELERANT:
			return (void *)durango_init;
#if CLONE_SUPPORT
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void *)ACCELERANT_CLONE_INFO_SIZE;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void *)GET_ACCELERANT_CLONE_INFO;
		case B_CLONE_ACCELERANT:
			return (void *)CLONE_ACCELERANT;
#endif
		case B_UNINIT_ACCELERANT:
			return (void *)durango_uninit;

		case B_ACCELERANT_MODE_COUNT:			/* required */
			return (void *)durango_accelerant_mode_count;
		case B_GET_MODE_LIST:			/* required */
			return (void *)durango_get_mode_list;
#if CLONE_SUPPORT
		case B_PROPOSE_DISPLAY_MODE:	/* optional */
			return (void *)0;
#endif
		case B_SET_DISPLAY_MODE:		/* required */
			return (void *)durango_set_mode;
		case B_GET_DISPLAY_MODE:		/* required */
			return (void *)durango_get_mode;
		case B_GET_FRAME_BUFFER_CONFIG:	/* required */
			return (void *)durango_get_fb_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void *)durango_get_pixel_clock_limits;
#if CLONE_SUPPORT
		case B_MOVE_DISPLAY:				/* optional */
			return (void *)durango_move_display_area;
#endif
#if INDEXED_COLOR_SUPPORT
		case B_SET_INDEXED_COLORS:		/* required if driver supports 8bit indexed modes */
			return (void *)durango_set_indexed_colors;
#endif

		case B_SET_CURSOR_SHAPE:
			return (void *)durango_set_cursor_shape;
		case B_MOVE_CURSOR:
			return (void *)durango_move_cursor;
		case B_SHOW_CURSOR:
			return (void *)durango_show_cursor;
#if _SUPPORTS_FEATURE_CURSOR_COLORS
		case B_SET_CURSOR_COLORS:
			return (void *) setcursorcolors;
#endif

		/* synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return (void *)durango_get_engine_count;
		case B_ACQUIRE_ENGINE:
			return (void *)durango_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void *)durango_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return (void *)durango_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return (void *)durango_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return (void *)durango_sync_to_token;

		case B_SCREEN_TO_SCREEN_BLIT:
			return scrn2scrn;
		case B_FILL_RECTANGLE:
			return (void *)rectfill16;
		case B_FILL_SPAN:
				return (void *)spanfill16;

		/* video overlays */
#define HOOK(x) case B_##x: return (void *)(x)
		HOOK(OVERLAY_COUNT);
		HOOK(OVERLAY_SUPPORTED_SPACES);
		HOOK(OVERLAY_SUPPORTED_FEATURES);
		HOOK(ALLOCATE_OVERLAY_BUFFER);
		HOOK(RELEASE_OVERLAY_BUFFER);
		HOOK(GET_OVERLAY_CONSTRAINTS);
		HOOK(ALLOCATE_OVERLAY);
		HOOK(RELEASE_OVERLAY);
		HOOK(CONFIGURE_OVERLAY);
#undef HOOK

#if CH7005_SUPPORT
		case B_GET_TV_OUT_ADJUSTMENTS_FOR_MODE:
			return (void *)GET_TV_OUT_ADJUSTMENTS_FOR_MODE;
		case B_GET_TV_OUT_ADJUSTMENTS:
			return (void *)GET_TV_OUT_ADJUSTMENTS;
		case B_SET_TV_OUT_ADJUSTMENTS:
			return (void *)SET_TV_OUT_ADJUSTMENTS;
#endif
	}
	return 0;
}


/*----------------------------------------------------------
 OS dependent functions
----------------------------------------------------------*/

static uint32 durango_accelerant_mode_count(void) {
	xprintf(("durango_accelerant_mode_count returns %ld\n", mode_count));
	return mode_count;
}
static status_t durango_get_mode_list(display_mode *modes) {
	xprintf(("durango_get_mode_list(modes: %p)\n", modes));
	if (mode_list) {
		memcpy(modes, mode_list, sizeof(display_mode) * mode_count);
		return B_OK;
	}
	return B_ERROR;
}

static status_t durango_get_mode(display_mode *dm) {
	xprintf(("durango_get_mode(dm: %p)\n", dm));
	*dm = ai->dm;
	return B_OK;
}

static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate) {
	double pc;
	pc = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)refresh_rate;
	return (uint32)(pc / 1000);
}

static status_t durango_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high) {
	/* convert to pixel clocks given the current display_mode parameters */
	*low = calc_pixel_clock_from_refresh(dm, 60.0);
	*high = calc_pixel_clock_from_refresh(dm, 85.0);
	return B_OK;
}

static status_t durango_get_fb_config(frame_buffer_config *a_frame_buffer) {
	*a_frame_buffer = ai->fbc;
	xprintf(("durango_get_fb_config - fb: %p, rowbytes: 0x%08lx (%ld)\n", a_frame_buffer->frame_buffer, a_frame_buffer->bytes_per_row, a_frame_buffer->bytes_per_row));
	return B_OK;
}

static status_t durango_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask)
{
	int i;
	uint32 *data = ai->cursor.data;
	uint32 value;

	/* build the new cursor */
	for (i = 0; i < height; i++)
	{
		/* first 16 pixels */
		value = *andMask++;
		value <<= 8;
		value |= (width > 8) ? *andMask++ : 0xff;
		value <<= 8;
		value |= *xorMask++;
		value <<= 8;
		value |= (width > 8) ? *xorMask++ : 0x00;
		*data++ = value;
		/* second 16 pixels */
		value = (width > 16) ? *andMask++ : 0xff;
		value <<= 8;
		value |= (width > 24) ? *andMask++ : 0xff;
		value <<= 8;
		value |= (width > 16) ? *xorMask++ : 0x00;
		value <<= 8;
		value |= (width > 24) ? *xorMask++ : 0x00;
		yprintf(("second 16 pixels: %.8lx\n", value));
		*data++ = value;
	}
	for ( ;i < 32; i++)
	{
		*data++ = 0xffff0000;
		*data++ = 0xffff0000;
	}
	ai->cursor.hot_x = hot_x;
	ai->cursor.hot_y = hot_y;
	return B_OK;
}

static void durango_move_cursor(uint16 x, uint16 y) {
	gfx_set_cursor_position(ai->cursor.cursor_spec.ms_MR.mr_Offset, x, y, ai->cursor.hot_x, ai->cursor.hot_y);
}

static void durango_show_cursor(bool is_visible) {
	gfx_set_cursor_enable(is_visible);
	ai->cursor.is_visible = is_visible;
}

static status_t
setcursorcolors (uint32 blackcolor, uint32 whitecolor)
{
	gfx_set_cursor_colors(whitecolor, blackcolor);
	xprintf(("setcursorcolors(%.8lx, %.8lx)\n", blackcolor, whitecolor));
	return (B_OK);
}

#if ONLY_PRESET_TIMINGS
#else
static void parms_to_mode(DISPLAYMODE *parms, display_mode *dm) {
	dm->virtual_width = dm->timing.h_display = parms->hactive;
	dm->timing.h_sync_start = parms->hsyncstart;
	dm->timing.h_sync_end = parms->hsyncend;
	dm->timing.h_total = parms->htotal;
	dm->virtual_height = dm->timing.v_display = parms->vactive;
	dm->timing.v_sync_start = parms->vsyncstart;
	dm->timing.v_sync_end = parms->vsyncend;
	dm->timing.v_total = parms->vtotal;
	dm->timing.flags = 0;
	if ((parms->flags & GFX_MODE_NEG_HSYNC)== 0) dm->timing.flags |= B_POSITIVE_HSYNC;
	if ((parms->flags & GFX_MODE_NEG_VSYNC)== 0) dm->timing.flags |= B_POSITIVE_VSYNC;
	dm->h_display_start = dm->v_display_start = 0;
	dm->timing.pixel_clock = ((uint64)parms->frequency * 1000) >> 16;
	xprintf(("timing.pixel_clock == %ld\n", dm->timing.pixel_clock));
	dm->flags = B_PARALLEL_ACCESS | B_HARDWARE_CURSOR;
}
#endif

#if CH7005_SUPPORT

#define	DMR_IR_512x384	0x00
#define DMR_IR_720x400	0x20
#define DMR_IR_640x400	0x40
#define DMR_IR_640x480	0x60
#define DMR_IR_800x600	0x80
#define DMR_IR_720x576i	0xA0 // PAL interlaced
#define DMR_IR_720x480i	0xA0 // NTSC interlaced
#define DMR_IR_800x500i 0xC0 // PAL interlaced
#define DMR_IR_640x400i 0xC0 // NTSC interlaced

#define DMR_VOS_PAL		0x00
#define DMR_VOS_NTSC	0x08
#define DMR_VOS_PAL_M	0x10
#define DMR_VOS_NTSC_J	0x18
#define DMR_VOS_MASK	0x18

#define DMR_SR_5x4_A	0x00 // 5/4
#define DMR_SR_1x1		0x01 // 1/1
#define DMR_SR_5x4_B	0x02 // 5/4
#define DMR_SR_7x8		0x02 // 7/8
#define DMR_SR_5x6		0x03 // 5/6
#define DMR_SR_3x4		0x04 // 3/4
#define DMR_SR_7x10		0x05 // 7/10

#define MODE_FLAGS (B_PARALLEL_ACCESS | B_HARDWARE_CURSOR)
#define MODE_FLAGS_P (MODE_FLAGS | B_TV_OUT_PAL)
#define MODE_FLAGS_N (MODE_FLAGS | B_TV_OUT_NTSC)
#define T_POSITIVE_SYNC (B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)

typedef struct {
	display_mode dm;
	uint8 DMR;
	uint8 PLLCAP;
	uint16 h_pos;
	uint16 v_pos;
	uint16 start_active_video;
	uint32 FSCI[3];
} tv_display_mode;

/*
	I agree that it's ugly not to have all the NTSC and PAL modes together,
	but it's primarily because these are in the order that the CH7005 docs
	describe them.  Makes it easier to check the numbers for various parameters.
*/
static const tv_display_mode TV_BASE_MODES[] = {

#if PAL_MODES
  { { {  21000,  512,  648,  712,  840,  384,  385,  389,  500,     T_POSITIVE_SYNC}, B_RGB16,  512,  384, 0, 0, MODE_FLAGS_P}, DMR_IR_512x384 | DMR_VOS_PAL  | DMR_SR_5x4_A, 1, 0, 0, 0, { 806021060, 651209077, 0} },
  { { {  26250,  512,  648,  712,  840,  384,  385,  389,  625,     T_POSITIVE_SYNC}, B_RGB16,  512,  384, 0, 0, MODE_FLAGS_P}, DMR_IR_512x384 | DMR_VOS_PAL  | DMR_SR_1x1,   1, 0, 0, 0, { 644816848, 520967262, 0} },
#endif
#if NTSC_MODES
  { { {  20139,  512,  624,  688,  800,  384,  385,  389,  420,     T_POSITIVE_SYNC}, B_RGB16,  512,  384, 0, 0, MODE_FLAGS_N}, DMR_IR_512x384 | DMR_VOS_NTSC | DMR_SR_5x4_A, 1, 0, 0, 0, { 763363328, 762524467, 763366524} },
  { { {  24671,  512,  624,  688,  784,  384,  437,  441,  525,     T_POSITIVE_SYNC}, B_RGB16,  512,  384, 0, 0, MODE_FLAGS_N}, DMR_IR_512x384 | DMR_VOS_NTSC | DMR_SR_1x1,   0, 45, 0, 162, { 623153737, 622468953, 623156346} },
#endif
  
#if PAL_MODES
  { { {  28125,  720,  880,  968, 1124,  400,  447,  451,  500,     T_POSITIVE_SYNC}, B_RGB16,  720,  400, 0, 0, MODE_FLAGS_P}, DMR_IR_720x400 | DMR_VOS_PAL  | DMR_SR_5x4_A, 1, 0, 0, 0, { 601829058, 486236111, 0} },
  { { {  34875,  720,  872,  960, 1116,  400,  447,  451,  625,     T_POSITIVE_SYNC}, B_RGB16,  720,  400, 0, 0, MODE_FLAGS_P}, DMR_IR_720x400 | DMR_VOS_PAL  | DMR_SR_1x1,   0, 0, 0, 0, { 485346014, 392125896, 0} },
#endif
#if NTSC_MODES
  { { {  23790,  720,  800,  872,  944,  400,  401,  404,  420,     T_POSITIVE_SYNC}, B_RGB16,  720,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_720x400 | DMR_VOS_NTSC | DMR_SR_5x4_A, 1, 0, 0, 0, { 574429782, 573789541, 574432187} },
  { { {  29454,  720,  792,  864,  936,  400,  447,  451,  525,     T_POSITIVE_SYNC}, B_RGB16,  720,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_720x400 | DMR_VOS_NTSC | DMR_SR_1x1,   1, 57, 0, 144, { 463962517, 463452668, 463964495} },
#endif
  
#if PAL_MODES
  { { {  25000,  640,  784,  864, 1000,  400,  447,  451,  500,     T_POSITIVE_SYNC}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_P}, DMR_IR_640x400 | DMR_VOS_PAL  | DMR_SR_5x4_B, 0, 0, 0, 0, { 677057690, 547015625, 0} },
  { { {  31500,  640,  784,  864, 1008,  400,  447,  451,  625,     T_POSITIVE_SYNC}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_P}, DMR_IR_640x400 | DMR_VOS_PAL  | DMR_SR_1x1,   1, 0, 0, 0, { 537347373, 434139385, 0} },
#endif
#if NTSC_MODES
  { { {  21146,  640,  712,  776,  840,  400,  401,  404,  420,     T_POSITIVE_SYNC}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_640x400 | DMR_VOS_NTSC | DMR_SR_5x4_A, 1, 0, 0, 0, { 646233505, 645523358, 646236211} },
  { { {  26433,  640,  712,  776,  840,  400,  447,  451,  525,     T_POSITIVE_SYNC}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_640x400 | DMR_VOS_NTSC | DMR_SR_1x1,   1, 51, 0, 128, { 516986804, 516418687, 516988968} },
  { { {  30209,  640,  712,  776,  840,  400,  480,  484,  600,     T_POSITIVE_SYNC}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_640x400 | DMR_VOS_NTSC | DMR_SR_7x8,   0, 69, 0, 128, { 452363454, 451866351, 452365347} },
#endif

#if PAL_MODES
  { { {  21000,  640,  712,  776,  840,  480,  490,  494,  500,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_P}, DMR_IR_640x480 | DMR_VOS_PAL  | DMR_SR_5x4_A, 1, 0, 0, 0, { 806021060, 651209077, 0} },
  { { {  26250,  640,  712,  776,  840,  480,  490,  494,  625,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_P}, DMR_IR_640x480 | DMR_VOS_PAL  | DMR_SR_1x1,   1, 53, 21, 128, { 644816848, 520967262, 0} },
  { { {  31500,  640,  712,  776,  840,  480,  490,  494,  750,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_P}, DMR_IR_640x480 | DMR_VOS_PAL  | DMR_SR_5x6,   1, 0, 0, 0, { 537347373, 434139385, 0} },
#endif
#if NTSC_MODES
  { { {  24671,  640,  680,  744,  784,  480,  490,  494,  525,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_N}, DMR_IR_640x480 | DMR_VOS_NTSC | DMR_SR_1x1,   0, 28, 0, 104, { 623153737, 622468953, 623156346} },
  { { {  28195,  640,  680,  744,  784,  480,  521,  525,  600,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_N}, DMR_IR_640x480 | DMR_VOS_NTSC | DMR_SR_7x8,   0, 45, 0, 104, { 545259520, 544660334, 545261803} },
  { { {  30209,  640,  688,  752,  800,  480,  490,  494,  630,     T_POSITIVE_SYNC}, B_RGB16,  640,  480, 0, 0, MODE_FLAGS_N}, DMR_IR_640x480 | DMR_VOS_NTSC | DMR_SR_5x6,   0, 53, 18, 112, { 508908885, 508349645, 508911016} },
#endif

#if PAL_MODES
  { { {  29500,  800,  840,  912,  944,  600,  601,  605,  625,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_P}, DMR_IR_800x600 | DMR_VOS_PAL  | DMR_SR_1x1,   0, 0, 0, 0, { 645499916, 521519134, 0} },
  { { {  36000,  800,  840,  920,  960,  600,  601,  605,  750,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_P}, DMR_IR_800x600 | DMR_VOS_PAL  | DMR_SR_5x6,   1, 0, 0, 0, { 528951320, 427355957, 0} },
  { { {  39000,  800,  840,  912,  944,  600,  601,  605,  836,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_P}, DMR_IR_800x600 | DMR_VOS_PAL  | DMR_SR_3x4,   0, 0, 0, 0, { 488262757, 394482422, 0} },
#endif
#if NTSC_MODES
  { { {  39272,  800,  856,  936, 1040,  600,  601,  605,  630,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_N}, DMR_IR_800x600 | DMR_VOS_NTSC | DMR_SR_5x6,   1, 28, 0, 184, { 521957831, 521384251, 521960016} },
  { { {  43636,  800,  856,  936, 1040,  600,  631,  635,  700,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_N}, DMR_IR_800x600 | DMR_VOS_NTSC | DMR_SR_3x4,   1, 44, 0, 184, { 469762048, 469245826, 469764015} },
  { { {  47831,  800,  856,  944, 1064,  600,  648,  651,  750,     T_POSITIVE_SYNC}, B_RGB16,  800,  600, 0, 0, MODE_FLAGS_N}, DMR_IR_800x600 | DMR_VOS_NTSC | DMR_SR_7x10,  0, 61, 0, 208, { 428554851, 428083911, 438556645} },
#endif

#if INTERLACED_TIMING_SUPPORT
#if PAL_MODES
  { { {  13500,  720,  760,  832,  864,  576,  577,  581,  625,     B_TIMING_INTERLACED}, B_RGB16,  720,  576, 0, 0, MODE_FLAGS_P}, DMR_IR_720x576i | DMR_VOS_PAL  | DMR_SR_1x1, 1, 0, 0, 0, { 705268427, 569807942, 0} },
#endif
#if NTSC_MODES
  { { {  13500,  720,  752,  824,  856,  480,  481,  485,  525,     B_TIMING_INTERLACED}, B_RGB16,  720,  480, 0, 0, MODE_FLAGS_N}, DMR_IR_720x480i | DMR_VOS_NTSC | DMR_SR_1x1, 1, 0, 0, 0, { 569408543, 568782819, 569410927} },
#endif
#if PAL_MODES
  { { {  17734,  800,  928, 1016, 1136,  500,  501,  505,  625,     B_TIMING_INTERLACED}, B_RGB16,  800,  500, 0, 0, MODE_FLAGS_P}, DMR_IR_800x500i | DMR_VOS_PAL  | DMR_SR_1x1, 0, 0, 0, 0, { 1073747879, 867513766, 0} },
#endif
#if NTSC_MODES
  { { {  14318,  640,  744,  816,  912,  400,  401,  405,  525,     B_TIMING_INTERLACED}, B_RGB16,  640,  400, 0, 0, MODE_FLAGS_N}, DMR_IR_640x400i | DMR_VOS_NTSC | DMR_SR_1x1, 1, 0, 0, 0, { 1073741824, 1072561888, 1073746319} }
#endif
#endif

};
#define TV_MODE_COUNT (sizeof(TV_BASE_MODES) / sizeof(TV_BASE_MODES[0]))

#if MASTER_MODE
static void ch7005_calc_pll(uint32 khz_clock, uint8 *pll_values)
{
	const int32 kRefFreq = 14318;

	int32 delta, best_delta;
	int first_n;
	int n, m;
	int best_n = 0, best_m = 0;

	first_n = (khz_clock / kRefFreq) - 1;
	if (first_n < 0) first_n = 0;
	xprintf(("first_n: %d\n", first_n));

	best_delta = khz_clock * 2;
	for (n = first_n; n < (1 << 10); n++)
	{
		for (m = 0; m < (1 << 9); m++)
		{
			delta = (int32)khz_clock - ((kRefFreq * (n + 2)) / (m + 2));
			if (delta < 0) delta = -delta;
			if (delta < best_delta)
			{
				xprintf(("new best delta: %d - N: %d, M: %d\n", delta, n, m));
				best_n = n;
				best_m = m;
				best_delta = delta;
			}
		}
	}
	xprintf(("N, M: %d, %d\n", best_n, best_m));
	*pll_values++ = ((best_m & 0x100) >> 8) | ((best_n & 0x300) >> 7);
	*pll_values++ = best_m & 0x0ff;
	*pll_values   = best_n & 0x0ff;
	gfx_set_clock_frequency(
#if DOUBLE_CLOCK
		(2 *		/* double the clock rate */
#endif
		((uint64)((kRefFreq * (best_n + 2)) / (best_m + 2)) << 16) / 1000
#if DOUBLE_CLOCK
		| 0x80000000 /* but divide by two for the display */
		)
#endif
	);
}
#endif

static const uint8 ch7005_reg_masks[0x3E] = {
	0x00, // 0x00 800x600 NTSC 7/10
	0x00, // 0x01
	0xFF, // 0x02 NOT USED
	0x00, // 0x03 Video Bandwidth
	0x90, // 0x04 input data format
	0xFF, // 0x05 NOT USED
	0x20, // 0x06 Clock mode
	0x00, // 0x07 Start Active Video
	0xF8, // 0x08 Position overflow
	0x00, // 0x09 Black level (7F is neutral)
	0x00, // 0x0A Horiz. pos.
	0x00, // 0x0B Verti. pos.
	0xFF, // 0x0C NOT USED
	0xF0, // 0x0D sync polarity
	0xE0, // 0x0E Power managment (output select) 
	0xFF, // 0x0F NOT USED
	0x00, // 0x10 Connection detect
	0xF8, // 0x11 Contrast enhancement (0x03 is neutral)
	0xFF, // 0x12 NOT USED
	0xE0, // 0x13 PLL Overflow
	0x00, // 0x14 PLL M
	0x00, // 0x15 PLL N
	0xFF, // 0x16 NOT USED
	0xC0, // 0x17 Buffered clock output (14 MHz)
	0xF0, // 0x18 SubCarrier values (not used in slave mode, but...)
	0xF0, // 0x19 for mode 24, there are two values:
	0xF0, // 0x1A normal dot crawl: 428,554,851
	0xE0, // 0x1B no dot crawl    : 438,556,645
	0xE0, // 0x1C (also, buffered clock out enable)
	0xF0, // 0x1D
	0xF0, // 0x1E
	0xF0, // 0x1F
	0xC0, // 0x20 PLL control
	0xE0, // 0x21 CIV control (automatic)
	0xFF, // 0x22
	0xFF, // 0x23
	0xFF, // 0x24
	0xFF, // 0x25
	0xFF, // 0x26
	0xFF, // 0x27
	0xFF, // 0x28
	0xFF, // 0x29
	0xFF, // 0x2A
	0xFF, // 0x2B
	0xFF, // 0x2C
	0xFF, // 0x2D
	0xFF, // 0x2E
	0xFF, // 0x2F
	0xFF, // 0x30
	0xFF, // 0x31
	0xFF, // 0x32
	0xFF, // 0x33
	0xFF, // 0x34
	0xFF, // 0x35
	0xFF, // 0x36
	0xFF, // 0x37
	0xFF, // 0x38
	0xFF, // 0x39
	0xFF, // 0x3A
	0xFF, // 0x3B
	0xFF, // 0x3C
	0xF8  // 0x3D
};

#if OVERRIDE_TV_SETTINGS
void override_tv_settings(void)
{
	FILE *f = fopen("/boot/home/config/settings/durango_tv_settings", "r");
	int force = 1;
	char line_buf[128]; // the line buffer
	char *s;			// for striping off comments
	int index;			// the register to read
	int and_mask;			// the value's logical AND mask
	int or_mask;			// the value's logical OR mask
	durango_ch7005_io dci;	// ioctl() control block
	uint8 data;			// storage for the register read

	/* bail if no settings file */
	if (!f) return;
	/* prep the ioctl() data block */
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dci.data = &data;
	/* should we force a write of "unused" registers? */
	dci.count = force ? 0x81 : 1;

	/* read a line from the input stream */
	while (fgets(line_buf, sizeof(line_buf), f))
	{
		/* remove any comments */
		s = strchr(line_buf, '#');
		if (s) *s = '\0';
		/* parse three numbers from the string */
		if (sscanf(line_buf, "%i %i %i", &index, &and_mask, &or_mask) == 3)
		{
			/* a valid index? */
			if ((index >= 0) && (index <= 0x3e))
			{
				status_t result;
				dci.index = (uint8)index;
				/* read the data */
				result = ioctl(file_handle, DURANGO_READ_CH7005, &dci, sizeof(dci));
				if (result == B_OK)
				{
#if 0
					/* save the read value */
					int orig_data = (int)data;
#endif
					/* store the data to write */
					data &= (uint8)and_mask;
					data |= (uint8)or_mask;
					/* call the driver */
					result = ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
#if 0
					if (result == B_OK)
					{
						fprintf(stdout, "read_mod_write: index %3d (0x%.2x) read %3d (0x%.2x) wrote %3d (0x%.2x)\n",
							index, index, orig_data, orig_data, data, data);
					}
					else
					{
						fprintf(stderr, "read_mod_write: failed writing index %3d (0x%.2x) with reason %s\n",
							index, index, strerror(errno));
					}
				}
				else
				{
					fprintf(stderr, "read_mod_write: failed read of index %3d (0x%.2x) with reason %s\n",
						index, index, strerror(errno));							
				}
			}
			else
			{
				fprintf(stderr, "write_regs: index %3d out of range\n", index);
			}
#else
				}
			}
#endif
		}
	}
	fclose(f);
}
#endif

static void ch7005_disable_tv_out(void)
{
	uint8 aByte = 0x0c;
	durango_ch7005_io dci;
	dci.data = &aByte;
	dci.index = 0x0E;
	dci.count = 1;
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
}

static status_t ch7005_set_tv_out_mode(int index, display_mode *dm)
{
#if MASTER_MODE
//#define DATA_SIZE 0x3E
#define DATA_SIZE 0x22
#else
#define DATA_SIZE 0x22
#endif
	/* whee! */
	durango_ch7005_io dci;
	uint8 data[DATA_SIZE];
	status_t result;
	uint16 SAV = (dm->timing.h_total - dm->timing.h_sync_start) & ~0x1;
	uint16 HPR = (dm->timing.h_total - dm->timing.h_display) / 4;
	uint16 VPR = 0; //(dm->timing.v_total - dm->timing.v_display) / 2; //???
	uint8 DMR = TV_BASE_MODES[index].DMR;
	uint8 VOS = (DMR & DMR_VOS_MASK);

	/* use the table's values if there are some */
	if (TV_BASE_MODES[index].h_pos || TV_BASE_MODES[index].v_pos || TV_BASE_MODES[index].start_active_video)
	{
		HPR = TV_BASE_MODES[index].h_pos;
		VPR = TV_BASE_MODES[index].v_pos;
		SAV = TV_BASE_MODES[index].start_active_video;
	}

	/* read-modify-write */
	dci.data = data;
	dci.count = DATA_SIZE;
	dci.index = 0x00;
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	result = ioctl(file_handle, DURANGO_READ_CH7005, &dci, sizeof(dci));
	if (result != B_OK) return result;

#if DEBUG > 0
	{
	int i;
	xprintf(("Read from ch7005:\n"));
	for (i = 0; i < DATA_SIZE; i++)
		xprintf((" [%.2x] = %.2x\n", i, data[i]));
	}
#endif

	/* clear out the bits we can set */
	{
	int i;
	for (i = 0; i < DATA_SIZE; i++)
		data[i] &= ch7005_reg_masks[i];
	}

	/* display mode */
	data[0x00] |= DMR;
	/* flicker filter */
	data[0x01] |= 0x38;
	/* video bandwidth */
#if MASTER_MODE
	data[0x03] |= 0x00;
#else
	data[0x03] |= 0x31;
#endif
	/* input data format */
	data[0x04] |= 0x00; // 565
	/* clock mode */
	data[0x06] |= (
#if MASTER_MODE
		0x40	// master mode
#else
		0x00	// slave
#endif
#if POSITIVE_EDGE_SELECT
	|	0x10	
#endif
#if DOT_CRAWL_REDUCTION
	|	0x80	// CFRB
#endif
#if DOUBLE_CLOCK
	|	0x01	// 2x P-OUT
#endif
	);	
//	0x50; //0xC1; // chroma locked, master mode, negative edge, 1x XCLK, 2x P-OUT (ie: input mode 0)
//#else
//	data[0x06] |= 0x10; // slave
//#endif
	/* start active video */
	data[0x07] |= SAV & 0xff;
	/* position overflow */
	data[0x08] |= ((SAV & 0x100) >> 6) | ((HPR & 0x100) >> 7) | ((VPR & 0x100) >> 8);
	/* black level */
	data[0x09] |= (VOS == DMR_VOS_NTSC_J) ? 100 : ((VOS == DMR_VOS_PAL) ? 105 : 127);
	/* horizontal position */
	data[0x0A] |= HPR & 0xff;
	/* vertical position */
	data[0x0B] |= VPR & 0xff;
	/* sync polarity */
#if 0 // MASTER_MODE
	data[0x0D] = 0; // per FIC
#else
	data[0x0D] |=
		((dm->timing.flags & B_POSITIVE_HSYNC) ? 0x01 : 0) |
		((dm->timing.flags & B_POSITIVE_VSYNC) ? 0x02 : 0);
#endif
	/* power management */
#if MASTER_MODE
	data[0x0E] |= 0x4B; // per FIC
#else
	data[0x0E] |= 0x0B;
#endif
	/* connection detect */
	data[0x10] |= 0;
	/* contrast */
#if MASTER_MODE
	data[0x11] = 0x03; // per FIC
#else
	data[0x11] |= 0x03;
#endif
#if MASTER_MODE
	/* PLL */
	ch7005_calc_pll(dm->timing.pixel_clock, data + 0x13);
#endif
	/* buffered clock output */
	data[0x17] |= 0;
#if DOT_CRAWL_REDUCTION
	/* FSCI regs */
	{
	int dot_crawl_index = (VOS >> 4);
	uint32 FSCI = TV_BASE_MODES[index].FSCI[dot_crawl_index];
	int i;
	for (i = 0x1f; i > 0x17; i--)
	{
		data[i] |= FSCI & 0x0f;
		FSCI >>= 4;
	}
	// clear DSEN, as per FIC
	data[0x1C] &= ~0x10;
	}
#endif
	data[0x1B] |= 0x10;	// as per chrontel

	/* PLL control */
	data[0x20] |= 0x0A | ((TV_BASE_MODES[index].PLLCAP & 0x01) << 4);
	/* CIV control */
#if DOT_CRAWL_REDUCTION
	data[0x21] |= 0x00;
#else
	data[0x21] |= 0x01;
#endif

	//data[0x3D] |= 0x00; // FIC - disable macrovision
	{
		uint8 aByte = 0;
		durango_ch7005_io dci;
		dci.data = &aByte;
		dci.index = 0x3D;
		dci.count = 1;
		dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
		ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
	}

#if DEBUG > 0
	{
	int i;
	xprintf(("Write to ch7005:\n"));
	for (i = 0; i < DATA_SIZE; i++)
		xprintf((" [%.2x] = %.2x\n", i, data[i]));
	}
#endif

	/* write the data */
	result = ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
	xprintf(("DURANGO_WRITE_CH7005: %.8lx, %.8lx\n", result, (uint32)errno));
	if (result != B_OK) return result;
#if OVERRIDE_TV_SETTINGS
	override_tv_settings();
#endif
	return B_OK;
}

status_t 
GET_TV_OUT_ADJUSTMENTS_FOR_MODE(const display_mode *dm, uint32 *which, tv_out_adjustment_limits *adj)
{
	uint32 i;
	uint32 masked_mode = dm->flags & B_TV_OUT_MASK;

	/* match this mode against one of ours? */
	for (i = 0; i < TV_MODE_COUNT; i++)
	{
		if ((dm->timing.pixel_clock == TV_BASE_MODES[i].dm.timing.pixel_clock) &&
			(dm->timing.h_display == TV_BASE_MODES[i].dm.timing.h_display) &&
			(dm->timing.v_display == TV_BASE_MODES[i].dm.timing.v_display))
		{
			//*dm = TV_BASE_MODES[i].dm;
			xprintf(("GET_TV_OUT_ADJUSTMENTS_FOR_MODE: matched mode at index %ld\n", i));
			break;
		}
	}	

	/* assume dm contains a TV-out enabled mode */
	memset(adj, 0, sizeof(tv_out_adjustments));
	*which = B_TV_OUT_H_POS | B_TV_OUT_V_POS | B_TV_OUT_START_ACTIVE | B_TV_OUT_BLACK_LEVEL | B_TV_OUT_CONTRAST;
	/* h_pos */
	adj->h_pos.max_value = 0x1ff;
	adj->h_pos.default_value = (i != TV_MODE_COUNT) ? TV_BASE_MODES[i].h_pos : (dm->timing.h_total - dm->timing.h_display) / 4;
	/* v_pos */
	adj->v_pos.max_value = ((masked_mode == B_TV_OUT_NTSC_J) || (masked_mode == B_TV_OUT_NTSC)) ? 262 : 313;
	adj->v_pos.default_value = (i != TV_MODE_COUNT) ? TV_BASE_MODES[i].v_pos : 0;
	/* start active */
	adj->start_active_video.max_value = 0x1ff;
	adj->start_active_video.default_value = (i != TV_MODE_COUNT) ? TV_BASE_MODES[i].start_active_video : dm->timing.h_total - dm->timing.h_sync_start;
	/* black level */
	/* range is 90 -> 208 */
	adj->black_level.max_value = 118;
	adj->black_level.default_value = ((masked_mode == B_TV_OUT_NTSC_J) ? 100 : ((masked_mode == B_TV_OUT_PAL) ? 105 : 127)) - 90;

	/* contrast */
	adj->contrast.max_value = 0x07;
	adj->contrast.default_value = 0x03;
	
	/* all done */
	return B_OK;
}

status_t 
GET_TV_OUT_ADJUSTMENTS(uint32 *which, tv_out_adjustments *adj)
{
	durango_ch7005_io dci;
	uint8 data[0x22];
	status_t result;

	/* if the current mode is not a TV mode, bail with error */

	/* nuke all values to zero */
	memset(adj, 0, sizeof(tv_out_adjustments));
	*which = 0;

	/* get the parameters from the chip */
	dci.data = data;
	dci.count = 0x22;
	dci.index = 0x00;
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	result = ioctl(file_handle, DURANGO_READ_CH7005, &dci, sizeof(dci));
	if (result != B_OK) return result;

	/* B_TV_OUT_H_POS */
	*which |= B_TV_OUT_H_POS;
	adj->h_pos = (uint16)data[0x0A] | (((uint16)data[0x08] & 0x02) << 7);
	
	/* B_TV_OUT_V_POS */
	*which |= B_TV_OUT_V_POS;
	adj->v_pos = (uint16)data[0x0B] | (((uint16)data[0x08] & 0x01) << 8);
	
	/* B_TV_OUT_START_ACTIVE */
	*which |= B_TV_OUT_START_ACTIVE;
	adj->start_active_video = (uint16)data[0x07] | (((uint16)data[0x08] & 0x04) << 6);

	/* B_TV_OUT_BLACK_LEVEL (aka brightness) */
	*which |= B_TV_OUT_BLACK_LEVEL;
	adj->black_level = ((uint16)data[0x09]) - 90;

	/* B_TV_OUT_CONTRAST */
	*which |= B_TV_OUT_CONTRAST;
	adj->contrast = (uint16)data[0x11] & 0x07;

	return B_OK;
}

status_t 
SET_TV_OUT_ADJUSTMENTS(uint32 which, tv_out_adjustments *adj)
{
	durango_ch7005_io dci;
	uint8 data[0x22];
	status_t result;
	uint16 value;

	/* if the current mode is not a TV mode, bail with error */

	/* get the parameters from the chip */
	dci.data = data;
	dci.count = 0x22;
	dci.index = 0x00;
	dci.magic = DURANGO_PRIVATE_DATA_MAGIC;
	result = ioctl(file_handle, DURANGO_READ_CH7005, &dci, sizeof(dci));
	if (result != B_OK) return result;

	/* these three share regs */
	if (which & (B_TV_OUT_H_POS | B_TV_OUT_V_POS | B_TV_OUT_START_ACTIVE))
	{
		if (which & B_TV_OUT_H_POS)
		{
			/* limit to 0 -> 511 */
			value = adj->h_pos & 0x1ff;
			data[0x0A] = value & 0xff;
			/* position overflow */
			data[0x08] &= ~0x02;
			data[0x08] |= ((value & 0x100) >> 7);
		}
		if (which & B_TV_OUT_V_POS)
		{
			/* limit to 0 -> 511 */
			value = adj->v_pos & 0x1ff;
			data[0x0B] = value & 0xff;
			/* position overflow */
			data[0x08] &= ~0x01;
			data[0x08] |= ((value & 0x100) >> 8);
		}
		if (which & B_TV_OUT_START_ACTIVE)
		{
			/* limit to 0 -> 511 */
			value = adj->start_active_video & 0x1ff;
			data[0x07] = value & 0xff;
			/* position overflow */
			data[0x08] &= ~0x04;
			data[0x08] |= ((value & 0x100) >> 6);
		}
		dci.count = 5;
		dci.index = 0x7;
		dci.data = data + dci.index;
		result = ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
		if (result != B_OK) return result;
	}
	if (which & B_TV_OUT_BLACK_LEVEL)
	{
		value = adj->black_level;
		if (value > 118) value = 118;
		value += 90;
		data[0x09] = value & 0xff;
		dci.count = 1;
		dci.index = 0x9;
		dci.data = data + dci.index;
		result = ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
		if (result != B_OK) return result;
	}
	if (which & B_TV_OUT_CONTRAST)
	{
		value = adj->contrast;
		if (value > 7) value = 7;
		data[0x11] &= ch7005_reg_masks[0x11];
		data[0x11] |= value & 0x7;
		dci.count = 1;
		dci.index = 0x9;
		dci.data = data + dci.index;
		result = ioctl(file_handle, DURANGO_WRITE_CH7005, &dci, sizeof(dci));
		if (result != B_OK) return result;
	}
	return B_OK;
}
#endif

#if ONLY_PRESET_TIMINGS
static void display_mode_from_regs(display_mode *dm)
{
	uint32 dcfg = READ_VID32(CS5530_DISPLAY_CONFIG);

	dm->timing.flags = 0;
	if ((dcfg & CS5530_DCFG_CRT_HSYNC_POL)== 0) dm->timing.flags |= B_POSITIVE_HSYNC;
	if ((dcfg & CS5530_DCFG_CRT_VSYNC_POL)== 0) dm->timing.flags |= B_POSITIVE_VSYNC;
	
	dm->timing.pixel_clock = ((uint64)gfx_get_clock_frequency() * 1000) >> 16;

	dm->timing.h_display = gfx_get_hactive();
	dm->timing.h_sync_start = gfx_get_hsync_start();
	dm->timing.h_sync_end = gfx_get_hsync_end();
	dm->timing.h_total = gfx_get_htotal();

	dm->timing.v_display = gfx_get_vactive();
	dm->timing.v_sync_start = gfx_get_vsync_start();
	dm->timing.v_sync_end = gfx_get_vsync_end();
	dm->timing.v_total = gfx_get_vtotal();

	dm->virtual_width = dm->timing.h_display;
	dm->virtual_height = dm->timing.v_display;
	dm->h_display_start = dm->v_display_start = 0;
	dm->flags = B_HARDWARE_CURSOR | B_PARALLEL_ACCESS;
}
#endif

static void make_mode_list(void) {
#if ONLY_PRESET_TIMINGS
#else
	uint32 i;
#endif
	display_mode *dm;

	dm = mode_list = (display_mode *)calloc(
#if ONLY_PRESET_TIMINGS
		1
#else
		(
		(sizeof(DisplayParams) / sizeof(DisplayParams[0]))
#if INDEXED_COLOR_SUPPORT
		* 2
#endif
		)
#if CH7005_SUPPORT
		+ TV_MODE_COUNT
#endif
#endif
		, sizeof(display_mode));

#if ONLY_PRESET_TIMINGS
	display_mode_from_regs(dm);
	dm->space = B_RGB16;
	mode_count = 1;
#if DEBUG > 0
	dump_mode(dm);
#endif
#else
	mode_count = 0;

#if INDEXED_COLOR_SUPPORT
	for (i = 0 ; i < sizeof(DisplayParams) / sizeof(DisplayParams[0]) ; i++) {
		if (DisplayParams[i].flags & GFX_MODE_8BPP)
		{
			parms_to_mode(&(DisplayParams[i]), dm);
			dm->space = B_CMAP8;
			dm++; mode_count++;
		}
	}
#endif

	// 16bit modes
	for (i = 0 ; i < sizeof(DisplayParams) / sizeof(DisplayParams[0]) ; i++) {
		if (DisplayParams[i].flags & GFX_MODE_16BPP)
		{
			parms_to_mode(&(DisplayParams[i]), dm);
			dm->space = B_RGB16;
			dm++; mode_count++;
		}
	}

#if CH7005_SUPPORT
	// TV modes
	for (i = 0 ; i < TV_MODE_COUNT ; i++)
	{
		*dm++ = TV_BASE_MODES[i].dm;
		mode_count++;
	}
#endif

#endif
}

static status_t lock_driver(int state)
{
	durango_set_bool_state sbs;
	sbs.magic = DURANGO_PRIVATE_DATA_MAGIC;
	sbs.do_it = state ? 1 : 0;
	return ioctl(file_handle, DURANGO_LOCK, &sbs, sizeof(sbs));
}

static status_t durango_set_mode(display_mode *dm) {
#if CH7005_SUPPORT
	uint32 i = 0;
#endif
	uint32 temp32;
	display_timing *dt;
#if ONLY_PRESET_TIMINGS
	display_mode a_mode = *mode_list;
#else
	display_mode a_mode = *dm;
#endif
	status_t result;

	/* make sure we don't go to sleep while setting the mode */
	lock_driver(1);
	/* throw away any existing frame buffer allocations */
	if (ai->fb_spec.ms_MR.mr_Size)
	{
		result = DURANGO_FREE_MEM(&ai->fb_spec);
		if (result != B_OK)
		{
			lock_driver(0);
			return result;
		}
		memset(&(ai->fb_spec), 0, sizeof(ai->fb_spec));
	}
	/* and the compression buffer spec */
	if (ai->compression_spec.ms_MR.mr_Size)
	{
		result = DURANGO_FREE_MEM(&ai->compression_spec);
		if (result != B_OK)
		{
			lock_driver(0);
			return result;
		}
		memset(&(ai->compression_spec), 0, sizeof(ai->compression_spec));
	}

#if CH7005_SUPPORT
	if (dm->flags & B_TV_OUT_MASK)
	{
		/* match this mode against one of ours? */
		for (i = 0; i < TV_MODE_COUNT; i++)
		{
			if ((dm->timing.pixel_clock == TV_BASE_MODES[i].dm.timing.pixel_clock) &&
				(dm->timing.h_display == TV_BASE_MODES[i].dm.timing.h_display) &&
				(dm->timing.v_display == TV_BASE_MODES[i].dm.timing.v_display) &&
				((dm->flags & B_TV_OUT_MASK) == (TV_BASE_MODES[i].dm.flags & B_TV_OUT_MASK))) // match the TV-out mode, too
			{
	#if DEBUG < 2
				a_mode = TV_BASE_MODES[i].dm;
	#endif
				break;
			}
		}
	}
#endif

	dt = &(a_mode.timing);

#if DEBUG > 0
	xprintf(("durango_set_mode() - setting\n"));
	dump_mode(&a_mode);
#endif

	/* set the mode or die trying */
	{
	uint32 flags = GFX_MODE_NEG_HSYNC | GFX_MODE_NEG_VSYNC; // negative both syncs
	if (dt->flags & B_POSITIVE_HSYNC) flags &= ~GFX_MODE_NEG_HSYNC; // clear negative HSYNC
	if (dt->flags & B_POSITIVE_VSYNC) flags &= ~GFX_MODE_NEG_VSYNC; // clear negative VSYNC
	if (dt->flags & B_TIMING_INTERLACED) flags |= GFX_MODE_INTERLACED;
#if CH7005_SUPPORT
	// FIXME: devices doing BOTH LCD and TV output need work.
	if (a_mode.flags & B_TV_OUT_MASK)
		flags |= GFX_MODE_PANEL_ON;
	// turn tv off to switch modes
	ch7005_disable_tv_out();
#else
	#if ENABLE_FLAT_PANEL
	flags |= GFX_MODE_PANEL_ON;
	#endif
#endif

	/* allocate frame buffer */
	temp32 = a_mode.timing.h_display * 2;
	ai->fbc.bytes_per_row = temp32 <= 1024 ? 1024 : 2048;
	ai->fb_spec.ms_MR.mr_Size = ai->fbc.bytes_per_row * a_mode.timing.v_display;
	/* we must allocate the frame buffer at offset 0 for compression to work */
	ai->fb_spec.ms_AddrCareBits = ~0;
	ai->fb_spec.ms_AddrStateBits = 0;
	result = DURANGO_ALLOC_MEM(&ai->fb_spec);
	if (result != B_OK)
	{
		memset(&(ai->fb_spec), 0, sizeof(ai->fb_spec));
		lock_driver(0);
		return result;
	}
		
	if (gfx_set_display_timings(
		16, flags,
		dt->h_display, dt->h_display, dt->h_sync_start, dt->h_sync_end, dt->h_total, dt->h_total,
		dt->v_display, dt->v_display, dt->v_sync_start, dt->v_sync_end, dt->v_total, dt->v_total,
		(uint32)(
#if DOUBLE_CLOCK
		2 *		/* double the clock rate */
#endif
		((uint64)dt->pixel_clock << 16) / 1000
		)
#if DOUBLE_CLOCK
		| 0x80000000 /* but divide by two for the display */
#endif
		) != 0)
		{
			lock_driver(0);
			return B_ERROR;
		}
#if CH7005_SUPPORT
	/* set the tv-out */
	if (a_mode.flags & B_TV_OUT_MASK)
		ch7005_set_tv_out_mode(i, &a_mode);
	else
		ch7005_disable_tv_out();
#endif
	}

	ai->fbc.frame_buffer = si->framebuffer;
	ai->fbc.frame_buffer_dma = si->framebuffer_dma;
	/* compression buffer location and pitch */
	if (ai->fbc.bytes_per_row - temp32 >= 256)
	{
		/* compression buffer is in-line */
		gfx_set_compression_pitch(ai->fbc.bytes_per_row);
		gfx_set_compression_offset(temp32);
	}
	else
	{
		/* allocate the compression buffer */
		ai->compression_spec.ms_MR.mr_Size = 256 * a_mode.timing.v_display;
		/* 256 byte alignment */
		ai->compression_spec.ms_AddrCareBits = 255;
		ai->compression_spec.ms_AddrStateBits = 0;
		result = DURANGO_ALLOC_MEM(&ai->compression_spec);
		if (result != B_OK)
		{
			memset(&(ai->compression_spec), 0, sizeof(ai->compression_spec));
			lock_driver(0);
			return result;
		}
		gfx_set_compression_pitch(256);
		gfx_set_compression_offset(ai->compression_spec.ms_MR.mr_Offset);
	}
	/* compression buffer is always the same size */
	gfx_set_compression_size(256);
	
	a_mode.h_display_start = a_mode.v_display_start = 0;
	/* this enables compression, if it can */
	gfx_set_compression_enable(1);
	gfx_set_display_offset(0);

	/* record for posterity */
	ai->dm = a_mode;

	/* we're only doing 16bpp modes */
	// FIXME: 8 bpp support
	gfx_set_bpp(16);
	
#if DEBUG > 0
	{
	frame_buffer_config fbc;
	durango_get_fb_config(&fbc);
	paint_for_blit(&(ai->dm), &fbc);
	}
#endif
	xprintf(("durango_set_mode() completes\n"));
	lock_driver(0);
	return B_OK;
}

/*----------------------------------------------------------
 acceleration hooks
----------------------------------------------------------*/

static void scrn2scrn(engine_token *et, blit_params *list, uint32 count) {
	/* avoid unused warning */
	(void)et;
	gfx_set_solid_pattern(0);
	gfx_set_raster_operation(0xCC); /* SRCCOPY */
	while (count--) {
		gfx_screen_to_screen_blt(list->src_left, list->src_top, list->dest_left, list->dest_top, list->width+1, list->height+1);
		list++;
	}
	ai->fifo_count++;
	return;
}

static void rectfill16(engine_token *et, uint32 color, fill_rect_params *list, uint32 count) {
	/* avoid unused warning */
	(void)et;
	gfx_set_solid_pattern(color);
	gfx_set_raster_operation(0xF0); /* PATCOPY */
	while (count--) {
		gfx_pattern_fill(list->left, list->top, list->right - list->left + 1, list->bottom - list->top + 1);
		list++;
	}
	ai->fifo_count++;
	return;
}

static void spanfill16(engine_token *et, uint32 color, uint16 *list, uint32 count) {
	/* avoid unused warning */
	(void)et;
	gfx_set_solid_pattern(color);
	gfx_set_raster_operation(0xF0); /* PATCOPY */
	while (count--) {
		uint32 y = (uint32)(*list++);
		uint32 left = (uint32)(*list++);
		uint32 right = (uint32)(*list++);
		gfx_pattern_fill(left, y, right - left + 1, 1);
	}
	ai->fifo_count++;
	return;
}

static uint32 durango_get_engine_count(void) {
	xprintf(("durango_get_engine_count()\n"));
	return 1;
}

static status_t durango_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et) {
	/* avoid unused warning */
	(void)caps;(void)max_wait;

	/* acquire the shared benaphore */
	ACQUIRE_BEN(ai->engine);

	yprintf(("durango_acquire_engine(st:0x%08x, et:0x%08x)\n", st, et));
	/* sync if required */
	if (st) durango_sync_to_token(st);

	/* return an engine_token */
	*et = &durango_engine_token;
	return B_OK;
}

static status_t durango_release_engine(engine_token *et, sync_token *st) {

	yprintf(("durango_release_engine(et:0x%08x, st:0x%08x)\n", et, st));
	if (!et) {
		yprintf(("ackthp! durango_release_engine() called with null engine_token!\n"));
		return B_ERROR;
	}
	/* update the sync token, if any */
	if (st) {
		yprintf(("updating sync token - id: %d, fifo %Ld\n", et->engine_id, ai->fifo_count));
		st->engine_id = et->engine_id;
		st->counter = ai->fifo_count;
	}

	/* release the shared benaphore */
	RELEASE_BEN(ai->engine);
	yprintf(("durango_release_engine() completes\n"));
	return B_OK;
}

static void durango_wait_engine_idle(void) {
	yprintf(("durango_wait_engine_idle()\n"));
	GFX_WAIT_BUSY;
}

static status_t durango_get_sync_token(engine_token *et, sync_token *st) {
	yprintf(("durango_get_sync_token(et:0x%08x, st:0x%08x)\n", et, st));
	st->engine_id = et->engine_id;
	st->counter = ai->fifo_count;
	yprintf(("durango_get_sync_token() completes\n"));
	return B_OK;
}

static status_t durango_sync_to_token(sync_token *st) {
	/* avoid unused warning */
	(void)st;

	yprintf(("durango_sync_to_token(st: 0x%08x)\n", st));
	durango_wait_engine_idle();
	yprintf(("durango_sync_to_token() completes\n"));
	return B_OK;
}

static const uint32 overlay_spaces[] = { B_YCbCr422, B_NO_COLOR_SPACE };
static const uint16      pitch_max[] = { 4092,       0 };

uint32 
OVERLAY_COUNT(const display_mode *dm)
{
	/* avoid unused warning */
	(void)dm;

	return 1;
}

const uint32 *
OVERLAY_SUPPORTED_SPACES(const display_mode *dm)
{
	/* avoid unused warning */
	(void)dm;
	return overlay_spaces;
}

uint32
OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space)
{
	uint32 result;

	switch (a_color_space) {
		case B_YCbCr422:
			result =
				B_OVERLAY_COLOR_KEY |
/*				B_OVERLAY_CHROMA_KEY | */
				B_OVERLAY_HORIZONTAL_FITLERING |
				B_OVERLAY_VERTICAL_FILTERING;
			break;
		default:
			result = 0;
			break;
	}
	return result;
}

#define MAX_BUFFERS (sizeof(ai->ovl_buffers) / sizeof(ai->ovl_buffers[0]))

const overlay_buffer *
ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height)
{
	uint32 i, index;
	uint32 space;
	uint32 bytes_per_pixel = 0;
	uint32 pitch_mask = 0;
	BMemSpec *ms;
	overlay_buffer *ob;

	xprintf(("ALLOCATE_OVERLAY_BUFFER(%s, %d, %d)\n", spaceToString(cs), width, height));

	if (!width || !height) return 0;
	if (cs == B_NO_COLOR_SPACE) return 0;

	/* find an empty overlay buffer */
	for (index = 0; index < MAX_BUFFERS; index++)
		if (ai->ovl_buffers[index].space == B_NO_COLOR_SPACE)
			break;

	xprintf(("%ld == %ld : %s\n", index, MAX_BUFFERS, index == MAX_BUFFERS ? "true" : "false"));

	/* all in use - then fail */
	if (index == MAX_BUFFERS) return 0;
	ob = &(ai->ovl_buffers[index]);

	/* otherwise try and make a buffer */

	/* validate color_space */
	i = 0;
	while (overlay_spaces[i]) {
		if (overlay_spaces[i] == cs) break;
		i++;
	}
	
	xprintf(("overlay_spaces[%ld] == B_NO_COLOR_SPACE: %s\n", i, overlay_spaces[i] == B_NO_COLOR_SPACE ? "true" : "false"));
	xprintf(("%d > %d : %s\n", width, pitch_max[i], width > pitch_max[i] ? "true" : "false"));

	if (overlay_spaces[i] == B_NO_COLOR_SPACE) return 0;
	if (width > pitch_max[i]) return 0;

	space = overlay_spaces[i];

	// calculate buffer size
	switch (space) {
	case B_YCbCr422:
		bytes_per_pixel = 2;
		pitch_mask = 1;	/* bits which must be clear */
		break;
	}
	/* make width be a proper multiple for the back-end scaler */
	width = (width + pitch_mask) & ~pitch_mask;
	ob->bytes_per_row = bytes_per_pixel * width;
	ms = &(ai->ovl_buffer_specs[index]);
	ms->ms_MR.mr_Size = ((ob->bytes_per_row * height) + 63) & ~63;
	/* max of 1MB for video buffer */
	if (ms->ms_MR.mr_Size > (1024 * 1024)) return 0;
	/* require 16 byte alignment */
	ms->ms_AddrCareBits = 0x0f;
	ms->ms_AddrStateBits = 0;
	ms->ms_AllocFlags = 0;
	if (DURANGO_ALLOC_MEM(ms) != B_OK) return 0;

	ob->space = space;
	ob->width = width;
	ob->height = height;
	ob->buffer = (void *)((uint8*)si->framebuffer + ms->ms_MR.mr_Offset);
	ob->buffer_dma = (void *)((uint8*)si->framebuffer_dma + ms->ms_MR.mr_Offset);
	xprintf(("ALLOCATE_OVERLAY_BUFFER: returning overlay_buffer %p\n", ob));
	xprintf((" framebuffer start: %p\n", si->framebuffer));
	xprintf(("    overlay buffer: %p\n", ob->buffer));
	xprintf(("overlay buffer_dma: %p\n", ob->buffer_dma));
	xprintf(("compression buffer: %p\n", ((uint8*)si->framebuffer + ai->compression_spec.ms_MR.mr_Offset)));
	return ob;
}

status_t 
RELEASE_OVERLAY_BUFFER(const overlay_buffer *_ob)
{
	overlay_buffer *ob = (overlay_buffer *)_ob;
	uint32 index = ob - ai->ovl_buffers;

	/* validated buffer */
	if (index >= MAX_BUFFERS) {
		xprintf(("RELEASE_OVERLAY_BUFFER: bad buffer pointer\n"));
		return B_ERROR;
	}
	if (ob->space == B_NO_COLOR_SPACE) {
		xprintf(("RELEASE_OVERLAY_BUFFER: releasing unallocated buffer\n"));
		return B_ERROR;
	}

	/* free the memory */
	DURANGO_FREE_MEM(&(ai->ovl_buffer_specs[index]));
	/* mark as empty */
	ob->space = B_NO_COLOR_SPACE;
	xprintf(("RELEASE_OVERLAY_BUFFER: OK\n"));
	return B_OK;
}

status_t
GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc)
{
	/* avert unused warning */
	(void)dm;

	/* minimum size of view */
	oc->view.width.min = 4;
	oc->view.height.min = 1;
	/* view may not be larger than the buffer or 768x1024, which ever is lower */
	oc->view.width.max = ob->width > 768 ? 768 : ob->width;
	oc->view.height.max = ob->height > 1024 ? 1024 : ob->height;
	/* view alignment */
	oc->view.h_alignment = 3;
	oc->view.v_alignment = 0;
	oc->view.width_alignment = 3;
	oc->view.height_alignment = 0;
	
	/* minium size of window */
	oc->window.width.min = 2;
	oc->window.height.min = 2;
	/* upper usefull size is limited by screen realestate */
	oc->window.width.max = 4096;
	oc->window.height.max = 4096;
	/* window alignment */
	oc->window.h_alignment = 0;
	oc->window.v_alignment = 0;
	oc->window.width_alignment = 3;
	oc->window.height_alignment = 0;
	
	/* overall scaling factors */
	oc->h_scale.min = 1.0;	/* no down scaling for the CS55x0 */
	oc->v_scale.min = 1.0;
	oc->h_scale.max = 8.0;
	oc->v_scale.max = 8.0;

	return B_OK;
}

#define MAX_OVERLAYS (sizeof(ai->ovl_tokens) / sizeof(ai->ovl_tokens[0]))

overlay_token 
ALLOCATE_OVERLAY(void)
{
	uint32 index;
	/* find an unused overlay token */
	for (index = 0; index < MAX_OVERLAYS; index++)
		if (ai->ovl_tokens[index].used == 0) break;
	if (index >= MAX_OVERLAYS) {
		xprintf(("ALLOCATE_OVERLAY: no more overlays\n"));
		return 0;
	}

	/* mark it in use */
	ai->ovl_tokens[index].used++;
	xprintf(("ALLOCATE_OVERLAY: returning %p\n", &(ai->ovl_tokens[index])));
	return (overlay_token)&(ai->ovl_tokens[index]);
}

status_t 
RELEASE_OVERLAY(overlay_token ot)
{
	status_t result;
	durango_overlay_token *aot = (durango_overlay_token *)ot;
	uint32 index = aot - &(ai->ovl_tokens[0]);

	if (index >= MAX_OVERLAYS) {
		xprintf(("RELEASE_OVERLAY: bad overlay_token\n"));
		return B_ERROR;
	}
	if (!ai->ovl_tokens[index].used) {
		xprintf(("RELEASE_OVERLAY: releasing unused overlay\n"));
		return B_ERROR;
	}
	/* de-configure overlay */
	result = CONFIGURE_OVERLAY(ot, NULL, NULL, NULL);
	/* mark unused */
	ai->ovl_tokens[index].used = 0;
	xprintf(("RELEASE_OVERLAY: returning %ld\n", result));
	return result;
}

status_t 
CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov)
{
	durango_overlay_token *aot = (durango_overlay_token *)ot;
	uint32 index = aot - &(ai->ovl_tokens[0]);
	status_t result;
	uint32 key_color = 0;
	uint32 key_mask = 0;

	/* validate the overlay token */
	if (index >= MAX_OVERLAYS) {
		xprintf(("CONFIGURE_OVERLAY: invalid overlay_token\n"));
		return B_ERROR;
	}
	if (!aot->used) {
		xprintf(("CONFIGURE_OVERLAY: trying to configure un-allocated overlay\n"));
		return B_ERROR;
	}

	if ((ob == 0) || (ow == 0) || (ov == 0)) {
		/* disable overlay */
		gfx_set_video_enable(0);
		/* note disabled state for virtual desktop sliding */
		aot->ob = 0;
		xprintf(("CONFIGURE_OVERLAY: disabling overlay\n"));
		return B_OK;
	}

	xprintf(("overlay_buffer: %u x %u\n", ob->width, ob->height));
	xprintf(("overlay_window: %u x %u @ %i,%i (%u %u %u %u)\n", ow->width, ow->height, ow->h_start, ow->v_start, ow->offset_left, ow->offset_top, ow->offset_right, ow->offset_bottom));
	xprintf(("overlay_view  : %u x %u @ %i,%i\n", ov->width, ov->height, ov->h_start, ov->v_start));
	/* check scaling limits, etc. */
	/* make sure the view fits in the buffer */
	if (((ov->width + ov->h_start) > ob->width) ||
		((ov->height + ov->v_start) > ob->height)) {
		xprintf(("CONFIGURE_OVERLAY: overlay_view does not fit in overlay_buffer\n"));
		return B_ERROR;
	}

	switch (ob->space) {
		case B_YCbCr422:
			result = gfx_set_video_format(2);
			xprintf(("gfx_set_video_format() returns %.8lx\n", result));
			break;
		default:
			return B_ERROR;
	}
	if (ow->flags & B_OVERLAY_COLOR_KEY)
	{
		/* */
		key_color = ow->red.value << 3; // 8 - bits per component
		key_color <<= 8;
		key_color |= ow->green.value << 2;
		key_color <<= 8;
		key_color |= ow->blue.value << 3;
		key_mask = ow->red.mask << 3;
		key_mask <<= 8;
		key_mask |= ow->green.mask << 2;
		key_mask <<= 8;
		key_mask |= ow->blue.mask << 3;
	}
#define BYTES_PER_PIXEL 2
	result = gfx_set_video_offset(
		((uint8 *)ob->buffer - (uint8 *)si->framebuffer) +
		(ov->v_start * ob->bytes_per_row) +
		(ov->h_start * BYTES_PER_PIXEL)
	);
	xprintf(("gfx_set_video_offset(%.8lx) returns %.8lx\n",
		((uint8 *)ob->buffer - (uint8 *)si->framebuffer) +
		(ov->v_start * ob->bytes_per_row) +
		(ov->h_start * BYTES_PER_PIXEL),
		result));
	result = gfx_set_video_size(ob->width, ob->height);
	xprintf(("gfx_set_video_size() returns %.8lx\n", result));
	result = gfx_set_video_color_key(key_color, key_mask, 1 /* 1 == color key */);
	xprintf(("gfx_set_video_color_key() returns %.8lx\n", result));
	result = gfx_set_video_palette(0); /* no gamma correction */
	xprintf(("gfx_set_video_palette() returns %.8lx\n", result));
	result = gfx_set_video_filter(ow->flags & B_OVERLAY_HORIZONTAL_FITLERING, ow->flags & B_OVERLAY_VERTICAL_FILTERING);
	xprintf(("gfx_set_video_filter() returns %.8lx\n", result));
	/* the ratio of view to window sets the scale */
	result = gfx_set_video_scale(ov->width, ov->height, ow->width, ow->height);
	xprintf(("gfx_set_video_scale() returns %.8lx\n", result));
	xprintf(("gfx_get_video_scale() : %.8lx\n", gfx_get_video_scale()));
	/* the actual pixels touched by the overlay */
	result = gfx_set_video_window(
		ow->h_start + ow->offset_left,
		ow->v_start + ow->offset_top,
		ow->width - (ow->offset_left + ow->offset_right),
		ow->height - (ow->offset_top + ow->offset_bottom)
	);
	xprintf(("gfx_set_video_window(%u,%u, %u,%u) returns %.8lx\n",
		ow->h_start + ow->offset_left,
		ow->v_start + ow->offset_top,
		ow->width - (ow->offset_left + ow->offset_right),
		ow->height - (ow->offset_top + ow->offset_bottom),
		result));
	//WRITE_VID32(CS5530_VIDEO_SCALE, 0x03000300);
	result = gfx_set_video_enable(1);
	xprintf(("gfx_set_video_enable() returns %.8lx\n", result));
#if DEBUG > 1
	{
	uint32 i;
	xprintf(("CS5530 regs:\n"));
	for (i = CS5530_VIDEO_CONFIG; i < CS5530_PALETTE_ADDRESS; i += 4)
		xprintf(("%.04lx %.08lx\n", i, READ_VID32(i)));
	xprintf(("GX regs:\n"));
	for (i = DC_GENERAL_CFG; i < DC_CURSOR_X; i += 4)
		xprintf(("%.04lx %.08lx\n", i, READ_REG32(i)));
	}
#endif
	return B_OK;
}

static status_t 
DURANGO_ALLOC_MEM(BMemSpec *ms)
{
	status_t result;
	durango_alloc_mem dam;
	dam.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dam.ms = ms;
	result = ioctl(file_handle, DURANGO_ALLOC, &dam, sizeof(dam));
	xprintf(("DURANGO_ALLOC_MEM(%p) returns %ld (%p)\n", ms, result, (void*)result));
	return result;
}

static status_t 
DURANGO_FREE_MEM(BMemSpec *ms)
{
	status_t result;
	durango_alloc_mem dam;
	dam.magic = DURANGO_PRIVATE_DATA_MAGIC;
	dam.ms = ms;
	result = ioctl(file_handle, DURANGO_FREE, &dam, sizeof(dam));
	xprintf(("DURANGO_FREE_MEM(%p) returns %ld (%p)\n", ms, result, (void*)result));
	return result;
}


