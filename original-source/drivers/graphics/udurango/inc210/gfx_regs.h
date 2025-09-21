/*-----------------------------------------------------------------------------
 * GFX_REGS.H
 *
 * Version 2.0 - February 21, 2000
 *
 * This header file contains the graphics register definitions.
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/*----------------------------------*/
/*  FIRST GENERATION GRAPHICS UNIT  */
/*----------------------------------*/

#define GP_DST_XCOOR			0x8100		/* x destination origin		*/
#define GP_DST_YCOOR			0x8102		/* y destination origin		*/
#define GP_WIDTH				0x8104		/* pixel width				*/
#define GP_HEIGHT				0x8106		/* pixel height				*/
#define GP_SRC_XCOOR			0x8108		/* x source origin			*/
#define GP_SRC_YCOOR			0x810A		/* y source origin			*/

#define GP_VECTOR_LENGTH		0x8104		/* vector length			*/
#define GP_INIT_ERROR			0x8106		/* vector initial error		*/
#define GP_AXIAL_ERROR			0x8108		/* axial error increment	*/
#define GP_DIAG_ERROR			0x810A		/* diagonal error increment */

#define GP_SRC_COLOR_0			0x810C		/* source color 0			*/
#define GP_SRC_COLOR_1			0x810E		/* source color 1			*/
#define GP_PAT_COLOR_0			0x8110		/* pattern color 0          */
#define GP_PAT_COLOR_1			0x8112		/* pattern color 1          */
#define GP_PAT_COLOR_2			0x8114		/* pattern color 2          */
#define GP_PAT_COLOR_3			0x8116		/* pattern color 3          */
#define GP_PAT_DATA_0			0x8120		/* bits 31:0 of pattern		*/
#define GP_PAT_DATA_1			0x8124		/* bits 63:32 of pattern	*/
#define GP_PAT_DATA_2			0x8128		/* bits 95:64 of pattern	*/
#define GP_PAT_DATA_3			0x812C		/* bits 127:96 of pattern	*/

#define GP_VGA_WRITE			0x8140		/* VGA write path control   */
#define GP_VGA_READ				0x8144		/* VGA read path control    */

#define GP_RASTER_MODE			0x8200		/* raster operation			*/
#define GP_VECTOR_MODE			0x8204		/* vector mode register		*/
#define GP_BLIT_MODE			0x8208		/* blit mode register		*/
#define GP_BLIT_STATUS			0x820C		/* blit status register		*/

#define GP_VGA_BASE				0x8210		/* VGA memory offset (x64K) */
#define GP_VGA_LATCH			0x8214		/* VGA display latch        */

/* "GP_VECTOR_MODE" BIT DEFINITIONS */

#define VM_X_MAJOR				0x0000		/* X major vector			*/
#define VM_Y_MAJOR				0x0001		/* Y major vector			*/
#define VM_MAJOR_INC			0x0002		/* positive major axis step */
#define VM_MINOR_INC			0x0004		/* positive minor axis step */
#define VM_READ_DST_FB			0x0008		/* read destination data	*/

/* "GP_RASTER_MODE" BIT DEFINITIONS */

#define RM_PAT_DISABLE			0x0000		/* pattern is disabled		*/
#define RM_PAT_MONO				0x0100		/* 1BPP pattern expansion	*/
#define RM_PAT_DITHER			0x0200		/* 2BPP pattern expansion	*/
#define RM_PAT_COLOR			0x0300		/* 8BPP or 16BPP pattern	*/
#define RM_PAT_MASK				0x0300		/* mask for pattern mode	*/
#define RM_PAT_TRANSPARENT		0x0400		/* transparent 1BPP pattern	*/
#define RM_SRC_TRANSPARENT		0x0800		/* transparent 1BPP source	*/

/* "GP_BLIT_STATIS" BIT DEFINITIONS */

#define BS_BLIT_BUSY			0x0001		/* blit engine is busy		*/
#define BS_PIPELINE_BUSY		0x0002		/* graphics pipeline is busy*/
#define BS_BLIT_PENDING			0x0004		/* blit pending				*/
#define BC_FLUSH				0x0080		/* flush pipeline requests  */
#define BC_8BPP					0x0000		/* 8BPP mode				*/
#define BC_16BPP				0x0100		/* 16BPP mode				*/
#define BC_FB_WIDTH_1024		0x0000		/* framebuffer width = 1024 */
#define BC_FB_WIDTH_2048		0x0200		/* framebuffer width = 2048 */

/* "GP_BLIT_MODE" BIT DEFINITIONS */

