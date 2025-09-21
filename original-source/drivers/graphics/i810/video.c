/* :ts=8 bk=0
 *
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/i810defs.h>
#include <graphics_p/i810/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
/*
 * The docs say 48MHz, but back-calculating from the MNP values in the
 * mode table from Intel suggests this is correct.  48MHz may in fact be
 * correct, but that would require messing with the magic constant '4' in
 * the ClockSelect() function, so I'll just fudge it here.
 */
#define MASTERCLK		(48000.0/2.0)


#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR \
			 | B_PARALLEL_ACCESS | B_DPMS \
			 | B_SUPPORTS_OVERLAYS)
#define	SYNC_HnVn	0
#define	SYNC_HnVP	B_POSITIVE_VSYNC
#define	SYNC_HPVn	B_POSITIVE_HSYNC
#define	SYNC_HPVP	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)


/*****************************************************************************
 * Structure definitions
 */
typedef struct waterentry {
	float	we_PixClock;
	uint32	we_RegValue[6];	/*  Add WATERIDX_ values together  */
} waterentry;
#define	WATERIDX_100MHZ		0
#define	WATERIDX_133MHZ		3

#define	WATERIDX_8BPP		0
#define	WATERIDX_16BPP		1
#define	WATERIDX_24BPP		2


/*****************************************************************************
 * Local prototypes.
 */
static void write_table(vuint8 *addr, const uint8 *table, int first, int last);
static void write_synctable(vuint8 *addr, const uint8 *table);
static status_t initGTT(struct devinfo *di, register struct gfx_card_info *ci);
static status_t calcmodes(struct devinfo *di);
static int testbuildmodes(struct devinfo *di, register display_mode *dst);


/*****************************************************************************
 * Globals.
 */
extern pci_module_info	*pci_bus;
extern genpool_module	*ggpm;
extern driverdata	*gdd;
extern BPoolInfo	*gfbpi;


/*
 * This table is formatted for 132 columns, so stretch that window...
 * Taken from Intel's I810 mode table.
 */
