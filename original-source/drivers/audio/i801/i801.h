
#if !defined(i810_h)
#define i810_h

/* NABMBAR registers */
#define I801_CAPTURE_BASE	0x00
#define I801_CAPTURE_INFO	0x04
#define I801_CAPTURE_CIX	0x04
#define I801_CAPTURE_LVIX	0x05
#define I801_CAPTURE_SR		0x06
#define I801_CAPTURE_POS	0x08
#define I801_CAPTURE_PIX	0x0a
#define I801_CAPTURE_CR		0x0b

#define I801_PLAYBACK_BASE	0x10		/* SG table pointer */
#define I801_PLAYBACK_INFO	0x14		/* longword with info (individual access is also OK) */
#define I801_PLAYBACK_CIX	0x14		/* current buffer index */
#define I801_PLAYBACK_LVIX	0x15		/* last valid entry index */
#define I801_PLAYBACK_SR	0x16		/* status register */
#define I801_PLAYBACK_POS	0x18		/* current DWORD within current buffer */
#define I801_PLAYBACK_PIX	0x1a		/* prefetched buffer index */
#define I801_PLAYBACK_CR	0x1b		/* control register */

#define I801_MICIN_BASE		0x20
#define I801_MICIN_INFO		0x24
#define I801_MICIN_CIX		0x24
#define I801_MICIN_LVIX		0x25
#define I801_MICIN_SR		0x26
#define I801_MICIN_POS		0x28
#define I801_MICIN_PIX		0x2a
#define I801_MICIN_CR		0x2b

#define I801_GLOBAL_CONTROL	0x2c
#define I801_GLOBAL_STATUS	0x30
#define I801_CODEC_ACCESS	0x34

/* DMA SG table stuff */
#define I801_SG_TABLE_ENTS	32
#define I801_IX_MASK		0x1f
#define I801_CMD_INT		0x8000
#define I801_CMD_ZEROP		0x4000

/* get individual registers from reading the INFO longword */
#define I801_INFO_CIX(x)	((x)&0x1f)
#define I801_INFO_LVIX(x)	(((x)>>8)&0x1f)
#define I801_INFO_SR(x)		(((x)>>16)&0x1f)

/* flags in the Status Register */
#define I801_SR_FIFOE		0x10	/* FIFO Error, clear by writing 1 */
#define I801_SR_BCIS		0x08	/* Buffer Completion Interrupt Status, clear by writing 1 */
#define I801_SR_LVBCI		0x04	/* Last Valid Buffer Completed Interrupt, clear by writing 1 */
#define I801_SR_CELV		0x02	/* Current Equals Last Valid state	*/
#define I801_SR_DCH			0x01	/* DMA Controller Halted state */

/* flags in the Control Register */
#define I801_CR_IOCE		0x10	/* Interrupt On Completion Enable (if CMD_INT is set) */
#define I801_CR_FEIE		0x08	/* FIFO Error Interrupt Enable */
#define I801_CR_LVBIE		0x04	/* Last Valid Buffer Interrupt Enable */
#define I801_CR_RR			0x02	/* Reset Registers (write as 1 only when halted) */
#define I801_CR_RPBM		0x01	/* set to 1 to run/continue DMA */

/* flags in the Global Control register */
#define I801_GC_SRIE		0x20	/* Secondary Resume Interrupt Enable */
#define I801_GC_PRIE		0x10	/* Primary Resume Interrupt Enable */
#define I801_GC_ACOFF		0x08	/* ACLINK Shut Off */
#define I801_GC_ACWR		0x04	/* AC97 Warm Reset (self-clearing) */
#define I801_GC_ACCR		0x02	/* AC97 Cold Reset (write to 1 to take AC97 out of reset) */
#define I801_GC_GPIE		0x01	/* GPIO interrupt enable */

/* flags in the global status register */
#define I801_GS_MD3			0x20000	/* Power Down Semaphore for Modem */
#define I801_GS_AD3			0x10000	/* Power Down Semaphore for Audio */
#define I801_GS_RCS			0x8000	/* Read Completion Status (1 == time-out, clear in SW) */
#define I801_GS_SLOT12		0x7000	/* Mask for most recent slot 12 bits 3:1 */
#define I801_GS_SRI			0x0800	/* Secondary Resume Interrupt (clear by writing 1)	*/
#define I801_GS_PRI			0x0400	/* Primary Resume Interrupt (clear by writing 1)	*/
#define I801_GS_SCR			0x0200	/* Secondary Codec Ready (available) */
#define I801_GS_PCR			0x0100	/* Primary Codec Ready (available) */
#define	I801_GS_MINT		0x0080	/* Mic Interrupt */
#define I801_GS_POINT		0x0040	/* PCM Out (playback) Interrupt */
#define I801_GS_PIINT		0x0020	/* PCM In (capture) Interrupt */
#define I801_GS_MOINT		0x0004	/* Modem Out Interrupt */
#define I801_GS_MIINT		0x0002	/* Modem In Interrupt */
#define I801_GS_GSCI		0x0001	/* GPIO Status Change Interrupt (clear by writing 1) */

/* codec access flag */
#define I801_CA_IN_PROGRESS	0x1

#endif	//	i810_h

