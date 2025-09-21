/* :ts=8 bk=0
 *
 * dev.c:	Device-specific hooks.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.08.22
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <device/graphic_driver.h>
#include <interface/GraphicsDefs.h>
#include <dinky/bena4.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/debug.h>

#include <stdlib.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * Globals.
 */
extern pci_module_info	*gpci_bus;
extern genpool_module	*ggpm;
extern driverdata	*gdd;


/****************************************************************************
 * Open an instance of the device.
 */
static status_t
open_hook (const char *name, uint32 flags, void **cookie)
{
	devinfo		*di;
	status_t	retval = B_OK;
	int		n;

dprintf (("+++ open_hook(%s, %ld, 0x%08lx)\n", name, flags, (uint32)cookie));

	BLockBena4 (&gdd->dd_DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0;
	     gdd->dd_DevNames[n]  &&  strcmp (name, gdd->dd_DevNames[n]);
	     n++)
		;
	di = &gdd->dd_DI[n];

	if (di->di_Opened == 0) {
		if ((retval = firstopen (di)) != B_OK)
			goto err;	/*  Look down  */
	}

	/*  Mark the device open.  */
	di->di_Opened++;

	/*  Squirrel this away.  */
	*cookie = di;
	
	/*  End of critical section.  */
err:	BUnlockBena4 (&gdd->dd_DriverLock);

	/*  All done, return the status.  */
dprintf (("+++ open_hook returning 0x%08lx\n", retval));
	return (retval);
}