#define	BM_READ_SRC_NONE		0x0000		/* source foreground color	*/
#define BM_READ_SRC_FB			0x0001		/* read source from FB		*/
#define BM_READ_SRC_BB0			0x0002		/* read source from BB0		*/
#define BM_READ_SRC_BB1			0x0003		/* read source from BB1		*/
#define BM_READ_SRC_MASK		0x0003		/* read source mask			*/

#define	BM_READ_DST_NONE		0x0000		/* no destination data		*/
#define BM_READ_DST_BB0			0x0008		/* destination from BB0		*/
#define BM_READ_DST_BB1			0x000C		/* destination from BB1		*/
#define BM_READ_DST_FB0			0x0010		/* dest from FB (store BB0) */
#define BM_READ_DST_FB1			0x0014		/* dest from FB (store BB1) */
#define BM_READ_DST_MASK		0x001C		/* read destination mask	*/

#define BM_WRITE_FB				0x0000		/* write to framebuffer		*/
#define	BM_WRITE_MEM			0x0020		/* write to memory			*/
#define BM_WRITE_MASK			0x0020		/* write mask				*/

#define	BM_SOURCE_COLOR			0x0000		/* source is 8BPP or 16BPP	*/
#define BM_SOURCE_EXPAND		0x0040		/* source is 1BPP			*/
#define BM_SOURCE_TEXT			0x00C0		/* source is 1BPP text		*/
#define BM_SOURCE_MASK			0x00C0		/* source mask				*/

#define BM_REVERSE_Y			0x0100		/* reverse Y direction		*/

/*---------------------------------------*/
/*  FIRST GENERATION DISPLAY CONTROLLER  */
/*---------------------------------------*/

#define DC_UNLOCK				0x8300		/* lock register			*/
#define DC_GENERAL_CFG			0x8304		/* config registers...		*/
#define DC_TIMING_CFG			0x8308
#define DC_OUTPUT_CFG			0x830C

#define DC_FB_ST_OFFSET			0x8310		/* framebuffer start offset */
#define DC_CB_ST_OFFSET			0x8314		/* compression start offset */
#define DC_CURS_ST_OFFSET		0x8318		/* cursor start offset		*/
#define DC_ICON_ST_OFFSET		0x831C		/* icon start offset		*/
#define DC_VID_ST_OFFSET		0x8320		/* video start offset		*/
#define DC_LINE_DELTA			0x8324		/* fb and cb skip counts	*/
#define DC_BUF_SIZE				0x8328		/* fb and cb line size		*/

#define DC_H_TIMING_1			0x8330		/* horizontal timing...		*/
#define DC_H_TIMING_2			0x8334
#define DC_H_TIMING_3			0x8338
#define DC_FP_H_TIMING			0x833C

#define DC_V_TIMING_1			0x8340		/* vertical timing...		*/
#define DC_V_TIMING_2			0x8344
#define DC_V_TIMING_3			0x8348
#define DC_FP_V_TIMING			0x834C

#define DC_CURSOR_X				0x8350		/* cursor x position		*/
#define DC_ICON_X				0x8354		/* HACK - 1.3 definition	*/
#define DC_V_LINE_CNT			0x8354		/* vertical line counter	*/
#define DC_CURSOR_Y				0x8358		/* cursor y position		*/
#define DC_ICON_Y				0x835C		/* HACK - 1.3 definition	*/
#define DC_SS_LINE_CMP			0x835C		/* line compare value		*/
#define DC_CURSOR_COLOR			0x8360		/* cursor colors			*/
#define DC_ICON_COLOR			0x8364		/* icon colors				*/
#define DC_BORDER_COLOR			0x8368		/* border color				*/
#define DC_PAL_ADDRESS			0x8370		/* palette address			*/
#define DC_PAL_DATA				0x8374		/* palette data				*/
#define DC_DFIFO_DIAG			0x8378		/* display FIFO diagnostic	*/
#define DC_CFIFO_DIAG			0x837C		/* compression FIF0 diagnostic	*/

/* PALETTE LOCATIONS */

#define PAL_CURSOR_COLOR_0		0x100
#define PAL_CURSOR_COLOR_1		0x101
#define PAL_ICON_COLOR_0		0x102
#define PAL_ICON_COLOR_1		0x103
#define PAL_OVERSCAN_COLOR		0x104

/* UNLOCK VALUE */

#define DC_UNLOCK_VALUE		0x00004758		/* used to unlock DC regs	*/

/* "DC_GENERAL_CFG" BIT DEFINITIONS */

