/* :ts=8 bk=0
 *
 * dev.c:	Device-specific hooks.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <interface/GraphicsDefs.h>
#include <drivers/genpool_module.h>
#include <device/graphic_driver.h>
#include <dinky/bena4.h>
#include <stdlib.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>
#include <graphics_p/i810/gfxbuf.h>

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
	openerinfo	*oi;
	status_t	retval = B_OK;
	int		n;

dprintf (("+++ open_hook(%s, %ld, 0x%08lx)\n", name, flags, (uint32) cookie));

	if (!(oi = malloc (sizeof (*oi))))
		return (B_NO_MEMORY);

	acquire_sem (gdd->dd_DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0;
	     gdd->dd_DevNames[n]  &&  strcmp (name, gdd->dd_DevNames[n]);
	     n++)
		;
	di = &gdd->dd_DI[n];

	if (di->di_Opened == 0) {
		/*  Create and initialize per-device data.  */
		if ((retval = firstopen (di)) != B_OK)
			goto err;	/*  Look down  */
	}

	/*  Mark the device open.  */
	di->di_Opened++;

	/*  Fill out opener info.  */
	INITNODE (&oi->oi_Node);
	memcpy (&oi->oi_DC,
	        &di->di_GfxCardInfo->ci_DispConfig,
	        sizeof (dispconfig));
	oi->oi_DI		= di;
	oi->oi_GenPoolOwnerID	= (ggpm->gpm_GetUniqueOwnerID) ();

	/*  Squirrel this away.  */
	*cookie = oi;

	/*  End of critical section.  */
err:	if (retval < 0)
		free (oi);
	release_sem (gdd->dd_DriverLock);

	/*  All done, return the status.  */
dprintf (("+++ open_hook returning 0x%08lx\n", retval));
	return (retval);
}

