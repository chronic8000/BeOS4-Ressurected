/* :ts=8 bk=0
 *
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.06
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/lynxemdefs.h>
#include <graphics_p/lynxem/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define MASTERCLK	14318.18	/*  4*NTSC colorburst  */


//#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR \
//			 | B_PARALLEL_ACCESS | B_DPMS \
//			 | B_SUPPORTS_OVERLAYS)
#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR \
			 | B_PARALLEL_ACCESS)
#define	SYNC_HnVn	0
#define	SYNC_HnVP	B_POSITIVE_VSYNC
#define	SYNC_HPVn	B_POSITIVE_HSYNC
#define	SYNC_HPVP	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)


/*****************************************************************************
 * Structure definitions
 */


/*****************************************************************************
 * Local prototypes.
 */
static void setramppalette(register union vgaregs *vregs);
static void write_table(vuint8 *addr, const uint8 *table, int first, int last);
static void write_synctable(vuint8 *addr, const uint8 *table);
static status_t initGTT(struct devinfo *di, register struct gfx_card_info *ci);
static status_t calcmodes(struct devinfo *di);
static int testbuildmodes(struct devinfo *di, register display_mode *dst);


/*****************************************************************************
 * Globals.
 */
extern pci_module_info	*gpci_bus;
extern genpool_module	*ggpm;
extern BPoolInfo	*gfbpi;


/*
 * This table is formatted for 132 columns, so stretch that window...
 */
