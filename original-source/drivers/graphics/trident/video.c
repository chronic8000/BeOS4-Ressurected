/* :ts=8 bk=0
 *
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * $Id:$
 *
 * Leo L. Schwab					1999.09.07
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <drivers/genpool_module.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <graphics_p/trident/trident.h>
#include <graphics_p/trident/tridentdefs.h>
#include <graphics_p/trident/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define MASTERCLK_NTSC	14318.18	/*  4 * NTSC colorburst  */
#define MASTERCLK_PAL	17734.48

#define	SIZEOF_CURSORPAGE	4096


#if 0
/*  Normal, once everything's working...  */
#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | \
			 B_PARALLEL_ACCESS | B_DPMS)
#else
#define	MODE_FLAGS	(B_SCROLL | B_HARDWARE_CURSOR | \
			 B_PARALLEL_ACCESS | B_DPMS)
#endif
#define	SYNC_HnVn	0
#define	SYNC_HnVP	B_POSITIVE_VSYNC
#define	SYNC_HPVn	B_POSITIVE_HSYNC
#define	SYNC_HPVP	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)


/*****************************************************************************
 * Local structures.
 */
struct watermark {
	uint16	wide, high;
	uint8	depth8, depth16, depth32;
};

struct hsyncadjust {
	uint16	wide, high;
	uint8	pixelbytes;
	uint8	adj60hz, adj75hz, adj85hz;
};


/*****************************************************************************
 * Local prototypes.
 */
static status_t calcmodes (struct devinfo *di);
static int testbuildmodes (struct devinfo *di, register display_mode *dst);
static void setdispoffset (register struct tri_card_info *ci,
			   register struct tri_card_ctl *cc,
			   register display_mode *dm);
static int usesyncshadows (register union vgaregs *vregs, display_mode *dm);
static int findwatermark (display_mode *dm);
static int findhsyncadjust (display_mode *dm);
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
extern genpool_module	*gpm;
extern BPoolInfo	*gfbpi;


/*
 * This table is formatted for 132 columns, so stretch that window...
 * This is the VESA standard sync table, except where noted.
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


static struct watermark watermarks[] = {
 {	 640,	 480,	0xA,	0x8,	0x4	}, 
 {	 800,	 600,	0x8,	0x7,	0x5	},
 {	1024,	 768,	0x8,	0x6,	0x4	},
 {	1152,	 864,	0x7,	0x4,	0x3	},	/*  A guess  */
 {	1280,	1024,	0x6,	0x3,	0x0	},
 {	1600,	1200,	0x6,	0x3,	0x0	}
};
#define	NWATERMARKS	(sizeof (watermarks) / sizeof (struct watermark))

/*
 * Some of these numbers are from Trident; others have been tweaked by me.
 * Trident's original numbers are:
 * Mode			3x4.04	3x5.05	Characters
 * 10x7x8@75Hz		85=>86	91=>92	1
 * 10x7x16@75Hz		85=>86	91=>92	1
 * 10x7x32@75Hz		84=>85	90=>91	1
 * 12x10x8@60Hz		A6=>A7	34=>35	1
 * 12x10x8@75Hz		A2=>A8	34=>3A	6
 * 12x10x8@85Hz    	A8=>AA	3C=>3E	2
 * 12x10x16@60Hz	A6=>A7	34=>35	1
 * 12x10x16@75Hz	A2=>A8	34=>3A	6
 * 12x10x16@85Hz	A8=>AA	3C=>3E	2
 */
static struct hsyncadjust adjusts[] = {
 {	1024,	 768,	1,	 8,	 8,	 8	},
 {	1024,	 768,	2,	 8,	16,	16	},
 {	1024,	 768,	4,	 8,	16,	16	},
 {	1152,	 864,	1,	 8,	 8,	24	},	// I made
 {	1152,	 864,	2,	 8,	16,	24	},	// these
 {	1152,	 864,	4,	 8,	48,	24	},	// up
 {	1280,	1024,	1,	 8,	48,	16	},
 {	1280,	1024,	2,	 8,	48,	16	},
 {	1280,	1024,	4,	 8,	48,	16	},
 {	1600,	1200,	1,	16,	40,	40	},	// I made
 {	1600,	1200,	2,	16,	40,	40	},	// these
 {	1600,	1200,	4,	16,	40,	40	},	// up
};
#define	NADJUSTS	(sizeof (adjusts) / sizeof (*adjusts))


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

