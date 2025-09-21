#include "InputServer.h"
#include "AddOns.h"
#include "DeviceLooper.h"
#include "MethodReplicant.h"
#include "Keymap.h"
#include "BottomlineWindow.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <StreamIO.h>
#include <input_server_private.h>
#include <kb_mouse_driver.h>
#include <driver_settings_p.h>
#include <priv_syscalls.h>


// misc constants
const char		kInputServerSignature[]		= "application/x-vnd.Be-input_server";
const char*		kDeskbarSig					= "application/x-vnd.Be-TSKB";
const int32		kEventLoopPriority			= B_REAL_TIME_DISPLAY_PRIORITY + 3;
const int32		kEventPortQueueLength		= 1;
const int32		kAppServerPortQueueLength	= 100;

const bigtime_t	kDefaultKeyRepeatDelay		= 250000;
const int32		kDefaultKeyRepeatRate		= 250;
const mouse_map	kDefaultMouseMap			= { B_PRIMARY_MOUSE_BUTTON, 
												B_SECONDARY_MOUSE_BUTTON, 
												B_TERTIARY_MOUSE_BUTTON };
const int32		kDefaultMouseType			= 3;
const int32		kDefaultMouseSpeed			= 65536;
const int32		kDefaultMouseAccel			= 65536;
const bigtime_t	kDefaultClickSpeed			= 500000;


#define FAKE_MOUSE_MOVED			'_FMM'
#define SET_BOTTOMLINE_MESSENGER	'_SBL'	/* used by a browser plugin to give the input_server
											 * a messenger to something that handles the bottomline
											 * window functionality. */
#define ACTIVATE_BOTTOMLINE			'_ABL'	/* tells the browser plugin to activate the bottomline */
#define BOTTOMLINE_EVENTS			'_BLE'	/* used to send events from the bottomline to the input server */

bool InputServer::sSafeMode = false;
bool InputServer::sEventLoopRunning = false;

mouse_state InputServer::fMouseState;

int
main()
{	
	InputServer *app = new InputServer();
	app->Run();
	delete (app);
	
	return (B_NO_ERROR);
}


InputServer::InputServer()
	: BApplication(kInputServerSignature)
{
	char	data = 0;
	uint32	len = sizeof(data);

    //  check for disabled user add-ons
    if (!_kget_safemode_option_(SAFEMODE_DISABLE_USER_ADDONS, &data, &len))
		sSafeMode = true;
 
	// init keyboard/mouse settings
	InitKeyboardMouseStates();

	fScreenX = 1024;
	fScreenY = 768;

	// port that dispatches the events to the app_server
	fAppServerPort = create_port(kAppServerPortQueueLength, "is_as");

	thread_id app_server = B_ERROR;
	if (receive_data(&app_server, &fCursor, sizeof(fCursor)) != B_NO_ERROR)
		_sPrintf("input_server: receive_data() error!\n"); 
	
	if ((fCursor.clone = clone_area("isClone",(void**)&fCursor.atom,B_ANY_ADDRESS,
		B_READ_AREA|B_WRITE_AREA,fCursor.area)) != B_OK)
		_sPrintf("input_server: clone_area() error!\n");

	// port that the device add-ons queue their events into
	fEventPort = create_port(kEventPortQueueLength, "is_event_port");

	struct { port_id asPort,eventPort; } ports = { fAppServerPort, fEventPort };
	if (send_data(app_server, 0, &ports, sizeof(ports)) != B_NO_ERROR)
		_sPrintf("input_server: send_data() error!\n");
	
	// spawn main get/filter/dispatch event loop -- this is slightly higher
	// priority than PreInput (which is in the app_server)
	fEventLoopStopSem = B_ERROR;
	fEventLoopStartSem = B_ERROR;
	fEventLoop = spawn_thread(EventLoop, B_EMPTY_STRING, kEventLoopPriority, this);
	
	// initialize the message queue
	fMethodQueueAtom = 1;
	fMethodQueueSem = create_sem(0, "is_method_queue_sem");
	fMethodQueue = NULL;

	// no active method
	fActiveMethod = NULL;

	// no method replicant
	fMethodReplicant = NULL;

	// not aware of any view that can directly deal with B_INPUT_METHOD_EVENT
	fInlineAwareView = NULL;
	fBottomlineMessenger = NULL;
	fBottomlineActive = false;
}


void
InputServer::ArgvReceived(
	int32	argc,
	char	**argv)
{
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-q") == 0)
			PostMessage(B_QUIT_REQUESTED);
		else
			_sPrintf("input_server: unrecognized argument \"%s\"\n", argv[i]);
	}
}


void
InputServer::ReadyToRun()
{
	// pre-initialization is done, do more init stuff and 
	// start generating events
	resume_thread(fEventLoop);
}


