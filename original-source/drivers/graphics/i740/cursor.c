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

#include <graphics_p/i740/i740.h>
#include <graphics_p/i740/i740defs.h>
#include <graphics_p/i740/bena4.h>

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
	i740_card_info		*ci;
	i740_card_ctl		*cc;
	int32			srcoff, dstoff;
	int			sbpr, dbpr;
	uint8			tmp;

	ci = di->di_I740CardInfo;
	cc = di->di_I740CardCtl;
	vregs = &ci->ci_Regs->VGARegs;
	curs = ci->ci_CursorBase;

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
	acquire_sem (ci->ci_CLUTLock);
	vregs->w.XR_Idx = XR_PIXELCONFIG0;
	tmp = vregs->r.XR_Val;
	vregs->w.XR_Val = tmp | FIELDDEF (XR_PIXELCONFIG0, PALETTEMODE, EXTRA8);

	/*  Write white and black to extended palette registers 4 and 5  */
	setpaletteentry (vregs, 4, 0xff, 0xff, 0xff);
	setpaletteentry (vregs, 5, 0, 0, 0);

	/*  Back to normal palette  */
	vregs->w.XR_Val = tmp;
	release_sem (ci->ci_CLUTLock);

	/*  Set cursor image address  */
	dstoff = (uint32) ci->ci_CursorBase - (uint32) ci->ci_BaseAddr0;
	IDXREG_W (vregs, XR, XR_CURSORBASEPTR_LO,
		  FIELDVAL (XR_CURSORBASEPTR_LO,
			    15_12,
			    SetBitField (dstoff,
					 15:12,
					 XR_CURSORBASEPTR_LO_15_12)) |
		   FIELDVAL (XR_CURSORBASEPTR_LO, IMAGESEL, 0));
	IDXREG_W (vregs, XR, XR_CURSORBASEPTR_HI,
		  FIELDVAL (XR_CURSORBASEPTR_HI,
			    21_16,
			    SetBitField (dstoff,
					 21:16,
					 XR_CURSORBASEPTR_HI_21_16)));

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
	register i740_card_ctl	*cc;
	register vgaregs	*vregs;
	register int		sign;

	cc = di->di_I740CardCtl;

	vregs = &di->di_I740CardInfo->ci_Regs->VGARegs;
	cc->cc_MousePosX = x;
	cc->cc_MousePosY = y;
	x -= cc->cc_MouseHotX;
	y -= cc->cc_MouseHotY;

	/*  I740 cursor position is a sign-magnitude affair  */
	if (x < 0) {
		x = -x;
		sign = 1;
	} else
		sign = 0;

	IDXREG_W (vregs, XR, XR_CURSORXPOS_LO, Set8Bits (x));
	IDXREG_W (vregs, XR, XR_CURSORXPOS_HI,
		  FIELDVAL (XR_CURSORXPOS_HI, SIGN, sign) |
		   FIELDVAL (XR_CURSORXPOS_HI,
			     10_8,
			     SetBitField (x, 10:8, XR_CURSORXPOS_HI_10_8)));

	if (y < 0) {
		y = -y;
		sign = 1;
	} else
		sign = 0;

	IDXREG_W (vregs, XR, XR_CURSORYPOS_LO, Set8Bits (y));
	IDXREG_W (vregs, XR, XR_CURSORYPOS_HI,
		  FIELDVAL (XR_CURSORYPOS_HI, SIGN, sign) |
		   FIELDVAL (XR_CURSORYPOS_HI,
			     10_8,
			     SetBitField (y, 10:8, XR_CURSORYPOS_HI_10_8)));
}

void
showcursor (register struct i740_card_info *ci, bool on)
{
	register vgaregs	*vregs;
	register uint8		tmp;

	vregs = &ci->ci_Regs->VGARegs;

	vregs->w.XR_Idx = XR_PIXELCONFIG0;
	tmp = vregs->r.XR_Val;
	if (on)
		tmp |= FIELDVAL (XR_PIXELCONFIG0, SHOWCURSOR, TRUE);
	else
		tmp &= ~FIELDVAL (XR_PIXELCONFIG0, SHOWCURSOR, TRUE);

	vregs->w.XR_Val = tmp;
}