/* Sequencer
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
trident_init (register struct devinfo *di)
{
	register tri_card_info	*ci;
	register vgaregs	*vregs;
	tri_card_ctl		*cc;
	BMemSpec		ms;
	status_t		ret = B_OK;
	uint8			tmp;

	ci = di->di_TriCardInfo;
	cc = di->di_TriCardCtl;

	ci->ci_Regs = (TridentRegs *) ci->ci_BaseAddr1;
	/*  CR_GRVIDENGINECTL_GEBASE_IO_21xx also mem-maps the same offset.  */
	ci->ci_DDDG = (DDDGraphicsEngineRegs *) (ci->ci_BaseAddr1 + 0x2100);
	ci->ci_DPMSMode = B_DPMS_ON;
	cc->cc_CursColorBlk = 0;
	cc->cc_CursColorWht = 0x00ffffff;

	vregs = &ci->ci_Regs->loregs.vga;

	/*
	 * Unlock a buttload of registers.
	 * The chip only responds to I/O transactions at first.
	 */
	/*  Enable write access to standard VGA CRTC registers 0-7  */
	tmp = IDXREG_IOR (vregs, VGA_CR, CR_VSYNCEND);
	outb (offsetof (vgaregs, w.VGA_CR_Val),
	      tmp & ~MASKEXPAND (CR_VSYNCEND_PROTECT));
	
	/*  Switch chip to "new" register definitions.  */
	tmp = IDXREG_IOR (vregs, VGA_SR, SR_OLDNEWCTL);
	
	/*  Write-enable config ports.  */
	tmp = IDXREG_IOR (vregs, VGA_SR, SR_MODECTL1NEW) |
	      FIELDDEF (SR_MODECTL1NEW, CFGPORT, WRITEABLE);
	IDXREG_IOW (vregs, VGA_SR, SR_MODECTL1NEW, tmp);
	
	/*  Unprotect more extended registers  */
	IDXREG_IOW (vregs, VGA_SR, SR_PROTECT, SR_PROTECT_ACCESS_ALL);

	/*  Prepare to switch over to memory-mapped I/O.  */
	tmp = IDXREG_IOR (vregs, VGA_CR, CR_PCICTL) |
	      FIELDDEF (CR_PCICTL, MMIO, ENABLE);
	IDXREG_IOW (vregs, VGA_CR, CR_PCICTL, tmp);
	
	/*
	 * Chip is now speaking memory-mapped I/O, old-style I/O accesses
	 * are now *disabled*.
	 */
	/*  Enable graphics engine registers  */
	tmp = IDXREG_R (vregs, VGA_CR, CR_DACMODE) |
	      FIELDDEF (CR_DACMODE, GEIODECODE, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_DACMODE, tmp);

	/*
	 * Configure some more useful stuff.
	 */
	cc->cc_CRTC[CR_MODULETEST] =
	 FIELDDEF (CR_MODULETEST, MEMACCESS, ALL) |
	 FIELDDEF (CR_MODULETEST, MISCREGPROTECT, RW) |
	 FIELDDEF (CR_MODULETEST, INTERLACE, DISABLE) |
	 FIELDVAL (CR_MODULETEST, ZERO, 0);
	IDXREG_W (vregs, VGA_CR, CR_MODULETEST, cc->cc_CRTC[CR_MODULETEST]);

	tmp = IDXREG_R (vregs, VGA_CR, CR_GRVIDENGINECTL);
	tmp = tmp & ~(FIELDMASK (CR_GRVIDENGINECTL, GEBASE) |
		      FIELDMASK (CR_GRVIDENGINECTL, GEIO) |
		      FIELDMASK (CR_GRVIDENGINECTL, GE));
	tmp |= FIELDDEF (CR_GRVIDENGINECTL, GEBASE, IO_21xx) |
	       FIELDDEF (CR_GRVIDENGINECTL, GEIO, DISABLE) |
	       FIELDDEF (CR_GRVIDENGINECTL, GE, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_GRVIDENGINECTL, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_PHYSADDRCTL) |
#if (DEBUG  ||  FORCE_DEBUG)
	      FIELDDEF (CR_PHYSADDRCTL, BOTHIO, ENABLE) |
#endif
	      FIELDDEF (CR_PHYSADDRCTL, MAKEMEMLINEAR, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_PHYSADDRCTL, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_ADDRMODE) |
	      FIELDDEF (CR_ADDRMODE, LINEAR, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_ADDRMODE, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_IFSEL) |
	      FIELDDEF (CR_IFSEL, INTERNALWIDTH, 32);
	IDXREG_W (vregs, VGA_CR, CR_IFSEL, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_PCICTL) |
	      FIELDDEF (CR_PCICTL, BURSTREAD, DISABLE) |
	      FIELDDEF (CR_PCICTL, BURSTWRITE, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_PCICTL, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_PERFTUNE) |
	      FIELDDEF (CR_PERFTUNE, BLANKSRC, INVDISP) |
	      FIELDDEF (CR_PERFTUNE, DISPFIFODEPTH, 64);
	IDXREG_W (vregs, VGA_CR, CR_PERFTUNE, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_CURSCTL) &
	      ~(FIELDMASK (CR_CURSCTL, CURSOR) |
	        FIELDMASK (CR_CURSCTL, TYPE) |
	        FIELDMASK (CR_CURSCTL, SIZE));
	tmp |= FIELDDEF (CR_CURSCTL, CURSOR, DISABLE) |
	       FIELDDEF (CR_CURSCTL, TYPE, WINDOZE) |
	       FIELDDEF (CR_CURSCTL, SIZE, 32x32);
	IDXREG_W (vregs, VGA_CR, CR_CURSCTL, tmp);

	tmp = IDXREG_R (vregs, VGA_CR, CR_ENH0) |
	      FIELDDEF (CR_ENH0, PCIGERETRY, ENABLE);
	IDXREG_W (vregs, VGA_CR, CR_ENH0, tmp);

	tmp = IDXREG_R (vregs, VGA_GR, GR_MISCINTERNALCTL) |
	      FIELDDEF (GR_MISCINTERNALCTL, DISPFIFOTHRESH, ENABLE);
	IDXREG_W (vregs, VGA_GR, GR_MISCINTERNALCTL, tmp);

	/*
	 * Work out framebuffer size.
	 */
	ci->ci_MasterClockKHz	= MASTERCLK_NTSC;
	ci->ci_MemSize		= 1 << 20;
	switch (IDXREG_R (vregs, VGA_CR, CR_SWPROGRAMMING) &
	        FIELDMASK (CR_SWPROGRAMMING, MEMSIZE))
	{
	case FIELDDEF (CR_SWPROGRAMMING, MEMSIZE, 8M):
		ci->ci_MemSize <<= 1;
	case FIELDDEF (CR_SWPROGRAMMING, MEMSIZE, 4M):
		ci->ci_MemSize <<= 1;
	case FIELDDEF (CR_SWPROGRAMMING, MEMSIZE, 2M):
		ci->ci_MemSize <<= 1;
	case FIELDDEF (CR_SWPROGRAMMING, MEMSIZE, 1M):
		break;
	default:
		dprintf (("+++ UNKNOWN MEMSIZE!!\n"));
	}
