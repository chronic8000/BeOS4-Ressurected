//******************************************************************************
//
//	File:		vga_oldcolor.c
//
//	Description:	8 bit old color vga initialisation.
//	
//	Written by:	Steve Sakoman.
//			Benoit Schillings
//
//	Copyright 1993, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#include <stdio.h>
#include "gr_types.h"
#include "macro.h"
#include "proto.h"
#include "vga.h"

#define	STEVE

#define FIFO_ENABLE 1

#define VGA_REGS	(VGA_SLOT_BASE + 0x1000000)
#define VGA_VMEM	(VGA_SLOT_BASE + 0x00a0000)


#define VGA_MISCWT	(VGA_REGS+0x3c2)	/* Miscellaneous--for writes */
#define VGA_MISCRD	(VGA_REGS+0x3cc)	/* Miscellaneous--for reads  */
#define VGA_INSTS0	(VGA_REGS+0x3c2)	/* Input Status 0 */
#define VGA_INSTS1	(VGA_REGS+0x3da)	/* Input Status 1 */
#define VGA_FEATRWR	(VGA_REGS+0x3da)	/* Feature Control--for writes */
#define VGA_FEATRRD	(VGA_REGS+0x3ca)	/* Feature Control--for reads */
#define VGA_VGAEN	(VGA_REGS+0x3c3)	/* VGA enable register */
#define VGA_PELADRR	(VGA_REGS+0x3c7)	/* Clut address register for reads */
#define VGA_PELADRW	(VGA_REGS+0x3c8)	/* Clut address register for writes */
#define VGA_PELDATA	(VGA_REGS+0x3c9)	/* Clut data register */
#define VGA_PELMASK	(VGA_REGS+0x3c6)	/* Clut mask register */
#define VGA_SEQADR	(VGA_REGS+0x3c4)	/* Sequence registers address */
#define	VGA_SEQDAT	(VGA_REGS+0x3c5)	/* Sequence registers data */
#define VGA_CRTCADR	(VGA_REGS+0x3d4)	/* CRTC address register */
#define VGA_CRTCDAT	(VGA_REGS+0x3d5)	/* CRTC data register */
#define VGA_GRADR	(VGA_REGS+0x3ce)	/* Graphics address register */
#define VGA_GRDAT	(VGA_REGS+0x3cf)	/* Graphics data register */
#define VGA_ATRADR	(VGA_REGS+0x3c0)	/* Attribute address register */
#define VGA_ATRDATW	(VGA_REGS+0x3c0)	/* Attribute data register for writes */
#define VGA_ATRDATR	(VGA_REGS+0x3c1)	/* Attribute data register for reads */

#define	VGA_EXTADR	(VGA_REGS+0x3c4)	/* Extended registers address */
#define VGA_EXTDAT	(VGA_REGS+0x3c5)	/* Extended registers data    */

#define	VGA_ENHADR	(VGA_REGS+0x3d6)	/* Enhanced registers addr    */
#define VGA_ENHDAT	(VGA_REGS+0x3d7)	/* Enhanced registers data    */

/*----------------------------------------------------------------------*/

void video_init();
void c_writecrtc();
void c_writegr();
void c_writeatr();
void c_writepel();
void c_writeseq();
void c_writeext();
char c_readext();
void c_write_enh();
void set_pel(long, long, long, long);

/*----------------------------------------------------------------------*/

void	get_screen_desc(screen_desc *scrn)
{
	scrn->video_mode = COLOR256;
	scrn->bit_per_pixel = 8;
	scrn->base = (char *)VGA_VMEM;
	scrn->h_size = 640;
	scrn->v_size = 480;
	scrn->rowbyte = 640;
	scrn->byte_jump = 1;
}

/*----------------------------------------------------------------------*/