#define DC_GCFG_DFLE		0x00000001		/* display FIFO load enable */
#define DC_GCFG_CURE		0x00000002		/* cursor enable			*/
#define DC_GCFG_PLNO		0x00000004		/* planar offset LSB		*/
#define DC_GCFG_PPC			0x00000008		/* pixel pan compatibility  */
#define DC_GCFG_CMPE		0x00000010		/* compression enable       */
#define DC_GCFG_DECE		0x00000020		/* decompression enable     */
#define DC_GCFG_DCLK_MASK	0x000000C0		/* dotclock multiplier      */
#define DC_GCFG_DCLK_POS	6				/* dotclock multiplier      */
#define DC_GCFG_DFHPSL_MASK	0x00000F00		/* FIFO high-priority start */
#define DC_GCFG_DFHPSL_POS	8				/* FIFO high-priority start */
#define DC_GCFG_DFHPEL_MASK	0x0000F000		/* FIFO high-priority end   */
#define DC_GCFG_DFHPEL_POS	12				/* FIFO high-priority end   */
#define DC_GCFG_CIM_MASK	0x00030000		/* compressor insert mode   */
#define DC_GCFG_CIM_POS		16				/* compressor insert mode   */
#define DC_GCFG_FDTY		0x00040000		/* frame dirty mode         */
#define DC_GCFG_RTPM		0x00080000		/* real-time perf. monitor  */
#define DC_GCFG_DAC_RS_MASK	0x00700000		/* DAC register selects     */
#define DC_GCFG_DAC_RS_POS	20				/* DAC register selects     */
#define DC_GCFG_CKWR		0x00800000		/* clock write              */
#define DC_GCFG_LDBL		0x01000000		/* line double              */
#define DC_GCFG_DIAG		0x02000000		/* FIFO diagnostic mode     */
#define DC_GCFG_CH4S		0x04000000      /* sparse refresh mode		*/
#define DC_GCFG_SSLC		0x08000000		/* enable line compare		*/
#define DC_GCFG_VIDE		0x10000008		/* video enable			    */
#define DC_GCFG_DFCK		0x20000000		/* divide flat-panel clock - rev 2.3 down */
#define DC_GCFG_VRDY		0x20000000		/* video port speed - rev 2.4 up  */
#define DC_GCFG_DPCK		0x40000000		/* divide pixel clock       */
#define DC_GCFG_DDCK		0x80000000		/* divide dot clock         */

/* "DC_TIMING_CFG" BIT DEFINITIONS */

#define DC_TCFG_FPPE		0x00000001		/* flat-panel power enable  */
#define DC_TCFG_HSYE		0x00000002		/* horizontal sync enable   */
#define DC_TCFG_VSYE		0x00000004		/* vertical sync enable     */
#define DC_TCFG_BLKE		0x00000008		/* blank enable				*/
#define DC_TCFG_DDCK		0x00000010		/* DDC clock                */
#define DC_TCFG_TGEN		0x00000020		/* timing generator enable  */
#define DC_TCFG_VIEN		0x00000040		/* vertical interrupt enable*/
#define DC_TCFG_BLNK		0x00000080		/* blink enable             */
#define DC_TCFG_CHSP		0x00000100		/* horizontal sync polarity */
#define DC_TCFG_CVSP		0x00000200		/* vertical sync polarity   */
#define DC_TCFG_FHSP		0x00000400		/* panel horz sync polarity */
#define DC_TCFG_FVSP		0x00000800		/* panel vert sync polarity */
#define DC_TCFG_FCEN		0x00001000		/* flat-panel centering     */
#define DC_TCFG_CDCE		0x00002000		/* HACK - 1.3 definition	*/
#define DC_TCFG_PLNR		0x00002000		/* planar mode enable		*/
#define DC_TCFG_INTL		0x00004000		/* interlace scan           */
#define DC_TCFG_PXDB		0x00008000		/* pixel double             */
#define DC_TCFG_BKRT		0x00010000		/* blink rate               */
#define DC_TCFG_PSD_MASK	0x000E0000		/* power sequence delay     */
#define DC_TCFG_PSD_POS		17				/* power sequence delay     */
#define DC_TCFG_DDCI		0x08000000		/* DDC input (RO)           */
#define DC_TCFG_SENS		0x10000000		/* monitor sense (RO)       */
#define DC_TCFG_DNA			0x20000000		/* display not active (RO)  */
#define DC_TCFG_VNA			0x40000000		/* vertical not active (RO) */
#define DC_TCFG_VINT		0x80000000		/* vertical interrupt (RO)  */

/* "DC_OUTPUT_CFG" BIT DEFINITIONS */

