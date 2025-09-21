/*---------------------------------------------------------------------------
 * GFX_DEFS.H
 *
 * Version 2.0 - February 21, 2000
 *
 * This header file contains the macros used to access the hardware.  These
 * macros assume that 32-bit access is possible, which is true for most 
 * applications.  Projects using 16-bit compilers (the Windows98 display
 * driver) and special purpose applications (such as Darwin) need to define 
 * their own versions of these macros, which typically call a subroutine.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *---------------------------------------------------------------------------
 */

/* ACCESS TO THE CPU REGISTERS */
 
#define WRITE_REG8(offset, value) \
	(*(volatile unsigned char *)(gfx_regptr + (offset))) = (value)

#define WRITE_REG16(offset, value) \
	(*(volatile unsigned short *)(gfx_regptr + (offset))) = (value)

#define WRITE_REG32(offset, value) \
	(*(volatile unsigned long *)(gfx_regptr + (offset))) = (value)

#define READ_REG16(offset) \
    (*(volatile unsigned short *)(gfx_regptr + (offset)))

#define READ_REG32(offset) \
    (*(volatile unsigned long *)(gfx_regptr + (offset)))

/* ACCESS TO THE FRAME BUFFER */

#define WRITE_FB32(offset, value) \
	(*(volatile unsigned long *)(gfx_fbptr + (offset))) = (value)

/* ACCESS TO THE VIDEO HARDWARE */

#define READ_VID32(offset) \
	(*(volatile unsigned long *)(gfx_vidptr + (offset)))

#define WRITE_VID32(offset, value) \
	(*(volatile unsigned long *)(gfx_vidptr + (offset))) = (value)

/* ACCESS TO THE VIP HARDWARE */

#define READ_VIP32(offset) \
	(*(volatile unsigned long *)(gfx_vipptr + (offset)))

#define WRITE_VIP32(offset, value) \
	(*(volatile unsigned long *)(gfx_vipptr + (offset))) = (value)

/* END OF FILE */

