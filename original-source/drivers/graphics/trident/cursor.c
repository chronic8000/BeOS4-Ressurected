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

#include <graphics_p/trident/trident.h>
#include <graphics_p/trident/tridentdefs.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	CURS_MAXX			64
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
	tri_card_info		*ci;
	tri_card_ctl		*cc;
	int32			srcoff, dstoff;
	int			sbpr, dbpr;
	uint8			tmp;

	if (curswide > CURS_MAXX  ||  curshigh > CURS_MAXY)
		return (E2BIG);

	ci = di->di_TriCardInfo;
	cc = di->di_TriCardCtl;
	vregs = &ci->ci_Regs->loregs.vga;
	curs = ci->ci_BaseAddr0 + ci->ci_CursorBase;

	/*
	 * Layout of 32x32 2-bit AND/XOR cursors is as follows:
	 *
	 * Memory
	 * Offsets	Function
	 * -----------	--------
	 * 0x00 - 0x03	Line 0, plane 0 (AND)
	 * 0x04 - 0x07	Line 0, plane 1 (XOR)
	 * 0x08 - 0x0f  (visible only in 64x64 cursors)
	 * 0x10 - 0x13	Line 1, plane 0 (AND)
	 * 0x14 - 0x17	Line 1, plane 1 (XOR)
	 * 0x18 - 0x1f	(visible only in 64x64 cursors)
	 *  etc.
	 *
	 * Pixels within the cursor area appear as follows:
	 *
	 * AND  XOR	Appearance
	 * ---  ---	----------
	 *  0    0	Solid color; Cursor FG color	(Docs lied again)
	 *  0    1	Solid color; Cursor BG color
	 *  1    0	Transparent
	 *  1    1	Invert underlying framebuffer pixel
	 */
	sbpr = (curswide + 7) >> 3;	/*  Bytes per row  */
	dbpr = (CURS_MAXX + 7) >> 3;
	for (y = srcoff = 0;  y < CURS_MAXY;  y++, srcoff += sbpr) {
		for (x = 0;  x < dbpr;  x++) {
			dstoff = (y << 4) + ((x & ~3) << 1);
			if (x < sbpr  &&  y < curshigh) {
				/*
				 * DOC ERROR: andbits[] are set where the
				 * cursor is *transparent*, not opaque as
				 * documented.  However, it turns out that
				 * this is what the hardware wants.
				 * Coincidence?
				 */
				curs[dstoff     + x] = andbits[srcoff + x];
				curs[dstoff + 4 + x] = xorbits[srcoff + x];
			} else {
				/*  Transparent  */
				curs[dstoff     + x] = ~0;
				curs[dstoff + 4 + x] = 0;
			}
		}
	}

	setcursorcolors (di);

	/*  Configure cursor size  */
	tmp = IDXREG_R (vregs, VGA_CR, CR_CURSCTL) &
	      ~FIELDMASK (CR_CURSCTL, SIZE);
	if (curswide > 32  ||  curshigh > 32)
		tmp |= FIELDDEF (CR_CURSCTL, SIZE, 64x64);
	else
		tmp |= FIELDDEF (CR_CURSCTL, SIZE, 32x32);
	IDXREG_W (vregs, VGA_CR, CR_CURSCTL, tmp);

	/*  Set cursor image address  */
	dstoff = ci->ci_CursorBase >> 10;
	IDXREG_W (vregs, VGA_CR, CR_CURSIMGBASE_21_10,
		  dstoff & 0x00ff);
	IDXREG_W (vregs, VGA_CR, CR_CURSIMGBASE_21_10 + 1,
		  (dstoff & 0xff00) >> 8);

	cc->cc_MouseHotX = hot_x;
	cc->cc_MouseHotY = hot_y;

	/*  FIXME  */
	/*  New hotpoint may move actual upper-left corner; reposition  */
	movecursor (di, cc->cc_MousePosX, cc->cc_MousePosY);

	return (B_OK);
}

