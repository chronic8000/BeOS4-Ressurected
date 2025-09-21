/* :ts=8 bk=0
 *
 * overlay.c:	Common header file.
 *
 * Leo L. Schwab					2001.07.05
 *  based on XFree86 4.1.0 code
 *
 * Copyright 2001 Be, Incorporated.
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <math.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/riva_hw.h>
#include <graphics_p/nvidia/debug.h>

#include "driver.h"
#include "protos.h"


static void stopoverlay (struct gfx_card_info *ci);


/*****************************************************************************
 * Globals.
 */
extern genpool_module	*ggpm;


/*****************************************************************************
 * Code.
 */
uint32
allocateoverlaybuffer (
struct devinfo		*di,
color_space		cs,
uint16			width,
uint16			height
)
{
	gfx_card_info	*ci;
	BMemSpec	*ms;
	overlay_buffer	*ovb;
	uint32		idx, retval;
	int		i, rowbytes;

	if (!width  ||  !height  ||  cs == B_NO_COLOR_SPACE)
		return (~0);

	ci = di->di_GfxCardInfo;

	acquire_sem (ci->ci_OverlayLock);
	retval = ~0;

	/*  Search for free overlay buffer.  */
	for (idx = 0, ovb = ci->ci_OverlayBuffers, ms = ci->ci_OverlayAllocs;
	     idx < NUMOF (ci->ci_OverlayBuffers);
	     idx++, ovb++, ms++)
		if (ovb->space == B_NO_COLOR_SPACE)
			break;
	if (idx >= NUMOF (ci->ci_OverlayBuffers))
		goto ackphft;	/*  Look down  */

	/*
	 * Validate color space.  Calculate buffer size and allocate.
	 */
	switch (cs) {
	case B_YCbCr422:
		i = 2;
		break;
	default:
		/*  Unsupported color space.  */
		goto ackphft;	/*  Look down.  */
	}

	rowbytes = ((width * i) + 63) & ~63;	// FIXME: correct for nv?
	width = rowbytes / i;
	ms->ms_MR.mr_Size	= rowbytes * height;
	ms->ms_MR.mr_Owner	= 0;
	ms->ms_PoolID		= ci->ci_PoolID;
	ms->ms_AddrCareBits	= 7;
	ms->ms_AddrStateBits	= 0;
	ms->ms_AllocFlags	= 0;
	if ((ggpm->gpm_AllocByMemSpec) (ms) < 0)
		goto ackphft;

	/*  Fill out overlay_buffer info.  */	
	ovb->space		= cs;
	ovb->width		= width;
	ovb->height		= height;
	ovb->bytes_per_row	= rowbytes;
	ovb->buffer		= (void *) (ci->ci_BaseAddr1
				           + ms->ms_MR.mr_Offset);
	ovb->buffer_dma		= (void *) (ci->ci_BaseAddr1_DMA
				           + ms->ms_MR.mr_Offset);

	retval = idx;
ackphft:
	release_sem (ci->ci_OverlayLock);
	return (retval);
}

status_t
releaseoverlaybuffer (
struct devinfo		*di,
uint32			idx
)
{
	gfx_card_info	*ci;
	overlay_buffer	*ovb;
	status_t	retval;

	ci = di->di_GfxCardInfo;

	if (idx >= NUMOF (ci->ci_OverlayBuffers))
		/*  Invalid buffer index.  */
		return (B_ERROR);

dprintf (("+++ releaseoverlaybuffer()\n"));
	acquire_sem (ci->ci_OverlayLock);
	retval = B_ERROR;

	ovb = &ci->ci_OverlayBuffers[idx];
	if (ovb->space == B_NO_COLOR_SPACE)
		/*  Already freed/unallocated  */
		goto ackphft;	/*  Look down  */

	/*  Free the overlay buffer and mark as free.  */
	if ((retval = (ggpm->gpm_FreeByMemSpec)
	               (&ci->ci_OverlayAllocs[idx])) < 0)
		goto ackphft;
	
	ovb->space	= B_NO_COLOR_SPACE;
	ovb->buffer	=
	ovb->buffer_dma	= NULL;

	retval = B_OK;
ackphft:
	release_sem (ci->ci_OverlayLock);
dprintf (("+++ releaseoverlaybuffer() done\n"));
	return (retval);
}

