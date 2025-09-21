#include "DeviceLooper.h"
#include "AddOns.h"
#include "InputServer.h"

#include <Autolock.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <InputServerDevice.h>
#include <input_server_private.h>
#include <NodeMonitor.h>


class MonitorDesc {
public:
				MonitorDesc(_BDeviceAddOn_ *addOn, const char *device);
				~MonitorDesc();

	status_t	InitCheck() const;

	int32		AddMonitor(_BDeviceAddOn_ *addOn, const char *device);
	int32		RemoveMonitor(_BDeviceAddOn_ *addOn, const char *device);

	void		NotifyMonitor(BPath *path, BMessage *message);

private:
	status_t	fErr;
	BPath		fDev;
	BList		fMonitors;
};


DeviceLooper	*DeviceLooper::sDeviceLooper = NULL;
BMessenger		DeviceLooper::sDeviceLooperMessenger;


MonitorDesc::MonitorDesc(
	_BDeviceAddOn_	*addOn,
	const char		*device)
{
	fErr = B_ERROR;

	BPath dev("/dev/");
	dev.Append(device);

	fErr = create_directory(dev.Path(), 0755);
	if (fErr != B_OK && fErr != B_FILE_EXISTS) {
		_sPrintf("input_server: create_directory %s failed\n", dev.Path());
		return;
	}

	BNode node(dev.Path());
	fErr = node.InitCheck();
	if (fErr != B_NO_ERROR) {
		_sPrintf("input_server: BNode::InitCheck err %s\n", dev.Path());
		return;
	}

	if (!node.IsDirectory()) {
		_sPrintf("input_server: node is not a dir\n");
		fErr = B_NOT_A_DIRECTORY;
		return;
	}

	node_ref nodeRef;
	fErr = node.GetNodeRef(&nodeRef);
	if (fErr != B_NO_ERROR) {
		_sPrintf("input_server: BNode::GetNodeRef err\n");
		return;	
	}

	_sPrintf("input_server: watch_node -> B_WATCH_DIRECTORY\n");
	fErr = watch_node(&nodeRef, B_WATCH_DIRECTORY, DeviceLooper::sDeviceLooperMessenger);

	if (fErr == B_NO_ERROR) {
		fDev = dev;
		fMonitors.AddItem(addOn);
	}
	else {
		_sPrintf("input_server: watch_node err\n");
	}
}


MonitorDesc::~MonitorDesc()
{
	if (fErr != B_NO_ERROR)
		return;

	BNode node(fDev.Path());

	node_ref nodeRef;
	node.GetNodeRef(&nodeRef);

	_sPrintf("watch_node -> B_STOP_WATCHING\n");
	watch_node(&nodeRef, B_STOP_WATCHING, DeviceLooper::sDeviceLooperMessenger);
}


status_t
MonitorDesc::InitCheck() const
{
	return (fErr);
}


int32
MonitorDesc::AddMonitor(
	_BDeviceAddOn_	*addOn, 
	const char		*device)
{
	if (fErr != B_NO_ERROR)
		return (0);

	BPath dev("/dev/");
	dev.Append(device);

	if (dev != fDev)
		return (0);

	if (!fMonitors.HasItem(addOn))
		fMonitors.AddItem(addOn);

	return (fMonitors.CountItems());
}


int32
MonitorDesc::RemoveMonitor(
	_BDeviceAddOn_	*addOn, 
	const char		*device)
{
	if (fErr != B_NO_ERROR)
		return (0);

	BPath dev("/dev/");
	dev.Append(device);

	if (dev != fDev)
		return (0);

	fMonitors.RemoveItem(addOn);
	
	int32 numItems = fMonitors.CountItems();
	return ((numItems > 0) ? numItems : -1);
}


void
MonitorDesc::NotifyMonitor(
	BPath		*path,
	BMessage	*message)
{
	if (*path != fDev)
		return;

	int32 numItems = fMonitors.CountItems();
	for (int32 i = 0; i < numItems; i++)
		((_BDeviceAddOn_ *)fMonitors.ItemAt(i))->Control(message->what, message);	
}


DeviceLooper::DeviceLooper()
	: BLooper("DevicesLooper")
{
	// this looper has to be running before devices can be initialized
	Run();

	ASSERT(sDeviceLooper == NULL);
	sDeviceLooper = this;
	sDeviceLooperMessenger = BMessenger(sDeviceLooper);

	InitDevices();
}