dprintf (("+++ ci_MemSize: %d\n", ci->ci_MemSize));

	/*
	 * Declare framebuffer to genpool module.
	 * (Jason estimates an average of 128 allocations per megabyte.
	 * Hence the weird MemSize>>13 down there.)
	 */
	gfbpi->pi_Pool_AID	= di->di_BaseAddr0ID;
	gfbpi->pi_Pool		= (void *) ci->ci_BaseAddr0;
	gfbpi->pi_Size		= ci->ci_MemSize;
	strcpy (gfbpi->pi_Name, "Trident Framebuffer");
	if ((ret = (gpm->gpm_CreatePool) (gfbpi, ci->ci_MemSize >> 13, 0)) < 0)
	{
dprintf (("++! Failed to create genpool, retval = %d (0x%08lx)\n", ret, ret));
		return (ret);
	}
	ci->ci_PoolID = gfbpi->pi_PoolID;

	/*
	 * Allocate cursor out of the pool.
	 */
	ms.ms_MR.mr_Size	= SIZEOF_CURSORPAGE;
	ms.ms_MR.mr_Owner	= 0;
	ms.ms_PoolID		= ci->ci_PoolID;
	ms.ms_AddrCareBits	= (1 << 10) - 1;	/*  1K-aligned	*/
	ms.ms_AddrStateBits	= 0;
	ms.ms_AllocFlags	= 0;
	if ((ret = (gpm->gpm_AllocByMemSpec) (&ms)) < 0) {
dprintf (("++! Failed to allocate cursor, retval = %d (0x%08lx)\n", ret, ret));
		(gpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
		return (ret);
	}
	ci->ci_CursorBase = ms.ms_MR.mr_Offset;
dprintf (("+++ Cursor allocated at offset 0x%08lx\n", ci->ci_CursorBase));


	/*
	 * Select I/O address 0x3D?, enable archaic video RAM access, custom
	 * clock rate.
	 */
	vregs->w.VGA_Misc = FIELDDEF (MISC, CLKSEL, EXTRA) |
			    FIELDDEF (MISC, 0xA0000WINDOW, ENABLE) |
			    FIELDDEF (MISC, IOADDR, 3Dx);
	vregs->w.VGA_DACMask = ~0;



	strcpy (ci->ci_ADI.name, "Trident-based controller");
	strcpy (ci->ci_ADI.chipset, "Trident");
	ci->ci_ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ci_ADI.memory = ci->ci_MemSize;
	ci->ci_ADI.version = B_ACCELERANT_VERSION;

	memcpy (cc->cc_ATTR, attr_table_init, sizeof (cc->cc_ATTR));

// #if FORCE_CLEAR_SCREEN
	/*  Erase crud from previous boot cycle.  */
//	memset (ci->ci_BaseAddr0, 0, ci->ci_MemSize);
// #endif

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
		{ B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE };
#else
	static color_space spaces[] =
		{ B_CMAP8, B_RGB15_BIG, B_RGB16_BIG, B_RGB32_BIG };
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
			int	adjust;

			try = *src;
			try.space = low.space = high.space = spaces[j];

			/*
			 * Due to a bug in the Trident chip(s), the front
			 * porch needs to be a certain length, or display
			 * noise will result.  The HSYNC values need to be
			 * delayed by some number of character clocks for some
			 * modes to get the needed length.  We add the delay
			 * here because it varies according to refresh rate
			 * and bit depth.
			 */
			adjust = findhsyncadjust (&try);
			low.timing.h_sync_start  =
			high.timing.h_sync_start =
			(try.timing.h_sync_start += adjust);
			low.timing.h_sync_end  =
			high.timing.h_sync_end =
			(try.timing.h_sync_end += adjust);

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
	uint32		row_bytes, row_pixels, pixel_bytes;
	uint32		limit_clock_hi, limit_clock_lo;
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
	    di->di_TriCardInfo->ci_MemSize)
		target->virtual_height =
		 di->di_TriCardInfo->ci_MemSize / row_bytes;

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
	limit_clock_hi = CLOCK_MAX;
	limit_clock_lo = 48.0 *	/*  Until a monitors database does it...  */
			 ((double) target->timing.h_total) *
			 ((double) target->timing.v_total) /
			 1000.0;
	if (pixel_bytes != 1) {
		/*
		 * 15, 16, and 32 bpp require a doubled plxel clock.  However,
		 * we can't report that in .pixel_clock (because the $&^@!
		 * app_server/Screen prefs uses it to calculate vertical
		 * refresh rate), so we adjust the world around it.
		 */
		limit_clock_hi >>= 1;
		limit_clock_lo >>= 1;
	}
	if (limit_clock_lo > limit_clock_hi)
		result = B_ERROR;
	if (target->timing.pixel_clock > limit_clock_hi)
		target->timing.pixel_clock = limit_clock_hi;
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