#define DC_OCFG_8BPP		0x00000001		/* 8/16 bpp select          */
#define DC_OCFG_555			0x00000002		/* 16 bpp format            */
#define DC_OCFG_PCKE		0x00000004		/* PCLK enable              */
#define DC_OCFG_FRME		0x00000008		/* frame rate mod enable    */
#define DC_OCFG_DITE		0x00000010		/* dither enable            */
#define DC_OCFG_2PXE		0x00000020		/* 2 pixel enable           */
#define DC_OCFG_2XCK		0x00000040		/* 2 x pixel clock          */
#define DC_OCFG_2IND		0x00000080		/* 2 index enable           */
#define DC_OCFG_34ADD		0x00000100		/* 3- or 4-bit add          */
#define DC_OCFG_FRMS		0x00000200		/* frame rate mod select    */
#define DC_OCFG_CKSL		0x00000400		/* clock select             */
#define DC_OCFG_PRMP		0x00000800		/* palette re-map           */
#define DC_OCFG_PDEL		0x00001000		/* panel data enable low    */
#define DC_OCFG_PDEH		0x00002000		/* panel data enable high   */
#define DC_OCFG_CFRW		0x00004000		/* comp line buffer r/w sel */
#define DC_OCFG_DIAG		0x00008000		/* comp line buffer diag    */

#define MC_DR_ADD			0x00008418
#define MC_DR_ACC			0x0000841C

/*----------*/
/*  CS5530  */
/*----------*/

/* CS5530 REGISTER DEFINITIONS */

#define CS5530_VIDEO_CONFIG 		0x0000
#define CS5530_DISPLAY_CONFIG       0x0004
#define CS5530_VIDEO_X_POS          0x0008
#define CS5530_VIDEO_Y_POS          0x000C
#define CS5530_VIDEO_SCALE          0x0010
#define CS5530_VIDEO_COLOR_KEY		0x0014
#define CS5530_VIDEO_COLOR_MASK		0x0018
#define CS5530_PALETTE_ADDRESS 		0x001C
#define CS5530_PALETTE_DATA	 		0x0020
#define CS5530_DOT_CLK_CONFIG       0x0024
#define CS5530_CRCSIG_TFT_TV        0x0028

/* "CS5530_VIDEO_CONFIG" BIT DEFINITIONS */

#define CS5530_VCFG_VID_EN					0x00000001	
#define CS5530_VCFG_VID_REG_UPDATE			0x00000002	
#define CS5530_VCFG_VID_INP_FORMAT			0x0000000C	
#define CS5530_VCFG_8_BIT_4_2_0				0x00000004
#define CS5530_VCFG_16_BIT_4_2_0			0x00000008
#define CS5530_VCFG_GV_SEL					0x00000010	
#define CS5530_VCFG_CSC_BYPASS				0x00000020	
#define CS5530_VCFG_X_FILTER_EN				0x00000040	
#define CS5530_VCFG_Y_FILTER_EN				0x00000080	
#define CS5530_VCFG_LINE_SIZE_LOWER_MASK	0x0000FF00	
#define CS5530_VCFG_INIT_READ_MASK			0x01FF0000	
#define CS5530_VCFG_EARLY_VID_RDY  			0x02000000	
#define CS5530_VCFG_LINE_SIZE_UPPER			0x08000000	
#define CS5530_VCFG_4_2_0_MODE				0x10000000	
#define CS5530_VCFG_16_BIT_EN				0x20000000
#define CS5530_VCFG_HIGH_SPD_INT			0x40000000

/* "CS5530_DISPLAY_CONFIG" BIT DEFINITIONS */

#define CS5530_DCFG_DIS_EN					0x00000001	
#define CS5530_DCFG_HSYNC_EN				0x00000002	
#define CS5530_DCFG_VSYNC_EN				0x00000004	
#define CS5530_DCFG_DAC_BL_EN				0x00000008	
#define CS5530_DCFG_RESERVED_4				0x00000010	
#define CS5530_DCFG_DAC_PWDNX				0x00000020	
#define CS5530_DCFG_FP_PWR_EN				0x00000040	
#define CS5530_DCFG_FP_DATA_EN				0x00000080	
#define CS5530_DCFG_CRT_HSYNC_POL 			0x00000100	
#define CS5530_DCFG_CRT_VSYNC_POL 			0x00000200	
#define CS5530_DCFG_FP_HSYNC_POL  			0x00000400	
#define CS5530_DCFG_FP_VSYNC_POL  			0x00000800	
#define CS5530_DCFG_XGA_FP		  			0x00001000	
#define CS5530_DCFG_FP_DITH_EN				0x00002000	
#define CS5530_DCFG_CRT_SYNC_SKW_MASK		0x0001C000
#define CS5530_DCFG_CRT_SYNC_SKW_INIT		0x00010000
#define CS5530_DCFG_PWR_SEQ_DLY_MASK		0x000E0000
#define CS5530_DCFG_PWR_SEQ_DLY_INIT		0x00080000
#define CS5530_DCFG_VG_CK					0x00100000
#define CS5530_DCFG_GV_PAL_BYP				0x00200000
#define CS5530_DCFG_DDC_SCL					0x00400000
#define CS5530_DCFG_DDC_SDA					0x00800000
#define CS5530_DCFG_DDC_OE					0x01000000
#define CS5530_DCFG_16_BIT_EN				0x02000000
#define CS5530_DCFG_DAC_EXT_VOLT_EN			0x04000000
#define CS5530_DCFG_FP_ON					0x08000000
#define CS5530_DCFG_BLUE_COMPARE			0x10000000
#define CS5530_DCFG_GREEN_COMPARE			0x20000000
#define CS5530_DCFG_RED_COMPARE				0x40000000
#define CS5530_DCFG_DDC_INPUT				0x80000000