void	video_init()
{
int	i;
int	j;

volatile char *misc	=(char *)VGA_MISCRD;
volatile char *miscwr	=(char *)VGA_MISCWT;
volatile char *sts0	=(char *)VGA_INSTS0;
volatile char *sts1	=(char *)VGA_INSTS1;
volatile char *ftrctl	=(char *)VGA_FEATRRD;
volatile char *ftrctlwr	=(char *)VGA_FEATRWR;
volatile char *vgaen	=(char *)VGA_VGAEN;
volatile char *pelmask	=(char *)VGA_PELMASK;
volatile char *vmem	=(char *)VGA_VMEM;
volatile char *atradr	=(char *)VGA_ATRADR;


/* init the extended registers 					*/

/* extended register init for the old VGA			*/

	/*writeext(0xd,0x00);*/	/* bandwidth control register 	*/
	/*writeext(0xe,0x00);*/	/* io trap control register 	*/

/* New extended register for ATT VGA				*/

	c_writeext(0x0b,0x00);	/* fake write to select MCR1A	*/
#ifdef FIFO_ENABLE
	c_writeext(0x0e,0x98);	/* write MCR1A			*/
				/* select clock outputs, FIFO enable  */
				/* 16 bits bus			*/
#else
	c_writeext(0x0e,0x88);	/* write MCR1A			*/
				/* select clock outputs, FIFO disable  */
				/* 16 bits bus			*/
#endif

	c_readext(0x0b);	/* fake read to select MCR1B	*/
	c_writeext(0x0e,0x02);	/* write MCR1B			*/
				/* select bank 0.		*/

	c_writeext(0x0b,0x00);	/* dummy write to put us back in default mode */

	c_writeext(0x0d,0x00);	/* Clear Mode control 2		*/
				/* Full use of CPU bandwidth	*/
				/* during blanking.		*/
				/* Select VGA mode		*/

	c_writeext(0x0f,0x1f);	/* write power up 2		*/
				/* VGA enable at 0x3c3 	*/

/* Enhanced registers						*/


#ifdef	STEVE
	c_write_enh(0x00,0x66);   /* Pout enabled, 256K RAMS, 16 bit bus */
#else
	c_write_enh(0x00,0x6e);   /* Pout enabled, 256K RAMS, 16 bit bus */
#endif	


#ifdef FIFO_ENABLE
	c_write_enh(0x01,0x02);	/* enable FIFO (02/00 en/disable), VGA mode	*/
#else
	c_write_enh(0x01,0x00);	/* disable FIFO, enable VGA mode */
#endif
	c_write_enh(0x02,0x0f);	/* check this one Steve		*/
	c_write_enh(0x03,0x00);	/* check this one Steve		*/	

/* Now some more initialization */

	c_writeseq(0, 0);		/* reset register */
	*vgaen=1;		/* enable VGA and video memory address decoding */

/* steve was here: init crtc module test reg to set clk sel bits to xx0 */
	c_writecrtc(0x11,0x0);	/* enable writes to crtc regs 	*/

#ifdef STEVE
	c_writecrtc(0x1e,0x80);
#else
	c_writecrtc(0x1e,0x00);
#endif


/* steve was here: changed miscwr from e3 to eb (sets clk sel bits to 10x) */
	*miscwr=0xeb;  		/* set sync polarity for 480 lines */
				/* select high page		   */
				/* enables internal sync.	   */
				/* set clk to clk0		   */
				/* enable video ram 		   */
				/* set CRTC addr. to 3d4	   */
	*ftrctlwr=0x00;

/* now the sequencer registers */

	c_writeseq(1,0x01);	/* clocking mode register 	*/
	c_writeseq(2,0x0f);	/* map mask register 		*/
	c_writeseq(3,0x00);	/* character map select 	*/
	c_writeseq(4,0x0e);	/* memory mode 			*/

/* init the crt controller registers 				*/

#ifdef	STEVE
	c_writecrtc(0x11,0x0);	/* enable writes to crtc regs 	*/
	c_writecrtc(0x1e,0x80);
#else
	c_writecrtc(0x11,0x0);	/* enable writes to crtc regs 	*/
#endif
	c_writecrtc(0,0xc3); 	/* horizontal total 		*/
	c_writecrtc(1,0x9f);	/* horizontal display end 	*/	
	c_writecrtc(2,0xa0);	/* start horizontal blanking 	*/
	c_writecrtc(3,0x84);	/* end horizontal blanking 	84*/
	c_writecrtc(4,0xa2);	/* start horizontal retrace 	a2*/
	c_writecrtc(5,0x42);	/* end horizontal retrace 	*/
	c_writecrtc(6,0x0b);	/* vertical total 		0b*/
	c_writecrtc(7,0x3e);	/* overflow 			*/
	c_writecrtc(8,0x00);	/* preset row scan 		*/
	c_writecrtc(9,0x40);  	/* maximum scan line 		*/
	c_writecrtc(0xa,0x00);	/* cursor start 		*/
	c_writecrtc(0xb,0x00);	/* cursor end 			*/
	c_writecrtc(0xc,0x00);	/* start address high 		*/
	c_writecrtc(0xd,0x00);	/* start address low 		*/
	c_writecrtc(0xe,0x00);	/* cursor location high 	*/
	c_writecrtc(0xf,0x00);	/* cursor location low 		*/		
	c_writecrtc(0x10,0xea);	/* start vertical retrace 	*/
	c_writecrtc(0x11,0x8c);	/* end vertical retrace 	*/
	c_writecrtc(0x12,0xdf);	/* vertical display enable end 	*/
	c_writecrtc(0x13,0x50);	/* offset 			*/
	c_writecrtc(0x14,0x40);	/* underline location 		*/
	c_writecrtc(0x15,0xe7);	/* start vertical blanking 	e7*/
	c_writecrtc(0x16,0x04);	/* end vertical blanking 	04*/
	c_writecrtc(0x17,0xa3);	/* crtc mode control 		*/
	c_writecrtc(0x18,0xff);	/* line compare 		*/

/* now the graphics registers */

	c_writegr(0,0x00);	/* set/reset 			*/
	c_writegr(1,0x00);	/* enable set/reset 		*/
	c_writegr(2,0x00);	/* color compare 		*/
	c_writegr(3,0x00);	/* data rotate 			*/
	c_writegr(4,0x00);	/* read  map select 		*/
	c_writegr(5,0x40);	/* graphics mode 		*/
	c_writegr(6,0x01);	/* miscellaneous 		*/
	c_writegr(7,0x0f);	/* color don't care 		*/
	c_writegr(8,0xff);	/* bit mask 			*/


/* now the attribute registers */

	for (i=0; i < 16; i++)
		c_writeatr(i,i);

	c_writeatr(0x10,0x41);	/* mode control register 	*/
	c_writeatr(0x11,0x00);	/* Screen border color 		*/
	c_writeatr(0x12,0x0f);	/* Color plane enable register 	*/
	c_writeatr(0x13,0x00);	/* horizontal panning register 	*/
	c_writeatr(0x14,0x00);	/* Color select register 	*/

	*atradr=0x20;

	c_writeseq(0,0x03);	/* back the reset register 	*/

/* init the clut */

	char	tmp;
	volatile char *peladdr=(char *)VGA_PELADRW;

	tmp = *pelmask;
	tmp = *pelmask;
	tmp = *pelmask;
	tmp = *pelmask;

	*pelmask=0x00;
	tmp = *peladdr;
	*pelmask=0xff;
	for (i=0; i<10; i++);

	return;
}

