/* :ts=8 bk=0
 *
 * irq.c:	Interrupt handling.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.06
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/debug.h>

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
	/*
	 * Enable VBLANK interrupt (and *only* that).
	 */
}

void
enableinterrupts (register struct gfx_card_info *ci, int enable)
{
#if 0
	if (enable)
		ci->ci_Regs->IER |= MASKEXPAND (IxR_VBLANK0);
	else
		ci->ci_Regs->IER &= ~MASKEXPAND (IxR_VBLANK0);
#endif
}

int32
lynxem_interrupt (void *data)
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
	
	return (B_UNHANDLED_INTERRUPT);

#if 0
	if (!(bits = ci->ci_Regs->IIR))
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
	if (GetBF (bits, IxR_VBLANK0)) {
		register gfx_card_ctl	*cc;
		register int32		*flags;
		int32			dummy;

		cc = di->di_GfxCardCtl;

		/*  Writing the bit clears it.  */
		ci->ci_Regs->IIR = MASKEXPAND (IxR_VBLANK0);

		di->di_NVBlanks++;
		flags = &cc->cc_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE) {
			/*
			 * Change the framebuffer base address here.
			 * The regs_CRTC[] values have already been set up.
			 */
			register vgaregs *vregs;

			vregs = &ci->ci_Regs->VGARegs;

			dummy = vregs->r.VGA_ST01;
			vregs->w.VGA_AR_Idx = 0;/*  Enable attribute access */
			dummy = vregs->r.VGA_ST01;/*  Reset phase again   */

			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_17_10,
				  cc->cc_ModeRegs.mr_CRTC[CR_FBBASE_17_10]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_9_2,
				  cc->cc_ModeRegs.mr_CRTC[CR_FBBASE_9_2]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_31_24,
				  cc->cc_ModeRegs.mr_CRTC[CR_FBBASE_31_24]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_23_18,
				  cc->cc_ModeRegs.mr_CRTC[CR_FBBASE_23_18]);

			vregs->w.VGA_AR_Idx  = AR_HPAN;
			vregs->w.VGA_AR_ValW = cc->cc_ModeRegs.mr_ATTR[AR_HPAN];
			vregs->w.VGA_AR_Idx  = DEF2FIELD (AR_IDX, ACCESS, LOCK);
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
		bits &= ~MASKEXPAND (IxR_VBLANK0);
	}

	if (bits) {
		/*  Where did this come from?  */
		if (bits & ci->ci_Regs->IER) {
			dprintf (("+++ Weird LynxEM interrupt (tell ewhac): \
IIR = 0x%08lx\n", bits));
			/*  Clear extraneous sources.  */
			ci->ci_Regs->IIR = bits;
		}
	}

	return (B_HANDLED_INTERRUPT);
#endif
}
