/* :ts=8 bk=0
 *
 * gfxbuf.c:	Routines for procuring graphics buffers (surfaces?) in as
 *		abstract a manner as practical at this stage.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.10.12
 */
#include <kernel/OS.h>
#include <drivers/genpool_module.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/gfxbuf.h>
#include <graphics_p/i810/debug.h>

#include "driver.h"
#include "protos.h"


/*****************************************************************************
 * 2000.10.13:  Thoughts and Future Plans
 *
 * By "abstracting" the procurement of graphics buffers, we have the freedom
 * to obtain those buffers out of dedicated graphics RAM, or from AGP
 * memory.  In the case of the I810, everything is technically AGP memory
 * ("taken" RAM and the optional 4M graphics cache are inaccessible,
 * however, until they are mapped into the AGP aperture.)
 *
 * My current thinking is to conceptually split memory usable by the i810
 * into two major types: dedicated, and non-dedicated (or "variable").
 * Dedicated RAM would include "taken" memory and the optional 4M graphics
 * cache.  Non-dedicated RAM would be allocatable system pages up to the
 * configured size of the AGP aperture.
 *
 * How to report this to the app_server is a puzzle.  We can't report just
 * dedicated RAM, as that would restrict the available display modes.
 * However, reporting dedicated plus non-dedicated may be a lie, as there
 * may not be enough available system memory to back up the claim.
 *
 * There's also the further puzzle of restricted RAM usage.  For example,
 * the i810 really doesn't like having display buffers in the optional 4M
 * graphics cache.  This makes it usable only for Z buffers or off-screen 2D
 * images (such as textures).  So how much do you report, and to whom?
 *
 * For the immediate future, however, what I've put together so far should
 * be sufficient to allocate textures out of system RAM and map them into
 * the AGP aperture on an on-demand basis.  The procured system pages
 * wouldn't be released until the app exited, or there was a massive RAM
 * shortfall.
 */

/*****************************************************************************
 * Globals.
 */
extern genpool_module	*ggpm;


/*****************************************************************************
 * Basic allocation primitives.
 */
status_t
allocbymemspec (struct openerinfo *oi, struct BMemSpec *ms)
{
	(void) oi;

	return ((ggpm->gpm_AllocByMemSpec) (ms));
}

status_t
freebymemspec (struct openerinfo *oi, struct BMemSpec *ms)
{
	(void) oi;

	return ((ggpm->gpm_FreeByMemSpec) (ms));
}


/*****************************************************************************
 * Graphics buffer-specific allocation routines.  Takes a buffer ("surface")
 * description and turns it into an allocation specification.
 */
static status_t
formatmemspec_cmdbuf (register struct GfxBuf *gb)
{
	/*
	 * This is a special case:  Ordinarily, gb_Size is calculated from
	 * dimensions and passed back to the client.  Here, gb_Size is
	 * specified by the client; we quantize it to HW requirements.
	 */
	if (((gb->gb_Usage & ~GFXBUF_USAGE_CMDBUF)) == GFXBUF_CMDBUF_RING) {
		/*  4K-byte alignment requirement  */
		gb->gb_Size += (4 << 10) - 1;
		gb->gb_Size &= ~((4 << 10) - 1);
		gb->gb_Alloc.ms_AddrCareBits = (4 << 10) - 1;
	} else {
		/*  uint64 alignment requirement  */
		gb->gb_Size += sizeof (uint64) - 1;
		gb->gb_Size &= ~(sizeof (uint64) - 1);
		gb->gb_Alloc.ms_AddrCareBits = sizeof (uint64) - 1;
	}

	gb->gb_Alloc.ms_AddrStateBits	= 0;
	gb->gb_Alloc.ms_AllocFlags	= 0;	/*  FIXME: from end is nice  */

	gb->gb_ColorSpace		=
	gb->gb_Width			=
	gb->gb_Height			=
	gb->gb_Layout			=
	gb->gb_NMips			=
	gb->gb_FullWidth		=
	gb->gb_FullHeight		=
	gb->gb_BytesPerRow		=
	gb->gb_PixelBytes		=
	gb->gb_PixelBits		= 0;

	return (B_OK);
}