/*----------*/
/*  SC1400  */
/*----------*/

/* SC1400 VIDEO REGISTER DEFINITIONS */

#define SC1400_VIDEO_CONFIG 				0x000
#define SC1400_DISPLAY_CONFIG				0x004
#define SC1400_VIDEO_X_POS					0x008
#define SC1400_VIDEO_Y_POS					0x00C
#define SC1400_VIDEO_SCALE					0x010
#define SC1400_VIDEO_COLOR_KEY				0x014
#define SC1400_VIDEO_COLOR_MASK				0x018
#define SC1400_PALETTE_ADDRESS 				0x01C
#define SC1400_PALETTE_DATA	 				0x020
#define SC1400_VID_MISC						0x028
#define SC1400_VID_CLOCK_SELECT				0x02C
#define SC1400_VID_CRC						0x044
#define SC1400_TVOUT_HORZ_TIM				0x800
#define SC1400_TVOUT_HORZ_SYNC				0x804
#define SC1400_TVOUT_VERT_SYNC				0x808
#define SC1400_TVOUT_LINE_END				0x80C
#define SC1400_TVOUT_VERT_DOWNSCALE			0x810
#define SC1400_TVOUT_HORZ_SCALING			0x814
#define SC1400_TVOUT_EMMA_BYPASS			0x81C
#define SC1400_TVENC_TIM_CTRL_1				0xC00
#define SC1400_TVENC_TIM_CTRL_2				0xC04
#define SC1400_TVENC_TIM_CTRL_3				0xC08
#define SC1400_TVENC_SUB_FREQ				0xC0C
#define SC1400_TVENC_DISP_POS				0xC10
#define SC1400_TVENC_DISP_SIZE				0xC14
#define SC1400_TVENC_CC_DATA				0xC18
#define SC1400_TVENC_EDS_DATA				0xC1C
#define SC1400_TVENC_CGMS_DATA				0xC20
#define SC1400_TVENC_WSS_DATA				0xC24
#define SC1400_TVENC_CC_CONTROL				0xC28
#define SC1400_TVENC_DAC_CONTROL			0xC2C

/* "SC1400_VIDEO_CONFIG" BIT DEFINITIONS */

#define SC1400_VCFG_VID_EN					0x00000001	
#define SC1400_VCFG_VID_REG_UPDATE			0x00000002	
#define SC1400_VCFG_VID_INP_FORMAT			0x0000000C	
#define SC1400_VCFG_8_BIT_4_2_0				0x00000004
#define SC1400_VCFG_16_BIT_4_2_0			0x00000008
#define SC1400_VCFG_GV_SEL					0x00000010	
#define SC1400_VCFG_CSC_BYPASS				0x00000020	
#define SC1400_VCFG_X_FILTER_EN				0x00000040	
#define SC1400_VCFG_Y_FILTER_EN				0x00000080	
#define SC1400_VCFG_LINE_SIZE_LOWER_MASK	0x0000FF00	
#define SC1400_VCFG_INIT_READ_MASK			0x01FF0000	
#define SC1400_VCFG_EARLY_VID_RDY  			0x02000000	
#define SC1400_VCFG_LINE_SIZE_UPPER			0x08000000	
#define SC1400_VCFG_4_2_0_MODE				0x10000000	
#define SC1400_VCFG_16_BIT_EN				0x20000000
#define SC1400_VCFG_HIGH_SPD_INT			0x40000000

/* "SC1400_DISPLAY_CONFIG" BIT DEFINITIONS */

