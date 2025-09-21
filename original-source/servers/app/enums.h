
#ifndef ENUMS_H
#define ENUMS_H

enum {
	B_ENTERED=0,
	B_INSIDE,
	B_EXITED,
	B_OUTSIDE
};

enum {
	etPointerEvent	= 0x00000001,
	etFocusedEvent 	= 0x00000002,
	etOtherEvent	= 0x00000004,
	etHack			= 0x80000000
};

enum {
	rtThin			= 0x00000001,
	rtBroadcast 	= 0x00000002
};


enum {
	aoLockWindowFocus	= 0x00000001,
	aoUsurpViewFocus	= 0x00000002,
	aoNoPointerHistory	= 0x00000004
};

#endif
