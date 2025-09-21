/* :ts=8 bk=0
 *
 * cursor.c:	Hardware cursor manipulation.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.07.23
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>

#include <graphics_p/i810/i810.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	CURS_MAXX			32
#define	CURS_MAXY			CURS_MAXX


/*****************************************************************************
 * Code.
 */
int32
setcursorshape (
struct devinfo	*di,
uint8		*xorbits,
uint8		*andbits,
uint16		curswide,
uint16		curshigh,
uint16		hot_x,
uint16		hot_y
)
{
	register vgaregs	*vregs;
	register int		x, y;
	register uint8		*curs;
	gfx_card_info		*ci;
	cursorstate		*cs;
	int32			srcoff, dstoff;
	int			sbpr, dbpr;
	uint32			tmp;

	ci = di->di_GfxCardInfo;
	cs = &di->di_GfxCardCtl->cc_CursorState;
	vregs = &ci->ci_Regs->VGARegs;
	curs = (uint8 *) ci->ci_CursorBase;

	/*
	 * Layout of 32x32 2-bit AND/XOR cursors has been empirically
	 * discovered as follows:
	 *
	 * Memory
	 * Offsets	Function
	 * -----------	--------
	 * 0x00 - 0x03	Line 0, plane 0 (AND)
	 * 0x04 - 0x07	Line 1, plane 0 (AND)
	 * 0x08 - 0x0b	Line 0, plane 1 (XOR)
	 * 0x0c - 0x0f	Line 1, plane 1 (XOR)
	 * 0x10 - 0x13	Line 2, plane 0 (AND)
	 * 0x14 - 0x17	Line 3, plane 0 (AND)
	 * 0x18 - 0x1b	Line 2, plane 1 (XOR)
	 * 0x1c - 0x1f	Line 3, plane 1 (XOR)
	 *  etc.
	 *
	 * Pixels within the cursor area appear as follows:
	 *
	 * AND  XOR	Appearance
	 * ---  ---	----------
	 *  0    0	Solid color; extended palette register 4
	 *  0    1	Solid color; extended palette register 5
	 *  1    0	Transparent
	 *  1    1	Invert underlying framebuffer pixel
	 */
	sbpr = (curswide + 7) >> 3;	/*  Bytes per row  */
	dbpr = (CURS_MAXX + 7) >> 3;
	for (y = srcoff = 0;  y < CURS_MAXY;  y++, srcoff += sbpr) {
		/*
		 * The formula I use for calculating the offsets into the HW
		 * cursor block is slightly non-obvious, so here's how I
		 * derived it for the AND plane:
		 *
		 * offset = (y >> 1) * 0x10    + (y & 1) * 0x04
		 *        = (y >> 1) << 4      + ((y & 1) << 2)
		 *        = ((y << 3) & ~0x08) + ((y & 1) << 2)
		 *        = ((y << 3) + ((y & 1) << 2)) & ~0x08
		 *
		 * Simple inspection of the address sequence reveals that
		 * the XOR plane is:
		 *
		 * offset = ((y << 3) + ((y & 1) << 2)) | 0x08
		 */
		dstoff = (y << 3) + ((y & 1) << 2);
		for (x = 0;  x < dbpr;  x++) {
			if (x < sbpr  &&  y < curshigh) {
				/*
				 * DOC ERROR: andbits[] are set where the
				 * cursor is *transparent*, not opaque as
				 * documented.  However, it turns out that
				 * this is what the hardware wants.
				 * Coincidence?
				 */
				curs[(dstoff & ~8) + x] = andbits[srcoff + x];
				curs[(dstoff |  8) + x] = xorbits[srcoff + x];
			} else {
				/*  Transparent  */
				curs[(dstoff & ~8) + x] = ~0;
				curs[(dstoff |  8) + x] = 0;
			}
		}
	}

	/*  Switch in extended palette  */
	acquire_sem (ci->ci_VGARegsLock);
	tmp = ci->ci_Regs->PIXCONF & ~MASKFIELD (PIXCONF, PALETTEMODE);
	ci->ci_Regs->PIXCONF = tmp |
			       DEF2FIELD (PIXCONF, PALETTEMODE, EXTRA8);

	/*  Write white and black to extended palette registers 4 and 5  */
	writepaletteentry (vregs, 4, 0xff, 0xff, 0xff);
	writepaletteentry (vregs, 5, 0, 0, 0);

	/*  Back to normal palette  */
	ci->ci_Regs->PIXCONF = tmp;
	release_sem (ci->ci_VGARegsLock);

	/*  Set cursor image address  */
	ci->ci_Regs->CURSBASE = di->di_CursorBase_DMA &
				MASKFIELD (CURSBASE, BASEADDR);

	cs->cs_HotX = hot_x;
	cs->cs_HotY = hot_y;

	/*  FIXME  */
	/*  New hotpoint may move actual upper-left corner; reposition  */
	movecursor (di, cs->cs_X, cs->cs_Y);

	return (B_OK);
}

void
movecursor (struct devinfo *di, int32 x, int32 y)
{
	register gfx_card_info	*ci;
	register cursorstate	*cs;
	uint32			signs = 0;

	ci = di->di_GfxCardInfo;
	cs = &di->di_GfxCardCtl->cc_CursorState;

	cs->cs_X = x;	/*  This may be redundant  */
	cs->cs_Y = y;
	x -= cs->cs_HotX;
	y -= cs->cs_HotY;

	/*  I810 cursor position is a sign-magnitude affair  */
	if (x < 0) {
		x = -x;
		signs |= VAL2FIELD (CURSPOS, XPOSSIGN, 1);
	}
	if (y < 0) {
		y = -y;
		signs |= VAL2FIELD (CURSPOS, YPOSSIGN, 1);
	}
	
	ci->ci_Regs->CURSPOS = VAL2FIELD (CURSPOS, XPOS, x) |
			       VAL2FIELD (CURSPOS, YPOS, y) |
			       signs;
}


void
calcshowcursor (struct dispconfig *dc, struct devinfo *di, int write)
{
	register uint32 tmp;
	
	tmp = dc->dc_MR.mr_PIXCONF;
	if (di->di_GfxCardCtl->cc_CursorState.cs_Enabled)
		tmp |= VAL2FIELD (PIXCONF, SHOWCURSOR, TRUE);
	else
		tmp &= ~VAL2FIELD (PIXCONF, SHOWCURSOR, TRUE);

	dc->dc_MR.mr_PIXCONF = tmp;
	if (write)
		di->di_GfxCardInfo->ci_Regs->PIXCONF = tmp;
}
