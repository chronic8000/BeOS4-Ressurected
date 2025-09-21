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

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>

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
#if DEBUG_SYNCLOG
void
irqlog (register struct gfx_card_ctl *cc, uint32 code, uint32 val)
{
/*
 * This code is unsafe, and should only be turned on when you're actually
 * trying to nail down a thorny, interrupt-related problem.
 */
#if 0
	register int	idx, prev;

	idx = cc->cc_SyncLogIdx;
	if ((prev = idx - 1) < 0)
		prev += NSYNCLOGENTRIES;

	/*  Don't record duplicates  */
	if (cc->cc_SyncLog[prev][0] != code  ||
	    cc->cc_SyncLog[prev][1] != val)
	{
		cc->cc_SyncLog[idx][0] = code;
		cc->cc_SyncLog[idx][1] = val;
		if (++idx >= NSYNCLOGENTRIES)
			idx = 0;
		cc->cc_SyncLogIdx = idx;
	}
#else
	(void) cc;
	(void) code;
	(void) val;
#endif
}
#endif

void
clearinterrupts (register struct gfx_card_info *ci)
{
	/*
	 * Enable VBLANK interrupt (and *only* that).
	 */
	/*  IIR status enable  */
	ci->ci_Regs->IMR = (uint16) ~MASKEXPAND (IxR_VBLANK0);
	ci->ci_Regs->IIR = ~0; /*  Clear sources  */
}

void
enableinterrupts (register struct gfx_card_info *ci, int enable)
{
	if (enable)
		ci->ci_Regs->IER |= MASKEXPAND (IxR_VBLANK0);
	else
		ci->ci_Regs->IER &= ~MASKEXPAND (IxR_VBLANK0);
}

int32
i810_interrupt (void *data)
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
//irqlog (di->di_GfxCardCtl, 'irq>', (dbg_ncalls << 16) + (ci->ci_Regs->DISP_SL & 0xffff));

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

		cc = di->di_GfxCardCtl;

		/*  Writing the bit clears it.  */
		ci->ci_Regs->IIR = MASKEXPAND (IxR_VBLANK0);

		di->di_NVBlanks++;
		flags = &cc->cc_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE) {
			/*
			 * Change the framebuffer base address here.
			 * The mr_CRTC[] values have already been set up.
			 *
			 * FIXME: ci_OpenerList isn't interrupt-protected!!
			 */
			openerinfo	*oi;

			oi = (openerinfo *) FIRSTNODE (&ci->ci_OpenerList);
			writefbbase (&oi->oi_DC, ci);
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR) {
			/*
			 * Move the hardware cursor.
			 */
			movecursor (di,
			            cc->cc_CursorState.cs_X,
			            cc->cc_CursorState.cs_Y);
		}

		/*  Release the vblank semaphore.  */
		if (ci->ci_VBlankLock >= 0) {
			int32 blocked;

			if (get_sem_count (ci->ci_VBlankLock,
					   &blocked) == B_OK  &&
			    blocked < 0)
//{ irqlog (di->di_GfxCardCtl, 'rsem', -blocked);
				release_sem_etc (ci->ci_VBlankLock,
						 -blocked,
						 B_DO_NOT_RESCHEDULE);
//}
		}
		bits &= ~MASKEXPAND (IxR_VBLANK0);
	}

	if (bits) {
		/*  Where did this come from?  */
		if (bits & ci->ci_Regs->IER) {
			dprintf (("+++ Weird I810 interrupt (tell ewhac): \
IIR = 0x%08lx\n", bits));
			/*  Clear extraneous sources.  */
			ci->ci_Regs->IIR = bits;
		}
	}
//irqlog (di->di_GfxCardCtl, 'irq<', (dbg_ncalls << 16) + (ci->ci_Regs->DISP_SL & 0xffff));

	return (B_HANDLED_INTERRUPT);
}
