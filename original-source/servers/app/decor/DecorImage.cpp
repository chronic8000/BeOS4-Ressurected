
#include <stdio.h>
#include <TranslationKit.h>
#include <Bitmap.h>
#include <File.h>
#include <Path.h>

#include "as_support.h"
#include "DecorImage.h"
#include "macro.h"

DecorImage::DecorImage()
{
	// Mathias 12/09/2000: Don't leak bitmaps
	#if defined(DECOR_CLIENT)
		m_bitmap = NULL;
	#else
		m_bitmap.pixelData = NULL;
	#endif
	m_slices = NULL;
	m_region = NULL;
}

DecorImage::~DecorImage()
{
	// Mathias 12/09/2000: Don't leak bitmaps
	#if defined(DECOR_CLIENT)
		delete m_bitmap;
	#else
		grFree((void *)m_bitmap.pixelData);
	#endif
	delete m_slices;
	if (m_region)
		kill_region(m_region);
}

void DecorImage::Unflatten(DecorStream *f)
{
	int16 width,height,bpr;
	DecorAtom::Unflatten(f);

	if (m_region)
		kill_region(m_region);
		
	f->StartReadBytes();
	
	#if defined(DECOR_CLIENT)
		delete m_bitmap;
		
		f->Read(&width);
		f->Read(&height);
		f->Read(&m_offsetH);
		f->Read(&m_offsetV);
		f->Read(&m_virtualSizeH);
		f->Read(&m_virtualSizeV);
		if (width == 1) f->Read(&m_visibleSizeH);
		else m_visibleSizeH = width;
		if (height == 1) f->Read(&m_visibleSizeV);
		else m_visibleSizeV = height;
		int16 style;
		f->Read(&style);
		m_visualStyle = (visual_style)style;
		f->Read(&bpr);
		m_bitmap = new BBitmap(BRect(0,0,width-1,height-1),0,B_RGBA32,bpr);
		if (m_bitmap->Bits()) {
			f->Read(m_bitmap->Bits(),((int32)height)*bpr);
		} else {
			f->Skip(height*bpr);
		}
		f->Read(&m_region);
	#else
		grFree((void *)m_bitmap.pixelData);
		
		f->Read(&width);
		f->Read(&height);
		m_bitmap.w = width;
		m_bitmap.h = height;
		f->Read(&m_offsetH);
		f->Read(&m_offsetV);
		f->Read(&m_virtualSizeH);
		f->Read(&m_virtualSizeV);
		if (width == 1) f->Read(&m_visibleSizeH);
		else m_visibleSizeH = width;
		if (height == 1) f->Read(&m_visibleSizeV);
		else m_visibleSizeV = height;
		int16 style;
		f->Read(&style);
		m_visualStyle = (visual_style)style;
		f->Read(&bpr);
		m_bitmap.bytesPerRow = bpr;
		const int32 size = m_bitmap.h * m_bitmap.bytesPerRow;
		m_bitmap.pixelFormat = PIX_32BIT;
		m_bitmap.endianess = LENDIAN;
		m_bitmap.colorMap = (ColorMap*)system_cs->color_list;
		m_bitmap.pixelData = (uint8*)grMalloc(size, "DecorImage.m_bitmap.pixelData", MALLOC_CANNOT_FAIL);
		m_bitmap.pixelIsRotated = false;
		m_bitmap.isCompressed = false;
		m_bitmap.isLbxCompressed = false;
		f->Read(m_bitmap.pixelData,size);
		f->Read(&m_region);
	#endif
	
	f->FinishReadBytes();
}

BRect DecorImage::FindSlice(const char* name) const
{
	BRect r;
	if (!m_slices) return r;
	if (m_slices->FindRect(name, &r) != B_OK) r = BRect();
	return r;
}

DecorSubImage::DecorSubImage()
	:	m_image(NULL)
{
}

DecorSubImage::~DecorSubImage()
{
}

/**************************************************************/

