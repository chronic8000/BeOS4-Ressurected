	
#ifndef INPUT_H
#define INPUT_H

#include <kernel/OS.h>
#include <support2/MessageList.h>
#include <support2/String.h>
#include <support2/Locker.h>
#include <render2/Point.h>
#include <raster2/RasterPoint.h>

#define FAKE_MOUSE_MOVED '_FMM'
#define SHIFT_KEY			0x00000001
#define COMMAND_KEY			0x00000002
#define CONTROL_KEY			0x00000004
#define CAPS_LOCK			0x00000008
#define SCROLL_LOCK			0x00000010
#define NUM_LOCK			0x00000020
#define OPTION_KEY			0x00000040
#define MENU_KEY			0x00000080
#define LEFT_SHIFT_KEY		0x00000100
#define RIGHT_SHIFT_KEY		0x00000200
#define LEFT_COMMAND_KEY	0x00000400
#define RIGHT_COMMAND_KEY	0x00000800
#define LEFT_CONTROL_KEY	0x00001000
#define RIGHT_CONTROL_KEY	0x00002000
#define LEFT_OPTION_KEY		0x00004000
#define RIGHT_OPTION_KEY	0x00008000

using B::Support2::atom_ptr;

class BParcel;
class BPicassoServer;

namespace B {
	namespace Support2 {
		class BMessageList;
	}
}


class InputServerLink
{
public:
		InputServerLink(const char *server_name, const atom_ptr<BPicassoServer> &server);
	virtual ~InputServerLink();

	void enqueue_message(::BParcel *);
	void preenqueue_message(::BParcel *);
	void dequeue_message(B::Support2::BMessageList&);

	virtual void UserAction();

public:
	static	area_id				m_cursorArea;
	static	int32 *				m_cursorAtom;
	static	sem_id				m_cursorSem;

private:
			port_id				launch_input_server();
	static	int32 				input_thread(InputServerLink *that);
			int32				InputThread();

	B::Support2::BLocker		m_lock;
	B::Support2::BString		m_serverName;
	atom_ptr<BPicassoServer>	m_server;
	port_id						m_inputServerPort;
	port_id						m_isEventPort;
	B::Support2::BMessageList	m_queue;
	B::Render2::BPoint			m_cursPosition;
	int32						m_lastModifiers;
	int32						m_buttons;
};

extern InputServerLink *gInputServerLink;

#endif