static status_t
formatmemspec_gfx (register struct GfxBuf *gb)
{
	uint32	allocmask = 0;
	uint16	fullhigh;

	gb->gb_PixelBits	= colorspacebits (gb->gb_ColorSpace);
	gb->gb_PixelBytes	= (gb->gb_PixelBits + 7) >> 3;
	gb->gb_BytesPerRow	= gb->gb_PixelBytes * gb->gb_Width;
	
	fullhigh = gb->gb_Height;

	/*
	 * Quantize to hardware requirements.  This varies depending on what
	 * you want to do with the buffer.
	 * GFXBUF_USAGE_2DDEST has no alignment requirements.
	 */
	if (gb->gb_Usage & (GFXBUF_USAGE_3D_COLORBUF | GFXBUF_USAGE_3D_ZBUF))
	{
		/*  i810 only supports these four destination pitches  */
		if (gb->gb_BytesPerRow <= 512) {
			gb->gb_BytesPerRow = 512;
		} else if (gb->gb_BytesPerRow <= 1024) {
			gb->gb_BytesPerRow = 1024;
		} else if (gb->gb_BytesPerRow <= 2048) {
			gb->gb_BytesPerRow = 2048;
		} else if (gb->gb_BytesPerRow <= 4096) {
			gb->gb_BytesPerRow = 4096;
		} else
			return (B_ERROR);
		
		/*  Align to 4x pitch, or 4K, whichever is bigger.  */
		allocmask |= ((4 << 10) - 1) | ((gb->gb_BytesPerRow * 4) - 1);
	}
	if (gb->gb_Usage & GFXBUF_USAGE_3D_TEXTURE) {
		/*
		 * gb_ExtraData, if non-NULL, is assumed to point to an array
		 * of void*s gb_NMips in size.  gb_{Width,Height} are assumed
		 * to be powers of two, as that's what the hardware wants.
		 *
		 * FIXME: Validate the gb_ExtraData pointer before writing
		 * to it.
		 */
		void	**mipbase;
		int	val;
		uint16	h;

		if (gb->gb_BytesPerRow < 32)
			/*
			 * Bump to minimum pitch (undocumented; found lurking
			 * in XFree86 code).
			 */
			gb->gb_BytesPerRow = 32;
		gb->gb_BytesPerRow = (gb->gb_BytesPerRow + 7) & ~7;

		/*  Crank to power of two  */
		if ((val = 1 << highbit (gb->gb_BytesPerRow)) !=
		    gb->gb_BytesPerRow)
			gb->gb_BytesPerRow = val << 1;

		if (gb->gb_NMips > 1) {
			/*
			 * Calculate full height of texture with all mipmaps.
			 * Each mipmap must start on a 128-bit boundary.
			 * (We also alter 'fullhigh' here.)
			 */
			mipbase = (void **) gb->gb_ExtraData;
			h = gb->gb_Height;
			fullhigh = 0;
			for (val = 0;  val < gb->gb_NMips;  val++) {
				if (mipbase)
					mipbase[val] =
					 (void *) (gb->gb_BytesPerRow *
					           fullhigh);
		
				fullhigh += h;
				/*  Align to 128-bit boundary  */
				while ((gb->gb_BytesPerRow * fullhigh) & 0xf)
					fullhigh++;
				/*  Next mip level  */
				if (!(h >>= 1))
					h = 1;
			}
		}

		allocmask |= (2 << 10) - 1;	/*  2K boundary  */
	}
	if (gb->gb_Usage & (GFXBUF_USAGE_DISPLAY | GFXBUF_USAGE_OVERLAY)) {
		if ((gb->gb_Usage & GFXBUF_USAGE_OVERLAY)  &&
		    gb->gb_ColorSpace != B_RGB15  &&
		    gb->gb_ColorSpace != B_RGB16  &&
		    gb->gb_ColorSpace != B_YCbCr422)
			/*  Unsupported overlay colorspace.  */
			return (B_ERROR);

		gb->gb_BytesPerRow = (gb->gb_BytesPerRow + 7) & ~7;
		allocmask |= 7;
	}

	/*
	 * Report full allocated dimensions.
	 */
	gb->gb_FullWidth	= gb->gb_BytesPerRow / gb->gb_PixelBytes;
	gb->gb_FullHeight	= fullhigh;

	/*
	 * Go get the memory.
	 */
	gb->gb_Alloc.ms_MR.mr_Size	= gb->gb_BytesPerRow
					* gb->gb_FullHeight;
	gb->gb_Alloc.ms_AddrCareBits	= allocmask;
	gb->gb_Alloc.ms_AddrStateBits	= 0;
	gb->gb_Alloc.ms_AllocFlags	= 0;	/*  FIXME: from end is nice  */

	return (B_OK);
}

status_t
formatmemspec (struct openerinfo *oi, register struct GfxBuf *gb)
{
	status_t retval;

	gb->gb_Alloc.ms_PoolID = oi->oi_DI->di_GfxCardInfo->ci_PoolID;

	/*  FIXME: Check and handle gb_Layout.  */
	retval = (gb->gb_Usage & GFXBUF_USAGE_CMDBUF)
	       ? formatmemspec_cmdbuf (gb)
	       : formatmemspec_gfx (gb);

	return (retval);
}


/*****************************************************************************
 * Allocate and free graphics buffers.
 */