/*------------------------------------------------------------------*/

void	set_pel(long i, long r, long g, long b)
{
	unsigned long k;

	k = ((r >> 2) << 16) | ((g >> 2) << 8) | (b >> 2) ;
	c_writepel(i, k);
}

/*------------------------------------------------------------------*/

static
void	c_writecrtc(address,data)
char	address;
char	data;
{
	volatile char *crtcaddr=(char *)VGA_CRTCADR;
	volatile char *crtcdata=(char *)VGA_CRTCDAT;

	*crtcaddr=address;
	*crtcdata=data;
}

/*------------------------------------------------------------------*/

static
void	c_writegr(address,data)
char	address;
char	data;
{
	volatile char *graddr=(char *)VGA_GRADR;
	volatile char *grdata=(char *)VGA_GRDAT;

	*graddr=address;
	*grdata=data;
}

/*-----------------------------------------------------------------*/

static
char	c_readsts1()
{
	volatile char *sts1=(char *)VGA_INSTS1;

	return(*sts1);
}

/*----------------------------------------------------------------*/

static
void	c_writeatr(address,data)
char	address;
char	data;
{
	volatile char *atraddr=(char *)VGA_ATRADR;
	volatile char *atrdata=(char *)VGA_ATRDATW;

	c_readsts1();

	*atraddr=address;
	*atrdata=data;
}

/*----------------------------------------------------------------*/

static
void	c_writepel(address,data)
char	address;
unsigned int data;
{
	volatile char *peladdr=(char *)VGA_PELADRW;
	volatile char *peldata=(char *)VGA_PELDATA;
	unsigned char 	red;
	unsigned char 	green;
	unsigned char 	blue;
	int i;

	*peladdr=address;
	for (i=0; i<10; i++);
	
	red=(char)((data>>16)&0x3f);
	green=(char)((data>>8)&0x3f);
	blue=(char)((data)&0x3f);

	*peldata=red;
	for (i=0; i<10; i++);

	*peldata=green;
	for (i=0; i<10; i++);

	*peldata=blue;
	for (i=0; i<10; i++);
}

/*----------------------------------------------------------------*/

static
void	c_writeseq(address,data)
char	address;
char	data;
{
	volatile char 	*seqaddr=(char *)VGA_SEQADR;
	volatile char 	*seqdata=(char *)VGA_SEQDAT;

	*seqaddr=address;
	*seqdata=data;
}

/*----------------------------------------------------------------*/

static
void	c_writeext(address,data)
char	address;
char	data;
{
	volatile char *extaddr=(char *)VGA_EXTADR;
	volatile char *extdata=(char *)VGA_EXTDAT;

	*extaddr=address;
	*extdata=data;
}

/*----------------------------------------------------------------*/

static
char	c_readext(address)
char	address;
{
	volatile char	*extaddr=(char *)VGA_EXTADR;
	volatile char	*extdata=(char *)VGA_EXTDAT;

	*extaddr=address;
	return(*extdata);
}

/*----------------------------------------------------------------*/

static
void	c_write_enh(address,data)
char	address;
char	data;
{
	volatile char	*enh_addr=(char *)VGA_ENHADR;
	volatile char	*enh_data=(char *)VGA_ENHDAT;

	*enh_addr=address;
	*enh_data=data;
}

/*----------------------------------------------------------------*/

unsigned char*	c_getvgabase()
{
	return((unsigned char *)VGA_VMEM);
}

/*----------------------------------------------------------------*/

void	bank_select(long bank)
{
	char	v;
	volatile char *extaddr=(char *)VGA_EXTADR;
	volatile char *extdata=(char *)VGA_EXTDAT;
	
/* the following code is faster than a switch so ... */

	if (bank == 0)
		v = 0x02;
	else
		if (bank == 1)
			v = 0x00;
		else
			if (bank == 2)
				v = 0x06;
			else
				if (bank == 3)
					v = 0x04;

	c_readext(0x0b);		/* fake read to select MCR1B	*/

	*extaddr = 0x0e;
	*extdata = v;
}

/*----------------------------------------------------------------*/