void
InputServer::MessageReceived(
	BMessage	*message)
{
	bool		sendReply = true;
	BMessage 	reply;

	switch (message->what) {
		case SET_BOTTOMLINE_MESSENGER:
			EnqueueMethodMessage(DetachCurrentMessage());
			break;
		case BOTTOMLINE_EVENTS:
			EnqueueMethodMessage(DetachCurrentMessage());
			break;		
		case IS_START_DEVICES:
		case IS_STOP_DEVICES:
		case IS_CONTROL_DEVICES:
		case IS_DEVICE_RUNNING:
		case IS_FIND_DEVICES:
		case IS_WATCH_DEVICES:
			// don't send reply from here, DeviceLooper will take care of it
			DeviceLooper::sDeviceLooperMessenger.SendMessage(message);
			sendReply = false;	
			break;

		case IS_GET_KEYBOARD_ID:
			HandleGetKeyboardID(message, &reply);
			break;

		case IS_SET_KEYBOARD_LOCKS:
			HandleSetKeyboardLocks(message, &reply);
			break;

		case IS_GET_KEY_INFO:
			HandleGetKeyInfo(message, &reply);
			break;

		case IS_GET_MODIFIERS:
			HandleGetModifiers(message, &reply);
			break;

		case IS_SET_MODIFIER_KEY:
			HandleSetModifierKey(message, &reply);
			break;

		case IS_GET_KEY_MAP:
		case IS_SET_KEY_MAP:
			HandleGetSetKeyMap(message, &reply);
			break;

		case IS_GET_KEY_REPEAT_DELAY:
		case IS_SET_KEY_REPEAT_DELAY:
			HandleGetSetKeyRepeatDelay(message, &reply);
			break;

		case IS_GET_KEY_REPEAT_RATE:
		case IS_SET_KEY_REPEAT_RATE:
			HandleGetSetKeyRepeatRate(message, &reply);
			break;

		case IS_GET_MOUSE_MAP:
		case IS_SET_MOUSE_MAP:
			HandleGetSetMouseMap(message, &reply);
			break;

		case IS_GET_MOUSE_TYPE:
		case IS_SET_MOUSE_TYPE:
			HandleGetSetMouseType(message, &reply);
			break;

		case IS_GET_MOUSE_SPEED:
		case IS_SET_MOUSE_SPEED:
			HandleGetSetMouseSpeed(message, &reply);
			break;

		case IS_GET_MOUSE_ACCELERATION:
		case IS_SET_MOUSE_ACCELERATION:
			HandleGetSetMouseAcceleration(message, &reply);
			break;

		case IS_GET_CLICK_SPEED:
		case IS_SET_CLICK_SPEED:
			HandleGetSetClickSpeed(message, &reply);
			break;

		case IS_SET_MOUSE_POSITION:
			HandleSetMousePosition(message, &reply);
			break;

		case IS_FOCUS_IM_AWARE_VIEW:
		case IS_UNFOCUS_IM_AWARE_VIEW:
			HandleFocusUnfocusIMAwareView(message, &reply);
			break;

		case msg_MethodReplicantAddress:
		{
			BMessenger theReplicant;
			if (message->FindMessenger("address", &theReplicant) == B_NO_ERROR) {
				delete (fMethodReplicant);
				fMethodReplicant = new BMessenger(theReplicant);
			}
			sendReply = false;
			break;
		}

		case msg_SetMethod:
			HandleSetMethod(message);
			sendReply = false;
			break;

		case B_SOME_APP_LAUNCHED:
		{	
			const char *launchedSig = NULL;
			if (message->FindString("be:signature", &launchedSig) == B_NO_ERROR) {
				if (strcasecmp(launchedSig, kDeskbarSig) == 0) {
					BMessage replicant(B_ARCHIVED_OBJECT);
					MethodReplicant::ArchiveFactory(&replicant, &fMethodAddOnList);

					if (BMessenger(kDeskbarSig).SendMessage(&replicant) != B_NO_ERROR) {
						_sPrintf("input_server: re-attempting to add replicant to Deskbar...\n");
						BMessage retry(B_SOME_APP_LAUNCHED);
						retry.AddString("be:signature", kDeskbarSig);
						PostMessage(&retry);
					}
				}
			}
			sendReply = false;
			break;
		}

		default:
			BApplication::MessageReceived(message);
			sendReply = false;
			break;
	}

	if (sendReply)
		message->SendReply(&reply);
}


bool
InputServer::QuitRequested()
{
	BMessage synchronizer;
	DeviceLooper::sDeviceLooperMessenger.SendMessage(msg_SystemShuttingDown, &synchronizer);

	bool systemShuttingDown = false;
	CurrentMessage()->FindBool("_shutdown_", &systemShuttingDown);

	// signal to the event loop that it should stop processing events
	fEventLoopStartSem = (systemShuttingDown) ? create_sem(0, "is_event_loop_start_sem") : B_ERROR;
	fEventLoopStopSem = create_sem(0, "is_event_loop_stop_sem");

	// get rid of the port (this unblocks InputServer::GetNextEvents())
	delete_port(fEventPort);
	fEventPort = B_ERROR;

	// wait around for 5 seconds
	if (acquire_sem_etc(fEventLoopStopSem, 1, B_TIMEOUT, 5000000) != B_NO_ERROR) {
		// oh no!  this is bad, the thread is blocked...  just kill it
		_sPrintf("input_server: couldn't stop _input_server_event_loop_, killing it!\n");
		kill_thread(fEventLoop);
		fEventLoop = B_ERROR;
	}

	fActiveMethod = NULL;
	_BMethodAddOn_ *methodAddOn = NULL;
	while ((methodAddOn = (_BMethodAddOn_ *)fMethodAddOnList.RemoveItem((int32)0)) != NULL)
		delete (methodAddOn);

	_BFilterAddOn_ *filterAddOn = NULL;
	while ((filterAddOn = (_BFilterAddOn_ *)fFilterAddOnList.RemoveItem((int32)0)) != NULL)
		delete (filterAddOn);

	// if event loop is going to resume, re-create port that the 
	// device add-ons queue their events into
	if (fEventLoopStartSem != B_ERROR)
		fEventPort = create_port(kEventPortQueueLength, "is_event_port");

	delete_sem(fEventLoopStopSem);
	fEventLoopStopSem = B_ERROR;
	delete_sem(fEventLoopStartSem);
	fEventLoopStartSem = B_ERROR;	

	// strange logic here: if the system is shutting down, we don't 
	// want to go away because the app_server will re-launch us.
	// therefore, return false when shutting down, otherwise return 
	// true, and the app_server can re-launch us if it so desires.
	return ((systemShuttingDown) ? false : true);
}


status_t
InputServer::EnqueueDeviceMessage(
	BMessage	*message)
{
	return (write_port(fEventPort, (int32)message, NULL, 0));
}


status_t
InputServer::EnqueueMethodMessage(
	BMessage	*message)
{	
	status_t err = B_NO_ERROR;

	LockMethodQueue();

	if (fMethodQueue == NULL)
		fMethodQueue = new BList();

	fMethodQueue->AddItem(message);

	// write with a timeout of 0, this will serve as a 'wake-up call'
	err = write_port_etc(fEventPort, 0, NULL, 0, B_TIMEOUT, 0);

	UnlockMethodQueue();
	return (err);
}


const BMessenger*
InputServer::MethodReplicant() const
{
	return (fMethodReplicant);
}