overlay_token
allocateoverlay (struct gfx_card_info *ci)
{
	nv_overlay	*ovl, *retval;
	int		i;

	acquire_sem (ci->ci_OverlayLock);
	retval = NULL;

	for (i = 0, ovl = ci->ci_ActiveOverlays;
	     i < NUMOF (ci->ci_ActiveOverlays);
	     i++)
		if (!ovl->ovl_InUse)
			break;
	if (i < NUMOF (ci->ci_ActiveOverlays)) {
		ovl->ovl_InUse = TRUE;
		retval = ovl;
	}

	release_sem (ci->ci_OverlayLock);
	return ((overlay_token) retval);
}


status_t
releaseoverlay (struct gfx_card_info *ci, overlay_token ot)
{
	nv_overlay	*ovl;
	status_t	retval;
	int		i;

	ovl = (nv_overlay *) ot;
	i = ovl - ci->ci_ActiveOverlays;
	if (i < 0  ||  i >= NUMOF (ci->ci_ActiveOverlays))
		/*  Invalid token.  */
		return (B_ERROR);

dprintf (("+++ releaseoverlay()\n"));
	acquire_sem (ci->ci_OverlayLock);
	retval = B_ERROR;

	if (ovl->ovl_InUse) {
		ovl->ovl_InUse = FALSE;
		stopoverlay (ci);
		retval = B_OK;
	}

	release_sem (ci->ci_OverlayLock);
dprintf (("+++ releaseoverlay() done\n"));
	return (retval);
}