void
movecursor (struct devinfo *di, int32 x, int32 y)
{
	register tri_card_ctl	*cc;
	register vgaregs	*vregs;
	register int		sign;
	int32			xoff, yoff;
	uint32			posreg;

	cc = di->di_TriCardCtl;

	vregs = &di->di_TriCardInfo->ci_Regs->loregs.vga;
	cc->cc_MousePosX = x;
	cc->cc_MousePosY = y;
	x -= cc->cc_MouseHotX;
	y -= cc->cc_MouseHotY;
	if (x < 0)	xoff = -x, x = 0;
	else		xoff = 0;
	if (y < 0)	yoff = -y, y = 0;
	else		yoff = 0;

	IDXREG_W (vregs, VGA_CR, CR_CURSOFFSET_X, xoff);
	IDXREG_W (vregs, VGA_CR, CR_CURSOFFSET_Y, yoff);
	
	posreg = FIELDVAL (CR_CURSPOS, XPOS, x) |
		 FIELDVAL (CR_CURSPOS, YPOS, y);

	IDXREG_W (vregs, VGA_CR, CR_CURSPOS    ,  posreg        & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSPOS + 1, (posreg >>  8) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSPOS + 2, (posreg >> 16) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSPOS + 3, (posreg >> 24) & 0xff);
}

void
showcursor (register struct tri_card_info *ci, bool on)
{
	register vgaregs	*vregs;
	register uint8		tmp;

	vregs = &ci->ci_Regs->loregs.vga;

	tmp = IDXREG_R (vregs, VGA_CR, CR_CURSCTL) &
	      ~FIELDMASK (CR_CURSCTL, CURSOR);
	if (on)
		tmp |= FIELDDEF (CR_CURSCTL, CURSOR, ENABLE);
	else
		tmp |= FIELDDEF (CR_CURSCTL, CURSOR, DISABLE);

	vregs->w.VGA_CR_Val = tmp;
}

void
setcursorcolors (struct devinfo *di)
{
	register vgaregs	*vregs;
	tri_card_info		*ci;
	tri_card_ctl		*cc;
	uint32			blkclr, whtclr;

	ci = di->di_TriCardInfo;
	cc = di->di_TriCardCtl;
	vregs = &ci->ci_Regs->loregs.vga;

	/*
	 * Write white and black to cursor color registers.
	 * Note that, in CMAP mode, the cursor color register value is
	 * not the RGB value, but an index into display CMAP where the
	 * colors are stored.  Fortunately, BeOS keeps black and white
	 * in the expected places, so this approach works for CMAP and
	 * RGB displays.
	 */
	switch (ci->ci_Depth) {
	case 8:
		/*  We punt to black and white.  Sorry...  */
		blkclr = 0;
		whtclr = 0x00ffffff;
		break;
	case 15:
		blkclr = ((cc->cc_CursColorBlk & 0x00f80000) >> 9) |
			 ((cc->cc_CursColorBlk & 0x0000f800) >> 6) |
			 ((cc->cc_CursColorBlk & 0x000000f8) >> 3);
		whtclr = ((cc->cc_CursColorWht & 0x00f80000) >> 9) |
			 ((cc->cc_CursColorWht & 0x0000f800) >> 6) |
			 ((cc->cc_CursColorWht & 0x000000f8) >> 3);
		break;
	case 16:
		blkclr = ((cc->cc_CursColorBlk & 0x00f80000) >> 8) |
			 ((cc->cc_CursColorBlk & 0x0000fc00) >> 5) |
			 ((cc->cc_CursColorBlk & 0x000000f8) >> 3);
		whtclr = ((cc->cc_CursColorWht & 0x00f80000) >> 8) |
			 ((cc->cc_CursColorWht & 0x0000fc00) >> 5) |
			 ((cc->cc_CursColorWht & 0x000000f8) >> 3);
		break;
	case 32:
		blkclr = cc->cc_CursColorBlk;
		whtclr = cc->cc_CursColorWht;
		break;
	}
	IDXREG_W (vregs, VGA_CR, CR_CURSFGCOLOR    ,  blkclr        & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSFGCOLOR + 1, (blkclr >>  8) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSFGCOLOR + 2, (blkclr >> 16) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSFGCOLOR + 3, (blkclr >> 24) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSBGCOLOR    ,  whtclr        & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSBGCOLOR + 1, (whtclr >>  8) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSBGCOLOR + 2, (whtclr >> 16) & 0xff);
	IDXREG_W (vregs, VGA_CR, CR_CURSBGCOLOR + 3, (whtclr >> 24) & 0xff);
}
