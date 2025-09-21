
#ifndef INPUT_H
#define INPUT_H

#include "messages.h"

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


class BParcel;

void		enqueue_message(BParcel *);
void		enqueue_message(const char *, int32);
void		preenqueue_message(BParcel *);
BParcel *	dequeue_message();


#endif