#if STEAL_UNUSED_FB_RAM

extern int return_bios_frames(char *paddr, uint32 size, int slow);
extern unsigned int	memsize;

static bool
test_memory(void* base, uint32 len)
{
	uint32	i;
	volatile uint8*	p = (uint8*) base;
	uint32	d = 1;
	bool ret = TRUE;
	
	char	m[80];
	
	dprintf(("Testing 0x%08lx bytes @ %p\n", len, base));
	
	for(i=0; i<len; i++)
		p[i] = (uint8)(i*13);

	snooze(60*1000);

	for(i=0; i<len; i += d)
		if(p[i] != (uint8)(i*13))
		{
			sprintf	(m, "Mistmatch @ %p: 0x%02x, expected 0x%02x\n", &p[i], p[i], (uint8)(i*13));
			kernel_debugger(m);
			ret = FALSE;
			d *= 2;
		}
	return ret;
}


status_t
steal_unused_frame_buffer_ram(void)
{
	uint32			vendor_device_id;
	area_id			phys_ram;
	area_info		ar_info;
	uint8*			addr0;
	status_t		ret;

	if((phys_ram = find_area("physical_ram")) < 0)
	{
		dprintf(("Can't find the the area for physical RAM\n"));
		return phys_ram;
	}
	ret = get_area_info(phys_ram, &ar_info);
	if(ret != B_OK)
		return ret;
	addr0 = ar_info.address;
	
	/* check for VIA MVP4 */
	vendor_device_id = read_pci_config(0, 0, 0, 0, 4);
	if(vendor_device_id == 0x05011106)
	{
		/* steal memory from the 2 MB frame buffer: only 2*800*600 bytes is used */ 
		uint32	fba, fbs, base, len;
		uint16	tmp;
		uint8	fbsb;
		const uint32	used_fbs = (((2*800*600) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1))+B_PAGE_SIZE;
		
		tmp = pci_bus->read_pci_config(0, 0, 0, 0xFA, 2);
		if(tmp & 0x0800)	/* CPU direct access to the frame buffer is already enabled */
			return B_BUSY;
		
		fba = memsize;
		fbsb = (tmp >> 12) & 3;
		if(fbsb == 0)	/* buffer size is 0 */
			return B_ERROR;
		fbs = 0x100000 << fbsb;
		
		if(fbs == 0x200000)	
			fba -= fbs; /* memsize is rounded up to 4MB */
		
		pci_bus->write_pci_config(0, 0, 0, 0xFA, 2,
			(tmp & 0x8000)	|	/* preserve VGA enable bit */
			0x1000 			|	/* 2MB frame buffer */
			0x0800			|	/* enable CPU direct access to the frame buffer  */
			(uint16)(fba >> 21)	/* CPU address for the frame buffer */
			);
		//dprintf("fba = 0x%08lx, FBFA reg = 0x%04lx\n", fba, read_pci_config(0, 0, 0, 0xFA, 2)); 
/*
		tmp = read_pci_config(0, 0, 0, 0x63, 1);
		tmp &= 0xFC;
		tmp |= 1; 
		write_pci_config(0, 0, 0, 0x63, 1, tmp);
*/

		base = fba + used_fbs;
		len = (1024*1024 - used_fbs) + 0xB5000;	/* last ~307 kB are unusable. Why???? */
		//base = fba + 1024*1024;
		//len = 0x6A000;
		if(test_memory(addr0+base, len))
		{
			ret = return_bios_frames((char*)base, len, 1);
		}
		else
		{
			dprintf(("Frame buffer memory test failed.\n"));
			return B_ERROR; 
		}
		
		if(ret != B_OK)
			return ret;
		
		/*  reuse VGA memory*/
		tmp = pci_bus->read_pci_config(0, 0, 0, 0x63, 1);
		tmp &= ~3;
		tmp |= 1;
		pci_bus->write_pci_config(0, 0, 0, 0x63, 1, tmp);
		base = 0xA0000;
		len = 128*1024;
		if(test_memory(addr0+base, len))
		{
			return return_bios_frames((char*)base, len, 1);
		}
		else
		{
			dprintf(("VGA memory test failed.\n"));
			return B_ERROR; 
		}
	}
	return ENODEV;
}
#endif



