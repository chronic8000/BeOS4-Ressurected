
#include <stdio.h>

#include "as_support.h"
#include "DecorWidget.h"
#include "DecorImage.h"
#include "DecorState.h"

#include <Rect.h>
#include <Region.h>
#include <View.h>

DecorWidget::DecorWidget()
	: 	m_subRegion(NULL)
{
}

void DecorWidget::FetchLimits(
	DecorState *instance,
	LimitsStruct *lim)
{
	*lim = Limits();
};

DecorWidget::~DecorWidget()
{	
	// Mathias 12/09/2000: Don't leak regions
	if (m_subRegion)
		kill_region(m_subRegion);
}

void DecorWidget::GetImage(DecorState *instance, DecorImage **image)
{
	*image = (DecorImage*)m_image->Choose(instance->Space());
};

extern void ShowRegionReal(region *r, char *s);

void DecorWidget::CollectRegion(CollectRegionStruct *collect, rect bounds)
{
	if (m_subRegion && collect->subRegion) {
		scale_region(	m_subRegion,m_imageRect,collect->tmpRegion[0],bounds,
						bounds.left,bounds.top,
						(m_rules&RULE_X) == RULE_X_TILE,
						(m_rules&RULE_Y) == RULE_Y_TILE);
		or_region(collect->tmpRegion[0],collect->subRegion,collect->tmpRegion[1]);
		region *swapper = collect->subRegion;
		collect->subRegion = collect->tmpRegion[1];
		collect->tmpRegion[1] = swapper;
	};

	DecorImage *image;
	GetImage(collect->instance, &image);
	const region *r = image ? image->Region() : NULL;
	scale_region(	r,m_imageRect,collect->tmpRegion[0],bounds,
					bounds.left,bounds.top,
					(m_rules&RULE_X) == RULE_X_TILE,
					(m_rules&RULE_Y) == RULE_Y_TILE);
	or_region(collect->tmpRegion[0],collect->theRegion,collect->tmpRegion[1]);
	region *swapper = collect->theRegion;
	collect->theRegion = collect->tmpRegion[1];
	collect->tmpRegion[1] = swapper;
}

void DecorWidget::Unflatten(DecorStream *f)
{
	// Mathias 12/09/2000: Don't leak regions
	if (m_subRegion)
		kill_region(m_subRegion);

	DecorImagePart::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_image);
	f->Read(&m_imageRect);
	f->Read(&m_subRegion);
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)
	
	#include "LuaDecor.h"
	DecorWidget::DecorWidget(DecorTable *t) : DecorImagePart(t)
	{
		m_image = t->GetImage("Image");
		
		if (m_image) {
			DecorImage *i = (DecorImage*)m_image->Choose(NULL);
			int32 width = i->VirtualSize().h;
			int32 height = i->VirtualSize().v;
			m_imageRect.left = m_imageRect.top = 0;
			m_imageRect.right = width-1;
			m_imageRect.bottom = height-1;
			if (m_limits.widths[MIN_LIMIT] == NO_LIMIT) m_limits.widths[MIN_LIMIT] = width;
			if (m_limits.heights[MIN_LIMIT] == NO_LIMIT) m_limits.heights[MIN_LIMIT] = height;
			if (m_limits.widths[MAX_LIMIT] == NO_LIMIT) m_limits.widths[MAX_LIMIT] = width;
			if (m_limits.heights[MAX_LIMIT] == NO_LIMIT) m_limits.heights[MAX_LIMIT] = height;
		};

		m_subRegion = NULL;

		DecorAtom *mask = t->GetImage("Mask");
		
		if (mask) {
			if (!dynamic_cast<DecorImage*>(mask))
				panic(("Mask cannot be a choice!  Must be an image...\n"));
			m_subRegion = newregion();
			copy_region(((DecorImage*)mask)->Region(), m_subRegion);
			//delete mask;
		};
	}
	
	void DecorWidget::RegisterAtomTree()
	{
		DecorPart::RegisterAtomTree();
		m_image->RegisterAtomTree();
		m_image->RegisterDependency(this,dfChoice);
	}
	
	DecorAtom * DecorWidget::Reduce(int32 *proceed)
	{
		int32 subProceed=1;
		while (subProceed) m_image = m_image->Reduce(&subProceed);
		*proceed = 0;
		return this;
	}

	bool 
	DecorWidget::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorWidget)) || DecorPart::IsTypeOf(t);
	}

	void DecorWidget::Flatten(DecorStream *f)
	{
		DecorImagePart::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_image);
		f->Write(&m_imageRect);
		f->Write(m_subRegion);
		f->FinishWriteBytes();
	}

#endif
