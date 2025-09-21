/* :ts=8 bk=0
 *
 * cursor.c:	Hardware cursor manipulation.
 *
 * $Id:$
 *
 * Leo L. Schwab					9806.02
 *  Based on code from The XFree86 Project, Inc.
 *  Further modified for R4				1998.09.02
 *  Minor hacks for RIVA TNT				1998.10.26
 *  Even more hacks for the "generalized" nvidia code	2000.09.03
 ****
 * Copyright 1996-1997  David J. McKay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID J. MCKAY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/nv/nv3cursor.c,v 1.1.2.3 1998/02/08 01:12:44 dawes Exp $ */

#include <kernel/OS.h>
#include <support/Debug.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/riva_hw.h>

#include "driver.h"

#include "protos.h"


/*****************************************************************************
 * #defines
 */
#define	CURS_MAXX			32
#define	CURS_MAXY			CURS_MAXX


/*****************************************************************************
 * Global data.
 */
extern pci_module_info	*pci_bus;


/* The NV[34] supports true colour cursors, useless for X but essential
 * for those animated cursors under Win95!
 * Cursors are 15 bpp (555).  The high bit determines whether the color value
 * XORs the pixel underneath (=0) or if the color is sent through as is (=1).
 */
static const unsigned short	curspix[] = {
	0x0000,		/*  Transparent		*/
	0x7FFF,		/*  XOR all bits	*/
	0xFFFF,		/*  White		*/
	0x8000		/*  Black		*/
};


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
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;
	register int	i, x, y;
	unsigned long	*curs_lp;
	unsigned short	cursor[CURS_MAXX * CURS_MAXY];
	int		rowoffset, bpr, idx, bitnum, pixel;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;

	/*
	 * Perform initial conversion from 4-bit Be format to 15-bit
	 * NV3 format.
	 */
	bpr = (curswide + 7) >> 3;	/*  Bytes per row  */
	for (i = y = rowoffset = 0;  y < CURS_MAXY;  y++, rowoffset += bpr) {
		for (x = 0;  x < CURS_MAXX;  x++, i++) {
			if (x < curswide  &&  y < curshigh) {
				idx = rowoffset + (x >> 3);
				bitnum = ~x & 7;
				/*
				 * DOC ERROR: andbits[] are set where the
				 * cursor is *transparent*, not opaque as
				 * documented.  So we invert them here.
				 */
				pixel = ((xorbits[idx] >> bitnum) & 1) |
					(((~andbits[idx] >> bitnum) & 1) << 1);
				
				cursor[i] = curspix[pixel];
			} else
				cursor[i] = curspix[0];
		}
	}

	/*
	 * Tell hardware about our new cursor.
	 */
	(ci->ci_HWInst.ShowHideCursor) (&ci->ci_HWInst, FALSE);
	
	/*  Upload the cursor to card's Privileged RAM  */
	curs_lp = (unsigned long *) cursor;
	for (i = 0;  i < sizeof (cursor) / sizeof (unsigned long);  i++)
		ci->ci_HWInst.CURSOR[i] = *curs_lp++;

	cc->cc_MouseHotX = hot_x;
	cc->cc_MouseHotY = hot_y;
	(ci->ci_HWInst.ShowHideCursor) (&ci->ci_HWInst, TRUE);

	/*
	 * New hotpoint may move actual upper-left corner.
	 */
	movecursor (di, cc->cc_MousePosX, cc->cc_MousePosY);

	return (B_OK);
}


void
showcursor (register struct gfx_card_info *ci, bool on)
{
	(ci->ci_HWInst.ShowHideCursor) (&ci->ci_HWInst, on);
}


void
movecursor (struct devinfo *di, int32 x, int32 y)
{
	register gfx_card_info	*ci;
	register gfx_card_ctl	*cc;
	
	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	
	cc->cc_MousePosX = x;   /*  This may be redundant  */
	cc->cc_MousePosY = y;
	x -= cc->cc_MouseHotX;
	y -= cc->cc_MouseHotY;

	*ci->ci_HWInst.CURSORPOS = (x & 0xFFFF) | (y << 16);
}