/*****************************************************************************
 * Display manipulation.
 ****
 * NOTE: It is extremely important that the accelerant re-initialize
 * its acceleration state after reconfiguring the display mode.
 */
status_t
SetupCRTC (
register struct devinfo		*di,
register struct tri_card_info	*ci,
register display_mode		*dm
)
{
	register vgaregs	*vregs;
	tri_card_ctl		*cc;
	BMemSpec		oldalloc;
	float			clockin, clockout;
	status_t		retval;
	int			m, n, k;
	int			hdisp, hsyncstart, hsyncend, htotal;
	int			vdisp, vsyncstart, vsyncend, vtotal;
	int			htotalpix, vtotalpix;
	int			depth, bytespp, bytesperrow;
	uint8			grmisc, pix, daccmd;
	uint8			tmp;

	cc = di->di_TriCardCtl;
	vregs = &ci->ci_Regs->loregs.vga;

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

	depth		= colorspacebits (dm->space);
	bytespp		= (depth + 7) >> 3;
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
	if (depth != 8)	/*  15, 16, and 32  */
		clockin *= 2;
	if (!ClockSelect (ci->ci_MasterClockKHz, clockin, &clockout,
			  &m, &n, &k))
	{
		dprintf (("Trident: Can't compute (M,N,K) triplet: %fKHz\n",
			  clockin));
		return (B_ERROR);
	}

	/*
	 * Get memory for the new framebuffer.
	 */
	/*  Save previous allocation.  */
	memcpy (&oldalloc, &ci->ci_FBAlloc, sizeof (oldalloc));

	/*  Free existing framebuffer.  */
	if (ci->ci_FBAlloc.ms_MR.mr_Size) {
		if ((retval = (gpm->gpm_FreeByMemSpec) (&ci->ci_FBAlloc)) < 0)
			return (retval);
		ci->ci_FBAlloc.ms_MR.mr_Size = 0;
	}

	/*  Get the new one.  */
	ci->ci_FBAlloc.ms_MR.mr_Size	= dm->virtual_width * bytespp *
					  dm->virtual_height;
	ci->ci_FBAlloc.ms_MR.mr_Owner	= 0;
	ci->ci_FBAlloc.ms_PoolID	= ci->ci_PoolID;
	ci->ci_FBAlloc.ms_AddrCareBits	= 3;	/*  32-bit-aligned  */
	ci->ci_FBAlloc.ms_AddrStateBits	= 0;
	ci->ci_FBAlloc.ms_AllocFlags	= 0;
	if ((retval = (gpm->gpm_AllocByMemSpec) (&ci->ci_FBAlloc)) < 0) {
dprintf (("+++!!! Display buffer allocation failed: %d (0x%08lx)\n", retval, retval));
		/*  CRAP!  Try and restore previous allocation.  */
		memcpy (&ci->ci_FBAlloc, &oldalloc, sizeof (oldalloc));
		if ((retval = (gpm->gpm_AllocByMemSpec) (&ci->ci_FBAlloc)) < 0)
		{
			/*  Well, I don't know what else I can do...  */
dprintf (("+++!!! YIKES!  Can't restore framebuffer allocation!!  retval = %d\n",
 retval));
		}
		return (retval);
	}
	ci->ci_FBBase = ci->ci_FBAlloc.ms_MR.mr_Offset;
dprintf (("+++ ci->ci_FBBase allocated at offset 0x%08lx\n", ci->ci_FBBase));

	ci->ci_BytesPerRow = dm->virtual_width * bytespp;
	ci->ci_Depth = depth;
dprintf (("+++ Requested dimensions: %dx%dx%d\n",
	  dm->timing.h_display, dm->timing.v_display, ci->ci_Depth));

	/*
	 * CRTC Controller
	 * Not fully symbolized, but tridentdefs.h should provide some clues.
	 * Hardware-dependent fenceposts are added (or subtracted) here.
	 */     
	cc->cc_CRTC[CR_HTOTAL_7_0]	= Set8Bits (htotal - 5);
	cc->cc_CRTC[CR_HDISPEND_7_0]	= Set8Bits (hdisp - 1);
	cc->cc_CRTC[CR_HBLANKSTART_7_0]	= Set8Bits (hdisp);	// No border
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
	cc->cc_CRTC[CR_VBLANKSTART_7_0]	= Set8Bits (vdisp);	// No border
	cc->cc_CRTC[CR_VBLANKEND_7_0]	= Set8Bits (vtotal - 1);// No border

	/*  -- Non-standard regs --  */
	cc->cc_CRTC[CR_HSYNCEXTRA]	= SetBitField (hdisp, 8:8, 4:4) |
					  SetBitField (hsyncstart, 8:8, 3:3) |
					  SetBitField (hdisp - 1, 8:8, 1:1) |
					  SetBitField (htotal - 5, 8:8, 0:0);
	cc->cc_CRTC[CR_VSYNCEXTRA]	= SetBitField (vtotal - 2, 10:10, 7:7) |
					  SetBitField (vdisp, 10:10, 6:6) |
					  SetBitField (vsyncstart, 10:10, 5:5) |
					  SetBitField (vdisp - 1, 10:10, 4:4) |
					  SetBit (3);

	cc->cc_CRTC[CR_DACMODE]		= IDXREG_R (vregs, VGA_CR, CR_DACMODE) &
					  ~(FIELDMASK (CR_DACMODE, PITCH_9_8));
	cc->cc_CRTC[CR_DACMODE]		|= SetBitField (bytesperrow, 9:8,
							CR_DACMODE_PITCH_9_8);

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
	IDXREG_W (vregs, VGA_SR, SR_VCLK0CTL0, Set8Bits (n));
	IDXREG_W (vregs, VGA_SR, SR_VCLK0CTL1,
		  FIELDVAL (SR_VCLKxCTL1, K, k) |
		   FIELDVAL (SR_VCLKxCTL1, M, m));

	/*  Extended configuration registers  */
	pix = 0;
	daccmd = 0;
	grmisc = IDXREG_R (vregs, VGA_GR, GR_MISCCTL) &
		 ~(FIELDMASK (GR_MISCCTL, VCLKDIV1) |
		   FIELDMASK (GR_MISCCTL, VCLKDIV0));
	grmisc |= FIELDDEF (GR_MISCCTL, ZCHAIN4CPU, ENABLE) |
		  FIELDDEF (GR_MISCCTL, ZCHAIN4DISP, ENABLE);
	switch (ci->ci_Depth) {
	case 8:
		grmisc |= FIELDDEF (GR_MISCCTL, VCLKDIVx, DIV1);
		daccmd = FIELDDEF (DACCMD, COLORMODE, 8_CLUT);
		break;
	case 15:
		pix |= FIELDDEF (CR_PIXBUSMODE, HIGHCOLOR, ENABLE);
		grmisc |= FIELDDEF (GR_MISCCTL, VCLKDIVx, DIV2);
		daccmd = FIELDDEF (DACCMD, COLORMODE, 15_RGB);
		break;
	case 16:
		pix |= FIELDDEF (CR_PIXBUSMODE, HIGHCOLOR, ENABLE);
		grmisc |= FIELDDEF (GR_MISCCTL, VCLKDIVx, DIV2);
		daccmd = FIELDDEF (DACCMD, COLORMODE, 16_RGB);
		break;
	case 32:
		pix |= FIELDDEF (CR_PIXBUSMODE, TRUECOLOR, ENABLE) |
		       FIELDDEF (CR_PIXBUSMODE, 16BITBUS, ENABLE);
		grmisc |= FIELDDEF (GR_MISCCTL, VCLKDIVx, DIV2);
		daccmd = FIELDDEF (DACCMD, COLORMODE, 24_RGB);
		break;
	}
	IDXREG_W (vregs, VGA_CR, CR_PIXBUSMODE, pix);
	IDXREG_W (vregs, VGA_GR, GR_MISCCTL, grmisc);
	/*  Get at DAC Command register; read DACMask four times.  */
	tmp = vregs->r.VGA_DACWIdx;
	tmp = vregs->r.VGA_DACMask; tmp = vregs->r.VGA_DACMask;
	tmp = vregs->r.VGA_DACMask; tmp = vregs->r.VGA_DACMask;
	vregs->w.VGA_DACMask = daccmd;	/*  Finally...  */
	tmp = vregs->r.VGA_DACWIdx;

	/*  Misc output register  */
	vregs->w.VGA_Misc = cc->cc_RegMisc;

	/*  Sequencer registers  */
	write_table (&vregs->w.VGA_SR_Idx, seq_table,
		     SR_CLOCK, SR_MODE);
	tmp = IDXREG_R (vregs, VGA_SR, SR_MODECTL2NEW) &
	      ~FIELDMASK (SR_MODECTL2NEW, FIFOTHRESH);
	IDXREG_W (vregs,
		  VGA_SR,
		  SR_MODECTL2NEW,
		  tmp | FIELDVAL (SR_MODECTL2NEW,
				  FIFOTHRESH,
				  findwatermark (dm)));

	/*  Graphics controller registers  */
	write_table (&vregs->w.VGA_GR_Idx, gcr_table,
		     GR_SETRESET, GR_LATCHMASK);
	tmp = IDXREG_R (vregs, VGA_GR, GR_LCDCTL) &
	      ~(FIELDMASK (GR_LCDCTL, VSYNCSHADOW) |
	        FIELDMASK (GR_LCDCTL, HSYNCSHADOW) |
	        FIELDMASK (GR_LCDCTL, SIGNALDELAY));
	if (usesyncshadows (vregs, dm))
		/*  SIGNALDELAY settings pulled from VESA mode sets.  */
		tmp |= FIELDDEF (GR_LCDCTL, VSYNCSHADOW, ENABLE) |
		       FIELDDEF (GR_LCDCTL, HSYNCSHADOW, ENABLE) |
		       FIELDVAL (GR_LCDCTL, SIGNALDELAY, -(1) + 2);
	else
		tmp |= FIELDDEF (GR_LCDCTL, VSYNCSHADOW, DISABLE) |
		       FIELDDEF (GR_LCDCTL, HSYNCSHADOW, DISABLE) |
		       FIELDVAL (GR_LCDCTL, SIGNALDELAY, -(0) + 2);
	IDXREG_W (vregs, VGA_GR, GR_LCDCTL, tmp);

	/*  CRTC registers are loaded in several disparate "blocks."  */
	write_table (&vregs->w.VGA_CR_Idx, cc->cc_CRTC,
		     CR_HTOTAL_7_0, CR_LINECMP_7_0);
	IDXREG_W (vregs, VGA_CR, CR_MODULETEST, cc->cc_CRTC[CR_MODULETEST]);
	IDXREG_W (vregs, VGA_CR, CR_DACMODE, cc->cc_CRTC[CR_DACMODE]);
	IDXREG_W (vregs, VGA_CR, CR_HSYNCEXTRA, cc->cc_CRTC[CR_HSYNCEXTRA]);
	IDXREG_W (vregs, VGA_CR, CR_VSYNCEXTRA, cc->cc_CRTC[CR_VSYNCEXTRA]);

	/*  Attribute registers  */
	n = vregs->r.VGA_ST01;
	for (n = 0;  n < NREGS_ATTR;  n++)	/*  Resuing 'n'  */
	{
		vregs->w.VGA_AR_Idx = n;
		vregs->w.VGA_AR_ValW = cc->cc_ATTR[n];
	}
	vregs->w.VGA_AR_Idx = FIELDDEF (AR_IDX, ACCESS, LOCK);


	snooze (1000);
	screen_on (vregs);
#if FORCE_CLEAR_SCREEN
	/*  Erase crud from previous mode.  */
	memset (ci->ci_BaseAddr0 + ci->ci_FBBase,
		0,
		ci->ci_FBAlloc.ms_MR.mr_Size);
#endif

#if STEAL_UNUSED_FB_RAM
	steal_unused_frame_buffer_ram();
#endif

	return (B_OK);
}