void
InputServer::StartStopDevices(
	const char			*name,
	input_device_type	type, 
	bool				start)
{
	BMessage command((start) ? IS_START_DEVICES : IS_STOP_DEVICES);

	if (name != NULL)
		command.AddString(IS_DEVICE_NAME, name);
	else
		command.AddInt32(IS_DEVICE_TYPE, type);

	DeviceLooper::sDeviceLooperMessenger.SendMessage(&command);
}


void
InputServer::ControlDevices(
	const char			*name,
	input_device_type	type,
	uint32				code,
	BMessage			*message)
{	
	BMessage command(IS_CONTROL_DEVICES);

	if (name != NULL)
		command.AddString(IS_DEVICE_NAME, name);
	else
		command.AddInt32(IS_DEVICE_TYPE, type);

	command.AddInt32(IS_CONTROL_CODE, code);

	if (message != NULL)
		command.AddMessage(IS_CONTROL_MESSAGE, message);	

	DeviceLooper::sDeviceLooperMessenger.SendMessage(&command, (BHandler *)NULL, 0);
}


bool
InputServer::SafeMode()
{
	return (sSafeMode);
}


bool
InputServer::EventLoopRunning()
{
	return (sEventLoopRunning);
}


void
InputServer::InitKeyboardMouseStates()
{
	fMouseX = 0;
	fMouseY = 0;
	fTempMouseX = 0;
	fTempMouseY = 0;
	fMouseButtons = 0;

	memset(&fKeyboardState, 0, sizeof(fKeyboardState));
	memset(&fMouseState, 0, sizeof(fMouseState));

	init_keymap(&fKeyboardState.keyMap, &fKeyboardState.keyMapChars, &fKeyboardState.keyMapCharsSize);

	BPath settingsDirPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsDirPath) == B_NO_ERROR) {
		int fd = -1;

		BPath kbSettingsPath(settingsDirPath);
		kbSettingsPath.Append(kb_settings_file);
		fd = open(kbSettingsPath.Path(), O_RDONLY);
		if (fd >= 0) {
			kb_settings kbSettings;
			read(fd, &kbSettings, sizeof(kbSettings));
			close(fd);

			fKeyboardState.keyRepeatDelay = kbSettings.key_repeat_delay;
			fKeyboardState.keyRepeatRate = kbSettings.key_repeat_rate;
		}
		else {
			fKeyboardState.keyRepeatDelay = kDefaultKeyRepeatDelay;
			fKeyboardState.keyRepeatRate = kDefaultKeyRepeatRate;	
		}
	
		BPath msSettingsPath(settingsDirPath);
		msSettingsPath.Append(mouse_settings_file);
		fd = open(msSettingsPath.Path(), O_RDONLY);
		if (fd >= 0) {
			mouse_settings msSettings;
			read(fd, &msSettings, sizeof(msSettings));
			close(fd);

			memcpy(&fMouseState.mouseMap, &msSettings.map, sizeof(fMouseState.mouseMap));
			fMouseState.mouseType = msSettings.type;
			if (msSettings.accel.speed>256)
				fMouseState.mouseSpeed = msSettings.accel.speed;
			else
				fMouseState.mouseSpeed = kDefaultMouseSpeed;
			fMouseState.mouseAccel = msSettings.accel.accel_factor;
			fMouseState.clickSpeed = msSettings.click_speed;
		}
		else {
			memcpy(&fMouseState.mouseMap, &kDefaultMouseMap, sizeof(fMouseState.mouseMap));
			fMouseState.mouseType = kDefaultMouseType;
			fMouseState.mouseSpeed = kDefaultMouseSpeed;
			fMouseState.mouseAccel = kDefaultMouseAccel;
			fMouseState.clickSpeed = kDefaultClickSpeed;
		}
	}
}


void
InputServer::InitMethods()
{
	init_add_ons(sSafeMode, 
				 _BMethodAddOn_::Factory, 
				 _BMethodAddOn_::Directory(), 
				 &fMethodAddOnList);

	if (fMethodAddOnList.CountItems() > 0) {
		// add default (built-in) roman method
		fActiveMethod = new KeymapMethodAddOn();
		fMethodAddOnList.AddItem(fActiveMethod, 0);

		// start watching for Deskbar
		be_roster->StartWatching(be_app_messenger, B_REQUEST_LAUNCHED);
	}
}


void
InputServer::InitFilters()
{
	init_add_ons(sSafeMode, 
				 _BFilterAddOn_::Factory, 
				 _BFilterAddOn_::Directory(), 
				 &fFilterAddOnList);
}


bool
InputServer::GetNextEvents(
	BList	*events)
{
	do {
		if (fMethodQueue != NULL) {
			// events generated by methods are pending to be dispatched
			LockMethodQueue();
	
			events->AddList(fMethodQueue);
			delete (fMethodQueue);
			fMethodQueue = NULL;

			UnlockMethodQueue();
			return (true);		
		}

		BMessage	eventBody;
		int32		code;
		status_t	err = eventBody.ReadPort(fEventPort, -1, &code);
		
		BMessage*	event = NULL;
		
		if (err == B_BAD_PORT_ID) {
			// The port has been deleted.
			return (false);
		}

		if (err == B_NOT_A_MESSAGE) {
			// The port did not contain message data -- if there is no
			// data, the code is a pointer to a message.
			if (eventBody.RawPortSize() == 0) event = (BMessage*)code;
		}
		
		if (err == B_OK) {
			// An actual BMessage was read from the port.
			event = new BMessage(eventBody);
		}
		
		// a NULL event is a 'wake-up call' from EnqueueMethodMessage(), don't enqueue it
		if (event != NULL) {
			events->AddItem(event);
			return (false);
		}
	} while (true);

	return (false);
}


void
InputServer::SetMousePos(
	int32	*x, 
	int32	*y,
	BPoint	where)
{
	int32 scr_w = fScreenX - 1;
	int32 scr_h = fScreenY - 1;
	
	int32 tempX = (int32)floor(where.x + 0.5);
	int32 tempY = (int32)floor(where.y + 0.5);

	if (tempX > scr_w)
		tempX = scr_w;

	if (tempX < 0)
		tempX = 0;

	if (tempY > scr_h)
		tempY = scr_h;

	if (tempY < 0) 
		tempY = 0;

	*x = tempX;
	*y = tempY;
}