static status_t
close_hook (void *dev)
{
	dprintf(("+++ close_hook(%08lx)\n", (uint32) dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/*
 * free_hook - close down the device
 */
static status_t
free_hook (void *dev)
{
	devinfo		*di;
	openerinfo	*oi = dev;

dprintf (("+++ free_hook() begins...\n"));

	acquire_sem (gdd->dd_DriverLock);
	di = oi->oi_DI;

	/*  Free everything they allocated.  */
	(ggpm->gpm_UnmarkRangesOwnedBy) (di->di_GfxCardInfo->ci_PoolID,
	                                 oi->oi_GenPoolOwnerID);

	/*  Mark the device available.  */
	if (--di->di_Opened == 0)
		lastclose (di);
	else {
		gfx_card_info	*ci;
		openerinfo	*headopen;

		ci = di->di_GfxCardInfo;
		headopen = (openerinfo *) FIRSTNODE (&ci->ci_OpenerList);
		RemNode (&oi->oi_Node);
		if (headopen == oi) {
			/*
			 * This opener was the owner of the display.
			 * Change display to next head opener.  First check
			 * to make sure they're really different...
			 */
			if (memcmp (&oi->oi_DC,
			            &ci->ci_DispConfig,
			            sizeof (dispconfig)))
			{
dprintf (("+++ Restoring mode of previous opener.\n"));
				/*  Yes, they're really different.  */
				headopen = (openerinfo *)
				           FIRSTNODE (&ci->ci_OpenerList);
				if (NEXTNODE (headopen)) {
/*- - - - - - - - - - - - - - - - - - -*/
/*  FIXME: And if these fail...?  */
acquire_sem (ci->ci_DispConfigLock);
acquire_sem (ci->ci_VGARegsLock);
calcshowcursor (&headopen->oi_DC, di, FALSE);
writemode (&headopen->oi_DC, di, ci);
release_sem (ci->ci_VGARegsLock);
release_sem (ci->ci_DispConfigLock);
/*- - - - - - - - - - - - - - - - - - -*/
				}
			}
		}
	}

	free (oi);

	release_sem (gdd->dd_DriverLock);
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
	(void) dev;
	(void) pos;
	(void) buf;

	*len = 0;
	return B_NOT_ALLOWED;
}

static status_t
write_hook (void *dev, off_t pos, const void *buf, size_t *len)
{
	(void) dev;
	(void) pos;
	(void) buf;

	*len = 0;
	return B_NOT_ALLOWED;
}

/*
 * control_hook - where the real work is done
 */
static status_t
control_hook (void *dev, uint32 msg, void *buf, size_t len)
{
	devinfo		*di;
	openerinfo	*oi = dev;
	gfx_card_info	*ci;
	status_t	retval = B_DEV_INVALID_IOCTL;

//if (msg != MAXIOCTL_GDRV + B_SET_CURSOR_SHAPE  &&
//    msg != MAXIOCTL_GDRV + B_SHOW_CURSOR)
//     dprintf (("+++ ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

	di = oi->oi_DI;
	ci = di->di_GfxCardInfo;

	/*  Handle GfxBuf requests.  */
	if (msg >= GFX_IOCTL_ALLOCBYMEMSPEC  &&  msg < MAXIOCTL_GFXBUF)
		return (control_gfxbuf (oi, msg, buf, len));

	switch (msg) {
	/*
	 * Stuff the accelerant needs to do business.
	 */
	/*
	 * The only PUBLIC ioctl.
	 */
	case B_GET_ACCELERANT_SIGNATURE:
		strcpy ((char *) buf, "i810.accelerant");
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
		if (GG->gg_ProtocolVersion == I810_IOCTLPROTOCOL_VERSION) {
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

	case GDRV_IOCTL_SET_DISPMODE:
		retval = setopenertomode (oi, ci, (display_mode *) buf);
#if 0
dprintf (("+++ Just set mode %d*%d*%d:\n",
 ci->ci_DispConfig.dc_DispMode.timing.h_display,
 ci->ci_DispConfig.dc_DispMode.timing.v_display,
 ci->ci_DispConfig.dc_Depth));
dprintf (("+++  [HV]Total: %d, %d  Pixel clock: %d KHz\n",
 ci->ci_DispConfig.dc_DispMode.timing.h_total,
 ci->ci_DispConfig.dc_DispMode.timing.v_total,
 ci->ci_DispConfig.dc_DispMode.timing.pixel_clock));
dprintf (("+++  Calculated refresh rate: ~%d Hz\n",
 ci->ci_DispConfig.dc_DispMode.timing.pixel_clock * 1000 / 
 (ci->ci_DispConfig.dc_DispMode.timing.h_total *
  ci->ci_DispConfig.dc_DispMode.timing.v_total)));
#endif
		break;

	case GDRV_IOCTL_SET_FBBASE:
		oi->oi_DC.dc_FBBase = *(uint32 *) buf;
		setopenerfbbaseregs (oi);
		retval = B_OK;
		break;

	case I810DRV_ADDFENCE:
	 {	register int	i;

		acquire_sem (ci->ci_OtherRegsLock);
		for (i = 8;  --i >= 0; ) {
			if (!ci->ci_Regs->FENCE[i]) {
				ci->ci_Regs->FENCE[i] = (uint32) buf;
				break;
			}
		}
		release_sem (ci->ci_OtherRegsLock);
		if (i < 0)	retval = B_NO_MEMORY;
		else		retval = B_OK;
		break;
	 }

	case I810DRV_REMFENCE:
	 {	register int	i;

		acquire_sem (ci->ci_OtherRegsLock);
		for (i = 8;  --i >= 0; ) {
			if (ci->ci_Regs->FENCE[i] == (uint32) buf) {
				ci->ci_Regs->FENCE[i] = 0;
				break;
			}
		}
		release_sem (ci->ci_OtherRegsLock);
		retval = B_OK;
		break;
	 }

	case I810DRV_SET_GL_DISPMODE:
		retval = setopenertoGLmode
		          (oi, ci, (i810drv_set_gl_disp *) buf);
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

	case MAXIOCTL_GDRV + B_GET_FRAME_BUFFER_CONFIG:
#define	FBI	((frame_buffer_config *) buf)

		FBI->frame_buffer =
		 (void *) di->di_GfxCardCtl->cc_DispBuf.gb_BaseAddr;
		FBI->frame_buffer_dma =
		 (void *) di->di_GfxCardCtl->cc_DispBuf.gb_BaseAddr_PCI;
		FBI->bytes_per_row =
		 di->di_GfxCardCtl->cc_DispBuf.gb_BytesPerRow;
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
		PCL->pcl_Hi = CLOCK_MAX;
		retval = B_OK;
		break;
	 }

	case MAXIOCTL_GDRV + B_MOVE_DISPLAY:
#define	MD	((gdrv_move_display *) buf)

		/*
		 * It is intuitively obvious to the experienced observer that
		 * the correct openerinfo on which to operate is the current
		 * one ('oi').
		 *
		 * Well, guess what:  This doesn't work for game kit apps.
		 * When a game kit app launches, a new opener appears and
		 * gets a clone of the current Tracker desktop.  However,
		 * when it requests a mode change, it gets performed by the
		 * *app_server*, not the game kit client.  So the game kit
		 * client sits there with an openerinfo still thinking we
		 * have Tracker dimensions, and the app_server has
		 * precedence, anyway, since it was last to mess with the
		 * display mode.  *sigh*  So we have to operate on the
		 * current display configuration instead, which is the
		 * top-most node on the ci_OpenerList.
		 */
		retval =
		 movedisplay ((openerinfo *) FIRSTNODE (&ci->ci_OpenerList),
		              MD->md_HDisplayStart,
		              MD->md_VDisplayStart);
		break;
#undef MD

	case MAXIOCTL_GDRV + B_SET_INDEXED_COLORS:
	 {
#define	SIC	((gdrv_set_indexed_colors *) buf)	
		int	count, first;
		uint8	*color;

		/*
		 * The I810 can optionally display 16- and 32-bit pixels in
		 * "3DO mode" (long story), which basically requires that a
		 * simple ramp palette be installed when 16- and 32-bit
		 * modes are active.  We therefore deliberately ignore
		 * requests to set palette entries when not in 8-bit mode.
		 */
		if (ci->ci_DispConfig.dc_DispMode.space == B_CMAP8) {
			count = SIC->sic_Count;
			color = SIC->sic_ColorData;
			first = SIC->sic_First;

			memcpy (oi->oi_DC.dc_CMAP[first], color, count * 3);
			
			if (isowner (oi)) {
				acquire_sem (ci->ci_VGARegsLock);
				writepaletterange (&ci->ci_Regs->VGARegs,
				                   oi->oi_DC.dc_CMAP[0],
				                   first,
				                   first + count - 1);
				release_sem (ci->ci_VGARegsLock);
			}
		}
		retval = B_OK;
		break;
#undef SIC
	 }


	/*  DPMS management  */
	case MAXIOCTL_GDRV + B_DPMS_MODE:
		switch (ci->ci_Regs->HVSYNC & (MASKFIELD (HVSYNC, VCTL) |
					       MASKFIELD (HVSYNC, HCTL)))
		{
		case DEF2FIELD (HVSYNC, HCTL, NORMAL) |
		     DEF2FIELD (HVSYNC, VCTL, NORMAL):
			*(uint32 *) buf = B_DPMS_ON;
			break;
		case DEF2FIELD (HVSYNC, HCTL, DATA) |
		     DEF2FIELD (HVSYNC, VCTL, NORMAL):
			*(uint32 *) buf = B_DPMS_STAND_BY;
			break;
		case DEF2FIELD (HVSYNC, HCTL, NORMAL) |
		     DEF2FIELD (HVSYNC, VCTL, DATA):
			*(uint32 *) buf = B_DPMS_SUSPEND;
			break;
		case DEF2FIELD (HVSYNC, HCTL, DATA) |
		     DEF2FIELD (HVSYNC, VCTL, DATA):
			*(uint32 *) buf = B_DPMS_OFF;
			break;
		}
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_SET_DPMS_MODE:
	 {
		int	hpol, vpol;
		uint32	dpmsctl;

		hpol = (ci->ci_DispConfig.dc_DispMode.timing.flags
		       & B_POSITIVE_HSYNC) != 0;
		vpol = (ci->ci_DispConfig.dc_DispMode.timing.flags
		       & B_POSITIVE_VSYNC) != 0;
		
		dpmsctl = ci->ci_Regs->HVSYNC &
		 	  ~(MASKFIELD (HVSYNC, HCTL) |
			    MASKFIELD (HVSYNC, HDATA) |
			    MASKFIELD (HVSYNC, VCTL) |
			    MASKFIELD (HVSYNC, VDATA));
		dpmsctl |= VAL2FIELD (HVSYNC, HDATA, hpol) |
			   VAL2FIELD (HVSYNC, VDATA, vpol);

		switch (*(uint32 *) buf) {
		case B_DPMS_ON:		// H:  on, V:  on
			dpmsctl |=
			 DEF2FIELD (HVSYNC, HCTL, NORMAL) |
			 DEF2FIELD (HVSYNC, VCTL, NORMAL);
			screen_on (&ci->ci_Regs->VGARegs);
			break;
		case B_DPMS_STAND_BY:	// H: off, V:  on
			dpmsctl |=
			 DEF2FIELD (HVSYNC, HCTL, DATA) |
			 DEF2FIELD (HVSYNC, VCTL, NORMAL);
			screen_off (&ci->ci_Regs->VGARegs);
			break;
		case B_DPMS_SUSPEND:	// H:  on, V: off
			dpmsctl |=
			 DEF2FIELD (HVSYNC, HCTL, NORMAL) |
			 DEF2FIELD (HVSYNC, VCTL, DATA);
			screen_off (&ci->ci_Regs->VGARegs);
			break;
		case B_DPMS_OFF:	// H: off, V: off
			dpmsctl |=
			 DEF2FIELD (HVSYNC, HCTL, DATA) |
			 DEF2FIELD (HVSYNC, VCTL, DATA);
			screen_off (&ci->ci_Regs->VGARegs);
			break;
		}
		ci->ci_Regs->HVSYNC = dpmsctl;
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
		di->di_GfxCardCtl->cc_CursorState.cs_Enabled = *(bool *) buf;
		calcshowcursor (&ci->ci_DispConfig, di, TRUE);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_MOVE_CURSOR:
		/*
		 * This is only called by the accelerant if interrupts aren't
		 * enabled on the card.
		 */
		movecursor (di,
			    di->di_GfxCardCtl->cc_CursorState.cs_X,
			    di->di_GfxCardCtl->cc_CursorState.cs_Y);
		retval = B_OK;
		break;


	/*  Overlay management  */
	case MAXIOCTL_GDRV + B_ALLOCATE_OVERLAY_BUFFER:
#define	OB	((overlay_buffer *) buf)

		/*  Index of allocated buffer returned in OB->bytes_per_row  */
		OB->bytes_per_row = allocateoverlaybuffer
		                     (oi, OB->space, OB->width, OB->height);
		retval = B_OK;
		break;

	case MAXIOCTL_GDRV + B_RELEASE_OVERLAY_BUFFER:
		retval = releaseoverlaybuffer (oi, (uint32) buf);
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

		retval = configureoverlay (ci,
		                           CO->co_Token,
		                           &CO->co_Buffer,
		                           &CO->co_Window,
		                           &CO->co_View);
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
	case MAXIOCTL_GDRV + B_MOVE_DISPLAY:
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