static const display_mode	mode_list[] = {
/* Intel: 640x480 @ 60Hz */
 {	{  25175,  640,  648,  744,  800,  480,  489,  491,  525, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Intel: 640x480 @ 72Hz */
 {	{  31500,  640,  656,  696,  832,  480,  488,  491,  520, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Intel: 640x480 @ 75Hz */
 {	{  31500,  640,  648,  712,  840,  480,  480,  483,  500, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Intel: 640X480 @ 85Hz */
 {	{  36000,  640,  688,  744,  832,  480,  480,  483,  509, SYNC_HnVn},		B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Intel: 800x600 @ 56Hz */
 {	{  36000,  800,  816,  888, 1024,  600,  600,  602,  625, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Intel: 800x600 @ 60Hz */
 {	{  40000,  800,  832,  960, 1056,  600,  600,  604,  628, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Intel: 800x600 @ 72Hz */
 {	{  50000,  800,  848,  968, 1040,  600,  636,  642,  666, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Intel: 800x600 @ 75Hz */
 {	{  49500,  800,  808,  888, 1056,  600,  600,  603,  625, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Intel: 800x600 @ 85Hz */
 {	{  56250,  800,  824,  888, 1048,  600,  600,  603,  631, SYNC_HPVP},		B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* VESA: 1024x768 @ 43Hz interlaced */
 {	{  46600, 1024, 1032, 1208, 1264,  384,  385,  388,  408, B_TIMING_INTERLACED},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Intel: 1024x768 @ 60Hz */
 {	{  65000, 1024, 1040, 1176, 1344,  768,  770,  776,  806, SYNC_HnVn},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Intel: 1024x768 @ 70Hz */
 {	{  75000, 1024, 1040, 1176, 1328,  768,  770,  776,  806, SYNC_HnVn},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Intel: 1024x768 @ 75Hz */
 {	{  78750, 1024, 1032, 1128, 1312,  768,  768,  771,  800, SYNC_HPVP},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Intel: 1024x768 @ 85Hz */
 {	{  94500, 1024, 1064, 1160, 1376,  768,  768,  771,  808, SYNC_HPVP},		B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Intel: 1152x864 @ 60Hz */
 {	{  80000, 1152, 1176, 1272, 1472,  864,  864,  867,  905, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
 {	{  94200, 1152, 1216, 1336, 1536,  864,  864,  867,  900, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* VESA: 1152x864 @ 75Hz */
 {	{ 108000, 1152, 1208, 1336, 1600,  864,  864,  867,  900, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
 {	{ 121500, 1152, 1208, 1336, 1576,  864,  864,  867,  917, SYNC_HPVP},		B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
/* Intel: 1280x1024 @ 60Hz */
 {	{ 108000, 1280, 1320, 1432, 1688, 1024, 1024, 1027, 1066, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Intel: 1280x1024 @ 70Hz */
 {	{ 128890, 1280, 1360, 1496, 1728, 1024, 1024, 1027, 1066, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Intel: 1280x1024 @ 75Hz */
 {	{ 135000, 1280, 1288, 1432, 1688, 1024, 1024, 1027, 1066, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Intel: 1280x1024 @ 85Hz */
 {	{ 157500, 1280, 1336, 1496, 1728, 1024, 1024, 1027, 1072, SYNC_HPVP},		B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS},
/* Intel: 1600x1200 @ 60Hz */
 {	{ 162000, 1600, 1656, 1848, 2160, 1200, 1200, 1203, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Intel: 1600x1200 @ 65Hz */
 {	{ 175500, 1600, 1656, 1848, 2160, 1200, 1200, 1203, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Intel: 1600x1200 @ 70Hz */
 {	{ 189000, 1600, 1656, 1848, 2160, 1200, 1200, 1203, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Intel: 1600x1200 @ 75Hz */
 {	{ 202500, 1600, 1656, 1848, 2160, 1200, 1200, 1203, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Matrox: Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
 {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Intel: 1600x1200 @ 85Hz */
 {	{ 229500, 1600, 1656, 1848, 2160, 1200, 1200, 1203, 1250, SYNC_HPVP},		B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
/* Intel: 1792x1344 @ 60Hz */
 {	{ 204750, 1792, 1912, 2112, 2448, 1344, 1344, 1347, 1394, SYNC_HnVP},		B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS},
/* VESA: 1792x1344 @ 75Hz */
 {	{ 261000, 1792, 1888, 2104, 2456, 1344, 1345, 1348, 1417, SYNC_HnVP},		B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS},
/* Intel: 1856x1392 @ 60Hz */
 {	{ 218250, 1856, 1944, 2168, 2528, 1392, 1392, 1395, 1439, SYNC_HnVP},		B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS},
/* VESA: 1856x1392 @ 75Hz */
 {	{ 288000, 1856, 1984, 2208, 2560, 1392, 1393, 1396, 1500, SYNC_HnVP},		B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS},
/* Intel: 1920x1440 @ 60Hz */
 {	{ 234000, 1920, 2040, 2248, 2600, 1440, 1440, 1443, 1500, SYNC_HnVP},		B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS},
/* VESA: 1920x1440 @ 75Hz */
 {	{ 297000, 1920, 2064, 2288, 2640, 1440, 1441, 1444, 1500, SYNC_HnVP},		B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS},
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))

/*
 * FW_BLC entries for I810.  Taken from Intel's mode table.  May not work in
 * the general case, as the I810 (and I740) appear to be sensitive to sync
 * positions as well as clock rate.
 */
static waterentry	watertable[] = {
 {
	0,	/*  Failsafe entry  */
	0x22002000, 0x22004000, 0x22006000,	/* (Ignore spurious warning; */
	0x22002000, 0x22004000, 0x22006000	/* Absent braces is legal C) */
 }, {
	25000,
	0x22002000, 0x22004000, 0x22006000,
	0x22002000, 0x22004000, 0x22006000
 }, {
	28000,
	0x22002000, 0x22004000, 0x22006000,
	0x22002000, 0x22004000, 0x22006000
 }, {
	31000,
	0x22003000, 0x22005000, 0x22007000,
	0x22003000, 0x22005000, 0x22007000
 }, {
	33000,
	0x22002000, 0x22005000, 0x22006000,
	0x22002000, 0x22005000, 0x22006000
 }, {
	35000,
	0x22003000, 0x22004000, 0x22008000,
	0x22003000, 0x22004000, 0x22008000
 }, {
	36000,
	0x22003000, 0x22005000, 0x22107000,
	0x22003000, 0x22005000, 0x22107000
 }, {
	40000,
	0x22003000, 0x22006000, 0x22108000,
	0x22003000, 0x22006000, 0x22108000
 }, {
	43000,
	0x22004000, 0x22006000, 0x22008000,
	0x22004000, 0x22006000, 0x22008000
 }, {
	45000,
	0x22004000, 0x22007000, 0x2210a000,
	0x22004000, 0x22007000, 0x2210a000
 }, {
	49000,
	0x22004000, 0x22007000, 0x2210b000,
	0x22004000, 0x22007000, 0x2210b000
 }, {
	50000,
	0x22004000, 0x22007000, 0x2210a000,
	0x22004000, 0x22007000, 0x2210a000
 }, {
	56000,
	0x22004000, 0x22108000, 0x2210b000,
	0x22004000, 0x22108000, 0x2210b000
 }, {
	65000,
	0x22005000, 0x22109000, 0x2220d000,
	0x22005000, 0x22109000, 0x2220d000
 }, {
	74000,
	0x2210a000, 0x22210000, 0x22415000,
	0x2210a000, 0x22210000, 0x22415000
 }, {
	75000,
	0x22005000, 0x2210a000, 0x2220f000,
	0x22005000, 0x2210a000, 0x2220f000
 }, {
	78000,
	0x22006000, 0x2210b000, 0x22210000,
	0x22006000, 0x2210b000, 0x22210000
 }, {
//	80000,	/*  This entry is rather weird; I don't trust it...  */
//	0x2220c000, 0x22210000, 0x22415000,
//	0x2220c000, 0x22210000, 0x22415000
// }, {
	94000,
	0x22007000, 0x2220e000, 0x22212000,
	0x22007000, 0x2220e000, 0x22212000
 }, {
	96000,
	0x22107000, 0x22210000, 0x22415000,
	0x22107000, 0x22210000, 0x22415000
 }, {
	99000,
	0x22107000, 0x22210000, 0x22415000,
	0x22107000, 0x22210000, 0x22415000
 }, {
	108000,
	0x22107000, 0x22210000, 0x22415000,
	0x22107000, 0x22210000, 0x22415000
 }, {
	108000,
	0x22107000, 0x22210000, 0x22415000,
	0x22107000, 0x22210000, 0x22415000
 }, {
	108000,
	0x2210a000, 0x22210000, 0x22415000,
	0x2210a000, 0x22210000, 0x22415000
 }, {
	110000,
	0x2210a000, 0x22220000, 0x2241b000,
	0x2210a000, 0x22220000, 0x2241b000
 }, {
	119000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	121000,
	0x2220c000, 0x22210000, 0x22415000,
	0x2220c000, 0x22210000, 0x22415000
 }, {
	129000,
	0x22107000, 0x22210000, 0x22419000,
	0x22107000, 0x22210000, 0x22419000
 }, {
	132000,
	0x22109000, 0x22314000, 0x22515000,
	0x22109000, 0x22314000, 0x22515000
 }, {
	135000,
	0x22109000, 0x22314000, 0x2251d000,
	0x22109000, 0x22314000, 0x2251c000
 }, {
	148000,
	0x2210a000, 0x22220000, 0x2241d000,
	0x2210a000, 0x22220000, 0x2241d000
 }, {
	152000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	157000,
	0x2210b000, 0x22415000, 0x2251d000,
	0x2210b000, 0x22415000, 0x2251d000
 }, {
	162000,
	0x2210b000, 0x22416000, 0x44419000,
	0x2210b000, 0x22416000, 0x44419000
 }, {
	175000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	189000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	195000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	202000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	204000,
	0x2220e000, 0x22416000, 0x44419000,
	0x2220e000, 0x22416000, 0x44419000
 }, {
	218000,
	0x2220f000, 0x00000000, 0x00000000,
	0x2220f000, 0x00000000, 0x00000000
 }, {
	229000,
	0x22210000, 0x22416000, 0x00000000,
	0x22210000, 0x22416000, 0x00000000
 }, {
	234000,
	0x22210000, 0x00000000, 0x00000000,
	0x22210000, 0x00000000, 0x00000000
 }
};
#define	NWATERS		(sizeof (watertable) / sizeof (waterentry))


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
int
isowner (struct openerinfo *oi)
{
	/*  Return TRUE if this opener currently 0WNZ the display  */
	return (oi == (openerinfo *)
	              FIRSTNODE (&oi->oi_DI->di_GfxCardInfo->ci_OpenerList));
}


status_t
setopenertomode (
register struct openerinfo	*oi,
register struct gfx_card_info	*ci,
register display_mode		*dm
)
{
	status_t	retval;

	if ((retval = calcdispconfig (&oi->oi_DC, dm, oi->oi_DI)) == B_OK) {
//		gfx_card_info	*ci = oi->oi_DI->di_GfxCardInfo;

		/*  Move to head of opener list; we now own the display.  */
		/*  FIXME:  Ideally this list operation should be protected  */
		RemNode (&oi->oi_Node);
		AddHead (&oi->oi_Node, &ci->ci_OpenerList);

		acquire_sem (ci->ci_DispConfigLock);
		acquire_sem (ci->ci_VGARegsLock);
		writemode (&oi->oi_DC, oi->oi_DI, ci);
		release_sem (ci->ci_VGARegsLock);
		release_sem (ci->ci_DispConfigLock);
	}
	return (retval);
}

status_t
setopenertoGLmode (
struct openerinfo		*oi,
struct gfx_card_info		*ci,
struct i810drv_set_gl_disp	*sgd
)
{
	dispconfig	*dc = NULL;
	status_t	retval = B_ERROR;
	int		makeowner = FALSE;

	if (sgd->sgd_Width  &&  sgd->sgd_Height) {
		/*
		 * Search for best match.  This is allowed to fail.
		 */
		display_mode	*dm, *mode = NULL;
		float		diff = 9.9e99;
		int		i;

		for (dm = oi->oi_DI->di_DispModes, i = oi->oi_DI->di_NDispModes;
		     --i >= 0;
		     dm++)
		{
			if (dm->timing.h_display == sgd->sgd_Width  &&
			    dm->timing.v_display == sgd->sgd_Height  &&
			    dm->space == sgd->sgd_ColorSpace)
			{
				float test;
	
				test = (dm->timing.pixel_clock * 1000.0)
				     / (float) (dm->timing.h_total
				               * dm->timing.v_total);
				if ((test -= (float) sgd->sgd_FieldRate) < 0)
					test = -test;
				if (test < diff) {
					diff = test;
					mode = dm;
				}
			}
		}
		if (mode) {
			if ((retval = calcdispconfig
			               (&oi->oi_DC, mode, oi->oi_DI)) == B_OK)
			{
				dc = &oi->oi_DC;
				makeowner = TRUE;
			}
		}
dprintf (("+++ Setting OpenGL mode:\n"));
	} else {
		/*
		 * "I'll have what he's having."  If this opener is the owner
		 * of the display, set the mode to the same as the guy next
		 * in the OpenerList, if any.
		 */
		openerinfo	*first, *next;

		first = (openerinfo *) FIRSTNODE (&ci->ci_OpenerList);
		if (first == oi) {
			/*  This is the top-most opener.  */
			for (next = first;
			     NEXTNODE (next);
			     next = (openerinfo *) NEXTNODE (next))
			{
				if (next != oi) {
					dc = &oi->oi_DC;
					memcpy (dc, &next->oi_DC,
					        sizeof (dispconfig));
					/*  Cursor state is "global"  */
					calcshowcursor (dc, oi->oi_DI, FALSE);
					break;
				}
			}
		}
dprintf (("+++ Recovering from OpenGL mode:\n"));
	}

	if (dc) {
dprintf (("+++   %d * %d @ %ld pixclk\n",
 dc->dc_DispMode.timing.h_display, dc->dc_DispMode.timing.v_display,
 dc->dc_DispMode.timing.pixel_clock));

		acquire_sem (ci->ci_DispConfigLock);
		acquire_sem (ci->ci_VGARegsLock);
		writemode (dc, oi->oi_DI, ci);
		release_sem (ci->ci_VGARegsLock);
		release_sem (ci->ci_DispConfigLock);

		if (makeowner) {
			RemNode (&oi->oi_Node);
			AddHead (&oi->oi_Node, &ci->ci_OpenerList);
		}
		retval = B_OK;
	}
	return (retval);
}


status_t
movedisplay (struct openerinfo *oi, uint16 x, uint16 y)
{
	display_mode	*dm;

	dm = &oi->oi_DC.dc_DispMode;

	if (x + dm->timing.h_display > dm->virtual_width  ||
	    y + dm->timing.v_display > dm->virtual_height)
		return (B_ERROR);

	dm->h_display_start = x;
	dm->v_display_start = y;
	oi->oi_DC.dc_FBULOffset = (y * dm->virtual_width + x)
	                        * ((oi->oi_DC.dc_Depth + 7) >> 3);

	setopenerfbbaseregs (oi);

	return (B_OK);
}

void
setopenerfbbaseregs (struct openerinfo *oi)
{
	calcfbbaseregs (&oi->oi_DC);

	if (isowner (oi)) {
		/*  This opener owns the display  */
#if 1
		/*  Flip page at next vblank.  */
		gfx_card_ctl	*cc = oi->oi_DI->di_GfxCardCtl;

		atomic_or (&cc->cc_IRQFlags, IRQF_SETFBBASE);
#else
		/*
		 * Flip page ASAP.
		 * This is disabled due to visual artifacting problems.
		 * (Might be covered in Intel errata sheet.)
		 */
		gfx_card_info	*ci = oi->oi_DI->di_GfxCardInfo;

		acquire_sem (ci->ci_DispConfigLock);
		acquire_sem (ci->ci_VGARegsLock);
		writefbbase (&oi->oi_DC, ci);
		release_sem (ci->ci_VGARegsLock);
		release_sem (ci->ci_DispConfigLock);
#endif
	}
}


/*****************************************************************************
 * PLL coefficient calculator.
 ****
 * The following figures are taken from the Intel I810 PRM and "Changing the
 * Display Resolutions and Refresh Rates" document for the I740.  All
 * frequencies are in KHz.
 */
/*  Min/max on M and N provided by Intel.  */
#define M_MIN		1
#define M_MAX		125

#define	N_MIN		1
#define	N_MAX		125

#define P_MIN		0
#define P_MAX		5

#define	VCO_MIN		600000.0
#define	VCO_MAX		1200000.0	/*  Wow!  1.2 GHz!  */


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
	 * According to the Intel docs, the formula for calculating clock
	 * values is:
	 *
	 * F<out> = F<in> * ((4 * M) / N) / (2**P)
	 *
	 * (For the I740, it was:)
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
	 * NOTE: For the I810, back-calculating from Intel's mode table, and
	 * assuming the documented 48MHz input frequency, the magic constant
	 * appears to be '2'.  However, rather than fudge this known-working
	 * code, MASTERCLK is simply declared as half of 48MHz.
	 *
	 * There are intermediate restrictions:
	 *
	 * VCO_MIN <= F<out> * 2**P <= VCO_MAX
	 */
	/*  Calculate largest P  */
	diff = VCO_MAX / clockReq;
	for (pMax = P_MAX;  pMax > P_MIN;  pMax--)
		if (diff >= (1 << pMax))
			break;

	/*  Calculate smallest P  */
	diff = VCO_MIN / clockReq;
	for (pMin = P_MIN;  pMin <= P_MAX;  pMin++)
		if (diff <= (1 << pMin))
			break;

	if (pMax < 0  ||  pMin > P_MAX)
		return (FALSE);

	for (p = pMin;  p <= pMax;  p++) {
		float	vcoreq;
		float	f2expp = (1 << p);

		vcoreq = clockReq * f2expp;

		for (n = N_MIN;  n <= N_MAX;  n++) {
			float	fn = n;

			m = (int) (vcoreq * (fn + 2) /
				   (masterClock * 4)) - 2;
			if (m < M_MIN  ||  m > M_MAX)
				continue;

			target = masterClock * 4 * ((float) m + 2) / (fn + 2);

			diff = fabs (target - vcoreq);
			if (diff < bestDiff) {
				bestDiff = diff;
				best = target;
				*mOut = m;  *nOut = n;  *pOut = p;
				*clockOut = best / f2expp;
			}
		}
	}
	return (best!=0.0);    
}


/*****************************************************************************
 * Display manipulation.
 * Calculate dispconfig for the given display_mode; deposit results in cl_dc.
 * If the display_mode is nonsense, return an error and do not modify cl_dc.
 * dc_FBBase and dc_CMAP are initialized based on what's already in cl_dc.
 */
status_t
calcdispconfig (
struct dispconfig		*cl_dc,
register const display_mode	*dm,
register struct devinfo		*di
)
{
	dispconfig	dc;
	moderegs	*mr;
	float		clockin, clockout;
	int		m, n, p;
	int		hdisp, hsyncstart, hsyncend, htotal;
	int		vdisp, vsyncstart, vsyncend, vtotal;
	int		htotalpix, vtotalpix;
	int		bytesperrow, bytespp;
	int		i;

	mr = &dc.dc_MR;

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

	memcpy (&dc.dc_DispMode, dm, sizeof (display_mode));
	memcpy (&dc.dc_CMAP, &cl_dc->dc_CMAP, sizeof (dc.dc_CMAP));

	dc.dc_Depth	= colorspacebits (dm->space);
	bytespp		= (dc.dc_Depth + 7) >> 3;
	bytesperrow	= (dm->virtual_width >> 3) * bytespp;

	dc.dc_FBBase		= cl_dc->dc_FBBase;
	dc.dc_BytesPerRow	= dm->virtual_width * bytespp;
	dc.dc_FBULOffset	= (dm->v_display_start * dm->virtual_width +
				   dm->h_display_start) * bytespp;

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
dprintf (("+++ Requesting pixel clock %ld KHz...\n", dm->timing.pixel_clock));
	if (!ClockSelect (di->di_GfxCardInfo->ci_MasterClockKHz,
			  clockin, &clockout,
			  &m, &n, &p))
	{
		dprintf (("I810: Can't compute (M,N,P) triplet: %ld KHz\n",
			  dm->timing.pixel_clock));
		return (B_ERROR);
	}
	mr->mr_DCLK_MN2	= VAL2FIELD (DCLK_MNx, N, n) |
			  VAL2FIELD (DCLK_MNx, M, m);
	mr->mr_DCLK_P2	= VAL2FIELD (DCLK_Px, 2EXPP, p) |
			  DEF2FIELD (DCLK_Px, VCODIV, DIV4);

	/*  Find FIFO watermark value.  */
	/*  FIXME:  This doesn't really work all that well...  */
	mr->mr_FW_BLC = 0;
	for (i = 0;
	     i < NWATERS  &&  clockout > watertable[i].we_PixClock;
	     i++)
		;
	if (i == NWATERS)
		i = NWATERS - 1;

	/*  Which watermark do we grab?  */
	if (di->di_FSBFreq == 133)	n = WATERIDX_133MHZ;
	else				n = WATERIDX_100MHZ;
	switch (dc.dc_Depth) {
	case 8:
		n += WATERIDX_8BPP;
		break;
	case 15:
	case 16:
		n += WATERIDX_16BPP;
		break;
	}
	mr->mr_FW_BLC = watertable[i].we_RegValue[n];
dprintf (("+++ Selecting waterentry %d @ %d KHz.\n",
	  i, (int) watertable[i].we_PixClock));

	if (!mr->mr_FW_BLC) {
dprintf (("+++ Can't find watermark entry!\n"));
		return (B_ERROR);
	}


	/*
	 * CRTC sync parameters.
	 * Not fully symbolized, but i810defs.h should provide some clues.
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

	/*  I810 regs  */
	mr->mr_CRTC[CR_VTOTAL_11_8]	= SetBitField (vtotal - 2, 11:8, 3:0);
	mr->mr_CRTC[CR_VDISPEND_11_8]	= SetBitField (vdisp - 1, 11:8, 3:0);
	mr->mr_CRTC[CR_VSYNCSTART_11_8]	= SetBitField (vsyncstart, 11:8, 3:0);
	mr->mr_CRTC[CR_VBLANKSTART_11_8]= SetBitField (vdisp - 1, 11:8, 3:0);
	mr->mr_CRTC[CR_HTOTAL_8]	= SetBitField (htotal - 5, 8:8, 0:0);
	mr->mr_CRTC[CR_HBLANKEND_6]	= SetBitField (htotal - 1, 6:6, 0:0);

	mr->mr_CRTC[CR_PITCH_11_8]	= SetBitField (bytesperrow, 11:8, 3:0);

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
	calcfbbaseregs (&dc);

	/*  Set sync polarities.  */
	mr->mr_Misc = di->di_GfxCardInfo->ci_DispConfig.dc_MR.mr_Misc &
		      ~(MASKFIELD (MISC, VSYNCPOL) |
			MASKFIELD (MISC, HSYNCPOL));
	if (!(dm->timing.flags & B_POSITIVE_HSYNC))
		/*  Sense of MISC bits is inverted  */
		mr->mr_Misc |= DEF2FIELD (MISC, HSYNCPOL, NEGATIVE);
	if (!(dm->timing.flags & B_POSITIVE_VSYNC))
		mr->mr_Misc |= DEF2FIELD (MISC, VSYNCPOL, NEGATIVE);

	/*  Extended configuration registers  */
	mr->mr_PIXCONF = di->di_GfxCardInfo->ci_DispConfig.dc_MR.mr_PIXCONF &
			 ~MASKFIELD (PIXCONF, DISPMODE);
	switch (dc.dc_Depth) {
	case 8:
		mr->mr_BLTCTL = DEF2FIELD (BLTCTL, EXPANDMODE, 8BPP);
		mr->mr_PIXCONF |= DEF2FIELD (PIXCONF, DISPMODE, 8BPP);
		break;
	case 15:
		mr->mr_BLTCTL = DEF2FIELD (BLTCTL, EXPANDMODE, 16BPP);
		mr->mr_PIXCONF |= DEF2FIELD (PIXCONF, DISPMODE, 16BPP_555);
		break;
	case 16:
		mr->mr_BLTCTL = DEF2FIELD (BLTCTL, EXPANDMODE, 16BPP);
		mr->mr_PIXCONF |= DEF2FIELD (PIXCONF, DISPMODE, 16BPP_565);
		break;
	}
	
	calcshowcursor (&dc, di, FALSE);

	memcpy (cl_dc, &dc, sizeof (dc));

	return (B_OK);
}


/*****************************************************************************
 * Write HW mode regs.
 ****
 * NOTE: It is important that the accelerant re-initialize its acceleration/
 * rendering state after reconfiguring the display mode.
 */
void
writemode (
const struct dispconfig		*dc,
register struct devinfo		*di,
register struct gfx_card_info	*ci
)
{
	register const moderegs	*mr;
	register vgaregs	*vregs;
	gfx_card_ctl		*cc;
	uint32			val;
	int			i;

	cc = di->di_GfxCardCtl;
	mr = &dc->dc_MR;
	vregs = &ci->ci_Regs->VGARegs;

	/*
	 * Okay, start slamming registers (right up against the WALL!!).
	 * This is the sequence described in the Intel programming doc, so
	 * any failures are not my fault...
	 */
	screen_off (vregs);
	
	/*
	 * Turn off DRAM refresh, as per Intel doc.
	 */
	ci->ci_Regs->DRAMCH = SETDEF2FIELD (ci->ci_Regs->DRAMCH,
					    DRAMCH, REFRESH, DISABLE);
	snooze (1000);

	/*  Write PLL clock coefficients  */
	ci->ci_Regs->DCLK_MN2	= mr->mr_DCLK_MN2;
	ci->ci_Regs->DCLK_P2	= mr->mr_DCLK_P2;

	/*  Setup *only* the DAC width.  */
	val = ci->ci_Regs->PIXCONF & ~MASKFIELD (PIXCONF, DACWIDTH);
	ci->ci_Regs->PIXCONF	= val
				| (mr->mr_PIXCONF
				 & MASKFIELD (PIXCONF, DACWIDTH));

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
	IDXREG_W (vregs, VGA_CR, CR_PITCH_11_8,   mr->mr_CRTC[CR_PITCH_11_8]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_17_10, mr->mr_CRTC[CR_FBBASE_17_10]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_9_2,   mr->mr_CRTC[CR_FBBASE_9_2]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_23_18, mr->mr_CRTC[CR_FBBASE_23_18]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_31_24, mr->mr_CRTC[CR_FBBASE_31_24]);

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

	/*  This write latches the full FBBASE value to the hardware.  */
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_23_18,
	          mr->mr_CRTC[CR_FBBASE_23_18]
	          | VAL2FIELD (CR_FBBASE_23_18, LOADADDR, TRUE));

	/*
	 * The CR_LACECTL or GR_ADDRMAP registers are set up only once at
	 * initialization, so we don't touch them here (like the Intel doc
	 * says to).
	 */

	/*  Turn DRAM refresh back on.  */
	ci->ci_Regs->DRAMCH = SETDEF2FIELD (ci->ci_Regs->DRAMCH,
					    DRAMCH, REFRESH, ENABLE);

	/*  Extended configuration registers  */
	ci->ci_Regs->BLTCTL	= mr->mr_BLTCTL;
	ci->ci_Regs->PIXCONF	= mr->mr_PIXCONF;
	/*  Black magic from Intel to make overlays work.  */
	ci->ci_Regs->LCD_OverlayActive =
	  VAL2FIELD (OVERLAYACTIVE, END, ((mr->mr_CRTC[1] + 1) * 8) - 32)
	| VAL2FIELD (OVERLAYACTIVE, START, ((mr->mr_CRTC[0] + 5) * 8) - 32);

	/*  Interrupts and HW status are managed elsewhere.  */

dprintf (("+++ Writing 0x%08lx to FW_BLC.\n", mr->mr_FW_BLC));
	val = ci->ci_Regs->FW_BLC & ~(MASKFIELD (FW_BLC, MAINBURST)
	                            | MASKFIELD (FW_BLC, MAINLOWATER)
	                            | MASKFIELD (FW_BLC, LOCALBURST)
	                            | MASKFIELD (FW_BLC, LOCALWATER));
	ci->ci_Regs->FW_BLC	= val
				| (mr->mr_FW_BLC
				 & (MASKFIELD (FW_BLC, MAINBURST)
				  | MASKFIELD (FW_BLC, MAINLOWATER)
				  | MASKFIELD (FW_BLC, LOCALBURST)
				  | MASKFIELD (FW_BLC, LOCALWATER)));

	/*  Color map table  */
	writepaletterange (vregs, dc->dc_CMAP[0], 0, 255);

	/*  Wait for VSync again.  */
	i = ~(vregs->r.VGA_ST01) & MASKFIELD (ST01, VBLACTIVE);
	while (i ^ (vregs->r.VGA_ST01 & MASKFIELD (ST01, VBLACTIVE)))
		/*  Wait for state to change  */
		;

	vregs->w.VGA_AR_Idx = DEF2FIELD (AR_IDX, ACCESS, LOCK);

	screen_on (vregs);

	/*  Copy to globals  */
	memcpy (&ci->ci_DispConfig, dc, sizeof (*dc));

dprintf (("+++ display mode set done.\n"));
}


/*
 * Take dc_FBBase and dc_FBULOffset, and convert to register representation
 * for later stuffing.
 */
void
calcfbbaseregs (struct dispconfig *dc)
{
	moderegs	*mr;
	uint32		val;

	mr = &dc->dc_MR;

	val = dc->dc_FBBase + dc->dc_FBULOffset;

	mr->mr_ATTR[AR_HPAN] = (val & 3) << 1;	// Pixel panning

//	addr >>= 2;	// 32-bit quantization
	mr->mr_CRTC[CR_FBBASE_9_2]	= SetBitField (val, 9:2, 7:0);
	mr->mr_CRTC[CR_FBBASE_17_10]	= SetBitField (val, 17:10, 7:0);
	mr->mr_CRTC[CR_FBBASE_23_18]	= SetBitField (val, 23:18, 5:0);
	mr->mr_CRTC[CR_FBBASE_31_24]	= SetBitField (val, 31:24, 7:0);
}

void
writefbbase (
register const struct dispconfig	*dc,
register struct gfx_card_info		*ci
)
{
	register vgaregs	*vregs;
	uint8			dummy;

	vregs = &ci->ci_Regs->VGARegs;

	/*  Write HW registers  */
	dummy = vregs->r.VGA_ST01;
	vregs->w.VGA_AR_Idx = 0;	/*  Enable attribute access */
	dummy = vregs->r.VGA_ST01;	/*  Reset phase again   */

	/*  Write base address...  */
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_17_10,
	          dc->dc_MR.mr_CRTC[CR_FBBASE_17_10]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_9_2,
	          dc->dc_MR.mr_CRTC[CR_FBBASE_9_2]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_23_18,
		  dc->dc_MR.mr_CRTC[CR_FBBASE_23_18]);
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_31_24,
		  dc->dc_MR.mr_CRTC[CR_FBBASE_31_24]);

	/*  ...and horizontal pan...  */
	vregs->w.VGA_AR_Idx  = AR_HPAN;
	vregs->w.VGA_AR_ValW = dc->dc_MR.mr_ATTR[AR_HPAN];
	vregs->w.VGA_AR_Idx  = DEF2FIELD (AR_IDX, ACCESS, LOCK);

	/*  Values stable; set pageflip bit.  */
	IDXREG_W (vregs, VGA_CR, CR_FBBASE_23_18,
	          dc->dc_MR.mr_CRTC[CR_FBBASE_23_18]
	          | VAL2FIELD (CR_FBBASE_23_18, LOADADDR, TRUE));

	/*  Copy to globals  */
	ci->ci_DispConfig.dc_FBBase			 = dc->dc_FBBase;
	ci->ci_DispConfig.dc_FBULOffset			 = dc->dc_FBULOffset;
	ci->ci_DispConfig.dc_MR.mr_CRTC[CR_FBBASE_17_10] =
	 dc->dc_MR.mr_CRTC[CR_FBBASE_17_10];
	ci->ci_DispConfig.dc_MR.mr_CRTC[CR_FBBASE_9_2]   =
	 dc->dc_MR.mr_CRTC[CR_FBBASE_9_2];
	ci->ci_DispConfig.dc_MR.mr_CRTC[CR_FBBASE_31_24] =
	 dc->dc_MR.mr_CRTC[CR_FBBASE_31_24];
	ci->ci_DispConfig.dc_MR.mr_CRTC[CR_FBBASE_23_18] =
	 dc->dc_MR.mr_CRTC[CR_FBBASE_23_18];
	ci->ci_DispConfig.dc_MR.mr_ATTR[AR_HPAN] =
	 dc->dc_MR.mr_ATTR[AR_HPAN];
}


/*****************************************************************************
 * Miscellaneous useful routines.
 */
void
writepaletterange (
register union vgaregs	*vregs,
register const uint8	*colors,
int			first,
int			last
)
{
	vregs->w.VGA_DACWIdx = (uint8) first;
	
	for ( ;  first <= last;  colors += 3, first++) {
		vregs->w.VGA_DACVal = colors[0];
		vregs->w.VGA_DACVal = colors[1];
		vregs->w.VGA_DACVal = colors[2];
	}
}

void
writepaletteentry (
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

#if 0
static void
writeramppalette (register union vgaregs *vregs)
{
	register int	i;

	vregs->w.VGA_DACWIdx = 0;	/*  Start at zero  */
	for (i = 0;  i < 256;  i++) {
		vregs->w.VGA_DACVal = i;
		vregs->w.VGA_DACVal = i;
		vregs->w.VGA_DACVal = i;
	}
}
#endif

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
		CR_VTOTAL_11_8,
		CR_VDISPEND_11_8,
		CR_VSYNCSTART_11_8,
		CR_VBLANKSTART_11_8,
		CR_HTOTAL_8,
		CR_HBLANKEND_6,
	};
#define	NSYNCS		(sizeof (idxs) / sizeof (uint8))
	
	for (i = 0;  i < NSYNCS;  i++)
		(*(vuint16 *) addr = ((uint16) table[idxs[i]] << 8) | idxs[i]);
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
	uint32		row_bytes, pixel_bytes, limit_clock_lo;
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
//	if (h_sync_start < (h_display + 2))	h_sync_start = h_display + 2;
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
//	if (v_sync_start < (v_display + 1))	v_sync_start = v_display + 1;
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
	
	/*  I810 doesn't handle 32-bpp at all.  */
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
	int			i;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;

	ci->ci_Regs		= (I810Regs *) ci->ci_BaseAddr1;
	vregs = &ci->ci_Regs->VGARegs;
	ci->ci_MasterClockKHz	= MASTERCLK;

	/*
	 * Determine size and physical address of "framebuffer" RAM.
	 */
	ci->ci_MemSize = di->di_TakenSize + di->di_ExtraSize
	               + di->di_CacheSize;
dprintf (("+++ ci_MemSize: %ld (%ld taken + %ld extra + %ld cache)\n",
 ci->ci_MemSize, di->di_TakenSize, di->di_ExtraSize, di->di_CacheSize));
	if (!(di->di_TakenSize + di->di_ExtraSize)) {
		/*
		 * Can't live with only optional cache memory; the chip
		 * doesn't handle it well.
		 * FIXME: Someday, we may wish to update this driver to
		 * allocate system memory as needed.
		 */
		dprintf (("+++! No system RAM reserved for i810!  Aborting.\n"));
		return (B_ERROR);
	}

	/*
	 * Declare framebuffer to genpool module.
	 * (Jason estimates an average of 128 allocations per megabyte.
	 * Hence the weird MemSize>>13 down there.)
	 */
	gfbpi->pi_Pool_AID	= di->di_BaseAddr0ID;
	gfbpi->pi_Pool		= (void *) ci->ci_BaseAddr0;
	gfbpi->pi_Size		= ci->ci_MemSize;
	strcpy (gfbpi->pi_Name, "I810 Framebuffer");
	if ((ret = (ggpm->gpm_CreatePool) (gfbpi, ci->ci_MemSize >> 13, 0)) < 0)
	{
dprintf (("++! Failed to create genpool, retval = %ld (0x%08lx)\n", ret, ret));
		vid_uninit (di);
		return (ret);
	}
	ci->ci_PoolID = gfbpi->pi_PoolID;

	/*
	 * Allocate command buffers out of the pool.
	 * (Maybe one day I'll thunk out to allocgfxbuf().)
	 */
	ci->ci_CmdSize = VAL2FIELD (RINGBUF2, BUFBASE, 1);  /*  One 4K page  */
	ms.ms_MR.mr_Size	= ci->ci_CmdSize * 2;	/*  Gimme two  */
	ms.ms_MR.mr_Owner	= 0;
	ms.ms_PoolID		= ci->ci_PoolID;
	ms.ms_AddrCareBits	= (4 << 10) - 1;	/*  4K-aligned	*/
	ms.ms_AddrStateBits	= 0;
	ms.ms_AllocFlags	= 0;		/*  FIXME: alloc from end?  */
	if ((ret = (ggpm->gpm_AllocByMemSpec) (&ms)) < 0) {
dprintf (("++! Failed to allocate cmdbuf, retval = %ld (0x%08lx)\n", ret, ret));
		(ggpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
		vid_uninit (di);
		return (ret);
	}
	ci->ci_CmdBase = ms.ms_MR.mr_Offset;
	ci->ci_IRQCmdBase = ci->ci_CmdBase + ci->ci_CmdSize;
	cc->cc_CmdTail =
	cc->cc_IRQCmdTail = 0;
dprintf (("+++ Cmdbuf allocated at offset 0x%08lx\n", ci->ci_CmdBase));


	/*  Get physical address of cursor image buffer  */
	get_memory_map
	 ((void *) ci->ci_CursorBase, SIZEOF_CURSORPAGE, &entry, 1);
	di->di_CursorBase_DMA = (uint32) entry.address;

	/*  Set HW status page  */
//	ci->ci_HWStatusPage = (HWStatusPage *) di->di_HWStatusPage;
	ci->ci_Regs->HWS_PGA = di->di_HWStatusPage_DMA;
	
	/*  Set GTT base address  */
	get_memory_map
	 (di->di_GTT, di->di_GTTSize * sizeof (uint32), &entry, 1);
	ci->ci_Regs->PGTBL_CTL =
	 ((uint32) entry.address & MASKFIELD (PGTBL_CTL, GTTBASE)) |
	 VAL2FIELD (PGTBL_CTL, VALID, TRUE);

	if ((ret = initGTT (di, ci)) < 0)
		return (ret);


	/*  Set up correct value for GR06 (GR_MISC)  */
	gcr_table[GR_MISC] = IDXREG_R (vregs, VGA_GR, GR_MISC)
	                   & MASKFIELD (GR_MISC, MEMMAP)
	                   | gcr_table[GR_MISC];


	/*
	 * Set up LPRING to use command buffer.
	 */
	ci->ci_Regs->LPRING.RINGBUF0 =
	 VAL2FIELD (RINGBUF0, TAILOFFSET, cc->cc_CmdTail);
	ci->ci_Regs->LPRING.RINGBUF1 = VAL2FIELD (RINGBUF1, HEADOFFSET, 0);
	ci->ci_Regs->LPRING.RINGBUF2 = ci->ci_CmdBase;
	ci->ci_Regs->LPRING.RINGBUF3 =
	 ((ci->ci_CmdSize - VAL2FIELD (RINGBUF3, BUFSIZE, 1)) &
	  MASKFIELD (RINGBUF3, BUFSIZE)) |
	 DEF2FIELD (RINGBUF3, AUTOREPORTHEAD, DISABLE) |
	 DEF2FIELD (RINGBUF3, BUFFER, ENABLE);

#if 0
	/*
	 * Ensure IRQ ring is disabled.
	 */
	ci->ci_Regs->IRQRING.RINGBUF3 = DEF2FIELD (RINGBUF3, BUFFER, DISABLE);
#else
	/*
	 * Set up IRQRING likewise.
	 */
	ci->ci_Regs->IRQRING.RINGBUF0 =
	 VAL2FIELD (RINGBUF0, TAILOFFSET, cc->cc_IRQCmdTail);
	ci->ci_Regs->IRQRING.RINGBUF1 = VAL2FIELD (RINGBUF1, HEADOFFSET, 0);
	ci->ci_Regs->IRQRING.RINGBUF2 = ci->ci_IRQCmdBase;
	ci->ci_Regs->IRQRING.RINGBUF3 =
	 ((ci->ci_CmdSize - VAL2FIELD (RINGBUF3, BUFSIZE, 1)) &
	  MASKFIELD (RINGBUF3, BUFSIZE)) |
	 DEF2FIELD (RINGBUF3, AUTOREPORTHEAD, DISABLE) |
	 DEF2FIELD (RINGBUF3, BUFFER, ENABLE);
#endif


	/*
	 * Select I/O address 0x3D?, custom clock rate.  (Default settings
	 * from Intel.)
	 */
	ci->ci_DispConfig.dc_MR.mr_Misc = vregs->r.VGA_Misc
	                                & ~(MASKFIELD (MISC, VSYNCPOL)
	                                  | MASKFIELD (MISC, HSYNCPOL)
	                                  | MASKFIELD (MISC, ODDEVENPAGESEL)
	                                  | MASKFIELD (MISC, CLKSEL)
	                                  | MASKFIELD (MISC, IOADDR));
	ci->ci_DispConfig.dc_MR.mr_Misc |= DEF2FIELD (MISC, ODDEVENPAGESEL, 64K)
	                                 | DEF2FIELD (MISC, CLKSEL, EXTRA)
	                                 | DEF2FIELD (MISC, IOADDR, 3Dx);
	vregs->w.VGA_Misc = ci->ci_DispConfig.dc_MR.mr_Misc;

	/*  Enable write access to standard VGA CRTC registers 0-7  */
	vregs->w.VGA_CR_Idx = CR_VSYNCEND;
	vregs->w.VGA_CR_Val = SETVAL2FIELD (vregs->r.VGA_CR_Val,
					    CR_VSYNCEND, PROTECT, FALSE);

	vregs->w.VGA_DACMask = ~0;

	/*
	 * Turn on I810 native modes (oh, and disable 256K framebuffer wraps).
	 */
	ci->ci_Regs->PIXCONF			=
	ci->ci_DispConfig.dc_MR.mr_PIXCONF	=
		DEF2FIELD (PIXCONF, VGAWRAP, DISABLE) |
		DEF2FIELD (PIXCONF, NATIVEMODE, ENABLE) |
		DEF2FIELD (PIXCONF, PALETTEMODE, NORMAL) |
		DEF2FIELD (PIXCONF, OSCANCOLOR, DISABLE) |
		DEF2FIELD (PIXCONF, EXTENDEDREAD, DISABLE) |
		VAL2FIELD (PIXCONF, SHOWCURSOR, FALSE) |
		DEF2FIELD (PIXCONF, DACWIDTH, 8BPP) |
		DEF2FIELD (PIXCONF, DISPMODE, 8BPP) |
		DEF2FIELD (PIXCONF, VGADELAYS, DISABLE) |
		DEF2FIELD (PIXCONF, OVLGAMMA, DISABLE) |
		DEF2FIELD (PIXCONF, FBGAMMA, DISABLE);

	/*  Enable native Attribute and CRTC register extensions  */
	IDXREG_W (vregs, VGA_CR, CR_IOCTL,
		  DEF2FIELD (CR_IOCTL, AREXTENSIONS, ENABLE) |
		   DEF2FIELD (CR_IOCTL, CREXTENSIONS, ENABLE));

	/*  Initialize address mapping  */
	i = IDXREG_R (vregs, VGA_GR, GR_ADDRMAP)
	  & MASKFIELD (GR_ADDRMAP, _RESERVED);
	IDXREG_W (vregs, VGA_GR, GR_ADDRMAP,
	          i | DEF2FIELD (GR_ADDRMAP, PAGEMODE, VGABUF)
	            | DEF2FIELD (GR_ADDRMAP, MAPSEL, MEMMAP)
	            | DEF2FIELD (GR_ADDRMAP, PACKEDMODE, BUSED)
	            | DEF2FIELD (GR_ADDRMAP, LINEARMAP, ENABLE)
	            | DEF2FIELD (GR_ADDRMAP, PAGEMAP, DISABLE));
	IDXREG_W (vregs, VGA_GR, GR_PAGESEL, 0);

	/*  Initialize other display features  */
	ci->ci_Regs->CURSCTL = DEF2FIELD (CURSCTL, ORIGIN, NORMAL) |
			       DEF2FIELD (CURSCTL, MODE, 32x32x2_ANDXOR);
			
	/*  Turn on VBLank.  */
	ci->ci_Regs->DISPSTAS= SETVAL2FIELD (ci->ci_Regs->DISPSTAS,
					     DISPSTAS, REPORTVBLANK, TRUE);

	/*
	 * Write identification info.
	 */
	strcpy (ci->ci_ADI.name, "Intel I810-based board");
	strcpy (ci->ci_ADI.chipset, "I810");
	ci->ci_ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ci_ADI.memory = ci->ci_MemSize;
	ci->ci_ADI.version = B_ACCELERANT_VERSION;

	memcpy (ci->ci_DispConfig.dc_MR.mr_ATTR,
		attr_table_init,
		sizeof (ci->ci_DispConfig.dc_MR.mr_ATTR));

	return (calcmodes (di));	/*  FIXME: Move this up  */
}

static status_t
initGTT (struct devinfo *di, register struct gfx_card_info *ci)
{
	register int	i, size;
	physical_entry	mapentry;
	uint32		pbase;
	uint8		*vbase;

	/*
	 * Initialize GTT to describe "taken" and "extra" memory.
	 * First map "taken" memory.
	 */
	i = 0;
	for (pbase = di->di_TakenBase_DMA, size = di->di_TakenSize;
	     size > 0;
	     pbase += B_PAGE_SIZE, size -= B_PAGE_SIZE)
	{
		ci->ci_Regs->GTT[i++] =
		  (pbase & MASKFIELD (GTTENTRY, PAGEADDR))
		| DEF2FIELD (GTTENTRY, PAGELOC, SYSRAM)
		| VAL2FIELD (GTTENTRY, VALID, TRUE);
	}
	
	/*
	 * Iterate until we describe the entire physical space of the extra
	 * memory.  Note that the index 'i' continues to advance.
	 */
	vbase = (uint8 *) di->di_ExtraBase;
	size = di->di_ExtraSize;
	while (size > 0) {
		get_memory_map (vbase, size, &mapentry, 1);
		vbase += mapentry.size;	/*  Must do now;  */
		size -= mapentry.size;	/*  .size will get trashed  */

		pbase = (uint32) mapentry.address;
		while (mapentry.size > 0) {
			ci->ci_Regs->GTT[i++] =
			  (pbase & MASKFIELD (GTTENTRY, PAGEADDR))
			| DEF2FIELD (GTTENTRY, PAGELOC, SYSRAM)
			| VAL2FIELD (GTTENTRY, VALID, TRUE);
			pbase += B_PAGE_SIZE;
			mapentry.size -= B_PAGE_SIZE;
		}
	}
	
	/*
	 * Map optional local cache memory, if present.  Again, 'i' continues
	 * to advance.
	 */
	for (pbase = 0, size = di->di_CacheSize;
	     size > 0;
	     pbase += B_PAGE_SIZE, size -= B_PAGE_SIZE)
	{
		ci->ci_Regs->GTT[i++] =
		  (pbase & MASKFIELD (GTTENTRY, PAGEADDR))
		| DEF2FIELD (GTTENTRY, PAGELOC, CACHE)
		| VAL2FIELD (GTTENTRY, VALID, TRUE);
	}

	/*
	 * "Page faults" on this thing are unrecoverable, so point "empty"
	 * entries at first page of "taken" RAM.
	 */
	while (i < di->di_GTTSize) {
		ci->ci_Regs->GTT[i++] =
		  (di->di_TakenBase_DMA & MASKFIELD (GTTENTRY, PAGEADDR))
		| DEF2FIELD (GTTENTRY, PAGELOC, SYSRAM)
		| VAL2FIELD (GTTENTRY, VALID, TRUE);
	}
	return (B_OK);
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
	vregs = &ci->ci_Regs->VGARegs;

	/*  Turn off cursor; image RAM's about to vanish  */
	ci->ci_Regs->PIXCONF = SETVAL2FIELD (ci->ci_Regs->PIXCONF,
					     PIXCONF, SHOWCURSOR, FALSE);

	/*  Disable LPRING.  */
	ci->ci_Regs->LPRING.RINGBUF3 =
	 SETDEF2FIELD (ci->ci_Regs->LPRING.RINGBUF3,
		       RINGBUF3, BUFFER, DISABLE);
		
	/*  Reset to default pixel clock.  */
	ci->ci_DispConfig.dc_MR.mr_Misc =
	 SETDEF2FIELD (ci->ci_DispConfig.dc_MR.mr_Misc, MISC, CLKSEL, 25MHZ);
	vregs->w.VGA_Misc = ci->ci_DispConfig.dc_MR.mr_Misc;

	/*  Flip back to cheezy old VGA mode.  */
	tmp = IDXREG_R (vregs, VGA_GR, GR_ADDRMAP);
	IDXREG_W (vregs, VGA_GR, GR_ADDRMAP,
		  SETDEF2FIELD (tmp, GR_ADDRMAP, LINEARMAP, DISABLE));

	/*
	 * Turn off page table.
	 * FIXME: To do this right, we need to restore a proper legacy VGA
	 * state.  I don't fully do that here, so it's possible the card will
	 * immediately take a page fault after this operation, possibly
	 * locking things up terribly.
	 */
	ci->ci_Regs->PGTBL_CTL = SETVAL2FIELD (ci->ci_Regs->PGTBL_CTL,
					       PGTBL_CTL, VALID, FALSE);

	/*  Set HW Status page to powerup default  */
	ci->ci_Regs->HWS_PGA = 0x1FFFF000;	/*  Intel docs  */

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