void
InputServer::SetMousePos(
	int32	*x, 
	int32	*y,
	int32	whereX,
	int32	whereY)
{
	int32 scr_w = fScreenX - 1;
	int32 scr_h = fScreenY - 1;
	
	int32 tempX = *x + whereX;
	int32 tempY = *y - whereY;

	if (tempX > scr_w)
		tempX = scr_w;

	if (tempX < 0)
		tempX = 0;

	if (tempY > scr_h)
		tempY = scr_h;

	if (tempY < 0) 
		tempY = 0;

	*x = tempX;
	*y = tempY;
}


void
InputServer::SetMousePos(
	int32	*x,
	int32	*y,
	float	whereX, 
	float	whereY)
{
	int32 scr_w = fScreenX - 1;
	int32 scr_h = fScreenY - 1;

	int32 tempX = (int32)floor(((float)scr_w * (float)whereX) + 0.5);
	int32 tempY = (int32)floor(((float)scr_h * (float)whereY) + 0.5);

	if (tempX > scr_w)
		tempX = scr_w;

	if (tempX < 0)
		tempX = 0;

	if (tempY > scr_h)
		tempY = scr_h;

	if (tempY < 0) 
		tempY = 0;

	*x = tempX;
	*y = tempY;
}

void InputServer::DoMouseAcceleration(
	int32* whereX,
	int32* whereY)
{
	static bigtime_t last_mmove=0LL;
	static double px=0;
	static double py=0;

    double sx=*whereX;
    double sy=*whereY;
    double v=0;

    bigtime_t now_mmove;
    bigtime_t elapsed;

	// find the time since the last mouse move
    now_mmove=system_time();
    elapsed=now_mmove-last_mmove;
    last_mmove=now_mmove;

	// get the actual mouse speed along the axis
    sx*=fMouseState.mouseSpeed/65536.;
	sx/=elapsed;
    sy*=fMouseState.mouseSpeed/65536.;
	sy/=elapsed;

	// don't accelerate if messages are received in a too short period.
	// (because the machine might have been swapping).
	// this breaks acceleration if the mouse samples at more than 300Hz.
	if (elapsed>3000) {
		// get the speed ratio
	    v=sqrt(sx*sx+sy*sy)*fMouseState.mouseAccel/16;
	} else {
		v=0;
	}
	v+=1;
	v*=elapsed;

    sx*=v;
    sy*=v;

    sx+=px;
    sy+=py;

	if (sx > 65536)
		sx = 65536;
	else if (sx < -65536)
		sx = -65536;
	
	if (sy > 65536)
		sy = 65536;
	else if (sy < -65536)
		sy = -65536;

	if (sx>0) *whereX=int32(floor(sx)); else *whereX=int32(ceil(sx));
	if (sy>0) *whereY=int32(floor(sy)); else *whereY=int32(ceil(sy));

    px=sx-*whereX;
    py=sy-*whereY;	
}

bool
InputServer::SanitizeEvents(
	BList	*events)
{
	int32 numEvents = events->CountItems();

	for (int32 i = 0; i < numEvents; i++) {
		BMessage	*event = (BMessage *)events->ItemAt(i);
		bool		dispatchEvent = true;
		
		switch (event->what) {	
			case B_KEY_DOWN:
				if (fActiveMethod != NULL) {
					// look for command-space, the shortcut to switch active methods
					if (fKeyboardState.keyInfo.modifiers & B_COMMAND_KEY) {
						uint32 theChar = 0;
						event->FindInt32("raw_char", (int32 *)&theChar);
	
						if (theChar == ' ') {
							SetNextMethod(!(fKeyboardState.keyInfo.modifiers & B_SHIFT_KEY));
							dispatchEvent = false;
							break;
						}						
					}
				}
				break;

			case FAKE_MOUSE_MOVED:
				fScreenX = event->FindInt32("width");
				fScreenY = event->FindInt32("height");
				event->RemoveData("width");
				event->RemoveData("height");

				event->what = B_MOUSE_MOVED;
				event->AddInt32("buttons", fMouseButtons);
				event->AddInt64("when", system_time());
				goto mouseMovedCase;

			case B_MOUSE_DOWN:
				if (event->FindInt32("buttons") == B_PRIMARY_MOUSE_BUTTON) {
					if ( (fKeyboardState.modifiers & B_CONTROL_KEY) && 
						 (fKeyboardState.modifiers & B_COMMAND_KEY) ) {
						// simulate secondary mouse button
						event->ReplaceInt32("buttons", B_SECONDARY_MOUSE_BUTTON);
						event->ReplaceInt32("modifiers", 0);
					} else if ( (fKeyboardState.modifiers & B_CONTROL_KEY) && 
								(fKeyboardState.modifiers & B_OPTION_KEY) ) {
						// simulate tertiary mouse button
						event->ReplaceInt32("buttons", B_TERTIARY_MOUSE_BUTTON);
						event->ReplaceInt32("modifiers", 0);
					}
				}
				// fall thru

			case B_MOUSE_UP:
			case B_MOUSE_MOVED:
mouseMovedCase:
			{
				int32	xInt = 0;
				int32	yInt = 0;
				float 	xFloat = 0.0;
				float 	yFloat = 0.0;
				BPoint	where;
				bool	removeXY = false;
			
				if ((event->FindInt32("x", &xInt) == B_OK) && (event->FindInt32("y", &yInt) == B_OK)) { 
					SetMousePos(&fTempMouseX, &fTempMouseY, xInt, yInt);
					removeXY = true;
				}
				else if ((event->FindFloat("x", &xFloat) == B_OK) && (event->FindFloat("y", &yFloat) == B_OK)) {
					SetMousePos(&fTempMouseX, &fTempMouseY, xFloat, yFloat);
					removeXY = true;
				} else if (event->FindPoint("where", &where) == B_OK) {
					SetMousePos(&fTempMouseX, &fTempMouseY, where);
					event->RemoveData("where");
				}
				else {
					dispatchEvent = false;
					break;
				}
				
				if (removeXY) {
					event->RemoveData("x");
					event->RemoveData("y");
				}

				event->AddPoint("where", BPoint(fTempMouseX, fTempMouseY));

				if (!event->HasData("modifiers", B_INT32_TYPE))
					event->AddInt32("modifiers", fKeyboardState.modifiers);
				break;
			}

			default:
				break;
		}

		if (!dispatchEvent) {
			events->RemoveItem(i);
			delete (event);
			numEvents--;
			i--;
		}
	}

	return (numEvents > 0);
}


