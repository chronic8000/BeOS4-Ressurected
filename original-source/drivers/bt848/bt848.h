/* ++++++++++
	bt848.h

	-- DO NOT DISTRIBUTE OUTSIDE BE INC --
	
	This driver developed with information received
	from Brooktree under nondisclosure
	 
	Header file of device driver for Brooktree Bt848 video capture chip.

+++++ */

/* Bt848 ID */
#define VENDORID		0x109e
#define DEVICEID		0x0350	/* 848 */
#define DEVICEID2		0x0351	/* 849 */
#define DEVICEID3		0x036e	/* 878 */
#define DEVICEID4		0x036f	/* 879 */

/* Bt848 registers */

#define DSTATUS 		0x000
#define IFORM			0x004
#define TDEC			0x008
#define	E_CROP			0x00c
#define	O_CROP			0x08c
#define	E_VDELAY_LO		0x010
#define O_VDELAY_LO		0x090
#define E_VACTIVE_LO	0x014
#define O_VACTIVE_LO	0x094
#define E_HDELAY_LO		0x018
#define	O_HDELAY_LO		0x098
#define E_HACTIVE_LO	0x01c
#define O_HACTIVE_LO	0x09c
#define E_HSCALE_HI		0x020
#define O_HSCALE_HI		0x0a0
#define E_HSCALE_LO		0x024
#define O_HSCALE_LO		0x0a4
#define BRIGHT			0x028
#define	E_CONTROL		0x02c
#define O_CONTROL		0x0ac
#define CONTRAST_LO		0x030
#define SAT_U_LO		0x034
#define SAT_V_LO		0x038
#define HUE				0x03c
#define E_SCLOOP		0x040
#define O_SCLOOP		0x0c0
#define WC_UP			0x044	/* 878/879 only */
#define OFORM			0x048
#define E_VSCALE_HI		0x04c
#define O_VSCALE_HI		0x0cc
#define E_VSCALE_LO		0x050
#define O_VSCALE_LO		0x0d0
#define TEST			0x054
#define ADELAY			0x060
#define BDELAY			0x064
#define	ADC				0x068
#define E_VTC			0x06c
#define O_VTC			0x0ec
#define SRESET			0x07c
#define WC_DOWN			0x078	/* 878/879 only */
#define TGLB			0x080	/* 878/879 only */
#define TGCTRL			0x084	/* 878/879 only */
#define VTOTAL_LO		0x0b0	/* 878/879 only */
#define VTOTAL_HI		0x0b4	/* 878/879 only */
#define COLOR_FMT		0x0d4
#define COLOR_CTL		0x0d8
#define CAP_CTL			0x0dc
#define VBI_PACK_SIZE	0x0e0
#define VBI_PACK_DEL	0x0e4
#define	FCAP			0x0e8	/* 878/879 only */
#define	PLL_F_LO		0x0f0	/* 878/879 only */
#define	PLL_F_HI		0x0f4	/* 878/879 only */
#define	PLL_XCI			0x0f8	/* 878/879 only */
#define DVSIF			0x0fc	/* 878/879 only */
#define INT_STAT		0x100
#define INT_MASK		0x104
#define GPIO_DMA_CTL	0x10c
#define I2C				0x110
#define RISC_STRT_ADD	0x114
#define GPIO_OUT_EN		0x118
#define GPIO_REG_INP	0x11c
#define RISC_COUNT		0x120
#define GPIO_DATA		0x200

/* Risc instruction defines */
#define WRITE			(unsigned long)0x10000000
#define WRITEC			(unsigned long)0x50000000
#define	SYNC			(unsigned long)0x80000000
#define	SKIP			(unsigned long)0x20000000
#define JUMP			(unsigned long)0x70000000

#define SOL				(unsigned long)0x08000000
#define EOL				(unsigned long)0x04000000
#define	IRQ				(unsigned long)0x01000000
#define RESYNC			(unsigned long)0x00008000

#define VRE				(unsigned long)0x00000004
#define VRO				(unsigned long)0x0000000c
#define	FM1				(unsigned long)0x00000006
