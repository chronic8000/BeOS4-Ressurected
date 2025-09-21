#ifndef _BOTTOMLINEINPUT_H
#define _BOTTOMLINEINPUT_H

#include <Window.h>


class BTextView;
class BFont;


class BottomlineWindow : public BWindow {
public:
						BottomlineWindow(const BFont *font);

	virtual void		FrameMoved(BPoint newLocation);
	virtual void		MessageReceived(BMessage *message);

	bool				HandleInputMethodEvent(BMessage *event, BList *events);

private:
	void				GenerateKeyDowns(const char *string, BList *events);

private:
	BTextView*			fTextView;
	static BPoint		sLocation;
};


#endif
