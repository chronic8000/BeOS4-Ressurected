/*-----------------------------------------------------------------------------
 * GFX_MODE.H
 *
 * Version 2.0 - February 21, 2000
 *
 * This header file contains the mode tables.  It is used by the "gfx_disp.c" 
 * file to set a display mode.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* MODE FLAGS (BITWISE-OR) */

#define GFX_MODE_8BPP		0x00000001
#define GFX_MODE_16BPP		0x00000002
#define GFX_MODE_60HZ		0x00000004
#define GFX_MODE_75HZ		0x00000008
#define GFX_MODE_85HZ		0x00000010
#define GFX_MODE_NEG_HSYNC	0x00000020
#define GFX_MODE_NEG_VSYNC	0x00000040
#define GFX_MODE_INTERLACED 0x00000080
#define GFX_MODE_PANEL_ON	0x00000100

/* STRUCTURE DEFINITION */

typedef struct tagDISPLAYMODE
{
	/* DISPLAY MODE FLAGS */
	/* Specify valid color depths and the refresh rate. */

	unsigned long flags;

	/* TIMINGS */

	unsigned short hactive;
	unsigned short hblankstart;
	unsigned short hsyncstart;
	unsigned short hsyncend;
	unsigned short hblankend;
	unsigned short htotal;

	unsigned short vactive;
	unsigned short vblankstart;
	unsigned short vsyncstart;
	unsigned short vsyncend;
	unsigned short vblankend;
	unsigned short vtotal;

	/* CLOCK FREQUENCY */
	
	unsigned long frequency;

} DISPLAYMODE;

