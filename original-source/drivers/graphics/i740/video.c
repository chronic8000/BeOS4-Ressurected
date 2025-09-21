/* :ts=8 bk=0
 *
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.07.13
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <graphics_p/i740/i740.h>
#include <graphics_p/i740/i740defs.h>
#include <graphics_p/i740/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define MASTERCLK	66666.66666	/*  The frequency of the Beast	*/

#define	SIZEOF_CURSORPAGE	4096


#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | \
			 B_PARALLEL_ACCESS | B_DPMS)
#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)


/*****************************************************************************
 * Local prototypes.
 */
static status_t calcmodes (struct devinfo *di);
static int testbuildmodes (struct devinfo *di, register display_mode *dst);
static void setdispoffset (register struct i740_card_info *ci,
			   register struct i740_card_ctl *cc,
			   register display_mode *dm);
static void calcfifo (uint8 *lowater_retn,
		      uint8 *burstlen_retn,
		      float busclk,
		      float pixclk,
		      int32 pixdepth);
static void setramppalette (register union vgaregs *vregs);
static void write_table (vuint8 *addr, const uint8 *table, int first, int last);
static int ClockSelect (float masterClock,
			float clockReq,
			float *clockOut,
			int *mOut,
			int *nOut,
			int *pOut);


/*****************************************************************************
 * Globals.
 */
extern pci_module_info	*pci_bus;


/*
 * This table is formatted for 132 columns, so stretch that window...
 * Stolen from Trey's R4 Matrox driver and reformatted to improve readability
 * (after a fashion).  Some entries have been updated with Intel's "official"
 * numbers.
 */
