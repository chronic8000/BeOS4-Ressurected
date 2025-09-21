
//******************************************************************************
//
//	File:			bitmap.h
//	Description:	An SAtom based class to handle bitmaps
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef BITMAP_H
#define BITMAP_H

#include <math.h>
#include "atom.h"
#include "heap.h"
#include "render.h"
#include "as_debug.h"
#include "graphicDevice.h"
#include "bitmapcollection.h"

enum {
	BITMAP_IS_SHARED =		0x00000001,
	BITMAP_IS_INVALID =		0x00000002,
	BITMAP_IS_OVERLAY =		0x00000004,
};

enum { // keep in sync with Bitmap.h
	BITMAP_CLEAR_TO_WHITE			= 0x00000001,
	BITMAP_ACCEPTS_VIEWS			= 0x00000002,
	BITMAP_IS_AREA					= 0x00000004,
	BITMAP_IS_LOCKED				= 0x00000008,
	BITMAP_IS_CONTIGUOUS			= 0x00000010,
	BITMAP_IS_OFFSCREEN				= 0x00000020,
	BITMAP_WILL_OVERLAY				= 0x00000040,
	BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080,
	BITMAP_IS_COMPRESSED			= 0x00000100,
	BITMAP_IS_LBX_COMPRESSED		= 0x00000200,
#if ROTATE_DISPLAY
	B_BITMAP_IS_ROTATED				= 0x40000000,
#endif
};

class SBitmap : public SAtom
{
	public:
						SBitmap(
							int32 x, int32 y, int32 pixelFormat, 
							uint8 endianess, bool clientBitmap=false,
							uint32 flags=0, int32 bytesPerRow=-1,
							Heap *serverHeap=NULL, SApp *team=NULL);
						SBitmap(
							int32 x, int32 y, uint32 pixelFormat, 
							bool clientBitmap=false,
							uint32 flags=0, int32 bytesPerRow=-1,
							Heap *serverHeap=NULL, SApp *team=NULL);
		void			InitData(
							int32 x, int32 y, uint32 pixelFormat, int32 colorSpace, 
							uint8 endianess=LENDIAN, bool clientBitmap=false,
							uint32 flags=0, int32 bytesPerRow=-1,
							Heap *serverHeap=NULL, SApp *team=NULL);
						~SBitmap();

		area_id			AreaID();
		bool			IsShared();
		RenderCanvas *	Canvas();
		uint32			ServerHeapOffset();
		bool			IsValid();
		
		bool			IsOverlay();
		Overlay *		OverlayFor();

		int32			Token();
		void			SetToken(int32 token);

		void			LockPixels();
		void			UnlockPixels();

		#if AS_DEBUG
		virtual void	DumpDebugInfo(DebugInfo *);
		virtual char * 	ObjectName() { return "SBitmap"; };
		#endif

		void			SetPort(RenderSubPort *port);
		inline void		PreloadPixels(frect fr);
		void			PreloadPixels(rect *r);

		int32 			SelectLbxBitmap(int32 lbx_token, int32 index);

	private:

		friend RenderCanvas *	LockBitmap(SBitmap *b, region *bound, uint32 *token, uint32 *flags);
		friend void				UnlockBitmap(SBitmap *b, region *bound, sync_token *token);
		friend RenderCanvas *	LockOverlayBitmap(SBitmap *b, region *bound, uint32 *token, uint32 *flags);
		friend void				UnlockOverlayBitmap(SBitmap *b, region *bound, sync_token *token);
		Heap *					serverHeap;
		uint32					serverHeapOffset;
		
		area_id					areaID;
		uint32					properties;
		RenderCanvas			canvas;
		int32					server_token;
		Overlay *				overlay;

		lbx_pixel_data_t		fLbxPixelData;
};

inline void SBitmap::PreloadPixels(frect fr)
{
	rect r;
	r.top = (int32)floor(fr.top);
	r.bottom = (int32)floor(fr.bottom+1.0);
	PreloadPixels(&r);
};

inline void SBitmap::LockPixels()
{
	if (overlay) {
		overlay->LockBits();
		canvas.pixels.pixelData = (uint8*)overlay->Bits();
	} else if (serverHeap) {
		serverHeap->Lock();
		canvas.pixels.pixelData = (uint8*)serverHeap->Ptr(serverHeapOffset);
	};
};

inline void SBitmap::UnlockPixels()
{
	if (overlay) overlay->UnlockBits();
	if (serverHeap) serverHeap->Unlock();
};

#endif