DISPLAYMODE DisplayParams[] = {

#if ONLY_PRESET_TIMINGS
#else

#if NORMAL_MODES || LCD_800_600
/* 640x480 */

{ GFX_MODE_60HZ |									/* refresh rate = 60  */
  GFX_MODE_8BPP | GFX_MODE_16BPP |					/* 8 and 16 BPP valid */
  GFX_MODE_NEG_HSYNC | GFX_MODE_NEG_VSYNC,			/* negative syncs     */
  0x0280, 0x0288, 0x0290, 0x02E8, 0x0318, 0x0320,	/* horizontal timings */
  0x01E0, 0x01E8, 0x01EA, 0x01EC, 0x0205, 0x020D,   /* vertical timings   */
  0x00192CCC, 										/* freq = 25.175 MHz  */
},

{ GFX_MODE_75HZ |									/* refresh rate = 75  */
  GFX_MODE_8BPP | GFX_MODE_16BPP |					/* 8 and 16 BPP valid */
  GFX_MODE_NEG_HSYNC | GFX_MODE_NEG_VSYNC,			/* negative syncs     */
  0x0280, 0x0280, 0x0290, 0x02D0, 0x0348, 0x0348,	/* horizontal timings */
  0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01F4, 0x01F4,	/* vertical timings   */
  0x001F8000, 										/* freq = 31.5 MHz    */
},

{ GFX_MODE_85HZ |									/* refresh rate = 85  */
  GFX_MODE_8BPP | GFX_MODE_16BPP |					/* 8 and 16 BPP valid */
  GFX_MODE_NEG_HSYNC | GFX_MODE_NEG_VSYNC,			/* negative syncs     */
  0x0280, 0x0280, 0x02B8, 0x02F0, 0x0340, 0x0340,   /* horizontal timings */
  0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01FD, 0x01FD,   /* vertical timings   */
  0x00240000,  									    /* freq = 36.0 MHz    */
},

/* 800x600 */

{ GFX_MODE_60HZ |									/* refresh rate = 60  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0320, 0x0328, 0x0348, 0x03D0, 0x0418, 0x0420,	/* horizontal timings */
  0x0258, 0x0258, 0x0259, 0x025D, 0x0274, 0x0274,   /* vertical timings   */
  0x00280000, 										/* freq = 40.00 MHz   */
},

{ GFX_MODE_75HZ |									/* refresh rate = 75  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0320, 0x0320, 0x0330, 0x0380, 0x0420, 0x0420,   /* horizontal timings */
  0x0258, 0x0258, 0x0259, 0x025C, 0x0271, 0x0271,	/* vertical timings   */
  0x00318000, 										/* freq = 49.5 MHz    */
},

{ GFX_MODE_85HZ |									/* refresh rate = 85  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0320, 0x0320, 0x0340, 0x0380, 0x0418, 0x0418,   /* horizontal timings */
  0x0258, 0x0258, 0x0259, 0x025C, 0x0277, 0x0277,   /* vertical timings   */
  0x00384000, 									    /* freq = 56.25 MHz   */
},

/* 1024x768 */

{ GFX_MODE_60HZ |									/* refresh rate = 60  */
  GFX_MODE_8BPP | GFX_MODE_16BPP |					/* 8 and 16 BPP valid */
  GFX_MODE_NEG_HSYNC | GFX_MODE_NEG_VSYNC,			/* negative syncs     */
  0x0400, 0x0400, 0x0418, 0x04A0, 0x0540, 0x0540,	/* horizontal timings */
  0x0300, 0x0300, 0x0303, 0x0309, 0x0326, 0x0326,   /* vertical timings   */
  0x00410000, 									    /* freq = 65.00 MHz   */
},

{ GFX_MODE_75HZ |									/* refresh rate = 75  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0400, 0x0400, 0x0410, 0x0470, 0x0520, 0x0520,   /* horizontal timings */
  0x0300, 0x0300, 0x0301, 0x0304, 0x0320, 0x0320,   /* vertical timings   */
  0x004EC000,									    /* freq = 78.75 MHz   */
},

{ GFX_MODE_85HZ |									/* refresh rate = 85  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0400, 0x0400, 0x0430, 0x0490, 0x0560, 0x0560,   /* horizontal timings */
  0x0300, 0x0300, 0x0301, 0x0304, 0x0328, 0x0328,   /* vertical timings   */
  0x005E8000,									    /* freq = 94.50 MHz   */
},

#if INDEXED_COLOR_SUPPORT
/* 1280x1024 */

{ GFX_MODE_60HZ |									/* refresh rate = 60  */
  GFX_MODE_8BPP,									/* only 8 BPP valid   */
  0x0500, 0x0508, 0x0540, 0x05C0, 0x0698, 0x06A0,   /* horizontal timings */
  0x0400, 0x0400, 0x0401, 0x0404, 0x042A, 0x042A,   /* vertical timings   */
  0x006C0000, 										/* freq = 108.0 MHz   */
},

{ GFX_MODE_75HZ |									/* refresh rate = 75  */
  GFX_MODE_8BPP,									/* only 8 BPP valid   */
  0x0500, 0x0500, 0x0510, 0x05A0, 0x0698, 0x0698,   /* horizontal timings */
  0x0400, 0x0400, 0x0401, 0x0404, 0x042A, 0x042A,   /* vertical timings   */
  0x00870000, 										/* freq = 135.0 MHz   */
},

{ GFX_MODE_85HZ |									/* refresh rate = 85  */
  GFX_MODE_8BPP,									/* only 8 BPP valid   */
  0x0500, 0x0500, 0x0540, 0x05E0, 0x06C0, 0x06C0,   /* horizontal timings */
  0x0400, 0x0400, 0x0401, 0x0404, 0x0430, 0x0430,   /* vertical timings   */
  0x009D8000, 									    /* freq = 157.5 MHz   */
},
#endif
#endif

#if SONY_ROTATED
/* 1024x800 */

#if 0
{ GFX_MODE_60HZ |									/* refresh rate = 72Hz  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0400, 0x0400, 0x0438, 0x04B4, 0x0550, 0x0550,   /* horizontal timings */
  0x0320, 0x0320, 0x0321, 0x0324, 0x0342, 0x0342,   /* vertical timings   */
  0x00440e14,									    /* freq = 81.665 MHz   */
},

#endif
{ GFX_MODE_75HZ |									/* refresh rate = 72Hz  */
  GFX_MODE_8BPP | GFX_MODE_16BPP,					/* 8 and 16 BPP valid */
  0x0400, 0x0400, 0x0438, 0x04B4, 0x0550, 0x0550,   /* horizontal timings */
  0x0320, 0x0320, 0x0321, 0x0324, 0x0342, 0x0342,   /* vertical timings   */
  0x0051aa3d,									    /* freq = 81.665 MHz   */
},

#endif

#endif

};

#define NUM_DISPLAY_MODES sizeof(DisplayParams) / sizeof(DISPLAYMODE)

/* END OF FILE */