#define SC1400_DCFG_DIS_EN					0x00000001	
#define SC1400_DCFG_HSYNC_EN				0x00000002	
#define SC1400_DCFG_VSYNC_EN				0x00000004	
#define SC1400_DCFG_DAC_BL_EN				0x00000008	
#define SC1400_DCFG_DAC_PWDNX				0x00000020	
#define SC1400_DCFG_TVOUT_EN				0x000000C0	
#define SC1400_DCFG_CRT_HSYNC_POL 			0x00000100	
#define SC1400_DCFG_CRT_VSYNC_POL 			0x00000200	
#define SC1400_DCFG_FP_HSYNC_POL  			0x00000400	
#define SC1400_DCFG_FP_VSYNC_POL  			0x00000800	
#define SC1400_DCFG_XGA_FP		  			0x00001000	
#define SC1400_DCFG_FP_DITH_EN				0x00002000	
#define SC1400_DCFG_CRT_SYNC_SKW_MASK		0x0001C000
#define SC1400_DCFG_CRT_SYNC_SKW_INIT		0x00010000
#define SC1400_DCFG_PWR_SEQ_DLY_MASK		0x000E0000
#define SC1400_DCFG_PWR_SEQ_DLY_INIT		0x00080000
#define SC1400_DCFG_VG_CK					0x00100000
#define SC1400_DCFG_GV_PAL_BYP				0x00200000
#define SC1400_DCFG_DDC_SCL					0x00400000
#define SC1400_DCFG_DDC_SDA					0x00800000
#define SC1400_DCFG_DDC_OE					0x01000000
#define SC1400_DCFG_16_BIT_EN				0x02000000

/* SC1400 VIP REGISTER DEFINITIONS */

#define SC1400_VIP_CONFIG					0x00000000
#define SC1400_VIP_CONTROL					0x00000004
#define SC1400_VIP_STATUS					0x00000008
#define SC1400_VIP_CURRENT_LINE				0x00000010
#define SC1400_VIP_LINE_TARGET				0x00000014
#define SC1400_VIP_ODD_BASE					0x00000020
#define SC1400_VIP_EVEN_BASE				0x00000024
#define SC1400_VIP_PITCH					0x00000028
#define SC1400_VBI_ODD_BASE					0x00000040
#define SC1400_VBI_EVEN_BASE				0x00000044
#define SC1400_VBI_PITCH					0x00000048

/* "SC1400_VIP_CONTROL" BIT DEFINITIONS */

#define	SC1400_VIP_DATA_CAPTURE_EN			0x00000100
#define	SC1400_VIP_VBI_CAPTURE_EN			0x00000200

/* SC1400 CONFIGURATION BLOCK */

#define SC1400_CB_BASE_ADDR					0x9000   
#define SC1400_CB_MISC_CONFIG				0x0034     

/* MISCELLANEOUS CONFIG REGISTER BIT DEFINITIONS */

#define SC1400_MCR_GENLOCK_SINGLE           0x00000004  
#define SC1400_MCR_VPIN_CCIR656             0x00000010
#define SC1400_MCR_VPOUT_MODE               0x00000020 
#define SC1400_MCR_VPOUT_CK_SELECT          0x00004000 
#define SC1400_MCR_VPOUT_CK_SOURCE          0x00008000
#define SC1400_MCR_GENLOCK_SYNC_FALLING     0x00010000
#define SC1400_MCR_GENLOCK_CRT_FALLING      0x00020000  
#define SC1400_MCR_GENLOCK_CONTINUE         0x00040000  
#define SC1400_MCR_GENLOCK_TIMEOUT_ENABLE   0x00080000  

/*----------*/
/*  SC1200  */
/*----------*/

/* SC1200 VIDEO REGISTER DEFINITIONS */

