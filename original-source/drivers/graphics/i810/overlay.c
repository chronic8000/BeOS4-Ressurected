/* :ts=8 bk=0
 *
 * overlay.c:	Overlay support.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.06.26
 *  Conceptually hacked from Trey's Matrox code.
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <add-ons/graphics/GraphicsCard.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/i810defs.h>
#include <graphics_p/i810/gfxbuf.h>
#include <graphics_p/i810/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * Globals.
 */
extern genpool_module	*ggpm;
extern BPoolInfo	*gfbpi;


/*****************************************************************************
 * $(STUFF)
 */
uint32
allocateoverlaybuffer (
struct openerinfo	*oi,
color_space		cs,
uint16			width,
uint16			height
)
{
	gfx_card_info	*ci;
//	BMemSpec	*ms;
	GfxBuf		*gb;
	overlay_buffer	*ovb;
	uint32		idx, retval;

	if (!width  ||  !height  ||  cs == B_NO_COLOR_SPACE)
		return (~0);

	ci = oi->oi_DI->di_GfxCardInfo;

	acquire_sem (ci->ci_OverlayLock);
	retval = ~0;

	/*  Search for free overlay buffer.  */
	for (idx = 0, ovb = ci->ci_OverlayBuffers, gb = ci->ci_OverlayAllocs;
	     idx < NUMOF (ci->ci_OverlayBuffers);
	     idx++, ovb++, gb++)
		if (ovb->space == B_NO_COLOR_SPACE)
			break;
	if (idx >= NUMOF (ci->ci_OverlayBuffers))
		goto ackphft;	/*  Look down  */

#if 0
	/*
	 * Validate color space.  Calculate buffer size and allocate.
	 */
	switch (cs) {
	case B_RGB15:
	case B_RGB16:
		i = 2;
		break;
	case B_YCbCr422:
		i = 2;	/*  Or is it 3?  */
		break;
	default:
		/*  Bad color space.  */
		goto ackphft;	/*  Look down.  */
	}
	
	rowbytes = ((width * i) + 7) & ~7;
	width = rowbytes / i;
	ms->ms_MR.mr_Size	= rowbytes * height;
	ms->ms_MR.mr_Owner	= oi->oi_GenPoolOwnerID;
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
	ovb->buffer		= (void *) (ci->ci_BaseAddr0
				           + ms->ms_MR.mr_Offset);
	ovb->buffer_dma		= (void *) (ci->ci_BaseAddr0_DMA
				           + ms->ms_MR.mr_Offset);
#else
	/*  New method  */
	gb->gb_ColorSpace	= cs;
	gb->gb_Width		= width;
	gb->gb_Height		= height;
	gb->gb_Layout		= GFXBUF_LAYOUT_PACKEDLINEAR;
	gb->gb_Usage		= GFXBUF_USAGE_OVERLAY;
	gb->gb_NMips		= 0;
	gb->gb_ExtraData	= NULL;
	if (allocgfxbuf (oi, gb) < 0)
		goto ackphft;

	/*  Fill out overlay_buffer info.  */	
	ovb->space		= cs;
	ovb->width		= gb->gb_FullWidth;
	ovb->height		= gb->gb_FullHeight;
	ovb->bytes_per_row	= gb->gb_BytesPerRow;
	ovb->buffer		= (void *) gb->gb_BaseAddr;
	ovb->buffer_dma		= (void *) gb->gb_BaseAddr_PCI;
#endif

	retval = idx;
ackphft:
	release_sem (ci->ci_OverlayLock);
	return (retval);
}

