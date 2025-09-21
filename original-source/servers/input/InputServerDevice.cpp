#include <InputServerDevice.h>
#include "InputServer.h"
#include "AddOns.h"
#include "DeviceLooper.h"


BInputServerDevice::BInputServerDevice()
{
	fOwner = _BDeviceAddOn_::sCurDeviceAddOn;
}


BInputServerDevice::~BInputServerDevice()
{
}


status_t
BInputServerDevice::InitCheck()
{
	return (B_NO_ERROR);
}


status_t
BInputServerDevice::SystemShuttingDown()
{
	return (B_NO_ERROR);
}


status_t
BInputServerDevice::Start(
	const char*,
	void*)
{
	return (B_NO_ERROR);
}


status_t
BInputServerDevice::Stop(
	const char*,
	void*)
{
	return (B_NO_ERROR);
}


status_t
BInputServerDevice::Control(
	const char*,
	void*,
	uint32,
	BMessage*)
{
	return (B_NO_ERROR);
}


status_t
BInputServerDevice::RegisterDevices(
	input_device_ref	**devices)
{
	return (fOwner->RegisterDevices(devices));
}


status_t
BInputServerDevice::UnregisterDevices(
	input_device_ref	**devices)
{
	return (fOwner->UnregisterDevices(devices));
}


status_t
BInputServerDevice::EnqueueMessage(
	BMessage	*message)
{
	return (((InputServer *)be_app)->EnqueueDeviceMessage(message));
}


status_t
BInputServerDevice::StartMonitoringDevice(
	const char	*device)
{	
	return (DeviceLooper::sDeviceLooper->StartMonitoringDevice(fOwner, device));
}


status_t
BInputServerDevice::StopMonitoringDevice(
	const char	*device)
{
	return (DeviceLooper::sDeviceLooper->StopMonitoringDevice(fOwner, device));
}


void
BInputServerDevice::_ReservedInputServerDevice1()
{
}


void
BInputServerDevice::_ReservedInputServerDevice2()
{
}


void
BInputServerDevice::_ReservedInputServerDevice3()
{
}


void
BInputServerDevice::_ReservedInputServerDevice4()
{
}