bool
InputServer::MethodizeEvents(
	BList		*events,
	bool		isMethodEvents)
{
	if ((fActiveMethod == NULL) || (isMethodEvents))
		return (true);

	BList	extraEvents;
	int32	numEvents = events->CountItems();

	for (int32 i = 0; i < numEvents; i++) {
		BMessage *event = (BMessage *)events->ItemAt(i);
		
		if (fActiveMethod->Filter(event, &extraEvents) == B_SKIP_MESSAGE) {
			// method wants to drop this event
			events->RemoveItem((int32)i);
			delete (event);			

			if (--numEvents < 1)
				return (false);

			i--;
		}
		else {
			int32 numExtraEvents = extraEvents.CountItems();

			if (numExtraEvents > 0) {
				// delete original event if it's no longer needed
				bool origEventInUse = false;

				for (int32 j = 0; j < numExtraEvents; j++) {
					if (extraEvents.ItemAt(j) == event) {
						origEventInUse = true;
						break;
					}
				}

				if (!origEventInUse)
					delete (event);

				// method is replacing current event with a list of
				// artificially generated events
				events->RemoveItem((int32)i);
				events->AddList(&extraEvents, i);

				numExtraEvents--;
				numEvents += numExtraEvents;
				i += numExtraEvents;
			}
		}

		extraEvents.MakeEmpty();
	}
	
	return (true);
}


bool
InputServer::FilterEvents(
	BList	*events)
{
	BList	extraEvents;
	int32	numEvents = events->CountItems();
	int32	numFilters = fFilterAddOnList.CountItems();

	for (int32 i = 0; i < numFilters; i++) {
		_BFilterAddOn_ *filterAddOn = (_BFilterAddOn_ *)fFilterAddOnList.ItemAt(i);

		for (int32 j = 0; j < numEvents; j++) {
			BMessage *event = (BMessage *)events->ItemAt(j);
			
			if (filterAddOn->Filter(event, &extraEvents) == B_SKIP_MESSAGE) {
				// filter wants to drop this event
				events->RemoveItem((int32)j);
				delete (event);			

				if (--numEvents < 1)
					return (false);

				j--;
			}
			else {
				int32 numExtraEvents = extraEvents.CountItems();

				if (numExtraEvents > 0) {
					// delete original event if it's no longer needed
					bool origEventInUse = false;

					for (int32 k = 0; k < numExtraEvents; k++) {
						if (extraEvents.ItemAt(k) == event) {
							origEventInUse = true;
							break;
						}
					}

					if (!origEventInUse)
						delete (event);

					// filter is replacing current event with a list of
					// artificially generated events
					events->RemoveItem((int32)j);
					events->AddList(&extraEvents, j);

					numExtraEvents--;
					numEvents += numExtraEvents;
					j += numExtraEvents;
				}
			}

			extraEvents.MakeEmpty();
		}
	}
	
	return (true);
}


void
InputServer::CacheEvents(
	BList	*events)
{
	int32 numEvents = events->CountItems();

	for (int32 i = 0; i < numEvents; i++) {
		BMessage *event = (BMessage *)events->ItemAt(i);
		
		switch (event->what) {	
			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
			{
				// cache last key states
				const void	*key_states = NULL;
				ssize_t		size = 0;
				event->FindData("states", B_UINT8_TYPE, &key_states, &size);
				if (size == sizeof(fKeyboardState.keyInfo.key_states))
					memcpy(&fKeyboardState.keyInfo.key_states, key_states, size);

				// cache last modifier state
				fKeyboardState.modifiers = event->FindInt32("modifiers");
				fKeyboardState.keyInfo.modifiers = fKeyboardState.modifiers;
				break;
			}

			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_MOVED:
			{
				bool	skip = false;
				BPoint	where;
		
				if (event->FindPoint("where", &where) == B_OK)
					SetMousePos(&fMouseX, &fMouseY, where);
				else {
					int32	xInt = 0;
					int32	yInt = 0;
					float 	xFloat = 0.0;
					float 	yFloat = 0.0;

					if ((event->FindInt32("x", &xInt) == B_OK) && (event->FindInt32("y", &yInt) == B_OK))
						SetMousePos(&fMouseX, &fMouseY, xInt, yInt);
					else if ((event->FindFloat("x", &xFloat) == B_OK) && (event->FindFloat("y", &yFloat) == B_OK))
						SetMousePos(&fMouseX, &fMouseY, xFloat, yFloat);
					else		
						skip = true;

					if (!skip) {
						event->RemoveData("where");
						event->AddPoint("where", BPoint(fMouseX, fMouseY));
					}
				}

				if (!skip)
					fMouseButtons = event->FindInt32("buttons");
				break;
			}

			default:
				break;
		}
	}
}