static status_t
close_hook (void *dev)
{
	dprintf(("+++ close_hook(%08lx)\n", (uint32)dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/*
 * free_hook - close down the device
 */
static status_t
free_hook (void *dev)
{
	devinfo		*di = dev;

dprintf (("+++ free_hook() begins...\n"));

	BLockBena4 (&gdd->dd_DriverLock);

	/*  Mark the device available.  */
	if (--di->di_Opened == 0)
		lastclose (di);

	BUnlockBena4 (&gdd->dd_DriverLock);
dprintf (("+++ free_hook() completes.\n"));

	return (B_OK);
}


/*
 * Read, write, and close hooks.  They do nothing gracefully.  Or rather, they
 * gracefully do nothing.  Whatever...
 */
static status_t
read_hook (void *dev, off_t pos, void *buf, size_t *len)
{
	(void)dev; (void)pos; (void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}

static status_t
write_hook (void *dev, off_t pos, const void *buf, size_t *len)
{
	(void)dev; (void)pos; (void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}

/*
 * control_hook - where the real work is done
 */
static status_t
control_hook (void *dev, uint32 msg, void *buf, size_t len)
{
	devinfo		*di = dev;
	gfx_card_info	*ci = di->di_GfxCardInfo;
	status_t	retval = B_DEV_INVALID_IOCTL;

	(void)len;

//if (msg != MAXIOCTL_GDRV + B_SET_CURSOR_SHAPE  &&
//    msg != MAXIOCTL_GDRV + B_SHOW_CURSOR)
//     dprintf (("+++ ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

	switch (msg) {
	/*
	 * Stuff the accelerant needs to do business.
	 */
	/*
	 * The only PUBLIC ioctl.
	 */
	case B_GET_ACCELERANT_SIGNATURE:
		strcpy ((char *) buf, "nvidia.accelerant");
		retval = B_OK;
		break;
	
	/*
	 * PRIVATE ioctl from here on.
	 */
	case GDRV_IOCTL_GETGLOBALS:
#define	GG	((gdrv_getglobals *) buf)

		/*
		 * Check protocol version to see if client knows how to talk
		 * to us.
		 */
		if (GG->gg_ProtocolVersion == NVIDIA_IOCTLPROTOCOL_VERSION) {
			GG->gg_GlobalArea_CI = di->di_GfxCardInfo_AreaID;
			GG->gg_GlobalArea_CC = di->di_GfxCardCtl_AreaID;
			retval = B_OK;
		}
		break;

	case GDRV_IOCTL_GET_PCI:
#define	GSP	((gdrv_getsetpci *) buf)

		GSP->gsp_Value = get_pci (&di->di_PCI,
					  GSP->gsp_Offset,
					  GSP->gsp_Size);
		retval = B_OK;
		break;

	case GDRV_IOCTL_SET_PCI:
		set_pci (&di->di_PCI, GSP->gsp_Offset,
			 GSP->gsp_Size, GSP->gsp_Value);
		retval = B_OK;
		break;


	/*
	 * Accelerant operations which are, on balance, better handled inside
	 * the kernel device.  (Note the cutesy method of assigning ioctl 'msg'
	 * values.)
	 */
	/*  Mode configuration  */
	case MAXIOCTL_GDRV + B_ACCELERANT_MODE_COUNT:
		*(uint32 *) buf = di->di_NDispModes;
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_GET_MODE_LIST:
		memcpy (buf,
			di->di_DispModes,
			di->di_NDispModes * sizeof (display_mode));
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_PROPOSE_DISPLAY_MODE:
#define	PDM	((gdrv_propose_display_mode *) buf)

		retval = propose_video_mode
			  (di, PDM->pdm_Target, PDM->pdm_Lo, PDM->pdm_Hi);
		break;

	case MAXIOCTL_GDRV + B_SET_DISPLAY_MODE:
		if ((retval = vid_selectmode
			       (di, ci, (display_mode *) buf)) == B_OK)
		{
			memcpy (&ci->ci_CurDispMode, buf,
				sizeof (ci->ci_CurDispMode));
#if 0
dprintf (("+++ Just set mode %d*%d*%d:\n",
 ci->ci_CurDispMode.timing.h_display,
 ci->ci_CurDispMode.timing.v_display,
 ci->ci_Depth));
dprintf (("+++  [HV]Total: %d, %d  Pixel clock: %d KHz\n",
 ci->ci_CurDispMode.timing.h_total,
 ci->ci_CurDispMode.timing.v_total,
 ci->ci_CurDispMode.timing.pixel_clock));
dprintf (("+++  Calculated refresh rate: ~%d Hz\n",
 ci->ci_CurDispMode.timing.pixel_clock * 1000 / 
 (ci->ci_CurDispMode.timing.h_total * ci->ci_CurDispMode.timing.v_total)));
#endif
		}
		break;

	case MAXIOCTL_GDRV + B_GET_FRAME_BUFFER_CONFIG:
#define	FBI	((frame_buffer_config *) buf)

		FBI->frame_buffer =
		 (void *) (ci->ci_FBBase + ci->ci_BaseAddr1);
		FBI->frame_buffer_dma =
		 (void *) (ci->ci_FBBase + ci->ci_BaseAddr1_DMA);
		FBI->bytes_per_row = ci->ci_BytesPerRow;
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_GET_PIXEL_CLOCK_LIMITS:
	 {
#define	PCL	((gdrv_pixel_clock_limits *) buf)
		register uint32	total_pix;

		/*
		 * Constrain low-end to 48 Hz, until a monitors database
		 * shows up...
		 */
		total_pix = PCL->pcl_DisplayMode->timing.h_total * 
			    PCL->pcl_DisplayMode->timing.v_total;
		PCL->pcl_Lo = (total_pix * 48L) / 1000L;
		PCL->pcl_Hi = ci->ci_MaxPixClkKHz;
		retval = B_OK;
		break;
	 }

	case MAXIOCTL_GDRV + B_MOVE_DISPLAY:
		/*
		 * This is only called by the accelerant if interrupts aren't
		 * enabled on the card.
		 */
		(ci->ci_HWInst.SetStartAddress)
		 (&ci->ci_HWInst,
		  ci->ci_FBBase + di->di_GfxCardCtl->cc_ModeRegs.mr_FBULOffset);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_SET_INDEXED_COLORS:
	 {
#define	SIC	((gdrv_set_indexed_colors *) buf)	
		register int	count, idx;
		register uint8	*color;

		/*
		 * NVidia chips can optionally display 16- and 32-bit pixels
		 * in "3DO mode" (long story), which basically requires that
		 * a simple ramp palette be installed when 16- and 32-bit
		 * modes are active.  We therefore deliberately ignore
		 * requests to set palette entries when not in 8-bit mode.
		 */
		if (ci->ci_CurDispMode.space == B_CMAP8) {
			acquire_sem (ci->ci_CLUTLock);

			count = SIC->sic_Count;
			color = SIC->sic_ColorData;
			idx = SIC->sic_First;

			while (count--) {
				setpaletteentry (NULL,
						 idx++,
						 color[0],
						 color[1],
						 color[2]);
				color += 3;
			}

			release_sem (ci->ci_CLUTLock);
		}
		break;
	 }


	/*  DPMS management  */
	case MAXIOCTL_GDRV + B_DPMS_MODE:
		/*  See nv3ref.h or nv4ref.h to see what I'm doing.  */
		switch (di->di_GfxCardCtl->cc_ModeRegs.mr_HWState.repaint1 &
		        0xc0)
		{
		case 0x00:
			*(uint32 *) buf = B_DPMS_ON;
			break;
		case 0x80:
			*(uint32 *) buf = B_DPMS_STAND_BY;
			break;
		case 0x40:
			*(uint32 *) buf = B_DPMS_SUSPEND;
			break;
		case 0xc0:
			*(uint32 *) buf = B_DPMS_OFF;
			break;
		}
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_SET_DPMS_MODE:
	 {
		RIVA_HW_STATE	*hws;

		hws = &di->di_GfxCardCtl->cc_ModeRegs.mr_HWState;
		hws->repaint1 &= ~0xc0;

		switch (*(uint32 *) buf) {
		case B_DPMS_ON:		// H:  on, V:  on
			hws->repaint1 |= 0x00;
			screen_on (NULL);
			break;
		case B_DPMS_STAND_BY:	// H: off, V:  on
			hws->repaint1 |= 0x80;
			screen_off (NULL);
			break;
		case B_DPMS_SUSPEND:	// H:  on, V: off
			hws->repaint1 |= 0x40;
			screen_off (NULL);
			break;
		case B_DPMS_OFF:	// H: off, V: off
			hws->repaint1 |= 0xc0;
			screen_off (NULL);
			break;
		}
		outw (0x3d4, (hws->repaint1 << 8) | 0x1a);
		retval = B_OK;
		break;
	 }


	/*  Cursor managment  */
	case MAXIOCTL_GDRV + B_SET_CURSOR_SHAPE:
#define	SCS	((gdrv_set_cursor_shape *) buf)

		retval = setcursorshape (di,
					 SCS->scs_XORMask, SCS->scs_ANDMask,
					 SCS->scs_Width, SCS->scs_Height,
					 SCS->scs_HotX, SCS->scs_HotY);
		break;

	case MAXIOCTL_GDRV + B_SHOW_CURSOR:
		showcursor (ci, *(bool *) buf);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_MOVE_CURSOR:
		/*
		 * This is only called by the accelerant if interrupts aren't
		 * enabled on the card.
		 */
		movecursor (di,
			    di->di_GfxCardCtl->cc_MousePosX,
			    di->di_GfxCardCtl->cc_MousePosY);
		retval = B_OK;
		break;


	/*  Overlay management  */
	case MAXIOCTL_GDRV + B_ALLOCATE_OVERLAY_BUFFER:
#define	OB	((overlay_buffer *) buf)

		/*  Index of allocated buffer returned in OB->bytes_per_row  */
		OB->bytes_per_row = allocateoverlaybuffer
		                     (di, OB->space, OB->width, OB->height);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_RELEASE_OVERLAY_BUFFER:
		retval = releaseoverlaybuffer (di, (uint32) buf);
		break;

	case MAXIOCTL_GDRV + B_ALLOCATE_OVERLAY:
		*(overlay_token *) buf = allocateoverlay (ci);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_RELEASE_OVERLAY:
		retval = releaseoverlay (ci, *(overlay_token *) buf);
		break;

	case MAXIOCTL_GDRV + B_CONFIGURE_OVERLAY:
#define	CO	((gdrv_configure_overlay *) buf)

		retval = configureoverlay
		          (ci,
		           CO->co_Token,
		           (CO->co_Flags & CONFIGOVF_BUFVALID)
		            ? &CO->co_Buffer : NULL,
		           (CO->co_Flags & CONFIGOVF_WINVALID)
		            ? &CO->co_Window : NULL,
		           (CO->co_Flags & CONFIGOVF_VIEWVALID)
		            ? &CO->co_View : NULL);
		break;


#if 0
	/*
	 * Unimplemented operations; handled either in the accelerant or not
	 * at all.
	 */
	/*  Initialization  */
	case MAXIOCTL_GDRV + B_GET_CONFIG_INFO_SIZE:
	case MAXIOCTL_GDRV + B_GET_CONFIG_INFO:
	case MAXIOCTL_GDRV + B_SET_CONFIG_INFO:

	/*  Mode configuration  */
	case MAXIOCTL_GDRV + B_GET_DISPLAY_MODE:
	case MAXIOCTL_GDRV + B_SET_GAMMA:

	/*  Cursor managment  */
	case MAXIOCTL_GDRV + B_MOVE_CURSOR:

	/*  Synchronization  */
	case MAXIOCTL_GDRV + B_ACCELERANT_ENGINE_COUNT:
	case MAXIOCTL_GDRV + B_ACQUIRE_ENGINE:
	case MAXIOCTL_GDRV + B_RELEASE_ENGINE:
	case MAXIOCTL_GDRV + B_WAIT_ENGINE_IDLE:
	case MAXIOCTL_GDRV + B_GET_SYNC_TOKEN:
	case MAXIOCTL_GDRV + B_SYNC_TO_TOKEN:

	/*  2D acceleration  */
	case MAXIOCTL_GDRV + B_SCREEN_TO_SCREEN_BLIT:
	case MAXIOCTL_GDRV + B_FILL_RECTANGLE:
	case MAXIOCTL_GDRV + B_INVERT_RECTANGLE:
	case MAXIOCTL_GDRV + B_FILL_SPAN:
	
	/*  3D acceleration  */
	case MAXIOCTL_GDRV + B_ACCELERANT_PRIVATE_START:
#endif

	default:
		break;
	}
	return (retval);
}


/****************************************************************************
 * Device hook table.
 */
device_hooks	graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};
