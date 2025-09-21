/*---------------------------------------------------------------------------
 * GFX_RTNS.H
 *
 * Version 2.1 - March 3, 2000.
 *
 * This header file defines the Durango routines and variables used
 * to access the memory mapped regions.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */

/* COMPILER OPTION FOR C++ PROGRAMS */

#ifdef __cplusplus
extern "C" {
#endif

/* DURANGO MEMORY POITNERS */

extern unsigned char *gfx_regptr;
extern unsigned char *gfx_fbptr;
extern unsigned char *gfx_vidptr;
extern unsigned char *gfx_vipptr;

/* VGA STRUCTURE */

#define GFX_STD_CRTC_REGS 25
#define GFX_EXT_CRTC_REGS 16

#define GFX_VGA_FLAG_MISC_OUTPUT	0x00000001
#define GFX_VGA_FLAG_STD_CRTC		0x00000002
#define GFX_VGA_FLAG_EXT_CRTC		0x00000004

typedef struct {
	int xsize;
	int ysize;
	int hz;
	int clock;
	unsigned char miscOutput;
	unsigned char stdCRTCregs[GFX_STD_CRTC_REGS];
	unsigned char extCRTCregs[GFX_EXT_CRTC_REGS];
} gfx_vga_struct;

/* DURANGO VARIBLES FOR RUNTIME SELECTION AND POSSIBLE VALUES */

extern int gfx_display_type;
#define GFX_DISPLAY_TYPE_GU1		0x0001
#define GFX_DISPLAY_TYPE_GU2		0x0002

extern int gfx_2daccel_type;
#define GFX_2DACCEL_TYPE_GU1		0x0001
#define GFX_2DACCEL_TYPE_GU2		0x0002

extern int gfx_video_type;
#define GFX_VIDEO_TYPE_CS5530		0x0001
#define GFX_VIDEO_TYPE_SC1400		0x0002
#define GFX_VIDEO_TYPE_SC1200		0x0004

extern int gfx_vip_type;
#define GFX_VIP_TYPE_SC1400			0x0002
#define GFX_VIP_TYPE_SC1200			0x0004

extern int gfx_decoder_type;
#define GFX_DECODER_TYPE_SAA7114	0x0001

extern int gfx_tv_type;
#define GFX_TV_TYPE_GEODE			0x0001
#define GFX_TV_TYPE_FS451			0x0002

extern int gfx_i2c_type;
#define GFX_I2C_TYPE_ACCESS			0x0001
#define GFX_I2C_TYPE_GPIO			0x0002


/* POSSIBLE STATUS VALUES */

#define GFX_STATUS_OK				0
#define GFX_STATUS_UNSUPPORTED		1
#define GFX_STATUS_BAD_PARAMETER	2

/* ROUTINES IN GFX_INIT.C */

unsigned long gfx_pci_config_read(unsigned long address);
unsigned char gfx_gxm_config_read(unsigned char index);
unsigned long gfx_detect_cpu(void);
unsigned long gfx_detect_video(void);
unsigned long gfx_get_cpu_register_base(void);
unsigned long gfx_get_frame_buffer_base(void);
unsigned long gfx_get_frame_buffer_size(void);
unsigned long gfx_get_vid_register_base(void);
unsigned long gfx_get_vip_register_base(void);
 
/* ROUTINES IN GFX_DISP.C */

int gfx_set_display_mode(int xres, int yres, int bpp, int hz);
int	gfx_set_display_timings(unsigned short bpp, unsigned short flags,
	unsigned short hactive, unsigned short hblank_start, 
	unsigned short hsync_start, unsigned short hsync_end, 
	unsigned short hblank_end, unsigned short htotal, 
    unsigned short vactive, unsigned short vblank_start, 
	unsigned short vsync_start, unsigned short vsync_end, 
	unsigned short vblank_end, unsigned short vtotal,
	unsigned long frequency);
unsigned short gfx_get_display_pitch(void);
void gfx_set_display_pitch(unsigned short pitch);
void gfx_set_display_offset(unsigned long offset);
int gfx_set_display_palette(unsigned long *palette);
void gfx_video_shutdown(void);
void gfx_set_clock_frequency(unsigned long frequency);
unsigned long gfx_get_clock_frequency(void);
int gfx_get_sync_polarities(void);
void gfx_set_cursor_enable(int enable);
void gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor);
void gfx_set_cursor_position(unsigned long memoffset, 
	unsigned short xpos, unsigned short ypos, 
	unsigned short xhotspot, unsigned short yhotspot);