status_t
allocgfxbuf (struct openerinfo *oi, register struct GfxBuf *gb)
{
	status_t retval;

dprintf (("+++ allocgfxbuf(), dims = %d,%d; gb_Usage = 0x%02x\n",
 gb->gb_Width, gb->gb_Height, gb->gb_Usage));
	if (gb->gb_BaseAddr)
		/*  Memory presumed already allocated.  */
		return (B_ERROR);

	if ((retval = formatmemspec (oi, gb)) < 0)
		return (retval);

	if ((retval = (ggpm->gpm_AllocByMemSpec) (&gb->gb_Alloc)) < 0)
		return (retval);

	gb->gb_BaseAddr		= gb->gb_BaseOffset
				+ oi->oi_DI->di_GfxCardInfo->ci_BaseAddr0;
	gb->gb_BaseAddr_PCI	= gb->gb_BaseOffset
				+ oi->oi_DI->di_GfxCardInfo->ci_BaseAddr0_DMA;
	gb->gb_BaseAddr_AGP	= 0;	/*  FIXME?  */
	
	return (B_OK);
}

status_t
freegfxbuf (struct openerinfo *oi, register struct GfxBuf *gb)
{
	status_t retval;

	(void) oi;

	if (gb->gb_BaseAddr) {
		if ((retval = (ggpm->gpm_FreeByMemSpec) (&gb->gb_Alloc)) == B_OK)
		{
			gb->gb_Size		= 0;
			gb->gb_BaseAddr		= NULL;
			gb->gb_BaseAddr_PCI	=
			gb->gb_BaseAddr_AGP	= 0;
		}
	} else
		retval = B_OK;

	return (retval);
}


/*****************************************************************************
 * ioctl() processing.
 */
status_t
control_gfxbuf (openerinfo *oi, uint32 msg, void *buf, size_t len)
{
	(void) len;

	/*
	 * Q: Why are you setting mr_Owner here?  Wouldn't it be neater to
	 *    set it in the called functions?
	 * A: Yes, but the driver also needs to be able to perform "kernel"
	 *    allocations, which have an mr_Owner of zero.  Rather than write
	 *    two versions of all the functions, we put the mr_Owner
	 *    assignments here, where it will only be called by user-mode
	 *    clients.
	 */
	switch (msg) {
	case GFX_IOCTL_ALLOCBYMEMSPEC:
		((BMemSpec *) buf)->ms_MR.mr_Owner = oi->oi_GenPoolOwnerID;
		return (allocbymemspec (oi, buf));

	case GFX_IOCTL_FREEBYMEMSPEC:
		return (freebymemspec (oi, buf));


	case GFX_IOCTL_FORMATMEMSPEC:
		((GfxBuf *) buf)->gb_Alloc.ms_MR.mr_Owner =
		 oi->oi_GenPoolOwnerID;
		return (formatmemspec (oi, buf));

	case GFX_IOCTL_ALLOCGFXBUF:
		((GfxBuf *) buf)->gb_Alloc.ms_MR.mr_Owner =
		 oi->oi_GenPoolOwnerID;
		return (allocgfxbuf (oi, buf));

	case GFX_IOCTL_FREEGFXBUF:
		return (freegfxbuf (oi, buf));

	default:
		return (B_DEV_INVALID_IOCTL);
	}
}


/*****************************************************************************
 * Misc routine.
 */
uint32
colorspacebits (uint32 cs /* a color_space */)
{
	switch (cs) {
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		return (32);
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
		return (24);
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
	case B_YCbCr422:	/*  This is technically incorrect.  */
		return (16);
	case B_RGB15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_BIG:
	case B_RGBA15_LITTLE:
		return (15);
	case B_CMAP8:
		return (8);
	}
	return (0);
}


/****************************************************************************
 * Returns bit number of most significant bit set in 'val'.
 */
#if __INTEL__
int
highbit (uint32 val)
{
	int retval;
	__asm__ ("BSR %%edx, %%eax": "=a" (retval) : "d" (val));
	return (retval);
}

#elif __POWERPC__

// returns -1 when val == 0 on PPC, but don't rely on it for other CPUs
asm int
highbit (uint32 val)
{
	cntlzw	r5, r3
	neg	r4, r5
	addi	r3, r4, 31
	blr
}

#else

// slow C version
int
highbit (uint32 val)
{
	uint32	t;
	int	retval = 0;

	if (t = val & 0xffff0000)	val = t, retval += 16;
	if (t = val & 0xff00ff00)	val = t, retval += 8;
	if (t = val & 0xf0f0f0f0)	val = t, retval += 4;
	if (t = val & 0xcccccccc)	val = t, retval += 2;
	if (val & 0xaaaaaaaa)		retval += 1;

	return (retval);
}
#endif