void
InputServer::DispatchEvents(
	BList	*events)
{
	BMessage *event = NULL;	

	while ((event = (BMessage *)events->RemoveItem((int32)0)) != NULL) {
		if ((event->what == B_INPUT_METHOD_EVENT) && (fInlineAwareView == NULL))
		{
			// display bottomline window for input methods when necessary
			uint32 opcode = 0;
			if (event->FindInt32("be:opcode", (int32 *)&opcode) == B_NO_ERROR) {
				if (opcode == B_INPUT_METHOD_STARTED) {
					fBottomlineActive = true;
					// Only pop up a BottomlineWindow if the browser is not running.
					// Any input method running on BeIA should provide Bottomline
					// support in a browser plugin.  If the browser is running and
					// it has already notified us of the bottomline messenger, tell
					// it to activate.
					BMessenger msngr("application/x-vnd.Web");
					if (!msngr.IsValid()) {
						BottomlineWindow *win = new BottomlineWindow(be_plain_font);
						fBottomlineMessenger = new BMessenger(win);	
					} else if (fBottomlineMessenger != NULL) {
						BMessage activator(ACTIVATE_BOTTOMLINE);
						fBottomlineMessenger->SendMessage(&activator, (BHandler*)NULL, 1000000LL);						
					}
				}
			}

			if (fBottomlineMessenger != NULL) {
				// send the event to the bottomline messenger, 1 second delivery/reply timeout
				// to guard against no response or a deadlocked targetted looper.
				BMessenger appMessenger(be_app);
				status_t r = fBottomlineMessenger->SendMessage(event, appMessenger, 1000000LL);
				if (r != B_OK) {
					// There was an problem talking to the bottomline messenger.
					fBottomlineActive = false;
					BMessage close(B_CLOSE_REQUESTED);
					fBottomlineMessenger->SendMessage(&close, (BHandler*)NULL, 1000000LL);
					delete fBottomlineMessenger;
					fBottomlineMessenger = NULL;
				}
			}
		}
		else if (event->what == BOTTOMLINE_EVENTS) {
			// the bottomline window or plugin sent us these events in response
			// to interaction with the input method.  We need to stick any events
			// in this message into the 'events' list.
			BMessage genMsg;
			for (int32 i = 0; event->FindMessage("generated_event", i, &genMsg) == B_OK; i++) {
				events->AddItem(new BMessage(genMsg), i);
			}
			if (event->HasBool("cancel_im")) {
				fBottomlineActive = false;
				if (fBottomlineMessenger != NULL) {
					BMessage close(B_CLOSE_REQUESTED);
					fBottomlineMessenger->SendMessage(&close, (BHandler*)NULL, 1000000LL);
					delete fBottomlineMessenger;
					fBottomlineMessenger = NULL;
				}
			}
		}
		else if (event->what == SET_BOTTOMLINE_MESSENGER) {
			if (fBottomlineMessenger != NULL) {
				BMessage close(B_CLOSE_REQUESTED);			
				fBottomlineMessenger->SendMessage(&close, (BHandler*)NULL, 1000000LL);
				delete fBottomlineMessenger;
			}
			fBottomlineMessenger = new BMessenger();
			if (event->FindMessenger("bottomline", fBottomlineMessenger) == B_OK
				&& fBottomlineMessenger->IsValid())
			{
				if (fBottomlineActive) {
					BMessage activator(ACTIVATE_BOTTOMLINE);
					fBottomlineMessenger->SendMessage(&activator, (BHandler*)NULL, 1000000LL);
				}
			} else {
				delete fBottomlineMessenger;
				fBottomlineMessenger = NULL;				
			}
		}
		else {
			if (event->what == B_MOUSE_MOVED) {
				if (event->FindBool("be:set_mouse") == false) {
					int32 theAtom,bits;
					BPoint p = event->FindPoint("where");
					theAtom = (((((int32)p.x) & 0x7FFF) << 15) | (((int32)p.y) & 0x7FFF)) | 0xC0000000;
					bits = atomic_and(fCursor.atom,0x40000000);
					bits &= atomic_or(fCursor.atom,theAtom);
					if (!(bits & 0x40000000)) release_sem(fCursor.sem);
				}
			}
			event->WritePort(fAppServerPort, 0);
		}

		delete (event);
	}
}


void
InputServer::SetNextMethod(
	bool	next)
{
	if (fActiveMethod == NULL)
		return;

	int32 maxMethodIndex = fMethodAddOnList.CountItems() - 1;
	int32 curMethodIndex = fMethodAddOnList.IndexOf(fActiveMethod);
	if (next) {
		curMethodIndex++;
		curMethodIndex = (curMethodIndex > maxMethodIndex) ? 0 : curMethodIndex; 
	}
	else {
		curMethodIndex--;
		curMethodIndex = (curMethodIndex < 0) ? maxMethodIndex : curMethodIndex;
	}

	SetActiveMethod((_BMethodAddOn_ *)fMethodAddOnList.ItemAt(curMethodIndex));
}


void
InputServer::SetActiveMethod(
	_BMethodAddOn_	*method)
{
	_BMethodAddOn_ *oldMethod = fActiveMethod;
	fActiveMethod = NULL;
	oldMethod->MethodActivated(false);
	method->MethodActivated(true);
	fActiveMethod = method;
	
	if (fMethodReplicant != NULL) {
		BMessage notification(msg_UpdateMethod);
		notification.AddInt32("cookie", (int32)fActiveMethod);

		fMethodReplicant->SendMessage(&notification);	
	}
}


