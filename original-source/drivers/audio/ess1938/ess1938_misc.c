#include "ess1938.h"


status_t
reset_ess1938(
	ess1938_dev * card)
{
	int cnt = 0;
	ddprintf("ess1938: reset\n");
	/* see page 22 */
	set_direct(card->sbbase + AUDIO_RESET, 0x3, 0xff);
	spin(200);
	set_direct(card->sbbase + AUDIO_RESET, 0x0, 0xff);
	
	/* "In a loop lasting at least 1 mS, poll port sbbase */
	/* + 6h bit 7 for Read Data Available. Exit the loop  */
	/* only if bit 7 is high and sbbase + Ah returns AAh" */

	/* It's a bad idea to poll for 1 mS with ints disabled */
	/* so don't call this routine with ints disables!	   */
	/* (also note that we snooze....)					   */	
	while ( !((get_direct(card->sbbase + AUDIO_RESET) & 0x80) &&
			(get_direct(card->sbbase + CONTROL_READ) == 0xAA)) ) {
			snooze(1000); /* 1 mS */
			if ( cnt++ > 25 ) {
				ddprintf("ess1938: Error resetting board \n");
				return B_ERROR;	/* something is terribly wrong */
			}
	}
	return B_OK;
}

status_t
reset_mixer(
	ess1938_dev * card)
{
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					RESET,
					0x00 ,			
					0xff );

	release_spinlock(&card->spinlock);
	restore_interrupts(cp);
	return B_OK;
}

void
setup_dma_1(
	ess1938_dev * card)
{
	uchar u;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_MODE,
					0x28 ,			/* always (!) set bit 3 */					
					0xff );

	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

	/* see page 30 */
	/* (3) sound set parameters determines the ADC_SOURCE */
	/* (4) volume is set in set parameters */

	acquire_sem(card->controller_sem);

	/* (5) B8 = CTRL_2 */
	set_controller(card, AUDIO_1_CTRL_2);
	u = 0x0E;  /* DMA is ADC | AUTO_INIT | DMA is READ (ADC) */
	set_controller(card, u);
	/* (5) A8 = ANALOG_CTRL */
	set_controller(card, READ_CONTROL_REG);
	set_controller(card, ANALOG_CTRL);
	u=get_controller(card);
	u = (u & 0xf4) | 0x01 ;  /* stereo (record monitor is 0x08 */
	set_controller(card, ANALOG_CTRL); /* redundant ? */
	set_controller(card, u);
	/* (5) B9 = AUDIO_1_XFER */
	u = 0x02; /* header vs. doc bug use 0x02 */
	set_controller(card, AUDIO_1_XFER); /* NO MASK! */
	set_controller(card, u);
	/* (6) A1 = SR */		
	u = 0x6e; /* 44.1 if bit 5 in reg 71 set (we set it below) */
	set_controller(card, AUDIO_1_SR); /* NO MASK! */
	set_controller(card, u);
	/* (6) A2 = FILTER */
	u = 0xfc; /* 252   256 - 252 = 4 ~ 17 khz */
	set_controller(card, AUDIO_1_FILTER); /* NO MASK! */
	set_controller(card, u);
	/* (6) A4/A5 Xfer count reload */ 
	u = (~(card->pcm.config.rec_buf_size) + 1) & 0xff ;	/* in bytes!! (2's comp) */
	set_controller(card, AUDIO_1_XFR_L); /* NO MASK! */
	set_controller(card, u);
	u = ((~(card->pcm.config.rec_buf_size) + 1) & 0xff00) >> 8; /* in bytes!! (2's comp) */
	set_controller(card, AUDIO_1_XFR_H); /* NO MASK! */
	set_controller(card, u);
	/* (7) delay 300 mS */
	snooze(300*1000);
	/* (8) turn on record monitor if desired */
	/* (8.5) undocumented 8-bit bit */ 
	u = 0x00; 
	set_controller(card, 0xB6); /* NO MASK! */
	set_controller(card, u);
	/* (9) B7 = AUDIO_1_CTRL_1 */
	u = 0x71; /* init DAC, prevent pops */
	set_controller(card, AUDIO_1_CTRL_1); /* NO MASK! */
	set_controller(card, u);
	u = 0xBC; /* signed, 16 bit stereo */
	set_controller(card, AUDIO_1_CTRL_1); /* NO MASK! */
	set_controller(card, u);
	/* (10) B1 = Legacy Int. Control */	
	set_controller(card, READ_CONTROL_REG);
	set_controller(card, LEGACY_IRQ_CTRL);
	u=get_controller(card);
	u = (u & 0x5F) | 0x50; /* clear 7 & 5, "verify" 4 & 6 are high */
	set_controller(card, LEGACY_IRQ_CTRL); /* Redundant? */
	set_controller(card, u);
	/* (10) B2 = Legacy DRQ Control */	
	set_controller(card, READ_CONTROL_REG);
	set_controller(card, LEGACY_DRQ_CTRL);
	u=get_controller(card);
	u = (u & 0x5F) | 0x50; /* clear 7 & 5, "verify" 4 & 6 are high */
	set_controller(card, LEGACY_DRQ_CTRL); /* Redundant? */
	set_controller(card, u);

	release_sem(card->controller_sem);
	/* see page 24 */
	/* (11) DMA controller and system interrupt controller */
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);
	set_direct(	card->vcbase + DDMA_CLEAR, 		/* Master clear */
				0xaa,	
				0xff );

	set_direct(	card->vcbase + DDMA_MASK, 		/* Mask DMA */
				0x01,	
				0xff );

	set_direct(	card->vcbase + DDMA_MODE, 		/* set mode */
				0x14,  							/*  0x14    */		
				0xff );

	/* our card expects the address in little endian format	*/
	PCI_IO_WR( card->vcbase,    ((uint32)card->dma_cap_phys.address)&0xff);
	PCI_IO_WR( card->vcbase+1, (((uint32)card->dma_cap_phys.address)>>8)&0xff);
	PCI_IO_WR( card->vcbase+2, (((uint32)card->dma_cap_phys.address)>>16)&0xff);
	
	u = (( card->pcm.config.rec_buf_size << 1) - 1 ) & 0xff;
	set_direct(	card->vcbase + DDMA_COUNT_L, 	/* dma count */
				u,	
				0xff );
    u = ((( card->pcm.config.rec_buf_size << 1) - 1 ) >> 8) & 0xff;	
	set_direct(	card->vcbase + DDMA_COUNT_H, 	/* dma count */
				u,
				0xff );
	set_direct(	card->vcbase + DDMA_MASK, 		/* Unmask DMA */
				0x00,	
				0xff );
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);


}

