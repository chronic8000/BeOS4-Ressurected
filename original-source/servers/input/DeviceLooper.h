#ifndef _DEVICELOOPER_H
#define _DEVICELOOPER_H

#include <Looper.h>


const uint32 msg_SystemShuttingDown = 'SSDn';


class _BDeviceAddOn_;


class DeviceLooper : public BLooper {
public:
						DeviceLooper();

	virtual void		MessageReceived(BMessage *message);	

	status_t			StartMonitoringDevice(_BDeviceAddOn_ *addOn, const char *device);
	status_t			StopMonitoringDevice(_BDeviceAddOn_ *addOn, const char *device);

private:
	void				InitDevices();

	bool				HandleStartStopDevices(BMessage *command, BMessage *reply);
	bool				HandleControlDevices(BMessage *command, BMessage *reply);
	bool				HandleIsDeviceRunning(BMessage *command, BMessage *reply);
	bool				HandleFindDevices(BMessage *command, BMessage *reply);
	bool				HandleWatchDevices(BMessage *command, BMessage *reply);

	void				HandleSystemShuttingDown(BMessage *command, BMessage *reply);	
	void				HandleNodeMonitor(BMessage *message);

private:
	BList				fDeviceAddOnList;
	BList				fDeviceWatcherList;
	BList				fMonitorDescList;

public:
	static DeviceLooper*	sDeviceLooper;
	static BMessenger		sDeviceLooperMessenger;
};


#endif