static void
setdispoffset (
register struct tri_card_info	*ci,
register struct tri_card_ctl	*cc,
register display_mode		*dm
)
{
	int	bytespp;
	uint32	addr;

	bytespp = (ci->ci_Depth + 7) >> 3;
	addr = ci->ci_FBBase +
	       (dm->v_display_start * dm->virtual_width +
	        dm->h_display_start) * bytespp;
//	cc->cc_ATTR[AR_HPAN] = (addr & 3) << 1;	// Pixel panning

	addr >>= 2;	// 32-bit quantization
	cc->cc_CRTC[CR_FBBASE_9_2]	= Set8Bits (addr);
	cc->cc_CRTC[CR_FBBASE_17_10]	= SetBitField (addr, 15:8, 7:0);
	cc->cc_CRTC[CR_MODULETEST]	&= ~FIELDMASK
					     (CR_MODULETEST, FBBASE_16);
	cc->cc_CRTC[CR_MODULETEST]	|= SetBitField
					    (addr,
					     16:16,
					     CR_MODULETEST_FBBASE_16);
	cc->cc_CRTC[CR_VSYNCEXTRA]	&= ~FIELDMASK
					     (CR_VSYNCEXTRA, FBBASE_19_17);
	cc->cc_CRTC[CR_VSYNCEXTRA]	|= SetBitField
					    (addr,
					     19:17,
					     CR_VSYNCEXTRA_FBBASE_19_17);
}