status_t
configureoverlay (
struct gfx_card_info	*ci,
overlay_token		ot,
const overlay_buffer	*ob,
const overlay_window	*ow,
const overlay_view	*ov
)
{
	nv_overlay	*ovl;
	status_t	retval;
	float		hscale, vscale;
	uint32		baseaddr;
	uint32		destpitch;
	uint32		keyval;
	int32		deskl, deskr, deskt, deskb;
	int32		bcropl, bcropr, bcropt, bcropb;
	int32		wcropl, wcropr, wcropt, wcropb;
	int32		wr, wb;
	int		i, buffer, pixbytes, isYUV42x = FALSE;

	/*
	 * FIXME: Try and detect if the only change to the overlay is the
	 * bitmap pointer, and short-circuit all the intervening mish-mash
	 * that hasn't changed.
	 */
	ovl = (nv_overlay *) ot;
	i = ovl - ci->ci_ActiveOverlays;
	if (i < 0  ||  i >= NUMOF (ci->ci_ActiveOverlays))
		/*  Invalid token.  */
		return (B_ERROR);

	if (!ob)	ob = ovl->ovl_Buffer;
	if (!ow)	ow = &ovl->ovl_Window;
	if (!ov)	ov = &ovl->ovl_View;

	acquire_sem (ci->ci_OverlayLock);
	retval = B_ERROR;

	if (!ovl->ovl_InUse)
		/*  Not allocated.  */
		goto ackphft;	/*  Look down  */

	if (!ob->width  &&  !ob->height) {
		/*  Size reduced to nothing; shut overlay off.  */
		stopoverlay (ci);
		goto savesettings;	/*  Look down  */
	}

	/*  Check color space.  */
	switch (ob->space) {
	case B_YCbCr422:
		pixbytes = 2;
		isYUV42x = TRUE;
		break;
	default:
		/*  Bad color space.  */
		goto ackphft;	/*  Look down  */
	}

	/*  Check view boundaries.  */
	if (ov->width + ov->h_start > ob->width  ||
	    ov->height + ov->v_start > ob->height)
		goto ackphft;	/*  Look down  */

	/*  Check window location on display.  */
	deskl = ci->ci_CurDispMode.h_display_start;
	deskt = ci->ci_CurDispMode.v_display_start;
	deskr = deskl + ci->ci_CurDispMode.timing.h_display;
	deskb = deskt + ci->ci_CurDispMode.timing.v_display;
	wr = ow->h_start + ow->width;
	wb = ow->v_start + ow->height;
	
	if (ow->h_start >= deskr  ||  wr <= deskl  ||
	    ow->v_start >= deskb  ||  wb <= deskt)
	{
		/*  Window completely outside bounds of visible desktop.  */
		stopoverlay (ci);
		goto savesettings;	/*  Look down  */
	}

	/*  "Source" pixels to crop off each edge of overlay_buffer  */
	bcropl = ov->h_start;
	bcropr = ob->width - ov->width - ov->h_start;
	bcropt = ov->v_start;
	bcropb = ob->height - ov->height - ov->v_start;

	/*  "Destination" pixels to crop off each edge of overlay_window  */
	wcropl = ((int32) ow->h_start + ow->offset_left < deskl)
	         ? deskl - (int32) ow->h_start
	         : ow->offset_left;
	wcropt = ((int32) ow->v_start + ow->offset_top  < deskt)
	         ? deskt - (int32) ow->v_start
	         : ow->offset_top;
	wcropr = (wr - ow->offset_right > deskr)
	         ? wr - deskr
	         : ow->offset_right;
	wcropb = (wb - ow->offset_bottom > deskb)
	         ? wb - deskb
	         : ow->offset_bottom;

	/*  (1 << 16) specifies YCrCb pixels.  */
	destpitch = (1 << 16) | ob->bytes_per_row;

	/*
	 * Color key values
	 * The supplied masks and color values are right-justified (*GRRR!*)
	 */
	if (ow->flags & B_OVERLAY_COLOR_KEY) {
		/*
		 * Colorkey comparison appears to be done pixel-wise on
		 * this chip.
		 */
		switch (ci->ci_Depth) {
		default:
		case 8:
			/*
			 * FIXME: Overlay API needs to be changed to handle
			 * this chip, whose colorkey comparator appears to be
			 * before the DAC, not after it.
			 * Assume use of color register 0xff for now.
			 */
			keyval = 0xff;
			break;
		case 15:
			keyval = (ow->red.value << 10)
			       | (ow->green.value << 5)
			       | ow->blue.value;
			break;
		case 16:
			keyval = (ow->red.value << 11)
			       | (ow->green.value << 5)
			       | ow->blue.value;
			break;
		case 32:
			keyval = ((ow->red.value & ow->red.mask) << 16)
			       | ((ow->green.value & ow->green.mask) << 8)
			       | (ow->blue.value & ow->blue.mask);
			break;
		}
		destpitch |= 1 << 20;	/*  Enable color key  */
	} else {
		keyval = (1 << 24) - 1;
	}

	/*
	 * Scale factors
	 */
	hscale = (float) ov->width / (float) ow->width;
	vscale = (float) ov->height / (float) ow->height;

	/*  "We will control the horizontal..."  */
	bcropl += ((float) wcropl * hscale);
	bcropr += ((float) wcropr * hscale);

	/*  "We will control the vertical..."  */
	bcropt += ((float) wcropt * vscale);
	bcropb += ((float) wcropb * vscale);

	baseaddr = (uint32) ob->buffer_dma - (uint32) ci->ci_BaseAddr1_DMA;

	/*
	 * Defaults taken from XFree86 4.1.0.  Our API doesn't provide
	 * for setting the brightness, contrast, hue, or saturation (which
	 * is what is happening here).
	 */
	ci->ci_HWInst.PMC[0x00008910/4] = (0 << 16) | 4096;
	ci->ci_HWInst.PMC[0x00008914/4] = (0 << 16) | 4096;
	ci->ci_HWInst.PMC[0x00008918/4] = (0 << 16) | (4096 & 0xffff);
	ci->ci_HWInst.PMC[0x0000891C/4] = (0 << 16) | (4096 & 0xffff);

	buffer = 0;	/*  Does NV have two overlay channels?  */
	ci->ci_HWInst.PMC[(0x8900/4) + buffer] =
	 baseaddr;
	ci->ci_HWInst.PMC[(0x8928/4) + buffer] =
	 (ob->height << 16) | ob->width;
	ci->ci_HWInst.PMC[(0x8930/4) + buffer] =
	 ((bcropt & 0xfff) << 20) | ((bcropl & 0xfff) << 4);
	ci->ci_HWInst.PMC[(0x8938/4) + buffer] =
	 (ov->width << 20) / ow->width;
	ci->ci_HWInst.PMC[(0x8940/4) + buffer] =
	 (ov->height << 20) / ow->height;
	ci->ci_HWInst.PMC[(0x8948/4) + buffer] =
	 ((ow->v_start + wcropt - deskt) << 16)
	 | (ow->h_start + wcropl - deskl);
	ci->ci_HWInst.PMC[(0x8950/4) + buffer] =
	 ((ow->height - wcropt - wcropb) << 16)
	 | (ow->width - wcropl - wcropr);
	ci->ci_HWInst.PMC[(0x8958/4) + buffer] = destpitch;

	ci->ci_HWInst.PMC[0x00008b00/4] = keyval;

	ci->ci_HWInst.PMC[0x00008704/4] = 0;
	ci->ci_HWInst.PMC[0x8700/4] = 1 << (buffer << 2);
	
	retval = B_OK;

savesettings:
	ovl->ovl_Window	= *ow;
	ovl->ovl_View	= *ov;
	ovl->ovl_Buffer	= ob;

ackphft:
	release_sem (ci->ci_OverlayLock);
	return (retval);
}