void
start_dma_1(
	ess1938_dev * card)
{
	uchar u;
			
	acquire_sem(card->controller_sem);

	set_controller(card, READ_CONTROL_REG);
	set_controller(card, AUDIO_1_CTRL_2);
	u=get_controller(card);
	u = u | 0x01 ;  /* start DMA */
	set_controller(card, AUDIO_1_CTRL_2); /* redundant ? */
	set_controller(card, u);

	release_sem(card->controller_sem);

}

void
stop_dma_1(
	ess1938_dev * card)
{
	uchar u;
	cpu_status cp;

	acquire_sem(card->controller_sem);
	
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_direct(	card->vcbase + DDMA_CLEAR, 		/* Master clear */
				0xaa,	
				0xff );
	
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

	release_sem(card->controller_sem);
	
	snooze(500000);

	acquire_sem(card->controller_sem);
	/* (5) B8 = CTRL_2 */
	set_controller_fast(card, AUDIO_1_CTRL_2, 0x0);
	/* (5) A8 = ANALOG_CTRL */
	set_controller_fast(card, ANALOG_CTRL, 0x11);
	/* (5) B9 = AUDIO_1_XFER */
	set_controller_fast(card, AUDIO_1_XFER, 0x0);
	/* (6) A1 = SR */		
	set_controller_fast(card, AUDIO_1_SR, 0x0);
	/* (6) A2 = FILTER */
	set_controller_fast(card, AUDIO_1_FILTER, 0x0);
	/* (6) A4/A5 Xfer count reload */ 
	set_controller_fast(card, AUDIO_1_XFR_L, 0x0);
	set_controller_fast(card, AUDIO_1_XFR_H, 0x0);
	/* (7) delay 300 mS */
	snooze(300*1000);
	/* (8) turn on record monitor if desired */
	/* (8.5) undocumented 8-bit bit */ 
	set_controller_fast(card, 0xB6, 0x0);
	/* (9) B7 = AUDIO_1_CTRL_1 */
	set_controller_fast(card, AUDIO_1_CTRL_2, 0x3A);
	/* (10) B1 = Legacy Int. Control */	
	set_controller_fast(card, LEGACY_IRQ_CTRL, 0x0);
	/* (10) B2 = Legacy DRQ Control */	
	set_controller_fast(card, LEGACY_DRQ_CTRL, 0x0);

	release_sem(card->controller_sem);

	/* (11) DMA controller and system interrupt controller */
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);
	set_direct(	card->vcbase + DDMA_CLEAR, 		/* Master clear */
				0xaa,	
				0xff );
	set_direct(	card->vcbase + DDMA_MASK, 		/* Mask DMA */
				0x01,	
				0xff );
	set_direct(	card->vcbase + DDMA_MODE, 		/* set mode */
				0x00,  							/*  0x14    */		
				0xff );
	set_direct(	card->vcbase + DDMA_COUNT_L, 	/* dma count */
				0x00,	
				0xff );
 	set_direct(	card->vcbase + DDMA_COUNT_H, 	/* dma count */
				0x00,
				0xff );
	set_direct(	card->vcbase + DDMA_MASK, 		/* Unmask DMA */
				0x00,	
				0xff );
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);
	
}	