static int
usesyncshadows (register union vgaregs *vregs, display_mode *dm)
{
	/*
	 * Returns TRUE if there is a flat panel AND the proposed visible
	 * display size does not match the panel size.
	 */
	static struct {
		uint16	x, y;
	} paneldims[] = {
		/*  Direct mapping of the GR_LCDVDISPCTL register bits.  */
		{ 1280, 1024	},
		{  640,  480	},
		{ 1024,  768	},
		{  800,  600	}
	};
	int	i;

	/*
	 * I'm pulling this out of my posterior, but it seems to me that,
	 * if none of the LCD panel interface signals are active, then it's
	 * probably a fairly safe bet that there's no LCD panel on this
	 * thing.
	 * (I also used to check for LCD power sequencing (GR_PWRSTATUS_LCD)
	 * but it turns out some BIOSen don't necessarily set it.)
	 */
	if (IDXREG_R (vregs, VGA_GR, GR_SOFTPWRCTL) &
	     (FIELDMASK (GR_SOFTPWRCTL, PANELLIGHT) |
	      FIELDMASK (GR_SOFTPWRCTL, PANELVDD) |
	      FIELDMASK (GR_SOFTPWRCTL, PANELIF) |
	      FIELDMASK (GR_SOFTPWRCTL, PANELVEE)))
	{
		i = IDXREG_R (vregs, VGA_GR, GR_LCDVDISPCTL) &
		    FIELDMASK (GR_LCDVDISPCTL, PANELSIZE);
		i >>= 4;	/*  I need a new macro for this stuff...  */
		return (dm->timing.h_display != paneldims[i].x  ||
		        dm->timing.v_display != paneldims[i].y);
	} else {
		return (FALSE);
	}
}

