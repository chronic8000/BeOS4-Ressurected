#include "AddOns.h"
#include "InputServer.h"
#include "MethodReplicant.h"

#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <input_server_private.h>
#include <List.h>
#include <Menu.h>
#include <MenuItem.h>


// add-on directories
const char* kDeviceAddOnDirectory = "input_server/devices/";
const char* kFilterAddOnDirectory = "input_server/filters/";
const char* kMethodAddOnDirectory = "input_server/methods/";

// add-on function symbols
const char *kDeviceInstantiateFuncSymbol = "instantiate_input_device";
const char *kFilterInstantiateFuncSymbol = "instantiate_input_filter";
const char *kMethodInstantiateFuncSymbol = "instantiate_input_method";


// name/icon of KeymapMethodAddOn
const char	*kKeymapMethodName = "Roman";
const uchar	kKeymapMethodIcon[] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x17,0x17,0x00,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x17,0x0b,0x17,0x17,0x18,0x00,0x00,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x00,0x00,
	0xff,0xff,0xff,0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0b,0x00,
	0xff,0xff,0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,
	0xff,0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,
	0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,
	0x00,0x17,0x17,0x17,0x0a,0x18,0x0b,0x17,0x17,0x0a,0x0b,0x00,0x0f,0x0f,0xff,0xff,
	0x00,0x11,0x11,0x17,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,
	0x00,0x11,0x11,0x11,0x11,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0x00,0x00,0x11,0x11,0x11,0x0b,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x00,0x11,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};


struct device_ref : input_device_ref {
	bool	running;

			device_ref(input_device_ref *idr);
			~device_ref();

	bool	Matches(input_device_ref *idr);
};


struct control_message : BMessage {
	uint32	code;

			control_message(uint32 inCode, BMessage *inMessage)
				:	BMessage(*inMessage) 
			{
				code = inCode;
			}
};

// static member variables
_BDeviceAddOn_* _BDeviceAddOn_::sCurDeviceAddOn = NULL;
_BMethodAddOn_* _BMethodAddOn_::sCurMethodAddOn = NULL;	


void 
init_add_ons(
	bool					safeMode,
	addon_instantiate_func	new_add_on,
	const char*				leaf,
	BList					*list)
{
	const directory_which kDirectories[] = {
		B_USER_ADDONS_DIRECTORY,
		// B_COMMON_ADDONS_DIRECTORY, this is the same as B_USER_ADDONS_DIRECTORY (as of R4)
		B_BEOS_ADDONS_DIRECTORY
	};
	const int32 kNumDirectories = sizeof(kDirectories) / sizeof(kDirectories[0]);

	for (int32 i = (safeMode) ? 1 : 0; i < kNumDirectories; i++) {
		BPath dirPath;
		find_directory(kDirectories[i], &dirPath);
		dirPath.Append(leaf);
	
		BDirectory theDir(dirPath.Path());
		if (theDir.InitCheck() == B_NO_ERROR) {
			BEntry entry;
	
			theDir.Rewind();
			while (theDir.GetNextEntry(&entry) == B_NO_ERROR) {
				BPath entryPath;
				entry.GetPath(&entryPath);
	
				AddOn *addOn = new_add_on(entryPath.Path());
				if (addOn->Load() == B_NO_ERROR)
					list->AddItem(addOn);
				else	
					delete (addOn);
			}
		}
	}
}


AddOn::AddOn(
	const char	*path)
{
	fPath.SetTo(path);
	fImage = B_ERROR;
}


AddOn::~AddOn()
{
	if (IsLoaded())
		Unload();
}


AddOn*
AddOn::Factory(
	const char	*path)
{
	return (new AddOn(path));
}


const char*
AddOn::Directory()
{
	return (NULL);
}


