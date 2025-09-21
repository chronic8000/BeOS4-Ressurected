#ifndef _ADDONS_H
#define _ADDONS_H

#include <image.h>
#include <List.h>
#include <Locker.h>
#include <Path.h>
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>


class AddOn;
class DeviceLooper;
class MonitorDesc;


typedef AddOn*				(*addon_instantiate_func)(const char *);
typedef BInputServerDevice*	(*device_instantiate_func)();
typedef BInputServerFilter*	(*filter_instantiate_func)();
typedef BInputServerMethod*	(*method_instantiate_func)();


void init_add_ons(bool						safeMode,
				  addon_instantiate_func	new_add_on,
				  const char*				leaf,
				  BList						*list);


class AddOn {
public:
							AddOn(const char *path);
	virtual					~AddOn();

	static AddOn*			Factory(const char *path);
	static const char*		Directory();

	virtual status_t		Load();
	virtual status_t		Unload();
	bool					IsLoaded() const;

private:
	virtual status_t		GetSymbols();	

protected:
	BPath					fPath;
	image_id				fImage;
};


class _BDeviceAddOn_ : public AddOn {
public:
							_BDeviceAddOn_(const char *path);
	virtual 				~_BDeviceAddOn_();

	static AddOn*			Factory(const char *path);
	static const char*		Directory();

	virtual status_t		Load();
	virtual status_t		Unload();

private:
	status_t				Start(const char *name);
	status_t				Start(input_device_type type);

	status_t				Stop(const char *name);
	status_t				Stop(input_device_type type);

	status_t				Control(uint32 code, BMessage *message);
	status_t				Control(const char *name, uint32 code, BMessage *message);
	status_t				Control(input_device_type type, uint32 code, BMessage *message);
	
	status_t				RegisterDevices(input_device_ref **devices);
	status_t				UnregisterDevices(input_device_ref **devices);

	status_t				GetDevices(const char *name, BMessage *devices) const;
	bool					IsRunning(const char *name) const;

	status_t				NotifyShutdown();

	virtual status_t		GetSymbols();

	int32					IndexOfDevice(input_device_ref *idr) const;

protected:
friend class BInputServerDevice;
friend class DeviceLooper;
friend class MonitorDesc;

	device_instantiate_func	fInstantiateFunc;
	BInputServerDevice*		fInputDevice;
	BLocker					fDeviceRefListLock;
	BList					fDeviceRefList;
	BList					fControlMessagesList;	

	static _BDeviceAddOn_*	sCurDeviceAddOn;
};


class _BFilterAddOn_ : public AddOn {
public:
							_BFilterAddOn_(const char *path);
	virtual					~_BFilterAddOn_();

	static AddOn*			Factory(const char *path);
	static const char*		Directory();

	virtual status_t		Load();
	virtual status_t		Unload();

	virtual filter_result	Filter(BMessage *message, BList *outList);

private:
	virtual status_t		GetSymbols();

protected:
	filter_instantiate_func	fInstantiateFunc;
	BInputServerFilter*		fInputFilter;
};


class _BMethodAddOn_ : public AddOn {
public:
							_BMethodAddOn_(const char *path);
							~_BMethodAddOn_();

	static AddOn*			Factory(const char *path);
	static const char*		Directory();

	virtual status_t		Load();
	virtual status_t		Unload();

	virtual status_t		MethodActivated(bool active);
	virtual filter_result	Filter(BMessage *message, BList *outList);

	status_t				SetName(const char *name);
	status_t				SetIcon(const uchar *icon);
	status_t				SetMenu(const BMenu *menu, const BMessenger target);

	const char*				Name() const;
	const uchar*			Icon() const;
	int32					IconSize() const;
	const BMessage*			MenuArchive() const;
	const BMessenger		MenuTarget() const;

private:
	virtual status_t		GetSymbols();

protected:
friend class BInputServerMethod;

	method_instantiate_func	fInstantiateFunc;
	BInputServerMethod*		fInputMethod;
	char*					fName;
	uchar					fIconData[256];
	BMessage				fMenuArchive;
	BMessenger				fMenuTarget;

	static _BMethodAddOn_*	sCurMethodAddOn;
};


class KeymapMethodAddOn : public _BMethodAddOn_ {
public:
							KeymapMethodAddOn();
	virtual					~KeymapMethodAddOn();

	virtual status_t		Load();
	virtual status_t		Unload();

	virtual status_t		MethodActivated(bool active);
	virtual filter_result	Filter(BMessage *message, BList *outList);

private:
	virtual status_t		GetSymbols();
};


#endif