void gfx_set_cursor_shape32(unsigned long memoffset, 
	unsigned long *andmask, unsigned long *xormask);
int gfx_set_compression_enable(int enable);
int gfx_set_compression_offset(unsigned long offset);
int gfx_set_compression_pitch(unsigned short pitch);
int gfx_set_compression_size(unsigned short size);
int gfx_test_timing_active(void);
int gfx_test_vertical_active(void);
int gfx_wait_vertical_blank(void);
void gfx_delay_milliseconds(unsigned long milliseconds);

/* "READ" ROUTINES IN GFX_DISP.C */
 
unsigned short gfx_get_hactive(void);
unsigned short gfx_get_hblank_start(void);
unsigned short gfx_get_hsync_start(void);
unsigned short gfx_get_hsync_end(void);
unsigned short gfx_get_hblank_end(void);
unsigned short gfx_get_htotal(void);
unsigned short gfx_get_vactive(void);
unsigned short gfx_get_vblank_start(void);
unsigned short gfx_get_vsync_start(void);
unsigned short gfx_get_vsync_end(void);
unsigned short gfx_get_vblank_end(void);
unsigned short gfx_get_vtotal(void);
unsigned short gfx_get_display_bpp(void);
unsigned long gfx_get_display_offset(void);
void gfx_get_display_palette(unsigned long *palette);
unsigned long gfx_get_cursor_enable(void);
unsigned long gfx_get_cursor_offset(void);
unsigned long gfx_get_cursor_position(void);
unsigned long gfx_get_cursor_clip(void);
unsigned long gfx_get_cursor_color(int color);
int gfx_get_compression_enable(void);
unsigned long gfx_get_compression_offset(void);
unsigned short gfx_get_compression_pitch(void);
unsigned short gfx_get_compression_size(void);
int gfx_get_valid_bit(int line);

/* ROUTINES IN GFX_RNDR.C */

void gfx_set_bpp(unsigned short bpp);
void gfx_set_solid_pattern(unsigned long color);
void gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor, 
	unsigned long data0, unsigned long data1, unsigned char transparency);
void gfx_set_solid_source(unsigned long color);
void gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
	unsigned short transparent);
void gfx_set_raster_operation(unsigned char rop);
void gfx_pattern_fill(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height);
void gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height);
void gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned long color);
void gfx_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch); 
void gfx_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch, 
	unsigned long color); 
void gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
	unsigned short dstx, unsigned short dsty, unsigned short width, 
	unsigned short height, unsigned char *data, unsigned short pitch); 
void gfx_bresenham_line(unsigned short x, unsigned short y, 
	unsigned short length, unsigned short initerr, 
	unsigned short axialerr, unsigned short diagerr, 
	unsigned short flags);
void gfx_wait_until_idle(void);
int gfx_test_blt_pending(void);

/* ROUTINES IN GFX_VID.C */

int gfx_set_video_enable(int enable);
int gfx_set_video_format(unsigned long format);
int gfx_set_video_size(unsigned short width, unsigned short height);
int gfx_set_video_offset(unsigned long offset);
int gfx_set_video_window(short x, short y, unsigned short w, 
	unsigned short h);
int gfx_set_video_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth);
int gfx_set_video_color_key(unsigned long key, unsigned long mask, 
	int bluescreen);
int gfx_set_video_filter(int xfilter, int yfilter);
int gfx_set_video_palette(unsigned long *palette);

int gfx_select_alpha_region(int region);
int gfx_set_alpha_enable(int enable);
int gfx_set_alpha_size(unsigned short x, unsigned short y, 
	unsigned short width, unsigned short height);
int gfx_set_alpha_value(unsigned char alpha, char delta);
int gfx_set_alpha_priority(int priority);
int gfx_set_alpha_color(unsigned long color);

/* READ ROUTINES IN GFX_VID.C */

int gfx_get_video_enable(void);
int gfx_get_video_format(void);
unsigned long gfx_get_video_src_size(void);
unsigned long gfx_get_video_line_size(void);
unsigned long gfx_get_video_xclip(void);
unsigned long gfx_get_video_offset(void);
unsigned long gfx_get_video_scale(void);
unsigned long gfx_get_video_dst_size(void);
unsigned long gfx_get_video_position(void);
unsigned long gfx_get_video_color_key(void);
unsigned long gfx_get_video_color_key_mask(void);
int gfx_get_video_color_key_src(void);
int gfx_get_video_filter(void);
unsigned long gfx_read_crc(void);