#define SC1200_VIDEO_CONFIG 				0x000
#define SC1200_DISPLAY_CONFIG				0x004
#define SC1200_VIDEO_X_POS					0x008
#define SC1200_VIDEO_Y_POS					0x00C
#define SC1200_VIDEO_SCALE					0x010
#define SC1200_VIDEO_COLOR_KEY				0x014
#define SC1200_VIDEO_COLOR_MASK				0x018
#define SC1200_PALETTE_ADDRESS 				0x01C
#define SC1200_PALETTE_DATA	 				0x020
#define SC1200_VID_MISC						0x028
#define SC1200_VID_CLOCK_SELECT				0x02C
#define SC1200_VID_CRC						0x044
#define SC1200_DEVICE_ID					0x048
#define SC1200_VID_ALPHA_CONTROL			0x04C
#define SC1200_CURSOR_COLOR_KEY				0x050
#define SC1200_CURSOR_COLOR_MASK			0x054
#define SC1200_CURSOR_COLOR_1				0x058
#define SC1200_CURSOR_COLOR_2				0x05C
#define SC1200_ALPHA_XPOS_1					0x060
#define SC1200_ALPHA_YPOS_1					0x064
#define SC1200_ALPHA_COLOR_1				0x068
#define SC1200_ALPHA_CONTROL_1				0x06C
#define SC1200_ALPHA_XPOS_2					0x070
#define SC1200_ALPHA_YPOS_2					0x074
#define SC1200_ALPHA_COLOR_2				0x078
#define SC1200_ALPHA_CONTROL_2				0x07C
#define SC1200_ALPHA_XPOS_3					0x080
#define SC1200_ALPHA_YPOS_3					0x084
#define SC1200_ALPHA_COLOR_3				0x088
#define SC1200_ALPHA_CONTROL_3				0x08C
#define SC1200_TVOUT_HORZ_TIM				0x800
#define SC1200_TVOUT_HORZ_SYNC				0x804
#define SC1200_TVOUT_VERT_SYNC				0x808
#define SC1200_TVOUT_LINE_END				0x80C
#define SC1200_TVOUT_VERT_DOWNSCALE			0x810
#define SC1200_TVOUT_HORZ_SCALING			0x814
#define SC1200_TVOUT_EMMA_BYPASS			0x81C
#define SC1200_TVENC_TIM_CTRL_1				0xC00
#define SC1200_TVENC_TIM_CTRL_2				0xC04
#define SC1200_TVENC_TIM_CTRL_3				0xC08
#define SC1200_TVENC_SUB_FREQ				0xC0C
#define SC1200_TVENC_DISP_POS				0xC10
#define SC1200_TVENC_DISP_SIZE				0xC14
#define SC1200_TVENC_CC_DATA				0xC18
#define SC1200_TVENC_EDS_DATA				0xC1C
#define SC1200_TVENC_CGMS_DATA				0xC20
#define SC1200_TVENC_WSS_DATA				0xC24
#define SC1200_TVENC_CC_CONTROL				0xC28
#define SC1200_TVENC_DAC_CONTROL			0xC2C

/* "SC1200_VIDEO_CONFIG" BIT DEFINITIONS */

#define SC1200_VCFG_VID_EN					0x00000001	
#define SC1200_VCFG_VID_REG_UPDATE			0x00000002	
#define SC1200_VCFG_VID_INP_FORMAT			0x0000000C	
#define SC1200_VCFG_8_BIT_4_2_0				0x00000004
#define SC1200_VCFG_16_BIT_4_2_0			0x00000008
#define SC1200_VCFG_GV_SEL					0x00000010	
#define SC1200_VCFG_CSC_BYPASS				0x00000020	
#define SC1200_VCFG_X_FILTER_EN				0x00000040	
#define SC1200_VCFG_Y_FILTER_EN				0x00000080	
#define SC1200_VCFG_LINE_SIZE_LOWER_MASK	0x0000FF00	
#define SC1200_VCFG_INIT_READ_MASK			0x01FF0000	
#define SC1200_VCFG_EARLY_VID_RDY  			0x02000000	
#define SC1200_VCFG_LINE_SIZE_UPPER			0x08000000	
#define SC1200_VCFG_4_2_0_MODE				0x10000000	
#define SC1200_VCFG_16_BIT_EN				0x20000000
#define SC1200_VCFG_HIGH_SPD_INT			0x40000000

/* "SC1200_DISPLAY_CONFIG" BIT DEFINITIONS */

#define SC1200_DCFG_DIS_EN					0x00000001	
#define SC1200_DCFG_HSYNC_EN				0x00000002	
#define SC1200_DCFG_VSYNC_EN				0x00000004	
#define SC1200_DCFG_DAC_BL_EN				0x00000008	
#define SC1200_DCFG_DAC_PWDNX				0x00000020	
#define SC1200_DCFG_TVOUT_EN				0x000000C0	
#define SC1200_DCFG_CRT_HSYNC_POL 			0x00000100	
#define SC1200_DCFG_CRT_VSYNC_POL 			0x00000200	
#define SC1200_DCFG_FP_HSYNC_POL  			0x00000400	
#define SC1200_DCFG_FP_VSYNC_POL  			0x00000800	
#define SC1200_DCFG_XGA_FP		  			0x00001000	
#define SC1200_DCFG_FP_DITH_EN				0x00002000	
#define SC1200_DCFG_CRT_SYNC_SKW_MASK		0x0001C000
#define SC1200_DCFG_CRT_SYNC_SKW_INIT		0x00010000
#define SC1200_DCFG_PWR_SEQ_DLY_MASK		0x000E0000
#define SC1200_DCFG_PWR_SEQ_DLY_INIT		0x00080000
#define SC1200_DCFG_VG_CK					0x00100000
#define SC1200_DCFG_GV_PAL_BYP				0x00200000
#define SC1200_DCFG_DDC_SCL					0x00400000
#define SC1200_DCFG_DDC_SDA					0x00800000
#define SC1200_DCFG_DDC_OE					0x01000000
#define SC1200_DCFG_16_BIT_EN				0x02000000

/* VIDEO DE-INTERLACING AND ALPHA CONTROL (REGISTER 0x4C) */

