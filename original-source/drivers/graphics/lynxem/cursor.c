/* :ts=8 bk=0
 *
 * cursor.c:	Hardware cursor manipulation.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.06
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/debug.h>

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
	gfx_card_ctl		*cc;
	int32			srcoff, dstoff;
	int			sbpr, dbpr;
	uint8			val;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	vregs = ci->ci_VGARegs;
	curs = (uint8 *) ci->ci_BaseAddr0 + ci->ci_CursorOffset + (1 << 10);

	/*
	 * Layout of 32x32 2-bit AND/XOR cursors has been empirically
	 * discovered as follows:
	 *
	 * Memory
	 * Offsets	Function
	 * -----------	--------
	 * 0x00		Line 0 pixels 0-7, plane 0 (AND)
	 * 0x01		Line 0 pixels 0-7, plane 1 (XOR)
	 * 0x02		Line 0 pixels 8-15, plane 0 (AND)
	 * 0x03		Line 0 pixels 8-15, plane 1 (XOR)
	 * 0x04 - 0x07  (unused)
	 * 0x08		Line 0 pixels 16-23, plane 0 (AND)
	 * 0x09		Line 0 pixels 16-23, plane 1 (XOR)
	 * 0x0a		Line 0 pixels 24-31, plane 0 (AND)
	 * 0x0b		Line 0 pixels 24-31, plane 1 (XOR)
	 * 0x0c - 0x0f  (unused)
	 * 0x10		Line 1 pixels 0-7, plane 0 (AND)
	 * 0x11		Line 1 pixels 0-7, plane 1 (XOR)
	 * 0x12		Line 1 pixels 8-15, plane 0 (AND)
	 * 0x13		Line 1 pixels 8-15, plane 1 (XOR)
	 * 0x14 - 0x17  (unused)
	 *  etc.
	 *
	 * MSB is to the left.
	 * Pixels within the cursor area appear as follows:
	 *
	 * AND  XOR	Appearance
	 * ---  ---	----------
	 *  0    0	Solid color; SR_CURSFGCOLOR
	 *  0    1	Solid color; SR_CURSBGCOLOR
	 *  1    0	Transparent
	 *  1    1	Invert underlying framebuffer pixel
	 */
	sbpr = (curswide + 7) >> 3;	/*  Bytes per row  */
	dbpr = (CURS_MAXX + 7) >> 3;
	for (y = srcoff = 0;  y < CURS_MAXY;  y++, srcoff += sbpr) {
		for (x = 0;  x < dbpr;  x++) {
			/*
			 * If you look at the address sequence, this
			 * formula makes sense (*giggle*)...
			 */
			dstoff = (y << 4) + ((x & 1) << 1) + ((x & 2) << 2);
			if (x < sbpr  &&  y < curshigh) {
				/*
				 * DOC ERROR: andbits[] are set where the
				 * cursor is *transparent*, not opaque as
				 * documented.  However, it turns out that
				 * this is what the hardware wants.
				 * Coincidence?
				 */
				curs[dstoff    ] = andbits[srcoff + x];
				curs[dstoff + 1] = xorbits[srcoff + x];
			} else {
				/*  Transparent  */
				curs[dstoff    ] = ~0;
				curs[dstoff + 1] = 0;
			}
		}
	}

	/*  Write white and black to cursor color registers.  */
	IDXREG_W (vregs, VGA_SR, SR_CURSFGCOLOR, 0x00);
	IDXREG_W (vregs, VGA_SR, SR_CURSBGCOLOR, 0xff);

	/*  Set cursor image address.  */
	IDXREG_W (vregs, VGA_SR, SR_CURSBASEADDR_7_0,
	          Set8Bits (ci->ci_CursorOffset));
	val = IDXREG_R (vregs, VGA_SR, SR_CURSENABLE)
	    & ~MASKFIELD (SR_CURSENABLE, BASEADDR_10_8);
	IDXREG_W (vregs,
	          VGA_SR,
	          SR_CURSENABLE,
	          val | SetBitField (ci->ci_CursorOffset,
	                             10:8,
	                             SR_CURSENABLE_BASEADDR_10_8));

	cc->cc_MouseHotX = hot_x;
	cc->cc_MouseHotY = hot_y;

	/*  New hotpoint may move actual upper-left corner; reposition  */
	movecursor (di, cc->cc_MousePosX, cc->cc_MousePosY);

	return (B_OK);
}

void
movecursor (struct devinfo *di, int32 x, int32 y)
{
	register gfx_card_info	*ci;
	register gfx_card_ctl	*cc;
	register vgaregs	*vregs;
	uint8			sign;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	vregs = ci->ci_VGARegs;

	cc->cc_MousePosX = x;	/*  This may be redundant  */
	cc->cc_MousePosY = y;
	x -= cc->cc_MouseHotX;
	y -= cc->cc_MouseHotY;

	/*  $(CHIP) cursor position is a sign-magnitude affair  */
	if (x < 0) {
		x = -x;
		sign = VAL2FIELD (SR_CURSXPOS, SIGN, 1);
	} else
		sign = VAL2FIELD (SR_CURSXPOS, SIGN, 0);
	IDXREG_W (vregs, VGA_SR, SR_CURSXPOS_7_0, Set8Bits (x));
	IDXREG_W (vregs, VGA_SR, SR_CURSXPOS,
		  SetBitField (x, 10:8, SR_CURSXPOS_XPOS_10_8)
		  | sign);

	if (y < 0) {
		y = -y;
		sign = VAL2FIELD (SR_CURSYPOS, SIGN, 1);
	} else
		sign = VAL2FIELD (SR_CURSYPOS, SIGN, 0);
	IDXREG_W (vregs, VGA_SR, SR_CURSYPOS_7_0, Set8Bits (y));
	IDXREG_W (vregs, VGA_SR, SR_CURSYPOS,
		  SetBitField (y, 10:8, SR_CURSYPOS_YPOS_10_8)
		  | sign);
}

void
showcursor (register struct gfx_card_info *ci, bool on)
{
	register vgaregs	*vregs;
	register uint8		tmp;

	vregs = ci->ci_VGARegs;
	tmp = IDXREG_R (vregs, VGA_SR, SR_CURSENABLE);
	if (on)
		tmp |= VAL2FIELD (SR_CURSENABLE, CURSENABLE, TRUE);
	else
		tmp &= ~VAL2FIELD (SR_CURSENABLE, CURSENABLE, TRUE);

	IDXREG_W (vregs, VGA_SR, SR_CURSENABLE, tmp);
}
