
#include <InputServer.h>
#include <InputServerFilter.h>


BInputServerFilter::BInputServerFilter()
{
}


BInputServerFilter::~BInputServerFilter()
{
}


status_t
BInputServerFilter::InitCheck()
{
	return (B_NO_ERROR);
}


filter_result
BInputServerFilter::Filter(
	BMessage*,
	BList*)
{
	return (B_DISPATCH_MESSAGE);
}

status_t 
BInputServerFilter::GetScreenRegion(BRegion *region) const
{
	return ((InputServer*)be_app)->GetScreenRegion(region);
}

void
BInputServerFilter::_ReservedInputServerFilter1()
{
}


void
BInputServerFilter::_ReservedInputServerFilter2()
{
}


void
BInputServerFilter::_ReservedInputServerFilter3()
{
}


void
BInputServerFilter::_ReservedInputServerFilter4()
{
}
