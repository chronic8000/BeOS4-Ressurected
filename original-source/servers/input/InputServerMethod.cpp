#include <InputServerMethod.h>
#include "InputServer.h"
#include "AddOns.h"


BInputServerMethod::BInputServerMethod(
	const char		*name,
	const uchar		*icon)
		: BInputServerFilter()
{
	fOwner = _BMethodAddOn_::sCurMethodAddOn;

	SetName(name);
	SetIcon(icon);
}


BInputServerMethod::~BInputServerMethod()
{
}


status_t
BInputServerMethod::MethodActivated(
	bool)
{
	return (B_NO_ERROR);
}


status_t
BInputServerMethod::EnqueueMessage(
	BMessage	*message)
{
	return (((InputServer *)be_app)->EnqueueMethodMessage(message));	
}


status_t
BInputServerMethod::SetName(
	const char	*name)
{
	return (fOwner->SetName(name));
}


status_t
BInputServerMethod::SetIcon(
	const uchar	*icon)
{
	return (fOwner->SetIcon(icon));
}


status_t
BInputServerMethod::SetMenu(
	const BMenu			*menu,
	const BMessenger	target)
{
	return (fOwner->SetMenu(menu, target));
}


void
BInputServerMethod::_ReservedInputServerMethod1()
{
}


void
BInputServerMethod::_ReservedInputServerMethod2()
{
}


void
BInputServerMethod::_ReservedInputServerMethod3()
{
}


void
BInputServerMethod::_ReservedInputServerMethod4()
{
}