static const display_mode	mode_list[] = {
/* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
 {	{  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* 640X480X60Hz */
 {	{  27500,  640,  672,  768,  800,  480,  488,  494,  530, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* SVGA_640X480X60HzNI */
 {	{  30500,  640,  672,  768,  800,  480,  517,  523,  588, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  768,  800,  480,  489,  492,  520, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  736,  840,  480,  481,  484,  500, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
 {	{  36000,  640,  712,  768,  832,  480,  481,  484,  509, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* SVGA_800X600X56HzNI */
 {	{  38100,  800,  832,  960, 1088,  600,  602,  606,  620, 0},			B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
 {	{  40000,  800,  856,  984, 1056,  600,  601,  605,  628, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
 {	{  49500,  800,  832,  912, 1056,  600,  601,  604,  625, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
 {	{  50000,  800,  832,  912, 1056,  600,  637,  643,  666, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
 {	{  56250,  800,  848,  912, 1048,  600,  601,  604,  631, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* SVGA_1024X768X43HzI */
 {	{  46600, 1024, 1088, 1216, 1312,  384,  385,  388,  404, B_TIMING_INTERLACED},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
 {	{  65000, 1024, 1064, 1200, 1344,  768,  771,  777,  806, 0},			B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
 {	{  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806, 0},			B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
 {	{  78750, 1024, 1056, 1152, 1312,  768,  769,  772,  800, T_POSITIVE_SYNC},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
 {	{  94500, 1024, 1088, 1184, 1376,  768,  769,  772,  808, T_POSITIVE_SYNC},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
 {	{  94200, 1152, 1184, 1280, 1472,  864,  865,  868,  914, T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
 {	{ 108000, 1152, 1216, 1344, 1600,  864,  865,  868,  900, T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
 {	{ 121500, 1152, 1216, 1344, 1568,  864,  865,  868,  911, T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
 {	{ 108000, 1280, 1344, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
 {	{ 135000, 1280, 1312, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
 {	{ 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC},	B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
 {	{ 162000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
 {	{ 175500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
 {	{ 189000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
 {	{ 202500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
 {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
 {	{ 229500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))


/*
 * Initial state of chip registers.  In the case of the GCR and sequencer
 * registers, they never change.  The attribute and CRT controller registers
 * get clobbered every chance we can :-)...
 */
/* Graphics Display Controller
 */
static const uint8 gcr_table[] =
{
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	FIELDDEF (GR_MODE, SHIFTCTL, 256COLOR) |	/*  0x40  */
	 FIELDDEF (GR_MODE, ODDEVEN, SEQUENTIAL) |
	 FIELDDEF (GR_MODE, READMODE, NORMAL) |
	 FIELDVAL (GR_MODE, WRITEMODE, 0),
	FIELDDEF (GR_MISC, MEMMAP, A0_AF) |		/*  0x05  */
	 FIELDDEF (GR_MISC, CHAINODDEVEN, NORMAL) |
	 FIELDDEF (GR_MISC, DISPMODE, GRAPHICS),
	0x0F,
	0xFF
};
#define	NREGS_GCR	(sizeof (gcr_table) / sizeof (gcr_table[0]))

/* Time Sequencer
 */
static const uint8 seq_table[] =
{
	0x00,
	FIELDDEF (SR_CLOCK, TXCELLWIDTH, 8),		/*  0x01  */
	0x0F,
	0x00, 	/* Font select */
	FIELDVAL (SR_MODE, CHAIN4, TRUE) |		/*  0x0E  */
	 FIELDDEF (SR_MODE, ODDEVEN, SEQUENTIAL) |
	 FIELDDEF (SR_MODE, EXTMEM, ENABLE)
//
//	0x03,
//	0x01,
//	0x0F,
//	0x00, 	/* Font select */
//	0x06	/* Misc */
};
#define	NREGS_SEQ	(sizeof (seq_table) / sizeof (seq_table[0]))

static const uint8 attr_table_init[NREGS_ATTR] =
{
	0x00,
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x0D,
	0x0E,
	0x0F,
	FIELDDEF (AR_MODE, PAL54SRC, PALETTE) |		/*  0x41  */
	 FIELDDEF (AR_MODE, PIXWIDTH, 8) |
	 FIELDDEF (AR_MODE, PANSEL, BOTH) |
	 FIELDDEF (AR_MODE, BLINK, DISABLE) |
	 FIELDDEF (AR_MODE, LINEGR, DISABLE) |
	 FIELDDEF (AR_MODE, DISPTYPE, COLOR) |
	 FIELDDEF (AR_MODE, DISPMODE, GRAPHICS),
	0xFF,
	0x0F,
	0x00,
	0x00
};


/*****************************************************************************
 * Initialization sequence.
 */
status_t
i740_init (register struct devinfo *di)
{
	register i740_card_info	*ci;
	register vgaregs	*vregs;
	i740_card_ctl		*cc;
	status_t		ret = B_OK;
	uint8			tmp;

	ci = di->di_I740CardInfo;
	cc = di->di_I740CardCtl;

	ci->ci_CursorBase	= (void *) ci->ci_BaseAddr0;
	ci->ci_FBBase		= (void *) ((uint8 *) ci->ci_BaseAddr0 +
					    SIZEOF_CURSORPAGE);
	ci->ci_FBBase_DMA	=
	 (void *) ((uint8 *) di->di_PCI.u.h0.base_registers_pci[0] +
		   SIZEOF_CURSORPAGE);
	ci->ci_Regs		= (I740Regs *) ci->ci_BaseAddr1;
	vregs = &ci->ci_Regs->VGARegs;

	/*
	 * Select I/O address 0x3D?, enable archaic video RAM access, custom
	 * clock rate.
	 */
	vregs->w.VGA_Misc = FIELDDEF (MISC, CLKSEL, EXTRA) |
			    FIELDDEF (MISC, 0xA0000WINDOW, ENABLE) |
			    FIELDDEF (MISC, IOADDR, 3Dx);

	/*  Enable write access to standard VGA CRTC registers 0-7  */
	vregs->w.VGA_CR_Idx = CR_VSYNCEND;
	tmp = vregs->r.VGA_CR_Val;
	vregs->w.VGA_CR_Val = tmp & ~MASKEXPAND (CR_VSYNCEND_PROTECT);

	vregs->w.VGA_DACMask = ~0;

	/*
	 * Turn on I740 native modes (oh, and disable 256K framebuffer wraps).
	 */
	IDXREG_W (vregs, XR, XR_DISPCTL,
		  FIELDDEF (XR_DISPCTL, VGAWRAP, DISABLED) |
		   FIELDDEF (XR_DISPCTL, NATIVEMODE, ENABLE));

	/*  Enable native Attribute and CRTC register extensions  */
	IDXREG_W (vregs, XR, XR_IOCTL,
		  FIELDDEF (XR_IOCTL, AREXTENSIONS, ENABLE) |
		   FIELDDEF (XR_IOCTL, CREXTENSIONS, ENABLE));

	/*  Initialize address mapping  */
	IDXREG_W (vregs, XR, XR_ADDRMAPPING,
		  FIELDDEF (XR_ADDRMAPPING, BIOSMAP, NORMAL) |
		   FIELDDEF (XR_ADDRMAPPING, FBMAP, LOCAL) |
		   FIELDVAL (XR_ADDRMAPPING, PACKEDMODE, FALSE) |
		   FIELDVAL (XR_ADDRMAPPING, LINEARMODE, TRUE) |
		   FIELDVAL (XR_ADDRMAPPING, PAGEMODE, FALSE));

	/*  Initialize other display features  */
	IDXREG_W (vregs, XR, XR_PIXELCONFIG0,
		  FIELDDEF (XR_PIXELCONFIG0, DACWIDTH, 8BPP) |
		   FIELDVAL (XR_PIXELCONFIG0, SHOWCURSOR, FALSE) |
		   FIELDDEF (XR_PIXELCONFIG0, EXTENDEDREAD, DISABLE) |
		   FIELDDEF (XR_PIXELCONFIG0, OSCANCOLOR, DISABLE) |
		   FIELDDEF (XR_PIXELCONFIG0, PALETTEMODE, NORMAL));

	IDXREG_W (vregs, XR, XR_PIXELCONFIG1,
		  FIELDDEF (XR_PIXELCONFIG1, VGADELAYS, ENABLE) |
		   FIELDDEF (XR_PIXELCONFIG1, DISPMODE, 8BPP));

	IDXREG_W (vregs, XR, XR_PIXELCONFIG2,
		  FIELDDEF (XR_PIXELCONFIG2, FBGAMMA, DISABLE) |
		   FIELDDEF (XR_PIXELCONFIG2, OVLGAMMA, DISABLE));

	IDXREG_W (vregs, XR, XR_CURSORCTL,
		  FIELDVAL (XR_CURSORCTL, BLINKEN, FALSE) |
		   FIELDDEF (XR_CURSORCTL, ORIGIN, NORMAL) |
		   FIELDDEF (XR_CURSORCTL, VSIZE, FIXED) |
		   FIELDDEF (XR_CURSORCTL, MODE, 32x32x2_ANDXOR));

	IDXREG_W (vregs, XR, XR_MCLK, XR_MCLK_FREQ_100MHZ);

	/*  Determine available framebuffer memory  */
	ci->ci_MemSize = IDXREG_R (vregs, XR, XR_DRAMROWBOUND1) << 20;
	ci->ci_MasterClockKHz = MASTERCLK;
dprintf (("+++ ci_MemSize: %d\n", ci->ci_MemSize));

	strcpy (ci->ci_ADI.name, "Intel I740-based board");
	strcpy (ci->ci_ADI.chipset, "I740");
	ci->ci_ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ci_ADI.memory = ci->ci_MemSize;
	ci->ci_ADI.version = B_ACCELERANT_VERSION;

	memcpy (cc->cc_ATTR, attr_table_init, sizeof (cc->cc_ATTR));
//	ci->ci_CurrentROP = 1;	/*  A fairly unlikely ROP code  */

	return (calcmodes (di));
}


static status_t
calcmodes (struct devinfo *di)
{
	/*
	 * First count the modes so we know how much memory to allocate.
	 */
	if (di->di_NDispModes = testbuildmodes (di, NULL)) {
		/*
		 * Okay, build the list for real this time.
		 */
		if (di->di_DispModes = malloc ((di->di_NDispModes + 1) *
					       sizeof (display_mode)))
		{
			testbuildmodes (di, di->di_DispModes);
			return (B_OK);
		}
	}
	return (B_ERROR);
}

static int
testbuildmodes (struct devinfo *di, register display_mode *dst)
{
	const display_mode	*src;
	display_mode		low, high, try;
	uint32			i, j, pix_clk_range;
	int			count;

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	static color_space spaces[] =
		{ B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE/*, B_RGB32_LITTLE*/ };
#else
	static color_space spaces[] =
		{ B_CMAP8, B_RGB15_BIG, B_RGB16_BIG/*, B_RGB32_BIG*/ };
#endif
#define	NSPACES		(sizeof (spaces) / sizeof (color_space))

	/*
	 * Walk through our predefined list and see which modes fit this
	 * device.
	 */
	src = mode_list;
	count = 0;
	for (i = 0;  i < NMODES;  i++) {
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */ 
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;

		/*
		 * Test each color_space we support.
		 */
		for (j = 0;  j < NSPACES;  j++) {
			try = *src;
			try.space = low.space = high.space = spaces[j];
			if (propose_video_mode (di, &try, &low, &high) == B_OK)
			{
				/*  It's good; record it if there's storage  */
				if (dst)
					*dst++ = try;
				count++;
			}
		}
		/* advance to next mode */
		src++;
	}

	return (count);
}


status_t
propose_video_mode (
struct devinfo		*di,
display_mode		*target,
const display_mode	*low,
const display_mode	*high
)
{
	status_t	result = B_OK;
	double		target_refresh;
	uint32		row_bytes, row_pixels, pixel_bytes, limit_clock_lo;
	uint16		width, height, dispwide, disphigh;
	bool		want_same_width, want_same_height;

	target_refresh = ((double) target->timing.pixel_clock * 1000.0) /
			 ((double) target->timing.h_total *
			  (double) target->timing.v_total);
	want_same_width = target->timing.h_display == target->virtual_width;
	want_same_height = target->timing.v_display == target->virtual_height;
	width = height = dispwide = disphigh = 0;

	/* 
	 * Quantization to other hardware limits (performed later) may nudge
	 * previously validated values, so we need to be able to re-validate.
	 */
revalidate:
      {
	/*
	 * Validate horizontal timings.
	 * Horizontal must be multiples of 8.
	 * FIXME: RIVA 128 restrictions:
	 *	Visible width must be quantized to 32 pixels.
	 */
	uint16 h_display	= ((target->timing.h_display + 31) & ~31) >> 3;
	uint16 h_sync_start	= target->timing.h_sync_start >> 3;
	uint16 h_sync_end	= target->timing.h_sync_end >> 3;
	uint16 h_total		= target->timing.h_total >> 3;

	/*  Ensure reasonable minium display and sequential order of parms  */
	if (h_display < (320 >> 3))		h_display = 320 >> 3;
	if (h_display > (2048 >> 3))		h_display = 2048 >> 3;
	if (h_sync_start < (h_display + 2))	h_sync_start = h_display + 2;
	if (h_sync_end < (h_sync_start + 3))	h_sync_end = h_sync_start + 3;
						/*(0x001f >> 2);*/
	if (h_total < (h_sync_end + 1))		h_total = h_sync_end + 1;

	/*  Adjust for register limitations  */
	if (h_total - h_display > 0x007f) {
		h_total = h_display + 0x007f;
		if (h_sync_end > h_total - 1)
			h_sync_end = h_total - 1;
		if (h_sync_start > h_sync_end)
			h_sync_start = h_sync_end - 0x001f;
	}
	if (h_sync_end - h_sync_start > 0x001f)
		h_sync_end = h_sync_start + 0x001f;

	target->timing.h_display	= h_display << 3;
	target->timing.h_sync_start	= h_sync_start << 3;
	target->timing.h_sync_end	= h_sync_end << 3;
	target->timing.h_total		= h_total << 3;
      }
	if (target->timing.h_display < low->timing.h_display  ||
	    target->timing.h_display > high->timing.h_display  ||
	    target->timing.h_sync_start < low->timing.h_sync_start  ||
	    target->timing.h_sync_start > high->timing.h_sync_start  ||
	    target->timing.h_sync_end < low->timing.h_sync_end  ||
	    target->timing.h_sync_end > high->timing.h_sync_end  ||
	    target->timing.h_total < low->timing.h_total  ||
	    target->timing.h_total > high->timing.h_total)
		result = B_BAD_VALUE;

      {
	/*  Validate vertical timings  */
	uint16 v_display	= target->timing.v_display;
	uint16 v_sync_start	= target->timing.v_sync_start;
	uint16 v_sync_end	= target->timing.v_sync_end;
	uint16 v_total		= target->timing.v_total;

	/*  Ensure reasonable minium display and sequential order of parms  */
	if (v_display < 200)			v_display = 200;
	if (v_display > 2048)			v_display = 2048; /* ha! */
	if (v_sync_start < (v_display + 1))	v_sync_start = v_display + 1;
	if (v_sync_end < v_sync_start)		v_sync_end = v_sync_start +
							      (0x000f >> 2);
	if (v_total < (v_sync_end + 1))		v_total = v_sync_end + 1;

	/*  Adjust for register limitations  */
	if (v_total - v_display > 0x00ff) {
		v_total = v_display + 0x00ff;
		if (v_sync_end > v_total - 1)
			v_sync_end = v_total - 1;
		if (v_sync_start > v_sync_end)
			v_sync_start = v_sync_end - 0x000f;
	}
	if (v_sync_end - v_sync_start > 0x000f)
		v_sync_end = v_sync_start + 0x000f;

	target->timing.v_display	= v_display;
	target->timing.v_sync_start	= v_sync_start;
	target->timing.v_sync_end	= v_sync_end;
	target->timing.v_total		= v_total;
      }
	if (target->timing.v_display < low->timing.v_display  ||
	    target->timing.v_display > high->timing.v_display  ||
	    target->timing.v_sync_start < low->timing.v_sync_start  ||
	    target->timing.v_sync_start > high->timing.h_sync_start  ||
	    target->timing.v_sync_end < low->timing.v_sync_end  ||
	    target->timing.v_sync_end > high->timing.v_sync_end  ||
	    target->timing.v_total < low->timing.v_total  ||
	    target->timing.v_total > high->timing.v_total)
		result = B_BAD_VALUE;

	/*
	 * Validate display vs. virtual.
	 * Save current settings so we can test later if they changed.
	 */
	width = target->virtual_width;
	height = target->virtual_height;
	dispwide = target->timing.h_display;
	disphigh = target->timing.v_display;

	if (target->timing.h_display > target->virtual_width  ||
	    want_same_width)
		target->virtual_width = target->timing.h_display;
	if (target->timing.v_display > target->virtual_height  ||
	    want_same_height)
		target->virtual_height = target->timing.v_display;
	if (target->virtual_width > 2048)
		target->virtual_width = 2048;

	/*
	 * FIXME:
	 * RIVA restriction: Quantize virtual_width to 8 bytes (should fall
	 * right out of the 32-pixel quantization earlier, but do it anyway).
	 */
	pixel_bytes = (colorspacebits (target->space) + 7) >> 3;
	row_bytes = (pixel_bytes * target->virtual_width + 7) & ~7;
	target->virtual_width = row_bytes / pixel_bytes;
	if (want_same_width)
		target->timing.h_display = target->virtual_width;

	/*  Adjust virtual width for engine limitations  */
	if (target->virtual_width < low->virtual_width  ||
	    target->virtual_width > high->virtual_width)
		result = B_BAD_VALUE;

	/*  Memory requirement for frame buffer  */
	if (row_bytes * target->virtual_height >
	    di->di_I740CardInfo->ci_MemSize)
		target->virtual_height =
		 di->di_I740CardInfo->ci_MemSize / row_bytes;

	if (target->virtual_height < target->timing.v_display)
		/* not enough frame buffer memory for the mode */
		result = B_ERROR;
	else if (target->virtual_height < low->virtual_height  ||
		 target->virtual_height > high->virtual_height)
		result = B_BAD_VALUE;

	if (width != target->virtual_width  ||
	    height != target->virtual_height  ||
	    dispwide != target->timing.h_display  ||
	    disphigh != target->timing.v_display)
		/*  Something changed; we have to re-check.  */
		goto revalidate;	/*  Look up  */

	/*
	 * Adjust pixel clock for DAC limits and target refresh rate
	 * pixel_clock is recalculated because [hv]_total may have been nudged.
	 */
	target->timing.pixel_clock = target_refresh *
				     ((double) target->timing.h_total) *
				     ((double) target->timing.v_total) /
				     1000.0 + 0.5;
	limit_clock_lo = 48.0 *	/*  Until a monitors database does it...  */
			 ((double) target->timing.h_total) *
			 ((double) target->timing.v_total) /
			 1000.0;
	if (limit_clock_lo > CLOCK_MAX)
		result = B_ERROR;
	if (target->timing.pixel_clock > CLOCK_MAX)
		target->timing.pixel_clock = CLOCK_MAX;
	if (target->timing.pixel_clock < limit_clock_lo)
		target->timing.pixel_clock = limit_clock_lo;
	if (target->timing.pixel_clock < low->timing.pixel_clock  ||
	    target->timing.pixel_clock > high->timing.pixel_clock)
		result = B_BAD_VALUE;
	
	/*  I740 doesn't handle 32-bpp at all well.  */
	if (colorspacebits (target->space) == 32)
		result = B_ERROR;

	/*
	 * Bandwidth check.
	 * This is amazingly primitive, based on some sketchy numbers I saw
	 * on Intel's Web site.  (The mutiplication by 8 is because
	 * colorspacebits() reports number of bits, not bytes.)
	if (target->timing.pixel_clock * colorspacebits (target->space) >
	    CLOCK_MAX * 8.0)
		result = B_ERROR;
	 */

	/*  flags?  */
	return (result);
}


/*****************************************************************************
 * Display manipulation.
 ****
 * NOTE: It is extremely important that the accelerant re-initialize
 * its acceleration state after reconfiguring the display mode.
 */
status_t
SetupCRTC (
register struct devinfo		*di,
register struct i740_card_info	*ci,
register display_mode		*dm
)
{
	register vgaregs	*vregs;
	i740_card_ctl		*cc;
	float			clockin, clockout;
	status_t		retval;
	int			m, n, p;
	int			hdisp, hsyncstart, hsyncend, htotal;
	int			vdisp, vsyncstart, vsyncend, vtotal;
	int			htotalpix, vtotalpix;
	int			bytespp, bytesperrow;
	uint8			bltctl, pix1;
	uint8			lowater, burstlen;

	cc = di->di_I740CardCtl;
	vregs = &ci->ci_Regs->VGARegs;

	htotalpix	= dm->timing.h_total;
	hdisp		= dm->timing.h_display >> 3;
	hsyncstart	= dm->timing.h_sync_start >> 3;
	hsyncend	= dm->timing.h_sync_end >> 3;
	htotal		= htotalpix >> 3;

	vtotalpix	= dm->timing.v_total;
	vdisp		= dm->timing.v_display;
	vsyncstart	= dm->timing.v_sync_start;
	vsyncend	= dm->timing.v_sync_end;
	vtotal		= vtotalpix;

	bytespp		= (colorspacebits (dm->space) + 7) >> 3;
	bytesperrow	= (dm->virtual_width >> 3) * bytespp;
	
#ifdef SCANDOUBLING_WORKS
/*  1998.08.18: Made impossible by current R4 API  */
	if (vt->vt_ScanDouble) {
		vdisp <<= 1;
		vsyncstart <<= 1;
		vsyncend <<= 1;
		vtotal <<= 1;
		vtotalpix <<= 1;
	}
#endif

	clockin = dm->timing.pixel_clock;
	if (!ClockSelect (ci->ci_MasterClockKHz, clockin, &clockout,
			  &m, &n, &p))
	{
		dprintf (("I740: Can't compute (M,N,P) triplet: %fKHz\n",
			  clockin));
		return (B_ERROR);
	}

	ci->ci_BytesPerRow = dm->virtual_width * bytespp;
	ci->ci_Depth = colorspacebits (dm->space);

	/*
	 * CRTC Controller
	 * Not fully symbolized, but i740defs.h should provide some clues.
	 * Hardware-dependent fenceposts are added (or subtracted) here.
	 */     
	cc->cc_CRTC[CR_HTOTAL_7_0]	= Set8Bits (htotal - 5);
	cc->cc_CRTC[CR_HDISPEND_7_0]	= Set8Bits (hdisp - 1);
	cc->cc_CRTC[CR_HBLANKSTART_7_0]	= Set8Bits (hdisp - 1);	// No border
	cc->cc_CRTC[CR_HBLANKEND]	= SetBitField (htotal - 1, 4:0, 4:0) |
					  SetBit (7);
	cc->cc_CRTC[CR_HSYNCSTART_7_0]	= Set8Bits (hsyncstart);
	cc->cc_CRTC[CR_HSYNCEND]	= SetBitField (htotal - 1, 5:5, 7:7) |
					  SetBitField (hsyncend, 4:0, 4:0);
	cc->cc_CRTC[CR_VTOTAL_7_0]	= SetBitField (vtotal - 2, 7:0, 7:0);

	cc->cc_CRTC[CR_OVERFLOW]	= SetBitField (vtotal - 2, 8:8, 0:0) |
					  SetBitField (vdisp - 1, 8:8, 1:1) |
					  SetBitField (vsyncstart, 8:8, 2:2) |
	/*  No border  */		  SetBitField (vdisp, 8:8, 3:3) |
					  SetBit (4) |
					  SetBitField (vtotal - 2, 9:9, 5:5) |
					  SetBitField (vdisp - 1, 9:9, 6:6) |
					  SetBitField (vsyncstart, 9:9, 7:7);

	cc->cc_CRTC[CR_FONTHEIGHT]	= SetBitField (vdisp, 9:9, 5:5) |
					  SetBit (6);
	cc->cc_CRTC[CR_VSYNCSTART_7_0]	= Set8Bits (vsyncstart);
	cc->cc_CRTC[CR_VSYNCEND]	= SetBitField (vsyncend, 3:0, 3:0) |
					  SetBit (5);
	cc->cc_CRTC[CR_VDISPEND_7_0]	= Set8Bits (vdisp - 1);
	cc->cc_CRTC[CR_PITCH_7_0]	= Set8Bits (bytesperrow);
	cc->cc_CRTC[CR_VBLANKSTART_7_0]	= Set8Bits (vdisp - 1);	// No border
	cc->cc_CRTC[CR_VBLANKEND_7_0]	= Set8Bits (vtotal - 1);// No border

	cc->cc_CRTC[CR_VTOTAL_11_8]	= SetBitField (vtotal - 2, 11:8, 3:0);
	cc->cc_CRTC[CR_VDISPEND_11_8]	= SetBitField (vdisp - 1, 11:8, 3:0);
	cc->cc_CRTC[CR_VSYNCSTART_11_8]	= SetBitField (vsyncstart, 11:8, 3:0);
	cc->cc_CRTC[CR_VBLANKSTART_11_8]= SetBitField (vdisp - 1, 11:8, 3:0);
	cc->cc_CRTC[CR_HTOTAL_8]	= SetBitField (htotal - 5, 8:8, 0:0);
	cc->cc_CRTC[CR_HBLANKEND_6]	= SetBitField (htotal - 1, 6:6, 0:0);

	cc->cc_CRTC[CR_PITCH_11_8]	= SetBitField (bytesperrow, 11:8, 3:0);

	/*  Default values for registers not otherwise initialized.  */
	cc->cc_CRTC[CR_FONTROLL]	=
	cc->cc_CRTC[CR_TXCURSTOP]	=
	cc->cc_CRTC[CR_TXCURSBOT]	=
	cc->cc_CRTC[CR_TXCURSLOC_15_8]	=
	cc->cc_CRTC[CR_TXCURSLOC_7_0]	=
	cc->cc_CRTC[CR_ULLOC]		= 0x00;
	cc->cc_CRTC[CR_MODE]		= MASKEXPAND (CR_MODE_NOTRESET) |
					  MASKEXPAND (CR_MODE_FETCH8) |
					  MASKEXPAND (CR_MODE_SELECTROWSCAN) |
					  MASKEXPAND (CR_MODE_COMPAT);
	cc->cc_CRTC[CR_LINECMP_7_0]	= 0xFF;
#ifdef SCANDOUBLING_WORKS
	if (vt->vt_ScanDouble)
		cc->cc_CRTC[CR_FONTHEIGHT] |=
		 MASKEXPAND (CR_FONTHEIGHT_SCANDOUBLE);
#endif

	/*  Set framebuffer base and horizontal panning values.  */
	setdispoffset (ci, cc, dm);


	cc->cc_RegMisc = FIELDDEF (MISC, ODDEVENPAGESEL, 64K) |
			 FIELDDEF (MISC, CLKSEL, EXTRA) |
			 FIELDDEF (MISC, 0xA0000WINDOW, ENABLE) |
			 FIELDDEF (MISC, IOADDR, 3Dx);
	if (!(dm->timing.flags & B_POSITIVE_HSYNC))
		/*  Sense of MISC bits is inverted  */
		cc->cc_RegMisc |= FIELDDEF (MISC, HSYNCPOL, NEGATIVE);
	if (!(dm->timing.flags & B_POSITIVE_VSYNC))
		cc->cc_RegMisc |= FIELDDEF (MISC, VSYNCPOL, NEGATIVE);


	screen_off (vregs);
	snooze (1000);

	/*  Write PLL clock coefficients  */
	IDXREG_W (vregs, XR, XR_VCLK2_M, Set8Bits (m));
	IDXREG_W (vregs, XR, XR_VCLK2_N, Set8Bits (n));
	IDXREG_W (vregs, XR, XR_VCLK2_MNHI,
		  SetBitField (n, 9:8, XR_VCLKx_MNHI_N_9_8) |
		   SetBitField (m, 9:8, XR_VCLKx_MNHI_M_9_8));
	IDXREG_W (vregs, XR, XR_VCLK2_PDIV,
		  FIELDVAL (XR_VCLKx_PDIV, 2EXPP, p) |
		   FIELDDEF (XR_VCLKx_PDIV, REFDIVSEL, DIV1));

	/*  Extended configuration registers  */
	pix1 = FIELDDEF (XR_PIXELCONFIG1, VGADELAYS, ENABLE);
	switch (ci->ci_Depth) {
	case 8:
		bltctl = FIELDDEF (XR_BLTCTL, EXPANDMODE, 8BPP);
		pix1 |= FIELDDEF (XR_PIXELCONFIG1, DISPMODE, 8BPP);
		break;
	case 15:
		bltctl = FIELDDEF (XR_BLTCTL, EXPANDMODE, 16BPP);
		pix1 |= FIELDDEF (XR_PIXELCONFIG1, DISPMODE, 16BPP_555);
		break;
	case 16:
		bltctl = FIELDDEF (XR_BLTCTL, EXPANDMODE, 16BPP);
		pix1 |= FIELDDEF (XR_PIXELCONFIG1, DISPMODE, 16BPP_565);
		break;
	case 32:
		bltctl = FIELDDEF (XR_BLTCTL, EXPANDMODE, 24BPP);
		pix1 |= FIELDDEF (XR_PIXELCONFIG1, DISPMODE, 32BPP);
		break;
	}
	IDXREG_W (vregs, XR, XR_BLTCTL, bltctl);
	IDXREG_W (vregs, XR, XR_PIXELCONFIG1, pix1);

	/*  Misc output register  */
	vregs->w.VGA_Misc = cc->cc_RegMisc;

	/*  Sequencer registers  */
	write_table (&vregs->w.VGA_SR_Idx, seq_table,
		     SR_CLOCK, SR_MODE);

	/*  Graphics controller registers  */
	write_table (&vregs->w.VGA_GR_Idx, gcr_table,
		     GR_SETRESET, GR_LATCHMASK);

	/*  CRTC registers are loaded in several disparate "blocks."  */
	write_table (&vregs->w.VGA_CR_Idx, cc->cc_CRTC,
		     CR_HTOTAL_7_0, CR_LINECMP_7_0);
	write_table (&vregs->w.VGA_CR_Idx, cc->cc_CRTC,
		     CR_VTOTAL_11_8, CR_VBLANKSTART_11_8);
	IDXREG_W (vregs, VGA_CR, CR_HTOTAL_8, cc->cc_CRTC[CR_HTOTAL_8]);
	IDXREG_W (vregs, VGA_CR, CR_HBLANKEND_6, cc->cc_CRTC[CR_HBLANKEND_6]);
	IDXREG_W (vregs, VGA_CR, CR_PITCH_11_8, cc->cc_CRTC[CR_PITCH_11_8]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_31_24, cc->cc_CRTC[CR_FBBASE_31_24]);
	/*  This write latches the full FBBASE value to the hardware.  */
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_23_18, cc->cc_CRTC[CR_FBBASE_23_18]);

	/*  Attribute registers  */
	n = vregs->r.VGA_ST01;
	for (n = 0;  n < NREGS_ATTR;  n++)	/*  Resuing 'n'  */
	{
		vregs->w.VGA_AR_Idx = n;
		vregs->w.VGA_AR_ValW = cc->cc_ATTR[n];
	}
	vregs->w.VGA_AR_Idx = FIELDDEF (AR_IDX, ACCESS, LOCK);

	calcfifo (&lowater, &burstlen, 100000.0, clockout, ci->ci_Depth);
#if 1
	/*
	 * FIFO watermark register
	 *
	 * These values were determined empirically by fiddling with them in
	 * the debugger.  The total FIFO depth is 48 64-bit words.
	 * FIXME: They still don't work for 32 bpp modes.
	 ***
	 * I was using 16 for both values.  Now I'm using the maximum values
	 * pulled out of the I740 BIOS.  I've also chopped AGP FIFO sizes to
	 * zero, since we're not fetching the framebuffer out of AGP memory.
	 */
	ci->ci_Regs->FWATER_BLC = FIELDVAL (FWATERBLC, LMI_BURSTLEN, 0x1A) |
				  FIELDVAL (FWATERBLC, LMI_LOWATER, 0x16) |
				  FIELDVAL (FWATERBLC, AGP_BURSTLEN, 0) |
				  FIELDVAL (FWATERBLC, AGP_LOWATER, 0);
#else
	/*  Dratted thing doesn't work yet.  */
	ci->ci_Regs->FWATER_BLC = FIELDVAL (FWATERBLC, LMI_BURSTLEN, burstlen) |
				  FIELDVAL (FWATERBLC, LMI_LOWATER, lowater) |
				  FIELDVAL (FWATERBLC, AGP_BURSTLEN, 8) |
				  FIELDVAL (FWATERBLC, AGP_LOWATER, 8);
#endif


	snooze (1000);
	screen_on (vregs);

	return (B_OK);
}

static void
setdispoffset (
register struct i740_card_info	*ci,
register struct i740_card_ctl	*cc,
register display_mode		*dm
)
{
	int	bytespp;
	uint32	addr;

	bytespp = (ci->ci_Depth + 7) >> 3;
	addr = (uint32) ci->ci_FBBase - (uint32) ci->ci_BaseAddr0 +
	       (dm->v_display_start * dm->virtual_width +
	        dm->h_display_start) * bytespp;
	cc->cc_ATTR[AR_HPAN] = (addr & 3) << 1;	// Pixel panning

//	addr >>= 2;	// 32-bit quantization
	cc->cc_CRTC[CR_FBBASE_9_2]	= SetBitField (addr, 9:2, 7:0);
	cc->cc_CRTC[CR_FBBASE_17_10]	= SetBitField (addr, 17:10, 7:0);
	cc->cc_CRTC[CR_FBBASE_23_18]	= SetBitField (addr, 23:18, 5:0) |
					  MASKEXPAND (CR_FBBASE_23_18_LOADADDR);
	cc->cc_CRTC[CR_FBBASE_31_24]	= SetBitField (addr, 31:24, 7:0);
}


static void
calcfifo (
uint8	*lowater_retn,
uint8	*burstlen_retn,
float	busclk,
float	pixclk,
int32	pixdepth
)
{
	/*
	 * FIFO lines are filled one per bus clock.  FIFO lines are 8 bytes
	 * each.  Maximum FIFO size is 48 lines.
	 *
	 * FIXME:
	 * Calculation of the maximum latency is a complete hack out of thin
	 * air.  In my travels, I've seen collections of figures purporting
	 * to be related to DRAM timings.  These numbers look like this:
	 *
	 * 5-3-1-1-1
	 *
	 * I presume this to mean 5 clocks setup time, 3 clocks to get first
	 * datum, one clock for each successive datum.  So, with absolutely
	 * no confirmation of this suspicion at all, that's what I'm going
	 * to use as maximum latency (11 clocks).
	 */
#define	SIZEOF_FIFOLINE	8
#define	MAX_LATENCY	11
	float	drainrate, fillrate;
	int32	lowater, burstlen;

	pixdepth += 7;	/*  Convert to bytes  */
	pixdepth >>= 3;
	drainrate = pixclk * pixdepth / busclk;
	fillrate = SIZEOF_FIFOLINE - drainrate;

	lowater = ((int32) (drainrate * MAX_LATENCY) + SIZEOF_FIFOLINE - 1) &
		  ~(SIZEOF_FIFOLINE - 1);
	burstlen = ceil (lowater / fillrate);

	*lowater_retn = lowater / SIZEOF_FIFOLINE;
	*burstlen_retn = burstlen;
dprintf (("+++ drainrate: %d, fillrate: %d\n",
 (int32) (drainrate * 1000), (int32) (fillrate * 1000)));
dprintf (("+++ lowater, burstlen: %d, %d\n", *lowater_retn, *burstlen_retn));
}


void
setpaletteentry (
register union vgaregs	*vregs,
int32			idx,
uint8			r,
uint8			g,
uint8			b
)
{
	vregs->w.VGA_DACWIdx = idx;

	vregs->w.VGA_DACVal = r;
	vregs->w.VGA_DACVal = g;
	vregs->w.VGA_DACVal = b;
}

static void
setramppalette (register union vgaregs *vregs)
{
	register int	i;

	vregs->w.VGA_DACWIdx = 0;	/*  Start at zero  */
	for (i = 0;  i < 256;  i++) {
		vregs->w.VGA_DACVal = i;
		vregs->w.VGA_DACVal = i;
		vregs->w.VGA_DACVal = i;
	}
}

static void
write_table (vuint8 *addr, const uint8 *table, int first, int last)
{
	int i;

	for (i = first;  i <= last;  i++)
		(*(vuint16 *) addr = ((uint16) table[i] << 8) | i);
}


uint32
colorspacebits (uint32 cs /* a color_space */)
{
	switch (cs) {
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		return (32);
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
		return (24);
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		return (16);
	case B_RGB15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_BIG:
	case B_RGBA15_LITTLE:
		return (15);
	case B_CMAP8:
		return (8);
	}
	return (0);
}


void
screen_off (register union vgaregs *vregs)
{
	/*  Place chip in reset  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, FIELDDEF (SR_RESET, ASYNC, ENABLE) |
			    FIELDDEF (SR_RESET, SYNC, DISABLE));

	/*  Turn off display  */
	vregs->w.VGA_SR_Idx = SR_CLOCK;
	IDXREG_W (vregs, VGA_SR,
		  SR_CLOCK,
		  vregs->r.VGA_SR_Val | MASKEXPAND (SR_CLOCK_VIDDISABLE));

	/*  Release chip  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, FIELDDEF (SR_RESET, ASYNC, DISABLE) |
			    FIELDDEF (SR_RESET, SYNC, DISABLE));
}

void
screen_on (register union vgaregs *vregs)
{
	/*  Place chip in reset  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, FIELDDEF (SR_RESET, ASYNC, ENABLE) |
			    FIELDDEF (SR_RESET, SYNC, DISABLE));

	/*  Turn on display  */
	vregs->w.VGA_SR_Idx = SR_CLOCK;
	IDXREG_W (vregs, VGA_SR,
		  SR_CLOCK,
		  vregs->r.VGA_SR_Val & ~MASKEXPAND (SR_CLOCK_VIDDISABLE));

	/*  Release chip  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, FIELDDEF (SR_RESET, ASYNC, DISABLE) |
			    FIELDDEF (SR_RESET, SYNC, DISABLE));
}



/*
 * The following figures are taken from the Intel I740 PRM and "Changing the
 * Display Resolutions and Refresh Rates" documents.  All frequencies are in
 * KHz.
 */
/*  Actual HW range is 0 - 1024, but this is sufficient.  */
#define M_MIN		0
#define M_MAX		128

#define	N_MIN		0
#define	N_MAX		128

#define P_MIN		0
#define P_MAX		5

#define	VCO_MAX		550000.0


static int
ClockSelect (
float	masterClock,
float	clockReq,
float	*clockOut,
int	*mOut,
int	*nOut,
int	*pOut
)
{ 
	int	m, n, p;
	float	bestDiff = 1e12;
	float	target = 0.0;
	float	best = 0.0;
	float	diff;
	int	pMax;

	*clockOut = 0.0;
	if (clockReq < CLOCK_MIN  ||  clockReq > CLOCK_MAX)
		dprintf (("Requested clock rate %f KHz out of range; trying anyway...",
			  clockReq));

	/* 
	 * According to the Intel docs, the formula for calculating clock
	 * values is:
	 *
	 * F<out> = F<in> * ((M + 2) * 1/(2**P)) / (N + 2)
	 *
	 * However, if you back-calculate from the MNP values in their example
	 * spreadsheet, you get:
	 *
	 * F<out> = F<in> * ((M + 2) * 1/(2**P)) / (N + 2) * 4
	 *
	 * I'm guessing the '4' is really register XRCB, bit 2 (VCO Divide),
	 * which means it could be 16 as well as 4.
	 *
	 * There are intermediate restrictions:
	 *
	 * F<out> * 2**P <= VCO_MAX
	 */
	/*  Calculate largest P  */
	diff = VCO_MAX / clockReq;
	for (pMax = P_MAX;  pMax > P_MIN;  pMax--)
		if (diff > (1 << pMax))
			break;

	if (pMax < 0)
		/*  Impossible, but the check is cheap...  */
		return (FALSE);

	for (p = P_MIN;  p <= pMax;  p++) {
		for (n = N_MIN;  n <= N_MAX;  n++) {
			float	fn = n;
			float	f2expp = (1 << p);

			m = (int) (clockReq * f2expp * (fn + 2) /
				   (masterClock * 4)) - 2;
			if (m < M_MIN  ||  m > M_MAX)
				continue;

			target = masterClock * 4 * ((float) m + 2) /
				 (f2expp * (fn + 2));
			if (target > CLOCK_MAX  ||  target < CLOCK_MIN)
				continue;

			diff = fabs (target - clockReq);
			if (diff < bestDiff) {
				bestDiff = diff;
				best = target;
				*mOut = m;  *nOut = n;  *pOut = p;
				*clockOut = best;
			}
		}
	}
	return (best!=0.0);    
}
