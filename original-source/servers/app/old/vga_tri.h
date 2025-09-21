/* ++++++++++
        vga.h
        Copyright (C) 1992 Be Labs, Inc.  All Rights Reserved.
        Definitions relating to the vga card.

        Modification History (most recent first):
        27 may 92       rwh     new today
+++++ */

#ifndef _vgah_
#define _vgah_

#include "vga.h"

#define VGA_BASE	(VGA_SLOT << 28)

#define VGA_REGS	(VGA_BASE+0x01000000)
#define VGA_VMEM	(VGA_BASE+0x00000000)
#define VGA_VMEMSIZE	(2*1024*1024)
#define VGA_BYTESTEP	1


#define roff		VGA_BYTESTEP
#define VGA_MISCWT	(VGA_REGS+0x3c2*roff)	/* Miscellaneous--for writes */
#define VGA_MISCRD	(VGA_REGS+0x3cc*roff)	/* Miscellaneous--for reads  */
#define VGA_INSTS0	(VGA_REGS+0x3c2*roff)	/* Input Status 0 */
#define VGA_INSTS1	(VGA_REGS+0x3da*roff)	/* Input Status 1 */
#define VGA_FEATRWR	(VGA_REGS+0x3da*roff)	/* Feature Control--for writes */
#define VGA_FEATRRD	(VGA_REGS+0x3ca*roff)	/* Feature Control--for reads */
#define VGA_VGAEN	(VGA_REGS+0x3c3*roff)	/* VGA enable register */
#define VGA_PELADRR	(VGA_REGS+0x3c7*roff)	/* Clut address register for reads */
#define VGA_PELADRW	(VGA_REGS+0x3c8*roff)	/* Clut address register for writes */
#define VGA_PELDATA	(VGA_REGS+0x3c9*roff)	/* Clut data register */
#define VGA_PELMASK	(VGA_REGS+0x3c6*roff)	/* Clut mask register */
#define VGA_SEQADR	(VGA_REGS+0x3c4*roff)	/* Sequence registers address */
#define	VGA_SEQDAT	(VGA_REGS+0x3c5*roff)	/* Sequence registers data */
#define VGA_CRTCADR	(VGA_REGS+0x3d4*roff)	/* CRTC address register */
#define VGA_CRTCDAT	(VGA_REGS+0x3d5*roff)	/* CRTC data register */
#define VGA_GRADR	(VGA_REGS+0x3ce*roff)	/* Graphics address register */
#define VGA_GRDAT	(VGA_REGS+0x3cf*roff)	/* Graphics data register */
#define VGA_ATRADR	(VGA_REGS+0x3c0*roff)	/* Attribute address register */
#define VGA_ATRDATW	(VGA_REGS+0x3c0*roff)	/* Attribute data register for writes */
#define VGA_ATRDATR	(VGA_REGS+0x3c1*roff)	/* Attribute data register for reads */
/*#define VGA_EXTADR	(VGA_REGS+0x3de*roff)*/	/* Extended registers address */
/*#define VGA_EXTDAT	(VGA_REGS+0x3df*roff)*/ 	 /* Extended registers data */

#define	VGA_EXTADR	(VGA_REGS+0x3c4*roff)	/* Extended registers address */
#define VGA_EXTDAT	(VGA_REGS+0x3c5*roff)	/* Extended registers data    */

#define	VGA_ENHADR	(VGA_REGS+0x3d6*roff)	/* Enhanced registers addr    */
#define VGA_ENHDAT	(VGA_REGS+0x3d7*roff)	/* Enhanced registers data    */

#endif

