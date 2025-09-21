
#include "input.h"
#include "messages.h"
#include "sam.h"
#include "server.h"
#include "parcel.h"
#include "enums.h"
#include "graphicDevice.h"
#include "cursor.h"

#include <unistd.h>
#include <image.h>

extern long			export_modifiers;
extern long			curs_old_h;
extern long			curs_old_v;

port_id			isEventPort;
static	long	my_curs_h;
static	long	my_curs_v;

inline point npoint(int32 h, int32 v) { point p; p.h = h; p.v = v; return p; };
inline fpoint nfpoint(float h, float v) { fpoint p; p.h = h; p.v = v; return p; };
BParcel *prequeued = NULL;
extern char app_server_name[80];

static port_id launch_input_server()
{
	if (TServer::m_cursorArea < 0) {
		TServer::m_cursorAtom = (int32*)0x90000000;
		TServer::m_cursorArea = Area::CreateArea("isArea",(void**)&TServer::m_cursorAtom,
			B_BASE_ADDRESS,B_PAGE_SIZE,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA);
		if (TServer::m_cursorArea < 0) {
			DebugPrintf(("Failed to create input_server shared area\n"));
			return B_ERROR;
		};
	};
	
	*TServer::m_cursorAtom = 0;
	
	#if MINI_SERVER
	DebugPrintf(("Using teat from parent app_server for input.\n"));
	return create_port(100,"mini_as_port");
	#endif
	
	DebugPrintf(("Launching input_server\n"));

	// the input_server is a BApplication, wait around for the registrar
	DebugPrintf(("Waiting for _roster_thread_...  "));
	while (find_thread("_roster_thread_") == B_NAME_NOT_FOUND) snooze(250000);	
	DebugPrintf(("found _roster_thread_.\n"));

	int32	argC = 1;
	char	**argV = (char **)grMalloc(sizeof(char *) * (argC + 1),"argV",MALLOC_CANNOT_FAIL);
	argV[0] = "/system/servers/input_server";
	argV[1] = NULL;

	char **ne = (char **)environ;
	char asn[256];
	char md[256];
	int32 ecount = 0;
	while (environ[ecount] != NULL) ecount++;
	ecount+=2;
	ne = (char**)malloc(4*ecount);
	for (int32 i=0;i<(ecount-2);i++) {
		if (!strncmp(((char **)environ)[i],"MALLOC_DEBUG",12)) {
			ne[i] = md;
		} else
			ne[i] = ((char **)environ)[i];
	};
	sprintf(md,"MALLOC_DEBUG=1");
	ne[ecount-1] = NULL;

	if (strcmp(app_server_name,"picasso")) {
		sprintf(asn,"APP_SERVER_NAME=%s",app_server_name);
		ne[ecount-2] = asn;
	} else
		ne[ecount-2] = NULL;
	
	thread_id input_server = load_image(argC, (const char **)argV, (const char **)ne);

	grFree(argV);

	if (input_server == B_ERROR) {
		DebugPrintf(("Failed to load_image() input_server!\n"));
		return (B_ERROR);
	}
	else {
		struct {
			sem_id		cursorSem;
			area_id		cursorArea;
		} isData = { TServer::m_cursorSem, TServer::m_cursorArea };

		if (send_data(input_server, 0, &isData, sizeof(isData)) != B_NO_ERROR) {
			DebugPrintf(("send_data() to input_server failed!\n"));
			return (B_ERROR);
		}

		if (resume_thread(input_server) != B_NO_ERROR) {
			DebugPrintf(("resume_thread() of input_server failed!\n"));
			return (B_ERROR);
		}

		struct { port_id rcvPort,sndPort; } ports;
		thread_id	sender = B_ERROR;
		do {
			if (receive_data(&sender, &ports, sizeof(ports)) != B_NO_ERROR) { 
				DebugPrintf(("receive_data() from input_server failed!\n"));
				return (B_ERROR);
			}
		} while (sender != input_server);

		DebugPrintf(("Launched input_server.\n"));
		isEventPort = ports.sndPort;

		BParcel msg(FAKE_MOUSE_MOVED);
		msg.AddFloat("x",0.5);
		msg.AddFloat("y",0.5);
		msg.AddInt32("width",ScreenX());
		msg.AddInt32("height",ScreenY());
		preenqueue_message(&msg);

		return (ports.rcvPort);
	}

	DebugPrintf(("Failed to launch input_server!\n"));
	return (B_ERROR);
}

void preenqueue_message(BParcel *msg)
{
	if (!prequeued) {
		if (msg->WritePort(isEventPort, 0, B_TIMEOUT, 0) == B_OK) return;
	};

	BParcel **put = &prequeued;
	while (*put) put = &(*put)->next;
	*put = new BParcel(*msg);
}

void enqueue_message(const char *buffer, int32 bufferSize)
{
	write_port(the_server->fInputServerPort, 0, buffer, bufferSize);
}