status_t
releaseoverlaybuffer (
struct openerinfo	*oi,
uint32			idx
)
{
	gfx_card_info	*ci;
	overlay_buffer	*ovb;
	status_t	retval;

	ci = oi->oi_DI->di_GfxCardInfo;

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
#if 0
	if ((retval = (ggpm->gpm_FreeByMemSpec)
	               (&ci->ci_OverlayAllocs[idx])) < 0)
		goto ackphft;
#else
	if ((retval = freegfxbuf (oi, &ci->ci_OverlayAllocs[idx])) < 0)
		goto ackphft;
#endif
	
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
	i810_overlay	*ovl, *retval;
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
	i810_overlay	*ovl;
	status_t	retval;
	int		i;

	ovl = (i810_overlay *) ot;
	i = ovl - ci->ci_ActiveOverlays;
	if (i < 0  ||  i >= NUMOF (ci->ci_ActiveOverlays))
		/*  Invalid token.  */
		return (B_ERROR);

dprintf (("+++ releaseoverlay()\n"));
	acquire_sem (ci->ci_OverlayLock);
	retval = B_ERROR;

	if (ovl->ovl_InUse) {
		ovl->ovl_InUse = FALSE;
	
		ci->ci_OverlayRegs->OV0CMD =
		 SETVAL2FIELD (ci->ci_OverlayRegs->OV0CMD,
		               OV0CMD, ENABLE, FALSE);
		__asm__ ("wbinvd");
		ci->ci_Regs->OV0ADDR = ci->ci_OverlayRegs_DMA | (1 << 31);
		retval = 1;	/*  Notify outside to flush ci_OverlayRegs  */
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
	i810_overlay	*ovl;
	OverlayRegs	*ovr;
	status_t	retval;
	float		hscale, vscale;
	uint32		srcfmt, ovfilt, uvscale, baseaddr;
	uint32		keyval, keymask;
	int32		hscaleint, hscalefrac, vscaleint, vscalefrac;
	int32		deskl, deskr, deskt, deskb;
	int32		bcropl, bcropr, bcropt, bcropb;
	int32		wcropl, wcropr, wcropt, wcropb;
	int32		wr, wb;
	int		i, pixbytes, isYUV42x = FALSE;

	/*
	 * FIXME: Try and detect if the only change to the overlay is the
	 * bitmap pointer, and short-circuit all the intervening mish-mash
	 * that hasn't changed.
	 */
	ovr = ci->ci_OverlayRegs;

	ovl = (i810_overlay *) ot;
	i = ovl - ci->ci_ActiveOverlays;
	if (i < 0  ||  i >= NUMOF (ci->ci_ActiveOverlays))
		/*  Invalid token.  */
		return (B_ERROR);

	acquire_sem (ci->ci_OverlayLock);
	retval = B_ERROR;

	if (!ovl->ovl_InUse)
		/*  Not allocated.  */
		goto ackphft;	/*  Look down  */

	if (!ob->width  &&  !ob->height) {
		ovr->OV0CMD =
		 SETVAL2FIELD (ovr->OV0CMD, OV0CMD, ENABLE, FALSE);
		goto savesettings;	/*  Look down  */
	}

	/*  Check color space.  */
	switch (ob->space) {
	case B_RGB15:
		pixbytes = 2;
		srcfmt = DEF2FIELD (OV0CMD, SRCFORMAT, RGB555);
		break;
	case B_RGB16:
		pixbytes = 2;
		srcfmt = DEF2FIELD (OV0CMD, SRCFORMAT, RGB565);
		break;
	case B_YCbCr422:
		pixbytes = 2;	/*  Or is it 3?  */
		srcfmt = DEF2FIELD (OV0CMD, SRCFORMAT, YUV422);
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
	deskl = ci->ci_DispConfig.dc_DispMode.h_display_start;
	deskt = ci->ci_DispConfig.dc_DispMode.v_display_start;
	deskr = deskl + ci->ci_DispConfig.dc_DispMode.timing.h_display;
	deskb = deskt + ci->ci_DispConfig.dc_DispMode.timing.v_display;
	wr = ow->h_start + ow->width;
	wb = ow->v_start + ow->height;
	
	if (ow->h_start >= deskr  ||  wr <= deskl  ||
	    ow->v_start >= deskb  ||  wb <= deskt)
	{
		/*  Window completely outside bounds of visible desktop.  */
		ovr->OV0CMD =
		 SETVAL2FIELD (ovr->OV0CMD, OV0CMD, ENABLE, FALSE);
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

	/*  Color key values  */
	/*  The masks and color values are right-justified (*GRRR!*)  */
	if (ow->flags & B_OVERLAY_COLOR_KEY) {
		switch (ci->ci_DispConfig.dc_Depth) {
		default:
		case 8:
			/*
			 * FIXME: Overlay API needs to be changed to handle
			 * this chip, whose colorkey comparator is (sort of)
			 * before the DAC, not after it.
			 * Assume use of color register 0xff for now.
			 */
			keymask = ~0xff;
			keyval = 0xff;
			break;
		case 15:
			keymask = ~(  (ow->red.mask   << (3 + 16))
			            | (ow->green.mask << (3 + 8))
			            | (ow->blue.mask  << 3));
			keyval = (ow->red.value   << (3 + 16))
			       | (ow->green.value << (3 + 8))
			       | (ow->blue.value  << 3);
			break;
		case 16:
			keymask = ~(  (ow->red.mask   << (3 + 16))
			            | (ow->green.mask << (2 + 8))
			            | (ow->blue.mask  << 3));
			keyval = (ow->red.value   << (3 + 16))
			       | (ow->green.value << (2 + 8))
			       | (ow->blue.value  << 3);
			break;
		}
	} else {
		keymask = keyval = 0;
	}

	/*
	 * Scale factors
	 */
	ovfilt = 0;
	hscale = (float) ov->width / (float) ow->width;
	hscaleint = hscale;
	hscalefrac = hscale * (1 << 12);
	vscale = (float) ov->height / (float) ow->height;
	vscaleint = vscale;
	vscalefrac = vscale * (1 << 12);

	/*  "We will control the horizontal..."  */
	bcropl += ((float) wcropl * hscale);
	bcropr += ((float) wcropr * hscale);
	if (ov->width > ow->width) {
		/*  Minification  */
		ovfilt |= VAL2FIELD (OV0CMD, FILTERHLUMA, OVFILT_MIN_LINEAR);
	} else if (ov->width < ow->width) {
		/*  Magnification  */
		ovfilt |= VAL2FIELD (OV0CMD, FILTERHLUMA, OVFILT_MAG_LINEAR);
	}

	/*  "We will control the vertical..."  */
	bcropt += ((float) wcropt * vscale);
	bcropb += ((float) wcropb * vscale);
	if (ov->height > ow->height) {
		/*  Minification  */
		ovfilt |= VAL2FIELD (OV0CMD, FILTERVLUMA, OVFILT_MIN_LINEAR);
	} else if (ov->height < ow->height) {
		/*  Magnification  */
		ovfilt |= VAL2FIELD (OV0CMD, FILTERVLUMA, OVFILT_MAG_LINEAR);
	}

	/*
	 * Additional scaling voodoo from Intel.  Since chrominance resolution
	 * is typically half of luminance resolution, the chroma scalers need
	 * to be enabled (even for packed modes) and set to one-half of the
	 * luma scalers.  Otherwise you'll get big color blocks in the wrong
	 * places.  (Thanks, Murali.)
	 */
	if (isYUV42x) {
		int32	hsi, hsf, vsi, vsf;

		hsi = hsf = vsi = vsf = 0;
		hsf = hscalefrac >> 1;
		if (hsi = hscaleint >> 1) {
			ovfilt |= VAL2FIELD (OV0CMD, FILTERHCHROMA,
			                     OVFILT_MIN_LINEAR);
		} else {
			ovfilt |= VAL2FIELD (OV0CMD, FILTERHCHROMA,
			                     OVFILT_MAG_LINEAR);
		}
		/*
		 * Vertical divide by two unnecessary (but harmless) for
		 * packed modes; the hardware ignores it.
		 */
		vsf = vscalefrac >> 1;
//		if (vsi = vscaleint >> 1) {
		if (vscaleint) {
			ovfilt |= VAL2FIELD (OV0CMD, FILTERVCHROMA,
			                     OVFILT_MIN_LINEAR);
		} else {
			ovfilt |= VAL2FIELD (OV0CMD, FILTERVCHROMA,
			                     OVFILT_MAG_LINEAR);
		}
		uvscale = VAL2MASKD (UVSCALE, VFRAC, vscalefrac)
		        | VAL2MASKD (UVSCALE, HFRAC, hsf)
		        | VAL2MASKD (UVSCALE, V, vscaleint);
	} else
		uvscale = 0;

	baseaddr = (uint32) ob->buffer_dma - (uint32) ci->ci_BaseAddr0_DMA
	         + bcropt * ob->bytes_per_row
	         + bcropl * pixbytes;


	ovr->OBUF_0Y	= VAL2FIELD (OVBUF_xx, BUFPTR, baseaddr);
	ovr->OBUF_0U	=
	ovr->OBUF_0V	=
	ovr->OBUF_1Y	=
	ovr->OBUF_1U	=
	ovr->OBUF_1V	= 0;

	ovr->OV0STRIDE	= /*VAL2FIELD (OV0STRIDE, PITCHUV, ob->bytes_per_row)
			| */VAL2FIELD (OV0STRIDE, PITCHY, ob->bytes_per_row);
	ovr->YRGB_VPH	= VAL2FIELD (YRGB_VPH, VPHASE1, 0)
			| VAL2FIELD (YRGB_VPH, VPHASE0, 0);
	ovr->UV_VPH	= 0;
	ovr->HORZ_PH	= VAL2FIELD (HORZ_PH, PHASE_UV, 0)
			| VAL2FIELD (HORZ_PH, PHASE_Y, 0);
	ovr->INIT_PH	= 0;	/*  No inversions  */
	ovr->DWINPOS	= VAL2FIELD (DWINPOS, TOP,
			             ow->v_start + wcropt - deskt)
			| VAL2FIELD (DWINPOS, LEFT,
			             ow->h_start + wcropl - deskl);
	ovr->DWINSZ	= VAL2FIELD (DWINSZ, HEIGHT,
			             ow->height - wcropt - wcropb)
			| VAL2FIELD (DWINSZ, WIDTH,
			             ow->width - wcropl - wcropr);
	ovr->SWID	= /*VAL2FIELD (SWID, UV,
			             (ov->width - bcropl - bcropr) * pixbytes)
			| */VAL2FIELD (SWID, Y, ob->bytes_per_row);
	ovr->SWIDQW	= /*VAL2FIELD (SWIDQW, UV, ob->bytes_per_row >> 3)
			| */VAL2FIELD (SWIDQW, Y, ob->bytes_per_row >> 3);
	ovr->SHEIGHT	= /*VAL2FIELD (SHEIGHT, UV,
			             ov->height - bcropt - bcropb)
			| */VAL2FIELD (SHEIGHT, Y,
			             ov->height - bcropt - bcropb);
	ovr->YRGBSCALE	= VAL2MASKD (YRGBSCALE, VFRAC, vscalefrac)
			| VAL2MASKD (YRGBSCALE, H, hscaleint)
			| VAL2MASKD (YRGBSCALE, HFRAC, hscalefrac)
			| VAL2MASKD (YRGBSCALE, V, vscaleint);
	ovr->UVSCALE	= uvscale;
	ovr->OV0CLRC0	= VAL2FIELD (OV0CLRC0, CONTRAST, 1 << 6)
			| VAL2FIELD (OV0CLRC0, BRIGHTNESS, 0);
	ovr->OV0CLRC1	= VAL2FIELD (OV0CLRC1, SATURATION, 1 << 7);
	ovr->DCLRKV	= keyval;
	ovr->DCLRKM	= VAL2FIELD (DCLRKM, KEYENABLE, keymask != 0)
			| VAL2FIELD (DCLRKM, DESTALPHABLENDENABLE, FALSE)
			| VAL2FIELD (DCLRKM, ALPHABLENDENABLE, FALSE)
			| VAL2MASKD (DCLRKM, KEYMASK, keymask);
	ovr->SCLRKVH	=
	ovr->SCLRKVL	= 0;
	ovr->SCLRKM	= 0;
	ovr->OV0CONF	= (hscaleint || vscaleint)
			  ? DEF2FIELD (OV0CONF, LINEBUF, 1x1440)
			  : DEF2FIELD (OV0CONF, LINEBUF, 2x720);
	ovr->OV0CMD	= VAL2FIELD (OV0CMD, TOPOVERLAY, 0)
			| ovfilt
			| DEF2FIELD (OV0CMD, MIRROR, NONE)
			| DEF2FIELD (OV0CMD, YADJUST, FULL)
			| DEF2FIELD (OV0CMD, YUV422ORDER, NORMAL)
			| srcfmt
			| DEF2FIELD (OV0CMD, TVOUTFLIPPOL, 0TO1)
			| DEF2FIELD (OV0CMD, FLIPQUAL, IMMEDIATE)
			| DEF2FIELD (OV0CMD, INITVPHASE, USE0)
			| DEF2FIELD (OV0CMD, FLIPTYPE, FRAME)
			| VAL2FIELD (OV0CMD, IGNOREBUFFERFIELD, FALSE)
			| VAL2MASKD (OV0CMD, BUFFERFIELDSEL, 0)
			| VAL2FIELD (OV0CMD, ENABLE, TRUE);
	ovr->AWINPOS	= ovr->DWINPOS;
	ovr->AWINZ	= ovr->DWINSZ;

savesettings:
	ovl->ovl_Window	= *ow;
	ovl->ovl_View	= *ov;
	ovl->ovl_Buffer	= ob;

	retval = 1;	/*  Notify outside to flush ci_OverlayRegs  */
//	ci->ci_Regs->OV0ADDR = ci->ci_OverlayRegs_DMA | (1 << 31);
ackphft:
	release_sem (ci->ci_OverlayLock);
	return (retval);
}


/*****************************************************************************
 * Initialization.
 */
void
initOverlay (struct devinfo *di)
{
	gfx_card_info	*ci;

	ci = di->di_GfxCardInfo;

	/*  Use extra space we made after cursor.  */
	ci->ci_OverlayRegs = (OverlayRegs *) ((uint8 *) ci->ci_CursorBase
	                                     + SIZEOF_CURSORPAGE);
	ci->ci_OverlayRegs_DMA = di->di_CursorBase_DMA + SIZEOF_CURSORPAGE;
	memset (ci->ci_OverlayRegs, 0, 4 << 10);
dprintf (("+++ OverlayRegs at 0x%08lx (phys 0x%08lx)\n",
 (uint32) ci->ci_OverlayRegs, ci->ci_OverlayRegs_DMA));
}
