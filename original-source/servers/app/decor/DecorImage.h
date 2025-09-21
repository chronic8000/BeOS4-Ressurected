
#ifndef DECORIMAGE_H
#define DECORIMAGE_H

#if defined(DECOR_CLIENT)
#include <Bitmap.h>
#endif

#include <SupportDefs.h>
#include "DecorTypes.h"
#include "DecorAtom.h"

/**************************************************************/

class DecorTable;
class DecorImage : public DecorAtom
{
#if defined(DECOR_CLIENT)
				DecorBitmap *	m_bitmap;
#else
				DecorBitmap		m_bitmap;
#endif

				BMessage *		m_slices;
				region *		m_region;
				int16			m_offsetH, m_offsetV;
				int16			m_virtualSizeH, m_virtualSizeV;
				int16			m_visibleSizeH, m_visibleSizeV;
				visual_style	m_visualStyle;

				DecorImage&		operator=(DecorImage&);
								DecorImage(DecorImage&);
	
	public:
								DecorImage();
		virtual					~DecorImage();
						
#if defined(DECOR_CLIENT)
								DecorImage(DecorBitmap *bitmap, visual_style style = B_OPAQUE_VISUAL);
								DecorImage(DecorTable *table);
								DecorImage(DecorBitmap *bitmap, point offset,
										   point virtSize, point visSize, visual_style style);

				DecorAtom *		CropImage(rect subRect);

		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
		virtual	bool			PrettyMuchEquivalentTo(DecorAtom *atom);
		virtual	DecorAtom *		Iterate(int32 *proceed);
				void			MakeRegion();

#endif
		virtual	void			Unflatten(DecorStream *f);
		
				BRect			FindSlice(const char* name) const;
		
				visual_style	VisualStyle() const { return m_visualStyle; }

		// This is the location of the first non-transparent pixel of the
		// bitmap in the larger virtual space it is defined for.
		inline	point			Offset() { point p; p.h=m_offsetH; p.v=m_offsetV; return p; };
		
		// This is the larger virtual size of the bitmap.  The physical bitmap
		// may be smaller if repeated or transparent colors were removed from its
		// edges.
		inline	point			VirtualSize() { point s; s.h=m_virtualSizeH; s.v=m_virtualSizeV; return s; };

		// This is the actual size of the bitmap that should be drawn.  If the bitmap
		// has repeated pixels in the X/Y dimension, it will be reduced to 1 pixel
		// in the corresponding dimension and VisibleSize() will be the actual size
		// of the drawn bitmap.
		inline	point			VisibleSize() { point s; s.h=m_visibleSizeH; s.v=m_visibleSizeV; return s; };
		
#if defined(DECOR_CLIENT)
		inline	DecorBitmap *	Bitmap() { return m_bitmap; };
		inline	uint32 *		Bits() { return (uint32*)m_bitmap->Bits(); };
		inline	int32			Width() { return (int32)m_bitmap->Bounds().right+1; };
		inline	int32			Height() { return (int32)m_bitmap->Bounds().bottom+1; };
#else
		inline	DecorBitmap *	Bitmap() { return &m_bitmap; };
		inline	uint32 *		Bits() { return (uint32*)m_bitmap.pixelData; };
		inline	int32			Width() { return m_bitmap.w; };
		inline	int32			Height() { return m_bitmap.h; };
#endif

		inline	const region*	Region() { return m_region; }
				
		#define THISCLASS		DecorImage
		#include				"DecorAtomInclude.h"
};

class DecorSubImage : public DecorAtom
{
				rect			m_subRect;
				DecorAtom *		m_image;

				DecorSubImage&	operator=(DecorSubImage&);
								DecorSubImage(DecorSubImage&);
	
	public:
								DecorSubImage();
		virtual					~DecorSubImage();
		
#if defined(DECOR_CLIENT)
								DecorSubImage(DecorTable *table);
		virtual	DecorAtom *		Iterate(int32 *proceed);
#endif

		#define INTERMEDIARY_CLASS
		#define THISCLASS		DecorSubImage
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