static const display_mode	mode_list[] = {
/* VESA: 640x480 @ 60Hz */
 {	{  25175,  640,  656,  752,  800,  480,  490,  492,  525, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* VESA: 640x480 @ 72Hz */
 {	{  31500,  640,  664,  704,  832,  480,  489,  492,  520, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* VESA: 640x480 @ 75Hz */
 {	{  31500,  640,  656,  720,  840,  480,  481,  484,  500, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* VESA: 640X480 @ 85Hz */
 {	{  36000,  640,  696,  752,  832,  480,  481,  484,  509, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* VESA: 800x600 @ 56Hz */
 {	{  36000,  800,  824,  896, 1024,  600,  601,  603,  625, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 800x600 @ 60Hz */
 {	{  40000,  800,  840,  968, 1056,  600,  601,  605,  628, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 800x600 @ 72Hz */
 {	{  50000,  800,  856,  976, 1040,  600,  637,  643,  666, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 800x600 @ 75Hz */
 {	{  49500,  800,  816,  896, 1056,  600,  601,  604,  625, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 800x600 @ 85Hz */
 {	{  56250,  800,  832,  896, 1048,  600,  601,  604,  631, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 43Hz interlaced */
 {	{  46600, 1024, 1032, 1208, 1264,  384,  385,  388,  408, B_TIMING_INTERLACED},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 60Hz */
 {	{  65000, 1024, 1048, 1184, 1344,  768,  771,  777,  806, SYNC_HnVn},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 70Hz */
 {	{  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806, SYNC_HnVn},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 75Hz */
 {	{  78750, 1024, 1040, 1136, 1312,  768,  769,  772,  800, SYNC_HPVP},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 85Hz */
 {	{  94500, 1024, 1072, 1168, 1376,  768,  769,  772,  808, SYNC_HPVP},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
 {	{  94200, 1152, 1184, 1280, 1472,  864,  865,  868,  914, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* VESA: 1152x864 @ 75Hz */
 {	{ 108000, 1152, 1216, 1344, 1600,  864,  865,  868,  900, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
 {	{ 121500, 1152, 1216, 1344, 1568,  864,  865,  868,  911, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* VESA: 1280x1024 @ 60Hz */
 {	{ 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* VESA: 1280x1024 @ 75Hz */
 {	{ 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* VESA: 1280x1024 @ 85Hz */
 {	{ 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* VESA: 1600x1200 @ 60Hz */
 {	{ 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* VESA: 1600x1200 @ 65Hz */
 {	{ 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* VESA: 1600x1200 @ 70Hz */
 {	{ 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* VESA: 1600x1200 @ 75Hz */
 {	{ 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
 {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* VESA: 1600x1200 @ 85Hz */
 {	{ 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* VESA: 1792x1344 @ 60Hz */
 {	{ 204750, 1792, 1920, 2120, 2448, 1344, 1345, 1348, 1394, SYNC_HnVP},		B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS},
/* VESA: 1792x1344 @ 75Hz */
 {	{ 261000, 1792, 1888, 2104, 2456, 1344, 1345, 1348, 1417, SYNC_HnVP},		B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS},
/* VESA: 1856x1392 @ 60Hz */
 {	{ 218250, 1856, 1952, 2176, 2528, 1392, 1393, 1396, 1439, SYNC_HnVP},		B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS},
/* VESA: 1856x1392 @ 75Hz */
 {	{ 288000, 1856, 1984, 2208, 2560, 1392, 1393, 1396, 1500, SYNC_HnVP},		B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS},
/* VESA: 1920x1440 @ 60Hz */
 {	{ 234000, 1920, 2048, 2256, 2600, 1440, 1441, 1444, 1500, SYNC_HnVP},		B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS},
/* VESA: 1920x1440 @ 75Hz */
 {	{ 297000, 1920, 2064, 2288, 2640, 1440, 1441, 1444, 1500, SYNC_HnVP},		B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS},
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))


/*
 * Initial state of chip registers.  In the case of the GCR and sequencer
 * registers, they never change.  The attribute and CRT controller registers
 * get clobbered every chance we can :-)...
 */
/* Graphics Display Controller
 */
static uint8 gcr_table[] =
{
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
//	DEF2FIELD (GR_MODE, SHIFTCTL, 256COLOR) |	/*  0x40  */
//	 DEF2FIELD (GR_MODE, ODDEVEN, SEQUENTIAL) |
//	 DEF2FIELD (GR_MODE, READMODE, NORMAL) |
//	 VAL2FIELD (GR_MODE, WRITEMODE, 0),
	DEF2FIELD (GR_MODE, SHIFTCTL, 16COLOR) |	/*  0x00  */
	 DEF2FIELD (GR_MODE, ODDEVEN, SEQUENTIAL) |
	 DEF2FIELD (GR_MODE, READMODE, NORMAL) |
	 VAL2FIELD (GR_MODE, WRITEMODE, 0),
	DEF2FIELD (GR_MISC, CHAINODDEVEN, NORMAL) |	/*  0x01  */
	 DEF2FIELD (GR_MISC, DISPMODE, GRAPHICS),
	0x0F,
	0xFF
};
#define	NREGS_GCR	(sizeof (gcr_table) / sizeof (gcr_table[0]))

/* Time Sequencer
 */
static const uint8 seq_table[] =
{
	0x00,
	DEF2FIELD (SR_CLOCK, TXCELLWIDTH, 8),		/*  0x01  */
	0x0F,
	0x00, 	/* Font select */
	VAL2FIELD (SR_MODE, CHAIN4, TRUE) |		/*  0x0E  */
	 DEF2FIELD (SR_MODE, ODDEVEN, SEQUENTIAL) |
	 DEF2FIELD (SR_MODE, EXTMEM, ENABLE)
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
	DEF2FIELD (AR_MODE, PAL54SRC, PALETTE) |	/*  0x41  */
	 DEF2FIELD (AR_MODE, PIXWIDTH, 8) |
	 DEF2FIELD (AR_MODE, PANSEL, BOTH) |
	 DEF2FIELD (AR_MODE, BLINK, DISABLE) |
	 DEF2FIELD (AR_MODE, LINEGR, DISABLE) |
	 DEF2FIELD (AR_MODE, DISPTYPE, COLOR) |
	 DEF2FIELD (AR_MODE, DISPMODE, GRAPHICS),
	0xFF,
	0x0F,
	0x00,
	0x00
};


/****************************************************************************
 * Reconfigure the display.
 */
status_t
vid_selectmode (
register struct devinfo		*di,
register struct gfx_card_info	*ci,
register display_mode		*dm
)
{
	status_t	retval;
	moderegs	newregs;

	/*
	 * We are about to Change The World, so grab everything.
	 */
	acquire_sem (ci->ci_ModeRegsLock);
	acquire_sem (ci->ci_CLUTLock);
	enableinterrupts (ci, FALSE);

	if ((retval = calcmoderegs (&newregs, dm, di)) < 0)
		return (retval);
	if ((retval = setmode (&newregs, di, ci)) == B_OK)
		memcpy (&di->di_GfxCardCtl->cc_ModeRegs,
			&newregs,
			sizeof (moderegs));

	clearinterrupts (ci);
	enableinterrupts (ci, TRUE);

	/*
	 * The driver pays no attention to the accelerant's locks, since
	 * they can be trashed.  Anyone calling this function better be
	 * prepared to keep the rendering engine locked down.
	 */
	release_sem (ci->ci_CLUTLock);
	release_sem (ci->ci_ModeRegsLock);

	return (retval);
}


/*****************************************************************************
 * PLL coefficient calculator.
 ****
 * The following figures are pulled out of my posterior, based on the bit
 * widths of the relevant registers.
 */
#define D_MIN		1
#define D_MAX		63

#define	N_MIN		1
#define	N_MAX		255

#define P_MIN		0
#define P_MAX		3


static int
ClockSelect (
float	masterClock,
float	clockReq,
float	*clockOut,
int	*dOut,
int	*nOut,
int	*pOut
)
{ 
	int	d, n, p;
	float	bestDiff = 9e99;
	float	target = 0.0;
	float	best = 0.0;
	float	diff;
	int	pMin, pMax;

	*clockOut = 0.0;
	if (clockReq < CLOCK_MIN  ||  clockReq > CLOCK_MAX)
		dprintf (("Requested clock rate %f KHz out of range; trying anyway...",
			  clockReq));

	/*
	 * The LynxEM PLL formula is unusually simple:
	 *
	 * F<out> = F<in> * NUM / DENOM
	 *
	 * If you're using the VCLK (as opposed to VCLK2), then you get an
	 * extra parameter:
	 *
	 * F<out> = F<in> * NUM / (DENOM * (1 + PDIV))
	 *
	 * But I won't be using this.
	 *
	 * There are intermediate restrictions (or are there?):
	 */
	for (d = D_MIN;  d <= D_MAX;  d++) {
		float	fd = d;

		n = (int) (clockReq * fd / masterClock);
		if (n < N_MIN  ||  n > N_MAX)
			continue;

		target = masterClock * (float) n / fd;

		diff = fabs (target - clockReq);
		if (diff < bestDiff) {
			bestDiff = diff;
			best = target;
			*dOut = d;  *nOut = n;  *pOut = 0;
			*clockOut = target;
		}
	}
	return (best!=0.0);
}


/*****************************************************************************
 * Display manipulation.
 */
status_t
calcmoderegs (
register struct moderegs	*mr,
register display_mode		*dm,
register struct devinfo		*di
)
{
	float		clockin, clockout;
	uint32		val32;
	int		d, n, p;
	int		hdisp, hsyncstart, hsyncend, htotal;
	int		vdisp, vsyncstart, vsyncend, vtotal;
	int		htotalpix, vtotalpix;
	int		bytesperrow, bytespp;
	int		i;

	/*  Convert sync parameters into archaic VGA character clocks.  */
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

	mr->mr_Depth	= colorspacebits (dm->space);
	bytespp		= (mr->mr_Depth + 7) >> 3;
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

	/*  Calculate PLL clock coefficients  */
	clockin = dm->timing.pixel_clock;
dprintf (("+++ Requesting pixel clock %d KHz...\n", dm->timing.pixel_clock));
	if (!ClockSelect (di->di_GfxCardInfo->ci_MasterClockKHz,
			  clockin, &clockout,
			  &d, &n, &p))
	{
		dprintf (("LynxEM: Can't compute (M,N,P) triplet: %d KHz\n",
			  dm->timing.pixel_clock));
		return (B_ERROR);
	}
	mr->mr_VCLK_NUM   = n;
	mr->mr_VCLK_DENOM = VAL2FIELD (SR_VCLK_DENOM, PDIV, p)
	                  | VAL2FIELD (SR_VCLK_DENOM, DENOM, d);

	mr->mr_BytesPerRow	= dm->virtual_width * bytespp;
	mr->mr_FBSize		= mr->mr_BytesPerRow * dm->virtual_height;
	mr->mr_FBULOffset	= (dm->v_display_start * dm->virtual_width +
				   dm->h_display_start) * bytespp;

	/*
	 * CRTC sync parameters.
	 * Not fully symbolized, but lynxemdefs.h should provide some clues.
	 * Hardware-dependent fenceposts are added (or subtracted) here.
	 */     
	mr->mr_CRTC[CR_HTOTAL_7_0]	= Set8Bits (htotal - 5);
	mr->mr_CRTC[CR_HDISPEND_7_0]	= Set8Bits (hdisp - 1);
	mr->mr_CRTC[CR_HBLANKSTART_7_0]	= Set8Bits (hdisp - 1);	// No border
	mr->mr_CRTC[CR_HBLANKEND]	= SetBitField (htotal - 1, 4:0, 4:0) |
					  SetBit (7);
	mr->mr_CRTC[CR_HSYNCSTART_7_0]	= Set8Bits (hsyncstart);
	mr->mr_CRTC[CR_HSYNCEND]	= SetBitField (htotal - 1, 5:5, 7:7) |
					  SetBitField (hsyncend, 4:0, 4:0);
	mr->mr_CRTC[CR_VTOTAL_7_0]	= SetBitField (vtotal - 2, 7:0, 7:0);

	mr->mr_CRTC[CR_OVERFLOW]	= SetBitField (vtotal - 2, 8:8, 0:0) |
					  SetBitField (vdisp - 1, 8:8, 1:1) |
					  SetBitField (vsyncstart, 8:8, 2:2) |
	/*  No border  */		  SetBitField (vdisp, 8:8, 3:3) |
					  SetBit (4) |
					  SetBitField (vtotal - 2, 9:9, 5:5) |
					  SetBitField (vdisp - 1, 9:9, 6:6) |
					  SetBitField (vsyncstart, 9:9, 7:7);

	mr->mr_CRTC[CR_FONTHEIGHT]	= SetBitField (vdisp, 9:9, 5:5) |
					  SetBit (6);
	mr->mr_CRTC[CR_VSYNCSTART_7_0]	= Set8Bits (vsyncstart);
	mr->mr_CRTC[CR_VSYNCEND]	= SetBitField (vsyncend, 3:0, 3:0) |
					  SetBit (5);
	mr->mr_CRTC[CR_VDISPEND_7_0]	= Set8Bits (vdisp - 1);
	mr->mr_CRTC[CR_PITCH_7_0]	= Set8Bits (bytesperrow);
	mr->mr_CRTC[CR_VBLANKSTART_7_0]	= Set8Bits (vdisp - 1);	// No border
	mr->mr_CRTC[CR_VBLANKEND_7_0]	= Set8Bits (vtotal - 1);// No border

	/*  LynxEM regs  */
	mr->mr_CRTC[CR_EXTRAS]		= SetBitField (vtotal - 2, 10:10, 3:3)
					| SetBitField (vdisp - 1, 10:10, 2:2)
					| SetBitField (vdisp, 10:10, 1:1)
					| SetBitField (vsyncstart, 10:10, 0:0);

	/*  Default values for registers not otherwise initialized.  */
	mr->mr_CRTC[CR_FONTROLL]	=
	mr->mr_CRTC[CR_TXCURSTOP]	=
	mr->mr_CRTC[CR_TXCURSBOT]	=
	mr->mr_CRTC[CR_TXCURSLOC_15_8]	=
	mr->mr_CRTC[CR_TXCURSLOC_7_0]	=
	mr->mr_CRTC[CR_ULLOC]		= 0x00;
	mr->mr_CRTC[CR_MODE]		= MASKEXPAND (CR_MODE_NOTRESET) |
					  MASKEXPAND (CR_MODE_FETCH8) |
					  MASKEXPAND (CR_MODE_SELECTROWSCAN) |
					  MASKEXPAND (CR_MODE_COMPAT);
	mr->mr_CRTC[CR_LINECMP_7_0]	= 0xFF;
#ifdef SCANDOUBLING_WORKS
	if (vt->vt_ScanDouble)
		mr->mr_CRTC[CR_FONTHEIGHT] |=
		 MASKEXPAND (CR_FONTHEIGHT_SCANDOUBLE);
#endif

	/*  Set framebuffer base and horizontal panning values.  */
//	setdispoffset (ci, cc, dm);

	/*  Set sync polarities.  */
	mr->mr_Misc = di->di_GfxCardCtl->cc_ModeRegs.mr_Misc &
		      ~(MASKFIELD (MISC, VSYNCPOL) |
			MASKFIELD (MISC, HSYNCPOL));
	if (!(dm->timing.flags & B_POSITIVE_HSYNC))
		/*  Sense of MISC bits is inverted  */
		mr->mr_Misc |= DEF2FIELD (MISC, HSYNCPOL, NEGATIVE);
	if (!(dm->timing.flags & B_POSITIVE_VSYNC))
		mr->mr_Misc |= DEF2FIELD (MISC, VSYNCPOL, NEGATIVE);

	/*  Extended configuration registers  */
	/*  Setup the DAC width.  */
	val32 = mr->mr_GrVidCtl & ~MASKFIELD (VP_GRVIDCTL, GFXFORMAT);
	switch (mr->mr_Depth) {
	case 8:
		val32 |= VAL2FIELD (VP_GRVIDCTL, GFXFORMAT, FBFORMAT_CMAP8);
		break;
	case 15:
		val32 |= VAL2FIELD (VP_GRVIDCTL, GFXFORMAT, FBFORMAT_RGB555);
		break;
	case 16:
		val32 |= VAL2FIELD (VP_GRVIDCTL, GFXFORMAT, FBFORMAT_RGB565);
		break;
	case 32:
		val32 |= VAL2FIELD (VP_GRVIDCTL, GFXFORMAT, FBFORMAT_xRGB8888);
		break;
	}
	mr->mr_GrVidCtl = val32;
	
	mr->mr_FBWidthPitch	= VAL2FIELD (VP_FBWIDTHPITCH,
				             WIDTH,
				             (dm->timing.h_display * bytespp)
				              >> 3)
				| VAL2FIELD (VP_FBWIDTHPITCH,
				             PITCH,
				             mr->mr_BytesPerRow >> 3);

	return (B_OK);
}


/*****************************************************************************
 * Set HW mode regs.
 ****
 * NOTE: It is extremely important that the accelerant re-initialize
 * its acceleration state after reconfiguring the display mode.
 */
status_t
setmode (
register struct moderegs	*mr,
register struct devinfo		*di,
register struct gfx_card_info	*ci
)
{
	register vgaregs	*vregs;
	gfx_card_ctl		*cc;
	BMemSpec		oldalloc;
	status_t		retval;
	uint32			addr, val32;
	int			i;
	uint8			val8;

	cc = di->di_GfxCardCtl;
	vregs = ci->ci_VGARegs;

	/*
	 * Get memory for the new framebuffer.
	 */
	/*  Save previous allocation.  */
	memcpy (&oldalloc, &ci->ci_FBAlloc, sizeof (oldalloc));

	/*  Free existing framebuffer.  */
	if (ci->ci_FBAlloc.ms_MR.mr_Size) {
		if ((retval = (ggpm->gpm_FreeByMemSpec) (&ci->ci_FBAlloc)) < 0)
		{
dprintf (("+++ FREE OF OLD FRAMEBUFFER FAILED!\n"));
			return (retval);
		}
		ci->ci_FBAlloc.ms_MR.mr_Size = 0;
	}

	/*  Get the new one.  */
	ci->ci_FBAlloc.ms_MR.mr_Size	= mr->mr_FBSize;
	ci->ci_FBAlloc.ms_MR.mr_Owner	= 0;
	ci->ci_FBAlloc.ms_PoolID	= ci->ci_PoolID;
	ci->ci_FBAlloc.ms_AddrCareBits	= 3;	/*  64-bit-aligned  */
	ci->ci_FBAlloc.ms_AddrStateBits	= 0;
	ci->ci_FBAlloc.ms_AllocFlags	= 0;
	if ((retval = (ggpm->gpm_AllocByMemSpec) (&ci->ci_FBAlloc)) < 0) {
dprintf (("+++!!! Display buffer allocation failed: %d (0x%08lx)\n", retval, retval));
		/*  CRAP!  Try and restore previous allocation.  */
		memcpy (&ci->ci_FBAlloc, &oldalloc, sizeof (oldalloc));
		if ((retval = (ggpm->gpm_AllocByMemSpec) (&ci->ci_FBAlloc)) < 0)
		{
			/*  Well, I don't know what else I can do...  */
dprintf (("+++!!! YIKES!  Can't restore framebuffer allocation!!  retval = %d\n",
 retval));
		}
		return (retval);
	}
	ci->ci_FBBase = ci->ci_FBAlloc.ms_MR.mr_Offset;
dprintf (("+++ ci->ci_FBBase allocated at offset 0x%08lx\n", ci->ci_FBBase));

	ci->ci_BytesPerRow = mr->mr_BytesPerRow;
	ci->ci_Depth = mr->mr_Depth;

	/*
	 * Set framebuffer base and horizontal panning values.
	 * We couldn't do this in calcmoderegs() because we didn't know what
	 * the value of ci_FBBase would turn out to be.
	 */
	addr = ci->ci_FBBase + mr->mr_FBULOffset;
	mr->mr_ATTR[AR_HPAN] = (addr & 3) << 1;	// Pixel panning

//	addr >>= 2;	// 32-bit quantization
	mr->mr_CRTC[CR_FBBASE_9_2]	= SetBitField (addr, 9:2, 7:0);
	mr->mr_CRTC[CR_FBBASE_17_10]	= SetBitField (addr, 17:10, 7:0);
	val8 = mr->mr_CRTC[CR_EXTRAS] & ~MASKFIELD (CR_EXTRAS, FBBASE_20_18);
	mr->mr_CRTC[CR_EXTRAS]		= val8 | SetBitField (addr, 20:18, 6:4);

	/*
	 * Okay, start slamming registers (right up against the WALL!!).
	 */
	screen_off (vregs);

	/*  Write PLL clock coefficients  */
	IDXREG_W (vregs, VGA_SR, SR_VCLK_NUM_7_0, mr->mr_VCLK_NUM);
	IDXREG_W (vregs, VGA_SR, SR_VCLK_DENOM, mr->mr_VCLK_DENOM);

	/*
	 * Write sync and other legacy registers.  This is the same
	 * sequence as the XFree86 code.
	 */
	/*  Misc output register  */
	vregs->w.VGA_Misc = mr->mr_Misc;

	/*  Sequencer registers  */
	write_table (&vregs->w.VGA_SR_Idx, seq_table,
		     SR_CLOCK, SR_MODE);

	write_synctable (&vregs->w.VGA_CR_Idx, mr->mr_CRTC);
	/*  Set the pitch and FBBase.  */
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_9_2,   mr->mr_CRTC[CR_FBBASE_9_2]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_17_10, mr->mr_CRTC[CR_FBBASE_17_10]);

	/*  Graphics controller registers  */
	write_table (&vregs->w.VGA_GR_Idx, gcr_table,
		     GR_SETRESET, GR_LATCHMASK);

	/*  Attribute registers  */
	i = vregs->r.VGA_ST01;
	for (i = 0;  i < NREGS_ATTR;  i++)
	{
		vregs->w.VGA_AR_Idx = i;
		vregs->w.VGA_AR_ValW = mr->mr_ATTR[i];
	}

	/*  Setup the DAC width.  */
	ci->ci_VPRegs->GrVidCtl = mr->mr_GrVidCtl;

	/*  Setup display base address, width, and pitch.  */
	ci->ci_VPRegs->FBBase		= VAL2FIELD (VP_FBBASE, BASEADDR,
					             addr >> 3);
	ci->ci_VPRegs->FBWidthPitch	= mr->mr_FBWidthPitch;

	/*  Wait for VSync again.  */
//	i = ~(vregs->r.VGA_ST01) & MASKFIELD (ST01, VBLACTIVE);
//	while (i ^ (vregs->r.VGA_ST01 & MASKFIELD (ST01, VBLACTIVE)))
//		/*  Wait for state to change  */
//		;

	vregs->w.VGA_AR_Idx = DEF2FIELD (AR_IDX, ACCESS, LOCK);

	screen_on (vregs);
dprintf (("+++ display mode set done.\n"));

	return (B_OK);
}


/*****************************************************************************
 * Miscellaneous useful routines.
 */
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

static void
write_synctable (vuint8 *addr, const uint8 *table)
{
	register int	i;
	static uint8	idxs[] = {
		CR_HTOTAL_7_0,
		CR_HDISPEND_7_0,
		CR_HBLANKSTART_7_0,
		CR_HBLANKEND,
		CR_HSYNCSTART_7_0,
		CR_HSYNCEND,
		CR_VTOTAL_7_0,
		CR_OVERFLOW,
		CR_FONTHEIGHT,
		CR_VSYNCSTART_7_0,
		CR_VSYNCEND,
		CR_VDISPEND_7_0,
		CR_PITCH_7_0,
		CR_VBLANKSTART_7_0,
		CR_VBLANKEND_7_0,
		CR_EXTRAS
	};
#define	NSYNCS		(sizeof (idxs) / sizeof (uint8))
	
	for (i = 0;  i < NSYNCS;  i++)
		(*(vuint16 *) addr = ((uint16) table[idxs[i]] << 8) | idxs[i]);
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
		  SR_RESET, DEF2FIELD (SR_RESET, ASYNC, ENABLE) |
			    DEF2FIELD (SR_RESET, SYNC, DISABLE));

	/*  Turn off display  */
	vregs->w.VGA_SR_Idx = SR_CLOCK;
	IDXREG_W (vregs, VGA_SR,
		  SR_CLOCK,
		  vregs->r.VGA_SR_Val | MASKEXPAND (SR_CLOCK_VIDDISABLE));

	/*  Release chip  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, DEF2FIELD (SR_RESET, ASYNC, DISABLE) |
			    DEF2FIELD (SR_RESET, SYNC, DISABLE));
}

void
screen_on (register union vgaregs *vregs)
{
	/*  Place chip in reset  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, DEF2FIELD (SR_RESET, ASYNC, ENABLE) |
			    DEF2FIELD (SR_RESET, SYNC, DISABLE));

	/*  Turn on display  */
	vregs->w.VGA_SR_Idx = SR_CLOCK;
	IDXREG_W (vregs, VGA_SR,
		  SR_CLOCK,
		  vregs->r.VGA_SR_Val & ~MASKEXPAND (SR_CLOCK_VIDDISABLE));

	/*  Release chip  */
	IDXREG_W (vregs, VGA_SR,
		  SR_RESET, DEF2FIELD (SR_RESET, ASYNC, DISABLE) |
			    DEF2FIELD (SR_RESET, SYNC, DISABLE));
}


/*****************************************************************************
 * Validate proposed mode settings.
 */
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
	    di->di_GfxCardInfo->ci_MemSize)
		target->virtual_height =
		 di->di_GfxCardInfo->ci_MemSize / row_bytes;

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
 * Initialization sequence.
 */
status_t
vid_init (register struct devinfo *di)
{
	register gfx_card_info	*ci;
	register vgaregs	*vregs;
	gfx_card_ctl		*cc;
	BMemSpec		ms;
	physical_entry		entry;
	status_t		ret = B_OK;
	uint32			base;
	uint8			val;
	int			i;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;

	vregs = ci->ci_VGARegs;
	ci->ci_MasterClockKHz = MASTERCLK;

	/*
	 * Determine size and physical address of "framebuffer" RAM.
	 */
	val = FIELD2VAL (SR_BIOSDATA, MEMSIZE,
	                 IDXREG_IOR (vregs, VGA_SR, SR_BIOSDATA));
	switch (val) {
	/*  FIXME: These values don't work for Lynx3D[M] series.  */
	default:
		dprintf (("+++ Bad memory size recorded by BIOS; assuming 2M.\n"));
	case 1:
		ci->ci_MemSize = 2 << 20;
		break;
	case 2:
		ci->ci_MemSize = 4 << 20;
		break;
	}
dprintf (("+++ ci_MemSize: %d\n", ci->ci_MemSize));

	/*
	 * Determine panel size.
	 */
	val = FIELD2VAL (SR_LCDTYPE, PANELTYPE,
	                 IDXREG_IOR (vregs, VGA_SR, SR_LCDTYPE));
	switch (val) {
	case SR_LCDTYPE_SIZE_640x480:
		ci->ci_LCDWidth = 640;  ci->ci_LCDHeight = 480;  break;
	case SR_LCDTYPE_SIZE_800x600:
		ci->ci_LCDWidth = 800;  ci->ci_LCDHeight = 600;  break;
	case SR_LCDTYPE_SIZE_1024x768:
		ci->ci_LCDWidth = 1024; ci->ci_LCDHeight = 768;  break;
	case SR_LCDTYPE_SIZE_1280x1024:
		ci->ci_LCDWidth = 1280; ci->ci_LCDHeight = 1024; break;
	}

	/*
	 * Declare framebuffer to genpool module.
	 * (Jason estimates an average of 128 allocations per megabyte.
	 * Hence the weird MemSize>>13 down there.)
	 */
	gfbpi->pi_Pool_AID	= di->di_BaseAddr0ID;
	gfbpi->pi_Pool		= (void *) ci->ci_BaseAddr0;
	gfbpi->pi_Size		= ci->ci_MemSize;
	strcpy (gfbpi->pi_Name, "LynxEM Framebuffer");
	if ((ret = (ggpm->gpm_CreatePool) (gfbpi, ci->ci_MemSize >> 13, 0)) < 0)
	{
dprintf (("++! Failed to create genpool, retval = %d (0x%08lx)\n", ret, ret));
		vid_uninit (di);
		return (ret);
	}
	ci->ci_PoolID = gfbpi->pi_PoolID;

	/*
	 * Allocate cursor out of the pool.
	 */
	ms.ms_MR.mr_Size	= 2 << 10;	/*  2K  */
	ms.ms_MR.mr_Owner	= 0;
	ms.ms_PoolID		= ci->ci_PoolID;
	ms.ms_AddrCareBits	= 3;		/*  32-bit-aligned	*/
	ms.ms_AddrStateBits	= 0;
	ms.ms_AllocFlags	= 0;		/*  FIXME: alloc from end?  */
	if ((ret = (ggpm->gpm_AllocByMemSpec) (&ms)) < 0) {
dprintf (("++! Failed to allocate cursor, retval = %d (0x%08lx)\n", ret, ret));
		(ggpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
		vid_uninit (di);
		return (ret);
	}
	ci->ci_CursorOffset = ms.ms_MR.mr_Offset;
dprintf (("+++ Cursor allocated at offset 0x%08lx\n", ci->ci_CursorOffset));

	/*
	 * Unlock a buttload of stuff.
	 */
	/*  Enable write access to standard VGA CRTC registers 0-7  */
	val = IDXREG_IOR (vregs, VGA_CR, CR_VSYNCEND);
	outb (offsetof (vgaregs, w.VGA_CR_Val),
	      SETVAL2FIELD (val, CR_VSYNCEND, PROTECT, FALSE));

	/*  Enable write access to all other Lynx registers.  */
	val = inb (offsetof (vgaregs, r.VGA_Lock));
	outb (offsetof (vgaregs, w.VGA_Lock),
	      SETDEF2FIELD (val, LOCK, PROTECT, DISABLE));

	/*  Set up the chip.  */
	val = IDXREG_IOR (vregs, VGA_SR, SR_GFXCMD1)
	    & ~(MASKFIELD (SR_GFXCMD1, PCIBURSTRWENABLE)
	      | MASKFIELD (SR_GFXCMD1, MEMMAP));
	IDXREG_IOW (vregs, VGA_SR, SR_GFXCMD1,
	            val
	            | VAL2FIELD (SR_GFXCMD1, PCIBURSTRWENABLE, TRUE)
	            | DEF2FIELD (SR_GFXCMD1, MEMMAP, NORMAL));

	val = IDXREG_IOR (vregs, VGA_SR, SR_GFXCMD2)
	    & ~(MASKFIELD (SR_GFXCMD2, MEMAPERTURE)
	      | MASKFIELD (SR_GFXCMD2, DISPMODE)
	      | MASKFIELD (SR_GFXCMD2, CPUPATH)
	      | MASKFIELD (SR_GFXCMD2, LINEARMODEENABLE));
	IDXREG_IOW (vregs, VGA_SR, SR_GFXCMD2,
	            val
	            | DEF2FIELD (SR_GFXCMD2, MEMAPERTURE, DUAL)
	            | DEF2FIELD (SR_GFXCMD2, DISPMODE, MODERN)
	            | DEF2FIELD (SR_GFXCMD2, CPUPATH, DIRECT)
	            | VAL2FIELD (SR_GFXCMD2, LINEARMODEENABLE, TRUE));

	/*  We should be talking memory-mapped I/O by now.  */
	IDXREG_W (vregs, VGA_SR, SR_DISABLES,
	          SETVAL2FIELD (IDXREG_R (vregs, VGA_SR, SR_DISABLES),
	                        SR_DISABLES,
	                        VIDEO,
	                        FALSE));

	vregs->w.VGA_DACMask = ~0;

	/*  Turn off "dot expansion"  */
	IDXREG_W (vregs, VGA_CR, CR_EXPANCENTER1,
	          VAL2FIELD (CR_EXPANCENTER1, 12DOTEXPAND, FALSE)
	          | VAL2FIELD (CR_EXPANCENTER1, 10DOTEXPAND, FALSE));

	/*
	 * Select I/O address 0x3D?, custom clock rate.  (Default settings
	 * from Intel.)
	 */
	cc->cc_ModeRegs.mr_Misc = vregs->r.VGA_Misc &
				   ~(MASKFIELD (MISC, VSYNCPOL) |
				     MASKFIELD (MISC, HSYNCPOL) |
				     MASKFIELD (MISC, ODDEVENPAGESEL) |
				     MASKFIELD (MISC, CLKSEL) |
				     MASKFIELD (MISC, IOADDR));
	cc->cc_ModeRegs.mr_Misc |= DEF2FIELD (MISC, ODDEVENPAGESEL, 64K) |
				   DEF2FIELD (MISC, CLKSEL, EXTRA) |
				   DEF2FIELD (MISC, IOADDR, 3Dx);
	vregs->w.VGA_Misc = cc->cc_ModeRegs.mr_Misc;

	/*  Turn on VBLank.  */

	/*
	 * Write identification info.
	 */
	strcpy (ci->ci_ADI.name, "SiliconMotion LynxEM-based board");
	strcpy (ci->ci_ADI.chipset, "LynxEM");
	ci->ci_ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ci_ADI.memory = ci->ci_MemSize;
	ci->ci_ADI.version = B_ACCELERANT_VERSION;

	memcpy (cc->cc_ModeRegs.mr_ATTR,
		attr_table_init,
		sizeof (cc->cc_ModeRegs.mr_ATTR));

	return (calcmodes (di));	/*  FIXME: Move this up  */
}


void
vid_uninit (register struct devinfo *di)
{
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;
	vgaregs		*vregs;
	uint8		tmp;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	vregs = ci->ci_VGARegs;

	/*  Reset to default pixel clock.  */
	cc->cc_ModeRegs.mr_Misc = SETDEF2FIELD (cc->cc_ModeRegs.mr_Misc,
						MISC, CLKSEL, 25MHZ);
	vregs->w.VGA_Misc = cc->cc_ModeRegs.mr_Misc;

	/*  Flip back to cheezy old VGA mode.  */

	/*  Free computed display modes.  */
	if (di->di_DispModes) {
		free (di->di_DispModes);
		di->di_DispModes = NULL;
		di->di_NDispModes = 0;
	}

	/*  Delete pool describing FB RAM.  */
	if (ci->ci_PoolID) {
		(ggpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
	}
}


/*****************************************************************************
 * Calculate available display modes.
 */
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
		if (di->di_DispModes = malloc (di->di_NDispModes *
					       sizeof (display_mode)))
		{
			testbuildmodes (di, di->di_DispModes);
			return (B_OK);
		}
	}
	return (B_NO_MEMORY);
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
		{ /*B_CMAP8, B_RGB15_LITTLE,*/ B_RGB16_LITTLE/*, B_RGB32_LITTLE*/ };
#else
	static color_space spaces[] =
		{ /*B_CMAP8, B_RGB15_BIG,*/ B_RGB16_BIG/*, B_RGB32_BIG*/ };
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
