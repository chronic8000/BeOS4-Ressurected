
#include <Rect.h>
#include <Region.h>
#include <View.h>
#include <stdio.h>
#include <ByteOrder.h>

#include "rect.h"
#include "DecorWidget.h"
#include "DecorImage.h"
#include "DecorImagePart.h"
#include "DecorState.h"

DecorImagePart::~DecorImagePart()
{	
}

visual_style DecorImagePart::VisualStyle(DecorState *instance)
{
	DecorImage *image;
	GetImage(instance,&image);
	return image->VisualStyle();
}
		
void DecorImagePart::FetchLimits(
	DecorState *changes,
	LimitsStruct *lim)
{
	*lim = Limits();

	DecorImage *image;
	GetImage(changes,&image);

	if (image) {
		int32 width = image->VirtualSize().h;
		int32 height = image->VirtualSize().v;
		if (lim->widths[MIN_LIMIT] == NO_LIMIT) lim->widths[MIN_LIMIT] = width;
		if (lim->widths[MAX_LIMIT] == NO_LIMIT) lim->widths[MAX_LIMIT] = width;
		if (lim->heights[MIN_LIMIT] == NO_LIMIT) lim->heights[MIN_LIMIT] = height;
		if (lim->heights[MAX_LIMIT] == NO_LIMIT) lim->heights[MAX_LIMIT] = height;
	};
};

void DecorImagePart::PropogateChanges(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags,
	DecorAtom *newChoice)
{
	DecorPart::PropogateChanges(space,instigator,changes,dfRedraw,NULL);
};

#if B_HOST_IS_LENDIAN
#define PixelToRGBColor(pixel,c) 		\
	(c).alpha = 0xFF;					\
	(c).red = ((pixel)>>16) & 0xFF;		\
	(c).green = ((pixel)>>8) & 0xFF;	\
	(c).blue = (pixel) & 0xFF;
#else
#define PixelToRGBColor(pixel,c) 		\
	(c).alpha = 0xFF;					\
	(c).red = ((pixel)>>8) & 0xFF;		\
	(c).green = ((pixel)>>16) & 0xFF;	\
	(c).blue = ((pixel)>>24) & 0xFF;
#endif

