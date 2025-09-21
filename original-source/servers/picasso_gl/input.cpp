#include <unistd.h>
#include <image.h>

#include <support2/MessageList.h>
#include <support2/StdIO.h>
#include <support2/String.h>
#include <support2/Value.h>
#include <support2/Looper.h>
#include <app/AppDefs.h>
#include "old/Message.h"

#include "enums.h"
#include "input.h"
#include "parcel.h"
#include "server.h"

using B::Support2::BTeam;
using B::Support2::BLooper;
using B::Support2::BValue;
using B::Support2::BMessageList;
using B::Support2::BString;
using B::Support2::bout;
using B::Support2::bser;
using B::Support2::endl;
using B::Render2::BPoint;
using B::Raster2::BRasterPoint;

//todo: I don't understand, this should not be needed thanks to unistd.h
//extern char *environ;

// These are shared with the input_server, that's why they're not part of
// the InputServerLink class.
//area_id	InputServerLink::m_cursorArea = -1;
//int32 *	InputServerLink::m_cursorAtom = NULL;
//sem_id	InputServerLink::m_cursorSem = -1;

// --------------------------------------------------------

InputServerLink::InputServerLink(const char *server_name, const atom_ptr<BPicassoServer> &server)
	:	m_serverName(BString(server_name)),
		m_server(server),
		m_cursPosition(BPoint(0,0)),
		m_lastModifiers(0),
		m_buttons(0)
{
//	if (InputServerLink::m_cursorArea != -1) {
//		debugger("only one InputServerLink allowed");
//	}
//	// Init the input connection
//	m_cursorSem = create_sem(0, "PicassoIsCursorSem");
	m_inputServerPort = launch_input_server();

	// Launch the input thread

	thread_id tid;
	tid = BLooper::SpawnThread((thread_entry)input_thread,"input",B_REAL_TIME_DISPLAY_PRIORITY,this);
	resume_thread(tid);
}

InputServerLink::~InputServerLink()
{
}

void InputServerLink::UserAction()
{
}

void InputServerLink::enqueue_message(BParcel *msg)
{
	m_lock.Lock();
	bool sig = m_queue.IsEmpty();
	m_queue.EnqueueMessage(msg);
	m_lock.Unlock();

	if (sig) write_port(m_inputServerPort,0,0,0);
}

void InputServerLink::dequeue_message(BMessageList& returnMsg)
{
	int32 messageCode;

	while (returnMsg.IsEmpty() || port_count(m_inputServerPort)) {
		ssize_t messageSize = port_buffer_size(m_inputServerPort);
		if (messageSize < 0) {
			m_inputServerPort = launch_input_server();
		} else {
			B::Old::BMessage m;
			status_t err = m.ReadPort(m_inputServerPort, messageSize, &messageCode);
			if ((err != B_OK) && (err != B_NOT_A_MESSAGE)) {
				bser << "app_server error reading input port: " << strerror(err) << endl;
				continue;
			}
						
			// Notify a user action
			UserAction();
			
			while (true) {
				m_lock.Lock();
				if (err != B_NOT_A_MESSAGE) m_queue.EnqueueMessage(new BParcel(m.AsValue()));
				BParcel *event = (BParcel*)m_queue.DequeueMessage();
				m_lock.Unlock();
				
				if (!event) break;
	
				BValue value = event->Data();
				switch (event->What()) {
					case FAKE_MOUSE_MOVED:
					{
						BPoint where(m_cursPosition.x, m_cursPosition.y);
						event->SetWhat(B_MOUSE_MOVED);
						value.Overlay("where",			where);
						value.Overlay("screen_where",	where);
						value.Overlay("buttons",		m_buttons);
						value.Overlay("modifiers",		m_lastModifiers);
						break;
					}
					case B_MOUSE_DOWN:
					case B_MOUSE_UP:
					case B_MOUSE_MOVED:
					{
						uint32 butt = value["buttons"].AsInteger();
	
						if (((event->What() == B_MOUSE_DOWN) && m_buttons) ||
							((event->What() == B_MOUSE_UP) && butt)) {
							event->SetWhat(B_MOUSE_MOVED);
						}
				
						BPoint location(value["where"]);
						value.Overlay("screen_where", location);					
						
	//					BRasterPoint screenLocation((int32)location.x, (int32)location.y);
						m_cursPosition.x = (int32)location.x;
						m_cursPosition.y = (int32)location.y;
						m_lastModifiers = value["modifiers"].AsInteger();
						m_buttons = butt;
	
	//					#if MINI_SERVER
	//					if (event->What() == B_MOUSE_MOVED) {
	//						// We are not attached to the input_server, so we need to
	//						// fake the input_server's cursor location communication.
	//						int32 x = screenLocation.x;
	//						int32 y = screenLocation.y;
	//						if (x < 0) x = 0;
	//						if (x > 0x7FFF) x = 0x7FFF;
	//						if (y < 0) y = 0;
	//						if (y > 0x7FFF) y = 0x7FFF;
	//						int32 theAtom,bits;
	//						theAtom = ((x << 15) | y) | 0xC0000000;
	//						bits = atomic_and(InputServerLink::m_cursorAtom,0x40000000);
	//						bits &= atomic_or(InputServerLink::m_cursorAtom,theAtom);
	//						if (!(bits & 0x40000000)) release_sem(InputServerLink::m_cursorSem);
	//					}
	//					#endif
						break;
					}
			
					case B_KEY_DOWN:
					case B_KEY_UP:
						break;
	
					case B_MODIFIERS_CHANGED:
						m_lastModifiers = value["modifiers"].AsInteger();
						break;
			
					default:
						break;
				}
				event->SetData(value);
				returnMsg.EnqueueMessage(event);
			}
		}
	}
/*
	// Try to send any messages that were prequeued
	while (m_prequeued.IsEmpty() == false) {
		B::Support2::BMessage *msg = m_prequeued.RemoveHead();
		B::Old::BMessage m(msg->Data());
		if (m.WritePort(m_isEventPort, 0, B_TIMEOUT, 0) != B_OK) break;
	}
*/
}