#if defined(DECOR_CLIENT)
	
	#include "LuaDecor.h"
	extern char gDecorLocalPath[256];
	
	DecorImage::DecorImage(DecorTable *t)
			: m_slices(NULL)
	{
		char tmpname[512];
		const char *name = t->GetString("src");
		const char *filename = name;
		if (filename[0] != '/') {
			sprintf(tmpname,"%s%s",gDecorLocalPath,name);
			filename = tmpname;
		};
		BPath path(filename);
		m_bitmap = BTranslationUtils::GetBitmapFile(path.Path());
		int32 len = m_bitmap->BitsLength();
		
		const char *sliceStr = t->GetString("Slices");
		if (sliceStr) {
			const char *filename = sliceStr;
			if (filename[0] != '/') {
				sprintf(tmpname,"%s%s",gDecorLocalPath,filename);
				filename = tmpname;
			}
			BFile file(filename, B_READ_ONLY);
			status_t result = file.InitCheck();
			if (result >= 0) {
				m_slices = new BMessage;
				result = m_slices->Unflatten(&file);
			}
			if (result < B_OK) {
				fprintf(stderr, "*** Error loading slice file '%s': %s\n",
							filename, strerror(result));
			}
		}
		
		m_region = NULL;
	
		uint32 mask = 0xFF000000;
		uint32 value = 0;
	
		const char *maskStr = t->GetString("TransparencyMask");
		const char *valStr = t->GetString("TransparencyValue");
		if (valStr) {
			mask = 0xFFFFFFFF;
			sscanf(valStr,"0x%lx",&value);
		};
		if (maskStr) {
			sscanf(maskStr,"0x%lx",&mask);
		};
	
		uint32 *ptr = (uint32*)m_bitmap->Bits();
		while (len >= 4) {
			if (((*ptr) & mask) == value)
				*ptr = B_TRANSPARENT_MAGIC_RGBA32;
			ptr++;
			len -= 4;
		};
		
		m_visualStyle = B_TRANSPARENT_VISUAL;
		m_offsetH = m_offsetV = 0;
		m_virtualSizeH = m_visibleSizeH = (int16)Width();
		m_virtualSizeV = m_visibleSizeV = (int16)Height();
	}
	
	DecorImage::DecorImage(DecorBitmap *bitmap, visual_style style)
	{
		m_bitmap = bitmap;
		m_slices = NULL;
		m_region = NULL;
		m_offsetH = m_offsetV = 0;
		m_virtualSizeH = m_visibleSizeH = (int16)Width();
		m_virtualSizeV = m_visibleSizeV = (int16)Height();
		m_visualStyle = style;
	}

	DecorImage::DecorImage(	DecorBitmap *bitmap, point offset,
							point virtSize, point visSize, visual_style style)
	{
		m_bitmap = bitmap;
		m_slices = NULL;
		m_region = NULL;
		m_offsetH = (int16)offset.h;
		m_offsetV = (int16)offset.v;
		m_virtualSizeH = (int16)virtSize.h;
		m_virtualSizeV = (int16)virtSize.v;
		m_visibleSizeH = (int16)visSize.h;
		m_visibleSizeV = (int16)visSize.v;
		if (m_visibleSizeH > Width() && m_visibleSizeH != m_virtualSizeH)
			debugger("Bad compressed bitmap (horizontal)!");
		if (m_visibleSizeV > Height() && m_visibleSizeV != m_virtualSizeV)
			debugger("Bad compressed bitmap (horizontal)!");
		m_visualStyle = style;
	}

	bool DecorImage::PrettyMuchEquivalentTo(DecorAtom *atom)
	{
		DecorImage *other = dynamic_cast<DecorImage*>(atom);
		if (!other) return false;
		if ((other->Width() != Width()) ||
			(other->Height() != Height()) ||
			(other->VirtualSize().h != VirtualSize().h) ||
			(other->VirtualSize().v != VirtualSize().v) ||
			(other->VisibleSize().h != VisibleSize().h) ||
			(other->VisibleSize().v != VisibleSize().v) ||
			(other->Offset().h != Offset().h) ||
			(other->Offset().v != Offset().v)) return false;
			
		return !memcmp(Bits(),other->Bits(),m_bitmap->BytesPerRow()*Height());
	}

	bool DecorImage::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(*this)) || DecorAtom::IsTypeOf(t);
	}

	void DecorImage::MakeRegion()
	{
		if (m_region)
			kill_region(m_region);
		m_region = newregion();
		
		const uint32 *pix;
		const uint8 *ptr = (const uint8*)m_bitmap->Bits();
		uint32 srcPixel;
		const int32 bpr = m_bitmap->BytesPerRow();
		rect r;
		r.left = r.top = 0;
		r.right = m_visibleSizeH-1;
		r.bottom = m_visibleSizeV-1;
		int32 thisStartX,y,x;
		const int32 width = r.right-r.left+1;
		int32 height = r.bottom-r.top+1;
		const int32 realWidth = Width();
		const int32 realHeight = Height();
		int32 count;
		ptr += bpr*r.top + (r.left*4);
		y = 0;
		
		const int32 rc = ((width/2 + 1)*height + 1);
		region *reg = m_region;
		rect *rects = reg->CreateRects(rc, rc);
		rect *rp=rects;
		rect bound;
		bound.top = 0x7FFFFFFF;
		bound.left = 0x7FFFFFFF;
		bound.right = -0x7FFFFFFF;
		
//		printf("Making region for %p (%ldx%ld)\n", this, width, height);
//		printf("%d,%d,%d,%d\n",Width(),Height(),width,height);
		while (height--) {
			pix = (uint32*)ptr;
			count = width;
			x = thisStartX = 0;
			srcPixel = *pix;
//			printf((srcPixel == B_TRANSPARENT_MAGIC_RGBA32)?" ":"*");
			if (srcPixel != B_TRANSPARENT_MAGIC_RGBA32) goto endOfInSpan;
			else										goto endOfOutSpan;
	
			while (1) {
				srcPixel = *pix;
//				printf((srcPixel == B_TRANSPARENT_MAGIC_RGBA32)?" ":"*");
				if (srcPixel == B_TRANSPARENT_MAGIC_RGBA32) {
					rp->top = rp->bottom = y;
					rp->left = thisStartX;
					rp->right = x-1;
//					printf("%d: %d->%d\n",rp->top,rp->left,rp->right);
					rp++;
	
					if (bound.top == 0x7FFFFFFF) bound.top = y;
					bound.bottom = y;
					if (bound.left > thisStartX) bound.left = thisStartX;
					if (bound.right < (x-1)) bound.right = x-1;
	
					goto endOfOutSpan;
				};
				endOfInSpan:
				if (++x < realWidth) pix++;
				if (!(--count)) goto lastSpan;
			};
	
			while (1) {
				srcPixel = *pix;
//				printf((srcPixel == B_TRANSPARENT_MAGIC_RGBA32)?" ":"*");
				if (srcPixel != B_TRANSPARENT_MAGIC_RGBA32) {
					thisStartX = x;
					goto endOfInSpan;
				};
				endOfOutSpan:
				if (++x < realWidth) pix++;
				if (!(--count)) goto doneLine;
			};
	
			lastSpan:
			rp->top = rp->bottom = y;
			rp->left = thisStartX;
			rp->right = x-1;
//			printf("%d: %d->%d\n",rp->top,rp->left,rp->right);
			rp++;
	
			if (bound.top == 0x7FFFFFFF) bound.top = y;
			bound.bottom = y;
			if (bound.left > thisStartX) bound.left = thisStartX;
			if (bound.right < (x-1)) bound.right = x-1;
			doneLine:
	
//			printf("\n");
			if (++y < realHeight) ptr += bpr;
		};
		
		reg->SetRectCount(rp-rects);
		reg->Bounds() = bound;
		compress_spans(reg);
		reg->CompressMemory();
		offset_region(reg,m_offsetH,m_offsetV);
//		printf("final count=%d\n",reg->count);
	}
	
	#if B_HOST_IS_LENDIAN
	static const uint32		kAlphaMask	= 0xFF000000;
	#else
	static const uint32		kAlphaMask	= 0x000000FF;
	#endif

	DecorAtom * DecorImage::Iterate(int32 *proceed)
	{
		rect transRect;
		transRect.top = transRect.left = 0x7FFFFFFF;
		transRect.bottom = transRect.right = -0x7FFFFFFF;
		visual_style style = B_OPAQUE_VISUAL;
		bool single_x = true, single_y = true;
		bool grey = true;
		{
			int32 sbpr = m_bitmap->BytesPerRow();
			int32 width = Width();
			int32 height = Height();
			const uint8 *sptr = (const uint8*)m_bitmap->Bits();
			const uint32 *colPtr = (const uint32*)sptr;
			const uint32 *srowPtr;
			const uint32 all_color = *(uint32*)sptr;
			for (int32 y=0;y<height;y++) {
				srowPtr = (const uint32*)sptr;
				const uint32 x_color = *srowPtr;
				for (int32 x=0;x<width;x++) {
					// First look for repeated colors.
					const uint32 col = *srowPtr;
					if (single_x && col != x_color) single_x = false;
					if (single_y && col != colPtr[x]) single_y = false;
					if (grey && (((const uint8*)&col)[0] != ((const uint8*)&col)[1]
								|| ((const uint8*)&col)[0] != ((const uint8*)&col)[2]))
						grey = false;
					if (col != B_TRANSPARENT_MAGIC_RGBA32) {
						if (transRect.left > x) transRect.left = x;
						if (transRect.right < x) transRect.right = x;
						if (transRect.top > y) transRect.top = y;
						if (transRect.bottom < y) transRect.bottom = y;
						if ((col&kAlphaMask) != kAlphaMask) {
							if (style < B_TRANSLUCENT_VISUAL) style = B_TRANSLUCENT_VISUAL;
						}
					} else {
						if (style < B_TRANSPARENT_VISUAL) style = B_TRANSPARENT_VISUAL;
					}
					srowPtr++;
				};
				sptr += sbpr;
			};
		};

//		printf("Iterate image %p (%ldx%ld): grey=%d, single_x=%d, single_y=%d\n",
//				this, Width(), Height(), grey, single_x, single_y);
		
		rect clipRect = transRect;
		
		if (single_x && m_virtualSizeH == Width()) {
			// We can compress the X dimension to one pixel!
//			printf("*** Reducing X dimension from %ld to 1 pixel!\n", Width());
			clipRect.right = clipRect.left;
		}
		if (single_y && m_virtualSizeV == Height()) {
			// We can compress the Y dimension to one pixel!
//			printf("*** Reducing Y dimension from %ld to 1 pixel!\n", Height());
			clipRect.bottom = clipRect.top;
		}
		
		if ((clipRect.left == 0) && (clipRect.top == 0) &&
			(clipRect.right == Width()-1) && (clipRect.bottom == Height()-1)) {
			*proceed = 0;
			m_visualStyle = style;
			MakeRegion();
//			printf("Returning self.\n");
			return this;
		};

		if (empty_rect(clipRect)) {
			panic(("Part specified with completely transparent image!\n"));
		};

		style = B_OPAQUE_VISUAL;
		
		rect m_subRect = clipRect;
		BRect r(0,0,m_subRect.right-m_subRect.left,m_subRect.bottom-m_subRect.top);
		BBitmap *sb = Bitmap();
		BBitmap *db = new BBitmap(r,0,B_RGBA32);
		int32 sbpr = sb->BytesPerRow();
		int32 dbpr = db->BytesPerRow();
		uint8 *sptr = (uint8*)sb->Bits();
		uint8 *dptr = (uint8*)db->Bits();
		uint32 *srowPtr,*drowPtr,*sLow,*sHigh;
		sLow = (uint32*)sptr;
		sHigh = (uint32*)(sptr + Height()*sbpr + Width()*4);
		sptr += sbpr*(m_subRect.top) + (m_subRect.left)*4;
		
		for (int32 y=m_subRect.top;y<=m_subRect.bottom;y++) {
			srowPtr = (uint32*)sptr;
			drowPtr = (uint32*)dptr;
			for (int32 x=m_subRect.left;x<=m_subRect.right;x++) {
				if ((srowPtr >= sLow) && (srowPtr < sHigh)) {
					*drowPtr = *srowPtr;
					const uint32 alpha = (*srowPtr) & kAlphaMask;
					if (alpha == 0) {
						if (style < B_TRANSPARENT_VISUAL) style = B_TRANSPARENT_VISUAL;
					} else if (alpha != kAlphaMask) {
						if (style < B_TRANSLUCENT_VISUAL) style = B_TRANSLUCENT_VISUAL;
					}
				} else {
					*drowPtr = B_TRANSPARENT_MAGIC_RGBA32;
					if (style < B_TRANSPARENT_VISUAL) style = B_TRANSPARENT_VISUAL;
				};
				drowPtr++;
				srowPtr++;
			};
			sptr += sbpr;
			dptr += dbpr;
		};
		
		point offset,virtualSize,visibleSize;
		offset.h = clipRect.left;
		offset.v = clipRect.top;
		virtualSize.h = (int16)Width();
		virtualSize.v = (int16)Height();
		visibleSize.h = (int16)(transRect.right-transRect.left+1);
		visibleSize.v = (int16)(transRect.bottom-transRect.top+1);
		DecorImage *newObject = new DecorImage(db,offset,virtualSize,visibleSize,style);
		if (m_slices)
			newObject->m_slices = new BMessage(*m_slices);
		*proceed = 1;
		delete this;
//		printf("Returning image %p: offset=(%ld,%ld), vSize=(%ldx%ld)\n",
//				newObject, offset.h, offset.v, virtualSize.h, virtualSize.v);
		return newObject;
	};

	void DecorImage::Flatten(DecorStream *f)
	{
		DecorAtom::Flatten(f);
		
		int16 width = (int16)(m_bitmap->Bounds().right+1);
		int16 height = (int16)(m_bitmap->Bounds().bottom+1);
		int16 bpr = m_bitmap->BytesPerRow();
		int32 size = height * bpr;
		
		f->StartWriteBytes();
		f->Write(&width);
		f->Write(&height);
		f->Write(&m_offsetH);
		f->Write(&m_offsetV);
		f->Write(&m_virtualSizeH);
		f->Write(&m_virtualSizeV);
		if (width == 1) f->Write(&m_visibleSizeH);
		if (height == 1) f->Write(&m_visibleSizeV);
		int16 style = m_visualStyle;
		f->Write(&style);
		f->Write(&bpr);
		f->Write(m_bitmap->Bits(),size);
		f->Write(m_region);
		f->FinishWriteBytes();
	}
	
	DecorSubImage::DecorSubImage(DecorTable *t)
	{
		m_image = t->GetImage("Image");
		const char *sliceStr = t->GetString("Slice");
		DecorTable *t2 = t->GetDecorTable("ImageRect");
		if (t2) {
			m_subRect.left = (int32)t2->GetNumber(1);
			m_subRect.top = (int32)t2->GetNumber(2);
			m_subRect.right = (int32)t2->GetNumber(3);
			m_subRect.bottom = (int32)t2->GetNumber(4);
			delete t2;
		} else if (sliceStr) {
			DecorImage* im = dynamic_cast<DecorImage*>(m_image->Choose(NULL));
			if (im) {
				BRect r = im->FindSlice(sliceStr);
				if (r.IsValid()) {
					m_subRect.left = (int32)(r.left+.5);
					m_subRect.top = (int32)(r.top+.5);
					m_subRect.right = (int32)(r.right+.5);
					m_subRect.bottom = (int32)(r.bottom+.5);
				} else {
					panic(("Slice '%s' does not exist!\n", sliceStr));
				}
			} else {
				panic(("SubImage not given a valid Image!\n"));
			}
		} else {
			panic(("Sub-image specified without dimensions!\n"));
		}
	}
	
	DecorAtom * DecorImage::CropImage(rect subRect)
	{
		BRect r(0,0,subRect.right-subRect.left,subRect.bottom-subRect.top);
		point offset = Offset();
		BBitmap *sb = Bitmap();
		BBitmap *db = new BBitmap(r,0,B_RGBA32);
		int32 sbpr = sb->BytesPerRow();
		int32 dbpr = db->BytesPerRow();
		uint8 *sptr = (uint8*)sb->Bits();
		uint8 *dptr = (uint8*)db->Bits();
		uint32 *srowPtr,*drowPtr,*sLow,*sHigh;
		sLow = (uint32*)sptr;
		sHigh = (uint32*)(sptr + Height()*sbpr + Width()*4);
		sptr += sbpr*(subRect.top-offset.v) + (subRect.left-offset.h)*4;
		
		for (int32 y=subRect.top;y<=subRect.bottom;y++) {
			srowPtr = (uint32*)sptr;
			drowPtr = (uint32*)dptr;
			for (int32 x=subRect.left;x<=subRect.right;x++) {
				if ((srowPtr >= sLow) && (srowPtr < sHigh))
					*drowPtr = *srowPtr;
				else
					*drowPtr = B_TRANSPARENT_MAGIC_RGBA32;
				drowPtr++;
				srowPtr++;
			};
			sptr += sbpr;
			dptr += dbpr;
		};
		
		int32 proceed = 1;
		DecorAtom *i = new DecorImage(db);
		while (proceed) i = i->Iterate(&proceed);
		return i;
	}

	DecorAtom * ImageCropper(rect *subRect, DecorImage *leaf)
	{
		return leaf->CropImage(*subRect);
	};

	DecorAtom * DecorSubImage::Iterate(int32 *proceed)
	{
		DecorAtom *newAtom = m_image->CopySubTree(typeid(DecorImage),(AtomGenerator)&ImageCropper,&m_subRect);
		*proceed = 1;
		delete this;
		return newAtom;
	}

#endif