void
InputServer::HandleGetKeyboardID(
	BMessage*,
	BMessage	*reply)
{
	reply->AddInt16(IS_KEYBOARD_ID, fKeyboardState.keyboardID);
	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleSetKeyboardLocks(
	BMessage	*command,
	BMessage	*reply)
{
	fKeyboardState.keyMap.lock_settings = command->FindInt32(IS_KEY_LOCKS);
	ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_LOCKS_CHANGED, NULL);

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetKeyInfo(
	BMessage*,
	BMessage	*reply)
{
	reply->AddData(IS_KEY_INFO, B_ANY_TYPE, &fKeyboardState.keyInfo, sizeof(fKeyboardState.keyInfo));
	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetModifiers(
	BMessage*,
	BMessage	*reply)
{
	reply->AddInt32(IS_MODIFIERS, fKeyboardState.modifiers);
	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleSetModifierKey(
	BMessage	*command, 
	BMessage	*reply)
{
	uint32 modifier = command->FindInt32(IS_MODIFIER);
	uint32 key = command->FindInt32(IS_KEY);

	if (modifier & B_CAPS_LOCK)
		fKeyboardState.keyMap.caps_key = key;
	else if (modifier & B_SCROLL_LOCK)
		fKeyboardState.keyMap.scroll_key = key;
	else if (modifier & B_NUM_LOCK)
		fKeyboardState.keyMap.num_key = key;
	else if (modifier & B_MENU_KEY)
		fKeyboardState.keyMap.menu_key = key;
	else if (modifier & B_LEFT_SHIFT_KEY)
		fKeyboardState.keyMap.left_shift_key = key;
	else if (modifier & B_RIGHT_SHIFT_KEY)
		fKeyboardState.keyMap.right_shift_key = key;
	else if (modifier & B_LEFT_COMMAND_KEY)
		fKeyboardState.keyMap.left_command_key = key;
	else if (modifier & B_RIGHT_COMMAND_KEY)
		fKeyboardState.keyMap.right_command_key = key;
	else if (modifier & B_LEFT_CONTROL_KEY)
		fKeyboardState.keyMap.left_control_key = key;
	else if (modifier & B_RIGHT_CONTROL_KEY)
		fKeyboardState.keyMap.right_control_key = key;
	else if (modifier & B_LEFT_OPTION_KEY)
		fKeyboardState.keyMap.left_option_key = key;
	else if (modifier & B_RIGHT_OPTION_KEY)
		fKeyboardState.keyMap.right_option_key = key;

	ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_MAP_CHANGED, NULL);

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetKeyMap(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_KEY_MAP) {
		reply->AddData(IS_KEY_MAP, B_ANY_TYPE, &fKeyboardState.keyMap, sizeof(fKeyboardState.keyMap));
		reply->AddData(IS_KEY_BUFFER, B_ANY_TYPE, fKeyboardState.keyMapChars, fKeyboardState.keyMapCharsSize);
	}
	else {
		if (fKeyboardState.keyMapChars != NULL) {
			free(fKeyboardState.keyMapChars);
			fKeyboardState.keyMapChars = NULL;
			fKeyboardState.keyMapCharsSize = 0;
		}

		init_keymap(&fKeyboardState.keyMap, &fKeyboardState.keyMapChars, &fKeyboardState.keyMapCharsSize);
		ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_MAP_CHANGED, NULL);
	}		
	
	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetKeyRepeatDelay(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_KEY_REPEAT_DELAY)
		reply->AddInt64(IS_DELAY, fKeyboardState.keyRepeatDelay);
	else {
		command->FindInt64(IS_DELAY, &fKeyboardState.keyRepeatDelay);
		ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_REPEAT_DELAY_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetKeyRepeatRate(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_KEY_REPEAT_RATE)
		reply->AddInt32(IS_RATE, fKeyboardState.keyRepeatRate);
	else {
		command->FindInt32(IS_RATE, &fKeyboardState.keyRepeatRate);
		ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_REPEAT_RATE_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);	
}


void
InputServer::HandleGetSetMouseMap(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_MOUSE_MAP)
		reply->AddData(IS_MOUSE_MAP, B_ANY_TYPE, &fMouseState.mouseMap, sizeof(fMouseState.mouseMap));
	else {
		const void	*theMouseMap = NULL;
		ssize_t		theMouseMapSize = 0;
		command->FindData(IS_MOUSE_MAP, B_ANY_TYPE, &theMouseMap, &theMouseMapSize);
		memcpy(&fMouseState.mouseMap, theMouseMap, sizeof(fMouseState.mouseMap)); 

		ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_MAP_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetMouseType(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_MOUSE_TYPE)
		reply->AddInt32(IS_MOUSE_TYPE, fMouseState.mouseType);
	else {
		command->FindInt32(IS_MOUSE_TYPE, &fMouseState.mouseType);
		ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_TYPE_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);	
}


void
InputServer::HandleGetSetMouseSpeed(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_MOUSE_SPEED)
		reply->AddInt32(IS_SPEED, fMouseState.mouseSpeed);
	else {
		command->FindInt32(IS_SPEED, &fMouseState.mouseSpeed);
		ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_SPEED_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetMouseAcceleration(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_MOUSE_ACCELERATION)
		reply->AddInt32(IS_SPEED, fMouseState.mouseAccel);
	else {
		command->FindInt32(IS_SPEED, &fMouseState.mouseAccel);
		ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_ACCELERATION_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleGetSetClickSpeed(
	BMessage	*command,
	BMessage	*reply)
{
	if (command->what == IS_GET_CLICK_SPEED)
		reply->AddInt64(IS_SPEED, fMouseState.clickSpeed);
	else {
		command->FindInt64(IS_SPEED, &fMouseState.clickSpeed);
		ControlDevices(NULL, B_POINTING_DEVICE, B_CLICK_SPEED_CHANGED, NULL);
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleSetMousePosition(
	BMessage	*command,
	BMessage	*reply)
{
	BPoint		where = command->FindPoint(IS_WHERE);
	BMessage	*event = new BMessage(B_MOUSE_MOVED);
	event->AddInt64("when", system_time());
	event->AddFloat("x", where.x / (fScreenX-1));
	event->AddFloat("y", where.y / (fScreenY-1));
	event->AddInt32("buttons", 0);
	event->AddBool("be:set_mouse", true);
	EnqueueDeviceMessage(event);

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleFocusUnfocusIMAwareView(
	BMessage	*command,
	BMessage	*reply)
{
	BMessenger theView;
	command->FindMessenger(IS_VIEW, &theView);

	if (command->what == IS_FOCUS_IM_AWARE_VIEW) {
		if (fInlineAwareView == NULL)
			fInlineAwareView = new BMessenger(theView);
		else 
			*fInlineAwareView = theView;		
	}
	else {
		if (fInlineAwareView != NULL) {
			if (*fInlineAwareView == theView) {
				delete (fInlineAwareView);
				fInlineAwareView = NULL;
			}
		}	
	}

	reply->AddInt32(IS_STATUS, B_NO_ERROR);
}


void
InputServer::HandleSetMethod(
	BMessage	*command)
{
	_BMethodAddOn_ *cookie = (_BMethodAddOn_ *)command->FindInt32("cookie");

	int32 numMethods = fMethodAddOnList.CountItems();
	for (int32 i = 0; i < numMethods; i++) {
		_BMethodAddOn_ *methodAddOn = (_BMethodAddOn_ *)fMethodAddOnList.ItemAt(i);

		if (methodAddOn == cookie) {
			SetActiveMethod(methodAddOn);
			break;
		}	
	}
}


void
InputServer::LockMethodQueue()
{
	if (atomic_add(&fMethodQueueAtom, -1) <= 0) {
		while (acquire_sem(fMethodQueueSem) == B_INTERRUPTED)
			/* nothing */;
	}
}


void
InputServer::UnlockMethodQueue()
{
	if (atomic_add(&fMethodQueueAtom, 1) < 0)
		release_sem(fMethodQueueSem);
}


int32
InputServer::EventLoop(
	void	*arg)
{
	BMessage *event;
	InputServer *app = (InputServer *)arg;

	// load the method add-ons
	// ** don't load in main thread to prevent deadlocks **
	app->InitMethods();

	// load the filter add-ons
	// ** don't load in main thread to prevent deadlocks **
	app->InitFilters();

	// load the device add-ons, also start handler of 
	// input device-related client-side API
	// ** don't load in main thread to prevent deadlocks **
	new DeviceLooper();

	// signal to the app_server that we are ready
	// this moves the cursor to the center of the screen
/*
	BMessage *event = new BMessage(B_MOUSE_MOVED);
	event->AddInt64("when", system_time());
	event->AddFloat("x", 0.5);
	event->AddFloat("y", 0.5);
	event->AddInt32("buttons", 0);
	app->EnqueueDeviceMessage(event);
*/

	// the event generation starts here, so do this at the very last 
	// moment possible (after everything else has been initialized) 
	app->StartStopDevices(NULL, B_POINTING_DEVICE, true);
	app->StartStopDevices(NULL, B_KEYBOARD_DEVICE, true);

	// Added by Alan, there does not seem to be any reason not to start
	// -all- input devices.
	app->StartStopDevices(NULL, B_UNDEFINED_DEVICE, true);

	// signal to the world (Bootscript) that we are ready
	rename_thread(find_thread(NULL), "_input_server_event_loop_");
	sEventLoopRunning = true;

	do {
		BList events;
		while (app->fEventLoopStopSem == B_ERROR) {
			// get next queued event
			bool isMethodEvents = app->GetNextEvents(&events);

/* Useful debug code (to check all incoming keyDown or keyUp events.			
			for (int ii = 0; ii<events.CountItems(); ii++) {
				BMessage	*m;
				int32		index;
				char		*aString;
				uint8		*data8;
				int32		anInt32;
				int64		anInt64;
				ssize_t		sizet;
				
				
				m = (BMessage*)events.ItemAt(ii);
				switch (m->what) {
				case B_KEY_DOWN:
					_sPrintf("#### B_KEY_DOWN message :\n");
					goto and_after_that;
				case B_KEY_UP:
					_sPrintf("#### B_KEY_UP message :\n");
					goto and_after_that;
				case B_UNMAPPED_KEY_DOWN:
					_sPrintf("#### B_UNMAPPED_KEY_DOWN message :\n");
					goto and_after_that;
				case B_UNMAPPED_KEY_UP:		
					_sPrintf("#### B_UNMAPPED_KEY_UP message :\n");
			and_after_that:		
					if (m->FindInt64("when", &anInt64) == B_OK)
						_sPrintf("  when : enabled   ", anInt64);
					else
						_sPrintf("  when : < N/A >   ");
					if (m->FindInt32("modifiers", &anInt32) == B_OK)
						_sPrintf("modifiers : 0x%08x   ", anInt32);
					else
						_sPrintf("modifiers : < N/A >      ");
					if (m->FindInt32("key", &anInt32) == B_OK)
						_sPrintf("key : 0x%02x      ", anInt32);
					else
						_sPrintf("key : < N/A >   ");
					if (m->FindData("states", B_UINT8_TYPE, (void**)&data8, &sizet) == B_OK) {
						_sPrintf("states : 0x%02x%02x%02x%02x-%02x%02x%02x%02x-",
								 data8[15], data8[14], data8[13], data8[12],
								 data8[11], data8[10], data8[9], data8[8]);
						_sPrintf("%02x%02x%02x%02x-%02x%02x%02x%02x\n",
								 data8[7], data8[6], data8[5], data8[4],
								 data8[3], data8[2], data8[1], data8[0]);
					}
					else
						_sPrintf("states : < N/A >\n");
					if (m->FindInt32("raw_char", &anInt32) == B_OK)
						_sPrintf("  raw_char : %c[0x%02x]   ", (char)anInt32, anInt32);
					else
						_sPrintf("  raw_char : < N/A >   ");
					if (m->FindData("byte", B_INT8_TYPE, (void**)&data8, &sizet) == B_OK) {
						_sPrintf("byte :");
						index = 0;
						while (m->FindData("byte", B_INT8_TYPE, index++, (void**)&data8, &sizet) == B_OK)
							_sPrintf(" %c[0x%02x]", (char)*data8, (int32)*data8);
						_sPrintf("   ");
					}
					else
						_sPrintf("byte : < N/A >   ");
					if (m->FindString("bytes", &aString) == B_OK)
						_sPrintf("bytes : \"%s\"   ", aString);
					else
						_sPrintf("bytes : < N/A >   ");
					if (m->FindInt32("be:key_repeat", &anInt32) == B_OK)
						_sPrintf("be:key_repeat : %d\n", anInt32);
					else
						_sPrintf("\n");
					break;
				}
			}
*/
	
			// init the temp mouse coordinates
			app->fTempMouseX = app->fMouseX;
			app->fTempMouseY = app->fMouseY;

			// inspect the event
			if (app->SanitizeEvents(&events)) {

				// let the active method process the event
				if (app->MethodizeEvents(&events, isMethodEvents)) {

					// pass the events onto the filters
					if (app->FilterEvents(&events)) {

						// cache the various mouse/keyboard states
						app->CacheEvents(&events);

						// dispatch and then delete the events
						app->DispatchEvents(&events);
						continue;	// fast path to GetNextEvents()
					}
				}
			}
	
			// delete the unhandled events
			while ((event = (BMessage *)events.RemoveItem((int32)0)) != NULL)
				delete (event);
		}

		release_sem(app->fEventLoopStopSem);	

		if (app->fEventLoopStartSem == B_ERROR)
			break;
		else
			acquire_sem(app->fEventLoopStartSem);
	} while (true); 

	app->fEventLoop = B_ERROR;
	
	return (B_NO_ERROR);
}