#define SC1200_ALPHA1_PRIORITY_POS			16
#define SC1200_ALPHA1_PRIORITY_MASK			0x00030000
#define SC1200_ALPHA2_PRIORITY_POS			18
#define SC1200_ALPHA2_PRIORITY_MASK			0x000C0000
#define SC1200_ALPHA3_PRIORITY_POS			20
#define SC1200_ALPHA3_PRIORITY_MASK			0x00300000

/* ALPHA CONTROL BIT DEFINITIONS (REGISTERS 0x6C, 0x7C, AND 0x8C) */

#define SC1200_ACTRL_WIN_ENABLE				0x00010000
#define SC1200_ACTRL_LOAD_ALPHA				0x00020000

/* SC1200 VIP REGISTER DEFINITIONS */

#define SC1200_VIP_CONFIG					0x00000000
#define SC1200_VIP_CONTROL					0x00000004
#define SC1200_VIP_STATUS					0x00000008
#define SC1200_VIP_CURRENT_LINE				0x00000010
#define SC1200_VIP_LINE_TARGET				0x00000014
#define SC1200_VIP_ODD_BASE					0x00000020
#define SC1200_VIP_EVEN_BASE				0x00000024
#define SC1200_VIP_PITCH					0x00000028
#define SC1200_VBI_ODD_BASE					0x00000040
#define SC1200_VBI_EVEN_BASE				0x00000044
#define SC1200_VBI_PITCH					0x00000048

/* "SC1200_VIP_CONTROL" BIT DEFINITIONS */

#define	SC1200_VIP_DATA_CAPTURE_EN			0x00000100
#define	SC1200_VIP_VBI_CAPTURE_EN			0x00000200

/* SC1200 CONFIGURATION BLOCK */

#define SC1200_CB_BASE_ADDR					0x9000   
#define SC1200_CB_MISC_CONFIG				0x0034     

/* MISCELLANEOUS CONFIG REGISTER BIT DEFINITIONS */

#define SC1200_MCR_GENLOCK_SINGLE           0x00000004  
#define SC1200_MCR_VPIN_CCIR656             0x00000010
#define SC1200_MCR_VPOUT_MODE               0x00000020 
#define SC1200_MCR_VPOUT_CK_SELECT          0x00004000 
#define SC1200_MCR_VPOUT_CK_SOURCE          0x00008000
#define SC1200_MCR_GENLOCK_SYNC_FALLING     0x00010000
#define SC1200_MCR_GENLOCK_CRT_FALLING      0x00020000  
#define SC1200_MCR_GENLOCK_CONTINUE         0x00040000  
#define SC1200_MCR_GENLOCK_TIMEOUT_ENABLE   0x00080000  

/*---------------------------------*/
/*  PHILIPS SAA7114 VIDEO DECODER  */
/*---------------------------------*/

#define SAA7114_CHIPADDR					0x42

#define SAA7114_BRIGHTNESS					0x0A
#define SAA7114_CONTRAST					0x0B
#define SAA7114_SATURATION					0x0C
#define SAA7114_HUE							0x0D
#define SAA7114_HORZ_OFFSET_LO				0x94
#define SAA7114_HORZ_OFFSET_HI				0x95
#define SAA7114_HORZ_INPUT_LO				0x96
#define SAA7114_HORZ_INPUT_HI				0x97
#define SAA7114_VERT_OFFSET_LO				0x98
#define SAA7114_VERT_OFFSET_HI				0x99
#define SAA7114_VERT_INPUT_LO				0x9A
#define SAA7114_VERT_INPUT_HI				0x9B
#define SAA7114_HORZ_OUTPUT_LO				0x9C
#define SAA7114_HORZ_OUTPUT_HI				0x9D
#define SAA7114_VERT_OUTPUT_LO				0x9E
#define SAA7114_VERT_OUTPUT_HI				0x9F
#define SAA7114_HORZ_PRESCALER				0xA0
#define SAA7114_FILTER_CONTRAST				0xA5
#define SAA7114_FILTER_SATURATION			0xA6
#define SAA7114_HSCALE_LUMA_LO				0xA8
#define SAA7114_HSCALE_LUMA_HI				0xA9
#define SAA7114_HSCALE_CHROMA_LO			0xAC
#define SAA7114_HSCALE_CHROMA_HI			0xAD
#define SAA7114_VSCALE_LUMA_LO				0xB0
#define SAA7114_VSCALE_LUMA_HI				0xB1
#define SAA7114_VSCALE_CHROMA_LO			0xB2
#define SAA7114_VSCALE_CHROMA_HI			0xB3
#define SAA7114_VSCALE_CONTROL				0xB4

/* END OF FILE */

