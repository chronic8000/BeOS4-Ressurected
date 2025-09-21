/*-----------------------------------------------------------------------------
 * GFX_INIT.C
 *
 * Version 2.0 - February 21, 2000
 *
 * This file contains routines typically used in driver initialization.
 *
 * Routines:
 * 
 *       gfx_pci_config_read
 *       gfx_cpu_config_read
 *       gfx_detect_cpu
 *       gfx_detect_video
 *       gfx_get_cpu_register_base
 *       gfx_get_frame_buffer_base
 *       gfx_get_frame_buffer_size
 *       gfx_get_vid_register_base
 *       gfx_get_vip_register_base
 *
 * History:
 *    Versions 0.1 through 2.0 by Brian Falardeau.
 *]
 * Copyright (c) 1999-2000 National Semiconductor.
 *-----------------------------------------------------------------------------
 */

/* CONSTANTS USED BY THE INITIALIZATION CODE */

#define PCI_CONFIG_ADDR			0x0CF8
#define PCI_CONFIG_DATA			0x0CFC
#define PCI_VENDOR_DEVICE_GXM	0x00011078

#define GXM_CONFIG_GCR			0xB8
#define GXM_CONFIG_CCR3			0xC3
#define GXM_CONFIG_DIR0			0xFE
#define GXM_CONFIG_DIR1			0xFF

#define GFX_CPU_GXLV		1
#define GFX_CPU_SC1400		2
#define GFX_CPU_SC1200		3

#define GFX_VID_CS5530		1
#define GFX_VID_SC1400		2
#define GFX_VID_SC1200		3

/* STATIC VARIABLES FOR THIS FILE */

unsigned long gfx_cpu_version = 0;
unsigned long gfx_vid_version = 0;

/*-----------------------------------------------------------------------------
 * gfx_pci_config_read
 * 
 * This routine reads a 32-bit value from the specified location in PCI
 * configuration space.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_pci_config_read(unsigned long address)
{
	unsigned long value = 0xFFFFFFFF;
	OUTD(PCI_CONFIG_ADDR, address);
	value = IND(PCI_CONFIG_DATA);
	return(value);
}

void gfx_pci_config_write(unsigned long address, unsigned long data)
{
	OUTD(PCI_CONFIG_ADDR, address);
	OUTD(PCI_CONFIG_DATA, data);
	return;
}
/*-----------------------------------------------------------------------------
 * gfx_gxm_config_read
 * 
 * This routine reads the value of the specified GXm configuration register.
 *-----------------------------------------------------------------------------
 */
unsigned char gfx_gxm_config_read(unsigned char index)
{
	unsigned char value = 0xFF;
	unsigned char lock;

	OUTB(0x22, GXM_CONFIG_CCR3);
	lock = INB(0x23);
	OUTB(0x22, GXM_CONFIG_CCR3);
	OUTB(0x23, (unsigned char) (lock | 0x10));
	OUTB(0x22, index);		
	value = INB(0x23);
	OUTB(0x22, GXM_CONFIG_CCR3);
	OUTB(0x23, lock);
	return(value);
}