void
DeviceLooper::MessageReceived(
	BMessage	*message)
{
	bool		sendReply = true;
	BMessage	reply;

	switch (message->what) {
		case IS_START_DEVICES:
		case IS_STOP_DEVICES:
			sendReply = HandleStartStopDevices(message, &reply);
			break;
		
		case IS_CONTROL_DEVICES:
			sendReply = HandleControlDevices(message, &reply);
			break;

		case IS_DEVICE_RUNNING:
			sendReply = HandleIsDeviceRunning(message, &reply);
			break;

		case IS_FIND_DEVICES:
			sendReply = HandleFindDevices(message, &reply);
			break;

		case IS_WATCH_DEVICES:
			sendReply = HandleWatchDevices(message, &reply);	
			break;

		case msg_SystemShuttingDown:
			HandleSystemShuttingDown(message, &reply);
			break;

		case B_NODE_MONITOR:
			HandleNodeMonitor(message);
			sendReply = false;
			break;

		default:
			BLooper::MessageReceived(message);
			sendReply = false;
			break;
	}

	if (sendReply)
		message->SendReply(&reply);
}


status_t
DeviceLooper::StartMonitoringDevice(
	_BDeviceAddOn_	*addOn,
	const char		*device)
{
	BAutolock lock(this);
	if (!lock.IsLocked())
		return (B_ERROR);

	_sPrintf("input_server: StartMonitoringDevice %s\n", device);

	int32 numDescs = fMonitorDescList.CountItems();
	for (int32 i = 0; i < numDescs; i++) {
		if (((MonitorDesc *)fMonitorDescList.ItemAt(i))->AddMonitor(addOn, device) > 0)
			return (B_NO_ERROR);
	}

	MonitorDesc	*desc = new MonitorDesc(addOn, device);
	status_t	err = desc->InitCheck();
	if (err != B_NO_ERROR) {
		delete (desc);
		return (err);
	}

	fMonitorDescList.AddItem(desc);

	return (B_NO_ERROR);
}


status_t
DeviceLooper::StopMonitoringDevice(
	_BDeviceAddOn_	*addOn,
	const char		*device)
{
	BAutolock lock(this);
	if (!lock.IsLocked())
		return (B_ERROR);

	_sPrintf("input_server: StopMonitoringDevice %s\n", device);

	int32 numDescs = fMonitorDescList.CountItems();
	for (int32 i = 0; i < numDescs; i++) {
		MonitorDesc *desc = (MonitorDesc *)fMonitorDescList.ItemAt(i);

		int32 result = desc->RemoveMonitor(addOn, device);
		if (result > 0) 
			return (B_NO_ERROR);
		else {
			if (result < 0) {
				fMonitorDescList.RemoveItem(i);
				delete (desc);
				return (B_NO_ERROR);	
			}
		}
	}

	return (B_ERROR);
}


void
DeviceLooper::InitDevices()
{
	init_add_ons(InputServer::SafeMode(), 
				 _BDeviceAddOn_::Factory, 
				 _BDeviceAddOn_::Directory(), 
				 &fDeviceAddOnList);
}


bool
DeviceLooper::HandleStartStopDevices(
	BMessage	*command,
	BMessage	*reply)
{
	status_t			err = B_ERROR;
	bool				start = command->what == IS_START_DEVICES;
	const char			*name = command->FindString(IS_DEVICE_NAME);
	input_device_type	type = (input_device_type)command->FindInt32(IS_DEVICE_TYPE);

	int32 numDeviceAddOns = fDeviceAddOnList.CountItems();
	for (int32 i = 0; i < numDeviceAddOns; i++) { 
		status_t theErr = B_ERROR;

		if (name != NULL) {
			if (start)			
				theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Start(name);
			else
				theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Stop(name);
		}
		else {
			if (start)
				theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Start(type);
			else
				theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Stop(type);
		}

		if (err != B_NO_ERROR)
			err = (theErr == B_NO_ERROR) ? theErr : (status_t)B_ERROR;
	}

	if (!command->IsSourceWaiting())
		return (false);

	reply->AddInt32(IS_STATUS, err);	
	return (true);
}


