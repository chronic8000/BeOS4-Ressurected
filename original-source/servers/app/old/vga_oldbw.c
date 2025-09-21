//******************************************************************************
//
//	File:		vga.c
//
//	Description:	1 bit vga initialisation.
//	
//	Written by:	Steve Sakoman.
//			Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/
// Right now, there is not a lot of things going on in this
// file since the init for this card is handled by the kernel.
// Note also that the slot is forced into slot E which is the kernel
// default


#include "gr_types.h"

//******************************************************************************/
#define VGA_SLOT	0x0e	
#define VGA_SLOT_BASE	(VGA_SLOT << 28)
//******************************************************************************/

#define VGA_REGS	(VGA_SLOT_BASE+0x01000002)
#define VGA_VMEM	(VGA_SLOT_BASE+0x00280002)
#define VGA_BYTESTEP	4

#define VGA_MISCWT	(VGA_REGS+0x3c2*4)	/* Miscellaneous--for writes */
#define VGA_MISCRD	(VGA_REGS+0x3cc*4)	/* Miscellaneous--for reads  */
#define VGA_INSTS0	(VGA_REGS+0x3c2*4)	/* Input Status 0 */
#define VGA_INSTS1	(VGA_REGS+0x3da*4)	/* Input Status 1 */
#define VGA_FEATRWR	(VGA_REGS+0x3da*4)	/* Feature Control--for writes */
#define VGA_FEATRRD	(VGA_REGS+0x3ca*4)	/* Feature Control--for reads */
#define VGA_VGAEN	(VGA_REGS+0x3c3*4)	/* VGA enable register */
#define VGA_PELADRR	(VGA_REGS+0x3c7*4)	/* Clut address register for reads */
#define VGA_PELADRW	(VGA_REGS+0x3c8*4)	/* Clut address register for writes */
#define VGA_PELDATA	(VGA_REGS+0x3c9*4)	/* Clut data register */
#define VGA_PELMASK	(VGA_REGS+0x3c6*4)	/* Clut mask register */
#define VGA_SEQADR	(VGA_REGS+0x3c4*4)	/* Sequence registers address */
#define	VGA_SEQDAT	(VGA_REGS+0x3c5*4)	/* Sequence registers data */
#define VGA_CRTCADR	(VGA_REGS+0x3d4*4)	/* CRTC address register */
#define VGA_CRTCDAT	(VGA_REGS+0x3d5*4)	/* CRTC data register */
#define VGA_GRADR	(VGA_REGS+0x3ce*4)	/* Graphics address register */
#define VGA_GRDAT	(VGA_REGS+0x3cf*4)	/* Graphics data register */
#define VGA_ATRADR	(VGA_REGS+0x3c0*4)	/* Attribute address register */
#define VGA_ATRDATW	(VGA_REGS+0x3c0*4)	/* Attribute data register for writes */
#define VGA_ATRDATR	(VGA_REGS+0x3c1*4)	/* Attribute data register for reads */
#define VGA_EXTADR	(VGA_REGS+0x3de*4)	/* Extended registers address */
#define VGA_EXTDAT	(VGA_REGS+0x3df*4) 	 /* Extended registers data */


/*-----------------------------------------------------------------------*/

void	get_screen_desc(screen_desc *scrn)
{
	scrn->video_mode = MONO;
	scrn->bit_per_pixel = 1;
	scrn->base = (char *)VGA_VMEM;
	scrn->h_size = 640;
	scrn->v_size = 480;
	scrn->rowbyte = (640 / 8) * 4;
	scrn->byte_jump = 4;
}

/*----------------------------------------------------------------------*/


char video_init()
{
int i,j;

volatile	char *pelmask	=(char *)VGA_PELMASK;


/* init the clut */

	*pelmask = 0xff;

	writepel(0, 0xffffffff);
	for (i = 1;i < 255; i++) 
		writepel(i, 0x00);

	return 0;
}

/*------------------------------------------------------------------*/

writecrtc(address,data)
char address,data;
{
char *crtcaddr=(char *)VGA_CRTCADR;
char *crtcdata=(char *)VGA_CRTCDAT;

*crtcaddr=address;
*crtcdata=data;
}

/*------------------------------------------------------------------*/

writegr(address,data)
char address,data;
{
char *graddr=(char *)VGA_GRADR;
char *grdata=(char *)VGA_GRDAT;

*graddr=address;
*grdata=data;
}

/*-----------------------------------------------------------------*/

writeatr(address,data)
char address,data;
{
char *sts1=(char *)VGA_INSTS1;
char *atraddr=(char *)VGA_ATRADR;
char *atrdata=(char *)VGA_ATRDATW;
char temp;

temp=*sts1;	/* set internal address flip flop to accept address */

*atraddr=address;
*atrdata=data;
}

/*----------------------------------------------------------------*/


writepel(address,data)
char address;
unsigned int data;
{
volatile char *peladdr=(char *)VGA_PELADRW;
volatile char *peldata=(char *)VGA_PELDATA;
unsigned char red,green,blue;
int i;

*peladdr=address;
for(i=0; i<10; i++);
red=(char)((data>>16)&0x3f);
green=(char)((data>>8)&0x3f);
blue=(char)((data)&0x3f);
*peldata=red;
for(i=0; i<10; i++);
*peldata=green;
for(i=0; i<10; i++);
*peldata=blue;
for(i=0; i<10; i++);
}

/*----------------------------------------------------------------*/

writeseq(address,data)
char address,data;
{
char *seqaddr=(char *)VGA_SEQADR;
char *seqdata=(char *)VGA_SEQDAT;
int	 i;

for (i=0;i<9;i++);
*seqaddr=address;
for (i=0;i<5;i++);
*seqdata=data;
}

/*----------------------------------------------------------------*/


writeext(address,data)
char address,data;
{
char *extaddr=(char *)VGA_EXTADR;
char *extdata=(char *)VGA_EXTDAT;

*extaddr=address;
*extdata=data;
}

/*----------------------------------------------------------------*/

void	  set_pel(long index, long r, long g, long b)
{}
