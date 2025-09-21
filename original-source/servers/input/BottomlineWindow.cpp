#include "BottomlineWindow.h"

#include <Input.h>
#include <Screen.h>
#include <TextView.h>

#include <malloc.h>
#include <string.h>

#define BOTTOMLINE_EVENTS	'_BLE'	/* used to send events from the bottomline to the input server */

inline int32
utf8_char_len(
	uchar	c)
{ 
	return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1);
}


const window_look	kVerticalFloatingWindowLook	= (window_look)25;
const float		kWindowWidth			= 500.0;
const float		kScreenLeftOffset		= 40.0;
const float		kScreenBottomOffset		= 120.0;
const float		kTextInset				= 2.0;
const BPoint	kUninitializedLocation(-10000.0, -10000.0);


BPoint BottomlineWindow::sLocation(kUninitializedLocation);


BottomlineWindow::BottomlineWindow(
	const BFont	*font)
		: BWindow(BRect(0.0, 0.0, 0.0, 0.0), B_EMPTY_STRING, 
				  kVerticalFloatingWindowLook/*B_FLOATING_WINDOW_LOOK*/, B_FLOATING_ALL_WINDOW_FEEL, 
				  B_WILL_ACCEPT_FIRST_CLICK | B_NOT_CLOSABLE | B_NOT_V_RESIZABLE | 
				  B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS)
{
	float minH = 0.0;
	float maxH = 0.0;
	float minV = 0.0;
	float maxV = 0.0;
	GetSizeLimits(&minH, &maxH, &minV, &maxV);

	font_height fontH;
	font->GetHeight(&fontH);
	float height = (kTextInset * 2) + ceil(fontH.ascent + fontH.descent + fontH.leading);

	SetSizeLimits(kWindowWidth, maxH, height, maxV);
	ResizeTo(kWindowWidth, height);

	BPoint windowLocation(sLocation);
	if (windowLocation == kUninitializedLocation) {
		BRect screenFrame = BScreen().Frame();
		windowLocation.x = screenFrame.left + kScreenLeftOffset;
		windowLocation.y = screenFrame.bottom - kScreenBottomOffset;
	}
	MoveTo(windowLocation);

	BRect textViewBounds = Bounds();
	BRect textRect = textViewBounds;
	textRect.InsetBy(kTextInset, kTextInset);
	fTextView = new BTextView(textViewBounds, B_EMPTY_STRING, textRect, font, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	fTextView->SetFlags(fTextView->Flags() & ~B_INPUT_METHOD_AWARE);
	AddChild(fTextView);

	// fool the TextView into thinking that its window is 'active'
	// despite the window's B_AVOID_FOCUS flag 
	// this is a hack
	fTextView->MakeFocus(true);
	fTextView->WindowActivated(true);

	Show();
}


void
BottomlineWindow::FrameMoved(
	BPoint	newLocation)
{
	sLocation = newLocation;
}


void
BottomlineWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case B_INPUT_METHOD_EVENT:
		{
			BMessage reply(BOTTOMLINE_EVENTS);
			BList events;
			bool r = HandleInputMethodEvent(message, &events);
			if (!r) {
				reply.AddBool("cancel_im", false);
			}
			BMessage *m = NULL;
			while ((m = (BMessage *)events.RemoveItem((int32)0)) != NULL) {
				reply.AddMessage("generated_event", m);
			}
			message->SendReply(&reply, (BHandler*)NULL, 1000000LL);
		}
		break;
	default:
		BWindow::MessageReceived(message);
	}
}


bool
BottomlineWindow::HandleInputMethodEvent(
	BMessage	*event,
	BList		*events)
{
	bool	result = true;
	uint32	opcode = 0;
	event->FindInt32("be:opcode", (int32 *)&opcode);

	switch (opcode) {
		case B_INPUT_METHOD_CHANGED:
		{
			const char *string = NULL;
			event->FindString("be:string", &string);

			if ((string == NULL) || (strlen(string) < 1)) {
				result = false;
				break;
			}

			bool confirmed = false;
			event->FindBool("be:confirmed", &confirmed);

			if (confirmed)
				GenerateKeyDowns(string, events);	
			break;
		}

		case B_INPUT_METHOD_STOPPED:
			// return false so that the window will get closed
			result = false;
			break;

		default:
			break;
	}

	PostMessage(event, fTextView);
	return (result);
}


void
BottomlineWindow::GenerateKeyDowns(
	const char	*string,
	BList		*events)
{
	int32 textLen = strlen(string);
	if (textLen < 1)
		return;

	const int32	kZeros[4] = {0, 0, 0, 0};
	char		*text = strdup(string);
	bigtime_t	when = system_time();

	int32 offset = 0;
	int32 numEventsGenerated = 0;
	while (offset < textLen) {
		int32 charLen = utf8_char_len(text[offset]);

		BMessage *event = new BMessage(B_KEY_DOWN);
		event->AddInt64("when", when);
		event->AddInt32("key", 0);
		event->AddInt32("modifiers", 0);
		event->AddInt32("raw_char", B_RETURN);
		event->AddData("states", B_UINT8_TYPE, kZeros, sizeof(kZeros));	

		for (int32 i = 0; i < charLen; i++)
			event->AddInt8("byte", text[offset + i]);
		
		char saveChar = text[offset + charLen];
		text[offset + charLen] = '\0';
		event->AddString("bytes", text + offset);
		text[offset + charLen] = saveChar;

		events->AddItem(event, numEventsGenerated);

		offset += charLen;
		numEventsGenerated++;
	}

	free(text);
}

