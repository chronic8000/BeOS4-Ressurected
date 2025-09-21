/* :ts=8 bk=0
 *
 * irq.c:	Interrupt handling.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/riva_hw.h>
#include <graphics_p/nvidia/debug.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * Globals.
 */
extern driverdata	*gdd;
#ifdef DEBUG
extern uint32		dbg_ncalls;
#endif


/****************************************************************************
 * Interrupt management.
 */
void
clearinterrupts (register struct gfx_card_info *ci)
{
	*ci->ci_HWInst.VBLANK = ci->ci_HWInst.VBlankBit;
}

void
enableinterrupts (register struct gfx_card_info *ci, int enable)
{
	/*
	 * Enable VBLANK interrupt (and *only* that).
	 */
//	ci->ci_HWInst.PMC[0x00000140/4] = enable ?  1 :  0;
	*ci->ci_HWInst.VBLANKENABLE = enable
	                            ? ci->ci_HWInst.VBlankBit
	                            : 0;
}

int32
nvidia_interrupt (void *data)
{
	register devinfo	*di;
	register gfx_card_info	*ci;
	register uint32		bits;

	di = data;
#ifdef DEBUG
	dbg_ncalls++;
#endif
	ci = di->di_GfxCardInfo;
	if (!ci->ci_IRQEnabled)
		return (B_UNHANDLED_INTERRUPT);

	if (!(bits = *ci->ci_HWInst.VBLANK))
		/*  Wasn't us.  */
		return (B_UNHANDLED_INTERRUPT);

	if (gdd)
		gdd->dd_NInterrupts++;
	di->di_NInterrupts++;
	di->di_LastIrqMask = bits;

	/*
	 * Most common case (not to mention the only case) is VBLANK;
	 * handle it first.
	 */
	if (bits & ci->ci_HWInst.VBlankBit) {
		register gfx_card_ctl	*cc;
		register int32		*flags;

		cc = di->di_GfxCardCtl;

		/*  Writing the bit clears it.  */
		*ci->ci_HWInst.VBLANK = ci->ci_HWInst.VBlankBit;

		di->di_NVBlanks++;
		flags = &cc->cc_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE) {
			/*
			 * Change the framebuffer base address here.
			 */
			(ci->ci_HWInst.SetStartAddress)
			 (&ci->ci_HWInst,
			  ci->ci_FBBase + cc->cc_ModeRegs.mr_FBULOffset);
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR) {
			/*
			 * Move the hardware cursor.
			 */
			movecursor (di, cc->cc_MousePosX, cc->cc_MousePosY);
		}

		/*  Release the vblank semaphore.  */
		if (ci->ci_VBlankLock >= 0) {
			int32 blocked;

			if (get_sem_count (ci->ci_VBlankLock,
					   &blocked) == B_OK  &&
			    blocked < 0)
				release_sem_etc (ci->ci_VBlankLock,
						 -blocked,
						 B_DO_NOT_RESCHEDULE);
		}
		bits &= ~ci->ci_HWInst.VBlankBit;
	}

	if (bits) {
		/*  Where did this come from?  */
		dprintf (("+++ Weird NVidia interrupt (tell ewhac): \
VBLANK = 0x%08lx\n", bits));
		/*  Clear extraneous sources.  */
		*ci->ci_HWInst.VBLANK = bits;
	}

	return (B_HANDLED_INTERRUPT);
}