static void
stopoverlay (struct gfx_card_info *ci)
{
	ci->ci_HWInst.PMC[0x00008704/4] = 1;
}


#if 0
/*
 * Original XFree96 4.1.0 code, for reference.
 */
static void
NVResetVideo (ScrnInfoPtr pScrnInfo)
{
	NVPtr          pNv     = NVPTR(pScrnInfo);
	NVPortPrivPtr  pPriv   = GET_OVERLAY_PRIVATE(pNv);
	RIVA_HW_INST  *pRiva   = &(pNv->riva);
	int            satSine, satCosine;
	double         angle;

	angle = (double) pPriv->hue * M_PI / 180.0;

	satSine = pPriv->saturation * sin(angle);
	if (satSine < -1024)
		satSine = -1024;
	satCosine = pPriv->saturation * cos(angle);
	if (satCosine < -1024)
		satCosine = -1024;

	ci->ci_HWInst.PMC[0x00008910/4] = (pPriv->brightness << 16) | pPriv->contrast;
	ci->ci_HWInst.PMC[0x00008914/4] = (pPriv->brightness << 16) | pPriv->contrast;
	ci->ci_HWInst.PMC[0x00008918/4] = (satSine << 16) | (satCosine & 0xffff);
	ci->ci_HWInst.PMC[0x0000891C/4] = (satSine << 16) | (satCosine & 0xffff);
	ci->ci_HWInst.PMC[0x00008b00/4] = pPriv->colorKey;
}

static void
NVPutOverlayImage (
ScrnInfoPtr	pScrnInfo,
int		offset,
int		id,
int		dstPitch,
BoxPtr		dstBox,
int		x1,
int		y1,
int		x2,
int		y2,
short		width,
short		height,
short		src_w,
short		src_h,
short		drw_w,
short		drw_h,
RegionPtr	clipBoxes
)
{
	NVPtr          pNv     = NVPTR(pScrnInfo);
	NVPortPrivPtr  pPriv   = GET_OVERLAY_PRIVATE(pNv);
	RIVA_HW_INST  *pRiva   = &(pNv->riva);
	int buffer = pPriv->currentBuffer;

	/* paint the color key */
	if (pPriv->autopaintColorKey  &&
	    (pPriv->grabbedByV4L || !RegionsEqual(&pPriv->clip, clipBoxes)))
	{
		/* we always paint V4L's color key */
		if(!pPriv->grabbedByV4L)
			REGION_COPY(pScrnInfo->pScreen, &pPriv->clip, clipBoxes);
		(*pNv->AccelInfoRec->FillSolidRects)(pScrnInfo, pPriv->colorKey,
		                                     GXcopy, ~0,
		                                     REGION_NUM_RECTS(clipBoxes),
		                                     REGION_RECTS(clipBoxes));
	}

	pRiva->PMC[(0x8900/4) + buffer] = offset;
	pRiva->PMC[(0x8928/4) + buffer] = (height << 16) | width;
	pRiva->PMC[(0x8930/4) + buffer] = ((y1 << 4) & 0xffff0000) | (x1 >> 12);
	pRiva->PMC[(0x8938/4) + buffer] = (src_w << 20) / drw_w;
	pRiva->PMC[(0x8940/4) + buffer] = (src_h << 20) / drw_h;
	pRiva->PMC[(0x8948/4) + buffer] = (dstBox->y1 << 16) | dstBox->x1;
	pRiva->PMC[(0x8950/4) + buffer] = ((dstBox->y2 - dstBox->y1) << 16)
	                                | (dstBox->x2 - dstBox->x1);
	
	dstPitch |= 1 << 20;   /* use color key */
	
	if(id != FOURCC_UYVY)
		dstPitch |= 1 << 16;
	
	pRiva->PMC[(0x8958/4) + buffer] = dstPitch;
	pRiva->PMC[0x00008704/4] = 0;
	pRiva->PMC[0x8700/4] = 1 << (buffer << 2);
	
	pPriv->videoStatus = CLIENT_VIDEO_ON;
}
#endif
