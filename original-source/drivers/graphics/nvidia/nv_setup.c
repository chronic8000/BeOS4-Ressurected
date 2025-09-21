/* :ts=8 bk=0
 *
 * nv_setup.c:	NVidia card setup.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.08.18
 *  Based on the XFree86 driver by David J. McKay, et al.
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/riva_hw.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * Code.
 */
typedef volatile unsigned *vup;
typedef volatile uint8 *vu8p;

static void
NVCommonSetup (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	/*
	 * Woo-hoo!  We use VBlank interrupts!
	 */
	ci->ci_HWInst.EnableIRQ	= 1;
	ci->ci_HWInst.IO	= 0x3D0;
	ci->ci_HWInst.FIFOFreeCntPtr =
	 &di->di_GfxCardCtl->cc_FIFOFreeCount;

	/*
	 * Map remaining registers.  This MUST be done in the OS specific
	 * driver code.
	 *
	 * XFree86 does this by mapping each range separately.  We could
	 * do that, too (it would be good if/when we get to fully kernel-
	 * protected drivers, so that "harmful" register ranges can only
	 * be touched by the kernel driver).  However, just assigning
	 * pointers is good enough.
	 */
	ci->ci_HWInst.PRAMDAC	=			/*   v Size v   */
	 (vup) (ci->ci_BaseAddr0 + 0x00680000);		/*  0x00001000  */
	ci->ci_HWInst.PFB	=
	 (vup) (ci->ci_BaseAddr0 + 0x00100000);		/*  0x00001000  */
	ci->ci_HWInst.PFIFO	=
	 (vup) (ci->ci_BaseAddr0 + 0x00002000);		/*  0x00002000  */
	ci->ci_HWInst.PGRAPH	=
	 (vup) (ci->ci_BaseAddr0 + 0x00400000);		/*  0x00002000  */
	ci->ci_HWInst.PEXTDEV	=
	 (vup) (ci->ci_BaseAddr0 + 0x00101000);		/*  0x00001000  */
	ci->ci_HWInst.PTIMER	=
	 (vup) (ci->ci_BaseAddr0 + 0x00009000);		/*  0x00001000  */
	ci->ci_HWInst.PMC	=
	 (vup) (ci->ci_BaseAddr0 + 0x00000000);		/*  0x00009000  */
	ci->ci_HWInst.FIFO	=
	 (vup) (ci->ci_BaseAddr0 + 0x00800000);		/*  0x00010000  */
	ci->ci_HWInst.PCIO	=
	 (vu8p) (ci->ci_BaseAddr0 + 0x00601000);	/*  0x00001000  */
	ci->ci_HWInst.PDIO	=
	 (vu8p) (ci->ci_BaseAddr0 + 0x00681000);	/*  0x00001000  */
	ci->ci_HWInst.PVIO	=
	 (vu8p) (ci->ci_BaseAddr0 + 0x000C0000);	/*  0x00001000  */

	RivaGetConfig (&ci->ci_HWInst);

	ci->ci_MaxPixClkKHz = ci->ci_HWInst.MaxVClockFreqKHz;

//	RivaEnterLeave (pScrn, TRUE);
}

void
NV1Setup (struct devinfo *di)
{
	(void) di;
}

void
NV3Setup (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	/*
	 * Record chip architecture based in PCI probe.
	 */
	ci->ci_HWInst.Architecture = 3;

	/*
	 * Map chip-specific memory-mapped registers. This MUST be done in
	 * the OS specific driver code.
	 */
	ci->ci_HWInst.PRAMIN	=			/*   v Size v   */
	 (vup) (ci->ci_BaseAddr1 + 0x00C00000);		/*  0x00008000  */
	    
	NVCommonSetup (di);
}

void
NV4Setup (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	ci->ci_HWInst.Architecture = 4;

	/*
	 * Map chip-specific memory-mapped registers. This MUST be done in
	 * the OS specific driver code.
	 */
	ci->ci_HWInst.PRAMIN	=			/*   v Size v   */
	 (vup) (ci->ci_BaseAddr0 + 0x00710000);		/*  0x00010000  */
	ci->ci_HWInst.PCRTC	=
	 (vup) (ci->ci_BaseAddr0 + 0x00600000);		/*  0x00001000  */

	NVCommonSetup (di);
}

void
NV10Setup (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	ci->ci_HWInst.Architecture = 0x10;

	/*
	 * Map chip-specific memory-mapped registers. This MUST be done in
	 * the OS specific driver code.
	 */
	ci->ci_HWInst.PRAMIN	=			/*   v Size v   */
	 (vup) (ci->ci_BaseAddr0 + 0x00710000);		/*  0x00010000  */
	ci->ci_HWInst.PCRTC	=
	 (vup) (ci->ci_BaseAddr0 + 0x00600000);		/*  0x00001000  */

	NVCommonSetup (di);
}

void
NV20Setup (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	ci->ci_HWInst.Architecture = 0x20;

	/*
	 * Map chip-specific memory-mapped registers. This MUST be done in
	 * the OS specific driver code.
	 */
	ci->ci_HWInst.PRAMIN	=			/*   v Size v   */
	 (vup) (ci->ci_BaseAddr0 + 0x00710000);		/*  0x00010000  */
	ci->ci_HWInst.PCRTC	=
	 (vup) (ci->ci_BaseAddr0 + 0x00600000);		/*  0x00001000  */

	NVCommonSetup (di);
}




#if 0

/*
 * Lock and unlock VGA and SVGA registers.
 */
void RivaEnterLeave(ScrnInfoPtr pScrn, Bool enter)
{
    unsigned char tmp;
    CARD16 vgaIOBase = VGAHW_GET_IOBASE();
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NVPtr pNv = NVPTR(pScrn);
     
    if (enter)
    {
        vgaHWUnlock(hwp);
        outb(vgaIOBase + 4, 0x11);
        tmp = inb(vgaIOBase + 5);
        outb(vgaIOBase + 5, tmp & 0x7F);
        outb(ci->ci_HWInst.LockUnlockIO, ci->ci_HWInst.LockUnlockIndex);
        outb(ci->ci_HWInst.LockUnlockIO + 1, 0x57);
    }
    else
    {
        outb(vgaIOBase + 4, 0x11);
        tmp = inb(vgaIOBase + 5);
        outb(vgaIOBase + 5, (tmp & 0x7F) | 0x80);
        outb(ci->ci_HWInst.LockUnlockIO, ci->ci_HWInst.LockUnlockIndex);
        outb(ci->ci_HWInst.LockUnlockIO + 1, 0x99);
        vgaHWLock(hwp);
    }
}
#endif