status_t
AddOn::Load()
{
	ASSERT(!IsLoaded());

	fImage = load_add_on(fPath.Path());

	if (fImage != B_ERROR) {
		if (GetSymbols() != B_NO_ERROR)
			Unload();
	}

	_sPrintf("input_server: load_add_on(%s) %s\n", fPath.Leaf(), (fImage == B_ERROR) ? "failed" : "success"); 
	return ((fImage == B_ERROR) ? B_ERROR : B_NO_ERROR);
}


status_t
AddOn::Unload()
{
	ASSERT(IsLoaded());

	_sPrintf("input_server: unload_add_on(%s)\n", fPath.Leaf());
	status_t err = unload_add_on(fImage);
	fImage = B_ERROR;

	return (err);
}


bool
AddOn::IsLoaded() const
{
	return (fImage != B_ERROR);
}


status_t
AddOn::GetSymbols()
{
	return (B_NO_ERROR);
}


device_ref::device_ref(
	input_device_ref	*idr)
{
	name = strdup(idr->name);
	type = idr->type;
	cookie = idr->cookie;
	running = false;
}


device_ref::~device_ref()
{
	free(name);
}


bool
device_ref::Matches(
	input_device_ref	*idr)
{
	return ( (strcmp(name, idr->name) == 0) && 
			 (type == idr->type) && 
			 (cookie == idr->cookie) );
}


_BDeviceAddOn_::_BDeviceAddOn_(
	const char	*path)
		: AddOn(path)
{
	fInstantiateFunc = NULL;
	fInputDevice = NULL;
}


_BDeviceAddOn_::~_BDeviceAddOn_()
{
	control_message *controlMessage = NULL;
	while ((controlMessage = (control_message *)fControlMessagesList.RemoveItem((int32)0)) != NULL)
		delete (controlMessage);

	// call Unload() here even though it's called 
	// from AddOn's destructor.  it's virtual, so 
	// AddOn will not be able to call _BDeviceAddOn_'s 
	// version of Unload() from its destructor
	if (fInputDevice != NULL)
		Unload();
}


AddOn*
_BDeviceAddOn_::Factory(
	const char	*path)
{
	return (new _BDeviceAddOn_(path));
}


const char*
_BDeviceAddOn_::Directory()
{
	return (kDeviceAddOnDirectory);
}


status_t
_BDeviceAddOn_::Load()
{
	status_t err = AddOn::Load();
	if (err != B_NO_ERROR)
		return (err);

	_sPrintf("input_server: instantiating device %s\n", fPath.Leaf());
	sCurDeviceAddOn = this;
	fInputDevice = fInstantiateFunc();
	sCurDeviceAddOn = NULL;

	if (fInputDevice->InitCheck() != B_NO_ERROR) {
		// InputServerDevice didn't want to be loaded
		_sPrintf("input_server: InitCheck() of device %s failed\n", fPath.Leaf());
		delete (fInputDevice);
		fInputDevice = NULL;
	}
	else {
		control_message *controlMessage = NULL;
		while ((controlMessage = (control_message *)fControlMessagesList.RemoveItem((int32)0)) != NULL) {
			Control(controlMessage->code, controlMessage);
			delete (controlMessage);
		}
	}

	return ((fInputDevice == NULL) ? B_ERROR : B_NO_ERROR);
}


status_t
_BDeviceAddOn_::Unload()
{
	BAutolock lock(fDeviceRefListLock);

	if (fInputDevice != NULL) {
		device_ref *deviceRef = NULL;
		while ((deviceRef = (device_ref *)fDeviceRefList.RemoveItem((int32)0)) != NULL) {
			if (deviceRef->running) {
				_sPrintf("input_server: stopping device %s in %s\n", deviceRef->name, fPath.Leaf());				
				fInputDevice->Stop(deviceRef->name, deviceRef->cookie);
			}

			delete (deviceRef);
		}

		_sPrintf("input_server: destructing device %s\n", fPath.Leaf());
		delete (fInputDevice);
		fInputDevice = NULL;
	}

	return (AddOn::Unload());
}


