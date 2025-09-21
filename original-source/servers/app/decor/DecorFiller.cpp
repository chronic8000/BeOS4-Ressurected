
#include "rect.h"
#include "DecorFiller.h"

static LimitsStruct def_limits = {
	{ 0, NO_LIMIT, INFINITE_LIMIT },
	{ 0, NO_LIMIT, INFINITE_LIMIT },
};

void DecorFiller::FetchLimits(
	DecorState *changes,
	LimitsStruct *lim)
{
	*lim = Limits();
};

DecorFiller::DecorFiller()
{
	SetLimits(def_limits);
}

DecorFiller::~DecorFiller()
{	
}

uint32 DecorFiller::Layout(
	CollectRegionStruct *collect,
	rect newBounds, rect oldBounds)
{
	// mathias - 10/25/2000: We use the filler in the noborder decor. We need to return LAYOUT_REGION_CHANGED
	// so that the noborder decor is updated
	if (!empty_rect(oldBounds)) CollectRegion(collect,oldBounds);
	return LAYOUT_REGION_CHANGED;
}

void DecorFiller::Unflatten(DecorStream *f)
{
	DecorPart::Unflatten(f);
	
	f->StartReadBytes();
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	DecorFiller::DecorFiller(DecorTable *t) : DecorPart(t)
	{
		FillInLimits(def_limits);
	};
	
	bool DecorFiller::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorFiller)) || DecorPart::IsTypeOf(t);
	}

	void DecorFiller::Flatten(DecorStream *f)
	{
		DecorPart::Flatten(f);
		
		f->StartWriteBytes();
		f->FinishWriteBytes();
	}

#endif