void enqueue_message(BParcel *msg)
{
	msg->WritePort(the_server->fInputServerPort, 0);
}

BParcel * dequeue_message()
{
	int32 msgCount=0;
	int32 messageCode;
	uint32 buttons = the_server->button;
	ssize_t messageSize;
	fpoint location;
	point screenLocation;
	BParcel	*returnMsg = NULL,*old;
	BParcel	**queueEnd = &returnMsg;

	while (!returnMsg || port_count(the_server->fInputServerPort)) {
		messageSize = port_buffer_size(the_server->fInputServerPort);
		if (messageSize < 0) {
			the_server->fInputServerPort = launch_input_server();
		} else {
			BParcel *event = new BParcel;
			status_t err = event->ReadPort(the_server->fInputServerPort,
										   messageSize, &messageCode);
			if (err != B_OK) {
				DebugPrintf(("app_server error reading input port: %s\n",
							strerror(err)));
				delete event;
				continue;
			}
			
			switch (event->what) {
				case FAKE_MOUSE_MOVED:
				{
					fpoint where = nfpoint(curs_old_h,curs_old_v);
					event->what = B_MOUSE_MOVED;
					event->AddPoint("where",where);
					event->AddPoint("screen_where",where);
					event->AddInt32("buttons",the_server->button);
					event->AddInt32("modifiers",the_server->last_modifiers);
					event->AddInt32(B_MOUSE_CURSOR_NEEDED, B_CURSOR_MAYBE_NEEDED);
					event->SetLocation(where);
					event->SetEventMask(etPointerEvent);
					the_server->user_action();
					*queueEnd = event;
					queueEnd = &event->next;
					break;
				}
				case B_MOUSE_DOWN:
				case B_MOUSE_UP:
				case B_MOUSE_MOVED:
				{
					uint32 butt = event->FindInt32("buttons");
					int32 cursor = event->FindInt32(B_MOUSE_CURSOR_NEEDED);
					if (cursor == B_CURSOR_NEEDED)
						the_cursor->SetCursorState(butt != 0 ? true : false, true);
					else if (cursor == B_CURSOR_NOT_NEEDED)
						the_cursor->SetCursorState(butt != 0 ? true : false, false);
					
					the_server->user_action();
					if (((event->what == B_MOUSE_DOWN) && the_server->button) ||
						((event->what == B_MOUSE_UP) && butt)) {
						event->what = B_MOUSE_MOVED;
					};
			
					event->FindPoint("where", &location);
					screenLocation.h = (int32)location.h;
					screenLocation.v = (int32)location.v;
					event->AddPoint("screen_where",location);
					event->SetLocation(location);
					event->SetScreenLocation(screenLocation);
					event->SetEventMask(etPointerEvent);

					export_modifiers = the_server->last_modifiers = event->FindInt32("modifiers");
					curs_old_h = the_server->curs_h = (int32)location.h;
					curs_old_v = the_server->curs_v = (int32)location.v;

					the_server->button = butt;
					#if MINI_SERVER
					if (event->what == B_MOUSE_MOVED) {
						// We are not attached to the input_server, so we need to
						// fake the input_server's cursor location communication.
						int32 x = screenLocation.h;
						int32 y = screenLocation.v;
						if (x < 0) x = 0;
						if (x > 0x7FFF) x = 0x7FFF;
						if (y < 0) y = 0;
						if (y > 0x7FFF) y = 0x7FFF;
						int32 theAtom,bits;
						theAtom = ((x << 15) | y) | 0xC0000000;
						bits = atomic_and(TServer::m_cursorAtom,0x40000000);
						bits &= atomic_or(TServer::m_cursorAtom,theAtom);
						if (!(bits & 0x40000000)) release_sem(TServer::m_cursorSem);
					}
					#endif
					
					*queueEnd = event;
					queueEnd = &event->next;
					break;
				}
		
				case B_KEY_DOWN:
				case B_KEY_UP:
				{	
					event->SetEventMask(etFocusedEvent);
					the_server->user_action();
					*queueEnd = event;
					queueEnd = &event->next;
					break;
				}

				case B_MODIFIERS_CHANGED:
					export_modifiers = the_server->last_modifiers = event->FindInt32("modifiers");
					event->SetEventMask(etFocusedEvent);
					the_server->user_action();
					*queueEnd = event;
					queueEnd = &event->next;
					break;
		
				default:
//					DebugPrintf(("UNKNOWN message from input_server: '%.4s'\n", &event->what));
					event->SetEventMask(etOtherEvent);
					the_server->user_action();
					*queueEnd = event;
					queueEnd = &event->next;
					break;
			};
		};
	};

	/* Try to send any messages that were prequeued */
	while (prequeued) {
		if (prequeued->WritePort(isEventPort, 0, B_TIMEOUT, 0) != B_OK) break;
		old = prequeued;
		prequeued = prequeued->next;
		delete old;
	};

	return returnMsg;
}