status_t
_BDeviceAddOn_::Start(
	const char	*name)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if ((!deviceRef->running) && (strcmp(deviceRef->name, name) == 0)) {
			_sPrintf("input_server: starting device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			deviceRef->running = true;
			fInputDevice->Start(deviceRef->name, deviceRef->cookie);
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::Start(
	input_device_type	type)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if ((!deviceRef->running) && (deviceRef->type == type)) {
			_sPrintf("input_server: starting device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			deviceRef->running = true;
			fInputDevice->Start(deviceRef->name, deviceRef->cookie);
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::Stop(
	const char	*name)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if ((deviceRef->running) && (strcmp(deviceRef->name, name) == 0)) {
			_sPrintf("input_server: stopping device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			fInputDevice->Stop(deviceRef->name, deviceRef->cookie);
			deviceRef->running = false;
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::Stop(
	input_device_type	type)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if ((deviceRef->running) && (deviceRef->type == type)) {
			_sPrintf("input_server: stopping device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			fInputDevice->Stop(deviceRef->name, deviceRef->cookie);
			deviceRef->running = false;
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::Control(
	uint32		code,
	BMessage	*message)
{
	if (fInputDevice != NULL)
		fInputDevice->Control(NULL, NULL, code, message);
	else
		// fControlMessagesList is used only for caching add-on wide
		// (as opposed to add-on device_ref specific) control messages
		// B_NODE_MONITOR is a good example for this type of message
		fControlMessagesList.AddItem(new control_message(code, message));

	return (B_NO_ERROR);
}


status_t
_BDeviceAddOn_::Control(
	const char	*name,	
	uint32		code,
	BMessage	*message)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;
	
	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if (strcmp(deviceRef->name, name) == 0) {
			// _sPrintf("input_server: controlling device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			fInputDevice->Control(deviceRef->name, deviceRef->cookie, code, message);
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::Control(
	input_device_type	type,	
	uint32				code,
	BMessage			*message)
{
	ASSERT(fInputDevice);

	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;
	
	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if (deviceRef->type == type) {
			// _sPrintf("input_server: controlling device %s (%d) in %s\n", deviceRef->name, deviceRef->type, fPath.Leaf());
			fInputDevice->Control(deviceRef->name, deviceRef->cookie, code, message);
			err = B_NO_ERROR;
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::RegisterDevices(
	input_device_ref	**devices)
{
	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;
	bool		pointingDevice = false;
	bool		keyboardDevice = false;

	input_device_ref *device = NULL;
	for (int32 i = 0; (device = devices[i]) != NULL; i++) {
		if (device->name == NULL) {
			_sPrintf("input_server: error registering device %s %d %x from %s (device name NULL)\n", device->name, device->type, device->cookie, fPath.Leaf());
			continue;
		}

		if (IndexOfDevice(device) >= 0) {
			_sPrintf("input_server: error registering device %s %d %x from %s (device already registered)\n", device->name, device->type, device->cookie, fPath.Leaf());
			continue;
		}

		_sPrintf("input_server: registering device %s %d %x from %s\n", device->name, device->type, device->cookie, fPath.Leaf());
		fDeviceRefList.AddItem(new device_ref(device));

		pointingDevice |= device->type == B_POINTING_DEVICE;
		keyboardDevice |= device->type == B_KEYBOARD_DEVICE;

		err = B_NO_ERROR;
	}

	if (err == B_NO_ERROR) {
		if (InputServer::EventLoopRunning()) {
			if (pointingDevice)
				InputServer::StartStopDevices(NULL, B_POINTING_DEVICE, true);

			if (keyboardDevice)
				InputServer::StartStopDevices(NULL, B_KEYBOARD_DEVICE, true);			
		}
	}

	return (err);
}


status_t
_BDeviceAddOn_::UnregisterDevices(
	input_device_ref	**devices)
{
	BAutolock	lock(fDeviceRefListLock);
	status_t 	err = B_ERROR;

	input_device_ref *device = NULL;
	for (int32 i = 0; (device = devices[i]) != NULL; i++) {
		if (device->name == NULL) {
			_sPrintf("input_server: error unregistering device %s %d %x from %s (device name NULL)\n", device->name, device->type, device->cookie, fPath.Leaf());
			continue;
		}

		int32 index = IndexOfDevice(device);
		if (index < 0) {
			_sPrintf("input_server: error unregistering device %s %d %x from %s (device not registered)\n", device->name, device->type, device->cookie, fPath.Leaf());
			continue;
		}

		_sPrintf("input_server: unregistering device %s %d %x from %s\n", device->name, device->type, device->cookie, fPath.Leaf());
		device_ref *deviceRef = (device_ref *)fDeviceRefList.RemoveItem(index);

		if (deviceRef->running)
			fInputDevice->Stop(deviceRef->name, deviceRef->cookie);
			
		delete (deviceRef);		

		err = B_NO_ERROR;
	}

	return (err);
}


status_t
_BDeviceAddOn_::GetDevices(
	const char	*name,
	BMessage	*devices) const
{
	BAutolock lock((const_cast<_BDeviceAddOn_ *>(this))->fDeviceRefListLock);

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if ((name == NULL) || (strcmp(name, deviceRef->name) == 0)) {
			devices->AddString(IS_DEVICE_NAME, deviceRef->name);
			devices->AddInt32(IS_DEVICE_TYPE, deviceRef->type);
		}
	}

	return (B_NO_ERROR);
}


bool
_BDeviceAddOn_::IsRunning(
	const char	*name) const
{
	BAutolock lock((const_cast<_BDeviceAddOn_ *>(this))->fDeviceRefListLock);

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if (strcmp(name, deviceRef->name) == 0) {
			if (deviceRef->running)
				return (true);
		}	
	}

	return (false);	
}


status_t
_BDeviceAddOn_::NotifyShutdown()
{
	ASSERT(fInputDevice);

	_sPrintf("input_server: notify shutdown to device %s\n", fPath.Path()); 
	fInputDevice->SystemShuttingDown();

	return (B_NO_ERROR);	
}


status_t
_BDeviceAddOn_::GetSymbols()
{
	if (get_image_symbol(fImage, kDeviceInstantiateFuncSymbol, B_SYMBOL_TYPE_TEXT,
						 (void **)&fInstantiateFunc) != B_NO_ERROR) {
		_sPrintf("input_server: get_image_symbol(%s) of %s failed!\n", fPath.Leaf(), kDeviceInstantiateFuncSymbol);
		fInstantiateFunc = NULL;

		return (B_ERROR);
	}

	return (B_NO_ERROR);
}
	

int32
_BDeviceAddOn_::IndexOfDevice(
	input_device_ref	*idr) const
{
	BAutolock lock((const_cast<_BDeviceAddOn_ *>(this))->fDeviceRefListLock);

	int32 numDeviceRefs = fDeviceRefList.CountItems();
	for (int32 i = 0; i < numDeviceRefs; i++) {
		device_ref *deviceRef = (device_ref *)fDeviceRefList.ItemAt(i);

		if (deviceRef->Matches(idr))
			return (i);
	}

	return (-1);
}


_BFilterAddOn_::_BFilterAddOn_(
	const char	*path)
		: AddOn(path)
{
	fInstantiateFunc = NULL;
	fInputFilter = NULL;
}


_BFilterAddOn_::~_BFilterAddOn_()
{
	// call Unload() here even though it's called 
	// from AddOn's destructor.  it's virtual, so 
	// AddOn will not be able to call _BFilterAddOn_'s 
	// version of Unload() from its destructor
	if (fInputFilter != NULL)
		Unload();
}


AddOn*
_BFilterAddOn_::Factory(
	const char	*path)
{
	return (new _BFilterAddOn_(path));
}


const char*
_BFilterAddOn_::Directory()
{
	return (kFilterAddOnDirectory);
}


status_t
_BFilterAddOn_::Load()
{
	status_t err = AddOn::Load();
	if (err != B_NO_ERROR)
		return (err);

	_sPrintf("input_server: instantiating filter %s\n", fPath.Leaf());
	fInputFilter = fInstantiateFunc();

	if (fInputFilter->InitCheck() != B_NO_ERROR) {
		// InputServerFilter didn't want to be loaded
		_sPrintf("input_server: InitCheck() of filter %s failed\n", fPath.Leaf());
		delete (fInputFilter);
		fInputFilter = NULL;
	}

	return ((fInputFilter == NULL) ? B_ERROR : B_NO_ERROR);
}


status_t
_BFilterAddOn_::Unload()
{
	if (fInputFilter != NULL) {
		_sPrintf("input_server: destructing filter %s\n", fPath.Leaf());
		delete (fInputFilter);
		fInputFilter = NULL;
	}

	return (AddOn::Unload());
}


filter_result
_BFilterAddOn_::Filter(
	BMessage	*message,
	BList		*outList)
{
	ASSERT(fInputFilter);	
	return (fInputFilter->Filter(message, outList));
}


status_t
_BFilterAddOn_::GetSymbols()
{
	if (get_image_symbol(fImage, kFilterInstantiateFuncSymbol, B_SYMBOL_TYPE_TEXT,
						 (void **)&fInstantiateFunc) != B_NO_ERROR) {
		_sPrintf("input_server: get_image_symbol(%s) of %s failed!\n", fPath.Leaf(), kFilterInstantiateFuncSymbol);
		fInstantiateFunc = NULL;

		return (B_ERROR);
	}

	return (B_NO_ERROR);
}


_BMethodAddOn_::_BMethodAddOn_(
	const char	*path)
		: AddOn(path)
{
	fInstantiateFunc = NULL;
	fInputMethod = NULL;
	fName = NULL;
}


_BMethodAddOn_::~_BMethodAddOn_()
{
	// call Unload() here even though it's called 
	// from AddOn's destructor.  it's virtual, so 
	// AddOn will not be able to call _BMethodAddOn_'s 
	// version of Unload() from its destructor
	if (fInputMethod != NULL)
		Unload();
}


AddOn*
_BMethodAddOn_::Factory(
	const char	*path)
{
	return (new _BMethodAddOn_(path));
}


const char*
_BMethodAddOn_::Directory()
{
	return (kMethodAddOnDirectory);
}


status_t
_BMethodAddOn_::Load()
{
	status_t err = AddOn::Load();
	if (err != B_NO_ERROR)
		return (err);

	_sPrintf("input_server: instantiating method %s\n", fPath.Leaf());
	sCurMethodAddOn = this;
	fInputMethod = fInstantiateFunc();
	sCurMethodAddOn = NULL;	

	if (fInputMethod->InitCheck() != B_NO_ERROR) {
		// InputServerMethod didn't want to be loaded
		_sPrintf("input_server: InitCheck() of method %s failed\n", fPath.Leaf());
		delete (fInputMethod);
		fInputMethod = NULL;
	}

	return ((fInputMethod == NULL) ? B_ERROR : B_NO_ERROR);
}


status_t
_BMethodAddOn_::Unload()
{
	if (fInputMethod != NULL) {
		_sPrintf("input_server: destructing method %s\n", fPath.Leaf());
		delete (fInputMethod);
		fInputMethod = NULL;
	}

	return (AddOn::Unload());
}


status_t
_BMethodAddOn_::MethodActivated(
	bool	active)
{
	ASSERT(fInputMethod);		
	return (fInputMethod->MethodActivated(active));
}


filter_result
_BMethodAddOn_::Filter(
	BMessage	*message,
	BList		*outList)
{
	ASSERT(fInputMethod);
	return (fInputMethod->Filter(message, outList));
}


status_t
_BMethodAddOn_::SetName(
	const char	*name)
{
	free(fName);
	fName = NULL;

	if (name != NULL)
		fName = strdup(name);

	const BMessenger *methodReplicant = ((InputServer *)be_app)->MethodReplicant();
	if (methodReplicant != NULL) {
		BMessage message(msg_UpdateMethodName);
		message.AddInt32("cookie", (int32)this);
		message.AddString("name", (Name() == NULL) ? "" : Name());

		methodReplicant->SendMessage(&message);
	}

	return (B_NO_ERROR);
}


status_t
_BMethodAddOn_::SetIcon(
	const uchar	*icon)
{
	memset(fIconData, 0, sizeof(fIconData));

	if (icon != NULL)
		memcpy(fIconData, icon, sizeof(fIconData));
	else
		memcpy(fIconData, kKeymapMethodIcon, sizeof(fIconData));

	const BMessenger *methodReplicant = ((InputServer *)be_app)->MethodReplicant();
	if (methodReplicant != NULL) {
		BMessage message(msg_UpdateMethodIcon);
		message.AddInt32("cookie", (int32)this);
		message.AddData("icon", B_RAW_TYPE, Icon(), IconSize());

		methodReplicant->SendMessage(&message);
	}

	return (B_NO_ERROR);
}


status_t
_BMethodAddOn_::SetMenu(
	const BMenu			*menu,
	const BMessenger	target)
{
	fMenuArchive.MakeEmpty();
	fMenuTarget = BMessenger();

	if (menu != NULL) {
		menu->Archive(&fMenuArchive);
		fMenuTarget = target;
	}

	const BMessenger *methodReplicant = ((InputServer *)be_app)->MethodReplicant();
	if (methodReplicant != NULL) {
		BMessage message(msg_UpdateMethodMenu);
		message.AddInt32("cookie", (int32)this);
		message.AddMessage("menu", MenuArchive());	
		message.AddMessenger("target", MenuTarget());

		methodReplicant->SendMessage(&message);
	}

	return (B_NO_ERROR);
}


const char*
_BMethodAddOn_::Name() const
{
	return (fName);
}


const uchar*
_BMethodAddOn_::Icon() const
{
	return (fIconData);
}


int32
_BMethodAddOn_::IconSize() const
{
	return (sizeof(fIconData));
}


const BMessage*
_BMethodAddOn_::MenuArchive() const
{
	return (&fMenuArchive);
}


const BMessenger
_BMethodAddOn_::MenuTarget() const
{
	return (fMenuTarget);
}


status_t
_BMethodAddOn_::GetSymbols()
{
	if (get_image_symbol(fImage, kMethodInstantiateFuncSymbol, B_SYMBOL_TYPE_TEXT,
						 (void **)&fInstantiateFunc) != B_NO_ERROR) {
		_sPrintf("input_server: get_image_symbol(%s) of %s failed!\n", fPath.Leaf(), kMethodInstantiateFuncSymbol);
		fInstantiateFunc = NULL;

		return (B_ERROR);
	}

	return (B_NO_ERROR);
}


KeymapMethodAddOn::KeymapMethodAddOn()
	: _BMethodAddOn_(NULL)
{
	fName = strdup(kKeymapMethodName);
	memcpy(fIconData, kKeymapMethodIcon, sizeof(fIconData));
}


KeymapMethodAddOn::~KeymapMethodAddOn()
{
}


status_t
KeymapMethodAddOn::Load()
{
	return (B_NO_ERROR);
}


status_t
KeymapMethodAddOn::Unload()
{
	return (B_NO_ERROR);
}


status_t
KeymapMethodAddOn::MethodActivated(
	bool)
{
	return (B_NO_ERROR);
}


filter_result
KeymapMethodAddOn::Filter(
	BMessage*,
	BList*)
{
	return (B_DISPATCH_MESSAGE);
}


status_t
KeymapMethodAddOn::GetSymbols()
{
	return (B_NO_ERROR);
}