/*-----------------------------------------------------------------------------
 * gfx_detect_cpu
 * 
 * This routine returns the type and revison of the CPU.  If a Geode 
 * processor is not present, the routine returns zero.
 *
 * The return value is as follows:
 *     bits[24:16] = minor version
 *     bits[15:8] = major version 
 *     bits[7:0] = type (1 = GXm, 2 = SC1400)
 *
 * A return value of 0x00020501, for example, indicates GXm version 5.2.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_detect_cpu(void)
{
	unsigned long value;
	unsigned long version = 0;
	unsigned char dir0, dir1;

	value = gfx_pci_config_read(0x80000000);
	if (value == PCI_VENDOR_DEVICE_GXM)
	{
		dir0 = gfx_gxm_config_read(GXM_CONFIG_DIR0) & 0xF0;
		dir1 = gfx_gxm_config_read(GXM_CONFIG_DIR1);

		if (dir0 == 0x40)
		{
			/* CHECK FOR GXLV (and GXm) (DIR1 = 0x30 THROUGH 0x82) */

			if ((dir1 >= 0x30) && (dir1 <= 0x82))
			{
				/* Major version is one less than what appears in DIR1 */

				version = GFX_CPU_GXLV | 
					((((unsigned long) dir1 >> 4) - 1) << 8) | /* major */
					((((unsigned long) dir1 & 0x0F)) << 16); /* minor */

				/* Currently always CS5530 for video overlay. */

				#if GFX_VIDEO_DYNAMIC
				gfx_video_type = GFX_VIDEO_TYPE_CS5530;
				#endif

				/* Currently always CS5530 GPIOs for I2C access. */

				#if GFX_I2C_DYNAMIC
				gfx_i2c_type = GFX_I2C_TYPE_GPIO;
				#endif
			}
			/* CHECK FOR SC1400 (DIR1 = 0x83 OR HIGHER) */

			else if (dir1 >= 0x83)
			{
				/*-----------------*/
				/* SC1400, REV A.1 */
				/*-----------------*/

				version = GFX_CPU_SC1400 | 0x0100;

				/* SC1400 for video overlay and VIP. */

				#if GFX_VIDEO_DYNAMIC
				gfx_video_type = GFX_VIDEO_TYPE_SC1400;
				#endif

				#if GFX_VIP_DYNAMIC
				gfx_vip_type = GFX_VIP_TYPE_SC1400;
				#endif

				/* Currently always SAA7114 decoder. */

				#if GFX_DECODER_DYNAMIC
				gfx_decoder_type = GFX_DECODER_TYPE_SAA7114;
				#endif

				/* Currently always Geode TV encoder */

				#if GFX_TV_DYNAMIC
				gfx_tv_type = GFX_TV_TYPE_GEODE;
				#endif

				/* Currently always ACCESS.bus for I2C access. */

				#if GFX_I2C_DYNAMIC
				gfx_i2c_type = GFX_I2C_TYPE_ACCESS;
				#endif
			}
		}
		else if (dir0 == 0xB0)
		{
			/* CHECK FOR SC1200 */

			if (dir1 == 0x70)
			{
				/*-----------------*/
				/* SC1200, rev B.1 */
				/*-----------------*/

				version = GFX_CPU_SC1200 | 0x0100;

				/* SC1400 for video overlay and VIP. */

				#if GFX_VIDEO_DYNAMIC
				gfx_video_type = GFX_VIDEO_TYPE_SC1200;
				#endif

				#if GFX_VIP_DYNAMIC
				gfx_vip_type = GFX_VIP_TYPE_SC1200;
				#endif

				/* Currently always SAA7114 decoder. */

				#if GFX_DECODER_DYNAMIC
				gfx_decoder_type = GFX_DECODER_TYPE_SAA7114;
				#endif

				/* Currently always Geode TV encoder */

				#if GFX_TV_DYNAMIC
				gfx_tv_type = GFX_TV_TYPE_GEODE;
				#endif

				/* Currently always ACCESS.bus for I2C access. */

				#if GFX_I2C_DYNAMIC
				gfx_i2c_type = GFX_I2C_TYPE_ACCESS;
				#endif
			}
		}
	}
				
	if (version)
	{
		/* ALWAYS FIRST GENERATION GRAPHICS UNIT */

		#if GFX_DISPLAY_DYNAMIC
		gfx_display_type = GFX_DISPLAY_TYPE_GU1;
		#endif
		#if GFX_2DACCEL_DYNAMIC
		gfx_2daccel_type = GFX_2DACCEL_TYPE_GU1;
		#endif
	}
	gfx_cpu_version = version;
	return(version);
}

/*-----------------------------------------------------------------------------
 * gfx_detect_video
 * 
 * This routine returns the type of the video hardware.
 *
 * The return value is as follows:
 *     bits[7:0] = type (1 = CS5530, 2 = SC1400)
 *
 * Currently this routine does not actually detect any hardware, and bases
 * the video hardware entirely on the detected CPU.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_detect_video(void)
{
	unsigned long version = 0;
	if ((gfx_cpu_version & 0xFF) == GFX_CPU_GXLV)
		version = GFX_VID_CS5530;
	else if ((gfx_cpu_version & 0xFF) == GFX_CPU_SC1400)
		version = GFX_VID_SC1400;
	else if ((gfx_cpu_version & 0xFF) == GFX_CPU_SC1200)
		version = GFX_VID_SC1200;
	gfx_vid_version = version;
	return(version);
}

/*-----------------------------------------------------------------------------
 * gfx_get_frame_buffer_base
 * 
 * This routine returns the base address for graphics memory.  This address 
 * is specified in the GCR register.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_cpu_register_base(void)
{
	unsigned long base;
	base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
	base = (base & 0x03) << 30;
	return(base);
}

/*-----------------------------------------------------------------------------
 * gfx_get_frame_buffer_base
 * 
 * This routine returns the base address for graphics memory.  This is an 
 * offset of 0x00800000 from the base address specified in the GCR register.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_frame_buffer_base(void)
{
	unsigned long base;
	base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
	base = (base & 0x03) << 30;
	if (base) base |= 0x00800000;
	return(base);
}

/*-----------------------------------------------------------------------------
 * gfx_get_frame_buffer_size
 * 
 * This routine returns the total size of graphics memory, in bytes.
 *
 * Currently this routine is hardcoded to return 2 Meg.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_frame_buffer_size(void)
{
	return(0x00200000);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vid_register_base
 * 
 * This routine returns the base address for the video hardware.  It assumes
 * an offset of 0x00010000 from the base address specified by the GCR.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vid_register_base(void)
{
	unsigned long base;
	base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
	base = (base & 0x03) << 30;
	if (base) base |= 0x00010000;
	return(base);
}

/*-----------------------------------------------------------------------------
 * gfx_get_vip_register_base
 * 
 * This routine returns the base address for the VIP hardware.  This is 
 * only applicable to the SC14000, for which this routine assumes an offset 
 * of 0x00050000 from the base address specified by the GCR.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *-----------------------------------------------------------------------------
 */
unsigned long gfx_get_vip_register_base(void)
{
	unsigned long base = 0;
	if (((gfx_cpu_version & 0xFF) == GFX_CPU_SC1400) ||
		((gfx_cpu_version & 0xFF) == GFX_CPU_SC1200))
	{
		base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
		base = (base & 0x03) << 30;
		if (base) base |= 0x00050000;
	}
	return(base);
}

/* END OF FILE */


