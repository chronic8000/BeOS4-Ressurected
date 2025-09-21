#ifndef _INPUTSERVER_H
#define _INPUTSERVER_H

#include <Application.h>
#include <Region.h>
#include <Input.h>


struct keyboard_state {
	uint16		keyboardID;
	key_info	keyInfo;
	uint32		modifiers;
	key_map		keyMap;
	char*		keyMapChars;
	int32		keyMapCharsSize;
	bigtime_t	keyRepeatDelay;
	int32		keyRepeatRate;
};

struct mouse_state {
	mouse_map	mouseMap;
	int32		mouseType;
	int32 		mouseSpeed;
	int32 		mouseAccel;
	bigtime_t	clickSpeed;
};


extern const char kInputServerSignature[];


class _BMethodAddOn_;
class BottomlineWindow;


class InputServer : public BApplication {
public:
						InputServer();

	virtual void		ArgvReceived(int32 argc, char **argv);
	virtual void		ReadyToRun();
	virtual void		MessageReceived(BMessage *message);
	virtual bool		QuitRequested();

	status_t			GetScreenRegion(BRegion *region) const;

	status_t			EnqueueDeviceMessage(BMessage *message);
	status_t			EnqueueMethodMessage(BMessage *message);

	const BMessenger*	MethodReplicant() const;

	static void			StartStopDevices(const char			*name,
										 input_device_type	type, 
										 bool				start);
	static void			ControlDevices(const char			*name,
									   input_device_type	type,
									   uint32				code,
									   BMessage				*message);

	static bool			SafeMode();
	static bool			EventLoopRunning();

	static void			DoMouseAcceleration(int32 *whereX, int32* whereY);
private:
	void				InitKeyboardMouseStates();
	void				InitMethods();
	void				InitFilters();

	void				SetMousePos(int32 *x, int32 *y, BPoint where);
	void				SetMousePos(int32 *x, int32 *y, int32 whereX, int32 whereY);
	void				SetMousePos(int32 *x, int32 *y, float whereX, float whereY);

	bool				GetNextEvents(BList *events);
	bool				SanitizeEvents(BList *events);
	bool				MethodizeEvents(BList *events, bool isMethodEvents);
	bool				FilterEvents(BList *events);
	void				CacheEvents(BList *events);
	void				DispatchEvents(BList *events);

	void				SetNextMethod(bool next);
	void				SetActiveMethod(_BMethodAddOn_ *method);

	void				HandleStartStopDevices(BMessage *command, BMessage *reply);
	void				HandleControlDevices(BMessage *command, BMessage *reply);
	void				HandleIsDeviceRunning(BMessage *command, BMessage *reply);
	void				HandleFindDevices(BMessage *command, BMessage *reply);
	void				HandleWatchDevices(BMessage *command, BMessage *reply);
	void				HandleGetKeyboardID(BMessage *command, BMessage *reply);
	void				HandleSetKeyboardLocks(BMessage *command, BMessage *reply);
	void				HandleGetKeyInfo(BMessage *command, BMessage *reply);
	void				HandleGetModifiers(BMessage *command, BMessage *reply);
	void				HandleSetModifierKey(BMessage *command, BMessage *reply);
	void				HandleGetSetKeyMap(BMessage *command, BMessage *reply);
	void				HandleGetSetKeyRepeatDelay(BMessage *command, BMessage *reply);
	void				HandleGetSetKeyRepeatRate(BMessage *command, BMessage *reply);
	void				HandleGetSetMouseMap(BMessage *command, BMessage *reply);
	void				HandleGetSetMouseType(BMessage *command, BMessage *reply);
	void				HandleGetSetMouseSpeed(BMessage *command, BMessage *reply);
	void				HandleGetSetMouseAcceleration(BMessage *command, BMessage *reply);
	void				HandleGetSetClickSpeed(BMessage *command, BMessage *reply);
	void				HandleSetMousePosition(BMessage *command, BMessage *reply);
	void				HandleFocusUnfocusIMAwareView(BMessage *command, BMessage *reply);
	void				HandleSetMethod(BMessage *command);

	void				LockMethodQueue();
	void				UnlockMethodQueue();

	static int32		EventLoop(void *arg);

private:

	struct CursorData { sem_id sem; area_id area, clone; int32 *atom; };

	keyboard_state		fKeyboardState;
	static mouse_state	fMouseState;
	port_id				fAppServerPort;
	port_id				fEventPort;
	sem_id				fEventLoopStopSem;
	sem_id				fEventLoopStartSem;
	thread_id			fEventLoop;
	int32				fMethodQueueAtom;
	sem_id				fMethodQueueSem;
	BList*				fMethodQueue;
	BList				fMethodAddOnList;
	_BMethodAddOn_*		fActiveMethod;
	BList				fFilterAddOnList;
	BMessenger*			fMethodReplicant;
	BMessenger*			fInlineAwareView;
	BMessenger*			fBottomlineMessenger;
	int32				fScreenX, fScreenY;
	int32				fMouseX, fMouseY;
	int32				fTempMouseX, fTempMouseY;
	uint32				fMouseButtons;
	CursorData			fCursor;
	bool				fBottomlineActive;

	static bool			sSafeMode;
	static bool			sEventLoopRunning;
};


inline status_t InputServer::GetScreenRegion(BRegion *region) const
{
	region->Set(BRect(0.0, 0.0, fScreenX - 1, fScreenY - 1));

	return (B_NO_ERROR);
};


#endif