void gfx_get_alpha_enable(int *enable);
void gfx_get_alpha_size(unsigned short *x, unsigned short *y, 
	unsigned short *width, unsigned short *height);
void gfx_get_alpha_value(unsigned char *alpha, char *delta);
void gfx_get_alpha_priority(int *priority);
void gfx_get_alpha_color(unsigned long *color);

/* ROUTINES IN GFX_VIP.C */

int gfx_set_vip_enable(int enable);
int gfx_set_vip_base(unsigned long even, unsigned long odd);
int gfx_set_vip_pitch(unsigned long pitch);
int gfx_set_vbi_enable(int enable);
int gfx_set_vbi_base(unsigned long even, unsigned long odd);
int gfx_set_vbi_pitch(unsigned long pitch);

/* READ ROUTINES IN GFX_VIP.C */

int gfx_get_vip_enable(void);
unsigned long gfx_get_vip_base(int even);
unsigned long gfx_get_vip_pitch(void);
int gfx_get_vbi_enable(void);
unsigned long gfx_get_vbi_base(int even);
unsigned long gfx_get_vbi_pitch(void);

/* ROUTINES IN GFX_DCDR.C */

int gfx_set_decoder_defaults(void);
int gfx_set_decoder_brightness(unsigned char brightness);
int gfx_set_decoder_contrast(unsigned char contrast);
int gfx_set_decoder_hue(char hue);
int gfx_set_decoder_saturation(unsigned char saturation);
int gfx_set_decoder_input_offset(unsigned short x, unsigned short y);
int gfx_set_decoder_input_size(unsigned short width, unsigned short height);
int gfx_set_decoder_output_size(unsigned short width, unsigned short height);
int gfx_set_decoder_scale(unsigned short srcw, unsigned short srch, 
	unsigned short dstw, unsigned short dsth);
int gfx_set_decoder_vbi_format(int format);

/* READ ROUTINES IN GFX_DCDR.C */

unsigned char gfx_get_decoder_brightness(void);
unsigned char gfx_get_decoder_contrast(void);
char gfx_get_decoder_hue(void);
unsigned char gfx_get_decoder_saturation(void);
unsigned long gfx_get_decoder_input_offset(void);
unsigned long gfx_get_decoder_input_size(void);
unsigned long gfx_get_decoder_output_size(void);
int gfx_get_decoder_vbi_format(void);

/* ROUTINES IN GFX_I2C.C */

void gfx_i2c_reset(void);
int gfx_i2c_select_bus(int bus);
int gfx_i2c_select_gpio(int clock, int data);
int gfx_i2c_write(unsigned char chipadr, unsigned char subadr, 
	unsigned char data);
int gfx_i2c_read(unsigned char chipadr, unsigned char subadr, 
	unsigned char *data);
int gfx_i2c_write_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data);
int gfx_i2c_read_multiple(unsigned char chipadr, unsigned char subadr, 
	unsigned char count, unsigned char *data);

/* ROUTINES IN GFX_TV.C */

int gfx_set_tv_defaults(int format);
int gfx_set_tv_enable(int enable);
int gfx_set_tv_cc_enable(int enable);
int gfx_set_tv_cc_data(unsigned char data1, unsigned char data2);

/* ROUTINES IN GFX_VGA.C */

int gfx_vga_test_pci(void);
unsigned char gfx_vga_get_pci_command(void);
int gfx_vga_set_pci_command(unsigned char command);
int gfx_vga_seq_reset(int reset);
int gfx_vga_set_graphics_bits(void);
int gfx_vga_mode(gfx_vga_struct *vga, int xres, int yres, int bpp, int hz);
int gfx_vga_pitch(gfx_vga_struct *vga, unsigned short pitch);
int gfx_vga_save(gfx_vga_struct *vga, int flags);
int gfx_vga_restore(gfx_vga_struct *vga, int flags);
int gfx_vga_mode_switch(int active);
void gfx_vga_clear_extended(void);

/* CLOSE BRACKET FOR C++ COMPLILATION */

#ifdef __cplusplus
}
#endif

/* END OF FILE */