// --------------------------------------------------------
// #pragma mark -

port_id InputServerLink::launch_input_server()
{		
#if MINI_SERVER
	bser << "Using teat from parent app_server for input" << endl;
	return create_port(100,"mini_as_port");
#endif

debugger("picasso input ONLY in miniserver mode!");

//	if (InputServerLink::m_cursorArea < 0) {
//		InputServerLink::m_cursorAtom = (int32*)0x90000000;
//		InputServerLink::m_cursorArea = Area::CreateArea("isArea",(void**)&InputServerLink::m_cursorAtom,
//			B_BASE_ADDRESS,B_PAGE_SIZE,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA);
//		if (InputServerLink::m_cursorArea < 0) {
//			bser << "Failed to create input_server shared area" << endl;
//			return B_ERROR;
//		}
//	}
//	*InputServerLink::m_cursorAtom = 0;
//
//	bser << "Launching input_server" << endl;
//
//	// the input_server is a BApplication, wait around for the registrar
//	DebugPrintf(("Waiting for _roster_thread_...  "));
//	while (find_thread("_roster_thread_") == B_NAME_NOT_FOUND) snooze(250000);	
//	DebugPrintf(("found _roster_thread_.\n"));
//
//	int32	argC = 1;
//	char	**argV = (char **)grMalloc(sizeof(char *) * (argC + 1),"argV",MALLOC_CANNOT_FAIL);
//	argV[0] = "/system/servers/input_server";
//	argV[1] = NULL;
//
//	char **ne = (char **)environ;
//	char asn[256];
//	char md[256];
//	int32 ecount = 0;
//	while (environ[ecount] != '\0') ecount++;
//	ecount+=2;
//	ne = (char**)malloc(4*ecount);
//	for (int32 i=0;i<(ecount-2);i++) {
//		if (!strncmp(((char **)environ)[i],"MALLOC_DEBUG",12)) {
//			ne[i] = md;
//		} else
//			ne[i] = ((char **)environ)[i];
//	};
//	sprintf(md,"MALLOC_DEBUG=1");
//	ne[ecount-1] = NULL;
//
//	if (strcmp(m_serverName.String(), "picasso")) {
//		sprintf(asn,"APP_SERVER_NAME=%s", m_serverName.String());
//		ne[ecount-2] = asn;
//	} else
//		ne[ecount-2] = NULL;
//	
//	thread_id input_server = load_image(argC, (const char **)argV, (const char **)ne);
//
//	grFree(argV);
//
//	if (input_server == B_ERROR) {
//		DebugPrintf(("Failed to load_image() input_server!\n"));
//		return (B_ERROR);
//	}
//	else {
//		struct {
//			sem_id		cursorSem;
//			area_id		cursorArea;
//		} isData = { InputServerLink::m_cursorSem, InputServerLink::m_cursorArea };
//
//		if (send_data(input_server, 0, &isData, sizeof(isData)) != B_NO_ERROR) {
//			DebugPrintf(("send_data() to input_server failed!\n"));
//			return (B_ERROR);
//		}
//
//		if (resume_thread(input_server) != B_NO_ERROR) {
//			DebugPrintf(("resume_thread() of input_server failed!\n"));
//			return (B_ERROR);
//		}
//
//		struct { port_id rcvPort,sndPort; } ports;
//		thread_id	sender = B_ERROR;
//		do {
//			if (receive_data(&sender, &ports, sizeof(ports)) != B_NO_ERROR) { 
//				DebugPrintf(("receive_data() from input_server failed!\n"));
//				return (B_ERROR);
//			}
//		} while (sender != input_server);
//
//		DebugPrintf(("Launched input_server.\n"));
//		m_isEventPort = ports.sndPort;
///*
//		BValue v;
//		v.Overlay("x",0.5);
//		v.Overlay("y",0.5);
//		v.Overlay("width", ScreenX());
//		v.Overlay("height", ScreenY());
//		BParcel msg(FAKE_MOUSE_MOVED);
//		msg.SetData(v);
//		preenqueue_message(&msg);
//*/
//		return (ports.rcvPort);
//	}
//
//	DebugPrintf(("Failed to launch input_server!\n"));
//	return (B_ERROR);
}

// --------------------------------------------------------
// #pragma mark -

int32 InputServerLink::input_thread(InputServerLink *that)
{
	return that->InputThread();
}

int32 InputServerLink::InputThread()
{
	while (true) {
		BMessageList message_list;
		dequeue_message(message_list);
		while (message_list.IsEmpty() == false) {
			BParcel *msg = static_cast<BParcel *>(message_list.RemoveHead());
			m_server->DispatchEvent(*msg);
//			snooze(20000); // Slow down event processing to expose bugs.
			delete msg;
		}
	}
	return 0;
}
