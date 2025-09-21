#include <SupportDefs.h>
#include "driver.h"
#include "driver_ioctl.h"
#include "sis630defs.h"


/******************************
-------- INTERRUPT MANAGEMENT --------

Tested on SiS630 only ( disabled by hardware on other devices )

*******************************/

void clearinterrupts (devinfo *di) {
	uchar t;
	// Enable VBLANK interrupt (and *only* that).
	isa_outb(CRTC_INDEX,0x11);
	t=isa_inb(CRTC_DATA);
	t|=0x20; // Disable vertical interrupt
	t&=~0x10; //  Clear vertical interrupt
	isa_outb(CRTC_DATA,t); 
	di->di_IRQEnabled = 0 ;
	}

void enableinterrupts (devinfo *di, int enable) {
	uchar t;
	isa_outb(CRTC_INDEX,0x11);
	t=isa_inb(CRTC_DATA);
	if (enable) {
		t&=~0x20; // Enable vertical interrupt
		di->di_IRQEnabled = 1 ;
		}
	else {
		t|=0x20; // Disable vertical interrupt
		di->di_IRQEnabled = 0 ;
		}
	isa_outb(CRTC_DATA,t); 
	}

int32 sis_interrupt (void *data) {
	register devinfo	*di;
	register sis_card_info	*ci;
	register uint32		bits;
	uchar t;

ddprintf((".\n"));

	di = data;

	// We don't care
	if (!di->di_IRQEnabled)
		return (B_UNHANDLED_INTERRUPT);

	ci = di->di_sisCardInfo;
ci->vbl_calls++ ;
	
	isa_outb(CRTC_INDEX,0x11);
	t=isa_inb(CRTC_DATA);
	// The VBlank interrupt is disabled in CRT config or bit is clear
	if ((t&0x30) != 0x10 )
		return(B_UNHANDLED_INTERRUPT);

	// Most common case (not to mention the only case) is VBLANK;  handle it.

	di->di_NInterrupts++;

	t&=~0x30; // enable and clear this interrupt
	isa_outb(CRTC_DATA,t);

ddprintf((";\n"));
		
	di->di_NVBlanks++;
	//movecursor (ci, ci->ci_MousePosX, ci->ci_MousePosY);
outl( 0x850c , ci->ci_MousePosX & 0xfff ) ; // X
outl( 0x8510 , ci->ci_MousePosY & 0x7ff ) ; // Y

	//  Release the vblank semaphore.
	if (ci->ci_VBlankLock >= 0) {
		int32 blocked;

		if (get_sem_count (ci->ci_VBlankLock,
				   &blocked) == B_OK  &&
		    blocked < 0)
			release_sem_etc (ci->ci_VBlankLock,
					 -blocked,
					 B_DO_NOT_RESCHEDULE);
		}

	return (B_HANDLED_INTERRUPT);
	}