void DecorImagePart::Draw(
	DecorState *instance,
	rect r, DrawingContext context,
	region *update)
{
	rect dstR = r;
	rect dstRy,srcR;
	DecorImage *image;
	bool needToClip = false;
	
	if (update && !overlap_rect(update->Bounds(),r)) return;

//	DebugPrintf(("Drawing %s at %ld,%ld,%ld,%ld\n",m_name,r.left,r.top,r.right,r.bottom));

	GetImage(instance,&image);
	
	const int32 srcWidth = image->VirtualSize().h;
	const int32 srcHeight = image->VirtualSize().v;
	int32 dstWidth = dstR.right - dstR.left + 1;
	int32 dstHeight = dstR.bottom - dstR.top + 1;
	point offset = image->Offset();
	offset.h = offset.h * dstWidth / srcWidth;
	offset.v = offset.v * dstHeight / srcHeight;

	dstR.left += offset.h;
	dstR.right -= ((srcWidth - image->VisibleSize().h) * dstWidth / srcWidth) - offset.h;
	dstR.top += offset.v;
	dstR.bottom -= ((srcHeight - image->VisibleSize().v) * dstHeight / srcHeight) - offset.v;

	srcR.left = srcR.top = 0;
	srcR.right = image->Width()-1;
	srcR.bottom = image->Height()-1;

	if ((m_rules & RULE_X) == RULE_X_TILE) {
		dstR.right = dstR.left + srcWidth-1;
		dstWidth = srcWidth;
		needToClip = true;
	};

	if ((m_rules & RULE_Y) == RULE_Y_TILE) {
		dstR.bottom = dstR.top + srcHeight-1;
		dstHeight = srcHeight;
		needToClip = true;
	};
	
	if ((srcWidth==1) || ((image->Width()==1) && ((m_rules & RULE_X) == RULE_X_SCALE))) {
		uint32 pixel,*p = image->Bits();
		int32 end=0,last = srcR.bottom;
		rgb_color c;
		c.alpha = 255;
		pixel = *p++;
		PixelToRGBColor(pixel,c);
		dstRy.left = dstR.left;
		dstRy.right = dstR.right;
		dstRy.top = dstR.top;

//		printf("width rect fill case: %s\n",Name());

		while (end < last) {
			if (*p == pixel) {
				p++; end++;
			} else {
				dstRy.bottom = r.top + ((image->Offset().v + end + 1) * dstHeight / srcHeight) - 1;
				
				#if defined(DECOR_CLIENT)
					BRect dbr(dstRy.left,dstRy.top,dstRy.right,dstRy.bottom);
					context->SetHighColor(c);
					context->FillRect(dbr);
				#else
					grSetForeColor(context,c);
					grFillIRect(context,dstRy);
				#endif
				
				dstRy.top = dstRy.bottom+1;
				pixel = *p++;
				end++;
				PixelToRGBColor(pixel,c);
			};
		};

		dstRy.bottom = dstR.bottom;
		#if defined(DECOR_CLIENT)
			BRect dbr(dstRy.left,dstRy.top,dstRy.right,dstRy.bottom);
			context->SetHighColor(c);
			context->FillRect(dbr);
		#else
			grSetForeColor(context,c);
			grFillIRect(context,dstRy);
		#endif
		return;
	} else if ((srcHeight==1) || ((image->Height()==1) && ((m_rules & RULE_Y) == RULE_Y_SCALE))) {
		uint32 pixel,*p = image->Bits();
		int32 end=0,last = srcR.right;
		rgb_color c;
		c.alpha = 255;
		pixel = *p++;
		PixelToRGBColor(pixel,c);
		dstRy.left = dstR.left;
		dstRy.top = dstR.top;
		dstRy.bottom = dstR.bottom;

//		printf("height rect fill case: %s\n",Name());

		while (end < last) {
			if (*p == pixel) {
				p++; end++;
			} else {
				dstRy.right = r.left + ((image->Offset().h + end + 1) * dstWidth / srcWidth) - 1;
				
				#if defined(DECOR_CLIENT)
					BRect dbr(dstRy.left,dstRy.top,dstRy.right,dstRy.bottom);
					context->SetHighColor(c);
					context->FillRect(dbr);
				#else
					grSetForeColor(context,c);
					grFillIRect(context,dstRy);
				#endif
				
				dstRy.left = dstRy.right+1;
				pixel = *p++;
				end++;
				PixelToRGBColor(pixel,c);
			};
		};

		dstRy.right = dstR.right;
		#if defined(DECOR_CLIENT)
			BRect dbr(dstRy.left,dstRy.top,dstRy.right,dstRy.bottom);
			context->SetHighColor(c);
			context->FillRect(dbr);
		#else
			grSetForeColor(context,c);
			grFillIRect(context,dstRy);
		#endif
		return;
	};
	
	if (needToClip) {
		#if defined(DECOR_CLIENT)
			BRect cr(r.left,r.top,r.right,r.bottom);
			BRegion reg;
			reg.Include(cr);
			context->ConstrainClippingRegion(&reg);
		#else
			IRegion reg(r);
			grClipToRegion(context,&reg);
		#endif
	};

	drawing_mode drawOp;
	switch (image->VisualStyle()) {
		case B_TRANSPARENT_VISUAL:	drawOp = B_OP_OVER;		break;
		case B_TRANSLUCENT_VISUAL:	drawOp = B_OP_ALPHA;	break;
		default:
		case B_OPAQUE_VISUAL:		drawOp = B_OP_COPY;		break;
	}
	
	#if defined(DECOR_CLIENT)
		context->SetDrawingMode(drawOp);
		if (drawOp == B_OP_ALPHA) context->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
	#else
		grSetDrawOp(context,drawOp);
		if (drawOp == B_OP_ALPHA) grSetBlendingMode(context,B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
	#endif

	while (dstR.left <= r.right) {
		dstRy = dstR;
		while (dstRy.top <= r.bottom) {
			#if defined(DECOR_CLIENT)
				BRect sr(srcR.left,srcR.top,srcR.right,srcR.bottom);
				BRect dr(dstRy.left,dstRy.top,dstRy.right,dstRy.bottom);
				context->DrawBitmapAsync(image->Bitmap(),sr,dr);
			#else
				grDrawIPixels(context,srcR,dstRy,image->Bitmap());
			#endif
			dstRy.top += dstHeight;
			dstRy.bottom += dstHeight;
		};
		dstR.left += dstWidth;
		dstR.right += dstWidth;
	};

	if (needToClip) {
		#if defined(DECOR_CLIENT)
			context->ConstrainClippingRegion(NULL);
		#else
			grClearClip(context);
		#endif
	};
}

void DecorImagePart::Unflatten(DecorStream *f)
{
	DecorPart::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_rules);
	f->FinishReadBytes();
	
	m_visualStyle = (visual_style)-1;
}

/**************************************************************/

#if defined(DECOR_CLIENT)
		
	#include "LuaDecor.h"
	
	DecorImagePart::DecorImagePart(DecorTable *t) : DecorPart(t)
	{
		m_rules = RULE_X_SCALE | RULE_Y_SCALE;
	
		const char *rule;
		
		rule = t->GetString("HorizontalRule");
		if (rule) m_rules = (m_rules & ~RULE_X) | ((!strcmp(rule,"scale")) ? RULE_X_SCALE : RULE_X_TILE);
	
		rule = t->GetString("VerticalRule");
		if (rule) m_rules = (m_rules & ~RULE_Y) | ((!strcmp(rule,"scale")) ? RULE_Y_SCALE : RULE_Y_TILE);
		
		m_visualStyle = (visual_style)-1;
	}
	
	bool DecorImagePart::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(*this)) || DecorPart::IsTypeOf(t);
	}

	void DecorImagePart::Flatten(DecorStream *f)
	{
		DecorPart::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_rules);
		f->FinishWriteBytes();
	}
	
#endif