bool
DeviceLooper::HandleControlDevices(
	BMessage	*command,
	BMessage	*reply)
{
	status_t			err = B_ERROR;
	const char			*name = command->FindString(IS_DEVICE_NAME);
	input_device_type	type = (input_device_type)command->FindInt32(IS_DEVICE_TYPE);
	uint32				code = command->FindInt32(IS_CONTROL_CODE);
	BMessage			message;
	command->FindMessage(IS_CONTROL_MESSAGE, &message);

	int32 numDeviceAddOns = fDeviceAddOnList.CountItems();
	for (int32 i = 0; i < numDeviceAddOns; i++) { 
		status_t theErr = B_ERROR;

		if (name != NULL)
			theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Control(name, code, &message);
		else
			theErr = ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->Control(type, code, &message);

		if (err != B_NO_ERROR)
			err = (theErr == B_NO_ERROR) ? theErr : (status_t)B_ERROR;
	}

	if (!command->IsSourceWaiting())
		return (false);
	
	reply->ReplaceMessage(IS_CONTROL_MESSAGE, &message);
	reply->AddInt32(IS_STATUS, err);
	return (true);
}


bool
DeviceLooper::HandleIsDeviceRunning(
	BMessage	*command,
	BMessage	*reply)
{
	bool		running = false;
	const char	*name = command->FindString(IS_DEVICE_NAME);

	int32 numDeviceAddOns = fDeviceAddOnList.CountItems();
	for (int32 i = 0; i < numDeviceAddOns; i++)
		running |= ((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->IsRunning(name);

	if (!command->IsSourceWaiting())
		return (false);

	reply->AddInt32(IS_STATUS, (running) ? B_NO_ERROR : B_ERROR);
	return (true);
}


bool
DeviceLooper::HandleFindDevices(
	BMessage	*command,
	BMessage	*reply)
{
	const char *name = command->FindString(IS_DEVICE_NAME);

	int32 numDeviceAddOns = fDeviceAddOnList.CountItems();
	for (int32 i = 0; i < numDeviceAddOns; i++)
		((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->GetDevices(name, reply);

	status_t err = B_NO_ERROR;

	if (name != NULL) {
		type_code	theType = B_STRING_TYPE;
		int32		theCount = 0;
		reply->GetInfo(IS_DEVICE_NAME, &theType, &theCount);
		
		err = (theCount < 1) ? B_ERROR : B_NO_ERROR;
	}	
	
	if (!command->IsSourceWaiting())
		return (false);

	reply->AddInt32(IS_STATUS, err);
	return (true);
}


bool
DeviceLooper::HandleWatchDevices(
	BMessage	*command,
	BMessage	*reply)
{
	int32		foundIndex = -1;
	BMessenger	target;
	command->FindMessenger(IS_WATCH_TARGET, &target);

	int32 numWatchers = fDeviceWatcherList.CountItems();
	for (int32 i = 0; i < numWatchers; i++) {
		if ((*(BMessenger *)fDeviceWatcherList.ItemAt(i)) == target) {
			foundIndex = i;
			break;
		}	
	}

	if (command->FindBool(IS_WATCH_START)) {
		if (foundIndex < 0)
			fDeviceWatcherList.AddItem(new BMessenger(target));
	}
	else {
		if (foundIndex >= 0)
			delete (BMessenger *)(fDeviceWatcherList.RemoveItem(foundIndex));
	}

	if (!command->IsSourceWaiting())
		return (false);

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
	return (true);
}


void
DeviceLooper::HandleSystemShuttingDown(
	BMessage*,
	BMessage	*reply)
{
	int32 numDeviceAddOns = fDeviceAddOnList.CountItems();
	for (int32 i = 0; i < numDeviceAddOns; i++)
		((_BDeviceAddOn_ *)fDeviceAddOnList.ItemAt(i))->NotifyShutdown();

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
DeviceLooper::HandleNodeMonitor(
	BMessage	*message)
{
	node_ref nodeRef;
	if (message->FindInt64("directory", &nodeRef.node) != B_NO_ERROR)
		return;
	if (message->FindInt32("device", &nodeRef.device) != B_NO_ERROR)
		return;

	BPath		path;		
	BDirectory	dir(&nodeRef);
	BEntry		entry(&dir, NULL);
	if (entry.GetPath(&path) != B_NO_ERROR)
		return;

	int32 numDescs = fMonitorDescList.CountItems();
	for (int32 i = 0; i < numDescs; i++)
		((MonitorDesc *)fMonitorDescList.ItemAt(i))->NotifyMonitor(&path, message);
}