static int
findwatermark (display_mode *dm)
{
	register int		i;
	struct watermark	*wm;
	uint8			deep;
	
	deep = colorspacebits (dm->space);
	for (wm = watermarks, i = NWATERMARKS;  --i >= 0;  wm++) {
		if (dm->timing.h_display == wm->wide  &&
		    dm->timing.v_display == wm->high)
		{
			switch (deep) {
			case 8:
				return (wm->depth8);
			case 15:
			case 16:
				return (wm->depth16);
			case 32:
				return (wm->depth32);
			}
		}
	}
	return (0);
}

static int
findhsyncadjust (display_mode *dm)
{
	register int		i;
	struct hsyncadjust	*hsa;
	int32			rate;
	uint8			bytes;

	rate = dm->timing.pixel_clock * 1000 /
	       (dm->timing.h_total * dm->timing.v_total);
	bytes = (colorspacebits (dm->space) + 7) >> 3;
	for (hsa = adjusts, i = NADJUSTS;  --i >= 0;  hsa++) {
		if (dm->timing.h_display == hsa->wide  &&
		    dm->timing.v_display == hsa->high  &&
		    bytes == hsa->pixelbytes)
		{
			if (rate <= 67)
				return (hsa->adj60hz);
			else if (rate > 67  &&  rate <= 80)
				return (hsa->adj75hz);
			else if (rate > 80)
				return (hsa->adj85hz);
		}
	}
	return (0);
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

	/*  FIXME: If we ever figure out an 8-bit DAC value...  */
	vregs->w.VGA_DACVal = r >> 2;
	vregs->w.VGA_DACVal = g >> 2;
	vregs->w.VGA_DACVal = b >> 2;
}

static void
setramppalette (register union vgaregs *vregs)
{
	register int	i;

	vregs->w.VGA_DACWIdx = 0;	/*  Start at zero  */
	for (i = 0;  i < 256;  i++) {
		vregs->w.VGA_DACVal = i >> 2;
		vregs->w.VGA_DACVal = i >> 2;
		vregs->w.VGA_DACVal = i >> 2;
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



static int
ClockSelect (
float	masterClock,
float	clockReq,
float	*clockOut,
int	*mOut,
int	*nOut,
int	*kOut
)
{
	int	m, n, k;
	float	bestDiff = 1e12;
	float	target = 0.0;
	float	best = 0.0;
	float	diff;
	int	startn, endn;
	int	endm, endk;

	*clockOut = 0.0;
	if (clockReq < CLOCK_MIN  ||  clockReq > CLOCK_MAX)
		dprintf (("Requested clock rate %f KHz out of range; trying anyway...",
			  clockReq));

	/*
	 * The formula for calculating PLL clocking triplets is as follows:
	 *
	 * F<out> = F<in> * (M + 8) / ((N + 2) * 2**K)
	 *
	 * There is an intermediate restriction (which we're ignoring):
	 *
	 * 349  <  (n + 8) * 100 / (m + 2)  <  978
	 */
//	if (NewClockCode) {
		startn = 24;	// 64;
		endn = 255;
		endm = 63;
		endk = 3;
//	} else {
//		startn = 0;
//		endn = 121;
//		endm = 31;
//		endk = 1;
//	}

//	if ((vgaBitsPerPixel == 16) && (TVGAchipset <= CYBER9320))
//		clockReq *= 2; 
//	if ((TVGAchipset < TGUI96xx) && (vgaBitsPerPixel == 24))
//		clockReq *= 3;
//	if (vgaBitsPerPixel == 32)
//		clockReq *= 2;

	for (k = 0;  k <= endk;  k++) {
		for (m = 1;  m <= endm;  m++) {
			float	fm = m;
			float	f2expk = (1 << k);
			
			n = (int) (clockReq * (fm + 2) * f2expk /
				   masterClock + 0.5) - 8;
			if (n < startn  ||  n > endn)
				continue;
			
			target = ((float) n + 8) * masterClock /
			         ((fm + 2) * f2expk);
			if (target < CLOCK_MIN  ||  target > CLOCK_MAX)
				continue;
				
/*
 * It seems that the 9440 docs have this STRICT limitation, although
 * most 9440 boards seem to cope. 96xx/Cyber chips don't need this limit
 * so, I'm gonna remove it and it allows lower clocks < 25.175 too ! 
 */
#ifdef STRICT
			if ((n + 8) * 100 / (m + 2) >= 978  &&
			    (n + 8) * 100 / (m + 2) <= 349)
				continue;
#endif

			diff = fabs (clockReq - target);
			if (diff < bestDiff) {
				bestDiff = diff;
				best = target;
				*mOut = m;  *nOut = n;  *kOut = k;
				*clockOut = best;
			}
		}
	}
	return (best != 0.0);
}