void
setup_dma_2(
	ess1938_dev * card)
{
	uchar u;
	cpu_status cp;

	acquire_sem(card->controller_sem);
	u = 0x6e; /* 44.1 if bit 5 in reg 71 set */
	set_controller(card, AUDIO_1_SR); /* NO MASK! */
	set_controller(card, u);
	u = 0xfc; /* 252   256 - 252 = 4 ~ 17 khz */
	set_controller(card, AUDIO_1_FILTER); /* NO MASK! */
	set_controller(card, u);
	release_sem(card->controller_sem);
		
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_1,
					0x10,			/* auto-init */
					0x10 );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_SR,
					0x6e,			/* 44.1 KHz */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_MODE,
					0x28 ,			/* always (!) set bit 3 */					
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_FILTER,
					0x6e,			/* shouldn't matter, slaved to ch 1 */					
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_XFR_L,
					(~(card->pcm.config.play_buf_size >> 1)+1) & 0xff,	/* in words (2's comp) */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_XFR_H,
					((~(card->pcm.config.play_buf_size >> 1)+1) & 0xff00) >> 8,/* in words (2's comp) */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_2,
					0x07,			/* signed 16 bit stereo */
					0x07 );
	/* enable IRQ here-- reg 7A and port iobase + 7 */
	/* stuff goes here 								*/
	
	/* our card expects the address in little endian format	*/
	PCI_IO_WR( card->iobase,    ((uint32)card->dma_phys.address)&0xff);
	PCI_IO_WR( card->iobase+1, (((uint32)card->dma_phys.address)>>8)&0xff);
	PCI_IO_WR( card->iobase+2, (((uint32)card->dma_phys.address)>>16)&0xff);
	PCI_IO_WR( card->iobase+3, (((uint32)card->dma_phys.address)>>24)&0xff);
	
	u = (card->pcm.config.play_buf_size << 1) & 0xff;
	set_direct(	card->iobase + 4, 			/* dma count */
				u,	
				0xff );
    u = ((card->pcm.config.play_buf_size << 1) >> 8) & 0xff;	
	set_direct(	card->iobase + 5, 			/* dma count */
				u,
				0xff );
	set_direct(	card->iobase + 6, 			/* dma/autoinit enable */
				0x0a,	
				0x0a );

	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

}
void
start_dma_2(
	ess1938_dev * card)
{
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_1,
					0x03,			/* enable fifo, enable DMA */
					0x03 );
	/* delay ~ 100 mS here */				
	/* enable audio???     */
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

}

void
stop_dma_2(
	ess1938_dev * card)
{
	/* stopping it directly seems to leave the FIFO in an indeterminate state-- */
	/* and the FIFO resets (such as they are) don't seem to work.               */
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_1,
					0x00,			/* wait for this buffer to play out */
					0x10 );
	
	//what if an int is pending??????
	
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

	/* wait for interrupt */
	acquire_sem(card->lastint_sem); 
	/* wait for fifo to clear (32 words)*/
	snooze(1000);
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_1,
					0x00,			/* disable DMA, disable the FIFO */
					0x13 );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_SR,
					0x0,			/* 44.1 KHz */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_MODE,
					0x0 ,			/* always (!) set bit 3 */					
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_FILTER,
					0x0,			/* shouldn't matter, slaved to ch 1 */					
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_XFR_L,
					0x0,	/* in words (2's comp) */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_XFR_H,
					0x0,/* in words (2's comp) */
					0xff );
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_2,
					0x00,			/* signed 16 bit stereo */
					0x07 );

	set_direct(	card->iobase + 4, 			/* dma count */
				0,	
				0xff );
	set_direct(	card->iobase + 5, 			/* dma count */
				0,
				0xff );
	set_direct(	card->iobase + 6, 			/* dma/autoinit enable */
				0x0,	
				0x0f );
	
	
	release_spinlock(&card->spinlock);
	restore_interrupts(cp);
}
